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

#include "PerksProgramPackets.h"
#include "PacketOperators.h"

namespace WorldPackets::PerksProgram
{

void PerksProgramRequestPurchase::Read()
{
    _worldPacket >> VendorItemID;
    _worldPacket >> VendorGuid;
}

void PerksProgramRequestCartCheckout::Read()
{
    uint32 itemCount;
    _worldPacket >> itemCount;
    _worldPacket >> VendorGuid;

    VendorItemIDs.resize(itemCount);
    for (uint32 i = 0; i < itemCount; ++i)
        _worldPacket >> VendorItemIDs[i];
}

void PerksProgramSetFrozenVendorItem::Read()
{
    _worldPacket >> Bits<1>(IsFreezing);
    _worldPacket.ResetBitPos();
    _worldPacket >> VendorItemID;
    _worldPacket >> Unknown;
}

WorldPacket const* AccountStoreFrontUpdate::Write()
{
    _worldPacket << uint8(Status);
    _worldPacket << uint32(StoreFrontID);
    _worldPacket << uint64(Expiry);
    _worldPacket << Bits<1>(Flag1);
    _worldPacket << Bits<1>(Flag2);
    _worldPacket.FlushBits();

    return &_worldPacket;
}

WorldPacket const* PerksAnimToggleKillSwitch::Write()
{
    _worldPacket << Bits<1>(AnimEnabled);
    _worldPacket << Bits<1>(Unknown);
    _worldPacket.FlushBits();

    return &_worldPacket;
}

WorldPacket const* PerksProgramActivityUpdate::Write()
{
    _worldPacket << uint32(5 + ActivityIDs.size());
    _worldPacket << MonthEndDate;
    _worldPacket << MonthStartDate;
    _worldPacket << uint32(UnknownField);

    for (uint32 activityID : ActivityIDs)
        _worldPacket << uint32(activityID);
    _worldPacket << int32(IntervalActivityID1);
    _worldPacket << int32(IntervalActivityID2);
    _worldPacket << int32(IntervalActivityID3);
    _worldPacket << int32(IntervalActivityID4);
    _worldPacket << int32(IntervalActivityID5);

    return &_worldPacket;
}

WorldPacket const* PerksProgramResultCurrencyUpdate::Write()
{
    _worldPacket << uint8(0x40); // Type
    _worldPacket << uint32(PurchasedCount);
    _worldPacket << uint32(PurchasedCountCap);
    _worldPacket << uint32(CurrencyAmount);
    _worldPacket << uint32(TotalEarned);
    _worldPacket << uint8(UnknownFlags);

    return &_worldPacket;
}

WorldPacket const* PerksProgramResultVendorList::Write()
{
    _worldPacket << uint8(0x50);
    _worldPacket << VendorGuid;
    _worldPacket << PlayerGuid;
    _worldPacket << uint32(Items.size());
    _worldPacket << FrozenItem.AvailableUntil;
    _worldPacket << int32(FrozenItem.VendorItemID);
    _worldPacket << int32(FrozenItem.MountID);
    _worldPacket << int32(FrozenItem.BattlePetSpeciesID);
    _worldPacket << int32(FrozenItem.TransmogSetID);
    _worldPacket << int32(FrozenItem.ItemModifiedAppearanceID);
    for (PerksVendorItem const& item : Items)
        _worldPacket << item;

    return &_worldPacket;
}

WorldPacket const* PerksProgramResultPurchase::Write()
{
    _worldPacket << uint8(0x20);
    _worldPacket << int32(PurchasedVendorItemID);
    _worldPacket << uint32(PurchasedItems.size());

    for (PerksShortItem const& item : PurchasedItems)
    {
        _worldPacket << int32(item.VendorItemID);
        _worldPacket << item.PurchaseTime;
        _worldPacket << uint8(item.Flags);
    }

    return &_worldPacket;
}

WorldPacket const* ResponsePerkPendingRewards::Write()
{
    _worldPacket << uint32(PendingRewardCount);

    return &_worldPacket;
}

WorldPacket const* ResponsePerkRecentPurchases::Write()
{
    _worldPacket << uint32(RecentPurchases.size());

    for (PerksShortItem const& item : RecentPurchases)
    {
        _worldPacket << int32(item.VendorItemID);
        _worldPacket << item.PurchaseTime;
        _worldPacket << uint8(item.Flags);
    }

    return &_worldPacket;
}
}