/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * ACHIEVEMENT GRINDER
 *
 * Phase 3: Humanization Core (GOD_TIER Task 10)
 *
 * Executes achievement grinding strategies for bots:
 * - Exploration achievements (zone discovery)
 * - Quest achievements (zone quest completion)
 * - Kill achievements (creature kills)
 * - Dungeon/Raid achievements (instance completion)
 * - Collection achievements (mounts, pets, toys)
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
class AchievementManager;

/**
 * @enum AchievementGrindType
 * @brief Type of achievement grind activity
 */
enum class AchievementGrindType : uint8
{
    NONE = 0,
    EXPLORATION,        // Discovering zones/areas
    QUEST,              // Completing quests
    KILL,               // Killing creatures
    DUNGEON,            // Dungeon achievements
    RAID,               // Raid achievements
    COLLECTION,         // Collect X items/mounts/pets
    REPUTATION,         // Reputation grinding
    PROFESSION,         // Profession-related
    PVP,                // PvP achievements
    MISC                // Other achievement types
};

/**
 * @enum GrindPhase
 * @brief Current phase of achievement grinding
 */
enum class GrindPhase : uint8
{
    IDLE = 0,
    NAVIGATING,         // Moving to location
    EXECUTING,          // Performing grind action
    WAITING,            // Waiting for cooldown/respawn
    COMPLETED           // Achievement completed
};

/**
 * @struct ExplorationTarget
 * @brief Target area for exploration achievement
 */
struct ExplorationTarget
{
    uint32 areaId{0};
    uint32 zoneId{0};
    std::string name;
    Position centerPoint;
    float explorationRadius{0.0f};
    bool isDiscovered{false};
    bool isSubZone{false};

    float GetPriorityScore() const
    {
        float score = 100.0f;
        if (isDiscovered)
            return 0.0f;
        if (isSubZone)
            score -= 10.0f;  // Prefer main zones
        return score;
    }
};

/**
 * @struct KillTarget
 * @brief Target creature for kill achievement
 */
struct KillTarget
{
    uint32 creatureEntry{0};
    std::string name;
    uint32 requiredKills{0};
    uint32 currentKills{0};
    std::vector<Position> spawnLocations;
    uint32 respawnTime{0};      // Seconds
    bool isBoss{false};
    bool isRare{false};
    uint32 instanceId{0};       // Non-zero if in instance

    float GetProgress() const
    {
        if (requiredKills == 0)
            return 1.0f;
        return std::min(1.0f, static_cast<float>(currentKills) / static_cast<float>(requiredKills));
    }

    float GetPriorityScore() const
    {
        float score = 100.0f;
        score += GetProgress() * 50.0f;  // Boost nearly complete
        if (isBoss)
            score -= 20.0f;  // Bosses are harder
        if (isRare)
            score -= 15.0f;  // Rares are harder to find
        if (instanceId > 0)
            score -= 10.0f;  // Instance access required
        return score;
    }
};

/**
 * @struct AchievementGrindSession
 * @brief Tracks an active achievement grinding session
 */
struct AchievementGrindSession
{
    uint32 achievementId{0};
    AchievementGrindType type{AchievementGrindType::NONE};
    GrindPhase phase{GrindPhase::IDLE};
    std::chrono::steady_clock::time_point startTime;
    bool isActive{false};

    // Progress tracking
    uint32 criteriaCompleted{0};
    uint32 totalCriteria{0};
    float progressAtStart{0.0f};
    float currentProgress{0.0f};

    // Navigation state
    Position targetPosition;
    uint32 targetAreaId{0};
    uint32 targetCreatureEntry{0};
    bool isNavigating{false};

    // Execution state
    uint32 killsThisSession{0};
    uint32 areasExploredThisSession{0};
    uint32 questsCompletedThisSession{0};

