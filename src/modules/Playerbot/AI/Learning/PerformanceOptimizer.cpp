/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PerformanceOptimizer.h"
#include "AI/BotAI.h"
#include "Player.h"
#include "Log.h"
#include "Performance/BotPerformanceMonitor.h"
#include <random>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <cmath>

namespace Playerbot
{

// PerformanceSample Implementation
float PerformanceSample::GetEffectiveness() const
{
    float effectiveness = 0.0f;

    // Damage effectiveness
    if (damageDealt > 0)
        effectiveness += damageDealt / 10000.0f;  // Normalized

    // Healing effectiveness
    if (healingDone > 0)
        effectiveness += healingDone / 10000.0f;

    // Survival bonus
    if (!died)
        effectiveness += 0.5f;
    else
        effectiveness -= 1.0f;

    // Damage mitigation
    if (damageTaken > 0)
        effectiveness -= damageTaken / 20000.0f;

    // Resource efficiency
    if (resourceUsed > 0)
    {
        float efficiency = (damageDealt + healingDone) / resourceUsed;
        effectiveness += efficiency / 100.0f;
    }

    // Action success rate
    if (abilitiesUsed > 0)
    {
        float successRate = static_cast<float>(successfulActions) / abilitiesUsed;
        effectiveness += successRate * 0.3f;
    }

    // Objective bonus
    if (objectiveCompleted)
        effectiveness += 1.0f;

    return effectiveness;
}

// OptimizationStrategy Implementation
void OptimizationStrategy::Update(float newFitness)
{
    // Exponential moving average
    float alpha = 0.1f;
    fitness = (iterations > 0) ? (fitness * (1.0f - alpha) + newFitness * alpha) : newFitness;
    iterations++;
    lastUpdated = ::std::chrono::steady_clock::now();
}

float OptimizationStrategy::GetConfidence() const
{
    // Confidence based on number of iterations and recency
    float iterationConfidence = ::std::min(1.0f, iterations / 100.0f);

    auto now = ::std::chrono::steady_clock::now();
    auto timeSinceUpdate = ::std::chrono::duration_cast<::std::chrono::hours>(
        now - lastUpdated).count();
    float recencyFactor = ::std::exp(-timeSinceUpdate / 24.0f);  // Decay over 24 hours

    return iterationConfidence * recencyFactor;
}

// StrategyChromosome Implementation
void StrategyChromosome::Mutate(float mutationRate)
{
    ::std::random_device rd;
    ::std::mt19937 gen(rd());
    ::std::uniform_real_distribution<float> mutationDist(0.0f, 1.0f);
    ::std::normal_distribution<float> valueDist(0.0f, 0.1f);

    for (float& gene : genes)
    {
        if (mutationDist(gen) < mutationRate)
        {
            gene += valueDist(gen);
            gene = ::std::clamp(gene, 0.0f, 1.0f);
        }
    }
}

StrategyChromosome StrategyChromosome::Crossover(const StrategyChromosome& other) const
{
    StrategyChromosome offspring;
    offspring.generation = ::std::max(generation, other.generation) + 1;

    if (genes.size() != other.genes.size())
        return offspring;

    offspring.genes.resize(genes.size());

    ::std::random_device rd;
    ::std::mt19937 gen(rd());
    ::std::uniform_int_distribution<size_t> crossoverPoint(1, genes.size() - 1);

    // Two-point crossover
    size_t point1 = crossoverPoint(gen);
    size_t point2 = crossoverPoint(gen);
    if (point1 > point2)
        ::std::swap(point1, point2);

    for (size_t i = 0; i < genes.size(); ++i)
    {
        if (i < point1 || i >= point2)
            offspring.genes[i] = genes[i];
        else
            offspring.genes[i] = other.genes[i];
    }

    return offspring;
}

// PerformanceProfile Implementation
PerformanceProfile::PerformanceProfile(ObjectGuid guid)
    : _guid(guid), _metricsValid(false), _baselineScore(0.0f)
{
}

PerformanceProfile::~PerformanceProfile() = default;

void PerformanceProfile::AddSample(const PerformanceSample& sample)
{
    _samples.push_back(sample);

    if (_samples.size() > MAX_SAMPLES)
        _samples.pop_front();

    InvalidateCache();

    // Update score history
    float currentScore = sample.GetEffectiveness();
    _scoreHistory.push_back(currentScore);

    // Set baseline after initial samples
    if (_scoreHistory.size() == 10)
    {
        _baselineScore = ::std::accumulate(_scoreHistory.begin(), _scoreHistory.end(), 0.0f) /
                         _scoreHistory.size();
    }
}

float PerformanceProfile::GetMetric(PerformanceMetric metric) const
{
    if (!_metricsValid)
        UpdateCache();

    auto it = _cachedMetrics.find(metric);
    if (it != _cachedMetrics.end())
        return it->second;

    return 0.0f;
}

::std::vector<float> PerformanceProfile::GetAllMetrics() const
{
    ::std::vector<float> metrics;

    for (uint8_t i = 0; i <= static_cast<uint8_t>(PerformanceMetric::OVERALL_EFFECTIVENESS); ++i)
    {
        metrics.push_back(GetMetric(static_cast<PerformanceMetric>(i)));
    }

    return metrics;
}

float PerformanceProfile::GetOverallScore() const
{
    if (_samples.empty())
        return 0.0f;

    float totalScore = 0.0f;
    for (const auto& sample : _samples)
        totalScore += sample.GetEffectiveness();

    return totalScore / _samples.size();
}

void PerformanceProfile::UpdateCache() const
{
    _cachedMetrics.clear();

    for (uint8_t i = 0; i <= static_cast<uint8_t>(PerformanceMetric::OVERALL_EFFECTIVENESS); ++i)
    {
        PerformanceMetric metric = static_cast<PerformanceMetric>(i);
        _cachedMetrics[metric] = CalculateMetric(metric);
    }

    _metricsValid = true;
}

float PerformanceProfile::CalculateMetric(PerformanceMetric metric) const
{
    if (_samples.empty())
        return 0.0f;

    switch (metric)
    {
        case PerformanceMetric::DAMAGE_PER_SECOND:
        {
            float totalDamage = 0.0f;
            float totalTime = 0.0f;

            for (size_t i = 1; i < _samples.size(); ++i)
            {
                totalDamage += _samples[i].damageDealt;
                totalTime += (_samples[i].timestamp - _samples[i-1].timestamp) / 1000000.0f;
            }

            return totalTime > 0 ? totalDamage / totalTime : 0.0f;
        }

        case PerformanceMetric::HEALING_PER_SECOND:
        {
            float totalHealing = 0.0f;
            float totalTime = 0.0f;

            for (size_t i = 1; i < _samples.size(); ++i)
            {
                totalHealing += _samples[i].healingDone;
                totalTime += (_samples[i].timestamp - _samples[i-1].timestamp) / 1000000.0f;
            }

            return totalTime > 0 ? totalHealing / totalTime : 0.0f;
        }

        case PerformanceMetric::DAMAGE_TAKEN_PER_SECOND:
        {
            float totalDamageTaken = 0.0f;
            float totalTime = 0.0f;

            for (size_t i = 1; i < _samples.size(); ++i)
            {
                totalDamageTaken += _samples[i].damageTaken;
                totalTime += (_samples[i].timestamp - _samples[i-1].timestamp) / 1000000.0f;
            }

            return totalTime > 0 ? totalDamageTaken / totalTime : 0.0f;
        }

        case PerformanceMetric::RESOURCE_EFFICIENCY:
        {
            float totalOutput = 0.0f;
            float totalResourceUsed = 0.0f;

            for (const auto& sample : _samples)
            {
                totalOutput += sample.damageDealt + sample.healingDone;
                totalResourceUsed += sample.resourceUsed;
            }

            return totalResourceUsed > 0 ? totalOutput / totalResourceUsed : 0.0f;
        }

        case PerformanceMetric::ABILITY_ACCURACY:
        {
            uint32_t totalSuccessful = 0;
            uint32_t totalAttempts = 0;

            for (const auto& sample : _samples)
            {
                totalSuccessful += sample.successfulActions;
                totalAttempts += sample.abilitiesUsed;
            }

            return totalAttempts > 0 ? static_cast<float>(totalSuccessful) / totalAttempts : 0.0f;
        }

        case PerformanceMetric::POSITIONING_QUALITY:
        {
            // Simple metric based on movement efficiency
            float totalMovement = 0.0f;
            float effectiveActions = 0.0f;

            for (const auto& sample : _samples)
            {
                totalMovement += sample.distanceMoved;
                effectiveActions += sample.successfulActions;
            }

            return totalMovement > 0 ? effectiveActions / totalMovement : 0.0f;
        }

        case PerformanceMetric::SURVIVAL_RATE:
        {
            uint32_t deaths = 0;
            for (const auto& sample : _samples)
            {
                if (sample.died)
                    deaths++;
            }

            return 1.0f - (static_cast<float>(deaths) / _samples.size());
        }

        case PerformanceMetric::OBJECTIVE_COMPLETION:
        {
            uint32_t completed = 0;
            for (const auto& sample : _samples)
            {
                if (sample.objectiveCompleted)
                    completed++;
            }

            return static_cast<float>(completed) / _samples.size();
        }

        case PerformanceMetric::OVERALL_EFFECTIVENESS:
            return GetOverallScore();

        default:
            return 0.0f;
    }
}

float PerformanceProfile::GetMetricTrend(PerformanceMetric metric) const
{
    if (_samples.size() < 10)
        return 0.0f;

    // Calculate trend using linear regression on recent samples
    ::std::vector<float> recentValues;
    size_t sampleCount = ::std::min<size_t>(20, _samples.size());

    for (size_t i = _samples.size() - sampleCount; i < _samples.size(); ++i)
    {
        // Simplified: just use effectiveness as proxy for trend
        recentValues.push_back(_samples[i].GetEffectiveness());
    }

    // Calculate slope
    float sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
    for (size_t i = 0; i < recentValues.size(); ++i)
    {
        sumX += i;
        sumY += recentValues[i];
        sumXY += i * recentValues[i];
        sumX2 += i * i;
    }

    float n = recentValues.size();
    float slope = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);

