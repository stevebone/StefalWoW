/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "InstanceBotPool.h"
#include "InstanceBotOrchestrator.h"
#include "BotCloneEngine.h"
#include "BotTemplateRepository.h"
#include "BotPostLoginConfigurator.h"
#include "BotCharacterCreator.h"
#include "BotSpawner.h"
#include "Account/BotAccountMgr.h"
#include "Config/PlayerbotConfig.h"
#include "Session/BotWorldSessionMgr.h"
#include "PvP/BGBotManager.h"
#include "BattlegroundMgr.h"
#include "CharacterCache.h"
#include "DatabaseEnv.h"
#include "Database/PlayerbotDatabase.h"
#include "DB2Stores.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Timer.h"
#include "World.h"
#include <algorithm>
#include <random>
#include <sstream>
#include <thread>

namespace Playerbot
{

// ============================================================================
// SINGLETON
// ============================================================================

InstanceBotPool* InstanceBotPool::Instance()
{
    static InstanceBotPool instance;
    return &instance;
}

// ============================================================================
// LIFECYCLE
// ============================================================================

bool InstanceBotPool::Initialize()
{
    if (_initialized.load())
    {
        TC_LOG_WARN("playerbot.pool", "InstanceBotPool already initialized");
        return true;
    }

    TC_LOG_INFO("playerbot.pool", "Initializing Instance Bot Pool...");

    // Load configuration
    LoadConfig();

    if (!_config.enabled)
    {
        TC_LOG_INFO("playerbot.pool", "Instance Bot Pool is disabled in configuration");
        return true;
    }

    // Initialize timing
    _hourStart = std::chrono::system_clock::now();
    _dayStart = _hourStart;

    // Initialize statistics
    _stats.Reset();

    // Load from database if configured
    if (_config.behavior.persistToDatabase)
    {
        LoadFromDatabase();
    }

    _initialized.store(true);

    TC_LOG_INFO("playerbot.pool", "Instance Bot Pool initialized successfully");

    // NOTE: Pool warmup is DEFERRED until Update() runs
    // This is because during Initialize(), the async database worker threads may not
    // be fully operational yet. The BotCharacterCreator uses Player::Create() which
    // internally calls async-only prepared statements. Calling these synchronously
    // during server startup causes assertion crashes.
    //
    // By deferring warmup to the first Update() tick, we ensure:
    // 1. The world is fully loaded
    // 2. Async database threads are running
    // 3. The same code path as .bot spawn command (which works) is used
    //
    // Human players wait 1-2 minutes for queues anyway - we have time to warm up.
    if (_config.behavior.warmOnStartup)
    {
        _warmupPending.store(true);
        TC_LOG_INFO("playerbot.pool", "Pool warmup deferred until world is fully running");
    }

    return true;
}

void InstanceBotPool::Shutdown()
{
    if (!_initialized.load())
        return;

    TC_LOG_INFO("playerbot.pool", "Shutting down Instance Bot Pool...");

    _shuttingDown.store(true);

    // ========================================================================
    // WARM POOL PERSISTENCE (2026-01-16)
    //
    // CRITICAL CHANGE: Warm pool bots are NO LONGER deleted on shutdown!
    //
    // Old behavior: Delete ALL pool bot characters from database
    // New behavior: Persist warm pool bots to database for reuse on next startup
    //
    // - Warm Pool Bots: PERSIST in database, restored at next startup
    // - JIT Bots: Deleted on shutdown by JITBotFactory (separate system)
    //
    // This fixes the issue where 800 bots were being recreated on every restart.
    // ========================================================================

    // Save warm pool state to database for persistence across restarts
    if (_config.behavior.persistToDatabase)
    {
        SyncToDatabase();
        TC_LOG_INFO("playerbot.pool", "Warm pool bot state saved to database ({} bots)", _slots.size());
    }

    // DO NOT delete warm pool bot characters!
    // They persist in the database and will be loaded at next startup.
    // This prevents the 800-bot recreation on every server restart.
    //
    // Note: JIT bots are deleted separately by JITBotFactory::Shutdown()
    TC_LOG_INFO("playerbot.pool", "Warm pool bots preserved in database for next startup ({} bots)", _slots.size());

    // Clear all slots
    {
        std::unique_lock lock(_slotsMutex);
        _slots.clear();
    }

    // Clear reservations
    {
        std::lock_guard lock(_reservationMutex);
        _reservations.clear();
    }

    // Clear ready index (reset all vectors in the std::array structure)
    {
        std::unique_lock lock(_readyIndexMutex);
        for (auto& roleMap : _readyIndex)
            for (auto& factionMap : roleMap)
                for (auto& bracketVec : factionMap)
                    bracketVec.clear();
    }

    // Clear bracket counts
    {
        std::unique_lock lock(_bracketCountsMutex);
        _bracketCounts.Reset();
    }

    _initialized.store(false);
    _shuttingDown.store(false);

    TC_LOG_INFO("playerbot.pool", "Instance Bot Pool shutdown complete");
}

void InstanceBotPool::Update(uint32 diff)
{
    if (!_initialized.load() || !_config.enabled || _shuttingDown.load())
        return;

    // Deferred warmup - runs once after world is fully loaded
    // This ensures async database threads are operational before we create bots
    //
    // ========================================================================
    // WARM POOL RECONCILIATION (2026-01-16)
    //
    // Instead of always creating fresh bots, we now:
    // 1. Check how many bots were loaded from database (LoadFromDatabase)
    // 2. Calculate the shortage per bracket/faction/role
    // 3. Only create missing bots to reach target distribution
    //
    // This prevents the 800-bot recreation on every server restart.
    // ========================================================================
    if (_warmupPending.load())
    {
        _warmupPending.store(false);

        // Initialize configuration and calculate total target
        _config.poolSize.InitializeDefaultBracketPools();
        _warmupTotalTarget = _config.poolSize.GetTotalBotsAcrossAllBrackets();

        // Count how many bots we already have from LoadFromDatabase()
        uint32 existingBots = GetTotalPoolSize();

        if (existingBots >= _warmupTotalTarget)
        {
            // We have enough warm pool bots from database - no creation needed!
            TC_LOG_INFO("playerbot.pool", "Warm pool already at target capacity ({}/{} bots) - skipping creation",
                existingBots, _warmupTotalTarget);
            TC_LOG_INFO("playerbot.pool", "Warm pool bots loaded from database are ready for assignment");
            _statsDirty.store(true);
            return; // Skip warmup entirely
        }

        uint32 botsToCreate = _warmupTotalTarget - existingBots;

        TC_LOG_INFO("playerbot.pool", "Warm pool reconciliation: {} existing bots, {} target, creating {} new bots",
            existingBots, _warmupTotalTarget, botsToCreate);

        // Reset incremental warmup state
        _warmupBracketIndex = 0;
        _warmupFactionPhase = 0;
        _warmupRoleIndex = 0;
        _warmupRoleCount = 0;
        _warmupTotalCreated = 0;

        // Start incremental warmup (only creates missing bots)
        _warmingInProgress.store(true);
        _incrementalWarmupActive.store(true);

        TC_LOG_INFO("playerbot.pool", "Starting incremental warmup ({} bots/tick to prevent freeze detector)",
            WARMUP_BOTS_PER_TICK);
    }

    // Process incremental warmup - creates WARMUP_BOTS_PER_TICK bots per tick
    // This spreads the 800 bot creation over ~160 update ticks instead of blocking
    if (_incrementalWarmupActive.load())
    {
        ProcessIncrementalWarmup();
    }

    // Main update at configured interval
    _updateAccumulator += diff;
    if (_updateAccumulator >= _config.timing.updateIntervalMs)
    {
        _updateAccumulator = 0;

        // Process cooldown expirations
        ProcessCooldowns();

        // Process reservation timeouts
        ProcessReservations();

        // Retry warming bots that failed initial warmup (async DB commit delay)
        ProcessWarmingRetries();

        // Check hourly reset
        CheckHourlyReset();
    }

    // Statistics update at configured interval
    _statsAccumulator += diff;
    if (_statsAccumulator >= _config.timing.statsIntervalMs)
    {
        _statsAccumulator = 0;
        UpdateStatistics();
    }

    // Pool replenishment at configured interval
    if (_config.behavior.autoReplenish)
    {
        _replenishAccumulator += diff;
        if (_replenishAccumulator >= _config.timing.replenishIntervalMs)
        {
            _replenishAccumulator = 0;
            ReplenishPool();
        }
    }

    // Database sync at configured interval
    if (_config.behavior.persistToDatabase)
    {
        _dbSyncAccumulator += diff;
        if (_dbSyncAccumulator >= _config.timing.dbSyncIntervalMs)
        {
            _dbSyncAccumulator = 0;
            SyncToDatabase();
        }
    }
}

void InstanceBotPool::LoadConfig()
{
    TC_LOG_INFO("playerbot.pool", "Loading Instance Bot Pool configuration...");

    // Master enable
    _config.enabled = sPlayerbotConfig->GetBool("Playerbot.Instance.Pool.Enable", true);

    // Pool sizes - Alliance
    _config.poolSize.allianceTanks = sPlayerbotConfig->GetInt("Playerbot.Instance.Pool.Alliance.Tanks", 20);
    _config.poolSize.allianceHealers = sPlayerbotConfig->GetInt("Playerbot.Instance.Pool.Alliance.Healers", 30);
    _config.poolSize.allianceDPS = sPlayerbotConfig->GetInt("Playerbot.Instance.Pool.Alliance.DPS", 50);

    // Pool sizes - Horde
    _config.poolSize.hordeTanks = sPlayerbotConfig->GetInt("Playerbot.Instance.Pool.Horde.Tanks", 20);
    _config.poolSize.hordeHealers = sPlayerbotConfig->GetInt("Playerbot.Instance.Pool.Horde.Healers", 30);
    _config.poolSize.hordeDPS = sPlayerbotConfig->GetInt("Playerbot.Instance.Pool.Horde.DPS", 50);

    // Overflow settings
    _config.poolSize.maxOverflowBots = sPlayerbotConfig->GetInt("Playerbot.Instance.Pool.MaxOverflow", 500);
    _config.poolSize.overflowCreationRate = sPlayerbotConfig->GetInt("Playerbot.Instance.Pool.OverflowRate", 10);
    _config.poolSize.maxConcurrentCreations = sPlayerbotConfig->GetInt("Playerbot.Instance.Pool.MaxConcurrentCreations", 10);

    // Timing
    _config.timing.cooldownDuration = std::chrono::seconds(
        sPlayerbotConfig->GetInt("Playerbot.Instance.Pool.CooldownSeconds", 300));
    _config.timing.reservationTimeout = std::chrono::milliseconds(
        sPlayerbotConfig->GetInt("Playerbot.Instance.Pool.ReservationTimeoutMs", 60000));
    _config.timing.warmupTimeout = std::chrono::milliseconds(
        sPlayerbotConfig->GetInt("Playerbot.Instance.Pool.WarmupTimeoutMs", 30000));
    _config.timing.updateIntervalMs = sPlayerbotConfig->GetInt("Playerbot.Instance.Pool.UpdateIntervalMs", 1000);
    _config.timing.dbSyncIntervalMs = sPlayerbotConfig->GetInt("Playerbot.Instance.Pool.DbSyncIntervalMs", 60000);

    // Behavior
    _config.behavior.autoReplenish = sPlayerbotConfig->GetBool("Playerbot.Instance.Pool.AutoReplenish", true);
    _config.behavior.persistToDatabase = sPlayerbotConfig->GetBool("Playerbot.Instance.Pool.Persist", true);
    _config.behavior.warmOnStartup = sPlayerbotConfig->GetBool("Playerbot.Instance.Pool.WarmOnStartup", true);
    _config.behavior.enableJITFactory = sPlayerbotConfig->GetBool("Playerbot.Instance.JIT.Enable", true);
    _config.behavior.jitThresholdPct = sPlayerbotConfig->GetInt("Playerbot.Instance.Pool.JITThresholdPct", 20);

    // Logging
    _config.logging.logAssignments = sPlayerbotConfig->GetBool("Playerbot.Instance.Pool.LogAssignments", true);
    _config.logging.logPoolChanges = sPlayerbotConfig->GetBool("Playerbot.Instance.Pool.LogChanges", false);
    _config.logging.logReservations = sPlayerbotConfig->GetBool("Playerbot.Instance.Pool.LogReservations", true);

    TC_LOG_INFO("playerbot.pool",
        "Instance Bot Pool config: enabled={}, alliance={}/{}/{}, horde={}/{}/{}, cooldown={}s",
        _config.enabled,
        _config.poolSize.allianceTanks, _config.poolSize.allianceHealers, _config.poolSize.allianceDPS,
        _config.poolSize.hordeTanks, _config.poolSize.hordeHealers, _config.poolSize.hordeDPS,
        _config.timing.cooldownDuration.count());
}

// ============================================================================
// POOL MANAGEMENT
// ============================================================================

void InstanceBotPool::WarmPool()
{
    if (_warmingInProgress.load())
    {
        TC_LOG_DEBUG("playerbot.pool", "Pool warming already in progress");
        return;
    }

    _warmingInProgress.store(true);

    // ========================================================================
    // REFACTORED (2026-01-15): Per-bracket pool system
    // 8 level brackets × 2 factions × 50 bots = 800 total
    // Each bracket has: 10 tanks, 15 healers, 25 DPS per faction
    //
    // Pool bots are DATABASE RECORDS ONLY - NOT logged in until needed
    // We have 1-2 minutes of queue time to login bots when assigned
    // ========================================================================

    TC_LOG_INFO("playerbot.pool", "Creating per-bracket pool bot characters (database records only - NOT logged in)...");

    // Initialize bracket pools from configuration
    _config.poolSize.InitializeDefaultBracketPools();

    uint32 totalToCreate = _config.poolSize.GetTotalBotsAcrossAllBrackets();
    uint32 created = 0;

    // Create bots for each bracket
    for (uint8 bracketIdx = 0; bracketIdx < NUM_LEVEL_BRACKETS; ++bracketIdx)
    {
        PoolBracket bracket = static_cast<PoolBracket>(bracketIdx);
        BracketPoolConfig const& bracketConfig = _config.poolSize.bracketPools[bracketIdx];

        if (!bracketConfig.enabled)
        {
            TC_LOG_DEBUG("playerbot.pool", "Bracket {} is disabled, skipping", bracketIdx);
            continue;
        }

        uint32 minLevel, maxLevel;
        GetBracketLevelRange(bracket, minLevel, maxLevel);

        TC_LOG_INFO("playerbot.pool", "Creating bots for bracket {} (level {}-{}): A[T={}/H={}/D={}] H[T={}/H={}/D={}]",
            bracketIdx, minLevel, maxLevel,
            bracketConfig.alliance.tanks, bracketConfig.alliance.healers, bracketConfig.alliance.dps,
            bracketConfig.horde.tanks, bracketConfig.horde.healers, bracketConfig.horde.dps);

        // Create Alliance bots for this bracket
        for (uint32 i = 0; i < bracketConfig.alliance.tanks; ++i)
        {
            if (CreatePoolBot(BotRole::Tank, Faction::Alliance, bracket) != ObjectGuid::Empty)
                ++created;
        }
        for (uint32 i = 0; i < bracketConfig.alliance.healers; ++i)
        {
            if (CreatePoolBot(BotRole::Healer, Faction::Alliance, bracket) != ObjectGuid::Empty)
                ++created;
        }
        for (uint32 i = 0; i < bracketConfig.alliance.dps; ++i)
        {
            if (CreatePoolBot(BotRole::DPS, Faction::Alliance, bracket) != ObjectGuid::Empty)
                ++created;
        }

        // Create Horde bots for this bracket
        for (uint32 i = 0; i < bracketConfig.horde.tanks; ++i)
        {
            if (CreatePoolBot(BotRole::Tank, Faction::Horde, bracket) != ObjectGuid::Empty)
                ++created;
        }
        for (uint32 i = 0; i < bracketConfig.horde.healers; ++i)
        {
            if (CreatePoolBot(BotRole::Healer, Faction::Horde, bracket) != ObjectGuid::Empty)
                ++created;
        }
        for (uint32 i = 0; i < bracketConfig.horde.dps; ++i)
        {
            if (CreatePoolBot(BotRole::DPS, Faction::Horde, bracket) != ObjectGuid::Empty)
                ++created;
        }
    }

    // Rebuild ready index after mass creation
    RebuildReadyIndex();

    _warmingInProgress.store(false);

    TC_LOG_INFO("playerbot.pool", "Pool creation complete: {} of {} bot characters created (database records only)",
        created, totalToCreate);
    TC_LOG_INFO("playerbot.pool", "Pool bots are READY but NOT logged in - they will login via BotSpawner when needed");
    TC_LOG_INFO("playerbot.pool", "Per-bracket distribution: 8 brackets × 2 factions × 50 bots = 800 total");

    _statsDirty.store(true);
}

void InstanceBotPool::ProcessIncrementalWarmup()
{
    // ========================================================================
    // INCREMENTAL WARMUP WITH RECONCILIATION (2026-01-16)
    //
    // RECONCILIATION MODE: This method now checks existing bots before creating.
    // If bots were loaded from database, it skips creation for filled slots.
    //
    // Problem: Creating 800 bots synchronously blocks the world thread for 60+ seconds,
    // triggering the TrinityCore freeze detector which crashes the server.
    //
    // Solution: Create WARMUP_BOTS_PER_TICK bots (default 5) per Update() tick.
    // At ~100ms per update cycle and 5 bots/tick, we create 50 bots/second.
    //
    // State machine:
    // - _warmupBracketIndex: Current bracket (0-7)
    // - _warmupFactionPhase: 0=Alliance, 1=Horde
    // - _warmupRoleIndex: 0=Tank, 1=Healer, 2=DPS
    // - _warmupRoleCount: Bots processed for current role (may be skipped if already exist)
    // ========================================================================

    if (!_incrementalWarmupActive.load())
        return;

    // Track how many bots we create this tick
    uint32 botsThisTick = 0;

    // Log start of incremental warmup (first tick)
    static bool firstTickLogged = false;
    if (!firstTickLogged && _warmupTotalCreated == 0)
    {
        firstTickLogged = true;
        TC_LOG_INFO("playerbot.pool", "Starting incremental pool reconciliation ({} bots/tick to prevent freeze detector)...",
            WARMUP_BOTS_PER_TICK);
    }

    while (botsThisTick < WARMUP_BOTS_PER_TICK)
    {
        // Check if warmup is complete
        if (_warmupBracketIndex >= NUM_LEVEL_BRACKETS)
        {
            // Warmup complete - rebuild indices and finish
            RebuildReadyIndex();
            _warmingInProgress.store(false);
            _incrementalWarmupActive.store(false);
            firstTickLogged = false; // Reset for next time

            uint32 totalBots = GetTotalPoolSize();
            TC_LOG_INFO("playerbot.pool", "Incremental pool reconciliation complete: {} new bots created, {} total in pool",
                _warmupTotalCreated, totalBots);
            TC_LOG_INFO("playerbot.pool", "Pool bots are READY but NOT logged in - they will login via BotSpawner when needed");
            _statsDirty.store(true);
            return;
        }

        PoolBracket bracket = static_cast<PoolBracket>(_warmupBracketIndex);
        BracketPoolConfig const& bracketConfig = _config.poolSize.bracketPools[_warmupBracketIndex];

        // Skip disabled brackets
        if (!bracketConfig.enabled)
        {
            _warmupBracketIndex++;
            _warmupFactionPhase = 0;
            _warmupRoleIndex = 0;
            _warmupRoleCount = 0;
            continue;
        }

        // Determine current faction and target count for current role
        Faction faction = (_warmupFactionPhase == 0) ? Faction::Alliance : Faction::Horde;
        BracketRoleDistribution const& factionConfig = (_warmupFactionPhase == 0)
            ? bracketConfig.alliance
            : bracketConfig.horde;

        uint32 targetForRole = 0;
        BotRole role = BotRole::Tank;

        switch (_warmupRoleIndex)
        {
            case 0:  // Tank
                targetForRole = factionConfig.tanks;
                role = BotRole::Tank;
                break;
            case 1:  // Healer
                targetForRole = factionConfig.healers;
                role = BotRole::Healer;
                break;
            case 2:  // DPS
                targetForRole = factionConfig.dps;
                role = BotRole::DPS;
                break;
            default:
                // Move to next faction or bracket
                if (_warmupFactionPhase == 0)
                {
                    _warmupFactionPhase = 1;  // Switch to Horde
                    _warmupRoleIndex = 0;
                    _warmupRoleCount = 0;
                }
                else
                {
                    _warmupBracketIndex++;    // Next bracket
                    _warmupFactionPhase = 0;
                    _warmupRoleIndex = 0;
                    _warmupRoleCount = 0;
                }
                continue;
        }

        // RECONCILIATION: Check how many bots we ALREADY have for this bracket/faction/role
        uint32 existingCount = GetAvailableCountForBracket(bracket, faction, role);

        // Calculate how many more bots we need for this role
        uint32 botsNeeded = (existingCount < targetForRole) ? (targetForRole - existingCount) : 0;

        // Create bot for current role if more needed
        if (_warmupRoleCount < botsNeeded)
        {
            if (CreatePoolBot(role, faction, bracket) != ObjectGuid::Empty)
            {
                ++_warmupTotalCreated;
            }
            ++_warmupRoleCount;
            ++botsThisTick;
        }
        else
        {
            // This role is filled (either existing bots or newly created) - move to next role
            if (existingCount >= targetForRole && _warmupRoleCount == 0)
            {
                TC_LOG_DEBUG("playerbot.pool", "Bracket {} {} {} already has {}/{} bots - skipping",
                    PoolBracketToString(bracket), FactionToString(faction), BotRoleToString(role),
                    existingCount, targetForRole);
            }
            _warmupRoleIndex++;
            _warmupRoleCount = 0;
        }
    }

    // Log progress every 50 bots created (not just processed)
    if (_warmupTotalCreated > 0 && (_warmupTotalCreated % 50 == 0))
    {
        uint32 totalBots = GetTotalPoolSize();
        float pct = static_cast<float>(totalBots) / static_cast<float>(_warmupTotalTarget) * 100.0f;
        TC_LOG_INFO("playerbot.pool", "Reconciliation progress: {} new bots created, {}/{} total ({:.1f}%)",
            _warmupTotalCreated, totalBots, _warmupTotalTarget, pct);
    }
}

void InstanceBotPool::ReplenishPool()
{
    if (_warmingInProgress.load())
        return;

    // ========================================================================
    // REFACTORED (2026-01-15): Per-bracket replenishment
    // Check each bracket independently and request JIT bots for shortages
    // Uses BracketCounts for O(1) shortage detection
    // ========================================================================

    if (!_config.behavior.enableJITFactory || !_overflowNeededCallback)
        return;

    std::vector<PoolBracket> bracketsWithShortage = GetBracketsWithShortage();

    if (bracketsWithShortage.empty())
        return;

    if (_config.logging.logPoolChanges)
    {
        TC_LOG_INFO("playerbot.pool", "Pool replenishment needed - {} brackets have shortages",
            bracketsWithShortage.size());
    }

    // Request JIT bots for each bracket with shortage
    for (PoolBracket bracket : bracketsWithShortage)
    {
        PoolBracketStats stats = GetBracketStatistics(bracket);
        BracketPoolConfig const& config = _config.poolSize.bracketPools[static_cast<size_t>(bracket)];

        if (!config.enabled)
            continue;

        // Check Alliance shortages by role
        std::shared_lock lock(_bracketCountsMutex);
        uint32 allianceTanksReady = _bracketCounts.GetReadyByRole(bracket, Faction::Alliance, BotRole::Tank);
        uint32 allianceHealersReady = _bracketCounts.GetReadyByRole(bracket, Faction::Alliance, BotRole::Healer);
        uint32 allianceDPSReady = _bracketCounts.GetReadyByRole(bracket, Faction::Alliance, BotRole::DPS);

        uint32 hordeTanksReady = _bracketCounts.GetReadyByRole(bracket, Faction::Horde, BotRole::Tank);
        uint32 hordeHealersReady = _bracketCounts.GetReadyByRole(bracket, Faction::Horde, BotRole::Healer);
        uint32 hordeDPSReady = _bracketCounts.GetReadyByRole(bracket, Faction::Horde, BotRole::DPS);
        lock.unlock();

        // Request Alliance JIT bots
        if (allianceTanksReady < config.alliance.tanks)
            _overflowNeededCallback(BotRole::Tank, Faction::Alliance, bracket,
                config.alliance.tanks - allianceTanksReady);
        if (allianceHealersReady < config.alliance.healers)
            _overflowNeededCallback(BotRole::Healer, Faction::Alliance, bracket,
                config.alliance.healers - allianceHealersReady);
        if (allianceDPSReady < config.alliance.dps)
            _overflowNeededCallback(BotRole::DPS, Faction::Alliance, bracket,
                config.alliance.dps - allianceDPSReady);

        // Request Horde JIT bots
        if (hordeTanksReady < config.horde.tanks)
            _overflowNeededCallback(BotRole::Tank, Faction::Horde, bracket,
                config.horde.tanks - hordeTanksReady);
        if (hordeHealersReady < config.horde.healers)
            _overflowNeededCallback(BotRole::Healer, Faction::Horde, bracket,
                config.horde.healers - hordeHealersReady);
        if (hordeDPSReady < config.horde.dps)
            _overflowNeededCallback(BotRole::DPS, Faction::Horde, bracket,
                config.horde.dps - hordeDPSReady);

        if (_config.logging.logPoolChanges)
        {
            uint32 minLevel, maxLevel;
            GetBracketLevelRange(bracket, minLevel, maxLevel);
            TC_LOG_DEBUG("playerbot.pool", "Bracket {}-{} shortage: A[T={}/H={}/D={}] H[T={}/H={}/D={}]",
                minLevel, maxLevel,
                allianceTanksReady, allianceHealersReady, allianceDPSReady,
                hordeTanksReady, hordeHealersReady, hordeDPSReady);
        }
    }
}

uint32 InstanceBotPool::GetAvailableCount(BotRole role, PoolType poolType) const
{
    Faction faction = GetFactionForPoolType(poolType);
    return GetAvailableCount(role, faction);
}

uint32 InstanceBotPool::GetAvailableCount(BotRole role, Faction faction) const
{
    std::shared_lock lock(_slotsMutex);

    uint32 count = 0;
    for (auto const& [guid, slot] : _slots)
    {
        if (slot.state == PoolSlotState::Ready &&
            slot.role == role &&
            slot.faction == faction)
        {
            ++count;
        }
    }
    return count;
}

uint32 InstanceBotPool::GetAvailableCountForLevel(BotRole role, Faction faction,
                                                    uint32 level, uint32 range) const
{
    std::shared_lock lock(_slotsMutex);

    uint32 count = 0;
    for (auto const& [guid, slot] : _slots)
    {
        if (slot.state == PoolSlotState::Ready &&
            slot.role == role &&
            slot.faction == faction &&
            slot.IsInLevelRange(level, range))
        {
            ++count;
        }
    }
    return count;
}

uint32 InstanceBotPool::GetTotalPoolSize() const
{
    std::shared_lock lock(_slotsMutex);
    return static_cast<uint32>(_slots.size());
}

uint32 InstanceBotPool::GetReadyCount() const
{
    std::shared_lock lock(_slotsMutex);

    uint32 count = 0;
    for (auto const& [guid, slot] : _slots)
    {
        if (slot.state == PoolSlotState::Ready)
            ++count;
    }
    return count;
}

uint32 InstanceBotPool::GetAssignedCount() const
{
    std::shared_lock lock(_slotsMutex);

    uint32 count = 0;
    for (auto const& [guid, slot] : _slots)
    {
        if (slot.state == PoolSlotState::Assigned)
            ++count;
    }
    return count;
}

// ============================================================================
// BOT ASSIGNMENT - PvE
// ============================================================================

std::vector<ObjectGuid> InstanceBotPool::AssignForDungeon(
    uint32 dungeonId, uint32 playerLevel, Faction playerFaction,
    uint32 tanksNeeded, uint32 healersNeeded, uint32 dpsNeeded)
{
    auto startTime = std::chrono::steady_clock::now();

    std::vector<ObjectGuid> result;

    // Select tanks
    auto tanks = SelectBots(BotRole::Tank, playerFaction, playerLevel, tanksNeeded,
        _config.levelConfig.normalDungeonMinGS);
    result.insert(result.end(), tanks.begin(), tanks.end());

    // Select healers
    auto healers = SelectBots(BotRole::Healer, playerFaction, playerLevel, healersNeeded,
        _config.levelConfig.normalDungeonMinGS);
    result.insert(result.end(), healers.begin(), healers.end());

    // Select DPS
    auto dps = SelectBots(BotRole::DPS, playerFaction, playerLevel, dpsNeeded,
        _config.levelConfig.normalDungeonMinGS);
    result.insert(result.end(), dps.begin(), dps.end());

    // Check if we got enough
    bool success = (tanks.size() == tanksNeeded &&
                   healers.size() == healersNeeded &&
                   dps.size() == dpsNeeded);

    if (!success)
    {
        // Release any partially selected bots
        ReleaseBots(result);
        result.clear();

        if (_config.logging.logAssignments)
        {
            TC_LOG_WARN("playerbot.pool",
                "Failed to assign bots for dungeon {}: needed T={}/H={}/D={}, got T={}/H={}/D={}",
                dungeonId, tanksNeeded, healersNeeded, dpsNeeded,
                tanks.size(), healers.size(), dps.size());
        }

        if (_assignmentFailedCallback)
        {
            _assignmentFailedCallback(InstanceType::Dungeon, dungeonId,
                "Insufficient bots available");
        }

        // Update failure stats
        std::unique_lock statsLock(_statsMutex);
        ++_stats.activity.failedRequestsThisHour;
        return result;
    }

    // Assign all selected bots
    for (ObjectGuid guid : result)
    {
        AssignBot(guid, 0, dungeonId, InstanceType::Dungeon, playerLevel);
    }

    // Record timing
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

    {
        std::unique_lock statsLock(_statsMutex);
        _stats.timing.RecordAssignment(duration);
        ++_stats.activity.dungeonsFilledThisHour;
        ++_stats.activity.successfulRequestsThisHour;
        _stats.activity.assignmentsThisHour += static_cast<uint32>(result.size());
    }

    if (_config.logging.logAssignments)
    {
        TC_LOG_INFO("playerbot.pool",
            "Assigned {} bots for dungeon {} in {}µs",
            result.size(), dungeonId, duration.count());
    }

    _statsDirty.store(true);
    return result;
}

std::vector<ObjectGuid> InstanceBotPool::AssignForRaid(
    uint32 raidId, uint32 playerLevel, Faction playerFaction,
    uint32 tanksNeeded, uint32 healersNeeded, uint32 dpsNeeded)
{
    auto startTime = std::chrono::steady_clock::now();

    std::vector<ObjectGuid> result;

    // Select tanks
    auto tanks = SelectBots(BotRole::Tank, playerFaction, playerLevel, tanksNeeded,
        _config.levelConfig.normalRaidMinGS);
    result.insert(result.end(), tanks.begin(), tanks.end());

    // Select healers
    auto healers = SelectBots(BotRole::Healer, playerFaction, playerLevel, healersNeeded,
        _config.levelConfig.normalRaidMinGS);
    result.insert(result.end(), healers.begin(), healers.end());

    // Select DPS
    auto dps = SelectBots(BotRole::DPS, playerFaction, playerLevel, dpsNeeded,
        _config.levelConfig.normalRaidMinGS);
    result.insert(result.end(), dps.begin(), dps.end());

    // Check if we got enough (allow partial for raids - progressive filling)
    uint32 totalNeeded = tanksNeeded + healersNeeded + dpsNeeded;
    uint32 totalGot = static_cast<uint32>(result.size());
    float fillPct = totalNeeded > 0 ? (static_cast<float>(totalGot) / totalNeeded) * 100.0f : 100.0f;

    // Assign all selected bots
    for (ObjectGuid guid : result)
    {
        AssignBot(guid, 0, raidId, InstanceType::Raid, playerLevel);
    }

    // Record timing
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

    {
        std::unique_lock statsLock(_statsMutex);
        _stats.timing.RecordAssignment(duration);
        ++_stats.activity.raidsFilledThisHour;
        if (fillPct >= 100.0f)
            ++_stats.activity.successfulRequestsThisHour;
        else
            ++_stats.activity.failedRequestsThisHour;
        _stats.activity.assignmentsThisHour += static_cast<uint32>(result.size());
    }

    if (_config.logging.logAssignments)
    {
        TC_LOG_INFO("playerbot.pool",
            "Assigned {} of {} bots for raid {} ({:.1f}% fill) in {}µs",
            result.size(), totalNeeded, raidId, fillPct, duration.count());
    }

    _statsDirty.store(true);
    return result;
}

// ============================================================================
// BOT ASSIGNMENT - PvP
// ============================================================================

BGAssignment InstanceBotPool::AssignForBattleground(
    uint32 bgTypeId, uint32 bracketLevel,
    uint32 allianceNeeded, uint32 hordeNeeded,
    ObjectGuid humanPlayerGuid)
{
    auto startTime = std::chrono::steady_clock::now();

    BGAssignment result;

    // ========================================================================
    // REFACTORED (2026-01-15): Per-bracket selection with O(1) lookup
    // Uses ReadyIndex[role][faction][bracket] for fast bot retrieval
    // Role distribution: 15% tanks, 25% healers, 60% DPS
    // ========================================================================

    PoolBracket bracket = GetBracketForLevel(bracketLevel);

    // Calculate role distribution for Alliance
    uint32 allianceTanks = static_cast<uint32>(allianceNeeded * 0.15f);
    uint32 allianceHealers = static_cast<uint32>(allianceNeeded * 0.25f);
    uint32 allianceDPS = allianceNeeded - allianceTanks - allianceHealers;

    // Select Alliance bots by role using bracket-aware selection
    auto allianceTankBots = SelectBotsFromBracket(BotRole::Tank, Faction::Alliance, bracket, allianceTanks);
    result.allianceBots.insert(result.allianceBots.end(), allianceTankBots.begin(), allianceTankBots.end());

    auto allianceHealerBots = SelectBotsFromBracket(BotRole::Healer, Faction::Alliance, bracket, allianceHealers);
    result.allianceBots.insert(result.allianceBots.end(), allianceHealerBots.begin(), allianceHealerBots.end());

    auto allianceDPSBots = SelectBotsFromBracket(BotRole::DPS, Faction::Alliance, bracket, allianceDPS);
    result.allianceBots.insert(result.allianceBots.end(), allianceDPSBots.begin(), allianceDPSBots.end());

    // If not enough bots, try to fill from other roles
    uint32 allianceStillNeeded = allianceNeeded - static_cast<uint32>(result.allianceBots.size());
    if (allianceStillNeeded > 0)
    {
        for (uint8 r = 0; r < static_cast<uint8>(BotRole::Max) && allianceStillNeeded > 0; ++r)
        {
            auto extraBots = SelectBotsFromBracket(static_cast<BotRole>(r), Faction::Alliance, bracket, allianceStillNeeded);
            result.allianceBots.insert(result.allianceBots.end(), extraBots.begin(), extraBots.end());
            allianceStillNeeded -= static_cast<uint32>(extraBots.size());
        }
    }

    // Calculate role distribution for Horde
    uint32 hordeTanks = static_cast<uint32>(hordeNeeded * 0.15f);
    uint32 hordeHealers = static_cast<uint32>(hordeNeeded * 0.25f);
    uint32 hordeDPS = hordeNeeded - hordeTanks - hordeHealers;

    // Select Horde bots by role using bracket-aware selection
    auto hordeTankBots = SelectBotsFromBracket(BotRole::Tank, Faction::Horde, bracket, hordeTanks);
    result.hordeBots.insert(result.hordeBots.end(), hordeTankBots.begin(), hordeTankBots.end());

    auto hordeHealerBots = SelectBotsFromBracket(BotRole::Healer, Faction::Horde, bracket, hordeHealers);
    result.hordeBots.insert(result.hordeBots.end(), hordeHealerBots.begin(), hordeHealerBots.end());

    auto hordeDPSBots = SelectBotsFromBracket(BotRole::DPS, Faction::Horde, bracket, hordeDPS);
    result.hordeBots.insert(result.hordeBots.end(), hordeDPSBots.begin(), hordeDPSBots.end());

    // If not enough bots, try to fill from other roles
    uint32 hordeStillNeeded = hordeNeeded - static_cast<uint32>(result.hordeBots.size());
    if (hordeStillNeeded > 0)
    {
        for (uint8 r = 0; r < static_cast<uint8>(BotRole::Max) && hordeStillNeeded > 0; ++r)
        {
            auto extraBots = SelectBotsFromBracket(static_cast<BotRole>(r), Faction::Horde, bracket, hordeStillNeeded);
            result.hordeBots.insert(result.hordeBots.end(), extraBots.begin(), extraBots.end());
            hordeStillNeeded -= static_cast<uint32>(extraBots.size());
        }
    }

    // Check success
    result.success = (result.allianceBots.size() >= allianceNeeded &&
                     result.hordeBots.size() >= hordeNeeded);

    // Request JIT if insufficient bots
    if (!result.success && _overflowNeededCallback)
    {
        if (result.allianceBots.size() < allianceNeeded)
        {
            uint32 shortage = allianceNeeded - static_cast<uint32>(result.allianceBots.size());
            _overflowNeededCallback(BotRole::DPS, Faction::Alliance, bracket, shortage);
        }
        if (result.hordeBots.size() < hordeNeeded)
        {
            uint32 shortage = hordeNeeded - static_cast<uint32>(result.hordeBots.size());
            _overflowNeededCallback(BotRole::DPS, Faction::Horde, bracket, shortage);
        }
    }

    // DIAGNOSTIC: Log selection results before assignment
    TC_LOG_INFO("playerbot.pool", "AssignForBattleground: Selected Alliance={} Horde={} (requested A={} H={}) from bracket {}",
        result.allianceBots.size(), result.hordeBots.size(), allianceNeeded, hordeNeeded, static_cast<uint32>(bracket));

    // Assign all selected bots - this triggers async login via WarmUpBot
    // NOTE: Bots will NOT be immediately online after this! They need 1-2 seconds to log in.
    // CRITICAL: Pass humanPlayerGuid so bots use QueueBotForBGWithTracking for proper invitation handling
    for (ObjectGuid guid : result.allianceBots)
    {
        bool assigned = AssignBot(guid, 0, bgTypeId, InstanceType::Battleground, bracketLevel, humanPlayerGuid);
        TC_LOG_DEBUG("playerbot.pool", "AssignForBattleground: Alliance bot {} assign result: {} (async login started, tracking human {})",
            guid.ToString(), assigned, humanPlayerGuid.ToString());
    }
    for (ObjectGuid guid : result.hordeBots)
    {
        bool assigned = AssignBot(guid, 0, bgTypeId, InstanceType::Battleground, bracketLevel, humanPlayerGuid);
        TC_LOG_DEBUG("playerbot.pool", "AssignForBattleground: Horde bot {} assign result: {} (async login started, tracking human {})",
            guid.ToString(), assigned, humanPlayerGuid.ToString());
    }

    // Record timing
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

    {
        std::unique_lock statsLock(_statsMutex);
        _stats.timing.RecordAssignment(duration);
        ++_stats.activity.battlegroundsFilledThisHour;
        if (result.success)
            ++_stats.activity.successfulRequestsThisHour;
        else
            ++_stats.activity.failedRequestsThisHour;
        _stats.activity.assignmentsThisHour +=
            static_cast<uint32>(result.allianceBots.size() + result.hordeBots.size());
    }

    if (_config.logging.logAssignments)
    {
        uint32 minLevel, maxLevel;
        GetBracketLevelRange(bracket, minLevel, maxLevel);
        TC_LOG_INFO("playerbot.pool",
            "BG {} bracket {}-{} assignment: Alliance={}/{}, Horde={}/{}, success={}, {}µs",
            bgTypeId, minLevel, maxLevel, result.allianceBots.size(), allianceNeeded,
            result.hordeBots.size(), hordeNeeded, result.success, duration.count());
    }

    _statsDirty.store(true);
    return result;
}

ArenaAssignment InstanceBotPool::AssignForArena(
    uint32 arenaType, uint32 bracketLevel, Faction playerFaction,
    uint32 teammatesNeeded, uint32 opponentsNeeded)
{
    auto startTime = std::chrono::steady_clock::now();

    ArenaAssignment result;
    Faction opponentFaction = (playerFaction == Faction::Alliance) ? Faction::Horde : Faction::Alliance;

    // Select teammates (same faction as player)
    auto teammates = SelectBots(BotRole::DPS, playerFaction, bracketLevel, teammatesNeeded, 0);
    result.teammates = teammates;

    // Select opponents (opposite faction)
    auto opponents = SelectBots(BotRole::DPS, opponentFaction, bracketLevel, opponentsNeeded, 0);
    result.opponents = opponents;

    result.success = (teammates.size() == teammatesNeeded &&
                     opponents.size() == opponentsNeeded);

    // Assign all selected bots
    for (ObjectGuid guid : result.teammates)
        AssignBot(guid, 0, arenaType, InstanceType::Arena, bracketLevel);
    for (ObjectGuid guid : result.opponents)
        AssignBot(guid, 0, arenaType, InstanceType::Arena, bracketLevel);

    // Record timing
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

    {
        std::unique_lock statsLock(_statsMutex);
        _stats.timing.RecordAssignment(duration);
        ++_stats.activity.arenasFilledThisHour;
        if (result.success)
            ++_stats.activity.successfulRequestsThisHour;
        else
            ++_stats.activity.failedRequestsThisHour;
    }

    if (_config.logging.logAssignments)
    {
        TC_LOG_INFO("playerbot.pool",
            "Arena {} assignment: teammates={}/{}, opponents={}/{}, success={}, {}µs",
            arenaType, teammates.size(), teammatesNeeded,
            opponents.size(), opponentsNeeded, result.success, duration.count());
    }

    _statsDirty.store(true);
    return result;
}

// ============================================================================
// BOT RELEASE
// ============================================================================

void InstanceBotPool::ReleaseBots(std::vector<ObjectGuid> const& bots)
{
    for (ObjectGuid guid : bots)
        ReleaseBot(guid, true);
}

void InstanceBotPool::ReleaseBot(ObjectGuid botGuid, bool success)
{
    BotRole role;
    Faction faction;
    uint32 level;
    PoolSlotState newState;

    {
        std::unique_lock lock(_slotsMutex);

        auto it = _slots.find(botGuid);
        if (it == _slots.end())
            return;

        if (it->second.state != PoolSlotState::Assigned)
            return;

        // CRITICAL FIX: Capture current player state before release
        // This ensures level-ups and gear changes during the instance are preserved in pool metadata
        Player* player = ObjectAccessor::FindPlayer(botGuid);
        if (player)
        {
            it->second.level = player->GetLevel();
            it->second.gearScore = static_cast<uint32>(std::round(player->GetAverageItemLevel()));
            TC_LOG_DEBUG("playerbot.pool", "ReleaseBot: Updated bot {} metadata - Level={}, GS={}",
                botGuid.ToString(), it->second.level, it->second.gearScore);
        }

        // Store slot info before state change
        role = it->second.role;
        faction = it->second.faction;
        level = it->second.level;

        it->second.ReleaseFromInstance(success);
        newState = it->second.state;
    }

    // If bot transitioned to Ready (or Cooldown that will become Ready),
    // add back to ready index
    if (newState == PoolSlotState::Ready)
    {
        PoolBracket bracket = GetBracketForLevel(level);
        AddToReadyIndex(botGuid, role, faction, bracket);

        // Update bracket counts
        {
            std::unique_lock bracketLock(_bracketCountsMutex);
            _bracketCounts.IncrementReady(bracket, faction, role);
        }
    }

    ++_stats.activity.releasesThisHour;
    ++_stats.activity.releasesToday;

    if (_config.logging.logAssignments)
    {
        TC_LOG_DEBUG("playerbot.pool", "Released bot {} from instance (success={}, newState={})",
            botGuid.ToString(), success, static_cast<int>(newState));
    }

    _statsDirty.store(true);
}

void InstanceBotPool::ReleaseBotsFromInstance(uint32 instanceId)
{
    std::unique_lock lock(_slotsMutex);

    for (auto& [guid, slot] : _slots)
    {
        if (slot.state == PoolSlotState::Assigned &&
            slot.currentInstanceId == instanceId)
        {
            slot.ReleaseFromInstance(true);
            ++_stats.activity.releasesThisHour;
        }
    }

    _statsDirty.store(true);
}

// ============================================================================
// RESERVATION SYSTEM
// ============================================================================

uint32 InstanceBotPool::CreateReservation(
    InstanceType type, uint32 contentId, uint32 playerLevel, Faction playerFaction,
    uint32 tanksNeeded, uint32 healersNeeded, uint32 dpsNeeded,
    uint32 allianceNeeded, uint32 hordeNeeded)
{
    uint32 reservationId = _nextReservationId.fetch_add(1);

    Reservation reservation;
    reservation.reservationId = reservationId;
    reservation.instanceType = type;
    reservation.contentId = contentId;
    reservation.playerLevel = playerLevel;
    reservation.playerFaction = playerFaction;
    reservation.tanksNeeded = tanksNeeded;
    reservation.healersNeeded = healersNeeded;
    reservation.dpsNeeded = dpsNeeded;
    reservation.allianceNeeded = allianceNeeded;
    reservation.hordeNeeded = hordeNeeded;
    reservation.createdAt = std::chrono::steady_clock::now();
    reservation.deadline = reservation.createdAt + _config.timing.reservationTimeout;

    // Reserve bots
    if (RequiresBothFactions(type))
    {
        // PvP - reserve from both factions
        auto allianceBots = SelectBots(BotRole::DPS, Faction::Alliance, playerLevel, allianceNeeded, 0);
        auto hordeBots = SelectBots(BotRole::DPS, Faction::Horde, playerLevel, hordeNeeded, 0);

        std::unique_lock lock(_slotsMutex);
        for (ObjectGuid guid : allianceBots)
        {
            auto it = _slots.find(guid);
            if (it != _slots.end() && it->second.Reserve(reservationId))
            {
                reservation.reservedBots.push_back(guid);
                ++reservation.allianceFulfilled;
            }
        }
        for (ObjectGuid guid : hordeBots)
        {
            auto it = _slots.find(guid);
            if (it != _slots.end() && it->second.Reserve(reservationId))
            {
                reservation.reservedBots.push_back(guid);
                ++reservation.hordeFulfilled;
            }
        }
    }
    else
    {
        // PvE - reserve by role
        auto tanks = SelectBots(BotRole::Tank, playerFaction, playerLevel, tanksNeeded, 0);
        auto healers = SelectBots(BotRole::Healer, playerFaction, playerLevel, healersNeeded, 0);
        auto dps = SelectBots(BotRole::DPS, playerFaction, playerLevel, dpsNeeded, 0);

        std::unique_lock lock(_slotsMutex);
        for (ObjectGuid guid : tanks)
        {
            auto it = _slots.find(guid);
            if (it != _slots.end() && it->second.Reserve(reservationId))
            {
                reservation.reservedBots.push_back(guid);
                ++reservation.tanksFulfilled;
            }
        }
        for (ObjectGuid guid : healers)
        {
            auto it = _slots.find(guid);
            if (it != _slots.end() && it->second.Reserve(reservationId))
            {
                reservation.reservedBots.push_back(guid);
                ++reservation.healersFulfilled;
            }
        }
        for (ObjectGuid guid : dps)
        {
            auto it = _slots.find(guid);
            if (it != _slots.end() && it->second.Reserve(reservationId))
            {
                reservation.reservedBots.push_back(guid);
                ++reservation.dpsFulfilled;
            }
        }
    }

    // Store reservation
    {
        std::lock_guard lock(_reservationMutex);
        _reservations[reservationId] = std::move(reservation);
    }

    ++_stats.activity.reservationsThisHour;

    if (_config.logging.logReservations)
    {
        TC_LOG_INFO("playerbot.pool",
            "Created reservation {} for {} content {}: {:.1f}% fulfilled",
            reservationId, InstanceTypeToString(type), contentId,
            GetReservationFulfillment(reservationId));
    }

    _statsDirty.store(true);
    return reservationId;
}

bool InstanceBotPool::CanFulfillReservation(uint32 reservationId) const
{
    std::lock_guard lock(_reservationMutex);

    auto it = _reservations.find(reservationId);
    if (it == _reservations.end())
        return false;

    return it->second.IsFulfilled();
}

float InstanceBotPool::GetReservationFulfillment(uint32 reservationId) const
{
    std::lock_guard lock(_reservationMutex);

    auto it = _reservations.find(reservationId);
    if (it == _reservations.end())
        return 0.0f;

    return it->second.GetFulfillmentPct();
}

std::vector<ObjectGuid> InstanceBotPool::FulfillReservation(uint32 reservationId)
{
    std::vector<ObjectGuid> result;

    std::lock_guard resLock(_reservationMutex);

    auto it = _reservations.find(reservationId);
    if (it == _reservations.end())
        return result;

    Reservation& reservation = it->second;

    // Transition all reserved bots to assigned
    std::unique_lock slotLock(_slotsMutex);
    for (ObjectGuid guid : reservation.reservedBots)
    {
        auto slotIt = _slots.find(guid);
        if (slotIt != _slots.end())
        {
            if (slotIt->second.FulfillReservation(0, reservation.contentId, reservation.instanceType))
            {
                result.push_back(guid);
            }
        }
    }
    slotLock.unlock();

    // Remove reservation
    _reservations.erase(it);

    if (_config.logging.logReservations)
    {
        TC_LOG_INFO("playerbot.pool", "Fulfilled reservation {}: {} bots assigned",
            reservationId, result.size());
    }

    _statsDirty.store(true);
    return result;
}

void InstanceBotPool::CancelReservation(uint32 reservationId)
{
    std::lock_guard resLock(_reservationMutex);

    auto it = _reservations.find(reservationId);
    if (it == _reservations.end())
        return;

    // Return reserved bots to ready state
    std::unique_lock slotLock(_slotsMutex);
    for (ObjectGuid guid : it->second.reservedBots)
    {
        auto slotIt = _slots.find(guid);
        if (slotIt != _slots.end())
        {
            slotIt->second.CancelReservation();
        }
    }
    slotLock.unlock();

    _reservations.erase(it);
    ++_stats.activity.cancellationsThisHour;

    if (_config.logging.logReservations)
    {
        TC_LOG_INFO("playerbot.pool", "Cancelled reservation {}", reservationId);
    }

    _statsDirty.store(true);
}

std::chrono::milliseconds InstanceBotPool::GetEstimatedWaitTime(
    InstanceType type, uint32 /*contentId*/,
    uint32 tanksNeeded, uint32 healersNeeded, uint32 dpsNeeded) const
{
    // Check current availability
    uint32 availableTanks = GetReadyCount(); // Simplified - would need role breakdown
    uint32 totalNeeded = tanksNeeded + healersNeeded + dpsNeeded;

    if (GetReadyCount() >= totalNeeded)
        return std::chrono::milliseconds(0); // Instant

    // Estimate based on JIT factory speed
    uint32 deficit = totalNeeded - GetReadyCount();
    uint32 creationRate = _config.poolSize.overflowCreationRate;
    if (creationRate == 0)
        creationRate = 1;

    uint32 estimatedSeconds = deficit / creationRate;

    return std::chrono::seconds(estimatedSeconds);
}

// ============================================================================
// QUERIES
// ============================================================================

InstanceBotSlot const* InstanceBotPool::GetSlot(ObjectGuid botGuid) const
{
    std::shared_lock lock(_slotsMutex);

    auto it = _slots.find(botGuid);
    if (it == _slots.end())
        return nullptr;

    return &it->second;
}

bool InstanceBotPool::IsPoolBot(ObjectGuid botGuid) const
{
    std::shared_lock lock(_slotsMutex);
    return _slots.find(botGuid) != _slots.end();
}

uint32 InstanceBotPool::GetBotInstanceId(ObjectGuid botGuid) const
{
    std::shared_lock lock(_slotsMutex);

    auto it = _slots.find(botGuid);
    if (it == _slots.end())
        return 0;

    return it->second.currentInstanceId;
}

bool InstanceBotPool::CanProvideBotsFor(
    InstanceType /*type*/,
    uint32 tanksNeeded, uint32 healersNeeded, uint32 dpsNeeded) const
{
    // Simple check - could be more sophisticated with role breakdown
    uint32 totalNeeded = tanksNeeded + healersNeeded + dpsNeeded;
    return GetReadyCount() >= totalNeeded;
}

// ============================================================================
// STATISTICS
// ============================================================================

PoolStatistics InstanceBotPool::GetStatistics() const
{
    std::shared_lock lock(_statsMutex);
    return _stats;
}

void InstanceBotPool::PrintStatusReport() const
{
    PoolStatistics stats = GetStatistics();

    TC_LOG_INFO("playerbot.pool", "=== Instance Bot Pool Status ===");
    TC_LOG_INFO("playerbot.pool", "Total: {} | Ready: {} | Assigned: {} | Cooldown: {}",
        stats.slotStats.GetTotal(), stats.slotStats.readySlots,
        stats.slotStats.assignedSlots, stats.slotStats.cooldownSlots);
    TC_LOG_INFO("playerbot.pool", "Utilization: {:.1f}% | Availability: {:.1f}%",
        stats.GetUtilization(), stats.GetAvailability());
    TC_LOG_INFO("playerbot.pool", "Hourly: {} assignments, {} releases, {} failures",
        stats.activity.assignmentsThisHour, stats.activity.releasesThisHour,
        stats.activity.failedRequestsThisHour);
    TC_LOG_INFO("playerbot.pool", "Avg assignment time: {}µs",
        stats.timing.avgAssignmentTime.count());
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void InstanceBotPool::SetConfig(InstanceBotPoolConfig const& config)
{
    _config = config;
}

void InstanceBotPool::SetAssignmentFailedCallback(AssignmentFailedCallback callback)
{
    _assignmentFailedCallback = std::move(callback);
}

void InstanceBotPool::SetOverflowNeededCallback(OverflowNeededCallback callback)
{
    _overflowNeededCallback = std::move(callback);
}

// ============================================================================
// INTERNAL METHODS - Bot Creation
// ============================================================================

ObjectGuid InstanceBotPool::CreatePoolBot(BotRole role, Faction faction, PoolBracket bracket, bool /*deferWarmup*/)
{
    // ========================================================================
    // REFACTORED (2026-01-15): Per-bracket pool bot creation
    // Pool bots are DATABASE RECORDS ONLY - NOT logged in until needed
    // Bot level is set to bracket midpoint (e.g., bracket 20-29 = level 25)
    //
    // Flow:
    // 1. CreatePoolBot: Create character in database, store in _slots as Ready
    // 2. AssignFor*: When needed, login via BotSpawner (we have 1-2 min queue time)
    // 3. ReleaseBot: Log out and return to Ready pool
    // ========================================================================

    // Get level for this bracket (midpoint)
    uint32 level = GetBracketMidpointLevel(bracket);

    // Step 1: Get template for class/spec info using SelectRandomTemplate
    BotTemplate const* tmpl = sBotTemplateRepository->SelectRandomTemplate(role, faction);
    if (!tmpl || !tmpl->IsValid())
    {
        TC_LOG_WARN("playerbot.pool", "InstanceBotPool::CreatePoolBot - No valid template for role {} faction {}",
            BotRoleToString(role), FactionToString(faction));
        return ObjectGuid::Empty;
    }

    // Step 2: Get race for faction from template
    uint8 race = tmpl->GetRandomRace(faction);
    if (race == 0)
    {
        TC_LOG_WARN("playerbot.pool", "InstanceBotPool::CreatePoolBot - No valid race for {} in template {}",
            FactionToString(faction), tmpl->templateName);
        return ObjectGuid::Empty;
    }

    // Step 3: Allocate account from bot account pool (using BotAccountMgr)
    uint32 accountId = sBotAccountMgr->AcquireAccount();
    if (accountId == 0)
    {
        TC_LOG_WARN("playerbot.pool", "InstanceBotPool::CreatePoolBot - Failed to allocate account");
        return ObjectGuid::Empty;
    }

    // Step 4: Create character using BotSpawner's working async-safe method
    // NOTE: BotSpawner::CreateBotCharacter uses sPlayerbotCharDB which handles sync/async properly
    // BotCharacterCreator::CreateBotCharacter would crash during warmup due to async-only statements
    ObjectGuid botGuid = sBotSpawner->CreateBotCharacter(accountId);

    if (botGuid.IsEmpty())
    {
        sBotAccountMgr->ReleaseAccount(accountId);
        TC_LOG_WARN("playerbot.pool", "InstanceBotPool::CreatePoolBot - Character creation failed via BotSpawner");
        return ObjectGuid::Empty;
    }

    // Get the created character's info from cache
    CharacterCacheEntry const* charInfo = sCharacterCache->GetCharacterCacheByGuid(botGuid);
    std::string name = charInfo ? charInfo->Name : "Unknown";
    uint8 actualClass = charInfo ? charInfo->Class : tmpl->playerClass;

    // Determine pool type for compatibility
    PoolType poolType = (faction == Faction::Alliance) ? PoolType::PvP_Alliance : PoolType::PvP_Horde;

    uint32 minLevel, maxLevel;
    GetBracketLevelRange(bracket, minLevel, maxLevel);

    TC_LOG_DEBUG("playerbot.pool", "InstanceBotPool::CreatePoolBot - Created pool bot {} ({}), Role {}, Bracket {}-{}, Level {} (NOT logged in)",
        name, botGuid.ToString(), BotRoleToString(role), minLevel, maxLevel, level);

    // Create slot for the newly created bot - mark as READY (not logged in)
    // Bot will be logged in via BotSpawner when actually needed for an instance/BG
    InstanceBotSlot slot;
    slot.Initialize(botGuid, accountId, name, poolType, role);
    slot.level = level;
    slot.faction = faction;
    slot.gearScore = 0; // Will be set after spawn and gear application
    slot.playerClass = actualClass;
    slot.specId = 0; // Will be set after spawn
    slot.ForceState(PoolSlotState::Ready); // Ready in pool (NOT logged in yet)

    {
        std::unique_lock lock(_slotsMutex);
        _slots[botGuid] = std::move(slot);
    }

    // Add to ready index for O(1) lookup
    AddToReadyIndex(botGuid, role, faction, bracket);

    // Update bracket counts
    {
        std::unique_lock lock(_bracketCountsMutex);
        _bracketCounts.IncrementReady(bracket, faction, role);
        _bracketCounts.IncrementTotal(bracket, faction);
    }

    _statsDirty.store(true);
    return botGuid;
}

ObjectGuid InstanceBotPool::CreatePoolBotLegacy(BotRole role, PoolType poolType, uint32 level, bool deferWarmup)
{
    // Legacy wrapper - convert to per-bracket call
    Faction faction = GetFactionForPoolType(poolType);
    PoolBracket bracket = GetBracketForLevel(level);
    return CreatePoolBot(role, faction, bracket, deferWarmup);
}

bool InstanceBotPool::WarmUpBot(ObjectGuid botGuid)
{
    // ========================================================================
    // REFACTORED (2026-01-12): Login bot via BotSpawner when ACTUALLY NEEDED
    // Pool bots are NOT pre-logged-in. This method is called when assigning
    // a bot to an instance/BG - we have 1-2 minutes of queue time to login.
    //
    // FIX (2026-01-15): Register pending configuration BEFORE login so that
    // BotPostLoginConfigurator applies the correct level. Previously, pool bots
    // stayed at level 1 because no pending config was registered.
    // ========================================================================

    // Get slot info (accountId, target level, queue info, and human player for BG tracking)
    uint32 accountId = 0;
    uint32 targetLevel = 1;
    uint32 specId = 0;
    uint32 contentId = 0;
    InstanceType instanceType = InstanceType::Dungeon;
    ObjectGuid humanPlayerGuid;  // For BG invitation tracking
    {
        std::shared_lock lock(_slotsMutex);
        auto it = _slots.find(botGuid);
        if (it == _slots.end())
        {
            TC_LOG_WARN("playerbot.pool", "InstanceBotPool::WarmUpBot - Bot {} not found in pool",
                botGuid.ToString());
            return false;
        }
        accountId = it->second.accountId;
        targetLevel = it->second.level;  // Level from pool slot metadata
        specId = it->second.specId;
        // Get queue info set by AssignBot
        contentId = it->second.currentContentId;
        instanceType = it->second.currentInstanceType;
        humanPlayerGuid = it->second.humanPlayerGuid;  // For BG invitation tracking
    }

    // ========================================================================
    // CRITICAL FIX (2026-02-03): Handle already-online bots
    // ========================================================================
    // If the bot is already logged in, we should NOT try to spawn it again.
    // Instead, queue it directly for the content (BG/Dungeon/Arena).
    // Previously, spawn would "fail" and bot would be moved to Maintenance,
    // causing warm pool bots to never actually join BG queues.
    // ========================================================================
    if (Player* existingPlayer = ObjectAccessor::FindPlayer(botGuid))
    {
        TC_LOG_INFO("playerbot.pool", "InstanceBotPool::WarmUpBot - Bot {} already online, queueing directly for content {}",
            botGuid.ToString(), contentId);

        // Mark as instance bot if not already
        sBotWorldSessionMgr->MarkAsInstanceBot(botGuid);

        // Queue for content based on instance type
        bool queueSuccess = false;
        if (contentId > 0)
        {
            switch (instanceType)
            {
                case InstanceType::Battleground:
                {
                    BattlegroundTypeId bgTypeId = static_cast<BattlegroundTypeId>(contentId);
                    BattlegroundTemplate const* bgTemplate = sBattlegroundMgr->GetBattlegroundTemplateByTypeId(bgTypeId);
                    if (bgTemplate && !bgTemplate->MapIDs.empty())
                    {
                        PVPDifficultyEntry const* bracketEntry = DB2Manager::GetBattlegroundBracketByLevel(
                            bgTemplate->MapIDs.front(), existingPlayer->GetLevel());
                        if (bracketEntry)
                        {
                            BattlegroundBracketId bracketId = bracketEntry->GetBracketId();
                            queueSuccess = sBGBotManager->QueueBotForBG(existingPlayer, bgTypeId, bracketId);
                            TC_LOG_INFO("playerbot.pool", "InstanceBotPool::WarmUpBot - Queued already-online bot {} for BG {} bracket {}: {}",
                                botGuid.ToString(), contentId, static_cast<uint8>(bracketId), queueSuccess ? "SUCCESS" : "FAILED");
                        }
                    }
                    break;
                }
                case InstanceType::Dungeon:
                    // Dungeon queueing is handled by LFGBotManager, not direct warm-up.
                    // Bots queue via the LFG system for proper group composition.
                    TC_LOG_DEBUG("playerbot.pool", "InstanceBotPool::WarmUpBot - Dungeon bots queue via LFGBotManager, guid: {}",
                        botGuid.ToString());
                    break;
                case InstanceType::Arena:
                    // Arena queueing requires matchmaking logic (MMR, team composition).
                    // Not supported in direct warm-up path — bots join arenas reactively.
                    TC_LOG_DEBUG("playerbot.pool", "InstanceBotPool::WarmUpBot - Arena direct queueing not supported, guid: {}",
                        botGuid.ToString());
                    break;
                default:
                    break;
            }
        }

        // Update slot state to Assigned (queued for content)
        {
            std::unique_lock lock(_slotsMutex);
            auto it = _slots.find(botGuid);
            if (it != _slots.end())
                it->second.ForceState(PoolSlotState::Assigned);
        }

        // Call warmup complete with success (bot is already usable)
        OnBotWarmupComplete(botGuid, true);
        return true;
    }

    // Fallback: Try to get account ID from CharacterCache if not in slot
    if (accountId == 0)
    {
        CharacterCacheEntry const* charInfo = sCharacterCache->GetCharacterCacheByGuid(botGuid);
        if (charInfo && charInfo->AccountId > 0)
        {
            accountId = charInfo->AccountId;
            TC_LOG_INFO("playerbot.pool", "InstanceBotPool::WarmUpBot - Got account ID {} from CharacterCache for bot {}",
                accountId, botGuid.ToString());

            // Update the slot with the correct account ID
            {
                std::unique_lock lock(_slotsMutex);
                auto it = _slots.find(botGuid);
                if (it != _slots.end())
                    it->second.accountId = accountId;
            }
        }
        else
        {
            TC_LOG_WARN("playerbot.pool", "InstanceBotPool::WarmUpBot - No account ID for bot {} (not in slot or CharacterCache)",
                botGuid.ToString());
            return false;
        }
    }

    // ========================================================================
    // CRITICAL: Register pending configuration BEFORE bot logs in
    // This ensures BotPostLoginConfigurator::ApplyPendingConfiguration()
    // will apply the correct level when the bot enters the world.
    //
    // CRITICAL FIX (2026-01-18): Include targetGearScore so BotGearFactory
    // generates appropriate gear. Pool bots don't have templates, so they
    // rely entirely on BotGearFactory for equipment.
    // ========================================================================
    BotPendingConfiguration pendingConfig;
    pendingConfig.botGuid = botGuid;
    pendingConfig.targetLevel = targetLevel;
    pendingConfig.specId = specId;
    pendingConfig.targetGearScore = targetLevel * 10;  // Approximate gear score based on level
    pendingConfig.createdAt = std::chrono::steady_clock::now();
    pendingConfig.humanPlayerGuid = humanPlayerGuid;  // For BG invitation tracking

    // CRITICAL FIX: Set queue info so bot auto-queues for content after login
    // This fixes the issue where warm pool bots are "assigned" but never actually
    // queued for BG because they weren't in the world when QueueStatePoller tried
    // to call ObjectAccessor::FindPlayer()
    if (contentId > 0)
    {
        switch (instanceType)
        {
            case InstanceType::Battleground:
                pendingConfig.battlegroundIdToQueue = contentId;
                TC_LOG_DEBUG("playerbot.pool", "InstanceBotPool::WarmUpBot - Bot {} will queue for BG {} after login (tracking human {})",
                    botGuid.ToString(), contentId, humanPlayerGuid.ToString());
                break;
            case InstanceType::Dungeon:
                pendingConfig.dungeonIdToQueue = contentId;
                TC_LOG_DEBUG("playerbot.pool", "InstanceBotPool::WarmUpBot - Bot {} will queue for dungeon {} after login",
                    botGuid.ToString(), contentId);
                break;
            case InstanceType::Arena:
                pendingConfig.arenaTypeToQueue = contentId;
                TC_LOG_DEBUG("playerbot.pool", "InstanceBotPool::WarmUpBot - Bot {} will queue for arena {} after login",
                    botGuid.ToString(), contentId);
                break;
            default:
                break;
        }
    }

    // Mark as instance bot - this will be applied after login by BotPostLoginConfigurator
    // CRITICAL: This ensures warm pool bots get proper idle timeout and restricted behavior
    pendingConfig.markAsInstanceBot = true;

    sBotPostLoginConfigurator->RegisterPendingConfig(std::move(pendingConfig));

    TC_LOG_INFO("playerbot.pool", "InstanceBotPool::WarmUpBot - Registered pending config for bot {} (level={}, spec={}, gearScore={}, contentId={}, type={})",
        botGuid.ToString(), targetLevel, specId, pendingConfig.targetGearScore, contentId, static_cast<uint8>(instanceType));

    // Use BotSpawner to spawn the bot (same flow as regular bots)
    // This uses the proven workflow: SpawnBot -> async character selection -> login
    SpawnRequest request;
    request.type = SpawnRequest::SPECIFIC_CHARACTER;
    request.accountId = accountId;
    request.characterGuid = botGuid;
    request.bypassMaxBotsLimit = true;  // Pool bots bypass MaxBots limit - they're temporary for BG/dungeon/arena

    TC_LOG_INFO("playerbot.pool", "WarmUpBot - Creating SpawnRequest: type=SPECIFIC_CHARACTER, guid={}, accountId={}, bypassMaxBotsLimit={}",
        botGuid.ToString(), accountId, request.bypassMaxBotsLimit);
    request.callback = [this, botGuid](bool success, ObjectGuid guid) {
        if (success)
        {
            TC_LOG_INFO("playerbot.pool", "InstanceBotPool: Pool bot {} successfully logged in via BotSpawner",
                botGuid.ToString());
            OnBotWarmupComplete(botGuid, true);
        }
        else
        {
            TC_LOG_WARN("playerbot.pool", "InstanceBotPool: Pool bot {} failed to login via BotSpawner",
                botGuid.ToString());
            OnBotWarmupComplete(botGuid, false);
        }
    };

    // Update slot state to Warming (login in progress)
    {
        std::unique_lock lock(_slotsMutex);
        auto it = _slots.find(botGuid);
        if (it != _slots.end())
            it->second.ForceState(PoolSlotState::Warming);
    }

    bool queued = sBotSpawner->SpawnBot(request);
    if (queued)
    {
        TC_LOG_INFO("playerbot.pool", "InstanceBotPool::WarmUpBot - Queued pool bot {} for login via BotSpawner",
            botGuid.ToString());
    }
    else
    {
        TC_LOG_WARN("playerbot.pool", "InstanceBotPool::WarmUpBot - Failed to queue bot {} via BotSpawner",
            botGuid.ToString());
        // Revert state
        std::unique_lock lock(_slotsMutex);
        auto it = _slots.find(botGuid);
        if (it != _slots.end())
            it->second.ForceState(PoolSlotState::Ready);
    }

    return queued;
}

void InstanceBotPool::OnBotWarmupComplete(ObjectGuid botGuid, bool success)
{
    std::unique_lock lock(_slotsMutex);

    auto it = _slots.find(botGuid);
    if (it == _slots.end())
        return;

    if (success)
    {
        // Check if bot was assigned to content (has assignment info)
        if (it->second.currentContentId != 0 || it->second.currentInstanceId != 0)
        {
            // Bot was assigned - transition to Assigned state
            it->second.TransitionTo(PoolSlotState::Assigned);
            it->second.lastAssignment = std::chrono::steady_clock::now();
            ++it->second.assignmentCount;
            ++_stats.activity.assignmentsThisHour;

            TC_LOG_INFO("playerbot.pool", "InstanceBotPool: Bot {} now ASSIGNED and logged in (content: {}, instance: {})",
                botGuid.ToString(), it->second.currentContentId, it->second.currentInstanceId);
        }
        else
        {
            // Bot was just warming (no assignment) - back to Ready
            it->second.TransitionTo(PoolSlotState::Ready);
            TC_LOG_DEBUG("playerbot.pool", "InstanceBotPool: Bot {} warmup complete, now Ready",
                botGuid.ToString());
        }
        ++_stats.activity.warmupsThisHour;
    }
    else
    {
        // Login failed - reset assignment info and put in maintenance
        it->second.currentInstanceId = 0;
        it->second.currentContentId = 0;
        it->second.ForceState(PoolSlotState::Maintenance);

        TC_LOG_WARN("playerbot.pool", "InstanceBotPool: Bot {} warmup FAILED, moved to Maintenance",
            botGuid.ToString());
    }

    _statsDirty.store(true);
}

// ============================================================================
// INTERNAL METHODS - Bot Selection
// ============================================================================

ObjectGuid InstanceBotPool::SelectBestBot(BotRole role, Faction faction,
                                           uint32 level, uint32 minGearScore)
{
    std::unique_lock lock(_slotsMutex);

    ObjectGuid bestGuid = ObjectGuid::Empty;
    float bestScore = -1.0f;

    for (auto& [guid, slot] : _slots)
    {
        if (slot.state != PoolSlotState::Ready)
            continue;
        if (slot.role != role)
            continue;
        if (slot.faction != faction)
            continue;
        if (!slot.IsInLevelRange(level, 10))
            continue;
        if (slot.gearScore < minGearScore)
            continue;

        float score = slot.CalculateAssignmentScore(level, minGearScore);
        if (score > bestScore)
        {
            bestScore = score;
            bestGuid = guid;
        }
    }

    return bestGuid;
}

std::vector<ObjectGuid> InstanceBotPool::SelectBots(BotRole role, Faction faction,
                                                     uint32 level, uint32 count,
                                                     uint32 minGearScore)
{
    std::vector<ObjectGuid> result;
    result.reserve(count);

    std::unique_lock lock(_slotsMutex);

    // Collect candidates
    std::vector<std::pair<ObjectGuid, float>> candidates;

    for (auto& [guid, slot] : _slots)
    {
        if (slot.state != PoolSlotState::Ready)
            continue;
        if (slot.role != role)
            continue;
        if (slot.faction != faction)
            continue;
        if (!slot.IsInLevelRange(level, 10))
            continue;
        if (slot.gearScore < minGearScore)
            continue;

        float score = slot.CalculateAssignmentScore(level, minGearScore);
        candidates.emplace_back(guid, score);
    }

    // Sort by score (descending)
    std::sort(candidates.begin(), candidates.end(),
        [](auto const& a, auto const& b) { return a.second > b.second; });

    // Take top N
    for (size_t i = 0; i < count && i < candidates.size(); ++i)
    {
        result.push_back(candidates[i].first);
    }

    return result;
}

bool InstanceBotPool::AssignBot(ObjectGuid botGuid, uint32 instanceId,
                                 uint32 contentId, InstanceType type, uint32 targetLevel,
                                 ObjectGuid humanPlayerGuid)
{
    // ========================================================================
    // REFACTORED (2026-01-12): Pool bots login on-demand via BotSpawner
    // When assigning a bot, we need to actually log them in since they're
    // stored as database records only (not pre-logged-in)
    //
    // FIX (2026-01-15): Store targetLevel in slot so WarmUpBot can register
    // the correct level in pending configuration (not the bracket level).
    //
    // FIX (2026-02-05): Store humanPlayerGuid for BG invitation tracking.
    // This allows WarmUpBot to pass it to BotPendingConfiguration so that
    // QueueBotForBGWithTracking is used instead of QueueBotForBG.
    // ========================================================================

    {
        std::unique_lock lock(_slotsMutex);

        auto it = _slots.find(botGuid);
        if (it == _slots.end())
            return false;

        // Store assignment info before warming (in case login completes fast)
        it->second.currentInstanceId = instanceId;
        it->second.currentContentId = contentId;
        it->second.currentInstanceType = type;
        it->second.humanPlayerGuid = humanPlayerGuid;  // For BG invitation tracking

        // FIX: Update slot.level to target level so WarmUpBot uses correct level
        // for pending configuration. Pool bots are created at bracket midpoint
        // (5, 35, 65, 75) but need to be leveled to match the player.
        if (targetLevel > 0)
            it->second.level = targetLevel;
    }

    // Initiate login via BotSpawner (this uses the proven regular bot workflow)
    // The callback in WarmUpBot will update state to Assigned when login completes
    if (!WarmUpBot(botGuid))
    {
        TC_LOG_WARN("playerbot.pool", "InstanceBotPool::AssignBot - Failed to initiate login for bot {}",
            botGuid.ToString());

        // Revert assignment info
        std::unique_lock lock(_slotsMutex);
        auto it = _slots.find(botGuid);
        if (it != _slots.end())
        {
            it->second.currentInstanceId = 0;
            it->second.currentContentId = 0;
        }
        return false;
    }

    TC_LOG_DEBUG("playerbot.pool", "InstanceBotPool::AssignBot - Bot {} assigned to {} {} (login in progress)",
        botGuid.ToString(), InstanceTypeToString(type), contentId);

    return true;
}

// ============================================================================
// INTERNAL METHODS - Pool Maintenance
// ============================================================================

void InstanceBotPool::ProcessWarmingRetries()
{
    // ========================================================================
    // REFACTORED (2026-01-12): Handle stuck/timed-out warming bots
    // Bots only enter Warming state when being assigned (WarmUpBot called)
    // If they stay in Warming too long, the login failed - return to Ready
    // ========================================================================

    std::vector<ObjectGuid> stuckBots;
    auto warmupTimeout = _config.timing.warmupTimeout;

    {
        std::shared_lock lock(_slotsMutex);
        for (auto const& [guid, slot] : _slots)
        {
            if (slot.state == PoolSlotState::Warming)
            {
                // If bot has been warming for too long (default 30s), consider it stuck
                auto timeSinceStateChange = slot.TimeSinceStateChange();
                if (timeSinceStateChange >= warmupTimeout)
                {
                    stuckBots.push_back(guid);
                }
            }
        }
    }

    // Reset stuck bots back to Ready state
    if (!stuckBots.empty())
    {
        TC_LOG_WARN("playerbot.pool", "ProcessWarmingRetries - {} bots stuck in Warming state (timeout {}ms), resetting to Ready",
            stuckBots.size(), warmupTimeout.count());

        std::unique_lock lock(_slotsMutex);
        for (ObjectGuid const& guid : stuckBots)
        {
            auto it = _slots.find(guid);
            if (it != _slots.end() && it->second.state == PoolSlotState::Warming)
            {
                it->second.ForceState(PoolSlotState::Ready);
                TC_LOG_DEBUG("playerbot.pool", "ProcessWarmingRetries - Reset stuck bot {} to Ready",
                    guid.ToString());
            }
        }
    }
}

void InstanceBotPool::ProcessCooldowns()
{
    std::vector<std::tuple<ObjectGuid, BotRole, Faction, uint32>> expiredBots;

    {
        std::unique_lock lock(_slotsMutex);

        auto cooldownDuration = _config.timing.cooldownDuration;

        for (auto& [guid, slot] : _slots)
        {
            if (slot.state == PoolSlotState::Cooldown)
            {
                if (slot.IsCooldownExpired(cooldownDuration))
                {
                    // Store info before transition
                    expiredBots.emplace_back(guid, slot.role, slot.faction, slot.level);
                    slot.TransitionTo(PoolSlotState::Ready);
                    ++_stats.activity.cooldownsExpiredThisHour;

                    if (_config.logging.logCooldowns)
                    {
                        TC_LOG_DEBUG("playerbot.pool", "Bot {} cooldown expired, now ready",
                            guid.ToString());
                    }
                }
            }
        }
    }

    // Add expired bots back to ready index
    for (auto const& [guid, role, faction, level] : expiredBots)
    {
        PoolBracket bracket = GetBracketForLevel(level);
        AddToReadyIndex(guid, role, faction, bracket);

        // Update bracket counts
        {
            std::unique_lock bracketLock(_bracketCountsMutex);
            _bracketCounts.IncrementReady(bracket, faction, role);
        }
    }

    _statsDirty.store(true);
}

void InstanceBotPool::ProcessReservations()
{
    std::lock_guard lock(_reservationMutex);

    auto now = std::chrono::steady_clock::now();
    std::vector<uint32> expiredReservations;

    for (auto const& [id, reservation] : _reservations)
    {
        if (reservation.IsExpired())
        {
            expiredReservations.push_back(id);
        }
    }

    // Can't call CancelReservation while holding lock, so collect and process
    for (uint32 id : expiredReservations)
    {
        // Return reserved bots to ready state
        auto it = _reservations.find(id);
        if (it != _reservations.end())
        {
            std::unique_lock slotLock(_slotsMutex);
            for (ObjectGuid guid : it->second.reservedBots)
            {
                auto slotIt = _slots.find(guid);
                if (slotIt != _slots.end())
                {
                    slotIt->second.CancelReservation();
                }
            }
            slotLock.unlock();

            if (_config.logging.logReservations)
            {
                TC_LOG_INFO("playerbot.pool", "Reservation {} expired and cancelled", id);
            }

            _reservations.erase(it);
            ++_stats.activity.timeoutRequestsThisHour;
        }
    }

    _statsDirty.store(true);
}

void InstanceBotPool::UpdateStatistics()
{
    if (!_statsDirty.load())
        return;

    std::shared_lock slotLock(_slotsMutex);
    std::unique_lock statsLock(_statsMutex);

    // Reset slot stats
    _stats.slotStats.Reset();

    // Reset bracket stats
    for (auto& bracketStat : _stats.bracketStats.brackets)
    {
        bracketStat.totalSlots = 0;
        bracketStat.readySlots = 0;
        bracketStat.assignedSlots = 0;
        bracketStat.allianceReady = 0;
        bracketStat.hordeReady = 0;
        bracketStat.tanksReady = 0;
        bracketStat.healersReady = 0;
        bracketStat.dpsReady = 0;
    }

    // Count slots by state
    for (auto const& [guid, slot] : _slots)
    {
        switch (slot.state)
        {
            case PoolSlotState::Empty:       ++_stats.slotStats.emptySlots; break;
            case PoolSlotState::Creating:    ++_stats.slotStats.creatingSlots; break;
            case PoolSlotState::Warming:     ++_stats.slotStats.warmingSlots; break;
            case PoolSlotState::Ready:       ++_stats.slotStats.readySlots; break;
            case PoolSlotState::Reserved:    ++_stats.slotStats.reservedSlots; break;
            case PoolSlotState::Assigned:    ++_stats.slotStats.assignedSlots; break;
            case PoolSlotState::Cooldown:    ++_stats.slotStats.cooldownSlots; break;
            case PoolSlotState::Maintenance: ++_stats.slotStats.maintenanceSlots; break;
            default: break;
        }

        // Update role stats
        auto roleIdx = static_cast<size_t>(slot.role);
        if (roleIdx < _stats.roleStats.size())
        {
            ++_stats.roleStats[roleIdx].totalSlots;
            if (slot.state == PoolSlotState::Ready)
                ++_stats.roleStats[roleIdx].readySlots;
            else if (slot.state == PoolSlotState::Assigned)
                ++_stats.roleStats[roleIdx].assignedSlots;
        }

        // Update faction stats
        auto factionIdx = static_cast<size_t>(slot.faction);
        if (factionIdx < _stats.factionStats.size())
        {
            ++_stats.factionStats[factionIdx].totalSlots;
            if (slot.state == PoolSlotState::Ready)
                ++_stats.factionStats[factionIdx].readySlots;
            else if (slot.state == PoolSlotState::Assigned)
                ++_stats.factionStats[factionIdx].assignedSlots;
        }

        // Update per-bracket stats
        PoolBracket bracket = GetBracketForLevel(slot.level);
        auto bracketIdx = static_cast<size_t>(bracket);
        if (bracketIdx < NUM_LEVEL_BRACKETS)
        {
            PoolBracketStats& bs = _stats.bracketStats.brackets[bracketIdx];
            bs.bracket = bracket;
            ++bs.totalSlots;

            if (slot.state == PoolSlotState::Ready)
            {
                ++bs.readySlots;
                if (slot.faction == Faction::Alliance)
                    ++bs.allianceReady;
                else
                    ++bs.hordeReady;

                switch (slot.role)
                {
                    case BotRole::Tank:   ++bs.tanksReady; break;
                    case BotRole::Healer: ++bs.healersReady; break;
                    case BotRole::DPS:    ++bs.dpsReady; break;
                    default: break;
                }
            }
            else if (slot.state == PoolSlotState::Assigned)
            {
                ++bs.assignedSlots;
            }

            // Set configured slots from config
            bs.configuredSlots = _config.poolSize.bracketPools[bracketIdx].GetTotalBots();
        }
    }

    _stats.timestamp = std::chrono::system_clock::now();
    _statsDirty.store(false);
}

void InstanceBotPool::CheckHourlyReset()
{
    auto now = std::chrono::system_clock::now();
    auto hoursSinceStart = std::chrono::duration_cast<std::chrono::hours>(now - _hourStart);

    if (hoursSinceStart >= std::chrono::hours(1))
    {
        std::unique_lock lock(_statsMutex);
        _stats.ResetHourly();
        _hourStart = now;

        if (_config.logging.logDetailedStats)
        {
            TC_LOG_INFO("playerbot.pool", "Hourly statistics reset");
        }
    }

    auto daysSinceStart = std::chrono::duration_cast<std::chrono::hours>(now - _dayStart);
    if (daysSinceStart >= std::chrono::hours(24))
    {
        std::unique_lock lock(_statsMutex);
        _stats.ResetDaily();
        _dayStart = now;

        if (_config.logging.logDetailedStats)
        {
            TC_LOG_INFO("playerbot.pool", "Daily statistics reset");
        }
    }
}

// ============================================================================
// INTERNAL METHODS - Database
// ============================================================================

void InstanceBotPool::SyncToDatabase()
{
    // ========================================================================
    // WARM POOL PERSISTENCE (2026-01-16)
    // Save warm pool bot state to database for persistence across restarts.
    //
    // Warm pool bots PERSIST in the database:
    // - Character data remains in standard character tables
    // - Pool metadata (role, bracket, state) is saved to playerbot_instance_pool
    //
    // JIT bots are NOT saved here - they are tracked separately in
    // playerbot_jit_bots and deleted on shutdown.
    //
    // IMPORTANT: This method builds a BATCHED query to avoid blocking the
    // main thread. Individual INSERT/REPLACE per bot caused 60+ second
    // freezes with large pools (freeze detector crash).
    // ========================================================================

    // Take a snapshot of the slots under lock, then release lock before DB operations
    std::vector<std::tuple<ObjectGuid, uint32, std::string, BotRole, Faction, uint8, uint32, uint8, uint32, PoolSlotState, uint32, uint32, uint32, uint32>> slotSnapshot;

    {
        std::shared_lock lock(_slotsMutex);

        if (_slots.empty())
        {
            TC_LOG_DEBUG("playerbot.pool", "SyncToDatabase: No warm pool slots to sync");
            return;
        }

        slotSnapshot.reserve(_slots.size());

        for (auto const& [guid, slot] : _slots)
        {
            if (slot.botGuid.IsEmpty())
                continue;

            slotSnapshot.emplace_back(
                slot.botGuid,
                slot.accountId,
                slot.botName,
                slot.role,
                slot.faction,
                slot.playerClass,
                slot.specId,
                slot.level,
                slot.gearScore,
                slot.state,
                slot.assignmentCount,
                slot.successfulCompletions,
                slot.earlyExits,
                slot.totalInstanceTime
            );
        }
    }

    if (slotSnapshot.empty())
    {
        TC_LOG_DEBUG("playerbot.pool", "SyncToDatabase: No valid warm pool slots to sync");
        return;
    }

    TC_LOG_DEBUG("playerbot.pool", "SyncToDatabase: Preparing batch sync for {} bots", slotSnapshot.size());

    // Run database sync in a background thread to avoid blocking world thread
    // This prevents freeze detector crashes when syncing large pools
    std::thread([snapshot = std::move(slotSnapshot)]() mutable {
        try
        {
            // Build a batched multi-row INSERT query
            // Use INSERT ... ON DUPLICATE KEY UPDATE for better performance than REPLACE
            std::ostringstream query;
            query << "INSERT INTO `playerbot_instance_pool` "
                  << "(`bot_guid`, `account_id`, `bot_name`, `role`, `faction`, `player_class`, "
                  << "`spec_id`, `level`, `bracket`, `is_warm_pool`, `gear_score`, `slot_state`, "
                  << "`assignment_count`, `successful_completions`, `early_exits`, `total_instance_time`) VALUES ";

            bool first = true;
            uint32 batchCount = 0;
            constexpr uint32 BATCH_SIZE = 100;  // Insert 100 rows per batch

            for (auto const& [botGuid, accountId, botName, role, faction, playerClass, specId, level, gearScore, state, assignCount, successCount, earlyExits, totalTime] : snapshot)
            {
                PoolBracket bracket = GetBracketForLevel(level);

                std::string factionStr = (faction == Faction::Alliance) ? "ALLIANCE" : "HORDE";
                std::string roleStr;
                switch (role)
                {
                    case BotRole::Tank:   roleStr = "TANK"; break;
                    case BotRole::Healer: roleStr = "HEALER"; break;
                    default:              roleStr = "DPS"; break;
                }
                std::string stateStr;
                switch (state)
                {
                    case PoolSlotState::Ready:    stateStr = "READY"; break;
                    case PoolSlotState::Assigned: stateStr = "ASSIGNED"; break;
                    case PoolSlotState::Cooldown: stateStr = "COOLDOWN"; break;
                    default:                      stateStr = "READY"; break;
                }

                // Escape bot name for SQL (simple escape for single quotes)
                std::string escapedName = botName;
                size_t pos = 0;
                while ((pos = escapedName.find('\'', pos)) != std::string::npos)
                {
                    escapedName.replace(pos, 1, "''");
                    pos += 2;
                }

                if (!first)
                    query << ", ";
                first = false;

                query << "(" << botGuid.GetCounter()
                      << ", " << accountId
                      << ", '" << escapedName << "'"
                      << ", '" << roleStr << "'"
                      << ", '" << factionStr << "'"
                      << ", " << static_cast<uint32>(playerClass)
                      << ", " << specId
                      << ", " << static_cast<uint32>(level)
                      << ", " << static_cast<uint32>(bracket)
                      << ", 1"  // is_warm_pool
                      << ", " << gearScore
                      << ", '" << stateStr << "'"
                      << ", " << assignCount
                      << ", " << successCount
                      << ", " << earlyExits
                      << ", " << totalTime
                      << ")";

                ++batchCount;

                // Execute batch when reaching BATCH_SIZE
                if (batchCount >= BATCH_SIZE)
                {
                    query << " ON DUPLICATE KEY UPDATE "
                          << "`slot_state` = VALUES(`slot_state`), "
                          << "`assignment_count` = VALUES(`assignment_count`), "
                          << "`successful_completions` = VALUES(`successful_completions`), "
                          << "`early_exits` = VALUES(`early_exits`), "
                          << "`total_instance_time` = VALUES(`total_instance_time`)";

                    sPlayerbotDatabase->Execute(query.str().c_str());

                    // Reset for next batch
                    query.str("");
                    query.clear();
                    query << "INSERT INTO `playerbot_instance_pool` "
                          << "(`bot_guid`, `account_id`, `bot_name`, `role`, `faction`, `player_class`, "
                          << "`spec_id`, `level`, `bracket`, `is_warm_pool`, `gear_score`, `slot_state`, "
                          << "`assignment_count`, `successful_completions`, `early_exits`, `total_instance_time`) VALUES ";
                    first = true;
                    batchCount = 0;
                }
            }

            // Execute any remaining rows
            if (batchCount > 0)
            {
                // CRITICAL FIX: Include account_id in ON DUPLICATE KEY UPDATE
                // Previously, if a bot was saved with account_id = 0, it would never
                // be updated even after WarmUpBot corrected it from CharacterCache.
                // This caused "No account ID for bot" errors after server restart.
                query << " ON DUPLICATE KEY UPDATE "
                      << "`account_id` = VALUES(`account_id`), "
                      << "`slot_state` = VALUES(`slot_state`), "
                      << "`assignment_count` = VALUES(`assignment_count`), "
                      << "`successful_completions` = VALUES(`successful_completions`), "
                      << "`early_exits` = VALUES(`early_exits`), "
                      << "`total_instance_time` = VALUES(`total_instance_time`)";

                sPlayerbotDatabase->Execute(query.str().c_str());
            }

            TC_LOG_DEBUG("playerbot.pool", "SyncToDatabase: Batch sync complete for {} bots", snapshot.size());
        }
        catch (std::exception const& e)
        {
            TC_LOG_ERROR("playerbot.pool", "SyncToDatabase: Exception during batch sync: {}", e.what());
        }
    }).detach();
}

void InstanceBotPool::LoadFromDatabase()
{
    // ========================================================================
    // WARM POOL PERSISTENCE (2026-01-16)
    // Load existing warm pool bots from database and restore their slots.
    //
    // This method:
    // 1. Queries playerbot_instance_pool for warm pool bots
    // 2. Verifies each character still exists in characters table
    // 3. Restores the InstanceBotSlot with saved metadata
    // 4. Adds to ready index for fast assignment lookup
    //
    // After loading, ReconcileBracketDistribution() should be called to
    // create missing bots or remove excess bots to match target distribution.
    // ========================================================================

    TC_LOG_INFO("playerbot.pool", "Loading warm pool bots from database...");

    // Query playerbot database (NOT characters database)
    // Character existence is verified via CharacterCache after loading
    QueryResult result = sPlayerbotDatabase->Query(
        "SELECT `bot_guid`, `account_id`, `bot_name`, `role`, `faction`, "
        "`player_class`, `spec_id`, `level`, `bracket`, `gear_score`, "
        "`slot_state`, `assignment_count`, `successful_completions`, "
        "`early_exits`, `total_instance_time` "
        "FROM `playerbot_instance_pool` "
        "WHERE `is_warm_pool` = 1 "
        "ORDER BY `bracket`, `faction`, `role`"
    );

    if (!result)
    {
        TC_LOG_INFO("playerbot.pool", "No existing warm pool bots found in database");
        return;
    }

    std::unique_lock lock(_slotsMutex);

    uint32 loadedCount = 0;
    uint32 orphanedCount = 0;
    std::vector<uint64> orphanedGuids; // Collect orphaned GUIDs for cleanup
    std::vector<uint32> accountsToMark; // Collect account IDs to mark as in-use

    do
    {
        Field* fields = result->Fetch();

        uint64 guidLow = fields[0].GetUInt64();
        ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(guidLow);

        // Verify character exists in CharacterCache (cross-database check)
        if (!sCharacterCache->HasCharacterCacheEntry(guid))
        {
            TC_LOG_WARN("playerbot.pool", "Warm pool bot {} not found in character cache, skipping",
                guid.ToString());
            orphanedGuids.push_back(guidLow);
            ++orphanedCount;
            continue;
        }

        // Create and populate slot
        InstanceBotSlot slot;
        slot.botGuid = guid;
        slot.accountId = fields[1].GetUInt32();
        slot.botName = fields[2].GetString();

        // CRITICAL FIX: Repair account_id = 0 from CharacterCache
        // This fixes bots that were previously saved with account_id = 0
        // The character exists (we checked above), so CharacterCache should have it
        if (slot.accountId == 0)
        {
            CharacterCacheEntry const* charInfo = sCharacterCache->GetCharacterCacheByGuid(guid);
            if (charInfo && charInfo->AccountId > 0)
            {
                slot.accountId = charInfo->AccountId;
                TC_LOG_INFO("playerbot.pool", "LoadFromDatabase: Repaired account_id for {} from CharacterCache (accountId={})",
                    guid.ToString(), slot.accountId);
            }
            else
            {
                // Character cache doesn't have account - this bot won't be able to login
                TC_LOG_WARN("playerbot.pool", "LoadFromDatabase: Bot {} has account_id=0 and CharacterCache has no account, bot may fail to login",
                    guid.ToString());
            }
        }

        // Parse role enum
        std::string roleStr = fields[3].GetString();
        if (roleStr == "TANK")
            slot.role = BotRole::Tank;
        else if (roleStr == "HEALER")
            slot.role = BotRole::Healer;
        else
            slot.role = BotRole::DPS;

        // Parse faction enum
        std::string factionStr = fields[4].GetString();
        slot.faction = (factionStr == "HORDE") ? Faction::Horde : Faction::Alliance;

        slot.playerClass = fields[5].GetUInt8();
        slot.specId = fields[6].GetUInt32();
        slot.level = fields[7].GetUInt8();

        // Bracket is stored in DB but we recalculate from level for safety
        // PoolBracket bracket = static_cast<PoolBracket>(fields[8].GetUInt8());

        slot.gearScore = fields[9].GetUInt32();

        // Parse state enum - always start as Ready for warm pool bots
        // (previous Assigned/Cooldown states are stale after restart)
        slot.state = PoolSlotState::Ready;
        slot.stateChangeTime = std::chrono::steady_clock::now();

        slot.assignmentCount = fields[11].GetUInt32();
        slot.successfulCompletions = fields[12].GetUInt32();
        slot.earlyExits = fields[13].GetUInt32();
        slot.totalInstanceTime = fields[14].GetUInt32();

        // Determine pool type based on faction (for PvP content)
        slot.poolType = (slot.faction == Faction::Alliance) ? PoolType::PvP_Alliance : PoolType::PvP_Horde;

        // Add to slots map
        _slots[guid] = slot;
        ++loadedCount;

        // Collect account ID to mark as in-use (prevent zone spawning from using pool bot accounts)
        if (slot.accountId > 0)
        {
            accountsToMark.push_back(slot.accountId);
        }

        TC_LOG_DEBUG("playerbot.pool", "Loaded warm pool bot: {} ({}) Role={} Faction={} Level={}",
            slot.botName, guid.ToString(),
            BotRoleToString(slot.role), FactionToString(slot.faction), slot.level);

    } while (result->NextRow());

    lock.unlock();

    // ========================================================================
    // CRITICAL FIX: Mark pool bot accounts as in-use in BotAccountMgr
    // This prevents zone-based spawning from acquiring pool bot accounts,
    // which would cause character selection to find pool bot characters and
    // spawn them with bypassLimit=false (causing "MAX BOTS LIMIT" errors)
    // ========================================================================
    if (!accountsToMark.empty())
    {
        uint32 markedCount = 0;
        for (uint32 accountId : accountsToMark)
        {
            if (sBotAccountMgr->MarkAccountInUse(accountId))
            {
                ++markedCount;
            }
        }
        TC_LOG_INFO("playerbot.pool", "Marked {} pool bot accounts as in-use in BotAccountMgr",
            markedCount);
    }

    // Rebuild ready index with loaded bots
    RebuildReadyIndex();

    // Clean up orphaned entries (bots in pool table but character deleted)
    if (!orphanedGuids.empty())
    {
        TC_LOG_WARN("playerbot.pool", "Cleaning up {} orphaned warm pool entries from database", orphanedGuids.size());

        // Delete each orphaned entry from playerbot database by GUID
        for (uint64 orphanedGuid : orphanedGuids)
        {
            std::string query = Trinity::StringFormat(
                "DELETE FROM `playerbot_instance_pool` WHERE `bot_guid` = {}",
                orphanedGuid
            );
            sPlayerbotDatabase->Execute(query.c_str());
        }
    }

    TC_LOG_INFO("playerbot.pool", "Loaded {} warm pool bots from database ({} orphaned removed)",
        loadedCount, orphanedCount);
}

// ============================================================================
// PER-BRACKET POOL QUERIES
// ============================================================================

uint32 InstanceBotPool::GetAvailableCountForBracket(PoolBracket bracket, Faction faction, BotRole role) const
{
    std::shared_lock lock(_bracketCountsMutex);

    if (role == BotRole::Max)
    {
        // All roles
        return _bracketCounts.GetReady(bracket, faction);
    }
    else
    {
        return _bracketCounts.GetReadyByRole(bracket, faction, role);
    }
}

PoolBracketStats InstanceBotPool::GetBracketStatistics(PoolBracket bracket) const
{
    PoolBracketStats stats;
    stats.bracket = bracket;

    auto bracketIdx = static_cast<size_t>(bracket);
    if (bracketIdx >= NUM_LEVEL_BRACKETS)
        return stats;

    BracketPoolConfig const& config = _config.poolSize.bracketPools[bracketIdx];
    stats.configuredSlots = config.GetTotalBots();

    std::shared_lock bracketLock(_bracketCountsMutex);
    stats.allianceReady = _bracketCounts.allianceReady[bracketIdx];
    stats.hordeReady = _bracketCounts.hordeReady[bracketIdx];
    stats.readySlots = stats.allianceReady + stats.hordeReady;

    stats.totalSlots = _bracketCounts.allianceTotal[bracketIdx] + _bracketCounts.hordeTotal[bracketIdx];
    stats.assignedSlots = stats.totalSlots - stats.readySlots;

    // Get per-role counts
    stats.tanksReady = _bracketCounts.GetReadyByRole(bracket, Faction::Alliance, BotRole::Tank) +
                       _bracketCounts.GetReadyByRole(bracket, Faction::Horde, BotRole::Tank);
    stats.healersReady = _bracketCounts.GetReadyByRole(bracket, Faction::Alliance, BotRole::Healer) +
                         _bracketCounts.GetReadyByRole(bracket, Faction::Horde, BotRole::Healer);
    stats.dpsReady = _bracketCounts.GetReadyByRole(bracket, Faction::Alliance, BotRole::DPS) +
                     _bracketCounts.GetReadyByRole(bracket, Faction::Horde, BotRole::DPS);

    return stats;
}

AllPoolBracketStats InstanceBotPool::GetAllBracketStatistics() const
{
    AllPoolBracketStats allStats;

    for (uint8 i = 0; i < NUM_LEVEL_BRACKETS; ++i)
    {
        allStats.brackets[i] = GetBracketStatistics(static_cast<PoolBracket>(i));
    }

    return allStats;
}

bool InstanceBotPool::CanBracketSupportDungeon(PoolBracket bracket, Faction faction) const
{
    std::shared_lock lock(_bracketCountsMutex);

    // Need 1 tank, 1 healer, 3 DPS minimum for a dungeon
    uint32 tanks = _bracketCounts.GetReadyByRole(bracket, faction, BotRole::Tank);
    uint32 healers = _bracketCounts.GetReadyByRole(bracket, faction, BotRole::Healer);
    uint32 dps = _bracketCounts.GetReadyByRole(bracket, faction, BotRole::DPS);

    return (tanks >= 1 && healers >= 1 && dps >= 3);
}

bool InstanceBotPool::CanBracketSupportBG(PoolBracket bracket, uint32 allianceNeeded, uint32 hordeNeeded) const
{
    std::shared_lock lock(_bracketCountsMutex);

    uint32 allianceReady = _bracketCounts.GetReady(bracket, Faction::Alliance);
    uint32 hordeReady = _bracketCounts.GetReady(bracket, Faction::Horde);

    return (allianceReady >= allianceNeeded && hordeReady >= hordeNeeded);
}

std::vector<PoolBracket> InstanceBotPool::GetBracketsWithShortage() const
{
    std::vector<PoolBracket> result;

    for (uint8 i = 0; i < NUM_LEVEL_BRACKETS; ++i)
    {
        PoolBracket bracket = static_cast<PoolBracket>(i);
        PoolBracketStats stats = GetBracketStatistics(bracket);

        // Consider shortage if below 80% of configured capacity
        if (stats.HasShortage())
        {
            result.push_back(bracket);
        }
    }

    return result;
}

PoolBracket InstanceBotPool::GetMostDepletedBracket() const
{
    PoolBracket mostDepleted = PoolBracket::Bracket_80_Max;
    float lowestPct = 100.0f;

    for (uint8 i = 0; i < NUM_LEVEL_BRACKETS; ++i)
    {
        PoolBracket bracket = static_cast<PoolBracket>(i);
        PoolBracketStats stats = GetBracketStatistics(bracket);
        float availPct = stats.GetAvailabilityPct();

        if (availPct < lowestPct)
        {
            lowestPct = availPct;
            mostDepleted = bracket;
        }
    }

    return mostDepleted;
}

// ============================================================================
// PER-BRACKET BOT SELECTION
// ============================================================================

ObjectGuid InstanceBotPool::SelectBestBotFromBracket(BotRole role, Faction faction, PoolBracket bracket)
{
    std::unique_lock indexLock(_readyIndexMutex);

    auto roleIdx = static_cast<size_t>(role);
    auto factionIdx = static_cast<size_t>(faction);
    auto bracketIdx = static_cast<size_t>(bracket);

    if (roleIdx >= static_cast<size_t>(BotRole::Max) ||
        factionIdx >= static_cast<size_t>(Faction::Max) ||
        bracketIdx >= NUM_LEVEL_BRACKETS)
    {
        return ObjectGuid::Empty;
    }

    std::vector<ObjectGuid>& bracketBots = _readyIndex[roleIdx][factionIdx][bracketIdx];

    if (bracketBots.empty())
        return ObjectGuid::Empty;

    // Take the first available bot (could add scoring later)
    ObjectGuid selected = bracketBots.back();
    bracketBots.pop_back();

    indexLock.unlock();

    // Update bracket counts
    {
        std::unique_lock bracketLock(_bracketCountsMutex);
        _bracketCounts.DecrementReady(bracket, faction, role);
    }

    return selected;
}

std::vector<ObjectGuid> InstanceBotPool::SelectBotsFromBracket(BotRole role, Faction faction,
                                                                PoolBracket bracket, uint32 count)
{
    std::vector<ObjectGuid> result;
    result.reserve(count);

    std::unique_lock indexLock(_readyIndexMutex);

    auto roleIdx = static_cast<size_t>(role);
    auto factionIdx = static_cast<size_t>(faction);
    auto bracketIdx = static_cast<size_t>(bracket);

    if (roleIdx >= static_cast<size_t>(BotRole::Max) ||
        factionIdx >= static_cast<size_t>(Faction::Max) ||
        bracketIdx >= NUM_LEVEL_BRACKETS)
    {
        TC_LOG_DEBUG("playerbot.pool", "SelectBotsFromBracket: Invalid indices - role={} faction={} bracket={}",
            roleIdx, factionIdx, bracketIdx);
        return result;
    }

    std::vector<ObjectGuid>& bracketBots = _readyIndex[roleIdx][factionIdx][bracketIdx];

    uint32 available = static_cast<uint32>(bracketBots.size());
    uint32 toSelect = std::min(count, available);

    // DIAGNOSTIC: Log ready index state
    TC_LOG_INFO("playerbot.pool", "SelectBotsFromBracket: role={} faction={} bracket={} requested={} available={} selecting={}",
        BotRoleToString(role), FactionToString(faction), static_cast<uint32>(bracket),
        count, available, toSelect);

    for (uint32 i = 0; i < toSelect; ++i)
    {
        result.push_back(bracketBots.back());
        bracketBots.pop_back();
    }

    indexLock.unlock();

    // Update bracket counts
    {
        std::unique_lock bracketLock(_bracketCountsMutex);
        for (uint32 i = 0; i < toSelect; ++i)
        {
            _bracketCounts.DecrementReady(bracket, faction, role);
        }
    }

    return result;
}

// ============================================================================
// READY INDEX MANAGEMENT
// ============================================================================

void InstanceBotPool::AddToReadyIndex(ObjectGuid botGuid, BotRole role, Faction faction, PoolBracket bracket)
{
    std::unique_lock lock(_readyIndexMutex);

    auto roleIdx = static_cast<size_t>(role);
    auto factionIdx = static_cast<size_t>(faction);
    auto bracketIdx = static_cast<size_t>(bracket);

    if (roleIdx >= static_cast<size_t>(BotRole::Max) ||
        factionIdx >= static_cast<size_t>(Faction::Max) ||
        bracketIdx >= NUM_LEVEL_BRACKETS)
    {
        return;
    }

    _readyIndex[roleIdx][factionIdx][bracketIdx].push_back(botGuid);
}

void InstanceBotPool::RemoveFromReadyIndex(ObjectGuid botGuid, BotRole role, Faction faction, PoolBracket bracket)
{
    std::unique_lock lock(_readyIndexMutex);

    auto roleIdx = static_cast<size_t>(role);
    auto factionIdx = static_cast<size_t>(faction);
    auto bracketIdx = static_cast<size_t>(bracket);

    if (roleIdx >= static_cast<size_t>(BotRole::Max) ||
        factionIdx >= static_cast<size_t>(Faction::Max) ||
        bracketIdx >= NUM_LEVEL_BRACKETS)
    {
        return;
    }

    auto& vec = _readyIndex[roleIdx][factionIdx][bracketIdx];
    vec.erase(std::remove(vec.begin(), vec.end(), botGuid), vec.end());
}

void InstanceBotPool::RegisterJITBot(ObjectGuid botGuid, uint32 accountId, BotRole role, Faction faction, PoolBracket bracket)
{
    // ========================================================================
    // FIX (2026-01-15): Properly integrate JIT-created bots with pool tracking
    //
    // Problem: The overflow callback only called AddToReadyIndex() without
    // updating _slots or _bracketCounts. This caused ReplenishPool() to
    // continuously detect shortages and create more JIT bots (1200 bots bug).
    //
    // Solution: This method performs the SAME tracking as CreatePoolBot():
    // 1. Create InstanceBotSlot entry
    // 2. Add to _slots map
    // 3. Add to ready index
    // 4. Update _bracketCounts (ready + total)
    // ========================================================================

    // Check if bot already registered (prevent double registration)
    {
        std::shared_lock lock(_slotsMutex);
        if (_slots.find(botGuid) != _slots.end())
        {
            TC_LOG_DEBUG("playerbot.pool", "InstanceBotPool::RegisterJITBot - Bot {} already registered",
                botGuid.ToString());
            return;
        }
    }

    // Get bot info from character cache
    CharacterCacheEntry const* charInfo = sCharacterCache->GetCharacterCacheByGuid(botGuid);
    std::string name = charInfo ? charInfo->Name : "JITBot";
    uint8 playerClass = charInfo ? charInfo->Class : 0;

    // Get level for this bracket (midpoint)
    uint32 level = GetBracketMidpointLevel(bracket);

    // Determine pool type
    PoolType poolType = (faction == Faction::Alliance) ? PoolType::PvP_Alliance : PoolType::PvP_Horde;

    // Create slot for the JIT-created bot
    InstanceBotSlot slot;
    slot.Initialize(botGuid, accountId, name, poolType, role);
    slot.level = level;
    slot.faction = faction;
    slot.gearScore = 0;
    slot.playerClass = playerClass;
    slot.specId = 0;
    slot.ForceState(PoolSlotState::Ready); // JIT bots are already logged in and ready

    // Add to slots map
    {
        std::unique_lock lock(_slotsMutex);
        _slots[botGuid] = std::move(slot);
    }

    // Add to ready index for O(1) lookup
    AddToReadyIndex(botGuid, role, faction, bracket);

    // Update bracket counts (CRITICAL - this was missing in the overflow callback!)
    {
        std::unique_lock lock(_bracketCountsMutex);
        _bracketCounts.IncrementReady(bracket, faction, role);
        _bracketCounts.IncrementTotal(bracket, faction);
    }

    _statsDirty.store(true);

    uint32 minLevel, maxLevel;
    GetBracketLevelRange(bracket, minLevel, maxLevel);

    TC_LOG_INFO("playerbot.pool", "InstanceBotPool::RegisterJITBot - Registered JIT bot {} ({}), Role {}, Bracket {}-{}, Level {}",
        name, botGuid.ToString(), BotRoleToString(role), minLevel, maxLevel, level);
}

void InstanceBotPool::RebuildReadyIndex()
{
    TC_LOG_INFO("playerbot.pool", "Rebuilding ready index from {} slots...", _slots.size());

    // Clear existing index
    {
        std::unique_lock lock(_readyIndexMutex);
        for (auto& roleMap : _readyIndex)
            for (auto& factionMap : roleMap)
                for (auto& bracketVec : factionMap)
                    bracketVec.clear();
    }

    // Clear bracket counts
    {
        std::unique_lock lock(_bracketCountsMutex);
        _bracketCounts.Reset();
    }

    // Rebuild from slots
    std::shared_lock slotLock(_slotsMutex);
    uint32 readyCount = 0;

    for (auto const& [guid, slot] : _slots)
    {
        // Update total counts
        PoolBracket bracket = GetBracketForLevel(slot.level);
        {
            std::unique_lock lock(_bracketCountsMutex);
            _bracketCounts.IncrementTotal(bracket, slot.faction);
        }

        if (slot.state != PoolSlotState::Ready)
            continue;

        // Add to ready index
        AddToReadyIndex(guid, slot.role, slot.faction, bracket);

        // Update ready counts
        {
            std::unique_lock lock(_bracketCountsMutex);
            _bracketCounts.IncrementReady(bracket, slot.faction, slot.role);
        }

        ++readyCount;
    }

    TC_LOG_INFO("playerbot.pool", "Ready index rebuilt: {} ready bots indexed", readyCount);
}

void InstanceBotPool::UpdateBracketCounts()
{
    // Recalculate bracket counts from slots
    std::unique_lock bracketLock(_bracketCountsMutex);
    _bracketCounts.Reset();

    std::shared_lock slotLock(_slotsMutex);
    for (auto const& [guid, slot] : _slots)
    {
        PoolBracket bracket = GetBracketForLevel(slot.level);
        _bracketCounts.IncrementTotal(bracket, slot.faction);

        if (slot.state == PoolSlotState::Ready)
        {
            _bracketCounts.IncrementReady(bracket, slot.faction, slot.role);
        }
    }
}

// ============================================================================
// STRING UTILITIES
// ============================================================================

std::string PoolStatistics::ToSummaryString() const
{
    return fmt::format(
        "Pool: {} total, {} ready ({:.1f}%), {} assigned ({:.1f}%)",
        slotStats.GetTotal(),
        slotStats.readySlots,
        slotStats.GetAvailabilityPct(),
        slotStats.assignedSlots,
        slotStats.GetUtilizationPct());
}

std::string PoolStatistics::ToDetailedString() const
{
    return fmt::format(
        "Pool Statistics:\n"
        "  Slots: total={}, empty={}, creating={}, warming={}, ready={}, reserved={}, assigned={}, cooldown={}, maintenance={}\n"
        "  Activity: assignments={}, releases={}, jit={}, reservations={}, cancels={}\n"
        "  Timing: avg_assign={}µs, avg_warmup={}ms, peak_assign={}µs\n"
        "  Success: {:.1f}%",
        slotStats.GetTotal(),
        slotStats.emptySlots, slotStats.creatingSlots, slotStats.warmingSlots,
        slotStats.readySlots, slotStats.reservedSlots, slotStats.assignedSlots,
        slotStats.cooldownSlots, slotStats.maintenanceSlots,
        activity.assignmentsThisHour, activity.releasesThisHour, activity.jitCreationsThisHour,
        activity.reservationsThisHour, activity.cancellationsThisHour,
        timing.avgAssignmentTime.count(), timing.avgWarmupTime.count(), timing.peakAssignmentTime.count(),
        activity.GetSuccessRatePct());
}

void PoolStatistics::PrintToLog() const
{
    TC_LOG_INFO("playerbot.pool", "{}", ToDetailedString());
}

// ============================================================================
// CONFIGURATION LOADING
// ============================================================================

void InstanceBotPoolConfig::LoadFromConfig()
{
    // Delegate to pool's LoadConfig
}

bool InstanceBotPoolConfig::Validate() const
{
    // Validate pool sizes are reasonable
    if (poolSize.GetTotalWarmPool() > 10000)
    {
        TC_LOG_WARN("playerbot.pool", "Total pool size {} exceeds recommended maximum of 10000",
            poolSize.GetTotalWarmPool());
    }

    // Validate timing
    if (timing.cooldownDuration < std::chrono::seconds(30))
    {
        TC_LOG_WARN("playerbot.pool", "Cooldown duration {}s is very short, may cause thrashing",
            timing.cooldownDuration.count());
    }

    return true;
}

InstanceBotPoolConfig InstanceBotPoolConfig::GetDefault()
{
    return InstanceBotPoolConfig();
}

void InstanceBotPoolConfig::PrintToLog() const
{
    TC_LOG_INFO("playerbot.pool", "Instance Bot Pool Configuration:");
    TC_LOG_INFO("playerbot.pool", "  Enabled: {}", enabled);
    TC_LOG_INFO("playerbot.pool", "  Alliance: T={}, H={}, D={}",
        poolSize.allianceTanks, poolSize.allianceHealers, poolSize.allianceDPS);
    TC_LOG_INFO("playerbot.pool", "  Horde: T={}, H={}, D={}",
        poolSize.hordeTanks, poolSize.hordeHealers, poolSize.hordeDPS);
    TC_LOG_INFO("playerbot.pool", "  Cooldown: {}s", timing.cooldownDuration.count());
}

void JITFactoryConfig::LoadFromConfig()
{
    enabled = sPlayerbotConfig->GetBool("Playerbot.Instance.JIT.Enable", true);
    maxConcurrentCreations = sPlayerbotConfig->GetInt("Playerbot.Instance.JIT.MaxConcurrentCreations", 10);
    recycleTimeoutMinutes = sPlayerbotConfig->GetInt("Playerbot.Instance.JIT.RecycleTimeoutMinutes", 5);
    maxRecycledBots = sPlayerbotConfig->GetInt("Playerbot.Instance.JIT.MaxRecycledBots", 100);
}

void InstanceOrchestratorConfig::LoadFromConfig()
{
    enabled = sPlayerbotConfig->GetBool("Playerbot.Instance.Orchestrator.Enable", true);
    dungeonTimeoutMs = sPlayerbotConfig->GetInt("Playerbot.Instance.Orchestrator.DungeonTimeoutMs", 30000);
    raidTimeoutMs = sPlayerbotConfig->GetInt("Playerbot.Instance.Orchestrator.RaidTimeoutMs", 60000);
    bgTimeoutMs = sPlayerbotConfig->GetInt("Playerbot.Instance.Orchestrator.BattlegroundTimeoutMs", 120000);
    arenaTimeoutMs = sPlayerbotConfig->GetInt("Playerbot.Instance.Orchestrator.ArenaTimeoutMs", 15000);
    useOverflowThresholdPct = sPlayerbotConfig->GetInt("Playerbot.Instance.Orchestrator.OverflowThresholdPct", 80);
}

} // namespace Playerbot
