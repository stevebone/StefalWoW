/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * MOUNT COLLECTION MANAGER
 *
 * Phase 3: Humanization Core (GOD_TIER Task 7)
 *
 * Manages mount collection and farming for bots:
 * - Identifies obtainable mounts for bot's level/faction
 * - Farms specific mounts (raid, reputation, achievement)
 * - Tracks collection progress and priorities
 * - Coordinates with MountManager and other systems
 */

#pragma once

#include "Define.h"
#include "AI/BehaviorManager.h"
#include "ObjectGuid.h"
#include "Position.h"
#include <atomic>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <functional>
#include <memory>

class Player;

namespace Playerbot
{

class BotAI;
class MountManager;

/**
 * @enum MountSource
 * @brief Source/method to obtain a mount
 */
enum class MountSource : uint8
{
    NONE = 0,
    VENDOR,             // Purchasable from vendor
    REPUTATION,         // Requires reputation standing
    RAID_DROP,          // Drops from raid bosses
    DUNGEON_DROP,       // Drops from dungeon bosses
    WORLD_DROP,         // Drops from world bosses/rares
    ACHIEVEMENT,        // Reward from achievement
    QUEST,              // Quest reward
    PROFESSION,         // Crafted via profession
    PVP,                // PvP reward (arena, battleground)
    PROMOTION,          // Promotional (collector's edition, etc.)
    WORLD_EVENT,        // Holiday/world event reward
    TRADING_POST,       // Trading Post reward
    STORE,              // In-game store
    UNKNOWN
};

/**
 * @enum MountRarity
 * @brief Rarity classification for prioritization
 */
enum class MountRarity : uint8
{
    COMMON = 0,         // Easy to obtain
    UNCOMMON,           // Some effort required
    RARE,               // Significant effort
    EPIC,               // Major time investment
    LEGENDARY           // Extremely rare/difficult
};

/**
 * @struct CollectibleMount
 * @brief Information about a mount that can be collected
 */
struct CollectibleMount
{
    uint32 spellId{0};              // Mount spell ID
    uint32 itemId{0};               // Item that teaches mount (if applicable)
    std::string name;
    MountSource source{MountSource::UNKNOWN};
    MountRarity rarity{MountRarity::COMMON};

    // Requirements
    uint8 requiredLevel{0};
    uint32 requiredReputation{0};       // Faction ID if reputation required
    uint32 requiredReputationStanding{0}; // Standing level required
    uint32 requiredAchievement{0};      // Achievement ID if achievement required
    uint32 requiredQuest{0};            // Quest ID if quest required
    uint64 goldCost{0};                 // Gold cost in copper

    // Farm information
    uint32 dropSourceEntry{0};          // Creature/GO entry that drops mount
    uint32 dropSourceInstanceId{0};     // Instance ID for raid/dungeon mounts
    float dropChance{0.0f};             // Approximate drop chance (0-100)
    bool isWeeklyLockout{false};        // Subject to weekly lockout
    bool isLegacy{false};               // Old content (soloable)

    // State
    bool isOwned{false};                // Already owned by bot
    bool isFarmable{false};             // Can bot currently farm this
    uint32 farmAttempts{0};             // Number of farm attempts

    /**
     * @brief Calculate farming priority score
     * @return Priority score (higher = farm sooner)
     */
    float GetPriorityScore() const
    {
        float score = 100.0f;

        // Reduce score based on rarity (easier mounts first)
        score -= static_cast<float>(rarity) * 10.0f;

        // Boost legacy content (soloable)
        if (isLegacy)
            score += 20.0f;

        // Boost higher drop chances
        score += dropChance * 0.5f;

        // Reduce score for weekly lockouts (limited attempts)
        if (isWeeklyLockout)
            score -= 5.0f;

        return score;
    }
};

/**
 * @struct MountFarmSession
 * @brief Tracks an active mount farming session
 */
struct MountFarmSession
{
    uint32 targetMountSpellId{0};
    MountSource source{MountSource::NONE};
    std::chrono::steady_clock::time_point startTime;
    uint32 attemptsThisSession{0};
    bool isActive{false};

    // Navigation state
    Position targetPosition;
    uint32 targetInstanceId{0};
    bool isNavigating{false};
    bool isInInstance{false};

    void Reset()
    {
        targetMountSpellId = 0;
        source = MountSource::NONE;
        attemptsThisSession = 0;
        isActive = false;
        isNavigating = false;
        isInInstance = false;
    }

