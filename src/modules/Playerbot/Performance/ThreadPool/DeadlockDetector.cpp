/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * DeadlockDetector Implementation
 */

#include "DeadlockDetector.h"
#include "ThreadPool.h"
#include "DebuggerIntegration.h"
#include "ETWProvider.h"
#include "Log.h"
#include <sstream>
#include <iomanip>
#include <filesystem>

// Undefine Windows macros that conflict with our enums
#ifdef ERROR
#undef ERROR
#endif
#ifdef CRITICAL
#undef CRITICAL
#endif

namespace Playerbot {
namespace Performance {

DeadlockDetector::DeadlockDetector(ThreadPool* pool, DeadlockDetectorConfig config)
    : _pool(pool)
    , _config(::std::move(config))
{
    _stats.startTime = ::std::chrono::steady_clock::now();
    _state.lastCheckTime = ::std::chrono::steady_clock::now();
    _state.lastDumpTime = ::std::chrono::steady_clock::now() - ::std::chrono::hours(1); // Allow immediate dump

    // Create dump directory if needed
    if (_config.enableAutoDump)
    {
        try
        {
            ::std::filesystem::create_directories(_config.dumpDirectory);
        }
        catch (const ::std::exception& e)
        {
            TC_LOG_ERROR("playerbot.threadpool.deadlock",
                "Failed to create dump directory {}: {}",
                _config.dumpDirectory, e.what());
        }
    }
}

DeadlockDetector::~DeadlockDetector()
{
    Stop();
}

void DeadlockDetector::Start()
{
    if (_running.exchange(true))
        return; // Already running

    _detectorThread = ::std::make_unique<::std::thread>(&DeadlockDetector::DetectionLoop, this);

    TC_LOG_INFO("playerbot.threadpool.deadlock",
        "Deadlock detector started with {}ms check interval",
        _config.checkInterval.count());
}

void DeadlockDetector::Stop()
{
    if (!_running.exchange(false))
        return; // Already stopped
    if (_detectorThread && _detectorThread->joinable())
    {
        _detectorThread->join();
        _detectorThread.reset();
    }

    TC_LOG_INFO("playerbot.threadpool.deadlock",
        "Deadlock detector stopped. Detected {} deadlocks, issued {} warnings",
        _stats.deadlocksDetected.load(), _stats.warningsIssued.load());
}

void DeadlockDetector::DetectionLoop()
{
    while (_running.load(::std::memory_order_relaxed))
    {
        // Sleep for check interval
        ::std::this_thread::sleep_for(_config.checkInterval);

        if (_paused.load(::std::memory_order_relaxed))
            continue;

        try
        {
            auto result = PerformCheck();

            if (result.severity != DeadlockCheckResult::Severity::NONE)
            {
                LogResult(result);
                NotifyCallbacks(result);

                if (result.requiresDump && _config.enableAutoDump)
                {
                    GenerateDiagnosticDump(result);
                }

                if (result.requiresRecovery && _config.enableAutoRecovery)
                {
                    AttemptRecovery(result);
                }
            }
        }
        catch (const ::std::exception& e)
        {
            TC_LOG_ERROR("playerbot.threadpool.deadlock",
                "Exception in deadlock detection loop: {}", e.what());
        }
    }
}

DeadlockCheckResult DeadlockDetector::PerformCheck()
{
    DeadlockCheckResult result;
    _stats.checksPerformed.fetch_add(1, ::std::memory_order_relaxed);

    // Get current time
    auto now = ::std::chrono::steady_clock::now();

    // Check various deadlock conditions
    bool allSleeping = CheckAllWorkersSleeping(result);
    bool majoritySleeping = CheckMajorityWorkersSleeping(result);
    bool hasStuckWorkers = CheckStuckWorkers(result);
    bool queueGrowing = CheckQueueGrowth(result);

    // Determine severity
    if (allSleeping && result.totalQueuedTasks > 0)
    {
        result.severity = DeadlockCheckResult::Severity::CRITICAL;
        result.description = "CRITICAL: All workers sleeping with pending tasks";
        result.requiresDump = true;
        result.requiresRecovery = true;
        _stats.deadlocksDetected.fetch_add(1, ::std::memory_order_relaxed);
    }
    else if (hasStuckWorkers && queueGrowing)
    {
        result.severity = DeadlockCheckResult::Severity::ERROR;
        result.description = "ERROR: Workers stuck and queue growing";
        result.requiresDump = true;
        _stats.deadlocksDetected.fetch_add(1, ::std::memory_order_relaxed);
    }
    else if (majoritySleeping && result.totalQueuedTasks > _config.maxQueueSizeBeforeAlert)
    {
        result.severity = DeadlockCheckResult::Severity::WARNING;
        result.description = "WARNING: Majority of workers sleeping with large queue";
        _stats.warningsIssued.fetch_add(1, ::std::memory_order_relaxed);
    }
    else if (hasStuckWorkers)
    {
        result.severity = DeadlockCheckResult::Severity::WARNING;
        result.description = "WARNING: Workers stuck in same state";
        _stats.warningsIssued.fetch_add(1, ::std::memory_order_relaxed);
    }
    else if (queueGrowing)
    {
        result.severity = DeadlockCheckResult::Severity::INFO;
        result.description = "INFO: Queue growing but workers active";
    }

    // Update state for next check
    _state.lastCheckTime = now;

    return result;
}

bool DeadlockDetector::CheckAllWorkersSleeping(DeadlockCheckResult& result)
{
    if (!_pool)
        return false;

    auto workers = GetWorkerDiagnostics();
    if (workers.empty())
        return false;

    uint32 sleepingCount = 0;
    uint32 totalWorkers = 0;

    for (const auto& [workerId, diag] : workers)
    {
        if (!diag)
            continue;

        totalWorkers++;
        WorkerState state = diag->currentState.load(::std::memory_order_relaxed);

        if (state == WorkerState::IDLE_SLEEPING)
        {
            sleepingCount++;

            auto sleepDuration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
                ::std::chrono::steady_clock::now() - diag->stateEnterTime);

            if (sleepDuration >= _config.allWorkersSleepThreshold)
            {
                result.workerIssues.push_back({
                    workerId, state, sleepDuration,
                    "Sleeping for " + ::std::to_string(sleepDuration.count()) + "ms"
                });
            }
        }
    }

