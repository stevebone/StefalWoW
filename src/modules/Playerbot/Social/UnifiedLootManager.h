/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "LootDistribution.h"
#include "Player.h"
#include "Group.h"
#include "Item.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <atomic>
#include "GameTime.h"

namespace Playerbot
{

/**
 * @brief Unified loot management system
 *
 * This system provides a unified interface for loot management operations.
 * Note: LootAnalysis and LootCoordination were stub interfaces with no implementations
 * and have been removed during consolidation. Real loot logic is in LootDistribution.
 *
 * **Architecture:**
 * ```
 * UnifiedLootManager
 *   ├─> AnalysisModule     (TODO: Implement item scoring, upgrade detection)
 *   ├─> CoordinationModule (TODO: Implement session management, orchestration)
 *   └─> DistributionModule (delegates to LootDistribution for now)
 * ```
 *
 * **Thread Safety:**
 * - Uses OrderedMutex<LOOT_MANAGER> for all operations
 * - Modules share data through thread-safe interfaces
 * - Lock ordering prevents deadlocks
 *
 * **Status:**
 * - AnalysisModule and CoordinationModule need real implementations
 * - DistributionModule delegates to existing LootDistribution system
 * - Ready for future feature development
 */
class TC_GAME_API UnifiedLootManager final 
{
public:
    static UnifiedLootManager* instance();

    // Singleton management
    UnifiedLootManager();
    ~UnifiedLootManager();

    // Non-copyable, non-movable
    UnifiedLootManager(UnifiedLootManager const&) = delete;
    UnifiedLootManager& operator=(UnifiedLootManager const&) = delete;
    UnifiedLootManager(UnifiedLootManager&&) = delete;
    UnifiedLootManager& operator=(UnifiedLootManager&&) = delete;

    // ========================================================================
    // ANALYSIS MODULE INTERFACE
    // ========================================================================

    float CalculateItemValue(Player* player, LootItem const& item);
    float CalculateUpgradeValue(Player* player, LootItem const& item);
    bool IsSignificantUpgrade(Player* player, LootItem const& item);
    float CalculateStatWeight(Player* player, uint32 statType);
    float CompareItems(Player* player, LootItem const& newItem, Item const* currentItem);
    float CalculateItemScore(Player* player, LootItem const& item);
    std::vector<std::pair<uint32, float>> GetStatPriorities(Player* player);

    // ========================================================================
    // COORDINATION MODULE INTERFACE
    // ========================================================================

    void InitiateLootSession(Group* group, Loot* loot);
    void ProcessLootSession(Group* group, uint32 lootSessionId);
    void CompleteLootSession(uint32 lootSessionId);
    void HandleLootSessionTimeout(uint32 lootSessionId);
    void OrchestrateLootDistribution(Group* group, std::vector<LootItem> const& items);
    void PrioritizeLootDistribution(Group* group, std::vector<LootItem>& items);
    void OptimizeLootSequence(Group* group, std::vector<LootItem>& items);
    void FacilitateGroupLootDiscussion(Group* group, LootItem const& item);
    void HandleLootConflictResolution(Group* group, LootItem const& item);
    void BroadcastLootRecommendations(Group* group, LootItem const& item);
    void OptimizeLootEfficiency(Group* group);
    void MinimizeLootTime(Group* group, uint32 sessionId);
    void MaximizeLootFairness(Group* group, uint32 sessionId);

    // ========================================================================
    // DISTRIBUTION MODULE INTERFACE
    // ========================================================================

    void DistributeLoot(Group* group, LootItem const& item);
    void HandleLootRoll(Player* player, uint32 rollId, LootRollType rollType);
    LootRollType DetermineLootDecision(Player* player, LootItem const& item,
                                        LootDecisionStrategy strategy);
    LootPriority CalculateLootPriority(Player* player, LootItem const& item);
    bool ShouldRollNeed(Player* player, LootItem const& item);
    bool ShouldRollGreed(Player* player, LootItem const& item);
    bool IsItemForClass(Player* player, LootItem const& item);
    bool IsItemForMainSpec(Player* player, LootItem const& item);
    bool IsItemForOffSpec(Player* player, LootItem const& item);
    void ExecuteLootDistribution(Group* group, uint32 rollId);
    void ResolveRollTies(Group* group, uint32 rollId);
    void HandleLootNinja(Group* group, uint32 suspectedPlayer);

