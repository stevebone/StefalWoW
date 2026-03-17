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
#include "Guild.h"
#include <unordered_map>
#include <vector>
#include <atomic>
#include <mutex>
#include <functional>

namespace Playerbot
{
    // Interface


enum class ValidationLevel : uint8
{
    BASIC       = 0,
    STANDARD    = 1,
    THOROUGH    = 2,
    EXHAUSTIVE  = 3
};

enum class SystemComponent : uint8
{
    GROUP_MANAGER           = 0,
    ROLE_ASSIGNMENT         = 1,
    QUEST_AUTOMATION        = 2,
    DUNGEON_BEHAVIOR        = 3,
    LOOT_DISTRIBUTION       = 4,
    TRADE_SYSTEM           = 5,
    AUCTION_HOUSE          = 6,
    GUILD_INTEGRATION      = 7,
    ALL_SYSTEMS            = 8
};

struct ValidationResult
{
    SystemComponent component;
    bool isValid;
    float healthScore; // 0.0 = critical, 1.0 = perfect
    ::std::vector<::std::string> issues;
    ::std::vector<::std::string> warnings;
    ::std::vector<::std::string> recommendations;
    uint32 validationTime;
    uint32 totalChecks;
    uint32 passedChecks;

    ValidationResult(SystemComponent comp) : component(comp), isValid(true)
        , healthScore(1.0f), validationTime(0), totalChecks(0), passedChecks(0) {}
};

/**
 * @brief Comprehensive system validation framework for playerbot integrity
 *
 * This system provides thorough validation of all playerbot systems, detecting
 * inconsistencies, performance issues, and ensuring overall system health.
 */
class TC_GAME_API SystemValidation final 
{
public:
    static SystemValidation* instance();

    // Core validation framework
    ValidationResult ValidateSystem(SystemComponent component, ValidationLevel level = ValidationLevel::STANDARD);
    ::std::vector<ValidationResult> ValidateAllSystems(ValidationLevel level = ValidationLevel::STANDARD);
    bool RunSystemHealthCheck();
    void PerformSystemDiagnostics();

    // Component-specific validation
    ValidationResult ValidateGroupManager(ValidationLevel level);
    ValidationResult ValidateRoleAssignment(ValidationLevel level);
    ValidationResult ValidateQuestAutomation(ValidationLevel level);
    ValidationResult ValidateDungeonBehavior(ValidationLevel level);
    ValidationResult ValidateLootDistribution(ValidationLevel level);
    ValidationResult ValidateTradeSystem(ValidationLevel level);
    ValidationResult ValidateAuctionHouse(ValidationLevel level);
    ValidationResult ValidateGuildIntegration(ValidationLevel level);

    // Data integrity validation
    bool ValidatePlayerData(::Player* player);
    bool ValidateGroupData(::Group* group);
    bool ValidateGuildData(::Guild* guild);
    bool ValidateQuestData(::Player* player);
    bool ValidateLootData(::Group* group);

    // Performance validation
    struct PerformanceValidation
    {
        SystemComponent component;
        float cpuUsage;
        size_t memoryUsage;
        uint32 responseTime;
        uint32 throughput;
        bool meetsPerformanceTargets;
        ::std::vector<::std::string> bottlenecks;

        PerformanceValidation(SystemComponent comp) : component(comp), cpuUsage(0.0f)
            , memoryUsage(0), responseTime(0), throughput(0), meetsPerformanceTargets(true) {}
    };

    PerformanceValidation ValidateSystemPerformance(SystemComponent component);
    ::std::vector<PerformanceValidation> ValidateAllPerformance();
    bool ValidateMemoryUsage();
    bool ValidateResponseTimes();

    // Consistency validation
    bool ValidateCrossSystemConsistency();
    bool ValidateDataSynchronization();
    bool ValidateStateConsistency(::Player* player);
    bool ValidateGroupStateConsistency(::Group* group);
    bool ValidateGuildStateConsistency(::Guild* guild);

    // Configuration validation
    bool ValidateSystemConfiguration();
    bool ValidatePlayerBotConfigurations();
    bool ValidateDatabaseIntegrity();
    bool ValidateModuleIntegration();

    // Runtime validation
    void EnableContinuousValidation(bool enable);
    void SetValidationInterval(uint32 intervalMs);
    void RegisterValidationTrigger(const ::std::string& triggerName, ::std::function<bool()> validator);
    void ValidateOnEvent(const ::std::string& eventName);

    // Validation reporting
    struct SystemHealthReport
    {
        ::std::vector<ValidationResult> componentResults;
        ::std::vector<PerformanceValidation> performanceResults;
        float overallHealthScore;
        uint32 criticalIssues;
        uint32 warnings;
        uint32 totalChecks;
        ::std::chrono::steady_clock::time_point reportTime;
        bool systemHealthy;

        SystemHealthReport() : overallHealthScore(1.0f), criticalIssues(0), warnings(0)
            , totalChecks(0), reportTime(::std::chrono::steady_clock::now()), systemHealthy(true) {}
    };

    SystemHealthReport GenerateHealthReport();
    void ExportValidationReport(const ::std::string& filename);
    void LogValidationResults(const ValidationResult& result);
    ::std::vector<::std::string> GetCriticalIssues();

    // Automated fixing and recovery
    bool AttemptAutomaticFix(const ValidationResult& result);
    void SuggestManualFixes(const ValidationResult& result);
    bool RecoverFromValidationFailure(SystemComponent component);
    void RestoreSystemDefaults(SystemComponent component);

