/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "AdaptiveDifficulty.h"
#include "AI/BotAI.h"
#include "Player.h"
#include "Unit.h"
#include "Group.h"
#include "Log.h"
#include "Performance/BotPerformanceMonitor.h"
#include "GameTime.h"
#include "SpellHistory.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace Playerbot
{

// SkillIndicators Implementation
float SkillIndicators::GetOverallSkill() const
{
    // Weighted average of all indicators
    float weights[] = { 1.5f, 1.2f, 0.8f, 1.0f, 1.0f, 1.1f, 1.3f, 0.6f };
    float values[] = { accuracy, 1.0f / reactionTime, apm / 100.0f, survivalRate,
                      damageEfficiency, positioningQuality, decisionQuality, learningRate };

    float totalWeight = 0.0f;
    float weightedSum = 0.0f;

    for (int i = 0; i < 8; ++i)
    {
        weightedSum += values[i] * weights[i];
        totalWeight += weights[i];
    }

    return totalWeight > 0 ? weightedSum / totalWeight : 0.5f;
}

// PerformanceWindow Implementation
float PerformanceWindow::GetWinRate() const
{
    uint32_t total = playerWins + botWins + draws;
    return total > 0 ? static_cast<float>(playerWins) / total : 0.5f;
}

float PerformanceWindow::GetBalance() const
{
    uint32_t total = playerWins + botWins;
    if (total == 0)
        return 1.0f;  // Perfect balance when no games

    float winRate = GetWinRate();
    // Balance is highest at 50% win rate
    return 1.0f - ::std::abs(winRate - 0.5f) * 2.0f;
}

// DifficultyCurve Implementation
DifficultyCurve::DifficultyCurve(float initialDifficulty)
    : _slope(1.0f), _intercept(initialDifficulty - 0.5f), _fitted(false)
{
}

DifficultyCurve::~DifficultyCurve() = default;

void DifficultyCurve::AddDataPoint(float playerSkill, float optimalDifficulty)
{
    DataPoint point;
    point.skill = ::std::clamp(playerSkill, 0.0f, 1.0f);
    point.difficulty = ::std::clamp(optimalDifficulty, 0.0f, 1.0f);
    point.weight = 1.0f;

    _dataPoints.push_back(point);

    // Remove old points if exceeding max
    if (_dataPoints.size() > MAX_DATA_POINTS)
    {
        _dataPoints.erase(_dataPoints.begin());
    }

    // Refit curve with new data
    if (_dataPoints.size() >= MIN_POINTS_FOR_TRAINING)
    {
        FitCurve();
    }
}

float DifficultyCurve::GetDifficulty(float playerSkill) const
{
    playerSkill = ::std::clamp(playerSkill, 0.0f, 1.0f);

    if (!_fitted || _dataPoints.size() < MIN_POINTS_FOR_TRAINING)
    {
        // Default linear progression
        return ::std::clamp(playerSkill, 0.1f, 0.9f);
    }

    return Interpolate(playerSkill);
}

void DifficultyCurve::FitCurve()
{
    if (_dataPoints.size() < 2)
        return;

    // Linear regression using least squares
    float sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0, sumW = 0;

    for (const auto& point : _dataPoints)
    {
        float w = point.weight;
        sumX += point.skill * w;
        sumY += point.difficulty * w;
        sumXY += point.skill * point.difficulty * w;
        sumX2 += point.skill * point.skill * w;
        sumW += w;
    }

    if (sumW == 0)
        return;

    float meanX = sumX / sumW;
    float meanY = sumY / sumW;

    // Calculate slope and intercept
    float numerator = sumXY - sumX * sumY / sumW;
    float denominator = sumX2 - sumX * sumX / sumW;

    if (::std::abs(denominator) < 0.0001f)
    {
        _slope = 0.0f;
        _intercept = meanY;
    }
    else
    {
        _slope = numerator / denominator;
        _intercept = meanY - _slope * meanX;
    }

    // Constrain slope to reasonable values
    _slope = ::std::clamp(_slope, 0.5f, 2.0f);
    _fitted = true;
}

float DifficultyCurve::Interpolate(float skill) const
{
    float difficulty = _slope * skill + _intercept;
    return ::std::clamp(difficulty, 0.0f, 1.0f);
}

void DifficultyCurve::Smooth(float smoothingFactor)
{
    if (_dataPoints.size() < 2)
        return;

    smoothingFactor = ::std::clamp(smoothingFactor, 0.0f, 1.0f);

    // Apply exponential smoothing to data points
    for (size_t i = 1; i < _dataPoints.size(); ++i)
    {
        _dataPoints[i].difficulty = _dataPoints[i].difficulty * (1.0f - smoothingFactor) +
                                   _dataPoints[i-1].difficulty * smoothingFactor;
    }

    FitCurve();
}

// PlayerSkillProfile Implementation
PlayerSkillProfile::PlayerSkillProfile(ObjectGuid playerGuid)
    : _playerGuid(playerGuid)
    , _skillLevel(0.5f)
    , _frustrationLevel(0.0f)
    , _engagementLevel(0.5f)
    , _totalEngagements(0)
    , _consecutiveWins(0)
    , _consecutiveLosses(0)
{
    _lastUpdate = ::std::chrono::steady_clock::now();
    _profileCreated = _lastUpdate;
}

PlayerSkillProfile::~PlayerSkillProfile() = default;

void PlayerSkillProfile::UpdateSkillIndicators(const SkillIndicators& indicators)
{
    _currentSkill = indicators;
    _skillHistory.push_back(indicators);

    if (_skillHistory.size() > MAX_HISTORY_SIZE)
        _skillHistory.pop_front();

    UpdateSkillLevel();
    CalculateEngagement();
    CalculateFrustration();

    _lastUpdate = ::std::chrono::steady_clock::now();
}

float PlayerSkillProfile::GetSkillLevel() const
{
    return _skillLevel.load();
}

float PlayerSkillProfile::GetSkillTrend() const
{
    if (_skillHistory.size() < 5)
        return 0.0f;

    // Calculate trend using recent skill levels
    ::std::vector<float> recentSkills;
    for (const auto& indicators : _skillHistory)
        recentSkills.push_back(indicators.GetOverallSkill());

    // Simple linear regression for trend
    float sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
    size_t n = recentSkills.size();

    for (size_t i = 0; i < n; ++i)
    {
        sumX += i;
        sumY += recentSkills[i];
        sumXY += i * recentSkills[i];
        sumX2 += i * i;
    }

    float slope = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
    return slope;
}

void PlayerSkillProfile::RecordEngagement(bool playerWon, float duration)
{
    _totalEngagements++;

    if (playerWon)
    {
        _consecutiveWins++;
        _consecutiveLosses = 0;
    }
    else
    {
        _consecutiveLosses++;
        _consecutiveWins = 0;
    }

    // Update performance window
    auto now = ::std::chrono::steady_clock::now().time_since_epoch().count();

    if (_performanceHistory.empty() ||
        _performanceHistory.back().endTime + 300000000 < now)  // New window every 5 minutes
    {
        PerformanceWindow window;
        window.startTime = now;
        _performanceHistory.push_back(window);
    }

    auto& currentWindow = _performanceHistory.back();
    if (playerWon)
        currentWindow.playerWins++;
    else
        currentWindow.botWins++;

    currentWindow.averageEngagementDuration =
        (currentWindow.averageEngagementDuration * (currentWindow.playerWins + currentWindow.botWins - 1) + duration) /
        (currentWindow.playerWins + currentWindow.botWins);

    currentWindow.endTime = now;

    if (_performanceHistory.size() > MAX_HISTORY_SIZE)
        _performanceHistory.pop_front();

    CalculateFrustration();
    CalculateEngagement();
}

float PlayerSkillProfile::GetRecommendedDifficulty() const
{
    float skillLevel = GetSkillLevel();

    // Adjust based on frustration and engagement
    float frustrationAdjustment = -_frustrationLevel * 0.2f;
    float engagementBoost = (_engagementLevel - 0.5f) * 0.1f;

    float recommendedDifficulty = skillLevel + frustrationAdjustment + engagementBoost;

    // Consider win streaks
    if (_consecutiveWins > 3)
        recommendedDifficulty += 0.05f * (_consecutiveWins - 3);
    if (_consecutiveLosses > 3)
        recommendedDifficulty -= 0.05f * (_consecutiveLosses - 3);

    return ::std::clamp(recommendedDifficulty, 0.1f, 0.9f);
}

void PlayerSkillProfile::CalculateFrustration()
{
    float frustration = 0.0f;

    // Consecutive losses increase frustration
    if (_consecutiveLosses > 0)
    {
        frustration += ::std::min(1.0f, _consecutiveLosses / static_cast<float>(FRUSTRATION_THRESHOLD));
    }

    // Recent performance affects frustration
    if (!_performanceHistory.empty())
    {
        const auto& recent = _performanceHistory.back();
        float winRate = recent.GetWinRate();

        if (winRate < 0.3f)
            frustration += (0.3f - winRate) * 2.0f;
    }

    _frustrationLevel = ::std::clamp(frustration, 0.0f, 1.0f);
}

void PlayerSkillProfile::CalculateEngagement()
{
    float engagement = 0.5f;  // Neutral baseline

    // Balanced performance increases engagement
    if (!_performanceHistory.empty())
    {
        const auto& recent = _performanceHistory.back();
        float balance = recent.GetBalance();
        engagement += balance * 0.3f;
    }

    // Neither too many wins nor losses
    if (_consecutiveWins < BOREDOM_THRESHOLD && _consecutiveLosses < FRUSTRATION_THRESHOLD)
    {
        engagement += 0.2f;
    }

    // Skill improvement increases engagement
    float trend = GetSkillTrend();
    if (trend > 0)
        engagement += trend * 10.0f;  // Scale trend to meaningful range

    _engagementLevel = ::std::clamp(engagement, 0.0f, 1.0f);
}

void PlayerSkillProfile::UpdateSkillLevel()
{
    if (_skillHistory.empty())
        return;

    // Average of recent skill indicators
    float totalSkill = 0.0f;
    size_t count = ::std::min<size_t>(10, _skillHistory.size());

    for (size_t i = _skillHistory.size() - count; i < _skillHistory.size(); ++i)
    {
        totalSkill += _skillHistory[i].GetOverallSkill();
    }

    _skillLevel = totalSkill / count;
}

bool PlayerSkillProfile::NeedsDifficultyAdjustment() const
{
    // Check if adjustment is needed based on various factors
    if (_frustrationLevel > 0.7f || _engagementLevel < 0.3f)
        return true;

    if (_consecutiveWins > BOREDOM_THRESHOLD || _consecutiveLosses > FRUSTRATION_THRESHOLD)
        return true;

    // Check recent performance balance
    if (!_performanceHistory.empty())
    {
        const auto& recent = _performanceHistory.back();
        float winRate = recent.GetWinRate();

        if (winRate < 0.3f || winRate > 0.7f)
            return true;
    }

    return false;
}

// DifficultySettings Implementation
AdaptiveDifficulty::DifficultySettings::DifficultySettings()
    : reactionTimeMultiplier(1.0f)
    , accuracyModifier(0.0f)
    , damageModifier(1.0f)
    , healthModifier(1.0f)
    , aggressionLevel(0.5f)
    , cooperationLevel(0.5f)
    , adaptationSpeed(0.5f)
    , resourceEfficiency(0.5f)
    , positioningQuality(0.5f)
    , abilityUsageOptimization(0.5f)
{
}

void AdaptiveDifficulty::DifficultySettings::ApplyDifficulty(float difficulty)
{
    difficulty = ::std::clamp(difficulty, 0.0f, 1.0f);

    // Map difficulty to various parameters
    reactionTimeMultiplier = 2.0f - 1.5f * difficulty;  // 2.0 at diff 0, 0.5 at diff 1
    accuracyModifier = -0.3f + 0.6f * difficulty;       // -0.3 at diff 0, 0.3 at diff 1
    damageModifier = 0.7f + 0.6f * difficulty;          // 0.7 at diff 0, 1.3 at diff 1
    healthModifier = 0.8f + 0.4f * difficulty;          // 0.8 at diff 0, 1.2 at diff 1
    aggressionLevel = 0.3f + 0.5f * difficulty;         // 0.3 at diff 0, 0.8 at diff 1
    cooperationLevel = 0.2f + 0.6f * difficulty;        // 0.2 at diff 0, 0.8 at diff 1
    adaptationSpeed = 0.2f + 0.6f * difficulty;         // 0.2 at diff 0, 0.8 at diff 1
    resourceEfficiency = 0.3f + 0.5f * difficulty;      // 0.3 at diff 0, 0.8 at diff 1
    positioningQuality = 0.3f + 0.6f * difficulty;      // 0.3 at diff 0, 0.9 at diff 1
    abilityUsageOptimization = 0.3f + 0.6f * difficulty;// 0.3 at diff 0, 0.9 at diff 1
}

void AdaptiveDifficulty::DifficultySettings::ApplyAspect(DifficultyAspect aspect, float value)
{
    value = ::std::clamp(value, 0.0f, 1.0f);

    switch (aspect)
    {
        case DifficultyAspect::REACTION_TIME:
            reactionTimeMultiplier = 2.0f - 1.5f * value;
            break;
        case DifficultyAspect::ACCURACY:
            accuracyModifier = -0.3f + 0.6f * value;
            break;
        case DifficultyAspect::AGGRESSION:
            aggressionLevel = value;
            break;
        case DifficultyAspect::COOPERATION:
            cooperationLevel = value;
            break;
        case DifficultyAspect::RESOURCE_MGMT:
            resourceEfficiency = value;
            break;
        case DifficultyAspect::POSITIONING:
            positioningQuality = value;
            break;
        case DifficultyAspect::ADAPTATION_SPEED:
            adaptationSpeed = value;
            break;
        case DifficultyAspect::OVERALL:
            ApplyDifficulty(value);
            break;
    }
}

// AdaptiveDifficulty Implementation
AdaptiveDifficulty::AdaptiveDifficulty()
    : _initialized(false)
    , _enabled(true)
    , _difficultyMode("adaptive")
    , _adjustmentSpeed(0.1f)
    , _targetWinRate(0.5f)
{
}

AdaptiveDifficulty::~AdaptiveDifficulty()
{
    Shutdown();
}

bool AdaptiveDifficulty::Initialize()
{
    if (_initialized)
        return true;

    TC_LOG_INFO("playerbot.difficulty", "Initializing Adaptive Difficulty System");

    _initialized = true;
    TC_LOG_INFO("playerbot.difficulty", "Adaptive Difficulty System initialized successfully");
    return true;
}

void AdaptiveDifficulty::Shutdown()
{
    if (!_initialized)
        return;

    TC_LOG_INFO("playerbot.difficulty", "Shutting down Adaptive Difficulty System");

    ::std::lock_guard profileLock(_profilesMutex);
    ::std::lock_guard difficultyLock(_botDifficultyMutex);

    _playerProfiles.clear();
    _botDifficulties.clear();
    _difficultyCurves.clear();

    _initialized = false;
}

void AdaptiveDifficulty::CreatePlayerProfile(Player* player)
{
    if (!player || !_initialized)
        return;

    ObjectGuid guid = player->GetGUID();
    ::std::lock_guard lock(_profilesMutex);

    if (_playerProfiles.find(guid) == _playerProfiles.end())
    {
        auto profile = ::std::make_shared<PlayerSkillProfile>(guid);
        _playerProfiles[guid] = profile;
        _difficultyCurves[guid] = ::std::make_unique<DifficultyCurve>(DEFAULT_DIFFICULTY);
        _metrics.profilesTracked++;
    }
}

::std::shared_ptr<PlayerSkillProfile> AdaptiveDifficulty::GetPlayerProfile(ObjectGuid guid) const
{
    ::std::lock_guard lock(_profilesMutex);

    auto it = _playerProfiles.find(guid);
    if (it != _playerProfiles.end())
        return it->second;

    return nullptr;
}

void AdaptiveDifficulty::AssessPlayerSkill(Player* player)
{
    if (!player || !_initialized)
        return;

    auto profile = GetOrCreateProfile(player->GetGUID());
    if (!profile)
        return;

    SkillIndicators indicators = CalculateSkillIndicators(player);
    profile->UpdateSkillIndicators(indicators);
}

SkillIndicators AdaptiveDifficulty::CalculateSkillIndicators(Player* player) const
{
    SkillIndicators indicators;

    if (!player)
        return indicators;

    ObjectGuid playerGuid = player->GetGUID();

    // Get or access the tracker for this player
    ::std::lock_guard trackerLock(_trackerMutex);
    auto trackerIt = _playerTrackers.find(playerGuid);

    if (trackerIt != _playerTrackers.end())
    {
        const PlayerActionTracker& tracker = trackerIt->second;
        uint32 now = GameTime::GetGameTimeMS();

        // Calculate accuracy from tracked spell hits/misses
        if (tracker.spellsCastTotal > 0)
        {
            indicators.accuracy = static_cast<float>(tracker.spellsHitTotal) /
                                 static_cast<float>(tracker.spellsCastTotal);
        }
        else
        {
            // Use player's combat rating if no tracked data
            // Hit rating affects spell hit chance
            float hitRating = player->GetRatingBonusValue(CR_HIT_SPELL);
            float critRating = player->GetRatingBonusValue(CR_CRIT_SPELL);
            // Normalize hit rating: 0% = 0.5 base accuracy, +15% hit = ~0.85 accuracy
            indicators.accuracy = ::std::clamp(0.7f + (hitRating / 100.0f) + (critRating / 200.0f), 0.3f, 1.0f);
        }

        // Calculate APM from tracked actions
        if (tracker.trackingStartTime > 0 && now > tracker.trackingStartTime)
        {
            float minutesTracked = static_cast<float>(now - tracker.trackingStartTime) / 60000.0f;
            if (minutesTracked > 0.1f) // At least 6 seconds of tracking
            {
                indicators.apm = static_cast<float>(tracker.actionCount) / minutesTracked;
            }
            else
            {
                // Estimate from player haste - higher haste = faster actions
                float hasteRating = player->GetRatingBonusValue(CR_HASTE_SPELL);
                indicators.apm = 30.0f + (hasteRating / 5.0f); // Base 30 APM + haste bonus
            }
        }
        else
        {
            // Estimate APM from haste rating
            float hasteRating = player->GetRatingBonusValue(CR_HASTE_SPELL);
            indicators.apm = 30.0f + (hasteRating / 5.0f);
        }

        // Calculate reaction time from tracked samples
        if (tracker.reactionSamples > 0)
        {
            indicators.reactionTime = tracker.totalReactionTime / static_cast<float>(tracker.reactionSamples);
        }
        else
        {
            // Estimate reaction time inversely from haste
            // Base 500ms, reduced by haste (faster players = lower reaction time)
            float hasteRating = player->GetRatingBonusValue(CR_HASTE_SPELL);
            indicators.reactionTime = ::std::max(200.0f, 500.0f - hasteRating * 2.0f);
        }

        // Calculate damage efficiency from resource usage
        if (tracker.resourceUsed > 0)
        {
            float wasteRatio = static_cast<float>(tracker.resourceWasted) /
                              static_cast<float>(tracker.resourceUsed + tracker.resourceWasted);
            indicators.damageEfficiency = 1.0f - wasteRatio;
        }
        else
        {
            // Estimate from mastery - higher mastery = better resource management knowledge
            float masteryRating = player->GetRatingBonusValue(CR_MASTERY);
            indicators.damageEfficiency = ::std::clamp(0.5f + (masteryRating / 100.0f), 0.3f, 0.95f);
        }

        // Calculate positioning quality from tracking
        if (tracker.positionChecks > 0)
        {
            indicators.positioningQuality = static_cast<float>(tracker.goodPositionCount) /
                                           static_cast<float>(tracker.positionChecks);
        }
        else
        {
            // Estimate from movement speed and versatility
            // Players who use movement well tend to have good positioning
            float movementSpeed = player->GetSpeed(MOVE_RUN) / 7.0f; // Normalized to base speed
            float versatilityRating = player->GetRatingBonusValue(CR_VERSATILITY_DAMAGE_DONE);
            // Better speed utilization and versatility = better positioning
            indicators.positioningQuality = ::std::clamp(0.4f + (movementSpeed - 1.0f) * 0.3f +
                                                        versatilityRating / 200.0f, 0.2f, 0.95f);
        }

        // Calculate decision quality from tracked good/bad decisions
        if (tracker.totalDecisions > 0)
        {
            indicators.decisionQuality = static_cast<float>(tracker.goodDecisions) /
                                        static_cast<float>(tracker.totalDecisions);
        }
        else
        {
            // Estimate from overall player power - experienced players have better gear
            uint32 avgItemLevel = player->GetAverageItemLevel();
            // Normalize: ilvl 200 = 0.5, ilvl 400+ = 0.9 (rough scale for modern WoW)
            indicators.decisionQuality = ::std::clamp(0.3f + (static_cast<float>(avgItemLevel) - 100.0f) / 500.0f,
                                                      0.3f, 0.9f);
        }
    }
    else
    {
        // No tracker data - estimate all metrics from player stats
        // This happens for new players or when tracking hasn't started

        // Accuracy from hit/crit ratings
        float hitRating = player->GetRatingBonusValue(CR_HIT_SPELL);
        float critRating = player->GetRatingBonusValue(CR_CRIT_SPELL);
        indicators.accuracy = ::std::clamp(0.7f + (hitRating / 100.0f) + (critRating / 200.0f), 0.3f, 1.0f);

        // APM from haste rating
        float hasteRating = player->GetRatingBonusValue(CR_HASTE_SPELL);
        indicators.apm = 30.0f + (hasteRating / 5.0f);

        // Reaction time inversely from haste
        indicators.reactionTime = ::std::max(200.0f, 500.0f - hasteRating * 2.0f);

        // Damage efficiency from mastery
        float masteryRating = player->GetRatingBonusValue(CR_MASTERY);
        indicators.damageEfficiency = ::std::clamp(0.5f + (masteryRating / 100.0f), 0.3f, 0.95f);

        // Positioning from movement speed and versatility
        float movementSpeed = player->GetSpeed(MOVE_RUN) / 7.0f;
        float versatilityRating = player->GetRatingBonusValue(CR_VERSATILITY_DAMAGE_DONE);
        indicators.positioningQuality = ::std::clamp(0.4f + (movementSpeed - 1.0f) * 0.3f +
                                                    versatilityRating / 200.0f, 0.2f, 0.95f);

        // Decision quality from item level
        uint32 avgItemLevel = player->GetAverageItemLevel();
        indicators.decisionQuality = ::std::clamp(0.3f + (static_cast<float>(avgItemLevel) - 100.0f) / 500.0f,
                                                  0.3f, 0.9f);
    }

    // Calculate survival rate - this is always available from player state
    // Consider current health, defensive cooldowns active, and recent death count
    float healthPercent = player->GetHealthPct() / 100.0f;
    bool hasDefensiveBuff = false;

    // Check for common defensive auras
    if (player->HasAura(871) ||    // Shield Wall
        player->HasAura(12975) ||  // Last Stand
        player->HasAura(498) ||    // Divine Protection
        player->HasAura(642) ||    // Divine Shield
        player->HasAura(48792) ||  // Icebound Fortitude
        player->HasAura(61336) ||  // Survival Instincts
        player->HasAura(22812))    // Barkskin
    {
        hasDefensiveBuff = true;
    }

    // Base survival rate on health and defensive state
    if (player->IsAlive())
    {
        indicators.survivalRate = healthPercent * 0.6f + 0.3f;
        if (hasDefensiveBuff)
            indicators.survivalRate += 0.1f;
    }
    else
    {
        indicators.survivalRate = 0.1f;
    }
    indicators.survivalRate = ::std::clamp(indicators.survivalRate, 0.1f, 0.95f);

    // Calculate learning rate from profile history if available
    auto profile = GetPlayerProfile(playerGuid);
    if (profile)
    {
        indicators.learningRate = profile->GetSkillTrend();
    }
    else
    {
        indicators.learningRate = 0.0f;
    }

    return indicators;
}

void AdaptiveDifficulty::AdjustBotDifficulty(BotAI* bot, Player* opponent)
{
    if (!bot || !opponent || !_initialized)
        return;

    MEASURE_PERFORMANCE(MetricType::AI_DECISION_TIME, bot->GetBot()->GetGUID().GetCounter(), "DifficultyAdjustment");

    auto profile = GetOrCreateProfile(opponent->GetGUID());
    if (!profile)
        return;

    // Check if adjustment is needed
    if (!profile->NeedsDifficultyAdjustment())
        return;

    // Get recommended difficulty
    float targetDifficulty = profile->GetRecommendedDifficulty();

    // Apply adjustment with smoothing
    float currentDifficulty = GetBotDifficulty(bot);
    float newDifficulty = currentDifficulty + (targetDifficulty - currentDifficulty) * _adjustmentSpeed;

    SetBotDifficulty(bot, newDifficulty);
    _metrics.adjustmentsMade++;

    TC_LOG_DEBUG("playerbot.difficulty", "Adjusted bot %s difficulty from %.2f to %.2f for player %s",
        bot->GetBot()->GetName().c_str(), currentDifficulty, newDifficulty, opponent->GetName().c_str());
}

void AdaptiveDifficulty::SetBotDifficulty(BotAI* bot, float difficulty)
{
    if (!bot)
        return;

    difficulty = ::std::clamp(difficulty, MIN_DIFFICULTY, MAX_DIFFICULTY);

    ::std::lock_guard lock(_botDifficultyMutex);

    uint32_t botId = bot->GetBot()->GetGUID().GetCounter();
    DifficultySettings settings;
    settings.ApplyDifficulty(difficulty);

    _botDifficulties[botId] = settings;
}

float AdaptiveDifficulty::GetBotDifficulty(BotAI* bot) const
{
    if (!bot)
        return DEFAULT_DIFFICULTY;

    ::std::lock_guard lock(_botDifficultyMutex);

    uint32_t botId = bot->GetBot()->GetGUID().GetCounter();
    auto it = _botDifficulties.find(botId);

    if (it != _botDifficulties.end())
    {
        // Reverse calculate difficulty from settings
        const DifficultySettings& settings = it->second;
        return (settings.aggressionLevel + settings.cooperationLevel +
                settings.resourceEfficiency + settings.positioningQuality) / 4.0f;
    }

    return DEFAULT_DIFFICULTY;
}

void AdaptiveDifficulty::RecordCombatOutcome(Player* player, BotAI* bot, bool playerWon, float duration)
{
    if (!player || !bot || !_initialized)
        return;

    auto profile = GetOrCreateProfile(player->GetGUID());
    if (profile)
    {
        profile->RecordEngagement(playerWon, duration);

        // Update difficulty curve
        float playerSkill = profile->GetSkillLevel();
        float currentDifficulty = GetBotDifficulty(bot);

        // Optimal difficulty is where player had good engagement
    if (profile->GetEngagementLevel() > 0.7f)
        {
            TrainDifficultyCurve(player->GetGUID(), playerSkill, currentDifficulty);
        }
    }
}

void AdaptiveDifficulty::OptimizeForFlow(BotAI* bot, Player* player)
{
    if (!bot || !player || !_initialized)
        return;

    auto profile = GetOrCreateProfile(player->GetGUID());
    if (!profile)
        return;

    // Flow state requires balance between challenge and skill
    float playerSkill = profile->GetSkillLevel();
    float frustration = profile->GetFrustrationLevel();
    float engagement = profile->GetEngagementLevel();

    // Calculate optimal difficulty for flow
    float optimalDifficulty = playerSkill;

    // Adjust based on psychological state
    if (frustration > 0.5f)
        optimalDifficulty -= 0.1f;
    if (engagement < 0.5f)
        optimalDifficulty += 0.05f;

    // Ensure challenge is neither too easy nor too hard
    optimalDifficulty = ::std::clamp(optimalDifficulty, playerSkill - 0.15f, playerSkill + 0.15f);
    SetBotDifficulty(bot, optimalDifficulty);

    // Record flow state achievement
    if (IsInFlowState(player))
    {
        _metrics.flowStatesAchieved++;
    }
}

bool AdaptiveDifficulty::IsInFlowState(Player* player) const
{
    if (!player)
        return false;

    auto profile = GetPlayerProfile(player->GetGUID());
    if (!profile)
        return false;

    float engagement = profile->GetEngagementLevel();
    float frustration = profile->GetFrustrationLevel();

    // Flow state indicators
    bool highEngagement = engagement > FLOW_STATE_THRESHOLD;
    bool lowFrustration = frustration < 0.3f;
    bool balancedPerformance = false;

    auto perfWindow = profile->GetRecentPerformance();
    if (perfWindow.playerWins + perfWindow.botWins > 0)
    {
        float balance = perfWindow.GetBalance();
        balancedPerformance = balance > 0.7f;
    }

    return highEngagement && lowFrustration && balancedPerformance;
}

float AdaptiveDifficulty::GetFlowStateScore(Player* player) const
{
    if (!player)
        return 0.0f;

    auto profile = GetPlayerProfile(player->GetGUID());
    if (!profile)
        return 0.0f;

    float engagement = profile->GetEngagementLevel();
    float frustration = 1.0f - profile->GetFrustrationLevel();

    auto perfWindow = profile->GetRecentPerformance();
    float balance = perfWindow.GetBalance();

    // Weighted average of flow indicators
    return (engagement * 0.4f + frustration * 0.3f + balance * 0.3f);
}

void AdaptiveDifficulty::TrainDifficultyCurve(ObjectGuid playerGuid, float skill, float optimalDifficulty)
{
    auto it = _difficultyCurves.find(playerGuid);
    if (it != _difficultyCurves.end())
    {
        it->second->AddDataPoint(skill, optimalDifficulty);
    }
}

float AdaptiveDifficulty::GetOptimalDifficulty(ObjectGuid playerGuid, float currentSkill) const
{
    auto it = _difficultyCurves.find(playerGuid);
    if (it != _difficultyCurves.end())
    {
        return it->second->GetDifficulty(currentSkill);
    }

    return currentSkill;  // Default to matching skill level
}

::std::shared_ptr<PlayerSkillProfile> AdaptiveDifficulty::GetOrCreateProfile(ObjectGuid guid)
{
    auto profile = GetPlayerProfile(guid);
    if (!profile)
    {
        // Create profile if it doesn't exist
        ::std::lock_guard lock(_profilesMutex);
        profile = ::std::make_shared<PlayerSkillProfile>(guid);
        _playerProfiles[guid] = profile;
        _difficultyCurves[guid] = ::std::make_unique<DifficultyCurve>(DEFAULT_DIFFICULTY);
        _metrics.profilesTracked++;
    }
    return profile;
}

void AdaptiveDifficulty::ApplyPreset(BotAI* bot, DifficultyPreset preset)
{
    if (!bot)
        return;

    DifficultySettings settings = GetPresetSettings(preset);

    ::std::lock_guard lock(_botDifficultyMutex);
    uint32_t botId = bot->GetBot()->GetGUID().GetCounter();
    _botDifficulties[botId] = settings;
}

AdaptiveDifficulty::DifficultySettings AdaptiveDifficulty::GetPresetSettings(DifficultyPreset preset) const
{
    DifficultySettings settings;

    switch (preset)
    {
        case DifficultyPreset::BEGINNER:
            settings.ApplyDifficulty(0.2f);
            break;
        case DifficultyPreset::EASY:
            settings.ApplyDifficulty(0.35f);
            break;
        case DifficultyPreset::NORMAL:
            settings.ApplyDifficulty(0.5f);
            break;
        case DifficultyPreset::HARD:
            settings.ApplyDifficulty(0.7f);
            break;
        case DifficultyPreset::EXPERT:
            settings.ApplyDifficulty(0.9f);
            break;
        case DifficultyPreset::ADAPTIVE:
            // Adaptive starts at normal and adjusts
            settings.ApplyDifficulty(0.5f);
            break;
    }

    return settings;
}

// ScopedDifficultyAdjustment Implementation
ScopedDifficultyAdjustment::ScopedDifficultyAdjustment(BotAI* bot, Player* player)
    : _bot(bot)
    , _player(player)
    , _playerSuccesses(0)
    , _botSuccesses(0)
    , _totalReactionTime(0)
    , _reactionSamples(0)
{
    _startTime = ::std::chrono::steady_clock::now();
    _initialDifficulty = sAdaptiveDifficulty.GetBotDifficulty(bot);
}

ScopedDifficultyAdjustment::~ScopedDifficultyAdjustment()
{
    if (_bot && _player)
    {
        auto endTime = ::std::chrono::steady_clock::now();
        float duration = ::std::chrono::duration<float>(endTime - _startTime).count();

        // Update player skill indicators
        SkillIndicators indicators = sAdaptiveDifficulty.CalculateSkillIndicators(_player);
        if (_reactionSamples > 0)
            indicators.reactionTime = _totalReactionTime / _reactionSamples;
        if (_playerSuccesses + _botSuccesses > 0)
            indicators.accuracy = static_cast<float>(_playerSuccesses) / (_playerSuccesses + _botSuccesses);

        sAdaptiveDifficulty.UpdatePlayerSkill(_player, indicators);

        // Adjust difficulty if needed
        sAdaptiveDifficulty.AdjustBotDifficulty(_bot, _player);
    }
}

void ScopedDifficultyAdjustment::RecordPlayerAction(bool successful, float reactionTime)
{
    if (successful)
        _playerSuccesses++;

    _totalReactionTime += reactionTime;
    _reactionSamples++;
}

void ScopedDifficultyAdjustment::RecordBotAction(bool successful)
{
    if (successful)
        _botSuccesses++;
}

void ScopedDifficultyAdjustment::MarkCombatEnd(bool playerWon)
{
    if (_bot && _player)
    {
        auto endTime = ::std::chrono::steady_clock::now();
        float duration = ::std::chrono::duration<float>(endTime - _startTime).count();

        sAdaptiveDifficulty.RecordCombatOutcome(_player, _bot, playerWon, duration);
    }
}

} // namespace Playerbot