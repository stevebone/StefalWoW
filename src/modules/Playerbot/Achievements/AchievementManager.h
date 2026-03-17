/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * ACHIEVEMENT MANAGER
 *
 * Phase 3: Humanization Core (Task 6)
 *
 * Manages achievement hunting for bots:
 * - Tracks achievement progress
 * - Suggests achievements to work on
 * - Prioritizes by difficulty, rewards, and bot capability
 * - Coordinates with other systems (questing, exploration, etc.)
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
 * @enum AchievementCategory
 * @brief Categories of achievements for bot prioritization
 */
enum class AchievementCategory : uint8
{
    NONE = 0,
    GENERAL,            // General achievements
    QUESTS,             // Quest-related achievements
    EXPLORATION,        // Exploration achievements
    PVP,                // PvP achievements
    DUNGEONS_RAIDS,     // Dungeon and raid achievements
    PROFESSIONS,        // Profession achievements
    REPUTATION,         // Reputation achievements
    WORLD_EVENTS,       // World event achievements
    FEATS_OF_STRENGTH,  // Feats of strength
    COLLECTIONS,        // Mounts, pets, toys, etc.

    MAX_CATEGORY
};

/**
 * @enum AchievementPriority
 * @brief Priority levels for achievement hunting
 */
enum class AchievementPriority : uint8
{
    LOW = 0,            // Nice to have
    NORMAL,             // Standard priority
    HIGH,               // Should work on soon
    URGENT              // Work on immediately (time-limited, etc.)
};

/**
 * @struct AchievementGoal
 * @brief A goal for achievement progress
 */
struct AchievementGoal
{
    uint32 achievementId{0};
    uint32 criteriaId{0};           // Specific criteria within achievement
    uint32 targetValue{0};          // Target count/progress
    uint32 currentValue{0};         // Current progress
    AchievementCategory category{AchievementCategory::NONE};
    AchievementPriority priority{AchievementPriority::NORMAL};
    std::string description;
    bool isCompleted{false};
    bool isTimeLimited{false};      // World event, etc.
    std::chrono::system_clock::time_point deadline;  // For time-limited

    float GetProgress() const
    {
        if (targetValue == 0)
            return 0.0f;
        return std::min(1.0f, static_cast<float>(currentValue) / static_cast<float>(targetValue));
    }

    bool IsComplete() const
    {
        return isCompleted || (targetValue > 0 && currentValue >= targetValue);
    }
};

/**
 * @struct AchievementSession
 * @brief Tracks an achievement hunting session
 */
struct AchievementSession
{
    std::vector<AchievementGoal> activeGoals;
    std::chrono::steady_clock::time_point startTime;
    uint32 achievementsCompleted{0};
    uint32 criteriaProgress{0};
    bool isActive{false};

