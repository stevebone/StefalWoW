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
#include "Threading/LockHierarchy.h"
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>

namespace Playerbot
{

/**
 * @class BotPerformanceMonitor
 * @brief Real-time performance monitoring for 5000+ bot scalability
 *
 * PERFORMANCE CRITICAL: Monitors system performance to ensure 5000 bot
 * scalability targets are met. Tracks latency, throughput, and resource usage.
 *
 * Key Metrics:
 * - Spawn latency (target: <100ms P95)
 * - Database query performance (target: <10ms P95)
 * - Memory usage per bot (target: <10MB)
 * - CPU utilization (target: <80% total)
 * - Lock contention (target: <1% time blocked)
 */
class TC_GAME_API BotPerformanceMonitor
{
public:
    BotPerformanceMonitor();
    ~BotPerformanceMonitor() = default;

    // Singleton access
    static BotPerformanceMonitor* instance();

    // Lifecycle
    bool Initialize();
    void Shutdown();
    void Update(uint32 diff);

    // === LATENCY TRACKING ===
    struct LatencyTracker
    {
        ::std::atomic<uint64> totalTime{0};    // microseconds
        ::std::atomic<uint32> operationCount{0};
        ::std::atomic<uint64> minTime{UINT64_MAX};
        ::std::atomic<uint64> maxTime{0};

        void RecordOperation(uint64 microseconds);
        float GetAverageMs() const;
        uint64 GetMinMs() const { return minTime.load() / 1000; }
        uint64 GetMaxMs() const { return maxTime.load() / 1000; }
        void Reset();
    };

    // Record performance events
    void RecordSpawnLatency(uint64 microseconds) { _spawnLatency.RecordOperation(microseconds); }
    void RecordDatabaseQuery(uint64 microseconds) { _databaseLatency.RecordOperation(microseconds); }
    void RecordAsyncCallback(uint64 microseconds) { _callbackLatency.RecordOperation(microseconds); }

    // === THROUGHPUT TRACKING ===
    void RecordSpawnRequest() { _spawnRequestsPerSecond.fetch_add(1); }
    void RecordSuccessfulSpawn() { _successfulSpawnsPerSecond.fetch_add(1); }
    void RecordFailedSpawn() { _failedSpawnsPerSecond.fetch_add(1); }

    // === RESOURCE USAGE ===
    void RecordMemoryUsage(uint32 botCount, uint64 totalMemoryBytes);
    void RecordCpuUsage(float cpuPercent) { _cpuUsage.store(cpuPercent); }

    // === LOCK CONTENTION TRACKING ===
    void RecordLockWait(uint64 microseconds) { _lockWaitTime.RecordOperation(microseconds); }

    // === PERFORMANCE METRICS ACCESS ===
    struct PerformanceSnapshot
    {
        // Latency metrics (milliseconds)
        float avgSpawnLatency = 0.0f;
        float avgDatabaseLatency = 0.0f;
        float avgCallbackLatency = 0.0f;
        float avgLockWaitTime = 0.0f;

        // Throughput metrics (per second)
        uint32 spawnRequestsPerSec = 0;
        uint32 successfulSpawnsPerSec = 0;
        uint32 failedSpawnsPerSec = 0;
        float spawnSuccessRate = 0.0f;

        // Resource usage
        uint32 activeBotCount = 0;
        uint64 memoryPerBotMB = 0;
        float cpuUsagePercent = 0.0f;

        // Scalability indicators
        bool scalabilityHealthy = true;
        ::std::string performanceStatus = "HEALTHY";
    };

    PerformanceSnapshot GetSnapshot() const;

    // === ALERTING SYSTEM ===
    bool IsPerformanceHealthy() const;
    ::std::string GetPerformanceStatus() const;

    // === RESET COUNTERS ===
    void ResetCounters();

    // === SCOPED TIMERS ===
    class ScopedTimer
    {
    public:
        ScopedTimer(LatencyTracker& tracker);
        ~ScopedTimer();

    private:
        LatencyTracker& _tracker;
        ::std::chrono::high_resolution_clock::time_point _start;
    };

    // Convenience macros for timing
    ScopedTimer CreateSpawnTimer() { return ScopedTimer(_spawnLatency); }
    ScopedTimer CreateDatabaseTimer() { return ScopedTimer(_databaseLatency); }
    ScopedTimer CreateCallbackTimer() { return ScopedTimer(_callbackLatency); }

private:
    // Update throughput calculations
    void UpdateThroughputMetrics();

    // Performance thresholds for 5000 bot scaling
    static constexpr uint64 MAX_SPAWN_LATENCY_MS = 100;      // 100ms P95
    static constexpr uint64 MAX_DATABASE_LATENCY_MS = 10;    // 10ms P95
    static constexpr float MAX_CPU_USAGE_PERCENT = 80.0f;    // 80% total CPU
    static constexpr uint64 MAX_MEMORY_PER_BOT_MB = 10;      // 10MB per bot
    static constexpr float MIN_SUCCESS_RATE = 0.99f;        // 99% spawn success

    // Performance counters
    LatencyTracker _spawnLatency;
    LatencyTracker _databaseLatency;
    LatencyTracker _callbackLatency;
    LatencyTracker _lockWaitTime;

    // Throughput counters (per interval)
    ::std::atomic<uint32> _spawnRequestsPerSecond{0};
    ::std::atomic<uint32> _successfulSpawnsPerSecond{0};
    ::std::atomic<uint32> _failedSpawnsPerSecond{0};

    // Resource usage
    ::std::atomic<uint64> _totalMemoryBytes{0};
    ::std::atomic<uint32> _currentBotCount{0};
    ::std::atomic<float> _cpuUsage{0.0f};

    // Timing for throughput calculation
    ::std::chrono::steady_clock::time_point _lastUpdate;
    uint32 _updateInterval = 1000; // 1 second

    // Singleton
    inline static ::std::unique_ptr<BotPerformanceMonitor> _instance;
    inline static Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_SPAWNER> _instanceMutex;

    // Non-copyable
    BotPerformanceMonitor(BotPerformanceMonitor const&) = delete;
    BotPerformanceMonitor& operator=(BotPerformanceMonitor const&) = delete;
};

// Convenience macros for performance monitoring
#define MONITOR_SPAWN_LATENCY() auto _spawn_timer = sBotPerformanceMonitor->CreateSpawnTimer()
#define MONITOR_DATABASE_LATENCY() auto _db_timer = sBotPerformanceMonitor->CreateDatabaseTimer()
#define MONITOR_CALLBACK_LATENCY() auto _callback_timer = sBotPerformanceMonitor->CreateCallbackTimer()

#define sBotPerformanceMonitor BotPerformanceMonitor::instance()

} // namespace Playerbot