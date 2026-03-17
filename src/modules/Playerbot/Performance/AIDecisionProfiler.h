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
#include <array>
#include <thread>
#include <condition_variable>
#include <queue>

namespace Playerbot
{

// AI decision categories for detailed analysis
enum class AIDecisionCategory : uint8
{
    COMBAT_ROTATION     = 0,  // Combat ability rotation decisions
    TARGET_SELECTION    = 1,  // Target selection and switching
    MOVEMENT_DECISION   = 2,  // Movement and positioning decisions
    RESOURCE_MANAGEMENT = 3,  // Resource usage decisions (mana, energy, etc.)
    COOLDOWN_USAGE      = 4,  // Cooldown and ability timing decisions
    DEFENSIVE_ACTIONS   = 5,  // Defensive ability usage
    BUFF_MANAGEMENT     = 6,  // Buff application and maintenance
    CROWD_CONTROL       = 7,  // CC usage and breaking decisions
    GROUP_COORDINATION  = 8,  // Group/raid coordination decisions
    QUEST_OBJECTIVES    = 9,  // Quest completion decisions
    INVENTORY_MGMT      = 10, // Inventory and equipment decisions
    SOCIAL_INTERACTION  = 11, // Guild, chat, and social decisions
    ERROR_RECOVERY      = 12, // Error handling and recovery decisions
    LEARNING_ADAPTATION = 13, // Learning and adaptation decisions
    UNKNOWN             = 14  // Uncategorized decisions
};

// Decision complexity levels
enum class DecisionComplexity : uint8
{
    TRIVIAL    = 0,  // Simple binary decisions
    SIMPLE     = 1,  // Basic decisions with few options
    MODERATE   = 2,  // Moderate complexity with multiple factors
    COMPLEX    = 3,  // Complex decisions with many variables
    CRITICAL   = 4   // Critical decisions requiring extensive analysis
};

// Decision outcome tracking
enum class DecisionOutcome : uint8
{
    SUCCESS         = 0,  // Decision executed successfully
    PARTIAL_SUCCESS = 1,  // Decision partially successful
    FAILURE         = 2,  // Decision failed to execute
    TIMEOUT         = 3,  // Decision timed out
    CANCELLED       = 4,  // Decision was cancelled
    ERROR_OUTCOME   = 5   // Error occurred during decision
};

// Individual AI decision record
struct AIDecisionRecord
{
    uint64_t timestamp;              // Microseconds since epoch
    uint32_t botGuid;
    AIDecisionCategory category;
    DecisionComplexity complexity;
    uint64_t decisionTime;           // Microseconds taken to make decision
    uint64_t executionTime;          // Microseconds taken to execute decision
    DecisionOutcome outcome;
    ::std::string context;             // Additional context information
    ::std::string stackTrace;          // Call stack for debugging
    uint32_t sequenceNumber;         // Decision sequence in bot's timeline

    AIDecisionRecord() : timestamp(0), botGuid(0), category(AIDecisionCategory::UNKNOWN),
        complexity(DecisionComplexity::SIMPLE), decisionTime(0), executionTime(0),
        outcome(DecisionOutcome::SUCCESS), sequenceNumber(0) {}

    AIDecisionRecord(uint32_t guid, AIDecisionCategory cat, DecisionComplexity comp, const ::std::string& ctx)
        : botGuid(guid), category(cat), complexity(comp), context(ctx), outcome(DecisionOutcome::SUCCESS)
    {
        timestamp = ::std::chrono::duration_cast<::std::chrono::microseconds>(
            ::std::chrono::steady_clock::now().time_since_epoch()).count();
        decisionTime = 0;
        executionTime = 0;
        sequenceNumber = 0;
    }
};

// Category-specific performance statistics
struct CategoryPerformanceStats
{
    ::std::atomic<uint64_t> totalDecisions{0};
    ::std::atomic<uint64_t> successfulDecisions{0};
    ::std::atomic<uint64_t> totalDecisionTime{0};      // Microseconds
    ::std::atomic<uint64_t> totalExecutionTime{0};     // Microseconds
    ::std::atomic<uint64_t> minDecisionTime{UINT64_MAX};
    ::std::atomic<uint64_t> maxDecisionTime{0};
    ::std::atomic<uint64_t> lastDecisionTime{0};
    ::std::atomic<double> averageDecisionTime{0.0};
    ::std::atomic<double> successRate{0.0};
    ::std::atomic<uint32_t> timeoutCount{0};
    ::std::atomic<uint32_t> errorCount{0};

