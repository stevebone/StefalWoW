/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * COMPLETE EQUIPMENT MANAGEMENT SYSTEM
 *
 * This system provides comprehensive equipment analysis, comparison, and
 * auto-equip functionality for all 13 WoW classes across all specializations.
 *
 * Key Features:
 * - Class/spec stat priority evaluation
 * - Item level + stat weight comparison
 * - Auto-equip better gear
 * - Junk item identification
 * - Set bonus tracking
 * - Weapon DPS comparison
 */

#pragma once

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "Player.h"
#include "Item.h"
#include "ItemTemplate.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>

namespace Playerbot
{

/**
 * @brief Stat types for item comparison based on WoW 12.0 itemMods
 */
enum class StatType : uint8
{
    STRENGTH = 0,
    AGILITY = 1,
    STAMINA = 2,
    INTELLECT = 3,
    SPIRIT = 4,
    CRITICAL_STRIKE = 5,
    HASTE = 6,
    VERSATILITY = 7,
    MASTERY = 8,
    ARMOR = 9,
    WEAPON_DPS = 10,
    ITEM_LEVEL = 11
};

/**
 * @brief Equipment quality levels
 */
enum class ItemQualityLevel : uint8
{
    POOR = 0,       // Grey - Junk, always sell
    COMMON = 1,     // White - Low value
    UNCOMMON = 2,   // Green - Quest rewards
    RARE = 3,       // Blue - Dungeon drops
    EPIC = 4,       // Purple - Raid/High-end
    LEGENDARY = 5,  // Orange - Legendary items
    ARTIFACT = 6,   // Gold - Artifact weapons
    HEIRLOOM = 7    // Blue with gold border
};

/**
 * @brief Item categories for organization
 */
enum class ItemCategory : uint8
{
    WEAPON,
    ARMOR,
    CONSUMABLE,
    TRADE_GOODS,
    QUEST_ITEM,
    JUNK,
    VALUABLE_BIND_ON_EQUIP,
    UNKNOWN
};

/**
 * @brief Stat priority configuration for class/spec combinations
 */
struct StatPriority
{
    uint8 classId;
    uint8 specId;
    std::vector<std::pair<StatType, float>> statWeights; // stat -> weight (0.0-1.0)

    StatPriority() : classId(0), specId(0) {}
    StatPriority(uint8 cls, uint8 spec) : classId(cls), specId(spec) {}

    float GetStatWeight(StatType stat) const
    {
        for (const auto& [statType, weight] : statWeights)
        {
            if (statType == stat)
                return weight;
        }
        return 0.0f;
    }
};

/**
 * @brief Item comparison result
 */
struct ItemComparisonResult
{
    bool isUpgrade = false;
    float scoreDifference = 0.0f;
    float currentItemScore = 0.0f;
    float newItemScore = 0.0f;
    uint32 currentItemLevel = 0;
    uint32 newItemLevel = 0;
    std::string upgradeReason;

    ItemComparisonResult() = default;
};

/**
 * @brief Complete equipment manager for all bot equipment operations
 *
 * **Phase 6.1: Singleton → Per-Bot Instance Pattern**
 *
 * DESIGN PRINCIPLE: Per-bot instance owned by GameSystemsManager
 * - Each bot has its own EquipmentManager instance
 * - No mutex locking (per-bot isolation)
 * - Direct member access (no map lookups)
 * - Integrates with stat priority system via facade
 * - Coordinates auto-equip, junk detection, and consumable management
 *
 * **Ownership:**
 * - Owned by GameSystemsManager via std::unique_ptr
 * - Constructed per-bot with Player* reference
 * - Destroyed with bot cleanup
 *
 * Implements IEquipmentManager for dependency injection compatibility.
 */
/**
 * @brief Equipment operation metrics for monitoring
 */
struct EquipmentMetrics
{
    uint32 itemsEvaluated{0};
    uint32 itemsEquipped{0};
    uint32 upgradesFound{0};
    uint32 junkIdentified{0};
    uint64 goldFromVendorSales{0};
    uint64 totalGoldFromJunk{0};

    EquipmentMetrics() = default;
};

class TC_GAME_API EquipmentManager final
{
public:
    /**
     * @brief Construct equipment manager for bot
     * @param bot The bot player this manager serves
     */
    explicit EquipmentManager(Player* bot);

    /**
     * @brief Destructor - cleanup per-bot resources
     */
    ~EquipmentManager();

