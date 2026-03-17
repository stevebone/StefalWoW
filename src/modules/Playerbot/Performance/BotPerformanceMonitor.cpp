/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BotPerformanceMonitor.h"
#include "Log.h"
#include "Util.h"
#include "../Config/PlayerbotConfig.h"
#include <algorithm>
#include <sstream>
#include <fstream>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#include <unistd.h>
#include <fstream>
#endif

namespace Playerbot
{

// MetricStatistics Implementation
void MetricStatistics::Update(uint64_t value)
{
    totalSamples.fetch_add(1);
    totalValue.fetch_add(value);
    lastValue.store(value);
    lastUpdate.store(::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now().time_since_epoch()).count());

    // Update min/max
    uint64_t currentMin = minValue.load();
    while (value < currentMin && !minValue.compare_exchange_weak(currentMin, value)) {}

    uint64_t currentMax = maxValue.load();
    while (value > currentMax && !maxValue.compare_exchange_weak(currentMax, value)) {}
}

double MetricStatistics::GetAverage() const
{
    uint64_t samples = totalSamples.load();
    return samples > 0 ? static_cast<double>(totalValue.load()) / samples : 0.0;
}

void MetricStatistics::Reset()
{
    totalSamples.store(0);
    totalValue.store(0);
    minValue.store(UINT64_MAX);
    maxValue.store(0);
    lastValue.store(0);
    lastUpdate.store(0);
    p50.store(0);
    p95.store(0);
    p99.store(0);
}

// ScopedPerformanceMeasurement Implementation
ScopedPerformanceMeasurement::ScopedPerformanceMeasurement(MetricType type, uint32_t botGuid, ::std::string context)
    : _type(type), _botGuid(botGuid), _context(::std::move(context))
{
    _timer.Reset();
}

ScopedPerformanceMeasurement::~ScopedPerformanceMeasurement()
{
    if (sPerfMonitor.IsEnabled())
    {
        uint64_t elapsed = _timer.GetElapsedMicroseconds();
        sPerfMonitor.RecordMetric(_type, elapsed, _botGuid, _context);
    }
}

// BotPerformanceMonitor Implementation
bool BotPerformanceMonitor::Initialize()
{
    TC_LOG_INFO("playerbot", "Initializing Bot Performance Monitor...");

    // Load configuration from PlayerbotConfig
    _configEnabled = sPlayerbotConfig->GetBool("Playerbot.Performance.EnableMonitoring", false);
    _cpuWarningThreshold = sPlayerbotConfig->GetFloat("Playerbot.Performance.CpuWarningThreshold", 70.0f);
    _memoryWarningThreshold = sPlayerbotConfig->GetFloat("Playerbot.Performance.MemoryWarningThreshold", 80.0f);
    _reportIntervalSeconds = sPlayerbotConfig->GetUInt("Playerbot.Performance.ReportInterval", 60);
    _enableDetailedTracking = sPlayerbotConfig->GetBool("Playerbot.Performance.DetailedTracking", false);
    _metricsExportPath = sPlayerbotConfig->GetString("Playerbot.Performance.ExportPath", "");

    TC_LOG_DEBUG("playerbot", "Performance Monitor Config: Enable={}, CpuWarn={}%, MemWarn={}%, Interval={}s, Detailed={}",
        _configEnabled, _cpuWarningThreshold, _memoryWarningThreshold, _reportIntervalSeconds, _enableDetailedTracking);

    if (!_configEnabled)
    {
        TC_LOG_INFO("playerbot", "Bot Performance Monitor disabled via config");
        return true;
    }

    // Initialize default thresholds
    InitializeDefaultThresholds();

    // Start worker threads
    StartWorkerThreads();

    _enabled.store(true);

    TC_LOG_INFO("playerbot", "Bot Performance Monitor initialized successfully");
    return true;
}

void BotPerformanceMonitor::Shutdown()
{
    TC_LOG_INFO("playerbot", "Shutting down Bot Performance Monitor...");

    _enabled.store(false);
    _shutdownRequested.store(true);

    // Stop worker threads
    StopWorkerThreads();

    // Flush remaining metrics
    FlushMetrics();

    TC_LOG_INFO("playerbot", "Bot Performance Monitor shut down successfully");
}

