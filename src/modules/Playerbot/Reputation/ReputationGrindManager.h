/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * REPUTATION GRIND MANAGER
 *
 * Phase 3: Humanization Core (Task 9)
 *
 * Manages reputation grinding for bots:
 * - Tracks reputation progress with factions
 * - Suggests factions to grind based on rewards
 * - Coordinates quests and mob kills for reputation
 * - Integrates with humanization system
 */

#pragma once

#include "Define.h"
#include "AI/BehaviorManager.h"
#include "ObjectGuid.h"
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
 * @enum ReputationStanding
 * @brief Reputation standing levels
 */
enum class ReputationStanding : uint8
{
    HATED = 0,
    HOSTILE,
    UNFRIENDLY,
    NEUTRAL,
    FRIENDLY,
    HONORED,
    REVERED,
    EXALTED,

    MAX_STANDING
};

/**
 * @enum ReputationGrindMethod
 * @brief Methods to gain reputation
 */
enum class ReputationGrindMethod : uint8
{
    NONE = 0,
    QUESTS,             // Complete quests
    MOB_KILLS,          // Kill mobs
    DUNGEON_RUNS,       // Run dungeons
    ITEM_TURNINS,       // Turn in items
    WORLD_QUESTS,       // World quests (if applicable)
    CONTRACTS,          // Reputation contracts
    EVENTS              // World events
};

/**
 * @struct FactionInfo
 * @brief Information about a faction
 */
struct FactionInfo
{
    uint32 factionId{0};
    std::string name;
    ReputationStanding standing{ReputationStanding::NEUTRAL};
    int32 currentRep{0};                // Current reputation value
    int32 standingMin{0};               // Minimum rep for current standing
    int32 standingMax{0};               // Maximum rep for current standing
    bool isAtWar{false};
    bool canToggleAtWar{false};
    uint32 rewards{0};                  // Number of unlockable rewards

    float GetStandingProgress() const
    {
        if (standingMax <= standingMin)
            return 1.0f;
        return static_cast<float>(currentRep - standingMin) /
               static_cast<float>(standingMax - standingMin);
    }

    int32 GetRepToNextStanding() const
    {
        return standingMax - currentRep;
    }
};

/**
 * @struct ReputationGoal
 * @brief A goal for reputation grinding
 */
struct ReputationGoal
{
    uint32 factionId{0};
    std::string factionName;
    ReputationStanding targetStanding{ReputationStanding::EXALTED};
    ReputationStanding currentStanding{ReputationStanding::NEUTRAL};
    int32 currentRep{0};
    int32 targetRep{0};
    ReputationGrindMethod preferredMethod{ReputationGrindMethod::QUESTS};
    uint32 estimatedRepPerHour{0};
    bool isActive{false};

    float GetProgress() const
    {
        if (targetRep <= 0)
            return 0.0f;
        return std::min(1.0f, static_cast<float>(currentRep) / static_cast<float>(targetRep));
    }

    bool IsComplete() const
    {
        return currentStanding >= targetStanding;
    }
};

/**
 * @struct ReputationGrindSession
 * @brief Tracks a reputation grinding session
 */
struct ReputationGrindSession
{
    ReputationGoal activeGoal;
    std::chrono::steady_clock::time_point startTime;
    int32 startRep{0};
    int32 repGained{0};
    uint32 questsCompleted{0};
    uint32 mobsKilled{0};
    uint32 itemsTurnedIn{0};
    bool isActive{false};

    void Reset()
    {
        activeGoal = ReputationGoal();
        repGained = 0;
        questsCompleted = 0;
        mobsKilled = 0;
        itemsTurnedIn = 0;
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

    uint32 GetRepPerHour() const
    {
        uint32 elapsedMs = GetElapsedMs();
        if (elapsedMs < 60000)  // Less than 1 minute
            return 0;
        return static_cast<uint32>(repGained * 3600000.0f / elapsedMs);
    }
};

/**
 * @brief Callback for reputation events
 */
using ReputationCallback = std::function<void(uint32 factionId, ReputationStanding newStanding)>;

/**
 * @class ReputationGrindManager
 * @brief Manages reputation grinding for bots
 *
 * This manager:
 * - Tracks reputation with all factions
 * - Suggests factions to grind based on rewards and progress
 * - Coordinates activities to gain reputation
 * - Optimizes grinding efficiency
 *
 * Update interval: 5000ms (5 seconds)
 */
class TC_GAME_API ReputationGrindManager : public BehaviorManager
{
public:
    explicit ReputationGrindManager(Player* bot, BotAI* ai);
    ~ReputationGrindManager() override = default;

