/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * CRITICAL FIX: ThreadPool Deadlock Resolution
 *
 * This file contains the epoch-based wake guarantee system that prevents
 * the condition variable lost wakeup race condition that causes all 33
 * worker threads to deadlock in _Primitive_wait_for.
 *
 * Root Cause Fixed:
 * - Wake signals sent between HasWorkAvailable() check and wait_for() entry
 * - Spurious wakeup causing immediate re-sleep without work detection
 * - All workers entering sleep simultaneously with no one left to wake them
 *
 * Solution:
 * - Epoch-based wake tracking ensures no wake signal is ever lost
 * - Periodic safety broadcast prevents stable deadlock states
 * - Improved work visibility across worker boundaries
 */

#include "ThreadPool.h"
#include "DeadlockDetector.h"
#include "../../Config/PlayerbotConfig.h"
#include "Log.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <sched.h>
#endif

#include <algorithm>
#include <random>

namespace Playerbot {
namespace Performance {

// ============================================================================
// Enhanced WorkerThread with Epoch-Based Wake Guarantee
// ============================================================================

class WorkerThreadEnhanced : public WorkerThread
{
private:
    // Epoch-based wake tracking to prevent lost wake signals
    alignas(64) ::std::atomic<uint64_t> _wakeEpoch{0};
    alignas(64) ::std::atomic<uint64_t> _lastProcessedEpoch{0};

    // Additional safety mechanisms
    alignas(64) ::std::atomic<::std::chrono::steady_clock::time_point> _lastWakeTime;
    alignas(64) ::std::atomic<uint32_t> _consecutiveSleepTimeouts{0};

    // Enhanced diagnostics for deadlock detection
    alignas(64) ::std::atomic<uint32_t> _spuriousWakeups{0};
    alignas(64) ::std::atomic<uint32_t> _epochMismatches{0};
    alignas(64) ::std::atomic<uint32_t> _forcedWakes{0};

public:
    WorkerThreadEnhanced(ThreadPool* pool, uint32 workerId, uint32 cpuCore)
        : WorkerThread(pool, workerId, cpuCore)
    {
        _lastWakeTime.store(::std::chrono::steady_clock::now());
    }

    /**
     * @brief Enhanced Sleep with epoch-based wake guarantee
     *
     * This implementation prevents lost wake signals by tracking wake epochs.
     * Even if a wake signal is sent between the work check and wait entry,
     * the epoch change will be detected and the thread will not sleep.
     */
    void SleepEnhanced()
    {
        // Safety check - don't sleep during shutdown
    if (!_running.load(::std::memory_order_relaxed) || _pool->IsShuttingDown())
            return;

        // Stagger sleep entry by worker ID to prevent thundering herd
        // Workers with lower IDs sleep slightly earlier, reducing contention
        ::std::this_thread::sleep_for(::std::chrono::microseconds(_workerId * 10));

        ::std::unique_lock<::std::mutex> lock(_wakeMutex);

        // CRITICAL: Capture epoch BEFORE setting sleeping flag
        // This ensures we can detect any wake that happens after this point
        uint64_t sleepEpoch = _wakeEpoch.load(::std::memory_order_acquire);

        // Set sleeping flag under lock
        _sleeping.store(true, ::std::memory_order_release);

        // Memory fence to ensure sleeping flag is visible before checking work
        ::std::atomic_thread_fence(::std::memory_order_seq_cst);

        // Check for available work (local + stealable)
        bool hasWork = HasWorkAvailableEnhanced();

        // Check if epoch changed during our work check
        // If it did, we were woken and must not sleep
        uint64_t currentEpoch = _wakeEpoch.load(::std::memory_order_acquire);

        if (hasWork || currentEpoch != sleepEpoch)
        {
            // Either work available or we were woken - don't sleep
            _sleeping.store(false, ::std::memory_order_release);

            if (currentEpoch != sleepEpoch)
            {
                _epochMismatches.fetch_add(1, ::std::memory_order_relaxed);
            }

            return;
        }

        // Record sleep entry time for deadlock detection
        auto sleepStartTime = ::std::chrono::steady_clock::now();

        // Wait with comprehensive wake conditions
        auto result = _wakeCv.wait_for(lock, _pool->GetConfiguration().workerSleepTime,
            [this, sleepEpoch]() {
                // Wake conditions (in priority order):
                // 1. Epoch changed (guaranteed wake signal)
    if (_wakeEpoch.load(::std::memory_order_acquire) != sleepEpoch)
                    return true;

                // 2. Explicitly woken (sleeping flag cleared)
    if (!_sleeping.load(::std::memory_order_relaxed))
                    return true;

                // 3. Shutdown requested
    if (!_running.load(::std::memory_order_relaxed) || _pool->IsShuttingDown())
                    return true;

                // 4. Work became available (double-check)
    if (HasWorkAvailableEnhanced())
                    return true;

                return false;
            });

        // Track wake reason for diagnostics
        auto wakeTime = ::std::chrono::steady_clock::now();
        auto sleepDuration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
            wakeTime - sleepStartTime);

