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
#include <queue>

// Forward declarations
class Player;
class Unit;
class GameObject;
class WorldObject;
class Map;

namespace Playerbot
{

// Obstacle types for classification
enum class ObstacleType : uint8
{
    STATIC_TERRAIN = 0,     // Mountains, walls, permanent terrain
    DYNAMIC_OBJECT = 1,     // Doors, elevators, moving platforms
    UNIT_OBSTACLE = 2,      // Other players, NPCs, pets
    TEMPORARY_HAZARD = 3,   // Fire patches, poison clouds, AoE effects
    ENVIRONMENTAL = 4,      // Water, lava, environmental hazards
    PROJECTILE = 5,         // Moving projectiles, charges
    INTERACTIVE = 6,        // Quest objects, interactable items
    FORMATION = 7           // Group formation obstacles
};

// Obstacle priority levels
enum class ObstaclePriority : uint8
{
    CRITICAL = 0,           // Immediate collision avoidance required
    HIGH = 1,               // High priority avoidance
    MODERATE = 2,           // Standard avoidance
    LOW = 3,                // Minor obstacle
    IGNORE = 4              // Can be ignored safely
};

// Avoidance behavior types
enum class AvoidanceBehavior : uint8
{
    DIRECT_AVOIDANCE = 0,   // Direct path around obstacle
    WAIT_AND_PASS = 1,      // Wait for obstacle to move
    FORCE_THROUGH = 2,      // Push through if possible
    FIND_ALTERNATIVE = 3,   // Find completely different route
    EMERGENCY_STOP = 4,     // Stop immediately
    BACKTRACK = 5,          // Reverse and find new path
    CIRCUMNAVIGATE = 6,     // Go around in wide arc
    JUMP_OVER = 7           // Attempt to jump over obstacle
};

// Collision detection types
enum class CollisionType : uint8
{
    NONE = 0,
    IMMINENT = 1,           // Collision in <1 second
    NEAR_COLLISION = 2,     // Collision in 1-3 seconds
    DISTANT = 3,            // Collision in 3-5 seconds
    POTENTIAL = 4           // Potential future collision
};

// Obstacle detection flags
enum class DetectionFlags : uint32
{
    NONE = 0x00000000,
    TERRAIN = 0x00000001,           // Detect terrain obstacles
    UNITS = 0x00000002,             // Detect unit obstacles
    OBJECTS = 0x00000004,           // Detect game objects
    HAZARDS = 0x00000008,           // Detect environmental hazards
    PROJECTILES = 0x00000010,       // Detect moving projectiles
    PREDICTIVE = 0x00000020,        // Predict future positions
    FORMATION_AWARE = 0x00000040,   // Consider group formation
    DYNAMIC_ONLY = 0x00000080,      // Only dynamic obstacles
    STATIC_ONLY = 0x00000100,       // Only static obstacles

    // Common combinations
    BASIC = TERRAIN | UNITS | OBJECTS,
    COMBAT = BASIC | HAZARDS | PROJECTILES,
    FULL = COMBAT | PREDICTIVE | FORMATION_AWARE
};

DEFINE_ENUM_FLAG(DetectionFlags);

// Obstacle information structure
struct ObstacleInfo
{
    ObjectGuid guid;
    WorldObject* object;
    Position position;
    Position velocity;              // For moving obstacles
    Position predictedPosition;     // Predicted future position
    ObstacleType type;
    ObstaclePriority priority;
    float radius;
    float height;
    float mass;                     // For physics calculations
    bool isMoving;
    bool isTemporary;
    uint32 firstDetected;
    uint32 lastSeen;
    uint32 expirationTime;
    ::std::string name;

    // Collision prediction
    float timeToCollision;
    Position collisionPoint;
    bool willCollide;

    // Avoidance data
    AvoidanceBehavior recommendedBehavior;
    ::std::vector<Position> avoidanceWaypoints;
    float avoidanceRadius;

