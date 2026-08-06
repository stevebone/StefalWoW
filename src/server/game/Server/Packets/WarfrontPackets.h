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
    // !! ENTIRELY INFERRED (needs sniff validation) - see WARFRONT_OPCODE_SPEC.md §C / §E.
    // The client has NO serializer, NO 0x811C9DC5 reflection descriptor and NO jam_types row for this opcode offline;
    // its handler is a lazy-init thunk (0x7FF7290A6B90). Historically the message is small - a single uint32
    // battlefield/warfront id (possibly empty). Gated behind Warfront.NativeUI.Enable (default 0).
    class WarfrontComplete final : public ServerPacket
    {
    public:
        explicit WarfrontComplete() : ServerPacket(SMSG_WARFRONT_COMPLETE, 4) { }

        WorldPacket const* Write() override;

        uint32 BattlefieldId = 0;   // INFERRED (needs sniff validation) - see WARFRONT_OPCODE_SPEC.md §C
    };
}
}

#endif // TRINITYCORE_WARFRONT_PACKETS_H
