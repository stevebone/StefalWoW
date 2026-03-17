/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ObjectiveTracker.h"
#include "QuestCompletion.h"
#include "Player.h"
#include "QuestDef.h"
#include "Unit.h"
#include "Creature.h"
#include "GameObject.h"
#include "ObjectMgr.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Util.h"
#include "World.h"
#include "WorldSession.h"
#include "Map.h"
#include "Group.h"
#include "Session/BotSession.h"
#include "../AI/BotAI.h"  // P1 FIX: Required for unique_ptr<BotAI> in BotSession.h
#include "Core/Services/BotNpcLocationService.h"
#include "Core/Threading/SafeGridOperations.h"  // SEH-protected grid operations
#include <algorithm>
#include <cmath>
#include "GameTime.h"

namespace Playerbot
{

ObjectiveTracker::ObjectiveTracker(Player* bot) : _bot(bot)
{
    if (!_bot) TC_LOG_ERROR("playerbot.quest", "ObjectiveTracker: null bot!");
}

ObjectiveTracker::~ObjectiveTracker() {}

void ObjectiveTracker::StartTrackingObjective(Player* bot, const QuestObjectiveData& objective)
{
    if (!bot)
        return;

    uint32 botGuid = bot->GetGUID().GetCounter();
    ObjectiveState state(objective.questId, objective.objectiveIndex);
    state.status = ObjectiveStatus::IN_PROGRESS;
    state.requiredProgress = objective.requiredCount;
    state.lastUpdateTime = GameTime::GetGameTimeMS();  // CRITICAL: Initialize to prevent immediate cleanup
    state.timeStarted = GameTime::GetGameTimeMS();

    // CRITICAL FIX: Set objective position to actual quest target location, NOT bot's current position!
    // Get the spawn location of the quest target (creature/object/etc)
    Position targetPosition = FindObjectiveTargetLocation(bot, objective);

    // If we found a valid target location, use it. Otherwise, use bot's position as fallback.
    if (targetPosition.GetExactDist2d(0.0f, 0.0f) > 0.1f)
    {
        state.lastKnownPosition = targetPosition;
        TC_LOG_ERROR("module.playerbot.quest", "🎯 StartTrackingObjective: Bot {} - Using target spawn location ({:.1f}, {:.1f}, {:.1f}) for Quest {} Objective {}",
                    bot->GetName(),
                    targetPosition.GetPositionX(), targetPosition.GetPositionY(), targetPosition.GetPositionZ(),
                    objective.questId, objective.objectiveIndex);
    }
    else
    {
        state.lastKnownPosition = bot->GetPosition();
        TC_LOG_ERROR("module.playerbot.quest", "⚠️ StartTrackingObjective: Bot {} - NO target location found, using bot position ({:.1f}, {:.1f}, {:.1f}) for Quest {} Objective {}",
                    bot->GetName(),
                    bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(),
                    objective.questId, objective.objectiveIndex);
    }

    // DEADLOCK FIX: Initialize target detection BEFORE locking _trackingMutex
    // This prevents circular deadlock:
    //   CleanupInactiveTracking: _trackingMutex → _targetMutex
    //   StartTrackingObjective: _targetMutex → _trackingMutex (DEADLOCK!)
    // By calling DetectObjectiveTargets() before the lock, we ensure consistent lock ordering.
    std::vector<uint32> targets = DetectObjectiveTargets(bot, objective);
    state.targetIds = targets;

    size_t objectiveCount = 0;
    {
        // Thread safety: lock before modifying _botObjectiveStates
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        _botObjectiveStates[botGuid].push_back(state);
        objectiveCount = _botObjectiveStates[botGuid].size();
    }

    TC_LOG_ERROR("module.playerbot.quest", "✅ StartTrackingObjective: Bot {} - Quest {} Objective {} registered (Total objectives: {})",
                bot->GetName(), objective.questId, objective.objectiveIndex, objectiveCount);

    // Update analytics
    _globalAnalytics.objectivesStarted++;
}

void ObjectiveTracker::StopTrackingObjective(Player* bot, uint32 questId, uint32 objectiveIndex)
{
    if (!bot)
        return;

    uint32 botGuid = bot->GetGUID().GetCounter();

    // Thread safety: lock before modifying _botObjectiveStates
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    auto statesIt = _botObjectiveStates.find(botGuid);
    if (statesIt == _botObjectiveStates.end())
        return;

    auto& states = statesIt->second;
    states.erase(
        std::remove_if(states.begin(), states.end(),
            [questId, objectiveIndex](const ObjectiveState& state) {
                return state.questId == questId && state.objectiveIndex == objectiveIndex;
            }),
        states.end()
    );

    TC_LOG_DEBUG("playerbot.objectives", "Stopped tracking objective {} for quest {} for bot {}",
                objectiveIndex, questId, bot->GetName());
}

void ObjectiveTracker::UpdateObjectiveTracking(Player* bot, uint32 diff)
{
    if (!bot)
        return;

    uint32 botGuid = bot->GetGUID().GetCounter();

    // Thread safety: lock before accessing _botObjectiveStates
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    auto statesIt = _botObjectiveStates.find(botGuid);
    if (statesIt == _botObjectiveStates.end())
        return;

    for (auto& state : statesIt->second)
    {
        // CRITICAL FIX: Skip objectives for quests that are already COMPLETE or REWARDED
        // These should not be tracked/updated - the bot should turn in the quest instead
        QuestStatus questStatus = bot->GetQuestStatus(state.questId);
        if (questStatus == QUEST_STATUS_COMPLETE || questStatus == QUEST_STATUS_REWARDED)
        {
            // Quest is complete - don't flag it as "stuck", just skip tracking
            continue;
        }

        // Only track quests that are still INCOMPLETE
        if (questStatus != QUEST_STATUS_INCOMPLETE)
            continue;

        UpdateObjectiveProgress(bot, state);

        // Check for stuck state (check directly on state to avoid recursive lock)
        uint32 currentTime = GameTime::GetGameTimeMS();
        bool isStalled = (currentTime - state.lastUpdateTime > STUCK_DETECTION_TIME) &&
                        (state.completionVelocity < STALLED_PROGRESS_THRESHOLD);
        if (isStalled)
        {
            HandleStuckObjective(bot, state);
        }

        // Update velocity calculation
        CalculateObjectiveVelocity(state);

        // Estimate completion time
        EstimateCompletionTime(state);
    }
}

void ObjectiveTracker::RefreshObjectiveStates(Player* bot)
{
    if (!bot)
        return;

    uint32 botGuid = bot->GetGUID().GetCounter();

    // Thread safety: lock before accessing _botObjectiveStates
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    auto statesIt = _botObjectiveStates.find(botGuid);
    if (statesIt == _botObjectiveStates.end())
        return;

    // Refresh all objective states for this bot
    for (auto& state : statesIt->second)
    {
        RefreshObjectiveState(bot, state);
    }
}

void ObjectiveTracker::MonitorObjectiveProgress(Player* bot, uint32 questId, uint32 objectiveIndex)
{
    if (!bot)
        return;

    ObjectiveState state = GetObjectiveState(bot, questId, objectiveIndex);
    if (state.questId == 0)
        return;

    // Monitor progress and detect issues
    if (state.completionVelocity < MIN_VELOCITY_THRESHOLD)
    {
        TC_LOG_WARN("playerbot.objectives", "Low velocity detected for objective {} in quest {} for bot {}",
                   objectiveIndex, questId, bot->GetName());
    }

    // Update progress metrics
    UpdateProgressMetrics(bot, ConvertToQuestObjectiveData(state));
}

void ObjectiveTracker::UpdateProgressMetrics(Player* bot, const QuestObjectiveData& objective)
{
    if (!bot)
        return;

    uint32 botGuid = bot->GetGUID().GetCounter();

    // Thread safety: lock before accessing _botAnalytics
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    // Update bot-specific analytics
    auto& analytics = _botAnalytics[botGuid];
    analytics.lastAnalyticsUpdate = GameTime::GetGameTimeMS();

    // Check objective completion
    if (objective.requiredCount <= objective.currentCount)
    {
        analytics.objectivesCompleted++;
        _globalAnalytics.objectivesCompleted++;
    }
}

bool ObjectiveTracker::HasProgressStalled(Player* bot, uint32 questId, uint32 objectiveIndex)
{
    if (!bot)
        return false;

    ObjectiveState state = GetObjectiveState(bot, questId, objectiveIndex);
    if (state.questId == 0)
        return false;

    uint32 currentTime = GameTime::GetGameTimeMS();

    // Check if no progress has been made recently
    return (currentTime - state.lastUpdateTime > STUCK_DETECTION_TIME) &&
           (state.completionVelocity < STALLED_PROGRESS_THRESHOLD);
}

float ObjectiveTracker::CalculateObjectiveVelocity(Player* bot, uint32 questId, uint32 objectiveIndex)
{
    ObjectiveState state = GetObjectiveState(bot, questId, objectiveIndex);
    if (state.questId == 0)
        return 0.0f;

    return state.completionVelocity;
}

std::vector<uint32> ObjectiveTracker::DetectObjectiveTargets(Player* bot, const QuestObjectiveData& objective)
{
    std::vector<uint32> targets;

    if (!bot)
        return targets;

    const Quest* quest = sObjectMgr->GetQuestTemplate(objective.questId);
    if (!quest)
        return targets;

    // Scan for different types of targets based on objective type
    switch (objective.type)
    {
        case QuestObjectiveType::KILL_CREATURE:
            targets = ScanForKillTargets(bot, objective.targetId, 100.0f);
            break;
        case QuestObjectiveType::COLLECT_ITEM:
            targets = ScanForCollectibles(bot, objective.targetId, 50.0f);
            break;
        case QuestObjectiveType::USE_GAMEOBJECT:
            targets = ScanForGameObjects(bot, objective.targetId, 50.0f);
            break;
        default:
            break;
    }

    return targets;
}

std::vector<uint32> ObjectiveTracker::ScanForKillTargets(Player* bot, uint32 creatureId, float radius)
{
    std::vector<uint32> targets;

    if (!bot)
        return targets;

    // THREAD-SAFE: Use SafeGridOperations with SEH protection to catch access violations
    std::list<Creature*> nearbyCreatures;
    if (!SafeGridOperations::GetCreatureListSafe(bot, nearbyCreatures, creatureId, radius))
        return targets;

    for (Creature* creature : nearbyCreatures)
    {
        if (creature && creature->IsAlive())
        {
            targets.push_back(creature->GetGUID().GetCounter());

            // Update target tracking
            TrackTargetAvailability(bot, creature->GetEntry(), creature->GetEntry());
        }
    }

    _globalAnalytics.targetsFound += static_cast<uint32>(targets.size());
    return targets;
}

std::vector<uint32> ObjectiveTracker::ScanForCollectibles(Player* bot, uint32 itemId, float radius)
{
    std::vector<uint32> targets;

    if (!bot)
        return targets;

    // Search for collectible items or objects that drop the item
    // This would require knowledge of what creatures drop specific items

    // THREAD-SAFE: Use SafeGridOperations with SEH protection to catch access violations
    std::list<GameObject*> nearbyObjects;
    if (!SafeGridOperations::GetGameObjectListSafe(bot, nearbyObjects, 0, radius))
        return targets;

    for (GameObject* object : nearbyObjects)
    {
        if (object && object->getLootState() == GO_READY)
        {
            // Check if this object can provide the needed item
            targets.push_back(object->GetGUID().GetCounter());
        }
    }

    return targets;
}

std::vector<uint32> ObjectiveTracker::ScanForGameObjects(Player* bot, uint32 objectId, float radius)
{
    std::vector<uint32> targets;

    if (!bot)
        return targets;

    TC_LOG_TRACE("module.playerbot.quest", "ScanForGameObjects: Bot {} searching for entry {} within {:.1f}yd radius",
                 bot->GetName(), objectId, radius);

    // THREAD-SAFE: Use SafeGridOperations with SEH protection to catch access violations
    std::list<GameObject*> nearbyObjects;
    if (!SafeGridOperations::GetGameObjectListSafe(bot, nearbyObjects, objectId, radius))
        return targets;

    TC_LOG_ERROR("module.playerbot.quest", "  GetGameObjectListWithEntryInGrid returned {} objects",
                 nearbyObjects.size());

    uint32 foundCount = 0;
    for (GameObject* object : nearbyObjects)
    {
        if (object)
        {
            foundCount++;
            TC_LOG_ERROR("module.playerbot.quest", "  ✅ Found GO {}: entry={}, pos=({:.1f}, {:.1f}, {:.1f}), distance={:.1f}yd, spawned={}, state={}",
                         foundCount, object->GetEntry(),
                         object->GetPositionX(), object->GetPositionY(), object->GetPositionZ(),
                         bot->GetExactDist(object),
                         object->isSpawned(), object->GetGoState());
            targets.push_back(object->GetGUID().GetCounter());
        }
    }

    // If no GameObjects found, try searching for Creatures (some quests use creature triggers)
    if (targets.empty())
    {
        TC_LOG_TRACE("module.playerbot.quest", "ScanForGameObjects: No GameObjects found, searching for Creatures with entry {}...", objectId);

        // THREAD-SAFE: Use SafeGridOperations with SEH protection to catch access violations
        std::list<Creature*> nearbyCreatures;
        if (!SafeGridOperations::GetCreatureListSafe(bot, nearbyCreatures, objectId, radius))
            return targets;

        TC_LOG_TRACE("module.playerbot.quest", "ScanForGameObjects: GetCreatureListSafe returned {} creatures",
                     nearbyCreatures.size());

        for (Creature* creature : nearbyCreatures)
        {
            if (creature && creature->IsAlive())
            {
                foundCount++;
                TC_LOG_ERROR("module.playerbot.quest", "  ✅ Found Creature {}: entry={}, pos=({:.1f}, {:.1f}, {:.1f}), distance={:.1f}yd, name={}",
                             foundCount, creature->GetEntry(),
                             creature->GetPositionX(), creature->GetPositionY(), creature->GetPositionZ(),
                             bot->GetExactDist(creature),
                             creature->GetName());
                targets.push_back(creature->GetGUID().GetCounter());
            }
        }
    }

    TC_LOG_ERROR("module.playerbot.quest", "  📊 ScanForGameObjects result: {} valid targets found (returned {} GUIDs)",
                 foundCount, targets.size());

    return targets;
}

ObjectiveState ObjectiveTracker::GetObjectiveState(Player* bot, uint32 questId, uint32 objectiveIndex)
{
    if (!bot)
        return ObjectiveState(0, 0);

    uint32 botGuid = bot->GetGUID().GetCounter();

    // Thread safety: lock before accessing _botObjectiveStates
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    auto statesIt = _botObjectiveStates.find(botGuid);
    if (statesIt == _botObjectiveStates.end())
        return ObjectiveState(0, 0);

    for (const auto& state : statesIt->second)
    {
        if (state.questId == questId && state.objectiveIndex == objectiveIndex)
            return state;
    }

    return ObjectiveState(0, 0);
}

void ObjectiveTracker::UpdateObjectiveState(Player* bot, const ObjectiveState& state)
{
    if (!bot)
        return;

    uint32 botGuid = bot->GetGUID().GetCounter();

    // Thread safety: lock before accessing _botObjectiveStates
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    auto statesIt = _botObjectiveStates.find(botGuid);
    if (statesIt == _botObjectiveStates.end())
        return;

    for (auto& existingState : statesIt->second)
    {
        if (existingState.questId == state.questId &&
            existingState.objectiveIndex == state.objectiveIndex)
        {
            existingState = state;
            break;
        }
    }
}

std::vector<ObjectiveState> ObjectiveTracker::GetActiveObjectives(Player* bot)
{
    std::vector<ObjectiveState> activeObjectives;

    if (!bot)
        return activeObjectives;

    uint32 botGuid = bot->GetGUID().GetCounter();

    // Thread safety: lock before accessing _botObjectiveStates
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    auto statesIt = _botObjectiveStates.find(botGuid);
    if (statesIt == _botObjectiveStates.end())
        return activeObjectives;

    // CRITICAL FIX: Only return objectives for quests that are still INCOMPLETE
    // Quests with COMPLETE or REWARDED status should not have objectives worked on
    for (const auto& state : statesIt->second)
    {
        QuestStatus questStatus = bot->GetQuestStatus(state.questId);

        // Skip quests with COMPLETE or REWARDED status
        if (questStatus == QUEST_STATUS_COMPLETE || questStatus == QUEST_STATUS_REWARDED)
        {
            TC_LOG_DEBUG("module.playerbot.quest",
                "GetActiveObjectives: Skipping quest {} objective {} for bot {} - quest status is {} (should turn in/already rewarded)",
                state.questId, state.objectiveIndex, bot->GetName(), static_cast<uint32>(questStatus));
            continue;
        }

        // Only include objectives for INCOMPLETE quests
        if (questStatus != QUEST_STATUS_INCOMPLETE)
            continue;

        // CRITICAL FIX 2: Also check if this specific objective is already complete
        // This handles the case where quest status is INCOMPLETE but all objectives are done
        // (e.g., quest 26391 with 8/8 progress but status still INCOMPLETE)
        Quest const* quest = sObjectMgr->GetQuestTemplate(state.questId);
        if (quest && state.objectiveIndex < quest->Objectives.size())
        {
            QuestObjective const& objective = quest->Objectives[state.objectiveIndex];
            uint32 currentProgress = bot->GetQuestObjectiveData(state.questId, objective.StorageIndex);
            uint32 requiredAmount = static_cast<uint32>(objective.Amount);

            // CRITICAL FIX 3: For AREATRIGGER objectives (exploration quests), Amount is typically 0
            // but that doesn't mean it's complete - the bot must physically enter the area
            // Check quest status for these special objective types instead of progress
            bool isAreaTriggerObjective = (objective.Type == QUEST_OBJECTIVE_AREATRIGGER ||
                                           objective.Type == QUEST_OBJECTIVE_AREA_TRIGGER_ENTER ||
                                           objective.Type == QUEST_OBJECTIVE_AREA_TRIGGER_EXIT);

            if (isAreaTriggerObjective && requiredAmount == 0)
            {
                // For exploration objectives with Amount=0, check quest status instead
                // Quest status INCOMPLETE means the area hasn't been visited yet
                QuestStatus questStatus = bot->GetQuestStatus(state.questId);
                if (questStatus != QUEST_STATUS_COMPLETE)
                {
                    TC_LOG_DEBUG("module.playerbot.quest",
                        "GetActiveObjectives: AREATRIGGER objective for quest {} (bot {}) - status {} != COMPLETE, keeping as active",
                        state.questId, bot->GetName(), static_cast<int>(questStatus));
                    // Don't skip - this exploration objective is NOT complete
                }
                else
                {
                    TC_LOG_DEBUG("module.playerbot.quest",
                        "GetActiveObjectives: Skipping quest {} AREATRIGGER objective {} for bot {} - quest status is COMPLETE",
                        state.questId, state.objectiveIndex, bot->GetName());
                    continue;  // Quest is complete, skip this objective
                }
            }
            else if (currentProgress >= requiredAmount)
            {
                TC_LOG_DEBUG("module.playerbot.quest",
                    "GetActiveObjectives: Skipping quest {} objective {} for bot {} - objective already COMPLETE ({}/{})",
                    state.questId, state.objectiveIndex, bot->GetName(), currentProgress, requiredAmount);
                continue;  // This objective is done, don't include it
            }
        }

        activeObjectives.push_back(state);
    }

    return activeObjectives;
}

std::vector<ObjectivePriority> ObjectiveTracker::CalculateObjectivePriorities(Player* bot)
{
    std::vector<ObjectivePriority> priorities;

    if (!bot)
        return priorities;

    std::vector<ObjectiveState> activeObjectives = GetActiveObjectives(bot);
    TC_LOG_ERROR("module.playerbot.quest", "🔢 CalculateObjectivePriorities: Bot {} has {} active objectives",
                bot->GetName(), activeObjectives.size());

    for (const auto& state : activeObjectives)
    {
        ObjectivePriority priority(state.questId, state.objectiveIndex);

        // Calculate priority factors
        priority.urgencyFactor = CalculateUrgencyFactor(bot, state);
        priority.difficultyFactor = CalculateDifficultyFactor(bot, state);
        priority.efficiencyFactor = CalculateEfficiencyFactor(bot, state);
        priority.proximityFactor = CalculateProximityFactor(bot, state);

        // Calculate overall priority score
        priority.priorityScore = (priority.urgencyFactor * 0.3f) +
                               (priority.difficultyFactor * 0.2f) +
                               (priority.efficiencyFactor * 0.3f) +
                               (priority.proximityFactor * 0.2f);

        TC_LOG_ERROR("module.playerbot.quest", "  📊 Quest {} Objective {}: urgency={:.2f}, difficulty={:.2f}, efficiency={:.2f}, proximity={:.2f}, TOTAL={:.2f}",
                    state.questId, state.objectiveIndex,
                    priority.urgencyFactor, priority.difficultyFactor, priority.efficiencyFactor, priority.proximityFactor,
                    priority.priorityScore);

        priorities.push_back(priority);
    }

    return priorities;
}

ObjectivePriority ObjectiveTracker::GetHighestPriorityObjective(Player* bot)
{
    std::vector<ObjectivePriority> priorities = CalculateObjectivePriorities(bot);

    if (priorities.empty())
        return ObjectivePriority(0, 0);

    std::sort(priorities.begin(), priorities.end(),
        [](const ObjectivePriority& a, const ObjectivePriority& b) {
            return a.priorityScore > b.priorityScore;
        });

    return priorities[0];
}

void ObjectiveTracker::OptimizeObjectiveSequence(Player* bot, std::vector<ObjectivePriority>& priorities)
{
    if (!bot || priorities.empty())
        return;

    // Sort by priority score
    std::sort(priorities.begin(), priorities.end(),
        [](const ObjectivePriority& a, const ObjectivePriority& b) {
            return a.priorityScore > b.priorityScore;
        });

    // Additional optimization based on location, prerequisites, etc.
    // This would involve more complex scheduling algorithms
}

void ObjectiveTracker::TrackTargetAvailability(Player* bot, uint32 questId, uint32 targetId)
{
    if (!bot)
        return;

    // Thread safety: lock before accessing _targetTracking
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    auto& trackingData = _targetTracking[targetId];
    trackingData.targetId = targetId;
    trackingData.lastSeenTime = GameTime::GetGameTimeMS();
    trackingData.isAvailable = true;

    // Add current location to known locations
    trackingData.knownLocations.push_back(bot->GetPosition());

    // Limit known locations to prevent memory bloat
    if (trackingData.knownLocations.size() > 10)
    {
        trackingData.knownLocations.erase(trackingData.knownLocations.begin());
    }
}

bool ObjectiveTracker::IsTargetAvailable(uint32 targetId, const Position& location, float radius)
{
    // Thread safety: lock before accessing _targetTracking
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    auto trackingIt = _targetTracking.find(targetId);
    if (trackingIt == _targetTracking.end())
        return false;

    const TargetTrackingData& data = trackingIt->second;

    // Check if target was recently seen
    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime - data.lastSeenTime > TARGET_CACHE_DURATION)
        return false;
    // Check if any known location is within radius
    for (const Position& knownPos : data.knownLocations)
    {
        if (location.GetExactDist(&knownPos) <= radius)
            return true;
    }

