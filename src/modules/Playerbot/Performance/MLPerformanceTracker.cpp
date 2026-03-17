/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "MLPerformanceTracker.h"
#include "BotPerformanceMonitor.h"
#include "Log.h"
#include <sstream>
#include <algorithm>

namespace Playerbot
{

// MLPerformanceTracker Implementation
MLPerformanceTracker::MLPerformanceTracker()
    : _initialized(false)
    , _enabled(true)
    , _maxMLOverheadPercent(1.0f)
    , _targetInferenceTimeUs(1000)
    , _totalMLOperations(0)
    , _totalMLTimeUs(0)
{
    _startTime = ::std::chrono::steady_clock::now();
}

MLPerformanceTracker::~MLPerformanceTracker()
{
    Shutdown();
}

bool MLPerformanceTracker::Initialize()
{
    if (_initialized)
        return true;

    TC_LOG_INFO("playerbot.ml", "Initializing ML Performance Tracker");

    // Initialize operation metrics
    for (uint8_t i = 0; i <= static_cast<uint8_t>(MLOperationType::COLLECTIVE_UPDATE); ++i)
    {
        _operationMetrics[static_cast<MLOperationType>(i)] = OperationMetrics();
    }

    _initialized = true;
    TC_LOG_INFO("playerbot.ml", "ML Performance Tracker initialized successfully");
    return true;
}

void MLPerformanceTracker::Shutdown()
{
    if (!_initialized)
        return;

    TC_LOG_INFO("playerbot.ml", "Shutting down ML Performance Tracker");

    // Generate final report
    ::std::string report;
    GenerateMLPerformanceReport(report);
    TC_LOG_INFO("playerbot.ml", "Final ML Performance Report:\n%s", report.c_str());

    _initialized = false;
}

void MLPerformanceTracker::StartOperation(uint32_t botGuid, MLOperationType operation, const ::std::string& context)
{
    if (!_enabled)
        return;

    ::std::lock_guard lock(_operationsMutex);

    ActiveOperation op;
    op.type = operation;
    op.startTime = GetCurrentTimeMicroseconds();
    op.context = context;

    _activeOperations[botGuid][operation] = op;
}

void MLPerformanceTracker::EndOperation(uint32_t botGuid, MLOperationType operation, bool success, uint32_t samplesProcessed)
{
    if (!_enabled)
        return;

    ::std::lock_guard lock(_operationsMutex);

    auto botIt = _activeOperations.find(botGuid);
    if (botIt == _activeOperations.end())
        return;

    auto opIt = botIt->second.find(operation);
    if (opIt == botIt->second.end())
        return;

    // Create performance sample
    MLPerformanceSample sample;
    sample.operation = operation;
    sample.startTime = opIt->second.startTime;
    sample.endTime = GetCurrentTimeMicroseconds();
    sample.samplesProcessed = samplesProcessed;
    sample.success = success;
    sample.context = opIt->second.context;
    sample.memoryUsed = 0;  // Set separately if needed
    sample.accuracy = 0.0f;  // Set separately if applicable

    // Remove from active operations
    botIt->second.erase(opIt);
    if (botIt->second.empty())
        _activeOperations.erase(botIt);

    // Record the sample
    RecordSample(sample);
}

void MLPerformanceTracker::RecordSample(const MLPerformanceSample& sample)
{
    if (!_enabled)
        return;

    // Update operation metrics
    UpdateOperationMetrics(sample);

    // Store sample
    {
        ::std::lock_guard lock(_samplesMutex);
        _recentSamples.push_back(sample);

        // Maintain sample buffer size
    if (_recentSamples.size() > MAX_SAMPLES)
        {
            _recentSamples.pop_front();
        }
    }

    // Update global metrics
    _totalMLOperations++;
    _totalMLTimeUs += sample.GetDuration();
}

void MLPerformanceTracker::UpdateOperationMetrics(const MLPerformanceSample& sample)
{
    auto& metrics = _operationMetrics[sample.operation];

    metrics.totalCount++;
    metrics.totalTimeUs += sample.GetDuration();
    metrics.totalSamples += sample.samplesProcessed;

    if (!sample.success)
        metrics.failureCount++;

    // Update min/max
    uint64_t duration = sample.GetDuration();
    uint64_t currentMax = metrics.maxTimeUs.load();
    while (duration > currentMax && !metrics.maxTimeUs.compare_exchange_weak(currentMax, duration));

    uint64_t currentMin = metrics.minTimeUs.load();
    while (duration < currentMin && !metrics.minTimeUs.compare_exchange_weak(currentMin, duration));
}

void MLPerformanceTracker::RecordPrediction(uint32_t botGuid, bool correct)
{
    if (!_enabled)
        return;

    ::std::lock_guard lock(_statsMutex);

    _botStatistics[botGuid].totalPredictions++;
    if (correct)
        _botStatistics[botGuid].correctPredictions++;
}

void MLPerformanceTracker::RecordTrainingStep(uint32_t botGuid, float loss, float reward)
{
    if (!_enabled)
        return;

    ::std::lock_guard lock(_statsMutex);

    auto& stats = _botStatistics[botGuid];
    stats.totalTrainingSteps++;

    // Update average loss (exponential moving average)
    float alpha = 0.01f;
    float currentAvgLoss = stats.averageLoss.load();
    stats.averageLoss = currentAvgLoss * (1.0f - alpha) + loss * alpha;

    // Update average reward
    float currentAvgReward = stats.averageReward.load();
    stats.averageReward = currentAvgReward * (1.0f - alpha) + reward * alpha;
}

void MLPerformanceTracker::RecordModelUpdate(uint32_t botGuid, uint64_t experienceCount)
{
    if (!_enabled)
        return;

    ::std::lock_guard lock(_statsMutex);

    auto& stats = _botStatistics[botGuid];
    stats.modelUpdates++;
    stats.totalExperiences = experienceCount;
}

void MLPerformanceTracker::RecordInference(uint32_t botGuid, uint64_t durationUs)
{
    if (!_enabled)
        return;

    ::std::lock_guard lock(_statsMutex);
    _botStatistics[botGuid].totalInferenceTimeUs += durationUs;
}

void MLPerformanceTracker::RecordTraining(uint32_t botGuid, uint64_t durationUs)
{
    if (!_enabled)
        return;

    ::std::lock_guard lock(_statsMutex);
    _botStatistics[botGuid].totalTrainingTimeUs += durationUs;
}

void MLPerformanceTracker::RecordMemoryUsage(uint32_t botGuid, MLOperationType operation, uint64_t bytes)
{
    if (!_enabled)
        return;

    ::std::lock_guard lock(_memoryMutex);

    uint64_t oldUsage = _memoryUsage[botGuid][operation];
    _memoryUsage[botGuid][operation] = bytes;

    // Update total
    _totalMLMemory = _totalMLMemory - oldUsage + bytes;
}

uint64_t MLPerformanceTracker::GetTotalMLMemoryUsage() const
{
    return _totalMLMemory.load();
}

uint64_t MLPerformanceTracker::GetBotMLMemoryUsage(uint32_t botGuid) const
{
    ::std::lock_guard lock(_memoryMutex);

    auto it = _memoryUsage.find(botGuid);
    if (it == _memoryUsage.end())
        return 0;

    uint64_t total = 0;
    for (const auto& [operation, bytes] : it->second)
    {
        total += bytes;
    }

    return total;
}

ModelStatistics MLPerformanceTracker::GetModelStatistics(uint32_t botGuid) const
{
    ::std::lock_guard lock(_statsMutex);

    auto it = _botStatistics.find(botGuid);
    if (it != _botStatistics.end())
        return it->second;

    return ModelStatistics();
}

float MLPerformanceTracker::GetOperationAverageTime(MLOperationType operation) const
{
    auto it = _operationMetrics.find(operation);
    if (it == _operationMetrics.end())
        return 0.0f;

    uint64_t count = it->second.totalCount.load();
    uint64_t totalTime = it->second.totalTimeUs.load();

    return count > 0 ? (totalTime / static_cast<float>(count)) : 0.0f;
}

float MLPerformanceTracker::GetOperationThroughput(MLOperationType operation) const
{
    auto it = _operationMetrics.find(operation);
    if (it == _operationMetrics.end())
        return 0.0f;

    uint64_t totalTime = it->second.totalTimeUs.load();
    uint64_t totalSamples = it->second.totalSamples.load();

    // Samples per second
    return totalTime > 0 ? (totalSamples * 1000000.0f / totalTime) : 0.0f;
}

uint32_t MLPerformanceTracker::GetActiveMLBots() const
{
    ::std::lock_guard lock(_statsMutex);
    return _botStatistics.size();
}

bool MLPerformanceTracker::IsMLPerformanceAcceptable() const
{
    // Check CPU usage
    if (CalculateMLCpuUsage() > _maxMLOverheadPercent)
        return false;

    // Check inference times
    float avgInferenceTime = GetOperationAverageTime(MLOperationType::NEURAL_FORWARD_PASS);
    if (avgInferenceTime > _targetInferenceTimeUs)
        return false;

    // Check memory usage
    if (_totalMLMemory > _limits.maxMemoryPerBot * GetActiveMLBots())
        return false;

    return true;
}

::std::vector<::std::string> MLPerformanceTracker::GetPerformanceIssues() const
{
    ::std::vector<::std::string> issues;

    // Check CPU usage
    float cpuUsage = CalculateMLCpuUsage();
    if (cpuUsage > _maxMLOverheadPercent)
    {
        ::std::stringstream ss;
        ss << "ML CPU usage too high: " << cpuUsage << "% (max: " << _maxMLOverheadPercent << "%)";
        issues.push_back(ss.str());
    }

    // Check inference time
    float avgInferenceTime = GetOperationAverageTime(MLOperationType::NEURAL_FORWARD_PASS);
    if (avgInferenceTime > _limits.maxInferenceTimeUs)
    {
        ::std::stringstream ss;
        ss << "Inference time too slow: " << avgInferenceTime << "us (target: " << _limits.maxInferenceTimeUs << "us)";
        issues.push_back(ss.str());
    }

    // Check training time
    float avgTrainingTime = GetOperationAverageTime(MLOperationType::BATCH_LEARNING);
    if (avgTrainingTime > _limits.maxTrainingTimeUs)
    {
        ::std::stringstream ss;
        ss << "Training time too slow: " << avgTrainingTime << "us (max: " << _limits.maxTrainingTimeUs << "us)";
        issues.push_back(ss.str());
    }

    // Check memory usage
    uint64_t avgMemoryPerBot = GetActiveMLBots() > 0 ? (_totalMLMemory / GetActiveMLBots()) : 0;
    if (avgMemoryPerBot > _limits.maxMemoryPerBot)
    {
        ::std::stringstream ss;
        ss << "ML memory usage too high: " << (avgMemoryPerBot / 1048576.0f) << "MB per bot (max: "
            << (_limits.maxMemoryPerBot / 1048576.0f) << "MB)";
        issues.push_back(ss.str());
    }

    return issues;
}

void MLPerformanceTracker::GenerateMLPerformanceReport(::std::string& report) const
{
    ::std::stringstream ss;

    ss << "===== ML Performance Report =====\n";

    // System overview
    auto now = ::std::chrono::steady_clock::now();
    auto runtime = ::std::chrono::duration_cast<::std::chrono::seconds>(now - _startTime).count();
    ss << "Runtime: " << runtime << " seconds\n";
    ss << "Active ML Bots: " << GetActiveMLBots() << "\n";
    ss << "Total ML Operations: " << _totalMLOperations.load() << "\n";
    ss << "Total ML Time: " << (_totalMLTimeUs.load() / 1000000.0f) << " seconds\n";
    ss << "ML CPU Usage: " << CalculateMLCpuUsage() << "%\n";
    ss << "Total ML Memory: " << (_totalMLMemory.load() / 1048576.0f) << " MB\n\n";

    // Operation performance
    ss << "Operation Performance:\n";
    for (const auto& [operation, metrics] : _operationMetrics)
    {
        uint64_t count = metrics.totalCount.load();
        if (count == 0)
            continue;

        ::std::string opName;
        switch (operation)
        {
            case MLOperationType::FEATURE_EXTRACTION: opName = "Feature Extraction"; break;
            case MLOperationType::NEURAL_FORWARD_PASS: opName = "Neural Forward Pass"; break;
            case MLOperationType::NEURAL_BACKWARD_PASS: opName = "Neural Backward Pass"; break;
            case MLOperationType::Q_VALUE_CALCULATION: opName = "Q-Value Calculation"; break;
            case MLOperationType::ACTION_SELECTION: opName = "Action Selection"; break;
            case MLOperationType::EXPERIENCE_STORAGE: opName = "Experience Storage"; break;
            case MLOperationType::BATCH_LEARNING: opName = "Batch Learning"; break;
            case MLOperationType::PATTERN_RECOGNITION: opName = "Pattern Recognition"; break;
            case MLOperationType::DIFFICULTY_ADJUSTMENT: opName = "Difficulty Adjustment"; break;
            case MLOperationType::PERFORMANCE_OPTIMIZATION: opName = "Performance Optimization"; break;
            case MLOperationType::MODEL_SAVE_LOAD: opName = "Model Save/Load"; break;
            case MLOperationType::COLLECTIVE_UPDATE: opName = "Collective Update"; break;
            default: opName = "Unknown"; break;
        }

        float avgTime = metrics.totalTimeUs.load() / static_cast<float>(count);
        float successRate = 100.0f * (1.0f - metrics.failureCount.load() / static_cast<float>(count));
        float throughput = GetOperationThroughput(operation);

        ss << "  " << opName << ":\n";
        ss << "    Count: " << count << "\n";
        ss << "    Avg Time: " << avgTime << " us\n";
        ss << "    Min/Max Time: " << metrics.minTimeUs.load() << "/" << metrics.maxTimeUs.load() << " us\n";
        ss << "    Success Rate: " << successRate << "%\n";
        ss << "    Throughput: " << throughput << " samples/sec\n";
    }

    ss << "\n";

    // Model statistics summary
    ss << "Model Performance Summary:\n";

    uint32_t totalBots = 0;
    float totalAccuracy = 0.0f;
    float totalAvgLoss = 0.0f;
    float totalAvgReward = 0.0f;

    {
        ::std::lock_guard lock(_statsMutex);
        for (const auto& [botGuid, stats] : _botStatistics)
        {
            totalBots++;
            totalAccuracy += stats.GetAccuracy();
            totalAvgLoss += stats.averageLoss.load();
            totalAvgReward += stats.averageReward.load();
        }
    }

    if (totalBots > 0)
    {
        ss << "  Average Accuracy: " << (totalAccuracy / totalBots * 100.0f) << "%\n";
        ss << "  Average Loss: " << (totalAvgLoss / totalBots) << "\n";
        ss << "  Average Reward: " << (totalAvgReward / totalBots) << "\n";
    }

    // Performance issues
    auto issues = GetPerformanceIssues();
    if (!issues.empty())
    {
        ss << "\nPerformance Issues:\n";
        for (const auto& issue : issues)
        {
            ss << "  - " << issue << "\n";
        }
    }

    // Optimization suggestions
    auto suggestions = GetOptimizationSuggestions();
    if (!suggestions.empty())
    {
        ss << "\nOptimization Suggestions:\n";
        for (const auto& suggestion : suggestions)
        {
            ss << "  - " << suggestion << "\n";
        }
    }

    report = ss.str();
}

::std::vector<::std::string> MLPerformanceTracker::GetOptimizationSuggestions() const
{
    ::std::vector<::std::string> suggestions;

    // Check if should reduce complexity
    if (ShouldReduceMLComplexity())
    {
        suggestions.push_back("Consider reducing neural network size or using simpler models");
        suggestions.push_back("Reduce experience replay buffer size");
        suggestions.push_back("Decrease learning frequency");
    }

    // Check if can increase batch size
    if (ShouldIncreaseMLBatchSize())
    {
        suggestions.push_back("Increase batch size for more efficient GPU/CPU utilization");
        suggestions.push_back("Use experience prioritization to reduce sample count");
    }

    // Check inference optimization
    float avgInferenceTime = GetOperationAverageTime(MLOperationType::NEURAL_FORWARD_PASS);
    if (avgInferenceTime > _targetInferenceTimeUs * 0.8f)
    {
        suggestions.push_back("Consider model quantization or pruning");
        suggestions.push_back("Implement inference caching for similar states");
        suggestions.push_back("Use action repeat to reduce inference frequency");
    }

    // Check memory optimization
    uint64_t avgMemoryPerBot = GetActiveMLBots() > 0 ? (_totalMLMemory / GetActiveMLBots()) : 0;
    if (avgMemoryPerBot > _limits.maxMemoryPerBot * 0.8f)
    {
        suggestions.push_back("Reduce experience replay buffer size");
        suggestions.push_back("Implement experience compression");
        suggestions.push_back("Share models between similar bots");
    }

    return suggestions;
}

bool MLPerformanceTracker::ShouldReduceMLComplexity() const
{
    // Check if ML overhead is too high
    if (CalculateMLCpuUsage() > _maxMLOverheadPercent * 0.8f)
        return true;

    // Check if inference is too slow
    float avgInferenceTime = GetOperationAverageTime(MLOperationType::NEURAL_FORWARD_PASS);
    if (avgInferenceTime > _limits.maxInferenceTimeUs)
        return true;

    // Check if training is too slow
    float avgTrainingTime = GetOperationAverageTime(MLOperationType::BATCH_LEARNING);
    if (avgTrainingTime > _limits.maxTrainingTimeUs)
        return true;

    return false;
}

bool MLPerformanceTracker::ShouldIncreaseMLBatchSize() const
{
    // Check if we have CPU headroom
    if (CalculateMLCpuUsage() < _maxMLOverheadPercent * 0.5f)
    {
        // Check if current batch operations are fast
        float avgBatchTime = GetOperationAverageTime(MLOperationType::BATCH_LEARNING);
        if (avgBatchTime < _limits.maxTrainingTimeUs * 0.5f)
            return true;
    }

    return false;
}

uint64_t MLPerformanceTracker::GetCurrentTimeMicroseconds() const
{
    return ::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now().time_since_epoch()).count();
}

