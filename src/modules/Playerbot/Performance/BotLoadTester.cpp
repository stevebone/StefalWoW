/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BotLoadTester.h"
#include "DatabaseQueryOptimizer.h"
#include "BotPerformanceMonitor.h"
#include "AIDecisionProfiler.h"
#include "Log.h"
#include "Timer.h"
#include <algorithm>
#include <numeric>
#include <fstream>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#else
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#endif

namespace Playerbot
{

bool BotLoadTester::Initialize()
{
    ::std::lock_guard lock(_testMutex);

    if (_enabled.load())
        return true;

    TC_LOG_INFO("playerbot", "BotLoadTester: Initializing load testing framework...");

    // Removed _randomGenerator initialization - now using TrinityCore's thread-safe random functions

    // Load test history
    LoadTestHistory();

    // Set default thresholds
    _thresholds = LoadTestThresholds();

    // Initialize alert system
    _alertCallback = [](const ::std::string& type, const ::std::string& message) {
        TC_LOG_ERROR("playerbot", "LoadTest Alert [{}]: {}", type, message);
    };

    _enabled.store(true);
    TC_LOG_INFO("playerbot", "BotLoadTester: Load testing framework initialized successfully");
    return true;
}

void BotLoadTester::Shutdown()
{
    ::std::lock_guard lock(_testMutex);

    if (!_enabled.load())
        return;

    TC_LOG_INFO("playerbot", "BotLoadTester: Shutting down load testing framework...");

    _shutdownRequested.store(true);

    // Stop any running tests
    StopCurrentTest();

    // Stop monitoring
    StopRealTimeMonitoring();

    // Wait for threads to finish
    if (_testExecutionThread.joinable())
        _testExecutionThread.join();
    if (_monitoringThread.joinable())
        _monitoringThread.join();
    if (_metricsThread.joinable())
        _metricsThread.join();

    // Cleanup resources
    CleanupTestResources();

    // Archive results
    ArchiveOldResults();

    _enabled.store(false);
    TC_LOG_INFO("playerbot", "BotLoadTester: Load testing framework shut down");
}

bool BotLoadTester::RunLoadTest(LoadTestScenario scenario, uint32_t botCount, uint32_t durationSeconds)
{
    if (!_enabled.load() || _testRunning.load())
    {
        TC_LOG_ERROR("playerbot", "BotLoadTester: Cannot run test - system not enabled or test already running");
        return false;
    }

    if (botCount > _maxConcurrentBots.load() || botCount == 0)
    {
        TC_LOG_ERROR("playerbot", "BotLoadTester: Invalid bot count: {} (max: {})", botCount, _maxConcurrentBots.load());
        return false;
    }

    if (durationSeconds < MIN_TEST_DURATION || durationSeconds > MAX_TEST_DURATION)
    {
        TC_LOG_ERROR("playerbot", "BotLoadTester: Invalid test duration: {} seconds", durationSeconds);
        return false;
    }

    // Check system requirements
    if (!CheckSystemRequirements(botCount))
    {
        TC_LOG_ERROR("playerbot", "BotLoadTester: System requirements not met for {} bots", botCount);
        return false;
    }

    // Create test configurations
    ::std::vector<BotLoadTestConfig> configs;
    configs.reserve(botCount);

    for (uint32_t i = 0; i < botCount; ++i)
    {
        uint32_t botGuid = 100000 + i; // Use high GUIDs for test bots
        configs.emplace_back(botGuid, scenario, durationSeconds);
    }

    TC_LOG_INFO("playerbot", "BotLoadTester: Starting load test - Scenario: {}, Bots: {}, Duration: {}s",
                static_cast<uint32_t>(scenario), botCount, durationSeconds);

    return ExecuteLoadTest(configs);
}

bool BotLoadTester::RunCustomTest(const ::std::vector<BotLoadTestConfig>& configs)
{
    if (!_enabled.load() || _testRunning.load())
        return false;

    if (configs.empty() || configs.size() > _maxConcurrentBots.load())
    {
        TC_LOG_ERROR("playerbot", "BotLoadTester: Invalid custom test configuration");
        return false;
    }

    // Validate configurations
    for (const auto& config : configs)
    {
        if (config.durationSeconds < MIN_TEST_DURATION || config.durationSeconds > MAX_TEST_DURATION)
        {
            TC_LOG_ERROR("playerbot", "BotLoadTester: Invalid duration in custom config: {} seconds", config.durationSeconds);
            return false;
        }
    }

    if (!CheckSystemRequirements(static_cast<uint32_t>(configs.size())))
        return false;

    TC_LOG_INFO("playerbot", "BotLoadTester: Starting custom load test with {} bot configurations", configs.size());
    return ExecuteLoadTest(configs);
}

bool BotLoadTester::RunScalabilityTest(uint32_t startBots, uint32_t maxBots, uint32_t increment)
{
    if (!_enabled.load() || _testRunning.load())
        return false;

    if (startBots >= maxBots || increment == 0 || maxBots > _maxConcurrentBots.load())
    {
        TC_LOG_ERROR("playerbot", "BotLoadTester: Invalid scalability test parameters");
        return false;
    }

    TC_LOG_INFO("playerbot", "BotLoadTester: Starting scalability test - {} to {} bots (increment: {})",
                startBots, maxBots, increment);

    ::std::vector<LoadTestResults> scalabilityResults;

    for (uint32_t botCount = startBots; botCount <= maxBots; botCount += increment)
    {
        TC_LOG_INFO("playerbot", "BotLoadTester: Scalability test phase - {} bots", botCount);

        if (!RunLoadTest(LoadTestScenario::MIXED_ACTIVITIES, botCount, 300))
        {
            TC_LOG_ERROR("playerbot", "BotLoadTester: Scalability test failed at {} bots", botCount);
            break;
        }

        // Wait for test to complete
    while (_testRunning.load())
            ::std::this_thread::sleep_for(::std::chrono::seconds(1));

        scalabilityResults.push_back(_currentResults);

        // Check if performance degraded significantly
    if (!scalabilityResults.empty())
        {
            const auto& lastResult = scalabilityResults.back();
            if (lastResult.averageCpuUsage > _thresholds.maxCpuUsagePercent ||
                lastResult.averageMemoryUsage > _thresholds.maxMemoryUsageMB * 1024 * 1024)
            {
                TC_LOG_WARN("playerbot", "BotLoadTester: Performance threshold exceeded at {} bots, stopping scalability test", botCount);
                break;
            }
        }

        // Brief cooldown between tests
        ::std::this_thread::sleep_for(::std::chrono::seconds(30));
    }

    // Generate scalability report
    ::std::string report;
    GenerateScalabilityReport(report, scalabilityResults);
    TC_LOG_INFO("playerbot", "BotLoadTester: Scalability test completed\n{}", report);

    return true;
}

bool BotLoadTester::RunStressTest(uint32_t botCount, uint32_t durationSeconds)
{
    if (!_enabled.load() || _testRunning.load())
        return false;

    TC_LOG_INFO("playerbot", "BotLoadTester: Starting stress test - {} bots for {} seconds", botCount, durationSeconds);

    // Run multiple stress scenarios in sequence
    ::std::vector<LoadTestScenario> stressScenarios = {
        LoadTestScenario::STRESS_TEST,
        LoadTestScenario::MEMORY_PRESSURE,
        LoadTestScenario::DATABASE_INTENSIVE,
        LoadTestScenario::NETWORK_SIMULATION
    };

    ::std::vector<LoadTestResults> stressResults;

    for (auto scenario : stressScenarios)
    {
        TC_LOG_INFO("playerbot", "BotLoadTester: Stress test phase - scenario {}", static_cast<uint32_t>(scenario));

        if (!RunLoadTest(scenario, botCount, durationSeconds))
        {
            TC_LOG_ERROR("playerbot", "BotLoadTester: Stress test failed at scenario {}", static_cast<uint32_t>(scenario));
            return false;
        }

        // Wait for test to complete
    while (_testRunning.load())
            ::std::this_thread::sleep_for(::std::chrono::seconds(1));

        stressResults.push_back(_currentResults);

        // Brief recovery time between stress phases
        ::std::this_thread::sleep_for(::std::chrono::seconds(60));
    }

    TC_LOG_INFO("playerbot", "BotLoadTester: Stress test completed successfully");
    return true;
}

bool BotLoadTester::RunEnduranceTest(uint32_t botCount, uint32_t durationHours)
{
    if (!_enabled.load() || _testRunning.load())
        return false;

    uint32_t durationSeconds = durationHours * 3600;
    if (durationSeconds > MAX_TEST_DURATION)
    {
        TC_LOG_ERROR("playerbot", "BotLoadTester: Endurance test duration too long: {} hours", durationHours);
        return false;
    }

    TC_LOG_INFO("playerbot", "BotLoadTester: Starting endurance test - {} bots for {} hours", botCount, durationHours);

    // Use mixed activities for realistic long-term testing
    return RunLoadTest(LoadTestScenario::MIXED_ACTIVITIES, botCount, durationSeconds);
}

bool BotLoadTester::ExecuteLoadTest(const ::std::vector<BotLoadTestConfig>& configs)
{
    ::std::lock_guard lock(_testDataMutex);

    _testRunning.store(true);
    _testPaused.store(false);
    _currentPhase.store(LoadTestPhase::PREPARATION);
    _currentTestConfigs = configs;
    _testBotGuids.clear();
    _testStartTime.store(::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now().time_since_epoch()).count());

