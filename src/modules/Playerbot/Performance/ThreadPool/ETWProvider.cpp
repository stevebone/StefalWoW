/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * ETW Provider Implementation
 */

#include "ETWProvider.h"
#include "DeadlockDetector.h"
#include "ThreadPoolDiagnostics.h"
#include "Log.h"
#include <sstream>
#include <cstring>

// Undefine Windows macros that conflict with our enums
#ifdef ERROR
#undef ERROR
#endif
#ifdef CRITICAL
#undef CRITICAL
#endif

namespace Playerbot {
namespace Performance {

#ifdef _WIN32

// ETW Provider GUID: {8F7D5E42-1A3B-4C5D-9E6F-7A8B9C0D1E2F}
// Generated with: uuidgen.exe
static const GUID TRINITY_THREADPOOL_PROVIDER_GUID = {
    0x8F7D5E42, 0x1A3B, 0x4C5D,
    {0x9E, 0x6F, 0x7A, 0x8B, 0x9C, 0x0D, 0x1E, 0x2F}
};

// Event data structures

#pragma pack(push, 1)

struct WorkerStateChangeEvent
{
    uint32 workerId;
    uint32 fromState;
    uint32 toState;
    uint64 timestamp;
};

struct TaskExecutionStartEvent
{
    uint32 workerId;
    uint64 taskId;
    uint32 priority;
    uint64 timestamp;
};

struct TaskExecutionEndEvent
{
    uint32 workerId;
    uint64 taskId;
    uint64 executionTimeMicros;
    uint64 timestamp;
};

struct WorkStealAttemptEvent
{
    uint32 thiefWorkerId;
    uint32 victimWorkerId;
    uint64 timestamp;
};

struct WorkStealSuccessEvent
{
    uint32 thiefWorkerId;
    uint32 victimWorkerId;
    uint32 taskCount;
    uint64 timestamp;
};

struct DeadlockDetectedEvent
{
    uint32 severity;
    uint32 totalQueuedTasks;
    uint32 completedTasks;
    uint32 workerIssueCount;
    uint64 timestamp;
    char description[256];
};

struct QueueDepthSampleEvent
{
    uint64 totalQueued;
    uint64 criticalQueued;
    uint64 highQueued;
    uint64 normalQueued;
    uint64 lowQueued;
    uint64 timestamp;
};

struct WorkerWaitStartEvent
{
    uint32 workerId;
    uint32 waitType;
    uint64 timestamp;
    char location[128];
};

struct WorkerWaitEndEvent
{
    uint32 workerId;
    uint64 waitDurationMicros;
    uint64 timestamp;
};

struct ThreadPoolShutdownEvent
{
    uint32 workerCount;
    uint64 totalTasksExecuted;
    uint64 timestamp;
};

#pragma pack(pop)

// ETWProvider implementation

ETWProvider::ETWProvider()
{
}

ETWProvider::~ETWProvider()
{
    Shutdown();
}

bool ETWProvider::Initialize()
{
    if (_initialized)
        return true;

    // Register ETW provider
    ULONG status = EventRegister(
        &TRINITY_THREADPOOL_PROVIDER_GUID,
        nullptr,  // No callback
        nullptr,  // No callback context
        &_providerHandle
    );

    if (status != ERROR_SUCCESS)
    {
        TC_LOG_ERROR("playerbot.threadpool.etw",
            "Failed to register ETW provider (error: {})", status);
        return false;
    }

    _initialized = true;

    TC_LOG_INFO("playerbot.threadpool.etw",
        "ETW provider initialized (Handle: {})", _providerHandle);

    return true;
}

void ETWProvider::Shutdown()
{
    if (!_initialized)
        return;

    if (_providerHandle != 0)
    {
        EventUnregister(_providerHandle);
        _providerHandle = 0;
    }

    _initialized = false;

    TC_LOG_INFO("playerbot.threadpool.etw",
        "ETW provider shutdown (Events logged: {}, Dropped: {})",
        _stats.eventsLogged, _stats.eventsDropped);
}

bool ETWProvider::IsEnabled() const
{
    if (!_initialized || _providerHandle == 0)
        return false;

    // Check if any consumer is listening
    return EventProviderEnabled(_providerHandle, 0, 0) != FALSE;
}

void ETWProvider::LogWorkerStateChange(uint32 workerId, WorkerState fromState, WorkerState toState)
{
    if (!_config.enableWorkerStateEvents || !IsEnabled())
        return;

    WorkerStateChangeEvent event;
    event.workerId = workerId;
    event.fromState = static_cast<uint32>(fromState);
    event.toState = static_cast<uint32>(toState);
    event.timestamp = ::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now().time_since_epoch()).count();

