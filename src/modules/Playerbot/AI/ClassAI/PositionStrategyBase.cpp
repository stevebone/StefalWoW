/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PositionStrategyBase.h"
#include "GameTime.h"
#include "Player.h"
#include "Unit.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "MoveSplineInit.h"
#include "PathGenerator.h"
#include "Log.h"
#include <execution>
#include <queue>
#include <chrono>
#include <cmath>
#include "../../Spatial/SpatialGridQueryHelpers.h"  // PHASE 5F: Thread-safe queries

namespace Playerbot
{

// Constants for positioning calculations
constexpr float POSITION_TOLERANCE = 0.5f;
constexpr float HEIGHT_SEARCH_RANGE = 10.0f;
constexpr uint32 CACHE_DURATION_MS = 500;
constexpr uint32 MAX_PATH_LENGTH = 100;
constexpr float DANGER_DECAY_RATE = 0.1f;

PositionStrategyBase::PositionStrategyBase(Map* map)
    : _map(map)
    , _formationType(MovementFormationType::SPREAD)
    , _spatialGrid(::std::make_unique<SpatialGrid>())
    , _useAdvancedPathfinding(true)
    , _enableCollisionAvoidance(true)
    , _enableDangerAvoidance(true)
    , _maxPathNodes(1000)
    , _pathSmoothingFactor(0.5f)
{
    _botPositions.reserve(5000);  // Pre-allocate for 5000 bots
    _dangerZones.reserve(100);    // Pre-allocate danger zones
    _cache.entries.reserve(1000);  // Pre-allocate cache
    _cache.lastCleanup = GameTime::GetGameTimeMS();

    // Initialize spatial grid
    ClearGrid();
}

// Core position calculation with intelligent caching
Position PositionStrategyBase::CalculateOptimalPosition(Player* bot, Unit* target, float preferredRange)
{
    if (!bot || !target)
        return bot ? bot->GetPosition() : Position();

    auto startTime = ::std::chrono::high_resolution_clock::now();

    // Check cache first
    auto cached = GetCachedPosition(bot, target);
    if (cached.has_value())
    {
        _stats.cacheHits.fetch_add(1, ::std::memory_order_relaxed);
        return cached->position;
    }

    _stats.cacheMisses.fetch_add(1, ::std::memory_order_relaxed);

    // Determine position based on bot role
    Position optimalPos;
    float bestScore = -::std::numeric_limits<float>::max();

    // Generate candidate positions in a spiral pattern
    ::std::vector<Position> candidates;
    candidates.reserve(16);

    float angleStep = 2.0f * static_cast<float>(M_PI) / 8.0f;  // 8 positions around target
    for (int ring = 0; ring < 2; ++ring)
    {
        float range = preferredRange + (ring * 3.0f);
        for (int i = 0; i < 8; ++i)
        {

            float angle = i * angleStep;

            float x = target->GetPositionX() + cos(angle) * range;
            float y = target->GetPositionY() + sin(angle) * range;
            float z = target->GetPositionZ();
            // Validate height

            _map->GetHeight(bot->GetPhaseShift(), x, y, z);


            Position candidate(x, y, z, angle);

            if (ValidatePosition(candidate, bot))
            {

                candidates.push_back(candidate);

            }
        }
    }

    // Evaluate each candidate position
    for (const auto& pos : candidates)
    {
        float score = EvaluatePositionScore(pos, bot, target);
        if (score > bestScore)
        {

            bestScore = score;

            optimalPos = pos;
        }
    }

    // If no valid position found, use current position
    if (bestScore == -::std::numeric_limits<float>::max())
    {
        optimalPos = bot->GetPosition();    }

    // Cache the result
    CachePosition(bot, target, optimalPos, bestScore);

    // Update statistics
    auto endTime = ::std::chrono::high_resolution_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime);
    RecordCalculationTime(duration.count());
    _stats.positionsCalculated.fetch_add(1, ::std::memory_order_relaxed);

    return optimalPos;
}

