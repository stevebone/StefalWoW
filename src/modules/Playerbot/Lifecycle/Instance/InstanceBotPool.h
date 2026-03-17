/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * @file InstanceBotPool.h
 * @brief Pre-warmed per-bracket bot pool for instant instance assignment
 *
 * The InstanceBotPool maintains per-bracket pools of pre-created bots ready for
 * instant assignment to dungeons, raids, battlegrounds, and arenas at any level.
 *
 * NEW PER-BRACKET ARCHITECTURE (v2.0):
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │                    PER-BRACKET INSTANCE BOT POOL                        │
 * ├─────────────────────────────────────────────────────────────────────────┤
 * │                                                                         │
 * │   8 LEVEL BRACKETS × 2 FACTIONS × 50 BOTS = 800 TOTAL                  │
 * │                                                                         │
 * │   Bracket 10-19:  A[50] + H[50] = 100 bots (10T, 15H, 25D per faction) │
 * │   Bracket 20-29:  A[50] + H[50] = 100 bots                             │
 * │   Bracket 30-39:  A[50] + H[50] = 100 bots                             │
 * │   Bracket 40-49:  A[50] + H[50] = 100 bots                             │
 * │   Bracket 50-59:  A[50] + H[50] = 100 bots                             │
 * │   Bracket 60-69:  A[50] + H[50] = 100 bots                             │
 * │   Bracket 70-79:  A[50] + H[50] = 100 bots                             │
 * │   Bracket 80+:    A[50] + H[50] = 100 bots                             │
 * │                                                                         │
 * │   SUPPORTS PER BRACKET:                                                 │
 * │   - 2 full dungeon groups (10 players)                                  │
 * │   - 1 Alterac Valley (40 per faction)                                   │
 * │   - Parallel dungeon + BG content                                       │
 * │                                                                         │
 * │   READY INDEX (O(1) lookup):                                            │
 * │   _readyIndex[Role][Faction][Bracket] → vector<ObjectGuid>             │
 * │                                                                         │
 * │   SLOT STATES:                                                          │
 * │   [Ready] ──assign──> [Assigned] ──release──> [Cooldown] ──expire──>   │
 * │      ↑                                                              │   │
 * │      └──────────────────────────────────────────────────────────────┘   │
 * │                                                                         │
 * └─────────────────────────────────────────────────────────────────────────┘
 *
 * Thread Safety:
 * - All public methods are thread-safe
 * - Uses unordered_map with shared_mutex for thread safety
 * - ReadyIndex provides O(1) bracket lookup
 * - Statistics are atomic or protected by shared mutex
 *
 * Performance:
 * - O(1) bot lookup by GUID
 * - O(1) bracket lookup via ReadyIndex (role → faction → bracket)
 * - O(k) bot selection where k = bots in bracket (typically <50)
 * - Assignment target: <50ms for 5-man group
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "PoolSlotState.h"
#include "PoolConfiguration.h"
#include "PoolStatistics.h"
#include "InstanceBotSlot.h"
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <unordered_map>

namespace Playerbot
{

// Forward declarations
class BotFactory;
class BotSession;

/**
 * @brief Assignment result for battlegrounds (both factions)
 */
struct BGAssignment
{
    std::vector<ObjectGuid> allianceBots;   ///< Alliance bots assigned
    std::vector<ObjectGuid> hordeBots;      ///< Horde bots assigned
    bool success = false;                    ///< Whether assignment succeeded
    std::string errorMessage;                ///< Error message if failed
};

/**
 * @brief Assignment result for arenas
 */
struct ArenaAssignment
{
    std::vector<ObjectGuid> teammates;      ///< Teammate bots
    std::vector<ObjectGuid> opponents;      ///< Opponent bots
    bool success = false;
    std::string errorMessage;
};

/**
 * @brief Pre-warmed bot pool for instant instance assignment
 *
 * Singleton class managing a pool of pre-logged-in bots for instant
 * assignment to dungeons, raids, battlegrounds, and arenas.
 */
class TC_GAME_API InstanceBotPool
{
public:
    /**
     * @brief Get singleton instance
     */
    static InstanceBotPool* Instance();

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /**
     * @brief Initialize the pool
     * @return true if successful
     *
     * Loads configuration, initializes data structures, and optionally
     * loads persistent pool data from database.
     */
    bool Initialize();

