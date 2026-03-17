/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "QuestCompletion.h"
#include "Core/Events/GenericEventBus.h"
#include "QuestEvents.h"
#include "Core/Events/GenericEventBus.h"
#include "GameTime.h"
#include "Core/PlayerBotHelpers.h"  // GetBotAI, GetGameSystems
#include "Log.h"
#include "ObjectMgr.h"
#include "World.h"
#include "WorldSession.h"
#include "Group.h"
#include "GossipDef.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "QuestPickup.h"
#include "QuestTurnIn.h"
#include "ObjectiveTracker.h"
#include "Movement/LeaderFollowBehavior.h"
#include "../AI/BotAI.h"
#include "Movement/BotMovementUtil.h"
#include "Interaction/Core/InteractionManager.h"
#include "../Combat/TargetSelector.h"
#include "Config/PlayerbotConfig.h"
#include "SpellMgr.h"
#include "ObjectAccessor.h"
#include "../Spatial/SpatialGridManager.h"  // Lock-free spatial grid for deadlock fix
#include "Loot/LootItemType.h"  // For LootItemType::Item in RewardQuest
#include "Item.h"  // For Item* type in EvaluateItemUpgrade
#include "DataStores/DB2Stores.h" // For sFactionStore, sAreaTriggerStore
#include "ReputationMgr.h"        // For ReputationRank, GetReputationMgr
#include <algorithm>
#include <cmath>

namespace Playerbot
{

// Quest giver interaction range constant
constexpr float QUEST_GIVER_INTERACTION_RANGE = 5.0f;

/**
 * @brief Constructor
 *
 * Initializes quest completion manager and subscribes to EventBus for real-time
 * quest progress updates from the packet sniffer.
 */
QuestCompletion::QuestCompletion(Player* bot)
    : _bot(bot)
{
    if (!_bot)
    {
        TC_LOG_ERROR("playerbot.quest", "QuestCompletion: null bot!");
        return;
    }

    _globalMetrics.Reset();

    // Subscribe to EventBus for real-time quest events from packet sniffer
    // Using callback subscription since QuestCompletion is not a BotAI
    _eventBusSubscriptionId = EventBus<QuestEvent>::instance()->SubscribeCallback(
        [this](QuestEvent const& event) { HandleQuestEvent(event); },
        {
            QuestEventType::QUEST_CREDIT_ADDED,      // Real-time objective progress
            QuestEventType::QUEST_COMPLETED,         // Auto turn-in trigger
            QuestEventType::QUEST_FAILED,            // Recovery trigger
            QuestEventType::QUEST_OFFER_REWARD,      // Turn-in flow trigger
            QuestEventType::QUEST_POI_RECEIVED       // Navigation optimization
        }
    );

    TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Initialized for bot {}, EventBus subscription ID {}",
        _bot->GetName(), _eventBusSubscriptionId);
}

/**
 * @brief Destructor
 *
 * CRITICAL: Unsubscribes from EventBus to prevent dangling pointer crashes.
 */
QuestCompletion::~QuestCompletion()
{
    // CRITICAL: Unsubscribe from EventBus to prevent dangling pointer crashes
    if (_eventBusSubscriptionId != 0)
    {
        EventBus<QuestEvent>::instance()->UnsubscribeCallback(_eventBusSubscriptionId);
        TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Unsubscribed from EventBus (ID {})",
            _eventBusSubscriptionId);
        _eventBusSubscriptionId = 0;
    }
}

/**
 * @brief Start tracking quest completion for a bot
 * @param questId Quest ID to track
 * @param bot Bot player
 * @return True if quest tracking started successfully
 */
bool QuestCompletion::StartQuestCompletion(uint32 questId, Player* bot)
{
    if (!bot || !questId)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);

    // Check if bot has the quest
    if (bot->GetQuestStatus(questId) == QUEST_STATUS_NONE)
    {
        TC_LOG_DEBUG("playerbot", "QuestCompletion::StartQuestCompletion - Bot %s doesn't have quest %u",
            bot->GetName().c_str(), questId);
        return false;
    }

    // Initialize quest progress tracking
    QuestProgressData progress(questId, bot->GetGUID().GetCounter());
    progress.questGiverGuid = 0; // Will be populated when quest giver is found
    // Parse quest objectives
    ParseQuestObjectives(progress, quest);
    // Set appropriate completion strategy based on bot configuration
    if (bot->GetGroup())
        progress.strategy = QuestCompletionStrategy::GROUP_COORDINATION;
    else
        progress.strategy = QuestCompletionStrategy::EFFICIENT_COMPLETION;

    // Store progress data
    _botQuestProgress[bot->GetGUID().GetCounter()].push_back(progress);
    _botsInQuestMode.insert(bot->GetGUID().GetCounter());

    // Update metrics
    _globalMetrics.questsStarted++;
    _botMetrics[bot->GetGUID().GetCounter()].questsStarted++;

    TC_LOG_DEBUG("playerbot", "QuestCompletion::StartQuestCompletion - Started tracking quest %u for bot %s",
        questId, bot->GetName().c_str());

    return true;
}

/**
 * @brief Update quest progress for a bot
 * @param bot Bot player
 */
void QuestCompletion::UpdateQuestProgress(Player* bot)
{
    if (!bot)
        return;

    auto it = _botQuestProgress.find(bot->GetGUID().GetCounter());
    if (it == _botQuestProgress.end())
        return;

    for (auto& progress : it->second)
    {
        // Skip completed quests
        if (progress.completionPercentage >= 100.0f)
            continue;

        // Update each objective
        bool anyProgress = false;
        for (auto& objective : progress.objectives)
        {
            if (objective.status == ObjectiveStatus::COMPLETED)
                continue;

            UpdateQuestObjectiveFromProgress(objective, bot);

            if (objective.currentCount > 0)
                anyProgress = true;
        }

        // Calculate overall completion percentage
        float totalProgress = 0.0f;
        uint32 completedObjectives = 0;
        for (const auto& objective : progress.objectives)
        {
            if (objective.requiredCount > 0)
            {
                float objProgress = std::min(1.0f,
                    static_cast<float>(objective.currentCount) / objective.requiredCount);
                totalProgress += objProgress;

                if (objective.status == ObjectiveStatus::COMPLETED)
                    completedObjectives++;
            }
        }

        if (!progress.objectives.empty())
        {
            progress.completionPercentage = (totalProgress / progress.objectives.size()) * 100.0f;
        }
        // Check for stuck state
        if (!anyProgress)
        {
            uint32 currentTime = GameTime::GetGameTimeMS();
            if (currentTime - progress.lastUpdateTime > STUCK_DETECTION_TIME)
            {
                progress.isStuck = true;
                progress.stuckTime = currentTime;
                DetectStuckState(bot, progress.questId);
            }
        }
        else
        {
            progress.isStuck = false;
            progress.stuckTime = 0;
            progress.lastUpdateTime = GameTime::GetGameTimeMS();
        }

        // Check if quest is ready for turn-in
        if (progress.completionPercentage >= 100.0f && progress.requiresTurnIn)
        {
            CompleteQuest(progress.questId, bot);
        }
    }
}

/**
 * @brief Complete a quest for the bot
 * @param questId Quest ID
 * @param bot Bot player
 */
void QuestCompletion::CompleteQuest(uint32 questId, Player* bot)
{
    if (!bot || !questId)
        return;

    // Check if quest can be completed
    if (!bot->CanCompleteQuest(questId))
    {
        TC_LOG_DEBUG("playerbot", "QuestCompletion::CompleteQuest - Bot %s cannot complete quest %u yet",
            bot->GetName().c_str(), questId);
        return;
    }

    // Mark quest as complete
    bot->CompleteQuest(questId);

    // Update progress tracking
    auto it = _botQuestProgress.find(bot->GetGUID().GetCounter());
    if (it != _botQuestProgress.end())
    {
        auto questIt = std::find_if(it->second.begin(), it->second.end(),
            [questId](const QuestProgressData& data) { return data.questId == questId; });

        if (questIt != it->second.end())
        {
            questIt->completionPercentage = 100.0f;
            questIt->requiresTurnIn = true;

            // Add completion log entry
            questIt->completionLog.push_back("Quest completed at " + std::to_string(GameTime::GetGameTimeMS()));
        }
    }

    // Update metrics
    _globalMetrics.questsCompleted++;
    _botMetrics[bot->GetGUID().GetCounter()].questsCompleted++;

    TC_LOG_DEBUG("playerbot", "QuestCompletion::CompleteQuest - Bot %s completed quest %u",
        bot->GetName().c_str(), questId);

    // Schedule turn-in through QuestTurnIn system (per-bot)
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestTurnIn()->ScheduleQuestTurnIn(bot, questId);
}

/**
 * @brief Turn in a completed quest
 * @param questId Quest ID
 * @param bot Bot player
 * @return True if quest was turned in successfully
 */
bool QuestCompletion::TurnInQuest(uint32 questId, Player* bot)
{
    if (!bot || !questId)
        return false;

    // Delegate to QuestTurnIn system (per-bot)
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestTurnIn()->TurnInQuest(questId, bot);
    return false;
}

/**
 * @brief Track quest objectives for a bot
 * @param bot Bot player
 */
void QuestCompletion::TrackQuestObjectives(Player* bot)
{
    if (!bot)
        return;

    auto it = _botQuestProgress.find(bot->GetGUID().GetCounter());
    if (it == _botQuestProgress.end())
        return;

    for (auto& progress : it->second)
    {
        // Skip completed quests
        if (progress.completionPercentage >= 100.0f)
            continue;

        // Execute objectives based on strategy
        switch (progress.strategy)
        {
            case QuestCompletionStrategy::EFFICIENT_COMPLETION:
                ExecuteEfficientStrategy(bot, progress);
                break;
            case QuestCompletionStrategy::SAFE_COMPLETION:
                ExecuteSafeStrategy(bot, progress);
                break;
            case QuestCompletionStrategy::GROUP_COORDINATION:
                ExecuteGroupStrategy(bot, progress);
                break;
            case QuestCompletionStrategy::SOLO_FOCUS:
                ExecuteSoloStrategy(bot, progress);
                break;
            case QuestCompletionStrategy::EXPERIENCE_MAXIMIZING:
                ExecuteExperienceStrategy(bot, progress);
                break;
            case QuestCompletionStrategy::SPEED_COMPLETION:
                ExecuteSpeedStrategy(bot, progress);
                break;
            case QuestCompletionStrategy::THOROUGH_EXPLORATION:
                ExecuteExplorationStrategy(bot, progress);
                break;
        }
    }
}

/**
 * @brief Execute a quest objective
 * @param bot Bot player
 * @param objective Objective data
 */
void QuestCompletion::ExecuteObjective(Player* bot, QuestObjectiveData& objective)
{
    if (!bot || objective.status == ObjectiveStatus::COMPLETED)
        return;

    // Check if we can execute this objective
    if (!CanExecuteObjective(bot, objective))
    {
        objective.status = ObjectiveStatus::BLOCKED;
        return;
    }

    objective.status = ObjectiveStatus::IN_PROGRESS;

    // Execute based on objective type
    // Our enum now matches TrinityCore's IDs for types 0-22
    // Note: Aliases (KILL_CREATURE, COLLECT_ITEM, etc.) have the same numeric value
    // as their primary types, so we only need one case per value
    switch (objective.type)
    {
        // ====================================================================
        // TRINITYCORE STANDARD TYPES (IDs 0-22)
        // ====================================================================
        case QuestObjectiveType::MONSTER:           // 0 - Kill creature (alias: KILL_CREATURE)
            HandleKillObjective(bot, objective);
            break;

        case QuestObjectiveType::ITEM:              // 1 - Collect item (alias: COLLECT_ITEM)
            HandleCollectObjective(bot, objective);
            break;

        case QuestObjectiveType::GAMEOBJECT:        // 2 - Use gameobject (alias: USE_GAMEOBJECT)
            HandleGameObjectObjective(bot, objective);
            break;

        case QuestObjectiveType::TALKTO:            // 3 - Talk to NPC (alias: TALK_TO_NPC)
            HandleTalkToNpcObjective(bot, objective);
            break;

        case QuestObjectiveType::CURRENCY:          // 4 - Spend currency
            HandleCurrencyObjective(bot, objective);
            break;

        case QuestObjectiveType::LEARNSPELL:        // 5 - Learn spell (alias: LEARN_SPELL)
            HandleLearnSpellObjective(bot, objective);
            break;

        case QuestObjectiveType::MIN_REPUTATION:    // 6 - Minimum reputation
            HandleMinReputationObjective(bot, objective);
            break;

        case QuestObjectiveType::MAX_REPUTATION:    // 7 - Maximum reputation
            HandleMaxReputationObjective(bot, objective);
            break;

        case QuestObjectiveType::MONEY:             // 8 - Acquire gold
            HandleMoneyObjective(bot, objective);
            break;

        case QuestObjectiveType::PLAYERKILLS:       // 9 - Kill players
            HandlePlayerKillsObjective(bot, objective);
            break;

        case QuestObjectiveType::AREATRIGGER:       // 10 - Area trigger (alias: REACH_LOCATION)
            HandleAreaTriggerObjective(bot, objective);
            break;

        case QuestObjectiveType::WINPETBATTLEAGAINSTNPC:  // 11 - Win pet battle vs NPC
            HandlePetBattleNPCObjective(bot, objective);
            break;

        case QuestObjectiveType::DEFEATBATTLEPET:   // 12 - Defeat battle pet
            HandleDefeatBattlePetObjective(bot, objective);
            break;

        case QuestObjectiveType::WINPVPPETBATTLES:  // 13 - Win PvP pet battles
            HandlePvPPetBattlesObjective(bot, objective);
            break;

        case QuestObjectiveType::CRITERIA_TREE:     // 14 - Criteria tree
            HandleCriteriaTreeObjective(bot, objective);
            break;

        case QuestObjectiveType::PROGRESS_BAR:      // 15 - Progress bar
            HandleProgressBarObjective(bot, objective);
            break;

        case QuestObjectiveType::HAVE_CURRENCY:     // 16 - Have currency on turn-in
            HandleHaveCurrencyObjective(bot, objective);
            break;

        case QuestObjectiveType::OBTAIN_CURRENCY:   // 17 - Obtain currency
            HandleObtainCurrencyObjective(bot, objective);
            break;

        case QuestObjectiveType::INCREASE_REPUTATION: // 18 - Gain reputation
            HandleIncreaseReputationObjective(bot, objective);
            break;

        case QuestObjectiveType::AREA_TRIGGER_ENTER: // 19 - Enter area trigger
            HandleAreaTriggerEnterObjective(bot, objective);
            break;

        case QuestObjectiveType::AREA_TRIGGER_EXIT:  // 20 - Exit area trigger
            HandleAreaTriggerExitObjective(bot, objective);
            break;

        case QuestObjectiveType::KILL_WITH_LABEL:   // 21 - Kill with label
            HandleKillWithLabelObjective(bot, objective);
            break;

        // ====================================================================
        // EXTENDED PLAYERBOT TYPES (IDs 100+)
        // ====================================================================
        case QuestObjectiveType::CAST_SPELL:        // 100 - Cast spell
            HandleSpellCastObjective(bot, objective);
            break;

        case QuestObjectiveType::EMOTE_AT_TARGET:   // 101 - Emote
            HandleEmoteObjective(bot, objective);
            break;

        case QuestObjectiveType::ESCORT_NPC:        // 102 - Escort NPC
            HandleEscortObjective(bot, objective);
            break;

        case QuestObjectiveType::DEFEND_AREA:       // 103 - Defend area
        case QuestObjectiveType::SURVIVE_TIME:      // 104 - Survive time
        case QuestObjectiveType::WIN_BATTLEGROUND:  // 105 - Win BG
        case QuestObjectiveType::COMPLETE_DUNGEON:  // 106 - Complete dungeon
        case QuestObjectiveType::GAIN_EXPERIENCE:   // 107 - Gain XP
            // These complex objectives need special handling - mark as needing attention
            TC_LOG_DEBUG("playerbot.quest", "ExecuteObjective: Complex objective type {} for quest {} needs special handling",
                static_cast<uint32>(objective.type), objective.questId);
            objective.status = ObjectiveStatus::BLOCKED;
            break;

        case QuestObjectiveType::UNK_1127:          // 22 - Unknown
        case QuestObjectiveType::CUSTOM_OBJECTIVE:  // 255 - Fallback
        default:
            TC_LOG_WARN("playerbot.quest", "ExecuteObjective: Unhandled objective type {} for quest {}",
                static_cast<uint32>(objective.type), objective.questId);
            objective.status = ObjectiveStatus::BLOCKED;
            break;
    }

    // Update time spent
    objective.timeSpent += GameTime::GetGameTimeMS() - objective.lastUpdateTime;
    objective.lastUpdateTime = GameTime::GetGameTimeMS();
}

/**
 * @brief Update objective progress
 * @param bot Bot player
 * @param questId Quest ID
 * @param objectiveIndex Objective index
 */
void QuestCompletion::UpdateObjectiveProgress(Player* bot, uint32 questId, uint32 objectiveIndex)
{
    if (!bot)
        return;

    auto it = _botQuestProgress.find(bot->GetGUID().GetCounter());
    if (it == _botQuestProgress.end())
        return;

    auto questIt = std::find_if(it->second.begin(), it->second.end(),
        [questId](const QuestProgressData& data) { return data.questId == questId; });

    if (questIt == it->second.end() || objectiveIndex >= questIt->objectives.size())
        return;

    auto& objective = questIt->objectives[objectiveIndex];

    // Get quest from ObjectMgr to access objectives
    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return;

    QuestObjectives const& questObjectives = quest->GetObjectives();
    if (objectiveIndex >= questObjectives.size())
        return;

    QuestObjective const& questObj = questObjectives[objectiveIndex];

    // Update objective count using Player::GetQuestObjectiveData
    objective.currentCount = bot->GetQuestObjectiveData(questObj);
    if (objective.currentCount >= objective.requiredCount)
    {
        objective.status = ObjectiveStatus::COMPLETED;
        _globalMetrics.objectivesCompleted++;
        _botMetrics[bot->GetGUID().GetCounter()].objectivesCompleted++;
    }
}

/**
 * @brief Check if an objective is complete
 * @param objective Objective data
 * @return True if objective is complete
 */
bool QuestCompletion::IsObjectiveComplete(const QuestObjectiveData& objective)
{
    return objective.status == ObjectiveStatus::COMPLETED ||
           (objective.requiredCount > 0 && objective.currentCount >= objective.requiredCount);
}

/**
 * @brief Handle kill objectives
 * @param bot Bot player
 * @param objective Objective data
 */
void QuestCompletion::HandleKillObjective(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return;

    // Find creatures to kill
    if (!FindKillTarget(bot, objective))
    {
        // No targets found, move to spawn location if known
        if (objective.targetLocation.GetPositionX() != 0)
        {
            NavigateToObjective(bot, objective);
        }
        return;
    }

    // Get nearest target
    Unit* target = nullptr;
    float minDistance = objective.searchRadius;

    // DEADLOCK FIX: Use lock-free spatial grid
    Map* map = bot->GetMap();
    if (map)
    {
        DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
        {
            sSpatialGridManager.CreateGrid(map);
            spatialGrid = sSpatialGridManager.GetGrid(map);
        }

        if (spatialGrid)
        {
            // DEADLOCK FIX: Use snapshot-based query (thread-safe, lock-free)
            std::vector<DoubleBufferedSpatialGrid::CreatureSnapshot> nearbyCreatures =
                spatialGrid->QueryNearbyCreatures(bot->GetPosition(), objective.searchRadius);

            ObjectGuid targetGuid;  // Store GUID instead of pointer

            for (auto const& snapshot : nearbyCreatures)
            {
                // Use snapshot fields instead of calling methods on pointers
                if (snapshot.isDead || !snapshot.isVisible)
                    continue;
                if (snapshot.entry != objective.targetId)
                    continue;

                float distance = bot->GetExactDist(snapshot.position);
                if (distance < minDistance)
                {
                    minDistance = distance;
                    targetGuid = snapshot.guid;  // Store GUID
                }
            }

            // Store the GUID for later use (main thread will resolve)
            if (!targetGuid.IsEmpty())
                target = ObjectAccessor::GetCreature(*bot, targetGuid);  // Safe: this method may run on main thread context
        }
    }

    if (target)
    {
        // Engage target through combat system
        if (BotAI* ai = dynamic_cast<BotAI*>(bot->GetAI()))
        {
            // Set the bot's target to the creature
            bot->SetSelection(target->GetGUID());
            // Attack the target
            bot->Attack(target, true);
        }

        // Track kill credit - Update using proper API
        Quest const* quest = sObjectMgr->GetQuestTemplate(objective.questId);
        if (quest)
        {
            QuestObjectives const& questObjectives = quest->GetObjectives();
            if (objective.objectiveIndex < questObjectives.size())
            {
                objective.currentCount = bot->GetQuestObjectiveData(questObjectives[objective.objectiveIndex]);
            }
        }

        TC_LOG_DEBUG("playerbot", "QuestCompletion::HandleKillObjective - Bot %s engaging %s for quest %u",
            bot->GetName().c_str(), target->GetName().c_str(), objective.questId);
    }
}

/**
 * @brief Handle collect objectives
 * @param bot Bot player
 * @param objective Objective data
 */
void QuestCompletion::HandleCollectObjective(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return;

    // Check if we already have the items
    uint32 itemCount = bot->GetItemCount(objective.targetId);
    if (itemCount >= objective.requiredCount)
    {
        objective.currentCount = itemCount;
        objective.status = ObjectiveStatus::COMPLETED;
        return;
    }

    // Find items to collect (from creatures or game objects)
    if (!FindCollectibleItem(bot, objective))
    {
        // Move to collection area
        if (objective.targetLocation.GetPositionX() != 0)
        {
            NavigateToObjective(bot, objective);
        }
        return;
    }

    // Update collection progress
    objective.currentCount = bot->GetItemCount(objective.targetId);
}

/**
 * @brief Handle talk to NPC objectives
 * @param bot Bot player
 * @param objective Objective data
 */
void QuestCompletion::HandleTalkToNpcObjective(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return;

    // Find the NPC
    Creature* npc = nullptr;
    if (objective.targetId)
    {
        // DEADLOCK FIX: Use lock-free spatial grid
        Map* map = bot->GetMap();
        if (map)
        {
            DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
            if (!spatialGrid)
            {
                sSpatialGridManager.CreateGrid(map);
                spatialGrid = sSpatialGridManager.GetGrid(map);
            }
            if (spatialGrid)
            {
                // DEADLOCK FIX: Use snapshot-based query (thread-safe, lock-free)
                std::vector<DoubleBufferedSpatialGrid::CreatureSnapshot> nearbyCreatures =
                    spatialGrid->QueryNearbyCreatures(bot->GetPosition(), objective.searchRadius);
                ObjectGuid npcGuid;
                float minDistance = objective.searchRadius;

                for (auto const& snapshot : nearbyCreatures)
                {
                    // Use snapshot fields instead of pointer method calls
                    if (!snapshot.isVisible)
                        continue;
                    if (snapshot.entry != objective.targetId)
                        continue;
                    float distance = bot->GetExactDist(snapshot.position);
                    if (distance < minDistance)
                    {
                        minDistance = distance;
                        npcGuid = snapshot.guid;
                    }
                }

                // Resolve GUID to pointer after loop (safe if method runs on main thread context)
                if (!npcGuid.IsEmpty())
                    npc = ObjectAccessor::GetCreature(*bot, npcGuid);
            }
        }
    }

    if (npc)
    {
        // Move to NPC if not in range
        if (bot->GetDistance(npc) > QUEST_GIVER_INTERACTION_RANGE)
        {
            BotMovementUtil::MoveToUnit(bot, npc, QUEST_GIVER_INTERACTION_RANGE - 1.0f);
            return;
        }

        // Interact with NPC - use StartInteraction instead of InteractWithNPC
        InteractionManager::Instance()->StartInteraction(bot, npc, InteractionType::None);

        // Mark objective as complete
        objective.status = ObjectiveStatus::COMPLETED;
        objective.currentCount = objective.requiredCount;

        TC_LOG_DEBUG("playerbot", "QuestCompletion::HandleTalkToNpcObjective - Bot %s talked to %s for quest %u",
            bot->GetName().c_str(), npc->GetName().c_str(), objective.questId);
    }
    else
    {
        // Move to NPC location if known
        if (objective.targetLocation.GetPositionX() != 0)
        {
            NavigateToObjective(bot, objective);
        }
    }
}

/**
 * @brief Handle location objectives
 * @param bot Bot player
 * @param objective Objective data
 */
void QuestCompletion::HandleLocationObjective(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return;

    // Check if we're at the location
    float distance = bot->GetDistance(objective.targetLocation);
    if (distance <= OBJECTIVE_COMPLETION_RADIUS)
    {
        // Trigger area exploration
        bot->AreaExploredOrEventHappens(objective.questId);

        objective.status = ObjectiveStatus::COMPLETED;
        objective.currentCount = objective.requiredCount;

        TC_LOG_DEBUG("playerbot", "QuestCompletion::HandleLocationObjective - Bot %s reached location for quest %u",
            bot->GetName().c_str(), objective.questId);
    }
    else
    {
        // Navigate to location
        NavigateToObjective(bot, objective);
    }
}
/**
 * @brief Handle game object objectives
 * @param bot Bot player
 * @param objective Objective data
 */
void QuestCompletion::HandleGameObjectObjective(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return;

    // DEADLOCK FIX: Use lock-free spatial grid to find game object
    GameObject* gameObject = nullptr;
    if (objective.targetId)
    {
        Map* map = bot->GetMap();
        if (!map)
            return;

        DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
        {
            sSpatialGridManager.CreateGrid(map);
            spatialGrid = sSpatialGridManager.GetGrid(map);
            if (!spatialGrid)
                return;
        }

        // Query nearby GameObjects (lock-free!)
        std::vector<DoubleBufferedSpatialGrid::GameObjectSnapshot> nearbyObjects =
            spatialGrid->QueryNearbyGameObjects(bot->GetPosition(), objective.searchRadius);
        // Find matching GameObject using snapshots
        ObjectGuid targetGuid;
        for (auto const& snapshot : nearbyObjects)
        {
            if (snapshot.entry == objective.targetId && snapshot.isSpawned)
            {
                targetGuid = snapshot.guid;
                break;
            }
        }

        // Resolve GUID after loop
        if (!targetGuid.IsEmpty())
            gameObject = ObjectAccessor::GetGameObject(*bot, targetGuid);
    }

    if (gameObject)
    {
        // Move to object if not in range
        if (bot->GetDistance(gameObject) > QUEST_GIVER_INTERACTION_RANGE)
        {
            BotMovementUtil::MoveToPosition(bot, gameObject->GetPosition());
            return;
        }

        // Use the game object
        gameObject->Use(bot);

        // Update progress
        objective.currentCount++;
        if (objective.currentCount >= objective.requiredCount)
        {
            objective.status = ObjectiveStatus::COMPLETED;
        }

        TC_LOG_DEBUG("playerbot", "QuestCompletion::HandleGameObjectObjective - Bot %s used %s for quest %u",
            bot->GetName().c_str(), gameObject->GetName().c_str(), objective.questId);
    }
    else
    {
        // Move to object location if known
        if (objective.targetLocation.GetPositionX() != 0)
        {
            NavigateToObjective(bot, objective);
        }
    }
}

/**
 * @brief Handle spell cast objectives
 * @param bot Bot player
 * @param objective Objective data
 */
