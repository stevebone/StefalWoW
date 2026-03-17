/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PathfindingAdapter.h"
#include "Player.h"
#include "Map.h"
#include "../BotMovementUtil.h"
#include "../RoadNetwork/RoadNetworkManager.h"

#include "PhaseShift.h"
#include "Log.h"
#include <algorithm>

namespace Playerbot
{
    PathfindingAdapter::PathfindingAdapter()
        : _enableCaching(true), _enableSmoothing(true),
          _maxCacheSize(100), _cacheDuration(5000),
          _maxPathNodes(74), _straightPathDistance(10.0f),
          _maxSearchDistance(100.0f), _cacheCleanInterval(10000),
          _cacheHits(0), _cacheMisses(0), _cacheEvictions(0),
          _totalPathsGenerated(0), _totalGenerationTime(0),
          _maxGenerationTime(0)
    {
        _lastCacheClean = ::std::chrono::steady_clock::now();
    }

    PathfindingAdapter::~PathfindingAdapter()
    {
        Shutdown();
    }

    bool PathfindingAdapter::Initialize(uint32 cacheSize, uint32 cacheDuration)
    {
        _maxCacheSize = cacheSize;
        _cacheDuration = cacheDuration;

        TC_LOG_INFO("playerbot.movement", "PathfindingAdapter initialized with cache size %u, duration %u ms",
            _maxCacheSize, _cacheDuration);

        return true;
    }

    void PathfindingAdapter::Shutdown()
    {
        ::std::lock_guard lock(_cacheLock);
        _pathCache.clear();
        while (!_cacheOrder.empty())
            _cacheOrder.pop();
    }

    bool PathfindingAdapter::CalculatePath(Player* bot, Position const& destination,
                                          MovementPath& path, bool forceDirect)
    {
        if (!bot || !bot->GetMap())
            return false;

        auto startTime = ::std::chrono::high_resolution_clock::now();

        // Check cache first if enabled
    if (_enableCaching && !forceDirect)
        {
            if (GetCachedPath(bot, destination, path))
            {
                _cacheHits.fetch_add(1);
                return true;
            }
            _cacheMisses.fetch_add(1);
        }

        // Clear existing path
        path.Clear();

        Position start = bot->GetPosition();
        float distance = start.GetExactDist(&destination);

        // Road-aware routing for long-distance travel
        if (!forceDirect && !bot->CanFly() &&
            sRoadNetworkMgr->HasRoadNetwork(bot->GetMapId()))
        {
            std::vector<Position> roadWaypoints;
            if (sRoadNetworkMgr->CalculateRoadAwarePath(
                    bot->GetMapId(), start, destination, roadWaypoints))
            {
                // Generate navmesh sub-paths between consecutive road waypoints
                Position prev = start;
                float totalLen = 0.0f;
                for (Position const& wp : roadWaypoints)
                {
                    PathGenerator gen(bot);
                    gen.CalculatePath(wp.GetPositionX(), wp.GetPositionY(), wp.GetPositionZ());
                    for (auto const& pt : gen.GetPath())
                    {
                        path.nodes.push_back(PathNode(
                            Position(pt.x, pt.y, pt.z), bot->GetSpeed(MOVE_RUN)));
                    }
                    totalLen += prev.GetExactDist(wp);
                    prev = wp;
                }
                // Final segment to destination
                PathGenerator gen(bot);
                gen.CalculatePath(destination.GetPositionX(),
                                  destination.GetPositionY(),
                                  destination.GetPositionZ());
                for (auto const& pt : gen.GetPath())
                {
                    path.nodes.push_back(PathNode(
                        Position(pt.x, pt.y, pt.z), bot->GetSpeed(MOVE_RUN)));
                }
                totalLen += prev.GetExactDist(destination);

                path.pathType = PathType::PATHFIND_NORMAL;
                path.totalLength = totalLen;
                path.isOptimized = false;

                // Cache and return the road-aware path
                auto endTime = ::std::chrono::high_resolution_clock::now();
                uint32 generationTime = static_cast<uint32>(
                    ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime).count());
                path.generationCost = generationTime;
                path.generatedTime = ::std::chrono::steady_clock::now();
                _totalPathsGenerated.fetch_add(1);
                _totalGenerationTime.fetch_add(generationTime);

                if (_enableCaching && path.IsValid())
                    CachePath(bot, destination, path);

                TC_LOG_DEBUG("playerbot.movement",
                    "Road-aware path for bot %s: %zu nodes, %.0f yards",
                    bot->GetName().c_str(), path.nodes.size(), totalLen);
                return true;
            }
        }

