/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_LOOT_DISTRIBUTION_H
#define PLAYERBOT_LOOT_DISTRIBUTION_H

#pragma once

// DEBUG: If you see this error twice, the header is being included multiple times despite guards
#ifdef PLAYERBOT_LOOT_DISTRIBUTION_INCLUDED_MARKER
#error "LootDistribution.h is being included multiple times - this should not happen with header guards!"
#endif
#define PLAYERBOT_LOOT_DISTRIBUTION_INCLUDED_MARKER

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "Player.h"
#include "Group.h"
#include "Item.h"
#include "Loot.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>
#include <atomic>
#include <mutex>
#include <chrono>
#include "GameTime.h"

class Player;
class Group;
class Item;
struct ItemTemplate;
struct Loot;

namespace Playerbot
{

enum class LootRollType : uint8
{
    NEED       = 0,
    GREED      = 1,
    PASS       = 2,
    DISENCHANT = 3
};

enum class LootDecisionStrategy : uint8
{
    NEED_BEFORE_GREED    = 0,  // Standard need/greed priority
    CLASS_PRIORITY       = 1,  // Prioritize class-appropriate items
    UPGRADE_PRIORITY     = 2,  // Prioritize actual upgrades
    FAIR_DISTRIBUTION    = 3,  // Ensure fair loot distribution
    RANDOM_ROLLS         = 4,  // Random decision making
    VENDOR_VALUE         = 5,  // Focus on vendor value
    MAINSPEC_PRIORITY    = 6,  // Main spec takes priority over off-spec
    CONSERVATIVE         = 7   // Pass on questionable items
};

enum class LootPriority : uint8
{
    CRITICAL_UPGRADE     = 0,  // Major upgrade for main spec
    SIGNIFICANT_UPGRADE  = 1,  // Notable improvement
    MINOR_UPGRADE        = 2,  // Small improvement
    SIDEGRADE           = 3,  // Similar power level
    OFF_SPEC_UPGRADE    = 4,  // Good for off-spec
    VENDOR_ITEM         = 5,  // Only valuable for vendor
    NOT_USEFUL          = 6   // No use for this player
};

struct LootItem
{
    uint32 itemId;
    uint32 itemCount;
    uint32 lootSlot;
    const ItemTemplate* itemTemplate;
    uint32 itemLevel;
    uint32 itemQuality;
    uint32 vendorValue;
    uint8 inventoryType;           // Equipment slot type (InventoryType enum)
    bool isClassRestricted;
    std::vector<uint32> allowedClasses;
    std::vector<uint32> allowedSpecs;
    bool isBoundOnPickup;
    bool isBoundOnEquip;
    std::string itemName;

    LootItem() : itemId(0), itemCount(0), lootSlot(0), itemTemplate(nullptr)
        , itemLevel(0), itemQuality(0), vendorValue(0), inventoryType(0), isClassRestricted(false)
        , isBoundOnPickup(false), isBoundOnEquip(false) {}

    LootItem(uint32 id, uint32 count, uint32 slot) : itemId(id), itemCount(count)
        , lootSlot(slot), itemTemplate(nullptr), itemLevel(0), itemQuality(0)
        , vendorValue(0), inventoryType(0), isClassRestricted(false), isBoundOnPickup(false)
        , isBoundOnEquip(false) {}
};

struct LootRoll
{
    uint32 rollId;
    uint32 itemId;
    uint32 lootSlot;
    uint32 groupId;
    std::unordered_map<uint32, LootRollType> playerRolls; // playerGuid -> roll type
    std::unordered_map<uint32, uint32> rollValues; // playerGuid -> roll value (1-100)
    std::unordered_set<uint32> eligiblePlayers;
    uint32 rollStartTime;
    uint32 rollTimeout;
    bool isCompleted;
    uint32 winnerGuid;
    LootRollType winningRollType;

    // Default constructor
    LootRoll() : rollId(0), itemId(0), lootSlot(0), groupId(0), rollStartTime(0),
        rollTimeout(0), isCompleted(false), winnerGuid(0), winningRollType(LootRollType::PASS) {}

    // Parametrized constructor
    LootRoll(uint32 id, uint32 item, uint32 slot, uint32 group) : rollId(id), itemId(item)
        , lootSlot(slot), groupId(group), rollStartTime(GameTime::GetGameTimeMS())
        , rollTimeout(GameTime::GetGameTimeMS() + 60000), isCompleted(false), winnerGuid(0)
        , winningRollType(LootRollType::PASS) {}

    // Copy constructor
    LootRoll(const LootRoll& other) = default;

    // Move constructor
    LootRoll(LootRoll&& other) = default;

    // Copy assignment operator
    LootRoll& operator=(const LootRoll& other) = default;

