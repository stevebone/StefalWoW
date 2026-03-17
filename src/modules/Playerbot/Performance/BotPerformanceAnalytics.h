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

namespace Playerbot
{

// Performance profile categories
enum class PerformanceProfile : uint8
{
    EXCELLENT   = 0,  // Top 10% performance
    GOOD        = 1,  // Top 25% performance
    AVERAGE     = 2,  // Average performance
    POOR        = 3,  // Bottom 25% performance
    CRITICAL    = 4   // Bottom 10% performance
};

// Bot behavior analysis categories
enum class BotBehaviorCategory : uint8
{
    COMBAT_EFFICIENCY     = 0,  // How efficiently the bot handles combat
    RESOURCE_MANAGEMENT   = 1,  // Efficiency in managing resources (mana, energy, etc.)
    MOVEMENT_OPTIMIZATION = 2,  // Movement and positioning efficiency
    DECISION_SPEED        = 3,  // Speed of AI decision making
    SPECIALIZATION_USAGE  = 4,  // How well the bot uses its specialization
    GROUP_COORDINATION    = 5,  // Performance in group scenarios
    QUEST_COMPLETION      = 6,  // Efficiency in quest completion
    PVP_PERFORMANCE       = 7,  // Performance in PvP scenarios
    DUNGEON_PERFORMANCE   = 8,  // Performance in dungeon scenarios
    ADAPTIVE_BEHAVIOR     = 9   // How well the bot adapts to different situations
};

// Individual bot performance profile
struct BotPerformanceProfile
{
    uint32_t botGuid;
    uint8_t botLevel;
    uint8_t botClass;
    uint8_t botSpecialization;

    // Overall performance metrics
    PerformanceProfile overallProfile;
    double performanceScore;        // 0.0 to 100.0
    uint64_t totalPlayTime;        // Microseconds
    uint64_t totalCombatTime;      // Microseconds

    // Behavior category scores (0.0 to 100.0)
    ::std::array<double, 10> behaviorScores;

    // Performance trends
    ::std::array<double, 24> hourlyPerformance;  // Performance by hour of day
    ::std::array<double, 7> dailyPerformance;    // Performance by day of week

    // Efficiency metrics
    double dpsEfficiency;           // Damage per second efficiency
    double hpsEfficiency;          // Healing per second efficiency
    double resourceEfficiency;     // Resource usage efficiency
    double movementEfficiency;     // Movement optimization efficiency
    double decisionEfficiency;     // AI decision making efficiency

    // Experience and learning
    uint64_t totalExperience;
    uint32_t skillImprovements;
    uint32_t adaptationEvents;

    // Error tracking
    uint32_t totalErrors;
    uint32_t criticalErrors;
    uint32_t recoveredErrors;

    BotPerformanceProfile() : botGuid(0), botLevel(0), botClass(0), botSpecialization(0),
        overallProfile(PerformanceProfile::AVERAGE), performanceScore(50.0),
        totalPlayTime(0), totalCombatTime(0), dpsEfficiency(0.0), hpsEfficiency(0.0),
        resourceEfficiency(0.0), movementEfficiency(0.0), decisionEfficiency(0.0),
        totalExperience(0), skillImprovements(0), adaptationEvents(0),
        totalErrors(0), criticalErrors(0), recoveredErrors(0)
    {
        behaviorScores.fill(50.0);
        hourlyPerformance.fill(50.0);
        dailyPerformance.fill(50.0);
    }
};

// System-wide performance analytics
struct SystemPerformanceAnalytics
{
    // Bot distribution by performance
    ::std::array<uint32_t, 5> performanceDistribution; // Count by PerformanceProfile

    // Class performance analysis
    ::std::unordered_map<uint8_t, double> classPerformanceAverage;
    ::std::unordered_map<uint8_t, uint32_t> classBotCount;

    // Specialization performance analysis
    ::std::unordered_map<uint8_t, ::std::unordered_map<uint8_t, double>> specializationPerformance;

    // System load metrics
    double averageSystemLoad;
    double peakSystemLoad;
    uint32_t concurrentBotsCount;
    uint64_t totalSystemUptime;

    // Performance trends
    ::std::vector<double> performanceTrend;  // Last 100 measurements
    ::std::vector<uint64_t> trendTimestamps;

    // Resource usage analytics
    uint64_t totalMemoryUsage;
    uint64_t peakMemoryUsage;
    double averageCpuUsage;
    double peakCpuUsage;

