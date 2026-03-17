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
#include "Threading/LockHierarchy.h"
#include "Player.h"
#include "Group.h"
#include <unordered_map>
#include <vector>
#include <atomic>
#include <mutex>
#include <chrono>

namespace Playerbot
{
    // Interface


enum class BenchmarkType : uint8
{
    AI_DECISION_SPEED       = 0,
    GROUP_COORDINATION      = 1,
    COMBAT_OPTIMIZATION     = 2,
    QUEST_EXECUTION         = 3,
    LOOT_PROCESSING        = 4,
    TRADE_OPERATIONS       = 5,
    AUCTION_ANALYSIS       = 6,
    GUILD_INTERACTIONS     = 7,
    DATABASE_OPERATIONS    = 8,
    MEMORY_EFFICIENCY      = 9,
    SCALABILITY_LIMITS     = 10
};

enum class LoadLevel : uint8
{
    LIGHT       = 0,    // 1-10 bots
    MODERATE    = 1,    // 11-50 bots
    HEAVY       = 2,    // 51-200 bots
    EXTREME     = 3,    // 201-500 bots
    STRESS      = 4     // 500+ bots
};

struct BenchmarkResult
{
    BenchmarkType type;
    LoadLevel loadLevel;
    uint32 botCount;
    uint32 duration;
    uint32 operationsPerSecond;
    float averageResponseTime; // milliseconds
    float cpuUsage; // percentage
    size_t memoryUsage; // bytes
    uint32 errorCount;
    bool passedTargets;
    ::std::vector<::std::string> bottlenecks;
    ::std::chrono::steady_clock::time_point timestamp;

    BenchmarkResult() : type(BenchmarkType::AI_DECISION_SPEED), loadLevel(LoadLevel::LIGHT)
        , botCount(0), duration(0), operationsPerSecond(0), averageResponseTime(0.0f)
        , cpuUsage(0.0f), memoryUsage(0), errorCount(0), passedTargets(false)
        , timestamp(::std::chrono::steady_clock::now()) {}
};

/**
 * @brief Comprehensive performance benchmarking system for playerbot optimization
 *
 * This system provides detailed performance analysis, stress testing, and optimization
 * insights for all playerbot systems under various load conditions.
 */
class TC_GAME_API PerformanceBenchmark final 
{
public:
    static PerformanceBenchmark* instance();

    // Core benchmarking framework
    BenchmarkResult RunBenchmark(BenchmarkType type, LoadLevel loadLevel, uint32 duration = 60000);
    ::std::vector<BenchmarkResult> RunBenchmarkSuite(LoadLevel loadLevel);
    void RunContinuousBenchmarking(uint32 intervalMs);
    void StopContinuousBenchmarking();

    // AI performance benchmarks
    BenchmarkResult BenchmarkAIDecisionSpeed(uint32 botCount, uint32 duration);
    BenchmarkResult BenchmarkCombatAI(uint32 botCount, uint32 duration);
    BenchmarkResult BenchmarkStrategyExecution(uint32 botCount, uint32 duration);
    BenchmarkResult BenchmarkActionSelection(uint32 botCount, uint32 duration);

    // System-specific benchmarks
    BenchmarkResult BenchmarkGroupCoordination(uint32 groupCount, uint32 duration);
    BenchmarkResult BenchmarkQuestExecution(uint32 botCount, uint32 duration);
    BenchmarkResult BenchmarkLootProcessing(uint32 botCount, uint32 duration);
    BenchmarkResult BenchmarkTradeOperations(uint32 botCount, uint32 duration);
    BenchmarkResult BenchmarkAuctionAnalysis(uint32 botCount, uint32 duration);
    BenchmarkResult BenchmarkGuildInteractions(uint32 botCount, uint32 duration);

    // Scalability testing
    struct ScalabilityTest
    {
        ::std::string testName;
        BenchmarkType benchmarkType;
        ::std::vector<uint32> botCounts; // Test at different bot counts
        uint32 duration;
        ::std::vector<BenchmarkResult> results;
        uint32 optimalBotCount;
        uint32 maximumBotCount;
        bool foundScalabilityLimit;

        ScalabilityTest() : benchmarkType(BenchmarkType::AI_DECISION_SPEED)
            , duration(60000), optimalBotCount(0), maximumBotCount(0)
            , foundScalabilityLimit(false) {}
    };

    ScalabilityTest RunScalabilityTest(BenchmarkType type);
    void FindPerformanceBreakpoints();
    uint32 DetermineOptimalBotCount();
    void AnalyzeScalingCharacteristics();

