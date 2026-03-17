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
#include "Core/LRUCache.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <atomic>
#include <shared_mutex>
#include <chrono>
#include <queue>
#include <functional>

// Forward declarations
class Player;
class Unit;
class Map;
class PathGenerator;

namespace Playerbot
{

// Pathfinding types and priorities
enum class PathType : uint8
{
    STRAIGHT_LINE = 0,      // Direct line movement
    GROUND_PATH = 1,        // Ground-based pathfinding
    WATER_PATH = 2,         // Swimming pathfinding
    FLYING_PATH = 3,        // Flying mount pathfinding
    JUMP_PATH = 4,          // Includes jumping
    TELEPORT_PATH = 5,      // Teleportation paths
    COMPLEX_PATH = 6        // Multi-terrain complex path
};

// Movement behavior types
enum class MovementBehavior : uint8
{
    DIRECT = 0,             // Move directly to target
    CAUTIOUS = 1,           // Avoid dangerous areas
    STEALTHY = 2,           // Avoid detection
    AGGRESSIVE = 3,         // Shortest path regardless of danger
    FORMATION = 4,          // Maintain group formation
    KITING = 5,             // Maintain distance while moving
    FLANKING = 6,           // Tactical positioning
    RETREAT = 7             // Emergency escape movement
};

// Path quality assessment
enum class PathQuality : uint8
{
    OPTIMAL = 0,            // Best possible path
    GOOD = 1,               // Acceptable path
    SUBOPTIMAL = 2,         // Usable but not ideal
    POOR = 3,               // Last resort path
    BLOCKED = 4             // No viable path
};

// Pathfinding flags
enum class PathFlags : uint32
{
    NONE = 0x00000000,
    ALLOW_JUMPING = 0x00000001,
    ALLOW_SWIMMING = 0x00000002,
    ALLOW_FLYING = 0x00000004,
    AVOID_ENEMIES = 0x00000008,
    AVOID_AOE = 0x00000010,
    AVOID_TERRAIN_DAMAGE = 0x00000020,
    SHORTEST_PATH = 0x00000040,
    SAFEST_PATH = 0x00000080,
    MAINTAIN_LOS = 0x00000100,
    AVOID_WATER = 0x00000200,
    AVOID_FALL_DAMAGE = 0x00000400,
    ALLOW_TELEPORT = 0x00000800,
    FORMATION_AWARE = 0x00001000,

    // Common flag combinations
    BASIC = ALLOW_JUMPING | AVOID_FALL_DAMAGE,
    COMBAT = BASIC | AVOID_ENEMIES | AVOID_AOE,
    SAFE = COMBAT | SAFEST_PATH | AVOID_TERRAIN_DAMAGE,
    FAST = BASIC | SHORTEST_PATH | ALLOW_TELEPORT
};

DEFINE_ENUM_FLAG(PathFlags);

// A* pathfinding node
struct PathNode
{
    Position position;
    float gCost;                // Distance from start
    float hCost;                // Heuristic distance to goal
    float fCost;                // Total cost (g + h)
    PathNode* parent;
    bool walkable;
    bool inWater;
    bool isJump;
    float dangerRating;
    uint32 nodeId;

    PathNode() : gCost(0.0f), hCost(0.0f), fCost(0.0f), parent(nullptr),
                walkable(true), inWater(false), isJump(false), dangerRating(0.0f), nodeId(0) {}

    PathNode(const Position& pos) : position(pos), gCost(0.0f), hCost(0.0f), fCost(0.0f),
                                   parent(nullptr), walkable(true), inWater(false), isJump(false),
                                   dangerRating(0.0f), nodeId(0) {}

    void CalculateFCost() { fCost = gCost + hCost; }

    bool operator<(const PathNode& other) const { return fCost > other.fCost; }

