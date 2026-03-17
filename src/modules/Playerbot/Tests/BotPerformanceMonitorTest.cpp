/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "Lifecycle/BotPerformanceMonitor.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <chrono>
#include <thread>

namespace Playerbot
{
namespace Test
{

/**
 * @class BotPerformanceMonitorTest
 * @brief Unit tests for BotPerformanceMonitor component
 *
 * Tests the performance monitoring system's ability to track latencies,
 * throughput, resource usage, and health status for 5000 concurrent bots.
 */
class BotPerformanceMonitorTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        monitor = BotPerformanceMonitor::instance();
        ASSERT_NE(monitor, nullptr);

        // Initialize the monitor
        EXPECT_TRUE(monitor->Initialize());
    }

    void TearDown() override
    {
        if (monitor)
        {
            monitor->Shutdown();
        }
    }

    BotPerformanceMonitor* monitor = nullptr;

    // Helper methods
    void SimulateSpawnLatency(uint64 microseconds)
    {
        auto timer = monitor->CreateSpawnTimer();
        ::std::this_thread::sleep_for(::std::chrono::microseconds(microseconds));
        // Timer automatically records on destruction
    }

    void SimulateWorkload(uint32 spawnRequests, uint32 successfulSpawns, uint32 failedSpawns)
    {
        for (uint32 i = 0; i < spawnRequests; ++i)
            monitor->RecordSpawnRequest();

        for (uint32 i = 0; i < successfulSpawns; ++i)
            monitor->RecordSuccessfulSpawn();

        for (uint32 i = 0; i < failedSpawns; ++i)
            monitor->RecordFailedSpawn();
    }
};

// === INITIALIZATION TESTS ===

TEST_F(BotPerformanceMonitorTest, InitializesSuccessfully)
{
    // Monitor should initialize and be ready for use
    EXPECT_TRUE(monitor != nullptr);

    auto snapshot = monitor->GetSnapshot();
    EXPECT_EQ(snapshot.spawnRequestsPerSec, 0);
    EXPECT_EQ(snapshot.successfulSpawnsPerSec, 0);
    EXPECT_EQ(snapshot.failedSpawnsPerSec, 0);
    EXPECT_TRUE(snapshot.scalabilityHealthy);
}

TEST_F(BotPerformanceMonitorTest, SingletonPatternWorksCorrectly)
{
    // Multiple calls to instance() should return the same object
    auto monitor1 = BotPerformanceMonitor::instance();
    auto monitor2 = BotPerformanceMonitor::instance();

    EXPECT_EQ(monitor1, monitor2);
}

// === LATENCY TRACKING TESTS ===

TEST_F(BotPerformanceMonitorTest, RecordsSpawnLatencyCorrectly)
{
    // Record some spawn latencies
    monitor->RecordSpawnLatency(1000);  // 1ms
    monitor->RecordSpawnLatency(2000);  // 2ms
    monitor->RecordSpawnLatency(3000);  // 3ms

    auto snapshot = monitor->GetSnapshot();

    // Average should be 2ms
    EXPECT_NEAR(snapshot.avgSpawnLatency, 2.0f, 0.1f);
}

TEST_F(BotPerformanceMonitorTest, RecordsDatabaseLatencyCorrectly)
{
    // Record database latencies
    monitor->RecordDatabaseLatency(500);   // 0.5ms
    monitor->RecordDatabaseLatency(1500);  // 1.5ms
    monitor->RecordDatabaseLatency(2000);  // 2ms

    auto snapshot = monitor->GetSnapshot();

    // Average should be approximately 1.33ms
    EXPECT_NEAR(snapshot.avgDatabaseLatency, 1.33f, 0.1f);
}

TEST_F(BotPerformanceMonitorTest, ScopedTimerRecordsAutomatically)
{
    // Test that scoped timer automatically records duration
    {
        auto timer = monitor->CreateSpawnTimer();
        ::std::this_thread::sleep_for(::std::chrono::microseconds(1000));
        // Timer destructor should record ~1000 microseconds
    }

    auto snapshot = monitor->GetSnapshot();

    // Should have recorded approximately 1ms
    EXPECT_GT(snapshot.avgSpawnLatency, 0.5f);  // At least 0.5ms
    EXPECT_LT(snapshot.avgSpawnLatency, 2.0f);  // Less than 2ms (allowing for variance)
}

// === THROUGHPUT TRACKING TESTS ===

TEST_F(BotPerformanceMonitorTest, RecordsThroughputMetricsCorrectly)
{
    // Simulate workload
    SimulateWorkload(100, 95, 5);  // 100 requests, 95 successful, 5 failed

    auto snapshot = monitor->GetSnapshot();

    EXPECT_EQ(snapshot.spawnRequestsPerSec, 100);
    EXPECT_EQ(snapshot.successfulSpawnsPerSec, 95);
    EXPECT_EQ(snapshot.failedSpawnsPerSec, 5);
    EXPECT_NEAR(snapshot.spawnSuccessRate, 0.95f, 0.01f);  // 95% success rate
}