void QuestCompletion::HandleSpellCastObjective(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return;

    // Check if bot knows the spell
    if (!bot->HasSpell(objective.targetId))
    {
        TC_LOG_DEBUG("playerbot", "QuestCompletion::HandleSpellCastObjective - Bot %s doesn't know spell %u for quest %u",
            bot->GetName().c_str(), objective.targetId, objective.questId);
        objective.status = ObjectiveStatus::BLOCKED;
        return;
    }

    // Find target for spell if needed
    Unit* target = bot->GetSelectedUnit();
    if (!target && objective.targetLocation.GetPositionX() != 0)
    {
        // Move to cast location
        NavigateToObjective(bot, objective);
        return;
    }

    // Cast the spell
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(objective.targetId, DIFFICULTY_NONE);
    if (spellInfo)
    {
        bot->CastSpell(target ? target : bot, objective.targetId, false);

        // Update progress
        objective.currentCount++;
        if (objective.currentCount >= objective.requiredCount)
        {
            objective.status = ObjectiveStatus::COMPLETED;
        }
        TC_LOG_DEBUG("playerbot", "QuestCompletion::HandleSpellCastObjective - Bot %s cast spell %u for quest %u",
            bot->GetName().c_str(), objective.targetId, objective.questId);
    }
}

/**
 * @brief Handle emote objectives
 * @param bot Bot player
 * @param objective Objective data
 */
void QuestCompletion::HandleEmoteObjective(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return;

    // Find target for emote
    Unit* target = nullptr;
    if (objective.targetId)
    {
        // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
        Map* map = bot->GetMap();
        if (!map)
            return;

        DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
        {
            sSpatialGridManager.CreateGrid(map);
            spatialGrid = sSpatialGridManager.GetGrid(map);
            if (!spatialGrid)
                return;
        }

        // Use snapshot-based query (thread-safe, lock-free)
        std::vector<DoubleBufferedSpatialGrid::CreatureSnapshot> nearbyCreatures =
            spatialGrid->QueryNearbyCreatures(bot->GetPosition(), QUEST_GIVER_INTERACTION_RANGE);

        ObjectGuid targetGuid;
        for (auto const& snapshot : nearbyCreatures)
        {
            if (snapshot.entry == objective.targetId && snapshot.isVisible)
            {
                targetGuid = snapshot.guid;
                break;  // Use first matching creature
            }
        }

        // Resolve GUID to pointer after loop
        if (!targetGuid.IsEmpty())
            target = ObjectAccessor::GetCreature(*bot, targetGuid);
    }

    if (target)
    {
        // Perform emote (cast targetId to Emote enum)
        bot->HandleEmoteCommand(static_cast<Emote>(objective.targetId));

        // Update progress
        objective.currentCount++;
        if (objective.currentCount >= objective.requiredCount)
        {
            objective.status = ObjectiveStatus::COMPLETED;
        }

        TC_LOG_DEBUG("playerbot", "QuestCompletion::HandleEmoteObjective - Bot %s performed emote for quest %u",
            bot->GetName().c_str(), objective.questId);
    }
    else
    {
        // Move to target location
        if (objective.targetLocation.GetPositionX() != 0)
        {
            NavigateToObjective(bot, objective);
        }
    }
}

/**
 * @brief Handle escort objectives
 * @param bot Bot player
 * @param objective Objective data
 */
void QuestCompletion::HandleEscortObjective(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return;

    // Find escort target
    Creature* escortTarget = nullptr;
    if (objective.targetId)
    {
        // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
        Map* map = bot->GetMap();
        if (!map)
            return;

        DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
        {
            sSpatialGridManager.CreateGrid(map);
            spatialGrid = sSpatialGridManager.GetGrid(map);
            if (!spatialGrid)
                return;
        }
        // Use snapshot-based query (thread-safe, lock-free)
        std::vector<DoubleBufferedSpatialGrid::CreatureSnapshot> nearbyCreatures =
            spatialGrid->QueryNearbyCreatures(bot->GetPosition(), objective.searchRadius);

        ObjectGuid escortGuid;
        for (auto const& snapshot : nearbyCreatures)
        {
            if (snapshot.entry == objective.targetId && !snapshot.isDead)
            {
                escortGuid = snapshot.guid;
                break;  // Use first alive matching creature
            }
        }

        // Resolve GUID to pointer after loop
        if (!escortGuid.IsEmpty())
            escortTarget = ObjectAccessor::GetCreature(*bot, escortGuid);
    }

    if (escortTarget)
    {
        // Follow the escort target
        if (bot->GetDistance(escortTarget) > 10.0f)
        {
            BotMovementUtil::MoveToUnit(bot, escortTarget, 5.0f);
        }

        // Check if escort is complete (handled by quest system)
        if (bot->IsQuestObjectiveComplete(objective.questId, objective.objectiveIndex))
        {
            objective.status = ObjectiveStatus::COMPLETED;
            objective.currentCount = objective.requiredCount;
        }

        TC_LOG_DEBUG("playerbot", "QuestCompletion::HandleEscortObjective - Bot %s escorting %s for quest %u",
            bot->GetName().c_str(), escortTarget->GetName().c_str(), objective.questId);
    }
    else
    {
        // Move to escort start location
        if (objective.targetLocation.GetPositionX() != 0)
        {
            NavigateToObjective(bot, objective);
        }
    }
}

/**
 * @brief Navigate to quest objective
 * @param bot Bot player
 * @param objective Objective data
 */
void QuestCompletion::NavigateToObjective(Player* bot, const QuestObjectiveData& objective)
{
    if (!bot)
        return;

    // Get optimal position for objective
    Position targetPos = GetOptimalObjectivePosition(bot, objective);
    // Use movement utility to navigate
    BotMovementUtil::MoveToPosition(bot, targetPos);

    TC_LOG_DEBUG("playerbot", "QuestCompletion::NavigateToObjective - Bot %s moving to objective for quest %u",
        bot->GetName().c_str(), objective.questId);
}

/**
 * @brief Find target for kill objective
 * @param bot Bot player
 * @param objective Objective data
 * @return True if target found
 */
bool QuestCompletion::FindKillTarget(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return false;
    // DEADLOCK FIX: Use lock-free spatial grid with snapshots
    Map* map = bot->GetMap();
    if (!map)
        return false;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return false;
    }

    // Use snapshot-based query (thread-safe, lock-free)
    std::vector<DoubleBufferedSpatialGrid::CreatureSnapshot> nearbyCreatures =
        spatialGrid->QueryNearbyCreatures(bot->GetPosition(), objective.searchRadius);

    // Filter valid targets using snapshot data
    DoubleBufferedSpatialGrid::CreatureSnapshot nearestSnapshot;
    float minDistance = objective.searchRadius;
    bool found = false;

    for (auto const& snapshot : nearbyCreatures)
    {
        // Filter: must match entry, be alive, visible, and hostile
        if (snapshot.entry != objective.targetId)
            continue;
        if (snapshot.isDead || !snapshot.isVisible)
            continue;
        // Note: faction check would require additional snapshot field or be done after GUID resolution

        float distance = bot->GetExactDist(snapshot.position);
        if (distance < minDistance)
        {
            minDistance = distance;
            nearestSnapshot = snapshot;
            found = true;
        }
    }

    if (found)
    {
        // Update objective location to nearest target (using snapshot position)
        objective.targetLocation = nearestSnapshot.position;
        return true;
    }

    return false;
}

/**
 * @brief Find collectible item for objective
 * @param bot Bot player
 * @param objective Objective data
 * @return True if item source found
 */
bool QuestCompletion::FindCollectibleItem(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return false;

    Map* map = bot->GetMap();
    if (!map)
        return false;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return false;
    }

    // DEADLOCK FIX: First check if item drops from creatures using snapshots
    std::vector<DoubleBufferedSpatialGrid::CreatureSnapshot> nearbyCreatures =
        spatialGrid->QueryNearbyCreatures(bot->GetPosition(), objective.searchRadius);

    for (auto const& snapshot : nearbyCreatures)
    {
        // Filter: must be alive and not friendly (assume non-friendly if we can't check faction)
        if (snapshot.isDead)
            continue;

        // Check if creature can drop the item (would need loot template access)
        // For now, assume creatures in quest area can drop quest items
        float distance = bot->GetExactDist(snapshot.position);
        if (distance < objective.searchRadius)
        {
            objective.targetLocation = snapshot.position;
            return true;
        }
    }

    // DEADLOCK FIX: Check for game objects using snapshots
    std::vector<DoubleBufferedSpatialGrid::GameObjectSnapshot> nearbyObjects =
        spatialGrid->QueryNearbyGameObjects(bot->GetPosition(), objective.searchRadius);

    for (auto const& snapshot : nearbyObjects)
    {
        // Check if game object is lootable using snapshot data
        if (snapshot.goType == GAMEOBJECT_TYPE_CHEST ||
            snapshot.goType == GAMEOBJECT_TYPE_GOOBER)
        {
            objective.targetLocation = snapshot.position;
            return true;
        }
    }

    return false;
}

/**
 * @brief Get optimal position for objective
 * @param bot Bot player
 * @param objective Objective data
 * @return Optimal position
 */
Position QuestCompletion::GetOptimalObjectivePosition(Player* bot, const QuestObjectiveData& objective)
{
    if (!bot)
        return Position();

    // If we have a specific target location, use it
    if (objective.targetLocation.GetPositionX() != 0)
        return objective.targetLocation;

    // Otherwise, try to find objective locations from database
    std::vector<Position> locations = GetObjectiveLocations(objective);
    if (!locations.empty())
    {
        // Return nearest location
        return *std::min_element(locations.begin(), locations.end(),
            [bot](const Position& a, const Position& b)
            {
                return bot->GetDistance(a) < bot->GetDistance(b);
            });
    }

    // Default to bot's current position
    return bot->GetPosition();
}

/**
 * @brief Get possible locations for objective
 * @param objective Objective data
 * @return Vector of possible positions
 */
std::vector<Position> QuestCompletion::GetObjectiveLocations(const QuestObjectiveData& objective)
{
    std::vector<Position> locations;

    // Add primary location if set
    if (objective.targetLocation.GetPositionX() != 0)
        locations.push_back(objective.targetLocation);

    // Could query database for spawn points of objective creatures/objects
    // For now, return what we have

    return locations;
}

/**
 * @brief Parse quest objectives from quest template
 * @param progress Quest progress data
 * @param quest Quest template
 */
void QuestCompletion::ParseQuestObjectives(QuestProgressData& progress, const Quest* quest)
{
    if (!quest)
        return;

    // Parse objectives from Quest::Objectives vector (modern TrinityCore API)
    QuestObjectives const& questObjectives = quest->GetObjectives();

    for (size_t i = 0; i < questObjectives.size(); ++i)
    {
        QuestObjective const& obj = questObjectives[i];

        // Map TrinityCore objective types DIRECTLY to our enum (1:1 mapping)
        // Our enum now matches TrinityCore's IDs exactly for types 0-22
        QuestObjectiveType internalType;
        switch (obj.Type)
        {
            // ====================================================================
            // DIRECT 1:1 MAPPINGS (TrinityCore ID == Playerbot ID)
            // ====================================================================
            case QUEST_OBJECTIVE_MONSTER:           // 0 - Kill creature
                internalType = QuestObjectiveType::MONSTER;
                break;
            case QUEST_OBJECTIVE_ITEM:              // 1 - Collect item
                internalType = QuestObjectiveType::ITEM;
                break;
            case QUEST_OBJECTIVE_GAMEOBJECT:        // 2 - Use gameobject
                internalType = QuestObjectiveType::GAMEOBJECT;
                break;
            case QUEST_OBJECTIVE_TALKTO:            // 3 - Talk to NPC
                internalType = QuestObjectiveType::TALKTO;
                break;
            case QUEST_OBJECTIVE_CURRENCY:          // 4 - Spend currency
                internalType = QuestObjectiveType::CURRENCY;
                break;
            case QUEST_OBJECTIVE_LEARNSPELL:        // 5 - Learn spell
                internalType = QuestObjectiveType::LEARNSPELL;
                break;
            case QUEST_OBJECTIVE_MIN_REPUTATION:    // 6 - Minimum reputation
                internalType = QuestObjectiveType::MIN_REPUTATION;
                break;
            case QUEST_OBJECTIVE_MAX_REPUTATION:    // 7 - Maximum reputation
                internalType = QuestObjectiveType::MAX_REPUTATION;
                break;
            case QUEST_OBJECTIVE_MONEY:             // 8 - Acquire gold
                internalType = QuestObjectiveType::MONEY;
                break;
            case QUEST_OBJECTIVE_PLAYERKILLS:       // 9 - Kill players
                internalType = QuestObjectiveType::PLAYERKILLS;
                break;
            case QUEST_OBJECTIVE_AREATRIGGER:       // 10 - Area trigger
                internalType = QuestObjectiveType::AREATRIGGER;
                break;
            case QUEST_OBJECTIVE_WINPETBATTLEAGAINSTNPC:  // 11 - Win pet battle vs NPC
                internalType = QuestObjectiveType::WINPETBATTLEAGAINSTNPC;
                break;
            case QUEST_OBJECTIVE_DEFEATBATTLEPET:   // 12 - Defeat battle pet
                internalType = QuestObjectiveType::DEFEATBATTLEPET;
                break;
            case QUEST_OBJECTIVE_WINPVPPETBATTLES:  // 13 - Win PvP pet battles
                internalType = QuestObjectiveType::WINPVPPETBATTLES;
                break;
            case QUEST_OBJECTIVE_CRITERIA_TREE:     // 14 - Criteria tree
                internalType = QuestObjectiveType::CRITERIA_TREE;
                break;
            case QUEST_OBJECTIVE_PROGRESS_BAR:      // 15 - Progress bar
                internalType = QuestObjectiveType::PROGRESS_BAR;
                break;
            case QUEST_OBJECTIVE_HAVE_CURRENCY:     // 16 - Have currency on turn-in
                internalType = QuestObjectiveType::HAVE_CURRENCY;
                break;
            case QUEST_OBJECTIVE_OBTAIN_CURRENCY:   // 17 - Obtain currency
                internalType = QuestObjectiveType::OBTAIN_CURRENCY;
                break;
            case QUEST_OBJECTIVE_INCREASE_REPUTATION: // 18 - Gain reputation
                internalType = QuestObjectiveType::INCREASE_REPUTATION;
                break;
            case QUEST_OBJECTIVE_AREA_TRIGGER_ENTER: // 19 - Enter area trigger
                internalType = QuestObjectiveType::AREA_TRIGGER_ENTER;
                break;
            case QUEST_OBJECTIVE_AREA_TRIGGER_EXIT:  // 20 - Exit area trigger
                internalType = QuestObjectiveType::AREA_TRIGGER_EXIT;
                break;
            case QUEST_OBJECTIVE_KILL_WITH_LABEL:   // 21 - Kill with label
                internalType = QuestObjectiveType::KILL_WITH_LABEL;
                break;
            case QUEST_OBJECTIVE_UNK_1127:          // 22 - Unknown 12.0.7
                internalType = QuestObjectiveType::UNK_1127;
                TC_LOG_DEBUG("playerbot.quest", "ParseQuestObjectives: Quest {} has unknown objective type 22 (UNK_1127)",
                    quest->GetQuestId());
                break;

            default:
                internalType = QuestObjectiveType::CUSTOM_OBJECTIVE;
                TC_LOG_WARN("playerbot.quest", "ParseQuestObjectives: Quest {} has unhandled objective type {}",
                    quest->GetQuestId(), static_cast<uint32>(obj.Type));
                break;
        }

        // Create objective data (handle objectives with Amount=0 for flag-based types)
        uint32 amount = obj.Amount > 0 ? obj.Amount : 1;

        // Flag-based objectives (like AREATRIGGER) may have Amount=0
        bool isFlagBased = (obj.Type == QUEST_OBJECTIVE_AREATRIGGER ||
                           obj.Type == QUEST_OBJECTIVE_AREA_TRIGGER_ENTER ||
                           obj.Type == QUEST_OBJECTIVE_AREA_TRIGGER_EXIT ||
                           obj.Type == QUEST_OBJECTIVE_WINPETBATTLEAGAINSTNPC ||
                           obj.Type == QUEST_OBJECTIVE_DEFEATBATTLEPET ||
                           obj.Type == QUEST_OBJECTIVE_CRITERIA_TREE);

        if (obj.Amount > 0 || isFlagBased)
        {
            QuestObjectiveData objective(quest->GetQuestId(), static_cast<uint32>(i),
                                        internalType, obj.ObjectID, amount);
            objective.description = obj.Description;
            objective.isOptional = (obj.Flags & QUEST_OBJECTIVE_FLAG_OPTIONAL) != 0;
            progress.objectives.push_back(objective);

            TC_LOG_DEBUG("playerbot.quest", "ParseQuestObjectives: Quest {} objective {}: type={}, target={}, amount={}",
                quest->GetQuestId(), i, static_cast<uint32>(internalType), obj.ObjectID, amount);
        }
    }
}

/**
 * @brief Update quest objective from game progress
 * @param objective Objective data to update
 * @param bot Bot player
 */
void QuestCompletion::UpdateQuestObjectiveFromProgress(QuestObjectiveData& objective, Player* bot)
{
    if (!bot)
        return;

    // Get quest from ObjectMgr to access objectives
    Quest const* quest = sObjectMgr->GetQuestTemplate(objective.questId);
    if (!quest)
        return;

    QuestObjectives const& questObjectives = quest->GetObjectives();
    if (objective.objectiveIndex >= questObjectives.size())
        return;

    QuestObjective const& questObj = questObjectives[objective.objectiveIndex];

    // Update based on objective type
    // Our enum now matches TrinityCore's IDs for types 0-22
    // Note: Aliases have same numeric values as their primaries, use only primaries in switch
    switch (objective.type)
    {
        // ====================================================================
        // COUNT-BASED OBJECTIVES (Use GetQuestObjectiveData)
        // ====================================================================
        case QuestObjectiveType::MONSTER:           // 0 - Kill creature (alias: KILL_CREATURE)
        case QuestObjectiveType::GAMEOBJECT:        // 2 - Use gameobject (alias: USE_GAMEOBJECT)
        case QuestObjectiveType::TALKTO:            // 3 - Talk to NPC (alias: TALK_TO_NPC)
        case QuestObjectiveType::PLAYERKILLS:       // 9 - Player kills
        case QuestObjectiveType::KILL_WITH_LABEL:   // 21 - Kill with label
            objective.currentCount = bot->GetQuestObjectiveData(questObj);
            break;

        // ====================================================================
        // ITEM-BASED OBJECTIVES (Use GetItemCount)
        // ====================================================================
        case QuestObjectiveType::ITEM:              // 1 - Collect item (alias: COLLECT_ITEM)
            objective.currentCount = bot->GetItemCount(objective.targetId);
            break;

        // ====================================================================
        // CURRENCY-BASED OBJECTIVES (Use GetCurrency)
        // ====================================================================
        case QuestObjectiveType::CURRENCY:          // 4 - Spend currency
        case QuestObjectiveType::HAVE_CURRENCY:     // 16 - Have currency
            objective.currentCount = bot->GetCurrencyQuantity(objective.targetId);
            break;

        case QuestObjectiveType::OBTAIN_CURRENCY:   // 17 - Obtain currency (tracked by quest)
            objective.currentCount = bot->GetQuestObjectiveData(questObj);
            break;

        // ====================================================================
        // SPELL-BASED OBJECTIVES (Use HasSpell)
        // ====================================================================
        case QuestObjectiveType::LEARNSPELL:        // 5 - Learn spell (alias: LEARN_SPELL)
            objective.currentCount = bot->HasSpell(objective.targetId) ? objective.requiredCount : 0;
            break;

        // ====================================================================
        // REPUTATION-BASED OBJECTIVES (Use GetReputation)
        // ====================================================================
        case QuestObjectiveType::MIN_REPUTATION:    // 6 - Minimum reputation
        case QuestObjectiveType::MAX_REPUTATION:    // 7 - Maximum reputation
        {
            FactionEntry const* factionEntry = sFactionStore.LookupEntry(objective.targetId);
            if (factionEntry)
            {
                int32 rep = bot->GetReputationMgr().GetReputation(factionEntry);
                // For MIN_REP: current >= required means complete
                // For MAX_REP: current <= required means complete
                if (objective.type == QuestObjectiveType::MAX_REPUTATION)
                    objective.currentCount = rep <= static_cast<int32>(objective.requiredCount) ? objective.requiredCount : 0;
                else
                    objective.currentCount = rep >= static_cast<int32>(objective.requiredCount) ? objective.requiredCount : static_cast<uint32>(std::max(0, rep));
            }
            break;
        }

        case QuestObjectiveType::INCREASE_REPUTATION: // 18 - Gain reputation (tracked by quest)
            objective.currentCount = bot->GetQuestObjectiveData(questObj);
            break;

        // ====================================================================
        // MONEY-BASED OBJECTIVES (Use GetMoney)
        // ====================================================================
        case QuestObjectiveType::MONEY:             // 8 - Acquire gold
            objective.currentCount = bot->GetMoney() >= objective.requiredCount ? objective.requiredCount : static_cast<uint32>(bot->GetMoney());
            break;

        // ====================================================================
        // FLAG-BASED OBJECTIVES (Use IsQuestObjectiveComplete or GetQuestObjectiveData)
        // ====================================================================
        case QuestObjectiveType::AREATRIGGER:       // 10 - Area trigger (alias: REACH_LOCATION)
        case QuestObjectiveType::AREA_TRIGGER_ENTER: // 19 - Enter area trigger
        case QuestObjectiveType::AREA_TRIGGER_EXIT:  // 20 - Exit area trigger
        case QuestObjectiveType::WINPETBATTLEAGAINSTNPC:  // 11 - Win pet battle vs NPC
        case QuestObjectiveType::DEFEATBATTLEPET:   // 12 - Defeat battle pet
        case QuestObjectiveType::WINPVPPETBATTLES:  // 13 - Win PvP pet battles
        case QuestObjectiveType::CRITERIA_TREE:     // 14 - Criteria tree
            if (bot->IsQuestObjectiveComplete(objective.questId, objective.objectiveIndex))
                objective.currentCount = objective.requiredCount;
            else
                objective.currentCount = bot->GetQuestObjectiveData(questObj);
            break;

        // ====================================================================
        // PROGRESS BAR OBJECTIVES (Use GetQuestObjectiveData)
        // ====================================================================
        case QuestObjectiveType::PROGRESS_BAR:      // 15 - Progress bar
            objective.currentCount = bot->GetQuestObjectiveData(questObj);
            break;

        // ====================================================================
        // EXTENDED PLAYERBOT TYPES (IDs 100+)
        // ====================================================================
        case QuestObjectiveType::CAST_SPELL:        // 100 - Cast spell
        case QuestObjectiveType::EMOTE_AT_TARGET:   // 101 - Emote
        case QuestObjectiveType::ESCORT_NPC:        // 102 - Escort NPC
        case QuestObjectiveType::DEFEND_AREA:       // 103 - Defend area
        case QuestObjectiveType::SURVIVE_TIME:      // 104 - Survive time
        case QuestObjectiveType::WIN_BATTLEGROUND:  // 105 - Win BG
        case QuestObjectiveType::COMPLETE_DUNGEON:  // 106 - Complete dungeon
        case QuestObjectiveType::GAIN_EXPERIENCE:   // 107 - Gain XP
            objective.currentCount = bot->GetQuestObjectiveData(questObj);
            break;

        // ====================================================================
        // UNKNOWN/FALLBACK
        // ====================================================================
        case QuestObjectiveType::UNK_1127:          // 22 - Unknown
        case QuestObjectiveType::CUSTOM_OBJECTIVE:  // 255 - Fallback
        default:
            // Check if objective is complete through quest system
            if (bot->IsQuestObjectiveComplete(objective.questId, objective.objectiveIndex))
            {
                objective.currentCount = objective.requiredCount;
            }
            else
            {
                objective.currentCount = bot->GetQuestObjectiveData(questObj);
            }
            break;
    }

    // Update status
    if (objective.currentCount >= objective.requiredCount)
    {
        objective.status = ObjectiveStatus::COMPLETED;
    }
}

/**
 * @brief Check if objective can be executed
 * @param bot Bot player
 * @param objective Objective data
 * @return True if objective can be executed
 */
bool QuestCompletion::CanExecuteObjective(Player* bot, const QuestObjectiveData& objective)
{
    if (!bot)
        return false;

    // Check if objective is already complete
    if (objective.status == ObjectiveStatus::COMPLETED)
        return false;

    // Check if bot is alive
    if (!bot->IsAlive())
        return false;

    // Check if bot is in combat for non-combat objectives
    // MONSTER (0) is the primary type for kill objectives (KILL_CREATURE is an alias with same value)
    if (bot->IsInCombat() && objective.type != QuestObjectiveType::MONSTER)
        return false;

    // Check if objective has failed too many times
    if (objective.retryCount >= MAX_OBJECTIVE_RETRIES)
        return false;

    // Check group requirements
    if (objective.requiresGroup && !bot->GetGroup())
        return false;

    return true;
}

/**
 * @brief Execute efficient completion strategy
 * @param bot Bot player
 * @param progress Quest progress data
 */
void QuestCompletion::ExecuteEfficientStrategy(Player* bot, QuestProgressData& progress)
{
    if (!bot)
        return;

    // Find nearest incomplete objective
    QuestObjectiveData* nearestObjective = nullptr;
    float minDistance = std::numeric_limits<float>::max();

    for (auto& objective : progress.objectives)
    {
        if (objective.status == ObjectiveStatus::COMPLETED)
            continue;

        float distance = bot->GetDistance(objective.targetLocation);
        if (distance < minDistance)
        {
            minDistance = distance;
            nearestObjective = &objective;
        }
    }

    if (nearestObjective)
    {
        ExecuteObjective(bot, *nearestObjective);
    }
}

/**
 * @brief Execute safe completion strategy
 * @param bot Bot player
 * @param progress Quest progress data
 */
void QuestCompletion::ExecuteSafeStrategy(Player* bot, QuestProgressData& progress)
{
    if (!bot)
        return;

    // Prioritize non-combat objectives when health is low
    if (bot->GetHealthPct() < 50.0f)
    {
        for (auto& objective : progress.objectives)
        {
            if (objective.status != ObjectiveStatus::COMPLETED &&
                objective.type != QuestObjectiveType::KILL_CREATURE)
            {
                ExecuteObjective(bot, objective);
                return;
            }
        }
    }

    // Otherwise execute normally
    ExecuteEfficientStrategy(bot, progress);
}

/**
 * @brief Execute group coordination strategy
 * @param bot Bot player
 * @param progress Quest progress data
 */
void QuestCompletion::ExecuteGroupStrategy(Player* bot, QuestProgressData& progress)
{
    if (!bot || !bot->GetGroup())
    {
        ExecuteEfficientStrategy(bot, progress);
        return;
    }

    // Coordinate with group members
    Group* group = bot->GetGroup();
    // Share objective progress
    ShareObjectiveProgress(group, progress.questId);

    // Find objectives that group is working on
    for (auto& objective : progress.objectives)
    {
        if (objective.status != ObjectiveStatus::COMPLETED)
        {
            // Check if other group members are near this objective
            bool groupNearby = false;
            for (GroupReference const& itr : group->GetMembers())
            {
                Player* member = itr.GetSource();
                if (member && member != bot && member->IsAlive())
                {
                    if (member->GetDistance(objective.targetLocation) < 50.0f)
                    {
                        groupNearby = true;
                        break;
                    }
                }
            }

            if (groupNearby)
            {
                ExecuteObjective(bot, objective);
                return;
            }
        }
    }

    // No group objectives, execute normally
    ExecuteEfficientStrategy(bot, progress);
}