    SystemPerformanceAnalytics()
    {
        performanceDistribution.fill(0);
        averageSystemLoad = 0.0;
        peakSystemLoad = 0.0;
        concurrentBotsCount = 0;
        totalSystemUptime = 0;
        totalMemoryUsage = 0;
        peakMemoryUsage = 0;
        averageCpuUsage = 0.0;
        peakCpuUsage = 0.0;
    }
};

// Performance comparison data
struct PerformanceComparison
{
    uint32_t botGuid;
    double currentScore;
    double previousScore;
    double improvementRate;
    ::std::vector<::std::pair<BotBehaviorCategory, double>> improvements;
    ::std::vector<::std::pair<BotBehaviorCategory, double>> regressions;
    uint64_t comparisonTimestamp;
};

// Bot Performance Analytics Engine
class TC_GAME_API BotPerformanceAnalytics
{
public:
    static BotPerformanceAnalytics& Instance()
    {
        static BotPerformanceAnalytics instance;
        return instance;
    }

    // Initialization and shutdown
    bool Initialize();
    void Shutdown();
    bool IsEnabled() const { return _enabled.load(); }

    // Bot registration and lifecycle
    void RegisterBot(uint32_t botGuid, uint8_t botClass, uint8_t botLevel, uint8_t specialization);
    void UnregisterBot(uint32_t botGuid);
    void UpdateBotLevel(uint32_t botGuid, uint8_t newLevel);
    void UpdateBotSpecialization(uint32_t botGuid, uint8_t newSpecialization);

    // Performance analysis
    void AnalyzeBotPerformance(uint32_t botGuid);
    void UpdateBehaviorScore(uint32_t botGuid, BotBehaviorCategory category, double score);
    void RecordPerformanceEvent(uint32_t botGuid, const ::std::string& eventType, double value);

    // Performance calculation
    double CalculateOverallPerformance(uint32_t botGuid);
    PerformanceProfile DeterminePerformanceProfile(double score);
    void UpdatePerformanceTrends(uint32_t botGuid);

    // System analytics
    void UpdateSystemAnalytics();
    SystemPerformanceAnalytics GetSystemAnalytics() const;
    void AnalyzeClassPerformance();
    void AnalyzeSpecializationPerformance();

    // Data retrieval
    BotPerformanceProfile GetBotProfile(uint32_t botGuid) const;
    ::std::vector<BotPerformanceProfile> GetTopPerformers(uint32_t count = 10) const;
    ::std::vector<BotPerformanceProfile> GetPoorPerformers(uint32_t count = 10) const;
    ::std::vector<uint32_t> GetBotsInPerformanceRange(PerformanceProfile minProfile, PerformanceProfile maxProfile) const;

    // Comparative analysis
    PerformanceComparison CompareBotPerformance(uint32_t botGuid, uint64_t timeWindow = 3600000000); // 1 hour in microseconds
    ::std::vector<PerformanceComparison> GetPerformanceImprovements(uint32_t count = 10) const;
    ::std::vector<PerformanceComparison> GetPerformanceRegressions(uint32_t count = 10) const;

    // Performance optimization suggestions
    ::std::vector<::std::string> GetOptimizationSuggestions(uint32_t botGuid) const;
    ::std::vector<::std::string> GetSystemOptimizationSuggestions() const;

    // Reporting
    void GeneratePerformanceReport(::std::string& report, uint32_t botGuid = 0) const;
    void GenerateSystemReport(::std::string& report) const;
    void GenerateClassAnalysisReport(::std::string& report, uint8_t classId) const;

    // Learning and adaptation
    void RecordAdaptationEvent(uint32_t botGuid, const ::std::string& eventType);
    void UpdateLearningMetrics(uint32_t botGuid, double learningRate);
    void AnalyzeLearningTrends();

    // Error tracking and analysis
    void RecordError(uint32_t botGuid, const ::std::string& errorType, const ::std::string& context);
    void RecordErrorRecovery(uint32_t botGuid, const ::std::string& errorType, uint64_t recoveryTime);
    void AnalyzeErrorPatterns();

    // Configuration and tuning
    void SetAnalyticsEnabled(bool enabled) { _enabled.store(enabled); }
    void SetUpdateInterval(uint64_t intervalMicroseconds) { _updateInterval = intervalMicroseconds; }
    void SetPerformanceWeights(const ::std::array<double, 10>& weights) { _behaviorWeights = weights; }

    // Data management
    void FlushAnalytics();
    void ArchiveOldData(uint64_t olderThanMicroseconds);
    void ExportAnalytics(const ::std::string& filename) const;
    void ImportAnalytics(const ::std::string& filename);