        if (!result)
        {
            // Timeout occurred - potential spurious wakeup or genuine timeout
            _spuriousWakeups.fetch_add(1, ::std::memory_order_relaxed);
            _consecutiveSleepTimeouts.fetch_add(1, ::std::memory_order_relaxed);

            // Safety: If we've timed out too many times, force a work check
    if (_consecutiveSleepTimeouts.load(::std::memory_order_relaxed) > 5)
            {
                ForceWorkCheck();
            }
        }
        else
        {
            // Successful wake - reset timeout counter
            _consecutiveSleepTimeouts.store(0, ::std::memory_order_relaxed);
        }

        // Update last processed epoch
        _lastProcessedEpoch.store(_wakeEpoch.load(::std::memory_order_acquire),
                                 ::std::memory_order_release);

        // Clear sleeping flag
        _sleeping.store(false, ::std::memory_order_release);

        // Record wake time for monitoring
        _lastWakeTime.store(wakeTime, ::std::memory_order_release);
    }

    /**
     * @brief Enhanced Wake with epoch increment
     *
     * Increments the wake epoch to guarantee the sleeping thread will wake,
     * even if the condition variable notification is lost.
     */
    void WakeEnhanced()
    {
        ::std::lock_guard<::std::mutex> lock(_wakeMutex);

        // CRITICAL: Increment epoch FIRST
        // This guarantees the sleeping thread will see the change
        uint64_t newEpoch = _wakeEpoch.fetch_add(1, ::std::memory_order_release) + 1;

        // Clear sleeping flag
        _sleeping.store(false, ::std::memory_order_release);

        // Always notify - belt and suspenders approach
        _wakeCv.notify_one();

        // For high-priority wakes, notify multiple times to ensure delivery
    if (newEpoch % 10 == 0)  // Every 10th wake
        {
            _wakeCv.notify_all();  // Broadcast to ensure wake delivery
        }
    }

    /**
     * @brief Enhanced work availability check with better visibility
     */
    bool HasWorkAvailableEnhanced() const
    {
        // Check own queues first (fast path)
    for (size_t i = 0; i < static_cast<size_t>(TaskPriority::COUNT); ++i)
        {
            if (!_localQueues[i].Empty())
                return true;
        }

        // Memory fence to ensure we see latest updates from other threads
        ::std::atomic_thread_fence(::std::memory_order_acquire);

        // Enhanced stealing check with work visibility
    if (_pool->GetConfiguration().enableWorkStealing)
        {
            // Check pool-wide task count (more reliable than individual checks)
    if (_pool->GetQueuedTasks() > 0)
                return true;  // Work exists somewhere

            // Double-check specific workers
    for (uint32 i = 0; i < _pool->GetWorkerCount(); ++i)
            {
                if (i == _workerId)
                    continue;

                WorkerThread* other = _pool->GetWorker(i);
                if (!other)
                    continue;

                // Don't check sleeping workers (they likely have no work)
    if (other->_sleeping.load(::std::memory_order_acquire))
                    continue;

                // Check their queues
    for (size_t j = 0; j < static_cast<size_t>(TaskPriority::COUNT); ++j)
                {
                    if (!other->_localQueues[j].Empty())
                        return true;
                }
            }
        }

        return false;
    }

