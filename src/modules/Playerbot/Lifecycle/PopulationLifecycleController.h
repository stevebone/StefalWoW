/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * @file PopulationLifecycleController.h
 * @brief Master orchestrator for bot population lifecycle management
 *
 * The PopulationLifecycleController coordinates all systems involved in
 * maintaining a healthy, distributed bot population:
 *
 * 1. Protection Registry - Tracks which bots cannot be retired
 * 2. Retirement Manager - Handles graceful bot removal
 * 3. Bracket Flow Predictor - Predicts population movement
 * 4. Player Activity Tracker - Monitors where players are
 * 5. Demand Calculator - Generates spawn requests
 *
 * Key Responsibilities:
 * - Maintain target population distribution across level brackets
 * - Respond to player activity with demand-driven spawning
 * - Retire unprotected bots from overpopulated brackets
 * - Create new bots in underpopulated brackets
 * - Protect socially-connected bots from retirement
 *
 * Thread Safety:
 * - All public methods are thread-safe
 * - Uses read-write locks for statistics access
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "BotLifecycleState.h"
#include "Character/ZoneLevelHelper.h"
#include <atomic>
#include <array>
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <memory>
#include <vector>

// Forward declaration OUTSIDE namespace for TrinityCore's Player class
class Player;

namespace Playerbot
{

// Forward declarations
class BotProtectionRegistry;
class BotRetirementManager;
class BracketFlowPredictor;
class PlayerActivityTracker;
class DemandCalculator;
class PopulationPIDController;
struct DemandSpawnRequest;

/**
 * @brief Configuration for the lifecycle controller
 */
struct LifecycleControllerConfig
{
    // Enable/disable
    bool enabled = true;

    // Target population
    uint32 targetPopulation = 500;

    // Bracket distribution (percentage of flowing population)
    std::array<uint32, 4> bracketTargetPct = {15, 45, 20, 20};  // Starting, Chromie, DF, TWW

    // Creation settings
    uint32 maxCreationsPerHour = 30;
    uint32 minDeficitForCreation = 5;
    bool prioritizeDemand = true;

    // Retirement settings
    uint32 maxRetirementsPerHour = 10;
    uint32 minSurplusForRetirement = 5;
    bool pauseDuringPeakHours = true;
    uint32 peakHourStart = 18;  // 6 PM
    uint32 peakHourEnd = 23;    // 11 PM

    // Update intervals
    uint32 analysisIntervalMs = 60000;     // 1 minute
    uint32 reportIntervalMs = 300000;      // 5 minutes
    uint32 balancingIntervalMs = 300000;   // 5 minutes

    // Logging
    bool logDecisions = true;
    bool logDetailedStats = false;
};

/**
 * @brief Statistics for a single bracket
 */
struct BracketStats
{
    ExpansionTier tier = ExpansionTier::Starting;
    uint32 currentBotCount = 0;
    uint32 targetBotCount = 0;
    uint32 protectedCount = 0;
    uint32 retireableCount = 0;
    int32 deficit = 0;
    int32 effectiveDeficit = 0;  // Considers flow predictions
    uint32 playerCount = 0;
    uint32 predictedOutflow = 0;
    uint32 predictedInflow = 0;
    float urgency = 0.0f;
};

/**
 * @brief Overall lifecycle statistics
 */
struct LifecycleStats
{
    // Population counts
    uint32 totalBots = 0;
    uint32 protectedBots = 0;
    uint32 unprotectedBots = 0;
    uint32 botsInRetirementQueue = 0;

    // Hourly activity
    uint32 botsCreatedThisHour = 0;
    uint32 botsRetiredThisHour = 0;
    uint32 botsLeveledUpThisHour = 0;

    // Per-bracket statistics
    std::array<BracketStats, 4> brackets;

    // Player activity
    uint32 activePlayerCount = 0;
    std::array<uint32, 4> playerCountPerBracket = {};

    // Performance
    uint32 lastAnalysisDurationMs = 0;
    uint32 pendingSpawnRequests = 0;

