/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PathCache.h"
#include "Log.h"
#include "Movement/BotMovement/Pathfinding/ValidatedPathGenerator.h"
#include "Movement/BotMovement/Core/BotMovementConfig.h"
#include "Movement/BotMovement/Core/BotMovementManager.h"
#include <cmath>

namespace Playerbot
{

PathCache::PathCache(Map* map)
    : _map(map)
{
    ASSERT(map, "PathCache requires valid Map pointer");

    TC_LOG_INFO("playerbot.spatial",
        "PathCache created for map {} ({}), max cached paths: {}, TTL: {}s, quantization: {:.1f} yards",
        map->GetId(), map->GetMapName(), MAX_CACHED_PATHS, CACHE_TTL_SECONDS, POSITION_QUANTIZATION);
}

Position PathCache::QuantizePosition(Position const& pos) const
{
    // Round each coordinate to nearest POSITION_QUANTIZATION yards (5.0 yards)
    // Algorithm: floor((value / quantization) + 0.5) * quantization

    float quantX = ::std::floor((pos.GetPositionX() / POSITION_QUANTIZATION) + 0.5f) * POSITION_QUANTIZATION;
    float quantY = ::std::floor((pos.GetPositionY() / POSITION_QUANTIZATION) + 0.5f) * POSITION_QUANTIZATION;
    float quantZ = ::std::floor((pos.GetPositionZ() / POSITION_QUANTIZATION) + 0.5f) * POSITION_QUANTIZATION;

    Position quantized;
    quantized.Relocate(quantX, quantY, quantZ, pos.GetOrientation());

    return quantized;
}

uint64_t PathCache::GetPathHash(Position const& src, Position const& dest) const
{
    // Pack quantized coordinates into 64-bit hash
    // We use 16 bits per coordinate (range: 0-65535)
    // Offset by 17066.67 to make all values positive, then scale to fit 16 bits

    auto PackCoord = [](float coord) -> uint16_t
    {
        // Offset: -17066.67 to +17066.67 → 0 to 34133.33
        float offset = coord + 17066.67f;
        // Scale: 0 to 34133.33 → 0 to 65535
        float scaled = (offset / 34133.33f) * 65535.0f;
        return static_cast<uint16_t>(::std::max(0.0f, ::std::min(65535.0f, scaled)));
    };

    uint16_t srcX = PackCoord(src.GetPositionX());
    uint16_t srcY = PackCoord(src.GetPositionY());
    uint16_t destX = PackCoord(dest.GetPositionX());
    uint16_t destY = PackCoord(dest.GetPositionY());

    // Pack into 64-bit key
    return (static_cast<uint64_t>(srcX) << 48) |
           (static_cast<uint64_t>(srcY) << 32) |
           (static_cast<uint64_t>(destX) << 16) |
            static_cast<uint64_t>(destY);
}

PathCache::PathResult PathCache::CalculateNewPath(Position const& src, Position const& dest, WorldObject const* owner)
{
    PathResult result;

    // NEW: Use ValidatedPathGenerator if BotMovement system is enabled
    if (sBotMovementManager->GetConfig().IsEnabled())
    {
        ValidatedPathGenerator validatedPath(owner);
        ValidatedPath vpResult = validatedPath.CalculateValidatedPath(src, dest, false);

        if (vpResult.IsValid())
        {
            // Success - validated path found
            result.points = vpResult.points;
            result.pathType = vpResult.pathType;
            result.length = validatedPath.GetPathLength();
            result.timestamp = ::std::chrono::steady_clock::now();

            TC_LOG_DEBUG("module.playerbot.movement",
                "ValidatedPath SUCCESS: {} waypoints, type={}, validated={}",
                result.points.size(),
                static_cast<uint32>(result.pathType),
                vpResult.validationResult.isValid);

            return result;
        }
        else
        {
            // Validation failed - log reason and fallback to legacy
            TC_LOG_WARN("module.playerbot.movement",
                "ValidatedPath FAILED: reason='{}', falling back to legacy pathfinding",
                vpResult.validationResult.failureReason);

            // Fallback to legacy pathfinding
            return CalculateNewPathLegacy(src, dest, owner);
        }
    }
    else
    {
        // BotMovement system disabled - use legacy pathfinding
        return CalculateNewPathLegacy(src, dest, owner);
    }
}

PathCache::PathResult PathCache::CalculateNewPathLegacy(Position const& src, Position const& dest, WorldObject const* owner)
{
    PathResult result;

    // Create PathGenerator instance
    PathGenerator path(owner);

    // Calculate path
    bool success = path.CalculatePath(
        dest.GetPositionX(),
        dest.GetPositionY(),
        dest.GetPositionZ(),
        false  // forceDest
    );

    if (!success)
    {
        result.pathType = PATHFIND_NOPATH;
        result.length = 0.0f;
        return result;
    }

    // Extract results
    result.points = path.GetPath();
    result.pathType = path.GetPathType();
    result.length = path.GetPathLength();
    result.timestamp = ::std::chrono::steady_clock::now();

    TC_LOG_TRACE("module.playerbot.movement",
        "Legacy PathGenerator: {} waypoints, type={}",
        result.points.size(),
        static_cast<uint32>(result.pathType));

    return result;
}

PathCache::PathResult PathCache::GetPath(Position const& src, Position const& dest, WorldObject const* owner)
{
    // Quantize positions to reduce key space
    Position quantSrc = QuantizePosition(src);
    Position quantDest = QuantizePosition(dest);

    // Generate cache key
    uint64_t pathHash = GetPathHash(quantSrc, quantDest);

    // Try shared lock first (concurrent reads)
    {
        ::std::shared_lock lock(_mutex);
        auto it = _cache.find(pathHash);

        if (it != _cache.end() && !it->second.IsExpired())
        {
            // Cache hit!
            ++_stats.hits;

            TC_LOG_TRACE("playerbot.spatial",
                "PathCache HIT for map {}: ({:.1f}, {:.1f}) -> ({:.1f}, {:.1f}), {} waypoints",
                _map->GetId(),
                src.GetPositionX(), src.GetPositionY(),
                dest.GetPositionX(), dest.GetPositionY(),
                it->second.points.size());

            return it->second;
        }
    }

    // Cache miss - need to calculate new path
    ::std::unique_lock lock(_mutex);
    ++_stats.misses;

    // Double-check cache after acquiring unique lock
    auto it = _cache.find(pathHash);
    if (it != _cache.end() && !it->second.IsExpired())
    {
        // Another thread populated cache while we were waiting
        --_stats.misses;
        ++_stats.hits;
        return it->second;
    }

    // Calculate new path using TrinityCore PathGenerator
    PathResult result = CalculateNewPath(src, dest, owner);

    // LRU eviction if cache is full
    if (_cache.size() >= MAX_CACHED_PATHS)
        EvictOldest();

    // Store in cache
    _cache[pathHash] = result;
    _lruQueue.push_back(pathHash);

    TC_LOG_TRACE("playerbot.spatial",
        "PathCache MISS for map {}: ({:.1f}, {:.1f}) -> ({:.1f}, {:.1f}), calculated {} waypoints, type: {}",
        _map->GetId(),
        src.GetPositionX(), src.GetPositionY(),
        dest.GetPositionX(), dest.GetPositionY(),
        result.points.size(), result.pathType);

    return result;
}

void PathCache::EvictOldest()
{
    if (_lruQueue.empty())
        return;

    // Remove oldest entry (front of queue)
    uint64_t oldestHash = _lruQueue.front();
    _lruQueue.pop_front();

    auto it = _cache.find(oldestHash);
    if (it != _cache.end())
    {
        _cache.erase(it);
        ++_stats.evictions;

        TC_LOG_TRACE("playerbot.spatial",
            "PathCache evicted oldest path for map {}, cache size: {}",
            _map->GetId(), _cache.size());
    }
}

void PathCache::InvalidateRegion(Position const& center, float radius)
{
    ::std::unique_lock lock(_mutex);

    // For simplicity, clear entire cache when region is invalidated
    // This is acceptable because region invalidation is rare
    uint32 clearedCount = _cache.size();

    _cache.clear();
    _lruQueue.clear();
    _stats.evictions += clearedCount;

    TC_LOG_INFO("playerbot.spatial",
        "PathCache invalidated region (center: {:.1f}, {:.1f}, radius: {:.1f}) for map {}, {} paths cleared",
        center.GetPositionX(), center.GetPositionY(), radius,
        _map->GetId(), clearedCount);
}

void PathCache::Clear()
{
    ::std::unique_lock lock(_mutex);

    uint32 clearedCount = _cache.size();
    _cache.clear();
    _lruQueue.clear();
    _stats.evictions += clearedCount;

    TC_LOG_INFO("playerbot.spatial",
        "PathCache cleared for map {} ({}), {} paths removed",
        _map->GetId(), _map->GetMapName(), clearedCount);
}

} // namespace Playerbot
