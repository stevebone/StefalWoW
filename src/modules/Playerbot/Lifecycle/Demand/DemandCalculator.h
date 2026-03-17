/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * @file DemandCalculator.h
 * @brief Calculates bot spawn demand based on player activity
 *
 * The DemandCalculator analyzes player activity data to determine:
 * 1. Which level brackets need more bots
 * 2. Which zones would benefit from bot presence
 * 3. Priority order for spawn requests
 *
 * Thread Safety:
 * - All public methods are thread-safe
 * - Uses data from PlayerActivityTracker and BotLevelDistribution
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "../BotLifecycleState.h"
#include "Character/BotLevelDistribution.h"
#include <atomic>
#include <chrono>
#include <mutex>
#include <vector>
#include <map>

namespace Playerbot
{

// Forward declarations
class PlayerActivityTracker;
class BotProtectionRegistry;
class BracketFlowPredictor;

/**
 * @brief Demand calculation for a level bracket
 */
struct BracketDemand
{
    /**
     * @brief The bracket being analyzed
     */
    LevelBracket const* bracket = nullptr;

    /**
     * @brief Expansion tier
     */
    ExpansionTier tier = ExpansionTier::Starting;

    /**
     * @brief Current bot count in bracket
     */
    uint32 currentBotCount = 0;

    /**
     * @brief Target bot count for bracket
     */
    uint32 targetBotCount = 0;

    /**
     * @brief Deficit (positive = need more bots)
     */
    int32 deficit = 0;

    /**
     * @brief Number of protected bots (can't be retired)
     */
    uint32 protectedCount = 0;

    /**
     * @brief Number of active players in bracket
     */
    uint32 playerCount = 0;

    /**
     * @brief Urgency score (0.0-1.0, higher = more urgent)
     */
    float urgency = 0.0f;

    /**
     * @brief Zones with player activity in this bracket
     */
    std::vector<uint32> demandZones;

    /**
     * @brief Predicted outflow from this bracket
     */
    uint32 predictedOutflow = 0;

    /**
     * @brief Predicted inflow to this bracket
     */
    uint32 predictedInflow = 0;
};

/**
 * @brief Demand-based spawn request for a new bot
 * @note Different from Playerbot::SpawnRequest in SpawnRequest.h - this is specifically for demand-driven spawning
 */
struct DemandSpawnRequest
{
    /**
     * @brief Target level for the new bot
     */
    uint32 targetLevel = 1;

    /**
     * @brief Target zone for spawning
     */
    uint32 preferredZoneId = 0;

    /**
     * @brief Priority (higher = spawn first)
     */
    float priority = 0.0f;

    /**
     * @brief Reason for spawn
     */
    std::string reason;

    /**
     * @brief Target bracket
     */
    ExpansionTier tier = ExpansionTier::Starting;

    /**
     * @brief Optional class restriction
     */
    uint8 preferredClass = 0;  // 0 = any
};

/**
 * @brief Zone scoring for spawn selection
 */
struct ZoneDemandScore
{
    uint32 zoneId = 0;
    float score = 0.0f;
    uint32 playerCount = 0;
    uint32 botCount = 0;
    uint32 recommendedLevel = 0;
    bool isQuestHub = false;
    bool hasActivePlayers = false;
};

/**
 * @brief Configuration for demand calculation
 */
struct DemandCalculatorConfig
{
    bool enabled = true;

    // Weight factors
    float playerProximityWeight = 100.0f;
    float bracketDeficitWeight = 50.0f;
    float questHubBonus = 30.0f;
    float flowPredictionWeight = 25.0f;

    // Thresholds
    uint32 minDeficitForSpawn = 5;
    float minUrgencyForSpawn = 0.1f;

    // Spawn distribution
    bool prioritizePlayerZones = true;
    bool avoidOverpopulatedZones = true;
    uint32 maxBotsPerZone = 50;

    // Update frequency
    uint32 recalculateIntervalMs = 30000;  // 30 seconds
};

/**
 * @brief Calculates bot spawn demand
 *
 * Singleton class analyzing player activity and bracket populations
 * to generate spawn requests.
 */
class TC_GAME_API DemandCalculator
{
public:
    /**
     * @brief Get singleton instance
     */
    static DemandCalculator* Instance();

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /**
     * @brief Initialize the calculator
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
    // DEMAND CALCULATION
    // ========================================================================

    /**
     * @brief Calculate demand for all brackets
     * @return Vector of bracket demands sorted by urgency
     */
    std::vector<BracketDemand> CalculateAllDemands() const;

