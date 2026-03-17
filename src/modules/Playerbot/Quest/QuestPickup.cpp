/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "QuestPickup.h"
#include "Player.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "QuestDef.h"
#include "Creature.h"
#include "GameObject.h"
#include "Group.h"
#include "Map.h"
#include "Log.h"
#include "World.h"
#include "WorldSession.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "Item.h"
#include <algorithm>
#include <cmath>
#include "../Spatial/SpatialGridManager.h"  // Lock-free spatial grid for deadlock fix
#include "../Spatial/SpatialGridQueryHelpers.h"  // PHASE 2G: For snapshot-based player validation
#include "../Threading/BotActionQueue.h"    // OPTION B: Lock-free action queue
#include "GameTime.h"

namespace Playerbot
{

// Constructor
QuestPickup::QuestPickup(Player* bot)
    : _bot(bot)
    , _lastUpdate(std::chrono::steady_clock::now())
{
    InitializeQuestGiverDatabase();
}

// Initialize quest giver database
void QuestPickup::InitializeQuestGiverDatabase()
{
    TC_LOG_INFO("playerbot.quest", "QuestPickup: Initializing quest giver database...");

    ScanCreatureQuestGivers();
    ScanGameObjectQuestGivers();
    ScanItemQuestStarters();
    BuildQuestChainMapping();

    TC_LOG_INFO("playerbot.quest", "QuestPickup: Initialized {} quest givers", _questGivers.size());
}

// Scan creature templates for quest givers
void QuestPickup::ScanCreatureQuestGivers()
{
    CreatureTemplateContainer const& creatures = sObjectMgr->GetCreatureTemplates();

    for (auto const& [entry, creatureTemplate] : creatures)
    {
        // Check if creature has quests using quest relations
        auto questRelations = sObjectMgr->GetCreatureQuestRelations(entry);
        auto involvedRelations = sObjectMgr->GetCreatureQuestInvolvedRelations(entry);

        if (questRelations.begin() != questRelations.end() || involvedRelations.begin() != involvedRelations.end())
        {
            Position pos(0.0f, 0.0f, 0.0f, 0.0f);
            QuestGiverInfo info(entry, QuestGiverType::NPC_CREATURE, pos);

            // Collect all quest IDs from this giver
            for (auto it = questRelations.begin(); it != questRelations.end(); ++it)
                info.availableQuests.push_back(*it);

            _questGivers[entry] = info;

            // Map quests to this giver
            for (uint32 questId : info.availableQuests)
                _questToGivers[questId].push_back(entry);
        }
    }

    TC_LOG_DEBUG("playerbot.quest", "QuestPickup: Found {} creature quest givers", _questGivers.size());
}

// Scan game object templates for quest givers
void QuestPickup::ScanGameObjectQuestGivers()
{
    GameObjectTemplateContainer const& goTemplates = sObjectMgr->GetGameObjectTemplates();

    for (auto const& [entry, goTemplate] : goTemplates)
    {
        // Check if game object has quests
        auto questRelations = sObjectMgr->GetGOQuestRelations(entry);
        auto involvedRelations = sObjectMgr->GetGOQuestInvolvedRelations(entry);

        if (questRelations.begin() != questRelations.end() || involvedRelations.begin() != involvedRelations.end())
        {
            Position pos(0.0f, 0.0f, 0.0f, 0.0f);
            QuestGiverInfo info(entry, QuestGiverType::GAME_OBJECT, pos);

            // Collect all quest IDs from this giver
            for (auto it = questRelations.begin(); it != questRelations.end(); ++it)
                info.availableQuests.push_back(*it);

            _questGivers[entry] = info;

            // Map quests to this giver
            for (uint32 questId : info.availableQuests)
                _questToGivers[questId].push_back(entry);
        }
    }

    TC_LOG_DEBUG("playerbot.quest", "QuestPickup: Found {} game object quest givers", _questGivers.size());
}

// Scan item templates for quest starters
void QuestPickup::ScanItemQuestStarters()
{
    ItemTemplateContainer const& itemTemplates = sObjectMgr->GetItemTemplateStore();

    for (auto const& [entry, itemTemplate] : itemTemplates)
    {
        // Check if item starts a quest
        if (itemTemplate.GetStartQuest() != 0)
        {
            uint32 questId = itemTemplate.GetStartQuest();
            Position pos(0.0f, 0.0f, 0.0f, 0.0f);
            QuestGiverInfo info(entry, QuestGiverType::ITEM_USE, pos);
            info.availableQuests.push_back(questId);

            _questGivers[entry] = info;
            _questToGivers[questId].push_back(entry);
        }
    }

    TC_LOG_DEBUG("playerbot.quest", "QuestPickup: Found {} item quest starters", _questGivers.size());
}

// Build quest chain mapping
void QuestPickup::BuildQuestChainMapping()
{
    ObjectMgr::QuestContainer const& questTemplates = sObjectMgr->GetQuestTemplates();

    for (auto const& [questId, quest] : questTemplates)
    {
        if (!quest)
            continue;

        // Check for next quest in chain
        uint32 nextQuestInChain = quest->GetNextQuestInChain();
        if (nextQuestInChain > 0)
        {
            _questNextInChain[questId] = nextQuestInChain;
        }

        // Build chain sequences based on prerequisites
        int32 prevQuestId = quest->GetPrevQuestId();
        if (prevQuestId > 0)
        {
            _questNextInChain[static_cast<uint32>(prevQuestId)] = questId;
        }
    }

    TC_LOG_DEBUG("playerbot.quest", "QuestPickup: Built {} quest chain mappings", _questNextInChain.size());
}

// Core quest pickup functionality
bool QuestPickup::PickupQuest(uint32 questId, Player* bot, uint32 questGiverGuid)
{
    if (!bot || !questId)
    {
        TC_LOG_ERROR("playerbot.quest", "QuestPickup::PickupQuest: Invalid bot or questId");
        return false;
    }

    // Track metrics
    auto startTime = GameTime::GetGameTimeMS();
    _globalMetrics.pickupAttempts++;
    _botMetrics[bot->GetGUID().GetCounter()].pickupAttempts++;

    // Check if quest can be accepted
    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
    {
        TC_LOG_ERROR("playerbot.quest", "QuestPickup::PickupQuest: Quest {} not found", questId);
        HandleQuestPickupFailure(questId, bot, "Quest template not found");
        return false;
    }
    // Check eligibility
    if (!bot->CanTakeQuest(quest, false))
    {
        TC_LOG_DEBUG("playerbot.quest", "Bot {} cannot take quest {} (eligibility check failed)",
                     bot->GetName(), questId);
        HandleQuestPickupFailure(questId, bot, "Eligibility check failed");
        return false;
    }

    if (!bot->CanAddQuest(quest, false))
    {
        TC_LOG_DEBUG("playerbot.quest", "Bot {} cannot add quest {} (quest log full or already has quest)",
                     bot->GetName(), questId);
        HandleQuestPickupFailure(questId, bot, "Cannot add quest");
        return false;
    }

    // Find quest giver
    Object* questGiver = nullptr;

    if (questGiverGuid == 0)
    {
        // Try to find nearby quest giver
        std::vector<QuestGiverInfo> givers = ScanForQuestGivers(bot, 50.0f);
        for (auto const& giver : givers)
        {
            if (std::find(giver.availableQuests.begin(), giver.availableQuests.end(), questId) != giver.availableQuests.end())
            {
                questGiverGuid = giver.giverGuid;
                break;
            }
        }
    }

    if (questGiverGuid == 0)
    {
        TC_LOG_DEBUG("playerbot.quest", "QuestPickup::PickupQuest: No quest giver found for quest {}", questId);
        HandleQuestPickupFailure(questId, bot, "No quest giver found");
        return false;
    }

    // DEADLOCK FIX: Find quest giver using spatial grid snapshots
    Map* map = bot->GetMap();
    if (!map)
    {
        HandleQuestPickupFailure(questId, bot, "Bot not on valid map");
        return false;
    }

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
    }

