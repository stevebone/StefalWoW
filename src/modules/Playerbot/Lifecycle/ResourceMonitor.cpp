/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ResourceMonitor.h"
#include "Config/PlayerbotConfig.h"
#include "Log.h"
#include "DatabaseEnv.h"
#include "MapManager.h"
#include "World.h"
#include "GameTime.h"

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#elif defined(__APPLE__)
#include <sys/times.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <unistd.h>
#include <fstream>
#else  // Linux
#include <sys/times.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <fstream>
#endif

namespace Playerbot
{

// ============================================================================
// ResourceMetrics Implementation
// ============================================================================

ResourcePressure ResourceMetrics::GetPressureLevel() const
{
    // Use the WORST pressure level from CPU or memory
    ResourcePressure cpuPressure = ResourcePressure::NORMAL;
    ResourcePressure memPressure = ResourcePressure::NORMAL;

    // Determine CPU pressure (using 30s average to smooth spikes)
    if (cpuUsage30sAvg >= 85.0f)
        cpuPressure = ResourcePressure::CRITICAL;
    else if (cpuUsage30sAvg >= 75.0f)
        cpuPressure = ResourcePressure::HIGH;
    else if (cpuUsage30sAvg >= 60.0f)
        cpuPressure = ResourcePressure::ELEVATED;

    // Determine memory pressure
    if (memoryUsagePercent >= 90.0f)
        memPressure = ResourcePressure::CRITICAL;
    else if (memoryUsagePercent >= 80.0f)
        memPressure = ResourcePressure::HIGH;
    else if (memoryUsagePercent >= 70.0f)
        memPressure = ResourcePressure::ELEVATED;

    // Return the more severe pressure level
    return ::std::max(cpuPressure, memPressure);
}

bool ResourceMetrics::IsSpawningSafe() const
{
    // Spawning is unsafe in CRITICAL pressure
    return GetPressureLevel() != ResourcePressure::CRITICAL;
}

float ResourceMetrics::GetSpawnRateMultiplier() const
{
    // Return spawn rate multiplier based on pressure level
    switch (GetPressureLevel())
    {
        case ResourcePressure::NORMAL:
            return 1.0f;    // 100% spawn rate
        case ResourcePressure::ELEVATED:
            return 0.5f;    // 50% spawn rate
        case ResourcePressure::HIGH:
            return 0.25f;   // 25% spawn rate
        case ResourcePressure::CRITICAL:
            return 0.0f;    // 0% spawn rate (pause)
        default:
            return 1.0f;
    }
}

// ============================================================================
// ResourceThresholds Implementation
// ============================================================================

void ResourceThresholds::LoadFromConfig()
{
    // Load CPU thresholds
    cpuNormalThreshold = sPlayerbotConfig->GetFloat("Playerbot.ResourceMonitor.CpuThreshold.Elevated", 60.0f);
    cpuElevatedThreshold = sPlayerbotConfig->GetFloat("Playerbot.ResourceMonitor.CpuThreshold.High", 75.0f);
    cpuHighThreshold = sPlayerbotConfig->GetFloat("Playerbot.ResourceMonitor.CpuThreshold.Critical", 85.0f);

    // Load memory thresholds
    memoryNormalThreshold = sPlayerbotConfig->GetFloat("Playerbot.ResourceMonitor.MemoryThreshold.Elevated", 70.0f);
    memoryElevatedThreshold = sPlayerbotConfig->GetFloat("Playerbot.ResourceMonitor.MemoryThreshold.High", 80.0f);
    memoryHighThreshold = sPlayerbotConfig->GetFloat("Playerbot.ResourceMonitor.MemoryThreshold.Critical", 90.0f);

    // Load DB threshold
    dbConnectionThreshold = sPlayerbotConfig->GetFloat("Playerbot.ResourceMonitor.DbConnectionThreshold", 80.0f);

    TC_LOG_INFO("module.playerbot.resource",
        "ResourceMonitor thresholds loaded: CPU({}%/{}%/{}%), Memory({}%/{}%/{}%), DB({}%)",
        cpuNormalThreshold, cpuElevatedThreshold, cpuHighThreshold,
        memoryNormalThreshold, memoryElevatedThreshold, memoryHighThreshold,
        dbConnectionThreshold);
}

// ============================================================================
// ResourceMonitor Implementation
// ============================================================================

ResourceMonitor::ResourceMonitor()
{
}

ResourceMonitor::~ResourceMonitor()
{
#ifdef _WIN32
    if (_processHandle)
    {
        CloseHandle(_processHandle);
        _processHandle = nullptr;
    }
#endif
}

bool ResourceMonitor::Initialize()
{
    if (_initialized)
        return true;

    TC_LOG_INFO("module.playerbot.resource", "Initializing ResourceMonitor...");

    // Load configuration thresholds
    _thresholds.LoadFromConfig();

#ifdef _WIN32
    // Windows: Open process handle for CPU monitoring
    _processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId());

