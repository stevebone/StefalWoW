/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * @file BotProtectionRegistry.h
 * @brief Central registry for bot protection status management
 *
 * The BotProtectionRegistry is the authoritative source for bot protection status.
 * It tracks WHY bots are protected (guild, friends, groups, etc.) and calculates
 * protection scores used by the retirement system to decide which bots to delete.
 *
 * Key Responsibilities:
 * 1. Aggregate protection info from multiple providers (guild, social, group, etc.)
 * 2. Cache protection status for fast queries
 * 3. Calculate protection scores for retirement priority
 * 4. Provide batch queries for bracket-level operations
 * 5. Handle protection change events from game systems
 *
 * Thread Safety:
 * - All public methods are thread-safe
 * - Uses parallel hashmap for lock-free concurrent reads
 * - Write operations use fine-grained locking per-bot
 *
 * Performance:
 * - O(1) protection lookup
 * - O(n) bracket queries (where n = bots in bracket)
 * - Periodic batch updates to database
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "ProtectionReason.h"
#include "ProtectionStatus.h"
#include "IProtectionProvider.h"
#include "../BotLifecycleState.h"
#include "Character/BotLevelDistribution.h"
#include <parallel_hashmap/phmap.h>
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>

namespace Playerbot
{

// Forward declarations
class BotSession;

/**
 * @brief Configuration for protection behavior
 */
struct ProtectionConfig
{
    // Which protection reasons are enabled
    bool enableGuildProtection = true;
    bool enableFriendProtection = true;
    bool enableGroupProtection = true;
    bool enableInteractionProtection = true;
    bool enableMailProtection = true;
    bool enableAuctionProtection = true;

    // Time windows
    uint32 interactionWindowHours = 24;  // How long RecentInteract lasts

    // Protection weights (override defaults)
    float guildWeight = 100.0f;
    float friendWeight = 80.0f;
    float groupWeight = 90.0f;
    float interactionWeight = 40.0f;
    float mailWeight = 50.0f;
    float auctionWeight = 30.0f;
    float manualWeight = 1000.0f;

    // Score bonuses
    float friendCountBonus = 10.0f;       // Per friend
    float interactionCountBonus = 1.0f;   // Per interaction
    float groupTimeBonus = 5.0f;          // Per hour since group

    // Database sync
    uint32 dbSyncIntervalMs = 60000;      // How often to sync to database
    bool enableDbSync = true;
};

/**
 * @brief Statistics about protected bots
 */
struct ProtectionStatistics
{
    uint32 totalTrackedBots = 0;
    uint32 protectedBots = 0;
    uint32 unprotectedBots = 0;

    // By protection reason
    uint32 botsInGuild = 0;
    uint32 botsOnFriendList = 0;
    uint32 botsInPlayerGroup = 0;
    uint32 botsWithRecentInteraction = 0;
    uint32 botsWithMail = 0;
    uint32 botsWithAuctions = 0;
    uint32 botsManuallyProtected = 0;

    // By bracket (4 expansion tiers)
    std::array<uint32, 4> protectedPerBracket = {0, 0, 0, 0};
    std::array<uint32, 4> unprotectedPerBracket = {0, 0, 0, 0};

    // Score distribution
    float minProtectionScore = 0.0f;
    float maxProtectionScore = 0.0f;
    float avgProtectionScore = 0.0f;

    // Timing
    std::chrono::system_clock::time_point lastUpdate;
};

/**
 * @brief Central registry for bot protection status
 *
 * Singleton class managing all bot protection information.
 * Aggregates data from guild, social, group, mail, and auction systems.
 */
class TC_GAME_API BotProtectionRegistry
{
public:
    /**
     * @brief Get singleton instance
     */
    static BotProtectionRegistry* Instance();

    // ========================================================================
    // INITIALIZATION
    // ========================================================================

    /**
     * @brief Initialize the registry
     * @return true if initialization successful
     */
    bool Initialize();

    /**
     * @brief Shutdown and cleanup
     */
    void Shutdown();

    /**
     * @brief Periodic update (call from world thread)
     * @param diff Time since last update in milliseconds
     */
    void Update(uint32 diff);

    /**
     * @brief Load configuration from playerbots.conf
     */
    void LoadConfig();

    // ========================================================================
    // PROTECTION QUERIES
    // ========================================================================

    /**
     * @brief Check if a bot is protected from retirement
     * @param botGuid Bot to check
     * @return true if bot has any active protection
     */
    bool IsProtected(ObjectGuid botGuid) const;

    /**
     * @brief Get full protection status for a bot
     * @param botGuid Bot to query
     * @return Protection status (or empty status if not tracked)
     */
    ProtectionStatus GetStatus(ObjectGuid botGuid) const;

    /**
     * @brief Get protection score for a bot
     * @param botGuid Bot to query
     * @return Protection score (higher = more protected)
     */
    float GetProtectionScore(ObjectGuid botGuid) const;