    // Timing
    std::chrono::system_clock::time_point lastUpdate;
    std::chrono::system_clock::time_point lastAnalysis;
    std::chrono::system_clock::time_point lastBalancing;
};

/**
 * @brief Decision record for logging/debugging
 */
struct LifecycleDecision
{
    enum class Type
    {
        SpawnBot,
        RetireBot,
        CancelRetirement,
        SkipSpawn,
        SkipRetirement,
        Rebalance
    };

    Type type = Type::SkipSpawn;
    ExpansionTier bracket = ExpansionTier::Starting;
    ObjectGuid botGuid;
    uint32 zoneId = 0;
    uint32 targetLevel = 0;
    float priority = 0.0f;
    std::string reason;
    std::chrono::system_clock::time_point timestamp;
};

/**
 * @brief Master orchestrator for population lifecycle
 *
 * Singleton class coordinating all lifecycle systems to maintain
 * healthy bot population distribution.
 */
class TC_GAME_API PopulationLifecycleController
{
public:
    /**
     * @brief Get singleton instance
     */
    static PopulationLifecycleController* Instance();

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /**
     * @brief Initialize the controller
     * @return true if successful
     */
    bool Initialize();

    /**
     * @brief Shutdown and cleanup
     */
    void Shutdown();

    /**
     * @brief Main update loop
     * @param diff Time since last update in milliseconds
     */
    void Update(uint32 diff);

    /**
     * @brief Load configuration from config file
     */
    void LoadConfig();

    // ========================================================================
    // POPULATION MANAGEMENT
    // ========================================================================

    /**
     * @brief Analyze current population state
     * @return Current lifecycle statistics
     */
    LifecycleStats AnalyzePopulation();

    /**
     * @brief Process retirement decisions
     * @return Number of retirements queued
     */
    uint32 ProcessRetirements();

    /**
     * @brief Process creation decisions
     * @return Number of spawn requests generated
     */
    uint32 ProcessCreations();

    /**
     * @brief Force population rebalancing
     */
    void ForceRebalance();

    // ========================================================================
    // QUERIES
    // ========================================================================

    /**
     * @brief Get current statistics
     * @return Lifecycle statistics
     */
    LifecycleStats const& GetStats() const;

    /**
     * @brief Get bracket statistics
     * @param tier Expansion tier
     * @return Bracket statistics
     */
    BracketStats GetBracketStats(ExpansionTier tier) const;

    /**
     * @brief Check if spawning is currently needed
     * @return true if any bracket has significant deficit
     */
    bool IsSpawningNeeded() const;

    /**
     * @brief Check if retirement is currently needed
     * @return true if any bracket has significant surplus
     */
    bool IsRetirementNeeded() const;

    /**
     * @brief Get pending spawn requests
     * @param maxCount Maximum requests to return
     * @return Vector of spawn requests
     */
    std::vector<DemandSpawnRequest> GetPendingSpawnRequests(uint32 maxCount) const;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /**
     * @brief Get current configuration
     */
    LifecycleControllerConfig const& GetConfig() const { return _config; }

    /**
     * @brief Set configuration
     */
    void SetConfig(LifecycleControllerConfig const& config);

    /**
     * @brief Check if controller is enabled
     */
    bool IsEnabled() const { return _config.enabled && _initialized.load(); }

    // ========================================================================
    // REPORTING
    // ========================================================================

    /**
     * @brief Print detailed status report to log
     */
    void PrintStatusReport() const;

    /**
     * @brief Print decision history to log
     * @param maxDecisions Maximum decisions to print
     */
    void PrintDecisionHistory(uint32 maxDecisions = 20) const;

    // ========================================================================
    // EVENT HANDLERS
    // ========================================================================

    /**
     * @brief Called when a bot levels up
     * @param botGuid Bot that leveled up
     * @param oldLevel Previous level
     * @param newLevel New level
     */
    void OnBotLeveledUp(ObjectGuid botGuid, uint32 oldLevel, uint32 newLevel);

    /**
     * @brief Called when a bot is created
     * @param botGuid New bot's GUID
     * @param level Bot's level
     */
    void OnBotCreated(ObjectGuid botGuid, uint32 level);