// Batch position calculation for multiple bots (optimized for massive scale)
::std::vector<Position> PositionStrategyBase::CalculateBatchPositions(
    ::std::span<PositionRequest> requests,
    MovementFormationType formation)
{
    if (requests.empty())
        return {};

    UpdatePeakBots(requests.size());

    ::std::vector<Position> results;
    results.reserve(requests.size());

    // Sort requests by priority
    ::std::vector<PositionRequest*> sortedRequests;
    sortedRequests.reserve(requests.size());
    for (auto& req : requests)
        sortedRequests.push_back(&req);

    ::std::sort(sortedRequests.begin(), sortedRequests.end(),
        [](const PositionRequest* a, const PositionRequest* b) {

            return static_cast<uint8_t>(a->priority) > static_cast<uint8_t>(b->priority);
        });

    // Process high-priority requests first
    ::std::unordered_set<::std::pair<uint32, uint32>, boost::hash<::std::pair<uint32, uint32>>> occupiedCells;

    // Use parallel execution for large batches
    if (sortedRequests.size() > 100)
    {
        ::std::recursive_mutex resultMutex;
        ::std::for_each(::std::execution::par_unseq,

            sortedRequests.begin(), sortedRequests.end(),

            [this, &results, &resultMutex, &occupiedCells](PositionRequest* req) {

                Position pos = CalculateOptimalPosition(req->bot, req->target, req->preferredRange);

                // Thread-safe result insertion

                ::std::lock_guard lock(resultMutex);

                results.push_back(pos);

                // Mark grid cell as occupied

                auto gridCoords = WorldToGrid(pos);

                occupiedCells.insert(gridCoords);

            });
    }
    else
    {
        // Sequential processing for smaller batches
    for (auto* req : sortedRequests)
        {

            Position pos = CalculateOptimalPosition(req->bot, req->target, req->preferredRange);
            // Check for collision with already assigned positions

            auto gridCoords = WorldToGrid(pos);

            if (occupiedCells.count(gridCoords) > 0)

            {
                // Find alternative position

                pos = FindAlternativePosition(pos, req->bot, occupiedCells);

            }


            results.push_back(pos);

            occupiedCells.insert(WorldToGrid(pos));

            // Register position for collision detection

            RegisterPosition(req->bot, pos);
        }
    }

    return results;
}

// Position validation with comprehensive checks
bool PositionStrategyBase::ValidatePosition(const Position& pos, Player* bot) const
{
    if (!bot || !_map)
        return false;

    // Check map bounds
    if (!_map->IsInLineOfSight(
        bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ() + 2.0f,
        pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ() + 2.0f,
        bot->GetPhaseShift()))
    {
        return false;
    }

    // Check terrain
    if (CheckCollisionWithTerrain(pos))
        return false;

    // Check danger zones
    if (_enableDangerAvoidance && !IsPositionSafe(pos))
        return false;    // Check collision with other bots
    if (_enableCollisionAvoidance && CheckCollisionWithOtherBots(pos, bot))
        return false;

    return true;
}

// Comprehensive position scoring
float PositionStrategyBase::EvaluatePositionScore(const Position& pos, Player* bot, Unit* target) const
{
    float score = 100.0f;

    // Distance score (prefer optimal range)
    float optimalRange = (bot->GetClass() == CLASS_WARRIOR || bot->GetClass() == CLASS_ROGUE) ?        GetOptimalMeleeRange() : GetOptimalRangedRange();
    score += CalculateDistanceScore(pos, target, optimalRange);    // Safety score (avoid danger zones)
    score += CalculateSafetyScore(pos) * 2.0f;  // Double weight for safety

    // Terrain score (prefer flat, accessible terrain)
    score += CalculateTerrainScore(pos);    // Group cohesion score (stay near allies but not too close)
    score += CalculateGroupCohesionScore(pos, bot);

    // Line of sight bonus
    if (_map->IsInLineOfSight(
        pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ() + 2.0f,
        target->GetPositionX(), target->GetPositionY(), target->GetPositionZ() + 2.0f,
        bot->GetPhaseShift()))
    {
        score += 20.0f;
    }

    return score;
}

// Grid coordinate conversion
::std::pair<uint32, uint32> PositionStrategyBase::WorldToGrid(const Position& pos) const
{
    // Normalize to positive coordinates
    float x = pos.GetPositionX() + (GRID_SIZE * GRID_CELL_SIZE / 2.0f);
    float y = pos.GetPositionY() + (GRID_SIZE * GRID_CELL_SIZE / 2.0f);

    uint32 gridX = static_cast<uint32>(x / GRID_CELL_SIZE);
    uint32 gridY = static_cast<uint32>(y / GRID_CELL_SIZE);

    // Clamp to grid bounds
    gridX = ::std::min(gridX, GRID_SIZE - 1);
    gridY = ::std::min(gridY, GRID_SIZE - 1);

    return {gridX, gridY};
}

// Position registration for collision detection
void PositionStrategyBase::RegisterPosition(Player* bot, const Position& pos)
{
    if (!bot)
        return;

    ::std::lock_guard lock(_positionMutex);

    uint64 guid = bot->GetGUID().GetRawValue();    // Unregister old position
    auto oldIt = _botPositions.find(guid);
    if (oldIt != _botPositions.end())
    {
        auto oldGrid = WorldToGrid(oldIt->second);
        UpdateGridCell(oldGrid.first, oldGrid.second, -1);
    }

    // Register new position
    _botPositions[guid] = pos;
    auto newGrid = WorldToGrid(pos);
    UpdateGridCell(newGrid.first, newGrid.second, 1);
}