    return false;
}

uint32 ObjectiveTracker::GetTargetRespawnTime(uint32 targetId)
{
    // Get respawn time for the target
    // This would require creature template lookup
    return 60000; // Default 1 minute respawn
}

Position ObjectiveTracker::GetOptimalTargetLocation(uint32 targetId, const Position& playerPosition)
{
    // Thread safety: lock before accessing _targetTracking
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    auto trackingIt = _targetTracking.find(targetId);
    if (trackingIt == _targetTracking.end())
        return playerPosition;

    const TargetTrackingData& data = trackingIt->second;

    if (data.knownLocations.empty())
        return playerPosition;

    // Find closest known location
    Position closest = data.knownLocations[0];
    float closestDist = playerPosition.GetExactDist(&closest);

    for (const Position& pos : data.knownLocations)
    {
        float dist = playerPosition.GetExactDist(&pos);
        if (dist < closestDist)
        {
            closest = pos;
            closestDist = dist;
        }
    }
    return closest;
}

void ObjectiveTracker::MonitorTargetCompetition(Player* bot, uint32 targetId)
{
    if (!bot)
        return;

    // Thread safety: lock before accessing _targetTracking
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    auto& trackingData = _targetTracking[targetId];

    // THREAD-SAFE: Use SafeGridOperations with SEH protection to catch access violations
    std::list<Player*> nearbyPlayers;
    if (!SafeGridOperations::GetPlayerListSafe(bot, nearbyPlayers, 50.0f))
        return;

    uint32 competitionLevel = 0;
    for (Player* player : nearbyPlayers)
    {
        if (player && player != bot)
        {
            // Check if player is working on same objective
            // This would require quest state checking
            competitionLevel++;
        }
    }

    trackingData.competitionLevel = competitionLevel;
}