    ObstacleInfo() : object(nullptr), type(ObstacleType::STATIC_TERRAIN),
                    priority(ObstaclePriority::MODERATE), radius(0.0f), height(0.0f),
                    mass(1.0f), isMoving(false), isTemporary(false),
                    firstDetected(0), lastSeen(0), expirationTime(0),
                    timeToCollision(0.0f), willCollide(false),
                    recommendedBehavior(AvoidanceBehavior::DIRECT_AVOIDANCE),
                    avoidanceRadius(0.0f) {}
};

// Collision prediction result
struct CollisionPrediction
{
    bool willCollide;
    float timeToCollision;
    Position collisionPoint;
    CollisionType collisionType;
    ObstacleInfo* obstacle;
    float collisionSeverity;        // 0.0 = minor, 1.0 = severe
    ::std::vector<Position> avoidancePath;
    AvoidanceBehavior recommendedAction;

    CollisionPrediction() : willCollide(false), timeToCollision(0.0f),
                           collisionType(CollisionType::NONE), obstacle(nullptr),
                           collisionSeverity(0.0f), recommendedAction(AvoidanceBehavior::DIRECT_AVOIDANCE) {}
};

// Avoidance maneuver
struct AvoidanceManeuver
{
    AvoidanceBehavior behavior;
    ::std::vector<Position> waypoints;
    float executionTime;
    float successProbability;
    float energyCost;
    uint32 priority;
    bool requiresJump;
    bool requiresSprint;
    bool maintainsFormation;
    ::std::string description;

    AvoidanceManeuver() : behavior(AvoidanceBehavior::DIRECT_AVOIDANCE),
                         executionTime(0.0f), successProbability(0.0f),
                         energyCost(0.0f), priority(0), requiresJump(false),
                         requiresSprint(false), maintainsFormation(true) {}

    bool operator<(const AvoidanceManeuver& other) const
    {
        if (priority != other.priority)
            return priority < other.priority;
        return successProbability > other.successProbability;
    }
};

// Detection context for obstacle scanning
struct DetectionContext
{
    Player* bot;
    Position currentPosition;
    Position targetPosition;
    Position velocity;
    float scanRadius;
    float lookaheadTime;
    DetectionFlags flags;
    ::std::vector<Player*> groupMembers;
    bool inCombat;
    bool emergencyMode;

    DetectionContext() : bot(nullptr), scanRadius(15.0f), lookaheadTime(3.0f),
                        flags(DetectionFlags::BASIC), inCombat(false), emergencyMode(false) {}
};

// Performance metrics for obstacle avoidance
struct AvoidanceMetrics
{
    ::std::atomic<uint32> obstaclesDetected{0};
    ::std::atomic<uint32> collisionsPrevented{0};
    ::std::atomic<uint32> avoidanceManeuvers{0};
    ::std::atomic<uint32> emergencyStops{0};
    ::std::atomic<uint32> falsePositives{0};
    ::std::chrono::microseconds averageDetectionTime{0};
    ::std::chrono::microseconds maxDetectionTime{0};
    ::std::chrono::steady_clock::time_point lastUpdate;

    void Reset()
    {
        obstaclesDetected = 0;
        collisionsPrevented = 0;
        avoidanceManeuvers = 0;
        emergencyStops = 0;
        falsePositives = 0;
        averageDetectionTime = ::std::chrono::microseconds{0};
        maxDetectionTime = ::std::chrono::microseconds{0};
        lastUpdate = ::std::chrono::steady_clock::now();
    }

    float GetSuccessRate() const
    {
        uint32 total = avoidanceManeuvers.load();
        return total > 0 ? static_cast<float>(collisionsPrevented.load()) / total : 0.0f;
    }
};

class TC_GAME_API ObstacleAvoidanceManager
{
public:
    explicit ObstacleAvoidanceManager(Player* bot);
    ~ObstacleAvoidanceManager() = default;