void BotPerformanceMonitor::RecordMetric(const PerformanceMetric& metric)
{
    if (!_enabled.load())
        return;

    {
        ::std::lock_guard lock(_metricsMutex);
        _metricsQueue.push(metric);
    }
    _metricsCondition.notify_one();

    // Check thresholds for immediate alerts
    CheckPerformanceThresholds(metric);
}

void BotPerformanceMonitor::RecordMetric(MetricType type, uint64_t value, uint32_t botGuid, const ::std::string& context)
{
    RecordMetric(PerformanceMetric(type, value, botGuid, context));
}

void BotPerformanceMonitor::RecordAIDecisionTime(uint32_t botGuid, uint64_t microseconds, const ::std::string& context)
{
    RecordMetric(MetricType::AI_DECISION_TIME, microseconds, botGuid, context);
}

void BotPerformanceMonitor::RecordMemoryUsage(uint32_t botGuid, uint64_t bytes, const ::std::string& context)
{
    RecordMetric(MetricType::MEMORY_USAGE, bytes, botGuid, context);

    // Update bot memory tracking
    {
        ::std::lock_guard lock(_botsMutex);
        _botMemoryUsage[botGuid] = bytes;
    }
}

void BotPerformanceMonitor::RecordDatabaseQueryTime(uint32_t botGuid, uint64_t microseconds, const ::std::string& query)
{
    RecordMetric(MetricType::DATABASE_QUERY_TIME, microseconds, botGuid, query);
}

void BotPerformanceMonitor::RecordSpellCastTime(uint32_t botGuid, uint64_t microseconds, uint32_t spellId)
{
    ::std::string context;
    context.reserve(32); // Pre-allocate reasonable size
    context = "SpellId: ";
    context += ::std::to_string(spellId);
    RecordMetric(MetricType::SPELL_CAST_TIME, microseconds, botGuid, context);
}

void BotPerformanceMonitor::RecordMovementCalculation(uint32_t botGuid, uint64_t microseconds, const ::std::string& context)
{
    RecordMetric(MetricType::MOVEMENT_CALCULATION, microseconds, botGuid, context);
}

MetricStatistics BotPerformanceMonitor::GetStatistics(MetricType type) const
{
    ::std::lock_guard lock(_metricsMutex);
    auto it = _globalStatistics.find(type);
    return it != _globalStatistics.end() ? it->second : MetricStatistics();
}

MetricStatistics BotPerformanceMonitor::GetBotStatistics(uint32_t botGuid, MetricType type) const
{
    ::std::lock_guard lock(_metricsMutex);
    auto botIt = _botStatistics.find(botGuid);
    if (botIt != _botStatistics.end())
    {
        auto typeIt = botIt->second.find(type);
        if (typeIt != botIt->second.end())
            return typeIt->second;
    }
    return MetricStatistics();
}

::std::vector<PerformanceAlert> BotPerformanceMonitor::GetRecentAlerts(uint32_t maxCount) const
{
    ::std::lock_guard lock(_alertsMutex);
    ::std::vector<PerformanceAlert> alerts;

    size_t count = ::std::min(static_cast<size_t>(maxCount), _recentAlerts.size());
    if (count > 0)
    {
        alerts.reserve(count);
        auto start = _recentAlerts.end() - count;
        alerts.assign(start, _recentAlerts.end());
    }

    return alerts;
}

bool BotPerformanceMonitor::IsPerformanceAcceptable(MetricType type, uint64_t value) const
{
    auto typeIt = _thresholds.find(type);
    if (typeIt != _thresholds.end())
    {
        auto criticalIt = typeIt->second.find(AlertLevel::CRITICAL);
        if (criticalIt != typeIt->second.end())
        {
            return value < criticalIt->second;
        }
    }
    return true; // No threshold defined, assume acceptable
}