    /**
     * @brief Shutdown and cleanup
     *
     * Saves pool state to database (if configured), releases all bots,
     * and cleans up resources.
     */
    void Shutdown();

    /**
     * @brief Main update loop
     * @param diff Time since last update in milliseconds
     *
     * Called from world update loop. Handles:
     * - Cooldown expiration
     * - Reservation timeouts
     * - Pool replenishment
     * - Statistics updates
     */
    void Update(uint32 diff);

    /**
     * @brief Load configuration from config file
     */
    void LoadConfig();

    // ========================================================================
    // POOL MANAGEMENT
    // ========================================================================

    /**
     * @brief Warm up the pool (create and login bots)
     *
     * Called during server startup or when pool needs replenishment.
     * Creates bots according to configuration and logs them in.
     */
    void WarmPool();

    /**
     * @brief Replenish depleted slots
     *
     * Checks pool levels and creates new bots as needed to maintain
     * configured pool sizes.
     */
    void ReplenishPool();

    /**
     * @brief Get count of available bots by role and pool type
     * @param role Bot role (Tank/Healer/DPS)
     * @param poolType Pool type (PvE/PvP_Alliance/PvP_Horde)
     * @return Number of ready bots matching criteria
     */
    uint32 GetAvailableCount(BotRole role, PoolType poolType) const;

    /**
     * @brief Get count of available bots by role and faction
     * @param role Bot role
     * @param faction Faction
     * @return Number of ready bots matching criteria
     */
    uint32 GetAvailableCount(BotRole role, Faction faction) const;

    /**
     * @brief Get count of available bots matching level range
     * @param role Bot role
     * @param faction Faction
     * @param level Target level
     * @param range Acceptable level variance (±range)
     * @return Number of ready bots matching criteria
     */
    uint32 GetAvailableCountForLevel(BotRole role, Faction faction,
                                      uint32 level, uint32 range = 5) const;

    // ========================================================================
    // PER-BRACKET POOL QUERIES
    // ========================================================================

    /**
     * @brief Get count of available bots in a specific bracket
     * @param bracket Level bracket
     * @param faction Faction
     * @param role Bot role (BotRole::Max for all roles)
     * @return Number of ready bots in bracket
     */
    uint32 GetAvailableCountForBracket(PoolBracket bracket, Faction faction,
                                        BotRole role = BotRole::Max) const;

    /**
     * @brief Get statistics for a specific bracket
     * @param bracket Level bracket
     * @return PoolBracketStats for the bracket
     */
    PoolBracketStats GetBracketStatistics(PoolBracket bracket) const;

    /**
     * @brief Get statistics for all brackets
     * @return AllPoolBracketStats containing all bracket info
     */
    AllPoolBracketStats GetAllBracketStatistics() const;

    /**
     * @brief Check if bracket has sufficient bots for a dungeon
     * @param bracket Level bracket
     * @param faction Faction
     * @return true if bracket can support 1 tank, 1 healer, 3 DPS
     */
    bool CanBracketSupportDungeon(PoolBracket bracket, Faction faction) const;

    /**
     * @brief Check if bracket has sufficient bots for a battleground
     * @param bracket Level bracket
     * @param allianceNeeded Alliance bots needed
     * @param hordeNeeded Horde bots needed
     * @return true if bracket can support the BG
     */
    bool CanBracketSupportBG(PoolBracket bracket, uint32 allianceNeeded, uint32 hordeNeeded) const;

    /**
     * @brief Get brackets with shortages (below 80% capacity)
     * @return Vector of brackets that need replenishment
     */
    std::vector<PoolBracket> GetBracketsWithShortage() const;

