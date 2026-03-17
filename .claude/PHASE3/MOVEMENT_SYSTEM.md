# Movement System Implementation Guide

## Overview

The Movement System provides intelligent pathfinding and navigation for bots, integrating with Trinity's existing MotionMaster and map systems while adding bot-specific enhancements.

## Core Components

### 1. BotMovementAI (`src/modules/Playerbot/Movement/BotMovementAI.h/cpp`)

```cpp
#pragma once

#include "MotionMaster.h"
#include "PathGenerator.h"
#include <tbb/concurrent_queue.h>
#include <phmap.h>

namespace Playerbot
{

enum class MovementPriority
{
    IDLE = 0,
    FOLLOW = 1,
    PATROL = 2,
    COMBAT = 3,
    FLEE = 4,
    FORCED = 5
};

struct MovementRequest
{
    MovementPriority priority;
    Position destination;
    float speed;
    bool generatePath;
    std::function<void()> callback;
};

class TC_GAME_API BotMovementAI
{
public:
    BotMovementAI(Player* bot);
    ~BotMovementAI();

    // Main update
    void Update(uint32 diff);

    // Movement commands
    void MoveTo(Position const& pos, MovementPriority priority = MovementPriority::FOLLOW);
    void MoveToUnit(Unit* target, float distance = 0.0f);
    void Follow(Unit* target, float minDist = 2.0f, float maxDist = 5.0f);
    void Stop();
    void Flee(Unit* threat, float distance = 20.0f);

    // Formation movement
    void SetFormation(Formation* formation, uint8 position);
    void UpdateFormation();

    // Path management
    bool GeneratePath(Position const& dest);
    bool HasPath() const { return !_currentPath.empty(); }
    void ClearPath();

    // State queries
    bool IsMoving() const;
    Position GetDestination() const { return _destination; }
    float GetDistanceToDestination() const;
    MovementPriority GetCurrentPriority() const { return _currentPriority; }

    // Obstacle detection
    bool IsObstacleAhead(float distance = 5.0f) const;
    void AvoidObstacle();

    // Performance metrics
    struct Metrics
    {
        uint32 pathsGenerated;
        uint32 pathsFailed;
        uint32 obstaclesAvoided;
        float averagePathTime;
    };
    Metrics const& GetMetrics() const { return _metrics; }

private:
    // Internal movement
    void ExecuteMovement();
    void UpdatePath();
    bool ValidateDestination(Position const& pos) const;
    
    // Collision detection
    bool CheckCollision(Position const& from, Position const& to) const;
    Position FindAlternativePath(Position const& blocked) const;

    // Jump/swim handling
    void HandleSpecialMovement();
    bool ShouldJump() const;
    bool ShouldSwim() const;

private:
    Player* _bot;
    PathGenerator* _pathGenerator;
    
    // Current movement state
    MovementPriority _currentPriority;
    Position _destination;
    std::vector<G3D::Vector3> _currentPath;
    size_t _currentPathIndex;
    
    // Movement queue
    tbb::concurrent_queue<MovementRequest> _movementQueue;
    
    // Formation
    Formation* _formation;
    uint8 _formationPosition;
    
    // Timing
    uint32 _lastUpdateTime;
    uint32 _stuckDetectTimer;
    Position _lastPosition;
    
    // Performance
    Metrics _metrics;
};

} // namespace Playerbot
```

### 2. PathfindingIntegration (`src/modules/Playerbot/Movement/PathfindingIntegration.h/cpp`)

```cpp
#pragma once

#include "PathGenerator.h"
#include "MMapFactory.h"
#include <tbb/concurrent_hash_map.h>

namespace Playerbot
{

// Path caching for performance
struct PathCacheEntry
{
    std::vector<G3D::Vector3> path;
    uint32 timestamp;
    uint32 useCount;
};

class TC_GAME_API PathfindingIntegration
{
public:
    static PathfindingIntegration* instance();

    // Path generation with caching
    bool GeneratePath(
        uint32 mapId,
        Position const& start,
        Position const& end,
        std::vector<G3D::Vector3>& path,
        Unit* unit = nullptr
    );

    // Specialized pathfinding
    bool GenerateFormationPath(
        uint32 mapId,
        std::vector<Position> const& starts,
        Position const& end,
        std::vector<std::vector<G3D::Vector3>>& paths
    );

    // Line of sight
    bool IsInLineOfSight(
        uint32 mapId,
        Position const& start,
        Position const& end
    ) const;

    // Terrain queries
    float GetGroundHeight(uint32 mapId, float x, float y, float z) const;
    bool IsInWater(uint32 mapId, float x, float y, float z) const;
    bool IsUnderwater(uint32 mapId, float x, float y, float z) const;

    // Cache management
    void ClearCache();
    void PruneCache(uint32 olderThanMs = 60000);
    
    struct CacheStats
    {
        uint32 hits;
        uint32 misses;
        uint32 entries;
        size_t memoryUsage;
    };
    CacheStats GetCacheStats() const;

private:
    PathfindingIntegration();
    ~PathfindingIntegration();

    // Cache key generation
    uint64 GetPathCacheKey(Position const& start, Position const& end) const;

    // Path validation
    bool ValidatePath(std::vector<G3D::Vector3> const& path) const;
    void SmoothPath(std::vector<G3D::Vector3>& path) const;
    void OptimizePath(std::vector<G3D::Vector3>& path) const;

private:
    // Path cache (thread-safe)
    tbb::concurrent_hash_map<uint64, PathCacheEntry> _pathCache;
    
    // Statistics
    mutable std::atomic<uint32> _cacheHits;
    mutable std::atomic<uint32> _cacheMisses;
};

#define sPathfinding PathfindingIntegration::instance()

} // namespace Playerbot
```