    // Initialize CPU time tracking
    FILETIME creationTime, exitTime, kernelTime, userTime;
    if (GetProcessTimes(_processHandle, &creationTime, &exitTime, &kernelTime, &userTime))
    {
        _lastCpuTime = ((uint64)kernelTime.dwHighDateTime << 32) | kernelTime.dwLowDateTime;
        _lastCpuTime += ((uint64)userTime.dwHighDateTime << 32) | userTime.dwLowDateTime;
    }

    FILETIME systemTime;
    GetSystemTimeAsFileTime(&systemTime);
    _lastSystemTime = ((uint64)systemTime.dwHighDateTime << 32) | systemTime.dwLowDateTime;

#else
    // Linux: Initialize CPU time tracking
    struct tms timeSample;
    _lastTimestamp = times(&timeSample);
    _lastCpuTime = timeSample.tms_utime + timeSample.tms_stime;
#endif

    _initialized = true;
    TC_LOG_INFO("module.playerbot.resource", " ResourceMonitor initialized successfully");
    return true;
}

void ResourceMonitor::Update(uint32 diff)
{
    if (!_initialized)
        return;

    _timeSinceLastUpdate += diff;

    // Update metrics every 1 second
    if (_timeSinceLastUpdate >= _updateInterval)
    {
        _timeSinceLastUpdate = 0;

        // Collect all metrics
        _currentMetrics.cpuUsagePercent = CollectCpuUsage();
        _currentMetrics.memoryUsagePercent = CollectMemoryUsage();
        CollectDatabaseMetrics();
        CollectMapMetrics();

        // Update moving averages
        UpdateMovingAverages();

        // Update timestamp
        _currentMetrics.collectionTime = GameTime::Now();

        // Log pressure level changes
        static ResourcePressure lastPressure = ResourcePressure::NORMAL;
        ResourcePressure currentPressure = _currentMetrics.GetPressureLevel();

        if (currentPressure != lastPressure)
        {
            const char* pressureNames[] = { "NORMAL", "ELEVATED", "HIGH", "CRITICAL" };
            TC_LOG_INFO("module.playerbot.resource",
                "Resource pressure changed: {} → {} (CPU: {:.1f}%, Mem: {:.1f}%)",
                pressureNames[static_cast<uint8>(lastPressure)],
                pressureNames[static_cast<uint8>(currentPressure)],
                _currentMetrics.cpuUsage30sAvg,
                _currentMetrics.memoryUsagePercent);

            lastPressure = currentPressure;
        }
    }
}

void ResourceMonitor::ForceUpdate()
{
    if (!_initialized)
        return;

    _currentMetrics.cpuUsagePercent = CollectCpuUsage();
    _currentMetrics.memoryUsagePercent = CollectMemoryUsage();
    CollectDatabaseMetrics();
    CollectMapMetrics();
    UpdateMovingAverages();
    _currentMetrics.collectionTime = GameTime::Now();
}

float ResourceMonitor::CollectCpuUsage()
{
#ifdef _WIN32
    // Windows implementation using GetProcessTimes
    FILETIME creationTime, exitTime, kernelTime, userTime;
    if (!GetProcessTimes(_processHandle, &creationTime, &exitTime, &kernelTime, &userTime))
        return 0.0f;

    uint64 currentCpuTime = ((uint64)kernelTime.dwHighDateTime << 32) | kernelTime.dwLowDateTime;
    currentCpuTime += ((uint64)userTime.dwHighDateTime << 32) | userTime.dwLowDateTime;

    FILETIME systemTime;
    GetSystemTimeAsFileTime(&systemTime);
    uint64 currentSystemTime = ((uint64)systemTime.dwHighDateTime << 32) | systemTime.dwLowDateTime;

    // Calculate CPU usage percentage
    uint64 cpuDelta = currentCpuTime - _lastCpuTime;
    uint64 systemDelta = currentSystemTime - _lastSystemTime;

    float cpuUsage = 0.0f;
    if (systemDelta > 0)
    {
        // Convert to percentage (account for multiple cores)
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        cpuUsage = (float)(cpuDelta * 100.0 / systemDelta / sysInfo.dwNumberOfProcessors);
    }

    // Update tracking variables
    _lastCpuTime = currentCpuTime;
    _lastSystemTime = currentSystemTime;

    return ::std::min(cpuUsage, 100.0f);

#else
    // Linux implementation using /proc/stat
    struct tms timeSample;
    clock_t now = times(&timeSample);

    if (now <= _lastTimestamp || _lastTimestamp == 0)
        return 0.0f;

    uint64 currentCpuTime = timeSample.tms_utime + timeSample.tms_stime;
    uint64 cpuDelta = currentCpuTime - _lastCpuTime;
    uint64 timeDelta = now - _lastTimestamp;

    float cpuUsage = 0.0f;
    if (timeDelta > 0)
    {
        long clockTicks = sysconf(_SC_CLK_TCK);
        cpuUsage = (float)(cpuDelta * 100.0 / timeDelta / clockTicks);
    }

    _lastCpuTime = currentCpuTime;
    _lastTimestamp = now;

    return ::std::min(cpuUsage, 100.0f);
#endif
}

