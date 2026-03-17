/*
 * Copyright (C) 2024 TrinityCore Playerbot Module
 *
 * Comprehensive Threading Stress Test for 5000+ Bot Scalability
 */

#include "ThreadingStressTest.h"
#include "BotWorldSessionMgrOptimized.h"
#include "BotSpawnerOptimized.h"
#include "InterruptCoordinatorFixed.h"
#include "ThreadSafeClassAI.h"
#include "Player.h"
#include "World.h"
#include "Log.h"
#include <thread>
#include <chrono>
#include <random>
#include <barrier>
#include <latch>

namespace Playerbot
{
namespace Testing
{

/**
 * COMPREHENSIVE THREADING TEST SUITE
 *
 * Tests for:
 * 1. Deadlock detection
 * 2. Race condition validation
 * 3. Lock contention measurement
 * 4. Scalability verification
 * 5. Performance regression detection
 */
class ThreadingStressTest
{
public:
    struct TestConfig
    {
        uint32 numBots{1000};
        uint32 numThreads{16};
        uint32 testDurationSeconds{60};
        uint32 spawnRate{10};          // Bots per second
        uint32 updateFrequencyMs{50};
        bool enableChaosMode{false};   // Random delays and failures
        bool enableDeadlockDetection{true};
        bool enableContentionAnalysis{true};
    };

    struct TestResults
    {
        // Performance metrics
        uint64 totalUpdates{0};
        uint64 totalSpawns{0};
        uint64 totalDespawns{0};
        double averageUpdateTimeMs{0};
        double maxUpdateTimeMs{0};
        double p99UpdateTimeMs{0};

        // Concurrency metrics
        uint32 deadlocksDetected{0};
        uint32 racesDetected{0};
        uint32 contentionEvents{0};
        double averageLockWaitMs{0};
        double maxLockWaitMs{0};

        // Scalability metrics
        double throughputOpsPerSec{0};
        double cpuUtilization{0};
        uint64 memoryUsedMB{0};
        double scalabilityFactor{0};  // Linear = 1.0

        // Error counts
        uint32 failedSpawns{0};
        uint32 crashCount{0};
        uint32 assertionFailures{0};

        bool TestPassed() const
        {
            return deadlocksDetected == 0 &&
                   racesDetected == 0 &&
                   crashCount == 0 &&
                   assertionFailures == 0 &&
                   scalabilityFactor > 0.8;  // 80% linear scaling minimum
        }

        ::std::string GetSummary() const
        {
            ::std::stringstream ss;
            ss << "=== Threading Stress Test Results ===\n";
            ss << "Performance:\n";
            ss << "  Total Updates: " << totalUpdates << "\n";
            ss << "  Avg Update Time: " << averageUpdateTimeMs << " ms\n";
            ss << "  Max Update Time: " << maxUpdateTimeMs << " ms\n";
            ss << "  P99 Update Time: " << p99UpdateTimeMs << " ms\n";
            ss << "  Throughput: " << throughputOpsPerSec << " ops/sec\n\n";

            ss << "Concurrency:\n";
            ss << "  Deadlocks: " << deadlocksDetected << "\n";
            ss << "  Race Conditions: " << racesDetected << "\n";
            ss << "  Contention Events: " << contentionEvents << "\n";
            ss << "  Avg Lock Wait: " << averageLockWaitMs << " ms\n\n";

            ss << "Scalability:\n";
            ss << "  Scalability Factor: " << scalabilityFactor << "\n";
            ss << "  CPU Utilization: " << cpuUtilization << "%\n";
            ss << "  Memory Used: " << memoryUsedMB << " MB\n\n";

            ss << "Result: " << (TestPassed() ? "PASSED" : "FAILED") << "\n";
            return ss.str();
        }
    };