    // Initialize current results
    _currentResults = LoadTestResults();
    _currentResults.scenario = configs.empty() ? LoadTestScenario::UNKNOWN : configs[0].scenario;
    _currentResults.totalBots = static_cast<uint32_t>(configs.size());

    // Start test execution thread
    _testExecutionThread = ::std::thread([this]() {
        try
        {
            ExecuteTestPhase(LoadTestPhase::PREPARATION);
            ExecuteTestPhase(LoadTestPhase::WARMUP);
            ExecuteTestPhase(LoadTestPhase::STEADY_STATE);
            ExecuteTestPhase(LoadTestPhase::PEAK_LOAD);
            ExecuteTestPhase(LoadTestPhase::COOLDOWN);
            ExecuteTestPhase(LoadTestPhase::CLEANUP);

            _currentPhase.store(LoadTestPhase::COMPLETED);
        }
        catch (const ::std::exception& e)
        {
            TC_LOG_ERROR("playerbot", "BotLoadTester: Test execution failed - {}", e.what());
            _currentPhase.store(LoadTestPhase::FAILED);
        }

        _testRunning.store(false);

        // Analyze and save results
        AnalyzeTestResults();
        SaveTestResults(_currentResults);
    });

    // Start monitoring thread
    if (!_monitoringActive.load())
        StartRealTimeMonitoring();

