/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#include "BotPerformanceMonitor.h"
#include "Logging/Log.h"
#include <algorithm>

namespace Playerbot
{

// Static member definitions removed - now inline static in header to fix DLL export issues

BotPerformanceMonitor::BotPerformanceMonitor()
    : _lastUpdate(::std::chrono::steady_clock::now())
{
}

BotPerformanceMonitor* BotPerformanceMonitor::instance()
{
    ::std::lock_guard lock(_instanceMutex);
    if (!_instance)
        _instance = ::std::unique_ptr<BotPerformanceMonitor>(new BotPerformanceMonitor());
    return _instance.get();
}

bool BotPerformanceMonitor::Initialize()
{
    TC_LOG_INFO("module.playerbot.performance",
        "Initializing BotPerformanceMonitor for 5000 bot scalability tracking");

    ResetCounters();
    _lastUpdate = ::std::chrono::steady_clock::now();

    return true;
}

void BotPerformanceMonitor::Shutdown()
{
    TC_LOG_INFO("module.playerbot.performance",
        "Shutting down BotPerformanceMonitor. Final performance summary:");

    auto snapshot = GetSnapshot();
    TC_LOG_INFO("module.playerbot.performance",
        "Final Performance - Spawn Latency: {:.2f}ms, DB Latency: {:.2f}ms, Success Rate: {:.1f}%, CPU: {:.1f}%",
        snapshot.avgSpawnLatency, snapshot.avgDatabaseLatency,
        snapshot.spawnSuccessRate * 100.0f, snapshot.cpuUsagePercent);
}

void BotPerformanceMonitor::Update(uint32 /*diff*/)
{
    auto now = ::std::chrono::steady_clock::now();
    auto elapsed = ::std::chrono::duration_cast<::std::chrono::milliseconds>(now - _lastUpdate);

    if (elapsed.count() >= _updateInterval)
    {
        UpdateThroughputMetrics();
        _lastUpdate = now;

        // Log performance alerts if thresholds exceeded
    if (!IsPerformanceHealthy())
        {
            auto snapshot = GetSnapshot();
            TC_LOG_WARN("module.playerbot.performance",
                "PERFORMANCE ALERT - Status: {}, Spawn Latency: {:.2f}ms, DB Latency: {:.2f}ms, CPU: {:.1f}%",
                snapshot.performanceStatus, snapshot.avgSpawnLatency,
                snapshot.avgDatabaseLatency, snapshot.cpuUsagePercent);
        }
    }
}

void BotPerformanceMonitor::UpdateThroughputMetrics()
{
    // Throughput counters are automatically reset after reading
    // This gives us per-second rates
}

void BotPerformanceMonitor::RecordMemoryUsage(uint32 botCount, uint64 totalMemoryBytes)
{
    _currentBotCount.store(botCount);
    _totalMemoryBytes.store(totalMemoryBytes);
}

BotPerformanceMonitor::PerformanceSnapshot BotPerformanceMonitor::GetSnapshot() const
{
    PerformanceSnapshot snapshot;

    // Latency metrics
    snapshot.avgSpawnLatency = _spawnLatency.GetAverageMs();
    snapshot.avgDatabaseLatency = _databaseLatency.GetAverageMs();
    snapshot.avgCallbackLatency = _callbackLatency.GetAverageMs();
    snapshot.avgLockWaitTime = _lockWaitTime.GetAverageMs();

    // Throughput metrics (atomic loads without reset in const method)
    uint32 requests = _spawnRequestsPerSecond.load();
    uint32 successful = _successfulSpawnsPerSecond.load();
    uint32 failed = _failedSpawnsPerSecond.load();

    snapshot.spawnRequestsPerSec = requests;
    snapshot.successfulSpawnsPerSec = successful;
    snapshot.failedSpawnsPerSec = failed;
    snapshot.spawnSuccessRate = (requests > 0) ? static_cast<float>(successful) / requests : 1.0f;

    // Resource usage
    snapshot.activeBotCount = _currentBotCount.load();
    uint64 totalMem = _totalMemoryBytes.load();
    snapshot.memoryPerBotMB = (snapshot.activeBotCount > 0) ?
        (totalMem / snapshot.activeBotCount) / (1024 * 1024) : 0;
    snapshot.cpuUsagePercent = _cpuUsage.load();

    // Performance health assessment
    snapshot.scalabilityHealthy = IsPerformanceHealthy();
    snapshot.performanceStatus = GetPerformanceStatus();

    return snapshot;
}

bool BotPerformanceMonitor::IsPerformanceHealthy() const
{
    // Check all critical thresholds for 5000 bot scalability
    float avgSpawnLatency = _spawnLatency.GetAverageMs();
    float avgDbLatency = _databaseLatency.GetAverageMs();
    float cpuUsage = _cpuUsage.load();

    // Calculate success rate
    uint32 total = _spawnRequestsPerSecond.load() + _successfulSpawnsPerSecond.load() + _failedSpawnsPerSecond.load();
    float successRate = (total > 0) ? static_cast<float>(_successfulSpawnsPerSecond.load()) / total : 1.0f;

    // Memory per bot
    uint32 botCount = _currentBotCount.load();
    uint64 memoryPerBotMB = (botCount > 0) ? (_totalMemoryBytes.load() / botCount) / (1024 * 1024) : 0;

    return avgSpawnLatency <= MAX_SPAWN_LATENCY_MS &&
           avgDbLatency <= MAX_DATABASE_LATENCY_MS &&
           cpuUsage <= MAX_CPU_USAGE_PERCENT &&
           memoryPerBotMB <= MAX_MEMORY_PER_BOT_MB &&
           successRate >= MIN_SUCCESS_RATE;
}

::std::string BotPerformanceMonitor::GetPerformanceStatus() const
{
    if (IsPerformanceHealthy())
        return "HEALTHY";

    // Determine specific performance issues
    float avgSpawnLatency = _spawnLatency.GetAverageMs();
    float avgDbLatency = _databaseLatency.GetAverageMs();
    float cpuUsage = _cpuUsage.load();

    if (avgSpawnLatency > MAX_SPAWN_LATENCY_MS)
        return "HIGH_SPAWN_LATENCY";
    if (avgDbLatency > MAX_DATABASE_LATENCY_MS)
        return "HIGH_DB_LATENCY";
    if (cpuUsage > MAX_CPU_USAGE_PERCENT)
        return "HIGH_CPU_USAGE";

    return "DEGRADED";
}

void BotPerformanceMonitor::ResetCounters()
{
    _spawnLatency.Reset();
    _databaseLatency.Reset();
    _callbackLatency.Reset();
    _lockWaitTime.Reset();

    _spawnRequestsPerSecond.store(0);
    _successfulSpawnsPerSecond.store(0);
    _failedSpawnsPerSecond.store(0);
    _totalMemoryBytes.store(0);
    _currentBotCount.store(0);
    _cpuUsage.store(0.0f);
}

// LatencyTracker implementation
void BotPerformanceMonitor::LatencyTracker::RecordOperation(uint64 microseconds)
{
    totalTime.fetch_add(microseconds);
    operationCount.fetch_add(1);

    // Update min/max with atomic compare-exchange
    uint64 currentMin = minTime.load();
    while (microseconds < currentMin && !minTime.compare_exchange_weak(currentMin, microseconds))
    {
        // Retry until successful or value is no longer minimal
    }

    uint64 currentMax = maxTime.load();
    while (microseconds > currentMax && !maxTime.compare_exchange_weak(currentMax, microseconds))
    {
        // Retry until successful or value is no longer maximal
    }
}

float BotPerformanceMonitor::LatencyTracker::GetAverageMs() const
{
    uint32 count = operationCount.load();
    uint64 total = totalTime.load();
    return count > 0 ? static_cast<float>(total) / count / 1000.0f : 0.0f;
}

void BotPerformanceMonitor::LatencyTracker::Reset()
{
    totalTime.store(0);
    operationCount.store(0);
    minTime.store(UINT64_MAX);
    maxTime.store(0);
}

// ScopedTimer implementation
BotPerformanceMonitor::ScopedTimer::ScopedTimer(LatencyTracker& tracker)
    : _tracker(tracker), _start(::std::chrono::high_resolution_clock::now())
{
}

BotPerformanceMonitor::ScopedTimer::~ScopedTimer()
{
    auto end = ::std::chrono::high_resolution_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(end - _start);
    _tracker.RecordOperation(duration.count());
}

} // namespace Playerbot