/**
 * @brief Share objective progress with group
 * @param group Group pointer
 * @param questId Quest ID
 */
void QuestCompletion::ShareObjectiveProgress(Group* group, uint32 questId)
{
    if (!group)
        return;

    // Update group quest sharing data
    _groupQuestSharing[group->GetGUID().GetCounter()].push_back(questId);
    _groupObjectiveSync[group->GetGUID().GetCounter()][questId] = GameTime::GetGameTimeMS();
}

/**
 * @brief Detect stuck state for quest
 * @param bot Bot player
 * @param questId Quest ID
 */
void QuestCompletion::DetectStuckState(Player* bot, uint32 questId)
{
    if (!bot)
        return;

    TC_LOG_DEBUG("playerbot", "QuestCompletion::DetectStuckState - Bot %s stuck on quest %u",
        bot->GetName().c_str(), questId);

    auto it = _botQuestProgress.find(bot->GetGUID().GetCounter());
    if (it == _botQuestProgress.end())
        return;

    auto questIt = std::find_if(it->second.begin(), it->second.end(),
        [questId](const QuestProgressData& data) { return data.questId == questId; });

    if (questIt != it->second.end())
    {
        questIt->isStuck = true;
        questIt->stuckTime = GameTime::GetGameTimeMS();
        questIt->consecutiveFailures++;

        // Update metrics
        _globalMetrics.stuckInstances++;
        _botMetrics[bot->GetGUID().GetCounter()].stuckInstances++;

        // Try to recover
        RecoverFromStuckState(bot, questId);
    }
}

/**
 * @brief Recover from stuck state
 * @param bot Bot player
 * @param questId Quest ID
 */
void QuestCompletion::RecoverFromStuckState(Player* bot, uint32 questId)
{
    if (!bot)
        return;

    TC_LOG_DEBUG("playerbot", "QuestCompletion::RecoverFromStuckState - Attempting recovery for bot %s on quest %u",
        bot->GetName().c_str(), questId);

    // Try different recovery strategies
    auto it = _botQuestProgress.find(bot->GetGUID().GetCounter());
    if (it == _botQuestProgress.end())
        return;

    auto questIt = std::find_if(it->second.begin(), it->second.end(),
        [questId](const QuestProgressData& data) { return data.questId == questId; });

    if (questIt != it->second.end())
    {
        // Reset stuck objectives
        for (auto& objective : questIt->objectives)
        {
            if (objective.status == ObjectiveStatus::IN_PROGRESS)
            {
                objective.status = ObjectiveStatus::NOT_STARTED;
                objective.retryCount++;
            }
        }

        // Change strategy
        if (questIt->strategy == QuestCompletionStrategy::EFFICIENT_COMPLETION)
            questIt->strategy = QuestCompletionStrategy::SAFE_COMPLETION;

        // Reset stuck flag
        questIt->isStuck = false;
        questIt->stuckTime = 0;
    }
}

/**
 * @brief Get bot completion metrics
 * @param botGuid Bot GUID
 * @return Completion metrics
 */
QuestCompletion::QuestCompletionMetricsSnapshot QuestCompletion::GetBotCompletionMetrics(uint32 botGuid)
{
    auto it = _botMetrics.find(botGuid);
    QuestCompletion::QuestCompletionMetricsSnapshot result{};

    if (it != _botMetrics.end())
    {
        auto snapshot = it->second.CreateSnapshot();
        result.questsStarted = snapshot.questsStarted;
        result.questsCompleted = snapshot.questsCompleted;
        result.questsFailed = snapshot.questsFailed;
        result.objectivesCompleted = snapshot.objectivesCompleted;
        result.stuckInstances = snapshot.stuckInstances;
        result.averageCompletionTime = snapshot.averageCompletionTime;
        result.completionSuccessRate = snapshot.completionSuccessRate;
        result.objectiveEfficiency = snapshot.objectiveEfficiency;
        result.totalDistanceTraveled = snapshot.totalDistanceTraveled;
    }

    return result;
}

/**
 * @brief Get global completion metrics
 * @return Global completion metrics
 */
QuestCompletion::QuestCompletionMetricsSnapshot QuestCompletion::GetGlobalCompletionMetrics()
{
    auto snapshot = _globalMetrics.CreateSnapshot();
    QuestCompletion::QuestCompletionMetricsSnapshot result{};
    result.questsStarted = snapshot.questsStarted;
    result.questsCompleted = snapshot.questsCompleted;
    result.questsFailed = snapshot.questsFailed;
    result.objectivesCompleted = snapshot.objectivesCompleted;
    result.stuckInstances = snapshot.stuckInstances;
    result.averageCompletionTime = snapshot.averageCompletionTime;
    result.completionSuccessRate = snapshot.completionSuccessRate;
    result.objectiveEfficiency = snapshot.objectiveEfficiency;
    result.totalDistanceTraveled = snapshot.totalDistanceTraveled;
    return result;
}

/**
 * @brief Set quest completion strategy for bot
 * @param botGuid Bot GUID
 * @param strategy Completion strategy
 */
void QuestCompletion::SetQuestCompletionStrategy(uint32 botGuid, QuestCompletionStrategy strategy)
{
    _botStrategies[botGuid] = strategy;
}

/**
 * @brief Get quest completion strategy for bot
 * @param botGuid Bot GUID
 * @return Completion strategy
 */
QuestCompletionStrategy QuestCompletion::GetQuestCompletionStrategy(uint32 botGuid)
{
    auto it = _botStrategies.find(botGuid);
    if (it != _botStrategies.end())
        return it->second;

    return QuestCompletionStrategy::EFFICIENT_COMPLETION;
}

/**
 * @brief Update quest completion system
 * @param diff Time difference
 */
void QuestCompletion::Update(uint32 diff)
{
    // Clean up completed quests periodically
    static uint32 cleanupTimer = 0;
    cleanupTimer += diff;

    if (cleanupTimer >= 60000) // Every minute
    {
        CleanupCompletedQuests();
        cleanupTimer = 0;
    }

    // Note: Quest turn-in processing is handled by QuestTurnIn system
}

/**
 * @brief Clean up completed quests
 */
void QuestCompletion::CleanupCompletedQuests()
{
    uint32 currentTime = GameTime::GetGameTimeMS();

    for (auto& [botGuid, progressList] : _botQuestProgress)
    {
        progressList.erase(
            std::remove_if(progressList.begin(), progressList.end(),
                [currentTime](const QuestProgressData& progress)
                {
                    return progress.completionPercentage >= 100.0f &&
                           (currentTime - progress.lastUpdateTime) > 300000; // 5 minutes
                }),
            progressList.end()
        );
    }
}

/**
 * @brief Execute solo focus strategy
 * @param bot Bot player
 * @param progress Quest progress data
 */
void QuestCompletion::ExecuteSoloStrategy(Player* bot, QuestProgressData& progress)
{
    if (!bot)
        return;

    // Focus on objectives that don't require group
    for (auto& objective : progress.objectives)
    {
        if (objective.status != ObjectiveStatus::COMPLETED && !objective.requiresGroup)
        {
            ExecuteObjective(bot, objective);
            return;
        }
    }

    // If all require group, try anyway
    ExecuteEfficientStrategy(bot, progress);
}

/**
 * @brief Execute experience maximizing strategy
 * @param bot Bot player
 * @param progress Quest progress data
 */
void QuestCompletion::ExecuteExperienceStrategy(Player* bot, QuestProgressData& progress)
{
    if (!bot)
        return;

    // Prioritize kill objectives for experience
    for (auto& objective : progress.objectives)
    {
        if (objective.status != ObjectiveStatus::COMPLETED &&
            objective.type == QuestObjectiveType::KILL_CREATURE)
        {
            ExecuteObjective(bot, objective);
            return;
        }
    }

    // Then do other objectives
    ExecuteEfficientStrategy(bot, progress);
}

/**
 * @brief Execute speed completion strategy
 * @param bot Bot player
 * @param progress Quest progress data
 */
void QuestCompletion::ExecuteSpeedStrategy(Player* bot, QuestProgressData& progress)
{
    if (!bot)
        return;

    // Execute multiple objectives in parallel if possible
    uint32 activeObjectives = 0;

    for (auto& objective : progress.objectives)
    {
        if (objective.status == ObjectiveStatus::IN_PROGRESS)
            activeObjectives++;
    }

    if (activeObjectives < MAX_CONCURRENT_OBJECTIVES)
    {
        for (auto& objective : progress.objectives)
        {
            if (objective.status == ObjectiveStatus::NOT_STARTED)
            {
                ExecuteObjective(bot, objective);
                activeObjectives++;

                if (activeObjectives >= MAX_CONCURRENT_OBJECTIVES)
                    break;
            }
        }
    }
}

/**
 * @brief Execute exploration strategy
 * @param bot Bot player
 * @param progress Quest progress data
 */
void QuestCompletion::ExecuteExplorationStrategy(Player* bot, QuestProgressData& progress)
{
    if (!bot)
        return;

    // Explore widely while completing objectives
    for (auto& objective : progress.objectives)
    {
        if (objective.status != ObjectiveStatus::COMPLETED)
        {
            // Increase search radius for exploration
            objective.searchRadius = OBJECTIVE_SEARCH_RADIUS * 1.5f;
            ExecuteObjective(bot, objective);
            return;
        }
    }
}


bool QuestCompletion::FindObjectiveTarget(Player* player, QuestObjectiveData& objective)
{
    if (!player)
        return false;

    // Find target for the specified quest objective
    TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Finding objective target for player {}", player->GetName());
    return false;
}

// ============================================================================
// EVENTBUS CALLBACK HANDLERS
// ============================================================================

/**
 * @brief Handle quest events from EventBus (callback subscription)
 *
 * Dispatches quest events to specific handlers based on event type.
 * Called by EventBus during ProcessEvents() via callback.
 *
 * @param event Quest event from packet sniffer
 */
void QuestCompletion::HandleQuestEvent(QuestEvent const& event)
{
    // Validate event is for our bot
    if (!_bot || event.playerGuid != _bot->GetGUID())
        return;

    // Check if bot is paused (due to death)
    // CRITICAL FIX: Use shared_lock for read-only check (allows concurrent readers)
    {
        std::shared_lock<std::shared_mutex> lock(_pausedBotsMutex);
        if (_pausedBots.find(_bot->GetGUID().GetCounter()) != _pausedBots.end())
        {
            TC_LOG_TRACE("playerbot.quest", "QuestCompletion: Bot {} paused, skipping event {}",
                _bot->GetName(), static_cast<uint8>(event.type));
            return;
        }
    }

    // Dispatch to specific handler based on event type
    switch (event.type)
    {
        case QuestEventType::QUEST_CREDIT_ADDED:
            OnQuestCreditAdded(event);
            break;

        case QuestEventType::QUEST_COMPLETED:
            OnQuestCompleted(event);
            break;

        case QuestEventType::QUEST_FAILED:
            OnQuestFailed(event);
            break;

        case QuestEventType::QUEST_POI_RECEIVED:
            OnQuestPOIReceived(event);
            break;

        case QuestEventType::QUEST_OFFER_REWARD:
            OnQuestOfferReward(event);
            break;

        default:
            // Unhandled event type - this shouldn't happen if subscription is correct
            TC_LOG_WARN("playerbot.quest", "QuestCompletion: Unhandled event type {} for bot {}",
                static_cast<uint8>(event.type), _bot->GetName());
            break;
    }
}

/**
 * @brief Handle QUEST_CREDIT_ADDED event - real-time objective progress
 *
 * Updates the progress cache with exact counts from packet sniffer.
 * Eliminates need for DB polling - provides <5μs progress lookups.
 *
 * @param event Event containing questId, objectiveId, and exact count from packet
 */
void QuestCompletion::OnQuestCreditAdded(QuestEvent const& event)
{
    if (!_bot)
        return;

    Quest const* quest = sObjectMgr->GetQuestTemplate(event.questId);
    if (!quest)
        return;

    // Calculate progress from event data (no DB query needed!)
    QuestObjectives const& objectives = quest->GetObjectives();
    float totalProgress = 0.0f;
    uint32 objCount = 0;

    uint16 slot = _bot->FindQuestSlot(event.questId);
    if (slot >= MAX_QUEST_LOG_SIZE)
        return;

    for (auto const& obj : objectives)
    {
        int32 current;
        if (obj.ObjectID == static_cast<int32>(event.objectiveId))
        {
            // Use packet data directly for the updated objective
            current = static_cast<int32>(event.objectiveCount);
        }
        else
        {
            // Get other objectives from player's quest slot data
            // TrinityCore 12.0 API: GetQuestSlotObjectiveData takes QuestObjective const&
            current = _bot->GetQuestSlotObjectiveData(slot, obj);
        }

        if (obj.Amount > 0)
        {
            float objProg = std::min(1.0f, static_cast<float>(std::max(0, current)) / static_cast<float>(obj.Amount));
            totalProgress += objProg;
            ++objCount;
        }
    }

    float progress = objCount > 0 ? totalProgress / objCount : 0.0f;

    // Update cache with real-time progress
    UpdateProgressCache(_bot->GetGUID(), event.questId, progress);

    TC_LOG_DEBUG("playerbot.quest", "OnQuestCreditAdded: Bot {} quest {} objective {} count {} = {:.1f}%",
        _bot->GetName(), event.questId, event.objectiveId, event.objectiveCount, progress * 100.0f);

    // Also update internal tracking if we're tracking this quest
    auto it = _botQuestProgress.find(_bot->GetGUID().GetCounter());
    if (it != _botQuestProgress.end())
    {
        auto questIt = std::find_if(it->second.begin(), it->second.end(),
            [&event](QuestProgressData const& data) { return data.questId == event.questId; });

        if (questIt != it->second.end())
        {
            questIt->completionPercentage = progress * 100.0f;
            questIt->lastUpdateTime = GameTime::GetGameTimeMS();
            questIt->isStuck = false;  // Progress means not stuck

            // Update specific objective if found
            for (auto& objective : questIt->objectives)
            {
                if (objective.targetId == event.objectiveId)
                {
                    objective.currentCount = event.objectiveCount;
                    if (objective.currentCount >= objective.requiredCount)
                    {
                        objective.status = ObjectiveStatus::COMPLETED;
                        _globalMetrics.objectivesCompleted++;
                        _botMetrics[_bot->GetGUID().GetCounter()].objectivesCompleted++;
                    }
                    break;
                }
            }
        }
    }
}

/**
 * @brief Handle QUEST_COMPLETED event - trigger auto turn-in
 *
 * Updates cache to 100% and queues turn-in action.
 *
 * @param event Event containing questId of completed quest
 */
void QuestCompletion::OnQuestCompleted(QuestEvent const& event)
{
    if (!_bot)
        return;

    // Update cache to 100%
    UpdateProgressCache(_bot->GetGUID(), event.questId, 1.0f);

    TC_LOG_DEBUG("playerbot.quest", "OnQuestCompleted: Bot {} quest {} completed - queueing turn-in",
        _bot->GetName(), event.questId);

    // Update internal tracking
    auto it = _botQuestProgress.find(_bot->GetGUID().GetCounter());
    if (it != _botQuestProgress.end())
    {
        auto questIt = std::find_if(it->second.begin(), it->second.end(),
            [&event](QuestProgressData const& data) { return data.questId == event.questId; });

        if (questIt != it->second.end())
        {
            questIt->completionPercentage = 100.0f;
            questIt->requiresTurnIn = true;
            questIt->lastUpdateTime = GameTime::GetGameTimeMS();

            // Mark all objectives complete
            for (auto& objective : questIt->objectives)
            {
                if (objective.status != ObjectiveStatus::COMPLETED)
                {
                    objective.status = ObjectiveStatus::COMPLETED;
                    objective.currentCount = objective.requiredCount;
                }
            }
        }
    }

    // Queue turn-in action through QuestTurnIn system
    if (FindQuestTurnInNpc(_bot, event.questId))
    {
        ProcessQuestTurnIn(_bot, event.questId);
    }
}

/**
 * @brief Handle QUEST_FAILED event - trigger recovery
 *
 * Initiates recovery process for failed quests.
 *
 * @param event Event containing questId of failed quest
 */
void QuestCompletion::OnQuestFailed(QuestEvent const& event)
{
    if (!_bot)
        return;

    TC_LOG_WARN("playerbot.quest", "OnQuestFailed: Bot {} quest {} failed - initiating recovery",
        _bot->GetName(), event.questId);

    // Invalidate cache for this quest
    InvalidateProgressCache(_bot->GetGUID(), event.questId);

    // Update metrics
    _globalMetrics.questsFailed++;
    _botMetrics[_bot->GetGUID().GetCounter()].questsFailed++;

    // Update internal tracking
    auto it = _botQuestProgress.find(_bot->GetGUID().GetCounter());
    if (it != _botQuestProgress.end())
    {
        auto questIt = std::find_if(it->second.begin(), it->second.end(),
            [&event](QuestProgressData const& data) { return data.questId == event.questId; });

        if (questIt != it->second.end())
        {
            questIt->consecutiveFailures++;
            questIt->isStuck = true;
            questIt->stuckTime = GameTime::GetGameTimeMS();
        }
    }

    // Trigger recovery
    RecoverFromStuckState(_bot, event.questId);
}

/**
 * @brief Handle QUEST_POI_RECEIVED event - cache objective locations
 *
 * Stores server-provided POI data for navigation optimization.
 * Replaces expensive spatial queries with server-provided locations.
 *
 * @param event Event containing quest POI data from server
 */
void QuestCompletion::OnQuestPOIReceived(QuestEvent const& event)
{
    if (!_bot)
        return;

    TC_LOG_DEBUG("playerbot.quest", "OnQuestPOIReceived: Bot {} quest {} POI data received",
        _bot->GetName(), event.questId);

    // Cache POI data for FindEfficientCompletionPath()
    CacheQuestPOIData(_bot->GetGUID(), event.questId);
}

/**
 * @brief Handle QUEST_OFFER_REWARD event - auto-trigger turn-in flow
 *
 * Automatically processes quest turn-in when reward UI is shown.
 *
 * @param event Event indicating quest reward is being offered
 */
void QuestCompletion::OnQuestOfferReward(QuestEvent const& event)
{
    if (!_bot)
        return;

    TC_LOG_DEBUG("playerbot.quest", "OnQuestOfferReward: Bot {} quest {} reward offered - auto-completing",
        _bot->GetName(), event.questId);

    // Auto-complete the quest dialog
    CompleteQuestDialog(_bot, event.questId);
}

// ============================================================================
// EVENTBUS PROGRESS CACHE METHODS
// ============================================================================

/**
 * @brief Get cached progress for a quest (populated by EventBus events)
 *
 * @param botGuid Bot's GUID
 * @param questId Quest ID to look up
 * @return Optional cached progress if available and fresh (< 5 seconds old)
 */
std::optional<CachedQuestProgress> QuestCompletion::GetCachedQuestProgress(ObjectGuid botGuid, uint32 questId) const
{
    std::shared_lock lock(_progressCacheMutex);

    auto botIt = _questProgressCache.find(botGuid.GetCounter());
    if (botIt == _questProgressCache.end())
        return std::nullopt;

    auto questIt = botIt->second.find(questId);
    if (questIt == botIt->second.end())
        return std::nullopt;

    // Check if cache entry is still fresh (5 second TTL)
    if (!questIt->second.IsFresh())
        return std::nullopt;

    return questIt->second;
}

/**
 * @brief Update progress cache with new data
 *
 * @param botGuid Bot's GUID
 * @param questId Quest ID
 * @param progress Progress value (0.0 to 1.0)
 */
void QuestCompletion::UpdateProgressCache(ObjectGuid botGuid, uint32 questId, float progress)
{
    std::unique_lock lock(_progressCacheMutex);

    _questProgressCache[botGuid.GetCounter()][questId] = CachedQuestProgress(
        progress,
        GameTime::GetGameTimeMS(),
        questId
    );

    TC_LOG_TRACE("playerbot.quest", "UpdateProgressCache: Bot {} quest {} = {:.1f}%",
        botGuid.ToString(), questId, progress * 100.0f);
}

/**
 * @brief Cache POI data for a quest
 *
 * Queries server for quest POI locations and stores them for navigation optimization.
 *
 * @param botGuid Bot's GUID
 * @param questId Quest ID
 */
void QuestCompletion::CacheQuestPOIData(ObjectGuid botGuid, uint32 questId)
{
    std::unique_lock lock(_poiCacheMutex);

    CachedQuestPOI poiData;
    poiData.questId = questId;
    poiData.lastUpdateTime = GameTime::GetGameTimeMS();

    // Get POI data from ObjectMgr (TrinityCore 12.0 API: QuestPOIData)
    QuestPOIData const* questPOI = sObjectMgr->GetQuestPOIData(questId);
    if (questPOI)
    {
        for (auto const& blob : questPOI->Blobs)
        {
            for (auto const& point : blob.Points)
            {
                Position pos;
                pos.m_positionX = static_cast<float>(point.X);
                pos.m_positionY = static_cast<float>(point.Y);
                pos.m_positionZ = static_cast<float>(point.Z);
                pos.SetOrientation(0.0f);
                poiData.objectiveLocations.push_back(pos);
            }
        }
    }

    _questPOICache[botGuid.GetCounter()][questId] = poiData;

    TC_LOG_DEBUG("playerbot.quest", "CacheQuestPOIData: Bot {} quest {} cached {} locations",
        botGuid.ToString(), questId, poiData.objectiveLocations.size());
}

/**
 * @brief Invalidate cache for a quest (on completion or abandonment)
 *
 * @param botGuid Bot's GUID
 * @param questId Quest ID
 */
void QuestCompletion::InvalidateProgressCache(ObjectGuid botGuid, uint32 questId)
{
    {
        std::unique_lock lock(_progressCacheMutex);
        auto botIt = _questProgressCache.find(botGuid.GetCounter());
        if (botIt != _questProgressCache.end())
        {
            botIt->second.erase(questId);
        }
    }

    {
        std::unique_lock lock(_poiCacheMutex);
        auto botIt = _questPOICache.find(botGuid.GetCounter());
        if (botIt != _questPOICache.end())
        {
            botIt->second.erase(questId);
        }
    }

    TC_LOG_DEBUG("playerbot.quest", "InvalidateProgressCache: Bot {} quest {} cache invalidated",
        botGuid.ToString(), questId);
}

/**
 * @brief Get cached POI data for a quest
 *
 * @param botGuid Bot's GUID
 * @param questId Quest ID
 * @return Optional cached POI data if available
 */
std::optional<CachedQuestPOI> QuestCompletion::GetCachedQuestPOI(ObjectGuid botGuid, uint32 questId) const
{
    std::shared_lock lock(_poiCacheMutex);

    auto botIt = _questPOICache.find(botGuid.GetCounter());
    if (botIt == _questPOICache.end())
        return std::nullopt;

    auto questIt = botIt->second.find(questId);
    if (questIt == botIt->second.end())
        return std::nullopt;

    return questIt->second;
}

/**
 * @brief Pause quest completion (called on death via hook)
 *
 * @param botGuid Bot's GUID counter
 */
void QuestCompletion::PauseQuestCompletion(uint32 botGuid)
{
    // WRITE operation - requires exclusive lock
    std::unique_lock<std::shared_mutex> lock(_pausedBotsMutex);
    _pausedBots.insert(botGuid);

    TC_LOG_DEBUG("playerbot.quest", "PauseQuestCompletion: Bot {} paused", botGuid);
}

/**
 * @brief Resume quest completion (called on resurrection via hook)
 *
 * @param botGuid Bot's GUID counter
 */