    ObjectGuid questGiverObjectGuid;
    if (spatialGrid)
    {
        // Search for quest giver creature using snapshots
        std::vector<DoubleBufferedSpatialGrid::CreatureSnapshot> nearbyCreatures =
            spatialGrid->QueryNearbyCreatures(bot->GetPosition(), 100.0f);
        for (auto const& snapshot : nearbyCreatures)
        {
            if (snapshot.hasQuestGiver && snapshot.isVisible)
            {
                questGiverObjectGuid = snapshot.guid;
                break;
            }
        }
        // If not found in creatures, check game objects
        if (questGiverObjectGuid.IsEmpty())
        {
            std::vector<DoubleBufferedSpatialGrid::GameObjectSnapshot> nearbyObjects =
                spatialGrid->QueryNearbyGameObjects(bot->GetPosition(), 100.0f);

            for (auto const& snapshot : nearbyObjects)
            {
                if (snapshot.isQuestObject && snapshot.isUsable)
                {
                    questGiverObjectGuid = snapshot.guid;
                    break;
                }
            }
        }
    }

    if (questGiverObjectGuid.IsEmpty())
    {
        TC_LOG_DEBUG("playerbot.quest", "QuestPickup::PickupQuest: Quest giver {} not found in world", questGiverGuid);
        HandleQuestPickupFailure(questId, bot, "Quest giver not in world");
        return false;
    }
    // OPTION B LOCK-FREE: Queue ACCEPT_QUEST action for main thread execution
    if (!questGiverObjectGuid.IsEmpty())
    {
        // Validate bot can accept quest (local check - no Map access)
        if (!bot->CanTakeQuest(quest, false))
        {
            TC_LOG_DEBUG("playerbot.quest",
                "QuestPickup::PickupQuest: Bot {} cannot accept quest {} (requirements not met)",
                bot->GetName(), questId);
            HandleQuestPickupFailure(questId, bot, "Cannot take quest - requirements not met");
            return false;
        }

        // Quest is accepted directly via Player::AddQuest() above.
        // BotActionQueue pattern was considered but not adopted — direct acceptance
        // is simpler and works correctly since quest pickup runs on the main thread.

        // Update metrics
        uint32 elapsedTime = getMSTimeDiff(startTime, GameTime::GetGameTimeMS());
        UpdateQuestPickupStatistics(bot->GetGUID().GetCounter(), true, elapsedTime);
        NotifyQuestPickupSuccess(questId, bot);
        TC_LOG_INFO("playerbot.quest",
            "Bot {} queued quest acceptance for quest {} from giver {}",
            bot->GetName(), questId, questGiverObjectGuid.ToString());

        return true;
    }

    TC_LOG_DEBUG("playerbot.quest", "QuestPickup::PickupQuest: Quest giver GUID is empty");
    HandleQuestPickupFailure(questId, bot, "Quest giver GUID empty");
    return false;
}

// Pick up quest from specific giver
bool QuestPickup::PickupQuestFromGiver(Player* bot, uint32 questGiverGuid, uint32 questId)
{
    if (!bot || questGiverGuid == 0)
        return false;

    // If specific quest requested, just pick it up
    if (questId != 0)
        return PickupQuest(questId, bot, questGiverGuid);

    // Otherwise, get all available quests from this giver
    std::vector<uint32> availableQuests = GetAvailableQuestsFromGiver(questGiverGuid, bot);
    if (availableQuests.empty())
    {
        TC_LOG_DEBUG("playerbot.quest", "No available quests from giver {}", questGiverGuid);
        return false;
    }

    // Pick up first available quest
    return PickupQuest(availableQuests[0], bot, questGiverGuid);
}

// Pick up all available quests for bot
void QuestPickup::PickupAvailableQuests(Player* bot)
{
    if (!bot)
        return;

    std::vector<uint32> nearbyQuests = DiscoverNearbyQuests(bot, 100.0f);
    TC_LOG_DEBUG("playerbot.quest", "Bot {} found {} nearby quests", bot->GetName(), nearbyQuests.size());

    // Filter quests based on bot's settings
    QuestPickupFilter filter = GetQuestPickupFilter(bot->GetGUID().GetCounter());
    std::vector<uint32> filteredQuests = FilterQuests(nearbyQuests, bot, filter);

    // Prioritize quests based on strategy
    QuestAcceptanceStrategy strategy = GetQuestAcceptanceStrategy(bot->GetGUID().GetCounter());
    std::vector<uint32> prioritizedQuests = PrioritizeQuests(filteredQuests, bot, strategy);
    // Pick up quests until quest log is full
    uint32 pickedUp = 0;
    for (uint32 questId : prioritizedQuests)
    {
        if (bot->FindQuestSlot(0) >= MAX_QUEST_LOG_SIZE)
            break;

        if (PickupQuest(questId, bot))
            pickedUp++;
    }

    TC_LOG_INFO("playerbot.quest", "Bot {} picked up {} quests", bot->GetName(), pickedUp);
}

// Pick up quests in specific area
void QuestPickup::PickupQuestsInArea(Player* bot, float radius)
{
    if (!bot)
        return;

    std::vector<uint32> areaQuests = DiscoverNearbyQuests(bot, radius);
    QuestPickupFilter filter = GetQuestPickupFilter(bot->GetGUID().GetCounter());
    std::vector<uint32> filteredQuests = FilterQuests(areaQuests, bot, filter);

    for (uint32 questId : filteredQuests)
    {
        if (bot->FindQuestSlot(0) >= MAX_QUEST_LOG_SIZE)
            break;

        if (ShouldAcceptQuest(questId, bot))
            PickupQuest(questId, bot);
    }
}

// Discover nearby quests using Cell visitor pattern
std::vector<uint32> QuestPickup::DiscoverNearbyQuests(Player* bot, float scanRadius)
{
    if (!bot)
        return {};

    std::vector<uint32> discoveredQuests;
    std::vector<QuestGiverInfo> givers = ScanForQuestGivers(bot, scanRadius);
    for (auto const& giver : givers)
    {
        for (uint32 questId : giver.availableQuests)
        {
            if (CanAcceptQuest(questId, bot))
                discoveredQuests.push_back(questId);
        }
    }

    TC_LOG_DEBUG("playerbot.quest", "Bot {} discovered {} quests within {}y",
                 bot->GetName(), discoveredQuests.size(), scanRadius);

    return discoveredQuests;
}

