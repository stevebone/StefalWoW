/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * ETW (Event Tracing for Windows) Provider
 *
 * Provides integration with Windows Performance Analyzer (WPA) for
 * advanced performance analysis and visualization.
 *
 * Features:
 * - Real-time event tracing for worker state changes
 * - Task execution timeline visualization in WPA
 * - Deadlock detection events
 * - Performance counter integration
 * - Low overhead (<0.05% CPU when enabled)
 *
 * Usage with Windows Performance Analyzer:
 * 1. Start ETW trace: xperf -start trinitybot -on TrinityCore-PlayerBot-ThreadPool
 * 2. Run worldserver with bots
 * 3. Stop trace: xperf -stop trinitybot -d trace.etl
 * 4. Open trace.etl in Windows Performance Analyzer
 * 5. View ThreadPool activity timeline, state transitions, deadlocks
 *
 * Event Types:
 * - WorkerStateChange: Thread state transitions
 * - TaskExecution: Task start/end with latency
 * - WorkSteal: Steal attempts and results
 * - DeadlockDetected: Deadlock detection events
 * - QueueDepth: Queue depth samples
 */

#ifndef PLAYERBOT_ETW_PROVIDER_H
#define PLAYERBOT_ETW_PROVIDER_H

#include "Define.h"
#include "ThreadPoolDiagnostics.h"

#ifdef _WIN32
#include <Windows.h>
#include <evntprov.h>
#pragma comment(lib, "Advapi32.lib")
#endif

namespace Playerbot {
namespace Performance {

// Forward declarations
struct DeadlockCheckResult;
enum class TaskPriority : uint8;

/**
 * @brief Wait type for ETW logging (simplified)
 */
enum class WaitType : uint32
{
    NONE = 0,
    CONDITION_VARIABLE = 1,
    MUTEX = 2,
    ATOMIC_WAIT = 3,
    SLEEP = 4,
    YIELD = 5
};

#ifdef _WIN32

/**
 * @brief ETW Provider configuration
 */
struct ETWProviderConfig
{
    bool enableWorkerStateEvents{true};      // Trace worker state changes
    bool enableTaskExecutionEvents{true};    // Trace task execution
    bool enableWorkStealEvents{true};        // Trace work stealing
    bool enableDeadlockEvents{true};         // Trace deadlock detection
    bool enableQueueDepthEvents{true};       // Trace queue depth sampling
    bool enableHighFrequencyEvents{false};   // Enable high-frequency events (more overhead)

    // Sampling rates (to reduce overhead)
    uint32 queueDepthSampleIntervalMs{100};  // Sample queue depth every 100ms
    uint32 taskExecutionSampleRate{10};      // Log every Nth task execution
};

/**
 * @brief ETW Event IDs
 */
enum class ETWEventId : UCHAR
{
    WorkerStateChange = 1,
    TaskExecutionStart = 2,
    TaskExecutionEnd = 3,
    WorkStealAttempt = 4,
    WorkStealSuccess = 5,
    DeadlockDetected = 6,
    QueueDepthSample = 7,
    WorkerWaitStart = 8,
    WorkerWaitEnd = 9,
    ThreadPoolShutdown = 10
};

/**
 * @brief ETW Event Levels
 */
enum class ETWLevel : UCHAR
{
    LogAlways = 0,
    Critical = 1,
    Error = 2,
    Warning = 3,
    Information = 4,
    Verbose = 5
};

/**
 * @brief ETW Provider wrapper
 *
 * Provides easy-to-use interface for logging ThreadPool events to ETW
 */
class ETWProvider
{
public:
    ETWProvider();
    ~ETWProvider();

    // Lifecycle
    bool Initialize();
    void Shutdown();
    bool IsInitialized() const { return _initialized; }

    // Configuration
    void SetConfig(ETWProviderConfig config) { _config = ::std::move(config); }
    ETWProviderConfig GetConfig() const { return _config; }

    // Event logging
    void LogWorkerStateChange(uint32 workerId, WorkerState fromState, WorkerState toState);
    void LogTaskExecutionStart(uint32 workerId, uint64 taskId, TaskPriority priority);
    void LogTaskExecutionEnd(uint32 workerId, uint64 taskId, uint64 executionTimeMicros);
    void LogWorkStealAttempt(uint32 thiefWorkerId, uint32 victimWorkerId);
    void LogWorkStealSuccess(uint32 thiefWorkerId, uint32 victimWorkerId, uint32 taskCount);
    void LogDeadlockDetected(const DeadlockCheckResult& result);
    void LogQueueDepthSample(size_t totalQueued, size_t criticalQueued, size_t highQueued,
                            size_t normalQueued, size_t lowQueued);
    void LogWorkerWaitStart(uint32 workerId, WaitType waitType, const char* location);
    void LogWorkerWaitEnd(uint32 workerId, uint64 waitDurationMicros);
    void LogThreadPoolShutdown(uint32 workerCount, uint64 totalTasksExecuted);