void BotPerformanceMonitor::CheckPerformanceThresholds(const PerformanceMetric& metric)
{
    auto typeIt = _thresholds.find(metric.type);
    if (typeIt == _thresholds.end())
        return;

    const auto& thresholds = typeIt->second;

    // Check thresholds in order of severity
    for (auto level : {AlertLevel::EMERGENCY, AlertLevel::CRITICAL, AlertLevel::WARNING})
    {
        auto thresholdIt = thresholds.find(level);
        if (thresholdIt != thresholds.end() && metric.value >= thresholdIt->second)
        {
            ::std::string message;
            message.reserve(128); // Pre-allocate for efficiency
            message = "Performance threshold exceeded: ";
            message += GetMetricTypeName(metric.type);
            message += " = ";
            message += ::std::to_string(metric.value);
            if (!metric.context.empty())
            {
                message += " (";
                message += metric.context;
                message += ")";
            }

            GenerateAlert(level, metric.type, metric.botGuid, metric.value, thresholdIt->second, message);
            break; // Only generate the highest severity alert
        }
    }
}

void BotPerformanceMonitor::GeneratePerformanceReport(::std::string& report) const
{
    ::std::ostringstream oss;
    oss << "=== Bot Performance Monitor Report ===\n";
    oss << "Generated at: " << TimeToTimestampStr(time(nullptr)) << "\n\n";

    // System overview
    oss << "System Overview:\n";
    oss << "- Active Bots: " << GetActiveBotsCount() << "\n";
    oss << "- Total Memory Usage: " << GetTotalMemoryUsage() / (1024 * 1024) << " MB\n";
    oss << "- System CPU Usage: " << ::std::fixed << ::std::setprecision(2) << GetSystemCpuUsage() << "%\n\n";

    // Metric statistics
    oss << "Performance Metrics:\n";
    ::std::lock_guard lock(_metricsMutex);

    for (const auto& [type, stats] : _globalStatistics)
    {
        if (stats.totalSamples.load() > 0)
        {
            oss << "- " << GetMetricTypeName(type) << ":\n";
            oss << "  Samples: " << stats.totalSamples.load() << "\n";
            oss << "  Average: " << ::std::fixed << ::std::setprecision(2) << stats.GetAverage();

            if (type == MetricType::MEMORY_USAGE)
                oss << " bytes\n";
            else
                oss << " μs\n";

            oss << "  Min: " << stats.minValue.load();
            oss << ", Max: " << stats.maxValue.load();
            oss << ", Last: " << stats.lastValue.load() << "\n";
            oss << "  P95: " << stats.p95.load();
            oss << ", P99: " << stats.p99.load() << "\n\n";
        }
    }

    // Recent alerts
    {
        ::std::lock_guard alertLock(_alertsMutex);
        if (!_recentAlerts.empty())
        {
            oss << "Recent Alerts (" << ::std::min(size_t(10), _recentAlerts.size()) << " most recent):\n";
            size_t count = 0;
            for (auto it = _recentAlerts.rbegin(); it != _recentAlerts.rend() && count < 10; ++it, ++count)
            {
                oss << "- [" << GetAlertLevelName(it->level) << "] Bot " << it->botGuid
                    << ": " << it->message << "\n";
            }
        }
    }

    report = oss.str();
}

void BotPerformanceMonitor::SetThreshold(MetricType type, AlertLevel level, uint64_t threshold)
{
    _thresholds[type][level] = threshold;
}

uint64_t BotPerformanceMonitor::GetThreshold(MetricType type, AlertLevel level) const
{
    auto typeIt = _thresholds.find(type);
    if (typeIt != _thresholds.end())
    {
        auto levelIt = typeIt->second.find(level);
        if (levelIt != typeIt->second.end())
            return levelIt->second;
    }
    return 0;
}

void BotPerformanceMonitor::RegisterBot(uint32_t botGuid)
{
    ::std::lock_guard lock(_botsMutex);
    _registeredBots.insert(botGuid);
    _botMemoryUsage[botGuid] = 0;

    TC_LOG_DEBUG("playerbot", "Registered bot {} for performance monitoring", botGuid);
}

