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
#include <unordered_map>
#include <vector>
#include <memory>
#include <atomic>
#include <shared_mutex>
#include <chrono>

// Forward declarations
class Player;
class Unit;
class Spell;

namespace Playerbot
{

// Kiting behavior types
enum class KitingType : uint8
{
    NONE = 0,               // No kiting
    BASIC_KITING = 1,       // Simple distance maintenance
    CIRCULAR_KITING = 2,    // Circular movement around target
    LINE_KITING = 3,        // Straight line retreat while attacking
    FIGURE_EIGHT = 4,       // Figure-8 pattern kiting
    SPIRAL_KITING = 5,      // Spiral movement pattern
    TACTICAL_RETREAT = 6,   // Strategic withdrawal
    HIT_AND_RUN = 7,        // Quick strikes with retreats
    STUTTER_STEP = 8,       // Short movement bursts
    ZIGZAG_KITING = 9,      // Zigzag evasion pattern
    OBSTACLE_KITING = 10    // Using terrain/obstacles
};

// Kiting priority levels
enum class KitingPriority : uint8
{
    CRITICAL = 0,           // Emergency kiting required
    HIGH = 1,               // High priority kiting
    MODERATE = 2,           // Standard kiting
    LOW = 3,                // Optional kiting
    NONE = 4                // No kiting needed
};

// Kiting state machine
enum class KitingState : uint8
{
    INACTIVE = 0,           // Not kiting
    EVALUATING = 1,         // Assessing kiting need
    POSITIONING = 2,        // Moving to kiting position
    KITING = 3,             // Actively kiting
    ATTACKING = 4,          // Attack phase during kiting
    RETREATING = 5,         // Retreat phase
    REPOSITIONING = 6,      // Adjusting position
    EMERGENCY_ESCAPE = 7    // Emergency escape mode
};

// Kiting trigger conditions
enum class KitingTrigger : uint32
{
    NONE = 0x00000000,
    DISTANCE_TOO_CLOSE = 0x00000001,        // Target too close
    LOW_HEALTH = 0x00000002,                // Bot health low
    CASTING_INTERRUPT = 0x00000004,         // Need to avoid interrupts
    MULTIPLE_ENEMIES = 0x00000008,          // Multiple threats
    TERRAIN_ADVANTAGE = 0x00000010,         // Favorable terrain
    COOLDOWN_MANAGEMENT = 0x00000020,       // Managing ability cooldowns
    RESOURCE_MANAGEMENT = 0x00000040,       // Mana/energy management
    FORMATION_ROLE = 0x00000080,            // Group role requires kiting
    THREAT_MANAGEMENT = 0x00000100,         // Threat level management
    ENVIRONMENTAL_HAZARD = 0x00000200,      // Avoiding hazards

    // Common combinations
    DEFENSIVE = LOW_HEALTH | MULTIPLE_ENEMIES | ENVIRONMENTAL_HAZARD,
    TACTICAL = COOLDOWN_MANAGEMENT | RESOURCE_MANAGEMENT | TERRAIN_ADVANTAGE,
    EMERGENCY = DISTANCE_TOO_CLOSE | LOW_HEALTH | CASTING_INTERRUPT
};

DEFINE_ENUM_FLAG(KitingTrigger);

// Kiting target information
struct KitingTarget
{
    ObjectGuid guid;
    Unit* unit;
    Position position;
    Position velocity;
    Position predictedPosition;
    float distance;
    float relativeSpeed;
    float threatLevel;
    bool isMoving;
    bool isCharging;
    bool isCasting;
    uint32 lastUpdate;
    ::std::string name;

    KitingTarget() : unit(nullptr), distance(0.0f), relativeSpeed(0.0f),
                    threatLevel(0.0f), isMoving(false), isCharging(false),
                    isCasting(false), lastUpdate(0) {}
};

// Kiting movement pattern
struct KitingPattern
{
    KitingType type;
    ::std::vector<Position> waypoints;
    float optimalDistance;
    float minDistance;
    float maxDistance;
    float movementSpeed;
    float attackWindow;         // Time window for attacks
    float movementWindow;       // Time window for movement
    bool maintainLoS;           // Maintain line of sight
    bool useObstacles;          // Utilize terrain obstacles
    uint32 patternDuration;     // How long to maintain pattern
    ::std::string description;

