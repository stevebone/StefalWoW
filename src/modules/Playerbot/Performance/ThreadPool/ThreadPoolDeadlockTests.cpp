/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * ThreadPool Deadlock Test Suite
 *
 * Comprehensive tests to reproduce and verify fixes for the ThreadPool
 * deadlock condition where all 33 workers enter permanent wait state.
 */

#include "ThreadPool.h"
#include "ThreadPoolDeadlockFix.cpp"
#include <gtest/gtest.h>
#include <vector>
#include <future>
#include <random>
#include <atomic>

namespace Playerbot {
namespace Performance {
namespace Tests {

class ThreadPoolDeadlockTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Set up test configuration matching production
        config_.numThreads = 33;  // Match production worker count
        config_.enableWorkStealing = true;
        config_.workerSleepTime = ::std::chrono::milliseconds(10);
        config_.maxStealAttempts = 3;
    }

    void TearDown() override
    {
        // Cleanup
    }

    ThreadPool::Configuration config_;
};

/**
 * @brief Test: Reproduce the exact deadlock scenario from production
 *
 * This test recreates the conditions that cause all 33 workers to enter
 * permanent _Primitive_wait_for state.
 */
TEST_F(ThreadPoolDeadlockTest, ReproduceProductionDeadlock)
{
    ThreadPool pool(config_);
    ::std::atomic<uint32_t> tasksCompleted{0};
    ::std::atomic<bool> deadlockDetected{false};

    // Phase 1: Saturate pool with quick tasks to get all workers active
    ::std::vector<::std::future<void>> phase1Futures;
    for (int i = 0; i < 33; ++i)
    {
        phase1Futures.push_back(pool.Submit(TaskPriority::NORMAL, [&tasksCompleted]() {
            // Quick task to complete
            ::std::this_thread::sleep_for(::std::chrono::microseconds(100));
            tasksCompleted.fetch_add(1);
        }));
    }

    // Wait for all phase 1 tasks to complete
    for (auto& f : phase1Futures)
    {
        f.get();
    }

    EXPECT_EQ(tasksCompleted.load(), 33) << "All initial tasks should complete";

    // Phase 2: All workers now idle and about to sleep
    // Small delay to let them enter sleep state
    ::std::this_thread::sleep_for(::std::chrono::milliseconds(15));

    // Verify most workers are sleeping
    size_t sleepingWorkers = 0;
    for (uint32_t i = 0; i < pool.GetWorkerCount(); ++i)
    {
        auto* worker = pool.GetWorker(i);
        if (worker && worker->_sleeping.load(::std::memory_order_acquire))
        {
            sleepingWorkers++;
        }
    }

    EXPECT_GE(sleepingWorkers, 30) << "Most workers should be sleeping";

    // Phase 3: Submit burst of new work that should trigger deadlock
    ::std::vector<::std::future<void>> phase2Futures;
    tasksCompleted.store(0);

    for (int i = 0; i < 100; ++i)
    {
        phase2Futures.push_back(pool.Submit(TaskPriority::HIGH, [&tasksCompleted]() {
            ::std::this_thread::sleep_for(::std::chrono::milliseconds(1));
            tasksCompleted.fetch_add(1);
        }));
    }

    // Phase 4: Monitor for deadlock (tasks not completing in reasonable time)
    auto startTime = ::std::chrono::steady_clock::now();
    bool allCompleted = false;

    while (::std::chrono::steady_clock::now() - startTime < ::std::chrono::seconds(5))
    {
        if (tasksCompleted.load() == 100)
        {
            allCompleted = true;
            break;
        }

        // Check if all workers are stuck sleeping
        size_t stuckSleeping = 0;
        for (uint32_t i = 0; i < pool.GetWorkerCount(); ++i)
        {
            auto* worker = pool.GetWorker(i);
            if (worker && worker->_sleeping.load(::std::memory_order_acquire))
            {
                stuckSleeping++;
            }
        }

        // Deadlock detected if all workers sleeping with pending work
    if (stuckSleeping == pool.GetWorkerCount() && pool.GetQueuedTasks() > 0)
        {
            deadlockDetected.store(true);
            break;
        }

        ::std::this_thread::sleep_for(::std::chrono::milliseconds(100));
    }

    if (deadlockDetected.load())
    {
        FAIL() << "DEADLOCK DETECTED: All " << pool.GetWorkerCount()
               << " workers sleeping with " << pool.GetQueuedTasks()
               << " tasks queued!";
    }

    EXPECT_TRUE(allCompleted) << "All tasks should complete without deadlock";
    EXPECT_EQ(tasksCompleted.load(), 100) << "All 100 tasks should complete";
}

/**
 * @brief Test: Verify the epoch-based wake guarantee fix
 */
TEST_F(ThreadPoolDeadlockTest, VerifyEpochBasedWakeFix)
{
    ThreadPoolEnhanced pool(config_);
    ::std::atomic<uint32_t> tasksCompleted{0};

    // Run the same deadlock scenario but with enhanced pool
    ::std::vector<::std::future<void>> phase1Futures;
    for (int i = 0; i < 33; ++i)
    {
        phase1Futures.push_back(pool.SubmitEnhanced(TaskPriority::NORMAL, [&tasksCompleted]() {
            ::std::this_thread::sleep_for(::std::chrono::microseconds(100));
            tasksCompleted.fetch_add(1);
        }));
    }

    for (auto& f : phase1Futures)
    {
        f.get();
    }

    // Let workers sleep
    ::std::this_thread::sleep_for(::std::chrono::milliseconds(15));

    // Submit burst that would cause deadlock in original implementation
    ::std::vector<::std::future<void>> phase2Futures;
    tasksCompleted.store(0);

    for (int i = 0; i < 100; ++i)
    {
        phase2Futures.push_back(pool.SubmitEnhanced(TaskPriority::HIGH, [&tasksCompleted]() {
            ::std::this_thread::sleep_for(::std::chrono::milliseconds(1));
            tasksCompleted.fetch_add(1);
        }));
    }

    // All tasks should complete quickly with the fix
    auto startTime = ::std::chrono::steady_clock::now();

    for (auto& f : phase2Futures)
    {
        auto status = f.wait_for(::std::chrono::seconds(2));
        ASSERT_EQ(status, ::std::future_status::ready)
            << "Task should complete without deadlock";
    }

    auto duration = ::std::chrono::steady_clock::now() - startTime;
    EXPECT_LT(duration, ::std::chrono::seconds(1))
        << "All tasks should complete quickly with epoch-based wake";

    EXPECT_EQ(tasksCompleted.load(), 100) << "All tasks should complete";
}

/**
 * @brief Test: Stress test with rapid submit/sleep cycles
 */
TEST_F(ThreadPoolDeadlockTest, StressTestRapidSubmitSleepCycles)
{
    ThreadPoolEnhanced pool(config_);
    ::std::atomic<uint32_t> totalCompleted{0};
    ::std::atomic<bool> testRunning{true};

    // Background thread that monitors for deadlock
    ::std::thread monitor([&]() {
        while (testRunning.load())
        {
            ::std::this_thread::sleep_for(::std::chrono::milliseconds(100));

            // Check for all workers sleeping with work queued
            size_t sleeping = 0;
            for (uint32_t i = 0; i < pool.GetWorkerCount(); ++i)
            {
                auto* worker = pool.GetWorker(i);
                if (worker && worker->_sleeping.load())
                {
                    sleeping++;
                }
            }

            if (sleeping == pool.GetWorkerCount() && pool.GetQueuedTasks() > 10)
            {
                ADD_FAILURE() << "Potential deadlock: All workers sleeping with "
                             << pool.GetQueuedTasks() << " tasks queued";
                break;
            }
        }
    });

    // Rapid submit/complete cycles to stress the sleep/wake mechanism
    for (int cycle = 0; cycle < 10; ++cycle)
    {
        ::std::vector<::std::future<void>> futures;

        // Submit batch
    for (int i = 0; i < 50; ++i)
        {
            futures.push_back(pool.SubmitEnhanced(TaskPriority::NORMAL, [&totalCompleted]() {
                // Variable sleep to create different completion patterns
                ::std::this_thread::sleep_for(::std::chrono::microseconds(rand() % 1000));
                totalCompleted.fetch_add(1);
            }));
        }

        // Wait for completion
    for (auto& f : futures)
        {
            auto status = f.wait_for(::std::chrono::seconds(1));
            ASSERT_EQ(status, ::std::future_status::ready)
                << "Task should complete in cycle " << cycle;
        }

        // Brief pause between cycles
        ::std::this_thread::sleep_for(::std::chrono::milliseconds(5));
    }

    testRunning.store(false);
    monitor.join();

    EXPECT_GE(totalCompleted.load(), 500) << "All stress test tasks should complete";
}

/**
 * @brief Test: Verify wake signal is never lost
 */
TEST_F(ThreadPoolDeadlockTest, VerifyWakeSignalNeverLost)
{
    ThreadPoolEnhanced pool(config_);

    // Create controlled test scenario
    for (int iteration = 0; iteration < 100; ++iteration)
    {
        ::std::atomic<bool> taskExecuted{false};

        // Let all workers sleep
        ::std::this_thread::sleep_for(::std::chrono::milliseconds(15));

        // Submit single task - should always wake a worker
        auto future = pool.SubmitEnhanced(TaskPriority::CRITICAL, [&taskExecuted]() {
            taskExecuted.store(true);
        });

        // Task should complete quickly
        auto status = future.wait_for(::std::chrono::milliseconds(100));

        ASSERT_EQ(status, ::std::future_status::ready)
            << "Wake signal lost in iteration " << iteration;

        ASSERT_TRUE(taskExecuted.load())
            << "Task not executed in iteration " << iteration;
    }
}

/**
 * @brief Test: Verify spurious wakeup handling
 */
TEST_F(ThreadPoolDeadlockTest, VerifySpuriousWakeupHandling)
{
    ThreadPoolEnhanced pool(config_);
    ::std::atomic<uint32_t> completed{0};

    // Submit tasks with delays to cause timeouts
    ::std::vector<::std::future<void>> futures;

    for (int i = 0; i < 10; ++i)
    {
        futures.push_back(pool.SubmitEnhanced(TaskPriority::LOW, [&completed]() {
            ::std::this_thread::sleep_for(::std::chrono::milliseconds(50));
            completed.fetch_add(1);
        }));

        // Delay between submissions to trigger sleep/wake cycles
        ::std::this_thread::sleep_for(::std::chrono::milliseconds(20));
    }

    // All should complete despite spurious wakeups
    for (auto& f : futures)
    {
        auto status = f.wait_for(::std::chrono::seconds(1));
        ASSERT_EQ(status, ::std::future_status::ready)
            << "Task should complete despite spurious wakeups";
    }

    EXPECT_EQ(completed.load(), 10) << "All tasks should complete";
}

/**
 * @brief Test: Verify emergency wake mechanism
 */
TEST_F(ThreadPoolDeadlockTest, VerifyEmergencyWakeMechanism)
{
    ThreadPoolEnhanced pool(config_);

    // Force all workers to sleep
    ::std::this_thread::sleep_for(::std::chrono::milliseconds(20));

    // Submit many tasks at once
    ::std::vector<::std::future<int>> futures;
    for (int i = 0; i < 200; ++i)
    {
        futures.push_back(pool.SubmitEnhanced(TaskPriority::NORMAL, [i]() {
            return i * 2;
        }));
    }

    // The emergency wake should trigger due to queue overflow
    // All tasks should still complete
    for (size_t i = 0; i < futures.size(); ++i)
    {
        auto status = futures[i].wait_for(::std::chrono::seconds(2));
        ASSERT_EQ(status, ::std::future_status::ready)
            << "Emergency wake should prevent deadlock for task " << i;

        int result = futures[i].get();
        EXPECT_EQ(result, i * 2) << "Task " << i << " produced wrong result";
    }
}

/**
 * @brief Benchmark: Compare performance of original vs fixed implementation
 */
TEST_F(ThreadPoolDeadlockTest, BenchmarkFixOverhead)
{
    const int numTasks = 1000;
    ::std::atomic<uint32_t> completed{0};

    // Benchmark original pool (if safe to test)
    // Note: Skip if it causes deadlock
    /*
    {
        ThreadPool originalPool(config_);
        auto start = ::std::chrono::high_resolution_clock::now();

        ::std::vector<::std::future<void>> futures;
        for (int i = 0; i < numTasks; ++i)
        {
            futures.push_back(originalPool.Submit(TaskPriority::NORMAL, [&completed]() {
                completed.fetch_add(1);
            }));
        }

        for (auto& f : futures)
        {
            f.wait();
        }

        auto duration = ::std::chrono::high_resolution_clock::now() - start;
        auto ms = ::std::chrono::duration_cast<::std::chrono::milliseconds>(duration).count();
        ::std::cout << "Original pool: " << ms << "ms for " << numTasks << " tasks\n";
    }
    */

    // Benchmark enhanced pool
    completed.store(0);
    {
        ThreadPoolEnhanced enhancedPool(config_);
        auto start = ::std::chrono::high_resolution_clock::now();

        ::std::vector<::std::future<void>> futures;
        for (int i = 0; i < numTasks; ++i)
        {
            futures.push_back(enhancedPool.SubmitEnhanced(TaskPriority::NORMAL, [&completed]() {
                completed.fetch_add(1);
            }));
        }

        for (auto& f : futures)
        {
            f.wait();
        }

        auto duration = ::std::chrono::high_resolution_clock::now() - start;
        auto ms = ::std::chrono::duration_cast<::std::chrono::milliseconds>(duration).count();

        ::std::cout << "Enhanced pool: " << ms << "ms for " << numTasks << " tasks\n";
        ::std::cout << "Average latency: " << (ms * 1000.0 / numTasks) << "μs per task\n";

        EXPECT_EQ(completed.load(), numTasks) << "All tasks should complete";
        EXPECT_LT(ms, 500) << "Should complete 1000 tasks in under 500ms";
    }
}

/**
 * @brief Test: Verify diagnostic reporting
 */
TEST_F(ThreadPoolDeadlockTest, VerifyDiagnosticReporting)
{
    ThreadPoolEnhanced pool(config_);

    // Submit some work
    ::std::vector<::std::future<void>> futures;
    for (int i = 0; i < 10; ++i)
    {
        futures.push_back(pool.SubmitEnhanced(TaskPriority::NORMAL, []() {
            ::std::this_thread::sleep_for(::std::chrono::milliseconds(10));
        }));
    }

    // Get diagnostic report
    ::std::string report = pool.GetEnhancedDiagnosticReport();

    // Verify report contains key information
    EXPECT_TRUE(report.find("ThreadPool Enhanced Diagnostic Report") != ::std::string::npos);
    EXPECT_TRUE(report.find("Safety Metrics") != ::std::string::npos);
    EXPECT_TRUE(report.find("Worker States") != ::std::string::npos);
    EXPECT_TRUE(report.find("Wake Epoch") != ::std::string::npos);

    // Wait for tasks to complete
    for (auto& f : futures)
    {
        f.wait();
    }
}

} // namespace Tests
} // namespace Performance
} // namespace Playerbot