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

#ifndef TRINITYCORE_MAJOR_FACTION_PACKETS_H
#define TRINITYCORE_MAJOR_FACTION_PACKETS_H

#include "Packet.h"
#include <vector>

namespace WorldPackets
{
    namespace MajorFactions
    {
        // CMSG_COVENANT_RENOWN_REQUEST_CATCHUP_STATE (0x3B0111).
        // The client sends this when opening the Renown UI (covenant or major
        // faction). Empty payload - the server replies with the current
        // catchup state for every applicable faction.
        class RequestCatchupState final : public ClientPacket
        {
        public:
            explicit RequestCatchupState(WorldPacket&& packet) : ClientPacket(CMSG_COVENANT_RENOWN_REQUEST_CATCHUP_STATE, std::move(packet)) { }

            void Read() override { }
        };

        // SMSG_COVENANT_RENOWN_SEND_CATCHUP_STATE (0x42030D).
        //
        // Wire format derived from IDA decomp of ParseCovenantRenownCatchupState
        // (sub_7FF75C0EB140) at build 12.0.5.67186:
        //   uint8  Header   - (PayloadLen << 1) & 0xFE; top 7 bits = byte length,
        //                     bit 0 always 0 (reserved).
        //   uint8  Payload[PayloadLen]  - array of inner records, each:
        //       int32 FactionID
        //       int32 CatchupPercent  (0..100; client divides by 100 if
        //                              Faction.dynamicFlags & 8, else uses raw)
        //
        // The header's 7-bit length limits the total payload to 127 bytes,
        // i.e. up to 15 (factionID, percent) records per packet. With 20
        // major factions today this is sufficient for any realistic catchup
        // state because only factions where char renown < account renown
        // are emitted.
        class CovenantRenownSendCatchupState final : public ServerPacket
        {
        public:
            struct Entry
            {
                int32 FactionID;
                int32 CatchupPercent;
            };

            CovenantRenownSendCatchupState()
                : ServerPacket(SMSG_COVENANT_RENOWN_SEND_CATCHUP_STATE, 1 + 15 * 8) { }

            WorldPacket const* Write() override;

            std::vector<Entry> Entries;
        };
    }
}

#endif // TRINITYCORE_MAJOR_FACTION_PACKETS_H