/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "BotPerformanceMonitor.h"
#include "BotPerformanceAnalytics.h"
#include "BotMemoryManager.h"
#include "AIDecisionProfiler.h"
#include "DatabaseQueryOptimizer.h"
#include "BotLoadTester.h"
#include <chrono>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <condition_variable>
#include <queue>
#include <fstream>
#include <stack>

namespace Playerbot
{

// Profiling scope levels for granular analysis
enum class ProfilingScope : uint8
{
    SYSTEM_WIDE     = 0,  // Entire server performance
    BOT_SPECIFIC    = 1,  // Individual bot performance
    CLASS_SPECIFIC  = 2,  // Per-class analysis
    GROUP_ANALYSIS  = 3,  // Group/raid performance
    ZONE_ANALYSIS   = 4,  // Zone-specific performance
    FEATURE_FOCUSED = 5,  // Specific feature profiling
    CUSTOM_SCOPE    = 6   // User-defined scope
};

// Profiling session types
enum class ProfilingSessionType : uint8
{
    CONTINUOUS      = 0,  // Always-on background profiling
    SNAPSHOT        = 1,  // Point-in-time analysis
    BENCHMARK       = 2,  // Comparative benchmarking
    REGRESSION      = 3,  // Performance regression testing
    STRESS_PROFILE  = 4,  // Under stress conditions
    IDLE_BASELINE   = 5,  // Baseline measurements
    CUSTOM_SESSION  = 6   // User-defined session
};

// Performance hotspot categories
enum class HotspotCategory : uint8
{
    CPU_INTENSIVE       = 0,  // High CPU usage areas
    MEMORY_INTENSIVE    = 1,  // High memory usage areas
    IO_BOUND           = 2,  // Database/file I/O bottlenecks
    NETWORK_BOUND      = 3,  // Network-related delays
    SYNCHRONIZATION    = 4,  // Lock contention issues
    ALLOCATION_HEAVY   = 5,  // Memory allocation hotspots
    CACHE_MISSES       = 6,  // Cache inefficiency
    ALGORITHMIC        = 7,  // Algorithm efficiency issues
    UNKNOWN_HOTSPOT    = 8
};

// Profiling data aggregation levels
enum class AggregationLevel : uint8
{
    RAW_DATA        = 0,  // Individual measurements
    PER_SECOND      = 1,  // Second-level aggregation
    PER_MINUTE      = 2,  // Minute-level aggregation
    PER_HOUR        = 3,  // Hour-level aggregation
    PER_DAY         = 4,  // Daily aggregation
    CUSTOM_INTERVAL = 5   // User-defined interval
};

// Performance call stack frame
struct ProfileCallFrame
{
    ::std::string functionName;
    ::std::string fileName;
    uint32_t lineNumber;
    uint64_t timestamp;
    uint64_t duration;
    uint64_t inclusiveTime;   // Time including child calls
    uint64_t exclusiveTime;   // Time excluding child calls
    uint32_t callCount;
    ::std::vector<::std::shared_ptr<ProfileCallFrame>> children;

    ProfileCallFrame() : lineNumber(0), timestamp(0), duration(0),
        inclusiveTime(0), exclusiveTime(0), callCount(0) {}

    ProfileCallFrame(const ::std::string& func, const ::std::string& file, uint32_t line)
        : functionName(func), fileName(file), lineNumber(line), timestamp(0),
          duration(0), inclusiveTime(0), exclusiveTime(0), callCount(1) {}
};

// Comprehensive performance hotspot data
struct PerformanceHotspot
{
    HotspotCategory category;
    ::std::string location;         // Function/method name
    ::std::string description;      // Human-readable description
    double impact;               // Performance impact score (0-100)
    uint64_t hitCount;           // Number of times hit
    uint64_t totalTime;          // Total time spent (microseconds)
    uint64_t averageTime;        // Average time per hit
    uint64_t maxTime;            // Maximum time observed
    double percentOfTotal;       // Percentage of total execution time
    ::std::string stackTrace;      // Call stack leading to hotspot
    ::std::vector<::std::string> optimizationSuggestions;

