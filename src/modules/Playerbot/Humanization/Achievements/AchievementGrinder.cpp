/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "AchievementGrinder.h"
#include "AI/BotAI.h"
#include "Player.h"
#include "Achievements/AchievementManager.h"
#include "DB2Stores.h"
#include "AchievementMgr.h"
#include "Log.h"
#include "World.h"
#include "GameTime.h"
#include "Map.h"
#include "MapManager.h"
#include <algorithm>

namespace Playerbot
{

// Static member definitions
AchievementGrinder::GrindStatistics AchievementGrinder::_globalStatistics;
std::unordered_map<uint32, AchievementGrindType> AchievementGrinder::_achievementTypes;
bool AchievementGrinder::_databaseLoaded = false;

AchievementGrinder::AchievementGrinder(Player* bot, BotAI* ai)
    : BehaviorManager(bot, ai, 5000, "AchievementGrinder")  // 5 second update
{
    // Enable all grind types by default
    _enabledTypes.insert(AchievementGrindType::EXPLORATION);
    _enabledTypes.insert(AchievementGrindType::QUEST);
    _enabledTypes.insert(AchievementGrindType::KILL);
    _enabledTypes.insert(AchievementGrindType::DUNGEON);
    _enabledTypes.insert(AchievementGrindType::COLLECTION);
    _enabledTypes.insert(AchievementGrindType::REPUTATION);
}

bool AchievementGrinder::OnInitialize()
{
    if (!GetBot() || !GetBot()->IsInWorld())
        return false;

    // Load static achievement type database once
    if (!_databaseLoaded)
    {
        // Load achievement type mappings
        // This would be populated from DB2/DBC data
        _databaseLoaded = true;
    }

    TC_LOG_DEBUG("module.playerbot.achievements",
        "AchievementGrinder: Initialized for {}",
        GetBot()->GetName());

    return true;
}

void AchievementGrinder::OnShutdown()
{
    if (_currentSession.isActive)
    {
        StopGrind("Shutdown");
    }

    _explorationTargets.clear();
    _killTargets.clear();
    _requiredQuests.clear();
    _discoveredAreas.clear();
    _completedQuests.clear();
}

void AchievementGrinder::OnUpdate(uint32 elapsed)
{
    if (!GetBot() || !GetBot()->IsInWorld())
        return;

    // Update grind session if active
    if (_currentSession.isActive)
    {
        UpdateGrindSession(elapsed);
    }
}

void AchievementGrinder::UpdateGrindSession(uint32 /*elapsed*/)
{
    if (!_currentSession.isActive)
        return;

    // Check if achievement completed
    CheckAchievementCompleted();

    // Check max duration
    if (_currentSession.GetElapsedMs() > _maxGrindDuration)
    {
        StopGrind("Max duration reached");
        return;
    }

    // Update progress
    UpdateProgress();

    // Execute grind step based on type
    switch (_currentSession.type)
    {
        case AchievementGrindType::EXPLORATION:
            ExecuteExplorationStep();
            break;
        case AchievementGrindType::KILL:
            ExecuteKillStep();
            break;
        case AchievementGrindType::QUEST:
            ExecuteQuestStep();
            break;
        case AchievementGrindType::DUNGEON:
        case AchievementGrindType::RAID:
            ExecuteDungeonStep();
            break;
        case AchievementGrindType::COLLECTION:
            ExecuteCollectionStep();
            break;
        default:
            break;
    }
}

void AchievementGrinder::ExecuteExplorationStep()
{
    // Get next exploration target
    ExplorationTarget target = GetNextExplorationTarget();
    if (target.areaId == 0)
    {
        // All areas explored for this achievement
        _currentSession.phase = GrindPhase::COMPLETED;
        return;
    }

    // Navigate to target if needed
    if (_currentSession.phase == GrindPhase::IDLE)
    {
        if (NavigateToExploration(target))
        {
            _currentSession.phase = GrindPhase::NAVIGATING;
            _currentSession.targetAreaId = target.areaId;
            _currentSession.targetPosition = target.centerPoint;
        }
    }
    else if (_currentSession.phase == GrindPhase::NAVIGATING)
    {
        // Check if we've reached the area
        if (!GetBot())
            return;

        float dist = GetBot()->GetDistance(target.centerPoint);
        if (dist < EXPLORATION_DISCOVER_RADIUS)
        {
            // Area discovered
            _discoveredAreas.insert(target.areaId);
            _currentSession.areasExploredThisSession++;
            _statistics.areasExplored.fetch_add(1, std::memory_order_relaxed);
            _globalStatistics.areasExplored.fetch_add(1, std::memory_order_relaxed);

            TC_LOG_DEBUG("module.playerbot.achievements",
                "AchievementGrinder: {} discovered area {} ({})",
                GetBot()->GetName(), target.areaId, target.name);

            // Reset to find next target
            _currentSession.phase = GrindPhase::IDLE;
        }
    }
}

void AchievementGrinder::ExecuteKillStep()
{
    // Get next kill target
    KillTarget target = GetNextKillTarget();
    if (target.creatureEntry == 0)
    {
        // All kills complete
        _currentSession.phase = GrindPhase::COMPLETED;
        return;
    }

    // Navigate to target if needed
    if (_currentSession.phase == GrindPhase::IDLE)
    {
        if (NavigateToKillTarget(target))
        {
            _currentSession.phase = GrindPhase::NAVIGATING;
            _currentSession.targetCreatureEntry = target.creatureEntry;
            if (!target.spawnLocations.empty())
            {
                _currentSession.targetPosition = target.spawnLocations[0];
            }
        }
    }
    else if (_currentSession.phase == GrindPhase::NAVIGATING)
    {
        // Check if we've reached the spawn location
        if (!GetBot())
            return;

        float dist = GetBot()->GetDistance(_currentSession.targetPosition);
        if (dist < 30.0f)
        {
            // At spawn location, start executing kills
            _currentSession.phase = GrindPhase::EXECUTING;
        }
    }
    else if (_currentSession.phase == GrindPhase::EXECUTING)
    {
        // Combat handled by CombatManager
        // We just track kills via OnCreatureKilled callback
    }
}

void AchievementGrinder::ExecuteQuestStep()
{
    // Get next quest to complete
    uint32 questId = GetNextQuestToComplete();
    if (questId == 0)
    {
        // All quests complete
        _currentSession.phase = GrindPhase::COMPLETED;
        return;
    }

    // Quest execution handled by QuestManager
    // We track completion via OnQuestCompleted callback
    _currentSession.phase = GrindPhase::EXECUTING;
}

void AchievementGrinder::ExecuteDungeonStep()
{
    // Get instance for achievement
    uint32 mapId = GetAchievementInstance(_currentSession.achievementId);
    if (mapId == 0)
        return;

    // Check if in instance
    if (!GetBot())
        return;

    if (GetBot()->GetMapId() == mapId)
    {
        _inInstance = true;
        _currentSession.phase = GrindPhase::EXECUTING;
        // Instance progression handled by InstanceManager/DungeonManager
    }
    else if (!_inInstance)
    {
        // Navigate to instance entrance
        if (CanEnterInstance(mapId))
        {
            StartInstanceRun(_currentSession.achievementId);
        }
    }
}

void AchievementGrinder::ExecuteCollectionStep()
{
    // Collection handled by MountCollectionManager/PetCollectionManager
    // We just track overall progress
    _currentSession.phase = GrindPhase::EXECUTING;
}

void AchievementGrinder::CheckAchievementCompleted()
{
    if (!GetBot() || !_currentSession.isActive)
        return;

    // Check if achievement is now complete
    // This would query TrinityCore's achievement system
    AchievementManager* achMgr = GetAchievementManager();
    if (achMgr && achMgr->IsAchievementCompleted(_currentSession.achievementId))
    {
        _statistics.achievementsCompleted.fetch_add(1, std::memory_order_relaxed);
        _globalStatistics.achievementsCompleted.fetch_add(1, std::memory_order_relaxed);

        TC_LOG_INFO("module.playerbot.achievements",
            "AchievementGrinder: {} completed achievement {}!",
            GetBot()->GetName(), _currentSession.achievementId);

        NotifyCallback(_currentSession.achievementId, true);
        StopGrind("Achievement completed");
    }
}

void AchievementGrinder::UpdateProgress()
{
    AchievementManager* achMgr = GetAchievementManager();
    if (!achMgr)
        return;

    auto progress = achMgr->GetAchievementProgress(_currentSession.achievementId);
    _currentSession.currentProgress = progress.GetProgress();

    // Calculate completed criteria from progress
    // totalCriteria is a count, so we estimate completed based on progress percentage
    if (_currentSession.totalCriteria > 0)
    {
        _currentSession.criteriaCompleted = static_cast<uint32>(
            _currentSession.currentProgress * static_cast<float>(_currentSession.totalCriteria));
    }
    else
    {
        _currentSession.criteriaCompleted = 0;
    }
}

bool AchievementGrinder::StartGrind(uint32 achievementId)
{
    if (_currentSession.isActive)
    {
        StopGrind("Starting new grind");
    }

    // Determine grind type
    AchievementGrindType type = DetermineGrindType(achievementId);
    if (!IsGrindTypeEnabled(type))
    {
        TC_LOG_DEBUG("module.playerbot.achievements",
            "AchievementGrinder: Grind type {} not enabled for {}",
            static_cast<uint8>(type), GetBot() ? GetBot()->GetName() : "unknown");
        return false;
    }

    // Load achievement data
    LoadAchievementData(achievementId);

    // Start session
    _currentSession.Reset();
    _currentSession.achievementId = achievementId;
    _currentSession.type = type;
    _currentSession.phase = GrindPhase::IDLE;
    _currentSession.startTime = std::chrono::steady_clock::now();
    _currentSession.isActive = true;

    // Get initial progress
    AchievementManager* achMgr = GetAchievementManager();
    if (achMgr)
    {
        auto progress = achMgr->GetAchievementProgress(achievementId);
        _currentSession.progressAtStart = progress.GetProgress();
        _currentSession.currentProgress = _currentSession.progressAtStart;
    }

    TC_LOG_DEBUG("module.playerbot.achievements",
        "AchievementGrinder: {} started grinding achievement {} (type: {})",
        GetBot() ? GetBot()->GetName() : "unknown",
        achievementId, static_cast<uint8>(type));

    return true;
}

void AchievementGrinder::StopGrind(std::string const& reason)
{
    if (!_currentSession.isActive)
        return;

    _statistics.totalGrindTimeMs.fetch_add(_currentSession.GetElapsedMs(), std::memory_order_relaxed);
    _globalStatistics.totalGrindTimeMs.fetch_add(_currentSession.GetElapsedMs(), std::memory_order_relaxed);

    TC_LOG_DEBUG("module.playerbot.achievements",
        "AchievementGrinder: {} stopped grinding achievement {}, reason: {}, progress: {:.1f}% -> {:.1f}%",
        GetBot() ? GetBot()->GetName() : "unknown",
        _currentSession.achievementId,
        reason.empty() ? "none" : reason.c_str(),
        _currentSession.progressAtStart * 100.0f,
        _currentSession.currentProgress * 100.0f);

    _currentSession.Reset();
    _explorationTargets.clear();
    _killTargets.clear();
    _requiredQuests.clear();
}

bool AchievementGrinder::ExecuteExplorationAchievement(uint32 achievementId)
{
    if (DetermineGrindType(achievementId) != AchievementGrindType::EXPLORATION)
        return false;

    return StartGrind(achievementId);
}

bool AchievementGrinder::ExecuteQuestAchievement(uint32 achievementId)
{
    if (DetermineGrindType(achievementId) != AchievementGrindType::QUEST)
        return false;

    return StartGrind(achievementId);
}

bool AchievementGrinder::ExecuteKillAchievement(uint32 achievementId)
{
    if (DetermineGrindType(achievementId) != AchievementGrindType::KILL)
        return false;

    return StartGrind(achievementId);
}

bool AchievementGrinder::ExecuteDungeonAchievement(uint32 achievementId)
{
    auto type = DetermineGrindType(achievementId);
    if (type != AchievementGrindType::DUNGEON && type != AchievementGrindType::RAID)
        return false;

    return StartGrind(achievementId);
}

bool AchievementGrinder::ExecuteRaidAchievement(uint32 achievementId)
{
    return ExecuteDungeonAchievement(achievementId);
}

bool AchievementGrinder::ExecuteCollectionAchievement(uint32 achievementId)
{
    if (DetermineGrindType(achievementId) != AchievementGrindType::COLLECTION)
        return false;

    return StartGrind(achievementId);
}

bool AchievementGrinder::ExecuteReputationAchievement(uint32 achievementId)
{
    if (DetermineGrindType(achievementId) != AchievementGrindType::REPUTATION)
        return false;

    return StartGrind(achievementId);
}

std::vector<ExplorationTarget> AchievementGrinder::GetExplorationTargets(uint32 /*achievementId*/) const
{
    return _explorationTargets;
}

ExplorationTarget AchievementGrinder::GetNextExplorationTarget() const
{
    for (auto const& target : _explorationTargets)
    {
        if (!target.isDiscovered && _discoveredAreas.count(target.areaId) == 0)
        {
            return target;
        }
    }
    return ExplorationTarget();
}

bool AchievementGrinder::IsAreaDiscovered(uint32 areaId) const
{
    return _discoveredAreas.count(areaId) > 0;
}

bool AchievementGrinder::NavigateToExploration(ExplorationTarget const& target)
{
    return NavigateToPosition(target.centerPoint);
}

std::vector<KillTarget> AchievementGrinder::GetKillTargets(uint32 /*achievementId*/) const
{
    return _killTargets;
}

KillTarget AchievementGrinder::GetNextKillTarget() const
{
    for (auto const& target : _killTargets)
    {
        auto it = _killProgress.find(target.creatureEntry);
        uint32 currentKills = (it != _killProgress.end()) ? it->second : 0;

        if (currentKills < target.requiredKills)
        {
            KillTarget result = target;
            result.currentKills = currentKills;
            return result;
        }
    }
    return KillTarget();
}

bool AchievementGrinder::NavigateToKillTarget(KillTarget const& target)
{
    if (target.spawnLocations.empty())
        return false;

    // Find nearest spawn location
    if (!GetBot())
        return false;

    Position nearest = target.spawnLocations[0];
    float nearestDist = GetBot()->GetDistance(nearest);

    for (size_t i = 1; i < target.spawnLocations.size(); ++i)
    {
        float dist = GetBot()->GetDistance(target.spawnLocations[i]);
        if (dist < nearestDist)
        {
            nearestDist = dist;
            nearest = target.spawnLocations[i];
        }
    }

    return NavigateToPosition(nearest);
}

void AchievementGrinder::OnCreatureKilled(uint32 creatureEntry)
{
    if (!_currentSession.isActive || _currentSession.type != AchievementGrindType::KILL)
        return;

    // Update kill progress
    _killProgress[creatureEntry]++;
    _currentSession.killsThisSession++;
    _statistics.creaturesKilled.fetch_add(1, std::memory_order_relaxed);
    _globalStatistics.creaturesKilled.fetch_add(1, std::memory_order_relaxed);

    TC_LOG_DEBUG("module.playerbot.achievements",
        "AchievementGrinder: {} killed creature {} (total: {})",
        GetBot() ? GetBot()->GetName() : "unknown",
        creatureEntry, _killProgress[creatureEntry]);

    // Check if target is complete
    for (auto& target : _killTargets)
    {
        if (target.creatureEntry == creatureEntry)
        {
            auto it = _killProgress.find(creatureEntry);
            if (it != _killProgress.end() && it->second >= target.requiredKills)
            {
                TC_LOG_DEBUG("module.playerbot.achievements",
                    "AchievementGrinder: {} completed kill target {} ({}/{})",
                    GetBot() ? GetBot()->GetName() : "unknown",
                    creatureEntry, it->second, target.requiredKills);
            }
            break;
        }
    }
}

std::vector<uint32> AchievementGrinder::GetAchievementQuests(uint32 /*achievementId*/) const
{
    return _requiredQuests;
}

uint32 AchievementGrinder::GetNextQuestToComplete() const
{
    for (uint32 questId : _requiredQuests)
    {
        if (_completedQuests.count(questId) == 0)
        {
            return questId;
        }
    }
    return 0;
}

void AchievementGrinder::OnQuestCompleted(uint32 questId)
{
    if (!_currentSession.isActive)
        return;

    _completedQuests.insert(questId);
    _currentSession.questsCompletedThisSession++;
    _statistics.questsCompleted.fetch_add(1, std::memory_order_relaxed);
    _globalStatistics.questsCompleted.fetch_add(1, std::memory_order_relaxed);

    TC_LOG_DEBUG("module.playerbot.achievements",
        "AchievementGrinder: {} completed quest {} for achievement {}",
        GetBot() ? GetBot()->GetName() : "unknown",
        questId, _currentSession.achievementId);
}

uint32 AchievementGrinder::GetAchievementInstance(uint32 /*achievementId*/) const
{
    return _targetInstanceId;
}

bool AchievementGrinder::CanEnterInstance(uint32 mapId) const
{
    if (!GetBot())
        return false;

    // Check level requirement
    MapEntry const* mapEntry = sMapStore.LookupEntry(mapId);
    if (!mapEntry)
        return false;

    // Would check various requirements (level, attunement, lockout, etc.)
    return true;
}

bool AchievementGrinder::StartInstanceRun(uint32 achievementId)
{
    uint32 mapId = GetAchievementInstance(achievementId);
    if (mapId == 0)
        return false;

    if (!CanEnterInstance(mapId))
        return false;

    // Instance entry handled by InstanceManager
    _currentSession.phase = GrindPhase::NAVIGATING;
    return true;
}

void AchievementGrinder::SetGrindTypeEnabled(AchievementGrindType type, bool enabled)
{
    if (enabled)
        _enabledTypes.insert(type);
    else
        _enabledTypes.erase(type);
}

bool AchievementGrinder::IsGrindTypeEnabled(AchievementGrindType type) const
{
    return _enabledTypes.count(type) > 0;
}

AchievementGrindType AchievementGrinder::DetermineGrindType(uint32 achievementId) const
{
    // Check cached type
    auto it = _achievementTypes.find(achievementId);
    if (it != _achievementTypes.end())
    {
        return it->second;
    }

    // Would analyze achievement criteria from DB2 to determine type
    // For now, return MISC as fallback
    return AchievementGrindType::MISC;
}

bool AchievementGrinder::NavigateToPosition(Position const& pos)
{
    // Navigation handled by MovementManager/NavigationManager
    _currentSession.targetPosition = pos;
    _currentSession.isNavigating = true;
    return true;
}

AchievementManager* AchievementGrinder::GetAchievementManager() const
{
    // Would retrieve from GameSystemsManager
    // For now, return nullptr - actual integration done at higher level
    return nullptr;
}

void AchievementGrinder::LoadAchievementData(uint32 achievementId)
{
    // Clear previous data
    _explorationTargets.clear();
    _killTargets.clear();
    _requiredQuests.clear();
    _discoveredAreas.clear();
    _completedQuests.clear();
    _killProgress.clear();
    _targetInstanceId = 0;

    // Determine type and load appropriate data
    AchievementGrindType type = DetermineGrindType(achievementId);

    switch (type)
    {
        case AchievementGrindType::EXPLORATION:
            // Load exploration areas from achievement criteria
            // Would query CriteriaTree for EXPLORE_AREA criteria
            break;

        case AchievementGrindType::KILL:
            // Load kill targets from achievement criteria
            // Would query CriteriaTree for KILL_CREATURE criteria
            break;

        case AchievementGrindType::QUEST:
            // Load required quests from achievement criteria
            // Would query CriteriaTree for COMPLETE_QUEST criteria
            break;

        case AchievementGrindType::DUNGEON:
        case AchievementGrindType::RAID:
            // Load instance info from achievement criteria
            // Would query CriteriaTree for instance-related criteria
            break;

        default:
            break;
    }

    TC_LOG_DEBUG("module.playerbot.achievements",
        "AchievementGrinder: Loaded data for achievement {} (type: {}, {} exploration, {} kill, {} quest targets)",
        achievementId, static_cast<uint8>(type),
        _explorationTargets.size(), _killTargets.size(), _requiredQuests.size());
}

void AchievementGrinder::NotifyCallback(uint32 achievementId, bool completed)
{
    if (_callback)
    {
        _callback(achievementId, completed);
    }
}

} // namespace Playerbot