void BotPerformanceMonitor::UnregisterBot(uint32_t botGuid)
{
    {
        ::std::lock_guard lock(_botsMutex);
        _registeredBots.erase(botGuid);
        _botMemoryUsage.erase(botGuid);
    }

    ClearBotMetrics(botGuid);

    TC_LOG_DEBUG("playerbot", "Unregistered bot {} from performance monitoring", botGuid);
}

void BotPerformanceMonitor::ClearBotMetrics(uint32_t botGuid)
{
    ::std::lock_guard lock(_metricsMutex);
    _botStatistics.erase(botGuid);
}

uint32_t BotPerformanceMonitor::GetActiveBotsCount() const
{
    ::std::lock_guard lock(_botsMutex);
    return static_cast<uint32_t>(_registeredBots.size());
}

uint64_t BotPerformanceMonitor::GetTotalMemoryUsage() const
{
    ::std::lock_guard lock(_botsMutex);
    uint64_t total = 0;
    for (const auto& [botGuid, memory] : _botMemoryUsage)
        total += memory;
    return total;
}

double BotPerformanceMonitor::GetSystemCpuUsage() const
{
    return _systemCpuUsage.load();
}

void BotPerformanceMonitor::UpdateSystemMetrics()
{
    uint64_t now = ::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now().time_since_epoch()).count();

    if (now - _lastSystemUpdate.load() < SYSTEM_METRICS_UPDATE_INTERVAL_US)
        return;

    // Update CPU usage (platform-specific implementation)
#ifdef _WIN32
    // Windows implementation
    static ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
    static int numProcessors = -1;
    static HANDLE self = GetCurrentProcess();

    if (numProcessors == -1)
    {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        numProcessors = sysInfo.dwNumberOfProcessors;
    }

    ULARGE_INTEGER now, sys, user;
    FILETIME ftime, fsys, fuser;

    GetSystemTimeAsFileTime(&ftime);
    memcpy(&now, &ftime, sizeof(FILETIME));

    GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
    memcpy(&sys, &fsys, sizeof(FILETIME));
    memcpy(&user, &fuser, sizeof(FILETIME));

    double percent = 0.0;
    if (lastCPU.QuadPart != 0)
    {
        percent = (sys.QuadPart - lastSysCPU.QuadPart) + (user.QuadPart - lastUserCPU.QuadPart);
        percent /= (now.QuadPart - lastCPU.QuadPart);
        percent /= numProcessors;
        percent *= 100;
    }

    lastCPU = now;
    lastUserCPU = user;
    lastSysCPU = sys;

    _systemCpuUsage.store(percent);
#else
    // Linux implementation
    static clock_t lastCPU = 0;
    static clock_t lastSysCPU = 0;
    static clock_t lastUserCPU = 0;
    static int numProcessors = sysconf(_SC_NPROCESSORS_ONLN);

    struct rusage rusage;
    if (getrusage(RUSAGE_SELF, &rusage) == 0)
    {
        clock_t now = clock();
        clock_t sys = rusage.ru_stime.tv_sec * CLOCKS_PER_SEC + rusage.ru_stime.tv_usec;
        clock_t user = rusage.ru_utime.tv_sec * CLOCKS_PER_SEC + rusage.ru_utime.tv_usec;

        double percent = 0.0;
        if (lastCPU != 0)
        {
            percent = (sys - lastSysCPU) + (user - lastUserCPU);
            percent /= (now - lastCPU);
            percent *= 100;
        }

        lastCPU = now;
        lastUserCPU = user;
        lastSysCPU = sys;

        _systemCpuUsage.store(percent);
    }
#endif

    _lastSystemUpdate.store(now);
}

void BotPerformanceMonitor::FlushMetrics()
{
    ::std::lock_guard lock(_metricsMutex);

    while (!_metricsQueue.empty())
    {
        const PerformanceMetric& metric = _metricsQueue.front();
        UpdateStatistics(metric);
        _metricsQueue.pop();
    }
}