void QuestCompletion::ResumeQuestCompletion(uint32 botGuid)
{
    // WRITE operation - requires exclusive lock
    std::unique_lock<std::shared_mutex> lock(_pausedBotsMutex);
    _pausedBots.erase(botGuid);

    TC_LOG_DEBUG("playerbot.quest", "ResumeQuestCompletion: Bot {} resumed", botGuid);
}
void QuestCompletion::CoordinateGroupQuestCompletion(Group* group, uint32 questId)
{
    if (!group || questId == 0)
        return;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return;

    TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Coordinating group quest {} completion", questId);

    // Collect all members who have this quest
    std::vector<Player*> membersWithQuest;
    membersWithQuest.reserve(group->GetMembersCount());

    for (GroupReference const& itr : group->GetMembers())
    {
        Player* member = itr.GetSource();
        if (!member)
            continue;

        QuestStatus status = member->GetQuestStatus(questId);
        if (status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE)
            membersWithQuest.push_back(member);
    }

    if (membersWithQuest.empty())
        return;

    // Analyze objectives and coordinate
    QuestObjectives const& objectives = quest->GetObjectives();
    for (auto const& objective : objectives)
    {
        switch (objective.Type)
        {
            case QUEST_OBJECTIVE_MONSTER:
            {
                // Kill objectives - share credit, coordinate target selection
                // Find who needs this objective most
                Player* neediest = nullptr;
                int32 lowestProgress = INT32_MAX;

                for (Player* member : membersWithQuest)
                {
                    uint16 slot = member->FindQuestSlot(questId);
                    if (slot >= MAX_QUEST_LOG_SIZE)
                        continue;

                    int32 current = member->GetQuestSlotObjectiveData(slot, objective);
                    int32 needed = static_cast<int32>(objective.Amount) - current;
                    if (needed > 0 && current < lowestProgress)
                    {
                        lowestProgress = current;
                        neediest = member;
                    }
                }

                if (neediest)
                {
                    TC_LOG_DEBUG("playerbot.quest", "Group coordination: {} needs most kills for objective {}",
                        neediest->GetName(), objective.ObjectID);
                }
                break;
            }

            case QUEST_OBJECTIVE_ITEM:
            {
                // Loot objectives - coordinate loot distribution
                // Track who needs items most for fair distribution
                std::vector<std::pair<Player*, int32>> needList;
                for (Player* member : membersWithQuest)
                {
                    uint16 slot = member->FindQuestSlot(questId);
                    if (slot >= MAX_QUEST_LOG_SIZE)
                        continue;

                    int32 current = member->GetQuestSlotObjectiveData(slot, objective);
                    int32 needed = static_cast<int32>(objective.Amount) - current;
                    if (needed > 0)
                        needList.emplace_back(member, needed);
                }

                // Sort by need (descending)
                std::sort(needList.begin(), needList.end(),
                    [](auto const& a, auto const& b) { return a.second > b.second; });

                // Store loot priority for this objective
                if (!needList.empty())
                {
                    std::lock_guard<std::mutex> lock(_groupCoordinationMutex);
                    _groupLootPriority[group->GetGUID().GetCounter()][objective.ObjectID] =
                        needList[0].first->GetGUID().GetCounter();
                }
                break;
            }

            case QUEST_OBJECTIVE_GAMEOBJECT:
            {
                // Interaction objectives - queue interaction order
                // Closest member interacts first
                break;
            }

            default:
                break;
        }
    }

    // Update coordination state
    {
        std::lock_guard<std::mutex> lock(_groupCoordinationMutex);
        _groupQuestCoordination[group->GetGUID().GetCounter()][questId] = {
            GameTime::GetGameTimeMS(),
            static_cast<uint32>(membersWithQuest.size()),
            true
        };
    }
}
void QuestCompletion::SynchronizeGroupObjectives(Group* group, uint32 questId)
{
    if (!group || questId == 0)
        return;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return;

    TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Synchronizing group objectives for quest {}", questId);

    // Find the member with lowest progress to pace the group
    Player* slowestMember = nullptr;
    float lowestProgress = 1.0f;

    for (GroupReference const& itr : group->GetMembers())
    {
        Player* member = itr.GetSource();
        if (!member)
            continue;

        QuestStatus status = member->GetQuestStatus(questId);
        if (status != QUEST_STATUS_INCOMPLETE)
            continue;

        float progress = CalculateQuestProgress(questId, member);
        if (progress < lowestProgress)
        {
            lowestProgress = progress;
            slowestMember = member;
        }
    }

    if (!slowestMember)
        return;

    // Identify which objective the slowest member needs help with
    QuestObjectives const& objectives = quest->GetObjectives();
    uint16 slot = slowestMember->FindQuestSlot(questId);
    if (slot >= MAX_QUEST_LOG_SIZE)
        return;

    for (auto const& objective : objectives)
    {
        int32 current = slowestMember->GetQuestSlotObjectiveData(slot, objective);
        if (current < static_cast<int32>(objective.Amount))
        {
            // This objective needs work - update sync time
            {
                std::lock_guard<std::mutex> lock(_groupCoordinationMutex);
                _groupObjectiveSync[group->GetGUID().GetCounter()][questId] = GameTime::GetGameTimeMS();
            }

            TC_LOG_DEBUG("playerbot.quest", "Group sync: {} needs help with objective {} ({}/{})",
                slowestMember->GetName(), objective.ObjectID, current, objective.Amount);
            break;
        }
    }
}
void QuestCompletion::HandleGroupObjectiveConflict(Group* group, uint32 questId, uint32 objectiveIndex)
{
    if (!group || questId == 0)
        return;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return;

    QuestObjectives const& objectives = quest->GetObjectives();
    if (objectiveIndex >= objectives.size())
        return;

    auto const& objective = objectives[objectiveIndex];

    TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Handling group objective conflict quest {} obj {}",
        questId, objectiveIndex);

    // Conflict resolution based on objective type
    switch (objective.Type)
    {
        case QUEST_OBJECTIVE_ITEM:
        {
            // Loot conflict - implement round-robin assignment
            std::vector<std::pair<Player*, int32>> needers;

            for (GroupReference const& itr : group->GetMembers())
            {
                Player* member = itr.GetSource();
                if (!member)
                    continue;

                uint16 slot = member->FindQuestSlot(questId);
                if (slot >= MAX_QUEST_LOG_SIZE)
                    continue;

                int32 current = member->GetQuestSlotObjectiveData(slot, objective);
                int32 needed = static_cast<int32>(objective.Amount) - current;
                if (needed > 0)
                    needers.emplace_back(member, current);
            }

            if (!needers.empty())
            {
                // Sort by lowest current count (need most)
                std::sort(needers.begin(), needers.end(),
                    [](auto const& a, auto const& b) { return a.second < b.second; });

                // Set priority player for this item
                {
                    std::lock_guard<std::mutex> lock(_groupCoordinationMutex);
                    _groupLootPriority[group->GetGUID().GetCounter()][objective.ObjectID] =
                        needers[0].first->GetGUID().GetCounter();
                }

                TC_LOG_DEBUG("playerbot.quest", "Loot priority for item {} set to {}",
                    objective.ObjectID, needers[0].first->GetName());
            }
            break;
        }

        case QUEST_OBJECTIVE_GAMEOBJECT:
        {
            // Interaction conflict - implement distance-based queue
            // Closest member gets first interaction priority
            std::vector<std::pair<Player*, float>> distanceList;

            // Get average location of the objective (from objective targets)
            Position avgPos;
            bool hasPos = false;

            for (GroupReference const& itr : group->GetMembers())
            {
                Player* member = itr.GetSource();
                if (!member)
                    continue;

                // Track members who need this objective
                uint16 slot = member->FindQuestSlot(questId);
                if (slot >= MAX_QUEST_LOG_SIZE)
                    continue;

                int32 current = member->GetQuestSlotObjectiveData(slot, objective);
                if (current >= static_cast<int32>(objective.Amount))
                    continue;

                // For now, use member position as reference
                if (!hasPos)
                {
                    avgPos = member->GetPosition();
                    hasPos = true;
                }

                distanceList.emplace_back(member, 0.0f); // Distance will be computed when GO found
            }

            TC_LOG_DEBUG("playerbot.quest", "Interaction queue for objective {} has {} members",
                objective.ObjectID, distanceList.size());
            break;
        }

        default:
            // Kill objectives typically don't conflict - shared credit in groups
            TC_LOG_DEBUG("playerbot.quest", "No conflict resolution needed for objective type {}",
                static_cast<uint32>(objective.Type));
            break;
    }
}
void QuestCompletion::OptimizeQuestCompletionOrder(Player* player)
{
    if (!player)
        return;

    std::vector<uint32> activeQuests = GetActiveQuests(player);
    if (activeQuests.size() <= 1)
    {
        TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: {} has {} quests, no optimization needed",
            player->GetName(), activeQuests.size());
        return;
    }

    TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Optimizing quest completion order for {} ({} quests)",
        player->GetName(), activeQuests.size());

    // Build quest priority list with multiple factors
    struct QuestPriorityInfo
    {
        uint32 questId;
        float progress;
        int32 priority;
        bool hasNearbyObjectives;
    };

    std::vector<QuestPriorityInfo> questPriorities;
    questPriorities.reserve(activeQuests.size());

    for (uint32 questId : activeQuests)
    {
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            continue;

        QuestPriorityInfo info;
        info.questId = questId;
        info.progress = CalculateQuestProgress(questId, player);
        info.priority = CalculateQuestPriorityInternal(player, quest);

        // Check if any objectives are nearby (within 100 yards)
        info.hasNearbyObjectives = false;
        QuestObjectives const& objectives = quest->GetObjectives();
        uint16 slot = player->FindQuestSlot(questId);

        if (slot < MAX_QUEST_LOG_SIZE)
        {
            for (auto const& objective : objectives)
            {
                int32 current = player->GetQuestSlotObjectiveData(slot, objective);
                if (current < static_cast<int32>(objective.Amount))
                {
                    // Check POI cache for nearby locations
                    auto poiCache = GetCachedQuestPOI(player->GetGUID(), questId);
                    if (poiCache.has_value())
                    {
                        for (auto const& poi : poiCache->objectiveLocations)
                        {
                            float dist = player->GetDistance(poi);
                            if (dist < 100.0f)
                            {
                                info.hasNearbyObjectives = true;
                                break;
                            }
                        }
                    }
                    if (info.hasNearbyObjectives)
                        break;
                }
            }
        }

        questPriorities.push_back(info);
    }

    // Sort by: nearby objectives first, then by progress (higher first), then by priority
    std::sort(questPriorities.begin(), questPriorities.end(),
        [](QuestPriorityInfo const& a, QuestPriorityInfo const& b) {
            if (a.hasNearbyObjectives != b.hasNearbyObjectives)
                return a.hasNearbyObjectives;
            if (std::abs(a.progress - b.progress) > 0.2f)
                return a.progress > b.progress;  // Prefer nearly complete
            return a.priority > b.priority;
        });

    // Update internal quest order cache
    std::vector<uint32> optimizedOrder;
    optimizedOrder.reserve(questPriorities.size());
    for (auto const& info : questPriorities)
    {
        optimizedOrder.push_back(info.questId);
    }

    {
        std::unique_lock<std::shared_mutex> lock(_questOrderMutex);
        _botQuestOrder[player->GetGUID().GetCounter()] = std::move(optimizedOrder);
    }

    TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Optimized {} quests for {}",
        questPriorities.size(), player->GetName());
}
void QuestCompletion::OptimizeObjectiveSequence(Player* player, uint32 questId)
{
    if (!player || questId == 0)
        return;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return;

    QuestObjectives const& objectives = quest->GetObjectives();
    if (objectives.size() <= 1)
    {
        TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Quest {} has {} objectives, no optimization needed",
            questId, objectives.size());
        return;
    }

    TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Optimizing objective sequence for quest {}", questId);

    // Build objective info with cached locations
    struct ObjectiveInfo
    {
        uint32 index;
        uint32 remaining;
        Position nearestLocation;
        float distance;
    };

    std::vector<ObjectiveInfo> objectiveInfos;
    uint16 slot = player->FindQuestSlot(questId);
    if (slot >= MAX_QUEST_LOG_SIZE)
        return;

    auto poiCache = GetCachedQuestPOI(player->GetGUID(), questId);

    for (uint32 i = 0; i < objectives.size(); ++i)
    {
        auto const& obj = objectives[i];
        int32 current = player->GetQuestSlotObjectiveData(slot, obj);

        if (current >= static_cast<int32>(obj.Amount))
            continue;  // Already complete

        ObjectiveInfo info;
        info.index = i;
        info.remaining = obj.Amount - static_cast<uint32>(std::max(0, current));

        // Get nearest location from POI cache
        if (poiCache.has_value() && !poiCache->objectiveLocations.empty())
        {
            // Find closest POI point
            float minDist = std::numeric_limits<float>::max();
            for (auto const& poi : poiCache->objectiveLocations)
            {
                float dist = player->GetDistance(poi);
                if (dist < minDist)
                {
                    minDist = dist;
                    info.nearestLocation = poi;
                }
            }
            info.distance = minDist;
        }
        else
        {
            // No POI data - use large default distance
            info.nearestLocation = player->GetPosition();
            info.distance = 1000.0f;
        }

        objectiveInfos.push_back(info);
    }

    if (objectiveInfos.empty())
        return;

    // Traveling Salesman approximation - nearest neighbor heuristic
    std::vector<uint32> optimizedOrder;
    optimizedOrder.reserve(objectiveInfos.size());
    Position currentPos = player->GetPosition();

    while (!objectiveInfos.empty())
    {
        // Find nearest objective to current position
        auto nearest = std::min_element(objectiveInfos.begin(), objectiveInfos.end(),
            [&currentPos](ObjectiveInfo const& a, ObjectiveInfo const& b) {
                float distA = currentPos.GetExactDist(&a.nearestLocation);
                float distB = currentPos.GetExactDist(&b.nearestLocation);
                return distA < distB;
            });

        optimizedOrder.push_back(nearest->index);
        currentPos = nearest->nearestLocation;
        objectiveInfos.erase(nearest);
    }

    // Store optimized sequence
    {
        std::unique_lock<std::shared_mutex> lock(_objectiveOrderMutex);
        _botObjectiveOrder[player->GetGUID().GetCounter()][questId] = std::move(optimizedOrder);
    }

    TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Optimized {} objectives for quest {}",
        optimizedOrder.size(), questId);
}
void QuestCompletion::FindEfficientCompletionPath(Player* player, ::std::vector<uint32> const& questIds)
{
    if (!player || questIds.empty())
        return;

    TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Finding efficient path for {} quests", questIds.size());

    // Collect all incomplete objectives across all quests
    struct WaypointInfo
    {
        uint32 questId;
        uint32 objectiveIndex;
        Position location;
    };

    std::vector<WaypointInfo> waypoints;
    waypoints.reserve(questIds.size() * 3);  // Estimate 3 objectives per quest

    for (uint32 questId : questIds)
    {
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            continue;

        QuestObjectives const& objectives = quest->GetObjectives();
        uint16 slot = player->FindQuestSlot(questId);
        if (slot >= MAX_QUEST_LOG_SIZE)
            continue;

        auto poiCache = GetCachedQuestPOI(player->GetGUID(), questId);

        for (uint32 i = 0; i < objectives.size(); ++i)
        {
            auto const& obj = objectives[i];
            int32 current = player->GetQuestSlotObjectiveData(slot, obj);

            if (current >= static_cast<int32>(obj.Amount))
                continue;

            // Get locations from POI cache
            if (poiCache.has_value() && !poiCache->objectiveLocations.empty())
            {
                for (auto const& loc : poiCache->objectiveLocations)
                {
                    waypoints.push_back({ questId, i, loc });
                }
            }
        }
    }

    if (waypoints.empty())
    {
        TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: No waypoints found for path optimization");
        return;
    }

    // Cluster nearby waypoints (within 50 yards)
    constexpr float CLUSTER_RADIUS = 50.0f;
    std::vector<std::vector<WaypointInfo>> clusters;

    for (auto const& wp : waypoints)
    {
        bool addedToCluster = false;
        for (auto& cluster : clusters)
        {
            if (!cluster.empty())
            {
                float dist = cluster[0].location.GetExactDist(&wp.location);
                if (dist < CLUSTER_RADIUS)
                {
                    cluster.push_back(wp);
                    addedToCluster = true;
                    break;
                }
            }
        }

        if (!addedToCluster)
        {
            clusters.push_back({ wp });
        }
    }

    // Order clusters by distance (nearest neighbor heuristic)
    std::vector<Position> pathPoints;
    pathPoints.reserve(clusters.size());
    Position currentPos = player->GetPosition();

    while (!clusters.empty())
    {
        // Find nearest cluster centroid
        auto nearest = std::min_element(clusters.begin(), clusters.end(),
            [&currentPos](std::vector<WaypointInfo> const& a, std::vector<WaypointInfo> const& b) {
                // Use first point as cluster center (simplified)
                float distA = currentPos.GetExactDist(&a[0].location);
                float distB = currentPos.GetExactDist(&b[0].location);
                return distA < distB;
            });

        // Add cluster center to path
        pathPoints.push_back(nearest->at(0).location);
        currentPos = nearest->at(0).location;
        clusters.erase(nearest);
    }

    // Store computed path
    {
        std::lock_guard<std::mutex> lock(_pathMutex);
        _botQuestPaths[player->GetGUID().GetCounter()] = std::move(pathPoints);
    }

    TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Computed path with {} waypoints",
        pathPoints.size());
}
void QuestCompletion::MinimizeTravelTime(Player* player, ::std::vector<QuestObjectiveData> const& objectives)
{
    if (!player || objectives.empty())
        return;

    TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Minimizing travel time for {} objectives",
        objectives.size());

    // Sort objectives by distance from player, then cluster nearby ones
    std::vector<std::pair<size_t, float>> distanceIndexPairs;
    distanceIndexPairs.reserve(objectives.size());

    for (size_t i = 0; i < objectives.size(); ++i)
    {
        auto const& obj = objectives[i];
        if (obj.status == ObjectiveStatus::COMPLETED)
            continue;

        float dist = player->GetDistance(obj.targetLocation);
        distanceIndexPairs.emplace_back(i, dist);
    }

    // Sort by distance
    std::sort(distanceIndexPairs.begin(), distanceIndexPairs.end(),
        [](auto const& a, auto const& b) { return a.second < b.second; });

    // Reorder objectives based on distance
    std::vector<uint32> optimizedOrder;
    optimizedOrder.reserve(distanceIndexPairs.size());

    for (auto const& pair : distanceIndexPairs)
    {
        optimizedOrder.push_back(static_cast<uint32>(pair.first));
    }

    TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Travel optimization complete, {} objectives reordered",
        optimizedOrder.size());
}
/**
 * @brief Handle a stuck objective with progressive recovery strategies
 *
 * This function implements a multi-tier recovery system for objectives that have
 * stalled. Each retry attempts increasingly aggressive strategies:
 *
 * Strategy Progression:
 * 1. Retry 1-3: Try alternative targets if available
 * 2. Retry 4-5: Expand search radius by 50% per retry
 * 3. Retry 6-7: Move to alternative search area
 * 4. Retry 8+: Mark as blocked for manual intervention or skip if optional
 *
 * Performance: O(1) for most operations, O(n) for alternative area search
 *
 * @param player The bot player
 * @param objective The stuck objective data (modified in place)
 */
void QuestCompletion::HandleStuckObjective(Player* player, QuestObjectiveData& objective)
{
    if (!player)
        return;

    objective.retryCount++;
    uint32 const currentTime = GameTime::GetGameTimeMS();

    TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Handling stuck objective for {} - "
        "Quest {}, Objective {}, Retry {} (Type: {})",
        player->GetName(), objective.questId, objective.objectiveIndex,
        objective.retryCount, static_cast<uint8>(objective.type));

    // Record time spent on this objective for metrics
    objective.timeSpent = currentTime - objective.lastUpdateTime;

    // ========================================================================
    // STRATEGY 1: Try alternative targets (Retry 1-3)
    // ========================================================================
    if (objective.retryCount <= 3 && !objective.alternativeTargets.empty())
    {
        // Cycle through alternative targets
        uint32 const altIndex = (objective.retryCount - 1) % objective.alternativeTargets.size();
        uint32 const previousTarget = objective.targetId;
        objective.targetId = objective.alternativeTargets[altIndex];
        objective.status = ObjectiveStatus::IN_PROGRESS;
        objective.lastUpdateTime = currentTime;

        TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Trying alternative target {} "
            "(was {}) for bot {}", objective.targetId, previousTarget, player->GetName());

        return;
    }

    // ========================================================================
    // STRATEGY 2: Expand search radius (Retry 4-5)
    // ========================================================================
    if (objective.retryCount <= 5)
    {
        float const previousRadius = objective.searchRadius;
        objective.searchRadius *= 1.5f;  // 50% increase per retry

        // Cap at maximum reasonable search radius (500 yards)
        if (objective.searchRadius > 500.0f)
            objective.searchRadius = 500.0f;

        objective.status = ObjectiveStatus::IN_PROGRESS;
        objective.lastUpdateTime = currentTime;

        TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Expanded search radius from {:.1f} "
            "to {:.1f} for bot {}", previousRadius, objective.searchRadius, player->GetName());

        return;
    }

    // ========================================================================
    // STRATEGY 3: Move to alternative search area (Retry 6-7)
    // ========================================================================
    if (objective.retryCount <= 7)
    {
        // Calculate alternative search area based on objective type
        Position newSearchPos = FindAlternativeSearchArea(player, objective);

        if (newSearchPos.IsPositionValid())
        {
            // Update objective target location to new area
            objective.targetLocation = newSearchPos;
            objective.status = ObjectiveStatus::IN_PROGRESS;
            objective.lastUpdateTime = currentTime;

            // Request movement to new area through navigation system
            NavigateToObjective(player, objective);

            TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Moving bot {} to alternative "
                "search area ({:.1f}, {:.1f}, {:.1f})",
                player->GetName(), newSearchPos.GetPositionX(),
                newSearchPos.GetPositionY(), newSearchPos.GetPositionZ());

            return;
        }
        else
        {
            TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: No alternative search area found "
                "for bot {}", player->GetName());
        }
    }

    // ========================================================================
    // STRATEGY 4: Mark as blocked or skip (Retry 8+)
    // ========================================================================
    if (objective.isOptional)
    {
        // Optional objectives can be skipped
        SkipProblematicObjective(player, objective);
        return;
    }

    // Required objective - mark as blocked for potential manual intervention
    objective.status = ObjectiveStatus::BLOCKED;

    TC_LOG_WARN("playerbot.quest", "QuestCompletion: Required objective blocked after {} "
        "retries: Bot {} Quest {} Objective {}",
        objective.retryCount, player->GetName(), objective.questId, objective.objectiveIndex);

    // Update metrics for stuck tracking
    _globalMetrics.stuckInstances++;

    // Log to completion log for diagnostics
    auto it = _botQuestProgress.find(player->GetGUID().GetCounter());
    if (it != _botQuestProgress.end())
    {
        for (auto& progress : it->second)
        {
            if (progress.questId == objective.questId)
            {
                progress.isStuck = true;
                progress.stuckTime = currentTime;
                progress.consecutiveFailures++;
                progress.completionLog.push_back(
                    "Objective " + std::to_string(objective.objectiveIndex) +
                    " blocked after " + std::to_string(objective.retryCount) + " retries");
                break;
            }
        }
    }

    // Try to diagnose why we're stuck
    DiagnoseObjectiveIssue(player, objective);
}

/**
 * @brief Find an alternative search area for an objective
 *
 * Based on objective type, finds a new area to search that might have
 * the required targets.
 *
 * @param player The bot player
 * @param objective The stuck objective
 * @return Alternative position to search, or invalid position if none found
 */
Position QuestCompletion::FindAlternativeSearchArea(Player* player, QuestObjectiveData const& objective)
{
    if (!player)
        return Position();

    Quest const* quest = sObjectMgr->GetQuestTemplate(objective.questId);
    if (!quest)
        return Position();

    Position currentPos = player->GetPosition();
    Position alternativePos;

    // Try to get POI data from cache
    {
        std::shared_lock lock(_poiCacheMutex);
        auto botIt = _questPOICache.find(player->GetGUID().GetCounter());
        if (botIt != _questPOICache.end())
        {
            auto questIt = botIt->second.find(objective.questId);
            if (questIt != botIt->second.end() && !questIt->second.objectiveLocations.empty())
            {
                // Find the POI location furthest from current position (we've likely checked nearby)
                float maxDist = 0.0f;
                for (auto const& poi : questIt->second.objectiveLocations)
                {
                    float dist = currentPos.GetExactDist(&poi);
                    if (dist > maxDist && dist > objective.searchRadius)
                    {
                        maxDist = dist;
                        alternativePos = poi;
                    }
                }

                if (alternativePos.IsPositionValid())
                    return alternativePos;
            }
        }
    }

    // Fallback: Generate search position based on objective type
    switch (objective.type)
    {
        case QuestObjectiveType::KILL_CREATURE:
        case QuestObjectiveType::COLLECT_ITEM:
        {
            // For kill/collect objectives, try to find creature spawns in the zone
            if (objective.targetId > 0)
            {
                // Query creature spawns from database (cached in memory)
                auto const* creatureData = sObjectMgr->GetCreatureData(objective.targetId);
                if (creatureData)
                {
                    return Position(creatureData->spawnPoint);
                }
            }
            break;
        }

        case QuestObjectiveType::USE_GAMEOBJECT:
        {
            // For GO objectives, find gameobject spawns
            if (objective.targetId > 0)
            {
                auto const* goData = sObjectMgr->GetGameObjectData(objective.targetId);
                if (goData)
                {
                    return Position(goData->spawnPoint);
                }
            }
            break;
        }

        case QuestObjectiveType::TALK_TO_NPC:
        {
            // For talk objectives, find NPC spawn location
            if (objective.targetId > 0)
            {
                auto const* creatureData = sObjectMgr->GetCreatureData(objective.targetId);
                if (creatureData)
                {
                    return Position(creatureData->spawnPoint);
                }
            }
            break;
        }

        case QuestObjectiveType::REACH_LOCATION:
        {
            // For exploration objectives, the target location should already be set
            if (objective.targetLocation.IsPositionValid())
            {
                return objective.targetLocation;
            }
            break;
        }

        default:
            break;
    }

    // Last resort: Generate a random nearby position in a different direction
    float const angle = currentPos.GetOrientation() + float(M_PI);  // Opposite direction
    float const distance = objective.searchRadius * 2.0f;

    alternativePos = Position(
        currentPos.GetPositionX() + std::cos(angle) * distance,
        currentPos.GetPositionY() + std::sin(angle) * distance,
        currentPos.GetPositionZ(),
        currentPos.GetOrientation()
    );

    return alternativePos;
}

/**
 * @brief Diagnose why an objective is stuck
 *
 * Logs detailed diagnostic information to help identify the root cause
 * of an objective being stuck.
 *
 * @param player The bot player
 * @param objective The stuck objective
 */
void QuestCompletion::DiagnoseObjectiveIssue(Player* player, QuestObjectiveData const& objective)
{
    if (!player)
        return;

    std::vector<std::string> issues;

    // Check based on objective type
    switch (objective.type)
    {
        case QuestObjectiveType::KILL_CREATURE:
        {
            // Check if creature exists in world
            CreatureTemplate const* cTemplate = sObjectMgr->GetCreatureTemplate(objective.targetId);
            if (!cTemplate)
            {
                issues.push_back("Target creature template " + std::to_string(objective.targetId) + " not found");
            }
            else
            {
                // Check if any spawns exist nearby
                Creature* nearestCreature = player->FindNearestCreature(objective.targetId, objective.searchRadius);
                if (!nearestCreature)
                {
                    issues.push_back("No spawns of creature " + std::to_string(objective.targetId) +
                        " within " + std::to_string(objective.searchRadius) + " yards");
                }
                else if (!nearestCreature->IsAlive())
                {
                    issues.push_back("Nearest creature is dead, waiting for respawn");
                }
                else if (nearestCreature->IsInCombat())
                {
                    issues.push_back("Target creature is in combat with another player");
                }
            }
            break;
        }

        case QuestObjectiveType::COLLECT_ITEM:
        {
            // Check if item exists
            ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(objective.targetId);
            if (!itemTemplate)
            {
                issues.push_back("Target item " + std::to_string(objective.targetId) + " not found in database");
            }
            else
            {
                // Check if bot's inventory is full
                // Check if bot has inventory space using CanStoreNewItem
                ItemPosCountVec dest;
                if (player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, objective.targetId, 1) != EQUIP_ERR_OK)
                {
                    issues.push_back("Bot inventory is full - cannot collect items");
                }
            }
            break;
        }

        case QuestObjectiveType::USE_GAMEOBJECT:
        {
            // Check if GO exists
            GameObjectTemplate const* goTemplate = sObjectMgr->GetGameObjectTemplate(objective.targetId);
            if (!goTemplate)
            {
                issues.push_back("Target gameobject " + std::to_string(objective.targetId) + " not found");
            }
            else
            {
                GameObject* nearestGO = player->FindNearestGameObject(objective.targetId, objective.searchRadius);
                if (!nearestGO)
                {
                    issues.push_back("No spawns of gameobject " + std::to_string(objective.targetId) +
                        " within " + std::to_string(objective.searchRadius) + " yards");
                }
                else if (!nearestGO->isSpawned())
                {
                    issues.push_back("Gameobject not spawned, waiting for respawn");
                }
            }
            break;
        }

        case QuestObjectiveType::TALK_TO_NPC:
        {
            // Check if NPC exists and is accessible
            CreatureTemplate const* cTemplate = sObjectMgr->GetCreatureTemplate(objective.targetId);
            if (!cTemplate)
            {
                issues.push_back("Target NPC " + std::to_string(objective.targetId) + " not found");
            }
            else
            {
                Creature* nearestNpc = player->FindNearestCreature(objective.targetId, objective.searchRadius);
                if (!nearestNpc)
                {
                    issues.push_back("NPC " + std::to_string(objective.targetId) + " not found nearby");
                }
                else if (!nearestNpc->IsAlive())
                {
                    issues.push_back("Target NPC is dead");
                }
                else if (nearestNpc->IsInCombat())
                {
                    issues.push_back("Target NPC is in combat");
                }
            }
            break;
        }

        case QuestObjectiveType::REACH_LOCATION:
        {
            // Check if location is reachable
            if (!objective.targetLocation.IsPositionValid())
            {
                issues.push_back("Target location is invalid");
            }
            else
            {
                float dist = player->GetExactDist(&objective.targetLocation);
                if (dist > 1000.0f)
                {
                    issues.push_back("Target location is very far: " + std::to_string(dist) + " yards");
                }
            }
            break;
        }

        default:
            issues.push_back("Unhandled objective type: " + std::to_string(static_cast<uint8>(objective.type)));
            break;
    }

    // Check common issues
    if (player->IsInCombat())
    {
        issues.push_back("Bot is in combat");
    }
    if (player->isDead())
    {
        issues.push_back("Bot is dead");
    }
    if (player->GetMotionMaster()->GetCurrentMovementGeneratorType() == IDLE_MOTION_TYPE)
    {
        issues.push_back("Bot movement is idle - may be stuck");
    }

    // Log all issues
    if (issues.empty())
    {
        TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: No obvious issues found for stuck objective");
    }
    else
    {
        TC_LOG_WARN("playerbot.quest", "QuestCompletion: Diagnostic issues for bot {} quest {} objective {}:",
            player->GetName(), objective.questId, objective.objectiveIndex);
        for (auto const& issue : issues)
        {
            TC_LOG_WARN("playerbot.quest", "  - {}", issue);
        }
    }
}
/**
 * @brief Skip a problematic objective that cannot be completed
 *
 * This function marks an optional objective as skipped after multiple failed retry
 * attempts. It updates internal tracking, metrics, and advances to the next objective.
 *
 * **Important**: This should only be called for optional objectives. Required objectives
 * should remain BLOCKED for potential manual intervention.
 *
 * Side Effects:
 * - Updates objective status to SKIPPED
 * - Increments global stuck metrics
 * - Logs skip event to quest completion log
 * - Advances to next incomplete objective in the quest
 *
 * @param player The bot player
 * @param objective The objective to skip (modified in place)
 */