TEST_F(BotPerformanceMonitorTest, HandlesZeroRequestsGracefully)
{
    // Test behavior with no requests
    auto snapshot = monitor->GetSnapshot();

    EXPECT_EQ(snapshot.spawnRequestsPerSec, 0);
    EXPECT_EQ(snapshot.spawnSuccessRate, 1.0f);  // Should default to 100% when no data
}

TEST_F(BotPerformanceMonitorTest, CountersResetAfterSnapshot)
{
    // Simulate workload
    SimulateWorkload(50, 45, 5);

    // First snapshot should show data
    auto snapshot1 = monitor->GetSnapshot();
    EXPECT_EQ(snapshot1.spawnRequestsPerSec, 50);

    // Second snapshot should show reset counters (they exchange to 0)
    auto snapshot2 = monitor->GetSnapshot();
    EXPECT_EQ(snapshot2.spawnRequestsPerSec, 0);
}

// === RESOURCE MONITORING TESTS ===

TEST_F(BotPerformanceMonitorTest, RecordsMemoryUsageCorrectly)
{
    // Record memory usage for 1000 bots using 10GB total
    uint32 botCount = 1000;
    uint64 totalMemoryBytes = 10ULL * 1024 * 1024 * 1024;  // 10GB

    monitor->RecordMemoryUsage(botCount, totalMemoryBytes);

    auto snapshot = monitor->GetSnapshot();

    EXPECT_EQ(snapshot.activeBotCount, botCount);
    EXPECT_EQ(snapshot.memoryPerBotMB, 10);  // 10MB per bot
}

TEST_F(BotPerformanceMonitorTest, HandlesCpuUsageCorrectly)
{
    // Simulate CPU usage recording (would normally come from system monitoring)
    monitor->RecordCpuUsage(75.5f);  // 75.5% CPU usage

    auto snapshot = monitor->GetSnapshot();

    EXPECT_NEAR(snapshot.cpuUsagePercent, 75.5f, 0.1f);
}

// === HEALTH ASSESSMENT TESTS ===

TEST_F(BotPerformanceMonitorTest, HealthyPerformanceReturnsTrueForGoodMetrics)
{
    // Simulate healthy performance metrics
    monitor->RecordSpawnLatency(500);    // 0.5ms spawn latency (good)
    monitor->RecordDatabaseLatency(300); // 0.3ms DB latency (good)
    monitor->RecordCpuUsage(60.0f);      // 60% CPU usage (acceptable)
    monitor->RecordMemoryUsage(1000, 5ULL * 1024 * 1024 * 1024);  // 5MB per bot (good)
    SimulateWorkload(100, 98, 2);        // 98% success rate (good)

    EXPECT_TRUE(monitor->IsPerformanceHealthy());
    EXPECT_EQ(monitor->GetPerformanceStatus(), "HEALTHY");
}

TEST_F(BotPerformanceMonitorTest, UnhealthyPerformanceReturnsFalseForHighLatency)
{
    // Simulate high spawn latency (exceeding threshold)
    monitor->RecordSpawnLatency(15000);  // 15ms spawn latency (too high)
    monitor->RecordDatabaseLatency(300); // DB latency is fine
    monitor->RecordCpuUsage(60.0f);      // CPU usage is fine
    monitor->RecordMemoryUsage(1000, 5ULL * 1024 * 1024 * 1024);  // Memory is fine
    SimulateWorkload(100, 98, 2);        // Success rate is fine

    EXPECT_FALSE(monitor->IsPerformanceHealthy());
    EXPECT_EQ(monitor->GetPerformanceStatus(), "HIGH_SPAWN_LATENCY");
}

TEST_F(BotPerformanceMonitorTest, UnhealthyPerformanceReturnsFalseForHighCpuUsage)
{
    // Simulate high CPU usage
    monitor->RecordSpawnLatency(500);    // Spawn latency is fine
    monitor->RecordDatabaseLatency(300); // DB latency is fine
    monitor->RecordCpuUsage(95.0f);      // 95% CPU usage (too high)
    monitor->RecordMemoryUsage(1000, 5ULL * 1024 * 1024 * 1024);  // Memory is fine
    SimulateWorkload(100, 98, 2);        // Success rate is fine

    EXPECT_FALSE(monitor->IsPerformanceHealthy());
    EXPECT_EQ(monitor->GetPerformanceStatus(), "HIGH_CPU_USAGE");
}

TEST_F(BotPerformanceMonitorTest, UnhealthyPerformanceReturnsFalseForHighMemoryUsage)
{
    // Simulate high memory usage per bot
    monitor->RecordSpawnLatency(500);    // Spawn latency is fine
    monitor->RecordDatabaseLatency(300); // DB latency is fine
    monitor->RecordCpuUsage(60.0f);      // CPU usage is fine
    monitor->RecordMemoryUsage(1000, 25ULL * 1024 * 1024 * 1024);  // 25MB per bot (too high)
    SimulateWorkload(100, 98, 2);        // Success rate is fine

    EXPECT_FALSE(monitor->IsPerformanceHealthy());
}

