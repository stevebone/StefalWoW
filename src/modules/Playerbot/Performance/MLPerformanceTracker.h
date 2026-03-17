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
#include <mutex>
#include <memory>
#include <vector>
#include <deque>

namespace Playerbot
{

// ML operation types for tracking
enum class MLOperationType : uint8
{
    FEATURE_EXTRACTION,
    NEURAL_FORWARD_PASS,
    NEURAL_BACKWARD_PASS,
    Q_VALUE_CALCULATION,
    ACTION_SELECTION,
    EXPERIENCE_STORAGE,
    BATCH_LEARNING,
    PATTERN_RECOGNITION,
    DIFFICULTY_ADJUSTMENT,
    PERFORMANCE_OPTIMIZATION,
    MODEL_SAVE_LOAD,
    COLLECTIVE_UPDATE
};

// ML performance sample
struct MLPerformanceSample
{
    MLOperationType operation;
    uint64_t startTime;        // Microseconds
    uint64_t endTime;          // Microseconds
    uint64_t memoryUsed;       // Bytes
    uint32_t samplesProcessed;
    float accuracy;            // For operations with accuracy metrics
    bool success;
    ::std::string context;

    uint64_t GetDuration() const { return endTime - startTime; }
    float GetThroughput() const
    {
        uint64_t duration = GetDuration();
        return duration > 0 ? (samplesProcessed * 1000000.0f / duration) : 0.0f;
    }
};

// ML model statistics
struct ModelStatistics
{
    ::std::atomic<uint64_t> totalPredictions{0};
    ::std::atomic<uint64_t> correctPredictions{0};
    ::std::atomic<uint64_t> totalTrainingSteps{0};
    ::std::atomic<float> averageLoss{0.0f};
    ::std::atomic<float> averageReward{0.0f};
    ::std::atomic<uint64_t> totalExperiences{0};
    ::std::atomic<uint64_t> modelUpdates{0};
    ::std::atomic<uint64_t> totalInferenceTimeUs{0};
    ::std::atomic<uint64_t> totalTrainingTimeUs{0};

    float GetAccuracy() const
    {
        uint64_t total = totalPredictions.load();
        return total > 0 ? (correctPredictions.load() / static_cast<float>(total)) : 0.0f;
    }

    float GetAverageInferenceTime() const
    {
        uint64_t predictions = totalPredictions.load();
        return predictions > 0 ? (totalInferenceTimeUs.load() / static_cast<float>(predictions)) : 0.0f;
    }
};

// ML performance tracker
class TC_GAME_API MLPerformanceTracker
{
public:
    static MLPerformanceTracker& Instance()
    {
        static MLPerformanceTracker instance;
        return instance;
    }

    // Initialization
    bool Initialize();
    void Shutdown();
    bool IsEnabled() const { return _enabled; }

    // Performance recording
    void StartOperation(uint32_t botGuid, MLOperationType operation, const ::std::string& context = "");
    void EndOperation(uint32_t botGuid, MLOperationType operation, bool success = true,
                     uint32_t samplesProcessed = 1);
    void RecordSample(const MLPerformanceSample& sample);

    // Model performance tracking
    void RecordPrediction(uint32_t botGuid, bool correct);
    void RecordTrainingStep(uint32_t botGuid, float loss, float reward);
    void RecordModelUpdate(uint32_t botGuid, uint64_t experienceCount);
    void RecordInference(uint32_t botGuid, uint64_t durationUs);
    void RecordTraining(uint32_t botGuid, uint64_t durationUs);

    // Memory tracking
    void RecordMemoryUsage(uint32_t botGuid, MLOperationType operation, uint64_t bytes);
    uint64_t GetTotalMLMemoryUsage() const;
    uint64_t GetBotMLMemoryUsage(uint32_t botGuid) const;

    // Performance metrics
    ModelStatistics GetModelStatistics(uint32_t botGuid) const;
    float GetOperationAverageTime(MLOperationType operation) const;
    float GetOperationThroughput(MLOperationType operation) const;
    uint32_t GetActiveMLBots() const;

    // Performance analysis
    bool IsMLPerformanceAcceptable() const;
    ::std::vector<::std::string> GetPerformanceIssues() const;
    void GenerateMLPerformanceReport(::std::string& report) const;

    // Optimization suggestions
    ::std::vector<::std::string> GetOptimizationSuggestions() const;
    bool ShouldReduceMLComplexity() const;
    bool ShouldIncreaseMLBatchSize() const;