float ResourceMonitor::CollectMemoryUsage()
{
#ifdef _WIN32
    // Windows implementation using GetProcessMemoryInfo
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (!GetProcessMemoryInfo(_processHandle, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
        return 0.0f;

    _currentMetrics.workingSetMB = pmc.WorkingSetSize / (1024 * 1024);
    _currentMetrics.commitSizeMB = pmc.PrivateUsage / (1024 * 1024);

    // Get total system memory
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (!GlobalMemoryStatusEx(&memInfo))
        return 0.0f;

    // Calculate percentage of total physical memory
    float memoryUsagePercent = (float)(pmc.WorkingSetSize * 100.0 / memInfo.ullTotalPhys);
    return ::std::min(memoryUsagePercent, 100.0f);

#elif defined(__APPLE__)
    // macOS implementation using mach API
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &infoCount) != KERN_SUCCESS)
        return 0.0f;

    _currentMetrics.workingSetMB = info.resident_size / (1024 * 1024);
    _currentMetrics.commitSizeMB = info.virtual_size / (1024 * 1024);

    // Get total system memory via sysctl
    int64_t totalMemory = 0;
    size_t size = sizeof(totalMemory);
    if (sysctlbyname("hw.memsize", &totalMemory, &size, nullptr, 0) != 0)
        return 0.0f;

    float memoryUsagePercent = (float)(info.resident_size * 100.0 / totalMemory);
    return ::std::min(memoryUsagePercent, 100.0f);

#else
    // Linux implementation using /proc/self/statm
    ::std::ifstream statm("/proc/self/statm");
    if (!statm)
        return 0.0f;

    long pageSize = sysconf(_SC_PAGESIZE);
    unsigned long vmSize, vmRSS;
    statm >> vmSize >> vmRSS;
    statm.close();

    _currentMetrics.workingSetMB = (vmRSS * pageSize) / (1024 * 1024);
    _currentMetrics.commitSizeMB = (vmSize * pageSize) / (1024 * 1024);

    // Get total system memory
    struct sysinfo memInfo;
    if (sysinfo(&memInfo) != 0)
        return 0.0f;

    float memoryUsagePercent = (float)((vmRSS * pageSize) * 100.0 / memInfo.totalram);
    return ::std::min(memoryUsagePercent, 100.0f);
#endif
}

void ResourceMonitor::CollectDatabaseMetrics()
{
    // LIMITATION: TrinityCore's DatabaseWorkerPool does not expose active connection count.
    // Using placeholder values. For real monitoring, extend DatabaseWorkerPool with GetActiveConnectionCount().
    _currentMetrics.activeDbConnections = 0;
    _currentMetrics.maxDbConnections = 100;  // Default TrinityCore pool size
    _currentMetrics.dbConnectionUsagePercent = 0.0f;

    // This would require access to DatabaseWorkerPool internals
    // For Phase 2, we'll monitor indirectly via query latency
}

void ResourceMonitor::CollectMapMetrics()
{
    // Get map instance count
    _currentMetrics.activeMapInstances = sMapMgr->GetNumInstances();

    // Get total active bots (will be set by BotSpawner)
    // _currentMetrics.totalActiveBots is updated externally by BotSpawner::Update()
}

void ResourceMonitor::UpdateMovingAverages()
{
    // Add current CPU sample to windows
    _cpuSamples5s.push_back(_currentMetrics.cpuUsagePercent);
    _cpuSamples30s.push_back(_currentMetrics.cpuUsagePercent);
    _cpuSamples60s.push_back(_currentMetrics.cpuUsagePercent);

    // Maintain window sizes (@ 1Hz sampling)
    while (_cpuSamples5s.size() > 5)
        _cpuSamples5s.pop_front();

    while (_cpuSamples30s.size() > 30)
        _cpuSamples30s.pop_front();

    while (_cpuSamples60s.size() > 60)
        _cpuSamples60s.pop_front();

    // Calculate averages
    _currentMetrics.cpuUsage5sAvg = CalculateAverage(_cpuSamples5s);
    _currentMetrics.cpuUsage30sAvg = CalculateAverage(_cpuSamples30s);
    _currentMetrics.cpuUsage60sAvg = CalculateAverage(_cpuSamples60s);
}

float ResourceMonitor::CalculateAverage(const ::std::deque<float>& window) const
{
    if (window.empty())
        return 0.0f;

    float sum = 0.0f;
    for (float sample : window)
        sum += sample;

    return sum / window.size();
}

} // namespace Playerbot
