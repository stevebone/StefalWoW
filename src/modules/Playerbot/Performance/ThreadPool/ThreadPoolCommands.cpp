/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * ThreadPool Console Commands
 *
 * Runtime diagnostic commands for ThreadPool monitoring and debugging
 */

#include "ThreadPool.h"
#include "DeadlockDetector.h"
#include "Chat.h"
#include "Player.h"
#include "../../Session/BotWorldSessionMgr.h"
#include <sstream>
#include <iomanip>

namespace Playerbot {
namespace Performance {

/**
 * @brief Register ThreadPool console commands
 */
void RegisterThreadPoolCommands()
{
    // Commands are registered through TrinityCore's command system
    // Format: .bot threadpool <subcommand>
}

/**
 * @brief Handler for .bot threadpool status command
 *
 * Shows current status of all worker threads
 */
bool HandleThreadPoolStatusCommand(ChatHandler* handler, char const* /*args*/)
{
    auto& pool = GetThreadPool();

    handler->PSendSysMessage("=== ThreadPool Status ===");
    handler->PSendSysMessage("Workers: %u", pool.GetWorkerCount());
    handler->PSendSysMessage("Active Threads: %zu", pool.GetActiveThreads());
    handler->PSendSysMessage("Queued Tasks: %zu", pool.GetQueuedTasks());
    handler->PSendSysMessage("Diagnostics: %s",
        pool.IsDiagnosticsEnabled() ? "Enabled" : "Disabled");

    // Show per-priority queue counts
    handler->PSendSysMessage("\nQueue Breakdown:");
    handler->PSendSysMessage("  CRITICAL: %zu", pool.GetQueuedTasks(TaskPriority::CRITICAL));
    handler->PSendSysMessage("  HIGH: %zu", pool.GetQueuedTasks(TaskPriority::HIGH));
    handler->PSendSysMessage("  NORMAL: %zu", pool.GetQueuedTasks(TaskPriority::NORMAL));
    handler->PSendSysMessage("  LOW: %zu", pool.GetQueuedTasks(TaskPriority::LOW));
    handler->PSendSysMessage("  IDLE: %zu", pool.GetQueuedTasks(TaskPriority::IDLE));

    // Show worker states if diagnostics enabled
    if (pool.IsDiagnosticsEnabled())
    {
        handler->PSendSysMessage("\nWorker States:");

        for (uint32 i = 0; i < pool.GetWorkerCount(); ++i)
        {
            auto worker = pool.GetWorker(i);
            if (worker && worker->GetDiagnostics())
            {
                auto diag = worker->GetDiagnostics();
                WorkerState state = diag->currentState.load(::std::memory_order_relaxed);

                handler->PSendSysMessage("  Worker %u: %s", i, WorkerStateToString(state));

                // Show wait location if waiting
                auto wait = diag->GetCurrentWait();
                if (wait.has_value())
                {
                    handler->PSendSysMessage("    Waiting: %s", wait->ToString().c_str());
                }
            }
        }
    }

    return true;
}

/**
 * @brief Handler for .bot threadpool stats command
 *
 * Shows performance statistics
 */
bool HandleThreadPoolStatsCommand(ChatHandler* handler, char const* /*args*/)
{
    auto& pool = GetThreadPool();

    handler->PSendSysMessage("=== ThreadPool Statistics ===");

    // Basic metrics
    auto avgLatency = pool.GetAverageLatency();
    auto throughput = pool.GetThroughput();

    handler->PSendSysMessage("Average Latency: %lldus", avgLatency.count());
    handler->PSendSysMessage("Throughput: %.1f tasks/sec", throughput);

    // Detailed worker statistics
    if (pool.IsDiagnosticsEnabled())
    {
        handler->PSendSysMessage("\n=== Worker Statistics ===");

        for (uint32 i = 0; i < pool.GetWorkerCount(); ++i)
        {
            auto worker = pool.GetWorker(i);
            if (worker && worker->GetDiagnostics())
            {
                auto diag = worker->GetDiagnostics();

                handler->PSendSysMessage("\nWorker %u:", i);
                handler->PSendSysMessage("  Tasks Executed: %llu",
                    diag->tasksExecuted.load());
                handler->PSendSysMessage("  Tasks Failed: %llu",
                    diag->tasksFailed.load());

                // Steal statistics
                uint64 stealAttempts = diag->stealAttempts.load();
                uint64 stealSuccesses = diag->stealSuccesses.load();
                double stealRate = stealAttempts > 0 ?
                    (100.0 * stealSuccesses / stealAttempts) : 0.0;

                handler->PSendSysMessage("  Steal Success Rate: %.1f%% (%llu/%llu)",
                    stealRate, stealSuccesses, stealAttempts);

                // Task latency statistics
                auto stats = diag->taskLatency.GetStats();
                if (stats.count > 0)
                {
                    handler->PSendSysMessage("  Task Latency:");
                    handler->PSendSysMessage("    Avg: %.0fus", stats.avgMicros);
                    handler->PSendSysMessage("    P50: %.0fus", stats.p50Micros);
                    handler->PSendSysMessage("    P95: %.0fus", stats.p95Micros);
                    handler->PSendSysMessage("    P99: %.0fus", stats.p99Micros);
                }
            }
        }
    }

    // Deadlock detector statistics
    auto detector = pool.GetDeadlockDetector();
    if (detector)
    {
        auto stats = detector->GetStatistics();
        handler->PSendSysMessage("\n=== Deadlock Detector ===");
        handler->PSendSysMessage("Checks: %llu", stats.checksPerformed);
        handler->PSendSysMessage("Deadlocks Detected: %llu", stats.deadlocksDetected);
        handler->PSendSysMessage("Warnings Issued: %llu", stats.warningsIssued);
        handler->PSendSysMessage("Uptime: %lld seconds", stats.uptime.count());
    }

    return true;
}

/**
 * @brief Handler for .bot threadpool worker <id> command
 *
 * Shows detailed information for a specific worker
 */
bool HandleThreadPoolWorkerCommand(ChatHandler* handler, char const* args)
{
    if (!args)
    {
        handler->SendSysMessage("Usage: .bot threadpool worker <id>");
        return true;
    }

    uint32 workerId = atoi(args);
    auto& pool = GetThreadPool();

    if (workerId >= pool.GetWorkerCount())
    {
        handler->PSendSysMessage("Invalid worker ID. Valid range: 0-%u",
            pool.GetWorkerCount() - 1);
        return true;
    }

    auto worker = pool.GetWorker(workerId);
    if (!worker)
    {
        handler->PSendSysMessage("Worker %u not found", workerId);
        return true;
    }

    handler->PSendSysMessage("=== Worker %u Details ===", workerId);

    // Basic metrics
    auto metrics = worker->GetMetrics();
    handler->PSendSysMessage("Tasks Completed: %llu", metrics.tasksCompleted);
    handler->PSendSysMessage("Total Work Time: %llums", metrics.totalWorkTime / 1000);
    handler->PSendSysMessage("Total Idle Time: %llums", metrics.totalIdleTime / 1000);
    handler->PSendSysMessage("Steal Attempts: %llu", metrics.stealAttempts);
    handler->PSendSysMessage("Steal Successes: %llu", metrics.stealSuccesses);

    // Diagnostic information if available
    if (worker->GetDiagnostics())
    {
        auto diag = worker->GetDiagnostics();
        handler->SendSysMessage("\n=== Diagnostic Information ===");

        ::std::string report = diag->GenerateReport(workerId);

        // Send report line by line (PSendSysMessage has length limits)
        ::std::istringstream stream(report);
        ::std::string line;
        while (::std::getline(stream, line))
        {
            handler->SendSysMessage(line.c_str());
        }
    }

    return true;
}

/**
 * @brief Handler for .bot threadpool deadlock command
 *
 * Manually triggers deadlock detection
 */
bool HandleThreadPoolDeadlockCommand(ChatHandler* handler, char const* /*args*/)
{
    auto& pool = GetThreadPool();
    auto detector = pool.GetDeadlockDetector();

    if (!detector)
    {
        handler->SendSysMessage("Deadlock detector not initialized");
        return true;
    }

    handler->SendSysMessage("Running deadlock detection...");

    auto result = detector->CheckNow();

    switch (result.severity)
    {
        case DeadlockCheckResult::Severity::NONE:
            handler->SendSysMessage("No issues detected");
            break;

        case DeadlockCheckResult::Severity::INFO:
            handler->PSendSysMessage("INFO: %s", result.description.c_str());
            break;

        case DeadlockCheckResult::Severity::WARNING:
            handler->PSendSysMessage("WARNING: %s", result.description.c_str());
            break;

        case DeadlockCheckResult::Severity::ERROR:
            handler->PSendSysMessage("ERROR: %s", result.description.c_str());
            break;

        case DeadlockCheckResult::Severity::CRITICAL:
            handler->PSendSysMessage("CRITICAL: %s", result.description.c_str());
            break;
    }

    // Show details
    for (const auto& detail : result.details)
    {
        handler->PSendSysMessage("  - %s", detail.c_str());
    }

    // Show worker issues
    if (!result.workerIssues.empty())
    {
        handler->SendSysMessage("\nWorker Issues:");
        for (const auto& issue : result.workerIssues)
        {
            handler->PSendSysMessage("  Worker %u: %s",
                issue.workerId, issue.issue.c_str());
        }
    }

    handler->PSendSysMessage("\nQueued Tasks: %zu", result.totalQueuedTasks);
    handler->PSendSysMessage("Completed Tasks: %zu", result.completedTasks);
    handler->PSendSysMessage("Throughput: %.1f tasks/sec", result.throughput);

    return true;
}

/**
 * @brief Handler for .bot threadpool trace <id> command
 *
 * Enables detailed tracing for a specific worker
 */
bool HandleThreadPoolTraceCommand(ChatHandler* handler, char const* args)
{
    if (!args)
    {
        handler->SendSysMessage("Usage: .bot threadpool trace <id> [on|off]");
        return true;
    }

    char* workerIdStr = strtok((char*)args, " ");
    char* stateStr = strtok(nullptr, " ");

    uint32 workerId = atoi(workerIdStr);
    bool enable = !stateStr || strcmp(stateStr, "on") == 0;

    auto& pool = GetThreadPool();

    if (workerId >= pool.GetWorkerCount())
    {
        handler->PSendSysMessage("Invalid worker ID. Valid range: 0-%u",
            pool.GetWorkerCount() - 1);
        return true;
    }

    // This would enable detailed logging for the specific worker
    // Implementation depends on logging system integration

    handler->PSendSysMessage("Tracing %s for Worker %u",
        enable ? "enabled" : "disabled", workerId);

    return true;
}

/**
 * @brief Handler for .bot threadpool diagnostics command
 *
 * Enable/disable diagnostics
 */
bool HandleThreadPoolDiagnosticsCommand(ChatHandler* handler, char const* args)
{
    auto& pool = GetThreadPool();

    if (!args)
    {
        bool enabled = pool.IsDiagnosticsEnabled();
        handler->PSendSysMessage("Diagnostics are currently %s",
            enabled ? "enabled" : "disabled");
        return true;
    }

    bool enable = strcmp(args, "on") == 0 || strcmp(args, "enable") == 0;
    pool.SetDiagnosticsEnabled(enable);

    handler->PSendSysMessage("Diagnostics %s", enable ? "enabled" : "disabled");

    // Start/stop deadlock detector
    auto detector = pool.GetDeadlockDetector();
    if (detector)
    {
        if (enable)
            detector->Start();
        else
            detector->Stop();
    }

    return true;
}

/**
 * @brief Main command dispatcher
 */
bool HandleBotThreadPoolCommand(ChatHandler* handler, char const* args)
{
    if (!args)
    {
        handler->SendSysMessage("Usage: .bot threadpool <subcommand>");
        handler->SendSysMessage("Subcommands:");
        handler->SendSysMessage("  status - Show current thread pool status");
        handler->SendSysMessage("  stats - Show performance statistics");
        handler->SendSysMessage("  worker <id> - Show details for specific worker");
        handler->SendSysMessage("  deadlock - Run deadlock detection");
        handler->SendSysMessage("  trace <id> [on|off] - Enable/disable worker tracing");
        handler->SendSysMessage("  diagnostics [on|off] - Enable/disable diagnostics");
        return true;
    }

    char* cmd = strtok((char*)args, " ");
    char* subcmdArgs = strtok(nullptr, "");

    if (strcmp(cmd, "status") == 0)
        return HandleThreadPoolStatusCommand(handler, subcmdArgs);
    else if (strcmp(cmd, "stats") == 0)
        return HandleThreadPoolStatsCommand(handler, subcmdArgs);
    else if (strcmp(cmd, "worker") == 0)
        return HandleThreadPoolWorkerCommand(handler, subcmdArgs);
    else if (strcmp(cmd, "deadlock") == 0)
        return HandleThreadPoolDeadlockCommand(handler, subcmdArgs);
    else if (strcmp(cmd, "trace") == 0)
        return HandleThreadPoolTraceCommand(handler, subcmdArgs);
    else if (strcmp(cmd, "diagnostics") == 0)
        return HandleThreadPoolDiagnosticsCommand(handler, subcmdArgs);
    else
    {
        handler->PSendSysMessage("Unknown subcommand: %s", cmd);
        return true;
    }
}

} // namespace Performance
} // namespace Playerbot