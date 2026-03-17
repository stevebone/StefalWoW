/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include <chrono>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <condition_variable>
#include <queue>

namespace Playerbot
{

// Performance metric types
enum class MetricType : uint8
{
    AI_DECISION_TIME        = 0,  // Microseconds for AI decision making
    MEMORY_USAGE           = 1,  // Bytes of memory used
    DATABASE_QUERY_TIME    = 2,  // Microseconds for database operations
    SPELL_CAST_TIME        = 3,  // Microseconds for spell casting decisions
    MOVEMENT_CALCULATION   = 4,  // Microseconds for movement calculations
    COMBAT_ROTATION_TIME   = 5,  // Microseconds for combat rotation
    SPECIALIZATION_UPDATE  = 6,  // Microseconds for specialization updates
    RESOURCE_MANAGEMENT    = 7,  // Microseconds for resource management
    TARGET_SELECTION       = 8,  // Microseconds for target selection
    COOLDOWN_MANAGEMENT    = 9   // Microseconds for cooldown management
};

// Performance alert levels
enum class AlertLevel : uint8
{
    INFO     = 0,
    WARNING  = 1,
    CRITICAL = 2,
    EMERGENCY = 3
};

// Individual performance metric
struct PerformanceMetric
{
    MetricType type;
    uint64_t value;              // Microseconds or bytes
    uint64_t timestamp;          // Microseconds since epoch
    uint32_t botGuid;
    ::std::string context;         // Additional context information

    PerformanceMetric() : type(MetricType::AI_DECISION_TIME), value(0), timestamp(0), botGuid(0) {}
    PerformanceMetric(MetricType t, uint64_t v, uint32_t guid, ::std::string ctx = "")
        : type(t), value(v), timestamp(GetMicrosecondTimestamp()), botGuid(guid), context(::std::move(ctx)) {}

private:
    static uint64_t GetMicrosecondTimestamp()
    {
        return ::std::chrono::duration_cast<::std::chrono::microseconds>(
            ::std::chrono::steady_clock::now().time_since_epoch()).count();
    }
};

// Aggregated statistics for a metric type
struct MetricStatistics
{
    ::std::atomic<uint64_t> totalSamples{0};
    ::std::atomic<uint64_t> totalValue{0};
    ::std::atomic<uint64_t> minValue{UINT64_MAX};
    ::std::atomic<uint64_t> maxValue{0};
    ::std::atomic<uint64_t> lastValue{0};
    ::std::atomic<uint64_t> lastUpdate{0};

    // Percentile tracking
    ::std::atomic<uint64_t> p50{0};  // Median
    ::std::atomic<uint64_t> p95{0};  // 95th percentile
    ::std::atomic<uint64_t> p99{0};  // 99th percentile

    void Update(uint64_t value);
    double GetAverage() const;
    void Reset();
};

// Performance alert
struct PerformanceAlert
{
    AlertLevel level;
    MetricType metricType;
    uint32_t botGuid;
    uint64_t value;
    uint64_t threshold;
    uint64_t timestamp;
    ::std::string message;
    ::std::string stackTrace;

    PerformanceAlert(AlertLevel lvl, MetricType type, uint32_t guid, uint64_t val, uint64_t thresh, ::std::string msg)
        : level(lvl), metricType(type), botGuid(guid), value(val), threshold(thresh), message(::std::move(msg))
    {
        timestamp = ::std::chrono::duration_cast<::std::chrono::microseconds>(
            ::std::chrono::steady_clock::now().time_since_epoch()).count();
    }
};

// High-resolution timer for performance measurements
class TC_GAME_API PerformanceTimer
{
public:
    PerformanceTimer() : _startTime(::std::chrono::steady_clock::now()) {}

    void Reset() { _startTime = ::std::chrono::steady_clock::now(); }

    uint64_t GetElapsedMicroseconds() const
    {
        auto now = ::std::chrono::steady_clock::now();
        return ::std::chrono::duration_cast<::std::chrono::microseconds>(now - _startTime).count();
    }