    /**
     * @brief Check if bot has specific protection reason
     * @param botGuid Bot to check
     * @param reason Protection reason to check
     * @return true if the specific reason is active
     */
    bool HasProtectionReason(ObjectGuid botGuid, ProtectionReason reason) const;

    /**
     * @brief Get interaction count for a bot
     * @param botGuid Bot to query
     * @return Number of recorded player interactions
     */
    uint32 GetInteractionCount(ObjectGuid botGuid) const;

    // ========================================================================
    // BULK QUERIES (for bracket operations)
    // ========================================================================

    /**
     * @brief Get unprotected bots in a specific bracket
     * @param bracket Level bracket to query
     * @return List of unprotected bot GUIDs
     */
    std::vector<ObjectGuid> GetUnprotectedBotsInBracket(LevelBracket const* bracket) const;

    /**
     * @brief Get protected bots in a specific bracket
     * @param bracket Level bracket to query
     * @return List of protected bot GUIDs
     */
    std::vector<ObjectGuid> GetProtectedBotsInBracket(LevelBracket const* bracket) const;

    /**
     * @brief Get count of protected bots in a bracket
     * @param bracket Level bracket to query
     * @return Number of protected bots
     */
    uint32 GetProtectedCountInBracket(LevelBracket const* bracket) const;

    /**
     * @brief Get count of unprotected bots in a bracket
     * @param bracket Level bracket to query
     * @return Number of unprotected bots
     */
    uint32 GetUnprotectedCountInBracket(LevelBracket const* bracket) const;

    /**
     * @brief Get bots sorted by protection score (ascending - lowest first)
     * @param bracket Level bracket to query
     * @param maxCount Maximum bots to return
     * @return List of bot GUIDs sorted by protection score
     */
    std::vector<ObjectGuid> GetRetirementCandidates(LevelBracket const* bracket, uint32 maxCount) const;

    // ========================================================================
    // PROTECTION EVENTS (called by game systems)
    // ========================================================================

    /**
     * @brief Called when a bot joins a guild
     * @param botGuid The bot
     * @param guildGuid The guild
     */
    void OnBotJoinedGuild(ObjectGuid botGuid, ObjectGuid guildGuid);

    /**
     * @brief Called when a bot leaves a guild
     * @param botGuid The bot
     */
    void OnBotLeftGuild(ObjectGuid botGuid);

    /**
     * @brief Called when a player adds a bot to their friend list
     * @param playerGuid The player
     * @param botGuid The bot
     */
    void OnPlayerAddedFriend(ObjectGuid playerGuid, ObjectGuid botGuid);

    /**
     * @brief Called when a player removes a bot from their friend list
     * @param playerGuid The player
     * @param botGuid The bot
     */
    void OnPlayerRemovedFriend(ObjectGuid playerGuid, ObjectGuid botGuid);

    /**
     * @brief Called when a player interacts with a bot
     * @param playerGuid The player
     * @param botGuid The bot
     *
     * Interactions include: trade, whisper, invite, assist, duel request
     */
    void OnPlayerInteraction(ObjectGuid playerGuid, ObjectGuid botGuid);

    /**
     * @brief Called when a bot joins a group with a real player
     * @param botGuid The bot
     * @param playerGuid A real player in the group
     */
    void OnBotGroupedWithPlayer(ObjectGuid botGuid, ObjectGuid playerGuid);

    /**
     * @brief Called when a bot leaves a player group
     * @param botGuid The bot
     */
    void OnBotLeftGroup(ObjectGuid botGuid);

    /**
     * @brief Called when a bot receives mail
     * @param botGuid The bot
     */
    void OnBotMailReceived(ObjectGuid botGuid);

    /**
     * @brief Called when all mail is cleared for a bot
     * @param botGuid The bot
     */
    void OnBotMailCleared(ObjectGuid botGuid);

    /**
     * @brief Called when a bot creates an auction
     * @param botGuid The bot
     */
    void OnBotAuctionCreated(ObjectGuid botGuid);

    /**
     * @brief Called when all auctions are cleared for a bot
     * @param botGuid The bot
     */
    void OnBotAuctionCleared(ObjectGuid botGuid);

    /**
     * @brief Called when a bot is created/spawned
     * @param botGuid The new bot
     * @param level Initial level
     */
    void OnBotCreated(ObjectGuid botGuid, uint32 level);

    /**
     * @brief Called when a bot is being deleted
     * @param botGuid The bot being deleted
     */
    void OnBotDeleted(ObjectGuid botGuid);

    /**
     * @brief Called when a bot levels up
     * @param botGuid The bot
     * @param oldLevel Previous level
     * @param newLevel New level
     */
    void OnBotLevelUp(ObjectGuid botGuid, uint32 oldLevel, uint32 newLevel);

    // ========================================================================
    // ADMINISTRATIVE
    // ========================================================================

