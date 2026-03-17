/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "SessionTransitions.h"
#include "Player.h"
#include "Map.h"
#include "Battleground.h"
#include "InstanceScript.h"
#include "Log.h"
#include <algorithm>
#include <random>

namespace Playerbot
{
namespace Humanization
{

// Static member initialization
SessionTransitions::TransitionMetrics SessionTransitions::_globalMetrics;

SessionTransitions::SessionTransitions(Player* bot)
    : _bot(bot)
    , _botGuid(bot ? bot->GetGUID() : ObjectGuid::Empty)
    , _lastTransitionTime(std::chrono::steady_clock::now())
{
}

SessionTransitions::~SessionTransitions()
{
    Shutdown();
}

void SessionTransitions::Initialize()
{
    if (_initialized)
        return;

    InitializeDefaultRules();
    InitializeFlowPatterns();

    _initialized = true;

    TC_LOG_DEBUG("playerbots.humanization", "SessionTransitions initialized for bot {}",
        _botGuid.GetCounter());
}

void SessionTransitions::InitializeDefaultRules()
{
    // ========================================================================
    // QUESTING TRANSITIONS
    // ========================================================================

    // Questing -> Break (after long session)
    {
        TransitionRule rule;
        rule.fromActivity = ActivityType::QUEST_OBJECTIVE;
        rule.toActivity = ActivityType::AFK_SHORT;
        rule.priority = 80;
        rule.minWrapUpMs = 2000;
        rule.maxWrapUpMs = 8000;
        rule.prepTimeMs = 1000;
        rule.requiresTravel = false;
        rule.transitionName = "Quest Break";
        SetRule(rule);
    }

    // Questing -> Gathering (opportunistic)
    {
        TransitionRule rule;
        rule.fromActivity = ActivityType::QUEST_OBJECTIVE;
        rule.toActivity = ActivityType::MINING;
        rule.priority = 60;
        rule.minWrapUpMs = 1000;
        rule.maxWrapUpMs = 3000;
        rule.prepTimeMs = 500;
        rule.requiresTravel = false;
        rule.transitionName = "Gather Resources";
        SetRule(rule);
    }

    // Questing -> City Visit
    {
        TransitionRule rule;
        rule.fromActivity = ActivityType::QUEST_OBJECTIVE;
        rule.toActivity = ActivityType::CITY_WANDERING;
        rule.priority = 50;
        rule.minWrapUpMs = 3000;
        rule.maxWrapUpMs = 10000;
        rule.prepTimeMs = 2000;
        rule.requiresTravel = true;
        rule.transitionName = "Return to City";
        SetRule(rule);
    }

    // ========================================================================
    // GATHERING TRANSITIONS
    // ========================================================================

    // Gathering -> Questing
    {
        TransitionRule rule;
        rule.fromActivity = ActivityType::MINING;
        rule.toActivity = ActivityType::QUEST_OBJECTIVE;
        rule.priority = 70;
        rule.minWrapUpMs = 1000;
        rule.maxWrapUpMs = 5000;
        rule.prepTimeMs = 1000;
        rule.requiresTravel = false;
        rule.transitionName = "Resume Questing";
        SetRule(rule);
    }

    // Gathering -> Auction House
    {
        TransitionRule rule;
        rule.fromActivity = ActivityType::MINING;
        rule.toActivity = ActivityType::AUCTION_BROWSING;
        rule.priority = 60;
        rule.minWrapUpMs = 2000;
        rule.maxWrapUpMs = 5000;
        rule.prepTimeMs = 3000;
        rule.requiresTravel = true;
        rule.transitionName = "Sell at Auction";
        SetRule(rule);
    }

    // ========================================================================
    // CITY TRANSITIONS
    // ========================================================================

    // City Visit -> Auction House
    {
        TransitionRule rule;
        rule.fromActivity = ActivityType::CITY_WANDERING;
        rule.toActivity = ActivityType::AUCTION_BROWSING;
        rule.priority = 70;
        rule.minWrapUpMs = 500;
        rule.maxWrapUpMs = 2000;
        rule.prepTimeMs = 1000;
        rule.requiresTravel = false;
        rule.transitionName = "Check Auctions";
        SetRule(rule);
    }

    // City Visit -> Bank Visit
    {
        TransitionRule rule;
        rule.fromActivity = ActivityType::CITY_WANDERING;
        rule.toActivity = ActivityType::BANK_VISIT;
        rule.priority = 65;
        rule.minWrapUpMs = 500;
        rule.maxWrapUpMs = 2000;
        rule.prepTimeMs = 1000;
        rule.requiresTravel = false;
        rule.transitionName = "Visit Bank";
        SetRule(rule);
    }

    // City Visit -> Mail Check
    {
        TransitionRule rule;
        rule.fromActivity = ActivityType::CITY_WANDERING;
        rule.toActivity = ActivityType::MAILBOX_CHECK;
        rule.priority = 60;
        rule.minWrapUpMs = 500;
        rule.maxWrapUpMs = 1500;
        rule.prepTimeMs = 500;
        rule.requiresTravel = false;
        rule.transitionName = "Check Mail";
        SetRule(rule);
    }

    // City Visit -> Trainer Visit
    {
        TransitionRule rule;
        rule.fromActivity = ActivityType::CITY_WANDERING;
        rule.toActivity = ActivityType::TRAINER_VISIT;
        rule.priority = 55;
        rule.minWrapUpMs = 500;
        rule.maxWrapUpMs = 2000;
        rule.prepTimeMs = 1000;
        rule.requiresTravel = false;
        rule.transitionName = "Visit Trainer";
        SetRule(rule);
    }

    // City Visit -> Inn Rest
    {
        TransitionRule rule;
        rule.fromActivity = ActivityType::CITY_WANDERING;
        rule.toActivity = ActivityType::INN_REST;
        rule.priority = 40;
        rule.minWrapUpMs = 1000;
        rule.maxWrapUpMs = 3000;
        rule.prepTimeMs = 2000;
        rule.requiresTravel = false;
        rule.transitionName = "Rest at Inn";
        SetRule(rule);
    }

    // ========================================================================
    // BREAK TRANSITIONS
    // ========================================================================

    // Break -> Questing
    {
        TransitionRule rule;
        rule.fromActivity = ActivityType::AFK_SHORT;
        rule.toActivity = ActivityType::QUEST_OBJECTIVE;
        rule.priority = 75;
        rule.minWrapUpMs = 2000;
        rule.maxWrapUpMs = 10000;
        rule.prepTimeMs = 3000;
        rule.requiresTravel = false;
        rule.transitionName = "Back to Questing";
        SetRule(rule);
    }

    // Break -> Gathering
    {
        TransitionRule rule;
        rule.fromActivity = ActivityType::AFK_SHORT;
        rule.toActivity = ActivityType::MINING;
        rule.priority = 60;
        rule.minWrapUpMs = 2000;
        rule.maxWrapUpMs = 8000;
        rule.prepTimeMs = 2000;
        rule.requiresTravel = false;
        rule.transitionName = "Start Gathering";
        SetRule(rule);
    }

    // ========================================================================
    // DUNGEON TRANSITIONS
    // ========================================================================

    // Dungeon -> Break (always after dungeon)
    {
        TransitionRule rule;
        rule.fromActivity = ActivityType::DUNGEON_RUN;
        rule.toActivity = ActivityType::AFK_SHORT;
        rule.priority = 90;
        rule.minWrapUpMs = 5000;
        rule.maxWrapUpMs = 15000;
        rule.prepTimeMs = 1000;
        rule.requiresTravel = true;
        rule.transitionName = "Post-Dungeon Break";
        SetRule(rule);
    }

    // Dungeon -> City Visit
    {
        TransitionRule rule;
        rule.fromActivity = ActivityType::DUNGEON_RUN;
        rule.toActivity = ActivityType::CITY_WANDERING;
        rule.priority = 85;
        rule.minWrapUpMs = 5000;
        rule.maxWrapUpMs = 15000;
        rule.prepTimeMs = 3000;
        rule.requiresTravel = true;
        rule.transitionName = "Return from Dungeon";
        SetRule(rule);
    }

    // ========================================================================
    // FISHING TRANSITIONS
    // ========================================================================

    // Fishing -> Questing
    {
        TransitionRule rule;
        rule.fromActivity = ActivityType::FISHING;
        rule.toActivity = ActivityType::QUEST_OBJECTIVE;
        rule.priority = 50;
        rule.minWrapUpMs = 3000;
        rule.maxWrapUpMs = 10000;
        rule.prepTimeMs = 2000;
        rule.requiresTravel = false;
        rule.transitionName = "Done Fishing";
        SetRule(rule);
    }

    // Fishing -> Cooking (natural follow-up)
    {
        TransitionRule rule;
        rule.fromActivity = ActivityType::FISHING;
        rule.toActivity = ActivityType::CRAFTING_SESSION;
        rule.priority = 70;
        rule.minWrapUpMs = 1000;
        rule.maxWrapUpMs = 3000;
        rule.prepTimeMs = 2000;
        rule.requiresTravel = false;
        rule.transitionName = "Cook the Catch";
        SetRule(rule);
    }

    // ========================================================================
    // PROFESSION TRANSITIONS
    // ========================================================================

    // Crafting -> Auction House
    {
        TransitionRule rule;
        rule.fromActivity = ActivityType::CRAFTING_SESSION;
        rule.toActivity = ActivityType::AUCTION_BROWSING;
        rule.priority = 65;
        rule.minWrapUpMs = 2000;
        rule.maxWrapUpMs = 5000;
        rule.prepTimeMs = 2000;
        rule.requiresTravel = true;
        rule.transitionName = "Sell Crafts";
        SetRule(rule);
    }

    // Cooking -> Break (food coma!)
    {
        TransitionRule rule;
        rule.fromActivity = ActivityType::CRAFTING_SESSION;
        rule.toActivity = ActivityType::AFK_SHORT;
        rule.priority = 55;
        rule.minWrapUpMs = 1000;
        rule.maxWrapUpMs = 3000;
        rule.prepTimeMs = 1000;
        rule.requiresTravel = false;
        rule.transitionName = "Cooking Break";
        SetRule(rule);
    }

    TC_LOG_DEBUG("playerbots.humanization", "Initialized {} transition rules",
        _rules.size());
}

void SessionTransitions::InitializeFlowPatterns()
{
    // ========================================================================
    // CASUAL PLAYER PATTERNS
    // ========================================================================

    {
        TransitionFlowPattern pattern;
        pattern.patternName = "Casual Quester";
        pattern.sequence = {
            ActivityType::QUEST_OBJECTIVE,
            ActivityType::AFK_SHORT,
            ActivityType::CITY_WANDERING,
            ActivityType::QUEST_OBJECTIVE
        };
        pattern.weight = 80;
        pattern.preferredBy = PersonalityType::CASUAL;
        _flowPatterns.push_back(pattern);
    }

    {
        TransitionFlowPattern pattern;
        pattern.patternName = "Relaxed Gatherer";
        pattern.sequence = {
            ActivityType::MINING,
            ActivityType::FISHING,
            ActivityType::AFK_SHORT,
            ActivityType::CITY_WANDERING
        };
        pattern.weight = 70;
        pattern.preferredBy = PersonalityType::CASUAL;
        _flowPatterns.push_back(pattern);
    }

    // ========================================================================
    // EFFICIENT PLAYER PATTERNS
    // ========================================================================

    {
        TransitionFlowPattern pattern;
        pattern.patternName = "Efficient Grinder";
        pattern.sequence = {
            ActivityType::QUEST_OBJECTIVE,
            ActivityType::MINING,
            ActivityType::QUEST_OBJECTIVE,
            ActivityType::CITY_WANDERING
        };
        pattern.weight = 85;
        pattern.preferredBy = PersonalityType::HARDCORE;
        _flowPatterns.push_back(pattern);
    }

    {
        TransitionFlowPattern pattern;
        pattern.patternName = "Quick City Run";
        pattern.sequence = {
            ActivityType::CITY_WANDERING,
            ActivityType::AUCTION_BROWSING,
            ActivityType::BANK_VISIT,
            ActivityType::MAILBOX_CHECK,
            ActivityType::QUEST_OBJECTIVE
        };
        pattern.weight = 90;
        pattern.preferredBy = PersonalityType::HARDCORE;
        _flowPatterns.push_back(pattern);
    }

    // ========================================================================
    // EXPLORER PATTERNS
    // ========================================================================

    {
        TransitionFlowPattern pattern;
        pattern.patternName = "World Explorer";
        pattern.sequence = {
            ActivityType::ZONE_EXPLORATION,
            ActivityType::MINING,
            ActivityType::ZONE_EXPLORATION,
            ActivityType::AFK_SHORT
        };
        pattern.weight = 75;
        pattern.preferredBy = PersonalityType::EXPLORER;
        _flowPatterns.push_back(pattern);
    }

    // ========================================================================
    // SOCIAL PATTERNS
    // ========================================================================

    {
        TransitionFlowPattern pattern;
        pattern.patternName = "City Socializer";
        pattern.sequence = {
            ActivityType::CITY_WANDERING,
            ActivityType::CHATTING,
            ActivityType::AUCTION_BROWSING,
            ActivityType::INN_REST
        };
        pattern.weight = 80;
        pattern.preferredBy = PersonalityType::SOCIAL;
        _flowPatterns.push_back(pattern);
    }

    // ========================================================================
    // COMPLETIONIST PATTERNS
    // ========================================================================

    {
        TransitionFlowPattern pattern;
        pattern.patternName = "Achievement Hunter";
        pattern.sequence = {
            ActivityType::QUEST_OBJECTIVE,
            ActivityType::DUNGEON_RUN,
            ActivityType::AFK_SHORT,
            ActivityType::QUEST_OBJECTIVE
        };
        pattern.weight = 85;
        pattern.preferredBy = PersonalityType::COMPLETIONIST;
        _flowPatterns.push_back(pattern);
    }

    {
        TransitionFlowPattern pattern;
        pattern.patternName = "Profession Master";
        pattern.sequence = {
            ActivityType::MINING,
            ActivityType::CRAFTING_SESSION,
            ActivityType::TRAINER_VISIT,
            ActivityType::MINING
        };
        pattern.weight = 80;
        pattern.preferredBy = PersonalityType::COMPLETIONIST;
        _flowPatterns.push_back(pattern);
    }

    TC_LOG_DEBUG("playerbots.humanization", "Initialized {} flow patterns",
        _flowPatterns.size());
}

bool SessionTransitions::Update(uint32 diff)
{
    if (!_initialized || !_bot || !_bot->IsInWorld())
        return false;

    if (_activeTransition.state == TransitionState::NONE)
        return false;

    switch (_activeTransition.state)
    {
        case TransitionState::WRAP_UP:
            ProcessWrapUp(diff);
            break;
        case TransitionState::TRAVEL:
            ProcessTravel(diff);
            break;
        case TransitionState::PREPARATION:
            ProcessPreparation(diff);
            break;
        case TransitionState::READY:
            // Waiting for external completion call
            return true;
        case TransitionState::COMPLETED:
        case TransitionState::FAILED:
            // Already finished
            return true;
        default:
            break;
    }

    return _activeTransition.state == TransitionState::READY ||
           _activeTransition.state == TransitionState::COMPLETED;
}

void SessionTransitions::Shutdown()
{
    if (_activeTransition.state != TransitionState::NONE &&
        _activeTransition.state != TransitionState::COMPLETED &&
        _activeTransition.state != TransitionState::FAILED)
    {
        CancelTransition();
    }

    _initialized = false;
}

bool SessionTransitions::StartTransition(ActivityType fromActivity, ActivityType toActivity, bool forced)
{
    if (!_initialized)
        return false;

    // Check if already transitioning
    if (IsTransitioning() && !forced)
    {
        TC_LOG_DEBUG("playerbots.humanization", "Bot {}: Cannot start transition, already transitioning",
            _botGuid.GetCounter());
        return false;
    }

    // Cancel any existing transition
    if (IsTransitioning())
        CancelTransition();

    // Check global blocks
    if (!forced && IsTransitionBlocked())
    {
        _metrics.blockedAttempts++;
        _globalMetrics.blockedAttempts++;
        TC_LOG_DEBUG("playerbots.humanization", "Bot {}: Transition blocked (reason: {})",
            _botGuid.GetCounter(), static_cast<uint8>(GetGlobalBlockReason()));
        return false;
    }

    // Check specific transition
    if (!forced && !CanTransition(fromActivity, toActivity))
    {
        _metrics.blockedAttempts++;
        _globalMetrics.blockedAttempts++;
        TC_LOG_DEBUG("playerbots.humanization", "Bot {}: Transition from {} to {} not allowed",
            _botGuid.GetCounter(), static_cast<uint8>(fromActivity), static_cast<uint8>(toActivity));
        return false;
    }

    // Get transition rule
    TransitionRule const* rule = GetRule(fromActivity, toActivity);

    // Initialize transition
    _activeTransition = ActiveTransition();
    _activeTransition.fromActivity = fromActivity;
    _activeTransition.toActivity = toActivity;
    _activeTransition.startTime = std::chrono::steady_clock::now();
    _activeTransition.stateStartTime = _activeTransition.startTime;
    _activeTransition.isForced = forced;

    // Calculate timing
    if (rule)
    {
        // Random wrap-up time between min and max
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32> dist(rule->minWrapUpMs, rule->maxWrapUpMs);
        _activeTransition.wrapUpDurationMs = dist(gen);
        _activeTransition.prepDurationMs = rule->prepTimeMs;

        if (rule->requiresTravel)
        {
            _activeTransition.travelDurationMs = EstimateTravelTime(toActivity);
            _activeTransition.targetPosition = GetActivityPosition(toActivity);
        }
    }
    else
    {
        // Use defaults
        _activeTransition.wrapUpDurationMs = DEFAULT_WRAP_UP_MS;
        _activeTransition.prepDurationMs = DEFAULT_PREP_MS;
    }

    // Start wrap-up phase (or skip if immediate)
    if (fromActivity == ActivityType::NONE || _activeTransition.wrapUpDurationMs == 0)
    {
        // Skip to travel or prep
        if (_activeTransition.travelDurationMs > 0)
            _activeTransition.state = TransitionState::TRAVEL;
        else if (_activeTransition.prepDurationMs > 0)
            _activeTransition.state = TransitionState::PREPARATION;
        else
            _activeTransition.state = TransitionState::READY;
    }
    else
    {
        _activeTransition.state = TransitionState::WRAP_UP;
    }

    _activeTransition.stateStartTime = std::chrono::steady_clock::now();

    _metrics.totalTransitions++;
    _globalMetrics.totalTransitions++;

    TC_LOG_DEBUG("playerbots.humanization", "Bot {}: Started transition {} -> {} (state: {})",
        _botGuid.GetCounter(),
        static_cast<uint8>(fromActivity),
        static_cast<uint8>(toActivity),
        static_cast<uint8>(_activeTransition.state));

    return true;
}

void SessionTransitions::CancelTransition()
{
    if (_activeTransition.state == TransitionState::NONE)
        return;

    TC_LOG_DEBUG("playerbots.humanization", "Bot {}: Cancelled transition {} -> {}",
        _botGuid.GetCounter(),
        static_cast<uint8>(_activeTransition.fromActivity),
        static_cast<uint8>(_activeTransition.toActivity));

    _activeTransition.state = TransitionState::FAILED;
    _metrics.cancelledTransitions++;
    _globalMetrics.cancelledTransitions++;

    // Reset after a moment
    _activeTransition = ActiveTransition();
}

void SessionTransitions::CompleteTransition()
{
    if (_activeTransition.state != TransitionState::READY)
    {
        TC_LOG_WARN("playerbots.humanization", "Bot {}: CompleteTransition called but state is {}",
            _botGuid.GetCounter(), static_cast<uint8>(_activeTransition.state));
        return;
    }

    auto totalTime = _activeTransition.GetElapsedMs();

    _activeTransition.state = TransitionState::COMPLETED;
    _lastTransitionTime = std::chrono::steady_clock::now();

    _metrics.completedTransitions++;
    _metrics.totalTransitionTimeMs += totalTime;
    _globalMetrics.completedTransitions++;
    _globalMetrics.totalTransitionTimeMs += totalTime;

    TC_LOG_DEBUG("playerbots.humanization", "Bot {}: Completed transition {} -> {} in {}ms",
        _botGuid.GetCounter(),
        static_cast<uint8>(_activeTransition.fromActivity),
        static_cast<uint8>(_activeTransition.toActivity),
        totalTime);

    // Reset for next transition
    _activeTransition = ActiveTransition();
}

bool SessionTransitions::CanTransition(ActivityType fromActivity, ActivityType toActivity) const
{
    // Always allow transition to NONE (stopping)
    if (toActivity == ActivityType::NONE)
        return true;

    // Check cooldown
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - _lastTransitionTime).count();
    if (elapsed < MIN_TRANSITION_COOLDOWN_MS)
        return false;