    /**
     * @brief Get the most depleted bracket
     * @return Bracket with lowest availability percentage
     */
    PoolBracket GetMostDepletedBracket() const;

    /**
     * @brief Get total pool size
     * @return Total number of slots (all states)
     */
    uint32 GetTotalPoolSize() const;

    /**
     * @brief Get count of ready bots
     * @return Number of bots ready for assignment
     */
    uint32 GetReadyCount() const;

    /**
     * @brief Get count of assigned bots
     * @return Number of bots currently in instances
     */
    uint32 GetAssignedCount() const;

    // ========================================================================
    // BOT ASSIGNMENT - PvE
    // ========================================================================

    /**
     * @brief Assign bots for dungeon (5-man)
     * @param dungeonId Dungeon ID
     * @param playerLevel Player's level (for matching)
     * @param playerFaction Player's faction
     * @param tanksNeeded Number of tanks needed
     * @param healersNeeded Number of healers needed
     * @param dpsNeeded Number of DPS needed
     * @return Vector of assigned bot GUIDs (empty if failed)
     */
    std::vector<ObjectGuid> AssignForDungeon(
        uint32 dungeonId,
        uint32 playerLevel,
        Faction playerFaction,
        uint32 tanksNeeded,
        uint32 healersNeeded,
        uint32 dpsNeeded);

    /**
     * @brief Assign bots for raid
     * @param raidId Raid ID
     * @param playerLevel Player's level
     * @param playerFaction Player's faction
     * @param tanksNeeded Number of tanks needed
     * @param healersNeeded Number of healers needed
     * @param dpsNeeded Number of DPS needed
     * @return Vector of assigned bot GUIDs
     */
    std::vector<ObjectGuid> AssignForRaid(
        uint32 raidId,
        uint32 playerLevel,
        Faction playerFaction,
        uint32 tanksNeeded,
        uint32 healersNeeded,
        uint32 dpsNeeded);

    // ========================================================================
    // BOT ASSIGNMENT - PvP
    // ========================================================================

    /**
     * @brief Assign bots for battleground (both factions)
     * @param bgTypeId Battleground type ID
     * @param bracketLevel Level bracket
     * @param allianceNeeded Alliance bots needed
     * @param hordeNeeded Horde bots needed
     * @param humanPlayerGuid Human player GUID for BG invitation tracking
     * @return BGAssignment with both faction bot lists
     */
    BGAssignment AssignForBattleground(
        uint32 bgTypeId,
        uint32 bracketLevel,
        uint32 allianceNeeded,
        uint32 hordeNeeded,
        ObjectGuid humanPlayerGuid = ObjectGuid::Empty);

    /**
     * @brief Assign bots for arena
     * @param arenaType Arena type (2v2, 3v3, 5v5)
     * @param bracketLevel Level bracket
     * @param playerFaction Player's faction
     * @param teammatesNeeded Teammate bots needed
     * @param opponentsNeeded Opponent bots needed
     * @return ArenaAssignment with teammates and opponents
     */
    ArenaAssignment AssignForArena(
        uint32 arenaType,
        uint32 bracketLevel,
        Faction playerFaction,
        uint32 teammatesNeeded,
        uint32 opponentsNeeded);

    // ========================================================================
    // BOT RELEASE
    // ========================================================================

    /**
     * @brief Release bots back to pool
     * @param bots Vector of bot GUIDs to release
     */
    void ReleaseBots(std::vector<ObjectGuid> const& bots);

    /**
     * @brief Release single bot back to pool
     * @param botGuid Bot GUID
     * @param success Whether the instance was completed successfully
     */
    void ReleaseBot(ObjectGuid botGuid, bool success = true);

    /**
     * @brief Release all bots from an instance
     * @param instanceId Instance ID
     */
    void ReleaseBotsFromInstance(uint32 instanceId);

    // ========================================================================
    // RESERVATION SYSTEM
    // ========================================================================

