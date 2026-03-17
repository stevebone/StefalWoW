/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
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

#include "VendorPurchaseManager.h"
#include "Bag.h"
#include "Creature.h"
#include "CreatureData.h"
#include "Equipment/EquipmentManager.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Unit.h"
#include "UnitAI.h"
#include "AI/BotAI.h"
#include "Core/Managers/GameSystemsManager.h"
#include <algorithm>
#include <cmath>

namespace Playerbot
{
    // ============================================================================
    // VendorPurchaseManager Implementation
    // ============================================================================

    VendorPurchaseResult VendorPurchaseManager::PurchaseItem(
        Player* player,
        VendorPurchaseRequest const& request)
    {
        // Validate player
    if (!player)
        {
            TC_LOG_ERROR("playerbot.vendor", "VendorPurchaseManager: Invalid player (nullptr)");
            return VendorPurchaseResult::PURCHASE_FAILED;
        }

        // Find vendor creature
        Creature* vendor = ObjectAccessor::GetCreature(*player, request.vendorGuid);
        if (!vendor)
        {
            TC_LOG_ERROR("playerbot.vendor",
                "VendorPurchaseManager: Vendor {} not found for player {}",
                request.vendorGuid.ToString(), player->GetName());
            return VendorPurchaseResult::VENDOR_NOT_FOUND;
        }

        // Check if creature is a vendor
    if (!vendor->IsVendor())
        {
            TC_LOG_ERROR("playerbot.vendor",
                "VendorPurchaseManager: Creature {} ({}) is not a vendor",
                vendor->GetName(), vendor->GetEntry());
            return VendorPurchaseResult::NOT_A_VENDOR;
        }

        // Find item in vendor inventory
        ::std::optional<uint32> vendorSlotOpt = FindVendorSlot(vendor, request.itemId);
        if (!vendorSlotOpt.has_value())
        {
            TC_LOG_WARN("playerbot.vendor",
                "VendorPurchaseManager: Item {} not found in vendor {} inventory",
                request.itemId, vendor->GetEntry());
            return VendorPurchaseResult::ITEM_NOT_FOUND;
        }

        uint32 vendorSlot = *vendorSlotOpt;

        // Validate purchase
        VendorPurchaseResult validationResult = ValidatePurchase(
            player, vendor, vendorSlot, request.itemId, request.quantity);

        if (validationResult != VendorPurchaseResult::SUCCESS)
            return validationResult;

        // Execute purchase using TrinityCore API
        bool success = player->BuyItemFromVendorSlot(
            request.vendorGuid,         // Vendor GUID
            vendorSlot,                 // Vendor slot index
            request.itemId,             // Item ID
            request.quantity,           // Quantity
            NULL_BAG,                   // Auto-select bag
            NULL_SLOT                   // Auto-select slot
        );

        if (success)
        {
            TC_LOG_DEBUG("playerbot.vendor",
                "VendorPurchaseManager: Player {} successfully purchased {}x item {} from vendor {}",
                player->GetName(), request.quantity, request.itemId, vendor->GetEntry());
            return VendorPurchaseResult::SUCCESS;
        }
        else
        {
            TC_LOG_ERROR("playerbot.vendor",
                "VendorPurchaseManager: Purchase failed for player {} - item {} from vendor {}",
                player->GetName(), request.itemId, vendor->GetEntry());
            return VendorPurchaseResult::PURCHASE_FAILED;
        }
    }

