/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "VendorInteractionManager.h"
#include "Player.h"
#include "Creature.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "ObjectMgr.h"
#include "CreatureData.h"
#include "Log.h"
#include "Bag.h"
#include <chrono>
#include <algorithm>

namespace Playerbot
{

// Constants for budget allocation percentages
static constexpr float BUDGET_CRITICAL_PERCENT = 0.50f;  // 50% for critical items
static constexpr float BUDGET_HIGH_PERCENT     = 0.30f;  // 30% for high priority
static constexpr float BUDGET_MEDIUM_PERCENT   = 0.15f;  // 15% for medium priority
static constexpr float BUDGET_LOW_PERCENT      = 0.05f;  // 5% for low priority

// Repair cost reservation (as percentage of total gold)
static constexpr float REPAIR_RESERVE_PERCENT = 0.20f;   // Reserve 20% for repairs

// Consumable stack sizes
static constexpr uint32 FOOD_STACK_SIZE = 20;
static constexpr uint32 WATER_STACK_SIZE = 20;
static constexpr uint32 AMMO_STACK_SIZE = 200;
static constexpr uint32 REAGENT_STACK_SIZE = 20;

VendorInteractionManager::VendorInteractionManager(Player* bot)
    : m_bot(bot)
    , m_stats()
    , m_cpuUsage(0.0f)
    , m_totalPurchaseTime(0)
    , m_purchaseCount(0)
{
}

// ============================================================================
// Core Purchase Methods
// ============================================================================

bool VendorInteractionManager::PurchaseItem(Creature* vendor, uint32 itemId, uint32 quantity)
{
    if (!m_bot || !vendor || !vendor->IsVendor())
        return false;

    auto startTime = ::std::chrono::high_resolution_clock::now();

    m_stats.purchaseAttempts++;
    // Find item in vendor's inventory
    VendorItem const* vendorItem = FindVendorItem(vendor, itemId);
    if (!vendorItem)
    {
        TC_LOG_DEBUG("bot.playerbot", "Bot %s: Item %u not found in vendor %u inventory",

            m_bot->GetName().c_str(), itemId, vendor->GetEntry());
        m_stats.purchaseFailures++;
        return false;
    }

    ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
    if (!itemTemplate)
    {
        m_stats.purchaseFailures++;
        return false;
    }

    // Calculate total cost - slot is the index in vendor's item list
    // We need to find the vendor slot index for this item
    uint32 vendorSlot = 0;
    VendorItemData const* vendorItems = vendor->GetVendorItems();
    if (vendorItems)
    {
        for (size_t i = 0; i < vendorItems->m_items.size(); ++i)
        {

            if (vendorItems->m_items[i].item == itemId)

            {

                vendorSlot = static_cast<uint32>(i);

                break;

            }
        }
    }

    uint64 goldCost = GetVendorPrice(vendor, itemId, vendorSlot, quantity);

    // Check if bot can afford it
    if (!CanAfford(goldCost, vendorItem->ExtendedCost))
    {
        TC_LOG_DEBUG("bot.playerbot", "Bot %s: Cannot afford item %u (cost: %llu, available: %llu)",

            m_bot->GetName().c_str(), itemId, goldCost, m_bot->GetMoney());
        m_stats.insufficientGold++;
        RecordPurchase(itemId, goldCost, false);
        return false;
    }

    // Check bag space
    if (!HasBagSpace(itemId, quantity))
    {
        TC_LOG_DEBUG("bot.playerbot", "Bot %s: No bag space for item %u",

            m_bot->GetName().c_str(), itemId);
        m_stats.noBagSpace++;
        RecordPurchase(itemId, goldCost, false);
        return false;
    }

    // Execute purchase via TrinityCore API (vendorSlot was calculated above)
    bool success = ExecutePurchase(vendor, vendorSlot, itemId, quantity, 255, 0);

    if (success)
    {
        TC_LOG_DEBUG("bot.playerbot", "Bot %s: Successfully purchased %u x %u for %llu copper",

            m_bot->GetName().c_str(), quantity, itemId, goldCost);
    }

    RecordPurchase(itemId, goldCost, success);

    // Track performance
    auto endTime = ::std::chrono::high_resolution_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime);
    m_totalPurchaseTime += duration.count();
    m_purchaseCount++;
    m_cpuUsage = m_purchaseCount > 0 ? (float)m_totalPurchaseTime / m_purchaseCount / 1000.0f : 0.0f;

