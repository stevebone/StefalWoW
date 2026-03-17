/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "DynamicQuestSystem.h"
#include "Player.h"
#include "QuestDef.h"
#include "Group.h"
#include "ObjectMgr.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Util.h"
#include "World.h"
#include "WorldSession.h"
#include "Creature.h"
#include "SharedDefines.h"
#include "GameObject.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "Session/BotSession.h"
#include "../AI/BotAI.h"  // P1 FIX: Required for unique_ptr<BotAI> in BotSession.h
#include "Spatial/SpatialGridQueryHelpers.h"  // PHASE 2G: For snapshot-based player validation
#include <algorithm>
#include <random>
#include <cmath>
#include <unordered_set>
#include "GameTime.h"

namespace Playerbot
{

// ============================================================================
// STATIC MEMBER DEFINITIONS - Shared across all DynamicQuestSystem instances
// ============================================================================
std::unordered_map<uint32, QuestMetadata> DynamicQuestSystem::_questMetadata;
std::unordered_map<uint32, std::vector<uint32>> DynamicQuestSystem::_questChains;
std::unordered_map<uint32, std::vector<uint32>> DynamicQuestSystem::_questPrerequisites;
std::unordered_map<uint32, std::vector<uint32>> DynamicQuestSystem::_questFollowups;
std::unordered_map<uint32, std::vector<uint32>> DynamicQuestSystem::_zoneQuests;
std::unordered_map<uint32, std::vector<Position>> DynamicQuestSystem::_questHotspots;
std::mutex DynamicQuestSystem::_staticDataMutex;
bool DynamicQuestSystem::_staticDataInitialized = false;

DynamicQuestSystem::DynamicQuestSystem(Player* bot)
    : _bot(bot)
{
    if (!_bot)
        TC_LOG_ERROR("playerbot.quest", "DynamicQuestSystem: null bot!");

    // Only load static quest data once (first bot to initialize)
    {
        std::lock_guard<std::mutex> lock(_staticDataMutex);
        if (!_staticDataInitialized)
        {
            TC_LOG_INFO("playerbot.quest", "DynamicQuestSystem: Initializing shared quest data (first instance)...");
            LoadQuestMetadata();
            AnalyzeQuestDependencies();
            BuildQuestChains();
            OptimizeQuestRoutes();
            _staticDataInitialized = true;
            TC_LOG_INFO("playerbot.quest", "DynamicQuestSystem: Shared quest data initialized");
        }
    }
}

DynamicQuestSystem::~DynamicQuestSystem() {}

std::vector<uint32> DynamicQuestSystem::DiscoverAvailableQuests(Player* bot)
{
    std::vector<uint32> availableQuests;

    if (!bot)
        return availableQuests;

    uint32 botLevel = bot->GetLevel();
    uint32 botGuid = bot->GetGUID().GetCounter();
    // Scan through all quest templates
    for (auto const& questPair : sObjectMgr->GetQuestTemplates())
    {
        Quest const* quest = questPair.second.get();
        if (!quest)
            continue;

        uint32 questId = quest->GetQuestId();

        // Check if quest is available to this bot
        if (bot->CanTakeQuest(quest, false) &&
            (quest->GetMaxLevel() == 0 || quest->GetMaxLevel() >= botLevel))
        {
            availableQuests.push_back(questId);
        }
    }

    return availableQuests;
}

std::vector<uint32> DynamicQuestSystem::GetRecommendedQuests(Player* bot, QuestSelectionStrategy strategy)
{
    std::vector<uint32> recommendedQuests;

    if (!bot)
        return recommendedQuests;

    std::vector<uint32> availableQuests = DiscoverAvailableQuests(bot);

    // Apply strategy-specific filtering and sorting
    switch (strategy)
    {
        case QuestSelectionStrategy::SOLO_FOCUSED:
            ExecuteSoloStrategy(bot);
            break;
        case QuestSelectionStrategy::GROUP_PREFERRED:
            ExecuteGroupStrategy(bot);
            break;
        case QuestSelectionStrategy::ZONE_OPTIMIZATION:
            ExecuteZoneStrategy(bot);
            break;
        case QuestSelectionStrategy::LEVEL_PROGRESSION:
            ExecuteLevelStrategy(bot);
            break;
        case QuestSelectionStrategy::GEAR_PROGRESSION:
            ExecuteGearStrategy(bot);
            break;
        case QuestSelectionStrategy::STORY_PROGRESSION:
            ExecuteStoryStrategy(bot);
            break;
        case QuestSelectionStrategy::REPUTATION_FOCUSED:
            ExecuteReputationStrategy(bot);
            break;
        default:
            ExecuteLevelStrategy(bot);
            break;
    }

    // Filter by strategy and sort by priority
    for (uint32 questId : availableQuests)
    {
        QuestPriority priority = CalculateQuestPriority(questId, bot);
        if (priority >= QuestPriority::NORMAL)
        {
            recommendedQuests.push_back(questId);
        }
    }

    // Sort by priority
    recommendedQuests = SortQuestsByPriority(recommendedQuests, bot);

    // Limit to reasonable number
    if (recommendedQuests.size() > 15)
        recommendedQuests.resize(15);

    return recommendedQuests;
}

bool DynamicQuestSystem::AssignQuestToBot(uint32 questId, Player* bot)
{
    if (!bot)
        return false;

    const Quest* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    // Check if bot can take the quest
    if (!bot->CanTakeQuest(quest, false))
        return false;

    uint32 botGuid = bot->GetGUID().GetCounter();
    // Create quest progress tracking
    QuestProgress progress(questId, botGuid);

    // Initialize quest metadata
    auto metadataIt = _questMetadata.find(questId);
    if (metadataIt == _questMetadata.end())
    {
        _questMetadata[questId] = QuestMetadata(questId);
        PopulateQuestMetadata(questId);
    }

    // Add to bot's quest progress
    {
        _botQuestProgress[botGuid].push_back(progress);
    }

    // Update metrics
    auto& metrics = _botMetrics[botGuid];
    metrics.questsStarted++;

    TC_LOG_DEBUG("playerbot.quest", "Assigned quest {} to bot {}", questId, bot->GetName());
    return true;
}

void DynamicQuestSystem::AutoAssignQuests(Player* bot, uint32 maxQuests)
{
    if (!bot)
        return;

    uint32 botGuid = bot->GetGUID().GetCounter();
    QuestSelectionStrategy strategy = GetQuestStrategy(botGuid);

    // Get current quest count
    uint32 currentQuests = 0;
    {
        auto it = _botQuestProgress.find(botGuid);
        if (it != _botQuestProgress.end())
            currentQuests = static_cast<uint32>(it->second.size());
    }

    if (currentQuests >= maxQuests)
        return;

    // Get recommended quests
    std::vector<uint32> recommended = GetRecommendedQuests(bot, strategy);

    // Assign quests up to the limit
    uint32 questsToAssign = std::min(maxQuests - currentQuests, static_cast<uint32>(recommended.size()));

    for (uint32 i = 0; i < questsToAssign; ++i)
    {
        AssignQuestToBot(recommended[i], bot);
    }
}

QuestPriority DynamicQuestSystem::CalculateQuestPriority(uint32 questId, Player* bot)
{
    if (!bot)
        return QuestPriority::TRIVIAL;

    const Quest* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return QuestPriority::TRIVIAL;

    uint32 botLevel = bot->GetLevel();
    // Estimate quest level from max level or use bot level as baseline
    uint32 questLevel = quest->GetMaxLevel() > 0 ? quest->GetMaxLevel() : botLevel;

    // Base priority on level difference
    if (questLevel > botLevel + 2)
        return QuestPriority::TRIVIAL;
    else if (questLevel < botLevel - 5)
        return QuestPriority::LOW;
    else if (questLevel <= botLevel + 1)
        return QuestPriority::NORMAL;

    // Boost priority for special quest types
    // Note: TrinityCore doesn't have QUEST_FLAGS_ELITE, using other criteria for special quests

    if (quest->IsDFQuest() || quest->IsRaidQuest(DIFFICULTY_NORMAL))
        return QuestPriority::LEGENDARY;

    // Check for quest chains
    if (IsPartOfQuestChain(questId))
        return QuestPriority::HIGH;

    // Boost for high experience rewards
    uint32 expReward = quest->XPValue(bot);
    if (expReward > botLevel * 200)
        return QuestPriority::HIGH;

    return QuestPriority::NORMAL;
}

std::vector<uint32> DynamicQuestSystem::SortQuestsByPriority(const std::vector<uint32>& questIds, Player* bot)
{
    std::vector<uint32> sortedQuests = questIds;

    std::sort(sortedQuests.begin(), sortedQuests.end(),
        [this, bot](uint32 a, uint32 b) {
            QuestPriority priorityA = CalculateQuestPriority(a, bot);
            QuestPriority priorityB = CalculateQuestPriority(b, bot);

            if (priorityA != priorityB)
                return priorityA > priorityB;

            // Secondary sort by experience reward
            const Quest* questA = sObjectMgr->GetQuestTemplate(a);
            const Quest* questB = sObjectMgr->GetQuestTemplate(b);

            if (questA && questB)
                return questA->XPValue(bot) > questB->XPValue(bot);

            return false;
        });

    return sortedQuests;
}

bool DynamicQuestSystem::ShouldAbandonQuest(uint32 questId, Player* bot)
{
    if (!bot)
        return false;

    const Quest* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return true;

    uint32 botGuid = bot->GetGUID().GetCounter();
    // Find quest progress
    auto progressIt = _botQuestProgress.find(botGuid);
    if (progressIt == _botQuestProgress.end())
        return false;

    auto questProgress = std::find_if(progressIt->second.begin(), progressIt->second.end(),
        [questId](const QuestProgress& progress) {
            return progress.questId == questId;
        });

    if (questProgress == progressIt->second.end())
        return false;

    // Check if quest is stuck
    if (questProgress->isStuck && questProgress->retryCount >= MAX_QUEST_RETRIES)
        return true;

    // Check if quest is taking too long
    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime - questProgress->startTime > 3600000) // 1 hour
        return true;