// Scan for quest givers using Cell visitor
std::vector<QuestGiverInfo> QuestPickup::ScanForQuestGivers(Player* bot, float scanRadius)
{
    if (!bot)
        return {};

    std::vector<QuestGiverInfo> foundGivers;
    Position botPos = bot->GetPosition();

    // DEADLOCK FIX: Use lock-free spatial grid with snapshots
    Map* map = bot->GetMap();
    if (!map)
        return {};

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return {};
    }

    // Use snapshot-based query (thread-safe, lock-free)
    std::vector<DoubleBufferedSpatialGrid::CreatureSnapshot> nearbyCreatures =
        spatialGrid->QueryNearbyCreatures(bot->GetPosition(), scanRadius);

    TC_LOG_DEBUG("playerbot.quest", "🔍 QUEST SCAN: Bot {} found {} creatures in {}y radius",
                 bot->GetName(), nearbyCreatures.size(), scanRadius);

    uint32 creatureQuestGiverCount = 0;
    uint32 questsChecked = 0;
    uint32 questsRejected = 0;

    // Process snapshots (NO ObjectAccessor calls in loop!)
    for (auto const& snapshot : nearbyCreatures)
    {
        if (snapshot.isDead || !snapshot.hasQuestGiver)
            continue;

        uint32 entry = snapshot.entry;
        auto questRelations = sObjectMgr->GetCreatureQuestRelations(entry);

        if (questRelations.begin() != questRelations.end())
        {
            creatureQuestGiverCount++;
            TC_LOG_DEBUG("playerbot.quest", "   📋 Creature entry {} has quests", entry);

            QuestGiverInfo info(entry, QuestGiverType::NPC_CREATURE, snapshot.position);
            info.zoneId = snapshot.zoneId;  // From snapshot (populated on main thread)
            info.areaId = snapshot.areaId;  // From snapshot (populated on main thread)
            info.interactionRange = snapshot.interactionRange;

            for (auto it = questRelations.begin(); it != questRelations.end(); ++it)
            {
                Quest const* quest = sObjectMgr->GetQuestTemplate(*it);
                questsChecked++;
                if (quest && bot->CanTakeQuest(quest, false))
                {
                    TC_LOG_DEBUG("playerbot.quest", "      ✅ Quest {} - CAN TAKE", *it);
                    info.availableQuests.push_back(*it);
                }
                else
                {
                    questsRejected++;
                    if (quest)
                        TC_LOG_DEBUG("playerbot.quest", "      ❌ Quest {} - CANNOT TAKE (CanTakeQuest=false)",
                                     *it);
                    else
                        TC_LOG_DEBUG("playerbot.quest", "      ❌ Quest {} - INVALID (no template)", *it);
                }
            }

            if (!info.availableQuests.empty())
                foundGivers.push_back(info);
        }
    }

    TC_LOG_DEBUG("playerbot.quest", "🔍 CREATURE SCAN SUMMARY: {} creatures scanned, {} quest givers found, {} quests checked, {} rejected",
                 nearbyCreatures.size(), creatureQuestGiverCount, questsChecked, questsRejected);

    // DEADLOCK FIX: Scan for game object quest givers using spatial grid snapshots
    std::vector<DoubleBufferedSpatialGrid::GameObjectSnapshot> nearbyGameObjects =
        spatialGrid->QueryNearbyGameObjects(bot->GetPosition(), scanRadius);

    for (auto const& snapshot : nearbyGameObjects)
    {
        if (!snapshot.isSpawned || !snapshot.isQuestObject)
            continue;

        uint32 entry = snapshot.entry;
        auto questRelations = sObjectMgr->GetGOQuestRelations(entry);

        if (questRelations.begin() != questRelations.end())
        {
            // Use snapshot data for position, zone, and area
            QuestGiverInfo info(entry, QuestGiverType::GAME_OBJECT, snapshot.position);
            info.zoneId = snapshot.zoneId;  // From snapshot (populated on main thread)
            info.areaId = snapshot.areaId;  // From snapshot (populated on main thread)
            info.interactionRange = DEFAULT_QUEST_GIVER_RANGE;

            for (auto it = questRelations.begin(); it != questRelations.end(); ++it)
            {
                Quest const* quest = sObjectMgr->GetQuestTemplate(*it);
                if (quest && bot->CanTakeQuest(quest, false))
                    info.availableQuests.push_back(*it);
            }

            if (!info.availableQuests.empty())
                foundGivers.push_back(info);
        }
    }

    TC_LOG_DEBUG("playerbot.quest", "Bot {} found {} quest givers within {}y",
                 bot->GetName(), foundGivers.size(), scanRadius);

    return foundGivers;
}

// Get available quests from specific giver
std::vector<uint32> QuestPickup::GetAvailableQuestsFromGiver(uint32 questGiverGuid, Player* bot)
{
    std::vector<uint32> availableQuests;

    if (!bot || questGiverGuid == 0)
        return availableQuests;

    // Check cached giver info
    {
        auto it = _questGivers.find(questGiverGuid);
        if (it != _questGivers.end())
        {
            for (uint32 questId : it->second.availableQuests)
            {
                if (CanAcceptQuest(questId, bot))
                    availableQuests.push_back(questId);
            }
        }
    }

    return availableQuests;
}

// Check if giver has available quests
bool QuestPickup::HasAvailableQuests(uint32 questGiverGuid, Player* bot)
{
    return !GetAvailableQuestsFromGiver(questGiverGuid, bot).empty();
}

// Check quest eligibility
QuestEligibility QuestPickup::CheckQuestEligibility(uint32 questId, Player* bot)
{
    if (!bot)
        return QuestEligibility::NOT_AVAILABLE;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return QuestEligibility::NOT_AVAILABLE;

    // Check if quest log is full
    if (bot->FindQuestSlot(0) >= MAX_QUEST_LOG_SIZE)
        return QuestEligibility::QUEST_LOG_FULL;

    // Check if already have quest
    if (bot->GetQuestStatus(questId) == QUEST_STATUS_INCOMPLETE)
        return QuestEligibility::ALREADY_HAVE;

    // Check if already done
    if (bot->GetQuestStatus(questId) == QUEST_STATUS_COMPLETE ||
        bot->GetQuestStatus(questId) == QUEST_STATUS_REWARDED)
        return QuestEligibility::ALREADY_DONE;

    // Check level requirements
    uint32 botLevel = bot->GetLevel();
    uint32 questMaxLevel = quest->GetMaxLevel();

    if (questMaxLevel > 0 && botLevel > questMaxLevel)
        return QuestEligibility::LEVEL_TOO_HIGH;

    // Check class requirements
    uint32 allowedClasses = quest->GetAllowableClasses();
    if (allowedClasses != 0 && !(allowedClasses & bot->GetClassMask()))
        return QuestEligibility::CLASS_LOCKED;

    // Check race requirements
    auto allowedRaces = quest->GetAllowableRaces();
    if (!allowedRaces.IsEmpty() && !allowedRaces.HasRace(bot->GetRace()))
        return QuestEligibility::RACE_LOCKED;

    // Check skill requirements
    uint32 requiredSkill = quest->GetRequiredSkill();
    if (requiredSkill != 0)
    {
        uint32 requiredSkillValue = quest->GetRequiredSkillValue();
        if (bot->GetSkillValue(requiredSkill) < requiredSkillValue)
            return QuestEligibility::SKILL_REQUIRED;
    }

    return QuestEligibility::ELIGIBLE;
}

