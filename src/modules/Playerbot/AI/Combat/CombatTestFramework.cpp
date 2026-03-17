/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "CombatTestFramework.h"
#include "Player.h"
#include "Creature.h"
#include "GameObject.h"
#include "Group.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "Log.h"
#include "Random.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace Playerbot
{

CombatTestFramework::CombatTestFramework()
    : _initialized(false), _debugMode(false), _nextTestId(1),
      _defaultDurationMs(DEFAULT_TEST_DURATION), _maxConcurrentTests(MAX_CONCURRENT_TESTS),
      _monitoringIntervalMs(MONITORING_INTERVAL), _lastMonitoringUpdate(0),
      _performanceMonitoring(false)
{
}

CombatTestFramework::~CombatTestFramework()
{
    Shutdown();
}

bool CombatTestFramework::Initialize()
{
    if (_initialized)
        return true;

    TC_LOG_INFO("playerbot", "CombatTestFramework: Initializing combat testing framework");

    // Initialize performance monitoring
    _performanceMonitoring = false;
    _lastMonitoringUpdate = 0;

    // Clear any existing state
    _scenarios.clear();
    _registeredSystems.clear();
    _globalMetrics.clear();
    _testHistory.clear();
    _testLog.clear();

    // Reset test context
    _currentContext.reset();

    _initialized = true;
    TC_LOG_INFO("playerbot", "CombatTestFramework: Initialization complete");
    return true;
}

void CombatTestFramework::Shutdown()
{
    if (!_initialized)
        return;

    TC_LOG_INFO("playerbot", "CombatTestFramework: Shutting down testing framework");

    // Stop any running scenario
    StopCurrentScenario();

    // Clear all registered systems
    _registeredSystems.clear();

    // Clear metrics and history
    _globalMetrics.clear();
    _testHistory.clear();
    _testLog.clear();

    // Clear scenarios
    _scenarios.clear();

    _initialized = false;
    TC_LOG_INFO("playerbot", "CombatTestFramework: Shutdown complete");
}

void CombatTestFramework::Update(uint32 diff)
{
    if (!_initialized)
        return;

    // Update current scenario if running
    if (_currentContext && _currentContext->isRunning && !_currentContext->isPaused)
    {
        ExecuteScenarioUpdate(*_currentContext, diff);
    }

    // Update performance monitoring
    if (_performanceMonitoring)
    {
        _lastMonitoringUpdate += diff;
        if (_lastMonitoringUpdate >= _monitoringIntervalMs)
        {
            if (_currentContext)
                MonitorCombatSystems(*_currentContext, diff);
            _lastMonitoringUpdate = 0;
        }
    }
}

bool CombatTestFramework::LoadScenario(const ::std::string& scenarioFile)
{
    ::std::ifstream file(scenarioFile);
    if (!file.is_open())
    {
        TC_LOG_ERROR("playerbot", "CombatTestFramework: Failed to open scenario file: {}", scenarioFile);
        return false;
    }

    // Simple JSON-like parsing for scenario files
    // In a real implementation, you'd use a proper JSON parser
    ::std::string line;
    TestScenario scenario;

    while (::std::getline(file, line))
    {
        // Parse basic scenario properties
    if (line.find("\"name\":") != ::std::string::npos)
        {
            size_t start = line.find("\"", line.find(":") + 1) + 1;
            size_t end = line.find("\"", start);
            scenario.name = line.substr(start, end - start);
        }
        else if (line.find("\"description\":") != ::std::string::npos)
        {
            size_t start = line.find("\"", line.find(":") + 1) + 1;
            size_t end = line.find("\"", start);
            scenario.description = line.substr(start, end - start);
        }
        // Add more parsing logic as needed
    }

    if (ValidateScenario(scenario))
    {
        _scenarios[scenario.name] = scenario;
        TC_LOG_INFO("playerbot", "CombatTestFramework: Loaded scenario '{}'", scenario.name);
        return true;
    }

    TC_LOG_ERROR("playerbot", "CombatTestFramework: Invalid scenario in file: {}", scenarioFile);
    return false;
}

bool CombatTestFramework::CreateScenario(const TestScenario& scenario)
{
    if (!ValidateScenario(scenario))
    {
        TC_LOG_ERROR("playerbot", "CombatTestFramework: Invalid scenario: {}", scenario.name);
        return false;
    }

    _scenarios[scenario.name] = scenario;
    TC_LOG_INFO("playerbot", "CombatTestFramework: Created scenario '{}'", scenario.name);
    return true;
}

::std::vector<::std::string> CombatTestFramework::GetAvailableScenarios() const
{
    ::std::vector<::std::string> scenarioNames;
    scenarioNames.reserve(_scenarios.size());

    for (const auto& [name, scenario] : _scenarios)
        scenarioNames.push_back(name);

    return scenarioNames;
}

TestScenario* CombatTestFramework::GetScenario(const ::std::string& name)
{
    auto it = _scenarios.find(name);
    return it != _scenarios.end() ? &it->second : nullptr;
}

bool CombatTestFramework::SaveScenario(const TestScenario& scenario, const ::std::string& filename)
{
    ::std::ofstream file(filename);
    if (!file.is_open())
    {
        TC_LOG_ERROR("playerbot", "CombatTestFramework: Failed to create scenario file: {}", filename);
        return false;
    }

    // Simple JSON-like format
    file << "{\n";
    file << "  \"name\": \"" << scenario.name << "\",\n";
    file << "  \"description\": \"" << scenario.description << "\",\n";
    file << "  \"type\": " << static_cast<uint32>(scenario.type) << ",\n";
    file << "  \"environment\": " << static_cast<uint32>(scenario.environment) << ",\n";
    file << "  \"duration\": " << scenario.durationMs << ",\n";
    file << "  \"maxParticipants\": " << scenario.maxParticipants << ",\n";
    file << "  \"centerPosition\": {\n";
    file << "    \"x\": " << scenario.centerPosition.GetPositionX() << ",\n";
    file << "    \"y\": " << scenario.centerPosition.GetPositionY() << ",\n";
    file << "    \"z\": " << scenario.centerPosition.GetPositionZ() << "\n";
    file << "  },\n";
    file << "  \"arenaRadius\": " << scenario.arenaRadius << "\n";
    file << "}\n";

    TC_LOG_INFO("playerbot", "CombatTestFramework: Saved scenario '{}' to {}", scenario.name, filename);
    return true;
}

TestResult CombatTestFramework::ExecuteScenario(const ::std::string& scenarioName)
{
    auto it = _scenarios.find(scenarioName);
    if (it == _scenarios.end())
    {
        TestResult result;
        result.scenarioName = scenarioName;
        result.success = false;
        result.failures.push_back("Scenario not found: " + scenarioName);
        return result;
    }

    return ExecuteScenario(it->second);
}

TestResult CombatTestFramework::ExecuteScenario(const TestScenario& scenario)
{
    TestResult result;
    result.scenarioName = scenario.name;
    result.startTime = ::std::chrono::steady_clock::now();

    LogTestEvent("Starting scenario: " + scenario.name);

    // Stop any running scenario first
    StopCurrentScenario();

    // Create new test context
    _currentContext = ::std::make_unique<TestContext>();
    _currentContext->scenario = scenario;

    // Initialize the scenario
    if (!InitializeScenario(scenario, *_currentContext))
    {
        result.success = false;
        result.failures.push_back("Failed to initialize scenario");
        result.endTime = ::std::chrono::steady_clock::now();
        result.executionTimeMs = static_cast<uint32>(
            ::std::chrono::duration_cast<::std::chrono::milliseconds>(result.endTime - result.startTime).count());
        return result;
    }

    // Start performance monitoring
    if (!_performanceMonitoring)
        StartPerformanceMonitoring();

    // Run the scenario
    _currentContext->isRunning = true;
    _currentContext->currentTimeMs = 0;

    // Main execution loop (simplified - in real implementation this would be event-driven)
    uint32 updateInterval = 100; // 100ms updates
    while (_currentContext->isRunning && _currentContext->currentTimeMs < scenario.durationMs)
    {
        if (!_currentContext->isPaused)
        {
            ExecuteScenarioUpdate(*_currentContext, updateInterval);
            _currentContext->currentTimeMs += updateInterval;

            // Check success criteria
    if (CheckSuccessCriteria(scenario, *_currentContext))
            {
                LogTestEvent("Success criteria met early");
                break;
            }
        }

        // In a real implementation, this would be handled by the server update loop
        ::std::this_thread::sleep_for(::std::chrono::milliseconds(updateInterval));
    }

    // Finalize the scenario
    FinalizeScenario(*_currentContext, result);

    result.endTime = ::std::chrono::steady_clock::now();
    result.executionTimeMs = static_cast<uint32>(
        ::std::chrono::duration_cast<::std::chrono::milliseconds>(result.endTime - result.startTime).count());

    // Store in history
    {
        ::std::lock_guard lock(_resultMutex);
        _scenarioHistory[scenario.name].push_back(result);
        if (_scenarioHistory[scenario.name].size() > MAX_HISTORY_ENTRIES)
            _scenarioHistory[scenario.name].erase(_scenarioHistory[scenario.name].begin());
    }

    LogTestEvent("Scenario completed: " + scenario.name + " (Success: " + (result.success ? "Yes" : "No") + ")");

    return result;
}

bool CombatTestFramework::StartScenario(const ::std::string& scenarioName)
{
    auto it = _scenarios.find(scenarioName);
    if (it == _scenarios.end())
    {
        TC_LOG_ERROR("playerbot", "CombatTestFramework: Scenario not found: {}", scenarioName);
        return false;
    }

    // Stop any running scenario first
    StopCurrentScenario();

    // Create new test context
    _currentContext = ::std::make_unique<TestContext>();
    _currentContext->scenario = it->second;

    // Initialize the scenario
    if (!InitializeScenario(it->second, *_currentContext))
    {
        TC_LOG_ERROR("playerbot", "CombatTestFramework: Failed to initialize scenario: {}", scenarioName);
        _currentContext.reset();
        return false;
    }

    // Start the scenario
    _currentContext->isRunning = true;
    _currentContext->currentTimeMs = 0;

    if (!_performanceMonitoring)
        StartPerformanceMonitoring();

    LogTestEvent("Started scenario: " + scenarioName);
    return true;
}

void CombatTestFramework::StopCurrentScenario()
{
    if (!_currentContext)
        return;

    if (_currentContext->isRunning)
    {
        LogTestEvent("Stopping scenario: " + _currentContext->scenario.name);
        _currentContext->isRunning = false;
    }

    // Cleanup
    CleanupScenario(*_currentContext);
    _currentContext.reset();
}

void CombatTestFramework::PauseCurrentScenario()
{
    if (_currentContext && _currentContext->isRunning)
    {
        _currentContext->isPaused = true;
        LogTestEvent("Paused scenario: " + _currentContext->scenario.name);
    }
}

void CombatTestFramework::ResumeCurrentScenario()
{
    if (_currentContext && _currentContext->isRunning)
    {
        _currentContext->isPaused = false;
        LogTestEvent("Resumed scenario: " + _currentContext->scenario.name);
    }
}

bool CombatTestFramework::AddParticipant(const TestParticipant& participant)
{
    if (!_currentContext)
        return false;

    if (!ValidateParticipant(participant))
        return false;

    _currentContext->scenario.participants.push_back(participant);
    LogTestEvent("Added participant: " + participant.name);
    return true;
}

bool CombatTestFramework::RemoveParticipant(ObjectGuid guid)
{
    if (!_currentContext)
        return false;

    auto& participants = _currentContext->scenario.participants;
    auto it = ::std::find_if(participants.begin(), participants.end(),
        [guid](const TestParticipant& p) { return p.guid == guid; });

    if (it != participants.end())
    {
        RemoveParticipantFromWorld(*it);
        participants.erase(it);
        LogTestEvent("Removed participant: " + guid.ToString());
        return true;
    }

    return false;
}

TestParticipant* CombatTestFramework::GetParticipant(ObjectGuid guid)
{
    if (!_currentContext)
        return nullptr;

    auto& participants = _currentContext->scenario.participants;
    auto it = ::std::find_if(participants.begin(), participants.end(),
        [guid](const TestParticipant& p) { return p.guid == guid; });

    return it != participants.end() ? &(*it) : nullptr;
}

::std::vector<TestParticipant*> CombatTestFramework::GetParticipantsByRole(TestRole role)
{
    ::std::vector<TestParticipant*> result;
    if (!_currentContext)
        return result;

    for (auto& participant : _currentContext->scenario.participants)
    {
        if (participant.role == role)
            result.push_back(&participant);
    }

    return result;
}

void CombatTestFramework::ClearParticipants()
{
    if (!_currentContext)
        return;

    for (const auto& participant : _currentContext->scenario.participants)
        RemoveParticipantFromWorld(participant);

    _currentContext->scenario.participants.clear();
    LogTestEvent("Cleared all participants");
}

bool CombatTestFramework::SetupTestEnvironment(TestEnvironment environment, const Position& center, float radius)
{
    if (!ValidateEnvironment(environment, center, radius))
        return false;

    if (!CreateTestArea(environment, center, radius))
        return false;

    LogTestEvent("Setup test environment: " + ::std::to_string(static_cast<uint32>(environment)));
    return true;
}

bool CombatTestFramework::AddObstacle(const TestObstacle& obstacle)
{
    if (!_currentContext)
        return false;

    _currentContext->scenario.obstacles.push_back(obstacle);
    LogTestEvent("Added obstacle: " + obstacle.name);
    return true;
}

bool CombatTestFramework::RemoveObstacle(ObjectGuid guid)
{
    if (!_currentContext)
        return false;

    auto& obstacles = _currentContext->scenario.obstacles;
    auto it = ::std::find_if(obstacles.begin(), obstacles.end(),
        [guid](const TestObstacle& o) { return o.guid == guid; });

    if (it != obstacles.end())
    {
        obstacles.erase(it);
        LogTestEvent("Removed obstacle: " + guid.ToString());
        return true;
    }

    return false;
}

void CombatTestFramework::ClearObstacles()
{
    if (!_currentContext)
        return;

    _currentContext->scenario.obstacles.clear();
    LogTestEvent("Cleared all obstacles");
}

bool CombatTestFramework::SpawnTestCreatures(const ::std::vector<TestParticipant>& enemies)
{
    if (!_currentContext)
        return false;

    for (const auto& enemy : enemies)
    {
        if (!SpawnEnemy(enemy, *_currentContext))
        {
            TC_LOG_ERROR("playerbot", "CombatTestFramework: Failed to spawn enemy: {}", enemy.name);
            return false;
        }
    }

    LogTestEvent("Spawned " + ::std::to_string(enemies.size()) + " test creatures");
    return true;
}

void CombatTestFramework::RegisterCombatSystem(const ::std::string& name, void* system)
{
    _registeredSystems[name] = system;

    // Initialize metrics for this system
    CombatSystemMetrics metrics;
    metrics.systemName = name;
    metrics.Reset();
    _globalMetrics[name] = metrics;

    LogTestEvent("Registered combat system: " + name);
}

void CombatTestFramework::UnregisterCombatSystem(const ::std::string& name)
{
    _registeredSystems.erase(name);
    _globalMetrics.erase(name);
    LogTestEvent("Unregistered combat system: " + name);
}

bool CombatTestFramework::IsCombatSystemRegistered(const ::std::string& name) const
{
    return _registeredSystems.find(name) != _registeredSystems.end();
}

CombatSystemMetrics* CombatTestFramework::GetSystemMetrics(const ::std::string& name)
{
    auto it = _globalMetrics.find(name);
    return it != _globalMetrics.end() ? &it->second : nullptr;
}

void CombatTestFramework::StartPerformanceMonitoring()
{
    _performanceMonitoring = true;
    _lastMonitoringUpdate = 0;

    // Reset all metrics
    for (auto& [name, metrics] : _globalMetrics)
        metrics.Reset();

    LogTestEvent("Started performance monitoring");
}

void CombatTestFramework::StopPerformanceMonitoring()
{
    _performanceMonitoring = false;
    LogTestEvent("Stopped performance monitoring");
}

void CombatTestFramework::ResetMetrics()
{
    for (auto& [name, metrics] : _globalMetrics)
        metrics.Reset();

    LogTestEvent("Reset all metrics");
}

::std::unordered_map<::std::string, CombatSystemMetrics> CombatTestFramework::GetAllMetrics() const
{
    return _globalMetrics;
}

float CombatTestFramework::CalculateOverallPerformanceScore() const
{
    if (_globalMetrics.empty())
        return 0.0f;

    float totalScore = 0.0f;
    uint32 systemCount = 0;

    for (const auto& [name, metrics] : _globalMetrics)
    {
        float systemScore = metrics.GetSuccessRate() * 100.0f;

        // Penalize for high execution times
    if (metrics.averageExecutionTime.count() > 1000) // > 1ms
            systemScore *= 0.8f;

        totalScore += systemScore;
        ++systemCount;
    }

    return systemCount > 0 ? totalScore / systemCount : 0.0f;
}

bool CombatTestFramework::ValidateScenario(const TestScenario& scenario) const
{
    if (scenario.name.empty())
        return false;

    if (scenario.participants.size() > scenario.maxParticipants)
        return false;

    if (scenario.arenaRadius < MIN_ARENA_RADIUS || scenario.arenaRadius > MAX_ARENA_RADIUS)
        return false;

    if (scenario.durationMs == 0)
        return false;

    return true;
}

::std::vector<::std::string> CombatTestFramework::GetScenarioValidationErrors(const TestScenario& scenario) const
{
    ::std::vector<::std::string> errors;

    if (scenario.name.empty())
        errors.push_back("Scenario name cannot be empty");

    if (scenario.participants.size() > scenario.maxParticipants)
        errors.push_back("Too many participants (" + ::std::to_string(scenario.participants.size()) +
                        " > " + ::std::to_string(scenario.maxParticipants) + ")");

    if (scenario.arenaRadius < MIN_ARENA_RADIUS)
        errors.push_back("Arena radius too small (minimum: " + ::std::to_string(MIN_ARENA_RADIUS) + ")");

    if (scenario.arenaRadius > MAX_ARENA_RADIUS)
        errors.push_back("Arena radius too large (maximum: " + ::std::to_string(MAX_ARENA_RADIUS) + ")");

    if (scenario.durationMs == 0)
        errors.push_back("Test duration cannot be zero");

    return errors;
}

bool CombatTestFramework::ValidateTestResults(const TestResult& result) const
{
    if (result.scenarioName.empty())
        return false;

    if (result.overallScore < 0.0f || result.overallScore > 100.0f)
        return false;

    return true;
}

float CombatTestFramework::EvaluateCriteria(TestCriteria criteria, const TestContext& context) const
{
    switch (criteria)
    {
        case TestCriteria::SURVIVAL:
            return EvaluateSurvivalCriteria(context);
        case TestCriteria::TIME_LIMIT:
            return EvaluateTimeLimitCriteria(context);
        case TestCriteria::DAMAGE_DEALT:
            return EvaluateDamageDealtCriteria(context);
        case TestCriteria::POSITIONING_ACCURACY:
            return EvaluatePositioningAccuracy(context);
        case TestCriteria::FORMATION_INTEGRITY:
            return EvaluateFormationIntegrity(context);
        case TestCriteria::INTERRUPT_SUCCESS:
            return EvaluateInterruptSuccess(context);
        case TestCriteria::THREAT_MANAGEMENT:
            return EvaluateThreatManagement(context);
        case TestCriteria::COORDINATION:
            return EvaluateCoordination(context);
        default:
            return 0.0f;
    }
}

bool CombatTestFramework::CheckSuccessCriteria(const TestScenario& scenario, const TestContext& context) const
{
    for (TestCriteria criteria : scenario.successCriteria)
    {
        float score = EvaluateCriteria(criteria, context);
        if (score < 80.0f) // Require 80% success for each criteria
            return false;
    }
    return true;
}

::std::unordered_map<TestCriteria, float> CombatTestFramework::EvaluateAllCriteria(const TestContext& context) const
{
    ::std::unordered_map<TestCriteria, float> results;

    for (TestCriteria criteria : context.scenario.successCriteria)
        results[criteria] = EvaluateCriteria(criteria, context);

    return results;
}

::std::string CombatTestFramework::GenerateDetailedReport(const TestResult& result) const
{
    ::std::ostringstream report;

    report << "=== Combat Test Framework - Detailed Report ===\n";
    report << "Scenario: " << result.scenarioName << "\n";
    report << "Success: " << (result.success ? "YES" : "NO") << "\n";
    report << "Overall Score: " << ::std::fixed << ::std::setprecision(2) << result.overallScore << "%\n";
    report << "Execution Time: " << result.executionTimeMs << "ms\n";
    report << "Start Time: " << ::std::chrono::duration_cast<::std::chrono::milliseconds>(
        result.startTime.time_since_epoch()).count() << "\n";
    report << "End Time: " << ::std::chrono::duration_cast<::std::chrono::milliseconds>(
        result.endTime.time_since_epoch()).count() << "\n\n";

    report << "=== Criteria Scores ===\n";
    for (const auto& [criteria, score] : result.criteriaScores)
    {
        report << "- " << static_cast<uint32>(criteria) << ": "
               << ::std::fixed << ::std::setprecision(2) << score << "%\n";
    }
    report << "\n";

    report << "=== System Performance ===\n";
    for (const auto& [system, performance] : result.systemPerformance)
    {
        report << "- " << system << ": "
               << ::std::fixed << ::std::setprecision(2) << performance << "%\n";
    }
    report << "\n";

    if (!result.failures.empty())
    {
        report << "=== Failures ===\n";
        for (const auto& failure : result.failures)
            report << "- " << failure << "\n";
        report << "\n";
    }

    if (!result.warnings.empty())
    {
        report << "=== Warnings ===\n";
        for (const auto& warning : result.warnings)
            report << "- " << warning << "\n";
        report << "\n";
    }

    if (!result.detailedLog.empty())
    {
        report << "=== Detailed Log ===\n";
        report << result.detailedLog << "\n";
    }

    return report.str();
}

::std::string CombatTestFramework::GeneratePerformanceReport() const
{
    ::std::ostringstream report;

    report << "=== Combat Test Framework - Performance Report ===\n";
    report << "Overall Score: " << ::std::fixed << ::std::setprecision(2)
           << CalculateOverallPerformanceScore() << "%\n\n";

    report << "=== System Metrics ===\n";
    for (const auto& [name, metrics] : _globalMetrics)
    {
        report << "System: " << name << "\n";
        report << "  Update Calls: " << metrics.updateCalls.load() << "\n";
        report << "  Success Rate: " << ::std::fixed << ::std::setprecision(2)
               << metrics.GetSuccessRate() * 100.0f << "%\n";
        report << "  Avg Execution Time: " << metrics.averageExecutionTime.count() << "μs\n";
        report << "  Min Execution Time: " << metrics.minExecutionTime.count() << "μs\n";
        report << "  Max Execution Time: " << metrics.maxExecutionTime.count() << "μs\n";
        report << "  Memory Usage: " << metrics.memoryUsage.load() << " bytes\n\n";
    }

    return report.str();
}

bool CombatTestFramework::SaveTestResults(const TestResult& result, const ::std::string& filename) const
{
    ::std::ofstream file(filename);
    if (!file.is_open())
        return false;

    file << GenerateDetailedReport(result);
    return true;
}

::std::vector<TestResult> CombatTestFramework::LoadTestHistory(const ::std::string& scenarioName) const
{
    ::std::lock_guard lock(_resultMutex);
    auto it = _scenarioHistory.find(scenarioName);
    return it != _scenarioHistory.end() ? it->second : ::std::vector<TestResult>();
}

void CombatTestFramework::LogTestEvent(const ::std::string& event, const ::std::string& details)
{
    ::std::lock_guard lock(_logMutex);

    auto now = ::std::chrono::steady_clock::now();
    auto timestamp = ::std::chrono::duration_cast<::std::chrono::milliseconds>(now.time_since_epoch()).count();

    ::std::string logEntry = "[" + ::std::to_string(timestamp) + "] " + event;
    if (!details.empty())
        logEntry += " - " + details;

    _testLog.push_back(logEntry);

    // Limit log size
    if (_testLog.size() > MAX_TEST_LOG_ENTRIES)
        _testLog.erase(_testLog.begin());

    if (_debugMode)
        TC_LOG_DEBUG("playerbot", "CombatTestFramework: {}", logEntry);
}

TestScenario CombatTestFramework::CreateBasicCombatScenario(uint32 botCount, uint32 enemyCount)
{
    return ScenarioBuilder("Basic Combat " + ::std::to_string(botCount) + "v" + ::std::to_string(enemyCount))
        .SetType(TestScenarioType::BASIC_COMBAT)
        .SetEnvironment(TestEnvironment::OPEN_FIELD)
        .SetDuration(60000)
        .SetArena(Position(0, 0, 0), 50.0f)
        .RequireSurvival()
        .RequireTimeLimit(60000)
        .Build();
}

TestScenario CombatTestFramework::CreateGroupFormationScenario(uint32 groupSize)
{
    return ScenarioBuilder("Group Formation " + ::std::to_string(groupSize))
        .SetType(TestScenarioType::FORMATION_TEST)
        .SetEnvironment(TestEnvironment::OPEN_FIELD)
        .SetDuration(30000)
        .SetArena(Position(0, 0, 0), 30.0f)
        .RequireFormationIntegrity(80.0f)
        .RequireCoordination(70.0f)
        .Build();
}

TestScenario CombatTestFramework::CreateKitingScenario(TestRole kitingRole)
{
    return ScenarioBuilder("Kiting Test")
        .SetType(TestScenarioType::KITING_TEST)
        .SetEnvironment(TestEnvironment::OPEN_FIELD)
        .SetDuration(45000)
        .SetArena(Position(0, 0, 0), 40.0f)
        .RequirePositioning(75.0f)
        .RequireSurvival()
        .Build();
}

TestScenario CombatTestFramework::CreateInterruptScenario(uint32 casterCount)
{
    return ScenarioBuilder("Interrupt Test " + ::std::to_string(casterCount))
        .SetType(TestScenarioType::INTERRUPT_TEST)
        .SetEnvironment(TestEnvironment::OPEN_FIELD)
        .SetDuration(40000)
        .SetArena(Position(0, 0, 0), 35.0f)
        .RequireInterruptSuccess(70.0f)
        .RequireCoordination(60.0f)
        .Build();
}

TestScenario CombatTestFramework::CreatePositioningScenario(TestEnvironment environment)
{
    return ScenarioBuilder("Positioning Test")
        .SetType(TestScenarioType::POSITIONING_TEST)
        .SetEnvironment(environment)
        .SetDuration(50000)
        .SetArena(Position(0, 0, 0), 45.0f)
        .RequirePositioning(85.0f)
        .RequireSurvival()
        .Build();
}

TestScenario CombatTestFramework::CreatePathfindingScenario(uint32 obstacleCount)
{
    return ScenarioBuilder("Pathfinding Test " + ::std::to_string(obstacleCount))
        .SetType(TestScenarioType::PATHFINDING_TEST)
        .SetEnvironment(TestEnvironment::OBSTACLE_COURSE)
        .SetDuration(35000)
        .SetArena(Position(0, 0, 0), 40.0f)
        .RequirePositioning(70.0f)
        .Build();
}

TestScenario CombatTestFramework::CreateThreatManagementScenario()
{
    return ScenarioBuilder("Threat Management Test")
        .SetType(TestScenarioType::THREAT_TEST)
        .SetEnvironment(TestEnvironment::OPEN_FIELD)
        .SetDuration(55000)
        .SetArena(Position(0, 0, 0), 50.0f)
        .RequireThreatManagement()
        .RequireCoordination(75.0f)
        .Build();
}

TestScenario CombatTestFramework::CreateBossEncounterScenario(uint32 bossId)
{
    return ScenarioBuilder("Boss Encounter " + ::std::to_string(bossId))
        .SetType(TestScenarioType::BOSS_MECHANICS_TEST)
        .SetEnvironment(TestEnvironment::DUNGEON_ROOM)
        .SetDuration(120000)
        .SetArena(Position(0, 0, 0), 60.0f)
        .RequireSurvival()
        .RequireThreatManagement()
        .RequireCoordination(80.0f)
        .Build();
}

float CombatTestFramework::GetAverageExecutionTime(const ::std::string& scenarioName) const
{
    ::std::lock_guard lock(_resultMutex);
    auto it = _scenarioHistory.find(scenarioName);
    if (it == _scenarioHistory.end() || it->second.empty())
        return 0.0f;

    uint64 totalTime = 0;
    for (const auto& result : it->second)
        totalTime += result.executionTimeMs;

    return static_cast<float>(totalTime) / it->second.size();
}

float CombatTestFramework::GetScenarioSuccessRate(const ::std::string& scenarioName) const
{
    ::std::lock_guard lock(_resultMutex);
    auto it = _scenarioHistory.find(scenarioName);
    if (it == _scenarioHistory.end() || it->second.empty())
        return 0.0f;

    uint32 successCount = 0;
    for (const auto& result : it->second)
    {
        if (result.success)
            ++successCount;
    }

    return static_cast<float>(successCount) / it->second.size() * 100.0f;
}

::std::vector<::std::string> CombatTestFramework::GetMostFailedScenarios(uint32 count) const
{
    ::std::vector<::std::pair<::std::string, float>> failureRates;

    {
        ::std::lock_guard lock(_resultMutex);
        for (const auto& [name, history] : _scenarioHistory)
        {
            if (!history.empty())
            {
                float successRate = GetScenarioSuccessRate(name);
                failureRates.emplace_back(name, 100.0f - successRate);
            }
        }
    }

    ::std::sort(failureRates.begin(), failureRates.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    ::std::vector<::std::string> result;
    size_t maxCount = ::std::min(static_cast<size_t>(count), failureRates.size());
    for (size_t i = 0; i < maxCount; ++i)
        result.push_back(failureRates[i].first);

    return result;
}

::std::unordered_map<::std::string, float> CombatTestFramework::GetSystemPerformanceRanking() const
{
    ::std::unordered_map<::std::string, float> ranking;

    for (const auto& [name, metrics] : _globalMetrics)
    {
        float score = metrics.GetSuccessRate() * 100.0f;

        // Adjust score based on performance characteristics
    if (metrics.averageExecutionTime.count() > 1000) // > 1ms
            score *= 0.9f;
        if (metrics.memoryUsage.load() > 1048576) // > 1MB
            score *= 0.95f;

        ranking[name] = score;
    }

    return ranking;
}

::std::string CombatTestFramework::GetCurrentScenarioName() const
{
    return _currentContext ? _currentContext->scenario.name : "";
}

uint32 CombatTestFramework::GetCurrentScenarioTimeMs() const
{
    return _currentContext ? _currentContext->currentTimeMs : 0;
}

float CombatTestFramework::GetCurrentScenarioProgress() const
{
    if (!_currentContext || _currentContext->scenario.durationMs == 0)
        return 0.0f;

    return static_cast<float>(_currentContext->currentTimeMs) / _currentContext->scenario.durationMs;
}

uint32 CombatTestFramework::GetActiveParticipantCount() const
{
    if (!_currentContext)
        return 0;

    uint32 count = 0;
    for (const auto& participant : _currentContext->scenario.participants)
    {
        if (participant.isAlive)
            ++count;
    }
    return count;
}

// Private method implementations

bool CombatTestFramework::InitializeScenario(const TestScenario& scenario, TestContext& context)
{
    LogTestEvent("Initializing scenario: " + scenario.name);

    // Setup test environment
    if (!SetupTestEnvironment(scenario.environment, scenario.centerPosition, scenario.arenaRadius))
    {
        TC_LOG_ERROR("playerbot", "CombatTestFramework: Failed to setup test environment");
        return false;
    }

    // Spawn obstacles
    if (!scenario.obstacles.empty())
        SpawnObstacles(scenario.obstacles, context);

    // Spawn participants
    for (const auto& participant : scenario.participants)
    {
        if (participant.isBot)
        {
            if (!SpawnBot(participant, context))
            {
                TC_LOG_ERROR("playerbot", "CombatTestFramework: Failed to spawn bot: {}", participant.name);
                return false;
            }
        }
        else
        {
            if (!SpawnEnemy(participant, context))
            {
                TC_LOG_ERROR("playerbot", "CombatTestFramework: Failed to spawn enemy: {}", participant.name);
                return false;
            }
        }
    }

    // Initialize system metrics
    for (const auto& systemName : scenario.requiredSystems)
    {
        if (_globalMetrics.find(systemName) != _globalMetrics.end())
        {
            context.systemMetrics[systemName] = _globalMetrics[systemName];
            context.systemMetrics[systemName].Reset();
        }
    }

    context.currentPhase = "Running";
    LogTestEvent("Scenario initialization complete");
    return true;
}

void CombatTestFramework::ExecuteScenarioUpdate(TestContext& context, uint32 diff)
{
    context.lastUpdateMs = diff;

    // Update all participants
    for (auto& participant : context.scenario.participants)
        UpdateParticipant(participant, diff);

    // Monitor combat systems
    if (_performanceMonitoring)
        MonitorCombatSystems(context, diff);

    // Update dynamic obstacles
    for (auto& obstacle : context.scenario.obstacles)
    {
        if (obstacle.isDynamic && obstacle.lifespan > 0)
        {
            if (obstacle.lifespan <= diff)
            {
                // Remove expired obstacle
                obstacle.lifespan = 0;
            }
            else
            {
                obstacle.lifespan -= diff;
            }
        }
    }
}

void CombatTestFramework::FinalizeScenario(TestContext& context, TestResult& result)
{
    LogTestEvent("Finalizing scenario: " + context.scenario.name);

    context.currentPhase = "Finalizing";

    // Evaluate all success criteria
    result.criteriaScores = EvaluateAllCriteria(context);

    // Calculate overall score
    float totalScore = 0.0f;
    for (const auto& [criteria, score] : result.criteriaScores)
        totalScore += score;

    result.overallScore = result.criteriaScores.empty() ? 0.0f : totalScore / result.criteriaScores.size();

    // Check if scenario was successful
    result.success = CheckSuccessCriteria(context.scenario, context);

    // Collect system performance data
    for (const auto& [name, metrics] : context.systemMetrics)
        result.systemPerformance[name] = metrics.GetSuccessRate() * 100.0f;

    // Generate detailed log
    ::std::ostringstream log;
    for (const auto& entry : _testLog)
        log << entry << "\n";
    result.detailedLog = log.str();

    // Cleanup
    CleanupScenario(context);

    LogTestEvent("Scenario finalization complete. Success: " + ::std::to_string(result.success));
}

void CombatTestFramework::CleanupScenario(TestContext& context)
{
    LogTestEvent("Cleaning up scenario: " + context.scenario.name);

    // Execute cleanup callbacks
    for (auto& callback : context.cleanupCallbacks)
        callback();

    // Remove all participants from world
    for (const auto& participant : context.scenario.participants)
        RemoveParticipantFromWorld(participant);

    // Clean up environment
    CleanupEnvironment(context);

    context.currentPhase = "Cleanup Complete";
}

bool CombatTestFramework::SpawnBot(const TestParticipant& participant, TestContext& context)
{
    // In a real implementation, this would create and configure a bot player
    LogTestEvent("Spawning bot: " + participant.name);
    return true;
}

bool CombatTestFramework::SpawnEnemy(const TestParticipant& participant, TestContext& context)
{
    // In a real implementation, this would spawn and configure an enemy creature
    LogTestEvent("Spawning enemy: " + participant.name);
    return true;
}

void CombatTestFramework::UpdateParticipant(TestParticipant& participant, uint32 /*diff*/)
{
    // Update participant state
    if (participant.isAlive && participant.health <= 0)
    {
        participant.isAlive = false;
        LogTestEvent("Participant died: " + participant.name);
    }
}

void CombatTestFramework::RemoveParticipantFromWorld(const TestParticipant& participant)
{
    // In a real implementation, this would remove the participant from the game world
    LogTestEvent("Removed participant from world: " + participant.name);
}

bool CombatTestFramework::CreateTestArea(TestEnvironment environment, const Position& center, float radius)
{
    // In a real implementation, this would create the test environment
    LogTestEvent("Created test area - Environment: " + ::std::to_string(static_cast<uint32>(environment)) +
                ", Radius: " + ::std::to_string(radius));
    return true;
}

void CombatTestFramework::SpawnObstacles(const ::std::vector<TestObstacle>& obstacles, TestContext& context)
{
    for (const auto& obstacle : obstacles)
        LogTestEvent("Spawned obstacle: " + obstacle.name);
}

void CombatTestFramework::CleanupEnvironment(TestContext& context)
{
    LogTestEvent("Cleaned up test environment");
}

Position CombatTestFramework::GenerateRandomPosition(const Position& center, float radius, float minDistance) const
{
    // Use TrinityCore's thread-safe random functions instead of static generators
    float angle = frand(0.0f, 2.0f * static_cast<float>(M_PI));
    float dist = frand(minDistance, radius);

    float x = center.GetPositionX() + dist * ::std::cos(angle);
    float y = center.GetPositionY() + dist * ::std::sin(angle);
    float z = center.GetPositionZ();

    return Position(x, y, z);
}

void CombatTestFramework::MonitorCombatSystems(TestContext& context, uint32 /*diff*/)
{
    // Monitor registered combat systems performance
    for (const auto& [name, system] : _registeredSystems)
    {
        auto& metrics = context.systemMetrics[name];
        metrics.updateCalls++;

        // In a real implementation, you would measure actual system performance here
        auto executionTime = ::std::chrono::microseconds(100); // Simulated execution time
        bool success = true; // Simulated success

        metrics.UpdateExecutionTime(executionTime);
        if (success)
            metrics.successfulOperations++;
        else
            metrics.failedOperations++;
    }
}

void CombatTestFramework::UpdateSystemMetrics(const ::std::string& systemName, ::std::chrono::microseconds executionTime, bool success)
{
    auto it = _globalMetrics.find(systemName);
    if (it != _globalMetrics.end())
    {
        it->second.updateCalls++;
        it->second.UpdateExecutionTime(executionTime);

        if (success)
            it->second.successfulOperations++;
        else
            it->second.failedOperations++;
    }
}

void CombatTestFramework::RecordSystemMemoryUsage(const ::std::string& systemName, uint64 memoryBytes)
{
    auto it = _globalMetrics.find(systemName);
    if (it != _globalMetrics.end())
        it->second.memoryUsage = memoryBytes;
}

// Criteria evaluation implementations
float CombatTestFramework::EvaluateSurvivalCriteria(const TestContext& context) const
{
    uint32 aliveCount = 0;
    uint32 totalBots = 0;

    for (const auto& participant : context.scenario.participants)
    {
        if (participant.isBot)
        {
            totalBots++;
            if (participant.isAlive)
                aliveCount++;
        }
    }

    return totalBots > 0 ? (static_cast<float>(aliveCount) / totalBots) * 100.0f : 0.0f;
}

float CombatTestFramework::EvaluateTimeLimitCriteria(const TestContext& context) const
{
    if (context.scenario.durationMs == 0)
        return 100.0f;

    float progress = static_cast<float>(context.currentTimeMs) / context.scenario.durationMs;
    return ::std::min(100.0f, progress * 100.0f);
}

float CombatTestFramework::EvaluateDamageDealtCriteria(const TestContext& context) const
{
    // Simulated damage evaluation - in real implementation would track actual damage
    return 75.0f;
}

float CombatTestFramework::EvaluatePositioningAccuracy(const TestContext& context) const
{
    // Simulated positioning evaluation - in real implementation would check actual positions
    return 80.0f;
}

float CombatTestFramework::EvaluateFormationIntegrity(const TestContext& context) const
{
    // Simulated formation evaluation - in real implementation would check formation adherence
    return 85.0f;
}

float CombatTestFramework::EvaluateInterruptSuccess(const TestContext& context) const
{
    // Simulated interrupt evaluation - in real implementation would track interrupt success rate
    return 70.0f;
}

float CombatTestFramework::EvaluateThreatManagement(const TestContext& context) const
{
    // Simulated threat evaluation - in real implementation would check threat distribution
    return 78.0f;
}

float CombatTestFramework::EvaluateCoordination(const TestContext& context) const
{
    // Simulated coordination evaluation - in real implementation would measure team coordination
    return 72.0f;
}

bool CombatTestFramework::ValidateParticipant(const TestParticipant& participant) const
{
    if (participant.name.empty())
        return false;

    if (participant.level == 0 || participant.level > 85)
        return false;

    if (participant.playerClass == 0 || participant.playerClass > 13)
        return false;

    return true;
}

bool CombatTestFramework::ValidateEnvironment(TestEnvironment environment, const Position& center, float radius) const
{
    if (radius < MIN_ARENA_RADIUS || radius > MAX_ARENA_RADIUS)
        return false;

    // Additional environment-specific validation would go here

    return true;
}

::std::string CombatTestFramework::GenerateUniqueTestId() const
{
    return "TEST_" + ::std::to_string(_nextTestId++);
}

uint32 CombatTestFramework::CalculateTestScore(const ::std::unordered_map<TestCriteria, float>& scores) const
{
    if (scores.empty())
        return 0;

    float total = 0.0f;
    for (const auto& [criteria, score] : scores)
        total += score;

    return static_cast<uint32>(total / scores.size());
}

// ScenarioBuilder implementation

ScenarioBuilder::ScenarioBuilder(const ::std::string& name)
    : _nextParticipantId(1), _nextObstacleId(1)
{
    _scenario.name = name;
}

ScenarioBuilder& ScenarioBuilder::SetType(TestScenarioType type)
{
    _scenario.type = type;
    return *this;
}

ScenarioBuilder& ScenarioBuilder::SetEnvironment(TestEnvironment environment)
{
    _scenario.environment = environment;
    return *this;
}

ScenarioBuilder& ScenarioBuilder::SetDuration(uint32 durationMs)
{
    _scenario.durationMs = durationMs;
    return *this;
}

ScenarioBuilder& ScenarioBuilder::SetArena(const Position& center, float radius)
{
    _scenario.centerPosition = center;
    _scenario.arenaRadius = radius;
    return *this;
}

ScenarioBuilder& ScenarioBuilder::SetDescription(const ::std::string& description)
{
    _scenario.description = description;
    return *this;
}

ScenarioBuilder& ScenarioBuilder::AddBot(TestRole role, uint8 playerClass, uint8 level)
{
    TestParticipant participant;
    participant.role = role;
    participant.playerClass = playerClass;
    participant.level = level;
    participant.isBot = true;
    participant.name = "Bot" + ::std::to_string(_nextParticipantId++);
    _scenario.participants.push_back(participant);
    return *this;
}

ScenarioBuilder& ScenarioBuilder::AddEnemy(TestRole role, uint8 level)
{
    TestParticipant participant;
    participant.role = role;
    participant.level = level;
    participant.isBot = false;
    participant.name = "Enemy" + ::std::to_string(_nextParticipantId++);
    _scenario.participants.push_back(participant);
    return *this;
}

ScenarioBuilder& ScenarioBuilder::AddCustomParticipant(const TestParticipant& participant)
{
    _scenario.participants.push_back(participant);
    return *this;
}

ScenarioBuilder& ScenarioBuilder::AddObstacle(const Position& pos, float radius, bool blocksLoS)
{
    TestObstacle obstacle;
    obstacle.position = pos;
    obstacle.radius = radius;
    obstacle.blocksLoS = blocksLoS;
    obstacle.name = "Obstacle" + ::std::to_string(_nextObstacleId++);
    _scenario.obstacles.push_back(obstacle);
    return *this;
}

ScenarioBuilder& ScenarioBuilder::AddDynamicObstacle(const Position& pos, float radius, uint32 lifespanMs)
{
    TestObstacle obstacle;
    obstacle.position = pos;
    obstacle.radius = radius;
    obstacle.isDynamic = true;
    obstacle.lifespan = lifespanMs;
    obstacle.name = "DynamicObstacle" + ::std::to_string(_nextObstacleId++);
    _scenario.obstacles.push_back(obstacle);
    return *this;
}

ScenarioBuilder& ScenarioBuilder::RequireSurvival()
{
    _scenario.successCriteria.push_back(TestCriteria::SURVIVAL);
    return *this;
}

ScenarioBuilder& ScenarioBuilder::RequireTimeLimit(uint32 maxTimeMs)
{
    _scenario.successCriteria.push_back(TestCriteria::TIME_LIMIT);
    _scenario.parameters["maxTimeMs"] = static_cast<float>(maxTimeMs);
    return *this;
}

ScenarioBuilder& ScenarioBuilder::RequireDamageDealt(float minDamage)
{
    _scenario.successCriteria.push_back(TestCriteria::DAMAGE_DEALT);
    _scenario.parameters["minDamage"] = minDamage;
    return *this;
}

ScenarioBuilder& ScenarioBuilder::RequirePositioning(float accuracy)
{
    _scenario.successCriteria.push_back(TestCriteria::POSITIONING_ACCURACY);
    _scenario.parameters["positioningAccuracy"] = accuracy;
    return *this;
}

ScenarioBuilder& ScenarioBuilder::RequireFormationIntegrity(float integrity)
{
    _scenario.successCriteria.push_back(TestCriteria::FORMATION_INTEGRITY);
    _scenario.parameters["formationIntegrity"] = integrity;
    return *this;
}

ScenarioBuilder& ScenarioBuilder::RequireInterruptSuccess(float successRate)
{
    _scenario.successCriteria.push_back(TestCriteria::INTERRUPT_SUCCESS);
    _scenario.parameters["interruptSuccessRate"] = successRate;
    return *this;
}

ScenarioBuilder& ScenarioBuilder::RequireThreatManagement()
{
    _scenario.successCriteria.push_back(TestCriteria::THREAT_MANAGEMENT);
    return *this;
}

ScenarioBuilder& ScenarioBuilder::RequireCoordination(float minScore)
{
    _scenario.successCriteria.push_back(TestCriteria::COORDINATION);
    _scenario.parameters["coordinationMinScore"] = minScore;
    return *this;
}

ScenarioBuilder& ScenarioBuilder::RequireSystem(const ::std::string& systemName)
{
    _scenario.requiredSystems.push_back(systemName);
    return *this;
}

ScenarioBuilder& ScenarioBuilder::SetParameter(const ::std::string& key, float value)
{
    _scenario.parameters[key] = value;
    return *this;
}

TestScenario ScenarioBuilder::Build() const
{
    return _scenario;
}

} // namespace Playerbot