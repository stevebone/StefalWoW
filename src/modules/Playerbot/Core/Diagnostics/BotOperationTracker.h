/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * Enterprise-Grade Bot Operation Tracker
 *
 * This system provides comprehensive error tracking and diagnostics for:
 * - Bot creation (JITBotFactory, BotFactory, BotCharacterCreator)
 * - Bot spawning (BotSpawner, BotSession)
 * - Queue operations (BGBotManager, LFGBotManager)
 * - Equipment (BotGearFactory, EquipmentManager)
 *
 * Features:
 * - Structured error codes with subsystem categorization
 * - Per-operation metrics (success/failure counts, rates)
 * - Error context capture (bot GUID, operation details)
 * - Aggregated failure reports
 * - Threshold-based alerting
 * - Real-time diagnostics
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>

namespace Playerbot
{

// ============================================================================
// ERROR CODE TAXONOMY
// ============================================================================

enum class BotOperationCategory : uint8
{
    CREATION = 0,       // Bot character creation
    SPAWN = 1,          // Bot session/login
    BG_QUEUE = 2,       // Battleground queue
    LFG_QUEUE = 3,      // Dungeon finder queue
    EQUIPMENT = 4,      // Gear/equipment
    LIFECYCLE = 5,      // State transitions
    DATABASE = 6,       // Database operations
    NETWORK = 7,        // Session/packet operations
    MAX_CATEGORY
};

enum class BotOperationResult : uint8
{
    SUCCESS = 0,
    FAILED = 1,
    PARTIAL = 2,        // Some operations succeeded, some failed
    TIMEOUT = 3,
    SKIPPED = 4,        // Intentionally not performed
    RETRY = 5           // Will retry
};

// Detailed error codes per category
enum class CreationErrorCode : uint16
{
    SUCCESS = 0,
    ACCOUNT_CAPACITY_EXCEEDED = 100,
    ACCOUNT_ALLOCATION_FAILED = 101,
    CHARACTER_LIMIT_REACHED = 102,
    NAME_ALLOCATION_FAILED = 103,
    INVALID_RACE_CLASS_COMBO = 104,
    PLAYER_CREATE_FAILED = 105,
    INVALID_STARTING_POSITION = 106,
    DATABASE_SAVE_FAILED = 107,
    DATABASE_COMMIT_TIMEOUT = 108,
    SESSION_CREATE_FAILED = 109,
    CLONE_ENGINE_FAILED = 110,
    TEMPLATE_NOT_FOUND = 111,
    DB2_VALIDATION_FAILED = 112
};

enum class SpawnErrorCode : uint16
{
    SUCCESS = 0,
    NO_ACCOUNT_AVAILABLE = 200,
    NO_CHARACTER_AVAILABLE = 201,
    SESSION_CREATE_FAILED = 202,
    LOGIN_FAILED = 203,
    PLAYER_NOT_CREATED = 204,
    AI_CREATE_FAILED = 205,
    LIFECYCLE_TRANSITION_FAILED = 206,
    GLOBAL_CAP_REACHED = 207,
    ZONE_CAP_REACHED = 208,
    MAP_CAP_REACHED = 209,
    THROTTLED = 210,
    CIRCUIT_BREAKER_OPEN = 211,
    CHARACTER_LOOKUP_FAILED = 212
};

enum class BGQueueErrorCode : uint16
{
    SUCCESS = 0,
    BOT_UNAVAILABLE = 300,
    BOT_IN_GROUP = 301,
    BOT_IN_BATTLEGROUND = 302,
    BOT_IN_ARENA = 303,
    BOT_ALREADY_QUEUED = 304,
    BOT_QUEUE_FULL = 305,
    BOT_DEAD = 306,
    BOT_HAS_DESERTER = 307,
    BG_TEMPLATE_NOT_FOUND = 308,
    BRACKET_NOT_FOUND = 309,
    ADD_GROUP_FAILED = 310,
    INVITATION_EXPIRED = 311,
    INVITATION_NOT_FOUND = 312,
    BG_INSTANCE_NOT_FOUND = 313,
    TELEPORT_FAILED = 314,
    INSUFFICIENT_BOTS_ALLIANCE = 315,
    INSUFFICIENT_BOTS_HORDE = 316,
    HUMAN_PLAYER_NOT_FOUND = 317
};

enum class LFGQueueErrorCode : uint16
{
    SUCCESS = 0,
    BOT_UNAVAILABLE = 400,
    BOT_IN_GROUP = 401,
    BOT_TOO_LOW_LEVEL = 402,
    BOT_HAS_DESERTER = 403,
    BOT_INVALID_STATE = 404,
    ROLE_VALIDATION_FAILED = 405,
    DUNGEON_NOT_FOUND = 406,
    JOIN_LFG_FAILED = 407,
    PROPOSAL_ACCEPT_FAILED = 408,
    ROLE_CHECK_FAILED = 409,
    GROUP_FORMATION_FAILED = 410,
    TELEPORT_FAILED = 411,
    INSUFFICIENT_TANKS = 412,
    INSUFFICIENT_HEALERS = 413,
    INSUFFICIENT_DPS = 414,
    HUMAN_PLAYER_NOT_FOUND = 415,
    JIT_BOT_TIMEOUT = 416
};

enum class EquipmentErrorCode : uint16
{
    SUCCESS = 0,
    CACHE_NOT_READY = 500,
    NO_ITEMS_FOR_SLOT = 501,
    ITEM_TEMPLATE_NOT_FOUND = 502,
    CANNOT_EQUIP_ITEM = 503,
    EQUIP_FAILED = 504,
    BAG_INSERTION_FAILED = 505,
    WRONG_ARMOR_TYPE = 506,
    WRONG_WEAPON_TYPE = 507,
    LEVEL_REQUIREMENT_NOT_MET = 508,
    CLASS_RESTRICTION = 509,
    SKILL_REQUIREMENT_NOT_MET = 510,
    SAVE_DEFERRED_NOT_EXECUTED = 511,
    EMPTY_GEAR_SET = 512,
    QUALITY_FALLBACK_USED = 513,
    UNKNOWN_CLASS_DEFAULT = 514
};

// ============================================================================
// ERROR CONTEXT
// ============================================================================

struct BotOperationError
{
    // Identification
    uint64 errorId = 0;
    std::chrono::system_clock::time_point timestamp;