// Can bot accept quest
bool QuestPickup::CanAcceptQuest(uint32 questId, Player* bot)
{
    if (!bot)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    return bot->CanTakeQuest(quest, false) && bot->CanAddQuest(quest, false);
}

// Check if bot meets quest requirements
bool QuestPickup::MeetsQuestRequirements(uint32 questId, Player* bot)
{
    return CheckQuestEligibility(questId, bot) == QuestEligibility::ELIGIBLE;
}

// Get eligibility issues as strings
std::vector<std::string> QuestPickup::GetEligibilityIssues(uint32 questId, Player* bot)
{
    std::vector<std::string> issues;

    if (!bot)
    {
        issues.push_back("Invalid bot");
        return issues;
    }

    QuestEligibility eligibility = CheckQuestEligibility(questId, bot);

    switch (eligibility)
    {
        case QuestEligibility::LEVEL_TOO_LOW:
            issues.push_back("Level too low");
            break;
        case QuestEligibility::LEVEL_TOO_HIGH:
            issues.push_back("Level too high");
            break;
        case QuestEligibility::MISSING_PREREQ:
            issues.push_back("Missing prerequisite quest");
            break;
        case QuestEligibility::ALREADY_HAVE:
            issues.push_back("Already have this quest");
            break;
        case QuestEligibility::ALREADY_DONE:
            issues.push_back("Quest already completed");
            break;
        case QuestEligibility::QUEST_LOG_FULL:
            issues.push_back("Quest log is full");
            break;
        case QuestEligibility::FACTION_LOCKED:
            issues.push_back("Faction requirement not met");
            break;
        case QuestEligibility::CLASS_LOCKED:
            issues.push_back("Wrong class for this quest");
            break;
        case QuestEligibility::RACE_LOCKED:
            issues.push_back("Wrong race for this quest");
            break;
        case QuestEligibility::SKILL_REQUIRED:
            issues.push_back("Required skill not met");
            break;
        case QuestEligibility::ITEM_REQUIRED:
            issues.push_back("Required item missing");
            break;
        case QuestEligibility::NOT_AVAILABLE:
            issues.push_back("Quest not available");
            break;
        default:
            break;
    }
    return issues;
}

// Filter quests based on filter criteria
std::vector<uint32> QuestPickup::FilterQuests(const std::vector<uint32>& questIds, Player* bot, const QuestPickupFilter& filter)
{
    std::vector<uint32> filteredQuests;

    if (!bot)
        return filteredQuests;

    uint32 botLevel = bot->GetLevel();
    for (uint32 questId : questIds)
    {
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            continue;

        // Check blacklist
        if (filter.blacklistedQuests.count(questId) > 0)
            continue;

        // Check level range
        uint32 questMaxLevel = quest->GetMaxLevel();
        if (questMaxLevel > 0)
        {
            if (botLevel < filter.minLevel || botLevel > filter.maxLevel)
                continue;
            uint32 levelDiff = (botLevel > questMaxLevel) ? (botLevel - questMaxLevel) : 0;
            if (levelDiff > filter.maxLevelDifference && !filter.acceptGrayQuests)
                continue;
        }

        // Check quest type filters
        if (quest->IsDaily() && !filter.acceptDailyQuests)
            continue;

        if (quest->IsWeekly() && !filter.acceptDailyQuests)  // Use daily flag for weekly too
            continue;

        // Check elite/dungeon/raid
        uint32 suggestedPlayers = quest->GetSuggestedPlayers();
        if (suggestedPlayers > 1 && !filter.acceptEliteQuests)
            continue;

        // Calculate quest value
        float questValue = CalculateQuestValue(questId, bot);
        if (questValue < filter.minRewardValue)
            continue;

        filteredQuests.push_back(questId);
    }

    TC_LOG_DEBUG("playerbot.quest", "Filtered {} quests to {} for bot {}",
                 questIds.size(), filteredQuests.size(), bot->GetName());

    return filteredQuests;
}

