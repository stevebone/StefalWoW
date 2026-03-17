/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "DemandCalculator.h"
#include "PlayerActivityTracker.h"
#include "../Protection/BotProtectionRegistry.h"
#include "../Prediction/BracketFlowPredictor.h"
#include "Character/BotLevelDistribution.h"
#include "Log.h"
#include "Config/PlayerbotConfig.h"
#include "Quest/QuestHubDatabase.h"
#include "Lifecycle/BotSpawner.h"
#include <algorithm>
#include <cmath>

namespace Playerbot
{

DemandCalculator* DemandCalculator::Instance()
{
    static DemandCalculator instance;
    return &instance;
}

bool DemandCalculator::Initialize()
{
    if (_initialized.exchange(true))
        return true;

    LoadConfig();

    TC_LOG_INFO("playerbot.lifecycle", "DemandCalculator initialized");
    return true;
}

void DemandCalculator::Shutdown()
{
    if (!_initialized.exchange(false))
        return;

    TC_LOG_INFO("playerbot.lifecycle", "DemandCalculator shutdown");
}

void DemandCalculator::Update(uint32 diff)
{
    if (!_initialized || !_config.enabled)
        return;

    _updateAccumulator += diff;

    if (_updateAccumulator >= _config.recalculateIntervalMs)
    {
        _updateAccumulator = 0;
        RecalculateDemands();
    }
}

void DemandCalculator::LoadConfig()
{
    _config.enabled = sPlayerbotConfig->GetBool(
        "Playerbot.Lifecycle.Demand.Enable", true);
    _config.playerProximityWeight = sPlayerbotConfig->GetFloat(
        "Playerbot.Lifecycle.Demand.PlayerProximityWeight", 100.0f);
    _config.bracketDeficitWeight = sPlayerbotConfig->GetFloat(
        "Playerbot.Lifecycle.Demand.BracketDeficitWeight", 50.0f);
    _config.questHubBonus = sPlayerbotConfig->GetFloat(
        "Playerbot.Lifecycle.Demand.QuestHubBonus", 30.0f);
    _config.flowPredictionWeight = sPlayerbotConfig->GetFloat(
        "Playerbot.Lifecycle.Demand.FlowPredictionWeight", 25.0f);
    _config.minDeficitForSpawn = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Demand.MinDeficitForSpawn", 5);
    _config.minUrgencyForSpawn = sPlayerbotConfig->GetFloat(
        "Playerbot.Lifecycle.Demand.MinUrgencyForSpawn", 0.1f);
    _config.prioritizePlayerZones = sPlayerbotConfig->GetBool(
        "Playerbot.Lifecycle.Demand.PrioritizePlayerZones", true);
    _config.avoidOverpopulatedZones = sPlayerbotConfig->GetBool(
        "Playerbot.Lifecycle.Demand.AvoidOverpopulatedZones", true);
    _config.maxBotsPerZone = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Demand.MaxBotsPerZone", 50);
    _config.recalculateIntervalMs = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Demand.RecalculateIntervalMs", 30000);

    TC_LOG_INFO("playerbot.lifecycle", "DemandCalculator config loaded: "
        "ProximityWeight={:.1f}, DeficitWeight={:.1f}, MinDeficit={}",
        _config.playerProximityWeight, _config.bracketDeficitWeight,
        _config.minDeficitForSpawn);
}

// ============================================================================
// DEMAND CALCULATION
// ============================================================================

std::vector<BracketDemand> DemandCalculator::CalculateAllDemands() const
{
    std::vector<BracketDemand> demands;

    BotLevelDistribution* dist = BotLevelDistribution::instance();
    if (!dist)
        return demands;

    // Iterate over all expansion tiers for both factions
    for (uint8 tierIdx = 0; tierIdx < static_cast<uint8>(ExpansionTier::Max); ++tierIdx)
    {
        ExpansionTier tier = static_cast<ExpansionTier>(tierIdx);
        // Get bracket for neutral faction (combined)
        LevelBracket const* bracket = dist->GetBracketForTier(tier, TEAM_NEUTRAL);
        if (bracket)
        {
            demands.push_back(CalculateBracketDemand(bracket));
        }
    }

    // Sort by urgency (highest first)
    std::sort(demands.begin(), demands.end(),
        [](auto const& a, auto const& b) { return a.urgency > b.urgency; });

    return demands;
}

BracketDemand DemandCalculator::CalculateBracketDemand(LevelBracket const* bracket) const
{
    BracketDemand demand;
    if (!bracket)
        return demand;

    demand.bracket = bracket;
    demand.tier = bracket->tier;

    BotLevelDistribution* dist = BotLevelDistribution::instance();
    if (!dist)
        return demand;

    // Get current and target counts
    demand.currentBotCount = bracket->GetCount();
    auto stats = dist->GetDistributionStats();
    demand.targetBotCount = bracket->GetTargetCount(stats.totalBots);

    // Get protected count
    if (_protectionRegistry)
    {
        demand.protectedCount = _protectionRegistry->GetProtectedCountInBracket(bracket);
    }

    // Calculate deficit
    demand.deficit = CalculateEffectiveDeficit(bracket);

    // Get player count
    if (_activityTracker)
    {
        auto bracketCounts = _activityTracker->GetPlayerCountByBracket();
        demand.playerCount = bracketCounts[bracket->tier];

        // Get demand zones (zones with player activity)
        auto zones = _activityTracker->GetZonesWithPlayersAtLevel(
            (bracket->minLevel + bracket->maxLevel) / 2, 10);
        demand.demandZones = zones;
    }

    // Get flow predictions
    if (_flowPredictor)
    {
        auto prediction = _flowPredictor->PredictBracketFlow(bracket,
            std::chrono::hours(1));
        demand.predictedOutflow = prediction.predictedOutflow;
        demand.predictedInflow = prediction.predictedInflow;
    }

    // Calculate urgency
    demand.urgency = CalculateUrgency(bracket, demand.deficit);

    return demand;
}

int32 DemandCalculator::CalculateEffectiveDeficit(LevelBracket const* bracket) const
{
    if (!bracket)
        return 0;

    BotLevelDistribution* dist = BotLevelDistribution::instance();
    if (!dist)
        return 0;

    int32 currentCount = static_cast<int32>(bracket->GetCount());
    auto stats = dist->GetDistributionStats();
    int32 targetCount = static_cast<int32>(bracket->GetTargetCount(stats.totalBots));

    // Basic deficit
    int32 deficit = targetCount - currentCount;

    // Factor in predicted flow
    if (_flowPredictor)
    {
        auto prediction = _flowPredictor->PredictBracketFlow(bracket,
            std::chrono::hours(1));

        // If we're losing bots faster than gaining them, increase deficit
        int32 netFlow = static_cast<int32>(prediction.predictedInflow) -
                       static_cast<int32>(prediction.predictedOutflow);

        // Adjust deficit by predicted loss (weighted)
        if (netFlow < 0)
        {
            deficit -= netFlow;  // Increase deficit by predicted loss
        }
    }

    // Subtract protected bots from effective count for retirement decisions
    // (protected bots are "permanent" and shouldn't count towards overpopulation)

    return deficit;
}

float DemandCalculator::CalculateUrgency(LevelBracket const* bracket, int32 deficit) const
{
    if (!bracket || deficit <= 0)
        return 0.0f;

    BotLevelDistribution* dist = BotLevelDistribution::instance();
    if (!dist)
        return 0.0f;

    auto stats = dist->GetDistributionStats();
    uint32 targetCount = bracket->GetTargetCount(stats.totalBots);
    if (targetCount == 0)
        return 0.0f;

    // Base urgency from deficit percentage
    float deficitRatio = static_cast<float>(deficit) / targetCount;
    float urgency = std::min(1.0f, deficitRatio);

    // Bonus for player activity
    if (_activityTracker)
    {
        auto bracketCounts = _activityTracker->GetPlayerCountByBracket();
        uint32 playerCount = bracketCounts[bracket->tier];

        if (playerCount > 0)
        {
            // More players = more urgent
            urgency += 0.1f * std::min(playerCount, 10u);
        }
    }

    // Bonus for predicted outflow (bracket will empty soon)
    if (_flowPredictor)
    {
        auto prediction = _flowPredictor->PredictBracketFlow(bracket,
            std::chrono::hours(1));

        if (prediction.predictedOutflow > prediction.predictedInflow)
        {
            float flowImbalance = static_cast<float>(
                prediction.predictedOutflow - prediction.predictedInflow) / targetCount;
            urgency += flowImbalance * 0.5f;
        }
    }

    return std::min(1.0f, urgency);
}

// ============================================================================
// SPAWN REQUESTS
// ============================================================================

std::vector<DemandSpawnRequest> DemandCalculator::GenerateSpawnRequests(uint32 maxCount) const
{
    std::vector<DemandSpawnRequest> requests;

    auto demands = CalculateAllDemands();

    for (auto const& demand : demands)
    {
        if (requests.size() >= maxCount)
            break;

        // Skip if no deficit or too low urgency
        if (demand.deficit <= 0 ||
            demand.deficit < static_cast<int32>(_config.minDeficitForSpawn) ||
            demand.urgency < _config.minUrgencyForSpawn)
        {
            continue;
        }

        // Generate requests for this bracket
        uint32 toSpawn = std::min(
            static_cast<uint32>(demand.deficit),
            maxCount - static_cast<uint32>(requests.size()));

        for (uint32 i = 0; i < toSpawn; ++i)
        {
            DemandSpawnRequest request;
            request.tier = demand.tier;

            // Select level within bracket
            if (demand.bracket)
            {
                // Prefer mid-range levels
                uint32 range = demand.bracket->maxLevel - demand.bracket->minLevel;
                uint32 offset = range / 4 + (rand() % (range / 2 + 1));
                request.targetLevel = demand.bracket->minLevel + offset;
            }

            // Select zone
            if (!demand.demandZones.empty() && _config.prioritizePlayerZones)
            {
                // Prefer zones with player activity
                request.preferredZoneId = demand.demandZones[
                    rand() % demand.demandZones.size()];
            }
            else
            {
                request.preferredZoneId = SelectSpawnZoneForLevel(request.targetLevel);
            }

            // Calculate priority
            request.priority = demand.urgency * _config.bracketDeficitWeight;
            if (request.preferredZoneId != 0 &&
                _activityTracker &&
                _activityTracker->HasActivePlayersInZone(request.preferredZoneId))
            {
                request.priority += _config.playerProximityWeight;
            }

            // Set reason
            request.reason = "Bracket deficit: " + std::to_string(demand.deficit);

            requests.push_back(request);
        }
    }

    // Sort by priority (highest first)
    std::sort(requests.begin(), requests.end(),
        [](auto const& a, auto const& b) { return a.priority > b.priority; });

    return requests;
}

bool DemandCalculator::IsSpawningNeeded() const
{
    auto demands = CalculateAllDemands();

    for (auto const& demand : demands)
    {
        if (demand.deficit >= static_cast<int32>(_config.minDeficitForSpawn) &&
            demand.urgency >= _config.minUrgencyForSpawn)
        {
            return true;
        }
    }

    return false;
}

uint32 DemandCalculator::GetTotalSpawnDeficit() const
{
    uint32 total = 0;

    auto demands = CalculateAllDemands();
    for (auto const& demand : demands)
    {
        if (demand.deficit > 0)
        {
            total += demand.deficit;
        }
    }

    return total;
}

// ============================================================================
// ZONE SELECTION
// ============================================================================

uint32 DemandCalculator::SelectSpawnZoneForLevel(uint32 targetLevel) const
{
    auto scoredZones = GetTopSpawnZones(targetLevel, 5);

    if (scoredZones.empty())
        return 0;

    // Weighted random selection from top zones
    float totalScore = 0.0f;
    for (auto const& zone : scoredZones)
    {
        totalScore += zone.score;
    }

    if (totalScore <= 0)
        return scoredZones[0].zoneId;

    float roll = static_cast<float>(rand()) / RAND_MAX * totalScore;
    float cumulative = 0.0f;

    for (auto const& zone : scoredZones)
    {
        cumulative += zone.score;
        if (roll <= cumulative)
        {
            return zone.zoneId;
        }
    }

    return scoredZones[0].zoneId;
}

ZoneDemandScore DemandCalculator::ScoreZoneForSpawning(uint32 zoneId, uint32 targetLevel) const
{
    ZoneDemandScore score;
    score.zoneId = zoneId;
    score.recommendedLevel = targetLevel;

    // Get player activity
    if (_activityTracker)
    {
        auto summary = _activityTracker->GetZoneActivitySummary(zoneId);
        score.playerCount = summary.playerCount;
        score.hasActivePlayers = summary.hasActivePlayers;

        // Higher score for player presence
        if (summary.hasActivePlayers)
        {
            score.score += _config.playerProximityWeight;

            // Bonus if level matches
            if (summary.averageLevel > 0)
            {
                int32 levelDiff = std::abs(static_cast<int32>(targetLevel) -
                                          static_cast<int32>(summary.averageLevel));
                if (levelDiff <= 5)
                {
                    score.score += _config.playerProximityWeight * 0.5f;
                }
            }
        }
    }

    // Check if quest hub
    score.isQuestHub = IsQuestHub(zoneId);
    if (score.isQuestHub)
    {
        score.score += _config.questHubBonus;
    }

    // Get current bot count
    score.botCount = GetBotCountInZone(zoneId);

    // Penalty for overpopulated zones
    if (_config.avoidOverpopulatedZones && score.botCount > _config.maxBotsPerZone / 2)
    {
        float overpopPenalty = static_cast<float>(score.botCount) / _config.maxBotsPerZone;
        score.score *= (1.0f - overpopPenalty);
    }

    return score;
}

std::vector<ZoneDemandScore> DemandCalculator::GetTopSpawnZones(
    uint32 targetLevel, uint32 maxCount) const
{
    std::vector<ZoneDemandScore> scores;

    auto zones = GetZonesForLevel(targetLevel);

    for (uint32 zoneId : zones)
    {
        scores.push_back(ScoreZoneForSpawning(zoneId, targetLevel));
    }

    // Sort by score (highest first)
    std::sort(scores.begin(), scores.end(),
        [](auto const& a, auto const& b) { return a.score > b.score; });

    // Trim to max count
    if (scores.size() > maxCount)
    {
        scores.resize(maxCount);
    }

    return scores;
}

// ============================================================================
// DEPENDENCIES
// ============================================================================

void DemandCalculator::SetActivityTracker(PlayerActivityTracker* tracker)
{
    _activityTracker = tracker;
}

void DemandCalculator::SetProtectionRegistry(BotProtectionRegistry* registry)
{
    _protectionRegistry = registry;
}

void DemandCalculator::SetFlowPredictor(BracketFlowPredictor* predictor)
{
    _flowPredictor = predictor;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void DemandCalculator::SetConfig(DemandCalculatorConfig const& config)
{
    _config = config;
}

// ============================================================================
// STATISTICS
// ============================================================================

void DemandCalculator::PrintDemandReport() const
{
    TC_LOG_INFO("playerbot.lifecycle", "=== Demand Calculator Report ===");

    auto demands = CalculateAllDemands();

    for (auto const& demand : demands)
    {
        std::string tierName;
        switch (demand.tier)
        {
            case ExpansionTier::Starting:    tierName = "Starting"; break;
            case ExpansionTier::ChromieTime: tierName = "ChromieTime"; break;
            case ExpansionTier::Dragonflight: tierName = "Dragonflight"; break;
            case ExpansionTier::TheWarWithin: tierName = "TheWarWithin"; break;
        }

        TC_LOG_INFO("playerbot.lifecycle", "{}: Current={}, Target={}, Deficit={}, "
            "Players={}, Urgency={:.2f}, Zones={}",
            tierName, demand.currentBotCount, demand.targetBotCount, demand.deficit,
            demand.playerCount, demand.urgency, demand.demandZones.size());
    }

    if (IsSpawningNeeded())
    {
        TC_LOG_INFO("playerbot.lifecycle", "Spawning NEEDED: Total deficit = {}",
            GetTotalSpawnDeficit());
    }
    else
    {
        TC_LOG_INFO("playerbot.lifecycle", "Spawning NOT needed");
    }
}

// ============================================================================
// INTERNAL METHODS
// ============================================================================

void DemandCalculator::RecalculateDemands()
{
    std::lock_guard<std::mutex> lock(_cacheMutex);
    _cachedDemands = CalculateAllDemands();
    _lastRecalculate = std::chrono::system_clock::now();
}

std::vector<uint32> DemandCalculator::GetZonesForLevel(uint32 level) const
{
    std::vector<uint32> zones;

    // Get zones from activity tracker first
    if (_activityTracker)
    {
        zones = _activityTracker->GetZonesWithPlayersAtLevel(level, 10);
    }

    // If no active player zones found, return empty — StartupSpawnOrchestrator
    // handles default zone assignment for new bots.

    return zones;
}

bool DemandCalculator::IsQuestHub(uint32 zoneId) const
{
    // Query QuestHubDatabase for quest hubs in this zone
    auto const& hubDb = Playerbot::QuestHubDatabase::Instance();
    auto hubs = hubDb.GetQuestHubsInZone(zoneId);
    return !hubs.empty();
}

uint32 DemandCalculator::GetBotCountInZone(uint32 zoneId) const
{
    // Query BotSpawner for active bots in this zone
    auto bots = Playerbot::sBotSpawner->GetActiveBotsInZone(zoneId);
    return static_cast<uint32>(bots.size());
}

} // namespace Playerbot
