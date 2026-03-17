/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "AI/Strategy/Strategy.h"
#include "AI/Combat/FormationManager.h"
#include "Position.h"
#include "ObjectGuid.h"
#include <memory>
#include <chrono>
#include <atomic>
#include <unordered_map>

// Forward declarations
class Player;
class Group;
class Unit;
class PathGenerator;

namespace Playerbot
{

// Forward declarations
class BotAI;
class Action;
class Trigger;
class PositionManager;
class PathfindingManager;

// Follow behavior states
enum class FollowState : uint8
{
    IDLE = 0,               // Not following anyone
    FOLLOWING = 1,          // Actively following leader
    WAITING = 2,            // Waiting for leader to move
    CATCHING_UP = 3,        // Moving faster to catch up
    TELEPORTING = 4,        // Teleporting to leader
    POSITIONING = 5,        // Adjusting formation position
    COMBAT_FOLLOW = 6,      // Following during combat
    LOST = 7,               // Lost sight of leader
    PAUSED = 8             // Temporarily paused following
};

// Follow mode types
enum class FollowMode : uint8
{
    TIGHT = 0,              // Stay very close (3-5 yards)
    NORMAL = 1,             // Normal follow distance (8-12 yards)
    LOOSE = 2,              // Loose follow (15-20 yards)
    FORMATION = 3,          // Use formation positioning
    CUSTOM = 4              // Custom distance set by user
};

// Formation position preferences for follow behavior
// NOTE: FormationPosition is also defined in MovementTypes.h with different fields
// This definition is specific to LeaderFollowBehavior and uses different semantics
struct FollowFormationPosition
{
    float angle;            // Angle relative to leader (0 = front, PI = behind)
    float distance;         // Distance from leader
    float height;           // Height offset for flying/swimming
    bool maintainOrientation; // Keep facing same direction as leader
    bool allowVariation;    // Allow slight position variations
    float variationRange;   // Range of allowed variation
};

// Follow behavior configuration
struct FollowConfig
{
    FollowMode mode = FollowMode::TIGHT;
    float minDistance = 2.0f;  // Closer for melee combat
    float maxDistance = 5.0f;   // Within melee range
    float teleportDistance = 100.0f;
    float catchUpSpeedBoost = 1.5f;
    bool autoTeleport = true;
    bool maintainLineOfSight = true;
    bool avoidAoEAreas = true;
    bool followThroughPortals = true;
    bool followInCombat = true;
    // REFACTORED: Removed updateInterval - now runs every frame
    uint32 teleportDelay = 2000;  // ms before teleporting
};

// Follow target information
struct FollowTarget
{
    ObjectGuid guid;
    Player* player = nullptr;
    Position lastKnownPosition;
    Position predictedPosition;
    float currentDistance = 0.0f;
    float currentSpeed = 0.0f;
    bool isMoving = false;
    bool inLineOfSight = true;
    uint32 lastSeen = 0;
    uint32 lostDuration = 0;
};

// Performance metrics for follow behavior
struct FollowMetrics
{
    ::std::atomic<uint32> positionUpdates{0};
    ::std::atomic<uint32> teleportCount{0};
    ::std::atomic<uint32> pathRecalculations{0};
    ::std::atomic<uint32> formationAdjustments{0};
    ::std::atomic<uint32> lostLeaderEvents{0};
    ::std::chrono::microseconds averageUpdateTime{0};
    ::std::chrono::microseconds maxUpdateTime{0};
    float averageDistance = 0.0f;
    float maxDeviation = 0.0f;

    void Reset()
    {
        positionUpdates = 0;
        teleportCount = 0;
        pathRecalculations = 0;
        formationAdjustments = 0;
        lostLeaderEvents = 0;
        averageUpdateTime = ::std::chrono::microseconds{0};
        maxUpdateTime = ::std::chrono::microseconds{0};
        averageDistance = 0.0f;
        maxDeviation = 0.0f;
    }
};

class TC_GAME_API LeaderFollowBehavior : public Strategy
{
public:
    LeaderFollowBehavior();
    ~LeaderFollowBehavior() = default;

    // Strategy interface implementation
    virtual void InitializeActions();
    virtual void InitializeTriggers();
    virtual void InitializeValues();
    virtual float GetRelevance(BotAI* ai) const;
    virtual void OnActivate(BotAI* ai);
    virtual void OnDeactivate(BotAI* ai);

    // Override UpdateBehavior for every-frame updates with no throttling
    virtual void UpdateBehavior(BotAI* ai, uint32 diff);

    // Main follow behavior update - now called from UpdateBehavior
    void UpdateFollowBehavior(BotAI* ai, uint32 diff);

    // Leader management
    bool SetFollowTarget(Player* leader);
    void ClearFollowTarget();
    Player* GetFollowTarget() const;
    bool HasFollowTarget() const { return _followTarget.player != nullptr; }

