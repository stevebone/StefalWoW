/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "QuestTurnIn.h"
#include "Log.h"
#include "GameTime.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "World.h"
#include "WorldSession.h"
#include "Group.h"
#include "GossipDef.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "QuestPickup.h"
#include "QuestValidation.h"
#include "Movement/BotMovementUtil.h"
#include "Interaction/Core/InteractionManager.h"
#include "../AI/BotAI.h"
#include "Config/PlayerbotConfig.h"
#include <algorithm>
#include <cmath>
#include "../Spatial/SpatialGridManager.h"  // Lock-free spatial grid for deadlock fix

namespace Playerbot
{

/**
 * @brief Constructor
 */
QuestTurnIn::QuestTurnIn(Player* bot)
    : _bot(bot)
{
    if (!_bot)
        TC_LOG_ERROR("playerbot.quest", "QuestTurnIn: null bot!");
    _globalMetrics.Reset();
    LoadQuestGiverDatabase();
}

QuestTurnIn::~QuestTurnIn() {}

/**
 * @brief Turn in a quest for the bot
 * @param questId Quest ID to turn in
 * @param bot Bot player
 * @return True if quest was turned in successfully
 */
bool QuestTurnIn::TurnInQuest(uint32 questId, Player* bot)
{
    if (!bot || !questId)
        return false;

    // Validate quest is ready for turn-in
    if (!IsQuestReadyForTurnIn(questId, bot))
    {
        TC_LOG_DEBUG("playerbot", "QuestTurnIn::TurnInQuest - Quest %u not ready for turn-in for bot %s",
            questId, bot->GetName().c_str());
        return false;
    }

    // Find quest turn-in NPC
    if (!FindQuestTurnInNpc(bot, questId))
    {
        TC_LOG_DEBUG("playerbot", "QuestTurnIn::TurnInQuest - Cannot find turn-in NPC for quest %u", questId);
        HandleQuestGiverNotFound(bot, questId);
        return false;
    }

    // Process the turn-in
    ProcessQuestTurnIn(bot, questId);

    return true;
}

/**
 * @brief Process quest turn-in workflow
 * @param bot Bot player
 * @param questId Quest ID
 */
void QuestTurnIn::ProcessQuestTurnIn(Player* bot, uint32 questId)
{
    if (!bot || !questId)
        return;

    // Initialize turn-in data
    InitializeTurnInData(bot, questId);

    auto it = _botTurnInQueues.find(bot->GetGUID().GetCounter());
    if (it == _botTurnInQueues.end())
        return;

    auto turnInIt = std::find_if(it->second.begin(), it->second.end(),
        [questId](const QuestTurnInData& data) { return data.questId == questId; });

    if (turnInIt == it->second.end())
        return;

    // Execute turn-in workflow
    ExecuteTurnInWorkflow(bot, *turnInIt);
}

/**
 * @brief Process batch turn-in for multiple quests
 * @param bot Bot player
 * @param batch Batch of quests to turn in
 */
void QuestTurnIn::ProcessBatchTurnIn(Player* bot, const TurnInBatch& batch)
{
    if (!bot || batch.questIds.empty())
        return;

    TC_LOG_DEBUG("playerbot", "QuestTurnIn::ProcessBatchTurnIn - Processing batch of %zu quests for bot %s",
        batch.questIds.size(), bot->GetName().c_str());

    // Optimize route for batch turn-in
    std::vector<uint32> optimizedOrder = batch.questIds;
    OptimizeTravelRoute(bot, optimizedOrder);

    // Process each quest in optimized order
    for (uint32 questId : optimizedOrder)
    {
        TurnInQuest(questId, bot);
    }

    // Update metrics
    _globalMetrics.questsTurnedIn += batch.questIds.size();
    _botMetrics[bot->GetGUID().GetCounter()].questsTurnedIn += batch.questIds.size();
}

/**
 * @brief Schedule a quest turn-in for later
 * @param bot Bot player
 * @param questId Quest ID
 * @param delayMs Delay in milliseconds
 */
void QuestTurnIn::ScheduleQuestTurnIn(Player* bot, uint32 questId, uint32 delayMs)
{
    if (!bot || !questId)
        return;

    // Add to scheduled turn-ins
    _scheduledTurnIns.push({ bot->GetGUID().GetCounter(), questId });

    TC_LOG_DEBUG("playerbot", "QuestTurnIn::ScheduleQuestTurnIn - Scheduled quest %u for bot %s with %u ms delay",
        questId, bot->GetName().c_str(), delayMs);
}

/**
 * @brief Get list of completed quests for bot
 * @param bot Bot player
 * @return Vector of completed quest IDs
 */
std::vector<uint32> QuestTurnIn::GetCompletedQuests(Player* bot)
{
    std::vector<uint32> completedQuests;

    if (!bot)
        return completedQuests;

    // Check all quests in quest log
    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = bot->GetQuestSlotQuestId(slot);
        if (!questId)
            continue;

        if (bot->CanCompleteQuest(questId))
            completedQuests.push_back(questId);
    }

    return completedQuests;
}

/**
 * @brief Check if quest is ready for turn-in
 * @param questId Quest ID
 * @param bot Bot player
 * @return True if quest can be turned in
 */
bool QuestTurnIn::IsQuestReadyForTurnIn(uint32 questId, Player* bot)
{
    if (!bot || !questId)
        return false;

    // Check quest status
    QuestStatus status = bot->GetQuestStatus(questId);
    if (status != QUEST_STATUS_COMPLETE)
        return false;

    // Check if bot can complete the quest
    if (!bot->CanCompleteQuest(questId))
        return false;

    // Validate quest state
    if (!ValidateQuestTurnIn(bot, questId))
        return false;
    return true;
}

/**
 * @brief Monitor quest completion status
 * @param bot Bot player
 */
void QuestTurnIn::MonitorQuestCompletion(Player* bot)
{
    if (!bot)
        return;

    std::vector<uint32> completedQuests = GetCompletedQuests(bot);
    for (uint32 questId : completedQuests)
    {
        HandleQuestCompletion(bot, questId);
    }
}

/**
 * @brief Handle quest completion event
 * @param bot Bot player
 * @param questId Quest ID
 */
void QuestTurnIn::HandleQuestCompletion(Player* bot, uint32 questId)
{
    if (!bot || !questId)
        return;

    TC_LOG_DEBUG("playerbot", "QuestTurnIn::HandleQuestCompletion - Quest %u completed for bot %s",
        questId, bot->GetName().c_str());

    // Get turn-in strategy
    TurnInStrategy strategy = GetTurnInStrategy(bot->GetGUID().GetCounter());
    switch (strategy)
    {
        case TurnInStrategy::IMMEDIATE_TURNIN:
            TurnInQuest(questId, bot);
            break;

        case TurnInStrategy::BATCH_TURNIN:
        {
            auto& queue = _botTurnInQueues[bot->GetGUID().GetCounter()];
            // Check if we have enough for batch
            if (queue.size() >= BATCH_TURNIN_THRESHOLD)
            {
                std::vector<uint32> batchQuests;
                for (const auto& data : queue)
                    batchQuests.push_back(data.questId);

                TurnInBatch batch(bot->GetGUID().GetCounter());
                batch.questIds = batchQuests;
                ProcessBatchTurnIn(bot, batch);
            }
            else
            {
                ScheduleQuestTurnIn(bot, questId);
            }
            break;
        }

        case TurnInStrategy::OPTIMAL_ROUTING:
            PlanOptimalTurnInRoute(bot);
            break;

        case TurnInStrategy::GROUP_COORDINATION:
            if (bot->GetGroup())
                CoordinateGroupTurnIns(bot->GetGroup());
            else
                TurnInQuest(questId, bot);
            break;

        default:
            TurnInQuest(questId, bot);
            break;
    }
}

