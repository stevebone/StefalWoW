/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
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

#include "InventoryManager.h"
#include "Player.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Creature.h"
#include "GameObject.h"
#include "Loot.h"
#include "LootMgr.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "Log.h"
#include "Config/PlayerbotConfig.h"
#include "Bag.h"
#include "World.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include <algorithm>
#include <execution>
#include "../Spatial/SpatialGridManager.h"  // Lock-free spatial grid for deadlock fix
#include "../Spatial/SpatialGridQueryHelpers.h"  // Thread-safe spatial queries
#include "GameTime.h"

namespace Playerbot
{

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

InventoryManager::InventoryManager(Player* bot)
    : _bot(bot)
    , _itemScoreCache(256)
    , _itemUsableCache(256)
{

    // Load configuration
    PlayerbotConfig* config = PlayerbotConfig::instance();
    _autoLootEnabled = config->GetBool("Playerbot.Inventory.AutoLoot", true);
    _autoEquipEnabled = config->GetBool("Playerbot.Inventory.AutoEquip", true);
    _autoSellEnabled = config->GetBool("Playerbot.Inventory.AutoSell", true);
    _updateInterval = config->GetUInt("Playerbot.Inventory.UpdateInterval", 2000);
    _minLootQuality = config->GetUInt("Playerbot.Inventory.MinLootQuality", 0);
    _minFreeSlots = config->GetUInt("Playerbot.Inventory.MinFreeSlots", 5);

    // Initialize stat weights based on class
    InitializeStatWeights();

    // Initial cache update
    UpdateEquipmentCache();
    UpdateInventoryCache();
}

InventoryManager::~InventoryManager()
{
    // Clean up caches
    _itemScoreCache.Clear();
    _itemUsableCache.Clear();
    _equippedItems.clear();
    _itemCounts.clear();
    _inventoryItems.clear();
}

// ============================================================================
// MAIN UPDATE
// ============================================================================

void InventoryManager::Update(uint32 diff)
{
    if (!_bot || !_bot->IsInWorld())
        return;

    auto startTime = ::std::chrono::steady_clock::now();

    // Throttle updates based on configuration
    _lastUpdateTime += diff;
    if (_lastUpdateTime < _updateInterval)
        return;

    _lastUpdateTime = 0;
    _metrics.totalUpdates++;

    // Update caches periodically (every 10 seconds)
    if (GameTime::GetGameTimeMS() - _lastCacheUpdate > 10000)
    {
        UpdateEquipmentCache();
        UpdateInventoryCache();
        _lastCacheUpdate = GameTime::GetGameTimeMS();
    }

    // Auto-loot nearby corpses and objects
    if (_autoLootEnabled && !_bot->IsInCombat())
    {
        uint32 lootedCount = AutoLoot();
        _metrics.itemsLooted += lootedCount;
    }

    // Optimize equipment (scan every 5 seconds)
    if (_autoEquipEnabled && GameTime::GetGameTimeMS() - _lastEquipScan > 5000)
    {
        uint32 equippedCount = OptimizeEquipment();
        _metrics.itemsEquipped += equippedCount;
        _lastEquipScan = GameTime::GetGameTimeMS();
    }

    // Organize bags when getting full (every 30 seconds or when < minFreeSlots)
    uint32 freeSlots = GetBagSpace();
    if (freeSlots < _minFreeSlots || GameTime::GetGameTimeMS() - _lastBagOrganize > 30000)
    {
        OrganizeBags();
        _metrics.bagsOrganized++;
        _lastBagOrganize = GameTime::GetGameTimeMS();

        // Destroy vendor trash if still full
    if (GetBagSpace() < _minFreeSlots)
        {
            uint32 destroyed = DestroyItemsForSpace(_minFreeSlots - GetBagSpace());
            _metrics.itemsDestroyed += destroyed;
        }
    }

    UpdateMetrics(startTime);
}

// ============================================================================
// LOOTING OPERATIONS
// ============================================================================

uint32 InventoryManager::AutoLoot(float maxRange)
{
    if (!_bot || !_bot->IsAlive() || IsBagsFull())
        return 0;

    uint32 itemsLooted = 0;
    auto lootables = FindLootableObjects(maxRange);

    // Clear looted objects cache periodically (every 60 seconds)
    static uint32 lastClear = 0;
    uint32 now = GameTime::GetGameTimeMS();
    if (now - lastClear > 60000)
    {
        _lootedObjects.clear();
        lastClear = now;
    }

    for (ObjectGuid const& guid : lootables)
    {
        // Skip recently looted objects
    if (_lootedObjects.find(guid) != _lootedObjects.end())
            continue;

        if (guid.IsCreature())
        {
            // PHASE 5D: Thread-safe spatial grid validation
            auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, guid);
            Creature* creature = nullptr;

            if (snapshot)
            {
                // Get Creature* for loot check (validated via snapshot first)
                creature = ObjectAccessor::GetCreature(*_bot, guid);
            }

            if (creature && LootCorpse(creature))
            {
                _lootedObjects.insert(guid);
                itemsLooted++;
            }
        }
        else if (guid.IsGameObject())
        {
            // PHASE 5D: Thread-safe spatial grid validation
            auto snapshot = SpatialGridQueryHelpers::FindGameObjectByGuid(_bot, guid);
            GameObject* go = nullptr;

            if (snapshot)
            {
                // Get GameObject* for loot check (validated via snapshot first)
                go = ObjectAccessor::GetGameObject(*_bot, guid);
            }

            if (go && LootGameObject(go))
            {
                _lootedObjects.insert(guid);
                itemsLooted++;
            }
        }

        // Stop if bags are full
    if (IsBagsFull())
            break;
    }

