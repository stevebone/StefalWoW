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

#ifndef TRINITYCORE_COVENANT_PACKETS_H
#define TRINITYCORE_COVENANT_PACKETS_H

#include "Packet.h"
#include "ObjectGuid.h"

namespace WorldPackets
{
namespace Covenant
{
    // CMSG_ACTIVATE_SOULBIND (0x3A028F). Serializer sub_7FF72914B2B0 writes a single uint32 (the soulbind id).
    class ActivateSoulbind final : public ClientPacket
    {
    public:
        explicit ActivateSoulbind(WorldPacket&& packet) : ClientPacket(CMSG_ACTIVATE_SOULBIND, std::move(packet)) { }

        void Read() override;

        int32 SoulbindID = 0;
    };

    // Reason codes for SMSG_ACTIVATE_SOULBIND_FAILED. The client turns the reason into a GAMEERROR string itself
    // (sub_7FF72A6F1940, a 10-entry jump table); values 2, 4, 5 and anything above 9 fall through to
    // ERR_CANT_DO_THAT_RIGHT_NOW. Because the client displays the error from this code, the server must NOT also
    // send SMSG_DISPLAY_GAME_ERROR for the same failure - that would double up the message.
    enum class SoulbindActivationError : uint8
    {
        None                = 0,    // maps to GAMEERROR 1241 == the table size, i.e. nothing is displayed
        CantDoThatRightNow  = 1,    // ERR_CANT_DO_THAT_RIGHT_NOW
        NoSpec              = 3,    // ERR_NO_SPEC
        AffectingCombat     = 6,    // ERR_AFFECTING_COMBAT
        ChallengeModeActive = 7,    // ERR_CANT_DO_THAT_CHALLENGE_MODE_ACTIVE
        RestArea            = 8,    // ERR_ACTIVATE_SOULBIND_FAILED_REST_AREA
        PlayerDead          = 9     // ERR_PLAYER_DEAD
    };

    // SMSG_ACTIVATE_SOULBIND_FAILED (0x5F0023). Sent when CMSG_ACTIVATE_SOULBIND is rejected, so the client stops
    // waiting on the requested soulbind and shows the reason.
    //
    // Wire, read straight off the dispatcher sub_7FF72911F1D0, case 0x5F0023, at 0x7FF7291207AD:
    //     Bits<4> Reason   - sub_7FF7290650F0, a 4-bit MSB-first read, so it is the HIGH nibble of byte 0
    //     uint32           - sub_7FF72BE6C410, a plain byte-aligned little-endian dword
    // That is 5 bytes on the wire, NOT the bare { uint32 } recorded in all_smsg_layouts_68275.json: that extractor
    // does not model bit-packed fields and silently dropped the leading nibble. Trust this disassembly over the JSON.
    //
    // Only Reason is consumed. The sole subscriber (sub_7FF72A6F19B0) reads obj+0x20 (Reason) and never touches
    // obj+0x24 (the dword), and no Lua event is fired on this path, so the dword's meaning is UNDETERMINED. We echo
    // the requested SoulbindID - the plausible reading, since CMSG_ACTIVATE_SOULBIND is itself { uint32 SoulbindID } -
    // and it is inert either way.
    class ActivateSoulbindFailed final : public ServerPacket
    {
    public:
        ActivateSoulbindFailed() : ServerPacket(SMSG_ACTIVATE_SOULBIND_FAILED, 1 + 4) { }

        WorldPacket const* Write() override;

        SoulbindActivationError Reason = SoulbindActivationError::CantDoThatRightNow;
        int32 SoulbindID = 0;
    };

    // SMSG_COVENANT_PREVIEW_OPEN_NPC (0x4202A5). Wire (client reader sub_7FF7290AF0E0, dispatcher 0x7FF729103660):
    // ObjectGuid (16) followed by uint32 (4). Drives the client's COVENANT_PREVIEW_OPEN Lua event.
    //
    // CovenantID is CONFIRMED, not inferred: the listener (0x7FF72AB65D90 -> sub_7FF72ACD2D40) passes the dword to
    // Covenant.db2 GetRecord() - the same call C_Covenants.GetCovenantData(covenantID) makes - and to
    // UICovenantPreview secondary index #0, which is keyed on the CovenantID column, not on the row ID. So it is
    // the covenant id 1-4, NOT a UiCovenantPreview row id (5/6/7 would fail the Covenant.db2 lookup outright).
    // We source it from the gossip option's GossipNPCOption.db2 row, never guessed per-creature.
    //
    // NpcGUID is read into obj+0x20 but ignored by that listener; it is sent for correctness/consistency.
    class CovenantPreviewOpenNpc final : public ServerPacket
    {
    public:
        CovenantPreviewOpenNpc() : ServerPacket(SMSG_COVENANT_PREVIEW_OPEN_NPC, 16 + 4) { }

        WorldPacket const* Write() override;

        ObjectGuid NpcGUID;
        int32 CovenantID = 0;
    };

    // CMSG_REQUEST_COVENANT_CALLINGS (0x3A0269). Empty payload; the client asks which covenant callings (bounties) are available.
    class RequestCovenantCallings final : public ClientPacket
    {
    public:
        explicit RequestCovenantCallings(WorldPacket&& packet) : ClientPacket(CMSG_REQUEST_COVENANT_CALLINGS, std::move(packet)) { }

        void Read() override { }
    };

    // SMSG_COVENANT_CALLINGS_AVAILABILITY_RESPONSE (0x600024). Deserializer reads Bits<1> CallingsUnlocked, then uint32 count, then count x uint32 Bounty.db2 ID.
    class CovenantCallingsAvailabilityResponse final : public ServerPacket
    {
    public:
        CovenantCallingsAvailabilityResponse() : ServerPacket(SMSG_COVENANT_CALLINGS_AVAILABILITY_RESPONSE, 1 + 4) { }

        WorldPacket const* Write() override;

        bool CallingsUnlocked = false;
        std::vector<int32> BountyIDs;
    };

    // CMSG_COVENANT_RENOWN_REQUEST_CATCHUP_STATE (0x3B0111). Empty payload; the client asks whether accelerated
    // renown catch-up is currently active for the player.
    class CovenantRenownRequestCatchupState final : public ClientPacket
    {
    public:
        explicit CovenantRenownRequestCatchupState(WorldPacket&& packet) : ClientPacket(CMSG_COVENANT_RENOWN_REQUEST_CATCHUP_STATE, std::move(packet)) { }

        void Read() override { }
    };

    // SMSG_COVENANT_RENOWN_SEND_CATCHUP_STATE (0x42030D). Wire (client reader, all_smsg_layouts): a single Bits<1>.
    // Core does not implement accelerated renown catch-up, so the answer is false (no catch-up active).
    class CovenantRenownSendCatchupState final : public ServerPacket
    {
    public:
        CovenantRenownSendCatchupState() : ServerPacket(SMSG_COVENANT_RENOWN_SEND_CATCHUP_STATE, 1) { }

        WorldPacket const* Write() override;

        bool IsActive = false;
    };
}
}

#endif // TRINITYCORE_COVENANT_PACKETS_H