/**
 * @brief Plan optimal turn-in route
 * @param bot Bot player
 */
void QuestTurnIn::PlanOptimalTurnInRoute(Player* bot)
{
    if (!bot)
        return;

    std::vector<uint32> completedQuests = GetCompletedQuests(bot);
    if (completedQuests.empty())
        return;

    // Create batch with optimized routing
    TurnInBatch batch = CreateTurnInBatch(bot, completedQuests);
    // Process the batch
    ProcessBatchTurnIn(bot, batch);
}

/**
 * @brief Create turn-in batch
 * @param bot Bot player
 * @param questIds Quest IDs to batch
 * @return Turn-in batch
 */
TurnInBatch QuestTurnIn::CreateTurnInBatch(Player* bot, const std::vector<uint32>& questIds)
{
    TurnInBatch batch(bot->GetGUID().GetCounter());
    batch.questIds = questIds;

    if (!bot || questIds.empty())
        return batch;

    // Calculate central location
    float sumX = 0, sumY = 0, sumZ = 0;
    uint32 count = 0;

    for (uint32 questId : questIds)
    {
        Position pos = GetQuestTurnInLocation(questId);
        if (pos.GetPositionX() != 0)
        {
            sumX += pos.GetPositionX();
            sumY += pos.GetPositionY();
            sumZ += pos.GetPositionZ();
            count++;
        }
    }

    if (count > 0)
    {
        batch.centralLocation.Relocate(sumX / count, sumY / count, sumZ / count);
        batch.totalTravelTime = CalculateTravelTime(bot, batch.centralLocation) * batch.questIds.size();
    }

    batch.isOptimized = true;
    return batch;
}

/**
 * @brief Find quest turn-in NPC
 * @param bot Bot player
 * @param questId Quest ID
 * @return True if NPC found
 */
bool QuestTurnIn::FindQuestTurnInNpc(Player* bot, uint32 questId)
{
    if (!bot || !questId)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
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

    // DEADLOCK FIX: Check for quest ender NPCs using snapshots (thread-safe!)
    Creature* questEnder = nullptr;
    float minDistance = MAX_TURNIN_DISTANCE;

    std::vector<DoubleBufferedSpatialGrid::CreatureSnapshot> nearbyCreatures =
        spatialGrid->QueryNearbyCreatures(bot->GetPosition(), MAX_TURNIN_DISTANCE);
    ObjectGuid questEnderGuid;
    for (auto const& snapshot : nearbyCreatures)
    {
        if (!snapshot.isVisible || !snapshot.hasQuestGiver)
            continue;

        // Check distance using snapshot position
        float distance = bot->GetExactDist(snapshot.position);
        if (distance < minDistance)
        {
            minDistance = distance;
            questEnderGuid = snapshot.guid;
        }
    }

    // Resolve GUID to pointer after loop
    if (!questEnderGuid.IsEmpty())
        questEnder = ObjectAccessor::GetCreature(*bot, questEnderGuid);
    if (questEnder)
    {
        _questToTurnInNpc[questId] = questEnder->GetGUID().GetCounter();
        _questGiverLocations[questEnder->GetGUID().GetCounter()] = questEnder->GetPosition();

        TC_LOG_DEBUG("playerbot", "QuestTurnIn::FindQuestTurnInNpc - Found quest ender %s for quest %u",
            questEnder->GetName().c_str(), questId);
        return true;
    }

    return false;
}

/**
 * @brief Get quest turn-in location
 * @param questId Quest ID
 * @return Position of turn-in location
 */
Position QuestTurnIn::GetQuestTurnInLocation(uint32 questId)
{
    auto it = _questToTurnInNpc.find(questId);
    if (it != _questToTurnInNpc.end())
    {
        auto locIt = _questGiverLocations.find(it->second);
        if (locIt != _questGiverLocations.end())
            return locIt->second;
    }

    return Position();
}

/**
 * @brief Navigate to quest giver
 * @param bot Bot player
 * @param questGiverGuid Quest giver GUID
 * @return True if navigation started
 */
bool QuestTurnIn::NavigateToQuestGiver(Player* bot, uint32 questGiverGuid)
{
    if (!bot || !questGiverGuid)
        return false;

    auto it = _questGiverLocations.find(questGiverGuid);
    if (it == _questGiverLocations.end())
        return false;

    // Use movement utility to navigate
    BotMovementUtil::MoveToPosition(bot, it->second);

    TC_LOG_DEBUG("playerbot", "QuestTurnIn::NavigateToQuestGiver - Bot %s navigating to quest giver %u",
        bot->GetName().c_str(), questGiverGuid);

    return true;
}

/**
 * @brief Check if bot is at quest giver
 * @param bot Bot player
 * @param questGiverGuid Quest giver GUID
 * @return True if bot is in interaction range
 */
bool QuestTurnIn::IsAtQuestGiver(Player* bot, uint32 questGiverGuid)
{
    if (!bot || !questGiverGuid)
        return false;

    Creature* questGiver = bot->GetMap()->GetCreature(ObjectGuid::Create<HighGuid::Creature>(bot->GetMapId(), 0, questGiverGuid));
    if (!questGiver)
        return false;

    return bot->GetDistance(questGiver) <= QUEST_GIVER_INTERACTION_RANGE;
}

/**
 * @brief Analyze quest rewards
 * @param turnInData Turn-in data
 * @param bot Bot player
 */
void QuestTurnIn::AnalyzeQuestRewards(QuestTurnInData& turnInData, Player* bot)
{
    if (!bot)
        return;

    Quest const* quest = sObjectMgr->GetQuestTemplate(turnInData.questId);
    if (!quest)
        return;

    turnInData.availableRewards.clear();

    // Analyze reward items
    for (uint8 i = 0; i < QUEST_REWARD_CHOICES_COUNT; ++i)
    {
        if (quest->RewardChoiceItemId[i])
        {
            QuestRewardItem reward(quest->RewardChoiceItemId[i], quest->RewardChoiceItemCount[i]);

            // Calculate item value
            reward.itemValue = CalculateItemValue(reward, bot);

            // Check if it's an upgrade
            EvaluateItemUpgrades({ reward }, bot);

            // Check if class appropriate
            ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(reward.itemId);
            if (itemTemplate)
            {
                reward.vendorValue = itemTemplate->GetSellPrice() * reward.itemCount;
                reward.isClassAppropriate = bot->CanUseItem(itemTemplate) == EQUIP_ERR_OK;
            }

            turnInData.availableRewards.push_back(reward);
        }
    }
}

/**
 * @brief Select optimal reward
 * @param rewards Available rewards
 * @param bot Bot player
 * @param strategy Reward selection strategy
 * @return Index of selected reward
 */
uint32 QuestTurnIn::SelectOptimalReward(const std::vector<QuestRewardItem>& rewards, Player* bot, RewardSelectionStrategy strategy)
{
    if (!bot || rewards.empty())
        return 0;

    switch (strategy)
    {
        case RewardSelectionStrategy::HIGHEST_VALUE:
            return SelectHighestValueReward(rewards, bot);

        case RewardSelectionStrategy::BEST_UPGRADE:
            return SelectBestUpgradeReward(rewards, bot);

        case RewardSelectionStrategy::VENDOR_VALUE:
            return SelectHighestVendorValueReward(rewards, bot);

        case RewardSelectionStrategy::STAT_PRIORITY:
            return SelectStatPriorityReward(rewards, bot);

        case RewardSelectionStrategy::CLASS_APPROPRIATE:
            return SelectClassAppropriateReward(rewards, bot);

        case RewardSelectionStrategy::RANDOM_SELECTION:
            return urand(0, static_cast<uint32>(rewards.size() - 1));

        default:
            return SelectBestUpgradeReward(rewards, bot);
    }
}

