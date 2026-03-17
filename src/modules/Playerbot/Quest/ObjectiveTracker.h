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
#include "QuestCompletion.h"
#include "Player.h"
#include "QuestDef.h"
#include "Unit.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <atomic>
#include <mutex>
#include <chrono>
#include "GameTime.h"

namespace Playerbot
{

/**
 * @brief Objective tracking state information
 */
struct ObjectiveState
{
    uint32 questId{0};
    uint32 objectiveIndex{0};
    QuestObjectiveType type{QuestObjectiveType::MONSTER};
    uint32 targetId{0};
    uint32 requiredCount{0};
    uint32 currentCount{0};
    uint32 lastProgress{0};
    uint32 lastUpdateTime{0};
    uint32 stuckDuration{0};
    float progressVelocity{0.0f};
    float estimatedCompletionTime{0.0f};
    Position targetLocation;
    Position lastKnownPosition;  // Last known position of objective target
    bool isActive{false};
    bool isComplete{false};
    bool isStuck{false};
    bool needsRefresh{true};
    // Additional tracking fields
    ObjectiveStatus status{ObjectiveStatus::NOT_STARTED};
    uint32 requiredProgress{0};      // Same as requiredCount, for compatibility
    uint32 currentProgress{0};       // Same as currentCount, for compatibility
    uint32 timeStarted{0};           // GameTime when tracking started
    float completionVelocity{0.0f};  // Rate of progress completion
    float estimatedTimeRemaining{0.0f}; // Estimated time to complete
    bool isOptimized{false};         // Has optimization been applied
    uint32 failureCount{0};          // Number of failed attempts
    uint32 stuckTime{0};             // Time stuck on this objective
    std::vector<uint32> targetIds;   // Multiple target IDs for multi-target objectives

    ObjectiveState() = default;
    ObjectiveState(uint32 qId, uint32 idx) : questId(qId), objectiveIndex(idx) {}
};

/**
 * @brief Objective priority calculation result
 */
struct ObjectivePriority
{
    uint32 questId{0};
    uint32 objectiveIndex{0};
    float priority{0.0f};
    float urgency{0.0f};
    float difficulty{0.0f};
    float efficiency{0.0f};
    float proximity{0.0f};
    uint32 calculationTime{0};
    float urgencyFactor{1.0f};
    float difficultyFactor{1.0f};
    float efficiencyFactor{1.0f};
    float proximityFactor{1.0f};
    float priorityScore{0.0f};

    ObjectivePriority() = default;
    ObjectivePriority(uint32 qId, uint32 idx)
        : questId(qId), objectiveIndex(idx) {}
    ObjectivePriority(uint32 qId, uint32 idx, float prio)
        : questId(qId), objectiveIndex(idx), priority(prio), priorityScore(prio) {}

    bool operator<(const ObjectivePriority& other) const { return priority > other.priority; }
};

/**
 * @brief Objective tracking analytics
 */
struct ObjectiveAnalytics
{
    uint32 objectivesTracked{0};
    uint32 objectivesCompleted{0};
    uint32 objectivesFailed{0};
    uint32 objectivesStarted{0};
    uint32 stuckInstances{0};
    float averageCompletionTime{0.0f};
    float trackingAccuracy{0.0f};
    uint32 targetScans{0};
    uint32 successfulScans{0};
    uint32 targetsFound{0};
    uint32 lastUpdateTime{0};
    uint32 lastAnalyticsUpdate{0};

    void Reset()
    {
        objectivesTracked = 0;
        objectivesCompleted = 0;
        objectivesFailed = 0;
        objectivesStarted = 0;
        stuckInstances = 0;
        averageCompletionTime = 0.0f;
        trackingAccuracy = 0.0f;
        targetScans = 0;
        successfulScans = 0;
        targetsFound = 0;
        lastUpdateTime = 0;
        lastAnalyticsUpdate = 0;
    }
};

/**
 * @brief Advanced objective tracking system for quest completion monitoring
 *
 * This system provides real-time tracking of quest objectives, progress monitoring,
 * and intelligent adaptation to changing quest states for optimal completion efficiency.
 */
class TC_GAME_API ObjectiveTracker final
{
public:
    explicit ObjectiveTracker(Player* bot);
    ~ObjectiveTracker();
    ObjectiveTracker(ObjectiveTracker const&) = delete;
    ObjectiveTracker& operator=(ObjectiveTracker const&) = delete;

    // Core objective tracking
    void StartTrackingObjective(Player* bot, const QuestObjectiveData& objective);
    void StopTrackingObjective(Player* bot, uint32 questId, uint32 objectiveIndex);
    void UpdateObjectiveTracking(Player* bot, uint32 diff);
    void RefreshObjectiveStates(Player* bot);
    void RefreshObjectiveState(Player* bot, ObjectiveState& state);

