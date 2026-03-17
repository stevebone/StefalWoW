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
#include "Player.h"
#include "QuestDef.h"
#include "Creature.h"
#include "Position.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>
#include <atomic>
#include <mutex>
#include <chrono>

class Player;
class Quest;
class Creature;
class Group;

namespace Playerbot
{

enum class TurnInStrategy : uint8
{
    IMMEDIATE_TURNIN        = 0,  // Turn in quests as soon as completed
    BATCH_TURNIN           = 1,  // Collect multiple completed quests, turn in together
    OPTIMAL_ROUTING        = 2,  // Plan efficient route for multiple turn-ins
    GROUP_COORDINATION     = 3,  // Coordinate turn-ins with group members
    REWARD_OPTIMIZATION    = 4,  // Optimize reward selection and timing
    CHAIN_CONTINUATION     = 5   // Prioritize quest chain progression
};

enum class RewardSelectionStrategy : uint8
{
    HIGHEST_VALUE          = 0,  // Select most valuable reward
    BEST_UPGRADE          = 1,  // Select best gear upgrade
    VENDOR_VALUE          = 2,  // Select highest vendor sell value
    STAT_PRIORITY         = 3,  // Select based on stat priorities
    CLASS_APPROPRIATE     = 4,  // Select class-appropriate items
    RANDOM_SELECTION      = 5,  // Random selection (roleplay)
    MANUAL_SELECTION      = 6   // Wait for manual selection
};

struct QuestRewardItem
{
    uint32 itemId;
    uint32 itemCount;
    float itemValue;
    float upgradeValue;
    float vendorValue;
    bool isClassAppropriate;
    std::vector<uint32> itemStats;
    std::string description;

    QuestRewardItem(uint32 id, uint32 count) : itemId(id), itemCount(count)
        , itemValue(0.0f), upgradeValue(0.0f), vendorValue(0.0f)
        , isClassAppropriate(false) {}
};

struct QuestTurnInData
{
    uint32 questId;
    uint32 botGuid;
    uint32 questGiverGuid;
    Position questGiverLocation;
    bool isCompleted;
    bool requiresTravel;
    uint32 estimatedTravelTime;
    std::vector<QuestRewardItem> availableRewards;
    uint32 selectedRewardIndex;
    RewardSelectionStrategy rewardStrategy;
    uint32 turnInPriority;
    uint32 scheduledTurnInTime;
    std::string turnInReason;

    QuestTurnInData(uint32 qId, uint32 bGuid, uint32 gGuid) : questId(qId), botGuid(bGuid)
        , questGiverGuid(gGuid), isCompleted(false), requiresTravel(true)
        , estimatedTravelTime(0), selectedRewardIndex(0)
        , rewardStrategy(RewardSelectionStrategy::BEST_UPGRADE), turnInPriority(100)
        , scheduledTurnInTime(0) {}
};

struct TurnInBatch
{
    uint32 botGuid;
    std::vector<uint32> questIds;
    std::vector<uint32> questGiverGuids;
    Position centralLocation;
    uint32 totalTravelTime;
    uint32 batchPriority;
    uint32 scheduledTime;
    bool isOptimized;

    TurnInBatch() : botGuid(0), totalTravelTime(0)
        , batchPriority(100), scheduledTime(0), isOptimized(false) {}

    TurnInBatch(uint32 bGuid) : botGuid(bGuid), totalTravelTime(0)
        , batchPriority(100), scheduledTime(0), isOptimized(false) {}
};

class TC_GAME_API QuestTurnIn final 
{
public:
    explicit QuestTurnIn(Player* bot);
    ~QuestTurnIn();
    QuestTurnIn(QuestTurnIn const&) = delete;
    QuestTurnIn& operator=(QuestTurnIn const&) = delete;

