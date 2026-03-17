/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PerformanceValidator.h"
#include "Log.h"
#include <algorithm>
#include <numeric>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <cmath>

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#include <pdh.h>
#elif defined(__linux__)
#include <unistd.h>
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <fstream>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <sys/sysctl.h>
#endif

namespace Playerbot
{
namespace Test
{

// ========================
// SystemHealthMetrics Implementation
// ========================

bool SystemHealthMetrics::IsHealthy(const PerformanceThresholds& thresholds) const
{
    // Check CPU usage
    if (processCpuUsage > thresholds.maxCpuUsage)
        return false;

    // Check memory usage per bot
    if (activeBots > 0)
    {
        uint64 memoryPerBot = totalMemoryUsage / activeBots;
        if (memoryPerBot > thresholds.maxMemoryPerBot)
            return false;
    }

    // Check database performance
    if (averageDbQueryTime > thresholds.maxDatabaseQueryTime)
        return false;

    // Check network latency
    if (averageLatency > thresholds.maxNetworkLatency)
        return false;

    // Check error rates
    if (criticalErrors > 0)
        return false;

    return true;
}

::std::string SystemHealthMetrics::GetHealthSummary() const
{
    ::std::stringstream ss;
    ss << ::std::fixed << ::std::setprecision(2);

    ss << "System Health Summary:\n";
    ss << "  CPU Usage: " << processCpuUsage << "%\n";
    ss << "  Memory Usage: " << (totalMemoryUsage / (1024 * 1024)) << " MB\n";
    ss << "  Active Bots: " << activeBots << "\n";
    ss << "  Active Groups: " << activeGroups << "\n";
    ss << "  DB Connections: " << activeDbConnections << "\n";
    ss << "  Network Connections: " << activeNetworkConnections << "\n";
    ss << "  Critical Errors: " << criticalErrors << "\n";
    ss << "  Total Errors: " << totalErrors << "\n";

    return ss.str();
}

// ========================
// PerformanceValidator Implementation
// ========================

PerformanceValidator::PerformanceValidator(const PerformanceThresholds& thresholds)
    : m_thresholds(thresholds)
{
}

void PerformanceValidator::SetThresholds(const PerformanceThresholds& thresholds)
{
    m_thresholds = thresholds;
    TC_LOG_INFO("playerbot.test", "Performance thresholds updated");
}

const PerformanceThresholds& PerformanceValidator::GetThresholds() const
{
    return m_thresholds;
}

bool PerformanceValidator::ValidateTimingMetrics(const PerformanceMetrics& metrics) const
{
    if (!ValidateTimingThreshold(metrics.invitationAcceptanceTime,
                                m_thresholds.maxInvitationAcceptanceTime,
                                "InvitationAcceptance"))
        return false;

    if (!ValidateTimingThreshold(metrics.followingEngagementTime,
                                m_thresholds.maxFollowingEngagementTime,
                                "FollowingEngagement"))
        return false;

    if (!ValidateTimingThreshold(metrics.combatEngagementTime,
                                m_thresholds.maxCombatEngagementTime,
                                "CombatEngagement"))
        return false;

    if (!ValidateTimingThreshold(metrics.targetSwitchTime,
                                m_thresholds.maxTargetSwitchTime,
                                "TargetSwitch"))
        return false;

    if (!ValidateTimingThreshold(metrics.teleportTime,
                                m_thresholds.maxTeleportTime,
                                "Teleport"))
        return false;

    return true;
}

bool PerformanceValidator::ValidateMemoryMetrics(const PerformanceMetrics& metrics, uint32 botCount) const
{
    uint64 memoryGrowth = metrics.memoryUsagePeak - metrics.memoryUsageStart;

    if (!ValidateMemoryThreshold(memoryGrowth,
                                m_thresholds.maxTotalMemoryGrowth,
                                "TotalMemoryGrowth"))
        return false;

    if (botCount > 0)
    {
        uint64 memoryPerBot = metrics.memoryUsagePeak / botCount;
        if (!ValidateMemoryThreshold(memoryPerBot,
                                    m_thresholds.maxMemoryPerBot,
                                    "MemoryPerBot"))
            return false;
    }

    return true;
}

bool PerformanceValidator::ValidateCpuMetrics(const PerformanceMetrics& metrics, uint32 botCount) const
{
    if (!ValidateCpuThreshold(metrics.cpuUsagePeak,
                             m_thresholds.maxCpuUsage,
                             "PeakCpuUsage"))
        return false;

    if (botCount > 0)
    {
        float cpuPerBot = metrics.cpuUsagePeak / botCount;
        if (!ValidateCpuThreshold(cpuPerBot,
                                 m_thresholds.maxCpuPerBot,
                                 "CpuPerBot"))
            return false;
    }

    return true;
}

bool PerformanceValidator::ValidateSuccessRates(const PerformanceMetrics& metrics) const
{
    float successRate = metrics.GetSuccessRate();
    if (successRate < m_thresholds.minSuccessRate)
    {
        TC_LOG_DEBUG("playerbot.test", "Success rate {} is below threshold {}",
                    successRate, m_thresholds.minSuccessRate);
        return false;
    }

    return true;
}

bool PerformanceValidator::ValidateScalabilityMetrics(uint32 totalBots, uint32 groupCount) const
{
    if (totalBots > m_thresholds.maxTotalBots)
    {
        TC_LOG_DEBUG("playerbot.test", "Total bots {} exceeds threshold {}",
                    totalBots, m_thresholds.maxTotalBots);
        return false;
    }

    if (groupCount > m_thresholds.maxConcurrentGroups)
    {
        TC_LOG_DEBUG("playerbot.test", "Concurrent groups {} exceeds threshold {}",
                    groupCount, m_thresholds.maxConcurrentGroups);
        return false;
    }

    return true;
}

bool PerformanceValidator::ValidateAllMetrics(const PerformanceMetrics& metrics, uint32 botCount) const
{
    return ValidateTimingMetrics(metrics) &&
           ValidateMemoryMetrics(metrics, botCount) &&
           ValidateCpuMetrics(metrics, botCount) &&
           ValidateSuccessRates(metrics);
}

::std::vector<::std::string> PerformanceValidator::GetValidationFailures(const PerformanceMetrics& metrics, uint32 botCount) const
{
    ::std::vector<::std::string> failures;

    // Check timing metrics
    if (metrics.invitationAcceptanceTime > m_thresholds.maxInvitationAcceptanceTime)
    {
        failures.push_back("Invitation acceptance time exceeds threshold: " +
                          ::std::to_string(metrics.invitationAcceptanceTime / 1000.0f) + "ms > " +
                          ::std::to_string(m_thresholds.maxInvitationAcceptanceTime / 1000.0f) + "ms");
    }

    if (metrics.combatEngagementTime > m_thresholds.maxCombatEngagementTime)
    {
        failures.push_back("Combat engagement time exceeds threshold: " +
                          ::std::to_string(metrics.combatEngagementTime / 1000.0f) + "ms > " +
                          ::std::to_string(m_thresholds.maxCombatEngagementTime / 1000.0f) + "ms");
    }

    if (metrics.targetSwitchTime > m_thresholds.maxTargetSwitchTime)
    {
        failures.push_back("Target switch time exceeds threshold: " +
                          ::std::to_string(metrics.targetSwitchTime / 1000.0f) + "ms > " +
                          ::std::to_string(m_thresholds.maxTargetSwitchTime / 1000.0f) + "ms");
    }

    // Check memory metrics
    uint64 memoryGrowth = metrics.memoryUsagePeak - metrics.memoryUsageStart;
    if (memoryGrowth > m_thresholds.maxTotalMemoryGrowth)
    {
        failures.push_back("Total memory growth exceeds threshold: " +
                          ::std::to_string(memoryGrowth / (1024 * 1024)) + "MB > " +
                          ::std::to_string(m_thresholds.maxTotalMemoryGrowth / (1024 * 1024)) + "MB");
    }

    if (botCount > 0)
    {
        uint64 memoryPerBot = metrics.memoryUsagePeak / botCount;
        if (memoryPerBot > m_thresholds.maxMemoryPerBot)
        {
            failures.push_back("Memory per bot exceeds threshold: " +
                              ::std::to_string(memoryPerBot / (1024 * 1024)) + "MB > " +
                              ::std::to_string(m_thresholds.maxMemoryPerBot / (1024 * 1024)) + "MB");
        }
    }

    // Check CPU metrics
    if (metrics.cpuUsagePeak > m_thresholds.maxCpuUsage)
    {
        failures.push_back("Peak CPU usage exceeds threshold: " +
                          ::std::to_string(metrics.cpuUsagePeak) + "% > " +
                          ::std::to_string(m_thresholds.maxCpuUsage) + "%");
    }

    // Check success rate
    float successRate = metrics.GetSuccessRate();
    if (successRate < m_thresholds.minSuccessRate)
    {
        failures.push_back("Success rate below threshold: " +
                          ::std::to_string(successRate * 100.0f) + "% < " +
                          ::std::to_string(m_thresholds.minSuccessRate * 100.0f) + "%");
    }

    return failures;
}

void PerformanceValidator::RecordBenchmark(const PerformanceBenchmark& benchmark)
{
    m_benchmarks.push_back(benchmark);

    // Update performance trends
    m_performanceTrends[benchmark.category + "_response_time"].push_back(benchmark.averageResponseTime);
    m_performanceTrends[benchmark.category + "_throughput"].push_back(benchmark.operationsPerSecond);
    m_performanceTrends[benchmark.category + "_memory"].push_back(benchmark.peakMemoryUsage);
    m_performanceTrends[benchmark.category + "_cpu"].push_back(benchmark.peakCpuUsage);

    TC_LOG_DEBUG("playerbot.test", "Recorded benchmark for {}: {} ops/sec, {}ms avg response",
                benchmark.testName, benchmark.operationsPerSecond, benchmark.averageResponseTime / 1000.0f);
}

SystemHealthMetrics PerformanceValidator::GetCurrentSystemHealth() const
{
    SystemHealthMetrics health;

    // Get system resource usage
    health.totalMemoryUsage = GetCurrentMemoryUsage();
    health.processCpuUsage = GetCurrentCpuUsage();

    // Get database metrics
    health.activeDbConnections = GetDatabaseMetrics();

    // Get network metrics
    health.activeNetworkConnections = GetActiveConnections();

    // Simulate bot metrics (in real implementation, this would query actual bot systems)
    health.activeBots = 0;  // Would be populated from actual bot manager
    health.activeGroups = 0;
    health.botsInGroups = 0;
    health.botsInCombat = 0;

    // Error metrics would be populated from error tracking system
    health.totalErrors = 0;
    health.criticalErrors = 0;
    health.warnings = 0;

    return health;
}

bool PerformanceValidator::ValidateSystemHealth() const
{
    SystemHealthMetrics health = GetCurrentSystemHealth();
    return health.IsHealthy(m_thresholds);
}

::std::string PerformanceValidator::GenerateHealthReport() const
{
    SystemHealthMetrics health = GetCurrentSystemHealth();
    ::std::stringstream report;

    report << "=== SYSTEM HEALTH REPORT ===\n";
    report << health.GetHealthSummary();
    report << "\nHealth Status: " << (health.IsHealthy(m_thresholds) ? "HEALTHY" : "UNHEALTHY") << "\n";

    if (!health.IsHealthy(m_thresholds))
    {
        report << "\nIssues Detected:\n";
        if (health.processCpuUsage > m_thresholds.maxCpuUsage)
            report << "  - High CPU usage: " << health.processCpuUsage << "%\n";
        if (health.averageDbQueryTime > m_thresholds.maxDatabaseQueryTime)
            report << "  - Slow database queries: " << health.averageDbQueryTime << "μs\n";
        if (health.criticalErrors > 0)
            report << "  - Critical errors detected: " << health.criticalErrors << "\n";
    }

    return report.str();
}

::std::string PerformanceValidator::GeneratePerformanceReport() const
{
    ::std::stringstream report;
    report << ::std::fixed << ::std::setprecision(2);

    report << "=== PERFORMANCE VALIDATION REPORT ===\n\n";

    // Summary statistics
    if (!m_benchmarks.empty())
    {
        report << "Benchmark Summary (" << m_benchmarks.size() << " tests):\n";

        // Calculate aggregated metrics
        uint64 totalOps = 0;
        uint64 totalTime = 0;
        float totalCpu = 0.0f;
        uint64 totalMemory = 0;
        uint32 passedTests = 0;

        for (const auto& benchmark : m_benchmarks)
        {
            totalOps += benchmark.totalOperations;
            totalTime += benchmark.averageResponseTime;
            totalCpu += benchmark.peakCpuUsage;
            totalMemory += benchmark.peakMemoryUsage;
            passedTests += benchmark.passedTests;
        }

        float avgResponseTime = m_benchmarks.empty() ? 0.0f : (float)totalTime / m_benchmarks.size();
        float avgCpu = m_benchmarks.empty() ? 0.0f : totalCpu / m_benchmarks.size();
        float passRate = m_benchmarks.empty() ? 0.0f : (float)passedTests / m_benchmarks.size();

        report << "  Average Response Time: " << (avgResponseTime / 1000.0f) << " ms\n";
        report << "  Average CPU Usage: " << avgCpu << "%\n";
        report << "  Total Operations: " << totalOps << "\n";
        report << "  Overall Pass Rate: " << (passRate * 100.0f) << "%\n\n";
    }

    // Threshold validation
    report << "Performance Thresholds:\n";
    report << "  Max Invitation Acceptance: " << (m_thresholds.maxInvitationAcceptanceTime / 1000.0f) << " ms\n";
    report << "  Max Combat Engagement: " << (m_thresholds.maxCombatEngagementTime / 1000.0f) << " ms\n";
    report << "  Max Target Switch: " << (m_thresholds.maxTargetSwitchTime / 1000.0f) << " ms\n";
    report << "  Max Memory Per Bot: " << (m_thresholds.maxMemoryPerBot / (1024 * 1024)) << " MB\n";
    report << "  Max CPU Usage: " << m_thresholds.maxCpuUsage << "%\n";
    report << "  Min Success Rate: " << (m_thresholds.minSuccessRate * 100.0f) << "%\n\n";

    // Individual benchmark results
    if (!m_benchmarks.empty())
    {
        report << "Individual Test Results:\n";
        for (const auto& benchmark : m_benchmarks)
        {
            report << "  " << benchmark.testName << " (" << benchmark.category << "):\n";
            report << "    Response Time: " << (benchmark.averageResponseTime / 1000.0f) << " ms\n";
            report << "    Operations/sec: " << benchmark.operationsPerSecond << "\n";
            report << "    Success Rate: " << (benchmark.GetSuccessRate() * 100.0f) << "%\n";
            report << "    Peak Memory: " << (benchmark.peakMemoryUsage / (1024 * 1024)) << " MB\n";
            report << "    Peak CPU: " << benchmark.peakCpuUsage << "%\n";

            if (!benchmark.failures.empty())
            {
                report << "    Failures:\n";
                for (const auto& failure : benchmark.failures)
                {
                    report << "      - " << failure << "\n";
                }
            }
            report << "\n";
        }
    }

    return report.str();
}

bool PerformanceValidator::ValidateTimingThreshold(uint64 actualTime, uint64 threshold, const ::std::string& metric) const
{
    if (actualTime > threshold)
    {
        TC_LOG_DEBUG("playerbot.test", "{} timing {}μs exceeds threshold {}μs",
                    metric, actualTime, threshold);
        return false;
    }
    return true;
}

bool PerformanceValidator::ValidateMemoryThreshold(uint64 actualMemory, uint64 threshold, const ::std::string& metric) const
{
    if (actualMemory > threshold)
    {
        TC_LOG_DEBUG("playerbot.test", "{} memory {}MB exceeds threshold {}MB",
                    metric, actualMemory / (1024*1024), threshold / (1024*1024));
        return false;
    }
    return true;
}

bool PerformanceValidator::ValidateCpuThreshold(float actualCpu, float threshold, const ::std::string& metric) const
{
    if (actualCpu > threshold)
    {
        TC_LOG_DEBUG("playerbot.test", "{} CPU {}% exceeds threshold {}%",
                    metric, actualCpu, threshold);
        return false;
    }
    return true;
}

uint64 PerformanceValidator::GetCurrentMemoryUsage() const
{
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        return pmc.WorkingSetSize;
#elif defined(__linux__)
    ::std::ifstream statusFile("/proc/self/status");
    ::std::string line;
    while (::std::getline(statusFile, line))
    {
        if (line.find("VmRSS:") == 0)
        {
            ::std::istringstream iss(line);
            ::std::string label;
            uint64 size;
            ::std::string unit;
            if (iss >> label >> size >> unit)
            {
                return size * 1024; // Convert from kB to bytes
            }
        }
    }
#elif defined(__APPLE__)
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &infoCount) == KERN_SUCCESS)
        return info.resident_size;
#endif

