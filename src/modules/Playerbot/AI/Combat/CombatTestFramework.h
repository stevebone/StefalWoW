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
#include "ObjectGuid.h"
#include "Position.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <atomic>
#include <chrono>
#include <functional>
#include <string>

// Forward declarations
class Player;
class Unit;
class Creature;
class GameObject;
class Group;

namespace Playerbot
{

// Forward declarations for combat systems
class BotThreatManager;
class TargetSelector;
class PositionManager;
class LineOfSightManager;
class PathfindingManager;
class ObstacleAvoidanceManager;
class FormationManager;
class KitingManager;
class InterruptManager;

// Test scenario types
enum class TestScenarioType : uint8
{
    BASIC_COMBAT = 0,           // Simple 1v1 combat
    GROUP_COMBAT = 1,           // Group vs group combat
    DUNGEON_COMBAT = 2,         // Dungeon encounter simulation
    RAID_COMBAT = 3,            // Raid encounter simulation
    PVP_COMBAT = 4,             // Player vs player combat
    FORMATION_TEST = 5,         // Formation movement testing
    KITING_TEST = 6,            // Kiting behavior testing
    INTERRUPT_TEST = 7,         // Interrupt timing testing
    POSITIONING_TEST = 8,       // Positioning optimization testing
    PATHFINDING_TEST = 9,       // Pathfinding algorithm testing
    THREAT_TEST = 10,           // Threat management testing
    LINE_OF_SIGHT_TEST = 11,    // LoS validation testing
    MULTI_TARGET_TEST = 12,     // Multiple target scenarios
    BOSS_MECHANICS_TEST = 13,   // Boss encounter mechanics
    SURVIVAL_TEST = 14          // Survival under pressure
};

// Test environment configurations
enum class TestEnvironment : uint8
{
    OPEN_FIELD = 0,             // Open terrain with no obstacles
    DUNGEON_ROOM = 1,           // Enclosed room with walls
    NARROW_CORRIDOR = 2,        // Narrow passage
    MULTI_LEVEL = 3,            // Multiple elevation levels
    OBSTACLE_COURSE = 4,        // Dense obstacles
    WATER_TERRAIN = 5,          // Water/swimming areas
    CUSTOM_LAYOUT = 6           // User-defined layout
};

// Test participant role
enum class TestRole : uint8
{
    TANK = 0,                   // Tank role
    HEALER = 1,                 // Healer role
    MELEE_DPS = 2,              // Melee damage dealer
    RANGED_DPS = 3,             // Ranged damage dealer
    SUPPORT = 4,                // Support/utility
    ENEMY_MELEE = 5,            // Enemy melee unit
    ENEMY_RANGED = 6,           // Enemy ranged unit
    ENEMY_CASTER = 7,           // Enemy spellcaster
    ENEMY_BOSS = 8,             // Boss enemy
    NEUTRAL_NPC = 9             // Neutral participant
};

// Test success criteria
enum class TestCriteria : uint8
{
    SURVIVAL = 0,               // All bots survive
    TIME_LIMIT = 1,             // Complete within time limit
    DAMAGE_DEALT = 2,           // Deal minimum damage
    DAMAGE_TAKEN = 3,           // Take maximum damage
    HEALING_DONE = 4,           // Heal minimum amount
    POSITIONING_ACCURACY = 5,   // Maintain correct positions
    FORMATION_INTEGRITY = 6,    // Keep formation intact
    INTERRUPT_SUCCESS = 7,      // Successful interrupt rate
    THREAT_MANAGEMENT = 8,      // Proper threat distribution
    RESOURCE_EFFICIENCY = 9,    // Efficient resource usage
    MECHANICS_EXECUTION = 10,   // Execute mechanics correctly
    COORDINATION = 11           // Team coordination score
};

// Test participant information
struct TestParticipant
{
    ObjectGuid guid;
    Player* player;
    Creature* creature;
    TestRole role;
    uint8 level;
    uint8 playerClass;
    Position startPosition;
    Position currentPosition;
    float health;
    float maxHealth;
    float mana;
    float maxMana;
    bool isBot;
    bool isAlive;
    ::std::string name;
    ::std::vector<uint32> testSpells;
    ::std::unordered_map<::std::string, float> customProperties;

