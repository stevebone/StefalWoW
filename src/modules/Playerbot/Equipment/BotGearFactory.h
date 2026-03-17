/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * BOT GEAR FACTORY - Automated Gear Generation System
 *
 * Purpose: Generate complete gear sets for bots during instant level-up
 * Uses immutable cache for lock-free, high-performance item selection
 *
 * INTEGRATION: Uses EquipmentManager for stat weight calculations
 * - GetStatPriorityByClassSpec() provides class/spec stat weights
 * - NO duplicate stat scoring logic - delegates to EquipmentManager
 * - Ensures consistency between gear generation and inventory management
 */

#pragma once

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "SharedDefines.h"
#include "Player.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>

namespace Playerbot
{

// Forward declarations
class EquipmentManager;

/**
 * @brief Statistics for gear factory performance monitoring
 */
struct GearFactoryStats
{
    ::std::atomic<uint64> setsGenerated{0};
    ::std::atomic<uint64> itemsSelected{0};
    ::std::atomic<uint64> itemsApplied{0};
    ::std::atomic<uint64> cacheLookups{0};
    ::std::atomic<uint64> qualityRolls{0};
    ::std::atomic<uint64> cacheSize{0};
};

/**
 * @brief Cached item data for gear selection
 */
struct CachedItem
{
    uint32 itemId{0};
    uint32 itemEntry{0};       // Database entry ID (same as itemId for most cases)
    uint32 itemLevel{0};
    uint8 inventoryType{0};
    uint8 quality{0};
    uint8 itemClass{0};
    uint8 itemSubClass{0};
    uint8 armorType{0};        // Armor subclass for armor items
    uint32 requiredLevel{0};
    int32 statScore{0};  // Pre-computed score for class/spec
    uint32 allowableClass{0};  // Class mask
    uint32 allowableRace{0};   // Race mask

    bool operator<(CachedItem const& other) const { return statScore > other.statScore; }  // Higher score first
};

/**
 * @brief Quality distribution configuration for level ranges
 */
struct QualityDistribution
{
    uint32 minLevel{0};
    uint32 maxLevel{0};
    uint8 commonWeight{0};      // White items
    uint8 uncommonWeight{0};    // Green items
    uint8 rareWeight{0};        // Blue items
    uint8 epicWeight{0};        // Purple items
    // Percentage-based distribution (alternative to weights)
    float greenPercent{0.0f};   // Uncommon item percentage
    float bluePercent{0.0f};    // Rare item percentage
    float purplePercent{0.0f};  // Epic item percentage

    uint8 GetRandomQuality() const;  // Select quality based on weights
};

/**
 * @brief Complete gear set generated for a bot
 */
struct GearSet
{
    ::std::map<uint8, uint32> equipment;      // slot -> itemId
    ::std::map<uint8, uint32> items;          // slot -> itemId (equipment slots)
    ::std::vector<uint32> bags;               // bag item IDs
    ::std::map<uint32, uint32> consumables;   // itemId -> count
    uint8 classId{0};
    uint32 specId{0};
    uint32 level{0};
    uint32 setLevel{0};                       // Level the gear set was generated for
    TeamId faction{TEAM_NEUTRAL};
    float averageIlvl{0.0f};                  // Average item level

    bool IsComplete() const { return !equipment.empty() && equipment.size() >= 10; }
    uint32 GetItemCount() const { return static_cast<uint32>(items.size()); }
};

/**
 * @brief Immutable gear cache for lock-free item generation
 *
 * Cache Structure:
 * - class -> spec -> level -> slot -> [CachedItem]
 * - Built once at startup from item_template database
 * - Never modified after initialization (lock-free reads)
 * - Pre-scored items for each class/spec combination
 */
class TC_GAME_API BotGearFactory final
{
public:
    static BotGearFactory* instance();

    // IBotGearFactory interface implementation

    /**
     * Initialize the gear factory and build immutable cache
     * Called once at server startup
     */
    void Initialize();

    /**
     * Check if factory is ready to generate gear
     */
    bool IsReady() const { return _cacheReady.load(::std::memory_order_acquire); }

    /**
     * Generate complete gear set for bot
     * Thread-safe (lock-free cache reads)
     *
     * @param cls       Class ID (1-13)
     * @param specId    Specialization ID (0-3)
     * @param level     Character level (1-80)
     * @param faction   Faction for faction-specific items
     * @return Complete gear set with items, bags, consumables
     */
    GearSet BuildGearSet(uint8 cls, uint32 specId, uint32 level, TeamId faction);

    /**
     * Apply gear set to player (create items and equip)
     * Must be called from main thread (uses Player API)
     *
     * @param player    Player object to equip
     * @param gearSet   Pre-generated gear set
     * @return true if successfully equipped
     */
    bool ApplyGearSet(Player* player, GearSet const& gearSet);