    // ========================================================================
    // UNIFIED OPERATIONS
    // ========================================================================

    void ProcessCompleteLootFlow(Group* group, Loot* loot);
    std::string GetLootRecommendation(Player* player, LootItem const& item);
    std::string GetLootStatistics() const;

private:
    // ========================================================================
    // INTERNAL MODULES
    // ========================================================================

    /**
     * @brief Analysis module - item evaluation and scoring
     */
    class AnalysisModule
    {
    public:
        float CalculateItemValue(Player* player, LootItem const& item);
        float CalculateUpgradeValue(Player* player, LootItem const& item);
        bool IsSignificantUpgrade(Player* player, LootItem const& item);
        float CalculateStatWeight(Player* player, uint32 statType);
        float CompareItems(Player* player, LootItem const& newItem, Item const* currentItem);
        float CalculateItemScore(Player* player, LootItem const& item);
        std::vector<std::pair<uint32, float>> GetStatPriorities(Player* player);

    private:
        float CalculateArmorValue(Player* player, LootItem const& item);
        float CalculateWeaponValue(Player* player, LootItem const& item);
        float CalculateAccessoryValue(Player* player, LootItem const& item);
        bool IsItemForMainSpec(Player* player, LootItem const& item);
        bool IsItemForOffSpec(Player* player, LootItem const& item);

        // Statistics
        std::atomic<uint64> _itemsAnalyzed{0};
        std::atomic<uint64> _upgradesDetected{0};

    public:
        // Getters for statistics (used by GetLootStatistics)
        uint64 GetItemsAnalyzed() const { return _itemsAnalyzed.load(); }
        uint64 GetUpgradesDetected() const { return _upgradesDetected.load(); }
    };

    /**
     * @brief Coordination module - session management and orchestration
     */
    class CoordinationModule
    {
    public:
        void InitiateLootSession(Group* group, Loot* loot);
        void ProcessLootSession(Group* group, uint32 lootSessionId);
        void CompleteLootSession(uint32 lootSessionId);
        void HandleLootSessionTimeout(uint32 lootSessionId);
        void OrchestrateLootDistribution(Group* group, std::vector<LootItem> const& items);
        void PrioritizeLootDistribution(Group* group, std::vector<LootItem>& items);
        void OptimizeLootSequence(Group* group, std::vector<LootItem>& items);
        void FacilitateGroupLootDiscussion(Group* group, LootItem const& item);
        void HandleLootConflictResolution(Group* group, LootItem const& item);
        void BroadcastLootRecommendations(Group* group, LootItem const& item);
        void OptimizeLootEfficiency(Group* group);
        void MinimizeLootTime(Group* group, uint32 sessionId);
        void MaximizeLootFairness(Group* group, uint32 sessionId);

    private:
        struct LootSession
        {
            uint32 sessionId;
            uint32 groupId;
            std::vector<LootItem> availableItems;
            std::vector<uint32> activeRolls;
            uint32 sessionStartTime;
            uint32 sessionTimeout;
            bool isActive;

            // Default constructor for map usage
            LootSession()
                : sessionId(0), groupId(0)
                , sessionStartTime(0)
                , sessionTimeout(0)
                , isActive(false)
            {}

            LootSession(uint32 id, uint32 gId)
                : sessionId(id), groupId(gId)
                , sessionStartTime(GameTime::GetGameTimeMS())
                , sessionTimeout(GameTime::GetGameTimeMS() + 300000) // 5 minutes
                , isActive(true)
            {}
        };

        std::unordered_map<uint32, LootSession> _activeSessions;
        uint32 _nextSessionId{1};
        Playerbot::OrderedMutex<Playerbot::LockOrder::LOOT_MANAGER> _sessionMutex;

        // Efficiency settings for optimized loot processing
        struct EfficiencySettings
        {
            uint32 optimalBatchSize{5};      // Optimal number of items to process per batch
            bool canUseFastPath{false};       // True if all members are bots (no human approval needed)
            uint32 lastOptimizationTime{0};   // GameTime when last optimized
            uint32 rollTimeoutMs{15000};      // Timeout for roll decisions (15s default)
            float targetItemsPerSecond{2.0f}; // Target throughput for loot distribution
        };
        EfficiencySettings _efficiencySettings;