bool ObjectiveTracker::IsTargetContested(uint32 targetId, float radius)
{
    // Thread safety: lock before accessing _targetTracking
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    auto trackingIt = _targetTracking.find(targetId);
    if (trackingIt == _targetTracking.end())
        return false;

    return trackingIt->second.competitionLevel >= COMPETITION_THRESHOLD;
}

void ObjectiveTracker::HandleTargetCompetition(Player* bot, uint32 targetId)
{
    if (!bot)
        return;

    // Find alternative locations for this target
    std::vector<Position> alternatives = FindAlternativeTargetLocations(targetId, bot->GetPosition());
    if (!alternatives.empty())
    {
        // Move to less contested area
        Position alternative = alternatives[0];
        TC_LOG_DEBUG("playerbot.objectives", "Bot {} moving to alternative location for target {} due to competition",
                    bot->GetName(), targetId);
    }
}

std::vector<Position> ObjectiveTracker::FindAlternativeTargetLocations(uint32 targetId, const Position& currentLocation)
{
    std::vector<Position> alternatives;

    // Thread safety: lock before accessing _targetTracking
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    auto trackingIt = _targetTracking.find(targetId);
    if (trackingIt == _targetTracking.end())
        return alternatives;

    const TargetTrackingData& data = trackingIt->second;

    // Return known locations sorted by distance (farthest first to avoid competition)
    alternatives = data.knownLocations;

    std::sort(alternatives.begin(), alternatives.end(),
        [&currentLocation](const Position& a, const Position& b) {
            return currentLocation.GetExactDist(&a) > currentLocation.GetExactDist(&b);
        });

    return alternatives;
}