    // ============================================================================
    // IEquipmentManager interface implementation
    // ============================================================================

    /**
     * Scan all inventory items and auto-equip better gear
     * This is the main entry point called by TradeAutomation
     */
    void AutoEquipBestGear();

    /**
     * Compare two items for the same equipment slot
     * Returns true if newItem is better than currentItem for this bot
     */
    ItemComparisonResult CompareItems(::Item* currentItem, ::Item* newItem);

    /**
     * Calculate item score based on class/spec stat priorities
     */
    float CalculateItemScore(::Item* item);

    /**
     * Determine if item is an upgrade for any equipment slot
     */
    bool IsItemUpgrade(::Item* item);

    /**
     * Calculate item score for an ItemTemplate (quest rewards, vendor items, etc.)
     * Uses class/spec stat priorities to evaluate items before they exist
     * @param itemTemplate Item template to evaluate
     * @return Weighted score based on stat priorities
     */
    float CalculateItemTemplateScore(ItemTemplate const* itemTemplate);

    // ============================================================================
    // JUNK IDENTIFICATION
    // ============================================================================

    /**
     * Identify all junk items in bot's inventory
     * Returns item GUIDs that should be sold to vendor
     */
    std::vector<ObjectGuid> IdentifyJunkItems();

    /**
     * Check if specific item is junk (grey quality, low ilvl, wrong stats)
     */
    bool IsJunkItem(::Item* item);

    /**
     * Check if item should NEVER be sold (quest items, valuables, set items)
     */
    bool IsProtectedItem(::Item* item);

    /**
     * Evaluate if BoE item is valuable enough for AH vs vendor selling
     */
    bool IsValuableBoE(::Item* item);

    // ============================================================================
    // CONSUMABLE MANAGEMENT
    // ============================================================================

    /**
     * Get list of consumables this bot needs to restock
     * Returns itemId -> quantity needed
     */
    std::unordered_map<uint32, uint32> GetConsumableNeeds();

    /**
     * Check if bot has sufficient consumables for their class
     */
    bool NeedsConsumableRestocking();

    /**
     * Get class-specific consumable requirements (food, potions, reagents)
     */
    std::vector<uint32> GetClassConsumables(uint8 classId);

    /**
     * Check current consumable quantities
     */
    uint32 GetConsumableCount(uint32 itemId);

    // ============================================================================
    // STAT PRIORITY SYSTEM
    // ============================================================================

    /**
     * Get stat priority configuration for bot's current class/spec
     */
    StatPriority const& GetStatPriority();

    /**
     * Get stat priority configuration by class/spec ID directly
     * Used by BotGearFactory for cache building without Player objects
     */
    static StatPriority const& GetStatPriorityByClassSpec(uint8 classId, uint32 specId);

    /**
     * Update stat priorities when bot changes spec
     */
    void UpdateStatPriority();

    // ============================================================================
    // ITEM CATEGORIZATION
    // ============================================================================

    /**
     * Determine item category for organization and selling decisions
     */
    ItemCategory GetItemCategory(::Item* item);

    /**
     * Check if item can be equipped by this bot (class/level restrictions)
     */
    bool CanEquipItem(ItemTemplate const* itemTemplate);

    /**
     * Get equipment slot for this item (EQUIPMENT_SLOT_HEAD, etc.)
     * NOTE: For multi-slot items (rings, trinkets), returns the first slot.
     */
    uint8 GetItemEquipmentSlot(ItemTemplate const* itemTemplate);

    /**
     * Get all valid equipment slots for this item type
     * Handles multi-slot items (rings: FINGER1/FINGER2, trinkets: TRINKET1/TRINKET2)
     */
    std::vector<uint8> GetMultiSlotEquipmentSlots(ItemTemplate const* itemTemplate);

    /**
     * Get the worst (lowest score) equipped item from a set of slots
     * Returns nullptr if any slot is empty (indicating empty slot available)
     */
    ::Item* GetWorstEquippedItemForSlots(std::vector<uint8> const& slots);

    // ============================================================================
    // ADVANCED FEATURES
    // ============================================================================

    /**
     * Check if item is part of a set and evaluate set bonuses
     */
    bool IsSetItem(::Item* item);

    /**
     * Count equipped set pieces for set bonus calculation
     */
    uint32 GetEquippedSetPieceCount(uint32 setId);

    /**
     * Evaluate weapon DPS (important for melee classes)
     */
    float GetWeaponDPS(::Item* item);