    /**
     * @brief Create reservation for upcoming instance
     * @param type Instance type
     * @param contentId Content ID (dungeon/raid/bg ID)
     * @param playerLevel Player's level
     * @param playerFaction Player's faction
     * @param tanksNeeded Tanks needed
     * @param healersNeeded Healers needed
     * @param dpsNeeded DPS needed
     * @param allianceNeeded Alliance bots (for PvP)
     * @param hordeNeeded Horde bots (for PvP)
     * @return Reservation ID (0 if failed)
     */
    uint32 CreateReservation(
        InstanceType type,
        uint32 contentId,
        uint32 playerLevel,
        Faction playerFaction,
        uint32 tanksNeeded,
        uint32 healersNeeded,
        uint32 dpsNeeded,
        uint32 allianceNeeded = 0,
        uint32 hordeNeeded = 0);

    /**
     * @brief Check if reservation can be fulfilled
     * @param reservationId Reservation ID
     * @return true if all reserved bots are still available
     */
    bool CanFulfillReservation(uint32 reservationId) const;

    /**
     * @brief Get fulfillment percentage for reservation
     * @param reservationId Reservation ID
     * @return Percentage of bots reserved (0-100)
     */
    float GetReservationFulfillment(uint32 reservationId) const;

    /**
     * @brief Fulfill reservation and get bots
     * @param reservationId Reservation ID
     * @return Vector of bot GUIDs (empty if failed)
     *
     * Transitions reserved bots to Assigned state.
     */
    std::vector<ObjectGuid> FulfillReservation(uint32 reservationId);

    /**
     * @brief Cancel reservation
     * @param reservationId Reservation ID
     *
     * Returns reserved bots to Ready state.
     */
    void CancelReservation(uint32 reservationId);

    /**
     * @brief Get estimated wait time for content
     * @param type Instance type
     * @param contentId Content ID
     * @param tanksNeeded Tanks needed
     * @param healersNeeded Healers needed
     * @param dpsNeeded DPS needed
     * @return Estimated wait time (0 if instant)
     */
    std::chrono::milliseconds GetEstimatedWaitTime(
        InstanceType type,
        uint32 contentId,
        uint32 tanksNeeded,
        uint32 healersNeeded,
        uint32 dpsNeeded) const;

    // ========================================================================
    // QUERIES
    // ========================================================================

    /**
     * @brief Get slot info for a bot
     * @param botGuid Bot GUID
     * @return Pointer to slot (nullptr if not found)
     */
    InstanceBotSlot const* GetSlot(ObjectGuid botGuid) const;

    /**
     * @brief Check if bot is from pool
     * @param botGuid Bot GUID
     * @return true if bot is a pool bot
     */
    bool IsPoolBot(ObjectGuid botGuid) const;

    /**
     * @brief Get bot's current instance
     * @param botGuid Bot GUID
     * @return Instance ID (0 if not assigned)
     */
    uint32 GetBotInstanceId(ObjectGuid botGuid) const;

    /**
     * @brief Check if pool can provide bots for content
     * @param type Instance type
     * @param tanksNeeded Tanks needed
     * @param healersNeeded Healers needed
     * @param dpsNeeded DPS needed
     * @return true if pool has sufficient bots
     */
    bool CanProvideBotsFor(
        InstanceType type,
        uint32 tanksNeeded,
        uint32 healersNeeded,
        uint32 dpsNeeded) const;

    // ========================================================================
    // STATISTICS
    // ========================================================================

    /**
     * @brief Get current statistics snapshot
     * @return Pool statistics
     */
    PoolStatistics GetStatistics() const;

    /**
     * @brief Print status report to log
     */
    void PrintStatusReport() const;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /**
     * @brief Get current configuration
     * @return Configuration reference
     */
    InstanceBotPoolConfig const& GetConfig() const { return _config; }

    /**
     * @brief Set configuration
     * @param config New configuration
     */
    void SetConfig(InstanceBotPoolConfig const& config);

    /**
     * @brief Check if pool is enabled
     * @return true if enabled and initialized
     */
    bool IsEnabled() const { return _config.enabled && _initialized.load(); }

