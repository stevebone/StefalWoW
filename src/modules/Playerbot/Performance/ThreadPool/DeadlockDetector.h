/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * Automatic Deadlock Detection System for ThreadPool
 *
 * Monitors worker threads for potential deadlock conditions and
 * automatically generates diagnostic reports when detected.
 *
 * Detection Criteria:
 * - All workers sleeping for >2 seconds with pending tasks
 * - >50% workers sleeping for >5 seconds
 * - Any worker stuck in same state for >30 seconds
 * - Queue growth with no task completion
 *
 * Response Actions:
 * - Generate comprehensive diagnostic dump
 * - Log warning/error messages
 * - Optional automatic recovery attempts
 * - Notify monitoring systems
 */

#ifndef PLAYERBOT_DEADLOCK_DETECTOR_H
#define PLAYERBOT_DEADLOCK_DETECTOR_H

#include "Define.h"
#include "ThreadPoolDiagnostics.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <memory>
#include <functional>
#include <fstream>

// Undefine Windows macros that conflict with our enums
#ifdef ERROR
#undef ERROR
#endif
#ifdef CRITICAL
#undef CRITICAL
#endif

// Forward declarations for new debugging integrations
namespace Playerbot {
namespace Performance {
    class DebuggerIntegration;
    class ContinuousDiagnosticLogger;
    class ETWProvider;
}
}

namespace Playerbot {
namespace Performance {

// Forward declarations
class ThreadPool;
class WorkerThread;

/**
 * @brief Deadlock detection configuration
 */
struct DeadlockDetectorConfig
{
    // Detection thresholds
    ::std::chrono::milliseconds checkInterval{1000};        // How often to check (1 second)
    ::std::chrono::milliseconds allWorkersSleepThreshold{2000};  // All workers sleeping
    ::std::chrono::milliseconds majorityWorkersSleepThreshold{5000}; // >50% sleeping
    ::std::chrono::milliseconds singleWorkerStuckThreshold{30000};   // Worker stuck in state
    float majorityThreshold{0.5f};                        // Percentage for majority

    // Queue monitoring
    uint32 queueGrowthCheckCount{5};                      // Consecutive checks with growth
    uint32 maxQueueSizeBeforeAlert{1000};                 // Alert if queue exceeds this

    // Response configuration
    bool enableAutoDump{true};                            // Auto-generate diagnostic dumps
    bool enableAutoRecovery{false};                       // Attempt automatic recovery
    bool enableConsoleAlerts{true};                       // Show console warnings
    ::std::string dumpDirectory{"logs/threadpool/"};        // Where to save dumps

    // Logging thresholds
    uint32 maxConsecutiveWarnings{10};                    // Stop spamming after N warnings
};

/**
 * @brief Deadlock detection result
 */
struct DeadlockCheckResult
{
    enum class Severity
    {
        NONE,       // No issues detected
        INFO,       // Informational (minor slowdown)
        WARNING,    // Potential issue detected
        ERROR,      // Likely deadlock detected
        CRITICAL    // Definite deadlock, intervention required
    };

    Severity severity{Severity::NONE};
    ::std::string description;
    ::std::vector<::std::string> details;
    bool requiresDump{false};
    bool requiresRecovery{false};

    // Worker-specific issues
    struct WorkerIssue
    {
        uint32 workerId;
        WorkerState state;
        ::std::chrono::milliseconds timeInState;
        ::std::string issue;
    };
    ::std::vector<WorkerIssue> workerIssues;

    // Queue statistics at detection time
    size_t totalQueuedTasks{0};
    size_t completedTasks{0};
    double throughput{0.0};
};

/**
 * @brief Automatic deadlock detection and response system
 */
class DeadlockDetector
{
private:
    ThreadPool* _pool;
    DeadlockDetectorConfig _config;

    // Detection thread
    ::std::unique_ptr<::std::thread> _detectorThread;
    ::std::atomic<bool> _running{false};
    ::std::atomic<bool> _paused{false};

    // Detection state
    struct DetectionState
    {
        uint32 consecutiveQueueGrowths{0};
        size_t lastQueueSize{0};
        uint64 lastCompletedTasks{0};
        ::std::chrono::steady_clock::time_point lastCheckTime;
        uint32 warningCount{0};
        ::std::chrono::steady_clock::time_point lastDumpTime;
    } _state;

    // Statistics
    struct Statistics
    {
        ::std::atomic<uint64> checksPerformed{0};
        ::std::atomic<uint64> deadlocksDetected{0};
        ::std::atomic<uint64> warningsIssued{0};
        ::std::atomic<uint64> dumpsGenerated{0};
        ::std::atomic<uint64> recoveriesAttempted{0};
        ::std::chrono::steady_clock::time_point startTime;
    } _stats;

    // Callbacks for external notification
    using DeadlockCallback = ::std::function<void(const DeadlockCheckResult&)>;
    ::std::vector<DeadlockCallback> _callbacks;
    ::std::mutex _callbackMutex;

