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
#include <chrono>
#include <atomic>
#include <unordered_map>
#include <vector>
#include <deque>
#include <mutex>
#include <memory>

namespace Playerbot
{

// Learning curve data point
struct LearningDataPoint
{
    uint64_t timestamp;
    uint32_t episode;
    float performance;
    float loss;
    float reward;
    float explorationRate;
    uint32_t experienceCount;
    float accuracy;

    LearningDataPoint() : timestamp(0), episode(0), performance(0), loss(0),
        reward(0), explorationRate(0), experienceCount(0), accuracy(0) {}
};

// Learning phase detection
enum class LearningPhase : uint8
{
    EXPLORATION,    // High exploration, random actions
    LEARNING,       // Balanced exploration/exploitation
    EXPLOITATION,   // Low exploration, using learned policy
    PLATEAU,        // Performance has plateaued
    REGRESSION,     // Performance is declining
    CONVERGED      // Model has converged
};

// Learning trend analysis
struct LearningTrend
{
    float performanceSlope;      // Rate of performance improvement
    float lossSlope;             // Rate of loss decrease
    float rewardTrend;           // Trend in average reward
    float varianceChange;        // Change in performance variance
    bool isImproving;
    bool isStable;
    bool hasConverged;

    LearningTrend() : performanceSlope(0), lossSlope(0), rewardTrend(0),
        varianceChange(0), isImproving(false), isStable(false), hasConverged(false) {}
};

// Model comparison metrics
struct ModelComparison
{
    ::std::string modelA;
    ::std::string modelB;
    float performanceDelta;      // A - B performance
    float efficiencyDelta;       // A - B efficiency
    float stabilityDelta;        // A - B stability
    ::std::string betterModel;
    float confidence;

    ModelComparison() : performanceDelta(0), efficiencyDelta(0),
        stabilityDelta(0), confidence(0) {}
};

// Learning experiment results
struct ExperimentResult
{
    ::std::string name;
    ::std::string configuration;
    uint32_t iterations;
    float finalPerformance;
    float peakPerformance;
    float convergenceTime;       // Time to converge in seconds
    float efficiency;            // Performance per computation
    bool successful;
    ::std::string notes;

    ExperimentResult() : iterations(0), finalPerformance(0), peakPerformance(0),
        convergenceTime(0), efficiency(0), successful(false) {}
};

// Learning analytics engine
class TC_GAME_API LearningAnalytics
{
public:
    static LearningAnalytics& Instance()
    {
        static LearningAnalytics instance;
        return instance;
    }

    // System initialization
    bool Initialize();
    void Shutdown();

    // Data collection
    void RecordLearningStep(uint32_t botGuid, const LearningDataPoint& dataPoint);
    void RecordEpisode(uint32_t botGuid, uint32_t episode, float totalReward, float avgLoss);
    void RecordModelUpdate(uint32_t botGuid, uint32_t modelVersion, float performance);

    // Learning curve analysis
    LearningTrend AnalyzeLearningTrend(uint32_t botGuid) const;
    LearningPhase DetectLearningPhase(uint32_t botGuid) const;
    float GetLearningRate(uint32_t botGuid) const;
    float GetConvergenceProgress(uint32_t botGuid) const;

    // Performance analytics
    float GetAveragePerformance(uint32_t botGuid, uint32_t lastEpisodes = 100) const;
    float GetPerformanceVariance(uint32_t botGuid) const;
    float GetPeakPerformance(uint32_t botGuid) const;
    ::std::pair<uint32_t, float> GetBestEpisode(uint32_t botGuid) const;

    // Learning efficiency metrics
    float GetSampleEfficiency(uint32_t botGuid) const;  // Performance per experience
    float GetTimeEfficiency(uint32_t botGuid) const;    // Performance per time
    float GetComputeEfficiency(uint32_t botGuid) const; // Performance per computation

    // Convergence detection
    bool HasConverged(uint32_t botGuid) const;
    float GetConvergenceScore(uint32_t botGuid) const;
    uint32_t GetConvergenceEpisode(uint32_t botGuid) const;
    float EstimateTimeToConvergence(uint32_t botGuid) const;

    // Plateau and regression detection
    bool IsInPlateau(uint32_t botGuid) const;
    bool IsRegressing(uint32_t botGuid) const;
    uint32_t GetPlateauDuration(uint32_t botGuid) const;
    ::std::vector<::std::string> GetPlateauBreakingSuggestions(uint32_t botGuid) const;

    // Model comparison
    ModelComparison CompareModels(uint32_t botGuidA, uint32_t botGuidB) const;
    ::std::vector<uint32_t> RankBotsByPerformance() const;
    uint32_t GetBestPerformingBot() const;

    // Experiment management
    void StartExperiment(const ::std::string& name, const ::std::string& configuration);
    void EndExperiment(const ::std::string& name, bool successful);
    ExperimentResult GetExperimentResult(const ::std::string& name) const;
    ::std::vector<ExperimentResult> GetAllExperiments() const;

