/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * @file BracketFlowPredictor.h
 * @brief Predicts bot flow between level brackets
 *
 * The BracketFlowPredictor tracks and predicts bot transitions between
 * level brackets. This information helps the lifecycle controller make
 * proactive decisions about bot creation and retirement.
 *
 * Key Responsibilities:
 * 1. Track bot level-up events and bracket transitions
 * 2. Calculate average time bots spend in each bracket
 * 3. Predict future bracket populations
 * 4. Identify bots likely to leave brackets soon
 * 5. Support retirement priority calculations
 *
 * Thread Safety:
 * - All public methods are thread-safe
 * - Uses parallel hashmap for concurrent access
 * - Atomic operations for counters
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "../BotLifecycleState.h"
#include "Character/BotLevelDistribution.h"
#include <parallel_hashmap/phmap.h>
#include <atomic>
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <array>

namespace Playerbot
{

/**
 * @brief Prediction of bracket flow for a time window
 */
struct FlowPrediction
{
    /**
     * @brief The bracket being predicted
     */
    LevelBracket const* bracket = nullptr;

    /**
     * @brief Predicted number of bots leaving this bracket
     */
    uint32 predictedOutflow = 0;

    /**
     * @brief Predicted number of bots entering this bracket
     */
    uint32 predictedInflow = 0;

    /**
     * @brief Net change (outflow - inflow)
     */
    int32 netChange = 0;

    /**
     * @brief Time until bracket would be empty (if no creation)
     */
    std::chrono::seconds timeToEmpty{0};

    /**
     * @brief Confidence level (0.0-1.0)
     */
    float confidence = 0.0f;

    /**
     * @brief Number of samples used for prediction
     */
    uint32 sampleCount = 0;
};

/**
 * @brief Entry tracking when a bot entered its current bracket
 */
struct BotBracketEntry
{
    ObjectGuid botGuid;
    uint32 level = 0;
    ExpansionTier tier = ExpansionTier::Starting;
    std::chrono::system_clock::time_point entryTime;
    uint32 bracketIndex = 0;
};

/**
 * @brief Statistics for bracket transitions
 */
struct BracketTransitionStats
{
    /**
     * @brief Average time spent in bracket (seconds)
     */
    uint32 avgTimeInBracketSeconds = 0;

    /**
     * @brief Sample count for the average
     */
    uint32 sampleCount = 0;

    /**
     * @brief Minimum observed time
     */
    uint32 minTimeSeconds = UINT32_MAX;

    /**
     * @brief Maximum observed time
     */
    uint32 maxTimeSeconds = 0;

    /**
     * @brief Standard deviation of time (for confidence calculation)
     */
    float stdDevSeconds = 0.0f;

    /**
     * @brief Last update timestamp
     */
    std::chrono::system_clock::time_point lastUpdate;
};

/**
 * @brief Configuration for flow prediction
 */
struct FlowPredictionConfig
{
    // Prediction parameters
    bool enabled = true;
    uint32 minSamplesForPrediction = 10;
    float confidenceThreshold = 0.5f;

    // Database settings
    bool persistToDatabase = true;
    uint32 dbSyncIntervalMs = 60000;
    uint32 maxHistoryDays = 30;  // Cleanup older data

    // Default values (when no data)
    std::array<uint32, 4> defaultAvgTimeHours = {2, 24, 12, 0};  // Per bracket
};

/**
 * @brief Predicts bot flow between level brackets
 *
 * Singleton class tracking bracket transitions and predicting future flow.
 */
class TC_GAME_API BracketFlowPredictor
{
public:
    /**
     * @brief Get singleton instance
     */
    static BracketFlowPredictor* Instance();

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /**
     * @brief Initialize the predictor
     * @return true if successful
     */
    bool Initialize();

    /**
     * @brief Shutdown and cleanup
     */
    void Shutdown();

    /**
     * @brief Periodic update
     * @param diff Time since last update in milliseconds
     */
    void Update(uint32 diff);

    /**
     * @brief Load configuration
     */
    void LoadConfig();

    // ========================================================================
    // EVENT TRACKING
    // ========================================================================

    /**
     * @brief Called when a bot levels up
     * @param botGuid The bot
     * @param oldLevel Previous level
     * @param newLevel New level
     */
    void OnBotLeveledUp(ObjectGuid botGuid, uint32 oldLevel, uint32 newLevel);

    /**
     * @brief Called when a bot enters a bracket (creation or level-up)
     * @param botGuid The bot
     * @param bracket The bracket entered
     */
    void OnBotEnteredBracket(ObjectGuid botGuid, LevelBracket const* bracket);

    /**
     * @brief Called when a bot leaves a bracket (level-up or deletion)
     * @param botGuid The bot
     * @param bracket The bracket left
     * @param reason "levelup" or "deleted"
     */
    void OnBotLeftBracket(ObjectGuid botGuid, LevelBracket const* bracket, std::string const& reason);

    /**
     * @brief Called when a bot is created
     * @param botGuid The new bot
     * @param level Initial level
     */
    void OnBotCreated(ObjectGuid botGuid, uint32 level);

    /**
     * @brief Called when a bot is deleted
     * @param botGuid The deleted bot
     */
    void OnBotDeleted(ObjectGuid botGuid);

    // ========================================================================
    // PREDICTIONS
    // ========================================================================

    /**
     * @brief Predict bracket flow for a time window
     * @param bracket Bracket to predict
     * @param timeWindow Time window for prediction
     * @return Flow prediction
     */
    FlowPrediction PredictBracketFlow(LevelBracket const* bracket,
                                       std::chrono::seconds timeWindow) const;

