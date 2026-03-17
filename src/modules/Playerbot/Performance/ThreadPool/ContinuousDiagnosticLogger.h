/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * Continuous Diagnostic Logging System
 *
 * Provides real-time monitoring and logging of ThreadPool performance metrics
 * with multiple output formats for analysis and debugging.
 *
 * Features:
 * - CSV format for data analysis and graphing
 * - JSON format for parsing tools and monitoring systems
 * - Real-time dashboard (text format, updates every second)
 * - Performance timeline tracking
 * - Automatic log rotation
 * - Low overhead (<0.1% CPU)
 */

#ifndef PLAYERBOT_CONTINUOUS_DIAGNOSTIC_LOGGER_H
#define PLAYERBOT_CONTINUOUS_DIAGNOSTIC_LOGGER_H

#include "Define.h"
#include "ThreadPoolDiagnostics.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <memory>
#include <fstream>
#include <deque>

namespace Playerbot {
namespace Performance {

// Forward declarations
class ThreadPool;

/**
 * @brief Configuration for continuous logging
 */
struct ContinuousLoggerConfig
{
    // Logging intervals
    ::std::chrono::milliseconds metricsInterval{1000};      // Sample metrics every 1s
    ::std::chrono::milliseconds csvFlushInterval{5000};     // Flush CSV every 5s
    ::std::chrono::milliseconds dashboardUpdateInterval{1000}; // Update dashboard every 1s

    // Output control
    bool enableCsvLogging{true};
    bool enableJsonLogging{true};
    bool enableDashboard{true};
    bool enableTimelineHistory{true};

    // File paths
    ::std::string logDirectory{"logs/threadpool/"};
    ::std::string csvFilename{"threadpool_metrics.csv"};
    ::std::string jsonFilename{"threadpool_metrics.json"};
    ::std::string dashboardFilename{"threadpool_dashboard.txt"};
    ::std::string timelineFilename{"threadpool_timeline.csv"};

    // History settings
    uint32 maxTimelineEntries{3600};  // Keep 1 hour at 1s interval
    uint32 maxJsonEntries{100};       // Keep last 100 metrics in JSON

    // Log rotation
    bool enableLogRotation{true};
    uint64 maxCsvSizeBytes{100 * 1024 * 1024};  // 100MB
    uint32 maxRotatedFiles{5};
};

/**
 * @brief Snapshot of ThreadPool metrics at a point in time
 */
struct MetricsSnapshot
{
    // Timestamp
    ::std::chrono::steady_clock::time_point timestamp;
    ::std::chrono::system_clock::time_point wallTime;

    // Worker statistics
    uint32 totalWorkers{0};
    uint32 activeWorkers{0};
    uint32 sleepingWorkers{0};
    uint32 stealingWorkers{0};
    uint32 executingWorkers{0};

    // Queue statistics
    size_t totalQueuedTasks{0};
    size_t criticalQueuedTasks{0};
    size_t highQueuedTasks{0};
    size_t normalQueuedTasks{0};
    size_t lowQueuedTasks{0};

    // Performance metrics
    uint64 totalTasksExecuted{0};
    uint64 totalTasksSubmitted{0};
    uint64 totalStealAttempts{0};
    uint64 successfulSteals{0};
    double stealSuccessRate{0.0};

    // Throughput (tasks/second)
    double tasksPerSecond{0.0};
    double avgTaskLatencyMicros{0.0};

    // Resource usage
    double cpuUsagePercent{0.0};
    uint64 memoryUsageBytes{0};

    // Wait statistics
    uint32 workersInWait{0};
    ::std::chrono::milliseconds maxWaitDuration{0};
    ::std::chrono::milliseconds avgWaitDuration{0};

    // State durations (average time in each state, microseconds)
    double avgIdleDurationMicros{0.0};
    double avgExecutingDurationMicros{0.0};
    double avgStealingDurationMicros{0.0};

    // Helper: Convert to CSV line
    ::std::string ToCsvLine() const;

    // Helper: Convert to JSON object
    ::std::string ToJson() const;