    TestParticipant() : player(nullptr), creature(nullptr), role(TestRole::MELEE_DPS),
                       level(80), playerClass(1), health(100.0f), maxHealth(100.0f),
                       mana(100.0f), maxMana(100.0f), isBot(true), isAlive(true) {}
};

// Test obstacle definition
struct TestObstacle
{
    ObjectGuid guid;
    GameObject* gameObject;
    Position position;
    float radius;
    float height;
    bool blocksMovement;
    bool blocksLoS;
    bool isDynamic;
    uint32 lifespan;
    ::std::string name;

    TestObstacle() : gameObject(nullptr), radius(1.0f), height(2.0f),
                    blocksMovement(true), blocksLoS(true), isDynamic(false), lifespan(0) {}
};

// Test scenario configuration
struct TestScenario
{
    ::std::string name;
    ::std::string description;
    TestScenarioType type;
    TestEnvironment environment;
    uint32 durationMs;
    uint32 maxParticipants;
    Position centerPosition;
    float arenaRadius;
    ::std::vector<TestParticipant> participants;
    ::std::vector<TestObstacle> obstacles;
    ::std::vector<TestCriteria> successCriteria;
    ::std::unordered_map<::std::string, float> parameters;
    ::std::vector<::std::string> requiredSystems;

    TestScenario() : type(TestScenarioType::BASIC_COMBAT), environment(TestEnvironment::OPEN_FIELD),
                    durationMs(60000), maxParticipants(10), arenaRadius(50.0f) {}
};

// Test execution result
struct TestResult
{
    ::std::string scenarioName;
    bool success;
    uint32 executionTimeMs;
    float overallScore;
    ::std::unordered_map<TestCriteria, float> criteriaScores;
    ::std::unordered_map<::std::string, float> systemPerformance;
    ::std::vector<::std::string> failures;
    ::std::vector<::std::string> warnings;
    ::std::string detailedLog;
    ::std::chrono::steady_clock::time_point startTime;
    ::std::chrono::steady_clock::time_point endTime;

    TestResult() : success(false), executionTimeMs(0), overallScore(0.0f) {}
};

// Performance metrics for combat systems
struct CombatSystemMetrics
{
    ::std::string systemName;
    ::std::atomic<uint32> updateCalls{0};
    ::std::atomic<uint32> successfulOperations{0};
    ::std::atomic<uint32> failedOperations{0};
    ::std::chrono::microseconds totalExecutionTime{0};
    ::std::chrono::microseconds minExecutionTime{::std::chrono::microseconds::max()};
    ::std::chrono::microseconds maxExecutionTime{0};
    ::std::chrono::microseconds averageExecutionTime{0};
    ::std::atomic<uint64> memoryUsage{0};
    ::std::chrono::steady_clock::time_point lastReset;

    void Reset()
    {
        updateCalls = 0;
        successfulOperations = 0;
        failedOperations = 0;
        totalExecutionTime = ::std::chrono::microseconds{0};
        minExecutionTime = ::std::chrono::microseconds::max();
        maxExecutionTime = ::std::chrono::microseconds{0};
        averageExecutionTime = ::std::chrono::microseconds{0};
        memoryUsage = 0;
        lastReset = ::std::chrono::steady_clock::now();
    }

    float GetSuccessRate() const
    {
        uint32 total = updateCalls.load();
        return total > 0 ? static_cast<float>(successfulOperations.load()) / total : 0.0f;
    }