void PositionStrategyBase::UnregisterPosition(Player* bot)
{
    if (!bot)
        return;

    ::std::lock_guard lock(_positionMutex);

    uint64 guid = bot->GetGUID().GetRawValue();
    auto it = _botPositions.find(guid);
    if (it != _botPositions.end())
    {
        auto grid = WorldToGrid(it->second);
        UpdateGridCell(grid.first, grid.second, -1);
        _botPositions.erase(it);
    }
}

// Danger zone management
void PositionStrategyBase::AddDangerZone(const Position& center, float radius, float duration, float dangerLevel)
{
    ::std::lock_guard lock(_dangerMutex);

    DangerZone zone;
    zone.center = center;
    zone.radius = radius;
    zone.dangerLevel = dangerLevel;
    zone.expirationTime = GameTime::GetGameTimeMS() + static_cast<uint32>(duration * 1000);

    _dangerZones.push_back(zone);

    // Update grid danger levels
    int32 gridRadius = static_cast<int32>(radius / GRID_CELL_SIZE) + 1;
    auto centerGrid = WorldToGrid(center);

    for (int32 dx = -gridRadius; dx <= gridRadius; ++dx)
    {
        for (int32 dy = -gridRadius; dy <= gridRadius; ++dy)
        {

            int32 x = centerGrid.first + dx;

            int32 y = centerGrid.second + dy;


            if (x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE)

            {

                float cellDistance = ::std::sqrt(dx * dx + dy * dy) * GRID_CELL_SIZE;

                if (cellDistance <= radius)

                {

                    float cellDanger = dangerLevel * (1.0f - cellDistance / radius);

                    UpdateGridDanger(x, y, cellDanger);

                }

            }
        }
    }
}

bool PositionStrategyBase::IsPositionSafe(const Position& pos) const
{
    return GetDangerLevel(pos) < 0.3f;
}

float PositionStrategyBase::GetDangerLevel(const Position& pos) const
{
    // Check grid danger
    auto grid = WorldToGrid(pos);
    float gridDanger = (*_spatialGrid)[grid.first][grid.second].dangerLevel.load(::std::memory_order_acquire);

    // Check specific danger zones
    ::std::lock_guard lock(_dangerMutex);
    float maxDanger = gridDanger;

    for (const auto& zone : _dangerZones)
    {
        float distance = pos.GetExactDist(&zone.center);
        if (distance <= zone.radius)
        {

            float zoneDanger = zone.dangerLevel * (1.0f - distance / zone.radius);

            maxDanger = ::std::max(maxDanger, zoneDanger);
        }
    }

    return maxDanger;
}

void PositionStrategyBase::UpdateDangerZones(uint32 diff)
{
    ::std::lock_guard lock(_dangerMutex);

    uint32 currentTime = GameTime::GetGameTimeMS();

    // Remove expired danger zones
    _dangerZones.erase(
        ::std::remove_if(_dangerZones.begin(), _dangerZones.end(),

            [currentTime](const DangerZone& zone) {

                return zone.expirationTime <= currentTime;

            }),
        _dangerZones.end());

    // Decay grid danger levels
    for (uint32 x = 0; x < GRID_SIZE; ++x)
    {
        for (uint32 y = 0; y < GRID_SIZE; ++y)
        {

            auto& cell = (*_spatialGrid)[x][y];

            float currentDanger = cell.dangerLevel.load(::std::memory_order_acquire);

            if (currentDanger > 0.0f)

            {

                float newDanger = ::std::max(0.0f, currentDanger - DANGER_DECAY_RATE * diff / 1000.0f);

                cell.dangerLevel.store(newDanger, ::std::memory_order_release);

            }
        }
    }
}

// A* pathfinding implementation
::std::vector<Position> PositionStrategyBase::CalculatePath(
    const Position& start,
    const Position& end,
    bool avoidDanger)
{
    if (!_useAdvancedPathfinding)
    {
        // Simple direct path
        return {start, end};
    }

    auto startTime = ::std::chrono::high_resolution_clock::now();

    ::std::vector<Position> path = FindPathAStar(start, end, avoidDanger);

    // Smooth the path
    if (path.size() > 2 && _pathSmoothingFactor > 0.0f)
    {
        path = SmoothPath(path);
    }

    auto endTime = ::std::chrono::high_resolution_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime);
    RecordCalculationTime(duration.count());
    _stats.pathsCalculated.fetch_add(1, ::std::memory_order_relaxed);

    return path;
}

