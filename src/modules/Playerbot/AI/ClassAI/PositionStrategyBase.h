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
#include "Position.h"
#include "../../Movement/Core/MovementTypes.h"
#include <vector>
#include <array>
#include <atomic>
#include <memory>
#include <optional>
#include <span>

class Player;
class Unit;
class Map;

namespace Playerbot
{
    // MovementFormationType defined in IUnifiedMovementCoordinator.h
    // MovementPriority defined in Movement/Core/MovementTypes.h

// Spatial grid cell for efficient position tracking
struct GridCell
{
    ::std::atomic<uint16_t> occupantCount{0};
    ::std::atomic<uint32_t> lastUpdate{0};
    ::std::atomic<float> dangerLevel{0.0f};

    bool IsOccupied() const { return occupantCount.load(::std::memory_order_acquire) > 0; }
    bool IsSafe() const { return dangerLevel.load(::std::memory_order_acquire) < 0.3f; }
};

// Position request for batch processing
struct PositionRequest
{
    Player* bot;
    Unit* target;
    float preferredRange;
    MovementPriority priority;
    uint32 requestTime;
    Position suggestedPosition;
};

// Position cache entry for performance
struct CachedPosition
{
    Position position;
    uint32 calculatedTime;
    float score;
    bool isValid;
};

// High-performance position strategy base class
class TC_GAME_API PositionStrategyBase
{
public:
    explicit PositionStrategyBase(Map* map);
    virtual ~PositionStrategyBase() = default;

    // Core positioning interface
    virtual Position CalculateOptimalPosition(Player* bot, Unit* target, float preferredRange);
    virtual bool ValidatePosition(const Position& pos, Player* bot) const;
    virtual float EvaluatePositionScore(const Position& pos, Player* bot, Unit* target) const;

    // Batch position calculation for multiple bots (optimized for 5000+ bots)
    virtual ::std::vector<Position> CalculateBatchPositions(
        ::std::span<PositionRequest> requests,
        MovementFormationType formation = MovementFormationType::SPREAD);

    // Formation management
    virtual void SetFormation(MovementFormationType type) { _formationType = type; }
    virtual MovementFormationType GetFormation() const { return _formationType; }
    virtual void UpdateFormationPositions(::std::vector<Player*> bots, Unit* centerTarget);

    // Collision avoidance and spatial management
    virtual bool IsPositionOccupied(const Position& pos, float radius = 2.0f) const;
    virtual void RegisterPosition(Player* bot, const Position& pos);
    virtual void UnregisterPosition(Player* bot);
    virtual void ClearAllPositions();

    // Danger zone management (AoE avoidance)
    virtual void AddDangerZone(const Position& center, float radius, float duration, float dangerLevel = 1.0f);
    virtual void RemoveDangerZone(const Position& center);
    virtual bool IsPositionSafe(const Position& pos) const;
    virtual float GetDangerLevel(const Position& pos) const;
    virtual void UpdateDangerZones(uint32 diff);

    // Path optimization
    virtual ::std::vector<Position> CalculatePath(
        const Position& start,
        const Position& end,
        bool avoidDanger = true);

    virtual float CalculatePathLength(const ::std::vector<Position>& path) const;
    virtual bool IsPathClear(const Position& start, const Position& end) const;

    // Performance monitoring
    struct PerformanceStats
    {
        ::std::atomic<uint64_t> positionsCalculated{0};
        ::std::atomic<uint64_t> pathsCalculated{0};
        ::std::atomic<uint64_t> collisionChecks{0};
        ::std::atomic<uint64_t> cacheHits{0};
        ::std::atomic<uint64_t> cacheMisses{0};
        ::std::atomic<uint32_t> averageCalculationTimeUs{0};
        ::std::atomic<uint32_t> peakBotsProcessed{0};
    };

    const PerformanceStats& GetStats() const { return _stats; }
    void ResetStats();

    // Utility methods
    static float GetOptimalMeleeRange() { return 3.0f; }
    static float GetOptimalRangedRange() { return 25.0f; }
    static float GetMinimumSpacing() { return 2.0f; }
    static float GetMaximumRange() { return 40.0f; }

protected:
    // Grid-based spatial indexing for O(1) position lookups
    static constexpr uint32 GRID_SIZE = 256;  // 256x256 grid
    static constexpr float GRID_CELL_SIZE = 4.0f;  // 4 yards per cell

    using SpatialGrid = ::std::array<::std::array<GridCell, GRID_SIZE>, GRID_SIZE>;

    // Convert world position to grid coordinates
    ::std::pair<uint32, uint32> WorldToGrid(const Position& pos) const;
    Position GridToWorld(uint32 x, uint32 y) const;

    // Internal positioning algorithms
    Position CalculateMeleePosition(Player* bot, Unit* target);
    Position CalculateRangedPosition(Player* bot, Unit* target);
    Position CalculateTankPosition(Player* bot, Unit* target);
    Position CalculateHealerPosition(Player* bot, ::std::vector<Player*> allies);

    // Formation calculations
    ::std::vector<Position> CalculateLineFormation(
        ::std::vector<Player*> bots,
        Unit* target,
        float spacing = 3.0f);

    ::std::vector<Position> CalculateWedgeFormation(
        ::std::vector<Player*> bots,
        Unit* target,
        float angle = 45.0f);

