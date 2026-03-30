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
#include "PerksProgramMgr.h"
#include "MiscPackets.h"
#include "WorldSession.h"
#include "Player.h"
#include "CollectionMgr.h"
#include "DB2Stores.h"
#include "DBCEnums.h"
#include "ObjectAccessor.h"
#include "Log.h"
#include "UpdateFields.h"
#include "GameTime.h"

namespace
{
    bool HasAlreadyPurchased(Player const* player, int32 vendorItemID)
    {
        for (auto const& entry : player->GetPerksPurchases())
            if (entry.VendorItemID == vendorItemID)
                return true;
        return false;
    }

    void DeliverPerksReward(Player* player, WorldPackets::PerksProgram::PerksVendorItem const& item)
    {
        TC_LOG_DEBUG("network", "DeliverPerksReward: VendorItemID={}, MountID={}, ItemModifiedAppearanceID={}, TransmogSetID={}, TransmogIllusionID={}, ToyID={}, WarbandSceneID={}",
            item.VendorItemID, item.MountID, item.ItemModifiedAppearanceID, item.TransmogSetID, item.TransmogIllusionID, item.ToyID, item.WarbandSceneID);

        if (item.MountID != 0)
            player->GetSession()->GetCollectionMgr()->AddMount(item.MountID, MOUNT_NEEDS_FANFARE);

        if (item.ItemModifiedAppearanceID != 0)
        {
            if (ItemModifiedAppearanceEntry const* appearance = sItemModifiedAppearanceStore.LookupEntry(item.ItemModifiedAppearanceID))
                player->GetSession()->GetCollectionMgr()->AddItemAppearance(appearance->ItemID, appearance->ItemAppearanceModifierID);
        }

        if (item.TransmogSetID != 0)
            player->GetSession()->GetCollectionMgr()->AddTransmogSet(item.TransmogSetID);

        if (item.TransmogIllusionID != 0)
            player->GetSession()->GetCollectionMgr()->AddTransmogIllusion(item.TransmogIllusionID);

        if (item.ToyID != 0)
            player->GetSession()->GetCollectionMgr()->AddToy(item.ToyID, false, true);

        if (item.WarbandSceneID != 0)
            player->GetSession()->GetCollectionMgr()->AddWarbandScene(item.WarbandSceneID);
    }
}

void WorldSession::SendPerksProgramActivityUpdate()
{
    Player* player = GetPlayer();
    if (!player)
        return;

    WorldPackets::PerksProgram::PerksProgramActivityUpdate response;

    response.MonthEndDate   = time_t(sPerksProgramMgr->GetCurrentMonthEnd());
    response.MonthStartDate = time_t(sPerksProgramMgr->GetCurrentMonthStart());
    response.UnknownField   = 0;

    PerksProgramActivityLists activityLists = sPerksProgramMgr->GetCurrentMonthActivityLists();
    response.ActivityIDs.assign(activityLists.MainActivityIDs.begin(), activityLists.MainActivityIDs.end());

    {
        std::vector<int32> completed;
        for (int32 id : activityLists.ThresholdActivityIDs)
            if (player->HasPerksMilestone(id))
                completed.push_back(id);

        response.IntervalActivityID1 = completed.size() > 0 ? completed[0] : 0;
        response.IntervalActivityID2 = completed.size() > 1 ? completed[1] : 0;
        response.IntervalActivityID3 = completed.size() > 2 ? completed[2] : 0;
        response.IntervalActivityID4 = completed.size() > 3 ? completed[3] : 0;
        response.IntervalActivityID5 = completed.size() > 4 ? completed[4] : 0;
    }

    TC_LOG_ERROR("misc", "SendPerksProgramActivityUpdate: player={} MonthStart={} MonthEnd={} ActivityCount={}",
        player->GetName(), uint32(sPerksProgramMgr->GetCurrentMonthStart()), uint32(sPerksProgramMgr->GetCurrentMonthEnd()), uint32(response.ActivityIDs.size()));

    SendPacket(response.Write());
}

void WorldSession::HandleRequestStoreFrontInfoUpdate(WorldPackets::PerksProgram::RequestStoreFrontInfoUpdate& /*packet*/)
{
    WorldPackets::PerksProgram::AccountStoreFrontUpdate response;
    SendPacket(response.Write());
}

void WorldSession::HandlePerksProgramStatusRequest(WorldPackets::PerksProgram::PerksProgramStatusRequest& /*packet*/)
{
    SendPerksProgramActivityUpdate();
}

void WorldSession::HandlePerksProgramReqestPendingRewards(WorldPackets::Misc::PerksProgramReqestPendingRewards& /*packet*/)
{
    WorldPackets::PerksProgram::ResponsePerkPendingRewards response;
    response.PendingRewardCount = 0;
    SendPacket(response.Write());
}