    // Primary obstacle detection and avoidance interface
    void UpdateObstacleDetection(const DetectionContext& context);
    ::std::vector<CollisionPrediction> PredictCollisions(const DetectionContext& context);
    ::std::vector<AvoidanceManeuver> GenerateAvoidanceManeuvers(const CollisionPrediction& collision);
    bool ExecuteAvoidanceManeuver(const AvoidanceManeuver& maneuver);

    // Obstacle detection methods
    ::std::vector<ObstacleInfo> ScanForObstacles(const DetectionContext& context);
    ::std::vector<ObstacleInfo> DetectStaticObstacles(const DetectionContext& context);
    ::std::vector<ObstacleInfo> DetectDynamicObstacles(const DetectionContext& context);
    ::std::vector<ObstacleInfo> DetectUnitObstacles(const DetectionContext& context);
    ::std::vector<ObstacleInfo> DetectEnvironmentalHazards(const DetectionContext& context);

    // Collision prediction and analysis
    CollisionPrediction PredictCollisionWithObstacle(const ObstacleInfo& obstacle, const DetectionContext& context);
    bool WillCollideWithPath(const ::std::vector<Position>& path, const ObstacleInfo& obstacle);
    float CalculateTimeToCollision(const ObstacleInfo& obstacle, const DetectionContext& context);
    Position PredictObstaclePosition(const ObstacleInfo& obstacle, float timeAhead);

    // Avoidance strategy generation
    AvoidanceManeuver GenerateDirectAvoidance(const CollisionPrediction& collision);
    AvoidanceManeuver GenerateCircumnavigation(const CollisionPrediction& collision);
    AvoidanceManeuver GenerateWaitAndPass(const CollisionPrediction& collision);
    AvoidanceManeuver GenerateJumpOver(const CollisionPrediction& collision);
    AvoidanceManeuver GenerateBacktrack(const CollisionPrediction& collision);

    // Real-time collision avoidance
    bool RequiresImmediateAvoidance();
    Position CalculateEmergencyAvoidancePosition();
    void ExecuteEmergencyStop();
    bool CanSafelyProceed(const Position& nextPosition);

    // Formation-aware avoidance
    ::std::vector<AvoidanceManeuver> GenerateFormationAwareAvoidance(const CollisionPrediction& collision);
    bool WillManeuverBreakFormation(const AvoidanceManeuver& maneuver, const ::std::vector<Player*>& group);
    Position AdjustAvoidanceForFormation(const Position& avoidancePos, const ::std::vector<Player*>& group);

    // Obstacle classification and analysis
    ObstacleType ClassifyObstacle(WorldObject* object);
    ObstaclePriority AssessObstaclePriority(const ObstacleInfo& obstacle, const DetectionContext& context);
    float CalculateObstacleRadius(WorldObject* object, ObstacleType type);
    bool IsObstacleTemporary(const ObstacleInfo& obstacle);

    // Predictive systems
    void UpdateObstaclePredictions();
    Position PredictUnitMovement(Unit* unit, float timeAhead);
    ::std::vector<Position> PredictProjectilePath(WorldObject* projectile);
    bool WillObstacleBlock(const ObstacleInfo& obstacle, const Position& from, const Position& to, float timeOffset = 0.0f);

    // Obstacle management
    void RegisterObstacle(const ObstacleInfo& obstacle);
    void UpdateObstacle(const ObstacleInfo& obstacle);
    void RemoveObstacle(ObjectGuid guid);
    void CleanupExpiredObstacles();
    ObstacleInfo* GetObstacle(ObjectGuid guid);

    // Query methods
    bool IsPathBlocked(const ::std::vector<Position>& path);
    bool IsPositionBlocked(const Position& pos, float radius = 1.0f);
    ::std::vector<ObstacleInfo> GetObstaclesInRadius(const Position& center, float radius);
    ::std::vector<ObstacleInfo> GetObstaclesOnPath(const Position& start, const Position& end);