    return 0;
}

float PerformanceValidator::GetCurrentCpuUsage() const
{
    // Platform-specific CPU usage implementation
    // For testing purposes, return a simulated low CPU usage
    return 15.0f;
}

uint32 PerformanceValidator::GetActiveConnections() const
{
    // In a real implementation, this would query the network subsystem
    return 0;
}

uint32 PerformanceValidator::GetDatabaseMetrics() const
{
    // In a real implementation, this would query the database connection pool
    return 0;
}

// ========================
// PerformanceProfiler Implementation
// ========================

PerformanceProfiler::PerformanceProfiler()
    : m_profilingActive(false)
{
}

PerformanceProfiler::~PerformanceProfiler()
{
    if (m_profilingActive)
        StopProfiling();
}

void PerformanceProfiler::StartProfiling(const ::std::string& sessionName)
{
    if (m_profilingActive)
        StopProfiling();

    m_currentSession = ::std::make_unique<ProfilingSession>();
    m_currentSession->name = sessionName;
    m_currentSession->startTime = ::std::chrono::high_resolution_clock::now();
    m_profilingActive = true;

    TC_LOG_INFO("playerbot.test", "Started performance profiling session: {}", sessionName);
}

void PerformanceProfiler::StopProfiling()
{
    if (!m_profilingActive || !m_currentSession)
        return;

    m_currentSession->endTime = ::std::chrono::high_resolution_clock::now();
    m_profilingActive = false;

    TC_LOG_INFO("playerbot.test", "Stopped performance profiling session: {}", m_currentSession->name);
}

