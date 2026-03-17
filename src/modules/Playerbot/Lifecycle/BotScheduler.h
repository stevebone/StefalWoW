/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "ObjectGuid.h"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <chrono>
#include <functional>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <queue>

namespace Playerbot
{

// Scheduled action entry with time-based priority
struct ScheduleEntry
{
    enum Action
    {
        LOGIN,
        LOGOUT,
        ZONE_CHANGE,
        ACTIVITY_CHANGE,
        IDLE_CHECK,
        HEARTBEAT
    };

    ObjectGuid botGuid;
    ::std::chrono::system_clock::time_point executeTime;
    Action action;
    uint32 data1 = 0;        // Action-specific data
    uint32 data2 = 0;        // Action-specific data

    // Priority queue comparison (earlier times have higher priority)
    bool operator<(ScheduleEntry const& other) const
    {
        return executeTime > other.executeTime; // Reverse for min-heap
    }
};

// Scheduled action for lifecycle management
struct ScheduledAction
{
    ObjectGuid botGuid;
    ScheduleEntry::Action action;
    ::std::chrono::system_clock::time_point when;
    ::std::string patternName;
    uint32 data1 = 0;
    uint32 data2 = 0;

    ScheduledAction() = default;
    ScheduledAction(ObjectGuid guid, ScheduleEntry::Action act, ::std::chrono::system_clock::time_point time)
        : botGuid(guid), action(act), when(time) {}
    ScheduledAction(ObjectGuid guid, ScheduleEntry::Action act, ::std::chrono::system_clock::time_point time, ::std::string const& pattern)
        : botGuid(guid), action(act), when(time), patternName(pattern) {}
};

// Activity pattern for realistic bot behavior
struct ActivityPattern
{
    ::std::string name;

    // Active hour ranges (24-hour format)
    ::std::vector<::std::pair<uint32, uint32>> activeHours;

    // Active days of week (0=Sunday, 6=Saturday)
    ::std::vector<uint32> activeDays;

    // Behavioral parameters
    float loginProbability = 1.0f;        // Base login probability
    uint32 minSessionDuration = 1800;     // 30 minutes in seconds
    uint32 maxSessionDuration = 14400;    // 4 hours in seconds
    uint32 averageSessionsPerDay = 2;     // Sessions per day
    bool preferPeakHours = true;          // Weight towards peak hours

    // Advanced parameters
    float weekendMultiplier = 1.5f;       // Weekend activity multiplier
    float peakHourBonus = 2.0f;           // Peak hour login bonus
    uint32 jitterMinutes = 30;            // Random time variation
};

// Bot schedule state
struct BotScheduleState
{
    ObjectGuid guid;
    ::std::string patternName;
    ::std::chrono::system_clock::time_point nextLogin;
    ::std::chrono::system_clock::time_point nextLogout;
    ::std::chrono::system_clock::time_point lastActivity;
    ::std::chrono::system_clock::time_point lastLogin;
    ::std::chrono::system_clock::time_point currentSessionStart;
    ::std::chrono::system_clock::time_point nextRetry;
    uint32 totalSessions = 0;
    uint32 totalPlaytime = 0;           // Total seconds played
    uint32 consecutiveFailures = 0;
    ::std::string lastFailureReason;
    bool isScheduled = false;
    bool isActive = false;
};

// Scheduler configuration
struct SchedulerConfig
{
    bool enabled = true;
    bool useRealisticPatterns = true;
    uint32 lookaheadMinutes = 60;       // How far ahead to schedule
    uint32 heartbeatIntervalSeconds = 300; // 5 minutes
    uint32 idleTimeoutSeconds = 1800;   // 30 minutes
    uint32 maxConcurrentActions = 1000; // Queue size limit
    float peakHourMultiplier = 2.0f;
    float offPeakMultiplier = 0.5f;

    // Peak hours definition (24-hour format)
    uint32 peakStartHour = 18;          // 6 PM
    uint32 peakEndHour = 23;            // 11 PM
};

// Scheduler statistics
struct SchedulerStats
{
    ::std::atomic<uint32> totalScheduled{0};
    ::std::atomic<uint32> totalExecuted{0};
    ::std::atomic<uint32> loginActions{0};
    ::std::atomic<uint32> logoutActions{0};
    ::std::atomic<uint32> missedActions{0};
    ::std::atomic<uint64> averageExecutionTime{0}; // microseconds
    ::std::atomic<uint32> activeSchedules{0};
    ::std::atomic<uint32> queueSize{0};

    float GetExecutionRate() const
    {
        uint32 scheduled = totalScheduled.load();
        return scheduled > 0 ? static_cast<float>(totalExecuted.load()) / scheduled * 100.0f : 0.0f;
    }

    float GetAverageExecutionTimeMs() const
    {
        return averageExecutionTime.load() / 1000.0f;
    }
};

class TC_GAME_API BotScheduler final 
{
public:
    BotScheduler(BotScheduler const&) = delete;
    BotScheduler& operator=(BotScheduler const&) = delete;

    static BotScheduler* instance();

    // IBotScheduler interface implementation
    using SchedulerConfig = Playerbot::SchedulerConfig;
    using SchedulerStats = Playerbot::SchedulerStats;

    // Initialization and shutdown
    bool Initialize();
    void Shutdown();
    void Update(uint32 diff);

    // Configuration management
    void LoadConfig();
    SchedulerConfig const& GetConfig() const { return _config; }
    void SetConfig(SchedulerConfig const& config) { _config = config; }