    // ST-3: Reset node state for pool reuse
    void Reset(const Position& pos)
    {
        position = pos;
        gCost = 0.0f;
        hCost = 0.0f;
        fCost = 0.0f;
        parent = nullptr;
        walkable = true;
        inWater = false;
        isJump = false;
        dangerRating = 0.0f;
        // nodeId is set by pool
    }
};

// Pathfinding request
struct PathRequest
{
    ObjectGuid botGuid;
    Position startPos;
    Position goalPos;
    PathType pathType;
    MovementBehavior behavior;
    PathFlags flags;
    float maxRange;
    float nodeSpacing;
    uint32 maxNodes;
    uint32 timeoutMs;
    ::std::vector<Position> avoidAreas;
    ::std::vector<Position> waypoints;
    uint32 priority;
    ::std::function<void(const ::std::vector<Position>&)> callback;

    PathRequest() : pathType(PathType::GROUND_PATH), behavior(MovementBehavior::DIRECT),
                   flags(PathFlags::BASIC), maxRange(1000.0f), nodeSpacing(1.0f),
                   maxNodes(500), timeoutMs(100), priority(0) {}
};

// Pathfinding result
struct PathResult
{
    bool success;
    ::std::vector<Position> waypoints;
    PathQuality quality;
    PathType usedPathType;
    float totalDistance;
    float estimatedTime;
    uint32 nodeCount;
    uint32 calculationTime;
    ::std::string failureReason;
    bool partialPath;
    Position furthestReachable;

    PathResult() : success(false), quality(PathQuality::BLOCKED), usedPathType(PathType::STRAIGHT_LINE),
                  totalDistance(0.0f), estimatedTime(0.0f), nodeCount(0), calculationTime(0),
                  partialPath(false) {}
};

// Danger zone for pathfinding avoidance
struct DangerZone
{
    Position center;
    float radius;
    float dangerLevel;      // 0.0 = safe, 1.0 = maximum danger
    uint32 startTime;
    uint32 duration;
    bool isActive;
    ::std::string source;     // What causes the danger

    DangerZone() : radius(0.0f), dangerLevel(0.0f), startTime(0), duration(0), isActive(false) {}

    bool IsPositionInDanger(const Position& pos, uint32 currentTime) const
    {
        if (!isActive || currentTime > startTime + duration)
            return false;
        return center.GetExactDist(&pos) <= radius;
    }

    float GetDangerAtPosition(const Position& pos, uint32 currentTime) const
    {
        if (!IsPositionInDanger(pos, currentTime))
            return 0.0f;

        float distance = center.GetExactDist(&pos);
        float normalizedDistance = distance / radius;
        return dangerLevel * (1.0f - normalizedDistance);
    }
};

// Pathfinding performance metrics
struct PathfindingMetrics
{
    ::std::atomic<uint32> pathRequests{0};
    ::std::atomic<uint32> successfulPaths{0};
    ::std::atomic<uint32> failedPaths{0};
    ::std::atomic<uint32> partialPaths{0};
    ::std::atomic<uint32> cacheHits{0};
    ::std::atomic<uint32> cacheMisses{0};
    ::std::chrono::microseconds averageCalculationTime{0};
    ::std::chrono::microseconds maxCalculationTime{0};
    ::std::chrono::steady_clock::time_point lastUpdate;

    void Reset()
    {
        pathRequests = 0;
        successfulPaths = 0;
        failedPaths = 0;
        partialPaths = 0;
        cacheHits = 0;
        cacheMisses = 0;
        averageCalculationTime = ::std::chrono::microseconds{0};
        maxCalculationTime = ::std::chrono::microseconds{0};
        lastUpdate = ::std::chrono::steady_clock::now();
    }

    float GetSuccessRate() const
    {
        uint32 total = pathRequests.load();
        return total > 0 ? static_cast<float>(successfulPaths.load()) / total : 0.0f;
    }

