/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * Lock-Free Refactored Quest Completion System
 * This implementation removes all ObjectAccessor calls from worker threads
 * and uses action queuing for main thread execution.
 */

#include "QuestCompletion.h"
#include "GameTime.h"
#include "Player.h"
#include "Creature.h"
#include "GameObject.h"
#include "ObjectMgr.h"
#include "QuestDef.h"
#include "Log.h"
#include "Map.h"
#include "Spatial/SpatialGridManager.h"
#include "Threading/BotActionQueue.h"
#include "Threading/BotAction.h"
#include "InteractionManager.h"
#include "BotMovementUtil.h"

namespace Playerbot
{

// Constants
static constexpr float QUEST_SEARCH_RANGE = 100.0f;
static constexpr float QUEST_GIVER_INTERACTION_RANGE = 5.0f;
static constexpr float QUEST_COMBAT_RANGE = 40.0f;
static constexpr float QUEST_OBJECT_INTERACTION_RANGE = 10.0f;

/**
 * Lock-Free Implementation of HandleKillObjective
 * Uses spatial grid snapshots and action queuing instead of direct ObjectAccessor calls
 */
void QuestCompletion::HandleKillObjective_LockFree(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return;

    // Check if we already have enough kills
    if (objective.currentCount >= objective.requiredCount)
    {
        objective.status = ObjectiveStatus::COMPLETED;
        return;
    }

    // Get spatial grid for creature detection (thread-safe)
    Map* map = bot->GetMap();
    if (!map)
        return;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
        return;

    // Find target using snapshot data (NO ObjectAccessor!)
    float searchRange = QUEST_COMBAT_RANGE;
    ::std::vector<DoubleBufferedSpatialGrid::CreatureSnapshot> creatures =
        spatialGrid->QueryNearbyCreatures(bot->GetPosition(), searchRange);

    ObjectGuid targetGuid = ObjectGuid::Empty;
    float minDistance = searchRange + 1.0f;
    Position targetPosition;

    for (auto const& snapshot : creatures)
    {
        // Check if creature matches quest requirements
    if (!snapshot.isAlive)
            continue;

        // Check creature entry matches quest target
    if (snapshot.entry != objective.targetId)
            continue;

        // Check if hostile to bot
    if (!IsHostileSnapshot(snapshot, bot))
            continue;

        // Check if already engaged with too many enemies
    if (snapshot.threatListSize > 5)
            continue;

        float distance = snapshot.position.GetExactDist(bot->GetPosition());
        if (distance < minDistance)
        {
            minDistance = distance;
            targetGuid = snapshot.guid;
            targetPosition = snapshot.position;
        }
    }

    // Queue action instead of direct manipulation
    if (!targetGuid.IsEmpty())
    {
        // Check if in combat range
    if (minDistance <= bot->GetAttackDistance())
        {
            // Queue kill action for main thread
            BotAction action;
            action.type = BotActionType::KILL_QUEST_TARGET;
            action.botGuid = bot->GetGUID();
            action.targetGuid = targetGuid;
            action.questId = objective.questId;
            action.priority = 8;  // Quest combat is important
            action.queuedTime = GameTime::GetGameTimeMS();
            BotActionQueue::Instance()->Push(action);

            // Mark objective as in-progress (tracking only, no game state change)
            objective.status = ObjectiveStatus::IN_PROGRESS;

            TC_LOG_DEBUG("playerbot.quest",
                "QuestCompletion: Bot %s queued kill action for quest %u target %s at distance %.1f",
                bot->GetName().c_str(), objective.questId,
                targetGuid.ToString().c_str(), minDistance);
        }
        else
        {
            // Need to move closer first
            Position moveTarget = CalculateInterceptPosition(
                bot->GetPosition(), targetPosition,
                bot->GetAttackDistance() - 2.0f);

            BotAction moveAction = BotAction::MoveToPosition(
                bot->GetGUID(), moveTarget, GameTime::GetGameTimeMS()
            );
            moveAction.priority = 7;  // Movement for quest is moderately important

            BotActionQueue::Instance()->Push(moveAction);

            TC_LOG_DEBUG("playerbot.quest",
                "QuestCompletion: Bot %s moving to quest target, distance %.1f",
                bot->GetName().c_str(), minDistance);
        }
    }
    else
    {
        // No valid target found, may need to explore
    if (objective.targetLocation.GetPositionX() != 0)
        {
            // We have a known spawn location, navigate there
            NavigateToObjective_LockFree(bot, objective);
        }
    }
}

/**
 * Lock-Free Implementation of HandleTalkToNpcObjective
 * Uses spatial grid snapshots and action queuing
 */
void QuestCompletion::HandleTalkToNpcObjective_LockFree(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return;

    // Check if already completed
    if (objective.status == ObjectiveStatus::COMPLETED)
        return;

    Map* map = bot->GetMap();
    if (!map)
        return;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
        return;

    // Find NPC using snapshot data
    float searchRange = QUEST_SEARCH_RANGE;
    ::std::vector<DoubleBufferedSpatialGrid::CreatureSnapshot> creatures =
        spatialGrid->QueryNearbyCreatures(bot->GetPosition(), searchRange);

    ObjectGuid npcGuid = ObjectGuid::Empty;
    float minDistance = searchRange + 1.0f;
    Position npcPosition;
    for (auto const& snapshot : creatures)
    {
        // Check if creature is the quest NPC
    if (snapshot.entry != objective.targetId)
            continue;

        if (!snapshot.isAlive)
            continue;

        // Check if NPC is accessible (not in combat, not hostile)
    if (snapshot.isInCombat && snapshot.threatListSize > 0)
            continue;

        float distance = snapshot.position.GetExactDist(bot->GetPosition());
        if (distance < minDistance)
        {
            minDistance = distance;
            npcGuid = snapshot.guid;
            npcPosition = snapshot.position;
        }
    }

    if (!npcGuid.IsEmpty())
    {
        if (minDistance <= QUEST_GIVER_INTERACTION_RANGE)
        {
            // Queue interaction for main thread
            BotAction action;
            action.type = BotActionType::TALK_TO_QUEST_NPC;
            action.botGuid = bot->GetGUID();
            action.targetGuid = npcGuid;
            action.questId = objective.questId;
            action.priority = 6;
            action.queuedTime = GameTime::GetGameTimeMS();

            BotActionQueue::Instance()->Push(action);

            // Mark as in progress
            objective.status = ObjectiveStatus::IN_PROGRESS;

            TC_LOG_DEBUG("playerbot.quest",
                "QuestCompletion: Bot %s queued NPC interaction for quest %u, NPC %s",
                bot->GetName().c_str(), objective.questId,
                npcGuid.ToString().c_str());
        }
        else
        {
            // Need to move closer first
            Position targetPos = CalculateApproachPosition(
                bot->GetPosition(), npcPosition,
                QUEST_GIVER_INTERACTION_RANGE - 1.0f);

            BotAction moveAction = BotAction::MoveToPosition(
                bot->GetGUID(), targetPos, GameTime::GetGameTimeMS()
            );
            moveAction.priority = 6;
            BotActionQueue::Instance()->Push(moveAction);

            TC_LOG_DEBUG("playerbot.quest",
                "QuestCompletion: Bot %s moving to quest NPC, distance %.1f",
                bot->GetName().c_str(), minDistance);
        }
    }
    else
    {
        // NPC not found, navigate to known location if available
    if (objective.targetLocation.GetPositionX() != 0)
        {
            NavigateToObjective_LockFree(bot, objective);
        }
    }
}

/**
 * Lock-Free Implementation of HandleInteractObjectObjective
 * Uses spatial grid snapshots for GameObjects
 */
void QuestCompletion::HandleInteractObjectObjective_LockFree(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return;

    if (objective.status == ObjectiveStatus::COMPLETED)
        return;

    Map* map = bot->GetMap();
    if (!map)
        return;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
        return;

    // Find GameObject using snapshot data
    float searchRange = QUEST_SEARCH_RANGE;
    ::std::vector<DoubleBufferedSpatialGrid::GameObjectSnapshot> objects =
        spatialGrid->QueryNearbyGameObjects(bot->GetPosition(), searchRange);
    ObjectGuid objectGuid = ObjectGuid::Empty;
    float minDistance = searchRange + 1.0f;
    Position objectPosition;

    for (auto const& snapshot : objects)
    {
        // Check if object matches quest requirements
    if (snapshot.entry != objective.targetId)
            continue;

        // Check if object is usable
    if (!snapshot.isSpawned || snapshot.isInUse)
            continue;

        float distance = snapshot.position.GetExactDist(bot->GetPosition());
        if (distance < minDistance)
        {
            minDistance = distance;
            objectGuid = snapshot.guid;
            objectPosition = snapshot.position;
        }
    }

    if (!objectGuid.IsEmpty())
    {
        if (minDistance <= QUEST_OBJECT_INTERACTION_RANGE)
        {
            // Queue interaction for main thread
            BotAction action;
            action.type = BotActionType::INTERACT_QUEST_OBJECT;
            action.botGuid = bot->GetGUID();
            action.targetGuid = objectGuid;
            action.questId = objective.questId;
            action.priority = 6;
            action.queuedTime = GameTime::GetGameTimeMS();

            BotActionQueue::Instance()->Push(action);

            objective.status = ObjectiveStatus::IN_PROGRESS;

            TC_LOG_DEBUG("playerbot.quest",
                "QuestCompletion: Bot %s queued object interaction for quest %u, object %s",
                bot->GetName().c_str(), objective.questId,
                objectGuid.ToString().c_str());
        }
        else
        {
            // Move closer to object
            Position targetPos = CalculateApproachPosition(
                bot->GetPosition(), objectPosition,
                QUEST_OBJECT_INTERACTION_RANGE - 1.0f);

            BotAction moveAction = BotAction::MoveToPosition(
                bot->GetGUID(), targetPos, GameTime::GetGameTimeMS()
            );
            moveAction.priority = 6;

            BotActionQueue::Instance()->Push(moveAction);

            TC_LOG_DEBUG("playerbot.quest",
                "QuestCompletion: Bot %s moving to quest object, distance %.1f",
                bot->GetName().c_str(), minDistance);
        }
    }
    else
    {
        // Object not found, navigate to known location
    if (objective.targetLocation.GetPositionX() != 0)
        {
            NavigateToObjective_LockFree(bot, objective);
        }
    }
}

/**
 * Lock-Free Implementation of HandleEscortObjective
 * Manages escort NPCs without direct object access
 */
void QuestCompletion::HandleEscortObjective_LockFree(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return;

    Map* map = bot->GetMap();
    if (!map)
        return;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
        return;

    // Check if we have an escort target
    if (objective.escortGuid.IsEmpty())
    {
        // Find the escort NPC
        float searchRange = QUEST_SEARCH_RANGE;
        ::std::vector<DoubleBufferedSpatialGrid::CreatureSnapshot> creatures =
            spatialGrid->QueryNearbyCreatures(bot->GetPosition(), searchRange);

        for (auto const& snapshot : creatures)
        {
            if (snapshot.entry != objective.targetId)
                continue;

            if (!snapshot.isAlive)
                continue;

            // Found escort NPC
            objective.escortGuid = snapshot.guid;

            // Queue escort start action
            BotAction action;
            action.type = BotActionType::ESCORT_NPC;
            action.botGuid = bot->GetGUID();
            action.targetGuid = snapshot.guid;
            action.questId = objective.questId;
            action.priority = 7;
            action.queuedTime = GameTime::GetGameTimeMS();

            BotActionQueue::Instance()->Push(action);

            TC_LOG_DEBUG("playerbot.quest",
                "QuestCompletion: Bot %s starting escort for quest %u, NPC %s",
                bot->GetName().c_str(), objective.questId,
                snapshot.guid.ToString().c_str());

            break;
        }
    }
    else
    {
        // We have an escort target, check if it's still valid
        ::std::vector<DoubleBufferedSpatialGrid::CreatureSnapshot> creatures =
            spatialGrid->QueryNearbyCreatures(bot->GetPosition(), 100.0f);

        bool escortValid = false;
        Position escortPosition;

        for (auto const& snapshot : creatures)
        {
            if (snapshot.guid == objective.escortGuid)
            {
                if (snapshot.isAlive)
                {
                    escortValid = true;
                    escortPosition = snapshot.position;

                    // Check if escort is under attack
    if (snapshot.isInCombat && snapshot.healthPct < 50)
                    {
                        // Help defend the escort
    if (!snapshot.victim.IsEmpty())
                        {
                            BotAction action;
                            action.type = BotActionType::ATTACK_TARGET;
                            action.botGuid = bot->GetGUID();
                            action.targetGuid = snapshot.victim;
                            action.priority = 9;  // High priority to defend escort
                            action.queuedTime = GameTime::GetGameTimeMS();

                            BotActionQueue::Instance()->Push(action);
                        }
                    }
                }
                break;
            }
        }

        if (!escortValid)
        {
            // Escort died or despawned
            objective.status = ObjectiveStatus::FAILED;
            objective.escortGuid = ObjectGuid::Empty;

            TC_LOG_DEBUG("playerbot.quest",
                "QuestCompletion: Escort failed for quest %u",
                objective.questId);
        }
        else
        {
            // Follow the escort
            float distance = escortPosition.GetExactDist(bot->GetPosition());
            if (distance > 10.0f)
            {
                BotAction action = BotAction::FollowTarget(
                    bot->GetGUID(), objective.escortGuid, GameTime::GetGameTimeMS()
                );
                action.priority = 6;

                BotActionQueue::Instance()->Push(action);
            }
        }
    }
}

/**
 * Helper: Navigate to objective location without ObjectAccessor
 */
void QuestCompletion::NavigateToObjective_LockFree(Player* bot, QuestObjectiveData const& objective)
{
    if (!bot || objective.targetLocation.GetPositionX() == 0)
        return;

    float distance = bot->GetDistance(objective.targetLocation);
    if (distance > 5.0f)
    {
        BotAction action = BotAction::MoveToPosition(
            bot->GetGUID(), objective.targetLocation, GameTime::GetGameTimeMS()
        );
        action.priority = 5;

        BotActionQueue::Instance()->Push(action);

        TC_LOG_DEBUG("playerbot.quest",
            "QuestCompletion: Bot %s navigating to quest location for quest %u",
            bot->GetName().c_str(), objective.questId);
    }
}

/**
 * Helper: Check if creature snapshot is hostile to bot
 */
bool QuestCompletion::IsHostileSnapshot(
    DoubleBufferedSpatialGrid::CreatureSnapshot const& snapshot,
    Player* bot)
{
    if (!bot)
        return false;

    // Check faction hostility using snapshot data
    // This is a simplified check - real implementation would use faction templates
    if (snapshot.faction == 0)
        return false;

    // Check if creature is already attacking the bot
    if (snapshot.victim == bot->GetGUID())
        return true;

    // For quest mobs, we generally consider them hostile if they're the target
    // This avoids complex faction calculations in worker thread
    return true;
}

/**
 * Helper: Calculate position to approach a target
 */
Position QuestCompletion::CalculateApproachPosition(
    Position const& from,
    Position const& to,
    float desiredDistance)
{
    float angle = from.GetAngle(&to);
    float currentDist = from.GetExactDist(&to);

    if (currentDist <= desiredDistance)
        return from;  // Already close enough

    float moveDist = currentDist - desiredDistance;

    Position result;
    result.m_positionX = from.m_positionX + moveDist * cos(angle);
    result.m_positionY = from.m_positionY + moveDist * sin(angle);
    result.m_positionZ = from.m_positionZ;  // Will be corrected by pathfinding

    return result;
}

/**
 * Helper: Calculate intercept position for moving target
 */
Position QuestCompletion::CalculateInterceptPosition(
    Position const& from,
    Position const& targetPos,
    float desiredDistance)
{
    // Simple intercept - move toward where target is
    // More complex implementation would predict future position
    return CalculateApproachPosition(from, targetPos, desiredDistance);
}

} // namespace Playerbot