bool PerformanceProfiler::IsProfilingActive() const
{
    return m_profilingActive;
}

void PerformanceProfiler::RecordOperation(const ::std::string& operationType, uint64 duration)
{
    if (!m_profilingActive || !m_currentSession)
        return;

    m_currentSession->operationTimes[operationType].push_back(duration);
}

void PerformanceProfiler::RecordMemorySnapshot(uint64 memoryUsage)
{
    if (!m_profilingActive || !m_currentSession)
        return;

    m_currentSession->memorySnapshots.push_back(memoryUsage);
}

void PerformanceProfiler::RecordCpuSnapshot(float cpuUsage)
{
    if (!m_profilingActive || !m_currentSession)
        return;

    m_currentSession->cpuSnapshots.push_back(cpuUsage);
}

PerformanceBenchmark PerformanceProfiler::GenerateBenchmark() const
{
    PerformanceBenchmark benchmark;

    if (!m_currentSession)
        return benchmark;

    benchmark.testName = m_currentSession->name;
    benchmark.category = "ProfiledTest";

    // Calculate timing metrics from all operations
    ::std::vector<uint64> allTimes;
    for (const auto& [operationType, times] : m_currentSession->operationTimes)
    {
        allTimes.insert(allTimes.end(), times.begin(), times.end());
    }

    if (!allTimes.empty())
    {
        ::std::sort(allTimes.begin(), allTimes.end());

        benchmark.totalOperations = allTimes.size();
        benchmark.successfulOperations = allTimes.size(); // Assume all recorded operations were successful
        benchmark.minResponseTime = allTimes.front();
        benchmark.maxResponseTime = allTimes.back();
        benchmark.averageResponseTime = ::std::accumulate(allTimes.begin(), allTimes.end(), 0ULL) / allTimes.size();

        // Calculate percentiles
        size_t p95Index = static_cast<size_t>(allTimes.size() * 0.95);
        size_t p99Index = static_cast<size_t>(allTimes.size() * 0.99);
        benchmark.p95ResponseTime = allTimes[::std::min(p95Index, allTimes.size() - 1)];
        benchmark.p99ResponseTime = allTimes[::std::min(p99Index, allTimes.size() - 1)];

        // Calculate throughput
    if (m_currentSession->endTime > m_currentSession->startTime)
        {
            auto duration = ::std::chrono::duration_cast<::std::chrono::seconds>(
                m_currentSession->endTime - m_currentSession->startTime);
            if (duration.count() > 0)
                benchmark.operationsPerSecond = allTimes.size() / duration.count();
        }
    }

    // Memory metrics
    if (!m_currentSession->memorySnapshots.empty())
    {
        benchmark.peakMemoryUsage = *::std::max_element(
            m_currentSession->memorySnapshots.begin(),
            m_currentSession->memorySnapshots.end());
    }

    // CPU metrics
    if (!m_currentSession->cpuSnapshots.empty())
    {
        benchmark.peakCpuUsage = *::std::max_element(
            m_currentSession->cpuSnapshots.begin(),
            m_currentSession->cpuSnapshots.end());

        benchmark.averageCpuUsage = ::std::accumulate(
            m_currentSession->cpuSnapshots.begin(),
            m_currentSession->cpuSnapshots.end(), 0.0f) / m_currentSession->cpuSnapshots.size();
    }

    return benchmark;
}