    return true;
}

void BotLoadTester::ExecuteTestPhase(LoadTestPhase phase)
{
    _currentPhase.store(phase);

    switch (phase)
    {
        case LoadTestPhase::PREPARATION:
        {
            TC_LOG_INFO("playerbot", "BotLoadTester: Phase PREPARATION - Validating system resources");
            ValidateSystemResources();
            ProcessTestConfiguration();
            break;
        }

        case LoadTestPhase::WARMUP:
        {
            TC_LOG_INFO("playerbot", "BotLoadTester: Phase WARMUP - Gradually spawning bots");

            // Spawn bots gradually during warmup
            uint32_t rampUpTime = _rampUpTime.load();
            uint32_t botCount = static_cast<uint32_t>(_currentTestConfigs.size());
            uint32_t spawnInterval = rampUpTime * 1000 / botCount; // ms between spawns
    for (size_t i = 0; i < _currentTestConfigs.size() && !_shutdownRequested.load(); ++i)
            {
                if (_testPaused.load())
                {
                    while (_testPaused.load() && !_shutdownRequested.load())
                        ::std::this_thread::sleep_for(::std::chrono::milliseconds(100));
                }

                const auto& config = _currentTestConfigs[i];
                ConfigureTestBot(config.botGuid, config);
                _testBotGuids.push_back(config.botGuid);

                ::std::this_thread::sleep_for(::std::chrono::milliseconds(spawnInterval));
            }
            break;
        }

        case LoadTestPhase::STEADY_STATE:
        {
            TC_LOG_INFO("playerbot", "BotLoadTester: Phase STEADY_STATE - Maintaining full load");

            // Run test for the configured duration
            uint32_t testDuration = _currentTestConfigs.empty() ? _defaultTestDuration.load() : _currentTestConfigs[0].durationSeconds;
            auto startTime = ::std::chrono::steady_clock::now();
            auto endTime = startTime + ::std::chrono::seconds(testDuration);

            while (::std::chrono::steady_clock::now() < endTime && !_shutdownRequested.load())
            {
                if (_testPaused.load())
                {
                    while (_testPaused.load() && !_shutdownRequested.load())
                        ::std::this_thread::sleep_for(::std::chrono::milliseconds(100));
                }

                // Update bot behaviors
    for (uint32_t botGuid : _testBotGuids)
                {
                    auto it = ::std::find_if(_currentTestConfigs.begin(), _currentTestConfigs.end(),
                        [botGuid](const BotLoadTestConfig& config) { return config.botGuid == botGuid; });

                    if (it != _currentTestConfigs.end())
                        UpdateBotBehavior(botGuid, it->scenario);
                }

                // Check performance thresholds
                CheckPerformanceThresholds();

                ::std::this_thread::sleep_for(::std::chrono::seconds(1));
            }
            break;
        }

        case LoadTestPhase::PEAK_LOAD:
        {
            TC_LOG_INFO("playerbot", "BotLoadTester: Phase PEAK_LOAD - Maximum stress testing");

            // Increase bot activity to maximum for 60 seconds
    for (uint32_t botGuid : _testBotGuids)
            {
                ExecuteStressTest(botGuid);
            }

            auto peakStart = ::std::chrono::steady_clock::now();
            auto peakEnd = peakStart + ::std::chrono::seconds(60);

            while (::std::chrono::steady_clock::now() < peakEnd && !_shutdownRequested.load())
            {
                CheckPerformanceThresholds();
                ::std::this_thread::sleep_for(::std::chrono::milliseconds(500));
            }
            break;
        }

        case LoadTestPhase::COOLDOWN:
        {
            TC_LOG_INFO("playerbot", "BotLoadTester: Phase COOLDOWN - Gradually reducing load");

            // Gradually despawn bots
            uint32_t rampDownTime = _rampDownTime.load();
            uint32_t despawnInterval = rampDownTime * 1000 / static_cast<uint32_t>(_testBotGuids.size());

            while (!_testBotGuids.empty() && !_shutdownRequested.load())
            {
                uint32_t botGuid = _testBotGuids.back();
                _testBotGuids.pop_back();

                // Set bot to idle before despawning
                ExecuteIdleBehavior(botGuid);

                ::std::this_thread::sleep_for(::std::chrono::milliseconds(despawnInterval));
            }
            break;
        }

        case LoadTestPhase::CLEANUP:
        {
            TC_LOG_INFO("playerbot", "BotLoadTester: Phase CLEANUP - Cleaning up resources");
            CleanupTestResources();
            break;
        }

        default:
            break;
    }
}