    // Resource utilization analysis
    struct ResourceAnalysis
    {
        float cpuUsageBaseline;
        float cpuUsagePerBot;
        size_t memoryUsageBaseline;
        size_t memoryUsagePerBot;
        uint32 databaseQueriesPerSecond;
        uint32 networkOperationsPerSecond;
        ::std::vector<::std::string> resourceBottlenecks;

        ResourceAnalysis() : cpuUsageBaseline(0.0f), cpuUsagePerBot(0.0f)
            , memoryUsageBaseline(0), memoryUsagePerBot(0)
            , databaseQueriesPerSecond(0), networkOperationsPerSecond(0) {}
    };

    ResourceAnalysis AnalyzeResourceUtilization(LoadLevel loadLevel);
    void ProfileMemoryUsagePatterns();
    void AnalyzeCPUHotspots();
    void MeasureDatabasePerformance();

    // Performance regression testing
    bool RunRegressionBenchmarks();
    void EstablishPerformanceBaseline();
    bool DetectPerformanceRegression();
    void CompareWithBaseline(const ::std::vector<BenchmarkResult>& currentResults);

    // Stress testing
    struct StressTest
    {
        ::std::string testName;
        uint32 maxBotCount;
        uint32 rampUpTime;
        uint32 sustainedLoadTime;
        uint32 rampDownTime;
        ::std::vector<BenchmarkResult> progressResults;
        bool systemStable;
        uint32 failurePoint;
        ::std::vector<::std::string> failureReasons;

        StressTest() : maxBotCount(1000), rampUpTime(300000), sustainedLoadTime(600000)
            , rampDownTime(300000), systemStable(true), failurePoint(0) {}
    };

    StressTest RunStressTest(const StressTest& testConfig);
    void TestSystemLimits();
    void MeasureRecoveryTime();
    bool ValidateSystemStability(uint32 duration);

    // Performance optimization insights
    struct OptimizationRecommendation
    {
        ::std::string area;
        ::std::string issue;
        ::std::string recommendation;
        float expectedImprovement; // Performance improvement percentage
        uint32 implementationComplexity; // 1-10 scale
        bool isHighPriority;

        OptimizationRecommendation() : expectedImprovement(0.0f)
            , implementationComplexity(5), isHighPriority(false) {}
    };

    ::std::vector<OptimizationRecommendation> GenerateOptimizationRecommendations();
    void AnalyzePerformancePatterns();
    void IdentifyBottlenecks();
    void SuggestConfigurationTuning();

    // Comparative benchmarking
    void CompareBenchmarkResults(const BenchmarkResult& baseline, const BenchmarkResult& current);
    void GeneratePerformanceReport();
    void TrackPerformanceTrends();
    void BenchmarkAgainstTargets();

    // Real-time monitoring
    void StartPerformanceMonitoring();
    void StopPerformanceMonitoring();
    struct PerformanceSnapshot
    {
        uint32 activeBotCount;
        float currentCpuUsage;
        size_t currentMemoryUsage;
        uint32 operationsPerSecond;
        uint32 averageResponseTime;
        uint32 errorRate;
        ::std::chrono::steady_clock::time_point timestamp;

        PerformanceSnapshot() : activeBotCount(0), currentCpuUsage(0.0f)
            , currentMemoryUsage(0), operationsPerSecond(0)
            , averageResponseTime(0), errorRate(0)
            , timestamp(::std::chrono::steady_clock::now()) {}
    };

    PerformanceSnapshot GetCurrentPerformanceSnapshot();
    ::std::vector<PerformanceSnapshot> GetPerformanceHistory(uint32 durationMs);

    // Configuration and settings
    void SetPerformanceTargets(BenchmarkType type, uint32 targetOps, float targetResponseTime);
    void SetBenchmarkTimeout(uint32 timeoutMs);
    void EnableDetailedProfiling(bool enable);
    void SetBenchmarkReportLevel(uint32 level);

    // Update and maintenance
    void Update(uint32 diff);
    void ProcessBenchmarkQueue();
    void CleanupBenchmarkData();

private:
    PerformanceBenchmark();
    ~PerformanceBenchmark() = default;

    // Core benchmark data
    ::std::unordered_map<BenchmarkType, ::std::vector<BenchmarkResult>> _benchmarkHistory;
    ::std::unordered_map<BenchmarkType, BenchmarkResult> _performanceBaseline;
    ::std::queue<PerformanceSnapshot> _performanceHistory;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BEHAVIOR_MANAGER> _benchmarkMutex;