void QuestCompletion::SkipProblematicObjective(Player* player, QuestObjectiveData& objective)
{
    if (!player)
        return;

    uint32 const currentTime = GameTime::GetGameTimeMS();

    // Safety check: Only skip optional objectives
    if (!objective.isOptional)
    {
        TC_LOG_WARN("playerbot.quest", "QuestCompletion: Attempted to skip required objective - "
            "Bot {} Quest {} Objective {}. Marking as BLOCKED instead.",
            player->GetName(), objective.questId, objective.objectiveIndex);

        objective.status = ObjectiveStatus::BLOCKED;
        return;
    }

    TC_LOG_INFO("playerbot.quest", "QuestCompletion: Skipping optional objective - "
        "Bot {} Quest {} Objective {} (Type: {}, Retries: {})",
        player->GetName(), objective.questId, objective.objectiveIndex,
        static_cast<uint8>(objective.type), objective.retryCount);

    // Mark objective as skipped
    ObjectiveStatus const previousStatus = objective.status;
    objective.status = ObjectiveStatus::SKIPPED;
    objective.lastUpdateTime = currentTime;

    // Update global metrics
    _globalMetrics.stuckInstances++;

    // Update quest progress data
    uint32 const botGuidCounter = player->GetGUID().GetCounter();
    auto it = _botQuestProgress.find(botGuidCounter);
    if (it != _botQuestProgress.end())
    {
        for (auto& progress : it->second)
        {
            if (progress.questId == objective.questId)
            {
                // Log the skip event
                progress.completionLog.push_back(
                    "Skipped optional objective " + std::to_string(objective.objectiveIndex) +
                    " after " + std::to_string(objective.retryCount) + " retries" +
                    " (was " + GetObjectiveStatusName(previousStatus) + ")");

                // Update progress timestamp
                progress.lastUpdateTime = currentTime;

                // Find and activate the next incomplete objective
                bool foundNextObjective = false;
                for (auto& obj : progress.objectives)
                {
                    if (obj.status == ObjectiveStatus::NOT_STARTED ||
                        obj.status == ObjectiveStatus::IN_PROGRESS)
                    {
                        // Skip the current objective we just marked as skipped
                        if (obj.objectiveIndex == objective.objectiveIndex)
                            continue;

                        obj.status = ObjectiveStatus::IN_PROGRESS;
                        foundNextObjective = true;

                        TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Advancing to objective {} "
                            "for bot {} quest {}", obj.objectiveIndex, player->GetName(), progress.questId);
                        break;
                    }
                }

                // Check if all objectives are now complete or skipped
                if (!foundNextObjective)
                {
                    bool allComplete = true;
                    for (auto const& obj : progress.objectives)
                    {
                        if (obj.status != ObjectiveStatus::COMPLETED &&
                            obj.status != ObjectiveStatus::SKIPPED)
                        {
                            // Blocked required objectives prevent completion
                            if (!obj.isOptional)
                            {
                                allComplete = false;
                                break;
                            }
                        }
                    }

                    if (allComplete)
                    {
                        TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: All objectives complete/skipped "
                            "for bot {} quest {} - checking if quest can be turned in",
                            player->GetName(), progress.questId);

                        // Check actual quest status from player
                        QuestStatus questStatus = player->GetQuestStatus(progress.questId);
                        if (questStatus == QUEST_STATUS_COMPLETE)
                        {
                            progress.requiresTurnIn = true;
                            progress.completionLog.push_back("Quest ready for turn-in");
                        }
                    }
                }

                break;
            }
        }
    }

    // Reset any cached objective target state
    _botCurrentObjective.erase(botGuidCounter);
}

/**
 * @brief Get human-readable name for objective status
 *
 * @param status The objective status enum value
 * @return String representation of the status
 */
std::string QuestCompletion::GetObjectiveStatusName(ObjectiveStatus status)
{
    switch (status)
    {
        case ObjectiveStatus::NOT_STARTED:  return "NOT_STARTED";
        case ObjectiveStatus::IN_PROGRESS:  return "IN_PROGRESS";
        case ObjectiveStatus::COMPLETED:    return "COMPLETED";
        case ObjectiveStatus::FAILED:       return "FAILED";
        case ObjectiveStatus::BLOCKED:      return "BLOCKED";
        case ObjectiveStatus::SKIPPED:      return "SKIPPED";
        default:                            return "UNKNOWN";
    }
}
/**
 * @brief Process quest turn-in for a completed quest
 *
 * This function orchestrates the entire quest turn-in process:
 * 1. Validates quest is ready for turn-in
 * 2. Finds the turn-in NPC
 * 3. Navigates to NPC if not in range
 * 4. Selects optimal reward
 * 5. Completes the quest dialog
 *
 * Thread Safety: Uses internal mutexes for state management
 * Performance: O(n) where n is number of NPCs checked
 *
 * @param player The bot player
 * @param questId The quest ID to turn in
 */
void QuestCompletion::ProcessQuestTurnIn(Player* player, uint32 questId)
{
    if (!player || questId == 0)
        return;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
    {
        TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Quest {} not found", questId);
        return;
    }

    // Step 1: Validate quest is ready for turn-in
    QuestStatus status = player->GetQuestStatus(questId);
    if (status != QUEST_STATUS_COMPLETE)
    {
        TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Quest {} not ready for turn-in (status: {})",
            questId, static_cast<uint32>(status));
        return;
    }

    TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Processing turn-in for bot {} quest {}",
        player->GetName(), questId);

    // Step 2: Find the turn-in NPC
    if (!FindQuestTurnInNpc(player, questId))
    {
        TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Could not find turn-in NPC for quest {}",
            questId);

        // Update progress data with error
        auto it = _botQuestProgress.find(player->GetGUID().GetCounter());
        if (it != _botQuestProgress.end())
        {
            for (auto& progress : it->second)
            {
                if (progress.questId == questId)
                {
                    progress.completionLog.push_back("Failed to find turn-in NPC");
                    break;
                }
            }
        }
        return;
    }

    // Step 3: Handle reward selection if quest has choice rewards
    if (quest->GetRewChoiceItemsCount() > 0)
    {
        HandleQuestRewardSelection(player, questId);
    }

    // Step 4: Complete the quest dialog
    CompleteQuestDialog(player, questId);

    TC_LOG_INFO("playerbot.quest", "QuestCompletion: Bot {} completed turn-in for quest {}",
        player->GetName(), questId);
}

/**
 * @brief Find the NPC to turn in a quest
 *
 * Searches for quest enders (NPCs that can accept quest completion) using:
 * 1. Quest POI data from cache
 * 2. CreatureQuestInvolvedRelation from database
 * 3. GameObjectQuestInvolvedRelation for GO-based turn-ins
 *
 * If found, either navigates to NPC or validates we're in interaction range.
 *
 * @param player The bot player
 * @param questId The quest to find turn-in NPC for
 * @return true if turn-in target found and accessible, false otherwise
 */
bool QuestCompletion::FindQuestTurnInNpc(Player* player, uint32 questId)
{
    if (!player || questId == 0)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    // Check progress data for stored quest giver info
    uint32 const botGuidCounter = player->GetGUID().GetCounter();
    {
        auto it = _botQuestProgress.find(botGuidCounter);
        if (it != _botQuestProgress.end())
        {
            for (auto& progress : it->second)
            {
                if (progress.questId == questId && progress.questGiverGuid != 0)
                {
                    // Try to find the stored quest giver
                    Creature* storedGiver = player->GetMap()->GetCreature(ObjectGuid::Create<HighGuid::Creature>(
                        player->GetMapId(), progress.questGiverGuid, progress.questGiverGuid));
                    if (storedGiver && storedGiver->IsAlive())
                    {
                        float dist = player->GetDistance(storedGiver);
                        if (dist <= INTERACTION_DISTANCE)
                        {
                            TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Found stored quest giver {} "
                                "for quest {} (dist: {:.1f})", storedGiver->GetName(), questId, dist);
                            return true;
                        }
                        else
                        {
                            // Navigate to stored giver
                            progress.questGiverLocation = storedGiver->GetPosition();
                            NavigateToTurnInNpc(player, storedGiver);
                            return true;
                        }
                    }
                }
            }
        }
    }

    // Search using CreatureQuestInvolvedRelation
    Creature* nearestTurnInNpc = nullptr;
    float nearestDist = std::numeric_limits<float>::max();

    for (auto const& [questIdKey, creatureEntry] : sObjectMgr->GetCreatureQuestInvolvedRelationReverseBounds(questId))
    {
        // Search for this creature nearby
        Creature* creature = player->FindNearestCreature(creatureEntry, OBJECTIVE_SEARCH_RADIUS);
        if (creature && creature->IsAlive())
        {
            float dist = player->GetDistance(creature);
            if (dist < nearestDist)
            {
                nearestDist = dist;
                nearestTurnInNpc = creature;
            }
        }
    }

    // Also check GameObjectQuestInvolvedRelation
    for (auto const& [questIdKey, goEntry] : sObjectMgr->GetGOQuestInvolvedRelationReverseBounds(questId))
    {
        GameObject* go = player->FindNearestGameObject(goEntry, OBJECTIVE_SEARCH_RADIUS);
        if (go && go->isSpawned())
        {
            float dist = player->GetDistance(go);
            if (dist <= INTERACTION_DISTANCE)
            {
                // Update progress data
                auto it = _botQuestProgress.find(botGuidCounter);
                if (it != _botQuestProgress.end())
                {
                    for (auto& progress : it->second)
                    {
                        if (progress.questId == questId)
                        {
                            progress.questGiverGuid = go->GetSpawnId();
                            progress.questGiverLocation = go->GetPosition();
                            break;
                        }
                    }
                }

                TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Found turn-in GO {} for quest {} "
                    "(dist: {:.1f})", go->GetName(), questId, dist);
                return true;
            }
            else if (dist < nearestDist)
            {
                // Navigate to GO
                NavigateToTurnInGO(player, go);
                return true;
            }
        }
    }

    // Handle NPC found
    if (nearestTurnInNpc)
    {
        // Update progress data with turn-in NPC info
        auto it = _botQuestProgress.find(botGuidCounter);
        if (it != _botQuestProgress.end())
        {
            for (auto& progress : it->second)
            {
                if (progress.questId == questId)
                {
                    progress.questGiverGuid = nearestTurnInNpc->GetSpawnId();
                    progress.questGiverLocation = nearestTurnInNpc->GetPosition();
                    break;
                }
            }
        }

        if (nearestDist <= INTERACTION_DISTANCE)
        {
            TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Found turn-in NPC {} for quest {} "
                "(dist: {:.1f})", nearestTurnInNpc->GetName(), questId, nearestDist);
            return true;
        }
        else
        {
            // Navigate to NPC
            NavigateToTurnInNpc(player, nearestTurnInNpc);
            return true;
        }
    }

    // No turn-in target found nearby - try to find spawn location from DB
    for (auto const& [questIdKey, creatureEntry] : sObjectMgr->GetCreatureQuestInvolvedRelationReverseBounds(questId))
    {
        CreatureData const* data = sObjectMgr->GetCreatureData(creatureEntry);
        if (data)
        {
            // Store location for navigation
            auto it = _botQuestProgress.find(botGuidCounter);
            if (it != _botQuestProgress.end())
            {
                for (auto& progress : it->second)
                {
                    if (progress.questId == questId)
                    {
                        progress.questGiverLocation = Position(data->spawnPoint);
                        progress.completionLog.push_back("Navigating to turn-in NPC spawn location");
                        break;
                    }
                }
            }

            TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Found turn-in NPC spawn for quest {} "
                "at ({:.1f}, {:.1f}, {:.1f})",
                questId, data->spawnPoint.GetPositionX(),
                data->spawnPoint.GetPositionY(), data->spawnPoint.GetPositionZ());
            return true;
        }
    }

    TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: No turn-in NPC found for quest {}", questId);
    return false;
}

/**
 * @brief Handle reward selection for quests with choice rewards
 *
 * Analyzes available rewards and selects the most appropriate one based on:
 * 1. Item usability (level, class restrictions)
 * 2. Item upgrade potential (vs currently equipped)
 * 3. Stat weights for bot's class/spec
 * 4. Vendor value as fallback
 *
 * @param player The bot player
 * @param questId The quest with reward choices
 */
void QuestCompletion::HandleQuestRewardSelection(Player* player, uint32 questId)
{
    if (!player || questId == 0)
        return;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest || quest->GetRewChoiceItemsCount() == 0)
        return;

    uint32 bestRewardIndex = 0;
    float bestRewardScore = -1.0f;
    std::string bestRewardReason;

    TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Evaluating {} reward choices for bot {} quest {}",
        quest->GetRewChoiceItemsCount(), player->GetName(), questId);

    // Evaluate each reward choice
    for (uint32 i = 0; i < quest->GetRewChoiceItemsCount(); ++i)
    {
        uint32 itemId = quest->RewardChoiceItemId[i];
        uint32 itemCount = quest->RewardChoiceItemCount[i];

        if (itemId == 0)
            continue;

        ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
        if (!itemTemplate)
            continue;

        float score = 0.0f;
        std::string reason;

        // Factor 1: Can bot use this item? (-1000 penalty if not usable)
        if (itemTemplate->GetBaseRequiredLevel() > static_cast<int32>(player->GetLevel()))
        {
            score -= 1000.0f;
            reason = "Level too low";
        }
        else if (!CanBotUseItem(player, itemTemplate))
        {
            score -= 500.0f;
            reason = "Class/race restriction";
        }
        else
        {
            // Factor 2: Is this an upgrade?
            float upgradeValue = EvaluateItemUpgrade(player, itemTemplate);
            score += upgradeValue;

            if (upgradeValue > 0)
                reason = "Upgrade (" + std::to_string(static_cast<int>(upgradeValue)) + " pts)";

            // Factor 3: Item level contributes to base value
            score += itemTemplate->GetBaseItemLevel() * 0.5f;

            // Factor 4: Stat appropriateness
            score += EvaluateItemStats(player, itemTemplate);

            // Factor 5: Stack value (multiple items worth more)
            score += itemCount * 10.0f;

            if (reason.empty())
                reason = "Base value";
        }

        // Factor 6: Vendor value as tiebreaker
        score += static_cast<float>(itemTemplate->GetSellPrice()) / 100000.0f;

        TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Reward {} ({}): score {:.1f} - {}",
            i, itemTemplate->GetName(LOCALE_enUS), score, reason);

        if (score > bestRewardScore)
        {
            bestRewardScore = score;
            bestRewardIndex = i;
            bestRewardReason = reason;
        }
    }

    // Store selected reward for use in CompleteQuestDialog
    {
        auto it = _botQuestProgress.find(player->GetGUID().GetCounter());
        if (it != _botQuestProgress.end())
        {
            for (auto& progress : it->second)
            {
                if (progress.questId == questId)
                {
                    progress.completionLog.push_back(
                        "Selected reward " + std::to_string(bestRewardIndex) +
                        " (" + bestRewardReason + ")");
                    break;
                }
            }
        }
    }

    // Store in pending reward selection map
    _pendingRewardSelections[player->GetGUID().GetCounter()] = {questId, bestRewardIndex};

    TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Bot {} selected reward {} for quest {} ({})",
        player->GetName(), bestRewardIndex, questId, bestRewardReason);
}

/**
 * @brief Complete quest dialog and receive rewards
 *
 * Finalizes quest turn-in by:
 * 1. Sending quest offer reward packet
 * 2. Selecting chosen reward (if applicable)
 * 3. Completing the quest via Player API
 * 4. Updating internal tracking and metrics
 *
 * @param player The bot player
 * @param questId The quest to complete
 */
void QuestCompletion::CompleteQuestDialog(Player* player, uint32 questId)
{
    if (!player || questId == 0)
        return;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return;

    QuestStatus status = player->GetQuestStatus(questId);
    if (status != QUEST_STATUS_COMPLETE)
    {
        TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Quest {} not ready for dialog completion",
            questId);
        return;
    }

    // Find the quest giver
    ObjectGuid questGiverGuid;
    WorldObject* questGiver = nullptr;

    // Check stored quest giver
    uint32 const botGuidCounter = player->GetGUID().GetCounter();
    {
        auto it = _botQuestProgress.find(botGuidCounter);
        if (it != _botQuestProgress.end())
        {
            for (auto const& progress : it->second)
            {
                if (progress.questId == questId && progress.questGiverGuid != 0)
                {
                    // Try to resolve quest giver
                    if (Creature* creature = player->FindNearestCreature(progress.questGiverGuid, INTERACTION_DISTANCE))
                    {
                        questGiver = creature;
                        questGiverGuid = creature->GetGUID();
                    }
                    break;
                }
            }
        }
    }

    // If no stored quest giver, find nearest quest ender
    if (!questGiver)
    {
        for (auto const& [questIdKey, creatureEntry] : sObjectMgr->GetCreatureQuestInvolvedRelationReverseBounds(questId))
        {
            Creature* creature = player->FindNearestCreature(creatureEntry, INTERACTION_DISTANCE);
            if (creature && creature->IsAlive())
            {
                questGiver = creature;
                questGiverGuid = creature->GetGUID();
                break;
            }
        }
    }

    // Try game objects if no creature found
    if (!questGiver)
    {
        for (auto const& [questIdKey, goEntry] : sObjectMgr->GetGOQuestInvolvedRelationReverseBounds(questId))
        {
            GameObject* go = player->FindNearestGameObject(goEntry, INTERACTION_DISTANCE);
            if (go && go->isSpawned())
            {
                questGiver = go;
                questGiverGuid = go->GetGUID();
                break;
            }
        }
    }

    if (!questGiver)
    {
        TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: No quest giver in range for quest {} "
            "turn-in", questId);
        return;
    }

    // Get selected reward (default to 0)
    uint32 rewardChoice = 0;
    auto rewardIt = _pendingRewardSelections.find(botGuidCounter);
    if (rewardIt != _pendingRewardSelections.end() && rewardIt->second.first == questId)
    {
        rewardChoice = rewardIt->second.second;
        _pendingRewardSelections.erase(rewardIt);
    }

    // Complete the quest
    TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Bot {} completing quest {} with reward choice {}",
        player->GetName(), questId, rewardChoice);

    // Send quest offer reward (for proper packet handling)
    if (Creature* creature = questGiver->ToCreature())
    {
        player->PlayerTalkClass->SendQuestGiverOfferReward(quest, questGiverGuid, true);
    }

    // Reward the quest - TrinityCore 12.0 API requires LootItemType and item ID
    uint32 rewardItemId = 0;
    if (rewardChoice < QUEST_REWARD_CHOICES_COUNT && quest->RewardChoiceItemId[rewardChoice] != 0)
        rewardItemId = quest->RewardChoiceItemId[rewardChoice];
    player->RewardQuest(quest, LootItemType::Item, rewardItemId, questGiver, true);

    // Update internal tracking
    {
        auto it = _botQuestProgress.find(botGuidCounter);
        if (it != _botQuestProgress.end())
        {
            // Remove completed quest from progress tracking
            auto& progressList = it->second;
            progressList.erase(
                std::remove_if(progressList.begin(), progressList.end(),
                    [questId](QuestProgressData const& p) { return p.questId == questId; }),
                progressList.end());
        }
    }

    // Update metrics
    _globalMetrics.questsCompleted++;

    // Invalidate progress cache
    InvalidateProgressCache(player->GetGUID(), questId);

    TC_LOG_INFO("playerbot.quest", "QuestCompletion: Bot {} completed quest {} [{}]",
        player->GetName(), questId, quest->GetLogTitle());
}

/**
 * @brief Navigate to turn-in NPC
 *
 * @param player The bot player
 * @param npc The turn-in NPC
 */
void QuestCompletion::NavigateToTurnInNpc(Player* player, Creature* npc)
{
    if (!player || !npc)
        return;

    // Stop any current movement
    player->GetMotionMaster()->Clear(MOTION_PRIORITY_NORMAL);

    // Move to NPC with interaction distance
    float dist = player->GetDistance(npc);
    if (dist > INTERACTION_DISTANCE)
    {
        Position targetPos;
        npc->GetNearPoint(player, targetPos.m_positionX, targetPos.m_positionY, targetPos.m_positionZ,
            INTERACTION_DISTANCE - 1.0f, npc->GetAbsoluteAngle(player));

        // NEW: Use BotMovementController for validated movement
        if (BotAI* ai = GetBotAI(player))
        {
            if (!ai->MoveTo(targetPos, true))
            {
                // Fallback to legacy MotionMaster
                player->GetMotionMaster()->MovePoint(0, targetPos);
            }
        }
        else
        {
            // Non-bot player - use standard movement
            player->GetMotionMaster()->MovePoint(0, targetPos);
        }

        TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Bot {} navigating to NPC {} "
            "(dist: {:.1f})", player->GetName(), npc->GetName(), dist);
    }
}

/**
 * @brief Navigate to turn-in GameObject
 *
 * @param player The bot player
 * @param go The turn-in GameObject
 */
void QuestCompletion::NavigateToTurnInGO(Player* player, GameObject* go)
{
    if (!player || !go)
        return;

    // Stop any current movement
    player->GetMotionMaster()->Clear(MOTION_PRIORITY_NORMAL);

    // Move to GO with interaction distance
    float dist = player->GetDistance(go);
    if (dist > INTERACTION_DISTANCE)
    {
        Position targetPos = go->GetPosition();
        // Adjust position slightly towards player
        float angle = go->GetAbsoluteAngle(player);
        targetPos.m_positionX += std::cos(angle) * (INTERACTION_DISTANCE - 1.0f);
        targetPos.m_positionY += std::sin(angle) * (INTERACTION_DISTANCE - 1.0f);

        // NEW: Use BotMovementController for validated movement
        if (BotAI* ai = GetBotAI(player))
        {
            if (!ai->MoveTo(targetPos, true))
            {
                // Fallback to legacy MotionMaster
                player->GetMotionMaster()->MovePoint(0, targetPos);
            }
        }
        else
        {
            // Non-bot player - use standard movement
            player->GetMotionMaster()->MovePoint(0, targetPos);
        }

        TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Bot {} navigating to GO {} "
            "(dist: {:.1f})", player->GetName(), go->GetName(), dist);
    }
}

/**
 * @brief Check if bot can use an item
 *
 * @param player The bot player
 * @param item The item template
 * @return true if bot can use the item
 */
bool QuestCompletion::CanBotUseItem(Player* player, ItemTemplate const* item)
{
    if (!player || !item)
        return false;

    // Check class requirement
    int32 allowableClass = item->GetAllowableClass();
    if (allowableClass != 0 && !(allowableClass & player->GetClassMask()))
        return false;

    // Check race requirement - TrinityCore 12.0 uses Trinity::RaceMask<int64>
    Trinity::RaceMask<int64> allowableRace = item->GetAllowableRace();
    if (!allowableRace.IsEmpty() && !allowableRace.HasRace(player->GetRace()))
        return false;

    // Check skill requirements
    uint32 requiredSkill = item->GetRequiredSkill();
    if (requiredSkill != 0)
    {
        if (!player->HasSkill(requiredSkill) ||
            player->GetSkillValue(requiredSkill) < item->GetRequiredSkillRank())
            return false;
    }

    // Check reputation requirements
    uint32 requiredRepFaction = item->GetRequiredReputationFaction();
    if (requiredRepFaction != 0)
    {
        if (player->GetReputationRank(requiredRepFaction) < static_cast<ReputationRank>(item->GetRequiredReputationRank()))
            return false;
    }

    return true;
}

/**
 * @brief Evaluate if item is an upgrade for bot
 *
 * @param player The bot player
 * @param item The item template
 * @return Upgrade score (positive = upgrade, negative = downgrade)
 */
float QuestCompletion::EvaluateItemUpgrade(Player* player, ItemTemplate const* item)
{
    if (!player || !item)
        return 0.0f;

    float upgradeValue = 0.0f;

    // Find slot for this item
    uint32 destSlot = 0;
    switch (item->GetInventoryType())
    {
        case INVTYPE_HEAD:          destSlot = EQUIPMENT_SLOT_HEAD; break;
        case INVTYPE_NECK:          destSlot = EQUIPMENT_SLOT_NECK; break;
        case INVTYPE_SHOULDERS:     destSlot = EQUIPMENT_SLOT_SHOULDERS; break;
        case INVTYPE_BODY:          destSlot = EQUIPMENT_SLOT_BODY; break;
        case INVTYPE_CHEST:
        case INVTYPE_ROBE:          destSlot = EQUIPMENT_SLOT_CHEST; break;
        case INVTYPE_WAIST:         destSlot = EQUIPMENT_SLOT_WAIST; break;
        case INVTYPE_LEGS:          destSlot = EQUIPMENT_SLOT_LEGS; break;
        case INVTYPE_FEET:          destSlot = EQUIPMENT_SLOT_FEET; break;
        case INVTYPE_WRISTS:        destSlot = EQUIPMENT_SLOT_WRISTS; break;
        case INVTYPE_HANDS:         destSlot = EQUIPMENT_SLOT_HANDS; break;
        case INVTYPE_FINGER:        destSlot = EQUIPMENT_SLOT_FINGER1; break;
        case INVTYPE_TRINKET:       destSlot = EQUIPMENT_SLOT_TRINKET1; break;
        case INVTYPE_CLOAK:         destSlot = EQUIPMENT_SLOT_BACK; break;
        case INVTYPE_WEAPON:
        case INVTYPE_2HWEAPON:
        case INVTYPE_WEAPONMAINHAND: destSlot = EQUIPMENT_SLOT_MAINHAND; break;
        case INVTYPE_SHIELD:
        case INVTYPE_WEAPONOFFHAND: destSlot = EQUIPMENT_SLOT_OFFHAND; break;
        case INVTYPE_RANGED:
        case INVTYPE_RANGEDRIGHT:   destSlot = EQUIPMENT_SLOT_MAINHAND; break;
        default:
            return 0.0f;  // Not equippable
    }

    // Get currently equipped item
    Item* currentItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, static_cast<uint8>(destSlot));
    if (!currentItem)
    {
        // Empty slot - any item is an upgrade
        return item->GetBaseItemLevel() * 2.0f;
    }

    ItemTemplate const* currentTemplate = currentItem->GetTemplate();
    if (!currentTemplate)
        return item->GetBaseItemLevel() * 2.0f;

    // Compare item levels
    int32 itemLevelDiff = static_cast<int32>(item->GetBaseItemLevel()) - static_cast<int32>(currentTemplate->GetBaseItemLevel());
    upgradeValue += itemLevelDiff * 5.0f;

    // Compare quality
    int32 qualityDiff = static_cast<int32>(item->GetQuality()) - static_cast<int32>(currentTemplate->GetQuality());
    upgradeValue += qualityDiff * 20.0f;

    return upgradeValue;
}

