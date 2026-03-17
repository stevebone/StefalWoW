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

#include "BotMonitor.h"
#include "Log.h"
#include "World.h"
#include "WorldSession.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "DatabaseEnv.h"
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace Playerbot
{
    BotMonitor* BotMonitor::instance()
    {
        static BotMonitor instance;
        return &instance;
    }

    BotMonitor::BotMonitor()
        : _initialized(false)
        , _totalUpdateTime(0.0)
        , _maxUpdateTime(0.0)
        , _updateCount(0)
        , _totalQueries(0)
        , _totalCacheHits(0)
        , _totalCacheMisses(0)
        , _totalQueryTime(0.0)
        , _maxQueryTime(0.0)
        , _lastCpuUsage(0.0)
        , _errorCount(0)
        , _warningCount(0)
    {
    }

    BotMonitor::~BotMonitor()
    {
        Shutdown();
    }

    bool BotMonitor::Initialize()
    {
        ::std::lock_guard lock(_mutex);

        if (_initialized)

            return true;

        _initTime = ::std::chrono::system_clock::now();
        _lastUpdateTime = _initTime;
        _lastCpuCheck = _initTime;

        // Initialize default alert thresholds
        _alertThresholds = AlertThresholds{};

        _initialized = true;

        TC_LOG_INFO("playerbot", "BotMonitor: Initialized successfully");
        return true;
    }

    void BotMonitor::Shutdown()
    {
        ::std::lock_guard lock(_mutex);

        if (!_initialized)

            return;

        _botActivityState.clear();
        _botActivityStartTime.clear();
        _botsInCombat.clear();
        _botsQuesting.clear();
        _botsDead.clear();
        _updateTimes.clear();
        _aiDecisionTimes.clear();
        _queryTimes.clear();
        _snapshotHistory.clear();
        _alertHistory.clear();
        _alertCallbacks.clear();
        _errorsByCategory.clear();
        _warningsByCategory.clear();

        _initialized = false;

        TC_LOG_INFO("playerbot", "BotMonitor: Shutdown complete");
    }

    void BotMonitor::Update(uint32 /*diff*/)
    {
        ::std::lock_guard lock(_mutex);

        if (!_initialized)

            return;

        auto now = ::std::chrono::system_clock::now();
        auto elapsedSinceLastUpdate = ::std::chrono::duration_cast<::std::chrono::milliseconds>(now - _lastUpdateTime).count();

        // Update every 60 seconds (1 minute)
    if (elapsedSinceLastUpdate >= 60000)
        {

            UpdateActivityMetrics();

            UpdateResourceMetrics();

            UpdateDatabaseMetrics();

            UpdateTrendData();

            CheckAlerts();

            // Capture snapshot

            PerformanceSnapshot snapshot = CaptureSnapshot();

            _snapshotHistory.push_back(snapshot);

            // Keep only MAX_SNAPSHOT_HISTORY snapshots
    if (_snapshotHistory.size() > MAX_SNAPSHOT_HISTORY)

                _snapshotHistory.pop_front();


            _lastUpdateTime = now;
        }
    }

    // =====================================================================
    // METRICS COLLECTION
    // =====================================================================

    PerformanceSnapshot BotMonitor::CaptureSnapshot()
    {
        ::std::lock_guard lock(_mutex);

        PerformanceSnapshot snapshot;
        snapshot.timestamp = ::std::chrono::system_clock::now();
        snapshot.activity = CollectActivityMetrics();
        snapshot.resources = CollectResourceMetrics();
        snapshot.database = CollectDatabaseMetrics();

        // Calculate average update time
    if (!_updateTimes.empty())
        {

            double sum = ::std::accumulate(_updateTimes.begin(), _updateTimes.end(), 0.0);

            snapshot.avgUpdateTimeMs = sum / _updateTimes.size();
        }
        else
        {

            snapshot.avgUpdateTimeMs = 0.0;
        }

        snapshot.maxUpdateTimeMs = _maxUpdateTime;

        // Calculate average AI decision time
    if (!_aiDecisionTimes.empty())
        {

            double sum = ::std::accumulate(_aiDecisionTimes.begin(), _aiDecisionTimes.end(), 0.0);

            snapshot.avgAIDecisionTimeMs = sum / _aiDecisionTimes.size();
        }
        else
        {

            snapshot.avgAIDecisionTimeMs = 0.0;
        }

        snapshot.uptimeSeconds = GetUptimeSeconds();
        snapshot.errorCount = _errorCount;
        snapshot.warningCount = _warningCount;

        return snapshot;
    }

    PerformanceSnapshot BotMonitor::GetLatestSnapshot() const
    {
        ::std::lock_guard lock(_mutex);

        if (_snapshotHistory.empty())

            return const_cast<BotMonitor*>(this)->CaptureSnapshot();

        return _snapshotHistory.back();
    }

    ::std::vector<PerformanceSnapshot> BotMonitor::GetSnapshotHistory(uint32 count) const
    {
        ::std::lock_guard lock(_mutex);

        ::std::vector<PerformanceSnapshot> result;

        if (count == 0 || count > _snapshotHistory.size())

            count = _snapshotHistory.size();

        // Return newest snapshots first
        auto it = _snapshotHistory.rbegin();
        for (uint32 i = 0; i < count && it != _snapshotHistory.rend(); ++i, ++it)

            result.push_back(*it);

        return result;
    }

    // =====================================================================
    // ACTIVITY TRACKING
    // =====================================================================

    void BotMonitor::RecordBotCombatStart(ObjectGuid botGuid)
    {
        ::std::lock_guard lock(_mutex);
        _botsInCombat.insert(botGuid);
        _botActivityState[botGuid] = "combat";
        _botActivityStartTime[botGuid] = ::std::chrono::system_clock::now();
    }

    void BotMonitor::RecordBotCombatEnd(ObjectGuid botGuid)
    {
        ::std::lock_guard lock(_mutex);
        _botsInCombat.erase(botGuid);
        _botActivityState[botGuid] = "idle";
    }

    void BotMonitor::RecordBotQuestStart(ObjectGuid botGuid)
    {
        ::std::lock_guard lock(_mutex);
        _botsQuesting.insert(botGuid);
        _botActivityState[botGuid] = "questing";
        _botActivityStartTime[botGuid] = ::std::chrono::system_clock::now();
    }

    void BotMonitor::RecordBotQuestEnd(ObjectGuid botGuid)
    {
        ::std::lock_guard lock(_mutex);
        _botsQuesting.erase(botGuid);
        _botActivityState[botGuid] = "idle";
    }

    void BotMonitor::RecordBotDeath(ObjectGuid botGuid)
    {
        ::std::lock_guard lock(_mutex);
        _botsDead.insert(botGuid);
        _botActivityState[botGuid] = "dead";
        _botActivityStartTime[botGuid] = ::std::chrono::system_clock::now();
    }

    void BotMonitor::RecordBotResurrection(ObjectGuid botGuid)
    {
        ::std::lock_guard lock(_mutex);
        _botsDead.erase(botGuid);
        _botActivityState[botGuid] = "idle";
    }

    void BotMonitor::RecordBotUpdateTime(ObjectGuid /*botGuid*/, double updateTimeMs)
    {
        ::std::lock_guard lock(_mutex);

        _updateTimes.push_back(updateTimeMs);
        _totalUpdateTime += updateTimeMs;
        _updateCount++;

        if (updateTimeMs > _maxUpdateTime)

            _maxUpdateTime = updateTimeMs;

        // Keep only last 1000 update times
    if (_updateTimes.size() > 1000)
        {

            _totalUpdateTime -= _updateTimes.front();

            _updateTimes.pop_front();
        }
    }

    void BotMonitor::RecordAIDecisionTime(ObjectGuid /*botGuid*/, double decisionTimeMs)
    {
        ::std::lock_guard lock(_mutex);

        _aiDecisionTimes.push_back(decisionTimeMs);

        // Keep only last 1000 decision times
    if (_aiDecisionTimes.size() > 1000)

            _aiDecisionTimes.pop_front();
    }

    // =====================================================================
    // RESOURCE TRACKING
    // =====================================================================

    void BotMonitor::RecordDatabaseQuery(double queryTimeMs)
    {
        ::std::lock_guard lock(_mutex);

        _queryTimes.push_back(queryTimeMs);
        _totalQueries++;
        _totalQueryTime += queryTimeMs;

        if (queryTimeMs > _maxQueryTime)

            _maxQueryTime = queryTimeMs;

        // Keep only last 1000 query times
    if (_queryTimes.size() > 1000)
        {

            _totalQueryTime -= _queryTimes.front();

            _queryTimes.pop_front();
        }
    }

    void BotMonitor::RecordDatabaseCacheHit()
    {
        ::std::lock_guard lock(_mutex);
        _totalCacheHits++;
    }

    void BotMonitor::RecordDatabaseCacheMiss()
    {
        ::std::lock_guard lock(_mutex);
        _totalCacheMisses++;
    }

    void BotMonitor::RecordError(::std::string const& category, ::std::string const& message)
    {
        ::std::lock_guard lock(_mutex);
        _errorCount++;
        _errorsByCategory[category]++;

        TC_LOG_ERROR("playerbot", "BotMonitor: Error in %s: %s", category.c_str(), message.c_str());
    }

    void BotMonitor::RecordWarning(::std::string const& category, ::std::string const& message)
    {
        ::std::lock_guard lock(_mutex);
        _warningCount++;
        _warningsByCategory[category]++;

        TC_LOG_WARN("playerbot", "BotMonitor: Warning in %s: %s", category.c_str(), message.c_str());
    }

    // =====================================================================
    // TREND ANALYSIS
    // =====================================================================

    TrendData BotMonitor::GetCpuTrend() const
    {
        ::std::lock_guard lock(_mutex);
        return _cpuTrend;
    }

    TrendData BotMonitor::GetMemoryTrend() const
    {
        ::std::lock_guard lock(_mutex);
        return _memoryTrend;
    }

    TrendData BotMonitor::GetBotCountTrend() const
    {
        ::std::lock_guard lock(_mutex);
        return _botCountTrend;
    }

    TrendData BotMonitor::GetQueryTimeTrend() const
    {
        ::std::lock_guard lock(_mutex);
        return _queryTimeTrend;
    }

    // =====================================================================
    // ALERT MANAGEMENT
    // =====================================================================

    AlertThresholds BotMonitor::GetAlertThresholds() const
    {
        ::std::lock_guard lock(_mutex);
        return _alertThresholds;
    }

    void BotMonitor::SetAlertThresholds(AlertThresholds const& thresholds)
    {
        ::std::lock_guard lock(_mutex);
        _alertThresholds = thresholds;
    }

    ::std::vector<PerformanceAlert> BotMonitor::GetActiveAlerts(AlertLevel minLevel) const
    {
        ::std::lock_guard lock(_mutex);

        ::std::vector<PerformanceAlert> result;

        // Get recent alerts (last 5 minutes)
        auto now = ::std::chrono::system_clock::now();
        auto fiveMinutesAgo = now - ::std::chrono::minutes(5);

        for (auto it = _alertHistory.rbegin(); it != _alertHistory.rend(); ++it)
        {

            if (it->timestamp < fiveMinutesAgo)

                break;


            if (it->level >= minLevel)

                result.push_back(*it);
        }

        return result;
    }

    ::std::vector<PerformanceAlert> BotMonitor::GetAlertHistory(uint32 count) const
    {
        ::std::lock_guard lock(_mutex);

        ::std::vector<PerformanceAlert> result;

        if (count == 0 || count > _alertHistory.size())

            count = _alertHistory.size();

        // Return newest alerts first
        auto it = _alertHistory.rbegin();
        for (uint32 i = 0; i < count && it != _alertHistory.rend(); ++i, ++it)

            result.push_back(*it);

        return result;
    }

    void BotMonitor::ClearAlertHistory()
    {
        ::std::lock_guard lock(_mutex);
        _alertHistory.clear();
    }

    void BotMonitor::RegisterAlertCallback(::std::function<void(PerformanceAlert const&)> callback)
    {
        ::std::lock_guard lock(_mutex);
        _alertCallbacks.push_back(callback);
    }

    // =====================================================================
    // STATISTICS
    // =====================================================================

    ::std::string BotMonitor::GetStatisticsSummary() const
    {
        ::std::lock_guard lock(_mutex);

        PerformanceSnapshot snapshot = const_cast<BotMonitor*>(this)->CaptureSnapshot();

        ::std::ostringstream oss;
        oss << ::std::fixed << ::std::setprecision(2);

        oss << "Playerbot Performance Summary\n";
        oss << "================================================================================\n\n";

        // Bot Activity
        oss << "[Bot Activity]\n";
        oss << "  Total Active:    " << snapshot.activity.totalActive << "\n";
        oss << "  In Combat:       " << snapshot.activity.combatCount << "\n";
        oss << "  Questing:        " << snapshot.activity.questingCount << "\n";
        oss << "  Traveling:       " << snapshot.activity.travelingCount << "\n";

        oss << "  Idle:            " << snapshot.activity.idleCount << "\n";

        oss << "  Dead:            " << snapshot.activity.deadCount << "\n\n";

        // System Resources
        oss << "[System Resources]\n";
        oss << "  CPU Usage:       " << snapshot.resources.cpuUsagePercent << "%\n";
        oss << "  CPU per Bot:     " << snapshot.resources.cpuPerBotPercent << "%\n";
        oss << "  Memory Usage:    " << (snapshot.resources.memoryUsedBytes / 1024 / 1024) << " MB\n";
        oss << "  Memory per Bot:  " << (snapshot.resources.memoryPerBotBytes / 1024) << " KB\n";
        oss << "  Thread Count:    " << snapshot.resources.threadCount << "\n";
        oss << "  Network:         " << snapshot.resources.networkThroughputMbps << " Mbps\n\n";

        // Database Performance
        oss << "[Database Performance]\n";
        oss << "  Total Queries:   " << snapshot.database.queryCount << "\n";
        oss << "  Queries/Second:  " << snapshot.database.queriesPerSecond << "\n";
        oss << "  Avg Query Time:  " << snapshot.database.avgQueryTimeMs << " ms\n";
        oss << "  Max Query Time:  " << snapshot.database.maxQueryTimeMs << " ms\n";
        oss << "  Active Conns:    " << snapshot.database.activeConnections << "\n";
        oss << "  Pool Size:       " << snapshot.database.connectionPoolSize << "\n";
        oss << "  Cache Hit Rate:  " << snapshot.database.cacheHitRate() << "%\n\n";

        // Timing Metrics
        oss << "[Timing Metrics]\n";
        oss << "  Avg Update Time: " << snapshot.avgUpdateTimeMs << " ms\n";
        oss << "  Max Update Time: " << snapshot.maxUpdateTimeMs << " ms\n";
        oss << "  Avg AI Decision: " << snapshot.avgAIDecisionTimeMs << " ms\n\n";

        // System Info
        oss << "[System Info]\n";
        oss << "  Uptime:          " << (snapshot.uptimeSeconds / 3600) << " hours\n";
        oss << "  Errors:          " << snapshot.errorCount << "\n";
        oss << "  Warnings:        " << snapshot.warningCount << "\n\n";

        oss << "================================================================================\n";

        return oss.str();
    }

    uint64 BotMonitor::GetUptimeSeconds() const
    {
        ::std::lock_guard lock(_mutex);

        if (!_initialized)

            return 0;

        auto now = ::std::chrono::system_clock::now();
        auto elapsed = ::std::chrono::duration_cast<::std::chrono::seconds>(now - _initTime);
        return elapsed.count();
    }

    void BotMonitor::ResetStatistics()
    {
        ::std::lock_guard lock(_mutex);

        _totalUpdateTime = 0.0;
        _maxUpdateTime = 0.0;
        _updateCount = 0;
        _totalQueries = 0;
        _totalCacheHits = 0;
        _totalCacheMisses = 0;
        _totalQueryTime = 0.0;
        _maxQueryTime = 0.0;
        _errorCount = 0;
        _warningCount = 0;

        _updateTimes.clear();
        _aiDecisionTimes.clear();
        _queryTimes.clear();
        _errorsByCategory.clear();
        _warningsByCategory.clear();

        TC_LOG_INFO("playerbot", "BotMonitor: Statistics reset");
    }

    // =====================================================================
    // INTERNAL UPDATE METHODS
    // =====================================================================

    void BotMonitor::UpdateActivityMetrics()
    {
        // Activity metrics are updated in real-time via RecordBot* methods
        // This method is for periodic cleanup and validation
    }

    void BotMonitor::UpdateResourceMetrics()
    {
        // Resource metrics are calculated on-demand in CollectResourceMetrics()
    }

    void BotMonitor::UpdateDatabaseMetrics()
    {
        // Database metrics are updated in real-time via RecordDatabase* methods
    }

    void BotMonitor::UpdateTrendData()
    {
        SystemResourceMetrics resources = CollectResourceMetrics();
        BotActivityMetrics activity = CollectActivityMetrics();
        DatabaseMetrics database = CollectDatabaseMetrics();

        _cpuTrend.AddDataPoint(resources.cpuUsagePercent);
        _memoryTrend.AddDataPoint(static_cast<double>(resources.memoryUsedBytes) / 1024 / 1024);  // MB
        _botCountTrend.AddDataPoint(activity.totalActive);

        if (!_queryTimes.empty())
        {

            double avgQueryTime = ::std::accumulate(_queryTimes.begin(), _queryTimes.end(), 0.0) / _queryTimes.size();

            _queryTimeTrend.AddDataPoint(avgQueryTime);
        }
    }

    void BotMonitor::CheckAlerts()
    {
        SystemResourceMetrics resources = CollectResourceMetrics();
        DatabaseMetrics database = CollectDatabaseMetrics();

        // Check CPU usage
    if (resources.cpuUsagePercent >= _alertThresholds.cpuCritical)
        {

            PerformanceAlert alert{

                AlertLevel::CRITICAL,

                "CPU",

                "CPU usage critical",

                ::std::chrono::system_clock::now(),

                resources.cpuUsagePercent,

                _alertThresholds.cpuCritical

            };

            TriggerAlert(alert);
        }
        else if (resources.cpuUsagePercent >= _alertThresholds.cpuWarning)
        {

            PerformanceAlert alert{

                AlertLevel::WARNING,

                "CPU",

                "CPU usage high",

                ::std::chrono::system_clock::now(),

                resources.cpuUsagePercent,

                _alertThresholds.cpuWarning

            };

            TriggerAlert(alert);
        }

        // Check memory usage
        uint64 memoryMB = resources.memoryUsedBytes / 1024 / 1024;
        if (memoryMB >= _alertThresholds.memoryCriticalMB)
        {

            PerformanceAlert alert{

                AlertLevel::CRITICAL,

                "Memory",

                "Memory usage critical",

                ::std::chrono::system_clock::now(),

                static_cast<double>(memoryMB),

                static_cast<double>(_alertThresholds.memoryCriticalMB)

            };

            TriggerAlert(alert);
        }
        else if (memoryMB >= _alertThresholds.memoryWarningMB)
        {

            PerformanceAlert alert{

                AlertLevel::WARNING,

                "Memory",

                "Memory usage high",

                ::std::chrono::system_clock::now(),

                static_cast<double>(memoryMB),

                static_cast<double>(_alertThresholds.memoryWarningMB)

            };

            TriggerAlert(alert);
        }

        // Check query time
    if (database.avgQueryTimeMs >= _alertThresholds.queryTimeCriticalMs)
        {

            PerformanceAlert alert{

                AlertLevel::CRITICAL,

                "Database",

                "Database query time critical",

                ::std::chrono::system_clock::now(),

                database.avgQueryTimeMs,

                _alertThresholds.queryTimeCriticalMs

            };

            TriggerAlert(alert);
        }
        else if (database.avgQueryTimeMs >= _alertThresholds.queryTimeWarningMs)
        {

            PerformanceAlert alert{

                AlertLevel::WARNING,

                "Database",

                "Database query time high",

                ::std::chrono::system_clock::now(),

                database.avgQueryTimeMs,

                _alertThresholds.queryTimeWarningMs

            };

            TriggerAlert(alert);
        }
    }

    void BotMonitor::TriggerAlert(PerformanceAlert const& alert)
    {
        _alertHistory.push_back(alert);

        // Keep only MAX_ALERT_HISTORY alerts
    if (_alertHistory.size() > MAX_ALERT_HISTORY)

            _alertHistory.pop_front();

        // Trigger callbacks
    for (auto const& callback : _alertCallbacks)
        {

            try

            {

                callback(alert);

            }

            catch (::std::exception const& ex)

            {

                TC_LOG_ERROR("playerbot", "BotMonitor: Alert callback exception: %s", ex.what());

            }
        }

        // Log alert
        char const* levelStr = "";
        switch (alert.level)
        {

            case AlertLevel::INFO:
            levelStr = "INFO"; break;

            case AlertLevel::WARNING:  levelStr = "WARNING"; break;

            case AlertLevel::CRITICAL: levelStr = "CRITICAL"; break;

            default:
            levelStr = "UNKNOWN"; break;
        }

        TC_LOG_WARN("playerbot", "BotMonitor ALERT [%s] %s: %s (current: %.2f, threshold: %.2f)",

                    levelStr, alert.category.c_str(), alert.message.c_str(),

                    alert.currentValue, alert.thresholdValue);
    }

    // =====================================================================
    // HELPER METHODS
    // =====================================================================

    BotActivityMetrics BotMonitor::CollectActivityMetrics() const
    {
        BotActivityMetrics metrics;

        metrics.combatCount = _botsInCombat.size();
        metrics.questingCount = _botsQuesting.size();
        metrics.deadCount = _botsDead.size();

        // Count traveling and idle bots
    for (auto const& [guid, state] : _botActivityState)
        {

            if (state == "traveling")

                metrics.travelingCount++;

            else if (state == "idle")

                metrics.idleCount++;
        }

        metrics.totalActive = _botActivityState.size();

        return metrics;
    }

    SystemResourceMetrics BotMonitor::CollectResourceMetrics() const
    {
        SystemResourceMetrics metrics;

        metrics.cpuUsagePercent = CalculateCpuUsage();
        metrics.memoryUsedBytes = CalculateMemoryUsage();
        metrics.threadCount = GetActiveThreadCount();
        metrics.networkThroughputMbps = CalculateNetworkThroughput();

        BotActivityMetrics activity = CollectActivityMetrics();
        if (activity.totalActive > 0)
        {

            metrics.cpuPerBotPercent = metrics.cpuUsagePercent / activity.totalActive;

            metrics.memoryPerBotBytes = metrics.memoryUsedBytes / activity.totalActive;
        }

        return metrics;
    }

    DatabaseMetrics BotMonitor::CollectDatabaseMetrics() const
    {
        DatabaseMetrics metrics;

        metrics.queryCount = _totalQueries;
        metrics.cacheHits = _totalCacheHits;
        metrics.cacheMisses = _totalCacheMisses;

        // Calculate queries per second
        uint64 uptime = GetUptimeSeconds();
        if (uptime > 0)

            metrics.queriesPerSecond = _totalQueries / uptime;

        // Calculate average query time
    if (!_queryTimes.empty())
        {

            double sum = ::std::accumulate(_queryTimes.begin(), _queryTimes.end(), 0.0);

            metrics.avgQueryTimeMs = sum / _queryTimes.size();
        }

        metrics.maxQueryTimeMs = _maxQueryTime;

        // Full database pool monitoring with connection health checks
        // Returns static placeholder values until TrinityCore API integration is implemented
        // Full implementation should:
        // - Access DatabaseWorkerPool::GetActiveConnectionCount() for real-time connection count
        // - Query DatabaseWorkerPool::GetPoolSize() for configured pool size
        // - Monitor connection queue depth for performance analysis
        // - Track connection wait times and timeouts
        // - Integrate with sWorld->GetPlayerbotDatabase()->GetConnectionInfo()
        // Reference: DatabaseWorkerPool API, worldserver.conf DatabaseWorkerPool settings
        metrics.activeConnections = 0;
        metrics.connectionPoolSize = 50;

        return metrics;
    }

    double BotMonitor::CalculateCpuUsage() const
    {
#ifdef _WIN32
        FILETIME idleTime, kernelTime, userTime;
        if (GetSystemTimes(&idleTime, &kernelTime, &userTime))
        {

            static ULARGE_INTEGER lastKernel = {0}, lastUser = {0}, lastIdle = {0};

            ULARGE_INTEGER kernel, user, idle;


            kernel.LowPart = kernelTime.dwLowDateTime;

            kernel.HighPart = kernelTime.dwHighDateTime;

            user.LowPart = userTime.dwLowDateTime;

            user.HighPart = userTime.dwHighDateTime;

            idle.LowPart = idleTime.dwLowDateTime;

            idle.HighPart = idleTime.dwHighDateTime;


            if (lastKernel.QuadPart != 0)

            {

                uint64 kernelDiff = kernel.QuadPart - lastKernel.QuadPart;

                uint64 userDiff = user.QuadPart - lastUser.QuadPart;

                uint64 idleDiff = idle.QuadPart - lastIdle.QuadPart;


                uint64 totalDiff = kernelDiff + userDiff;

                if (totalDiff > 0)

                {

                    double cpuUsage = 100.0 * (1.0 - (static_cast<double>(idleDiff) / totalDiff));

                    lastKernel = kernel;

                    lastUser = user;

                    lastIdle = idle;

                    return cpuUsage;

                }

            }


            lastKernel = kernel;

            lastUser = user;

            lastIdle = idle;
        }
#else
        // Linux/Unix implementation using /proc/stat
        // This is a simplified version - production would need full implementation
        static long lastTotal = 0, lastIdle = 0;

        ::std::ifstream file("/proc/stat");
        if (file.is_open())
        {

            ::std::string cpu;

            long user, nice, system, idle;

            file >> cpu >> user >> nice >> system >> idle;


            long total = user + nice + system + idle;


            if (lastTotal != 0)

            {

                long totalDiff = total - lastTotal;

                long idleDiff = idle - lastIdle;


                if (totalDiff > 0)

                {

                    double cpuUsage = 100.0 * (1.0 - (static_cast<double>(idleDiff) / totalDiff));

                    lastTotal = total;

                    lastIdle = idle;

                    return cpuUsage;

                }

            }


            lastTotal = total;

            lastIdle = idle;
        }
#endif

        return _lastCpuUsage;  // Return last known value if calculation fails
    }

    uint64 BotMonitor::CalculateMemoryUsage() const
    {
#ifdef _WIN32
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))

            return pmc.WorkingSetSize;
#else
        struct rusage usage;
        if (getrusage(RUSAGE_SELF, &usage) == 0)

            return usage.ru_maxrss * 1024;  // Convert KB to bytes
#endif

        return 0;
    }

    uint32 BotMonitor::GetActiveThreadCount() const
    {
#ifdef _WIN32
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        return sysInfo.dwNumberOfProcessors;
#else
        return sysconf(_SC_NPROCESSORS_ONLN);
#endif
    }

    double BotMonitor::CalculateNetworkThroughput() const
    {
        // Full network metrics collection with per-bot bandwidth tracking
        // Returns 0.0 as default until network statistics integration is implemented
        // Full implementation should:
        // - Access WorldSocket::GetBytesReceived() and GetBytesSent() per bot session
        // - Aggregate network I/O across all active bot WorldSessions
        // - Calculate throughput in Mbps using delta measurements
        // - Track packet rate and average packet size
        // - Monitor network latency per bot connection
        // Reference: WorldSocket, WorldSession network statistics, sWorld->GetNetworkStats()
        return 0.0;
    }

} // namespace Playerbot
