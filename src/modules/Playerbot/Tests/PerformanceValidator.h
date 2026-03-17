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
#include "TestUtilities.h"
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <functional>
#include <unordered_map>

namespace Playerbot
{
namespace Test
{

/**
 * @struct PerformanceThresholds
 * @brief Defines acceptable performance thresholds for group functionality
 */
struct PerformanceThresholds
{
    // Timing thresholds (microseconds)
    uint64 maxInvitationAcceptanceTime = 3000000;    // 3 seconds
    uint64 maxFollowingEngagementTime = 5000000;     // 5 seconds
    uint64 maxCombatEngagementTime = 3000000;        // 3 seconds
    uint64 maxTargetSwitchTime = 1000000;           // 1 second
    uint64 maxTeleportTime = 2000000;               // 2 seconds

    // Memory thresholds (bytes)
    uint64 maxMemoryPerBot = 10ULL * 1024 * 1024;   // 10MB per bot
    uint64 maxTotalMemoryGrowth = 100ULL * 1024 * 1024; // 100MB total growth during test

    // CPU thresholds (percentage)
    float maxCpuUsage = 90.0f;                      // 90% max CPU usage
    float maxCpuPerBot = 0.1f;                      // 0.1% CPU per bot

    // Success rate thresholds
    float minSuccessRate = 0.95f;                   // 95% minimum success rate
    float minUptimePercentage = 0.99f;              // 99% uptime requirement

    // Network and database thresholds
    uint64 maxDatabaseQueryTime = 10000;            // 10ms max database query time
    uint64 maxNetworkLatency = 100000;              // 100ms max network latency
    uint32 maxConcurrentConnections = 5000;        // 5000 max concurrent connections

    // Scalability thresholds
    uint32 maxBotsPerGroup = 5;                     // Standard group size
    uint32 maxConcurrentGroups = 1000;              // 1000 max concurrent groups
    uint32 maxTotalBots = 5000;                     // 5000 max total bots
};

/**
 * @struct PerformanceBenchmark
 * @brief Represents a performance benchmark result
 */
struct PerformanceBenchmark
{
    ::std::string testName;
    ::std::string category;

    // Timing metrics
    uint64 averageResponseTime = 0;
    uint64 minResponseTime = UINT64_MAX;
    uint64 maxResponseTime = 0;
    uint64 p95ResponseTime = 0;
    uint64 p99ResponseTime = 0;

    // Throughput metrics
    uint32 operationsPerSecond = 0;
    uint32 totalOperations = 0;
    uint32 successfulOperations = 0;
    uint32 failedOperations = 0;

    // Resource metrics
    uint64 peakMemoryUsage = 0;
    float averageCpuUsage = 0.0f;
    float peakCpuUsage = 0.0f;

    // Reliability metrics
    uint32 totalTests = 0;
    uint32 passedTests = 0;
    uint32 failedTests = 0;
    ::std::vector<::std::string> failures;

    // Calculated metrics
    float GetSuccessRate() const { return totalOperations > 0 ? (float)successfulOperations / totalOperations : 0.0f; }
    float GetPassRate() const { return totalTests > 0 ? (float)passedTests / totalTests : 0.0f; }
};

/**
 * @struct SystemHealthMetrics
 * @brief Comprehensive system health metrics
 */
struct SystemHealthMetrics
{
    // System resource usage
    uint64 totalMemoryUsage = 0;
    uint64 availableMemory = 0;
    float systemCpuUsage = 0.0f;
    float processCpuUsage = 0.0f;

    // Database metrics
    uint32 activeDbConnections = 0;
    uint32 dbQueriesPerSecond = 0;
    uint64 averageDbQueryTime = 0;
    uint32 failedDbQueries = 0;

    // Network metrics
    uint32 activeNetworkConnections = 0;
    uint64 networkBytesPerSecond = 0;
    uint32 networkErrors = 0;
    uint64 averageLatency = 0;

    // Bot-specific metrics
    uint32 activeBots = 0;
    uint32 botsInGroups = 0;
    uint32 activeGroups = 0;
    uint32 botsInCombat = 0;

    // Error tracking
    uint32 totalErrors = 0;
    uint32 criticalErrors = 0;
    uint32 warnings = 0;
    ::std::vector<::std::string> recentErrors;