void BotPerformanceMonitor::ArchiveOldMetrics(uint64_t olderThanMicroseconds)
{
    uint64_t cutoffTime = ::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now().time_since_epoch()).count() - olderThanMicroseconds;

    ::std::lock_guard lock(_alertsMutex);

    // Remove old alerts
    _recentAlerts.erase(
        ::std::remove_if(_recentAlerts.begin(), _recentAlerts.end(),
            [cutoffTime](const PerformanceAlert& alert) { return alert.timestamp < cutoffTime; }),
        _recentAlerts.end());
}

void BotPerformanceMonitor::ExportMetrics(const ::std::string& filename) const
{
    ::std::ofstream file(filename);
    if (!file.is_open())
    {
        TC_LOG_ERROR("playerbot", "Failed to open file {} for metrics export", filename);
        return;
    }

    file << "timestamp,botGuid,metricType,value,context\n";

    ::std::lock_guard lock(_metricsMutex);

    // Export current statistics
    for (const auto& [type, stats] : _globalStatistics)
    {
        if (stats.totalSamples.load() > 0)
        {
            file << stats.lastUpdate.load() << ",0," << static_cast<int>(type)
                 << "," << stats.lastValue.load() << ",global_stats\n";
        }
    }

    file.close();
    TC_LOG_INFO("playerbot", "Exported performance metrics to {}", filename);
}

void BotPerformanceMonitor::ProcessMetrics()
{
    while (!_shutdownRequested.load())
    {
        ::std::vector<PerformanceMetric> metricsToProcess;

        // Collect metrics while holding lock
        {
            ::std::unique_lock<::std::recursive_mutex> lock(_metricsMutex);
            _metricsCondition.wait(lock, [this] { return !_metricsQueue.empty() || _shutdownRequested.load(); });

            if (_shutdownRequested.load())
                break;

            // Collect up to 1000 metrics at once
    while (!_metricsQueue.empty() && metricsToProcess.size() < 1000)
            {
                metricsToProcess.push_back(_metricsQueue.front());
                _metricsQueue.pop();
            }
        } // Release lock before processing

        // Process metrics without holding queue lock (UpdateStatistics needs _metricsMutex)
    for (auto const& metric : metricsToProcess)
        {
            UpdateStatistics(metric);
        }
    }
}

void BotPerformanceMonitor::ProcessAlertsQueue()
{
    while (!_shutdownRequested.load())
    {
        ::std::unique_lock<::std::recursive_mutex> lock(_alertsMutex);
        _alertsCondition.wait(lock, [this] { return !_alertsQueue.empty() || _shutdownRequested.load(); });

        if (_shutdownRequested.load())
            break;

        while (!_alertsQueue.empty())
        {
            PerformanceAlert alert = _alertsQueue.front();
            _alertsQueue.pop();

            _recentAlerts.push_back(alert);

            // Keep only recent alerts
    if (_recentAlerts.size() > MAX_RECENT_ALERTS)
                _recentAlerts.erase(_recentAlerts.begin());

            // Log alert
            TC_LOG_WARN("playerbot", "Performance Alert [{}]: {}",
                       GetAlertLevelName(alert.level), alert.message);
        }
    }
}

void BotPerformanceMonitor::UpdateStatistics(const PerformanceMetric& metric)
{
    // Update global statistics
    _globalStatistics[metric.type].Update(metric.value);

    // Update bot-specific statistics
    _botStatistics[metric.botGuid][metric.type].Update(metric.value);
}

void BotPerformanceMonitor::CalculatePercentiles(MetricType type)
{
    // This is a simplified implementation
    // In a production system, you'd want to maintain a reservoir sample or histogram
    auto& stats = _globalStatistics[type];

    // For now, estimate percentiles based on current statistics
    uint64_t avg = static_cast<uint64_t>(stats.GetAverage());
    uint64_t range = stats.maxValue.load() - stats.minValue.load();

    stats.p50.store(avg); // Median approximation
    stats.p95.store(stats.minValue.load() + range * 95 / 100); // 95th percentile approximation
    stats.p99.store(stats.minValue.load() + range * 99 / 100); // 99th percentile approximation
}