    void UpdateExecutionTime(::std::chrono::microseconds executionTime)
    {
        totalExecutionTime += executionTime;
        if (executionTime < minExecutionTime)
            minExecutionTime = executionTime;
        if (executionTime > maxExecutionTime)
            maxExecutionTime = executionTime;

        uint32 calls = updateCalls.load();
        if (calls > 0)
            averageExecutionTime = totalExecutionTime / calls;
    }
};

// Test execution context
struct TestContext
{
    TestScenario scenario;
    ::std::vector<Player*> bots;
    ::std::vector<Creature*> enemies;
    Group* testGroup;
    ::std::unordered_map<::std::string, CombatSystemMetrics> systemMetrics;
    bool isRunning;
    bool isPaused;
    uint32 currentTimeMs;
    uint32 lastUpdateMs;
    ::std::string currentPhase;
    ::std::vector<::std::function<void()>> cleanupCallbacks;

    TestContext() : testGroup(nullptr), isRunning(false), isPaused(false),
                   currentTimeMs(0), lastUpdateMs(0), currentPhase("Initialization") {}
};

class TC_GAME_API CombatTestFramework
{
public:
    CombatTestFramework();
    ~CombatTestFramework();

    // Framework lifecycle
    bool Initialize();
    void Shutdown();
    void Update(uint32 diff);

    // Scenario management
    bool LoadScenario(const ::std::string& scenarioFile);
    bool CreateScenario(const TestScenario& scenario);
    ::std::vector<::std::string> GetAvailableScenarios() const;
    TestScenario* GetScenario(const ::std::string& name);
    bool SaveScenario(const TestScenario& scenario, const ::std::string& filename);

    // Test execution
    TestResult ExecuteScenario(const ::std::string& scenarioName);
    TestResult ExecuteScenario(const TestScenario& scenario);
    bool StartScenario(const ::std::string& scenarioName);
    void StopCurrentScenario();
    void PauseCurrentScenario();
    void ResumeCurrentScenario();

    // Participant management
    bool AddParticipant(const TestParticipant& participant);
    bool RemoveParticipant(ObjectGuid guid);
    TestParticipant* GetParticipant(ObjectGuid guid);
    ::std::vector<TestParticipant*> GetParticipantsByRole(TestRole role);
    void ClearParticipants();

    // Environment setup
    bool SetupTestEnvironment(TestEnvironment environment, const Position& center, float radius);
    bool AddObstacle(const TestObstacle& obstacle);
    bool RemoveObstacle(ObjectGuid guid);
    void ClearObstacles();
    bool SpawnTestCreatures(const ::std::vector<TestParticipant>& enemies);

    // Combat system integration
    void RegisterCombatSystem(const ::std::string& name, void* system);
    void UnregisterCombatSystem(const ::std::string& name);
    bool IsCombatSystemRegistered(const ::std::string& name) const;
    CombatSystemMetrics* GetSystemMetrics(const ::std::string& name);

    // Performance monitoring
    void StartPerformanceMonitoring();
    void StopPerformanceMonitoring();
    void ResetMetrics();
    ::std::unordered_map<::std::string, CombatSystemMetrics> GetAllMetrics() const;
    float CalculateOverallPerformanceScore() const;

    // Test validation
    bool ValidateScenario(const TestScenario& scenario) const;
    ::std::vector<::std::string> GetScenarioValidationErrors(const TestScenario& scenario) const;
    bool ValidateTestResults(const TestResult& result) const;

    // Criteria evaluation
    float EvaluateCriteria(TestCriteria criteria, const TestContext& context) const;
    bool CheckSuccessCriteria(const TestScenario& scenario, const TestContext& context) const;
    ::std::unordered_map<TestCriteria, float> EvaluateAllCriteria(const TestContext& context) const;

    // Reporting and analysis
    ::std::string GenerateDetailedReport(const TestResult& result) const;
    ::std::string GeneratePerformanceReport() const;
    bool SaveTestResults(const TestResult& result, const ::std::string& filename) const;
    ::std::vector<TestResult> LoadTestHistory(const ::std::string& scenarioName) const;

