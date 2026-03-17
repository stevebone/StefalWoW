/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * Debugger Integration Implementation
 */

#include "DebuggerIntegration.h"
#include "DeadlockDetector.h"
#include "ThreadPoolDiagnostics.h"
#include "Log.h"
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <ctime>

#ifdef _WIN32
#include <process.h>
// Undefine Windows macros that conflict with our enums
#ifdef ERROR
#undef ERROR
#endif
#ifdef CRITICAL
#undef CRITICAL
#endif
#endif

namespace Playerbot {
namespace Performance {

DebuggerIntegration::DebuggerIntegration(DebuggerIntegrationConfig config)
    : _config(::std::move(config))
    , _lastBreakTime(::std::chrono::steady_clock::now() - ::std::chrono::hours(1))
    , _lastDumpTime(::std::chrono::steady_clock::now() - ::std::chrono::hours(1))
{
    EnsureDumpDirectoryExists();
}

void DebuggerIntegration::Enable()
{
    _enabled = true;
    TC_LOG_INFO("playerbot.threadpool.debugger",
        "Debugger integration enabled (AutoBreak: {}, Minidumps: {})",
        _config.enableAutoBreak, _config.enableMinidumps);
}

void DebuggerIntegration::Disable()
{
    _enabled = false;
    TC_LOG_INFO("playerbot.threadpool.debugger",
        "Debugger integration disabled");
}

void DebuggerIntegration::SetConfig(DebuggerIntegrationConfig config)
{
    _config = ::std::move(config);
    EnsureDumpDirectoryExists();
}

void DebuggerIntegration::OnDeadlockDetected(const DeadlockCheckResult& result)
{
    if (!_enabled)
        return;

    // Handle automatic breakpoint
    if (_config.enableAutoBreak)
    {
        HandleAutoBreak(result);
    }

    // Handle minidump generation
    if (_config.enableMinidumps)
    {
        HandleMinidumpGeneration(result);
    }
}

void DebuggerIntegration::HandleAutoBreak(const DeadlockCheckResult& result)
{
    if (!ShouldBreak(result))
    {
        _stats.breakpointsSkipped++;
        return;
    }

    // Check if debugger is attached (if required)
    if (_config.breakOnlyWhenAttached && !IsDebuggerAttached())
    {
        TC_LOG_DEBUG("playerbot.threadpool.debugger",
            "Skipping DebugBreak - no debugger attached");
        return;
    }

    // Trigger breakpoint
    DoDebugBreak(result.description);
    _stats.breakpointsTriggered++;
    _lastBreakTime = ::std::chrono::steady_clock::now();
}

void DebuggerIntegration::HandleMinidumpGeneration(const DeadlockCheckResult& result)
{
    if (!ShouldDump(result))
    {
        _stats.minidumpsSkipped++;
        return;
    }

    // Generate minidump
    ::std::string reason = result.description;
    if (GenerateMinidump(reason))
    {
        _stats.minidumpsGenerated++;
        _lastDumpTime = ::std::chrono::steady_clock::now();
    }
}

bool DebuggerIntegration::ShouldBreak(const DeadlockCheckResult& result)
{
    // Check severity
    bool severityMatch = false;
    switch (result.severity)
    {
        case DeadlockCheckResult::Severity::CRITICAL:
            severityMatch = _config.breakOnCritical;
            break;
        case DeadlockCheckResult::Severity::ERROR:
            severityMatch = _config.breakOnError;
            break;
        case DeadlockCheckResult::Severity::WARNING:
            severityMatch = _config.breakOnWarning;
            break;
        default:
            return false;
    }

    if (!severityMatch)
        return false;

    // Check rate limiting
    auto now = ::std::chrono::steady_clock::now();
    auto timeSinceLastBreak = ::std::chrono::duration_cast<::std::chrono::seconds>(
        now - _lastBreakTime);

    return timeSinceLastBreak >= _config.minTimeBetweenBreaks;
}

bool DebuggerIntegration::ShouldDump(const DeadlockCheckResult& result)
{
    // Check severity
    bool severityMatch = false;
    switch (result.severity)
    {
        case DeadlockCheckResult::Severity::CRITICAL:
            severityMatch = _config.dumpOnCritical;
            break;
        case DeadlockCheckResult::Severity::ERROR:
            severityMatch = _config.dumpOnError;
            break;
        default:
            return false;
    }

    if (!severityMatch)
        return false;

    // Check rate limiting
    auto now = ::std::chrono::steady_clock::now();
    auto timeSinceLastDump = ::std::chrono::duration_cast<::std::chrono::seconds>(
        now - _lastDumpTime);

    return timeSinceLastDump >= _config.minTimeBetweenDumps;
}

void DebuggerIntegration::DoDebugBreak(const ::std::string& reason)
{
#ifdef _WIN32
    TC_LOG_ERROR("playerbot.threadpool.debugger",
        "TRIGGERING DEBUGBREAK: {}", reason);

    // Output to debugger console
    ::std::string debugMessage = "=== THREADPOOL DEADLOCK DETECTED ===\n" + reason + "\n";
    OutputDebugStringA(debugMessage.c_str());

    // Trigger breakpoint
    __debugbreak();
#else
    TC_LOG_ERROR("playerbot.threadpool.debugger",
        "DebugBreak requested but not supported on this platform: {}", reason);
#endif
}

void DebuggerIntegration::TriggerBreakpoint(const ::std::string& reason)
{
    if (!_enabled)
        return;

    if (_config.breakOnlyWhenAttached && !IsDebuggerAttached())
    {
        TC_LOG_DEBUG("playerbot.threadpool.debugger",
            "Manual breakpoint skipped - no debugger attached");
        return;
    }

    DoDebugBreak(reason);
    _stats.breakpointsTriggered++;
}

bool DebuggerIntegration::GenerateMinidump(const ::std::string& reason)
{
    ::std::string filename = GenerateDumpFilename(reason);

#ifdef _WIN32
    return GenerateMinidumpWindows(filename, reason);
#else
    return GenerateMinidumpStub(filename, reason);
#endif
}

#ifdef _WIN32
bool DebuggerIntegration::GenerateMinidumpWindows(const ::std::string& filename, const ::std::string& reason)
{
    try
    {
        // Open file
        HANDLE hFile = CreateFileA(
            filename.c_str(),
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        if (hFile == INVALID_HANDLE_VALUE)
        {
            TC_LOG_ERROR("playerbot.threadpool.debugger",
                "Failed to create minidump file: {} (error: {})",
                filename, GetLastError());
            return false;
        }

        // Prepare minidump info
        MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
        ZeroMemory(&exceptionInfo, sizeof(exceptionInfo));
        exceptionInfo.ThreadId = GetCurrentThreadId();
        exceptionInfo.ExceptionPointers = NULL;
        exceptionInfo.ClientPointers = FALSE;

        // Get minidump type
        MINIDUMP_TYPE dumpType = GetMinidumpType();

        // Write minidump
        BOOL success = MiniDumpWriteDump(
            GetCurrentProcess(),
            GetCurrentProcessId(),
            hFile,
            dumpType,
            NULL, // No exception info (not a crash)
            NULL, // No user stream
            NULL  // No callback
        );

        CloseHandle(hFile);

        if (success)
        {
            TC_LOG_WARN("playerbot.threadpool.debugger",
                "Minidump generated: {} (Reason: {})", filename, reason);

            // Get file size
            try
            {
                auto fileSize = ::std::filesystem::file_size(filename);
                TC_LOG_INFO("playerbot.threadpool.debugger",
                    "Minidump size: {} MB", fileSize / (1024 * 1024));
            }
            catch (...) {}

            return true;
        }
        else
        {
            TC_LOG_ERROR("playerbot.threadpool.debugger",
                "MiniDumpWriteDump failed (error: {})", GetLastError());
            return false;
        }
    }
    catch (const ::std::exception& e)
    {
        TC_LOG_ERROR("playerbot.threadpool.debugger",
            "Exception while generating minidump: {}", e.what());
        return false;
    }
}

MINIDUMP_TYPE DebuggerIntegration::GetMinidumpType() const
{
    switch (_config.dumpType)
    {
        case DebuggerIntegrationConfig::DumpType::MINI:
            return MiniDumpNormal;

        case DebuggerIntegrationConfig::DumpType::WITH_DATA_SEGS:
            return static_cast<MINIDUMP_TYPE>(
                MiniDumpWithDataSegs |
                MiniDumpWithHandleData |
                MiniDumpWithThreadInfo |
                MiniDumpWithProcessThreadData |
                MiniDumpWithFullMemoryInfo
            );

        case DebuggerIntegrationConfig::DumpType::FULL_MEMORY:
            return static_cast<MINIDUMP_TYPE>(
                MiniDumpWithFullMemory |
                MiniDumpWithDataSegs |
                MiniDumpWithHandleData |
                MiniDumpWithThreadInfo |
                MiniDumpWithProcessThreadData |
                MiniDumpWithFullMemoryInfo
            );

        default:
            return MiniDumpNormal;
    }
}

// Top-level exception handler
LONG WINAPI MinidumpExceptionFilter(EXCEPTION_POINTERS* exceptionInfo)
{
    try
    {
        // Generate crash dump filename
        auto now = ::std::chrono::system_clock::now();
        auto time_t_val = ::std::chrono::system_clock::to_time_t(now);
        ::std::tm tm_val = *::std::localtime(&time_t_val);

        ::std::stringstream ss;
        ss << "logs/dumps/crash_"
           << ::std::put_time(&tm_val, "%Y%m%d_%H%M%S")
           << ".dmp";
        ::std::string filename = ss.str();

        // Ensure directory exists
        ::std::filesystem::create_directories("logs/dumps/");

        // Create dump file
        HANDLE hFile = CreateFileA(
            filename.c_str(),
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        if (hFile != INVALID_HANDLE_VALUE)
        {
            MINIDUMP_EXCEPTION_INFORMATION exInfo;
            exInfo.ThreadId = GetCurrentThreadId();
            exInfo.ExceptionPointers = exceptionInfo;
            exInfo.ClientPointers = FALSE;

            // Full memory dump for crashes
            MINIDUMP_TYPE dumpType = static_cast<MINIDUMP_TYPE>(
                MiniDumpWithFullMemory |
                MiniDumpWithDataSegs |
                MiniDumpWithHandleData |
                MiniDumpWithThreadInfo |
                MiniDumpWithProcessThreadData |
                MiniDumpWithFullMemoryInfo
            );

            MiniDumpWriteDump(
                GetCurrentProcess(),
                GetCurrentProcessId(),
                hFile,
                dumpType,
                &exInfo,
                NULL,
                NULL
            );

            CloseHandle(hFile);
        }
    }
    catch (...)
    {
        // Don't let exception handler crash
    }

    return EXCEPTION_EXECUTE_HANDLER;
}
#endif // _WIN32

#ifndef _WIN32
bool DebuggerIntegration::GenerateMinidumpStub(const ::std::string& filename, const ::std::string& reason)
{
    TC_LOG_WARN("playerbot.threadpool.debugger",
        "Minidump generation requested but not supported on this platform: {}",
        reason);
    return false;
}
#endif

bool DebuggerIntegration::IsDebuggerAttached()
{
#ifdef _WIN32
    return IsDebuggerPresent() != 0;
#else
    // Linux: Check /proc/self/status for TracerPid
    // For now, return false
    return false;
#endif
}

::std::string DebuggerIntegration::GenerateDumpFilename(const ::std::string& reason) const
{
    auto now = ::std::chrono::system_clock::now();
    auto time_t_val = ::std::chrono::system_clock::to_time_t(now);
    ::std::tm tm_val = *::std::localtime(&time_t_val);

    ::std::stringstream ss;
    ss << _config.dumpDirectory
       << "threadpool_deadlock_"
       << ::std::put_time(&tm_val, "%Y%m%d_%H%M%S")
       << ".dmp";

    return ss.str();
}

void DebuggerIntegration::EnsureDumpDirectoryExists()
{
    try
    {
        ::std::filesystem::create_directories(_config.dumpDirectory);
    }
    catch (const ::std::exception& e)
    {
        TC_LOG_ERROR("playerbot.threadpool.debugger",
            "Failed to create dump directory {}: {}",
            _config.dumpDirectory, e.what());
    }
}

::std::string DebuggerIntegration::GetTimestampString() const
{
    auto now = ::std::chrono::system_clock::now();
    auto time_t_val = ::std::chrono::system_clock::to_time_t(now);
    ::std::tm tm_val = *::std::localtime(&time_t_val);

    ::std::stringstream ss;
    ss << ::std::put_time(&tm_val, "%Y%m%d_%H%M%S");
    return ss.str();
}

} // namespace Performance
} // namespace Playerbot