    /**
     * @brief Called when a bot is deleted
     * @param botGuid Deleted bot's GUID
     * @param level Bot's level at deletion
     */
    void OnBotDeleted(ObjectGuid botGuid, uint32 level);

    /**
     * @brief Called when a bot's protection status changes
     * @param botGuid Bot whose protection changed
     * @param isProtected New protection status
     */
    void OnProtectionChanged(ObjectGuid botGuid, bool isProtected);

    /**
     * @brief Called when a player logs in
     * @param player The player
     */
    void OnPlayerLogin(Player* player);

    /**
     * @brief Called when a player logs out
     * @param player The player
     */
    void OnPlayerLogout(Player* player);

private:
    PopulationLifecycleController() = default;
    ~PopulationLifecycleController() = default;
    PopulationLifecycleController(PopulationLifecycleController const&) = delete;
    PopulationLifecycleController& operator=(PopulationLifecycleController const&) = delete;

    // ========================================================================
    // INTERNAL ANALYSIS
    // ========================================================================

    /**
     * @brief Calculate bracket targets based on configuration
     */
    void CalculateBracketTargets();

    /**
     * @brief Update bracket statistics
     */
    void UpdateBracketStats();

    /**
     * @brief Calculate effective deficit for a bracket
     * @param tier Bracket tier
     * @return Effective deficit considering flow predictions
     */
    int32 CalculateEffectiveDeficit(ExpansionTier tier) const;

    /**
     * @brief Check if it's currently peak hours
     * @return true if within peak hours
     */
    bool IsPeakHour() const;

    /**
     * @brief Check rate limits for creations
     * @return true if can create more bots this hour
     */
    bool CanCreateMoreBots() const;

    /**
     * @brief Check rate limits for retirements
     * @return true if can retire more bots this hour
     */
    bool CanRetireMoreBots() const;

    // ========================================================================
    // DECISION MAKING
    // ========================================================================

    /**
     * @brief Select bots for retirement from surplus brackets
     * @return Number of bots queued for retirement
     */
    uint32 SelectBotsForRetirement();

    /**
     * @brief Generate spawn requests for deficit brackets
     * @return Number of spawn requests generated
     */
    uint32 GenerateSpawnRequests();

    /**
     * @brief Record a lifecycle decision
     * @param decision Decision to record
     */
    void RecordDecision(LifecycleDecision const& decision);

    // ========================================================================
    // HOURLY TRACKING
    // ========================================================================

    /**
     * @brief Reset hourly counters if hour has changed
     */
    void ResetHourlyCountersIfNeeded();

    // ========================================================================
    // DATA MEMBERS
    // ========================================================================

    // Configuration
    LifecycleControllerConfig _config;

    // Component references (set during initialization)
    BotProtectionRegistry* _protectionRegistry = nullptr;
    BotRetirementManager* _retirementManager = nullptr;
    BracketFlowPredictor* _flowPredictor = nullptr;
    PlayerActivityTracker* _activityTracker = nullptr;
    DemandCalculator* _demandCalculator = nullptr;
    std::unique_ptr<PopulationPIDController> _pidController;

    // Statistics
    mutable LifecycleStats _stats;
    mutable std::shared_mutex _statsMutex;

    // Decision history (circular buffer)
    static constexpr size_t MAX_DECISION_HISTORY = 100;
    std::vector<LifecycleDecision> _decisionHistory;
    mutable std::mutex _decisionMutex;

    // Bracket targets (calculated from config)
    std::array<uint32, 4> _bracketTargets = {};

    // Hourly tracking
    std::atomic<uint32> _creationsThisHour{0};
    std::atomic<uint32> _retirementsThisHour{0};
    std::chrono::system_clock::time_point _hourStart;

    // Pending spawn requests
    mutable std::vector<DemandSpawnRequest> _pendingSpawnRequests;
    mutable std::mutex _spawnRequestMutex;

    // Timing accumulators
    uint32 _analysisAccumulator = 0;
    uint32 _reportAccumulator = 0;
    uint32 _balancingAccumulator = 0;

    // Initialization state
    std::atomic<bool> _initialized{false};
};

} // namespace Playerbot

#define sPopulationLifecycleController Playerbot::PopulationLifecycleController::Instance()