    return success;
}

uint32 VendorInteractionManager::PurchaseItems(Creature* vendor, ::std::vector<uint32> const& itemIds)
{
    if (!m_bot || !vendor || itemIds.empty())
        return 0;

    // Calculate budget
    BudgetAllocation budget = CalculateBudget();

    // Evaluate all items
    ::std::vector<VendorItemEvaluation> evaluations;
    evaluations.reserve(itemIds.size());

    for (uint32 itemId : itemIds)
    {
        VendorItem const* vendorItem = FindVendorItem(vendor, itemId);
        if (!vendorItem)

            continue;

        ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
        if (!itemTemplate)

            continue;

        // Calculate vendor slot index (slot is the index in the vendor's item list)
        uint32 vendorSlot = 0;
        VendorItemData const* vendorItems = vendor->GetVendorItems();
        if (vendorItems)
        {

            for (size_t i = 0; i < vendorItems->m_items.size(); ++i)

            {

                if (vendorItems->m_items[i].item == itemId)

                {

                    vendorSlot = static_cast<uint32>(i);

                    break;

                }

            }
        }

        VendorItemEvaluation eval = EvaluateVendorItem(vendor, itemTemplate, vendorSlot);
        if (eval.shouldPurchase)

            evaluations.push_back(eval);
    }

    // Sort by priority (highest priority first)
    ::std::sort(evaluations.begin(), evaluations.end(),
        [](VendorItemEvaluation const& a, VendorItemEvaluation const& b)
        {

            return static_cast<uint8>(a.priority) < static_cast<uint8>(b.priority);
        });

    // Purchase items in priority order within budget
    uint32 purchasedCount = 0;

    for (VendorItemEvaluation const& eval : evaluations)
    {
        if (!FitsWithinBudget(eval.goldCost, eval.priority, budget))
        {

            TC_LOG_DEBUG("bot.playerbot", "Bot %s: Item %u doesn't fit budget (priority: %u, cost: %llu)",

                m_bot->GetName().c_str(), eval.itemId, static_cast<uint8>(eval.priority), eval.goldCost);

            continue;
        }

        if (PurchaseItem(vendor, eval.itemId, eval.recommendedQuantity))
        {

            purchasedCount++;

            // Deduct from appropriate budget category
    switch (eval.priority)

            {

                case PurchasePriority::CRITICAL:

                    budget.criticalBudget -= eval.goldCost;

                    break;

                case PurchasePriority::HIGH:

                    budget.highBudget -= eval.goldCost;

                    break;

                case PurchasePriority::MEDIUM:

                    budget.mediumBudget -= eval.goldCost;

                    break;

                case PurchasePriority::LOW:

                    budget.lowBudget -= eval.goldCost;

                    break;

            }
        }
    }

    return purchasedCount;
}

uint32 VendorInteractionManager::SmartPurchase(Creature* vendor)
{
    if (!m_bot || !vendor || !vendor->IsVendor())
        return 0;

    ::std::vector<uint32> itemsToPurchase;

    // Step 1: Add required reagents (CRITICAL priority)
    ::std::vector<uint32> reagents = GetRequiredReagents();
    itemsToPurchase.insert(itemsToPurchase.end(), reagents.begin(), reagents.end());

    // Step 2: Add consumables (HIGH priority)
    ::std::vector<uint32> consumables = GetRequiredConsumables();
    itemsToPurchase.insert(itemsToPurchase.end(), consumables.begin(), consumables.end());

    // Step 3: Add ammunition if hunter (HIGH priority)
    if (NeedsAmmunition())
    {
        uint32 ammo = GetAppropriateAmmunition();
        if (ammo != 0)

            itemsToPurchase.push_back(ammo);
    }

    // Step 4: Scan vendor for useful items (MEDIUM/LOW priority)
    ::std::vector<VendorItem const*> vendorItems = GetVendorItems(vendor);
    for (VendorItem const* vendorItem : vendorItems)
    {
        if (!vendorItem)

            continue;

        ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(vendorItem->item);
        if (!itemTemplate)

            continue;

        // Skip items already in our purchase list
    if (::std::find(itemsToPurchase.begin(), itemsToPurchase.end(), vendorItem->item) != itemsToPurchase.end())

            continue;

        // Check if it's a useful equipment upgrade
    if (IsEquipmentUpgrade(itemTemplate))

            itemsToPurchase.push_back(vendorItem->item);
    }

    // Execute purchases
    return PurchaseItems(vendor, itemsToPurchase);
}

