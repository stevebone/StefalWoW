/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "WorldSocket.h"
#include "AuthenticationPackets.h"
#include "BattlenetRpcErrorCodes.h"
#include "CharacterPackets.h"
#include "CryptoHash.h"
#include "CryptoRandom.h"
#include "DatabaseEnv.h"
#include "Errors.h"
#include "GameTime.h"
#include "HMAC.h"
#include "IPLocation.h"
#include "IpBanCheckConnectionInitializer.h"
#include "PacketLog.h"
#include "ProtobufJSON.h"
#include "RealmList.h"
#include "RBAC.h"
#include "RealmList.pb.h"
#include "ScriptMgr.h"
#include "SessionKeyGenerator.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include <zlib.h>
#include "Config.h"

#pragma pack(push, 1)

struct CompressedWorldPacket
{
    uint32 UncompressedSize;
    uint32 UncompressedAdler;
    uint32 CompressedAdler;
};

#pragma pack(pop)

uint32 const WorldSocket::MinSizeForCompression = 0x400;

std::array<uint8, 32> const WorldSocket::AuthCheckSeed = { 0xDE, 0x3A, 0x2A, 0x8E, 0x6B, 0x89, 0x52, 0x66, 0x88, 0x9D, 0x7E, 0x7A, 0x77, 0x1D, 0x5D, 0x1F,
    0x4E, 0xD9, 0x0C, 0x23, 0x9B, 0xCD, 0x0E, 0xDC, 0xD2, 0xE8, 0x04, 0x3A, 0x68, 0x64, 0xC7, 0xB0 };
std::array<uint8, 32> const WorldSocket::SessionKeySeed = { 0xE8, 0x1E, 0x8B, 0x59, 0x27, 0x62, 0x1E, 0xAA, 0x86, 0x15, 0x18, 0xEA, 0xC0, 0xBF, 0x66, 0x8C,
    0x6D, 0xBF, 0x83, 0x93, 0xBC, 0xAA, 0x80, 0x52, 0x5B, 0x1E, 0xDC, 0x23, 0xA0, 0x12, 0xB7, 0x50 };
std::array<uint8, 32> const WorldSocket::ContinuedSessionSeed = { 0x56, 0x5C, 0x61, 0x9C, 0x48, 0x3A, 0x52, 0x1F, 0x61, 0x5D, 0x05, 0x49, 0xB2, 0x9A, 0x39, 0xBF,
    0x4B, 0x97, 0xB0, 0x1B, 0xF9, 0x6C, 0xDE, 0xD6, 0x80, 0x1D, 0xAB, 0x26, 0x02, 0xA9, 0x9B, 0x9D };
std::array<uint8, 32> const WorldSocket::EncryptionKeySeed = { 0x71, 0xC9, 0xED, 0x5A, 0xA7, 0x0E, 0x4D, 0xFF, 0x4C, 0x36, 0xA6, 0x5A, 0x3E, 0x46, 0x8A, 0x4A,
    0x5D, 0xA1, 0x48, 0xC8, 0x30, 0x47, 0x4A, 0xDE, 0xF6, 0x0D, 0x6C, 0xBE, 0x6F, 0xE4, 0x55, 0x73 };

WorldSocket::WorldSocket(Trinity::Net::IoContextTcpSocket&& socket) : BaseSocket(std::move(socket)),
    _type(CONNECTION_TYPE_REALM), _key(0), _serverChallenge(), _sessionKey(), _encryptKey(), _OverSpeedPings(0),
    _worldSession(nullptr), _authed(false), _canRequestHotfixes(true), _headerBuffer(sizeof(IncomingPacketHeader)), _sendBufferSize(4096), _compressionStream(nullptr)
{
}

WorldSocket::~WorldSocket()
{
    if (_compressionStream)
    {
        deflateEnd(_compressionStream);
        delete _compressionStream;
    }
}

struct WorldSocketProtocolInitializer final : Trinity::Net::SocketConnectionInitializer
{
    static constexpr std::string_view ServerConnectionInitialize = "WORLD OF WARCRAFT CONNECTION - SERVER TO CLIENT - V2\n";
    static constexpr std::string_view ClientConnectionInitialize = "WORLD OF WARCRAFT CONNECTION - CLIENT TO SERVER - V2\n";

    explicit WorldSocketProtocolInitializer(WorldSocket* socket) : _socket(socket) { }

    void Start() override
    {
        _packetBuffer.Resize(ClientConnectionInitialize.length());

        AsyncRead();

        MessageBuffer initializer;
        initializer.Write(ServerConnectionInitialize.data(), ServerConnectionInitialize.length());

        // - IoContext.run thread, safe.
        _socket->QueuePacket(std::move(initializer));
    }

    void AsyncRead()
    {
        _socket->AsyncRead(
            [socketRef = _socket->weak_from_this(), self = static_pointer_cast<WorldSocketProtocolInitializer>(this->shared_from_this())]
            {
                if (!socketRef.expired())
                    return self->ReadHandler();

                return Trinity::Net::SocketReadCallbackResult::Stop;
            });
    }

    Trinity::Net::SocketReadCallbackResult ReadHandler();

    void HandleDataReady();

private:
    WorldSocket* _socket;
    MessageBuffer _packetBuffer;
};

void WorldSocket::Start()
{
    // build initializer chain
    std::array<std::shared_ptr<Trinity::Net::SocketConnectionInitializer>, 3> initializers =
    { {
        std::make_shared<Trinity::Net::IpBanCheckConnectionInitializer<WorldSocket>>(this),
        std::make_shared<WorldSocketProtocolInitializer>(this),
        std::make_shared<Trinity::Net::ReadConnectionInitializer<WorldSocket>>(this),
    } };

    Trinity::Net::SocketConnectionInitializer::SetupChain(initializers)->Start();
}

