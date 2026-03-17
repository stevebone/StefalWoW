/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * Phase 5: Performance Optimization - ThreadPool Implementation
 */

#include "ThreadPool.h"
#include "DeadlockDetector.h"
#include "../../Config/PlayerbotConfig.h"
#include "Log.h"

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <pthread.h>
#include <mach/thread_policy.h>
#include <mach/thread_act.h>
#else  // Linux
#include <pthread.h>
#include <sched.h>
#endif

#include <algorithm>
#include <random>

namespace Playerbot {
namespace Performance {

// ============================================================================
// WorkStealingQueue Implementation
// ============================================================================

template<typename T>
WorkStealingQueue<T>::WorkStealingQueue()
    : _array(::std::make_unique<Node[]>(INITIAL_CAPACITY))
{
}

template<typename T>
bool WorkStealingQueue<T>::Push(T item)
{
    int64_t bottom = _bottom.load(::std::memory_order_relaxed);
    int64_t top = _top.load(::std::memory_order_acquire);

    // Check if queue is full
    if (bottom - top >= static_cast<int64_t>(_capacity.load(::std::memory_order_relaxed)))
    {
        // Try to expand
    if (_capacity.load(::std::memory_order_relaxed) < MAX_CAPACITY)
        {
            Expand();
        }
        else
        {
            return false; // Queue full, cannot expand further
        }
    }

    // Store item
    size_t index = static_cast<size_t>(bottom) & IndexMask();
    _array[index].data.store(item, ::std::memory_order_relaxed);

    // Ensure item is written before incrementing bottom
    ::std::atomic_thread_fence(::std::memory_order_release);

    _bottom.store(bottom + 1, ::std::memory_order_relaxed);
    return true;
}

template<typename T>
bool WorkStealingQueue<T>::Pop(T& item)
{
    int64_t bottom = _bottom.load(::std::memory_order_relaxed) - 1;
    _bottom.store(bottom, ::std::memory_order_relaxed);

    ::std::atomic_thread_fence(::std::memory_order_seq_cst);

    int64_t top = _top.load(::std::memory_order_relaxed);

    if (top <= bottom)
    {
        // Non-empty queue
        size_t index = static_cast<size_t>(bottom) & IndexMask();
        item = _array[index].data.load(::std::memory_order_relaxed);

        if (top == bottom)
        {
            // Last item - compete with steal
    if (!_top.compare_exchange_strong(top, top + 1,
                ::std::memory_order_seq_cst,
                ::std::memory_order_relaxed))
            {
                // Lost race with steal
                item = nullptr;
                _bottom.store(bottom + 1, ::std::memory_order_relaxed);
                return false;
            }
            _bottom.store(bottom + 1, ::std::memory_order_relaxed);
        }
        return true;
    }
    else
    {
        // Empty queue
        _bottom.store(bottom + 1, ::std::memory_order_relaxed);
        return false;
    }
}

template<typename T>
bool WorkStealingQueue<T>::Steal(T& item)
{
    int64_t top = _top.load(::std::memory_order_acquire);
    ::std::atomic_thread_fence(::std::memory_order_seq_cst);
    int64_t bottom = _bottom.load(::std::memory_order_acquire);

    if (top < bottom)
    {
        // Non-empty queue
        size_t index = static_cast<size_t>(top) & IndexMask();
        item = _array[index].data.load(::std::memory_order_relaxed);

        // Try to increment top
    if (!_top.compare_exchange_strong(top, top + 1,
            ::std::memory_order_seq_cst,
            ::std::memory_order_relaxed))
        {
            // Lost race
            return false;
        }

        return true;
    }

    return false; // Empty queue
}

template<typename T>
void WorkStealingQueue<T>::Expand()
{
    ::std::lock_guard lock(_expansionMutex);

    size_t oldCapacity = _capacity.load(::std::memory_order_relaxed);
    size_t newCapacity = ::std::min(oldCapacity * 2, MAX_CAPACITY);

    if (newCapacity == oldCapacity)
        return; // Already at max

    auto newArray = ::std::make_unique<Node[]>(newCapacity);

    // Copy existing items
    int64_t bottom = _bottom.load(::std::memory_order_relaxed);
    int64_t top = _top.load(::std::memory_order_relaxed);

    for (int64_t i = top; i < bottom; ++i)
    {
        size_t oldIndex = static_cast<size_t>(i) & (oldCapacity - 1);
        size_t newIndex = static_cast<size_t>(i) & (newCapacity - 1);
        newArray[newIndex].data.store(
            _array[oldIndex].data.load(::std::memory_order_relaxed),
            ::std::memory_order_relaxed);
    }

    _array = ::std::move(newArray);
    _capacity.store(newCapacity, ::std::memory_order_release);
}

// Explicit template instantiation
template class WorkStealingQueue<Task*>;

// ============================================================================
// WorkerThread Implementation
// ============================================================================

WorkerThread::WorkerThread(ThreadPool* pool, uint32 workerId, uint32 cpuCore)
    : _pool(pool)
    , _workerId(workerId)
    , _cpuCore(cpuCore)
{
    // CRITICAL FIX: Thread is NOT started in constructor to prevent 60s hang
    // Thread will be started explicitly by calling Start() after all workers are constructed
    // This prevents race conditions and thread startup storms during initialization

    // Initialize diagnostics if enabled
    InitializeDiagnostics();
}

WorkerThread::~WorkerThread()
{
    if (_thread.joinable())
    {
        Shutdown();
        _thread.join();
    }
}

void WorkerThread::Start()
{
    // CRITICAL FIX: Deferred thread start with proper initialization
    // This method is called AFTER all workers are constructed in ThreadPool
    // Prevents race conditions and startup storms
    if (_initialized.exchange(true))
        return; // Already started

    // Start thread with error handling
    try
    {
        _thread = ::std::thread(&WorkerThread::Run, this);

        // Optional: Set thread name for debugging (platform-specific)
#ifdef _WIN32
        if (_thread.joinable())
        {
            ::std::string threadName = "PlayerBot-Worker-" + ::std::to_string(_workerId);
            // Windows thread naming requires special handling
        }
#endif
    }
    catch (::std::exception const& e)
    {
        _initialized.store(false);
        throw ::std::runtime_error(::std::string("Failed to start worker thread ") + ::std::to_string(_workerId) + ": " + e.what());
    }
}

void WorkerThread::Run()
{
    // Set initial state
    if (_diagnostics)
        WORKER_SET_STATE(_diagnostics, INITIALIZING);

    // CRITICAL FIX: Add small startup delay to prevent thread storm
    // Stagger thread startup by worker ID to reduce contention
    ::std::this_thread::sleep_for(::std::chrono::milliseconds(_workerId * 5));

    if (_diagnostics)
        WORKER_SET_STATE(_diagnostics, CHECKING_QUEUES);

    auto lastActiveTime = ::std::chrono::steady_clock::now();

    // Main worker loop with improved error handling
    while (_running.load(::std::memory_order_relaxed))
    {
        bool didWork = false;

        try
        {
            // Try to execute task from local queues (priority order)
    if (TryExecuteTask())
            {
                didWork = true;
                lastActiveTime = ::std::chrono::steady_clock::now();
            }
            // Try to steal work from other workers (check pool not shutting down first)
            else if (!_pool->IsShuttingDown() && _pool->GetConfiguration().enableWorkStealing)
            {
                if (_diagnostics)
                    WORKER_SET_STATE(_diagnostics, STEALING);

                if (TryStealTask())
                {
                    didWork = true;
                    lastActiveTime = ::std::chrono::steady_clock::now();
                }
            }
        }
        catch (::std::exception const& e)
        {
            // Log error but continue running
            // NOTE: Cannot use TC_LOG here as it might not be initialized
            // Error will be recorded in metrics instead
            _metrics.tasksCompleted.fetch_add(1, ::std::memory_order_relaxed); // Count as completed but failed

            // CRITICAL FIX: Also update POOL counter to maintain in-flight balance!
            // If a task was popped but an exception occurred before RecordTaskCompletion,
            // the pool's totalCompleted would never be updated, causing GetInFlightTasks()
            // to return a permanently inflated value (leading to "1 in-flight, 0 active workers")
            // CRITICAL: Use release ordering to synchronize with acquire loads in WaitForCompletion
            _pool->_metrics.totalCompleted.fetch_add(1, ::std::memory_order_release);

            if (_diagnostics)
            {
                _diagnostics->tasksFailed.fetch_add(1, ::std::memory_order_relaxed);
            }
        }

        if (!didWork)
        {
            // Track idle time
            auto now = ::std::chrono::steady_clock::now();
            auto idleTime = ::std::chrono::duration_cast<::std::chrono::microseconds>(now - lastActiveTime).count();
            _metrics.totalIdleTime.fetch_add(idleTime, ::std::memory_order_relaxed);

            // Sleep if no work available
            Sleep();

            // After waking, go back to checking queues
    if (_diagnostics)
                WORKER_SET_STATE(_diagnostics, CHECKING_QUEUES);
        }
    }

    // Shutting down
    if (_diagnostics)
        WORKER_SET_STATE(_diagnostics, SHUTTING_DOWN);
}

bool WorkerThread::TryExecuteTask()
{
    // Try each priority level in order
    for (size_t i = 0; i < static_cast<size_t>(TaskPriority::COUNT); ++i)
    {
        Task* task = nullptr;
        if (_localQueues[i].Pop(task) && task)
        {
            if (_diagnostics)
                WORKER_SET_STATE(_diagnostics, EXECUTING);

            auto startTime = ::std::chrono::steady_clock::now();

            // CRITICAL FIX: Wrap task execution in try-catch to prevent std::terminate()
            // If a task throws an exception and it's not caught, it will call std::terminate()
            // which crashes the entire application. We catch all exceptions here to log them
            // and continue processing other tasks.
            bool taskSucceeded = true;
            try
            {
                // Execute task
                task->Execute();
            }
            catch (::std::exception const& e)
            {
                taskSucceeded = false;
                // Log the exception but don't crash - continue with other tasks
                // Note: We can't use TC_LOG here safely as it might throw too
                // Instead, just increment a failure counter
                _metrics.tasksFailed.fetch_add(1, ::std::memory_order_relaxed);

                // Try to log if possible (may fail during shutdown)
                try
                {
                    TC_LOG_ERROR("module.playerbot.threadpool",
                        "Worker {} caught exception during task execution: {}",
                        _workerId, e.what());
                }
                catch (...) { /* Ignore logging failures */ }
            }
            catch (...)
            {
                taskSucceeded = false;
                _metrics.tasksFailed.fetch_add(1, ::std::memory_order_relaxed);

                try
                {
                    TC_LOG_ERROR("module.playerbot.threadpool",
                        "Worker {} caught unknown exception during task execution", _workerId);
                }
                catch (...) { /* Ignore logging failures */ }
            }

            auto endTime = ::std::chrono::steady_clock::now();
            auto workTime = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime);

            // Update metrics (regardless of success/failure)
            _metrics.tasksCompleted.fetch_add(1, ::std::memory_order_relaxed);
            _metrics.totalWorkTime.fetch_add(workTime.count(), ::std::memory_order_relaxed);

            // Update diagnostics
            if (_diagnostics)
            {
                _diagnostics->tasksExecuted.fetch_add(1, ::std::memory_order_relaxed);
                _diagnostics->executionTime.Record(workTime);

                // Record queue wait time if available
                auto queueTime = ::std::chrono::duration_cast<::std::chrono::microseconds>(
                    task->startedAt - task->submittedAt);
                _diagnostics->queueWaitTime.Record(queueTime);

                // Record total latency
                auto totalLatency = ::std::chrono::duration_cast<::std::chrono::microseconds>(
                    task->completedAt - task->submittedAt);
                _diagnostics->taskLatency.Record(totalLatency);

                WORKER_SET_STATE(_diagnostics, CHECKING_QUEUES);
            }

            // Notify pool (must happen even for failed tasks to maintain counter balance)
            _pool->RecordTaskCompletion(task);

            return true;
        }
    }