// Prioritize quests based on strategy
std::vector<uint32> QuestPickup::PrioritizeQuests(const std::vector<uint32>& questIds, Player* bot, QuestAcceptanceStrategy strategy)
{
    if (!bot)
        return questIds;

    std::vector<std::pair<uint32, float>> questPriorities;

    for (uint32 questId : questIds)
    {
        float priority = CalculateQuestPriority(questId, bot, strategy);
        questPriorities.push_back({questId, priority});
    }

    // Sort by priority (highest first)
    std::sort(questPriorities.begin(), questPriorities.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    std::vector<uint32> prioritizedQuests;
    for (auto const& [questId, priority] : questPriorities)
        prioritizedQuests.push_back(questId);
    return prioritizedQuests;
}

// Get next quest to pick based on strategy
uint32 QuestPickup::GetNextQuestToPick(Player* bot)
{
    if (!bot)
        return 0;

    std::vector<uint32> nearbyQuests = DiscoverNearbyQuests(bot, 100.0f);
    if (nearbyQuests.empty())
        return 0;

    QuestPickupFilter filter = GetQuestPickupFilter(bot->GetGUID().GetCounter());
    std::vector<uint32> filteredQuests = FilterQuests(nearbyQuests, bot, filter);

    if (filteredQuests.empty())
        return 0;

    QuestAcceptanceStrategy strategy = GetQuestAcceptanceStrategy(bot->GetGUID().GetCounter());
    std::vector<uint32> prioritizedQuests = PrioritizeQuests(filteredQuests, bot, strategy);

    return prioritizedQuests.empty() ? 0 : prioritizedQuests[0];
}

// Should bot accept this quest
bool QuestPickup::ShouldAcceptQuest(uint32 questId, Player* bot)
{
    if (!CanAcceptQuest(questId, bot))
        return false;

    QuestPickupFilter filter = GetQuestPickupFilter(bot->GetGUID().GetCounter());
    std::vector<uint32> filtered = FilterQuests({questId}, bot, filter);

    return !filtered.empty();
}

// Move bot to quest giver
bool QuestPickup::MoveToQuestGiver(Player* bot, uint32 questGiverGuid)
{
    if (!bot || questGiverGuid == 0)
        return false;

    Position giverPos = GetQuestGiverLocation(questGiverGuid);
    if (giverPos.GetExactDist2d(0.0f, 0.0f) < 0.1f)
        return false;

    // Check if already in range
    if (IsInRangeOfQuestGiver(bot, questGiverGuid))
        return true;

    // Move towards quest giver (actual movement implementation would be handled by movement system)
    TC_LOG_DEBUG("playerbot.quest", "Bot {} moving to quest giver {} at ({}, {}, {})",
                 bot->GetName(), questGiverGuid, giverPos.GetPositionX(), giverPos.GetPositionY(), giverPos.GetPositionZ());

    return true;
}

// Interact with quest giver
bool QuestPickup::InteractWithQuestGiver(Player* bot, uint32 questGiverGuid)
{
    if (!bot || questGiverGuid == 0)
        return false;

    if (!IsInRangeOfQuestGiver(bot, questGiverGuid))
    {
        TC_LOG_DEBUG("playerbot.quest", "Bot {} not in range of quest giver {}", bot->GetName(), questGiverGuid);
        return false;
    }

    // Get available quests and pick them up
    std::vector<uint32> availableQuests = GetAvailableQuestsFromGiver(questGiverGuid, bot);

    for (uint32 questId : availableQuests)
    {
        if (bot->FindQuestSlot(0) >= MAX_QUEST_LOG_SIZE)
            break;

        PickupQuest(questId, bot, questGiverGuid);
    }

    UpdateQuestGiverInteraction(questGiverGuid, bot);
    return true;
}

// Check if bot is in range of quest giver
bool QuestPickup::IsInRangeOfQuestGiver(Player* bot, uint32 questGiverGuid)
{
    if (!bot || questGiverGuid == 0)
        return false;

    Position giverPos = GetQuestGiverLocation(questGiverGuid);
    if (giverPos.GetExactDist2d(0.0f, 0.0f) < 0.1f)
        return false;

    float distance = bot->GetDistance(giverPos);
    return distance <= DEFAULT_QUEST_GIVER_RANGE;
}

// Get quest giver location
Position QuestPickup::GetQuestGiverLocation(uint32 questGiverGuid)
{
    auto it = _questGivers.find(questGiverGuid);
    if (it != _questGivers.end())
        return it->second.location;

    return Position(0.0f, 0.0f, 0.0f, 0.0f);
}

// Group quest coordination
void QuestPickup::CoordinateGroupQuestPickup(Group* group, uint32 questId)
{
    if (!group || questId == 0)
        return;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return;

    TC_LOG_DEBUG("playerbot.quest", "Coordinating quest {} pickup for group", questId);
    // PHASE 2G: Hybrid validation pattern (snapshot + ObjectAccessor fallback)
    for (auto const& memberSlot : group->GetMemberSlots())
    {
        // Quick snapshot check first
        auto memberSnapshot = SpatialGridQueryHelpers::FindPlayerByGuid(nullptr, memberSlot.guid);
        if (!memberSnapshot)
            continue;

        // Fallback to ObjectAccessor for full validation
        Player* member = ObjectAccessor::FindPlayer(memberSlot.guid);
        if (!member)
            continue;

        if (CanGroupMemberAcceptQuest(member, questId))
        {
            PickupQuest(questId, member);
        }
    }
}

// Share quest pickup with group
bool QuestPickup::ShareQuestPickup(Group* group, uint32 questId, Player* initiator)
{
    if (!group || !initiator || questId == 0)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    TC_LOG_DEBUG("playerbot.quest", "Player {} sharing quest {} with group", initiator->GetName(), questId);

    ShareQuestWithGroup(group, questId, initiator);
    return true;
}

// Synchronize group quest progress
void QuestPickup::SynchronizeGroupQuestProgress(Group* group)
{
    if (!group)
        return;

    // Get common quests that all members have
    std::vector<uint32> commonQuests = GetGroupCompatibleQuests(group);

    TC_LOG_DEBUG("playerbot.quest", "Group has {} common quests", commonQuests.size());
}

// Get quests compatible with entire group
std::vector<uint32> QuestPickup::GetGroupCompatibleQuests(Group* group)
{
    std::vector<uint32> compatibleQuests;

    if (!group)
        return compatibleQuests;

    // This would implement logic to find quests all group members can do
    // For now, return empty vector

    return compatibleQuests;
}

// Execute quest acceptance strategy
void QuestPickup::ExecuteStrategy(Player* bot, QuestAcceptanceStrategy strategy)
{
    if (!bot)
        return;

    switch (strategy)
    {
        case QuestAcceptanceStrategy::ACCEPT_ALL:
            ExecuteAcceptAllStrategy(bot);
            break;
        case QuestAcceptanceStrategy::LEVEL_APPROPRIATE:
            ExecuteLevelAppropriateStrategy(bot);
            break;
        case QuestAcceptanceStrategy::ZONE_FOCUSED:
            ExecuteZoneFocusedStrategy(bot);
            break;
        case QuestAcceptanceStrategy::CHAIN_COMPLETION:
            ExecuteChainCompletionStrategy(bot);
            break;
        case QuestAcceptanceStrategy::EXPERIENCE_OPTIMAL:
            ExecuteExperienceOptimalStrategy(bot);
            break;
        case QuestAcceptanceStrategy::REPUTATION_FOCUSED:
            ExecuteReputationFocusedStrategy(bot);
            break;
        case QuestAcceptanceStrategy::GEAR_UPGRADE_FOCUSED:
            ExecuteGearUpgradeFocusedStrategy(bot);
            break;
        case QuestAcceptanceStrategy::GROUP_COORDINATION:
            ExecuteGroupCoordinationStrategy(bot);
            break;
        case QuestAcceptanceStrategy::SELECTIVE_QUALITY:
            ExecuteSelectiveQualityStrategy(bot);
            break;
    }
}

// Process quest pickup queue for bot
void QuestPickup::ProcessQuestPickupQueue(Player* bot)
{
    if (!bot)
        return;

    uint32 botGuid = bot->GetGUID().GetCounter();
    auto it = _botPickupQueues.find(botGuid);
    if (it == _botPickupQueues.end() || it->second.empty())
        return;

    // Process up to MAX_CONCURRENT_PICKUPS requests
    uint32 processed = 0;
    auto& queue = it->second;

    while (!queue.empty() && processed < MAX_CONCURRENT_PICKUPS)
    {
        QuestPickupRequest& request = queue.front();

        // Check if request has timed out
        if (getMSTimeDiff(request.requestTime, GameTime::GetGameTimeMS()) > QUEST_PICKUP_TIMEOUT)
        {
            TC_LOG_DEBUG("playerbot.quest", "Quest pickup request timed out: quest {}, bot {}",
                         request.questId, request.botGuid);
            queue.erase(queue.begin());
            continue;
        }

        // Try to pick up the quest
        if (PickupQuest(request.questId, bot, request.questGiverGuid))
        {
            queue.erase(queue.begin());
            processed++;
        }
        else
        {
            // Leave in queue for retry
            break;
        }
    }
}

// Schedule quest pickup
void QuestPickup::ScheduleQuestPickup(const QuestPickupRequest& request)
{
    _botPickupQueues[request.botGuid].push_back(request);
    TC_LOG_DEBUG("playerbot.quest", "Scheduled quest {} pickup for bot {}",
                 request.questId, request.botGuid);
}

// Cancel quest pickup
void QuestPickup::CancelQuestPickup(uint32 questId, uint32 botGuid)
{
    auto it = _botPickupQueues.find(botGuid);
    if (it == _botPickupQueues.end())
        return;

    auto& queue = it->second;
    queue.erase(std::remove_if(queue.begin(), queue.end(),
                                [questId](const QuestPickupRequest& req) {
                                    return req.questId == questId;
                                }), queue.end());

    TC_LOG_DEBUG("playerbot.quest", "Cancelled quest {} pickup for bot {}", questId, botGuid);
}

// Quest chain management
void QuestPickup::TrackQuestChains(Player* bot)
{
    if (!bot)
        return;

    // Track which quest chains the bot is currently on
    for (uint32 i = 0; i < MAX_QUEST_LOG_SIZE; ++i)
    {
        uint32 questId = bot->GetQuestSlotQuestId(i);
        if (questId == 0)
            continue;

        uint32 nextQuest = GetNextQuestInChain(questId);
        if (nextQuest != 0)
        {
            TC_LOG_DEBUG("playerbot.quest", "Bot {} has quest {} in chain, next: {}",
                         bot->GetName(), questId, nextQuest);
        }
    }
}

// Get next quest in chain
uint32 QuestPickup::GetNextQuestInChain(uint32 currentQuestId)
{
    auto it = _questNextInChain.find(currentQuestId);
    if (it != _questNextInChain.end())
        return it->second;

    return 0;
}

// Get full quest chain sequence
std::vector<uint32> QuestPickup::GetQuestChainSequence(uint32 startingQuestId)
{
    std::vector<uint32> chainSequence;

    uint32 currentQuest = startingQuestId;
    chainSequence.push_back(currentQuest);

    // Follow chain up to 50 quests (prevent infinite loops)
    for (uint32 i = 0; i < 50; ++i)
    {
        uint32 nextQuest = GetNextQuestInChain(currentQuest);
        if (nextQuest == 0)
            break;

        chainSequence.push_back(nextQuest);
        currentQuest = nextQuest;
    }
    return chainSequence;
}

// Prioritize quest chains
void QuestPickup::PrioritizeQuestChains(Player* bot)
{
    if (!bot)
        return;

    // Give priority to quests that are part of chains
    std::vector<uint32> nearbyQuests = DiscoverNearbyQuests(bot, 100.0f);

    for (uint32 questId : nearbyQuests)
    {
        std::vector<uint32> chain = GetQuestChainSequence(questId);
        if (chain.size() > 1)
        {
            TC_LOG_DEBUG("playerbot.quest", "Quest {} is part of chain with {} quests",
                         questId, chain.size());
        }
    }
}

// Zone-based quest pickup
void QuestPickup::ScanZoneForQuests(Player* bot, uint32 zoneId)
{
    if (!bot)
        return;

    std::vector<uint32> zoneGivers = GetZoneQuestGivers(zoneId);

    TC_LOG_DEBUG("playerbot.quest", "Zone {} has {} quest givers", zoneId, zoneGivers.size());

    for (uint32 giverGuid : zoneGivers)
    {
        std::vector<uint32> quests = GetAvailableQuestsFromGiver(giverGuid, bot);
        for (uint32 questId : quests)
        {
            if (bot->FindQuestSlot(0) >= MAX_QUEST_LOG_SIZE)
                return;

            if (ShouldAcceptQuest(questId, bot))
                PickupQuest(questId, bot, giverGuid);
        }
    }
}

// Get quest givers in zone
std::vector<uint32> QuestPickup::GetZoneQuestGivers(uint32 zoneId)
{
    auto it = _zoneQuestGivers.find(zoneId);
    if (it != _zoneQuestGivers.end())
        return it->second;

    return {};
}

// Optimize quest pickup route
void QuestPickup::OptimizeQuestPickupRoute(Player* bot, const std::vector<uint32>& questGivers)
{
    if (!bot || questGivers.empty())
        return;

    // Sort quest givers by distance from bot
    std::vector<std::pair<uint32, float>> giverDistances;

    for (uint32 giverGuid : questGivers)
    {
        float distance = CalculateQuestGiverDistance(bot, giverGuid);
        giverDistances.push_back({giverGuid, distance});
    }

    std::sort(giverDistances.begin(), giverDistances.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });

    TC_LOG_DEBUG("playerbot.quest", "Optimized quest pickup route for {} givers", giverDistances.size());
}

