/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * @file PlayerActivityTracker.h
 * @brief Tracks real player activity for demand-driven bot spawning
 *
 * The PlayerActivityTracker monitors where real players are playing
 * to enable demand-driven bot spawning. Bots are spawned in areas
 * where players are active, creating a more populated and immersive
 * game world.
 *
 * Thread Safety:
 * - All public methods are thread-safe
 * - Uses parallel hashmap for concurrent access
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "../BotLifecycleState.h"
#include "Character/BotLevelDistribution.h"
#include <parallel_hashmap/phmap.h>
#include <atomic>
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <array>
#include <map>

class Player;

namespace Playerbot
{

/**
 * @brief Information about a player's current activity
 */
struct PlayerActivity
{
    ObjectGuid playerGuid;
    std::string playerName;
    uint32 zoneId = 0;
    uint32 areaId = 0;
    uint32 mapId = 0;
    uint32 level = 0;
    uint8 playerClass = 0;
    bool isInGroup = false;
    bool isInInstance = false;
    bool isInBattleground = false;
    std::chrono::system_clock::time_point lastUpdate;
    bool isActive = false;  // Updated within stale threshold
};

/**
 * @brief Activity summary for a zone
 */
struct ZoneActivitySummary
{
    uint32 zoneId = 0;
    std::string zoneName;
    uint32 playerCount = 0;
    uint32 averageLevel = 0;
    uint32 minLevel = 0;
    uint32 maxLevel = 0;
    std::map<uint8, uint32> classCounts;  // class -> count
    bool hasActivePlayers = false;
};

/**
 * @brief Activity summary for a level bracket
 */
struct BracketActivitySummary
{
    ExpansionTier tier = ExpansionTier::Starting;
    uint32 playerCount = 0;
    std::vector<uint32> activeZones;
    float activityScore = 0.0f;  // Weighted score
};

/**
 * @brief Configuration for activity tracking
 */
struct ActivityTrackerConfig
{
    bool enabled = true;
    uint32 staleThresholdSeconds = 300;  // 5 minutes
    uint32 updateIntervalMs = 10000;     // 10 seconds
    uint32 cleanupIntervalMs = 60000;    // 1 minute
    bool trackInstances = true;
    bool trackBattlegrounds = true;
    bool logActivityChanges = false;
};

/**
 * @brief Tracks real player activity
 *
 * Singleton class monitoring player locations and activity.
 */
class TC_GAME_API PlayerActivityTracker
{
public:
    /**
     * @brief Get singleton instance
     */
    static PlayerActivityTracker* Instance();

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /**
     * @brief Initialize the tracker
     * @return true if successful
     */
    bool Initialize();

    /**
     * @brief Shutdown and cleanup
     */
    void Shutdown();

    /**
     * @brief Periodic update
     * @param diff Time since last update in milliseconds
     */
    void Update(uint32 diff);

    /**
     * @brief Load configuration
     */
    void LoadConfig();

    // ========================================================================
    // EVENT TRACKING
    // ========================================================================

    /**
     * @brief Called when a player logs in
     * @param player The player
     */
    void OnPlayerLogin(Player* player);

    /**
     * @brief Called when a player logs out
     * @param player The player
     */
    void OnPlayerLogout(Player* player);

    /**
     * @brief Called when a player changes zone
     * @param player The player
     * @param newZoneId New zone ID
     * @param newAreaId New area ID
     */
    void OnPlayerZoneChange(Player* player, uint32 newZoneId, uint32 newAreaId);

    /**
     * @brief Called when a player levels up
     * @param player The player
     * @param newLevel New level
     */
    void OnPlayerLevelUp(Player* player, uint32 newLevel);

    /**
     * @brief Called when a player enters/leaves a group
     * @param player The player
     * @param isInGroup Whether now in group
     */
    void OnPlayerGroupChange(Player* player, bool isInGroup);

    /**
     * @brief Update player activity (called periodically for each player)
     * @param player The player
     */
    void UpdatePlayerActivity(Player* player);

    // ========================================================================
    // ACTIVITY QUERIES
    // ========================================================================

