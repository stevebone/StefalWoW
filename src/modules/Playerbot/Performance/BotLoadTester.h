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
#include <functional>
#include <random>

namespace Playerbot
{

// Load test scenarios for different bot behaviors
enum class LoadTestScenario : uint8
{
    IDLE_BOTS           = 0,  // Bots standing idle
    RANDOM_MOVEMENT     = 1,  // Random movement around spawn
    COMBAT_TRAINING     = 2,  // Combat against training dummies
    DUNGEON_SIMULATION  = 3,  // Group dungeon behavior
    RAID_SIMULATION     = 4,  // 25-man raid behavior
    PVP_BATTLEGROUND    = 5,  // PvP combat simulation
    QUEST_AUTOMATION    = 6,  // Quest completion behavior
    AUCTION_HOUSE       = 7,  // Trading and auction activity
    GUILD_ACTIVITIES    = 8,  // Guild chat and activities
    MIXED_ACTIVITIES    = 9,  // Random mix of all behaviors
    STRESS_TEST         = 10, // Maximum load stress test
    MEMORY_PRESSURE     = 11, // Memory allocation stress
    DATABASE_INTENSIVE  = 12, // Heavy database operations
    NETWORK_SIMULATION  = 13, // Simulated network conditions
    UNKNOWN             = 14
};

// Load test phases for gradual scaling
enum class LoadTestPhase : uint8
{
    PREPARATION    = 0,  // Setup and validation
    WARMUP         = 1,  // Gradual bot spawning
    STEADY_STATE   = 2,  // Full load maintenance
    PEAK_LOAD      = 3,  // Maximum stress testing
    COOLDOWN       = 4,  // Gradual bot removal
    CLEANUP        = 5,  // Resource cleanup
    COMPLETED      = 6,  // Test finished
    FAILED         = 7   // Test failed
};

// Performance thresholds for different metrics
struct LoadTestThresholds
{
    double maxCpuUsagePercent = 80.0;           // Maximum CPU usage
    uint64_t maxMemoryUsageMB = 8192;           // Maximum memory usage (8GB)
    uint64_t maxResponseTimeMs = 100;           // Maximum response time
    double minTickRate = 45.0;                  // Minimum server tick rate
    uint64_t maxDatabaseLatencyMs = 50;         // Maximum database latency
    uint32_t maxPacketsPerSecond = 100000;     // Maximum packet rate
    double maxPacketLossPercent = 1.0;          // Maximum packet loss
    uint32_t maxConnectionsPerSecond = 1000;   // Maximum new connections/sec

    LoadTestThresholds() = default;

    LoadTestThresholds(double cpu, uint64_t memory, uint64_t response, double tick)
        : maxCpuUsagePercent(cpu), maxMemoryUsageMB(memory),
          maxResponseTimeMs(response), minTickRate(tick) {}
};

// Individual bot load test configuration
struct BotLoadTestConfig
{
    uint32_t botGuid;
    LoadTestScenario scenario;
    uint32_t durationSeconds;
    uint32_t actionIntervalMs;
    bool enableAI;
    bool enableCombat;
    bool enableMovement;
    bool enableSocial;
    ::std::string customBehavior;

    BotLoadTestConfig() : botGuid(0), scenario(LoadTestScenario::IDLE_BOTS),
        durationSeconds(300), actionIntervalMs(1000), enableAI(true),
        enableCombat(false), enableMovement(false), enableSocial(false) {}

    BotLoadTestConfig(uint32_t guid, LoadTestScenario scen, uint32_t duration)
        : botGuid(guid), scenario(scen), durationSeconds(duration),
          actionIntervalMs(1000), enableAI(true), enableCombat(true),
          enableMovement(true), enableSocial(true) {}
};

// Load test results for analysis
struct LoadTestResults
{
    LoadTestScenario scenario;
    uint32_t totalBots;
    uint32_t successfulBots;
    uint32_t failedBots;
    uint64_t testDurationMs;

    // Performance metrics
    double averageCpuUsage;
    double peakCpuUsage;
    uint64_t averageMemoryUsage;
    uint64_t peakMemoryUsage;
    uint64_t averageResponseTime;
    uint64_t maxResponseTime;
    double averageTickRate;
    double minTickRate;