    ::std::vector<VendorPurchaseRecommendation> VendorPurchaseManager::GetPurchaseRecommendations(
        Player const* player,
        Creature const* vendor,
        uint64 goldBudget) const
    {
        ::std::vector<VendorPurchaseRecommendation> recommendations;

        // Validate inputs
    if (!player || !vendor)
            return recommendations;

        // Get vendor inventory
        VendorItemData const* vendorItems = vendor->GetVendorItems();
        if (!vendorItems || vendorItems->Empty())
        {
            TC_LOG_DEBUG("playerbot.vendor",
                "VendorPurchaseManager: Vendor {} has no items",
                vendor->GetEntry());
            return recommendations;
        }

        uint32 itemCount = vendorItems->GetItemCount();
        recommendations.reserve(itemCount);

        uint64 remainingBudget = goldBudget;

        // Analyze each vendor item
    for (uint32 slot = 0; slot < itemCount; ++slot)
        {
            VendorItem const* vendorItem = vendorItems->GetItem(slot);
            if (!vendorItem)
                continue;

            // Get item template
            ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(vendorItem->item);
            if (!itemTemplate)
                continue;

            // Calculate item priority
            ItemPurchasePriority priority = CalculateItemPriority(player, itemTemplate);
            if (priority == ItemPurchasePriority::NONE)
                continue; // Skip items with no priority

            // Calculate purchase cost
            uint64 goldCost = CalculatePurchaseCost(player, vendor, vendorItem, itemTemplate, 1);

            // Check budget
    if (goldCost > remainingBudget)
                continue; // Can't afford

            // Check for gear upgrades
            bool isUpgrade = false;
            float upgradeScore = 0.0f;
            if (itemTemplate->GetInventoryType() != INVTYPE_NON_EQUIP)
            {
                isUpgrade = IsItemUpgrade(player, itemTemplate, upgradeScore);
            }

            // Determine recommended quantity
            uint32 suggestedQuantity = 1;
            if (itemTemplate->GetClass() == ITEM_CLASS_CONSUMABLE)
            {
                suggestedQuantity = GetRecommendedConsumableQuantity(player, itemTemplate);
                // Adjust for budget
                uint64 totalCost = goldCost * suggestedQuantity;
                if (totalCost > remainingBudget)
                {
                    suggestedQuantity = static_cast<uint32>(remainingBudget / goldCost);
                    if (suggestedQuantity == 0)
                        continue; // Can't afford even 1
                }
            }

            // Create recommendation
            VendorPurchaseRecommendation rec;
            rec.itemId = vendorItem->item;
            rec.vendorSlot = slot;
            rec.suggestedQuantity = suggestedQuantity;
            rec.priority = priority;
            rec.goldCost = goldCost * suggestedQuantity;
            rec.isUpgrade = isUpgrade;
            rec.upgradeScore = upgradeScore;

            // Generate purchase reason
    if (isUpgrade)
            {
                rec.reason = "Gear upgrade (score: " + ::std::to_string(static_cast<int>(upgradeScore)) + ")";
            }
            else if (itemTemplate->GetClass() == ITEM_CLASS_CONSUMABLE)
            {
                rec.reason = "Consumable restock";
            }
            else
            {
                rec.reason = "Useful item";
            }

            recommendations.push_back(rec);

            // Deduct from remaining budget
            remainingBudget -= rec.goldCost;
        }

        // Sort by priority (CRITICAL first, then HIGH, etc.)
        ::std::sort(recommendations.begin(), recommendations.end(),
            [](auto const& a, auto const& b)
            {
                if (a.priority != b.priority)
                    return a.priority < b.priority; // Lower enum value = higher priority

                // Within same priority, sort by upgrade score
    if (a.isUpgrade && b.isUpgrade)
                    return a.upgradeScore > b.upgradeScore;

                // Upgrades before non-upgrades
    if (a.isUpgrade != b.isUpgrade)
                    return a.isUpgrade;

                // Finally, sort by item ID for consistency
                return a.itemId < b.itemId;
            });

        TC_LOG_DEBUG("playerbot.vendor",
            "VendorPurchaseManager: Generated {} purchase recommendations for player {} from vendor {} (budget: {}copper)",
            recommendations.size(), player->GetName(), vendor->GetEntry(), goldBudget);

        return recommendations;
    }