void BotPerformanceMonitor::GenerateAlert(AlertLevel level, MetricType type, uint32_t botGuid,
                                        uint64_t value, uint64_t threshold, const ::std::string& message)
{
    {
        ::std::lock_guard lock(_alertsMutex);
        _alertsQueue.emplace(level, type, botGuid, value, threshold, message);
    }
    _alertsCondition.notify_one();
}

void BotPerformanceMonitor::InitializeDefaultThresholds()
{
    // AI Decision Time thresholds
    SetThreshold(MetricType::AI_DECISION_TIME, AlertLevel::WARNING, DEFAULT_AI_DECISION_WARNING_US);
    SetThreshold(MetricType::AI_DECISION_TIME, AlertLevel::CRITICAL, DEFAULT_AI_DECISION_CRITICAL_US);

    // Database Query Time thresholds
    SetThreshold(MetricType::DATABASE_QUERY_TIME, AlertLevel::WARNING, DEFAULT_DATABASE_WARNING_US);
    SetThreshold(MetricType::DATABASE_QUERY_TIME, AlertLevel::CRITICAL, DEFAULT_DATABASE_CRITICAL_US);

    // Memory Usage thresholds
    SetThreshold(MetricType::MEMORY_USAGE, AlertLevel::WARNING, DEFAULT_MEMORY_WARNING_BYTES);
    SetThreshold(MetricType::MEMORY_USAGE, AlertLevel::CRITICAL, DEFAULT_MEMORY_CRITICAL_BYTES);

    // Additional thresholds for other metric types
    SetThreshold(MetricType::SPELL_CAST_TIME, AlertLevel::WARNING, 10000);      // 10ms
    SetThreshold(MetricType::SPELL_CAST_TIME, AlertLevel::CRITICAL, 50000);     // 50ms

    SetThreshold(MetricType::MOVEMENT_CALCULATION, AlertLevel::WARNING, 5000);  // 5ms
    SetThreshold(MetricType::MOVEMENT_CALCULATION, AlertLevel::CRITICAL, 25000); // 25ms
}

void BotPerformanceMonitor::StartWorkerThreads()
{
    // Start metrics processing thread
    _workerThreads.emplace_back([this] { ProcessMetrics(); });

    // Start alerts processing thread
    _workerThreads.emplace_back([this] { ProcessAlertsQueue(); });
}

void BotPerformanceMonitor::StopWorkerThreads()
{
    _shutdownRequested.store(true);
    _metricsCondition.notify_all();
    _alertsCondition.notify_all();

    for (auto& thread : _workerThreads)
    {
        if (thread.joinable())
            thread.join();
    }

    _workerThreads.clear();
}

// Utility functions
::std::string GetMetricTypeName(MetricType type)
{
    switch (type)
    {
        case MetricType::AI_DECISION_TIME: return "AI Decision Time";
        case MetricType::MEMORY_USAGE: return "Memory Usage";
        case MetricType::DATABASE_QUERY_TIME: return "Database Query Time";
        case MetricType::SPELL_CAST_TIME: return "Spell Cast Time";
        case MetricType::MOVEMENT_CALCULATION: return "Movement Calculation";
        case MetricType::COMBAT_ROTATION_TIME: return "Combat Rotation Time";
        case MetricType::SPECIALIZATION_UPDATE: return "Specialization Update";
        case MetricType::RESOURCE_MANAGEMENT: return "Resource Management";
        case MetricType::TARGET_SELECTION: return "Target Selection";
        case MetricType::COOLDOWN_MANAGEMENT: return "Cooldown Management";
        default: return "Unknown";
    }
}

::std::string GetAlertLevelName(AlertLevel level)
{
    switch (level)
    {
        case AlertLevel::INFO: return "INFO";
        case AlertLevel::WARNING: return "WARNING";
        case AlertLevel::CRITICAL: return "CRITICAL";
        case AlertLevel::EMERGENCY: return "EMERGENCY";
        default: return "UNKNOWN";
    }
}

} // namespace Playerbot