Trinity::Net::SocketReadCallbackResult WorldSocketProtocolInitializer::ReadHandler()
{
    MessageBuffer& packet = _socket->GetReadBuffer();
    if (packet.GetActiveSize() > 0 && _packetBuffer.GetRemainingSpace() > 0)
    {
        // need to receive the header
        std::size_t readHeaderSize = std::min(packet.GetActiveSize(), _packetBuffer.GetRemainingSpace());
        _packetBuffer.Write(packet.GetReadPointer(), readHeaderSize);
        packet.ReadCompleted(readHeaderSize);

        if (_packetBuffer.GetRemainingSpace() == 0)
        {
            HandleDataReady();
            return Trinity::Net::SocketReadCallbackResult::Stop;
        }

        // Couldn't receive the whole header this time.
        ASSERT(packet.GetActiveSize() == 0);
    }

    return Trinity::Net::SocketReadCallbackResult::KeepReading;
}

void WorldSocketProtocolInitializer::HandleDataReady()
{
    try
    {
        ByteBuffer buffer(std::move(_packetBuffer).Release());
        if (buffer.ReadString(ClientConnectionInitialize.length()) != ClientConnectionInitialize)
        {
            _socket->CloseSocket();
            return;
        }
    }
    catch (ByteBufferException const& ex)
    {
        TC_LOG_ERROR("network", "WorldSocket::InitializeHandler ByteBufferException {} occured while parsing initial packet from {}",
            ex.what(), _socket->GetRemoteIpAddress());
        _socket->CloseSocket();
        return;
    }

    if (!_socket->InitializeCompression())
        return;

    _socket->SendAuthSession();
    if (next)
        next->Start();
}

bool WorldSocket::InitializeCompression()
{
    _compressionStream = new z_stream();
    _compressionStream->zalloc = (alloc_func)nullptr;
    _compressionStream->zfree = (free_func)nullptr;
    _compressionStream->opaque = (voidpf)nullptr;
    _compressionStream->avail_in = 0;
    _compressionStream->next_in = nullptr;
    int32 z_res = deflateInit2(_compressionStream, sWorld->getIntConfig(CONFIG_COMPRESSION), Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);
    if (z_res != Z_OK)
    {
        CloseSocket();
        TC_LOG_ERROR("network", "Can't initialize packet compression (zlib: deflateInit) Error code: {} ({})", z_res, zError(z_res));
        return false;
    }

    return true;
}

bool WorldSocket::Update()
{
    EncryptablePacket* queued;
    MessageBuffer buffer(_sendBufferSize);
    while (_bufferQueue.Dequeue(queued))
    {
        uint32 packetSize = queued->size() + 4 /*opcode*/;
        if (packetSize > MinSizeForCompression && queued->NeedsEncryption())
            packetSize = deflateBound(_compressionStream, packetSize) + sizeof(CompressedWorldPacket);

        // Flush current buffer if too small for next packet
        if (buffer.GetRemainingSpace() < packetSize + sizeof(PacketHeader))
        {
            QueuePacket(std::move(buffer));
            buffer.Resize(_sendBufferSize);
        }

        if (buffer.GetRemainingSpace() >= packetSize + sizeof(PacketHeader))
            WritePacketToBuffer(*queued, buffer);
        else    // single packet larger than _sendBufferSize
        {
            MessageBuffer packetBuffer(packetSize + sizeof(PacketHeader));
            WritePacketToBuffer(*queued, packetBuffer);
            QueuePacket(std::move(packetBuffer));
        }

        delete queued;
    }

    if (buffer.GetActiveSize() > 0)
        QueuePacket(std::move(buffer));

    if (!BaseSocket::Update())
        return false;

    _queryProcessor.ProcessReadyCallbacks();

    return true;
}

void WorldSocket::SendAuthSession()
{
    Trinity::Crypto::GetRandomBytes(_serverChallenge);

    WorldPackets::Auth::AuthChallenge challenge;
    challenge.Challenge = _serverChallenge;
    memcpy(challenge.DosChallenge.data(), Trinity::Crypto::GetRandomBytes<32>().data(), 32);
    challenge.DosZeroBits = 1;

    SendPacketAndLogOpcode(*challenge.Write());
}

void WorldSocket::OnClose()
{
    {
        std::lock_guard<std::mutex> sessionGuard(_worldSessionLock);
        _worldSession = nullptr;
    }
}

Trinity::Net::SocketReadCallbackResult WorldSocket::ReadHandler()
{
    MessageBuffer& packet = GetReadBuffer();
    while (packet.GetActiveSize() > 0)
    {
        if (_headerBuffer.GetRemainingSpace() > 0)
        {
            // need to receive the header
            std::size_t readHeaderSize = std::min(packet.GetActiveSize(), _headerBuffer.GetRemainingSpace());
            _headerBuffer.Write(packet.GetReadPointer(), readHeaderSize);
            packet.ReadCompleted(readHeaderSize);

            if (_headerBuffer.GetRemainingSpace() > 0)
            {
                // Couldn't receive the whole header this time.
                ASSERT(packet.GetActiveSize() == 0);
                break;
            }

            // We just received nice new header
            if (!ReadHeaderHandler())
            {
                CloseSocket();
                return Trinity::Net::SocketReadCallbackResult::Stop;
            }
        }

        // We have full read header, now check the data payload
        if (_packetBuffer.GetRemainingSpace() > 0)
        {
            // need more data in the payload
            std::size_t readDataSize = std::min(packet.GetActiveSize(), _packetBuffer.GetRemainingSpace());
            _packetBuffer.Write(packet.GetReadPointer(), readDataSize);
            packet.ReadCompleted(readDataSize);

            if (_packetBuffer.GetRemainingSpace() > 0)
            {
                // Couldn't receive the whole data this time.
                ASSERT(packet.GetActiveSize() == 0);
                break;
            }
        }

        // just received fresh new payload
        ReadDataHandlerResult result = ReadDataHandler();
        _headerBuffer.Reset();
        if (result != ReadDataHandlerResult::Ok)
        {
            if (result != ReadDataHandlerResult::WaitingForQuery)
                CloseSocket();

            return Trinity::Net::SocketReadCallbackResult::Stop;
        }
    }

    return Trinity::Net::SocketReadCallbackResult::KeepReading;
}