    // Move assignment operator
    LootRoll& operator=(LootRoll&& other) = default;
};

struct PlayerLootProfile
{
    uint32 playerGuid;
    uint8 playerClass;
    uint8 playerSpec;
    uint32 playerLevel;
    LootDecisionStrategy strategy;
    std::unordered_map<uint32, LootPriority> itemPriorities; // itemId -> priority
    std::unordered_set<uint32> neededItemTypes; // item subtypes needed
    std::unordered_set<uint32> wantedItemTypes; // item subtypes wanted
    std::unordered_set<uint32> blacklistedItems; // items to always pass
    float greedThreshold; // minimum item value to greed (0.0-1.0)
    bool needMainSpecOnly;
    bool greedOffSpec;
    bool disenchantUnneeded;
    uint32 lastLootTime;
    uint32 totalLootReceived;

    // Default constructor
    PlayerLootProfile() : playerGuid(0), playerClass(CLASS_WARRIOR), playerSpec(0), playerLevel(1)
        , strategy(LootDecisionStrategy::NEED_BEFORE_GREED), greedThreshold(0.3f)
        , needMainSpecOnly(true), greedOffSpec(true), disenchantUnneeded(false)
        , lastLootTime(0), totalLootReceived(0) {}

    // Parameterized constructor
    PlayerLootProfile(uint32 guid, uint8 cls, uint8 spec) : playerGuid(guid)
        , playerClass(cls), playerSpec(spec), playerLevel(1)
        , strategy(LootDecisionStrategy::NEED_BEFORE_GREED), greedThreshold(0.3f)
        , needMainSpecOnly(true), greedOffSpec(true), disenchantUnneeded(false)
        , lastLootTime(0), totalLootReceived(0) {}

    // Copy constructor
    PlayerLootProfile(const PlayerLootProfile& other) = default;

    // Move constructor
    PlayerLootProfile(PlayerLootProfile&& other) = default;

    // Copy assignment operator
    PlayerLootProfile& operator=(const PlayerLootProfile& other) = default;

    // Move assignment operator
    PlayerLootProfile& operator=(PlayerLootProfile&& other) = default;
};

// Loot fairness tracker - defined at namespace level for interface compatibility
struct LootFairnessTracker
{
    std::unordered_map<uint32, uint32> playerLootCount; // playerGuid -> items received
    std::unordered_map<uint32, uint32> playerLootValue; // playerGuid -> total value received
    std::unordered_map<uint32, uint32> playerNeedRolls; // playerGuid -> need rolls won
    std::unordered_map<uint32, uint32> playerGreedRolls; // playerGuid -> greed rolls won
    uint32 totalItemsDistributed;
    uint32 totalValueDistributed;
    float fairnessScore; // 0.0 = unfair, 1.0 = perfectly fair

    LootFairnessTracker() : totalItemsDistributed(0), totalValueDistributed(0), fairnessScore(1.0f) {}
};

// Loot metrics - defined at namespace level for interface compatibility
struct LootMetrics
{
    std::atomic<uint32> totalRollsInitiated{0};
    std::atomic<uint32> totalRollsCompleted{0};
    std::atomic<uint32> needRollsWon{0};
    std::atomic<uint32> greedRollsWon{0};
    std::atomic<uint32> itemsPassed{0};
    std::atomic<uint32> rollTimeouts{0};
    std::atomic<float> averageRollTime{30000.0f}; // 30 seconds
    std::atomic<float> decisionAccuracy{0.9f};
    std::atomic<float> playerSatisfaction{0.8f};
    std::chrono::steady_clock::time_point lastUpdate;

    LootMetrics() = default;

    LootMetrics(const LootMetrics& other) :
        totalRollsInitiated(other.totalRollsInitiated.load()),
        totalRollsCompleted(other.totalRollsCompleted.load()),
        needRollsWon(other.needRollsWon.load()),
        greedRollsWon(other.greedRollsWon.load()),
        itemsPassed(other.itemsPassed.load()),
        rollTimeouts(other.rollTimeouts.load()),
        averageRollTime(other.averageRollTime.load()),
        decisionAccuracy(other.decisionAccuracy.load()),
        playerSatisfaction(other.playerSatisfaction.load()),
        lastUpdate(other.lastUpdate) {}

    LootMetrics& operator=(const LootMetrics& other) {
        if (this != &other) {
            totalRollsInitiated.store(other.totalRollsInitiated.load());
            totalRollsCompleted.store(other.totalRollsCompleted.load());
            needRollsWon.store(other.needRollsWon.load());
            greedRollsWon.store(other.greedRollsWon.load());
            itemsPassed.store(other.itemsPassed.load());
            rollTimeouts.store(other.rollTimeouts.load());
            averageRollTime.store(other.averageRollTime.load());
            decisionAccuracy.store(other.decisionAccuracy.load());
            playerSatisfaction.store(other.playerSatisfaction.load());
            lastUpdate = other.lastUpdate;
        }
        return *this;
    }

