/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "AchievementManager.h"
#include "AI/BotAI.h"
#include "Player.h"
#include "AchievementMgr.h"
#include "AchievementPackets.h"
#include "DB2Stores.h"
#include "Log.h"
#include <algorithm>

namespace Playerbot
{

AchievementManager::AchievementManager(Player* bot, BotAI* ai)
    : BehaviorManager(bot, ai, 5000, "AchievementManager")  // 5 second update
{
    // Enable all categories by default
    for (uint8 i = 0; i < static_cast<uint8>(AchievementCategory::MAX_CATEGORY); ++i)
    {
        _enabledCategories.insert(static_cast<AchievementCategory>(i));
    }
}

bool AchievementManager::OnInitialize()
{
    if (!GetBot() || !GetBot()->IsInWorld())
        return false;

    AnalyzeAchievements();
    _lastAnalysis = std::chrono::steady_clock::now();

    return true;
}

void AchievementManager::OnShutdown()
{
    if (_currentSession.isActive)
    {
        StopSession("Shutdown");
    }

    _completedAchievements.clear();
    _inProgressCache.clear();
}

void AchievementManager::OnUpdate(uint32 elapsed)
{
    if (!GetBot() || !GetBot()->IsInWorld())
        return;

    // Re-analyze achievements periodically
    auto now = std::chrono::steady_clock::now();
    auto timeSinceAnalysis = std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastAnalysis).count();
    if (timeSinceAnalysis >= ANALYSIS_INTERVAL_MS)
    {
        AnalyzeAchievements();
        _lastAnalysis = now;
    }

    // Update progress on active goals
    if (_currentSession.isActive)
    {
        UpdateGoalProgress();

        // Auto-select new goals if needed
        if (_autoSelect && _currentSession.activeGoals.empty())
        {
            auto newGoals = AutoSelectGoals();
            for (auto const& goal : newGoals)
            {
                _currentSession.activeGoals.push_back(goal);
            }
        }
    }
}

void AchievementManager::AnalyzeAchievements()
{
    if (!GetBot())
        return;

    // Clear caches
    _completedAchievements.clear();
    _inProgressCache.clear();

    // Get player's achievement manager
    // Note: TrinityCore handles achievement data internally
    // We query the player's completed achievements

    // Build completed achievement set
    // TrinityCore 11.x uses Player::GetAchievementMgr() for achievement tracking
    // The actual implementation depends on TrinityCore's current API

    TC_LOG_DEBUG("module.playerbot.achievements",
        "AchievementManager: Analyzed achievements for bot {}",
        GetBot()->GetName());
}

void AchievementManager::UpdateGoalProgress()
{
    if (!GetBot())
        return;

    bool anyCompleted = false;

    for (auto& goal : _currentSession.activeGoals)
    {
        if (goal.isCompleted)
            continue;

        // Query current progress from TrinityCore
        // Note: Actual implementation would use TrinityCore's achievement criteria tracking

        // Check for completion
        if (goal.IsComplete() && !goal.isCompleted)
        {
            goal.isCompleted = true;
            anyCompleted = true;
            _currentSession.achievementsCompleted++;
            _statistics.sessionCompleted++;
            _statistics.totalCompleted++;

            NotifyCallback(goal.achievementId, true);

            TC_LOG_DEBUG("module.playerbot.achievements",
                "AchievementManager: Bot {} completed achievement {}",
                GetBot()->GetName(), goal.achievementId);
        }
    }

    // Remove completed goals
    if (anyCompleted)
    {
        _currentSession.activeGoals.erase(
            std::remove_if(_currentSession.activeGoals.begin(), _currentSession.activeGoals.end(),
                [](AchievementGoal const& g) { return g.isCompleted; }),
            _currentSession.activeGoals.end());
    }
}

bool AchievementManager::IsAchievementCompleted(uint32 achievementId) const
{
    return _completedAchievements.count(achievementId) > 0;
}

bool AchievementManager::CanWorkOnAchievement(uint32 achievementId) const
{
    // Check if already completed
    if (IsAchievementCompleted(achievementId))
        return false;

    // Check if feasible
    if (!IsAchievementFeasible(achievementId))
        return false;

    // Check if category is enabled
    AchievementCategory category = GetAchievementCategory(achievementId);
    if (!IsCategoryEnabled(category))
        return false;

    return true;
}

