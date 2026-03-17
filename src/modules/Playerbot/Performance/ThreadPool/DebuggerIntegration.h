/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * Debugger Integration and Crash Dump Generation
 *
 * Provides advanced debugging capabilities including:
 * - Automatic breakpoint triggering on deadlock detection
 * - Windows minidump generation for post-mortem analysis
 * - Integration with Visual Studio debugger
 * - Crash dump with full memory snapshot
 *
 * Features:
 * - DebugBreak() on CRITICAL deadlock (only when debugger attached)
 * - Full minidump generation with heap, threads, and handles
 * - Automatic dump on unrecoverable deadlock
 * - Post-mortem analysis support
 * - Cross-platform stub for Linux (no-op)
 */

#ifndef PLAYERBOT_DEBUGGER_INTEGRATION_H
#define PLAYERBOT_DEBUGGER_INTEGRATION_H

#include "Define.h"
#include "ThreadPoolDiagnostics.h"
#include <string>
#include <chrono>

#ifdef _WIN32
#include <Windows.h>
#include <DbgHelp.h>
#pragma comment(lib, "Dbghelp.lib")
#endif

namespace Playerbot {
namespace Performance {

// Forward declarations
struct DeadlockCheckResult;

/**
 * @brief Configuration for debugger integration
 */
struct DebuggerIntegrationConfig
{
    // DebugBreak settings
    bool enableAutoBreak{true};              // Trigger DebugBreak() on critical deadlock
    bool breakOnlyWhenAttached{true};        // Only break if debugger is attached
    bool breakOnCritical{true};              // Break on CRITICAL severity
    bool breakOnError{false};                // Break on ERROR severity
    bool breakOnWarning{false};              // Break on WARNING severity

    // Minidump settings
    bool enableMinidumps{true};              // Generate minidumps on deadlock
    bool dumpOnCritical{true};               // Dump on CRITICAL severity
    bool dumpOnError{false};                 // Dump on ERROR severity
    ::std::string dumpDirectory{"logs/dumps/"}; // Where to save dumps

    // Dump type
    enum class DumpType
    {
        MINI,                   // Small dump (~1-5 MB): Stack, modules, basic info
        WITH_DATA_SEGS,         // Medium dump (~10-50 MB): + data segments
        FULL_MEMORY             // Large dump (~500MB-2GB): Complete memory snapshot
    };
    DumpType dumpType{DumpType::WITH_DATA_SEGS};

    // Rate limiting
    ::std::chrono::seconds minTimeBetweenBreaks{60};   // Don't break more than once per minute
    ::std::chrono::seconds minTimeBetweenDumps{300};   // Don't dump more than once per 5 minutes
};

/**
 * @brief Debugger integration manager
 *
 * Handles automatic debugging interventions when deadlocks are detected
 */
class DebuggerIntegration
{
public:
    explicit DebuggerIntegration(DebuggerIntegrationConfig config = {});
    ~DebuggerIntegration() = default;

    // Lifecycle
    void Enable();
    void Disable();
    bool IsEnabled() const { return _enabled; }

    // Configuration
    void SetConfig(DebuggerIntegrationConfig config);
    DebuggerIntegrationConfig GetConfig() const { return _config; }

    // Main handler - called by DeadlockDetector
    void OnDeadlockDetected(const DeadlockCheckResult& result);

    // Manual triggers
    void TriggerBreakpoint(const ::std::string& reason);
    bool GenerateMinidump(const ::std::string& reason);

    // Debugger detection
    static bool IsDebuggerAttached();

    // Statistics
    struct Stats
    {
        uint32 breakpointsTriggered{0};
        uint32 minidumpsGenerated{0};
        uint32 breakpointsSkipped{0};      // Skipped due to rate limiting
        uint32 minidumpsSkipped{0};        // Skipped due to rate limiting
    };
    Stats GetStatistics() const { return _stats; }

private:
    // Internal handlers
    void HandleAutoBreak(const DeadlockCheckResult& result);
    void HandleMinidumpGeneration(const DeadlockCheckResult& result);

    // Breakpoint triggering
    void DoDebugBreak(const ::std::string& reason);

    // Minidump generation (Windows-specific)
#ifdef _WIN32
    bool GenerateMinidumpWindows(const ::std::string& filename, const ::std::string& reason);
    MINIDUMP_TYPE GetMinidumpType() const;
#endif

    // Cross-platform stub
#ifndef _WIN32
    bool GenerateMinidumpStub(const ::std::string& filename, const ::std::string& reason);
#endif

    // Rate limiting
    bool ShouldBreak(const DeadlockCheckResult& result);
    bool ShouldDump(const DeadlockCheckResult& result);

    // File management
    ::std::string GenerateDumpFilename(const ::std::string& reason) const;
    void EnsureDumpDirectoryExists();

    // Helper: Get timestamp string
    ::std::string GetTimestampString() const;

    DebuggerIntegrationConfig _config;
    bool _enabled{true};

    // Rate limiting state
    ::std::chrono::steady_clock::time_point _lastBreakTime;
    ::std::chrono::steady_clock::time_point _lastDumpTime;

    // Statistics
    Stats _stats;
};

/**
 * @brief RAII helper to trigger DebugBreak on scope exit
 *
 * Usage:
 *   if (criticalCondition)
 {
 *       ScopedDebugBreak breakpoint("Critical condition detected");
 *       // ... cleanup code ...
 *       // DebugBreak() called on scope exit
 *   }
 */
class ScopedDebugBreak
{
public:
    explicit ScopedDebugBreak(::std::string reason)
        : _reason(::std::move(reason))
        , _shouldBreak(DebuggerIntegration::IsDebuggerAttached())
    {
    }

    ~ScopedDebugBreak()
    {
        if (_shouldBreak)
        {
#ifdef _WIN32
            OutputDebugStringA(("ScopedDebugBreak: " + _reason + "\n").c_str());
            __debugbreak();
#endif
        }
    }

    // Disable breaking
    void Cancel() { _shouldBreak = false; }

private:
    ::std::string _reason;
    bool _shouldBreak;
};

/**
 * @brief Minidump exception filter for SEH
 *
 * Can be installed as a top-level exception handler to capture crashes
 */
#ifdef _WIN32
LONG WINAPI MinidumpExceptionFilter(EXCEPTION_POINTERS* exceptionInfo);
#endif

} // namespace Performance
} // namespace Playerbot

#endif // PLAYERBOT_DEBUGGER_INTEGRATION_H