/**
 * @brief Evaluate item upgrades
 * @param rewards Available rewards
 * @param bot Bot player
 */
void QuestTurnIn::EvaluateItemUpgrades(const std::vector<QuestRewardItem>& rewards, Player* bot)
{
    if (!bot)
        return;
    for (auto& reward : const_cast<std::vector<QuestRewardItem>&>(rewards))
    {
        ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(reward.itemId);
        if (!itemTemplate)
            continue;

        // Check if item is equippable
        if (itemTemplate->GetClass() == ITEM_CLASS_WEAPON || itemTemplate->GetClass() == ITEM_CLASS_ARMOR)
        {
            // Get currently equipped item in same slot
            Item* currentItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, itemTemplate->GetInventoryType());
            if (currentItem)
            {
                ItemTemplate const* currentTemplate = currentItem->GetTemplate();
                if (currentTemplate)
                {
                    // Simple upgrade calculation based on item level
                    float upgradeValue = itemTemplate->GetBaseItemLevel() - currentTemplate->GetBaseItemLevel();

                    // Factor in quality difference
                    upgradeValue += (itemTemplate->GetQuality() - currentTemplate->GetQuality()) * 10;

                    reward.upgradeValue = upgradeValue;
                }
            }
            else
            {
                // No item in slot, significant upgrade
                reward.upgradeValue = itemTemplate->GetBaseItemLevel() * 1.5f;
            }
        }
    }
}

/**
 * @brief Calculate item value for bot
 * @param reward Reward item
 * @param bot Bot player
 * @return Calculated value
 */
float QuestTurnIn::CalculateItemValue(const QuestRewardItem& reward, Player* bot)
{
    if (!bot)
        return 0.0f;

    ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(reward.itemId);
    if (!itemTemplate)
        return 0.0f;

    float value = 0.0f;

    // Base value from item level and quality
    value += itemTemplate->GetBaseItemLevel() * (1.0f + itemTemplate->GetQuality() * 0.5f);

    // Add vendor value component
    value += itemTemplate->GetSellPrice() / 10000.0f; // Normalize to reasonable range

    // Factor in class usability
    if (bot->CanUseItem(itemTemplate) == EQUIP_ERR_OK)
        value *= 1.5f;

    // Factor in item count
    value *= reward.itemCount;

    return value;
}

/**
 * @brief Coordinate group turn-ins
 * @param group Group pointer
 */
void QuestTurnIn::CoordinateGroupTurnIns(Group* group)
{
    if (!group)
        return;

    TC_LOG_DEBUG("playerbot", "QuestTurnIn::CoordinateGroupTurnIns - Coordinating turn-ins for group %u",
        group->GetGUID().GetCounter());

    std::unordered_map<uint32, std::vector<Player*>> questCompletions;

    // Gather quest completion status from all members
    for (GroupReference const& itr : group->GetMembers())
    {
        Player* member = itr.GetSource();
        if (!member || !member->IsAlive())
            continue;

        std::vector<uint32> completedQuests = GetCompletedQuests(member);
        for (uint32 questId : completedQuests)
        {
            questCompletions[questId].push_back(member);
        }
    }

    // Process shared quest completions
    for (const auto& [questId, members] : questCompletions)
    {
        if (members.size() >= 2) // Multiple members have same quest
        {
            SynchronizeGroupRewardSelection(group, questId);

            // Turn in together
            for (Player* member : members)
            {
                ScheduleQuestTurnIn(member, questId);
            }
        }
    }
}

/**
 * @brief Synchronize group reward selection
 * @param group Group pointer
 * @param questId Quest ID
 */
void QuestTurnIn::SynchronizeGroupRewardSelection(Group* group, uint32 questId)
{
    if (!group || !questId)
        return;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return;

    // Note: Group reward coordination would require role detection system
    // This is a placeholder for future implementation
    TC_LOG_DEBUG("playerbot", "QuestTurnIn::SynchronizeGroupRewardSelection - Synchronizing rewards for quest %u", questId);
}

/**
 * @brief Handle quest giver dialog
 * @param bot Bot player
 * @param questGiverGuid Quest giver GUID
 * @param questId Quest ID
 */
void QuestTurnIn::HandleQuestGiverDialog(Player* bot, uint32 questGiverGuid, uint32 questId)
{
    if (!bot || !questGiverGuid || !questId)
        return;

    Creature* questGiver = bot->GetMap()->GetCreature(ObjectGuid::Create<HighGuid::Creature>(bot->GetMapId(), 0, questGiverGuid));
    if (!questGiver)
        return;

    // Open quest dialog
    bot->PrepareQuestMenu(questGiver->GetGUID());

    // Complete the quest
    bot->CompleteQuest(questId);

    // Handle reward selection
    HandleTurnInDialog(bot, questId);
}

/**
 * @brief Select quest reward
 * @param bot Bot player
 * @param questId Quest ID
 * @param rewardIndex Reward index
 */
void QuestTurnIn::SelectQuestReward(Player* bot, uint32 questId, uint32 rewardIndex)
{
    if (!bot || !questId)
        return;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return;

    // Reward the quest
    Object* questGiver = bot->GetSelectedUnit();
    if (!questGiver)
    {
        // Try to find GameObject through target
        ObjectGuid targetGuid = bot->GetTarget();
        if (targetGuid.IsGameObject())
            questGiver = bot->GetMap()->GetGameObject(targetGuid);
    }

    if (questGiver)
    {
        bot->RewardQuest(quest, LootItemType::Item, rewardIndex, questGiver, true);

        // Update metrics
        _globalMetrics.successfulTurnIns++;
        _globalMetrics.rewardsSelected++;
        _botMetrics[bot->GetGUID().GetCounter()].successfulTurnIns++;
        _botMetrics[bot->GetGUID().GetCounter()].rewardsSelected++;

        TC_LOG_DEBUG("playerbot", "QuestTurnIn::SelectQuestReward - Bot %s selected reward %u for quest %u",
            bot->GetName().c_str(), rewardIndex, questId);
    }
}

/**
 * @brief Handle turn-in dialog
 * @param bot Bot player
 * @param questId Quest ID
 */
void QuestTurnIn::HandleTurnInDialog(Player* bot, uint32 questId)
{
    if (!bot || !questId)
        return;

    auto it = _botTurnInQueues.find(bot->GetGUID().GetCounter());
    if (it == _botTurnInQueues.end())
        return;

    auto turnInIt = std::find_if(it->second.begin(), it->second.end(),
        [questId](const QuestTurnInData& data) { return data.questId == questId; });

    if (turnInIt == it->second.end())
        return;

    // Analyze rewards if not done
    if (turnInIt->availableRewards.empty())
        AnalyzeQuestRewards(*turnInIt, bot);

    // Select optimal reward
    RewardSelectionStrategy strategy = GetRewardSelectionStrategy(bot->GetGUID().GetCounter());
    uint32 rewardIndex = SelectOptimalReward(turnInIt->availableRewards, bot, strategy);

    // Complete the turn-in
    SelectQuestReward(bot, questId, rewardIndex);

    // Remove from queue
    it->second.erase(turnInIt);
}

/**
 * @brief Handle quest chain progression
 * @param bot Bot player
 * @param completedQuestId Completed quest ID
 */
void QuestTurnIn::HandleQuestChainProgression(Player* bot, uint32 completedQuestId)
{
    if (!bot || !completedQuestId)
        return;

    // Get next quest in chain
    uint32 nextQuestId = GetNextQuestInChain(completedQuestId);
    if (nextQuestId)
    {
        // Auto-accept follow-up quest
        AutoAcceptFollowUpQuests(bot, completedQuestId);

        TC_LOG_DEBUG("playerbot", "QuestTurnIn::HandleQuestChainProgression - Bot %s continuing quest chain from %u to %u",
            bot->GetName().c_str(), completedQuestId, nextQuestId);
    }
}