    // Get queue stats
    result.totalQueuedTasks = _pool->GetQueuedTasks();

    // Check if all are sleeping with work pending
    return (sleepingCount == totalWorkers && totalWorkers > 0 && result.totalQueuedTasks > 0);
}

bool DeadlockDetector::CheckMajorityWorkersSleeping(DeadlockCheckResult& result)
{
    if (!_pool)
        return false;

    auto workers = GetWorkerDiagnostics();
    if (workers.empty())
        return false;

    uint32 sleepingLongCount = 0;
    uint32 totalWorkers = 0;

    for (const auto& [workerId, diag] : workers)
    {
        if (!diag)
            continue;

        totalWorkers++;
        WorkerState state = diag->currentState.load(::std::memory_order_relaxed);

        if (state == WorkerState::IDLE_SLEEPING)
        {
            auto sleepDuration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
                ::std::chrono::steady_clock::now() - diag->stateEnterTime);

            if (sleepDuration >= _config.majorityWorkersSleepThreshold)
            {
                sleepingLongCount++;
                result.workerIssues.push_back({
                    workerId, state, sleepDuration,
                    "Extended sleep: " + ::std::to_string(sleepDuration.count()) + "ms"
                });
            }
        }
    }

    float sleepRatio = totalWorkers > 0 ?
        static_cast<float>(sleepingLongCount) / totalWorkers : 0.0f;

    if (sleepRatio > _config.majorityThreshold)
    {
        result.details.push_back(
            ::std::to_string(sleepingLongCount) + "/" + ::std::to_string(totalWorkers) +
            " workers sleeping for >" +
            ::std::to_string(_config.majorityWorkersSleepThreshold.count()) + "ms");
        return true;
    }

    return false;
}

