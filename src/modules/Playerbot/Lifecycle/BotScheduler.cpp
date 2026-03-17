/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>

#include "BotScheduler.h"
#include "Config/PlayerbotConfig.h"
#include "PlayerbotDatabase.h"
#include "BotSpawner.h"
#include "Log.h"
#include "Random.h"
#include "GameTime.h"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace Playerbot
{

BotScheduler* BotScheduler::instance()
{
    static BotScheduler instance;
    return &instance;
}

bool BotScheduler::Initialize()
{
    TC_LOG_INFO("module.playerbot.scheduler", "Initializing Bot Scheduler...");

    LoadConfig();
    LoadActivityPatterns();
    LoadBotSchedules();

    _lastUpdate = ::std::chrono::steady_clock::now();
    _lastDatabaseSync = ::std::chrono::steady_clock::now();

    TC_LOG_INFO("module.playerbot.scheduler",
        "Bot Scheduler initialized - {} patterns loaded, {} bots scheduled",
        _activityPatterns.size(), _botSchedules.size());

    return true;
}

void BotScheduler::Shutdown()
{
    TC_LOG_INFO("module.playerbot.scheduler", "Shutting down Bot Scheduler...");

    // Save all current schedules to database
    UpdateScheduleDatabase();

    // Clear data structures
    {
        ::std::lock_guard lock(_patternMutex);
        _activityPatterns.clear();
    }

    _botSchedules.clear();

    // Clear the schedule queue (TBB -> std::priority_queue)
    {
        ::std::lock_guard lock(_scheduleQueueMutex);
        while (!_scheduleQueue.empty())
        {
            _scheduleQueue.pop();
        }
    }

    TC_LOG_INFO("module.playerbot.scheduler", "Bot Scheduler shutdown complete");
}

void BotScheduler::Update(uint32 /*diff*/)
{
    if (!_config.enabled)
        return;

    auto currentTime = ::std::chrono::steady_clock::now();
    _lastUpdate = currentTime;

    // Process scheduled actions
    ProcessSchedule();

    // Update statistics
    _stats.queueSize.store(_scheduleQueue.size());

    // Periodic database synchronization
    if (::std::chrono::duration_cast<::std::chrono::milliseconds>(currentTime - _lastDatabaseSync).count() > DATABASE_SYNC_INTERVAL)
    {
        UpdateScheduleDatabase();
        _lastDatabaseSync = currentTime;
    }
}

void BotScheduler::LoadConfig()
{
    // Use default values until configuration system is fully resolved
    _config.enabled = true;
    _config.useRealisticPatterns = true;
    _config.lookaheadMinutes = 60;
    _config.heartbeatIntervalSeconds = 300;
    _config.idleTimeoutSeconds = 1800;
    _config.maxConcurrentActions = 1000;
    _config.peakHourMultiplier = 2.0f;
    _config.offPeakMultiplier = 0.5f;
    _config.peakStartHour = 18;
    _config.peakEndHour = 23;

    TC_LOG_DEBUG("module.playerbot.scheduler", "Loaded scheduler configuration");
}

void BotScheduler::LoadActivityPatterns()
{
    ::std::lock_guard lock(_patternMutex);

    // Load default patterns first
    LoadDefaultPatterns();

    // Load custom patterns from database
    LoadDatabasePatterns();

    TC_LOG_INFO("module.playerbot.scheduler",
        "Loaded {} activity patterns", _activityPatterns.size());
}

void BotScheduler::LoadDefaultPatterns()
{
    // Default pattern - balanced activity
    _activityPatterns["default"] = CreateDefaultPattern();

    // Casual pattern - lighter activity
    _activityPatterns["casual"] = CreateCasualPattern();

    // Hardcore pattern - heavy activity
    _activityPatterns["hardcore"] = CreateHardcorePattern();

    // Weekend pattern - weekend-focused activity
    _activityPatterns["weekend"] = CreateWeekendPattern();
}

ActivityPattern BotScheduler::CreateDefaultPattern() const
{
    ActivityPattern pattern;
    pattern.name = "default";
    pattern.activeHours = {{9, 12}, {14, 18}, {19, 23}}; // Morning, afternoon, evening
    pattern.activeDays = {1, 2, 3, 4, 5, 6, 7}; // All days
    pattern.loginProbability = 0.8f;
    pattern.minSessionDuration = 1800;  // 30 minutes
    pattern.maxSessionDuration = 7200;  // 2 hours
    pattern.averageSessionsPerDay = 2;
    pattern.preferPeakHours = true;
    pattern.weekendMultiplier = 1.2f;
    pattern.peakHourBonus = 1.5f;
    pattern.jitterMinutes = 15;
    return pattern;
}

ActivityPattern BotScheduler::CreateCasualPattern() const
{
    ActivityPattern pattern;
    pattern.name = "casual";
    pattern.activeHours = {{19, 22}}; // Evening only
    pattern.activeDays = {1, 2, 3, 4, 5, 6, 7}; // All days but lighter
    pattern.loginProbability = 0.5f;
    pattern.minSessionDuration = 900;   // 15 minutes
    pattern.maxSessionDuration = 3600;  // 1 hour
    pattern.averageSessionsPerDay = 1;
    pattern.preferPeakHours = true;
    pattern.weekendMultiplier = 1.8f;   // More active on weekends
    pattern.peakHourBonus = 2.0f;
    pattern.jitterMinutes = 30;
    return pattern;
}

ActivityPattern BotScheduler::CreateHardcorePattern() const
{
    ActivityPattern pattern;
    pattern.name = "hardcore";
    pattern.activeHours = {{8, 12}, {13, 18}, {19, 24}}; // Extended hours
    pattern.activeDays = {1, 2, 3, 4, 5, 6, 7}; // Every day
    pattern.loginProbability = 0.9f;
    pattern.minSessionDuration = 3600;  // 1 hour
    pattern.maxSessionDuration = 14400; // 4 hours
    pattern.averageSessionsPerDay = 3;
    pattern.preferPeakHours = false;    // Play anytime
    pattern.weekendMultiplier = 1.5f;
    pattern.peakHourBonus = 1.2f;
    pattern.jitterMinutes = 10;         // More predictable
    return pattern;
}

ActivityPattern BotScheduler::CreateWeekendPattern() const
{
    ActivityPattern pattern;
    pattern.name = "weekend";
    pattern.activeHours = {{10, 14}, {16, 20}, {21, 24}}; // Relaxed schedule
    pattern.activeDays = {1, 6, 7}; // Friday, Saturday, Sunday only
    pattern.loginProbability = 0.7f;
    pattern.minSessionDuration = 2700;  // 45 minutes
    pattern.maxSessionDuration = 10800; // 3 hours
    pattern.averageSessionsPerDay = 2;
    pattern.preferPeakHours = false;
    pattern.weekendMultiplier = 1.0f;   // Already weekend-focused
    pattern.peakHourBonus = 1.3f;
    pattern.jitterMinutes = 45;
    return pattern;
}

void BotScheduler::RegisterPattern(::std::string const& name, ActivityPattern const& pattern)
{
    ::std::lock_guard lock(_patternMutex);
    _activityPatterns[name] = pattern;

    // Save to database for persistence
    SavePatternToDatabase(name, pattern);

    TC_LOG_DEBUG("module.playerbot.scheduler",
        "Registered activity pattern '{}'", name);
}

ActivityPattern const* BotScheduler::GetPattern(::std::string const& name) const
{
    ::std::lock_guard lock(_patternMutex);
    auto it = _activityPatterns.find(name);
    return it != _activityPatterns.end() ? &it->second : nullptr;
}

void BotScheduler::ScheduleBot(ObjectGuid guid, ::std::string const& patternName)
{
    // Verify pattern exists
    if (!GetPattern(patternName))
    {
        TC_LOG_ERROR("module.playerbot.scheduler",
            "Cannot schedule bot {} with unknown pattern '{}'", guid.ToString(), patternName);
        return;
    }

    // Create or update bot schedule state
    BotScheduleState state;
    state.guid = guid;
    state.patternName = patternName;
    state.isScheduled = true;
    state.isActive = false;
    state.lastActivity = ::std::chrono::system_clock::now();

    // Calculate initial schedule
    state.nextLogin = CalculateNextLogin(guid);

    // Store the state
    _botSchedules[guid] = state;

    // Schedule the first login
    ScheduleLogin(guid, state.nextLogin);

    // Save to database
    SaveBotSchedule(state);

    _stats.totalScheduled.fetch_add(1);
    _stats.activeSchedules.fetch_add(1);

    TC_LOG_INFO("module.playerbot.scheduler",
        "Scheduled bot {} with pattern '{}', next login: {}",
        guid.ToString(), patternName,
        ::std::chrono::duration_cast<::std::chrono::seconds>(state.nextLogin.time_since_epoch()).count());
}

void BotScheduler::UnscheduleBot(ObjectGuid guid)
{
    auto it = _botSchedules.find(guid);
    if (it == _botSchedules.end())
        return;

    BotScheduleState& state = it->second;
    state.isScheduled = false;
    state.isActive = false;

    _stats.activeSchedules.fetch_sub(1);

    TC_LOG_DEBUG("module.playerbot.scheduler",
        "Unscheduled bot {}", guid.ToString());
}

::std::chrono::system_clock::time_point BotScheduler::CalculateNextLogin(ObjectGuid guid)
{
    auto it = _botSchedules.find(guid);
    if (it == _botSchedules.end())
        return ::std::chrono::system_clock::now();

    BotScheduleState const& state = it->second;
    ActivityPattern const* pattern = GetPattern(state.patternName);
    if (!pattern)
        return ::std::chrono::system_clock::now();

    auto now = ::std::chrono::system_clock::now();
    auto currentTime = ::std::chrono::system_clock::to_time_t(now);
    auto* tm = ::std::localtime(&currentTime);

    // Find next active period
    for (uint32 dayOffset = 0; dayOffset < 7; ++dayOffset)
    {
        uint32 dayOfWeek = (tm->tm_wday + dayOffset) % 7;
        if (dayOfWeek == 0) dayOfWeek = 7; // Convert Sunday from 0 to 7

        // Check if this day is active
        bool isDayActive = ::std::find(pattern->activeDays.begin(), pattern->activeDays.end(), dayOfWeek) != pattern->activeDays.end();
        if (!isDayActive)
            continue;

        // Check active hours for this day
    for (auto const& hourRange : pattern->activeHours)
        {
            uint32 startHour = hourRange.first;
            uint32 endHour = hourRange.second;

            // Calculate the target time
            auto targetTime = now + ::std::chrono::hours(24 * dayOffset);
            auto targetTimeT = ::std::chrono::system_clock::to_time_t(targetTime);
            auto* targetTm = ::std::localtime(&targetTimeT);

            targetTm->tm_hour = startHour;
            targetTm->tm_min = urand(0, 59);
            targetTm->tm_sec = urand(0, 59);

            auto loginTime = ::std::chrono::system_clock::from_time_t(::std::mktime(targetTm));

            // Apply probability check
    if (frand(0.0f, 1.0f) > pattern->loginProbability)
                continue;

            // Apply time multiplier and jitter
            float multiplier = GetTimeMultiplier(loginTime, *pattern);
            if (frand(0.0f, 1.0f) > multiplier)
                continue;

            // Add jitter
            loginTime = AddJitter(loginTime, pattern->jitterMinutes);

            // Must be in the future
    if (loginTime > now)
                return loginTime;
        }
    }

    // Fallback: schedule for tomorrow at a random active time
    auto tomorrow = now + ::std::chrono::hours(24);
    return AddJitter(tomorrow, pattern->jitterMinutes);
}

::std::chrono::system_clock::time_point BotScheduler::CalculateSessionEnd(ObjectGuid guid, uint32 minDuration, uint32 maxDuration)
{
    auto now = ::std::chrono::system_clock::now();
    uint32 duration = urand(minDuration, maxDuration);
    return now + ::std::chrono::seconds(duration);
}

void BotScheduler::ScheduleLogin(ObjectGuid guid, ::std::chrono::system_clock::time_point when)
{
    ScheduleEntry entry;
    entry.botGuid = guid;
    entry.executeTime = when;
    entry.action = ScheduleEntry::LOGIN;

    ScheduleAction(entry);
}

void BotScheduler::ScheduleLogout(ObjectGuid guid, ::std::chrono::system_clock::time_point when)
{
    ScheduleEntry entry;
    entry.botGuid = guid;
    entry.executeTime = when;
    entry.action = ScheduleEntry::LOGOUT;

    ScheduleAction(entry);
}

void BotScheduler::ScheduleAction(ScheduleEntry const& entry)
{
    if (_scheduleQueue.size() >= _config.maxConcurrentActions)
    {
        TC_LOG_WARN("module.playerbot.scheduler",
            "Schedule queue full ({} actions), dropping action for bot {}",
            _config.maxConcurrentActions, entry.botGuid.ToString());
        return;
    }

    _scheduleQueue.push(entry);

    TC_LOG_DEBUG("module.playerbot.scheduler",
        "Scheduled action {} for bot {} at time {}",
        static_cast<int>(entry.action), entry.botGuid.ToString(),
        ::std::chrono::duration_cast<::std::chrono::seconds>(entry.executeTime.time_since_epoch()).count());
}

void BotScheduler::ProcessSchedule()
{
    auto now = ::std::chrono::system_clock::now();
    uint32 actionsProcessed = 0;

    // Replace TBB try_pop with std::priority_queue + mutex
    while (actionsProcessed < MAX_ACTIONS_PER_UPDATE)
    {
        ScheduleEntry entry;
        {
            ::std::lock_guard lock(_scheduleQueueMutex);
            if (_scheduleQueue.empty())
                break;

            entry = _scheduleQueue.top();
            _scheduleQueue.pop();
        } // End of lock scope

        // Process the entry
    if (entry.executeTime <= now)
        {
            ExecuteScheduledAction(entry);
            ++actionsProcessed;
        }
        else
        {
            // Put it back - not time yet
            _scheduleQueue.push(entry);
            break;
        }
    }

    if (actionsProcessed > 0)
    {
        TC_LOG_DEBUG("module.playerbot.scheduler",
            "Processed {} scheduled actions", actionsProcessed);
    }
}

void BotScheduler::ExecuteScheduledAction(ScheduleEntry const& entry)
{
    auto startTime = ::std::chrono::high_resolution_clock::now();

    switch (entry.action)
    {
        case ScheduleEntry::LOGIN:
        {
            // Attempt to spawn the bot
            SpawnRequest request;
            request.type = SpawnRequest::SPECIFIC_CHARACTER;
            request.characterGuid = entry.botGuid;

            bool success = sBotSpawner->SpawnBot(request);
            if (success)
            {
                // Update bot state
                auto it = _botSchedules.find(entry.botGuid);
                if (it != _botSchedules.end())
                {
                    BotScheduleState& state = it->second;
                    state.isActive = true;
                    state.lastActivity = ::std::chrono::system_clock::now();
                    ++state.totalSessions;

                    // Calculate session end time
                    ActivityPattern const* pattern = GetPattern(state.patternName);
                    if (pattern)
                    {
                        state.nextLogout = CalculateSessionEnd(entry.botGuid,
                            pattern->minSessionDuration, pattern->maxSessionDuration);
                        ScheduleLogout(entry.botGuid, state.nextLogout);
                    }
                }
                _stats.loginActions.fetch_add(1);
            }
            else
            {
                _stats.missedActions.fetch_add(1);
                TC_LOG_WARN("module.playerbot.scheduler",
                    "Failed to execute login for bot {}", entry.botGuid.ToString());
            }
            break;
        }

        case ScheduleEntry::LOGOUT:
        {
            // Despawn the bot
            sBotSpawner->DespawnBot(entry.botGuid);

            // Update bot state and schedule next login
            auto it = _botSchedules.find(entry.botGuid);
            if (it != _botSchedules.end())
            {
                BotScheduleState& state = it->second;
                state.isActive = false;

                // Calculate playtime for this session
                auto sessionTime = ::std::chrono::duration_cast<::std::chrono::seconds>(
                    ::std::chrono::system_clock::now() - state.lastActivity).count();
                state.totalPlaytime += sessionTime;

                // Schedule next login
                state.nextLogin = CalculateNextLogin(entry.botGuid);
                ScheduleLogin(entry.botGuid, state.nextLogin);
            }
            _stats.logoutActions.fetch_add(1);
            break;
        }

        case ScheduleEntry::HEARTBEAT:
            // Verify bot is still active and responsive
            // This could check if the bot session is still valid
            break;

        default:
            TC_LOG_WARN("module.playerbot.scheduler",
                "Unknown scheduled action type: {}", static_cast<int>(entry.action));
            break;
    }

    auto endTime = ::std::chrono::high_resolution_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime);

    // Update execution time statistics
    uint64 currentAvg = _stats.averageExecutionTime.load();
    uint64 newAvg = (currentAvg + duration.count()) / 2;
    _stats.averageExecutionTime.store(newAvg);

    _stats.totalExecuted.fetch_add(1);

    TC_LOG_DEBUG("module.playerbot.scheduler",
        "Executed action {} for bot {} in {:.2f}ms",
        static_cast<int>(entry.action), entry.botGuid.ToString(), duration.count() / 1000.0f);
}

