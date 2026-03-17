/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * INSTANCE FARMING MANAGER
 *
 * Phase 3: Humanization Core (Task 14)
 *
 * Manages instance farming for mounts, transmog, and gold:
 * - Tracks farmable instances with loot tables
 * - Prioritizes instances by mount drop chance
 * - Coordinates lockouts and weekly resets
 * - Integrates with MountManager and gold tracking
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

class Player;

namespace Playerbot
{

class BotAI;

/**
 * @enum FarmingContentType
 * @brief Type of content being farmed
 */
enum class FarmingContentType : uint8
{
    NONE = 0,
    MOUNT,              // Farming for mount drops
    TRANSMOG,           // Farming for transmog appearances
    GOLD,               // Farming for gold/vendorable items
    PET,                // Farming for battle pets
    TOY,                // Farming for toys
    ACHIEVEMENT,        // Farming for instance achievements
    REPUTATION,         // Farming for reputation
    MIXED               // Mixed farming goals
};

/**
 * @enum InstanceDifficulty
 * @brief Instance difficulty settings
 * @note WoW 12.0: Changed from uint8 to int16 to match core Difficulty type
 */
enum class InstanceDifficulty : int16
{
    NORMAL = 0,
    HEROIC,
    MYTHIC,
    LEGACY_10N,
    LEGACY_10H,
    LEGACY_25N,
    LEGACY_25H,
    LEGACY_40
};

/**
 * @struct FarmableInstance
 * @brief Information about an instance that can be farmed
 */
struct FarmableInstance
{
    uint32 mapId{0};
    std::string name;
    uint32 minLevel{0};             // Minimum level to enter
    uint32 maxPlayers{5};           // 5-man, 10-man, 25-man, 40-man
    bool isRaid{false};
    InstanceDifficulty difficulty{InstanceDifficulty::NORMAL};
    std::vector<uint32> mountDrops;         // Mount spell IDs that can drop
    std::vector<uint32> transmogItems;      // Notable transmog item IDs
    std::vector<uint32> petDrops;           // Pet item/spell IDs
    uint32 estimatedGoldValue{0};           // Estimated gold from full clear
    uint32 estimatedClearTimeMs{0};         // Estimated time to clear
    bool hasWeeklyLockout{false};           // True for raids
    Position entrancePos;                   // Instance entrance position

    float GetMountChance() const
    {
        // Simplified - most mounts are 1-3% drop rate
        return mountDrops.empty() ? 0.0f : 0.01f;
    }

    bool HasMounts() const { return !mountDrops.empty(); }
    bool HasTransmog() const { return !transmogItems.empty(); }
    bool HasPets() const { return !petDrops.empty(); }
};

/**
 * @struct InstanceLockout
 * @brief Tracks lockout status for an instance
 */
struct InstanceLockout
{
    uint32 mapId{0};
    InstanceDifficulty difficulty{InstanceDifficulty::NORMAL};
    std::chrono::system_clock::time_point resetTime;
    bool isExtended{false};
    std::vector<uint32> killedBosses;

    bool IsLocked() const
    {
        return std::chrono::system_clock::now() < resetTime;
    }

    uint32 GetTimeUntilResetMs() const
    {
        auto now = std::chrono::system_clock::now();
        if (now >= resetTime)
            return 0;
        return static_cast<uint32>(
            std::chrono::duration_cast<std::chrono::milliseconds>(resetTime - now).count());
    }
};

/**
 * @struct FarmingGoal
 * @brief A specific farming goal
 */
struct FarmingGoal
{
    FarmingContentType type{FarmingContentType::NONE};
    uint32 targetItemId{0};         // Specific mount/pet/item to farm
    std::string targetName;
    std::vector<uint32> instancesWithDrop;  // Instances that can drop this
    uint32 attemptsCount{0};        // Number of attempts made
    bool isAcquired{false};

    FarmingGoal() = default;
    FarmingGoal(FarmingContentType t, uint32 itemId, std::string const& name)
        : type(t), targetItemId(itemId), targetName(name) {}
};

/**
 * @struct FarmingSession
 * @brief Tracks an instance farming session
 */
struct FarmingSession
{
    FarmableInstance currentInstance;
    FarmingContentType primaryGoal{FarmingContentType::MIXED};
    std::vector<FarmingGoal> activeGoals;
    std::chrono::steady_clock::time_point startTime;
    uint32 instancesCleared{0};
    uint32 bossesKilled{0};
    uint64 goldEarned{0};
    uint32 itemsLooted{0};
    uint32 mountsAcquired{0};
    uint32 transmogsAcquired{0};
    bool isActive{false};