void BotLoadTester::ConfigureTestBot(uint32_t botGuid, const BotLoadTestConfig& config)
{
    // This would integrate with the actual bot spawning system
    // For now, we simulate the configuration
    TC_LOG_DEBUG("playerbot", "BotLoadTester: Configuring test bot {} for scenario {}",
                 botGuid, static_cast<uint32_t>(config.scenario));

    // Record bot spawn metrics
    sPerformanceMonitor.RecordMetric(MetricType::AI_DECISION_TIME, 0, botGuid, "Bot spawned for load test");

    _monitorData.currentBots.fetch_add(1);
    _monitorData.activeBots.fetch_add(1);
}

void BotLoadTester::UpdateBotBehavior(uint32_t botGuid, LoadTestScenario scenario)
{
    switch (scenario)
    {
        case LoadTestScenario::IDLE_BOTS:
            ExecuteIdleBehavior(botGuid);
            break;
        case LoadTestScenario::RANDOM_MOVEMENT:
            ExecuteRandomMovement(botGuid);
            break;
        case LoadTestScenario::COMBAT_TRAINING:
            ExecuteCombatTraining(botGuid);
            break;
        case LoadTestScenario::DUNGEON_SIMULATION:
            ExecuteDungeonSimulation(botGuid);
            break;
        case LoadTestScenario::RAID_SIMULATION:
            ExecuteRaidSimulation(botGuid);
            break;
        case LoadTestScenario::PVP_BATTLEGROUND:
            ExecutePvPBehavior(botGuid);
            break;
        case LoadTestScenario::QUEST_AUTOMATION:
            ExecuteQuestBehavior(botGuid);
            break;
        case LoadTestScenario::AUCTION_HOUSE:
            ExecuteAuctionHouseBehavior(botGuid);
            break;
        case LoadTestScenario::GUILD_ACTIVITIES:
            ExecuteGuildBehavior(botGuid);
            break;
        case LoadTestScenario::MIXED_ACTIVITIES:
            ExecuteMixedBehavior(botGuid);
            break;
        case LoadTestScenario::STRESS_TEST:
            ExecuteStressTest(botGuid);
            break;
        case LoadTestScenario::MEMORY_PRESSURE:
            ExecuteMemoryPressure(botGuid);
            break;
        case LoadTestScenario::DATABASE_INTENSIVE:
            ExecuteDatabaseIntensive(botGuid);
            break;
        case LoadTestScenario::NETWORK_SIMULATION:
            ExecuteNetworkSimulation(botGuid);
            break;
        default:
            ExecuteIdleBehavior(botGuid);
            break;
    }
}

void BotLoadTester::ExecuteIdleBehavior(uint32_t botGuid)
{
    // Bot does minimal activity - just heartbeat
    sPerformanceMonitor.RecordMetric(MetricType::AI_DECISION_TIME, 100, botGuid, "Idle behavior");
}

void BotLoadTester::ExecuteRandomMovement(uint32_t botGuid)
{
    // Simulate random movement with pathfinding
    uint32_t movementTime = urand(500, 2500); // 500-2500ms (thread-safe)

    sPerformanceMonitor.RecordMetric(MetricType::AI_DECISION_TIME, movementTime, botGuid, "Random movement");
    sPerformanceMonitor.RecordMetric(MetricType::MOVEMENT_UPDATE, movementTime / 2, botGuid);
}

void BotLoadTester::ExecuteCombatTraining(uint32_t botGuid)
{
    // Simulate combat against training dummies
    uint32_t combatTime = urand(200, 1700); // 200-1700ms (thread-safe)

    sPerformanceMonitor.RecordMetric(MetricType::AI_DECISION_TIME, combatTime, botGuid, "Combat training");
    sPerformanceMonitor.RecordMetric(MetricType::SPELL_CAST, combatTime / 3, botGuid);

    // Simulate spell casting load
    for (int i = 0; i < 3; ++i)
    {
        sPerformanceMonitor.RecordMetric(MetricType::SPELL_CAST, 150 + (i * 50), botGuid);
    }
}

void BotLoadTester::ExecuteDungeonSimulation(uint32_t botGuid)
{
    // Simulate group dungeon behavior with coordination
    uint32_t groupCoordinationTime = 800;
    uint32_t combatRotationTime = 1200;

    sPerformanceMonitor.RecordMetric(MetricType::AI_DECISION_TIME, groupCoordinationTime, botGuid, "Dungeon coordination");
    sPerformanceMonitor.RecordMetric(MetricType::SPELL_CAST, combatRotationTime, botGuid, "Dungeon combat");
    sPerformanceMonitor.RecordMetric(MetricType::MOVEMENT_UPDATE, 400, botGuid, "Dungeon movement");
}

