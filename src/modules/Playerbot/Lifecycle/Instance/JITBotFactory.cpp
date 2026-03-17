/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * @file JITBotFactory.cpp
 * @brief Implementation of the Just-In-Time Bot Factory
 */

#include "JITBotFactory.h"
#include "BotCloneEngine.h"
#include "BotTemplateRepository.h"
#include "Account/BotAccountMgr.h"
#include "Config/PlayerbotConfig.h"
#include "Session/BotWorldSessionMgr.h"
#include "Database/PlayerbotDatabase.h"
#include "Core/Diagnostics/BotOperationTracker.h"
#include "Group/GroupRoleEnums.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "DB2Stores.h"
#include "Log.h"
#include "DatabaseEnv.h"
#include "CharacterDatabase.h"
#include "LoginDatabase.h"
#include <fmt/format.h>
#include <cmath>

namespace Playerbot
{

// ============================================================================
// ROLE DETECTION UTILITIES
// ============================================================================

// Note: GetPlayerSpecRole is declared in Group/GroupRoleEnums.h and
// implemented in Group/GroupRoleUtils.cpp

/**
 * @brief Convert GroupRole to BotRole for the pool system
 * @param groupRole The GroupRole from spec detection
 * @return Corresponding BotRole (Tank, Healer, or DPS)
 */
static BotRole GroupRoleToBotRole(GroupRole groupRole)
{
    switch (groupRole)
    {
        case GroupRole::TANK:
            return BotRole::Tank;

        case GroupRole::HEALER:
            return BotRole::Healer;

        case GroupRole::MELEE_DPS:
        case GroupRole::RANGED_DPS:
        case GroupRole::SUPPORT:
        case GroupRole::UTILITY:
        case GroupRole::NONE:
        default:
            return BotRole::DPS;
    }
}

/**
 * @brief Determine the BotRole for a player based on their specialization
 * @param player The player to analyze
 * @return BotRole (Tank, Healer, or DPS) based on active spec
 */
static BotRole DetermineBotRole(Player const* player)
{
    if (!player)
        return BotRole::DPS;

    GroupRole groupRole = GetPlayerSpecRole(player);
    return GroupRoleToBotRole(groupRole);
}

// ============================================================================
// SINGLETON
// ============================================================================

JITBotFactory* JITBotFactory::Instance()
{
    static JITBotFactory instance;
    return &instance;
}

// ============================================================================
// LIFECYCLE
// ============================================================================

bool JITBotFactory::Initialize()
{
    if (_initialized.load())
    {
        TC_LOG_WARN("playerbot.jit", "JITBotFactory::Initialize - Already initialized");
        return true;
    }

    TC_LOG_INFO("playerbot.jit", "JITBotFactory::Initialize - Starting initialization");

    // Load configuration
    LoadConfig();

    if (!_config.enabled)
    {
        TC_LOG_INFO("playerbot.jit", "JITBotFactory::Initialize - JIT factory is disabled");
        _initialized.store(true);
        return true;
    }

    // CRITICAL FIX: Clean up orphaned bot characters from previous runs
    // This prevents the "Account already has 10 characters" error that occurs when:
    // 1. Server crashes without clean shutdown
    // 2. JIT bot characters weren't deleted properly
    // 3. Accounts accumulate orphaned characters across restarts
    //
    // This cleanup is safe because:
    // - Bot accounts (@playerbot.local) are only used for transient bots
    // - Pool bots haven't been created yet (WarmPool runs after Initialize)
    // - Any characters on bot accounts from previous runs are orphans
    CleanupOrphanedBotCharacters();

    // Initialize statistics
    _hourStart = std::chrono::system_clock::now();
    _completedThisHour.store(0);
    _failedThisHour.store(0);
    _cancelledThisHour.store(0);
    _botsCreatedThisHour.store(0);
    _botsRecycledThisHour.store(0);

    // Start worker thread
    _running.store(true);
    _workerThread = std::make_unique<std::thread>(&JITBotFactory::WorkerThread, this);

    _initialized.store(true);
    TC_LOG_INFO("playerbot.jit", "JITBotFactory::Initialize - Initialization complete");

    return true;
}

void JITBotFactory::Shutdown()
{
    if (!_initialized.load())
        return;

    TC_LOG_INFO("playerbot.jit", "JITBotFactory::Shutdown - Starting shutdown");

    // Stop worker thread
    _running.store(false);
    if (_workerThread && _workerThread->joinable())
    {
        _workerThread->join();
        _workerThread.reset();
    }

    // Clear pending requests
    {
        std::lock_guard<std::mutex> lock(_queueMutex);
        while (!_pendingQueue.empty())
            _pendingQueue.pop();
    }

    // Clear active requests
    {
        std::lock_guard<std::mutex> lock(_requestMutex);
        _activeRequests.clear();
    }

    // Clear recycled bots
    {
        std::lock_guard<std::mutex> lock(_recycleMutex);
        _recycledBots.clear();
    }

    // CRITICAL FIX: Delete all JIT bot characters from database to prevent orphaned records
    // JIT bots are created on-demand and must be cleaned up on shutdown to prevent:
    // 1. Orphaned character records accumulating in the database
    // 2. Bot accounts hitting the 10-character-per-account limit
    // 3. Future JIT bot creation failures due to "Account already has 10 characters"
    {
        std::lock_guard<std::mutex> lock(_accountMapMutex);
        uint32 deletedCount = 0;

        TC_LOG_INFO("playerbot.jit", "JITBotFactory::Shutdown - Deleting {} JIT bot characters from database...", _botAccountMap.size());

        for (auto const& [botGuid, accountId] : _botAccountMap)
        {
            // Use TrinityCore's proper character deletion to clean up all related tables
            // deleteFinally=true ensures immediate deletion without respecting CharDelete.KeepDays
            // updateRealmChars=false since these are bot accounts
            Player::DeleteFromDB(botGuid, accountId, false /* updateRealmChars */, true /* deleteFinally */);
            ++deletedCount;
        }

        TC_LOG_INFO("playerbot.jit", "JITBotFactory::Shutdown - Deleted {} JIT bot characters", deletedCount);

        _botAccountMap.clear();

        // Also clear the tracking table since we've deleted all JIT bots
        // This ensures a clean state for the next server start
        // NOTE: playerbot_jit_bots is in the PLAYERBOT database, not characters
        try
        {
            sPlayerbotDatabase->Execute("TRUNCATE TABLE playerbot_jit_bots");
            TC_LOG_DEBUG("playerbot.jit", "JITBotFactory::Shutdown - Cleared JIT bot tracking table");
        }
        catch (std::exception const& e)
        {
            TC_LOG_WARN("playerbot.jit", "JITBotFactory::Shutdown - Failed to clear JIT bot tracking table: {}", e.what());
        }
    }

    _initialized.store(false);
    TC_LOG_INFO("playerbot.jit", "JITBotFactory::Shutdown - Shutdown complete");
}

void JITBotFactory::Update(uint32 /*diff*/)
{
    if (!_initialized.load() || !_config.enabled)
        return;

    // Reset hourly statistics if needed
    auto now = std::chrono::system_clock::now();
    auto hoursSinceStart = std::chrono::duration_cast<std::chrono::hours>(now - _hourStart).count();
    if (hoursSinceStart >= 1)
    {
        std::lock_guard<std::mutex> lock(_statsMutex);
        _completedThisHour.store(0);
        _failedThisHour.store(0);
        _cancelledThisHour.store(0);
        _botsCreatedThisHour.store(0);
        _botsRecycledThisHour.store(0);
        _hourStart = now;
    }

    // Process timeouts
    ProcessTimeouts();

    // Cleanup expired recycled bots
    CleanupRecycledBots();
}

void JITBotFactory::LoadConfig()
{
    TC_LOG_DEBUG("playerbot.jit", "JITBotFactory::LoadConfig - Loading configuration");

    _config.enabled = sPlayerbotConfig->GetBool("Playerbot.Instance.JIT.Enable", true);
    _config.maxConcurrentCreations = sPlayerbotConfig->GetInt("Playerbot.Instance.JIT.MaxConcurrentCreations", 10);
    _config.recycleTimeoutMinutes = sPlayerbotConfig->GetInt("Playerbot.Instance.JIT.RecycleTimeoutMinutes", 5);
    _config.maxRecycledBots = sPlayerbotConfig->GetInt("Playerbot.Instance.JIT.MaxRecycledBots", 100);

    // SAFETY HARD CAP (2026-01-22 FIX): Absolute maximum instance bots to prevent explosion
    _config.maxTotalInstanceBots = sPlayerbotConfig->GetInt("Playerbot.Instance.JIT.MaxTotalInstanceBots", 500);

    _config.dungeonPriority = static_cast<uint8>(sPlayerbotConfig->GetInt("Playerbot.Instance.JIT.DungeonPriority", 1));
    _config.arenaPriority = static_cast<uint8>(sPlayerbotConfig->GetInt("Playerbot.Instance.JIT.ArenaPriority", 2));
    _config.raidPriority = static_cast<uint8>(sPlayerbotConfig->GetInt("Playerbot.Instance.JIT.RaidPriority", 3));
    _config.battlegroundPriority = static_cast<uint8>(sPlayerbotConfig->GetInt("Playerbot.Instance.JIT.BattlegroundPriority", 4));

    _config.dungeonTimeoutMs = sPlayerbotConfig->GetInt("Playerbot.Instance.JIT.DungeonTimeoutMs", 30000);
    _config.raidTimeoutMs = sPlayerbotConfig->GetInt("Playerbot.Instance.JIT.RaidTimeoutMs", 60000);
    _config.battlegroundTimeoutMs = sPlayerbotConfig->GetInt("Playerbot.Instance.JIT.BattlegroundTimeoutMs", 120000);
    _config.arenaTimeoutMs = sPlayerbotConfig->GetInt("Playerbot.Instance.JIT.ArenaTimeoutMs", 15000);

    TC_LOG_INFO("playerbot.jit", "JITBotFactory::LoadConfig - JIT Factory: enabled={}, maxConcurrent={}, recycleTimeout={}min, maxTotalInstanceBots={}",
        _config.enabled, _config.maxConcurrentCreations, _config.recycleTimeoutMinutes, _config.maxTotalInstanceBots);
}

// ============================================================================
// REQUEST SUBMISSION
// ============================================================================

uint32 JITBotFactory::SubmitRequest(FactoryRequest request)
{
    if (!_initialized.load() || !_config.enabled)
    {
        TC_LOG_WARN("playerbot.jit", "JITBotFactory::SubmitRequest - Factory not available");
        if (request.onFailed)
            request.onFailed("JIT factory not available");
        return 0;
    }

    if (!request.IsValid())
    {
        TC_LOG_WARN("playerbot.jit", "JITBotFactory::SubmitRequest - Invalid request");
        if (request.onFailed)
            request.onFailed("Invalid request parameters");
        return 0;
    }

    // ========================================================================
    // SAFETY HARD CAP CHECK (2026-01-22 FIX)
    // ========================================================================
    // Prevent bot explosion by enforcing absolute maximum instance bot count.
    // This check runs BEFORE queueing the request to avoid runaway creation.
    //
    // Note: Instance bots include:
    // - JIT bots created on-demand for LFG/BG/Arena/Raid
    // - Warm pool bots assigned to instances
    //
    // If this cap is reached, the request is rejected with an error.
    // The bot explosion bug (1500+ bots with 50s lag) was caused by:
    // 1. JIT bots not being marked as instance bots (now fixed above)
    // 2. No hard cap to catch runaway creation (fixed here)
    // ========================================================================
    if (_config.maxTotalInstanceBots > 0)
    {
        uint32 currentInstanceBots = sBotWorldSessionMgr->GetInstanceBotCount();
        if (currentInstanceBots >= _config.maxTotalInstanceBots)
        {
            TC_LOG_ERROR("playerbot.jit", "JITBotFactory::SubmitRequest - HARD CAP REACHED! "
                "Current instance bots ({}) >= max ({}). Rejecting new JIT request to prevent explosion.",
                currentInstanceBots, _config.maxTotalInstanceBots);

            if (request.onFailed)
                request.onFailed("Instance bot hard cap reached - try again later");
            return 0;
        }

        // Also log a warning if we're approaching the cap (>80% full)
        uint32 warningThreshold = static_cast<uint32>(_config.maxTotalInstanceBots * 0.8f);
        if (currentInstanceBots >= warningThreshold)
        {
            TC_LOG_WARN("playerbot.jit", "JITBotFactory::SubmitRequest - Approaching hard cap: {}/{} instance bots ({}%)",
                currentInstanceBots, _config.maxTotalInstanceBots,
                (currentInstanceBots * 100) / _config.maxTotalInstanceBots);
        }
    }

    // Assign request ID
    request.requestId = _nextRequestId.fetch_add(1);
    request.createdAt = std::chrono::system_clock::now();

    // Set priority and timeout based on instance type
    request.priority = GetPriorityForType(request.instanceType);
    request.timeout = GetTimeoutForType(request.instanceType);

    TC_LOG_INFO("playerbot.jit", "JITBotFactory::SubmitRequest - Request {} submitted: type={}, total={}, priority={}",
        request.requestId, InstanceTypeToString(request.instanceType),
        request.GetTotalNeeded(), request.priority);

    // Add to pending queue
    {
        std::lock_guard<std::mutex> lock(_queueMutex);
        _pendingQueue.push(std::move(request));
    }

    return request.requestId;
}

uint32 JITBotFactory::SubmitBracketRequest(
    BotRole role,
    Faction faction,
    PoolBracket bracket,
    uint32 count,
    InstanceType instanceType,
    uint32 contentId,
    std::function<void(std::vector<ObjectGuid> const&)> onComplete)
{
    if (count == 0)
    {
        TC_LOG_WARN("playerbot.jit", "JITBotFactory::SubmitBracketRequest - Zero bots requested");
        return 0;
    }

    FactoryRequest request;
    request.instanceType = instanceType;
    request.contentId = contentId;
    request.bracket = bracket;
    request.useBracketMatching = true;
    request.targetFaction = faction;
    request.playerFaction = faction;
    request.source = fmt::format("BracketOverflow:{}", PoolBracketToString(bracket));

    // Get level for the bracket (use midpoint)
    request.playerLevel = GetBracketMidpointLevel(bracket);

    // Set role-based requirements
    switch (role)
    {
        case BotRole::Tank:
            if (instanceType == InstanceType::Battleground || instanceType == InstanceType::Arena)
            {
                // PvP: set faction-based
                if (faction == Faction::Alliance)
                    request.allianceNeeded = count;
                else
                    request.hordeNeeded = count;
            }
            else
            {
                request.tanksNeeded = count;
            }
            break;
        case BotRole::Healer:
            if (instanceType == InstanceType::Battleground || instanceType == InstanceType::Arena)
            {
                if (faction == Faction::Alliance)
                    request.allianceNeeded = count;
                else
                    request.hordeNeeded = count;
            }
            else
            {
                request.healersNeeded = count;
            }
            break;
        case BotRole::DPS:
        default:
            if (instanceType == InstanceType::Battleground || instanceType == InstanceType::Arena)
            {
                if (faction == Faction::Alliance)
                    request.allianceNeeded = count;
                else
                    request.hordeNeeded = count;
            }
            else
            {
                request.dpsNeeded = count;
            }
            break;
    }

    request.onComplete = std::move(onComplete);

    TC_LOG_INFO("playerbot.jit", "JITBotFactory::SubmitBracketRequest - {} {} {} bots for bracket {}",
        count, FactionToString(faction), BotRoleToString(role), PoolBracketToString(bracket));

    return SubmitRequest(std::move(request));
}

void JITBotFactory::CancelRequest(uint32 requestId)
{
    TC_LOG_DEBUG("playerbot.jit", "JITBotFactory::CancelRequest - Cancelling request {}", requestId);

    // Try to remove from active requests
    {
        std::lock_guard<std::mutex> lock(_requestMutex);
        auto it = _activeRequests.find(requestId);
        if (it != _activeRequests.end())
        {
            it->second.progress.status = RequestStatus::Cancelled;
            if (it->second.request.onFailed)
                it->second.request.onFailed("Request cancelled");
            _activeRequests.erase(it);
            _cancelledThisHour.fetch_add(1);
            return;
        }
    }

    // Request might be in pending queue - mark for cancellation
    // (will be handled when dequeued)
    TC_LOG_DEBUG("playerbot.jit", "JITBotFactory::CancelRequest - Request {} not active, may be pending",
        requestId);
}

RequestStatus JITBotFactory::GetRequestStatus(uint32 requestId) const
{
    std::lock_guard<std::mutex> lock(_requestMutex);

    auto it = _activeRequests.find(requestId);
    if (it != _activeRequests.end())
        return it->second.progress.status;

    return RequestStatus::Pending;  // Might be in queue
}

RequestProgress JITBotFactory::GetRequestProgress(uint32 requestId) const
{
    std::lock_guard<std::mutex> lock(_requestMutex);

    auto it = _activeRequests.find(requestId);
    if (it != _activeRequests.end())
        return it->second.progress;

    RequestProgress defaultProgress;
    defaultProgress.requestId = requestId;
    defaultProgress.status = RequestStatus::Pending;
    return defaultProgress;
}

std::chrono::milliseconds JITBotFactory::GetEstimatedCompletion(uint32 requestId) const
{
    std::lock_guard<std::mutex> lock(_requestMutex);

    auto it = _activeRequests.find(requestId);
    if (it != _activeRequests.end())
        return it->second.progress.estimatedRemaining;

    // For pending requests, estimate based on queue position and avg time
    return std::chrono::milliseconds(5000);  // Default estimate
}

// ============================================================================
// BOT RECYCLING
// ============================================================================

void JITBotFactory::RecycleBot(Player* player, BotRole role)
{
    if (!player || !_initialized.load())
        return;

    ObjectGuid guid = player->GetGUID();
    Faction faction = (player->GetTeamId() == TEAM_ALLIANCE) ? Faction::Alliance : Faction::Horde;
    uint32 level = player->GetLevel();
    uint32 gearScore = static_cast<uint32>(std::round(player->GetAverageItemLevel()));
    uint8 playerClass = player->GetClass();
    PoolBracket bracket = GetBracketForLevel(level);

    RecycleBot(guid, role, faction, bracket, level, gearScore, playerClass);
}

void JITBotFactory::RecycleBot(
    ObjectGuid botGuid,
    BotRole role,
    Faction faction,
    PoolBracket bracket,
    uint32 level,
    uint32 gearScore,
    uint8 playerClass)
{
    if (!_initialized.load())
        return;

    std::lock_guard<std::mutex> lock(_recycleMutex);

    // Check if we're at capacity
    if (_recycledBots.size() >= _config.maxRecycledBots)
    {
        TC_LOG_DEBUG("playerbot.jit", "JITBotFactory::RecycleBot - Recycle pool full, discarding bot {}",
            botGuid.ToString());
        return;
    }

    // Check if already recycled
    for (auto const& bot : _recycledBots)
    {
        if (bot.guid == botGuid)
        {
            TC_LOG_DEBUG("playerbot.jit", "JITBotFactory::RecycleBot - Bot {} already recycled",
                botGuid.ToString());
            return;
        }
    }

    RecycledBot bot;
    bot.guid = botGuid;
    bot.role = role;
    bot.faction = faction;
    bot.bracket = bracket;
    bot.level = level;
    bot.gearScore = gearScore;
    bot.playerClass = playerClass;
    bot.recycleTime = std::chrono::system_clock::now();

    _recycledBots.push_back(std::move(bot));
    _botsRecycledThisHour.fetch_add(1);

    // Update per-bracket statistics
    uint8 bracketIdx = static_cast<uint8>(bracket);
    if (bracketIdx < NUM_LEVEL_BRACKETS)
        _bracketStats[bracketIdx].recycled.fetch_add(1);

    TC_LOG_DEBUG("playerbot.jit", "JITBotFactory::RecycleBot - Bot {} recycled (bracket={}, total: {})",
        botGuid.ToString(), PoolBracketToString(bracket), _recycledBots.size());
}

void JITBotFactory::RecycleBotLegacy(
    ObjectGuid botGuid,
    BotRole role,
    Faction faction,
    uint32 level,
    uint32 gearScore,
    uint8 playerClass)
{
    // Auto-determine bracket from level
    PoolBracket bracket = GetBracketForLevel(level);
    RecycleBot(botGuid, role, faction, bracket, level, gearScore, playerClass);
}

void JITBotFactory::RecycleBots(std::vector<ObjectGuid> const& bots)
{
    uint32 recycledCount = 0;
    for (auto const& guid : bots)
    {
        Player* player = ObjectAccessor::FindPlayer(guid);
        if (player)
        {
            // Determine role from player's specialization using DB2 data
            BotRole role = DetermineBotRole(player);

            TC_LOG_DEBUG("playerbot.jit", "JITBotFactory::RecycleBots - Bot {} has role {} based on spec",
                player->GetName(), BotRoleToString(role));

            RecycleBot(player, role);
            ++recycledCount;
        }
    }

    if (recycledCount > 0)
    {
        TC_LOG_INFO("playerbot.jit", "JITBotFactory::RecycleBots - Recycled {} bots from instance",
            recycledCount);
    }
}

ObjectGuid JITBotFactory::GetRecycledBotForBracket(
    BotRole role,
    Faction faction,
    PoolBracket bracket,
    uint32 minGearScore)
{
    std::lock_guard<std::mutex> lock(_recycleMutex);

    for (auto it = _recycledBots.begin(); it != _recycledBots.end(); ++it)
    {
        if (it->MatchesBracket(role, faction, bracket, minGearScore))
        {
            ObjectGuid guid = it->guid;
            _recycledBots.erase(it);

            TC_LOG_DEBUG("playerbot.jit", "JITBotFactory::GetRecycledBotForBracket - Found recycled bot {} for bracket {}",
                guid.ToString(), PoolBracketToString(bracket));
            return guid;
        }
    }

    return ObjectGuid::Empty;
}

std::vector<ObjectGuid> JITBotFactory::GetRecycledBotsForBracket(
    BotRole role,
    Faction faction,
    PoolBracket bracket,
    uint32 count,
    uint32 minGearScore)
{
    std::vector<ObjectGuid> result;
    result.reserve(count);

    std::lock_guard<std::mutex> lock(_recycleMutex);

    auto it = _recycledBots.begin();
    while (it != _recycledBots.end() && result.size() < count)
    {
        if (it->MatchesBracket(role, faction, bracket, minGearScore))
        {
            result.push_back(it->guid);
            it = _recycledBots.erase(it);
        }
        else
        {
            ++it;
        }
    }

    if (!result.empty())
    {
        TC_LOG_DEBUG("playerbot.jit", "JITBotFactory::GetRecycledBotsForBracket - Found {} recycled bots for bracket {}",
            result.size(), PoolBracketToString(bracket));
    }

    return result;
}

ObjectGuid JITBotFactory::GetRecycledBot(
    BotRole role,
    Faction faction,
    uint32 level,
    uint32 minGearScore)
{
    // Legacy level-based lookup - convert to bracket-based
    PoolBracket bracket = GetBracketForLevel(level);
    return GetRecycledBotForBracket(role, faction, bracket, minGearScore);
}

uint32 JITBotFactory::GetRecycledBotCount() const
{
    std::lock_guard<std::mutex> lock(_recycleMutex);
    return static_cast<uint32>(_recycledBots.size());
}

void JITBotFactory::CleanupRecycledBots()
{
    std::lock_guard<std::mutex> lock(_recycleMutex);

    auto now = std::chrono::system_clock::now();
    auto timeout = std::chrono::minutes(_config.recycleTimeoutMinutes);

    auto it = std::remove_if(_recycledBots.begin(), _recycledBots.end(),
        [now, timeout](RecycledBot const& bot) {
            return (now - bot.recycleTime) >= timeout;
        });

    if (it != _recycledBots.end())
    {
        size_t removed = std::distance(it, _recycledBots.end());
        _recycledBots.erase(it, _recycledBots.end());
        TC_LOG_DEBUG("playerbot.jit", "JITBotFactory::CleanupRecycledBots - Removed {} expired bots",
            removed);
    }
}

// ============================================================================
// QUERIES
// ============================================================================

bool JITBotFactory::IsBusy() const
{
    std::lock_guard<std::mutex> lock(_requestMutex);
    return _activeRequests.size() >= _config.maxConcurrentCreations;
}

uint32 JITBotFactory::GetPendingRequestCount() const
{
    std::lock_guard<std::mutex> lock(_queueMutex);
    return static_cast<uint32>(_pendingQueue.size());
}

bool JITBotFactory::CanHandleRequest(FactoryRequest const& request) const
{
    if (!_initialized.load() || !_config.enabled)
        return false;

    if (!request.IsValid())
        return false;

    // Check if we can create the required bots
    uint32 totalNeeded = request.GetTotalNeeded();
    if (totalNeeded > 100)  // Hard limit
        return false;

    return true;
}

uint32 JITBotFactory::GetAccountForBot(ObjectGuid botGuid) const
{
    std::lock_guard<std::mutex> lock(_accountMapMutex);
    auto it = _botAccountMap.find(botGuid);
    if (it != _botAccountMap.end())
        return it->second;
    return 0;
}

void JITBotFactory::StoreAccountForBot(ObjectGuid botGuid, uint32 accountId)
{
    std::lock_guard<std::mutex> lock(_accountMapMutex);
    _botAccountMap[botGuid] = accountId;
    TC_LOG_DEBUG("playerbot.jit", "JITBotFactory::StoreAccountForBot - Stored account {} for bot {}",
        accountId, botGuid.ToString());

    // CRITICAL FIX: Register JIT bot in tracking table so cleanup doesn't delete BotSpawner's characters
    // This table allows CleanupOrphanedBotCharacters() to target ONLY JIT-created bots
    // NOTE: playerbot_jit_bots is in the PLAYERBOT database, not characters
    try
    {
        sPlayerbotDatabase->Execute(Trinity::StringFormat(
            "INSERT INTO playerbot_jit_bots (bot_guid, account_id, instance_type, request_id) "
            "VALUES ({}, {}, 'DUNGEON', 0) "
            "ON DUPLICATE KEY UPDATE created_at = CURRENT_TIMESTAMP",
            botGuid.GetCounter(), accountId));

        TC_LOG_DEBUG("playerbot.jit", "JITBotFactory::StoreAccountForBot - Registered JIT bot {} in tracking table",
            botGuid.ToString());
    }
    catch (std::exception const& e)
    {
        TC_LOG_WARN("playerbot.jit", "JITBotFactory::StoreAccountForBot - Failed to register JIT bot {} in tracking table: {}",
            botGuid.ToString(), e.what());
    }
}

// ============================================================================
// STATISTICS
// ============================================================================

JITBotFactory::FactoryStatistics JITBotFactory::GetStatistics() const
{
    FactoryStatistics stats;

    {
        std::lock_guard<std::mutex> lock(_queueMutex);
        stats.pendingRequests = static_cast<uint32>(_pendingQueue.size());
    }

    {
        std::lock_guard<std::mutex> lock(_requestMutex);
        stats.inProgressRequests = static_cast<uint32>(_activeRequests.size());
    }

    stats.completedThisHour = _completedThisHour.load();
    stats.failedThisHour = _failedThisHour.load();
    stats.cancelledThisHour = _cancelledThisHour.load();
    stats.recycledBotsAvailable = GetRecycledBotCount();
    stats.botsCreatedThisHour = _botsCreatedThisHour.load();
    stats.botsRecycledThisHour = _botsRecycledThisHour.load();
    stats.avgCreationTime = _avgCreationTime;
    stats.avgRequestTime = _avgRequestTime;

    // Per-bracket statistics
    for (uint8 i = 0; i < NUM_LEVEL_BRACKETS; ++i)
    {
        stats.bracketStats[i].created = _bracketStats[i].created.load();
        stats.bracketStats[i].recycled = _bracketStats[i].recycled.load();
        stats.bracketStats[i].requestsFulfilled = _bracketStats[i].requestsFulfilled.load();
    }

    return stats;
}

void JITBotFactory::PrintStatistics() const
{
    FactoryStatistics stats = GetStatistics();

    TC_LOG_INFO("playerbot.jit", "=== JITBotFactory Statistics ===");
    TC_LOG_INFO("playerbot.jit", "Pending Requests: {}", stats.pendingRequests);
    TC_LOG_INFO("playerbot.jit", "In Progress: {}", stats.inProgressRequests);
    TC_LOG_INFO("playerbot.jit", "Completed This Hour: {}", stats.completedThisHour);
    TC_LOG_INFO("playerbot.jit", "Failed This Hour: {}", stats.failedThisHour);
    TC_LOG_INFO("playerbot.jit", "Cancelled This Hour: {}", stats.cancelledThisHour);
    TC_LOG_INFO("playerbot.jit", "Recycled Available: {}", stats.recycledBotsAvailable);
    TC_LOG_INFO("playerbot.jit", "Bots Created This Hour: {}", stats.botsCreatedThisHour);
    TC_LOG_INFO("playerbot.jit", "Bots Recycled This Hour: {}", stats.botsRecycledThisHour);
    TC_LOG_INFO("playerbot.jit", "Avg Creation Time: {}ms", stats.avgCreationTime.count());
    TC_LOG_INFO("playerbot.jit", "Avg Request Time: {}ms", stats.avgRequestTime.count());
}

void JITBotFactory::SetConfig(JITFactoryConfig const& config)
{
    _config = config;
    TC_LOG_INFO("playerbot.jit", "JITBotFactory::SetConfig - Configuration updated");
}

// ============================================================================
// INTERNAL METHODS
// ============================================================================

void JITBotFactory::WorkerThread()
{
    TC_LOG_INFO("playerbot.jit", "JITBotFactory::WorkerThread - Worker thread started");

    while (_running.load())
    {
        // Get next request from queue
        FactoryRequest request;
        bool hasRequest = false;

        {
            std::lock_guard<std::mutex> lock(_queueMutex);
            if (!_pendingQueue.empty())
            {
                // Check if we have capacity
                std::lock_guard<std::mutex> reqLock(_requestMutex);
                if (_activeRequests.size() < _config.maxConcurrentCreations)
                {
                    request = _pendingQueue.top();
                    _pendingQueue.pop();
                    hasRequest = true;
                }
            }
        }

        if (hasRequest)
        {
            // Check if request timed out while in queue
            if (request.HasTimedOut())
            {
                TC_LOG_WARN("playerbot.jit", "JITBotFactory::WorkerThread - Request {} timed out in queue",
                    request.requestId);
                if (request.onFailed)
                    request.onFailed("Request timed out");
                _failedThisHour.fetch_add(1);
                continue;
            }

            // Create active request entry
            ActiveRequest active;
            active.request = request;
            active.progress.requestId = request.requestId;
            active.progress.status = RequestStatus::InProgress;
            active.progress.totalNeeded = request.GetTotalNeeded();
            active.progress.startTime = std::chrono::system_clock::now();

            {
                std::lock_guard<std::mutex> lock(_requestMutex);
                _activeRequests[request.requestId] = std::move(active);
            }

            TC_LOG_DEBUG("playerbot.jit", "JITBotFactory::WorkerThread - Processing request {}",
                request.requestId);

            // Process the request
            RequestProgress progress = ProcessRequest(request);

            // Update final status
            {
                std::lock_guard<std::mutex> lock(_requestMutex);
                auto it = _activeRequests.find(request.requestId);
                if (it != _activeRequests.end())
                {
                    it->second.progress = progress;

                    // Call appropriate callback based on status
                    if (progress.status == RequestStatus::Completed ||
                        progress.status == RequestStatus::PartiallyCompleted)
                    {
                        // Both complete and partial success call onComplete with the created bots
                        if (request.onComplete)
                            request.onComplete(progress.createdBots);

                        if (progress.status == RequestStatus::Completed)
                            _completedThisHour.fetch_add(1);
                        else
                            TC_LOG_INFO("playerbot.jit", "JITBotFactory::WorkerThread - Partial success accepted: {}", progress.errorMessage);

                        _activeRequests.erase(it);
                    }
                    else if (progress.status == RequestStatus::Failed)
                    {
                        if (request.onFailed)
                            request.onFailed(progress.errorMessage);
                        _failedThisHour.fetch_add(1);
                        _activeRequests.erase(it);
                    }
                }
            }

            // Update timing statistics
            auto requestDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now() - progress.startTime);

            {
                std::lock_guard<std::mutex> lock(_statsMutex);
                ++_requestTimeSamples;
                auto totalMs = _avgRequestTime.count() * (_requestTimeSamples - 1) + requestDuration.count();
                _avgRequestTime = std::chrono::milliseconds(totalMs / _requestTimeSamples);
            }
        }
        else
        {
            // No work, sleep briefly
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    TC_LOG_INFO("playerbot.jit", "JITBotFactory::WorkerThread - Worker thread stopped");
}

RequestProgress JITBotFactory::ProcessRequest(FactoryRequest& request)
{
    RequestProgress progress;
    progress.requestId = request.requestId;
    progress.status = RequestStatus::InProgress;
    progress.totalNeeded = request.GetTotalNeeded();
    progress.startTime = std::chrono::system_clock::now();

    TC_LOG_INFO("playerbot.jit", "JITBotFactory::ProcessRequest - Processing request {}: {} bots needed",
        request.requestId, progress.totalNeeded);

    try
    {
        // CRITICAL: Ensure we have enough account capacity BEFORE trying to create bots
        // Each bot needs an account, so we need at least totalNeeded accounts available
        // Adding 20% buffer to avoid running out mid-creation
        uint32 accountsNeeded = static_cast<uint32>(progress.totalNeeded * 1.2) + 5;
        if (!sBotAccountMgr->EnsureAccountCapacity(accountsNeeded))
        {
            TC_LOG_ERROR("playerbot.jit", "JITBotFactory::ProcessRequest - Cannot ensure account capacity for {} accounts",
                accountsNeeded);
            BOT_TRACK_CREATION_ERROR(
                CreationErrorCode::ACCOUNT_ALLOCATION_FAILED,
                fmt::format("Cannot ensure account capacity for {} accounts", accountsNeeded),
                ObjectGuid::Empty,
                0);
            progress.status = RequestStatus::Failed;
            progress.errorMessage = "Failed to ensure account capacity";
            return progress;
        }

        std::vector<ObjectGuid> createdBots;

        if (request.IsPvP())
        {
            createdBots = CreatePvPBots(request, progress);
        }
        else
        {
            createdBots = CreatePvEBots(request, progress);
        }

        progress.createdBots = std::move(createdBots);
        progress.created = static_cast<uint32>(progress.createdBots.size());

        // LOGIN BOTS: After character creation, login each bot so they're available in-world
        // This is CRITICAL for the onComplete callback to find them via ObjectAccessor::FindPlayer()
        uint32 loginSuccessCount = 0;
        for (ObjectGuid const& botGuid : progress.createdBots)
        {
            // Get the account ID we stored during creation
            uint32 accountId = GetAccountForBot(botGuid);
            if (accountId == 0)
            {
                TC_LOG_WARN("playerbot.jit", "JITBotFactory::ProcessRequest - No account ID stored for bot {}",
                    botGuid.ToString());
                BOT_TRACK_SPAWN_ERROR(
                    SpawnErrorCode::NO_ACCOUNT_AVAILABLE,
                    fmt::format("No account ID stored for bot {}", botGuid.ToString()),
                    botGuid,
                    0);
                continue;
            }

            // Login the bot using BotWorldSessionMgr
            if (sBotWorldSessionMgr->AddPlayerBot(botGuid, accountId, true))  // bypassLimit=true for JIT bots
            {
                ++loginSuccessCount;

                // ====================================================================
                // NOTE: Instance bot marking is now handled via BotPostLoginConfigurator!
                // ====================================================================
                // Previously, we called MarkAsInstanceBot() here, but AddPlayerBot()
                // only queues the spawn to _pendingSpawns - the session doesn't exist yet!
                // This caused "session not found" errors and bots not being properly tracked.
                //
                // The fix: Set markAsInstanceBot=true in BotPendingConfiguration (done in
                // BotCloneEngine.cpp), and BotPostLoginConfigurator::ApplyPendingConfiguration()
                // will call MarkAsInstanceBot() AFTER the session is created.
                // ====================================================================

                TC_LOG_DEBUG("playerbot.jit", "JITBotFactory::ProcessRequest - Logged in bot {} (account {}), will be marked as INSTANCE BOT after login completes",
                    botGuid.ToString(), accountId);
                BOT_TRACK_SUCCESS(BotOperationCategory::SPAWN, "JITBotFactory::LoginBot", botGuid);
            }
            else
            {
                TC_LOG_WARN("playerbot.jit", "JITBotFactory::ProcessRequest - Failed to login bot {} (account {})",
                    botGuid.ToString(), accountId);
                BOT_TRACK_SPAWN_ERROR(
                    SpawnErrorCode::LOGIN_FAILED,
                    fmt::format("Failed to login JIT bot {} (account {})", botGuid.ToString(), accountId),
                    botGuid,
                    accountId);
            }
        }

        TC_LOG_INFO("playerbot.jit", "JITBotFactory::ProcessRequest - Logged in {}/{} bots (will be marked as INSTANCE BOTS after post-login config)",
            loginSuccessCount, progress.created);

        // Determine status - STRICT requirements to ensure full groups
        // With proper JIT bot cleanup on shutdown, all bots should be creatable
        // If this fails, there's an underlying issue that needs fixing, not masking
        if (progress.created >= progress.totalNeeded)
        {
            progress.status = RequestStatus::Completed;
        }
        else
        {
            progress.status = RequestStatus::Failed;
            progress.errorMessage = fmt::format("Failed to create all bots: {}/{}", progress.created, progress.totalNeeded);
        }

        auto endTime = std::chrono::system_clock::now();
        progress.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - progress.startTime);

        TC_LOG_INFO("playerbot.jit", "JITBotFactory::ProcessRequest - Request {} {}: {}/{} bots in {}ms",
            request.requestId, RequestStatusToString(progress.status),
            progress.created, progress.totalNeeded, progress.elapsed.count());
    }
    catch (std::exception const& e)
    {
        progress.status = RequestStatus::Failed;
        progress.errorMessage = fmt::format("Exception: {}", e.what());
        TC_LOG_ERROR("playerbot.jit", "JITBotFactory::ProcessRequest - Request {} failed with exception: {}",
            request.requestId, e.what());
        BOT_TRACK_CREATION_ERROR(
            CreationErrorCode::CLONE_ENGINE_FAILED,
            fmt::format("JIT request {} failed with exception: {}", request.requestId, e.what()),
            ObjectGuid::Empty,
            0);
    }

    return progress;
}

// ============================================================================
// BATCH CLONE WITH RETRY
// ============================================================================

std::vector<ObjectGuid> JITBotFactory::BatchCloneWithRetry(
    BatchCloneRequest const& baseReq,
    uint32 maxRetries,
    RequestProgress& progress)
{
    std::vector<ObjectGuid> result;
    uint32 remaining = baseReq.count;
    uint32 retryCount = 0;

    while (remaining > 0 && retryCount <= maxRetries)
    {
        if (retryCount > 0)
        {
            TC_LOG_DEBUG("playerbot.jit", "BatchCloneWithRetry - Retry {}/{} for {} {} bots",
                retryCount, maxRetries, remaining, BotRoleToString(baseReq.role));

            // Brief delay between retries to allow account releases
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        BatchCloneRequest cloneReq = baseReq;
        cloneReq.count = remaining;

        auto cloneResults = sBotCloneEngine->BatchClone(cloneReq);

        uint32 successCount = 0;
        for (auto const& cr : cloneResults)
        {
            if (cr.success)
            {
                result.push_back(cr.botGuid);
                progress.fromClone++;
                _botsCreatedThisHour.fetch_add(1);
                successCount++;

                // Store the account ID mapping for later lookup
                // This is critical because database commits are async
                StoreAccountForBot(cr.botGuid, cr.accountId);
            }
        }

        remaining -= successCount;

        if (successCount == 0)
        {
            // No progress made, increment retry counter
            retryCount++;
        }
        else
        {
            // Made progress, reset retry counter but continue for remaining
            retryCount = 0;
        }
    }

    if (remaining > 0)
    {
        TC_LOG_WARN("playerbot.jit", "BatchCloneWithRetry - Failed to create {} {} bots after {} retries",
            remaining, BotRoleToString(baseReq.role), maxRetries);
        // Track each failed bot creation attempt
        for (uint32 i = 0; i < remaining; ++i)
        {
            BOT_TRACK_CREATION_ERROR(
                CreationErrorCode::CLONE_ENGINE_FAILED,
                fmt::format("Failed to clone {} bot after {} retries", BotRoleToString(baseReq.role), maxRetries),
                ObjectGuid::Empty,
                0);
        }
    }

    return result;
}

std::vector<ObjectGuid> JITBotFactory::CreatePvEBots(
    FactoryRequest const& request,
    RequestProgress& progress)
{
    std::vector<ObjectGuid> result;
    constexpr uint32 MAX_RETRIES = 3;

    // Create tanks
    if (request.tanksNeeded > 0)
    {
        auto recycled = GetRecycledBotsForRole(BotRole::Tank, request.playerFaction,
            request.playerLevel, request.minGearScore, request.tanksNeeded);

        progress.fromRecycled += static_cast<uint32>(recycled.size());
        result.insert(result.end(), recycled.begin(), recycled.end());

        uint32 remaining = request.tanksNeeded - static_cast<uint32>(recycled.size());
        if (remaining > 0)
        {
            BatchCloneRequest cloneReq;
            cloneReq.role = BotRole::Tank;
            cloneReq.count = remaining;
            cloneReq.targetLevel = request.playerLevel;
            cloneReq.faction = request.playerFaction;
            cloneReq.minGearScore = request.minGearScore;
            // Pass queue configuration for JIT-created bots
            cloneReq.dungeonIdToQueue = request.dungeonIdToQueue;
            cloneReq.battlegroundIdToQueue = request.battlegroundIdToQueue;
            cloneReq.arenaTypeToQueue = request.arenaTypeToQueue;

            auto clonedBots = BatchCloneWithRetry(cloneReq, MAX_RETRIES, progress);
            result.insert(result.end(), clonedBots.begin(), clonedBots.end());
        }

        // Report progress
        progress.created = static_cast<uint32>(result.size());
        if (request.onProgress)
            request.onProgress(progress.GetProgress());
    }

    // Create healers
    if (request.healersNeeded > 0)
    {
        auto recycled = GetRecycledBotsForRole(BotRole::Healer, request.playerFaction,
            request.playerLevel, request.minGearScore, request.healersNeeded);

        progress.fromRecycled += static_cast<uint32>(recycled.size());
        result.insert(result.end(), recycled.begin(), recycled.end());

        uint32 remaining = request.healersNeeded - static_cast<uint32>(recycled.size());
        if (remaining > 0)
        {
            BatchCloneRequest cloneReq;
            cloneReq.role = BotRole::Healer;
            cloneReq.count = remaining;
            cloneReq.targetLevel = request.playerLevel;
            cloneReq.faction = request.playerFaction;
            cloneReq.minGearScore = request.minGearScore;
            // Pass queue configuration for JIT-created bots
            cloneReq.dungeonIdToQueue = request.dungeonIdToQueue;
            cloneReq.battlegroundIdToQueue = request.battlegroundIdToQueue;
            cloneReq.arenaTypeToQueue = request.arenaTypeToQueue;

            auto clonedBots = BatchCloneWithRetry(cloneReq, MAX_RETRIES, progress);
            result.insert(result.end(), clonedBots.begin(), clonedBots.end());
        }

        progress.created = static_cast<uint32>(result.size());
        if (request.onProgress)
            request.onProgress(progress.GetProgress());
    }

    // Create DPS
    if (request.dpsNeeded > 0)
    {
        auto recycled = GetRecycledBotsForRole(BotRole::DPS, request.playerFaction,
            request.playerLevel, request.minGearScore, request.dpsNeeded);

        progress.fromRecycled += static_cast<uint32>(recycled.size());
        result.insert(result.end(), recycled.begin(), recycled.end());

        uint32 remaining = request.dpsNeeded - static_cast<uint32>(recycled.size());
        if (remaining > 0)
        {
            BatchCloneRequest cloneReq;
            cloneReq.role = BotRole::DPS;
            cloneReq.count = remaining;
            cloneReq.targetLevel = request.playerLevel;
            cloneReq.faction = request.playerFaction;
            cloneReq.minGearScore = request.minGearScore;
            // Pass queue configuration for JIT-created bots
            cloneReq.dungeonIdToQueue = request.dungeonIdToQueue;
            cloneReq.battlegroundIdToQueue = request.battlegroundIdToQueue;
            cloneReq.arenaTypeToQueue = request.arenaTypeToQueue;

            auto clonedBots = BatchCloneWithRetry(cloneReq, MAX_RETRIES, progress);
            result.insert(result.end(), clonedBots.begin(), clonedBots.end());
        }

        progress.created = static_cast<uint32>(result.size());
        if (request.onProgress)
            request.onProgress(progress.GetProgress());
    }

    return result;
}

std::vector<ObjectGuid> JITBotFactory::CreatePvPBots(
    FactoryRequest const& request,
    RequestProgress& progress)
{
    std::vector<ObjectGuid> result;
    constexpr uint32 MAX_RETRIES = 3;

    // Create Alliance bots
    if (request.allianceNeeded > 0)
    {
        TC_LOG_DEBUG("playerbot.jit", "JITBotFactory::CreatePvPBots - Creating {} Alliance bots",
            request.allianceNeeded);

        // For PvP, distribute roles: 10% tank, 15% healer, 75% DPS
        uint32 allianceTanks = std::max(1u, request.allianceNeeded / 10);
        uint32 allianceHealers = std::max(1u, request.allianceNeeded * 15 / 100);
        uint32 allianceDPS = request.allianceNeeded - allianceTanks - allianceHealers;

        // Create Alliance tanks
        {
            auto recycled = GetRecycledBotsForRole(BotRole::Tank, Faction::Alliance,
                request.playerLevel, request.minGearScore, allianceTanks);
            progress.fromRecycled += static_cast<uint32>(recycled.size());
            result.insert(result.end(), recycled.begin(), recycled.end());

            uint32 remaining = allianceTanks - static_cast<uint32>(recycled.size());
            if (remaining > 0)
            {
                BatchCloneRequest cloneReq;
                cloneReq.role = BotRole::Tank;
                cloneReq.count = remaining;
                cloneReq.targetLevel = request.playerLevel;
                cloneReq.faction = Faction::Alliance;
                cloneReq.minGearScore = request.minGearScore;
                cloneReq.dungeonIdToQueue = request.dungeonIdToQueue;
                cloneReq.battlegroundIdToQueue = request.battlegroundIdToQueue;
                cloneReq.arenaTypeToQueue = request.arenaTypeToQueue;

                auto clonedBots = BatchCloneWithRetry(cloneReq, MAX_RETRIES, progress);
                result.insert(result.end(), clonedBots.begin(), clonedBots.end());
            }
        }

        // Create Alliance healers
        {
            auto recycled = GetRecycledBotsForRole(BotRole::Healer, Faction::Alliance,
                request.playerLevel, request.minGearScore, allianceHealers);
            progress.fromRecycled += static_cast<uint32>(recycled.size());
            result.insert(result.end(), recycled.begin(), recycled.end());

            uint32 remaining = allianceHealers - static_cast<uint32>(recycled.size());
            if (remaining > 0)
            {
                BatchCloneRequest cloneReq;
                cloneReq.role = BotRole::Healer;
                cloneReq.count = remaining;
                cloneReq.targetLevel = request.playerLevel;
                cloneReq.faction = Faction::Alliance;
                cloneReq.minGearScore = request.minGearScore;
                cloneReq.dungeonIdToQueue = request.dungeonIdToQueue;
                cloneReq.battlegroundIdToQueue = request.battlegroundIdToQueue;
                cloneReq.arenaTypeToQueue = request.arenaTypeToQueue;

                auto clonedBots = BatchCloneWithRetry(cloneReq, MAX_RETRIES, progress);
                result.insert(result.end(), clonedBots.begin(), clonedBots.end());
            }
        }

        // Create Alliance DPS
        {
            auto recycled = GetRecycledBotsForRole(BotRole::DPS, Faction::Alliance,
                request.playerLevel, request.minGearScore, allianceDPS);
            progress.fromRecycled += static_cast<uint32>(recycled.size());
            result.insert(result.end(), recycled.begin(), recycled.end());

            uint32 remaining = allianceDPS - static_cast<uint32>(recycled.size());
            if (remaining > 0)
            {
                BatchCloneRequest cloneReq;
                cloneReq.role = BotRole::DPS;
                cloneReq.count = remaining;
                cloneReq.targetLevel = request.playerLevel;
                cloneReq.faction = Faction::Alliance;
                cloneReq.minGearScore = request.minGearScore;
                cloneReq.dungeonIdToQueue = request.dungeonIdToQueue;
                cloneReq.battlegroundIdToQueue = request.battlegroundIdToQueue;
                cloneReq.arenaTypeToQueue = request.arenaTypeToQueue;

                auto clonedBots = BatchCloneWithRetry(cloneReq, MAX_RETRIES, progress);
                result.insert(result.end(), clonedBots.begin(), clonedBots.end());
            }
        }

        progress.created = static_cast<uint32>(result.size());
        if (request.onProgress)
            request.onProgress(progress.GetProgress() * 0.5f);
    }

    // Create Horde bots
    if (request.hordeNeeded > 0)
    {
        TC_LOG_DEBUG("playerbot.jit", "JITBotFactory::CreatePvPBots - Creating {} Horde bots",
            request.hordeNeeded);

        uint32 hordeTanks = std::max(1u, request.hordeNeeded / 10);
        uint32 hordeHealers = std::max(1u, request.hordeNeeded * 15 / 100);
        uint32 hordeDPS = request.hordeNeeded - hordeTanks - hordeHealers;

        // Create Horde tanks
        {
            auto recycled = GetRecycledBotsForRole(BotRole::Tank, Faction::Horde,
                request.playerLevel, request.minGearScore, hordeTanks);
            progress.fromRecycled += static_cast<uint32>(recycled.size());
            result.insert(result.end(), recycled.begin(), recycled.end());

            uint32 remaining = hordeTanks - static_cast<uint32>(recycled.size());
            if (remaining > 0)
            {
                BatchCloneRequest cloneReq;
                cloneReq.role = BotRole::Tank;
                cloneReq.count = remaining;
                cloneReq.targetLevel = request.playerLevel;
                cloneReq.faction = Faction::Horde;
                cloneReq.minGearScore = request.minGearScore;
                cloneReq.dungeonIdToQueue = request.dungeonIdToQueue;
                cloneReq.battlegroundIdToQueue = request.battlegroundIdToQueue;
                cloneReq.arenaTypeToQueue = request.arenaTypeToQueue;

                auto clonedBots = BatchCloneWithRetry(cloneReq, MAX_RETRIES, progress);
                result.insert(result.end(), clonedBots.begin(), clonedBots.end());
            }
        }

        // Create Horde healers
        {
            auto recycled = GetRecycledBotsForRole(BotRole::Healer, Faction::Horde,
                request.playerLevel, request.minGearScore, hordeHealers);
            progress.fromRecycled += static_cast<uint32>(recycled.size());
            result.insert(result.end(), recycled.begin(), recycled.end());

            uint32 remaining = hordeHealers - static_cast<uint32>(recycled.size());
            if (remaining > 0)
            {
                BatchCloneRequest cloneReq;
                cloneReq.role = BotRole::Healer;
                cloneReq.count = remaining;
                cloneReq.targetLevel = request.playerLevel;
                cloneReq.faction = Faction::Horde;
                cloneReq.minGearScore = request.minGearScore;
                cloneReq.dungeonIdToQueue = request.dungeonIdToQueue;
                cloneReq.battlegroundIdToQueue = request.battlegroundIdToQueue;
                cloneReq.arenaTypeToQueue = request.arenaTypeToQueue;

                auto clonedBots = BatchCloneWithRetry(cloneReq, MAX_RETRIES, progress);
                result.insert(result.end(), clonedBots.begin(), clonedBots.end());
            }
        }

        // Create Horde DPS
        {
            auto recycled = GetRecycledBotsForRole(BotRole::DPS, Faction::Horde,
                request.playerLevel, request.minGearScore, hordeDPS);
            progress.fromRecycled += static_cast<uint32>(recycled.size());
            result.insert(result.end(), recycled.begin(), recycled.end());

            uint32 remaining = hordeDPS - static_cast<uint32>(recycled.size());
            if (remaining > 0)
            {
                BatchCloneRequest cloneReq;
                cloneReq.role = BotRole::DPS;
                cloneReq.count = remaining;
                cloneReq.targetLevel = request.playerLevel;
                cloneReq.faction = Faction::Horde;
                cloneReq.minGearScore = request.minGearScore;
                cloneReq.dungeonIdToQueue = request.dungeonIdToQueue;
                cloneReq.battlegroundIdToQueue = request.battlegroundIdToQueue;
                cloneReq.arenaTypeToQueue = request.arenaTypeToQueue;

                auto clonedBots = BatchCloneWithRetry(cloneReq, MAX_RETRIES, progress);
                result.insert(result.end(), clonedBots.begin(), clonedBots.end());
            }
        }

        progress.created = static_cast<uint32>(result.size());
        if (request.onProgress)
            request.onProgress(progress.GetProgress());
    }

    return result;
}

std::vector<ObjectGuid> JITBotFactory::GetRecycledBotsForRoleBracket(
    BotRole role,
    Faction faction,
    PoolBracket bracket,
    uint32 minGearScore,
    uint32 count)
{
    std::vector<ObjectGuid> result;
    result.reserve(count);

    std::lock_guard<std::mutex> lock(_recycleMutex);

    auto it = _recycledBots.begin();
    while (it != _recycledBots.end() && result.size() < count)
    {
        if (it->MatchesBracket(role, faction, bracket, minGearScore))
        {
            result.push_back(it->guid);
            it = _recycledBots.erase(it);
        }
        else
        {
            ++it;
        }
    }

    if (!result.empty())
    {
        TC_LOG_DEBUG("playerbot.jit", "JITBotFactory::GetRecycledBotsForRoleBracket - Found {} recycled bots for {} {} bracket {}",
            result.size(), BotRoleToString(role), FactionToString(faction), PoolBracketToString(bracket));
    }

    return result;
}

std::vector<ObjectGuid> JITBotFactory::GetRecycledBotsForRole(
    BotRole role,
    Faction faction,
    uint32 level,
    uint32 minGearScore,
    uint32 count)
{
    // Legacy level-based - convert to bracket-based
    PoolBracket bracket = GetBracketForLevel(level);
    return GetRecycledBotsForRoleBracket(role, faction, bracket, minGearScore, count);
}

uint8 JITBotFactory::GetPriorityForType(InstanceType type) const
{
    switch (type)
    {
        case InstanceType::Dungeon: return _config.dungeonPriority;
        case InstanceType::Arena: return _config.arenaPriority;
        case InstanceType::Raid: return _config.raidPriority;
        case InstanceType::Battleground: return _config.battlegroundPriority;
        default: return 5;
    }
}

std::chrono::milliseconds JITBotFactory::GetTimeoutForType(InstanceType type) const
{
    switch (type)
    {
        case InstanceType::Dungeon: return std::chrono::milliseconds(_config.dungeonTimeoutMs);
        case InstanceType::Arena: return std::chrono::milliseconds(_config.arenaTimeoutMs);
        case InstanceType::Raid: return std::chrono::milliseconds(_config.raidTimeoutMs);
        case InstanceType::Battleground: return std::chrono::milliseconds(_config.battlegroundTimeoutMs);
        default: return std::chrono::milliseconds(60000);
    }
}

void JITBotFactory::ProcessTimeouts()
{
    std::lock_guard<std::mutex> lock(_requestMutex);

    auto now = std::chrono::system_clock::now();

    for (auto it = _activeRequests.begin(); it != _activeRequests.end(); )
    {
        if (it->second.request.HasTimedOut())
        {
            TC_LOG_WARN("playerbot.jit", "JITBotFactory::ProcessTimeouts - Request {} timed out",
                it->first);

            it->second.progress.status = RequestStatus::TimedOut;
            if (it->second.request.onFailed)
                it->second.request.onFailed("Request timed out");

            _failedThisHour.fetch_add(1);
            it = _activeRequests.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

// ============================================================================
// ORPHANED CHARACTER CLEANUP
// ============================================================================

void JITBotFactory::CleanupOrphanedBotCharacters()
{
    TC_LOG_INFO("playerbot.jit", "JITBotFactory::CleanupOrphanedBotCharacters - Starting cleanup of orphaned JIT bot characters");

    // CRITICAL FIX: Only delete characters that are registered in the playerbot_jit_bots tracking table.
    // This preserves BotSpawner's world population characters which are designed to be REUSED.
    //
    // Previous implementation deleted ALL characters on @playerbot.local accounts, which:
    // 1. Destroyed BotSpawner's reusable character pool
    // 2. Caused unnecessary database churn on every restart
    // 3. Defeated the purpose of BotSpawner's character reuse optimization
    //
    // Now we ONLY delete JIT bots (tracked in playerbot_jit_bots) - BotSpawner bots are preserved.

    uint32 jitBotsDeleted = 0;

    try
    {
        // Step 1: Query the JIT bot tracking table for orphaned JIT bots
        // These are bots that were created by JITBotFactory but not cleaned up
        // (e.g., due to server crash before proper shutdown)
        // NOTE: playerbot_jit_bots is in the PLAYERBOT database, not characters
        QueryResult jitBotResult = sPlayerbotDatabase->Query(
            "SELECT jb.bot_guid, jb.account_id FROM playerbot_jit_bots jb");

        if (!jitBotResult)
        {
            TC_LOG_INFO("playerbot.jit", "JITBotFactory::CleanupOrphanedBotCharacters - No orphaned JIT bots found in tracking table");
            return;
        }

        // Collect JIT bot GUIDs to delete
        std::vector<std::pair<ObjectGuid, uint32>> jitBotsToDelete;
        do
        {
            Field* fields = jitBotResult->Fetch();
            ObjectGuid::LowType guidLow = fields[0].GetUInt64();
            uint32 accountId = fields[1].GetUInt32();
            ObjectGuid botGuid = ObjectGuid::Create<HighGuid::Player>(guidLow);
            jitBotsToDelete.push_back({botGuid, accountId});
        } while (jitBotResult->NextRow());

        TC_LOG_INFO("playerbot.jit", "JITBotFactory::CleanupOrphanedBotCharacters - Found {} orphaned JIT bots to clean up",
            jitBotsToDelete.size());

        // Step 2: Delete each JIT bot character
        for (auto const& [botGuid, accountId] : jitBotsToDelete)
        {
            // Delete the character using TrinityCore's proper deletion
            // This handles all related tables (inventory, spells, skills, etc.)
            Player::DeleteFromDB(botGuid, accountId, false /* updateRealmChars */, true /* deleteFinally */);
            ++jitBotsDeleted;

            TC_LOG_DEBUG("playerbot.jit",
                "JITBotFactory::CleanupOrphanedBotCharacters - Deleted orphaned JIT bot {} from account {}",
                botGuid.ToString(), accountId);
        }

        // Step 3: Clear the tracking table since we've deleted all tracked bots
        sPlayerbotDatabase->Execute("TRUNCATE TABLE playerbot_jit_bots");

        TC_LOG_INFO("playerbot.jit",
            "JITBotFactory::CleanupOrphanedBotCharacters - Cleanup complete: deleted {} orphaned JIT bots (BotSpawner characters preserved)",
            jitBotsDeleted);
    }
    catch (std::exception const& e)
    {
        TC_LOG_ERROR("playerbot.jit",
            "JITBotFactory::CleanupOrphanedBotCharacters - Exception during cleanup: {}", e.what());
    }
}

} // namespace Playerbot