    // Progress monitoring
    void MonitorObjectiveProgress(Player* bot, uint32 questId, uint32 objectiveIndex);
    void UpdateProgressMetrics(Player* bot, const QuestObjectiveData& objective);
    bool HasProgressStalled(Player* bot, uint32 questId, uint32 objectiveIndex);
    float CalculateObjectiveVelocity(Player* bot, uint32 questId, uint32 objectiveIndex);

    // Target detection and analysis
    std::vector<uint32> DetectObjectiveTargets(Player* bot, const QuestObjectiveData& objective);
    std::vector<uint32> ScanForKillTargets(Player* bot, uint32 creatureId, float radius = 100.0f);
    std::vector<uint32> ScanForCollectibles(Player* bot, uint32 itemId, float radius = 50.0f);
    std::vector<uint32> ScanForGameObjects(Player* bot, uint32 objectId, float radius = 50.0f);

    // Objective state management
    ObjectiveState GetObjectiveState(Player* bot, uint32 questId, uint32 objectiveIndex);
    void UpdateObjectiveState(Player* bot, const ObjectiveState& state);
    std::vector<ObjectiveState> GetActiveObjectives(Player* bot);

    // Intelligent objective prioritization
    std::vector<ObjectivePriority> CalculateObjectivePriorities(Player* bot);
    ObjectivePriority GetHighestPriorityObjective(Player* bot);
    void OptimizeObjectiveSequence(Player* bot, std::vector<ObjectivePriority>& priorities);

    // Target availability and spawn tracking
    void TrackTargetAvailability(Player* bot, uint32 questId, uint32 targetId);
    bool IsTargetAvailable(uint32 targetId, const Position& location, float radius = 100.0f);
    uint32 GetTargetRespawnTime(uint32 targetId);
    Position GetOptimalTargetLocation(uint32 targetId, const Position& playerPosition);

    // Competition and interference management
    void MonitorTargetCompetition(Player* bot, uint32 targetId);
    bool IsTargetContested(uint32 targetId, float radius = 50.0f);
    void HandleTargetCompetition(Player* bot, uint32 targetId);
    std::vector<Position> FindAlternativeTargetLocations(uint32 targetId, const Position& currentLocation);

    // Group objective coordination
    void CoordinateGroupObjectives(Group* group, uint32 questId);
    void DistributeObjectiveTargets(Group* group, uint32 questId, uint32 objectiveIndex);
    void SynchronizeObjectiveProgress(Group* group, uint32 questId);
    void HandleObjectiveConflicts(Group* group, uint32 questId, uint32 objectiveIndex);

    // Performance analytics
    const ObjectiveAnalytics& GetBotObjectiveAnalytics(uint32 botGuid);
    const ObjectiveAnalytics& GetGlobalObjectiveAnalytics();

    // Advanced tracking features
    void EnablePredictiveTracking(Player* bot, bool enable);
    void PredictObjectiveCompletion(Player* bot, uint32 questId, uint32 objectiveIndex);
    void AdaptTrackingStrategy(Player* bot, const ObjectiveState& state);
    void OptimizeTrackingPerformance(Player* bot);

    // Error detection and recovery
    void DetectTrackingErrors(Player* bot);
    void HandleTrackingFailure(Player* bot, uint32 questId, uint32 objectiveIndex, const std::string& error);
    void HandleStuckObjective(Player* bot, ObjectiveState& state);
    void RecoverTrackingState(Player* bot, uint32 questId);
    void ValidateObjectiveConsistency(Player* bot);

    // Data conversion utilities
    QuestObjectiveData ConvertToQuestObjectiveData(const ObjectiveState& state);

    // Configuration and settings
    void SetTrackingPrecision(uint32 botGuid, float precision); // 0.0 = low, 1.0 = high
    void SetUpdateFrequency(uint32 botGuid, uint32 frequencyMs);
    void EnableAdvancedFeatures(uint32 botGuid, bool enable);

    // Update and maintenance
    void Update(uint32 diff);
    void UpdateBotTracking(Player* bot, uint32 diff);
    void CleanupInactiveTracking();

    // Position finding (PUBLIC - used by QuestStrategy for dynamic position refresh)
    Position FindObjectiveTargetLocation(Player* bot, const QuestObjectiveData& objective);

private:
    Player* _bot;

    // Thread safety - MUST be locked when accessing any member maps
    // Using recursive_mutex to allow nested calls from the same thread
    mutable std::recursive_mutex _mutex;

