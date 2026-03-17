/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * ST-1: Adaptive AI Update Throttling System - Implementation
 */

#include "AdaptiveAIUpdateThrottler.h"
#include "BotAI.h"
#include "Player.h"
#include "WorldSession.h"
#include "Group.h"
#include "Map.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "ObjectAccessor.h"
#include "Log.h"
#include "GameTime.h"

namespace Playerbot
{

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

AdaptiveAIUpdateThrottler::AdaptiveAIUpdateThrottler(Player* bot, BotAI* ai)
    : _bot(bot)
    , _ai(ai)
    , _config(ThrottleConfig::Default())
    , _currentTier(ThrottleTier::FULL_RATE)
    , _effectiveInterval(_config.baseUpdateInterval)
    , _nearestHumanDistance(std::numeric_limits<float>::max())
    , _nearestHumanGuid()
    , _currentActivity(AIActivityType::IDLE)
    , _inCombat(false)
    , _timeSinceLastUpdate(0)
    , _timeSinceProximityCheck(0)
    , _timeSinceActivityCheck(0)
    , _forceNextUpdate(false)
    , _enabled(true)
{
    _metrics.Reset();
}

// ============================================================================
// MAIN THROTTLE INTERFACE
// ============================================================================

bool AdaptiveAIUpdateThrottler::ShouldUpdate(uint32 diff)
{
    // Throttling disabled - always update
    if (!_enabled)
    {
        _metrics.totalUpdatesProcessed++;
        GlobalThrottleStatistics::Instance().RecordUpdateProcessed();
        return true;
    }

    // Validate bot pointer
    if (!_bot || !_bot->IsInWorld())
    {
        return false;
    }

    // Accumulate time
    _timeSinceLastUpdate += diff;
    _timeSinceProximityCheck += diff;
    _timeSinceActivityCheck += diff;

    // Force update flag takes priority
    if (_forceNextUpdate)
    {
        _forceNextUpdate = false;
        _timeSinceLastUpdate = 0;
        _metrics.totalUpdatesProcessed++;
        GlobalThrottleStatistics::Instance().RecordUpdateProcessed();
        return true;
    }

    // Periodic proximity check (expensive, don't do every frame)
    if (_timeSinceProximityCheck >= _config.proximityCheckInterval)
    {
        UpdateNearestHumanDistance();
        _timeSinceProximityCheck = 0;
    }

    // Periodic activity classification
    if (_timeSinceActivityCheck >= _config.activityCheckInterval)
    {
        _currentActivity = ClassifyActivity();
        _timeSinceActivityCheck = 0;
    }

    // Calculate current tier and effective interval
    _currentTier = CalculateThrottleTier();
    _effectiveInterval = CalculateEffectiveInterval(_currentTier);

    // Update metrics
    _metrics.currentThrottleTier = static_cast<uint32>(_currentTier);
    _metrics.nearestHumanDistance = _nearestHumanDistance;
    _metrics.inCombat = _inCombat;
    _metrics.currentActivity = _currentActivity;

    // Check if enough time has passed since last update
    if (_timeSinceLastUpdate >= _effectiveInterval)
    {
        _timeSinceLastUpdate = 0;
        _metrics.totalUpdatesProcessed++;
        GlobalThrottleStatistics::Instance().RecordUpdateProcessed();

        // Update average interval (exponential moving average)
        _metrics.averageUpdateInterval = _metrics.averageUpdateInterval * 0.9f +
                                         static_cast<float>(_effectiveInterval) * 0.1f;

        TC_LOG_TRACE("module.playerbot.throttle",
            "Bot {} UPDATE: tier={}, interval={}ms, humanDist={:.1f}m, activity={}",
            _bot->GetName(),
            static_cast<uint32>(_currentTier),
            _effectiveInterval,
            _nearestHumanDistance,
            static_cast<uint32>(_currentActivity));

        return true;
    }

    // Update skipped
    _metrics.totalUpdatesSkipped++;
    GlobalThrottleStatistics::Instance().RecordUpdateSkipped();
    return false;
}

void AdaptiveAIUpdateThrottler::ForceNextUpdate()
{
    _forceNextUpdate = true;
}

// ============================================================================
// STATE NOTIFICATIONS
// ============================================================================

void AdaptiveAIUpdateThrottler::OnCombatStart()
{
    _inCombat = true;
    _currentTier = ThrottleTier::FULL_RATE;
    _effectiveInterval = _config.baseUpdateInterval;
    ForceNextUpdate();

    TC_LOG_DEBUG("module.playerbot.throttle",
        "Bot {} entered combat - switching to FULL_RATE",
        _bot ? _bot->GetName() : "unknown");
}

void AdaptiveAIUpdateThrottler::OnCombatEnd()
{
    _inCombat = false;
    // Tier will be recalculated on next proximity check

    TC_LOG_DEBUG("module.playerbot.throttle",
        "Bot {} left combat - will reassess throttle tier",
        _bot ? _bot->GetName() : "unknown");
}

void AdaptiveAIUpdateThrottler::OnHumanNearby(ObjectGuid humanGuid, float distance)
{
    if (distance < _nearestHumanDistance)
    {
        _nearestHumanDistance = distance;
        _nearestHumanGuid = humanGuid;

        // If human is very close, ensure we're at full rate
        if (distance < _config.nearHumanDistance)
        {
            _currentTier = ThrottleTier::FULL_RATE;
            _effectiveInterval = _config.baseUpdateInterval;
        }
    }
}

void AdaptiveAIUpdateThrottler::OnActivityChange(AIActivityType newActivity)
{
    if (_currentActivity != newActivity)
    {
        AIActivityType oldActivity = _currentActivity;
        _currentActivity = newActivity;

        TC_LOG_DEBUG("module.playerbot.throttle",
            "Bot {} activity changed: {} -> {}",
            _bot ? _bot->GetName() : "unknown",
            static_cast<uint32>(oldActivity),
            static_cast<uint32>(newActivity));

        // Critical activities force update
        if (newActivity == AIActivityType::COMBAT)
        {
            ForceNextUpdate();
        }
    }
}

// ============================================================================
// INTERNAL CALCULATIONS
// ============================================================================

void AdaptiveAIUpdateThrottler::UpdateNearestHumanDistance()
{
    if (!_bot || !_bot->IsInWorld())
    {
        _nearestHumanDistance = std::numeric_limits<float>::max();
        _nearestHumanGuid = ObjectGuid::Empty;
        return;
    }

    // Search for nearby human players
    float searchRadius = _config.outOfRangeDistance;
    float nearestDistance = std::numeric_limits<float>::max();
    ObjectGuid nearestGuid = ObjectGuid::Empty;

    // Use grid visitor to find nearby players
    ::std::list<Player*> nearbyPlayers;
    Trinity::AnyPlayerInPositionRangeCheck check(_bot, searchRadius, true);
    Trinity::PlayerListSearcher<Trinity::AnyPlayerInPositionRangeCheck> searcher(_bot, nearbyPlayers, check);
    Cell::VisitWorldObjects(_bot, searcher, searchRadius);

    // Find nearest human (non-bot) player
    for (Player* player : nearbyPlayers)
    {
        if (!player || player == _bot)
            continue;

        // Skip if this is a bot
        WorldSession* session = player->GetSession();
        if (session && session->IsBot())
            continue;

        float distance = _bot->GetDistance(player);
        if (distance < nearestDistance)
        {
            nearestDistance = distance;
            nearestGuid = player->GetGUID();
        }
    }

    _nearestHumanDistance = nearestDistance;
    _nearestHumanGuid = nearestGuid;

    TC_LOG_TRACE("module.playerbot.throttle",
        "Bot {} proximity check: nearestHuman={:.1f}m (found {} players in range)",
        _bot->GetName(),
        nearestDistance,
        nearbyPlayers.size());
}

ThrottleTier AdaptiveAIUpdateThrottler::CalculateThrottleTier() const
{
    // Priority 1: Combat always gets full rate
    if (_inCombat)
    {
        return ThrottleTier::FULL_RATE;
    }

    // Priority 2: In group with human leader gets full rate
    if (IsInGroupWithHumanLeader())
    {
        return ThrottleTier::FULL_RATE;
    }

    // Priority 3: Activity-based boost
    // Combat activity is already handled above
    // Questing and grinding need higher responsiveness
    if (_currentActivity == AIActivityType::QUESTING ||
        _currentActivity == AIActivityType::GRINDING ||
        _currentActivity == AIActivityType::GATHERING)
    {
        // But still respect distance
        if (_nearestHumanDistance < _config.nearHumanDistance)
            return ThrottleTier::FULL_RATE;
        else if (_nearestHumanDistance < _config.midRangeDistance)
            return ThrottleTier::HIGH_RATE;
        else
            return ThrottleTier::MEDIUM_RATE;
    }

    // Priority 4: Distance-based tier calculation
    if (_nearestHumanDistance < _config.nearHumanDistance)
    {
        return ThrottleTier::FULL_RATE;
    }
    else if (_nearestHumanDistance < _config.midRangeDistance)
    {
        return ThrottleTier::HIGH_RATE;
    }
    else if (_nearestHumanDistance < _config.farDistance)
    {
        return ThrottleTier::MEDIUM_RATE;
    }
    else if (_nearestHumanDistance < _config.outOfRangeDistance)
    {
        return ThrottleTier::LOW_RATE;
    }

    // Very far or idle activity
    if (_currentActivity == AIActivityType::IDLE ||
        _currentActivity == AIActivityType::RESTING)
    {
        return ThrottleTier::MINIMAL_RATE;
    }

    return ThrottleTier::LOW_RATE;
}

uint32 AdaptiveAIUpdateThrottler::CalculateEffectiveInterval(ThrottleTier tier) const
{
    float multiplier = 1.0f;

    switch (tier)
    {
        case ThrottleTier::FULL_RATE:
            multiplier = _config.fullRateMultiplier;
            break;
        case ThrottleTier::HIGH_RATE:
            multiplier = _config.highRateMultiplier;
            break;
        case ThrottleTier::MEDIUM_RATE:
            multiplier = _config.mediumRateMultiplier;
            break;
        case ThrottleTier::LOW_RATE:
            multiplier = _config.lowRateMultiplier;
            break;
        case ThrottleTier::MINIMAL_RATE:
            multiplier = _config.minimalRateMultiplier;
            break;
    }

    // Inverse multiplier: lower multiplier = longer interval
    // e.g., 0.25 multiplier = 4x longer interval (25% update rate)
    uint32 interval = static_cast<uint32>(_config.baseUpdateInterval / multiplier);

    // Clamp to reasonable range (100ms to 5000ms)
    return ::std::clamp(interval, 100u, 5000u);
}

AIActivityType AdaptiveAIUpdateThrottler::ClassifyActivity() const
{
    if (!_bot || !_ai)
        return AIActivityType::IDLE;

    // Check combat first
    if (_bot->IsInCombat() || _inCombat)
    {
        return AIActivityType::COMBAT;
    }

    // Check if questing (has active objectives)
    if (_ai->IsQuestingActive())
    {
        return AIActivityType::QUESTING;
    }

    // Check if in a group and following
    if (_bot->GetGroup())
    {
        // If group has human leader or we're in an instance, treat as following
        if (IsInGroupWithHumanLeader())
        {
            return AIActivityType::FOLLOWING;
        }
    }

    // Check movement state - if moving toward a destination, we're traveling
    if (_bot->isMoving())
    {
        return AIActivityType::TRAVELING;
    }

    // Check if resting (eating/drinking)
    if (_bot->HasAura(433) ||  // Food aura example
        _bot->HasAura(430))    // Drink aura example
    {
        return AIActivityType::RESTING;
    }

    // Default to idle
    return AIActivityType::IDLE;
}

bool AdaptiveAIUpdateThrottler::IsInGroupWithHumanLeader() const
{
    if (!_bot)
        return false;

    Group* group = _bot->GetGroup();
    if (!group)
        return false;

    ObjectGuid leaderGuid = group->GetLeaderGUID();
    if (leaderGuid.IsEmpty())
        return false;

    // Check if leader is a human player
    Player* leader = ObjectAccessor::FindPlayer(leaderGuid);
    if (!leader)
        return false;

    WorldSession* session = leader->GetSession();
    if (!session)
        return false;

    // Human leader = session is NOT a bot
    return !session->IsBot();
}

} // namespace Playerbot
