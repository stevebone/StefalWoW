/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#include "BotWorldSessionMgr.h"
#include "BotSession.h"
#include "BotPriorityManager.h"
#include "BotPerformanceMonitor.h"
#include "BotHealthCheck.h"
#include "Player.h"
#include "World.h"
#include "DatabaseEnv.h"
#include "CharacterCache.h"
#include "ObjectAccessor.h"
#include "Log.h"
#include "WorldSession.h"
#include "Account.h"
#include "Item.h"
#include "Group.h"
#include "LFGMgr.h"
#include "Config/PlayerbotConfig.h"
#include "../Character/BotLevelManager.h"
#include "../Performance/ThreadPool/ThreadPool.h"
#include "../Spatial/SpatialGridManager.h"
#include "../Spatial/SpatialGridScheduler.h"
#include "../AI/BotAI.h"
#include "../AI/Cache/AuraStateCache.h"
#include "../Lifecycle/DeathRecoveryManager.h"
#include "../Lifecycle/Instance/BotPostLoginConfigurator.h"  // For HasPendingConfiguration - skip JIT bots
#include "../Lifecycle/Instance/InstanceBotOrchestrator.h"   // For IsManagedBot - skip JIT bots from LevelManager
#include <chrono>
#include <queue>
#include "GameTime.h"

namespace Playerbot {

// ============================================================================
// STUCK TASK DETECTION: Track currently executing bot tasks for diagnostics
// ============================================================================
namespace {
    struct ExecutingTask {
        ObjectGuid guid;
        ::std::chrono::steady_clock::time_point startTime;
        ::std::string botName;
    };

    // Thread-safe registry of currently executing bot tasks
    ::std::mutex _executingTasksMutex;
    ::std::unordered_map<ObjectGuid, ExecutingTask> _executingTasks;

    void RegisterTaskStart(ObjectGuid guid, ::std::string const& botName)
    {
        ::std::lock_guard lock(_executingTasksMutex);
        _executingTasks[guid] = { guid, ::std::chrono::steady_clock::now(), botName };

        // DEBUG: Log registration to verify code is being called
        static uint32 regCount = 0;
        if (++regCount % 100 == 1)  // Log every 100th registration to avoid spam
        {
            TC_LOG_INFO("module.playerbot.session",
                "RegisterTaskStart: Bot {} (GUID: {}), total tracked: {}",
                botName, guid.ToString(), _executingTasks.size());
        }
    }

    void RegisterTaskEnd(ObjectGuid guid)
    {
        ::std::lock_guard lock(_executingTasksMutex);
        _executingTasks.erase(guid);
    }

    // Called when timeout occurs to identify stuck tasks
    void LogStuckTasks(uint32 thresholdMs)
    {
        ::std::lock_guard lock(_executingTasksMutex);
        auto now = ::std::chrono::steady_clock::now();

        size_t totalTracked = _executingTasks.size();
        size_t stuckCount = 0;

        for (auto const& [guid, task] : _executingTasks)
        {
            auto elapsed = ::std::chrono::duration_cast<::std::chrono::milliseconds>(now - task.startTime).count();
            if (elapsed > thresholdMs)
            {
                ++stuckCount;
                TC_LOG_ERROR("module.playerbot.session",
                    "STUCK TASK DETECTED: Bot {} (GUID: {}) has been executing for {}ms!",
                    task.botName, guid.ToString(), elapsed);
            }
        }

        // Always log summary for diagnostics
        TC_LOG_ERROR("module.playerbot.session",
            "LogStuckTasks: {} tasks tracked, {} stuck (>{} ms)",
            totalTracked, stuckCount, thresholdMs);
    }
} // anonymous namespace

// ============================================================================
// PHASE A: ThreadPool Integration - Priority Mapping
// ============================================================================

/**
 * @brief Map BotPriority to ThreadPool TaskPriority
 *
 * Mapping Strategy:
 * - EMERGENCY (health < 20%, combat death) ? CRITICAL (0-10ms tolerance)
 * - HIGH (combat, groups) ? HIGH (10-50ms tolerance)
 * - MEDIUM (active movement) ? NORMAL (50-200ms tolerance)
 * - LOW (idle, resting) ? LOW (200-1000ms tolerance)
 * - SUSPENDED (dead, disconnected) ? IDLE (no time constraints)
 */
inline Performance::TaskPriority MapBotPriorityToTaskPriority(BotPriority botPriority)
{
    switch (botPriority)
    {
        case BotPriority::EMERGENCY:
            return Performance::TaskPriority::CRITICAL;
        case BotPriority::HIGH:
            return Performance::TaskPriority::HIGH;
        case BotPriority::MEDIUM:
            return Performance::TaskPriority::NORMAL;
        case BotPriority::LOW:
            return Performance::TaskPriority::LOW;
        case BotPriority::SUSPENDED:
            return Performance::TaskPriority::IDLE;
        default:
            return Performance::TaskPriority::NORMAL;
    }
}

bool BotWorldSessionMgr::Initialize()
{
    if (_initialized.load())
        return true;

    TC_LOG_INFO("module.playerbot.session", "?? BotWorldSessionMgr: Initializing with native TrinityCore login pattern + Enterprise System");

    // Initialize enterprise components
    if (!sBotPriorityMgr->Initialize())
    {
        TC_LOG_ERROR("module.playerbot.session", "Failed to initialize BotPriorityManager");
        return false;
    }

    if (!sBotPerformanceMon->Initialize())
    {
        TC_LOG_ERROR("module.playerbot.session", "Failed to initialize BotPerformanceMonitor");
        return false;
    }

    if (!sBotHealthCheck->Initialize())
    {
        TC_LOG_ERROR("module.playerbot.session", "Failed to initialize BotHealthCheck");
        return false;
    }

    // Load spawn throttling config
    _maxSpawnsPerTick = sPlayerbotConfig->GetInt("Playerbot.LevelManager.MaxBotsPerUpdate", 10);

    // Initialize spatial grid system (grids created on-demand per map)
    TC_LOG_INFO("module.playerbot.session",
        "?? BotWorldSessionMgr: Spatial grid system initialized (lock-free double-buffered architecture)");

    _enabled.store(true);
    _initialized.store(true);

    TC_LOG_INFO("module.playerbot.session",
        "?? BotWorldSessionMgr: ENTERPRISE MODE enabled for 5000 bot scalability (spawn rate: {}/tick)",
        _maxSpawnsPerTick);

    return true;
}

void BotWorldSessionMgr::Shutdown()
{
    if (!_initialized.load())
        return;

    TC_LOG_INFO("module.playerbot.session", "?? BotWorldSessionMgr: Shutting down");

    _enabled.store(false);

    // Shutdown enterprise components first
    sBotHealthCheck->Shutdown();
    sBotPerformanceMon->Shutdown();
    sBotPriorityMgr->Shutdown();

    ::std::lock_guard lock(_sessionsMutex);

    // Clean logout all bot sessions
    for (auto& [guid, session] : _botSessions)
    {
        if (!session)
        {
            TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: session in Shutdown");
            continue;  // Skip null sessions
        }
        if (session && session->GetPlayer())
        {
            // MEMORY SAFETY: Protect against use-after-free when accessing Player name
            Player* player = session->GetPlayer();
            try {
                TC_LOG_INFO("module.playerbot.session", "🛑 Logging out bot: {}", player->GetName());
            }
            catch (...)
            {
                TC_LOG_INFO("module.playerbot.session", "🛑 Logging out bot (name unavailable - use-after-free protection)");
            }

            // PRE-CLEANUP: Clean up bot state before logout to prevent stuck auras
            // This helps avoid Unit::RemoveAllAuras() assertion during shutdown
            try
            {
                // Leave LFG queue first (prevents LFG-related aura issues)
                if (sLFGMgr->GetState(player->GetGUID()) != lfg::LFG_STATE_NONE)
                {
                    sLFGMgr->LeaveLfg(player->GetGUID());
                }

                // Leave group if in one (prevents group-related aura issues)
                if (Group* group = player->GetGroup())
                {
                    group->RemoveMember(player->GetGUID(), GROUP_REMOVEMETHOD_LEAVE, ObjectGuid::Empty, "Bot shutdown cleanup");
                }

                // Remove LFG-related auras that might be problematic during cleanup
                player->RemoveAurasDueToSpell(71328); // LFG_SPELL_DUNGEON_COOLDOWN
                player->RemoveAurasDueToSpell(71041); // LFG_SPELL_DUNGEON_DESERTER
                player->RemoveAurasDueToSpell(72221); // LFG_SPELL_LUCK_OF_THE_DRAW

                // CRITICAL FIX: Exit vehicle before logout (Vehicle crash prevention)
                if (player->GetVehicle())
                {
                    player->ExitVehicle();
                }

                // CRITICAL FIX (Map.cpp:1942 crash): Remove from _updateObjects BEFORE logout
                // This prevents Map::SendObjectUpdates from accessing the bot after destruction.
                static_cast<Object*>(player)->ClearUpdateMask(true);
            }
            catch (...)
            {
                TC_LOG_ERROR("module.playerbot.session", "🛑 Exception during bot pre-cleanup");
            }

            session->LogoutPlayer(true);
        }
    }

    _botSessions.clear();
    _botsLoading.clear();

    // Destroy all spatial grids and stop worker threads
    sSpatialGridManager.DestroyAllGrids();
    TC_LOG_INFO("module.playerbot.session", "?? BotWorldSessionMgr: Spatial grid system shut down");

    _initialized.store(false);
}

bool BotWorldSessionMgr::AddPlayerBot(ObjectGuid playerGuid, uint32 masterAccountId, bool bypassLimit)
{
    // TRACE: Log all AddPlayerBot calls to identify spawn origin
    TC_LOG_INFO("module.playerbot.session", "?? AddPlayerBot ENTRY - guid={}, accountId={}, bypassLimit={}",
        playerGuid.ToString(), masterAccountId, bypassLimit);

    if (!_enabled.load() || !_initialized.load())
    {
        TC_LOG_ERROR("module.playerbot.session", "?? BotWorldSessionMgr not enabled or initialized");
        return false;
    }

    ::std::lock_guard lock(_sessionsMutex);

    // Check if bot is already loading
    if (_botsLoading.find(playerGuid) != _botsLoading.end())
    {
        TC_LOG_DEBUG("module.playerbot.session", "?? Bot {} already loading", playerGuid.ToString());
        return false;
    }

    // FIX #22: CRITICAL - Eliminate ObjectAccessor call to prevent deadlock
    // Check if bot is already added by scanning existing sessions (avoids ObjectAccessor lock)
    for (auto const& [guid, session] : _botSessions)
    {
        if (guid == playerGuid && session && session->GetPlayer())
        {
            TC_LOG_DEBUG("module.playerbot.session", "?? Bot {} already in world (found in _botSessions)", playerGuid.ToString());
            return false;
        }
    }

    // Get account ID for this character from playerbot database directly
    uint32 accountId = 0;

    // Query playerbot_characters database directly since character cache won't find bot accounts
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_PINFO);
    stmt->setUInt64(0, playerGuid.GetCounter());
    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (result)
    {
        // CHAR_SEL_CHAR_PINFO returns: totaltime, level, money, account, race, class, map, zone, gender, health, playerFlags
        // Account is at index 3
        accountId = (*result)[3].GetUInt32();
        TC_LOG_DEBUG("module.playerbot.session", "?? Found account ID {} for character {}", accountId, playerGuid.ToString());
    }

