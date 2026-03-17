/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * @file ContentRequirements.h
 * @brief Content requirements database for instance bot assignment
 *
 * Defines the bot requirements for all content types:
 * - Dungeons (5-man, mythic+)
 * - Raids (10-40 man)
 * - Battlegrounds (10-80 players, both factions)
 * - Arenas (2v2, 3v3)
 *
 * Requirements Hierarchy:
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │                    CONTENT REQUIREMENTS                                  │
 * ├─────────────────────────────────────────────────────────────────────────┤
 * │                                                                         │
 * │   DUNGEONS                          RAIDS                               │
 * │   ┌──────────────────┐              ┌──────────────────┐                │
 * │   │ Normal (5-man)   │              │ 10-man           │                │
 * │   │ - 1 Tank         │              │ - 2 Tanks        │                │
 * │   │ - 1 Healer       │              │ - 2-3 Healers    │                │
 * │   │ - 3 DPS          │              │ - 5-6 DPS        │                │
 * │   ├──────────────────┤              ├──────────────────┤                │
 * │   │ Heroic           │              │ 25-man           │                │
 * │   │ - Higher GS req  │              │ - 2-3 Tanks      │                │
 * │   ├──────────────────┤              │ - 5-7 Healers    │                │
 * │   │ Mythic+          │              │ - 15-18 DPS      │                │
 * │   │ - Scaling diff   │              ├──────────────────┤                │
 * │   └──────────────────┘              │ 40-man           │                │
 * │                                     │ - 4-5 Tanks      │                │
 * │   BATTLEGROUNDS                     │ - 10-12 Healers  │                │
 * │   ┌──────────────────┐              │ - 23-26 DPS      │                │
 * │   │ Warsong Gulch    │              └──────────────────┘                │
 * │   │ - 10v10          │                                                  │
 * │   ├──────────────────┤              ARENAS                              │
 * │   │ Arathi Basin     │              ┌──────────────────┐                │
 * │   │ - 15v15          │              │ 2v2              │                │
 * │   ├──────────────────┤              │ - Any comp       │                │
 * │   │ Alterac Valley   │              ├──────────────────┤                │
 * │   │ - 40v40          │              │ 3v3              │                │
 * │   │ - Both factions! │              │ - Healer+2 DPS   │                │
 * │   ├──────────────────┤              └──────────────────┘                │
 * │   │ Isle of Conquest │                                                  │
 * │   │ - 40v40          │                                                  │
 * │   └──────────────────┘                                                  │
 * │                                                                         │
 * └─────────────────────────────────────────────────────────────────────────┘
 */

#pragma once

