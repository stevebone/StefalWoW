/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BracketFlowPredictor.h"
#include "Log.h"
#include "Config/PlayerbotConfig.h"
#include "Database/PlayerbotDatabase.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "CharacterCache.h"
#include "DatabaseEnv.h"
#include "StringFormat.h"
#include "Character/BotLevelDistribution.h"
#include <cmath>
#include <algorithm>
#include <numeric>

namespace Playerbot
{

BracketFlowPredictor* BracketFlowPredictor::Instance()
{
    static BracketFlowPredictor instance;
    return &instance;
}

bool BracketFlowPredictor::Initialize()
{
    if (_initialized.exchange(true))
        return true;

    LoadConfig();

    // Initialize statistics with defaults
    {
        std::unique_lock<std::shared_mutex> lock(_statsMutex);
        for (size_t i = 0; i < 4; ++i)
        {
            _bracketStats[i].avgTimeInBracketSeconds =
                _config.defaultAvgTimeHours[i] * 3600;
            _bracketStats[i].sampleCount = 0;
            _bracketStats[i].lastUpdate = std::chrono::system_clock::now();
        }
    }

    // Load from database
    if (_config.persistToDatabase)
    {
        LoadStatisticsFromDatabase();
    }

    TC_LOG_INFO("playerbot.lifecycle", "BracketFlowPredictor initialized. "
        "Tracking {} bots", GetTrackedBotCount());
    return true;
}

void BracketFlowPredictor::Shutdown()
{
    if (!_initialized.exchange(false))
        return;

    // Save to database
    if (_config.persistToDatabase)
    {
        SaveStatisticsToDatabase();
    }

    TC_LOG_INFO("playerbot.lifecycle", "BracketFlowPredictor shutdown. "
        "Final tracked bots: {}", GetTrackedBotCount());
}

void BracketFlowPredictor::Update(uint32 diff)
{
    if (!_initialized || !_config.enabled)
        return;

    // Database sync
    if (_config.persistToDatabase)
    {
        _dbSyncAccumulator += diff;
        if (_dbSyncAccumulator >= _config.dbSyncIntervalMs)
        {
            _dbSyncAccumulator = 0;
            SaveStatisticsToDatabase();
        }
    }

    // Cleanup old records
    _cleanupAccumulator += diff;
    if (_cleanupAccumulator >= CLEANUP_INTERVAL_MS)
    {
        _cleanupAccumulator = 0;
        CleanupOldRecords();
    }
}

void BracketFlowPredictor::LoadConfig()
{
    _config.enabled = sPlayerbotConfig->GetBool(
        "Playerbot.Lifecycle.Prediction.Enable", true);
    _config.minSamplesForPrediction = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Prediction.MinSamples", 10);
    _config.confidenceThreshold = sPlayerbotConfig->GetFloat(
        "Playerbot.Lifecycle.Prediction.ConfidenceThreshold", 0.5f);
    _config.persistToDatabase = sPlayerbotConfig->GetBool(
        "Playerbot.Lifecycle.Prediction.PersistToDatabase", true);
    _config.dbSyncIntervalMs = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Prediction.DbSyncIntervalMs", 60000);
    _config.maxHistoryDays = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Prediction.MaxHistoryDays", 30);

    // Default average times per bracket (hours)
    _config.defaultAvgTimeHours[0] = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Prediction.DefaultTimeStarting", 2);
    _config.defaultAvgTimeHours[1] = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Prediction.DefaultTimeChromie", 24);
    _config.defaultAvgTimeHours[2] = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Prediction.DefaultTimeDragonflight", 12);
    _config.defaultAvgTimeHours[3] = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Prediction.DefaultTimeTWW", 0);  // Max level, no exit

    TC_LOG_INFO("playerbot.lifecycle", "BracketFlowPredictor config loaded: "
        "MinSamples={}, ConfidenceThreshold={:.2f}",
        _config.minSamplesForPrediction, _config.confidenceThreshold);
}

// ============================================================================
// EVENT TRACKING
// ============================================================================

void BracketFlowPredictor::OnBotLeveledUp(ObjectGuid botGuid, uint32 oldLevel, uint32 newLevel)
{
    if (!_initialized)
        return;

    BotLevelDistribution* dist = BotLevelDistribution::instance();
    if (!dist)
        return;

    LevelBracket const* oldBracket = dist->GetBracketForLevel(oldLevel, TEAM_NEUTRAL);
    LevelBracket const* newBracket = dist->GetBracketForLevel(newLevel, TEAM_NEUTRAL);

    if (!oldBracket || !newBracket)
        return;

    // Check if bracket changed
    if (oldBracket->tier != newBracket->tier)
    {
        OnBotLeftBracket(botGuid, oldBracket, "levelup");
        OnBotEnteredBracket(botGuid, newBracket);
    }
}

void BracketFlowPredictor::OnBotEnteredBracket(ObjectGuid botGuid, LevelBracket const* bracket)
{
    if (!_initialized || !bracket)
        return;

    BotBracketEntry entry;
    entry.botGuid = botGuid;
    entry.tier = bracket->tier;
    entry.bracketIndex = GetBracketIndex(bracket->tier);
    entry.entryTime = std::chrono::system_clock::now();

    // Get current level
    if (Player* player = ObjectAccessor::FindPlayer(botGuid))
    {
        entry.level = player->GetLevel();
    }
    else if (CharacterCacheEntry const* cache = sCharacterCache->GetCharacterCacheByGuid(botGuid))
    {
        entry.level = cache->Level;
    }

    _bracketEntries.insert_or_assign(botGuid, entry);

    TC_LOG_DEBUG("playerbot.lifecycle", "Bot {} entered bracket {} (tier {})",
        botGuid.ToString(), entry.bracketIndex,
        static_cast<uint8>(entry.tier));
}

void BracketFlowPredictor::OnBotLeftBracket(ObjectGuid botGuid, LevelBracket const* bracket,
                                            std::string const& reason)
{
    if (!_initialized || !bracket)
        return;

    auto entryIt = _bracketEntries.find(botGuid);
    if (entryIt == _bracketEntries.end())
        return;

    // Calculate time in bracket
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(
        now - entryIt->second.entryTime);
    uint32 timeSeconds = static_cast<uint32>(duration.count());

    // Update statistics
    uint8 bracketIndex = GetBracketIndex(bracket->tier);
    UpdateBracketStatistics(bracketIndex, timeSeconds);

    // Record to database
    if (_config.persistToDatabase && reason == "levelup")
    {
        uint8 nextBracket = (bracketIndex < 3) ? (bracketIndex + 1) : bracketIndex;
        RecordTransitionToDatabase(botGuid, bracketIndex, nextBracket, timeSeconds);
    }

    // Add to recent transitions
    {
        std::lock_guard<std::mutex> lock(_transitionMutex);
        RecentTransition trans;
        trans.bracketIndex = bracketIndex;
        trans.timeSeconds = timeSeconds;
        trans.when = now;
        _recentTransitions.push_back(trans);

        // Trim old transitions
        if (_recentTransitions.size() > MAX_RECENT_TRANSITIONS)
        {
            _recentTransitions.erase(_recentTransitions.begin(),
                _recentTransitions.begin() + (_recentTransitions.size() - MAX_RECENT_TRANSITIONS));
        }
    }

    // Remove from tracking if deleted
    if (reason == "deleted")
    {
        _bracketEntries.erase(botGuid);
    }

    TC_LOG_DEBUG("playerbot.lifecycle", "Bot {} left bracket {} after {}s (reason: {})",
        botGuid.ToString(), bracketIndex, timeSeconds, reason);
}

void BracketFlowPredictor::OnBotCreated(ObjectGuid botGuid, uint32 level)
{
    BotLevelDistribution* dist = BotLevelDistribution::instance();
    if (!dist)
        return;

    LevelBracket const* bracket = dist->GetBracketForLevel(level, TEAM_NEUTRAL);
    if (bracket)
    {
        OnBotEnteredBracket(botGuid, bracket);
    }
}

void BracketFlowPredictor::OnBotDeleted(ObjectGuid botGuid)
{
    auto entryIt = _bracketEntries.find(botGuid);
    if (entryIt != _bracketEntries.end())
    {
        BotLevelDistribution* dist = BotLevelDistribution::instance();
        if (dist)
        {
            LevelBracket const* bracket = dist->GetBracketForLevel(entryIt->second.level, TEAM_NEUTRAL);
            if (bracket)
            {
                OnBotLeftBracket(botGuid, bracket, "deleted");
            }
        }
    }
    _bracketEntries.erase(botGuid);
}

// ============================================================================
// PREDICTIONS
// ============================================================================

FlowPrediction BracketFlowPredictor::PredictBracketFlow(LevelBracket const* bracket,
                                                         std::chrono::seconds timeWindow) const
{
    FlowPrediction prediction;
    prediction.bracket = bracket;

    if (!bracket)
        return prediction;

    uint8 bracketIndex = GetBracketIndex(bracket->tier);
    BracketTransitionStats stats;
    {
        std::shared_lock<std::shared_mutex> lock(_statsMutex);
        stats = _bracketStats[bracketIndex];
    }

    prediction.sampleCount = stats.sampleCount;
    prediction.confidence = CalculateConfidence(stats);

    if (stats.sampleCount < _config.minSamplesForPrediction ||
        stats.avgTimeInBracketSeconds == 0)
    {
        // Not enough data for prediction
        prediction.confidence = 0.0f;
        return prediction;
    }

    // Count bots in this bracket and estimate outflow
    uint32 botsInBracket = 0;
    uint32 likelyToLeave = 0;

    _bracketEntries.for_each([&](auto const& pair) {
        if (pair.second.bracketIndex == bracketIndex)
        {
            ++botsInBracket;

            // Check if this bot is likely to leave within time window
            auto timeInBracket = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now() - pair.second.entryTime);
            auto remainingTime = std::chrono::seconds(stats.avgTimeInBracketSeconds) - timeInBracket;

            if (remainingTime <= timeWindow)
            {
                ++likelyToLeave;
            }
        }
    });