void WorldSocket::QueueQuery(QueryCallback&& queryCallback)
{
    _queryProcessor.AddCallback(std::move(queryCallback));
}

void WorldSocket::SetWorldSession(WorldSession* session)
{
    std::lock_guard<std::mutex> sessionGuard(_worldSessionLock);
    _worldSession = session;
    _authed = true;
}

bool WorldSocket::ReadHeaderHandler()
{
    ASSERT(_headerBuffer.GetActiveSize() == sizeof(IncomingPacketHeader), "Header size " SZFMTD " different than expected " SZFMTD, _headerBuffer.GetActiveSize(), sizeof(IncomingPacketHeader));

    IncomingPacketHeader* header = reinterpret_cast<IncomingPacketHeader*>(_headerBuffer.GetReadPointer());
    uint32 encryptedOpcode = header->EncryptedOpcode;

    if (!header->IsValidSize())
    {
        _authCrypt.PeekDecryptRecv(reinterpret_cast<uint8*>(&header->EncryptedOpcode), sizeof(encryptedOpcode));

        // CMSG_HOTFIX_REQUEST can be much larger than normal packets, allow receiving it once per session
        if (header->EncryptedOpcode != CMSG_HOTFIX_REQUEST || header->Size > 0x100000 || !_canRequestHotfixes)
        {
            TC_LOG_ERROR("network", "WorldSocket::ReadHeaderHandler(): client {} sent malformed packet (size: {}, opcode {})",
                GetRemoteIpAddress(), header->Size, uint32(header->EncryptedOpcode));
            return false;
        }
    }

    _packetBuffer.Resize(header->Size);
    _packetBuffer.Write(&encryptedOpcode, sizeof(encryptedOpcode));
    return true;
}

WorldSocket::ReadDataHandlerResult WorldSocket::ReadDataHandler()
{
    PacketHeader* header = reinterpret_cast<PacketHeader*>(_headerBuffer.GetReadPointer());

    if (!_authCrypt.DecryptRecv(_packetBuffer.GetReadPointer(), header->Size, header->Tag))
    {
        TC_LOG_ERROR("network", "WorldSocket::ReadHeaderHandler(): client {} failed to decrypt packet (size: {})",
            GetRemoteIpAddress(), header->Size);
        return ReadDataHandlerResult::Error;
    }

    WorldPacket packet(std::move(_packetBuffer).Release(), GetConnectionType());
    OpcodeClient opcode = packet.read<OpcodeClient>();
    if (!opcodeTable.IsValid(opcode))
    {
        TC_LOG_ERROR("network", "WorldSocket::ReadHeaderHandler(): client {} sent wrong opcode (opcode: {})",
            GetRemoteIpAddress(), uint32(opcode));
        return ReadDataHandlerResult::Error;
    }

    packet.SetOpcode(opcode);

    if (sPacketLog->CanLogPacket())
        sPacketLog->LogPacket(packet, CLIENT_TO_SERVER, GetRemoteIpAddress(), GetRemotePort(), GetConnectionType());

    std::unique_lock<std::mutex> sessionGuard(_worldSessionLock, std::defer_lock);

    switch (opcode)
    {
        case CMSG_PING:
        {
            LogOpcodeText(opcode, sessionGuard);
            WorldPackets::Auth::Ping ping(std::move(packet));
            if (!ping.ReadNoThrow())
            {
                TC_LOG_ERROR("network", "WorldSocket::ReadDataHandler(): client {} sent malformed CMSG_PING", GetRemoteIpAddress());
                return ReadDataHandlerResult::Error;
            }
            if (!HandlePing(ping))
                return ReadDataHandlerResult::Error;
            break;
        }
        case CMSG_AUTH_SESSION:
        {
            LogOpcodeText(opcode, sessionGuard);
            if (_authed)
            {
                // locking just to safely log offending user is probably overkill but we are disconnecting him anyway
                if (sessionGuard.try_lock())
                    TC_LOG_ERROR("network", "WorldSocket::ProcessIncoming: received duplicate CMSG_AUTH_SESSION from {}", _worldSession->GetPlayerInfo());
                return ReadDataHandlerResult::Error;
            }

            std::shared_ptr<WorldPackets::Auth::AuthSession> authSession = std::make_shared<WorldPackets::Auth::AuthSession>(std::move(packet));
            if (!authSession->ReadNoThrow())
            {
                TC_LOG_ERROR("network", "WorldSocket::ReadDataHandler(): client {} sent malformed CMSG_AUTH_SESSION", GetRemoteIpAddress());
                return ReadDataHandlerResult::Error;
            }
            HandleAuthSession(authSession);
            return ReadDataHandlerResult::WaitingForQuery;
        }
        case CMSG_AUTH_CONTINUED_SESSION:
        {
            LogOpcodeText(opcode, sessionGuard);
            if (_authed)
            {
                // locking just to safely log offending user is probably overkill but we are disconnecting him anyway
                if (sessionGuard.try_lock())
                    TC_LOG_ERROR("network", "WorldSocket::ProcessIncoming: received duplicate CMSG_AUTH_CONTINUED_SESSION from {}", _worldSession->GetPlayerInfo());
                return ReadDataHandlerResult::Error;
            }

            std::shared_ptr<WorldPackets::Auth::AuthContinuedSession> authSession = std::make_shared<WorldPackets::Auth::AuthContinuedSession>(std::move(packet));
            if (!authSession->ReadNoThrow())
            {
                TC_LOG_ERROR("network", "WorldSocket::ReadDataHandler(): client {} sent malformed CMSG_AUTH_CONTINUED_SESSION", GetRemoteIpAddress());
                return ReadDataHandlerResult::Error;
            }
            HandleAuthContinuedSession(authSession);
            return ReadDataHandlerResult::WaitingForQuery;
        }
        case CMSG_KEEP_ALIVE:
            sessionGuard.lock();
            LogOpcodeText(opcode, sessionGuard);
            if (_worldSession)
            {
                _worldSession->ResetTimeOutTime(true);
                return ReadDataHandlerResult::Ok;
            }
            TC_LOG_ERROR("network", "WorldSocket::ReadDataHandler: client {} sent CMSG_KEEP_ALIVE without being authenticated", GetRemoteIpAddress());
            return ReadDataHandlerResult::Error;
        case CMSG_LOG_DISCONNECT:
            LogOpcodeText(opcode, sessionGuard);
            TC_LOG_DEBUG("network", "WorldSocket::ReadDataHandler: client {} sent CMSG_LOG_DISCONNECT reason {}", GetRemoteIpAddress(), packet.read<uint32>());
            break;
        case CMSG_ENABLE_NAGLE:
            LogOpcodeText(opcode, sessionGuard);
            SetNoDelay(false);
            break;
        case CMSG_CONNECT_TO_FAILED:
        {
            sessionGuard.lock();

            LogOpcodeText(opcode, sessionGuard);
            WorldPackets::Auth::ConnectToFailed connectToFailed(std::move(packet));
            if (!connectToFailed.ReadNoThrow())
            {
                TC_LOG_ERROR("network", "WorldSocket::ReadDataHandler(): client {} sent malformed CMSG_CONNECT_TO_FAILED", GetRemoteIpAddress());
                return ReadDataHandlerResult::Error;
            }
            HandleConnectToFailed(connectToFailed);
            break;
        }
        case CMSG_ENTER_ENCRYPTED_MODE_ACK:
            LogOpcodeText(opcode, sessionGuard);
            HandleEnterEncryptedModeAck();
            break;
        case CMSG_HOTFIX_REQUEST:
            _canRequestHotfixes = false;
            [[fallthrough]];
        default:
        {
            if (opcode == CMSG_TIME_SYNC_RESPONSE || opcode == CMSG_MOVE_INIT_ACTIVE_MOVER_COMPLETE || opcode == CMSG_QUEUED_MESSAGES_END)
                packet.SetReceiveTime(std::chrono::steady_clock::now());

            sessionGuard.lock();

            LogOpcodeText(opcode, sessionGuard);

            if (!_worldSession)
            {
                TC_LOG_ERROR("network.opcode", "ProcessIncoming: Client not authed opcode = {}", uint32(opcode));
                return ReadDataHandlerResult::Error;
            }

            ClientOpcodeHandler const* handler = opcodeTable[opcode];
            if (!handler)
            {
                if (sWorld->getBoolConfig(CONFIG_LOG_UNHANDLED_OPCODES))
                {
                    TC_LOG_ERROR("network.opcode", "No defined handler for opcode {} sent by {}", GetOpcodeNameForLogging(static_cast<OpcodeClient>(packet.GetOpcode())), _worldSession->GetPlayerInfo());
                }
                break;
            }

            // Our Idle timer will reset on any non PING opcodes on login screen, allowing us to catch people idling.
            _worldSession->ResetTimeOutTime(false);

            // Copy the packet to the heap before enqueuing
            _worldSession->QueuePacket(new WorldPacket(std::move(packet)));
            break;
        }
    }

    return ReadDataHandlerResult::Ok;
}