bool BotScheduler::IsPeakHour(uint32 hour) const
{
    return hour >= _config.peakStartHour && hour <= _config.peakEndHour;
}

float BotScheduler::GetTimeMultiplier(::std::chrono::system_clock::time_point time, ActivityPattern const& pattern) const
{
    auto timeT = ::std::chrono::system_clock::to_time_t(time);
    auto* tm = ::std::localtime(&timeT);

    float multiplier = 1.0f;

    // Peak hour bonus
    if (pattern.preferPeakHours && IsPeakHour(tm->tm_hour))
    {
        multiplier *= pattern.peakHourBonus;
    }

    // Weekend multiplier
    if (tm->tm_wday == 0 || tm->tm_wday == 6) // Sunday or Saturday
    {
        multiplier *= pattern.weekendMultiplier;
    }

    return ::std::min(multiplier, 5.0f); // Cap at 5x
}

::std::chrono::system_clock::time_point BotScheduler::AddJitter(::std::chrono::system_clock::time_point time, uint32 jitterMinutes) const
{
    if (jitterMinutes == 0)
        return time;

    int32 jitter = urand(0, jitterMinutes * 2) - jitterMinutes; // +/- jitter
    return time + ::std::chrono::minutes(jitter);
}

void BotScheduler::LoadBotSchedules()
{
    // This would load existing bot schedules from the database
    // For now, this is a placeholder
    TC_LOG_DEBUG("module.playerbot.scheduler", "Loading bot schedules from database...");
}

