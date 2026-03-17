/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * Adaptive Query Optimization for 5000 Bot Scalability
 */

#ifndef TRINITYCORE_SPATIAL_QUERY_OPTIMIZER_H
#define TRINITYCORE_SPATIAL_QUERY_OPTIMIZER_H

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include <atomic>
#include <chrono>
#include <array>
#include <vector>

namespace Playerbot
{

/**
 * Performance metrics for spatial queries
 * Tracks per-frame and rolling window statistics
 */
class SpatialQueryMetrics
{
public:
    struct FrameMetrics
    {
        uint32 queryCount;
        uint32 cacheHits;
        uint32 gridScans;
        uint64 totalTimeUs;
        uint64 maxTimeUs;
    };

    void RecordQuery(uint32 timeUs, bool cacheHit);
    void RecordGridScan(uint32 timeUs);
    void NextFrame();

    FrameMetrics GetCurrentFrame() const;
    FrameMetrics GetRollingAverage() const;
    float GetCacheHitRate() const;
    uint32 GetAverageQueryTimeUs() const;

private:
    static constexpr size_t WINDOW_SIZE = 60; // 1 second at 60 FPS

    ::std::atomic<uint32> _currentFrame{0};
    ::std::array<FrameMetrics, WINDOW_SIZE> _frames{};
    ::std::atomic<uint64> _totalQueries{0};
    ::std::atomic<uint64> _totalCacheHits{0};
};

/**
 * Adaptive throttling based on server load
 * Automatically adjusts query rates to maintain performance
 */
class AdaptiveThrottler
{
public:
    AdaptiveThrottler();

    // Check if query should be throttled
    bool ShouldThrottle(uint32 botPriority);

    // Update throttling based on metrics
    void UpdateThrottling(const SpatialQueryMetrics::FrameMetrics& metrics);

    // Get current throttle settings
    struct ThrottleSettings
    {
        uint32 maxQueriesPerFrame;
        uint32 minQueryIntervalMs;
        float throttleRatio;      // 0.0 = no throttle, 1.0 = full throttle
        bool emergencyMode;        // True when system is overloaded
    };

    ThrottleSettings GetSettings() const;

private:
    // Adaptive parameters
    ::std::atomic<uint32> _maxQueriesPerFrame{500};
    ::std::atomic<uint32> _minQueryIntervalMs{100};
    ::std::atomic<float> _throttleRatio{0.0f};
    ::std::atomic<bool> _emergencyMode{false};

    // Load tracking
    ::std::atomic<uint32> _recentQueryCount{0};
    ::std::atomic<uint64> _recentQueryTimeUs{0};

    // Frame timing
    ::std::chrono::steady_clock::time_point _lastFrameTime;
    uint32 _frameTimeUs{16667}; // Target 60 FPS

    void CalculateThrottleRatio(uint32 queryTimeUs, uint32 queryCount);
};

/**
 * Query batching and deduplication
 * Combines similar queries to reduce redundant work
 */
class QueryBatcher
{
public:
    struct BatchedQuery
    {
        float x, y, z;
        float range;
        uint32 zoneId;
        ::std::vector<ObjectGuid> requestingBots;
    };

    // Add query to batch
    void AddQuery(ObjectGuid bot, float x, float y, float z, float range, uint32 zoneId);

    // Process batched queries
    ::std::vector<BatchedQuery> GetBatchedQueries();

    // Clear processed batches
    void ClearBatches();

private:
    static constexpr float POSITION_EPSILON = 5.0f;  // Positions within 5 yards are considered same
    static constexpr float RANGE_EPSILON = 2.0f;     // Ranges within 2 yards are considered same

    ::std::vector<BatchedQuery> _pendingBatches;
    Playerbot::OrderedMutex<Playerbot::LockOrder::SPATIAL_GRID> _batchMutex;

    BatchedQuery* FindSimilarQuery(float x, float y, float z, float range, uint32 zoneId);
};

/**
 * Main optimizer coordinating all optimization strategies
 */
class SpatialQueryOptimizer
{
public:
    static SpatialQueryOptimizer& Instance();

    // Query interface with optimization
    struct OptimizedQuery
    {
        bool useCache;
        bool throttled;
        bool batched;
        uint32 delayMs;
        ::std::vector<HostileEntry> cachedResults;
    };

    OptimizedQuery OptimizeQuery(
        Player* bot,
        float range,
        uint32 priority = 100);

    // Frame update
    void OnFrameStart();
    void OnFrameEnd();

    // Configuration
    struct OptimizerConfig
    {
        bool enableCaching = true;
        bool enableBatching = true;
        bool enableThrottling = true;
        bool enableEventDriven = true;
        uint32 maxBotsPerFrame = 500;
        uint32 cacheLifetimeMs = 500;
    };

    void SetConfig(const OptimizerConfig& config);
    OptimizerConfig GetConfig() const;

    // Statistics
    struct OptimizerStats
    {
        SpatialQueryMetrics::FrameMetrics frameMetrics;
        AdaptiveThrottler::ThrottleSettings throttleSettings;
        uint32 batchedQueries;
        uint32 throttledQueries;
        float optimizationRatio; // Queries avoided / total requests
    };

    OptimizerStats GetStatistics() const;

private:
    SpatialQueryOptimizer();
    ~SpatialQueryOptimizer();

    // Components
    ::std::unique_ptr<SpatialQueryMetrics> _metrics;
    ::std::unique_ptr<AdaptiveThrottler> _throttler;
    ::std::unique_ptr<QueryBatcher> _batcher;

    // Configuration
    OptimizerConfig _config;

    // Per-frame tracking
    ::std::atomic<uint32> _frameQueryCount{0};
    ::std::atomic<uint32> _frameThrottledCount{0};
    ::std::atomic<uint32> _frameBatchedCount{0};

    // Deleted operations
    SpatialQueryOptimizer(const SpatialQueryOptimizer&) = delete;
    SpatialQueryOptimizer& operator=(const SpatialQueryOptimizer&) = delete;
};

/**
 * Priority calculator for bots
 * Higher priority = less throttling
 */
class BotPriorityCalculator
{
public:
    static uint32 CalculatePriority(Player* bot);

private:
    static uint32 GetCombatPriority(Player* bot);
    static uint32 GetProximityPriority(Player* bot);
    static uint32 GetRolePriority(Player* bot);
    static uint32 GetGroupPriority(Player* bot);
};

} // namespace Playerbot

#endif // TRINITYCORE_SPATIAL_QUERY_OPTIMIZER_H