    // Test execution
    static TestResults RunStressTest(TestConfig const& config);
    static TestResults RunDeadlockTest();
    static TestResults RunRaceConditionTest();
    static TestResults RunScalabilityTest(uint32 minBots, uint32 maxBots);
    static TestResults RunContentionTest();

private:
    // Test scenarios
    static void SimulateConcurrentSpawns(TestConfig const& config, TestResults& results);
    static void SimulateConcurrentUpdates(TestConfig const& config, TestResults& results);
    static void SimulateConcurrentCombat(TestConfig const& config, TestResults& results);
    static void SimulateRandomOperations(TestConfig const& config, TestResults& results);

    // Analysis methods
    static void DetectDeadlocks(TestResults& results);
    static void DetectRaceConditions(TestResults& results);
    static void MeasureContention(TestResults& results);
    static void CalculateScalability(TestResults& results);

    // Helper methods
    static void SpawnBotsParallel(uint32 count, uint32 numThreads);
    static void UpdateBotsParallel(uint32 count, uint32 numThreads, uint32 durationMs);
    static void StressSessionManager(uint32 numOperations, uint32 numThreads);
    static void StressInterruptCoordinator(uint32 numBots, uint32 numCasts);
};

TestResults ThreadingStressTest::RunStressTest(TestConfig const& config)
{
    TC_LOG_INFO("test.playerbot.threading", "Starting comprehensive threading stress test");
    TC_LOG_INFO("test.playerbot.threading", "Config: {} bots, {} threads, {} seconds",
                config.numBots, config.numThreads, config.testDurationSeconds);

    TestResults results;
    auto startTime = ::std::chrono::high_resolution_clock::now();
    auto endTime = startTime + ::std::chrono::seconds(config.testDurationSeconds);

    // Initialize systems
    BotSpawnerOptimized::Instance().Initialize();
    BotWorldSessionMgrOptimized::Instance().Initialize();

    // Create thread pool for concurrent testing
    ::std::vector<::std::thread> workers;
    ::std::atomic<bool> stopFlag{false};
    ::std::barrier syncBarrier(config.numThreads);
    ::std::latch startLatch(config.numThreads);

    // Start worker threads
    for (uint32 i = 0; i < config.numThreads; ++i)
    {
        workers.emplace_back([&, i]() {
            // Thread-local random generator
            ::std::random_device rd;
            ::std::mt19937 gen(rd());
            ::std::uniform_int_distribution<> opDist(0, 3);
            ::std::uniform_int_distribution<> delayDist(1, 10);

            startLatch.count_down();
            startLatch.wait();

            while (!stopFlag.load(::std::memory_order_acquire))
            {
                auto now = ::std::chrono::high_resolution_clock::now();
                if (now >= endTime)
                {
                    stopFlag.store(true, ::std::memory_order_release);
                    break;
                }

                // Random operation
    switch (opDist(gen))
                {
                case 0: // Spawn operation
                {
                    auto opStart = ::std::chrono::high_resolution_clock::now();

                    SpawnRequest req;
                    req.type = SpawnRequest::SPAWN_ZONE;
                    req.zoneId = 1519; // Stormwind
                    req.level = 80;

                    bool success = BotSpawnerOptimized::Instance().RequestSpawn(req);
                    if (success)
                        results.totalSpawns++;
                    else
                        results.failedSpawns++;

                    auto opEnd = ::std::chrono::high_resolution_clock::now();
                    auto duration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(opEnd - opStart);

                    if (duration.count() > results.maxUpdateTimeMs)
                        results.maxUpdateTimeMs = duration.count();

                    break;
                }
                case 1: // Update operation
                {
                    BotWorldSessionMgrOptimized::Instance().UpdateAllSessions(50);
                    results.totalUpdates++;
                    break;
                }
                case 2: // Combat simulation
                {
                    // Simulate interrupt coordination
                    static thread_local ::std::unique_ptr<InterruptCoordinatorFixed> coordinator;
                    if (!coordinator)
                        coordinator = ::std::make_unique<InterruptCoordinatorFixed>(nullptr);

                    coordinator->Update(50);
                    break;
                }
                case 3: // Despawn operation
                {
                    auto bots = BotWorldSessionMgrOptimized::Instance().GetAllBotGuids();
                    if (!bots.empty())
                    {
                        ObjectGuid guid = bots[gen() % bots.size()];
                        if (BotSpawnerOptimized::Instance().DespawnBot(guid))
                            results.totalDespawns++;
                    }
                    break;
                }
                }

                // Chaos mode - random delays
    if (config.enableChaosMode)
                {
                    ::std::this_thread::sleep_for(::std::chrono::milliseconds(delayDist(gen)));
                }

                // Periodic synchronization to stress concurrent access
    if ((results.totalUpdates % 100) == 0)
                {
                    syncBarrier.arrive_and_wait();
                }
            }
        });
    }

    // Monitor thread for deadlock detection
    ::std::thread monitor([&]() {
        auto lastProgress = results.totalUpdates.load();
        uint32 stalledCount = 0;

        while (!stopFlag.load(::std::memory_order_acquire))
        {
            ::std::this_thread::sleep_for(::std::chrono::seconds(5));

            auto currentProgress = results.totalUpdates.load();
            if (currentProgress == lastProgress)
            {
                stalledCount++;
                if (stalledCount >= 3)  // 15 seconds of no progress
                {
                    TC_LOG_ERROR("test.playerbot.threading", "DEADLOCK DETECTED! No progress for 15 seconds");
                    results.deadlocksDetected++;
                    stopFlag.store(true, ::std::memory_order_release);
                }
            }
            else
            {
                stalledCount = 0;
                lastProgress = currentProgress;
            }

            // Memory usage check
            results.memoryUsedMB = BotWorldSessionMgrOptimized::Instance().GetBotCount() * 10; // Estimate
        }
    });

    // Wait for test duration
    ::std::this_thread::sleep_for(::std::chrono::seconds(config.testDurationSeconds));
    stopFlag.store(true, ::std::memory_order_release);

    // Join all threads
    for (auto& worker : workers)
        worker.join();
    monitor.join();

    // Calculate final metrics
    auto totalDuration = ::std::chrono::high_resolution_clock::now() - startTime;
    auto durationMs = ::std::chrono::duration_cast<::std::chrono::milliseconds>(totalDuration).count();

    results.throughputOpsPerSec = (results.totalUpdates + results.totalSpawns + results.totalDespawns) * 1000.0 / durationMs;
    results.averageUpdateTimeMs = durationMs / double(results.totalUpdates + 1);

    // Scalability calculation
    double expectedOps = config.numBots * 20;  // Expected 20 ops/sec per bot
    results.scalabilityFactor = results.throughputOpsPerSec / expectedOps;

    // Get statistics from managers
    auto spawnerStats = BotSpawnerOptimized::Instance().GetStatistics();
    auto sessionStats = BotWorldSessionMgrOptimized::Instance().GetStatistics();

    TC_LOG_INFO("test.playerbot.threading", "Test completed: {}", results.GetSummary());

    // Cleanup
    BotSpawnerOptimized::Instance().DespawnAllBots();
    BotWorldSessionMgrOptimized::Instance().DisconnectAllBots();

    return results;
}

TestResults ThreadingStressTest::RunDeadlockTest()
{
    TC_LOG_INFO("test.playerbot.threading", "Running deadlock detection test");
    TestResults results;

    // Create scenario that would deadlock with old design
    ::std::vector<::std::thread> threads;
    ::std::atomic<uint32> completedOps{0};

    // Thread 1: Spawner -> SessionManager
    threads.emplace_back([&]() {
        for (uint32 i = 0; i < 1000; ++i)
        {
            SpawnRequest req;
            req.type = SpawnRequest::SPAWN_ZONE;
            BotSpawnerOptimized::Instance().RequestSpawn(req);

            auto sessions = BotWorldSessionMgrOptimized::Instance().GetAllBotGuids();
            completedOps++;
        }
    });

    // Thread 2: SessionManager -> Spawner (reverse order)
    threads.emplace_back([&]() {
        for (uint32 i = 0; i < 1000; ++i)
        {
            BotWorldSessionMgrOptimized::Instance().UpdateAllSessions(10);

            auto stats = BotSpawnerOptimized::Instance().GetStatistics();
            completedOps++;
        }
    });

    // Thread 3: Interrupt Coordinator with nested operations
    threads.emplace_back([&]() {
        InterruptCoordinatorFixed coordinator(nullptr);

        for (uint32 i = 0; i < 1000; ++i)
        {
            coordinator.RegisterBot(nullptr, nullptr);
            coordinator.Update(10);
            coordinator.OnEnemyCastStart(nullptr, 12345, 2000);
            completedOps++;
        }
    });

    // Monitor for deadlock
    auto startTime = ::std::chrono::high_resolution_clock::now();
    bool deadlocked = false;

    while (completedOps < 3000)
    {
        ::std::this_thread::sleep_for(::std::chrono::milliseconds(100));

        auto elapsed = ::std::chrono::high_resolution_clock::now() - startTime;
        if (elapsed > ::std::chrono::seconds(30))
        {
            deadlocked = true;
            results.deadlocksDetected = 1;
            TC_LOG_ERROR("test.playerbot.threading", "Deadlock test FAILED - timeout after 30 seconds");
            break;
        }
    }

    if (!deadlocked)
    {
        TC_LOG_INFO("test.playerbot.threading", "Deadlock test PASSED - all operations completed");
    }

    // Cleanup threads
    for (auto& thread : threads)
    {
        if (thread.joinable())
            thread.join();
    }

    return results;
}

TestResults ThreadingStressTest::RunScalabilityTest(uint32 minBots, uint32 maxBots)
{
    TC_LOG_INFO("test.playerbot.threading", "Running scalability test from {} to {} bots", minBots, maxBots);
    TestResults results;

    struct ScalabilityPoint
    {
        uint32 numBots;
        double throughput;
        double avgLatency;
        double cpuUsage;
    };

    ::std::vector<ScalabilityPoint> dataPoints;

    // Test at different scales
    for (uint32 numBots = minBots; numBots <= maxBots; numBots *= 2)
    {
        TC_LOG_INFO("test.playerbot.threading", "Testing with {} bots", numBots);

        // Spawn bots
    for (uint32 i = 0; i < numBots; ++i)
        {
            SpawnRequest req;
            req.type = SpawnRequest::SPAWN_ZONE;
            BotSpawnerOptimized::Instance().RequestSpawn(req);
        }

        // Wait for spawns to complete
    while (BotSpawnerOptimized::Instance().GetActiveBotCount() < numBots * 0.9)
        {
            ::std::this_thread::sleep_for(::std::chrono::milliseconds(100));
        }

        // Measure performance
        auto startTime = ::std::chrono::high_resolution_clock::now();
        uint64 operations = 0;

        // Run for 10 seconds
    while (::std::chrono::high_resolution_clock::now() - startTime < ::std::chrono::seconds(10))
        {
            BotWorldSessionMgrOptimized::Instance().UpdateAllSessions(50);
            operations++;
        }

        auto duration = ::std::chrono::high_resolution_clock::now() - startTime;
        auto durationMs = ::std::chrono::duration_cast<::std::chrono::milliseconds>(duration).count();

        ScalabilityPoint point;
        point.numBots = numBots;
        point.throughput = operations * 1000.0 / durationMs;
        point.avgLatency = durationMs / double(operations);
        dataPoints.push_back(point);

        // Cleanup for next iteration
        BotSpawnerOptimized::Instance().DespawnAllBots();
        ::std::this_thread::sleep_for(::std::chrono::seconds(1));
    }

    // Calculate scalability factor
    if (dataPoints.size() >= 2)
    {
        auto& first = dataPoints[0];
        auto& last = dataPoints.back();

        double expectedScaling = last.numBots / double(first.numBots);
        double actualScaling = last.throughput / first.throughput;
        results.scalabilityFactor = actualScaling / expectedScaling;

        TC_LOG_INFO("test.playerbot.threading", "Scalability factor: {:.2f} (1.0 = perfectly linear)",
                    results.scalabilityFactor);
    }

    return results;
}

} // namespace Testing
} // namespace Playerbot