    return false;
}

bool WorkerThread::TryStealTask()
{
    uint32 attempts = 0;
    uint32 maxAttempts = _pool->GetConfiguration().maxStealAttempts;
    uint32 yieldsPerAttempt = 1;  // Start with 1 yield
    while (attempts < maxAttempts)
    {
        _metrics.stealAttempts.fetch_add(1, ::std::memory_order_relaxed);

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
    if (victim->_sleeping.load(::std::memory_order_relaxed))
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
                _metrics.stealSuccesses.fetch_add(1, ::std::memory_order_relaxed);

                auto startTime = ::std::chrono::steady_clock::now();

                // CRITICAL FIX: Wrap stolen task execution in try-catch (same as TryExecuteTask)
                try
                {
                    // Execute stolen task
                    task->Execute();
                }
                catch (::std::exception const& e)
                {
                    _metrics.tasksFailed.fetch_add(1, ::std::memory_order_relaxed);
                    try
                    {
                        TC_LOG_ERROR("module.playerbot.threadpool",
                            "Worker {} caught exception during stolen task execution: {}",
                            _workerId, e.what());
                    }
                    catch (...) { /* Ignore logging failures */ }
                }
                catch (...)
                {
                    _metrics.tasksFailed.fetch_add(1, ::std::memory_order_relaxed);
                    try
                    {
                        TC_LOG_ERROR("module.playerbot.threadpool",
                            "Worker {} caught unknown exception during stolen task execution", _workerId);
                    }
                    catch (...) { /* Ignore logging failures */ }
                }

                auto endTime = ::std::chrono::steady_clock::now();
                auto workTime = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime).count();

                _metrics.tasksCompleted.fetch_add(1, ::std::memory_order_relaxed);
                _metrics.totalWorkTime.fetch_add(workTime, ::std::memory_order_relaxed);

                _pool->RecordTaskCompletion(task);

                return true;
            }
        }

        ++attempts;

        // CRITICAL FIX D: Replace CV wait with yield-based backoff
        // This prevents deadlock when all threads enter backoff simultaneously
        // Using yield instead of condition variable eliminates the 1ms CV wait deadlock
    if (attempts < maxAttempts)
        {
            // Progressive yield strategy - more yields on repeated failures
    for (uint32 y = 0; y < yieldsPerAttempt; ++y)
            {
                // Check for shutdown or new work before each yield
    if (!_running.load(::std::memory_order_relaxed) ||
                    _pool->IsShuttingDown() ||
                    !_localQueues[0].Empty())  // Check CRITICAL queue for urgent work
                {
                    return false;  // Exit early if shutdown or urgent work
                }

                // Yield CPU to other threads
                ::std::this_thread::yield();
            }

            // Exponentially increase yields (cap at 8 to prevent excessive spinning)
            yieldsPerAttempt = ::std::min(yieldsPerAttempt * 2, 8u);
        }
    }

    return false;
}