    /**
     * Get total armor value from item
     */
    uint32 GetItemArmor(::Item* item);

    /**
     * Extract stat value from item (Strength, Agility, etc.)
     */
    int32 GetItemStatValue(::Item* item, StatType stat);

    // ============================================================================
    // AUTOMATION CONTROL
    // ============================================================================

    struct EquipmentAutomationProfile
    {
        bool autoEquipEnabled = true;
        bool autoSellJunkEnabled = true;
        bool considerSetBonuses = true;
        bool preferHigherItemLevel = true;
        float minUpgradeThreshold = 5.0f; // Minimum % improvement to equip
        uint32 minItemLevelToKeep = 1;    // Sell items below this ilvl
        bool keepValuableBoE = true;
        std::unordered_set<uint32> neverSellItems; // Item IDs to always keep

        EquipmentAutomationProfile() = default;
    };

    void SetAutomationProfile(EquipmentAutomationProfile const& profile);
    EquipmentAutomationProfile const& GetAutomationProfile() const;

    // ============================================================================
    // METRICS AND MONITORING
    // ============================================================================

    EquipmentMetrics const& GetMetrics();
    EquipmentMetrics const& GetGlobalMetrics();

private:
    // Non-copyable
    EquipmentManager(EquipmentManager const&) = delete;
    EquipmentManager& operator=(EquipmentManager const&) = delete;

    // ============================================================================
    // PER-BOT INSTANCE DATA
    // ============================================================================

    Player* _bot;                               // Bot reference (non-owning)
    EquipmentAutomationProfile _profile;        // Automation profile for this bot
    EquipmentMetrics _metrics;                  // Metrics for this bot

    // ============================================================================
    // SHARED STATIC DATA
    // ============================================================================

    // Stat priority database (classId + specId -> StatPriority)
    // Shared across all bots, initialized once
    static std::unordered_map<uint16, StatPriority> _statPriorities; // key = (classId << 8) | specId
    static bool _statPrioritiesInitialized;

    // Global metrics across all bots
    static EquipmentMetrics _globalMetrics;

    // ============================================================================
    // STAT PRIORITY INITIALIZATION (ALL 13 CLASSES)
    // ============================================================================

    static void InitializeStatPriorities();  // Load shared stat priorities once
    static void InitializeWarriorPriorities();
    static void InitializePaladinPriorities();
    static void InitializeHunterPriorities();
    static void InitializeRoguePriorities();
    static void InitializePriestPriorities();
    static void InitializeShamanPriorities();
    static void InitializeMagePriorities();
    static void InitializeWarlockPriorities();
    static void InitializeDruidPriorities();
    static void InitializeDeathKnightPriorities();
    static void InitializeMonkPriorities();
    static void InitializeDemonHunterPriorities();
    static void InitializeEvokerPriorities();

    // ============================================================================
    // HELPER METHODS
    // ============================================================================

    // Item score calculation helpers
    float CalculateWeaponScore(::Item* weapon);
    float CalculateArmorScore(::Item* armor);
    float CalculateAccessoryScore(::Item* accessory);

    // Item analysis helpers
    bool IsOutdatedGear(::Item* item);
    bool HasWrongPrimaryStats(::Item* item);
    bool IsLowerQualityThanEquipped(::Item* item);

    // Equipment slot helpers
    ::Item* GetEquippedItemInSlot(uint8 slot);
    bool CanEquipInSlot(::Item* item, uint8 slot);
    void EquipItemInSlot(::Item* item, uint8 slot);

    // Consumable helpers
    uint32 GetRecommendedFoodLevel();
    uint32 GetRecommendedPotionLevel();
    std::vector<uint32> GetClassReagents(uint8 classId);

    // Stat extraction from ItemTemplate (TrinityCore 12.0 API)
    static int32 ExtractStatValue(ItemTemplate const* proto, StatType stat);
    static float CalculateTotalStats(ItemTemplate const* proto, std::vector<std::pair<StatType, float>> const& weights);

    // Metrics updates
    void UpdateMetrics(bool wasEquipped, bool wasUpgrade, uint32 goldValue = 0);

    // Utility
    static uint16 MakeStatPriorityKey(uint8 classId, uint8 specId) { return (static_cast<uint16>(classId) << 8) | specId; }
    void LogEquipmentDecision(std::string const& action, std::string const& reason);
};

} // namespace Playerbot