    return slope;
}

bool PerformanceProfile::IsImproving() const
{
    return GetMetricTrend(PerformanceMetric::OVERALL_EFFECTIVENESS) > 0.0f;
}

float PerformanceProfile::GetImprovementRate() const
{
    if (_scoreHistory.size() < 10 || _baselineScore == 0.0f)
        return 0.0f;

    float recentAverage = 0.0f;
    size_t recentCount = ::std::min<size_t>(10, _scoreHistory.size());

    for (size_t i = _scoreHistory.size() - recentCount; i < _scoreHistory.size(); ++i)
        recentAverage += _scoreHistory[i];

    recentAverage /= recentCount;

    return (recentAverage - _baselineScore) / _baselineScore;
}

// EvolutionaryOptimizer Implementation
EvolutionaryOptimizer::EvolutionaryOptimizer(size_t populationSize)
    : _populationSize(populationSize)
    , _generation(0)
    , _mutationRate(0.1f)
    , _crossoverRate(0.7f)
    , _elitismCount(2)
{
}

EvolutionaryOptimizer::~EvolutionaryOptimizer() = default;

void EvolutionaryOptimizer::InitializePopulation(size_t chromosomeSize)
{
    _population.clear();
    _population.reserve(_populationSize);

    ::std::random_device rd;
    ::std::mt19937 gen(rd());
    ::std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for (size_t i = 0; i < _populationSize; ++i)
    {
        StrategyChromosome chromosome;
        chromosome.genes.resize(chromosomeSize);
        chromosome.generation = 0;

        for (float& gene : chromosome.genes)
            gene = dist(gen);

        _population.push_back(chromosome);
    }
}

