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
#include "DungeonBehavior.h"
#include "Player.h"
#include "Group.h"
#include "Map.h"
#include "InstanceScript.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>
#include <atomic>
#include <mutex>
#include <chrono>

namespace Playerbot
{

/**
 * @brief Instance progress tracking information
 */
struct InstanceProgress
{
    uint32 groupId{0};
    uint32 instanceId{0};
    uint32 mapId{0};
    uint32 encountersCompleted{0};
    uint32 totalEncounters{0};
    uint32 progressPercent{0};
    uint32 startTime{0};
    uint32 lastUpdateTime{0};
    ::std::vector<::std::string> progressNotes;

    // Additional fields for coordination
    float progressPercentage{0.0f};
    bool isOnTrack{true};
    uint32 currentCheckpoint{0};
    ::std::vector<uint32> collectedLoot;
    uint32 estimatedCompletionTime{0};
    ::std::vector<uint32> completedEncounters;

    InstanceProgress() = default;

    InstanceProgress(uint32 gId, uint32 iId, uint32 mId)
        : groupId(gId), instanceId(iId), mapId(mId)
        , encountersCompleted(0), totalEncounters(0), progressPercent(0)
        , startTime(0), lastUpdateTime(0)
        , progressPercentage(0.0f), isOnTrack(true), currentCheckpoint(0)
        , estimatedCompletionTime(0) {}
};

/**
 * @brief Non-atomic coordination metrics (copyable)
 */
struct CoordinationMetrics
{
    uint32 instancesCoordinated{0};
    uint32 successfulCompletions{0};
    uint32 failedInstances{0};
    uint32 totalGroupCommands{0};
    uint32 formationBreaks{0};
    uint32 averageCompletionTimeMs{0};
    uint32 lastCoordinationTime{0};

    // Additional coordination fields
    uint32 coordinationEvents{0};
    uint32 successfulCoordinations{0};
    uint32 coordinationFailures{0};
    float groupSynchronization{1.0f};
    uint32 communicationEvents{0};
    float movementEfficiency{1.0f};
    uint32 averageResponseTime{0};

    CoordinationMetrics() = default;

    void Reset()
    {
        instancesCoordinated = 0;
        successfulCompletions = 0;
        failedInstances = 0;
        totalGroupCommands = 0;
        formationBreaks = 0;
        averageCompletionTimeMs = 0;
        lastCoordinationTime = 0;
        coordinationEvents = 0;
        successfulCoordinations = 0;
        coordinationFailures = 0;
        groupSynchronization = 1.0f;
        communicationEvents = 0;
        movementEfficiency = 1.0f;
        averageResponseTime = 0;
    }
};

/**
 * @brief Atomic coordination metrics for thread-safe operations
 */
struct AtomicCoordinationMetrics
{
    ::std::atomic<uint32> instancesCoordinated{0};
    ::std::atomic<uint32> successfulCompletions{0};
    ::std::atomic<uint32> failedInstances{0};
    ::std::atomic<uint32> totalGroupCommands{0};
    ::std::atomic<uint32> formationBreaks{0};
    ::std::atomic<uint32> averageCompletionTimeMs{0};
    ::std::atomic<uint32> lastCoordinationTime{0};

    // Additional coordination fields
    ::std::atomic<uint32> coordinationEvents{0};
    ::std::atomic<uint32> successfulCoordinations{0};
    ::std::atomic<uint32> coordinationFailures{0};
    ::std::atomic<float> groupSynchronization{1.0f};
    ::std::atomic<uint32> communicationEvents{0};
    ::std::atomic<float> movementEfficiency{1.0f};
    ::std::atomic<uint32> averageResponseTime{0};

    AtomicCoordinationMetrics() = default;

    float GetCoordinationSuccessRate() const
    {
        uint32 total = successfulCoordinations.load() + coordinationFailures.load();
        if (total == 0)
            return 1.0f;
        return static_cast<float>(successfulCoordinations.load()) / static_cast<float>(total);
    }