    float GetCacheHitRate() const
    {
        uint32 total = pathRequests.load();
        return total > 0 ? static_cast<float>(cacheHits.load()) / total : 0.0f;
    }
};

// Path cache entry
struct PathCacheEntry
{
    Position startPos;
    Position goalPos;
    ::std::vector<Position> waypoints;
    PathQuality quality;
    uint32 timestamp;
    uint32 expirationTime;
    uint32 accessCount;

    bool IsExpired(uint32 currentTime) const
    {
        return currentTime > expirationTime;
    }

    bool IsValid(const Position& start, const Position& goal, float tolerance = 2.0f) const
    {
        return startPos.GetExactDist(&start) <= tolerance && goalPos.GetExactDist(&goal) <= tolerance;
    }
};

class TC_GAME_API PathfindingManager
{
public:
    explicit PathfindingManager(Player* bot);
    ~PathfindingManager() = default;

    // Primary pathfinding interface
    PathResult FindPath(const PathRequest& request);
    PathResult FindPath(const Position& goal, PathFlags flags = PathFlags::BASIC);
    PathResult FindPathToUnit(Unit* target, float range = 0.0f, PathFlags flags = PathFlags::BASIC);
    PathResult FindEscapePath(const ::std::vector<Unit*>& threats, float minDistance = 15.0f);

    // Async pathfinding
    void FindPathAsync(const PathRequest& request);
    bool IsPathfindingComplete(uint32 requestId) const;
    PathResult GetAsyncResult(uint32 requestId);

    // A* algorithm implementation
    PathResult CalculateAStarPath(const Position& start, const Position& goal, const PathRequest& request);
    ::std::vector<PathNode*> ReconstructPath(PathNode* goalNode);
    float CalculateHeuristic(const Position& current, const Position& goal);

    // Path validation and optimization
    bool IsPathValid(const ::std::vector<Position>& waypoints);
    ::std::vector<Position> OptimizePath(const ::std::vector<Position>& waypoints);
    ::std::vector<Position> SmoothPath(const ::std::vector<Position>& waypoints);
    PathQuality AssessPathQuality(const ::std::vector<Position>& waypoints, const PathRequest& request);

    // Node and terrain analysis
    bool IsNodeWalkable(const Position& pos, const PathRequest& request);
    float GetNodeCost(const Position& from, const Position& to, const PathRequest& request);
    ::std::vector<Position> GetNeighborNodes(const Position& center, float spacing);
    float CalculateTerrainCost(const Position& pos);

    // Danger and avoidance systems
    void RegisterDangerZone(const DangerZone& zone);
    void UpdateDangerZones(uint32 currentTime);
    void ClearExpiredDangerZones(uint32 currentTime);
    float GetDangerAtPosition(const Position& pos);
    bool IsPositionSafe(const Position& pos, float safetyThreshold = 0.5f);

    // Special movement types
    PathResult FindJumpPath(const Position& start, const Position& goal);
    PathResult FindSwimmingPath(const Position& start, const Position& goal);
    PathResult FindFlyingPath(const Position& start, const Position& goal);
    PathResult FindFormationPath(const Position& goal, const ::std::vector<Player*>& groupMembers);

    // Line of sight integration
    bool MaintainsLineOfSight(const ::std::vector<Position>& waypoints, Unit* target);
    ::std::vector<Position> FindLoSPreservingPath(const Position& goal, Unit* target);
    Position GetNextLoSPosition(const Position& current, Unit* target);

    // Cache management
    void ClearPathCache();
    void ClearExpiredCacheEntries();
    void SetCacheDuration(uint32 durationMs) { _cacheDuration = durationMs; }
    uint32 GetCacheDuration() const { return _cacheDuration; }

    // Performance monitoring
    PathfindingMetrics const& GetMetrics() const { return _metrics; }
    void ResetMetrics() { _metrics.Reset(); }

