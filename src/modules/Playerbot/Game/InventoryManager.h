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

#ifndef TRINITY_BOT_INVENTORY_MANAGER_H
#define TRINITY_BOT_INVENTORY_MANAGER_H

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "ItemDefines.h"
#include "ObjectGuid.h"
#include <chrono>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <list>
#include <mutex>

class Player;
class Item;
class Creature;
class GameObject;
class Loot;
struct ItemTemplate;
struct ItemPosCount;

// Forward declare ItemPosCountVec from Player.h
using ItemPosCountVec = std::vector<ItemPosCount>;

namespace Playerbot
{

/**
 * @class InventoryManager
 * @brief Manages bot inventory operations including looting, equipment optimization, and bag organization
 *
 * This class provides comprehensive inventory management for bots including:
 * - Automatic looting from corpses and containers
 * - Equipment optimization and auto-equipping
 * - Bag organization and space management
 * - Vendor trash selling and item destruction
 * - Item quality filtering and preference management
 * - Performance-optimized caching and throttling
 *
 * Performance targets:
 * - <0.03% CPU per bot
 * - <200KB memory per bot
 * - Update throttling at 2 second intervals
 */
class TC_GAME_API InventoryManager
{
public:
    explicit InventoryManager(Player* bot);
    ~InventoryManager();

    // ========================================================================
    // MAIN UPDATE - Called from BotAI::UpdateStrategies()
    // ========================================================================

    /**
     * @brief Main update method for inventory operations
     * @param diff Time elapsed since last update in milliseconds
     *
     * This method is throttled to run at configured intervals (default 2000ms)
     * to prevent excessive CPU usage from constant inventory scans.
     */
    void Update(uint32 diff);

    // ========================================================================
    // LOOTING OPERATIONS
    // ========================================================================

    /**
     * @brief Automatically loot nearby corpses and containers
     * @param maxRange Maximum range to search for lootable objects (default 30 yards)
     * @return Number of objects successfully looted
     */
    uint32 AutoLoot(float maxRange = 30.0f);

    /**
     * @brief Loot a specific corpse
     * @param creature Creature to loot
     * @return true if successfully looted
     */
    bool LootCorpse(Creature* creature);

    /**
     * @brief Loot a specific game object (chest, herb, ore, etc.)
     * @param go GameObject to loot
     * @return true if successfully looted
     */
    bool LootGameObject(GameObject* go);

    /**
     * @brief Process loot from an object
     * @param loot Loot object to process
     * @return Number of items looted
     */
    uint32 ProcessLoot(Loot* loot);

    /**
     * @brief Check if an item should be looted based on quality/filters
     * @param itemId Item ID to check
     * @return true if item should be looted
     */
    bool ShouldLootItem(uint32 itemId) const;

    /**
     * @brief Set minimum item quality to loot
     * @param quality Minimum quality (0=grey, 1=white, 2=green, etc.)
     */
    void SetMinimumLootQuality(uint32 quality) { _minLootQuality = quality; }

    // ========================================================================
    // EQUIPMENT OPTIMIZATION
    // ========================================================================

    /**
     * @brief Scan bags for equipment upgrades and equip them
     * @return Number of items equipped
     */
    uint32 OptimizeEquipment();

    /**
     * @brief Check if an item is an upgrade over current equipment
     * @param item Item to evaluate
     * @return true if item is an upgrade
     */
    bool CanEquipUpgrade(Item* item) const;

    /**
     * @brief Compare two items for the same slot
     * @param item1 First item
     * @param item2 Second item
     * @return Score difference (positive means item1 is better)
     */
    float CompareItems(Item* item1, Item* item2) const;

    /**
     * @brief Calculate item score based on stats for bot's class/spec
     * @param item Item to score
     * @return Item score value
     */
    float CalculateItemScore(Item* item) const;

    /**
     * @brief Equip an item from inventory
     * @param item Item to equip
     * @return true if successfully equipped
     */
    bool EquipItem(Item* item);

    /**
     * @brief Unequip an item to inventory
     * @param slot Equipment slot to unequip
     * @return true if successfully unequipped
     */
    bool UnequipItem(uint8 slot);

    // ========================================================================
    // BAG MANAGEMENT
    // ========================================================================

    /**
     * @brief Organize bags by consolidating stacks and sorting items
     */
    void OrganizeBags();

    /**
     * @brief Get number of free bag slots
     * @return Number of empty slots across all bags
     */
    uint32 GetBagSpace() const;

    /**
     * @brief Get total bag capacity
     * @return Total number of bag slots
     */
    uint32 GetBagCapacity() const;

    /**
     * @brief Check if bags are full
     * @return true if no free slots available
     */
    bool IsBagsFull() const { return GetBagSpace() == 0; }

    /**
     * @brief Find best bag slot for an item
     * @param itemId Item ID to place
     * @param count Number of items
     * @param dest Output destination slots
     * @return EQUIP_ERR_OK if space found
     */
    InventoryResult FindBagSlot(uint32 itemId, uint32 count, ItemPosCountVec& dest);