    void Reset()
    {
        instancesCoordinated = 0;
        successfulCompletions = 0;
        failedInstances = 0;
        totalGroupCommands = 0;
        formationBreaks = 0;
        averageCompletionTimeMs = 0;
        lastCoordinationTime = 0;
        coordinationEvents = 0;
        successfulCoordinations = 0;
        coordinationFailures = 0;
        groupSynchronization = 1.0f;
        communicationEvents = 0;
    }

    CoordinationMetrics GetSnapshot() const
    {
        CoordinationMetrics snapshot;
        snapshot.instancesCoordinated = instancesCoordinated.load();
        snapshot.successfulCompletions = successfulCompletions.load();
        snapshot.failedInstances = failedInstances.load();
        snapshot.totalGroupCommands = totalGroupCommands.load();
        snapshot.formationBreaks = formationBreaks.load();
        snapshot.averageCompletionTimeMs = averageCompletionTimeMs.load();
        snapshot.lastCoordinationTime = lastCoordinationTime.load();
        snapshot.coordinationEvents = coordinationEvents.load();
        snapshot.successfulCoordinations = successfulCoordinations.load();
        snapshot.coordinationFailures = coordinationFailures.load();
        snapshot.groupSynchronization = groupSynchronization.load();
        snapshot.communicationEvents = communicationEvents.load();
        snapshot.movementEfficiency = movementEfficiency.load();
        snapshot.averageResponseTime = averageResponseTime.load();
        return snapshot;
    }
};

/**
 * @brief Advanced instance coordination system for dungeon group management
 *
 * This system handles instance-specific coordination, including formation movement,
 * encounter preparation, loot distribution, and group communication within dungeons.
 */
class TC_GAME_API InstanceCoordination final 
{
public:
    static InstanceCoordination* instance();

    // Core instance coordination
    void InitializeInstanceCoordination(Group* group, Map* instanceMap);
    void UpdateInstanceCoordination(Group* group, uint32 diff);
    void HandleInstanceCompletion(Group* group);
    void HandleInstanceFailure(Group* group);

    // Group formation and movement
    void CoordinateGroupMovement(Group* group, const Position& destination);
    void MaintainDungeonFormation(Group* group);
    void HandleFormationBreaks(Group* group);
    void AdaptFormationToTerrain(Group* group, const Position& location);

    // Encounter preparation and coordination
    void PrepareForEncounter(Group* group, uint32 encounterId);
    void CoordinateEncounterStart(Group* group, uint32 encounterId);
    void MonitorEncounterProgress(Group* group, uint32 encounterId);
    void HandleEncounterRecovery(Group* group, uint32 encounterId);

    // Resource management and optimization
    void CoordinateResourceUsage(Group* group);
    void ManageGroupMana(Group* group);
    void CoordinateRestBreaks(Group* group);
    void OptimizeGroupEfficiency(Group* group);

    // Communication and coordination
    void BroadcastInstanceInformation(Group* group, const ::std::string& message);
    void CoordinateGroupActions(Group* group, const ::std::string& action);
    void HandleGroupDecisionMaking(Group* group, const ::std::string& decision);
    void SynchronizeGroupStates(Group* group);

    // Loot coordination and distribution
    void CoordinateLootDistribution(Group* group, const ::std::vector<uint32>& lootItems);
    void HandleLootRolling(Group* group, uint32 itemId);
    void ManageLootPriorities(Group* group);
    void ResolveeLootConflicts(Group* group, uint32 itemId);

    // Progress tracking and optimization (InstanceProgress defined in IInstanceCoordination.h interface)
    InstanceProgress GetInstanceProgress(uint32 groupId);
    void UpdateInstanceProgress(Group* group);
    void AnalyzeProgressEfficiency(Group* group);