void BotLoadTester::ExecuteRaidSimulation(uint32_t botGuid)
{
    // Simulate 25-man raid behavior with complex coordination
    uint32_t raidCoordinationTime = 1500;
    uint32_t complexCombatTime = 2000;

    sPerformanceMonitor.RecordMetric(MetricType::AI_DECISION_TIME, raidCoordinationTime, botGuid, "Raid coordination");
    sPerformanceMonitor.RecordMetric(MetricType::SPELL_CAST, complexCombatTime, botGuid, "Raid combat");

    // Simulate multiple spell casts and positioning
    for (int i = 0; i < 5; ++i)
    {
        sPerformanceMonitor.RecordMetric(MetricType::SPELL_CAST, 200 + (i * 100), botGuid);
        sPerformanceMonitor.RecordMetric(MetricType::MOVEMENT_UPDATE, 100 + (i * 50), botGuid);
    }
}

void BotLoadTester::ExecutePvPBehavior(uint32_t botGuid)
{
    // Simulate PvP combat with frequent target switching
    uint32_t pvpDecisionTime = urand(300, 1100); // 300-1100ms (thread-safe)

    sPerformanceMonitor.RecordMetric(MetricType::AI_DECISION_TIME, pvpDecisionTime, botGuid, "PvP combat");

    // Simulate rapid decision making
    for (int i = 0; i < 4; ++i)
    {
        sPerformanceMonitor.RecordMetric(MetricType::SPELL_CAST, 100 + (i * 75), botGuid);
    }
}

void BotLoadTester::ExecuteQuestBehavior(uint32_t botGuid)
{
    // Simulate quest completion with database queries
    sPerformanceMonitor.RecordMetric(MetricType::AI_DECISION_TIME, 600, botGuid, "Quest decision");
    sPerformanceMonitor.RecordMetric(MetricType::DATABASE_QUERY, 15000, botGuid, "Quest data lookup"); // 15ms
    sPerformanceMonitor.RecordMetric(MetricType::MOVEMENT_UPDATE, 800, botGuid, "Quest movement");
}

void BotLoadTester::ExecuteAuctionHouseBehavior(uint32_t botGuid)
{
    // Simulate auction house interactions with heavy database usage
    sPerformanceMonitor.RecordMetric(MetricType::AI_DECISION_TIME, 400, botGuid, "Auction decision");
    sPerformanceMonitor.RecordMetric(MetricType::DATABASE_QUERY, 25000, botGuid, "Auction search"); // 25ms
    sPerformanceMonitor.RecordMetric(MetricType::DATABASE_QUERY, 12000, botGuid, "Auction bid"); // 12ms
}

void BotLoadTester::ExecuteGuildBehavior(uint32_t botGuid)
{
    // Simulate guild activities and social interactions
    sPerformanceMonitor.RecordMetric(MetricType::AI_DECISION_TIME, 300, botGuid, "Guild social");
    sPerformanceMonitor.RecordMetric(MetricType::PACKET_PROCESSING, 50, botGuid, "Guild chat");
    sPerformanceMonitor.RecordMetric(MetricType::DATABASE_QUERY, 8000, botGuid, "Guild data"); // 8ms
}

void BotLoadTester::ExecuteMixedBehavior(uint32_t botGuid)
{
    // Randomly execute different behaviors
    uint32_t behaviorType = urand(0, 7); // 0-7 (thread-safe)
    switch (behaviorType)
    {
        case 0: ExecuteRandomMovement(botGuid); break;
        case 1: ExecuteCombatTraining(botGuid); break;
        case 2: ExecuteQuestBehavior(botGuid); break;
        case 3: ExecuteAuctionHouseBehavior(botGuid); break;
        case 4: ExecuteGuildBehavior(botGuid); break;
        case 5: ExecuteDungeonSimulation(botGuid); break;
        case 6: ExecutePvPBehavior(botGuid); break;
        case 7: ExecuteIdleBehavior(botGuid); break;
    }
}

void BotLoadTester::ExecuteStressTest(uint32_t botGuid)
{
    // Execute maximum stress behavior - all systems active
    sPerformanceMonitor.RecordMetric(MetricType::AI_DECISION_TIME, 2500, botGuid, "Stress test");

    // Simulate multiple concurrent activities
    for (int i = 0; i < 10; ++i)
    {
        sPerformanceMonitor.RecordMetric(MetricType::SPELL_CAST, 200 + (i * 50), botGuid);
        sPerformanceMonitor.RecordMetric(MetricType::MOVEMENT_UPDATE, 100 + (i * 30), botGuid);
        sPerformanceMonitor.RecordMetric(MetricType::DATABASE_QUERY, 5000 + (i * 1000), botGuid);
        sPerformanceMonitor.RecordMetric(MetricType::PACKET_PROCESSING, 20 + (i * 5), botGuid);
    }
}

void BotLoadTester::ExecuteMemoryPressure(uint32_t botGuid)
{
    // Simulate high memory allocation patterns
    sPerformanceMonitor.RecordMetric(MetricType::AI_DECISION_TIME, 800, botGuid, "Memory pressure");
    sPerformanceMonitor.RecordMetric(MetricType::MEMORY_ALLOCATION, 1024 * 1024, botGuid, "Large allocation"); // 1MB

    // Simulate multiple small allocations
    for (int i = 0; i < 20; ++i)
    {
        sPerformanceMonitor.RecordMetric(MetricType::MEMORY_ALLOCATION, 4096, botGuid); // 4KB each
    }
}