    // Configuration
    void SetNodeSpacing(float spacing) { _defaultNodeSpacing = spacing; }
    float GetNodeSpacing() const { return _defaultNodeSpacing; }
    void SetMaxNodes(uint32 maxNodes) { _maxNodes = maxNodes; }
    uint32 GetMaxNodes() const { return _maxNodes; }
    void SetTimeout(uint32 timeoutMs) { _pathfindingTimeout = timeoutMs; }
    uint32 GetTimeout() const { return _pathfindingTimeout; }

    // Advanced pathfinding features
    bool CanReachPosition(const Position& pos, float tolerance = 1.0f);
    float EstimatePathLength(const Position& start, const Position& goal);
    Position GetClosestReachablePosition(const Position& target);
    ::std::vector<Position> GetAlternativePaths(const Position& goal, uint32 maxAlternatives = 3);

    // Group coordination
    PathResult FindGroupCoordinatedPath(const Position& goal, const ::std::vector<Player*>& group);
    bool WillPathIntersectWithGroupMember(const ::std::vector<Position>& waypoints, const ::std::vector<Player*>& group);
    Position AdjustPathForGroupAvoidance(const Position& intended, const ::std::vector<Player*>& group);

private:
    // Core A* algorithm components
    PathNode* CreateNode(const Position& pos);
    void AddToOpenSet(PathNode* node, ::std::priority_queue<PathNode>& openSet);
    PathNode* GetLowestFCostNode(::std::vector<PathNode*>& openSet);
    bool IsInClosedSet(const Position& pos, const ::std::vector<PathNode*>& closedSet);

    // Path calculation helpers
    ::std::vector<Position> GenerateWaypoints(const Position& start, const Position& goal, uint32 count);
    float CalculateMovementCost(const Position& from, const Position& to, const PathRequest& request);
    bool IsDirectPathPossible(const Position& start, const Position& goal, const PathRequest& request);
    bool RequiresComplexPathfinding(const Position& start, const Position& goal, const PathRequest& request);

    // Terrain and environment analysis
    bool IsWaterNode(const Position& pos);
    bool IsClimbableNode(const Position& pos);
    bool RequiresJump(const Position& from, const Position& to);
    float GetFallDamageRisk(const Position& from, const Position& to);
    bool IsInDangerousArea(const Position& pos);

    // Cache management helpers
    void AddCacheEntry(const PathCacheEntry& entry);
    ::std::string GenerateCacheKey(const Position& start, const Position& goal);

    // Performance tracking
    void TrackPerformance(::std::chrono::microseconds duration, bool successful, bool cached);

    // Utility methods
    Position ClampToWorldBounds(const Position& pos);
    bool IsPositionInWorld(const Position& pos);
    float GetTerrainSlope(const Position& pos);

private:
    Player* _bot;

    // Configuration
    float _defaultNodeSpacing;
    uint32 _maxNodes;
    uint32 _pathfindingTimeout;
    uint32 _cacheDuration;
    bool _enableCaching;
    bool _enableDangerAvoidance;

    // Pathfinding state
    BoundedHistory<DangerZone> _dangerZones{MAX_DANGER_ZONES};  // Bounded danger zones
    uint32 _lastDangerUpdate;

    // Cache system - NOW USES LRU CACHE for automatic memory management
    LRUCache<::std::string, PathCacheEntry> _pathCache{MAX_CACHE_SIZE,
        ::std::chrono::milliseconds(DEFAULT_CACHE_DURATION)};  // LRU with TTL

    // Async pathfinding - bounded to prevent unbounded growth
    BoundedMap<uint32, PathResult> _asyncResults{MAX_ASYNC_RESULTS};
    ::std::atomic<uint32> _nextRequestId{1};

    // Performance metrics
    mutable PathfindingMetrics _metrics;

    // Thread safety
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _mutex;

    // ============================================================================
    // ST-3: PathNode Pool for Reduced Allocations
    // ============================================================================
    // Pre-allocated storage eliminates heap allocations during A* pathfinding.
    // Nodes are reused across pathfinding calls within the same manager instance.
    // Expected reduction: 250k allocations/sec → near zero for typical 5000-bot scenarios.
    // ============================================================================

