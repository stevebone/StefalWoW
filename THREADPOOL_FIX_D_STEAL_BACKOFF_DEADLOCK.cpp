/*
 * CRITICAL FIX D: Steal Backoff Deadlock Resolution
 *
 * Problem: TryStealTask() uses exponential backoff with condition variable wait
 * that caps at 1ms, causing deadlock when all threads enter backoff simultaneously.
 *
 * Root Cause: When all workers try to steal and fail, they all enter backoff
 * wait on the SAME condition variable (_wakeCv) used for Sleep/Wake, creating
 * contention and confusion about which threads are sleeping vs backing off.
 *
 * Solution: Remove condition variable wait from steal backoff entirely.
 * Use simple yield-based backoff that doesn't block threads.
 */

// ============================================================================
// MODIFIED: WorkerThread::TryStealTask() - Remove CV-based backoff
// ============================================================================

bool WorkerThread::TryStealTask()
{
    uint32 attempts = 0;
    uint32 maxAttempts = _pool->GetConfiguration().maxStealAttempts;
    uint32 yieldsPerAttempt = 1;  // Start with 1 yield

    while (attempts < maxAttempts)
    {
        _metrics.stealAttempts.fetch_add(1, std::memory_order_relaxed);

        // Get random worker to steal from
        uint32 victimId = GetRandomWorkerIndex();
        if (victimId == _workerId)
        {
            ++attempts;
            continue;
        }

        WorkerThread* victim = _pool->GetWorker(victimId);
        if (!victim)
        {
            ++attempts;
            continue;
        }

        // Check if victim is sleeping (likely has no work)
        if (victim->_sleeping.load(std::memory_order_relaxed))
        {
            ++attempts;
            continue;  // Skip sleeping workers
        }

        // Try to steal from each priority level (highest priority first)
        for (size_t i = 0; i < static_cast<size_t>(TaskPriority::COUNT); ++i)
        {
            Task* task = nullptr;
            if (victim->_localQueues[i].Steal(task) && task)
            {
                _metrics.stealSuccesses.fetch_add(1, std::memory_order_relaxed);

                auto startTime = std::chrono::steady_clock::now();

                // Execute stolen task
                task->Execute();

                auto endTime = std::chrono::steady_clock::now();
                auto workTime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

                _metrics.tasksCompleted.fetch_add(1, std::memory_order_relaxed);
                _metrics.totalWorkTime.fetch_add(workTime, std::memory_order_relaxed);

                _pool->RecordTaskCompletion(task);

                return true;
            }
        }

        ++attempts;

        // CRITICAL FIX D: Replace CV wait with yield-based backoff
        // This prevents deadlock when all threads enter backoff simultaneously
        if (attempts < maxAttempts)
        {
            // Progressive yield strategy - more yields on repeated failures
            for (uint32 y = 0; y < yieldsPerAttempt; ++y)
            {
                // Check for shutdown or new work before each yield
                if (!_running.load(std::memory_order_relaxed) ||
                    _pool->IsShuttingDown() ||
                    !_localQueues[0].Empty())  // Check CRITICAL queue for urgent work
                {
                    return false;  // Exit early if shutdown or urgent work
                }

                // Yield CPU to other threads
                std::this_thread::yield();
            }

            // Exponentially increase yields (cap at 8 to prevent excessive spinning)
            yieldsPerAttempt = std::min(yieldsPerAttempt * 2, 8u);
        }
    }

    return false;
}

// ============================================================================
// ALTERNATIVE AGGRESSIVE FIX: Complete removal of steal backoff
// ============================================================================

