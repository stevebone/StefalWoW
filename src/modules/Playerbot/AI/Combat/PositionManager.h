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
#include "ObjectGuid.h"
#include "Position.h"
#include "BotThreatManager.h"
#include "EnumFlag.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <atomic>
#include <shared_mutex>
#include <chrono>
#include "../../Movement/Core/MovementTypes.h"

// Forward declarations
class Player;
class Unit;
class Map;

namespace Playerbot
{

// DUPLICATE ENUMS REMOVED - Using definitions from IUnifiedMovementCoordinator.h
// enum class PositionType : uint8 { ... } - REMOVED
// enum class MovementPriority : uint8 { ... } - REMOVED
// Include at line 21 provides these definitions

// Position validation flags
enum class PositionValidation : uint32
{
    NONE = 0x00000000,
    WALKABLE = 0x00000001,
    LINE_OF_SIGHT = 0x00000002,
    SAFE_DISTANCE = 0x00000004,
    NO_OBSTACLES = 0x00000008,
    IN_COMBAT_RANGE = 0x00000010,
    AVOID_AOE = 0x00000020,
    GROUP_PROXIMITY = 0x00000040,
    ESCAPE_ROUTE = 0x00000080,
    OPTIMAL_ANGLE = 0x00000100,
    STABLE_GROUND = 0x00000200,
    AVOID_PATROL = 0x00000400,
    SPELL_RANGE = 0x00000800,
    MELEE_REACH = 0x00001000,

    // Common validation combinations
    BASIC = WALKABLE | NO_OBSTACLES | STABLE_GROUND,
    COMBAT = BASIC | LINE_OF_SIGHT | IN_COMBAT_RANGE,
    SAFE = COMBAT | SAFE_DISTANCE | AVOID_AOE,
    OPTIMAL = SAFE | OPTIMAL_ANGLE | ESCAPE_ROUTE
};

DEFINE_ENUM_FLAG(PositionValidation);

// Position scoring weights
struct PositionWeights
{
    float distanceWeight = 1.0f;
    float safetyWeight = 2.0f;
    float losWeight = 1.5f;
    float angleWeight = 0.8f;
    float groupWeight = 1.2f;
    float escapeWeight = 1.0f;
    float optimalRangeWeight = 1.3f;
    float movementCostWeight = 0.5f;

    // Role-specific weight modifiers
    float tankFrontWeight = 2.0f;
    float meleeBackWeight = 1.5f;
    float rangedSpreadWeight = 1.2f;
    float healerSafetyWeight = 2.0f;
};

// Position evaluation result
struct PositionInfo
{
    Position position;
    float score;
    PositionType type;
    MovementPriority priority;
    float distanceToTarget;
    float safetyRating;
    bool hasLineOfSight;
    bool isOptimalRange;
    float movementCost;
    ::std::string reason;
    uint32 evaluationTime;

    PositionInfo() : score(0.0f), type(PositionType::MELEE_COMBAT), priority(MovementPriority::PRIORITY_NONE),
                    distanceToTarget(0.0f), safetyRating(0.0f), hasLineOfSight(false),
                    isOptimalRange(false), movementCost(0.0f), evaluationTime(0) {}

    bool operator>(const PositionInfo& other) const
    {
        if (priority != other.priority)
            return priority < other.priority;  // Lower enum = higher priority
        return score > other.score;
    }
};

// Movement context for positioning decisions
struct MovementContext
{
    Player* bot;
    Unit* target;
    Unit* primaryThreat;
    ::std::vector<Unit*> nearbyEnemies;
    ::std::vector<Player*> groupMembers;
    PositionType desiredType;
    ThreatRole botRole;
    float preferredRange;
    float maxRange;
    bool inCombat;
    bool emergencyMode;
    PositionValidation validationFlags;
    PositionWeights weights;
    ::std::vector<Position> avoidZones;  // Areas to avoid (fire, aoe, etc.)

    MovementContext() : bot(nullptr), target(nullptr), primaryThreat(nullptr),
                       desiredType(PositionType::MELEE_COMBAT), botRole(ThreatRole::DPS),
                       preferredRange(5.0f), maxRange(40.0f), inCombat(false),
                       emergencyMode(false), validationFlags(PositionValidation::BASIC) {}
};

// Position-specific movement execution result
struct PositionMovementResult
{
    bool success;
    Position targetPosition;
    Position currentPosition;
    MovementPriority priority;
    float estimatedTime;
    float pathDistance;
    ::std::string failureReason;
    ::std::vector<Position> waypoints;
    bool requiresJump;
    bool requiresSprint;

    PositionMovementResult() : success(false), priority(MovementPriority::PRIORITY_NONE),
                      estimatedTime(0.0f), pathDistance(0.0f),
                      requiresJump(false), requiresSprint(false) {}
};

// Area of Effect danger zone
struct AoEZone
{
    Position center;
    float radius;
    uint32 spellId;
    uint32 startTime;
    uint32 duration;
    float damageRating;
    bool isActive;