    ::std::vector<Position> CalculateCircleFormation(
        ::std::vector<Player*> bots,
        Unit* target,
        float radius = 15.0f);

    ::std::vector<Position> CalculateSpreadFormation(
        ::std::vector<Player*> bots,
        Unit* target,
        float minSpacing = 5.0f);

    // Collision detection helpers
    bool CheckCollisionWithTerrain(const Position& pos) const;
    bool CheckCollisionWithObjects(const Position& pos, float radius) const;
    bool CheckCollisionWithOtherBots(const Position& pos, Player* excludeBot = nullptr) const;

    // Score calculation factors
    float CalculateDistanceScore(const Position& pos, Unit* target, float optimalRange) const;
    float CalculateSafetyScore(const Position& pos) const;
    float CalculateTerrainScore(const Position& pos) const;
    float CalculateGroupCohesionScore(const Position& pos, Player* bot) const;

    // A* pathfinding node
    struct PathNode
    {
        Position pos;
        float g_cost;  // Cost from start
        float h_cost;  // Heuristic to end
        float f_cost() const { return g_cost + h_cost; }
        PathNode* parent;
    };

    // Optimized A* implementation
    ::std::vector<Position> FindPathAStar(
        const Position& start,
        const Position& end,
        bool avoidDanger);

protected:
    Map* _map;
    MovementFormationType _formationType;

    // Spatial grid for collision detection
    ::std::unique_ptr<SpatialGrid> _spatialGrid;

    // Position tracking
    ::std::unordered_map<uint64_t, Position> _botPositions;  // Bot GUID -> Position
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _positionMutex;

    // Danger zones
    struct DangerZone
    {
        Position center;
        float radius;
        float dangerLevel;
        uint32 expirationTime;
    };
    ::std::vector<DangerZone> _dangerZones;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _dangerMutex;

    // Position cache for performance
    struct PositionCache
    {
        ::std::unordered_map<uint64_t, CachedPosition> entries;
        uint32 lastCleanup;
    };
    mutable PositionCache _cache;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _cacheMutex;

    // Performance statistics
    mutable PerformanceStats _stats;

    // Configuration
    bool _useAdvancedPathfinding;
    bool _enableCollisionAvoidance;
    bool _enableDangerAvoidance;
    uint32 _maxPathNodes;
    float _pathSmoothingFactor;

private:
    // Cache management
    void CleanupCache();
    ::std::optional<CachedPosition> GetCachedPosition(Player* bot, Unit* target) const;
    void CachePosition(Player* bot, Unit* target, const Position& pos, float score);

    // Grid management
    void UpdateGridCell(uint32 x, uint32 y, int32 deltaOccupants);
    void UpdateGridDanger(uint32 x, uint32 y, float dangerLevel);
    void ClearGrid();

    // Performance measurement
    void RecordCalculationTime(uint64_t microseconds);
    void UpdatePeakBots(uint32 botCount);
};

// Specialized position strategies for different combat scenarios
class MeleePositionStrategy : public PositionStrategyBase
{
public:
    explicit MeleePositionStrategy(Map* map) : PositionStrategyBase(map) {}

    Position CalculateOptimalPosition(Player* bot, Unit* target, float preferredRange);
    float EvaluatePositionScore(const Position& pos, Player* bot, Unit* target) const;

private:
    Position GetBackstabPosition(Unit* target) const;
    Position GetFlankPosition(Unit* target, bool leftSide) const;
    bool CanReachPosition(Player* bot, const Position& pos, float timeLimit) const;
};

class RangedPositionStrategy : public PositionStrategyBase
{
public:
    explicit RangedPositionStrategy(Map* map) : PositionStrategyBase(map) {}

    Position CalculateOptimalPosition(Player* bot, Unit* target, float preferredRange);
    float EvaluatePositionScore(const Position& pos, Player* bot, Unit* target) const;

private:
    Position GetKitingPosition(Player* bot, Unit* target) const;
    Position GetHighGroundPosition(const Position& current) const;
    bool HasLineOfSight(const Position& from, const Position& to) const;
};

class TankPositionStrategy : public PositionStrategyBase
{
public:
    explicit TankPositionStrategy(Map* map) : PositionStrategyBase(map) {}

    Position CalculateOptimalPosition(Player* bot, Unit* target, float preferredRange);
    void UpdateFormationPositions(::std::vector<Player*> bots, Unit* centerTarget);

private:
    Position GetTankingPosition(Unit* target, bool mainTank = true) const;
    void PositionForCleave(Player* bot, ::std::vector<Unit*> enemies);
    float CalculateThreatPosition(Player* bot, Unit* target) const;
};

class HealerPositionStrategy : public PositionStrategyBase
{
public:
    explicit HealerPositionStrategy(Map* map) : PositionStrategyBase(map) {}

    Position CalculateOptimalPosition(Player* bot, Unit* target, float preferredRange);
    float EvaluatePositionScore(const Position& pos, Player* bot, Unit* target) const;

private:
    Position GetSafeHealingPosition(Player* bot, ::std::vector<Player*> allies) const;
    bool CanReachAllAllies(const Position& pos, ::std::vector<Player*> allies) const;
    float GetAverageAllyDistance(const Position& pos, ::std::vector<Player*> allies) const;
};

} // namespace Playerbot