void WorldSession::HandlePerksProgramGetRecentPurchases(WorldPackets::PerksProgram::PerksProgramGetRecentPurchases& /*packet*/)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    WorldPackets::PerksProgram::ResponsePerkRecentPurchases response;

    for (auto const& entry : player->GetPerksPurchases())
    {
        WorldPackets::PerksProgram::PerksShortItem shortItem;
        shortItem.VendorItemID = entry.VendorItemID;
        shortItem.PurchaseTime = time_t(entry.PurchaseTime);
        shortItem.Flags = 0x80; // Bit 7 = refundable/purchased flag (from sniff)
        response.RecentPurchases.push_back(shortItem);
    }

    SendPacket(response.Write());
}

void WorldSession::HandlePerksProgramRequestPurchase(WorldPackets::PerksProgram::PerksProgramRequestPurchase& packet)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    TC_LOG_DEBUG("network", "HandlePerksProgramRequestPurchase: VendorItemID={}, VendorGuid={}",
        packet.VendorItemID, packet.VendorGuid.ToString());

    PerksProgramVendorItemData const* vendorItem = sPerksProgramMgr->GetVendorItem(packet.VendorItemID);
    if (!vendorItem)
    {
        TC_LOG_DEBUG("network", "HandlePerksProgramRequestPurchase: VendorItemID {} not found.", packet.VendorItemID);
        return;
    }

    if (vendorItem->PacketData.Disabled)
    {
        TC_LOG_DEBUG("network", "HandlePerksProgramRequestPurchase: VendorItemID {} is disabled.", packet.VendorItemID);
        return;
    }

    if (!HasAlreadyPurchased(player, packet.VendorItemID))
    {
        if (!player->ModifyPerksCurrency(-vendorItem->PacketData.Price))
        {
            TC_LOG_DEBUG("network", "HandlePerksProgramRequestPurchase: Player {} has insufficient Traders Tender for item {}.",
                player->GetGUID().ToString(), packet.VendorItemID);
            return;
        }

        uint32 purchaseTime = uint32(GameTime::GetGameTime());
        player->AddPerksPurchase(packet.VendorItemID, purchaseTime);
        TC_LOG_DEBUG("network", "HandlePerksProgramRequestPurchase: About to DeliverPerksReward for VendorItemID {}", packet.VendorItemID);
        DeliverPerksReward(player, vendorItem->PacketData);
        TC_LOG_DEBUG("network", "HandlePerksProgramRequestPurchase: DeliverPerksReward done");
    }
    else
    {
        TC_LOG_DEBUG("network", "HandlePerksProgramRequestPurchase: Player {} already purchased VendorItemID {}, sending existing list.",
            player->GetGUID().ToString(), packet.VendorItemID);
    }

    // Send purchase confirmation (Type 0x20)
    WorldPackets::PerksProgram::PerksProgramResultPurchase response;
    response.PurchasedVendorItemID = packet.VendorItemID;

    for (auto const& entry : player->GetPerksPurchases())
    {
        WorldPackets::PerksProgram::PerksShortItem shortItem;
        shortItem.VendorItemID = entry.VendorItemID;
        shortItem.PurchaseTime = time_t(entry.PurchaseTime);
        shortItem.Flags = 0x80;
        response.PurchasedItems.push_back(shortItem);
    }
    SendPacket(response.Write());
    SendPerksProgramVendorList(packet.VendorGuid);
}

void WorldSession::HandlePerksProgramRequestCartCheckout(WorldPackets::PerksProgram::PerksProgramRequestCartCheckout& packet)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    TC_LOG_DEBUG("network", "HandlePerksProgramRequestCartCheckout: {} items, VendorGuid={}",
        packet.VendorItemIDs.size(), packet.VendorGuid.ToString());
    int32 totalCost = 0;
    std::vector<PerksProgramVendorItemData const*> validItems;
    for (int32 vendorItemID : packet.VendorItemIDs)
    {
        PerksProgramVendorItemData const* vendorItem = sPerksProgramMgr->GetVendorItem(vendorItemID);
        if (!vendorItem || vendorItem->PacketData.Disabled)
        {
            TC_LOG_DEBUG("network", "HandlePerksProgramRequestCartCheckout: VendorItemID {} invalid or disabled.", vendorItemID);
            return;
        }
        if (HasAlreadyPurchased(player, vendorItemID))
        {
            TC_LOG_DEBUG("network", "HandlePerksProgramRequestCartCheckout: Player {} already purchased VendorItemID {}.",
                player->GetGUID().ToString(), vendorItemID);
            return;
        }
        totalCost += vendorItem->PacketData.Price;
        validItems.push_back(vendorItem);
    }

    if (!player->ModifyPerksCurrency(-totalCost))
    {
        TC_LOG_DEBUG("network", "HandlePerksProgramRequestCartCheckout: Player {} has insufficient Traders Tender (need {}).",
            player->GetGUID().ToString(), totalCost);
        return;
    }

    uint32 purchaseTime = uint32(GameTime::GetGameTime());
    for (PerksProgramVendorItemData const* item : validItems)
    {
        player->AddPerksPurchase(item->PacketData.VendorItemID, purchaseTime);
        DeliverPerksReward(player, item->PacketData);
    }

    for (PerksProgramVendorItemData const* item : validItems)
    {
        WorldPackets::PerksProgram::PerksProgramResultPurchase response;
        response.PurchasedVendorItemID = item->PacketData.VendorItemID;

        for (auto const& entry : player->GetPerksPurchases())
        {
            WorldPackets::PerksProgram::PerksShortItem shortItem;
            shortItem.VendorItemID = entry.VendorItemID;
            shortItem.PurchaseTime = time_t(entry.PurchaseTime);
            shortItem.Flags = 0x80;
            response.PurchasedItems.push_back(shortItem);
        }

        SendPacket(response.Write());
    }

    SendPerksProgramVendorList(packet.VendorGuid);
}

