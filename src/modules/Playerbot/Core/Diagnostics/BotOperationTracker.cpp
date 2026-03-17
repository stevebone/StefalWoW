/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * Enterprise-Grade Bot Operation Tracker - Implementation
 */

#include "BotOperationTracker.h"
#include "Log.h"
#include "GameTime.h"
#include <sstream>
#include <iomanip>

namespace Playerbot
{

BotOperationTracker* BotOperationTracker::instance()
{
    static BotOperationTracker instance;
    return &instance;
}

void BotOperationTracker::Initialize()
{
    _startTime = std::chrono::system_clock::now();

    // Set default alert thresholds (trigger alert if success rate drops below)
    _alertThresholds[static_cast<size_t>(BotOperationCategory::CREATION)] = 0.90f;
    _alertThresholds[static_cast<size_t>(BotOperationCategory::SPAWN)] = 0.85f;
    _alertThresholds[static_cast<size_t>(BotOperationCategory::BG_QUEUE)] = 0.80f;
    _alertThresholds[static_cast<size_t>(BotOperationCategory::LFG_QUEUE)] = 0.80f;
    _alertThresholds[static_cast<size_t>(BotOperationCategory::EQUIPMENT)] = 0.95f;
    _alertThresholds[static_cast<size_t>(BotOperationCategory::LIFECYCLE)] = 0.90f;
    _alertThresholds[static_cast<size_t>(BotOperationCategory::DATABASE)] = 0.95f;
    _alertThresholds[static_cast<size_t>(BotOperationCategory::NETWORK)] = 0.90f;

    // Reset alert states
    for (size_t i = 0; i < static_cast<size_t>(BotOperationCategory::MAX_CATEGORY); ++i)
        _alertActive[i] = false;

    // Initialize metrics window start
    auto now = std::chrono::system_clock::now();
    for (auto& categoryMetrics : _metrics)
        categoryMetrics.overall.windowStart = now;

    TC_LOG_INFO("module.playerbot.diagnostics",
        "BotOperationTracker initialized - tracking {} operation categories",
        static_cast<size_t>(BotOperationCategory::MAX_CATEGORY));
}

void BotOperationTracker::Shutdown()
{
    PrintStatus();
    TC_LOG_INFO("module.playerbot.diagnostics", "BotOperationTracker shutdown");
}

void BotOperationTracker::Update(uint32 diff)
{
    if (!_enabled)
        return;

    _updateAccumulator += diff;

    // Update metrics window periodically
    if (_updateAccumulator >= METRICS_UPDATE_INTERVAL)
    {
        UpdateMetricsWindow();
        _updateAccumulator = 0;
    }

    // Check alerts more frequently
    static uint32 alertAccumulator = 0;
    alertAccumulator += diff;
    if (alertAccumulator >= ALERT_CHECK_INTERVAL)
    {
        CheckAlerts();
        alertAccumulator = 0;
    }
}

// ============================================================================
// ERROR RECORDING
// ============================================================================

void BotOperationTracker::RecordSuccess(BotOperationCategory category, std::string_view operation,
                                         ObjectGuid botGuid)
{
    if (!_enabled)
        return;

    size_t catIndex = static_cast<size_t>(category);
    auto& metrics = _metrics[catIndex].overall;

    metrics.totalOperations++;
    metrics.successCount++;
    metrics.recentSuccess++;
    metrics.lastSuccess = std::chrono::system_clock::now();

    TC_LOG_TRACE("module.playerbot.diagnostics", "✓ {} success: {} (bot: {})",
        CategoryToString(category), operation,
        botGuid.IsEmpty() ? "N/A" : std::to_string(botGuid.GetCounter()));
}

uint64 BotOperationTracker::RecordError(BotOperationCategory category, uint16 errorCode,
                                         std::string_view operation, std::string_view message,
                                         ObjectGuid botGuid, ObjectGuid relatedGuid,
                                         uint32 accountId, uint32 contentId)
{
    BotOperationError error;
    error.timestamp = std::chrono::system_clock::now();
    error.category = category;
    error.result = BotOperationResult::FAILED;
    error.errorCode = errorCode;
    error.botGuid = botGuid;
    error.relatedGuid = relatedGuid;
    error.accountId = accountId;
    error.contentId = contentId;
    error.operation = std::string(operation);
    error.message = std::string(message);

    return RecordError(error);
}

uint64 BotOperationTracker::RecordError(BotOperationError const& error)
{
    if (!_enabled)
        return 0;

    uint64 errorId = _nextErrorId++;
    BotOperationError errorCopy = error;
    errorCopy.errorId = errorId;

    // Update metrics
    size_t catIndex = static_cast<size_t>(error.category);
    {
        std::lock_guard<std::mutex> lock(_metricsMutex);
        auto& categoryMetrics = _metrics[catIndex];
        auto& overall = categoryMetrics.overall;

        overall.totalOperations++;
        overall.failureCount++;
        overall.recentFailure++;
        overall.lastFailure = std::chrono::system_clock::now();

        // Track by error code
        auto& codeMetrics = categoryMetrics.byErrorCode[error.errorCode];
        codeMetrics.totalOperations++;
        codeMetrics.failureCount++;
        codeMetrics.recentFailure++;
        codeMetrics.lastFailure = std::chrono::system_clock::now();
    }

    // Store error
    {
        std::lock_guard<std::mutex> lock(_errorMutex);

        // Prune old errors if needed
        while (_recentErrors.size() >= _maxRecentErrors)
        {
            uint64 oldId = _recentErrors.front().errorId;
            _errorIndex.erase(oldId);
            _recentErrors.pop_front();
        }

        _recentErrors.push_back(errorCopy);
        _errorIndex[errorId] = _recentErrors.size() - 1;
    }

    // Log the error
    LogError(errorCopy);

    return errorId;
}

void BotOperationTracker::RecordPartial(BotOperationCategory category, std::string_view operation,
                                         uint32 successCount, uint32 failCount, ObjectGuid botGuid)
{
    if (!_enabled)
        return;

    size_t catIndex = static_cast<size_t>(category);
    auto& metrics = _metrics[catIndex].overall;

    metrics.totalOperations++;
    metrics.partialCount++;
    metrics.recentSuccess += successCount;
    metrics.recentFailure += failCount;

    if (failCount > 0)
        metrics.lastFailure = std::chrono::system_clock::now();
    if (successCount > 0)
        metrics.lastSuccess = std::chrono::system_clock::now();

    TC_LOG_DEBUG("module.playerbot.diagnostics", "⚠ {} partial: {} ({}/{} success, bot: {})",
        CategoryToString(category), operation, successCount, successCount + failCount,
        botGuid.IsEmpty() ? "N/A" : std::to_string(botGuid.GetCounter()));
}

void BotOperationTracker::RecordRetry(uint64 errorId)
{
    std::lock_guard<std::mutex> lock(_errorMutex);

    auto it = _errorIndex.find(errorId);
    if (it != _errorIndex.end() && it->second < _recentErrors.size())
    {
        _recentErrors[it->second].retryCount++;
        _recentErrors[it->second].result = BotOperationResult::RETRY;

        size_t catIndex = static_cast<size_t>(_recentErrors[it->second].category);
        _metrics[catIndex].overall.retryCount++;
    }
}

void BotOperationTracker::RecordRecovery(uint64 errorId)
{
    std::lock_guard<std::mutex> lock(_errorMutex);

    auto it = _errorIndex.find(errorId);
    if (it != _errorIndex.end() && it->second < _recentErrors.size())
    {
        _recentErrors[it->second].recovered = true;
        _recentErrors[it->second].result = BotOperationResult::SUCCESS;

        TC_LOG_DEBUG("module.playerbot.diagnostics", "✓ Error {} recovered after {} retries",
            errorId, _recentErrors[it->second].retryCount);
    }
}

// ============================================================================
// CONVENIENCE METHODS
// ============================================================================

uint64 BotOperationTracker::RecordCreationError(CreationErrorCode code, std::string_view message,
                                                 ObjectGuid botGuid, uint32 accountId)
{
    return RecordError(BotOperationCategory::CREATION, static_cast<uint16>(code),
                       "BotCreation", message, botGuid, ObjectGuid::Empty, accountId, 0);
}

uint64 BotOperationTracker::RecordSpawnError(SpawnErrorCode code, std::string_view message,
                                              ObjectGuid botGuid, uint32 accountId)
{
    return RecordError(BotOperationCategory::SPAWN, static_cast<uint16>(code),
                       "BotSpawn", message, botGuid, ObjectGuid::Empty, accountId, 0);
}

uint64 BotOperationTracker::RecordBGQueueError(BGQueueErrorCode code, std::string_view message,
                                                ObjectGuid botGuid, ObjectGuid humanGuid, uint32 bgTypeId)
{
    return RecordError(BotOperationCategory::BG_QUEUE, static_cast<uint16>(code),
                       "BGQueue", message, botGuid, humanGuid, 0, bgTypeId);
}

uint64 BotOperationTracker::RecordLFGQueueError(LFGQueueErrorCode code, std::string_view message,
                                                 ObjectGuid botGuid, ObjectGuid humanGuid, uint32 dungeonId)
{
    return RecordError(BotOperationCategory::LFG_QUEUE, static_cast<uint16>(code),
                       "LFGQueue", message, botGuid, humanGuid, 0, dungeonId);
}

uint64 BotOperationTracker::RecordEquipmentError(EquipmentErrorCode code, std::string_view message,
                                                  ObjectGuid botGuid, uint32 itemEntry, uint8 slot)
{
    BotOperationError error;
    error.timestamp = std::chrono::system_clock::now();
    error.category = BotOperationCategory::EQUIPMENT;
    error.result = BotOperationResult::FAILED;
    error.errorCode = static_cast<uint16>(code);
    error.botGuid = botGuid;
    error.operation = "Equipment";
    error.message = std::string(message);

    // Store item/slot info in context
    std::ostringstream ctx;
    ctx << "{\"itemEntry\":" << itemEntry << ",\"slot\":" << static_cast<int>(slot) << "}";
    error.context = ctx.str();

    return RecordError(error);
}

// ============================================================================
// QUERIES
// ============================================================================

CategoryMetrics const& BotOperationTracker::GetCategoryMetrics(BotOperationCategory category) const
{
    return _metrics[static_cast<size_t>(category)];
}

float BotOperationTracker::GetOverallSuccessRate() const
{
    uint64 totalOps = 0;
    uint64 totalSuccess = 0;

    for (auto const& categoryMetrics : _metrics)
    {
        totalOps += categoryMetrics.overall.totalOperations.load();
        totalSuccess += categoryMetrics.overall.successCount.load();
    }

    return totalOps > 0 ? static_cast<float>(totalSuccess) / totalOps : 1.0f;
}

std::vector<BotOperationError> BotOperationTracker::GetRecentErrors(BotOperationCategory category,
                                                                      uint32 maxCount) const
{
    std::vector<BotOperationError> result;
    std::lock_guard<std::mutex> lock(_errorMutex);

    for (auto it = _recentErrors.rbegin(); it != _recentErrors.rend() && result.size() < maxCount; ++it)
    {
        if (it->category == category)
            result.push_back(*it);
    }

    return result;
}

BotOperationError const* BotOperationTracker::GetError(uint64 errorId) const
{
    std::lock_guard<std::mutex> lock(_errorMutex);

    auto it = _errorIndex.find(errorId);
    if (it != _errorIndex.end() && it->second < _recentErrors.size())
        return &_recentErrors[it->second];

    return nullptr;
}

DiagnosticReport BotOperationTracker::GenerateReport() const
{
    DiagnosticReport report;
    report.generatedAt = std::chrono::system_clock::now();
    report.uptime = std::chrono::duration_cast<std::chrono::seconds>(
        report.generatedAt - _startTime);

    // Calculate overall metrics
    for (size_t i = 0; i < static_cast<size_t>(BotOperationCategory::MAX_CATEGORY); ++i)
    {
        auto const& categoryMetrics = _metrics[i];
        report.totalOperations += categoryMetrics.overall.totalOperations.load();
        report.totalFailures += categoryMetrics.overall.failureCount.load();

        // Build category summary
        DiagnosticReport::CategorySummary summary;
        summary.category = static_cast<BotOperationCategory>(i);
        summary.categoryName = CategoryToString(summary.category);
        summary.operations = categoryMetrics.overall.totalOperations.load();
        summary.failures = categoryMetrics.overall.failureCount.load();
        summary.successRate = categoryMetrics.overall.GetSuccessRate();

        // Get top errors for this category
        std::vector<std::pair<uint16, uint64>> errorCounts;
        for (auto const& [code, codeMetrics] : categoryMetrics.byErrorCode)
        {
            errorCounts.emplace_back(code, codeMetrics.failureCount.load());
        }
        std::sort(errorCounts.begin(), errorCounts.end(),
            [](auto const& a, auto const& b) { return a.second > b.second; });

        if (errorCounts.size() > 5)
            errorCounts.resize(5);
        summary.topErrors = std::move(errorCounts);

        report.categories.push_back(std::move(summary));
    }

    report.overallSuccessRate = GetOverallSuccessRate();

    // Get recent errors
    {
        std::lock_guard<std::mutex> lock(_errorMutex);
        size_t count = std::min(static_cast<size_t>(100), _recentErrors.size());
        for (auto it = _recentErrors.rbegin(); count > 0 && it != _recentErrors.rend(); ++it, --count)
        {
            report.recentErrors.push_back(*it);
        }
    }

    // Get active alerts
    report.activeAlerts = GetActiveAlerts();

    return report;
}

void BotOperationTracker::PrintStatus() const
{
    auto report = GenerateReport();

    TC_LOG_INFO("module.playerbot.diagnostics",
        "=== BOT OPERATION TRACKER STATUS ===");
    TC_LOG_INFO("module.playerbot.diagnostics",
        "Uptime: {}s | Total Ops: {} | Failures: {} | Success Rate: {:.1f}%",
        report.uptime.count(), report.totalOperations, report.totalFailures,
        report.overallSuccessRate * 100.0f);

    for (auto const& cat : report.categories)
    {
        if (cat.operations == 0)
            continue;

        std::string alertMarker = _alertActive[static_cast<size_t>(cat.category)] ? " [ALERT]" : "";
        TC_LOG_INFO("module.playerbot.diagnostics",
            "  {}: {} ops, {} failures ({:.1f}% success){}",
            cat.categoryName, cat.operations, cat.failures,
            cat.successRate * 100.0f, alertMarker);

        for (auto const& [code, count] : cat.topErrors)
        {
            TC_LOG_INFO("module.playerbot.diagnostics",
                "    - {} (code {}): {} occurrences",
                ErrorCodeToString(cat.category, code), code, count);
        }
    }

    if (!report.activeAlerts.empty())
    {
        TC_LOG_WARN("module.playerbot.diagnostics", "Active Alerts:");
        for (auto const& alert : report.activeAlerts)
            TC_LOG_WARN("module.playerbot.diagnostics", "  ⚠ {}", alert);
    }
}

// ============================================================================
// ALERTING
// ============================================================================

bool BotOperationTracker::IsAlertActive(BotOperationCategory category) const
{
    return _alertActive[static_cast<size_t>(category)];
}

std::vector<std::string> BotOperationTracker::GetActiveAlerts() const
{
    std::vector<std::string> alerts;

    for (size_t i = 0; i < static_cast<size_t>(BotOperationCategory::MAX_CATEGORY); ++i)
    {
        if (_alertActive[i])
        {
            auto const& metrics = _metrics[i].overall;
            std::ostringstream ss;
            ss << CategoryToString(static_cast<BotOperationCategory>(i))
               << " success rate dropped to "
               << std::fixed << std::setprecision(1)
               << (metrics.GetRecentSuccessRate() * 100.0f)
               << "% (threshold: "
               << (_alertThresholds[i] * 100.0f)
               << "%)";
            alerts.push_back(ss.str());
        }
    }

    return alerts;
}

void BotOperationTracker::SetAlertThreshold(BotOperationCategory category, float threshold)
{
    _alertThresholds[static_cast<size_t>(category)] = threshold;
}

// ============================================================================
// HELPERS
// ============================================================================

void BotOperationTracker::UpdateMetricsWindow()
{
    auto now = std::chrono::system_clock::now();

    for (auto& categoryMetrics : _metrics)
    {
        auto& overall = categoryMetrics.overall;
        auto windowAge = std::chrono::duration_cast<std::chrono::seconds>(
            now - overall.windowStart).count();

        if (windowAge >= static_cast<int64_t>(_metricsWindowSeconds))
        {
            // Reset window counters
            overall.recentSuccess = 0;
            overall.recentFailure = 0;
            overall.windowStart = now;

            // Also reset per-code recent counters
            for (auto& [code, codeMetrics] : categoryMetrics.byErrorCode)
            {
                codeMetrics.recentSuccess = 0;
                codeMetrics.recentFailure = 0;
                codeMetrics.windowStart = now;
            }
        }
    }
}

void BotOperationTracker::CheckAlerts()
{
    for (size_t i = 0; i < static_cast<size_t>(BotOperationCategory::MAX_CATEGORY); ++i)
    {
        auto const& metrics = _metrics[i].overall;
        uint32 recentTotal = metrics.recentSuccess.load() + metrics.recentFailure.load();

        // Need at least 10 operations to trigger alert
        if (recentTotal < 10)
        {
            _alertActive[i] = false;
            continue;
        }

        float recentSuccessRate = metrics.GetRecentSuccessRate();
        bool shouldAlert = recentSuccessRate < _alertThresholds[i];

        if (shouldAlert && !_alertActive[i])
        {
            // Alert just triggered
            _alertActive[i] = true;
            TC_LOG_WARN("module.playerbot.diagnostics",
                "⚠ ALERT: {} success rate dropped to {:.1f}% (threshold: {:.1f}%)",
                CategoryToString(static_cast<BotOperationCategory>(i)),
                recentSuccessRate * 100.0f,
                _alertThresholds[i] * 100.0f);
        }
        else if (!shouldAlert && _alertActive[i])
        {
            // Alert recovered
            _alertActive[i] = false;
            TC_LOG_INFO("module.playerbot.diagnostics",
                "✓ RESOLVED: {} success rate recovered to {:.1f}%",
                CategoryToString(static_cast<BotOperationCategory>(i)),
                recentSuccessRate * 100.0f);
        }
    }
}

std::string BotOperationTracker::CategoryToString(BotOperationCategory category) const
{
    switch (category)
    {
        case BotOperationCategory::CREATION:  return "CREATION";
        case BotOperationCategory::SPAWN:     return "SPAWN";
        case BotOperationCategory::BG_QUEUE:  return "BG_QUEUE";
        case BotOperationCategory::LFG_QUEUE: return "LFG_QUEUE";
        case BotOperationCategory::EQUIPMENT: return "EQUIPMENT";
        case BotOperationCategory::LIFECYCLE: return "LIFECYCLE";
        case BotOperationCategory::DATABASE:  return "DATABASE";
        case BotOperationCategory::NETWORK:   return "NETWORK";
        default: return "UNKNOWN";
    }
}

std::string BotOperationTracker::ErrorCodeToString(BotOperationCategory category, uint16 code) const
{
    // Provide human-readable names for common error codes
    switch (category)
    {
        case BotOperationCategory::CREATION:
            switch (static_cast<CreationErrorCode>(code))
            {
                case CreationErrorCode::SUCCESS: return "Success";
                case CreationErrorCode::ACCOUNT_CAPACITY_EXCEEDED: return "AccountCapacityExceeded";
                case CreationErrorCode::ACCOUNT_ALLOCATION_FAILED: return "AccountAllocationFailed";
                case CreationErrorCode::CHARACTER_LIMIT_REACHED: return "CharacterLimitReached";
                case CreationErrorCode::NAME_ALLOCATION_FAILED: return "NameAllocationFailed";
                case CreationErrorCode::INVALID_RACE_CLASS_COMBO: return "InvalidRaceClassCombo";
                case CreationErrorCode::PLAYER_CREATE_FAILED: return "PlayerCreateFailed";
                case CreationErrorCode::INVALID_STARTING_POSITION: return "InvalidStartingPosition";
                case CreationErrorCode::DATABASE_SAVE_FAILED: return "DatabaseSaveFailed";
                case CreationErrorCode::DATABASE_COMMIT_TIMEOUT: return "DatabaseCommitTimeout";
                case CreationErrorCode::SESSION_CREATE_FAILED: return "SessionCreateFailed";
                case CreationErrorCode::CLONE_ENGINE_FAILED: return "CloneEngineFailed";
                case CreationErrorCode::TEMPLATE_NOT_FOUND: return "TemplateNotFound";
                case CreationErrorCode::DB2_VALIDATION_FAILED: return "DB2ValidationFailed";
                default: return "UnknownCreationError";
            }

        case BotOperationCategory::SPAWN:
            switch (static_cast<SpawnErrorCode>(code))
            {
                case SpawnErrorCode::SUCCESS: return "Success";
                case SpawnErrorCode::NO_ACCOUNT_AVAILABLE: return "NoAccountAvailable";
                case SpawnErrorCode::NO_CHARACTER_AVAILABLE: return "NoCharacterAvailable";
                case SpawnErrorCode::SESSION_CREATE_FAILED: return "SessionCreateFailed";
                case SpawnErrorCode::LOGIN_FAILED: return "LoginFailed";
                case SpawnErrorCode::PLAYER_NOT_CREATED: return "PlayerNotCreated";
                case SpawnErrorCode::AI_CREATE_FAILED: return "AICreateFailed";
                case SpawnErrorCode::LIFECYCLE_TRANSITION_FAILED: return "LifecycleTransitionFailed";
                case SpawnErrorCode::GLOBAL_CAP_REACHED: return "GlobalCapReached";
                case SpawnErrorCode::ZONE_CAP_REACHED: return "ZoneCapReached";
                case SpawnErrorCode::MAP_CAP_REACHED: return "MapCapReached";
                case SpawnErrorCode::THROTTLED: return "Throttled";
                case SpawnErrorCode::CIRCUIT_BREAKER_OPEN: return "CircuitBreakerOpen";
                case SpawnErrorCode::CHARACTER_LOOKUP_FAILED: return "CharacterLookupFailed";
                default: return "UnknownSpawnError";
            }

        case BotOperationCategory::BG_QUEUE:
            switch (static_cast<BGQueueErrorCode>(code))
            {
                case BGQueueErrorCode::SUCCESS: return "Success";
                case BGQueueErrorCode::BOT_UNAVAILABLE: return "BotUnavailable";
                case BGQueueErrorCode::BOT_IN_GROUP: return "BotInGroup";
                case BGQueueErrorCode::BOT_IN_BATTLEGROUND: return "BotInBattleground";
                case BGQueueErrorCode::BOT_IN_ARENA: return "BotInArena";
                case BGQueueErrorCode::BOT_ALREADY_QUEUED: return "BotAlreadyQueued";
                case BGQueueErrorCode::BOT_QUEUE_FULL: return "BotQueueFull";
                case BGQueueErrorCode::BOT_DEAD: return "BotDead";
                case BGQueueErrorCode::BOT_HAS_DESERTER: return "BotHasDeserter";
                case BGQueueErrorCode::BG_TEMPLATE_NOT_FOUND: return "BGTemplateNotFound";
                case BGQueueErrorCode::BRACKET_NOT_FOUND: return "BracketNotFound";
                case BGQueueErrorCode::ADD_GROUP_FAILED: return "AddGroupFailed";
                case BGQueueErrorCode::INVITATION_EXPIRED: return "InvitationExpired";
                case BGQueueErrorCode::INVITATION_NOT_FOUND: return "InvitationNotFound";
                case BGQueueErrorCode::BG_INSTANCE_NOT_FOUND: return "BGInstanceNotFound";
                case BGQueueErrorCode::TELEPORT_FAILED: return "TeleportFailed";
                case BGQueueErrorCode::INSUFFICIENT_BOTS_ALLIANCE: return "InsufficientBotsAlliance";
                case BGQueueErrorCode::INSUFFICIENT_BOTS_HORDE: return "InsufficientBotsHorde";
                case BGQueueErrorCode::HUMAN_PLAYER_NOT_FOUND: return "HumanPlayerNotFound";
                default: return "UnknownBGQueueError";
            }

        case BotOperationCategory::LFG_QUEUE:
            switch (static_cast<LFGQueueErrorCode>(code))
            {
                case LFGQueueErrorCode::SUCCESS: return "Success";
                case LFGQueueErrorCode::BOT_UNAVAILABLE: return "BotUnavailable";
                case LFGQueueErrorCode::BOT_IN_GROUP: return "BotInGroup";
                case LFGQueueErrorCode::BOT_TOO_LOW_LEVEL: return "BotTooLowLevel";
                case LFGQueueErrorCode::BOT_HAS_DESERTER: return "BotHasDeserter";
                case LFGQueueErrorCode::BOT_INVALID_STATE: return "BotInvalidState";
                case LFGQueueErrorCode::ROLE_VALIDATION_FAILED: return "RoleValidationFailed";
                case LFGQueueErrorCode::DUNGEON_NOT_FOUND: return "DungeonNotFound";
                case LFGQueueErrorCode::JOIN_LFG_FAILED: return "JoinLFGFailed";
                case LFGQueueErrorCode::PROPOSAL_ACCEPT_FAILED: return "ProposalAcceptFailed";
                case LFGQueueErrorCode::ROLE_CHECK_FAILED: return "RoleCheckFailed";
                case LFGQueueErrorCode::GROUP_FORMATION_FAILED: return "GroupFormationFailed";
                case LFGQueueErrorCode::TELEPORT_FAILED: return "TeleportFailed";
                case LFGQueueErrorCode::INSUFFICIENT_TANKS: return "InsufficientTanks";
                case LFGQueueErrorCode::INSUFFICIENT_HEALERS: return "InsufficientHealers";
                case LFGQueueErrorCode::INSUFFICIENT_DPS: return "InsufficientDPS";
                case LFGQueueErrorCode::HUMAN_PLAYER_NOT_FOUND: return "HumanPlayerNotFound";
                case LFGQueueErrorCode::JIT_BOT_TIMEOUT: return "JITBotTimeout";
                default: return "UnknownLFGQueueError";
            }

        case BotOperationCategory::EQUIPMENT:
            switch (static_cast<EquipmentErrorCode>(code))
            {
                case EquipmentErrorCode::SUCCESS: return "Success";
                case EquipmentErrorCode::CACHE_NOT_READY: return "CacheNotReady";
                case EquipmentErrorCode::NO_ITEMS_FOR_SLOT: return "NoItemsForSlot";
                case EquipmentErrorCode::ITEM_TEMPLATE_NOT_FOUND: return "ItemTemplateNotFound";
                case EquipmentErrorCode::CANNOT_EQUIP_ITEM: return "CannotEquipItem";
                case EquipmentErrorCode::EQUIP_FAILED: return "EquipFailed";
                case EquipmentErrorCode::BAG_INSERTION_FAILED: return "BagInsertionFailed";
                case EquipmentErrorCode::WRONG_ARMOR_TYPE: return "WrongArmorType";
                case EquipmentErrorCode::WRONG_WEAPON_TYPE: return "WrongWeaponType";
                case EquipmentErrorCode::LEVEL_REQUIREMENT_NOT_MET: return "LevelRequirementNotMet";
                case EquipmentErrorCode::CLASS_RESTRICTION: return "ClassRestriction";
                case EquipmentErrorCode::SKILL_REQUIREMENT_NOT_MET: return "SkillRequirementNotMet";
                case EquipmentErrorCode::SAVE_DEFERRED_NOT_EXECUTED: return "SaveDeferredNotExecuted";
                case EquipmentErrorCode::EMPTY_GEAR_SET: return "EmptyGearSet";
                case EquipmentErrorCode::QUALITY_FALLBACK_USED: return "QualityFallbackUsed";
                case EquipmentErrorCode::UNKNOWN_CLASS_DEFAULT: return "UnknownClassDefault";
                default: return "UnknownEquipmentError";
            }

        default:
            return "Error" + std::to_string(code);
    }
}

void BotOperationTracker::LogError(BotOperationError const& error)
{
    std::string botInfo = error.botGuid.IsEmpty() ?
        "N/A" : std::to_string(error.botGuid.GetCounter());

    std::string relatedInfo = error.relatedGuid.IsEmpty() ?
        "" : Trinity::StringFormat(" (related: {})", error.relatedGuid.GetCounter());

    std::string contentInfo = error.contentId > 0 ?
        Trinity::StringFormat(" [content: {}]", error.contentId) : "";

    TC_LOG_ERROR("module.playerbot.diagnostics",
        "✗ {} ERROR [{}] {}: {} | Bot: {}{}{} | Account: {}",
        CategoryToString(error.category),
        error.errorCode,
        ErrorCodeToString(error.category, error.errorCode),
        error.message,
        botInfo, relatedInfo, contentInfo,
        error.accountId);
}

} // namespace Playerbot