    AoEZone() : radius(0.0f), spellId(0), startTime(0), duration(0),
               damageRating(0.0f), isActive(false) {}

    bool IsPositionInDanger(const Position& pos, uint32 currentTime) const
    {
        if (!isActive || currentTime > startTime + duration)
            return false;
        return center.GetExactDist(&pos) <= radius;
    }
};

// Performance metrics for positioning
struct PositionMetrics
{
    ::std::atomic<uint32> positionEvaluations{0};
    ::std::atomic<uint32> movementCommands{0};
    ::std::atomic<uint32> emergencyMoves{0};
    ::std::atomic<uint32> pathfindingCalls{0};
    ::std::chrono::microseconds averageEvaluationTime{0};
    ::std::chrono::microseconds maxEvaluationTime{0};
    ::std::chrono::steady_clock::time_point lastUpdate;

    void Reset()
    {
        positionEvaluations = 0;
        movementCommands = 0;
        emergencyMoves = 0;
        pathfindingCalls = 0;
        averageEvaluationTime = ::std::chrono::microseconds{0};
        maxEvaluationTime = ::std::chrono::microseconds{0};
        lastUpdate = ::std::chrono::steady_clock::now();
    }
};

class TC_GAME_API PositionManager
{
public:
    explicit PositionManager(Player* bot, BotThreatManager* threatManager);
    ~PositionManager() = default;

    // Main positioning interface
    PositionMovementResult UpdatePosition(const MovementContext& context);
    PositionMovementResult FindOptimalPosition(const MovementContext& context);
    PositionMovementResult ExecuteMovement(const Position& targetPos, MovementPriority priority);

    // Position evaluation
    PositionInfo EvaluatePosition(const Position& pos, const MovementContext& context);
    ::std::vector<PositionInfo> EvaluatePositions(const ::std::vector<Position>& positions, const MovementContext& context);
    ::std::vector<Position> GenerateCandidatePositions(const MovementContext& context);

    // Range and angle management
    Position FindRangePosition(Unit* target, float minRange, float maxRange, float preferredAngle = 0.0f);
    Position FindMeleePosition(Unit* target, bool preferBehind = true);
    Position FindRangedPosition(Unit* target, float preferredRange = 25.0f);
    Position FindHealingPosition(const ::std::vector<Player*>& allies);
    Position FindKitingPosition(Unit* threat, float minDistance = 15.0f);

    // Role-specific positioning
    Position FindTankPosition(Unit* target);
    Position FindDpsPosition(Unit* target, PositionType type = PositionType::MELEE_COMBAT);
    Position FindHealerPosition(const ::std::vector<Player*>& groupMembers);
    Position FindSupportPosition(const ::std::vector<Player*>& groupMembers);

    // Safety and avoidance
    bool IsPositionSafe(const Position& pos, const MovementContext& context);
    bool IsInDangerZone(const Position& pos);
    Position FindSafePosition(const Position& fromPos, float minDistance = 10.0f);
    Position FindEscapePosition(const ::std::vector<Unit*>& threats);

    // AoE and hazard management
    void RegisterAoEZone(const AoEZone& zone);
    void UpdateAoEZones(uint32 currentTime);
    void ClearExpiredZones(uint32 currentTime);
    ::std::vector<AoEZone> GetActiveZones() const;

    // Validation and pathfinding
    bool ValidatePosition(const Position& pos, PositionValidation flags);
    bool HasLineOfSight(const Position& from, const Position& to);
    bool IsWalkablePosition(const Position& pos);
    float CalculateMovementCost(const Position& from, const Position& to);

    // Group coordination
    Position FindFormationPosition(const ::std::vector<Player*>& groupMembers, PositionType formationType);
    bool ShouldMaintainGroupProximity();
    float GetOptimalGroupDistance(ThreatRole role);

    // Advanced movement features
    bool ShouldStrafe(Unit* target);
    bool ShouldCircleStrafe(Unit* target);
    Position CalculateStrafePosition(Unit* target, bool strafeLeft = true);
    Position PredictTargetPosition(Unit* target, float timeAhead);

    // Configuration
    void SetWeights(const PositionWeights& weights) { _weights = weights; }
    PositionWeights const& GetWeights() const { return _weights; }
    void SetUpdateInterval(uint32 intervalMs) { _updateInterval = intervalMs; }
    uint32 GetUpdateInterval() const { return _updateInterval; }

    // Performance monitoring
    PositionMetrics const& GetMetrics() const { return _metrics; }
    void ResetMetrics() { _metrics.Reset(); }