    /**
     * @brief Calculate demand for a specific bracket
     * @param bracket Bracket to analyze
     * @return Bracket demand
     */
    BracketDemand CalculateBracketDemand(LevelBracket const* bracket) const;

    /**
     * @brief Calculate effective deficit considering flow
     * @param bracket Bracket to analyze
     * @return Effective deficit
     */
    int32 CalculateEffectiveDeficit(LevelBracket const* bracket) const;

    /**
     * @brief Calculate urgency score
     * @param bracket Bracket to analyze
     * @param deficit Current deficit
     * @return Urgency score (0.0-1.0)
     */
    float CalculateUrgency(LevelBracket const* bracket, int32 deficit) const;

    // ========================================================================
    // SPAWN REQUESTS
    // ========================================================================

    /**
     * @brief Generate spawn requests based on current demand
     * @param maxCount Maximum requests to generate
     * @return Spawn requests sorted by priority
     */
    std::vector<DemandSpawnRequest> GenerateSpawnRequests(uint32 maxCount) const;

    /**
     * @brief Check if spawning is needed
     * @return true if any bracket has significant deficit
     */
    bool IsSpawningNeeded() const;

    /**
     * @brief Get number of bots that should be spawned
     * @return Total deficit across all brackets
     */
    uint32 GetTotalSpawnDeficit() const;

    // ========================================================================
    // ZONE SELECTION
    // ========================================================================

    /**
     * @brief Select best zone for spawning at a level
     * @param targetLevel Level for the new bot
     * @return Zone ID (0 if no suitable zone)
     */
    uint32 SelectSpawnZoneForLevel(uint32 targetLevel) const;

    /**
     * @brief Score a zone for spawning
     * @param zoneId Zone to score
     * @param targetLevel Level of bot to spawn
     * @return Score (higher = better)
     */
    ZoneDemandScore ScoreZoneForSpawning(uint32 zoneId, uint32 targetLevel) const;

    /**
     * @brief Get top zones for spawning
     * @param targetLevel Level for bots
     * @param maxCount Maximum zones to return
     * @return Scored zones sorted by score
     */
    std::vector<ZoneDemandScore> GetTopSpawnZones(uint32 targetLevel, uint32 maxCount = 5) const;

    // ========================================================================
    // DEPENDENCIES
    // ========================================================================

    /**
     * @brief Set activity tracker reference
     */
    void SetActivityTracker(PlayerActivityTracker* tracker);

    /**
     * @brief Set protection registry reference
     */
    void SetProtectionRegistry(BotProtectionRegistry* registry);

    /**
     * @brief Set flow predictor reference
     */
    void SetFlowPredictor(BracketFlowPredictor* predictor);

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /**
     * @brief Get current configuration
     */
    DemandCalculatorConfig const& GetConfig() const { return _config; }

    /**
     * @brief Set configuration
     */
    void SetConfig(DemandCalculatorConfig const& config);

    // ========================================================================
    // STATISTICS
    // ========================================================================

    /**
     * @brief Print demand report
     */
    void PrintDemandReport() const;

private:
    DemandCalculator() = default;
    ~DemandCalculator() = default;
    DemandCalculator(DemandCalculator const&) = delete;
    DemandCalculator& operator=(DemandCalculator const&) = delete;

    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /**
     * @brief Recalculate cached demands
     */
    void RecalculateDemands();

    /**
     * @brief Get zones suitable for a level
     */
    std::vector<uint32> GetZonesForLevel(uint32 level) const;

    /**
     * @brief Check if zone is a quest hub
     */
    bool IsQuestHub(uint32 zoneId) const;

    /**
     * @brief Get bot count in zone
     */
    uint32 GetBotCountInZone(uint32 zoneId) const;

    // ========================================================================
    // DATA MEMBERS
    // ========================================================================

    // Configuration
    DemandCalculatorConfig _config;

    // External references
    PlayerActivityTracker* _activityTracker = nullptr;
    BotProtectionRegistry* _protectionRegistry = nullptr;
    BracketFlowPredictor* _flowPredictor = nullptr;

    // Cached demands
    mutable std::vector<BracketDemand> _cachedDemands;
    mutable std::chrono::system_clock::time_point _lastRecalculate;
    mutable std::mutex _cacheMutex;

    // Timing
    uint32 _updateAccumulator = 0;

    // Initialization state
    std::atomic<bool> _initialized{false};
};

} // namespace Playerbot

#define sDemandCalculator Playerbot::DemandCalculator::Instance()