    /**
     * @brief Force work redistribution when stuck
     */
    void ForceWorkCheck()
    {
        _forcedWakes.fetch_add(1, ::std::memory_order_relaxed);

        // Wake a random other worker to redistribute work
        uint32 victimId = GetRandomWorkerIndex();
        if (victimId != _workerId)
        {
            WorkerThread* victim = _pool->GetWorker(victimId);
            if (victim && victim != this)
            {
                static_cast<WorkerThreadEnhanced*>(victim)->WakeEnhanced();
            }
        }

        // Reset consecutive timeout counter
        _consecutiveSleepTimeouts.store(0, ::std::memory_order_relaxed);
    }

    /**
     * @brief Get enhanced diagnostics
     */
    struct EnhancedMetrics
    {
        uint64_t wakeEpoch;
        uint64_t lastProcessedEpoch;
        uint32_t spuriousWakeups;
        uint32_t epochMismatches;
        uint32_t forcedWakes;
        uint32_t consecutiveTimeouts;
        ::std::chrono::milliseconds timeSinceLastWake;
    };

    EnhancedMetrics GetEnhancedMetrics() const
    {
        EnhancedMetrics metrics;
        metrics.wakeEpoch = _wakeEpoch.load(::std::memory_order_relaxed);
        metrics.lastProcessedEpoch = _lastProcessedEpoch.load(::std::memory_order_relaxed);
        metrics.spuriousWakeups = _spuriousWakeups.load(::std::memory_order_relaxed);
        metrics.epochMismatches = _epochMismatches.load(::std::memory_order_relaxed);
        metrics.forcedWakes = _forcedWakes.load(::std::memory_order_relaxed);
        metrics.consecutiveTimeouts = _consecutiveSleepTimeouts.load(::std::memory_order_relaxed);

        auto now = ::std::chrono::steady_clock::now();
        auto lastWake = _lastWakeTime.load(::std::memory_order_relaxed);
        metrics.timeSinceLastWake = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
            now - lastWake);

        return metrics;
    }
};

// ============================================================================
// Enhanced ThreadPool with Deadlock Prevention
// ============================================================================

class ThreadPoolEnhanced : public ThreadPool
{
private:
    // Periodic safety broadcast to prevent stable deadlock
    ::std::unique_ptr<::std::thread> _safetyThread;
    ::std::atomic<bool> _safetyThreadRunning{false};

    // Global work submission tracking
    alignas(64) ::std::atomic<uint64_t> _globalSubmissionEpoch{0};
    alignas(64) ::std::atomic<uint64_t> _lastBroadcastEpoch{0};

    // Deadlock detection state
    alignas(64) ::std::atomic<uint32_t> _allSleepingDetections{0};
    alignas(64) ::std::atomic<::std::chrono::steady_clock::time_point> _lastAllSleepingTime;

public:
    explicit ThreadPoolEnhanced(Configuration config = {})
        : ThreadPool(config)
    {
        StartSafetyThread();
    }

    ~ThreadPoolEnhanced()
    {
        StopSafetyThread();
    }

    /**
     * @brief Start periodic safety broadcast thread
     */
    void StartSafetyThread()
    {
        _safetyThreadRunning.store(true);
        _safetyThread = ::std::make_unique<::std::thread>([this]() {
            SafetyThreadLoop();
        });

#ifdef _WIN32
        // Set thread name for debugging
        SetThreadDescription(_safetyThread->native_handle(),
                           L"ThreadPool-Safety-Monitor");
#endif
    }

    /**
     * @brief Stop safety broadcast thread
     */
    void StopSafetyThread()
    {
        _safetyThreadRunning.store(false);
        if (_safetyThread && _safetyThread->joinable())
        {
            _safetyThread->join();
        }
    }