void BotScheduler::SaveBotSchedule(BotScheduleState const& state)
{
    // This would save the bot schedule to the database
    // For now, this is a placeholder
    TC_LOG_DEBUG("module.playerbot.scheduler",
        "Saving schedule for bot {} to database", state.guid.ToString());
}

void BotScheduler::LoadDatabasePatterns()
{
    // This would load custom patterns from the database
    // For now, this is a placeholder
    TC_LOG_DEBUG("module.playerbot.scheduler", "Loading patterns from database...");
}

void BotScheduler::SavePatternToDatabase(::std::string const& name, ActivityPattern const& pattern)
{
    // This would save the pattern to the database
    // For now, this is a placeholder
    TC_LOG_DEBUG("module.playerbot.scheduler",
        "Saving pattern '{}' to database", name);
}

void BotScheduler::UpdateScheduleDatabase()
{
    // Periodic database synchronization
    TC_LOG_DEBUG("module.playerbot.scheduler", "Synchronizing schedules with database");
}

void BotScheduler::ResetStats()
{
    _stats.totalScheduled.store(0);
    _stats.totalExecuted.store(0);
    _stats.loginActions.store(0);
    _stats.logoutActions.store(0);
    _stats.missedActions.store(0);
    _stats.averageExecutionTime.store(0);
    _stats.activeSchedules.store(0);
    _stats.queueSize.store(0);

    TC_LOG_INFO("module.playerbot.scheduler", "Scheduler statistics reset");
}

