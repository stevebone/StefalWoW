/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "SpatialGridScheduler.h"
#include "SpatialGridManager.h"
#include "DoubleBufferedSpatialGrid.h"
#include "MapManager.h"
#include "Map.h"
#include "Log.h"
#include <algorithm>

namespace Playerbot
{

SpatialGridScheduler::SpatialGridScheduler()
    : _lastUpdateTime(::std::chrono::steady_clock::now())
{
    TC_LOG_INFO("playerbot.spatial",
        "SpatialGridScheduler initialized with {}ms update interval",
        _updateInterval.load());
}

void SpatialGridScheduler::UpdateAllGrids(uint32 diff)
{
    if (!_enabled.load(::std::memory_order_relaxed))
        return;

    // Accumulate time
    _timeSinceLastUpdate += diff;

    // Check if enough time has passed
    uint32 interval = _updateInterval.load(::std::memory_order_relaxed);
    if (_timeSinceLastUpdate < interval)
    {
        _skippedUpdates.fetch_add(1, ::std::memory_order_relaxed);
        return;
    }

    auto startTime = ::std::chrono::steady_clock::now();

    // Update all spatial grids
    // This is the ONLY place where spatial grids are updated
    // No more contention from 25+ threads trying to update simultaneously

    uint32 gridsUpdated = 0;

    // Iterate all maps using MapManager's thread-safe iterator
    sMapMgr->DoForAllMaps([&gridsUpdated](Map* map)
    {
        if (!map)
            return;

        uint32 mapId = map->GetId();

        // Get spatial grid for this map
        DoubleBufferedSpatialGrid* grid = sSpatialGridManager.GetGrid(mapId);
        if (!grid)
        {
            // Create grid if it doesn't exist
            sSpatialGridManager.CreateGrid(map);
            grid = sSpatialGridManager.GetGrid(mapId);
        }

        if (grid)
        {
            // CRITICAL: Single, controlled update point
            // No more race conditions, no more deadlocks
            grid->Update();
            ++gridsUpdated;
        }
    });

    // Reset timer
    _timeSinceLastUpdate = 0;
    _lastUpdateTime = startTime;

    // Update statistics
    auto endTime = ::std::chrono::steady_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(endTime - startTime);

    _lastUpdateDurationMs.store(static_cast<uint32>(duration.count()), ::std::memory_order_relaxed);
    _totalUpdateTimeMs.fetch_add(duration.count(), ::std::memory_order_relaxed);
    _totalUpdates.fetch_add(1, ::std::memory_order_relaxed);

    if (duration.count() > 10)  // Warn if update takes >10ms
    {
        TC_LOG_WARN("playerbot.spatial",
            "SpatialGridScheduler::UpdateAllGrids took {}ms to update {} grids",
            duration.count(), gridsUpdated);
    }
    else
    {
        TC_LOG_TRACE("playerbot.spatial",
            "SpatialGridScheduler updated {} grids in {}ms",
            gridsUpdated, duration.count());
    }
}

void SpatialGridScheduler::UpdateMapGrid(uint32 mapId, bool forceUpdate)
{
    if (!_enabled.load(::std::memory_order_relaxed) && !forceUpdate)
        return;

    // Rate limiting unless forced
    if (!forceUpdate)
    {
        auto now = ::std::chrono::steady_clock::now();
        auto elapsed = ::std::chrono::duration_cast<::std::chrono::milliseconds>(now - _lastUpdateTime);

        uint32 interval = _updateInterval.load(::std::memory_order_relaxed);
        if (elapsed.count() < static_cast<int64>(interval))
        {
            _skippedUpdates.fetch_add(1, ::std::memory_order_relaxed);
            return;
        }
    }

    // Get and update the specific grid
    DoubleBufferedSpatialGrid* grid = sSpatialGridManager.GetGrid(mapId);
    if (grid)
    {
        auto startTime = ::std::chrono::steady_clock::now();

        grid->Update();

        auto endTime = ::std::chrono::steady_clock::now();
        auto duration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(endTime - startTime);

        _lastUpdateDurationMs.store(static_cast<uint32>(duration.count()), ::std::memory_order_relaxed);
        _totalUpdateTimeMs.fetch_add(duration.count(), ::std::memory_order_relaxed);
        _totalUpdates.fetch_add(1, ::std::memory_order_relaxed);
        _lastUpdateTime = startTime;

        TC_LOG_TRACE("playerbot.spatial",
            "Updated spatial grid for map {} in {}ms",
            mapId, duration.count());
    }
}

void SpatialGridScheduler::SetUpdateInterval(uint32 intervalMs)
{
    // Enforce minimum interval to prevent excessive updates
    uint32 clampedInterval = ::std::max(50u, intervalMs);
    _updateInterval.store(clampedInterval, ::std::memory_order_relaxed);

    TC_LOG_INFO("playerbot.spatial",
        "SpatialGridScheduler update interval set to {}ms",
        clampedInterval);
}

SpatialGridScheduler::Statistics SpatialGridScheduler::GetStatistics() const
{
    Statistics stats;

    stats.totalUpdates = _totalUpdates.load(::std::memory_order_relaxed);
    stats.skippedUpdates = _skippedUpdates.load(::std::memory_order_relaxed);
    stats.lastUpdateDurationMs = _lastUpdateDurationMs.load(::std::memory_order_relaxed);
    stats.lastUpdateTime = _lastUpdateTime;

    // Calculate average
    uint64 totalTime = _totalUpdateTimeMs.load(::std::memory_order_relaxed);
    if (stats.totalUpdates > 0)
        stats.averageUpdateDurationMs = static_cast<uint32>(totalTime / stats.totalUpdates);
    else
        stats.averageUpdateDurationMs = 0;

    return stats;
}

} // namespace Playerbot