    /**
     * Get statistics for monitoring performance
     * Fills the provided stats struct with current values
     */
    void GetStats(GearFactoryStats& stats) const
    {
        stats.setsGenerated.store(_stats.setsGenerated.load(::std::memory_order_relaxed), ::std::memory_order_relaxed);
        stats.itemsSelected.store(_stats.itemsSelected.load(::std::memory_order_relaxed), ::std::memory_order_relaxed);
        stats.cacheLookups.store(_stats.cacheLookups.load(::std::memory_order_relaxed), ::std::memory_order_relaxed);
        stats.qualityRolls.store(_stats.qualityRolls.load(::std::memory_order_relaxed), ::std::memory_order_relaxed);
        stats.cacheSize.store(_stats.cacheSize.load(::std::memory_order_relaxed), ::std::memory_order_relaxed);
    }

    /**
     * Get item level for character level (mapping)
     * L1 -> ilvl 5, L80 -> ilvl 593
     */
    uint32 GetItemLevelForCharLevel(uint32 charLevel);

    /**
     * Get appropriate bag item entries for level range
     */
    ::std::vector<uint32> GetBagItemsForLevel(uint32 level);

    /**
     * Get class-appropriate consumables
     */
    ::std::map<uint32, uint32> GetConsumablesForClass(uint8 cls, uint32 level);

private:
    BotGearFactory();
    ~BotGearFactory() = default;
    BotGearFactory(BotGearFactory const&) = delete;
    BotGearFactory& operator=(BotGearFactory const&) = delete;

    /**
     * Build immutable item cache from database
     */
    void BuildGearCache();

    /**
     * Initialize quality distribution rules
     */
    void InitializeQualityDistributions();

    /**
     * Query item_template for eligible items
     */
    void LoadItemsFromDatabase();

    /**
     * Pre-compute stat scores for all cached items
     * INTEGRATION: Calls EquipmentManager::GetStatPriorityByClassSpec()
     * Uses the SAME stat weights as inventory management (zero duplication)
     */
    void PrecomputeItemScores();

    /**
     * Select quality tier based on level and slot
     */
    uint32 SelectQuality(uint32 level, uint8 slot);

    /**
     * Get quality distribution for level range
     */
    QualityDistribution const* GetQualityDistribution(uint32 level) const;

    /**
     * Select best item from candidates
     */
    uint32 SelectBestItem(uint8 cls, uint32 specId, uint32 level, uint8 slot, uint32 targetQuality);

    /**
     * Get cached items for specific parameters
     */
    ::std::vector<CachedItem> const* GetItemsForSlot(uint8 cls, uint32 specId, uint32 level, uint8 slot) const;

    /**
     * Convert equipment slot to inventory type(s)
     * Some slots map to multiple inventory types (e.g., MAINHAND can be WEAPON, 2HWEAPON, etc.)
     */
    ::std::vector<uint8> GetInventoryTypesForSlot(uint8 slot) const;

    /**
     * Filter items by quality
     */
    ::std::vector<CachedItem> FilterByQuality(::std::vector<CachedItem> const& items, uint32 quality) const;

    /**
     * Get allowed armor types for class
     */
    ::std::vector<uint8> GetAllowedArmorTypes(uint8 cls) const;

    /**
     * Get allowed weapon subtypes for class
     * Returns vector of ITEM_SUBCLASS_WEAPON_* values
     */
    ::std::vector<uint8> GetAllowedWeaponTypes(uint8 cls) const;

    /**
     * Verify item is appropriate for class/spec
     */
    bool IsItemAppropriate(CachedItem const& item, uint8 cls, uint32 specId, uint32 level) const;

    // Cache: class -> spec -> level -> slot -> [items]
    using SlotCache = ::std::unordered_map<uint8, ::std::vector<CachedItem>>;
    using LevelCache = ::std::unordered_map<uint32, SlotCache>;
    using SpecCache = ::std::unordered_map<uint32, LevelCache>;
    using ClassCache = ::std::unordered_map<uint8, SpecCache>;

    ClassCache _gearCache;
    ::std::vector<QualityDistribution> _qualityDistributions;
    ::std::vector<CachedItem> _rawItems;  // Temporary storage for items before organizing into cache
    ::std::atomic<bool> _cacheReady{false};
    mutable GearFactoryStats _stats;
    Playerbot::OrderedMutex<Playerbot::LockOrder::BEHAVIOR_MANAGER> _initMutex;

    // Configuration (loaded from playerbots.conf)
    bool _enabled{true};                   // Playerbot.GearFactory.Enable
    uint32 _minItemLevel{5};               // Internal: minimum item level
    uint32 _minQuality{2};                 // Playerbot.GearFactory.QualityMin (Uncommon)
    uint32 _maxQuality{4};                 // Playerbot.GearFactory.QualityMax (Epic)
    uint32 _levelRange{5};                 // Playerbot.GearFactory.LevelRange
    bool _useStatWeighting{true};          // Internal: use stat weights
    bool _useSpecAppropriate{true};        // Playerbot.GearFactory.UseSpecAppropriate
    bool _enchantItems{true};              // Playerbot.GearFactory.EnchantItems
    bool _gemItems{true};                  // Playerbot.GearFactory.GemItems
    uint32 _refreshInterval{60};           // Playerbot.GearFactory.RefreshInterval (minutes)
};

#define sBotGearFactory BotGearFactory::instance()

} // namespace Playerbot
