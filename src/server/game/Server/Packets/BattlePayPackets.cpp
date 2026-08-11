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

#include "BattlePayPackets.h"
#include "StringFormat.h"
#include "Log.h"

namespace WorldPackets::BattlePay
{
WorldPacket const* ProductListResponse::Write()
{
    if (RawData && !RawData->empty())
        _worldPacket.append(RawData->data(), RawData->size());

    return &_worldPacket;
}

WorldPacket const* GetDistributionListResponse::Write()
{
    if (RawData && !RawData->empty())
        _worldPacket.append(RawData->data(), RawData->size());

    return &_worldPacket;
}

void StartPurchase::Read()
{
    // DIAGNOSTIC: the live client sends ~505 bytes here while this reader consumes 17
    // ("Unprocessed tail data (read stop at 17 from 505)"), and the leading u32 arrives as 1 - so it
    // is not the productID. The real layout was never captured: retail routes purchases through
    // CMSG_BATTLE_PAY_OPEN_CHECKOUT (web checkout), so no sniff in this workspace contains a
    // START_PURCHASE body. Dump the whole payload once per request so the layout can be derived from
    // a real one instead of guessed. Remove when the structure is settled.
    std::size_t const size = _worldPacket.size();
    std::string hex;
    hex.reserve(size * 3);
    for (std::size_t i = 0; i < size; ++i)
        hex += Trinity::StringFormat("{:02X} ", _worldPacket.data()[i]);

    TC_LOG_INFO("network", "BattlePay: StartPurchase RAW ({} bytes): {}", size, hex);

    _worldPacket >> ProductID;
    _worldPacket >> ScalarU64;
    Flag = _worldPacket.ReadBit();
    _worldPacket.rfinish();     // consume the rest so the tail-data warning does not mask the dump
}

void OpenCheckout::Read()
{
    _worldPacket >> ClientToken;
}

WorldPacket const* StartPurchaseResponse::Write()
{
    _worldPacket << ResultA;
    _worldPacket << ResultB;
    _worldPacket << PurchaseID;

    return &_worldPacket;
}

// INFERRED layout - see the ConfirmPurchase comment in the header. Gated off by default.
WorldPacket const* ConfirmPurchase::Write()
{
    _worldPacket << PurchaseID;
    _worldPacket << ProductID;
    _worldPacket << CurrentPriceFixedPoint;
    _worldPacket << ServerToken;
    _worldPacket.WriteBits(0u, 8);      // walletName length (empty), bit-packed like the other records
    _worldPacket.FlushBits();

    return &_worldPacket;
}

void ConfirmPurchaseResponse::Read()
{
    _worldPacket >> ServerToken;
    Confirmed = _worldPacket.ReadBit();
}

// Record order proven against the live 68974 purchase list (TESTER_SNIFF2_LINDORMI_MINE, 458 B =
// 8 + 10x45): { u64 PurchaseID, i32 Status, i32 ResultCode, u32 ProductID, u64 BasePrice,
// u64 UserPrice, i64 TimeCreated, u8 walletNameLen }. walletName sits at the END of the record -
// in all 10 live records the unix purchase time aligns at record offset 36 and byte 44 is the
// empty-wallet 0; with the u8 after ProductID the time would start at 37, one byte late.
// Shares the JamBattlePayPurchase record layout with PurchaseUpdate::Write (walletName length
// record-final - see the comment there). Answered honestly-empty today (no purchase ledger yet).
WorldPacket const* GetPurchaseListResponse::Write()
{
    _worldPacket << Result;
    _worldPacket << uint32(Purchases.size());
    for (PurchaseRecord const& p : Purchases)
    {
        _worldPacket << p.PurchaseID;
        _worldPacket << p.Status;
        _worldPacket << p.ResultCode;
        _worldPacket << p.ProductID;
        _worldPacket << p.BasePrice;
        _worldPacket << p.UserPrice;
        _worldPacket << p.TimeCreated;
        _worldPacket << uint8(0);       // walletName: empty (8-bit length primitive, value 0), record-final
    }

    return &_worldPacket;
}

WorldPacket const* PurchaseUpdate::Write()
{
    _worldPacket << Result;
    _worldPacket << uint32(Purchases.size());
    for (PurchaseRecord const& p : Purchases)
    {
        _worldPacket << p.PurchaseID;
        _worldPacket << p.Status;
        _worldPacket << p.ResultCode;
        _worldPacket << p.ProductID;
        _worldPacket << p.BasePrice;
        _worldPacket << p.UserPrice;
        _worldPacket << p.TimeCreated;
        _worldPacket << uint8(0);       // walletName: empty (8-bit length primitive, value 0), record-final
    }

    return &_worldPacket;
}
}