    PerformanceHotspot() : category(HotspotCategory::UNKNOWN_HOTSPOT), impact(0.0),
        hitCount(0), totalTime(0), averageTime(0), maxTime(0), percentOfTotal(0.0) {}

    PerformanceHotspot(HotspotCategory cat, const ::std::string& loc, double imp)
        : category(cat), location(loc), impact(imp), hitCount(0), totalTime(0),
          averageTime(0), maxTime(0), percentOfTotal(0.0) {}
};

// Profiling session configuration
struct ProfilingSession
{
    uint64_t sessionId;
    ProfilingSessionType type;
    ProfilingScope scope;
    ::std::string sessionName;
    ::std::string description;
    uint64_t startTime;
    uint64_t endTime;
    uint64_t duration;
    bool active;

    // Configuration parameters
    uint32_t samplingIntervalMs;
    bool enableCallStack;
    bool enableMemoryProfiling;
    bool enableDatabaseProfiling;
    bool enableNetworkProfiling;
    uint32_t maxCallStackDepth;
    ::std::vector<uint32_t> targetBotGuids;
    ::std::vector<::std::string> targetFunctions;

    // Results
    ::std::vector<PerformanceHotspot> hotspots;
    ::std::vector<::std::shared_ptr<ProfileCallFrame>> callStacks;
    ::std::unordered_map<::std::string, double> aggregatedMetrics;

    ProfilingSession() : sessionId(0), type(ProfilingSessionType::CONTINUOUS),
        scope(ProfilingScope::SYSTEM_WIDE), startTime(0), endTime(0), duration(0),
        active(false), samplingIntervalMs(100), enableCallStack(true),
        enableMemoryProfiling(true), enableDatabaseProfiling(true),
        enableNetworkProfiling(true), maxCallStackDepth(50) {}

    ProfilingSession(uint64_t id, const ::std::string& name, ProfilingSessionType sessionType)
        : sessionId(id), type(sessionType), scope(ProfilingScope::SYSTEM_WIDE),
          sessionName(name), startTime(0), endTime(0), duration(0), active(false),
          samplingIntervalMs(100), enableCallStack(true), enableMemoryProfiling(true),
          enableDatabaseProfiling(true), enableNetworkProfiling(true), maxCallStackDepth(50) {}
};

// Real-time profiling statistics
struct RealTimeProfilingStats
{
    ::std::atomic<uint64_t> totalSamples{0};
    ::std::atomic<uint64_t> activeProfilers{0};
    ::std::atomic<uint64_t> hotspotsDetected{0};
    ::std::atomic<double> overheadPercentage{0.0};
    ::std::atomic<uint64_t> memoryUsed{0};
    ::std::atomic<uint64_t> lastUpdateTime{0};

    // Performance counters
    ::std::atomic<uint64_t> cpuSamples{0};
    ::std::atomic<uint64_t> memorySamples{0};
    ::std::atomic<uint64_t> databaseSamples{0};
    ::std::atomic<uint64_t> networkSamples{0};

    void Reset()
    {
        totalSamples.store(0);
        activeProfilers.store(0);
        hotspotsDetected.store(0);
        overheadPercentage.store(0.0);
        memoryUsed.store(0);
        cpuSamples.store(0);
        memorySamples.store(0);
        databaseSamples.store(0);
        networkSamples.store(0);
        lastUpdateTime.store(::std::chrono::duration_cast<::std::chrono::microseconds>(
            ::std::chrono::steady_clock::now().time_since_epoch()).count());
    }
};

// RAII-based function profiler
class TC_GAME_API ScopedProfiler
{
public:
    ScopedProfiler(const ::std::string& functionName, const ::std::string& fileName = "", uint32_t lineNumber = 0);
    ~ScopedProfiler();