#include "Define.h"
#include "PoolSlotState.h"
#include <atomic>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace Playerbot
{

// ============================================================================
// CONTENT REQUIREMENT
// ============================================================================

/**
 * @brief Requirements for a specific content type
 */
struct TC_GAME_API ContentRequirement
{
    // ========================================================================
    // IDENTITY
    // ========================================================================

    uint32 contentId = 0;               ///< Content ID (dungeon/raid/bg ID)
    std::string contentName;            ///< Human-readable name
    InstanceType type = InstanceType::Dungeon;  ///< Content type

    // ========================================================================
    // PLAYER LIMITS
    // ========================================================================

    uint32 minPlayers = 1;              ///< Minimum players to start
    uint32 maxPlayers = 5;              ///< Maximum players allowed

    // ========================================================================
    // LEVEL REQUIREMENTS
    // ========================================================================

    uint32 minLevel = 1;                ///< Minimum level required
    uint32 maxLevel = 80;               ///< Maximum level allowed
    uint32 recommendedLevel = 80;       ///< Recommended level for scaling

    // ========================================================================
    // ROLE REQUIREMENTS (PVE)
    // ========================================================================

    uint32 minTanks = 0;                ///< Minimum tanks required
    uint32 maxTanks = 0;                ///< Maximum tanks allowed
    uint32 recommendedTanks = 0;        ///< Recommended number of tanks

    uint32 minHealers = 0;              ///< Minimum healers required
    uint32 maxHealers = 0;              ///< Maximum healers allowed
    uint32 recommendedHealers = 0;      ///< Recommended number of healers

    uint32 minDPS = 0;                  ///< Minimum DPS required
    uint32 maxDPS = 0;                  ///< Maximum DPS allowed
    uint32 recommendedDPS = 0;          ///< Recommended number of DPS

    // ========================================================================
    // GEAR REQUIREMENTS
    // ========================================================================

    uint32 minGearScore = 0;            ///< Minimum gear score
    uint32 recommendedGearScore = 0;    ///< Recommended gear score

    // ========================================================================
    // PVP REQUIREMENTS
    // ========================================================================

    bool requiresBothFactions = false;  ///< Requires Alliance AND Horde
    uint32 playersPerFaction = 0;       ///< Players needed per faction (for BGs)

    // ========================================================================
    // TIMING
    // ========================================================================

    uint32 estimatedDurationMinutes = 30;   ///< Estimated run time
    uint32 warmupTimeSeconds = 60;          ///< Time before content starts

    // ========================================================================
    // DIFFICULTY
    // ========================================================================

    uint8 difficulty = 0;               ///< Difficulty level (0=normal, 1=heroic, etc.)
    bool mythicPlus = false;            ///< Is this Mythic+ content
    uint32 mythicPlusLevel = 0;         ///< Mythic+ key level

    // ========================================================================
    // METHODS
    // ========================================================================

    /**
     * @brief Get total players recommended
     */
    uint32 GetTotalRecommended() const
    {
        return recommendedTanks + recommendedHealers + recommendedDPS;
    }

    /**
     * @brief Get total bots needed given current players
     */
    uint32 GetBotsNeeded(uint32 currentPlayers) const
    {
        if (currentPlayers >= maxPlayers)
            return 0;
        return maxPlayers - currentPlayers;
    }

    /**
     * @brief Check if requirements are valid
     */
    bool IsValid() const
    {
        if (contentId == 0)
            return false;
        if (maxPlayers < minPlayers)
            return false;
        if (type == InstanceType::Battleground || type == InstanceType::Arena)
            return playersPerFaction > 0 || maxPlayers > 0;
        return (recommendedTanks + recommendedHealers + recommendedDPS) > 0;
    }

    /**
     * @brief Get string representation
     */
    std::string ToString() const;
};

// ============================================================================
// BOTS NEEDED CALCULATION
// ============================================================================

/**
 * @brief Result of calculating bots needed for content
 */
struct BotsNeeded
{
    // PvE roles
    uint32 tanksNeeded = 0;
    uint32 healersNeeded = 0;
    uint32 dpsNeeded = 0;

    // PvP factions
    uint32 allianceNeeded = 0;
    uint32 hordeNeeded = 0;

    // Total
    uint32 totalNeeded = 0;

    // Gear requirement
    uint32 minGearScore = 0;

    /**
     * @brief Check if any bots are needed
     */
    bool NeedsBots() const { return totalNeeded > 0; }

    /**
     * @brief Get breakdown string
     */
    std::string ToString() const;
};

// ============================================================================
// CURRENT GROUP STATE
// ============================================================================

/**
 * @brief Current state of a group for bot calculation
 */
struct GroupState
{
    uint32 totalPlayers = 0;            ///< Total human players
    uint32 tanks = 0;                   ///< Current tank count
    uint32 healers = 0;                 ///< Current healer count
    uint32 dps = 0;                     ///< Current DPS count
    uint32 alliancePlayers = 0;         ///< Alliance players (PvP)
    uint32 hordePlayers = 0;            ///< Horde players (PvP)
    Faction leaderFaction = Faction::Alliance;  ///< Group leader's faction
    uint32 avgGearScore = 0;            ///< Average gear score
    uint32 avgLevel = 80;               ///< Average level
};

// ============================================================================
// CONTENT REQUIREMENT DATABASE
// ============================================================================

/**
 * @brief Database of content requirements for all instance types
 *
 * Provides requirements for dungeons, raids, battlegrounds, and arenas.
 * Loaded from database at startup.
 */
class TC_GAME_API ContentRequirementDatabase
{
public:
    /**
     * @brief Get singleton instance
     */
    static ContentRequirementDatabase* Instance();

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /**
     * @brief Initialize and load requirements
     * @return true if successful
     */
    bool Initialize();

    /**
     * @brief Shutdown and cleanup
     */
    void Shutdown();

    /**
     * @brief Reload requirements from database
     */
    void Reload();

    // ========================================================================
    // REQUIREMENT ACCESS
    // ========================================================================

    /**
     * @brief Get dungeon requirements
     * @param dungeonId LFG dungeon ID
     * @return Requirement or nullptr
     */
    ContentRequirement const* GetDungeonRequirement(uint32 dungeonId) const;

    /**
     * @brief Get raid requirements
     * @param raidId Map ID of raid
     * @return Requirement or nullptr
     */
    ContentRequirement const* GetRaidRequirement(uint32 raidId) const;

    /**
     * @brief Get battleground requirements
     * @param bgTypeId Battleground type ID
     * @return Requirement or nullptr
     */
    ContentRequirement const* GetBattlegroundRequirement(uint32 bgTypeId) const;

    /**
     * @brief Get arena requirements
     * @param arenaType Arena type (2, 3, or 5)
     * @return Requirement or nullptr
     */
    ContentRequirement const* GetArenaRequirement(uint32 arenaType) const;

    /**
     * @brief Get requirement by type and ID
     * @param type Instance type
     * @param contentId Content ID
     * @return Requirement or nullptr
     */
    ContentRequirement const* GetRequirement(InstanceType type, uint32 contentId) const;

    // ========================================================================
    // BOTS CALCULATION
    // ========================================================================

    /**
     * @brief Calculate bots needed for content
     * @param requirement Content requirement
     * @param groupState Current group state
     * @return Bots needed structure
     */
    BotsNeeded CalculateBotsNeeded(
        ContentRequirement const* requirement,
        GroupState const& groupState) const;

    /**
     * @brief Calculate bots for dungeon
     * @param dungeonId Dungeon ID
     * @param groupState Current group state
     * @return Bots needed
     */
    BotsNeeded CalculateDungeonBots(
        uint32 dungeonId,
        GroupState const& groupState) const;

    /**
     * @brief Calculate bots for raid
     * @param raidId Raid ID
     * @param groupState Current group state
     * @return Bots needed
     */
    BotsNeeded CalculateRaidBots(
        uint32 raidId,
        GroupState const& groupState) const;

    /**
     * @brief Calculate bots for battleground
     * @param bgTypeId Battleground type
     * @param groupState Current group state
     * @return Bots needed (both factions)
     */
    BotsNeeded CalculateBattlegroundBots(
        uint32 bgTypeId,
        GroupState const& groupState) const;

    /**
     * @brief Calculate bots for arena
     * @param arenaType Arena type
     * @param groupState Current group state
     * @param needOpponents Whether to include opponents
     * @return Bots needed
     */
    BotsNeeded CalculateArenaBots(
        uint32 arenaType,
        GroupState const& groupState,
        bool needOpponents) const;

    // ========================================================================
    // QUERIES
    // ========================================================================

    /**
     * @brief Get all dungeon requirements
     */
    std::vector<ContentRequirement const*> GetAllDungeons() const;

    /**
     * @brief Get all raid requirements
     */
    std::vector<ContentRequirement const*> GetAllRaids() const;

    /**
     * @brief Get all battleground requirements
     */
    std::vector<ContentRequirement const*> GetAllBattlegrounds() const;

    /**
     * @brief Get all arena requirements
     */
    std::vector<ContentRequirement const*> GetAllArenas() const;

    /**
     * @brief Get dungeons for level range
     */
    std::vector<ContentRequirement const*> GetDungeonsForLevel(uint32 level) const;

    /**
     * @brief Get battlegrounds for level bracket
     */
    std::vector<ContentRequirement const*> GetBattlegroundsForLevel(uint32 level) const;

    // ========================================================================
    // STATISTICS
    // ========================================================================

    /**
     * @brief Get total requirement count
     */
    uint32 GetTotalCount() const;

    /**
     * @brief Print statistics to log
     */
    void PrintStatistics() const;

private:
    ContentRequirementDatabase() = default;
    ~ContentRequirementDatabase() = default;
    ContentRequirementDatabase(ContentRequirementDatabase const&) = delete;
    ContentRequirementDatabase& operator=(ContentRequirementDatabase const&) = delete;

    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /**
     * @brief Load requirements from database
     */
    void LoadFromDatabase();

    /**
     * @brief Create default dungeon requirements
     */
    void CreateDefaultDungeons();

    /**
     * @brief Create default raid requirements
     */
    void CreateDefaultRaids();

    /**
     * @brief Create default battleground requirements
     */
    void CreateDefaultBattlegrounds();

    /**
     * @brief Create default arena requirements
     */
    void CreateDefaultArenas();

    /**
     * @brief Add a requirement
     */
    void AddRequirement(ContentRequirement requirement);

    /**
     * @brief Calculate optimal role distribution
     */
    void CalculateOptimalRoles(
        ContentRequirement const* req,
        GroupState const& groupState,
        BotsNeeded& result) const;

    /**
     * @brief Get a random actual BG requirement for Random Battleground
     * @note Must be called with mutex already held (shared_lock)
     * @return Random BG requirement (AV, WSG, AB, etc.)
     */
    ContentRequirement const* GetRandomBattlegroundRequirement_Locked() const;

    // ========================================================================
    // DATA MEMBERS
    // ========================================================================

    /// Requirements by type and content ID
    std::unordered_map<uint32, ContentRequirement> _dungeons;
    std::unordered_map<uint32, ContentRequirement> _raids;
    std::unordered_map<uint32, ContentRequirement> _battlegrounds;
    std::unordered_map<uint32, ContentRequirement> _arenas;

    /// Thread safety
    mutable std::shared_mutex _mutex;

    /// Initialization state
    std::atomic<bool> _initialized{false};
};

} // namespace Playerbot

#define sContentRequirementDb Playerbot::ContentRequirementDatabase::Instance()
