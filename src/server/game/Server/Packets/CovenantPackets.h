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