::std::vector<Position> PositionStrategyBase::FindPathAStar(
    const Position& start,
    const Position& end,
    bool avoidDanger)
{
    // Node comparison for priority queue
    auto nodeCompare = [](const PathNode* a, const PathNode* b) {
        return a->f_cost() > b->f_cost();
    };

    ::std::priority_queue<PathNode*, ::std::vector<PathNode*>, decltype(nodeCompare)> openSet(nodeCompare);
    ::std::unordered_set<::std::pair<uint32, uint32>, boost::hash<::std::pair<uint32, uint32>>> closedSet;
    ::std::vector<::std::unique_ptr<PathNode>> nodePool;  // Memory management

    // Create start node
    auto startNode = ::std::make_unique<PathNode>();
    startNode->pos = start;
    startNode->g_cost = 0.0f;
    startNode->h_cost = start.GetExactDist(&end);
    startNode->parent = nullptr;

    PathNode* startPtr = startNode.get();
    nodePool.push_back(::std::move(startNode));
    openSet.push(startPtr);

    uint32 nodesProcessed = 0;

    while (!openSet.empty() && nodesProcessed < _maxPathNodes)
    {
        PathNode* current = openSet.top();
        openSet.pop();
        nodesProcessed++;

        // Check if we reached the goal
    if (current->pos.GetExactDist(&end) < POSITION_TOLERANCE)
        {
            // Reconstruct path

            ::std::vector<Position> path;

            while (current != nullptr)

            {

                path.push_back(current->pos);

                current = current->parent;

            }

            ::std::reverse(path.begin(), path.end());

            return path;
        }

        auto currentGrid = WorldToGrid(current->pos);
        closedSet.insert(currentGrid);

        // Generate neighbors (8 directions)
    for (int dx = -1; dx <= 1; ++dx)
        {

            for (int dy = -1; dy <= 1; ++dy)

            {

                if (dx == 0 && dy == 0)

                    continue;


                int32 nx = currentGrid.first + dx;

                int32 ny = currentGrid.second + dy;


                if (nx < 0 || nx >= GRID_SIZE || ny < 0 || ny >= GRID_SIZE)

                    continue;


                ::std::pair<uint32, uint32> neighborGrid = {static_cast<uint32>(nx), static_cast<uint32>(ny)};


                if (closedSet.count(neighborGrid) > 0)

                    continue;


                Position neighborPos = GridToWorld(nx, ny);

                float z = neighborPos.GetPositionZ();
                _map->GetHeight(nullptr, neighborPos.GetPositionX(), neighborPos.GetPositionY(), z);

                neighborPos.Relocate(neighborPos.GetPositionX(), neighborPos.GetPositionY(), z);

                // Check if position is valid
    if (!ValidatePosition(neighborPos, nullptr))

                    continue;

                // Calculate costs                float moveCost = current->pos.GetExactDist(&neighborPos);                // Add danger cost if avoiding danger
    if (avoidDanger)

                {

                    float dangerLevel = GetDangerLevel(neighborPos);

                    moveCost += dangerLevel * 10.0f;  // Heavy penalty for dangerous areas

                }


                float g_cost = current->g_cost + moveCost;

                float h_cost = neighborPos.GetExactDist(&end);

                // Create new node

                auto newNode = ::std::make_unique<PathNode>();

                newNode->pos = neighborPos;

                newNode->g_cost = g_cost;

                newNode->h_cost = h_cost;

                newNode->parent = current;


                PathNode* newPtr = newNode.get();

                nodePool.push_back(::std::move(newNode));

                openSet.push(newPtr);

            }
        }
    }

    // No path found, return direct path    return {start, end};}// Formation calculations
::std::vector<Position> PositionStrategyBase::CalculateLineFormation(
    ::std::vector<Player*> bots,
    Unit* target,    float spacing)
{
    ::std::vector<Position> positions;
    positions.reserve(bots.size());

    if (bots.empty() || !target)
        return positions;

    float angle = target->GetOrientation();
    if (!target)
    {
        if (!excludeBot)
        {

            return nullptr;
        }
        return;
    }
    float perpAngle = angle + static_cast<float>(M_PI) / 2.0f;

    int32 halfCount = bots.size() / 2;

    for (size_t i = 0; i < bots.size(); ++i)
    {
        int32 offset = static_cast<int32>(i) - halfCount;
        float x = target->GetPositionX() + cos(perpAngle) * offset * spacing;
        float y = target->GetPositionY() + sin(perpAngle) * offset * spacing;        float z = target->GetPositionZ();        _map->GetHeight(bots[i]->GetPhaseShift(), x, y, z);
        positions.emplace_back(x, y, z, angle);
    }

    return positions;
}

::std::vector<Position> PositionStrategyBase::CalculateCircleFormation(
    ::std::vector<Player*> bots,
    Unit* target,
    float radius)
{
    ::std::vector<Position> positions;
    positions.reserve(bots.size());

    if (bots.empty() || !target)
        return positions;

    float angleStep = 2.0f * static_cast<float>(M_PI) / bots.size();

    for (size_t i = 0; i < bots.size(); ++i)    {
        float angle = i * angleStep;
        float x = target->GetPositionX() + cos(angle) * radius;
        float y = target->GetPositionY() + sin(angle) * radius;        float z = target->GetPositionZ();        _map->GetHeight(bots[i]->GetPhaseShift(), x, y, z);

        // Face the target
        float facing = atan2(target->GetPositionY() - y, target->GetPositionX() - x);
        positions.emplace_back(x, y, z, facing);
    }

    return positions;
}

// Collision detection
bool PositionStrategyBase::CheckCollisionWithOtherBots(const Position& pos, Player* excludeBot) const
{
    ::std::lock_guard lock(_positionMutex);
    uint64 excludeGuid = excludeBot ? excludeBot->GetGUID().GetRawValue() : 0;
    if (!excludeBot)
    {
        return;
    }

    for (const auto& [guid, botPos] : _botPositions)
    {
        if (guid == excludeGuid)

            continue;

        if (pos.GetExactDist(&botPos) < GetMinimumSpacing())

            return true;
    }

    return false;
}
// Score calculation helpers
float PositionStrategyBase::CalculateDistanceScore(const Position& pos, Unit* target, float optimalRange) const
{
    float distance = pos.GetExactDist(target);
    float diff = ::std::abs(distance - optimalRange);

    // Gaussian-like scoring: best at optimal range, decreases with distance
    return 50.0f * ::std::exp(-diff * diff / (2.0f * 5.0f * 5.0f));
}

float PositionStrategyBase::CalculateSafetyScore(const Position& pos) const
{
    float dangerLevel = GetDangerLevel(pos);
    return 30.0f * (1.0f - dangerLevel);
}

float PositionStrategyBase::CalculateTerrainScore(const Position& pos) const
{
    // Check if position is on even terrain (not steep slopes)
    float z1 = pos.GetPositionZ();
    float z2 = z1, z3 = z1, z4 = z1;

    _map->GetHeight(nullptr, pos.GetPositionX() + 1.0f, pos.GetPositionY(), z2);
    _map->GetHeight(nullptr, pos.GetPositionX() - 1.0f, pos.GetPositionY(), z3);
    _map->GetHeight(nullptr, pos.GetPositionX(), pos.GetPositionY() + 1.0f, z4);

    float maxDiff = ::std::max({::std::abs(z1 - z2), ::std::abs(z1 - z3), ::std::abs(z1 - z4)});

    // Prefer flatter terrain
    return 20.0f * ::std::exp(-maxDiff);
}

float PositionStrategyBase::CalculateGroupCohesionScore(const Position& pos, Player* bot) const
{
    ::std::lock_guard lock(_positionMutex);

    float totalDistance = 0.0f;
    uint32 allyCount = 0;

    for (const auto& [guid, allyPos] : _botPositions)
    {

        if (guid == bot->GetGUID().GetRawValue())
        continue;

        float distance = pos.GetExactDist(&allyPos);
        if (distance < 40.0f)  // Consider allies within 40 yards
        {

            totalDistance += distance;

            allyCount++;
        }
    }

    if (allyCount == 0)
        return 0.0f;

    float avgDistance = totalDistance / allyCount;

    // Optimal cohesion distance is 10-15 yards
    if (avgDistance >= 10.0f && avgDistance <= 15.0f)
        return 15.0f;

    if (avgDistance < 10.0f)
        return 15.0f * (avgDistance / 10.0f);  // Too close
    if (avgDistance > 15.0f)
        return 15.0f * ::std::exp(-(avgDistance - 15.0f) / 10.0f);  // Too far

    return 0.0f;
}

// Cache management
::std::optional<CachedPosition> PositionStrategyBase::GetCachedPosition(Player* bot, Unit* target) const
{
    ::std::lock_guard lock(_cacheMutex);

    uint64 key = (bot->GetGUID().GetRawValue() << 32) | target->GetGUID().GetRawValue();
    auto it = _cache.entries.find(key);

    if (it != _cache.entries.end())
    {
        uint32 currentTime = GameTime::GetGameTimeMS();
        if (currentTime - it->second.calculatedTime < CACHE_DURATION_MS)
        {

            return it->second;
        }
    }

    return ::std::nullopt;
}

void PositionStrategyBase::CachePosition(Player* bot, Unit* target, const Position& pos, float score)
{
    ::std::lock_guard lock(_cacheMutex);

    uint64 key = (bot->GetGUID().GetRawValue() << 32) | target->GetGUID().GetRawValue();
    CachedPosition cached;
    cached.position = pos;
    cached.calculatedTime = GameTime::GetGameTimeMS();
    cached.score = score;
    cached.isValid = true;

    _cache.entries[key] = cached;

    // Periodic cleanup
    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime - _cache.lastCleanup > 5000)  // Every 5 seconds
    {
        CleanupCache();
        _cache.lastCleanup = currentTime;
    }
}

