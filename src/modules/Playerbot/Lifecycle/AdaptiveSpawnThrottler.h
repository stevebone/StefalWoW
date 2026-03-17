/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_ADAPTIVE_SPAWN_THROTTLER_H
#define PLAYERBOT_ADAPTIVE_SPAWN_THROTTLER_H

#include "Define.h"
#include "Duration.h"
#include "ResourceMonitor.h"
#include "SpawnCircuitBreaker.h"
#include "SpawnPriorityQueue.h"

namespace Playerbot
{
    /**
     * @brief Configuration for adaptive spawn throttling
     */
    struct TC_GAME_API ThrottlerConfig
    {
        uint32 baseSpawnIntervalMs = 100;        ///< Base spawn interval (100ms = 10 bots/sec)
        uint32 minSpawnIntervalMs = 50;          ///< Minimum spawn interval (50ms = 20 bots/sec max)
        uint32 maxSpawnIntervalMs = 5000;        ///< Maximum spawn interval (5s = 0.2 bots/sec min)

        float normalPressureMultiplier = 1.0f;   ///< 100% spawn rate at normal pressure
        float elevatedPressureMultiplier = 0.5f; ///< 50% spawn rate at elevated pressure
        float highPressureMultiplier = 0.25f;    ///< 25% spawn rate at high pressure
        float criticalPressureMultiplier = 0.0f; ///< 0% spawn rate at critical pressure (pause)

        uint32 burstWindowMs = 10000;            ///< Burst detection window (10 seconds)
        uint32 maxBurstsPerWindow = 50;          ///< Max spawn bursts per window

        // ============================================================================
        // CRITICAL FIX: Per-update-cycle spawn limit to prevent visibility update hang
        // ============================================================================
        // When multiple bots spawn in the same Map::Update cycle, their visibility
        // updates accumulate and are processed together in Map::ProcessRelocationNotifies().
        // This causes O(n^2) processing time with many bots + creatures.
        // Limiting spawns per update cycle spreads visibility updates across cycles.
        // ============================================================================
        uint32 maxSpawnsPerUpdateCycle = 1;      ///< Max bots spawned per update cycle (1-2 recommended)

        bool enableAdaptiveThrottling = true;    ///< Master enable for adaptive throttling
        bool enableCircuitBreaker = true;        ///< Enable circuit breaker protection
        bool enableBurstPrevention = true;       ///< Enable burst prevention

        /**
         * @brief Load configuration from playerbots.conf
         */
        void LoadFromConfig();
    };

    /**
     * @brief Throttler metrics for monitoring and debugging
     */
    struct TC_GAME_API ThrottlerMetrics
    {
        uint32 currentSpawnIntervalMs = 0;       ///< Current calculated spawn interval
        float currentSpawnRatePerSec = 0.0f;     ///< Current spawn rate (bots/sec)
        float effectiveMultiplier = 1.0f;        ///< Effective rate multiplier (combined)

        ResourcePressure currentPressure = ResourcePressure::NORMAL;
        CircuitState circuitState = CircuitState::CLOSED;

        uint32 spawnsSinceLastUpdate = 0;        ///< Spawns in last update cycle
        uint32 spawnsThisUpdateCycle = 0;        ///< Spawns in current update cycle (before reset)
        uint32 updateCycleThrottleBlocks = 0;    ///< Times per-cycle limit blocked a spawn
        uint32 totalSpawnsThrottled = 0;         ///< Total spawns delayed/blocked
        uint32 burstPreventionTriggers = 0;      ///< Times burst prevention activated

        Milliseconds timeSinceLastSpawn = Milliseconds(0);
        Milliseconds averageSpawnInterval = Milliseconds(0);
    };

    /**
     * @brief Adaptive spawn throttler - Core throttling logic
     *
     * Integrates ResourceMonitor and SpawnCircuitBreaker to dynamically
     * adjust bot spawn rates based on:
     * - Server resource pressure (CPU/memory)
     * - Circuit breaker state (failure detection)
     * - Burst prevention (spike protection)
     * - Priority-based queueing
     *
     * Algorithm:
     * 1. Check circuit breaker state (OPEN blocks all spawns)
     * 2. Get resource pressure from ResourceMonitor
     * 3. Calculate combined multiplier (pressure × circuit state × burst)
     * 4. Adjust spawn interval: interval = baseInterval / multiplier
     * 5. Apply min/max clamps
     *
     * Features:
     * - Real-time spawn rate adjustment (10-20 bots/sec range)
     * - Automatic pressure relief (reduces rate when stressed)
     * - Circuit breaker integration (blocks on high failure rate)
     * - Burst spike prevention (smooths sudden spawn requests)
     * - Priority queue support (spawns high-priority bots first)
     *
     * Performance:
     * - Overhead: O(1) per spawn check
     * - Memory: ~2KB
     * - Update frequency: Every world update tick
     *
     * Thread Safety:
     * - Not thread-safe, must be called from world update thread
     *
     * @example
     * @code
     * AdaptiveSpawnThrottler throttler;
     * throttler.Initialize(resourceMonitor, circuitBreaker);
     *
     * // Check if spawn allowed
     * if (throttler.CanSpawnNow())
     * {
     *     SpawnRequest request = priorityQueue.DequeueNextRequest();
     *     if (TrySpawnBot(request))
     *         throttler.RecordSpawnSuccess();
     *     else
     *         throttler.RecordSpawnFailure();
     * }
     * @endcode
     */
    class TC_GAME_API AdaptiveSpawnThrottler
    {
    public:
        AdaptiveSpawnThrottler() = default;
        ~AdaptiveSpawnThrottler() = default;