bool WorkerThread::SubmitLocal(Task* task, TaskPriority priority)
{
    size_t index = static_cast<size_t>(priority);
    return _localQueues[index].Push(task);
}

void WorkerThread::Wake()
{
    // CRITICAL FIX B: Acquire lock BEFORE checking _sleeping flag
    // This prevents lost wake signals when Sleep() sets flag after Wake() checks
    ::std::lock_guard<::std::mutex> lock(_wakeMutex);

    // Clear sleeping flag under lock to ensure it's seen by Sleep()
    _sleeping.store(false, ::std::memory_order_relaxed);
    // NOTE: _stealBackoff removed in FIX D - no longer using CV-based backoff

    // Always notify - even if not currently sleeping, thread might be about to sleep
    _wakeCv.notify_one();
}

void WorkerThread::Shutdown()
{
    _running.store(false, ::std::memory_order_relaxed);
    Wake();
}

void WorkerThread::SetAffinity()
{
#ifdef _WIN32
    HANDLE thread = _thread.native_handle();
    DWORD_PTR mask = 1ULL << _cpuCore;
    SetThreadAffinityMask(thread, mask);
#elif defined(__APPLE__)
    // macOS uses thread affinity tags (not strict CPU binding)
    thread_affinity_policy_data_t policy = { static_cast<integer_t>(_cpuCore) };
    thread_policy_set(pthread_mach_thread_np(_thread.native_handle()),
                      THREAD_AFFINITY_POLICY,
                      reinterpret_cast<thread_policy_t>(&policy),
                      THREAD_AFFINITY_POLICY_COUNT);
#else  // Linux
    pthread_t thread = _thread.native_handle();
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(_cpuCore, &cpuset);
    pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
#endif
}