    // Check if we have a rule (explicit allow)
    if (GetRule(fromActivity, toActivity) != nullptr)
        return true;

    // Default: allow transitions between different categories
    ActivityCategory fromCat = GetActivityCategory(fromActivity);
    ActivityCategory toCat = GetActivityCategory(toActivity);

    // Same category transitions are generally okay
    if (fromCat == toCat)
        return true;

    // Cross-category transitions need explicit rules for some cases
    // Block dangerous combinations
    if (toCat == ActivityCategory::COMBAT && fromCat == ActivityCategory::IDLE)
        return false;  // Don't go from leisure directly to combat

    return true;
}

TransitionBlockReason SessionTransitions::GetBlockReason(ActivityType fromActivity, ActivityType toActivity) const
{
    if (!_bot || !_bot->IsInWorld())
        return TransitionBlockReason::NONE;

    // Check global blocks first
    TransitionBlockReason globalReason = GetGlobalBlockReason();
    if (globalReason != TransitionBlockReason::NONE)
        return globalReason;

    // Check cooldown
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - _lastTransitionTime).count();
    if (elapsed < MIN_TRANSITION_COOLDOWN_MS)
        return TransitionBlockReason::COOLDOWN;

    // Check context-specific blocks
    ActivityCategory toCat = GetActivityCategory(toActivity);