void ObjectiveTracker::CoordinateGroupObjectives(Group* group, uint32 questId)
{
    if (!group)
        return;

    // Coordinate objective completion among group members
    for (GroupReference const& itr : group->GetMembers())
    {
        Player* member = itr.GetSource();
        if (member && dynamic_cast<BotSession*>(member->GetSession()))
        {
            // Assign different objectives to different group members for efficiency
            uint32 assignedObjective = AssignObjectiveToGroupMember(group, member, questId);
            if (assignedObjective != 0)
            {
                TC_LOG_DEBUG("playerbot.objectives", "Assigned objective {} to group member {}",
                           assignedObjective, member->GetName());
            }
        }
    }
}

void ObjectiveTracker::DistributeObjectiveTargets(Group* group, uint32 questId, uint32 objectiveIndex)
{
    if (!group)
        return;
    // Distribute targets among group members to avoid competition
    std::vector<Player*> botMembers;
    for (GroupReference const& itr : group->GetMembers())
    {
        Player* member = itr.GetSource();
        if (member && dynamic_cast<BotSession*>(member->GetSession()))
        {
            botMembers.push_back(member);
        }
    }

    if (botMembers.empty())
        return;

    // Assign different areas or targets to each bot
    for (size_t i = 0; i < botMembers.size(); ++i)
    {
        // This would involve more sophisticated target distribution logic
        AssignSpecificTargetToBot(botMembers[i], questId, objectiveIndex, static_cast<uint32>(i));
    }
}