/**
 * @brief Get next quest in chain
 * @param completedQuestId Completed quest ID
 * @return Next quest ID or 0
 */
uint32 QuestTurnIn::GetNextQuestInChain(uint32 completedQuestId)
{
    Quest const* quest = sObjectMgr->GetQuestTemplate(completedQuestId);
    if (!quest)
        return 0;

    // Check for next quest in chain
    if (quest->GetNextQuestInChain())
        return quest->GetNextQuestInChain();

    return 0;
}

/**
 * @brief Auto-accept follow-up quests
 * @param bot Bot player
 * @param completedQuestId Completed quest ID
 */
void QuestTurnIn::AutoAcceptFollowUpQuests(Player* bot, uint32 completedQuestId)
{
    if (!bot || !completedQuestId)
        return;

    uint32 nextQuestId = GetNextQuestInChain(completedQuestId);
    if (!nextQuestId)
        return;

    // Auto-accept next quest in chain
    // Note: This would typically be handled by the quest giver dialog
    // For now, just log that we should accept the follow-up quest
    TC_LOG_DEBUG("playerbot", "QuestTurnIn::AutoAcceptFollowUpQuests - Bot %s should accept follow-up quest %u",
        bot->GetName().c_str(), nextQuestId);
}

/**
 * @brief Set turn-in strategy for bot
 * @param botGuid Bot GUID
 * @param strategy Turn-in strategy
 */
void QuestTurnIn::SetTurnInStrategy(uint32 botGuid, TurnInStrategy strategy)
{
    _botTurnInStrategies[botGuid] = strategy;
}

/**
 * @brief Get turn-in strategy for bot
 * @param botGuid Bot GUID
 * @return Turn-in strategy
 */
TurnInStrategy QuestTurnIn::GetTurnInStrategy(uint32 botGuid)
{
    auto it = _botTurnInStrategies.find(botGuid);
    if (it != _botTurnInStrategies.end())
        return it->second;

    return TurnInStrategy::IMMEDIATE_TURNIN;
}

/**
 * @brief Set reward selection strategy for bot
 * @param botGuid Bot GUID
 * @param strategy Reward selection strategy
 */
void QuestTurnIn::SetRewardSelectionStrategy(uint32 botGuid, RewardSelectionStrategy strategy)
{
    _botRewardStrategies[botGuid] = strategy;
}

/**
 * @brief Get reward selection strategy for bot
 * @param botGuid Bot GUID
 * @return Reward selection strategy
 */
RewardSelectionStrategy QuestTurnIn::GetRewardSelectionStrategy(uint32 botGuid)
{
    auto it = _botRewardStrategies.find(botGuid);
    if (it != _botRewardStrategies.end())
        return it->second;

    return RewardSelectionStrategy::BEST_UPGRADE;
}

/**
 * @brief Handle turn-in error
 * @param bot Bot player
 * @param questId Quest ID
 * @param error Error message
 */
void QuestTurnIn::HandleTurnInError(Player* bot, uint32 questId, const std::string& error)
{
    if (!bot)
        return;

    LogTurnInError(bot, questId, error);

    // Update metrics
    _globalMetrics.failedTurnIns++;
    _botMetrics[bot->GetGUID().GetCounter()].failedTurnIns++;

    // Attempt recovery
    RecoverFromTurnInFailure(bot, questId);
}

/**
 * @brief Recover from turn-in failure
 * @param bot Bot player
 * @param questId Quest ID
 */
void QuestTurnIn::RecoverFromTurnInFailure(Player* bot, uint32 questId)
{
    if (!bot || !questId)
        return;

    TC_LOG_DEBUG("playerbot", "QuestTurnIn::RecoverFromTurnInFailure - Attempting recovery for quest %u", questId);

    // Validate quest state
    ValidateTurnInState(bot, questId);

    // Retry after delay
    RetryFailedTurnIn(bot, questId);
}

/**
 * @brief Retry failed turn-in
 * @param bot Bot player
 * @param questId Quest ID
 */
void QuestTurnIn::RetryFailedTurnIn(Player* bot, uint32 questId)
{
    if (!bot || !questId)
        return;

    // Schedule retry
    ScheduleQuestTurnIn(bot, questId, TURNIN_RETRY_DELAY);

    TC_LOG_DEBUG("playerbot", "QuestTurnIn::RetryFailedTurnIn - Scheduled retry for quest %u in %u ms",
        questId, TURNIN_RETRY_DELAY);
}

/**
 * @brief Validate turn-in state
 * @param bot Bot player
 * @param questId Quest ID
 */
void QuestTurnIn::ValidateTurnInState(Player* bot, uint32 questId)
{
    if (!bot || !questId)
        return;

    // Check quest status
    QuestStatus status = bot->GetQuestStatus(questId);

    if (status == QUEST_STATUS_NONE)
    {
        TC_LOG_ERROR("playerbot", "QuestTurnIn::ValidateTurnInState - Bot doesn't have quest %u", questId);
        HandleInvalidQuestState(bot, questId);
    }
    else if (status != QUEST_STATUS_COMPLETE)
    {
        TC_LOG_DEBUG("playerbot", "QuestTurnIn::ValidateTurnInState - Quest %u not complete (status: %u)",
            questId, status);
    }
}

/**
 * @brief Update turn-in system
 * @param diff Time difference
 */
void QuestTurnIn::Update(uint32 /*diff*/)
{
    ProcessScheduledTurnIns();
    CleanupCompletedTurnIns();
}

/**
 * @brief Process scheduled turn-ins
 */
void QuestTurnIn::ProcessScheduledTurnIns()
{
    uint32 processed = 0;
    while (!_scheduledTurnIns.empty() && processed < MAX_SCHEDULED_TURNINS)
    {
        auto [botGuid, questId] = _scheduledTurnIns.front();
        _scheduledTurnIns.pop();

        // Find bot using low GUID
        Player* bot = ObjectAccessor::FindPlayerByLowGUID(botGuid);
        if (bot)
        {
            TurnInQuest(questId, bot);
        }

        processed++;
    }
}

/**
 * @brief Clean up completed turn-ins
 */
void QuestTurnIn::CleanupCompletedTurnIns()
{
    uint32 currentTime = GameTime::GetGameTimeMS();
    // Clean up old turn-in data
    for (auto& [botGuid, turnIns] : _botTurnInQueues)
    {
        turnIns.erase(
            std::remove_if(turnIns.begin(), turnIns.end(),
                [currentTime](const QuestTurnInData& data)
                {
                    return data.isCompleted && (currentTime - data.scheduledTurnInTime) > 300000; // 5 minutes
                }),
            turnIns.end()
        );
    }
}

/**
 * @brief Get bot turn-in metrics
 * @param botGuid Bot GUID
 * @return Turn-in metrics snapshot
 */
QuestTurnIn::TurnInMetricsSnapshot QuestTurnIn::GetBotTurnInMetrics(uint32 botGuid)
{
    auto it = _botMetrics.find(botGuid);
    if (it != _botMetrics.end())
    {
        auto snapshot = it->second.CreateSnapshot();
        QuestTurnIn::TurnInMetricsSnapshot result;
        result.questsTurnedIn = snapshot.questsTurnedIn;
        result.turnInAttempts = snapshot.turnInAttempts;
        result.successfulTurnIns = snapshot.successfulTurnIns;
        result.failedTurnIns = snapshot.failedTurnIns;
        result.averageTurnInTime = snapshot.averageTurnInTime;
        result.turnInSuccessRate = snapshot.turnInSuccessRate;
        result.totalTravelDistance = snapshot.totalTravelDistance;
        result.rewardsSelected = snapshot.rewardsSelected;
        result.rewardSelectionAccuracy = snapshot.rewardSelectionAccuracy;
        return result;
    }

    return QuestTurnIn::TurnInMetricsSnapshot();
}

