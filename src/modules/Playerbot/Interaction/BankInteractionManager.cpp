/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BankInteractionManager.h"
#include "CellImpl.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "GameObject.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Log.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "World.h"
#include "WorldSession.h"
#include <chrono>

namespace Playerbot
{

// Bank slot cost progression (in gold)
static constexpr uint32 BANK_BAG_SLOT_PRICES[] = {
    10 * GOLD,      // 1st slot: 10g
    25 * GOLD,      // 2nd slot: 25g
    50 * GOLD,      // 3rd slot: 50g
    100 * GOLD,     // 4th slot: 100g
    250 * GOLD,     // 5th slot: 250g
    500 * GOLD,     // 6th slot: 500g
    1000 * GOLD     // 7th slot: 1000g
};
static constexpr uint32 MAX_BANK_BAG_SLOTS = 7;

BankInteractionManager::BankInteractionManager(Player* bot)
    : m_bot(bot)
    , m_stats()
    , m_cpuUsage(0.0f)
    , m_totalOperationTime(0)
    , m_operationCount(0)
    , m_priorityCache()
    , m_cachedBankInfo()
    , m_lastBankInfoCheck(0)
{
}

// ============================================================================
// Core Bank Methods
// ============================================================================

bool BankInteractionManager::DepositItem(WorldObject* banker, Item* item, uint32 count)
{
    if (!m_bot || !banker || !item)
        return false;

    auto startTime = std::chrono::high_resolution_clock::now();

    // Verify banker
    if (!IsBanker(banker))
    {
        TC_LOG_DEBUG("playerbot", "BankInteractionManager[%s]: Invalid banker",
            m_bot->GetName().c_str());
        return false;
    }

    // Check distance
    if (!IsInBankRange(banker))
    {
        TC_LOG_DEBUG("playerbot", "BankInteractionManager[%s]: Too far from banker",
            m_bot->GetName().c_str());
        return false;
    }

    // Find empty bank slot
    uint8 bankSlot = FindEmptyBankSlot();
    if (bankSlot == 0xFF)
    {
        TC_LOG_DEBUG("playerbot", "BankInteractionManager[%s]: No empty bank slots",
            m_bot->GetName().c_str());
        return false;
    }

    // Determine count to deposit
    uint32 depositCount = (count == 0) ? item->GetCount() : std::min(count, item->GetCount());

    // Execute deposit
    bool success = ExecuteDeposit(item, bankSlot);

    if (success)
    {
        RecordDeposit(item->GetEntry(), depositCount);
        m_lastBankInfoCheck = 0; // Invalidate cache

        TC_LOG_DEBUG("playerbot", "BankInteractionManager[%s]: Deposited %u x %s",
            m_bot->GetName().c_str(), depositCount, item->GetTemplate()->GetName(sWorld->GetDefaultDbcLocale()));
    }

    // Track performance
    auto endTime = std::chrono::high_resolution_clock::now();
    uint32 duration = static_cast<uint32>(
        std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count());
    m_totalOperationTime += duration;
    m_operationCount++;
    m_cpuUsage = static_cast<float>(m_totalOperationTime) / (m_operationCount * 1000.0f);

    return success;
}

bool BankInteractionManager::DepositItemById(WorldObject* banker, uint32 itemId, uint32 count)
{
    if (!m_bot || !banker)
        return false;

    // Find items with this ID in inventory
    std::vector<Item*> items;
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
    {
        if (Bag* pBag = m_bot->GetBagByPos(bag))
        {
            for (uint8 slot = 0; slot < pBag->GetBagSize(); ++slot)
            {
                if (Item* item = pBag->GetItemByPos(slot))
                {
                    if (item->GetEntry() == itemId)
                        items.push_back(item);
                }
            }
        }
    }

    // Also check main inventory slots
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
    {
        if (Item* item = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
        {
            if (item->GetEntry() == itemId)
                items.push_back(item);
        }
    }

    if (items.empty())
    {
        TC_LOG_DEBUG("playerbot", "BankInteractionManager[%s]: Item %u not found in inventory",
            m_bot->GetName().c_str(), itemId);
        return false;
    }

    // Deposit items up to count
    uint32 deposited = 0;
    for (Item* item : items)
    {
        uint32 toDeposit = (count == 0) ? item->GetCount() : std::min(count - deposited, item->GetCount());
        if (toDeposit == 0)
            break;

        if (DepositItem(banker, item, toDeposit))
        {
            deposited += toDeposit;
            if (count > 0 && deposited >= count)
                break;
        }
    }

    return deposited > 0;
}

bool BankInteractionManager::WithdrawItem(WorldObject* banker, uint32 itemId, uint32 count)
{
    if (!m_bot || !banker)
        return false;

    auto startTime = std::chrono::high_resolution_clock::now();

    // Verify banker
    if (!IsBanker(banker))
        return false;

    // Check distance
    if (!IsInBankRange(banker))
        return false;

    // Find item in bank
    uint32 withdrawn = 0;

    // Search main bank slots
    for (uint8 slot = BANK_SLOT_START; slot < BANK_SLOT_END; ++slot)
    {
        Item* item = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!item || item->GetEntry() != itemId)
            continue;

        uint32 toWithdraw = (count == 0) ? item->GetCount() : std::min(count - withdrawn, item->GetCount());
        if (toWithdraw == 0)
            break;

        if (ExecuteWithdraw(slot, toWithdraw))
        {
            withdrawn += toWithdraw;
            RecordWithdraw(itemId, toWithdraw);

            if (count > 0 && withdrawn >= count)
                break;
        }
    }

    // Search bank bags
    for (uint8 bag = BANK_BAG_SLOT_START; bag < BANK_BAG_SLOT_END; ++bag)
    {
        if (count > 0 && withdrawn >= count)
            break;

        Bag* pBag = m_bot->GetBagByPos(bag);
        if (!pBag)
            continue;

        for (uint8 slot = 0; slot < pBag->GetBagSize(); ++slot)
        {
            Item* item = pBag->GetItemByPos(slot);
            if (!item || item->GetEntry() != itemId)
                continue;

            uint32 toWithdraw = (count == 0) ? item->GetCount() : std::min(count - withdrawn, item->GetCount());
            if (toWithdraw == 0)
                break;

            // Move to inventory
            ItemPosCountVec dest;
            InventoryResult result = m_bot->CanStoreItem(NULL_BAG, NULL_SLOT, dest, item, false);
            if (result == EQUIP_ERR_OK)
            {
                m_bot->RemoveItem(bag, slot, true);
                m_bot->StoreItem(dest, item, true);
                withdrawn += toWithdraw;
                RecordWithdraw(itemId, toWithdraw);
            }

            if (count > 0 && withdrawn >= count)
                break;
        }
    }

    if (withdrawn > 0)
    {
        m_lastBankInfoCheck = 0; // Invalidate cache

        TC_LOG_DEBUG("playerbot", "BankInteractionManager[%s]: Withdrew %u x item %u",
            m_bot->GetName().c_str(), withdrawn, itemId);
    }

    // Track performance
    auto endTime = std::chrono::high_resolution_clock::now();
    uint32 duration = static_cast<uint32>(
        std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count());
    m_totalOperationTime += duration;
    m_operationCount++;

    return withdrawn > 0;
}

uint32 BankInteractionManager::SmartDeposit(WorldObject* banker)
{
    if (!m_bot || !banker)
        return 0;

    // Get items that should be deposited
    std::vector<Item*> itemsToDeposit = GetItemsToDeposit();

    uint32 deposited = 0;
    for (Item* item : itemsToDeposit)
    {
        if (DepositItem(banker, item, 0))
            deposited++;
    }

    TC_LOG_DEBUG("playerbot", "BankInteractionManager[%s]: Smart deposit moved %u items",
        m_bot->GetName().c_str(), deposited);

    return deposited;
}

uint32 BankInteractionManager::SmartWithdraw(WorldObject* banker)
{
    if (!m_bot || !banker)
        return 0;

    // Get items that should be withdrawn
    std::vector<uint32> itemsToWithdraw = GetItemsToWithdraw();

    uint32 withdrawn = 0;
    for (uint32 itemId : itemsToWithdraw)
    {
        if (WithdrawItem(banker, itemId, 0))
            withdrawn++;
    }

    TC_LOG_DEBUG("playerbot", "BankInteractionManager[%s]: Smart withdraw moved %u items",
        m_bot->GetName().c_str(), withdrawn);

    return withdrawn;
}

bool BankInteractionManager::OrganizeBank(WorldObject* banker)
{
    if (!m_bot || !banker)
        return false;

    if (!IsBanker(banker) || !IsInBankRange(banker))
        return false;

    // Collect all bank items and sort them
    std::vector<Item*> bankItems = GetBankItems();

    // Sort by item class, then subclass, then name
    std::sort(bankItems.begin(), bankItems.end(), [](Item* a, Item* b) {
        ItemTemplate const* tmplA = a->GetTemplate();
        ItemTemplate const* tmplB = b->GetTemplate();

        if (tmplA->GetClass() != tmplB->GetClass())
            return tmplA->GetClass() < tmplB->GetClass();
        if (tmplA->GetSubClass() != tmplB->GetSubClass())
            return tmplA->GetSubClass() < tmplB->GetSubClass();
        return tmplA->GetId() < tmplB->GetId();
    });

    // Stack consolidation - find items that can be stacked
    std::unordered_map<uint32, std::vector<Item*>> stackableItems;
    for (Item* item : bankItems)
    {
        if (item->GetTemplate()->GetMaxStackSize() > 1)
        {
            stackableItems[item->GetEntry()].push_back(item);
        }
    }

    // Consolidate stacks
    for (auto& [itemId, items] : stackableItems)
    {
        if (items.size() < 2)
            continue;

        ItemTemplate const* tmpl = items[0]->GetTemplate();
        uint32 maxStack = tmpl->GetMaxStackSize();

        // Merge partial stacks
        for (size_t i = 0; i < items.size(); ++i)
        {
            if (!items[i] || items[i]->GetCount() >= maxStack)
                continue;

            for (size_t j = i + 1; j < items.size(); ++j)
            {
                if (!items[j] || items[j]->GetCount() == 0)
                    continue;

                uint32 canTake = maxStack - items[i]->GetCount();
                uint32 toTake = std::min(canTake, items[j]->GetCount());

                if (toTake > 0)
                {
                    items[i]->SetCount(items[i]->GetCount() + toTake);
                    items[j]->SetCount(items[j]->GetCount() - toTake);

                    if (items[j]->GetCount() == 0)
                    {
                        m_bot->DestroyItem(items[j]->GetBagSlot(), items[j]->GetSlot(), true);
                        items[j] = nullptr;
                    }
                }

                if (items[i]->GetCount() >= maxStack)
                    break;
            }
        }
    }

    m_stats.organizationRuns++;
    m_lastBankInfoCheck = 0; // Invalidate cache

    TC_LOG_DEBUG("playerbot", "BankInteractionManager[%s]: Organized bank",
        m_bot->GetName().c_str());

    return true;
}

// ============================================================================
// Bank Analysis Methods
// ============================================================================

BankInteractionManager::BankSpaceInfo BankInteractionManager::GetBankSpaceInfo() const
{
    if (!m_bot)
        return BankSpaceInfo();

    // Check cache
    uint32 currentTime = GameTime::GetGameTimeMS();
    if (m_lastBankInfoCheck > 0 &&
        (currentTime - m_lastBankInfoCheck) < BANK_CACHE_DURATION)
    {
        return m_cachedBankInfo;
    }

    BankSpaceInfo info;

    // Count main bank slots
    uint32 mainBankSlots = BANK_SLOT_END - BANK_SLOT_START;
    info.totalSlots = mainBankSlots;

    for (uint8 slot = BANK_SLOT_START; slot < BANK_SLOT_END; ++slot)
    {
        Item* item = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (item)
        {
            info.usedSlots++;
            info.estimatedValue += item->GetTemplate()->GetSellPrice() * item->GetCount();
        }
    }

    // Count bank bag slots
    info.bagSlotsPurchased = GetPurchasedBagSlots();
    info.maxBagSlots = MAX_BANK_BAG_SLOTS;

    // Count slots in bank bags
    for (uint8 bag = BANK_BAG_SLOT_START; bag < BANK_BAG_SLOT_END; ++bag)
    {
        Bag* pBag = m_bot->GetBagByPos(bag);
        if (!pBag)
            continue;

        info.totalSlots += pBag->GetBagSize();

        for (uint8 slot = 0; slot < pBag->GetBagSize(); ++slot)
        {
            Item* item = pBag->GetItemByPos(slot);
            if (item)
            {
                info.usedSlots++;
                info.estimatedValue += item->GetTemplate()->GetSellPrice() * item->GetCount();
            }
        }
    }

    info.freeSlots = info.totalSlots - info.usedSlots;

    // Update cache
    m_cachedBankInfo = info;
    m_lastBankInfoCheck = currentTime;

    return info;
}

BankInteractionManager::ItemEvaluation BankInteractionManager::EvaluateItem(Item* item) const
{
    ItemEvaluation eval;

    if (!item)
        return eval;

    ItemTemplate const* tmpl = item->GetTemplate();
    if (!tmpl)
        return eval;

    eval.itemId = item->GetEntry();
    eval.itemGuid = item->GetGUID().GetCounter();
    eval.stackCount = item->GetCount();
    eval.priority = CalculateStoragePriority(tmpl);

    // Determine actions based on priority
    switch (eval.priority)
    {
        case StoragePriority::INVENTORY_ONLY:
            eval.shouldBank = false;
            eval.shouldWithdraw = true;
            eval.reason = "Quest/equipped item - keep in inventory";
            break;

        case StoragePriority::PREFER_INVENTORY:
            eval.shouldBank = false;
            eval.shouldWithdraw = false;
            eval.reason = "Consumable/reagent - prefer inventory";
            break;

        case StoragePriority::PREFER_BANK:
            eval.shouldBank = true;
            eval.shouldWithdraw = false;
            eval.reason = "Profession material - prefer bank";
            break;

        case StoragePriority::BANK_ONLY:
            eval.shouldBank = true;
            eval.shouldWithdraw = false;
            eval.reason = "Rare/collectible item - keep in bank";
            break;
    }

    return eval;
}

BankInteractionManager::StoragePriority BankInteractionManager::CalculateStoragePriority(ItemTemplate const* itemTemplate) const
{
    if (!itemTemplate)
        return StoragePriority::PREFER_BANK;

    // Check priority cache
    auto it = m_priorityCache.find(itemTemplate->GetId());
    if (it != m_priorityCache.end())
        return it->second;

    StoragePriority priority = StoragePriority::PREFER_BANK;

    // Quest items - keep in inventory
    if (itemTemplate->GetClass() == ITEM_CLASS_QUEST ||
        (itemTemplate->GetFlags() & ITEM_FLAG_IS_BOUND_TO_ACCOUNT))
    {
        priority = StoragePriority::INVENTORY_ONLY;
    }
    // Consumables (food, potions, etc.)
    else if (IsConsumable(itemTemplate))
    {
        priority = StoragePriority::PREFER_INVENTORY;
    }
    // Equipment
    else if (itemTemplate->GetClass() == ITEM_CLASS_WEAPON ||
             itemTemplate->GetClass() == ITEM_CLASS_ARMOR)
    {
        // Soulbound equipment stays in inventory
        if (itemTemplate->GetBonding() == BIND_ON_ACQUIRE)
            priority = StoragePriority::PREFER_INVENTORY;
        else
            priority = StoragePriority::PREFER_BANK;
    }
    // Trade goods / profession materials
    else if (IsProfessionMaterial(itemTemplate))
    {
        priority = StoragePriority::PREFER_BANK;
    }
    // Rare/epic items
    else if (itemTemplate->GetQuality() >= ITEM_QUALITY_RARE)
    {
        priority = StoragePriority::BANK_ONLY;
    }

    // Cache result
    const_cast<BankInteractionManager*>(this)->m_priorityCache[itemTemplate->GetId()] = priority;

    return priority;
}

uint32 BankInteractionManager::GetBankItemCount(uint32 itemId) const
{
    if (!m_bot)
        return 0;

    uint32 count = 0;

    // Count in main bank slots
    for (uint8 slot = BANK_SLOT_START; slot < BANK_SLOT_END; ++slot)
    {
        Item* item = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (item && item->GetEntry() == itemId)
            count += item->GetCount();
    }

    // Count in bank bags
    for (uint8 bag = BANK_BAG_SLOT_START; bag < BANK_BAG_SLOT_END; ++bag)
    {
        Bag* pBag = m_bot->GetBagByPos(bag);
        if (!pBag)
            continue;

        for (uint8 slot = 0; slot < pBag->GetBagSize(); ++slot)
        {
            Item* item = pBag->GetItemByPos(slot);
            if (item && item->GetEntry() == itemId)
                count += item->GetCount();
        }
    }

    return count;
}

bool BankInteractionManager::IsItemInBank(uint32 itemId) const
{
    return GetBankItemCount(itemId) > 0;
}

std::vector<Item*> BankInteractionManager::GetBankItems() const
{
    std::vector<Item*> items;

    if (!m_bot)
        return items;

    // Get items from main bank slots
    for (uint8 slot = BANK_SLOT_START; slot < BANK_SLOT_END; ++slot)
    {
        Item* item = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (item)
            items.push_back(item);
    }

    // Get items from bank bags
    for (uint8 bag = BANK_BAG_SLOT_START; bag < BANK_BAG_SLOT_END; ++bag)
    {
        Bag* pBag = m_bot->GetBagByPos(bag);
        if (!pBag)
            continue;

        for (uint8 slot = 0; slot < pBag->GetBagSize(); ++slot)
        {
            Item* item = pBag->GetItemByPos(slot);
            if (item)
                items.push_back(item);
        }
    }

    return items;
}

bool BankInteractionManager::HasBankSpace(uint32 slotsNeeded) const
{
    BankSpaceInfo info = GetBankSpaceInfo();
    return info.freeSlots >= slotsNeeded;
}

// ============================================================================
// Bank Bag Slot Management
// ============================================================================

uint32 BankInteractionManager::GetPurchasedBagSlots() const
{
    if (!m_bot)
        return 0;

    uint32 purchased = 0;
    for (uint8 slot = BANK_BAG_SLOT_START; slot < BANK_BAG_SLOT_END; ++slot)
    {
        if (m_bot->GetBankBagSlotCount() > (slot - BANK_BAG_SLOT_START))
            purchased++;
    }

    return purchased;
}

uint32 BankInteractionManager::GetNextBagSlotCost() const
{
    uint32 purchased = GetPurchasedBagSlots();
    if (purchased >= MAX_BANK_BAG_SLOTS)
        return 0;

    return BANK_BAG_SLOT_PRICES[purchased];
}

bool BankInteractionManager::PurchaseBagSlot(Creature* banker)
{
    if (!m_bot || !banker)
        return false;

    // Verify banker
    if (!banker->IsBanker())
        return false;

    // Check if all slots purchased
    uint32 cost = GetNextBagSlotCost();
    if (cost == 0)
    {
        TC_LOG_DEBUG("playerbot", "BankInteractionManager[%s]: All bank bag slots already purchased",
            m_bot->GetName().c_str());
        return false;
    }

    // Check if can afford
    if (!CanAffordBagSlot())
    {
        TC_LOG_DEBUG("playerbot", "BankInteractionManager[%s]: Cannot afford bank bag slot (cost: %u)",
            m_bot->GetName().c_str(), cost);
        return false;
    }

    // Purchase the slot
    m_bot->ModifyMoney(-static_cast<int64>(cost));
    m_bot->SetBankBagSlotCount(m_bot->GetBankBagSlotCount() + 1);

    m_stats.bagSlotsPurchased++;
    m_stats.totalGoldSpent += cost;
    m_lastBankInfoCheck = 0; // Invalidate cache

    TC_LOG_DEBUG("playerbot", "BankInteractionManager[%s]: Purchased bank bag slot for %u copper",
        m_bot->GetName().c_str(), cost);

    return true;
}

bool BankInteractionManager::CanAffordBagSlot() const
{
    if (!m_bot)
        return false;

    uint32 cost = GetNextBagSlotCost();
    return cost > 0 && m_bot->HasEnoughMoney(cost);
}

// ============================================================================
// Utility Methods
// ============================================================================

bool BankInteractionManager::IsBanker(WorldObject* target) const
{
    if (!target)
        return false;

    // Check if it's a creature banker
    if (Creature* creature = target->ToCreature())
        return creature->IsBanker();

    // Check if it's a bank chest (GameObject)
    if (GameObject* go = target->ToGameObject())
    {
        // Full bank GameObject validation with faction and proximity checks
        // Current behavior: Checks if GameObject type is CHEST (any chest treated as bank)
        // Full implementation should:
        // - Verify GameObject has GAMEOBJECT_FLAG_INTERACT (interactable)
        // - Check specific GameObjectType for bank chests vs regular chests
        // - Validate faction access permissions (guild banks, faction-specific banks)
        // - Ensure GameObject has valid bank data (not a loot chest)
        // Reference: TrinityCore GameObjectData, GAMEOBJECT_TYPE_BANK constant
        return go->GetGoType() == GAMEOBJECT_TYPE_CHEST;
    }

    return false;
}

Creature* BankInteractionManager::FindNearestBanker(float maxRange) const
{
    if (!m_bot)
        return nullptr;

    Creature* nearest = nullptr;
    float nearestDist = maxRange;

    std::list<Creature*> creatures;
    Trinity::AllCreaturesOfEntryInRange check(m_bot, 0, maxRange);
    Trinity::CreatureListSearcher<Trinity::AllCreaturesOfEntryInRange> searcher(m_bot, creatures, check);
    Cell::VisitGridObjects(m_bot, searcher, maxRange);

    for (Creature* creature : creatures)
    {
        if (!creature || !creature->IsAlive())
            continue;

        if (!creature->IsBanker())
            continue;

        float dist = m_bot->GetDistance(creature);
        if (dist < nearestDist)
        {
            nearest = creature;
            nearestDist = dist;
        }
    }

    return nearest;
}

bool BankInteractionManager::IsInBankRange(WorldObject* banker) const
{
    if (!m_bot || !banker)
        return false;

    static constexpr float BANK_INTERACTION_DISTANCE = 10.0f;
    return m_bot->GetDistance(banker) <= BANK_INTERACTION_DISTANCE;
}

size_t BankInteractionManager::GetMemoryUsage() const
{
    size_t usage = sizeof(*this);
    usage += m_priorityCache.size() * sizeof(std::pair<uint32, StoragePriority>);
    return usage;
}

// ============================================================================
// Private Helper Methods
// ============================================================================

uint8 BankInteractionManager::FindEmptyBankSlot() const
{
    if (!m_bot)
        return 0xFF;

    // Check main bank slots first
    for (uint8 slot = BANK_SLOT_START; slot < BANK_SLOT_END; ++slot)
    {
        if (!m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            return slot;
    }

    // Check bank bags
    for (uint8 bag = BANK_BAG_SLOT_START; bag < BANK_BAG_SLOT_END; ++bag)
    {
        Bag* pBag = m_bot->GetBagByPos(bag);
        if (!pBag)
            continue;

        for (uint8 slot = 0; slot < pBag->GetBagSize(); ++slot)
        {
            if (!pBag->GetItemByPos(slot))
                return slot; // Note: This returns bag slot, need to track bag ID too
        }
    }

    return 0xFF;
}

bool BankInteractionManager::ExecuteDeposit(Item* item, uint8 bankSlot)
{
    if (!m_bot || !item)
        return false;

    // Use TrinityCore's bank item function
    ItemPosCountVec dest;
    dest.push_back(ItemPosCount((INVENTORY_SLOT_BAG_0 << 8) | bankSlot, item->GetCount()));

    InventoryResult result = m_bot->CanBankItem(NULL_BAG, NULL_SLOT, dest, item, false);
    if (result != EQUIP_ERR_OK)
    {
        TC_LOG_DEBUG("playerbot", "BankInteractionManager[%s]: CanBankItem failed: %u",
            m_bot->GetName().c_str(), static_cast<uint32>(result));
        return false;
    }

    m_bot->RemoveItem(item->GetBagSlot(), item->GetSlot(), true);
    m_bot->BankItem(dest, item, true);

    return true;
}

bool BankInteractionManager::ExecuteWithdraw(uint8 bankSlot, uint32 count)
{
    if (!m_bot)
        return false;

    Item* item = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, bankSlot);
    if (!item)
        return false;

    ItemPosCountVec dest;
    InventoryResult result = m_bot->CanStoreItem(NULL_BAG, NULL_SLOT, dest, item, false);
    if (result != EQUIP_ERR_OK)
    {
        TC_LOG_DEBUG("playerbot", "BankInteractionManager[%s]: CanStoreItem failed: %u",
            m_bot->GetName().c_str(), static_cast<uint32>(result));
        return false;
    }

    m_bot->RemoveItem(INVENTORY_SLOT_BAG_0, bankSlot, true);
    m_bot->StoreItem(dest, item, true);

    return true;
}

bool BankInteractionManager::IsQuestItem(uint32 itemId) const
{
    if (!m_bot)
        return false;

    ItemTemplate const* tmpl = sObjectMgr->GetItemTemplate(itemId);
    if (!tmpl)
        return false;

    // Check if it's a quest-starting item
    if (tmpl->GetClass() == ITEM_CLASS_QUEST)
        return true;

    // Check if any active quests need this item
    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = m_bot->GetQuestSlotQuestId(slot);
        if (!questId)
            continue;

        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            continue;

        // Check quest objectives for this item
        for (uint8 i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
        {
            if (quest->RequiredItemId[i] == itemId)
                return true;
        }
    }

    return false;
}

bool BankInteractionManager::IsProfessionMaterial(ItemTemplate const* itemTemplate) const
{
    if (!itemTemplate)
        return false;

    // Trade goods class
    if (itemTemplate->GetClass() == ITEM_CLASS_TRADE_GOODS)
        return true;

    // Recipes
    if (itemTemplate->GetClass() == ITEM_CLASS_RECIPE)
        return true;

    return false;
}

bool BankInteractionManager::IsConsumable(ItemTemplate const* itemTemplate) const
{
    if (!itemTemplate)
        return false;

    return itemTemplate->GetClass() == ITEM_CLASS_CONSUMABLE;
}

std::vector<Item*> BankInteractionManager::GetItemsToDeposit() const
{
    std::vector<Item*> items;

    if (!m_bot)
        return items;

    // Check inventory for items that should be banked
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
    {
        Bag* pBag = m_bot->GetBagByPos(bag);
        if (!pBag)
            continue;

        for (uint8 slot = 0; slot < pBag->GetBagSize(); ++slot)
        {
            Item* item = pBag->GetItemByPos(slot);
            if (!item)
                continue;

            ItemEvaluation eval = EvaluateItem(item);
            if (eval.shouldBank)
                items.push_back(item);
        }
    }

    // Check main inventory
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
    {
        Item* item = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!item)
            continue;

        ItemEvaluation eval = EvaluateItem(item);
        if (eval.shouldBank)
            items.push_back(item);
    }

    return items;
}

std::vector<uint32> BankInteractionManager::GetItemsToWithdraw() const
{
    std::vector<uint32> itemIds;

    if (!m_bot)
        return itemIds;

    // Check quest requirements
    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = m_bot->GetQuestSlotQuestId(slot);
        if (!questId)
            continue;

        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            continue;

        // Check if bank has quest items
        for (uint8 i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
        {
            uint32 itemId = quest->RequiredItemId[i];
            if (itemId && IsItemInBank(itemId))
            {
                itemIds.push_back(itemId);
            }
        }
    }

    // Remove duplicates
    std::sort(itemIds.begin(), itemIds.end());
    itemIds.erase(std::unique(itemIds.begin(), itemIds.end()), itemIds.end());

    return itemIds;
}

void BankInteractionManager::RecordDeposit(uint32 itemId, uint32 count)
{
    m_stats.itemsDeposited += count;
    m_stats.depositOperations++;
}

void BankInteractionManager::RecordWithdraw(uint32 itemId, uint32 count)
{
    m_stats.itemsWithdrawn += count;
    m_stats.withdrawOperations++;
}

} // namespace Playerbot