    // Configuration
    void SetMaxMLOverhead(float percentCPU) { _maxMLOverheadPercent = percentCPU; }
    void SetTargetInferenceTime(uint64_t microseconds) { _targetInferenceTimeUs = microseconds; }
    void SetEnabled(bool enabled) { _enabled = enabled; }

    // Performance limits
    struct PerformanceLimits
    {
        uint64_t maxInferenceTimeUs = 1000;      // 1ms
        uint64_t maxTrainingTimeUs = 10000;      // 10ms
        uint64_t maxMemoryPerBot = 10485760;     // 10MB
        float maxCpuPercent = 1.0f;              // 1% CPU per bot
        uint32_t maxExperiencesPerBot = 10000;
        uint32_t maxBatchSize = 64;
    };

    PerformanceLimits GetLimits() const { return _limits; }
    void SetLimits(const PerformanceLimits& limits) { _limits = limits; }

private:
    MLPerformanceTracker();
    ~MLPerformanceTracker();

    // System state
    bool _initialized;
    ::std::atomic<bool> _enabled;

    // Active operations tracking
    struct ActiveOperation
    {
        MLOperationType type;
        uint64_t startTime;
        ::std::string context;
    };

    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BEHAVIOR_MANAGER> _operationsMutex;
    ::std::unordered_map<uint32_t, ::std::unordered_map<MLOperationType, ActiveOperation>> _activeOperations;

    // Performance samples
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BEHAVIOR_MANAGER> _samplesMutex;
    ::std::deque<MLPerformanceSample> _recentSamples;
    static constexpr size_t MAX_SAMPLES = 10000;

    // Model statistics per bot
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BEHAVIOR_MANAGER> _statsMutex;
    ::std::unordered_map<uint32_t, ModelStatistics> _botStatistics;

    // Operation metrics
    struct OperationMetrics
    {
        ::std::atomic<uint64_t> totalCount{0};
        ::std::atomic<uint64_t> totalTimeUs{0};
        ::std::atomic<uint64_t> totalSamples{0};
        ::std::atomic<uint64_t> failureCount{0};
        ::std::atomic<uint64_t> maxTimeUs{0};
        ::std::atomic<uint64_t> minTimeUs{UINT64_MAX};
    };

    ::std::unordered_map<MLOperationType, OperationMetrics> _operationMetrics;

    // Memory tracking
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BEHAVIOR_MANAGER> _memoryMutex;
    ::std::unordered_map<uint32_t, ::std::unordered_map<MLOperationType, uint64_t>> _memoryUsage;
    ::std::atomic<uint64_t> _totalMLMemory{0};

    // Performance configuration
    float _maxMLOverheadPercent;
    uint64_t _targetInferenceTimeUs;
    PerformanceLimits _limits;

    // System metrics
    ::std::chrono::steady_clock::time_point _startTime;
    ::std::atomic<uint64_t> _totalMLOperations{0};
    ::std::atomic<uint64_t> _totalMLTimeUs{0};

    // Helper methods
    uint64_t GetCurrentTimeMicroseconds() const;
    void UpdateOperationMetrics(const MLPerformanceSample& sample);
    void CleanupOldSamples();
    void AnalyzePerformanceTrends();
    float CalculateMLCpuUsage() const;
};

// RAII helper for ML operation timing
class TC_GAME_API ScopedMLOperation
{
public:
    ScopedMLOperation(uint32_t botGuid, MLOperationType operation, const ::std::string& context = "");
    ~ScopedMLOperation();

    void SetSamplesProcessed(uint32_t count) { _samplesProcessed = count; }
    void SetSuccess(bool success) { _success = success; }
    void RecordMemoryUsed(uint64_t bytes);

private:
    uint32_t _botGuid;
    MLOperationType _operation;
    ::std::string _context;
    uint32_t _samplesProcessed;
    bool _success;
    uint64_t _memoryUsed;
    ::std::chrono::steady_clock::time_point _startTime;
};

#define sMLPerformanceTracker MLPerformanceTracker::Instance()

#define TRACK_ML_OPERATION(botGuid, operation) \
    ScopedMLOperation _ml_op(botGuid, operation)

#define TRACK_ML_OPERATION_CTX(botGuid, operation, context) \
    ScopedMLOperation _ml_op(botGuid, operation, context)

} // namespace Playerbot