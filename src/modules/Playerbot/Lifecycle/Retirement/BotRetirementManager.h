/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * @file BotRetirementManager.h
 * @brief Manages bot retirement queue and lifecycle
 *
 * The BotRetirementManager controls the retirement process for bots:
 * 1. Identifies candidates for retirement (unprotected, bracket overpopulated)
 * 2. Queues bots for retirement with cooling period
 * 3. Monitors protection changes to rescue bots
 * 4. Executes graceful exit when cooling expires
 * 5. Rate limits retirements to prevent disruption
 *
 * Thread Safety:
 * - All public methods are thread-safe
 * - Uses parallel hashmap for retirement queue
 * - Rate limiting uses atomic counters
 *
 * Performance:
 * - O(1) queue operations
 * - O(n log n) candidate selection
 * - Periodic batch processing
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "RetirementState.h"
#include "RetirementCandidate.h"
#include "GracefulExitHandler.h"
#include "../Protection/ProtectionStatus.h"
#include "Character/BotLevelDistribution.h"
#include <parallel_hashmap/phmap.h>
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <vector>

namespace Playerbot
{

// Forward declarations
class BotProtectionRegistry;

/**
 * @brief Configuration for retirement behavior
 */
struct RetirementConfig
{
    // Enable/disable retirement
    bool enabled = true;

    // Cooling period (default 7 days)
    uint32 coolingPeriodDays = 7;

    // Rate limiting
    uint32 maxRetirementsPerHour = 10;
    uint32 maxRetirementsPerDay = 100;

    // Peak hours (no retirements during these hours)
    uint32 peakHourStart = 18;  // 6 PM
    uint32 peakHourEnd = 23;    // 11 PM
    bool avoidPeakHours = true;

    // Graceful exit
    bool gracefulExit = true;
    uint32 gracefulExitTimeoutMs = 60000;

    // Priority calculation weights
    float overpopulationWeight = 200.0f;
    float timeInBracketWeight = 10.0f;  // Per hour
    float playtimeWeight = 0.1f;        // Per minute (inverse)
    float interactionWeight = 5.0f;     // Per interaction (inverse)

    // Thresholds
    float minOverpopulationForRetirement = 0.15f;  // 15% overpopulated
    uint32 minPlaytimeBeforeRetirement = 60;       // 1 hour minimum playtime

    // Database sync
    bool persistToDatabase = true;
    uint32 dbSyncIntervalMs = 60000;
};

/**
 * @brief Manages bot retirement queue and lifecycle
 *
 * Singleton class controlling all retirement operations.
 */
class TC_GAME_API BotRetirementManager
{
public:
    /**
     * @brief Get singleton instance
     */
    static BotRetirementManager* Instance();

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /**
     * @brief Initialize the manager
     * @return true if successful
     */
    bool Initialize();

    /**
     * @brief Shutdown and cleanup
     */
    void Shutdown();

    /**
     * @brief Periodic update (call from world thread)
     * @param diff Time since last update in milliseconds
     */
    void Update(uint32 diff);

    /**
     * @brief Load configuration
     */
    void LoadConfig();

    // ========================================================================
    // QUEUE MANAGEMENT
    // ========================================================================

    /**
     * @brief Queue a bot for retirement
     * @param botGuid Bot to retire
     * @param reason Human-readable reason
     * @return true if queued successfully
     */
    bool QueueForRetirement(ObjectGuid botGuid, std::string const& reason);

    /**
     * @brief Cancel retirement for a bot (rescue)
     * @param botGuid Bot to rescue
     * @param reason Why retirement was cancelled
     * @return true if cancelled successfully
     */
    bool CancelRetirement(ObjectGuid botGuid, RetirementCancelReason reason);

    /**
     * @brief Check if a bot is in retirement queue
     * @param botGuid Bot to check
     * @return true if in queue
     */
    bool IsInRetirementQueue(ObjectGuid botGuid) const;

    /**
     * @brief Get retirement state for a bot
     * @param botGuid Bot to query
     * @return Current retirement state
     */
    RetirementState GetRetirementState(ObjectGuid botGuid) const;