std::vector<AchievementGoal> AchievementManager::GetSuggestedAchievements(
    AchievementCategory category,
    uint32 maxCount) const
{
    std::vector<AchievementGoal> suggestions;

    // Start with in-progress achievements
    for (auto const& [id, goal] : _inProgressCache)
    {
        if (category != AchievementCategory::NONE && goal.category != category)
            continue;

        if (goal.priority < _minPriority)
            continue;

        if (!CanWorkOnAchievement(id))
            continue;

        suggestions.push_back(goal);
    }

    // Sort by priority (highest first), then by progress (highest first)
    std::sort(suggestions.begin(), suggestions.end(),
        [](AchievementGoal const& a, AchievementGoal const& b) {
            if (a.priority != b.priority)
                return a.priority > b.priority;
            return a.GetProgress() > b.GetProgress();
        });

    // Limit results
    if (suggestions.size() > maxCount)
    {
        suggestions.resize(maxCount);
    }

    return suggestions;
}

std::vector<AchievementGoal> AchievementManager::GetInProgressAchievements() const
{
    std::vector<AchievementGoal> result;

    for (auto const& [id, goal] : _inProgressCache)
    {
        if (goal.GetProgress() > 0.0f && !goal.isCompleted)
        {
            result.push_back(goal);
        }
    }

    return result;
}

AchievementGoal AchievementManager::GetAchievementProgress(uint32 achievementId) const
{
    auto it = _inProgressCache.find(achievementId);
    if (it != _inProgressCache.end())
    {
        return it->second;
    }

    return AchievementGoal();
}

uint32 AchievementManager::GetAchievementPoints() const
{
    if (!GetBot())
        return 0;

    // TrinityCore tracks achievement points internally
    // Would query via Player::GetAchievementMgr()->GetAchievementPoints()
    return 0;
}

float AchievementManager::GetCategoryCompletion(AchievementCategory /*category*/) const
{
    // Would iterate through achievements in category and calculate completion
    return 0.0f;
}

bool AchievementManager::StartSession(std::vector<AchievementGoal> const& goals)
{
    if (_currentSession.isActive)
    {
        TC_LOG_DEBUG("module.playerbot.achievements",
            "AchievementManager: Session already active for bot {}",
            GetBot() ? GetBot()->GetName() : "unknown");
        return false;
    }

    _currentSession.Reset();
    _currentSession.isActive = true;
    _currentSession.startTime = std::chrono::steady_clock::now();

    if (goals.empty() && _autoSelect)
    {
        _currentSession.activeGoals = AutoSelectGoals();
    }
    else
    {
        _currentSession.activeGoals = goals;
    }

    TC_LOG_DEBUG("module.playerbot.achievements",
        "AchievementManager: Started session with {} goals for bot {}",
        _currentSession.activeGoals.size(),
        GetBot() ? GetBot()->GetName() : "unknown");

    return true;
}

void AchievementManager::StopSession(std::string const& reason)
{
    if (!_currentSession.isActive)
        return;

    _statistics.totalHuntingTimeMs += _currentSession.GetElapsedMs();

    TC_LOG_DEBUG("module.playerbot.achievements",
        "AchievementManager: Stopped session for bot {}, reason: {}, completed: {}",
        GetBot() ? GetBot()->GetName() : "unknown",
        reason.empty() ? "none" : reason.c_str(),
        _currentSession.achievementsCompleted);

    _currentSession.Reset();
}

bool AchievementManager::AddGoal(uint32 achievementId)
{
    if (!_currentSession.isActive)
        return false;

    if (!CanWorkOnAchievement(achievementId))
        return false;

    // Check if already in session
    for (auto const& goal : _currentSession.activeGoals)
    {
        if (goal.achievementId == achievementId)
            return false;
    }

    // Check max goals
    if (_currentSession.activeGoals.size() >= MAX_ACTIVE_GOALS)
        return false;

    // Create goal
    AchievementGoal goal;
    goal.achievementId = achievementId;
    goal.category = GetAchievementCategory(achievementId);
    goal.priority = CalculatePriority(achievementId);

    // Get current progress from cache
    auto it = _inProgressCache.find(achievementId);
    if (it != _inProgressCache.end())
    {
        goal.currentValue = it->second.currentValue;
        goal.targetValue = it->second.targetValue;
        goal.description = it->second.description;
    }

    _currentSession.activeGoals.push_back(goal);
    _statistics.suggestionsFollowed++;

    return true;
}