    // Check if quest is no longer level appropriate
    uint32 botLevel = bot->GetLevel();
    uint32 questLevel = quest->GetMaxLevel() > 0 ? quest->GetMaxLevel() : botLevel;
    if (questLevel < botLevel - 7) // Too low level
        return true;

    return false;
}

void DynamicQuestSystem::UpdateQuestProgress(Player* bot)
{
    if (!bot)
        return;

    uint32 botGuid = bot->GetGUID().GetCounter();
    auto progressIt = _botQuestProgress.find(botGuid);
    if (progressIt == _botQuestProgress.end())
        return;

    for (auto& progress : progressIt->second)
    {
        UpdateQuestObjectiveProgress(progress, sObjectMgr->GetQuestTemplate(progress.questId), bot);

        // Check if quest is completed
        if (progress.completionPercentage >= 100.0f)
        {
            HandleQuestCompletion(bot, progress.questId);
        }
    }
}

void DynamicQuestSystem::ExecuteQuestObjective(Player* bot, uint32 questId, uint32 objectiveIndex)
{
    if (!bot)
        return;

    const Quest* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return;

    // Get objective details and execute appropriate actions
    // This would involve creature killing, item collection, etc.

    TC_LOG_DEBUG("playerbot.quest", "Bot {} executing objective {} for quest {}",
                bot->GetName(), objectiveIndex, questId);
}