    /**
     * @brief Consolidate stackable items
     * @return Number of slots freed
     */
    uint32 ConsolidateStacks();

    /**
     * @brief Sort items by type and quality
     */
    void SortBags();

    // ========================================================================
    // ITEM STORAGE
    // ========================================================================

    /**
     * @brief Store a new item in inventory
     * @param itemId Item ID to store
     * @param count Number of items
     * @return true if successfully stored
     */
    bool StoreNewItem(uint32 itemId, uint32 count);

    /**
     * @brief Store an existing item object
     * @param item Item to store
     * @return true if successfully stored
     */
    bool StoreItem(Item* item);

    /**
     * @brief Destroy an item
     * @param item Item to destroy
     * @param update Send update to client
     * @return true if successfully destroyed
     */
    bool DestroyItem(Item* item, bool update = false);

    /**
     * @brief Destroy items to make space
     * @param slots Number of slots to free
     * @return Number of slots actually freed
     */
    uint32 DestroyItemsForSpace(uint32 slots);

    // ========================================================================
    // VENDOR OPERATIONS
    // ========================================================================

    /**
     * @brief Sell all vendor trash (grey quality items)
     * @param vendor Vendor NPC (nullptr to just mark for selling)
     * @return Total gold earned from sales
     */
    uint32 SellVendorTrash(Creature* vendor = nullptr);

    /**
     * @brief Check if an item should be sold to vendor
     * @param item Item to check
     * @return true if item should be sold
     */
    bool ShouldSellItem(Item* item) const;

    /**
     * @brief Repair all equipment at vendor
     * @param vendor Repair vendor NPC
     * @return Repair cost in copper
     */
    uint32 RepairEquipment(Creature* vendor);

    /**
     * @brief Buy items from vendor
     * @param vendor Vendor NPC
     * @param itemId Item to buy
     * @param count Quantity to buy
     * @return true if successfully purchased
     */
    bool BuyFromVendor(Creature* vendor, uint32 itemId, uint32 count);

    // ========================================================================
    // CONSUMABLES MANAGEMENT
    // ========================================================================

    /**
     * @brief Use a consumable item
     * @param itemId Item ID to use
     * @return true if successfully used
     */
    bool UseConsumable(uint32 itemId);

    /**
     * @brief Get count of specific consumable
     * @param itemId Item ID to count
     * @return Number of items in inventory
     */
    uint32 GetConsumableCount(uint32 itemId) const;

    /**
     * @brief Check if bot needs food
     * @return true if health is low and food would help
     */
    bool NeedsFood() const;

    /**
     * @brief Check if bot needs drink
     * @return true if mana is low and drink would help
     */
    bool NeedsDrink() const;

    /**
     * @brief Use food if needed
     * @return true if food was consumed
     */
    bool UseFood();

    /**
     * @brief Use drink if needed
     * @return true if drink was consumed
     */
    bool UseDrink();

    // ========================================================================
    // ITEM QUERIES
    // ========================================================================

    /**
     * @brief Get all items in inventory
     * @return Vector of all items
     */
    ::std::vector<Item*> GetAllItems() const;

    /**
     * @brief Get items by quality
     * @param quality Item quality to filter
     * @return Vector of matching items
     */
    ::std::vector<Item*> GetItemsByQuality(uint32 quality) const;

    /**
     * @brief Get item by ID
     * @param itemId Item ID to find
     * @return First matching item or nullptr
     */
    Item* GetItemById(uint32 itemId) const;

    /**
     * @brief Get equipped item in slot
     * @param slot Equipment slot
     * @return Item or nullptr if empty
     */
    Item* GetEquippedItem(uint8 slot) const;

    /**
     * @brief Check if item is equipped
     * @param item Item to check
     * @return true if currently equipped
     */
    bool IsItemEquipped(Item* item) const;

    /**
     * @brief Get item count
     * @param itemId Item ID to count
     * @param includeBank Include bank items
     * @return Total count across all bags
     */
    uint32 GetItemCount(uint32 itemId, bool includeBank = false) const;

    // ========================================================================
    // PERFORMANCE METRICS
    // ========================================================================

    struct PerformanceMetrics
    {
        uint32 totalUpdates = 0;
        uint32 itemsLooted = 0;
        uint32 itemsEquipped = 0;
        uint32 itemsSold = 0;
        uint32 itemsDestroyed = 0;
        uint32 bagsOrganized = 0;
        ::std::chrono::microseconds averageUpdateTime{0};
        ::std::chrono::microseconds maxUpdateTime{0};
        float cacheHitRate = 0.0f;
    };

    PerformanceMetrics GetMetrics() const { return _metrics; }
    void ResetMetrics() { _metrics = {}; }

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /**
     * @brief Enable or disable auto-loot
     */
    void SetAutoLootEnabled(bool enabled) { _autoLootEnabled = enabled; }

