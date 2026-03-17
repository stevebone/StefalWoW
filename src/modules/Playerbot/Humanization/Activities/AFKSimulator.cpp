/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "AFKSimulator.h"
#include "AI/BotAI.h"
#include "Player.h"
#include "Map.h"
#include "GameTime.h"
#include "Log.h"
#include "Chat.h"
#include "SharedDefines.h"
#include <algorithm>

namespace Playerbot
{
namespace Humanization
{

// Static emote list for AFK behaviors
const std::vector<uint32> AFKSimulator::AFK_EMOTES = {
    10,     // EMOTE_ONESHOT_WAVE
    11,     // EMOTE_ONESHOT_BOW
    5,      // EMOTE_ONESHOT_EXCLAMATION
    6,      // EMOTE_ONESHOT_QUESTION
    14,     // EMOTE_ONESHOT_YES
    15,     // EMOTE_ONESHOT_NO
    20,     // EMOTE_ONESHOT_POINT
    23,     // EMOTE_ONESHOT_RUDE
    24,     // EMOTE_ONESHOT_ROAR
    26,     // EMOTE_ONESHOT_CHEER
    7,      // EMOTE_ONESHOT_EAT
    69,     // EMOTE_ONESHOT_TALK
    71,     // EMOTE_ONESHOT_LAUGH
    73,     // EMOTE_ONESHOT_DANCE
    94,     // EMOTE_ONESHOT_SHY
    113,    // EMOTE_ONESHOT_YAWN
};

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

AFKSimulator::AFKSimulator(Player* bot, BotAI* ai)
    : BehaviorManager(bot, ai, 5000, "AFKSimulator")  // 5 second update
{
    // Seed random generator
    std::random_device rd;
    _rng.seed(rd());
}

AFKSimulator::~AFKSimulator()
{
    if (_currentSession.isActive)
    {
        EndAFK(true);
    }
}

// ============================================================================
// LIFECYCLE
// ============================================================================

bool AFKSimulator::OnInitialize()
{
    if (!GetBot() || !GetBot()->IsInWorld())
        return false;

    _sessionStartTime = GetCurrentTimeMs();
    _lastActivityStartTime = _sessionStartTime;
    _currentSession.Reset();

    TC_LOG_DEBUG("module.playerbot.humanization",
        "AFKSimulator::OnInitialize - Bot {} AFK simulator initialized",
        GetBot() ? GetBot()->GetName() : "unknown");

    return true;
}

void AFKSimulator::OnShutdown()
{
    if (_currentSession.isActive)
    {
        EndAFK(true);
    }

    _callbacks.clear();

    // Note: Don't access GetBot() here - it may already be destroyed during shutdown
    TC_LOG_DEBUG("module.playerbot.humanization",
        "AFKSimulator::OnShutdown - AFK simulator shutdown complete");
}

void AFKSimulator::OnUpdate(uint32 elapsed)
{
    if (!GetBot() || !GetBot()->IsInWorld())
        return;

    if (_currentSession.isActive)
    {
        UpdateAFKState(elapsed);
    }
    else if (_autoAFKEnabled)
    {
        CheckAutoAFK();
    }
}

// ============================================================================
// AFK STATE
// ============================================================================

uint32 AFKSimulator::GetRemainingAFKTime() const
{
    if (!_currentSession.isActive)
        return 0;

    if (_currentSession.actualDurationMs >= _currentSession.plannedDurationMs)
        return 0;

    return _currentSession.plannedDurationMs - _currentSession.actualDurationMs;
}

float AFKSimulator::GetAFKProgress() const
{
    if (!_currentSession.isActive || _currentSession.plannedDurationMs == 0)
        return 0.0f;

    return static_cast<float>(_currentSession.actualDurationMs) /
           static_cast<float>(_currentSession.plannedDurationMs);
}

// ============================================================================
// AFK CONTROL
// ============================================================================

bool AFKSimulator::StartAFK(AFKType type, std::string const& reason)
{
    if (!CanGoAFK())
        return false;

    if (_currentSession.isActive)
    {
        TC_LOG_DEBUG("module.playerbot.humanization",
            "AFKSimulator::StartAFK - Bot {} already AFK",
            GetBot() ? GetBot()->GetName() : "unknown");
        return false;
    }

    uint32 duration = CalculateAFKDuration(type);
    return StartAFKWithDuration(duration, type, reason);
}

bool AFKSimulator::StartAFKWithDuration(uint32 durationMs, AFKType type,
    std::string const& reason)
{
    if (!CanGoAFK())
        return false;

    if (_currentSession.isActive)
        return false;

    // Initialize session
    _currentSession.type = type;
    _currentSession.startTimeMs = GetCurrentTimeMs();
    _currentSession.plannedDurationMs = durationMs;
    _currentSession.actualDurationMs = 0;
    _currentSession.isActive = true;
    _currentSession.wasInterrupted = false;
    _currentSession.reason = reason;

    // Set behaviors based on type
    switch (type)
    {
        case AFKType::LONG:
        case AFKType::BIO_BREAK:
        case AFKType::SESSION_END:
            _currentSession.behaviors = AFKBehaviorFlags::SIT_DOWN |
                AFKBehaviorFlags::MOUNT_DISMOUNT |
                AFKBehaviorFlags::USE_CHAT_AFK;
            break;

        case AFKType::MEDIUM:
        case AFKType::SNACK_BREAK:
            _currentSession.behaviors = AFKBehaviorFlags::SIT_DOWN |
                AFKBehaviorFlags::SLIGHT_MOVEMENT;
            break;

        case AFKType::SHORT:
        case AFKType::PHONE_CHECK:
        case AFKType::DISTRACTION:
        default:
            _currentSession.behaviors = AFKBehaviorFlags::SLIGHT_MOVEMENT |
                AFKBehaviorFlags::EMOTE_RANDOMLY;
            break;
    }

    // Perform initial behaviors
    if (HasFlag(_currentSession.behaviors, AFKBehaviorFlags::MOUNT_DISMOUNT))
    {
        DoDismount();
    }

    if (HasFlag(_currentSession.behaviors, AFKBehaviorFlags::SIT_DOWN))
    {
        DoSitDown();
    }

    if (HasFlag(_currentSession.behaviors, AFKBehaviorFlags::USE_CHAT_AFK))
    {
        SetChatAFKStatus(true);
    }

    // Schedule first behavior actions
    _nextEmoteTime = GetCurrentTimeMs() + RandomInRange(
        _timingConfig.emoteIntervalMinMs, _timingConfig.emoteIntervalMaxMs);
    _nextMovementTime = GetCurrentTimeMs() + RandomInRange(
        _timingConfig.movementIntervalMinMs, _timingConfig.movementIntervalMaxMs);

    // Update statistics
    _statistics.totalAFKCount++;
    switch (type)
    {
        case AFKType::SHORT:
        case AFKType::PHONE_CHECK:
        case AFKType::DISTRACTION:
            _statistics.shortAFKCount++;
            break;
        case AFKType::MEDIUM:
        case AFKType::SNACK_BREAK:
            _statistics.mediumAFKCount++;
            break;
        case AFKType::LONG:
        case AFKType::SESSION_END:
            _statistics.longAFKCount++;
            break;
        case AFKType::BIO_BREAK:
            _statistics.bioBreakCount++;
            break;
        default:
            break;
    }

    NotifyStateChange(true);

    TC_LOG_DEBUG("module.playerbot.humanization",
        "AFKSimulator::StartAFK - Bot {} started {} ({} ms) - {}",
        GetBot() ? GetBot()->GetName() : "unknown",
        GetAFKTypeName(type),
        durationMs,
        reason.empty() ? "no reason" : reason.c_str());

    return true;
}

void AFKSimulator::EndAFK(bool wasInterrupted)
{
    if (!_currentSession.isActive)
        return;

    _currentSession.wasInterrupted = wasInterrupted;
    _currentSession.isActive = false;

    // Perform end behaviors
    if (HasFlag(_currentSession.behaviors, AFKBehaviorFlags::SIT_DOWN))
    {
        DoStandUp();
    }

    if (HasFlag(_currentSession.behaviors, AFKBehaviorFlags::USE_CHAT_AFK))
    {
        SetChatAFKStatus(false);
    }

    // Update statistics
    _statistics.totalAFKTimeMs += _currentSession.actualDurationMs;
    if (wasInterrupted)
    {
        _statistics.interruptedCount++;
    }

    _lastAFKEndTime = GetCurrentTimeMs();
    _lastActivityStartTime = _lastAFKEndTime;

    NotifyStateChange(false);

    TC_LOG_DEBUG("module.playerbot.humanization",
        "AFKSimulator::EndAFK - Bot {} ended {} after {} ms (interrupted: {})",
        GetBot() ? GetBot()->GetName() : "unknown",
        GetAFKTypeName(_currentSession.type),
        _currentSession.actualDurationMs,
        wasInterrupted ? "yes" : "no");

    // Save type before reset for logging
    _currentSession.Reset();
}

void AFKSimulator::ForceEndAFK()
{
    EndAFK(true);
}

// ============================================================================
// AFK DECISIONS
// ============================================================================

bool AFKSimulator::ShouldGoAFK() const
{
    if (!CanGoAFK())
        return false;

    // Get base probability
    float probability = GetBaseAFKProbability();

    // Apply session modifier (longer sessions = higher chance)
    probability *= GetSessionModifier();

    // Apply personality modifier
    probability *= GetPersonalityAFKModifier();

    // Apply time of day modifier
    probability *= GetTimeOfDayModifier();

    // Random check
    return RandomFloat() < probability;
}

AFKType AFKSimulator::GetRecommendedAFKType() const
{
    uint32 timeSinceLastAFK = GetTimeSinceLastAFK();
    uint32 sessionDuration = GetTotalSessionTime();

    // Long session without AFK = recommend longer break
    if (timeSinceLastAFK > 7200000 || sessionDuration > 14400000)  // 2h / 4h
    {
        return AFKType::LONG;
    }

    if (timeSinceLastAFK > 3600000 || sessionDuration > 7200000)  // 1h / 2h
    {
        return AFKType::BIO_BREAK;
    }

    if (timeSinceLastAFK > 1800000)  // 30 min
    {
        return AFKType::MEDIUM;
    }

    // Default to weighted random selection
    return SelectWeightedAFKType();
}

bool AFKSimulator::CanGoAFK() const
{
    if (!GetBot() || !GetBot()->IsInWorld())
        return false;

    // Don't go AFK in combat
    if (GetBot()->IsInCombat())
        return false;

    // Don't go AFK if dead
    if (!GetBot()->IsAlive())
        return false;

    // Don't go AFK in dungeon/raid (unless specifically requested)
    if (GetBot()->GetMap() && GetBot()->GetMap()->IsDungeon())
        return false;

    // Don't go AFK in battleground
    if (GetBot()->InBattleground())
        return false;

    // Don't go AFK in arena
    if (GetBot()->InArena())
        return false;

    return true;
}

bool AFKSimulator::TryTriggerAFK()
{
    if (!ShouldGoAFK())
        return false;

    AFKType type = GetRecommendedAFKType();
    return StartAFK(type, "Auto-triggered");
}

// ============================================================================
// PERSONALITY
// ============================================================================

void AFKSimulator::SetPersonality(PersonalityProfile const* personality)
{
    _personality = personality;
}

float AFKSimulator::GetPersonalityAFKModifier() const
{
    if (!_personality)
        return 1.0f;

    // Higher afkFrequency trait = more likely to AFK
    float modifier = 1.0f + (_personality->GetTraits().afkFrequency * 2.0f);

    // Efficiency reduces AFK tendency
    modifier *= (2.0f - _personality->GetTraits().efficiency);

    // Break frequency affects AFK
    modifier *= (1.0f + _personality->GetTraits().breakFrequency);

    return std::clamp(modifier, 0.1f, 5.0f);
}

// ============================================================================
// TIMING CONFIGURATION
// ============================================================================

void AFKSimulator::GetDurationRange(AFKType type, uint32& outMinMs, uint32& outMaxMs) const
{
    switch (type)
    {
        case AFKType::SHORT:
            outMinMs = _timingConfig.shortMinMs;
            outMaxMs = _timingConfig.shortMaxMs;
            break;
        case AFKType::MEDIUM:
            outMinMs = _timingConfig.mediumMinMs;
            outMaxMs = _timingConfig.mediumMaxMs;
            break;
        case AFKType::LONG:
            outMinMs = _timingConfig.longMinMs;
            outMaxMs = _timingConfig.longMaxMs;
            break;
        case AFKType::BIO_BREAK:
            outMinMs = _timingConfig.bioBreakMinMs;
            outMaxMs = _timingConfig.bioBreakMaxMs;
            break;
        case AFKType::SNACK_BREAK:
            outMinMs = _timingConfig.snackBreakMinMs;
            outMaxMs = _timingConfig.snackBreakMaxMs;
            break;
        case AFKType::PHONE_CHECK:
            outMinMs = _timingConfig.phoneCheckMinMs;
            outMaxMs = _timingConfig.phoneCheckMaxMs;
            break;
        case AFKType::DISTRACTION:
            outMinMs = _timingConfig.distractionMinMs;
            outMaxMs = _timingConfig.distractionMaxMs;
            break;
        case AFKType::SESSION_END:
            outMinMs = _timingConfig.sessionEndMinMs;
            outMaxMs = _timingConfig.sessionEndMaxMs;
            break;
        default:
            outMinMs = _timingConfig.shortMinMs;
            outMaxMs = _timingConfig.shortMaxMs;
            break;
    }
}

// ============================================================================
// CALLBACKS
// ============================================================================

void AFKSimulator::OnAFKStateChange(AFKCallback callback)
{
    _callbacks.push_back(callback);
}

// ============================================================================
// SESSION TRACKING
// ============================================================================

uint32 AFKSimulator::GetTimeSinceLastAFK() const
{
    if (_lastAFKEndTime == 0)
    {
        // Never been AFK
        return GetTotalSessionTime();
    }

    return GetCurrentTimeMs() - _lastAFKEndTime;
}

uint32 AFKSimulator::GetTotalSessionTime() const
{
    return GetCurrentTimeMs() - _sessionStartTime;
}

uint32 AFKSimulator::GetCurrentActivityDuration() const
{
    return GetCurrentTimeMs() - _lastActivityStartTime;
}

// ============================================================================
// INTERNAL UPDATE METHODS
// ============================================================================

void AFKSimulator::UpdateAFKState(uint32 elapsed)
{
    _currentSession.actualDurationMs += elapsed;

    // Handle behaviors during AFK
    HandleAFKBehaviors(elapsed);

    // Check if AFK should end
    if (ShouldEndAFK())
    {
        EndAFK(false);
    }
}

void AFKSimulator::CheckAutoAFK()
{
    // This is called every update interval
    // Use probability to determine if we should trigger AFK

    TryTriggerAFK();
}

void AFKSimulator::HandleAFKBehaviors(uint32 /*elapsed*/)
{
    uint32 currentTime = GetCurrentTimeMs();

    // Random emote
    if (HasFlag(_currentSession.behaviors, AFKBehaviorFlags::EMOTE_RANDOMLY) &&
        currentTime >= _nextEmoteTime)
    {
        DoRandomEmote();
        _nextEmoteTime = currentTime + RandomInRange(
            _timingConfig.emoteIntervalMinMs, _timingConfig.emoteIntervalMaxMs);
    }

    // Slight movement
    if (HasFlag(_currentSession.behaviors, AFKBehaviorFlags::SLIGHT_MOVEMENT) &&
        currentTime >= _nextMovementTime)
    {
        DoSlightMovement();
        _nextMovementTime = currentTime + RandomInRange(
            _timingConfig.movementIntervalMinMs, _timingConfig.movementIntervalMaxMs);
    }
}

bool AFKSimulator::ShouldEndAFK() const
{
    // Check if planned duration has elapsed
    if (_currentSession.actualDurationMs >= _currentSession.plannedDurationMs)
    {
        return true;
    }

    // Check if we entered combat
    if (GetBot() && GetBot()->IsInCombat())
    {
        return true;
    }

    return false;
}

// ============================================================================
// BEHAVIOR METHODS
// ============================================================================

void AFKSimulator::DoSitDown()
{
    if (!GetBot())
        return;

    // SetStandState to sitting
    GetBot()->SetStandState(UNIT_STAND_STATE_SIT);
}

void AFKSimulator::DoStandUp()
{
    if (!GetBot())
        return;

    GetBot()->SetStandState(UNIT_STAND_STATE_STAND);
}

void AFKSimulator::DoRandomEmote()
{
    if (!GetBot() || AFK_EMOTES.empty())
        return;

    uint32 emoteIndex = RandomInRange(0, static_cast<uint32>(AFK_EMOTES.size() - 1));
    GetBot()->HandleEmoteCommand(static_cast<Emote>(AFK_EMOTES[emoteIndex]));
}

void AFKSimulator::DoSlightMovement()
{
    if (!GetBot())
        return;

    // Slight orientation change
    float currentO = GetBot()->GetOrientation();
    float delta = (RandomFloat() - 0.5f) * 0.5f;  // -0.25 to +0.25 radians
    GetBot()->SetFacingTo(currentO + delta);
}

void AFKSimulator::DoDismount()
{
    if (!GetBot())
        return;

    if (GetBot()->IsMounted())
    {
        GetBot()->Dismount();
    }
}

void AFKSimulator::SetChatAFKStatus(bool afk)
{
    if (!GetBot())
        return;

    if (afk)
    {
        GetBot()->SetPlayerFlag(PLAYER_FLAGS_AFK);
    }
    else
    {
        GetBot()->RemovePlayerFlag(PLAYER_FLAGS_AFK);
    }
}

// ============================================================================
// DURATION CALCULATION
// ============================================================================

uint32 AFKSimulator::CalculateAFKDuration(AFKType type) const
{
    uint32 minMs, maxMs;
    GetDurationRange(type, minMs, maxMs);

    uint32 baseDuration = RandomInRange(minMs, maxMs);
    return ApplyPersonalityModifiers(baseDuration);
}

uint32 AFKSimulator::ApplyPersonalityModifiers(uint32 baseDurationMs) const
{
    if (!_personality)
        return baseDurationMs;

    float modifier = _personality->GetTraits().breakDurationMultiplier;
    return static_cast<uint32>(baseDurationMs * modifier);
}

float AFKSimulator::GetTimeOfDayModifier() const
{
    // Get current hour (0-23)
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    int hour = timeinfo->tm_hour;

    // Late night = more likely to AFK
    if (hour >= 0 && hour < 6)   return 1.5f;   // Midnight to 6am
    if (hour >= 23)              return 1.3f;   // 11pm to midnight

    // Early morning = slightly more AFK
    if (hour >= 6 && hour < 9)   return 1.2f;

    // Normal hours
    return 1.0f;
}

// ============================================================================
// PROBABILITY METHODS
// ============================================================================

float AFKSimulator::GetBaseAFKProbability() const
{
    return _baseAFKProbability;
}

float AFKSimulator::GetSessionModifier() const
{
    uint32 timeSinceAFK = GetTimeSinceLastAFK();

    // Increase probability over time without AFK
    if (timeSinceAFK < 600000)    return 0.5f;   // < 10 min: low chance
    if (timeSinceAFK < 1800000)   return 1.0f;   // 10-30 min: normal
    if (timeSinceAFK < 3600000)   return 1.5f;   // 30-60 min: higher
    if (timeSinceAFK < 7200000)   return 2.0f;   // 1-2 hours: much higher
    return 3.0f;                                  // > 2 hours: very high
}

AFKType AFKSimulator::SelectWeightedAFKType() const
{
    float total = WEIGHT_SHORT + WEIGHT_MEDIUM + WEIGHT_LONG +
                  WEIGHT_BIO + WEIGHT_SNACK + WEIGHT_PHONE + WEIGHT_DISTRACTION;

    float roll = RandomFloat() * total;

    if (roll < WEIGHT_SHORT)                           return AFKType::SHORT;
    roll -= WEIGHT_SHORT;
    if (roll < WEIGHT_MEDIUM)                          return AFKType::MEDIUM;
    roll -= WEIGHT_MEDIUM;
    if (roll < WEIGHT_LONG)                            return AFKType::LONG;
    roll -= WEIGHT_LONG;
    if (roll < WEIGHT_BIO)                             return AFKType::BIO_BREAK;
    roll -= WEIGHT_BIO;
    if (roll < WEIGHT_SNACK)                           return AFKType::SNACK_BREAK;
    roll -= WEIGHT_SNACK;
    if (roll < WEIGHT_PHONE)                           return AFKType::PHONE_CHECK;

    return AFKType::DISTRACTION;
}

// ============================================================================
// NOTIFICATION
// ============================================================================

void AFKSimulator::NotifyStateChange(bool started)
{
    for (auto const& callback : _callbacks)
    {
        if (callback)
        {
            callback(_currentSession.type, started);
        }
    }
}

// ============================================================================
// HELPER METHODS
// ============================================================================

uint32 AFKSimulator::GetCurrentTimeMs() const
{
    return GameTime::GetGameTimeMS();
}

uint32 AFKSimulator::RandomInRange(uint32 min, uint32 max) const
{
    if (min >= max)
        return min;

    std::uniform_int_distribution<uint32> dist(min, max);
    return dist(_rng);
}

float AFKSimulator::RandomFloat() const
{
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(_rng);
}

} // namespace Humanization
} // namespace Playerbot