bool DynamicQuestSystem::CanCompleteQuestObjective(Player* bot, uint32 questId, uint32 objectiveIndex)
{
    if (!bot)
        return false;

    const Quest* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    // Check if bot has the necessary requirements to complete the objective
    // Level, items, location, etc.

    return true; // Simplified for now
}

void DynamicQuestSystem::HandleQuestCompletion(Player* bot, uint32 questId)
{
    if (!bot)
        return;

    uint32 botGuid = bot->GetGUID().GetCounter();
    TC_LOG_INFO("playerbot.quest", "Bot {} completed quest {}", bot->GetName(), questId);

    // Remove from active quests
    {
        auto progressIt = _botQuestProgress.find(botGuid);
        if (progressIt != _botQuestProgress.end())
        {
            progressIt->second.erase(
                std::remove_if(progressIt->second.begin(), progressIt->second.end(),
                    [questId](const QuestProgress& progress) {
                        return progress.questId == questId;
                    }),
                progressIt->second.end()
            );
        }
    }

    // Update metrics
    auto& metrics = _botMetrics[botGuid];
    metrics.questsCompleted++;

    // Check for quest chain progression
    uint32 nextQuest = GetNextQuestInChain(questId);
    if (nextQuest != 0)
    {
        AssignQuestToBot(nextQuest, bot);
    }
}
bool DynamicQuestSystem::FormQuestGroup(uint32 questId, Player* initiator)
{
    if (!initiator)
        return false;

    const Quest* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    // Only form groups for group quests (sharable or raid group quests)
    if (!quest->HasFlag(QUEST_FLAGS_SHARABLE) && !quest->HasFlag(QUEST_FLAGS_RAID_GROUP_OK))
        return false;

    // Find other players who need this quest
    std::vector<Player*> eligiblePlayers;

    // This would require a more sophisticated player lookup system
    // For now, just check group members if already in a group

    Group* group = initiator->GetGroup();
    if (group)
    {
        for (GroupReference const& itr : group->GetMembers())
        {
            Player* member = itr.GetSource();
            if (member && member != initiator && CanShareQuest(questId, initiator, member))
            {
                eligiblePlayers.push_back(member);
            }
        }
    }

    if (!eligiblePlayers.empty())
    {
        CoordinateGroupQuest(group, questId);
        return true;
    }

    return false;
}