/**
 * @brief Evaluate item stats for bot's class/role
 *
 * @param player The bot player
 * @param item The item template
 * @return Stat value score
 */
float QuestCompletion::EvaluateItemStats(Player* player, ItemTemplate const* item)
{
    if (!player || !item)
        return 0.0f;

    float statValue = 0.0f;
    uint8 playerClass = player->GetClass();

    // Define stat weights based on class
    std::unordered_map<uint32, float> statWeights;

    switch (playerClass)
    {
        case CLASS_WARRIOR:
        case CLASS_DEATH_KNIGHT:
            statWeights[ITEM_MOD_STRENGTH] = 2.0f;
            statWeights[ITEM_MOD_STAMINA] = 1.5f;
            statWeights[ITEM_MOD_CRIT_RATING] = 1.0f;
            statWeights[ITEM_MOD_HASTE_RATING] = 0.8f;
            break;

        case CLASS_PALADIN:
            statWeights[ITEM_MOD_STRENGTH] = 1.8f;
            statWeights[ITEM_MOD_INTELLECT] = 1.2f;
            statWeights[ITEM_MOD_STAMINA] = 1.5f;
            statWeights[ITEM_MOD_CRIT_RATING] = 1.0f;
            break;

        case CLASS_HUNTER:
        case CLASS_ROGUE:
        case CLASS_MONK:
        case CLASS_DEMON_HUNTER:
            statWeights[ITEM_MOD_AGILITY] = 2.0f;
            statWeights[ITEM_MOD_STAMINA] = 1.0f;
            statWeights[ITEM_MOD_CRIT_RATING] = 1.2f;
            statWeights[ITEM_MOD_HASTE_RATING] = 1.0f;
            break;

        case CLASS_PRIEST:
        case CLASS_MAGE:
        case CLASS_WARLOCK:
        case CLASS_EVOKER:
            statWeights[ITEM_MOD_INTELLECT] = 2.0f;
            statWeights[ITEM_MOD_STAMINA] = 0.8f;
            statWeights[ITEM_MOD_CRIT_RATING] = 1.2f;
            statWeights[ITEM_MOD_HASTE_RATING] = 1.5f;
            break;

        case CLASS_SHAMAN:
            statWeights[ITEM_MOD_INTELLECT] = 1.8f;
            statWeights[ITEM_MOD_AGILITY] = 1.5f;
            statWeights[ITEM_MOD_STAMINA] = 1.2f;
            break;

        case CLASS_DRUID:
            statWeights[ITEM_MOD_INTELLECT] = 1.5f;
            statWeights[ITEM_MOD_AGILITY] = 1.5f;
            statWeights[ITEM_MOD_STAMINA] = 1.2f;
            break;

        default:
            // Generic weights
            statWeights[ITEM_MOD_STAMINA] = 1.0f;
            statWeights[ITEM_MOD_STRENGTH] = 1.0f;
            statWeights[ITEM_MOD_AGILITY] = 1.0f;
            statWeights[ITEM_MOD_INTELLECT] = 1.0f;
            break;
    }

    // Evaluate item stats - TrinityCore 12.0 uses GetStatModifierBonusStat/GetStatPercentEditor
    for (uint8 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
    {
        int32 statType = item->GetStatModifierBonusStat(i);
        if (statType == 0)
            continue;

        int32 statVal = item->GetStatPercentEditor(i);
        auto weightIt = statWeights.find(statType);
        if (weightIt != statWeights.end())
        {
            statValue += statVal * weightIt->second;
        }
        else
        {
            // Unknown stat gets small value
            statValue += statVal * 0.5f;
        }
    }

    return statValue;
}
::std::vector<uint32> QuestCompletion::GetActiveQuests(Player* player)
{
    ::std::vector<uint32> activeQuests;
    if (!player)
        return activeQuests;

    // Reserve capacity for typical quest log size
    activeQuests.reserve(25);

    // Iterate through quest log slots (0 to MAX_QUEST_LOG_SIZE-1)
    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = player->GetQuestSlotQuestId(slot);
        if (questId == 0)
            continue;

        QuestStatus status = player->GetQuestStatus(questId);
        // Include INCOMPLETE and COMPLETE (ready for turn-in)
        if (status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE)
        {
            activeQuests.push_back(questId);
        }
    }

    TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Bot {} has {} active quests",
        player->GetName(), activeQuests.size());
    return activeQuests;
}
::std::vector<uint32> QuestCompletion::GetCompletableQuests(Player* player)
{
    ::std::vector<uint32> completableQuests;
    if (!player)
        return completableQuests;

    // Iterate through quest log slots
    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = player->GetQuestSlotQuestId(slot);
        if (questId == 0)
            continue;

        QuestStatus status = player->GetQuestStatus(questId);
        // Only include quests that are ready for turn-in
        if (status == QUEST_STATUS_COMPLETE)
        {
            completableQuests.push_back(questId);
        }
    }

    TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Bot {} has {} completable quests",
        player->GetName(), completableQuests.size());
    return completableQuests;
}
uint32 QuestCompletion::GetHighestPriorityQuest(Player* player)
{
    if (!player)
        return 0;

    std::vector<uint32> activeQuests = GetActiveQuests(player);
    if (activeQuests.empty())
        return 0;

    uint32 highestPriorityQuest = 0;
    int32 highestPriority = INT32_MIN;

    for (uint32 questId : activeQuests)
    {
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            continue;

        int32 priority = CalculateQuestPriorityInternal(player, quest);
        if (priority > highestPriority)
        {
            highestPriority = priority;
            highestPriorityQuest = questId;
        }
    }

    TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Bot {} highest priority quest is {} (priority {})",
        player->GetName(), highestPriorityQuest, highestPriority);
    return highestPriorityQuest;
}

/**
 * @brief Calculate priority score for a quest
 *
 * Higher score = higher priority. Factors include:
 * - Completion percentage (nearly complete quests get priority)
 * - Quest level relative to player
 * - Time-limited quests (daily/timed)
 * - Group quests when in group
 *
 * @param player The player
 * @param quest The quest template
 * @return Priority score (higher = more important)
 */
int32 QuestCompletion::CalculateQuestPriorityInternal(Player* player, Quest const* quest)
{
    if (!player || !quest)
        return 0;

    int32 priority = 0;

    // Factor 1: Completion percentage (higher = higher priority)
    float progress = CalculateQuestProgress(quest->GetQuestId(), player);
    priority += static_cast<int32>(progress * 100);

    // Factor 2: Quest level relative to player (use MaxLevel as approximate quest level)
    uint32 questMaxLevel = quest->GetMaxLevel();
    if (questMaxLevel > 0)
    {
        int32 levelDiff = std::abs(static_cast<int32>(player->GetLevel()) - static_cast<int32>(questMaxLevel));
        priority -= levelDiff * 10;
    }

    // Factor 3: Time-limited quests get priority boost
    if (quest->HasFlag(QUEST_FLAGS_DAILY) || quest->GetLimitTime() > 0)
        priority += 50;

    // Factor 4: Group quests when in group (use player's current difficulty)
    if (player->GetGroup() && quest->IsAllowedInRaid(player->GetMap()->GetDifficultyID()))
        priority += 30;

    // Factor 5: Ready for turn-in gets highest priority
    if (player->GetQuestStatus(quest->GetQuestId()) == QUEST_STATUS_COMPLETE)
        priority += 200;

    return priority;
}
/**
 * @brief Calculate quest progress (0.0 to 1.0)
 *
 * HYBRID APPROACH:
 * 1. First checks EventBus cache for real-time progress from packet sniffer (<5μs)
 * 2. Falls back to Player API if cache miss (100-500ms but more accurate)
 *
 * Performance: <5μs cache hit, <50ms cache miss
 *
 * @param questId Quest ID to check
 * @param player Player to check progress for
 * @return Progress from 0.0 (not started) to 1.0 (complete)
 */
float QuestCompletion::CalculateQuestProgress(uint32 questId, Player* player)
{
    if (!player || questId == 0)
        return 0.0f;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return 0.0f;

    QuestStatus status = player->GetQuestStatus(questId);
    if (status == QUEST_STATUS_REWARDED)
        return 1.0f;
    if (status == QUEST_STATUS_COMPLETE)
        return 1.0f;
    if (status == QUEST_STATUS_NONE)
        return 0.0f;

    // OPTIMIZATION: Check EventBus cache first (real-time from packet sniffer)
    // This provides <5μs lookups vs 100-500ms DB polling
    auto cachedProgress = GetCachedQuestProgress(player->GetGUID(), questId);
    if (cachedProgress.has_value() && cachedProgress->IsFresh())
    {
        TC_LOG_TRACE("playerbot.quest", "CalculateQuestProgress: Cache hit for quest {} = {:.1f}%",
            questId, cachedProgress->progress * 100.0f);
        return cachedProgress->progress;
    }

    // Fallback to Player API (for initial load or cache miss)
    uint16 slot = player->FindQuestSlot(questId);
    if (slot >= MAX_QUEST_LOG_SIZE)
        return 0.0f;

    QuestObjectives const& objectives = quest->GetObjectives();
    if (objectives.empty())
        return 1.0f;  // Quest with no objectives is considered complete

    float totalProgress = 0.0f;
    uint32 objectiveCount = 0;

    for (auto const& objective : objectives)
    {
        uint32 required = objective.Amount;
        if (required == 0)
            continue;

        // TrinityCore 12.0 API: GetQuestSlotObjectiveData takes QuestObjective const&
        int32 current = player->GetQuestSlotObjectiveData(slot, objective);
        float objProgress = std::min(1.0f, static_cast<float>(std::max(0, current)) / static_cast<float>(required));
        totalProgress += objProgress;
        ++objectiveCount;
    }

    float progress = objectiveCount > 0 ? totalProgress / objectiveCount : 1.0f;

    // Update cache for future queries (avoid repeated Player API calls)
    UpdateProgressCache(player->GetGUID(), questId, progress);

    TC_LOG_TRACE("playerbot.quest", "CalculateQuestProgress: Cache miss for quest {} = {:.1f}%",
        questId, progress * 100.0f);
    return progress;
}
/**
 * @brief Set maximum concurrent quests for a bot
 *
 * Limits how many quests a bot can actively work on simultaneously.
 * Default is 3 for efficiency, can be increased to 25 (max quest log size).
 *
 * @param playerId Bot's GUID counter
 * @param max Maximum concurrent quests (1-25)
 */
void QuestCompletion::SetMaxConcurrentQuests(uint32 playerId, uint32 max)
{
    // Clamp to valid range
    uint32 clampedMax = std::clamp(max, 1u, static_cast<uint32>(MAX_QUEST_LOG_SIZE));

    {
        std::lock_guard<std::mutex> lock(_configMutex);
        _botMaxConcurrentQuests[playerId] = clampedMax;
    }

    TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Set max concurrent quests to {} for player {}",
        clampedMax, playerId);
}

/**
 * @brief Enable or disable group coordination for a specific quest
 *
 * When enabled, bots in groups will coordinate their objective completion
 * to share kills, avoid loot conflicts, and synchronize progress.
 *
 * @param questId Quest ID to configure
 * @param enabled Whether to enable group coordination
 */
void QuestCompletion::EnableGroupCoordination(uint32 questId, bool enabled)
{
    {
        std::lock_guard<std::mutex> lock(_configMutex);
        if (enabled)
            _groupCoordinationEnabled.insert(questId);
        else
            _groupCoordinationEnabled.erase(questId);
    }

    TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: {} group coordination for quest {}",
        enabled ? "Enabled" : "Disabled", questId);
}

/**
 * @brief Handle dungeon-specific quests for a bot
 *
 * Sets appropriate strategy for dungeon quests:
 * - Enables group coordination
 * - Sets GROUP_COORDINATION strategy
 * - Prioritizes dungeon objectives in quest order
 *
 * @param player Bot player
 * @param dungeonId Dungeon/instance map ID
 */
void QuestCompletion::HandleDungeonQuests(Player* player, uint32 dungeonId)
{
    if (!player)
        return;

    TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Handling dungeon quests for {} in dungeon {}",
        player->GetName(), dungeonId);

    uint32 botGuid = player->GetGUID().GetCounter();

    // Find all quests that have objectives in this dungeon
    std::vector<uint32> dungeonQuests;

    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = player->GetQuestSlotQuestId(slot);
        if (questId == 0)
            continue;

        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            continue;

        QuestStatus status = player->GetQuestStatus(questId);
        if (status != QUEST_STATUS_INCOMPLETE)
            continue;

        // Check if quest has objectives in dungeons
        // Dungeon quests typically have flags indicating group content
        if (quest->IsRaidQuest(DIFFICULTY_NONE) || (quest->GetFlags() & QUEST_FLAGS_RAID_GROUP_OK))
        {
            dungeonQuests.push_back(questId);
        }
    }

    if (dungeonQuests.empty())
    {
        TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: No dungeon quests found for {} in dungeon {}",
            player->GetName(), dungeonId);
        return;
    }

    // Enable group coordination for all dungeon quests
    for (uint32 questId : dungeonQuests)
    {
        EnableGroupCoordination(questId, true);
    }

    // Set bot strategy to GROUP_COORDINATION
    SetQuestCompletionStrategy(botGuid, QuestCompletionStrategy::GROUP_COORDINATION);

    // Update quest order to prioritize dungeon quests
    {
        std::unique_lock<std::shared_mutex> lock(_questOrderMutex);
        std::vector<uint32>& questOrder = _botQuestOrder[botGuid];

        // Remove dungeon quests from current order
        questOrder.erase(
            std::remove_if(questOrder.begin(), questOrder.end(),
                [&dungeonQuests](uint32 qId) {
                    return std::find(dungeonQuests.begin(), dungeonQuests.end(), qId) != dungeonQuests.end();
                }),
            questOrder.end());

        // Insert dungeon quests at the front
        questOrder.insert(questOrder.begin(), dungeonQuests.begin(), dungeonQuests.end());
    }

    TC_LOG_INFO("playerbot.quest", "QuestCompletion: Bot {} now prioritizing {} dungeon quests",
        player->GetName(), dungeonQuests.size());
}

/**
 * @brief Handle PvP-specific quests for a bot
 *
 * Configures quest handling for PvP environments:
 * - Uses SPEED_COMPLETION strategy in battlegrounds
 * - Handles honor/kill-based objectives
 * - Coordinates with group for objective captures
 *
 * @param player Bot player
 * @param questId PvP quest ID
 */
void QuestCompletion::HandlePvPQuests(Player* player, uint32 questId)
{
    if (!player)
        return;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return;

    TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Handling PvP quest {} for {}",
        questId, player->GetName());

    uint32 botGuid = player->GetGUID().GetCounter();

    // Check if this is actually a PvP quest
    bool isPvPQuest = false;
    auto const& objectives = quest->GetObjectives();

    for (auto const& obj : objectives)
    {
        // Check for PvP-related objective types
        if (obj.Type == QUEST_OBJECTIVE_PLAYERKILLS ||
            obj.Type == QUEST_OBJECTIVE_HAVE_CURRENCY)  // Honor currency
        {
            isPvPQuest = true;
            break;
        }
    }

    // Also check quest flags for PvP flagging
    if (quest->GetFlags() & QUEST_FLAGS_FLAGS_PVP)
        isPvPQuest = true;

    if (!isPvPQuest)
    {
        TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Quest {} is not a PvP quest", questId);
        return;
    }

    // Set appropriate strategy for PvP
    if (player->InBattleground())
    {
        SetQuestCompletionStrategy(botGuid, QuestCompletionStrategy::SPEED_COMPLETION);
    }
    else if (player->GetGroup())
    {
        SetQuestCompletionStrategy(botGuid, QuestCompletionStrategy::GROUP_COORDINATION);
        EnableGroupCoordination(questId, true);
    }
    else
    {
        SetQuestCompletionStrategy(botGuid, QuestCompletionStrategy::SAFE_COMPLETION);
    }

    // Prioritize this quest
    {
        std::unique_lock<std::shared_mutex> lock(_questOrderMutex);
        std::vector<uint32>& questOrder = _botQuestOrder[botGuid];

        // Remove from current position
        questOrder.erase(
            std::remove(questOrder.begin(), questOrder.end(), questId),
            questOrder.end());

        // Add to front
        questOrder.insert(questOrder.begin(), questId);
    }

    TC_LOG_INFO("playerbot.quest", "QuestCompletion: Bot {} configured for PvP quest {} with {} strategy",
        player->GetName(), questId,
        player->InBattleground() ? "SPEED_COMPLETION" :
        (player->GetGroup() ? "GROUP_COORDINATION" : "SAFE_COMPLETION"));
}

/**
 * @brief Handle seasonal/holiday event quests
 *
 * Prioritizes time-limited seasonal quests during active game events.
 * Checks current active events and boosts priority of related quests.
 *
 * @param player Bot player
 */
void QuestCompletion::HandleSeasonalQuests(Player* player)
{
    if (!player)
        return;

    TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Handling seasonal quests for {}",
        player->GetName());

    uint32 botGuid = player->GetGUID().GetCounter();
    std::vector<uint32> seasonalQuests;

    // Iterate through quest log to find seasonal quests
    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = player->GetQuestSlotQuestId(slot);
        if (questId == 0)
            continue;

        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            continue;

        QuestStatus status = player->GetQuestStatus(questId);
        if (status != QUEST_STATUS_INCOMPLETE && status != QUEST_STATUS_COMPLETE)
            continue;

        // Check if quest is seasonal (has game event requirement or time limit)
        bool isSeasonal = false;

        // Check for event requirement
        if (quest->GetEventIdForQuest() > 0)
            isSeasonal = true;

        // Check for time limit (many holiday quests have time limits)
        if (quest->GetLimitTime() > 0)
            isSeasonal = true;

        // Check quest flags for special seasonal indicators
        if (quest->GetFlags() & QUEST_FLAGS_AUTO_ACCEPT)
        {
            // Many holiday quests auto-accept
            // Additional check: quest category or title patterns
            std::string questTitle = quest->GetLogTitle();
            if (questTitle.find("Festival") != std::string::npos ||
                questTitle.find("Holiday") != std::string::npos ||
                questTitle.find("Winter") != std::string::npos ||
                questTitle.find("Hallow") != std::string::npos ||
                questTitle.find("Love") != std::string::npos ||
                questTitle.find("Brewfest") != std::string::npos ||
                questTitle.find("Midsummer") != std::string::npos ||
                questTitle.find("Pilgrim") != std::string::npos ||
                questTitle.find("Noblegarden") != std::string::npos)
            {
                isSeasonal = true;
            }
        }

        if (isSeasonal)
            seasonalQuests.push_back(questId);
    }

    if (seasonalQuests.empty())
    {
        TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: No seasonal quests found for {}",
            player->GetName());
        return;
    }

    // Prioritize seasonal quests at the top of the quest order
    {
        std::unique_lock<std::shared_mutex> lock(_questOrderMutex);
        std::vector<uint32>& questOrder = _botQuestOrder[botGuid];

        // Remove seasonal quests from current positions
        for (uint32 seasonalId : seasonalQuests)
        {
            questOrder.erase(
                std::remove(questOrder.begin(), questOrder.end(), seasonalId),
                questOrder.end());
        }

        // Insert seasonal quests at the front (they're time-limited)
        questOrder.insert(questOrder.begin(), seasonalQuests.begin(), seasonalQuests.end());
    }

    // Use SPEED_COMPLETION strategy for time-limited content
    SetQuestCompletionStrategy(botGuid, QuestCompletionStrategy::SPEED_COMPLETION);

    TC_LOG_INFO("playerbot.quest", "QuestCompletion: Bot {} prioritizing {} seasonal quests",
        player->GetName(), seasonalQuests.size());
}

/**
 * @brief Handle daily quests with reset-aware prioritization
 *
 * Identifies and prioritizes daily quests that need completion before reset.
 * Adjusts strategy based on time remaining until daily reset.
 *
 * @param player Bot player
 */
void QuestCompletion::HandleDailyQuests(Player* player)
{
    if (!player)
        return;

    TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Handling daily quests for {}",
        player->GetName());

    uint32 botGuid = player->GetGUID().GetCounter();
    std::vector<uint32> dailyQuests;

    // Iterate through quest log to find daily quests
    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = player->GetQuestSlotQuestId(slot);
        if (questId == 0)
            continue;

        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            continue;

        QuestStatus status = player->GetQuestStatus(questId);
        if (status != QUEST_STATUS_INCOMPLETE && status != QUEST_STATUS_COMPLETE)
            continue;

        // Check if quest is daily
        if (quest->HasFlag(QUEST_FLAGS_DAILY) || quest->HasFlag(QUEST_FLAGS_WEEKLY))
        {
            dailyQuests.push_back(questId);
        }
    }

    if (dailyQuests.empty())
    {
        TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: No daily quests found for {}",
            player->GetName());
        return;
    }

    // Calculate time until daily reset
    time_t now = GameTime::GetGameTime();
    time_t nextReset = sWorld->GetNextDailyQuestsResetTime();
    time_t timeUntilReset = nextReset - now;

    // If less than 2 hours until reset, use SPEED_COMPLETION
    // If more time available, use EFFICIENT_COMPLETION
    QuestCompletionStrategy strategy = QuestCompletionStrategy::EFFICIENT_COMPLETION;
    if (timeUntilReset < 7200) // 2 hours
    {
        strategy = QuestCompletionStrategy::SPEED_COMPLETION;
        TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: {} hours until daily reset - using SPEED strategy",
            timeUntilReset / 3600.0f);
    }

    // Prioritize daily quests
    {
        std::unique_lock<std::shared_mutex> lock(_questOrderMutex);
        std::vector<uint32>& questOrder = _botQuestOrder[botGuid];

        // Remove daily quests from current positions
        for (uint32 dailyId : dailyQuests)
        {
            questOrder.erase(
                std::remove(questOrder.begin(), questOrder.end(), dailyId),
                questOrder.end());
        }

        // Insert daily quests at the front
        questOrder.insert(questOrder.begin(), dailyQuests.begin(), dailyQuests.end());
    }

    SetQuestCompletionStrategy(botGuid, strategy);

    TC_LOG_INFO("playerbot.quest", "QuestCompletion: Bot {} prioritizing {} daily quests ({:.1f}h until reset)",
        player->GetName(), dailyQuests.size(), timeUntilReset / 3600.0f);
}

/**
 * @brief Handle quest completion errors with tracking and recovery triggering
 *
 * Logs errors, tracks error history, and triggers automatic recovery
 * when error thresholds are exceeded.
 *
 * @param player Bot player
 * @param questId Quest that encountered the error
 * @param error Error description
 */
void QuestCompletion::HandleQuestCompletionError(Player* player, uint32 questId, ::std::string const& error)
{
    if (!player)
        return;

    uint32 botGuid = player->GetGUID().GetCounter();

    TC_LOG_WARN("playerbot.quest", "QuestCompletion: Error for bot {} on quest {}: {}",
        player->GetName(), questId, error);

    // Track the error
    QuestErrorInfo errorInfo;
    errorInfo.questId = questId;
    errorInfo.error = error;
    errorInfo.timestamp = GameTime::GetGameTimeMS();
    errorInfo.recoveryAttempts = 0;

    {
        std::lock_guard<std::mutex> lock(_errorMutex);
        auto& botErrors = _botQuestErrors[botGuid];
        botErrors.push_back(errorInfo);

        // Keep only last 50 errors per bot
        if (botErrors.size() > 50)
            botErrors.erase(botErrors.begin());

        // Count recent errors for this quest (last 5 minutes)
        uint32 recentErrorCount = 0;
        uint32 fiveMinutesAgo = GameTime::GetGameTimeMS() - 300000;

        for (auto const& err : botErrors)
        {
            if (err.questId == questId && err.timestamp >= fiveMinutesAgo)
                ++recentErrorCount;
        }

        // If too many recent errors, trigger recovery
        if (recentErrorCount >= 3)
        {
            TC_LOG_WARN("playerbot.quest", "QuestCompletion: Quest {} has {} recent errors - triggering recovery",
                questId, recentErrorCount);
            RecoverFromCompletionFailure(player, questId);
        }
    }

    // Update global metrics
    _globalMetrics.questsFailed++;
}

/**
 * @brief Recover from a quest completion failure
 *
 * Implements multi-tier recovery:
 * 1. Clear stuck state and reset objective progress tracking
 * 2. Re-diagnose quest issues
 * 3. Try alternative strategies
 * 4. Consider quest abandonment if unrecoverable
 *
 * @param player Bot player
 * @param questId Quest to recover
 */
void QuestCompletion::RecoverFromCompletionFailure(Player* player, uint32 questId)
{
    if (!player)
        return;

    uint32 botGuid = player->GetGUID().GetCounter();

    TC_LOG_INFO("playerbot.quest", "QuestCompletion: Initiating recovery for bot {} on quest {}",
        player->GetName(), questId);

    // Step 1: Diagnose current issues
    DiagnoseCompletionIssues(player, questId);

    // Step 2: Check diagnostics for recovery path
    QuestDiagnostics* diag = nullptr;
    {
        auto botIt = _questDiagnostics.find(botGuid);
        if (botIt != _questDiagnostics.end())
        {
            auto questIt = botIt->second.find(questId);
            if (questIt != botIt->second.end())
                diag = &questIt->second;
        }
    }

    // Step 3: Clear stuck state in progress tracking
    auto progIt = _botQuestProgress.find(botGuid);
    if (progIt != _botQuestProgress.end())
    {
        for (auto& progress : progIt->second)
        {
            if (progress.questId == questId)
            {
                progress.isStuck = false;
                progress.consecutiveFailures = 0;
                progress.lastUpdateTime = GameTime::GetGameTimeMS();

                // Reset all objective retry counters
                for (auto& obj : progress.objectives)
                {
                    if (obj.status == ObjectiveStatus::BLOCKED ||
                        obj.status == ObjectiveStatus::FAILED)
                    {
                        obj.status = ObjectiveStatus::IN_PROGRESS;
                        obj.retryCount = 0;
                    }
                }
                break;
            }
        }
    }

    // Step 4: Try alternative strategy
    QuestCompletionStrategy currentStrategy = GetQuestCompletionStrategy(botGuid);
    QuestCompletionStrategy newStrategy = currentStrategy;

    // Cycle through strategies based on what's been tried
    switch (currentStrategy)
    {
        case QuestCompletionStrategy::EFFICIENT_COMPLETION:
            newStrategy = QuestCompletionStrategy::SAFE_COMPLETION;
            break;
        case QuestCompletionStrategy::SAFE_COMPLETION:
            newStrategy = QuestCompletionStrategy::THOROUGH_EXPLORATION;
            break;
        case QuestCompletionStrategy::SPEED_COMPLETION:
            newStrategy = QuestCompletionStrategy::EFFICIENT_COMPLETION;
            break;
        default:
            newStrategy = QuestCompletionStrategy::EFFICIENT_COMPLETION;
            break;
    }

    SetQuestCompletionStrategy(botGuid, newStrategy);

    // Step 5: Check if quest is truly uncompletable
    if (diag && diag->isBlocked)
    {
        TC_LOG_WARN("playerbot.quest", "QuestCompletion: Quest {} appears uncompletable - reason: {}",
            questId, diag->blockReason);

        // Get error count from tracking
        uint32 recoveryAttempts = 0;
        {
            std::lock_guard<std::mutex> lock(_errorMutex);
            auto& errors = _botQuestErrors[botGuid];
            for (auto& err : errors)
            {
                if (err.questId == questId)
                    recoveryAttempts = std::max(recoveryAttempts, err.recoveryAttempts + 1);
            }
        }

        // If too many recovery attempts, consider abandonment
        if (recoveryAttempts >= 5)
        {
            TC_LOG_WARN("playerbot.quest", "QuestCompletion: Too many recovery attempts for quest {} - considering abandonment",
                questId);
            AbandonUncompletableQuest(player, questId);
            return;
        }
    }

    // Step 6: Reoptimize quest order
    OptimizeQuestCompletionOrder(player);

    TC_LOG_INFO("playerbot.quest", "QuestCompletion: Recovery complete for quest {} - new strategy: {}",
        questId, static_cast<uint8>(newStrategy));
}