    // Performance trend tracking
    ::std::array<::std::atomic<double>, 60> minutelyTrend;    // Last 60 minutes
    ::std::array<::std::atomic<double>, 24> hourlyTrend;      // Last 24 hours

    CategoryPerformanceStats()
    {
        for (auto& trend : minutelyTrend)
            trend.store(0.0);
        for (auto& trend : hourlyTrend)
            trend.store(0.0);
    }

    void UpdateStats(const AIDecisionRecord& record);
    void UpdateTrends();
    double GetEfficiencyScore() const;
    double GetReliabilityScore() const;
};

// Bot-specific AI decision profile
struct BotAIDecisionProfile
{
    uint32_t botGuid;
    uint8_t botClass;
    uint8_t botLevel;
    uint8_t botSpecialization;

    // Category-specific statistics
    ::std::array<CategoryPerformanceStats, 15> categoryStats;

    // Overall performance metrics
    ::std::atomic<uint64_t> totalDecisionsMade{0};
    ::std::atomic<uint64_t> totalDecisionTime{0};
    ::std::atomic<uint64_t> totalExecutionTime{0};
    ::std::atomic<double> overallSuccessRate{0.0};
    ::std::atomic<double> averageDecisionSpeed{0.0};      // Decisions per second
    ::std::atomic<double> decisionEfficiencyScore{0.0};   // 0.0 to 100.0
    ::std::atomic<double> adaptabilityScore{0.0};         // How well bot adapts to situations

    // Decision pattern analysis
    ::std::atomic<uint32_t> repetitiveDecisionCount{0};
    ::std::atomic<uint32_t> innovativeDecisionCount{0};
    ::std::atomic<uint32_t> contextualDecisionCount{0};

    // Learning and improvement tracking
    ::std::atomic<double> learningRate{0.0};              // Rate of improvement
    ::std::atomic<uint32_t> strategicDecisions{0};        // High-level strategic decisions
    ::std::atomic<uint32_t> reactiveDecisions{0};         // Reactive/emergency decisions

    // Performance degradation tracking
    ::std::atomic<uint32_t> performanceFlags{0};          // Bit flags for various issues
    ::std::atomic<uint64_t> lastPerformanceUpdate{0};

    BotAIDecisionProfile() : botGuid(0), botClass(0), botLevel(0), botSpecialization(0) {}
    explicit BotAIDecisionProfile(uint32_t guid, uint8_t cls, uint8_t lvl, uint8_t spec)
        : botGuid(guid), botClass(cls), botLevel(lvl), botSpecialization(spec) {}

    void UpdateOverallMetrics();
    double CalculateOverallScore() const;
    void AnalyzeDecisionPatterns();
    bool IsPerformanceDegrading() const;
};

// Decision timing profiler for individual decisions
class TC_GAME_API AIDecisionTimer
{
public:
    AIDecisionTimer(uint32_t botGuid, AIDecisionCategory category, DecisionComplexity complexity,
                   const ::std::string& context);
    ~AIDecisionTimer();

    // Disable copy/move
    AIDecisionTimer(const AIDecisionTimer&) = delete;
    AIDecisionTimer& operator=(const AIDecisionTimer&) = delete;
    AIDecisionTimer(AIDecisionTimer&&) = delete;
    AIDecisionTimer& operator=(AIDecisionTimer&&) = delete;

    // Decision lifecycle methods
    void StartExecution();
    void SetOutcome(DecisionOutcome outcome);
    void AddContext(const ::std::string& additionalContext);
    void RecordError(const ::std::string& errorMessage);

    // Timing information
    uint64_t GetDecisionTime() const;
    uint64_t GetExecutionTime() const;
    uint64_t GetTotalTime() const;

private:
    void RecordDecision();