    // Position calculation
    Position CalculateFollowPosition(Player* leader, FormationRole role);
    Position CalculateFormationPosition(Player* leader, uint32 memberIndex, uint32 totalMembers);
    Position PredictLeaderPosition(Player* leader, float timeAhead);
    float CalculateOptimalDistance(Player* bot, Player* leader);

    // Movement control
    bool MoveToFollowPosition(BotAI* ai, const Position& targetPos);
    bool ShouldTeleportToLeader(Player* bot, Player* leader);
    bool TeleportToLeader(Player* bot, Player* leader);
    void AdjustMovementSpeed(Player* bot, float distanceToTarget);

    // Formation integration
    FormationRole DetermineFormationRole(Player* bot);
    bool UpdateFormationPosition(BotAI* ai);
    void SyncWithFormation(Group* group);
    FollowFormationPosition GetFormationPosition(FormationRole role);

    // State management
    FollowState GetFollowState() const { return _state; }
    void SetFollowState(FollowState state);
    bool IsFollowing() const { return _state == FollowState::FOLLOWING || _state == FollowState::COMBAT_FOLLOW; }
    bool IsInPosition(float tolerance = 3.0f) const;

    // Configuration
    void SetFollowMode(FollowMode mode);
    FollowMode GetFollowMode() const { return _config.mode; }
    void SetFollowDistance(float min, float max);
    void SetTeleportDistance(float distance) { _config.teleportDistance = distance; }
    void SetAutoTeleport(bool enable) { _config.autoTeleport = enable; }
    FollowConfig& GetConfig() { return _config; }

    // Line of sight and obstacles
    bool CheckLineOfSight(Player* bot, Player* leader);
    Position FindAlternativePosition(Player* bot, const Position& targetPos);
    bool IsPositionSafe(const Position& pos);
    void HandleObstacles(Player* bot, ::std::vector<Position>& path);

    // Combat following
    void UpdateCombatFollowing(BotAI* ai);
    Position CalculateCombatPosition(Player* bot, Player* leader, ::Unit* target);
    bool ShouldMaintainCombatDistance(Player* bot);
    void AdjustForThreat(Player* bot, float& distance);

    // Group coordination
    void CoordinateWithGroup(Group* group);
    bool IsGroupFormationActive() const;
    void HandleGroupMovement(BotAI* ai, Group* group);
    Position GetGroupAssignedPosition(Player* bot, Group* group);

    // Path management
    bool GenerateFollowPath(Player* bot, const Position& destination);
    void OptimizePath(::std::vector<Position>& path);
    bool ValidatePath(const ::std::vector<Position>& path);
    void SmoothPath(::std::vector<Position>& path);

    // Emergency handling
    void HandleLostLeader(BotAI* ai);
    void HandleStuckSituation(BotAI* ai);
    void EmergencyTeleport(Player* bot);
    bool AttemptReconnection(BotAI* ai);

    // Terrain adaptation
    void AdaptToTerrain(Player* bot, Position& targetPos);
    bool HandleWaterFollowing(Player* bot, Player* leader);
    bool HandleFlyingFollowing(Player* bot, Player* leader);
    bool HandleIndoorFollowing(Player* bot, Player* leader);

    // Performance monitoring
    FollowMetrics const& GetMetrics() const { return _metrics; }
    void ResetMetrics() { _metrics.Reset(); }
    float GetAverageDistance() const { return _metrics.averageDistance; }
    uint32 GetTeleportCount() const { return _metrics.teleportCount; }

    // Query methods
    float GetDistanceToLeader() const { return _followTarget.currentDistance; }
    bool IsLeaderMoving() const { return _followTarget.isMoving; }
    bool IsLeaderInSight() const { return _followTarget.inLineOfSight; }
    uint32 GetTimeSinceLastSeen() const { return _followTarget.lostDuration; }

    // Advanced features
    void EnablePredictiveFollowing(bool enable) { _usePredictiveFollowing = enable; }
    void SetFormationStrictness(float strictness) { _formationStrictness = strictness; }
    void EnableSmartPathing(bool enable) { _useSmartPathing = enable; }
    void SetResponseTime(uint32 ms) { _responseTime = ms; }

private:
    // Internal update methods
    void UpdateFollowTarget(Player* bot, Player* leader);
    void UpdateMovement(BotAI* ai);
    void UpdateFormation(BotAI* ai);
    void UpdateCombatPosition(BotAI* ai);

    // Position calculation helpers
    Position CalculateBasePosition(Player* leader, float angle, float distance);
    Position ApplyFormationOffset(const Position& basePos, FormationRole role);
    float CalculateAngleForRole(FormationRole role, uint32 index, uint32 total);
    float NormalizeAngle(float angle);

    // Movement helpers
    bool StartMovement(Player* bot, const Position& destination);
    void StopMovement(Player* bot);
    float GetMovementSpeed(Player* bot);
    bool IsMoving(Player* bot);