void EvolutionaryOptimizer::EvaluateFitness(::std::function<float(const StrategyChromosome&)> fitnessFunc)
{
    for (auto& chromosome : _population)
    {
        chromosome.fitness = fitnessFunc(chromosome);
    }

    SortPopulation();
}

void EvolutionaryOptimizer::Evolve()
{
    if (_population.empty())
        return;

    ::std::vector<StrategyChromosome> newPopulation;
    newPopulation.reserve(_populationSize);

    // Elitism: keep best individuals
    SortPopulation();
    for (size_t i = 0; i < _elitismCount && i < _population.size(); ++i)
    {
        newPopulation.push_back(_population[i]);
    }

    // Generate offspring
    ::std::random_device rd;
    ::std::mt19937 gen(rd());
    ::std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    while (newPopulation.size() < _populationSize)
    {
        // Selection
        StrategyChromosome parent1 = TournamentSelection();
        StrategyChromosome parent2 = TournamentSelection();

        // Crossover
        StrategyChromosome offspring;
        if (dist(gen) < _crossoverRate)
        {
            offspring = parent1.Crossover(parent2);
        }
        else
        {
            offspring = (dist(gen) < 0.5f) ? parent1 : parent2;
        }

        // Mutation
        offspring.Mutate(_mutationRate);

        newPopulation.push_back(offspring);
    }

    _population = ::std::move(newPopulation);
    _generation++;
}

