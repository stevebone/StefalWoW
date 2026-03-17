/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 */

#include "AsyncBotInitializer.h"
#include "AI/BotAI.h"
#include "Core/Managers/LazyManagerFactory.h"
#include "Core/Events/BatchedEventSubscriber.h"
#include "Core/Events/EventDispatcher.h"
#include "Player.h"
#include "Log.h"
#include "Timer.h"

namespace Playerbot
{

// ============================================================================
// SINGLETON
// ============================================================================

AsyncBotInitializer& AsyncBotInitializer::Instance()
{
    static AsyncBotInitializer instance;
    return instance;
}

AsyncBotInitializer::AsyncBotInitializer()
{
    TC_LOG_INFO("module.playerbot.async", "AsyncBotInitializer created");
}

AsyncBotInitializer::~AsyncBotInitializer()
{
    Shutdown();
    TC_LOG_INFO("module.playerbot.async", "AsyncBotInitializer destroyed");
}

// ============================================================================
// INITIALIZATION & SHUTDOWN
// ============================================================================

void AsyncBotInitializer::Initialize(size_t numWorkerThreads)
{
    if (_running.load(::std::memory_order_acquire))
    {
        TC_LOG_WARN("module.playerbot.async", "AsyncBotInitializer already running");
        return;
    }

    _running.store(true, ::std::memory_order_release);
    _shutdown.store(false, ::std::memory_order_release);

    // Start worker threads
    for (size_t i = 0; i < numWorkerThreads; ++i)
    {
        _workerThreads.emplace_back(&AsyncBotInitializer::WorkerThreadMain, this, i);
    }

    TC_LOG_INFO("module.playerbot.async",
                " AsyncBotInitializer started with {} worker threads", numWorkerThreads);
}

void AsyncBotInitializer::Shutdown()
{
    if (!_running.load(::std::memory_order_acquire))
        return;

    TC_LOG_INFO("module.playerbot.async", "Shutting down AsyncBotInitializer...");

    // Signal shutdown
    _shutdown.store(true, ::std::memory_order_release);
    _pendingCV.notify_all();

    // Wait for all workers to finish
    for (auto& thread : _workerThreads)
    {
        if (thread.joinable())
            thread.join();
    }

    _workerThreads.clear();
    _running.store(false, ::std::memory_order_release);

    TC_LOG_INFO("module.playerbot.async", "AsyncBotInitializer shut down successfully");
}

// ============================================================================
// ASYNC INITIALIZATION
// ============================================================================

bool AsyncBotInitializer::InitializeAsync(Player* bot, InitCallback callback)
{
    return InitializeAsync(bot, ::std::move(callback), false);
}

bool AsyncBotInitializer::InitializeAsync(Player* bot, InitCallback callback, bool instanceOnlyMode)
{
    if (!bot || !callback)
    {
        TC_LOG_ERROR("module.playerbot.async", "InitializeAsync called with null parameter");
        return false;
    }

    if (_shutdown.load(::std::memory_order_acquire))
    {
        TC_LOG_ERROR("module.playerbot.async", "Cannot initialize bot - shutting down");
        return false;
    }

    // Check queue size limit
    if (_pendingCount.load(::std::memory_order_acquire) >= MAX_QUEUE_SIZE)
    {
        TC_LOG_ERROR("module.playerbot.async",
                     "Bot initialization queue full ({} pending) - cannot queue {}",
                     _pendingCount.load(), bot->GetName());
        return false;
    }

    // Queue the task
    {
        ::std::lock_guard lock(_pendingMutex);
        _pendingTasks.emplace(bot, ::std::move(callback), instanceOnlyMode);
        _pendingCount.fetch_add(1, ::std::memory_order_relaxed);
    }
    // Wake up a worker thread
    _pendingCV.notify_one();

    TC_LOG_DEBUG("module.playerbot.async",
                 "Bot {} queued for async initialization (queue depth: {}, instanceOnly: {})",
                 bot->GetName(), _pendingCount.load(), instanceOnlyMode);

    return true;
}

// ============================================================================
// PROCESS COMPLETED INITIALIZATIONS
// ============================================================================

size_t AsyncBotInitializer::ProcessCompletedInits(size_t maxToProcess)
{
    size_t processed = 0;

    ::std::lock_guard lock(_completedMutex);

    while (!_completedResults.empty() && processed < maxToProcess)
    {
        InitResult result = ::std::move(_completedResults.front());
        _completedResults.pop();
        _completedCount.fetch_sub(1, ::std::memory_order_relaxed);

        // Invoke callback (on main thread)
        try
        {
            if (result.callback)
            {
                result.callback(result.ai);  // Transfer ownership
                ++processed;

                ::std::lock_guard metricsLock(_metricsMutex);
                ++_metrics.callbacksProcessed;
            }
        }
        catch (::std::exception const& e)
        {
            TC_LOG_ERROR("module.playerbot.async",
                         "Exception in initialization callback for {}: {}",
                         result.bot ? result.bot->GetName() : "Unknown",
                         e.what());

            // Clean up AI if callback failed
            delete result.ai;
        }
    }

    return processed;
}

// ============================================================================
// WORKER THREAD
// ============================================================================

void AsyncBotInitializer::WorkerThreadMain(size_t workerId)
{
    TC_LOG_INFO("module.playerbot.async", "Worker thread {} started", workerId);

    while (!_shutdown.load(::std::memory_order_acquire))
    {
        ::std::unique_lock lock(_pendingMutex);

        // Wait for task or shutdown
        _pendingCV.wait(lock, [this] {
            return !_pendingTasks.empty() || _shutdown.load(::std::memory_order_acquire);
        });

        if (_shutdown.load(::std::memory_order_acquire) && _pendingTasks.empty())
            break;

        if (_pendingTasks.empty())
            continue;
        // Get task
        InitTask task = ::std::move(_pendingTasks.front());
        _pendingTasks.pop();
        _pendingCount.fetch_sub(1, ::std::memory_order_relaxed);
        _inProgressCount.fetch_add(1, ::std::memory_order_relaxed);

        lock.unlock();

        // Process task (heavy work happens here - off main thread!)
        InitResult result = ProcessInitTask(::std::move(task));

        _inProgressCount.fetch_sub(1, ::std::memory_order_relaxed);

        // Queue result for main thread callback
        {
            ::std::lock_guard completedLock(_completedMutex);
            _completedResults.push(::std::move(result));
            _completedCount.fetch_add(1, ::std::memory_order_relaxed);
        }
    }

    TC_LOG_INFO("module.playerbot.async", "Worker thread {} stopped", workerId);
}

AsyncBotInitializer::InitResult AsyncBotInitializer::ProcessInitTask(InitTask task)
{
    auto startTime = ::std::chrono::steady_clock::now();

    TC_LOG_DEBUG("module.playerbot.async",
                 "Worker processing initialization for {} (queued for {}ms)",
                 task.bot->GetName(),
                 ::std::chrono::duration_cast<::std::chrono::milliseconds>(
                     startTime - task.queueTime).count());

    BotAI* ai = nullptr;
    bool success = false;

    try
    {
        ai = CreateBotAI(task.bot, task.instanceOnlyMode);
        success = (ai != nullptr);
    }
    catch (::std::exception const& e)
    {
        TC_LOG_ERROR("module.playerbot.async",
                     "Exception creating BotAI for {}: {}",
                     task.bot->GetName(), e.what());
        success = false;
    }

    auto endTime = ::std::chrono::steady_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(endTime - startTime);

    // Update metrics
    {
        ::std::lock_guard lock(_metricsMutex);
        ++_metrics.totalInits;
        _totalProcessed.fetch_add(1, ::std::memory_order_relaxed);

        if (success)
            ++_metrics.successfulInits;
        else
            ++_metrics.failedInits;
        _metrics.totalTime += duration;

        if (_metrics.totalInits == 1)
            _metrics.minInitTime = _metrics.maxInitTime = duration;
        else
        {
            if (duration < _metrics.minInitTime)
                _metrics.minInitTime = duration;
            if (duration > _metrics.maxInitTime)
                _metrics.maxInitTime = duration;
        }

        _metrics.avgInitTime = ::std::chrono::milliseconds{
            _metrics.totalTime.count() / _metrics.totalInits
        };

        // Track max queue depth
        size_t currentPending = _pendingCount.load(::std::memory_order_relaxed);
        if (currentPending > _metrics.queueDepthMax)
            _metrics.queueDepthMax = currentPending;
    }

    TC_LOG_INFO("module.playerbot.async",
                "{} Bot {} initialization in {}ms",
                success ? "" : "",
                task.bot->GetName(),
                duration.count());

    return InitResult(task.bot, ai, ::std::move(task.callback), duration, success);
}

BotAI* AsyncBotInitializer::CreateBotAI(Player* bot, bool instanceOnlyMode)
{
    // This is the actual heavy work - happens off main thread!
    // Uses LazyManagerFactory so managers created on-demand

    // When instanceOnlyMode is true, the BotAI constructor will skip creating
    // expensive non-essential managers (questing, professions, AH, banking).
    // This significantly reduces CPU overhead for JIT bots created to fill
    // BG/LFG queues that only need combat capabilities.
    BotAI* ai = new BotAI(bot, instanceOnlyMode);

    // Event dispatcher already created in BotAI constructor
    // Managers will be created lazily when first accessed

    return ai;
}

// ============================================================================
// METRICS
// ============================================================================

AsyncBotInitializer::PerformanceMetrics AsyncBotInitializer::GetMetrics() const
{
    ::std::lock_guard lock(_metricsMutex);
    return _metrics;
}

void AsyncBotInitializer::ResetMetrics()
{
    ::std::lock_guard lock(_metricsMutex);
    _metrics = PerformanceMetrics{};
    TC_LOG_INFO("module.playerbot.async", "Performance metrics reset");
}

} // namespace Playerbot