    if (!accountId)
    {
        TC_LOG_ERROR("module.playerbot.session", "?? Could not find account for character {} in playerbot database", playerGuid.ToString());
        return false;
    }

    // CRITICAL FIX: Check for existing sessions by account ID to prevent duplicates
    for (auto const& [guid, session] : _botSessions)
    {
        if (session && session->GetAccountId() == accountId)
        {
            TC_LOG_WARN("module.playerbot.session", "?? DUPLICATE SESSION PREVENTION: Account {} already has an active bot session with character {}, rejecting new character {}",
                accountId, guid.ToString(), playerGuid.ToString());
            return false;
        }
    }

    // CRITICAL FIX: Enforce MaxBots limit (unless bypassed for Instance Bot Pool)
    if (!bypassLimit)
    {
        uint32 maxBots = sPlayerbotConfig->GetInt("Playerbot.MaxBots", 100);
        uint32 totalBots = static_cast<uint32>(_botSessions.size() + _pendingSpawns.size());
        if (totalBots >= maxBots)
        {
            TC_LOG_WARN("module.playerbot.session",
                "?? MAX BOTS LIMIT: Cannot queue bot {} - already at limit ({}/{} bots)",
                playerGuid.ToString(), totalBots, maxBots);
            return false;
        }
    }
    else
    {
        TC_LOG_DEBUG("module.playerbot.session",
            "?? Bypassing MaxBots limit for pool bot {}", playerGuid.ToString());
    }

    // CRITICAL FIX V3: RATE-LIMITED SPAWN QUEUE
    //
    // ROOT CAUSE OF SERVER HANG: "ENTERPRISE FIX V2" removed all throttling, causing:
    // - 296 bots spawning immediately at server startup
    // - 296  66 async queries = 19,536 database queries flooding connection pool
    // - ProcessQueryCallbacks() overwhelmed, world thread hangs 60+ seconds
    //
    // NEW SOLUTION: Queue-based rate-limited spawning
    // - AddPlayerBot() adds to _pendingSpawns queue (no immediate spawn)
    // - UpdateSessions() processes N spawns per tick (configurable, default 10)
    // - Prevents database overload while maintaining async benefits
    // - No threads, no deadlock, controlled flow
    //
    // Performance: MaxBotsPerUpdate config controls rate (10/tick = 10/50ms = 200 bots/sec)
    // Full 100-bot spawn completes in 500ms instead of overloading database instantly
    TC_LOG_INFO("module.playerbot.session",
        "?? Queueing bot {} for rate-limited spawn (queue position: {}, accountId: {})",
        playerGuid.ToString(), _pendingSpawns.size() + 1, accountId);

    // Add to pending spawn queue - will be processed in UpdateSessions()
    _pendingSpawns.push_back({playerGuid, accountId});

    TC_LOG_DEBUG("module.playerbot.session",
        "?? Bot {} added to spawn queue. Total pending: {}",
        playerGuid.ToString(), _pendingSpawns.size());

