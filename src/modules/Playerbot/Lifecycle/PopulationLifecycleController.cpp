/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PopulationLifecycleController.h"
#include "Protection/BotProtectionRegistry.h"
#include "Retirement/BotRetirementManager.h"
#include "Prediction/BracketFlowPredictor.h"
#include "Demand/PlayerActivityTracker.h"
#include "Demand/DemandCalculator.h"
#include "Demand/PopulationPIDController.h"
#include "Character/BotLevelDistribution.h"
#include "Log.h"
#include "Config/PlayerbotConfig.h"
#include "Player.h"
#include <algorithm>
#include <ctime>

namespace Playerbot
{

PopulationLifecycleController* PopulationLifecycleController::Instance()
{
    static PopulationLifecycleController instance;
    return &instance;
}

bool PopulationLifecycleController::Initialize()
{
    if (_initialized.exchange(true))
        return true;

    LoadConfig();

    // Get component references
    _protectionRegistry = BotProtectionRegistry::Instance();
    _retirementManager = BotRetirementManager::Instance();
    _flowPredictor = BracketFlowPredictor::Instance();
    _activityTracker = PlayerActivityTracker::Instance();
    _demandCalculator = DemandCalculator::Instance();

    // Calculate initial bracket targets
    CalculateBracketTargets();

    // Initialize PID controller for smooth population management
    _pidController = std::make_unique<PopulationPIDController>();
    PIDControllerConfig pidConfig;
    pidConfig.Kp = 0.3f;
    pidConfig.Ki = 0.05f;
    pidConfig.Kd = 0.1f;
    pidConfig.deadband = 2.0f;
    pidConfig.outputMax = static_cast<float>(_config.maxCreationsPerHour);
    pidConfig.outputMin = -static_cast<float>(_config.maxRetirementsPerHour);
    _pidController->Initialize(pidConfig);

    // Initialize timing
    _hourStart = std::chrono::system_clock::now();
    _stats.lastUpdate = std::chrono::system_clock::now();
    _stats.lastAnalysis = std::chrono::system_clock::now();
    _stats.lastBalancing = std::chrono::system_clock::now();

    // Reserve decision history space
    _decisionHistory.reserve(MAX_DECISION_HISTORY);

    TC_LOG_INFO("playerbot.lifecycle", "PopulationLifecycleController initialized - "
        "Target population: {}, Brackets: {}%/{}%/{}%/{}%",
        _config.targetPopulation,
        _config.bracketTargetPct[0], _config.bracketTargetPct[1],
        _config.bracketTargetPct[2], _config.bracketTargetPct[3]);

    return true;
}

void PopulationLifecycleController::Shutdown()
{
    if (!_initialized.exchange(false))
        return;

    // Print final statistics
    PrintStatusReport();

    // Clear component references
    _protectionRegistry = nullptr;
    _retirementManager = nullptr;
    _flowPredictor = nullptr;
    _activityTracker = nullptr;
    _demandCalculator = nullptr;

    // Clear data
    {
        std::lock_guard<std::mutex> lock(_decisionMutex);
        _decisionHistory.clear();
    }
    {
        std::lock_guard<std::mutex> lock(_spawnRequestMutex);
        _pendingSpawnRequests.clear();
    }

    TC_LOG_INFO("playerbot.lifecycle", "PopulationLifecycleController shutdown complete");
}

void PopulationLifecycleController::Update(uint32 diff)
{
    if (!_initialized || !_config.enabled)
        return;

    // Reset hourly counters if needed
    ResetHourlyCountersIfNeeded();

    // Accumulate timers
    _analysisAccumulator += diff;
    _reportAccumulator += diff;
    _balancingAccumulator += diff;

    // Periodic analysis
    if (_analysisAccumulator >= _config.analysisIntervalMs)
    {
        _analysisAccumulator = 0;

        auto startTime = std::chrono::high_resolution_clock::now();

        // Analyze population state
        AnalyzePopulation();

        // Process retirements if needed
        ProcessRetirements();

        // Process creations if needed
        ProcessCreations();

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        {
            std::unique_lock<std::shared_mutex> lock(_statsMutex);
            _stats.lastAnalysisDurationMs = static_cast<uint32>(duration.count());
            _stats.lastAnalysis = std::chrono::system_clock::now();
        }
    }

    // Periodic status report
    if (_reportAccumulator >= _config.reportIntervalMs)
    {
        _reportAccumulator = 0;

        if (_config.logDetailedStats)
        {
            PrintStatusReport();
        }
    }

    // Periodic rebalancing
    if (_balancingAccumulator >= _config.balancingIntervalMs)
    {
        _balancingAccumulator = 0;

        // Check if major rebalancing is needed
        bool majorImbalance = false;
        for (uint32 i = 0; i < 4; ++i)
        {
            int32 deficit = _stats.brackets[i].deficit;
            if (std::abs(deficit) > static_cast<int32>(_config.targetPopulation / 10))
            {
                majorImbalance = true;
                break;
            }
        }

        if (majorImbalance)
        {
            TC_LOG_INFO("playerbot.lifecycle", "Major population imbalance detected, triggering rebalance");
            ForceRebalance();
        }
    }
}

void PopulationLifecycleController::LoadConfig()
{
    _config.enabled = sPlayerbotConfig->GetBool(
        "Playerbot.Lifecycle.Controller.Enable", true);
    _config.targetPopulation = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.TargetPopulation", 500);

    // Bracket targets
    _config.bracketTargetPct[0] = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Bracket.Starting.Pct", 15);
    _config.bracketTargetPct[1] = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Bracket.ChromieTime.Pct", 45);
    _config.bracketTargetPct[2] = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Bracket.Dragonflight.Pct", 20);
    _config.bracketTargetPct[3] = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Bracket.TheWarWithin.Pct", 20);

    // Creation settings
    _config.maxCreationsPerHour = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Creation.MaxPerHour", 30);
    _config.minDeficitForCreation = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Creation.MinDeficit", 5);
    _config.prioritizeDemand = sPlayerbotConfig->GetBool(
        "Playerbot.Lifecycle.Creation.PrioritizeDemand", true);

    // Retirement settings
    _config.maxRetirementsPerHour = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Retirement.MaxPerHour", 10);
    _config.minSurplusForRetirement = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Retirement.MinSurplus", 5);
    _config.pauseDuringPeakHours = sPlayerbotConfig->GetBool(
        "Playerbot.Lifecycle.Retirement.PauseDuringPeak", true);
    _config.peakHourStart = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Retirement.PeakHourStart", 18);
    _config.peakHourEnd = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Retirement.PeakHourEnd", 23);

    // Update intervals
    _config.analysisIntervalMs = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Controller.AnalysisIntervalMs", 60000);
    _config.reportIntervalMs = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Controller.ReportIntervalMs", 300000);
    _config.balancingIntervalMs = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Controller.BalancingIntervalMs", 300000);

    // Logging
    _config.logDecisions = sPlayerbotConfig->GetBool(
        "Playerbot.Lifecycle.Controller.LogDecisions", true);
    _config.logDetailedStats = sPlayerbotConfig->GetBool(
        "Playerbot.Lifecycle.Controller.LogDetailedStats", false);

    // Recalculate targets
    CalculateBracketTargets();

    TC_LOG_INFO("playerbot.lifecycle", "PopulationLifecycleController config loaded: "
        "Target={}, MaxCreate={}/hr, MaxRetire={}/hr",
        _config.targetPopulation, _config.maxCreationsPerHour, _config.maxRetirementsPerHour);
}

// ============================================================================
// POPULATION MANAGEMENT
// ============================================================================

LifecycleStats PopulationLifecycleController::AnalyzePopulation()
{
    if (!_initialized)
        return _stats;

    std::unique_lock<std::shared_mutex> lock(_statsMutex);

    // Update bracket statistics
    UpdateBracketStats();

    // Calculate totals
    _stats.totalBots = 0;
    _stats.protectedBots = 0;
    _stats.unprotectedBots = 0;

    for (uint32 i = 0; i < 4; ++i)
    {
        _stats.totalBots += _stats.brackets[i].currentBotCount;
        _stats.protectedBots += _stats.brackets[i].protectedCount;
    }
    _stats.unprotectedBots = _stats.totalBots - _stats.protectedBots;

    // Get retirement queue count
    if (_retirementManager)
    {
        _stats.botsInRetirementQueue = _retirementManager->GetQueueSize();
    }

    // Get player activity
    if (_activityTracker)
    {
        _stats.activePlayerCount = _activityTracker->GetActivePlayerCount();

        auto bracketCounts = _activityTracker->GetPlayerCountByBracket();
        for (auto const& [tier, count] : bracketCounts)
        {
            uint32 idx = static_cast<uint32>(tier);
            if (idx < 4)
            {
                _stats.playerCountPerBracket[idx] = count;
                _stats.brackets[idx].playerCount = count;
            }
        }
    }

    // Update PID controller with current population data per bracket
    if (_pidController)
    {
        for (uint32 i = 0; i < 4; ++i)
        {
            _pidController->UpdateBracket(
                i,
                static_cast<int32>(_stats.brackets[i].currentBotCount),
                static_cast<int32>(_stats.brackets[i].targetBotCount));
        }
        PIDOutput pidOutput = _pidController->ComputeAggregate();

        // Log PID state periodically for diagnostics
        if (_config.logDetailedStats)
        {
            TC_LOG_DEBUG("module.playerbot", "PID: spawns={} retires={} err={:.1f}",
                pidOutput.totalRecommendedSpawns, pidOutput.totalRecommendedRetirements,
                pidOutput.totalError);
        }
    }

    // Get pending spawn requests
    if (_demandCalculator)
    {
        _stats.pendingSpawnRequests = _demandCalculator->GetTotalSpawnDeficit();
    }

    // Update hourly counters
    _stats.botsCreatedThisHour = _creationsThisHour.load();
    _stats.botsRetiredThisHour = _retirementsThisHour.load();

    _stats.lastUpdate = std::chrono::system_clock::now();

    return _stats;
}

uint32 PopulationLifecycleController::ProcessRetirements()
{
    if (!_initialized || !_retirementManager)
        return 0;

    // Check if we should retire during peak hours
    if (_config.pauseDuringPeakHours && IsPeakHour())
    {
        return 0;
    }

    // Check rate limit
    if (!CanRetireMoreBots())
    {
        return 0;
    }

    return SelectBotsForRetirement();
}

uint32 PopulationLifecycleController::ProcessCreations()
{
    if (!_initialized || !_demandCalculator)
        return 0;

    // Check rate limit
    if (!CanCreateMoreBots())
    {
        return 0;
    }

    return GenerateSpawnRequests();
}

void PopulationLifecycleController::ForceRebalance()
{
    if (!_initialized)
        return;

    TC_LOG_INFO("playerbot.lifecycle", "Force rebalancing population distribution...");

    // Re-analyze population
    AnalyzePopulation();

    // Process retirements with increased priority
    uint32 retired = ProcessRetirements();

    // Process creations with increased priority
    uint32 created = ProcessCreations();

    TC_LOG_INFO("playerbot.lifecycle", "Rebalance complete: {} retirements queued, {} spawn requests generated",
        retired, created);

    // Record decision
    LifecycleDecision decision;
    decision.type = LifecycleDecision::Type::Rebalance;
    decision.reason = std::string("Rebalanced: ") + std::to_string(retired) + " retired, " +
                     std::to_string(created) + " created";
    decision.timestamp = std::chrono::system_clock::now();
    RecordDecision(decision);
}

// ============================================================================
// QUERIES
// ============================================================================

LifecycleStats const& PopulationLifecycleController::GetStats() const
{
    return _stats;
}

BracketStats PopulationLifecycleController::GetBracketStats(ExpansionTier tier) const
{
    uint32 idx = static_cast<uint32>(tier);
    if (idx < 4)
    {
        std::shared_lock<std::shared_mutex> lock(_statsMutex);
        return _stats.brackets[idx];
    }
    return BracketStats{};
}

bool PopulationLifecycleController::IsSpawningNeeded() const
{
    std::shared_lock<std::shared_mutex> lock(_statsMutex);

    for (uint32 i = 0; i < 4; ++i)
    {
        if (_stats.brackets[i].effectiveDeficit >= static_cast<int32>(_config.minDeficitForCreation))
        {
            return true;
        }
    }
    return false;
}

bool PopulationLifecycleController::IsRetirementNeeded() const
{
    std::shared_lock<std::shared_mutex> lock(_statsMutex);

    for (uint32 i = 0; i < 4; ++i)
    {
        // Negative deficit = surplus
        if (_stats.brackets[i].deficit < -static_cast<int32>(_config.minSurplusForRetirement))
        {
            // Only if there are retireable bots
            if (_stats.brackets[i].retireableCount > 0)
            {
                return true;
            }
        }
    }
    return false;
}

std::vector<DemandSpawnRequest> PopulationLifecycleController::GetPendingSpawnRequests(uint32 maxCount) const
{
    std::lock_guard<std::mutex> lock(_spawnRequestMutex);

    if (_pendingSpawnRequests.empty())
    {
        // Generate fresh spawn requests from demand calculator
        if (_demandCalculator)
        {
            return _demandCalculator->GenerateSpawnRequests(maxCount);
        }
        return {};
    }

    // Return cached requests
    std::vector<DemandSpawnRequest> result;
    uint32 count = std::min(maxCount, static_cast<uint32>(_pendingSpawnRequests.size()));
    result.reserve(count);

    for (uint32 i = 0; i < count; ++i)
    {
        result.push_back(_pendingSpawnRequests[i]);
    }

    return result;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void PopulationLifecycleController::SetConfig(LifecycleControllerConfig const& config)
{
    _config = config;
    CalculateBracketTargets();
}

// ============================================================================
// REPORTING
// ============================================================================

void PopulationLifecycleController::PrintStatusReport() const
{
    std::shared_lock<std::shared_mutex> lock(_statsMutex);

    TC_LOG_INFO("playerbot.lifecycle", "=== Population Lifecycle Status Report ===");
    TC_LOG_INFO("playerbot.lifecycle", "Total Bots: {} (Protected: {}, Unprotected: {})",
        _stats.totalBots, _stats.protectedBots, _stats.unprotectedBots);
    TC_LOG_INFO("playerbot.lifecycle", "Retirement Queue: {} | Active Players: {}",
        _stats.botsInRetirementQueue, _stats.activePlayerCount);
    TC_LOG_INFO("playerbot.lifecycle", "Hourly Activity: {} created, {} retired",
        _stats.botsCreatedThisHour, _stats.botsRetiredThisHour);

    TC_LOG_INFO("playerbot.lifecycle", "--- Bracket Status ---");
    const char* tierNames[] = {"Starting", "ChromieTime", "Dragonflight", "TheWarWithin"};

    for (uint32 i = 0; i < 4; ++i)
    {
        BracketStats const& bracket = _stats.brackets[i];
        TC_LOG_INFO("playerbot.lifecycle", "  {}: Current={} Target={} Deficit={} (Effective={}) Protected={} Players={}",
            tierNames[i],
            bracket.currentBotCount,
            bracket.targetBotCount,
            bracket.deficit,
            bracket.effectiveDeficit,
            bracket.protectedCount,
            bracket.playerCount);
    }

    TC_LOG_INFO("playerbot.lifecycle", "Performance: Last analysis took {}ms",
        _stats.lastAnalysisDurationMs);
}

void PopulationLifecycleController::PrintDecisionHistory(uint32 maxDecisions) const
{
    std::lock_guard<std::mutex> lock(_decisionMutex);

    TC_LOG_INFO("playerbot.lifecycle", "=== Recent Lifecycle Decisions ===");

    size_t count = std::min(static_cast<size_t>(maxDecisions), _decisionHistory.size());
    size_t startIdx = _decisionHistory.size() > static_cast<size_t>(maxDecisions) ?
                     _decisionHistory.size() - static_cast<size_t>(maxDecisions) : 0;

    for (size_t i = startIdx; i < _decisionHistory.size(); ++i)
    {
        LifecycleDecision const& decision = _decisionHistory[i];
        const char* typeNames[] = {"SpawnBot", "RetireBot", "CancelRetirement",
                                   "SkipSpawn", "SkipRetirement", "Rebalance"};

        TC_LOG_INFO("playerbot.lifecycle", "  [{}] Type={} Reason={}",
            i, typeNames[static_cast<int>(decision.type)], decision.reason);
    }
}

// ============================================================================
// EVENT HANDLERS
// ============================================================================

void PopulationLifecycleController::OnBotLeveledUp(ObjectGuid botGuid, uint32 oldLevel, uint32 newLevel)
{
    if (!_initialized)
        return;

    // Forward to flow predictor
    if (_flowPredictor)
    {
        _flowPredictor->OnBotLeveledUp(botGuid, oldLevel, newLevel);
    }

    // Update hourly counter
    {
        std::unique_lock<std::shared_mutex> lock(_statsMutex);
        _stats.botsLeveledUpThisHour++;
    }

    // Log if crossing bracket boundary
    BotLevelDistribution* dist = BotLevelDistribution::instance();
    if (dist)
    {
        LevelBracket const* oldBracket = dist->GetBracketForLevel(oldLevel, TEAM_NEUTRAL);
        LevelBracket const* newBracket = dist->GetBracketForLevel(newLevel, TEAM_NEUTRAL);

        if (oldBracket && newBracket && oldBracket->tier != newBracket->tier)
        {
            TC_LOG_DEBUG("playerbot.lifecycle", "Bot {} crossed bracket boundary: {} -> {}",
                botGuid.ToString(),
                static_cast<int>(oldBracket->tier),
                static_cast<int>(newBracket->tier));
        }
    }
}

void PopulationLifecycleController::OnBotCreated(ObjectGuid botGuid, uint32 level)
{
    if (!_initialized)
        return;

    // Forward to flow predictor
    if (_flowPredictor)
    {
        _flowPredictor->OnBotCreated(botGuid, level);
    }

    // Update creation counter
    _creationsThisHour++;

    TC_LOG_DEBUG("playerbot.lifecycle", "Bot {} created at level {}",
        botGuid.ToString(), level);
}

void PopulationLifecycleController::OnBotDeleted(ObjectGuid botGuid, uint32 level)
{
    if (!_initialized)
        return;

    // Forward to flow predictor
    if (_flowPredictor)
    {
        _flowPredictor->OnBotDeleted(botGuid);
    }

    // Update retirement counter
    _retirementsThisHour++;

    TC_LOG_DEBUG("playerbot.lifecycle", "Bot {} deleted at level {}",
        botGuid.ToString(), level);
}

void PopulationLifecycleController::OnProtectionChanged(ObjectGuid botGuid, bool isProtected)
{
    if (!_initialized)
        return;

    // Forward to retirement manager to potentially cancel retirements
    if (_retirementManager && isProtected)
    {
        _retirementManager->CancelRetirement(botGuid, RetirementCancelReason::AdminProtected);
    }

    TC_LOG_DEBUG("playerbot.lifecycle", "Bot {} protection changed to {}",
        botGuid.ToString(), isProtected ? "protected" : "unprotected");
}

void PopulationLifecycleController::OnPlayerLogin(Player* player)
{
    if (!_initialized || !player)
        return;

    // Forward to activity tracker
    if (_activityTracker)
    {
        _activityTracker->OnPlayerLogin(player);
    }
}

void PopulationLifecycleController::OnPlayerLogout(Player* player)
{
    if (!_initialized || !player)
        return;

    // Forward to activity tracker
    if (_activityTracker)
    {
        _activityTracker->OnPlayerLogout(player);
    }
}

// ============================================================================
// INTERNAL ANALYSIS
// ============================================================================

void PopulationLifecycleController::CalculateBracketTargets()
{
    // Normalize percentages if they don't add up to 100
    uint32 totalPct = 0;
    for (uint32 i = 0; i < 4; ++i)
    {
        totalPct += _config.bracketTargetPct[i];
    }

    if (totalPct == 0)
    {
        // Default to equal distribution
        for (uint32 i = 0; i < 4; ++i)
        {
            _bracketTargets[i] = _config.targetPopulation / 4;
        }
        return;
    }

    // Calculate targets based on percentages
    for (uint32 i = 0; i < 4; ++i)
    {
        _bracketTargets[i] = (_config.targetPopulation * _config.bracketTargetPct[i]) / totalPct;
    }

    // Ensure minimum of 1 for each bracket
    for (uint32 i = 0; i < 4; ++i)
    {
        if (_bracketTargets[i] == 0)
        {
            _bracketTargets[i] = 1;
        }
    }
}

void PopulationLifecycleController::UpdateBracketStats()
{
    BotLevelDistribution* dist = BotLevelDistribution::instance();
    if (!dist)
        return;

    // Get flow predictions if available
    std::array<FlowPrediction, 4> predictions = {};
    if (_flowPredictor)
    {
        auto allPredictions = _flowPredictor->PredictAllBrackets(std::chrono::hours(1));
        for (auto const& pred : allPredictions)
        {
            LevelBracket const* bracket = pred.bracket;
            if (bracket)
            {
                uint32 idx = static_cast<uint32>(bracket->tier);
                if (idx < 4)
                {
                    predictions[idx] = pred;
                }
            }
        }
    }

    // Update each bracket
    for (uint32 i = 0; i < 4; ++i)
    {
        ExpansionTier tier = static_cast<ExpansionTier>(i);
        LevelBracket const* bracket = dist->GetBracketForTier(tier, TEAM_NEUTRAL);
        if (!bracket)
            continue;

        BracketStats& stats = _stats.brackets[i];
        stats.tier = tier;
        stats.targetBotCount = _bracketTargets[i];

        // Get current bot count
        stats.currentBotCount = bracket->GetCount();

        // Get protection count
        if (_protectionRegistry)
        {
            stats.protectedCount = _protectionRegistry->GetProtectedCountInBracket(bracket);
        }
        stats.retireableCount = stats.currentBotCount - stats.protectedCount;

        // Calculate deficit
        stats.deficit = static_cast<int32>(stats.targetBotCount) - static_cast<int32>(stats.currentBotCount);

        // Calculate effective deficit (considering flow)
        stats.predictedOutflow = predictions[i].predictedOutflow;
        stats.predictedInflow = predictions[i].predictedInflow;
        stats.effectiveDeficit = CalculateEffectiveDeficit(tier);

        // Calculate urgency (0.0 - 1.0)
        if (stats.targetBotCount > 0)
        {
            float deficitRatio = static_cast<float>(std::abs(stats.deficit)) /
                                static_cast<float>(stats.targetBotCount);
            stats.urgency = std::min(1.0f, deficitRatio);
        }
    }
}

int32 PopulationLifecycleController::CalculateEffectiveDeficit(ExpansionTier tier) const
{
    uint32 idx = static_cast<uint32>(tier);
    if (idx >= 4)
        return 0;

    BracketStats const& stats = _stats.brackets[idx];

    // Start with raw deficit
    int32 effective = stats.deficit;

    // Adjust for predicted flow
    // If outflow is high, we'll need more bots soon (increase deficit)
    // If inflow is high, we'll have more bots soon (decrease deficit)
    effective += static_cast<int32>(stats.predictedOutflow);
    effective -= static_cast<int32>(stats.predictedInflow);

    // Account for protected bots when deficit is negative (surplus)
    if (effective < 0)
    {
        // We can only retire unprotected bots
        int32 maxRetireable = static_cast<int32>(stats.retireableCount);
        if (-effective > maxRetireable)
        {
            effective = -maxRetireable;
        }
    }

    return effective;
}

bool PopulationLifecycleController::IsPeakHour() const
{
    auto now = std::chrono::system_clock::now();
    std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm* localTime = std::localtime(&nowTime);

    uint32 hour = localTime->tm_hour;
    return hour >= _config.peakHourStart && hour < _config.peakHourEnd;
}

bool PopulationLifecycleController::CanCreateMoreBots() const
{
    return _creationsThisHour.load() < _config.maxCreationsPerHour;
}

bool PopulationLifecycleController::CanRetireMoreBots() const
{
    return _retirementsThisHour.load() < _config.maxRetirementsPerHour;
}

// ============================================================================
// DECISION MAKING
// ============================================================================

uint32 PopulationLifecycleController::SelectBotsForRetirement()
{
    if (!_retirementManager || !_protectionRegistry)
        return 0;

    BotLevelDistribution* dist = BotLevelDistribution::instance();
    if (!dist)
        return 0;

    uint32 totalRetired = 0;

    // Find brackets with surplus
    for (uint32 i = 0; i < 4; ++i)
    {
        BracketStats const& stats = _stats.brackets[i];

        // Check if this bracket has surplus (negative deficit)
        if (stats.deficit >= -static_cast<int32>(_config.minSurplusForRetirement))
            continue;

        // Check if there are retireable bots
        if (stats.retireableCount == 0)
            continue;

        // Calculate how many to retire
        uint32 surplus = static_cast<uint32>(-stats.deficit);
        uint32 toRetire = std::min(surplus, stats.retireableCount);

        // Limit by rate
        uint32 remainingRate = _config.maxRetirementsPerHour - _retirementsThisHour.load();
        toRetire = std::min(toRetire, remainingRate);

        if (toRetire == 0)
            continue;

        // Get candidates from retirement manager
        ExpansionTier tier = static_cast<ExpansionTier>(i);
        LevelBracket const* bracket = dist->GetBracketForTier(tier, TEAM_NEUTRAL);
        if (!bracket)
            continue;

        auto candidates = _retirementManager->GetRetirementCandidates(bracket, toRetire);

        for (ObjectGuid const& botGuid : candidates)
        {
            // Double-check protection status
            if (_protectionRegistry->IsProtected(botGuid))
                continue;

            // Queue for retirement
            std::string reason = "Bracket surplus (tier " + std::to_string(i) + ")";
            _retirementManager->QueueForRetirement(botGuid, reason);

            // Record decision
            if (_config.logDecisions)
            {
                LifecycleDecision decision;
                decision.type = LifecycleDecision::Type::RetireBot;
                decision.bracket = tier;
                decision.botGuid = botGuid;
                decision.reason = reason;
                decision.timestamp = std::chrono::system_clock::now();
                RecordDecision(decision);
            }

            ++totalRetired;

            // Check rate limit
            if (!CanRetireMoreBots())
                break;
        }

        if (!CanRetireMoreBots())
            break;
    }

    if (totalRetired > 0)
    {
        TC_LOG_INFO("playerbot.lifecycle", "Queued {} bots for retirement",
            totalRetired);
    }

    return totalRetired;
}

uint32 PopulationLifecycleController::GenerateSpawnRequests()
{
    if (!_demandCalculator)
        return 0;

    // Calculate how many more we can create
    uint32 remainingRate = _config.maxCreationsPerHour - _creationsThisHour.load();
    if (remainingRate == 0)
        return 0;

    // Use PID controller to smooth the spawn count instead of raw deficit
    uint32 pidAdjustedCount = remainingRate;
    if (_pidController)
    {
        int32 smoothed = _pidController->GetSmoothedSpawnCount(
            remainingRate, _config.maxRetirementsPerHour);
        if (smoothed <= 0)
        {
            // PID says don't spawn (population at or above target)
            return 0;
        }
        pidAdjustedCount = std::min(static_cast<uint32>(smoothed), remainingRate);
    }

    // Generate spawn requests using PID-smoothed count
    auto requests = _demandCalculator->GenerateSpawnRequests(pidAdjustedCount);

    // Store in pending requests
    {
        std::lock_guard<std::mutex> lock(_spawnRequestMutex);
        _pendingSpawnRequests = std::move(requests);
    }

    uint32 count = static_cast<uint32>(_pendingSpawnRequests.size());

    // Record decisions
    if (_config.logDecisions && count > 0)
    {
        for (DemandSpawnRequest const& request : _pendingSpawnRequests)
        {
            LifecycleDecision decision;
            decision.type = LifecycleDecision::Type::SpawnBot;
            decision.bracket = request.tier;
            decision.zoneId = request.preferredZoneId;
            decision.targetLevel = request.targetLevel;
            decision.priority = request.priority;
            decision.reason = request.reason;
            decision.timestamp = std::chrono::system_clock::now();
            RecordDecision(decision);
        }
    }

    if (count > 0)
    {
        TC_LOG_INFO("playerbot.lifecycle", "Generated {} spawn requests",
            count);
    }

    return count;
}

void PopulationLifecycleController::RecordDecision(LifecycleDecision const& decision)
{
    std::lock_guard<std::mutex> lock(_decisionMutex);

    // Maintain circular buffer
    if (_decisionHistory.size() >= MAX_DECISION_HISTORY)
    {
        _decisionHistory.erase(_decisionHistory.begin());
    }

    _decisionHistory.push_back(decision);
}

// ============================================================================
// HOURLY TRACKING
// ============================================================================

void PopulationLifecycleController::ResetHourlyCountersIfNeeded()
{
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::hours>(now - _hourStart);

    if (elapsed.count() >= 1)
    {
        _creationsThisHour = 0;
        _retirementsThisHour = 0;
        _hourStart = now;

        {
            std::unique_lock<std::shared_mutex> lock(_statsMutex);
            _stats.botsLeveledUpThisHour = 0;
        }

        TC_LOG_DEBUG("playerbot.lifecycle", "Hourly counters reset");
    }
}

} // namespace Playerbot