void WorldSocket::LogOpcodeText(OpcodeClient opcode, std::unique_lock<std::mutex> const& guard) const
{
    if (!guard || !_worldSession)
    {
        TC_LOG_TRACE("network.opcode", "C->S: {} {}", GetRemoteIpAddress(), GetOpcodeNameForLogging(opcode));
    }
    else
    {
        TC_LOG_TRACE("network.opcode", "C->S: {} {}", _worldSession->GetPlayerInfo(), GetOpcodeNameForLogging(opcode));
    }
}

void WorldSocket::SendPacketAndLogOpcode(WorldPacket const& packet)
{
    TC_LOG_TRACE("network.opcode", "S->C: {} {}", GetRemoteIpAddress(), GetOpcodeNameForLogging(static_cast<OpcodeServer>(packet.GetOpcode())));
    SendPacket(packet);
}

void WorldSocket::SendPacket(WorldPacket const& packet)
{
    if (!IsOpen())
        return;

    if (sPacketLog->CanLogPacket())
        sPacketLog->LogPacket(packet, SERVER_TO_CLIENT, GetRemoteIpAddress(), GetRemotePort(), GetConnectionType());

    _bufferQueue.Enqueue(new EncryptablePacket(packet, _authCrypt.IsInitialized()));
}

void WorldSocket::WritePacketToBuffer(EncryptablePacket const& packet, MessageBuffer& buffer)
{
    uint32 opcode = packet.GetOpcode();
    uint32 packetSize = packet.size();

    // Reserve space for buffer
    uint8* headerPos = buffer.GetWritePointer();
    buffer.WriteCompleted(sizeof(PacketHeader));
    uint8* dataPos = buffer.GetWritePointer();
    buffer.WriteCompleted(sizeof(opcode));

    if (packetSize > MinSizeForCompression && packet.NeedsEncryption())
    {
        CompressedWorldPacket cmp;
        cmp.UncompressedSize = packetSize + sizeof(opcode);
        cmp.UncompressedAdler = adler32(adler32(0x9827D8F1, (Bytef*)&opcode, sizeof(opcode)), packet.data(), packetSize);

        // Reserve space for compression info - uncompressed size and checksums
        uint8* compressionInfo = buffer.GetWritePointer();
        buffer.WriteCompleted(sizeof(CompressedWorldPacket));

        uint32 compressedSize = CompressPacket(buffer.GetWritePointer(), packet);

        cmp.CompressedAdler = adler32(0x9827D8F1, buffer.GetWritePointer(), compressedSize);

        memcpy(compressionInfo, &cmp, sizeof(CompressedWorldPacket));
        buffer.WriteCompleted(compressedSize);
        packetSize = compressedSize + sizeof(CompressedWorldPacket);

        opcode = SMSG_COMPRESSED_PACKET;
    }
    else if (!packet.empty())
        buffer.Write(packet.data(), packet.size());

    memcpy(dataPos, &opcode, sizeof(opcode));
    packetSize += sizeof(opcode);

    PacketHeader header;
    header.Size = packetSize;
    _authCrypt.EncryptSend(dataPos, header.Size, header.Tag);

    memcpy(headerPos, &header, sizeof(PacketHeader));
}