    KitingPattern() : type(KitingType::BASIC_KITING), optimalDistance(20.0f),
                     minDistance(15.0f), maxDistance(30.0f), movementSpeed(0.0f),
                     attackWindow(2.0f), movementWindow(1.0f), maintainLoS(true),
                     useObstacles(false), patternDuration(0) {}
};

// Kiting context for decision making
struct KitingContext
{
    Player* bot;
    ::std::vector<Unit*> threats;
    Unit* primaryTarget;
    Position currentPosition;
    Position safeDirection;
    float currentHealth;
    float currentMana;
    bool inCombat;
    bool isMoving;
    bool isCasting;
    KitingTrigger triggers;
    float availableSpace;       // Available space for kiting
    ::std::vector<Position> obstacles;
    ::std::vector<Player*> groupMembers;

    KitingContext() : bot(nullptr), primaryTarget(nullptr), currentHealth(100.0f),
                     currentMana(100.0f), inCombat(false), isMoving(false),
                     isCasting(false), triggers(KitingTrigger::NONE),
                     availableSpace(50.0f) {}
};

// Kiting execution result
struct KitingResult
{
    bool success;
    KitingType usedType;
    Position nextPosition;
    Position attackPosition;
    float estimatedDuration;
    float safetyImprovement;
    bool requiresSprint;
    bool requiresJump;
    bool breaksFormation;
    ::std::string failureReason;
    ::std::vector<Position> alternativePositions;

    KitingResult() : success(false), usedType(KitingType::NONE),
                    estimatedDuration(0.0f), safetyImprovement(0.0f),
                    requiresSprint(false), requiresJump(false),
                    breaksFormation(false) {}
};

// Kiting performance metrics
struct KitingMetrics
{
    ::std::atomic<uint32> kitingActivations{0};
    ::std::atomic<uint32> successfulKites{0};
    ::std::atomic<uint32> failedKites{0};
    ::std::atomic<uint32> emergencyEscapes{0};
    ::std::atomic<uint32> damageAvoided{0};
    ::std::chrono::microseconds averageKitingDuration{0};
    ::std::chrono::microseconds maxKitingDuration{0};
    float averageDistanceMaintained{0.0f};
    float optimalDistanceRatio{0.0f};
    ::std::chrono::steady_clock::time_point lastUpdate;

    void Reset()
    {
        kitingActivations = 0;
        successfulKites = 0;
        failedKites = 0;
        emergencyEscapes = 0;
        damageAvoided = 0;
        averageKitingDuration = ::std::chrono::microseconds{0};
        maxKitingDuration = ::std::chrono::microseconds{0};
        averageDistanceMaintained = 0.0f;
        optimalDistanceRatio = 0.0f;
        lastUpdate = ::std::chrono::steady_clock::now();
    }

    float GetSuccessRate() const
    {
        uint32 total = kitingActivations.load();
        return total > 0 ? static_cast<float>(successfulKites.load()) / total : 0.0f;
    }
};

class TC_GAME_API KitingManager
{
public:
    explicit KitingManager(Player* bot);
    ~KitingManager() = default;

    // Primary kiting interface
    void UpdateKiting(uint32 diff);
    KitingResult EvaluateKitingNeed(const KitingContext& context);
    KitingResult ExecuteKiting(const KitingContext& context);
    void StopKiting();

    // Kiting pattern selection
    KitingType SelectOptimalKitingType(const KitingContext& context);
    KitingPattern GenerateKitingPattern(KitingType type, const KitingContext& context);
    ::std::vector<KitingPattern> GetAvailablePatterns(const KitingContext& context);

    // Distance management
    bool ShouldMaintainDistance(Unit* target);
    float GetOptimalKitingDistance(Unit* target);
    float GetMinimumSafeDistance(Unit* target);
    bool IsAtOptimalKitingDistance(Unit* target);

    // Movement pattern execution
    KitingResult ExecuteCircularKiting(const KitingContext& context);
    KitingResult ExecuteLineKiting(const KitingContext& context);
    KitingResult ExecuteStutterStep(const KitingContext& context);
    KitingResult ExecuteHitAndRun(const KitingContext& context);
    KitingResult ExecuteFigureEight(const KitingContext& context);

    // Target analysis
    ::std::vector<KitingTarget> AnalyzeThreats(const ::std::vector<Unit*>& enemies);
    KitingPriority AssessKitingPriority(Unit* target);
    bool IsKiteable(Unit* target);
    float CalculateKitingEfficiency(Unit* target, KitingType type);

    // Positioning calculations
    Position CalculateKitingPosition(Unit* target, KitingType type);
    Position FindSafeKitingDirection(const ::std::vector<Unit*>& threats);
    Position GetCircularKitingPosition(Unit* target, float angle);
    Position GetRetreatPosition(const ::std::vector<Unit*>& threats, float distance);

