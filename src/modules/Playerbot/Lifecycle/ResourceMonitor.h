/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PLAYERBOT_RESOURCE_MONITOR_H
#define PLAYERBOT_RESOURCE_MONITOR_H

#include "Define.h"
#include "Duration.h"
#include <array>
#include <deque>
#include <chrono>

namespace Playerbot
{
    /**
     * @brief Resource pressure levels for adaptive throttling
     *
     * Pressure levels determine how aggressively the spawn throttler
     * should reduce spawn rates to protect server stability.
     */
    enum class ResourcePressure : uint8
    {
        NORMAL = 0,     ///< <60% CPU, <70% memory - full speed spawning allowed
        ELEVATED = 1,   ///< 60-75% CPU, 70-80% memory - reduce spawn rate by 50%
        HIGH = 2,       ///< 75-85% CPU, 80-90% memory - reduce spawn rate by 75%
        CRITICAL = 3    ///< >85% CPU, >90% memory - pause spawning, trigger circuit breaker
    };

    /**
     * @brief Real-time server resource metrics snapshot
     *
     * Captures CPU, memory, database, and map instance metrics
     * at a specific point in time for throttling decisions.
     */
    struct TC_GAME_API ResourceMetrics
    {
        // CPU metrics
        float cpuUsagePercent = 0.0f;        ///< Current CPU usage (0-100%)
        float cpuUsage5sAvg = 0.0f;          ///< 5-second moving average
        float cpuUsage30sAvg = 0.0f;         ///< 30-second moving average
        float cpuUsage60sAvg = 0.0f;         ///< 60-second moving average

        // Memory metrics
        float memoryUsagePercent = 0.0f;     ///< Current memory usage (0-100%)
        uint64 workingSetMB = 0;             ///< Working set size in MB
        uint64 commitSizeMB = 0;             ///< Commit size in MB

        // Database metrics
        uint32 activeDbConnections = 0;      ///< Currently active DB connections
        uint32 maxDbConnections = 0;         ///< Maximum DB connection pool size
        float dbConnectionUsagePercent = 0.0f; ///< DB pool utilization (0-100%)

        // Map instance metrics
        uint32 activeMapInstances = 0;       ///< Number of loaded map instances
        uint32 totalActiveBots = 0;          ///< Total currently spawned bots

        // Timestamp
        TimePoint collectionTime;            ///< When metrics were collected

        /**
         * @brief Get overall resource pressure level
         * @return ResourcePressure enum indicating current system load
         */
        ResourcePressure GetPressureLevel() const;

        /**
         * @brief Check if spawning is safe at current resource levels
         * @return true if resources are below critical thresholds
         */
        bool IsSpawningSafe() const;

        /**
         * @brief Get recommended spawn rate multiplier based on resources
         * @return float 0.0-1.0 representing safe spawn rate percentage
         */
        float GetSpawnRateMultiplier() const;
    };

    /**
     * @brief Configuration for resource pressure thresholds
     *
     * Defines CPU and memory percentage thresholds for each
     * ResourcePressure level. Loaded from playerbots.conf.
     */
    struct TC_GAME_API ResourceThresholds
    {
        // CPU thresholds (percentage)
        float cpuNormalThreshold = 60.0f;     ///< Threshold for NORMAL → ELEVATED
        float cpuElevatedThreshold = 75.0f;   ///< Threshold for ELEVATED → HIGH
        float cpuHighThreshold = 85.0f;       ///< Threshold for HIGH → CRITICAL

        // Memory thresholds (percentage)
        float memoryNormalThreshold = 70.0f;  ///< Threshold for NORMAL → ELEVATED
        float memoryElevatedThreshold = 80.0f;///< Threshold for ELEVATED → HIGH
        float memoryHighThreshold = 90.0f;    ///< Threshold for HIGH → CRITICAL

        // DB connection thresholds (percentage)
        float dbConnectionThreshold = 80.0f;  ///< DB pool usage warning threshold

        /**
         * @brief Load thresholds from configuration
         */
        void LoadFromConfig();
    };