    void Reset()
    {
        activeGoals.clear();
        achievementsCompleted = 0;
        criteriaProgress = 0;
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
 * @brief Callback for achievement events
 */
using AchievementCallback = std::function<void(uint32 achievementId, bool completed)>;

/**
 * @class AchievementManager
 * @brief Manages achievement hunting for bots
 *
 * This manager:
 * - Analyzes available achievements
 * - Suggests achievements based on bot capabilities
 * - Tracks progress on active goals
 * - Coordinates with questing, exploration, and other systems
 *
 * Update interval: 5000ms (5 seconds)
 */
class TC_GAME_API AchievementManager : public BehaviorManager
{
public:
    explicit AchievementManager(Player* bot, BotAI* ai);
    ~AchievementManager() override = default;

    // ========================================================================
    // FAST STATE QUERIES
    // ========================================================================

    /**
     * @brief Check if bot is actively hunting achievements
     */
    bool IsHunting() const { return _currentSession.isActive; }

    /**
     * @brief Get number of active achievement goals
     */
    uint32 GetActiveGoalCount() const { return static_cast<uint32>(_currentSession.activeGoals.size()); }

    /**
     * @brief Check if specific achievement is completed
     * @param achievementId Achievement ID
     */
    bool IsAchievementCompleted(uint32 achievementId) const;

    /**
     * @brief Check if bot can work on an achievement
     * @param achievementId Achievement ID
     */
    bool CanWorkOnAchievement(uint32 achievementId) const;

    // ========================================================================
    // ACHIEVEMENT ANALYSIS
    // ========================================================================

    /**
     * @brief Get suggested achievements to work on
     * @param category Filter by category (NONE = all)
     * @param maxCount Maximum number of suggestions
     * @return Vector of achievement goals sorted by priority
     */
    std::vector<AchievementGoal> GetSuggestedAchievements(
        AchievementCategory category = AchievementCategory::NONE,
        uint32 maxCount = 10) const;

    /**
     * @brief Get achievements in progress (partially completed)
     * @return Vector of achievement goals with progress
     */
    std::vector<AchievementGoal> GetInProgressAchievements() const;

    /**
     * @brief Get progress on specific achievement
     * @param achievementId Achievement ID
     * @return Achievement goal with progress, or empty if not found
     */
    AchievementGoal GetAchievementProgress(uint32 achievementId) const;

    /**
     * @brief Get total achievement points
     */
    uint32 GetAchievementPoints() const;

    /**
     * @brief Get completion percentage by category
     * @param category Achievement category
     * @return Completion percentage (0.0 to 1.0)
     */
    float GetCategoryCompletion(AchievementCategory category) const;

    // ========================================================================
    // SESSION CONTROL
    // ========================================================================

    /**
     * @brief Start an achievement hunting session
     * @param goals Specific goals to work on (empty = auto-select)
     * @return true if session started
     */
    bool StartSession(std::vector<AchievementGoal> const& goals = {});

    /**
     * @brief Stop the current session
     * @param reason Why stopping
     */
    void StopSession(std::string const& reason = "");

    /**
     * @brief Add a goal to current session
     * @param achievementId Achievement to add
     * @return true if added
     */
    bool AddGoal(uint32 achievementId);

    /**
     * @brief Remove a goal from current session
     * @param achievementId Achievement to remove
     */
    void RemoveGoal(uint32 achievementId);

    /**
     * @brief Get current session info
     */
    AchievementSession const& GetCurrentSession() const { return _currentSession; }

    // ========================================================================
    // SPECIFIC ACHIEVEMENT TYPES
    // ========================================================================

    /**
     * @brief Get exploration achievements for current zone
     * @return Vector of exploration goals
     */
    std::vector<AchievementGoal> GetExplorationGoals() const;

    /**
     * @brief Get quest achievements for current zone
     * @return Vector of quest-related goals
     */
    std::vector<AchievementGoal> GetQuestGoals() const;

    /**
     * @brief Get profession achievements
     * @return Vector of profession goals
     */
    std::vector<AchievementGoal> GetProfessionGoals() const;

    /**
     * @brief Get reputation achievements
     * @return Vector of reputation goals
     */
    std::vector<AchievementGoal> GetReputationGoals() const;

    /**
     * @brief Get collection achievements (mounts, pets, etc.)
     * @return Vector of collection goals
     */
    std::vector<AchievementGoal> GetCollectionGoals() const;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /**
     * @brief Set whether to auto-select achievements
     */
    void SetAutoSelect(bool enable) { _autoSelect = enable; }

    /**
     * @brief Set priority filter (only work on achievements >= this priority)
     */
    void SetMinPriority(AchievementPriority priority) { _minPriority = priority; }

    /**
     * @brief Set callback for achievement events
     */
    void SetCallback(AchievementCallback callback) { _callback = std::move(callback); }

    /**
     * @brief Enable/disable specific category
     */
    void SetCategoryEnabled(AchievementCategory category, bool enabled);

    /**
     * @brief Check if category is enabled
     */
    bool IsCategoryEnabled(AchievementCategory category) const;

    // ========================================================================
    // STATISTICS
    // ========================================================================

    struct AchievementStatistics
    {
        std::atomic<uint32> totalCompleted{0};
        std::atomic<uint32> sessionCompleted{0};
        std::atomic<uint32> criteriaUpdated{0};
        std::atomic<uint32> suggestionsFollowed{0};
        std::atomic<uint64> totalHuntingTimeMs{0};

        void Reset()
        {
            totalCompleted = 0;
            sessionCompleted = 0;
            criteriaUpdated = 0;
            suggestionsFollowed = 0;
            totalHuntingTimeMs = 0;
        }
    };

    AchievementStatistics const& GetStatistics() const { return _statistics; }

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
     * @brief Analyze achievements and build cache
     */
    void AnalyzeAchievements();

    /**
     * @brief Update progress on active goals
     */
    void UpdateGoalProgress();

    /**
     * @brief Select goals automatically based on context
     * @return Vector of selected goals
     */
    std::vector<AchievementGoal> AutoSelectGoals() const;

    /**
     * @brief Calculate priority for an achievement
     * @param achievementId Achievement ID
     * @return Priority level
     */
    AchievementPriority CalculatePriority(uint32 achievementId) const;

    /**
     * @brief Get category for achievement
     * @param achievementId Achievement ID
     * @return Achievement category
     */
    AchievementCategory GetAchievementCategory(uint32 achievementId) const;

    /**
     * @brief Check if achievement is feasible for bot
     * @param achievementId Achievement ID
     * @return true if bot can work toward this
     */
    bool IsAchievementFeasible(uint32 achievementId) const;

    /**
     * @brief Notify callback of achievement event
     */
    void NotifyCallback(uint32 achievementId, bool completed);

    // ========================================================================
    // DATA
    // ========================================================================

    // Session state
    AchievementSession _currentSession;

    // Configuration
    bool _autoSelect{true};
    AchievementPriority _minPriority{AchievementPriority::LOW};
    std::unordered_set<AchievementCategory> _enabledCategories;

    // Cache
    std::unordered_set<uint32> _completedAchievements;
    std::unordered_map<uint32, AchievementGoal> _inProgressCache;
    std::chrono::steady_clock::time_point _lastAnalysis;

    // Callback
    AchievementCallback _callback;

    // Statistics
    AchievementStatistics _statistics;

    // Constants
    static constexpr uint32 ANALYSIS_INTERVAL_MS = 60000;   // 1 minute
    static constexpr uint32 MAX_ACTIVE_GOALS = 5;
};

} // namespace Playerbot