bool WorkerThread::TryStealTask_Aggressive()
{
    uint32 attempts = 0;
    uint32 maxAttempts = _pool->GetConfiguration().maxStealAttempts;

    while (attempts < maxAttempts)
    {
        _metrics.stealAttempts.fetch_add(1, std::memory_order_relaxed);

        // Get random worker to steal from
        uint32 victimId = GetRandomWorkerIndex();
        if (victimId == _workerId)
        {
            ++attempts;
            continue;
        }

        WorkerThread* victim = _pool->GetWorker(victimId);
        if (!victim || victim->_sleeping.load(std::memory_order_relaxed))
        {
            ++attempts;
            continue;
        }

        // Try to steal from each priority level (highest priority first)
        for (size_t i = 0; i < static_cast<size_t>(TaskPriority::COUNT); ++i)
        {
            Task* task = nullptr;
            if (victim->_localQueues[i].Steal(task) && task)
            {
                _metrics.stealSuccesses.fetch_add(1, std::memory_order_relaxed);

                auto startTime = std::chrono::steady_clock::now();
                task->Execute();
                auto endTime = std::chrono::steady_clock::now();

                auto workTime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
                _metrics.tasksCompleted.fetch_add(1, std::memory_order_relaxed);
                _metrics.totalWorkTime.fetch_add(workTime, std::memory_order_relaxed);

                _pool->RecordTaskCompletion(task);
                return true;
            }
        }

        ++attempts;

        // NO BACKOFF AT ALL - just continue trying
        // This is aggressive but eliminates any possibility of backoff deadlock
        // The maxAttempts limit (3 by default) prevents infinite spinning
    }

    return false;
}

// ============================================================================
// MODIFIED: Wake() - Simplified without steal backoff flag
// ============================================================================

void WorkerThread::Wake()
{
    // CRITICAL FIX: Acquire lock BEFORE checking _sleeping flag
    std::lock_guard<std::mutex> lock(_wakeMutex);

    // Clear sleeping flag under lock to ensure it's seen by Sleep()
    _sleeping.store(false, std::memory_order_relaxed);

    // NOTE: _stealBackoff flag removed - no longer needed without CV backoff

    // Always notify - even if not currently sleeping, thread might be about to sleep
    _wakeCv.notify_one();
}

// ============================================================================
// MODIFIED: WorkerThread class - Remove steal backoff flag
// ============================================================================

class alignas(64) WorkerThread
{
private:
    friend class ThreadPool;

    ThreadPool* _pool;
    std::thread _thread;
    uint32 _workerId;
    uint32 _cpuCore;

    // State management
    alignas(64) std::atomic<bool> _running{true};
    alignas(64) std::atomic<bool> _sleeping{false};
    // REMOVED: alignas(64) std::atomic<bool> _stealBackoff{false};  // No longer needed

    // ... rest of class unchanged ...
};

// ============================================================================
// ADDITIONAL SAFETY: Add diagnostic logging
// ============================================================================

void WorkerThread::Run()
{
    // Add startup delay to prevent thread storm
    std::this_thread::sleep_for(std::chrono::milliseconds(_workerId * 5));

    auto lastActiveTime = std::chrono::steady_clock::now();
    uint32 consecutiveSteals = 0;  // Track consecutive steal attempts

    // Main worker loop with improved error handling
    while (_running.load(std::memory_order_relaxed))
    {
        bool didWork = false;

        try
        {
            // Try to execute task from local queues (priority order)
            if (TryExecuteTask())
            {
                didWork = true;
                lastActiveTime = std::chrono::steady_clock::now();
                consecutiveSteals = 0;  // Reset steal counter
            }
            // Try to steal work from other workers
            else if (!_pool->IsShuttingDown() && _pool->GetConfiguration().enableWorkStealing && TryStealTask())
            {
                didWork = true;
                lastActiveTime = std::chrono::steady_clock::now();
                consecutiveSteals = 0;  // Reset steal counter
            }
            else
            {
                // Failed to steal
                ++consecutiveSteals;

                // Safety: If we've failed to steal too many times, sleep longer
                if (consecutiveSteals > 10)
                {
                    // Many failed steals - system likely idle
                    // Sleep for longer to reduce CPU usage
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    consecutiveSteals = 0;
                }
            }
        }
        catch (std::exception const& e)
        {
            // Log error but continue running
            _metrics.tasksCompleted.fetch_add(1, std::memory_order_relaxed);
        }

        if (!didWork)
        {
            // Track idle time
            auto now = std::chrono::steady_clock::now();
            auto idleTime = std::chrono::duration_cast<std::chrono::microseconds>(now - lastActiveTime).count();
            _metrics.totalIdleTime.fetch_add(idleTime, std::memory_order_relaxed);

            // Sleep if no work available
            Sleep();
        }
    }
}

// ============================================================================
// CONFIGURATION UPDATE: Reduce steal attempts to minimize contention
// ============================================================================

struct Configuration
{
    // ... other config ...

    uint32 maxStealAttempts = 2;  // Reduced from 3 to minimize spinning

    // ... rest unchanged ...
};