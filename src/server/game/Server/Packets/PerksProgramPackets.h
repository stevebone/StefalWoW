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

#ifndef TRINITYCORE_PERKS_PROGRAM_PACKETS_H
#define TRINITYCORE_PERKS_PROGRAM_PACKETS_H

#include "ObjectGuid.h"
#include "Packet.h"
#include "PacketUtilities.h"
#include "PerksProgramPacketsCommon.h"

namespace WorldPackets
{
    namespace PerksProgram
    {
        struct PerksShortItem
        {
            int32 VendorItemID = 0;
            Timestamp<> PurchaseTime;
            uint8 Flags = 0;
        };

        class PerksProgramStatusRequest final : public ClientPacket
        {
        public:
            explicit PerksProgramStatusRequest(WorldPacket&& packet) : ClientPacket(CMSG_PERKS_PROGRAM_STATUS_REQUEST, std::move(packet)) { }

            void Read() override { }
        };

        class PerksProgramGetRecentPurchases final : public ClientPacket
        {
        public:
            explicit PerksProgramGetRecentPurchases(WorldPacket&& packet) : ClientPacket(CMSG_PERKS_PROGRAM_GET_RECENT_PURCHASES, std::move(packet)) { }

            void Read() override { }
        };

        class PerksProgramRequestPurchase final : public ClientPacket
        {
        public:
            explicit PerksProgramRequestPurchase(WorldPacket&& packet) : ClientPacket(CMSG_PERKS_PROGRAM_REQUEST_PURCHASE, std::move(packet)) { }

            void Read() override;

            int32 VendorItemID = 0;
            ObjectGuid VendorGuid;
        };

        class PerksProgramRequestCartCheckout final : public ClientPacket
        {
        public:
            explicit PerksProgramRequestCartCheckout(WorldPacket&& packet) : ClientPacket(CMSG_PERKS_PROGRAM_REQUEST_CART_CHECKOUT, std::move(packet)) { }

            void Read() override;

            ObjectGuid VendorGuid;
            std::vector<int32> VendorItemIDs;
        };

        class PerksProgramSetFrozenVendorItem final : public ClientPacket
        {
        public:
            explicit PerksProgramSetFrozenVendorItem(WorldPacket&& packet) : ClientPacket(CMSG_PERKS_PROGRAM_SET_FROZEN_VENDOR_ITEM, std::move(packet)) { }

            void Read() override;

            int32 VendorItemID = 0;
            uint16 Unknown = 0;
            bool IsFreezing = false;
        };

        class PerksProgramItemsRefreshed final : public ClientPacket
        {
        public:
            explicit PerksProgramItemsRefreshed(WorldPacket&& packet) : ClientPacket(CMSG_PERKS_PROGRAM_ITEMS_REFRESHED, std::move(packet)) { }

            void Read() override { }
        };

        class PerksProgramRequestRefund final : public ClientPacket
        {
        public:
            explicit PerksProgramRequestRefund(WorldPacket&& packet) : ClientPacket(CMSG_PERKS_PROGRAM_REQUEST_REFUND, std::move(packet)) { }

            void Read() override { _worldPacket.rfinish(); }
        };

        class RequestStoreFrontInfoUpdate final : public ClientPacket
        {
        public:
            explicit RequestStoreFrontInfoUpdate(WorldPacket&& packet) : ClientPacket(CMSG_REQUEST_STORE_FRONT_INFO_UPDATE, std::move(packet)) { }

            void Read() override { _worldPacket.rfinish(); }
        };

        class AccountStoreFrontUpdate final : public ServerPacket
        {
        public:
            explicit AccountStoreFrontUpdate() : ServerPacket(SMSG_ACCOUNT_STORE_FRONT_UPDATE, 14) { }

            WorldPacket const* Write() override;

            uint8  Status       = 2;
            uint32 StoreFrontID = 1;
            uint64 Expiry       = 0;
            bool   Flag1        = true;
            bool   Flag2        = true;
        };

        class PerksAnimToggleKillSwitch final : public ServerPacket
        {
        public:
            explicit PerksAnimToggleKillSwitch() : ServerPacket(SMSG_PERKS_ANIM_TOGGLE_KILL_SWITCH, 1) { }

            WorldPacket const* Write() override;

            bool AnimEnabled = true;
            bool Unknown = true;
        };

        class PerksProgramActivityUpdate final : public ServerPacket
        {
        public:
            explicit PerksProgramActivityUpdate() : ServerPacket(SMSG_PERKS_PROGRAM_ACTIVITY_UPDATE, 316) { }

            WorldPacket const* Write() override;

            Timestamp<> MonthEndDate;
            Timestamp<> MonthStartDate;
            uint32 UnknownField = 0;
            std::vector<uint32> ActivityIDs;
            int32 IntervalActivityID1 = 0;
            int32 IntervalActivityID2 = 0;
            int32 IntervalActivityID3 = 0;
            int32 IntervalActivityID4 = 0;
            int32 IntervalActivityID5 = 0;
        };

        class PerksProgramResultCurrencyUpdate final : public ServerPacket
        {
        public:
            explicit PerksProgramResultCurrencyUpdate() : ServerPacket(SMSG_PERKS_PROGRAM_RESULT, 17) { }

            WorldPacket const* Write() override;

            uint32 PurchasedCount = 0;
            uint32 PurchasedCountCap = 0;
            uint32 CurrencyAmount = 0;
            uint32 TotalEarned = 0;
            uint8 UnknownFlags = 0;
        };

        class PerksProgramResultVendorList final : public ServerPacket
        {
        public:
            explicit PerksProgramResultVendorList() : ServerPacket(SMSG_PERKS_PROGRAM_RESULT, 1872) { }

            WorldPacket const* Write() override;

            ObjectGuid VendorGuid;
            ObjectGuid PlayerGuid;
            PerksVendorItem FrozenItem; 
            std::vector<PerksVendorItem> Items;
        };

        class PerksProgramResultPurchase final : public ServerPacket
        {
        public:
            explicit PerksProgramResultPurchase() : ServerPacket(SMSG_PERKS_PROGRAM_RESULT, 48) { }

            WorldPacket const* Write() override;

            int32 PurchasedVendorItemID = 0;
            std::vector<PerksShortItem> PurchasedItems;
        };

        class ResponsePerkPendingRewards final : public ServerPacket
        {
        public:
            explicit ResponsePerkPendingRewards() : ServerPacket(SMSG_RESPONSE_PERK_PENDING_REWARDS, 4) { }

            WorldPacket const* Write() override;

            uint32 PendingRewardCount = 0;
        };

        class ResponsePerkRecentPurchases final : public ServerPacket
        {
        public:
            explicit ResponsePerkRecentPurchases() : ServerPacket(SMSG_RESPONSE_PERK_RECENT_PURCHASES, 4) { }

            WorldPacket const* Write() override;

            std::vector<PerksShortItem> RecentPurchases;
        };
    }
}

#endif // TRINITYCORE_PERKS_PROGRAM_PACKETS_H