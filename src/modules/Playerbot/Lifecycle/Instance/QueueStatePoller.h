/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * @file QueueStatePoller.h
 * @brief Polls TrinityCore queue systems to detect player shortages
 *
 * This class uses READ-ONLY access to TrinityCore systems:
 * - BattlegroundMgr::GetBattlegroundQueue() for BG queue state
 * - LFGMgr for dungeon queue state
 *
 * NO CORE MODIFICATIONS REQUIRED - pure module-only implementation.
 *
 * Architecture:
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │                    QUEUE STATE POLLER                                   │
 * ├─────────────────────────────────────────────────────────────────────────┤
 * │                                                                         │
 * │  ┌──────────────────────────────────────────────────────────────────┐  │
 * │  │              TrinityCore Queue APIs (READ-ONLY)                  │  │
 * │  │  BattlegroundMgr::GetBattlegroundQueue()                         │  │
 * │  │  BattlegroundQueue::GetPlayersInQueue(TeamId)                    │  │
 * │  │  LFGMgr queue information                                        │  │
 * │  └────────────────────────────┬─────────────────────────────────────┘  │
 * │                               │                                        │
 * │                               ▼                                        │
 * │  ┌──────────────────────────────────────────────────────────────────┐  │
 * │  │                    QueueStatePoller::Update()                    │  │
 * │  │  - Polls active queues every 5 seconds                          │  │
 * │  │  - Detects faction imbalances                                   │  │
 * │  │  - Calculates shortages                                         │  │
 * │  └────────────────────────────┬─────────────────────────────────────┘  │
 * │                               │                                        │
 * │                               ▼                                        │
 * │  ┌──────────────────────────────────────────────────────────────────┐  │
 * │  │            JITBotFactory::SubmitRequest()                        │  │
 * │  │  - Creates bots for shortage faction                            │  │
 * │  │  - Queues bots via callback                                     │  │
 * │  └──────────────────────────────────────────────────────────────────┘  │
 * │                                                                         │
 * └─────────────────────────────────────────────────────────────────────────┘
 */

#pragma once

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "ObjectGuid.h"
#include "SharedDefines.h"  // BattlegroundTypeId, Team
#include "DBCEnums.h"       // BattlegroundBracketId
#include <unordered_map>
#include <unordered_set>
#include <atomic>
#include <chrono>
#include <mutex>

namespace Playerbot
{

// ============================================================================
// QUEUE SNAPSHOT STRUCTURES
// ============================================================================

/**
 * @brief Snapshot of BG queue state at a point in time
 */
struct TC_GAME_API BGQueueSnapshot
{
    BattlegroundTypeId bgTypeId;
    BattlegroundBracketId bracketId;

    // Faction counts (from BattlegroundQueue::GetPlayersInQueue)
    uint32 allianceCount;
    uint32 hordeCount;

    // Requirements (from BattlegroundTemplate)
    uint32 minPlayersPerTeam;
    uint32 maxPlayersPerTeam;

    // Calculated shortages (positive = need more, negative = surplus)
    int32 allianceShortage;
    int32 hordeShortage;

    // When this snapshot was taken
    time_t timestamp;

    BGQueueSnapshot();

    /**
     * @brief Check if there's any shortage
     */
    bool HasShortage() const { return allianceShortage > 0 || hordeShortage > 0; }

    /**
     * @brief Get total shortage count
     */
    uint32 GetTotalShortage() const;
};

/**
 * @brief Snapshot of LFG queue state
 */
struct TC_GAME_API LFGQueueSnapshot
{
    uint32 dungeonId;
    uint8 minLevel;
    uint8 maxLevel;

    // Role counts in queue
    uint32 tankCount;
    uint32 healerCount;
    uint32 dpsCount;

    // Requirements (standard 5-man: 1 tank, 1 healer, 3 DPS)
    uint32 tankNeeded;
    uint32 healerNeeded;
    uint32 dpsNeeded;

    time_t timestamp;

    LFGQueueSnapshot();

    /**
     * @brief Check if there's a role shortage
     */
    bool HasShortage() const;

    /**
     * @brief Get the most urgent role needed
     * @return 0 = tank, 1 = healer, 2 = dps, 255 = none needed
     */
    uint8 GetMostUrgentRole() const;
};

/**
 * @brief Arena queue snapshot
 */
struct TC_GAME_API ArenaQueueSnapshot
{
    uint8 arenaType;  // 2v2, 3v3, 5v5
    BattlegroundBracketId bracketId;

