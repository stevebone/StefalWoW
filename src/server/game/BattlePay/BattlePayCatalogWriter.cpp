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

#include "BattlePayCatalogWriter.h"
#include "Log.h"
#include <algorithm>
#include <cstring>

namespace
{
    constexpr uint32 HDR_FIELDS    = 7;     // result, unk4, count0..count4 (u32 @ 0..27)
    constexpr size_t PROD_START    = 28;
    constexpr size_t HDR_SIZE      = 89;    // per-record sub-header
    constexpr size_t NAME1_LEN_BIT = 428;   // record-relative bit offset of the name length (8 bit)
    constexpr size_t NAME2_LEN_BIT = 451;   // record-relative bit offset of the description length
    constexpr size_t FRAME_AFTER_BLOCKA = 18;  // mid + tail total between blockA and record end
    constexpr uint32 MAX_SIMPLE    = 9;     // simple-shape slots decodable in the 68275 template

    uint32 ReadU32LE(std::vector<uint8> const& b, size_t o)
    {
        return uint32(b[o]) | (uint32(b[o + 1]) << 8) | (uint32(b[o + 2]) << 16) | (uint32(b[o + 3]) << 24);
    }

    uint64 ReadU64LE(std::vector<uint8> const& b, size_t o)
    {
        uint64 v = 0;
        for (size_t i = 0; i < 8; ++i)
            v |= uint64(b[o + i]) << (8 * i);
        return v;
    }

    void WriteU32LE(std::vector<uint8>& b, size_t o, uint32 v)
    {
        for (size_t i = 0; i < 4; ++i)
            b[o + i] = uint8((v >> (8 * i)) & 0xFF);
    }

    void WriteU64LE(std::vector<uint8>& b, size_t o, uint64 v)
    {
        for (size_t i = 0; i < 8; ++i)
            b[o + i] = uint8((v >> (8 * i)) & 0xFF);
    }

    bool Printable(uint8 c) { return c >= 32 && c < 127; }
}

uint32 BattlePayCatalogWriter::ReadBitsMsb(std::vector<uint8> const& buf, size_t bitPos, uint32 width)
{
    uint32 v = 0;
    for (uint32 i = 0; i < width; ++i)
    {
        size_t const p = bitPos + i;
        uint32 const bit = (buf[p >> 3] >> (7 - (p & 7))) & 1;
        v = (v << 1) | bit;
    }
    return v;
}

void BattlePayCatalogWriter::WriteBitsMsb(std::vector<uint8>& buf, size_t bitPos, uint32 width, uint32 val)
{
    for (uint32 i = 0; i < width; ++i)
    {
        size_t const p = bitPos + i;
        uint32 const bit = (val >> (width - 1 - i)) & 1;
        uint8 const mask = uint8(1 << (7 - (p & 7)));
        uint8& target = buf[p >> 3];
        target = uint8((target & ~mask) | (bit ? mask : 0));
    }
}

bool BattlePayCatalogWriter::Parse(std::vector<uint8> const& body, std::vector<uint32>& header,
    std::vector<BattlePayCatalogRecord>& records, std::vector<uint8>& remainder)
{
    header.clear();
    records.clear();
    remainder.clear();

    if (body.size() < PROD_START)
        return false;

    header.reserve(HDR_FIELDS);
    for (uint32 i = 0; i < HDR_FIELDS; ++i)
        header.push_back(ReadU32LE(body, i * 4));

    size_t pos = PROD_START;
    while (records.size() < MAX_SIMPLE && pos + HDR_SIZE < body.size())
    {
        // Name/description lengths from the record's bit section (fact B).
        uint32 const n1 = ReadBitsMsb(body, pos * 8 + NAME1_LEN_BIT, 8);
        uint32 const n2 = ReadBitsMsb(body, pos * 8 + NAME2_LEN_BIT, 8);

        size_t const blockA = pos + HDR_SIZE;
        size_t const blockAEnd = blockA + n1 + n2;
        size_t const recEnd = blockAEnd + FRAME_AFTER_BLOCKA + n1;  // mid + name3 + tail

        // Sanity: a simple-shape record must fit and start with a printable title of the stated length.
        if (n1 == 0 || recEnd > body.size())
            break;
        bool titlePrintable = true;
        for (uint32 i = 0; i < n1 && titlePrintable; ++i)
            titlePrintable = Printable(body[blockA + i]);
        if (!titlePrintable)
            break;

        std::string const name(reinterpret_cast<char const*>(&body[blockA]), n1);
        std::string const description(reinterpret_cast<char const*>(&body[blockA + n1]), n2);

        // Locate the repeated title (name3) within the post-blockA framing; mid = bytes before it,
        // tail = bytes after. Any valid location reproduces the region byte-exact on serialize.
        size_t name3Pos = std::string::npos;
        for (size_t off = blockAEnd; off + n1 <= recEnd; ++off)
        {
            if (std::memcmp(&body[off], name.data(), n1) == 0)
            {
                name3Pos = off;
                break;
            }
        }
        if (name3Pos == std::string::npos || name3Pos < blockAEnd + 2)   // need >=2 mid bytes for the name3 length
            break;

        BattlePayCatalogRecord rec;
        rec.NormalPrice  = ReadU64LE(body, pos + 0);
        rec.CurrentPrice = ReadU64LE(body, pos + 8);
        rec.ProductID    = ReadU32LE(body, pos + 81);
        rec.Flags        = ReadU32LE(body, pos + 85);
        rec.Name         = name;
        rec.Description  = description;
        rec.Header.assign(body.begin() + pos, body.begin() + blockA);
        rec.Mid.assign(body.begin() + blockAEnd, body.begin() + name3Pos);
        rec.Tail.assign(body.begin() + name3Pos + n1, body.begin() + recEnd);

        records.push_back(std::move(rec));
        pos = recEnd;
    }

    remainder.assign(body.begin() + pos, body.end());
    return !records.empty();
}

