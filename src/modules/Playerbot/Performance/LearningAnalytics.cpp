/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "LearningAnalytics.h"
#include "MLPerformanceTracker.h"
#include "Log.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <fstream>
#include <sstream>

namespace Playerbot
{

// LearningAnalytics Implementation
LearningAnalytics::LearningAnalytics() : _initialized(false)
{
}

LearningAnalytics::~LearningAnalytics()
{
    Shutdown();
}

bool LearningAnalytics::Initialize()
{
    if (_initialized)
        return true;

    TC_LOG_INFO("playerbot.learning", "Initializing Learning Analytics");

    _initialized = true;
    TC_LOG_INFO("playerbot.learning", "Learning Analytics initialized successfully");
    return true;
}

void LearningAnalytics::Shutdown()
{
    if (!_initialized)
        return;

    TC_LOG_INFO("playerbot.learning", "Shutting down Learning Analytics");

    // Export final reports for all bots
    ::std::lock_guard lock(_dataMutex);
    for (const auto& [botGuid, data] : _botData)
    {
        ::std::string report = GenerateLearningReport(botGuid);
        TC_LOG_INFO("playerbot.learning", "Final report for bot %u:\n%s", botGuid, report.c_str());
    }

    _initialized = false;
}

void LearningAnalytics::RecordLearningStep(uint32_t botGuid, const LearningDataPoint& dataPoint)
{
    if (!_initialized)
        return;

    TRACK_ML_OPERATION(botGuid, MLOperationType::EXPERIENCE_STORAGE);

    ::std::lock_guard lock(_dataMutex);

    BotLearningData* data = GetOrCreateBotData(botGuid);
    if (!data)
        return;

    data->dataPoints.push_back(dataPoint);

    // Maintain buffer size
    if (data->dataPoints.size() > BotLearningData::MAX_DATA_POINTS)
        data->dataPoints.pop_front();

    // Update peak performance
    if (dataPoint.performance > data->peakPerformance)
        data->peakPerformance = dataPoint.performance;

    // Update phase detection
    data->currentPhase = DetectLearningPhase(botGuid);

    // Update global metrics
    _globalMetrics.totalLearningSteps++;
    _globalMetrics.totalExperiences += dataPoint.experienceCount;

    UpdateGlobalMetrics();
}

void LearningAnalytics::RecordEpisode(uint32_t botGuid, uint32_t episode, float totalReward, float avgLoss)
{
    if (!_initialized)
        return;

    ::std::lock_guard lock(_dataMutex);

    BotLearningData* data = GetOrCreateBotData(botGuid);
    if (!data)
        return;

    data->episodeRewards[episode] = totalReward;

    // Create data point for this episode
    LearningDataPoint point;
    point.timestamp = ::std::chrono::steady_clock::now().time_since_epoch().count();
    point.episode = episode;
    point.reward = totalReward;
    point.loss = avgLoss;
    point.performance = totalReward;  // Simple mapping, could be more complex

    data->dataPoints.push_back(point);
}

LearningAnalytics::BotLearningData* LearningAnalytics::GetOrCreateBotData(uint32_t botGuid)
{
    auto it = _botData.find(botGuid);
    if (it == _botData.end())
    {
        auto data = ::std::make_unique<BotLearningData>();
        data->currentPhase = LearningPhase::EXPLORATION;
        data->plateauStartEpisode = 0;
        data->convergenceEpisode = 0;
        data->peakPerformance = 0.0f;
        data->startTime = ::std::chrono::steady_clock::now();

        auto* ptr = data.get();
        _botData[botGuid] = ::std::move(data);
        _globalMetrics.totalBots++;
        return ptr;
    }
    return it->second.get();
}

LearningTrend LearningAnalytics::AnalyzeLearningTrend(uint32_t botGuid) const
{
    LearningTrend trend;

    ::std::lock_guard lock(_dataMutex);

    auto it = _botData.find(botGuid);
    if (it == _botData.end() || it->second->dataPoints.size() < _minDataPoints)
        return trend;

    const auto& dataPoints = it->second->dataPoints;

    // Extract performance series
    ::std::vector<float> performances = ExtractMetricSeries(dataPoints,
        [](const LearningDataPoint& p) { return p.performance; });
    ::std::vector<float> losses = ExtractMetricSeries(dataPoints,
        [](const LearningDataPoint& p) { return p.loss; });
    ::std::vector<float> rewards = ExtractMetricSeries(dataPoints,
        [](const LearningDataPoint& p) { return p.reward; });

    // Calculate slopes
    trend.performanceSlope = CalculateSlope(performances);
    trend.lossSlope = CalculateSlope(losses);
    trend.rewardTrend = CalculateSlope(rewards);

    // Calculate variance change
    size_t halfPoint = performances.size() / 2;
    if (halfPoint > 10)
    {
        ::std::vector<float> firstHalf(performances.begin(), performances.begin() + halfPoint);
        ::std::vector<float> secondHalf(performances.begin() + halfPoint, performances.end());

        float var1 = CalculateVariance(firstHalf);
        float var2 = CalculateVariance(secondHalf);
        trend.varianceChange = var2 - var1;
    }

    // Determine trend status
    trend.isImproving = trend.performanceSlope > 0.001f && trend.lossSlope < -0.001f;
    trend.isStable = ::std::abs(trend.performanceSlope) < 0.0001f &&
                     ::std::abs(trend.varianceChange) < 0.01f;
    trend.hasConverged = trend.isStable && CalculateVariance(performances) < _convergenceThreshold;

    return trend;
}

LearningPhase LearningAnalytics::DetectLearningPhase(uint32_t botGuid) const
{
    ::std::lock_guard lock(_dataMutex);

    auto it = _botData.find(botGuid);
    if (it == _botData.end() || it->second->dataPoints.empty())
        return LearningPhase::EXPLORATION;

    const auto& dataPoints = it->second->dataPoints;

    // Check exploration rate if available
    if (!dataPoints.empty())
    {
        float avgExploration = 0;
        size_t count = ::std::min<size_t>(10, dataPoints.size());
        for (size_t i = dataPoints.size() - count; i < dataPoints.size(); ++i)
        {
            avgExploration += dataPoints[i].explorationRate;
        }
        avgExploration /= count;

        if (avgExploration > 0.5f)
            return LearningPhase::EXPLORATION;
    }

    // Analyze trend
    LearningTrend trend = AnalyzeLearningTrend(botGuid);

    if (trend.hasConverged)
        return LearningPhase::CONVERGED;

    if (trend.performanceSlope < -0.001f)
        return LearningPhase::REGRESSION;

    if (trend.isStable && !trend.isImproving)
        return LearningPhase::PLATEAU;

    if (dataPoints.back().explorationRate < 0.2f)
        return LearningPhase::EXPLOITATION;

    return LearningPhase::LEARNING;
}

float LearningAnalytics::GetLearningRate(uint32_t botGuid) const
{
    LearningTrend trend = AnalyzeLearningTrend(botGuid);
    return ::std::abs(trend.performanceSlope);
}

float LearningAnalytics::GetConvergenceProgress(uint32_t botGuid) const
{
    ::std::lock_guard lock(_dataMutex);

    auto it = _botData.find(botGuid);
    if (it == _botData.end() || it->second->dataPoints.size() < _minDataPoints)
        return 0.0f;

    const auto& dataPoints = it->second->dataPoints;

    // Calculate current variance
    ::std::vector<float> recentPerformances;
    size_t count = ::std::min<size_t>(50, dataPoints.size());
    for (size_t i = dataPoints.size() - count; i < dataPoints.size(); ++i)
    {
        recentPerformances.push_back(dataPoints[i].performance);
    }

    float currentVariance = CalculateVariance(recentPerformances);

    // Progress is inverse of variance (lower variance = more converged)
    float progress = 1.0f - ::std::min(1.0f, currentVariance / 0.1f);
    return ::std::clamp(progress, 0.0f, 1.0f);
}

bool LearningAnalytics::HasConverged(uint32_t botGuid) const
{
    ::std::lock_guard lock(_dataMutex);

    auto it = _botData.find(botGuid);
    if (it == _botData.end())
        return false;

    return it->second->currentPhase == LearningPhase::CONVERGED ||
           it->second->convergenceEpisode > 0;
}

bool LearningAnalytics::IsInPlateau(uint32_t botGuid) const
{
    ::std::lock_guard lock(_dataMutex);

    auto it = _botData.find(botGuid);
    if (it == _botData.end())
        return false;

    return it->second->currentPhase == LearningPhase::PLATEAU;
}

bool LearningAnalytics::IsRegressing(uint32_t botGuid) const
{
    ::std::lock_guard lock(_dataMutex);

    auto it = _botData.find(botGuid);
    if (it == _botData.end())
        return false;

    return it->second->currentPhase == LearningPhase::REGRESSION;
}

float LearningAnalytics::GetAveragePerformance(uint32_t botGuid, uint32_t lastEpisodes) const
{
    ::std::lock_guard lock(_dataMutex);

    auto it = _botData.find(botGuid);
    if (it == _botData.end() || it->second->dataPoints.empty())
        return 0.0f;

    const auto& dataPoints = it->second->dataPoints;

    float sum = 0.0f;
    uint32_t count = 0;

    // Get performance from last N episodes
    for (auto rit = dataPoints.rbegin(); rit != dataPoints.rend() && count < lastEpisodes; ++rit)
    {
        sum += rit->performance;
        count++;
    }

    return count > 0 ? sum / count : 0.0f;
}

float LearningAnalytics::GetSampleEfficiency(uint32_t botGuid) const
{
    ::std::lock_guard lock(_dataMutex);

    auto it = _botData.find(botGuid);
    if (it == _botData.end() || it->second->dataPoints.empty())
        return 0.0f;

    const auto& data = it->second;
    float currentPerformance = GetAveragePerformance(botGuid, 10);

    uint64_t totalExperiences = 0;
    for (const auto& point : data->dataPoints)
        totalExperiences += point.experienceCount;

    return totalExperiences > 0 ? currentPerformance / (totalExperiences / 1000.0f) : 0.0f;
}

ModelComparison LearningAnalytics::CompareModels(uint32_t botGuidA, uint32_t botGuidB) const
{
    ModelComparison comparison;
    comparison.modelA = "Bot_" + ::std::to_string(botGuidA);
    comparison.modelB = "Bot_" + ::std::to_string(botGuidB);

    float perfA = GetAveragePerformance(botGuidA);
    float perfB = GetAveragePerformance(botGuidB);
    comparison.performanceDelta = perfA - perfB;

    float effA = GetSampleEfficiency(botGuidA);
    float effB = GetSampleEfficiency(botGuidB);
    comparison.efficiencyDelta = effA - effB;

    float varA = GetPerformanceVariance(botGuidA);
    float varB = GetPerformanceVariance(botGuidB);
    comparison.stabilityDelta = varB - varA;  // Lower variance is better

    // Determine better model
    float score = comparison.performanceDelta * 0.5f +
                 comparison.efficiencyDelta * 0.3f +
                 comparison.stabilityDelta * 0.2f;

    comparison.betterModel = score > 0 ? comparison.modelA : comparison.modelB;
    comparison.confidence = ::std::min(1.0f, ::std::abs(score));

    return comparison;
}

::std::string LearningAnalytics::GenerateLearningReport(uint32_t botGuid) const
{
    ::std::stringstream ss;

    ss << "===== Learning Report for Bot " << botGuid << " =====\n\n";

    // Current status
    LearningPhase phase = DetectLearningPhase(botGuid);
    ::std::string phaseStr;
    switch (phase)
    {
        case LearningPhase::EXPLORATION: phaseStr = "Exploration"; break;
        case LearningPhase::LEARNING: phaseStr = "Learning"; break;
        case LearningPhase::EXPLOITATION: phaseStr = "Exploitation"; break;
        case LearningPhase::PLATEAU: phaseStr = "Plateau"; break;
        case LearningPhase::REGRESSION: phaseStr = "Regression"; break;
        case LearningPhase::CONVERGED: phaseStr = "Converged"; break;
        default: phaseStr = "Unknown"; break;
    }

    ss << "Learning Phase: " << phaseStr << "\n";
    ss << "Convergence Progress: " << (GetConvergenceProgress(botGuid) * 100.0f) << "%\n\n";

    // Performance metrics
    ss << "Performance Metrics:\n";
    ss << "  Current Performance: " << GetAveragePerformance(botGuid, 10) << "\n";
    ss << "  Peak Performance: " << GetPeakPerformance(botGuid) << "\n";
    ss << "  Performance Variance: " << GetPerformanceVariance(botGuid) << "\n";
    ss << "  Learning Rate: " << GetLearningRate(botGuid) << "\n\n";

    // Efficiency metrics
    ss << "Efficiency Metrics:\n";
    ss << "  Sample Efficiency: " << GetSampleEfficiency(botGuid) << "\n";
    ss << "  Time Efficiency: " << GetTimeEfficiency(botGuid) << "\n";
    ss << "  Compute Efficiency: " << GetComputeEfficiency(botGuid) << "\n\n";

    // Learning trend
    LearningTrend trend = AnalyzeLearningTrend(botGuid);
    ss << "Learning Trend:\n";
    ss << "  Performance Slope: " << trend.performanceSlope << "\n";
    ss << "  Loss Slope: " << trend.lossSlope << "\n";
    ss << "  Reward Trend: " << trend.rewardTrend << "\n";
    ss << "  Variance Change: " << trend.varianceChange << "\n";
    ss << "  Is Improving: " << (trend.isImproving ? "Yes" : "No") << "\n";
    ss << "  Is Stable: " << (trend.isStable ? "Yes" : "No") << "\n";
    ss << "  Has Converged: " << (trend.hasConverged ? "Yes" : "No") << "\n\n";

    // Issues and suggestions
    if (IsInPlateau(botGuid))
    {
        ss << "Warning: Learning has plateaued\n";
        auto suggestions = GetPlateauBreakingSuggestions(botGuid);
        if (!suggestions.empty())
        {
            ss << "Suggestions:\n";
            for (const auto& suggestion : suggestions)
                ss << "  - " << suggestion << "\n";
        }
    }

    if (IsRegressing(botGuid))
    {
        ss << "Warning: Performance is regressing\n";
    }

    // Anomalies
    auto anomalies = GetLearningAnomalies(botGuid);
    if (!anomalies.empty())
    {
        ss << "\nDetected Anomalies:\n";
        for (const auto& anomaly : anomalies)
            ss << "  - " << anomaly << "\n";
    }

    return ss.str();
}

::std::vector<::std::string> LearningAnalytics::GetPlateauBreakingSuggestions(uint32_t botGuid) const
{
    ::std::vector<::std::string> suggestions;

    LearningTrend trend = AnalyzeLearningTrend(botGuid);

    // Based on the characteristics of the plateau
    if (trend.varianceChange < -0.01f)
    {
        suggestions.push_back("Increase exploration rate to discover new strategies");
        suggestions.push_back("Add noise to actions for diversity");
    }

    if (::std::abs(trend.lossSlope) < 0.0001f)
    {
        suggestions.push_back("Adjust learning rate (current may be too small or too large)");
        suggestions.push_back("Consider curriculum learning with harder tasks");
    }

    suggestions.push_back("Try different reward shaping");
    suggestions.push_back("Increase network capacity if underfitting");
    suggestions.push_back("Add regularization if overfitting");
    suggestions.push_back("Reset optimizer state or use different optimizer");

    return suggestions;
}

float LearningAnalytics::CalculateSlope(const ::std::vector<float>& values) const
{
    if (values.size() < 2)
        return 0.0f;

    // Simple linear regression
    float sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
    size_t n = values.size();

    for (size_t i = 0; i < n; ++i)
    {
        sumX += i;
        sumY += values[i];
        sumXY += i * values[i];
        sumX2 += i * i;
    }

    float denominator = n * sumX2 - sumX * sumX;
    if (::std::abs(denominator) < 0.0001f)
        return 0.0f;

    return (n * sumXY - sumX * sumY) / denominator;
}

float LearningAnalytics::CalculateVariance(const ::std::vector<float>& values) const
{
    if (values.empty())
        return 0.0f;

    float mean = ::std::accumulate(values.begin(), values.end(), 0.0f) / values.size();
    float variance = 0.0f;

    for (float value : values)
    {
        float diff = value - mean;
        variance += diff * diff;
    }

    return variance / values.size();
}

::std::vector<float> LearningAnalytics::ExtractMetricSeries(
    const ::std::deque<LearningDataPoint>& points,
    auto getValue) const
{
    ::std::vector<float> series;
    series.reserve(points.size());

    for (const auto& point : points)
        series.push_back(getValue(point));

    return series;
}

float LearningAnalytics::GetPeakPerformance(uint32_t botGuid) const
{
    ::std::lock_guard lock(_dataMutex);

    auto it = _botData.find(botGuid);
    if (it != _botData.end())
        return it->second->peakPerformance;

    return 0.0f;
}

float LearningAnalytics::GetPerformanceVariance(uint32_t botGuid) const
{
    ::std::lock_guard lock(_dataMutex);

    auto it = _botData.find(botGuid);
    if (it == _botData.end() || it->second->dataPoints.empty())
        return 0.0f;

    ::std::vector<float> performances = ExtractMetricSeries(it->second->dataPoints,
        [](const LearningDataPoint& p) { return p.performance; });

    return CalculateVariance(performances);
}

float LearningAnalytics::GetTimeEfficiency(uint32_t botGuid) const
{
    ::std::lock_guard lock(_dataMutex);

    auto it = _botData.find(botGuid);
    if (it == _botData.end() || it->second->dataPoints.empty())
        return 0.0f;

    auto now = ::std::chrono::steady_clock::now();
    auto runtime = ::std::chrono::duration_cast<::std::chrono::seconds>(now - it->second->startTime).count();

    if (runtime == 0)
        return 0.0f;

    float currentPerformance = GetAveragePerformance(botGuid, 10);
    return currentPerformance / (runtime / 3600.0f);  // Performance per hour
}

float LearningAnalytics::GetComputeEfficiency(uint32_t botGuid) const
{
    // Get ML compute time from performance tracker
    ModelStatistics stats = sMLPerformanceTracker.GetModelStatistics(botGuid);

    uint64_t totalComputeTime = stats.totalInferenceTimeUs + stats.totalTrainingTimeUs;
    if (totalComputeTime == 0)
        return 0.0f;

    float currentPerformance = GetAveragePerformance(botGuid, 10);
    return currentPerformance / (totalComputeTime / 1000000.0f);  // Performance per second of compute
}

::std::vector<::std::string> LearningAnalytics::GetLearningAnomalies(uint32_t botGuid) const
{
    ::std::vector<::std::string> anomalies;

    LearningTrend trend = AnalyzeLearningTrend(botGuid);

    // Sudden performance drops
    if (trend.performanceSlope < -0.01f)
        anomalies.push_back("Sudden performance degradation detected");

    // Excessive variance
    float variance = GetPerformanceVariance(botGuid);
    if (variance > 0.1f)
        anomalies.push_back("Excessive performance variance");

    // Loss explosion
    if (trend.lossSlope > 0.01f)
        anomalies.push_back("Loss is increasing (possible gradient explosion)");

    // Stuck in local minimum
    if (trend.isStable && GetAveragePerformance(botGuid) < 0.3f)
        anomalies.push_back("Possibly stuck in local minimum");

    return anomalies;
}

void LearningAnalytics::UpdateGlobalMetrics()
{
    uint32_t converged = 0;
    uint32_t improving = 0;
    uint32_t regressing = 0;
    float totalPerf = 0.0f;
    float totalLearningRate = 0.0f;
    uint32_t count = 0;

    for (const auto& [botGuid, data] : _botData)
    {
        count++;

        if (data->currentPhase == LearningPhase::CONVERGED)
            converged++;
        else if (data->currentPhase == LearningPhase::REGRESSION)
            regressing++;
        else
        {
            LearningTrend trend = AnalyzeLearningTrend(botGuid);
            if (trend.isImproving)
                improving++;
        }

        totalPerf += GetAveragePerformance(botGuid, 10);
        totalLearningRate += GetLearningRate(botGuid);
    }

    _globalMetrics.convergedBots = converged;
    _globalMetrics.improvingBots = improving;
    _globalMetrics.regressingBots = regressing;

    if (count > 0)
    {
        _globalMetrics.averagePerformance = totalPerf / count;
        _globalMetrics.averageLearningRate = totalLearningRate / count;
    }
}

// ScopedLearningExperiment Implementation
ScopedLearningExperiment::ScopedLearningExperiment(const ::std::string& name, const ::std::string& configuration)
    : _name(name)
    , _configuration(configuration)
    , _successful(false)
{
    _startTime = ::std::chrono::steady_clock::now();
    sLearningAnalytics.StartExperiment(_name, _configuration);
}

ScopedLearningExperiment::~ScopedLearningExperiment()
{
    sLearningAnalytics.EndExperiment(_name, _successful);
}

void ScopedLearningExperiment::RecordMetric(const ::std::string& name, float value)
{
    _metrics[name] = value;
}

} // namespace Playerbot