    // Helper: Get CSV header
    static ::std::string GetCsvHeader();
};

/**
 * @brief Continuous diagnostic logger
 *
 * Runs in background thread, periodically sampling ThreadPool metrics
 * and writing to various output formats.
 */
class ContinuousDiagnosticLogger
{
public:
    explicit ContinuousDiagnosticLogger(ThreadPool* pool, ContinuousLoggerConfig config = {});
    ~ContinuousDiagnosticLogger();

    // Lifecycle
    void Start();
    void Stop();
    bool IsRunning() const { return _running.load(::std::memory_order_relaxed); }

    // Control
    void Pause() { _paused.store(true, ::std::memory_order_release); }
    void Resume() { _paused.store(false, ::std::memory_order_release); }
    bool IsPaused() const { return _paused.load(::std::memory_order_acquire); }

    // Manual snapshot
    MetricsSnapshot TakeSnapshot();

    // Configuration
    void SetConfig(ContinuousLoggerConfig config);
    ContinuousLoggerConfig GetConfig() const { return _config; }

    // Statistics
    struct Stats
    {
        uint64 snapshotsTaken{0};
        uint64 csvLinesWritten{0};
        uint64 jsonEntriesWritten{0};
        uint64 dashboardUpdates{0};
        uint64 logRotations{0};
        ::std::chrono::seconds uptime{0};
    };
    Stats GetStatistics() const;

private:
    // Main logging loop
    void LoggingLoop();

    // Metric collection
    MetricsSnapshot CollectMetrics();
    void CalculateThroughput(MetricsSnapshot& snapshot);
    void CalculateWaitStatistics(MetricsSnapshot& snapshot);
    void CalculateStateDurations(MetricsSnapshot& snapshot);

    // Output writers
    void WriteCsv(const MetricsSnapshot& snapshot);
    void WriteJson(const MetricsSnapshot& snapshot);
    void WriteDashboard(const MetricsSnapshot& snapshot);
    void UpdateTimeline(const MetricsSnapshot& snapshot);

    // File management
    void InitializeFiles();
    void RotateCsvIfNeeded();
    void RotateFile(const ::std::string& filename, const ::std::string& extension);

    // Dashboard rendering
    ::std::string RenderDashboard(const MetricsSnapshot& snapshot);
    ::std::string RenderWorkerStates(const MetricsSnapshot& snapshot);
    ::std::string RenderQueueStatus(const MetricsSnapshot& snapshot);
    ::std::string RenderPerformanceMetrics(const MetricsSnapshot& snapshot);
    ::std::string RenderMiniGraph(const ::std::deque<double>& values, uint32 width, uint32 height);

    // Helper: Get worker diagnostics
    ::std::vector<::std::pair<uint32, WorkerDiagnostics*>> GetWorkerDiagnostics();

    ThreadPool* _pool;
    ContinuousLoggerConfig _config;

    // Thread management
    ::std::unique_ptr<::std::thread> _loggerThread;
    ::std::atomic<bool> _running{false};
    ::std::atomic<bool> _paused{false};

    // File handles
    ::std::ofstream _csvFile;
    ::std::ofstream _jsonFile;
    ::std::ofstream _dashboardFile;
    ::std::ofstream _timelineFile;

    // History tracking
    ::std::deque<MetricsSnapshot> _timeline;
    MetricsSnapshot _lastSnapshot;

    // Statistics
    struct InternalStats
    {
        ::std::atomic<uint64> snapshotsTaken{0};
        ::std::atomic<uint64> csvLinesWritten{0};
        ::std::atomic<uint64> jsonEntriesWritten{0};
        ::std::atomic<uint64> dashboardUpdates{0};
        ::std::atomic<uint64> logRotations{0};
        ::std::chrono::steady_clock::time_point startTime;
    } _stats;

    // CSV size tracking
    ::std::atomic<uint64> _csvSize{0};
};

} // namespace Performance
} // namespace Playerbot

#endif // PLAYERBOT_CONTINUOUS_DIAGNOSTIC_LOGGER_H
