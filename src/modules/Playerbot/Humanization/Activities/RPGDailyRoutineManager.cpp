/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * RPG Daily Routine Manager implementation.
 */

#include "RPGDailyRoutineManager.h"
#include "PersonalityProfile.h"
#include "Player.h"
#include "Group.h"
#include "Map.h"
#include "GameTime.h"
#include "ObjectAccessor.h"
#include "Log.h"
#include <random>
#include <ctime>

namespace Playerbot
{
namespace Humanization
{

RPGDailyRoutineManager::RPGDailyRoutineManager(Player* bot)
    : _bot(bot)
    , _currentState(RPGState::INACTIVE)
    , _currentActivity(ActivityType::STANDING_IDLE)
    , _scheduleGenerated(false)
    , _activityStartTime(0)
    , _activityDurationMs(0)
    , _stateCheckTimer(0)
    , _nextActivityCheckMs(0)
    , _lastScheduleDay(0)
{
}

bool RPGDailyRoutineManager::ShouldBeActive() const
{
    if (!_bot || !_bot->IsInWorld() || !_bot->IsAlive())
        return false;

    // Only active when bot has no master and no group
    // (autonomous bot mode)
    if (_bot->GetGroup() && _bot->GetGroup()->GetMembersCount() > 1)
        return false;

    return true;
}

void RPGDailyRoutineManager::Update(uint32 diff)
{
    if (!ShouldBeActive())
    {
        if (_currentState != RPGState::INACTIVE)
        {
            _currentState = RPGState::INACTIVE;
            TC_LOG_DEBUG("module.playerbot", "RPGDailyRoutine: Bot {} deactivated (has master/group)",
                _bot->GetGUID().GetCounter());
        }
        return;
    }

    // Activate if we were inactive
    if (_currentState == RPGState::INACTIVE)
    {
        _currentState = RPGState::IDLE;
        TC_LOG_DEBUG("module.playerbot", "RPGDailyRoutine: Bot {} activated",
            _bot->GetGUID().GetCounter());
    }

    // Generate/regenerate schedule for new day
    time_t gameTime = GameTime::GetGameTime();
    struct tm timeInfo;
#ifdef _WIN32
    localtime_s(&timeInfo, &gameTime);
#else
    localtime_r(&gameTime, &timeInfo);
#endif
    uint32 currentDay = timeInfo.tm_yday;

    if (!_scheduleGenerated || currentDay != _lastScheduleDay)
    {
        RegenerateSchedule();
        _lastScheduleDay = currentDay;
    }

    _stateCheckTimer += diff;

    // Periodic state check
    if (_stateCheckTimer >= STATE_CHECK_INTERVAL_MS)
    {
        _stateCheckTimer = 0;

        uint32 now = GameTime::GetGameTimeMS();

        // Check if current activity has expired
        if (_activityDurationMs > 0 && _activityStartTime > 0)
        {
            uint32 elapsed = now - _activityStartTime;
            if (elapsed >= _activityDurationMs)
            {
                TC_LOG_DEBUG("module.playerbot", "RPGDailyRoutine: Bot {} activity {} expired after {}s",
                    _bot->GetGUID().GetCounter(),
                    GetRPGStateName(_currentState),
                    elapsed / 1000);

                SelectNextActivity();
            }
        }
        else if (_currentState == RPGState::IDLE)
        {
            // In idle, select next activity immediately
            SelectNextActivity();
        }
    }

    // Process current state behavior
    ProcessCurrentState(diff);
}

void RPGDailyRoutineManager::SelectNextActivity()
{
    if (!_scheduleGenerated)
        return;

    // Select activity from schedule based on current time period
    ActivityType nextActivity = _scheduler.SelectActivityForCurrentTime(_dailySchedule);

    RPGState nextState = ActivityToRPGState(nextActivity);

    // Check if zone is appropriate; if not, travel first
    if (!_zoneSelector.IsInAppropriateZone(_bot, nextActivity) && nextState != RPGState::TRAVELING)
    {
        TransitionTo(RPGState::TRAVELING, nextActivity);
        return;
    }

    // Determine duration based on schedule slot
    TimePeriod period = RPGActivityScheduler::GetCurrentTimePeriod();
    const auto& slots = _dailySchedule.GetSlotsForPeriod(period);

    float durationMinutes = 5.0f; // Default 5 minutes
    for (const auto& slot : slots)
    {
        if (slot.activity == nextActivity)
        {
            durationMinutes = slot.durationMinutes;
            break;
        }
    }

    // Add some randomness to duration (+-25%)
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(0.75f, 1.25f);
    durationMinutes *= dist(rng);

    TransitionTo(nextState, nextActivity);
    _activityDurationMs = static_cast<uint32>(durationMinutes * 60.0f * 1000.0f);
    _activityDurationMs = std::clamp(_activityDurationMs, MIN_ACTIVITY_DURATION_MS, MAX_ACTIVITY_DURATION_MS);
}

void RPGDailyRoutineManager::TransitionTo(RPGState newState, ActivityType activity)
{
    if (newState == _currentState && activity == _currentActivity)
        return;

    TC_LOG_DEBUG("module.playerbot", "RPGDailyRoutine: Bot {} transitioning {} -> {} (activity: {})",
        _bot->GetGUID().GetCounter(),
        GetRPGStateName(_currentState),
        GetRPGStateName(newState),
        GetActivityName(activity));

    _currentState = newState;
    _currentActivity = activity;
    _activityStartTime = GameTime::GetGameTimeMS();
}

void RPGDailyRoutineManager::ProcessCurrentState(uint32 diff)
{
    switch (_currentState)
    {
        case RPGState::IDLE:        ProcessIdle(diff); break;
        case RPGState::CITY_LIFE:   ProcessCityLife(diff); break;
        case RPGState::GRINDING:    ProcessGrinding(diff); break;
        case RPGState::QUESTING:    ProcessQuesting(diff); break;
        case RPGState::GATHERING:   ProcessGathering(diff); break;
        case RPGState::EXPLORING:   ProcessExploring(diff); break;
        case RPGState::RESTING:     ProcessResting(diff); break;
        case RPGState::TRAVELING:   ProcessTraveling(diff); break;
        default: break;
    }
}

void RPGDailyRoutineManager::ProcessIdle(uint32 /*diff*/)
{
    // Idle: bot stands around, occasional emotes via BotIdleBehaviorManager
    // The existing idle behavior manager handles this
}

void RPGDailyRoutineManager::ProcessCityLife(uint32 /*diff*/)
{
    // City life: delegate to CityLifeBehaviorManager
    // The existing city life manager handles activities like
    // visiting auction house, bank, vendor, trainer, inn
}

void RPGDailyRoutineManager::ProcessGrinding(uint32 /*diff*/)
{
    // Grinding: bot should be killing nearby mobs
    // This is handled by the combat AI when enemies are nearby
    // If no enemies are found, the bot wanders looking for targets
    if (!_bot->IsInCombat())
    {
        // The bot's main AI loop handles target acquisition
        // We just need to be in an appropriate zone
    }
}

void RPGDailyRoutineManager::ProcessQuesting(uint32 /*diff*/)
{
    // Questing: delegate to quest automation systems
    // The existing QuestManager handles quest pickup, objectives, and turnin
}

void RPGDailyRoutineManager::ProcessGathering(uint32 /*diff*/)
{
    // Gathering: delegate to GatheringManager
    // Bots look for nearby mining/herb nodes and gather them
}

void RPGDailyRoutineManager::ProcessExploring(uint32 /*diff*/)
{
    // Exploring: bot wanders to undiscovered areas
    // Uses random movement towards unexplored map areas
    if (!_bot->isMoving())
    {
        // Select a random direction and move that way
        // The idle behavior manager's wandering handles this
    }
}

void RPGDailyRoutineManager::ProcessResting(uint32 /*diff*/)
{
    // Resting: bot sits down in a safe area
    // If at an inn, gets rested XP bonus
    if (!_bot->IsSitState())
    {
        _bot->SetStandState(UNIT_STAND_STATE_SIT);
    }
}

void RPGDailyRoutineManager::ProcessTraveling(uint32 /*diff*/)
{
    // Traveling: bot is moving to a destination zone
    // Uses flight paths if available, otherwise walks
    // When destination is reached, transitions to target activity
    if (!_bot->isMoving())
    {
        // Arrived at destination, switch to intended activity
        RPGState targetState = ActivityToRPGState(_currentActivity);
        if (targetState != RPGState::TRAVELING)
        {
            TransitionTo(targetState, _currentActivity);
        }
    }
}

void RPGDailyRoutineManager::ForceState(RPGState newState)
{
    TransitionTo(newState, _currentActivity);
    _activityDurationMs = MAX_ACTIVITY_DURATION_MS; // Give forced state full duration
}

void RPGDailyRoutineManager::RegenerateSchedule()
{
    // Use a default personality for now - in production, this would come from
    // the bot's HumanizationManager PersonalityProfile
    PersonalityProfile defaultPersonality;
    _dailySchedule = _scheduler.GenerateSchedule(defaultPersonality);
    _scheduleGenerated = true;

    TC_LOG_DEBUG("module.playerbot", "RPGDailyRoutine: Bot {} generated new daily schedule",
        _bot->GetGUID().GetCounter());
}

uint32 RPGDailyRoutineManager::GetActivityTimeRemaining() const
{
    if (_activityDurationMs == 0 || _activityStartTime == 0)
        return 0;

    uint32 now = GameTime::GetGameTimeMS();
    uint32 elapsed = now - _activityStartTime;
    if (elapsed >= _activityDurationMs)
        return 0;

    return _activityDurationMs - elapsed;
}

} // namespace Humanization
} // namespace Playerbot