float MLPerformanceTracker::CalculateMLCpuUsage() const
{
    auto now = ::std::chrono::steady_clock::now();
    auto runtime = ::std::chrono::duration_cast<::std::chrono::microseconds>(now - _startTime).count();

    if (runtime == 0)
        return 0.0f;

    uint64_t totalMLTime = _totalMLTimeUs.load();
    return (totalMLTime * 100.0f) / runtime;
}

// ScopedMLOperation Implementation
ScopedMLOperation::ScopedMLOperation(uint32_t botGuid, MLOperationType operation, const ::std::string& context)
    : _botGuid(botGuid)
    , _operation(operation)
    , _context(context)
    , _samplesProcessed(1)
    , _success(true)
    , _memoryUsed(0)
{
    _startTime = ::std::chrono::steady_clock::now();
    sMLPerformanceTracker.StartOperation(_botGuid, _operation, _context);
}

ScopedMLOperation::~ScopedMLOperation()
{
    auto endTime = ::std::chrono::steady_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - _startTime);

    sMLPerformanceTracker.EndOperation(_botGuid, _operation, _success, _samplesProcessed);
    sMLPerformanceTracker.RecordInference(_botGuid, duration.count());

    if (_memoryUsed > 0)
    {
        sMLPerformanceTracker.RecordMemoryUsage(_botGuid, _operation, _memoryUsed);
    }
}

void ScopedMLOperation::RecordMemoryUsed(uint64_t bytes)
{
    _memoryUsed = bytes;
}

} // namespace Playerbot