std::vector<uint8> BattlePayCatalogWriter::RebuildRecord(BattlePayCatalogRecord const& rec)
{
    uint32 const n1 = uint32(rec.Name.size());
    uint32 const n2 = uint32(rec.Description.size());

    std::vector<uint8> hdr = rec.Header;
    WriteU64LE(hdr, 0, rec.NormalPrice);
    WriteU64LE(hdr, 8, rec.CurrentPrice);
    WriteU32LE(hdr, 81, rec.ProductID);
    WriteU32LE(hdr, 85, rec.Flags);
    WriteBitsMsb(hdr, NAME1_LEN_BIT, 8, n1 & 0xFF);
    WriteBitsMsb(hdr, NAME2_LEN_BIT, 8, n2 & 0xFF);

    std::vector<uint8> mid = rec.Mid;
    // name3 (repeated title) length packed at mid[0..1] (inverse of battlepay_wire.py decode_name_len).
    uint32 const n3 = n1;
    if (mid.size() >= 2)
    {
        mid[0] = uint8((mid[0] & 0xC0) | ((n3 >> 2) & 0x3F));
        mid[1] = uint8((mid[1] & 0x3F) | ((n3 & 3) << 6));
    }

    std::vector<uint8> out;
    out.reserve(hdr.size() + n1 + n2 + mid.size() + n1 + rec.Tail.size());
    out.insert(out.end(), hdr.begin(), hdr.end());
    out.insert(out.end(), rec.Name.begin(), rec.Name.end());
    out.insert(out.end(), rec.Description.begin(), rec.Description.end());
    out.insert(out.end(), mid.begin(), mid.end());
    out.insert(out.end(), rec.Name.begin(), rec.Name.end());     // name3 == title
    out.insert(out.end(), rec.Tail.begin(), rec.Tail.end());
    return out;
}

std::vector<uint8> BattlePayCatalogWriter::Serialize(std::vector<uint32> const& header,
    std::vector<BattlePayCatalogRecord> const& records, std::vector<uint8> const& remainder)
{
    std::vector<uint8> out;
    out.reserve(HDR_FIELDS * 4 + remainder.size() + records.size() * 200);
    for (uint32 i = 0; i < HDR_FIELDS; ++i)
    {
        uint32 const v = i < header.size() ? header[i] : 0;
        for (size_t b = 0; b < 4; ++b)
            out.push_back(uint8((v >> (8 * b)) & 0xFF));
    }
    for (BattlePayCatalogRecord const& rec : records)
    {
        std::vector<uint8> const recBytes = RebuildRecord(rec);
        out.insert(out.end(), recBytes.begin(), recBytes.end());
    }
    out.insert(out.end(), remainder.begin(), remainder.end());
    return out;
}

bool BattlePayCatalogWriter::SelfCheck(std::vector<uint8> const& templateBlob)
{
    std::vector<uint32> header;
    std::vector<BattlePayCatalogRecord> records;
    std::vector<uint8> remainder;
    if (!Parse(templateBlob, header, records, remainder))
    {
        TC_LOG_ERROR("server.loading", "BattlePayCatalogWriter: self-check FAILED - template did not parse.");
        return false;
    }

    std::vector<uint8> const rebuilt = Serialize(header, records, remainder);
    if (rebuilt == templateBlob)
    {
        TC_LOG_INFO("server.loading", "BattlePayCatalogWriter: self-check PASS - {} simple slots, byte-exact round trip.",
            records.size());
        return true;
    }

    size_t firstDiff = 0;
    size_t const cmpLen = std::min(rebuilt.size(), templateBlob.size());
    while (firstDiff < cmpLen && rebuilt[firstDiff] == templateBlob[firstDiff])
        ++firstDiff;
    TC_LOG_ERROR("server.loading", "BattlePayCatalogWriter: self-check FAILED - sizes {}/{}, first diff @ byte {}.",
        rebuilt.size(), templateBlob.size(), firstDiff);
    return false;
}