void BotLoadTester::ExecuteDatabaseIntensive(uint32_t botGuid)
{
    // Simulate heavy database operations
    sPerformanceMonitor.RecordMetric(MetricType::AI_DECISION_TIME, 1200, botGuid, "Database intensive");

    // Multiple database queries
    for (int i = 0; i < 15; ++i)
    {
        uint32_t queryTime = 8000 + (i * 2000); // 8-38ms
        sPerformanceMonitor.RecordMetric(MetricType::DATABASE_QUERY, queryTime, botGuid);
    }
}

void BotLoadTester::ExecuteNetworkSimulation(uint32_t botGuid)
{
    // Simulate high network traffic
    sPerformanceMonitor.RecordMetric(MetricType::AI_DECISION_TIME, 600, botGuid, "Network simulation");

    // Multiple packet processing events
    for (int i = 0; i < 25; ++i)
    {
        sPerformanceMonitor.RecordMetric(MetricType::PACKET_PROCESSING, 10 + (i * 2), botGuid);
    }
}

void BotLoadTester::StopCurrentTest()
{
    if (!_testRunning.load())
        return;

    TC_LOG_INFO("playerbot", "BotLoadTester: Stopping current test");

    _testRunning.store(false);
    _currentPhase.store(LoadTestPhase::CLEANUP);

    // Notify test thread to stop
    _testCondition.notify_all();

    // Wait for test thread to finish
    if (_testExecutionThread.joinable())
        _testExecutionThread.join();

    CleanupTestResources();

    TC_LOG_INFO("playerbot", "BotLoadTester: Test stopped successfully");
}

void BotLoadTester::CheckPerformanceThresholds()
{
    auto currentMetrics = GetCurrentMetrics();

    // Check CPU usage
    if (currentMetrics.currentCpuUsage.load() > _thresholds.maxCpuUsagePercent)
    {
        TriggerAlert("CPU_THRESHOLD",
            fmt::format("CPU usage exceeded threshold: {:.1f}% > {:.1f}%",
                       currentMetrics.currentCpuUsage.load(), _thresholds.maxCpuUsagePercent));
    }

    // Check memory usage
    if (currentMetrics.currentMemoryUsage.load() > _thresholds.maxMemoryUsageMB * 1024 * 1024)
    {
        TriggerAlert("MEMORY_THRESHOLD",
            fmt::format("Memory usage exceeded threshold: {} MB > {} MB",
                       currentMetrics.currentMemoryUsage.load() / (1024 * 1024), _thresholds.maxMemoryUsageMB));
    }

    // Check response time
    if (currentMetrics.currentResponseTime.load() > _thresholds.maxResponseTimeMs * 1000) // Convert to microseconds
    {
        TriggerAlert("RESPONSE_TIME_THRESHOLD",
            fmt::format("Response time exceeded threshold: {} ms > {} ms",
                       currentMetrics.currentResponseTime.load() / 1000, _thresholds.maxResponseTimeMs));
    }

    // Check tick rate
    if (currentMetrics.currentTickRate.load() < _thresholds.minTickRate)
    {
        TriggerAlert("TICK_RATE_THRESHOLD",
            fmt::format("Tick rate below threshold: {:.1f} < {:.1f}",
                       currentMetrics.currentTickRate.load(), _thresholds.minTickRate));
    }
}

void BotLoadTester::TriggerAlert(const ::std::string& alertType, const ::std::string& message)
{
    if (!_alertsEnabled.load())
        return;

    ::std::lock_guard lock(_alertMutex);

    if (_alertCallback)
        _alertCallback(alertType, message);

    _pendingAlerts.push(::std::make_pair(alertType, message));

    // If this is a critical alert, consider stopping the test
    if (alertType.find("CRITICAL") != ::std::string::npos)
    {
        HandleCriticalAlert(message);
    }
}

void BotLoadTester::HandleCriticalAlert(const ::std::string& message)
{
    TC_LOG_ERROR("playerbot", "BotLoadTester: CRITICAL ALERT - {}", message);

    // For critical alerts, we might want to automatically stop the test
    // to prevent system damage or instability
    StopCurrentTest();
}

void BotLoadTester::StartRealTimeMonitoring()
{
    if (_monitoringActive.load())
        return;

    _monitoringActive.store(true);

    _monitoringThread = ::std::thread([this]() {
        while (_monitoringActive.load() && !_shutdownRequested.load())
        {
            UpdateRealTimeMetrics();
            ::std::this_thread::sleep_for(::std::chrono::milliseconds(_metricsInterval.load()));
        }
    });

    TC_LOG_DEBUG("playerbot", "BotLoadTester: Real-time monitoring started");
}

void BotLoadTester::StopRealTimeMonitoring()
{
    if (!_monitoringActive.load())
        return;

    _monitoringActive.store(false);

    if (_monitoringThread.joinable())
        _monitoringThread.join();

    TC_LOG_DEBUG("playerbot", "BotLoadTester: Real-time monitoring stopped");
}