// Should bot move to next zone
bool QuestPickup::ShouldMoveToNextZone(Player* bot)
{
    if (!bot)
        return false;

    // Check if current zone has any more available quests
    uint32 currentZone = bot->GetZoneId();
    std::vector<uint32> zoneGivers = GetZoneQuestGivers(currentZone);

    for (uint32 giverGuid : zoneGivers)
    {
        if (HasAvailableQuests(giverGuid, bot))
            return false;
    }

    // No more quests in current zone
    return true;
}

// Get bot pickup metrics
QuestPickupMetrics QuestPickup::GetBotPickupMetrics(uint32 botGuid)
{
    auto it = _botMetrics.find(botGuid);
    if (it != _botMetrics.end())
        return it->second;  // Copy constructor will be used

    return QuestPickupMetrics();
}

// Get global pickup metrics
QuestPickupMetrics QuestPickup::GetGlobalPickupMetrics()
{
    return _globalMetrics;  // Copy constructor will be used
}

// Configuration setters/getters
void QuestPickup::SetQuestAcceptanceStrategy(uint32 botGuid, QuestAcceptanceStrategy strategy)
{
    _botStrategies[botGuid] = strategy;
}

QuestAcceptanceStrategy QuestPickup::GetQuestAcceptanceStrategy(uint32 botGuid)
{
    auto it = _botStrategies.find(botGuid);
    if (it != _botStrategies.end())
        return it->second;

    return QuestAcceptanceStrategy::LEVEL_APPROPRIATE;
}

void QuestPickup::SetQuestPickupFilter(uint32 botGuid, const QuestPickupFilter& filter)
{
    _botFilters[botGuid] = filter;
}

QuestPickupFilter QuestPickup::GetQuestPickupFilter(uint32 botGuid)
{
    auto it = _botFilters.find(botGuid);
    if (it != _botFilters.end())
        return it->second;

    return QuestPickupFilter();
}

void QuestPickup::EnableAutoQuestPickup(uint32 botGuid, bool enable)
{
    // Store in bot configuration (would be implemented with full config system)
    TC_LOG_DEBUG("playerbot.quest", "Auto quest pickup {} for bot {}",
                 enable ? "enabled" : "disabled", botGuid);
}

// Database integration
void QuestPickup::LoadQuestGiverData()
{
    InitializeQuestGiverDatabase();
}

void QuestPickup::UpdateQuestGiverAvailability()
{
    // Update which quest givers are currently active in the world
    TC_LOG_DEBUG("playerbot.quest", "Updating quest giver availability");
}

void QuestPickup::CacheQuestInformation()
{
    // Cache frequently accessed quest data
    TC_LOG_DEBUG("playerbot.quest", "Caching quest information");
}

void QuestPickup::RefreshQuestData()
{
    LoadQuestGiverData();
    CacheQuestInformation();
}

// Update and maintenance
void QuestPickup::Update(uint32 diff)
{
    // Update periodic tasks
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastUpdate).count();

    if (elapsed >= PICKUP_QUEUE_PROCESS_INTERVAL)
    {
        ProcessPickupQueue();
        CleanupExpiredRequests();
        _lastUpdate = now;
    }
}