        // Fairness tracking for session-based fairness maximization
        struct FairnessTracker
        {
            std::unordered_map<ObjectGuid, uint32> itemsWonThisSession;
            std::unordered_map<ObjectGuid, float> totalUpgradeValueReceived;
            uint32 sessionStartTime{0};
            bool isActive{false};

            void Reset()
            {
                itemsWonThisSession.clear();
                totalUpgradeValueReceived.clear();
                sessionStartTime = 0;
                isActive = false;
            }
        };
        std::unordered_map<uint32, FairnessTracker> _sessionFairness;

        // Statistics
        std::atomic<uint64> _sessionsCreated{0};
        std::atomic<uint64> _sessionsCompleted{0};

    public:
        // Getters for statistics (used by GetLootStatistics)
        uint64 GetSessionsCreated() const { return _sessionsCreated.load(); }
        uint64 GetSessionsCompleted() const { return _sessionsCompleted.load(); }
        uint32 GetActiveSessionCount() const { return static_cast<uint32>(_activeSessions.size()); }
    };

    /**
     * @brief Distribution module - roll handling and execution
     */
    class DistributionModule
    {
    public:
        void DistributeLoot(Group* group, LootItem const& item);
        void HandleLootRoll(Player* player, uint32 rollId, LootRollType rollType);
        LootRollType DetermineLootDecision(Player* player, LootItem const& item,
                                            LootDecisionStrategy strategy);
        LootPriority CalculateLootPriority(Player* player, LootItem const& item);
        bool ShouldRollNeed(Player* player, LootItem const& item);
        bool ShouldRollGreed(Player* player, LootItem const& item);
        bool IsItemForClass(Player* player, LootItem const& item);
        bool IsItemForMainSpec(Player* player, LootItem const& item);
        bool IsItemForOffSpec(Player* player, LootItem const& item);
        void ExecuteLootDistribution(Group* group, uint32 rollId);
        void ResolveRollTies(Group* group, uint32 rollId);
        void HandleLootNinja(Group* group, uint32 suspectedPlayer);

    private:
        // Helper methods for group loot coordination
        void HandleMasterLoot(Group* group, LootItem const& item);
        void HandleGroupLoot(Group* group, LootItem const& item);

        struct BotRollEvaluation {
            Player* bot;
            LootRollType rollType;
            uint32 rollValue;
            float upgradeValue;
            LootPriority priority;
        };

        Player* DetermineGroupLootWinner(std::vector<BotRollEvaluation>& rolls);

        struct LootRoll
        {
            uint32 rollId;
            uint32 itemId;
            uint32 lootSlot;
            uint32 groupId;
            uint32 winnerGuid{0};  // GUID of the player who won the roll
            std::unordered_map<uint32, LootRollType> playerRolls;
            std::unordered_map<uint32, uint32> rollValues;
            bool isComplete;

            // Default constructor for map usage
            LootRoll()
                : rollId(0), itemId(0), lootSlot(0), groupId(0), winnerGuid(0), isComplete(false)
            {}

            LootRoll(uint32 id) : rollId(id), itemId(0), lootSlot(0), groupId(0), winnerGuid(0), isComplete(false) {}
        };

        /**
         * @brief Awards an item to a player using TrinityCore's inventory system
         * @param player The player to award the item to
         * @param itemId The item entry ID to award
         * @param count Number of items to award (default 1)
         * @return True if item was successfully awarded
         */
        bool AwardItemToPlayer(Player* player, uint32 itemId, uint32 count = 1);

        std::unordered_map<uint32, LootRoll> _activeRolls;
        uint32 _nextRollId{1};
        Playerbot::OrderedMutex<Playerbot::LockOrder::LOOT_MANAGER> _rollMutex;

        // Statistics
        std::atomic<uint64> _rollsProcessed{0};
        std::atomic<uint64> _itemsDistributed{0};
    };

    // Module instances
    std::unique_ptr<AnalysisModule> _analysis;
    std::unique_ptr<CoordinationModule> _coordination;
    std::unique_ptr<DistributionModule> _distribution;

    // Global mutex for unified operations
    mutable Playerbot::OrderedMutex<Playerbot::LockOrder::LOOT_MANAGER> _mutex;

    // Statistics
    std::atomic<uint64> _totalOperations{0};
    std::atomic<uint64> _totalProcessingTimeMs{0};
};

} // namespace Playerbot