    /**
     * @brief Check if pool is initialized
     * @return true if initialized
     */
    bool IsInitialized() const { return _initialized.load(); }

    // ========================================================================
    // CALLBACKS
    // ========================================================================

    /// Callback for when bot assignment fails
    using AssignmentFailedCallback = std::function<void(InstanceType, uint32, std::string const&)>;

    /**
     * @brief Set callback for assignment failures
     * @param callback Callback function
     */
    void SetAssignmentFailedCallback(AssignmentFailedCallback callback);

    /// Callback for when pool needs overflow (JIT)
    /// Parameters: role, faction, bracket, count
    using OverflowNeededCallback = std::function<void(BotRole, Faction, PoolBracket, uint32)>;

    /**
     * @brief Set callback for overflow requirements
     * @param callback Callback function (role, faction, bracket, count)
     */
    void SetOverflowNeededCallback(OverflowNeededCallback callback);

private:
    // Friend classes that need internal access
    friend class InstanceBotOrchestrator;
    friend class JITBotFactory;

    InstanceBotPool() = default;
    ~InstanceBotPool() = default;
    InstanceBotPool(InstanceBotPool const&) = delete;
    InstanceBotPool& operator=(InstanceBotPool const&) = delete;

    // ========================================================================
    // INTERNAL METHODS - Bot Creation
    // ========================================================================

    /**
     * @brief Create a new pool bot for a specific bracket
     * @param role Bot role
     * @param faction Faction (Alliance/Horde)
     * @param bracket Target level bracket
     * @param deferWarmup If true, skip immediate login attempt (ProcessWarmingRetries will handle it gradually)
     * @return Bot GUID (empty if failed)
     */
    ObjectGuid CreatePoolBot(BotRole role, Faction faction, PoolBracket bracket, bool deferWarmup = false);

    /**
     * @brief Create a new pool bot (legacy signature for compatibility)
     * @param role Bot role
     * @param poolType Pool type
     * @param level Target level
     * @param deferWarmup If true, skip immediate login attempt
     * @return Bot GUID (empty if failed)
     */
    ObjectGuid CreatePoolBotLegacy(BotRole role, PoolType poolType, uint32 level, bool deferWarmup = false);

    /**
     * @brief Warm up a single bot (login)
     * @param botGuid Bot GUID
     * @return true if warmup started successfully
     */
    bool WarmUpBot(ObjectGuid botGuid);

    /**
     * @brief Called when bot warmup completes
     * @param botGuid Bot GUID
     * @param success Whether warmup succeeded
     */
    void OnBotWarmupComplete(ObjectGuid botGuid, bool success);

    // ========================================================================
    // INTERNAL METHODS - Bot Selection
    // ========================================================================

    /**
     * @brief Select best available bot matching criteria
     * @param role Required role
     * @param faction Required faction
     * @param level Target level
     * @param minGearScore Minimum gear score
     * @return Best matching bot GUID (empty if none available)
     */
    ObjectGuid SelectBestBot(BotRole role, Faction faction, uint32 level, uint32 minGearScore);

    /**
     * @brief Select best bot from a specific bracket (O(1) lookup)
     * @param role Required role
     * @param faction Required faction
     * @param bracket Target level bracket
     * @return Best matching bot GUID (empty if none available)
     */
    ObjectGuid SelectBestBotFromBracket(BotRole role, Faction faction, PoolBracket bracket);

    /**
     * @brief Select multiple bots matching criteria
     * @param role Required role
     * @param faction Required faction
     * @param level Target level
     * @param count Number of bots needed
     * @param minGearScore Minimum gear score
     * @return Vector of selected bot GUIDs
     */
    std::vector<ObjectGuid> SelectBots(BotRole role, Faction faction,
                                        uint32 level, uint32 count, uint32 minGearScore);