    // Core turn-in functionality
    bool TurnInQuest(uint32 questId, Player* bot);
    void ProcessQuestTurnIn(Player* bot, uint32 questId);
    void ProcessBatchTurnIn(Player* bot, const TurnInBatch& batch);
    void ScheduleQuestTurnIn(Player* bot, uint32 questId, uint32 delayMs = 0);

    // Quest completion detection
    std::vector<uint32> GetCompletedQuests(Player* bot);
    bool IsQuestReadyForTurnIn(uint32 questId, Player* bot);
    void MonitorQuestCompletion(Player* bot);
    void HandleQuestCompletion(Player* bot, uint32 questId);

    // Turn-in planning and optimization
    void PlanOptimalTurnInRoute(Player* bot);
    TurnInBatch CreateTurnInBatch(Player* bot, const std::vector<uint32>& questIds);
    void OptimizeTurnInSequence(Player* bot, std::vector<QuestTurnInData>& turnIns);
    void MinimizeTurnInTravel(Player* bot);

    // Quest giver location and navigation
    bool FindQuestTurnInNpc(Player* bot, uint32 questId);
    Position GetQuestTurnInLocation(uint32 questId);
    bool NavigateToQuestGiver(Player* bot, uint32 questGiverGuid);
    bool IsAtQuestGiver(Player* bot, uint32 questGiverGuid);

    // Reward selection and optimization
    void AnalyzeQuestRewards(QuestTurnInData& turnInData, Player* bot);
    uint32 SelectOptimalReward(const std::vector<QuestRewardItem>& rewards, Player* bot, RewardSelectionStrategy strategy);
    void EvaluateItemUpgrades(const std::vector<QuestRewardItem>& rewards, Player* bot);
    float CalculateItemValue(const QuestRewardItem& reward, Player* bot);

    // Group turn-in coordination
    void CoordinateGroupTurnIns(Group* group);
    void SynchronizeGroupRewardSelection(Group* group, uint32 questId);
    void HandleGroupTurnInConflicts(Group* group, uint32 questId);
    void ShareTurnInProgress(Group* group);

    // Turn-in dialog and interaction
    void HandleQuestGiverDialog(Player* bot, uint32 questGiverGuid, uint32 questId);
    void SelectQuestReward(Player* bot, uint32 questId, uint32 rewardIndex);
    void ConfirmQuestTurnIn(Player* bot, uint32 questId);
    void HandleTurnInDialog(Player* bot, uint32 questId);

    // Advanced turn-in strategies
    void ExecuteImmediateTurnInStrategy(Player* bot);
    void ExecuteBatchTurnInStrategy(Player* bot);
    void ExecuteOptimalRoutingStrategy(Player* bot);
    void ExecuteGroupCoordinationStrategy(Player* bot);
    void ExecuteRewardOptimizationStrategy(Player* bot);
    void ExecuteChainContinuationStrategy(Player* bot);

    // Turn-in performance monitoring
    struct TurnInMetrics
    {
        std::atomic<uint32> questsTurnedIn{0};
        std::atomic<uint32> turnInAttempts{0};
        std::atomic<uint32> successfulTurnIns{0};
        std::atomic<uint32> failedTurnIns{0};
        std::atomic<float> averageTurnInTime{15000.0f}; // 15 seconds
        std::atomic<float> turnInSuccessRate{0.95f};
        std::atomic<uint32> totalTravelDistance{0};
        std::atomic<uint32> rewardsSelected{0};
        std::atomic<float> rewardSelectionAccuracy{0.85f};
        std::chrono::steady_clock::time_point lastUpdate;

        // Delete copy operations (atomics are not copyable)
        TurnInMetrics() = default;
        TurnInMetrics(TurnInMetrics const&) = delete;
        TurnInMetrics& operator=(TurnInMetrics const&) = delete;

        void Reset()
        {
            questsTurnedIn = 0; turnInAttempts = 0; successfulTurnIns = 0;
            failedTurnIns = 0; averageTurnInTime = 15000.0f; turnInSuccessRate = 0.95f;
            totalTravelDistance = 0; rewardsSelected = 0; rewardSelectionAccuracy = 0.85f;
            lastUpdate = std::chrono::steady_clock::now();
        }