    // Database metrics
    uint64_t totalQueries;
    uint64_t averageQueryTime;
    uint64_t maxQueryTime;
    uint32_t queryErrors;
    double queriesPerSecond;

    // Network metrics
    uint64_t totalPackets;
    uint64_t packetsPerSecond;
    double packetLossRate;
    uint64_t totalBandwidth;

    // Error tracking
    uint32_t crashCount;
    uint32_t timeoutCount;
    uint32_t memoryLeaks;
    ::std::vector<::std::string> errorMessages;

    // Scalability analysis
    double scalabilityScore;     // 0.0 to 100.0
    uint32_t maxStableBots;      // Maximum stable bot count
    uint32_t recommendedBots;    // Recommended bot count

    LoadTestResults() : scenario(LoadTestScenario::UNKNOWN), totalBots(0),
        successfulBots(0), failedBots(0), testDurationMs(0),
        averageCpuUsage(0.0), peakCpuUsage(0.0), averageMemoryUsage(0),
        peakMemoryUsage(0), averageResponseTime(0), maxResponseTime(0),
        averageTickRate(0.0), minTickRate(0.0), totalQueries(0),
        averageQueryTime(0), maxQueryTime(0), queryErrors(0),
        queriesPerSecond(0.0), totalPackets(0), packetsPerSecond(0),
        packetLossRate(0.0), totalBandwidth(0), crashCount(0),
        timeoutCount(0), memoryLeaks(0), scalabilityScore(0.0),
        maxStableBots(0), recommendedBots(0) {}
};

// Real-time load test monitoring data
struct LoadTestMonitorData
{
    ::std::atomic<uint32_t> currentBots{0};
    ::std::atomic<uint32_t> activeBots{0};
    ::std::atomic<uint32_t> idleBots{0};
    ::std::atomic<uint32_t> errorBots{0};

    ::std::atomic<double> currentCpuUsage{0.0};
    ::std::atomic<uint64_t> currentMemoryUsage{0};
    ::std::atomic<uint64_t> currentResponseTime{0};
    ::std::atomic<double> currentTickRate{0.0};

    ::std::atomic<uint64_t> packetsLastSecond{0};
    ::std::atomic<uint64_t> queriesLastSecond{0};
    ::std::atomic<uint32_t> errorsLastSecond{0};

    ::std::atomic<uint64_t> lastUpdateTime{0};

    void UpdateMetrics(const LoadTestResults& results)
    {
        currentCpuUsage.store(results.averageCpuUsage);
        currentMemoryUsage.store(results.averageMemoryUsage);
        currentResponseTime.store(results.averageResponseTime);
        currentTickRate.store(results.averageTickRate);
        lastUpdateTime.store(::std::chrono::duration_cast<::std::chrono::microseconds>(
            ::std::chrono::steady_clock::now().time_since_epoch()).count());
    }
};

// Load test execution engine
class TC_GAME_API BotLoadTester
{
public:
    static BotLoadTester& Instance()
    {
        static BotLoadTester instance;
        return instance;
    }

    // Initialization and configuration
    bool Initialize();
    void Shutdown();
    bool IsEnabled() const { return _enabled.load(); }

    // Test configuration
    void SetThresholds(const LoadTestThresholds& thresholds) { _thresholds = thresholds; }
    void SetMaxConcurrentBots(uint32_t maxBots) { _maxConcurrentBots = maxBots; }
    void SetTestDuration(uint32_t seconds) { _defaultTestDuration = seconds; }
    void SetRampUpTime(uint32_t seconds) { _rampUpTime = seconds; }
    void SetRampDownTime(uint32_t seconds) { _rampDownTime = seconds; }

    // Single scenario testing
    bool RunLoadTest(LoadTestScenario scenario, uint32_t botCount, uint32_t durationSeconds = 300);
    bool RunCustomTest(const ::std::vector<BotLoadTestConfig>& configs);

    // Comprehensive testing suites
    bool RunScalabilityTest(uint32_t startBots = 10, uint32_t maxBots = 500, uint32_t increment = 10);
    bool RunStressTest(uint32_t botCount = 1000, uint32_t durationSeconds = 1800);
    bool RunEnduranceTest(uint32_t botCount = 200, uint32_t durationHours = 24);
    bool RunPerformanceRegression(const ::std::vector<LoadTestScenario>& scenarios);