    // Configuration
    void SetScanRadius(float radius) { _scanRadius = radius; }
    float GetScanRadius() const { return _scanRadius; }
    void SetLookaheadTime(float time) { _lookaheadTime = time; }
    float GetLookaheadTime() const { return _lookaheadTime; }
    void SetUpdateInterval(uint32 interval) { _updateInterval = interval; }
    uint32 GetUpdateInterval() const { return _updateInterval; }

    // Performance monitoring
    AvoidanceMetrics const& GetMetrics() const { return _metrics; }
    void ResetMetrics() { _metrics.Reset(); }

    // Advanced features
    void EnablePredictiveAvoidance(bool enable) { _predictiveAvoidance = enable; }
    bool IsPredictiveAvoidanceEnabled() const { return _predictiveAvoidance; }
    void SetCollisionTolerance(float tolerance) { _collisionTolerance = tolerance; }
    float GetCollisionTolerance() const { return _collisionTolerance; }

    // Emergency systems
    void ActivateEmergencyMode() { _emergencyMode = true; }
    void DeactivateEmergencyMode() { _emergencyMode = false; }
    bool IsEmergencyMode() const { return _emergencyMode; }
    ::std::vector<Position> GetEmergencyEscapeRoutes();

private:
    // Core detection algorithms
    void ScanTerrain(const DetectionContext& context, ::std::vector<ObstacleInfo>& obstacles);
    void ScanUnits(const DetectionContext& context, ::std::vector<ObstacleInfo>& obstacles);
    void ScanGameObjects(const DetectionContext& context, ::std::vector<ObstacleInfo>& obstacles);
    void ScanEnvironmentalHazards(const DetectionContext& context, ::std::vector<ObstacleInfo>& obstacles);

    // Collision calculation helpers
    bool CheckSphereCollision(const Position& center1, float radius1, const Position& center2, float radius2);
    bool CheckCapsuleCollision(const Position& start, const Position& end, float radius, const Position& center, float obstacleRadius);
    float CalculateClosestApproach(const Position& pos1, const Position& vel1, const Position& pos2, const Position& vel2);
    Position CalculateInterceptionPoint(const Position& target, const Position& targetVel, const Position& interceptor, float interceptorSpeed);

    // Avoidance calculation helpers
    ::std::vector<Position> CalculateAvoidancePath(const Position& start, const Position& goal, const ObstacleInfo& obstacle);
    Position FindNearestClearPosition(const Position& blocked, float searchRadius);
    bool IsPositionClear(const Position& pos, float radius, const ::std::vector<ObstacleInfo>& obstacles);
    float CalculateAvoidanceAngle(const Position& bot, const Position& obstacle, const Position& goal);

    // Predictive analysis
    void UpdateMovementPredictions();
    bool WillPathIntersectObstacle(const ::std::vector<Position>& path, const ObstacleInfo& obstacle, float timeOffset);
    float EstimateObstacleClearanceTime(const ObstacleInfo& obstacle);

    // Optimization and caching
    void OptimizeObstacleList();
    bool ShouldIgnoreObstacle(const ObstacleInfo& obstacle, const DetectionContext& context);
    void CacheAvoidanceCalculation(const ::std::string& key, const AvoidanceManeuver& maneuver);
    AvoidanceManeuver* GetCachedAvoidance(const ::std::string& key);

    // Performance tracking
    void TrackPerformance(::std::chrono::microseconds duration, const ::std::string& operation);

    // Utility methods
    float GetBotRadius() const;
    float GetBotSpeed() const;
    bool IsInScanRange(const Position& pos, const DetectionContext& context);
    ::std::string GenerateObstacleKey(const ObstacleInfo& obstacle);

private:
    Player* _bot;

    // Configuration
    float _scanRadius;
    float _lookaheadTime;
    uint32 _updateInterval;
    uint32 _lastUpdate;
    float _collisionTolerance;
    bool _predictiveAvoidance;
    bool _emergencyMode;