    uint64_t GetElapsedNanoseconds() const
    {
        auto now = ::std::chrono::steady_clock::now();
        return ::std::chrono::duration_cast<::std::chrono::nanoseconds>(now - _startTime).count();
    }

private:
    ::std::chrono::steady_clock::time_point _startTime;
};

// RAII Performance measurement helper
class TC_GAME_API ScopedPerformanceMeasurement
{
public:
    ScopedPerformanceMeasurement(MetricType type, uint32_t botGuid, ::std::string context = "");
    ~ScopedPerformanceMeasurement();

    // Disable copy/move
    ScopedPerformanceMeasurement(const ScopedPerformanceMeasurement&) = delete;
    ScopedPerformanceMeasurement& operator=(const ScopedPerformanceMeasurement&) = delete;
    ScopedPerformanceMeasurement(ScopedPerformanceMeasurement&&) = delete;
    ScopedPerformanceMeasurement& operator=(ScopedPerformanceMeasurement&&) = delete;

private:
    MetricType _type;
    uint32_t _botGuid;
    ::std::string _context;
    PerformanceTimer _timer;
};

// Main performance monitoring system
class TC_GAME_API BotPerformanceMonitor
{
public:
    static BotPerformanceMonitor& Instance()
    {
        static BotPerformanceMonitor instance;
        return instance;
    }

    // Initialization and shutdown
    bool Initialize();
    void Shutdown();
    bool IsEnabled() const { return _enabled.load(); }

    // Metric recording
    void RecordMetric(const PerformanceMetric& metric);
    void RecordMetric(MetricType type, uint64_t value, uint32_t botGuid, const ::std::string& context = "");

    // Specialized recording methods
    void RecordAIDecisionTime(uint32_t botGuid, uint64_t microseconds, const ::std::string& context = "");
    void RecordMemoryUsage(uint32_t botGuid, uint64_t bytes, const ::std::string& context = "");
    void RecordDatabaseQueryTime(uint32_t botGuid, uint64_t microseconds, const ::std::string& query = "");
    void RecordSpellCastTime(uint32_t botGuid, uint64_t microseconds, uint32_t spellId);
    void RecordMovementCalculation(uint32_t botGuid, uint64_t microseconds, const ::std::string& context = "");

    // Statistics retrieval
    MetricStatistics GetStatistics(MetricType type) const;
    MetricStatistics GetBotStatistics(uint32_t botGuid, MetricType type) const;
    ::std::vector<PerformanceAlert> GetRecentAlerts(uint32_t maxCount = 100) const;

    // Performance analysis
    bool IsPerformanceAcceptable(MetricType type, uint64_t value) const;
    void CheckPerformanceThresholds(const PerformanceMetric& metric);
    void GeneratePerformanceReport(::std::string& report) const;

    // Configuration
    void SetThreshold(MetricType type, AlertLevel level, uint64_t threshold);
    uint64_t GetThreshold(MetricType type, AlertLevel level) const;
    void SetEnabled(bool enabled) { _enabled.store(enabled); }

    // Bot lifecycle
    void RegisterBot(uint32_t botGuid);
    void UnregisterBot(uint32_t botGuid);
    void ClearBotMetrics(uint32_t botGuid);

    // System health
    uint32_t GetActiveBotsCount() const;
    uint64_t GetTotalMemoryUsage() const;
    double GetSystemCpuUsage() const;
    void UpdateSystemMetrics();

    // Data management
    void FlushMetrics();
    void ArchiveOldMetrics(uint64_t olderThanMicroseconds);
    void ExportMetrics(const ::std::string& filename) const;

private:
    BotPerformanceMonitor() = default;
    ~BotPerformanceMonitor() = default;

    // Internal metric processing
    void ProcessMetrics();
    void ProcessAlertsQueue();
    void UpdateStatistics(const PerformanceMetric& metric);
    void CalculatePercentiles(MetricType type);

    // Alert generation
    void GenerateAlert(AlertLevel level, MetricType type, uint32_t botGuid,
                      uint64_t value, uint64_t threshold, const ::std::string& message);

    // Default thresholds initialization
    void InitializeDefaultThresholds();

    // Worker thread management
    void StartWorkerThreads();
    void StopWorkerThreads();