/**
 * @brief Get global turn-in metrics
 * @return Global turn-in metrics snapshot
 */
QuestTurnIn::TurnInMetricsSnapshot QuestTurnIn::GetGlobalTurnInMetrics()
{
    auto snapshot = _globalMetrics.CreateSnapshot();
    QuestTurnIn::TurnInMetricsSnapshot result;
    result.questsTurnedIn = snapshot.questsTurnedIn;
    result.turnInAttempts = snapshot.turnInAttempts;
    result.successfulTurnIns = snapshot.successfulTurnIns;
    result.failedTurnIns = snapshot.failedTurnIns;
    result.averageTurnInTime = snapshot.averageTurnInTime;
    result.turnInSuccessRate = snapshot.turnInSuccessRate;
    result.totalTravelDistance = snapshot.totalTravelDistance;
    result.rewardsSelected = snapshot.rewardsSelected;
    result.rewardSelectionAccuracy = snapshot.rewardSelectionAccuracy;
    return result;
}

/**
 * @brief Initialize turn-in data for quest
 * @param bot Bot player
 * @param questId Quest ID
 */
void QuestTurnIn::InitializeTurnInData(Player* bot, uint32 questId)
{
    if (!bot || !questId)
        return;

    QuestTurnInData turnInData(questId, bot->GetGUID().GetCounter(), 0);
    turnInData.isCompleted = false;
    turnInData.rewardStrategy = GetRewardSelectionStrategy(bot->GetGUID().GetCounter());
    _botTurnInQueues[bot->GetGUID().GetCounter()].push_back(turnInData);
}

/**
 * @brief Load quest giver database
 */
void QuestTurnIn::LoadQuestGiverDatabase()
{
    // This would load quest giver locations from database
    // For now, it's populated dynamically as quests are discovered
}

/**
 * @brief Validate quest turn-in
 * @param bot Bot player
 * @param questId Quest ID
 * @return True if turn-in is valid
 */
bool QuestTurnIn::ValidateQuestTurnIn(Player* bot, uint32 questId)
{
    if (!bot || !questId)
        return false;

    // Check quest exists
    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    // Check quest status
    if (bot->GetQuestStatus(questId) != QUEST_STATUS_COMPLETE)
        return false;

    // Check if bot meets turn-in requirements
    if (!bot->CanRewardQuest(quest, false))
        return false;

    return true;
}

/**
 * @brief Execute turn-in workflow
 * @param bot Bot player
 * @param turnInData Turn-in data
 */
void QuestTurnIn::ExecuteTurnInWorkflow(Player* bot, const QuestTurnInData& turnInData)
{
    if (!bot)
        return;

    // Navigate to quest giver if needed
    if (turnInData.requiresTravel)
    {
        if (!IsAtQuestGiver(bot, turnInData.questGiverGuid))
        {
            NavigateToQuestGiver(bot, turnInData.questGiverGuid);
            return;
        }
    }

    // Handle quest dialog
    HandleQuestGiverDialog(bot, turnInData.questGiverGuid, turnInData.questId);

    // Process response
    ProcessQuestTurnInResponse(bot, turnInData.questId, true);

    // Handle chain progression
    HandleQuestChainProgression(bot, turnInData.questId);
}

/**
 * @brief Process quest turn-in response
 * @param bot Bot player
 * @param questId Quest ID
 * @param wasSuccessful Whether turn-in was successful
 */
void QuestTurnIn::ProcessQuestTurnInResponse(Player* bot, uint32 questId, bool wasSuccessful)
{
    if (!bot)
        return;

    if (wasSuccessful)
    {
        TC_LOG_DEBUG("playerbot", "QuestTurnIn::ProcessQuestTurnInResponse - Successfully turned in quest %u for bot %s",
            questId, bot->GetName().c_str());

        // Update metrics
        _globalMetrics.questsTurnedIn++;
        _globalMetrics.successfulTurnIns++;
        _botMetrics[bot->GetGUID().GetCounter()].questsTurnedIn++;
        _botMetrics[bot->GetGUID().GetCounter()].successfulTurnIns++;
    }
    else
    {
        HandleTurnInError(bot, questId, "Turn-in failed");
    }
}

/**
 * @brief Select highest value reward
 * @param rewards Available rewards
 * @param bot Bot player
 * @return Index of selected reward
 */
uint32 QuestTurnIn::SelectHighestValueReward(const std::vector<QuestRewardItem>& rewards, Player* bot)
{
    if (rewards.empty())
        return 0;

    uint32 bestIndex = 0;
    float bestValue = 0.0f;

    for (size_t i = 0; i < rewards.size(); ++i)
    {
        if (rewards[i].itemValue > bestValue)
        {
            bestValue = rewards[i].itemValue;
            bestIndex = i;
        }
    }

    return bestIndex;
}

/**
 * @brief Select best upgrade reward
 * @param rewards Available rewards
 * @param bot Bot player
 * @return Index of selected reward
 */
uint32 QuestTurnIn::SelectBestUpgradeReward(const std::vector<QuestRewardItem>& rewards, Player* bot)
{
    if (rewards.empty())
        return 0;

    uint32 bestIndex = 0;
    float bestUpgrade = -1000.0f;

    for (size_t i = 0; i < rewards.size(); ++i)
    {
        if (rewards[i].upgradeValue > bestUpgrade && rewards[i].isClassAppropriate)
        {
            bestUpgrade = rewards[i].upgradeValue;
            bestIndex = i;
        }
    }

    // If no upgrades found, fall back to highest value
    if (bestUpgrade <= 0)
        return SelectHighestValueReward(rewards, bot);

    return bestIndex;
}

/**
 * @brief Select highest vendor value reward
 * @param rewards Available rewards
 * @param bot Bot player
 * @return Index of selected reward
 */
uint32 QuestTurnIn::SelectHighestVendorValueReward(const std::vector<QuestRewardItem>& rewards, Player* bot)
{
    if (rewards.empty())
        return 0;

    uint32 bestIndex = 0;
    float bestValue = 0.0f;

    for (size_t i = 0; i < rewards.size(); ++i)
    {
        if (rewards[i].vendorValue > bestValue)
        {
            bestValue = rewards[i].vendorValue;
            bestIndex = i;
        }
    }

    return bestIndex;
}

/**
 * @brief Select stat priority reward
 * @param rewards Available rewards
 * @param bot Bot player
 * @return Index of selected reward
 */
uint32 QuestTurnIn::SelectStatPriorityReward(const std::vector<QuestRewardItem>& rewards, Player* bot)
{
    if (!bot || rewards.empty())
        return 0;

    // Get stat priorities based on class/spec
    // This would need proper stat weight calculation

    // For now, prefer class appropriate items
    return SelectClassAppropriateReward(rewards, bot);
}

/**
 * @brief Select class appropriate reward
 * @param rewards Available rewards
 * @param bot Bot player
 * @return Index of selected reward
 */
uint32 QuestTurnIn::SelectClassAppropriateReward(const std::vector<QuestRewardItem>& rewards, Player* bot)
{
    if (rewards.empty())
        return 0;

    // First, try to find class appropriate upgrade
    for (size_t i = 0; i < rewards.size(); ++i)
    {
        if (rewards[i].isClassAppropriate && rewards[i].upgradeValue > MIN_UPGRADE_VALUE_THRESHOLD)
            return i;
    }

    // Then, any class appropriate item
    for (size_t i = 0; i < rewards.size(); ++i)
    {
        if (rewards[i].isClassAppropriate)
            return i;
    }

    // Fall back to highest value
    return SelectHighestValueReward(rewards, bot);
}