    // Disable copy/move
    ScopedProfiler(const ScopedProfiler&) = delete;
    ScopedProfiler& operator=(const ScopedProfiler&) = delete;
    ScopedProfiler(ScopedProfiler&&) = delete;
    ScopedProfiler& operator=(ScopedProfiler&&) = delete;

    // Manual control
    void AddMetadata(const ::std::string& key, const ::std::string& value);
    void SetCategory(HotspotCategory category);
    void MarkAsHotspot(double impact);

private:
    void RecordProfile();

    ::std::string _functionName;
    ::std::string _fileName;
    uint32_t _lineNumber;
    uint64_t _startTime;
    HotspotCategory _category;
    double _impactScore;
    bool _isHotspot;
    ::std::unordered_map<::std::string, ::std::string> _metadata;
};

// Main profiler engine
class TC_GAME_API BotProfiler
{
public:
    static BotProfiler& Instance()
    {
        static BotProfiler instance;
        return instance;
    }

    // Initialization and configuration
    bool Initialize();
    void Shutdown();
    bool IsEnabled() const { return _enabled.load(); }

    // Session management
    uint64_t StartProfilingSession(const ::std::string& sessionName, ProfilingSessionType type = ProfilingSessionType::CONTINUOUS);
    bool StopProfilingSession(uint64_t sessionId);
    bool PauseProfilingSession(uint64_t sessionId);
    bool ResumeProfilingSession(uint64_t sessionId);
    void StopAllSessions();

    // Session configuration
    void ConfigureSession(uint64_t sessionId, ProfilingScope scope, uint32_t samplingIntervalMs = 100);
    void SetSessionTargets(uint64_t sessionId, const ::std::vector<uint32_t>& botGuids);
    void SetSessionFunctions(uint64_t sessionId, const ::std::vector<::std::string>& functions);
    void EnableCallStackProfiling(uint64_t sessionId, bool enable, uint32_t maxDepth = 50);

    // Data collection
    void RecordFunctionCall(const ::std::string& functionName, uint64_t duration, uint32_t botGuid = 0);
    void RecordHotspot(const PerformanceHotspot& hotspot, uint64_t sessionId = 0);
    void RecordCallStack(const ::std::vector<::std::string>& callStack, uint64_t duration, uint64_t sessionId = 0);

    // Analysis and reporting
    ::std::vector<PerformanceHotspot> AnalyzeHotspots(uint64_t sessionId = 0, uint32_t topN = 10);
    ::std::vector<PerformanceHotspot> FindBottlenecks(HotspotCategory category, uint64_t sessionId = 0);
    ::std::shared_ptr<ProfileCallFrame> GetCallStackAnalysis(uint64_t sessionId = 0);
    ::std::vector<::std::string> GetOptimizationRecommendations(uint64_t sessionId = 0);

    // Real-time monitoring
    RealTimeProfilingStats GetRealTimeStats() const;
    ::std::vector<PerformanceHotspot> GetCurrentHotspots(uint32_t topN = 5);
    double GetProfilingOverhead() const;

    // Integration with other performance systems
    void IntegratePerformanceMonitor();
    void IntegrateAnalytics();
    void IntegrateMemoryManager();
    void IntegrateAIProfiler();
    void IntegrateDatabaseOptimizer();
    void IntegrateLoadTester();

    // Comparative analysis
    bool CompareProfilingSessions(uint64_t sessionId1, uint64_t sessionId2, ::std::string& comparisonReport);
    bool CompareBotPerformance(uint32_t botGuid1, uint32_t botGuid2, ::std::string& comparisonReport);
    void GenerateRegressionReport(const ::std::vector<uint64_t>& sessionIds, ::std::string& report);