    // Real-time monitoring
    void StartRealTimeMonitoring();
    void StopRealTimeMonitoring();
    ::std::vector<::std::pair<uint32_t, double>> GetRealTimePerformance() const;

private:
    BotPerformanceAnalytics() = default;
    ~BotPerformanceAnalytics() = default;

    // Internal analysis methods
    void AnalyzeCombatEfficiency(uint32_t botGuid, BotPerformanceProfile& profile);
    void AnalyzeResourceManagement(uint32_t botGuid, BotPerformanceProfile& profile);
    void AnalyzeMovementOptimization(uint32_t botGuid, BotPerformanceProfile& profile);
    void AnalyzeDecisionSpeed(uint32_t botGuid, BotPerformanceProfile& profile);
    void AnalyzeSpecializationUsage(uint32_t botGuid, BotPerformanceProfile& profile);

    // Score calculation helpers
    double CalculateWeightedScore(const ::std::array<double, 10>& scores) const;
    double NormalizeScore(double rawScore, double minValue, double maxValue) const;
    double CalculateTrendScore(const ::std::vector<double>& values) const;

    // Data persistence
    void SaveBotProfile(const BotPerformanceProfile& profile);
    void LoadBotProfile(uint32_t botGuid);
    void SaveSystemAnalytics();
    void LoadSystemAnalytics();

    // Background processing
    void ProcessAnalytics();
    void UpdatePerformanceProfiles();
    void CalculateSystemMetrics();

    // Configuration
    ::std::atomic<bool> _enabled{false};
    ::std::atomic<bool> _shutdownRequested{false};
    uint64_t _updateInterval{60000000}; // 60 seconds in microseconds

    // Performance weighting for different behavior categories
    ::std::array<double, 10> _behaviorWeights{
        0.20, // COMBAT_EFFICIENCY
        0.15, // RESOURCE_MANAGEMENT
        0.10, // MOVEMENT_OPTIMIZATION
        0.15, // DECISION_SPEED
        0.15, // SPECIALIZATION_USAGE
        0.10, // GROUP_COORDINATION
        0.05, // QUEST_COMPLETION
        0.05, // PVP_PERFORMANCE
        0.05, // DUNGEON_PERFORMANCE
        0.00  // ADAPTIVE_BEHAVIOR (calculated differently)
    };

    // Data storage
    mutable ::std::recursive_mutex _profilesMutex;
    ::std::unordered_map<uint32_t, BotPerformanceProfile> _botProfiles;

    mutable ::std::recursive_mutex _systemAnalyticsMutex;
    SystemPerformanceAnalytics _systemAnalytics;

    mutable ::std::recursive_mutex _comparisonMutex;
    ::std::unordered_map<uint32_t, ::std::vector<PerformanceComparison>> _performanceHistory;

    // Background processing
    ::std::thread _analyticsThread;
    ::std::condition_variable _analyticsCondition;
    ::std::recursive_mutex _analyticsUpdateMutex;

    // Real-time monitoring
    ::std::atomic<bool> _realTimeMonitoring{false};
    ::std::thread _monitoringThread;
    mutable ::std::recursive_mutex _realTimeDataMutex;
    ::std::vector<::std::pair<uint32_t, double>> _realTimePerformanceData;

    // Constants
    static constexpr uint64_t DEFAULT_UPDATE_INTERVAL_US = 60000000;  // 60 seconds
    static constexpr uint64_t PERFORMANCE_HISTORY_RETENTION_US = 86400000000; // 24 hours
    static constexpr size_t MAX_PERFORMANCE_HISTORY_ENTRIES = 1000;
    static constexpr size_t MAX_REAL_TIME_ENTRIES = 1000;
};

// Convenience macros for analytics
#define sAnalytics BotPerformanceAnalytics::Instance()

#define RECORD_PERFORMANCE_EVENT(botGuid, eventType, value) \
    if (sAnalytics.IsEnabled()) \
        sAnalytics.RecordPerformanceEvent(botGuid, eventType, value)

#define UPDATE_BEHAVIOR_SCORE(botGuid, category, score) \
    if (sAnalytics.IsEnabled()) \
        sAnalytics.UpdateBehaviorScore(botGuid, category, score)

#define RECORD_ADAPTATION_EVENT(botGuid, eventType) \
    if (sAnalytics.IsEnabled()) \
        sAnalytics.RecordAdaptationEvent(botGuid, eventType)

#define RECORD_BOT_ERROR(botGuid, errorType, context) \
    if (sAnalytics.IsEnabled()) \
        sAnalytics.RecordError(botGuid, errorType, context)

} // namespace Playerbot