    // Attack timing integration
    bool CanAttackWhileKiting();
    float GetAttackWindow(KitingType type);
    Position GetAttackPosition(Unit* target);
    bool ShouldStopToAttack(Unit* target);

    // Environmental awareness
    void RegisterObstacle(const Position& obstacle, float radius);
    void ClearObstacles();
    bool CanKiteInDirection(const Position& direction, float distance);
    Position AdjustKitingForTerrain(const Position& intended);

    // Emergency kiting
    void ActivateEmergencyKiting(const ::std::vector<Unit*>& threats);
    void DeactivateEmergencyKiting();
    bool IsEmergencyKitingActive() const { return _emergencyKiting; }
    Position FindEmergencyEscapeRoute(const ::std::vector<Unit*>& threats);

    // Group coordination
    bool WillKitingBreakFormation();
    Position AdjustKitingForGroup(const Position& intended, const ::std::vector<Player*>& group);
    void CoordinateKitingWithGroup(const ::std::vector<Player*>& group);

    // State management
    KitingState GetCurrentState() const { return _currentState; }
    KitingType GetCurrentType() const { return _currentKitingType; }
    bool IsKitingActive() const { return _kitingActive; }
    Unit* GetKitingTarget() const { return _kitingTarget; }

    // Configuration
    void SetOptimalDistance(float distance) { _optimalKitingDistance = distance; }
    float GetOptimalDistance() const { return _optimalKitingDistance; }
    void SetMinDistance(float distance) { _minKitingDistance = distance; }
    float GetMinDistance() const { return _minKitingDistance; }
    void SetUpdateInterval(uint32 interval) { _updateInterval = interval; }
    uint32 GetUpdateInterval() const { return _updateInterval; }

    // Performance monitoring
    KitingMetrics const& GetMetrics() const { return _metrics; }
    void ResetMetrics() { _metrics.Reset(); }

    // Advanced features
    void EnablePredictiveKiting(bool enable) { _predictiveKiting = enable; }
    bool IsPredictiveKitingEnabled() const { return _predictiveKiting; }
    void SetKitingAggressiveness(float aggressiveness) { _kitingAggressiveness = aggressiveness; }
    float GetKitingAggressiveness() const { return _kitingAggressiveness; }

private:
    // Core kiting logic
    KitingTrigger EvaluateKitingTriggers(const KitingContext& context);
    bool ShouldActivateKiting(const KitingContext& context);
    void UpdateKitingState();
    void ExecuteCurrentPattern();

    // Pattern generation methods
    ::std::vector<Position> GenerateCircularWaypoints(Unit* target, float radius, uint32 points);
    ::std::vector<Position> GenerateLineWaypoints(const Position& start, const Position& direction, float distance);
    ::std::vector<Position> GenerateFigureEightWaypoints(Unit* target, float radius);
    ::std::vector<Position> GenerateZigzagWaypoints(const Position& start, const Position& direction, float distance);

    // Movement execution
    bool ExecuteMovementToPosition(const Position& target);
    void AdjustMovementSpeed();
    bool RequiresSprintForKiting();
    void HandleMovementFailure();

    // Attack integration
    void UpdateAttackTiming();
    bool IsInAttackWindow();
    void HandleAttackOpportunity();
    void SynchronizeAttackWithMovement();

    // Safety validation
    bool IsPositionSafe(const Position& pos, const ::std::vector<Unit*>& threats);
    float CalculateSafetyRating(const Position& pos, const ::std::vector<Unit*>& threats);
    bool ValidateKitingPath(const ::std::vector<Position>& waypoints);

    // Prediction and optimization
    void UpdateThreatPredictions();
    Position PredictTargetPosition(Unit* target, float timeAhead);
    void OptimizeKitingPath();
    void AdaptPatternToSituation();

    // Performance tracking
    void TrackPerformance(::std::chrono::microseconds duration, const ::std::string& operation);
    void UpdateKitingStatistics();

    // Utility methods
    float CalculateRelativeSpeed(Unit* target);
    bool HasLineOfSightForKiting(const Position& from, const Position& to);
    float GetTerrainDifficulty(const Position& pos);

private:
    Player* _bot;
    Unit* _kitingTarget;

    // Kiting state
    bool _kitingActive;
    KitingState _currentState;
    KitingType _currentKitingType;
    KitingPattern _currentPattern;
    ::std::vector<KitingTarget> _trackedTargets;