StrategyChromosome EvolutionaryOptimizer::GetBestChromosome() const
{
    if (_population.empty())
        return StrategyChromosome();

    return _population[0];  // Assuming sorted
}

StrategyChromosome EvolutionaryOptimizer::TournamentSelection(size_t tournamentSize) const
{
    if (_population.empty())
        return StrategyChromosome();

    ::std::random_device rd;
    ::std::mt19937 gen(rd());
    ::std::uniform_int_distribution<size_t> dist(0, _population.size() - 1);

    size_t bestIdx = dist(gen);
    float bestFitness = _population[bestIdx].fitness;

    for (size_t i = 1; i < tournamentSize; ++i)
    {
        size_t idx = dist(gen);
        if (_population[idx].fitness > bestFitness)
        {
            bestIdx = idx;
            bestFitness = _population[idx].fitness;
        }
    }

    return _population[bestIdx];
}

void EvolutionaryOptimizer::SortPopulation()
{
    ::std::sort(_population.begin(), _population.end(),
        [](const StrategyChromosome& a, const StrategyChromosome& b)
        {
            return a.fitness > b.fitness;
        });
}

float EvolutionaryOptimizer::GetAverageFitness() const
{
    if (_population.empty())
        return 0.0f;

    float sum = 0.0f;
    for (const auto& chromosome : _population)
        sum += chromosome.fitness;

    return sum / _population.size();
}

float EvolutionaryOptimizer::GetBestFitness() const
{
    if (_population.empty())
        return 0.0f;

    return _population[0].fitness;
}

// PerformanceOptimizer Implementation
PerformanceOptimizer::PerformanceOptimizer()
    : _initialized(false)
    , _enabled(true)
    , _autoOptimize(true)
    , _optimizationIntervalMs(DEFAULT_OPTIMIZATION_INTERVAL_MS)
    , _learningRate(DEFAULT_LEARNING_RATE)
{
    _metrics.startTime = ::std::chrono::steady_clock::now();
}

PerformanceOptimizer::~PerformanceOptimizer()
{
    Shutdown();
}

bool PerformanceOptimizer::Initialize()
{
    if (_initialized)
        return true;

    TC_LOG_INFO("playerbot.optimizer", "Initializing Performance Optimizer");

    InitializeStrategies();

    // Register default tuning parameters
    RegisterTuningParameter("aggression", 0.5f, 0.0f, 1.0f);
    RegisterTuningParameter("defensiveness", 0.5f, 0.0f, 1.0f);
    RegisterTuningParameter("resource_conservation", 0.3f, 0.0f, 1.0f);
    RegisterTuningParameter("reaction_speed", 0.7f, 0.0f, 1.0f);
    RegisterTuningParameter("risk_tolerance", 0.4f, 0.0f, 1.0f);

    _initialized = true;
    TC_LOG_INFO("playerbot.optimizer", "Performance Optimizer initialized successfully");
    return true;
}