    void Reset()
    {
        currentInstance = FarmableInstance();
        primaryGoal = FarmingContentType::MIXED;
        activeGoals.clear();
        instancesCleared = 0;
        bossesKilled = 0;
        goldEarned = 0;
        itemsLooted = 0;
        mountsAcquired = 0;
        transmogsAcquired = 0;
        isActive = false;
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
 * @brief Callback for farming events
 */
using FarmingCallback = std::function<void(FarmingContentType, uint32 itemId, bool acquired)>;

/**
 * @class InstanceFarmingManager
 * @brief Manages instance farming for mounts, transmog, and gold
 *
 * This manager:
 * - Maintains database of farmable instances
 * - Tracks lockouts and weekly resets
 * - Prioritizes instances by drop rates and goals
 * - Coordinates with MountManager for mount tracking
 * - Integrates with gold farming for efficiency
 *
 * Update interval: 10000ms (10 seconds)
 */
class TC_GAME_API InstanceFarmingManager : public BehaviorManager
{
public:
    explicit InstanceFarmingManager(Player* bot, BotAI* ai);
    ~InstanceFarmingManager() override = default;

    // ========================================================================
    // FAST STATE QUERIES
    // ========================================================================

    /**
     * @brief Check if bot is in farming mode
     */
    bool IsFarming() const { return _currentSession.isActive; }

    /**
     * @brief Check if bot is currently in an instance
     */
    bool IsInInstance() const;

    /**
     * @brief Get current farming goal type
     */
    FarmingContentType GetCurrentGoalType() const { return _currentSession.primaryGoal; }

    /**
     * @brief Get number of instances cleared this session
     */
    uint32 GetInstancesCleared() const { return _currentSession.instancesCleared; }

    // ========================================================================
    // INSTANCE DATABASE
    // ========================================================================

    /**
     * @brief Get all farmable instances
     * @param type Filter by content type (NONE = all)
     * @return Vector of farmable instances
     */
    std::vector<FarmableInstance> GetFarmableInstances(FarmingContentType type = FarmingContentType::NONE) const;

    /**
     * @brief Get instances with specific mount drop
     * @param mountSpellId Mount spell ID to find
     * @return Vector of instances that can drop this mount
     */
    std::vector<FarmableInstance> GetInstancesWithMount(uint32 mountSpellId) const;

    /**
     * @brief Get recommended instances for bot
     * @param maxCount Maximum recommendations
     * @return Vector of recommended instances
     */
    std::vector<FarmableInstance> GetRecommendedInstances(uint32 maxCount = 5) const;

    /**
     * @brief Get instance info by map ID
     * @param mapId Map ID
     * @return Instance info, or empty if not farmable
     */
    FarmableInstance GetInstanceInfo(uint32 mapId) const;

    // ========================================================================
    // LOCKOUT TRACKING
    // ========================================================================

    /**
     * @brief Check if instance is locked
     * @param mapId Map ID
     * @param difficulty Difficulty
     * @return true if locked out
     */
    bool IsInstanceLocked(uint32 mapId, InstanceDifficulty difficulty) const;

    /**
     * @brief Get lockout info for instance
     * @param mapId Map ID
     * @param difficulty Difficulty
     * @return Lockout info
     */
    InstanceLockout GetLockout(uint32 mapId, InstanceDifficulty difficulty) const;

    /**
     * @brief Get all current lockouts
     * @return Vector of active lockouts
     */
    std::vector<InstanceLockout> GetAllLockouts() const;

    /**
     * @brief Get instances available to run (not locked)
     * @return Vector of available instances
     */
    std::vector<FarmableInstance> GetAvailableInstances() const;

    /**
     * @brief Refresh lockout data from player
     */
    void RefreshLockouts();

    // ========================================================================
    // SESSION CONTROL
    // ========================================================================

    /**
     * @brief Start a farming session
     * @param goalType Primary goal type
     * @param specificGoals Specific items to farm (optional)
     * @return true if session started
     */
    bool StartSession(
        FarmingContentType goalType = FarmingContentType::MIXED,
        std::vector<FarmingGoal> const& specificGoals = {});

    /**
     * @brief Stop the current session
     * @param reason Why stopping
     */
    void StopSession(std::string const& reason = "");

    /**
     * @brief Queue an instance to run
     * @param instance Instance to queue
     * @return true if queued
     */
    bool QueueInstance(FarmableInstance const& instance);

    /**
     * @brief Get current session info
     */
    FarmingSession const& GetCurrentSession() const { return _currentSession; }

    // ========================================================================
    // FARMING GOALS
    // ========================================================================

    /**
     * @brief Add a specific farming goal
     * @param goal Goal to add
     */
    void AddGoal(FarmingGoal const& goal);

    /**
     * @brief Remove a farming goal
     * @param itemId Item ID to remove
     */
    void RemoveGoal(uint32 itemId);

    /**
     * @brief Mark goal as acquired
     * @param itemId Item ID acquired
     */
    void MarkGoalAcquired(uint32 itemId);

    /**
     * @brief Get all active goals
     */
    std::vector<FarmingGoal> const& GetActiveGoals() const { return _currentSession.activeGoals; }

    /**
     * @brief Get missing mounts that can be farmed
     * @return Vector of mount goals
     */
    std::vector<FarmingGoal> GetMissingMounts() const;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /**
     * @brief Set minimum level for instances
     */
    void SetMinInstanceLevel(uint32 minLevel) { _minInstanceLevel = minLevel; }

    /**
     * @brief Set whether to prioritize mounts
     */
    void SetPrioritizeMounts(bool prioritize) { _prioritizeMounts = prioritize; }

    /**
     * @brief Set maximum instances per session
     */
    void SetMaxInstancesPerSession(uint32 maxInstances) { _maxInstancesPerSession = maxInstances; }

    /**
     * @brief Set callback for farming events
     */
    void SetCallback(FarmingCallback callback) { _callback = std::move(callback); }

    // ========================================================================
    // STATISTICS
    // ========================================================================

    struct FarmingStatistics
    {
        std::atomic<uint32> totalInstancesCleared{0};
        std::atomic<uint32> totalBossesKilled{0};
        std::atomic<uint32> totalMountsAcquired{0};
        std::atomic<uint32> totalTransmogsAcquired{0};
        std::atomic<uint32> totalPetsAcquired{0};
        std::atomic<uint64> totalGoldEarned{0};
        std::atomic<uint64> totalFarmingTimeMs{0};

        void Reset()
        {
            totalInstancesCleared = 0;
            totalBossesKilled = 0;
            totalMountsAcquired = 0;
            totalTransmogsAcquired = 0;
            totalPetsAcquired = 0;
            totalGoldEarned = 0;
            totalFarmingTimeMs = 0;
        }
    };

    FarmingStatistics const& GetStatistics() const { return _statistics; }

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
     * @brief Initialize farmable instance database
     */
    void InitializeInstanceDatabase();

    /**
     * @brief Add a farmable instance to database
     */
    void AddFarmableInstance(FarmableInstance const& instance);

    /**
     * @brief Update current instance progress
     */
    void UpdateInstanceProgress();

    /**
     * @brief Select next instance to run
     */
    FarmableInstance SelectNextInstance() const;

    /**
     * @brief Check if bot can solo instance
     */
    bool CanSoloInstance(FarmableInstance const& instance) const;

    /**
     * @brief Notify callback
     */
    void NotifyCallback(FarmingContentType type, uint32 itemId, bool acquired);

    // ========================================================================
    // DATA
    // ========================================================================

    // Session state
    FarmingSession _currentSession;

    // Instance database
    std::unordered_map<uint32, FarmableInstance> _instanceDatabase;

    // Lockout tracking
    std::unordered_map<uint64, InstanceLockout> _lockouts;  // Key = mapId << 32 | difficulty
    std::chrono::steady_clock::time_point _lastLockoutRefresh;

    // Configuration
    uint32 _minInstanceLevel{1};
    bool _prioritizeMounts{true};
    uint32 _maxInstancesPerSession{10};

    // Callback
    FarmingCallback _callback;

    // Statistics
    FarmingStatistics _statistics;

    // Constants
    static constexpr uint32 LOCKOUT_REFRESH_INTERVAL_MS = 60000;  // 1 minute
};

} // namespace Playerbot