// ============================================================================
// Vendor Analysis Methods
// ============================================================================

::std::vector<VendorItem const*> VendorInteractionManager::GetVendorItems(Creature* vendor) const
{
    ::std::vector<VendorItem const*> items;

    if (!vendor || !vendor->IsVendor())
        return items;

    // Use TrinityCore API to get vendor items
    VendorItemData const* vendorItems = vendor->GetVendorItems();
    if (!vendorItems)
        return items;

    for (VendorItem const& vendorItem : vendorItems->m_items)
    {
        items.push_back(&vendorItem);
    }

    return items;
}

VendorInteractionManager::VendorItemEvaluation VendorInteractionManager::EvaluateVendorItem(
    Creature* vendor, ItemTemplate const* item, uint32 vendorSlot) const
{
    VendorItemEvaluation eval;
    eval.itemId = item->GetId();
    eval.vendorSlot = vendorSlot;
    eval.priority = CalculateItemPriority(item);
    eval.goldCost = GetVendorPrice(vendor, item->GetId(), vendorSlot, 1);
    eval.recommendedQuantity = GetRecommendedQuantity(item);
    eval.goldCost *= eval.recommendedQuantity;

    // Determine if we should purchase this item
    eval.shouldPurchase = false;
    eval.reason = "Not needed";

    // Critical items: Always purchase if we don't have enough
    if (eval.priority == PurchasePriority::CRITICAL)
    {
        if (IsClassReagent(item))
        {

            uint32 currentCount = m_bot->GetItemCount(item->GetId());

            if (currentCount < REAGENT_STACK_SIZE)

            {

                eval.shouldPurchase = true;

                eval.reason = "Critical class reagent";

                eval.recommendedQuantity = REAGENT_STACK_SIZE - currentCount;

                eval.goldCost = GetVendorPrice(vendor, item->GetId(), vendorSlot, eval.recommendedQuantity);

            }
        }
    }
    // High priority items: Purchase if we're running low
    else if (eval.priority == PurchasePriority::HIGH)
    {
        if (IsConsumable(item))
        {

            uint32 currentCount = m_bot->GetItemCount(item->GetId());

            uint32 targetCount = (item->GetClass() == ITEM_CLASS_CONSUMABLE) ? FOOD_STACK_SIZE : WATER_STACK_SIZE;


            if (currentCount < targetCount / 2) // Restock when below 50%

            {

                eval.shouldPurchase = true;

                eval.reason = "Consumable restock needed";

                eval.recommendedQuantity = targetCount - currentCount;

                eval.goldCost = GetVendorPrice(vendor, item->GetId(), vendorSlot, eval.recommendedQuantity);

            }
        }
    }
    // Medium priority: Equipment upgrades
    else if (eval.priority == PurchasePriority::MEDIUM)
    {
        if (IsEquipmentUpgrade(item))
        {

            eval.shouldPurchase = true;

            eval.reason = "Equipment upgrade available";

            eval.recommendedQuantity = 1;
        }
    }

    return eval;
}