    // Can't do outdoor activities from dungeon
    if (_bot->GetMap() && _bot->GetMap()->IsDungeon())
    {
        if (toCat == ActivityCategory::IDLE ||
            toActivity == ActivityType::MINING ||
            toActivity == ActivityType::ZONE_EXPLORATION)
        {
            return TransitionBlockReason::IN_DUNGEON;
        }
    }

    return TransitionBlockReason::NONE;
}

bool SessionTransitions::IsTransitionBlocked() const
{
    return GetGlobalBlockReason() != TransitionBlockReason::NONE;
}

TransitionBlockReason SessionTransitions::GetGlobalBlockReason() const
{
    if (!_bot || !_bot->IsInWorld())
        return TransitionBlockReason::NONE;

    // In combat
    if (_bot->IsInCombat())
        return TransitionBlockReason::IN_COMBAT;

    // Dead
    if (_bot->isDead())
        return TransitionBlockReason::DEAD;

    // In vehicle
    if (_bot->GetVehicle())
        return TransitionBlockReason::IN_VEHICLE;

    // In battleground
    if (_bot->InBattleground())
        return TransitionBlockReason::IN_BATTLEGROUND;

    return TransitionBlockReason::NONE;
}

ActivityType SessionTransitions::SuggestNextActivity(
    ActivityType currentActivity,
    PersonalityProfile const& personality) const
{
    auto ranked = GetRankedNextActivities(currentActivity, personality, 1);
    if (ranked.empty())
        return ActivityType::STANDING_IDLE;

    return ranked[0].first;
}