uint32 WorldSocket::CompressPacket(uint8* buffer, WorldPacket const& packet)
{
    uint32 opcode = packet.GetOpcode();
    uint32 bufferSize = deflateBound(_compressionStream, packet.size() + sizeof(opcode));

    _compressionStream->next_out = buffer;
    _compressionStream->avail_out = bufferSize;
    _compressionStream->next_in = (Bytef*)&opcode;
    _compressionStream->avail_in = sizeof(opcode);

    int32 z_res = deflate(_compressionStream, Z_NO_FLUSH);
    if (z_res != Z_OK)
    {
        TC_LOG_ERROR("network", "Can't compress packet opcode (zlib: deflate) Error code: {} ({}, msg: {})", z_res, zError(z_res), _compressionStream->msg);
        return 0;
    }

    _compressionStream->next_in = (Bytef*)packet.data();
    _compressionStream->avail_in = packet.size();

    z_res = deflate(_compressionStream, Z_SYNC_FLUSH);
    if (z_res != Z_OK)
    {
        TC_LOG_ERROR("network", "Can't compress packet data (zlib: deflate) Error code: {} ({}, msg: {})", z_res, zError(z_res), _compressionStream->msg);
        return 0;
    }

    return bufferSize - _compressionStream->avail_out;
}

struct AccountInfo
{
    struct
    {
        uint32 Id;
        bool IsLockedToIP;
        std::string LastIP;
        std::string LockCountry;
        bool IsBanned;
    } BattleNet;

    struct
    {
        uint32 Id;
        std::array<uint8, 64> KeyData;
        uint8 Expansion;
        int64 MuteTime;
        uint32 Build;
        LocaleConstant Locale;
        uint32 Recruiter;
        std::string OS;
        Minutes TimezoneOffset;
        bool IsRectuiter;
        AccountTypes Security;
        bool IsBanned;
    } Game;

    bool IsBanned() const { return BattleNet.IsBanned || Game.IsBanned; }

    explicit AccountInfo(Field const* fields)
    {
        //           0              1           2          3                4            5           6               7         8            9    10                 11     12                13
        // SELECT a.id, a.session_key, ba.last_ip, ba.locked, ba.lock_country, a.expansion, a.mutetime, a.client_build, a.locale, a.recruiter, a.os, a.timezone_offset, ba.id, aa.SecurityLevel,
        //                                                              14                                                            15    16
        // bab.unbandate > UNIX_TIMESTAMP() OR bab.unbandate = bab.bandate, ab.unbandate > UNIX_TIMESTAMP() OR ab.unbandate = ab.bandate, r.id
        // FROM account a LEFT JOIN battlenet_accounts ba ON a.battlenet_account = ba.id LEFT JOIN account_access aa ON a.id = aa.AccountID AND aa.RealmID IN (-1, ?)
        // LEFT JOIN battlenet_account_bans bab ON ba.id = bab.id LEFT JOIN account_banned ab ON a.id = ab.id LEFT JOIN account r ON a.id = r.recruiter
        // WHERE a.username = ? AND LENGTH(a.session_key) = 40 ORDER BY aa.RealmID DESC LIMIT 1
        Game.Id = fields[0].GetUInt32();
        Game.KeyData = fields[1].GetBinary<64>();
        BattleNet.LastIP = fields[2].GetString();
        BattleNet.IsLockedToIP = fields[3].GetBool();
        BattleNet.LockCountry = fields[4].GetString();
        Game.Expansion = fields[5].GetUInt8();
        Game.MuteTime = fields[6].GetInt64();
        Game.Build = fields[7].GetUInt32();
        Game.Locale = LocaleConstant(fields[8].GetUInt8());
        Game.Recruiter = fields[9].GetUInt32();
        Game.OS = fields[10].GetString();
        Game.TimezoneOffset = Minutes(fields[11].GetInt16());
        BattleNet.Id = fields[12].GetUInt32();
        Game.Security = AccountTypes(fields[13].GetUInt8());
        BattleNet.IsBanned = fields[14].GetUInt32() != 0;
        Game.IsBanned = fields[15].GetUInt32() != 0;
        Game.IsRectuiter = fields[16].GetUInt32() != 0;

        if (Game.Locale >= TOTAL_LOCALES)
            Game.Locale = LOCALE_enUS;
    }
};

void WorldSocket::HandleAuthSession(std::shared_ptr<WorldPackets::Auth::AuthSession> authSession)
{
    std::shared_ptr<JSON::RealmList::RealmJoinTicket> joinTicket = std::make_shared<JSON::RealmList::RealmJoinTicket>();
    if (!JSON::Deserialize(authSession->RealmJoinTicket, joinTicket.get()))
    {
        SendAuthResponseError(ERROR_WOW_SERVICES_INVALID_JOIN_TICKET);
        DelayedCloseSocket();
        return;
    }

    // Get the account information from the auth database
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_INFO_BY_NAME);
    stmt->setInt32(0, int32(sRealmList->GetCurrentRealmId().Realm));
    stmt->setString(1, joinTicket->gameaccount());

    QueueQuery(LoginDatabase.AsyncQuery(stmt).WithPreparedCallback([this, authSession = std::move(authSession), joinTicket = std::move(joinTicket)](PreparedQueryResult result) mutable
    {
        HandleAuthSessionCallback(std::move(authSession), std::move(joinTicket), std::move(result));
    }));
}