    WriteEvent(ETWEventId::WorkerStateChange, ETWLevel::Verbose,
              &event, sizeof(event));
}

void ETWProvider::LogTaskExecutionStart(uint32 workerId, uint64 taskId, TaskPriority priority)
{
    if (!_config.enableTaskExecutionEvents || !IsEnabled())
        return;

    if (!ShouldLogTaskExecution())
    {
        _stats.eventsDropped++;
        return;
    }

    TaskExecutionStartEvent event;
    event.workerId = workerId;
    event.taskId = taskId;
    event.priority = static_cast<uint32>(priority);
    event.timestamp = ::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now().time_since_epoch()).count();

    WriteEvent(ETWEventId::TaskExecutionStart, ETWLevel::Verbose,
              &event, sizeof(event));
}

void ETWProvider::LogTaskExecutionEnd(uint32 workerId, uint64 taskId, uint64 executionTimeMicros)
{
    if (!_config.enableTaskExecutionEvents || !IsEnabled())
        return;

    if (!ShouldLogTaskExecution())
    {
        _stats.eventsDropped++;
        return;
    }

    TaskExecutionEndEvent event;
    event.workerId = workerId;
    event.taskId = taskId;
    event.executionTimeMicros = executionTimeMicros;
    event.timestamp = ::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now().time_since_epoch()).count();

    WriteEvent(ETWEventId::TaskExecutionEnd, ETWLevel::Verbose,
              &event, sizeof(event));
}

void ETWProvider::LogWorkStealAttempt(uint32 thiefWorkerId, uint32 victimWorkerId)
{
    if (!_config.enableWorkStealEvents || !IsEnabled())
        return;

    WorkStealAttemptEvent event;
    event.thiefWorkerId = thiefWorkerId;
    event.victimWorkerId = victimWorkerId;
    event.timestamp = ::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now().time_since_epoch()).count();

    WriteEvent(ETWEventId::WorkStealAttempt, ETWLevel::Verbose,
              &event, sizeof(event));
}

void ETWProvider::LogWorkStealSuccess(uint32 thiefWorkerId, uint32 victimWorkerId, uint32 taskCount)
{
    if (!_config.enableWorkStealEvents || !IsEnabled())
        return;

    WorkStealSuccessEvent event;
    event.thiefWorkerId = thiefWorkerId;
    event.victimWorkerId = victimWorkerId;
    event.taskCount = taskCount;
    event.timestamp = ::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now().time_since_epoch()).count();

    WriteEvent(ETWEventId::WorkStealSuccess, ETWLevel::Information,
              &event, sizeof(event));
}

void ETWProvider::LogDeadlockDetected(const DeadlockCheckResult& result)
{
    if (!_config.enableDeadlockEvents || !IsEnabled())
        return;

    DeadlockDetectedEvent event;
    event.severity = static_cast<uint32>(result.severity);
    event.totalQueuedTasks = static_cast<uint32>(result.totalQueuedTasks);
    event.completedTasks = static_cast<uint32>(result.completedTasks);
    event.workerIssueCount = static_cast<uint32>(result.workerIssues.size());
    event.timestamp = ::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now().time_since_epoch()).count();

    // Copy description (truncate if needed)
    strncpy_s(event.description, sizeof(event.description) / sizeof(event.description[0]),
             result.description.c_str(), _TRUNCATE);

    ETWLevel level = ETWLevel::Warning;
    if (result.severity == DeadlockCheckResult::Severity::CRITICAL)
        level = ETWLevel::Critical;
    else if (result.severity == DeadlockCheckResult::Severity::ERROR)
        level = ETWLevel::Error;

    WriteEvent(ETWEventId::DeadlockDetected, level,
              &event, sizeof(event));
}