    /**
     * @brief Enable or disable auto-equip
     */
    void SetAutoEquipEnabled(bool enabled) { _autoEquipEnabled = enabled; }

    /**
     * @brief Enable or disable auto-sell
     */
    void SetAutoSellEnabled(bool enabled) { _autoSellEnabled = enabled; }

    /**
     * @brief Set update interval in milliseconds
     */
    void SetUpdateInterval(uint32 intervalMs) { _updateInterval = intervalMs; }

    /**
     * @brief Add item to ignore list (won't loot/equip)
     */
    void AddIgnoredItem(uint32 itemId) { _ignoredItems.insert(itemId); }

    /**
     * @brief Remove item from ignore list
     */
    void RemoveIgnoredItem(uint32 itemId) { _ignoredItems.erase(itemId); }

private:
    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /**
     * @brief Initialize stat weights based on class/spec
     */
    void InitializeStatWeights();

    /**
     * @brief Update equipment cache
     */
    void UpdateEquipmentCache();

    /**
     * @brief Update inventory cache
     */
    void UpdateInventoryCache();

    /**
     * @brief Clear all caches
     */
    void InvalidateCaches();

    /**
     * @brief Find nearby lootable objects
     */
    ::std::vector<ObjectGuid> FindLootableObjects(float range) const;

    /**
     * @brief Calculate item stat value
     */
    float CalculateStatValue(ItemTemplate const* proto) const;

    /**
     * @brief Check if item meets level requirements
     */
    bool CanUseItem(ItemTemplate const* proto) const;

    /**
     * @brief Log inventory action for debugging
     */
    void LogAction(::std::string const& action, Item* item = nullptr) const;

    /**
     * @brief Update performance metrics
     */
    void UpdateMetrics(::std::chrono::steady_clock::time_point startTime);

    // ========================================================================
    // LRU CACHE IMPLEMENTATION
    // ========================================================================

    template<typename Key, typename Value>
    class LRUCache
    {
    public:
        explicit LRUCache(size_t capacity) : _capacity(capacity) {}

        bool Get(Key const& key, Value& value) const
        {
            auto it = _cache.find(key);
            if (it == _cache.end())
                return false;

            // Move to front (most recently used)
            _lru.erase(it->second.second);
            _lru.push_front(key);
            it->second.second = _lru.begin();
            value = it->second.first;
            return true;
        }

        void Put(Key const& key, Value const& value)
        {
            auto it = _cache.find(key);
            if (it != _cache.end())
            {
                // Update existing
                _lru.erase(it->second.second);
                _lru.push_front(key);
                it->second = {value, _lru.begin()};
            }
            else
            {
                // Add new
                if (_cache.size() >= _capacity)
                {
                    // Evict least recently used
                    _cache.erase(_lru.back());
                    _lru.pop_back();
                }
                _lru.push_front(key);
                _cache[key] = {value, _lru.begin()};
            }
        }

        void Clear()
        {
            _cache.clear();
            _lru.clear();
        }

        size_t Size() const { return _cache.size(); }

    private:
        size_t _capacity;
        mutable ::std::list<Key> _lru;
        mutable ::std::unordered_map<Key, ::std::pair<Value, typename ::std::list<Key>::iterator>> _cache;
    };

    // ========================================================================
    // MEMBER VARIABLES
    // ========================================================================

    Player* _bot;

    // Configuration
    bool _autoLootEnabled = true;
    bool _autoEquipEnabled = true;
    bool _autoSellEnabled = true;
    uint32 _updateInterval = 2000;  // milliseconds
    uint32 _minLootQuality = 0;     // 0=grey, 1=white, 2=green, etc.
    uint32 _minFreeSlots = 5;       // Maintain this many free slots

    // Timing
    uint32 _lastUpdateTime = 0;
    uint32 _lastEquipScan = 0;
    uint32 _lastBagOrganize = 0;
    uint32 _lastLootScan = 0;

    // Caches
    LRUCache<uint32, float> _itemScoreCache{256};           // ItemId -> Score
    LRUCache<uint32, bool> _itemUsableCache{256};          // ItemId -> Usable
    ::std::unordered_map<uint8, Item*> _equippedItems;       // Slot -> Item
    ::std::unordered_map<uint32, uint32> _itemCounts;        // ItemId -> Count
    ::std::vector<Item*> _inventoryItems;                    // All bag items

    // State
    ::std::unordered_set<uint32> _ignoredItems;              // Items to never loot/equip
    ::std::unordered_set<ObjectGuid> _lootedObjects;         // Recently looted to avoid re-checking
    uint32 _lastCacheUpdate = 0;

    // Stat weights for item scoring (class-specific)
    ::std::unordered_map<uint32, float> _statWeights;

    // Performance metrics
    mutable PerformanceMetrics _metrics;

    // Thread safety
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BEHAVIOR_MANAGER> _mutex;
};

} // namespace Playerbot

#endif // TRINITY_BOT_INVENTORY_MANAGER_H