void PerformanceOptimizer::Shutdown()
{
    if (!_initialized)
        return;

    TC_LOG_INFO("playerbot.optimizer", "Shutting down Performance Optimizer");

    ::std::lock_guard profileLock(_profilesMutex);
    ::std::lock_guard paramLock(_parametersMutex);

    _profiles.clear();
    _optimizers.clear();
    _strategyDatabase.clear();
    _goalStrategies.clear();
    _tuningParameters.clear();
    _benchmarks.clear();

    _initialized = false;
}

void PerformanceOptimizer::InitializeStrategies()
{
    // Create default strategies for each goal
    for (uint8_t i = 0; i <= static_cast<uint8_t>(OptimizationGoal::BALANCE_ALL); ++i)
    {
        OptimizationGoal goal = static_cast<OptimizationGoal>(i);

        for (int j = 0; j < 3; ++j)  // 3 variants per goal
        {
            OptimizationStrategy strategy;
            strategy.goal = goal;
            strategy.name = "Strategy_" + ::std::to_string(i) + "_" + ::std::to_string(j);

            // Initialize with random parameters
            ::std::random_device rd;
            ::std::mt19937 gen(rd());
            ::std::uniform_real_distribution<float> dist(0.0f, 1.0f);

            for (int k = 0; k < 10; ++k)
                strategy.parameters.push_back(dist(gen));

            strategy.fitness = 0.0f;
            strategy.iterations = 0;

            _strategyDatabase.push_back(strategy);
            _goalStrategies[goal].push_back(strategy);
        }
    }
}

void PerformanceOptimizer::CreateProfile(uint32_t botGuid)
{
    ::std::lock_guard lock(_profilesMutex);

    if (_profiles.find(botGuid) == _profiles.end())
    {
        auto profile = ::std::make_shared<PerformanceProfile>(ObjectGuid(HighGuid::Player, botGuid));
        _profiles[botGuid] = profile;

        // Create evolutionary optimizer for this bot
        _optimizers[botGuid] = ::std::make_unique<EvolutionaryOptimizer>(EVOLUTION_POPULATION_SIZE);
        _optimizers[botGuid]->InitializePopulation(10);  // 10 parameters
    }
}

::std::shared_ptr<PerformanceProfile> PerformanceOptimizer::GetProfile(uint32_t botGuid) const
{
    ::std::lock_guard lock(_profilesMutex);

    auto it = _profiles.find(botGuid);
    if (it != _profiles.end())
        return it->second;

    return nullptr;
}

void PerformanceOptimizer::RecordPerformance(uint32_t botGuid, const PerformanceSample& sample)
{
    if (!_initialized)
        return;

    auto profile = GetOrCreateProfile(botGuid);
    if (profile)
    {
        profile->AddSample(sample);

        // Auto-optimize if enabled
    if (_autoOptimize)
        {
            static ::std::unordered_map<uint32_t, ::std::chrono::steady_clock::time_point> lastOptimization;
            auto now = ::std::chrono::steady_clock::now();

            if (lastOptimization[botGuid] + ::std::chrono::milliseconds(_optimizationIntervalMs) < now)
            {
                OptimizeBotPerformance(botGuid);
                lastOptimization[botGuid] = now;
            }
        }
    }
}

void PerformanceOptimizer::OptimizeBotPerformance(uint32_t botGuid)
{
    if (!_initialized)
        return;

    MEASURE_PERFORMANCE(MetricType::AI_DECISION_TIME, botGuid, "Optimization");

    auto profile = GetProfile(botGuid);
    if (!profile)
        return;

    // Use evolutionary optimization
    OptimizeWithEvolution(botGuid);

    // Update metrics
    _metrics.profilesOptimized++;

    float improvement = profile->GetImprovementRate();
    _metrics.averageImprovement = (_metrics.averageImprovement * 0.9f + improvement * 0.1f);

    if (improvement > _metrics.bestImprovement)
        _metrics.bestImprovement = improvement;

    TC_LOG_DEBUG("playerbot.optimizer", "Optimized bot %u, improvement: %.2f%%",
        botGuid, improvement * 100.0f);
}