    return itemsLooted;
}

bool InventoryManager::LootCorpse(Creature* creature)
{
    if (!creature || creature->GetHealth() > 0 || !creature->HasDynamicFlag(UNIT_DYNFLAG_LOOTABLE))
        return false;

    // Check if bot can loot this creature
    if (!creature->IsWithinDistInMap(_bot, INTERACTION_DISTANCE))
        return false;

    Loot* loot = creature->m_loot.get();
    if (!loot || loot->isLooted())
        return false;

    uint32 lootedItems = ProcessLoot(loot);

    if (lootedItems > 0)
    {
        creature->RemoveDynamicFlag(UNIT_DYNFLAG_LOOTABLE);
        return true;
    }
    return false;
}

bool InventoryManager::LootGameObject(GameObject* go)
{
    if (!go || !go->IsWithinDistInMap(_bot, INTERACTION_DISTANCE))
        return false;

    // Check if GameObject is lootable
    LootStore const& store = LootTemplates_Gameobject;
    if (!store.HaveLootFor(go->GetEntry()))
        return false;

    Loot* loot = go->m_loot.get();
    if (!loot || loot->isLooted())
        return false;

    uint32 lootedItems = ProcessLoot(loot);

    if (lootedItems > 0)
    {
        go->SetLootState(GO_JUST_DEACTIVATED);
        return true;
    }

    return false;
}

uint32 InventoryManager::ProcessLoot(Loot* loot)
{
    if (!loot)
        return 0;

    uint32 itemsLooted = 0;

    for (LootItem& lootItem : loot->items)
    {
        if (lootItem.is_looted)
            continue;

        // Check if we should loot this item
    if (!ShouldLootItem(lootItem.itemid))
            continue;

        // Check if we have space
        ItemPosCountVec dest;
        InventoryResult msg = _bot->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, lootItem.itemid, lootItem.count);

        if (msg != EQUIP_ERR_OK)
        {
            // Try to make space if bags are full
    if (msg == EQUIP_ERR_INV_FULL || msg == EQUIP_ERR_BAG_FULL)
            {
                uint32 freed = DestroyItemsForSpace(1);
                if (freed > 0)
                {
                    msg = _bot->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, lootItem.itemid, lootItem.count);
                }
            }

            if (msg != EQUIP_ERR_OK)
                continue;
        }

        // Store the item
        Item* newItem = _bot->StoreNewItem(dest, lootItem.itemid, true, lootItem.randomBonusListId);
        if (newItem)
        {
            lootItem.is_looted = true;
            itemsLooted++;
            LogAction("Looted", newItem);
        }
    }

    // Loot money
    uint32 gold = loot->gold;
    if (gold > 0)
    {
        _bot->ModifyMoney(gold);
        loot->gold = 0;
    }

    return itemsLooted;
}

bool InventoryManager::ShouldLootItem(uint32 itemId) const
{
    // Check ignored items list
    if (_ignoredItems.find(itemId) != _ignoredItems.end())
        return false;

    ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
    if (!proto)
        return false;

    // Check minimum quality
    if (proto->GetQuality() < _minLootQuality)
        return false;

    // Always loot quest items
    if (proto->GetStartQuest() > 0 || proto->GetBonding() == BIND_QUEST)
        return true;

    // Always loot money
    if (proto->GetClass() == ITEM_CLASS_MONEY)
        return true;

    // Check if usable by class
    int32 allowableClass = proto->GetAllowableClass();
    if (allowableClass && !(allowableClass & _bot->GetClassMask()))
        return false;

    // Check if usable by race
    if (!proto->GetAllowableRace().IsEmpty() && !proto->GetAllowableRace().HasRace(_bot->GetRace()))
        return false;

    return true;
}

// ============================================================================
// EQUIPMENT OPTIMIZATION
// ============================================================================

uint32 InventoryManager::OptimizeEquipment()
{
    if (!_bot || _bot->IsInCombat())
        return 0;

    uint32 itemsEquipped = 0;

    // Scan all items in bags
    for (Item* item : GetAllItems())
    {
        if (!item)
            continue;

        // Check if this item is an equipment upgrade
    if (CanEquipUpgrade(item))
        {
            if (EquipItem(item))
            {
                itemsEquipped++;
                LogAction("Equipped upgrade", item);
            }
        }
    }

    return itemsEquipped;
}