void PositionStrategyBase::CleanupCache()
{
    uint32 currentTime = GameTime::GetGameTimeMS();

    ::std::erase_if(_cache.entries, [currentTime](const auto& pair) {
        return currentTime - pair.second.calculatedTime > CACHE_DURATION_MS * 10;
    });
}

// Grid management
void PositionStrategyBase::UpdateGridCell(uint32 x, uint32 y, int32 deltaOccupants)
{
    if (x >= GRID_SIZE || y >= GRID_SIZE)
        return;

    auto& cell = (*_spatialGrid)[x][y];
    if (deltaOccupants > 0)
    {
        cell.occupantCount.fetch_add(deltaOccupants, ::std::memory_order_acq_rel);
    }
    else
    {
        uint16_t current = cell.occupantCount.load(::std::memory_order_acquire);
        uint16_t newValue = (current > -deltaOccupants) ? current + deltaOccupants : 0;
        cell.occupantCount.store(newValue, ::std::memory_order_release);
    }
    cell.lastUpdate.store(GameTime::GetGameTimeMS(), ::std::memory_order_release);
}

void PositionStrategyBase::UpdateGridDanger(uint32 x, uint32 y, float dangerLevel)
{
    if (x >= GRID_SIZE || y >= GRID_SIZE)
        return;

    auto& cell = (*_spatialGrid)[x][y];
    float current = cell.dangerLevel.load(::std::memory_order_acquire);
    float newDanger = ::std::max(current, dangerLevel);
    cell.dangerLevel.store(newDanger, ::std::memory_order_release);
}