std::vector<std::pair<ActivityType, float>> SessionTransitions::GetRankedNextActivities(
    ActivityType currentActivity,
    PersonalityProfile const& personality,
    uint32 maxResults) const
{
    std::vector<std::pair<ActivityType, float>> results;

    // Get all valid targets
    std::vector<ActivityType> targets = GetValidTargets(currentActivity);

    // Calculate scores
    for (ActivityType target : targets)
    {
        if (!CanTransition(currentActivity, target))
            continue;

        float score = CalculateActivityScore(currentActivity, target, personality);
        results.emplace_back(target, score);
    }

    // Sort by score (descending)
    std::sort(results.begin(), results.end(),
        [](auto const& a, auto const& b) { return a.second > b.second; });

    // Limit results
    if (maxResults > 0 && results.size() > maxResults)
        results.resize(maxResults);

    return results;
}

bool SessionTransitions::ShouldTakeBreak(
    std::vector<ActivityType> const& recentActivities,
    PersonalityProfile const& personality) const
{
    // Count non-break activities
    uint32 activeCount = 0;
    for (ActivityType activity : recentActivities)
    {
        if (activity != ActivityType::AFK_SHORT &&
            activity != ActivityType::AFK_MEDIUM &&
            activity != ActivityType::AFK_LONG &&
            activity != ActivityType::INN_REST &&
            activity != ActivityType::STANDING_IDLE)
        {
            activeCount++;
        }
    }

    // Different personalities have different break thresholds
    uint32 threshold = 3;  // Default: break after 3 activities

    switch (personality.GetType())
    {
        case PersonalityType::CASUAL:
            threshold = 2;  // Break more often
            break;
        case PersonalityType::HARDCORE:
            threshold = 5;  // Break less often
            break;
        case PersonalityType::SPEEDRUNNER:
            threshold = 6;  // Rarely break
            break;
        default:
            break;
    }

    return activeCount >= threshold;
}