    // Classification
    BotOperationCategory category = BotOperationCategory::CREATION;
    BotOperationResult result = BotOperationResult::FAILED;
    uint16 errorCode = 0;

    // Context
    ObjectGuid botGuid;
    ObjectGuid relatedGuid;      // Human player, group leader, etc.
    uint32 accountId = 0;
    uint32 contentId = 0;        // Dungeon ID, BG type, etc.

    // Details
    std::string operation;       // Method name
    std::string message;         // Human-readable description
    std::string context;         // Additional JSON context

    // Tracking
    uint32 retryCount = 0;
    bool recovered = false;
};

// ============================================================================
// METRICS
// ============================================================================

struct OperationMetrics
{
    std::atomic<uint64> totalOperations{0};
    std::atomic<uint64> successCount{0};
    std::atomic<uint64> failureCount{0};
    std::atomic<uint64> partialCount{0};
    std::atomic<uint64> timeoutCount{0};
    std::atomic<uint64> retryCount{0};

    // Recent window (last 5 minutes)
    std::atomic<uint32> recentSuccess{0};
    std::atomic<uint32> recentFailure{0};

    // Timing
    std::chrono::system_clock::time_point lastSuccess;
    std::chrono::system_clock::time_point lastFailure;
    std::chrono::system_clock::time_point windowStart;

    float GetSuccessRate() const
    {
        uint64 total = totalOperations.load();
        return total > 0 ? static_cast<float>(successCount.load()) / total : 1.0f;
    }