void PositionStrategyBase::ClearGrid()
{
    for (uint32 x = 0; x < GRID_SIZE; ++x)
    {
        for (uint32 y = 0; y < GRID_SIZE; ++y)
        {

            auto& cell = (*_spatialGrid)[x][y];

            cell.occupantCount.store(0, ::std::memory_order_release);

            cell.dangerLevel.store(0.0f, ::std::memory_order_release);

            cell.lastUpdate.store(0, ::std::memory_order_release);
        }
    }
}

Position PositionStrategyBase::GridToWorld(uint32 x, uint32 y) const
{
    float worldX = x * GRID_CELL_SIZE - (GRID_SIZE * GRID_CELL_SIZE / 2.0f);
    float worldY = y * GRID_CELL_SIZE - (GRID_SIZE * GRID_CELL_SIZE / 2.0f);
    float worldZ = 0.0f;  // Will be calculated later

    return Position(worldX, worldY, worldZ, 0.0f);
}

// Performance tracking
void PositionStrategyBase::RecordCalculationTime(uint64_t microseconds)
{
    // Update average using exponential moving average
    uint32 current = _stats.averageCalculationTimeUs.load(::std::memory_order_acquire);
    uint32 newAvg = static_cast<uint32>(current * 0.9 + microseconds * 0.1);
    _stats.averageCalculationTimeUs.store(newAvg, ::std::memory_order_release);
}

void PositionStrategyBase::UpdatePeakBots(uint32 botCount)
{
    uint32 current = _stats.peakBotsProcessed.load(::std::memory_order_acquire);
    if (botCount > current)
    {
        _stats.peakBotsProcessed.store(botCount, ::std::memory_order_release);
    }
}

void PositionStrategyBase::ResetStats()
{
    _stats.positionsCalculated.store(0, ::std::memory_order_release);
    _stats.pathsCalculated.store(0, ::std::memory_order_release);
    _stats.collisionChecks.store(0, ::std::memory_order_release);
    _stats.cacheHits.store(0, ::std::memory_order_release);
    _stats.cacheMisses.store(0, ::std::memory_order_release);
    _stats.averageCalculationTimeUs.store(0, ::std::memory_order_release);
    _stats.peakBotsProcessed.store(0, ::std::memory_order_release);
}

// Utility functions
bool PositionStrategyBase::CheckCollisionWithTerrain(const Position& pos) const
{
    // Check if position is underwater or in unreachable areas
    float waterLevel = _map->GetWaterLevel(pos.GetPositionX(), pos.GetPositionY());
    if (waterLevel > pos.GetPositionZ() + 2.0f)
        return true;  // Too deep underwater

    // Check for valid ground
    float groundZ = pos.GetPositionZ();
    if (!_map->GetHeight(nullptr, pos.GetPositionX(), pos.GetPositionY(), groundZ))
        return true;  // No valid ground

    return false;
}

bool PositionStrategyBase::IsPathClear(const Position& start, const Position& end) const
{
    return _map->IsInLineOfSight(
        start.GetPositionX(), start.GetPositionY(), start.GetPositionZ() + 2.0f,
        end.GetPositionX(), end.GetPositionY(), end.GetPositionZ() + 2.0f,
        nullptr);
}