        // Use direct path for short distances or if forced
    if (forceDirect || distance <= _straightPathDistance)
        {
            PathNode node(destination, bot->GetSpeed(MOVE_RUN));
            path.nodes.push_back(node);
            path.pathType = PathType::PATHFIND_NORMAL;
            path.totalLength = distance;
            path.isOptimized = false;
        }
        else
        {
            // Use TrinityCore's PathGenerator
            PathGenerator generator(bot);

            // Set generation options
    if (bot->CanFly())
                generator.SetUseStraightPath(true);

            bool result = InternalCalculatePath(generator, start, destination, bot);
            if (!result)
            {
                TC_LOG_DEBUG("playerbot.movement", "Path generation failed for bot %s to (%.2f, %.2f, %.2f)",
                    bot->GetName().c_str(), destination.GetPositionX(),
                    destination.GetPositionY(), destination.GetPositionZ());
                return false;
            }

            // Convert PathGenerator path to MovementPath
            ConvertPath(generator, path);

            // Optimize path if enabled
    if (_enableSmoothing && path.nodes.size() > 2)
            {
                OptimizePath(path);
                path.isOptimized = true;
            }
        }

        // Calculate generation time
        auto endTime = ::std::chrono::high_resolution_clock::now();
        uint32 generationTime = static_cast<uint32>(
            ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime).count());

        path.generationCost = generationTime;
        path.generatedTime = ::std::chrono::steady_clock::now();

        // Update statistics
        _totalPathsGenerated.fetch_add(1);
        _totalGenerationTime.fetch_add(generationTime);

        uint32 currentMax = _maxGenerationTime.load();
        while (generationTime > currentMax &&
               !_maxGenerationTime.compare_exchange_weak(currentMax, generationTime));

        // Cache the path if caching is enabled
    if (_enableCaching && path.IsValid())
        {
            CachePath(bot, destination, path);
        }

        TC_LOG_DEBUG("playerbot.movement", "Generated path with %zu nodes, length %.2f, time %u us",
            path.nodes.size(), path.totalLength, generationTime);

        return true;
    }

    bool PathfindingAdapter::CalculatePathToUnit(Player* bot, Unit* target,
                                                MovementPath& path, float range)
    {
        if (!bot || !target)
            return false;

        // Calculate position behind/beside target based on range
        Position targetPos = target->GetPosition();

        if (range > 0.0f)
        {
            // Adjust destination to maintain range from target
            float angle = target->GetAbsoluteAngle(bot);
            targetPos.m_positionX += range * cos(angle);
            targetPos.m_positionY += range * sin(angle);
            // Z will be corrected below
        }

        // CRITICAL FIX: Correct Z to actual ground level at the calculated position
        // When adjusting X/Y by range, the target's Z may not match the terrain
        // at the new position (e.g., target on hill, calculated position over cliff)
        BotMovementUtil::CorrectPositionToGround(bot, targetPos);

        return CalculatePath(bot, targetPos, path);
    }

    bool PathfindingAdapter::CalculateFormationPath(Player* bot, Unit* leader,
                                               Position const& offset, MovementPath& path)
    {
        if (!bot || !leader)
            return false;

        // Calculate formation position relative to leader
        Position leaderPos = leader->GetPosition();
        float leaderOrientation = leader->GetOrientation();

        Position formationPos;
        formationPos.m_positionX = leaderPos.GetPositionX() +
            offset.GetPositionX() * cos(leaderOrientation) -
            offset.GetPositionY() * sin(leaderOrientation);
        formationPos.m_positionY = leaderPos.GetPositionY() +
            offset.GetPositionX() * sin(leaderOrientation) +
            offset.GetPositionY() * cos(leaderOrientation);
        formationPos.m_positionZ = leaderPos.GetPositionZ();  // Initial estimate
        formationPos.SetOrientation(leaderOrientation);

        // CRITICAL FIX: Correct Z to actual ground level at the formation position
        // Formation positions are calculated with offset from leader, but terrain
        // at the formation X/Y may be at a completely different Z level
        // (e.g., leader on flat ground, formation position over a cliff edge)
        BotMovementUtil::CorrectPositionToGround(bot, formationPos);

        return CalculatePath(bot, formationPos, path);
    }

    bool PathfindingAdapter::CalculateFleePath(Player* bot, Unit* threat,
                                              float distance, MovementPath& path)
    {
        // CRITICAL FIX: Use FindMap() instead of GetMap() to avoid ASSERT crash
        // GetMap() has ASSERT(m_currMap) which crashes if map is null
        Map* map = bot ? bot->FindMap() : nullptr;
        if (!bot || !threat || !map)
            return false;

        // Calculate flee direction (opposite of threat)
        float angle = threat->GetAbsoluteAngle(bot) + static_cast<float>(M_PI);
        angle = Position::NormalizeOrientation(angle);

        // Try to find a valid flee position
        Position fleePos;
        bool found = false;

        // Try multiple angles if direct opposite is blocked
        for (int i = 0; i < 8; ++i)
        {
            float tryAngle = angle + (i % 2 == 0 ? i/2 * static_cast<float>(M_PI)/4 : -i/2 * static_cast<float>(M_PI)/4);
            tryAngle = Position::NormalizeOrientation(tryAngle);

            fleePos = bot->GetNearPosition(distance, tryAngle);

            // CRITICAL FIX: Correct Z to actual ground level at the flee position
            // GetNearPosition may return a position with incorrect Z for terrain
            BotMovementUtil::CorrectPositionToGround(bot, fleePos);

            // Check if position is valid
            if (IsWalkablePosition(map, fleePos))
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            TC_LOG_DEBUG("playerbot.movement", "No valid flee position found for bot %s",
                bot->GetName().c_str());
            return false;
        }

        return CalculatePath(bot, fleePos, path);
    }

    bool PathfindingAdapter::HasCachedPath(Player* bot, Position const& destination) const
    {
        if (!bot || !_enableCaching)
            return false;

        uint64 key = CalculateCacheKey(bot->GetGUID(), destination);
        ::std::lock_guard lock(_cacheLock);
        auto it = _pathCache.find(key);

        if (it != _pathCache.end() && it->second.isValid)
        {
            // Check if cache is expired
    if (!it->second.IsExpired(_cacheDuration))
            {
                // Check if destination is close enough to cached destination
                return ArePositionsClose(it->second.destination, destination);
            }
        }

        return false;
    }

    bool PathfindingAdapter::GetCachedPath(Player* bot, Position const& destination,
                                          MovementPath& path) const
    {
        if (!HasCachedPath(bot, destination))
            return false;

        uint64 key = CalculateCacheKey(bot->GetGUID(), destination);
        ::std::lock_guard lock(_cacheLock);
        auto it = _pathCache.find(key);

        if (it != _pathCache.end())
        {
            path = it->second.path;
            const_cast<CachedPath&>(it->second).hitCount++;
            return true;
        }

        return false;
    }

    void PathfindingAdapter::ClearCache(Player* bot)
    {
        if (!bot)
            return;

        ::std::lock_guard lock(_cacheLock);

        // Remove all cache entries for this bot
        auto it = _pathCache.begin();
        while (it != _pathCache.end())
        {
            // Extract bot GUID from key (first 64 bits)
            ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(it->first >> 32);
            if (guid == bot->GetGUID())
            {
                it = _pathCache.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void PathfindingAdapter::ClearAllCache()
    {
        ::std::lock_guard lock(_cacheLock);
        _pathCache.clear();
        while (!_cacheOrder.empty())
            _cacheOrder.pop();
    }

    void PathfindingAdapter::SetPathParameters(uint32 maxNodes, float straightDistance,
                                              float maxSearchDistance)
    {
        _maxPathNodes = ::std::min(maxNodes, static_cast<uint32>(MAX_PATH_LENGTH));
        _straightPathDistance = straightDistance;
        _maxSearchDistance = maxSearchDistance;
    }

    void PathfindingAdapter::SetCacheParameters(uint32 maxSize, uint32 duration)
    {
        _maxCacheSize = maxSize;
        _cacheDuration = duration;

        // Clean cache if it's now too large
    if (_pathCache.size() > _maxCacheSize)
        {
            CleanExpiredCache();
        }
    }

    void PathfindingAdapter::GetCacheStatistics(uint32& hits, uint32& misses,
                                               uint32& evictions) const
    {
        hits = _cacheHits.load();
        misses = _cacheMisses.load();
        evictions = _cacheEvictions.load();
    }

    void PathfindingAdapter::GetPathStatistics(uint32& totalPaths, uint32& avgTime,
                                              uint32& maxTime) const
    {
        totalPaths = _totalPathsGenerated.load();
        uint64 totalTime = _totalGenerationTime.load();
        avgTime = totalPaths > 0 ? static_cast<uint32>(totalTime / totalPaths) : 0;
        maxTime = _maxGenerationTime.load();
    }

    void PathfindingAdapter::ResetStatistics()
    {
        _cacheHits.store(0);
        _cacheMisses.store(0);
        _cacheEvictions.store(0);
        _totalPathsGenerated.store(0);
        _totalGenerationTime.store(0);
        _maxGenerationTime.store(0);
    }

    bool PathfindingAdapter::IsWalkablePosition(Map* map, Position const& position) const
    {
        if (!map)
            return false;

        // Check if position has valid ground height
        // Note: Map doesn't have GetPhaseShift(), use default PhaseShift
        PhaseShift phaseShift;
        float z = map->GetHeight(phaseShift, position.GetPositionX(),
            position.GetPositionY(), position.GetPositionZ(), true, 100.0f);

        if (z > INVALID_HEIGHT)
        {
            // Check if position is not too high above ground
            float heightDiff = ::std::abs(position.GetPositionZ() - z);
            return heightDiff < 10.0f;
        }

        return false;
    }

    bool PathfindingAdapter::GetNearestWalkablePosition(Map* map, Position const& position,
                                                       Position& walkable, float searchRange) const
    {
        if (!map)
            return false;

        // First check if current position is walkable
    if (IsWalkablePosition(map, position))
        {
            walkable = position;
            return true;
        }

        // Search in expanding circles
        const int steps = 8;
        for (float radius = 2.0f; radius <= searchRange; radius += 2.0f)
        {
            for (int i = 0; i < steps; ++i)
            {
                float angle = (2.0f * static_cast<float>(M_PI) * i) / steps;
                Position testPos;
                testPos.m_positionX = position.GetPositionX() + radius * cos(angle);
                testPos.m_positionY = position.GetPositionY() + radius * sin(angle);
                testPos.m_positionZ = position.GetPositionZ();

                if (IsWalkablePosition(map, testPos))
                {
                    walkable = testPos;
                    // Adjust Z to ground height
                    PhaseShift phaseShift;
                    walkable.m_positionZ = map->GetHeight(phaseShift,
                        walkable.GetPositionX(), walkable.GetPositionY(),
                        walkable.GetPositionZ(), true, 100.0f);
                    return true;
                }
            }
        }

        return false;
    }

    bool PathfindingAdapter::InternalCalculatePath(PathGenerator& generator,
                                                  Position const& start,
                                                  Position const& end,
                                                  Unit const* bot)
    {
        // Calculate the path using TrinityCore's pathfinding
        bool result = generator.CalculatePath(end.GetPositionX(), end.GetPositionY(),
            end.GetPositionZ(), false);

        if (!result)
        {
            // Try with partial path if allowed
            result = generator.CalculatePath(end.GetPositionX(), end.GetPositionY(),
                end.GetPositionZ(), true);
        }

        return result && generator.GetPathType() != PATHFIND_NOPATH;
    }

    void PathfindingAdapter::ConvertPath(PathGenerator const& generator, MovementPath& path)
    {
        auto const& points = generator.GetPath();

        path.nodes.clear();
        path.nodes.reserve(points.size());

        for (auto const& point : points)
        {
            PathNode node;
            node.position.Relocate(point.x, point.y, point.z);
            node.speed = 0.0f; // Will be set by movement generator
            node.delay = 0;
            node.terrain = TerrainType::TERRAIN_GROUND;
            node.isSmoothed = false;
            path.nodes.push_back(node);
        }

        // Set path type based on PathGenerator result
    switch (generator.GetPathType())
        {
        case PATHFIND_NORMAL:
            path.pathType = PathType::PATHFIND_NORMAL;
            break;
        case PATHFIND_SHORTCUT:
            path.pathType = PathType::PATHFIND_SHORTCUT;
            break;
        case PATHFIND_INCOMPLETE:
            path.pathType = PathType::PATHFIND_INCOMPLETE;
            break;
        case PATHFIND_NOPATH:
            path.pathType = PathType::PATHFIND_NOPATH;
            break;
        default:
            path.pathType = PathType::PATHFIND_NORMAL;
            break;
        }

        // Calculate total path length
        path.totalLength = 0.0f;
        for (size_t i = 1; i < path.nodes.size(); ++i)
        {
            path.totalLength += path.nodes[i - 1].position.GetExactDist(
                &path.nodes[i].position);
        }
    }

    void PathfindingAdapter::OptimizePath(MovementPath& path)
    {
        if (path.nodes.size() < 3)
            return;

        ::std::vector<PathNode> optimized;
        optimized.reserve(path.nodes.size());

        // Always keep start point
        optimized.push_back(path.nodes[0]);

        // Use Douglas-Peucker algorithm for path simplification
        size_t i = 0;
        while (i < path.nodes.size() - 1)
        {
            size_t furthest = i + 1;
            bool canSkip = true;

            // Look ahead to find furthest reachable point
    for (size_t j = i + 2; j < path.nodes.size() && j <= i + 5; ++j)
            {
                // Check if we can skip intermediate points
                Position const& start = path.nodes[i].position;
                Position const& end = path.nodes[j].position;

                // Simple line of sight check (would use actual LOS in production)
                float dist = start.GetExactDist(&end);
                if (dist < 20.0f) // Max skip distance
                {
                    furthest = j;
                }
                else
                {
                    break;
                }
            }

            // Add the furthest reachable point
    if (furthest < path.nodes.size())
            {
                path.nodes[furthest].isSmoothed = true;
                optimized.push_back(path.nodes[furthest]);
            }

            i = furthest;
        }

        // Always keep end point
    if (optimized.back().position.GetExactDist(&path.nodes.back().position) > 0.1f)
        {
            optimized.push_back(path.nodes.back());
        }

        // Replace original path with optimized version
        path.nodes = ::std::move(optimized);

        // Recalculate total length
        path.totalLength = 0.0f;
        for (size_t i = 1; i < path.nodes.size(); ++i)
        {
            path.totalLength += path.nodes[i - 1].position.GetExactDist(
                &path.nodes[i].position);
        }

        TC_LOG_DEBUG("playerbot.movement", "Optimized path from %zu to %zu nodes",
            path.GetNodeCount(), optimized.size());
    }

    void PathfindingAdapter::CachePath(Player* bot, Position const& destination,
                                      MovementPath const& path)
    {
        if (!bot)
            return;

        uint64 key = CalculateCacheKey(bot->GetGUID(), destination);

        ::std::lock_guard lock(_cacheLock);

        // Check cache size limit
    if (_pathCache.size() >= _maxCacheSize)
        {
            CleanExpiredCache();

            // If still too large, remove oldest entry
    if (_pathCache.size() >= _maxCacheSize && !_cacheOrder.empty())
            {
                uint64 oldestKey = _cacheOrder.front();
                _cacheOrder.pop();
                _pathCache.erase(oldestKey);
                _cacheEvictions.fetch_add(1);
            }
        }

        // Add new cache entry
        CachedPath& cached = _pathCache[key];
        cached.path = path;
        cached.destination = destination;
        cached.timestamp = ::std::chrono::steady_clock::now();
        cached.hitCount = 0;
        cached.isValid = true;

        _cacheOrder.push(key);
    }

    void PathfindingAdapter::CleanExpiredCache()
    {
        auto now = ::std::chrono::steady_clock::now();

        // Only clean periodically
        auto elapsed = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
            now - _lastCacheClean).count();
        if (elapsed < _cacheCleanInterval)
            return;

        _lastCacheClean = now;

        auto it = _pathCache.begin();
        while (it != _pathCache.end())
        {
            if (it->second.IsExpired(_cacheDuration))
            {
                it = _pathCache.erase(it);
                _cacheEvictions.fetch_add(1);
            }
            else
            {
                ++it;
            }
        }
    }

    uint64 PathfindingAdapter::CalculateCacheKey(ObjectGuid botGuid, Position const& destination) const
    {
        // Combine bot GUID and quantized destination into a single key
        uint64 key = botGuid.GetCounter();
        key <<= 32;

        // Quantize position to reduce cache misses for nearby positions
        int32 x = static_cast<int32>(destination.GetPositionX() / 2.0f);
        int32 y = static_cast<int32>(destination.GetPositionY() / 2.0f);
        key |= ((x & 0xFFFF) << 16) | (y & 0xFFFF);

        return key;
    }

    bool PathfindingAdapter::ArePositionsClose(Position const& pos1, Position const& pos2,
                                              float threshold) const
    {
        return pos1.GetExactDist(&pos2) <= threshold;
    }
}