    float GetRecentSuccessRate() const
    {
        uint32 recentTotal = recentSuccess.load() + recentFailure.load();
        return recentTotal > 0 ? static_cast<float>(recentSuccess.load()) / recentTotal : 1.0f;
    }
};

struct CategoryMetrics
{
    OperationMetrics overall;
    std::unordered_map<uint16, OperationMetrics> byErrorCode;
};

// ============================================================================
// DIAGNOSTIC REPORT
// ============================================================================

struct DiagnosticReport
{
    std::chrono::system_clock::time_point generatedAt;
    std::chrono::seconds uptime;

    // Overall health
    float overallSuccessRate = 0.0f;
    uint64 totalOperations = 0;
    uint64 totalFailures = 0;

    // Per-category summary
    struct CategorySummary
    {
        BotOperationCategory category;
        std::string categoryName;
        float successRate = 0.0f;
        uint64 operations = 0;
        uint64 failures = 0;
        std::vector<std::pair<uint16, uint64>> topErrors; // error code -> count
    };
    std::vector<CategorySummary> categories;

    // Recent errors (last 100)
    std::vector<BotOperationError> recentErrors;

    // Alerts
    std::vector<std::string> activeAlerts;
};

// ============================================================================
// BOT OPERATION TRACKER (Singleton)
// ============================================================================

class TC_GAME_API BotOperationTracker
{
public:
    static BotOperationTracker* instance();

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    void Initialize();
    void Shutdown();
    void Update(uint32 diff);

    // ========================================================================
    // ERROR RECORDING
    // ========================================================================

    /// Record a successful operation
    void RecordSuccess(BotOperationCategory category, std::string_view operation,
                       ObjectGuid botGuid = ObjectGuid::Empty);

    /// Record a failed operation with error code
    uint64 RecordError(BotOperationCategory category, uint16 errorCode,
                       std::string_view operation, std::string_view message,
                       ObjectGuid botGuid = ObjectGuid::Empty,
                       ObjectGuid relatedGuid = ObjectGuid::Empty,
                       uint32 accountId = 0, uint32 contentId = 0);

    /// Record error with full context
    uint64 RecordError(BotOperationError const& error);

    /// Record a partial success (some operations succeeded)
    void RecordPartial(BotOperationCategory category, std::string_view operation,
                       uint32 successCount, uint32 failCount,
                       ObjectGuid botGuid = ObjectGuid::Empty);

    /// Record a retry attempt
    void RecordRetry(uint64 errorId);

    /// Record recovery from error
    void RecordRecovery(uint64 errorId);

    // ========================================================================
    // CONVENIENCE METHODS
    // ========================================================================

    // Creation errors
    uint64 RecordCreationError(CreationErrorCode code, std::string_view message,
                               ObjectGuid botGuid = ObjectGuid::Empty,
                               uint32 accountId = 0);

    // Spawn errors
    uint64 RecordSpawnError(SpawnErrorCode code, std::string_view message,
                            ObjectGuid botGuid = ObjectGuid::Empty,
                            uint32 accountId = 0);

    // BG queue errors
    uint64 RecordBGQueueError(BGQueueErrorCode code, std::string_view message,
                              ObjectGuid botGuid = ObjectGuid::Empty,
                              ObjectGuid humanGuid = ObjectGuid::Empty,
                              uint32 bgTypeId = 0);

    // LFG queue errors
    uint64 RecordLFGQueueError(LFGQueueErrorCode code, std::string_view message,
                               ObjectGuid botGuid = ObjectGuid::Empty,
                               ObjectGuid humanGuid = ObjectGuid::Empty,
                               uint32 dungeonId = 0);

    // Equipment errors
    uint64 RecordEquipmentError(EquipmentErrorCode code, std::string_view message,
                                ObjectGuid botGuid = ObjectGuid::Empty,
                                uint32 itemEntry = 0, uint8 slot = 0);

    // ========================================================================
    // QUERIES
    // ========================================================================

    /// Get metrics for category
    CategoryMetrics const& GetCategoryMetrics(BotOperationCategory category) const;

    /// Get overall success rate
    float GetOverallSuccessRate() const;