    /**
     * @brief Get full retirement candidate info
     * @param botGuid Bot to query
     * @return Candidate info (or empty if not in queue)
     */
    RetirementCandidate GetCandidate(ObjectGuid botGuid) const;

    /**
     * @brief Get all candidates in a specific state
     * @param state State to filter by
     * @return Vector of candidates
     */
    std::vector<RetirementCandidate> GetCandidatesInState(RetirementState state) const;

    // ========================================================================
    // CANDIDATE SELECTION
    // ========================================================================

    /**
     * @brief Get retirement candidates from a bracket
     * @param bracket Level bracket to analyze
     * @param maxCount Maximum candidates to return
     * @return Candidates sorted by retirement priority (highest first)
     */
    std::vector<ObjectGuid> GetRetirementCandidates(LevelBracket const* bracket, uint32 maxCount);

    /**
     * @brief Calculate retirement priority for a bot
     * @param botGuid Bot to calculate for
     * @return Priority score (higher = retire sooner)
     *
     * Priority factors:
     * - Bracket overpopulation (positive)
     * - Time in bracket (positive if overpopulated)
     * - Protection score (negative)
     * - Playtime (negative - more played = less likely)
     * - Interaction count (negative)
     */
    float CalculateRetirementPriority(ObjectGuid botGuid) const;

    /**
     * @brief Get bracket overpopulation ratio
     * @param bracket Bracket to check
     * @return Ratio above target (0.15 = 15% overpopulated, negative = underpopulated)
     */
    float GetBracketOverpopulationRatio(LevelBracket const* bracket) const;

    /**
     * @brief Check if bracket needs retirement
     * @param bracket Bracket to check
     * @return true if bracket is overpopulated
     */
    bool BracketNeedsRetirement(LevelBracket const* bracket) const;

    // ========================================================================
    // RATE LIMITING
    // ========================================================================

    /**
     * @brief Check if more retirements can be processed
     * @return true if under rate limit
     */
    bool CanProcessMoreRetirements() const;

    /**
     * @brief Get remaining capacity this hour
     * @return Number of retirements that can still be processed
     */
    uint32 GetRemainingCapacity() const;

    /**
     * @brief Check if current time is peak hours
     * @return true if in peak hours
     */
    bool IsPeakHour() const;

    // ========================================================================
    // PROTECTION INTEGRATION
    // ========================================================================

    /**
     * @brief Handle protection change event
     * @param botGuid Bot whose protection changed
     * @param newStatus New protection status
     *
     * Called by BotProtectionRegistry when protection changes.
     * May rescue bot from retirement if protection gained.
     */
    void OnProtectionChanged(ObjectGuid botGuid, ProtectionStatus const& newStatus);

    /**
     * @brief Set protection registry reference
     * @param registry Protection registry instance
     */
    void SetProtectionRegistry(BotProtectionRegistry* registry);

    // ========================================================================
    // STATISTICS
    // ========================================================================

    /**
     * @brief Get current retirement statistics
     * @return Statistics struct
     */
    RetirementStatistics GetStatistics() const;

    /**
     * @brief Print status report to log
     */
    void PrintStatusReport() const;

    /**
     * @brief Get queue size
     * @return Total bots in retirement queue
     */
    uint32 GetQueueSize() const;

    /**
     * @brief Get retirements completed this hour
     */
    uint32 GetRetirementsThisHour() const { return _retirementsThisHour.load(); }

    /**
     * @brief Get retirements completed today
     */
    uint32 GetRetirementsToday() const { return _retirementsToday.load(); }

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /**
     * @brief Get current configuration
     */
    RetirementConfig const& GetConfig() const { return _config; }

    /**
     * @brief Set configuration
     */
    void SetConfig(RetirementConfig const& config);

    // ========================================================================
    // ADMIN OPERATIONS
    // ========================================================================

    /**
     * @brief Force immediate retirement of a bot (admin)
     * @param botGuid Bot to retire
     * @return true if successful
     */
    bool ForceRetirement(ObjectGuid botGuid);

    /**
     * @brief Clear entire retirement queue (admin)
     */
    void ClearQueue();

