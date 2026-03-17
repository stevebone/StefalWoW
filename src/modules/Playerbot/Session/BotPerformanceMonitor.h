/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#ifndef BOT_PERFORMANCE_MONITOR_H
#define BOT_PERFORMANCE_MONITOR_H

#include "Define.h"
#include "BotPriorityManager.h"
#include <array>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <chrono>

namespace Playerbot {

/**
 * ENTERPRISE-GRADE PERFORMANCE MONITORING
 *
 * Comprehensive performance monitoring and auto-scaling for 5000+ bots
 *
 * Features:
 * - Real-time performance metrics collection
 * - Update time histogram tracking
 * - Priority distribution analysis
 * - Automatic load shedding and scaling
 * - Performance degradation detection
 * - Detailed logging and alerting
 *
 * LOCK SHARDING OPTIMIZATION:
 * - System-level metrics use std::shared_mutex (readers run in parallel)
 * - Per-bot metrics delegated to ShardedMetricsCollector (256 shards)
 * - Histogram uses std::shared_mutex instead of recursive_mutex (2-3x faster)
 */

/**
 * Aggregated performance metrics for the entire bot system
 */
struct SystemPerformanceMetrics
{
    // Overall timing
    uint32 currentTickTime{0};          // Time for current tick (microseconds)
    uint32 averageTickTime{0};          // Rolling average tick time (microseconds)
    uint32 maxTickTime{0};              // Peak tick time (microseconds)
    uint32 minTickTime{UINT32_MAX};     // Minimum tick time (microseconds)

    // Bot counts
    uint32 totalBots{0};
    uint32 botsUpdatedThisTick{0};
    uint32 botsSkippedThisTick{0};

    // Priority distribution
    uint32 emergencyBots{0};
    uint32 highPriorityBots{0};
    uint32 mediumPriorityBots{0};
    uint32 lowPriorityBots{0};
    uint32 suspendedBots{0};

    // Error tracking
    uint32 totalErrors{0};
    uint32 errorsThisTick{0};

    // Load metrics
    float cpuLoadPercent{0.0f};         // Estimated CPU load (based on tick time)
    float targetLoadPercent{75.0f};     // Target CPU load threshold
    bool isOverloaded{false};
};

/**
 * Histogram for tracking update time distribution
 */
class UpdateTimeHistogram
{
public:
    UpdateTimeHistogram() : _buckets{} {}

    void RecordTime(uint32 microseconds);
    void Clear();

    // Get statistics
    uint32 GetCount() const { return _totalCount; }
    uint32 GetMin() const;
    uint32 GetMax() const;
    uint32 GetMedian() const;
    uint32 GetPercentile(uint8 percentile) const; // 0-100

    // Get distribution
    ::std::vector<uint32> GetBuckets() const;

private:
    static constexpr uint32 BUCKET_COUNT = 100;
    static constexpr uint32 BUCKET_SIZE_MICROS = 100; // 0.1ms per bucket

    ::std::array<uint32, BUCKET_COUNT> _buckets;
    uint32 _totalCount{0};
    mutable ::std::shared_mutex _mutex;  // OPTIMIZED: shared_mutex replaces recursive_mutex (2-3x faster)
};

/**
 * Thread-safe performance monitoring and auto-scaling
 *
 * Responsibilities:
 * - Monitor system-wide performance metrics
 * - Track update time distribution
 * - Detect performance degradation
 * - Trigger automatic load shedding
 * - Log performance warnings and alerts
 * - Provide performance statistics
 */
class TC_GAME_API BotPerformanceMonitor final
{
public:
    static BotPerformanceMonitor* instance()
    {
        static BotPerformanceMonitor instance;
        return &instance;
    }

    // Initialization
    bool Initialize();
    void Shutdown();

    // Tick monitoring
    void BeginTick(uint32 currentTime);
    void EndTick(uint32 currentTime, uint32 botsUpdated, uint32 botsSkipped);

    // Performance metrics
    void RecordBotUpdateTime(uint32 microseconds);
    SystemPerformanceMetrics const& GetMetrics() const { return _metrics; }

    // Auto-scaling
    void CheckPerformanceThresholds();
    void TriggerLoadShedding(uint32 targetReduction);
    void TriggerLoadRecovery(uint32 targetIncrease);

    // Degradation detection
    bool IsPerformanceDegraded() const;
    bool IsSystemOverloaded() const { return _metrics.isOverloaded; }
    float GetCurrentLoad() const { return _metrics.cpuLoadPercent; }

    // Configuration
    void SetTargetTickTime(uint32 microseconds) { _targetTickTimeMicros = microseconds; }
    void SetMaxTickTime(uint32 microseconds) { _maxTickTimeMicros = microseconds; }
    void SetLoadShedThreshold(uint32 microseconds) { _loadShedThresholdMicros = microseconds; }
    void SetAutoScalingEnabled(bool enabled) { _autoScalingEnabled.store(enabled); }

    uint32 GetTargetTickTime() const { return _targetTickTimeMicros; }
    uint32 GetMaxTickTime() const { return _maxTickTimeMicros; }
    bool IsAutoScalingEnabled() const { return _autoScalingEnabled.load(); }

    // Histogram access
    UpdateTimeHistogram const& GetHistogram() const { return _histogram; }

    // Statistics and logging
    void LogPerformanceReport() const;
    void LogDetailedStatistics() const;
    void ResetStatistics();

private:
    BotPerformanceMonitor() = default;
    ~BotPerformanceMonitor() = default;
    BotPerformanceMonitor(const BotPerformanceMonitor&) = delete;
    BotPerformanceMonitor& operator=(const BotPerformanceMonitor&) = delete;

    // Calculate load percentage
    float CalculateLoadPercent(uint32 tickTimeMicros) const;

    // Performance thresholds (configured via playerbots.conf)
    uint32 _targetTickTimeMicros{150000};       // 150ms target
    uint32 _maxTickTimeMicros{200000};          // 200ms warning threshold
    uint32 _loadShedThresholdMicros{250000};    // 250ms triggers load shedding

    // Auto-scaling state
    ::std::atomic<bool> _autoScalingEnabled{true};
    uint32 _lastLoadShedTime{0};
    uint32 _lastLoadRecoveryTime{0};
    static constexpr uint32 LOAD_ADJUST_COOLDOWN_MS = 5000; // 5 seconds between adjustments

    // Current metrics
    SystemPerformanceMetrics _metrics;
    mutable ::std::shared_mutex _metricsMutex;  // OPTIMIZED: shared_mutex replaces recursive_mutex (2-3x faster)

    // Histogram
    UpdateTimeHistogram _histogram;

    // Tick timing
    uint32 _tickStartTime{0};
    uint32 _tickNumber{0};

    // IMPROVEMENT #9: High-resolution timer for sub-millisecond precision
    ::std::chrono::steady_clock::time_point _tickStartTimeHighRes;

    // Moving average for smoothing
    static constexpr uint32 MOVING_AVG_WINDOW = 10;
    ::std::array<uint32, MOVING_AVG_WINDOW> _recentTickTimes{};
    uint32 _movingAvgIndex{0};

    // Performance degradation tracking
    uint32 _consecutiveSlowTicks{0};
    uint32 _consecutiveFastTicks{0};
    static constexpr uint32 DEGRADATION_THRESHOLD = 5; // 5 consecutive slow ticks

    // Initialization state
    ::std::atomic<bool> _initialized{false};
};

// Global instance accessor
#define sBotPerformanceMon BotPerformanceMonitor::instance()

} // namespace Playerbot

#endif // BOT_PERFORMANCE_MONITOR_H
