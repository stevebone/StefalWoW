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

    // SMSG_CONTRIBUTION_LAST_UPDATE_RESPONSE (0x4202C4): BYTE-RECOVERED from the 12.0.7 (68275) client as
    // exactly SIXTEEN bytes - { uint64 Data, int32 ContributionID, int32 ContributionGUID }. The old
    // "exactly twelve bytes" claim was wrong: the timestamp is 8 bytes wide, not 4.
    // Like SMSG_WARFRONT_COMPLETE this is not reflection-serialized; the handler casts the raw payload to a POD:
    //   - vtable slot 3 (RVA 0x611D20) returns 0x4202C4; ctor RVA 0x611D30 puts the buffer at obj+0x28
    //   - vtable slot 0 (RVA 0x5C45F0) allocates 0x38 = 56 bytes, so the body is 56 - 40 = 16
    //   - handler RVA 0x2230860 reads qword[p], then movsxd dword[p+8] and movsxd dword[p+0xC], and stores the
    //     qword as the VALUE of a ContributionCollector::StateChange* map keyed by the two dwords, then clears
    //     the "request pending" byte
    //   - the two dwords are exactly what the client sent: the CMSG_CONTRIBUTION_LAST_UPDATE_REQUEST serializer
    //     (RVA 0x6B4070) writes opcode 0x3B00FE followed by the same pair, sourced in C_ContributionCollector
    //     .GetState (RVA 0x222F600) from (contributionID, the record's second key)
    // It remains a timestamp acknowledgement only - no state, no percentage, no array.
    class ContributionLastUpdateResponse final : public ServerPacket
    {
    public:
        explicit ContributionLastUpdateResponse() : ServerPacket(SMSG_CONTRIBUTION_LAST_UPDATE_RESPONSE, 16) { }

        WorldPacket const* Write() override;

        uint64 Data = 0;                // time of the contribution's last update (unix time); 8 bytes on the wire
        uint32 ContributionID = 0;
        uint32 ContributionGUID = 0;    // echoed back from the request (the request's second key dword)
    };
}
}

#endif // TRINITYCORE_CONTRIBUTION_PACKETS_H