uint32 SessionTransitions::CalculateTransitionTime(ActivityType fromActivity, ActivityType toActivity) const
{
    TransitionRule const* rule = GetRule(fromActivity, toActivity);

    uint32 total = 0;

    if (rule)
    {
        // Average wrap-up time
        total += (rule->minWrapUpMs + rule->maxWrapUpMs) / 2;
        total += rule->prepTimeMs;

        if (rule->requiresTravel)
            total += EstimateTravelTime(toActivity);
    }
    else
    {
        total = DEFAULT_WRAP_UP_MS + DEFAULT_PREP_MS;
    }

    return total;
}

uint32 SessionTransitions::GetWrapUpTime(ActivityType activity, PersonalityProfile const& personality) const
{
    uint32 baseTime = DEFAULT_WRAP_UP_MS;

    // Check rules for this activity
    for (auto const& [key, rule] : _rules)
    {
        if (rule.fromActivity == activity)
        {
            baseTime = (rule.minWrapUpMs + rule.maxWrapUpMs) / 2;
            break;
        }
    }

    return ApplyPersonalityTiming(baseTime, personality, false);
}

uint32 SessionTransitions::GetPreparationTime(ActivityType activity, PersonalityProfile const& personality) const
{
    uint32 baseTime = DEFAULT_PREP_MS;

    // Check rules for this activity
    for (auto const& [key, rule] : _rules)
    {
        if (rule.toActivity == activity)
        {
            baseTime = rule.prepTimeMs;
            break;
        }
    }

    return ApplyPersonalityTiming(baseTime, personality, true);
}

