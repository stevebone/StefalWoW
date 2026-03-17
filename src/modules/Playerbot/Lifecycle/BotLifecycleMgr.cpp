/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BotLifecycleMgr.h"
#include "BotScheduler.h"
#include "BotSpawner.h"

using namespace Playerbot;
#include "PlayerbotDatabaseStatements.h"
#include "PlayerbotDatabase.h"
#include "PlayerbotMigrationMgr.h"
#include "Log.h"
#include "Timer.h"
#include "Util.h"
#include <sstream>
#include <iomanip>
#include <shared_mutex>

BotLifecycleMgr* BotLifecycleMgr::instance()
{
    static BotLifecycleMgr instance;
    return &instance;
}

bool BotLifecycleMgr::Initialize()
{
    LIFECYCLE_LOG_INFO("Initializing Bot Lifecycle Manager...");

    _startTime = std::chrono::system_clock::now();
    _lastUpdate = _startTime;
    _lastMaintenance = _startTime;

    // Note: Migration manager is initialized earlier in PlayerbotModule
    // to ensure database schema is ready before any component accesses it

    // Initialize components (use singleton instances)
    _scheduler = BotScheduler::instance();
    if (!_scheduler->Initialize())
    {
        LIFECYCLE_LOG_ERROR("Failed to initialize BotScheduler");
        return false;
    }

    _spawner = BotSpawner::instance();
    if (!_spawner->Initialize())
    {
        LIFECYCLE_LOG_ERROR("Failed to initialize BotSpawner");
        return false;
    }

    // Start worker thread
    _running = true;
    _workerThread = std::make_unique<std::thread>(&BotLifecycleMgr::WorkerThreadProc, this);

    // Initialize statistics
    _statistics.startTime = _startTime;
    _statistics.lastUpdate = _startTime;

    LIFECYCLE_LOG_INFO("Bot Lifecycle Manager initialized successfully");
    return true;
}

void BotLifecycleMgr::Shutdown()
{
    LIFECYCLE_LOG_INFO("Shutting down Bot Lifecycle Manager...");

    _enabled = false;
    _running = false;

    // Wait for worker thread to finish
    if (_workerThread && _workerThread->joinable())
    {
        _workerThread->join();
        _workerThread.reset();
    }

    // TBB task_group removed - no background tasks to wait for in simplified version
    // _taskGroup.wait();

    // Shutdown components (but don't destroy singletons)
    if (_spawner)
    {
        _spawner->Shutdown();
        _spawner = nullptr;
    }

    if (_scheduler)
    {
        _scheduler->Shutdown();
        _scheduler = nullptr;
    }

    // Log final statistics
    LogPerformanceReport();

    LIFECYCLE_LOG_INFO("Bot Lifecycle Manager shutdown complete");
}

void BotLifecycleMgr::Update(uint32 diff)
{
    if (!_enabled || !_running)
        return;

    auto now = std::chrono::system_clock::now();
    auto timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastUpdate).count();

    // Throttle updates based on configured interval
    if (timeSinceLastUpdate < _updateIntervalMs)
        return;

    _lastUpdate = now;

    // Update performance metrics
    UpdatePerformanceMetrics();

    // Update components
    if (_scheduler)
        _scheduler->Update(static_cast<uint32>(timeSinceLastUpdate));

    if (_spawner)
        _spawner->Update(static_cast<uint32>(timeSinceLastUpdate));

    // Coordinate between scheduler and spawner
    CoordinateSchedulerAndSpawner();

    // Run maintenance if needed
    auto timeSinceLastMaintenance = std::chrono::duration_cast<std::chrono::minutes>(now - _lastMaintenance).count();
    if (timeSinceLastMaintenance >= _maintenanceIntervalMinutes)
    {
        RunMaintenance();
        _lastMaintenance = now;
    }

    // Update statistics
    _statistics.lastUpdate = now;
}

