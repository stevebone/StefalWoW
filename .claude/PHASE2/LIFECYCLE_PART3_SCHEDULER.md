# Bot Lifecycle Management - Part 3: BotScheduler Implementation
## Week 11 Day 3-4: Scheduling System

### File: `src/modules/Playerbot/Lifecycle/BotScheduler.h`
```cpp
#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <memory>
#include <vector>
#include <chrono>
#include <functional>
#include <tbb/concurrent_priority_queue.h>
#include <phmap.h>

namespace Playerbot
{

// Schedule entry for bot activities
struct ScheduleEntry
{
    ObjectGuid botGuid;
    std::chrono::system_clock::time_point executeTime;

    enum Action
    {
        LOGIN,
        LOGOUT,
        ZONE_CHANGE,
        ACTIVITY_CHANGE,
        IDLE_CHECK,
        HEARTBEAT
    };
    Action action;

    uint32 data1 = 0; // Action-specific data
    uint32 data2 = 0;

    // Priority queue comparison (earliest time = highest priority)
    bool operator<(ScheduleEntry const& other) const
    {
        return executeTime > other.executeTime; // Inverted for min-heap
    }
};

// Bot activity pattern
struct ActivityPattern
{
    std::string name;
    std::vector<std::pair<uint32, uint32>> activeHours; // Hour ranges (0-23)
    std::vector<uint32> activeDays; // Days of week (0-6)
    float loginProbability = 1.0f;
    uint32 minSessionDuration = 1800; // Seconds
    uint32 maxSessionDuration = 14400;
    uint32 averageSessionsPerDay = 2;
    bool preferPeakHours = true;
};

// Bot schedule configuration
struct ScheduleConfig
{
    bool enableScheduling = true;
    bool useRealisticPatterns = true;
    uint32 scheduleLookaheadMinutes = 60;
    uint32 heartbeatIntervalSeconds = 300;
    uint32 idleTimeoutSeconds = 1800;
    float peakHourMultiplier = 2.0f;
    float offPeakMultiplier = 0.5f;
};

class TC_GAME_API BotScheduler
{
    BotScheduler();
    ~BotScheduler();
    BotScheduler(BotScheduler const&) = delete;
    BotScheduler& operator=(BotScheduler const&) = delete;

public:
    static BotScheduler* instance();

    // Initialization
    bool Initialize();
    void Shutdown();

    // Schedule management - Enterprise scheduling system
    void ScheduleBot(ObjectGuid guid);
    void UnscheduleBot(ObjectGuid guid);
    void ScheduleAction(ScheduleEntry const& entry);
    void CancelScheduledActions(ObjectGuid guid);

    // Pattern management
    void LoadActivityPatterns();
    void RegisterPattern(std::string const& name, ActivityPattern const& pattern);
    ActivityPattern const* GetPattern(std::string const& name) const;
    void AssignPattern(ObjectGuid guid, std::string const& patternName);

    // Time-based scheduling
    void ScheduleLogin(ObjectGuid guid, std::chrono::system_clock::time_point when);
    void ScheduleLogout(ObjectGuid guid, std::chrono::system_clock::time_point when);
    void ScheduleNextActivity(ObjectGuid guid);

    // Batch scheduling
    void SchedulePeakHourBots();
    void ScheduleOffPeakBots();
    void ScheduleWeekendBots();
    void ScheduleMaintenanceWindow();

    // Query functions
    bool IsBotScheduled(ObjectGuid guid) const;
    std::chrono::system_clock::time_point GetNextScheduledAction(ObjectGuid guid) const;
    uint32 GetScheduledBotCount() const { return _botSchedules.size(); }
    std::vector<ObjectGuid> GetBotsScheduledForLogin(uint32 nextMinutes) const;

    // Statistics
    struct Statistics
    {
        std::atomic<uint32> totalScheduled;
        std::atomic<uint32> actionsExecuted;
        std::atomic<uint32> loginsScheduled;
        std::atomic<uint32> logoutsScheduled;
        std::atomic<uint32> patternsAssigned;
        std::atomic<uint32> averageSessionMinutes;
    };
    Statistics const& GetStatistics() const { return _stats; }

    // Configuration
    void SetConfig(ScheduleConfig const& config) { _config = config; }
    ScheduleConfig const& GetConfig() const { return _config; }

private:
    // Internal scheduling logic
    void ProcessSchedule();
    void ExecuteScheduledAction(ScheduleEntry const& entry);

    // Pattern-based scheduling
    std::chrono::system_clock::time_point CalculateNextLogin(ObjectGuid guid);
    std::chrono::system_clock::time_point CalculateSessionEnd(
        ObjectGuid guid,
        std::chrono::system_clock::time_point loginTime
    );

    // Time calculations
    bool IsInActiveHours(ActivityPattern const& pattern,
                        std::chrono::system_clock::time_point time) const;
    bool IsPeakHour(std::chrono::system_clock::time_point time) const;
    float GetTimeMultiplier(std::chrono::system_clock::time_point time) const;

    // Session management
    void StartBotSession(ObjectGuid guid);
    void EndBotSession(ObjectGuid guid);
    void UpdateBotActivity(ObjectGuid guid);

private:
    // Priority queue for scheduled actions (min-heap by time)
    tbb::concurrent_priority_queue<ScheduleEntry> _scheduleQueue;

    // Bot schedules and patterns
    phmap::parallel_flat_hash_map<ObjectGuid::LowType, std::string> _botPatterns;
    phmap::parallel_flat_hash_map<std::string, ActivityPattern> _activityPatterns;
    phmap::parallel_flat_hash_set<ObjectGuid> _botSchedules;

    // Session tracking
    phmap::parallel_flat_hash_map<ObjectGuid::LowType,
        std::chrono::system_clock::time_point> _sessionStarts;

    // Configuration
    ScheduleConfig _config;

    // Statistics
    mutable Statistics _stats;

    // Processing thread
    std::thread _processingThread;
    std::atomic<bool> _shutdown;
};

#define sBotScheduler BotScheduler::instance()

} // namespace Playerbot
```