bool WorkerThread::HasWorkAvailable() const
{
    // Check own queues first (fast path)
    for (size_t i = 0; i < static_cast<size_t>(TaskPriority::COUNT); ++i)
    {
        if (!_localQueues[i].Empty())
            return true;
    }

    // Check if work stealing is enabled and other workers have work
    if (_pool->GetConfiguration().enableWorkStealing)
    {
        for (uint32 i = 0; i < _pool->GetWorkerCount(); ++i)
        {
            if (i == _workerId)
                continue;  // Skip self

            WorkerThread* other = _pool->GetWorker(i);
            if (!other)
                continue;

            // Check if other worker has stealable work
    for (size_t j = 0; j < static_cast<size_t>(TaskPriority::COUNT); ++j)
            {
                if (!other->_localQueues[j].Empty())
                    return true;  // Work available to steal
            }
        }
    }

    return false;  // No work available anywhere
}

void WorkerThread::Sleep()
{
    // CRITICAL FIX: Add safety check to prevent blocking during shutdown
    if (!_running.load(::std::memory_order_relaxed) || _pool->IsShuttingDown())
        return;

    // CRITICAL FIX: Acquire lock BEFORE setting sleeping flag
    // This ensures Wake() can't check the flag between setting it and waiting
    ::std::unique_lock<::std::mutex> lock(_wakeMutex);

    // Set sleeping flag AFTER acquiring lock
    _sleeping.store(true, ::std::memory_order_relaxed);

    // CRITICAL FIX #3: Use comprehensive work detection
    // Check both local queues and stealable work from other workers
    bool hasWork = HasWorkAvailable();

    if (hasWork)
    {
        // Work available, don't sleep
        _sleeping.store(false, ::std::memory_order_relaxed);
        return;
    }

    // Wait with timeout, check multiple conditions for wake-up
    // CRITICAL FIX: Check _sleeping flag in predicate (cleared by Wake())
    _wakeCv.wait_for(lock, _pool->GetConfiguration().workerSleepTime, [this]()
    {
        // Wake conditions:
        // 1. _sleeping cleared by Wake() (new work submitted)
        // 2. Thread shutdown requested
        // 3. Pool shutting down
        return !_sleeping.load(::std::memory_order_relaxed) ||
               !_running.load(::std::memory_order_relaxed) ||
               _pool->IsShuttingDown();
    });

    _sleeping.store(false, ::std::memory_order_relaxed);
}