    return true; // Queued successfully
}
void BotWorldSessionMgr::RemovePlayerBot(ObjectGuid playerGuid)
{
    // CRITICAL FIX (Map.cpp:686 crash): Do NOT call LogoutPlayer() synchronously!
    // Problem: RemovePlayerBot() can be called from commands while Map::Update() is running
    // If we call LogoutPlayer() here, it removes player from map IMMEDIATELY (WorldSession.cpp:715)
    // This invalidates Map iterators, causing Map.cpp:686 crash
    //
    // Solution: Use async disconnection queue (same pattern as worker thread disconnects)
    // The session will be cleaned up in next UpdateSessions() call on main thread
    // when Map::Update() is NOT iterating over players
    ::std::lock_guard lock(_sessionsMutex);

    auto it = _botSessions.find(playerGuid);
    if (it == _botSessions.end())
    {
        TC_LOG_DEBUG("module.playerbot.session", "?? Bot session not found for removal: {}", playerGuid.ToString());
        return;
    }

    ::std::shared_ptr<BotSession> session = it->second;
    if (session)
    {
        // Log removal (safely handle name access)
        Player* player = session->GetPlayer();
        if (player)
        {
            try {
                TC_LOG_INFO("module.playerbot.session", "Queuing bot for removal: {}", player->GetName());
            }
            catch (...)
            {
                TC_LOG_INFO("module.playerbot.session", "Queuing bot for removal (name unavailable)");
            }

            // CRITICAL FIX (Map.cpp:1968 use-after-free crash):
            // Mark bot as destroyed BEFORE clearing update mask to prevent re-adding to _updateObjects.
            //
            // Problem: After ClearUpdateMask(true) removes bot from _updateObjects, bot AI continues
            // running and may modify properties. Property setters call AddToObjectUpdateIfNeeded()
            // which re-adds the bot to _updateObjects. When bot is finally destroyed during logout,
            // MapUpdater worker threads find a dangling pointer -> ACCESS_VIOLATION.
            //
            // Solution: Set m_isDestroyedObject=true FIRST. This blocks AddToObjectUpdateIfNeeded()
            // from ever re-adding the bot to _updateObjects (check added in BaseEntity.cpp).
            player->SetDestroyedObject(true);
            TC_LOG_DEBUG("module.playerbot.session", "Marked bot {} as destroyed to prevent _updateObjects re-add", playerGuid.ToString());

            // CRITICAL FIX (Cell::Visit crash - CellImpl.h:65):
            // Clear visibility notification flags BEFORE queuing for removal.
            //
            // Problem: DelayedUnitRelocation::Visit() (GridNotifiers.cpp:222) iterates over
            // players in the grid and calls Cell::VisitAllObjects() for each player that has
            // NOTIFY_VISIBILITY_CHANGED set. If a bot is being removed while this iteration
            // runs on the MapUpdater worker thread, the bot's Map pointer may become corrupted
            // (observed value: 0x400000004E instead of valid pointer), causing ACCESS_VIOLATION.
            //
            // Solution: Clear all notification flags to prevent the bot from being processed
            // by Map::ProcessRelocationNotifies() after removal is queued.
            if (player->IsInWorld())
            {
                player->ResetAllNotifies();
                TC_LOG_DEBUG("module.playerbot.session", "Cleared visibility flags for bot {} to prevent Cell::Visit crash", playerGuid.ToString());
            }

            // CRITICAL FIX (Map.cpp:1945 crash - Map::SendObjectUpdates crash):
            // Clear update mask BEFORE queuing for removal to remove from Map::_updateObjects.
            //
            // Problem: Map::SendObjectUpdates() iterates over _updateObjects on MapUpdater worker
            // threads. If a bot is queued for removal here but still has pending updates in
            // _updateObjects, the worker thread may access a stale pointer causing ACCESS_VIOLATION.
            //
            // Race condition:
            //   1. Main thread: RemovePlayerBot() queues bot for deferred logout
            //   2. Main thread: MapManager::Update() spawns worker threads
            //   3. Worker thread: Map::SendObjectUpdates() iterates _updateObjects
            //   4. Worker thread: Accesses bot that's queued for removal -> CRASH
            //
            // Solution: Call ClearUpdateMask(true) which calls RemoveFromObjectUpdate()
            // to remove the bot from _updateObjects IMMEDIATELY, before MapUpdater workers run.
            // This is safe because:
            //   - We're on the main thread (OnBeforeWorldUpdate callback)
            //   - MapUpdater workers haven't started yet for this tick
            //   - The bot won't receive any more updates anyway (it's being removed)
            //
            // Note: Player::ClearUpdateMask is protected, but Object::ClearUpdateMask is public.
            // We cast to Object* to access the base class public method.
            static_cast<Object*>(player)->ClearUpdateMask(true);  // true = remove from _updateObjects
            TC_LOG_DEBUG("module.playerbot.session", "Cleared update mask for bot {} to prevent Map::SendObjectUpdates crash", playerGuid.ToString());

            // CRITICAL FIX (Map.cpp:1981 use-after-free crash in SendObjectUpdates):
            // Also mark the BattlenetAccount as destroyed and clear its update objects.
            //
            // Problem: Player, Account, and Item each have independent BaseEntity instances
            // that are independently added to the SAME Map::_updateObjects set. Clearing only
            // the Player's update mask leaves Account/Item pointers in _updateObjects.
            // When the bot is eventually destroyed during async disconnect, Account/Item memory
            // is freed but their stale pointers persist in _updateObjects -> ACCESS_VIOLATION.
            //
            // Solution: Mark Account as destroyed (blocks re-add) and clear its update mask.
            // For items, we mark them as destroyed and clear from _updateObjects too.
            if (WorldSession* ws = player->GetSession())
            {
                Battlenet::Account& account = ws->GetBattlenetAccount();
                account.SetDestroyedObject(true);
                account.ClearUpdateMask(true);
                TC_LOG_DEBUG("module.playerbot.session", "Cleared Account update objects for bot {}", playerGuid.ToString());
            }

            // Clear item update objects (items have independent BaseEntity in _updateObjects)
            for (uint8 i = PLAYER_SLOT_START; i < PLAYER_SLOT_END; ++i)
            {
                if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                {
                    item->SetDestroyedObject(true);
                    static_cast<Object*>(item)->ClearUpdateMask(true);
                }
            }
            TC_LOG_DEBUG("module.playerbot.session", "Cleared Item update objects for bot {}", playerGuid.ToString());
        }

        // Signal session termination - BotSession::Update() will return false next cycle
        session->KickPlayer("BotWorldSessionMgr::RemovePlayerBot - Bot removal requested");
    }

    // Push to async disconnection queue (lock-free, thread-safe)
    // Will be processed in UpdateSessions() Phase 3 when safe (after Map::Update() completes)
    _asyncDisconnections.push(playerGuid);

    TC_LOG_DEBUG("module.playerbot.session", "?? Bot {} queued for async removal", playerGuid.ToString());
}

Player* BotWorldSessionMgr::GetPlayerBot(ObjectGuid playerGuid) const
{
    ::std::lock_guard lock(_sessionsMutex);

    auto it = _botSessions.find(playerGuid);
    if (it == _botSessions.end())
        return nullptr;

    return it->second ? it->second->GetPlayer() : nullptr;
}