    /// Get recent errors for category
    std::vector<BotOperationError> GetRecentErrors(BotOperationCategory category,
                                                    uint32 maxCount = 50) const;

    /// Get error by ID
    BotOperationError const* GetError(uint64 errorId) const;

    /// Generate diagnostic report
    DiagnosticReport GenerateReport() const;

    /// Print status to log
    void PrintStatus() const;

    // ========================================================================
    // ALERTING
    // ========================================================================

    /// Check if category is in alert state (high failure rate)
    bool IsAlertActive(BotOperationCategory category) const;

    /// Get active alert messages
    std::vector<std::string> GetActiveAlerts() const;

    /// Set alert threshold (failure rate to trigger alert)
    void SetAlertThreshold(BotOperationCategory category, float threshold);

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    void SetEnabled(bool enabled) { _enabled = enabled; }
    bool IsEnabled() const { return _enabled; }
    void SetMaxRecentErrors(uint32 max) { _maxRecentErrors = max; }
    void SetMetricsWindowSeconds(uint32 seconds) { _metricsWindowSeconds = seconds; }

private:
    BotOperationTracker() = default;
    ~BotOperationTracker() = default;

    // Error storage
    std::deque<BotOperationError> _recentErrors;
    std::unordered_map<uint64, size_t> _errorIndex; // errorId -> index in deque
    std::atomic<uint64> _nextErrorId{1};
    mutable std::mutex _errorMutex;

    // Metrics per category
    std::array<CategoryMetrics, static_cast<size_t>(BotOperationCategory::MAX_CATEGORY)> _metrics;
    mutable std::mutex _metricsMutex;

    // Alerting
    std::array<float, static_cast<size_t>(BotOperationCategory::MAX_CATEGORY)> _alertThresholds;
    std::array<bool, static_cast<size_t>(BotOperationCategory::MAX_CATEGORY)> _alertActive;

    // Configuration
    std::atomic<bool> _enabled{true};
    uint32 _maxRecentErrors = 1000;
    uint32 _metricsWindowSeconds = 300; // 5 minutes

    // Timing
    std::chrono::system_clock::time_point _startTime;
    uint32 _updateAccumulator = 0;
    static constexpr uint32 METRICS_UPDATE_INTERVAL = 60000; // 1 minute
    static constexpr uint32 ALERT_CHECK_INTERVAL = 10000;    // 10 seconds

    // Helpers
    void UpdateMetricsWindow();
    void CheckAlerts();
    std::string CategoryToString(BotOperationCategory category) const;
    std::string ErrorCodeToString(BotOperationCategory category, uint16 code) const;
    void LogError(BotOperationError const& error);
};

// ============================================================================
// MACROS FOR EASY TRACKING
// ============================================================================

#define BOT_TRACK_SUCCESS(category, operation, botGuid) \
    BotOperationTracker::instance()->RecordSuccess(category, operation, botGuid)

#define BOT_TRACK_CREATION_ERROR(code, message, botGuid, accountId) \
    BotOperationTracker::instance()->RecordCreationError(code, message, botGuid, accountId)

#define BOT_TRACK_SPAWN_ERROR(code, message, botGuid, accountId) \
    BotOperationTracker::instance()->RecordSpawnError(code, message, botGuid, accountId)

#define BOT_TRACK_BG_ERROR(code, message, botGuid, humanGuid, bgTypeId) \
    BotOperationTracker::instance()->RecordBGQueueError(code, message, botGuid, humanGuid, bgTypeId)

#define BOT_TRACK_LFG_ERROR(code, message, botGuid, humanGuid, dungeonId) \
    BotOperationTracker::instance()->RecordLFGQueueError(code, message, botGuid, humanGuid, dungeonId)

#define BOT_TRACK_EQUIPMENT_ERROR(code, message, botGuid, itemEntry, slot) \
    BotOperationTracker::instance()->RecordEquipmentError(code, message, botGuid, itemEntry, slot)

} // namespace Playerbot

#define sBotOperationTracker Playerbot::BotOperationTracker::instance()
