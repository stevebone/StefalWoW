/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BotPerformanceAnalytics.h"
#include "Log.h"
#include "Util.h"
#include <algorithm>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <cmath>

namespace Playerbot
{

// BotPerformanceAnalytics Implementation
bool BotPerformanceAnalytics::Initialize()
{
    TC_LOG_INFO("playerbot", "Initializing Bot Performance Analytics...");

    // Load existing data
    LoadSystemAnalytics();

    // Start background processing
    _analyticsThread = ::std::thread([this] { ProcessAnalytics(); });

    _enabled.store(true);

    TC_LOG_INFO("playerbot", "Bot Performance Analytics initialized successfully");
    return true;
}

void BotPerformanceAnalytics::Shutdown()
{
    TC_LOG_INFO("playerbot", "Shutting down Bot Performance Analytics...");

    _enabled.store(false);
    _shutdownRequested.store(true);

    // Stop real-time monitoring
    StopRealTimeMonitoring();

    // Stop background processing
    _analyticsCondition.notify_all();
    if (_analyticsThread.joinable())
        _analyticsThread.join();

    // Save current data
    SaveSystemAnalytics();
    for (const auto& [botGuid, profile] : _botProfiles)
        SaveBotProfile(profile);

    TC_LOG_INFO("playerbot", "Bot Performance Analytics shut down successfully");
}

void BotPerformanceAnalytics::RegisterBot(uint32_t botGuid, uint8_t botClass, uint8_t botLevel, uint8_t specialization)
{
    ::std::lock_guard lock(_profilesMutex);

    BotPerformanceProfile profile;
    profile.botGuid = botGuid;
    profile.botClass = botClass;
    profile.botLevel = botLevel;
    profile.botSpecialization = specialization;
    profile.overallProfile = PerformanceProfile::AVERAGE;
    profile.performanceScore = 50.0;

    _botProfiles[botGuid] = profile;

    // Update system analytics
    {
        ::std::lock_guard systemLock(_systemAnalyticsMutex);
        _systemAnalytics.concurrentBotsCount++;
        _systemAnalytics.classBotCount[botClass]++;
    }

    TC_LOG_DEBUG("playerbot", "Registered bot {} (Class: {}, Level: {}, Spec: {}) for performance analytics",
                botGuid, botClass, botLevel, specialization);
}

void BotPerformanceAnalytics::UnregisterBot(uint32_t botGuid)
{
    ::std::lock_guard lock(_profilesMutex);

    auto it = _botProfiles.find(botGuid);
    if (it != _botProfiles.end())
    {
        // Save final profile before removal
        SaveBotProfile(it->second);

        // Update system analytics
        {
            ::std::lock_guard systemLock(_systemAnalyticsMutex);
            _systemAnalytics.concurrentBotsCount--;
            if (_systemAnalytics.classBotCount[it->second.botClass] > 0)
                _systemAnalytics.classBotCount[it->second.botClass]--;
        }

        _botProfiles.erase(it);

        TC_LOG_DEBUG("playerbot", "Unregistered bot {} from performance analytics", botGuid);
    }
}

void BotPerformanceAnalytics::UpdateBotLevel(uint32_t botGuid, uint8_t newLevel)
{
    ::std::lock_guard lock(_profilesMutex);
    auto it = _botProfiles.find(botGuid);
    if (it != _botProfiles.end())
    {
        it->second.botLevel = newLevel;
        RecordAdaptationEvent(botGuid, "level_up");
    }
}

void BotPerformanceAnalytics::UpdateBotSpecialization(uint32_t botGuid, uint8_t newSpecialization)
{
    ::std::lock_guard lock(_profilesMutex);
    auto it = _botProfiles.find(botGuid);
    if (it != _botProfiles.end())
    {
        it->second.botSpecialization = newSpecialization;
        RecordAdaptationEvent(botGuid, "specialization_change");
    }
}

void BotPerformanceAnalytics::AnalyzeBotPerformance(uint32_t botGuid)
{
    if (!_enabled.load())
        return;

    ::std::lock_guard lock(_profilesMutex);
    auto it = _botProfiles.find(botGuid);
    if (it == _botProfiles.end())
        return;

    BotPerformanceProfile& profile = it->second;

    // Analyze different aspects of bot performance
    AnalyzeCombatEfficiency(botGuid, profile);
    AnalyzeResourceManagement(botGuid, profile);
    AnalyzeMovementOptimization(botGuid, profile);
    AnalyzeDecisionSpeed(botGuid, profile);
    AnalyzeSpecializationUsage(botGuid, profile);

    // Calculate overall performance
    profile.performanceScore = CalculateOverallPerformance(botGuid);
    profile.overallProfile = DeterminePerformanceProfile(profile.performanceScore);

    // Update trends
    UpdatePerformanceTrends(botGuid);
}

void BotPerformanceAnalytics::UpdateBehaviorScore(uint32_t botGuid, BotBehaviorCategory category, double score)
{
    if (!_enabled.load())
        return;

    ::std::lock_guard lock(_profilesMutex);
    auto it = _botProfiles.find(botGuid);
    if (it != _botProfiles.end() && static_cast<size_t>(category) < it->second.behaviorScores.size())
    {
        // Apply exponential moving average to smooth score changes
        double& currentScore = it->second.behaviorScores[static_cast<size_t>(category)];
        currentScore = currentScore * 0.8 + score * 0.2;

        // Clamp score to valid range
        currentScore = ::std::max(0.0, ::std::min(100.0, currentScore));
    }
}

void BotPerformanceAnalytics::RecordPerformanceEvent(uint32_t botGuid, const ::std::string& eventType, double value)
{
    if (!_enabled.load())
        return;

    // Map event types to behavior categories and update scores
    if (eventType == "combat_dps")
    {
        double normalizedScore = NormalizeScore(value, 0, 10000); // Assuming max DPS of 10k
        UpdateBehaviorScore(botGuid, BotBehaviorCategory::COMBAT_EFFICIENCY, normalizedScore);
    }
    else if (eventType == "healing_hps")
    {
        double normalizedScore = NormalizeScore(value, 0, 15000); // Assuming max HPS of 15k
        UpdateBehaviorScore(botGuid, BotBehaviorCategory::COMBAT_EFFICIENCY, normalizedScore);
    }
    else if (eventType == "resource_efficiency")
    {
        UpdateBehaviorScore(botGuid, BotBehaviorCategory::RESOURCE_MANAGEMENT, value);
    }
    else if (eventType == "movement_distance")
    {
        double efficiency = ::std::max(0.0, 100.0 - value); // Lower distance = higher efficiency
        UpdateBehaviorScore(botGuid, BotBehaviorCategory::MOVEMENT_OPTIMIZATION, efficiency);
    }
    else if (eventType == "decision_time")
    {
        double efficiency = NormalizeScore(50000 - value, 0, 50000); // Faster decisions = higher score
        UpdateBehaviorScore(botGuid, BotBehaviorCategory::DECISION_SPEED, efficiency);
    }
}

double BotPerformanceAnalytics::CalculateOverallPerformance(uint32_t botGuid)
{
    ::std::lock_guard lock(_profilesMutex);
    auto it = _botProfiles.find(botGuid);
    if (it == _botProfiles.end())
        return 50.0;

    const BotPerformanceProfile& profile = it->second;
    return CalculateWeightedScore(profile.behaviorScores);
}

PerformanceProfile BotPerformanceAnalytics::DeterminePerformanceProfile(double score)
{
    if (score >= 90.0)
        return PerformanceProfile::EXCELLENT;
    else if (score >= 75.0)
        return PerformanceProfile::GOOD;
    else if (score >= 40.0)
        return PerformanceProfile::AVERAGE;
    else if (score >= 25.0)
        return PerformanceProfile::POOR;
    else
        return PerformanceProfile::CRITICAL;
}

void BotPerformanceAnalytics::UpdatePerformanceTrends(uint32_t botGuid)
{
    ::std::lock_guard lock(_profilesMutex);
    auto it = _botProfiles.find(botGuid);
    if (it == _botProfiles.end())
        return;

    BotPerformanceProfile& profile = it->second;

    // Update hourly performance trend
    auto now = ::std::chrono::system_clock::now();
    auto time_t = ::std::chrono::system_clock::to_time_t(now);
    auto* tm = ::std::localtime(&time_t);

    if (tm)
    {
        size_t hour = static_cast<size_t>(tm->tm_hour);
        if (hour < profile.hourlyPerformance.size())
        {
            // Exponential moving average for hourly trends
            profile.hourlyPerformance[hour] = profile.hourlyPerformance[hour] * 0.9 + profile.performanceScore * 0.1;
        }

        size_t day = static_cast<size_t>(tm->tm_wday);
        if (day < profile.dailyPerformance.size())
        {
            // Exponential moving average for daily trends
            profile.dailyPerformance[day] = profile.dailyPerformance[day] * 0.95 + profile.performanceScore * 0.05;
        }
    }
}

void BotPerformanceAnalytics::UpdateSystemAnalytics()
{
    if (!_enabled.load())
        return;

    ::std::lock_guard lock(_systemAnalyticsMutex);

    // Reset distribution counts
    _systemAnalytics.performanceDistribution.fill(0);

    // Calculate performance distribution
    double totalScore = 0.0;
    uint32_t totalBots = 0;

    {
        ::std::lock_guard profilesLock(_profilesMutex);
        for (const auto& [botGuid, profile] : _botProfiles)
        {
            _systemAnalytics.performanceDistribution[static_cast<size_t>(profile.overallProfile)]++;
            totalScore += profile.performanceScore;
            totalBots++;
        }
    }

    // Update system metrics
    if (totalBots > 0)
    {
        double averageScore = totalScore / totalBots;
        _systemAnalytics.performanceTrend.push_back(averageScore);
        _systemAnalytics.trendTimestamps.push_back(
            ::std::chrono::duration_cast<::std::chrono::microseconds>(
                ::std::chrono::steady_clock::now().time_since_epoch()).count());

        // Keep only recent trend data
    while (_systemAnalytics.performanceTrend.size() > 100)
        {
            _systemAnalytics.performanceTrend.erase(_systemAnalytics.performanceTrend.begin());
            _systemAnalytics.trendTimestamps.erase(_systemAnalytics.trendTimestamps.begin());
        }
    }

    // Update memory and CPU usage from performance monitor
    _systemAnalytics.totalMemoryUsage = sPerfMonitor.GetTotalMemoryUsage();
    _systemAnalytics.averageCpuUsage = sPerfMonitor.GetSystemCpuUsage();

    if (_systemAnalytics.totalMemoryUsage > _systemAnalytics.peakMemoryUsage)
        _systemAnalytics.peakMemoryUsage = _systemAnalytics.totalMemoryUsage;

    if (_systemAnalytics.averageCpuUsage > _systemAnalytics.peakCpuUsage)
        _systemAnalytics.peakCpuUsage = _systemAnalytics.averageCpuUsage;
}

SystemPerformanceAnalytics BotPerformanceAnalytics::GetSystemAnalytics() const
{
    ::std::lock_guard lock(_systemAnalyticsMutex);
    return _systemAnalytics;
}

void BotPerformanceAnalytics::AnalyzeClassPerformance()
{
    ::std::lock_guard profilesLock(_profilesMutex);
    ::std::lock_guard systemLock(_systemAnalyticsMutex);

    // Clear existing class performance data
    _systemAnalytics.classPerformanceAverage.clear();

    // Calculate average performance by class
    ::std::unordered_map<uint8_t, double> classTotalScore;
    ::std::unordered_map<uint8_t, uint32_t> classCount;

    for (const auto& [botGuid, profile] : _botProfiles)
    {
        classTotalScore[profile.botClass] += profile.performanceScore;
        classCount[profile.botClass]++;
    }

    for (const auto& [classId, totalScore] : classTotalScore)
    {
        if (classCount[classId] > 0)
        {
            _systemAnalytics.classPerformanceAverage[classId] = totalScore / classCount[classId];
        }
    }
}

void BotPerformanceAnalytics::AnalyzeSpecializationPerformance()
{
    ::std::lock_guard profilesLock(_profilesMutex);
    ::std::lock_guard systemLock(_systemAnalyticsMutex);

    // Clear existing specialization performance data
    _systemAnalytics.specializationPerformance.clear();

    // Calculate average performance by class and specialization
    ::std::unordered_map<uint8_t, ::std::unordered_map<uint8_t, double>> specTotalScore;
    ::std::unordered_map<uint8_t, ::std::unordered_map<uint8_t, uint32_t>> specCount;

    for (const auto& [botGuid, profile] : _botProfiles)
    {
        specTotalScore[profile.botClass][profile.botSpecialization] += profile.performanceScore;
        specCount[profile.botClass][profile.botSpecialization]++;
    }

    for (const auto& [classId, specs] : specTotalScore)
    {
        for (const auto& [specId, totalScore] : specs)
        {
            if (specCount[classId][specId] > 0)
            {
                _systemAnalytics.specializationPerformance[classId][specId] =
                    totalScore / specCount[classId][specId];
            }
        }
    }
}

BotPerformanceProfile BotPerformanceAnalytics::GetBotProfile(uint32_t botGuid) const
{
    ::std::lock_guard lock(_profilesMutex);
    auto it = _botProfiles.find(botGuid);
    return it != _botProfiles.end() ? it->second : BotPerformanceProfile();
}

::std::vector<BotPerformanceProfile> BotPerformanceAnalytics::GetTopPerformers(uint32_t count) const
{
    ::std::lock_guard lock(_profilesMutex);

    ::std::vector<BotPerformanceProfile> profiles;
    profiles.reserve(_botProfiles.size());

    for (const auto& [botGuid, profile] : _botProfiles)
        profiles.push_back(profile);

    // Sort by performance score (descending)
    ::std::sort(profiles.begin(), profiles.end(),
             [](const BotPerformanceProfile& a, const BotPerformanceProfile& b) {
                 return a.performanceScore > b.performanceScore;
             });

    // Return top performers
    if (profiles.size() > count)
        profiles.resize(count);

    return profiles;
}

::std::vector<BotPerformanceProfile> BotPerformanceAnalytics::GetPoorPerformers(uint32_t count) const
{
    ::std::lock_guard lock(_profilesMutex);

    ::std::vector<BotPerformanceProfile> profiles;
    profiles.reserve(_botProfiles.size());

    for (const auto& [botGuid, profile] : _botProfiles)
        profiles.push_back(profile);

    // Sort by performance score (ascending)
    ::std::sort(profiles.begin(), profiles.end(),
             [](const BotPerformanceProfile& a, const BotPerformanceProfile& b) {
                 return a.performanceScore < b.performanceScore;
             });

    // Return poor performers
    if (profiles.size() > count)
        profiles.resize(count);

    return profiles;
}

::std::vector<uint32_t> BotPerformanceAnalytics::GetBotsInPerformanceRange(PerformanceProfile minProfile, PerformanceProfile maxProfile) const
{
    ::std::lock_guard lock(_profilesMutex);

    ::std::vector<uint32_t> botGuids;

    for (const auto& [botGuid, profile] : _botProfiles)
    {
        if (profile.overallProfile >= minProfile && profile.overallProfile <= maxProfile)
            botGuids.push_back(botGuid);
    }

    return botGuids;
}

::std::vector<::std::string> BotPerformanceAnalytics::GetOptimizationSuggestions(uint32_t botGuid) const
{
    ::std::lock_guard lock(_profilesMutex);
    auto it = _botProfiles.find(botGuid);
    if (it == _botProfiles.end())
        return {};

    const BotPerformanceProfile& profile = it->second;
    ::std::vector<::std::string> suggestions;

    // Analyze weak areas and provide suggestions
    if (profile.behaviorScores[static_cast<size_t>(BotBehaviorCategory::COMBAT_EFFICIENCY)] < 60.0)
    {
        suggestions.push_back("Improve combat rotation efficiency and ability usage timing");
    }

    if (profile.behaviorScores[static_cast<size_t>(BotBehaviorCategory::RESOURCE_MANAGEMENT)] < 60.0)
    {
        suggestions.push_back("Optimize resource management - avoid resource waste and improve regeneration");
    }

    if (profile.behaviorScores[static_cast<size_t>(BotBehaviorCategory::MOVEMENT_OPTIMIZATION)] < 60.0)
    {
        suggestions.push_back("Reduce unnecessary movement and improve positioning efficiency");
    }

    if (profile.behaviorScores[static_cast<size_t>(BotBehaviorCategory::DECISION_SPEED)] < 60.0)
    {
        suggestions.push_back("Optimize AI decision-making speed to reduce response time");
    }

    if (profile.behaviorScores[static_cast<size_t>(BotBehaviorCategory::SPECIALIZATION_USAGE)] < 60.0)
    {
        suggestions.push_back("Improve specialization-specific ability usage and rotation optimization");
    }

    if (suggestions.empty())
    {
        suggestions.push_back("Performance is good - continue current optimization strategies");
    }

    return suggestions;
}

void BotPerformanceAnalytics::GeneratePerformanceReport(::std::string& report, uint32_t botGuid) const
{
    ::std::ostringstream oss;

    if (botGuid == 0)
    {
        // System-wide report
        oss << "=== Bot Performance Analytics Report ===\n";
        oss << "Generated at: " << TimeToTimestampStr(time(nullptr)) << "\n\n";

        SystemPerformanceAnalytics systemAnalytics = GetSystemAnalytics();

        oss << "System Overview:\n";
        oss << "- Active Bots: " << systemAnalytics.concurrentBotsCount << "\n";
        oss << "- Total Memory Usage: " << systemAnalytics.totalMemoryUsage / (1024 * 1024) << " MB\n";
        oss << "- Peak Memory Usage: " << systemAnalytics.peakMemoryUsage / (1024 * 1024) << " MB\n";
        oss << "- Average CPU Usage: " << ::std::fixed << ::std::setprecision(2) << systemAnalytics.averageCpuUsage << "%\n";
        oss << "- Peak CPU Usage: " << ::std::fixed << ::std::setprecision(2) << systemAnalytics.peakCpuUsage << "%\n\n";

        oss << "Performance Distribution:\n";
        const char* profileNames[] = {"Excellent", "Good", "Average", "Poor", "Critical"};
        for (size_t i = 0; i < systemAnalytics.performanceDistribution.size(); ++i)
        {
            oss << "- " << profileNames[i] << ": " << systemAnalytics.performanceDistribution[i] << " bots\n";
        }

        oss << "\nClass Performance Averages:\n";
        for (const auto& [classId, avgScore] : systemAnalytics.classPerformanceAverage)
        {
            oss << "- Class " << static_cast<int>(classId) << ": "
                << ::std::fixed << ::std::setprecision(1) << avgScore << "/100\n";
        }
    }
    else
    {
        // Individual bot report
        BotPerformanceProfile profile = GetBotProfile(botGuid);
        if (profile.botGuid == 0)
        {
            oss << "Bot " << botGuid << " not found in analytics database.\n";
        }
        else
        {
            oss << "=== Bot Performance Report ===\n";
            oss << "Bot GUID: " << profile.botGuid << "\n";
            oss << "Class: " << static_cast<int>(profile.botClass) << "\n";
            oss << "Level: " << static_cast<int>(profile.botLevel) << "\n";
            oss << "Specialization: " << static_cast<int>(profile.botSpecialization) << "\n";
            oss << "Overall Score: " << ::std::fixed << ::std::setprecision(1) << profile.performanceScore << "/100\n";
            oss << "Performance Profile: " << GetPerformanceProfileName(profile.overallProfile) << "\n\n";

            oss << "Behavior Scores:\n";
            const char* behaviorNames[] = {
                "Combat Efficiency", "Resource Management", "Movement Optimization",
                "Decision Speed", "Specialization Usage", "Group Coordination",
                "Quest Completion", "PvP Performance", "Dungeon Performance", "Adaptive Behavior"
            };

            for (size_t i = 0; i < profile.behaviorScores.size(); ++i)
            {
                oss << "- " << behaviorNames[i] << ": "
                    << ::std::fixed << ::std::setprecision(1) << profile.behaviorScores[i] << "/100\n";
            }

            oss << "\nOptimization Suggestions:\n";
            auto suggestions = GetOptimizationSuggestions(botGuid);
            for (const auto& suggestion : suggestions)
            {
                oss << "- " << suggestion << "\n";
            }
        }
    }

    report = oss.str();
}

void BotPerformanceAnalytics::RecordAdaptationEvent(uint32_t botGuid, const ::std::string& eventType)
{
    if (!_enabled.load())
        return;

    ::std::lock_guard lock(_profilesMutex);
    auto it = _botProfiles.find(botGuid);
    if (it != _botProfiles.end())
    {
        it->second.adaptationEvents++;

        // Different event types contribute differently to adaptive behavior score
        double scoreIncrease = 0.0;
        if (eventType == "level_up")
            scoreIncrease = 5.0;
        else if (eventType == "specialization_change")
            scoreIncrease = 10.0;
        else if (eventType == "strategy_adaptation")
            scoreIncrease = 3.0;
        else if (eventType == "error_recovery")
            scoreIncrease = 7.0;

        if (scoreIncrease > 0.0)
        {
            double& adaptiveScore = it->second.behaviorScores[static_cast<size_t>(BotBehaviorCategory::ADAPTIVE_BEHAVIOR)];
            adaptiveScore = ::std::min(100.0, adaptiveScore + scoreIncrease);
        }
    }
}

void BotPerformanceAnalytics::RecordError(uint32_t botGuid, const ::std::string& errorType, const ::std::string& context)
{
    if (!_enabled.load())
        return;

    ::std::lock_guard lock(_profilesMutex);
    auto it = _botProfiles.find(botGuid);
    if (it != _botProfiles.end())
    {
        it->second.totalErrors++;

        if (errorType.find("critical") != ::std::string::npos ||
            errorType.find("fatal") != ::std::string::npos)
        {
            it->second.criticalErrors++;
        }

        // Errors negatively impact performance scores
    for (auto& score : it->second.behaviorScores)
        {
            score = ::std::max(0.0, score - 1.0); // Small penalty for any error
        }
    }

    TC_LOG_DEBUG("playerbot", "Recorded error for bot {}: {} ({})", botGuid, errorType, context);
}

void BotPerformanceAnalytics::RecordErrorRecovery(uint32_t botGuid, const ::std::string& errorType, uint64_t recoveryTime)
{
    if (!_enabled.load())
        return;

    ::std::lock_guard lock(_profilesMutex);
    auto it = _botProfiles.find(botGuid);
    if (it != _botProfiles.end())
    {
        it->second.recoveredErrors++;

        // Quick recovery improves adaptive behavior score
        double recoveryBonus = ::std::max(0.0, 5.0 - static_cast<double>(recoveryTime) / 1000000.0); // Bonus for recovery under 5 seconds
        double& adaptiveScore = it->second.behaviorScores[static_cast<size_t>(BotBehaviorCategory::ADAPTIVE_BEHAVIOR)];
        adaptiveScore = ::std::min(100.0, adaptiveScore + recoveryBonus);

        RecordAdaptationEvent(botGuid, "error_recovery");
    }

    TC_LOG_DEBUG("playerbot", "Recorded error recovery for bot {}: {} ({}μs)", botGuid, errorType, recoveryTime);
}

// Internal analysis methods
void BotPerformanceAnalytics::AnalyzeCombatEfficiency(uint32_t botGuid, BotPerformanceProfile& profile)
{
    // Get combat-related metrics from performance monitor
    auto aiDecisionStats = sPerfMonitor.GetBotStatistics(botGuid, MetricType::AI_DECISION_TIME);
    auto combatRotationStats = sPerfMonitor.GetBotStatistics(botGuid, MetricType::COMBAT_ROTATION_TIME);

    if (aiDecisionStats.totalSamples.load() > 0)
    {
        double avgDecisionTime = aiDecisionStats.GetAverage();
        double efficiency = NormalizeScore(50000 - avgDecisionTime, 0, 50000); // Lower time = higher efficiency
        profile.decisionEfficiency = efficiency / 100.0;

        UpdateBehaviorScore(botGuid, BotBehaviorCategory::COMBAT_EFFICIENCY, efficiency);
    }
}

void BotPerformanceAnalytics::AnalyzeResourceManagement(uint32_t botGuid, BotPerformanceProfile& profile)
{
    auto resourceStats = sPerfMonitor.GetBotStatistics(botGuid, MetricType::RESOURCE_MANAGEMENT);

    if (resourceStats.totalSamples.load() > 0)
    {
        double avgResourceTime = resourceStats.GetAverage();
        double efficiency = NormalizeScore(10000 - avgResourceTime, 0, 10000);
        profile.resourceEfficiency = efficiency / 100.0;

        UpdateBehaviorScore(botGuid, BotBehaviorCategory::RESOURCE_MANAGEMENT, efficiency);
    }
}

void BotPerformanceAnalytics::AnalyzeMovementOptimization(uint32_t botGuid, BotPerformanceProfile& profile)
{
    auto movementStats = sPerfMonitor.GetBotStatistics(botGuid, MetricType::MOVEMENT_CALCULATION);

    if (movementStats.totalSamples.load() > 0)
    {
        double avgMovementTime = movementStats.GetAverage();
        double efficiency = NormalizeScore(5000 - avgMovementTime, 0, 5000);
        profile.movementEfficiency = efficiency / 100.0;

        UpdateBehaviorScore(botGuid, BotBehaviorCategory::MOVEMENT_OPTIMIZATION, efficiency);
    }
}

void BotPerformanceAnalytics::AnalyzeDecisionSpeed(uint32_t botGuid, BotPerformanceProfile& profile)
{
    auto decisionStats = sPerfMonitor.GetBotStatistics(botGuid, MetricType::AI_DECISION_TIME);

    if (decisionStats.totalSamples.load() > 0)
    {
        double avgDecisionTime = decisionStats.GetAverage();
        double speed = NormalizeScore(25000 - avgDecisionTime, 0, 25000); // Faster = higher score
        profile.decisionEfficiency = speed / 100.0;

        UpdateBehaviorScore(botGuid, BotBehaviorCategory::DECISION_SPEED, speed);
    }
}

void BotPerformanceAnalytics::AnalyzeSpecializationUsage(uint32_t botGuid, BotPerformanceProfile& profile)
{
    auto specializationStats = sPerfMonitor.GetBotStatistics(botGuid, MetricType::SPECIALIZATION_UPDATE);

    if (specializationStats.totalSamples.load() > 0)
    {
        double avgSpecTime = specializationStats.GetAverage();
        double efficiency = NormalizeScore(15000 - avgSpecTime, 0, 15000);

        UpdateBehaviorScore(botGuid, BotBehaviorCategory::SPECIALIZATION_USAGE, efficiency);
    }
}

// Helper methods
double BotPerformanceAnalytics::CalculateWeightedScore(const ::std::array<double, 10>& scores) const
{
    double totalScore = 0.0;
    double totalWeight = 0.0;

    for (size_t i = 0; i < scores.size() && i < _behaviorWeights.size(); ++i)
    {
        totalScore += scores[i] * _behaviorWeights[i];
        totalWeight += _behaviorWeights[i];
    }

    return totalWeight > 0.0 ? totalScore / totalWeight : 50.0;
}

double BotPerformanceAnalytics::NormalizeScore(double rawScore, double minValue, double maxValue) const
{
    if (maxValue <= minValue)
        return 50.0;

    double normalized = (rawScore - minValue) / (maxValue - minValue) * 100.0;
    return ::std::max(0.0, ::std::min(100.0, normalized));
}

double BotPerformanceAnalytics::CalculateTrendScore(const ::std::vector<double>& values) const
{
    if (values.size() < 2)
        return 0.0;

    // Calculate linear regression slope to determine trend
    double n = static_cast<double>(values.size());
    double sumX = 0.0, sumY = 0.0, sumXY = 0.0, sumX2 = 0.0;

    for (size_t i = 0; i < values.size(); ++i)
    {
        double x = static_cast<double>(i);
        double y = values[i];
        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumX2 += x * x;
    }

    double slope = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
    return slope;
}

void BotPerformanceAnalytics::ProcessAnalytics()
{
    while (!_shutdownRequested.load())
    {
        ::std::unique_lock<::std::recursive_mutex> lock(_analyticsUpdateMutex);
        _analyticsCondition.wait_for(lock, ::std::chrono::microseconds(_updateInterval),
                                   [this] { return _shutdownRequested.load(); });

        if (_shutdownRequested.load())
            break;

        // Update system analytics
        UpdateSystemAnalytics();
        AnalyzeClassPerformance();
        AnalyzeSpecializationPerformance();

        // Analyze all registered bots
        ::std::vector<uint32_t> botGuids;
        {
            ::std::lock_guard profilesLock(_profilesMutex);
            for (const auto& [botGuid, profile] : _botProfiles)
                botGuids.push_back(botGuid);
        }

        for (uint32_t botGuid : botGuids)
        {
            AnalyzeBotPerformance(botGuid);
        }
    }
}

void BotPerformanceAnalytics::SaveBotProfile(const BotPerformanceProfile& profile)
{
    // Implementation would save to database or file
    // Placeholder for persistence layer
}

void BotPerformanceAnalytics::LoadBotProfile(uint32_t botGuid)
{
    // Implementation would load from database or file
    // Placeholder for persistence layer
}

void BotPerformanceAnalytics::SaveSystemAnalytics()
{
    // Implementation would save system analytics to persistent storage
    // Placeholder for persistence layer
}

void BotPerformanceAnalytics::LoadSystemAnalytics()
{
    // Implementation would load system analytics from persistent storage
    // Placeholder for persistence layer
}

// Utility functions
::std::string GetPerformanceProfileName(PerformanceProfile profile)
{
    switch (profile)
    {
        case PerformanceProfile::EXCELLENT: return "Excellent";
        case PerformanceProfile::GOOD: return "Good";
        case PerformanceProfile::AVERAGE: return "Average";
        case PerformanceProfile::POOR: return "Poor";
        case PerformanceProfile::CRITICAL: return "Critical";
        default: return "Unknown";
    }
}

::std::string GetBehaviorCategoryName(BotBehaviorCategory category)
{
    switch (category)
    {
        case BotBehaviorCategory::COMBAT_EFFICIENCY: return "Combat Efficiency";
        case BotBehaviorCategory::RESOURCE_MANAGEMENT: return "Resource Management";
        case BotBehaviorCategory::MOVEMENT_OPTIMIZATION: return "Movement Optimization";
        case BotBehaviorCategory::DECISION_SPEED: return "Decision Speed";
        case BotBehaviorCategory::SPECIALIZATION_USAGE: return "Specialization Usage";
        case BotBehaviorCategory::GROUP_COORDINATION: return "Group Coordination";
        case BotBehaviorCategory::QUEST_COMPLETION: return "Quest Completion";
        case BotBehaviorCategory::PVP_PERFORMANCE: return "PvP Performance";
        case BotBehaviorCategory::DUNGEON_PERFORMANCE: return "Dungeon Performance";
        case BotBehaviorCategory::ADAPTIVE_BEHAVIOR: return "Adaptive Behavior";
        default: return "Unknown";
    }
}

} // namespace Playerbot