    // Debugging and visualization
    void EnableDebugMode(bool enable) { _debugMode = enable; }
    bool IsDebugMode() const { return _debugMode; }
    void LogTestEvent(const ::std::string& event, const ::std::string& details = "");
    ::std::vector<::std::string> GetTestLog() const { return _testLog; }
    void ClearTestLog() { _testLog.clear(); }

    // Scenario templates
    TestScenario CreateBasicCombatScenario(uint32 botCount, uint32 enemyCount);
    TestScenario CreateGroupFormationScenario(uint32 groupSize);
    TestScenario CreateKitingScenario(TestRole kitingRole);
    TestScenario CreateInterruptScenario(uint32 casterCount);
    TestScenario CreatePositioningScenario(TestEnvironment environment);
    TestScenario CreatePathfindingScenario(uint32 obstacleCount);
    TestScenario CreateThreatManagementScenario();
    TestScenario CreateBossEncounterScenario(uint32 bossId);

    // Statistics and analysis
    float GetAverageExecutionTime(const ::std::string& scenarioName) const;
    float GetScenarioSuccessRate(const ::std::string& scenarioName) const;
    ::std::vector<::std::string> GetMostFailedScenarios(uint32 count = 5) const;
    ::std::unordered_map<::std::string, float> GetSystemPerformanceRanking() const;

    // Configuration
    void SetDefaultTestDuration(uint32 durationMs) { _defaultDurationMs = durationMs; }
    uint32 GetDefaultTestDuration() const { return _defaultDurationMs; }
    void SetMaxConcurrentTests(uint32 maxTests) { _maxConcurrentTests = maxTests; }
    uint32 GetMaxConcurrentTests() const { return _maxConcurrentTests; }
    void SetPerformanceMonitoringInterval(uint32 intervalMs) { _monitoringIntervalMs = intervalMs; }

    // Query methods
    bool IsScenarioRunning() const { return _currentContext && _currentContext->isRunning; }
    ::std::string GetCurrentScenarioName() const;
    uint32 GetCurrentScenarioTimeMs() const;
    float GetCurrentScenarioProgress() const;
    uint32 GetActiveParticipantCount() const;

private:
    // Core execution methods
    bool InitializeScenario(const TestScenario& scenario, TestContext& context);
    void ExecuteScenarioUpdate(TestContext& context, uint32 diff);
    void FinalizeScenario(TestContext& context, TestResult& result);
    void CleanupScenario(TestContext& context);

    // Participant management helpers
    bool SpawnBot(const TestParticipant& participant, TestContext& context);
    bool SpawnEnemy(const TestParticipant& participant, TestContext& context);
    void UpdateParticipant(TestParticipant& participant, uint32 diff);
    void RemoveParticipantFromWorld(const TestParticipant& participant);

    // Environment helpers
    bool CreateTestArea(TestEnvironment environment, const Position& center, float radius);
    void SpawnObstacles(const ::std::vector<TestObstacle>& obstacles, TestContext& context);
    void CleanupEnvironment(TestContext& context);
    Position GenerateRandomPosition(const Position& center, float radius, float minDistance = 0.0f) const;

    // Combat system monitoring
    void MonitorCombatSystems(TestContext& context, uint32 diff);
    void UpdateSystemMetrics(const ::std::string& systemName, ::std::chrono::microseconds executionTime, bool success);
    void RecordSystemMemoryUsage(const ::std::string& systemName, uint64 memoryBytes);

    // Criteria evaluation helpers
    float EvaluateSurvivalCriteria(const TestContext& context) const;
    float EvaluateTimeLimitCriteria(const TestContext& context) const;
    float EvaluateDamageDealtCriteria(const TestContext& context) const;
    float EvaluatePositioningAccuracy(const TestContext& context) const;
    float EvaluateFormationIntegrity(const TestContext& context) const;
    float EvaluateInterruptSuccess(const TestContext& context) const;
    float EvaluateThreatManagement(const TestContext& context) const;
    float EvaluateCoordination(const TestContext& context) const;