    // Test control
    void StopCurrentTest();
    void PauseTest();
    void ResumeTest();
    void AbortTest();

    // Monitoring and status
    LoadTestPhase GetCurrentPhase() const { return _currentPhase.load(); }
    LoadTestMonitorData GetCurrentMetrics() const;
    LoadTestResults GetLastResults() const;
    ::std::vector<LoadTestResults> GetTestHistory() const;

    // Real-time monitoring
    void StartRealTimeMonitoring();
    void StopRealTimeMonitoring();
    bool IsMonitoringActive() const { return _monitoringActive.load(); }

    // Bot management during tests
    uint32_t GetActiveBotCount() const;
    ::std::vector<uint32_t> GetTestBotGuids() const;
    bool IsBotInTest(uint32_t botGuid) const;

    // Performance analysis
    double CalculateScalabilityScore(const ::std::vector<LoadTestResults>& results) const;
    uint32_t FindOptimalBotCount(LoadTestScenario scenario) const;
    ::std::vector<::std::string> GenerateOptimizationRecommendations() const;

    // Reporting
    void GenerateLoadTestReport(::std::string& report, const LoadTestResults& results) const;
    void GenerateScalabilityReport(::std::string& report, const ::std::vector<LoadTestResults>& results) const;
    void GeneratePerformanceComparison(::std::string& report, LoadTestScenario scenario1, LoadTestScenario scenario2) const;
    void ExportResults(const ::std::string& filename, const ::std::vector<LoadTestResults>& results) const;

    // Test scenario management
    BotLoadTestConfig CreateScenarioConfig(LoadTestScenario scenario, uint32_t botGuid) const;
    void RegisterCustomScenario(const ::std::string& name, ::std::function<void(uint32_t)> behavior);
    ::std::vector<LoadTestScenario> GetAvailableScenarios() const;

    // Alert and notification system
    void SetAlertCallback(::std::function<void(const ::std::string&, const ::std::string&)> callback);
    void EnableAlerts(bool enable) { _alertsEnabled.store(enable); }

    // Configuration
    void SetEnabled(bool enabled) { _enabled.store(enabled); }
    void SetVerboseLogging(bool verbose) { _verboseLogging.store(verbose); }
    void SetMetricsInterval(uint32_t intervalMs) { _metricsInterval = intervalMs; }

private:
    BotLoadTester() = default;
    ~BotLoadTester() = default;

    // Test execution
    bool ExecuteLoadTest(const ::std::vector<BotLoadTestConfig>& configs);
    void ExecuteTestPhase(LoadTestPhase phase);
    void ProcessTestConfiguration();

    // Bot management
    bool SpawnTestBots(const ::std::vector<BotLoadTestConfig>& configs);
    void DespawnTestBots();
    void ConfigureTestBot(uint32_t botGuid, const BotLoadTestConfig& config);
    void UpdateBotBehavior(uint32_t botGuid, LoadTestScenario scenario);

    // Monitoring and metrics
    void MonitorTestProgress();
    void CollectPerformanceMetrics();
    void UpdateRealTimeMetrics();
    void CheckPerformanceThresholds();

    // Analysis and calculation
    void AnalyzeTestResults();
    void CalculatePerformanceMetrics(LoadTestResults& results);
    void DetectPerformanceBottlenecks(LoadTestResults& results);
    double CalculateScalabilityMetric(uint32_t botCount, const LoadTestResults& results) const;

    // Test scenarios implementation
    void ExecuteIdleBehavior(uint32_t botGuid);
    void ExecuteRandomMovement(uint32_t botGuid);
    void ExecuteCombatTraining(uint32_t botGuid);
    void ExecuteDungeonSimulation(uint32_t botGuid);
    void ExecuteRaidSimulation(uint32_t botGuid);
    void ExecutePvPBehavior(uint32_t botGuid);
    void ExecuteQuestBehavior(uint32_t botGuid);
    void ExecuteAuctionHouseBehavior(uint32_t botGuid);
    void ExecuteGuildBehavior(uint32_t botGuid);
    void ExecuteMixedBehavior(uint32_t botGuid);
    void ExecuteStressTest(uint32_t botGuid);
    void ExecuteMemoryPressure(uint32_t botGuid);
    void ExecuteDatabaseIntensive(uint32_t botGuid);
    void ExecuteNetworkSimulation(uint32_t botGuid);