void ObjectiveTracker::SynchronizeObjectiveProgress(Group* group, uint32 questId)
{
    if (!group)
        return;

    // Synchronize progress updates among group members
    uint32 groupId = group->GetDbStoreId();

    _groupObjectiveSyncTime[groupId] = GameTime::GetGameTimeMS();

    // Share progress information among group members
    for (GroupReference const& itr : group->GetMembers())
    {
        Player* member = itr.GetSource();
        if (member && dynamic_cast<BotSession*>(member->GetSession()))
        {
            RefreshObjectiveStates(member);
        }
    }
}

void ObjectiveTracker::HandleObjectiveConflicts(Group* group, uint32 questId, uint32 objectiveIndex)
{
    if (!group)
        return;

    // Resolve conflicts when multiple group members compete for same objectives
    ResolveObjectiveConflicts(group, questId, objectiveIndex);
}

const ObjectiveAnalytics& ObjectiveTracker::GetBotObjectiveAnalytics(uint32 botGuid)
{
    // Thread safety: lock before accessing _botAnalytics
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    auto it = _botAnalytics.find(botGuid);
    if (it != _botAnalytics.end())
        return it->second;

    // Create a new analytics entry if it doesn't exist
    _botAnalytics[botGuid].Reset();
    return _botAnalytics[botGuid];
}