TransitionRule const* SessionTransitions::GetRule(ActivityType fromActivity, ActivityType toActivity) const
{
    uint64 key = MakeRuleKey(fromActivity, toActivity);
    auto it = _rules.find(key);
    return it != _rules.end() ? &it->second : nullptr;
}

std::vector<ActivityType> SessionTransitions::GetValidTargets(ActivityType fromActivity) const
{
    std::vector<ActivityType> targets;

    for (auto const& [key, rule] : _rules)
    {
        if (rule.fromActivity == fromActivity)
        {
            targets.push_back(rule.toActivity);
        }
    }

    // Add common fallbacks if not explicitly defined
    if (targets.empty())
    {
        targets.push_back(ActivityType::STANDING_IDLE);
        targets.push_back(ActivityType::AFK_SHORT);
    }

    return targets;
}

void SessionTransitions::SetRule(TransitionRule const& rule)
{
    uint64 key = MakeRuleKey(rule.fromActivity, rule.toActivity);
    _rules[key] = rule;
}

TransitionFlowPattern const* SessionTransitions::GetRecommendedFlow(PersonalityProfile const& personality) const
{
    std::vector<TransitionFlowPattern const*> candidates;
    uint32 totalWeight = 0;

    for (auto const& pattern : _flowPatterns)
    {
        // Prefer patterns matching personality
        uint32 weight = pattern.weight;
        if (pattern.preferredBy == personality.GetType())
            weight = static_cast<uint32>(weight * 1.5f);

        if (weight > 0)
        {
            candidates.push_back(&pattern);
            totalWeight += weight;
        }
    }

    if (candidates.empty() || totalWeight == 0)
        return nullptr;

    // Weighted random selection
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32> dist(0, totalWeight - 1);
    uint32 roll = dist(gen);

    uint32 cumulative = 0;
    for (auto const* pattern : candidates)
    {
        uint32 weight = pattern->weight;
        if (pattern->preferredBy == personality.GetType())
            weight = static_cast<uint32>(weight * 1.5f);

        cumulative += weight;
        if (roll < cumulative)
            return pattern;
    }

    return candidates.back();
}

