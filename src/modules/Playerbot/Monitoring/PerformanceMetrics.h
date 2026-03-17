/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
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

#ifndef _PLAYERBOT_PERFORMANCE_METRICS_H
#define _PLAYERBOT_PERFORMANCE_METRICS_H

#include "Define.h"
#include <chrono>
#include <string>
#include <map>

namespace Playerbot
{
    /**
     * @struct BotActivityMetrics
     * @brief Metrics for bot activity categorization
     */
    struct BotActivityMetrics
    {
        uint32 combatCount = 0;      // Bots currently in combat
        uint32 questingCount = 0;    // Bots currently questing
        uint32 travelingCount = 0;   // Bots currently traveling
        uint32 idleCount = 0;        // Bots currently idle
        uint32 deadCount = 0;        // Bots currently dead
        uint32 totalActive = 0;      // Total active bots
    };

    /**
     * @struct SystemResourceMetrics
     * @brief System resource usage metrics
     */
    struct SystemResourceMetrics
    {
        double cpuUsagePercent = 0.0;           // Total CPU usage
        double cpuPerBotPercent = 0.0;          // Average CPU per bot
        uint64 memoryUsedBytes = 0;             // Total memory used
        uint64 memoryPerBotBytes = 0;           // Average memory per bot
        uint32 threadCount = 0;                 // Active thread count
        double networkThroughputMbps = 0.0;     // Network throughput
    };

    /**
     * @struct DatabaseMetrics
     * @brief Database operation metrics
     */
    struct DatabaseMetrics
    {
        uint64 queryCount = 0;                  // Total queries executed
        uint64 queriesPerSecond = 0;            // Query throughput
        double avgQueryTimeMs = 0.0;            // Average query time
        double maxQueryTimeMs = 0.0;            // Maximum query time
        uint32 activeConnections = 0;           // Active DB connections
        uint32 connectionPoolSize = 0;          // Connection pool size
        uint64 cacheHits = 0;                   // Database cache hits
        uint64 cacheMisses = 0;                 // Database cache misses
        uint32 cacheHitRate() const
        {
            uint64 total = cacheHits + cacheMisses;
            return total > 0 ? static_cast<uint32>((cacheHits * 100) / total) : 0;
        }
    };

    /**
     * @struct PerformanceSnapshot
     * @brief Complete performance snapshot at a point in time
     */
    struct PerformanceSnapshot
    {
        ::std::chrono::system_clock::time_point timestamp;
        BotActivityMetrics activity;
        SystemResourceMetrics resources;
        DatabaseMetrics database;

        // Timing metrics
        double avgUpdateTimeMs = 0.0;           // Average bot update time
        double maxUpdateTimeMs = 0.0;           // Maximum bot update time
        double avgAIDecisionTimeMs = 0.0;       // Average AI decision time

        // Uptime
        uint64 uptimeSeconds = 0;               // System uptime

        // Error metrics
        uint32 errorCount = 0;                  // Total errors
        uint32 warningCount = 0;                // Total warnings
    };

    /**
     * @struct TrendData
     * @brief Time-series trend data for graphing
     */
    struct TrendData
    {
        ::std::vector<::std::chrono::system_clock::time_point> timestamps;
        ::std::vector<double> values;

        void AddDataPoint(double value)
        {
            timestamps.push_back(::std::chrono::system_clock::now());
            values.push_back(value);

            // Keep last 60 data points (for 1 hour of 1-minute samples)
            if (timestamps.size() > 60)
            {
                timestamps.erase(timestamps.begin());
                values.erase(values.begin());
            }
        }

        double GetAverage() const
        {
            if (values.empty())
                return 0.0;

            double sum = 0.0;
            for (double val : values)
                sum += val;

            return sum / values.size();
        }

        double GetMin() const
        {
            if (values.empty())
                return 0.0;

            return *::std::min_element(values.begin(), values.end());
        }

        double GetMax() const
        {
            if (values.empty())
                return 0.0;

            return *::std::max_element(values.begin(), values.end());
        }
    };

    /**
     * @struct AlertThresholds
     * @brief Thresholds for performance alerts
     */
    struct AlertThresholds
    {
        double cpuWarning = 70.0;               // CPU warning threshold (%)
        double cpuCritical = 90.0;              // CPU critical threshold (%)
        uint64 memoryWarningMB = 40000;         // Memory warning (40GB)
        uint64 memoryCriticalMB = 55000;        // Memory critical (55GB)
        double queryTimeWarningMs = 50.0;       // Query time warning
        double queryTimeCriticalMs = 100.0;     // Query time critical
        uint32 botCrashRateWarning = 5;         // Bot crash rate warning (%)
        uint32 botCrashRateCritical = 10;       // Bot crash rate critical (%)
    };

    /**
     * @enum AlertLevel
     * @brief Alert severity levels
     */
    enum class AlertLevel
    {
        NONE,
        INFO,
        WARNING,
        CRITICAL
    };

    /**
     * @struct PerformanceAlert
     * @brief Performance alert notification
     */
    struct PerformanceAlert
    {
        AlertLevel level;
        ::std::string category;                   // CPU, Memory, Database, Bots
        ::std::string message;
        ::std::chrono::system_clock::time_point timestamp;
        double currentValue;
        double thresholdValue;
    };

} // namespace Playerbot

#endif // _PLAYERBOT_PERFORMANCE_METRICS_H