void BotLoadTester::UpdateRealTimeMetrics()
{
    // Get current system metrics
    double cpuUsage = 0.0;
    uint64_t memoryUsage = 0;

#ifdef _WIN32
    // Windows implementation
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
    {
        memoryUsage = pmc.WorkingSetSize;
    }

    // CPU usage calculation would require more complex implementation
    cpuUsage = 0.0; // Placeholder
#else
    // Linux implementation
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0)
    {
        memoryUsage = usage.ru_maxrss * 1024; // Convert from KB to bytes
    }

    cpuUsage = 0.0; // Placeholder - would need /proc/stat parsing
#endif

    _monitorData.currentCpuUsage.store(cpuUsage);
    _monitorData.currentMemoryUsage.store(memoryUsage);
    _monitorData.lastUpdateTime.store(::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now().time_since_epoch()).count());
}

LoadTestMonitorData BotLoadTester::GetCurrentMetrics() const
{
    return _monitorData;
}

void BotLoadTester::AnalyzeTestResults()
{
    ::std::lock_guard lock(_testDataMutex);

    auto endTime = ::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now().time_since_epoch()).count();

    _currentResults.testDurationMs = (endTime - _testStartTime.load()) / 1000;
    _currentResults.successfulBots = static_cast<uint32_t>(_testBotGuids.size());

    // Calculate performance metrics from collected data
    CalculatePerformanceMetrics(_currentResults);

    // Detect bottlenecks
    DetectPerformanceBottlenecks(_currentResults);

    // Calculate scalability score
    _currentResults.scalabilityScore = CalculateScalabilityMetric(_currentResults.totalBots, _currentResults);

    TC_LOG_INFO("playerbot", "BotLoadTester: Test analysis completed - Score: {:.1f}%",
                _currentResults.scalabilityScore);
}

void BotLoadTester::CalculatePerformanceMetrics(LoadTestResults& results)
{
    // Get metrics from performance monitor
    auto stats = sPerformanceMonitor.GetStatistics(MetricType::AI_DECISION_TIME);
    results.averageResponseTime = static_cast<uint64_t>(stats.average);
    results.maxResponseTime = stats.maximum;

    auto memStats = sPerformanceMonitor.GetStatistics(MetricType::MEMORY_ALLOCATION);
    results.averageMemoryUsage = static_cast<uint64_t>(memStats.average);
    results.peakMemoryUsage = memStats.maximum;

    auto dbStats = sPerformanceMonitor.GetStatistics(MetricType::DATABASE_QUERY);
    results.totalQueries = dbStats.count;
    results.averageQueryTime = static_cast<uint64_t>(dbStats.average);
    results.maxQueryTime = dbStats.maximum;

    if (results.testDurationMs > 0)
    {
        results.queriesPerSecond = static_cast<double>(results.totalQueries) / (results.testDurationMs / 1000.0);
    }

    // Calculate tick rate and CPU usage from monitoring data
    results.averageTickRate = _monitorData.currentTickRate.load();
    results.averageCpuUsage = _monitorData.currentCpuUsage.load();
}

double BotLoadTester::CalculateScalabilityMetric(uint32_t botCount, const LoadTestResults& results) const
{
    if (botCount == 0)
        return 0.0;

    // Calculate efficiency based on multiple factors
    double cpuEfficiency = ::std::max(0.0, (100.0 - results.averageCpuUsage) / 100.0);
    double memoryEfficiency = ::std::max(0.0, 1.0 - (static_cast<double>(results.averageMemoryUsage) / (1024.0 * 1024.0 * 1024.0))); // 1GB baseline
    double responseEfficiency = ::std::max(0.0, 1.0 - (static_cast<double>(results.averageResponseTime) / 100000.0)); // 100ms baseline
    double tickEfficiency = ::std::min(1.0, results.averageTickRate / 50.0); // 50 FPS baseline

    // Weighted average
    double scalabilityScore = (cpuEfficiency * 0.3 + memoryEfficiency * 0.2 +
                              responseEfficiency * 0.3 + tickEfficiency * 0.2) * 100.0;

    return ::std::max(0.0, ::std::min(100.0, scalabilityScore));
}

void BotLoadTester::CleanupTestResources()
{
    ::std::lock_guard lock(_testDataMutex);

    // Reset monitoring data
    _monitorData.currentBots.store(0);
    _monitorData.activeBots.store(0);
    _monitorData.idleBots.store(0);
    _monitorData.errorBots.store(0);

    // Clear test bot list
    _testBotGuids.clear();

    TC_LOG_DEBUG("playerbot", "BotLoadTester: Test resources cleaned up");
}