    bool IsHealthy(const PerformanceThresholds& thresholds) const;
    ::std::string GetHealthSummary() const;
};

/**
 * @class PerformanceValidator
 * @brief Validates performance metrics against defined thresholds
 */
class PerformanceValidator
{
public:
    explicit PerformanceValidator(const PerformanceThresholds& thresholds = PerformanceThresholds{});
    ~PerformanceValidator() = default;

    // Threshold management
    void SetThresholds(const PerformanceThresholds& thresholds);
    const PerformanceThresholds& GetThresholds() const;
    void UpdateThreshold(const ::std::string& metric, double value);

    // Validation methods
    bool ValidateTimingMetrics(const PerformanceMetrics& metrics) const;
    bool ValidateMemoryMetrics(const PerformanceMetrics& metrics, uint32 botCount) const;
    bool ValidateCpuMetrics(const PerformanceMetrics& metrics, uint32 botCount) const;
    bool ValidateSuccessRates(const PerformanceMetrics& metrics) const;
    bool ValidateScalabilityMetrics(uint32 totalBots, uint32 groupCount) const;

    // Comprehensive validation
    bool ValidateAllMetrics(const PerformanceMetrics& metrics, uint32 botCount = 0) const;
    ::std::vector<::std::string> GetValidationFailures(const PerformanceMetrics& metrics, uint32 botCount = 0) const;

    // Benchmark management
    void RecordBenchmark(const PerformanceBenchmark& benchmark);
    ::std::vector<PerformanceBenchmark> GetBenchmarks(const ::std::string& category = "") const;
    PerformanceBenchmark GetAggregatedBenchmark(const ::std::string& category) const;

    // System health monitoring
    SystemHealthMetrics GetCurrentSystemHealth() const;
    bool ValidateSystemHealth() const;
    ::std::string GenerateHealthReport() const;

    // Performance analysis
    void AnalyzePerformanceTrends();
    ::std::string GeneratePerformanceReport() const;
    void ExportBenchmarkData(const ::std::string& filename) const;
    void ImportBenchmarkData(const ::std::string& filename);

    // Threshold recommendations
    PerformanceThresholds RecommendThresholds(const ::std::vector<PerformanceBenchmark>& benchmarks) const;
    void AutoTuneThresholds(float confidenceLevel = 0.95f);

private:
    PerformanceThresholds m_thresholds;
    ::std::vector<PerformanceBenchmark> m_benchmarks;
    ::std::unordered_map<::std::string, ::std::vector<double>> m_performanceTrends;

    // Internal validation helpers
    bool ValidateTimingThreshold(uint64 actualTime, uint64 threshold, const ::std::string& metric) const;
    bool ValidateMemoryThreshold(uint64 actualMemory, uint64 threshold, const ::std::string& metric) const;
    bool ValidateCpuThreshold(float actualCpu, float threshold, const ::std::string& metric) const;

    // System monitoring helpers
    uint64 GetCurrentMemoryUsage() const;
    float GetCurrentCpuUsage() const;
    uint32 GetActiveConnections() const;
    uint32 GetDatabaseMetrics() const;

    // Statistical analysis
    ::std::vector<double> CalculatePercentiles(::std::vector<uint64> values) const;
    double CalculateStandardDeviation(const ::std::vector<double>& values) const;
    bool IsOutlier(double value, const ::std::vector<double>& dataset, double threshold = 2.0) const;
};

/**
 * @class PerformanceProfiler
 * @brief Advanced performance profiling and analysis
 */
class PerformanceProfiler
{
public:
    PerformanceProfiler();
    ~PerformanceProfiler();

    // Profiling control
    void StartProfiling(const ::std::string& sessionName);
    void StopProfiling();
    bool IsProfilingActive() const;

    // Metric collection
    void RecordOperation(const ::std::string& operationType, uint64 duration);
    void RecordMemorySnapshot(uint64 memoryUsage);
    void RecordCpuSnapshot(float cpuUsage);
    void RecordCustomMetric(const ::std::string& metricName, double value);

    // Analysis methods
    PerformanceBenchmark GenerateBenchmark() const;
    ::std::string GenerateDetailedReport() const;
    void IdentifyBottlenecks();
    ::std::vector<::std::string> GetOptimizationSuggestions() const;

    // Real-time monitoring
    void SetRealTimeThresholds(const PerformanceThresholds& thresholds);
    void EnableRealTimeAlerting(::std::function<void(const ::std::string&)> callback);
    void CheckRealTimeThresholds();

