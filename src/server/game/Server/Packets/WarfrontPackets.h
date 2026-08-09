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

#ifndef TRINITYCORE_WARFRONT_PACKETS_H
#define TRINITYCORE_WARFRONT_PACKETS_H

#include "Packet.h"

namespace WorldPackets
{
namespace Warfront
{
    // SMSG_WARFRONT_COMPLETE (0x420206): fires the client's WARFRONT_COMPLETED event, which drives
    // Blizzard_WarfrontsPartyPoseUI's victory screen.
    //
    // BYTE-RECOVERED from the 12.0.7 (68275) client - exactly 8 bytes, { int32 MapID, int32 Winner }.
    // This opcode is not reflection-serialized at all, which is why descriptor/jam_types tracing found nothing:
    // the dispatcher hands the handler the raw remaining payload and the handler casts it to a POD struct.
    //   - ctor RVA 0x606BA0 seeds the two defaults in the inline buffer: dword[obj+0x28] = 0, dword[obj+0x2C] = -1
    //   - vtable slot 3 (RVA 0x606B90) returns 0x420206, proving the class/opcode pairing
    //   - vtable slot 0 (RVA 0x5C4690) allocates 0x30 = 48 bytes; buffer sits at +40, so the body is 48 - 40 = 8
    //   - handler RVA 0x1C83D40 reads exactly v1[0] and v1[1] and fires event 0xD32522534920666F, whose registrar
    //     (RVA 0x106D214) pairs that type-id with the string "WARFRONT_COMPLETED" (len 0x12)
    //   - Lua marshaller RVA 0xA08630 pushes exactly two numbers and returns 2
    // Confirmed against the client's own docs: LFGInfoDocumentation.lua WARFRONT_COMPLETED payload is
    // { mapID:number, winner:number }.
    class WarfrontComplete final : public ServerPacket
    {
    public:
        explicit WarfrontComplete() : ServerPacket(SMSG_WARFRONT_COMPLETE, 8) { }

        WorldPacket const* Write() override;

        // The uiMap the victory screen looks up - Blizzard_PartyPoseUI feeds it straight to
        // C_PartyPose.GetPartyPoseInfoByMapID. This is the battle map that was just completed, NOT a warfront id.
        int32 MapID = 0;

        // FACTION GROUP, not TeamId. SharedConstants.lua: PLAYER_FACTION_GROUP = { Horde = 0, Alliance = 1 },
        // i.e. the INVERSE of TC's TeamId (TEAM_ALLIANCE = 0). Blizzard_PartyPoseUI compares it against the
        // viewer's own faction group to decide whether to show the win or the loss pose. Client default is -1.
        int32 Winner = -1;
    };
}
}

#endif // TRINITYCORE_WARFRONT_PACKETS_H