    // Alert system
    void CheckForAlerts();
    void TriggerAlert(const ::std::string& alertType, const ::std::string& message);
    void HandleCriticalAlert(const ::std::string& message);

    // Resource management
    void CleanupTestResources();
    void ValidateSystemResources();
    bool CheckSystemRequirements(uint32_t botCount);

    // Data persistence
    void SaveTestResults(const LoadTestResults& results);
    void LoadTestHistory();
    void ArchiveOldResults();

    // Configuration
    ::std::atomic<bool> _enabled{false};
    ::std::atomic<bool> _testRunning{false};
    ::std::atomic<bool> _testPaused{false};
    ::std::atomic<bool> _shutdownRequested{false};
    ::std::atomic<bool> _monitoringActive{false};
    ::std::atomic<bool> _verboseLogging{false};
    ::std::atomic<bool> _alertsEnabled{true};

    // Test state
    ::std::atomic<LoadTestPhase> _currentPhase{LoadTestPhase::PREPARATION};
    LoadTestThresholds _thresholds;
    ::std::atomic<uint32_t> _maxConcurrentBots{500};
    ::std::atomic<uint32_t> _defaultTestDuration{300};
    ::std::atomic<uint32_t> _rampUpTime{60};
    ::std::atomic<uint32_t> _rampDownTime{60};
    ::std::atomic<uint32_t> _metricsInterval{1000};

    // Test data
    mutable ::std::recursive_mutex _testDataMutex;
    ::std::vector<BotLoadTestConfig> _currentTestConfigs;
    ::std::vector<uint32_t> _testBotGuids;
    LoadTestResults _currentResults;
    ::std::vector<LoadTestResults> _testHistory;
    LoadTestMonitorData _monitorData;

    // Custom scenarios
    mutable ::std::recursive_mutex _scenariosMutex;
    ::std::unordered_map<::std::string, ::std::function<void(uint32_t)>> _customScenarios;

    // Threading
    ::std::thread _testExecutionThread;
    ::std::thread _monitoringThread;
    ::std::thread _metricsThread;
    ::std::condition_variable _testCondition;
    ::std::recursive_mutex _testMutex;

    // Alert system
    ::std::function<void(const ::std::string&, const ::std::string&)> _alertCallback;
    mutable ::std::recursive_mutex _alertMutex;
    ::std::queue<::std::pair<::std::string, ::std::string>> _pendingAlerts;

    // Performance tracking
    ::std::atomic<uint64_t> _testStartTime{0};
    ::std::atomic<uint64_t> _lastMetricsUpdate{0};
    ::std::atomic<uint64_t> _totalTestsRun{0};
    ::std::atomic<uint64_t> _totalBotsSpawned{0};

    // Random number generation - Removed thread-unsafe std::mt19937
    // Now using TrinityCore's thread-safe urand() and frand() functions

    // Constants
    static constexpr uint32_t MAX_CONCURRENT_BOTS = 5000;
    static constexpr uint32_t MIN_TEST_DURATION = 30;      // 30 seconds
    static constexpr uint32_t MAX_TEST_DURATION = 86400;   // 24 hours
    static constexpr uint32_t DEFAULT_METRICS_INTERVAL = 1000; // 1 second
    static constexpr uint32_t HISTORY_RETENTION_DAYS = 30;
    static constexpr double SCALABILITY_THRESHOLD = 75.0;  // 75% efficiency
    static constexpr uint32_t ALERT_COOLDOWN_MS = 60000;   // 1 minute
};

// Convenience macros for load testing
#define sLoadTester BotLoadTester::Instance()

#define START_LOAD_TEST(scenario, botCount, duration) \
    sLoadTester.RunLoadTest(scenario, botCount, duration)

#define STOP_LOAD_TEST() \
    sLoadTester.StopCurrentTest()

#define GET_LOAD_TEST_METRICS() \
    sLoadTester.GetCurrentMetrics()

#define REGISTER_CUSTOM_SCENARIO(name, behavior) \
    sLoadTester.RegisterCustomScenario(name, behavior)

} // namespace Playerbot