    // ========================================================================
    // FAST STATE QUERIES
    // ========================================================================

    /**
     * @brief Check if bot is actively grinding reputation
     */
    bool IsGrinding() const { return _currentSession.isActive; }

    /**
     * @brief Get active faction ID
     */
    uint32 GetActiveFactionId() const { return _currentSession.activeGoal.factionId; }

    /**
     * @brief Get standing with faction
     * @param factionId Faction ID
     */
    ReputationStanding GetStanding(uint32 factionId) const;

    /**
     * @brief Get current reputation value with faction
     * @param factionId Faction ID
     */
    int32 GetReputation(uint32 factionId) const;

    /**
     * @brief Check if faction is at exalted
     * @param factionId Faction ID
     */
    bool IsExalted(uint32 factionId) const;

    // ========================================================================
    // FACTION ANALYSIS
    // ========================================================================

    /**
     * @brief Get all tracked factions
     * @return Vector of faction info
     */
    std::vector<FactionInfo> GetAllFactions() const;

    /**
     * @brief Get factions with rewards available at next standing
     * @return Vector of faction info with pending rewards
     */
    std::vector<FactionInfo> GetFactionsWithRewards() const;

    /**
     * @brief Get factions closest to next standing
     * @param maxCount Maximum number to return
     * @return Vector of faction info sorted by progress
     */
    std::vector<FactionInfo> GetNearestStandingFactions(uint32 maxCount = 5) const;

    /**
     * @brief Get suggested factions to grind
     * @param maxCount Maximum number of suggestions
     * @return Vector of reputation goals
     */
    std::vector<ReputationGoal> GetSuggestedFactions(uint32 maxCount = 5) const;

    /**
     * @brief Get faction info
     * @param factionId Faction ID
     * @return Faction info, or empty if not found
     */
    FactionInfo GetFactionInfo(uint32 factionId) const;

    // ========================================================================
    // SESSION CONTROL
    // ========================================================================

    /**
     * @brief Start a reputation grinding session
     * @param factionId Faction to grind
     * @param targetStanding Target standing to reach
     * @return true if session started
     */
    bool StartSession(uint32 factionId, ReputationStanding targetStanding = ReputationStanding::EXALTED);

    /**
     * @brief Stop the current session
     * @param reason Why stopping
     */
    void StopSession(std::string const& reason = "");

    /**
     * @brief Change target faction
     * @param factionId New faction to grind
     * @return true if changed
     */
    bool ChangeTargetFaction(uint32 factionId);

    /**
     * @brief Get current session info
     */
    ReputationGrindSession const& GetCurrentSession() const { return _currentSession; }

    // ========================================================================
    // GRIND METHODS
    // ========================================================================

    /**
     * @brief Get best grinding method for faction
     * @param factionId Faction ID
     * @return Best grinding method
     */
    ReputationGrindMethod GetBestGrindMethod(uint32 factionId) const;

    /**
     * @brief Get quests that give reputation for faction
     * @param factionId Faction ID
     * @return Vector of quest IDs
     */
    std::vector<uint32> GetReputationQuests(uint32 factionId) const;

    /**
     * @brief Get mobs that give reputation for faction
     * @param factionId Faction ID
     * @return Vector of creature entries
     */
    std::vector<uint32> GetReputationMobs(uint32 factionId) const;

    /**
     * @brief Get items that can be turned in for reputation
     * @param factionId Faction ID
     * @return Vector of item IDs
     */
    std::vector<uint32> GetTurnInItems(uint32 factionId) const;

