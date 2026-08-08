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

#ifndef TRINITYCORE_BATTLE_PAY_CATALOG_WRITER_H
#define TRINITYCORE_BATTLE_PAY_CATALOG_WRITER_H

#include "Define.h"
#include <string>
#include <vector>

// One field-decoded "simple shape" product record from the 68275
// SMSG_BATTLE_PAY_GET_PRODUCT_LIST_RESPONSE catalog. This is a C++ port of the byte-exact-proven
// wire writer C:/dumps/battlepay_wire.py (facts B/C/D):
//
//   normalPrice / currentPrice : u64 fixed-point (/100000) @ record+0 / +8
//   productID                  : u32 @ record+81
//   flags                      : u32 @ record+85  (BattlepayDisplayFlags)
//   name (card title)          : length = 8 bits @ record bit 428 (MSB-first); data @ record+89
//   description                : length = 8 bits @ record bit 451; data right after the name
//   name3 (card name repeat)   : == the title; its 8-bit length is packed at Mid[0..1]
//
// The record shape in the shipped 68275 template is uniform (verified for all 9 records): an 89-byte
// sub-header, then name, description, a 14-byte "mid" framing block, the title repeated, and a 4-byte
// tail. Header/Mid/Tail are carried verbatim; only the length bits inside them are rewritten.
struct BattlePayCatalogRecord
{
    uint64 NormalPrice = 0;
    uint64 CurrentPrice = 0;
    uint32 ProductID = 0;
    uint32 Flags = 0;
    std::string Name;
    std::string Description;

    std::vector<uint8> Header;      // 89-byte sub-header (length bits rewritten on serialize)
    std::vector<uint8> Mid;         // framing between description and the repeated title
    std::vector<uint8> Tail;        // trailing framing
};

// Byte-exact catalog (de)serializer. Rebuilds the leading simple-shape product records FROM FIELDS
// (a real writer, proven byte-exact against the template) and preserves everything past them verbatim.
class TC_GAME_API BattlePayCatalogWriter
{
public:
    // Parses the leading simple-shape records from a catalog body. Returns false if the body is not a
    // recognizable 68275 catalog. On success: header = the 7 leading u32s, records = decoded slots,
    // remainder = the bytes past the simple-shape prefix (complex records, kept verbatim).
    static bool Parse(std::vector<uint8> const& body, std::vector<uint32>& header,
        std::vector<BattlePayCatalogRecord>& records, std::vector<uint8>& remainder);

    // header + records (rebuilt from fields) + remainder -> catalog body.
    static std::vector<uint8> Serialize(std::vector<uint32> const& header,
        std::vector<BattlePayCatalogRecord> const& records, std::vector<uint8> const& remainder);

    // Self-check: Serialize(Parse(templateBlob)) must equal templateBlob byte-for-byte. The catalog
    // reskin approach must not be trusted unless this passes.
    static bool SelfCheck(std::vector<uint8> const& templateBlob);

private:
    static uint32 ReadBitsMsb(std::vector<uint8> const& buf, size_t bitPos, uint32 width);
    static void WriteBitsMsb(std::vector<uint8>& buf, size_t bitPos, uint32 width, uint32 val);
    static std::vector<uint8> RebuildRecord(BattlePayCatalogRecord const& rec);
};

#endif // TRINITYCORE_BATTLE_PAY_CATALOG_WRITER_H