    // Export and visualization
    bool ExportProfilingData(uint64_t sessionId, const ::std::string& filename, const ::std::string& format = "json");
    bool GenerateFlameGraph(uint64_t sessionId, const ::std::string& outputFile);
    bool GeneratePerformanceReport(uint64_t sessionId, ::std::string& report);
    bool GenerateHotspotReport(const ::std::vector<PerformanceHotspot>& hotspots, ::std::string& report);

    // Statistical analysis
    void PerformStatisticalAnalysis(uint64_t sessionId);
    ::std::vector<::std::pair<::std::string, double>> GetCorrelationAnalysis(uint64_t sessionId);
    ::std::vector<::std::string> IdentifyPerformancePatterns(uint64_t sessionId);

    // Advanced profiling features
    void StartSamplingProfiler(uint32_t intervalMicroseconds = 10000);
    void StopSamplingProfiler();
    void EnableInstrumentationProfiling(bool enable);
    void SetProfilingMode(const ::std::string& mode); // "sampling", "instrumentation", "hybrid"

    // Session queries
    ::std::vector<ProfilingSession> GetActiveSessions() const;
    ::std::vector<ProfilingSession> GetSessionHistory() const;
    ProfilingSession GetSession(uint64_t sessionId) const;
    bool IsSessionActive(uint64_t sessionId) const;

    // Configuration
    void SetEnabled(bool enabled) { _enabled.store(enabled); }
    void SetMaxSessions(uint32_t maxSessions) { _maxSessions = maxSessions; }
    void SetDataRetentionDays(uint32_t days) { _dataRetentionDays = days; }
    void SetSamplingRate(double rate) { _samplingRate = rate; }
    void SetProfilingOverheadLimit(double percent) { _maxOverheadPercent = percent; }

    // Utility functions
    ::std::string GetHotspotCategoryName(HotspotCategory category) const;
    ::std::string GetSessionTypeName(ProfilingSessionType type) const;
    ::std::string GetScopeName(ProfilingScope scope) const;

private:
    BotProfiler() = default;
    ~BotProfiler() = default;

    // Internal session management
    ProfilingSession* GetActiveSession(uint64_t sessionId);
    uint64_t GenerateSessionId();
    void CleanupExpiredSessions();

    // Data collection internals
    void CollectSample();
    void ProcessCallStack(const ::std::vector<::std::string>& callStack, uint64_t duration, ProfilingSession* session);
    void UpdateHotspotStatistics(PerformanceHotspot& hotspot, uint64_t duration);

    // Analysis internals
    void AnalyzeSessionData(ProfilingSession* session);
    void CalculateHotspotImpact(PerformanceHotspot& hotspot, const ProfilingSession* session);
    void BuildCallTree(ProfilingSession* session);
    void DetectPerformanceAnomalies(ProfilingSession* session);

    // Integration helpers
    void SyncWithPerformanceMonitor();
    void SyncWithAnalytics();
    void SyncWithMemoryManager();
    void SyncWithAIProfiler();
    void SyncWithDatabaseOptimizer();
    void SyncWithLoadTester();

    // Reporting helpers
    void GenerateSessionSummary(const ProfilingSession* session, ::std::string& summary);
    void GenerateHotspotAnalysis(const ::std::vector<PerformanceHotspot>& hotspots, ::std::string& analysis);
    void GenerateCallStackReport(const ::std::shared_ptr<ProfileCallFrame>& rootFrame, ::std::string& report);

    // Export helpers
    void ExportToJSON(const ProfilingSession* session, ::std::ofstream& file);
    void ExportToCSV(const ProfilingSession* session, ::std::ofstream& file);
    void ExportToXML(const ProfilingSession* session, ::std::ofstream& file);

    // Statistical helpers
    double CalculateStatisticalSignificance(const ::std::vector<double>& baseline, const ::std::vector<double>& current);
    ::std::vector<double> CalculatePercentiles(const ::std::vector<uint64_t>& values);
    double CalculateCorrelation(const ::std::vector<double>& x, const ::std::vector<double>& y);