    // Configuration (loaded from playerbots.conf)
    bool _configEnabled{false};              // Playerbot.Performance.EnableMonitoring
    float _cpuWarningThreshold{70.0f};       // Playerbot.Performance.CpuWarningThreshold
    float _memoryWarningThreshold{80.0f};    // Playerbot.Performance.MemoryWarningThreshold
    uint32 _reportIntervalSeconds{60};       // Playerbot.Performance.ReportInterval
    bool _enableDetailedTracking{false};     // Playerbot.Performance.DetailedTracking
    ::std::string _metricsExportPath;        // Playerbot.Performance.ExportPath

    // Runtime state
    ::std::atomic<bool> _enabled{false};
    ::std::atomic<bool> _shutdownRequested{false};

    // Metrics storage
    mutable ::std::recursive_mutex _metricsMutex;
    ::std::queue<PerformanceMetric> _metricsQueue;
    ::std::unordered_map<MetricType, MetricStatistics> _globalStatistics;
    ::std::unordered_map<uint32_t, ::std::unordered_map<MetricType, MetricStatistics>> _botStatistics;

    // Alerts
    mutable ::std::recursive_mutex _alertsMutex;
    ::std::queue<PerformanceAlert> _alertsQueue;
    ::std::vector<PerformanceAlert> _recentAlerts;
    static constexpr size_t MAX_RECENT_ALERTS = 1000;

    // Thresholds (microseconds or bytes)
    ::std::unordered_map<MetricType, ::std::unordered_map<AlertLevel, uint64_t>> _thresholds;

    // Bot tracking
    mutable ::std::recursive_mutex _botsMutex;
    ::std::unordered_set<uint32_t> _registeredBots;
    ::std::unordered_map<uint32_t, uint64_t> _botMemoryUsage;

    // Worker threads
    ::std::vector<::std::thread> _workerThreads;
    ::std::condition_variable _metricsCondition;
    ::std::condition_variable _alertsCondition;

    // System metrics
    mutable ::std::recursive_mutex _systemMetricsMutex;
    ::std::atomic<uint64_t> _totalSystemMemory{0};
    ::std::atomic<double> _systemCpuUsage{0.0};
    ::std::atomic<uint64_t> _lastSystemUpdate{0};

    // Performance constants
    static constexpr uint64_t DEFAULT_AI_DECISION_WARNING_US = 50000;      // 50ms
    static constexpr uint64_t DEFAULT_AI_DECISION_CRITICAL_US = 100000;    // 100ms
    static constexpr uint64_t DEFAULT_DATABASE_WARNING_US = 10000;         // 10ms
    static constexpr uint64_t DEFAULT_DATABASE_CRITICAL_US = 50000;        // 50ms
    static constexpr uint64_t DEFAULT_MEMORY_WARNING_BYTES = 10485760;     // 10MB
    static constexpr uint64_t DEFAULT_MEMORY_CRITICAL_BYTES = 52428800;    // 50MB
    static constexpr uint64_t METRICS_FLUSH_INTERVAL_US = 1000000;         // 1 second
    static constexpr uint64_t SYSTEM_METRICS_UPDATE_INTERVAL_US = 5000000; // 5 seconds
};

// Convenience macros for performance measurement
#define MEASURE_PERFORMANCE(type, botGuid, context) \
    ScopedPerformanceMeasurement _perf_measurement(type, botGuid, context)

#define MEASURE_AI_DECISION(botGuid) \
    MEASURE_PERFORMANCE(MetricType::AI_DECISION_TIME, botGuid, __FUNCTION__)

#define MEASURE_DATABASE_QUERY(botGuid, query) \
    MEASURE_PERFORMANCE(MetricType::DATABASE_QUERY_TIME, botGuid, query)

#define MEASURE_SPELL_CAST(botGuid) \
    MEASURE_PERFORMANCE(MetricType::SPELL_CAST_TIME, botGuid, __FUNCTION__)

#define MEASURE_MOVEMENT_CALC(botGuid) \
    MEASURE_PERFORMANCE(MetricType::MOVEMENT_CALCULATION, botGuid, __FUNCTION__)

// Performance monitoring singleton access
#define sPerfMonitor BotPerformanceMonitor::Instance()

} // namespace Playerbot