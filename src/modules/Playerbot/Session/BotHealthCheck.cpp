/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#include "BotHealthCheck.h"
#include "BotPriorityManager.h"
#include "Log.h"
#include "Timer.h"
#include "GameTime.h"
#include <algorithm>

namespace Playerbot {

bool BotHealthCheck::Initialize()
{
    if (_initialized.load())
        return true;

    TC_LOG_INFO("module.playerbot", "BotHealthCheck: Initializing enterprise health monitoring...");

    _stalledBots.clear();
    _recentErrors.clear();
    _healthIssues.clear();
    _systemDeadlocked.store(false);
    _lastHeartbeatTime = GameTime::GetGameTimeMS();

    _initialized.store(true);
    TC_LOG_INFO("module.playerbot", "BotHealthCheck: Health monitoring initialized successfully");
    TC_LOG_INFO("module.playerbot", "  Stall threshold: {}ms", _stallThresholdMs);
    TC_LOG_INFO("module.playerbot", "  Deadlock threshold: {}ms", _deadlockThresholdMs);
    TC_LOG_INFO("module.playerbot", "  Error rate threshold: {:.1f} errors/sec", _errorRateThreshold);
    TC_LOG_INFO("module.playerbot", "  Auto-recovery: {}", _autoRecoveryEnabled.load() ? "ENABLED" : "DISABLED");

    return true;
}

void BotHealthCheck::Shutdown()
{
    TC_LOG_INFO("module.playerbot", "BotHealthCheck: Shutting down...");

    // Log final health status
    LogDetailedHealthStatus();

    {
        ::std::lock_guard lock1(_stalledBotsMutex);
        _stalledBots.clear();
    }

    {
        ::std::lock_guard lock2(_errorsMutex);
        _recentErrors.clear();
    }

    {
        ::std::lock_guard lock3(_healthIssuesMutex);
        _healthIssues.clear();
    }

    _initialized.store(false);
    TC_LOG_INFO("module.playerbot", "BotHealthCheck: Shutdown complete");
}

void BotHealthCheck::PerformHealthChecks(uint32 currentTime)
{
    // Stall detection
    if (currentTime - _lastStallCheck >= STALL_CHECK_INTERVAL_MS)
    {
        CheckForStalledBots(currentTime);
        _lastStallCheck = currentTime;
    }

    // Deadlock detection
    if (currentTime - _lastDeadlockCheck >= DEADLOCK_CHECK_INTERVAL_MS)
    {
        CheckForDeadlocks(currentTime);
        _lastDeadlockCheck = currentTime;
    }

    // Error rate monitoring
    if (currentTime - _lastErrorCheck >= ERROR_CHECK_INTERVAL_MS)
    {
        PruneOldErrors(currentTime);

        if (IsErrorRateExcessive())
        {
            AddHealthIssue(HealthStatus::UNHEALTHY, "ErrorRate",
                "Excessive error rate detected", currentTime);

            TC_LOG_ERROR("module.playerbot.health",
                "ERROR RATE EXCESSIVE: {:.2f} errors/sec (threshold: {:.2f})",
                GetSystemErrorRate(), _errorRateThreshold);
        }

        _lastErrorCheck = currentTime;
    }
}

void BotHealthCheck::CheckForStalledBots(uint32 currentTime)
{
    // Use BotPriorityManager to detect stalled bots
    sBotPriorityMgr->DetectStalledBots(currentTime, _stallThresholdMs);

    ::std::vector<ObjectGuid> stalledBots = sBotPriorityMgr->GetStalledBots();

    if (stalledBots.empty())
    {
        // Clear previously stalled bots if they recovered
        ::std::lock_guard lock(_stalledBotsMutex);
        if (!_stalledBots.empty())
        {
            TC_LOG_INFO("module.playerbot.health", "All previously stalled bots have recovered");
            _stalledBots.clear();
        }
        return;
    }

    // Update stalled bots set
    {
        ::std::lock_guard lock(_stalledBotsMutex);

        // Find new stalls
    for (ObjectGuid guid : stalledBots)
        {
            if (_stalledBots.insert(guid).second) // Newly stalled
            {
                AddHealthIssue(HealthStatus::UNHEALTHY, "BotStall",
                    "Bot " + guid.ToString() + " is stalled", currentTime);

                TC_LOG_ERROR("module.playerbot.health", "Bot {} detected as STALLED", guid.ToString());

                // Trigger auto-recovery if enabled
    if (_autoRecoveryEnabled.load())
                {
                    TriggerAutomaticRecovery(guid);
                }
            }
        }
    }

    // Log summary if many bots are stalled
    if (stalledBots.size() > 10)
    {
        TC_LOG_ERROR("module.playerbot.health",
            "CRITICAL: {} bots are stalled! System may be overloaded.",
            stalledBots.size());

        AddHealthIssue(HealthStatus::CRITICAL, "MassStall",
            "Large number of bots stalled: " + ::std::to_string(stalledBots.size()), currentTime);
    }
}

::std::vector<ObjectGuid> BotHealthCheck::GetStalledBots() const
{
    ::std::lock_guard lock(_stalledBotsMutex);
    return ::std::vector<ObjectGuid>(_stalledBots.begin(), _stalledBots.end());
}

bool BotHealthCheck::IsBotStalled(ObjectGuid botGuid) const
{
    ::std::lock_guard lock(_stalledBotsMutex);
    return _stalledBots.find(botGuid) != _stalledBots.end();
}

void BotHealthCheck::CheckForDeadlocks(uint32 currentTime)
{
    ::std::lock_guard lock(_heartbeatMutex);

    uint32 timeSinceHeartbeat = currentTime - _lastHeartbeatTime;

    if (timeSinceHeartbeat > _deadlockThresholdMs)
    {
        if (!_systemDeadlocked.load())
        {
            _systemDeadlocked.store(true);

            AddHealthIssue(HealthStatus::CRITICAL, "SystemDeadlock",
                "System deadlock detected - no heartbeat for " + ::std::to_string(timeSinceHeartbeat) + "ms",
                currentTime);

            TC_LOG_FATAL("module.playerbot.health",
                "SYSTEM DEADLOCK DETECTED! No heartbeat for {}ms (threshold: {}ms)",
                timeSinceHeartbeat, _deadlockThresholdMs);

            // Trigger system-wide recovery
    if (_autoRecoveryEnabled.load())
            {
                TriggerSystemRecovery();
            }
        }
    }
    else
    {
        if (_systemDeadlocked.load())
        {
            _systemDeadlocked.store(false);
            TC_LOG_INFO("module.playerbot.health", "System deadlock resolved - heartbeat restored");
        }
    }
}

uint32 BotHealthCheck::GetTimeSinceLastProgress() const
{
    ::std::lock_guard lock(_heartbeatMutex);
    return GameTime::GetGameTimeMS() - _lastHeartbeatTime;
}

void BotHealthCheck::RecordHeartbeat(uint32 currentTime)
{
    ::std::lock_guard lock(_heartbeatMutex);
    _lastHeartbeatTime = currentTime;

    // Clear deadlock flag if set
    if (_systemDeadlocked.load())
    {
        _systemDeadlocked.store(false);
        TC_LOG_INFO("module.playerbot.health", "System heartbeat restored - deadlock cleared");
    }
}

void BotHealthCheck::RecordError(ObjectGuid botGuid, ::std::string const& errorType)
{
    ::std::lock_guard lock(_errorsMutex);

    ErrorRecord error;
    error.botGuid = botGuid;
    error.errorType = errorType;
    error.timestamp = GameTime::GetGameTimeMS();

    _recentErrors.push_back(error);

    // Limit error history size
    if (_recentErrors.size() > 1000)
    {
        _recentErrors.erase(_recentErrors.begin(), _recentErrors.begin() + 100);
    }
}

float BotHealthCheck::GetSystemErrorRate() const
{
    ::std::lock_guard lock(_errorsMutex);

    if (_recentErrors.empty())
        return 0.0f;

    uint32 currentTime = GameTime::GetGameTimeMS();
    uint32 oldestTime = _recentErrors.front().timestamp;
    uint32 durationMs = currentTime - oldestTime;

    if (durationMs == 0)
        return 0.0f;

    // Errors per second
    return (static_cast<float>(_recentErrors.size()) / durationMs) * 1000.0f;
}

bool BotHealthCheck::IsErrorRateExcessive() const
{
    return GetSystemErrorRate() > _errorRateThreshold;
}

HealthStatus BotHealthCheck::GetSystemHealth() const
{
    // CRITICAL: System deadlocked
    if (_systemDeadlocked.load())
        return HealthStatus::CRITICAL;

    // CRITICAL: Large number of stalled bots
    {
        ::std::lock_guard lock(_stalledBotsMutex);
        if (_stalledBots.size() > 50)
            return HealthStatus::CRITICAL;
    }

    // UNHEALTHY: Excessive error rate
    if (IsErrorRateExcessive())
        return HealthStatus::UNHEALTHY;

    // DEGRADED: Some bots stalled or moderate errors
    {
        ::std::lock_guard lock(_stalledBotsMutex);
        if (!_stalledBots.empty())
            return HealthStatus::DEGRADED;
    }

    return HealthStatus::HEALTHY;
}

HealthStatus BotHealthCheck::GetBotHealth(ObjectGuid botGuid) const
{
    // Check if bot is stalled
    if (IsBotStalled(botGuid))
        return HealthStatus::UNHEALTHY;

    // Check bot metrics from priority manager
    BotUpdateMetrics const* metrics = sBotPriorityMgr->GetMetrics(botGuid);
    if (!metrics)
        return HealthStatus::HEALTHY; // No data yet

    // Check for excessive errors
    if (metrics->errorCount > 10)
        return HealthStatus::DEGRADED;

    // Check for suspended status
    if (metrics->currentPriority == BotPriority::SUSPENDED)
        return HealthStatus::DEGRADED;

    return HealthStatus::HEALTHY;
}

::std::vector<HealthCheckResult> BotHealthCheck::GetRecentHealthIssues() const
{
    ::std::lock_guard lock(_healthIssuesMutex);
    return _healthIssues;
}

void BotHealthCheck::TriggerAutomaticRecovery(ObjectGuid botGuid)
{
    uint32 currentTime = GameTime::GetGameTimeMS();

    // Cooldown check
    if (currentTime - _lastRecoveryTime < RECOVERY_COOLDOWN_MS)
        return;

    TC_LOG_WARN("module.playerbot.health",
        "Attempting automatic recovery for bot {}", botGuid.ToString());

    // Recovery action: Promote to EMERGENCY priority for immediate attention
    sBotPriorityMgr->SetPriority(botGuid, BotPriority::EMERGENCY);

    _lastRecoveryTime = currentTime;
}

void BotHealthCheck::TriggerSystemRecovery()
{
    uint32 currentTime = GameTime::GetGameTimeMS();

    // Cooldown check
    if (currentTime - _lastRecoveryTime < RECOVERY_COOLDOWN_MS)
        return;

    TC_LOG_ERROR("module.playerbot.health", "Attempting system-wide recovery...");

    // System recovery actions:
    // 1. Clear all stalled bot flags
    {
        ::std::lock_guard lock(_stalledBotsMutex);
        _stalledBots.clear();
    }

    // 2. Log priority distribution
    sBotPriorityMgr->LogPriorityDistribution();

    // 3. Force heartbeat
    RecordHeartbeat(currentTime);

    _lastRecoveryTime = currentTime;

    TC_LOG_INFO("module.playerbot.health", "System recovery completed");
}

void BotHealthCheck::ClearStalledBot(ObjectGuid botGuid)
{
    ::std::lock_guard lock(_stalledBotsMutex);
    _stalledBots.erase(botGuid);
}

void BotHealthCheck::ClearAllHealthIssues()
{
    {
        ::std::lock_guard lock1(_stalledBotsMutex);
        _stalledBots.clear();
    }

    {
        ::std::lock_guard lock2(_errorsMutex);
        _recentErrors.clear();
    }

    {
        ::std::lock_guard lock3(_healthIssuesMutex);
        _healthIssues.clear();
    }

    _systemDeadlocked.store(false);

    TC_LOG_INFO("module.playerbot.health", "All health issues cleared");
}

void BotHealthCheck::LogHealthReport() const
{
    HealthStatus systemHealth = GetSystemHealth();
    const char* healthStr[] = { "HEALTHY", "DEGRADED", "UNHEALTHY", "CRITICAL" };

    TC_LOG_INFO("module.playerbot.health",
        "=== HEALTH CHECK REPORT ===");
    TC_LOG_INFO("module.playerbot.health",
        "System Health: {}", healthStr[static_cast<uint8>(systemHealth)]);

    ::std::lock_guard lock(_stalledBotsMutex);
    if (!_stalledBots.empty())
    {
        TC_LOG_WARN("module.playerbot.health",
            "Stalled Bots: {} detected", _stalledBots.size());
    }

    if (_systemDeadlocked.load())
    {
        TC_LOG_FATAL("module.playerbot.health",
            "DEADLOCK: System is deadlocked!");
    }

    float errorRate = GetSystemErrorRate();
    if (errorRate > 0.0f)
    {
        TC_LOG_INFO("module.playerbot.health",
            "Error Rate: {:.2f} errors/sec (threshold: {:.2f})",
            errorRate, _errorRateThreshold);
    }
}

void BotHealthCheck::LogDetailedHealthStatus() const
{
    TC_LOG_INFO("module.playerbot.health",
        "=== DETAILED HEALTH STATUS ===");

    HealthStatus systemHealth = GetSystemHealth();
    const char* healthStr[] = { "HEALTHY", "DEGRADED", "UNHEALTHY", "CRITICAL" };

    TC_LOG_INFO("module.playerbot.health",
        "System Health: {}", healthStr[static_cast<uint8>(systemHealth)]);

    // Stall status
    {
        ::std::lock_guard lock(_stalledBotsMutex);
        TC_LOG_INFO("module.playerbot.health",
            "Stalled Bots: {} | Threshold: {}ms",
            _stalledBots.size(), _stallThresholdMs);
    }

    // Deadlock status
    {
        ::std::lock_guard lock(_heartbeatMutex);
        uint32 timeSinceHeartbeat = GameTime::GetGameTimeMS() - _lastHeartbeatTime;
        TC_LOG_INFO("module.playerbot.health",
            "Deadlock Status: {} | Time since heartbeat: {}ms | Threshold: {}ms",
            _systemDeadlocked.load() ? "DEADLOCKED" : "Normal",
            timeSinceHeartbeat, _deadlockThresholdMs);
    }

    // Error status
    {
        ::std::lock_guard lock(_errorsMutex);
        float errorRate = GetSystemErrorRate();
        TC_LOG_INFO("module.playerbot.health",
            "Error Rate: {:.2f} errors/sec | Recent Errors: {} | Threshold: {:.2f} errors/sec",
            errorRate, _recentErrors.size(), _errorRateThreshold);
    }

    // Recent health issues
    {
        ::std::lock_guard lock(_healthIssuesMutex);
        if (!_healthIssues.empty())
        {
            TC_LOG_INFO("module.playerbot.health",
                "Recent Health Issues: {} recorded", _healthIssues.size());

            // Log last 5 issues
            size_t count = ::std::min(size_t(5), _healthIssues.size());
            for (size_t i = _healthIssues.size() - count; i < _healthIssues.size(); ++i)
            {
                auto const& issue = _healthIssues[i];
                TC_LOG_INFO("module.playerbot.health",
                    "  [{}] {}: {}",
                    healthStr[static_cast<uint8>(issue.status)],
                    issue.component,
                    issue.message);
            }
        }
    }
}

void BotHealthCheck::AddHealthIssue(HealthStatus status, ::std::string const& component, ::std::string const& message, uint32 currentTime)
{
    ::std::lock_guard lock(_healthIssuesMutex);

    HealthCheckResult issue;
    issue.status = status;
    issue.component = component;
    issue.message = message;
    issue.timestamp = currentTime;

    _healthIssues.push_back(issue);

    PruneHealthIssues();
}

void BotHealthCheck::PruneOldErrors(uint32 currentTime)
{
    ::std::lock_guard lock(_errorsMutex);

    // Remove errors older than ERROR_HISTORY_DURATION_MS
    _recentErrors.erase(
        ::std::remove_if(_recentErrors.begin(), _recentErrors.end(),
            [currentTime](ErrorRecord const& error) {
                return (currentTime - error.timestamp) > ERROR_HISTORY_DURATION_MS;
            }),
        _recentErrors.end()
    );
}

void BotHealthCheck::PruneHealthIssues()
{
    // Keep only last HEALTH_ISSUE_HISTORY_SIZE issues
    if (_healthIssues.size() > HEALTH_ISSUE_HISTORY_SIZE)
    {
        _healthIssues.erase(_healthIssues.begin(),
            _healthIssues.begin() + (_healthIssues.size() - HEALTH_ISSUE_HISTORY_SIZE));
    }
}

} // namespace Playerbot