    ::std::optional<uint32> VendorPurchaseManager::FindVendorSlot(
        Creature const* vendor,
        uint32 itemId)
    {
        if (!vendor)
            return ::std::nullopt;

        VendorItemData const* vendorItems = vendor->GetVendorItems();
        if (!vendorItems || vendorItems->Empty())
            return ::std::nullopt;

        // Linear search through vendor inventory
        uint32 itemCount = vendorItems->GetItemCount();
        for (uint32 slot = 0; slot < itemCount; ++slot)
        {
            VendorItem const* item = vendorItems->GetItem(slot);
            if (item && item->item == itemId)
                return slot;
        }

        return ::std::nullopt;
    }

    bool VendorPurchaseManager::IsItemUpgrade(
        Player const* player,
        ItemTemplate const* itemTemplate,
        float& upgradeScore)
    {
        upgradeScore = 0.0f;

        if (!player || !itemTemplate)
            return false;

        // Only evaluate equippable items
    if (itemTemplate->GetInventoryType() == INVTYPE_NON_EQUIP)
            return false;

        // Check if player can use item (class restriction)
    if (!(itemTemplate->GetAllowableClass() & player->GetClassMask()))
            return false;

        // Get EquipmentManager from bot's GameSystemsManager (Phase 6.1)
        Player* mutablePlayer = const_cast<Player*>(player);
        BotAI* botAI = dynamic_cast<BotAI*>(mutablePlayer->GetAI());

        // Only bots have EquipmentManager - non-bots can't use this feature
        if (!botAI)
        {
            TC_LOG_DEBUG("playerbot.vendor", "VendorPurchaseManager: Player is not a bot, cannot evaluate gear");
            return false;
        }

        EquipmentManager* equipMgr = botAI->GetGameSystems()->GetEquipmentManager();
        if (!equipMgr)
        {
            TC_LOG_ERROR("playerbot.vendor", "VendorPurchaseManager: EquipmentManager not available");
            return false;
        }

        // Get equipment slot for this item
        uint8 equipSlot = equipMgr->GetItemEquipmentSlot(itemTemplate);
        if (equipSlot == NULL_SLOT)
        {
            TC_LOG_DEBUG("playerbot.vendor", "VendorPurchaseManager: Item {} has no valid equipment slot", itemTemplate->GetId());
            return false;
        }

        // Get currently equipped item in that slot
        Item* currentItem = mutablePlayer->GetItemByPos(INVENTORY_SLOT_BAG_0, equipSlot);

        // Calculate score for vendor item using class/spec stat priorities
        float vendorItemScore = equipMgr->CalculateItemTemplateScore(itemTemplate);

        // If no item equipped, vendor item is an upgrade if it has positive score
    if (!currentItem)
        {
            if (vendorItemScore > 0.0f)
            {
                upgradeScore = vendorItemScore;
                TC_LOG_DEBUG("playerbot.vendor", "VendorPurchaseManager: Item {} fills empty slot {} (score: {:.1f})",
                    itemTemplate->GetId(), equipSlot, upgradeScore);
                return true;
            }
            return false;
        }

        // Calculate score for currently equipped item
        float currentItemScore = equipMgr->CalculateItemScore(currentItem);

        // Compare scores
        float scoreDifference = vendorItemScore - currentItemScore;
        upgradeScore = scoreDifference;

        // Item is upgrade if vendor item scores higher
        bool isUpgrade = scoreDifference > 0.0f;

        if (isUpgrade)
        {
            TC_LOG_DEBUG("playerbot.vendor", "VendorPurchaseManager: Item {} is upgrade over {} (score: {:.1f} vs {:.1f}, diff: +{:.1f})",
                itemTemplate->GetId(), currentItem->GetEntry(), vendorItemScore, currentItemScore, scoreDifference);
        }

        return isUpgrade;
    }