void BotWorldSessionMgr::UpdateSessions(uint32 diff)
{
    if (!_enabled.load())
        return;

    // ENTERPRISE-GRADE PRIORITY-BASED UPDATE SYSTEM
    // Designed for 5000+ concurrent bots with optimal performance
    //
    // Phase 1 (Simple rotation): 78% improvement (600ms ? 150ms)
    // Phase 2 (Enterprise priority): Targets 145ms per tick with 5000 bots
    //
    // Priority Levels:
    // - EMERGENCY (5 bots): Every tick - critical states
    // - HIGH (45 bots): Every tick - combat, groups
    // - MEDIUM (40 bots): Every 10 ticks - active movement
    // - LOW (91 bots): Every 50 ticks - idle, resting
    //
    // Total per-tick load: 5 + 45 + 40 + 91 = 181 bots @ 0.8ms = 145ms target

    uint32 currentTime = GameTime::GetGameTimeMS();
    _tickCounter++;

    // PHASE 0: Enterprise monitoring - Begin tick measurement
    sBotPerformanceMon->BeginTick(currentTime);
    sBotHealthCheck->RecordHeartbeat(currentTime);

    // PHASE 0.5: CRITICAL FIX - Process pending spawns at controlled rate
    // Prevents database overload by rate-limiting async login submissions
    {
        ::std::lock_guard lock(_sessionsMutex);

        _spawnsProcessedThisTick = 0;

        while (!_pendingSpawns.empty() && _spawnsProcessedThisTick < _maxSpawnsPerTick)
        {
            auto [playerGuid, accountId] = _pendingSpawns.front();
            _pendingSpawns.erase(_pendingSpawns.begin());

            TC_LOG_INFO("module.playerbot.session",
                "?? Processing queued spawn for bot {} (accountId: {}, remaining in queue: {})",
                playerGuid.ToString(), accountId, _pendingSpawns.size());

            // Synchronize character cache
    if (!SynchronizeCharacterCache(playerGuid))
            {
                TC_LOG_ERROR("module.playerbot.session",
                    "?? Failed to synchronize character cache for {}", playerGuid.ToString());
                continue;
            }

            // Mark as loading
            _botsLoading.insert(playerGuid);

            // Create BotSession
            ::std::shared_ptr<BotSession> botSession = BotSession::Create(accountId);
            if (!botSession)
            {
                TC_LOG_ERROR("module.playerbot.session",
                    "?? Failed to create BotSession for {}", playerGuid.ToString());
                _botsLoading.erase(playerGuid);
                continue;
            }

            // Store session
            _botSessions[playerGuid] = botSession;

            // Initiate async login (1 bot  66 queries)
    if (!botSession->LoginCharacter(playerGuid))
            {
                TC_LOG_ERROR("module.playerbot.session",
                    "?? Failed to initiate async login for {}", playerGuid.ToString());
                _botSessions.erase(playerGuid);
                _botsLoading.erase(playerGuid);
                continue;
            }

            TC_LOG_INFO("module.playerbot.session",
                "? Async bot login initiated for: {} ({}/{} spawns this tick)",
                playerGuid.ToString(), _spawnsProcessedThisTick + 1, _maxSpawnsPerTick);

            _spawnsProcessedThisTick++;
        }

        if (_spawnsProcessedThisTick > 0)
        {
            TC_LOG_INFO("module.playerbot.session",
                "?? Processed {} bot spawns this tick. Remaining in queue: {}",
                _spawnsProcessedThisTick, _pendingSpawns.size());
        }
    }

    // DEADLOCK FIX: Update all spatial grids ONCE before bot updates
    // This prevents 25+ threads from all trying to update simultaneously
    // Single, controlled update point = no contention = no deadlock
    if (_tickCounter % 2 == 0)  // Update every 100ms (every 2 ticks at 50ms/tick)
    {
        sSpatialGridScheduler.UpdateAllGrids(diff);
    }

    ::std::vector<::std::pair<ObjectGuid, ::std::shared_ptr<BotSession>>> sessionsToUpdate;
    ::std::vector<ObjectGuid> sessionsToRemove;
    uint32 botsSkipped = 0;

    // PHASE 1: Priority-based session collection (minimal lock time)
    {
        ::std::lock_guard lock(_sessionsMutex);

        if (_botSessions.empty() && _pendingSpawns.empty())
        {
            sBotPerformanceMon->EndTick(currentTime, 0, 0);
            return;
        }

        sessionsToUpdate.reserve(200); // Reserve for typical load
    for (auto it = _botSessions.begin(); it != _botSessions.end(); ++it)
        {
            ObjectGuid guid = it->first;
            ::std::shared_ptr<BotSession> session = it->second;

            // CRITICAL SAFETY: Validate session
    if (!session || !session->IsBot())
            {
                sessionsToRemove.push_back(guid);
                continue;
            }

            // Check if bot should update this tick based on priority
    if (_enterpriseMode && !sBotPriorityMgr->ShouldUpdateThisTick(guid, _tickCounter))
            {
                sBotPriorityMgr->RecordUpdateSkipped(guid);
                botsSkipped++;
                continue;
            }

            // Handle loading sessions
    if (_botsLoading.find(guid) != _botsLoading.end())
            {
                if (session->IsLoginComplete())
                {
                    _botsLoading.erase(guid);
                    TC_LOG_INFO("module.playerbot.session", "?? ? Bot login completed: {}", guid.ToString());

                    // Initialize priority for newly logged-in bot
                    // FIX: Changed from LOW to MEDIUM to ensure responsive updates
                    // LOW priority (50 tick interval = 2.5s) was too slow for:
                    // - Group invitation processing
                    // - Initial movement/strategy activation
                    // - Quest giver detection
                    // MEDIUM (10 tick interval = 500ms) gives bots time to:
                    // - Process group invites promptly
                    // - Start moving and get promoted to higher priority
                    // AutoAdjustPriority() will demote idle bots to LOW after 2.5s
    if (_enterpriseMode)
                        sBotPriorityMgr->SetPriority(guid, BotPriority::MEDIUM);

                    // ================================================================
                    // CRITICAL FIX: Submit bot to BotLevelManager for level/gear setup
                    // ================================================================
                    // When LevelManager is enabled, newly logged-in bots need to be
                    // submitted for level assignment, talent setup, gear, and zone placement.
                    // Without this, bots spawn at level 1 and never get leveled up.
                    //
                    // CRITICAL: SKIP JIT bots - they have pending configuration from
                    // BotPostLoginConfigurator which sets their level specifically for
                    // the content they were created for (BG, dungeon, raid).
                    // BotLevelManager would override that level with distribution-based
                    // redistribution, causing JIT bots to get wrong levels.
                    // ================================================================
                    if (sBotLevelManager->IsReady())
                    {
                        Player* bot = session->GetPlayer();
                        if (bot)
                        {
                            // CRITICAL: Skip JIT bots - they're configured by BotPostLoginConfigurator
                            // Check all three conditions:
                            // 1. HasPendingConfiguration - config not yet applied
                            // 2. WasRecentlyConfigured - config was applied but not yet cleared (prevents race condition)
                            // 3. IsManagedBot - config already applied, bot managed by orchestrator
                            bool wasRecentlyConfigured = sBotPostLoginConfigurator->WasRecentlyConfigured(guid);
                            if (sBotPostLoginConfigurator->HasPendingConfiguration(guid) ||
                                wasRecentlyConfigured ||
                                sInstanceBotOrchestrator->IsManagedBot(guid))
                            {
                                TC_LOG_INFO("module.playerbot.session",
                                    "Bot {} is JIT-configured - SKIPPING LevelManager submission to preserve target level (wasRecent={})",
                                    bot->GetName(), wasRecentlyConfigured ? "YES" : "NO");

                                // Clear the recently configured flag now that we've processed this bot
                                if (wasRecentlyConfigured)
                                {
                                    sBotPostLoginConfigurator->ClearRecentlyConfigured(guid);
                                }
                            }
                            else
                            {
                                // Regular pool bot - submit to LevelManager for distribution-based leveling
                                uint64 taskId = sBotLevelManager->CreateBotAsync(bot);
                                if (taskId > 0)
                                {
                                    TC_LOG_INFO("module.playerbot.session",
                                        "Bot {} submitted to LevelManager (task {})",
                                        bot->GetName(), taskId);
                                }
                            }
                        }
                    }

                    sessionsToUpdate.emplace_back(guid, session);
                }
                else if (session->IsLoginFailed())
                {
                    TC_LOG_ERROR("module.playerbot.session", "?? Bot login failed: {}", guid.ToString());
                    _botsLoading.erase(guid);
                    sessionsToRemove.push_back(guid);
                }
                else
                {
                    sessionsToUpdate.emplace_back(guid, session); // Still loading
                }
            }
            else
            {
                sessionsToUpdate.emplace_back(guid, session);
            }
        }

        // Clean up invalid sessions
    for (ObjectGuid const& guid : sessionsToRemove)
        {
            _botSessions.erase(guid);
            sBotPriorityMgr->RemoveBot(guid);
        }
    } // Release mutex - CRITICAL for deadlock prevention

    // PHASE 2: Update sessions with ThreadPool (parallel execution for 7x speedup)
    //
    // PHASE A THREADPOOL INTEGRATION: Replace sequential bot updates with parallel execution
    //
    // Before (Sequential): 145 bots  1ms = 145ms
    // After (Parallel, 8 threads): 145 bots  8 threads = 18 bots/thread  1ms = 18ms (7x speedup)
    //
    // Architecture:
    // 1. Submit each bot update as a task to ThreadPool with mapped priority
    // 2. Capture shared_ptr by value (thread-safe reference counting)
    // 3. Collect futures for synchronization barrier
    // 4. Handle errors through concurrent queue (thread-safe)
    // 5. Wait for all tasks to complete before proceeding
    //
    // CRITICAL SAFETY: Check if ThreadPool is safe to use (World must be fully initialized)
    // During server startup, World components aren't ready and ThreadPool workers would crash
    // Fallback to sequential execution if parallel execution isn't safe yet

    ::std::vector<ObjectGuid> disconnectedSessions;

    // OPTION 5: _asyncBotsUpdated and _asyncDisconnections now in header (class members)
    // Thread-safe without mutex - using boost::lockfree::queue and std::atomic

    // CRITICAL SAFETY: Never use ThreadPool during server startup
    // Check if we have any active bots that have completed login
    // During startup, no bots should be fully logged in yet
    bool serverReady = false;
    for (auto& [guid, session] : sessionsToUpdate)
    {
        if (session && session->IsLoginComplete())
        {
            serverReady = true;
            break;
        }
    }

    // ThreadPool enabled with CPU affinity removed to prevent 60s hang
    // CPU affinity (SetThreadAffinityMask) was causing blocking during thread creation
    // Workers now create without affinity and OS scheduler handles CPU assignment
    //
    // OPTION 5: Fire-and-forget - no futures to track
    // Tasks complete asynchronously and report via _asyncDisconnections queue
    bool useThreadPool = serverReady && !sessionsToUpdate.empty();

    // Cache member variables before lambda capture to avoid threading issues
    bool enterpriseMode = _enterpriseMode;
    uint32 tickCounter = _tickCounter;

    if (useThreadPool)
    {
        // CRITICAL FIX: Check ThreadPool saturation BEFORE submitting tasks
        // If workers are busy with previous batch, new tasks queue up and cause cascading delays
        // Better to skip this update cycle than create 10+ second backlogs
        size_t queuedTasks = Performance::GetThreadPool().GetQueuedTasks();
        size_t activeWorkers = Performance::GetThreadPool().GetActiveThreads();
        uint32 workerCount = Performance::GetThreadPool().GetWorkerCount();

        // REVISED SATURATION LOGIC: Check both queue depth AND worker availability
        //
        // Problem: Queue depth alone doesn't prevent 10s delays. Even with queue capacity,
        // if all workers are busy and we submit more tasks, the last task can wait
        // 10+ seconds in a worker's queue while that worker processes 10+ sequential tasks.
        //
        // Solution: Also check worker availability. If most workers are busy, skip updates.
        //
        // Thresholds:
        // 1. Queue depth: >100 tasks = backlog building up
        // 2. Worker utilization: >80% busy = insufficient capacity for new batch
        //
        // Example with 30 workers (32 cores - 2):
        // - >100 queued tasks = ~3 tasks per worker queued = ~300ms backlog (acceptable)
        // - >24 workers busy (80%) = only 6 free workers for new batch
        //
        // Example with 16 workers (18 cores - 2):
        // - >100 queued tasks = ~6 tasks per worker queued = ~600ms backlog
        // - >13 workers busy (80%) = only 3 free workers for new batch
        //
        // Rationale:
        // - 100 task threshold scales with worker count (3-6 tasks per worker)
        // - 80% busy threshold ensures enough free workers to handle new batch quickly
        // - Prevents scenario where new tasks queue behind long-running tasks
        // - Priority system spreads bots across ticks, so actual per-tick load is manageable
        uint32 busyThreshold = (workerCount * 4) / 5;  // 80% of workers

        // Safety: Ensure minimum threshold to handle edge cases
        // Even with minimum 16 workers enforced by ThreadPool, be defensive
        if (busyThreshold < 3)
            busyThreshold = 3;

        // CRITICAL FIX: Only skip if there's ACTUAL backlog (queued tasks)
        // Workers being active is NORMAL - they're processing current batch
        // We should only skip if NEW tasks would pile up behind existing queued tasks
        //
        // OLD (broken): queuedTasks > 100 || activeWorkers > busyThreshold
        //   - Skipped updates even when queue was EMPTY just because workers were busy
        //   - Caused bots to do nothing because updates were constantly skipped
        //
        // NEW: Only skip if queue has significant backlog AND workers are saturated
        if (queuedTasks > 100 && activeWorkers > busyThreshold)
        {
            TC_LOG_WARN("module.playerbot.session",
                "ThreadPool saturated (queue: {} tasks, active: {}/{} workers, busy threshold: {}) - skipping bot updates this tick",
                queuedTasks, activeWorkers, workerCount, busyThreshold);
            return;  // Skip this update cycle to let workers catch up
        }
    }

    // ========================================================================
    // THREAD-SAFETY: Pre-populate AuraStateCache on main thread BEFORE worker dispatch
    // ========================================================================
    //
    // CRITICAL: BehaviorTree nodes execute on ThreadPool worker threads and need
    // to query aura state. Unit::HasAura() is NOT thread-safe - it accesses
    // _appliedAuras which is also accessed by main thread (AreaTrigger::Update).
    //
    // Solution: Cache aura state here on main thread. Worker threads read from cache.
    //
    // Performance: ~0.1ms per bot for cache update (negligible vs 7x parallel speedup)
    //
    if (useThreadPool)
    {
        // Periodic cleanup of expired cache entries (every 10 ticks)
        static uint32 cacheCleanupCounter = 0;
        if (++cacheCleanupCounter >= 10)
        {
            sAuraStateCache->CleanupExpired();
            cacheCleanupCounter = 0;
        }

        // Pre-populate aura cache for all active bots
        for (auto& [guid, botSession] : sessionsToUpdate)
        {
            if (!botSession || !botSession->IsActive() || !botSession->IsLoginComplete())
                continue;

            Player* bot = botSession->GetPlayer();
            if (!bot || !bot->IsInWorld())
                continue;

            // Update bot's own auras (buffs, debuffs on self)
            sAuraStateCache->UpdateBotAuras(bot);

            // Update target's auras (debuffs applied BY bot TO target)
            if (Unit* target = bot->GetVictim())
            {
                sAuraStateCache->UpdateTargetAuras(bot, target);
            }
        }
    }

    for (auto& [guid, botSession] : sessionsToUpdate)
    {
        // Validate session before submitting task
    if (!botSession || !botSession->IsActive())
        {
            disconnectedSessions.push_back(guid);
            continue;
        }

        // ====================================================================
        // INSTANCE BOT IDLE LOGOUT CHECK
        // ====================================================================
        // Instance bots (JIT/warm pool) are for instanced content only.
        // They auto-logout after 60 seconds of being idle (not in queue/group).
        // This reduces server load when bots are not actively in use.
        // ====================================================================
        if (botSession->IsInstanceBot() && botSession->IsLoginComplete())
        {
            if (botSession->UpdateIdleStateAndCheckLogout(diff))
            {
                // Bot has been idle for 60+ seconds - schedule for logout
                Player* bot = botSession->GetPlayer();
                TC_LOG_INFO("module.playerbot.instance",
                    "📴 Instance bot {} idle timeout - scheduling logout to reduce server load",
                    bot ? bot->GetName() : guid.ToString().c_str());
                disconnectedSessions.push_back(guid);
                continue;
            }
        }

        // OPTION 5: Capture weak_ptr for session lifetime detection
        ::std::weak_ptr<BotSession> weakSession = botSession;

        // Capture bot name for stuck task diagnostics (before lambda capture)
        ::std::string botNameForDiag = botSession->GetPlayer() ? botSession->GetPlayer()->GetName() : guid.ToString();

        // Define the update logic (will be used in both parallel and sequential paths)
        auto updateLogic = [guid, weakSession, diff, currentTime, enterpriseMode, tickCounter, botNameForDiag, this]()
            {
                TC_LOG_TRACE("playerbot.session.task", "?? TASK START for bot {}", guid.ToString());

                // STUCK TASK DETECTION: Register this task as executing
                RegisterTaskStart(guid, botNameForDiag);

                // RAII guard to ensure task is always unregistered
                struct TaskEndGuard {
                    ObjectGuid _guid;
                    ~TaskEndGuard() { RegisterTaskEnd(_guid); }
                } taskGuard{guid};

                // OPTION 5: Check if session still exists (thread-safe with weak_ptr)
                auto session = weakSession.lock();
                if (!session)
                {
                    TC_LOG_WARN("module.playerbot.session", "?? Bot session destroyed during update: {}", guid.ToString());
                    _asyncDisconnections.push(guid);  // Lock-free push
                    return;
                }

                try
                {
                    // Create PacketFilter for bot session
                    class BotPacketFilter : public PacketFilter
                    {
                    public:
                        explicit BotPacketFilter(WorldSession* session) : PacketFilter(session) {}
                        virtual ~BotPacketFilter() override = default;
                        bool Process(WorldPacket*) override { return true; }
                        bool ProcessUnsafe() const override { return true; }
                    } filter(session.get());

                    // Update session
                    TC_LOG_TRACE("playerbot.session.update", "?? Starting Update() for bot {}", guid.ToString());
                    bool updateResult = session->Update(diff, filter);

                    if (!updateResult)
                    {
                        TC_LOG_WARN("module.playerbot.session", "?? Bot update failed: {}", guid.ToString());
                        _asyncDisconnections.push(guid);  // Lock-free push
                        return;
                    }

                    // Increment success counter (atomic, thread-safe)
                    _asyncBotsUpdated.fetch_add(1, ::std::memory_order_relaxed);

                    // IMPROVEMENT #1: ADAPTIVE AutoAdjustPriority frequency based on bot activity
                    // Active bots (combat/group) = more frequent checks (250ms)
                    // Idle bots (high health, not moving) = less frequent checks (2.5s)
                    if (enterpriseMode && session->IsLoginComplete())
                    {
                        Player* bot = session->GetPlayer();
                        // CRITICAL FIX: Do NOT logout bots that are temporarily not in world!
                        // During teleportation, map changes, or loading screens, bots can have
                        // IsInWorld() == false temporarily. This is NORMAL and should NOT trigger logout.
                        // Only skip the priority adjustment if bot is not available.
                        if (!bot || !bot->IsInWorld())
                        {
                            // Bot is transitioning (teleporting, loading) - skip priority check but DON'T logout
                            TC_LOG_TRACE("module.playerbot.session", "Bot {} not in world (transitioning?) - skipping priority adjustment", guid.ToString());
                            // DO NOT push to _asyncDisconnections - bot is just transitioning!
                        }
                        else
                        {
                            // Adaptive frequency: Adjust interval based on bot activity
                            uint32 adjustInterval = 10; // Default 500ms
                            if (bot->IsInCombat() || bot->GetGroup())
                                adjustInterval = 5;  // Active bots: 250ms (more responsive)
                            else if (!bot->isMoving() && bot->GetHealthPct() > 80.0f)
                                adjustInterval = 50; // Idle healthy bots: 2.5s (save CPU)

                            // Call AutoAdjustPriority at adaptive interval
                            if (tickCounter % adjustInterval == 0)
                            {
                                sBotPriorityMgr->AutoAdjustPriority(bot, currentTime);
                            }
                            // Fast-path critical state detection on other ticks (lightweight checks only)
                            else
                            {
                                // Immediate priority boost for critical situations (no group/movement checks)
                                if (bot->IsInCombat())
                                {
                                    sBotPriorityMgr->SetPriority(guid, BotPriority::HIGH);
                                }
                                else if (bot->GetHealthPct() < 20.0f)
                                {
                                    sBotPriorityMgr->SetPriority(guid, BotPriority::EMERGENCY);
                                }
                                // CRITICAL FIX: Pending group invitation needs fast response
                                // Without this boost, bot may timeout waiting for next update
                                else if (bot->GetGroupInvite())
                                {
                                    sBotPriorityMgr->SetPriority(guid, BotPriority::MEDIUM);
                                }
                            }
                        }
                    }
                }
                catch (::std::exception const& e)
                {
                    TC_LOG_ERROR("module.playerbot.session", "?? Exception updating bot {}: {}", guid.ToString(), e.what());
                    sBotHealthCheck->RecordError(guid, "UpdateException");
                    _asyncDisconnections.push(guid);  // Lock-free push
                }
                catch (...)
                {
                    TC_LOG_ERROR("module.playerbot.session", "?? Unknown exception updating bot {}", guid.ToString());
                    sBotHealthCheck->RecordError(guid, "UnknownException");
                    _asyncDisconnections.push(guid);  // Lock-free push
                }

                TC_LOG_TRACE("playerbot.session.task", "?? TASK END for bot {}", guid.ToString());
            };

        // Execute either parallel (ThreadPool) or sequential (direct call)
        // ============================================================================
        // CRITICAL FIX: Sessions with pending login MUST run on main thread!
        // ============================================================================
        // Problem: When LOGIN_IN_PROGRESS, the session has a pending SQL callback that
        //          will call HandleBotPlayerLogin → Map::AddPlayerToMap.
        //          Map operations are NOT thread-safe and MUST run on the map update thread.
        //          Running this on a thread pool worker causes ACCESS_VIOLATION crash in
        //          TerrainInfo::LoadMapAndVMap when acquiring the terrain mutex.
        //
        // Solution: Force sequential (main thread) execution for sessions that are
        //           actively logging in. Once login is complete, they can use thread pool.
        // ============================================================================
        bool useThreadPoolForThisSession = useThreadPool;
        if (botSession->GetLoginState() == BotSession::LoginState::LOGIN_IN_PROGRESS)
        {
            useThreadPoolForThisSession = false;  // Force main thread for login operations
            TC_LOG_DEBUG("module.playerbot.session", "?? Bot {} has pending login - forcing main thread update", guid.ToString());
        }

    if (useThreadPoolForThisSession)
        {
            // OPTION 5: Fire-and-forget submission (no future storage)
            try
            {
                BotPriority botPriority = sBotPriorityMgr->GetPriority(guid);
                Performance::TaskPriority taskPriority = MapBotPriorityToTaskPriority(botPriority);

                // Submit and immediately discard future - task runs async
                Performance::GetThreadPool().Submit(taskPriority, updateLogic);

                TC_LOG_TRACE("playerbot.session.submit", "?? Submitted async task for bot {}", guid.ToString());
            }
            catch (...)
            {
                // ThreadPool failed - execute sequentially
                useThreadPool = false;
                updateLogic();
            }
        }
        else
        {
            // Sequential path: Execute directly
            updateLogic();
        }
    }

    // ============================================================================
    // CRITICAL FIX (Map.cpp:686 crash): Wait for all bot update tasks to complete
    // ============================================================================
    // Problem: Bot update tasks run on ThreadPool workers CONCURRENTLY with main thread.
    //          If we process deferred logouts (calling LogoutPlayer) while workers are still
    //          accessing player data, we create a race condition. LogoutPlayer removes the
    //          player from the map immediately, which can corrupt Map iterators.
    //
    // Timing Issue:
    //   1. Main thread submits 100 bot update tasks to ThreadPool (line 718)
    //   2. Main thread IMMEDIATELY processes deferred logouts (calls LogoutPlayer)
    //   3. ThreadPool workers are STILL running, accessing player data
    //   4. Player removed from map while worker accesses it → Map.cpp:686 crash
    //
    // Solution: Wait for all ThreadPool tasks to complete before processing logouts.
    //           ONLY wait if there are actually pending logouts to process.
    //           Use short timeout (50ms) to avoid blocking world update.
    //           If timeout, skip logout processing this tick (will retry next tick).
    // ============================================================================
    bool canProcessLogouts = true;  // Assume we can process logouts

    // CRITICAL FIX (Unit.cpp:437 m_procDeep crash): ALWAYS wait for ThreadPool completion!
    // Problem: Bot ThreadPool workers call CastSpell which triggers procs (m_procDeep++).
    //          If workers are still running when MapUpdater starts, Map::Update calls
    //          Unit::Update which asserts !m_procDeep - but worker has it incremented!
    // Solution: Wait for ALL bot update tasks to complete before returning from UpdateSessions.
    //           This ensures procs are fully resolved before MapUpdater runs.
    //
    // CRITICAL: We MUST wait indefinitely - a 50ms timeout is NOT sufficient!
    // If we return while workers are still running, MapUpdater WILL crash on m_procDeep assertion.
    // The timeout approach only deferred logout processing but did NOT prevent the crash.
    //
    // CRITICAL FIX (Map.cpp:1973 crash): Must check HasPendingWork() not just GetQueuedTasks()!
    // Problem: GetQueuedTasks() only checks if tasks are QUEUED. A task can be dequeued
    //          (queue empty) but STILL EXECUTING on a worker thread. If we only check queues,
    //          we can proceed while workers still access bot objects → use-after-free crash.
    // Solution: Use HasPendingWork() which checks both queued AND in-flight (executing) tasks.
    if (useThreadPool)
    {
        // Check for ANY pending work (queued OR currently executing)
        if (Performance::GetThreadPool().HasPendingWork())
        {
            size_t queuedTasks = Performance::GetThreadPool().GetQueuedTasks();
            size_t inFlightTasks = Performance::GetThreadPool().GetInFlightTasks();

            auto startWait = ::std::chrono::steady_clock::now();

            // CRITICAL FIX (FreezeDetector 60s crash): Use much shorter timeouts!
            // FreezeDetector triggers at 60s. We must leave plenty of buffer for:
            // - Other World::Update operations after UpdateSessions
            // - Any unexpected delays
            // Total wait should be MAX 10 seconds (leaving 50s buffer)
            constexpr auto INITIAL_WAIT = ::std::chrono::milliseconds(2000);
            constexpr auto EXTENDED_WAIT = ::std::chrono::milliseconds(8000);
            constexpr auto MAX_TOTAL_WAIT = ::std::chrono::milliseconds(10000);

            bool completed = Performance::GetThreadPool().WaitForCompletion(INITIAL_WAIT);

            auto waitDuration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
                ::std::chrono::steady_clock::now() - startWait);

            if (!completed && waitDuration < MAX_TOTAL_WAIT)
            {
                // Tasks still running - log and try extended wait
                TC_LOG_WARN("module.playerbot.session",
                    "ThreadPool tasks still running after {}ms ({} queued, {} in-flight) - trying extended wait",
                    waitDuration.count(), queuedTasks, inFlightTasks);

                // STUCK TASK DETECTION: Early warning - log slow tasks
                LogStuckTasks(1500);  // Log tasks executing for more than 1.5 seconds

                // Calculate remaining time for extended wait
                auto remaining = MAX_TOTAL_WAIT - waitDuration;
                auto extendedTimeout = ::std::min(EXTENDED_WAIT, remaining);

                completed = Performance::GetThreadPool().WaitForCompletion(extendedTimeout);

                waitDuration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
                    ::std::chrono::steady_clock::now() - startWait);
            }

            if (!completed)
            {
                // CRITICAL: Still not completed - proceed anyway to prevent FreezeDetector crash
                size_t finalQueued = Performance::GetThreadPool().GetQueuedTasks();
                size_t finalInFlight = Performance::GetThreadPool().GetInFlightTasks();
                size_t activeThreads = Performance::GetThreadPool().GetActiveThreads();

                TC_LOG_ERROR("module.playerbot.session",
                    "ThreadPool wait timeout after {}ms! {} queued, {} in-flight, {} active workers. "
                    "PROCEEDING to prevent FreezeDetector crash - some bot updates may be incomplete!",
                    waitDuration.count(), finalQueued, finalInFlight, activeThreads);

                // STUCK TASK DETECTION: Log which bots have been executing for too long
                LogStuckTasks(2000);  // Log tasks executing for more than 2 seconds

                canProcessLogouts = false;
            }
            else if (waitDuration.count() > 500)
            {
                // Log if wait was notably long (>500ms)
                TC_LOG_DEBUG("module.playerbot.session",
                    "ThreadPool wait took {}ms for {} tasks", waitDuration.count(), queuedTasks);
            }
        }
    }

    // ============================================================================
    // CRITICAL FIX: Process Pending Resurrections (Map.cpp:686 crash prevention)
    // ============================================================================
    // Worker threads set _pendingMainThreadResurrection flag when a ghost bot needs
    // resurrection. We execute the actual resurrection HERE on the main thread
    // AFTER ThreadPool workers have completed (safe to modify player state).
    //
    // This prevents the Map.cpp:686 ACCESS_VIOLATION crash caused by calling
    // SendSpiritResurrect() (which modifies player state) from a worker thread
    // while Map::Update is iterating over players.
    // ============================================================================
    if (canProcessLogouts)  // Reuse the same condition - workers are done
    {
        ::std::lock_guard lock(_sessionsMutex);
        for (auto& [guid, session] : _botSessions)
        {
            if (!session || !session->IsLoginComplete())
                continue;

            Player* bot = session->GetPlayer();
            if (!bot || !bot->IsInWorld())
                continue;

            // Get BotAI directly from BotSession (avoids dynamic_cast and registry issues)
            if (BotAI* ai = session->GetAI())
            {
                if (auto* deathMgr = ai->GetDeathRecoveryManager())
                {
                    if (deathMgr->HasPendingMainThreadResurrection())
                    {
                        TC_LOG_INFO("module.playerbot.session",
                            "Processing pending resurrection for bot {} [MAIN THREAD]",
                            bot->GetName());
                        deathMgr->ExecutePendingMainThreadResurrection();
                    }
                }
            }
        }
    }

    // OPTION 5: Process async disconnections from lock-free queue
    // Worker threads push to _asyncDisconnections, we pop here (no mutex needed)
    ObjectGuid disconnectedGuid;
    while (_asyncDisconnections.pop(disconnectedGuid))
    {
        disconnectedSessions.push_back(disconnectedGuid);
    }

    // ============================================================================
    // CRITICAL FIX: Two-Phase Deferred Logout (Cell::Visit crash prevention)
    // ============================================================================
    // Problem: When LogoutPlayer() is called during UpdateSessions(), it removes the Player
    //          from the Grid. But Map worker threads (ProcessRelocationNotifies) may already
    //          have iterators pointing to that Player, causing ACCESS_VIOLATION in Cell::Visit
    //          when they access player->m_seer (GridNotifiers.cpp:237).
    //
    // Root Cause: World::Update() execution order:
    //   Line 2346: sScriptMgr->OnBeforeWorldUpdate() - triggers module updates (bots)
    //   Line 2363: sMapMgr->Update() - triggers map worker threads (ProcessRelocationNotifies)
    //
    // Race condition: Bot cleanup (LogoutPlayer) runs BEFORE map update completes, but map
    //                 workers may already hold references to the Player being logged out.
    //
    // Solution: Two-phase deferred cleanup:
    //   Phase 1 (this tick): Collect disconnected GUIDs into _pendingLogouts (don't logout yet)
    //   Phase 2 (next tick): Move _pendingLogouts to _readyForLogout, then call LogoutPlayer()
    //   This ensures LogoutPlayer() is only called AFTER the map update cycle for the tick
    //   where the disconnection was detected has fully completed.
    // ============================================================================

    // PHASE 2 FIRST: Process _readyForLogout (collected from PREVIOUS tick - safe now!)
    // CRITICAL: Only process if canProcessLogouts is true (ThreadPool workers finished)
    if (canProcessLogouts && !_readyForLogout.empty())
    {
        ::std::lock_guard lock(_sessionsMutex);
        for (ObjectGuid const& guid : _readyForLogout)
        {
            auto it = _botSessions.find(guid);
            if (it != _botSessions.end())
            {
                ::std::shared_ptr<BotSession> session = it->second;
                if (session)
                {
                    if (session->GetPlayer() && session->GetPlayer()->IsInWorld())
                    {
                        try {
                            // ============================================================
                            // CRITICAL FIX: Exit vehicle BEFORE logout to prevent crash
                            // ============================================================
                            // Problem: If bot is a vehicle passenger with pending VehicleJoinEvent,
                            //          LogoutPlayer() destroys the bot but the event still has a
                            //          dangling Vehicle* pointer. When Map::MoveAllCreaturesInMoveList
                            //          processes the bot and calls EventProcessor::KillAllEvents,
                            //          VehicleJoinEvent::Abort crashes accessing the freed Vehicle.
                            //
                            // Solution: Force bot to exit any vehicle BEFORE logout.
                            //           This properly cleans up vehicle events and references.
                            // ============================================================
                            Player* bot = session->GetPlayer();
                            if (bot->GetVehicle())
                            {
                                TC_LOG_DEBUG("module.playerbot.session",
                                    "Bot {} exiting vehicle before logout (Vehicle crash prevention)",
                                    bot->GetGUID().GetCounter());
                                bot->ExitVehicle();
                            }

                            // CRITICAL FIX (Map.cpp:1942 crash): Remove from _updateObjects BEFORE logout
                            // This prevents Map::SendObjectUpdates from accessing the bot after destruction.
                            // ClearUpdateMask(true) calls RemoveFromObjectUpdate() which removes the
                            // player from Map::_updateObjects, ensuring MapUpdater worker threads
                            // won't access the destroyed player.
                            static_cast<Object*>(bot)->ClearUpdateMask(true);

                            TC_LOG_DEBUG("module.playerbot.session",
                                "Deferred logout for bot {} (Cell::Visit crash prevention)",
                                bot->GetGUID().GetCounter());
                            session->LogoutPlayer(true);
                        }
                        catch (...)
                        {
                            TC_LOG_ERROR("module.playerbot.session",
                                "Exception during deferred LogoutPlayer() for bot {} - continuing cleanup",
                                guid.ToString());
                        }
                    }
                }
                _botSessions.erase(it);
            }
            sBotPriorityMgr->RemoveBot(guid);
        }
        _readyForLogout.clear();
    }

    // PHASE 1a: Move _pendingLogouts to _readyForLogout (will be processed NEXT tick)
    if (!_pendingLogouts.empty())
    {
        _readyForLogout = ::std::move(_pendingLogouts);
        _pendingLogouts.clear();
        TC_LOG_TRACE("module.playerbot.session",
            "Moved {} pending logouts to ready queue (will process next tick)",
            _readyForLogout.size());
    }

    // PHASE 1b: Add newly detected disconnections to _pendingLogouts
    for (ObjectGuid const& guid : disconnectedSessions)
    {
        _pendingLogouts.push_back(guid);
    }

    if (!disconnectedSessions.empty())
    {
        TC_LOG_DEBUG("module.playerbot.session",
            "Queued {} bot disconnections for deferred logout (Cell::Visit crash prevention)",
            disconnectedSessions.size());
    }

    // PHASE 4: Lightweight enterprise monitoring (reduced frequency to minimize overhead)
    if (_enterpriseMode)
    {
        // OPTION 5: Get async update count and reset for next tick
        uint32 asyncUpdated = _asyncBotsUpdated.exchange(0, ::std::memory_order_relaxed);
        sBotPerformanceMon->EndTick(currentTime, asyncUpdated, botsSkipped);

        // Only check thresholds every 10 ticks (500ms) instead of every tick
    if (_tickCounter % 10 == 0)
        {
            sBotPerformanceMon->CheckPerformanceThresholds();
            sBotHealthCheck->PerformHealthChecks(currentTime);
        }

        // Periodic enterprise logging (every 60 seconds)
        if (_tickCounter % 1200 == 0)
        {
            sBotPriorityMgr->LogPriorityDistribution();
            sBotPerformanceMon->LogPerformanceReport();

            // MEMORY MANAGEMENT: Clean up inactive spatial grids and log memory stats
            // This prevents unbounded memory growth from grids created for maps that bots
            // have since left. Grids inactive for 5+ minutes with zero population are freed.
            size_t cleaned = sSpatialGridManager.CleanupInactiveGrids();
            if (cleaned > 0)
            {
                TC_LOG_INFO("module.playerbot.session",
                    "Memory cleanup: {} inactive spatial grids freed", cleaned);
            }
            sSpatialGridManager.LogMemoryStats();
        }
    }
}