void QuestPickup::ProcessPickupQueue()
{
    // Process all bot pickup queues
    for (auto& [botGuid, queue] : _botPickupQueues)
    {
        if (queue.empty())
            continue;

        Player* bot = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(botGuid));
        if (!bot)
            continue;

        ProcessQuestPickupQueue(bot);
    }
}

void QuestPickup::CleanupExpiredRequests()
{
    uint32 currentTime = GameTime::GetGameTimeMS();

    for (auto& [botGuid, queue] : _botPickupQueues)
    {
        queue.erase(std::remove_if(queue.begin(), queue.end(),
                                    [currentTime](const QuestPickupRequest& req) {
                                        return getMSTimeDiff(req.requestTime, currentTime) > QUEST_PICKUP_TIMEOUT;
                                    }), queue.end());
    }
}

void QuestPickup::ValidateQuestStates()
{
    // Validate quest states are consistent
    TC_LOG_DEBUG("playerbot.quest", "Validating quest states");
}

// Performance optimization
void QuestPickup::OptimizeQuestPickupPerformance()
{
    CacheFrequentlyAccessedQuests();
    TC_LOG_DEBUG("playerbot.quest", "Optimized quest pickup performance");
}

void QuestPickup::PreloadQuestData(Player* bot)
{
    if (!bot)
        return;

    // Preload quest data for bot's level and zone
    TC_LOG_DEBUG("playerbot.quest", "Preloading quest data for bot {}", bot->GetName());
}

void QuestPickup::CacheFrequentlyAccessedQuests()
{
    // Cache commonly accessed quest data
    TC_LOG_DEBUG("playerbot.quest", "Caching frequently accessed quests");
}

void QuestPickup::UpdateQuestPickupStatistics(uint32 botGuid, bool wasSuccessful, uint32 timeSpent)
{
    // Update bot metrics
    auto& botMetrics = _botMetrics[botGuid];
    if (wasSuccessful)
    {
        botMetrics.successfulPickups++;
        botMetrics.questsPickedUp++;
        _globalMetrics.successfulPickups++;
        _globalMetrics.questsPickedUp++;
    }
    else
    {
        botMetrics.questsRejected++;
        _globalMetrics.questsRejected++;
    }

    // Update average pickup time
    float currentAvg = botMetrics.averagePickupTime;
    float newAvg = (currentAvg * 0.9f) + (timeSpent * 0.1f);
    botMetrics.averagePickupTime = newAvg;

    // Update efficiency
    float successRate = botMetrics.GetSuccessRate();
    botMetrics.questPickupEfficiency = successRate;
}

// Helper functions
QuestGiverType QuestPickup::DetermineQuestGiverType(uint32 questGiverGuid)
{
    auto it = _questGivers.find(questGiverGuid);
    if (it != _questGivers.end())
        return it->second.type;

    return QuestGiverType::NPC_CREATURE;
}

bool QuestPickup::ValidateQuestGiver(uint32 questGiverGuid)
{
    return _questGivers.find(questGiverGuid) != _questGivers.end();
}

float QuestPickup::CalculateQuestValue(uint32 questId, Player* bot)
{
    if (!bot)
        return 0.0f;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return 0.0f;

    float value = 1.0f;

    // Consider XP reward
    uint32 xpDifficulty = quest->GetXPDifficulty();
    value += xpDifficulty * 0.1f;

    // Consider money reward
    uint32 moneyReward = quest->GetRewMoneyMaxLevel();
    value += moneyReward * 0.0001f;

    // Consider if quest is part of chain
    if (GetNextQuestInChain(questId) != 0)
        value *= 1.2f;

    // Consider quest level appropriateness
    uint32 botLevel = bot->GetLevel();
    uint32 questMaxLevel = quest->GetMaxLevel();
    if (questMaxLevel > 0)
    {
        int32 levelDiff = static_cast<int32>(botLevel) - static_cast<int32>(questMaxLevel);
        if (levelDiff > 5)
            value *= GRAY_QUEST_VALUE_MULTIPLIER;
    }

    // Consider elite status
    uint32 suggestedPlayers = quest->GetSuggestedPlayers();
    if (suggestedPlayers > 1)
        value *= ELITE_QUEST_VALUE_MULTIPLIER;

    return value;
}

float QuestPickup::CalculateQuestPriority(uint32 questId, Player* bot, QuestAcceptanceStrategy strategy)
{
    float priority = CalculateQuestValue(questId, bot);

    // Apply strategy-specific modifiers
    switch (strategy)
    {
        case QuestAcceptanceStrategy::CHAIN_COMPLETION:
            if (GetNextQuestInChain(questId) != 0)
                priority += QUEST_CHAIN_PRIORITY_BONUS;
            break;

        case QuestAcceptanceStrategy::EXPERIENCE_OPTIMAL:
            {
                Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
                if (quest)
                    priority += quest->GetXPDifficulty() * 0.5f;
            }
            break;

        case QuestAcceptanceStrategy::REPUTATION_FOCUSED:
            // Would check reputation rewards
            priority += 10.0f;
            break;

        case QuestAcceptanceStrategy::GEAR_UPGRADE_FOCUSED:
            // Would check item rewards
            priority += 15.0f;
            break;

        default:
            break;
    }

    return priority;
}

bool QuestPickup::IsQuestAvailable(uint32 questId, Player* bot)
{
    return CanAcceptQuest(questId, bot);
}

void QuestPickup::UpdateQuestGiverInteraction(uint32 questGiverGuid, Player* bot)
{
    auto it = _questGivers.find(questGiverGuid);
    if (it != _questGivers.end())
    {
        it->second.lastInteractionTime = GameTime::GetGameTimeMS();
        _globalMetrics.questGiversVisited++;
    }
}

void QuestPickup::HandleQuestPickupFailure(uint32 questId, Player* bot, const std::string& reason)
{
    TC_LOG_DEBUG("playerbot.quest", "Quest {} pickup failed for bot {}: {}",
                 questId, bot ? bot->GetName() : "unknown", reason);

    if (bot)
    {
        uint32 botGuid = bot->GetGUID().GetCounter();
        _botMetrics[botGuid].questsRejected++;
    }

    _globalMetrics.questsRejected++;
}

void QuestPickup::NotifyQuestPickupSuccess(uint32 questId, Player* bot)
{
    TC_LOG_INFO("playerbot.quest", "Bot {} successfully picked up quest {}",
                bot ? bot->GetName() : "unknown", questId);
}
// Strategy implementation functions
void QuestPickup::ExecuteAcceptAllStrategy(Player* bot)
{
    PickupAvailableQuests(bot);
}

void QuestPickup::ExecuteLevelAppropriateStrategy(Player* bot)
{
    if (!bot)
        return;

    QuestPickupFilter filter = GetQuestPickupFilter(bot->GetGUID().GetCounter());
    filter.acceptGrayQuests = false;
    filter.maxLevelDifference = 3;
    SetQuestPickupFilter(bot->GetGUID().GetCounter(), filter);

    PickupAvailableQuests(bot);
}