    /**
     * @brief Process all pending retirements (admin)
     * @param ignoreRateLimits Bypass rate limiting
     */
    void ProcessAllPending(bool ignoreRateLimits = false);

private:
    BotRetirementManager() = default;
    ~BotRetirementManager() = default;
    BotRetirementManager(BotRetirementManager const&) = delete;
    BotRetirementManager& operator=(BotRetirementManager const&) = delete;

    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /**
     * @brief Process pending retirements
     */
    void ProcessPendingQueue();

    /**
     * @brief Process bots in cooling period
     */
    void ProcessCoolingQueue();

    /**
     * @brief Process bots in graceful exit
     */
    void ProcessExitingQueue();

    /**
     * @brief Execute graceful exit for a candidate
     */
    void ExecuteGracefulExit(RetirementCandidate& candidate);

    /**
     * @brief Finalize retirement (delete character)
     */
    void FinalizeRetirement(ObjectGuid botGuid);

    /**
     * @brief Handle graceful exit stage completion
     */
    void OnStageComplete(ObjectGuid botGuid, StageResult const& result);

    /**
     * @brief Update hourly counters
     */
    void UpdateHourlyCounters();

    /**
     * @brief Load queue from database
     */
    void LoadFromDatabase();

    /**
     * @brief Save queue to database
     */
    void SaveToDatabase();

    /**
     * @brief Save single candidate to database
     */
    void SaveCandidateToDatabase(RetirementCandidate const& candidate);

    /**
     * @brief Remove candidate from database
     */
    void RemoveCandidateFromDatabase(ObjectGuid botGuid);

    /**
     * @brief Mark candidate as dirty (needs DB save)
     */
    void MarkDirty(ObjectGuid botGuid);

    /**
     * @brief Get bot playtime
     */
    uint32 GetBotPlaytime(ObjectGuid botGuid) const;

    /**
     * @brief Get time bot has been in current bracket
     */
    uint32 GetTimeInCurrentBracket(ObjectGuid botGuid) const;

    /**
     * @brief Update cached statistics
     */
    void UpdateStatistics();

    // ========================================================================
    // DATA MEMBERS
    // ========================================================================

    // Configuration
    RetirementConfig _config;

    // Retirement queue using parallel hashmap
    using RetirementQueue = phmap::parallel_flat_hash_map<
        ObjectGuid,
        RetirementCandidate,
        std::hash<ObjectGuid>,
        std::equal_to<>,
        std::allocator<std::pair<ObjectGuid, RetirementCandidate>>,
        4,  // 4 submaps for concurrency
        std::shared_mutex
    >;
    RetirementQueue _retirementQueue;

    // Priority queue for processing order
    struct CandidatePriority
    {
        ObjectGuid guid;
        float priority;
        bool operator<(CandidatePriority const& other) const
        {
            return priority < other.priority;  // Higher priority first
        }
    };
    std::priority_queue<CandidatePriority> _processingQueue;
    mutable std::mutex _processingMutex;

    // Dirty tracking for database sync
    std::set<ObjectGuid> _dirtyCandidates;
    mutable std::mutex _dirtyMutex;

    // Rate limiting
    std::atomic<uint32> _retirementsThisHour{0};
    std::atomic<uint32> _retirementsToday{0};
    std::chrono::system_clock::time_point _hourStart;
    std::chrono::system_clock::time_point _dayStart;

    // Timing
    uint32 _updateAccumulator = 0;
    uint32 _dbSyncAccumulator = 0;
    uint32 _hourlyResetAccumulator = 0;
    static constexpr uint32 UPDATE_INTERVAL_MS = 10000;       // 10 seconds
    static constexpr uint32 HOURLY_RESET_INTERVAL_MS = 3600000;  // 1 hour

    // Statistics cache
    mutable RetirementStatistics _cachedStats;
    mutable std::atomic<bool> _statsDirty{true};

    // External references
    BotProtectionRegistry* _protectionRegistry = nullptr;

    // Initialization state
    std::atomic<bool> _initialized{false};
};

} // namespace Playerbot

#define sBotRetirementManager Playerbot::BotRetirementManager::Instance()