float PositionStrategyBase::CalculatePathLength(const ::std::vector<Position>& path) const
{
    if (path.size() < 2)
        return 0.0f;

    float totalLength = 0.0f;
    for (size_t i = 1; i < path.size(); ++i)
    {
        totalLength += path[i - 1].GetExactDist(&path[i]);
    }

    return totalLength;
}
Position PositionStrategyBase::FindAlternativePosition(
    const Position& original,
    Player* bot,
    const ::std::unordered_set<::std::pair<uint32, uint32>, boost::hash<::std::pair<uint32, uint32>>>& occupiedCells)
{
    // Search in expanding circles for unoccupied position
    for (int radius = 1; radius <= 3; ++radius)
    {
        for (int dx = -radius; dx <= radius; ++dx)
        {

            for (int dy = -radius; dy <= radius; ++dy)

            {

                if (::std::abs(dx) != radius && ::std::abs(dy) != radius)

                    continue;  // Only check perimeter
                    float x = original.GetPositionX() + dx * GRID_CELL_SIZE;
                    float y = original.GetPositionY() + dy * GRID_CELL_SIZE;
                    float z = original.GetPositionZ();


                _map->GetHeight(bot->GetPhaseShift(), x, y, z);

                Position candidate(x, y, z, original.GetOrientation());


                auto gridCoords = WorldToGrid(candidate);

                if (occupiedCells.count(gridCoords) == 0 && ValidatePosition(candidate, bot))

                {

                    return candidate;

                }

            }
        }
    }

    return original;  // No alternative found
}

::std::vector<Position> PositionStrategyBase::SmoothPath(const ::std::vector<Position>& path)
{
    if (path.size() < 3)
        return path;

    ::std::vector<Position> smoothed;
    smoothed.reserve(path.size());
    smoothed.push_back(path[0]);

    for (size_t i = 1; i < path.size() - 1; ++i)
    {

        float x = path[i].GetPositionX() * (1.0f - _pathSmoothingFactor) +
        (path[i - 1].GetPositionX() + path[i + 1].GetPositionX()) * 0.5f * _pathSmoothingFactor;
        float y = path[i].GetPositionY() * (1.0f - _pathSmoothingFactor) +
        (path[i - 1].GetPositionY() + path[i + 1].GetPositionY()) * 0.5f * _pathSmoothingFactor;
        float z = path[i].GetPositionZ();

        _map->GetHeight(nullptr, x, y, z);
        smoothed.emplace_back(x, y, z, path[i].GetOrientation());    }

    smoothed.push_back(path.back());
    return smoothed;
}

void PositionStrategyBase::ClearAllPositions()
{
    ::std::lock_guard lock(_positionMutex);
    _botPositions.clear();
    ClearGrid();
}

void PositionStrategyBase::UpdateFormationPositions(::std::vector<Player*> bots, Unit* centerTarget)
{
    if (bots.empty() || !centerTarget)
        return;

    ::std::vector<Position> positions;

    switch (_formationType)
    {
        case MovementFormationType::LINE:

            positions = CalculateLineFormation(bots, centerTarget);

            break;
        case MovementFormationType::WEDGE:

            positions = CalculateWedgeFormation(bots, centerTarget);

            break;
        case MovementFormationType::CIRCLE:

            positions = CalculateCircleFormation(bots, centerTarget);

            break;
        case MovementFormationType::SPREAD:

            positions = CalculateSpreadFormation(bots, centerTarget);

            break;
        default:

            return;
    }

    // Apply positions to bots
    for (size_t i = 0; i < bots.size() && i < positions.size(); ++i)    {
        RegisterPosition(bots[i], positions[i]);
    }
}

::std::vector<Position> PositionStrategyBase::CalculateWedgeFormation(
    ::std::vector<Player*> bots,
    Unit* target,
    float angle)
{
    ::std::vector<Position> positions;
    positions.reserve(bots.size());

    if (bots.empty() || !target)
        return positions;

    float baseAngle = target->GetOrientation();
    uint32 rows = static_cast<uint32>(::std::sqrt(bots.size())) + 1;
    float rowSpacing = 5.0f;
    float angleRad = angle * static_cast<float>(M_PI) / 180.0f;

    size_t botIndex = 0;
    for (uint32 row = 0; row < rows && botIndex < bots.size(); ++row)
    {
        uint32 botsInRow = row + 1;
        float rowDistance = row * rowSpacing + 5.0f;

        for (uint32 col = 0; col < botsInRow && botIndex < bots.size(); ++col, ++botIndex)
        {

            float offset = (col - botsInRow / 2.0f) * angleRad / botsInRow;

            float finalAngle = baseAngle + offset;


            float x = target->GetPositionX() + cos(finalAngle) * rowDistance;
            float y = target->GetPositionY() + sin(finalAngle) * rowDistance;
            float z = target->GetPositionZ();
            _map->GetHeight(bots[botIndex]->GetPhaseShift(), x, y, z);

            positions.emplace_back(x, y, z, finalAngle);
        }
    }

    return positions;
}