    /**
     * @brief Select multiple bots from a specific bracket (O(k) where k = bots in bracket)
     * @param role Required role
     * @param faction Required faction
     * @param bracket Target level bracket
     * @param count Number of bots needed
     * @return Vector of selected bot GUIDs
     */
    std::vector<ObjectGuid> SelectBotsFromBracket(BotRole role, Faction faction,
                                                   PoolBracket bracket, uint32 count);

    /**
     * @brief Assign bot to instance
     * @param botGuid Bot GUID
     * @param instanceId Instance ID
     * @param contentId Content ID
     * @param type Instance type
     * @param targetLevel Target level for the bot (player's level to match)
     * @param humanPlayerGuid Human player GUID for BG invitation tracking (optional)
     * @return true if assignment succeeded
     */
    bool AssignBot(ObjectGuid botGuid, uint32 instanceId, uint32 contentId, InstanceType type, uint32 targetLevel,
                   ObjectGuid humanPlayerGuid = ObjectGuid::Empty);

    // ========================================================================
    // INTERNAL METHODS - ReadyIndex Management
    // ========================================================================

    /**
     * @brief Add bot to ready index for O(1) lookup
     * @param botGuid Bot GUID
     * @param role Bot role
     * @param faction Bot faction
     * @param bracket Level bracket
     */
    void AddToReadyIndex(ObjectGuid botGuid, BotRole role, Faction faction, PoolBracket bracket);

    /**
     * @brief Register a JIT-created bot with the pool tracking system
     * @param botGuid Bot GUID (must already be logged in via JITBotFactory)
     * @param accountId Bot's account ID
     * @param role Bot role
     * @param faction Bot faction
     * @param bracket Target level bracket
     *
     * CRITICAL: This method properly integrates JIT bots with the pool by:
     * 1. Creating InstanceBotSlot entry
     * 2. Adding to _slots map
     * 3. Adding to ready index
     * 4. Updating _bracketCounts
     *
     * Without this, ReplenishPool() would keep detecting shortages and
     * trigger infinite JIT bot creation (the 1200 bots bug).
     */
    void RegisterJITBot(ObjectGuid botGuid, uint32 accountId, BotRole role, Faction faction, PoolBracket bracket);

    /**
     * @brief Remove bot from ready index
     * @param botGuid Bot GUID
     * @param role Bot role
     * @param faction Bot faction
     * @param bracket Level bracket
     */
    void RemoveFromReadyIndex(ObjectGuid botGuid, BotRole role, Faction faction, PoolBracket bracket);

    /**
     * @brief Rebuild entire ready index from _slots (after load or corruption)
     */
    void RebuildReadyIndex();

    /**
     * @brief Update bracket counts from ready index
     */
    void UpdateBracketCounts();

    // ========================================================================
    // INTERNAL METHODS - Pool Maintenance
    // ========================================================================

    /**
     * @brief Process incremental warmup (batched bot creation to prevent freeze detector)
     *
     * Creates WARMUP_BOTS_PER_TICK bots per Update() tick instead of all 800 at once.
     * This prevents blocking the world thread for 60+ seconds which triggers the freeze detector.
     */
    void ProcessIncrementalWarmup();

    /**
     * @brief Retry warmup for bots stuck in Warming state (async DB commit delay)
     */
    void ProcessWarmingRetries();

    /**
     * @brief Process cooldown expirations
     */
    void ProcessCooldowns();

    /**
     * @brief Process reservation timeouts
     */
    void ProcessReservations();

    /**
     * @brief Update statistics
     */
    void UpdateStatistics();

    /**
     * @brief Check hourly reset
     */
    void CheckHourlyReset();

    // ========================================================================
    // INTERNAL METHODS - Database
    // ========================================================================

    /**
     * @brief Sync pool state to database
     */
    void SyncToDatabase();

    /**
     * @brief Load pool state from database
     */
    void LoadFromDatabase();

    // ========================================================================
    // DATA MEMBERS - Configuration
    // ========================================================================

    InstanceBotPoolConfig _config;

    // ========================================================================
    // DATA MEMBERS - Pool Storage
    // ========================================================================