    uint32 GetElapsedMs() const
    {
        if (!isActive)
            return 0;
        auto now = std::chrono::steady_clock::now();
        return static_cast<uint32>(
            std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count());
    }
};

/**
 * @brief Callback for mount collection events
 */
using MountCollectionCallback = std::function<void(uint32 mountSpellId, bool obtained)>;

/**
 * @class MountCollectionManager
 * @brief Manages mount collection and farming for bots
 *
 * This manager:
 * - Analyzes available mounts based on bot capabilities
 * - Prioritizes mounts by obtainability and rarity
 * - Executes farming strategies for specific mounts
 * - Coordinates with MountManager, InstanceManager, ReputationManager
 *
 * Update interval: 10000ms (10 seconds)
 */
class TC_GAME_API MountCollectionManager : public BehaviorManager
{
public:
    explicit MountCollectionManager(Player* bot, BotAI* ai);
    ~MountCollectionManager() override = default;

    // ========================================================================
    // FAST STATE QUERIES
    // ========================================================================

    /**
     * @brief Check if bot is actively farming mounts
     */
    bool IsFarming() const { return _currentSession.isActive; }

    /**
     * @brief Get current farming target
     */
    uint32 GetCurrentTarget() const { return _currentSession.targetMountSpellId; }

    /**
     * @brief Get total mounts owned
     */
    uint32 GetOwnedMountCount() const { return _ownedMounts.load(std::memory_order_acquire); }

    /**
     * @brief Get total collectible mounts discovered
     */
    uint32 GetCollectibleCount() const { return static_cast<uint32>(_collectibleMounts.size()); }

    // ========================================================================
    // COLLECTION ANALYSIS
    // ========================================================================

    /**
     * @brief Get all obtainable mounts for this bot
     * @param source Filter by source (NONE = all sources)
     * @return Vector of collectible mounts sorted by priority
     */
    std::vector<CollectibleMount> GetObtainableMounts(
        MountSource source = MountSource::NONE) const;

    /**
     * @brief Get mounts by source type
     * @param source Mount source to filter by
     * @return Vector of mounts from specified source
     */
    std::vector<CollectibleMount> GetMountsBySource(MountSource source) const;

    /**
     * @brief Get mounts by rarity
     * @param rarity Rarity level to filter by
     * @return Vector of mounts with specified rarity
     */
    std::vector<CollectibleMount> GetMountsByRarity(MountRarity rarity) const;

    /**
     * @brief Get recommended mounts to farm
     * @param maxCount Maximum number of recommendations
     * @return Vector of recommended mounts sorted by priority
     */
    std::vector<CollectibleMount> GetRecommendedMounts(uint32 maxCount = 10) const;

    /**
     * @brief Check if specific mount is obtainable
     * @param spellId Mount spell ID
     * @return true if bot can obtain this mount
     */
    bool IsMountObtainable(uint32 spellId) const;

    /**
     * @brief Get collection completion percentage
     * @return Completion percentage (0.0 to 1.0)
     */
    float GetCollectionProgress() const;

    // ========================================================================
    // FARMING CONTROL
    // ========================================================================

    /**
     * @brief Start farming a specific mount
     * @param mountSpellId Mount spell ID to farm
     * @return true if farming started
     */
    bool FarmMount(uint32 mountSpellId);

    /**
     * @brief Stop current farming session
     * @param reason Reason for stopping
     */
    void StopFarming(std::string const& reason = "");

    /**
     * @brief Start farming old raid mounts (legacy content)
     * @return true if raid farming started
     */
    bool FarmRaidMounts();

    /**
     * @brief Start farming reputation mounts
     * @param factionId Specific faction (0 = any)
     * @return true if reputation farming started
     */
    bool FarmRepMounts(uint32 factionId = 0);

    /**
     * @brief Start farming achievement mounts
     * @return true if achievement farming started
     */
    bool FarmAchievementMounts();

    /**
     * @brief Start farming dungeon mounts
     * @return true if dungeon farming started
     */
    bool FarmDungeonMounts();

    /**
     * @brief Auto-select and farm best available mount
     * @return true if auto-farm started
     */
    bool AutoFarm();

    /**
     * @brief Get current farming session info
     */
    MountFarmSession const& GetCurrentSession() const { return _currentSession; }

    // ========================================================================
    // RAID MOUNT FARMING
    // ========================================================================

    /**
     * @brief Get list of farmable raid mounts
     * @return Vector of raid drop mounts
     */
    std::vector<CollectibleMount> GetFarmableRaidMounts() const;

    /**
     * @brief Check if raid mount is farmable this week
     * @param spellId Mount spell ID
     * @return true if not on lockout
     */
    bool IsRaidMountFarmable(uint32 spellId) const;

    /**
     * @brief Get raid lockout status for mount
     * @param spellId Mount spell ID
     * @return Lockout reset time (0 if not on lockout)
     */
    time_t GetRaidLockoutReset(uint32 spellId) const;

    // ========================================================================
    // REPUTATION MOUNT FARMING
    // ========================================================================

    /**
     * @brief Get list of farmable reputation mounts
     * @return Vector of reputation mounts
     */
    std::vector<CollectibleMount> GetFarmableRepMounts() const;