bool InventoryManager::CanEquipUpgrade(Item* item) const
{
    if (!item)
        return false;

    ItemTemplate const* proto = item->GetTemplate();
    if (!proto)
        return false;

    // Check if item is equipment
    if (proto->GetClass() != ITEM_CLASS_WEAPON && proto->GetClass() != ITEM_CLASS_ARMOR)
        return false;

    // Check if bot can use this item
    bool canUse = false;
    if (!_itemUsableCache.Get(proto->GetId(), canUse))
    {
        canUse = CanUseItem(proto);
        const_cast<InventoryManager*>(this)->_itemUsableCache.Put(proto->GetId(), canUse);
    }

    if (!canUse)
        return false;

    // Get the slot this item would equip to
    uint8 slot = 0;
    switch (proto->GetInventoryType())
    {
        case INVTYPE_HEAD:      slot = EQUIPMENT_SLOT_HEAD; break;
        case INVTYPE_NECK:      slot = EQUIPMENT_SLOT_NECK; break;
        case INVTYPE_SHOULDERS: slot = EQUIPMENT_SLOT_SHOULDERS; break;
        case INVTYPE_BODY:      slot = EQUIPMENT_SLOT_BODY; break;
        case INVTYPE_CHEST:     slot = EQUIPMENT_SLOT_CHEST; break;
        case INVTYPE_WAIST:     slot = EQUIPMENT_SLOT_WAIST; break;
        case INVTYPE_LEGS:      slot = EQUIPMENT_SLOT_LEGS; break;
        case INVTYPE_FEET:      slot = EQUIPMENT_SLOT_FEET; break;
        case INVTYPE_WRISTS:    slot = EQUIPMENT_SLOT_WRISTS; break;
        case INVTYPE_HANDS:     slot = EQUIPMENT_SLOT_HANDS; break;
        case INVTYPE_FINGER:    slot = EQUIPMENT_SLOT_FINGER1; break;
        case INVTYPE_TRINKET:   slot = EQUIPMENT_SLOT_TRINKET1; break;
        case INVTYPE_WEAPON:    slot = EQUIPMENT_SLOT_MAINHAND; break;
        case INVTYPE_SHIELD:    slot = EQUIPMENT_SLOT_OFFHAND; break;
        case INVTYPE_RANGED:    slot = EQUIPMENT_SLOT_RANGED; break;
        case INVTYPE_CLOAK:     slot = EQUIPMENT_SLOT_BACK; break;
        case INVTYPE_2HWEAPON:  slot = EQUIPMENT_SLOT_MAINHAND; break;
        case INVTYPE_TABARD:    slot = EQUIPMENT_SLOT_TABARD; break;
        case INVTYPE_WEAPONMAINHAND: slot = EQUIPMENT_SLOT_MAINHAND; break;
        case INVTYPE_WEAPONOFFHAND:  slot = EQUIPMENT_SLOT_OFFHAND; break;
        case INVTYPE_HOLDABLE:  slot = EQUIPMENT_SLOT_OFFHAND; break;
        case INVTYPE_THROWN:    slot = EQUIPMENT_SLOT_RANGED; break;
        default: return false;
    }

    // Get currently equipped item
    Item* equipped = GetEquippedItem(slot);

    // If nothing equipped, this is an upgrade
    if (!equipped)
        return true;

    // Compare items
    float scoreDiff = CompareItems(item, equipped);
    return scoreDiff > 0;
}

float InventoryManager::CompareItems(Item* item1, Item* item2) const
{
    if (!item1 || !item2)
        return 0;

    float score1 = CalculateItemScore(item1);
    float score2 = CalculateItemScore(item2);

    return score1 - score2;
}

float InventoryManager::CalculateItemScore(Item* item) const
{
    if (!item)
        return 0;

    // Check cache first
    float cachedScore = 0;
    if (_itemScoreCache.Get(item->GetEntry(), cachedScore))
        return cachedScore;

    ItemTemplate const* proto = item->GetTemplate();
    if (!proto)
        return 0;

    float score = 0;

    // Base score from item level and quality
    score += proto->GetBaseItemLevel() * 1.0f;
    score += proto->GetQuality() * 10.0f;

    // Add stat values
    score += CalculateStatValue(proto);

    // Add armor/damage values
    if (proto->GetClass() == ITEM_CLASS_ARMOR)
    {
        score += proto->GetArmor(proto->GetBaseItemLevel()) * 0.5f;
    }
    else if (proto->GetClass() == ITEM_CLASS_WEAPON)
    {
        float dps = proto->GetDPS(proto->GetBaseItemLevel());
        score += dps * 10.0f;
    }

    // Cache the score
    const_cast<InventoryManager*>(this)->_itemScoreCache.Put(item->GetEntry(), score);

    return score;
}
bool InventoryManager::EquipItem(Item* item)
{
    if (!item || !_bot)
        return false;

    uint16 dest = 0;
    InventoryResult msg = _bot->CanEquipItem(NULL_SLOT, dest, item, false);

    if (msg != EQUIP_ERR_OK)
    {
        TC_LOG_DEBUG("module.playerbot", "InventoryManager::EquipItem: Cannot equip item {} - error {}",
                  item->GetEntry(), uint32(msg));
        return false;
    }

    // Remove item from current position
    _bot->RemoveItem(item->GetBagSlot(), item->GetSlot(), true);

    // Equip the item
    Item* equipped = _bot->EquipItem(dest, item, true);
    if (equipped)
    {
        // Update equipment cache
        UpdateEquipmentCache();
        return true;
    }

    return false;
}