TransitionFlowPattern const* SessionTransitions::MatchFlowPattern(
    std::vector<ActivityType> const& recentActivities) const
{
    for (auto const& pattern : _flowPatterns)
    {
        if (pattern.sequence.size() > recentActivities.size())
            continue;

        // Check if recent activities match end of pattern
        bool matches = true;
        size_t offset = recentActivities.size() - pattern.sequence.size();

        for (size_t i = 0; i < pattern.sequence.size() && matches; ++i)
        {
            if (recentActivities[offset + i] != pattern.sequence[i])
                matches = false;
        }

        if (matches)
            return &pattern;
    }

    return nullptr;
}

void SessionTransitions::ProcessWrapUp(uint32 /*diff*/)
{
    uint32 elapsed = _activeTransition.GetStateElapsedMs();

    if (elapsed >= _activeTransition.wrapUpDurationMs)
    {
        AdvanceTransitionState();
    }
}

void SessionTransitions::ProcessTravel(uint32 /*diff*/)
{
    uint32 elapsed = _activeTransition.GetStateElapsedMs();

    // Check if we've arrived or time exceeded
    if (elapsed >= _activeTransition.travelDurationMs)
    {
        // In a real implementation, we'd check if bot actually arrived
        AdvanceTransitionState();
    }
}

void SessionTransitions::ProcessPreparation(uint32 /*diff*/)
{
    uint32 elapsed = _activeTransition.GetStateElapsedMs();

    if (elapsed >= _activeTransition.prepDurationMs)
    {
        AdvanceTransitionState();
    }
}

void SessionTransitions::AdvanceTransitionState()
{
    _activeTransition.stateStartTime = std::chrono::steady_clock::now();

    switch (_activeTransition.state)
    {
        case TransitionState::WRAP_UP:
            if (_activeTransition.travelDurationMs > 0)
                _activeTransition.state = TransitionState::TRAVEL;
            else if (_activeTransition.prepDurationMs > 0)
                _activeTransition.state = TransitionState::PREPARATION;
            else
                _activeTransition.state = TransitionState::READY;
            break;

        case TransitionState::TRAVEL:
            if (_activeTransition.prepDurationMs > 0)
                _activeTransition.state = TransitionState::PREPARATION;
            else
                _activeTransition.state = TransitionState::READY;
            break;

        case TransitionState::PREPARATION:
            _activeTransition.state = TransitionState::READY;
            break;

        default:
            break;
    }

    TC_LOG_DEBUG("playerbots.humanization", "Bot {}: Transition advanced to state {}",
        _botGuid.GetCounter(), static_cast<uint8>(_activeTransition.state));
}