bool DeadlockDetector::CheckStuckWorkers(DeadlockCheckResult& result)
{
    if (!_pool)
        return false;

    auto workers = GetWorkerDiagnostics();
    bool hasStuckWorkers = false;

    for (const auto& [workerId, diag] : workers)
    {
        if (!diag)
            continue;

        WorkerState state = diag->currentState.load(::std::memory_order_relaxed);
        auto timeInState = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
            ::std::chrono::steady_clock::now() - diag->stateEnterTime);

        // Check for stuck workers (excluding expected long-running states)
    if (timeInState >= _config.singleWorkerStuckThreshold &&
            state != WorkerState::IDLE_SLEEPING &&
            state != WorkerState::TERMINATED)
        {
            hasStuckWorkers = true;

            ::std::string stateStr = WorkerStateToString(state);
            result.workerIssues.push_back({
                workerId, state, timeInState,
                "Stuck in " + stateStr + " for " +
                ::std::to_string(timeInState.count()) + "ms"
            });

            // Add wait location if available
            auto wait = diag->GetCurrentWait();
            if (wait.has_value())
            {
                result.details.push_back(
                    "Worker " + ::std::to_string(workerId) +
                    " waiting at: " + wait->ToString());
            }
        }
    }

    return hasStuckWorkers;
}

bool DeadlockDetector::CheckQueueGrowth(DeadlockCheckResult& result)
{
    if (!_pool)
        return false;

    size_t currentQueueSize = _pool->GetQueuedTasks();
    result.totalQueuedTasks = currentQueueSize;

    // Check if queue is growing
    if (currentQueueSize > _state.lastQueueSize)
    {
        _state.consecutiveQueueGrowths++;
    }
    else
    {
        _state.consecutiveQueueGrowths = 0;
    }

    _state.lastQueueSize = currentQueueSize;

    // Check completed tasks
    uint64 completedTasks = 0;
    auto workers = GetWorkerDiagnostics();
    for (const auto& [workerId, diag] : workers)
    {
        if (diag)
            completedTasks += diag->tasksExecuted.load(::std::memory_order_relaxed);
    }

    result.completedTasks = completedTasks;

    // Calculate throughput
    auto timeDelta = ::std::chrono::duration_cast<::std::chrono::seconds>(
        ::std::chrono::steady_clock::now() - _state.lastCheckTime).count();
    if (timeDelta > 0 && _state.lastCompletedTasks > 0)
    {
        result.throughput = static_cast<double>(completedTasks - _state.lastCompletedTasks) / timeDelta;
    }

    _state.lastCompletedTasks = completedTasks;

    // Queue growing with no progress is a problem
    if (_state.consecutiveQueueGrowths >= _config.queueGrowthCheckCount)
    {
        result.details.push_back(
            "Queue growing for " +
            ::std::to_string(_state.consecutiveQueueGrowths) +
            " consecutive checks. Size: " + ::std::to_string(currentQueueSize));
        return true;
    }

    return false;
}