uint32 BotWorldSessionMgr::ProcessAllDeferredPackets()
{
    // CRITICAL: This method MUST be called from the main world thread ONLY!
    // Purpose: Process packets queued by bot worker threads that require serialization with Map::Update()
    //
    // Thread Safety:
    // - This method runs on main thread
    // - BotSession::QueueDeferredPacket() runs on worker threads (thread-safe with mutex)
    // - Deferred packets execute on main thread, synchronized with Map::Update()
    //
    // Performance:
    // - Processes all deferred packets across all bot sessions
    // - Each session limited to 50 packets/update (MAX_DEFERRED_PACKETS_PER_UPDATE)
    // - Expected load: 300-400 packets/sec with 5000 bots
    // - Main thread capacity: 1000 packets/sec (2.5x safety margin)

    uint32 totalProcessed = 0;

    // Collect sessions that need main thread processing:
    // - Deferred packets (race condition sensitive operations)
    // - Pending facing requests (SetFacingToObject requires main thread)
    // - Pending stop movement (MotionMaster::Clear requires main thread)
    // - Pending safe resurrection (SpawnCorpseBones crash fix)
    ::std::vector<::std::shared_ptr<BotSession>> sessionsToProcess;
    {
        ::std::lock_guard lock(_sessionsMutex);
        sessionsToProcess.reserve(_botSessions.size());

        uint32 lfgPendingCount = 0;  // Track how many sessions have pending LFG accepts
        for (auto const& [guid, session] : _botSessions)
        {
            // CRITICAL FIX: Check ALL main-thread-only operations, not just deferred packets!
            // Safe resurrection uses atomic flag (_pendingSafeResurrection), not deferred queue
            // Pending loot uses mutex-protected queue (_pendingLootTargets)
            // Pending object use uses mutex-protected queue (_pendingObjectUseTargets)
            // LFG proposal accepts use mutex-protected queue (_pendingLfgProposalAccepts)
            bool hasLfgPending = session && session->HasPendingLfgProposalAccepts();
            if (hasLfgPending)
            {
                ++lfgPendingCount;
                TC_LOG_DEBUG("module.playerbot.lfg",
                    "ProcessAllDeferredPackets: Bot {} has pending LFG proposal accepts",
                    session->GetPlayerName());
            }

            if (session && (session->HasDeferredPackets() ||
                           session->HasPendingFacing() ||
                           session->HasPendingStopMovement() ||
                           session->HasPendingSafeResurrection() ||
                           session->HasPendingLoot() ||
                           session->HasPendingObjectUse() ||
                           hasLfgPending))
            {
                sessionsToProcess.push_back(session);
            }
        }

        // Log summary of LFG pending accepts
        if (lfgPendingCount > 0)
        {
            TC_LOG_INFO("module.playerbot.lfg",
                "ProcessAllDeferredPackets: Found {} bot sessions with pending LFG proposal accepts",
                lfgPendingCount);
        }
    } // Release mutex before processing

    // Process deferred packets and facing for each session
    for (auto const& session : sessionsToProcess)
    {
        if (!session)
            continue;

        uint32 processed = session->ProcessDeferredPackets();
        totalProcessed += processed;

        if (processed > 0)
        {
            TC_LOG_TRACE("playerbot.packets.deferred",
                "Bot {} processed {} deferred packets on main thread",
                session->GetPlayerName(), processed);
        }

        // Process pending facing request (thread-safe facing system)
        // SetFacingToObject() requires main thread - worker threads queue facing via QueueFacingTarget()
        session->ProcessPendingFacing();

        // Process pending stop movement request (thread-safe movement system)
        // MotionMaster::Clear() requires main thread - worker threads queue via QueueStopMovement()
        session->ProcessPendingStopMovement();

        // Process pending safe resurrection request (SpawnCorpseBones crash fix)
        // ResurrectPlayer() on main thread - bypasses HandleReclaimCorpse/SpawnCorpseBones crash
        session->ProcessPendingSafeResurrection();

        // Process pending loot requests (SendLoot crash fix)
        // SendLoot() modifies _updateObjects which requires main thread
        session->ProcessPendingLoot();

        // Process pending object use requests (GameObject::Use crash fix)
        // Use() modifies game object state and triggers Map updates - requires main thread
        session->ProcessPendingObjectUse();

        // Process pending LFG proposal accepts (re-entrant crash fix)
        // UpdateProposal() must not be called from within LFGMgr iteration
        session->ProcessPendingLfgProposalAccepts();
    }

    // Log statistics if significant activity
    if (totalProcessed > 0)
    {
        TC_LOG_DEBUG("playerbot.packets.deferred",
            "ProcessAllDeferredPackets: {} total packets processed from {} bot sessions",
            totalProcessed, sessionsToProcess.size());
    }

    return totalProcessed;
}