uint32 WorkerThread::GetRandomWorkerIndex() const
{
    static thread_local ::std::mt19937 rng(::std::random_device{}());
    ::std::uniform_int_distribution<uint32> dist(0, _pool->GetWorkerCount() - 1);
    return dist(rng);
}

void WorkerThread::InitializeDiagnostics()
{
    if (_pool && _pool->IsDiagnosticsEnabled())
    {
        _diagnostics = ::std::make_unique<WorkerDiagnostics>();
        _diagnostics->SetState(WorkerState::UNINITIALIZED, "WorkerThread::WorkerThread");
    }
}

// ============================================================================
// ThreadPool Implementation
// ============================================================================

ThreadPool::ThreadPool()
    : ThreadPool(Configuration())
{
}

ThreadPool::ThreadPool(Configuration config)
    : _config(config)
{
    // CRITICAL VALIDATION: Ensure configuration is valid
    // NOTE: Cannot log here - constructor may be called before logging system is ready
    if (_config.numThreads < 1)
    {
        _config.numThreads = 1;  // Silent fix - any logging will happen in EnsureWorkersCreated()
    }

    // CRITICAL FIX: Don't create workers here anymore - they are created lazily on first Submit()
    // This prevents worker threads from starting before World is fully initialized
    _workers.reserve(_config.numThreads);
    // Workers will be created by EnsureWorkersCreated() on first Submit() call
}

ThreadPool::~ThreadPool()
{
    Shutdown(true);
}