    // Optional debugging integrations (owned externally)
    DebuggerIntegration* _debuggerIntegration{nullptr};
    ContinuousDiagnosticLogger* _continuousLogger{nullptr};
    ETWProvider* _etwProvider{nullptr};

public:
    /**
     * @brief Construct deadlock detector for given thread pool
     */
    explicit DeadlockDetector(ThreadPool* pool,
                            DeadlockDetectorConfig config = {});
    ~DeadlockDetector();

    /**
     * @brief Start detection thread
     */
    void Start();

    /**
     * @brief Stop detection thread
     */
    void Stop();

    /**
     * @brief Pause detection temporarily
     */
    void Pause() { _paused.store(true); }

    /**
     * @brief Resume detection
     */
    void Resume() { _paused.store(false); }

    /**
     * @brief Manually trigger deadlock check
     */
    DeadlockCheckResult CheckNow();

    /**
     * @brief Register callback for deadlock detection
     */
    void RegisterCallback(DeadlockCallback callback);

    /**
     * @brief Get detection statistics
     */
    struct Stats
    {
        uint64 checksPerformed;
        uint64 deadlocksDetected;
        uint64 warningsIssued;
        uint64 dumpsGenerated;
        uint64 recoveriesAttempted;
        ::std::chrono::seconds uptime;
    };
    Stats GetStatistics() const;

    /**
     * @brief Update configuration
     */
    void SetConfiguration(const DeadlockDetectorConfig& config) { _config = config; }

    /**
     * @brief Get current configuration
     */
    DeadlockDetectorConfig GetConfiguration() const { return _config; }

    /**
     * @brief Set debugger integration (optional)
     *
     * Enables automatic breakpoints and minidump generation
     */
    void SetDebuggerIntegration(DebuggerIntegration* integration) { _debuggerIntegration = integration; }

    /**
     * @brief Set continuous logger (optional)
     *
     * Enables real-time metrics logging to CSV/JSON/dashboard
     */
    void SetContinuousLogger(ContinuousDiagnosticLogger* logger) { _continuousLogger = logger; }

    /**
     * @brief Set ETW provider (optional)
     *
     * Enables Windows Performance Analyzer integration
     */
    void SetETWProvider(ETWProvider* provider) { _etwProvider = provider; }

private:
    /**
     * @brief Main detection loop
     */
    void DetectionLoop();

    /**
     * @brief Perform single deadlock check
     */
    DeadlockCheckResult PerformCheck();

    /**
     * @brief Check if all workers are sleeping
     */
    bool CheckAllWorkersSleeping(DeadlockCheckResult& result);

    /**
     * @brief Check if majority of workers are sleeping
     */
    bool CheckMajorityWorkersSleeping(DeadlockCheckResult& result);

    /**
     * @brief Check for stuck workers
     */
    bool CheckStuckWorkers(DeadlockCheckResult& result);

    /**
     * @brief Check queue growth patterns
     */
    bool CheckQueueGrowth(DeadlockCheckResult& result);

    /**
     * @brief Generate diagnostic dump file
     */
    void GenerateDiagnosticDump(const DeadlockCheckResult& result);

    /**
     * @brief Attempt automatic recovery
     */
    void AttemptRecovery(const DeadlockCheckResult& result);

    /**
     * @brief Notify registered callbacks
     */
    void NotifyCallbacks(const DeadlockCheckResult& result);

    /**
     * @brief Log detection result
     */
    void LogResult(const DeadlockCheckResult& result);

    /**
     * @brief Get worker diagnostics safely
     */
    ::std::vector<::std::pair<uint32, WorkerDiagnostics*>> GetWorkerDiagnostics();

    /**
     * @brief Format timestamp for file names
     */
    ::std::string GetTimestampString() const;
};

/**
 * @brief Scoped deadlock detection pause (RAII)
 *
 * Temporarily pauses detection during critical operations
 */
class ScopedDeadlockDetectionPause
{
private:
    DeadlockDetector* _detector;

public:
    explicit ScopedDeadlockDetectionPause(DeadlockDetector* detector)
        : _detector(detector)
    {
        if (_detector)
            _detector->Pause();
    }

    ~ScopedDeadlockDetectionPause()
    {
        if (_detector)
            _detector->Resume();
    }

    // Delete copy/move
    ScopedDeadlockDetectionPause(const ScopedDeadlockDetectionPause&) = delete;
    ScopedDeadlockDetectionPause& operator=(const ScopedDeadlockDetectionPause&) = delete;
    ScopedDeadlockDetectionPause(ScopedDeadlockDetectionPause&&) = delete;
    ScopedDeadlockDetectionPause& operator=(ScopedDeadlockDetectionPause&&) = delete;
};

} // namespace Performance
} // namespace Playerbot

#endif // PLAYERBOT_DEADLOCK_DETECTOR_H