uint32 BotWorldSessionMgr::GetBotCount() const
{
    ::std::lock_guard lock(_sessionsMutex);
    return static_cast<uint32>(_botSessions.size());
}

void BotWorldSessionMgr::TriggerCharacterLoginForAllSessions()
{
    // For compatibility with existing BotSpawner system
    // This method will work with the new native login system
    TC_LOG_INFO("module.playerbot.session", "?? TriggerCharacterLoginForAllSessions called");

    // The native login approach doesn't need this trigger mechanism
    // But we can use it to spawn new bots if needed
    TC_LOG_INFO("module.playerbot.session", "?? Using native login - no manual triggering needed");
}

bool BotWorldSessionMgr::SynchronizeCharacterCache(ObjectGuid playerGuid)
{
    TC_LOG_DEBUG("module.playerbot.session", "?? Synchronizing character cache for {}", playerGuid.ToString());

    // CRITICAL FIX: Use synchronous prepared statement for character lookup
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_DATA_FOR_GUILD);
    stmt->setUInt64(0, playerGuid.GetCounter());
    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (!result)
    {
        TC_LOG_ERROR("module.playerbot.session", "?? Character {} not found in characters table", playerGuid.ToString());
        return false;
    }

    Field* fields = result->Fetch();
    ::std::string dbName = fields[0].GetString();       // name is first field
    uint32 dbAccountId = fields[6].GetUInt32();      // account is 7th field (0-indexed: field 6)
    // Get current cache data
    ::std::string cacheName = "<unknown>";
    uint32 cacheAccountId = sCharacterCache->GetCharacterAccountIdByGuid(playerGuid);
    sCharacterCache->GetCharacterNameByGuid(playerGuid, cacheName);

    TC_LOG_INFO("module.playerbot.session", "?? Cache sync: DB({}, {}) vs Cache({}, {}) for {}",
                dbName, dbAccountId, cacheName, cacheAccountId, playerGuid.ToString());

    // Update cache if different
    if (dbName != cacheName)
    {
        TC_LOG_INFO("module.playerbot.session", "?? Updating character name cache: '{}' -> '{}'", cacheName, dbName);
        sCharacterCache->UpdateCharacterData(playerGuid, dbName);
    }

    if (dbAccountId != cacheAccountId)
    {
        TC_LOG_INFO("module.playerbot.session", "?? Updating character account cache: {} -> {}", cacheAccountId, dbAccountId);
        sCharacterCache->UpdateCharacterAccountId(playerGuid, dbAccountId);
    }

    TC_LOG_DEBUG("module.playerbot.session", "?? Character cache synchronized for {}", playerGuid.ToString());
    return true;
}