    ItemPurchasePriority VendorPurchaseManager::CalculateItemPriority(
        Player const* player,
        ItemTemplate const* itemTemplate)
    {
        if (!player || !itemTemplate)
            return ItemPurchasePriority::NONE;

        // Check class restrictions
    if (!(itemTemplate->GetAllowableClass() & player->GetClassMask()))
            return ItemPurchasePriority::NONE;

        // Check level requirements
    if (itemTemplate->GetBaseRequiredLevel() > player->GetLevel())
            return ItemPurchasePriority::NONE;

        uint32 itemClass = itemTemplate->GetClass();
        uint32 itemSubClass = itemTemplate->GetSubClass();

        // CRITICAL: Food and water
    if (itemClass == ITEM_CLASS_CONSUMABLE)
        {
            if (itemSubClass == ITEM_SUBCLASS_FOOD_DRINK|| itemSubClass == ITEM_SUBCLASS_CONSUMABLE) // Water is CONSUMABLE subclass
            {
                // Check current food/water stock
                uint32 currentCount = player->GetItemCount(itemTemplate->GetId());
                if (currentCount < 20) // Low stock threshold
                    return ItemPurchasePriority::CRITICAL;
                else
                    return ItemPurchasePriority::HIGH;
            }

            // Other consumables (potions, elixirs, etc.)
            return ItemPurchasePriority::HIGH;
        }

        // CRITICAL/HIGH: Reagents for casters
    if (itemClass == ITEM_CLASS_REAGENT)
        {
            uint32 currentCount = player->GetItemCount(itemTemplate->GetId());
            if (currentCount < 20)
                return ItemPurchasePriority::CRITICAL;
            else
                return ItemPurchasePriority::HIGH;
        }

        // CRITICAL: Ammo for hunters (if applicable)
    if (itemClass == ITEM_CLASS_PROJECTILE && player->GetClass() == CLASS_HUNTER)
        {
            uint32 currentCount = player->GetItemCount(itemTemplate->GetId());
            if (currentCount < 200)
                return ItemPurchasePriority::CRITICAL;
            else
                return ItemPurchasePriority::HIGH;
        }

        // HIGH: Gear upgrades
    if (itemTemplate->GetInventoryType() != INVTYPE_NON_EQUIP)
        {
            float upgradeScore = 0.0f;
            if (IsItemUpgrade(player, itemTemplate, upgradeScore))
                return ItemPurchasePriority::HIGH;
        }

        // MEDIUM: Trade goods, recipes
    if (itemClass == ITEM_CLASS_TRADE_GOODS || itemClass == ITEM_CLASS_RECIPE)
            return ItemPurchasePriority::MEDIUM;

        // LOW: Vanity items, pets, mounts
    if (itemClass == ITEM_CLASS_MISCELLANEOUS)
        {
            // Companion pets
    if (itemSubClass == ITEM_SUBCLASS_MISCELLANEOUS_COMPANION_PET)
                return ItemPurchasePriority::LOW;

            // Mounts
    if (itemSubClass == ITEM_SUBCLASS_MISCELLANEOUS_MOUNT)
                return ItemPurchasePriority::LOW;
        }

        // Default: No priority (skip)
        return ItemPurchasePriority::NONE;
    }