    void Reset()
    {
        totalRollsInitiated = 0; totalRollsCompleted = 0; needRollsWon = 0;
        greedRollsWon = 0; itemsPassed = 0; rollTimeouts = 0;
        averageRollTime = 30000.0f; decisionAccuracy = 0.9f; playerSatisfaction = 0.8f;
        lastUpdate = std::chrono::steady_clock::now();
    }
};

class TC_GAME_API LootDistribution final 
{
public:
    explicit LootDistribution(Player* bot);
    ~LootDistribution();
    LootDistribution(LootDistribution const&) = delete;
    LootDistribution& operator=(LootDistribution const&) = delete;

    // Core loot distribution functionality
    void HandleGroupLoot(Group* group, Loot* loot);
    void InitiateLootRoll(Group* group, const LootItem& item);
    void ProcessPlayerLootDecision(uint32 rollId, LootRollType rollType);
    void CompleteLootRoll(uint32 rollId);

    // Loot analysis and decision making
    LootRollType DetermineLootDecision(const LootItem& item);
    LootPriority AnalyzeItemPriority(const LootItem& item);
    bool IsItemUpgrade(const LootItem& item);
    bool IsClassAppropriate(const LootItem& item);

    // Need/Greed/Pass logic implementation
    bool CanPlayerNeedItem(const LootItem& item);
    bool ShouldPlayerGreedItem(const LootItem& item);
    bool ShouldPlayerPassItem(const LootItem& item);
    bool CanPlayerDisenchantItem(const LootItem& item);
    bool IsItemForMainSpec(const LootItem& item);
    bool IsItemUsefulForOffSpec(const LootItem& item);
    bool IsItemForOffSpec(const LootItem& item) { return IsItemUsefulForOffSpec(item); }

    // Roll processing and winner determination
    void ProcessLootRolls(uint32 rollId);
    uint32 DetermineRollWinner(const LootRoll& roll);
    void DistributeLootToWinner(uint32 rollId, uint32 winnerGuid);
    void HandleLootRollTimeout(uint32 rollId);

    // Loot distribution strategies
    void ExecuteNeedBeforeGreedStrategy(const LootItem& item, LootRollType& decision);
    void ExecuteClassPriorityStrategy(const LootItem& item, LootRollType& decision);
    void ExecuteUpgradePriorityStrategy(const LootItem& item, LootRollType& decision);
    void ExecuteFairDistributionStrategy(const LootItem& item, LootRollType& decision);
    void ExecuteMainSpecPriorityStrategy(const LootItem& item, LootRollType& decision);

    // Group loot settings and policies
    void SetGroupLootMethod(Group* group, LootMethod method);
    void SetGroupLootThreshold(Group* group, ItemQualities threshold);
    void SetMasterLooter(Group* group, Player* masterLooter);
    void HandleMasterLootDistribution(Group* group, const LootItem& item, Player* recipient);

    // Loot fairness and distribution tracking (struct defined at namespace level above)
    LootFairnessTracker GetGroupLootFairness(uint32 groupId);
    void UpdateLootFairness(uint32 groupId, uint32 winnerGuid, const LootItem& item);
    float CalculateFairnessScore(const LootFairnessTracker& tracker);

    // Performance monitoring (struct defined at namespace level above)
    LootMetrics GetPlayerLootMetrics();
    LootMetrics GetGroupLootMetrics(uint32 groupId);
    LootMetrics GetGlobalLootMetrics();

    // Advanced loot features
    void HandleReservedItems(Group* group, const std::vector<uint32>& reservedItems, Player* reserver);
    void ProcessLootCouncilDecision(Group* group, const LootItem& item, Player* recipient);
    void HandlePersonalLoot(const LootItem& item);
    void ManageLootHistory(Group* group, const LootItem& item, Player* recipient);

    // Loot prediction and optimization
    void PredictLootNeeds(Group* group);
    void OptimizeLootDistribution(Group* group);
    void RecommendLootSettings(Group* group);
    void AnalyzeGroupLootComposition(Group* group);

    // Player preferences and configuration
    void SetPlayerLootStrategy(LootDecisionStrategy strategy);
    LootDecisionStrategy GetPlayerLootStrategy();
    void SetPlayerLootPreferences(const PlayerLootProfile& profile);
    PlayerLootProfile GetPlayerLootProfile();

    // Error handling and edge cases
    void HandleLootConflicts(uint32 rollId);
    void HandleInvalidLootRoll(uint32 rollId);
    void HandlePlayerDisconnectDuringRoll(uint32 rollId);
    void RecoverFromLootSystemError(uint32 rollId);

