/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "QueueStatePoller.h"
#include "InstanceBotPool.h"
#include "PoolConfiguration.h"
#include "JITBotFactory.h"
#include "PvP/BGBotManager.h"
#include "LFG/LFGBotManager.h"
#include "LFG/LFGRoleDetector.h"
#include "Session/BotWorldSessionMgr.h"  // CRITICAL FIX (2026-01-22): For MarkAsInstanceBot
#include "BattlegroundMgr.h"
#include "BattlegroundQueue.h"
#include "ObjectAccessor.h"
#include "LFGMgr.h"
#include "Player.h"
#include "Log.h"
#include "DB2Stores.h"  // For BattlemasterList DBC lookup (BG player counts)

namespace Playerbot
{

// ============================================================================
// BGQueueSnapshot IMPLEMENTATION
// ============================================================================

BGQueueSnapshot::BGQueueSnapshot()
    : bgTypeId(BATTLEGROUND_TYPE_NONE)
    , bracketId(BG_BRACKET_ID_FIRST)
    , allianceCount(0)
    , hordeCount(0)
    , minPlayersPerTeam(0)
    , maxPlayersPerTeam(0)
    , allianceShortage(0)
    , hordeShortage(0)
    , timestamp(0)
{
}

uint32 BGQueueSnapshot::GetTotalShortage() const
{
    uint32 total = 0;
    if (allianceShortage > 0)
        total += static_cast<uint32>(allianceShortage);
    if (hordeShortage > 0)
        total += static_cast<uint32>(hordeShortage);
    return total;
}

// ============================================================================
// LFGQueueSnapshot IMPLEMENTATION
// ============================================================================

LFGQueueSnapshot::LFGQueueSnapshot()
    : dungeonId(0)
    , minLevel(0)
    , maxLevel(0)
    , tankCount(0)
    , healerCount(0)
    , dpsCount(0)
    , tankNeeded(1)
    , healerNeeded(1)
    , dpsNeeded(3)
    , timestamp(0)
{
}

bool LFGQueueSnapshot::HasShortage() const
{
    return (tankCount < tankNeeded) ||
           (healerCount < healerNeeded) ||
           (dpsCount < dpsNeeded);
}

uint8 LFGQueueSnapshot::GetMostUrgentRole() const
{
    // Priority: tank > healer > dps
    if (tankCount < tankNeeded)
        return 0;  // Tank
    if (healerCount < healerNeeded)
        return 1;  // Healer
    if (dpsCount < dpsNeeded)
        return 2;  // DPS
    return 255;    // None needed
}

// ============================================================================
// ArenaQueueSnapshot IMPLEMENTATION
// ============================================================================

ArenaQueueSnapshot::ArenaQueueSnapshot()
    : arenaType(0)
    , bracketId(BG_BRACKET_ID_FIRST)
    , allianceTeamsInQueue(0)
    , hordeTeamsInQueue(0)
    , playersPerTeam(0)
    , timestamp(0)
{
}

bool ArenaQueueSnapshot::HasShortage() const
{
    // Arena needs at least one team from each faction (or same faction for skirmish)
    return (allianceTeamsInQueue == 0 || hordeTeamsInQueue == 0);
}

// ============================================================================
// QueueStatePoller IMPLEMENTATION
// ============================================================================

QueueStatePoller::QueueStatePoller()
    : _enabled(true)
    , _pollingInterval(DEFAULT_POLLING_INTERVAL)
    , _jitThrottleSeconds(DEFAULT_JIT_THROTTLE_SECONDS)
    , _updateAccumulator(0)
    , _pollCount(0)
    , _bgShortagesDetected(0)
    , _lfgShortagesDetected(0)
    , _arenaShortagesDetected(0)
    , _jitRequestsTriggered(0)
{
}

QueueStatePoller::~QueueStatePoller() = default;

QueueStatePoller* QueueStatePoller::Instance()
{
    static QueueStatePoller instance;
    return &instance;
}

bool QueueStatePoller::Initialize()
{
    TC_LOG_INFO("playerbot.jit", "QueueStatePoller: Initialized (polling interval: {}ms, JIT throttle: {}s)",
        _pollingInterval, _jitThrottleSeconds);
    return true;
}

void QueueStatePoller::Shutdown()
{
    std::lock_guard<decltype(_mutex)> lock(_mutex);

    _activeBGQueues.clear();
    _activeLFGQueues.clear();
    _activeArenaQueues.clear();
    _bgSnapshots.clear();
    _lfgSnapshots.clear();
    _arenaSnapshots.clear();
    _lastJITTime.clear();
    _lfgQueueInfo.clear();
    _pendingBGVerifications.clear();

    TC_LOG_INFO("playerbot.jit", "QueueStatePoller: Shutdown complete");
}

void QueueStatePoller::Update(uint32 diff)
{
    if (!_enabled)
        return;

    _updateAccumulator += diff;
    if (_updateAccumulator < _pollingInterval)
        return;

    _updateAccumulator = 0;
    ++_pollCount;

    // Process pending BG verifications before polling
    // This checks if warm pool bots actually queued successfully
    ProcessPendingBGVerifications();

    // Poll all active queues
    PollBGQueues();
    PollLFGQueues();
    PollArenaQueues();
}

// ============================================================================
// QUEUE REGISTRATION
// ============================================================================

void QueueStatePoller::RegisterActiveBGQueue(BattlegroundTypeId bgTypeId, BattlegroundBracketId bracket)
{
    std::lock_guard<decltype(_mutex)> lock(_mutex);

    uint64 key = MakeBGKey(bgTypeId, bracket);
    _activeBGQueues.insert(key);

    TC_LOG_DEBUG("playerbot.jit", "QueueStatePoller: Registered active BG queue (type={}, bracket={})",
        static_cast<uint32>(bgTypeId), static_cast<uint32>(bracket));

    // Trigger immediate poll for this queue
    DoPollBGQueue(bgTypeId, bracket);
}

void QueueStatePoller::UnregisterActiveBGQueue(BattlegroundTypeId bgTypeId, BattlegroundBracketId bracket)
{
    std::lock_guard<decltype(_mutex)> lock(_mutex);

    uint64 key = MakeBGKey(bgTypeId, bracket);
    _activeBGQueues.erase(key);
    _bgSnapshots.erase(key);
    _lastJITTime.erase(key);
    _pendingBGVerifications.erase(key);

    TC_LOG_DEBUG("playerbot.jit", "QueueStatePoller: Unregistered BG queue (type={}, bracket={})",
        static_cast<uint32>(bgTypeId), static_cast<uint32>(bracket));
}

void QueueStatePoller::RegisterActiveLFGQueue(uint32 dungeonId, uint8 minLevel, uint8 maxLevel, uint8 humanPlayerLevel)
{
    std::lock_guard<decltype(_mutex)> lock(_mutex);

    _activeLFGQueues.insert(dungeonId);
    _lfgQueueInfo[dungeonId] = {minLevel, maxLevel, humanPlayerLevel};

    TC_LOG_INFO("playerbot.jit", "QueueStatePoller: Registered active LFG queue (dungeon={}, levels={}-{}, humanLevel={})",
        dungeonId, minLevel, maxLevel, humanPlayerLevel);

    // Trigger immediate poll
    DoPollLFGQueue(dungeonId, minLevel, maxLevel);
}

void QueueStatePoller::RegisterActiveLFGQueue(uint32 dungeonId, uint8 minLevel, uint8 maxLevel)
{
    // DEPRECATED: Calls new overload with humanPlayerLevel=0 (will use dungeon average as fallback)
    RegisterActiveLFGQueue(dungeonId, minLevel, maxLevel, 0);
}

void QueueStatePoller::RegisterActiveLFGQueue(uint32 dungeonId)
{
    // Use expansion-based level ranges as defaults
    // The exact level range isn't critical for JIT bot creation
    // Bots will be created with appropriate levels based on the content
    uint8 minLevel = 10;
    uint8 maxLevel = 80;

    // Estimate level range based on dungeon ID ranges
    // Classic dungeons: ~1-999, TBC: ~1000-1999, WotLK: ~2000-2999, etc.
    if (dungeonId < 1000)
    {
        minLevel = 15;
        maxLevel = 60;
    }
    else if (dungeonId < 2000)
    {
        minLevel = 58;
        maxLevel = 70;
    }
    else if (dungeonId < 3000)
    {
        minLevel = 68;
        maxLevel = 80;
    }
    else
    {
        // Modern content - use dynamic level range
        minLevel = 10;
        maxLevel = 80;
    }

    TC_LOG_DEBUG("playerbot.jit", "QueueStatePoller: Registering LFG queue {} with estimated levels {}-{}",
        dungeonId, minLevel, maxLevel);

    // Call the full implementation
    RegisterActiveLFGQueue(dungeonId, minLevel, maxLevel);
}

void QueueStatePoller::UnregisterActiveLFGQueue(uint32 dungeonId)
{
    std::lock_guard<decltype(_mutex)> lock(_mutex);

    _activeLFGQueues.erase(dungeonId);
    _lfgSnapshots.erase(dungeonId);
    _lfgQueueInfo.erase(dungeonId);
    _lastJITTime.erase(dungeonId);  // Using dungeonId as key for LFG

    TC_LOG_DEBUG("playerbot.jit", "QueueStatePoller: Unregistered LFG queue (dungeon={})", dungeonId);
}

void QueueStatePoller::RegisterActiveArenaQueue(uint8 arenaType, BattlegroundBracketId bracket)
{
    std::lock_guard<decltype(_mutex)> lock(_mutex);

    uint64 key = MakeArenaKey(arenaType, bracket);
    _activeArenaQueues.insert(key);

    TC_LOG_DEBUG("playerbot.jit", "QueueStatePoller: Registered active Arena queue (type={}, bracket={})",
        arenaType, static_cast<uint32>(bracket));

    // Trigger immediate poll
    DoPollArenaQueue(arenaType, bracket);
}

void QueueStatePoller::UnregisterActiveArenaQueue(uint8 arenaType, BattlegroundBracketId bracket)
{
    std::lock_guard<decltype(_mutex)> lock(_mutex);

    uint64 key = MakeArenaKey(arenaType, bracket);
    _activeArenaQueues.erase(key);
    _arenaSnapshots.erase(key);
    _lastJITTime.erase(key);

    TC_LOG_DEBUG("playerbot.jit", "QueueStatePoller: Unregistered Arena queue (type={}, bracket={})",
        arenaType, static_cast<uint32>(bracket));
}

// ============================================================================
// LFG ROLE COUNT UPDATES
// ============================================================================

void QueueStatePoller::UpdateLFGRoleCount(uint32 dungeonId, uint8 role, bool increment)
{
    std::lock_guard<decltype(_mutex)> lock(_mutex);

    // Create snapshot if it doesn't exist
    LFGQueueSnapshot& snapshot = _lfgSnapshots[dungeonId];
    snapshot.dungeonId = dungeonId;
    snapshot.timestamp = time(nullptr);

    // Update the appropriate role count
    // Role values: PLAYER_ROLE_TANK = 2, PLAYER_ROLE_HEALER = 4, PLAYER_ROLE_DAMAGE = 8
    if (role & 2)  // PLAYER_ROLE_TANK
    {
        if (increment)
            ++snapshot.tankCount;
        else if (snapshot.tankCount > 0)
            --snapshot.tankCount;

        TC_LOG_INFO("playerbot.jit", "QueueStatePoller: Updated LFG Tank count for dungeon {} to {}/{}",
            dungeonId, snapshot.tankCount, snapshot.tankNeeded);
    }
    else if (role & 4)  // PLAYER_ROLE_HEALER
    {
        if (increment)
            ++snapshot.healerCount;
        else if (snapshot.healerCount > 0)
            --snapshot.healerCount;

        TC_LOG_INFO("playerbot.jit", "QueueStatePoller: Updated LFG Healer count for dungeon {} to {}/{}",
            dungeonId, snapshot.healerCount, snapshot.healerNeeded);
    }
    else if (role & 8)  // PLAYER_ROLE_DAMAGE
    {
        if (increment)
            ++snapshot.dpsCount;
        else if (snapshot.dpsCount > 0)
            --snapshot.dpsCount;

        TC_LOG_INFO("playerbot.jit", "QueueStatePoller: Updated LFG DPS count for dungeon {} to {}/{}",
            dungeonId, snapshot.dpsCount, snapshot.dpsNeeded);
    }

    TC_LOG_INFO("playerbot.jit", "QueueStatePoller: LFG Queue Status for dungeon {}: T:{}/{} H:{}/{} D:{}/{} (HasShortage={})",
        dungeonId,
        snapshot.tankCount, snapshot.tankNeeded,
        snapshot.healerCount, snapshot.healerNeeded,
        snapshot.dpsCount, snapshot.dpsNeeded,
        snapshot.HasShortage() ? "YES" : "no");
}

void QueueStatePoller::SetLFGNeededCounts(uint32 dungeonId, uint32 tanksNeeded, uint32 healersNeeded, uint32 dpsNeeded)
{
    std::lock_guard<decltype(_mutex)> lock(_mutex);

    // Create snapshot if it doesn't exist
    LFGQueueSnapshot& snapshot = _lfgSnapshots[dungeonId];
    snapshot.dungeonId = dungeonId;
    snapshot.tankNeeded = tanksNeeded;
    snapshot.healerNeeded = healersNeeded;
    snapshot.dpsNeeded = dpsNeeded;
    snapshot.timestamp = time(nullptr);

    TC_LOG_INFO("playerbot.jit", "QueueStatePoller: Set LFG needed counts for dungeon {}: {} tanks, {} healers, {} DPS",
        dungeonId, tanksNeeded, healersNeeded, dpsNeeded);
}

// ============================================================================
// MANUAL POLL TRIGGERS
// ============================================================================

void QueueStatePoller::PollBGQueues()
{
    std::lock_guard<decltype(_mutex)> lock(_mutex);

    // Copy the set before iterating - ProcessBGShortage may unregister
    // satisfied queues during the loop, which would invalidate iterators
    auto activeQueues = _activeBGQueues;
    for (uint64 key : activeQueues)
    {
        // Skip if unregistered during this poll cycle
        if (_activeBGQueues.find(key) == _activeBGQueues.end())
            continue;

        BattlegroundTypeId bgTypeId = static_cast<BattlegroundTypeId>(key >> 32);
        BattlegroundBracketId bracket = static_cast<BattlegroundBracketId>(key & 0xFFFFFFFF);
        DoPollBGQueue(bgTypeId, bracket);
    }
}

void QueueStatePoller::PollLFGQueues()
{
    std::lock_guard<decltype(_mutex)> lock(_mutex);

    for (uint32 dungeonId : _activeLFGQueues)
    {
        auto it = _lfgQueueInfo.find(dungeonId);
        if (it != _lfgQueueInfo.end())
        {
            DoPollLFGQueue(dungeonId, it->second.minLevel, it->second.maxLevel);
        }
    }
}

void QueueStatePoller::PollArenaQueues()
{
    std::lock_guard<decltype(_mutex)> lock(_mutex);

    for (uint64 key : _activeArenaQueues)
    {
        uint8 arenaType = static_cast<uint8>(key >> 32);
        BattlegroundBracketId bracket = static_cast<BattlegroundBracketId>(key & 0xFFFFFFFF);
        DoPollArenaQueue(arenaType, bracket);
    }
}

// ============================================================================
// INTERNAL POLL METHODS
// ============================================================================

void QueueStatePoller::DoPollBGQueue(BattlegroundTypeId bgTypeId, BattlegroundBracketId bracket)
{
    // Get queue from BattlegroundMgr (READ-ONLY access)
    BattlegroundQueueTypeId queueTypeId = BattlegroundMgr::BGQueueTypeId(
        static_cast<uint16>(bgTypeId),
        BattlegroundQueueIdType::Battleground,
        false,  // not rated
        0       // team size N/A for BGs
    );

    if (!BattlegroundMgr::IsValidQueueId(queueTypeId))
    {
        TC_LOG_DEBUG("playerbot.jit", "QueueStatePoller: Invalid queue ID for BG type {}", static_cast<uint32>(bgTypeId));
        return;
    }

    BattlegroundQueue& queue = sBattlegroundMgr->GetBattlegroundQueue(queueTypeId);

    // Read queue counts using the correct function that counts from m_QueuedGroups
    // CRITICAL FIX: GetPlayersInQueue() returns from selection pool (matchmaking only)
    // GetQueuedPlayersCount() returns actual queued players for the specific bracket
    uint32 allianceCount = queue.GetQueuedPlayersCount(TEAM_ALLIANCE, bracket);
    uint32 hordeCount = queue.GetQueuedPlayersCount(TEAM_HORDE, bracket);

    // Get requirements from template (READ-ONLY)
    BattlegroundTemplate const* bgTemplate = sBattlegroundMgr->GetBattlegroundTemplateByTypeId(bgTypeId);
    if (!bgTemplate)
    {
        TC_LOG_DEBUG("playerbot.jit", "QueueStatePoller: No template for BG type {}", static_cast<uint32>(bgTypeId));
        return;
    }

    uint32 minPlayers = bgTemplate->GetMinPlayersPerTeam();
    uint32 maxPlayers = bgTemplate->GetMaxPlayersPerTeam();

    // ========================================================================
    // FIX: Handle BGs with missing/zero player counts in battleground_template
    // Many BGs (Temple of Kotmogu, Silvershard Mines, newer Warsong Gulch, etc.)
    // have MinPlayersPerTeam=0 and MaxPlayersPerTeam=0 in battleground_template.
    // Fall back to BattlemasterList DBC which has authoritative player counts.
    // ========================================================================
    if (maxPlayers == 0)
    {
        // Primary fallback: BGBotManager has explicit team sizes for all supported BGs
        maxPlayers = sBGBotManager->GetBGTeamSize(bgTypeId);
        if (maxPlayers > 0)
        {
            minPlayers = maxPlayers;
            TC_LOG_INFO("playerbot.jit",
                "QueueStatePoller: BG {} using BGBotManager team size {}v{}",
                static_cast<uint32>(bgTypeId), maxPlayers, maxPlayers);
        }
        else
        {
            // Secondary fallback: BattlemasterList DBC
            // NOTE: BattlemasterListEntry::MaxPlayers is ALREADY per-team
            // (GetMaxPlayersPerTeam() returns it directly without division)
            BattlemasterListEntry const* bgEntry = sBattlemasterListStore.LookupEntry(
                static_cast<uint32>(bgTypeId));
            if (bgEntry && bgEntry->MaxPlayers > 0)
            {
                maxPlayers = static_cast<uint32>(bgEntry->MaxPlayers);
                minPlayers = maxPlayers;

                TC_LOG_INFO("playerbot.jit",
                    "QueueStatePoller: BG {} using DBC MaxPlayers={} (per-team)",
                    static_cast<uint32>(bgTypeId), maxPlayers);
            }
            else
            {
                TC_LOG_WARN("playerbot.jit",
                    "QueueStatePoller: BG {} has no player count in template, BGBotManager, or DBC, skipping",
                    static_cast<uint32>(bgTypeId));
                return;
            }
        }
    }

    // Build snapshot
    uint64 key = MakeBGKey(bgTypeId, bracket);
    BGQueueSnapshot& snapshot = _bgSnapshots[key];
    snapshot.bgTypeId = bgTypeId;
    snapshot.bracketId = bracket;
    snapshot.allianceCount = allianceCount;
    snapshot.hordeCount = hordeCount;
    snapshot.minPlayersPerTeam = minPlayers;
    snapshot.maxPlayersPerTeam = maxPlayers;
    // Use maxPlayers for shortage calculation - we want to fill the BG, not just meet minimum
    snapshot.allianceShortage = static_cast<int32>(maxPlayers) - static_cast<int32>(allianceCount);
    snapshot.hordeShortage = static_cast<int32>(maxPlayers) - static_cast<int32>(hordeCount);
    snapshot.timestamp = time(nullptr);

    TC_LOG_DEBUG("playerbot.jit", "QueueStatePoller: BG Poll - Type={} Bracket={} Alliance={}/{} Horde={}/{} Shortage=A:{}/H:{}",
        static_cast<uint32>(bgTypeId), static_cast<uint32>(bracket),
        allianceCount, maxPlayers, hordeCount, maxPlayers,
        snapshot.allianceShortage, snapshot.hordeShortage);

    // Process shortage if detected
    if (snapshot.HasShortage())
    {
        ProcessBGShortage(snapshot);
    }
}

void QueueStatePoller::DoPollLFGQueue(uint32 dungeonId, uint8 minLevel, uint8 maxLevel)
{
    // LFG queue polling is more complex and requires LFGMgr access
    // For now, we rely on packet sniffer and script hooks for LFG data
    // The snapshot will be updated by those systems

    // Create or update snapshot with basic info
    LFGQueueSnapshot& snapshot = _lfgSnapshots[dungeonId];
    snapshot.dungeonId = dungeonId;
    snapshot.minLevel = minLevel;
    snapshot.maxLevel = maxLevel;
    snapshot.timestamp = time(nullptr);

    // Note: Role counts are updated by packet handlers (ParseLFGPacket_Typed.cpp)
    // or by LFGBotManager when it receives queue updates

    TC_LOG_DEBUG("playerbot.jit", "QueueStatePoller: LFG Poll - Dungeon={} Levels={}-{} T:{}/{} H:{}/{} D:{}/{}",
        dungeonId, minLevel, maxLevel,
        snapshot.tankCount, snapshot.tankNeeded,
        snapshot.healerCount, snapshot.healerNeeded,
        snapshot.dpsCount, snapshot.dpsNeeded);

    if (snapshot.HasShortage())
    {
        ProcessLFGShortage(snapshot);
    }
}

void QueueStatePoller::DoPollArenaQueue(uint8 arenaType, BattlegroundBracketId bracket)
{
    // Arena queue polling - similar to BG but for arena-specific queue type
    // Arenas need teams from both factions (or same faction for skirmish)

    uint64 key = MakeArenaKey(arenaType, bracket);
    ArenaQueueSnapshot& snapshot = _arenaSnapshots[key];
    snapshot.arenaType = arenaType;
    snapshot.bracketId = bracket;
    snapshot.playersPerTeam = arenaType;  // 2v2, 3v3, or 5v5
    snapshot.timestamp = time(nullptr);

    // Note: Arena team counts are typically updated by packet handlers
    // or ArenaBotManager when queue events occur

    TC_LOG_DEBUG("playerbot.jit", "QueueStatePoller: Arena Poll - Type={}v{} Bracket={} Alliance={} Horde={}",
        arenaType, arenaType, static_cast<uint32>(bracket),
        snapshot.allianceTeamsInQueue, snapshot.hordeTeamsInQueue);

    if (snapshot.HasShortage())
    {
        ProcessArenaShortage(snapshot);
    }
}

// ============================================================================
// BG VERIFICATION
// ============================================================================

void QueueStatePoller::ProcessPendingBGVerifications()
{
    std::lock_guard<decltype(_mutex)> lock(_mutex);

    if (_pendingBGVerifications.empty())
        return;

    auto now = std::chrono::steady_clock::now();
    std::vector<uint64> completed;

    for (auto& [key, verification] : _pendingBGVerifications)
    {
        if (now < verification.verificationTime)
            continue;

        // Verification time reached - check actual queue counts
        BattlegroundTypeId bgTypeId = static_cast<BattlegroundTypeId>(key >> 32);
        BattlegroundBracketId bracket = static_cast<BattlegroundBracketId>(key & 0xFFFFFFFF);

        BattlegroundQueueTypeId queueTypeId = BattlegroundMgr::BGQueueTypeId(
            static_cast<uint16>(bgTypeId),
            BattlegroundQueueIdType::Battleground,
            false, 0);

        if (!BattlegroundMgr::IsValidQueueId(queueTypeId))
        {
            TC_LOG_WARN("playerbot.jit",
                "QueueStatePoller: Verification skipped - invalid queue ID for BG type {}",
                static_cast<uint32>(bgTypeId));
            completed.push_back(key);
            continue;
        }

        BattlegroundQueue& queue = sBattlegroundMgr->GetBattlegroundQueue(queueTypeId);
        uint32 allianceCount = queue.GetQueuedPlayersCount(TEAM_ALLIANCE, bracket);
        uint32 hordeCount = queue.GetQueuedPlayersCount(TEAM_HORDE, bracket);

        BattlegroundTemplate const* bgTemplate = sBattlegroundMgr->GetBattlegroundTemplateByTypeId(bgTypeId);
        uint32 teamSize = bgTemplate ? bgTemplate->GetMaxPlayersPerTeam() : 0;
        if (teamSize == 0)
            teamSize = sBGBotManager->GetBGTeamSize(bgTypeId);

        TC_LOG_INFO("playerbot.jit",
            "QueueStatePoller: BG verification - Type={} Alliance={}/{} Horde={}/{} (expected warm pool: A:{} H:{})",
            static_cast<uint32>(bgTypeId), allianceCount, teamSize, hordeCount, teamSize,
            verification.expectedAlliance, verification.expectedHorde);

        bool allianceSatisfied = (teamSize == 0) || (allianceCount >= teamSize);
        bool hordeSatisfied = (teamSize == 0) || (hordeCount >= teamSize);

        if (allianceSatisfied && hordeSatisfied)
        {
            TC_LOG_INFO("playerbot.jit",
                "QueueStatePoller: BG verification PASSED - both teams have enough players, queue satisfied");
            completed.push_back(key);
        }
        else
        {
            // Not enough bots actually queued - re-register the queue for another poll cycle
            TC_LOG_WARN("playerbot.jit",
                "QueueStatePoller: BG verification FAILED - re-registering queue for BG type {} bracket {} "
                "(Alliance {}/{}, Horde {}/{})",
                static_cast<uint32>(bgTypeId), static_cast<uint32>(bracket),
                allianceCount, teamSize, hordeCount, teamSize);

            // Re-register the queue so the next poll cycle detects the shortage
            // and spawns additional bots (via JIT or warm pool)
            _activeBGQueues.insert(key);

            // Clear the JIT throttle so the shortage can be re-processed immediately
            _lastJITTime.erase(key);

            completed.push_back(key);
        }
    }

    for (uint64 key : completed)
        _pendingBGVerifications.erase(key);
}

// ============================================================================
// SHORTAGE PROCESSING
// ============================================================================

void QueueStatePoller::ProcessBGShortage(BGQueueSnapshot const& snapshot)
{
    uint64 key = MakeBGKey(snapshot.bgTypeId, snapshot.bracketId);

    // Check throttling
    if (IsJITThrottled(key))
    {
        TC_LOG_DEBUG("playerbot.jit", "QueueStatePoller: BG shortage throttled for type={}", static_cast<uint32>(snapshot.bgTypeId));
        return;
    }

    ++_bgShortagesDetected;

    TC_LOG_INFO("playerbot.jit", "QueueStatePoller: BG Shortage detected - Type={} Alliance need={} Horde need={}",
        static_cast<uint32>(snapshot.bgTypeId), snapshot.allianceShortage, snapshot.hordeShortage);

    // ========================================================================
    // STEP 1: TRY WARM POOL FIRST
    // Warm pool bots are pre-created and ready for instant assignment
    // Only fall back to JIT if warm pool doesn't have enough bots
    // ========================================================================

    uint32 allianceFromPool = 0;
    uint32 hordeFromPool = 0;
    uint32 allianceStillNeeded = snapshot.allianceShortage > 0 ? static_cast<uint32>(snapshot.allianceShortage) : 0;
    uint32 hordeStillNeeded = snapshot.hordeShortage > 0 ? static_cast<uint32>(snapshot.hordeShortage) : 0;

    // Get level from bracket for warm pool assignment
    // Use the midpoint of the bracket's level range
    uint32 minLevel, maxLevel;
    GetBracketLevelRange(static_cast<PoolBracket>(snapshot.bracketId), minLevel, maxLevel);
    uint32 bracketLevel = (minLevel + maxLevel) / 2;

    // CRITICAL FIX (2026-02-05): Get the human player that triggered this BG queue
    // This is required for proper BG invitation tracking - warm pool bots need to
    // use QueueBotForBGWithTracking instead of QueueBotForBG so they appear in
    // _queuedBots and can auto-accept invitations via ProcessPendingInvitations.
    ObjectGuid humanPlayerGuid = sBGBotManager->GetQueuedHumanForBG(
        snapshot.bgTypeId, snapshot.bracketId);

    if (!humanPlayerGuid.IsEmpty())
    {
        TC_LOG_INFO("playerbot.jit", "QueueStatePoller: Found human player {} for BG queue tracking",
            humanPlayerGuid.ToString());
    }
    else
    {
        TC_LOG_WARN("playerbot.jit", "QueueStatePoller: No human player found for BG type {} bracket {} - bots may not receive invitations",
            static_cast<uint32>(snapshot.bgTypeId), static_cast<uint32>(snapshot.bracketId));
    }

    if (allianceStillNeeded > 0 || hordeStillNeeded > 0)
    {
        // Try to get bots from the warm pool
        BGAssignment poolAssignment = sInstanceBotPool->AssignForBattleground(
            static_cast<uint32>(snapshot.bgTypeId),
            bracketLevel,
            allianceStillNeeded,
            hordeStillNeeded,
            humanPlayerGuid  // Pass human GUID for invitation tracking
        );

        if (poolAssignment.success)
        {
            allianceFromPool = static_cast<uint32>(poolAssignment.allianceBots.size());
            hordeFromPool = static_cast<uint32>(poolAssignment.hordeBots.size());

            TC_LOG_INFO("playerbot.jit", "QueueStatePoller: Got {}/{} Alliance and {}/{} Horde from warm pool",
                allianceFromPool, allianceStillNeeded, hordeFromPool, hordeStillNeeded);

            // ========================================================================
            // WARM POOL BOT BG QUEUEING - HANDLED BY POOL WORKFLOW
            // ========================================================================
            // DO NOT try to queue warm pool bots immediately here!
            //
            // Warm pool bots are NOT logged in at this point. They exist as database
            // records and are being logged in asynchronously via WarmUpBot(). The
            // queueing workflow is:
            //
            // 1. AssignForBattleground() -> selects bots from ready index
            // 2. AssignBot() -> stores contentId and calls WarmUpBot()
            // 3. WarmUpBot() -> sets pendingConfig.battlegroundIdToQueue and spawns bot
            // 4. Bot logs in -> BotPostLoginConfigurator::ApplyPendingConfiguration()
            // 5. ApplyPendingConfiguration() -> calls sBGBotManager->QueueBotForBG()
            //
            // Previously, we tried to use ObjectAccessor::FindPlayer() here, but it
            // returned nullptr because bots weren't logged in yet. This silently
            // failed and warm pool bots never queued for BG (the "warm bots not work"
            // bug). JIT bots worked because they're logged in FIRST, then queued.
            //
            // The markAsInstanceBot flag is now set via pendingConfig.markAsInstanceBot
            // in WarmUpBot() and applied by BotPostLoginConfigurator.
            // ========================================================================
            TC_LOG_DEBUG("playerbot.jit", "QueueStatePoller: Warm pool bots will queue for BG after login via BotPostLoginConfigurator");

            // Update remaining needs
            allianceStillNeeded = allianceStillNeeded > allianceFromPool ? allianceStillNeeded - allianceFromPool : 0;
            hordeStillNeeded = hordeStillNeeded > hordeFromPool ? hordeStillNeeded - hordeFromPool : 0;
        }
        else
        {
            TC_LOG_DEBUG("playerbot.jit", "QueueStatePoller: Warm pool assignment failed: {}",
                poolAssignment.errorMessage);
        }
    }

    // If warm pool fully satisfied the demand, schedule verification instead of
    // immediately unregistering. Warm pool bots login asynchronously and some may
    // fail to queue (e.g. dead bots, deserted bots). We verify after 30s that
    // enough bots actually entered the queue, and re-poll if not.
    if (allianceStillNeeded == 0 && hordeStillNeeded == 0)
    {
        RecordJITRequest(key);

        // Check if there's already a pending verification for this key
        if (_pendingBGVerifications.find(key) != _pendingBGVerifications.end())
        {
            TC_LOG_INFO("playerbot.jit",
                "QueueStatePoller: BG shortage satisfied from warm pool - verification already pending");
            _activeBGQueues.erase(key);
            _bgSnapshots.erase(key);
            return;
        }

        // Schedule a verification re-poll to confirm bots actually queued
        BGVerificationEntry verification;
        verification.verificationTime = std::chrono::steady_clock::now()
            + std::chrono::seconds(BG_VERIFICATION_DELAY_SECONDS);
        verification.expectedAlliance = allianceFromPool;
        verification.expectedHorde = hordeFromPool;
        _pendingBGVerifications[key] = verification;

        TC_LOG_INFO("playerbot.jit",
            "QueueStatePoller: BG shortage claimed by warm pool (A:{} H:{}) - "
            "scheduling verification in {}s instead of immediate unregister",
            allianceFromPool, hordeFromPool, BG_VERIFICATION_DELAY_SECONDS);

        // Unregister from active polling to prevent over-spawning, but
        // the verification entry will re-register if bots didn't actually queue
        _activeBGQueues.erase(key);
        _bgSnapshots.erase(key);
        return;
    }

    // ========================================================================
    // STEP 2: JIT CREATION FOR REMAINING SHORTAGE
    // Only create bots via JIT if warm pool couldn't satisfy demand
    // ========================================================================

    TC_LOG_INFO("playerbot.jit", "QueueStatePoller: Warm pool insufficient, requesting JIT for Alliance={} Horde={}",
        allianceStillNeeded, hordeStillNeeded);

    // Calculate priority based on queue fill rate
    // Higher fill = higher priority (closer to starting)
    float allianceFill = snapshot.minPlayersPerTeam > 0
        ? static_cast<float>(snapshot.allianceCount) / snapshot.minPlayersPerTeam
        : 0.0f;
    float hordeFill = snapshot.minPlayersPerTeam > 0
        ? static_cast<float>(snapshot.hordeCount) / snapshot.minPlayersPerTeam
        : 0.0f;
    float avgFill = (allianceFill + hordeFill) / 2.0f;

    // Priority 1-10, higher fill = lower number (higher priority)
    uint8 priority = static_cast<uint8>(std::clamp(10.0f - (avgFill * 9.0f), 1.0f, 10.0f));

    // Submit JIT requests for remaining shortage
    if (allianceStillNeeded > 0)
    {
        FactoryRequest request;
        request.instanceType = InstanceType::Battleground;
        request.contentId = static_cast<uint32>(snapshot.bgTypeId);
        request.playerFaction = Faction::Alliance;
        request.allianceNeeded = allianceStillNeeded;
        request.hordeNeeded = 0;
        request.priority = priority;
        request.createdAt = std::chrono::system_clock::now();

        // Set BG ID for post-login queueing
        // The BotPostLoginConfigurator will queue bots AFTER they're fully logged in
        // This avoids the timing issue where ObjectAccessor::FindPlayer returns nullptr
        // because the bots haven't entered the world yet when onComplete fires.
        request.battlegroundIdToQueue = static_cast<uint32>(snapshot.bgTypeId);

        // Callback for debugging (bots queue via BotPostLoginConfigurator, not here)
        request.onComplete = [bgTypeId = snapshot.bgTypeId](std::vector<ObjectGuid> const& botGuids) {
            TC_LOG_INFO("playerbot.jit", "QueueStatePoller: {} JIT Alliance bots created for BG {} - they will auto-queue after login",
                botGuids.size(), static_cast<uint32>(bgTypeId));
        };

        uint32 requestId = sJITBotFactory->SubmitRequest(std::move(request));
        if (requestId > 0)
        {
            ++_jitRequestsTriggered;
            TC_LOG_INFO("playerbot.jit", "QueueStatePoller: Submitted Alliance JIT request {} for {} bots (priority {})",
                requestId, allianceStillNeeded, priority);
        }
    }

    if (hordeStillNeeded > 0)
    {
        FactoryRequest request;
        request.instanceType = InstanceType::Battleground;
        request.contentId = static_cast<uint32>(snapshot.bgTypeId);
        request.playerFaction = Faction::Horde;
        request.allianceNeeded = 0;
        request.hordeNeeded = hordeStillNeeded;
        request.priority = priority;
        request.createdAt = std::chrono::system_clock::now();

        // Set BG ID for post-login queueing (same as Alliance - bracket determined by level)
        request.battlegroundIdToQueue = static_cast<uint32>(snapshot.bgTypeId);

        // Callback for debugging (bots queue via BotPostLoginConfigurator, not here)
        request.onComplete = [bgTypeId = snapshot.bgTypeId](std::vector<ObjectGuid> const& botGuids) {
            TC_LOG_INFO("playerbot.jit", "QueueStatePoller: {} JIT Horde bots created for BG {} - they will auto-queue after login",
                botGuids.size(), static_cast<uint32>(bgTypeId));
        };

        uint32 requestId = sJITBotFactory->SubmitRequest(std::move(request));
        if (requestId > 0)
        {
            ++_jitRequestsTriggered;
            TC_LOG_INFO("playerbot.jit", "QueueStatePoller: Submitted Horde JIT request {} for {} bots (priority {})",
                requestId, hordeStillNeeded, priority);
        }
    }

    // Record JIT request time for throttling
    RecordJITRequest(key);

    // Unregister the queue after submitting JIT requests.
    // JIT bots will login and queue asynchronously. Without this, the poller
    // re-polls and keeps spawning bots because JIT bots haven't entered the queue yet.
    _activeBGQueues.erase(key);
    _bgSnapshots.erase(key);
    TC_LOG_INFO("playerbot.jit", "QueueStatePoller: BG queue unregistered after JIT submission (bots will queue after login)");
}

void QueueStatePoller::ProcessLFGShortage(LFGQueueSnapshot const& snapshot)
{
    // Check throttling using dungeonId as key
    if (IsJITThrottled(snapshot.dungeonId))
    {
        TC_LOG_DEBUG("playerbot.jit", "QueueStatePoller: LFG shortage throttled for dungeon={}", snapshot.dungeonId);
        return;
    }

    ++_lfgShortagesDetected;

    TC_LOG_INFO("playerbot.jit", "QueueStatePoller: LFG Shortage detected - Dungeon={} Tank:{}/{} Healer:{}/{} DPS:{}/{}",
        snapshot.dungeonId,
        snapshot.tankCount, snapshot.tankNeeded,
        snapshot.healerCount, snapshot.healerNeeded,
        snapshot.dpsCount, snapshot.dpsNeeded);

    // Calculate shortages
    uint32 tanksShort = snapshot.tankNeeded > snapshot.tankCount ? snapshot.tankNeeded - snapshot.tankCount : 0;
    uint32 healersShort = snapshot.healerNeeded > snapshot.healerCount ? snapshot.healerNeeded - snapshot.healerCount : 0;
    uint32 dpsShort = snapshot.dpsNeeded > snapshot.dpsCount ? snapshot.dpsNeeded - snapshot.dpsCount : 0;

    if (tanksShort == 0 && healersShort == 0 && dpsShort == 0)
        return;

    uint32 tanksStillNeeded = tanksShort;
    uint32 healersStillNeeded = healersShort;
    uint32 dpsStillNeeded = dpsShort;

    // ========================================================================
    // CRITICAL FIX: Use HUMAN PLAYER'S LEVEL, not dungeon average!
    // ========================================================================
    // The human player queued at a specific level. Bots must match that level
    // so they can group together. Using the dungeon's average level creates
    // bots at the wrong level (e.g., level 37 for a level 26 player).
    //
    // Priority:
    // 1. Use humanPlayerLevel from _lfgQueueInfo if set (when human queued)
    // 2. Fall back to dungeon average if no human level tracked (shouldn't happen)
    // ========================================================================
    uint32 targetLevel = 0;
    auto queueInfoIt = _lfgQueueInfo.find(snapshot.dungeonId);
    if (queueInfoIt != _lfgQueueInfo.end() && queueInfoIt->second.humanPlayerLevel > 0)
    {
        targetLevel = queueInfoIt->second.humanPlayerLevel;
        TC_LOG_INFO("playerbot.jit", "QueueStatePoller: Using HUMAN PLAYER level {} for dungeon {} (dungeon range: {}-{})",
            targetLevel, snapshot.dungeonId, snapshot.minLevel, snapshot.maxLevel);
    }
    else
    {
        // Fallback: use dungeon average (this is the old, incorrect behavior)
        targetLevel = (snapshot.minLevel + snapshot.maxLevel) / 2;
        TC_LOG_WARN("playerbot.jit", "QueueStatePoller: ⚠️ No human player level found for dungeon {}, using dungeon average {} (SUBOPTIMAL)",
            snapshot.dungeonId, targetLevel);
    }

    // ========================================================================
    // STEP 1: TRY WARM POOL FIRST
    // The warm pool contains pre-logged-in bots ready for instant assignment.
    // We try both factions since modern WoW supports cross-faction LFG.
    // Each bot is queued via LFGBotManager::QueueJITBot() after assignment.
    // ========================================================================

    uint32 tanksFromPool = 0;
    uint32 healersFromPool = 0;
    uint32 dpsFromPool = 0;

    // Try Alliance pool first
    if (tanksStillNeeded > 0 || healersStillNeeded > 0 || dpsStillNeeded > 0)
    {
        std::vector<ObjectGuid> allianceBots = sInstanceBotPool->AssignForDungeon(
            snapshot.dungeonId,
            targetLevel,
            Faction::Alliance,
            tanksStillNeeded,
            healersStillNeeded,
            dpsStillNeeded
        );

        if (!allianceBots.empty())
        {
            TC_LOG_INFO("playerbot.jit", "QueueStatePoller: Got {} Alliance bots from warm pool for dungeon {}",
                allianceBots.size(), snapshot.dungeonId);

            // Queue each bot for LFG
            for (ObjectGuid const& botGuid : allianceBots)
            {
                if (Player* bot = ObjectAccessor::FindPlayer(botGuid))
                {
                    // Detect bot's role for tracking (QueueJITBot also does this internally)
                    uint8 detectedRole = sLFGRoleDetector->DetectBotRole(bot);

                    // Queue bot via LFGBotManager public API
                    if (sLFGBotManager->QueueJITBot(bot, snapshot.dungeonId))
                    {
                        // ====================================================================
                        // CRITICAL FIX (2026-01-22): Mark warm pool bots as INSTANCE BOTS!
                        // ====================================================================
                        // Without this, warm pool bots assigned via QueueStatePoller would:
                        // - NOT get the 60-second idle logout timeout
                        // - Stay online forever causing bot explosion
                        // - Do quests and other activities (no behavioral restriction)
                        // ====================================================================
                        sBotWorldSessionMgr->MarkAsInstanceBot(botGuid);

                        // Track which role was filled
                        if ((detectedRole & lfg::PLAYER_ROLE_TANK) && tanksStillNeeded > 0)
                        {
                            --tanksStillNeeded;
                            ++tanksFromPool;
                        }
                        else if ((detectedRole & lfg::PLAYER_ROLE_HEALER) && healersStillNeeded > 0)
                        {
                            --healersStillNeeded;
                            ++healersFromPool;
                        }
                        else if (dpsStillNeeded > 0)
                        {
                            --dpsStillNeeded;
                            ++dpsFromPool;
                        }

                        TC_LOG_DEBUG("playerbot.jit", "QueueStatePoller: Alliance bot {} queued for dungeon {} as role {}, marked as INSTANCE BOT",
                            bot->GetName(), snapshot.dungeonId, detectedRole);
                    }
                    else
                    {
                        TC_LOG_WARN("playerbot.jit", "QueueStatePoller: Failed to queue Alliance bot {} for dungeon {}",
                            bot->GetName(), snapshot.dungeonId);
                        // Release bot back to pool since queue failed
                        sInstanceBotPool->ReleaseBots({botGuid});
                    }
                }
                else
                {
                    TC_LOG_WARN("playerbot.jit", "QueueStatePoller: Alliance bot {} not found via ObjectAccessor",
                        botGuid.ToString());
                }
            }
        }
    }

    // Try Horde pool if still need more
    if (tanksStillNeeded > 0 || healersStillNeeded > 0 || dpsStillNeeded > 0)
    {
        std::vector<ObjectGuid> hordeBots = sInstanceBotPool->AssignForDungeon(
            snapshot.dungeonId,
            targetLevel,
            Faction::Horde,
            tanksStillNeeded,
            healersStillNeeded,
            dpsStillNeeded
        );

        if (!hordeBots.empty())
        {
            TC_LOG_INFO("playerbot.jit", "QueueStatePoller: Got {} Horde bots from warm pool for dungeon {}",
                hordeBots.size(), snapshot.dungeonId);

            // Queue each bot for LFG
            for (ObjectGuid const& botGuid : hordeBots)
            {
                if (Player* bot = ObjectAccessor::FindPlayer(botGuid))
                {
                    // Detect bot's role for tracking (QueueJITBot also does this internally)
                    uint8 detectedRole = sLFGRoleDetector->DetectBotRole(bot);

                    // Queue bot via LFGBotManager public API
                    if (sLFGBotManager->QueueJITBot(bot, snapshot.dungeonId))
                    {
                        // ====================================================================
                        // CRITICAL FIX (2026-01-22): Mark warm pool bots as INSTANCE BOTS!
                        // ====================================================================
                        // Without this, warm pool bots assigned via QueueStatePoller would:
                        // - NOT get the 60-second idle logout timeout
                        // - Stay online forever causing bot explosion
                        // - Do quests and other activities (no behavioral restriction)
                        // ====================================================================
                        sBotWorldSessionMgr->MarkAsInstanceBot(botGuid);

                        // Track which role was filled
                        if ((detectedRole & lfg::PLAYER_ROLE_TANK) && tanksStillNeeded > 0)
                        {
                            --tanksStillNeeded;
                            ++tanksFromPool;
                        }
                        else if ((detectedRole & lfg::PLAYER_ROLE_HEALER) && healersStillNeeded > 0)
                        {
                            --healersStillNeeded;
                            ++healersFromPool;
                        }
                        else if (dpsStillNeeded > 0)
                        {
                            --dpsStillNeeded;
                            ++dpsFromPool;
                        }

                        TC_LOG_DEBUG("playerbot.jit", "QueueStatePoller: Horde bot {} queued for dungeon {} as role {}, marked as INSTANCE BOT",
                            bot->GetName(), snapshot.dungeonId, detectedRole);
                    }
                    else
                    {
                        TC_LOG_WARN("playerbot.jit", "QueueStatePoller: Failed to queue Horde bot {} for dungeon {}",
                            bot->GetName(), snapshot.dungeonId);
                        // Release bot back to pool since queue failed
                        sInstanceBotPool->ReleaseBots({botGuid});
                    }
                }
                else
                {
                    TC_LOG_WARN("playerbot.jit", "QueueStatePoller: Horde bot {} not found via ObjectAccessor",
                        botGuid.ToString());
                }
            }
        }
    }

    // Log warm pool results
    uint32 totalFromPool = tanksFromPool + healersFromPool + dpsFromPool;
    if (totalFromPool > 0)
    {
        TC_LOG_INFO("playerbot.jit", "QueueStatePoller: Warm pool provided T:{}/H:{}/D:{} bots for dungeon {}",
            tanksFromPool, healersFromPool, dpsFromPool, snapshot.dungeonId);
    }

    // If warm pool fully satisfied the demand, we're done
    if (tanksStillNeeded == 0 && healersStillNeeded == 0 && dpsStillNeeded == 0)
    {
        TC_LOG_INFO("playerbot.jit", "QueueStatePoller: LFG shortage fully satisfied from warm pool");
        RecordJITRequest(snapshot.dungeonId);
        return;
    }

    // ========================================================================
    // STEP 2: JIT CREATION FOR REMAINING SHORTAGE
    // Only create bots via JIT if warm pool couldn't satisfy demand.
    // JIT bots will be queued after login via BotPostLoginConfigurator.
    // ========================================================================

    TC_LOG_INFO("playerbot.jit", "QueueStatePoller: Warm pool insufficient, requesting JIT for T:{}/H:{}/D:{}",
        tanksStillNeeded, healersStillNeeded, dpsStillNeeded);

    // LFG gets high priority (7 out of 10)
    uint8 priority = 7;

    FactoryRequest request;
    request.instanceType = InstanceType::Dungeon;
    request.contentId = snapshot.dungeonId;
    request.playerLevel = targetLevel;
    request.tanksNeeded = tanksStillNeeded;
    request.healersNeeded = healersStillNeeded;
    request.dpsNeeded = dpsStillNeeded;
    request.priority = priority;
    request.createdAt = std::chrono::system_clock::now();

    // Set dungeon ID for post-login queueing
    // The BotPostLoginConfigurator will queue bots AFTER they're fully logged in
    // This avoids the timing issue where ObjectAccessor::FindPlayer returns nullptr
    // because the bots haven't entered the world yet when onComplete fires.
    request.dungeonIdToQueue = snapshot.dungeonId;

    // Callback for debugging (bots queue via pendingConfig, not here)
    request.onComplete = [dungeonId = snapshot.dungeonId](std::vector<ObjectGuid> const& botGuids) {
        TC_LOG_INFO("playerbot.jit", "QueueStatePoller: {} JIT bots created for dungeon {} - they will auto-queue after login",
            botGuids.size(), dungeonId);
    };

    uint32 requestId = sJITBotFactory->SubmitRequest(std::move(request));
    if (requestId > 0)
    {
        ++_jitRequestsTriggered;
        TC_LOG_INFO("playerbot.jit", "QueueStatePoller: Submitted LFG JIT request {} for T:{}/H:{}/D:{} bots",
            requestId, tanksStillNeeded, healersStillNeeded, dpsStillNeeded);
    }

    RecordJITRequest(snapshot.dungeonId);
}

void QueueStatePoller::ProcessArenaShortage(ArenaQueueSnapshot const& snapshot)
{
    uint64 key = MakeArenaKey(snapshot.arenaType, snapshot.bracketId);

    if (IsJITThrottled(key))
    {
        TC_LOG_DEBUG("playerbot.jit", "QueueStatePoller: Arena shortage throttled for type={}v{}", snapshot.arenaType, snapshot.arenaType);
        return;
    }

    ++_arenaShortagesDetected;

    TC_LOG_INFO("playerbot.jit", "QueueStatePoller: Arena Shortage detected - Type={}v{} Alliance:{} Horde:{}",
        snapshot.arenaType, snapshot.arenaType,
        snapshot.allianceTeamsInQueue, snapshot.hordeTeamsInQueue);

    // Arena gets medium priority (5 out of 10)
    uint8 priority = 5;

    // For arena, we need complete teams
    uint32 allianceTeamsNeeded = snapshot.allianceTeamsInQueue == 0 ? 1 : 0;
    uint32 hordeTeamsNeeded = snapshot.hordeTeamsInQueue == 0 ? 1 : 0;

    uint32 allianceBotsNeeded = allianceTeamsNeeded * snapshot.playersPerTeam;
    uint32 hordeBotsNeeded = hordeTeamsNeeded * snapshot.playersPerTeam;

    // Get level from bracket for warm pool assignment
    uint32 minLevel, maxLevel;
    GetBracketLevelRange(static_cast<PoolBracket>(snapshot.bracketId), minLevel, maxLevel);
    uint32 bracketLevel = (minLevel + maxLevel) / 2;

    // ========================================================================
    // STEP 1: TRY WARM POOL FIRST
    // Warm pool bots are pre-created and ready for instant assignment
    // Only fall back to JIT if warm pool doesn't have enough bots
    // ========================================================================

    uint32 allianceFromPool = 0;
    uint32 hordeFromPool = 0;
    uint32 allianceStillNeeded = allianceBotsNeeded;
    uint32 hordeStillNeeded = hordeBotsNeeded;

    // Try Alliance first if needed
    if (allianceBotsNeeded > 0)
    {
        ArenaAssignment poolAssignment = sInstanceBotPool->AssignForArena(
            snapshot.arenaType,
            bracketLevel,
            Faction::Alliance,
            allianceBotsNeeded,  // teammates needed (all same faction for this call)
            0                    // no opponents in this call
        );

        if (poolAssignment.success && !poolAssignment.teammates.empty())
        {
            allianceFromPool = static_cast<uint32>(poolAssignment.teammates.size());
            TC_LOG_INFO("playerbot.jit", "QueueStatePoller: Got {}/{} Alliance bots from warm pool for {}v{}",
                allianceFromPool, allianceBotsNeeded, snapshot.arenaType, snapshot.arenaType);

            // Queue the bots from pool for arena and MARK AS INSTANCE BOTS
            // ====================================================================
            // CRITICAL FIX (2026-01-22): Mark warm pool arena bots as INSTANCE BOTS!
            // ====================================================================
            for (ObjectGuid const& guid : poolAssignment.teammates)
            {
                if (Player* bot = ObjectAccessor::FindPlayer(guid))
                {
                    // Mark as instance bot for 60-second idle timeout
                    sBotWorldSessionMgr->MarkAsInstanceBot(guid);
                    // Queue bot for arena (implementation in ArenaBotManager)
                    TC_LOG_DEBUG("playerbot.jit", "QueueStatePoller: Alliance arena bot {} ready from pool, marked as INSTANCE BOT",
                        guid.ToString());
                }
            }
            allianceStillNeeded = allianceStillNeeded > allianceFromPool ? allianceStillNeeded - allianceFromPool : 0;
        }
    }

    // Try Horde if needed
    if (hordeBotsNeeded > 0)
    {
        ArenaAssignment poolAssignment = sInstanceBotPool->AssignForArena(
            snapshot.arenaType,
            bracketLevel,
            Faction::Horde,
            hordeBotsNeeded,  // teammates needed
            0                 // no opponents
        );

        if (poolAssignment.success && !poolAssignment.teammates.empty())
        {
            hordeFromPool = static_cast<uint32>(poolAssignment.teammates.size());
            TC_LOG_INFO("playerbot.jit", "QueueStatePoller: Got {}/{} Horde bots from warm pool for {}v{}",
                hordeFromPool, hordeBotsNeeded, snapshot.arenaType, snapshot.arenaType);

            // Queue the bots from pool for arena and MARK AS INSTANCE BOTS
            // ====================================================================
            // CRITICAL FIX (2026-01-22): Mark warm pool arena bots as INSTANCE BOTS!
            // ====================================================================
            for (ObjectGuid const& guid : poolAssignment.teammates)
            {
                if (Player* bot = ObjectAccessor::FindPlayer(guid))
                {
                    // Mark as instance bot for 60-second idle timeout
                    sBotWorldSessionMgr->MarkAsInstanceBot(guid);
                    TC_LOG_DEBUG("playerbot.jit", "QueueStatePoller: Horde arena bot {} ready from pool, marked as INSTANCE BOT",
                        guid.ToString());
                }
            }
            hordeStillNeeded = hordeStillNeeded > hordeFromPool ? hordeStillNeeded - hordeFromPool : 0;
        }
    }

    // If warm pool fully satisfied the demand, we're done
    if (allianceStillNeeded == 0 && hordeStillNeeded == 0)
    {
        TC_LOG_INFO("playerbot.jit", "QueueStatePoller: Arena shortage fully satisfied from warm pool");
        RecordJITRequest(key);
        return;
    }

    // ========================================================================
    // STEP 2: JIT CREATION FOR REMAINING SHORTAGE
    // Only create bots via JIT if warm pool couldn't satisfy demand
    // ========================================================================

    TC_LOG_INFO("playerbot.jit", "QueueStatePoller: Warm pool insufficient, requesting JIT for Alliance={} Horde={}",
        allianceStillNeeded, hordeStillNeeded);

    if (allianceStillNeeded > 0)
    {
        FactoryRequest request;
        request.instanceType = InstanceType::Arena;
        request.contentId = snapshot.arenaType;  // 2, 3, or 5 for arena type
        request.playerFaction = Faction::Alliance;
        request.allianceNeeded = allianceStillNeeded;
        request.priority = priority;
        request.createdAt = std::chrono::system_clock::now();

        request.onComplete = [arenaType = snapshot.arenaType](std::vector<ObjectGuid> const& botGuids) {
            TC_LOG_DEBUG("playerbot.jit", "QueueStatePoller: {} Alliance arena bots ready for {}v{}",
                botGuids.size(), arenaType, arenaType);
        };

        uint32 requestId = sJITBotFactory->SubmitRequest(std::move(request));
        if (requestId > 0)
        {
            ++_jitRequestsTriggered;
        }
    }

    if (hordeStillNeeded > 0)
    {
        FactoryRequest request;
        request.instanceType = InstanceType::Arena;
        request.contentId = snapshot.arenaType;
        request.playerFaction = Faction::Horde;
        request.hordeNeeded = hordeStillNeeded;
        request.priority = priority;
        request.createdAt = std::chrono::system_clock::now();

        request.onComplete = [arenaType = snapshot.arenaType](std::vector<ObjectGuid> const& botGuids) {
            TC_LOG_DEBUG("playerbot.jit", "QueueStatePoller: {} Horde arena bots ready for {}v{}",
                botGuids.size(), arenaType, arenaType);
        };

        uint32 requestId = sJITBotFactory->SubmitRequest(std::move(request));
        if (requestId > 0)
        {
            ++_jitRequestsTriggered;
        }
    }

    RecordJITRequest(key);
}

// ============================================================================
// THROTTLING
// ============================================================================

bool QueueStatePoller::IsJITThrottled(uint64 queueKey) const
{
    auto it = _lastJITTime.find(queueKey);
    if (it == _lastJITTime.end())
        return false;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second);
    return elapsed.count() < static_cast<int64>(_jitThrottleSeconds);
}