    prediction.predictedOutflow = likelyToLeave;

    // Estimate inflow from previous bracket
    if (bracketIndex > 0)
    {
        BracketTransitionStats prevStats;
        {
            std::shared_lock<std::shared_mutex> lock(_statsMutex);
            prevStats = _bracketStats[bracketIndex - 1];
        }

        uint32 botsInPrevBracket = 0;
        uint32 likelyToEnter = 0;

        _bracketEntries.for_each([&](auto const& pair) {
            if (pair.second.bracketIndex == bracketIndex - 1)
            {
                ++botsInPrevBracket;

                if (prevStats.avgTimeInBracketSeconds > 0)
                {
                    auto timeInBracket = std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::system_clock::now() - pair.second.entryTime);
                    auto remainingTime = std::chrono::seconds(prevStats.avgTimeInBracketSeconds) - timeInBracket;

                    if (remainingTime <= timeWindow)
                    {
                        ++likelyToEnter;
                    }
                }
            }
        });

        prediction.predictedInflow = likelyToEnter;
    }

    prediction.netChange = static_cast<int32>(prediction.predictedOutflow) -
                          static_cast<int32>(prediction.predictedInflow);

    // Calculate time to empty
    if (prediction.predictedOutflow > prediction.predictedInflow && botsInBracket > 0)
    {
        float outflowRate = static_cast<float>(prediction.predictedOutflow) /
                           static_cast<float>(timeWindow.count());
        float inflowRate = static_cast<float>(prediction.predictedInflow) /
                          static_cast<float>(timeWindow.count());
        float netRate = outflowRate - inflowRate;

        if (netRate > 0)
        {
            float secondsToEmpty = static_cast<float>(botsInBracket) / netRate;
            prediction.timeToEmpty = std::chrono::seconds(static_cast<int64_t>(secondsToEmpty));
        }
    }

    return prediction;
}