const ObjectiveAnalytics& ObjectiveTracker::GetGlobalObjectiveAnalytics()
{
    return _globalAnalytics;
}

void ObjectiveTracker::EnablePredictiveTracking(Player* bot, bool enable)
{
    if (!bot)
        return;

    uint32 botGuid = bot->GetGUID().GetCounter();
    TC_LOG_DEBUG("playerbot.objectives", "Predictive tracking for bot {} set to {}", botGuid, enable);
}

void ObjectiveTracker::PredictObjectiveCompletion(Player* bot, uint32 questId, uint32 objectiveIndex)
{
    if (!bot)
        return;

    ObjectiveState state = GetObjectiveState(bot, questId, objectiveIndex);
    if (state.questId == 0)
        return;

    // Predict completion time based on current velocity
    if (state.completionVelocity > 0.0f)
    {
        float remainingProgress = float(state.requiredProgress - state.currentProgress);
        uint32 estimatedTime = static_cast<uint32>(remainingProgress / state.completionVelocity * 1000.0f);
        state.estimatedTimeRemaining = estimatedTime;

        UpdateObjectiveState(bot, state);
    }
}

void ObjectiveTracker::AdaptTrackingStrategy(Player* bot, const ObjectiveState& state)
{
    if (!bot)
        return;

    // Adapt tracking strategy based on objective performance
    if (state.failureCount > 3)
    {
        TC_LOG_INFO("playerbot.objectives", "Adapting tracking strategy for bot {} due to repeated failures",
                   bot->GetName());

        // Implement adaptive strategy changes
        // This could involve changing scan frequency, target selection, etc.
    }
}

void ObjectiveTracker::OptimizeTrackingPerformance(Player* bot)
{
    if (!bot)
        return;

    uint32 botGuid = bot->GetGUID().GetCounter();
    // Optimize tracking performance for this bot
    // Reduce scan frequency for low-priority objectives
    // Increase focus on high-priority objectives

    TC_LOG_DEBUG("playerbot.objectives", "Optimizing tracking performance for bot {}", bot->GetName());
}

void ObjectiveTracker::DetectTrackingErrors(Player* bot)
{
    if (!bot)
        return;

    std::vector<ObjectiveState> activeObjectives = GetActiveObjectives(bot);
    for (const auto& state : activeObjectives)
    {
        if (!ValidateObjectiveState(bot, state))
        {
            HandleTrackingFailure(bot, state.questId, state.objectiveIndex, "Invalid objective state");
        }
    }
}

void ObjectiveTracker::HandleTrackingFailure(Player* bot, uint32 questId, uint32 objectiveIndex, const std::string& error)
{
    if (!bot)
        return;

    TC_LOG_WARN("playerbot.objectives", "Tracking failure for bot {}: Quest {} Objective {} - {}",
               bot->GetName(), questId, objectiveIndex, error);

    // Update failure metrics
    uint32 botGuid = bot->GetGUID().GetCounter();
    UpdateTrackingAnalytics(botGuid, GetObjectiveState(bot, questId, objectiveIndex), false);

    // Attempt recovery
    RecoverTrackingState(bot, questId);
}

void ObjectiveTracker::RecoverTrackingState(Player* bot, uint32 questId)
{
    if (!bot)
        return;

    // Attempt to recover from tracking failures
    RefreshObjectiveStates(bot);

    TC_LOG_INFO("playerbot.objectives", "Attempting to recover tracking state for bot {} quest {}",
               bot->GetName(), questId);
}

void ObjectiveTracker::ValidateObjectiveConsistency(Player* bot)
{
    if (!bot)
        return;

    std::vector<ObjectiveState> activeObjectives = GetActiveObjectives(bot);
    for (const auto& state : activeObjectives)
    {
        if (!ValidateObjectiveState(bot, state))
        {
            TC_LOG_WARN("playerbot.objectives", "Inconsistent objective state detected for bot {}",
                       bot->GetName());
        }
    }
}

void ObjectiveTracker::SetTrackingPrecision(uint32 botGuid, float precision)
{
    precision = std::clamp(precision, 0.0f, 1.0f);
    TC_LOG_DEBUG("playerbot.objectives", "Set tracking precision for bot {} to {}", botGuid, precision);
}

void ObjectiveTracker::SetUpdateFrequency(uint32 botGuid, uint32 frequencyMs)
{
    TC_LOG_DEBUG("playerbot.objectives", "Set update frequency for bot {} to {} ms", botGuid, frequencyMs);
}

void ObjectiveTracker::EnableAdvancedFeatures(uint32 botGuid, bool enable)
{
    TC_LOG_DEBUG("playerbot.objectives", "Advanced features for bot {} set to {}", botGuid, enable);
}

void ObjectiveTracker::UpdateObjectiveProgress(Player* bot, ObjectiveState& state)
{
    if (!bot)
        return;

    const Quest* quest = sObjectMgr->GetQuestTemplate(state.questId);
    if (!quest)
        return;

    // Update progress for this objective
    uint32 currentProgress = GetCurrentObjectiveProgress(bot, quest, state.objectiveIndex);

    if (currentProgress != state.currentProgress)
    {
        state.currentProgress = currentProgress;
        state.lastUpdateTime = GameTime::GetGameTimeMS();

        // Recalculate velocity
        CalculateObjectiveVelocity(state);
    }
}

void ObjectiveTracker::CalculateObjectiveVelocity(ObjectiveState& state)
{
    uint32 currentTime = GameTime::GetGameTimeMS();
    uint32 timeDiff = currentTime - state.timeStarted;

    if (timeDiff > 0)
    {
        state.completionVelocity = float(state.currentProgress) / (float(timeDiff) / 1000.0f);
    }
}

void ObjectiveTracker::EstimateCompletionTime(ObjectiveState& state)
{
    if (state.completionVelocity > 0.0f)
    {
        float remainingProgress = float(state.requiredProgress - state.currentProgress);
        state.estimatedTimeRemaining = static_cast<uint32>(remainingProgress / state.completionVelocity * 1000.0f);
    }
    else
    {
        state.estimatedTimeRemaining = OBJECTIVE_TIMEOUT; // Default timeout
    }
}