VendorInteractionManager::PurchasePriority VendorInteractionManager::CalculateItemPriority(ItemTemplate const* item) const
{
    if (!item)
        return PurchasePriority::LOW;

    // Check cache first
    auto it = m_priorityCache.find(item->GetId());
    if (it != m_priorityCache.end())
        return it->second;
    PurchasePriority priority = PurchasePriority::LOW;

    // CRITICAL: Class-specific reagents
    if (IsClassReagent(item))
    {
        priority = PurchasePriority::CRITICAL;
    }
    // HIGH: Food, water, ammunition
    else if (IsConsumable(item))
    {
        priority = PurchasePriority::HIGH;
    }
    // HIGH: Ammunition (hunters)
    else if (item->GetClass() == ITEM_CLASS_PROJECTILE)
    {
        priority = PurchasePriority::HIGH;
    }
    // MEDIUM: Equipment upgrades
    else if (item->GetClass() == ITEM_CLASS_WEAPON || item->GetClass() == ITEM_CLASS_ARMOR)
    {
        priority = PurchasePriority::MEDIUM;
    }
    // LOW: Everything else
    else
    {
        priority = PurchasePriority::LOW;
    }

    // Cache the result
    const_cast<VendorInteractionManager*>(this)->m_priorityCache[item->GetId()] = priority;

    return priority;
}

bool VendorInteractionManager::CanAfford(uint64 goldCost, uint32 extendedCostId) const
{
    if (!m_bot)
        return false;

    // Check gold cost
    if (m_bot->GetMoney() < goldCost)
        return false;

    // LIMITATION: Extended cost vendors (honor, marks, tokens) not yet supported.
    // Only gold-cost purchases are handled. Extended costs require checking
    // player's currency/token inventory, which involves additional server queries.
    if (extendedCostId != 0)
    {
        TC_LOG_DEBUG("bot.playerbot", "Bot %s: Extended cost %u not yet supported",

            m_bot->GetName().c_str(), extendedCostId);
        return false;
    }

    return true;
}

uint64 VendorInteractionManager::GetVendorPrice(Creature* vendor, uint32 itemId, uint32 vendorSlot, uint32 quantity) const
{
    if (!vendor || quantity == 0)
        return 0;

    ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
    if (!itemTemplate)
        return 0;

    // Get base buy price from item template
    uint64 basePrice = itemTemplate->GetBuyPrice();

    // Apply reputation discount if applicable
    // TrinityCore handles this automatically in BuyItemFromVendorSlot

    return basePrice * quantity;
}

// ============================================================================
// Budget Management Methods
// ============================================================================

VendorInteractionManager::BudgetAllocation VendorInteractionManager::CalculateBudget() const
{
    BudgetAllocation budget;

    if (!m_bot)
        return budget;

    budget.totalAvailable = m_bot->GetMoney();
    // Reserve gold for repairs
    budget.reservedForRepairs = CalculateRepairCostEstimate();
    if (budget.reservedForRepairs > budget.totalAvailable)
        budget.reservedForRepairs = budget.totalAvailable;

    uint64 spendable = budget.totalAvailable - budget.reservedForRepairs;

    // Allocate remaining gold by priority
    budget.criticalBudget = static_cast<uint64>(spendable * BUDGET_CRITICAL_PERCENT);
    budget.highBudget     = static_cast<uint64>(spendable * BUDGET_HIGH_PERCENT);
    budget.mediumBudget   = static_cast<uint64>(spendable * BUDGET_MEDIUM_PERCENT);
    budget.lowBudget      = static_cast<uint64>(spendable * BUDGET_LOW_PERCENT);

    return budget;
}

uint64 VendorInteractionManager::CalculateRepairCostEstimate() const
{
    if (!m_bot)
        return 0;

    uint64 totalCost = 0;

    // Check all equipped items for durability
    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        Item* item = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!item)

            continue;

        uint32 maxDurability = *item->m_itemData->MaxDurability;
        uint32 durability = *item->m_itemData->Durability;

        if (maxDurability > 0 && durability < maxDurability)
        {
            // Simplified repair cost calculation

            uint32 itemLevel = item->GetTemplate()->GetBaseItemLevel();

            uint32 damagePercent = ((maxDurability - durability) * 100) / maxDurability;

            totalCost += (itemLevel * damagePercent) / 10;
        }
    }

    // Add 20% buffer for safety
    return static_cast<uint64>(totalCost * 1.2f);
}