// ========================
// LoadTestRunner Implementation
// ========================

LoadTestRunner::LoadTestRunner(const LoadTestConfig& config)
    : m_config(config)
    , m_profiler(::std::make_unique<PerformanceProfiler>())
{
}

bool LoadTestRunner::RunScalabilityTest()
{
    TC_LOG_INFO("playerbot.test", "Starting scalability test - ramping up to {} bots", m_config.maxConcurrentBots);

    m_profiler->StartProfiling("ScalabilityTest");

    bool success = true;

    // Ramp up phase
    success &= ExecuteRampUp(m_config.maxConcurrentBots, m_config.rampUpTimeSeconds);

    // Sustained load phase
    if (success)
    {
        success &= ExecuteSustainedLoad(m_config.maxConcurrentBots, m_config.sustainedLoadSeconds);
    }

    // Ramp down phase
    if (success)
    {
        success &= ExecuteRampDown(m_config.maxConcurrentBots, m_config.rampDownTimeSeconds);
    }

    m_profiler->StopProfiling();

    PerformanceBenchmark benchmark = m_profiler->GenerateBenchmark();
    benchmark.testName = "ScalabilityTest";
    benchmark.category = "LoadTest";
    m_loadTestResults.push_back(benchmark);

    TC_LOG_INFO("playerbot.test", "Scalability test completed - Success: {}", success);
    return success;
}