void WorldSocket::HandleAuthSessionCallback(std::shared_ptr<WorldPackets::Auth::AuthSession> authSession,
    std::shared_ptr<JSON::RealmList::RealmJoinTicket> joinTicket, PreparedQueryResult result)
{
    // Stop if the account is not found
    if (!result)
    {
        // We can not log here, as we do not know the account. Thus, no accountId.
        TC_LOG_ERROR("network", "WorldSocket::HandleAuthSession: Sent Auth Response (unknown account).");
        DelayedCloseSocket();
        return;
    }

    std::string address = GetRemoteIpAddress().to_string();

    AccountInfo account(result->Fetch());

    ClientBuild::Info const* buildInfo = ClientBuild::GetBuildInfo(account.Game.Build);
    if (!buildInfo)
    {
        SendAuthResponseError(ERROR_BAD_VERSION);
        TC_LOG_ERROR("network", "WorldSocket::HandleAuthSession: Missing client build info for build {} ({}).", account.Game.Build, address);
        DelayedCloseSocket();
        return;
    }

    ClientBuild::VariantId buildVariant = { .Platform = joinTicket->platform(), .Arch = joinTicket->clientarch(), .Type = joinTicket->type() };
    auto clientBuildAuthKey = std::ranges::find(buildInfo->AuthKeys, buildVariant, &ClientBuild::AuthKey::Variant);
    if (clientBuildAuthKey == buildInfo->AuthKeys.end())
    {
        SendAuthResponseError(ERROR_BAD_VERSION);
        TC_LOG_ERROR("network", "WorldSocket::HandleAuthSession: Missing client build auth key for build {} variant {}-{}-{} ({}).", account.Game.Build,
            ClientBuild::ToCharArray(buildVariant.Platform).data(), ClientBuild::ToCharArray(buildVariant.Arch).data(),
            ClientBuild::ToCharArray(buildVariant.Type).data(), address);
        DelayedCloseSocket();
        return;
    }

    Trinity::Crypto::SHA512 digestKeyHash;
    digestKeyHash.UpdateData(account.Game.KeyData.data(), account.Game.KeyData.size());
    digestKeyHash.UpdateData(clientBuildAuthKey->Key.data(), clientBuildAuthKey->Key.size());
    digestKeyHash.Finalize();

    Trinity::Crypto::HMAC_SHA512 hmac(digestKeyHash.GetDigest());
    hmac.UpdateData(authSession->LocalChallenge);
    hmac.UpdateData(_serverChallenge);
    hmac.UpdateData(AuthCheckSeed);
    hmac.Finalize();

    // Check that Key and account name are the same on client and server
    if (memcmp(hmac.GetDigest().data(), authSession->Digest.data(), authSession->Digest.size()) != 0)
    {
        SendAuthResponseError(ERROR_DENIED);
        TC_LOG_ERROR("network", "WorldSocket::HandleAuthSession: Authentication failed for account: {} ('{}') address: {}", account.Game.Id, joinTicket->gameaccount(), address);
        DelayedCloseSocket();
        return;
    }

    Trinity::Crypto::SHA512 keyData;
    keyData.UpdateData(account.Game.KeyData.data(), account.Game.KeyData.size());
    keyData.Finalize();

    Trinity::Crypto::HMAC_SHA512 sessionKeyHmac(keyData.GetDigest());
    sessionKeyHmac.UpdateData(_serverChallenge);
    sessionKeyHmac.UpdateData(authSession->LocalChallenge);
    sessionKeyHmac.UpdateData(SessionKeySeed);
    sessionKeyHmac.Finalize();

    SessionKeyGenerator<Trinity::Crypto::SHA512> sessionKeyGenerator(sessionKeyHmac.GetDigest());
    sessionKeyGenerator.Generate(_sessionKey.data(), 40);

    Trinity::Crypto::HMAC_SHA512 encryptKeyGen(_sessionKey);
    encryptKeyGen.UpdateData(authSession->LocalChallenge);
    encryptKeyGen.UpdateData(_serverChallenge);
    encryptKeyGen.UpdateData(EncryptionKeySeed);
    encryptKeyGen.Finalize();

    // only first 32 bytes of the hmac are used
    memcpy(_encryptKey.data(), encryptKeyGen.GetDigest().data(), 32);

    LoginDatabasePreparedStatement* stmt = nullptr;

    if (sWorld->getBoolConfig(CONFIG_ALLOW_LOGGING_IP_ADDRESSES_IN_DATABASE))
    {
        // As we don't know if attempted login process by ip works, we update last_attempt_ip right away
        stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_LAST_ATTEMPT_IP);
        stmt->setString(0, address);
        stmt->setString(1, joinTicket->gameaccount());
        LoginDatabase.Execute(stmt);
        // This also allows to check for possible "hack" attempts on account
    }

    stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_INFO_CONTINUED_SESSION);
    stmt->setBinary(0, _sessionKey);
    stmt->setUInt32(1, account.Game.Id);
    LoginDatabase.Execute(stmt);

    // First reject the connection if packet contains invalid data or realm state doesn't allow logging in
    if (sWorld->IsClosed())
    {
        SendAuthResponseError(ERROR_DENIED);
        TC_LOG_ERROR("network", "WorldSocket::HandleAuthSession: World closed, denying client ({}).", address);
        DelayedCloseSocket();
        return;
    }

    if (authSession->RealmID != sRealmList->GetCurrentRealmId().Realm)
    {
        SendAuthResponseError(ERROR_DENIED);
        TC_LOG_ERROR("network", "WorldSocket::HandleAuthSession: Client {} requested connecting with realm id {} but this realm has id {} set in config.",
            address, authSession->RealmID, sRealmList->GetCurrentRealmId().Realm);
        DelayedCloseSocket();
        return;
    }

    // Must be done before WorldSession is created
    bool wardenActive = sWorld->getBoolConfig(CONFIG_WARDEN_ENABLED);
    if (wardenActive && !ClientBuild::Platform::IsValid(account.Game.OS))
    {
        SendAuthResponseError(ERROR_DENIED);
        TC_LOG_ERROR("network", "WorldSocket::HandleAuthSession: Client {} attempted to log in using invalid client OS ({}).", address, account.Game.OS);
        DelayedCloseSocket();
        return;
    }

    if (IpLocationRecord const* location = sIPLocation->GetLocationRecord(address))
        _ipCountry = location->CountryCode;

    ///- Re-check ip locking (same check as in auth).
    if (account.BattleNet.IsLockedToIP)
    {
        if (account.BattleNet.LastIP != address)
        {
            SendAuthResponseError(ERROR_RISK_ACCOUNT_LOCKED);
            TC_LOG_DEBUG("network", "WorldSocket::HandleAuthSession: Sent Auth Response (Account IP differs. Original IP: {}, new IP: {}).", account.BattleNet.LastIP, address);
            // We could log on hook only instead of an additional db log, however action logger is config based. Better keep DB logging as well
            sScriptMgr->OnFailedAccountLogin(account.Game.Id);
            DelayedCloseSocket();
            return;
        }
    }
    else if (!account.BattleNet.LockCountry.empty() && account.BattleNet.LockCountry != "00" && !_ipCountry.empty())
    {
        if (account.BattleNet.LockCountry != _ipCountry)
        {
            SendAuthResponseError(ERROR_RISK_ACCOUNT_LOCKED);
            TC_LOG_DEBUG("network", "WorldSocket::HandleAuthSession: Sent Auth Response (Account country differs. Original country: {}, new country: {}).", account.BattleNet.LockCountry, _ipCountry);
            // We could log on hook only instead of an additional db log, however action logger is config based. Better keep DB logging as well
            sScriptMgr->OnFailedAccountLogin(account.Game.Id);
            DelayedCloseSocket();
            return;
        }
    }

    int64 mutetime = account.Game.MuteTime;
    //! Negative mutetime indicates amount of seconds to be muted effective on next login - which is now.
    if (mutetime < 0)
    {
        mutetime = GameTime::GetGameTime() - mutetime;

        stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_MUTE_TIME_LOGIN);
        stmt->setInt64(0, mutetime);
        stmt->setUInt32(1, account.Game.Id);
        LoginDatabase.Execute(stmt);
    }

    if (account.IsBanned())
    {
        SendAuthResponseError(ERROR_GAME_ACCOUNT_BANNED);
        TC_LOG_ERROR("network", "WorldSocket::HandleAuthSession: Sent Auth Response (Account banned).");
        sScriptMgr->OnFailedAccountLogin(account.Game.Id);
        DelayedCloseSocket();
        return;
    }

    // Check locked state for server
    AccountTypes allowedAccountType = sWorld->GetPlayerSecurityLimit();
    TC_LOG_DEBUG("network", "Allowed Level: {} Player Level {}", allowedAccountType, account.Game.Security);
    if (allowedAccountType > SEC_PLAYER && account.Game.Security < allowedAccountType)
    {
        SendAuthResponseError(ERROR_SERVER_IS_PRIVATE);
        TC_LOG_DEBUG("network", "WorldSocket::HandleAuthSession: User tries to login but his security level is not enough");
        sScriptMgr->OnFailedAccountLogin(account.Game.Id);
        DelayedCloseSocket();
        return;
    }

    TC_LOG_DEBUG("network", "WorldSocket::HandleAuthSession: Client '{}' authenticated successfully from {}.", joinTicket->gameaccount(), address);

    if (sWorld->getBoolConfig(CONFIG_ALLOW_LOGGING_IP_ADDRESSES_IN_DATABASE))
    {
        // Update the last_ip in the database as it was successful for login
        stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_LAST_IP);

        stmt->setString(0, address);
        stmt->setString(1, joinTicket->gameaccount());

        LoginDatabase.Execute(stmt);
    }

    // At this point, we can safely hook a successful login
    sScriptMgr->OnAccountLogin(account.Game.Id);

    _authed = true;
    _worldSession = new WorldSession(account.Game.Id, std::move(*joinTicket->mutable_gameaccount()), account.BattleNet.Id,
        static_pointer_cast<WorldSocket>(shared_from_this()), account.Game.Security, account.Game.Expansion, mutetime,
        account.Game.OS, account.Game.TimezoneOffset, account.Game.Build, buildVariant, account.Game.Locale,
        account.Game.Recruiter, account.Game.IsRectuiter);

    // Initialize Warden system only if it is enabled by config
    if (wardenActive)
        _worldSession->InitWarden(_sessionKey);

    QueueQuery(_worldSession->LoadPermissionsAsync().WithPreparedCallback([this](PreparedQueryResult result)
    {
        LoadSessionPermissionsCallback(std::move(result));
    }));
    AsyncRead(Trinity::Net::InvokeReadHandlerCallback<WorldSocket>{ .Socket = this });
}