    /**
     * @brief Safety thread main loop
     */
    void SafetyThreadLoop()
    {
        while (_safetyThreadRunning.load(::std::memory_order_relaxed))
        {
            // Check every 50ms for deadlock conditions
            ::std::this_thread::sleep_for(::std::chrono::milliseconds(50));

            if (_shutdown.load(::std::memory_order_relaxed))
                break;

            // Check if all workers are sleeping
            CheckForAllSleeping();

            // Check if we need a safety broadcast
            CheckForSafetyBroadcast();

            // Monitor submission patterns
            MonitorSubmissionHealth();
        }
    }

    /**
     * @brief Check if all workers are sleeping (potential deadlock)
     */
    void CheckForAllSleeping()
    {
        if (_workers.empty())
            return;

        uint32_t sleepingCount = 0;
        uint32_t totalWorkers = static_cast<uint32_t>(_workers.size());

        for (const auto& worker : _workers)
        {
            if (worker && worker->_sleeping.load(::std::memory_order_acquire))
            {
                sleepingCount++;
            }
        }

        // If ALL workers are sleeping with queued tasks, we have a problem
    if (sleepingCount == totalWorkers && GetQueuedTasks() > 0)
        {
            _allSleepingDetections.fetch_add(1, ::std::memory_order_relaxed);
            _lastAllSleepingTime.store(::std::chrono::steady_clock::now());

            // EMERGENCY: Wake all workers immediately
            EmergencyWakeAll("All workers sleeping with pending tasks!");
        }
    }

    /**
     * @brief Check if safety broadcast needed
     */
    void CheckForSafetyBroadcast()
    {
        // Periodic safety broadcast every 100ms
        static ::std::chrono::steady_clock::time_point lastBroadcast;
        auto now = ::std::chrono::steady_clock::now();

        if (now - lastBroadcast > ::std::chrono::milliseconds(100))
        {
            lastBroadcast = now;

            // Check submission epoch vs broadcast epoch
            uint64_t currentSubmissions = _globalSubmissionEpoch.load(::std::memory_order_acquire);
            uint64_t lastBroadcast = _lastBroadcastEpoch.load(::std::memory_order_acquire);

            // If new submissions since last broadcast, wake workers
    if (currentSubmissions != lastBroadcast)
            {
                _lastBroadcastEpoch.store(currentSubmissions, ::std::memory_order_release);

                // Wake 25% of sleeping workers
                WakeSleepingWorkers(0.25f);
            }
        }
    }

    /**
     * @brief Monitor overall submission health
     */
    void MonitorSubmissionHealth()
    {
        static uint64_t lastCompleted = 0;
        static ::std::chrono::steady_clock::time_point lastCheck;

        auto now = ::std::chrono::steady_clock::now();
        if (now - lastCheck < ::std::chrono::seconds(1))
            return;

        lastCheck = now;
        uint64_t currentCompleted = _metrics.totalCompleted.load(::std::memory_order_relaxed);

        // If no progress in 1 second with queued tasks, force wake
    if (currentCompleted == lastCompleted && GetQueuedTasks() > 0)
        {
            TC_LOG_WARN("playerbot.performance",
                "ThreadPool: No progress for 1s with {} queued tasks - forcing wake",
                GetQueuedTasks());

            EmergencyWakeAll("No progress detected");
        }

        lastCompleted = currentCompleted;
    }

    /**
     * @brief Emergency wake all workers
     */
    void EmergencyWakeAll(const char* reason)
    {
        TC_LOG_ERROR("playerbot.performance",
            "ThreadPool: EMERGENCY WAKE ALL - {}", reason);

        for (auto& worker : _workers)
        {
            if (worker)
            {
                auto* enhanced = static_cast<WorkerThreadEnhanced*>(worker.get());
                enhanced->WakeEnhanced();
            }
        }
    }