    /**
     * @brief Predict flow for all brackets
     * @param timeWindow Time window for prediction
     * @return Map of bracket to prediction
     */
    std::vector<FlowPrediction> PredictAllBrackets(std::chrono::seconds timeWindow) const;

    /**
     * @brief Get average time bots spend in a bracket
     * @param bracket Bracket to query
     * @return Average time in seconds
     */
    std::chrono::seconds GetAverageTimeInBracket(LevelBracket const* bracket) const;

    /**
     * @brief Get average time bots spend in a bracket by tier
     * @param tier Expansion tier
     * @return Average time in seconds
     */
    std::chrono::seconds GetAverageTimeInTier(ExpansionTier tier) const;

    /**
     * @brief Get bots likely to leave bracket within time window
     * @param bracket Bracket to query
     * @param timeWindow Time window to check
     * @return List of bot GUIDs sorted by likelihood
     */
    std::vector<ObjectGuid> GetBotsLikelyToLeave(LevelBracket const* bracket,
                                                  std::chrono::seconds timeWindow) const;

    /**
     * @brief Estimate when a bot will leave its current bracket
     * @param botGuid Bot to query
     * @return Estimated time until bracket exit
     */
    std::chrono::seconds EstimateTimeUntilBracketExit(ObjectGuid botGuid) const;

    // ========================================================================
    // TIME IN BRACKET QUERIES
    // ========================================================================

    /**
     * @brief Get time a bot has been in its current bracket
     * @param botGuid Bot to query
     * @return Time in current bracket
     */
    std::chrono::seconds GetTimeInCurrentBracket(ObjectGuid botGuid) const;

    /**
     * @brief Get current bracket entry for a bot
     * @param botGuid Bot to query
     * @return Entry info or nullptr if not tracked
     */
    BotBracketEntry const* GetBracketEntry(ObjectGuid botGuid) const;

    // ========================================================================
    // STATISTICS
    // ========================================================================

    /**
     * @brief Get transition statistics for a bracket
     * @param bracket Bracket to query
     * @return Transition statistics
     */
    BracketTransitionStats GetTransitionStats(LevelBracket const* bracket) const;

    /**
     * @brief Get transition statistics by tier
     * @param tier Expansion tier
     * @return Transition statistics
     */
    BracketTransitionStats GetTierStats(ExpansionTier tier) const;

    /**
     * @brief Print statistics report
     */
    void PrintStatisticsReport() const;

    /**
     * @brief Get number of tracked bots
     */
    uint32 GetTrackedBotCount() const;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /**
     * @brief Get current configuration
     */
    FlowPredictionConfig const& GetConfig() const { return _config; }

    /**
     * @brief Set configuration
     */
    void SetConfig(FlowPredictionConfig const& config);

    // ========================================================================
    // DATABASE OPERATIONS
    // ========================================================================

    /**
     * @brief Load statistics from database
     */
    void LoadStatisticsFromDatabase();

    /**
     * @brief Save statistics to database
     */
    void SaveStatisticsToDatabase();

    /**
     * @brief Record a transition to database
     */
    void RecordTransitionToDatabase(ObjectGuid botGuid, uint8 fromBracket,
                                     uint8 toBracket, uint32 timeInBracketSeconds);

private:
    BracketFlowPredictor() = default;
    ~BracketFlowPredictor() = default;
    BracketFlowPredictor(BracketFlowPredictor const&) = delete;
    BracketFlowPredictor& operator=(BracketFlowPredictor const&) = delete;

    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /**
     * @brief Update bracket statistics with a transition
     */
    void UpdateBracketStatistics(uint8 bracketIndex, uint32 timeInBracketSeconds);

    /**
     * @brief Calculate confidence for a prediction
     */
    float CalculateConfidence(BracketTransitionStats const& stats) const;

    /**
     * @brief Get bracket index for a level
     */
    uint8 GetBracketIndex(uint32 level) const;

    /**
     * @brief Get bracket index for a tier
     */
    uint8 GetBracketIndex(ExpansionTier tier) const;

    /**
     * @brief Cleanup old database records
     */
    void CleanupOldRecords();

    // ========================================================================
    // DATA MEMBERS
    // ========================================================================

    // Configuration
    FlowPredictionConfig _config;

    // Current bracket entries for tracked bots
    using EntryMap = phmap::parallel_flat_hash_map<
        ObjectGuid,
        BotBracketEntry,
        std::hash<ObjectGuid>,
        std::equal_to<>,
        std::allocator<std::pair<ObjectGuid, BotBracketEntry>>,
        4,
        std::shared_mutex
    >;
    EntryMap _bracketEntries;

    // Statistics per bracket (4 expansion tiers)
    std::array<BracketTransitionStats, 4> _bracketStats;
    mutable std::shared_mutex _statsMutex;

    // Recent transitions for moving average
    struct RecentTransition
    {
        uint8 bracketIndex;
        uint32 timeSeconds;
        std::chrono::system_clock::time_point when;
    };
    std::vector<RecentTransition> _recentTransitions;
    static constexpr size_t MAX_RECENT_TRANSITIONS = 1000;
    mutable std::mutex _transitionMutex;

    // Timing
    uint32 _dbSyncAccumulator = 0;
    uint32 _cleanupAccumulator = 0;
    static constexpr uint32 CLEANUP_INTERVAL_MS = 3600000;  // 1 hour

    // Initialization state
    std::atomic<bool> _initialized{false};
};

} // namespace Playerbot

#define sBracketFlowPredictor Playerbot::BracketFlowPredictor::Instance()
