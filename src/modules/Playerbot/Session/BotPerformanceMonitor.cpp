/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#include "BotPerformanceMonitor.h"
#include "BotPriorityManager.h"
#include "Log.h"
#include "Timer.h"
#include "GameTime.h"
#include <algorithm>
#include <numeric>
#include <shared_mutex>

namespace Playerbot {

// ========================================
// UpdateTimeHistogram Implementation
// ========================================

void UpdateTimeHistogram::RecordTime(uint32 microseconds)
{
    ::std::unique_lock lock(_mutex);  // WRITE: exclusive lock

    uint32 bucket = microseconds / BUCKET_SIZE_MICROS;
    if (bucket >= BUCKET_COUNT)
        bucket = BUCKET_COUNT - 1; // Cap at last bucket

    _buckets[bucket]++;
    _totalCount++;
}

void UpdateTimeHistogram::Clear()
{
    ::std::unique_lock lock(_mutex);  // WRITE: exclusive lock
    _buckets.fill(0);
    _totalCount = 0;
}

uint32 UpdateTimeHistogram::GetMin() const
{
    ::std::shared_lock lock(_mutex);  // READ: shared lock (parallel readers)

    for (uint32 i = 0; i < BUCKET_COUNT; ++i)
    {
        if (_buckets[i] > 0)
            return i * BUCKET_SIZE_MICROS;
    }
    return 0;
}

uint32 UpdateTimeHistogram::GetMax() const
{
    ::std::shared_lock lock(_mutex);  // READ: shared lock (parallel readers)

    for (int32 i = BUCKET_COUNT - 1; i >= 0; --i)
    {
        if (_buckets[i] > 0)
            return i * BUCKET_SIZE_MICROS;
    }
    return 0;
}

uint32 UpdateTimeHistogram::GetMedian() const
{
    return GetPercentile(50);
}

uint32 UpdateTimeHistogram::GetPercentile(uint8 percentile) const
{
    if (percentile > 100)
        percentile = 100;

    ::std::shared_lock lock(_mutex);  // READ: shared lock (parallel readers)

    if (_totalCount == 0)
        return 0;

    uint32 targetCount = (_totalCount * percentile) / 100;
    uint32 cumulative = 0;

    for (uint32 i = 0; i < BUCKET_COUNT; ++i)
    {
        cumulative += _buckets[i];
        if (cumulative >= targetCount)
            return i * BUCKET_SIZE_MICROS;
    }

    return (BUCKET_COUNT - 1) * BUCKET_SIZE_MICROS;
}

::std::vector<uint32> UpdateTimeHistogram::GetBuckets() const
{
    ::std::shared_lock lock(_mutex);  // READ: shared lock (parallel readers)
    return ::std::vector<uint32>(_buckets.begin(), _buckets.end());
}

// ========================================
// BotPerformanceMonitor Implementation
// ========================================

bool BotPerformanceMonitor::Initialize()
{
    if (_initialized.load())
        return true;

    TC_LOG_INFO("module.playerbot", "BotPerformanceMonitor: Initializing enterprise performance monitoring...");

    // Reset all metrics
    _metrics = SystemPerformanceMetrics{};
    _histogram.Clear();
    _recentTickTimes.fill(0);
    _movingAvgIndex = 0;
    _tickNumber = 0;
    _consecutiveSlowTicks = 0;
    _consecutiveFastTicks = 0;

    _initialized.store(true);
    TC_LOG_INFO("module.playerbot", "BotPerformanceMonitor: Performance monitoring initialized successfully");
    TC_LOG_INFO("module.playerbot", "  Target tick time: {:.2f}ms", _targetTickTimeMicros / 1000.0f);
    TC_LOG_INFO("module.playerbot", "  Max tick time: {:.2f}ms", _maxTickTimeMicros / 1000.0f);
    TC_LOG_INFO("module.playerbot", "  Load shed threshold: {:.2f}ms", _loadShedThresholdMicros / 1000.0f);
    TC_LOG_INFO("module.playerbot", "  Auto-scaling: {}", _autoScalingEnabled.load() ? "ENABLED" : "DISABLED");

    return true;
}

void BotPerformanceMonitor::Shutdown()
{
    TC_LOG_INFO("module.playerbot", "BotPerformanceMonitor: Shutting down...");

    // Log final statistics before shutdown
    LogDetailedStatistics();

    ::std::unique_lock lock(_metricsMutex);  // WRITE: exclusive lock
    _metrics = SystemPerformanceMetrics{};
    _histogram.Clear();

    _initialized.store(false);
    TC_LOG_INFO("module.playerbot", "BotPerformanceMonitor: Shutdown complete");
}

void BotPerformanceMonitor::BeginTick(uint32 currentTime)
{
    _tickStartTime = currentTime;
    _tickNumber++;

    // IMPROVEMENT #9: High-resolution timer - Use steady_clock for microsecond precision
    // This allows us to see actual sub-millisecond performance (e.g., 0.45ms instead of 0.00ms)
    _tickStartTimeHighRes = ::std::chrono::steady_clock::now();

    ::std::unique_lock lock(_metricsMutex);  // WRITE: exclusive lock
    _metrics.errorsThisTick = 0;
    _metrics.botsUpdatedThisTick = 0;
    _metrics.botsSkippedThisTick = 0;
}

void BotPerformanceMonitor::EndTick(uint32 currentTime, uint32 botsUpdated, uint32 botsSkipped)
{
    // IMPROVEMENT #9: High-resolution timer - Calculate duration using steady_clock
    // This provides actual microsecond precision instead of millisecond rounding
    auto tickEndTimeHighRes = ::std::chrono::steady_clock::now();
    auto durationMicros = ::std::chrono::duration_cast<::std::chrono::microseconds>(
        tickEndTimeHighRes - _tickStartTimeHighRes
    );
    uint32 tickDuration = static_cast<uint32>(durationMicros.count());

    ::std::unique_lock lock(_metricsMutex);  // WRITE: exclusive lock

    // Update current tick metrics
    _metrics.currentTickTime = tickDuration;
    _metrics.botsUpdatedThisTick = botsUpdated;
    _metrics.botsSkippedThisTick = botsSkipped;

    // Update moving average
    _recentTickTimes[_movingAvgIndex] = tickDuration;
    _movingAvgIndex = (_movingAvgIndex + 1) % MOVING_AVG_WINDOW;

    // Calculate average from moving window
    uint64 sum = 0;
    uint32 count = 0;
    for (uint32 time : _recentTickTimes)
    {
        if (time > 0)
        {
            sum += time;
            count++;
        }
    }
    _metrics.averageTickTime = count > 0 ? static_cast<uint32>(sum / count) : tickDuration;

    // Update min/max
    if (tickDuration > _metrics.maxTickTime)
        _metrics.maxTickTime = tickDuration;
    if (tickDuration < _metrics.minTickTime)
        _metrics.minTickTime = tickDuration;

    // Update priority distribution
    sBotPriorityMgr->GetPriorityDistribution(
        _metrics.emergencyBots,
        _metrics.highPriorityBots,
        _metrics.mediumPriorityBots,
        _metrics.lowPriorityBots,
        _metrics.suspendedBots
    );

    _metrics.totalBots = _metrics.emergencyBots + _metrics.highPriorityBots +
                         _metrics.mediumPriorityBots + _metrics.lowPriorityBots +
                         _metrics.suspendedBots;

    // Calculate load
    _metrics.cpuLoadPercent = CalculateLoadPercent(tickDuration);
    _metrics.isOverloaded = tickDuration > _loadShedThresholdMicros;

    // Track performance trends
    if (tickDuration > _maxTickTimeMicros)
    {
        _consecutiveSlowTicks++;
        _consecutiveFastTicks = 0;
    }
    else if (tickDuration < _targetTickTimeMicros)
    {
        _consecutiveFastTicks++;
        _consecutiveSlowTicks = 0;
    }
    else
    {
        _consecutiveSlowTicks = 0;
        _consecutiveFastTicks = 0;
    }

    // Record to histogram
    _histogram.RecordTime(tickDuration);
}

void BotPerformanceMonitor::RecordBotUpdateTime(uint32 /*microseconds*/)
{
    // PERFORMANCE FIX: Skip histogram recording to reduce overhead
    // Histogram adds significant mutex contention with 778+ bots
    // _histogram.RecordTime(microseconds);
}

void BotPerformanceMonitor::CheckPerformanceThresholds()
{
    if (!_autoScalingEnabled.load())
        return;

    uint32 currentTime = GameTime::GetGameTimeMS();
    ::std::unique_lock lock(_metricsMutex);  // WRITE: may modify _lastLoadShedTime/Recovery

    // Check if we need load shedding
    if (_metrics.isOverloaded && _consecutiveSlowTicks >= DEGRADATION_THRESHOLD)
    {
        // Cooldown check
    if (currentTime - _lastLoadShedTime < LOAD_ADJUST_COOLDOWN_MS)
            return;

        // Calculate how many bots to suspend (10% of low-priority bots)
        uint32 targetReduction = _metrics.lowPriorityBots / 10;
        if (targetReduction > 0)
        {
            TC_LOG_WARN("module.playerbot.performance",
                "Performance degradation detected! Tick time: {:.2f}ms (target: {:.2f}ms). Triggering load shedding...",
                _metrics.currentTickTime / 1000.0f,
                _targetTickTimeMicros / 1000.0f);

            TriggerLoadShedding(targetReduction);
            _lastLoadShedTime = currentTime;
        }
    }
    // Check if we can recover suspended bots
    else if (!_metrics.isOverloaded && _consecutiveFastTicks >= DEGRADATION_THRESHOLD && _metrics.suspendedBots > 0)
    {
        // Cooldown check
    if (currentTime - _lastLoadRecoveryTime < LOAD_ADJUST_COOLDOWN_MS)
            return;

        // Resume 10% of suspended bots
        uint32 targetIncrease = ::std::max(1u, _metrics.suspendedBots / 10);

        TC_LOG_INFO("module.playerbot.performance",
            "Performance improved! Tick time: {:.2f}ms. Resuming {} suspended bots...",
            _metrics.currentTickTime / 1000.0f,
            targetIncrease);

        TriggerLoadRecovery(targetIncrease);
        _lastLoadRecoveryTime = currentTime;
    }
}

void BotPerformanceMonitor::TriggerLoadShedding(uint32 targetReduction)
{
    sBotPriorityMgr->SuspendLowPriorityBots(targetReduction);
}

void BotPerformanceMonitor::TriggerLoadRecovery(uint32 targetIncrease)
{
    sBotPriorityMgr->ResumeSuspendedBots(targetIncrease);
}

bool BotPerformanceMonitor::IsPerformanceDegraded() const
{
    ::std::shared_lock lock(_metricsMutex);  // READ: shared lock (parallel readers)
    return _consecutiveSlowTicks >= DEGRADATION_THRESHOLD;
}

float BotPerformanceMonitor::CalculateLoadPercent(uint32 tickTimeMicros) const
{
    // Assume 50ms tick target = 100% load
    // Actual formula: (actual_time / target_time) * 100
    constexpr uint32 TICK_INTERVAL_MICROS = 50000; // 50ms
    return (static_cast<float>(tickTimeMicros) / TICK_INTERVAL_MICROS) * 100.0f;
}

void BotPerformanceMonitor::LogPerformanceReport() const
{
    ::std::shared_lock lock(_metricsMutex);  // READ: shared lock (parallel readers)

    TC_LOG_INFO("module.playerbot.performance",
        "=== PERFORMANCE REPORT ===");
    TC_LOG_INFO("module.playerbot.performance",
        "Tick #{}: {:.2f}ms (avg: {:.2f}ms, min: {:.2f}ms, max: {:.2f}ms)",
        _tickNumber,
        _metrics.currentTickTime / 1000.0f,
        _metrics.averageTickTime / 1000.0f,
        _metrics.minTickTime / 1000.0f,
        _metrics.maxTickTime / 1000.0f);
    TC_LOG_INFO("module.playerbot.performance",
        "Bots: {} total | Updated: {} | Skipped: {} | CPU Load: {:.1f}%",
        _metrics.totalBots,
        _metrics.botsUpdatedThisTick,
        _metrics.botsSkippedThisTick,
        _metrics.cpuLoadPercent);
    TC_LOG_INFO("module.playerbot.performance",
        "Priority Distribution - Emergency: {} | High: {} | Medium: {} | Low: {} | Suspended: {}",
        _metrics.emergencyBots,
        _metrics.highPriorityBots,
        _metrics.mediumPriorityBots,
        _metrics.lowPriorityBots,
        _metrics.suspendedBots);

    if (_metrics.isOverloaded)
    {
        TC_LOG_WARN("module.playerbot.performance", "WARNING: System is OVERLOADED!");
    }
}

void BotPerformanceMonitor::LogDetailedStatistics() const
{
    ::std::shared_lock lock(_metricsMutex);  // READ: shared lock (parallel readers)

    TC_LOG_INFO("module.playerbot.performance",
        "=== DETAILED PERFORMANCE STATISTICS ===");

    // Overall metrics
    TC_LOG_INFO("module.playerbot.performance",
        "Total Ticks: {} | Total Bots: {} | Suspended: {}",
        _tickNumber,
        _metrics.totalBots,
        _metrics.suspendedBots);

    // Timing statistics
    TC_LOG_INFO("module.playerbot.performance",
        "Tick Time - Current: {:.2f}ms | Average: {:.2f}ms | Min: {:.2f}ms | Max: {:.2f}ms",
        _metrics.currentTickTime / 1000.0f,
        _metrics.averageTickTime / 1000.0f,
        _metrics.minTickTime / 1000.0f,
        _metrics.maxTickTime / 1000.0f);

    // Histogram percentiles
    TC_LOG_INFO("module.playerbot.performance",
        "Histogram - P50: {:.2f}ms | P90: {:.2f}ms | P95: {:.2f}ms | P99: {:.2f}ms",
        _histogram.GetPercentile(50) / 1000.0f,
        _histogram.GetPercentile(90) / 1000.0f,
        _histogram.GetPercentile(95) / 1000.0f,
        _histogram.GetPercentile(99) / 1000.0f);

    // Load metrics
    TC_LOG_INFO("module.playerbot.performance",
        "CPU Load: {:.1f}% | Target: {:.1f}% | Overloaded: {}",
        _metrics.cpuLoadPercent,
        _metrics.targetLoadPercent,
        _metrics.isOverloaded ? "YES" : "NO");

    // Degradation tracking
    if (_consecutiveSlowTicks > 0)
    {
        TC_LOG_WARN("module.playerbot.performance",
            "Performance Degradation: {} consecutive slow ticks",
            _consecutiveSlowTicks);
    }

    // Priority manager statistics
    sBotPriorityMgr->LogPerformanceStatistics();
}

void BotPerformanceMonitor::ResetStatistics()
{
    ::std::unique_lock lock(_metricsMutex);  // WRITE: exclusive lock

    _metrics.maxTickTime = 0;
    _metrics.minTickTime = UINT32_MAX;
    _metrics.totalErrors = 0;
    _tickNumber = 0;
    _consecutiveSlowTicks = 0;
    _consecutiveFastTicks = 0;
    _histogram.Clear();
    _recentTickTimes.fill(0);
    _movingAvgIndex = 0;

    TC_LOG_INFO("module.playerbot.performance", "Performance statistics reset");
}

} // namespace Playerbot