bool InventoryManager::UnequipItem(uint8 slot)
{
    if (!_bot)
        return false;

    Item* item = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
    if (!item)
        return false;

    // Find free bag slot
    ItemPosCountVec dest;
    InventoryResult msg = _bot->CanStoreItem(NULL_BAG, NULL_SLOT, dest, item, false);
    if (msg != EQUIP_ERR_OK)
        return false;

    // Remove from equipment slot
    _bot->RemoveItem(INVENTORY_SLOT_BAG_0, slot, true);

    // Store in bags
    _bot->StoreItem(dest, item, true);

    // Update equipment cache
    UpdateEquipmentCache();

    return true;
}

// ============================================================================
// BAG MANAGEMENT
// ============================================================================

void InventoryManager::OrganizeBags()
{
    if (!_bot)
        return;

    // First consolidate stacks
    uint32 freedSlots = ConsolidateStacks();

    // Then sort items
    SortBags();

    // Update inventory cache
    UpdateInventoryCache();

    TC_LOG_DEBUG("module.playerbot", "InventoryManager::OrganizeBags: Freed {} slots", freedSlots);
}

uint32 InventoryManager::GetBagSpace() const
{
    if (!_bot)
        return 0;

    uint32 freeSlots = 0;

    // Check main backpack
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
    {
        if (!_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            freeSlots++;
    }

    // Check additional bags
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
    {
        Bag* pBag = _bot->GetBagByPos(bag);
        if (pBag)
        {
            for (uint32 slot = 0; slot < pBag->GetBagSize(); ++slot)
            {
                if (!pBag->GetItemByPos(slot))
                    freeSlots++;
            }
        }
    }

    return freeSlots;
}

uint32 InventoryManager::GetBagCapacity() const
{
    if (!_bot)
        return 0;

    uint32 capacity = INVENTORY_SLOT_ITEM_END - INVENTORY_SLOT_ITEM_START;

    // Add bag slots
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
    {
        Bag* pBag = _bot->GetBagByPos(bag);
        if (pBag)
            capacity += pBag->GetBagSize();
    }

    return capacity;
}

InventoryResult InventoryManager::FindBagSlot(uint32 itemId, uint32 count, ItemPosCountVec& dest)
{
    if (!_bot)
        return EQUIP_ERR_ITEM_NOT_FOUND;

    return _bot->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, count);
}

uint32 InventoryManager::ConsolidateStacks()
{
    if (!_bot)
        return 0;

    uint32 freedSlots = 0;
    ::std::unordered_map<uint32, ::std::vector<Item*>> stackableItems;

    // Find all stackable items
    for (Item* item : GetAllItems())
    {
        if (!item)
            continue;

        ItemTemplate const* proto = item->GetTemplate();
        if (!proto || proto->GetMaxStackSize() <= 1)
            continue;

        // Group by item ID
        stackableItems[item->GetEntry()].push_back(item);
    }

    // Consolidate each group
    for (auto& [itemId, items] : stackableItems)
    {
        if (items.size() <= 1)
            continue;

        // Sort by stack size (largest first)
        ::std::sort(items.begin(), items.end(),
            [](Item* a, Item* b) { return a->GetCount() > b->GetCount(); });

        // Merge smaller stacks into larger ones
    for (size_t i = 0; i < items.size() - 1; ++i)
        {
            Item* targetStack = items[i];
            uint32 maxStack = targetStack->GetTemplate()->GetMaxStackSize();

            if (targetStack->GetCount() >= maxStack)
                continue;

            for (size_t j = i + 1; j < items.size(); ++j)
            {
                Item* sourceStack = items[j];
                if (!sourceStack || sourceStack->GetCount() == 0)
                    continue;

                uint32 space = maxStack - targetStack->GetCount();
                uint32 toMove = ::std::min(space, sourceStack->GetCount());

                if (toMove > 0)
                {
                    targetStack->SetCount(targetStack->GetCount() + toMove);

                    if (sourceStack->GetCount() == toMove)
                    {
                        // Entire stack moved, destroy source
                        _bot->DestroyItem(sourceStack->GetBagSlot(), sourceStack->GetSlot(), true);
                        items[j] = nullptr;
                        freedSlots++;
                    }
                    else
                    {
                        // Partial stack moved
                        sourceStack->SetCount(sourceStack->GetCount() - toMove);
                    }
                }

                if (targetStack->GetCount() >= maxStack)
                    break;
            }
        }
    }

    return freedSlots;
}