    uint32 allianceTeamsInQueue;
    uint32 hordeTeamsInQueue;

    uint32 playersPerTeam;

    time_t timestamp;

    ArenaQueueSnapshot();

    bool HasShortage() const;
};

// ============================================================================
// QUEUE STATE POLLER
// ============================================================================

/**
 * @brief Polls TrinityCore queue APIs periodically to detect shortages
 *
 * This singleton monitors active BG/LFG/Arena queues and triggers JIT bot
 * creation when shortages are detected. It uses only READ-ONLY access to
 * TrinityCore systems.
 *
 * Thread Safety:
 * - Update() should be called from world thread
 * - RegisterActiveQueue/UnregisterActiveQueue are thread-safe
 * - All internal state protected by OrderedRecursiveMutex
 */
class TC_GAME_API QueueStatePoller
{
private:
    QueueStatePoller();
    ~QueueStatePoller();

public:
    /**
     * @brief Get singleton instance
     */
    static QueueStatePoller* Instance();

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /**
     * @brief Initialize the poller
     * @return true if initialization succeeded
     */
    bool Initialize();

    /**
     * @brief Shutdown and cleanup
     */
    void Shutdown();

    /**
     * @brief Periodic update - call from world thread
     * @param diff Time since last update in milliseconds
     */
    void Update(uint32 diff);

    // ========================================================================
    // QUEUE REGISTRATION
    // ========================================================================

    /**
     * @brief Register a BG queue as active (has human players)
     *
     * Called by PlayerbotBGScript when a human joins a queue.
     * The poller will start monitoring this queue for shortages.
     *
     * @param bgTypeId Battleground type
     * @param bracket Bracket (determines level range)
     */
    void RegisterActiveBGQueue(BattlegroundTypeId bgTypeId, BattlegroundBracketId bracket);

    /**
     * @brief Unregister a BG queue (no more humans in queue)
     *
     * Called when all humans leave the queue or BG starts.
     */
    void UnregisterActiveBGQueue(BattlegroundTypeId bgTypeId, BattlegroundBracketId bracket);

    /**
     * @brief Register an LFG queue as active with human player level (PREFERRED)
     *
     * CRITICAL: Always use this overload when a human player joins the queue.
     * Bots must be created at the HUMAN PLAYER'S LEVEL, not the dungeon's average.
     *
     * @param dungeonId The dungeon ID
     * @param minLevel Dungeon minimum level
     * @param maxLevel Dungeon maximum level
     * @param humanPlayerLevel The human player's current level (bots will match this)
     */
    void RegisterActiveLFGQueue(uint32 dungeonId, uint8 minLevel, uint8 maxLevel, uint8 humanPlayerLevel);

    /**
     * @brief Register an LFG queue as active
     * @deprecated Use the overload with humanPlayerLevel instead
     */
    void RegisterActiveLFGQueue(uint32 dungeonId, uint8 minLevel, uint8 maxLevel);

    /**
     * @brief Register an LFG queue as active (auto-detects level range from LFGDungeonEntry)
     * @deprecated Use the overload with humanPlayerLevel instead
     */
    void RegisterActiveLFGQueue(uint32 dungeonId);

    /**
     * @brief Unregister an LFG queue
     */
    void UnregisterActiveLFGQueue(uint32 dungeonId);

    /**
     * @brief Register an Arena queue as active
     */
    void RegisterActiveArenaQueue(uint8 arenaType, BattlegroundBracketId bracket);

    /**
     * @brief Unregister an Arena queue
     */
    void UnregisterActiveArenaQueue(uint8 arenaType, BattlegroundBracketId bracket);

    // ========================================================================
    // LFG ROLE COUNT UPDATES
    // ========================================================================

    /**
     * @brief Update LFG role counts when a bot is queued
     *
     * Called by LFGBotManager when a bot is successfully queued for a dungeon.
     * This keeps the QueueStatePoller's snapshot accurate so it doesn't
     * trigger unnecessary JIT requests.
     *
     * @param dungeonId The dungeon the bot was queued for
     * @param role The role the bot was queued as (PLAYER_ROLE_TANK/HEALER/DAMAGE)
     * @param increment True to increment count (bot queued), false to decrement (bot left)
     */
    void UpdateLFGRoleCount(uint32 dungeonId, uint8 role, bool increment = true);