        /**
         * @brief Initialize throttler with dependencies
         * @param resourceMonitor Pointer to ResourceMonitor instance
         * @param circuitBreaker Pointer to SpawnCircuitBreaker instance
         * @return true on successful initialization
         */
        bool Initialize(ResourceMonitor* resourceMonitor, SpawnCircuitBreaker* circuitBreaker);

        /**
         * @brief Update throttler state (called every world tick)
         * @param diff Milliseconds since last update
         */
        void Update(uint32 diff);

        /**
         * @brief Check if spawning a bot is allowed right now
         * @return true if spawn should proceed, false if throttled
         *
         * Checks:
         * 1. Circuit breaker allows spawn (not OPEN)
         * 2. Per-update-cycle spawn limit not exceeded (CRITICAL for visibility update performance)
         * 3. Enough time passed since last spawn (interval check)
         * 4. Not in burst prevention mode
         */
        bool CanSpawnNow() const;

        /**
         * @brief Record successful bot spawn
         *
         * Updates:
         * - Last spawn timestamp
         * - Burst tracking window
         * - Spawn success metrics
         */
        void RecordSpawnSuccess();

        /**
         * @brief Record failed bot spawn attempt
         * @param reason Optional failure reason for logging
         *
         * Forwards to circuit breaker for failure tracking
         */
        void RecordSpawnFailure(::std::string_view reason = "");

        /**
         * @brief Get current spawn interval in milliseconds
         * @return uint32 Current spawn interval (clamped to min/max)
         */
        uint32 GetCurrentSpawnInterval() const { return _currentSpawnInterval; }

        /**
         * @brief Get current effective spawn rate in bots/second
         * @return float Spawn rate (0.2 - 20.0 bots/sec)
         */
        float GetCurrentSpawnRate() const;

        /**
         * @brief Get time remaining until next spawn allowed
         * @return Milliseconds Time to wait (0 if can spawn now)
         */
        Milliseconds GetTimeUntilNextSpawn() const;

        /**
         * @brief Get throttler metrics for monitoring
         * @return ThrottlerMetrics Current state and statistics
         */
        ThrottlerMetrics GetMetrics() const;

        /**
         * @brief Force recalculation of spawn interval
         *
         * Useful after major config changes or manual pressure relief
         */
        void RecalculateInterval();

        /**
         * @brief Check if throttler is initialized
         * @return true if initialized
         */
        bool IsInitialized() const { return _initialized; }

    private:
        /**
         * @brief Calculate spawn interval based on current conditions
         * @return uint32 Calculated interval in milliseconds
         *
         * Formula:
         * interval = baseInterval / (pressureMultiplier × circuitMultiplier × burstMultiplier)
         * clamped to [minInterval, maxInterval]
         */
        uint32 CalculateSpawnInterval() const;

        /**
         * @brief Get multiplier for current resource pressure
         * @return float Multiplier (0.0 - 1.0)
         */
        float GetPressureMultiplier() const;

        /**
         * @brief Get multiplier for current circuit breaker state
         * @return float Multiplier (0.0 - 1.0)
         *
         * Returns:
         * - CLOSED: 1.0 (normal)
         * - HALF_OPEN: 0.5 (limited spawning)
         * - OPEN: 0.0 (blocked)
         */
        float GetCircuitBreakerMultiplier() const;

        /**
         * @brief Check if currently in burst prevention mode
         * @return bool true if burst detected and prevention active
         */
        bool IsInBurstPrevention() const;

        /**
         * @brief Update burst tracking window
         *
         * Removes old spawn timestamps outside burst window
         */
        void UpdateBurstTracking();

    private:
        // Configuration
        ThrottlerConfig _config;

        // Dependencies (not owned)
        ResourceMonitor* _resourceMonitor = nullptr;
        SpawnCircuitBreaker* _circuitBreaker = nullptr;

        // Spawn tracking
        TimePoint _lastSpawnTime;                ///< Last successful spawn timestamp
        uint32 _currentSpawnInterval = 100;      ///< Current calculated interval (ms)

        // Burst prevention tracking
        ::std::deque<TimePoint> _recentSpawnTimes; ///< Timestamps for burst detection
        uint32 _burstPreventionCount = 0;        ///< Times burst prevention triggered

        // Metrics
        uint32 _totalSpawnsThrottled = 0;        ///< Total spawns delayed/blocked
        uint32 _spawnsSinceLastUpdate = 0;       ///< Spawns in last update cycle

        // Per-update-cycle tracking (CRITICAL FIX for visibility update hang)
        mutable uint32 _spawnsThisUpdateCycle = 0;     ///< Spawns in current update cycle
        uint32 _updateCycleThrottleBlocks = 0;   ///< Times per-cycle limit blocked a spawn

        // State
        bool _initialized = false;
    };

} // namespace Playerbot

#endif // PLAYERBOT_ADAPTIVE_SPAWN_THROTTLER_H