### 3. MovementGenerator (`src/modules/Playerbot/Movement/MovementGenerator.h/cpp`)

```cpp
#pragma once

#include "MovementGenerator.h"
#include "Position.h"

namespace Playerbot
{

// Custom movement generators for bots
class BotFollowMovementGenerator : public MovementGeneratorMedium<Player>
{
public:
    explicit BotFollowMovementGenerator(Unit* target, float minDist, float maxDist);
    
    void Initialize(Player* owner) override;
    void Finalize(Player* owner) override;
    void Reset(Player* owner) override;
    bool Update(Player* owner, uint32 diff) override;
    
    MovementGeneratorType GetMovementGeneratorType() const override 
    { 
        return FOLLOW_MOTION_TYPE; 
    }

private:
    void UpdateFollowPosition(Player* owner);
    Position CalculateFollowPosition() const;
    
private:
    Unit* _target;
    float _minDistance;
    float _maxDistance;
    float _angle;
    uint32 _updateTimer;
    Position _lastTargetPosition;
};

class BotFleeMovementGenerator : public MovementGeneratorMedium<Player>
{
public:
    explicit BotFleeMovementGenerator(Unit* threat, float distance);
    
    void Initialize(Player* owner) override;
    void Finalize(Player* owner) override;
    void Reset(Player* owner) override;
    bool Update(Player* owner, uint32 diff) override;
    
    MovementGeneratorType GetMovementGeneratorType() const override 
    { 
        return FLEEING_MOTION_TYPE; 
    }

private:
    Position CalculateFleePosition(Player* owner) const;
    bool IsSafePosition(Position const& pos) const;
    
private:
    Unit* _threat;
    float _fleeDistance;
    Position _fleeDestination;
    uint32 _recheckTimer;
};

class BotFormationMovementGenerator : public MovementGeneratorMedium<Player>
{
public:
    BotFormationMovementGenerator(Formation* formation, uint8 position);
    
    void Initialize(Player* owner) override;
    void Finalize(Player* owner) override;
    void Reset(Player* owner) override;
    bool Update(Player* owner, uint32 diff) override;
    
    MovementGeneratorType GetMovementGeneratorType() const override 
    { 
        return FORMATION_MOTION_TYPE; 
    }

private:
    Position CalculateFormationPosition() const;
    void AdjustForTerrain(Position& pos) const;
    
private:
    Formation* _formation;
    uint8 _formationPosition;
    uint32 _syncTimer;
};

} // namespace Playerbot
```

## Implementation Details

### Path Caching Strategy

```cpp
// Efficient path caching with TBB
uint64 PathfindingIntegration::GetPathCacheKey(Position const& start, Position const& end) const
{
    // Create unique key from positions (rounded to avoid float precision issues)
    uint32 sx = uint32(start.GetPositionX() * 10);
    uint32 sy = uint32(start.GetPositionY() * 10);
    uint32 ex = uint32(end.GetPositionX() * 10);
    uint32 ey = uint32(end.GetPositionY() * 10);
    
    return (uint64(sx) << 48) | (uint64(sy) << 32) | (uint64(ex) << 16) | uint64(ey);
}

bool PathfindingIntegration::GeneratePath(
    uint32 mapId,
    Position const& start,
    Position const& end,
    std::vector<G3D::Vector3>& path,
    Unit* unit)
{
    // Check cache first
    uint64 key = GetPathCacheKey(start, end);
    
    {
        tbb::concurrent_hash_map<uint64, PathCacheEntry>::const_accessor accessor;
        if (_pathCache.find(accessor, key))
        {
            // Cache hit - check age
            if (getMSTime() - accessor->second.timestamp < 60000) // 1 minute cache
            {
                path = accessor->second.path;
                _cacheHits++;
                return !path.empty();
            }
        }
    }
    
    _cacheMisses++;
    
    // Generate new path using Trinity's PathGenerator
    PathGenerator generator(unit);
    bool result = generator.CalculatePath(
        start.GetPositionX(),
        start.GetPositionY(), 
        start.GetPositionZ(),
        end.GetPositionX(),
        end.GetPositionY(),
        end.GetPositionZ()
    );
    
    if (result)
    {
        path = generator.GetPath();
        
        // Optimize path
        SmoothPath(path);
        OptimizePath(path);
        
        // Cache result
        PathCacheEntry entry;
        entry.path = path;
        entry.timestamp = getMSTime();
        entry.useCount = 1;
        
        tbb::concurrent_hash_map<uint64, PathCacheEntry>::accessor accessor;
        _pathCache.insert(accessor, key);
        accessor->second = entry;
    }
    
    return result;
}
```