bool VendorInteractionManager::FitsWithinBudget(uint64 goldCost, PurchasePriority priority, BudgetAllocation const& budget) const
{
    switch (priority)
    {
        case PurchasePriority::CRITICAL:

            return goldCost <= budget.criticalBudget;
        case PurchasePriority::HIGH:

            return goldCost <= budget.highBudget;
        case PurchasePriority::MEDIUM:

            return goldCost <= budget.mediumBudget;
        case PurchasePriority::LOW:

            return goldCost <= budget.lowBudget;
        default:

            return false;
    }
}

// ============================================================================
// Reagent and Consumable Methods
// ============================================================================

::std::vector<uint32> VendorInteractionManager::GetRequiredReagents() const
{
    ::std::vector<uint32> reagents;

    if (!m_bot)
        return reagents;

    uint32 classId = m_bot->GetClass();
    // Class-specific reagents (WoW 12.0 item IDs)
    // Note: These are examples - actual item IDs should be verified in game DB
    switch (classId)
    {
        case CLASS_ROGUE:
            // Poisons, blinding powder

            reagents.push_back(5140);  // Flash Powder

            reagents.push_back(5530);  // Blinding Powder

            break;

        case CLASS_WARLOCK:
            // Soul shards handled differently (quest items)

            break;

        case CLASS_MAGE:

            reagents.push_back(17031); // Rune of Teleportation

            reagents.push_back(17032); // Rune of Portals

            break;

        case CLASS_PRIEST:

            reagents.push_back(17029); // Sacred Candle

            break;

        case CLASS_SHAMAN:

            reagents.push_back(17030); // Ankh

            break;

        case CLASS_DRUID:

            reagents.push_back(17034); // Maple Seed

            reagents.push_back(17035); // Stranglethorn Seed

            break;

        case CLASS_PALADIN:

            reagents.push_back(21177); // Symbol of Kings

            break;

        case CLASS_MONK:
            // No reagents typically

            break;

        case CLASS_DEATH_KNIGHT:
            // No consumable reagents

            break;

        case CLASS_DEMON_HUNTER:
            // No consumable reagents

            break;

        case CLASS_EVOKER:
            // No consumable reagents

            break;

        default:

            break;
    }

    return reagents;
}

::std::vector<uint32> VendorInteractionManager::GetRequiredConsumables() const
{
    ::std::vector<uint32> consumables;

    if (!m_bot)
        return consumables;

    uint32 botLevel = m_bot->GetLevel();
    // Level-appropriate food and water
    // Note: Item IDs should be verified in game database
    if (botLevel <= 5)
    {
        consumables.push_back(159);   // Refreshing Spring Water
        consumables.push_back(4540);  // Tough Hunk of Bread
    }
    else if (botLevel <= 15)
    {
        consumables.push_back(1179);  // Ice Cold Milk
        consumables.push_back(4541);  // Freshly Baked Bread
    }
    else if (botLevel <= 25)
    {
        consumables.push_back(1205);  // Melon Juice
        consumables.push_back(4542);  // Moist Cornbread
    }
    else if (botLevel <= 35)
    {
        consumables.push_back(1708);  // Sweet Nectar
        consumables.push_back(4544);  // Mulgore Spice Bread
    }
    else if (botLevel <= 45)
    {
        consumables.push_back(1645);  // Moonberry Juice
        consumables.push_back(4601);  // Soft Banana Bread
    }
    else
    {
        consumables.push_back(8766);  // Morning Glory Dew
        consumables.push_back(8950);  // Homemade Cherry Pie
    }

    return consumables;
}

bool VendorInteractionManager::NeedsAmmunition() const
{
    if (!m_bot)
        return false;

    // Only hunters use ammunition
    return m_bot->GetClass() == CLASS_HUNTER;
}