    // Update and maintenance
    void Update(uint32 diff);
    void ProcessActiveLootRolls();
    void CleanupExpiredRolls();
    void ValidateLootStates();

private:
    Player* _bot;

    // Core data structures
    std::unordered_map<uint32, LootRoll> _activeLootRolls; // rollId -> roll data
    std::unordered_map<uint32, PlayerLootProfile> _playerLootProfiles; // playerGuid -> profile
    std::unordered_map<uint32, LootFairnessTracker> _groupFairnessTracking; // groupId -> fairness
    std::unordered_map<uint32, LootMetrics> _playerMetrics; // playerGuid -> metrics
    

    // Roll management
    std::atomic<uint32> _nextRollId{1};
    std::queue<uint32> _completedRolls;
    std::unordered_map<uint32, uint32> _rollTimeouts; // rollId -> timeout time

    // Loot analysis cache
    std::unordered_map<uint32, std::unordered_map<uint32, LootPriority>> _itemPriorityCache; // playerGuid -> itemId -> priority
    std::unordered_map<uint32, std::unordered_map<uint32, bool>> _upgradeCache; // playerGuid -> itemId -> isUpgrade
    

    // Performance tracking
    LootMetrics _globalMetrics;

    // Helper functions
    void InitializePlayerLootProfile();
    void AnalyzeItemForPlayer(const LootItem& item);
    void UpdateItemPriorityCache(const LootItem& item, LootPriority priority);
    bool IsItemCachedUpgrade(uint32 itemId);
    void InvalidatePlayerCache();

    // Item analysis functions
    void PopulateLootItemData(LootItem& item);
    bool ShouldInitiateRoll(Group* group, const LootItem& item);
    void HandleAutoLoot(Group* group, const LootItem& item);
    bool CanParticipateInRoll(const LootItem& item);
    float CalculateUpgradeValue(const LootItem& item);
    bool IsItemTypeUsefulForClass(uint8 playerClass, const ItemTemplate* itemTemplate);
    bool IsWeaponUsableByClass(uint8 playerClass, uint8 weaponSubClass) const;
    bool IsArmorUpgrade(const LootItem& item);
    bool IsWeaponUpgrade(const LootItem& item);
    bool IsAccessoryUpgrade(const LootItem& item);
    float CalculateItemScore(const LootItem& item);
    float CalculateItemScore(Item* item);
    float CalculateStatPriority(uint32 statType);

    // Roll processing helpers
    void BroadcastLootRoll(Group* group, const LootRoll& roll);
    void NotifyRollResult(const LootRoll& roll);
    void HandleRollCompletion(LootRoll& roll);
    uint32 ProcessNeedRolls(LootRoll& roll);
    uint32 ProcessGreedRolls(LootRoll& roll);
    uint32 ProcessDisenchantRolls(LootRoll& roll);

    // Fairness and distribution algorithms
    void UpdateGroupLootHistory(uint32 groupId, uint32 winnerGuid, const LootItem& item);
    void BalanceLootDistribution(Group* group);
    void AdjustLootDecisionsForFairness(Group* group, LootRollType& decision);
    bool ShouldConsiderFairnessAdjustment(Group* group);

    // Strategy implementations
    LootRollType ExecuteStrategy(const LootItem& item, LootDecisionStrategy strategy);
    void ApplyStrategyModifiers(const LootItem& item, LootRollType& decision);
    void ConsiderGroupComposition(Group* group, const LootItem& item, LootRollType& decision);

    // Performance optimization
    void OptimizeLootProcessing();
    void PreloadItemData(const std::vector<LootItem>& items);
    void CachePlayerEquipment();
    void UpdateLootMetrics(const LootRoll& roll, bool wasWinner);

    // Constants
    static constexpr uint32 LOOT_ROLL_TIMEOUT = 60000; // 60 seconds
    static constexpr uint32 LOOT_UPDATE_INTERVAL = 1000; // 1 second
    static constexpr uint32 MAX_ACTIVE_ROLLS = 100;
    static constexpr float UPGRADE_THRESHOLD = 0.05f; // 5% improvement minimum
    static constexpr float GREED_VALUE_THRESHOLD = 0.3f; // 30% of max possible value
    static constexpr uint32 LOOT_HISTORY_SIZE = 100; // Remember last 100 items
    static constexpr float FAIRNESS_ADJUSTMENT_THRESHOLD = 0.7f; // Adjust if fairness < 70%
    static constexpr uint32 CACHE_CLEANUP_INTERVAL = 300000; // 5 minutes
    static constexpr uint32 PRIORITY_CACHE_SIZE = 1000; // Cache 1000 item priorities per player
    static constexpr uint32 ROLL_CLEANUP_INTERVAL = 10000; // 10 seconds
};

} // namespace Playerbot

#endif // PLAYERBOT_LOOT_DISTRIBUTION_H