    char const* VendorPurchaseManager::GetResultString(VendorPurchaseResult result)
    {
        switch (result)
        {
            case VendorPurchaseResult::SUCCESS:
                return "SUCCESS";
            case VendorPurchaseResult::VENDOR_NOT_FOUND:
                return "VENDOR_NOT_FOUND";
            case VendorPurchaseResult::NOT_A_VENDOR:
                return "NOT_A_VENDOR";
            case VendorPurchaseResult::OUT_OF_RANGE:
                return "OUT_OF_RANGE";
            case VendorPurchaseResult::ITEM_NOT_FOUND:
                return "ITEM_NOT_FOUND";
            case VendorPurchaseResult::INSUFFICIENT_GOLD:
                return "INSUFFICIENT_GOLD";
            case VendorPurchaseResult::INSUFFICIENT_CURRENCY:
                return "INSUFFICIENT_CURRENCY";
            case VendorPurchaseResult::INVENTORY_FULL:
                return "INVENTORY_FULL";
            case VendorPurchaseResult::ITEM_SOLD_OUT:
                return "ITEM_SOLD_OUT";
            case VendorPurchaseResult::REPUTATION_TOO_LOW:
                return "REPUTATION_TOO_LOW";
            case VendorPurchaseResult::LEVEL_TOO_LOW:
                return "LEVEL_TOO_LOW";
            case VendorPurchaseResult::CLASS_RESTRICTION:
                return "CLASS_RESTRICTION";
            case VendorPurchaseResult::FACTION_RESTRICTION:
                return "FACTION_RESTRICTION";
            case VendorPurchaseResult::ACHIEVEMENT_REQUIRED:
                return "ACHIEVEMENT_REQUIRED";
            case VendorPurchaseResult::CONDITION_NOT_MET:
                return "CONDITION_NOT_MET";
            case VendorPurchaseResult::PURCHASE_FAILED:
                return "PURCHASE_FAILED";
            default:
                return "UNKNOWN";
        }
    }

    // ============================================================================
    // Private Helper Methods
    // ============================================================================

    VendorPurchaseResult VendorPurchaseManager::ValidatePurchase(
        Player const* player,
        Creature const* vendor,
        uint32 vendorSlot,
        uint32 itemId,
        uint32 quantity)
    {
        // Validate player
    if (!player)
            return VendorPurchaseResult::PURCHASE_FAILED;

        // Validate vendor
    if (!vendor)
            return VendorPurchaseResult::VENDOR_NOT_FOUND;

        // Check interaction range (most vendor checks happen in BuyItemFromVendorSlot,
        // but we can pre-validate range for better error messages)
    if (!player->IsWithinDistInMap(vendor, 10.0f)) // Typical interaction range
            return VendorPurchaseResult::OUT_OF_RANGE;

        // Get vendor items
        VendorItemData const* vendorItems = vendor->GetVendorItems();
        if (!vendorItems || vendorItems->Empty())
            return VendorPurchaseResult::ITEM_NOT_FOUND;

        // Validate vendor slot
    if (vendorSlot >= vendorItems->GetItemCount())
            return VendorPurchaseResult::ITEM_NOT_FOUND;

        // Get vendor item
        VendorItem const* vendorItem = vendorItems->GetItem(vendorSlot);
        if (!vendorItem || vendorItem->item != itemId)
            return VendorPurchaseResult::ITEM_NOT_FOUND;

        // Get item template
        ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
        if (!itemTemplate)
            return VendorPurchaseResult::ITEM_NOT_FOUND;

        // Check level requirement
    if (itemTemplate->GetBaseRequiredLevel() > player->GetLevel())
            return VendorPurchaseResult::LEVEL_TOO_LOW;

        // Check class restriction
    if (!(itemTemplate->GetAllowableClass() & player->GetClassMask()))
            return VendorPurchaseResult::CLASS_RESTRICTION;

        // Check limited stock
    if (vendorItem->maxcount != 0)
        {
            // Note: GetVendorItemCurrentCount is not const in TrinityCore 12.0
            Creature* mutableVendor = const_cast<Creature*>(vendor);
            if (mutableVendor->GetVendorItemCurrentCount(vendorItem) < quantity)
                return VendorPurchaseResult::ITEM_SOLD_OUT;
        }

        // Calculate cost
        uint64 totalCost = CalculatePurchaseCost(player, vendor, vendorItem, itemTemplate, quantity);

        // Check if player has enough gold
    if (totalCost > player->GetMoney())
            return VendorPurchaseResult::INSUFFICIENT_GOLD;

        // Check inventory space
    if (!HasInventorySpace(player, itemTemplate, quantity))
            return VendorPurchaseResult::INVENTORY_FULL;

        return VendorPurchaseResult::SUCCESS;
    }