void DynamicQuestSystem::CoordinateGroupQuest(Group* group, uint32 questId)
{
    if (!group)
        return;

    // Coordinate quest execution among group members
    ShareQuestProgress(group, questId);

    // PHASE 2G: Hybrid validation pattern (snapshot + ObjectAccessor fallback)
    for (auto const& slot : group->GetMemberSlots())
    {
        // Quick snapshot check first
        auto memberSnapshot = SpatialGridQueryHelpers::FindPlayerByGuid(nullptr, slot.guid);
        if (!memberSnapshot)
            continue;

        // Fallback to ObjectAccessor for full validation
        Player* member = ObjectAccessor::FindConnectedPlayer(slot.guid);
        if (member && dynamic_cast<BotSession*>(member->GetSession()))
        {
            // Assign specific objectives to this member
            ExecuteQuestObjective(member, questId, 0);
        }
    }
}

void DynamicQuestSystem::ShareQuestProgress(Group* group, uint32 questId)
{
    if (!group)
        return;

    // Share quest progress updates among group members
    // Synchronize objective completion status
}

bool DynamicQuestSystem::CanShareQuest(uint32 questId, Player* from, Player* to)
{
    if (!from || !to)
        return false;

    const Quest* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    // Check if quest can be shared
    if (!quest->HasFlag(QUEST_FLAGS_SHARABLE))
        return false;

    // Check if target player can take the quest
    return to->CanTakeQuest(quest, false);
}

Position DynamicQuestSystem::GetNextQuestLocation(Player* bot, uint32 questId)
{
    Position location;

    if (!bot)
        return location;

    const Quest* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return location;

    // Find the next objective location for this quest
    Position startLocation = FindOptimalQuestStartLocation(questId, bot);

    return startLocation;
}

std::vector<Position> DynamicQuestSystem::GenerateQuestPath(Player* bot, uint32 questId)
{
    std::vector<Position> path;

    if (!bot)
        return path;

    // Generate optimal path for quest completion
    Position startPos = GetNextQuestLocation(bot, questId);
    path.push_back(startPos);

    // Add waypoints based on quest objectives
    // This would require detailed quest objective analysis

    return path;
}

void DynamicQuestSystem::HandleQuestNavigation(Player* bot, uint32 questId)
{
    if (!bot)
        return;

    // Handle navigation to quest objectives
    std::vector<Position> questPath = GenerateQuestPath(bot, questId);
    if (!questPath.empty())
    {
        // Move bot to the next waypoint
        // This would integrate with the bot's movement system
    }
}

bool DynamicQuestSystem::IsQuestLocationReachable(Player* bot, const Position& location)
{
    if (!bot)
        return false;

    // Check if the location is reachable by the bot
    // Consider bot's current position, movement capabilities, etc.

    float distance = bot->GetDistance(location);
    return distance < 1000.0f; // Simplified distance check
}

void DynamicQuestSystem::AdaptQuestDifficulty(uint32 questId, Player* bot)
{
    if (!bot)
        return;

    auto metadataIt = _questMetadata.find(questId);
    if (metadataIt == _questMetadata.end())
        return;

    QuestMetadata& metadata = metadataIt->second;

    // Scale quest for bot's current capabilities
    ScaleQuestForBot(metadata, bot);
}

void DynamicQuestSystem::HandleQuestStuckState(Player* bot, uint32 questId)
{
    if (!bot)
        return;

    uint32 botGuid = bot->GetGUID().GetCounter();
    auto progressIt = _botQuestProgress.find(botGuid);
    if (progressIt == _botQuestProgress.end())
        return;

    auto questProgress = std::find_if(progressIt->second.begin(), progressIt->second.end(),
        [questId](QuestProgress& progress) {
            return progress.questId == questId;
        });

    if (questProgress != progressIt->second.end())
    {
        questProgress->isStuck = true;
        questProgress->stuckTime = GameTime::GetGameTimeMS();
        questProgress->retryCount++;

        TC_LOG_WARN("playerbot.quest", "Bot {} is stuck on quest {}, retry count: {}",
                   bot->GetName(), questId, questProgress->retryCount);
    }
}

void DynamicQuestSystem::RetryFailedObjective(Player* bot, uint32 questId, uint32 objectiveIndex)
{
    if (!bot)
        return;

    // Retry a failed quest objective with different approach
    ExecuteQuestObjective(bot, questId, objectiveIndex);
}

void DynamicQuestSystem::OptimizeQuestOrder(Player* bot)
{
    if (!bot)
        return;

    uint32 botGuid = bot->GetGUID().GetCounter();
    auto progressIt = _botQuestProgress.find(botGuid);
    if (progressIt == _botQuestProgress.end())
        return;

    // Sort active quests by efficiency and location
    std::sort(progressIt->second.begin(), progressIt->second.end(),
        [this, bot](const QuestProgress& a, const QuestProgress& b) {
            float scoreA = CalculateQuestValue(a.questId, bot);
            float scoreB = CalculateQuestValue(b.questId, bot);
            return scoreA > scoreB;
        });
}