/**
 * @brief Abandon a quest that cannot be completed
 *
 * Safely abandons a quest after verifying it's truly incompletable.
 * Preserves any acquired quest items and logs the abandonment.
 *
 * @param player Bot player
 * @param questId Quest to abandon
 */
void QuestCompletion::AbandonUncompletableQuest(Player* player, uint32 questId)
{
    if (!player)
        return;

    uint32 botGuid = player->GetGUID().GetCounter();

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
    {
        TC_LOG_ERROR("playerbot.quest", "QuestCompletion: Cannot abandon quest {} - quest not found", questId);
        return;
    }

    QuestStatus status = player->GetQuestStatus(questId);
    if (status == QUEST_STATUS_NONE || status == QUEST_STATUS_REWARDED)
    {
        TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Quest {} not in player log - nothing to abandon", questId);
        return;
    }

    // Final diagnostic check
    DiagnoseCompletionIssues(player, questId);

    // Get diagnostic results
    bool shouldAbandon = false;
    std::string abandonReason = "Unknown";

    auto botIt = _questDiagnostics.find(botGuid);
    if (botIt != _questDiagnostics.end())
    {
        auto questIt = botIt->second.find(questId);
        if (questIt != botIt->second.end())
        {
            if (questIt->second.isBlocked)
            {
                shouldAbandon = true;
                abandonReason = questIt->second.blockReason;
            }
        }
    }

    // Also abandon if there are too many unrecoverable errors
    {
        std::lock_guard<std::mutex> lock(_errorMutex);
        auto& errors = _botQuestErrors[botGuid];
        uint32 questErrorCount = std::count_if(errors.begin(), errors.end(),
            [questId](QuestErrorInfo const& e) { return e.questId == questId; });

        if (questErrorCount >= 10)
        {
            shouldAbandon = true;
            abandonReason = "Too many errors (" + std::to_string(questErrorCount) + ")";
        }
    }

    if (!shouldAbandon)
    {
        TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Quest {} does not meet abandonment criteria",
            questId);
        return;
    }

    TC_LOG_WARN("playerbot.quest", "QuestCompletion: Bot {} abandoning quest {} ({}): {}",
        player->GetName(), questId, quest->GetLogTitle(), abandonReason);

    // Remove quest from quest log
    uint16 slot = player->FindQuestSlot(questId);
    if (slot < MAX_QUEST_LOG_SIZE)
    {
        player->SetQuestSlot(slot, 0);

        // Remove associated quest items
        player->TakeQuestSourceItem(questId, true);
    }

    // Clean up internal tracking
    auto progIt = _botQuestProgress.find(botGuid);
    if (progIt != _botQuestProgress.end())
    {
        progIt->second.erase(
            std::remove_if(progIt->second.begin(), progIt->second.end(),
                [questId](QuestProgressData const& p) { return p.questId == questId; }),
            progIt->second.end());
    }

    // Remove from quest order
    {
        std::unique_lock<std::shared_mutex> lock(_questOrderMutex);
        auto& order = _botQuestOrder[botGuid];
        order.erase(std::remove(order.begin(), order.end(), questId), order.end());
    }

    // Clear diagnostics for this quest
    if (botIt != _questDiagnostics.end())
        botIt->second.erase(questId);

    _globalMetrics.questsAbandoned++;

    TC_LOG_INFO("playerbot.quest", "QuestCompletion: Successfully abandoned quest {} for bot {}",
        questId, player->GetName());
}

/**
 * @brief Diagnose completion issues for a specific quest
 *
 * Performs comprehensive analysis of why a quest might be stuck:
 * - Prerequisite quests
 * - Level requirements
 * - Faction requirements
 * - Profession requirements
 * - Item requirements
 * - Missing objective targets
 *
 * @param player Bot player
 * @param questId Quest to diagnose
 */
void QuestCompletion::DiagnoseCompletionIssues(Player* player, uint32 questId)
{
    if (!player)
        return;

    uint32 botGuid = player->GetGUID().GetCounter();

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
    {
        TC_LOG_ERROR("playerbot.quest", "QuestCompletion: Cannot diagnose quest {} - not found", questId);
        return;
    }

    TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Diagnosing quest {} ({}) for {}",
        questId, quest->GetLogTitle(), player->GetName());

    QuestDiagnostics diag;
    diag.questId = questId;
    diag.lastDiagnosticTime = GameTime::GetGameTimeMS();
    diag.isBlocked = false;

    // Check 1: Quest status
    QuestStatus status = player->GetQuestStatus(questId);
    if (status == QUEST_STATUS_NONE)
    {
        diag.issues.push_back("Quest not in player's quest log");
        diag.isBlocked = true;
        diag.blockReason = "Quest not accepted";
    }
    else if (status == QUEST_STATUS_COMPLETE)
    {
        diag.issues.push_back("Quest is complete - needs turn-in");
    }
    else if (status == QUEST_STATUS_REWARDED)
    {
        diag.issues.push_back("Quest already completed and rewarded");
        diag.isBlocked = true;
        diag.blockReason = "Already completed";
    }

    // Check 2: Level requirements (via ContentTuning in 11.x)
    // ContentTuning handles level scaling - we check if quest is too high level
    uint32 contentTuningId = quest->GetContentTuningId();
    if (contentTuningId > 0)
    {
        // Content tuning exists - quest uses level scaling
        // We check if player is below the expected content range
        // For now, we assume level-scaled quests are always doable
        TC_LOG_DEBUG("playerbot.quest", "Quest {} uses ContentTuning {} - level scaling active",
            questId, contentTuningId);
    }

    // Check 3: Prerequisite quests
    int32 prevQuestId = quest->GetPrevQuestId();
    if (prevQuestId != 0)
    {
        if (prevQuestId > 0)
        {
            // Must have completed specific quest
            QuestStatus prevStatus = player->GetQuestStatus(static_cast<uint32>(prevQuestId));
            if (prevStatus != QUEST_STATUS_REWARDED)
            {
                diag.issues.push_back("Prerequisite quest " + std::to_string(prevQuestId) + " not completed");
                diag.isBlocked = true;
                diag.blockReason = "Missing prerequisite quest";
            }
        }
        else
        {
            // Must NOT have completed quest (negative ID)
            QuestStatus prevStatus = player->GetQuestStatus(static_cast<uint32>(-prevQuestId));
            if (prevStatus == QUEST_STATUS_REWARDED)
            {
                diag.issues.push_back("Exclusive quest " + std::to_string(-prevQuestId) + " already completed");
                diag.isBlocked = true;
                diag.blockReason = "Exclusive quest conflict";
            }
        }
    }

    // Check 4: Class/Race requirements
    Trinity::RaceMask<uint64> requiredRaces = quest->GetAllowableRaces();
    if (!requiredRaces.IsEmpty() && !requiredRaces.HasRace(player->GetRace()))
    {
        diag.issues.push_back("Player race not allowed for this quest");
        diag.isBlocked = true;
        diag.blockReason = "Race restriction";
    }

    int32 requiredClasses = quest->GetAllowableClasses();
    if (requiredClasses != 0)
    {
        if (!(requiredClasses & (1 << (player->GetClass() - 1))))
        {
            diag.issues.push_back("Player class not allowed for this quest");
            diag.isBlocked = true;
            diag.blockReason = "Class restriction";
        }
    }

    // Check 5: Required items for turn-in
    auto const& objectives = quest->GetObjectives();
    for (auto const& obj : objectives)
    {
        if (obj.Type == QUEST_OBJECTIVE_ITEM)
        {
            uint32 itemId = obj.ObjectID;
            uint32 requiredCount = obj.Amount;
            uint32 playerCount = player->GetItemCount(itemId, false);

            if (playerCount < requiredCount)
            {
                ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
                std::string itemName = itemTemplate ? itemTemplate->GetName(LOCALE_enUS) : "Unknown Item";
                diag.issues.push_back("Missing item: " + itemName + " (" +
                    std::to_string(playerCount) + "/" + std::to_string(requiredCount) + ")");
            }
        }
    }

    // Check 6: Quest timer
    if (quest->GetLimitTime() > 0)
    {
        uint16 slot = player->FindQuestSlot(questId);
        if (slot < MAX_QUEST_LOG_SIZE)
        {
            int64 endTime = player->GetQuestSlotEndTime(slot);
            int64 currentTime = static_cast<int64>(GameTime::GetGameTime());
            if (endTime > 0 && currentTime > endTime)
            {
                diag.issues.push_back("Quest timer expired");
                diag.isBlocked = true;
                diag.blockReason = "Timer expired";
            }
        }
    }

    // Store diagnostics
    _questDiagnostics[botGuid][questId] = std::move(diag);

    // Log summary
    auto& storedDiag = _questDiagnostics[botGuid][questId];
    if (storedDiag.issues.empty())
    {
        TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: No issues found for quest {}", questId);
    }
    else
    {
        TC_LOG_INFO("playerbot.quest", "QuestCompletion: Quest {} has {} issues, blocked: {}",
            questId, storedDiag.issues.size(), storedDiag.isBlocked);
        for (auto const& issue : storedDiag.issues)
        {
            TC_LOG_DEBUG("playerbot.quest", "  - {}", issue);
        }
    }
}
/**
 * @brief Periodic update for bot quest completion
 *
 * Called on each bot AI update tick. Handles:
 * - Progress tracking updates
 * - Stuck detection
 * - Strategy switching
 * - Quest turn-in triggering
 *
 * @param player Bot player
 * @param diff Time since last update in milliseconds
 */
void QuestCompletion::UpdateBotQuestCompletion(Player* player, uint32 diff)
{
    if (!player)
        return;

    uint32 botGuid = player->GetGUID().GetCounter();

    // Check if bot is paused (e.g., dead)
    // CRITICAL FIX: Use shared_lock for read-only check (allows concurrent readers)
    {
        std::shared_lock<std::shared_mutex> lock(_pausedBotsMutex);
        if (_pausedBots.find(botGuid) != _pausedBots.end())
            return;
    }

    // Throttle updates - only process every 500ms
    static std::unordered_map<uint32, uint32> lastUpdateTime;
    auto& lastUpdate = lastUpdateTime[botGuid];
    lastUpdate += diff;

    if (lastUpdate < 500)
        return;
    lastUpdate = 0;

    // Get current strategy
    QuestCompletionStrategy strategy = GetQuestCompletionStrategy(botGuid);

    // Process quest progress for all active quests
    auto progIt = _botQuestProgress.find(botGuid);
    if (progIt == _botQuestProgress.end() || progIt->second.empty())
    {
        // No tracked quests - scan for new ones
        std::vector<uint32> activeQuests = GetActiveQuests(player);
        if (!activeQuests.empty())
        {
            // Initialize progress tracking for active quests
            for (uint32 questId : activeQuests)
            {
                InitializeQuestProgress(player, questId);
            }
            // Optimize quest order
            OptimizeQuestCompletionOrder(player);
        }
        return;
    }

    // Check for completed quests that need turn-in
    std::vector<uint32> readyForTurnIn;
    for (auto& progress : progIt->second)
    {
        // Update progress percentage
        progress.completionPercentage = CalculateQuestProgress(progress.questId, player);

        // Check if quest is ready for turn-in
        QuestStatus status = player->GetQuestStatus(progress.questId);
        if (status == QUEST_STATUS_COMPLETE)
        {
            readyForTurnIn.push_back(progress.questId);
        }

        // Detect stuck state
        if (!progress.isStuck)
        {
            DetectStuckState(player, progress.questId);
        }
        else
        {
            // Already stuck - try recovery
            RecoverFromStuckState(player, progress.questId);
        }
    }

    // Process turn-ins
    if (!readyForTurnIn.empty())
    {
        // Sort by priority (use quest order if set)
        std::vector<uint32>* questOrder = nullptr;
        {
            std::unique_lock<std::shared_mutex> lock(_questOrderMutex);
            auto orderIt = _botQuestOrder.find(botGuid);
            if (orderIt != _botQuestOrder.end())
                questOrder = &orderIt->second;
        }

        if (questOrder && !questOrder->empty())
        {
            std::sort(readyForTurnIn.begin(), readyForTurnIn.end(),
                [questOrder](uint32 a, uint32 b) {
                    auto posA = std::find(questOrder->begin(), questOrder->end(), a);
                    auto posB = std::find(questOrder->begin(), questOrder->end(), b);
                    if (posA == questOrder->end()) return false;
                    if (posB == questOrder->end()) return true;
                    return posA < posB;
                });
        }

        // Process first turn-in (one at a time)
        ProcessQuestTurnIn(player, readyForTurnIn[0]);
    }

    // Execute strategy for highest priority quest
    uint32 highestPriorityQuest = GetHighestPriorityQuest(player);
    if (highestPriorityQuest > 0)
    {
        // Find progress data for this quest
        for (auto& progress : progIt->second)
        {
            if (progress.questId == highestPriorityQuest)
            {
                // Execute based on current strategy
                switch (strategy)
                {
                    case QuestCompletionStrategy::EFFICIENT_COMPLETION:
                        ExecuteEfficientStrategy(player, progress);
                        break;
                    case QuestCompletionStrategy::SAFE_COMPLETION:
                        ExecuteSafeStrategy(player, progress);
                        break;
                    case QuestCompletionStrategy::GROUP_COORDINATION:
                        ExecuteGroupStrategy(player, progress);
                        break;
                    case QuestCompletionStrategy::SOLO_FOCUS:
                        ExecuteSoloStrategy(player, progress);
                        break;
                    case QuestCompletionStrategy::EXPERIENCE_MAXIMIZING:
                        ExecuteExperienceStrategy(player, progress);
                        break;
                    case QuestCompletionStrategy::SPEED_COMPLETION:
                        ExecuteSpeedStrategy(player, progress);
                        break;
                    case QuestCompletionStrategy::THOROUGH_EXPLORATION:
                        ExecuteExplorationStrategy(player, progress);
                        break;
                }
                break;
            }
        }
    }

    // Handle special quest types
    HandleDailyQuests(player);
    HandleSeasonalQuests(player);
}

/**
 * @brief Validate quest states across all tracked bots
 *
 * Performs global validation:
 * - Removes tracking for completed/abandoned quests
 * - Detects invalid states
 * - Cleans up stale data
 * - Updates global metrics
 */
void QuestCompletion::ValidateQuestStates()
{
    TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Starting quest state validation");

    uint32 removedQuests = 0;
    uint32 invalidStates = 0;
    uint32 activeQuests = 0;

    // Validate all tracked bot quest progress
    for (auto& [botGuid, questList] : _botQuestProgress)
    {
        Player* player = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(botGuid));
        if (!player)
        {
            // Bot no longer exists - clear all tracking
            questList.clear();
            TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Cleared tracking for offline bot {}",
                botGuid);
            continue;
        }

        // Validate each quest's state
        auto it = questList.begin();
        while (it != questList.end())
        {
            uint32 questId = it->questId;
            QuestStatus status = player->GetQuestStatus(questId);

            bool shouldRemove = false;
            std::string removeReason;

            // Check for invalid states
            switch (status)
            {
                case QUEST_STATUS_NONE:
                    // Quest not in log - remove tracking
                    shouldRemove = true;
                    removeReason = "Quest not in player log";
                    break;

                case QUEST_STATUS_REWARDED:
                    // Quest completed - remove tracking
                    shouldRemove = true;
                    removeReason = "Quest already rewarded";
                    break;

                case QUEST_STATUS_INCOMPLETE:
                case QUEST_STATUS_COMPLETE:
                    // Valid states
                    ++activeQuests;
                    break;

                case QUEST_STATUS_FAILED:
                {
                    // Failed quest - check if we should remove or retry
                    Quest const* questTemplate = sObjectMgr->GetQuestTemplate(questId);
                    if (questTemplate && questTemplate->GetLimitTime() > 0)
                    {
                        // Timed quest failed - remove tracking
                        shouldRemove = true;
                        removeReason = "Timed quest failed";
                    }
                    else
                    {
                        // Mark progress as stuck for failed non-timed quests
                        ++invalidStates;
                        it->isStuck = true;
                        it->consecutiveFailures++;
                    }
                    break;
                }
            }

            if (shouldRemove)
            {
                TC_LOG_DEBUG("playerbot.quest", "QuestCompletion: Removing quest {} for bot {}: {}",
                    questId, player->GetName(), removeReason);

                it = questList.erase(it);
                ++removedQuests;

                // Also clean up related data structures
                {
                    std::unique_lock<std::shared_mutex> lock(_questOrderMutex);
                    auto& order = _botQuestOrder[botGuid];
                    order.erase(std::remove(order.begin(), order.end(), questId), order.end());
                }

                {
                    std::unique_lock<std::shared_mutex> lock(_objectiveOrderMutex);
                    _botObjectiveOrder[botGuid].erase(questId);
                }
            }
            else
            {
                // Validate objective states against actual progress
                uint16 slot = player->FindQuestSlot(questId);
                if (slot < MAX_QUEST_LOG_SIZE)
                {
                    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
                    if (quest)
                    {
                        auto const& objectives = quest->GetObjectives();
                        for (auto& objData : it->objectives)
                        {
                            if (objData.objectiveIndex < objectives.size())
                            {
                                auto const& obj = objectives[objData.objectiveIndex];
                                int32 current = player->GetQuestSlotObjectiveData(slot, obj);

                                if (current >= static_cast<int32>(obj.Amount))
                                {
                                    if (objData.status != ObjectiveStatus::COMPLETED)
                                    {
                                        objData.status = ObjectiveStatus::COMPLETED;
                                        TC_LOG_DEBUG("playerbot.quest",
                                            "QuestCompletion: Fixed objective status for quest {} objective {}",
                                            questId, objData.objectiveIndex);
                                    }
                                }
                            }
                        }
                    }
                }
                ++it;
            }
        }
    }

    // Clean up diagnostic data for removed quests
    for (auto& [botGuid, diagMap] : _questDiagnostics)
    {
        auto progIt = _botQuestProgress.find(botGuid);
        if (progIt == _botQuestProgress.end())
        {
            diagMap.clear();
            continue;
        }

        auto diagIt = diagMap.begin();
        while (diagIt != diagMap.end())
        {
            bool found = false;
            for (auto const& prog : progIt->second)
            {
                if (prog.questId == diagIt->first)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
                diagIt = diagMap.erase(diagIt);
            else
                ++diagIt;
        }
    }

    // Clean up error tracking
    {
        std::lock_guard<std::mutex> lock(_errorMutex);
        uint32 currentTime = GameTime::GetGameTimeMS();
        uint32 oneHourAgo = currentTime - 3600000;

        for (auto& [botGuid, errors] : _botQuestErrors)
        {
            // Remove errors older than 1 hour
            errors.erase(
                std::remove_if(errors.begin(), errors.end(),
                    [oneHourAgo](QuestErrorInfo const& e) {
                        return e.timestamp < oneHourAgo;
                    }),
                errors.end());
        }
    }

    // Update metrics
    _globalMetrics.activeQuests = activeQuests;

    TC_LOG_INFO("playerbot.quest",
        "QuestCompletion: Validation complete - {} active quests, {} removed, {} invalid states",
        activeQuests, removedQuests, invalidStates);
}

/**
 * @brief Initialize quest progress tracking for a bot's existing quest
 *
 * This function creates a QuestProgressData entry for quests that the bot
 * already has in their quest log but aren't being tracked yet. This typically
 * happens when:
 * - Bot logs in with quests already in progress
 * - UpdateBotQuestCompletion finds untracked active quests
 * - Manual tracking initialization is needed
 *
 * @param bot Bot player
 * @param questId Quest ID to initialize tracking for
 */
void QuestCompletion::InitializeQuestProgress(Player* bot, uint32 questId)
{
    if (!bot || !questId)
        return;

    uint32 botGuid = bot->GetGUID().GetCounter();

    // Check if already tracking this quest
    auto progIt = _botQuestProgress.find(botGuid);
    if (progIt != _botQuestProgress.end())
    {
        for (auto const& existing : progIt->second)
        {
            if (existing.questId == questId)
            {
                TC_LOG_DEBUG("playerbot.quest",
                    "QuestCompletion::InitializeQuestProgress - Quest {} already tracked for bot {}",
                    questId, bot->GetName());
                return;
            }
        }
    }

    // Verify bot has this quest
    QuestStatus status = bot->GetQuestStatus(questId);
    if (status == QUEST_STATUS_NONE || status == QUEST_STATUS_REWARDED)
    {
        TC_LOG_DEBUG("playerbot.quest",
            "QuestCompletion::InitializeQuestProgress - Bot {} doesn't have quest {} (status: {})",
            bot->GetName(), questId, static_cast<uint32>(status));
        return;
    }

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
    {
        TC_LOG_ERROR("playerbot.quest",
            "QuestCompletion::InitializeQuestProgress - Quest template {} not found",
            questId);
        return;
    }

    // Create new progress data
    QuestProgressData progress(questId, botGuid);

    // Set initial strategy based on bot context
    if (bot->GetGroup())
    {
        progress.strategy = QuestCompletionStrategy::GROUP_COORDINATION;
    }
    else
    {
        // Check if bot has a configured strategy
        auto stratIt = _botStrategies.find(botGuid);
        if (stratIt != _botStrategies.end())
        {
            progress.strategy = stratIt->second;
        }
        else
        {
            progress.strategy = QuestCompletionStrategy::EFFICIENT_COMPLETION;
        }
    }

    // Parse quest objectives from template
    ParseQuestObjectives(progress, quest);

    // Calculate initial progress from bot's current quest state
    uint16 slot = bot->FindQuestSlot(questId);
    if (slot < MAX_QUEST_LOG_SIZE)
    {
        // Get quest objectives from template for API calls
        QuestObjectives const& questObjectives = quest->GetObjectives();

        for (auto& objective : progress.objectives)
        {
            // Find matching quest objective from template for API call
            uint32 current = 0;
            for (auto const& questObj : questObjectives)
            {
                if (questObj.StorageIndex == static_cast<int8>(objective.objectiveIndex))
                {
                    // TrinityCore 12.0 API: GetQuestSlotObjectiveData takes QuestObjective reference
                    current = static_cast<uint32>(std::max(0, bot->GetQuestSlotObjectiveData(slot, questObj)));
                    break;
                }
            }
            objective.currentCount = current;

            // Update status based on progress
            if (current >= objective.requiredCount)
            {
                objective.status = ObjectiveStatus::COMPLETED;
            }
            else if (current > 0)
            {
                objective.status = ObjectiveStatus::IN_PROGRESS;
            }
            else
            {
                objective.status = ObjectiveStatus::NOT_STARTED;
            }
        }

        // Calculate overall completion percentage
        float totalProgress = 0.0f;
        uint32 objectiveCount = 0;
        for (auto const& obj : progress.objectives)
        {
            if (obj.requiredCount > 0)
            {
                totalProgress += std::min(1.0f,
                    static_cast<float>(obj.currentCount) / static_cast<float>(obj.requiredCount));
                ++objectiveCount;
            }
        }

        if (objectiveCount > 0)
        {
            progress.completionPercentage = (totalProgress / objectiveCount) * 100.0f;
        }
    }

    // Mark as ready for turn-in if quest is complete
    if (status == QUEST_STATUS_COMPLETE)
    {
        progress.completionPercentage = 100.0f;
        progress.requiresTurnIn = true;
    }

    // Store bot's last known location
    progress.lastKnownLocation = bot->GetPosition();

    // Find quest giver location for potential turn-in
    // TrinityCore 12.0 API: GetCreatureQuestInvolvedRelationReverseBounds
    auto bounds = sObjectMgr->GetCreatureQuestInvolvedRelationReverseBounds(questId);
    if (bounds.begin() != bounds.end())
    {
        uint32 questEnderEntry = bounds.begin()->second;

        // Try to find nearest spawn of quest ender
        Creature* nearestEnder = bot->FindNearestCreature(questEnderEntry, 1000.0f);
        if (nearestEnder)
        {
            progress.questGiverGuid = nearestEnder->GetGUID().GetCounter();
            progress.questGiverLocation = nearestEnder->GetPosition();
        }
        else
        {
            // Search through all creature spawn data to find nearest spawn
            CreatureData const* nearestSpawn = nullptr;
            float nearestDist = std::numeric_limits<float>::max();

            auto const& allCreatures = sObjectMgr->GetAllCreatureData();
            for (auto const& [spawnId, creatureData] : allCreatures)
            {
                if (creatureData.id == questEnderEntry && creatureData.mapId == bot->GetMapId())
                {
                    float dist = bot->GetDistance(creatureData.spawnPoint);
                    if (dist < nearestDist)
                    {
                        nearestDist = dist;
                        nearestSpawn = &creatureData;
                    }
                }
            }

            if (nearestSpawn)
            {
                progress.questGiverLocation = nearestSpawn->spawnPoint;
            }
        }
    }

    // Store progress data
    _botQuestProgress[botGuid].push_back(progress);

    // Track that this bot is in quest mode
    _botsInQuestMode.insert(botGuid);

    // Log initialization
    TC_LOG_DEBUG("playerbot.quest",
        "QuestCompletion::InitializeQuestProgress - Initialized quest {} for bot {} ({:.1f}% complete, {} objectives)",
        questId, bot->GetName(), progress.completionPercentage, progress.objectives.size());

    // Notify via EventBus that a new quest is being tracked
    QuestEvent trackingEvent;
    trackingEvent.type = QuestEventType::QUEST_OBJECTIVE_COMPLETE;  // Use existing enum value
    trackingEvent.playerGuid = bot->GetGUID();
    trackingEvent.questId = questId;
    EventBus<QuestEvent>::instance()->PublishEvent(trackingEvent);
}

// ============================================================================
// NEW TRINITYCORE OBJECTIVE HANDLERS (Phase 1 Quest System Completion)
// ============================================================================

/**
 * @brief Handle currency spending objectives (QUEST_OBJECTIVE_CURRENCY)
 *
 * The bot needs to spend a specific amount of currency. This typically means
 * interacting with an NPC vendor or using a currency-consuming item/ability.
 *
 * @param bot The bot player
 * @param objective The objective data containing currency ID and amount
 */
void QuestCompletion::HandleCurrencyObjective(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return;

    uint32 currencyId = objective.targetId;
    uint32 requiredAmount = objective.requiredCount;

    // Check current currency amount
    uint32 currentAmount = bot->GetCurrencyQuantity(currencyId);

    TC_LOG_DEBUG("playerbot.quest", "HandleCurrencyObjective: Bot {} needs to spend {} of currency {}, has {}",
        bot->GetName(), requiredAmount, currencyId, currentAmount);

    // If bot has enough currency, the objective should auto-complete when they spend it
    // This is typically handled by the quest system itself when interacting with vendors
    if (currentAmount >= requiredAmount)
    {
        // Bot has the currency - quest system will track spending
        objective.currentCount = std::min(currentAmount, requiredAmount);
        TC_LOG_DEBUG("playerbot.quest", "HandleCurrencyObjective: Bot {} has enough currency, waiting for spending action",
            bot->GetName());
    }
    else
    {
        // Bot needs to earn more currency first
        TC_LOG_DEBUG("playerbot.quest", "HandleCurrencyObjective: Bot {} needs to earn {} more currency {}",
            bot->GetName(), requiredAmount - currentAmount, currencyId);
        objective.status = ObjectiveStatus::BLOCKED;
    }
}

