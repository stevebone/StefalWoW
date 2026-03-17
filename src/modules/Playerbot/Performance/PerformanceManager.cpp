/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * Phase 5: Performance Optimization - PerformanceManager Implementation
 */

#include "PerformanceManager.h"
#include "../Config/PlayerbotConfig.h"
#include "Log.h"
#include <fstream>
#include <iomanip>

namespace Playerbot {
namespace Performance {

bool PerformanceManager::Initialize()
{
    if (_initialized.exchange(true))
        return true; // Already initialized

    TC_LOG_INFO("playerbot.performance", "Initializing Performance Optimization Systems...");

    // ThreadPool is initialized via GetThreadPool()
    ThreadPool& threadPool = GetThreadPool();

    // QueryOptimizer is initialized via Instance()
    QueryOptimizer& queryOpt = QueryOptimizer::Instance();

    // BotMemoryManager is initialized via Instance()
    BotMemoryManager& memMgr = BotMemoryManager::Instance();

    // Configure from playerbots.conf
    if (PlayerbotConfig::instance())
    {
        // Enable profiling if configured
    if (PlayerbotConfig::instance()->GetBool("Playerbot.Performance.Profiler.Enable", false))
        {
            StartProfiling();
        }

        // Set memory limit
        size_t maxMemoryMB = PlayerbotConfig::instance()->GetUInt("Playerbot.Performance.MemoryPool.MaxMemoryMB", 1024);
        memMgr.SetMaxMemory(maxMemoryMB * 1024 * 1024);
    }

    TC_LOG_INFO("playerbot.performance", "Performance Optimization Systems initialized successfully");
    return true;
}

void PerformanceManager::Shutdown()
{
    if (!_initialized.exchange(false))
        return; // Not initialized

    TC_LOG_INFO("playerbot.performance", "Shutting down Performance Optimization Systems...");

    // Stop profiling
    if (_profilingEnabled.load())
    {
        StopProfiling();
    }

    // ThreadPool shutdown is handled by its destructor

    TC_LOG_INFO("playerbot.performance", "Performance Optimization Systems shut down");
}

void PerformanceManager::StartProfiling()
{
    if (_profilingEnabled.exchange(true))
        return; // Already profiling

    Profiler::Instance().Enable();
    TC_LOG_INFO("playerbot.performance", "Profiling enabled");
}

void PerformanceManager::StopProfiling()
{
    if (!_profilingEnabled.exchange(false))
        return; // Not profiling

    Profiler::Instance().Disable();
    TC_LOG_INFO("playerbot.performance", "Profiling disabled");
}

void PerformanceManager::GeneratePerformanceReport(const ::std::string& filename)
{
    ::std::ofstream report(filename);
    if (!report.is_open())
    {
        TC_LOG_ERROR("playerbot.performance", "Failed to open performance report file: {}", filename);
        return;
    }

    report << "TrinityCore PlayerBot Performance Report\n";
    report << "========================================\n\n";

    // ThreadPool metrics
    ThreadPool& threadPool = GetThreadPool();
    report << "ThreadPool Statistics:\n";
    report << "  Active Threads: " << threadPool.GetActiveThreads() << "\n";
    report << "  Queued Tasks: " << threadPool.GetQueuedTasks() << "\n";
    report << "  Average Latency: " << threadPool.GetAverageLatency().count() << " us\n";
    report << "  Throughput: " << ::std::fixed << ::std::setprecision(2) << threadPool.GetThroughput() << " tasks/sec\n\n";

    // Memory metrics
    BotMemoryManager& memMgr = BotMemoryManager::Instance();
    report << "Memory Statistics:\n";
    report << "  Total Allocated: " << (memMgr.GetTotalAllocated() / 1024 / 1024) << " MB\n";
    report << "  Under Pressure: " << (memMgr.IsUnderMemoryPressure() ? "Yes" : "No") << "\n\n";

    // Query metrics
    QueryOptimizer& queryOpt = QueryOptimizer::Instance();
    auto queryMetrics = queryOpt.GetMetrics();
    report << "Query Optimizer Statistics:\n";
    report << "  Total Queries: " << queryMetrics.totalQueries << "\n";
    report << "  Cached Queries: " << queryMetrics.cachedQueries << "\n";
    report << "  Cache Hit Rate: " << ::std::fixed << ::std::setprecision(2)
           << (queryMetrics.GetCacheHitRate() * 100.0) << "%\n";
    report << "  Average Latency: " << ::std::fixed << ::std::setprecision(2)
           << queryMetrics.GetAverageLatency() << " us\n";
    report << "  Slow Queries: " << queryMetrics.slowQueries << "\n\n";

    // Profiler results (if enabled)
    if (_profilingEnabled.load())
    {
        auto profileResults = Profiler::Instance().GetResults();
        report << "Profiler Statistics:\n";
        for (const auto& [section, data] : profileResults.sections)
        {
            report << "  " << section << ":\n";
            report << "    Calls: " << data.callCount << "\n";
            report << "    Average: " << ::std::fixed << ::std::setprecision(2) << data.GetAverage() << " us\n";
            report << "    Min: " << data.minTime << " us\n";
            report << "    Max: " << data.maxTime << " us\n";
        }
    }

    report.close();
    TC_LOG_INFO("playerbot.performance", "Performance report generated: {}", filename);
}

void PerformanceManager::HandleMemoryPressure()
{
    TC_LOG_WARN("playerbot.performance", "Memory pressure detected - triggering cleanup");

    // In a full implementation, we would:
    // 1. Trigger MemoryPool defragmentation
    // 2. Request BotScheduler to reduce active bot count
    // 3. Clear non-essential caches
    // 4. Force garbage collection where applicable

    // For now, just log the event
    BotMemoryManager& memMgr = BotMemoryManager::Instance();
    TC_LOG_WARN("playerbot.performance",
        "Current memory usage: {} MB / {} MB",
        memMgr.GetTotalAllocated() / 1024 / 1024,
        1024);
}

PerformanceManager& PerformanceManager::Instance()
{
    static PerformanceManager instance;
    return instance;
}

} // namespace Performance
} // namespace Playerbot