### File: `src/modules/Playerbot/Lifecycle/BotScheduler.cpp`
```cpp
#include "BotScheduler.h"
#include "BotSpawner.h"
#include "BotSessionMgr.h"
#include "BotDatabasePool.h"
#include "World.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "Log.h"
#include <random>

namespace Playerbot
{

BotScheduler::BotScheduler()
    : _shutdown(false)
{
}

BotScheduler::~BotScheduler()
{
    Shutdown();
}

BotScheduler* BotScheduler::instance()
{
    static BotScheduler instance;
    return &instance;
}

bool BotScheduler::Initialize()
{
    TC_LOG_INFO("playerbot", "Initializing BotScheduler with enterprise scheduling...");

    // Load configuration
    _config.enableScheduling = sConfigMgr->GetBoolDefault("Playerbot.Schedule.Enable", true);
    _config.useRealisticPatterns = sConfigMgr->GetBoolDefault("Playerbot.Schedule.RealisticPatterns", true);
    _config.scheduleLookaheadMinutes = sConfigMgr->GetIntDefault("Playerbot.Schedule.LookaheadMinutes", 60);
    _config.heartbeatIntervalSeconds = sConfigMgr->GetIntDefault("Playerbot.Schedule.HeartbeatInterval", 300);
    _config.idleTimeoutSeconds = sConfigMgr->GetIntDefault("Playerbot.Schedule.IdleTimeout", 1800);
    _config.peakHourMultiplier = sConfigMgr->GetFloatDefault("Playerbot.Schedule.PeakMultiplier", 2.0f);
    _config.offPeakMultiplier = sConfigMgr->GetFloatDefault("Playerbot.Schedule.OffPeakMultiplier", 0.5f);

    // Load activity patterns
    LoadActivityPatterns();

    // Start processing thread
    if (_config.enableScheduling)
    {
        _processingThread = std::thread([this] { ProcessSchedule(); });
    }

    TC_LOG_INFO("playerbot", "BotScheduler initialized successfully");
    TC_LOG_INFO("playerbot", "  -> Scheduling: %s", _config.enableScheduling ? "enabled" : "disabled");
    TC_LOG_INFO("playerbot", "  -> Realistic patterns: %s",
                _config.useRealisticPatterns ? "enabled" : "disabled");
    TC_LOG_INFO("playerbot", "  -> Lookahead: %u minutes", _config.scheduleLookaheadMinutes);
    TC_LOG_INFO("playerbot", "  -> Loaded %zu activity patterns", _activityPatterns.size());

    return true;
}

void BotScheduler::Shutdown()
{
    TC_LOG_INFO("playerbot", "Shutting down BotScheduler...");

    _shutdown = true;

    // Stop processing thread
    if (_processingThread.joinable())
        _processingThread.join();

    // Clear all schedules
    _scheduleQueue.clear();
    _botSchedules.clear();
    _botPatterns.clear();

    TC_LOG_INFO("playerbot", "BotScheduler shutdown complete. Stats:");
    TC_LOG_INFO("playerbot", "  -> Total scheduled: %u", _stats.totalScheduled.load());
    TC_LOG_INFO("playerbot", "  -> Actions executed: %u", _stats.actionsExecuted.load());
    TC_LOG_INFO("playerbot", "  -> Average session: %u minutes", _stats.averageSessionMinutes.load());
}

void BotScheduler::ScheduleBot(ObjectGuid guid)
{
    if (!_config.enableScheduling)
    {
        // If scheduling disabled, just mark as active
        _botSchedules.insert(guid);
        return;
    }

    TC_LOG_DEBUG("playerbot", "Scheduling bot %s", guid.ToString().c_str());

    // Check if already scheduled
    if (_botSchedules.count(guid))
    {
        TC_LOG_DEBUG("playerbot", "Bot %s already scheduled", guid.ToString().c_str());
        return;
    }

    _botSchedules.insert(guid);
    _stats.totalScheduled++;

    // Assign pattern if not already assigned
    if (!_botPatterns.count(guid.GetRawValue()))
    {
        // Select random pattern or use default
        if (_config.useRealisticPatterns && !_activityPatterns.empty())
        {
            // Random pattern selection
            auto it = _activityPatterns.begin();
            std::advance(it, rand() % _activityPatterns.size());
            AssignPattern(guid, it->first);
        }
        else
        {
            AssignPattern(guid, "default");
        }
    }

    // Schedule next activity
    ScheduleNextActivity(guid);

    // Schedule heartbeat
    ScheduleEntry heartbeat;
    heartbeat.botGuid = guid;
    heartbeat.action = ScheduleEntry::HEARTBEAT;
    heartbeat.executeTime = std::chrono::system_clock::now() +
        std::chrono::seconds(_config.heartbeatIntervalSeconds);
    ScheduleAction(heartbeat);
}

void BotScheduler::ScheduleAction(ScheduleEntry const& entry)
{
    _scheduleQueue.push(entry);

    TC_LOG_DEBUG("playerbot", "Scheduled action %u for bot %s at %s",
                entry.action, entry.botGuid.ToString().c_str(),
                std::asctime(std::localtime(&std::chrono::system_clock::to_time_t(entry.executeTime))));
}

void BotScheduler::ScheduleNextActivity(ObjectGuid guid)
{
    if (!_config.useRealisticPatterns)
    {
        // Simple scheduling - random login/logout
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(300, 3600); // 5 min to 1 hour

        ScheduleEntry entry;
        entry.botGuid = guid;
        entry.action = ScheduleEntry::LOGOUT;
        entry.executeTime = std::chrono::system_clock::now() +
            std::chrono::seconds(dist(gen));
        ScheduleAction(entry);
        return;
    }

    // Get bot's pattern
    auto patternIt = _botPatterns.find(guid.GetRawValue());
    if (patternIt == _botPatterns.end())
        return;

    auto pattern = GetPattern(patternIt->second);
    if (!pattern)
        return;

    // Calculate next login time based on pattern
    auto nextLogin = CalculateNextLogin(guid);
    ScheduleLogin(guid, nextLogin);

    // Calculate session end
    auto sessionEnd = CalculateSessionEnd(guid, nextLogin);
    ScheduleLogout(guid, sessionEnd);
}

std::chrono::system_clock::time_point BotScheduler::CalculateNextLogin(ObjectGuid guid)
{
    auto patternIt = _botPatterns.find(guid.GetRawValue());
    if (patternIt == _botPatterns.end())
        return std::chrono::system_clock::now() + std::chrono::hours(1);

    auto pattern = GetPattern(patternIt->second);
    if (!pattern)
        return std::chrono::system_clock::now() + std::chrono::hours(1);

    std::random_device rd;
    std::mt19937 gen(rd());

    // Find next active period
    auto now = std::chrono::system_clock::now();
    auto currentTime = std::chrono::system_clock::to_time_t(now);
    struct tm* timeinfo = std::localtime(&currentTime);

    // Check if we're in active hours
    for (auto const& [startHour, endHour] : pattern->activeHours)
    {
        if (timeinfo->tm_hour >= startHour && timeinfo->tm_hour < endHour)
        {
            // Already in active hours, schedule within current period
            std::uniform_int_distribution<> dist(5, 30); // 5-30 minutes
            return now + std::chrono::minutes(dist(gen));
        }
        else if (timeinfo->tm_hour < startHour)
        {
            // Next active period is today
            timeinfo->tm_hour = startHour;
            timeinfo->tm_min = 0;
            timeinfo->tm_sec = 0;

            std::uniform_int_distribution<> jitter(0, 30); // 0-30 minute jitter
            return std::chrono::system_clock::from_time_t(std::mktime(timeinfo)) +
                   std::chrono::minutes(jitter(gen));
        }
    }

    // Schedule for tomorrow's first active period
    if (!pattern->activeHours.empty())
    {
        timeinfo->tm_mday++;
        timeinfo->tm_hour = pattern->activeHours[0].first;
        timeinfo->tm_min = 0;
        timeinfo->tm_sec = 0;

        std::uniform_int_distribution<> jitter(0, 30);
        return std::chrono::system_clock::from_time_t(std::mktime(timeinfo)) +
               std::chrono::minutes(jitter(gen));
    }

    // Fallback
    return now + std::chrono::hours(1);
}

std::chrono::system_clock::time_point BotScheduler::CalculateSessionEnd(
    ObjectGuid guid,
    std::chrono::system_clock::time_point loginTime)
{
    auto patternIt = _botPatterns.find(guid.GetRawValue());
    if (patternIt == _botPatterns.end())
        return loginTime + std::chrono::hours(1);

    auto pattern = GetPattern(patternIt->second);
    if (!pattern)
        return loginTime + std::chrono::hours(1);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(pattern->minSessionDuration,
                                         pattern->maxSessionDuration);

    return loginTime + std::chrono::seconds(dist(gen));
}

void BotScheduler::LoadActivityPatterns()
{
    TC_LOG_INFO("playerbot", "Loading bot activity patterns...");

    // Default pattern - active most of the day
    ActivityPattern defaultPattern;
    defaultPattern.name = "default";
    defaultPattern.activeHours = {{6, 23}}; // 6 AM to 11 PM
    defaultPattern.activeDays = {0, 1, 2, 3, 4, 5, 6}; // All days
    defaultPattern.loginProbability = 1.0f;
    defaultPattern.minSessionDuration = 1800; // 30 minutes
    defaultPattern.maxSessionDuration = 7200; // 2 hours
    defaultPattern.averageSessionsPerDay = 3;
    RegisterPattern("default", defaultPattern);

    // Casual pattern - evenings and weekends
    ActivityPattern casualPattern;
    casualPattern.name = "casual";
    casualPattern.activeHours = {{18, 23}}; // 6 PM to 11 PM
    casualPattern.activeDays = {0, 1, 2, 3, 4, 5, 6};
    casualPattern.loginProbability = 0.8f;
    casualPattern.minSessionDuration = 900; // 15 minutes
    casualPattern.maxSessionDuration = 3600; // 1 hour
    casualPattern.averageSessionsPerDay = 1;
    RegisterPattern("casual", casualPattern);

    // Hardcore pattern - long sessions, all hours
    ActivityPattern hardcorePattern;
    hardcorePattern.name = "hardcore";
    hardcorePattern.activeHours = {{0, 24}}; // All day
    hardcorePattern.activeDays = {0, 1, 2, 3, 4, 5, 6};
    hardcorePattern.loginProbability = 1.0f;
    hardcorePattern.minSessionDuration = 7200; // 2 hours
    hardcorePattern.maxSessionDuration = 28800; // 8 hours
    hardcorePattern.averageSessionsPerDay = 2;
    RegisterPattern("hardcore", hardcorePattern);

    // Weekend warrior - mainly weekends
    ActivityPattern weekendPattern;
    weekendPattern.name = "weekend";
    weekendPattern.activeHours = {{10, 23}}; // 10 AM to 11 PM
    weekendPattern.activeDays = {0, 6}; // Sunday, Saturday
    weekendPattern.loginProbability = 0.9f;
    weekendPattern.minSessionDuration = 3600; // 1 hour
    weekendPattern.maxSessionDuration = 14400; // 4 hours
    weekendPattern.averageSessionsPerDay = 2;
    RegisterPattern("weekend", weekendPattern);

    // Load custom patterns from database
    auto result = sBotDBPool->Query(
        "SELECT name, active_hours, active_days, login_probability, "
        "min_session, max_session, sessions_per_day "
        "FROM playerbot_activity_patterns"
    );

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            ActivityPattern pattern;
            pattern.name = fields[0].GetString();
            // Parse active hours and days from JSON or string format
            // ...
            pattern.loginProbability = fields[3].GetFloat();
            pattern.minSessionDuration = fields[4].GetUInt32();
            pattern.maxSessionDuration = fields[5].GetUInt32();
            pattern.averageSessionsPerDay = fields[6].GetUInt32();

            RegisterPattern(pattern.name, pattern);
        }
        while (result->NextRow());
    }

    TC_LOG_INFO("playerbot", "Loaded %zu activity patterns", _activityPatterns.size());
}

void BotScheduler::ProcessSchedule()
{
    while (!_shutdown)
    {
        auto now = std::chrono::system_clock::now();

        // Process due entries
        while (!_scheduleQueue.empty())
        {
            ScheduleEntry entry;
            if (_scheduleQueue.try_pop(entry))
            {
                if (entry.executeTime <= now)
                {
                    ExecuteScheduledAction(entry);
                    _stats.actionsExecuted++;
                }
                else
                {
                    // Not due yet, push back
                    _scheduleQueue.push(entry);
                    break;
                }
            }
        }

        // Sleep briefly
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void BotScheduler::ExecuteScheduledAction(ScheduleEntry const& entry)
{
    TC_LOG_DEBUG("playerbot", "Executing scheduled action %u for bot %s",
                entry.action, entry.botGuid.ToString().c_str());

    switch (entry.action)
    {
        case ScheduleEntry::LOGIN:
        {
            SpawnRequest request;
            request.type = SpawnRequest::SPECIFIC_CHARACTER;
            request.characterGuid = entry.botGuid;
            sBotSpawner->SpawnBot(request);
            _stats.loginsScheduled++;
            break;
        }
        case ScheduleEntry::LOGOUT:
        {
            sBotSpawner->DespawnBot(entry.botGuid);
            _stats.logoutsScheduled++;

            // Schedule next login
            ScheduleNextActivity(entry.botGuid);
            break;
        }
        case ScheduleEntry::HEARTBEAT:
        {
            // Check if bot is still active
            UpdateBotActivity(entry.botGuid);

            // Reschedule heartbeat
            ScheduleEntry nextHeartbeat = entry;
            nextHeartbeat.executeTime = std::chrono::system_clock::now() +
                std::chrono::seconds(_config.heartbeatIntervalSeconds);
            ScheduleAction(nextHeartbeat);
            break;
        }
        // Handle other actions...
    }
}

void BotScheduler::RegisterPattern(std::string const& name, ActivityPattern const& pattern)
{
    _activityPatterns[name] = pattern;
    _stats.patternsAssigned++;
}

void BotScheduler::AssignPattern(ObjectGuid guid, std::string const& patternName)
{
    _botPatterns[guid.GetRawValue()] = patternName;

    // Store in database for persistence
    auto stmt = sBotDBPool->GetPreparedStatement(BOT_UPD_ACTIVITY_PATTERN);
    stmt->setUInt64(0, guid.GetRawValue());
    stmt->setString(1, patternName);
    sBotDBPool->ExecuteAsync(stmt);
}

ActivityPattern const* BotScheduler::GetPattern(std::string const& name) const
{
    auto it = _activityPatterns.find(name);
    return it != _activityPatterns.end() ? &it->second : nullptr;
}

bool BotScheduler::IsBotScheduled(ObjectGuid guid) const
{
    return _botSchedules.count(guid) > 0;
}

void BotScheduler::UpdateBotActivity(ObjectGuid guid)
{
    // Check bot activity, handle idle timeouts, etc.
    // This is called periodically by heartbeat

    Player* player = ObjectAccessor::FindPlayer(guid);
    if (player)
    {
        // Check if idle
        // Update session time
        // etc.
    }
}

} // namespace Playerbot
```

## Next Parts
- [Part 1: Overview & Architecture](LIFECYCLE_PART1_OVERVIEW.md)
- [Part 2: BotSpawner Implementation](LIFECYCLE_PART2_SPAWNER.md)
- [Part 4: Lifecycle Coordinator](LIFECYCLE_PART4_COORDINATOR.md)
- [Part 5: Database & Testing](LIFECYCLE_PART5_DATABASE.md)

---

**Status**: Ready for implementation
**Dependencies**: BotSpawner, BotSessionMgr, BotDatabasePool
**Estimated Completion**: Day 3-4 of Week 11