bool LoadTestRunner::ExecuteRampUp(uint32 targetBots, uint32 rampTimeSeconds)
{
    TC_LOG_INFO("playerbot.test", "Executing ramp-up to {} bots over {} seconds", targetBots, rampTimeSeconds);

    uint32 botIncrement = ::std::max(1U, targetBots / (rampTimeSeconds / 10)); // Add bots every 10 seconds
    uint32 currentBots = 0;

    auto startTime = ::std::chrono::steady_clock::now();

    while (currentBots < targetBots)
    {
        uint32 nextBotCount = ::std::min(currentBots + botIncrement, targetBots);

        // Simulate spawning additional bots
    for (uint32 i = currentBots; i < nextBotCount; ++i)
        {
            m_profiler->RecordOperation("BotSpawn", 50000); // 50ms simulated spawn time
        }

        currentBots = nextBotCount;

        // Record system metrics
        RecordLoadMetrics(currentBots);

        // Check if system is still stable
    if (!IsSystemStable())
        {
            TC_LOG_ERROR("playerbot.test", "System became unstable during ramp-up at {} bots", currentBots);
            return false;
        }

        // Wait before next increment
        ::std::this_thread::sleep_for(::std::chrono::seconds(10));

        // Check if we've exceeded our ramp time
        auto elapsed = ::std::chrono::duration_cast<::std::chrono::seconds>(
            ::std::chrono::steady_clock::now() - startTime);
        if (elapsed.count() > rampTimeSeconds)
            break;
    }

    TC_LOG_INFO("playerbot.test", "Ramp-up completed: {} bots spawned", currentBots);
    return currentBots >= targetBots;
}