void WorldSession::HandlePerksProgramSetFrozenVendorItem(WorldPackets::PerksProgram::PerksProgramSetFrozenVendorItem& packet)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    TC_LOG_DEBUG("network", "HandlePerksProgramSetFrozenVendorItem: VendorItemID={}, IsFreezing={}",
        packet.VendorItemID, packet.IsFreezing ? "true" : "false");

    if (packet.IsFreezing)
    {
        PerksProgramVendorItemData const* vendorItem = sPerksProgramMgr->GetVendorItem(packet.VendorItemID);
        if (!vendorItem)
        {
            TC_LOG_DEBUG("network", "HandlePerksProgramSetFrozenVendorItem: VendorItemID {} not found.", packet.VendorItemID);
            return;
        }

        player->SetPerksFrozenVendorItem(packet.VendorItemID, &vendorItem->PacketData);
    }
    else
    {
        player->SetPerksFrozenVendorItem(0, nullptr);
    }
}

void WorldSession::HandlePerksProgramItemsRefreshed(WorldPackets::PerksProgram::PerksProgramItemsRefreshed& /*packet*/)
{
    TC_LOG_DEBUG("network", "HandlePerksProgramItemsRefreshed: stub");
}

void WorldSession::HandlePerksProgramRequestRefund(WorldPackets::PerksProgram::PerksProgramRequestRefund& /*packet*/)
{
    TC_LOG_DEBUG("network", "HandlePerksProgramRequestRefund: stub (not implemented)");
}

void WorldSession::SendPerksAnimToggleKillSwitch()
{
    WorldPackets::PerksProgram::PerksAnimToggleKillSwitch packet;
    packet.AnimEnabled = true;
    packet.Unknown = true;
    SendPacket(packet.Write());
}

void WorldSession::SendPerksProgramCurrencyUpdate()
{
    Player* player = GetPlayer();
    if (!player)
        return;

    WorldPackets::PerksProgram::PerksProgramResultCurrencyUpdate packet;
    packet.PurchasedCount    = player->GetPerksPurchasedCount();
    packet.PurchasedCountCap = player->GetPerksPurchasedCount(); // Same as count (no cap observed in sniff)
    packet.CurrencyAmount    = player->GetCurrencyQuantity(CURRENCY_TRADERS_TENDER);
    packet.TotalEarned       = player->GetPerksTotalEarned();
    packet.UnknownFlags      = 0;
    SendPacket(packet.Write());
}

void WorldSession::SendPerksProgramVendorList(ObjectGuid vendorGuid)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    WorldPackets::PerksProgram::PerksProgramResultVendorList packet;
    packet.VendorGuid = vendorGuid;
    packet.PlayerGuid = player->GetGUID();
    int32 frozenID = player->GetPerksFrozenVendorItemID();
    if (frozenID != 0)
    {
        if (PerksProgramVendorItemData const* frozenData = sPerksProgramMgr->GetVendorItem(frozenID))
            packet.FrozenItem = frozenData->PacketData;
    }
    std::vector<PerksProgramVendorItemData const*> rotationItems = sPerksProgramMgr->GetCurrentRotationItems();
    for (PerksProgramVendorItemData const* item : rotationItems)
        packet.Items.push_back(item->PacketData);

    TC_LOG_DEBUG("network", "SendPerksProgramVendorList: Sending {} items, frozenID={}, VendorGuid={}, PlayerGuid={}",
        packet.Items.size(), frozenID, vendorGuid.ToString(), player->GetGUID().ToString());

    SendPacket(packet.Write());
}