void DeadlockDetector::GenerateDiagnosticDump(const DeadlockCheckResult& result)
{
    // Rate limit dumps (max 1 per minute)
    auto now = ::std::chrono::steady_clock::now();
    auto timeSinceLastDump = ::std::chrono::duration_cast<::std::chrono::seconds>(
        now - _state.lastDumpTime).count();

    if (timeSinceLastDump < 60)
        return;

    _state.lastDumpTime = now;
    _stats.dumpsGenerated.fetch_add(1, ::std::memory_order_relaxed);

    // Generate filename with timestamp
    ::std::string filename = _config.dumpDirectory + "threadpool_deadlock_" +
                          GetTimestampString() + ".txt";

    try
    {
        ::std::ofstream dump(filename);
        if (!dump.is_open())
        {
            TC_LOG_ERROR("playerbot.threadpool.deadlock",
                "Failed to create diagnostic dump file: {}", filename);
            return;
        }

        // Write header
        dump << "========================================\n";
        dump << "ThreadPool Deadlock Diagnostic Report\n";
        dump << "========================================\n\n";
        dump << "Generated: " << GetTimestampString() << "\n";
        dump << "Severity: " << static_cast<int>(result.severity) << "\n";
        dump << "Description: " << result.description << "\n\n";

        // Write summary
        dump << "Summary\n";
        dump << "-------\n";
        dump << "Total Queued Tasks: " << result.totalQueuedTasks << "\n";
        dump << "Completed Tasks: " << result.completedTasks << "\n";
        dump << "Throughput: " << ::std::fixed << ::std::setprecision(2)
             << result.throughput << " tasks/sec\n\n";

        // Write details
    if (!result.details.empty())
        {
            dump << "Details\n";
            dump << "-------\n";
            for (const auto& detail : result.details)
            {
                dump << "- " << detail << "\n";
            }
            dump << "\n";
        }

        // Write worker issues
    if (!result.workerIssues.empty())
        {
            dump << "Worker Issues\n";
            dump << "-------------\n";
            for (const auto& issue : result.workerIssues)
            {
                dump << "Worker " << issue.workerId << ": " << issue.issue << "\n";
            }
            dump << "\n";
        }

        // Write detailed worker diagnostics
        dump << "Detailed Worker Diagnostics\n";
        dump << "==========================\n\n";

        auto workers = GetWorkerDiagnostics();
        for (const auto& [workerId, diag] : workers)
        {
            if (diag)
            {
                dump << diag->GenerateReport(workerId) << "\n";
                dump << "----------------------------------------\n\n";
            }
        }

        // Write pool configuration
    if (_pool)
        {
            auto config = _pool->GetConfiguration();
            dump << "ThreadPool Configuration\n";
            dump << "------------------------\n";
            dump << "Worker Threads: " << config.numThreads << "\n";
            dump << "Max Queue Size: " << config.maxQueueSize << "\n";
            dump << "Work Stealing: " << (config.enableWorkStealing ? "Enabled" : "Disabled") << "\n";
            dump << "CPU Affinity: " << (config.enableCpuAffinity ? "Enabled" : "Disabled") << "\n";
            dump << "Max Steal Attempts: " << config.maxStealAttempts << "\n";
            dump << "Worker Sleep Time: " << config.workerSleepTime.count() << "ms\n";
            dump << "\n";
        }

        // Write detector statistics
        dump << "Detector Statistics\n";
        dump << "-------------------\n";
        dump << "Checks Performed: " << _stats.checksPerformed.load() << "\n";
        dump << "Deadlocks Detected: " << _stats.deadlocksDetected.load() << "\n";
        dump << "Warnings Issued: " << _stats.warningsIssued.load() << "\n";
        dump << "Dumps Generated: " << _stats.dumpsGenerated.load() << "\n";
        dump << "Recoveries Attempted: " << _stats.recoveriesAttempted.load() << "\n";

        auto uptime = ::std::chrono::duration_cast<::std::chrono::seconds>(
            now - _stats.startTime).count();
        dump << "Detector Uptime: " << uptime << " seconds\n";

        dump.close();

        TC_LOG_WARN("playerbot.threadpool.deadlock",
            "Diagnostic dump generated: {}", filename);
    }
    catch (const ::std::exception& e)
    {
        TC_LOG_ERROR("playerbot.threadpool.deadlock",
            "Failed to write diagnostic dump: {}", e.what());
    }
}

void DeadlockDetector::AttemptRecovery(const DeadlockCheckResult& result)
{
    _stats.recoveriesAttempted.fetch_add(1, ::std::memory_order_relaxed);

    TC_LOG_WARN("playerbot.threadpool.deadlock",
        "Attempting automatic recovery for: {}", result.description);

    if (!_pool)
        return;

    // Recovery strategy depends on the issue
    if (result.severity == DeadlockCheckResult::Severity::CRITICAL)
    {
        // Critical deadlock - wake all workers forcefully
        uint32 workerCount = _pool->GetWorkerCount();
        for (uint32 i = 0; i < workerCount; ++i)
        {
            auto worker = _pool->GetWorker(i);
            if (worker)
            {
                worker->Wake();
            }
        }

        TC_LOG_WARN("playerbot.threadpool.deadlock",
            "Forcefully woke all {} workers", workerCount);
    }
    else if (result.severity == DeadlockCheckResult::Severity::ERROR)
    {
        // Error condition - wake sleeping workers
    for (const auto& issue : result.workerIssues)
        {
            if (issue.state == WorkerState::IDLE_SLEEPING)
            {
                auto worker = _pool->GetWorker(issue.workerId);
                if (worker)
                {
                    worker->Wake();
                    TC_LOG_DEBUG("playerbot.threadpool.deadlock",
                        "Woke sleeping worker {}", issue.workerId);
                }
            }
        }
    }
}