void WorldSocket::LoadSessionPermissionsCallback(PreparedQueryResult result)
{
    // RBAC must be loaded before adding session to check for skip queue permission
    _worldSession->GetRBACData()->LoadFromDBCallback(result);

    SendPacketAndLogOpcode(*WorldPackets::Auth::EnterEncryptedMode(_encryptKey, true).Write());
}

void WorldSocket::HandleAuthContinuedSession(std::shared_ptr<WorldPackets::Auth::AuthContinuedSession> authSession)
{
    WorldSession::ConnectToKey key;
    key.Raw = authSession->Key;

    _type = ConnectionType(key.Fields.ConnectionType);
    if (_type != CONNECTION_TYPE_INSTANCE)
    {
        SendAuthResponseError(ERROR_DENIED);
        DelayedCloseSocket();
        return;
    }

    uint32 accountId = uint32(key.Fields.AccountId);
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_INFO_CONTINUED_SESSION);
    stmt->setUInt32(0, accountId);

    QueueQuery(LoginDatabase.AsyncQuery(stmt).WithPreparedCallback([this, authSession = std::move(authSession)](PreparedQueryResult result) mutable
    {
        HandleAuthContinuedSessionCallback(std::move(authSession), std::move(result));
    }));
}

void WorldSocket::HandleAuthContinuedSessionCallback(std::shared_ptr<WorldPackets::Auth::AuthContinuedSession> authSession, PreparedQueryResult result)
{
    if (!result)
    {
        SendAuthResponseError(ERROR_DENIED);
        DelayedCloseSocket();
        return;
    }

    WorldSession::ConnectToKey key;
    _key = key.Raw = authSession->Key;

    uint32 accountId = uint32(key.Fields.AccountId);
    Field* fields = result->Fetch();
    std::string login = fields[0].GetString();
    _sessionKey = fields[1].GetBinary<SESSION_KEY_LENGTH>();

    Trinity::Crypto::HMAC_SHA512 hmac(_sessionKey);
    hmac.UpdateData(reinterpret_cast<uint8 const*>(&authSession->Key), sizeof(authSession->Key));
    hmac.UpdateData(authSession->LocalChallenge);
    hmac.UpdateData(_serverChallenge);
    hmac.UpdateData(ContinuedSessionSeed);
    hmac.Finalize();

    if (memcmp(hmac.GetDigest().data(), authSession->Digest.data(), authSession->Digest.size()))
    {
        TC_LOG_ERROR("network", "WorldSocket::HandleAuthContinuedSession: Authentication failed for account: {} ('{}') address: {}", accountId, login, GetRemoteIpAddress());
        DelayedCloseSocket();
        return;
    }

    Trinity::Crypto::HMAC_SHA512 encryptKeyGen(_sessionKey);
    encryptKeyGen.UpdateData(authSession->LocalChallenge);
    encryptKeyGen.UpdateData(_serverChallenge);
    encryptKeyGen.UpdateData(EncryptionKeySeed);
    encryptKeyGen.Finalize();

    // only first 32 bytes of the hmac are used
    memcpy(_encryptKey.data(), encryptKeyGen.GetDigest().data(), 32);

    SendPacketAndLogOpcode(*WorldPackets::Auth::EnterEncryptedMode(_encryptKey, true).Write());
    AsyncRead(Trinity::Net::InvokeReadHandlerCallback<WorldSocket>{ .Socket = this });
}