    // State transition helpers
    void TransitionToState(FollowState newState);
    bool CanTransitionTo(FollowState newState) const;
    void HandleStateTransition(FollowState oldState, FollowState newState);

    // Distance and timing helpers
    float CalculateDistance2D(const Position& pos1, const Position& pos2);
    float CalculateDistance3D(const Position& pos1, const Position& pos2);
    bool IsWithinRange(float distance, float min, float max);
    uint32 GetTimeSince(uint32 timestamp);

    // Role-based helpers
    float GetRoleBasedAngle(FormationRole role);
    float GetRoleBasedDistance(FormationRole role) const;

    // Performance tracking
    void TrackPerformance(::std::chrono::microseconds duration, const ::std::string& operation);
    void UpdateAverages();

private:
    // Core components
    FollowTarget _followTarget;
    FollowState _state = FollowState::IDLE;
    FollowConfig _config;

    // Formation data
    FormationRole _formationRole = FormationRole::SUPPORT;
    FollowFormationPosition _formationPos;
    bool _useFormation = true;
    float _formationStrictness = 0.8f;

    // Path data
    ::std::vector<Position> _currentPath;
    uint32 _currentPathIndex = 0;
    bool _pathGenerated = false;

    // Timing
    uint32 _lastUpdate = 0;
    uint32 _lastTeleport = 0;
    uint32 _lastPathGeneration = 0;
    uint32 _stuckCheckTimer = 0;
    uint32 _responseTime = 250; // ms

    // Position tracking
    Position _lastPosition;
    Position _targetPosition;
    float _lastDistance = 0.0f;
    uint32 _stationaryTime = 0;

    // Behavior flags
    bool _usePredictiveFollowing = true;
    bool _useSmartPathing = true;
    bool _isStuck = false;
    bool _needsNewPath = false;

    // Group coordination
    Group* _currentGroup = nullptr;
    uint32 _groupPosition = 0;
    bool _syncedWithGroup = false;

    // Performance metrics
    mutable FollowMetrics _metrics;

    // Movement optimization
    uint32 _movementOptimizationTimer = 0;
    float _currentSpeedModifier = 1.0f;

    // Constants
    static constexpr float MIN_FOLLOW_DISTANCE = 3.0f;
    static constexpr float MAX_FOLLOW_DISTANCE = 30.0f;
    static constexpr float DEFAULT_FOLLOW_DISTANCE = 10.0f;
    static constexpr float TELEPORT_DISTANCE = 100.0f;
    static constexpr float COMBAT_FOLLOW_DISTANCE = 15.0f;
    // REFACTORED: Removed UPDATE_INTERVAL - now runs every frame
    static constexpr uint32 STUCK_CHECK_INTERVAL = 2000;  // ms
    static constexpr uint32 PATH_UPDATE_INTERVAL = 1000;  // ms
    static constexpr uint32 LOST_LEADER_TIMEOUT = 5000;   // ms
    static constexpr float CATCHUP_SPEED_MODIFIER = 1.3f;
    static constexpr float POSITION_TOLERANCE = 2.0f;
};

// Follow behavior factory for creating specialized follow strategies
class TC_GAME_API FollowBehaviorFactory
{
public:
    static ::std::unique_ptr<LeaderFollowBehavior> CreateFollowBehavior(FollowMode mode);
    static ::std::unique_ptr<LeaderFollowBehavior> CreateRoleBasedFollowBehavior(FormationRole role);
    static ::std::unique_ptr<LeaderFollowBehavior> CreateCombatFollowBehavior();
    static ::std::unique_ptr<LeaderFollowBehavior> CreateFormationFollowBehavior(MovementFormationType formation);
};

// Utility functions for follow behavior
class TC_GAME_API FollowBehaviorUtils
{
public:
    // Distance calculations
    static float CalculateOptimalFollowDistance(Player* bot, Player* leader, FollowMode mode);
    static bool IsInFollowRange(Player* bot, Player* leader, float minDist, float maxDist);
    static float GetCombatFollowDistance(uint8 botClass, uint8 botSpec);

    // Position predictions
    static Position PredictMovement(Unit* unit, float timeAhead);
    static Position InterpolatePosition(const Position& start, const Position& end, float factor);
    static float EstimateArrivalTime(Player* bot, const Position& destination);

    // Formation utilities
    static FormationRole GetOptimalFormationRole(Player* bot);
    static float GetRoleBasedDistance(FormationRole role);
    static float GetRoleBasedAngle(FormationRole role);

    // Path validation
    static bool IsPathClear(Player* bot, const Position& destination);
    static bool RequiresTeleport(Player* bot, Player* leader);
    static ::std::vector<Position> SimplifyPath(const ::std::vector<Position>& path);

    // Group coordination
    static Position CalculateGroupCenterPosition(Group* group);
    static bool IsGroupMoving(Group* group);
    static float GetGroupSpread(Group* group);
};

} // namespace Playerbot