    /// Pool slots indexed by bot GUID
    /// Using unordered_map with shared_mutex for thread safety
    std::unordered_map<ObjectGuid, InstanceBotSlot> _slots;
    mutable std::shared_mutex _slotsMutex;

    /// Quick lookup indices for O(1) bracket lookup
    /// role -> faction -> level bracket -> list of ready bot GUIDs
    ///
    /// Example access: _readyIndex[BotRole::Tank][Faction::Alliance][PoolBracket::Bracket_20_29]
    /// Returns: vector of ready tank GUIDs for Alliance level 20-29 bracket
    using BracketGuidMap = std::array<std::vector<ObjectGuid>, NUM_LEVEL_BRACKETS>;
    using FactionBracketMap = std::array<BracketGuidMap, static_cast<size_t>(Faction::Max)>;
    using ReadyIndex = std::array<FactionBracketMap, static_cast<size_t>(BotRole::Max)>;
    ReadyIndex _readyIndex;
    mutable std::shared_mutex _readyIndexMutex;

    // ========================================================================
    // DATA MEMBERS - Per-Bracket Tracking
    // ========================================================================

    /// Per-bracket bot counts (for quick availability checks without iteration)
    struct BracketCounts
    {
        // Per-faction ready counts
        std::array<uint32, NUM_LEVEL_BRACKETS> allianceReady{};
        std::array<uint32, NUM_LEVEL_BRACKETS> hordeReady{};
        std::array<uint32, NUM_LEVEL_BRACKETS> allianceTotal{};
        std::array<uint32, NUM_LEVEL_BRACKETS> hordeTotal{};

        // Per-role ready counts [bracket][role]
        static constexpr size_t NUM_ROLES = static_cast<size_t>(BotRole::Max);
        std::array<std::array<uint32, NUM_ROLES>, NUM_LEVEL_BRACKETS> allianceRoleReady{};
        std::array<std::array<uint32, NUM_ROLES>, NUM_LEVEL_BRACKETS> hordeRoleReady{};

        void Reset()
        {
            allianceReady.fill(0);
            hordeReady.fill(0);
            allianceTotal.fill(0);
            hordeTotal.fill(0);
            for (auto& bracket : allianceRoleReady)
                bracket.fill(0);
            for (auto& bracket : hordeRoleReady)
                bracket.fill(0);
        }

        uint32 GetReady(PoolBracket bracket, Faction faction) const
        {
            auto idx = static_cast<size_t>(bracket);
            return (faction == Faction::Alliance) ? allianceReady[idx] : hordeReady[idx];
        }

        uint32 GetTotal(PoolBracket bracket, Faction faction) const
        {
            auto idx = static_cast<size_t>(bracket);
            return (faction == Faction::Alliance) ? allianceTotal[idx] : hordeTotal[idx];
        }

        uint32 GetReadyByRole(PoolBracket bracket, Faction faction, BotRole role) const
        {
            auto bracketIdx = static_cast<size_t>(bracket);
            auto roleIdx = static_cast<size_t>(role);
            if (roleIdx >= NUM_ROLES) return 0;
            return (faction == Faction::Alliance)
                ? allianceRoleReady[bracketIdx][roleIdx]
                : hordeRoleReady[bracketIdx][roleIdx];
        }

        void IncrementReady(PoolBracket bracket, Faction faction, BotRole role)
        {
            auto bracketIdx = static_cast<size_t>(bracket);
            auto roleIdx = static_cast<size_t>(role);
            if (faction == Faction::Alliance)
            {
                ++allianceReady[bracketIdx];
                if (roleIdx < NUM_ROLES)
                    ++allianceRoleReady[bracketIdx][roleIdx];
            }
            else
            {
                ++hordeReady[bracketIdx];
                if (roleIdx < NUM_ROLES)
                    ++hordeRoleReady[bracketIdx][roleIdx];
            }
        }

