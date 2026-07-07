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
// Wire recovered from the client serializers sub_7FF729154010 / sub_7FF729154070 (68275).
namespace WorldPackets
{
namespace Contribution
{
    // CMSG_CONTRIBUTION_CONTRIBUTE (0x3B00FD): { ObjectGuid CollectorGUID, uint32 ContributionID }.
    class ContributionContribute final : public ClientPacket
    {
    public:
        explicit ContributionContribute(WorldPacket&& packet) : ClientPacket(CMSG_CONTRIBUTION_CONTRIBUTE, std::move(packet)) { }

        void Read() override;

        ObjectGuid CollectorGUID;
        uint32 ContributionID = 0;
    };

    // CMSG_CONTRIBUTION_LAST_UPDATE_REQUEST (0x3B00FE): { uint32 ContributionID, uint32 Field1 }.
    // Field1 is read but its semantics are not resolved offline (documented in the plan).
    class ContributionLastUpdateRequest final : public ClientPacket
    {
    public:
        explicit ContributionLastUpdateRequest(WorldPacket&& packet) : ClientPacket(CMSG_CONTRIBUTION_LAST_UPDATE_REQUEST, std::move(packet)) { }

        void Read() override;

        uint32 ContributionID = 0;
        uint32 Field1 = 0;
    };
}
}

#endif // TRINITYCORE_CONTRIBUTION_PACKETS_H