// === TIMER TESTS ===

TEST_F(BotPerformanceMonitorTest, CreateSpawnTimerReturnsValidTimer)
{
    auto timer = monitor->CreateSpawnTimer();

    EXPECT_NE(timer, nullptr);
    EXPECT_GT(timer->GetElapsedMicroseconds(), 0);
}

TEST_F(BotPerformanceMonitorTest, CreateDatabaseTimerReturnsValidTimer)
{
    auto timer = monitor->CreateDatabaseTimer();

    EXPECT_NE(timer, nullptr);
    EXPECT_GT(timer->GetElapsedMicroseconds(), 0);
}

TEST_F(BotPerformanceMonitorTest, TimerCanBeCancelled)
{
    auto timer = monitor->CreateSpawnTimer();

    ::std::this_thread::sleep_for(::std::chrono::microseconds(500));
    timer->Cancel();  // Should prevent recording

    // Timer destruction after cancellation should not record metrics
    timer.reset();

    auto snapshot = monitor->GetSnapshot();
    EXPECT_EQ(snapshot.avgSpawnLatency, 0.0f);  // Should be 0 since timer was cancelled
}

// === RESET AND STATISTICS TESTS ===

TEST_F(BotPerformanceMonitorTest, ResetCountersClearsAllMetrics)
{
    // Generate some data
    monitor->RecordSpawnLatency(1000);
    monitor->RecordDatabaseLatency(500);
    SimulateWorkload(50, 45, 5);
    monitor->RecordMemoryUsage(1000, 10ULL * 1024 * 1024 * 1024);

    // Reset counters
    monitor->ResetCounters();

    auto snapshot = monitor->GetSnapshot();

    // Latency trackers should be reset
    EXPECT_EQ(snapshot.avgSpawnLatency, 0.0f);
    EXPECT_EQ(snapshot.avgDatabaseLatency, 0.0f);

    // Throughput counters should be reset
    EXPECT_EQ(snapshot.spawnRequestsPerSec, 0);
    EXPECT_EQ(snapshot.successfulSpawnsPerSec, 0);
    EXPECT_EQ(snapshot.failedSpawnsPerSec, 0);

    // Memory and CPU should be reset
    EXPECT_EQ(snapshot.activeBotCount, 0);
    EXPECT_EQ(snapshot.cpuUsagePercent, 0.0f);
}

// === PERFORMANCE STRESS TESTS ===

TEST_F(BotPerformanceMonitorTest, HandlesHighFrequencyUpdates)
{
    // Simulate high-frequency metric recording (like in 5000 bot scenario)
    auto start = ::std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10000; ++i)
    {
        monitor->RecordSpawnRequest();
        if (i % 10 == 0)  // 10% failure rate
            monitor->RecordFailedSpawn();
        else
            monitor->RecordSuccessfulSpawn();

        monitor->RecordSpawnLatency(500 + (i % 100));  // Varying latency
    }

    auto end = ::std::chrono::high_resolution_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(end - start);

    // Recording 10,000 metrics should complete quickly (under 100ms)
    EXPECT_LT(duration.count(), 100);

    auto snapshot = monitor->GetSnapshot();
    EXPECT_EQ(snapshot.spawnRequestsPerSec, 10000);
    EXPECT_EQ(snapshot.successfulSpawnsPerSec, 9000);
    EXPECT_EQ(snapshot.failedSpawnsPerSec, 1000);
}

// === CONCURRENT ACCESS TESTS ===

TEST_F(BotPerformanceMonitorTest, ThreadSafeMetricRecording)
{
    // Test concurrent access from multiple threads
    const int numThreads = 10;
    const int recordsPerThread = 1000;

    ::std::vector<::std::thread> threads;

    for (int t = 0; t < numThreads; ++t)
    {
        threads.emplace_back([this, recordsPerThread, t]() {
            for (int i = 0; i < recordsPerThread; ++i)
            {
                monitor->RecordSpawnLatency(1000 + t);  // Unique latency per thread
                monitor->RecordSpawnRequest();
                if (i % 2 == 0)
                    monitor->RecordSuccessfulSpawn();
                else
                    monitor->RecordFailedSpawn();
            }
        });
    }

    // Wait for all threads to complete
    for (auto& thread : threads)
    {
        thread.join();
    }

    auto snapshot = monitor->GetSnapshot();

    // Should have recorded all requests from all threads
    EXPECT_EQ(snapshot.spawnRequestsPerSec, numThreads * recordsPerThread);
    EXPECT_EQ(snapshot.successfulSpawnsPerSec + snapshot.failedSpawnsPerSec,
              numThreads * recordsPerThread);
}

} // namespace Test
} // namespace Playerbot