void DynamicQuestSystem::TrackQuestChains(Player* bot)
{
    if (!bot)
        return;

    // Track quest chain progression for the bot
    // Ensure appropriate chain advancement
}

std::vector<uint32> DynamicQuestSystem::GetQuestChain(uint32 questId)
{
    std::vector<uint32> chain;

    auto chainIt = _questChains.find(questId);
    if (chainIt != _questChains.end())
        return chainIt->second;

    return chain;
}

uint32 DynamicQuestSystem::GetNextQuestInChain(uint32 completedQuestId)
{
    auto followupIt = _questFollowups.find(completedQuestId);
    if (followupIt != _questFollowups.end() && !followupIt->second.empty())
        return followupIt->second[0];

    return 0;
}

void DynamicQuestSystem::AdvanceQuestChain(Player* bot, uint32 completedQuestId)
{
    if (!bot)
        return;

    uint32 nextQuest = GetNextQuestInChain(completedQuestId);
    if (nextQuest != 0)
    {
        AssignQuestToBot(nextQuest, bot);
    }
}

void DynamicQuestSystem::OptimizeZoneQuests(Player* bot)
{
    if (!bot)
        return;

    uint32 currentZone = bot->GetZoneId();
    std::vector<uint32> zoneQuests = GetZoneQuests(currentZone, bot);

    // Prioritize quests in current zone
    for (uint32 questId : zoneQuests)
    {
        if (CalculateQuestValue(questId, bot) > MIN_QUEST_VALUE_THRESHOLD)
        {
            AssignQuestToBot(questId, bot);
        }
    }
}

std::vector<uint32> DynamicQuestSystem::GetZoneQuests(uint32 zoneId, Player* bot)
{
    std::vector<uint32> zoneQuests;

    auto zoneIt = _zoneQuests.find(zoneId);
    if (zoneIt != _zoneQuests.end())
        return zoneIt->second;

    return zoneQuests;
}

void DynamicQuestSystem::PlanZoneCompletion(Player* bot, uint32 zoneId)
{
    if (!bot)
        return;

    std::vector<uint32> zoneQuests = GetZoneQuests(zoneId, bot);

    // Create completion plan for the zone
    for (uint32 questId : zoneQuests)
    {
        if (CalculateQuestPriority(questId, bot) >= QuestPriority::NORMAL)
        {
            AssignQuestToBot(questId, bot);
        }
    }
}

bool DynamicQuestSystem::ShouldMoveToNewZone(Player* bot)
{
    if (!bot)
        return false;

    uint32 currentZone = bot->GetZoneId();
    std::vector<uint32> zoneQuests = GetZoneQuests(currentZone, bot);

    // Check if there are enough valuable quests in current zone
    uint32 valuableQuests = 0;
    for (uint32 questId : zoneQuests)
    {
        if (CalculateQuestValue(questId, bot) > MIN_QUEST_VALUE_THRESHOLD)
            valuableQuests++;
    }

    return valuableQuests < ZONE_OPTIMIZATION_THRESHOLD;
}

QuestReward DynamicQuestSystem::AnalyzeQuestReward(uint32 questId, Player* bot)
{
    QuestReward reward;

    if (!bot)
        return reward;

    const Quest* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return reward;

    reward.experience = quest->XPValue(bot);
    reward.gold = quest->GetRewMoneyMaxLevel();

    // Analyze item rewards
    for (uint8 i = 0; i < QUEST_REWARD_ITEM_COUNT; ++i)
    {
        if (quest->RewardItemId[i])
        {
            reward.items.push_back(quest->RewardItemId[i]);
        }
    }

    // Calculate gear score improvement
    reward.gearScore = CalculateGearScoreImprovement(bot, reward.items);

    // Calculate overall value
    reward.rewardValue = float(reward.experience) + float(reward.gold) / 100.0f + reward.gearScore * 1000.0f;

    return reward;
}

float DynamicQuestSystem::CalculateQuestValue(uint32 questId, Player* bot)
{
    if (!bot)
        return 0.0f;

    QuestReward reward = AnalyzeQuestReward(questId, bot);
    auto metadataIt = _questMetadata.find(questId);

    if (metadataIt == _questMetadata.end())
        return reward.rewardValue;

    const QuestMetadata& metadata = metadataIt->second;

    // Factor in estimated completion time
    float efficiency = reward.rewardValue / float(metadata.estimatedDuration);

    // Factor in difficulty
    efficiency /= (1.0f + metadata.difficultyRating / 10.0f);

    return efficiency;
}

bool DynamicQuestSystem::IsQuestWorthwhile(uint32 questId, Player* bot)
{
    float value = CalculateQuestValue(questId, bot);
    return value >= MIN_QUEST_VALUE_THRESHOLD;
}