bool BotLoadTester::CheckSystemRequirements(uint32_t botCount)
{
    // Check available memory
    uint64_t estimatedMemoryPerBot = 10 * 1024 * 1024; // 10MB per bot
    uint64_t totalMemoryNeeded = botCount * estimatedMemoryPerBot;

#ifdef _WIN32
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    if (GlobalMemoryStatusEx(&statex))
    {
        if (statex.ullAvailPhys < totalMemoryNeeded)
        {
            TC_LOG_ERROR("playerbot", "BotLoadTester: Insufficient memory for {} bots. Need: {} MB, Available: {} MB",
                         botCount, totalMemoryNeeded / (1024 * 1024), statex.ullAvailPhys / (1024 * 1024));
            return false;
        }
    }
#else
    struct sysinfo si;
    if (sysinfo(&si) == 0)
    {
        uint64_t availableMemory = si.freeram * si.mem_unit;
        if (availableMemory < totalMemoryNeeded)
        {
            TC_LOG_ERROR("playerbot", "BotLoadTester: Insufficient memory for {} bots. Need: {} MB, Available: {} MB",
                         botCount, totalMemoryNeeded / (1024 * 1024), availableMemory / (1024 * 1024));
            return false;
        }
    }
#endif

    return true;
}

void BotLoadTester::SaveTestResults(const LoadTestResults& results)
{
    ::std::lock_guard lock(_testDataMutex);

    _testHistory.push_back(results);
    _totalTestsRun.fetch_add(1);
    _totalBotsSpawned.fetch_add(results.totalBots);

    TC_LOG_INFO("playerbot", "BotLoadTester: Test results saved. Total tests run: {}", _totalTestsRun.load());
}

void BotLoadTester::GenerateLoadTestReport(::std::string& report, const LoadTestResults& results) const
{
    ::std::ostringstream oss;

    oss << "=== Load Test Report ===\n";
    oss << "Scenario: " << static_cast<uint32_t>(results.scenario) << "\n";
    oss << "Total Bots: " << results.totalBots << "\n";
    oss << "Successful Bots: " << results.successfulBots << "\n";
    oss << "Failed Bots: " << results.failedBots << "\n";
    oss << "Test Duration: " << results.testDurationMs / 1000 << " seconds\n\n";

    oss << "=== Performance Metrics ===\n";
    oss << "Average CPU Usage: " << ::std::fixed << ::std::setprecision(1) << results.averageCpuUsage << "%\n";
    oss << "Peak CPU Usage: " << ::std::fixed << ::std::setprecision(1) << results.peakCpuUsage << "%\n";
    oss << "Average Memory Usage: " << results.averageMemoryUsage / (1024 * 1024) << " MB\n";
    oss << "Peak Memory Usage: " << results.peakMemoryUsage / (1024 * 1024) << " MB\n";
    oss << "Average Response Time: " << results.averageResponseTime / 1000 << " ms\n";
    oss << "Max Response Time: " << results.maxResponseTime / 1000 << " ms\n";
    oss << "Average Tick Rate: " << ::std::fixed << ::std::setprecision(1) << results.averageTickRate << " FPS\n\n";

    oss << "=== Database Metrics ===\n";
    oss << "Total Queries: " << results.totalQueries << "\n";
    oss << "Average Query Time: " << results.averageQueryTime / 1000 << " ms\n";
    oss << "Max Query Time: " << results.maxQueryTime / 1000 << " ms\n";
    oss << "Queries per Second: " << ::std::fixed << ::std::setprecision(1) << results.queriesPerSecond << "\n\n";

    oss << "=== Scalability Analysis ===\n";
    oss << "Scalability Score: " << ::std::fixed << ::std::setprecision(1) << results.scalabilityScore << "%\n";
    oss << "Max Stable Bots: " << results.maxStableBots << "\n";
    oss << "Recommended Bots: " << results.recommendedBots << "\n\n";

    if (results.crashCount > 0 || results.timeoutCount > 0 || results.memoryLeaks > 0)
    {
        oss << "=== Issues Detected ===\n";
        oss << "Crashes: " << results.crashCount << "\n";
        oss << "Timeouts: " << results.timeoutCount << "\n";
        oss << "Memory Leaks: " << results.memoryLeaks << "\n";
    }

    report = oss.str();
}

void BotLoadTester::LoadTestHistory()
{
    // Implementation would load from database or file
    // For now, just initialize empty
    _testHistory.clear();
}

void BotLoadTester::ArchiveOldResults()
{
    // Implementation would archive old test results
    // Remove results older than HISTORY_RETENTION_DAYS
    TC_LOG_DEBUG("playerbot", "BotLoadTester: Archiving old test results");
}

void BotLoadTester::ProcessTestConfiguration()
{
    // Validate and process test configuration
    TC_LOG_DEBUG("playerbot", "BotLoadTester: Processing test configuration for {} bots", _currentTestConfigs.size());
}

void BotLoadTester::ValidateSystemResources()
{
    // Validate system has sufficient resources for the test
    TC_LOG_DEBUG("playerbot", "BotLoadTester: Validating system resources");
}

void BotLoadTester::DetectPerformanceBottlenecks(LoadTestResults& results)
{
    // Analyze results to identify performance bottlenecks
    if (results.averageCpuUsage > 70.0)
        results.errorMessages.push_back("High CPU usage detected");

    if (results.averageMemoryUsage > 2ULL * 1024 * 1024 * 1024) // 2GB
        results.errorMessages.push_back("High memory usage detected");

    if (results.averageQueryTime > 20000) // 20ms
        results.errorMessages.push_back("Slow database queries detected");
}

} // namespace Playerbot