void AchievementManager::RemoveGoal(uint32 achievementId)
{
    _currentSession.activeGoals.erase(
        std::remove_if(_currentSession.activeGoals.begin(), _currentSession.activeGoals.end(),
            [achievementId](AchievementGoal const& g) { return g.achievementId == achievementId; }),
        _currentSession.activeGoals.end());
}

std::vector<AchievementGoal> AchievementManager::GetExplorationGoals() const
{
    return GetSuggestedAchievements(AchievementCategory::EXPLORATION, 5);
}

std::vector<AchievementGoal> AchievementManager::GetQuestGoals() const
{
    return GetSuggestedAchievements(AchievementCategory::QUESTS, 5);
}

std::vector<AchievementGoal> AchievementManager::GetProfessionGoals() const
{
    return GetSuggestedAchievements(AchievementCategory::PROFESSIONS, 5);
}

std::vector<AchievementGoal> AchievementManager::GetReputationGoals() const
{
    return GetSuggestedAchievements(AchievementCategory::REPUTATION, 5);
}

std::vector<AchievementGoal> AchievementManager::GetCollectionGoals() const
{
    return GetSuggestedAchievements(AchievementCategory::COLLECTIONS, 5);
}

void AchievementManager::SetCategoryEnabled(AchievementCategory category, bool enabled)
{
    if (enabled)
    {
        _enabledCategories.insert(category);
    }
    else
    {
        _enabledCategories.erase(category);
    }
}

bool AchievementManager::IsCategoryEnabled(AchievementCategory category) const
{
    return _enabledCategories.count(category) > 0;
}

std::vector<AchievementGoal> AchievementManager::AutoSelectGoals() const
{
    std::vector<AchievementGoal> selected;

    // Get suggestions
    auto suggestions = GetSuggestedAchievements(AchievementCategory::NONE, MAX_ACTIVE_GOALS);

    // Prioritize time-limited first
    for (auto const& goal : suggestions)
    {
        if (goal.isTimeLimited && selected.size() < MAX_ACTIVE_GOALS)
        {
            selected.push_back(goal);
        }
    }

    // Then add high-progress goals
    for (auto const& goal : suggestions)
    {
        if (!goal.isTimeLimited && goal.GetProgress() > 0.5f && selected.size() < MAX_ACTIVE_GOALS)
        {
            selected.push_back(goal);
        }
    }

    // Fill remaining slots
    for (auto const& goal : suggestions)
    {
        bool alreadySelected = false;
        for (auto const& sel : selected)
        {
            if (sel.achievementId == goal.achievementId)
            {
                alreadySelected = true;
                break;
            }
        }

        if (!alreadySelected && selected.size() < MAX_ACTIVE_GOALS)
        {
            selected.push_back(goal);
        }
    }

    return selected;
}

AchievementPriority AchievementManager::CalculatePriority(uint32 /*achievementId*/) const
{
    // Would analyze achievement requirements and calculate priority
    // - Time-limited = URGENT
    // - High progress = HIGH
    // - Easy to complete = NORMAL
    // - Long-term = LOW

    return AchievementPriority::NORMAL;
}

AchievementCategory AchievementManager::GetAchievementCategory(uint32 /*achievementId*/) const
{
    // Would look up achievement category from DB2 data
    return AchievementCategory::GENERAL;
}

bool AchievementManager::IsAchievementFeasible(uint32 /*achievementId*/) const
{
    // Would check:
    // - Level requirements
    // - Faction requirements
    // - Class/race requirements
    // - Prerequisites

    return true;
}

void AchievementManager::NotifyCallback(uint32 achievementId, bool completed)
{
    if (_callback)
    {
        _callback(achievementId, completed);
    }
}

} // namespace Playerbot