QuestMetrics DynamicQuestSystem::GetBotQuestMetrics(uint32 botGuid)
{
    auto it = _botMetrics.find(botGuid);
    if (it != _botMetrics.end())
        return it->second;

    QuestMetrics metrics;
    metrics.Reset();
    return metrics;
}

QuestMetrics DynamicQuestSystem::GetGlobalQuestMetrics()
{
    QuestMetrics globalMetrics;
    globalMetrics.Reset();

    // Aggregate all bot metrics
    for (const auto& metricsPair : _botMetrics)
    {
        const QuestMetrics& botMetrics = metricsPair.second;
        globalMetrics.questsStarted += botMetrics.questsStarted;
        globalMetrics.questsCompleted += botMetrics.questsCompleted;
        globalMetrics.questsAbandoned += botMetrics.questsAbandoned;
        globalMetrics.questsFailed += botMetrics.questsFailed;
        globalMetrics.experienceGained += botMetrics.experienceGained;
        globalMetrics.goldEarned += botMetrics.goldEarned;
    }

    return globalMetrics;
}

void DynamicQuestSystem::SetQuestStrategy(uint32 botGuid, QuestSelectionStrategy strategy)
{
    _botStrategies[botGuid] = strategy;
}

QuestSelectionStrategy DynamicQuestSystem::GetQuestStrategy(uint32 botGuid)
{
    auto it = _botStrategies.find(botGuid);
    if (it != _botStrategies.end())
        return it->second;

    return QuestSelectionStrategy::LEVEL_PROGRESSION;
}

void DynamicQuestSystem::SetMaxConcurrentQuests(uint32 botGuid, uint32 maxQuests)
{
    // Implementation would store this setting per bot
    // For now, just log the setting
    TC_LOG_DEBUG("playerbot.quest", "Set max concurrent quests for bot {} to {}", botGuid, maxQuests);
}

void DynamicQuestSystem::EnableQuestGrouping(uint32 botGuid, bool enable)
{
    // Implementation would store this setting per bot
    TC_LOG_DEBUG("playerbot.quest", "Quest grouping for bot {} set to {}", botGuid, enable);
}

void DynamicQuestSystem::LoadQuestMetadata()
{
    // Load quest metadata from database or initialize from templates
    for (auto const& questPair : sObjectMgr->GetQuestTemplates())
    {
        Quest const* quest = questPair.second.get();
        if (!quest)
            continue;

        uint32 questId = quest->GetQuestId();
        _questMetadata[questId] = QuestMetadata(questId);
        PopulateQuestMetadata(questId);
    }

    TC_LOG_INFO("playerbot.quest", "Loaded metadata for {} quests", _questMetadata.size());
}

void DynamicQuestSystem::AnalyzeQuestDependencies()
{
    // Analyze quest prerequisites and dependencies
    for (auto const& questPair : sObjectMgr->GetQuestTemplates())
    {
        Quest const* quest = questPair.second.get();
        if (!quest)
            continue;
        uint32 questId = quest->GetQuestId();

        // Check for prerequisite quests
        if (quest->GetPrevQuestId() != 0)
        {
            _questPrerequisites[questId].push_back(quest->GetPrevQuestId());
        }

        // Check for followup quests (simplified)
        if (quest->GetNextQuestId() != 0)
        {
            _questFollowups[questId].push_back(quest->GetNextQuestId());
        }
    }
}

void DynamicQuestSystem::BuildQuestChains()
{
    // Build quest chains from dependencies
    for (const auto& followupPair : _questFollowups)
    {
        uint32 questId = followupPair.first;
        std::vector<uint32> chain;
        std::unordered_set<uint32> visited; // Cycle detection

        // Build chain forward
        uint32 currentQuest = questId;
        while (currentQuest != 0)
        {
            // Cycle detection - prevent infinite loops from circular quest references
            if (visited.count(currentQuest) > 0)
            {
                TC_LOG_WARN("playerbot.quest", "DynamicQuestSystem: Circular quest chain detected at quest {}, breaking chain", currentQuest);
                break;
            }
            visited.insert(currentQuest);

            chain.push_back(currentQuest);
            auto nextIt = _questFollowups.find(currentQuest);
            if (nextIt != _questFollowups.end() && !nextIt->second.empty())
                currentQuest = nextIt->second[0];
            else
                break;
        }

        if (chain.size() > 1)
        {
            _questChains[questId] = chain;
        }
    }
}

void DynamicQuestSystem::OptimizeQuestRoutes()
{
    // Optimize quest routes by grouping quests by location
    // This would require detailed zone and location analysis
}