    // Visualization data export
    void ExportLearningCurve(uint32_t botGuid, const ::std::string& filename) const;
    void ExportComparisonChart(const ::std::vector<uint32_t>& botGuids, const ::std::string& filename) const;
    ::std::string GenerateLearningReport(uint32_t botGuid) const;

    // Hyperparameter impact analysis
    void RecordHyperparameters(uint32_t botGuid, const ::std::unordered_map<::std::string, float>& params);
    ::std::unordered_map<::std::string, float> AnalyzeHyperparameterImpact() const;
    ::std::unordered_map<::std::string, float> GetOptimalHyperparameters() const;

    // Anomaly detection
    bool DetectAnomalousLearning(uint32_t botGuid) const;
    ::std::vector<::std::string> GetLearningAnomalies(uint32_t botGuid) const;

    // Predictive analytics
    float PredictFuturePerformance(uint32_t botGuid, uint32_t futureEpisodes) const;
    float PredictConvergencePerformance(uint32_t botGuid) const;

    // Global learning metrics
    struct GlobalMetrics
    {
        ::std::atomic<uint32_t> totalBots{0};
        ::std::atomic<uint32_t> convergedBots{0};
        ::std::atomic<uint32_t> improvingBots{0};
        ::std::atomic<uint32_t> regressingBots{0};
        ::std::atomic<float> averagePerformance{0};
        ::std::atomic<float> averageLearningRate{0};
        ::std::atomic<uint64_t> totalLearningSteps{0};
        ::std::atomic<uint64_t> totalExperiences{0};
    };

    GlobalMetrics GetGlobalMetrics() const;

    // Configuration
    void SetConvergenceThreshold(float threshold) { _convergenceThreshold = threshold; }
    void SetPlateauThreshold(uint32_t episodes) { _plateauThreshold = episodes; }
    void SetMinDataPoints(uint32_t points) { _minDataPoints = points; }

private:
    LearningAnalytics();
    ~LearningAnalytics();

    // System state
    bool _initialized;

    // Learning data storage
    struct BotLearningData
    {
        ::std::deque<LearningDataPoint> dataPoints;
        ::std::unordered_map<uint32_t, float> episodeRewards;
        ::std::unordered_map<uint32_t, float> modelVersions;
        ::std::unordered_map<::std::string, float> hyperparameters;
        LearningPhase currentPhase;
        uint32_t plateauStartEpisode;
        uint32_t convergenceEpisode;
        float peakPerformance;
        ::std::chrono::steady_clock::time_point startTime;

        static constexpr size_t MAX_DATA_POINTS = 10000;
    };

    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BEHAVIOR_MANAGER> _dataMutex;
    ::std::unordered_map<uint32_t, ::std::unique_ptr<BotLearningData>> _botData;

    // Experiments
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BEHAVIOR_MANAGER> _experimentsMutex;
    ::std::unordered_map<::std::string, ExperimentResult> _experiments;
    ::std::unordered_map<::std::string, ::std::chrono::steady_clock::time_point> _activeExperiments;

    // Configuration
    float _convergenceThreshold = 0.01f;     // Performance variance for convergence
    uint32_t _plateauThreshold = 50;         // Episodes without improvement
    uint32_t _minDataPoints = 20;            // Minimum data for analysis

    // Global metrics
    mutable GlobalMetrics _globalMetrics;

    // Helper methods
    BotLearningData* GetOrCreateBotData(uint32_t botGuid);
    float CalculateSlope(const ::std::vector<float>& values) const;
    float CalculateVariance(const ::std::vector<float>& values) const;
    float CalculateMovingAverage(const ::std::deque<LearningDataPoint>& points,
                                 size_t window, auto getValue) const;
    bool DetectConvergence(const ::std::deque<LearningDataPoint>& points) const;
    bool DetectPlateau(const ::std::deque<LearningDataPoint>& points) const;
    void UpdateGlobalMetrics();
    ::std::vector<float> ExtractMetricSeries(const ::std::deque<LearningDataPoint>& points,
                                          auto getValue) const;
    float PredictWithLinearRegression(const ::std::vector<float>& values, uint32_t steps) const;
    float CalculateEfficiency(const BotLearningData* data) const;
};

// RAII helper for learning experiments
class TC_GAME_API ScopedLearningExperiment
{
public:
    ScopedLearningExperiment(const ::std::string& name, const ::std::string& configuration);
    ~ScopedLearningExperiment();

    void MarkSuccessful() { _successful = true; }
    void SetNote(const ::std::string& note) { _note = note; }
    void RecordMetric(const ::std::string& name, float value);

private:
    ::std::string _name;
    ::std::string _configuration;
    bool _successful;
    ::std::string _note;
    ::std::chrono::steady_clock::time_point _startTime;
    ::std::unordered_map<::std::string, float> _metrics;
};

#define sLearningAnalytics LearningAnalytics::Instance()

} // namespace Playerbot