    // Threading and synchronization
    void StartBackgroundProcessing();
    void StopBackgroundProcessing();
    void ProcessProfilingData();
    void UpdateRealTimeStatistics();

    // Configuration
    ::std::atomic<bool> _enabled{false};
    ::std::atomic<bool> _shutdownRequested{false};
    ::std::atomic<bool> _samplingActive{false};
    ::std::atomic<bool> _instrumentationActive{false};
    ::std::atomic<uint32_t> _maxSessions{100};
    ::std::atomic<uint32_t> _dataRetentionDays{30};
    ::std::atomic<double> _samplingRate{1.0};
    ::std::atomic<double> _maxOverheadPercent{5.0};

    // Session management
    mutable ::std::recursive_mutex _sessionsMutex;
    ::std::unordered_map<uint64_t, ProfilingSession> _activeSessions;
    ::std::vector<ProfilingSession> _sessionHistory;
    ::std::atomic<uint64_t> _nextSessionId{1};

    // Data storage
    mutable ::std::recursive_mutex _dataMutex;
    ::std::queue<::std::function<void()>> _pendingOperations;
    ::std::vector<PerformanceHotspot> _globalHotspots;

    // Real-time statistics
    RealTimeProfilingStats _realTimeStats;
    mutable ::std::recursive_mutex _statsMutex;

    // Background processing
    ::std::thread _processingThread;
    ::std::thread _samplingThread;
    ::std::condition_variable _processingCondition;
    ::std::recursive_mutex _processingMutex;

    // Call stack tracking
    thread_local ::std::stack<::std::shared_ptr<ProfileCallFrame>> _callStack;
    mutable ::std::recursive_mutex _callStackMutex;

    // Integration states
    ::std::atomic<bool> _performanceMonitorIntegrated{false};
    ::std::atomic<bool> _analyticsIntegrated{false};
    ::std::atomic<bool> _memoryManagerIntegrated{false};
    ::std::atomic<bool> _aiProfilerIntegrated{false};
    ::std::atomic<bool> _databaseOptimizerIntegrated{false};
    ::std::atomic<bool> _loadTesterIntegrated{false};

    // Constants
    static constexpr uint32_t MAX_PROFILING_SESSIONS = 1000;
    static constexpr uint32_t DEFAULT_SAMPLING_INTERVAL_US = 10000; // 10ms
    static constexpr uint32_t MAX_CALL_STACK_DEPTH = 100;
    static constexpr double DEFAULT_HOTSPOT_THRESHOLD = 5.0; // 5% of total time
    static constexpr uint32_t HOTSPOT_ANALYSIS_WINDOW = 300; // 5 minutes
    static constexpr double MAX_PROFILING_OVERHEAD = 10.0; // 10%
};

// Convenience macros for profiling
#define sProfiler BotProfiler::Instance()

// Only define if not already defined by another profiler header
#ifndef PROFILE_FUNCTION
#define PROFILE_FUNCTION() \
    ScopedProfiler _prof(__FUNCTION__, __FILE__, __LINE__)
#endif

#ifndef PROFILE_SCOPE
#define PROFILE_SCOPE(name) \
    ScopedProfiler _prof(name, __FILE__, __LINE__)
#endif

#define PROFILE_HOTSPOT(name, impact) \
    ScopedProfiler _prof(name, __FILE__, __LINE__); \
    _prof.MarkAsHotspot(impact)

#define START_PROFILING_SESSION(name, type) \
    sProfiler.StartProfilingSession(name, type)

#define STOP_PROFILING_SESSION(id) \
    sProfiler.StopProfilingSession(id)

#define RECORD_PERFORMANCE_HOTSPOT(category, location, impact) \
    do { \
        PerformanceHotspot hotspot(category, location, impact); \
        sProfiler.RecordHotspot(hotspot); \
    } while(0)

} // namespace Playerbot