### Obstacle Avoidance

```cpp
void BotMovementAI::AvoidObstacle()
{
    // Dynamic obstacle detection
    float checkDistance = 5.0f;
    float checkAngle = M_PI / 4; // 45 degrees
    
    // Check multiple angles for clear path
    std::vector<std::pair<float, Position>> alternatives;
    
    for (float angle = -M_PI; angle <= M_PI; angle += checkAngle)
    {
        Position checkPos = _bot->GetPosition();
        checkPos.RelocatePolarOffset(angle, checkDistance);
        
        if (!CheckCollision(_bot->GetPosition(), checkPos))
        {
            float distToGoal = checkPos.GetExactDist(_destination);
            alternatives.emplace_back(distToGoal, checkPos);
        }
    }
    
    // Choose best alternative (closest to goal)
    if (!alternatives.empty())
    {
        std::sort(alternatives.begin(), alternatives.end());
        MoveTo(alternatives.front().second, MovementPriority::FORCED);
    }
    else
    {
        // No clear path - stop and wait
        Stop();
        _stuckDetectTimer = 2000; // Wait 2 seconds
    }
    
    _metrics.obstaclesAvoided++;
}
```

### Formation Movement

```cpp
void BotMovementAI::UpdateFormation()
{
    if (!_formation)
        return;
    
    Position formationPos = _formation->GetPosition(_formationPosition);
    
    // Adjust for terrain
    float groundZ = _bot->GetMap()->GetHeight(
        _bot->GetPhaseShift(),
        formationPos.GetPositionX(),
        formationPos.GetPositionY(),
        formationPos.GetPositionZ()
    );
    
    formationPos.m_positionZ = groundZ + 0.5f;
    
    // Check if position needs update
    if (_bot->GetExactDist(formationPos) > 1.0f)
    {
        // Use formation movement with collision avoidance
        if (GeneratePath(formationPos))
        {
            ExecuteMovement();
        }
    }
}
```

## Performance Optimizations

### 1. Parallel Path Generation (TBB)

```cpp
void GenerateFormationPaths(std::vector<Player*> const& bots)
{
    tbb::parallel_for(
        tbb::blocked_range<size_t>(0, bots.size()),
        [&](tbb::blocked_range<size_t> const& range)
        {
            for (size_t i = range.begin(); i != range.end(); ++i)
            {
                bots[i]->GetBotMovementAI()->GeneratePath(destination);
            }
        }
    );
}
```

### 2. Movement Update Batching

```cpp
// Update bots in batches for cache efficiency
void UpdateBotMovement(std::vector<Player*> const& bots, uint32 diff)
{
    static constexpr size_t BATCH_SIZE = 64;
    
    for (size_t i = 0; i < bots.size(); i += BATCH_SIZE)
    {
        size_t end = std::min(i + BATCH_SIZE, bots.size());
        
        tbb::parallel_for(
            tbb::blocked_range<size_t>(i, end),
            [&](tbb::blocked_range<size_t> const& range)
            {
                for (size_t j = range.begin(); j != range.end(); ++j)
                {
                    bots[j]->GetBotMovementAI()->Update(diff);
                }
            }
        );
    }
}
```

### 3. Stuck Detection

```cpp
void BotMovementAI::Update(uint32 diff)
{
    // Stuck detection
    _stuckDetectTimer += diff;
    if (_stuckDetectTimer >= 1000) // Check every second
    {
        float distMoved = _bot->GetExactDist(_lastPosition);
        
        if (distMoved < 0.5f && IsMoving())
        {
            // Bot is stuck
            TC_LOG_DEBUG("playerbot", "Bot %s stuck at position %.2f %.2f %.2f",
                _bot->GetName().c_str(),
                _bot->GetPositionX(),
                _bot->GetPositionY(),
                _bot->GetPositionZ()
            );
            
            AvoidObstacle();
        }
        
        _lastPosition = _bot->GetPosition();
        _stuckDetectTimer = 0;
    }
    
    // Continue normal movement
    ExecuteMovement();
}
```

