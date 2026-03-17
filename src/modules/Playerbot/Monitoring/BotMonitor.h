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

#ifndef _PLAYERBOT_MONITOR_H
#define _PLAYERBOT_MONITOR_H

#include "PerformanceMetrics.h"
#include "Threading/LockHierarchy.h"
#include "Define.h"
#include "ObjectGuid.h"
#include <map>
#include <vector>
#include <deque>
#include <mutex>
#include <chrono>
#include <functional>
#include <set>

namespace Playerbot
{
    /**
     * @class BotMonitor
     * @brief Central monitoring system for playerbot performance and health
     *
     * Provides real-time metrics collection, trend analysis, alerting, and
     * performance snapshot capabilities for the playerbot system.
     *
     * Thread-safe singleton implementation for concurrent access.
     */
    class TC_GAME_API BotMonitor final 
    {
    public:
        /**
         * @brief Get singleton instance
         */
        static BotMonitor* instance();

        // IBotMonitor interface implementation

        /**
         * @brief Initialize monitoring system
         * @return true if initialization successful
         */
        bool Initialize();

        /**
         * @brief Shutdown monitoring system
         */
        void Shutdown();

        /**
         * @brief Update monitoring system (called periodically)
         * @param diff Time since last update in milliseconds
         */
        void Update(uint32 diff);

        // =====================================================================
        // METRICS COLLECTION
        // =====================================================================

        /**
         * @brief Capture current performance snapshot
         * @return Current performance metrics snapshot
         */
        PerformanceSnapshot CaptureSnapshot();

        /**
         * @brief Get most recent snapshot
         * @return Most recent performance snapshot
         */
        PerformanceSnapshot GetLatestSnapshot() const;

        /**
         * @brief Get historical snapshots
         * @param count Number of snapshots to retrieve (default: all)
         * @return Vector of historical snapshots (newest first)
         */
        ::std::vector<PerformanceSnapshot> GetSnapshotHistory(uint32 count = 0) const;

        // =====================================================================
        // ACTIVITY TRACKING
        // =====================================================================

        /**
         * @brief Record bot entering combat
         * @param botGuid Bot's ObjectGuid
         */
        void RecordBotCombatStart(ObjectGuid botGuid);

        /**
         * @brief Record bot leaving combat
         * @param botGuid Bot's ObjectGuid
         */
        void RecordBotCombatEnd(ObjectGuid botGuid);

        /**
         * @brief Record bot starting quest
         * @param botGuid Bot's ObjectGuid
         */
        void RecordBotQuestStart(ObjectGuid botGuid);

        /**
         * @brief Record bot completing quest
         * @param botGuid Bot's ObjectGuid
         */
        void RecordBotQuestEnd(ObjectGuid botGuid);

        /**
         * @brief Record bot death
         * @param botGuid Bot's ObjectGuid
         */
        void RecordBotDeath(ObjectGuid botGuid);

        /**
         * @brief Record bot resurrection
         * @param botGuid Bot's ObjectGuid
         */
        void RecordBotResurrection(ObjectGuid botGuid);

        /**
         * @brief Record bot update timing
         * @param botGuid Bot's ObjectGuid
         * @param updateTimeMs Update time in milliseconds
         */
        void RecordBotUpdateTime(ObjectGuid botGuid, double updateTimeMs);

        /**
         * @brief Record AI decision timing
         * @param botGuid Bot's ObjectGuid
         * @param decisionTimeMs Decision time in milliseconds
         */
        void RecordAIDecisionTime(ObjectGuid botGuid, double decisionTimeMs);

        // =====================================================================
        // RESOURCE TRACKING
        // =====================================================================

        /**
         * @brief Record database query execution
         * @param queryTimeMs Query execution time in milliseconds
         */
        void RecordDatabaseQuery(double queryTimeMs);

        /**
         * @brief Record database cache hit
         */
        void RecordDatabaseCacheHit();

        /**
         * @brief Record database cache miss
         */
        void RecordDatabaseCacheMiss();

        /**
         * @brief Record error occurrence
         * @param category Error category (e.g., "Combat", "Movement", "Database")
         * @param message Error message
         */
        void RecordError(::std::string const& category, ::std::string const& message);

        /**
         * @brief Record warning occurrence
         * @param category Warning category
         * @param message Warning message
         */
        void RecordWarning(::std::string const& category, ::std::string const& message);

        // =====================================================================
        // TREND ANALYSIS
        // =====================================================================

        /**
         * @brief Get CPU usage trend data
         * @return TrendData for CPU usage over time
         */
        TrendData GetCpuTrend() const;

        /**
         * @brief Get memory usage trend data
         * @return TrendData for memory usage over time
         */
        TrendData GetMemoryTrend() const;

        /**
         * @brief Get active bot count trend data
         * @return TrendData for bot count over time
         */
        TrendData GetBotCountTrend() const;

        /**
         * @brief Get database query performance trend
         * @return TrendData for database query times
         */
        TrendData GetQueryTimeTrend() const;