uint32 BotScheduler::GetScheduledBotCount() const
{
    return _stats.activeSchedules.load();
}

bool BotScheduler::IsBotScheduled(ObjectGuid guid) const
{
    auto it = _botSchedules.find(guid);
    return it != _botSchedules.end() && it->second.isScheduled;
}

bool BotScheduler::IsBotActive(ObjectGuid guid) const
{
    auto it = _botSchedules.find(guid);
    return it != _botSchedules.end() && it->second.isActive;
}

::std::vector<ScheduledAction> BotScheduler::GetBotsReadyForLogin(uint32 maxCount)
{
    ::std::vector<ScheduledAction> actions;
    actions.reserve(maxCount);

    auto now = ::std::chrono::system_clock::now();
    uint32 count = 0;

    for (auto& [guid, schedule] : _botSchedules)
    {
        if (count >= maxCount)
            break;

        if (!schedule.isScheduled || schedule.isActive)
            continue;

        // Check if bot is ready for login
    if (schedule.nextLogin <= now && !schedule.nextLogin.time_since_epoch().count() == 0)
        {
            ScheduledAction action;
            action.action = ScheduleEntry::LOGIN;
            action.botGuid = guid;
            action.when = schedule.nextLogin;
            action.patternName = schedule.patternName;

            actions.push_back(action);
            count++;
        }
    }

    // Sort by scheduled time (earliest first)
    ::std::sort(actions.begin(), actions.end(),
        [](const ScheduledAction& a, const ScheduledAction& b) {
            return a.when < b.when;
        });

    return actions;
}

