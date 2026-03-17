/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_SPATIAL_GRID_SCHEDULER_H
#define PLAYERBOT_SPATIAL_GRID_SCHEDULER_H

#include "Define.h"
#include <chrono>
#include <atomic>

namespace Playerbot
{

/**
 * @brief Centralized scheduler for spatial grid updates
 *
 * DEADLOCK FIX: Removes update contention by centralizing all spatial grid
 * updates to a single, predictable location instead of having 25+ threads
 * all trying to update simultaneously.
 *
 * This scheduler ensures:
 * - Updates happen at most once per server tick (50ms)
 * - No thread contention on update mutex
 * - Predictable update timing for all grids
 * - Zero blocking on query operations
 */
class TC_GAME_API SpatialGridScheduler
{
public:
    static SpatialGridScheduler& Instance()
    {
        static SpatialGridScheduler instance;
        return instance;
    }

    /**
     * @brief Update all spatial grids (call from World::Update or Map::Update)
     *
     * This should be called once per server tick from a single location,
     * NOT from multiple threads. Typical integration points:
     * - World::Update() for global update
     * - Map::Update() for per-map update
     * - BotWorldSessionMgr::UpdateBots() before bot updates
     *
     * @param diff Time since last update in milliseconds
     */
    void UpdateAllGrids(uint32 diff);

    /**
     * @brief Update a specific map's spatial grid
     * @param mapId Map ID to update
     * @param forceUpdate If true, bypass rate limiting
     */
    void UpdateMapGrid(uint32 mapId, bool forceUpdate = false);

    /**
     * @brief Check if updates are enabled
     */
    bool IsEnabled() const { return _enabled.load(::std::memory_order_relaxed); }

    /**
     * @brief Enable/disable automatic updates
     */
    void SetEnabled(bool enabled) { _enabled.store(enabled, ::std::memory_order_relaxed); }

    /**
     * @brief Get update interval in milliseconds
     */
    uint32 GetUpdateInterval() const { return _updateInterval; }

    /**
     * @brief Set update interval (minimum 50ms)
     */
    void SetUpdateInterval(uint32 intervalMs);

    /**
     * @brief Get statistics
     */
    struct Statistics
    {
        uint64 totalUpdates;
        uint64 skippedUpdates;
        uint32 lastUpdateDurationMs;
        uint32 averageUpdateDurationMs;
        ::std::chrono::steady_clock::time_point lastUpdateTime;
    };

    Statistics GetStatistics() const;

private:
    SpatialGridScheduler();
    ~SpatialGridScheduler() = default;

    // Configuration
    ::std::atomic<bool> _enabled{true};
    ::std::atomic<uint32> _updateInterval{100};  // 100ms default (10Hz)

    // Timing
    ::std::chrono::steady_clock::time_point _lastUpdateTime;
    uint32 _timeSinceLastUpdate{0};

    // Statistics
    ::std::atomic<uint64> _totalUpdates{0};
    ::std::atomic<uint64> _skippedUpdates{0};
    ::std::atomic<uint32> _lastUpdateDurationMs{0};
    ::std::atomic<uint64> _totalUpdateTimeMs{0};

    // Non-copyable
    SpatialGridScheduler(SpatialGridScheduler const&) = delete;
    SpatialGridScheduler& operator=(SpatialGridScheduler const&) = delete;
};

// Convenience accessor
#define sSpatialGridScheduler Playerbot::SpatialGridScheduler::Instance()

} // namespace Playerbot

#endif // PLAYERBOT_SPATIAL_GRID_SCHEDULER_H