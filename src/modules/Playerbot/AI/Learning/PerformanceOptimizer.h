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
#include <memory>
#include <vector>
#include <unordered_map>
#include <deque>
#include <chrono>
#include <atomic>
#include <mutex>
#include <array>

class Player;
class Unit;

namespace Playerbot
{

// Forward declarations
class BotAI;
struct ActionContext;

// Optimization objectives
enum class OptimizationGoal : uint8
{
    MAXIMIZE_DAMAGE,        // DPS optimization
    MINIMIZE_DAMAGE_TAKEN,  // Survival optimization
    MAXIMIZE_HEALING,       // Healing output
    MAXIMIZE_EFFICIENCY,    // Resource efficiency
    MINIMIZE_DOWNTIME,      // Activity optimization
    MAXIMIZE_WIN_RATE,      // Overall success
    BALANCE_ALL            // Balanced approach
};

// Performance metric types
enum class PerformanceMetric : uint8
{
    DAMAGE_PER_SECOND,
    HEALING_PER_SECOND,
    DAMAGE_TAKEN_PER_SECOND,
    RESOURCE_EFFICIENCY,
    ABILITY_ACCURACY,
    POSITIONING_QUALITY,
    REACTION_TIME,
    SURVIVAL_RATE,
    OBJECTIVE_COMPLETION,
    OVERALL_EFFECTIVENESS
};

// Performance sample for analysis
struct PerformanceSample
{
    uint64_t timestamp;
    float damageDealt;
    float healingDone;
    float damageTaken;
    float resourceUsed;
    float resourceGenerated;
    uint32_t abilitiesUsed;
    uint32_t successfulActions;
    uint32_t failedActions;
    float distanceMoved;
    bool died;
    bool objectiveCompleted;

    PerformanceSample() : timestamp(0), damageDealt(0), healingDone(0),
        damageTaken(0), resourceUsed(0), resourceGenerated(0),
        abilitiesUsed(0), successfulActions(0), failedActions(0),
        distanceMoved(0), died(false), objectiveCompleted(false) {}

    float GetEffectiveness() const;
};

// Optimization strategy
struct OptimizationStrategy
{
    ::std::string name;
    OptimizationGoal goal;
    ::std::vector<float> parameters;     // Strategy-specific parameters
    float fitness;                      // How well this strategy performs
    uint32_t iterations;                // How many times tested
    ::std::chrono::steady_clock::time_point lastUpdated;

    OptimizationStrategy() : goal(OptimizationGoal::BALANCE_ALL),
        fitness(0.0f), iterations(0) {}

    void Update(float newFitness);
    float GetConfidence() const;
};

// Genetic algorithm chromosome for strategy evolution
struct StrategyChromosome
{
    ::std::vector<float> genes;  // Strategy parameters encoded as genes
    float fitness;
    uint32_t generation;

    StrategyChromosome() : fitness(0.0f), generation(0) {}

    void Mutate(float mutationRate = 0.1f);
    StrategyChromosome Crossover(const StrategyChromosome& other) const;
};

// Performance profile for optimization
class TC_GAME_API PerformanceProfile
{
public:
    PerformanceProfile(ObjectGuid guid);
    ~PerformanceProfile();

    // Sample management
    void AddSample(const PerformanceSample& sample);
    void ClearSamples();

    // Metrics calculation
    float GetMetric(PerformanceMetric metric) const;
    ::std::vector<float> GetAllMetrics() const;
    float GetOverallScore() const;

    // Trend analysis
    float GetMetricTrend(PerformanceMetric metric) const;
    bool IsImproving() const;
    float GetImprovementRate() const;

    // Strategy evaluation
    float EvaluateStrategy(const OptimizationStrategy& strategy) const;
    OptimizationStrategy GetBestStrategy() const { return _currentStrategy; }
    void SetStrategy(const OptimizationStrategy& strategy) { _currentStrategy = strategy; }

    // Performance comparison
    float Compare(const PerformanceProfile& other) const;

private:
    ObjectGuid _guid;
    ::std::deque<PerformanceSample> _samples;
    static constexpr size_t MAX_SAMPLES = 500;