void ThreadPool::EnsureWorkersCreated()
{
    // Fast path: Already created (no locking needed)
    if (_workersCreated.load(::std::memory_order_acquire))
        return;

    // Slow path: Need to create workers (with locking for thread-safety)
    ::std::lock_guard lock(_workerCreationMutex);

    // Double-check after acquiring lock (another thread may have created workers)
    if (_workersCreated.load(::std::memory_order_relaxed))
        return;

    // CRITICAL VALIDATION: Runtime check before creating workers
    // NOTE: Cannot log here either - playerbot.performance logger may not be ready yet
    if (_config.numThreads < 4)
    {
        // Silent early return to prevent crash or deadlock from too few workers
        // If this happens, validation in GetThreadPool() and constructor failed
        // Minimum 4 threads required to prevent main thread deadlock
        return;
    }

    // CRITICAL FIX: Two-phase worker initialization to prevent 60s hang
    // Phase 1: Create all WorkerThread objects (no threads started yet)
    // Phase 2: Start all threads in a staggered manner

    try
    {
        // Phase 1: Create worker objects without starting threads
        _workers.reserve(_config.numThreads);
        for (uint32 i = 0; i < _config.numThreads; ++i)
        {
            uint32 cpuCore = i % ::std::thread::hardware_concurrency();
            _workers.push_back(::std::make_unique<WorkerThread>(this, i, cpuCore));
        }

        // Mark as created BEFORE starting threads
        // This ensures Submit() can proceed even if thread startup is slow
        _workersCreated.store(true, ::std::memory_order_release);

        // Phase 2: Start all worker threads with staggered startup
        // This prevents thread startup storm and reduces initialization time
    for (uint32 i = 0; i < _config.numThreads; ++i)
        {
            try
            {
                _workers[i]->Start();

                // Small delay between thread starts to prevent OS scheduler contention
                // Total delay: ~40ms for 8 threads (5ms each)
    if (i < _config.numThreads - 1)
                    ::std::this_thread::sleep_for(::std::chrono::milliseconds(5));
            }
            catch (::std::exception const& e)
            {
                // Thread start failed, but continue with other threads
                // The pool can still function with reduced worker count
                // Error will be visible in reduced GetActiveThreads() count
            }
        }

        // Optional: Try to log success if logger is ready
        // Use try-catch to prevent crash if logger not initialized
        try
        {
            uint32 hwCores = ::std::thread::hardware_concurrency();
            TC_LOG_INFO("playerbot.performance",
                "ThreadPool: Created and started {} worker threads (CPU: {} logical cores detected)",
                _config.numThreads, hwCores);
        }
        catch (...)
        {
            // Logger not ready, silently continue
        }
    }
    catch (::std::exception const& e)
    {
        // Critical failure - mark as not created
        _workersCreated.store(false, ::std::memory_order_release);
        _workers.clear();
        throw; // Re-throw to propagate error
    }
}

// Note: Submit() template implementation moved to header for lambda support

bool ThreadPool::WaitForCompletion(::std::chrono::milliseconds timeout)
{
    auto start = ::std::chrono::steady_clock::now();

    // CRITICAL SAFETY: Hard cap at 15 seconds to prevent FreezeDetector crash (60s)
    // Even if caller passes a larger timeout, we refuse to wait longer
    constexpr auto HARD_MAX_TIMEOUT = ::std::chrono::milliseconds(15000);
    auto effectiveTimeout = ::std::min(timeout, HARD_MAX_TIMEOUT);

    while (true)
    {
        // Check if all queues are empty
        bool allEmpty = true;
        for (const auto& worker : _workers)
        {
            for (size_t i = 0; i < static_cast<size_t>(TaskPriority::COUNT); ++i)
            {
                if (!worker->_localQueues[i].Empty())
                {
                    allEmpty = false;
                    break;
                }
            }
            if (!allEmpty)
                break;
        }

        // CRITICAL FIX (Map.cpp:1973 crash): Also check that all tasks have FINISHED executing!
        // Problem: A task can be dequeued (queue empty) but still EXECUTING on a worker thread.
        //          If we return early, the caller may destroy objects that the executing task
        //          is still accessing, causing use-after-free crashes in Map::SendObjectUpdates.
        //
        // Solution: Wait until totalSubmitted == totalCompleted, meaning all submitted tasks
        //           have finished their Execute() call, not just been dequeued.
        uint64 submitted = _metrics.totalSubmitted.load(::std::memory_order_acquire);
        uint64 completed = _metrics.totalCompleted.load(::std::memory_order_acquire);
        bool allTasksFinished = (submitted == completed);

        if (allEmpty && allTasksFinished)
            return true;

        // COUNTER MISMATCH DETECTION: If all queues are empty and no workers are active,
        // but counters show in-flight tasks, we have a "ghost counter" issue.
        // This can happen if an exception occurs during task submission or execution
        // that doesn't properly update the completed counter.
        if (allEmpty && GetActiveThreads() == 0 && submitted > completed)
        {
            uint64 inFlight = submitted - completed;
            TC_LOG_WARN("module.playerbot.threadpool",
                "COUNTER MISMATCH DETECTED: {} in-flight tasks but all queues empty and 0 active workers. "
                "Correcting counters to prevent permanent timeout.",
                inFlight);

            // Correct the mismatch by advancing totalCompleted
            // CRITICAL: Use release ordering to synchronize with acquire loads in WaitForCompletion
            _metrics.totalCompleted.fetch_add(inFlight, ::std::memory_order_release);
            return true;  // All work is actually done
        }

        // Check timeout - use effective timeout with hard cap
        auto now = ::std::chrono::steady_clock::now();
        if (::std::chrono::duration_cast<::std::chrono::milliseconds>(now - start) >= effectiveTimeout)
            return false;

        ::std::this_thread::sleep_for(::std::chrono::milliseconds(10));
    }
}