/**
 * @brief Calculate travel time to destination
 * @param bot Bot player
 * @param destination Destination position
 * @return Estimated travel time in milliseconds
 */
float QuestTurnIn::CalculateTravelTime(Player* bot, const Position& destination)
{
    if (!bot)
        return 0.0f;

    float distance = bot->GetDistance(destination);
    float speed = bot->GetSpeed(MOVE_RUN);

    // Basic travel time calculation
    float travelTime = (distance / speed) * 1000.0f; // Convert to milliseconds

    // Add buffer for obstacles and pathfinding
    travelTime *= 1.5f;

    return travelTime;
}

/**
 * @brief Optimize travel route for quest givers
 * @param bot Bot player
 * @param questGiverGuids Quest giver GUIDs (modified to be in optimal order)
 */
void QuestTurnIn::OptimizeTravelRoute(Player* bot, std::vector<uint32>& questGiverGuids)
{
    if (!bot || questGiverGuids.size() <= 1)
        return;

    // Simple nearest-neighbor optimization
    std::vector<uint32> optimized;
    std::vector<bool> visited(questGiverGuids.size(), false);

    // Start from current position
    Position currentPos = bot->GetPosition();

    while (optimized.size() < questGiverGuids.size())
    {
        float minDistance = std::numeric_limits<float>::max();
        size_t nearestIndex = 0;

        for (size_t i = 0; i < questGiverGuids.size(); ++i)
        {
            if (visited[i])
                continue;

            auto it = _questGiverLocations.find(questGiverGuids[i]);
            if (it == _questGiverLocations.end())
                continue;

            float distance = currentPos.GetExactDist(&it->second);
            if (distance < minDistance)
            {
                minDistance = distance;
                nearestIndex = i;
            }
        }

        visited[nearestIndex] = true;
        optimized.push_back(questGiverGuids[nearestIndex]);

        auto it = _questGiverLocations.find(questGiverGuids[nearestIndex]);
        if (it != _questGiverLocations.end())
            currentPos = it->second;
    }

    questGiverGuids = optimized;
}

/**
 * @brief Log turn-in error
 * @param bot Bot player
 * @param questId Quest ID
 * @param error Error message
 */
void QuestTurnIn::LogTurnInError(Player* bot, uint32 questId, const std::string& error)
{
    TC_LOG_ERROR("playerbot", "QuestTurnIn::LogTurnInError - Bot %s failed to turn in quest %u: %s",
        bot ? bot->GetName().c_str() : "unknown", questId, error.c_str());
}

/**
 * @brief Handle quest giver not found error
 * @param bot Bot player
 * @param questId Quest ID
 */
void QuestTurnIn::HandleQuestGiverNotFound(Player* bot, uint32 questId)
{
    if (!bot)
        return;

    TC_LOG_DEBUG("playerbot", "QuestTurnIn::HandleQuestGiverNotFound - Cannot find quest giver for quest %u", questId);

    // Try to find quest giver again with wider search
    FindQuestTurnInNpc(bot, questId);
}

/**
 * @brief Handle invalid quest state error
 * @param bot Bot player
 * @param questId Quest ID
 */
void QuestTurnIn::HandleInvalidQuestState(Player* bot, uint32 questId)
{
    if (!bot)
        return;

    TC_LOG_ERROR("playerbot", "QuestTurnIn::HandleInvalidQuestState - Invalid state for quest %u", questId);

    // Remove quest from turn-in queue
    auto it = _botTurnInQueues.find(bot->GetGUID().GetCounter());
    if (it != _botTurnInQueues.end())
    {
        it->second.erase(
            std::remove_if(it->second.begin(), it->second.end(),
                [questId](const QuestTurnInData& data) { return data.questId == questId; }),
            it->second.end()
        );
    }
}

/**
 * @brief Optimize turn-in sequence for efficient processing
 * @param bot Bot player
 * @param turnIns Vector of turn-in data to optimize
 */
void QuestTurnIn::OptimizeTurnInSequence(Player* bot, std::vector<QuestTurnInData>& turnIns)
{
    if (!bot || turnIns.empty())
        return;

    // Sort by priority (higher first), then by estimated travel time (lower first)
    std::sort(turnIns.begin(), turnIns.end(),
        [](const QuestTurnInData& a, const QuestTurnInData& b)
        {
            if (a.turnInPriority != b.turnInPriority)
                return a.turnInPriority > b.turnInPriority;
            return a.estimatedTravelTime < b.estimatedTravelTime;
        });

    // Group by quest giver location to minimize travel
    std::unordered_map<uint32, std::vector<size_t>> questGiverGroups;
    for (size_t i = 0; i < turnIns.size(); ++i)
    {
        questGiverGroups[turnIns[i].questGiverGuid].push_back(i);
    }

    // Reorder to process all quests from same NPC together
    std::vector<QuestTurnInData> optimized;
    std::vector<bool> processed(turnIns.size(), false);
    Position currentPos = bot->GetPosition();

    while (optimized.size() < turnIns.size())
    {
        float minDistance = std::numeric_limits<float>::max();
        uint32 nearestGiver = 0;

        // Find nearest unprocessed quest giver
        for (const auto& [giverGuid, indices] : questGiverGroups)
        {
            if (processed[indices[0]])
                continue;

            auto locIt = _questGiverLocations.find(giverGuid);
            if (locIt != _questGiverLocations.end())
            {
                float distance = currentPos.GetExactDist(&locIt->second);
                if (distance < minDistance)
                {
                    minDistance = distance;
                    nearestGiver = giverGuid;
                }
            }
        }

        // Add all quests from nearest giver
        if (nearestGiver != 0)
        {
            for (size_t idx : questGiverGroups[nearestGiver])
            {
                if (!processed[idx])
                {
                    optimized.push_back(turnIns[idx]);
                    processed[idx] = true;
                }
            }

            auto locIt = _questGiverLocations.find(nearestGiver);
            if (locIt != _questGiverLocations.end())
                currentPos = locIt->second;
        }
        else
        {
            break;
        }
    }

    // Add any remaining unprocessed quests
    for (size_t i = 0; i < turnIns.size(); ++i)
    {
        if (!processed[i])
            optimized.push_back(turnIns[i]);
    }

    turnIns = optimized;

    TC_LOG_DEBUG("playerbot", "QuestTurnIn::OptimizeTurnInSequence - Optimized %zu turn-ins for bot %s",
        turnIns.size(), bot->GetName().c_str());
}

/**
 * @brief Minimize travel distance for quest turn-ins
 * @param bot Bot player
 */
void QuestTurnIn::MinimizeTurnInTravel(Player* bot)
{
    if (!bot)
        return;

    uint32 botGuid = bot->GetGUID().GetCounter();
    auto it = _botTurnInQueues.find(botGuid);
    if (it == _botTurnInQueues.end() || it->second.empty())
        return;

    // Optimize the turn-in sequence
    OptimizeTurnInSequence(bot, it->second);

    // Calculate total estimated travel distance
    float totalDistance = 0.0f;
    Position currentPos = bot->GetPosition();

    for (const auto& turnInData : it->second)
    {
        auto locIt = _questGiverLocations.find(turnInData.questGiverGuid);
        if (locIt != _questGiverLocations.end())
        {
            float distance = currentPos.GetExactDist(&locIt->second);
            totalDistance += distance;
            currentPos = locIt->second;
        }
    }

    _botMetrics[botGuid].totalTravelDistance += static_cast<uint32>(totalDistance);

    TC_LOG_DEBUG("playerbot", "QuestTurnIn::MinimizeTurnInTravel - Bot %s total travel distance: %.2f yards",
        bot->GetName().c_str(), totalDistance);
}

/**
 * @brief Handle conflicts when multiple group members want to turn in same quest
 * @param group Group pointer
 * @param questId Quest ID
 */