        float GetSuccessRate() const
        {
            uint32 attempts = turnInAttempts.load();
            uint32 successful = successfulTurnIns.load();
            return attempts > 0 ? (float)successful / attempts : 0.0f;
        }

        // Snapshot structure for returning metrics
        struct Snapshot {
            uint32 questsTurnedIn;
            uint32 turnInAttempts;
            uint32 successfulTurnIns;
            uint32 failedTurnIns;
            float averageTurnInTime;
            float turnInSuccessRate;
            uint32 totalTravelDistance;
            uint32 rewardsSelected;
            float rewardSelectionAccuracy;
            std::chrono::steady_clock::time_point lastUpdate;

            float GetSuccessRate() const
            {
                return turnInAttempts > 0 ? (float)successfulTurnIns / turnInAttempts : 0.0f;
            }
        };

        // Create a snapshot of current metrics
        Snapshot CreateSnapshot() const
        {
            Snapshot snapshot;
            snapshot.questsTurnedIn = questsTurnedIn.load();
            snapshot.turnInAttempts = turnInAttempts.load();
            snapshot.successfulTurnIns = successfulTurnIns.load();
            snapshot.failedTurnIns = failedTurnIns.load();
            snapshot.averageTurnInTime = averageTurnInTime.load();
            snapshot.turnInSuccessRate = turnInSuccessRate.load();
            snapshot.totalTravelDistance = totalTravelDistance.load();
            snapshot.rewardsSelected = rewardsSelected.load();
            snapshot.rewardSelectionAccuracy = rewardSelectionAccuracy.load();
            snapshot.lastUpdate = lastUpdate;
            return snapshot;
        }
    };

    // Type alias for metrics snapshot
    using TurnInMetricsSnapshot = TurnInMetrics::Snapshot;

    TurnInMetricsSnapshot GetBotTurnInMetrics(uint32 botGuid);
    TurnInMetricsSnapshot GetGlobalTurnInMetrics();

    // Quest chain management
    void HandleQuestChainProgression(Player* bot, uint32 completedQuestId);
    uint32 GetNextQuestInChain(uint32 completedQuestId);
    void AutoAcceptFollowUpQuests(Player* bot, uint32 completedQuestId);
    void PrioritizeChainQuests(Player* bot);

    // Configuration and settings
    void SetTurnInStrategy(uint32 botGuid, TurnInStrategy strategy);
    TurnInStrategy GetTurnInStrategy(uint32 botGuid);
    void SetRewardSelectionStrategy(uint32 botGuid, RewardSelectionStrategy strategy);
    RewardSelectionStrategy GetRewardSelectionStrategy(uint32 botGuid);
    void SetBatchTurnInThreshold(uint32 botGuid, uint32 threshold);

    // Error handling and recovery
    void HandleTurnInError(Player* bot, uint32 questId, const std::string& error);
    void RecoverFromTurnInFailure(Player* bot, uint32 questId);
    void RetryFailedTurnIn(Player* bot, uint32 questId);
    void ValidateTurnInState(Player* bot, uint32 questId);

    // Update and maintenance
    void Update(uint32 diff);
    void UpdateBotTurnIns(Player* bot, uint32 diff);
    void ProcessScheduledTurnIns();
    void CleanupCompletedTurnIns();

private:
    Player* _bot;

    // Core data structures
    std::unordered_map<uint32, std::vector<QuestTurnInData>> _botTurnInQueues; // botGuid -> turnIns
    std::unordered_map<uint32, TurnInStrategy> _botTurnInStrategies;
    std::unordered_map<uint32, RewardSelectionStrategy> _botRewardStrategies;
    std::unordered_map<uint32, TurnInMetrics> _botMetrics;
    

    // Batch processing
    std::unordered_map<uint32, TurnInBatch> _scheduledBatches; // botGuid -> batch
    std::queue<std::pair<uint32, uint32>> _scheduledTurnIns; // <botGuid, questId>
    