::std::vector<Position> PositionStrategyBase::CalculateSpreadFormation(
    ::std::vector<Player*> bots,    Unit* target,
    float minSpacing)
{
    ::std::vector<Position> positions;
    positions.reserve(bots.size());

    if (bots.empty() || !target)
        return positions;

    // Use Fibonacci spiral for even distribution
    float goldenAngle = static_cast<float>(M_PI) * (3.0f - ::std::sqrt(5.0f));  // Golden angle in radians
    for (size_t i = 0; i < bots.size(); ++i)
    {
        float angle = i * goldenAngle;
        float radius = minSpacing * ::std::sqrt(i + 1);

        float x = target->GetPositionX() + cos(angle) * radius;
        float y = target->GetPositionY() + sin(angle) * radius;        float z = target->GetPositionZ();        _map->GetHeight(bots[i]->GetPhaseShift(), x, y, z);

        // Face the target
        float facing = atan2(target->GetPositionY() - y, target->GetPositionX() - x);
        positions.emplace_back(x, y, z, facing);
    }

    return positions;
}

bool PositionStrategyBase::IsPositionOccupied(const Position& pos, float radius) const
{
    auto grid = WorldToGrid(pos);
    int32 gridRadius = static_cast<int32>(radius / GRID_CELL_SIZE) + 1;

    for (int32 dx = -gridRadius; dx <= gridRadius; ++dx)
    {
        for (int32 dy = -gridRadius; dy <= gridRadius; ++dy)
        {

            int32 x = grid.first + dx;

            int32 y = grid.second + dy;


            if (x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE)

            {

                if ((*_spatialGrid)[x][y].IsOccupied())

                    return true;

            }
        }
    }

    return false;
}

bool PositionStrategyBase::CheckCollisionWithObjects(const Position& pos, float radius) const
{
    // This would check for collision with game objects, but requires access to object lists
    // For now, we rely on line-of-sight checks which include most obstacles
    return false;
}

// Specialized strategy implementations

Position MeleePositionStrategy::CalculateOptimalPosition(Player* bot, Unit* target, float preferredRange)
{
    if (!bot || !target)
        return bot ? bot->GetPosition() : Position();

    // Try to get behind target for backstab classes
    if (bot->GetClass() == CLASS_ROGUE)    {
        Position backstab = GetBackstabPosition(target);
        if (ValidatePosition(backstab, bot) && CanReachPosition(bot, backstab, 2.0f))

            return backstab;
    }

    // Try flanking positions
    Position leftFlank = GetFlankPosition(target, true);
    Position rightFlank = GetFlankPosition(target, false);

    float leftScore = EvaluatePositionScore(leftFlank, bot, target);
    float rightScore = EvaluatePositionScore(rightFlank, bot, target);

    if (leftScore > rightScore && ValidatePosition(leftFlank, bot))
        return leftFlank;
    else if (ValidatePosition(rightFlank, bot))
        return rightFlank;

    // Fall back to base implementation
    return PositionStrategyBase::CalculateOptimalPosition(bot, target, GetOptimalMeleeRange());
}

Position MeleePositionStrategy::GetBackstabPosition(Unit* target) const
{
    float angle = target->GetOrientation() + static_cast<float>(M_PI);  // Behind target    float x = target->GetPositionX() + cos(angle) * GetOptimalMeleeRange();    float y = target->GetPositionY() + sin(angle) * GetOptimalMeleeRange();    float z = target->GetPositionZ();    _map->GetHeight(nullptr, x, y, z);
    return Position(x, y, z, angle - static_cast<float>(M_PI));  // Face the target
}

Position MeleePositionStrategy::GetFlankPosition(Unit* target, bool leftSide) const
{
    float angle = target->GetOrientation() + (leftSide ? static_cast<float>(M_PI) / 2.0f : -static_cast<float>(M_PI) / 2.0f);
    float x = target->GetPositionX() + cos(angle) * GetOptimalMeleeRange();    float y = target->GetPositionY() + sin(angle) * GetOptimalMeleeRange();    float z = target->GetPositionZ();    _map->GetHeight(nullptr, x, y, z);
    return Position(x, y, z, angle + (leftSide ? -static_cast<float>(M_PI) / 2.0f : static_cast<float>(M_PI) / 2.0f));
}

bool MeleePositionStrategy::CanReachPosition(Player* bot, const Position& pos, float timeLimit) const
{
    float distance = bot->GetExactDist(&pos);
    float moveSpeed = bot->GetSpeed(MOVE_RUN);
    float timeToReach = distance / moveSpeed;

    return timeToReach <= timeLimit;
}

float MeleePositionStrategy::EvaluatePositionScore(const Position& pos, Player* bot, Unit* target) const
{
    float baseScore = PositionStrategyBase::EvaluatePositionScore(pos, bot, target);

    // Bonus for being behind target
    if (!target->HasInArc(static_cast<float>(M_PI), &pos))
        baseScore += 30.0f;

    // Penalty for being in front (for non-tanks)
    if (bot->GetClass() != CLASS_WARRIOR && target->HasInArc(static_cast<float>(M_PI) / 4.0f, &pos))        baseScore -= 20.0f;

    return baseScore;
}

} // namespace Playerbot