bool ObjectiveTracker::ValidateObjectiveState(Player* bot, const ObjectiveState& state)
{
    if (!bot)
        return false;

    // Validate that the objective state is consistent
    const Quest* quest = sObjectMgr->GetQuestTemplate(state.questId);
    if (!quest)
        return false;

    // Check if bot still has the quest
    if (bot->FindQuestSlot(state.questId) == MAX_QUEST_LOG_SIZE)
        return false;

    // Check if objective index is valid (quests typically have up to 10 objectives)
    if (state.objectiveIndex >= 10)
        return false;

    return true;
}

void ObjectiveTracker::OptimizeObjectiveExecution(Player* bot, ObjectiveState& state)
{
    if (!bot)
        return;

    // Optimize execution strategy for this objective
    if (state.isOptimized)
        return;

    // Implement optimization logic
    state.isOptimized = true;
}

float ObjectiveTracker::CalculateUrgencyFactor(Player* bot, const ObjectiveState& state)
{
    if (!bot)
        return 0.5f;

    // Calculate urgency based on quest level, rewards, and time constraints
    const Quest* quest = sObjectMgr->GetQuestTemplate(state.questId);
    if (!quest)
        return 0.5f;

    float urgency = 0.5f;

    // Higher urgency for higher level quests
    int32 botLevel = static_cast<int32>(bot->GetLevel());
    int32 questMinLevel = bot->GetQuestMinLevel(quest);
    if (questMinLevel >= botLevel)
        urgency += 0.3f;

    // Higher urgency for quests that have been active longer
    uint32 currentTime = GameTime::GetGameTimeMS();
    uint32 activeTime = currentTime - state.timeStarted;
    if (activeTime > 1800000) // 30 minutes
        urgency += 0.2f;

    return std::clamp(urgency, 0.0f, 1.0f);
}

float ObjectiveTracker::CalculateDifficultyFactor(Player* bot, const ObjectiveState& state)
{
    if (!bot)
        return 0.5f;

    // Lower difficulty = higher priority
    float difficulty = 0.5f;

    // Consider failure rate
    if (state.failureCount > 0)
        difficulty += 0.1f * state.failureCount;

    // Consider competition
    if (IsTargetContested(state.targetIds.empty() ? 0 : state.targetIds[0], 50.0f))
        difficulty += 0.2f;

    return 1.0f - std::clamp(difficulty, 0.0f, 1.0f);
}

float ObjectiveTracker::CalculateEfficiencyFactor(Player* bot, const ObjectiveState& state)
{
    if (!bot)
        return 0.5f;

    float efficiency = 0.5f;

    // Higher efficiency for faster completion velocity
    if (state.completionVelocity > 0.1f)
        efficiency += 0.3f;

    // Lower efficiency for stuck objectives
    if (state.isStuck)
        efficiency -= 0.4f;

    return std::clamp(efficiency, 0.0f, 1.0f);
}

float ObjectiveTracker::CalculateProximityFactor(Player* bot, const ObjectiveState& state)
{
    if (!bot)
        return 0.5f;

    float proximity = 0.5f;

    // Validate lastKnownPosition is initialized (not all zeros = uninitialized)
    // A position at exactly (0,0,0) is almost certainly uninitialized data
    if (state.lastKnownPosition.GetPositionX() == 0.0f &&
        state.lastKnownPosition.GetPositionY() == 0.0f &&
        state.lastKnownPosition.GetPositionZ() == 0.0f)
    {
        return proximity;  // Return default if position unknown
    }

    // Higher priority for closer objectives
    float distance = bot->GetDistance(state.lastKnownPosition);
    if (distance < 50.0f)
        proximity += 0.3f;
    else if (distance > 200.0f)
        proximity -= 0.2f;

    return std::clamp(proximity, 0.0f, 1.0f);
}

void ObjectiveTracker::RefreshObjectiveState(Player* bot, ObjectiveState& state)
{
    if (!bot)
        return;

    // Refresh objective state from current game state
    const Quest* quest = sObjectMgr->GetQuestTemplate(state.questId);
    if (!quest)
        return;

    uint32 currentProgress = GetCurrentObjectiveProgress(bot, quest, state.objectiveIndex);
    state.currentProgress = currentProgress;
    state.lastUpdateTime = GameTime::GetGameTimeMS();

    // CRITICAL FIX: Do NOT update position here!
    // lastKnownPosition stores the QUEST TARGET spawn location (set in StartTrackingObjective),
    // NOT the bot's current position. Overwriting this causes distance 0.0 bugs!
    // The position should only be set when the objective is first tracked.

    // Reset stuck state if progress was made
    if (currentProgress > 0 && state.isStuck)
    {
        state.isStuck = false;
        state.stuckTime = 0;
    }
}

uint32 ObjectiveTracker::GetCurrentObjectiveProgress(Player* bot, const Quest* quest, uint32 objectiveIndex)
{
    if (!bot || !quest || objectiveIndex >= 10)
        return 0;

    // Modern TrinityCore quest system uses QuestObjective vector
    if (objectiveIndex < quest->Objectives.size())
    {
        const QuestObjective& objective = quest->Objectives[objectiveIndex];

        // Get progress based on objective type
        // CRITICAL: In modern WoW (TrinityCore 11.x), quest items are NOT stored in bags!
        // Quest item progress is tracked via GetQuestObjectiveData(), not GetItemCount().
        // Quest items go into a special "quest item bag" that doesn't use inventory slots.
        switch (objective.Type)
        {
            case QUEST_OBJECTIVE_MONSTER:
            case QUEST_OBJECTIVE_GAMEOBJECT:
                // Kill/cast and game object objectives use kill credit tracking
                return bot->GetReqKillOrCastCurrentCount(quest->GetQuestId(), objective.ObjectID);

            case QUEST_OBJECTIVE_ITEM:
                // FIXED: Use GetQuestObjectiveData - quest items are tracked in quest log, not bags!
                // GetItemCount() returns 0 because quest items don't occupy bag slots in modern WoW.
                return bot->GetQuestObjectiveData(quest->GetQuestId(), objective.StorageIndex);

            default:
                // For other objective types, try GetQuestObjectiveData as fallback
                return bot->GetQuestObjectiveData(quest->GetQuestId(), objective.StorageIndex);
        }
    }

    return 0;
}