QuestType DynamicQuestSystem::DetermineQuestType(const Quest* quest)
{
    if (!quest)
        return QuestType::KILL_COLLECT;

    // Analyze quest objectives to determine type
    if (quest->IsDFQuest())
        return QuestType::DUNGEON;

    if (quest->IsRaidQuest(DIFFICULTY_NORMAL))
        return QuestType::ELITE;

    if (quest->HasFlag(QUEST_FLAGS_DAILY))
        return QuestType::DAILY;

    if (quest->HasFlag(QUEST_FLAGS_FLAGS_PVP))
        return QuestType::PVP;

    // Default classification based on objectives
    bool hasKillObjectives = false;
    bool hasItemObjectives = false;

    for (QuestObjective const& obj : quest->GetObjectives())
    {
        if (obj.Type == QUEST_OBJECTIVE_MONSTER)
            hasKillObjectives = true;
        else if (obj.Type == QUEST_OBJECTIVE_ITEM)
            hasItemObjectives = true;
    }

    if (hasKillObjectives || hasItemObjectives)
        return QuestType::KILL_COLLECT;

    return QuestType::INTERACTION;
}

float DynamicQuestSystem::CalculateQuestDifficulty(const Quest* quest, Player* bot)
{
    if (!quest || !bot)
        return 5.0f;

    float difficulty = 5.0f; // Base difficulty

    // Adjust for level difference
    uint32 questLevel = quest->GetMaxLevel() > 0 ? quest->GetMaxLevel() : bot->GetLevel();
    uint32 levelDiff = questLevel > bot->GetLevel() ?
                      questLevel - bot->GetLevel() : 0;
    difficulty += levelDiff * 0.5f;

    // Adjust for quest flags
    // Note: No QUEST_FLAGS_ELITE in TrinityCore, using alternative logic
    if (quest->IsDFQuest())
        difficulty *= 1.5f;

    if (quest->IsRaidQuest(DIFFICULTY_NORMAL))
        difficulty *= 3.0f;

    return std::clamp(difficulty, 1.0f, 10.0f);
}

bool DynamicQuestSystem::MeetsQuestRequirements(const Quest* quest, Player* bot)
{
    if (!quest || !bot)
        return false;

    return bot->CanTakeQuest(quest, false);
}

Position DynamicQuestSystem::FindOptimalQuestStartLocation(uint32 questId, Player* bot)
{
    Position location;

    if (!bot)
        return location;

    // Find the best starting location for this quest
    // This would require quest giver location lookup

    return bot->GetPosition(); // Default to current position
}

void DynamicQuestSystem::UpdateQuestObjectiveProgress(QuestProgress& progress, const Quest* quest, Player* bot)
{
    if (!quest || !bot)
        return;

    // Update objective completion percentages
    uint32 completedObjectives = 0;
    uint32 totalObjectives = 0;

    // Check objectives using modern TrinityCore API
    uint32 objectiveIndex = 0;
    for (QuestObjective const& obj : quest->GetObjectives())
    {
        totalObjectives++;
        uint32 currentCount = 0;

        switch (obj.Type)
        {
            case QUEST_OBJECTIVE_MONSTER:
                currentCount = bot->GetReqKillOrCastCurrentCount(quest->GetQuestId(), obj.ObjectID);
                break;
            case QUEST_OBJECTIVE_ITEM:
                currentCount = bot->GetItemCount(obj.ObjectID, true);
                break;
            default:
                // Handle other objective types as needed
                break;
        }

        progress.objectiveTargets[objectiveIndex] = obj.Amount;
        progress.objectiveProgress[objectiveIndex] = currentCount;

        if (currentCount >= obj.Amount)
            completedObjectives++;

        objectiveIndex++;
    }

    // Calculate completion percentage
    if (totalObjectives > 0)
    {
        progress.completionPercentage = (float(completedObjectives) / float(totalObjectives)) * 100.0f;
    }

    progress.lastUpdateTime = GameTime::GetGameTimeMS();
}

bool DynamicQuestSystem::IsQuestObjectiveComplete(const QuestProgress& progress, uint32 objectiveIndex)
{
    auto progressIt = progress.objectiveProgress.find(objectiveIndex);
    auto targetIt = progress.objectiveTargets.find(objectiveIndex);

    if (progressIt != progress.objectiveProgress.end() && targetIt != progress.objectiveTargets.end())
    {
        return progressIt->second >= targetIt->second;
    }

    return false;
}