void InventoryManager::SortBags()
{
    if (!_bot)
        return;

    // Collect all items
    ::std::vector<::std::pair<Item*, float>> itemsWithScore;

    for (Item* item : GetAllItems())
    {
        if (!item)
            continue;

        // Calculate sort score (quality * 1000 + item level)
        ItemTemplate const* proto = item->GetTemplate();
        float score = proto->GetQuality() * 1000 + proto->GetBaseItemLevel();
        itemsWithScore.push_back({item, score});
    }

    // Sort by score (highest first)
    ::std::sort(itemsWithScore.begin(), itemsWithScore.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    // Reorganize items in bags (best items in main backpack)
    uint8 targetBag = INVENTORY_SLOT_BAG_0;
    uint8 targetSlot = INVENTORY_SLOT_ITEM_START;

    for (auto& [item, score] : itemsWithScore)
    {
        // Skip if already in correct position
    if (item->GetBagSlot() == targetBag && item->GetSlot() == targetSlot)
        {
            targetSlot++;
            if (targetSlot >= INVENTORY_SLOT_ITEM_END && targetBag == INVENTORY_SLOT_BAG_0)
            {
                targetBag = INVENTORY_SLOT_BAG_START;
                targetSlot = 0;
            }
            continue;
        }

        // Move item to target position
        uint16 srcPos = (item->GetBagSlot() << 8) | item->GetSlot();
        uint16 dstPos = (targetBag << 8) | targetSlot;
        _bot->SwapItem(srcPos, dstPos);

        // Update target position
        targetSlot++;
        if (targetSlot >= INVENTORY_SLOT_ITEM_END && targetBag == INVENTORY_SLOT_BAG_0)
        {
            targetBag = INVENTORY_SLOT_BAG_START;
            targetSlot = 0;
        }
        else if (targetBag != INVENTORY_SLOT_BAG_0)
        {
            Bag* bag = _bot->GetBagByPos(targetBag);
            if (bag && targetSlot >= bag->GetBagSize())
            {
                targetBag++;
                targetSlot = 0;
                if (targetBag >= INVENTORY_SLOT_BAG_END)
                    break;
            }
        }
    }
}

// ============================================================================
// ITEM STORAGE
// ============================================================================

bool InventoryManager::StoreNewItem(uint32 itemId, uint32 count)
{
    if (!_bot)
        return false;

    ItemPosCountVec dest;
    InventoryResult msg = _bot->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, count);

    if (msg != EQUIP_ERR_OK)
    {
        TC_LOG_DEBUG("module.playerbot", "InventoryManager::StoreNewItem: Cannot store item {} x{} - error {}",
                  itemId, count, uint32(msg));
        return false;
    }

    Item* item = _bot->StoreNewItem(dest, itemId, true);
    if (item)
    {
        UpdateInventoryCache();
        return true;
    }

    return false;
}

bool InventoryManager::StoreItem(Item* item)
{
    if (!item || !_bot)
        return false;

    ItemPosCountVec dest;
    InventoryResult msg = _bot->CanStoreItem(NULL_BAG, NULL_SLOT, dest, item, false);

    if (msg != EQUIP_ERR_OK)
        return false;

    Item* stored = _bot->StoreItem(dest, item, true);
    if (stored)
    {
        UpdateInventoryCache();
        return true;
    }

    return false;
}

bool InventoryManager::DestroyItem(Item* item, bool update)
{
    if (!item || !_bot)
        return false;

    _bot->DestroyItem(item->GetBagSlot(), item->GetSlot(), update);
    UpdateInventoryCache();
    return true;
}

uint32 InventoryManager::DestroyItemsForSpace(uint32 slots)
{
    if (!_bot || slots == 0)
        return 0;

    uint32 freedSlots = 0;

    // Destroy items by priority (grey quality first, lowest value first)
    ::std::vector<::std::pair<Item*, float>> destroyableItems;

    for (Item* item : GetAllItems())
    {
        if (!item)
            continue;

        ItemTemplate const* proto = item->GetTemplate();
        if (!proto)
            continue;

        // Never destroy quest items
    if (proto->GetStartQuest() > 0 || proto->GetBonding() == BIND_QUEST)
            continue;

        // Never destroy items in ignore list
    if (_ignoredItems.find(proto->GetId()) != _ignoredItems.end())
            continue;

        // Calculate destroy priority (lower = destroy first)
        float priority = proto->GetQuality() * 1000 + proto->GetSellPrice();
        destroyableItems.push_back({item, priority});
    }

    // Sort by priority (lowest first)
    ::std::sort(destroyableItems.begin(), destroyableItems.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });

    // Destroy items until we have enough space
    for (auto& [item, priority] : destroyableItems)
    {
        if (freedSlots >= slots)
            break;

        if (DestroyItem(item, true))
        {
            freedSlots++;
            _metrics.itemsDestroyed++;
            LogAction("Destroyed for space", item);
        }
    }

    return freedSlots;
}

// ============================================================================
// VENDOR OPERATIONS
// ============================================================================

uint32 InventoryManager::SellVendorTrash(Creature* vendor)
{
    if (!_bot)
        return 0;

    uint32 totalGold = 0;
    ::std::vector<Item*> itemsToSell;

    // Find all vendor trash items
    for (Item* item : GetAllItems())
    {
        if (ShouldSellItem(item))
            itemsToSell.push_back(item);
    }

    // Sell items if vendor is provided
    if (vendor && vendor->IsVendor())
    {
        for (Item* item : itemsToSell)
        {
            uint32 sellPrice = item->GetTemplate()->GetSellPrice() * item->GetCount();

            // Destroy the item and add money (simulating vendor sale for bots)
            // Note: Full vendor transaction requires client packets which bots don't have
            _bot->DestroyItem(item->GetBagSlot(), item->GetSlot(), true);
            _bot->ModifyMoney(sellPrice);

            totalGold += sellPrice;
            _metrics.itemsSold++;
            LogAction("Sold to vendor", item);
        }
    }
    else
    {
        // Just calculate potential gold value
    for (Item* item : itemsToSell)
        {
            totalGold += item->GetTemplate()->GetSellPrice() * item->GetCount();
        }
    }

    return totalGold;
}