std::vector<FlowPrediction> BracketFlowPredictor::PredictAllBrackets(
    std::chrono::seconds timeWindow) const
{
    std::vector<FlowPrediction> predictions;

    BotLevelDistribution* dist = BotLevelDistribution::instance();
    if (!dist)
        return predictions;

    // Iterate over all expansion tiers
    for (uint8 tierIdx = 0; tierIdx < static_cast<uint8>(ExpansionTier::Max); ++tierIdx)
    {
        ExpansionTier tier = static_cast<ExpansionTier>(tierIdx);
        LevelBracket const* bracket = dist->GetBracketForTier(tier, TEAM_NEUTRAL);
        if (bracket)
        {
            predictions.push_back(PredictBracketFlow(bracket, timeWindow));
        }
    }

    return predictions;
}

std::chrono::seconds BracketFlowPredictor::GetAverageTimeInBracket(
    LevelBracket const* bracket) const
{
    if (!bracket)
        return std::chrono::seconds(0);

    uint8 bracketIndex = GetBracketIndex(bracket->tier);

    std::shared_lock<std::shared_mutex> lock(_statsMutex);
    return std::chrono::seconds(_bracketStats[bracketIndex].avgTimeInBracketSeconds);
}

std::chrono::seconds BracketFlowPredictor::GetAverageTimeInTier(ExpansionTier tier) const
{
    uint8 bracketIndex = GetBracketIndex(tier);

    std::shared_lock<std::shared_mutex> lock(_statsMutex);
    return std::chrono::seconds(_bracketStats[bracketIndex].avgTimeInBracketSeconds);
}

