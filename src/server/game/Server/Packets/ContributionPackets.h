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

#ifndef TRINITYCORE_CONTRIBUTION_PACKETS_H
#define TRINITYCORE_CONTRIBUTION_PACKETS_H

#include "Packet.h"
#include "ObjectGuid.h"

// War-effort "Contribution Collector": a player turns in the required currency/items (defined by the
// ManagedWorldStateInput.QuestID of the contribution) to advance a realm-wide managed world state.
//
// The bar itself is NOT carried by any of these messages. C_ContributionCollector.GetState() is answered entirely
// client-side from Contribution.db2 / ManagedWorldState.db2 plus the world-state values the server pushes with
// SMSG_UPDATE_WORLD_STATE (confirmed in the 8.0.1.27377 war-effort sniff: opcode 10132, {int32 VariableID,
// int32 Value, bit Hidden}, realm-wide, driving the war-effort totals). The only thing the collector round-trip
// carries is a small "last update" acknowledgement keyed by contribution id.
namespace WorldPackets
{
namespace Contribution
{
    // CMSG_CONTRIBUTION_CONTRIBUTE (0x3B00FD): { PackedGuid CollectorGUID, uint32 ContributionID }.
    // Wire recovered from the 68275 client serializer (sub_7FF729154010). The payload id is the *contribution id*:
    // the only argument of C_ContributionCollector.Contribute(contributionID), while GetOrderIndex(contributionID)
    // is a pure client-side Contribution.db2 lookup and therefore never travels on the wire.
    class ContributionContribute final : public ClientPacket
    {
    public:
        explicit ContributionContribute(WorldPacket&& packet) : ClientPacket(CMSG_CONTRIBUTION_CONTRIBUTE, std::move(packet)) { }

        void Read() override;

        ObjectGuid CollectorGUID;
        uint32 ContributionID = 0;
    };

    // CMSG_CONTRIBUTION_LAST_UPDATE_REQUEST (0x3B00FE): { uint32 ContributionID, uint32 ContributionGUID }.
    class ContributionLastUpdateRequest final : public ClientPacket
    {
    public:
        explicit ContributionLastUpdateRequest(WorldPacket&& packet) : ClientPacket(CMSG_CONTRIBUTION_LAST_UPDATE_REQUEST, std::move(packet)) { }

        void Read() override;

        uint32 ContributionID = 0;
        uint32 ContributionGUID = 0;
    };

    // SMSG_CONTRIBUTION_LAST_UPDATE_RESPONSE (0x4202C4): exactly twelve bytes -
    // { uint32 Data /*unix timestamp of the last update*/, uint32 ContributionID, uint32 ContributionGUID }.
    // It is a timestamp acknowledgement only - no state, no percentage, no array.
    class ContributionLastUpdateResponse final : public ServerPacket
    {
    public:
        explicit ContributionLastUpdateResponse() : ServerPacket(SMSG_CONTRIBUTION_LAST_UPDATE_RESPONSE, 12) { }

        WorldPacket const* Write() override;

        uint32 Data = 0;                // time of the contribution's last update (unix time)
        uint32 ContributionID = 0;
        uint32 ContributionGUID = 0;    // echoed back from the request
    };
}
}

#endif // TRINITYCORE_CONTRIBUTION_PACKETS_H