bool InventoryManager::ShouldSellItem(Item* item) const
{
    if (!item)
        return false;

    ItemTemplate const* proto = item->GetTemplate();
    if (!proto)
        return false;
    // Never sell quest items
    if (proto->GetStartQuest() > 0 || proto->GetBonding() == BIND_QUEST)
        return false;

    // Never sell ignored items
    if (_ignoredItems.find(proto->GetId()) != _ignoredItems.end())
        return false;

    // Sell grey quality items
    if (proto->GetQuality() == ITEM_QUALITY_POOR)
        return true;

    // Sell items that can't be used by this class
    int32 allowableClass = proto->GetAllowableClass();
    if (allowableClass && !(allowableClass & _bot->GetClassMask()))
        return true;
    return false;
}

uint32 InventoryManager::RepairEquipment(Creature* vendor)
{
    if (!_bot || !vendor || !vendor->IsArmorer())
        return 0;

    // Calculate total repair cost
    uint64 totalCost = 0;
    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        Item* item = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (item)
        {
            totalCost += item->CalculateDurabilityRepairCost(1.0f);
        }
    }

    if (totalCost <= 0)
        return 0;

    if (_bot->HasEnoughMoney(totalCost))
    {
        _bot->DurabilityRepairAll(true, 1.0f, false);
        return uint32(totalCost);
    }

    return 0;
}

bool InventoryManager::BuyFromVendor(Creature* vendor, uint32 itemId, uint32 count)
{
    if (!_bot || !vendor || !vendor->IsVendor())
        return false;

    // Check if vendor sells this item
    VendorItemData const* items = vendor->GetVendorItems();
    if (!items)
        return false;

    VendorItem const* item = items->FindItemCostPair(itemId, 0, 0);
    if (!item)
        return false;

    // Check if bot has enough money
    ItemTemplate const* itemProto = sObjectMgr->GetItemTemplate(itemId);
    uint32 price = itemProto ? itemProto->GetBuyPrice() : 0;

    if (_bot->GetMoney() < price * count)
        return false;

    // Check if bot has space
    ItemPosCountVec dest;
    InventoryResult msg = _bot->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, count);
    if (msg != EQUIP_ERR_OK)
        return false;

    // Buy the item
    _bot->BuyItemFromVendorSlot(vendor->GetGUID(), 0, itemId, count, NULL_BAG, NULL_SLOT);

    UpdateInventoryCache();
    return true;
}

// ============================================================================
// CONSUMABLES MANAGEMENT
// ============================================================================

bool InventoryManager::UseConsumable(uint32 itemId)
{
    if (!_bot)
        return false;

    Item* item = GetItemById(itemId);
    if (!item)
        return false;

    // Check if item is consumable
    ItemTemplate const* proto = item->GetTemplate();
    if (!proto || proto->GetClass() != ITEM_CLASS_CONSUMABLE)
        return false;

    // Use the consumable item
    // WoW 12.0: CastItemUseSpell signature changed to std::array<int32, 3>
    std::array<int32, 3> misc = { 0, 0, 0 };
    _bot->CastItemUseSpell(item, SpellCastTargets(), ObjectGuid::Empty, misc);
    return true;
}

uint32 InventoryManager::GetConsumableCount(uint32 itemId) const
{
    return GetItemCount(itemId, false);
}

bool InventoryManager::NeedsFood() const
{
    if (!_bot)
        return false;

    // Check if health is below 50%
    return _bot->GetHealthPct() < 50.0f && !_bot->IsInCombat();
}

bool InventoryManager::NeedsDrink() const
{
    if (!_bot)
        return false;

    // Check if mana user and mana is below 50%
    if (_bot->GetPowerType() != POWER_MANA)
        return false;

    return _bot->GetPowerPct(POWER_MANA) < 50.0f && !_bot->IsInCombat();
}

bool InventoryManager::UseFood()
{
    if (!NeedsFood())
        return false;

    // Find food items
    for (Item* item : GetAllItems())
    {
        if (!item)
            continue;

        ItemTemplate const* proto = item->GetTemplate();
        if (!proto || proto->GetClass() != ITEM_CLASS_CONSUMABLE || proto->GetSubClass() != ITEM_SUBCLASS_CONSUMABLE)
            continue;

        // Check if this is a food item by checking its spells
    for (auto const* effect : proto->Effects)
        {
            if (!effect || effect->SpellID <= 0)
                continue;

            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(effect->SpellID, DIFFICULTY_NONE);
            if (!spellInfo)
                continue;

            // Check if spell has a health restoration effect
    for (SpellEffectInfo const& spellEffect : spellInfo->GetEffects())
            {
                if (spellEffect.ApplyAuraName == SPELL_AURA_PERIODIC_HEAL ||
                    spellEffect.ApplyAuraName == SPELL_AURA_OBS_MOD_HEALTH)
                {
                    return UseConsumable(item->GetEntry());
                }
            }
        }
    }

    return false;
}