    // Validation metrics and analytics
    struct ValidationMetrics
    {
        ::std::atomic<uint32> totalValidations{0};
        ::std::atomic<uint32> successfulValidations{0};
        ::std::atomic<uint32> failedValidations{0};
        ::std::atomic<uint32> issuesDetected{0};
        ::std::atomic<uint32> issuesResolved{0};
        ::std::atomic<float> averageHealthScore{1.0f};
        ::std::atomic<uint32> averageValidationTime{0};

        void Reset()
        {
            totalValidations = 0; successfulValidations = 0; failedValidations = 0;
            issuesDetected = 0; issuesResolved = 0; averageHealthScore = 1.0f;
            averageValidationTime = 0;
        }
    };

    ValidationMetrics GetValidationMetrics() override { return _metrics; }

    // Advanced validation features
    void SetupValidationSchedule(const ::std::string& schedule);
    void ValidateAfterSystemChanges();
    void ValidateBeforeCriticalOperations();
    void MonitorSystemDegradation();

    // Custom validation rules
    void AddCustomValidationRule(const ::std::string& ruleName, ::std::function<bool(SystemComponent)> rule);
    void RemoveCustomValidationRule(const ::std::string& ruleName);
    ::std::vector<::std::string> GetActiveValidationRules();

    // Update and maintenance
    void Update(uint32 diff);
    void ProcessValidationQueue();
    void CleanupValidationData();

private:
    SystemValidation();
    ~SystemValidation() = default;

    // Core validation data
    ::std::unordered_map<SystemComponent, ValidationResult> _lastResults;
    ::std::unordered_map<::std::string, ::std::function<bool()>> _validationTriggers;
    ::std::unordered_map<::std::string, ::std::function<bool(SystemComponent)>> _customRules;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BEHAVIOR_MANAGER> _validationMutex;

    // Continuous validation
    ::std::atomic<bool> _continuousValidationEnabled{false};
    ::std::atomic<uint32> _validationInterval{300000}; // 5 minutes
    uint32 _lastValidationTime{0};

    // Performance tracking
    ValidationMetrics _metrics;

    // Validation implementations
    bool ValidateGroupManagerState();
    bool ValidateRoleAssignmentLogic();
    bool ValidateQuestAutomationFlow();
    bool ValidateDungeonBehaviorRules();
    bool ValidateLootDistributionFairness();
    bool ValidateTradeSystemSafety();
    bool ValidateAuctionHouseIntegrity();
    bool ValidateGuildIntegrationHealth();

    // Data validation helpers
    bool ValidatePlayerInventoryConsistency(Player* player);
    bool ValidatePlayerStatsConsistency(Player* player);
    bool ValidateGroupMembershipConsistency(Group* group);
    bool ValidateQuestProgressConsistency(Player* player);
    bool ValidateLootTableConsistency();

    // Performance measurement
    void MeasureComponentPerformance(SystemComponent component, PerformanceValidation& result);
    bool CheckPerformanceTargets(const PerformanceValidation& validation);
    void IdentifyPerformanceBottlenecks(PerformanceValidation& validation);

    // Issue detection and classification
    void ClassifyIssue(ValidationResult& result, const ::std::string& issue);
    bool IsCriticalIssue(const ::std::string& issue);
    bool IsPerformanceIssue(const ::std::string& issue);
    void PrioritizeIssues(ValidationResult& result);

    // Automatic fixing implementations
    bool FixGroupManagerIssues(const ValidationResult& result);
    bool FixRoleAssignmentIssues(const ValidationResult& result);
    bool FixQuestAutomationIssues(const ValidationResult& result);
    bool FixLootDistributionIssues(const ValidationResult& result);
    bool FixTradeSystemIssues(const ValidationResult& result);
    bool FixGuildIntegrationIssues(const ValidationResult& result);

    // Validation algorithms
    float CalculateHealthScore(const ValidationResult& result);
    bool RunValidationChecks(SystemComponent component, ValidationLevel level, ValidationResult& result);
    void AddValidationCheck(ValidationResult& result, const ::std::string& checkName, bool passed);
    void GenerateRecommendations(ValidationResult& result);

    // System state analysis
    void AnalyzeSystemTrends();
    void DetectAnomalousPatterns();
    void PredictSystemIssues();
    void MonitorResourceUsage();

    // Constants
    static constexpr uint32 VALIDATION_UPDATE_INTERVAL = 10000; // 10 seconds
    static constexpr uint32 DEFAULT_VALIDATION_INTERVAL = 300000; // 5 minutes
    static constexpr float CRITICAL_HEALTH_THRESHOLD = 0.6f;
    static constexpr float WARNING_HEALTH_THRESHOLD = 0.8f;
    static constexpr uint32 MAX_VALIDATION_TIME = 60000; // 1 minute
    static constexpr uint32 PERFORMANCE_SAMPLE_DURATION = 30000; // 30 seconds
    static constexpr float MAX_CPU_USAGE_PERCENT = 80.0f;
    static constexpr size_t MAX_MEMORY_USAGE_MB = 1024; // 1GB
    static constexpr uint32 MAX_RESPONSE_TIME_MS = 100;
    static constexpr uint32 MIN_THROUGHPUT_OPS = 1000;
};

} // namespace Playerbot