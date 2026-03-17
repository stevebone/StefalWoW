/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * Continuous Diagnostic Logger Implementation
 */

#include "ContinuousDiagnosticLogger.h"
#include "ThreadPool.h"
#include "Log.h"
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <algorithm>
#include <ctime>

namespace Playerbot {
namespace Performance {

// MetricsSnapshot implementations

::std::string MetricsSnapshot::ToCsvLine() const
{
    auto time_t_val = ::std::chrono::system_clock::to_time_t(wallTime);
    ::std::tm tm_val = *::std::localtime(&time_t_val);

    ::std::stringstream ss;
    ss << ::std::put_time(&tm_val, "%Y-%m-%d %H:%M:%S") << ","
       << totalWorkers << ","
       << activeWorkers << ","
       << sleepingWorkers << ","
       << stealingWorkers << ","
       << executingWorkers << ","
       << totalQueuedTasks << ","
       << criticalQueuedTasks << ","
       << highQueuedTasks << ","
       << normalQueuedTasks << ","
       << lowQueuedTasks << ","
       << totalTasksExecuted << ","
       << totalTasksSubmitted << ","
       << totalStealAttempts << ","
       << successfulSteals << ","
       << ::std::fixed << ::std::setprecision(2) << stealSuccessRate << ","
       << ::std::fixed << ::std::setprecision(2) << tasksPerSecond << ","
       << ::std::fixed << ::std::setprecision(2) << avgTaskLatencyMicros << ","
       << ::std::fixed << ::std::setprecision(2) << cpuUsagePercent << ","
       << memoryUsageBytes << ","
       << workersInWait << ","
       << maxWaitDuration.count() << ","
       << avgWaitDuration.count() << ","
       << ::std::fixed << ::std::setprecision(2) << avgIdleDurationMicros << ","
       << ::std::fixed << ::std::setprecision(2) << avgExecutingDurationMicros << ","
       << ::std::fixed << ::std::setprecision(2) << avgStealingDurationMicros;

    return ss.str();
}

::std::string MetricsSnapshot::ToJson() const
{
    auto time_t_val = ::std::chrono::system_clock::to_time_t(wallTime);
    ::std::tm tm_val = *::std::localtime(&time_t_val);

    ::std::stringstream ss;
    ss << "{\n"
       << "  \"timestamp\": \"" << ::std::put_time(&tm_val, "%Y-%m-%d %H:%M:%S") << "\",\n"
       << "  \"workers\": {\n"
       << "    \"total\": " << totalWorkers << ",\n"
       << "    \"active\": " << activeWorkers << ",\n"
       << "    \"sleeping\": " << sleepingWorkers << ",\n"
       << "    \"stealing\": " << stealingWorkers << ",\n"
       << "    \"executing\": " << executingWorkers << "\n"
       << "  },\n"
       << "  \"queue\": {\n"
       << "    \"total\": " << totalQueuedTasks << ",\n"
       << "    \"critical\": " << criticalQueuedTasks << ",\n"
       << "    \"high\": " << highQueuedTasks << ",\n"
       << "    \"normal\": " << normalQueuedTasks << ",\n"
       << "    \"low\": " << lowQueuedTasks << "\n"
       << "  },\n"
       << "  \"performance\": {\n"
       << "    \"tasksExecuted\": " << totalTasksExecuted << ",\n"
       << "    \"tasksSubmitted\": " << totalTasksSubmitted << ",\n"
       << "    \"tasksPerSecond\": " << ::std::fixed << ::std::setprecision(2) << tasksPerSecond << ",\n"
       << "    \"avgLatencyMicros\": " << ::std::fixed << ::std::setprecision(2) << avgTaskLatencyMicros << "\n"
       << "  },\n"
       << "  \"workStealing\": {\n"
       << "    \"totalAttempts\": " << totalStealAttempts << ",\n"
       << "    \"successfulSteals\": " << successfulSteals << ",\n"
       << "    \"successRate\": " << ::std::fixed << ::std::setprecision(2) << stealSuccessRate << "\n"
       << "  },\n"
       << "  \"resources\": {\n"
       << "    \"cpuPercent\": " << ::std::fixed << ::std::setprecision(2) << cpuUsagePercent << ",\n"
       << "    \"memoryBytes\": " << memoryUsageBytes << "\n"
       << "  },\n"
       << "  \"waits\": {\n"
       << "    \"workersWaiting\": " << workersInWait << ",\n"
       << "    \"maxWaitMs\": " << maxWaitDuration.count() << ",\n"
       << "    \"avgWaitMs\": " << avgWaitDuration.count() << "\n"
       << "  }\n"
       << "}";

    return ss.str();
}

::std::string MetricsSnapshot::GetCsvHeader()
{
    return "Timestamp,TotalWorkers,ActiveWorkers,SleepingWorkers,StealingWorkers,ExecutingWorkers,"
           "TotalQueuedTasks,CriticalQueue,HighQueue,NormalQueue,LowQueue,"
           "TasksExecuted,TasksSubmitted,StealAttempts,SuccessfulSteals,StealSuccessRate,"
           "TasksPerSecond,AvgLatencyMicros,CpuPercent,MemoryBytes,"
           "WorkersWaiting,MaxWaitMs,AvgWaitMs,"
           "AvgIdleDurationMicros,AvgExecutingDurationMicros,AvgStealingDurationMicros";
}

// ContinuousDiagnosticLogger implementation

ContinuousDiagnosticLogger::ContinuousDiagnosticLogger(ThreadPool* pool, ContinuousLoggerConfig config)
    : _pool(pool)
    , _config(::std::move(config))
{
    _stats.startTime = ::std::chrono::steady_clock::now();
}

ContinuousDiagnosticLogger::~ContinuousDiagnosticLogger()
{
    Stop();
}

void ContinuousDiagnosticLogger::Start()
{
    if (_running.exchange(true))
        return; // Already running

    // Initialize output files
    InitializeFiles();

    // Start logging thread
    _loggerThread = ::std::make_unique<::std::thread>(&ContinuousDiagnosticLogger::LoggingLoop, this);

    TC_LOG_INFO("playerbot.threadpool.logger",
        "Continuous diagnostic logger started (CSV: {}, JSON: {}, Dashboard: {})",
        _config.enableCsvLogging, _config.enableJsonLogging, _config.enableDashboard);
}

void ContinuousDiagnosticLogger::Stop()
{
    if (!_running.exchange(false))
        return; // Already stopped

    // Wait for thread to finish
    if (_loggerThread && _loggerThread->joinable())
    {
        _loggerThread->join();
        _loggerThread.reset();
    }

    // Close files
    if (_csvFile.is_open()) _csvFile.close();
    if (_jsonFile.is_open()) _jsonFile.close();
    if (_dashboardFile.is_open()) _dashboardFile.close();
    if (_timelineFile.is_open()) _timelineFile.close();

    auto stats = GetStatistics();
    TC_LOG_INFO("playerbot.threadpool.logger",
        "Continuous diagnostic logger stopped. Snapshots: {}, CSV lines: {}, Dashboard updates: {}",
        stats.snapshotsTaken, stats.csvLinesWritten, stats.dashboardUpdates);
}

void ContinuousDiagnosticLogger::LoggingLoop()
{
    auto lastCsvFlush = ::std::chrono::steady_clock::now();
    auto lastDashboardUpdate = ::std::chrono::steady_clock::now();

    while (_running.load(::std::memory_order_relaxed))
    {
        auto now = ::std::chrono::steady_clock::now();

        // Sleep for metrics interval
        ::std::this_thread::sleep_for(_config.metricsInterval);

        if (_paused.load(::std::memory_order_relaxed))
            continue;

        try
        {
            // Collect metrics
            MetricsSnapshot snapshot = CollectMetrics();
            _stats.snapshotsTaken.fetch_add(1, ::std::memory_order_relaxed);

            // Write CSV
    if (_config.enableCsvLogging)
            {
                WriteCsv(snapshot);
            }

            // Update timeline
    if (_config.enableTimelineHistory)
            {
                UpdateTimeline(snapshot);
            }

            // Flush CSV periodically
    if (::std::chrono::duration_cast<::std::chrono::milliseconds>(now - lastCsvFlush) >=
                _config.csvFlushInterval)
            {
                if (_csvFile.is_open())
                    _csvFile.flush();
                lastCsvFlush = now;
            }

            // Update dashboard
    if (_config.enableDashboard &&
                ::std::chrono::duration_cast<::std::chrono::milliseconds>(now - lastDashboardUpdate) >=
                _config.dashboardUpdateInterval)
            {
                WriteDashboard(snapshot);
                lastDashboardUpdate = now;
            }

            // Write JSON (contains timeline)
    if (_config.enableJsonLogging)
            {
                WriteJson(snapshot);
            }

            // Store for next iteration
            _lastSnapshot = snapshot;
        }
        catch (const ::std::exception& e)
        {
            TC_LOG_ERROR("playerbot.threadpool.logger",
                "Exception in logging loop: {}", e.what());
        }
    }
}

MetricsSnapshot ContinuousDiagnosticLogger::CollectMetrics()
{
    MetricsSnapshot snapshot;
    snapshot.timestamp = ::std::chrono::steady_clock::now();
    snapshot.wallTime = ::std::chrono::system_clock::now();

    if (!_pool)
        return snapshot;

    // Get worker diagnostics
    auto workers = GetWorkerDiagnostics();
    snapshot.totalWorkers = static_cast<uint32>(workers.size());

    // Count workers by state
    for (const auto& [workerId, diag] : workers)
    {
        if (!diag)
            continue;

        WorkerState state = diag->currentState.load(::std::memory_order_relaxed);

        switch (state)
        {
            case WorkerState::IDLE_SLEEPING:
                snapshot.sleepingWorkers++;
                break;
            case WorkerState::STEALING:
                snapshot.stealingWorkers++;
                snapshot.activeWorkers++;
                break;
            case WorkerState::EXECUTING:
                snapshot.executingWorkers++;
                snapshot.activeWorkers++;
                break;
            case WorkerState::IDLE_SPINNING:
                snapshot.activeWorkers++;
                break;
            default:
                break;
        }

        // Accumulate task counts
        snapshot.totalTasksExecuted += diag->tasksExecuted.load(::std::memory_order_relaxed);
        snapshot.totalStealAttempts += diag->stealAttempts.load(::std::memory_order_relaxed);
        snapshot.successfulSteals += diag->stealSuccesses.load(::std::memory_order_relaxed);
    }

    // Get queue statistics
    snapshot.totalQueuedTasks = _pool->GetQueuedTasks();

    // Calculate steal success rate
    if (snapshot.totalStealAttempts > 0)
    {
        snapshot.stealSuccessRate =
            (static_cast<double>(snapshot.successfulSteals) / snapshot.totalStealAttempts) * 100.0;
    }

    // Calculate derived metrics
    CalculateThroughput(snapshot);
    CalculateWaitStatistics(snapshot);
    CalculateStateDurations(snapshot);

    return snapshot;
}

void ContinuousDiagnosticLogger::CalculateThroughput(MetricsSnapshot& snapshot)
{
    if (_lastSnapshot.totalTasksExecuted == 0)
        return; // First snapshot

    auto timeDelta = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
        snapshot.timestamp - _lastSnapshot.timestamp).count();