bool InventoryManager::UseDrink()
{
    if (!NeedsDrink())
        return false;

    // Find drink items
    for (Item* item : GetAllItems())
    {
        if (!item)
            continue;

        ItemTemplate const* proto = item->GetTemplate();
        if (!proto || proto->GetClass() != ITEM_CLASS_CONSUMABLE || proto->GetSubClass() != ITEM_SUBCLASS_CONSUMABLE)
            continue;

        // Check if this is a drink item by checking its spells
    for (auto const* effect : proto->Effects)
        {
            if (!effect || effect->SpellID <= 0)
                continue;

            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(effect->SpellID, DIFFICULTY_NONE);
            if (!spellInfo)
                continue;

            // Check if spell has a mana restoration effect
    for (SpellEffectInfo const& spellEffect : spellInfo->GetEffects())
            {
                if (spellEffect.ApplyAuraName == SPELL_AURA_PERIODIC_MANA_LEECH ||
                    spellEffect.ApplyAuraName == SPELL_AURA_OBS_MOD_POWER)
                {
                    return UseConsumable(item->GetEntry());
                }
            }
        }
    }

    return false;
}

// ============================================================================
// ITEM QUERIES
// ============================================================================

::std::vector<Item*> InventoryManager::GetAllItems() const
{
    if (_inventoryItems.empty())
        const_cast<InventoryManager*>(this)->UpdateInventoryCache();

    return _inventoryItems;
}

::std::vector<Item*> InventoryManager::GetItemsByQuality(uint32 quality) const
{
    ::std::vector<Item*> items;

    for (Item* item : GetAllItems())
    {
        if (item && item->GetTemplate()->GetQuality() == quality)
            items.push_back(item);
    }

    return items;
}

Item* InventoryManager::GetItemById(uint32 itemId) const
{
    for (Item* item : GetAllItems())
    {
        if (item && item->GetEntry() == itemId)
            return item;
    }

    return nullptr;
}

Item* InventoryManager::GetEquippedItem(uint8 slot) const
{
    auto it = _equippedItems.find(slot);
    if (it != _equippedItems.end())
        return it->second;
    return _bot ? _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot) : nullptr;
}

bool InventoryManager::IsItemEquipped(Item* item) const
{
    if (!item)
        return false;

    return item->IsEquipped();
}
uint32 InventoryManager::GetItemCount(uint32 itemId, bool includeBank) const
{
    if (!_bot)
        return 0;

    return _bot->GetItemCount(itemId, includeBank);
}

// ============================================================================
// INTERNAL METHODS
// ============================================================================

void InventoryManager::InitializeStatWeights()
{
    if (!_bot)
        return;

    // Clear existing weights
    _statWeights.clear();

    // Set weights based on class
    switch (_bot->GetClass())
    {
        case CLASS_WARRIOR:
        case CLASS_DEATH_KNIGHT:
            _statWeights[ITEM_MOD_STRENGTH] = 2.0f;
            _statWeights[ITEM_MOD_STAMINA] = 1.5f;
            _statWeights[ITEM_MOD_CRIT_RATING] = 1.0f;
            _statWeights[ITEM_MOD_HIT_RATING] = 1.2f;
            break;

        case CLASS_PALADIN:
            // Paladins use mixed stats depending on spec
            // Since GetPrimaryTalentTree is not available, use generic weights
            _statWeights[ITEM_MOD_STRENGTH] = 1.5f;
            _statWeights[ITEM_MOD_INTELLECT] = 1.5f;
            _statWeights[ITEM_MOD_STAMINA] = 1.5f;
            _statWeights[ITEM_MOD_CRIT_RATING] = 1.0f;
            break;

        case CLASS_HUNTER:
        case CLASS_ROGUE:
            _statWeights[ITEM_MOD_AGILITY] = 2.0f;
            _statWeights[ITEM_MOD_STAMINA] = 1.0f;
            _statWeights[ITEM_MOD_CRIT_RATING] = 1.5f;
            _statWeights[ITEM_MOD_HIT_RATING] = 1.2f;
            break;

        case CLASS_PRIEST:
        case CLASS_MAGE:
        case CLASS_WARLOCK:
            _statWeights[ITEM_MOD_INTELLECT] = 2.0f;
            _statWeights[ITEM_MOD_SPELL_POWER] = 1.8f;
            _statWeights[ITEM_MOD_STAMINA] = 0.8f;
            _statWeights[ITEM_MOD_CRIT_RATING] = 1.2f;
            break;

        case CLASS_SHAMAN:
        case CLASS_DRUID:
            // Hybrid classes - depends on spec
            _statWeights[ITEM_MOD_INTELLECT] = 1.5f;
            _statWeights[ITEM_MOD_AGILITY] = 1.5f;
            _statWeights[ITEM_MOD_STRENGTH] = 1.0f;
            _statWeights[ITEM_MOD_STAMINA] = 1.2f;
            break;

        default:
            // Generic weights
            _statWeights[ITEM_MOD_STAMINA] = 1.0f;
            break;
    }
}

void InventoryManager::UpdateEquipmentCache()
{
    // No lock needed - inventory data is per-bot instance data

    _equippedItems.clear();

    if (!_bot)
        return;

    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        Item* item = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (item)
            _equippedItems[slot] = item;
    }
}