    /**
     * @brief Update LFG needed role counts based on dungeon requirements
     *
     * Called when a human player joins an LFG queue to set the required counts.
     *
     * @param dungeonId The dungeon ID
     * @param tanksNeeded Number of tanks needed
     * @param healersNeeded Number of healers needed
     * @param dpsNeeded Number of DPS needed
     */
    void SetLFGNeededCounts(uint32 dungeonId, uint32 tanksNeeded, uint32 healersNeeded, uint32 dpsNeeded);

    // ========================================================================
    // MANUAL POLL TRIGGERS
    // ========================================================================

    /**
     * @brief Immediately poll all BG queues
     *
     * Called by BGBotManager::OnPlayerJoinQueue for immediate response.
     */
    void PollBGQueues();

    /**
     * @brief Immediately poll all LFG queues
     */
    void PollLFGQueues();

    /**
     * @brief Immediately poll all Arena queues
     */
    void PollArenaQueues();

    // ========================================================================
    // SNAPSHOT ACCESS
    // ========================================================================

    /**
     * @brief Get current BG queue snapshot
     * @return Snapshot data (may be stale if queue not actively monitored)
     */
    BGQueueSnapshot GetBGSnapshot(BattlegroundTypeId bgTypeId, BattlegroundBracketId bracket) const;

    /**
     * @brief Get current LFG queue snapshot
     */
    LFGQueueSnapshot GetLFGSnapshot(uint32 dungeonId) const;

    /**
     * @brief Get current Arena queue snapshot
     */
    ArenaQueueSnapshot GetArenaSnapshot(uint8 arenaType, BattlegroundBracketId bracket) const;

    // ========================================================================
    // SHORTAGE QUERIES
    // ========================================================================

    /**
     * @brief Check if a BG queue has shortage
     */
    bool HasBGShortage(BattlegroundTypeId bgTypeId, BattlegroundBracketId bracket) const;

    /**
     * @brief Check if an LFG queue has shortage
     */
    bool HasLFGShortage(uint32 dungeonId) const;

    /**
     * @brief Check if an Arena queue has shortage
     */
    bool HasArenaShortage(uint8 arenaType, BattlegroundBracketId bracket) const;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /**
     * @brief Set polling interval
     * @param ms Interval in milliseconds (default: 5000)
     */
    void SetPollingInterval(uint32 ms) { _pollingInterval = ms; }

    /**
     * @brief Get current polling interval
     */
    uint32 GetPollingInterval() const { return _pollingInterval; }

    /**
     * @brief Enable or disable the poller
     */
    void SetEnabled(bool enable) { _enabled = enable; }

    /**
     * @brief Check if poller is enabled
     */
    bool IsEnabled() const { return _enabled; }

    /**
     * @brief Set minimum time between JIT requests for same queue
     * @param seconds Throttle time in seconds
     */
    void SetJITThrottleTime(uint32 seconds) { _jitThrottleSeconds = seconds; }

    // ========================================================================
    // STATISTICS
    // ========================================================================

    struct Statistics
    {
        uint32 pollCount;
        uint32 bgShortagesDetected;
        uint32 lfgShortagesDetected;
        uint32 arenaShortagesDetected;
        uint32 jitRequestsTriggered;
        uint32 activeBGQueues;
        uint32 activeLFGQueues;
        uint32 activeArenaQueues;
    };

    /**
     * @brief Get current statistics
     */
    Statistics GetStatistics() const;

    /**
     * @brief Reset statistics
     */
    void ResetStatistics();

private:
    // ========================================================================
    // INTERNAL POLL METHODS
    // ========================================================================

    /**
     * @brief Poll a specific BG queue
     */
    void DoPollBGQueue(BattlegroundTypeId bgTypeId, BattlegroundBracketId bracket);

    /**
     * @brief Poll a specific LFG queue
     */
    void DoPollLFGQueue(uint32 dungeonId, uint8 minLevel, uint8 maxLevel);

    /**
     * @brief Poll a specific Arena queue
     */
    void DoPollArenaQueue(uint8 arenaType, BattlegroundBracketId bracket);

    // ========================================================================
    // SHORTAGE PROCESSING
    // ========================================================================