    // Provider status
    bool IsEnabled() const;

    // Statistics
    struct Stats
    {
        uint64 eventsLogged{0};
        uint64 eventsDropped{0};       // Due to sampling
        uint64 writeErrors{0};
    };
    Stats GetStatistics() const { return _stats; }

private:
    // Helper: Write event to ETW
    bool WriteEvent(ETWEventId eventId, ETWLevel level, const void* eventData, ULONG dataSize);

    // Helper: Check if should log (sampling)
    bool ShouldLogTaskExecution();

    // Provider handle
    REGHANDLE _providerHandle{0};
    bool _initialized{false};

    // Configuration
    ETWProviderConfig _config;

    // Sampling counters
    ::std::atomic<uint32> _taskExecutionCounter{0};

    // Statistics
    mutable Stats _stats;
};

// Global ETW provider instance
ETWProvider& GetETWProvider();

/**
 * @brief RAII helper for ETW event scopes
 *
 * Usage:
 *   {
 *       ETWEventScope scope(workerId, "TaskExecution", taskId);
 *       // ... do work ...
 *       // Automatically logs duration on scope exit
 *   }
 */
class ETWEventScope
{
public:
    ETWEventScope(uint32 workerId, const char* eventName, uint64 eventId);
    ~ETWEventScope();

private:
    uint32 _workerId;
    const char* _eventName;
    uint64 _eventId;
    ::std::chrono::steady_clock::time_point _startTime;
};

#else // !_WIN32

// Stub implementations for non-Windows platforms

struct ETWProviderConfig
{
    bool enableWorkerStateEvents{false};
    bool enableTaskExecutionEvents{false};
    bool enableWorkStealEvents{false};
    bool enableDeadlockEvents{false};
    bool enableQueueDepthEvents{false};
    bool enableHighFrequencyEvents{false};
    uint32 queueDepthSampleIntervalMs{100};
    uint32 taskExecutionSampleRate{10};
};

class ETWProvider
{
public:
    ETWProvider() = default;
    ~ETWProvider() = default;

    bool Initialize() { return false; }
    void Shutdown() {}
    bool IsInitialized() const { return false; }

    void SetConfig(ETWProviderConfig) {}
    ETWProviderConfig GetConfig() const { return {}; }

    void LogWorkerStateChange(uint32, WorkerState, WorkerState) {}
    void LogTaskExecutionStart(uint32, uint64, TaskPriority) {}
    void LogTaskExecutionEnd(uint32, uint64, uint64) {}
    void LogWorkStealAttempt(uint32, uint32) {}
    void LogWorkStealSuccess(uint32, uint32, uint32) {}
    void LogDeadlockDetected(const DeadlockCheckResult&) {}
    void LogQueueDepthSample(size_t, size_t, size_t, size_t, size_t) {}
    void LogWorkerWaitStart(uint32, WaitType, const char*) {}
    void LogWorkerWaitEnd(uint32, uint64) {}
    void LogThreadPoolShutdown(uint32, uint64) {}

    bool IsEnabled() const { return false; }

    struct Stats { uint64 eventsLogged{0}; uint64 eventsDropped{0}; uint64 writeErrors{0}; };
    Stats GetStatistics() const { return {}; }
};

ETWProvider& GetETWProvider();

class ETWEventScope
{
public:
    ETWEventScope(uint32, const char*, uint64) {}
    ~ETWEventScope() = default;
};

#endif // _WIN32

} // namespace Performance
} // namespace Playerbot

// Convenience macros
#define ETW_LOG_WORKER_STATE_CHANGE(workerId, fromState, toState) \
    Playerbot::Performance::GetETWProvider().LogWorkerStateChange(workerId, fromState, toState)

#define ETW_LOG_TASK_START(workerId, taskId, priority) \
    Playerbot::Performance::GetETWProvider().LogTaskExecutionStart(workerId, taskId, priority)

#define ETW_LOG_TASK_END(workerId, taskId, durationMicros) \
    Playerbot::Performance::GetETWProvider().LogTaskExecutionEnd(workerId, taskId, durationMicros)

#define ETW_SCOPE(workerId, eventName, eventId) \
    Playerbot::Performance::ETWEventScope _etw_scope_##__LINE__(workerId, eventName, eventId)

#endif // PLAYERBOT_ETW_PROVIDER_H