void ThreadPool::Shutdown(bool waitForPending)
{
    if (_shutdown.exchange(true))
        return; // Already shutting down

    // NOTE: Do NOT log here! During static destruction (atexit), the Log singleton
    // may already be destroyed. Accessing it causes ACCESS_VIOLATION because the
    // internal unordered_map has been destructed. This is the C++ "static destruction
    // order fiasco" - we cannot control which static objects are destroyed first.
    // The ThreadPool destructor is called via atexit when g_threadPool is destroyed,
    // which may happen AFTER Log::instance() has been destroyed.

    if (waitForPending && _workersCreated.load(::std::memory_order_relaxed))
    {
        WaitForCompletion(_config.shutdownTimeout);
    }

    // Stop all workers (safe even if not all threads started)
    for (auto& worker : _workers)
    {
        if (worker)
        {
            worker->Shutdown();
        }
    }

    // Wait for threads to finish (check if thread was actually started)
    for (auto& worker : _workers)
    {
        if (worker && worker->_initialized.load(::std::memory_order_relaxed))
        {
            if (worker->_thread.joinable())
            {
                // Give thread time to finish gracefully
    if (worker->_thread.joinable())
                {
                    worker->_thread.join();
                }
            }
        }
    }

    _workers.clear();
    _workersCreated.store(false, ::std::memory_order_relaxed);

    // NOTE: Do NOT log here! Same reason as above - Log singleton may be destroyed.
}

size_t ThreadPool::GetActiveThreads() const
{
    size_t active = 0;
    for (const auto& worker : _workers)
    {
        if (!worker->_sleeping.load(::std::memory_order_relaxed))
        {
            ++active;
        }
    }
    return active;
}

size_t ThreadPool::GetQueuedTasks() const
{
    size_t total = 0;
    for (const auto& worker : _workers)
    {
        for (size_t i = 0; i < static_cast<size_t>(TaskPriority::COUNT); ++i)
        {
            total += worker->_localQueues[i].Size();
        }
    }
    return total;
}

size_t ThreadPool::GetQueuedTasks(TaskPriority priority) const
{
    size_t total = 0;
    size_t index = static_cast<size_t>(priority);
    for (const auto& worker : _workers)
    {
        total += worker->_localQueues[index].Size();
    }
    return total;
}

size_t ThreadPool::GetInFlightTasks() const
{
    // In-flight tasks = submitted - completed
    // These are tasks that have been dequeued but are still executing
    uint64 submitted = _metrics.totalSubmitted.load(::std::memory_order_acquire);
    uint64 completed = _metrics.totalCompleted.load(::std::memory_order_acquire);
    return static_cast<size_t>(submitted > completed ? submitted - completed : 0);
}

bool ThreadPool::HasPendingWork() const
{
    // Check both queued tasks AND in-flight tasks
    // This is the safe way to check if any work is still pending
    return GetQueuedTasks() > 0 || GetInFlightTasks() > 0;
}

::std::chrono::microseconds ThreadPool::GetAverageLatency() const
{
    uint64 completed = _metrics.totalCompleted.load(::std::memory_order_relaxed);
    if (completed == 0)
        return ::std::chrono::microseconds(0);

    uint64 totalLatency = _metrics.totalLatency.load(::std::memory_order_relaxed);
    return ::std::chrono::microseconds(totalLatency / completed);
}

double ThreadPool::GetThroughput() const
{
    uint64 completed = _metrics.totalCompleted.load(::std::memory_order_relaxed);

    // Calculate throughput over last second
    // This is a simplified calculation - in production, you'd want a sliding window
    return static_cast<double>(completed);
}

void ThreadPool::WakeAllWorkers()
{
    // CRITICAL FIX: Wake ALL workers to prevent batch-submission deadlock
    //
    // Problem: When submitting many tasks rapidly (e.g., 100+ bot updates):
    // 1. Submit task 1 → Wake worker 1
    // 2. Submit task 2 → Wake worker 1 (round-robin)
    // 3. Worker 1 completes task 1 → goes back to Sleep()
    // 4. Submit task 50 → Wake worker 2
    // 5. Main thread calls future.get() → BLOCKS
    // 6. DEADLOCK: Workers sleeping with tasks still in queues
    //
    // Solution: After batch submission, wake ALL workers before future.get()
    // This ensures maximum parallelism and prevents the race condition
    for (auto& worker : _workers)
    {
        if (worker)
        {
            worker->Wake();
        }
    }

    TC_LOG_TRACE("playerbot.threadpool", "WakeAllWorkers called - woke {} workers", _workers.size());
}

