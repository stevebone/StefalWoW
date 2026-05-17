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

#include "MajorFactionPackets.h"

namespace WorldPackets::MajorFactions
{
    WorldPacket const* CovenantRenownSendCatchupState::Write()
    {
        // Build the payload first so we can compute the length-prefix byte.
        ByteBuffer payload;
        for (Entry const& entry : Entries)
        {
            payload << int32(entry.FactionID);
            payload << int32(entry.CatchupPercent);
        }

        // Header byte: (payloadLen << 1) & 0xFE. The client masks bit 0
        // off and right-shifts by one to recover the length. The 7-bit
        // length field caps total payload at 127 bytes (15 entries).
        ASSERT(payload.size() <= 127, "Catchup state payload exceeds 127 bytes (%zu bytes for %zu entries)",
            payload.size(), Entries.size());

        uint8 header = uint8((payload.size() << 1) & 0xFE);
        _worldPacket << header;
        _worldPacket.append(payload.data(), payload.size());
        return &_worldPacket;
    }
}