void ObjectiveTracker::HandleStuckObjective(Player* bot, ObjectiveState& state)
{
    if (!bot)
        return;

    if (!state.isStuck)
    {
        state.isStuck = true;
        state.stuckTime = GameTime::GetGameTimeMS();
    }

    state.failureCount++;

    TC_LOG_WARN("playerbot.objectives", "Objective stuck for bot {}: Quest {} Objective {}",
               bot->GetName(), state.questId, state.objectiveIndex);

    // Attempt recovery strategies
    if (state.failureCount <= 3)
    {
        // Try different approach
        AdaptTrackingStrategy(bot, state);
    }
    else
    {
        // Consider abandoning this objective
        TC_LOG_ERROR("playerbot.objectives", "Objective repeatedly failing for bot {}, may need intervention",
                    bot->GetName());
    }
}

QuestObjectiveData ObjectiveTracker::ConvertToQuestObjectiveData(const ObjectiveState& state)
{
    QuestObjectiveData data(state.questId, state.objectiveIndex, QuestObjectiveType::KILL_CREATURE, 0, state.requiredProgress);
    data.currentCount = state.currentProgress;

    return data;
}

uint32 ObjectiveTracker::AssignObjectiveToGroupMember(Group* group, Player* member, uint32 questId)
{
    if (!group || !member)
        return 0;

    // Assign specific objective to group member based on role, location, etc.
    // For now, return a simple assignment
    return 0; // Simplified
}

void ObjectiveTracker::AssignSpecificTargetToBot(Player* bot, uint32 questId, uint32 objectiveIndex, uint32 targetIndex)
{
    if (!bot)
        return;

    TC_LOG_DEBUG("playerbot.objectives", "Assigned target {} for objective {} in quest {} to bot {}",
                targetIndex, objectiveIndex, questId, bot->GetName());
}

Position ObjectiveTracker::FindObjectiveTargetLocation(Player* bot, const QuestObjectiveData& objective)
{
    Position invalidPosition; // Returns 0,0,0 by default

    if (!bot)
        return invalidPosition;

    // ENTERPRISE FIX: Use BotNpcLocationService instead of deadlock-prone spawn iteration
    // This eliminates the 261K+ spawn iteration that was causing bot deadlocks
    auto result = sBotNpcLocationService->FindQuestObjectiveLocation(bot, objective.questId, objective.objectiveIndex);

    if (result.isValid)
    {
        TC_LOG_DEBUG("module.playerbot.quest", "✅ FindObjectiveTargetLocation: Quest {} Objective {} resolved via {} - Quality: {}, Distance: {:.1f}",
                    objective.questId, objective.objectiveIndex, result.sourceName, result.qualityScore, result.distance);
        return result.position;
    }

    TC_LOG_WARN("module.playerbot.quest", "❌ FindObjectiveTargetLocation: Failed to resolve Quest {} Objective {} - returning invalid position",
                objective.questId, objective.objectiveIndex);
    return invalidPosition;

}

void ObjectiveTracker::ResolveObjectiveConflicts(Group* group, uint32 questId, uint32 objectiveIndex)
{
    if (!group)
        return;

    // Resolve conflicts by reassigning objectives or targets
    DistributeObjectiveTargets(group, questId, objectiveIndex);
}

void ObjectiveTracker::UpdateTrackingAnalytics(uint32 botGuid, const ObjectiveState& state, bool wasSuccessful)
{
    // Thread safety: lock before accessing _botAnalytics
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    auto& analytics = _botAnalytics[botGuid];

    if (wasSuccessful)
    {
        analytics.objectivesCompleted++;
        _globalAnalytics.objectivesCompleted++;
    }
    else
    {
        analytics.objectivesFailed++;
        _globalAnalytics.objectivesFailed++;
    }

    analytics.lastAnalyticsUpdate = GameTime::GetGameTimeMS();
    _globalAnalytics.lastAnalyticsUpdate = GameTime::GetGameTimeMS();
}

void ObjectiveTracker::Update(uint32 diff)
{
    static uint32 lastUpdate = 0;
    uint32 currentTime = GameTime::GetGameTimeMS();

    if (currentTime - lastUpdate < TRACKING_UPDATE_INTERVAL)
        return;

    lastUpdate = currentTime;

    // Clean up inactive tracking
    CleanupInactiveTracking();

    // Update global analytics
    _globalAnalytics.lastAnalyticsUpdate = GameTime::GetGameTimeMS();
}

void ObjectiveTracker::UpdateBotTracking(Player* bot, uint32 diff)
{
    if (!bot)
        return;

    UpdateObjectiveTracking(bot, diff);
}

void ObjectiveTracker::CleanupInactiveTracking()
{
    uint32 currentTime = GameTime::GetGameTimeMS();

    // Thread safety: lock before modifying maps
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    // Clean up old objective states
    for (auto it = _botObjectiveStates.begin(); it != _botObjectiveStates.end();)
    {
        auto& states = it->second;
        states.erase(
            std::remove_if(states.begin(), states.end(),
                [currentTime](const ObjectiveState& state) {
                    return currentTime - state.lastUpdateTime > OBJECTIVE_TIMEOUT;
                }),
            states.end()
        );

        if (states.empty())
            it = _botObjectiveStates.erase(it);
        else
            ++it;
    }

    // Clean up old target tracking data
    for (auto it = _targetTracking.begin(); it != _targetTracking.end();)
    {
        if (currentTime - it->second.lastSeenTime > TARGET_CACHE_DURATION)
            it = _targetTracking.erase(it);
        else
            ++it;
    }
}

} // namespace Playerbot