std::vector<ObjectGuid> BracketFlowPredictor::GetBotsLikelyToLeave(
    LevelBracket const* bracket, std::chrono::seconds timeWindow) const
{
    std::vector<std::pair<ObjectGuid, std::chrono::seconds>> candidates;

    if (!bracket)
        return {};

    uint8 bracketIndex = GetBracketIndex(bracket->tier);
    BracketTransitionStats stats;
    {
        std::shared_lock<std::shared_mutex> lock(_statsMutex);
        stats = _bracketStats[bracketIndex];
    }

    if (stats.avgTimeInBracketSeconds == 0)
        return {};

    auto avgTime = std::chrono::seconds(stats.avgTimeInBracketSeconds);
    auto now = std::chrono::system_clock::now();

    _bracketEntries.for_each([&](auto const& pair) {
        if (pair.second.bracketIndex == bracketIndex)
        {
            auto timeInBracket = std::chrono::duration_cast<std::chrono::seconds>(
                now - pair.second.entryTime);
            auto remainingTime = avgTime - timeInBracket;

            if (remainingTime <= timeWindow)
            {
                candidates.emplace_back(pair.first, remainingTime);
            }
        }
    });

    // Sort by remaining time (shortest first)
    std::sort(candidates.begin(), candidates.end(),
        [](auto const& a, auto const& b) { return a.second < b.second; });

    std::vector<ObjectGuid> result;
    result.reserve(candidates.size());
    for (auto const& [guid, _] : candidates)
    {
        result.push_back(guid);
    }

    return result;
}

std::chrono::seconds BracketFlowPredictor::EstimateTimeUntilBracketExit(ObjectGuid botGuid) const
{
    auto entryIt = _bracketEntries.find(botGuid);
    if (entryIt == _bracketEntries.end())
        return std::chrono::seconds(0);

    BracketTransitionStats stats;
    {
        std::shared_lock<std::shared_mutex> lock(_statsMutex);
        stats = _bracketStats[entryIt->second.bracketIndex];
    }

    if (stats.avgTimeInBracketSeconds == 0)
        return std::chrono::seconds(0);

    auto timeInBracket = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now() - entryIt->second.entryTime);
    auto avgTime = std::chrono::seconds(stats.avgTimeInBracketSeconds);

    if (timeInBracket >= avgTime)
        return std::chrono::seconds(0);

    return avgTime - timeInBracket;
}

// ============================================================================
// TIME IN BRACKET QUERIES
// ============================================================================

std::chrono::seconds BracketFlowPredictor::GetTimeInCurrentBracket(ObjectGuid botGuid) const
{
    auto entryIt = _bracketEntries.find(botGuid);
    if (entryIt == _bracketEntries.end())
        return std::chrono::seconds(0);

    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now() - entryIt->second.entryTime);
}

BotBracketEntry const* BracketFlowPredictor::GetBracketEntry(ObjectGuid botGuid) const
{
    auto entryIt = _bracketEntries.find(botGuid);
    return entryIt != _bracketEntries.end() ? &entryIt->second : nullptr;
}

// ============================================================================
// STATISTICS
// ============================================================================