    // Route planning and navigation
    void PlanInstanceRoute(Group* group, const ::std::vector<uint32>& objectiveIds);
    void UpdateNavigationRoute(Group* group, const Position& currentLocation);
    void HandleNavigationObstacles(Group* group, const ::std::vector<Position>& obstacles);
    Position GetNextWaypoint(Group* group);

    // Safety and emergency coordination
    void MonitorGroupSafety(Group* group);
    void HandleEmergencySituations(Group* group, const ::std::string& emergency);
    void CoordinateEmergencyEvacuation(Group* group);
    void HandlePlayerIncapacitation(Group* group, Player* incapacitatedPlayer);

    // Performance optimization (CoordinationMetrics defined in IInstanceCoordination.h interface)
    CoordinationMetrics GetGroupCoordinationMetrics(uint32 groupId);
    CoordinationMetrics GetGlobalCoordinationMetrics();

    // Advanced coordination features
    void EnablePredictiveCoordination(Group* group, bool enable);
    void AdaptCoordinationToGroupSkill(Group* group);
    void OptimizeCoordinationAlgorithms(Group* group);
    void HandleDynamicGroupChanges(Group* group, Player* newMember = nullptr);

    // Instance-specific coordination strategies
    void ApplyInstanceSpecificStrategy(Group* group, uint32 instanceId);
    void HandleInstanceMechanics(Group* group, const ::std::string& mechanic);
    void AdaptToInstanceDifficulty(Group* group, float difficultyRating);

    // Configuration and settings
    void SetCoordinationPrecision(uint32 groupId, float precision); // 0.0 = loose, 1.0 = strict
    void SetFormationStyle(uint32 groupId, const ::std::string& formationStyle);
    void EnableAdvancedCoordination(uint32 groupId, bool enable);
    void SetCommunicationLevel(uint32 groupId, uint32 level); // 0 = minimal, 3 = verbose

    // Error handling and recovery
    void HandleCoordinationError(Group* group, const ::std::string& error);
    void RecoverFromCoordinationFailure(Group* group);
    void DiagnoseCoordinationIssues(Group* group);
    void ResetCoordinationState(Group* group);

    // Update and maintenance
    void Update(uint32 diff);
    void UpdateGroupCoordination(Group* group, uint32 diff);
    void CleanupInactiveCoordinations();

private:
    InstanceCoordination();
    ~InstanceCoordination() = default;

    // Core coordination data
    ::std::unordered_map<uint32, InstanceProgress> _instanceProgress; // groupId -> progress
    ::std::unordered_map<uint32, AtomicCoordinationMetrics> _groupMetrics;
    ::std::unordered_map<uint32, ::std::vector<Position>> _groupRoutes; // groupId -> waypoints
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BEHAVIOR_MANAGER> _coordinationMutex;

    // Formation and movement data
    struct FormationData
    {
        ::std::string formationType;
        ::std::unordered_map<uint32, Position> memberPositions; // playerGuid -> relative position
        Position centerPoint;
        float formationRadius;
        float movementSpeed;
        bool isCompact;
        uint32 lastUpdateTime;

        FormationData() : formationType("default"), formationRadius(10.0f)
            , movementSpeed(1.0f), isCompact(true), lastUpdateTime(GameTime::GetGameTimeMS()) {}
    };

    ::std::unordered_map<uint32, FormationData> _groupFormations; // groupId -> formation
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BEHAVIOR_MANAGER> _formationMutex;

    // Communication and decision tracking
    struct CoordinationState
    {
        uint32 groupId;
        ::std::queue<::std::string> pendingActions;
        ::std::unordered_map<::std::string, uint32> decisionVotes; // decision -> vote count
        ::std::vector<::std::string> recentCommunications;
        uint32 lastCoordinationTime;
        bool requiresConsensus;
        uint32 coordinationLevel;

        CoordinationState() : groupId(0), lastCoordinationTime(0)
            , requiresConsensus(false), coordinationLevel(2) {}