    // Obstacle tracking
    ::std::unordered_map<ObjectGuid, ObstacleInfo> _obstacles;
    ::std::vector<ObstacleInfo> _staticObstacles;
    ::std::vector<ObstacleInfo> _dynamicObstacles;
    uint32 _lastCleanup;

    // Avoidance cache
    ::std::unordered_map<::std::string, AvoidanceManeuver> _avoidanceCache;
    uint32 _lastCacheCleanup;

    // Performance metrics
    mutable AvoidanceMetrics _metrics;

    // Thread safety
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _mutex;

    // Constants
    static constexpr float DEFAULT_SCAN_RADIUS = 15.0f;         // 15 yards scan radius
    static constexpr float DEFAULT_LOOKAHEAD_TIME = 3.0f;       // 3 seconds lookahead
    static constexpr uint32 DEFAULT_UPDATE_INTERVAL = 100;      // 100ms updates
    static constexpr float DEFAULT_COLLISION_TOLERANCE = 0.5f;  // 0.5 yards tolerance
    static constexpr uint32 CLEANUP_INTERVAL = 5000;           // 5 seconds
    static constexpr uint32 CACHE_CLEANUP_INTERVAL = 10000;    // 10 seconds
    static constexpr size_t MAX_OBSTACLES = 100;               // Maximum tracked obstacles
    static constexpr size_t MAX_CACHE_SIZE = 50;               // Maximum cached maneuvers
};

// Obstacle avoidance utilities
class TC_GAME_API ObstacleUtils
{
public:
    // Geometric utilities
    static bool IsPointInCircle(const Position& point, const Position& center, float radius);
    static bool IsPointInRectangle(const Position& point, const Position& rectMin, const Position& rectMax);
    static float DistancePointToLine(const Position& point, const Position& lineStart, const Position& lineEnd);
    static Position ClosestPointOnLine(const Position& point, const Position& lineStart, const Position& lineEnd);

    // Collision detection utilities
    static bool DoCirclesIntersect(const Position& center1, float radius1, const Position& center2, float radius2);
    static bool DoesLineIntersectCircle(const Position& lineStart, const Position& lineEnd, const Position& center, float radius);
    static ::std::vector<Position> FindLineCircleIntersections(const Position& lineStart, const Position& lineEnd, const Position& center, float radius);

    // Avoidance calculation utilities
    static float CalculateAvoidanceRadius(float obstacleRadius, float botRadius, float safetyMargin = 1.0f);
    static ::std::vector<Position> GenerateAvoidanceWaypoints(const Position& start, const Position& goal, const Position& obstacle, float avoidanceRadius);
    static Position CalculateTangentPoint(const Position& center, const Position& obstacle, float radius, bool leftTangent = true);

    // Prediction utilities
    static Position PredictPosition(const Position& current, const Position& velocity, float timeAhead);
    static Position PredictInterception(const Position& target, const Position& targetVel, const Position& interceptor, float interceptorSpeed);
    static bool WillPathsIntersect(const Position& start1, const Position& end1, const Position& start2, const Position& end2, float& intersectionTime);

    // Formation utilities
    static bool WillAvoidanceBreakFormation(const Position& avoidancePos, const ::std::vector<Player*>& group, float maxDistance = 15.0f);
    static Position AdjustForFormationConstraints(const Position& intended, const ::std::vector<Player*>& group, float minDistance = 3.0f);
    static float CalculateFormationStrain(const Position& newPos, const ::std::vector<Player*>& group);

    // Safety assessment utilities
    static float CalculatePathSafety(const ::std::vector<Position>& path, const ::std::vector<ObstacleInfo>& obstacles);
    static bool IsPositionSafe(const Position& pos, const ::std::vector<ObstacleInfo>& obstacles, float safetyRadius = 2.0f);
    static Position FindSafestNearbyPosition(const Position& target, const ::std::vector<ObstacleInfo>& obstacles, float searchRadius = 10.0f);
};

} // namespace Playerbot