void WorldSocket::HandleConnectToFailed(WorldPackets::Auth::ConnectToFailed& connectToFailed)
{
    if (_worldSession)
    {
        if (_worldSession->PlayerLoading())
        {
            switch (connectToFailed.Serial)
            {
                case WorldPackets::Auth::ConnectToSerial::WorldAttempt1:
                    _worldSession->SendConnectToInstance(WorldPackets::Auth::ConnectToSerial::WorldAttempt2);
                    break;
                case WorldPackets::Auth::ConnectToSerial::WorldAttempt2:
                    _worldSession->SendConnectToInstance(WorldPackets::Auth::ConnectToSerial::WorldAttempt3);
                    break;
                case WorldPackets::Auth::ConnectToSerial::WorldAttempt3:
                    _worldSession->SendConnectToInstance(WorldPackets::Auth::ConnectToSerial::WorldAttempt4);
                    break;
                case WorldPackets::Auth::ConnectToSerial::WorldAttempt4:
                    _worldSession->SendConnectToInstance(WorldPackets::Auth::ConnectToSerial::WorldAttempt5);
                    break;
                case WorldPackets::Auth::ConnectToSerial::WorldAttempt5:
                {
                    TC_LOG_ERROR("network", "{} failed to connect 5 times to world socket, aborting login", _worldSession->GetPlayerInfo());
                    _worldSession->AbortLogin(WorldPackets::Character::LoginFailureReason::NoWorld);
                    break;
                }
                default:
                    return;
            }
        }
        //else
        //{
        //    transfer_aborted when/if we get map node redirection
        //    SendPacketAndLogOpcode(*WorldPackets::Auth::ResumeComms().Write());
        //}
    }
}

void WorldSocket::HandleEnterEncryptedModeAck()
{
    _authCrypt.Init(_encryptKey);
    if (_type == CONNECTION_TYPE_REALM)
        sWorld->AddSession(_worldSession);
    else
        sWorld->AddInstanceSocket(static_pointer_cast<WorldSocket>(shared_from_this()), _key);
}

void WorldSocket::SendAuthResponseError(uint32 code)
{
    WorldPackets::Auth::AuthResponse response;
    response.Result = code;
    SendPacketAndLogOpcode(*response.Write());
}

bool WorldSocket::HandlePing(WorldPackets::Auth::Ping& ping)
{
    using namespace std::chrono;

    if (_LastPingTime == steady_clock::time_point())
    {
        _LastPingTime = steady_clock::now();
    }
    else
    {
        steady_clock::time_point now = steady_clock::now();

        steady_clock::duration diff = now - _LastPingTime;

        _LastPingTime = now;

        if (diff < 27s)
        {
            ++_OverSpeedPings;

            uint32 maxAllowed = sWorld->getIntConfig(CONFIG_MAX_OVERSPEED_PINGS);

            if (maxAllowed && _OverSpeedPings > maxAllowed)
            {
                bool ignoresOverspeedPingsLimit = [&]
                {
                    std::lock_guard<std::mutex> sessionGuard(_worldSessionLock);
                    return _worldSession && !_worldSession->HasPermission(rbac::RBAC_PERM_SKIP_CHECK_OVERSPEED_PING);
                }();

                if (!ignoresOverspeedPingsLimit)
                {
                    TC_LOG_ERROR("network", "WorldSocket::HandlePing: {} kicked for over-speed pings (address: {})",
                        _worldSession->GetPlayerInfo(), GetRemoteIpAddress());

                    return false;
                }
            }
        }
        else
            _OverSpeedPings = 0;
    }

    bool success = [&]
    {
        std::lock_guard<std::mutex> sessionGuard(_worldSessionLock);
        if (_worldSession)
        {
            _worldSession->SetLatency(ping.Latency);
            return true;
        }
        return false;
    }();

    if (!success)
    {
        TC_LOG_ERROR("network", "WorldSocket::HandlePing: peer sent CMSG_PING, but is not authenticated or got recently kicked, address = {}", GetRemoteIpAddress());
        return false;

    }

    SendPacketAndLogOpcode(*WorldPackets::Auth::Pong(ping.Serial).Write());
    return true;
}