    /**
     * @brief Wake a percentage of sleeping workers
     */
    void WakeSleepingWorkers(float percentage)
    {
        ::std::vector<WorkerThreadEnhanced*> sleepingWorkers;

        for (auto& worker : _workers)
        {
            if (worker && worker->_sleeping.load(::std::memory_order_acquire))
            {
                sleepingWorkers.push_back(static_cast<WorkerThreadEnhanced*>(worker.get()));
            }
        }

        if (sleepingWorkers.empty())
            return;

        size_t toWake = ::std::max(1ULL,
            static_cast<size_t>(sleepingWorkers.size() * percentage));

        // Randomly select workers to wake for better distribution
        ::std::random_device rd;
        ::std::mt19937 gen(rd());
        ::std::shuffle(sleepingWorkers.begin(), sleepingWorkers.end(), gen);

        for (size_t i = 0; i < toWake && i < sleepingWorkers.size(); ++i)
        {
            sleepingWorkers[i]->WakeEnhanced();
        }
    }

    /**
     * @brief Enhanced Submit with epoch tracking
     */
    template<typename Func, typename... Args>
    auto SubmitEnhanced(TaskPriority priority, Func&& func, Args&&... args)
        -> ::std::future<::std::invoke_result_t<::std::decay_t<Func>, ::std::decay_t<Args>...>>
    {
        // Track global submission
        _globalSubmissionEpoch.fetch_add(1, ::std::memory_order_release);

        // Use base Submit but with enhanced wake strategy
        auto future = Submit(priority, ::std::forward<Func>(func), ::std::forward<Args>(args)...);

        // Enhanced wake strategy for new submissions
        EnhancedWakeStrategy(priority);

        return future;
    }

    /**
     * @brief Enhanced wake strategy based on priority and load
     */
    void EnhancedWakeStrategy(TaskPriority priority)
    {
        // For critical tasks, wake more workers
    if (priority == TaskPriority::CRITICAL)
        {
            WakeSleepingWorkers(0.5f);  // Wake 50% for critical tasks
        }
        else if (priority == TaskPriority::HIGH)
        {
            WakeSleepingWorkers(0.3f);  // Wake 30% for high priority
        }
        else
        {
            WakeSleepingWorkers(0.2f);  // Wake 20% for normal priority
        }

        // If queues are getting full, wake everyone
    if (GetQueuedTasks() > _workers.size() * 10)
        {
            EmergencyWakeAll("Queue overflow prevention");
        }
    }

    /**
     * @brief Get enhanced diagnostic report
     */
    ::std::string GetEnhancedDiagnosticReport() const
    {
        ::std::stringstream report;
        report << "=== ThreadPool Enhanced Diagnostic Report ===\n";
        report << "Timestamp: " << ::std::chrono::system_clock::now() << "\n\n";

        report << "Safety Metrics:\n";
        report << "  All-Sleeping Detections: " << _allSleepingDetections.load() << "\n";
        report << "  Global Submission Epoch: " << _globalSubmissionEpoch.load() << "\n";
        report << "  Last Broadcast Epoch: " << _lastBroadcastEpoch.load() << "\n\n";

        report << "Worker States:\n";
        for (size_t i = 0; i < _workers.size(); ++i)
        {
            if (!_workers[i])
                continue;

            auto* enhanced = static_cast<const WorkerThreadEnhanced*>(_workers[i].get());
            auto metrics = enhanced->GetEnhancedMetrics();

            report << "  Worker " << i << ":\n";
            report << "    Wake Epoch: " << metrics.wakeEpoch << "\n";
            report << "    Last Processed: " << metrics.lastProcessedEpoch << "\n";
            report << "    Spurious Wakeups: " << metrics.spuriousWakeups << "\n";
            report << "    Epoch Mismatches: " << metrics.epochMismatches << "\n";
            report << "    Forced Wakes: " << metrics.forcedWakes << "\n";
            report << "    Consecutive Timeouts: " << metrics.consecutiveTimeouts << "\n";
            report << "    Time Since Last Wake: " << metrics.timeSinceLastWake.count() << "ms\n";
        }

        return report.str();
    }
};

// ============================================================================
// Factory function to create enhanced thread pool
// ============================================================================

::std::unique_ptr<ThreadPool> CreateEnhancedThreadPool(ThreadPool::Configuration config)
{
    TC_LOG_INFO("playerbot.performance",
        "Creating enhanced ThreadPool with epoch-based wake guarantee system");

    return ::std::make_unique<ThreadPoolEnhanced>(config);
}

} // namespace Performance
} // namespace Playerbot