        // =====================================================================
        // ALERT MANAGEMENT
        // =====================================================================

        /**
         * @brief Get current alert thresholds
         * @return Current alert threshold configuration
         */
        AlertThresholds GetAlertThresholds() const;

        /**
         * @brief Set alert thresholds
         * @param thresholds New alert threshold configuration
         */
        void SetAlertThresholds(AlertThresholds const& thresholds);

        /**
         * @brief Get active alerts
         * @param minLevel Minimum alert level to retrieve (default: WARNING)
         * @return Vector of active alerts
         */
        ::std::vector<PerformanceAlert> GetActiveAlerts(AlertLevel minLevel = AlertLevel::WARNING) const;

        /**
         * @brief Get alert history
         * @param count Number of alerts to retrieve (default: 100)
         * @return Vector of historical alerts (newest first)
         */
        ::std::vector<PerformanceAlert> GetAlertHistory(uint32 count = 100) const;

        /**
         * @brief Clear alert history
         */
        void ClearAlertHistory();

        /**
         * @brief Register alert callback
         * @param callback Function to call when alert is triggered
         */
        void RegisterAlertCallback(::std::function<void(PerformanceAlert const&)> callback);

        // =====================================================================
        // STATISTICS
        // =====================================================================

        /**
         * @brief Get formatted statistics summary
         * @return Human-readable statistics string
         */
        ::std::string GetStatisticsSummary() const;

        /**
         * @brief Get uptime in seconds
         * @return System uptime since initialization
         */
        uint64 GetUptimeSeconds() const;

        /**
         * @brief Reset all statistics
         */
        void ResetStatistics();

    private:
        BotMonitor();
        ~BotMonitor();

        BotMonitor(BotMonitor const&) = delete;
        BotMonitor& operator=(BotMonitor const&) = delete;

        // Internal update methods
        void UpdateActivityMetrics();
        void UpdateResourceMetrics();
        void UpdateDatabaseMetrics();
        void UpdateTrendData();
        void CheckAlerts();
        void TriggerAlert(PerformanceAlert const& alert);

        // Helper methods
        BotActivityMetrics CollectActivityMetrics() const;
        SystemResourceMetrics CollectResourceMetrics() const;
        DatabaseMetrics CollectDatabaseMetrics() const;
        double CalculateCpuUsage() const;
        uint64 CalculateMemoryUsage() const;
        uint32 GetActiveThreadCount() const;
        double CalculateNetworkThroughput() const;

        // Thread safety
        mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BEHAVIOR_MANAGER> _mutex;

        // Initialization state
        bool _initialized;
        ::std::chrono::system_clock::time_point _initTime;
        ::std::chrono::system_clock::time_point _lastUpdateTime;

        // Activity tracking
        ::std::map<ObjectGuid, ::std::string> _botActivityState;  // botGuid -> state (combat/questing/traveling/idle/dead)
        ::std::map<ObjectGuid, ::std::chrono::system_clock::time_point> _botActivityStartTime;

        // Bot sets for quick categorization
        ::std::set<ObjectGuid> _botsInCombat;
        ::std::set<ObjectGuid> _botsQuesting;
        ::std::set<ObjectGuid> _botsDead;

        // Performance tracking
        ::std::deque<double> _updateTimes;           // Recent bot update times
        ::std::deque<double> _aiDecisionTimes;       // Recent AI decision times
        double _totalUpdateTime;
        double _maxUpdateTime;
        uint64 _updateCount;

        // Database tracking
        ::std::deque<double> _queryTimes;            // Recent query execution times
        uint64 _totalQueries;
        uint64 _totalCacheHits;
        uint64 _totalCacheMisses;
        double _totalQueryTime;
        double _maxQueryTime;

        // Resource tracking
        ::std::chrono::system_clock::time_point _lastCpuCheck;
        double _lastCpuUsage;

        // Snapshot history
        ::std::deque<PerformanceSnapshot> _snapshotHistory;
        static constexpr uint32 MAX_SNAPSHOT_HISTORY = 1440;  // 24 hours at 1-minute intervals

        // Trend data
        TrendData _cpuTrend;
        TrendData _memoryTrend;
        TrendData _botCountTrend;
        TrendData _queryTimeTrend;

        // Alert system
        AlertThresholds _alertThresholds;
        ::std::deque<PerformanceAlert> _alertHistory;
        ::std::vector<::std::function<void(PerformanceAlert const&)>> _alertCallbacks;
        static constexpr uint32 MAX_ALERT_HISTORY = 1000;

        // Error/warning tracking
        uint32 _errorCount;
        uint32 _warningCount;
        ::std::map<::std::string, uint32> _errorsByCategory;
        ::std::map<::std::string, uint32> _warningsByCategory;
    };

    #define sBotMonitor BotMonitor::instance()

} // namespace Playerbot

#endif // _PLAYERBOT_MONITOR_H