    // Tracking data storage
    std::unordered_map<uint32, std::vector<ObjectiveState>> _botObjectiveStates; // botGuid -> objectives
    std::unordered_map<uint32, std::vector<ObjectivePriority>> _botObjectivePriorities; // botGuid -> priorities
    std::unordered_map<uint32, ObjectiveAnalytics> _botAnalytics;
    

    // Target tracking and caching
    struct TargetTrackingData
    {
        uint32 targetId;
        std::vector<Position> knownLocations;
        uint32 lastSeenTime;
        uint32 respawnTime;
        bool isAvailable;
        uint32 competitionLevel; // Number of players competing for this target
        float spawnProbability;

        // Default constructor for std::unordered_map compatibility
        TargetTrackingData() : targetId(0), lastSeenTime(0), respawnTime(0)
            , isAvailable(true), competitionLevel(0), spawnProbability(1.0f) {}

        TargetTrackingData(uint32 id) : targetId(id), lastSeenTime(0), respawnTime(0)
            , isAvailable(true), competitionLevel(0), spawnProbability(1.0f) {}
    };

    std::unordered_map<uint32, TargetTrackingData> _targetTracking; // targetId -> data
    

    // Group coordination data
    std::unordered_map<uint32, std::unordered_map<uint32, uint32>> _groupObjectiveAssignments; // groupId -> memberGuid -> objectiveIndex
    std::unordered_map<uint32, uint32> _groupObjectiveSyncTime; // groupId -> lastSyncTime

    // Performance tracking
    ObjectiveAnalytics _globalAnalytics;

    // Helper functions
    void UpdateObjectiveProgress(Player* bot, ObjectiveState& state);
    void CalculateObjectiveVelocity(ObjectiveState& state);
    void EstimateCompletionTime(ObjectiveState& state);
    bool ValidateObjectiveState(Player* bot, const ObjectiveState& state);
    void OptimizeObjectiveExecution(Player* bot, ObjectiveState& state);
    uint32 GetCurrentObjectiveProgress(Player* bot, const Quest* quest, uint32 objectiveIndex);
    uint32 AssignObjectiveToGroupMember(Group* group, Player* member, uint32 questId);
    void AssignSpecificTargetToBot(Player* bot, uint32 questId, uint32 objectiveIndex, uint32 targetIndex);

    // Priority calculation helpers
    float CalculateUrgencyFactor(Player* bot, const ObjectiveState& state);
    float CalculateDifficultyFactor(Player* bot, const ObjectiveState& state);
    float CalculateEfficiencyFactor(Player* bot, const ObjectiveState& state);
    float CalculateProximityFactor(Player* bot, const ObjectiveState& state);

    // Target detection algorithms
    std::vector<uint32> ScanAreaForTargets(const Position& center, float radius, uint32 targetId);
    void UpdateTargetAvailability(uint32 targetId, const Position& location);
    void PredictTargetSpawns(uint32 targetId);
    void AnalyzeTargetCompetition(uint32 targetId, const Position& location);

    // Group coordination helpers
    void AssignObjectivesToGroupMembers(Group* group, uint32 questId, uint32 objectiveIndex);
    void BalanceObjectiveLoad(Group* group, uint32 questId);
    void ResolveObjectiveConflicts(Group* group, uint32 questId, uint32 objectiveIndex);

    // Performance optimization
    void OptimizeTrackingAlgorithms();
    void CacheFrequentlyAccessedData();
    void BatchTrackingUpdates();
    void PruneStaleTrackingData();

    // Analytics and reporting
    void UpdateTrackingAnalytics(uint32 botGuid, const ObjectiveState& state, bool wasSuccessful);
    void GenerateTrackingReport(Player* bot);
    void LogTrackingEvent(uint32 botGuid, const std::string& event, const std::string& details = "");

    // Constants
    static constexpr uint32 TRACKING_UPDATE_INTERVAL = 2000; // 2 seconds
    static constexpr uint32 TARGET_SCAN_INTERVAL = 5000; // 5 seconds
    static constexpr uint32 PRIORITY_RECALC_INTERVAL = 10000; // 10 seconds
    static constexpr float DEFAULT_TRACKING_PRECISION = 0.8f;
    static constexpr uint32 MAX_TRACKED_OBJECTIVES = 25;
    static constexpr float STALLED_PROGRESS_THRESHOLD = 0.1f;
    static constexpr uint32 STUCK_DETECTION_TIME = 120000; // 2 minutes
    static constexpr uint32 TARGET_CACHE_DURATION = 300000; // 5 minutes
    static constexpr float COMPETITION_THRESHOLD = 3.0f; // 3+ players = high competition
    static constexpr uint32 OBJECTIVE_TIMEOUT = 1800000; // 30 minutes
    static constexpr float MIN_VELOCITY_THRESHOLD = 0.01f; // objectives per second
};

} // namespace Playerbot