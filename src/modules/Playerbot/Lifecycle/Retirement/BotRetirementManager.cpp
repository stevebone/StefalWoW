/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BotRetirementManager.h"
#include "Log.h"
#include "Config/PlayerbotConfig.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "CharacterCache.h"
#include "Database/PlayerbotDatabase.h"
#include "DatabaseEnv.h"
#include "StringFormat.h"
#include "../Protection/BotProtectionRegistry.h"
#include "Character/BotLevelDistribution.h"
#include <ctime>
#include <iomanip>
#include <sstream>

namespace Playerbot
{

BotRetirementManager* BotRetirementManager::Instance()
{
    static BotRetirementManager instance;
    return &instance;
}

bool BotRetirementManager::Initialize()
{
    if (_initialized.exchange(true))
        return true;

    LoadConfig();

    // Initialize GracefulExitHandler
    if (!sGracefulExitHandler->Initialize())
    {
        TC_LOG_ERROR("playerbot.lifecycle", "Failed to initialize GracefulExitHandler");
        _initialized = false;
        return false;
    }

    // Set timing baselines
    auto now = std::chrono::system_clock::now();
    _hourStart = now;
    _dayStart = now;

    // Load queue from database
    LoadFromDatabase();

    TC_LOG_INFO("playerbot.lifecycle", "BotRetirementManager initialized. Queue size: {}",
        GetQueueSize());
    return true;
}

void BotRetirementManager::Shutdown()
{
    if (!_initialized.exchange(false))
        return;

    // Save all pending to database
    SaveToDatabase();

    // Shutdown graceful exit handler
    sGracefulExitHandler->Shutdown();

    TC_LOG_INFO("playerbot.lifecycle", "BotRetirementManager shutdown. "
        "Retirements today: {}, Total in queue: {}",
        _retirementsToday.load(), GetQueueSize());
}

void BotRetirementManager::Update(uint32 diff)
{
    if (!_initialized || !_config.enabled)
        return;

    // Accumulate time
    _updateAccumulator += diff;
    _dbSyncAccumulator += diff;
    _hourlyResetAccumulator += diff;

    // Hourly counter reset
    if (_hourlyResetAccumulator >= HOURLY_RESET_INTERVAL_MS)
    {
        _hourlyResetAccumulator = 0;
        UpdateHourlyCounters();
    }

    // Process queues at interval
    if (_updateAccumulator >= UPDATE_INTERVAL_MS)
    {
        _updateAccumulator = 0;

        // Process different queue states
        ProcessPendingQueue();
        ProcessCoolingQueue();
        ProcessExitingQueue();

        // Update statistics
        UpdateStatistics();
    }

    // Database sync
    if (_config.persistToDatabase && _dbSyncAccumulator >= _config.dbSyncIntervalMs)
    {
        _dbSyncAccumulator = 0;
        SaveToDatabase();
    }
}

void BotRetirementManager::LoadConfig()
{
    _config.enabled = sPlayerbotConfig->GetBool(
        "Playerbot.Lifecycle.Retirement.Enable", true);
    _config.coolingPeriodDays = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Retirement.CoolingPeriodDays", 7);
    _config.maxRetirementsPerHour = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Retirement.MaxPerHour", 10);
    _config.maxRetirementsPerDay = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Retirement.MaxPerDay", 100);
    _config.peakHourStart = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Retirement.PeakHourStart", 18);
    _config.peakHourEnd = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Retirement.PeakHourEnd", 23);
    _config.avoidPeakHours = sPlayerbotConfig->GetBool(
        "Playerbot.Lifecycle.Retirement.AvoidPeakHours", true);
    _config.gracefulExit = sPlayerbotConfig->GetBool(
        "Playerbot.Lifecycle.Retirement.GracefulExit", true);
    _config.gracefulExitTimeoutMs = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Retirement.GracefulExitTimeoutMs", 60000);
    _config.overpopulationWeight = sPlayerbotConfig->GetFloat(
        "Playerbot.Lifecycle.Retirement.OverpopulationWeight", 200.0f);
    _config.timeInBracketWeight = sPlayerbotConfig->GetFloat(
        "Playerbot.Lifecycle.Retirement.TimeInBracketWeight", 10.0f);
    _config.playtimeWeight = sPlayerbotConfig->GetFloat(
        "Playerbot.Lifecycle.Retirement.PlaytimeWeight", 0.1f);
    _config.interactionWeight = sPlayerbotConfig->GetFloat(
        "Playerbot.Lifecycle.Retirement.InteractionWeight", 5.0f);
    _config.minOverpopulationForRetirement = sPlayerbotConfig->GetFloat(
        "Playerbot.Lifecycle.Retirement.MinOverpopulation", 0.15f);
    _config.minPlaytimeBeforeRetirement = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Retirement.MinPlaytimeMinutes", 60);
    _config.persistToDatabase = sPlayerbotConfig->GetBool(
        "Playerbot.Lifecycle.Retirement.PersistToDatabase", true);
    _config.dbSyncIntervalMs = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Retirement.DbSyncIntervalMs", 60000);

    TC_LOG_INFO("playerbot.lifecycle", "BotRetirementManager config loaded: "
        "Enabled={}, CoolingDays={}, MaxPerHour={}, PeakHours={}-{}",
        _config.enabled, _config.coolingPeriodDays, _config.maxRetirementsPerHour,
        _config.peakHourStart, _config.peakHourEnd);
}

// ============================================================================
// QUEUE MANAGEMENT
// ============================================================================

bool BotRetirementManager::QueueForRetirement(ObjectGuid botGuid, std::string const& reason)
{
    if (!_initialized || !_config.enabled)
        return false;

    if (!botGuid)
    {
        TC_LOG_ERROR("playerbot.lifecycle", "QueueForRetirement called with invalid GUID");
        return false;
    }

    // Check if already in queue
    if (IsInRetirementQueue(botGuid))
    {
        TC_LOG_DEBUG("playerbot.lifecycle", "Bot {} already in retirement queue",
            botGuid.ToString());
        return false;
    }

    // Check if bot is protected
    if (_protectionRegistry && _protectionRegistry->IsProtected(botGuid))
    {
        TC_LOG_DEBUG("playerbot.lifecycle", "Bot {} is protected, cannot queue for retirement",
            botGuid.ToString());
        return false;
    }

    // Create candidate
    RetirementCandidate candidate(botGuid);
    candidate.retirementReason = reason;
    candidate.state = RetirementState::Pending;

    // Get bot info
    if (Player* player = ObjectAccessor::FindPlayer(botGuid))
    {
        candidate.botName = player->GetName();
        candidate.levelAtQueue = player->GetLevel();
        candidate.botClass = player->GetClass();
        candidate.botRace = player->GetRace();
    }
    else if (CharacterCacheEntry const* cache = sCharacterCache->GetCharacterCacheByGuid(botGuid))
    {
        candidate.botName = cache->Name;
        candidate.levelAtQueue = cache->Level;
        candidate.botClass = cache->Class;
        candidate.botRace = cache->Race;
    }

    // Get bracket info
    if (BotLevelDistribution* dist = BotLevelDistribution::instance())
    {
        if (LevelBracket const* bracket = dist->GetBracketForLevel(candidate.levelAtQueue, TEAM_NEUTRAL))
        {
            candidate.tierAtQueue = bracket->tier;
            candidate.bracketAtQueue = static_cast<uint8>(bracket->tier);
        }
    }

    // Get protection score
    if (_protectionRegistry)
    {
        candidate.protectionScoreAtQueue = _protectionRegistry->GetProtectionScore(botGuid);
        candidate.interactionCountAtQueue = _protectionRegistry->GetInteractionCount(botGuid);
    }

    // Get playtime
    candidate.playtimeMinutesAtQueue = GetBotPlaytime(botGuid);
    candidate.timeInBracketAtQueue = GetTimeInCurrentBracket(botGuid);

    // Calculate priority
    candidate.retirementPriority = CalculateRetirementPriority(botGuid);

    // Start cooling period
    candidate.StartCooling(_config.coolingPeriodDays);

    // Add to queue
    _retirementQueue.insert_or_assign(botGuid, candidate);
    MarkDirty(botGuid);

    TC_LOG_INFO("playerbot.lifecycle", "Bot {} ({}) queued for retirement. Reason: {}. "
        "Cooling ends: {} days",
        candidate.botName, botGuid.ToString(), reason, _config.coolingPeriodDays);

    _statsDirty = true;
    return true;
}

bool BotRetirementManager::CancelRetirement(ObjectGuid botGuid, RetirementCancelReason reason)
{
    if (!botGuid)
        return false;

    auto candidate = _retirementQueue.find(botGuid);
    if (candidate == _retirementQueue.end())
    {
        TC_LOG_DEBUG("playerbot.lifecycle", "Bot {} not in retirement queue, cannot cancel",
            botGuid.ToString());
        return false;
    }

    // Check if cancellation is still possible
    if (!CanCancelRetirement(candidate->second.state))
    {
        TC_LOG_WARN("playerbot.lifecycle", "Bot {} in state {}, cannot cancel retirement",
            botGuid.ToString(), RetirementStateToString(candidate->second.state));
        return false;
    }

    // Cancel the retirement
    candidate->second.Cancel(reason);

    TC_LOG_INFO("playerbot.lifecycle", "Bot {} ({}) rescued from retirement. Reason: {}",
        candidate->second.botName, botGuid.ToString(), RetirementCancelReasonToString(reason));

    // Remove from queue after processing
    RemoveCandidateFromDatabase(botGuid);
    _retirementQueue.erase(botGuid);

    _statsDirty = true;
    return true;
}

bool BotRetirementManager::IsInRetirementQueue(ObjectGuid botGuid) const
{
    return _retirementQueue.contains(botGuid);
}

RetirementState BotRetirementManager::GetRetirementState(ObjectGuid botGuid) const
{
    auto candidateIt = _retirementQueue.find(botGuid);
    return candidateIt != _retirementQueue.end() ? candidateIt->second.state : RetirementState::None;
}

RetirementCandidate BotRetirementManager::GetCandidate(ObjectGuid botGuid) const
{
    auto candidateIt = _retirementQueue.find(botGuid);
    return candidateIt != _retirementQueue.end() ? candidateIt->second : RetirementCandidate();
}

std::vector<RetirementCandidate> BotRetirementManager::GetCandidatesInState(RetirementState state) const
{
    std::vector<RetirementCandidate> result;

    _retirementQueue.for_each([&](auto const& pair) {
        if (pair.second.state == state)
            result.push_back(pair.second);
    });

    return result;
}

// ============================================================================
// CANDIDATE SELECTION
// ============================================================================

std::vector<ObjectGuid> BotRetirementManager::GetRetirementCandidates(
    LevelBracket const* bracket, uint32 maxCount)
{
    if (!bracket || !_protectionRegistry)
        return {};

    // Get unprotected bots in bracket
    auto unprotected = _protectionRegistry->GetUnprotectedBotsInBracket(bracket);

    // Filter out already queued bots
    std::vector<std::pair<ObjectGuid, float>> candidates;
    for (ObjectGuid guid : unprotected)
    {
        if (!IsInRetirementQueue(guid))
        {
            float priority = CalculateRetirementPriority(guid);
            candidates.emplace_back(guid, priority);
        }
    }

    // Sort by priority (highest first)
    std::sort(candidates.begin(), candidates.end(),
        [](auto const& a, auto const& b) { return a.second > b.second; });

    // Return top candidates
    std::vector<ObjectGuid> result;
    for (uint32 i = 0; i < maxCount && i < candidates.size(); ++i)
    {
        result.push_back(candidates[i].first);
    }

    return result;
}

float BotRetirementManager::CalculateRetirementPriority(ObjectGuid botGuid) const
{
    float priority = 0.0f;

    // Get protection score (higher protection = lower priority)
    float protectionScore = 0.0f;
    if (_protectionRegistry)
    {
        protectionScore = _protectionRegistry->GetProtectionScore(botGuid);
    }
    priority -= protectionScore;

    // Get bracket overpopulation
    uint32 level = 0;
    if (Player* player = ObjectAccessor::FindPlayer(botGuid))
    {
        level = player->GetLevel();
    }
    else if (CharacterCacheEntry const* cache = sCharacterCache->GetCharacterCacheByGuid(botGuid))
    {
        level = cache->Level;
    }

    if (BotLevelDistribution* dist = BotLevelDistribution::instance())
    {
        if (LevelBracket const* bracket = dist->GetBracketForLevel(level, TEAM_NEUTRAL))
        {
            float overpopulation = GetBracketOverpopulationRatio(bracket);
            priority += overpopulation * _config.overpopulationWeight;

            // More priority if overpopulated and been in bracket long
            if (overpopulation > _config.minOverpopulationForRetirement)
            {
                uint32 timeInBracket = GetTimeInCurrentBracket(botGuid);
                float hoursInBracket = timeInBracket / 3600.0f;
                priority += hoursInBracket * _config.timeInBracketWeight;
            }
        }
    }

    // Playtime factor (more playtime = lower priority)
    uint32 playtime = GetBotPlaytime(botGuid);
    priority -= std::min(playtime, 1000u) * _config.playtimeWeight;

    // Interaction factor
    uint32 interactions = 0;
    if (_protectionRegistry)
    {
        interactions = _protectionRegistry->GetInteractionCount(botGuid);
    }
    priority -= interactions * _config.interactionWeight;

    return priority;
}

float BotRetirementManager::GetBracketOverpopulationRatio(LevelBracket const* bracket) const
{
    if (!bracket)
        return 0.0f;

    BotLevelDistribution* dist = BotLevelDistribution::instance();
    if (!dist)
        return 0.0f;

    uint32 current = bracket->GetCount();
    auto stats = dist->GetDistributionStats();
    uint32 target = bracket->GetTargetCount(stats.totalBots);

    if (target == 0)
        return 0.0f;

    return (static_cast<float>(current) - target) / target;
}

bool BotRetirementManager::BracketNeedsRetirement(LevelBracket const* bracket) const
{
    return GetBracketOverpopulationRatio(bracket) > _config.minOverpopulationForRetirement;
}

// ============================================================================
// RATE LIMITING
// ============================================================================

bool BotRetirementManager::CanProcessMoreRetirements() const
{
    if (!_config.enabled)
        return false;

    if (_config.avoidPeakHours && IsPeakHour())
        return false;

    if (_retirementsThisHour >= _config.maxRetirementsPerHour)
        return false;

    if (_retirementsToday >= _config.maxRetirementsPerDay)
        return false;

    return true;
}

uint32 BotRetirementManager::GetRemainingCapacity() const
{
    uint32 hourlyRemaining = _config.maxRetirementsPerHour - _retirementsThisHour;
    uint32 dailyRemaining = _config.maxRetirementsPerDay - _retirementsToday;
    return std::min(hourlyRemaining, dailyRemaining);
}

bool BotRetirementManager::IsPeakHour() const
{
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm* tm = std::localtime(&time);

    uint32 hour = tm->tm_hour;
    return hour >= _config.peakHourStart && hour < _config.peakHourEnd;
}

// ============================================================================
// PROTECTION INTEGRATION
// ============================================================================

void BotRetirementManager::OnProtectionChanged(ObjectGuid botGuid, ProtectionStatus const& newStatus)
{
    if (!IsInRetirementQueue(botGuid))
        return;

    // If bot gained protection, cancel retirement
    if (newStatus.IsProtected())
    {
        RetirementCancelReason reason = RetirementCancelReason::None;

        // Determine specific reason
        if (newStatus.HasReason(ProtectionReason::InGuild))
            reason = RetirementCancelReason::JoinedGuild;
        else if (newStatus.HasReason(ProtectionReason::OnFriendList))
            reason = RetirementCancelReason::AddedToFriendList;
        else if (newStatus.HasReason(ProtectionReason::InPlayerGroup))
            reason = RetirementCancelReason::GroupedWithPlayer;
        else if (newStatus.HasReason(ProtectionReason::RecentInteract))
            reason = RetirementCancelReason::PlayerInteraction;
        else if (newStatus.HasReason(ProtectionReason::ManualProtect))
            reason = RetirementCancelReason::AdminProtected;
        else if (newStatus.HasReason(ProtectionReason::HasActiveMail))
            reason = RetirementCancelReason::ReceivedMail;
        else if (newStatus.HasReason(ProtectionReason::HasActiveAuction))
            reason = RetirementCancelReason::AuctionActivity;

        CancelRetirement(botGuid, reason);
    }
}

void BotRetirementManager::SetProtectionRegistry(BotProtectionRegistry* registry)
{
    _protectionRegistry = registry;
}

// ============================================================================
// STATISTICS
// ============================================================================

RetirementStatistics BotRetirementManager::GetStatistics() const
{
    if (!_statsDirty)
        return _cachedStats;

    RetirementStatistics stats;

    // Count by state
    _retirementQueue.for_each([&](auto const& pair) {
        switch (pair.second.state)
        {
            case RetirementState::Pending:
                ++stats.pendingCount;
                break;
            case RetirementState::Cooling:
                ++stats.coolingCount;
                break;
            case RetirementState::Preparing:
                ++stats.preparingCount;
                break;
            case RetirementState::Exiting:
                ++stats.exitingCount;
                break;
            default:
                break;
        }

        // Count by bracket
        if (pair.second.bracketAtQueue < 4)
            ++stats.queuedByBracket[pair.second.bracketAtQueue];
    });

    stats.completedThisHour = _retirementsThisHour;
    stats.completedToday = _retirementsToday;
    stats.maxPerHour = _config.maxRetirementsPerHour;
    stats.processedThisHour = _retirementsThisHour;
    stats.lastUpdate = std::chrono::system_clock::now();

    _cachedStats = stats;
    _statsDirty = false;

    return stats;
}

void BotRetirementManager::PrintStatusReport() const
{
    auto stats = GetStatistics();

    TC_LOG_INFO("playerbot.lifecycle", "=== Bot Retirement Manager Status ===");
    TC_LOG_INFO("playerbot.lifecycle", "Queue: {} pending, {} cooling, {} preparing, {} exiting",
        stats.pendingCount, stats.coolingCount, stats.preparingCount, stats.exitingCount);
    TC_LOG_INFO("playerbot.lifecycle", "Rate: {}/{} this hour, {}/{} today",
        stats.completedThisHour, _config.maxRetirementsPerHour,
        stats.completedToday, _config.maxRetirementsPerDay);
    TC_LOG_INFO("playerbot.lifecycle", "Peak hours: {} (currently {})",
        _config.avoidPeakHours ? "avoided" : "allowed",
        IsPeakHour() ? "peak" : "off-peak");
    TC_LOG_INFO("playerbot.lifecycle", "By bracket: Starting={}, ChromieTime={}, DF={}, TWW={}",
        stats.queuedByBracket[0], stats.queuedByBracket[1],
        stats.queuedByBracket[2], stats.queuedByBracket[3]);
}

uint32 BotRetirementManager::GetQueueSize() const
{
    return static_cast<uint32>(_retirementQueue.size());
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void BotRetirementManager::SetConfig(RetirementConfig const& config)
{
    _config = config;
}

// ============================================================================
// ADMIN OPERATIONS
// ============================================================================

bool BotRetirementManager::ForceRetirement(ObjectGuid botGuid)
{
    if (!botGuid)
        return false;

    // If not in queue, add it
    if (!IsInRetirementQueue(botGuid))
    {
        if (!QueueForRetirement(botGuid, "Admin forced retirement"))
            return false;
    }

    // Get candidate and force to exiting state
    auto candidate = _retirementQueue.find(botGuid);
    if (candidate == _retirementQueue.end())
        return false;

    candidate->second.state = RetirementState::Exiting;
    candidate->second.StartGracefulExit();

    ExecuteGracefulExit(candidate->second);
    return true;
}

void BotRetirementManager::ClearQueue()
{
    TC_LOG_INFO("playerbot.lifecycle", "Clearing retirement queue ({} candidates)",
        GetQueueSize());

    _retirementQueue.clear();
    {
        std::lock_guard<std::mutex> lock(_dirtyMutex);
        _dirtyCandidates.clear();
    }

    // Clear playerbot database
    sPlayerbotDatabase->Execute("DELETE FROM playerbot_retirement_queue");

    _statsDirty = true;
}

void BotRetirementManager::ProcessAllPending(bool ignoreRateLimits)
{
    TC_LOG_INFO("playerbot.lifecycle", "Processing all pending retirements (ignoreRateLimits={})",
        ignoreRateLimits);

    std::vector<ObjectGuid> toProcess;

    _retirementQueue.for_each([&](auto const& pair) {
        if (pair.second.state == RetirementState::Cooling &&
            pair.second.IsCoolingExpired())
        {
            toProcess.push_back(pair.first);
        }
    });

    for (ObjectGuid guid : toProcess)
    {
        if (!ignoreRateLimits && !CanProcessMoreRetirements())
            break;

        auto candidate = _retirementQueue.find(guid);
        if (candidate != _retirementQueue.end())
        {
            candidate->second.state = RetirementState::Exiting;
            candidate->second.StartGracefulExit();
            ExecuteGracefulExit(candidate->second);
        }
    }
}

// ============================================================================
// INTERNAL METHODS
// ============================================================================

void BotRetirementManager::ProcessPendingQueue()
{
    // Collect pending bots to transition to cooling (for_each is const)
    std::vector<ObjectGuid> pendingBots;
    _retirementQueue.for_each([&](auto const& pair) {
        if (pair.second.state == RetirementState::Pending)
        {
            pendingBots.push_back(pair.first);
        }
    });

    // Apply the transitions
    for (ObjectGuid const& guid : pendingBots)
    {
        auto it = _retirementQueue.find(guid);
        if (it != _retirementQueue.end())
        {
            it->second.StartCooling(_config.coolingPeriodDays);
            MarkDirty(guid);
        }
    }
}

void BotRetirementManager::ProcessCoolingQueue()
{
    if (!CanProcessMoreRetirements())
        return;

    std::vector<ObjectGuid> readyForExit;

    _retirementQueue.for_each([&](auto const& pair) {
        if (pair.second.state == RetirementState::Cooling &&
            pair.second.IsCoolingExpired())
        {
            readyForExit.push_back(pair.first);
        }
    });

    // Sort by priority and process up to rate limit
    std::sort(readyForExit.begin(), readyForExit.end(),
        [this](ObjectGuid a, ObjectGuid b) {
            auto candA = _retirementQueue.find(a);
            auto candB = _retirementQueue.find(b);
            if (candA == _retirementQueue.end() || candB == _retirementQueue.end())
                return false;
            return candA->second.retirementPriority > candB->second.retirementPriority;
        });

    uint32 processed = 0;
    for (ObjectGuid guid : readyForExit)
    {
        if (!CanProcessMoreRetirements())
            break;

        auto candidate = _retirementQueue.find(guid);
        if (candidate == _retirementQueue.end())
            continue;

        // Check protection one more time
        if (_protectionRegistry && _protectionRegistry->IsProtected(guid))
        {
            CancelRetirement(guid, RetirementCancelReason::PlayerInteraction);
            continue;
        }

        // Move to exiting state
        candidate->second.state = RetirementState::Preparing;
        candidate->second.state = RetirementState::Exiting;
        candidate->second.StartGracefulExit();
        MarkDirty(guid);

        if (_config.gracefulExit)
        {
            ExecuteGracefulExit(candidate->second);
        }
        else
        {
            FinalizeRetirement(guid);
        }

        ++processed;
    }

    if (processed > 0)
    {
        TC_LOG_INFO("playerbot.lifecycle", "Processed {} bots from cooling to exit",
            processed);
    }
}

void BotRetirementManager::ProcessExitingQueue()
{
    std::vector<ObjectGuid> inExit;

    _retirementQueue.for_each([&](auto const& pair) {
        if (pair.second.state == RetirementState::Exiting)
        {
            inExit.push_back(pair.first);
        }
    });

    for (ObjectGuid guid : inExit)
    {
        auto candidate = _retirementQueue.find(guid);
        if (candidate == _retirementQueue.end())
            continue;

        // Check for stuck stages
        if (candidate->second.IsStageTimedOut())
        {
            if (candidate->second.HasExceededRetries())
            {
                // Force advance to next stage
                TC_LOG_WARN("playerbot.lifecycle", "Bot {} stage {} timed out, force advancing",
                    guid.ToString(),
                    GracefulExitStageToString(candidate->second.exitStage));
                candidate->second.AdvanceExitStage();
            }
            else
            {
                candidate->second.RecordError("Stage timeout");
            }
            MarkDirty(guid);
        }

        // Check if exit complete
        if (candidate->second.exitStage == GracefulExitStage::Complete)
        {
            FinalizeRetirement(guid);
        }
    }
}

void BotRetirementManager::ExecuteGracefulExit(RetirementCandidate& candidate)
{
    sGracefulExitHandler->ExecuteStage(candidate,
        [this](ObjectGuid guid, StageResult const& result) {
            OnStageComplete(guid, result);
        });
}

void BotRetirementManager::FinalizeRetirement(ObjectGuid botGuid)
{
    auto candidate = _retirementQueue.find(botGuid);
    if (candidate == _retirementQueue.end())
        return;

    // Mark as completed
    candidate->second.Complete();

    TC_LOG_INFO("playerbot.lifecycle", "Bot {} ({}) retirement finalized",
        candidate->second.botName, botGuid.ToString());

    // Update counters
    ++_retirementsThisHour;
    ++_retirementsToday;

    // Update statistics by bracket
    _statsDirty = true;

    // Remove from queue
    RemoveCandidateFromDatabase(botGuid);
    _retirementQueue.erase(botGuid);
}

void BotRetirementManager::OnStageComplete(ObjectGuid botGuid, StageResult const& result)
{
    auto candidate = _retirementQueue.find(botGuid);
    if (candidate == _retirementQueue.end())
        return;

    if (result.success || result.advance)
    {
        candidate->second.AdvanceExitStage();
        MarkDirty(botGuid);

        // Continue to next stage if not complete
        if (candidate->second.exitStage != GracefulExitStage::Complete)
        {
            ExecuteGracefulExit(candidate->second);
        }
    }
    else if (result.retry)
    {
        candidate->second.RecordError(result.errorMessage);
        MarkDirty(botGuid);

        if (!candidate->second.HasExceededRetries())
        {
            // Retry after delay
            ExecuteGracefulExit(candidate->second);
        }
    }
}

void BotRetirementManager::UpdateHourlyCounters()
{
    auto now = std::chrono::system_clock::now();

    // Check if new hour
    auto hoursSinceHourStart = std::chrono::duration_cast<std::chrono::hours>(
        now - _hourStart).count();
    if (hoursSinceHourStart >= 1)
    {
        _hourStart = now;
        _retirementsThisHour = 0;
    }

    // Check if new day
    auto daysSinceDayStart = std::chrono::duration_cast<std::chrono::hours>(
        now - _dayStart).count();
    if (daysSinceDayStart >= 24)
    {
        _dayStart = now;
        _retirementsToday = 0;
    }
}

void BotRetirementManager::LoadFromDatabase()
{
    QueryResult result = sPlayerbotDatabase->Query(
        "SELECT bot_guid, queued_at, scheduled_deletion, retirement_reason, "
        "retirement_state, bracket_at_queue, protection_score_at_queue "
        "FROM playerbot_retirement_queue WHERE retirement_state NOT IN ('COMPLETED', 'CANCELLED')");

    if (!result)
    {
        TC_LOG_INFO("playerbot.lifecycle", "No pending retirements in database");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        ObjectGuid botGuid = ObjectGuid::Create<HighGuid::Player>(fields[0].GetUInt64());
        RetirementCandidate candidate(botGuid);

        // Parse timestamps
        std::string queuedAtStr = fields[1].GetString();
        std::string scheduledDeletionStr = fields[2].GetString();

        candidate.retirementReason = fields[3].GetString();

        std::string stateStr = fields[4].GetString();
        if (stateStr == "PENDING")
            candidate.state = RetirementState::Pending;
        else if (stateStr == "COOLING")
            candidate.state = RetirementState::Cooling;
        else if (stateStr == "PREPARING")
            candidate.state = RetirementState::Preparing;
        else if (stateStr == "EXITING")
            candidate.state = RetirementState::Exiting;

        candidate.bracketAtQueue = fields[5].GetUInt8();
        candidate.protectionScoreAtQueue = fields[6].GetFloat();

        // Get character info
        if (CharacterCacheEntry const* cache = sCharacterCache->GetCharacterCacheByGuid(botGuid))
        {
            candidate.botName = cache->Name;
            candidate.levelAtQueue = cache->Level;
            candidate.botClass = cache->Class;
            candidate.botRace = cache->Race;
        }

        _retirementQueue.insert_or_assign(botGuid, candidate);
        ++count;

    } while (result->NextRow());

    TC_LOG_INFO("playerbot.lifecycle", "Loaded {} pending retirements from database", count);
}

void BotRetirementManager::SaveToDatabase()
{
    std::set<ObjectGuid> dirty;
    {
        std::lock_guard<std::mutex> lock(_dirtyMutex);
        dirty = std::move(_dirtyCandidates);
        _dirtyCandidates.clear();
    }

    if (dirty.empty())
        return;

    // Save each dirty candidate to playerbot database
    for (ObjectGuid guid : dirty)
    {
        auto candidate = _retirementQueue.find(guid);
        if (candidate != _retirementQueue.end())
        {
            SaveCandidateToDatabase(candidate->second);
        }
    }

    TC_LOG_DEBUG("playerbot.lifecycle", "Saved {} retirement candidates to database",
        dirty.size());
}

void BotRetirementManager::SaveCandidateToDatabase(RetirementCandidate const& candidate)
{
    std::string stateStr;
    switch (candidate.state)
    {
        case RetirementState::Pending:   stateStr = "PENDING"; break;
        case RetirementState::Cooling:   stateStr = "COOLING"; break;
        case RetirementState::Preparing: stateStr = "PREPARING"; break;
        case RetirementState::Exiting:   stateStr = "EXITING"; break;
        case RetirementState::Cancelled: stateStr = "CANCELLED"; break;
        case RetirementState::Completed: stateStr = "COMPLETED"; break;
        default:                         stateStr = "NONE"; break;
    }

    sPlayerbotDatabase->Execute(Trinity::StringFormat(
        "REPLACE INTO playerbot_retirement_queue (bot_guid, reason, state, bracket, protection_score) "
        "VALUES ({}, '{}', '{}', {}, {})",
        candidate.botGuid.GetCounter(), candidate.retirementReason, stateStr,
        candidate.bracketAtQueue, candidate.protectionScoreAtQueue));
}

void BotRetirementManager::RemoveCandidateFromDatabase(ObjectGuid botGuid)
{
    sPlayerbotDatabase->Execute(Trinity::StringFormat(
        "DELETE FROM playerbot_retirement_queue WHERE bot_guid = {}",
        botGuid.GetCounter()));
}

void BotRetirementManager::MarkDirty(ObjectGuid botGuid)
{
    std::lock_guard<std::mutex> lock(_dirtyMutex);
    _dirtyCandidates.insert(botGuid);
}

uint32 BotRetirementManager::GetBotPlaytime(ObjectGuid botGuid) const
{
    if (Player* player = ObjectAccessor::FindPlayer(botGuid))
    {
        return player->GetTotalPlayedTime() / 60;  // Convert to minutes
    }

    // Query from database for offline bot
    QueryResult result = CharacterDatabase.Query(Trinity::StringFormat(
        "SELECT totaltime FROM characters WHERE guid = {}",
        botGuid.GetCounter()).c_str());

    if (result)
    {
        return (*result)[0].GetUInt32() / 60;  // Convert to minutes
    }

    return 0;
}

uint32 BotRetirementManager::GetTimeInCurrentBracket(ObjectGuid botGuid) const
{
    // This would ideally query from bracket tracking table
    // For now, return 0 (will be implemented in Phase 3 with BracketFlowPredictor)
    return 0;
}

void BotRetirementManager::UpdateStatistics()
{
    _statsDirty = true;
}

} // namespace Playerbot