    /**
     * @brief Process pending BG verification checks
     *
     * After warm pool claims to satisfy demand, we wait 30s then verify
     * bots actually queued. If not enough queued, re-registers the queue.
     */
    void ProcessPendingBGVerifications();

    /**
     * @brief Process detected BG shortage - trigger JIT if needed
     */
    void ProcessBGShortage(BGQueueSnapshot const& snapshot);

    /**
     * @brief Process detected LFG shortage
     */
    void ProcessLFGShortage(LFGQueueSnapshot const& snapshot);

    /**
     * @brief Process detected Arena shortage
     */
    void ProcessArenaShortage(ArenaQueueSnapshot const& snapshot);

    /**
     * @brief Check if JIT request is throttled for this queue
     * @return true if should skip JIT request (too soon since last)
     */
    bool IsJITThrottled(uint64 queueKey) const;

    /**
     * @brief Record JIT request time for throttling
     */
    void RecordJITRequest(uint64 queueKey);

    // ========================================================================
    // KEY GENERATION
    // ========================================================================

    /**
     * @brief Generate unique key for BG queue
     */
    static uint64 MakeBGKey(BattlegroundTypeId bgTypeId, BattlegroundBracketId bracket);

    /**
     * @brief Generate unique key for Arena queue
     */
    static uint64 MakeArenaKey(uint8 arenaType, BattlegroundBracketId bracket);

    // ========================================================================
    // DATA MEMBERS
    // ========================================================================

    mutable OrderedRecursiveMutex<LockOrder::QUEUE_MONITOR> _mutex;

    // Active queues being monitored (only poll queues with humans)
    std::unordered_set<uint64> _activeBGQueues;
    std::unordered_set<uint32> _activeLFGQueues;  // key = dungeonId
    std::unordered_set<uint64> _activeArenaQueues;

    // LFG level requirements per dungeon
    struct LFGQueueInfo
    {
        uint8 minLevel;         // Dungeon minimum level requirement
        uint8 maxLevel;         // Dungeon maximum level requirement
        uint8 humanPlayerLevel; // CRITICAL: Level of the human player who queued
                                // Bots must match the human's level, NOT the dungeon's level range
    };
    std::unordered_map<uint32, LFGQueueInfo> _lfgQueueInfo;

    // Queue state snapshots
    std::unordered_map<uint64, BGQueueSnapshot> _bgSnapshots;
    std::unordered_map<uint32, LFGQueueSnapshot> _lfgSnapshots;
    std::unordered_map<uint64, ArenaQueueSnapshot> _arenaSnapshots;

    // BG queue verification - tracks warm pool assignments that need re-verification
    // After warm pool claims success, we schedule a verification poll to confirm bots
    // actually queued. If not enough bots queued, the queue stays registered for re-polling.
    struct BGVerificationEntry
    {
        std::chrono::steady_clock::time_point verificationTime;  // When to verify
        uint32 expectedAlliance = 0;  // How many Alliance bots we expected
        uint32 expectedHorde = 0;     // How many Horde bots we expected
    };
    std::unordered_map<uint64, BGVerificationEntry> _pendingBGVerifications;

    // JIT throttling - last request time per queue
    std::unordered_map<uint64, std::chrono::steady_clock::time_point> _lastJITTime;

    // Configuration
    std::atomic<bool> _enabled;
    uint32 _pollingInterval;
    uint32 _jitThrottleSeconds;
    uint32 _updateAccumulator;

    // Statistics
    std::atomic<uint32> _pollCount;
    std::atomic<uint32> _bgShortagesDetected;
    std::atomic<uint32> _lfgShortagesDetected;
    std::atomic<uint32> _arenaShortagesDetected;
    std::atomic<uint32> _jitRequestsTriggered;

    // Constants
    static constexpr uint32 DEFAULT_POLLING_INTERVAL = 5 * 1000;  // 5 seconds
    static constexpr uint32 DEFAULT_JIT_THROTTLE_SECONDS = 10;
    static constexpr uint32 BG_VERIFICATION_DELAY_SECONDS = 30;   // Wait 30s before verifying warm pool bots queued

    // Singleton
    QueueStatePoller(QueueStatePoller const&) = delete;
    QueueStatePoller& operator=(QueueStatePoller const&) = delete;
};

} // namespace Playerbot

#define sQueueStatePoller Playerbot::QueueStatePoller::Instance()