    // Emergency response
    PositionMovementResult HandleEmergencyMovement(const MovementContext& context);
    bool IsInEmergencyPosition();
    Position FindEmergencyEscapePosition();

    // Position history and learning
    void RecordPositionSuccess(const Position& pos, PositionType type);
    void RecordPositionFailure(const Position& pos, const ::std::string& reason);
    float GetPositionSuccessRate(const Position& pos, float radius = 5.0f);

private:
    // Internal evaluation methods
    float CalculateDistanceScore(const Position& pos, const MovementContext& context);
    float CalculateSafetyScore(const Position& pos, const MovementContext& context);
    float CalculateLineOfSightScore(const Position& pos, const MovementContext& context);
    float CalculateAngleScore(const Position& pos, const MovementContext& context);
    float CalculateGroupScore(const Position& pos, const MovementContext& context);
    float CalculateEscapeScore(const Position& pos, const MovementContext& context);

    // Utility methods
    float GetOptimalRange(PositionType type, ThreatRole role);
    float GetOptimalAngle(PositionType type, Unit* target);
    ::std::vector<Position> GenerateCircularPositions(const Position& center, float radius, uint32 count);
    ::std::vector<Position> GenerateArcPositions(const Position& center, float radius, float startAngle, float endAngle, uint32 count);

    // Safety analysis
    float AnalyzeThreatLevel(const Position& pos, const ::std::vector<Unit*>& threats);
    float CalculateAoEThreat(const Position& pos);
    bool IsNearPatrolRoute(const Position& pos);

    // Movement execution helpers
    bool CanReachPosition(const Position& pos);
    ::std::vector<Position> CalculateWaypoints(const Position& from, const Position& to);
    float EstimateMovementTime(const Position& from, const Position& to);

    // Performance tracking
    void TrackPerformance(::std::chrono::microseconds duration, const ::std::string& operation);

private:
    Player* _bot;
    BotThreatManager* _threatManager;
    PositionWeights _weights;

    // Configuration
    uint32 _updateInterval;
    uint32 _lastUpdate;
    float _positionTolerance;
    uint32 _maxCandidates;

    // AoE and hazard tracking
    ::std::vector<AoEZone> _activeZones;
    uint32 _lastZoneUpdate;

    // Position history for learning
    ::std::unordered_map<::std::string, float> _positionSuccessRates;
    ::std::unordered_map<::std::string, uint32> _positionAttempts;

    // Performance metrics
    mutable PositionMetrics _metrics;

    // Movement state tracking (for duplicate prevention - Fix #1)
    Position _lastTargetPosition;
    uint32 _lastMovePointTime;

    // Thread safety
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _mutex;

    // Constants
    static constexpr uint32 DEFAULT_UPDATE_INTERVAL = 250;   // 250ms
    static constexpr float POSITION_TOLERANCE = 1.5f;       // 1.5 yards
    static constexpr uint32 MAX_CANDIDATES = 36;            // 36 position candidates
    static constexpr float EMERGENCY_DISTANCE = 20.0f;     // 20 yards emergency escape
    static constexpr uint32 ZONE_CLEANUP_INTERVAL = 5000;   // 5 seconds
};

// Position calculation utilities
class TC_GAME_API PositionUtils
{
public:
    // Geometric calculations
    static Position CalculatePositionAtAngle(const Position& center, float distance, float angle);
    static float CalculateAngleBetween(const Position& from, const Position& to);
    static float NormalizeAngle(float angle);
    static bool IsAngleInRange(float angle, float target, float tolerance);

    // Range and distance utilities
    static bool IsInMeleeRange(Player* bot, Unit* target);
    static bool IsInSpellRange(Player* bot, Unit* target, uint32 spellId);
    static bool IsInOptimalRange(Player* bot, Unit* target, PositionType type);
    static float GetMinRange(Player* bot, Unit* target);
    static float GetMaxRange(Player* bot, Unit* target);

    // Safety and validation utilities
    static bool IsPositionSafeFromAoE(const Position& pos, const ::std::vector<AoEZone>& zones);
    static bool IsPositionInWater(const Position& pos, Map* map);
    static bool IsPositionOnGround(const Position& pos, Map* map);
    static float GetGroundHeight(const Position& pos, Map* map);

    // Pathfinding utilities
    static bool CanWalkStraightLine(const Position& from, const Position& to, Map* map);
    static ::std::vector<Position> SmoothPath(const ::std::vector<Position>& rawPath);
    static Position GetNearestWalkablePosition(const Position& pos, Map* map, float searchRadius = 10.0f);

    // Group positioning utilities
    static Position CalculateGroupCenter(const ::std::vector<Player*>& players);
    static float CalculateGroupSpread(const ::std::vector<Player*>& players);
    static bool IsGroupTooSpread(const ::std::vector<Player*>& players, float maxDistance = 30.0f);
};

} // namespace Playerbot