uint32 VendorInteractionManager::GetAppropriateAmmunition() const
{
    if (!m_bot || m_bot->GetClass() != CLASS_HUNTER)
        return 0;

    uint32 botLevel = m_bot->GetLevel();
    // Level-appropriate ammunition
    // Note: Item IDs should be verified in game database
    if (botLevel <= 10)
        return 2512;  // Rough Arrow
    else if (botLevel <= 25)
        return 2515;  // Sharp Arrow
    else if (botLevel <= 40)
        return 3030;  // Razor Arrow
    else
        return 11285; // Jagged Arrow

    return 0;
}

// ============================================================================
// Inventory Validation
// ============================================================================

bool VendorInteractionManager::HasBagSpace(uint32 itemId, uint32 quantity) const
{
    if (!m_bot)
        return false;

    ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
    if (!itemTemplate)
        return false;

    // Use TrinityCore API to check bag space
    ItemPosCountVec dest;
    InventoryResult result = m_bot->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, quantity);
    return result == EQUIP_ERR_OK;
}

uint32 VendorInteractionManager::GetFreeBagSlots() const
{
    if (!m_bot)
        return 0;

    uint32 freeSlots = 0;

    // Check main bag
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
    {
        if (!m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))

            freeSlots++;
    }

    // Check additional bags
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
    {
        Bag* pBag = m_bot->GetBagByPos(bag);
        if (pBag)

            freeSlots += pBag->GetFreeSlots();
    }

    return freeSlots;
}

// ============================================================================
// Private Helper Methods
// ============================================================================

VendorItem const* VendorInteractionManager::FindVendorItem(Creature* vendor, uint32 itemId) const
{
    if (!vendor || !vendor->IsVendor())
        return nullptr;

    VendorItemData const* vendorItems = vendor->GetVendorItems();
    if (!vendorItems)
        return nullptr;

    for (VendorItem const& vendorItem : vendorItems->m_items)
    {
        if (vendorItem.item == itemId)

            return &vendorItem;
    }
    return nullptr;
}

bool VendorInteractionManager::ExecutePurchase(Creature* vendor, uint32 vendorSlot, uint32 itemId,

                                                uint32 quantity, uint8 bag, uint8 slot)
{
    if (!m_bot || !vendor)
        return false;

    // Use TrinityCore's Player::BuyItemFromVendorSlot API
    // This handles all the complex logic: gold deduction, bag management,
    // reputation discounts, extended costs, etc.
    bool success = m_bot->BuyItemFromVendorSlot(vendor->GetGUID(), vendorSlot, itemId, quantity, bag, slot);
    if (!success)
    {
        TC_LOG_DEBUG("bot.playerbot", "Bot %s: Purchase failed",

            m_bot->GetName().c_str());
        return false;
    }

    return true;
}

bool VendorInteractionManager::IsClassReagent(ItemTemplate const* item) const
{
    if (!item || !m_bot)
        return false;

    ::std::vector<uint32> reagents = GetRequiredReagents();
    return ::std::find(reagents.begin(), reagents.end(), item->GetId()) != reagents.end();
}

bool VendorInteractionManager::IsConsumable(ItemTemplate const* item) const
{
    if (!item)
        return false;

    return item->GetClass() == ITEM_CLASS_CONSUMABLE;
}