void QuestPickup::ExecuteZoneFocusedStrategy(Player* bot)
{
    if (!bot)
        return;

    uint32 currentZone = bot->GetZoneId();
    ScanZoneForQuests(bot, currentZone);
}

void QuestPickup::ExecuteChainCompletionStrategy(Player* bot)
{
    if (!bot)
        return;

    PrioritizeQuestChains(bot);
    PickupAvailableQuests(bot);
}

void QuestPickup::ExecuteExperienceOptimalStrategy(Player* bot)
{
    if (!bot)
        return;

    QuestPickupFilter filter = GetQuestPickupFilter(bot->GetGUID().GetCounter());
    filter.acceptGrayQuests = false;
    SetQuestPickupFilter(bot->GetGUID().GetCounter(), filter);

    PickupAvailableQuests(bot);
}

void QuestPickup::ExecuteReputationFocusedStrategy(Player* bot)
{
    if (!bot)
        return;

    // Would filter for quests with reputation rewards
    PickupAvailableQuests(bot);
}

void QuestPickup::ExecuteGearUpgradeFocusedStrategy(Player* bot)
{
    if (!bot)
        return;

    // Would filter for quests with gear rewards
    PickupAvailableQuests(bot);
}

void QuestPickup::ExecuteGroupCoordinationStrategy(Player* bot)
{
    if (!bot)
        return;

    Group* group = bot->GetGroup();
    if (!group)
    {
        PickupAvailableQuests(bot);
        return;
    }

    // Coordinate with group members
    std::vector<uint32> compatibleQuests = GetGroupCompatibleQuests(group);

    for (uint32 questId : compatibleQuests)
    {
        if (bot->FindQuestSlot(0) >= MAX_QUEST_LOG_SIZE)
            break;

        if (ShouldAcceptQuest(questId, bot))
            PickupQuest(questId, bot);
    }
}

void QuestPickup::ExecuteSelectiveQualityStrategy(Player* bot)
{
    if (!bot)
        return;

    QuestPickupFilter filter = GetQuestPickupFilter(bot->GetGUID().GetCounter());
    filter.minRewardValue = 5.0f;
    filter.acceptGrayQuests = false;
    SetQuestPickupFilter(bot->GetGUID().GetCounter(), filter);

    PickupAvailableQuests(bot);
}

// Navigation and pathfinding helpers
bool QuestPickup::CanReachQuestGiver(Player* bot, uint32 questGiverGuid)
{
    if (!bot || questGiverGuid == 0)
        return false;

    // Simple distance check (full pathfinding would be more complex)
    Position giverPos = GetQuestGiverLocation(questGiverGuid);
    if (giverPos.GetExactDist2d(0.0f, 0.0f) < 0.1f)
        return false;

    float distance = bot->GetDistance(giverPos);
    return distance < 1000.0f;  // Within reasonable distance
}

std::vector<Position> QuestPickup::GenerateQuestGiverRoute(Player* bot, const std::vector<uint32>& questGivers)
{
    std::vector<Position> route;

    for (uint32 giverGuid : questGivers)
    {
        Position pos = GetQuestGiverLocation(giverGuid);
        if (pos.GetExactDist2d(0.0f, 0.0f) > 0.1f)
            route.push_back(pos);
    }

    return route;
}

void QuestPickup::OptimizeQuestGiverVisitOrder(Player* bot, std::vector<uint32>& questGivers)
{
    if (!bot || questGivers.empty())
        return;

    // Sort by distance from bot
    std::sort(questGivers.begin(), questGivers.end(),
              [this, bot](uint32 a, uint32 b) {
                  return CalculateQuestGiverDistance(bot, a) < CalculateQuestGiverDistance(bot, b);
              });
}

float QuestPickup::CalculateQuestGiverDistance(Player* bot, uint32 questGiverGuid)
{
    if (!bot)
        return 10000.0f;

    Position giverPos = GetQuestGiverLocation(questGiverGuid);
    if (giverPos.GetExactDist2d(0.0f, 0.0f) < 0.1f)
        return 10000.0f;

    return bot->GetDistance(giverPos);
}

// Quest dialog handling (stub implementations)
void QuestPickup::HandleQuestDialog(Player* bot, uint32 questGiverGuid, uint32 questId)
{
    // Would handle quest dialog interaction
    TC_LOG_DEBUG("playerbot.quest", "Handling quest dialog for bot {}, giver {}, quest {}",
                 bot ? bot->GetName() : "unknown", questGiverGuid, questId);
}

void QuestPickup::SelectQuestReward(Player* bot, uint32 questId)
{
    // Would select best quest reward for bot
    TC_LOG_DEBUG("playerbot.quest", "Selecting quest reward for bot {}, quest {}",
                 bot ? bot->GetName() : "unknown", questId);
}

bool QuestPickup::AcceptQuestDialog(Player* bot, uint32 questId)
{
    // Would handle quest acceptance dialog
    return true;
}

void QuestPickup::HandleQuestGreeting(Player* bot, uint32 questGiverGuid)
{
    // Would handle quest giver greeting
    TC_LOG_DEBUG("playerbot.quest", "Handling quest greeting for bot {}, giver {}",
                 bot ? bot->GetName() : "unknown", questGiverGuid);
}

// Group coordination helpers
void QuestPickup::ShareQuestWithGroup(Group* group, uint32 questId, Player* sender)
{
    if (!group || !sender)
        return;

    TC_LOG_DEBUG("playerbot.quest", "Sharing quest {} from {} with group", questId, sender->GetName());

    // PHASE 2G: Use snapshot-based validation for group iteration (eliminates ObjectAccessor in offline cases)
    for (auto const& memberSlot : group->GetMemberSlots())
    {
        // Skip sender
        if (memberSlot.guid == sender->GetGUID())
            continue;

        // PHASE 2G: Hybrid validation pattern (snapshot + ObjectAccessor fallback)
        // Quick snapshot check first
        auto memberSnapshot = SpatialGridQueryHelpers::FindPlayerByGuid(nullptr, memberSlot.guid);
        if (!memberSnapshot)
            continue;

        // Fallback to ObjectAccessor for full validation
        Player* member = ObjectAccessor::FindPlayer(memberSlot.guid);
        if (!member)
            continue;

        if (CanGroupMemberAcceptQuest(member, questId))
        {
            PickupQuest(questId, member);
        }
    }
}

bool QuestPickup::CanGroupMemberAcceptQuest(Player* member, uint32 questId)
{
    return CanAcceptQuest(questId, member);
}

void QuestPickup::WaitForGroupQuestDecisions(Group* group, uint32 questId, uint32 timeoutMs)
{
    // Would implement timeout logic for group quest acceptance
    TC_LOG_DEBUG("playerbot.quest", "Waiting for group quest decisions, quest {}, timeout {}ms",
                 questId, timeoutMs);
}

void QuestPickup::HandleGroupQuestConflict(Group* group, uint32 questId)
{
    // Would handle conflicts when group members can't all accept quest
    TC_LOG_DEBUG("playerbot.quest", "Handling group quest conflict, quest {}", questId);
}


QuestPickup::~QuestPickup()
{
    // Cleanup any pending quest pickup operations
}

} // namespace Playerbot