    AIDecisionRecord _record;
    PerformanceTimer _decisionTimer;
    PerformanceTimer _executionTimer;
    bool _executionStarted;
    bool _recorded;
};

// AI Decision Analysis Engine
class TC_GAME_API AIDecisionProfiler
{
public:
    static AIDecisionProfiler& Instance()
    {
        static AIDecisionProfiler instance;
        return instance;
    }

    // Initialization and shutdown
    bool Initialize();
    void Shutdown();
    bool IsEnabled() const { return _enabled.load(); }

    // Bot registration and lifecycle
    void RegisterBot(uint32_t botGuid, uint8_t botClass, uint8_t botLevel, uint8_t specialization);
    void UnregisterBot(uint32_t botGuid);
    void UpdateBotSpecialization(uint32_t botGuid, uint8_t newSpecialization);
    void UpdateBotLevel(uint32_t botGuid, uint8_t newLevel);

    // Decision recording and analysis
    void RecordDecision(const AIDecisionRecord& record);
    void AnalyzeBotDecisionPerformance(uint32_t botGuid);
    void UpdateDecisionTrends();

    // Performance analysis
    BotAIDecisionProfile GetBotProfile(uint32_t botGuid) const;
    ::std::vector<uint32_t> GetSlowDecisionBots(uint32_t count = 10) const;
    ::std::vector<uint32_t> GetEfficientDecisionBots(uint32_t count = 10) const;
    ::std::vector<uint32_t> GetBotsWithErrors(uint32_t count = 10) const;

    // Category analysis
    CategoryPerformanceStats GetCategoryStats(AIDecisionCategory category) const;
    CategoryPerformanceStats GetBotCategoryStats(uint32_t botGuid, AIDecisionCategory category) const;
    ::std::vector<AIDecisionCategory> GetProblematicCategories(uint32_t botGuid) const;

    // Comparative analysis
    double CompareBotPerformance(uint32_t botGuid1, uint32_t botGuid2) const;
    ::std::vector<::std::pair<uint32_t, double>> GetDecisionEfficiencyRankings() const;
    void AnalyzeClassDecisionPatterns(uint8_t classId);

    // Optimization suggestions
    ::std::vector<::std::string> GetOptimizationSuggestions(uint32_t botGuid) const;
    ::std::vector<::std::string> GetCategoryOptimizationSuggestions(AIDecisionCategory category) const;
    void GeneratePerformanceInsights(uint32_t botGuid, ::std::string& insights) const;

    // Real-time monitoring
    void StartRealTimeMonitoring();
    void StopRealTimeMonitoring();
    ::std::vector<::std::pair<uint32_t, double>> GetRealTimeDecisionRates() const;
    void SetAlertThresholds(double maxDecisionTime, double minSuccessRate);

    // Decision pattern analysis
    void AnalyzeDecisionPatterns();
    void DetectAnomalousDecisions();
    void IdentifyOptimizationOpportunities();

    // Reporting and visualization
    void GenerateDecisionReport(::std::string& report, uint32_t botGuid = 0) const;
    void GenerateCategoryReport(::std::string& report, AIDecisionCategory category) const;
    void GenerateComparativeReport(::std::string& report, const ::std::vector<uint32_t>& botGuids) const;

    // Learning and adaptation analysis
    void AnalyzeLearningProgression(uint32_t botGuid);
    void TrackAdaptationEffectiveness();
    void MeasureDecisionQuality();

    // Configuration
    void SetEnabled(bool enabled) { _enabled.store(enabled); }
    void SetMaxRecordHistory(uint32_t maxRecords) { _maxRecordHistory = maxRecords; }
    void SetAnalysisInterval(uint64_t intervalMicroseconds) { _analysisInterval = intervalMicroseconds; }

    // Data management
    void FlushOldRecords(uint64_t olderThanMicroseconds);
    void ExportDecisionData(const ::std::string& filename) const;
    void ImportDecisionData(const ::std::string& filename);

private:
    AIDecisionProfiler() = default;
    ~AIDecisionProfiler() = default;

    // Internal analysis methods
    void AnalyzeDecisionSpeed(uint32_t botGuid, BotAIDecisionProfile& profile);
    void AnalyzeDecisionAccuracy(uint32_t botGuid, BotAIDecisionProfile& profile);
    void AnalyzeDecisionComplexity(uint32_t botGuid, BotAIDecisionProfile& profile);
    void AnalyzeDecisionAdaptation(uint32_t botGuid, BotAIDecisionProfile& profile);