    // Cached metrics
    mutable ::std::unordered_map<PerformanceMetric, float> _cachedMetrics;
    mutable bool _metricsValid;

    // Current optimization strategy
    OptimizationStrategy _currentStrategy;

    // Performance history
    ::std::vector<float> _scoreHistory;
    float _baselineScore;

    // Helper methods
    void InvalidateCache() { _metricsValid = false; }
    void UpdateCache() const;
    float CalculateMetric(PerformanceMetric metric) const;
};

// Evolutionary optimizer for strategy improvement
class TC_GAME_API EvolutionaryOptimizer
{
public:
    EvolutionaryOptimizer(size_t populationSize = 20);
    ~EvolutionaryOptimizer();

    // Evolution cycle
    void InitializePopulation(size_t chromosomeSize);
    void EvaluateFitness(::std::function<float(const StrategyChromosome&)> fitnessFunc);
    void Evolve();
    StrategyChromosome GetBestChromosome() const;

    // Configuration
    void SetMutationRate(float rate) { _mutationRate = ::std::clamp(rate, 0.0f, 1.0f); }
    void SetCrossoverRate(float rate) { _crossoverRate = ::std::clamp(rate, 0.0f, 1.0f); }
    void SetElitismCount(size_t count) { _elitismCount = ::std::min(count, _populationSize / 2); }

    // Statistics
    float GetAverageFitness() const;
    float GetBestFitness() const;
    uint32_t GetGeneration() const { return _generation; }

private:
    ::std::vector<StrategyChromosome> _population;
    size_t _populationSize;
    uint32_t _generation;

    // Evolution parameters
    float _mutationRate;
    float _crossoverRate;
    size_t _elitismCount;

    // Selection methods
    StrategyChromosome TournamentSelection(size_t tournamentSize = 3) const;
    StrategyChromosome RouletteWheelSelection() const;

    // Helper methods
    void SortPopulation();
};

// Main performance optimization engine
class TC_GAME_API PerformanceOptimizer
{
public:
    static PerformanceOptimizer& Instance()
    {
        static PerformanceOptimizer instance;
        return instance;
    }

    // System initialization
    bool Initialize();
    void Shutdown();
    bool IsEnabled() const { return _enabled; }

    // Profile management
    void CreateProfile(uint32_t botGuid);
    void DeleteProfile(uint32_t botGuid);
    ::std::shared_ptr<PerformanceProfile> GetProfile(uint32_t botGuid) const;

    // Performance tracking
    void RecordPerformance(uint32_t botGuid, const PerformanceSample& sample);
    void RecordCombatPerformance(uint32_t botGuid, float damage, float healing, float damageTaken);
    void RecordActionResult(uint32_t botGuid, const ::std::string& action, bool success);

    // Optimization
    void OptimizeBotPerformance(uint32_t botGuid);
    OptimizationStrategy GetOptimalStrategy(uint32_t botGuid, OptimizationGoal goal);
    void ApplyOptimization(BotAI* ai, const OptimizationStrategy& strategy);

    // Real-time adjustments
    void AdjustStrategyParameters(uint32_t botGuid, const ::std::vector<float>& feedback);
    void AdaptToSituation(uint32_t botGuid, const ::std::string& situation);

    // Performance analysis
    float GetBotEffectiveness(uint32_t botGuid) const;
    ::std::vector<::std::pair<PerformanceMetric, float>> GetBotMetrics(uint32_t botGuid) const;
    ::std::string GeneratePerformanceReport(uint32_t botGuid) const;

    // Collective optimization
    void OptimizeGroupPerformance(const ::std::vector<uint32_t>& botGuids);
    void ShareOptimizations(uint32_t sourceBotGuid, uint32_t targetBotGuid);
    void PropagateSuccessfulStrategies();

    // Self-tuning parameters
    struct TuningParameter
    {
        ::std::string name;
        float value;
        float minValue;
        float maxValue;
        float learningRate;
        float momentum;

        void Update(float gradient);
        float Normalize() const;
    };

    void RegisterTuningParameter(const ::std::string& name, float initial,
                                 float min, float max, float learningRate = 0.01f);
    float GetTuningParameter(const ::std::string& name) const;
    void UpdateTuningParameters(const ::std::unordered_map<::std::string, float>& gradients);