BracketTransitionStats BracketFlowPredictor::GetTransitionStats(LevelBracket const* bracket) const
{
    if (!bracket)
        return {};

    uint8 bracketIndex = GetBracketIndex(bracket->tier);
    std::shared_lock<std::shared_mutex> lock(_statsMutex);
    return _bracketStats[bracketIndex];
}

BracketTransitionStats BracketFlowPredictor::GetTierStats(ExpansionTier tier) const
{
    uint8 bracketIndex = GetBracketIndex(tier);
    std::shared_lock<std::shared_mutex> lock(_statsMutex);
    return _bracketStats[bracketIndex];
}

void BracketFlowPredictor::PrintStatisticsReport() const
{
    TC_LOG_INFO("playerbot.lifecycle", "=== Bracket Flow Predictor Statistics ===");
    TC_LOG_INFO("playerbot.lifecycle", "Tracked bots: {}", GetTrackedBotCount());

    std::shared_lock<std::shared_mutex> lock(_statsMutex);
    for (size_t i = 0; i < 4; ++i)
    {
        auto const& stats = _bracketStats[i];
        float avgHours = stats.avgTimeInBracketSeconds / 3600.0f;

        TC_LOG_INFO("playerbot.lifecycle", "Bracket {}: avg={:.1f}h, samples={}, "
            "min={}s, max={}s, stddev={:.0f}s",
            i, avgHours, stats.sampleCount,
            stats.minTimeSeconds == UINT32_MAX ? 0 : stats.minTimeSeconds,
            stats.maxTimeSeconds,
            stats.stdDevSeconds);
    }

    // Count bots per bracket
    std::array<uint32, 4> botsPerBracket = {0, 0, 0, 0};
    _bracketEntries.for_each([&](auto const& pair) {
        if (pair.second.bracketIndex < 4)
            ++botsPerBracket[pair.second.bracketIndex];
    });

    TC_LOG_INFO("playerbot.lifecycle", "Bots per bracket: Starting={}, Chromie={}, DF={}, TWW={}",
        botsPerBracket[0], botsPerBracket[1], botsPerBracket[2], botsPerBracket[3]);
}