void QueueStatePoller::RecordJITRequest(uint64 queueKey)
{
    _lastJITTime[queueKey] = std::chrono::steady_clock::now();
}

// ============================================================================
// KEY GENERATION
// ============================================================================

uint64 QueueStatePoller::MakeBGKey(BattlegroundTypeId bgTypeId, BattlegroundBracketId bracket)
{
    return (static_cast<uint64>(bgTypeId) << 32) | static_cast<uint32>(bracket);
}

uint64 QueueStatePoller::MakeArenaKey(uint8 arenaType, BattlegroundBracketId bracket)
{
    // Use high bit to distinguish from BG keys
    return (static_cast<uint64>(0x80000000 | arenaType) << 32) | static_cast<uint32>(bracket);
}

// ============================================================================
// SNAPSHOT ACCESS
// ============================================================================

BGQueueSnapshot QueueStatePoller::GetBGSnapshot(BattlegroundTypeId bgTypeId, BattlegroundBracketId bracket) const
{
    std::lock_guard<decltype(_mutex)> lock(_mutex);

    uint64 key = MakeBGKey(bgTypeId, bracket);
    auto it = _bgSnapshots.find(key);
    if (it != _bgSnapshots.end())
        return it->second;
    return BGQueueSnapshot();
}

LFGQueueSnapshot QueueStatePoller::GetLFGSnapshot(uint32 dungeonId) const
{
    std::lock_guard<decltype(_mutex)> lock(_mutex);

    auto it = _lfgSnapshots.find(dungeonId);
    if (it != _lfgSnapshots.end())
        return it->second;
    return LFGQueueSnapshot();
}

