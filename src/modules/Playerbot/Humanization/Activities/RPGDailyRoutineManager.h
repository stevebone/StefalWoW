/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * RPG Daily Routine Manager: Coordinates autonomous daily routines for bots.
 * When a bot has no master and no group, it follows a personality-driven
 * daily schedule of activities including grinding, questing, exploring,
 * city life, and resting.
 *
 * State Machine:
 *   IDLE -> CITY_LIFE -> GRINDING -> QUESTING -> TRAVELING -> TRAINING
 *        -> GATHERING -> EXPLORING -> RESTING -> DUNGEON -> SOCIALIZING
 *
 * Delegates to existing managers:
 *   CityLifeBehaviorManager for CITY_LIFE
 *   BotIdleBehaviorManager for IDLE
 *   Uses PersonalityProfile for schedule generation
 */

#pragma once

#include "Define.h"
#include "ActivityType.h"
#include "RPGActivityScheduler.h"
#include "ZoneSelector.h"
#include <memory>

class Player;

namespace Playerbot
{
namespace Humanization
{

class PersonalityProfile;

/// RPG simulation states
enum class RPGState : uint8
{
    INACTIVE    = 0,   // Manager not active (bot has master or is in group)
    IDLE        = 1,   // Waiting for next activity
    CITY_LIFE   = 2,   // In city doing social/merchant activities
    GRINDING    = 3,   // Killing mobs for XP/loot
    QUESTING    = 4,   // Following quest chains
    TRAVELING   = 5,   // Moving between zones
    TRAINING    = 6,   // At trainer, learning skills
    GATHERING   = 7,   // Mining, herbing, fishing
    EXPLORING   = 8,   // Discovering new areas
    RESTING     = 9,   // At inn or safe area
    DUNGEON     = 10,  // Running dungeon content
    SOCIALIZING = 11,  // Chatting, emoting, grouped activities
};

/// Returns the name of an RPG state.
inline const char* GetRPGStateName(RPGState state)
{
    switch (state)
    {
        case RPGState::INACTIVE:    return "Inactive";
        case RPGState::IDLE:        return "Idle";
        case RPGState::CITY_LIFE:   return "CityLife";
        case RPGState::GRINDING:    return "Grinding";
        case RPGState::QUESTING:    return "Questing";
        case RPGState::TRAVELING:   return "Traveling";
        case RPGState::TRAINING:    return "Training";
        case RPGState::GATHERING:   return "Gathering";
        case RPGState::EXPLORING:   return "Exploring";
        case RPGState::RESTING:     return "Resting";
        case RPGState::DUNGEON:     return "Dungeon";
        case RPGState::SOCIALIZING: return "Socializing";
        default:                    return "Unknown";
    }
}

/// Maps ActivityType categories to RPGState
inline RPGState ActivityToRPGState(ActivityType activity)
{
    ActivityCategory cat = GetActivityCategory(activity);
    switch (cat)
    {
        case ActivityCategory::CITY_LIFE:   return RPGState::CITY_LIFE;
        case ActivityCategory::COMBAT:      return RPGState::GRINDING;
        case ActivityCategory::QUESTING:    return RPGState::QUESTING;
        case ActivityCategory::GATHERING:   return RPGState::GATHERING;
        case ActivityCategory::EXPLORATION: return RPGState::EXPLORING;
        case ActivityCategory::SOCIAL:      return RPGState::SOCIALIZING;
        case ActivityCategory::DUNGEONS:    return RPGState::DUNGEON;
        case ActivityCategory::TRAVELING:   return RPGState::TRAVELING;
        case ActivityCategory::AFK:         return RPGState::RESTING;
        case ActivityCategory::IDLE:        return RPGState::IDLE;
        default:                            return RPGState::IDLE;
    }
}

/// Per-bot RPG daily routine manager.
class TC_GAME_API RPGDailyRoutineManager
{
public:
    explicit RPGDailyRoutineManager(Player* bot);
    ~RPGDailyRoutineManager() = default;

    /// Main update function, called from BotAI when bot is autonomous.
    void Update(uint32 diff);

    /// Check if this bot should use RPG simulation (no master, no group).
    bool ShouldBeActive() const;

    /// Get current RPG state.
    RPGState GetCurrentState() const { return _currentState; }

    /// Get current activity type.
    ActivityType GetCurrentActivity() const { return _currentActivity; }

    /// Force a state transition (for debugging/commands).
    void ForceState(RPGState newState);

    /// Reset and regenerate schedule.
    void RegenerateSchedule();

    /// Get time remaining in current activity (ms).
    uint32 GetActivityTimeRemaining() const;

private:
    /// Transition to a new RPG state.
    void TransitionTo(RPGState newState, ActivityType activity);

    /// Select the next activity based on schedule and personality.
    void SelectNextActivity();

    /// Process the current state's behavior.
    void ProcessCurrentState(uint32 diff);

    /// Handle state-specific logic.
    void ProcessIdle(uint32 diff);
    void ProcessCityLife(uint32 diff);
    void ProcessGrinding(uint32 diff);
    void ProcessQuesting(uint32 diff);
    void ProcessGathering(uint32 diff);
    void ProcessExploring(uint32 diff);
    void ProcessResting(uint32 diff);
    void ProcessTraveling(uint32 diff);

    Player* _bot;
    RPGState _currentState;
    ActivityType _currentActivity;

    // Schedule management
    RPGActivityScheduler _scheduler;
    ZoneSelector _zoneSelector;
    DailySchedule _dailySchedule;
    bool _scheduleGenerated;

    // Timing
    uint32 _activityStartTime;      // When current activity started
    uint32 _activityDurationMs;     // How long to stay in this activity
    uint32 _stateCheckTimer;        // Timer for periodic state checks
    uint32 _nextActivityCheckMs;    // When to check for next activity

    // State
    uint32 _lastScheduleDay;        // Day number when schedule was last generated

    static constexpr uint32 STATE_CHECK_INTERVAL_MS = 5000;     // Check every 5 seconds
    static constexpr uint32 MIN_ACTIVITY_DURATION_MS = 30000;   // Minimum 30s per activity
    static constexpr uint32 MAX_ACTIVITY_DURATION_MS = 600000;  // Maximum 10min per activity
};

} // namespace Humanization
} // namespace Playerbot