// ============================================================================
// Chat Command Support APIs - Added for PlayerBot command system
// ============================================================================

::std::vector<Player*> BotWorldSessionMgr::GetPlayerBotsByAccount(uint32 accountId) const
{
    ::std::vector<Player*> bots;
    ::std::lock_guard lock(_sessionsMutex);

    for (auto const& [guid, session] : _botSessions)
    {
        if (!session)
            continue;

        Player* bot = session->GetPlayer();
        if (!bot)
            continue;

        // Check if this bot belongs to the specified account
    if (bot->GetSession() && bot->GetSession()->GetAccountId() == accountId)
            bots.push_back(bot);
    }

    return bots;
}

void BotWorldSessionMgr::RemoveAllPlayerBots(uint32 accountId)
{
    ::std::vector<ObjectGuid> botsToRemove;

    // Collect GUIDs to remove (avoid modifying map while iterating)
    {
        ::std::lock_guard lock(_sessionsMutex);
        for (auto const& [guid, session] : _botSessions)
        {
            if (!session)
                continue;

            Player* bot = session->GetPlayer();
            if (bot && bot->GetSession() && bot->GetSession()->GetAccountId() == accountId)
                botsToRemove.push_back(guid);
        }
    }

    // Remove bots (this releases the mutex between iterations)
    for (ObjectGuid const& guid : botsToRemove)
    {
        RemovePlayerBot(guid);
        TC_LOG_INFO("module.playerbot.commands", "Removed bot {} for account {}", guid.ToString(), accountId);
    }
}