ArenaQueueSnapshot QueueStatePoller::GetArenaSnapshot(uint8 arenaType, BattlegroundBracketId bracket) const
{
    std::lock_guard<decltype(_mutex)> lock(_mutex);

    uint64 key = MakeArenaKey(arenaType, bracket);
    auto it = _arenaSnapshots.find(key);
    if (it != _arenaSnapshots.end())
        return it->second;
    return ArenaQueueSnapshot();
}

// ============================================================================
// SHORTAGE QUERIES
// ============================================================================

bool QueueStatePoller::HasBGShortage(BattlegroundTypeId bgTypeId, BattlegroundBracketId bracket) const
{
    BGQueueSnapshot snapshot = GetBGSnapshot(bgTypeId, bracket);
    return snapshot.HasShortage();
}

bool QueueStatePoller::HasLFGShortage(uint32 dungeonId) const
{
    LFGQueueSnapshot snapshot = GetLFGSnapshot(dungeonId);
    return snapshot.HasShortage();
}

bool QueueStatePoller::HasArenaShortage(uint8 arenaType, BattlegroundBracketId bracket) const
{
    ArenaQueueSnapshot snapshot = GetArenaSnapshot(arenaType, bracket);
    return snapshot.HasShortage();
}

// ============================================================================
// STATISTICS
// ============================================================================

QueueStatePoller::Statistics QueueStatePoller::GetStatistics() const
{
    std::lock_guard<decltype(_mutex)> lock(_mutex);

    Statistics stats;
    stats.pollCount = _pollCount;
    stats.bgShortagesDetected = _bgShortagesDetected;
    stats.lfgShortagesDetected = _lfgShortagesDetected;
    stats.arenaShortagesDetected = _arenaShortagesDetected;
    stats.jitRequestsTriggered = _jitRequestsTriggered;
    stats.activeBGQueues = static_cast<uint32>(_activeBGQueues.size());
    stats.activeLFGQueues = static_cast<uint32>(_activeLFGQueues.size());
    stats.activeArenaQueues = static_cast<uint32>(_activeArenaQueues.size());
    return stats;
}

void QueueStatePoller::ResetStatistics()
{
    _pollCount = 0;
    _bgShortagesDetected = 0;
    _lfgShortagesDetected = 0;
    _arenaShortagesDetected = 0;
    _jitRequestsTriggered = 0;
}

} // namespace Playerbot