    /**
     * @brief Set manual (admin) protection for a bot
     * @param botGuid Bot to protect
     * @param protect Whether to enable (true) or disable (false) protection
     */
    void SetManualProtection(ObjectGuid botGuid, bool protect);

    /**
     * @brief Force recalculation of all protection statuses
     *
     * Expensive operation - should only be called on reload or admin command.
     */
    void RecalculateAllProtection();

    /**
     * @brief Refresh protection from game systems
     *
     * Queries guild, social, and group managers for current state.
     */
    void RefreshFromGameSystems();

    /**
     * @brief Get protection statistics
     * @return Current protection statistics
     */
    ProtectionStatistics GetStatistics() const;

    /**
     * @brief Print protection report to log
     */
    void PrintReport() const;

    // ========================================================================
    // PROVIDER REGISTRATION
    // ========================================================================

    /**
     * @brief Register a protection provider
     * @param provider The provider to register
     */
    void RegisterProvider(std::shared_ptr<IProtectionProvider> provider);

    /**
     * @brief Unregister all providers
     */
    void ClearProviders();

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /**
     * @brief Get current configuration
     */
    ProtectionConfig const& GetConfig() const { return _config; }

    /**
     * @brief Set configuration
     */
    void SetConfig(ProtectionConfig const& config);

private:
    BotProtectionRegistry() = default;
    ~BotProtectionRegistry() = default;
    BotProtectionRegistry(BotProtectionRegistry const&) = delete;
    BotProtectionRegistry& operator=(BotProtectionRegistry const&) = delete;

    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /**
     * @brief Get or create protection status for a bot
     */
    ProtectionStatus& GetOrCreateStatus(ObjectGuid botGuid);

    /**
     * @brief Update protection status for a specific bot
     */
    void UpdateProtectionStatus(ObjectGuid botGuid);

    /**
     * @brief Recalculate protection score for a bot
     */
    void RecalculateScore(ObjectGuid botGuid);

    /**
     * @brief Check and update time-based protection flags
     */
    void UpdateTimeBasedFlags();

    /**
     * @brief Load protection data from database
     */
    void LoadFromDatabase();

    /**
     * @brief Save protection data to database
     */
    void SaveToDatabase();

    /**
     * @brief Save a single bot's protection status to database
     */
    void SaveBotToDatabase(ObjectGuid botGuid, ProtectionStatus const& status);

    /**
     * @brief Mark a bot's status as dirty (needs DB save)
     */
    void MarkDirty(ObjectGuid botGuid);

    /**
     * @brief Get bracket for a level
     */
    LevelBracket const* GetBracketForLevel(uint32 level) const;

    /**
     * @brief Get bracket index (0-3) for expansion tier
     */
    uint32 GetBracketIndex(ExpansionTier tier) const;

    // ========================================================================
    // DATA MEMBERS
    // ========================================================================

    // Configuration
    ProtectionConfig _config;

    // Main protection storage - parallel hashmap for thread-safe concurrent access
    using ProtectionMap = phmap::parallel_flat_hash_map<
        ObjectGuid,
        ProtectionStatus,
        std::hash<ObjectGuid>,
        std::equal_to<>,
        std::allocator<std::pair<ObjectGuid, ProtectionStatus>>,
        4,  // 4 submaps for concurrency
        std::shared_mutex
    >;
    ProtectionMap _protectionMap;

    // Bot level tracking for bracket queries
    using LevelMap = phmap::parallel_flat_hash_map<
        ObjectGuid,
        uint32,  // level
        std::hash<ObjectGuid>,
        std::equal_to<>,
        std::allocator<std::pair<ObjectGuid, uint32>>,
        4,
        std::shared_mutex
    >;
    LevelMap _botLevels;

    // Friend references: player -> set of bots they have as friends
    using FriendMap = phmap::parallel_flat_hash_map<
        ObjectGuid,
        std::set<ObjectGuid>,
        std::hash<ObjectGuid>,
        std::equal_to<>,
        std::allocator<std::pair<ObjectGuid, std::set<ObjectGuid>>>,
        4,
        std::shared_mutex
    >;
    FriendMap _friendReferences;

    // Registered protection providers
    std::vector<std::shared_ptr<IProtectionProvider>> _providers;
    mutable std::shared_mutex _providerMutex;

    // Dirty tracking for database sync
    std::set<ObjectGuid> _dirtyBots;
    mutable std::mutex _dirtyMutex;

    // Timing
    uint32 _dbSyncAccumulator = 0;
    uint32 _timeBasedUpdateAccumulator = 0;
    static constexpr uint32 TIME_BASED_UPDATE_INTERVAL_MS = 60000;  // 1 minute

    // Statistics cache
    mutable ProtectionStatistics _cachedStats;
    mutable std::atomic<bool> _statsDirty{true};

    // Initialization state
    std::atomic<bool> _initialized{false};
};

} // namespace Playerbot

#define sBotProtectionRegistry Playerbot::BotProtectionRegistry::Instance()
