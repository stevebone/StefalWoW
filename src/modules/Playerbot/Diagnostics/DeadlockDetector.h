/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * ENTERPRISE-GRADE DEADLOCK DETECTION SYSTEM
 *
 * Provides comprehensive deadlock detection with:
 * - Call stack capture
 * - Thread state monitoring
 * - Mutex ownership tracking
 * - Automatic diagnostic dumps
 * - Visual Studio integration
 */

#ifndef DEADLOCK_DETECTOR_H
#define DEADLOCK_DETECTOR_H

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "ObjectGuid.h"
#include <string>
#include <vector>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <thread>

namespace Playerbot {
namespace Diagnostics {

/**
 * Represents a single frame in a call stack
 */
struct CallStackFrame
{
    ::std::string functionName;
    ::std::string fileName;
    uint32 lineNumber{0};
    uintptr_t address{0};
};

/**
 * Represents the state of a thread at a specific point in time
 */
struct ThreadState
{
    ::std::thread::id threadId;
    ::std::string threadName;
    ::std::chrono::steady_clock::time_point captureTime;
    ::std::vector<CallStackFrame> callStack;
    bool isWaiting{false};
    ::std::string waitingOn; // Mutex/future/condition variable identifier
};

/**
 * Represents a detected deadlock with full diagnostic information
 */
struct DeadlockReport
{
    ::std::chrono::steady_clock::time_point detectionTime;
    ::std::vector<ThreadState> involvedThreads;
    ::std::string description;
    ::std::string suggestedFix;

    // Specific to bot updates
    ObjectGuid botGuid;
    uint32 futureIndex{0};
    uint32 totalFutures{0};
    uint32 waitTimeMs{0};
};

/**
 * Thread-safe deadlock detection and diagnostics
 *
 * Features:
 * - Real-time thread state monitoring
 * - Call stack capture using Win32/DbgHelp APIs
 * - Mutex ownership graph construction
 * - Automatic deadlock detection
 * - Rich diagnostic output for Visual Studio
 */
class TC_GAME_API DeadlockDetector final 
{
public:
    static DeadlockDetector* instance()
    {
        static DeadlockDetector instance;
        return &instance;
    }

    // Initialization
    bool Initialize();
    void Shutdown();

    // Thread registration (for named threads)
    void RegisterThread(::std::thread::id threadId, ::std::string const& name);
    void UnregisterThread(::std::thread::id threadId);

    // Deadlock detection
    DeadlockReport DetectFutureDeadlock(
        ObjectGuid botGuid,
        uint32 futureIndex,
        uint32 totalFutures,
        uint32 waitTimeMs,
        ::std::thread::id waitingThreadId);

    // Call stack capture
    ::std::vector<CallStackFrame> CaptureCallStack(uint32 skipFrames = 0, uint32 maxFrames = 64);
    ThreadState CaptureThreadState(::std::thread::id threadId);

    // Diagnostic output
    void DumpDeadlockReport(DeadlockReport const& report, ::std::string const& outputFile);
    void LogDeadlockReport(DeadlockReport const& report);

    // Visual Studio integration
    void WriteVisualStudioBreakpointFile(DeadlockReport const& report);
    void LaunchVisualStudioDebugger(DeadlockReport const& report);

    // Configuration
    void SetCallStackCaptureEnabled(bool enabled) override { _captureCallStacks = enabled; }
    void SetAutoLaunchDebugger(bool enabled) override { _autoLaunchDebugger = enabled; }
    void SetDumpDirectory(::std::string const& dir) override { _dumpDirectory = dir; }

    // Statistics
    uint32 GetTotalDeadlocksDetected() const override { return _totalDeadlocks; }
    ::std::vector<DeadlockReport> GetRecentDeadlocks(uint32 count = 10) const;

private:
    DeadlockDetector() = default;
    ~DeadlockDetector() = default;
    DeadlockDetector(const DeadlockDetector&) = delete;
    DeadlockDetector& operator=(const DeadlockDetector&) = delete;

    // Platform-specific call stack capture
    ::std::vector<CallStackFrame> CaptureCallStackWindows(uint32 skipFrames, uint32 maxFrames);
    ::std::vector<CallStackFrame> CaptureCallStackLinux(uint32 skipFrames, uint32 maxFrames);

    // Analysis helpers
    ::std::string AnalyzeFutureTimeout(ObjectGuid botGuid, uint32 waitTimeMs);
    ::std::string GenerateSuggestedFix(DeadlockReport const& report);

    // Configuration
    bool _initialized{false};
    bool _captureCallStacks{true};
    bool _autoLaunchDebugger{false};
    ::std::string _dumpDirectory{"./deadlock_dumps"};

    // Thread tracking
    mutable Playerbot::OrderedMutex<Playerbot::LockOrder::BEHAVIOR_MANAGER> _threadsMutex;
    ::std::unordered_map<::std::thread::id, ::std::string> _threadNames;

    // Statistics
    ::std::atomic<uint32> _totalDeadlocks{0};
    mutable Playerbot::OrderedMutex<Playerbot::LockOrder::BEHAVIOR_MANAGER> _reportsMutex;
    ::std::vector<DeadlockReport> _recentReports;
    constexpr static size_t MAX_RECENT_REPORTS = 50;
};

// Global instance accessor
#define sDeadlockDetector DeadlockDetector::instance()

} // namespace Diagnostics
} // namespace Playerbot

#endif // DEADLOCK_DETECTOR_H