    // Data export/import
    void ExportProfilingData(const ::std::string& filename) const;
    bool ImportProfilingData(const ::std::string& filename);

private:
    struct ProfilingSession
    {
        ::std::string name;
        ::std::chrono::high_resolution_clock::time_point startTime;
        ::std::chrono::high_resolution_clock::time_point endTime;
        ::std::unordered_map<::std::string, ::std::vector<uint64>> operationTimes;
        ::std::vector<uint64> memorySnapshots;
        ::std::vector<float> cpuSnapshots;
        ::std::unordered_map<::std::string, ::std::vector<double>> customMetrics;
    };

    bool m_profilingActive = false;
    ::std::unique_ptr<ProfilingSession> m_currentSession;
    PerformanceThresholds m_realTimeThresholds;
    ::std::function<void(const ::std::string&)> m_alertCallback;

    // Analysis helpers
    void AnalyzeOperationPerformance();
    void AnalyzeResourceUsage();
    void AnalyzeTrendData();
    ::std::vector<::std::string> m_optimizationSuggestions;
    ::std::vector<::std::string> m_identifiedBottlenecks;
};

/**
 * @class LoadTestRunner
 * @brief Specialized runner for load testing scenarios
 */
class LoadTestRunner
{
public:
    struct LoadTestConfig
    {
        uint32 maxConcurrentBots = 5000;
        uint32 botsPerGroup = 4;
        uint32 rampUpTimeSeconds = 300;      // 5 minutes
        uint32 sustainedLoadSeconds = 1800;  // 30 minutes
        uint32 rampDownTimeSeconds = 300;    // 5 minutes
        float targetCpuUtilization = 80.0f;
        uint64 maxMemoryUsage = 50ULL * 1024 * 1024 * 1024; // 50GB
    };

    explicit LoadTestRunner(const LoadTestConfig& config = LoadTestConfig{});

    // Load test execution
    bool RunScalabilityTest();
    bool RunEnduranceTest();
    bool RunStressTest();
    bool RunSpikeTest();

    // Results and analysis
    ::std::vector<PerformanceBenchmark> GetLoadTestResults() const;
    ::std::string GenerateLoadTestReport() const;
    bool ValidateLoadTestResults(const PerformanceValidator& validator) const;

    // Configuration
    void SetConfig(const LoadTestConfig& config);
    const LoadTestConfig& GetConfig() const;

private:
    LoadTestConfig m_config;
    ::std::vector<PerformanceBenchmark> m_loadTestResults;
    ::std::unique_ptr<PerformanceProfiler> m_profiler;

    // Load test implementations
    bool ExecuteRampUp(uint32 targetBots, uint32 rampTimeSeconds);
    bool ExecuteSustainedLoad(uint32 botCount, uint32 durationSeconds);
    bool ExecuteRampDown(uint32 fromBots, uint32 rampTimeSeconds);

    // Monitoring during load tests
    void MonitorSystemDuringLoad();
    void RecordLoadMetrics(uint32 currentBots);
    bool IsSystemStable() const;
};

// Utility macros for performance validation
#define VALIDATE_PERFORMANCE(validator, metrics, botCount) \
    EXPECT_TRUE(validator.ValidateAllMetrics(metrics, botCount)) \
    << "Performance validation failed: " << [&]() { \
        auto failures = validator.GetValidationFailures(metrics, botCount); \
        ::std::string result; \
        for (const auto& failure : failures) result += failure + "; "; \
        return result; \
    }()

#define EXPECT_TIMING_VALID(validator, actual, threshold, metric) \
    EXPECT_TRUE(validator.ValidateTimingThreshold(actual, threshold, metric)) \
    << metric << " timing " << (actual / 1000.0f) << "ms exceeds threshold " << (threshold / 1000.0f) << "ms"

#define EXPECT_MEMORY_WITHIN_LIMITS(validator, actual, threshold, description) \
    EXPECT_TRUE(validator.ValidateMemoryThreshold(actual, threshold, description)) \
    << description << " memory usage " << (actual / (1024*1024)) << "MB exceeds threshold " << (threshold / (1024*1024)) << "MB"

#define EXPECT_CPU_WITHIN_LIMITS(validator, actual, threshold, description) \
    EXPECT_TRUE(validator.ValidateCpuThreshold(actual, threshold, description)) \
    << description << " CPU usage " << actual << "% exceeds threshold " << threshold << "%"

} // namespace Test
} // namespace Playerbot