    uint64 VendorPurchaseManager::CalculatePurchaseCost(
        Player const* player,
        Creature const* vendor,
        VendorItem const* vendorItem,
        ItemTemplate const* itemTemplate,
        uint32 quantity)
    {
        if (!player || !vendor || !vendorItem || !itemTemplate)
            return 0;

        // Base price per item
        uint64 basePrice = itemTemplate->GetBuyPrice();
        if (basePrice == 0)
            return 0; // Free item or extended cost only

        // Calculate price for stack
        uint32 buyCount = itemTemplate->GetBuyCount();
        if (buyCount == 0)
            buyCount = 1;

        double pricePerItem = static_cast<double>(basePrice) / buyCount;
        uint64 totalPrice = static_cast<uint64>(pricePerItem * quantity);

        // Apply reputation discount
        float discount = player->GetReputationPriceDiscount(vendor);
        totalPrice = static_cast<uint64>(::std::floor(totalPrice * discount));

        // Ensure minimum price of 1 copper if item has base price
    if (basePrice > 0 && totalPrice == 0)
            totalPrice = 1;

        return totalPrice;
    }

    bool VendorPurchaseManager::HasInventorySpace(
        Player const* player,
        ItemTemplate const* itemTemplate,
        uint32 quantity)
    {
        if (!player || !itemTemplate)
            return false;

        // Simple check: Does player have at least one free bag slot?
        // More sophisticated check would account for stackable items and partial stacks
        // For production, use Player::CanStoreNewItem() or similar

        // Count free slots in all bags
        uint32 freeSlots = 0;

        // Check backpack
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
        {
            if (!player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                ++freeSlots;
        }

        // Check additional bags
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
        {
            if (Bag const* bagItem = player->GetBagByPos(bag))
            {
                freeSlots += bagItem->GetFreeSlots();
            }
        }

        // Estimate required slots
        uint32 maxStack = itemTemplate->GetMaxStackSize();
        if (maxStack == 0)
            maxStack = 1;

        uint32 requiredSlots = (quantity + maxStack - 1) / maxStack; // Ceiling division

        return freeSlots >= requiredSlots;
    }

    uint32 VendorPurchaseManager::GetRecommendedConsumableQuantity(
        Player const* player,
        ItemTemplate const* itemTemplate)
    {
        if (!player || !itemTemplate)
            return 1;

        // Get current count
        uint32 currentCount = player->GetItemCount(itemTemplate->GetId());

        // Target stock levels by consumable type
        uint32 targetStock = 20; // Default

        uint32 itemClass = itemTemplate->GetClass();
        uint32 itemSubClass = itemTemplate->GetSubClass();

        if (itemClass == ITEM_CLASS_CONSUMABLE)
        {
            if (itemSubClass == ITEM_SUBCLASS_FOOD_DRINK|| itemSubClass == ITEM_SUBCLASS_CONSUMABLE)
            {
                // Food/water: Stock up to 40
                targetStock = 40;
            }
            else
            {
                // Potions, elixirs: Stock up to 20
                targetStock = 20;
            }
        }
        else if (itemClass == ITEM_CLASS_REAGENT)
        {
            // Reagents: Stock up to 60
            targetStock = 60;
        }
        else if (itemClass == ITEM_CLASS_PROJECTILE)
        {
            // Ammo: Stock up to 1000
            targetStock = 1000;
        }

        // Calculate how many to buy
    if (currentCount >= targetStock)
            return 0; // Already stocked

        uint32 quantityNeeded = targetStock - currentCount;

        // Cap at max stack size for single purchase
        uint32 maxStack = itemTemplate->GetMaxStackSize();
        if (maxStack > 0 && quantityNeeded > maxStack * 2) // Buy max 2 stacks at once
            quantityNeeded = maxStack * 2;

        return quantityNeeded;
    }

} // namespace Playerbot