/**
 * @brief Handle spell learning objectives (QUEST_OBJECTIVE_LEARNSPELL)
 *
 * The bot needs to learn a specific spell, usually from a class/profession trainer.
 *
 * @param bot The bot player
 * @param objective The objective data containing spell ID
 */
void QuestCompletion::HandleLearnSpellObjective(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return;

    uint32 spellId = objective.targetId;

    // Check if bot already knows the spell
    if (bot->HasSpell(spellId))
    {
        objective.currentCount = objective.requiredCount;
        objective.status = ObjectiveStatus::COMPLETED;
        TC_LOG_DEBUG("playerbot.quest", "HandleLearnSpellObjective: Bot {} already knows spell {}",
            bot->GetName(), spellId);
        return;
    }

    // Find a trainer that can teach this spell
    Creature* trainer = FindTrainerForSpell(bot, spellId);
    if (trainer)
    {
        // Navigate to trainer
        float dist = bot->GetDistance(trainer);
        if (dist > QUEST_GIVER_INTERACTION_RANGE)
        {
            TC_LOG_DEBUG("playerbot.quest", "HandleLearnSpellObjective: Bot {} navigating to trainer {} for spell {}",
                bot->GetName(), trainer->GetName(), spellId);
            BotMovementUtil::MoveToPosition(bot, trainer->GetPosition(), 2.0f);
            return;
        }

        // At trainer - attempt to learn spell
        TC_LOG_DEBUG("playerbot.quest", "HandleLearnSpellObjective: Bot {} attempting to learn spell {} from {}",
            bot->GetName(), spellId, trainer->GetName());

        // Simulate trainer interaction - in a full implementation, this would use
        // the proper gossip/trainer interaction system
        if (bot->HasEnoughMoney(static_cast<uint64>(0))) // Trainer spells might cost gold
        {
            bot->LearnSpell(spellId, false);
            if (bot->HasSpell(spellId))
            {
                objective.currentCount = objective.requiredCount;
                objective.status = ObjectiveStatus::COMPLETED;
                TC_LOG_DEBUG("playerbot.quest", "HandleLearnSpellObjective: Bot {} learned spell {}",
                    bot->GetName(), spellId);
            }
        }
    }
    else
    {
        TC_LOG_WARN("playerbot.quest", "HandleLearnSpellObjective: Could not find trainer for spell {} for bot {}",
            spellId, bot->GetName());
        objective.status = ObjectiveStatus::BLOCKED;
    }
}

/**
 * @brief Handle minimum reputation objectives (QUEST_OBJECTIVE_MIN_REPUTATION)
 *
 * The bot needs to reach a minimum reputation level with a specific faction.
 *
 * @param bot The bot player
 * @param objective The objective data containing faction ID and required standing
 */
void QuestCompletion::HandleMinReputationObjective(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return;

    uint32 factionId = objective.targetId;
    int32 requiredStanding = static_cast<int32>(objective.requiredCount);

    // Get current reputation with faction
    FactionEntry const* factionEntry = sFactionStore.LookupEntry(factionId);
    if (!factionEntry)
    {
        TC_LOG_ERROR("playerbot.quest", "HandleMinReputationObjective: Invalid faction ID {} for quest {}",
            factionId, objective.questId);
        objective.status = ObjectiveStatus::FAILED;
        return;
    }

    ReputationRank currentRank = bot->GetReputationMgr().GetRank(factionEntry);
    int32 currentStanding = bot->GetReputationMgr().GetReputation(factionEntry);

    TC_LOG_DEBUG("playerbot.quest", "HandleMinReputationObjective: Bot {} has {} rep with faction {}, needs {}",
        bot->GetName(), currentStanding, factionId, requiredStanding);

    if (currentStanding >= requiredStanding)
    {
        objective.currentCount = objective.requiredCount;
        objective.status = ObjectiveStatus::COMPLETED;
        TC_LOG_DEBUG("playerbot.quest", "HandleMinReputationObjective: Bot {} has met reputation requirement",
            bot->GetName());
    }
    else
    {
        // Bot needs to gain reputation - find quests or mobs that give rep
        uint32 repSource = FindReputationSource(bot, factionId);
        if (repSource != 0)
        {
            TC_LOG_DEBUG("playerbot.quest", "HandleMinReputationObjective: Bot {} should grind rep via source {}",
                bot->GetName(), repSource);
        }
        objective.status = ObjectiveStatus::IN_PROGRESS;
    }
}

/**
 * @brief Handle maximum reputation objectives (QUEST_OBJECTIVE_MAX_REPUTATION)
 *
 * The bot must NOT exceed a maximum reputation level with a faction.
 * This is rare and usually for faction-specific quests.
 *
 * @param bot The bot player
 * @param objective The objective data containing faction ID and max standing
 */
void QuestCompletion::HandleMaxReputationObjective(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return;

    uint32 factionId = objective.targetId;
    int32 maxStanding = static_cast<int32>(objective.requiredCount);

    FactionEntry const* factionEntry = sFactionStore.LookupEntry(factionId);
    if (!factionEntry)
    {
        TC_LOG_ERROR("playerbot.quest", "HandleMaxReputationObjective: Invalid faction ID {} for quest {}",
            factionId, objective.questId);
        objective.status = ObjectiveStatus::FAILED;
        return;
    }

    int32 currentStanding = bot->GetReputationMgr().GetReputation(factionEntry);

    TC_LOG_DEBUG("playerbot.quest", "HandleMaxReputationObjective: Bot {} has {} rep with faction {}, max allowed {}",
        bot->GetName(), currentStanding, factionId, maxStanding);

    if (currentStanding <= maxStanding)
    {
        objective.currentCount = objective.requiredCount;
        objective.status = ObjectiveStatus::COMPLETED;
    }
    else
    {
        // Bot has too much reputation - this objective cannot be completed
        TC_LOG_WARN("playerbot.quest", "HandleMaxReputationObjective: Bot {} has too much rep ({} > {}), quest may be uncompletable",
            bot->GetName(), currentStanding, maxStanding);
        objective.status = ObjectiveStatus::FAILED;
    }
}

/**
 * @brief Handle money/gold objectives (QUEST_OBJECTIVE_MONEY)
 *
 * The bot needs to have a specific amount of gold.
 *
 * @param bot The bot player
 * @param objective The objective data containing required copper amount
 */
void QuestCompletion::HandleMoneyObjective(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return;

    uint32 requiredMoney = objective.requiredCount;  // In copper
    uint32 currentMoney = bot->GetMoney();

    TC_LOG_DEBUG("playerbot.quest", "HandleMoneyObjective: Bot {} has {} copper, needs {}",
        bot->GetName(), currentMoney, requiredMoney);

    if (currentMoney >= requiredMoney)
    {
        objective.currentCount = objective.requiredCount;
        objective.status = ObjectiveStatus::COMPLETED;
        TC_LOG_DEBUG("playerbot.quest", "HandleMoneyObjective: Bot {} has enough gold",
            bot->GetName());
    }
    else
    {
        // Bot needs to earn more gold
        uint32 needed = requiredMoney - currentMoney;
        TC_LOG_DEBUG("playerbot.quest", "HandleMoneyObjective: Bot {} needs {} more copper",
            bot->GetName(), needed);
        objective.status = ObjectiveStatus::IN_PROGRESS;
        // Bot should prioritize gold-earning activities (vendoring, questing, etc.)
    }
}

/**
 * @brief Handle player kill objectives (QUEST_OBJECTIVE_PLAYERKILLS)
 *
 * The bot needs to kill other players in PvP combat.
 *
 * @param bot The bot player
 * @param objective The objective data containing required kill count
 */
void QuestCompletion::HandlePlayerKillsObjective(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return;

    TC_LOG_DEBUG("playerbot.quest", "HandlePlayerKillsObjective: Bot {} needs {} player kills for quest {}",
        bot->GetName(), objective.requiredCount, objective.questId);

    // Check if bot is already in a PvP environment
    if (bot->InBattleground() || bot->InArena())
    {
        TC_LOG_DEBUG("playerbot.quest", "HandlePlayerKillsObjective: Bot {} is in PvP instance, kills will count",
            bot->GetName());
        objective.status = ObjectiveStatus::IN_PROGRESS;
        return;
    }

    // Queue for battleground to complete this objective
    StartPvPActivity(bot);
    objective.status = ObjectiveStatus::IN_PROGRESS;
}

/**
 * @brief Handle area trigger objectives (QUEST_OBJECTIVE_AREATRIGGER)
 *
 * The bot needs to enter or pass through a specific area trigger.
 *
 * @param bot The bot player
 * @param objective The objective data containing area trigger ID
 */
void QuestCompletion::HandleAreaTriggerObjective(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return;

    uint32 areaTriggerIdOrObjectId = objective.targetId;

    TC_LOG_DEBUG("playerbot.quest", "HandleAreaTriggerObjective: Bot {} needs to reach area trigger/location {}",
        bot->GetName(), areaTriggerIdOrObjectId);

    // Find the position of the area trigger
    Position targetPos = FindAreaTriggerPosition(areaTriggerIdOrObjectId);

    if (targetPos.GetPositionX() == 0.0f && targetPos.GetPositionY() == 0.0f)
    {
        TC_LOG_WARN("playerbot.quest", "HandleAreaTriggerObjective: Could not find position for area trigger {}",
            areaTriggerIdOrObjectId);
        objective.status = ObjectiveStatus::BLOCKED;
        return;
    }

    // Navigate to the area trigger
    float dist = bot->GetDistance(targetPos);
    if (dist > 10.0f)
    {
        TC_LOG_DEBUG("playerbot.quest", "HandleAreaTriggerObjective: Bot {} navigating to area trigger at ({}, {}, {})",
            bot->GetName(), targetPos.GetPositionX(), targetPos.GetPositionY(), targetPos.GetPositionZ());
        BotMovementUtil::MoveToPosition(bot, targetPos, 5.0f);
        objective.status = ObjectiveStatus::IN_PROGRESS;
    }
    else
    {
        // Should be triggered automatically by the game when entering the area
        TC_LOG_DEBUG("playerbot.quest", "HandleAreaTriggerObjective: Bot {} is at area trigger location",
            bot->GetName());
    }
}

/**
 * @brief Handle pet battle vs NPC objectives (QUEST_OBJECTIVE_WINPETBATTLEAGAINSTNPC)
 *
 * The bot needs to win a pet battle against a specific NPC trainer.
 * Note: Pet battles are a complex system that may not be fully implemented for bots.
 *
 * @param bot The bot player
 * @param objective The objective data containing NPC entry ID
 */
void QuestCompletion::HandlePetBattleNPCObjective(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return;

    uint32 npcEntry = objective.targetId;

    TC_LOG_DEBUG("playerbot.quest", "HandlePetBattleNPCObjective: Bot {} needs to defeat pet battle trainer entry {}",
        bot->GetName(), npcEntry);

    // Find the NPC
    Creature* petTrainer = bot->FindNearestCreature(npcEntry, 100.0f);
    if (!petTrainer)
    {
        // Search in creature spawn data
        TC_LOG_DEBUG("playerbot.quest", "HandlePetBattleNPCObjective: Pet trainer {} not nearby, searching world",
            npcEntry);
        objective.status = ObjectiveStatus::BLOCKED;
        return;
    }

    // Navigate to the pet battle trainer
    float dist = bot->GetDistance(petTrainer);
    if (dist > QUEST_GIVER_INTERACTION_RANGE)
    {
        BotMovementUtil::MoveToPosition(bot, petTrainer->GetPosition(), 2.0f);
        objective.status = ObjectiveStatus::IN_PROGRESS;
        return;
    }

    // Pet battle system interaction would go here
    // This requires the full pet battle system to be implemented for bots
    TC_LOG_WARN("playerbot.quest", "HandlePetBattleNPCObjective: Pet battle system not fully implemented for bots");
    objective.status = ObjectiveStatus::BLOCKED;
}

/**
 * @brief Handle defeat battle pet objectives (QUEST_OBJECTIVE_DEFEATBATTLEPET)
 *
 * The bot needs to defeat a specific wild battle pet.
 *
 * @param bot The bot player
 * @param objective The objective data containing battle pet species ID
 */
void QuestCompletion::HandleDefeatBattlePetObjective(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return;

    uint32 speciesId = objective.targetId;

    TC_LOG_DEBUG("playerbot.quest", "HandleDefeatBattlePetObjective: Bot {} needs to defeat battle pet species {}",
        bot->GetName(), speciesId);

    // Wild battle pets are complex - requires pet battle system
    TC_LOG_WARN("playerbot.quest", "HandleDefeatBattlePetObjective: Pet battle system not fully implemented for bots");
    objective.status = ObjectiveStatus::BLOCKED;
}

/**
 * @brief Handle PvP pet battle objectives (QUEST_OBJECTIVE_WINPVPPETBATTLES)
 *
 * The bot needs to win PvP pet battles against other players.
 *
 * @param bot The bot player
 * @param objective The objective data containing required win count
 */
void QuestCompletion::HandlePvPPetBattlesObjective(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return;

    TC_LOG_DEBUG("playerbot.quest", "HandlePvPPetBattlesObjective: Bot {} needs {} PvP pet battle wins",
        bot->GetName(), objective.requiredCount);

    // PvP pet battles require matching system and bot-vs-player pet battles
    TC_LOG_WARN("playerbot.quest", "HandlePvPPetBattlesObjective: PvP pet battle system not implemented for bots");
    objective.status = ObjectiveStatus::BLOCKED;
}

/**
 * @brief Handle criteria tree objectives (QUEST_OBJECTIVE_CRITERIA_TREE)
 *
 * The bot needs to complete achievement-like criteria, typically tracked by
 * the game's achievement/criteria system.
 *
 * @param bot The bot player
 * @param objective The objective data containing criteria tree ID
 */
void QuestCompletion::HandleCriteriaTreeObjective(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return;

    uint32 criteriaTreeId = objective.targetId;

    TC_LOG_DEBUG("playerbot.quest", "HandleCriteriaTreeObjective: Bot {} needs to complete criteria tree {}",
        bot->GetName(), criteriaTreeId);

    // Check if criteria is already completed
    // The criteria system tracks this automatically
    if (bot->IsQuestObjectiveComplete(objective.questId, objective.objectiveIndex))
    {
        objective.currentCount = objective.requiredCount;
        objective.status = ObjectiveStatus::COMPLETED;
        TC_LOG_DEBUG("playerbot.quest", "HandleCriteriaTreeObjective: Criteria tree {} already completed",
            criteriaTreeId);
    }
    else
    {
        // Criteria progress is typically tracked automatically by various game actions
        TC_LOG_DEBUG("playerbot.quest", "HandleCriteriaTreeObjective: Waiting for criteria tree {} progress",
            criteriaTreeId);
        objective.status = ObjectiveStatus::IN_PROGRESS;
    }
}

/**
 * @brief Handle progress bar objectives (QUEST_OBJECTIVE_PROGRESS_BAR)
 *
 * The bot needs to fill a progress bar through various actions.
 * The specific actions depend on the quest design.
 *
 * @param bot The bot player
 * @param objective The objective data
 */
void QuestCompletion::HandleProgressBarObjective(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return;

    TC_LOG_DEBUG("playerbot.quest", "HandleProgressBarObjective: Bot {} working on progress bar objective for quest {}",
        bot->GetName(), objective.questId);

    // Progress bar objectives are typically filled by:
    // - Killing mobs in an area
    // - Interacting with objects
    // - Being in a specific area
    // - Time-based progression

    // Check current progress via the quest system
    Quest const* quest = sObjectMgr->GetQuestTemplate(objective.questId);
    if (quest)
    {
        QuestObjectives const& questObjectives = quest->GetObjectives();
        if (objective.objectiveIndex < questObjectives.size())
        {
            QuestObjective const& questObj = questObjectives[objective.objectiveIndex];
            objective.currentCount = bot->GetQuestObjectiveData(questObj);

            if (objective.currentCount >= objective.requiredCount)
            {
                objective.status = ObjectiveStatus::COMPLETED;
            }
            else
            {
                objective.status = ObjectiveStatus::IN_PROGRESS;
            }
        }
    }
}

/**
 * @brief Handle have currency objectives (QUEST_OBJECTIVE_HAVE_CURRENCY)
 *
 * The bot must have X currency when turning in the quest (not consumed).
 *
 * @param bot The bot player
 * @param objective The objective data containing currency ID and amount
 */
void QuestCompletion::HandleHaveCurrencyObjective(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return;

    uint32 currencyId = objective.targetId;
    uint32 requiredAmount = objective.requiredCount;

    uint32 currentAmount = bot->GetCurrencyQuantity(currencyId);

    TC_LOG_DEBUG("playerbot.quest", "HandleHaveCurrencyObjective: Bot {} has {} of currency {}, needs {}",
        bot->GetName(), currentAmount, currencyId, requiredAmount);

    if (currentAmount >= requiredAmount)
    {
        objective.currentCount = objective.requiredCount;
        objective.status = ObjectiveStatus::COMPLETED;
    }
    else
    {
        // Bot needs to earn more currency
        objective.currentCount = currentAmount;
        objective.status = ObjectiveStatus::IN_PROGRESS;
        TC_LOG_DEBUG("playerbot.quest", "HandleHaveCurrencyObjective: Bot {} needs {} more currency",
            bot->GetName(), requiredAmount - currentAmount);
    }
}

/**
 * @brief Handle obtain currency objectives (QUEST_OBJECTIVE_OBTAIN_CURRENCY)
 *
 * The bot needs to obtain X currency after starting the quest (tracked separately).
 *
 * @param bot The bot player
 * @param objective The objective data containing currency ID and amount
 */
void QuestCompletion::HandleObtainCurrencyObjective(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return;

    uint32 currencyId = objective.targetId;

    TC_LOG_DEBUG("playerbot.quest", "HandleObtainCurrencyObjective: Bot {} needs to obtain {} of currency {}",
        bot->GetName(), objective.requiredCount, currencyId);

    // This objective tracks currency GAINED after quest acceptance
    // The quest system tracks this automatically
    Quest const* quest = sObjectMgr->GetQuestTemplate(objective.questId);
    if (quest)
    {
        QuestObjectives const& questObjectives = quest->GetObjectives();
        if (objective.objectiveIndex < questObjectives.size())
        {
            QuestObjective const& questObj = questObjectives[objective.objectiveIndex];
            objective.currentCount = bot->GetQuestObjectiveData(questObj);

            if (objective.currentCount >= objective.requiredCount)
            {
                objective.status = ObjectiveStatus::COMPLETED;
            }
            else
            {
                objective.status = ObjectiveStatus::IN_PROGRESS;
            }
        }
    }
}

/**
 * @brief Handle increase reputation objectives (QUEST_OBJECTIVE_INCREASE_REPUTATION)
 *
 * The bot needs to gain X reputation with a faction (tracked from quest acceptance).
 *
 * @param bot The bot player
 * @param objective The objective data containing faction ID and amount to gain
 */
void QuestCompletion::HandleIncreaseReputationObjective(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return;

    uint32 factionId = objective.targetId;

    TC_LOG_DEBUG("playerbot.quest", "HandleIncreaseReputationObjective: Bot {} needs to gain {} rep with faction {}",
        bot->GetName(), objective.requiredCount, factionId);

    // Reputation gain is tracked automatically by the quest system from acceptance
    Quest const* quest = sObjectMgr->GetQuestTemplate(objective.questId);
    if (quest)
    {
        QuestObjectives const& questObjectives = quest->GetObjectives();
        if (objective.objectiveIndex < questObjectives.size())
        {
            QuestObjective const& questObj = questObjectives[objective.objectiveIndex];
            objective.currentCount = bot->GetQuestObjectiveData(questObj);

            if (objective.currentCount >= objective.requiredCount)
            {
                objective.status = ObjectiveStatus::COMPLETED;
            }
            else
            {
                // Bot should do activities that grant rep with this faction
                uint32 repSource = FindReputationSource(bot, factionId);
                objective.status = ObjectiveStatus::IN_PROGRESS;
            }
        }
    }
}

/**
 * @brief Handle area trigger enter objectives (QUEST_OBJECTIVE_AREA_TRIGGER_ENTER)
 *
 * The bot needs to enter a specific area trigger zone.
 *
 * @param bot The bot player
 * @param objective The objective data containing area trigger ID
 */
void QuestCompletion::HandleAreaTriggerEnterObjective(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return;

    // Use the generic area trigger handler
    HandleAreaTriggerObjective(bot, objective);
}

/**
 * @brief Handle area trigger exit objectives (QUEST_OBJECTIVE_AREA_TRIGGER_EXIT)
 *
 * The bot needs to exit a specific area trigger zone.
 *
 * @param bot The bot player
 * @param objective The objective data containing area trigger ID
 */
void QuestCompletion::HandleAreaTriggerExitObjective(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return;

    uint32 areaTriggerIdOrObjectId = objective.targetId;

    TC_LOG_DEBUG("playerbot.quest", "HandleAreaTriggerExitObjective: Bot {} needs to exit area trigger {}",
        bot->GetName(), areaTriggerIdOrObjectId);

    // For exit objectives, bot needs to first enter then leave the area
    // The game tracks this automatically once the bot leaves
    Position targetPos = FindAreaTriggerPosition(areaTriggerIdOrObjectId);

    if (targetPos.GetPositionX() != 0.0f || targetPos.GetPositionY() != 0.0f)
    {
        // Check if we're inside the area trigger
        float dist = bot->GetDistance(targetPos);
        if (dist < 20.0f)
        {
            // Bot is near/in the area - move away to trigger exit
            Position awayPos = bot->GetPosition();
            awayPos.m_positionX += 30.0f;  // Move 30 yards away
            BotMovementUtil::MoveToPosition(bot, awayPos, 5.0f);
        }
    }

    objective.status = ObjectiveStatus::IN_PROGRESS;
}

/**
 * @brief Handle kill with label objectives (QUEST_OBJECTIVE_KILL_WITH_LABEL)
 *
 * The bot needs to kill creatures with a specific label/tag.
 * This is similar to MONSTER but targets creatures by a label criteria.
 *
 * @param bot The bot player
 * @param objective The objective data containing label information
 */
void QuestCompletion::HandleKillWithLabelObjective(Player* bot, QuestObjectiveData& objective)
{
    if (!bot)
        return;

    TC_LOG_DEBUG("playerbot.quest", "HandleKillWithLabelObjective: Bot {} needs to kill {} creatures with label for quest {}",
        bot->GetName(), objective.requiredCount, objective.questId);

    // Kill with label works like normal kill objectives but may target multiple creature types
    // that share a label. The quest system tracks kills automatically.
    HandleKillObjective(bot, objective);
}

// ============================================================================
// HELPER METHODS FOR OBJECTIVE HANDLERS
// ============================================================================

/**
 * @brief Find a trainer that can teach a specific spell
 *
 * @param bot The bot player
 * @param spellId The spell ID to learn
 * @return Trainer creature or nullptr if not found
 */
Creature* QuestCompletion::FindTrainerForSpell(Player* bot, uint32 spellId)
{
    if (!bot)
        return nullptr;

    // First check nearby trainers
    std::list<Creature*> creatureList;
    Trinity::AnyUnitInObjectRangeCheck check(bot, 100.0f);
    Trinity::CreatureListSearcher<Trinity::AnyUnitInObjectRangeCheck> searcher(bot, creatureList, check);
    Cell::VisitAllObjects(bot, searcher, 100.0f);

    for (Creature* creature : creatureList)
    {
        if (!creature->IsTrainer())
            continue;

        // Check if this creature is a trainer that can train the bot
        // Use GetTrainerId() to verify it's a trainer
        if (creature->GetTrainerId() > 0)
        {
            TC_LOG_DEBUG("playerbot.quest", "FindTrainerForSpell: Found potential trainer {} (ID: {}) for spell {}",
                creature->GetName(), creature->GetTrainerId(), spellId);
            return creature;
        }
    }

    TC_LOG_DEBUG("playerbot.quest", "FindTrainerForSpell: No nearby trainer found for spell {}", spellId);
    return nullptr;
}

/**
 * @brief Start PvP activity for player kills objective
 *
 * @param bot The bot player
 */
void QuestCompletion::StartPvPActivity(Player* bot)
{
    if (!bot)
        return;

    TC_LOG_DEBUG("playerbot.quest", "StartPvPActivity: Bot {} starting PvP activity", bot->GetName());

    // Enable PvP flag if not already
    if (!bot->IsPvP())
    {
        bot->SetPvP(true);
    }

    // Queue for a random battleground
    // In a full implementation, this would use the battleground queue system
    QueueForBattleground(bot, 0);  // 0 = random BG
}

/**
 * @brief Start dungeon activity for dungeon objectives
 *
 * @param bot The bot player
 */
void QuestCompletion::StartDungeonActivity(Player* bot)
{
    if (!bot)
        return;

    TC_LOG_DEBUG("playerbot.quest", "StartDungeonActivity: Bot {} should queue for dungeon", bot->GetName());
    // Dungeon finder queue would be implemented here
}

/**
 * @brief Queue bot for a specific battleground
 *
 * @param bot The bot player
 * @param bgTypeId Battleground type ID (0 = random)
 */
void QuestCompletion::QueueForBattleground(Player* bot, uint32 bgTypeId)
{
    if (!bot)
        return;

    TC_LOG_DEBUG("playerbot.quest", "QueueForBattleground: Bot {} queuing for BG type {}", bot->GetName(), bgTypeId);
    // Battleground queue implementation would go here
    // This requires integration with the BG queue system
}

/**
 * @brief Find area trigger position by ID
 *
 * @param areaTriggerIdOrObjectId Area trigger ID or related object ID
 * @return Position of the area trigger
 */
Position QuestCompletion::FindAreaTriggerPosition(uint32 areaTriggerIdOrObjectId)
{
    // Look up area trigger in the database
    AreaTriggerEntry const* areaTrigger = sAreaTriggerStore.LookupEntry(areaTriggerIdOrObjectId);
    if (areaTrigger)
    {
        Position pos;
        pos.m_positionX = areaTrigger->Pos.X;
        pos.m_positionY = areaTrigger->Pos.Y;
        pos.m_positionZ = areaTrigger->Pos.Z;
        pos.SetOrientation(0.0f);
        return pos;
    }

    // Return empty position if not found
    return Position();
}

/**
 * @brief Check if bot has enough currency
 *
 * @param bot The bot player
 * @param currencyId Currency type ID
 * @param amount Required amount
 * @return True if bot has enough
 */
bool QuestCompletion::HasEnoughCurrency(Player* bot, uint32 currencyId, uint32 amount)
{
    if (!bot)
        return false;

    return bot->GetCurrencyQuantity(currencyId) >= amount;
}

/**
 * @brief Find best reputation source for a faction
 *
 * @param bot The bot player
 * @param factionId Faction ID
 * @return Quest ID or creature ID that provides reputation, 0 if none found
 */
uint32 QuestCompletion::FindReputationSource(Player* bot, uint32 factionId)
{
    if (!bot)
        return 0;

    // This would search for:
    // 1. Quests that reward reputation with this faction
    // 2. Creatures that give rep when killed
    // 3. Items that can be turned in for rep

    TC_LOG_DEBUG("playerbot.quest", "FindReputationSource: Searching for rep sources for faction {}", factionId);

    // For now, return 0 - a full implementation would query the quest/creature databases
    return 0;
}

} // namespace Playerbot