void BotLifecycleMgr::WorkerThreadProc()
{
    LIFECYCLE_LOG_INFO("Lifecycle worker thread started");

    while (_running)
    {
        try
        {
            // Process event queue
            ProcessEventQueue();

            // Process scheduler events
            ProcessSchedulerEvents();

            // Process spawner events
            ProcessSpawnerEvents();

            // Brief sleep to prevent spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        catch (std::exception const& ex)
        {
            LIFECYCLE_LOG_ERROR("Exception in worker thread: {}", ex.what());
            ++_consecutiveErrors;

            if (_consecutiveErrors >= MAX_CONSECUTIVE_ERRORS)
            {
                LIFECYCLE_LOG_ERROR("Too many consecutive errors, emergency shutdown");
                EmergencyShutdown();
                break;
            }

            // Back off on errors
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    LIFECYCLE_LOG_INFO("Lifecycle worker thread stopped");
}

void BotLifecycleMgr::ProcessEventQueue()
{
    LifecycleEventInfo eventInfo;
    uint32 processedCount = 0;
    const uint32 maxEventsPerCycle = 50; // Prevent processing all events in one cycle

    // Replace TBB try_pop with std::queue + mutex
    while (processedCount < maxEventsPerCycle)
    {
        std::lock_guard lock(_eventQueueMutex);
        if (_eventQueue.empty())
            break;

        eventInfo = _eventQueue.front();
        _eventQueue.pop();
    } // End of lock scope

    {
        auto startTime = std::chrono::high_resolution_clock::now();

        ProcessEvent(eventInfo);

        auto endTime = std::chrono::high_resolution_clock::now();
        uint32 processingTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        // Update processing time in event info for logging
        eventInfo.processingTimeMs = processingTime;

        // Log event to database
        LogLifecycleEvent(eventInfo);

        // Update metrics
        RecordEvent(eventInfo.eventType, processingTime);

        ++processedCount;
        ++_statistics.totalLifecycleEvents;
    }
}

void BotLifecycleMgr::ProcessEvent(LifecycleEventInfo const& eventInfo)
{
    switch (eventInfo.eventType)
    {
        case LifecycleEventInfo::SCHEDULER_LOGIN:
            OnBotLoginRequested(eventInfo.botGuid, eventInfo.data);
            ++_statistics.scheduledLogins;
            break;

        case LifecycleEventInfo::SCHEDULER_LOGOUT:
            OnBotLogoutRequested(eventInfo.botGuid, eventInfo.data);
            ++_statistics.scheduledLogouts;
            break;

        case LifecycleEventInfo::SPAWNER_SUCCESS:
            OnBotSpawnSuccess(eventInfo.botGuid, eventInfo.accountId);
            ++_statistics.successfulSpawns;
            break;

        case LifecycleEventInfo::SPAWNER_FAILURE:
            OnBotSpawnFailure(eventInfo.botGuid, eventInfo.data);
            ++_statistics.failedSpawns;
            break;

        case LifecycleEventInfo::POPULATION_UPDATE:
            UpdateZonePopulations();
            ++_statistics.populationUpdates;
            break;

        case LifecycleEventInfo::MAINTENANCE_REQUIRED:
            RunMaintenance();
            ++_statistics.maintenanceRuns;
            break;

        case LifecycleEventInfo::SYSTEM_SHUTDOWN:
            _running = false;
            break;

        default:
            LIFECYCLE_LOG_WARN("Unknown event type: {}", static_cast<uint32>(eventInfo.eventType));
            break;
    }

    // Notify registered event handlers
    NotifyEventHandlers(eventInfo);
}

void BotLifecycleMgr::ProcessSchedulerEvents()
{
    if (!_scheduler)
        return;

    // Process scheduler actions that are due
    _scheduler->ProcessSchedule();

    // Handle any scheduler-generated events
    // Note: This is where we'd process any events that the scheduler pushes to us
}

void BotLifecycleMgr::ProcessSpawnerEvents()
{
    if (!_spawner)
        return;

    // Check for spawn completion events
    // Note: This is where we'd process any events that the spawner pushes to us
}

void BotLifecycleMgr::OnBotLoginRequested(ObjectGuid guid, std::string const& pattern)
{
    LIFECYCLE_LOG_DEBUG("Bot login requested: {} with pattern: {}", guid.ToString(), pattern);

    if (!_spawner)
    {
        LIFECYCLE_LOG_ERROR("Cannot process login request: spawner not available");
        return;
    }

    // Create spawn request
    SpawnRequest request;
    request.type = SpawnRequest::RANDOM;
    request.characterGuid = guid;

    // Submit to spawner
    bool success = _spawner->SpawnBot(request);

    if (!success)
    {
        LIFECYCLE_LOG_WARN("Failed to spawn bot for login request: {}", guid.ToString());

        // Create failure event
        LifecycleEventInfo failureEvent;
        failureEvent.eventType = LifecycleEventInfo::SPAWNER_FAILURE;
        failureEvent.botGuid = guid;
        failureEvent.data = "Failed to spawn for scheduled login";
        failureEvent.timestamp = std::chrono::system_clock::now();
        failureEvent.correlationId = GenerateCorrelationId();

        _eventQueue.push(failureEvent);
    }
}

void BotLifecycleMgr::OnBotLogoutRequested(ObjectGuid guid, std::string const& reason)
{
    LIFECYCLE_LOG_DEBUG("Bot logout requested: {} reason: {}", guid.ToString(), reason);

    if (!_spawner)
    {
        LIFECYCLE_LOG_ERROR("Cannot process logout request: spawner not available");
        return;
    }

    // Despawn the bot
    bool success = _spawner->DespawnBot(guid, reason);

    if (!success)
    {
        LIFECYCLE_LOG_WARN("Failed to despawn bot for logout request: {}", guid.ToString());
    }
}

void BotLifecycleMgr::OnBotSpawnSuccess(ObjectGuid guid, uint32 accountId)
{
    LIFECYCLE_LOG_DEBUG("Bot spawn successful: {} account: {}", guid.ToString(), accountId);

    // Update metrics
    ++_metrics.activeBots;

    // Update scheduler with successful login
    if (_scheduler)
    {
        _scheduler->OnBotLoggedIn(guid);
    }

    // Reset consecutive errors on success
    _consecutiveErrors = 0;
}

void BotLifecycleMgr::OnBotSpawnFailure(ObjectGuid guid, std::string const& reason)
{
    LIFECYCLE_LOG_ERROR("Bot spawn failed: {} reason: {}", guid.ToString(), reason);

    // Update metrics
    ++_metrics.failedSpawnsLastHour;

    // Update scheduler with failed login
    if (_scheduler)
    {
        _scheduler->OnBotLoginFailed(guid, reason);
    }

    ++_consecutiveErrors;
}

void BotLifecycleMgr::CoordinateSchedulerAndSpawner()
{
    if (!_scheduler || !_spawner)
        return;

    // Get bots ready for login from scheduler
    std::vector<ScheduledAction> loginActions = _scheduler->GetBotsReadyForLogin(10); // Limit to 10 per cycle

    // Process login requests
    for (auto const& action : loginActions)
    {
        LifecycleEventInfo loginEvent;
        loginEvent.eventType = LifecycleEventInfo::SCHEDULER_LOGIN;
        loginEvent.botGuid = action.botGuid;
        loginEvent.data = action.patternName;
        loginEvent.timestamp = std::chrono::system_clock::now();
        loginEvent.correlationId = GenerateCorrelationId();

        _eventQueue.push(loginEvent);
    }

    // Get bots ready for logout from scheduler
    std::vector<ScheduledAction> logoutActions = _scheduler->GetBotsReadyForLogout(10);

    // Process logout requests
    for (auto const& action : logoutActions)
    {
        LifecycleEventInfo logoutEvent;
        logoutEvent.eventType = LifecycleEventInfo::SCHEDULER_LOGOUT;
        logoutEvent.botGuid = action.botGuid;
        logoutEvent.data = "Scheduled logout";
        logoutEvent.timestamp = std::chrono::system_clock::now();
        logoutEvent.correlationId = GenerateCorrelationId();

        _eventQueue.push(logoutEvent);
    }
}

void BotLifecycleMgr::UpdateZonePopulations()
{
    LIFECYCLE_LOG_DEBUG("Updating zone populations");

    // Query current bot positions and update zone population counts
    // NOTE: Using direct SQL execution for now. Should convert to prepared statements
    // when PlayerbotDatabase prepared statement infrastructure is fully ready.

    try
    {
        // Query total population from playerbot_zone_populations (legacy table)
        QueryResult result = sPlayerbotDatabase->Query(
            "SELECT SUM(current_bots) as total_bots, SUM(target_population) as target_total "
            "FROM playerbot_zone_populations WHERE is_enabled = 1"
        );

        if (result)
        {
            Field* fields = result->Fetch();
            uint32 totalBots = fields[0].GetUInt32();
            uint32 targetTotal = fields[1].GetUInt32();

            _metrics.totalBotsManaged = totalBots;

            LIFECYCLE_LOG_DEBUG("Current population: {} / {} target", totalBots, targetTotal);
        }

        // Update zone population cache table (new migration 007 table)
        // This aggregates data for faster lookup
        result = sPlayerbotDatabase->Query(
            "SELECT zone_id, COUNT(*) as bot_count FROM playerbot_state "
            "GROUP BY zone_id"
        );

        if (result)
        {
            do
            {
                Field* fields = result->Fetch();
                uint32 zoneId = fields[0].GetUInt32();
                uint32 botCount = fields[1].GetUInt32();

                // Update the bot_zone_population table
                std::ostringstream ss;
                ss << "INSERT INTO bot_zone_population "
                   << "(zone_id, bot_count, player_count, total_count, max_capacity, density_score) "
                   << "VALUES (" << zoneId << ", " << botCount << ", 0, " << botCount << ", 100, "
                   << (static_cast<float>(botCount) / 100.0f) << ") "
                   << "ON DUPLICATE KEY UPDATE "
                   << "bot_count = " << botCount << ", "
                   << "total_count = bot_count + player_count, "
                   << "density_score = bot_count / GREATEST(max_capacity, 1), "
                   << "last_updated = CURRENT_TIMESTAMP";

                sPlayerbotDatabase->Execute(ss.str());

            } while (result->NextRow());

            LIFECYCLE_LOG_DEBUG("Updated zone population cache for all zones");
        }
    }
    catch (std::exception const& e)
    {
        LIFECYCLE_LOG_ERROR("Exception while updating zone populations: {}", e.what());
    }
}

void BotLifecycleMgr::LogLifecycleEvent(LifecycleEventInfo const& eventInfo)
{
    // Map event type to category and type strings
    std::string category, type;

    switch (eventInfo.eventType)
    {
        case LifecycleEventInfo::SCHEDULER_LOGIN:
        case LifecycleEventInfo::SCHEDULER_LOGOUT:
            category = "SCHEDULER";
            type = (eventInfo.eventType == LifecycleEventInfo::SCHEDULER_LOGIN) ? "LOGIN_REQUEST" : "LOGOUT_REQUEST";
            break;
        case LifecycleEventInfo::SPAWNER_SUCCESS:
        case LifecycleEventInfo::SPAWNER_FAILURE:
            category = "SPAWNER";
            type = (eventInfo.eventType == LifecycleEventInfo::SPAWNER_SUCCESS) ? "SPAWN_SUCCESS" : "SPAWN_FAILURE";
            break;
        case LifecycleEventInfo::POPULATION_UPDATE:
            category = "SYSTEM";
            type = "POPULATION_UPDATE";
            break;
        default:
            category = "SYSTEM";
            type = "UNKNOWN";
            break;
    }

    // Insert event into database
    // NOTE: Using direct SQL execution for now. Should convert to prepared statements
    // when PlayerbotDatabase prepared statement infrastructure is fully ready.

    try
    {
        std::ostringstream ss;
        ss << "INSERT INTO bot_lifecycle_events "
           << "(event_category, event_type, severity, bot_guid, account_id, zone_id, message, metadata) "
           << "VALUES ("
           << "'" << category << "', "
           << "'" << type << "', "
           << "'INFO', ";

        // bot_guid
    if (eventInfo.botGuid.IsEmpty())
            ss << "NULL, ";
        else
            ss << eventInfo.botGuid.GetCounter() << ", ";

        // account_id
        ss << eventInfo.accountId << ", ";

        // zone_id (extract from data if available)
        ss << "NULL, ";

        // message (escape single quotes)
        std::string escapedData = eventInfo.data;
        size_t pos = 0;
        while ((pos = escapedData.find("'", pos)) != std::string::npos)
        {
            escapedData.replace(pos, 1, "''");
            pos += 2;
        }
        ss << "'" << escapedData << "', ";

        // metadata (JSON)
        ss << "JSON_OBJECT("
           << "'processingTimeMs', " << eventInfo.processingTimeMs << ", "
           << "'memoryUsageMB', " << static_cast<float>(_metrics.memoryUsageMB) << ", "
           << "'activeBots', " << _metrics.activeBots;

        if (!eventInfo.correlationId.empty())
        {
            std::string escapedCorrelation = eventInfo.correlationId;
            pos = 0;
            while ((pos = escapedCorrelation.find("'", pos)) != std::string::npos)
            {
                escapedCorrelation.replace(pos, 1, "''");
                pos += 2;
            }
            ss << ", 'correlationId', '" << escapedCorrelation << "'";
        }

        ss << "))";

        if (sPlayerbotDatabase->Execute(ss.str()))
        {
            LIFECYCLE_LOG_TRACE("Logged lifecycle event: {} - {}", category, type);
        }
        else
        {
            LIFECYCLE_LOG_ERROR("Failed to insert lifecycle event into database");
        }
    }
    catch (std::exception const& e)
    {
        LIFECYCLE_LOG_ERROR("Exception while logging lifecycle event: {}", e.what());
    }
}

void BotLifecycleMgr::UpdatePerformanceMetrics()
{
    auto now = std::chrono::system_clock::now();
    auto timeSinceLastMetricsUpdate = std::chrono::duration_cast<std::chrono::seconds>(now - _metrics.lastUpdate).count();

    if (timeSinceLastMetricsUpdate >= 1) // Update every second
    {
        // Calculate events per second
        _metrics.eventsProcessedPerSecond = _metrics.eventCountThisSecond;

        // Calculate average processing time
    if (_metrics.eventCountThisSecond > 0)
        {
            _metrics.averageEventProcessingTimeMs = _metrics.totalProcessingTimeThisSecond / _metrics.eventCountThisSecond;
        }

        // Reset counters
        _metrics.eventCountThisSecond = 0;
        _metrics.totalProcessingTimeThisSecond = 0;
        _metrics.lastUpdate = now;

        // Update memory usage (simplified - would use actual memory monitoring)
        _metrics.memoryUsageMB = 10; // Placeholder

        // Update bot counts from scheduler
    if (_scheduler)
        {
            _metrics.scheduledBots = _scheduler->GetScheduledBotCount();
        }
    }
}

void BotLifecycleMgr::RecordEvent(LifecycleEventInfo::Type eventType, uint32 processingTimeMs)
{
    ++_metrics.eventCountThisSecond;
    _metrics.totalProcessingTimeThisSecond += processingTimeMs;
}

void BotLifecycleMgr::NotifyEventHandlers(LifecycleEventInfo const& eventInfo)
{
    std::lock_guard lock(_handlersMutex);

    for (auto const& subscription : _eventHandlers)
    {
        if (subscription.eventType == eventInfo.eventType)
        {
            try
            {
                subscription.handler(eventInfo);
            }
            catch (std::exception const& ex)
            {
                LIFECYCLE_LOG_ERROR("Exception in event handler {}: {}", subscription.id, ex.what());
            }
        }
    }
}

uint32 BotLifecycleMgr::RegisterEventHandler(LifecycleEventInfo::Type eventType, EventHandler handler)
{
    std::lock_guard lock(_handlersMutex);

    uint32 id = _nextHandlerId++;
    _eventHandlers.push_back({id, eventType, std::move(handler)});

    LIFECYCLE_LOG_DEBUG("Registered event handler {} for event type {}", id, static_cast<uint32>(eventType));
    return id;
}

void BotLifecycleMgr::UnregisterEventHandler(uint32 handlerId)
{
    std::lock_guard lock(_handlersMutex);

    _eventHandlers.erase(
        std::remove_if(_eventHandlers.begin(), _eventHandlers.end(),
            [handlerId](EventSubscription const& sub) { return sub.id == handlerId; }),
        _eventHandlers.end());

    LIFECYCLE_LOG_DEBUG("Unregistered event handler {}", handlerId);
}

std::string BotLifecycleMgr::GenerateCorrelationId()
{
    static std::atomic<uint32> counter{0};
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    return "LIFECYCLE_" + std::to_string(now) + "_" + std::to_string(++counter);
}

bool BotLifecycleMgr::IsHealthy() const
{
    return _running &&
           _consecutiveErrors < MAX_CONSECUTIVE_ERRORS &&
           _healthCheckFailures < MAX_HEALTH_CHECK_FAILURES &&
           _scheduler != nullptr &&
           _spawner != nullptr;
}

void BotLifecycleMgr::RunMaintenance()
{
    LIFECYCLE_LOG_INFO("Running lifecycle maintenance...");

    // Clean up old events
    CleanupOldEvents();

    // Optimize performance
    OptimizePerformance();

    // Validate system health
    ValidateSystemHealth();

    LIFECYCLE_LOG_INFO("Lifecycle maintenance completed");
}

void BotLifecycleMgr::CleanupOldEvents()
{
    // Clean up old lifecycle events from database
    // NOTE: Using direct SQL execution for now. Should convert to prepared statements
    // when PlayerbotDatabase prepared statement infrastructure is fully ready.

    try
    {
        // Clean up events older than 7 days (DEBUG and INFO only)
        // Keep WARNING, ERROR, CRITICAL events for 14 days
        std::ostringstream ss;
        ss << "DELETE FROM bot_lifecycle_events "
           << "WHERE timestamp < DATE_SUB(NOW(), INTERVAL 7 DAY) "
           << "AND severity IN ('DEBUG', 'INFO') "
           << "AND processed = 1";

        if (sPlayerbotDatabase->Execute(ss.str()))
        {
            LIFECYCLE_LOG_DEBUG("Cleaned up old DEBUG/INFO lifecycle events (> 7 days)");
        }

        // Clean up old WARNING/ERROR/CRITICAL events (> 14 days)
        ss.str("");
        ss.clear();
        ss << "DELETE FROM bot_lifecycle_events "
           << "WHERE timestamp < DATE_SUB(NOW(), INTERVAL 14 DAY) "
           << "AND severity IN ('WARNING', 'ERROR', 'CRITICAL') "
           << "AND processed = 1";

        if (sPlayerbotDatabase->Execute(ss.str()))
        {
            LIFECYCLE_LOG_DEBUG("Cleaned up old WARNING/ERROR/CRITICAL lifecycle events (> 14 days)");
        }

        // Clean up spawn logs older than 30 days (from legacy table)
        ss.str("");
        ss.clear();
        ss << "DELETE FROM playerbot_spawn_log "
           << "WHERE event_timestamp < DATE_SUB(NOW(), INTERVAL 30 DAY)";

        if (sPlayerbotDatabase->Execute(ss.str()))
        {
            LIFECYCLE_LOG_DEBUG("Cleaned up old spawn logs (> 30 days)");
        }

        // Optimize tables after cleanup to reclaim space
        sPlayerbotDatabase->Execute("OPTIMIZE TABLE bot_lifecycle_events");

        LIFECYCLE_LOG_INFO("Database cleanup completed successfully");
    }
    catch (std::exception const& e)
    {
        LIFECYCLE_LOG_ERROR("Exception while cleaning up old events: {}", e.what());
    }
}

void BotLifecycleMgr::OptimizePerformance()
{
    // Reset failed spawns counter every hour
    _metrics.failedSpawnsLastHour = 0;

    // Clear correlation tracking for old events
    std::lock_guard lock(_correlationMutex);
    _correlatedEvents.clear();
}

void BotLifecycleMgr::ValidateSystemHealth()
{
    bool healthy = IsHealthy();

    if (!healthy)
    {
        ++_healthCheckFailures;
        LIFECYCLE_LOG_WARN("System health check failed. Failures: {}", _healthCheckFailures.load());

        if (_healthCheckFailures >= MAX_HEALTH_CHECK_FAILURES)
        {
            LIFECYCLE_LOG_ERROR("Multiple health check failures, initiating emergency shutdown");
            EmergencyShutdown();
        }
    }
    else
    {
        _healthCheckFailures = 0;
        _consecutiveErrors = 0;
    }
}

void BotLifecycleMgr::EmergencyShutdown()
{
    LIFECYCLE_LOG_ERROR("EMERGENCY SHUTDOWN initiated");

    _enabled = false;
    _running = false;

    // Stop all operations immediately
    if (_scheduler)
        _scheduler->SetEnabled(false);

    if (_spawner)
        _spawner->SetEnabled(false);
}

void BotLifecycleMgr::LogPerformanceReport()
{
    auto now = std::chrono::system_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::hours>(now - _startTime).count();

    LIFECYCLE_LOG_INFO("=== Bot Lifecycle Manager Performance Report ===");
    LIFECYCLE_LOG_INFO("Uptime:                 {} hours", uptime);
    LIFECYCLE_LOG_INFO("Total Events Processed: {}", _statistics.totalLifecycleEvents);
    LIFECYCLE_LOG_INFO("Successful Spawns:      {}", _statistics.successfulSpawns);
    LIFECYCLE_LOG_INFO("Failed Spawns:          {}", _statistics.failedSpawns);
    LIFECYCLE_LOG_INFO("Scheduled Logins:       {}", _statistics.scheduledLogins);
    LIFECYCLE_LOG_INFO("Scheduled Logouts:      {}", _statistics.scheduledLogouts);

    // Load atomics before passing to logging to avoid immediate function issues
    uint32 activeBots = _metricsInternal.activeBots.load();
    uint32 scheduledBots = _metricsInternal.scheduledBots.load();
    uint32 eventsPerSec = _metricsInternal.eventsProcessedPerSecond.load();
    uint32 avgProcessingTime = _metricsInternal.averageEventProcessingTimeMs.load();
    uint32 memoryUsage = _metricsInternal.memoryUsageMB.load();

    LIFECYCLE_LOG_INFO("Active Bots: {}", activeBots);
    LIFECYCLE_LOG_INFO("Scheduled Bots: {}", scheduledBots);
    LIFECYCLE_LOG_INFO("Events/Second: {}", eventsPerSec);
    LIFECYCLE_LOG_INFO("Avg Processing Time: {}ms", avgProcessingTime);
    LIFECYCLE_LOG_INFO("Memory Usage: {}MB", memoryUsage);
    LIFECYCLE_LOG_INFO("Consecutive Errors:     {}", _consecutiveErrors.load());
    LIFECYCLE_LOG_INFO("Health Check Failures:  {}", _healthCheckFailures.load());
    LIFECYCLE_LOG_INFO("System Status:          {}", IsHealthy() ? "Healthy" : "Unhealthy");
}