    // Activity pattern management
    void LoadActivityPatterns();
    void RegisterPattern(::std::string const& name, ActivityPattern const& pattern);
    ActivityPattern const* GetPattern(::std::string const& name) const;
    ::std::vector<::std::string> GetAvailablePatterns() const;
    bool RemovePattern(::std::string const& name);

    // Bot scheduling operations
    void ScheduleBot(ObjectGuid guid, ::std::string const& patternName = "default");
    void UnscheduleBot(ObjectGuid guid);
    void ScheduleAction(ScheduleEntry const& entry);
    void ScheduleLogin(ObjectGuid guid, ::std::chrono::system_clock::time_point when);
    void ScheduleLogout(ObjectGuid guid, ::std::chrono::system_clock::time_point when);

    // Pattern assignment and management
    void AssignPattern(ObjectGuid guid, ::std::string const& patternName);
    ::std::string GetBotPattern(ObjectGuid guid) const;
    BotScheduleState const* GetBotScheduleState(ObjectGuid guid) const;

    // Time calculation algorithms
    ::std::chrono::system_clock::time_point CalculateNextLogin(ObjectGuid guid);
    ::std::chrono::system_clock::time_point CalculateNextLogout(ObjectGuid guid);
    ::std::chrono::system_clock::time_point CalculateSessionEnd(ObjectGuid guid, uint32 minDuration, uint32 maxDuration);

    // Schedule processing and execution
    void ProcessSchedule();
    void ExecuteScheduledAction(ScheduleEntry const& entry);
    void ProcessPendingActions();

    // Query operations
    bool IsBotScheduled(ObjectGuid guid) const;
    bool IsBotActive(ObjectGuid guid) const;
    uint32 GetScheduledBotCount() const;
    ::std::vector<ObjectGuid> GetScheduledBots() const;
    ::std::vector<ScheduleEntry> GetUpcomingActions(uint32 minutes = 60) const;

    // Statistics and monitoring
    SchedulerStats const& GetStats() const { return _stats; }
    void ResetStats();

    // Lifecycle management interface
    ::std::vector<ScheduledAction> GetBotsReadyForLogin(uint32 maxCount = 10);
    ::std::vector<ScheduledAction> GetBotsReadyForLogout(uint32 maxCount = 10);
    void OnBotLoggedIn(ObjectGuid guid);
    void OnBotLoginFailed(ObjectGuid guid, ::std::string const& reason = "");
    void SetEnabled(bool enabled) { _enabled = enabled; }
    bool IsEnabled() const { return _enabled.load(); }
    void UpdateScheduleDatabase();

    // Admin and debug commands
    void DumpSchedule() const;
    void DumpBotSchedule(ObjectGuid guid) const;
    bool ValidateSchedule() const;

private:
    BotScheduler() = default;
    ~BotScheduler() = default;

    // Internal scheduling logic
    void ScheduleInternal(ObjectGuid guid, ScheduleEntry::Action action,
                         ::std::chrono::system_clock::time_point when, uint32 data1 = 0, uint32 data2 = 0);
    void UpdateBotScheduleState(ObjectGuid guid);
    void RecalculateSchedule(ObjectGuid guid);

    // Time calculation helpers
    bool IsCurrentlyActiveTime(ActivityPattern const& pattern) const;
    bool IsPeakHour(uint32 hour) const;
    float GetTimeMultiplier(::std::chrono::system_clock::time_point time, ActivityPattern const& pattern) const;
    uint32 GetRandomSessionDuration(ActivityPattern const& pattern) const;
    ::std::chrono::system_clock::time_point AddJitter(::std::chrono::system_clock::time_point time, uint32 jitterMinutes) const;

    // Pattern management helpers
    void LoadDefaultPatterns();
    void LoadDatabasePatterns();
    ActivityPattern CreateDefaultPattern() const;
    ActivityPattern CreateCasualPattern() const;
    ActivityPattern CreateHardcorePattern() const;
    ActivityPattern CreateWeekendPattern() const;

    // Database operations
    void LoadBotSchedules();
    void SaveBotSchedule(BotScheduleState const& state);
    void LoadPatternFromDatabase(::std::string const& name);
    void SavePatternToDatabase(::std::string const& name, ActivityPattern const& pattern);

    // Internal data structures
    SchedulerConfig _config;
    SchedulerStats _stats;

    // Thread-safe activity pattern storage
    mutable OrderedRecursiveMutex<LockOrder::BOT_SPAWNER> _patternMutex;
    ::std::unordered_map<::std::string, ActivityPattern> _activityPatterns;

    // Thread-safe bot schedule state storage (TBB removed)
    mutable OrderedRecursiveMutex<LockOrder::BOT_SPAWNER> _scheduleMutex;
    ::std::unordered_map<ObjectGuid, BotScheduleState> _botSchedules;

    // Priority queue for scheduled actions (TBB removed)
    ::std::priority_queue<ScheduleEntry> _scheduleQueue;
    mutable OrderedRecursiveMutex<LockOrder::BOT_SPAWNER> _scheduleQueueMutex;

    // Runtime state
    ::std::atomic<bool> _enabled{true};

    // Timing and update management
    ::std::chrono::steady_clock::time_point _lastUpdate;
    ::std::chrono::steady_clock::time_point _lastDatabaseSync;
    uint32 _lastProcessTime = 0;

    // Performance monitoring
    static constexpr uint32 DATABASE_SYNC_INTERVAL = 300000; // 5 minutes
    static constexpr uint32 SCHEDULE_LOOKAHEAD = 3600000;    // 1 hour
    static constexpr uint32 MAX_ACTIONS_PER_UPDATE = 100;    // Batch size limit
};

#define sBotScheduler BotScheduler::instance()

} // namespace Playerbot