        void DecrementReady(PoolBracket bracket, Faction faction, BotRole role)
        {
            auto bracketIdx = static_cast<size_t>(bracket);
            auto roleIdx = static_cast<size_t>(role);
            if (faction == Faction::Alliance)
            {
                if (allianceReady[bracketIdx] > 0) --allianceReady[bracketIdx];
                if (roleIdx < NUM_ROLES && allianceRoleReady[bracketIdx][roleIdx] > 0)
                    --allianceRoleReady[bracketIdx][roleIdx];
            }
            else
            {
                if (hordeReady[bracketIdx] > 0) --hordeReady[bracketIdx];
                if (roleIdx < NUM_ROLES && hordeRoleReady[bracketIdx][roleIdx] > 0)
                    --hordeRoleReady[bracketIdx][roleIdx];
            }
        }

        void IncrementTotal(PoolBracket bracket, Faction faction)
        {
            auto idx = static_cast<size_t>(bracket);
            if (faction == Faction::Alliance)
                ++allianceTotal[idx];
            else
                ++hordeTotal[idx];
        }

        void DecrementTotal(PoolBracket bracket, Faction faction)
        {
            auto idx = static_cast<size_t>(bracket);
            if (faction == Faction::Alliance)
            {
                if (allianceTotal[idx] > 0) --allianceTotal[idx];
            }
            else
            {
                if (hordeTotal[idx] > 0) --hordeTotal[idx];
            }
        }
    };
    BracketCounts _bracketCounts;
    mutable std::shared_mutex _bracketCountsMutex;

    // ========================================================================
    // DATA MEMBERS - Reservations
    // ========================================================================

    std::unordered_map<uint32, Reservation> _reservations;
    std::atomic<uint32> _nextReservationId{1};
    mutable std::mutex _reservationMutex;

    // ========================================================================
    // DATA MEMBERS - Statistics
    // ========================================================================

    mutable PoolStatistics _stats;
    mutable std::shared_mutex _statsMutex;
    std::atomic<bool> _statsDirty{true};

    // ========================================================================
    // DATA MEMBERS - Hourly Tracking
    // ========================================================================

    std::chrono::system_clock::time_point _hourStart;
    std::chrono::system_clock::time_point _dayStart;

    // ========================================================================
    // DATA MEMBERS - Timing Accumulators
    // ========================================================================

    uint32 _updateAccumulator = 0;
    uint32 _dbSyncAccumulator = 0;
    uint32 _statsAccumulator = 0;
    uint32 _replenishAccumulator = 0;

    // ========================================================================
    // DATA MEMBERS - State
    // ========================================================================

    std::atomic<bool> _initialized{false};
    std::atomic<bool> _warmingInProgress{false};
    std::atomic<bool> _shuttingDown{false};
    std::atomic<bool> _warmupPending{false};  // Deferred warmup until world is running

    // ========================================================================
    // DATA MEMBERS - Incremental Warmup State
    // ========================================================================
    // Batched warmup to prevent freeze detector (creates ~5-10 bots per tick)
    // instead of all 800 at once which would block the world thread for 60+ seconds

    std::atomic<bool> _incrementalWarmupActive{false};  // Currently in incremental warmup phase
    uint8 _warmupBracketIndex{0};                        // Current bracket (0-7)
    uint8 _warmupFactionPhase{0};                        // 0=Alliance, 1=Horde
    uint8 _warmupRoleIndex{0};                           // 0=Tank, 1=Healer, 2=DPS
    uint32 _warmupRoleCount{0};                          // Bots created for current role
    uint32 _warmupTotalCreated{0};                       // Total bots created so far
    uint32 _warmupTotalTarget{0};                        // Total bots to create
    static constexpr uint32 WARMUP_BOTS_PER_TICK = 5;    // Bots to create per Update() tick

    // ========================================================================
    // DATA MEMBERS - Callbacks
    // ========================================================================

    AssignmentFailedCallback _assignmentFailedCallback;
    OverflowNeededCallback _overflowNeededCallback;
};

} // namespace Playerbot

#define sInstanceBotPool Playerbot::InstanceBotPool::Instance()