        CoordinationState(uint32 gId) : groupId(gId), lastCoordinationTime(GameTime::GetGameTimeMS())
            , requiresConsensus(false), coordinationLevel(2) {}
    };

    ::std::unordered_map<uint32, CoordinationState> _coordinationStates; // groupId -> state

    // Resource and efficiency tracking
    struct ResourceCoordination
    {
        ::std::unordered_map<uint32, float> memberMana; // playerGuid -> mana percentage
        ::std::unordered_map<uint32, float> memberHealth; // playerGuid -> health percentage
        ::std::unordered_map<uint32, uint32> memberCooldowns; // playerGuid -> major cooldown count
        uint32 groupReadiness; // 0-100% ready state
        uint32 lastResourceCheck;
        bool needsRestBreak;

        ResourceCoordination() : groupReadiness(100), lastResourceCheck(GameTime::GetGameTimeMS())
            , needsRestBreak(false) {}
    };

    ::std::unordered_map<uint32, ResourceCoordination> _resourceCoordination; // groupId -> resources

    // Performance tracking
    AtomicCoordinationMetrics _globalMetrics;

    // Helper functions
    void UpdateGroupFormation(Group* group);
    void CalculateOptimalFormation(Group* group, const Position& destination);
    void SynchronizeGroupMovement(Group* group, const Position& destination);
    void HandleMovementLaggers(Group* group);

    // Communication helpers
    void ProcessPendingActions(Group* group);
    void BroadcastCoordinationMessage(Group* group, const ::std::string& message);
    void GatherGroupInput(Group* group, const ::std::string& question);
    void ResolveGroupDecision(Group* group, const ::std::string& decision);

    // Resource management helpers
    void CheckGroupResources(Group* group);
    void CoordinateResourceRecovery(Group* group);
    void OptimizeResourceDistribution(Group* group);
    bool ShouldTakeRestBreak(Group* group);

    // Loot coordination helpers
    void AnalyzeLootValue(Group* group, uint32 itemId);
    void DetermineLootPriority(Group* group, uint32 itemId);
    void HandleLootDistribution(Group* group, uint32 itemId, Player* winner);
    void UpdateLootHistory(Group* group, uint32 itemId, Player* recipient);

    // Navigation and pathfinding
    ::std::vector<Position> CalculateOptimalRoute(Group* group, const ::std::vector<Position>& objectives);
    void UpdateRouteProgress(Group* group);
    void HandleRouteDeviations(Group* group);
    Position CalculateGroupCenterPoint(Group* group);

    // Performance optimization
    void OptimizeCoordinationPerformance(Group* group);
    void AnalyzeCoordinationEfficiency(Group* group);
    void AdaptCoordinationStrategy(Group* group);
    void UpdateCoordinationMetrics(uint32 groupId, bool wasSuccessful, uint32 responseTime);

    // Constants
    static constexpr uint32 COORDINATION_UPDATE_INTERVAL = 1000; // 1 second
    static constexpr uint32 FORMATION_UPDATE_INTERVAL = 2000; // 2 seconds
    static constexpr uint32 RESOURCE_CHECK_INTERVAL = 5000; // 5 seconds
    static constexpr float FORMATION_TOLERANCE = 5.0f; // 5 yards
    static constexpr float MOVEMENT_SYNC_TOLERANCE = 3.0f; // 3 yards
    static constexpr uint32 COMMUNICATION_TIMEOUT = 30000; // 30 seconds
    static constexpr float MIN_GROUP_MANA_THRESHOLD = 0.3f; // 30% mana
    static constexpr float MIN_GROUP_HEALTH_THRESHOLD = 0.5f; // 50% health
    static constexpr uint32 LOOT_DECISION_TIMEOUT = 15000; // 15 seconds
    static constexpr uint32 MAX_COORDINATION_FAILURES = 5;
    static constexpr float COORDINATION_SUCCESS_THRESHOLD = 0.8f; // 80%
};

} // namespace Playerbot