    /**
     * @brief Get reputation progress toward mount
     * @param spellId Mount spell ID
     * @return Progress percentage (0.0 to 1.0)
     */
    float GetRepMountProgress(uint32 spellId) const;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /**
     * @brief Set whether to prioritize legacy content
     */
    void SetPrioritizeLegacy(bool enable) { _prioritizeLegacy = enable; }

    /**
     * @brief Set maximum rarity to farm
     */
    void SetMaxRarity(MountRarity rarity) { _maxRarity = rarity; }

    /**
     * @brief Set callback for mount events
     */
    void SetCallback(MountCollectionCallback callback) { _callback = std::move(callback); }

    /**
     * @brief Enable/disable specific source types
     */
    void SetSourceEnabled(MountSource source, bool enabled);

    /**
     * @brief Check if source type is enabled
     */
    bool IsSourceEnabled(MountSource source) const;

    // ========================================================================
    // STATISTICS
    // ========================================================================

    struct CollectionStatistics
    {
        std::atomic<uint32> mountsObtained{0};
        std::atomic<uint32> farmAttempts{0};
        std::atomic<uint32> raidsCompleted{0};
        std::atomic<uint32> dungeonsCompleted{0};
        std::atomic<uint32> reputationsMaxed{0};
        std::atomic<uint64> totalFarmTimeMs{0};
        std::atomic<uint64> goldSpent{0};

        void Reset()
        {
            mountsObtained = 0;
            farmAttempts = 0;
            raidsCompleted = 0;
            dungeonsCompleted = 0;
            reputationsMaxed = 0;
            totalFarmTimeMs = 0;
            goldSpent = 0;
        }
    };

    CollectionStatistics const& GetStatistics() const { return _statistics; }
    static CollectionStatistics const& GetGlobalStatistics() { return _globalStatistics; }

protected:
    // ========================================================================
    // BEHAVIOR MANAGER INTERFACE
    // ========================================================================

    void OnUpdate(uint32 elapsed);
    bool OnInitialize();
    void OnShutdown();

private:
    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /**
     * @brief Analyze all mounts and build collection cache
     */
    void AnalyzeMounts();

    /**
     * @brief Update farming session state
     */
    void UpdateFarmingSession(uint32 elapsed);

    /**
     * @brief Execute raid mount farming step
     */
    void ExecuteRaidFarmStep();

    /**
     * @brief Execute reputation mount farming step
     */
    void ExecuteRepFarmStep();

    /**
     * @brief Execute dungeon mount farming step
     */
    void ExecuteDungeonFarmStep();

    /**
     * @brief Execute vendor mount purchase step
     */
    void ExecuteVendorFarmStep();

    /**
     * @brief Navigate to farming location
     * @return true if navigation in progress
     */
    bool NavigateToFarmLocation();

    /**
     * @brief Check if bot obtained target mount
     */
    void CheckMountObtained();

    /**
     * @brief Select next mount to farm based on priority
     * @return Mount spell ID or 0 if none available
     */
    uint32 SelectNextMountToFarm() const;

    /**
     * @brief Get MountManager reference
     */
    MountManager* GetMountManager() const;

    /**
     * @brief Load mount database from DBC/DB2
     */
    void LoadMountDatabase();

    /**
     * @brief Classify mount source from spell data
     */
    MountSource ClassifyMountSource(uint32 spellId) const;

    /**
     * @brief Calculate mount rarity
     */
    MountRarity CalculateMountRarity(CollectibleMount const& mount) const;

    /**
     * @brief Check if mount meets requirements
     */
    bool MeetsMountRequirements(CollectibleMount const& mount) const;

    /**
     * @brief Notify callback of mount event
     */
    void NotifyCallback(uint32 mountSpellId, bool obtained);

    // ========================================================================
    // DATA
    // ========================================================================

    // Session state
    MountFarmSession _currentSession;

    // Configuration
    bool _prioritizeLegacy{true};
    MountRarity _maxRarity{MountRarity::LEGENDARY};
    std::unordered_set<MountSource> _enabledSources;

    // Collection data
    std::vector<CollectibleMount> _collectibleMounts;
    std::unordered_set<uint32> _ownedMountSpells;
    std::atomic<uint32> _ownedMounts{0};

    // Cache
    std::chrono::steady_clock::time_point _lastAnalysis;

    // Callback
    MountCollectionCallback _callback;

    // Statistics
    CollectionStatistics _statistics;
    static CollectionStatistics _globalStatistics;

    // Static mount database (shared across all bots)
    static std::unordered_map<uint32, CollectibleMount> _mountDatabase;
    static bool _databaseLoaded;

    // Constants
    static constexpr uint32 ANALYSIS_INTERVAL_MS = 300000;  // 5 minutes
    static constexpr uint32 MAX_FARM_DURATION_MS = 3600000; // 1 hour max per session
};

} // namespace Playerbot