    // Performance benchmarking
    void StartBenchmark(uint32_t botGuid, const ::std::string& benchmarkName);
    void EndBenchmark(uint32_t botGuid, const ::std::string& benchmarkName);
    float GetBenchmarkScore(uint32_t botGuid, const ::std::string& benchmarkName) const;

    // Configuration
    void SetOptimizationInterval(uint32_t ms) { _optimizationIntervalMs = ms; }
    void SetLearningRate(float rate) { _learningRate = ::std::clamp(rate, 0.0001f, 0.1f); }
    void EnableAutoOptimization(bool enable) { _autoOptimize = enable; }

    // Metrics
    struct OptimizationMetrics
    {
        ::std::atomic<uint32_t> profilesOptimized{0};
        ::std::atomic<uint32_t> strategiesEvaluated{0};
        ::std::atomic<float> averageImprovement{0.0f};
        ::std::atomic<float> bestImprovement{0.0f};
        ::std::chrono::steady_clock::time_point startTime;
    };

    OptimizationMetrics GetMetrics() const { return _metrics; }

private:
    PerformanceOptimizer();
    ~PerformanceOptimizer();

    // System state
    bool _initialized;
    bool _enabled;
    bool _autoOptimize;

    // Profiles
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _profilesMutex;
    ::std::unordered_map<uint32_t, ::std::shared_ptr<PerformanceProfile>> _profiles;

    // Evolutionary optimizers per bot
    ::std::unordered_map<uint32_t, ::std::unique_ptr<EvolutionaryOptimizer>> _optimizers;

    // Strategy database
    ::std::vector<OptimizationStrategy> _strategyDatabase;
    ::std::unordered_map<OptimizationGoal, ::std::vector<OptimizationStrategy>> _goalStrategies;

    // Tuning parameters
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _parametersMutex;
    ::std::unordered_map<::std::string, TuningParameter> _tuningParameters;

    // Benchmarks
    struct BenchmarkData
    {
        ::std::chrono::steady_clock::time_point startTime;
        ::std::chrono::steady_clock::time_point endTime;
        float score;
        uint32_t iterations;
    };
    ::std::unordered_map<uint32_t, ::std::unordered_map<::std::string, BenchmarkData>> _benchmarks;

    // Configuration
    uint32_t _optimizationIntervalMs;
    float _learningRate;

    // Metrics
    mutable OptimizationMetrics _metrics;

    // Helper methods
    ::std::shared_ptr<PerformanceProfile> GetOrCreateProfile(uint32_t botGuid);
    void InitializeStrategies();
    void UpdateStrategyFitness(OptimizationStrategy& strategy, float performance);
    OptimizationStrategy CreateRandomStrategy(OptimizationGoal goal) const;
    OptimizationStrategy CombineStrategies(const OptimizationStrategy& a,
                                          const OptimizationStrategy& b) const;
    void OptimizeWithEvolution(uint32_t botGuid);
    void OptimizeWithGradientDescent(uint32_t botGuid);
    float EvaluateStrategyFitness(uint32_t botGuid, const StrategyChromosome& chromosome);

    // Constants
    static constexpr uint32_t DEFAULT_OPTIMIZATION_INTERVAL_MS = 30000;  // 30 seconds
    static constexpr float DEFAULT_LEARNING_RATE = 0.01f;
    static constexpr size_t MAX_STRATEGY_DATABASE_SIZE = 100;
    static constexpr size_t EVOLUTION_POPULATION_SIZE = 20;
    static constexpr uint32_t EVOLUTION_GENERATIONS = 10;
};

// RAII helper for performance measurement
class TC_GAME_API ScopedPerformanceMeasurement
{
public:
    ScopedPerformanceMeasurement(uint32_t botGuid, const ::std::string& operation);
    ~ScopedPerformanceMeasurement();

    void RecordMetric(const ::std::string& name, float value);
    void MarkSuccess();

private:
    uint32_t _botGuid;
    ::std::string _operation;
    ::std::chrono::steady_clock::time_point _startTime;
    PerformanceSample _sample;
    bool _success;
};

#define sPerformanceOptimizer PerformanceOptimizer::Instance()

} // namespace Playerbot