void InventoryManager::UpdateInventoryCache()
{
    // No lock needed - inventory data is per-bot instance data

    _inventoryItems.clear();
    _itemCounts.clear();
    if (!_bot)
        return;
    // Main backpack
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
    {
        Item* item = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (item)
        {
            _inventoryItems.push_back(item);
            _itemCounts[item->GetEntry()] += item->GetCount();
        }
    }

    // Additional bags
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
    {
        Bag* pBag = _bot->GetBagByPos(bag);
        if (!pBag)
            continue;

        for (uint32 slot = 0; slot < pBag->GetBagSize(); ++slot)
        {
            Item* item = pBag->GetItemByPos(slot);
            if (item)
            {
                _inventoryItems.push_back(item);
                _itemCounts[item->GetEntry()] += item->GetCount();
            }
        }
    }
}

void InventoryManager::InvalidateCaches()
{
    // No lock needed - inventory data is per-bot instance data

    _itemScoreCache.Clear();
    _itemUsableCache.Clear();
    _equippedItems.clear();
    _itemCounts.clear();
    _inventoryItems.clear();
    _lootedObjects.clear();
}
::std::vector<ObjectGuid> InventoryManager::FindLootableObjects(float range) const
{
    ::std::vector<ObjectGuid> lootables;

    if (!_bot)
        return lootables;

    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Map* map = _bot->GetMap();
    if (!map)
        return {};

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return {};
    }

    // Query nearby creatures (lock-free!)
    ::std::vector<ObjectGuid> creatureGuids = spatialGrid->QueryNearbyCreatureGuids(
        _bot->GetPosition(), range);

    for (ObjectGuid guid : creatureGuids)
    {
        // PHASE 5D: Thread-safe spatial grid validation
        auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, guid);
        Creature* creature = nullptr;

        if (snapshot)
        {
            // Get Creature* for lootability check (validated via snapshot first)
            creature = ObjectAccessor::GetCreature(*_bot, guid);
        }

        if (creature && creature->GetHealth() == 0 && creature->HasDynamicFlag(UNIT_DYNFLAG_LOOTABLE))
        {
            lootables.push_back(creature->GetGUID());
        }
    }

    // Query nearby game objects (lock-free!)
    ::std::vector<ObjectGuid> goGuids = spatialGrid->QueryNearbyGameObjectGuids(
        _bot->GetPosition(), range);

    for (ObjectGuid guid : goGuids)
    {
        GameObject* go = map->GetGameObject(guid);
        if (go && go->GetGoType() == GAMEOBJECT_TYPE_CHEST)
        {
            lootables.push_back(go->GetGUID());
        }
    }

    return lootables;
}

float InventoryManager::CalculateStatValue(ItemTemplate const* proto) const
{
    if (!proto)
        return 0;

    float value = 0;

    // Calculate stat value based on weights
    for (uint8 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
    {
        int32 statType = proto->GetStatModifierBonusStat(i);
        int32 statValue = proto->GetStatPercentEditor(i);

        if (statType > 0 && statValue != 0)
        {
            auto it = _statWeights.find(statType);
            if (it != _statWeights.end())
            {
                value += statValue * it->second;
            }
        }
    }

    return value;
}

bool InventoryManager::CanUseItem(ItemTemplate const* proto) const
{
    if (!proto || !_bot)
        return false;

    // Check level requirement
    if (proto->GetBaseRequiredLevel() > static_cast<int32>(_bot->GetLevel()))
        return false;

    // Check class requirement
    int32 allowableClass = proto->GetAllowableClass();
    if (allowableClass && !(allowableClass & _bot->GetClassMask()))
        return false;

    // Check race requirement
    if (!proto->GetAllowableRace().IsEmpty() && !proto->GetAllowableRace().HasRace(_bot->GetRace()))
        return false;

    // Check skill requirement
    if (proto->GetRequiredSkill())
    {
        if (!_bot->HasSkill(proto->GetRequiredSkill()))
            return false;

        if (proto->GetRequiredSkillRank() > _bot->GetSkillValue(proto->GetRequiredSkill()))
            return false;
    }

    // Check reputation requirement
    if (proto->GetRequiredReputationFaction() && proto->GetRequiredReputationRank())
    {
        if (uint32(_bot->GetReputationRank(proto->GetRequiredReputationFaction())) < proto->GetRequiredReputationRank())
            return false;
    }

    return true;
}

void InventoryManager::LogAction(::std::string const& action, Item* item) const
{
    if (!item)
    {
        TC_LOG_DEBUG("module.playerbot", "InventoryManager: {}", action);
    }
    else
    {
        TC_LOG_DEBUG("module.playerbot", "InventoryManager: {} - {} ({})",
                  action, item->GetTemplate()->GetName(DEFAULT_LOCALE), item->GetEntry());
    }
}

void InventoryManager::UpdateMetrics(::std::chrono::steady_clock::time_point startTime)
{
    auto endTime = ::std::chrono::steady_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime);

    // Update average
    if (_metrics.averageUpdateTime.count() == 0)
    {
        _metrics.averageUpdateTime = duration;
    }
    else
    {
        _metrics.averageUpdateTime = (_metrics.averageUpdateTime * 9 + duration) / 10;
    }

    // Update max
    if (duration > _metrics.maxUpdateTime)
    {
        _metrics.maxUpdateTime = duration;
    }

    // Calculate cache hit rate
    uint32 cacheRequests = _itemScoreCache.Size() + _itemUsableCache.Size();
    if (cacheRequests > 0)
    {
        _metrics.cacheHitRate = 0.8f; // Placeholder - would need actual hit tracking
    }
}

} // namespace Playerbot