void DynamicQuestSystem::PopulateQuestMetadata(uint32 questId)
{
    const Quest* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return;

    auto& metadata = _questMetadata[questId];
    metadata.questId = questId;
    metadata.type = DetermineQuestType(quest);
    uint32 questLevel = quest->GetMaxLevel() > 0 ? quest->GetMaxLevel() : 1;
    metadata.recommendedLevel = questLevel;
    metadata.minLevel = 1; // Quests are generally available from level 1
    metadata.maxLevel = quest->GetMaxLevel();
    metadata.isElite = false; // TrinityCore doesn't have QUEST_FLAGS_ELITE
    metadata.isDungeon = quest->IsDFQuest();
    metadata.isRaid = quest->IsRaidQuest(DIFFICULTY_NORMAL);
    metadata.isDaily = quest->HasFlag(QUEST_FLAGS_DAILY);

    // Estimate duration based on objectives using modern API
    metadata.estimatedDuration = 600; // Base 10 minutes

    // Count objectives to estimate duration
    uint32 killObjectives = 0;
    uint32 itemObjectives = 0;

    for (QuestObjective const& obj : quest->GetObjectives())
    {
        if (obj.Type == QUEST_OBJECTIVE_MONSTER)
            killObjectives += obj.Amount;
        else if (obj.Type == QUEST_OBJECTIVE_ITEM)
            itemObjectives += obj.Amount;
    }

    metadata.estimatedDuration += killObjectives * 30; // 30 seconds per kill
    metadata.estimatedDuration += itemObjectives * 60; // 1 minute per item

    if (metadata.isElite)
        metadata.estimatedDuration *= 2;
    if (metadata.isDungeon)
        metadata.estimatedDuration *= 3;
}

bool DynamicQuestSystem::IsPartOfQuestChain(uint32 questId)
{
    return _questChains.find(questId) != _questChains.end();
}

float DynamicQuestSystem::CalculateGearScoreImprovement(Player* bot, const std::vector<uint32>& items)
{
    if (!bot || items.empty())
        return 0.0f;

    float improvement = 0.0f;

    for (uint32 itemId : items)
    {
        const ItemTemplate* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
        if (itemTemplate)
        {
            // Simplified gear score calculation using proper API
            improvement += itemTemplate->GetBaseItemLevel() * 0.1f;
        }
    }

    return improvement;
}

void DynamicQuestSystem::ExecuteSoloStrategy(Player* bot)
{
    // Focus on quests that can be completed solo
}

void DynamicQuestSystem::ExecuteGroupStrategy(Player* bot)
{
    // Prefer group quests and elite content
}

void DynamicQuestSystem::ExecuteZoneStrategy(Player* bot)
{
    // Focus on completing all quests in current zone
    OptimizeZoneQuests(bot);
}

void DynamicQuestSystem::ExecuteLevelStrategy(Player* bot)
{
    // Focus on quests that provide optimal experience
}

void DynamicQuestSystem::ExecuteGearStrategy(Player* bot)
{
    // Focus on quests that provide equipment upgrades
}

void DynamicQuestSystem::ExecuteStoryStrategy(Player* bot)
{
    // Focus on main storyline and quest chains
}

void DynamicQuestSystem::ExecuteReputationStrategy(Player* bot)
{
    // Focus on reputation-granting quests
}

void DynamicQuestSystem::ScaleQuestForBot(QuestMetadata& metadata, Player* bot)
{
    if (!bot)
        return;

    // Adjust quest parameters based on bot's capabilities
    uint32 botLevel = bot->GetLevel();

    // Scale difficulty based on level
    if (metadata.recommendedLevel < botLevel - 2)
    {
        metadata.difficultyRating *= 0.8f; // Easier for higher level
    }
    else if (metadata.recommendedLevel > botLevel + 2)
    {
        metadata.difficultyRating *= 1.2f; // Harder for lower level
    }
}

void DynamicQuestSystem::Update(uint32 diff)
{
    static uint32 lastUpdate = 0;
    uint32 currentTime = GameTime::GetGameTimeMS();

    if (currentTime - lastUpdate < QUEST_UPDATE_INTERVAL)
        return;

    lastUpdate = currentTime;

    // Clean up completed quests
    CleanupCompletedQuests();

    // Validate quest states
    ValidateQuestStates();
}

void DynamicQuestSystem::CleanupCompletedQuests()
{
    uint32 currentTime = GameTime::GetGameTimeMS();

    // Clean up old quest progress data
    for (auto it = _botQuestProgress.begin(); it != _botQuestProgress.end(); ++it)
    {
        auto& progressList = it->second;
        progressList.erase(
            std::remove_if(progressList.begin(), progressList.end(),
                [currentTime](const QuestProgress& progress) {
                    return currentTime - progress.lastUpdateTime > 3600000; // 1 hour old
                }),
            progressList.end()
        );
    }
}

void DynamicQuestSystem::ValidateQuestStates()
{
    // Validate that quest system state is consistent
    // Check for orphaned data, invalid references, etc.
}

} // namespace Playerbot