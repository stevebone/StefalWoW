/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * Phase 5: Performance Optimization - PerformanceManager
 *
 * Central coordinator for all performance optimization systems
 * Integrates ThreadPool, MemoryPool, QueryOptimizer, and Profiler
 */

#ifndef PLAYERBOT_PERFORMANCEMANAGER_H
#define PLAYERBOT_PERFORMANCEMANAGER_H

#include "ThreadPool/ThreadPool.h"
#include "MemoryPool/MemoryPool.h"
#include "QueryOptimizer/QueryOptimizer.h"
#include "Profiler/Profiler.h"
#include <atomic>
#include <string>

namespace Playerbot {
namespace Performance {

/**
 * @brief Central performance management system
 *
 * Coordinates all Phase 5 performance optimization components
 * Provides unified interface for performance monitoring and optimization
 */
class PerformanceManager
{
private:
    ::std::atomic<bool> _initialized{false};
    ::std::atomic<bool> _profilingEnabled{false};

public:
    /**
     * @brief Initialize all performance systems
     * @return true if initialization successful
     */
    bool Initialize();

    /**
     * @brief Shutdown all performance systems gracefully
     */
    void Shutdown();

    /**
     * @brief Check if manager is initialized
     */
    bool IsInitialized() const { return _initialized.load(::std::memory_order_relaxed); }

    /**
     * @brief Enable/disable profiling
     */
    void StartProfiling();
    void StopProfiling();

    /**
     * @brief Generate comprehensive performance report
     * @param filename Output file for report
     */
    void GeneratePerformanceReport(const ::std::string& filename);

    /**
     * @brief Handle memory pressure situation
     */
    void HandleMemoryPressure();

    /**
     * @brief Get singleton instance
     */
    static PerformanceManager& Instance();

private:
    PerformanceManager() = default;
    ~PerformanceManager() { Shutdown(); }
};

} // namespace Performance
} // namespace Playerbot

#endif // PLAYERBOT_PERFORMANCEMANAGER_H