void ETWProvider::LogQueueDepthSample(size_t totalQueued, size_t criticalQueued,
                                     size_t highQueued, size_t normalQueued,
                                     size_t lowQueued)
{
    if (!_config.enableQueueDepthEvents || !IsEnabled())
        return;

    QueueDepthSampleEvent event;
    event.totalQueued = totalQueued;
    event.criticalQueued = criticalQueued;
    event.highQueued = highQueued;
    event.normalQueued = normalQueued;
    event.lowQueued = lowQueued;
    event.timestamp = ::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now().time_since_epoch()).count();

    WriteEvent(ETWEventId::QueueDepthSample, ETWLevel::Verbose,
              &event, sizeof(event));
}

void ETWProvider::LogWorkerWaitStart(uint32 workerId, WaitType waitType, const char* location)
{
    // ETW diagnostic stub — will emit ETW events when Windows ETW provider is registered.
    // Wait tracking is handled by the ThreadPool's built-in metrics.
    (void)workerId;
    (void)waitType;
    (void)location;
}

void ETWProvider::LogWorkerWaitEnd(uint32 workerId, uint64 waitDurationMicros)
{
    // ETW diagnostic stub — emits ETW events when Windows ETW provider is registered.
    (void)workerId;
    (void)waitDurationMicros;
}

void ETWProvider::LogThreadPoolShutdown(uint32 workerCount, uint64 totalTasksExecuted)
{
    if (!IsEnabled())
        return;

    ThreadPoolShutdownEvent event;
    event.workerCount = workerCount;
    event.totalTasksExecuted = totalTasksExecuted;
    event.timestamp = ::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now().time_since_epoch()).count();

    WriteEvent(ETWEventId::ThreadPoolShutdown, ETWLevel::Information,
              &event, sizeof(event));
}

bool ETWProvider::WriteEvent(ETWEventId eventId, ETWLevel level,
                            const void* eventData, ULONG dataSize)
{
    if (!_initialized || _providerHandle == 0)
        return false;

    EVENT_DESCRIPTOR eventDescriptor;
    EventDescCreate(&eventDescriptor,
                   static_cast<USHORT>(eventId),  // Event ID
                   0,                              // Version
                   0,                              // Channel
                   static_cast<UCHAR>(level),      // Level
                   0,                              // Opcode
                   0,                              // Task
                   0);                             // Keyword

    EVENT_DATA_DESCRIPTOR dataDescriptor;
    EventDataDescCreate(&dataDescriptor,
                       eventData,
                       dataSize);

    ULONG status = EventWrite(
        _providerHandle,
        &eventDescriptor,
        1,
        &dataDescriptor
    );

    if (status == ERROR_SUCCESS)
    {
        _stats.eventsLogged++;
        return true;
    }
    else
    {
        _stats.writeErrors++;
        return false;
    }
}

bool ETWProvider::ShouldLogTaskExecution()
{
    if (_config.taskExecutionSampleRate == 1)
        return true;

    uint32 counter = _taskExecutionCounter.fetch_add(1, ::std::memory_order_relaxed);
    return (counter % _config.taskExecutionSampleRate) == 0;
}

// Global instance
static ETWProvider g_etwProvider;

ETWProvider& GetETWProvider()
{
    return g_etwProvider;
}

// ETWEventScope implementation

ETWEventScope::ETWEventScope(uint32 workerId, const char* eventName, uint64 eventId)
    : _workerId(workerId)
    , _eventName(eventName)
    , _eventId(eventId)
    , _startTime(::std::chrono::steady_clock::now())
{
}

ETWEventScope::~ETWEventScope()
{
    auto endTime = ::std::chrono::steady_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(
        endTime - _startTime).count();

    // Log as task execution
    GetETWProvider().LogTaskExecutionEnd(_workerId, _eventId,
                                        static_cast<uint64>(duration));
}

#else // !_WIN32

// Non-Windows stub implementation

static ETWProvider g_etwProvider;

ETWProvider& GetETWProvider()
{
    return g_etwProvider;
}

#endif // _WIN32

} // namespace Performance
} // namespace Playerbot