::std::vector<ScheduledAction> BotScheduler::GetBotsReadyForLogout(uint32 maxCount)
{
    ::std::vector<ScheduledAction> actions;
    actions.reserve(maxCount);

    auto now = ::std::chrono::system_clock::now();
    uint32 count = 0;

    for (auto& [guid, schedule] : _botSchedules)
    {
        if (count >= maxCount)
            break;

        if (!schedule.isScheduled || !schedule.isActive)
            continue;

        // Check if bot is ready for logout
    if (schedule.nextLogout <= now && !schedule.nextLogout.time_since_epoch().count() == 0)
        {
            ScheduledAction action;
            action.action = ScheduleEntry::LOGOUT;
            action.botGuid = guid;
            action.when = schedule.nextLogout;
            action.patternName = schedule.patternName;

            actions.push_back(action);
            count++;
        }
    }

    // Sort by scheduled time (earliest first)
    ::std::sort(actions.begin(), actions.end(),
        [](const ScheduledAction& a, const ScheduledAction& b) {
            return a.when < b.when;
        });

    return actions;
}

void BotScheduler::OnBotLoggedIn(ObjectGuid guid)
{
    auto it = _botSchedules.find(guid);
    if (it == _botSchedules.end())
        return;

    auto& schedule = it->second;
    schedule.isActive = true;
    schedule.lastLogin = ::std::chrono::system_clock::now();
    schedule.currentSessionStart = schedule.lastLogin;
    schedule.totalSessions++;

    // Clear next login time and calculate next logout
    schedule.nextLogin = ::std::chrono::system_clock::time_point{};

    // Calculate session duration based on pattern
    if (auto patternIt = _activityPatterns.find(schedule.patternName); patternIt != _activityPatterns.end())
    {
        auto& pattern = patternIt->second;
        uint32 sessionDuration = urand(pattern.minSessionDuration, pattern.maxSessionDuration);
        schedule.nextLogout = schedule.lastLogin + ::std::chrono::seconds(sessionDuration);
    }
    else
    {
        // Default 1-2 hour session
        uint32 sessionDuration = urand(3600, 7200);
        schedule.nextLogout = schedule.lastLogin + ::std::chrono::seconds(sessionDuration);
    }

    // Save to database
    SaveBotSchedule(schedule);

    TC_LOG_DEBUG("module.playerbot.scheduler", "Bot {} logged in, next logout: {}",
        guid.ToString(), ::std::chrono::duration_cast<::std::chrono::seconds>(
            schedule.nextLogout.time_since_epoch()).count());
}

void BotScheduler::OnBotLoginFailed(ObjectGuid guid, ::std::string const& reason)
{
    auto it = _botSchedules.find(guid);
    if (it == _botSchedules.end())
        return;

    auto& schedule = it->second;
    schedule.consecutiveFailures++;
    schedule.lastFailureReason = reason;

    // Exponential backoff for retry
    uint32 retryDelay = ::std::min(300u * (1u << ::std::min(schedule.consecutiveFailures, 8u)), 3600u); // Max 1 hour
    schedule.nextRetry = ::std::chrono::system_clock::now() + ::std::chrono::seconds(retryDelay);

    // Don't retry immediately, wait for retry time
    schedule.nextLogin = schedule.nextRetry;

    // Save to database
    SaveBotSchedule(schedule);

    TC_LOG_WARN("module.playerbot.scheduler", "Bot {} login failed ({}), retry in {} seconds",
        guid.ToString(), reason, retryDelay);
}

} // namespace Playerbot