uint32 BracketFlowPredictor::GetTrackedBotCount() const
{
    return static_cast<uint32>(_bracketEntries.size());
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void BracketFlowPredictor::SetConfig(FlowPredictionConfig const& config)
{
    _config = config;
}

// ============================================================================
// DATABASE OPERATIONS
// ============================================================================

void BracketFlowPredictor::LoadStatisticsFromDatabase()
{
    QueryResult result = sPlayerbotDatabase->Query(
        "SELECT bracket_id, avg_time_in_bracket_seconds, sample_count "
        "FROM playerbot_bracket_statistics");

    if (!result)
    {
        TC_LOG_INFO("playerbot.lifecycle", "No bracket statistics in database");
        return;
    }

    std::unique_lock<std::shared_mutex> lock(_statsMutex);

    do
    {
        Field* fields = result->Fetch();
        uint8 bracketId = fields[0].GetUInt8();
        uint32 avgTime = fields[1].GetUInt32();
        uint32 samples = fields[2].GetUInt32();

        if (bracketId < 4)
        {
            _bracketStats[bracketId].avgTimeInBracketSeconds = avgTime;
            _bracketStats[bracketId].sampleCount = samples;
            _bracketStats[bracketId].lastUpdate = std::chrono::system_clock::now();
        }
    } while (result->NextRow());

    TC_LOG_INFO("playerbot.lifecycle", "Loaded bracket statistics from database");
}

void BracketFlowPredictor::SaveStatisticsToDatabase()
{
    std::shared_lock<std::shared_mutex> lock(_statsMutex);
    for (uint8 i = 0; i < 4; ++i)
    {
        auto const& stats = _bracketStats[i];

        sPlayerbotDatabase->Execute(Trinity::StringFormat(
            "REPLACE INTO playerbot_bracket_statistics (bracket_id, avg_time_in_bracket_seconds, sample_count) "
            "VALUES ({}, {}, {})",
            i, stats.avgTimeInBracketSeconds, stats.sampleCount).c_str());
    }

    TC_LOG_DEBUG("playerbot.lifecycle", "Saved bracket statistics to database");
}

void BracketFlowPredictor::RecordTransitionToDatabase(ObjectGuid botGuid, uint8 fromBracket,
                                                       uint8 toBracket, uint32 timeInBracketSeconds)
{
    sPlayerbotDatabase->Execute(Trinity::StringFormat(
        "INSERT INTO playerbot_bracket_transitions (bot_guid, from_bracket, to_bracket, time_in_bracket_seconds) "
        "VALUES ({}, {}, {}, {})",
        botGuid.GetCounter(), fromBracket, toBracket, timeInBracketSeconds).c_str());
}

// ============================================================================
// INTERNAL METHODS
// ============================================================================

void BracketFlowPredictor::UpdateBracketStatistics(uint8 bracketIndex, uint32 timeSeconds)
{
    if (bracketIndex >= 4)
        return;

    std::unique_lock<std::shared_mutex> lock(_statsMutex);
    auto& stats = _bracketStats[bracketIndex];

    // Update min/max
    stats.minTimeSeconds = std::min(stats.minTimeSeconds, timeSeconds);
    stats.maxTimeSeconds = std::max(stats.maxTimeSeconds, timeSeconds);

    // Update running average
    uint32 oldCount = stats.sampleCount;
    uint32 oldAvg = stats.avgTimeInBracketSeconds;

    stats.sampleCount++;
    stats.avgTimeInBracketSeconds = oldAvg +
        (static_cast<int32>(timeSeconds) - static_cast<int32>(oldAvg)) / stats.sampleCount;

    // Update standard deviation (Welford's algorithm)
    if (stats.sampleCount > 1)
    {
        float delta = timeSeconds - oldAvg;
        float delta2 = timeSeconds - stats.avgTimeInBracketSeconds;
        float variance = ((oldCount - 1) * (stats.stdDevSeconds * stats.stdDevSeconds) +
                         delta * delta2) / (stats.sampleCount - 1);
        stats.stdDevSeconds = std::sqrt(std::max(0.0f, variance));
    }

    stats.lastUpdate = std::chrono::system_clock::now();
}

float BracketFlowPredictor::CalculateConfidence(BracketTransitionStats const& stats) const
{
    if (stats.sampleCount < _config.minSamplesForPrediction)
        return 0.0f;

    // Confidence increases with sample count, decreases with stddev
    float sampleFactor = std::min(1.0f, static_cast<float>(stats.sampleCount) / 100.0f);
    float consistencyFactor = 1.0f;

    if (stats.avgTimeInBracketSeconds > 0 && stats.stdDevSeconds > 0)
    {
        float cv = stats.stdDevSeconds / stats.avgTimeInBracketSeconds;  // Coefficient of variation
        consistencyFactor = std::max(0.0f, 1.0f - cv);
    }

    return sampleFactor * consistencyFactor;
}

uint8 BracketFlowPredictor::GetBracketIndex(uint32 level) const
{
    BotLevelDistribution* dist = BotLevelDistribution::instance();
    if (!dist)
        return 0;

    LevelBracket const* bracket = dist->GetBracketForLevel(level, TEAM_NEUTRAL);
    if (!bracket)
        return 0;

    return GetBracketIndex(bracket->tier);
}

uint8 BracketFlowPredictor::GetBracketIndex(ExpansionTier tier) const
{
    switch (tier)
    {
        case ExpansionTier::Starting:      return 0;
        case ExpansionTier::ChromieTime:   return 1;
        case ExpansionTier::Dragonflight:  return 2;
        case ExpansionTier::TheWarWithin:  return 3;
        default:                           return 0;
    }
}

void BracketFlowPredictor::CleanupOldRecords()
{
    if (!_config.persistToDatabase)
        return;

    // Delete old transition records
    sPlayerbotDatabase->Execute(Trinity::StringFormat(
        "DELETE FROM playerbot_bracket_transitions WHERE timestamp < DATE_SUB(NOW(), INTERVAL {} DAY)",
        _config.maxHistoryDays).c_str());

    TC_LOG_DEBUG("playerbot.lifecycle", "Cleaned up bracket transitions older than {} days",
        _config.maxHistoryDays);
}

} // namespace Playerbot