    // Utility methods
    bool ValidateParticipant(const TestParticipant& participant) const;
    bool ValidateEnvironment(TestEnvironment environment, const Position& center, float radius) const;
    ::std::string GenerateUniqueTestId() const;
    uint32 CalculateTestScore(const ::std::unordered_map<TestCriteria, float>& scores) const;

private:
    // Framework state
    bool _initialized;
    bool _debugMode;
    uint32 _nextTestId;

    // Configuration
    uint32 _defaultDurationMs;
    uint32 _maxConcurrentTests;
    uint32 _monitoringIntervalMs;
    uint32 _lastMonitoringUpdate;

    // Current test execution
    ::std::unique_ptr<TestContext> _currentContext;
    ::std::unordered_map<::std::string, TestScenario> _scenarios;
    ::std::unordered_map<::std::string, void*> _registeredSystems;

    // Performance tracking
    ::std::unordered_map<::std::string, CombatSystemMetrics> _globalMetrics;
    ::std::vector<TestResult> _testHistory;
    bool _performanceMonitoring;

    // Logging and debugging
    ::std::vector<::std::string> _testLog;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _logMutex;

    // Test result storage
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _resultMutex;
    ::std::unordered_map<::std::string, ::std::vector<TestResult>> _scenarioHistory;

    // Constants
    static constexpr uint32 DEFAULT_TEST_DURATION = 60000;          // 60 seconds
    static constexpr uint32 MAX_CONCURRENT_TESTS = 1;              // One test at a time
    static constexpr uint32 MONITORING_INTERVAL = 1000;            // 1 second
    static constexpr uint32 MAX_PARTICIPANTS = 40;                 // Maximum participants per test
    static constexpr float MIN_ARENA_RADIUS = 10.0f;              // Minimum test area radius
    static constexpr float MAX_ARENA_RADIUS = 200.0f;             // Maximum test area radius
    static constexpr uint32 MAX_TEST_LOG_ENTRIES = 1000;          // Maximum log entries
    static constexpr uint32 MAX_HISTORY_ENTRIES = 100;            // Maximum history per scenario
};

// Scenario builder helper class
class TC_GAME_API ScenarioBuilder
{
public:
    ScenarioBuilder(const ::std::string& name);

    // Scenario configuration
    ScenarioBuilder& SetType(TestScenarioType type);
    ScenarioBuilder& SetEnvironment(TestEnvironment environment);
    ScenarioBuilder& SetDuration(uint32 durationMs);
    ScenarioBuilder& SetArena(const Position& center, float radius);
    ScenarioBuilder& SetDescription(const ::std::string& description);

    // Participant management
    ScenarioBuilder& AddBot(TestRole role, uint8 playerClass, uint8 level = 80);
    ScenarioBuilder& AddEnemy(TestRole role, uint8 level = 80);
    ScenarioBuilder& AddCustomParticipant(const TestParticipant& participant);

    // Environment configuration
    ScenarioBuilder& AddObstacle(const Position& pos, float radius, bool blocksLoS = true);
    ScenarioBuilder& AddDynamicObstacle(const Position& pos, float radius, uint32 lifespanMs);

    // Success criteria
    ScenarioBuilder& RequireSurvival();
    ScenarioBuilder& RequireTimeLimit(uint32 maxTimeMs);
    ScenarioBuilder& RequireDamageDealt(float minDamage);
    ScenarioBuilder& RequirePositioning(float accuracy);
    ScenarioBuilder& RequireFormationIntegrity(float integrity);
    ScenarioBuilder& RequireInterruptSuccess(float successRate);
    ScenarioBuilder& RequireThreatManagement();
    ScenarioBuilder& RequireCoordination(float minScore);

    // System requirements
    ScenarioBuilder& RequireSystem(const ::std::string& systemName);
    ScenarioBuilder& SetParameter(const ::std::string& key, float value);

    // Build the scenario
    TestScenario Build() const;

private:
    TestScenario _scenario;
    uint32 _nextParticipantId;
    uint32 _nextObstacleId;
};

} // namespace Playerbot