void DeadlockDetector::NotifyCallbacks(const DeadlockCheckResult& result)
{
    // Notify registered callbacks
    {
        ::std::lock_guard<::std::mutex> lock(_callbackMutex);
        for (const auto& callback : _callbacks)
        {
            try
            {
                callback(result);
            }
            catch (const ::std::exception& e)
            {
                TC_LOG_ERROR("playerbot.threadpool.deadlock",
                    "Exception in deadlock callback: {}", e.what());
            }
        }
    }

    // Notify optional debugging integrations
    if (_debuggerIntegration)
    {
        try
        {
            _debuggerIntegration->OnDeadlockDetected(result);
        }
        catch (const ::std::exception& e)
        {
            TC_LOG_ERROR("playerbot.threadpool.deadlock",
                "Exception in debugger integration: {}", e.what());
        }
    }

    if (_etwProvider)
    {
        try
        {
            _etwProvider->LogDeadlockDetected(result);
        }
        catch (const ::std::exception& e)
        {
            TC_LOG_ERROR("playerbot.threadpool.deadlock",
                "Exception in ETW provider: {}", e.what());
        }
    }
}

void DeadlockDetector::LogResult(const DeadlockCheckResult& result)
{
    // Don't spam logs
    if (_state.warningCount >= _config.maxConsecutiveWarnings &&
        result.severity <= DeadlockCheckResult::Severity::WARNING)
    {
        return;
    }

    switch (result.severity)
    {
        case DeadlockCheckResult::Severity::CRITICAL:
            TC_LOG_ERROR("playerbot.threadpool.deadlock", "{}", result.description);
            for (const auto& detail : result.details)
                TC_LOG_ERROR("playerbot.threadpool.deadlock", "  {}", detail);
            _state.warningCount++;
            break;

        case DeadlockCheckResult::Severity::ERROR:
            TC_LOG_WARN("playerbot.threadpool.deadlock", "{}", result.description);
            for (const auto& detail : result.details)
                TC_LOG_WARN("playerbot.threadpool.deadlock", "  {}", detail);
            _state.warningCount++;
            break;

        case DeadlockCheckResult::Severity::WARNING:
            TC_LOG_DEBUG("playerbot.threadpool.deadlock", "{}", result.description);
            _state.warningCount++;
            break;

        case DeadlockCheckResult::Severity::INFO:
            TC_LOG_TRACE("playerbot.threadpool.deadlock", "{}", result.description);
            _state.warningCount = 0; // Reset on info/none
            break;

        case DeadlockCheckResult::Severity::NONE:
            _state.warningCount = 0;
            break;
    }
}

::std::vector<::std::pair<uint32, WorkerDiagnostics*>> DeadlockDetector::GetWorkerDiagnostics()
{
    ::std::vector<::std::pair<uint32, WorkerDiagnostics*>> diagnostics;

    if (!_pool)
        return diagnostics;

    uint32 workerCount = _pool->GetWorkerCount();
    for (uint32 i = 0; i < workerCount; ++i)
    {
        auto worker = _pool->GetWorker(i);
        if (worker && worker->GetDiagnostics())
        {
            diagnostics.push_back({i, worker->GetDiagnostics()});
        }
    }

    return diagnostics;
}

::std::string DeadlockDetector::GetTimestampString() const
{
    auto now = ::std::chrono::system_clock::now();
    auto time_t = ::std::chrono::system_clock::to_time_t(now);
    ::std::stringstream ss;
    ss << ::std::put_time(::std::localtime(&time_t), "%Y%m%d_%H%M%S");
    return ss.str();
}

DeadlockCheckResult DeadlockDetector::CheckNow()
{
    return PerformCheck();
}

void DeadlockDetector::RegisterCallback(DeadlockCallback callback)
{
    ::std::lock_guard<::std::mutex> lock(_callbackMutex);
    _callbacks.push_back(::std::move(callback));
}

DeadlockDetector::Stats DeadlockDetector::GetStatistics() const
{
    Stats stats;
    stats.checksPerformed = _stats.checksPerformed.load();
    stats.deadlocksDetected = _stats.deadlocksDetected.load();
    stats.warningsIssued = _stats.warningsIssued.load();
    stats.dumpsGenerated = _stats.dumpsGenerated.load();
    stats.recoveriesAttempted = _stats.recoveriesAttempted.load();
    stats.uptime = ::std::chrono::duration_cast<::std::chrono::seconds>(
        ::std::chrono::steady_clock::now() - _stats.startTime);
    return stats;
}

} // namespace Performance
} // namespace Playerbot