void QuestTurnIn::HandleGroupTurnInConflicts(Group* group, uint32 questId)
{
    if (!group || !questId)
        return;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return;

    std::vector<Player*> membersWithQuest;

    // Gather all group members who have this quest completed
    for (GroupReference const& itr : group->GetMembers())
    {
        Player* member = itr.GetSource();
        if (!member || !member->IsAlive())
            continue;

        if (IsQuestReadyForTurnIn(questId, member))
            membersWithQuest.push_back(member);
    }

    if (membersWithQuest.empty())
        return;

    // Strategy: Turn in sequentially to avoid NPC dialog conflicts
    // Prioritize by role: tank > healer > dps, then by proximity to quest giver
    Position questGiverPos = GetQuestTurnInLocation(questId);

    std::sort(membersWithQuest.begin(), membersWithQuest.end(),
        [&questGiverPos](Player* a, Player* b)
        {
            float distA = a->GetDistance(questGiverPos);
            float distB = b->GetDistance(questGiverPos);
            return distA < distB;
        });

    // Schedule turn-ins with delays to prevent conflicts
    uint32 delay = 0;
    for (Player* member : membersWithQuest)
    {
        ScheduleQuestTurnIn(member, questId, delay);
        delay += 2000; // 2 second delay between group members
    }

    TC_LOG_DEBUG("playerbot", "QuestTurnIn::HandleGroupTurnInConflicts - Scheduled %zu group turn-ins for quest %u",
        membersWithQuest.size(), questId);
}

/**
 * @brief Share turn-in progress with group members
 * @param group Group pointer
 */
void QuestTurnIn::ShareTurnInProgress(Group* group)
{
    if (!group)
        return;

    std::unordered_map<uint32, std::vector<Player*>> questProgress;

    // Gather completion status from all members
    for (GroupReference const& itr : group->GetMembers())
    {
        Player* member = itr.GetSource();
        if (!member || !member->IsAlive())
            continue;

        std::vector<uint32> completedQuests = GetCompletedQuests(member);
        for (uint32 questId : completedQuests)
        {
            questProgress[questId].push_back(member);
        }
    }

    // Log shared progress
    for (const auto& [questId, members] : questProgress)
    {
        if (members.size() > 1)
        {
            TC_LOG_DEBUG("playerbot", "QuestTurnIn::ShareTurnInProgress - %zu group members ready to turn in quest %u",
                members.size(), questId);

            // Coordinate group turn-in for shared quests
            SynchronizeGroupRewardSelection(group, questId);
        }
    }
}

/**
 * @brief Confirm quest turn-in before executing
 * @param bot Bot player
 * @param questId Quest ID
 */
void QuestTurnIn::ConfirmQuestTurnIn(Player* bot, uint32 questId)
{
    if (!bot || !questId)
        return;

    // Validate quest is still ready for turn-in
    if (!IsQuestReadyForTurnIn(questId, bot))
    {
        TC_LOG_WARN("playerbot", "QuestTurnIn::ConfirmQuestTurnIn - Quest %u no longer ready for turn-in by bot %s",
            questId, bot->GetName().c_str());
        HandleTurnInError(bot, questId, "Quest validation failed during confirmation");
        return;
    }

    // Verify quest giver is accessible
    if (!FindQuestTurnInNpc(bot, questId))
    {
        TC_LOG_WARN("playerbot", "QuestTurnIn::ConfirmQuestTurnIn - Quest giver not found for quest %u",
            questId);
        HandleTurnInError(bot, questId, "Quest giver not accessible");
        return;
    }

    // Confirm inventory space for rewards
    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (quest)
    {
        for (uint8 i = 0; i < QUEST_REWARD_ITEM_COUNT; ++i)
        {
            if (quest->RewardItemId[i])
            {
                ItemPosCountVec dest;
                InventoryResult result = bot->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest,
                    quest->RewardItemId[i], quest->RewardItemCount[i]);
                if (result != EQUIP_ERR_OK)
                {
                    TC_LOG_WARN("playerbot", "QuestTurnIn::ConfirmQuestTurnIn - Insufficient inventory space for quest %u",
                        questId);
                    HandleTurnInError(bot, questId, "Insufficient inventory space for rewards");
                    return;
                }
            }
        }
    }

    // All confirmations passed, proceed with turn-in
    ProcessQuestTurnIn(bot, questId);

    TC_LOG_DEBUG("playerbot", "QuestTurnIn::ConfirmQuestTurnIn - Quest %u confirmed for turn-in by bot %s",
        questId, bot->GetName().c_str());
}

/**
 * @brief Execute immediate turn-in strategy
 * @param bot Bot player
 */
void QuestTurnIn::ExecuteImmediateTurnInStrategy(Player* bot)
{
    if (!bot)
        return;

    std::vector<uint32> completedQuests = GetCompletedQuests(bot);

    TC_LOG_DEBUG("playerbot", "QuestTurnIn::ExecuteImmediateTurnInStrategy - Bot %s has %zu completed quests",
        bot->GetName().c_str(), completedQuests.size());

    for (uint32 questId : completedQuests)
    {
        TurnInQuest(questId, bot);
    }

    _globalMetrics.turnInAttempts += completedQuests.size();
    _botMetrics[bot->GetGUID().GetCounter()].turnInAttempts += completedQuests.size();
}

/**
 * @brief Execute batch turn-in strategy
 * @param bot Bot player
 */
void QuestTurnIn::ExecuteBatchTurnInStrategy(Player* bot)
{
    if (!bot)
        return;

    std::vector<uint32> completedQuests = GetCompletedQuests(bot);

    if (completedQuests.empty())
        return;

    uint32 threshold = BATCH_TURNIN_THRESHOLD;

    // Only process if we have enough quests for a batch
    if (completedQuests.size() >= threshold)
    {
        TurnInBatch batch = CreateTurnInBatch(bot, completedQuests);
        ProcessBatchTurnIn(bot, batch);

        TC_LOG_DEBUG("playerbot", "QuestTurnIn::ExecuteBatchTurnInStrategy - Bot %s processing batch of %zu quests",
            bot->GetName().c_str(), completedQuests.size());
    }
    else
    {
        // Not enough for batch, add to queue
        for (uint32 questId : completedQuests)
        {
            InitializeTurnInData(bot, questId);
        }

        TC_LOG_DEBUG("playerbot", "QuestTurnIn::ExecuteBatchTurnInStrategy - Bot %s queuing %zu quests (need %u for batch)",
            bot->GetName().c_str(), completedQuests.size(), threshold);
    }
}

/**
 * @brief Execute optimal routing strategy
 * @param bot Bot player
 */
void QuestTurnIn::ExecuteOptimalRoutingStrategy(Player* bot)
{
    if (!bot)
        return;

    std::vector<uint32> completedQuests = GetCompletedQuests(bot);

    if (completedQuests.empty())
        return;

    TC_LOG_DEBUG("playerbot", "QuestTurnIn::ExecuteOptimalRoutingStrategy - Bot %s planning optimal route for %zu quests",
        bot->GetName().c_str(), completedQuests.size());

    // Create optimized batch
    TurnInBatch batch = CreateTurnInBatch(bot, completedQuests);

    // Optimize travel route
    std::vector<uint32> questGiverGuids;
    for (uint32 questId : completedQuests)
    {
        auto it = _questToTurnInNpc.find(questId);
        if (it != _questToTurnInNpc.end())
            questGiverGuids.push_back(it->second);
    }

    if (!questGiverGuids.empty())
    {
        OptimizeTravelRoute(bot, questGiverGuids);

        // Reorder quests based on optimized route
        std::vector<uint32> orderedQuests;
        for (uint32 giverGuid : questGiverGuids)
        {
            for (uint32 questId : completedQuests)
            {
                if (_questToTurnInNpc[questId] == giverGuid)
                    orderedQuests.push_back(questId);
            }
        }

        batch.questIds = orderedQuests;
    }

    batch.isOptimized = true;
    ProcessBatchTurnIn(bot, batch);
}