    // Quest giver database
    std::unordered_map<uint32, uint32> _questToTurnInNpc; // questId -> npcGuid
    std::unordered_map<uint32, Position> _questGiverLocations; // npcGuid -> position
    std::unordered_map<uint32, std::vector<uint32>> _npcQuests; // npcGuid -> questIds

    // Reward analysis cache
    std::unordered_map<uint32, std::vector<QuestRewardItem>> _questRewardCache; // questId -> rewards
    

    // Performance tracking
    TurnInMetrics _globalMetrics;

    // Helper functions
    void InitializeTurnInData(Player* bot, uint32 questId);
    void LoadQuestGiverDatabase();
    void UpdateTurnInQueue(Player* bot);
    bool ValidateQuestTurnIn(Player* bot, uint32 questId);
    void ExecuteTurnInWorkflow(Player* bot, const QuestTurnInData& turnInData);

    // Reward selection algorithms
    uint32 SelectHighestValueReward(const std::vector<QuestRewardItem>& rewards, Player* bot);
    uint32 SelectBestUpgradeReward(const std::vector<QuestRewardItem>& rewards, Player* bot);
    uint32 SelectHighestVendorValueReward(const std::vector<QuestRewardItem>& rewards, Player* bot);
    uint32 SelectStatPriorityReward(const std::vector<QuestRewardItem>& rewards, Player* bot);
    uint32 SelectClassAppropriateReward(const std::vector<QuestRewardItem>& rewards, Player* bot);

    // Navigation and pathfinding
    std::vector<Position> PlanTurnInRoute(Player* bot, const std::vector<uint32>& questGiverGuids);
    float CalculateTravelTime(Player* bot, const Position& destination);
    void OptimizeTravelRoute(Player* bot, std::vector<uint32>& questGiverGuids);

    // Dialog and interaction
    void InteractWithQuestGiver(Player* bot, uint32 questGiverGuid);
    void HandleQuestRewardDialog(Player* bot, uint32 questId);
    void ProcessQuestTurnInResponse(Player* bot, uint32 questId, bool wasSuccessful);

    // Group coordination helpers
    void BroadcastTurnInIntent(Group* group, uint32 questId, Player* bot);
    void SynchronizeTurnInTiming(Group* group, uint32 questId);
    void CoordinateRewardSelection(Group* group, uint32 questId);

    // Performance optimization
    void OptimizeTurnInPerformance(Player* bot);
    void CacheTurnInData(Player* bot);
    void PreloadQuestGiverData();
    void BatchTurnInOperations();

    // Error handling
    void LogTurnInError(Player* bot, uint32 questId, const std::string& error);
    void HandleQuestGiverNotFound(Player* bot, uint32 questId);
    void HandleInvalidQuestState(Player* bot, uint32 questId);
    void HandleRewardSelectionFailure(Player* bot, uint32 questId);

    // Constants
    static constexpr uint32 TURNIN_UPDATE_INTERVAL = 3000; // 3 seconds
    static constexpr uint32 BATCH_TURNIN_THRESHOLD = 3; // 3+ completed quests
    static constexpr uint32 MAX_TURNIN_DISTANCE = 200; // 200 yards
    static constexpr uint32 TURNIN_TIMEOUT = 30000; // 30 seconds
    static constexpr float QUEST_GIVER_INTERACTION_RANGE = 5.0f;
    static constexpr uint32 REWARD_SELECTION_TIMEOUT = 10000; // 10 seconds
    static constexpr uint32 MAX_SCHEDULED_TURNINS = 25;
    static constexpr uint32 CHAIN_QUEST_PRIORITY_BONUS = 50;
    static constexpr float MIN_UPGRADE_VALUE_THRESHOLD = 0.1f;
    static constexpr uint32 TURNIN_RETRY_DELAY = 5000; // 5 seconds
};

} // namespace Playerbot