bool VendorInteractionManager::IsEquipmentUpgrade(ItemTemplate const* item) const
{
    if (!item || !m_bot)
        return false;

    // Check if it's equipment
    if (item->GetClass() != ITEM_CLASS_WEAPON && item->GetClass() != ITEM_CLASS_ARMOR)
        return false;

    // Check if bot can use it
    if (item->GetAllowableClass() && !(item->GetAllowableClass() & m_bot->GetClassMask()))
        return false;

    // Check level requirement
    if (item->GetBaseRequiredLevel() > m_bot->GetLevel())
        return false;

    // Simplified upgrade check: Compare item level to currently equipped item
    // Map InventoryType to equipment slot
    InventoryType invType = item->GetInventoryType();
    if (invType != INVTYPE_NON_EQUIP)
    {
        // Get equipment slot from inventory type
        uint8 eslot = NULL_SLOT;

        switch (invType)
        {

            case INVTYPE_HEAD:
            eslot = EQUIPMENT_SLOT_HEAD; break;

            case INVTYPE_NECK:
            eslot = EQUIPMENT_SLOT_NECK; break;

            case INVTYPE_SHOULDERS:   eslot = EQUIPMENT_SLOT_SHOULDERS; break;

            case INVTYPE_BODY:
            eslot = EQUIPMENT_SLOT_BODY; break;

            case INVTYPE_CHEST:
            eslot = EQUIPMENT_SLOT_CHEST; break;

            case INVTYPE_WAIST:
            eslot = EQUIPMENT_SLOT_WAIST; break;

            case INVTYPE_LEGS:
            eslot = EQUIPMENT_SLOT_LEGS; break;

            case INVTYPE_FEET:
            eslot = EQUIPMENT_SLOT_FEET; break;

            case INVTYPE_WRISTS:
            eslot = EQUIPMENT_SLOT_WRISTS; break;

            case INVTYPE_HANDS:
            eslot = EQUIPMENT_SLOT_HANDS; break;

            case INVTYPE_FINGER:
            eslot = EQUIPMENT_SLOT_FINGER1; break; // Check both rings later

            case INVTYPE_TRINKET:
            eslot = EQUIPMENT_SLOT_TRINKET1; break; // Check both trinkets later

            case INVTYPE_CLOAK:
            eslot = EQUIPMENT_SLOT_BACK; break;

            case INVTYPE_WEAPON:
            eslot = EQUIPMENT_SLOT_MAINHAND; break;

            case INVTYPE_SHIELD:
            eslot = EQUIPMENT_SLOT_OFFHAND; break;

            case INVTYPE_RANGED:
            eslot = EQUIPMENT_SLOT_RANGED; break;

            case INVTYPE_TABARD:
            eslot = EQUIPMENT_SLOT_TABARD; break;

            case INVTYPE_WEAPONOFFHAND: eslot = EQUIPMENT_SLOT_OFFHAND; break;

            case INVTYPE_HOLDABLE:
            eslot = EQUIPMENT_SLOT_OFFHAND; break;

            case INVTYPE_2HWEAPON:
            eslot = EQUIPMENT_SLOT_MAINHAND; break;

            case INVTYPE_WEAPONMAINHAND: eslot = EQUIPMENT_SLOT_MAINHAND; break;

            default:
            eslot = NULL_SLOT; break;
        }

        if (eslot != NULL_SLOT)
        {

            Item* currentItem = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, eslot);

            if (currentItem)

            {

                ItemTemplate const* currentTemplate = currentItem->GetTemplate();

                if (currentTemplate && item->GetBaseItemLevel() > currentTemplate->GetBaseItemLevel())

                    return true;

            }

            else

            {
                // No item equipped in that slot - this is an upgrade

                return true;

            }
        }
    }

    return false;
}

uint32 VendorInteractionManager::GetRecommendedQuantity(ItemTemplate const* item) const
{
    if (!item)
        return 1;

    // For stackable consumables, recommend appropriate stack sizes
    if (item->GetMaxStackSize() > 1)
    {
        if (IsConsumable(item))

            return ::std::min(FOOD_STACK_SIZE, item->GetMaxStackSize());

        if (item->GetClass() == ITEM_CLASS_PROJECTILE)

            return ::std::min(AMMO_STACK_SIZE, item->GetMaxStackSize());

        if (IsClassReagent(item))

            return ::std::min(REAGENT_STACK_SIZE, item->GetMaxStackSize());
    }

    // Default to 1 for non-stackable items
    return 1;
}

void VendorInteractionManager::RecordPurchase(uint32 itemId, uint64 goldCost, bool success)
{
    if (success)
    {
        m_stats.itemsPurchased++;
        m_stats.totalGoldSpent += goldCost;
    }
    else
    {
        m_stats.purchaseFailures++;
    }
}

size_t VendorInteractionManager::GetMemoryUsage() const
{
    size_t memory = sizeof(VendorInteractionManager);
    memory += m_priorityCache.size() * (sizeof(uint32) + sizeof(PurchasePriority));
    memory += m_priceCache.size() * (sizeof(uint32) + sizeof(uint64));
    return memory;
}

} // namespace Playerbot