/**
 * @brief Execute group coordination strategy
 * @param bot Bot player
 */
void QuestTurnIn::ExecuteGroupCoordinationStrategy(Player* bot)
{
    if (!bot)
        return;

    Group* group = bot->GetGroup();
    if (!group)
    {
        // No group, fall back to immediate turn-in
        ExecuteImmediateTurnInStrategy(bot);
        return;
    }

    TC_LOG_DEBUG("playerbot", "QuestTurnIn::ExecuteGroupCoordinationStrategy - Bot %s coordinating with group",
        bot->GetName().c_str());

    // Share progress with group
    ShareTurnInProgress(group);

    // Coordinate turn-ins
    CoordinateGroupTurnIns(group);
}

/**
 * @brief Execute reward optimization strategy
 * @param bot Bot player
 */
void QuestTurnIn::ExecuteRewardOptimizationStrategy(Player* bot)
{
    if (!bot)
        return;

    std::vector<uint32> completedQuests = GetCompletedQuests(bot);

    if (completedQuests.empty())
        return;

    TC_LOG_DEBUG("playerbot", "QuestTurnIn::ExecuteRewardOptimizationStrategy - Bot %s analyzing rewards for %zu quests",
        bot->GetName().c_str(), completedQuests.size());

    uint32 botGuid = bot->GetGUID().GetCounter();
    RewardSelectionStrategy strategy = GetRewardSelectionStrategy(botGuid);

    // Analyze all quest rewards before turn-in
    for (uint32 questId : completedQuests)
    {
        InitializeTurnInData(bot, questId);

        auto it = _botTurnInQueues.find(botGuid);
        if (it != _botTurnInQueues.end())
        {
            auto turnInIt = std::find_if(it->second.begin(), it->second.end(),
                [questId](const QuestTurnInData& data) { return data.questId == questId; });

            if (turnInIt != it->second.end())
            {
                AnalyzeQuestRewards(*turnInIt, bot);
                turnInIt->selectedRewardIndex = SelectOptimalReward(turnInIt->availableRewards, bot, strategy);
            }
        }
    }

    // Turn in quests with pre-selected optimal rewards
    for (uint32 questId : completedQuests)
    {
        TurnInQuest(questId, bot);
    }
}

/**
 * @brief Execute chain continuation strategy
 * @param bot Bot player
 */
void QuestTurnIn::ExecuteChainContinuationStrategy(Player* bot)
{
    if (!bot)
        return;

    std::vector<uint32> completedQuests = GetCompletedQuests(bot);

    if (completedQuests.empty())
        return;

    TC_LOG_DEBUG("playerbot", "QuestTurnIn::ExecuteChainContinuationStrategy - Bot %s processing %zu completed quests",
        bot->GetName().c_str(), completedQuests.size());

    // Prioritize chain quests
    PrioritizeChainQuests(bot);

    // Sort quests by priority (chain quests first)
    std::vector<std::pair<uint32, uint32>> questPriorities;
    for (uint32 questId : completedQuests)
    {
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        uint32 priority = 100;

        if (quest && quest->GetNextQuestInChain())
        {
            priority += CHAIN_QUEST_PRIORITY_BONUS;
        }

        questPriorities.push_back({questId, priority});
    }

    // Sort by priority (highest first)
    std::sort(questPriorities.begin(), questPriorities.end(),
        [](const std::pair<uint32, uint32>& a, const std::pair<uint32, uint32>& b)
        {
            return a.second > b.second;
        });

    // Turn in quests in priority order
    for (const auto& [questId, priority] : questPriorities)
    {
        TurnInQuest(questId, bot);

        // Handle chain progression immediately after turn-in
        HandleQuestChainProgression(bot, questId);
    }
}

/**
 * @brief Prioritize chain quests for turn-in
 * @param bot Bot player
 */
void QuestTurnIn::PrioritizeChainQuests(Player* bot)
{
    if (!bot)
        return;

    uint32 botGuid = bot->GetGUID().GetCounter();
    auto it = _botTurnInQueues.find(botGuid);
    if (it == _botTurnInQueues.end() || it->second.empty())
        return;

    // Update priority for chain quests
    for (auto& turnInData : it->second)
    {
        Quest const* quest = sObjectMgr->GetQuestTemplate(turnInData.questId);
        if (quest && quest->GetNextQuestInChain())
        {
            turnInData.turnInPriority += CHAIN_QUEST_PRIORITY_BONUS;
            turnInData.turnInReason = "Chain quest progression";

            TC_LOG_DEBUG("playerbot", "QuestTurnIn::PrioritizeChainQuests - Increased priority for chain quest %u to %u",
                turnInData.questId, turnInData.turnInPriority);
        }
    }

    // Re-sort queue by priority
    std::sort(it->second.begin(), it->second.end(),
        [](const QuestTurnInData& a, const QuestTurnInData& b)
        {
            return a.turnInPriority > b.turnInPriority;
        });
}

/**
 * @brief Set batch turn-in threshold for bot
 * @param botGuid Bot GUID
 * @param threshold Minimum number of quests for batch turn-in
 */
void QuestTurnIn::SetBatchTurnInThreshold(uint32 botGuid, uint32 threshold)
{
    if (threshold == 0)
        threshold = 1;

    // Store in batch data
    auto& batch = _scheduledBatches[botGuid];
    batch.batchPriority = threshold;

    TC_LOG_DEBUG("playerbot", "QuestTurnIn::SetBatchTurnInThreshold - Set batch threshold to %u for bot %u",
        threshold, botGuid);
}

/**
 * @brief Update bot turn-in processing
 * @param bot Bot player
 * @param diff Time difference in milliseconds
 */
void QuestTurnIn::UpdateBotTurnIns(Player* bot, uint32 diff)
{
    if (!bot)
        return;

    uint32 botGuid = bot->GetGUID().GetCounter();

    // Update metrics last update time
    _botMetrics[botGuid].lastUpdate = std::chrono::steady_clock::now();

    // Monitor quest completion
    MonitorQuestCompletion(bot);

    // Process queued turn-ins based on strategy
    TurnInStrategy strategy = GetTurnInStrategy(botGuid);

    auto it = _botTurnInQueues.find(botGuid);
    if (it != _botTurnInQueues.end() && !it->second.empty())
    {
        switch (strategy)
        {
            case TurnInStrategy::IMMEDIATE_TURNIN:
                ExecuteImmediateTurnInStrategy(bot);
                break;

            case TurnInStrategy::BATCH_TURNIN:
                ExecuteBatchTurnInStrategy(bot);
                break;

            case TurnInStrategy::OPTIMAL_ROUTING:
                ExecuteOptimalRoutingStrategy(bot);
                break;

            case TurnInStrategy::GROUP_COORDINATION:
                ExecuteGroupCoordinationStrategy(bot);
                break;

            case TurnInStrategy::REWARD_OPTIMIZATION:
                ExecuteRewardOptimizationStrategy(bot);
                break;

            case TurnInStrategy::CHAIN_CONTINUATION:
                ExecuteChainContinuationStrategy(bot);
                break;

            default:
                ExecuteImmediateTurnInStrategy(bot);
                break;
        }
    }

    // Update success rate metrics
    uint32 attempts = _botMetrics[botGuid].turnInAttempts.load();
    uint32 successful = _botMetrics[botGuid].successfulTurnIns.load();
    if (attempts > 0)
    {
        float successRate = static_cast<float>(successful) / attempts;
        _botMetrics[botGuid].turnInSuccessRate.store(successRate);
    }
}

} // namespace Playerbot