    /**
     * @brief Real-time server resource monitoring system
     *
     * Monitors CPU, memory, database connections, and map instances to
     * provide adaptive throttling with resource-aware spawn rates.
     *
     * Features:
     * - Platform-specific CPU/memory monitoring (Windows/Linux)
     * - Moving average calculations (5s, 30s, 60s windows)
     * - Database connection pool monitoring
     * - Map instance tracking
     * - Pressure level calculation for adaptive throttling
     *
     * Performance:
     * - Update frequency: Every 1 second
     * - CPU overhead: <0.01% on modern systems
     * - Memory overhead: ~4KB (sliding window buffers)
     *
     * Thread Safety:
     * - Not thread-safe, must be called from world update thread
     * - Designed for single-threaded access pattern
     *
     * @example
     * @code
     * ResourceMonitor monitor;
     * monitor.Initialize();
     *
     * // In world update loop
     * monitor.Update(diff);
     * ResourceMetrics metrics = monitor.GetCurrentMetrics();
     *
     * if (metrics.GetPressureLevel() == ResourcePressure::CRITICAL)
     * {
     *     // Pause bot spawning
     * }
     * @endcode
     */
    class TC_GAME_API ResourceMonitor
    {
    public:
        ResourceMonitor();
        ~ResourceMonitor();

        /**
         * @brief Initialize resource monitoring system
         * @return true on successful initialization
         */
        bool Initialize();

        /**
         * @brief Update resource metrics (called every world tick)
         * @param diff Milliseconds since last update
         */
        void Update(uint32 diff);

        /**
         * @brief Get current resource metrics snapshot
         * @return ResourceMetrics Current server resource state
         */
        ResourceMetrics GetCurrentMetrics() const { return _currentMetrics; }

        /**
         * @brief Get current resource pressure level
         * @return ResourcePressure enum (NORMAL/ELEVATED/HIGH/CRITICAL)
         */
        ResourcePressure GetPressureLevel() const { return _currentMetrics.GetPressureLevel(); }

        /**
         * @brief Check if spawning is safe at current resource levels
         * @return true if resources are below critical thresholds
         */
        bool IsSpawningSafe() const { return _currentMetrics.IsSpawningSafe(); }

        /**
         * @brief Get recommended spawn rate multiplier
         * @return float 0.0-1.0 representing safe spawn rate percentage
         */
        float GetRecommendedSpawnRateMultiplier() const { return _currentMetrics.GetSpawnRateMultiplier(); }

        /**
         * @brief Get configured resource thresholds
         * @return ResourceThresholds Current threshold configuration
         */
        ResourceThresholds GetThresholds() const { return _thresholds; }

        /**
         * @brief Force immediate metrics collection (for testing)
         */
        void ForceUpdate();

        /**
         * @brief Set total active bot count for tracking
         * @param count Number of currently active bots
         *
         * Called by BotSpawner to update bot count for monitoring
         */
        void SetActiveBotCount(uint32 count) { _currentMetrics.totalActiveBots = count; }

    private:
        /**
         * @brief Collect CPU usage metrics
         * @return Current CPU usage percentage (0-100)
         */
        float CollectCpuUsage();

        /**
         * @brief Collect memory usage metrics
         * @return Current memory usage percentage (0-100)
         */
        float CollectMemoryUsage();

        /**
         * @brief Collect database connection metrics
         */
        void CollectDatabaseMetrics();

        /**
         * @brief Collect map instance metrics
         */
        void CollectMapMetrics();

        /**
         * @brief Update moving average calculations
         */
        void UpdateMovingAverages();

        /**
         * @brief Calculate moving average from sliding window
         * @param window Deque containing recent samples
         * @return Average value over window
         */
        float CalculateAverage(const ::std::deque<float>& window) const;

    private:
        // Current metrics
        ResourceMetrics _currentMetrics;

        // Configuration
        ResourceThresholds _thresholds;

        // Update timing
        uint32 _updateInterval = 1000;       ///< Update frequency (1 second)
        uint32 _timeSinceLastUpdate = 0;     ///< Accumulator for update timing

        // CPU usage tracking (sliding windows)
        ::std::deque<float> _cpuSamples5s;     ///< 5-second window (5 samples @ 1Hz)
        ::std::deque<float> _cpuSamples30s;    ///< 30-second window (30 samples @ 1Hz)
        ::std::deque<float> _cpuSamples60s;    ///< 60-second window (60 samples @ 1Hz)

        // Platform-specific CPU monitoring state
#ifdef _WIN32
        void* _processHandle = nullptr;      ///< Windows process handle
        uint64 _lastCpuTime = 0;             ///< Previous CPU time
        uint64 _lastSystemTime = 0;          ///< Previous system time
#else
        uint64 _lastCpuTime = 0;             ///< Previous CPU time (Linux)
        uint64 _lastTimestamp = 0;           ///< Previous timestamp (Linux)
#endif

        // Initialization state
        bool _initialized = false;
    };

} // namespace Playerbot

#endif // PLAYERBOT_RESOURCE_MONITOR_H
