/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#ifndef BOT_HEALTH_CHECK_H
#define BOT_HEALTH_CHECK_H

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "ObjectGuid.h"
#include <vector>
#include <unordered_set>
#include <mutex>
#include <atomic>

namespace Playerbot {

/**
 * ENTERPRISE-GRADE HEALTH MONITORING
 *
 * Comprehensive health checks and anomaly detection for 5000+ bots
 *
 * Features:
 * - Stall detection (bots not updating)
 * - Deadlock detection (system-wide hangs)
 * - Error rate monitoring
 * - Automatic recovery mechanisms
 * - Health status reporting
 * - Alert triggering
 */

/**
 * Health status for individual bots or the system
 */
enum class HealthStatus : uint8
{
    HEALTHY = 0,        // Operating normally
    DEGRADED = 1,       // Operating but with reduced performance
    UNHEALTHY = 2,      // Significant problems detected
    CRITICAL = 3        // Critical failure requiring immediate action
};

/**
 * Health check result for a bot or system component
 */
struct HealthCheckResult
{
    HealthStatus status{HealthStatus::HEALTHY};
    ::std::string component;
    ::std::string message;
    uint32 timestamp{0};
};

/**
 * Thread-safe health monitoring system
 *
 * Responsibilities:
 * - Detect stalled bots (not updating for extended period)
 * - Detect system deadlocks (entire system not progressing)
 * - Monitor error rates and trigger alerts
 * - Track health history for trend analysis
 * - Provide recovery recommendations
 * - Generate health reports
 */
class TC_GAME_API BotHealthCheck final
{
public:
    static BotHealthCheck* instance()
    {
        static BotHealthCheck instance;
        return &instance;
    }

    // IBotHealthCheck interface implementation
    bool Initialize();
    void Shutdown();
    void PerformHealthChecks(uint32 currentTime);

    // Stall detection
    void CheckForStalledBots(uint32 currentTime);
    ::std::vector<ObjectGuid> GetStalledBots() const;
    bool IsBotStalled(ObjectGuid botGuid) const;

    // Deadlock detection
    void CheckForDeadlocks(uint32 currentTime);
    bool IsSystemDeadlocked() const { return _systemDeadlocked.load(); }
    uint32 GetTimeSinceLastProgress() const;

    // Error rate monitoring
    void RecordError(ObjectGuid botGuid, ::std::string const& errorType);
    float GetSystemErrorRate() const;
    bool IsErrorRateExcessive() const;

    // System health
    HealthStatus GetSystemHealth() const;
    HealthStatus GetBotHealth(ObjectGuid botGuid) const;
    ::std::vector<HealthCheckResult> GetRecentHealthIssues() const;

    // Recovery actions
    void TriggerAutomaticRecovery(ObjectGuid botGuid);
    void TriggerSystemRecovery();

    // Configuration
    void SetStallThreshold(uint32 milliseconds) { _stallThresholdMs = milliseconds; }
    void SetDeadlockThreshold(uint32 milliseconds) { _deadlockThresholdMs = milliseconds; }
    void SetErrorRateThreshold(float errorsPerSecond) { _errorRateThreshold = errorsPerSecond; }
    void SetAutoRecoveryEnabled(bool enabled) { _autoRecoveryEnabled.store(enabled); }

    uint32 GetStallThreshold() const { return _stallThresholdMs; }
    uint32 GetDeadlockThreshold() const { return _deadlockThresholdMs; }
    float GetErrorRateThreshold() const { return _errorRateThreshold; }
    bool IsAutoRecoveryEnabled() const { return _autoRecoveryEnabled.load(); }

    // Heartbeat (called every tick to track system progress)
    void RecordHeartbeat(uint32 currentTime);

    // Reporting
    void LogHealthReport() const;
    void LogDetailedHealthStatus() const;

    // Administrative
    void ClearStalledBot(ObjectGuid botGuid);
    void ClearAllHealthIssues();

private:
    BotHealthCheck() = default;
    ~BotHealthCheck() = default;
    BotHealthCheck(const BotHealthCheck&) = delete;
    BotHealthCheck& operator=(const BotHealthCheck&) = delete;

    // Stall tracking
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::SESSION_MANAGER> _stalledBotsMutex;
    ::std::unordered_set<ObjectGuid> _stalledBots;
    uint32 _stallThresholdMs{5000}; // 5 seconds

    // Deadlock tracking
    ::std::atomic<bool> _systemDeadlocked{false};
    uint32 _lastHeartbeatTime{0};
    uint32 _deadlockThresholdMs{10000}; // 10 seconds
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::SESSION_MANAGER> _heartbeatMutex;

    // Error tracking
    struct ErrorRecord
    {
        ObjectGuid botGuid;
        ::std::string errorType;
        uint32 timestamp;
    };
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::SESSION_MANAGER> _errorsMutex;
    ::std::vector<ErrorRecord> _recentErrors;
    static constexpr uint32 ERROR_HISTORY_DURATION_MS = 60000; // 1 minute
    float _errorRateThreshold{10.0f}; // 10 errors per second

    // Health issues history
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::SESSION_MANAGER> _healthIssuesMutex;
    ::std::vector<HealthCheckResult> _healthIssues;
    static constexpr uint32 HEALTH_ISSUE_HISTORY_SIZE = 100;

    // Auto-recovery
    ::std::atomic<bool> _autoRecoveryEnabled{true};
    uint32 _lastRecoveryTime{0};
    static constexpr uint32 RECOVERY_COOLDOWN_MS = 30000; // 30 seconds between recoveries

    // Health check intervals
    uint32 _lastStallCheck{0};
    uint32 _lastDeadlockCheck{0};
    uint32 _lastErrorCheck{0};
    static constexpr uint32 STALL_CHECK_INTERVAL_MS = 1000;      // Check every 1 second
    static constexpr uint32 DEADLOCK_CHECK_INTERVAL_MS = 2000;   // Check every 2 seconds
    static constexpr uint32 ERROR_CHECK_INTERVAL_MS = 5000;      // Check every 5 seconds

    // Helper methods
    void AddHealthIssue(HealthStatus status, ::std::string const& component, ::std::string const& message, uint32 currentTime);
    void PruneOldErrors(uint32 currentTime);
    void PruneHealthIssues();

    // Initialization state
    ::std::atomic<bool> _initialized{false};
};

// Global instance accessor
#define sBotHealthCheck BotHealthCheck::instance()

} // namespace Playerbot

#endif // BOT_HEALTH_CHECK_H