uint32 BotWorldSessionMgr::GetBotCountByAccount(uint32 accountId) const
{
    uint32 count = 0;
    ::std::lock_guard lock(_sessionsMutex);

    for (auto const& [guid, session] : _botSessions)
    {
        if (!session)
            continue;

        Player* bot = session->GetPlayer();
        if (bot && bot->GetSession() && bot->GetSession()->GetAccountId() == accountId)
            ++count;
    }

    return count;
}

::std::vector<Player*> BotWorldSessionMgr::GetAllBotPlayers() const
{
    ::std::vector<Player*> bots;
    ::std::lock_guard lock(_sessionsMutex);

    bots.reserve(_botSessions.size());

    for (auto const& [guid, session] : _botSessions)
    {
        if (!session || !session->IsLoginComplete())
            continue;

        Player* bot = session->GetPlayer();
        if (!bot || !bot->IsInWorld())
            continue;

        bots.push_back(bot);
    }

    TC_LOG_DEBUG("module.playerbot.lfg", "GetAllBotPlayers - Returning {} bots from {} sessions",
                 bots.size(), _botSessions.size());

    return bots;
}

bool BotWorldSessionMgr::IsBotLoading(ObjectGuid botGuid) const
{
    ::std::lock_guard lock(_sessionsMutex);
    return _botsLoading.find(botGuid) != _botsLoading.end();
}

void BotWorldSessionMgr::MarkAsInstanceBot(ObjectGuid botGuid)
{
    ::std::lock_guard lock(_sessionsMutex);

    auto it = _botSessions.find(botGuid);
    if (it != _botSessions.end() && it->second)
    {
        it->second->SetInstanceBot(true);
        TC_LOG_DEBUG("module.playerbot.instance",
            "Marked bot {} as INSTANCE BOT - will auto-logout after 60s if idle",
            botGuid.ToString());
    }
    else
    {
        TC_LOG_DEBUG("module.playerbot.instance",
            "Could not mark bot {} as instance bot - session not found",
            botGuid.ToString());
    }
}

uint32 BotWorldSessionMgr::GetInstanceBotCount() const
{
    ::std::lock_guard lock(_sessionsMutex);

    uint32 count = 0;
    for (auto const& [guid, session] : _botSessions)
    {
        if (session && session->IsInstanceBot())
            ++count;
    }
    return count;
}

} // namespace Playerbot