    // Statistical analysis
    void CalculateDecisionStatistics();
    void UpdatePerformanceTrends();
    void DetectPerformanceAnomalies();

    // Background processing
    void ProcessDecisionRecords();
    void PerformPeriodicAnalysis();
    void UpdateRealTimeMetrics();

    // Data persistence
    void SaveBotProfile(const BotAIDecisionProfile& profile);
    void LoadBotProfile(uint32_t botGuid);
    void ArchiveDecisionRecords();

    // Alert system
    void CheckPerformanceAlerts();
    void GenerateAlert(uint32_t botGuid, const ::std::string& alertType, const ::std::string& message);

    // Configuration
    ::std::atomic<bool> _enabled{false};
    ::std::atomic<bool> _shutdownRequested{false};
    ::std::atomic<bool> _realTimeMonitoring{false};

    // Data storage
    mutable ::std::recursive_mutex _profilesMutex;
    ::std::unordered_map<uint32_t, BotAIDecisionProfile> _botProfiles;

    mutable ::std::recursive_mutex _recordsMutex;
    ::std::queue<AIDecisionRecord> _pendingRecords;
    ::std::vector<AIDecisionRecord> _recentRecords;

    mutable ::std::recursive_mutex _globalStatsMutex;
    ::std::array<CategoryPerformanceStats, 15> _globalCategoryStats;

    // Background processing
    ::std::thread _analysisThread;
    ::std::condition_variable _analysisCondition;
    ::std::recursive_mutex _analysisMutex;

    ::std::thread _realTimeThread;
    mutable ::std::recursive_mutex _realTimeDataMutex;
    ::std::vector<::std::pair<uint32_t, double>> _realTimeData;

    // Configuration
    ::std::atomic<uint32_t> _maxRecordHistory{100000};
    ::std::atomic<uint64_t> _analysisInterval{60000000}; // 60 seconds
    ::std::atomic<double> _alertMaxDecisionTime{50000.0}; // 50ms
    ::std::atomic<double> _alertMinSuccessRate{0.8}; // 80%

    // Performance tracking
    ::std::atomic<uint64_t> _totalDecisionsProcessed{0};
    ::std::atomic<uint64_t> _lastAnalysisTime{0};
    ::std::atomic<uint64_t> _lastArchiveTime{0};

    // Constants
    static constexpr uint64_t DEFAULT_ANALYSIS_INTERVAL_US = 60000000;  // 60 seconds
    static constexpr uint64_t ARCHIVE_INTERVAL_US = 3600000000;         // 1 hour
    static constexpr uint64_t RECORD_RETENTION_US = 86400000000;        // 24 hours
    static constexpr uint32_t MAX_REAL_TIME_ENTRIES = 1000;
    static constexpr double ANOMALY_THRESHOLD = 3.0; // 3 standard deviations
};

// Convenience macros for AI decision profiling
#define sAIProfiler AIDecisionProfiler::Instance()

#define PROFILE_AI_DECISION(botGuid, category, complexity, context) \
    AIDecisionTimer _ai_timer(botGuid, category, complexity, context)

#define PROFILE_COMBAT_DECISION(botGuid, context) \
    PROFILE_AI_DECISION(botGuid, AIDecisionCategory::COMBAT_ROTATION, DecisionComplexity::MODERATE, context)

#define PROFILE_TARGET_DECISION(botGuid, context) \
    PROFILE_AI_DECISION(botGuid, AIDecisionCategory::TARGET_SELECTION, DecisionComplexity::SIMPLE, context)

#define PROFILE_MOVEMENT_DECISION(botGuid, context) \
    PROFILE_AI_DECISION(botGuid, AIDecisionCategory::MOVEMENT_DECISION, DecisionComplexity::COMPLEX, context)

#define START_DECISION_EXECUTION() \
    _ai_timer.StartExecution()

#define SET_DECISION_OUTCOME(outcome) \
    _ai_timer.SetOutcome(outcome)

#define RECORD_DECISION_ERROR(error) \
    _ai_timer.RecordError(error)

} // namespace Playerbot