void PerformanceOptimizer::OptimizeWithEvolution(uint32_t botGuid)
{
    auto it = _optimizers.find(botGuid);
    if (it == _optimizers.end())
        return;

    auto& optimizer = it->second;
    auto profile = GetProfile(botGuid);
    if (!profile)
        return;

    // Evaluation function
    auto fitnessFunc = [this, botGuid, profile](const StrategyChromosome& chromosome)
    {
        return EvaluateStrategyFitness(botGuid, chromosome);
    };

    // Run evolution for several generations
    for (uint32_t gen = 0; gen < EVOLUTION_GENERATIONS; ++gen)
    {
        optimizer->EvaluateFitness(fitnessFunc);
        optimizer->Evolve();
    }

    // Apply best strategy
    StrategyChromosome best = optimizer->GetBestChromosome();
    OptimizationStrategy strategy;
    strategy.parameters = best.genes;
    strategy.fitness = best.fitness;
    strategy.goal = OptimizationGoal::BALANCE_ALL;

    profile->SetStrategy(strategy);
    _metrics.strategiesEvaluated += EVOLUTION_GENERATIONS * EVOLUTION_POPULATION_SIZE;
}

float PerformanceOptimizer::EvaluateStrategyFitness(uint32_t botGuid, const StrategyChromosome& chromosome)
{
    auto profile = GetProfile(botGuid);
    if (!profile)
        return 0.0f;

    // Create strategy from chromosome
    OptimizationStrategy strategy;
    strategy.parameters = chromosome.genes;

    // Evaluate based on profile metrics
    float fitness = profile->EvaluateStrategy(strategy);

    // Add diversity bonus to prevent premature convergence
    float diversity = 0.0f;
    for (size_t i = 0; i < chromosome.genes.size(); ++i)
    {
        diversity += ::std::abs(chromosome.genes[i] - 0.5f);
    }
    diversity /= chromosome.genes.size();

    fitness += diversity * 0.1f;  // Small diversity bonus

    return fitness;
}

void PerformanceOptimizer::RegisterTuningParameter(const ::std::string& name, float initial,
                                                   float min, float max, float learningRate)
{
    ::std::lock_guard lock(_parametersMutex);

    TuningParameter param;
    param.name = name;
    param.value = initial;
    param.minValue = min;
    param.maxValue = max;
    param.learningRate = learningRate;
    param.momentum = 0.0f;

    _tuningParameters[name] = param;
}

float PerformanceOptimizer::GetTuningParameter(const ::std::string& name) const
{
    ::std::lock_guard lock(_parametersMutex);

    auto it = _tuningParameters.find(name);
    if (it != _tuningParameters.end())
        return it->second.value;

    return 0.0f;
}

::std::shared_ptr<PerformanceProfile> PerformanceOptimizer::GetOrCreateProfile(uint32_t botGuid)
{
    auto profile = GetProfile(botGuid);
    if (!profile)
    {
        CreateProfile(botGuid);
        profile = GetProfile(botGuid);
    }
    return profile;
}

// TuningParameter Implementation
void PerformanceOptimizer::TuningParameter::Update(float gradient)
{
    // Gradient descent with momentum
    momentum = momentum * 0.9f + gradient * 0.1f;
    value -= learningRate * momentum;
    value = ::std::clamp(value, minValue, maxValue);
}

float PerformanceOptimizer::TuningParameter::Normalize() const
{
    if (maxValue == minValue)
        return 0.0f;
    return (value - minValue) / (maxValue - minValue);
}

// ScopedPerformanceMeasurement Implementation
ScopedPerformanceMeasurement::ScopedPerformanceMeasurement(uint32_t botGuid, const ::std::string& operation)
    : _botGuid(botGuid)
    , _operation(operation)
    , _success(false)
{
    _startTime = ::std::chrono::steady_clock::now();
    _sample.timestamp = _startTime.time_since_epoch().count();
}

ScopedPerformanceMeasurement::~ScopedPerformanceMeasurement()
{
    auto endTime = ::std::chrono::steady_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - _startTime);

    if (_success)
        _sample.successfulActions++;
    else
        _sample.failedActions++;

    _sample.abilitiesUsed++;

    sPerformanceOptimizer.RecordPerformance(_botGuid, _sample);
}

void ScopedPerformanceMeasurement::MarkSuccess()
{
    _success = true;
}

} // namespace Playerbot