## Testing

### Unit Tests

```cpp
TEST_F(MovementTest, PathfindingCache)
{
    // Test cache hit rate
    Position start(100, 100, 10);
    Position end(200, 200, 10);
    
    std::vector<G3D::Vector3> path1, path2;
    
    // First call - cache miss
    ASSERT_TRUE(sPathfinding->GeneratePath(0, start, end, path1));
    auto stats1 = sPathfinding->GetCacheStats();
    
    // Second call - cache hit
    ASSERT_TRUE(sPathfinding->GeneratePath(0, start, end, path2));
    auto stats2 = sPathfinding->GetCacheStats();
    
    EXPECT_EQ(stats2.hits, stats1.hits + 1);
    EXPECT_EQ(path1, path2);
}

TEST_F(MovementTest, ObstacleAvoidance)
{
    auto bot = CreateTestBot();
    auto movement = bot->GetBotMovementAI();
    
    // Place obstacle
    Position obstacle(150, 150, 10);
    Position destination(200, 200, 10);
    
    movement->MoveTo(destination);
    
    // Simulate updates
    for (int i = 0; i < 100; ++i)
    {
        movement->Update(100);
        
        // Check bot doesn't collide with obstacle
        EXPECT_GT(bot->GetExactDist(obstacle), 5.0f);
    }
    
    // Should reach destination
    EXPECT_LT(bot->GetExactDist(destination), 2.0f);
}
```

## Performance Metrics

### Target Performance
- **Path Generation**: <5ms for 100-unit path
- **Cache Hit Rate**: >80% in normal gameplay
- **Update Frequency**: 100Hz for active movement
- **Memory Usage**: <1KB per bot
- **CPU Usage**: <0.01% per bot

### Monitoring

```cpp
class MovementMonitor
{
public:
    static void RecordPathGeneration(uint32 timeMs)
    {
        if (timeMs > 5)
        {
            TC_LOG_WARN("playerbot", "Slow path generation: %ums", timeMs);
        }
        
        _totalPathTime += timeMs;
        _pathCount++;
    }
    
    static void PrintStats()
    {
        TC_LOG_INFO("playerbot", "Movement Stats:");
        TC_LOG_INFO("playerbot", "  Average path time: %.2fms",
            float(_totalPathTime) / _pathCount);
        TC_LOG_INFO("playerbot", "  Cache hit rate: %.1f%%",
            100.0f * sPathfinding->GetCacheStats().hits / 
            (sPathfinding->GetCacheStats().hits + sPathfinding->GetCacheStats().misses));
    }
    
private:
    static std::atomic<uint32> _totalPathTime;
    static std::atomic<uint32> _pathCount;
};
```

## Integration with Trinity

### Using MotionMaster

```cpp
void BotMovementAI::ExecuteMovement()
{
    if (_currentPath.empty())
        return;
    
    // Get next point in path
    if (_currentPathIndex >= _currentPath.size())
    {
        // Path complete
        ClearPath();
        return;
    }
    
    G3D::Vector3 const& point = _currentPath[_currentPathIndex];
    
    // Use Trinity's MotionMaster
    _bot->GetMotionMaster()->MovePoint(
        0,
        point.x,
        point.y,
        point.z
    );
    
    // Check if reached point
    if (_bot->GetExactDist2d(point.x, point.y) < 2.0f)
    {
        _currentPathIndex++;
    }
}
```

### Map System Integration

```cpp
bool BotMovementAI::ValidateDestination(Position const& pos) const
{
    // Use Trinity's map system for validation
    Map* map = _bot->GetMap();
    if (!map)
        return false;
    
    // Check if position is valid
    if (!Trinity::IsValidMapCoord(pos.GetPositionX(), pos.GetPositionY()))
        return false;
    
    // Check if position is in world
    float groundZ = map->GetHeight(
        _bot->GetPhaseShift(),
        pos.GetPositionX(),
        pos.GetPositionY(),
        pos.GetPositionZ()
    );
    
    if (groundZ == INVALID_HEIGHT)
        return false;
    
    // Check for water/lava
    LiquidData liquidData;
    ZLiquidStatus liquidStatus = map->GetLiquidStatus(
        _bot->GetPhaseShift(),
        pos.GetPositionX(),
        pos.GetPositionY(),
        pos.GetPositionZ(),
        MAP_ALL_LIQUIDS,
        &liquidData
    );
    
    if (liquidStatus == LIQUID_MAP_UNDER_WATER && !_bot->CanSwim())
        return false;
    
    return true;
}
```

---

**Next Steps**:
1. Implement `BotMovementAI` class
2. Set up pathfinding cache
3. Create movement generators
4. Add obstacle avoidance
5. Test with 100 bots simultaneously