uint32 ThreadPool::SelectWorkerRoundRobin()
{
    uint32 next = _nextWorker.fetch_add(1, ::std::memory_order_relaxed);
    return next % _config.numThreads;
}

uint32 ThreadPool::SelectWorkerLeastLoaded()
{
    uint32 leastLoaded = 0;
    size_t minQueueSize = SIZE_MAX;

    for (uint32 i = 0; i < _config.numThreads; ++i)
    {
        size_t queueSize = 0;
        for (size_t j = 0; j < static_cast<size_t>(TaskPriority::COUNT); ++j)
        {
            queueSize += _workers[i]->_localQueues[j].Size();
        }

        if (queueSize < minQueueSize)
        {
            minQueueSize = queueSize;
            leastLoaded = i;
        }
    }

    return leastLoaded;
}

void ThreadPool::RecordTaskCompletion(Task* task)
{
    auto completionTime = ::std::chrono::steady_clock::now();
    auto latency = ::std::chrono::duration_cast<::std::chrono::microseconds>(
        completionTime - task->submittedAt).count();

    // CRITICAL: Use release ordering to synchronize with acquire loads in WaitForCompletion
    _metrics.totalCompleted.fetch_add(1, ::std::memory_order_release);
    _metrics.totalLatency.fetch_add(latency, ::std::memory_order_relaxed);

    // Clean up task
    delete task;
}

// Global instance
static ::std::unique_ptr<ThreadPool> g_threadPool;
static ::std::recursive_mutex g_threadPoolMutex;

ThreadPool& GetThreadPool()
{
    ::std::lock_guard lock(g_threadPoolMutex);
    if (!g_threadPool)
    {
        ThreadPool::Configuration config;

        // Load configuration from playerbots.conf
        // CRITICAL FIX: Don't try to load config if PlayerbotConfig isn't initialized yet
        // Just use default configuration - config will be reloaded later if needed
        try
        {
            if (PlayerbotConfig::instance())
            {
                uint32 configThreads = PlayerbotConfig::instance()->GetUInt(
                    "Playerbot.Performance.ThreadPool.WorkerCount",
                    0);  // 0 means auto-detect

                // Handle special case: 0 = auto-detect (keep default formula)
                // Non-zero values override the default
    if (configThreads > 0)
                {
                    config.numThreads = configThreads;
                }
                // else: keep default from Configuration struct (auto-calculated)

                config.maxQueueSize = PlayerbotConfig::instance()->GetUInt(
                    "Playerbot.Performance.ThreadPool.MaxQueueSize",
                    config.maxQueueSize);
                config.enableWorkStealing = PlayerbotConfig::instance()->GetBool(
                    "Playerbot.Performance.ThreadPool.EnableWorkStealing",
                    config.enableWorkStealing);
                config.enableCpuAffinity = PlayerbotConfig::instance()->GetBool(
                    "Playerbot.Performance.ThreadPool.EnableCpuAffinity",
                    config.enableCpuAffinity);
            }
        }
        catch (...)
        {
            // Config not ready yet, use defaults
        }

        // CRITICAL VALIDATION: Ensure sufficient worker threads to prevent task starvation
        // With blocking future.get() pattern in BotWorldSessionMgr, each bot update blocks a worker.
        // If we have N bots updating simultaneously, we need AT LEAST N+2 worker threads
        // (N for bot updates, +2 for overhead/work stealing).
        //
        // However, hardware_concurrency() - 2 should typically provide enough threads.
        // Minimum is 16 to handle reasonable bot counts without starvation.
        //
        // REAL FIX NEEDED: Make botSession->Update() non-blocking or use a different threading model
    if (config.numThreads < 16)
        {
            config.numThreads = 16;  // Increased minimum from 4 to 16 to prevent task starvation
        }

        g_threadPool = ::std::make_unique<ThreadPool>(config);

        // CRITICAL FIX: Don't log here - GetThreadPool() may be called before logging system is ready
        // Worker creation logging happens in EnsureWorkersCreated() when workers are actually created
    }
    return *g_threadPool;
}

} // namespace Performance
} // namespace Playerbot