    if (timeDelta > 0)
    {
        uint64 tasksDelta = snapshot.totalTasksExecuted - _lastSnapshot.totalTasksExecuted;
        snapshot.tasksPerSecond = (static_cast<double>(tasksDelta) / timeDelta) * 1000.0;
    }
}

void ContinuousDiagnosticLogger::CalculateWaitStatistics(MetricsSnapshot& snapshot)
{
    auto workers = GetWorkerDiagnostics();
    ::std::chrono::milliseconds totalWaitDuration{0};
    uint32 waitingCount = 0;

    for (const auto& [workerId, diag] : workers)
    {
        if (!diag)
            continue;

        auto wait = diag->GetCurrentWait();
        if (wait.has_value())
        {
            waitingCount++;
            auto waitDuration = wait->GetWaitDuration();

            totalWaitDuration += waitDuration;

            if (waitDuration > snapshot.maxWaitDuration)
                snapshot.maxWaitDuration = waitDuration;
        }
    }

    snapshot.workersInWait = waitingCount;
    if (waitingCount > 0)
    {
        snapshot.avgWaitDuration = totalWaitDuration / waitingCount;
    }
}

void ContinuousDiagnosticLogger::CalculateStateDurations(MetricsSnapshot& snapshot)
{
    // This would require tracking state transition history
    // For now, we'll leave these as 0.0
    // Could be enhanced with state duration tracking in WorkerDiagnostics
}

void ContinuousDiagnosticLogger::WriteCsv(const MetricsSnapshot& snapshot)
{
    if (!_csvFile.is_open())
        return;

    try
    {
        // Check for rotation
        RotateCsvIfNeeded();

        // Write line
        ::std::string line = snapshot.ToCsvLine();
        _csvFile << line << "\n";

        _csvSize.fetch_add(line.length() + 1, ::std::memory_order_relaxed);
        _stats.csvLinesWritten.fetch_add(1, ::std::memory_order_relaxed);
    }
    catch (const ::std::exception& e)
    {
        TC_LOG_ERROR("playerbot.threadpool.logger",
            "Failed to write CSV line: {}", e.what());
    }
}

void ContinuousDiagnosticLogger::WriteJson(const MetricsSnapshot& snapshot)
{
    if (!_jsonFile.is_open())
        return;

    try
    {
        // Reopen file for overwriting
        _jsonFile.close();
        _jsonFile.open(_config.logDirectory + _config.jsonFilename, ::std::ios::out | ::std::ios::trunc);

        // Write JSON array with timeline
        _jsonFile << "{\n";
        _jsonFile << "  \"current\": " << snapshot.ToJson() << ",\n";
        _jsonFile << "  \"timeline\": [\n";

        bool first = true;
        for (const auto& entry : _timeline)
        {
            if (!first)
                _jsonFile << ",\n";
            _jsonFile << "    " << entry.ToJson();
            first = false;
        }

        _jsonFile << "\n  ]\n";
        _jsonFile << "}\n";
        _jsonFile.flush();

        _stats.jsonEntriesWritten.fetch_add(1, ::std::memory_order_relaxed);
    }
    catch (const ::std::exception& e)
    {
        TC_LOG_ERROR("playerbot.threadpool.logger",
            "Failed to write JSON: {}", e.what());
    }
}

void ContinuousDiagnosticLogger::WriteDashboard(const MetricsSnapshot& snapshot)
{
    if (!_dashboardFile.is_open())
        return;

    try
    {
        ::std::string dashboard = RenderDashboard(snapshot);

        // Rewrite entire file
        _dashboardFile.close();
        _dashboardFile.open(_config.logDirectory + _config.dashboardFilename,
                           ::std::ios::out | ::std::ios::trunc);
        _dashboardFile << dashboard;
        _dashboardFile.flush();

        _stats.dashboardUpdates.fetch_add(1, ::std::memory_order_relaxed);
    }
    catch (const ::std::exception& e)
    {
        TC_LOG_ERROR("playerbot.threadpool.logger",
            "Failed to write dashboard: {}", e.what());
    }
}

void ContinuousDiagnosticLogger::UpdateTimeline(const MetricsSnapshot& snapshot)
{
    _timeline.push_back(snapshot);

    // Keep only last N entries
    while (_timeline.size() > _config.maxTimelineEntries)
    {
        _timeline.pop_front();
    }
}

void ContinuousDiagnosticLogger::InitializeFiles()
{
    try
    {
        // Create directory
        ::std::filesystem::create_directories(_config.logDirectory);

        // Open CSV file (append mode)
    if (_config.enableCsvLogging)
        {
            ::std::string csvPath = _config.logDirectory + _config.csvFilename;
            bool fileExists = ::std::filesystem::exists(csvPath);

            _csvFile.open(csvPath, ::std::ios::out | ::std::ios::app);
            if (_csvFile.is_open())
            {
                // Write header if new file
    if (!fileExists)
                {
                    _csvFile << MetricsSnapshot::GetCsvHeader() << "\n";
                }

                // Get current file size
    if (fileExists)
                {
                    _csvSize.store(::std::filesystem::file_size(csvPath), ::std::memory_order_relaxed);
                }
            }
        }

        // Create JSON file
    if (_config.enableJsonLogging)
        {
            _jsonFile.open(_config.logDirectory + _config.jsonFilename, ::std::ios::out | ::std::ios::trunc);
        }

        // Create dashboard file
    if (_config.enableDashboard)
        {
            _dashboardFile.open(_config.logDirectory + _config.dashboardFilename,
                               ::std::ios::out | ::std::ios::trunc);
        }
    }
    catch (const ::std::exception& e)
    {
        TC_LOG_ERROR("playerbot.threadpool.logger",
            "Failed to initialize log files: {}", e.what());
    }
}

void ContinuousDiagnosticLogger::RotateCsvIfNeeded()
{
    if (!_config.enableLogRotation)
        return;

    uint64 currentSize = _csvSize.load(::std::memory_order_relaxed);
    if (currentSize < _config.maxCsvSizeBytes)
        return;

    try
    {
        _csvFile.close();

        // Rotate old files
        RotateFile(_config.csvFilename, ".csv");

        // Open new file
        _csvFile.open(_config.logDirectory + _config.csvFilename, ::std::ios::out | ::std::ios::trunc);
        if (_csvFile.is_open())
        {
            _csvFile << MetricsSnapshot::GetCsvHeader() << "\n";
            _csvSize.store(0, ::std::memory_order_relaxed);
            _stats.logRotations.fetch_add(1, ::std::memory_order_relaxed);

            TC_LOG_INFO("playerbot.threadpool.logger",
                "CSV log rotated ({} bytes)", currentSize);
        }
    }
    catch (const ::std::exception& e)
    {
        TC_LOG_ERROR("playerbot.threadpool.logger",
            "Failed to rotate CSV log: {}", e.what());
    }
}

void ContinuousDiagnosticLogger::RotateFile(const ::std::string& filename, const ::std::string& extension)
{
    // Rotate: file.csv -> file.1.csv -> file.2.csv -> ... -> file.N.csv (deleted)
    ::std::string basePath = _config.logDirectory + filename.substr(0, filename.find(extension));

    // Delete oldest
    ::std::string oldestPath = basePath + "." + ::std::to_string(_config.maxRotatedFiles) + extension;
    if (::std::filesystem::exists(oldestPath))
    {
        ::std::filesystem::remove(oldestPath);
    }

    // Shift existing files
    for (int i = _config.maxRotatedFiles - 1; i >= 1; --i)
    {
        ::std::string fromPath = basePath + "." + ::std::to_string(i) + extension;
        ::std::string toPath = basePath + "." + ::std::to_string(i + 1) + extension;

        if (::std::filesystem::exists(fromPath))
        {
            ::std::filesystem::rename(fromPath, toPath);
        }
    }

    // Rename current to .1
    ::std::string currentPath = _config.logDirectory + filename;
    ::std::string newPath = basePath + ".1" + extension;
    if (::std::filesystem::exists(currentPath))
    {
        ::std::filesystem::rename(currentPath, newPath);
    }
}

::std::string ContinuousDiagnosticLogger::RenderDashboard(const MetricsSnapshot& snapshot)
{
    ::std::stringstream ss;

    auto time_t_val = ::std::chrono::system_clock::to_time_t(snapshot.wallTime);
    ::std::tm tm_val = *::std::localtime(&time_t_val);

    ss << "================================================================================\n";
    ss << "                  THREADPOOL REAL-TIME DIAGNOSTIC DASHBOARD\n";
    ss << "================================================================================\n";
    ss << "Updated: " << ::std::put_time(&tm_val, "%Y-%m-%d %H:%M:%S") << "\n\n";

    ss << RenderWorkerStates(snapshot);
    ss << "\n";
    ss << RenderQueueStatus(snapshot);
    ss << "\n";
    ss << RenderPerformanceMetrics(snapshot);
    ss << "\n";

    // Mini graph of throughput (last 60 seconds)
    if (_timeline.size() > 10)
    {
        ss << "Throughput History (tasks/sec):\n";
        ::std::deque<double> throughputValues;
        for (const auto& entry : _timeline)
        {
            throughputValues.push_back(entry.tasksPerSecond);
        }
        ss << RenderMiniGraph(throughputValues, 70, 5);
        ss << "\n";
    }

    ss << "================================================================================\n";

    return ss.str();
}

::std::string ContinuousDiagnosticLogger::RenderWorkerStates(const MetricsSnapshot& snapshot)
{
    ::std::stringstream ss;
    ss << "WORKER STATES\n";
    ss << "-------------\n";
    ss << "Total Workers:     " << ::std::setw(3) << snapshot.totalWorkers << "\n";
    ss << "Active:            " << ::std::setw(3) << snapshot.activeWorkers
       << " (" << ::std::fixed << ::std::setprecision(1)
       << (snapshot.totalWorkers > 0 ? (snapshot.activeWorkers * 100.0 / snapshot.totalWorkers) : 0.0)
       << "%)\n";
    ss << "  - Executing:     " << ::std::setw(3) << snapshot.executingWorkers << "\n";
    ss << "  - Stealing:      " << ::std::setw(3) << snapshot.stealingWorkers << "\n";
    ss << "Sleeping:          " << ::std::setw(3) << snapshot.sleepingWorkers << "\n";
    ss << "In Wait:           " << ::std::setw(3) << snapshot.workersInWait << "\n";

    return ss.str();
}

::std::string ContinuousDiagnosticLogger::RenderQueueStatus(const MetricsSnapshot& snapshot)
{
    ::std::stringstream ss;
    ss << "QUEUE STATUS\n";
    ss << "------------\n";
    ss << "Total Queued:      " << ::std::setw(6) << snapshot.totalQueuedTasks << "\n";
    ss << "  - CRITICAL:      " << ::std::setw(6) << snapshot.criticalQueuedTasks << "\n";
    ss << "  - HIGH:          " << ::std::setw(6) << snapshot.highQueuedTasks << "\n";
    ss << "  - NORMAL:        " << ::std::setw(6) << snapshot.normalQueuedTasks << "\n";
    ss << "  - LOW:           " << ::std::setw(6) << snapshot.lowQueuedTasks << "\n";

    return ss.str();
}

::std::string ContinuousDiagnosticLogger::RenderPerformanceMetrics(const MetricsSnapshot& snapshot)
{
    ::std::stringstream ss;
    ss << "PERFORMANCE METRICS\n";
    ss << "-------------------\n";
    ss << "Tasks/Second:      " << ::std::fixed << ::std::setprecision(2) << ::std::setw(8)
       << snapshot.tasksPerSecond << "\n";
    ss << "Avg Latency:       " << ::std::fixed << ::std::setprecision(2) << ::std::setw(8)
       << snapshot.avgTaskLatencyMicros << " μs\n";
    ss << "Total Executed:    " << ::std::setw(10) << snapshot.totalTasksExecuted << "\n";
    ss << "\n";
    ss << "WORK STEALING\n";
    ss << "-------------\n";
    ss << "Steal Attempts:    " << ::std::setw(10) << snapshot.totalStealAttempts << "\n";
    ss << "Successful:        " << ::std::setw(10) << snapshot.successfulSteals << "\n";
    ss << "Success Rate:      " << ::std::fixed << ::std::setprecision(2) << ::std::setw(8)
       << snapshot.stealSuccessRate << "%\n";

    return ss.str();
}

::std::string ContinuousDiagnosticLogger::RenderMiniGraph(const ::std::deque<double>& values,
                                                         uint32 width, uint32 height)
{
    if (values.empty())
        return "";

    // Find min/max
    double minVal = *::std::min_element(values.begin(), values.end());
    double maxVal = *::std::max_element(values.begin(), values.end());

    if (maxVal == minVal)
        maxVal = minVal + 1.0; // Avoid division by zero

    // Sample values to fit width
    ::std::vector<double> sampledValues;
    if (values.size() <= width)
    {
        sampledValues.assign(values.begin(), values.end());
    }
    else
    {
        double step = static_cast<double>(values.size()) / width;
        for (uint32 i = 0; i < width; ++i)
        {
            size_t idx = static_cast<size_t>(i * step);
            sampledValues.push_back(values[idx]);
        }
    }

    // Render graph
    ::std::stringstream ss;
    const char* blocks[] = {" ", "", "", "", "", "", "", "", ""};
    const int numBlocks = 9;

    for (double val : sampledValues)
    {
        // Normalize to 0-1
        double normalized = (val - minVal) / (maxVal - minVal);
        int blockIdx = static_cast<int>(normalized * (numBlocks - 1));
        blockIdx = ::std::clamp(blockIdx, 0, numBlocks - 1);
        ss << blocks[blockIdx];
    }

    ss << "\n";
    ss << "Min: " << ::std::fixed << ::std::setprecision(1) << minVal
       << " | Max: " << maxVal;

    return ss.str();
}

::std::vector<::std::pair<uint32, WorkerDiagnostics*>> ContinuousDiagnosticLogger::GetWorkerDiagnostics()
{
    ::std::vector<::std::pair<uint32, WorkerDiagnostics*>> diagnostics;

    if (!_pool)
        return diagnostics;

    uint32 workerCount = _pool->GetWorkerCount();
    for (uint32 i = 0; i < workerCount; ++i)
    {
        auto worker = _pool->GetWorker(i);
        if (worker && worker->GetDiagnostics())
        {
            diagnostics.push_back({i, worker->GetDiagnostics()});
        }
    }

    return diagnostics;
}

MetricsSnapshot ContinuousDiagnosticLogger::TakeSnapshot()
{
    return CollectMetrics();
}

void ContinuousDiagnosticLogger::SetConfig(ContinuousLoggerConfig config)
{
    bool wasRunning = IsRunning();
    if (wasRunning)
        Stop();

    _config = ::std::move(config);

    if (wasRunning)
        Start();
}

ContinuousDiagnosticLogger::Stats ContinuousDiagnosticLogger::GetStatistics() const
{
    Stats stats;
    stats.snapshotsTaken = _stats.snapshotsTaken.load();
    stats.csvLinesWritten = _stats.csvLinesWritten.load();
    stats.jsonEntriesWritten = _stats.jsonEntriesWritten.load();
    stats.dashboardUpdates = _stats.dashboardUpdates.load();
    stats.logRotations = _stats.logRotations.load();
    stats.uptime = ::std::chrono::duration_cast<::std::chrono::seconds>(
        ::std::chrono::steady_clock::now() - _stats.startTime);
    return stats;
}

} // namespace Performance
} // namespace Playerbot