    // ========================================================================
    // REPUTATION TRACKING
    // ========================================================================

    /**
     * @brief Record reputation gain
     * @param factionId Faction ID
     * @param amount Amount gained
     * @param source Source of gain
     */
    void RecordRepGain(uint32 factionId, int32 amount, ReputationGrindMethod source);

    /**
     * @brief Get total reputation gained in session
     */
    int32 GetSessionRepGained() const { return _currentSession.repGained; }

    /**
     * @brief Get rep per hour in session
     */
    uint32 GetSessionRepPerHour() const { return _currentSession.GetRepPerHour(); }

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /**
     * @brief Set target standing for all grinds
     */
    void SetDefaultTarget(ReputationStanding standing) { _defaultTarget = standing; }

    /**
     * @brief Set preferred grinding method
     */
    void SetPreferredMethod(ReputationGrindMethod method) { _preferredMethod = method; }

    /**
     * @brief Set callback for reputation events
     */
    void SetCallback(ReputationCallback callback) { _callback = std::move(callback); }

    /**
     * @brief Add faction to priority list
     */
    void AddPriorityFaction(uint32 factionId);

    /**
     * @brief Remove faction from priority list
     */
    void RemovePriorityFaction(uint32 factionId);

    // ========================================================================
    // STATISTICS
    // ========================================================================

    struct ReputationStatistics
    {
        std::atomic<int64> totalRepGained{0};
        std::atomic<uint32> factionsExalted{0};
        std::atomic<uint32> questsCompleted{0};
        std::atomic<uint32> mobsKilled{0};
        std::atomic<uint32> itemsTurnedIn{0};
        std::atomic<uint64> totalGrindTimeMs{0};

        void Reset()
        {
            totalRepGained = 0;
            factionsExalted = 0;
            questsCompleted = 0;
            mobsKilled = 0;
            itemsTurnedIn = 0;
            totalGrindTimeMs = 0;
        }
    };

    ReputationStatistics const& GetStatistics() const { return _statistics; }

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
     * @brief Refresh faction data from player
     */
    void RefreshFactionData();

    /**
     * @brief Update current session progress
     */
    void UpdateSessionProgress();

    /**
     * @brief Check for standing changes
     */
    void CheckStandingChanges();

    /**
     * @brief Convert reputation value to standing
     */
    ReputationStanding GetStandingFromRep(int32 rep) const;

    /**
     * @brief Get reputation thresholds for standing
     */
    void GetStandingThresholds(ReputationStanding standing, int32& min, int32& max) const;

    /**
     * @brief Notify callback of standing change
     */
    void NotifyCallback(uint32 factionId, ReputationStanding newStanding);

    // ========================================================================
    // DATA
    // ========================================================================

    // Session state
    ReputationGrindSession _currentSession;

    // Faction cache
    std::unordered_map<uint32, FactionInfo> _factionCache;
    std::chrono::steady_clock::time_point _lastRefresh;

    // Configuration
    ReputationStanding _defaultTarget{ReputationStanding::EXALTED};
    ReputationGrindMethod _preferredMethod{ReputationGrindMethod::QUESTS};
    std::unordered_set<uint32> _priorityFactions;

    // Callback
    ReputationCallback _callback;

    // Statistics
    ReputationStatistics _statistics;

    // Constants
    static constexpr uint32 REFRESH_INTERVAL_MS = 30000;    // 30 seconds
    static constexpr int32 STANDING_HATED_MIN = -42000;
    static constexpr int32 STANDING_HOSTILE_MIN = -6000;
    static constexpr int32 STANDING_UNFRIENDLY_MIN = -3000;
    static constexpr int32 STANDING_NEUTRAL_MIN = 0;
    static constexpr int32 STANDING_FRIENDLY_MIN = 3000;
    static constexpr int32 STANDING_HONORED_MIN = 9000;
    static constexpr int32 STANDING_REVERED_MIN = 21000;
    static constexpr int32 STANDING_EXALTED_MIN = 42000;
};

} // namespace Playerbot