    /**
     * @brief Get player count by zone
     * @return Map of zoneId -> player count
     */
    std::map<uint32, uint32> GetPlayerCountByZone() const;

    /**
     * @brief Get player count by bracket
     * @return Map of bracket tier -> player count
     */
    std::map<ExpansionTier, uint32> GetPlayerCountByBracket() const;

    /**
     * @brief Get zones with highest activity
     * @param maxCount Maximum zones to return
     * @return Zone IDs sorted by activity (highest first)
     */
    std::vector<uint32> GetHighActivityZones(uint32 maxCount = 10) const;

    /**
     * @brief Get zones with players at a specific level range
     * @param targetLevel Target level
     * @param range Level range (+/-)
     * @return Zone IDs with players in level range
     */
    std::vector<uint32> GetZonesWithPlayersAtLevel(uint32 targetLevel, uint32 range = 5) const;

    /**
     * @brief Get level distribution of active players
     * @return Array of player counts per level (0-80)
     */
    std::array<uint32, 81> GetPlayerLevelDistribution() const;

    /**
     * @brief Get activity summary for a zone
     * @param zoneId Zone to query
     * @return Activity summary
     */
    ZoneActivitySummary GetZoneActivitySummary(uint32 zoneId) const;

    /**
     * @brief Get activity summary for a bracket
     * @param tier Bracket tier
     * @return Activity summary
     */
    BracketActivitySummary GetBracketActivitySummary(ExpansionTier tier) const;

    /**
     * @brief Get total active player count
     * @return Number of active players
     */
    uint32 GetActivePlayerCount() const;

    /**
     * @brief Check if a zone has active players
     * @param zoneId Zone to check
     * @return true if zone has active players
     */
    bool HasActivePlayersInZone(uint32 zoneId) const;

    /**
     * @brief Get player activity info
     * @param playerGuid Player to query
     * @return Activity info (or empty if not tracked)
     */
    PlayerActivity GetPlayerActivity(ObjectGuid playerGuid) const;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /**
     * @brief Get current configuration
     */
    ActivityTrackerConfig const& GetConfig() const { return _config; }

    /**
     * @brief Set configuration
     */
    void SetConfig(ActivityTrackerConfig const& config);

    // ========================================================================
    // STATISTICS
    // ========================================================================

    /**
     * @brief Print activity report
     */
    void PrintActivityReport() const;

    /**
     * @brief Get total players tracked (including stale)
     */
    uint32 GetTotalTrackedPlayers() const;

private:
    PlayerActivityTracker() = default;
    ~PlayerActivityTracker() = default;
    PlayerActivityTracker(PlayerActivityTracker const&) = delete;
    PlayerActivityTracker& operator=(PlayerActivityTracker const&) = delete;

    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /**
     * @brief Cleanup stale player entries
     */
    void CleanupStaleEntries();

    /**
     * @brief Check if activity is stale
     */
    bool IsStale(PlayerActivity const& activity) const;

    /**
     * @brief Update active status for all tracked players
     */
    void UpdateActiveStatus();

    /**
     * @brief Get bracket for level
     */
    ExpansionTier GetTierForLevel(uint32 level) const;

    // ========================================================================
    // DATA MEMBERS
    // ========================================================================

    // Configuration
    ActivityTrackerConfig _config;

    // Player activity tracking
    using ActivityMap = phmap::parallel_flat_hash_map<
        ObjectGuid,
        PlayerActivity,
        std::hash<ObjectGuid>,
        std::equal_to<>,
        std::allocator<std::pair<ObjectGuid, PlayerActivity>>,
        4,
        std::shared_mutex
    >;
    ActivityMap _playerActivity;

    // Cached counts for fast queries
    mutable std::atomic<uint32> _activePlayerCount{0};
    mutable std::atomic<bool> _countsDirty{true};

    // Timing
    uint32 _updateAccumulator = 0;
    uint32 _cleanupAccumulator = 0;

    // Initialization state
    std::atomic<bool> _initialized{false};
};

} // namespace Playerbot

#define sPlayerActivityTracker Playerbot::PlayerActivityTracker::Instance()