    /// Pre-allocated PathNode storage (grows if needed)
    ::std::vector<PathNode> _nodeStorage;

    /// Current allocation index (reset at start of each pathfinding)
    size_t _nodePoolIndex;

    /// Next unique node ID
    uint32 _nextNodeId;

    /// Acquire node from pool (reset state)
    PathNode* AcquireNode(const Position& pos);

    /// Reset pool for new pathfinding operation
    void ResetNodePool();

    /// Pool statistics
    struct NodePoolStats
    {
        uint64_t totalAcquires{0};
        uint64_t totalReuses{0};
        size_t peakUsage{0};
        size_t currentCapacity{0};
    };
    mutable NodePoolStats _nodePoolStats;

    // Constants
    static constexpr float DEFAULT_NODE_SPACING = 1.0f;     // 1 yard between nodes
    static constexpr uint32 DEFAULT_MAX_NODES = 500;        // Maximum nodes in path
    static constexpr uint32 DEFAULT_TIMEOUT = 100;          // 100ms timeout
    static constexpr uint32 DEFAULT_CACHE_DURATION = 5000;  // 5 seconds
    static constexpr uint32 CACHE_CLEANUP_INTERVAL = 10000; // 10 seconds
    static constexpr uint32 DANGER_UPDATE_INTERVAL = 1000;  // 1 second
    static constexpr size_t MAX_CACHE_SIZE = 200;           // Maximum cache entries
    static constexpr size_t INITIAL_NODE_POOL_SIZE = 256;   // Initial pool capacity
    static constexpr size_t MAX_DANGER_ZONES = 50;          // Maximum tracked danger zones
    static constexpr size_t MAX_ASYNC_RESULTS = 100;        // Maximum pending async results
};

// Pathfinding utilities
class TC_GAME_API PathfindingUtils
{
public:
    // Distance and cost calculations
    static float CalculateEuclideanDistance(const Position& a, const Position& b);
    static float CalculateManhattanDistance(const Position& a, const Position& b);
    static float CalculateOctileDistance(const Position& a, const Position& b);

    // Path analysis utilities
    static float CalculatePathLength(const ::std::vector<Position>& waypoints);
    static float CalculatePathSmoothness(const ::std::vector<Position>& waypoints);
    static bool IsPathTooComplex(const ::std::vector<Position>& waypoints, float maxComplexity = 2.0f);
    static Position InterpolateAlongPath(const ::std::vector<Position>& waypoints, float distance);

    // Terrain analysis utilities
    static bool IsPositionOnGround(const Position& pos, Map* map);
    static bool IsPositionInAir(const Position& pos, Map* map, float threshold = 5.0f);
    static float GetGroundDistance(const Position& pos, Map* map);
    static bool CanWalkBetween(const Position& a, const Position& b, Map* map);

    // Path optimization utilities
    static ::std::vector<Position> RemoveRedundantWaypoints(const ::std::vector<Position>& waypoints);
    static ::std::vector<Position> InsertIntermediateWaypoints(const ::std::vector<Position>& waypoints, float maxDistance);
    static ::std::vector<Position> AdjustWaypointsToGround(const ::std::vector<Position>& waypoints, Map* map);

    // Formation and group utilities
    static Position CalculateFormationPosition(const Position& leaderPos, float angle, float distance);
    static ::std::vector<Position> GenerateFormationPositions(const Position& center, uint32 memberCount, float spacing);
    static bool WillFormationCollide(const ::std::vector<Position>& formation, const ::std::vector<Position>& obstacles);

    // Emergency pathfinding utilities
    static Position FindNearestSafePosition(const Position& current, const ::std::vector<DangerZone>& dangers, Map* map);
    static ::std::vector<Position> GenerateEscapeRoutes(const Position& current, uint32 routeCount, float distance);
    static Position GetFurthestSafePosition(const Position& start, const Position& direction, float maxDistance, Map* map);
};

} // namespace Playerbot