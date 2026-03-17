/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ActivitySessionManager.h"
#include "SessionTransitions.h"
#include "Player.h"
#include "Log.h"
#include <algorithm>
#include <chrono>
#include <ctime>

namespace Playerbot
{
namespace Humanization
{

// ============================================================================
// ActivitySessionManager Implementation
// ============================================================================

ActivitySessionManager::ActivitySessionManager(Player* bot)
    : _bot(bot)
    , _botGuid(bot ? bot->GetGUID() : ObjectGuid::Empty)
    , _activityExecutor(std::make_unique<ActivityExecutor>(bot))
    , _sessionTransitions(std::make_unique<SessionTransitions>(bot))
    , _personality(PersonalityType::CASUAL)
{
}

ActivitySessionManager::~ActivitySessionManager()
{
    Shutdown();
}

void ActivitySessionManager::Initialize()
{
    if (_initialized)
        return;

    if (!_bot)
    {
        TC_LOG_ERROR("module.playerbot.humanization",
            "ActivitySessionManager::Initialize - No bot reference!");
        return;
    }

    // Assign random personality if configured
    if (sHumanizationConfig.AssignRandomPersonalities())
    {
        _personality = PersonalityProfile::CreateRandomProfile();
    }

    // Initialize activity executor
    if (_activityExecutor)
    {
        _activityExecutor->Initialize();
    }

    // Initialize session transitions
    if (_sessionTransitions)
    {
        _sessionTransitions->Initialize();
    }

    // Reset daily tracking
    _lastDayReset = std::chrono::system_clock::now();
    _todaySessionCount = 0;

    _initialized = true;

    TC_LOG_DEBUG("module.playerbot.humanization",
        "ActivitySessionManager::Initialize - Bot {} initialized with {} personality",
        _bot->GetName(), PersonalityProfile::GetTypeName(_personality.GetType()));
}

void ActivitySessionManager::Update(uint32 diff)
{
    if (!_initialized || !_bot || !_bot->IsInWorld())
        return;

    // Throttle updates
    _updateTimer += diff;
    if (_updateTimer < UPDATE_INTERVAL)
        return;

    uint32 elapsed = _updateTimer;
    _updateTimer = 0;

    // Check for day change
    auto now = std::chrono::system_clock::now();
    auto nowTime = std::chrono::system_clock::to_time_t(now);
    auto lastTime = std::chrono::system_clock::to_time_t(_lastDayReset);
    std::tm* nowTm = std::localtime(&nowTime);
    std::tm* lastTm = std::localtime(&lastTime);

    if (nowTm->tm_yday != lastTm->tm_yday)
    {
        _todaySessionCount = 0;
        _lastDayReset = now;
    }

    // Handle break state
    if (_isOnBreak)
    {
        CheckBreakExpiry();
        return;
    }

    // Handle active session
    if (_currentSession && _currentSession->IsActive())
    {
        CheckSessionExpiry();
    }

    // Process transition queue
    ProcessTransitionQueue();

    // Track idle time
    if (!HasActiveSession() && !_isOnBreak)
    {
        _idleTimeMs += elapsed;

        // Auto-start activity if idle too long
        if (_idleTimeMs > 30000) // 30 seconds idle
        {
            ActivityType nextActivity = SelectNextActivity();
            if (nextActivity != ActivityType::NONE)
            {
                StartSession(nextActivity);
            }
        }
    }
}

void ActivitySessionManager::Shutdown()
{
    if (_currentSession && _currentSession->IsActive())
    {
        EndSession(SessionEndReason::MANUAL_STOP);
    }

    // Shutdown activity executor
    if (_activityExecutor)
    {
        _activityExecutor->Shutdown();
    }

    // Shutdown session transitions
    if (_sessionTransitions)
    {
        _sessionTransitions->Shutdown();
    }

    _currentSession.reset();
    _initialized = false;
}

bool ActivitySessionManager::HasActiveSession() const
{
    return _currentSession && _currentSession->IsActive();
}

ActivityType ActivitySessionManager::GetCurrentActivity() const
{
    return _currentSession ? _currentSession->GetActivityType() : ActivityType::NONE;
}

ActivityCategory ActivitySessionManager::GetCurrentCategory() const
{
    return _currentSession ? _currentSession->GetCategory() : ActivityCategory::IDLE;
}

bool ActivitySessionManager::StartSession(ActivityType activity, uint32 durationMs)
{
    if (!_initialized)
        return false;

    // End current session if exists
    if (_currentSession && _currentSession->IsActive())
    {
        EndSession(SessionEndReason::TRANSITION);
    }

    // Calculate duration if not specified
    if (durationMs == 0)
    {
        durationMs = CalculateSessionDuration(activity);
    }

    // Create new session
    _currentSession = std::make_unique<ActivitySession>(_botGuid, activity, durationMs);

    if (!_currentSession->Start())
    {
        TC_LOG_WARN("module.playerbot.humanization",
            "ActivitySessionManager::StartSession - Failed to start session for bot {}",
            _bot ? _bot->GetName() : "unknown");
        _currentSession.reset();
        return false;
    }

    // Execute the actual activity behavior
    if (_activityExecutor)
    {
        ActivityExecutionContext context(activity, durationMs);
        ActivityExecutionResult result = _activityExecutor->StartActivity(context);

        if (result != ActivityExecutionResult::SUCCESS &&
            result != ActivityExecutionResult::NOT_IMPLEMENTED)
        {
            // Log but don't fail the session - some activities are just tracking
            TC_LOG_DEBUG("module.playerbot.humanization",
                "ActivitySessionManager::StartSession - Activity {} execution result: {}",
                GetActivityName(activity),
                static_cast<uint8>(result));
        }
    }

    // Update metrics
    _metrics.totalSessions++;
    _todaySessionCount++;
    _idleTimeMs = 0;

    TC_LOG_DEBUG("module.playerbot.humanization",
        "ActivitySessionManager::StartSession - Bot {} started {} session ({}ms)",
        _bot ? _bot->GetName() : "unknown",
        GetActivityName(activity),
        durationMs);

    return true;
}

void ActivitySessionManager::EndSession(SessionEndReason reason)
{
    if (!_currentSession)
        return;

    // Stop the activity behavior
    if (_activityExecutor)
    {
        _activityExecutor->StopActivity(_currentSession->GetActivityType());
    }

    switch (reason)
    {
        case SessionEndReason::DURATION_EXPIRED:
        case SessionEndReason::GOAL_ACHIEVED:
        case SessionEndReason::BAGS_FULL:
        case SessionEndReason::TRANSITION:
        case SessionEndReason::MANUAL_STOP:
            _currentSession->Complete(reason);
            _metrics.completedSessions++;
            break;

        case SessionEndReason::INTERRUPTED_COMBAT:
        case SessionEndReason::INTERRUPTED_USER:
        case SessionEndReason::INTERRUPTED_DEATH:
        case SessionEndReason::LOCATION_CHANGED:
        case SessionEndReason::RESOURCE_DEPLETED:
            _currentSession->Interrupt(reason);
            _metrics.interruptedSessions++;
            break;

        case SessionEndReason::SESSION_ERROR:
            _currentSession->Fail(reason);
            _metrics.failedSessions++;
            break;

        default:
            _currentSession->Complete(reason);
            break;
    }

    // Update total active time
    _metrics.totalActiveTimeMs += _currentSession->GetElapsedMs();

    // Archive to history
    ArchiveCurrentSession();

    TC_LOG_DEBUG("module.playerbot.humanization",
        "ActivitySessionManager::EndSession - Bot {} ended {} session (reason: {})",
        _bot ? _bot->GetName() : "unknown",
        GetActivityName(_currentSession->GetActivityType()),
        static_cast<uint8>(reason));

    _currentSession.reset();
}

bool ActivitySessionManager::PauseSession(std::string const& reason)
{
    if (!_currentSession || !_currentSession->IsActive())
        return false;

    return _currentSession->Pause(reason);
}

bool ActivitySessionManager::ResumeSession()
{
    if (!_currentSession || !_currentSession->IsPaused())
        return false;

    return _currentSession->Resume();
}

bool ActivitySessionManager::RequestTransition(SessionTransitionRequest const& request)
{
    _transitionQueue.push(request);
    return true;
}

void ActivitySessionManager::ForceTransition(ActivityType activity, uint32 durationMs)
{
    EndSession(SessionEndReason::TRANSITION);
    StartSession(activity, durationMs);
}

bool ActivitySessionManager::StartBreak(uint32 durationMs)
{
    if (_isOnBreak)
        return false;

    // End current session if any
    if (_currentSession && _currentSession->IsActive())
    {
        PauseSession("Break");
    }

    // Calculate break duration
    if (durationMs == 0)
    {
        durationMs = _personality.CalculateBreakDuration();
    }

    _isOnBreak = true;
    _breakStartTime = std::chrono::steady_clock::now();
    _breakDurationMs = durationMs;
    _metrics.totalBreaks++;

    TC_LOG_DEBUG("module.playerbot.humanization",
        "ActivitySessionManager::StartBreak - Bot {} starting {}ms break",
        _bot ? _bot->GetName() : "unknown",
        durationMs);

    return true;
}

void ActivitySessionManager::EndBreak()
{
    if (!_isOnBreak)
        return;

    auto breakDuration = std::chrono::steady_clock::now() - _breakStartTime;
    _metrics.totalBreakTimeMs += static_cast<uint64>(
        std::chrono::duration_cast<std::chrono::milliseconds>(breakDuration).count());

    _isOnBreak = false;

    // Resume session if was paused for break
    if (_currentSession && _currentSession->IsPaused())
    {
        ResumeSession();
    }

    TC_LOG_DEBUG("module.playerbot.humanization",
        "ActivitySessionManager::EndBreak - Bot {} ended break",
        _bot ? _bot->GetName() : "unknown");
}

uint32 ActivitySessionManager::GetRemainingBreakMs() const
{
    if (!_isOnBreak)
        return 0;

    auto elapsed = std::chrono::steady_clock::now() - _breakStartTime;
    uint32 elapsedMs = static_cast<uint32>(
        std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());

    return elapsedMs < _breakDurationMs ? _breakDurationMs - elapsedMs : 0;
}

bool ActivitySessionManager::ShouldTakeBreak() const
{
    if (_isOnBreak)
        return false;

    if (!_currentSession)
        return false;

    return _personality.ShouldTakeBreak(_currentSession->GetElapsedMs());
}

void ActivitySessionManager::SetPersonality(PersonalityProfile const& profile)
{
    _personality = profile;
}

ActivityType ActivitySessionManager::SelectNextActivity() const
{
    // Use SessionTransitions for intelligent activity selection
    if (_sessionTransitions)
    {
        ActivityType current = GetCurrentActivity();
        ActivityType suggested = _sessionTransitions->SuggestNextActivity(current, _personality);

        if (suggested != ActivityType::NONE)
            return suggested;
    }

    // Fallback to personality-weighted selection
    std::vector<ActivityType> available = GetAvailableActivities();

    if (available.empty())
        return ActivityType::NONE;

    return _personality.SelectWeightedActivity(available);
}

std::vector<ActivityType> ActivitySessionManager::GetAvailableActivities() const
{
    std::vector<ActivityType> available;

    // Add activities based on bot state and location
    // This is a simplified version - full implementation would check:
    // - Bot location (city, wilderness, dungeon)
    // - Bot level and skills
    // - Time since last activity
    // - Current objectives

    // Always available
    available.push_back(ActivityType::STANDING_IDLE);
    available.push_back(ActivityType::WALKING);

    // Questing (if bot has quests)
    available.push_back(ActivityType::QUEST_OBJECTIVE);
    available.push_back(ActivityType::QUEST_TRAVEL);

    // Gathering (if bot has profession)
    available.push_back(ActivityType::MINING);
    available.push_back(ActivityType::HERBALISM);

    // City life (if in city - simplified check)
    available.push_back(ActivityType::AUCTION_BROWSING);
    available.push_back(ActivityType::MAILBOX_CHECK);
    available.push_back(ActivityType::VENDOR_VISIT);

    // Combat
    available.push_back(ActivityType::SOLO_COMBAT);

    // Exploration
    available.push_back(ActivityType::ZONE_EXPLORATION);

    return available;
}

uint32 ActivitySessionManager::CalculateSessionDuration(ActivityType activity) const
{
    uint32 baseMin = GetDefaultDuration(activity);
    uint32 baseMax = baseMin * 2;

    return _personality.CalculateSessionDuration(activity, baseMin, baseMax);
}

std::vector<SessionHistoryEntry> ActivitySessionManager::GetHistory(uint32 maxEntries) const
{
    if (maxEntries == 0 || maxEntries >= _history.size())
        return _history;

    return std::vector<SessionHistoryEntry>(_history.begin(), _history.begin() + maxEntries);
}

uint32 ActivitySessionManager::GetTotalTimeInCategory(ActivityCategory category) const
{
    uint32 total = 0;
    for (auto const& entry : _history)
    {
        if (GetActivityCategory(entry.activityType) == category)
            total += entry.durationMs;
    }
    return total;
}

void ActivitySessionManager::ClearHistory()
{
    _history.clear();
}

void ActivitySessionManager::ProcessTransitionQueue()
{
    if (_transitionQueue.empty())
        return;

    // Don't process if session is still active and not immediate
    if (_currentSession && _currentSession->IsActive())
    {
        auto const& front = _transitionQueue.front();
        if (!front.immediate)
            return;
    }

    SessionTransitionRequest request = _transitionQueue.front();
    _transitionQueue.pop();

    if (request.immediate && _currentSession && _currentSession->IsActive())
    {
        EndSession(SessionEndReason::TRANSITION);
    }

    StartSession(request.targetActivity, request.targetDurationMs);
}

void ActivitySessionManager::CheckSessionExpiry()
{
    if (!_currentSession || !_currentSession->IsActive())
        return;

    // Check if duration expired
    if (_currentSession->GetRemainingMs() == 0)
    {
        // Check for extension chance
        auto const& sessionConfig = sHumanizationConfig.GetSessionConfig();
        if (_currentSession->GetExtensionCount() < sessionConfig.maxExtensions)
        {
            std::random_device rd;
            std::mt19937 rng(rd());
            std::uniform_int_distribution<uint32> dist(0, 99);

            if (dist(rng) < sessionConfig.extendChancePercent)
            {
                // Extend session
                uint32 extension = _personality.CalculateSessionDuration(
                    _currentSession->GetActivityType(),
                    sessionConfig.minDurationMs / 2,
                    sessionConfig.maxDurationMs / 2);
                _currentSession->Extend(extension);

                TC_LOG_DEBUG("module.playerbot.humanization",
                    "ActivitySessionManager::CheckSessionExpiry - Bot {} extended session by {}ms",
                    _bot ? _bot->GetName() : "unknown",
                    extension);
                return;
            }
        }

        EndSession(SessionEndReason::DURATION_EXPIRED);
    }

    // Check for break
    if (ShouldTakeBreak())
    {
        StartBreak();
    }
}

void ActivitySessionManager::CheckBreakExpiry()
{
    if (!_isOnBreak)
        return;

    if (GetRemainingBreakMs() == 0)
    {
        EndBreak();
    }
}

void ActivitySessionManager::ArchiveCurrentSession()
{
    if (!_currentSession)
        return;

    SessionHistoryEntry entry;
    entry.sessionId = _currentSession->GetSessionId();
    entry.activityType = _currentSession->GetActivityType();
    entry.startTime = _currentSession->GetStartTime();
    entry.endTime = _currentSession->GetEndTime();
    entry.endReason = _currentSession->GetEndReason();
    entry.durationMs = _currentSession->GetElapsedMs();
    entry.progressValue = _currentSession->GetProgressValue();

    // Add to front (newest first)
    _history.insert(_history.begin(), entry);

    // Trim history
    if (_history.size() > MAX_HISTORY_SIZE)
    {
        _history.resize(MAX_HISTORY_SIZE);
    }
}

uint32 ActivitySessionManager::GetDefaultDuration(ActivityType activity) const
{
    // Get category and use config
    ActivityCategory category = GetActivityCategory(activity);
    uint32 minDuration = sHumanizationConfig.GetActivityMinDuration(category);

    // Special handling for some activities
    switch (activity)
    {
        case ActivityType::MINING:
        case ActivityType::HERBALISM:
            return sHumanizationConfig.GetGatheringMinDuration();

        case ActivityType::FISHING:
            return sHumanizationConfig.GetFishingMinDuration();

        case ActivityType::AUCTION_BROWSING:
        case ActivityType::CITY_WANDERING:
        case ActivityType::INN_REST:
            return sHumanizationConfig.GetCityLifeMinDuration();

        case ActivityType::AFK_SHORT:
            return sHumanizationConfig.GetAFKConfig().shortAFKMinMs;
        case ActivityType::AFK_MEDIUM:
            return sHumanizationConfig.GetAFKConfig().mediumAFKMinMs;
        case ActivityType::AFK_LONG:
            return sHumanizationConfig.GetAFKConfig().longAFKMinMs;

        default:
            return minDuration;
    }
}

bool ActivitySessionManager::IsActivityAvailable(ActivityType activity) const
{
    // Check if activity is enabled in config
    ActivityCategory category = GetActivityCategory(activity);
    return sHumanizationConfig.IsActivityEnabled(category);
}

} // namespace Humanization
} // namespace Playerbot