    // Movement state
    Position _currentKitingPosition;
    Position _targetKitingPosition;
    ::std::vector<Position> _kitingWaypoints;
    uint32 _currentWaypointIndex;
    uint32 _lastMovementTime;

    // Attack timing
    uint32 _lastAttackTime;
    uint32 _attackWindowStart;
    uint32 _attackWindowEnd;
    bool _inAttackWindow;

    // Configuration
    float _optimalKitingDistance;
    float _minKitingDistance;
    float _maxKitingDistance;
    uint32 _updateInterval;
    float _kitingAggressiveness;
    bool _predictiveKiting;
    bool _emergencyKiting;

    // Environmental data
    ::std::vector<Position> _obstacles;
    float _availableKitingSpace;
    uint32 _lastObstacleUpdate;

    // Performance metrics
    mutable KitingMetrics _metrics;
    uint32 _kitingStartTime;

    // Thread safety
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _mutex;

    // Constants
    static constexpr float DEFAULT_OPTIMAL_DISTANCE = 20.0f;    // 20 yards
    static constexpr float DEFAULT_MIN_DISTANCE = 15.0f;       // 15 yards
    static constexpr float DEFAULT_MAX_DISTANCE = 35.0f;       // 35 yards
    static constexpr uint32 DEFAULT_UPDATE_INTERVAL = 200;     // 200ms
    static constexpr float DEFAULT_AGGRESSIVENESS = 0.7f;      // 70% aggressive
    static constexpr uint32 ATTACK_WINDOW_DURATION = 2000;     // 2 seconds
    static constexpr uint32 MOVEMENT_WINDOW_DURATION = 1000;   // 1 second
    static constexpr uint32 WAYPOINT_RECALC_INTERVAL = 3000;   // 3 seconds
};

// Kiting utilities
class TC_GAME_API KitingUtils
{
public:
    // Distance and positioning utilities
    static float CalculateOptimalKitingDistance(Player* bot, Unit* target);
    static Position FindBestKitingDirection(Player* bot, const ::std::vector<Unit*>& threats);
    static bool IsPositionGoodForKiting(const Position& pos, Player* bot, const ::std::vector<Unit*>& threats);
    static float CalculateKitingEfficiency(Player* bot, Unit* target, const Position& kitingPos);

    // Movement pattern utilities
    static ::std::vector<Position> InterpolateMovementPath(const Position& start, const Position& end, uint32 points);
    static Position CalculateInterceptionPoint(const Position& target, const Position& targetVel, const Position& interceptor, float interceptorSpeed);
    static bool WillPathIntersectThreat(const ::std::vector<Position>& path, Unit* threat, float safetyRadius);

    // Timing and synchronization utilities
    static float CalculateMovementTime(Player* bot, const Position& destination);
    static float CalculateCastTime(Player* bot, uint32 spellId);
    static bool CanCompleteActionBeforeContact(Player* bot, Unit* threat, float actionDuration);
    static float EstimateTimeToReachPlayer(Unit* threat, Player* player);

    // Terrain and environment utilities
    static bool IsTerrainSuitableForKiting(const Position& center, float radius);
    static ::std::vector<Position> FindKitingObstacles(const Position& center, float radius);
    static Position AdjustPositionForTerrain(const Position& intended, float searchRadius = 5.0f);
    static bool HasEscapeRoutes(const Position& pos, uint32 routeCount = 3);

    // Class-specific utilities
    static KitingType GetOptimalKitingTypeForClass(uint8 playerClass);
    static float GetClassKitingRange(uint8 playerClass);
    static bool CanClassKiteEffectively(uint8 playerClass);
    static ::std::vector<uint32> GetKitingSpells(uint8 playerClass);

    // Group coordination utilities
    static bool IsKitingCompatibleWithGroupRole(Player* bot);
    static Position AdjustKitingForGroupFormation(const Position& intended, const ::std::vector<Player*>& group);
    static bool WillKitingDisruptGroup(Player* bot, const Position& kitingPos, const ::std::vector<Player*>& group);

    // Safety assessment utilities
    static float CalculatePositionSafety(const Position& pos, const ::std::vector<Unit*>& threats);
    static bool IsPositionTrappable(const Position& pos, const ::std::vector<Unit*>& threats);
    static ::std::vector<Position> FindEscapeRoutes(const Position& current, const ::std::vector<Unit*>& threats);
    static Position FindSafestNearbyPosition(const Position& current, const ::std::vector<Unit*>& threats, float radius);
};

} // namespace Playerbot