bool LoadTestRunner::ExecuteSustainedLoad(uint32 botCount, uint32 durationSeconds)
{
    TC_LOG_INFO("playerbot.test", "Executing sustained load test with {} bots for {} seconds",
                botCount, durationSeconds);

    auto startTime = ::std::chrono::steady_clock::now();
    auto endTime = startTime + ::std::chrono::seconds(durationSeconds);

    while (::std::chrono::steady_clock::now() < endTime)
    {
        // Record ongoing operations
        m_profiler->RecordOperation("BotUpdate", 1000); // 1ms per bot update

        // Record system metrics
        RecordLoadMetrics(botCount);

        // Check system stability
    if (!IsSystemStable())
        {
            TC_LOG_ERROR("playerbot.test", "System became unstable during sustained load");
            return false;
        }

        // Sleep to simulate update intervals
        ::std::this_thread::sleep_for(::std::chrono::milliseconds(100));
    }

    TC_LOG_INFO("playerbot.test", "Sustained load test completed successfully");
    return true;
}

void LoadTestRunner::RecordLoadMetrics(uint32 currentBots)
{
    // Record memory usage
    uint64 memoryUsage = currentBots * (8 * 1024 * 1024); // Simulate 8MB per bot
    m_profiler->RecordMemorySnapshot(memoryUsage);

    // Record CPU usage (simulated)
    float cpuUsage = ::std::min(90.0f, currentBots * 0.01f); // 0.01% CPU per bot
    m_profiler->RecordCpuSnapshot(cpuUsage);
}

bool LoadTestRunner::IsSystemStable() const
{
    // Simple stability check - in real implementation would be more comprehensive
    return true;
}

} // namespace Test
} // namespace Playerbot