    void Reset()
    {
        achievementId = 0;
        type = AchievementGrindType::NONE;
        phase = GrindPhase::IDLE;
        isActive = false;
        criteriaCompleted = 0;
        totalCriteria = 0;
        progressAtStart = 0.0f;
        currentProgress = 0.0f;
        isNavigating = false;
        killsThisSession = 0;
        areasExploredThisSession = 0;
        questsCompletedThisSession = 0;
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
 * @brief Callback for grind events
 */
using AchievementGrindCallback = std::function<void(uint32 achievementId, bool completed)>;

/**
 * @class AchievementGrinder
 * @brief Executes achievement grinding strategies for bots
 *
 * This manager:
 * - Analyzes achievement requirements
 * - Executes appropriate grinding strategy
 * - Coordinates with navigation, combat, quest systems
 * - Tracks progress and adjusts strategy
 *
 * Works with existing AchievementManager for goal selection.
 * This class handles the EXECUTION of grinding.
 *
 * Update interval: 5000ms (5 seconds)
 */
class TC_GAME_API AchievementGrinder : public BehaviorManager
{
public:
    explicit AchievementGrinder(Player* bot, BotAI* ai);
    ~AchievementGrinder() override = default;

    // ========================================================================
    // FAST STATE QUERIES
    // ========================================================================

    /**
     * @brief Check if bot is actively grinding achievements
     */
    bool IsGrinding() const { return _currentSession.isActive; }

    /**
     * @brief Get current grind type
     */
    AchievementGrindType GetCurrentType() const { return _currentSession.type; }

    /**
     * @brief Get current grind phase
     */
    GrindPhase GetCurrentPhase() const { return _currentSession.phase; }

    /**
     * @brief Get current target achievement
     */
    uint32 GetCurrentAchievement() const { return _currentSession.achievementId; }

    /**
     * @brief Get current grind progress
     */
    float GetCurrentProgress() const { return _currentSession.currentProgress; }

    // ========================================================================
    // GRIND EXECUTION
    // ========================================================================

    /**
     * @brief Start grinding a specific achievement
     * @param achievementId Achievement to grind
     * @return true if grind started
     */
    bool StartGrind(uint32 achievementId);

    /**
     * @brief Stop current grind session
     * @param reason Reason for stopping
     */
    void StopGrind(std::string const& reason = "");

    /**
     * @brief Get current session info
     */
    AchievementGrindSession const& GetCurrentSession() const { return _currentSession; }

    // ========================================================================
    // SPECIFIC ACHIEVEMENT TYPES
    // ========================================================================

    /**
     * @brief Execute exploration achievement
     * @param achievementId Exploration achievement ID
     * @return true if execution started
     */
    bool ExecuteExplorationAchievement(uint32 achievementId);

    /**
     * @brief Execute quest achievement
     * @param achievementId Quest achievement ID
     * @return true if execution started
     */
    bool ExecuteQuestAchievement(uint32 achievementId);

    /**
     * @brief Execute kill achievement
     * @param achievementId Kill achievement ID
     * @return true if execution started
     */
    bool ExecuteKillAchievement(uint32 achievementId);

    /**
     * @brief Execute dungeon achievement
     * @param achievementId Dungeon achievement ID
     * @return true if execution started
     */
    bool ExecuteDungeonAchievement(uint32 achievementId);

    /**
     * @brief Execute raid achievement
     * @param achievementId Raid achievement ID
     * @return true if execution started
     */
    bool ExecuteRaidAchievement(uint32 achievementId);

    /**
     * @brief Execute collection achievement
     * @param achievementId Collection achievement ID
     * @return true if execution started
     */
    bool ExecuteCollectionAchievement(uint32 achievementId);

    /**
     * @brief Execute reputation achievement
     * @param achievementId Reputation achievement ID
     * @return true if execution started
     */
    bool ExecuteReputationAchievement(uint32 achievementId);

    // ========================================================================
    // EXPLORATION
    // ========================================================================

    /**
     * @brief Get exploration targets for achievement
     * @param achievementId Achievement ID
     * @return Vector of exploration targets
     */
    std::vector<ExplorationTarget> GetExplorationTargets(uint32 achievementId) const;

    /**
     * @brief Get next undiscovered area
     * @return Next exploration target or empty
     */
    ExplorationTarget GetNextExplorationTarget() const;

    /**
     * @brief Check if area is discovered
     * @param areaId Area ID to check
     * @return true if discovered
     */
    bool IsAreaDiscovered(uint32 areaId) const;

    /**
     * @brief Navigate to exploration target
     * @param target Target to explore
     * @return true if navigation started
     */
    bool NavigateToExploration(ExplorationTarget const& target);

    // ========================================================================
    // KILLS
    // ========================================================================

    /**
     * @brief Get kill targets for achievement
     * @param achievementId Achievement ID
     * @return Vector of kill targets
     */
    std::vector<KillTarget> GetKillTargets(uint32 achievementId) const;

    /**
     * @brief Get next creature to kill
     * @return Next kill target or empty
     */
    KillTarget GetNextKillTarget() const;

    /**
     * @brief Navigate to kill target
     * @param target Target to kill
     * @return true if navigation started
     */
    bool NavigateToKillTarget(KillTarget const& target);

    /**
     * @brief Handle creature killed
     * @param creatureEntry Creature entry killed
     */
    void OnCreatureKilled(uint32 creatureEntry);

    // ========================================================================
    // QUESTS
    // ========================================================================

    /**
     * @brief Get quests needed for achievement
     * @param achievementId Achievement ID
     * @return Vector of quest IDs
     */
    std::vector<uint32> GetAchievementQuests(uint32 achievementId) const;

    /**
     * @brief Get next quest to complete
     * @return Quest ID or 0 if none
     */
    uint32 GetNextQuestToComplete() const;

    /**
     * @brief Handle quest completed
     * @param questId Quest ID completed
     */
    void OnQuestCompleted(uint32 questId);

    // ========================================================================
    // DUNGEONS/RAIDS
    // ========================================================================

    /**
     * @brief Get instance ID for achievement
     * @param achievementId Achievement ID
     * @return Instance map ID or 0
     */
    uint32 GetAchievementInstance(uint32 achievementId) const;

    /**
     * @brief Check if instance is accessible
     * @param mapId Instance map ID
     * @return true if bot can enter
     */
    bool CanEnterInstance(uint32 mapId) const;

    /**
     * @brief Start instance run for achievement
     * @param achievementId Achievement ID
     * @return true if instance run started
     */
    bool StartInstanceRun(uint32 achievementId);

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /**
     * @brief Set maximum grind duration
     */
    void SetMaxGrindDuration(uint32 durationMs) { _maxGrindDuration = durationMs; }

    /**
     * @brief Set callback for grind events
     */
    void SetCallback(AchievementGrindCallback callback) { _callback = std::move(callback); }

    /**
     * @brief Enable/disable specific grind types
     */
    void SetGrindTypeEnabled(AchievementGrindType type, bool enabled);

    /**
     * @brief Check if grind type is enabled
     */
    bool IsGrindTypeEnabled(AchievementGrindType type) const;

    // ========================================================================
    // STATISTICS
    // ========================================================================

    struct GrindStatistics
    {
        std::atomic<uint32> achievementsCompleted{0};
        std::atomic<uint32> areasExplored{0};
        std::atomic<uint32> creaturesKilled{0};
        std::atomic<uint32> questsCompleted{0};
        std::atomic<uint32> dungeonsCleared{0};
        std::atomic<uint32> raidsCleared{0};
        std::atomic<uint64> totalGrindTimeMs{0};

        void Reset()
        {
            achievementsCompleted = 0;
            areasExplored = 0;
            creaturesKilled = 0;
            questsCompleted = 0;
            dungeonsCleared = 0;
            raidsCleared = 0;
            totalGrindTimeMs = 0;
        }
    };

    GrindStatistics const& GetStatistics() const { return _statistics; }
    static GrindStatistics const& GetGlobalStatistics() { return _globalStatistics; }

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
     * @brief Determine grind type for achievement
     */
    AchievementGrindType DetermineGrindType(uint32 achievementId) const;

    /**
     * @brief Update grind session state
     */
    void UpdateGrindSession(uint32 elapsed);

    /**
     * @brief Execute exploration grind step
     */
    void ExecuteExplorationStep();

    /**
     * @brief Execute kill grind step
     */
    void ExecuteKillStep();

    /**
     * @brief Execute quest grind step
     */
    void ExecuteQuestStep();

    /**
     * @brief Execute dungeon grind step
     */
    void ExecuteDungeonStep();

    /**
     * @brief Execute collection grind step
     */
    void ExecuteCollectionStep();

    /**
     * @brief Check if achievement completed
     */
    void CheckAchievementCompleted();

    /**
     * @brief Update progress tracking
     */
    void UpdateProgress();

    /**
     * @brief Navigate to target position
     */
    bool NavigateToPosition(Position const& pos);

    /**
     * @brief Get AchievementManager reference
     */
    AchievementManager* GetAchievementManager() const;

    /**
     * @brief Load achievement data
     */
    void LoadAchievementData(uint32 achievementId);

    /**
     * @brief Notify callback of grind event
     */
    void NotifyCallback(uint32 achievementId, bool completed);

    // ========================================================================
    // DATA
    // ========================================================================

    // Session state
    AchievementGrindSession _currentSession;

    // Configuration
    uint32 _maxGrindDuration{7200000};  // 2 hours default
    std::unordered_set<AchievementGrindType> _enabledTypes;

    // Exploration data
    std::vector<ExplorationTarget> _explorationTargets;
    std::unordered_set<uint32> _discoveredAreas;

    // Kill data
    std::vector<KillTarget> _killTargets;
    std::unordered_map<uint32, uint32> _killProgress;  // creatureEntry -> kills

    // Quest data
    std::vector<uint32> _requiredQuests;
    std::unordered_set<uint32> _completedQuests;

    // Instance data
    uint32 _targetInstanceId{0};
    bool _inInstance{false};

    // Callback
    AchievementGrindCallback _callback;

    // Statistics
    GrindStatistics _statistics;
    static GrindStatistics _globalStatistics;

    // Static achievement database (shared across all bots)
    static std::unordered_map<uint32, AchievementGrindType> _achievementTypes;
    static bool _databaseLoaded;

    // Constants
    static constexpr uint32 PROGRESS_CHECK_INTERVAL_MS = 10000;  // 10 seconds
    static constexpr float EXPLORATION_DISCOVER_RADIUS = 50.0f;  // Yards to "discover" area
};

} // namespace Playerbot