uint32 SessionTransitions::EstimateTravelTime(ActivityType toActivity) const
{
    if (!_bot || !_bot->IsInWorld())
        return DEFAULT_TRAVEL_MS;

    Position target = GetActivityPosition(toActivity);

    // If target is invalid, use default
    if (target.GetPositionX() == 0.0f && target.GetPositionY() == 0.0f)
        return DEFAULT_TRAVEL_MS;

    // Calculate distance
    float distance = _bot->GetDistance(target);

    // Estimate time based on movement speed (walking ~7 yards/sec, mounted ~14 yards/sec)
    float speed = _bot->GetSpeed(MOVE_RUN);
    if (speed < 1.0f)
        speed = 7.0f;

    uint32 estimatedMs = static_cast<uint32>((distance / speed) * 1000.0f);

    // Add buffer for pathing inefficiency
    estimatedMs = static_cast<uint32>(estimatedMs * 1.3f);

    // Clamp to reasonable range
    return std::clamp(estimatedMs, 5000u, 300000u);  // 5 sec to 5 min
}

Position SessionTransitions::GetActivityPosition(ActivityType activity) const
{
    // In a real implementation, this would query NPCs, locations, etc.
    // For now, return current position for most activities

    if (!_bot)
        return Position();

    // Activities that need specific locations would be looked up here
    // For now, just return current position
    return *_bot;
}

float SessionTransitions::CalculateActivityScore(
    ActivityType fromActivity,
    ActivityType toActivity,
    PersonalityProfile const& personality) const
{
    float score = 50.0f;  // Base score

    // Check rule priority
    TransitionRule const* rule = GetRule(fromActivity, toActivity);
    if (rule)
    {
        score = static_cast<float>(rule->priority);
    }

    // Apply personality modifiers
    ActivityCategory toCat = GetActivityCategory(toActivity);

    switch (personality.GetType())
    {
        case PersonalityType::CASUAL:
            if (toCat == ActivityCategory::IDLE)
                score *= 1.3f;
            if (toActivity == ActivityType::AFK_SHORT)
                score *= 1.4f;
            break;

        case PersonalityType::HARDCORE:
            if (toCat == ActivityCategory::QUESTING)
                score *= 1.3f;
            if (toActivity == ActivityType::AFK_SHORT)
                score *= 0.7f;
            break;

        case PersonalityType::EXPLORER:
            if (toActivity == ActivityType::ZONE_EXPLORATION)
                score *= 1.5f;
            if (toActivity == ActivityType::MINING)
                score *= 1.2f;
            break;

        case PersonalityType::SOCIAL:
            if (toActivity == ActivityType::CHATTING ||
                toActivity == ActivityType::CITY_WANDERING)
                score *= 1.4f;
            break;

        case PersonalityType::COMPLETIONIST:
            if (toCat == ActivityCategory::QUESTING)
                score *= 1.2f;
            if (toActivity == ActivityType::QUEST_OBJECTIVE)
                score *= 1.3f;
            break;

        case PersonalityType::PVP_ORIENTED:
            if (toCat == ActivityCategory::COMBAT || toCat == ActivityCategory::PVP)
                score *= 1.4f;
            if (toActivity == ActivityType::AFK_SHORT)
                score *= 0.5f;
            break;

        default:
            break;
    }

    // Small random variation for naturalness
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.9f, 1.1f);
    score *= dist(gen);

    return score;
}

uint32 SessionTransitions::ApplyPersonalityTiming(
    uint32 baseTimeMs,
    PersonalityProfile const& personality,
    bool isPatient) const
{
    float multiplier = 1.0f;

    switch (personality.GetType())
    {
        case PersonalityType::CASUAL:
            multiplier = isPatient ? 1.3f : 1.2f;  // Takes their time
            break;
        case PersonalityType::HARDCORE:
            multiplier = isPatient ? 0.8f : 0.7f;  // Quick transitions
            break;
        case PersonalityType::SPEEDRUNNER:
            multiplier = isPatient ? 0.6f : 0.5f;  // Very fast
            break;
        default:
            break;
    }

    return static_cast<uint32>(baseTimeMs * multiplier);
}

} // namespace Humanization
} // namespace Playerbot