    // Performance targets
    struct PerformanceTargets
    {
        BenchmarkType type;
        uint32 targetOperationsPerSecond;
        float targetResponseTimeMs;
        float targetCpuUsagePercent;
        size_t targetMemoryUsageMB;
        uint32 targetErrorRate;

        PerformanceTargets(BenchmarkType t) : type(t), targetOperationsPerSecond(1000)
            , targetResponseTimeMs(50.0f), targetCpuUsagePercent(70.0f)
            , targetMemoryUsageMB(512), targetErrorRate(0) {}
    };

    ::std::unordered_map<BenchmarkType, PerformanceTargets> _performanceTargets;

    // Monitoring state
    ::std::atomic<bool> _continuousBenchmarking{false};
    ::std::atomic<bool> _performanceMonitoring{false};
    ::std::atomic<uint32> _monitoringInterval{5000};
    uint32 _lastMonitoringTime{0};

    // Configuration
    ::std::atomic<uint32> _benchmarkTimeout{300000}; // 5 minutes
    ::std::atomic<bool> _detailedProfiling{false};
    ::std::atomic<uint32> _reportLevel{2}; // 0-3 verbosity

    // Helper functions
    void InitializePerformanceTargets();
    void SetupBenchmarkEnvironment(uint32 botCount);
    void CleanupBenchmarkEnvironment();
    Player* CreateBenchmarkBot(const ::std::string& name);

    // Measurement implementations
    void MeasureCPUUsage(BenchmarkResult& result, uint32 duration);
    void MeasureMemoryUsage(BenchmarkResult& result);
    void MeasureResponseTimes(BenchmarkResult& result, const ::std::vector<uint32>& responseTimes);
    void MeasureOperationThroughput(BenchmarkResult& result, uint32 operations, uint32 duration);

    // Benchmark implementations
    BenchmarkResult ExecuteAIDecisionBenchmark(uint32 botCount, uint32 duration);
    BenchmarkResult ExecuteGroupCoordinationBenchmark(uint32 groupCount, uint32 duration);
    BenchmarkResult ExecuteCombatBenchmark(uint32 botCount, uint32 duration);
    BenchmarkResult ExecuteQuestBenchmark(uint32 botCount, uint32 duration);
    BenchmarkResult ExecuteLootBenchmark(uint32 botCount, uint32 duration);
    BenchmarkResult ExecuteTradeBenchmark(uint32 botCount, uint32 duration);
    BenchmarkResult ExecuteAuctionBenchmark(uint32 botCount, uint32 duration);
    BenchmarkResult ExecuteGuildBenchmark(uint32 botCount, uint32 duration);
    BenchmarkResult ExecuteDatabaseBenchmark(uint32 botCount, uint32 duration);

    // Analysis implementations
    void AnalyzeBenchmarkResult(BenchmarkResult& result);
    void IdentifyPerformanceBottlenecks(BenchmarkResult& result);
    bool ValidatePerformanceTargets(const BenchmarkResult& result);
    void GenerateBenchmarkRecommendations(const BenchmarkResult& result);

    // Scalability analysis
    void AnalyzeScalingPattern(const ScalabilityTest& test);
    uint32 FindOptimalBotCount(const ScalabilityTest& test);
    uint32 FindMaximumBotCount(const ScalabilityTest& test);
    void PredictScalingBehavior(const ScalabilityTest& test);

    // Constants
    static constexpr uint32 BENCHMARK_UPDATE_INTERVAL = 1000; // 1 second
    static constexpr uint32 PERFORMANCE_HISTORY_SIZE = 1000;
    static constexpr uint32 DEFAULT_BENCHMARK_DURATION = 60000; // 1 minute
    static constexpr uint32 STRESS_TEST_MAX_BOTS = 1000;
    static constexpr float CPU_USAGE_WARNING_THRESHOLD = 80.0f;
    static constexpr size_t MEMORY_WARNING_THRESHOLD_MB = 2048; // 2GB
    static constexpr uint32 RESPONSE_TIME_WARNING_MS = 100;
    static constexpr uint32 MIN_OPERATIONS_PER_SECOND = 100;
    static constexpr uint32 MAX_ERROR_RATE_PERCENT = 5;
    static constexpr uint32 REGRESSION_THRESHOLD_PERCENT = 10;
};

} // namespace Playerbot