/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "QuestManager.h"
#include "GameTime.h"
#include "BotAI.h"
#include "Player.h"
#include "Creature.h"
#include "GameObject.h"
#include "GameObjectData.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "World.h"
#include "Log.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Bag.h"
#include "LootItemType.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "Config/PlayerbotConfig.h"
#include "Group.h"
#include "WorldSession.h"
#include "MotionMaster.h"
#include "Movement/BotMovementUtil.h"
#include "ReputationMgr.h"
#include "../Game/QuestAcceptanceManager.h"
#include <algorithm>
#include <cmath>
#include "../Spatial/SpatialGridManager.h"  // Lock-free spatial grid for deadlock fix
#include "../Spatial/SpatialGridQueryHelpers.h"  // Thread-safe spatial queries

namespace Playerbot
{
    // Configuration defaults
    static constexpr uint32 DEFAULT_UPDATE_INTERVAL = 5000;        // 5 seconds
    static constexpr uint32 QUEST_GIVER_SCAN_INTERVAL = 10000;    // 10 seconds
    static constexpr float QUEST_GIVER_SCAN_RANGE = 30.0f;        // 30 yards
    static constexpr float QUEST_INTERACT_DISTANCE = 5.0f;        // 5 yards
    static constexpr uint32 MAX_QUEST_LOG_SLOT = 25;              // Maximum quest log size
    static constexpr uint32 QUEST_CACHE_TTL = 30000;              // 30 seconds
    static constexpr uint32 MAX_IGNORED_QUESTS = 50;              // Maximum ignored quests
    static constexpr uint32 MAX_RECENTLY_COMPLETED = 20;          // Track last 20 completions

    // QuestStrategy implementation
    QuestSelectionAI::QuestSelectionAI(enum QuestSelectionAI::Strategy strategy)
        : m_strategy(strategy)
    {
    }

    float QuestSelectionAI::EvaluateQuest(Quest const* quest, Player* bot) const
    {
        if (!quest || !bot)
            return 0.0f;

        float score = 0.0f;

        switch (m_strategy)
        {
            case QuestSelectionAI::Strategy::SIMPLE:
                // Basic scoring - just level and XP
                score = 50.0f;
                if (bot->GetQuestLevel(quest) == bot->GetLevel())
                    score += 50.0f;
                break;

            case QuestSelectionAI::Strategy::OPTIMAL:
                // Complex scoring considering all factors
                score = 100.0f;
                // Implementation handled by QuestManager::EvaluateQuestPriority
                break;

            case QuestSelectionAI::Strategy::GROUP:
                // Prioritize group quests (has suggested players > 1)
    if (quest->GetSuggestedPlayers() > 1)
                    score = 100.0f;
                else
                    score = 50.0f;
                break;

            case QuestSelectionAI::Strategy::COMPLETIONIST:
                // Accept all quests in zone
                score = 75.0f;
                break;

            case QuestSelectionAI::Strategy::SPEED_LEVELING:
                // Focus on XP efficiency
                score = bot->GetQuestXPReward(quest) / 100.0f;
                break;
        }

        return score;
    }

    ::std::vector<uint32> QuestSelectionAI::SelectQuestPath(::std::vector<uint32> const& available, Player* bot) const
    {
        ::std::vector<::std::pair<uint32, float>> scoredQuests;

        for (uint32 questId : available)
        {
            if (Quest const* quest = sObjectMgr->GetQuestTemplate(questId))
            {
                float score = EvaluateQuest(quest, bot);
                scoredQuests.push_back({questId, score});
            }
        }

        // Sort by score
        ::std::sort(scoredQuests.begin(), scoredQuests.end(),
            [](auto const& a, auto const& b) { return a.second > b.second; });

        // Return sorted quest IDs
        ::std::vector<uint32> result;
        for (auto const& [questId, score] : scoredQuests)
            result.push_back(questId);

        return result;
    }

    // QuestCache implementation
    QuestCache::QuestCache()
        : m_lastUpdateTime(0)
        , m_isDirty(true)
    {
    }

    void QuestCache::Update(Player* bot)
    {
        if (!bot)
            return;

        // CRITICAL: Check IsInWorld() before accessing quest data!
        // During bot login, the quest status map (m_QuestStatus) may not be fully
        // loaded yet. Accessing it causes ACCESS_VIOLATION in Player::GetQuestStatus()
        // as std::map::find() crashes on uninitialized tree structure.
        if (!bot->IsInWorld())
            return;

        m_questCache.clear();
        m_activeQuests.clear();
        m_completableQuests.clear();

        // Cache all quest statuses
        for (uint8 slot = 0; slot < MAX_QUEST_LOG_SLOT; ++slot)
        {
            uint32 questId = bot->GetQuestSlotQuestId(slot);
            if (questId == 0)
                continue;

            QuestStatus status = bot->GetQuestStatus(questId);

            CachedQuest cached;
            cached.status = status;
            cached.progress = 0.0f;  // Calculate actual progress
            cached.updateTime = GameTime::GetGameTimeMS();

            m_questCache[questId] = cached;

            // Track active and completable quests
    if (status == QUEST_STATUS_INCOMPLETE)
                m_activeQuests.push_back(questId);
            else if (status == QUEST_STATUS_COMPLETE)
                m_completableQuests.push_back(questId);
        }

        m_lastUpdateTime = GameTime::GetGameTimeMS();
        m_isDirty = false;
    }

    void QuestCache::Invalidate()
    {
        m_isDirty = true;
    }

    bool QuestCache::GetQuestStatus(uint32 questId, QuestStatus& status) const
    {
        auto it = m_questCache.find(questId);
        if (it != m_questCache.end())
        {
            status = it->second.status;
            return true;
        }
        return false;
    }

    bool QuestCache::GetQuestProgress(uint32 questId, float& progress) const
    {
        auto it = m_questCache.find(questId);
        if (it != m_questCache.end())
        {
            progress = it->second.progress;
            return true;
        }
        return false;
    }

    bool QuestCache::IsQuestCached(uint32 questId) const
    {
        return m_questCache.find(questId) != m_questCache.end();
    }

    // QuestManager implementation
    QuestManager::QuestManager(Player* bot, BotAI* ai)
        : BehaviorManager(bot, ai, 2000, "QuestManager")
        , m_currentPhase(QuestPhase::IDLE)
        , m_phaseTimer(0)
        , m_lastQuestGiverScan(0)
        , m_questGiverScanInterval(QUEST_GIVER_SCAN_INTERVAL)
        , m_lastPriorityCalculation(0)
        , m_lastAvailableScan(0)
        , m_autoAccept(true)
        , m_autoComplete(true)
        , m_acceptDailies(true)
        , m_acceptDungeonQuests(false)
        , m_prioritizeGroupQuests(true)
        , m_maxActiveQuests(MAX_QUEST_LOG_SLOT)
        , m_maxTravelDistance(1000)
        , m_minQuestLevel(0.75f)  // 75% of bot level
        , m_maxQuestLevel(1.10f)  // 110% of bot level
        , m_strategy(::std::make_unique<QuestSelectionAI>(QuestSelectionAI::Strategy::OPTIMAL))
        , m_cache(::std::make_unique<QuestCache>())
    {
    }

    QuestManager::~QuestManager()
    {
        // OnShutdown will be called by BehaviorManager if needed
    }

    bool QuestManager::OnInitialize()
    {
        // Load configuration from PlayerbotConfig
        m_autoAccept = sPlayerbotConfig->GetBool("Playerbot.Quest.AutoAccept", true);
        m_autoComplete = sPlayerbotConfig->GetBool("Playerbot.Quest.AutoComplete", true);
        m_maxActiveQuests = sPlayerbotConfig->GetInt("Playerbot.Quest.MaxActive", MAX_QUEST_LOG_SLOT);
        m_prioritizeGroupQuests = sPlayerbotConfig->GetBool("Playerbot.Quest.PreferZoneQuests", true);

        // Level range config - MaxLevelDiff controls how far above bot level quests can be
        uint32 maxLevelDiff = sPlayerbotConfig->GetInt("Playerbot.Quest.MaxLevelDiff", 5);

        // Quest level filtering using percentage multipliers
        // m_minQuestLevel: 0.75 means accept quests >= 75% of bot level
        // m_maxQuestLevel: calculated from maxLevelDiff (e.g., 5 levels = 1.05 multiplier at level 100)
        m_minQuestLevel = 0.75f;  // Accept quests 75% of bot level
        m_maxQuestLevel = 1.0f + (static_cast<float>(maxLevelDiff) / 100.0f);

        // Group quest priority multiplier from config
        float groupPriority = sPlayerbotConfig->GetFloat("Playerbot.Quest.GroupQuestPriority", 1.5f);
        // Note: groupPriority is used in EvaluateQuestPriority() - stored in m_prioritizeGroupQuests as bool for now
        // A future enhancement could store the actual float multiplier

        TC_LOG_DEBUG("bot.playerbot", "QuestManager: Loaded config - AutoAccept=%d, AutoComplete=%d, MaxActive=%u, GroupPriority=%.2f",
            m_autoAccept, m_autoComplete, m_maxActiveQuests, groupPriority);

        // CRITICAL: Do NOT call UpdateQuestCache() during initialization!
        // The bot's quest status map (m_QuestStatus) is not loaded yet during the login
        // phase, causing ACCESS_VIOLATION in Player::GetQuestStatus() when it accesses
        // the internal std::map<uint32, QuestStatusData>. The map's tree structure
        // may not be initialized yet, causing crash in std::_Tree::_Find_lower_bound.
        // Quest cache update will happen on first OnUpdate() call when bot is in world.

        // CRITICAL: Do NOT scan for quests during initialization!
        // The bot may not be in the world yet, causing ACCESS_VIOLATION when
        // SpatialGridManager::GetGrid() calls map->GetId() on invalid Map pointer.
        // Quest scanning will happen on first Update() call when bot is in world.

        return true;  // Initialization successful
    }

    void QuestManager::OnUpdate(uint32 elapsed)
    {
        // This method is called by BehaviorManager::Update() at throttled intervals
        // elapsed is the time since last OnUpdate call

        // Update quest cache periodically
    if (GameTime::GetGameTimeMS() - m_cache->GetLastUpdateTime() > QUEST_CACHE_TTL)
            UpdateQuestCache();

        // Update quest phase state machine
        UpdateQuestPhase(elapsed);

        // Update quest progress tracking
        UpdateQuestProgress();

        // Scan for quest givers periodically
    if (GameTime::GetGameTimeMS() - m_lastQuestGiverScan > m_questGiverScanInterval)
        {
            ScanForQuests();
            m_lastQuestGiverScan = GameTime::GetGameTimeMS();
        }

        // Update atomic state flags for strategies
        _hasActiveQuests.store(!m_cache->GetActiveQuests().empty(), ::std::memory_order_release);
        _activeQuestCount.store(m_cache->GetActiveQuests().size(), ::std::memory_order_release);
    }

    void QuestManager::OnShutdown()
    {
        // Clean up all state
        m_currentPhase = QuestPhase::IDLE;
        m_phaseTimer = 0;
        m_questProgress.clear();
        m_ignoredQuests.clear();
        m_questGivers.clear();
        m_questPriorities.clear();
        m_availableQuests.clear();
        m_completableQuests.clear();
        InvalidateCache();

        TC_LOG_DEBUG("bot.playerbot", "QuestManager shutdown for bot %s", GetBot()->GetName().c_str());
    }

    bool QuestManager::CanAcceptQuest(uint32 questId) const
    {
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            return false;

        // Check if already on quest
    if (GetBot()->GetQuestStatus(questId) != QUEST_STATUS_NONE)
            return false;

        // Check if quest is ignored
    if (m_ignoredQuests.find(questId) != m_ignoredQuests.end())
            return false;

        // Check quest log space
    if (GetQuestLogSpace() == 0)
            return false;

        // Check level requirements
    if (IsQuestTooLowLevel(quest) || IsQuestTooHighLevel(quest))
            return false;

        // Check if it's a daily quest
    if (quest->IsDaily() && !m_acceptDailies)
            return false;

        // Check if it's a dungeon/raid quest
    if (quest->IsRaidQuest(DIFFICULTY_NORMAL) && !m_acceptDungeonQuests)
            return false;

        // CRITICAL: Use QuestAcceptanceManager for CORRECT prerequisite checking
        // This includes quest chain validation (GetPrevQuestId, quest_template_addon)
        QuestAcceptanceManager acceptanceMgr(const_cast<Player*>(GetBot()));
        return acceptanceMgr.IsQuestEligible(quest);
    }

    bool QuestManager::AcceptQuest(uint32 questId, WorldObject* questGiver)
    {
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest || !CanAcceptQuest(questId))
            return false;

        // Find quest giver if not provided
    if (!questGiver)
        {
            for (auto const& giver : m_questGivers)
            {
                // PHASE 5D: Thread-safe spatial grid validation
                auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(GetBot(), giver.guid);
                WorldObject* obj = nullptr;

                if (snapshot)
                {
                    // Get WorldObject* for quest acceptance (validated via snapshot first)
                    obj = ObjectAccessor::GetWorldObject(*GetBot(), giver.guid);
                }

                if (obj)
                {
                    questGiver = obj;
                    break;
                }
            }
        }

        // Move to quest giver if too far
    if (questGiver && GetBot()->GetDistance(questGiver) > QUEST_INTERACT_DISTANCE)
        {
            if (!MoveToQuestGiver(questGiver))
                return false;
        }

        // Accept the quest
        GetBot()->AddQuest(quest, questGiver);

        // Initialize quest progress tracking
        QuestProgress progress;
        progress.questId = questId;
        progress.startTime = GameTime::GetGameTimeMS();
        progress.lastUpdateTime = GameTime::GetGameTimeMS();
        m_questProgress[questId] = progress;

        // Update statistics
        m_stats.questsAccepted++;

        TC_LOG_DEBUG("bot.playerbot", "Bot %s accepted quest %u: %s",
            GetBot()->GetName().c_str(), questId, quest->GetLogTitle().c_str());

        return true;
    }

    bool QuestManager::CompleteQuest(uint32 questId, WorldObject* questGiver)
    {
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            return false;

        // Check if quest can be completed
    if (!GetBot()->CanCompleteQuest(questId))
            return false;

        // Complete the quest objectives
        GetBot()->CompleteQuest(questId);

        TC_LOG_DEBUG("bot.playerbot", "Bot %s completed quest %u: %s",
            GetBot()->GetName().c_str(), questId, quest->GetLogTitle().c_str());

        return true;
    }

    bool QuestManager::TurnInQuest(uint32 questId, uint32 rewardChoice, WorldObject* questGiver)
    {
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            return false;

        // Check if quest can be rewarded
    if (!GetBot()->CanRewardQuest(quest, true))
            return false;

        // Find quest giver if not provided
    if (!questGiver)
        {
            for (auto const& giver : m_questGivers)
            {
                // PHASE 5D: Thread-safe spatial grid validation
                auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(GetBot(), giver.guid);
                WorldObject* obj = nullptr;

                if (snapshot)
                {
                    // Get WorldObject* for quest turn-in (validated via snapshot first)
                    obj = ObjectAccessor::GetWorldObject(*GetBot(), giver.guid);
                }

                if (!obj)
                    continue;

                if (Creature* creature = obj->ToCreature())
                {
                    // Check if this creature is involved in the quest
    for (uint32 involvedQuestId : sObjectMgr->GetCreatureQuestInvolvedRelations(creature->GetEntry()))
                    {
                        if (involvedQuestId == questId)
                        {
                            questGiver = creature;
                            break;
                        }
                    }
                    if (questGiver)
                        break;
                }
            }
        }

        // Move to quest giver if too far
    if (questGiver && GetBot()->GetDistance(questGiver) > QUEST_INTERACT_DISTANCE)
        {
            if (!MoveToQuestGiver(questGiver))
                return false;
        }

        // Select reward if not specified
    if (rewardChoice == 0)
            rewardChoice = SelectBestReward(quest);

        // Turn in the quest - rewardChoice is 0-based index, need to get actual item
        LootItemType rewardType = LootItemType::Item;
        uint32 actualRewardId = 0;

        if (rewardChoice < QUEST_REWARD_CHOICES_COUNT && quest->RewardChoiceItemId[rewardChoice])
        {
            rewardType = quest->RewardChoiceItemType[rewardChoice];
            actualRewardId = quest->RewardChoiceItemId[rewardChoice];
        }

        GetBot()->RewardQuest(quest, rewardType, actualRewardId, questGiver, true);

        // Update progress tracking
    if (auto it = m_questProgress.find(questId); it != m_questProgress.end())
        {
            uint32 timeSpent = GameTime::GetGameTimeMS() - it->second.startTime;
            RecordQuestTime(questId, timeSpent);
            m_questProgress.erase(it);
        }

        // Add to recently completed
        m_recentlyCompleted.push_back(questId);
        if (m_recentlyCompleted.size() > MAX_RECENTLY_COMPLETED)
            m_recentlyCompleted.pop_front();

        // Update statistics
        UpdateStatistics(quest, true);

        TC_LOG_DEBUG("bot.playerbot", "Bot %s turned in quest %u: %s (reward choice: %u)",
            GetBot()->GetName().c_str(), questId, quest->GetLogTitle().c_str(), rewardChoice);

        return true;
    }

    bool QuestManager::AbandonQuest(uint32 questId)
    {
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            return false;

        // Check if we have the quest
    if (GetBot()->GetQuestStatus(questId) == QUEST_STATUS_NONE)
            return false;

        // Abandon the quest
        GetBot()->AbandonQuest(questId);

        // Add to ignored list
        m_ignoredQuests.insert(questId);
        if (m_ignoredQuests.size() > MAX_IGNORED_QUESTS)
        {
            // Remove oldest ignored quest
            m_ignoredQuests.erase(m_ignoredQuests.begin());
        }

        // Clean up tracking
        m_questProgress.erase(questId);

        // Update statistics
        m_stats.questsAbandoned++;

        TC_LOG_DEBUG("bot.playerbot", "Bot %s abandoned quest %u: %s",
            GetBot()->GetName().c_str(), questId, quest->GetLogTitle().c_str());

        return true;
    }

    void QuestManager::ScanForQuests()
    {
        m_questGivers.clear();
        m_availableQuests.clear();
        m_completableQuests.clear();

        ScanCreatureQuestGivers();
        ScanGameObjectQuestGivers();
        ScanItemQuests();

        TC_LOG_DEBUG("bot.playerbot", "Bot %s found %zu quest givers, %zu available quests, %zu completable quests",
            GetBot()->GetName().c_str(), m_questGivers.size(), m_availableQuests.size(), m_completableQuests.size());
    }

    void QuestManager::ScanCreatureQuestGivers()
    {
        // CRITICAL: Check IsInWorld() FIRST to prevent ACCESS_VIOLATION
        // During initialization, bot may not be in world and GetMap() returns invalid pointer
        if (!GetBot() || !GetBot()->IsInWorld())
            return;

        ::std::list<Creature*> creatures;
        Trinity::AnyUnitInObjectRangeCheck checker(GetBot(), QUEST_GIVER_SCAN_RANGE, true, true);
        Trinity::CreatureListSearcher searcher(GetBot(), creatures, checker);
        // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Map* map = GetBot()->GetMap();
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

    // Query nearby GUIDs (lock-free!)
    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        GetBot()->GetPosition(), QUEST_GIVER_SCAN_RANGE);

    // Process results (replace old searcher logic)
    for (ObjectGuid guid : nearbyGuids)
    {
        // PHASE 5D: Thread-safe spatial grid validation
        auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(GetBot(), guid);
        Creature* entity = nullptr;

        if (snapshot)
        {
            // Get Creature* for quest giver detection (validated via snapshot first)
            entity = ObjectAccessor::GetCreature(*GetBot(), guid);
        }

        if (!entity)
            continue;
        // Original filtering logic from searcher goes here
    }
    // End of spatial grid fix
    for (Creature* creature : creatures)
        {
            if (!creature || !creature->IsQuestGiver())
                continue;

            QuestGiverInfo info;
            info.guid = creature->GetGUID();
            info.entry = creature->GetEntry();
            info.distance = GetBot()->GetDistance(creature);
            info.lastCheckTime = GameTime::GetGameTimeMS();

            // Check available quests
    for (uint32 questId : sObjectMgr->GetCreatureQuestRelations(creature->GetEntry()))
            {
                if (CanAcceptQuest(questId))
                {
                    m_availableQuests.push_back(questId);
                    info.availableQuests++;
                }
            }

            // Check completable quests
    for (uint32 questId : sObjectMgr->GetCreatureQuestInvolvedRelations(creature->GetEntry()))
            {
                if (GetBot()->GetQuestStatus(questId) == QUEST_STATUS_COMPLETE)
                {
                    m_completableQuests.push_back(questId);
                    info.completableQuests++;
                }
            }

            if (info.availableQuests > 0 || info.completableQuests > 0)
                m_questGivers.push_back(info);
        }
    }

    void QuestManager::ScanGameObjectQuestGivers()
    {
        // CRITICAL: Check IsInWorld() FIRST to prevent ACCESS_VIOLATION
        // During initialization, bot may not be in world and GetMap() returns invalid pointer
        if (!GetBot() || !GetBot()->IsInWorld())
            return;

        ::std::list<GameObject*> objects;
        Trinity::AllWorldObjectsInRange checker(GetBot(), QUEST_GIVER_SCAN_RANGE);
        Trinity::GameObjectListSearcher searcher(GetBot(), objects, checker);
        // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Map* map = GetBot()->GetMap();
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

    // Query nearby GUIDs (lock-free!)
    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyGameObjectGuids(
        GetBot()->GetPosition(), QUEST_GIVER_SCAN_RANGE);

    // Process results (replace old searcher logic)
    for (ObjectGuid guid : nearbyGuids)
    {
        GameObject* entity = map->GetGameObject(guid);
        if (!entity)
            continue;
        // Original filtering logic from searcher goes here
    }
    // End of spatial grid fix
    for (GameObject* object : objects)
        {
            if (!object)
                continue;

            // Check if GameObject provides quests
            GameObjectTemplate const* goInfo = object->GetGOInfo();
            if (!goInfo || (goInfo->type != GAMEOBJECT_TYPE_QUESTGIVER &&
                             !object->hasQuest(0) && !object->hasInvolvedQuest(0)))
                continue;

            QuestGiverInfo info;
            info.guid = object->GetGUID();
            info.entry = object->GetEntry();
            info.distance = GetBot()->GetDistance(object);
            info.lastCheckTime = GameTime::GetGameTimeMS();

            // Check available quests
    for (uint32 questId : sObjectMgr->GetGOQuestRelations(object->GetEntry()))
            {
                if (CanAcceptQuest(questId))
                {
                    m_availableQuests.push_back(questId);
                    info.availableQuests++;
                }
            }

            // Check completable quests
    for (uint32 questId : sObjectMgr->GetGOQuestInvolvedRelations(object->GetEntry()))
            {
                if (GetBot()->GetQuestStatus(questId) == QUEST_STATUS_COMPLETE)
                {
                    m_completableQuests.push_back(questId);
                    info.completableQuests++;
                }
            }

            if (info.availableQuests > 0 || info.completableQuests > 0)
                m_questGivers.push_back(info);
        }
    }

    void QuestManager::ScanItemQuests()
    {
        // Check inventory for quest starting items
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
        {
            Item* item = GetBot()->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
            if (!item)
                continue;

            ItemTemplate const* proto = item->GetTemplate();
            if (!proto || proto->GetStartQuest() == 0)
                continue;

            uint32 questId = proto->GetStartQuest();
            if (questId && CanAcceptQuest(questId))
            {
                m_availableQuests.push_back(questId);
            }
        }

        // Check bags
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
        {
            Bag* pBag = GetBot()->GetBagByPos(bag);
            if (!pBag)
                continue;

            for (uint32 slot = 0; slot < pBag->GetBagSize(); ++slot)
            {
                Item* item = pBag->GetItemByPos(slot);
                if (!item)
                    continue;

                ItemTemplate const* proto = item->GetTemplate();
                if (!proto || proto->GetStartQuest() == 0)
                    continue;

                uint32 questId = proto->GetStartQuest();
                if (questId && CanAcceptQuest(questId))
                {
                    m_availableQuests.push_back(questId);
                }
            }
        }
    }

    ::std::vector<uint32> QuestManager::GetAvailableQuests() const
    {
        return m_availableQuests;
    }

    ::std::vector<uint32> QuestManager::GetActiveQuests() const
    {
        return m_cache->GetActiveQuests();
    }

    ::std::vector<uint32> QuestManager::GetCompletableQuests() const
    {
        return m_cache->GetCompletableQuests();
    }

    uint32 QuestManager::SelectBestQuest(::std::vector<uint32> const& availableQuests)
    {
        if (availableQuests.empty())
            return 0;

        // Evaluate all available quests
        ::std::vector<QuestPriority> priorities;
        for (uint32 questId : availableQuests)
        {
            Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
            if (!quest)
                continue;

            QuestPriority priority = EvaluateQuestPriority(quest);
            priorities.push_back(priority);
        }

        // Sort by score
        ::std::sort(priorities.begin(), priorities.end(),
            [](QuestPriority const& a, QuestPriority const& b) { return a.score > b.score; });

        // Return best quest
        return priorities.empty() ? 0 : priorities[0].questId;
    }

    float QuestManager::EvaluateQuest(Quest const* quest) const
    {
        if (!quest)
            return 0.0f;

        QuestPriority priority = EvaluateQuestPriority(quest);
        return priority.score;
    }

    QuestManager::QuestPriority QuestManager::EvaluateQuestPriority(Quest const* quest) const
    {
        QuestPriority priority;
        priority.questId = quest->GetQuestId();

        // Calculate component scores
        priority.xpReward = CalculateXPValue(quest);
        priority.goldReward = CalculateGoldValue(quest);
        priority.itemValue = CalculateItemValue(quest);
        priority.reputationValue = CalculateReputationValue(quest);
        priority.distanceScore = CalculateDistanceScore(quest);
        priority.levelScore = CalculateLevelScore(quest);
        priority.groupBonus = CalculateGroupBonus(quest);

        // Check quest type
        priority.isDaily = quest->IsDaily();
        priority.isDungeon = quest->IsRaidQuest(DIFFICULTY_NORMAL);
        priority.isGroupQuest = quest->GetSuggestedPlayers() > 1;

        // Calculate total score with weights
        float score = 0.0f;
        score += priority.xpReward * 2.0f;        // XP is most important
        score += priority.goldReward * 1.5f;      // Gold is important
        score += priority.itemValue * 1.0f;       // Items are useful
        score += priority.reputationValue * 0.5f; // Reputation is nice
        score += priority.distanceScore * 1.0f;   // Prefer nearby quests
        score += priority.levelScore * 1.5f;      // Level appropriate quests
        score += priority.groupBonus * 2.0f;      // Group quests when in group

        // Apply modifiers
    if (priority.isDaily)
            score *= 1.2f;  // Bonus for dailies
    if (priority.isDungeon && !m_acceptDungeonQuests)
            score *= 0.1f;  // Penalty if not accepting dungeon quests
    if (priority.isGroupQuest && m_prioritizeGroupQuests && GetBot()->GetGroup())
            score *= 1.5f;  // Bonus for group quests when grouped

        priority.score = score;
        return priority;
    }

    float QuestManager::CalculateXPValue(Quest const* quest) const
    {
        if (!quest || quest->GetXPDifficulty() == 0)
            return 0.0f;

        // Get base XP reward
        uint32 xp = GetBot()->GetQuestXPReward(quest);
        if (xp == 0)
            return 0.0f;

        // Normalize to 0-100 scale (assuming max XP per quest ~10000)
        return ::std::min(100.0f, xp / 100.0f);
    }

    float QuestManager::CalculateGoldValue(Quest const* quest) const
    {
        if (!quest)
            return 0.0f;

        uint32 gold = GetBot()->GetQuestMoneyReward(quest);

        // Normalize to 0-100 scale (assuming max gold ~100g)
        return ::std::min(100.0f, gold / 10000.0f);
    }

    float QuestManager::CalculateItemValue(Quest const* quest) const
    {
        if (!quest)
            return 0.0f;

        float totalValue = 0.0f;

        // Check reward items
    for (uint8 i = 0; i < QUEST_REWARD_ITEM_COUNT; ++i)
        {
            if (uint32 itemId = quest->RewardItemId[i])
            {
                if (ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId))
                {
                    float itemScore = CalculateItemScore(itemTemplate);
                    totalValue += itemScore * quest->RewardItemCount[i];
                }
            }
        }

        // Check choice items
    for (uint8 i = 0; i < QUEST_REWARD_CHOICES_COUNT; ++i)
        {
            if (uint32 itemId = quest->RewardChoiceItemId[i])
            {
                if (ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId))
                {
                    float itemScore = CalculateItemScore(itemTemplate);
                    totalValue = ::std::max(totalValue, itemScore * quest->RewardChoiceItemCount[i]);
                }
            }
        }

        // Normalize to 0-100 scale
        return ::std::min(100.0f, totalValue);
    }

    float QuestManager::CalculateReputationValue(Quest const* quest) const
    {
        if (!quest)
            return 0.0f;

        float totalRep = 0.0f;

        // Check reputation rewards
    for (uint8 i = 0; i < QUEST_REWARD_REPUTATIONS_COUNT; ++i)
        {
            if (quest->RewardFactionId[i])
            {
                int32 repValue = quest->RewardFactionValue[i];
                if (repValue == 0 && quest->RewardFactionOverride[i])
                    repValue = quest->RewardFactionOverride[i];

                totalRep += ::std::abs(repValue) / 100.0f;
            }
        }

        // Normalize to 0-100 scale
        return ::std::min(100.0f, totalRep);
    }

    float QuestManager::CalculateDistanceScore(Quest const* quest) const
    {
        if (!quest)
            return 0.0f;

        // Find nearest quest giver for this quest
        float minDistance = m_maxTravelDistance;

        for (auto const& giver : m_questGivers)
        {
            minDistance = ::std::min(minDistance, giver.distance);
        }

        // Calculate score (closer = higher score)
        float score = 100.0f * (1.0f - (minDistance / m_maxTravelDistance));
        return ::std::max(0.0f, score);
    }

    float QuestManager::CalculateLevelScore(Quest const* quest) const
    {
        if (!quest)
            return 0.0f;

        int32 botLevel = GetBot()->GetLevel();
        int32 questLevel = GetBot()->GetQuestLevel(quest);
        // There is no GetMinLevel() in Quest API - we use content tuning level scaling

        // Calculate level difference
        int32 levelDiff = questLevel - botLevel;

        // Perfect level match = 100 score
    if (levelDiff == 0)
            return 100.0f;

        // Within good range = high score
    if (levelDiff >= -2 && levelDiff <= 2)
            return 80.0f - (::std::abs(levelDiff) * 10.0f);

        // Outside optimal range
    if (levelDiff < -5)
            return ::std::max(0.0f, 30.0f + levelDiff);  // Too low level
    if (levelDiff > 5)
            return ::std::max(0.0f, 30.0f - levelDiff);  // Too high level

        return 50.0f - (::std::abs(levelDiff) * 5.0f);
    }

    float QuestManager::CalculateGroupBonus(Quest const* quest) const
    {
        if (!quest || !GetBot()->GetGroup())
            return 0.0f;

        // Check if group quest (has suggested players > 1)
    if (quest->GetSuggestedPlayers() > 1)
            return 50.0f;

        // Check if other group members have this quest
        uint32 membersWithQuest = 0;
        Group* group = GetBot()->GetGroup();
        for (GroupReference const& ref : group->GetMembers())
        {
            if (Player* member = ref.GetSource())
            {
                if (member != GetBot() && member->GetQuestStatus(quest->GetQuestId()) != QUEST_STATUS_NONE)
                    membersWithQuest++;
            }
        }

        // More group members with quest = higher bonus
        return membersWithQuest * 20.0f;
    }

    uint32 QuestManager::SelectBestReward(Quest const* quest) const
    {
        if (!quest)
            return 0;

        // Check if there are choice rewards
        bool hasChoices = false;
        for (uint8 i = 0; i < QUEST_REWARD_CHOICES_COUNT; ++i)
        {
            if (quest->RewardChoiceItemId[i])
            {
                hasChoices = true;
                break;
            }
        }

        if (!hasChoices)
            return 0;  // No choices available

        // Evaluate each choice
        uint32 bestChoice = 0;
        float bestScore = 0.0f;

        for (uint8 i = 0; i < QUEST_REWARD_CHOICES_COUNT; ++i)
        {
            if (uint32 itemId = quest->RewardChoiceItemId[i])
            {
                if (ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId))
                {
                    float score = CalculateItemScore(itemTemplate);
                    if (score > bestScore)
                    {
                        bestScore = score;
                        bestChoice = i;
                    }
                }
            }
        }

        return bestChoice;
    }

    float QuestManager::CalculateItemScore(ItemTemplate const* itemTemplate) const
    {
        if (!itemTemplate)
            return 0.0f;

        float score = 0.0f;

        // Base score from quality
    switch (itemTemplate->GetQuality())
        {
            case ITEM_QUALITY_POOR:     score = 1.0f; break;
            case ITEM_QUALITY_NORMAL:   score = 5.0f; break;
            case ITEM_QUALITY_UNCOMMON: score = 20.0f; break;
            case ITEM_QUALITY_RARE:     score = 50.0f; break;
            case ITEM_QUALITY_EPIC:     score = 100.0f; break;
            default: score = 1.0f; break;
        }

        // Check if item is useful for class
    if (IsRewardUseful(itemTemplate))
            score *= 2.0f;

        // Add vendor price component
        uint32 sellPrice = itemTemplate->GetSellPrice();
        if (sellPrice > 0)
            score += sellPrice / 10000.0f;  // Convert to gold

        return score;
    }

    bool QuestManager::IsRewardUseful(ItemTemplate const* itemTemplate) const
    {
        if (!itemTemplate)
            return false;

        // Check if it's equipment
    if (itemTemplate->GetClass() != ITEM_CLASS_WEAPON && itemTemplate->GetClass() != ITEM_CLASS_ARMOR)
            return true;  // Non-equipment is always potentially useful

        // Check class restrictions
        uint32 allowableClass = itemTemplate->GetAllowableClass();
        if (allowableClass && !(allowableClass & GetBot()->GetClassMask()))
            return false;

        // Check if it's an upgrade
        uint8 slot = itemTemplate->GetInventoryType();
        if (slot == INVTYPE_NON_EQUIP)
            return false;

        // Get current item in that slot
        Item* currentItem = GetBot()->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!currentItem)
            return true;  // Empty slot, any item is useful

        // Simple item level comparison
        return itemTemplate->GetBaseItemLevel() > currentItem->GetTemplate()->GetBaseItemLevel();
    }

    QuestStatus QuestManager::GetQuestStatus(uint32 questId) const
    {
        return GetBot()->GetQuestStatus(questId);
    }

    void QuestManager::UpdateQuestProgress()
    {
        for (auto& [questId, progress] : m_questProgress)
        {
            Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
            if (!quest)
                continue;

            UpdateObjectiveProgress(questId);

            // Check if quest is complete
    if (GetBot()->GetQuestStatus(questId) == QUEST_STATUS_COMPLETE)
            {
                progress.isComplete = true;
                progress.completionPercent = 100.0f;
            }
        }
    }

    void QuestManager::UpdateObjectiveProgress(uint32 questId)
    {
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            return;

        auto it = m_questProgress.find(questId);
        if (it == m_questProgress.end())
            return;

        QuestProgress& progress = it->second;
        float totalProgress = 0.0f;
        uint32 totalObjectives = 0;

        // Check each objective using the new API
        QuestObjectives const& objectives = quest->GetObjectives();
        uint8 objIndex = 0;
        for (QuestObjective const& obj : objectives)
        {
            if (obj.IsStoringValue())
            {
                totalObjectives++;
                uint16 currentCount = GetBot()->GetQuestSlotCounter(GetBot()->FindQuestSlot(questId), obj.StorageIndex);
                int32 requiredCount = obj.Amount;

                if (requiredCount > 0)
                {
                    float objProgress = (float)currentCount / (float)requiredCount;
                    totalProgress += ::std::min(1.0f, objProgress);
                    if (objIndex < progress.objectiveProgress.size())
                        progress.objectiveProgress[objIndex] = currentCount;
                }
            }
            objIndex++;
        }

        if (totalObjectives > 0)
            progress.completionPercent = (totalProgress / totalObjectives) * 100.0f;

        progress.lastUpdateTime = GameTime::GetGameTimeMS();
    }

    bool QuestManager::IsQuestComplete(uint32 questId) const
    {
        return GetBot()->GetQuestStatus(questId) == QUEST_STATUS_COMPLETE;
    }

    float QuestManager::GetQuestCompletionPercent(uint32 questId) const
    {
        auto it = m_questProgress.find(questId);
        if (it != m_questProgress.end())
            return it->second.completionPercent;

        return 0.0f;
    }

    bool QuestManager::IsQuestGiverNearby() const
    {
        return !m_questGivers.empty();
    }

    Creature* QuestManager::FindNearestQuestGiver() const
    {
        float minDistance = ::std::numeric_limits<float>::max();
        Creature* nearest = nullptr;

        for (auto const& giver : m_questGivers)
        {
            // PHASE 5D: Thread-safe spatial grid validation
            auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(GetBot(), giver.guid);
            WorldObject* obj = nullptr;

            if (snapshot)
            {
                // Get WorldObject* for nearest quest giver (validated via snapshot first)
                obj = ObjectAccessor::GetWorldObject(*GetBot(), giver.guid);
            }

            if (!obj)
                continue;

            if (Creature* creature = obj->ToCreature())
            {
                if (giver.distance < minDistance)
                {
                    minDistance = giver.distance;
                    nearest = creature;
                }
            }
        }

        return nearest;
    }

    GameObject* QuestManager::FindNearestQuestObject() const
    {
        float minDistance = ::std::numeric_limits<float>::max();
        GameObject* nearest = nullptr;

        for (auto const& giver : m_questGivers)
        {
            // PHASE 5D: Thread-safe spatial grid validation
            auto snapshot = SpatialGridQueryHelpers::FindGameObjectByGuid(GetBot(), giver.guid);
            WorldObject* obj = nullptr;

            if (snapshot)
            {
                // Get WorldObject* for nearest quest object (validated via snapshot first)
                obj = ObjectAccessor::GetWorldObject(*GetBot(), giver.guid);
            }

            if (!obj)
                continue;

            if (GameObject* gameObject = obj->ToGameObject())
            {
                if (giver.distance < minDistance)
                {
                    minDistance = giver.distance;
                    nearest = gameObject;
                }
            }
        }

        return nearest;
    }

    bool QuestManager::MoveToQuestGiver(WorldObject* questGiver)
    {
        if (!questGiver)
            return false;

        float distance = GetBot()->GetDistance(questGiver);
        if (distance <= QUEST_INTERACT_DISTANCE)
            return true;

        // Move to quest giver using centralized movement utility (prevents infinite loop)
        Position destination;
        destination.Relocate(questGiver->GetPositionX(), questGiver->GetPositionY(), questGiver->GetPositionZ());
        return BotMovementUtil::MoveToPosition(GetBot(), destination);
    }

    void QuestManager::ShareGroupQuests()
    {
        if (!GetBot()->GetGroup())
            return;

        // Share all shareable quests with group
    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SLOT; ++slot)
        {
            uint32 questId = GetBot()->GetQuestSlotQuestId(slot);
            if (questId == 0)
                continue;

            Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
            if (!quest || !quest->HasFlag(QUEST_FLAGS_SHARABLE))
                continue;

            // Share quest with group members
    if (WorldSession* session = GetBot()->GetSession())
            {
                // Quest sharing is handled internally by the server when appropriate
                // For bots, we skip the packet-based approach
                TC_LOG_DEBUG("bot.playerbot", "Bot %s wants to share quest %u with group",
                    GetBot()->GetName().c_str(), questId);
            }
        }
    }

    bool QuestManager::AcceptSharedQuest(uint32 questId)
    {
        if (!CanAcceptQuest(questId))
            return false;

        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            return false;

        // Accept shared quest
        GetBot()->AddQuest(quest, nullptr);

        TC_LOG_DEBUG("bot.playerbot", "Bot %s accepted shared quest %u: %s",
            GetBot()->GetName().c_str(), questId, quest->GetLogTitle().c_str());

        return true;
    }

    bool QuestManager::HasActiveQuest(uint32 questId) const
    {
        QuestStatus status = GetBot()->GetQuestStatus(questId);
        return status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE;
    }

    void QuestManager::UpdateQuestPhase(uint32 diff)
    {
        switch (m_currentPhase)
        {
            case QuestPhase::IDLE:
                ProcessIdlePhase();
                break;
            case QuestPhase::SCANNING:
                ProcessScanningPhase();
                break;
            case QuestPhase::ACCEPTING:
                ProcessAcceptingPhase();
                break;
            case QuestPhase::PROGRESSING:
                ProcessProgressingPhase();
                break;
            case QuestPhase::COMPLETING:
                ProcessCompletingPhase();
                break;
            case QuestPhase::MANAGING:
                ProcessManagingPhase();
                break;
        }
    }

    void QuestManager::ProcessIdlePhase()
    {
        // Check if we have completable quests
    if (!m_completableQuests.empty())
        {
            m_currentPhase = QuestPhase::COMPLETING;
            m_phaseTimer = 0;
            return;
        }

        // Check if we need to accept new quests
    if (GetQuestLogSpace() > 5 && !m_availableQuests.empty())
        {
            m_currentPhase = QuestPhase::ACCEPTING;
            m_phaseTimer = 0;
            return;
        }

        // Check if we need to manage quest log
    if (GetQuestLogSpace() < 3)
        {
            m_currentPhase = QuestPhase::MANAGING;
            m_phaseTimer = 0;
            return;
        }

        // Continue working on active quests
    if (!m_cache->GetActiveQuests().empty())
        {
            m_currentPhase = QuestPhase::PROGRESSING;
            m_phaseTimer = 0;
        }
    }

    void QuestManager::ProcessScanningPhase()
    {
        ScanForQuests();
        m_currentPhase = QuestPhase::IDLE;
        m_phaseTimer = 0;
    }

    void QuestManager::ProcessAcceptingPhase()
    {
        AcceptBestQuests();
        m_currentPhase = QuestPhase::IDLE;
        m_phaseTimer = 0;
    }

    void QuestManager::ProcessProgressingPhase()
    {
        // This phase is handled by other AI systems (combat, movement, etc.)
        // Just update progress tracking
        UpdateQuestProgress();
        m_currentPhase = QuestPhase::IDLE;
        m_phaseTimer = 0;
    }

    void QuestManager::ProcessCompletingPhase()
    {
        TurnInCompletedQuests();
        m_currentPhase = QuestPhase::IDLE;
        m_phaseTimer = 0;
    }

    void QuestManager::ProcessManagingPhase()
    {
        ManageQuestLog();
        m_currentPhase = QuestPhase::IDLE;
        m_phaseTimer = 0;
    }

    void QuestManager::AcceptBestQuests()
    {
        if (m_availableQuests.empty() || !m_autoAccept)
            return;

        // Sort available quests by priority
        ::std::sort(m_availableQuests.begin(), m_availableQuests.end(),
            [this](uint32 a, uint32 b)
            {
                Quest const* questA = sObjectMgr->GetQuestTemplate(a);
                Quest const* questB = sObjectMgr->GetQuestTemplate(b);
                return EvaluateQuest(questA) > EvaluateQuest(questB);
            });

        // Accept quests up to log limit
        uint32 space = GetQuestLogSpace();
        uint32 accepted = 0;

        for (uint32 questId : m_availableQuests)
        {
            if (space == 0)
                break;

            if (AcceptQuest(questId))
            {
                space--;
                accepted++;
            }
        }

        TC_LOG_DEBUG("bot.playerbot", "Bot %s accepted %u new quests", GetBot()->GetName().c_str(), accepted);
    }

    void QuestManager::TurnInCompletedQuests()
    {
        if (m_completableQuests.empty() || !m_autoComplete)
            return;

        uint32 completed = 0;

        for (uint32 questId : m_completableQuests)
        {
            if (TurnInQuest(questId))
                completed++;
        }

        TC_LOG_DEBUG("bot.playerbot", "Bot %s turned in %u completed quests", GetBot()->GetName().c_str(), completed);
    }

    void QuestManager::AbandonLowPriorityQuests()
    {
        if (GetQuestLogSpace() >= 5)
            return;  // Enough space

        // Build list of active quests with priorities
        ::std::vector<QuestPriority> activeQuests;

        for (auto const& questId : m_cache->GetActiveQuests())
        {
            Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
            if (!quest)
                continue;

            // Don't abandon nearly complete quests
    if (GetQuestCompletionPercent(questId) > 80.0f)
                continue;

            QuestPriority priority = EvaluateQuestPriority(quest);
            activeQuests.push_back(priority);
        }

        // Sort by priority (lowest first)
        ::std::sort(activeQuests.begin(), activeQuests.end(),
            [](QuestPriority const& a, QuestPriority const& b) { return a.score < b.score; });

        // Abandon lowest priority quests
        uint32 toAbandon = ::std::min(5u, (uint32)activeQuests.size());
        uint32 abandoned = 0;

        for (uint32 i = 0; i < toAbandon && GetQuestLogSpace() < 5; ++i)
        {
            if (AbandonQuest(activeQuests[i].questId))
                abandoned++;
        }

        if (abandoned > 0)
            TC_LOG_DEBUG("bot.playerbot", "Bot %s abandoned %u low priority quests", GetBot()->GetName().c_str(), abandoned);
    }

    void QuestManager::ManageQuestLog()
    {
        // Remove completed quests first
        TurnInCompletedQuests();

        // Abandon low priority quests if needed
        AbandonLowPriorityQuests();

        // Accept new quests if space available
    if (GetQuestLogSpace() > 3)
            AcceptBestQuests();
    }

    bool QuestManager::ShouldAbandonQuest(uint32 questId) const
    {
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            return false;

        // Never abandon nearly complete quests
    if (GetQuestCompletionPercent(questId) > 75.0f)
            return false;

        // Check if quest has been attempted too many times
        auto it = m_questProgress.find(questId);
        if (it != m_questProgress.end() && it->second.attemptCount > 3)
            return true;

        // Check if quest is too low level
    if (IsQuestTooLowLevel(quest))
            return true;

        // Check if quest has taken too long
    if (it != m_questProgress.end())
        {
            uint32 timeSpent = GameTime::GetGameTimeMS() - it->second.startTime;
            if (timeSpent > 30 * MINUTE * IN_MILLISECONDS)  // 30 minutes
                return true;
        }

        return false;
    }

    bool QuestManager::IsQuestTooLowLevel(Quest const* quest) const
    {
        if (!quest)
            return false;

        int32 levelDiff = GetBot()->GetLevel() - GetBot()->GetQuestLevel(quest);
        return levelDiff > 10;  // More than 10 levels below
    }

    bool QuestManager::IsQuestTooHighLevel(Quest const* quest) const
    {
        if (!quest)
            return false;

        // Check content tuning min level if available
        int32 questLevel = GetBot()->GetQuestLevel(quest);
        return questLevel > GetBot()->GetLevel() + 3;
    }

    bool QuestManager::HasQuestPrerequisites(Quest const* quest) const
    {
        if (!quest)
            return false;

        // Check previous quest requirements
    if (quest->GetPrevQuestId())
        {
            if (GetBot()->GetQuestRewardStatus(::std::abs(quest->GetPrevQuestId())) != (quest->GetPrevQuestId() > 0))
                return false;
        }

        // Check next quest in chain
    if (quest->GetNextQuestId())
        {
            if (GetBot()->GetQuestStatus(::std::abs(static_cast<int32>(quest->GetNextQuestId()))) != QUEST_STATUS_NONE)
                return false;
        }

        return true;
    }

    bool QuestManager::MeetsQuestRequirements(Quest const* quest) const
    {
        if (!quest)
            return false;

        // Check race/class requirements
    if (!quest->GetAllowableRaces().IsEmpty() && !quest->GetAllowableRaces().HasRace(GetBot()->GetRace()))
            return false;

        if (quest->GetAllowableClasses() && !(quest->GetAllowableClasses() & GetBot()->GetClassMask()))
            return false;

        // Check skill requirements
    if (quest->GetRequiredSkill())
        {
            if (!GetBot()->HasSkill(quest->GetRequiredSkill()))
                return false;

            if (GetBot()->GetSkillValue(quest->GetRequiredSkill()) < quest->GetRequiredSkillValue())
                return false;
        }

        // Check minimum reputation requirements
    if (quest->GetRequiredMinRepFaction())
        {
            if (GetBot()->GetReputationMgr().GetReputation(quest->GetRequiredMinRepFaction()) < quest->GetRequiredMinRepValue())
                return false;
        }

        // Check maximum reputation requirements
    if (quest->GetRequiredMaxRepFaction())
        {
            if (GetBot()->GetReputationMgr().GetReputation(quest->GetRequiredMaxRepFaction()) > quest->GetRequiredMaxRepValue())
                return false;
        }

        return true;
    }

    uint32 QuestManager::GetQuestLogSpace() const
    {
        uint32 usedSlots = 0;
        for (uint8 slot = 0; slot < MAX_QUEST_LOG_SLOT; ++slot)
        {
            if (GetBot()->GetQuestSlotQuestId(slot) != 0)
                usedSlots++;
        }
        return MAX_QUEST_LOG_SLOT - usedSlots;
    }

    void QuestManager::RecordQuestTime(uint32 questId, uint32 timeSpent)
    {
        // Update average quest completion time
    if (m_stats.questsCompleted > 0)
        {
            m_stats.avgTimePerQuest = (m_stats.avgTimePerQuest * m_stats.questsCompleted + timeSpent) /
                                      (m_stats.questsCompleted + 1);
        }
        else
        {
            m_stats.avgTimePerQuest = (float)timeSpent;
        }
    }

    void QuestManager::UpdateStatistics(Quest const* quest, bool completed)
    {
        if (!quest)
            return;

        if (completed)
        {
            m_stats.questsCompleted++;

            // Update XP earned
            m_stats.totalXPEarned += GetBot()->GetQuestXPReward(quest);

            // Update gold earned
            m_stats.totalGoldEarned += GetBot()->GetQuestMoneyReward(quest);

            // Update quest type counters
    if (quest->IsDaily())
                m_stats.dailyQuestsCompleted++;
            if (quest->IsRaidQuest(DIFFICULTY_NORMAL))
                m_stats.dungeonQuestsCompleted++;
        }
        else
        {
            m_stats.questsFailed++;
        }
    }

    void QuestManager::UpdateQuestCache()
    {
        m_cache->Update(GetBot());
    }

    void QuestManager::InvalidateCache()
    {
        m_cache->Invalidate();
    }

    void QuestManager::ClearQuestGiverCache()
    {
        m_questGivers.clear();
        m_lastQuestGiverScan = 0;
    }

    // Performance tracking is handled by BehaviorManager base class
    // These methods are no longer needed as BehaviorManager provides:
    // - Automatic performance monitoring in Update()
    // - GetLastUpdateDuration() for timing information
    // - Built-in slow update detection and logging

    size_t QuestManager::GetMemoryUsage() const
    {
        size_t memory = sizeof(*this);
        memory += m_questProgress.size() * sizeof(QuestProgress);
        memory += m_ignoredQuests.size() * sizeof(uint32);
        memory += m_questGivers.size() * sizeof(QuestGiverInfo);
        memory += m_questPriorities.size() * sizeof(QuestPriority);
        memory += m_availableQuests.size() * sizeof(uint32);
        memory += m_completableQuests.size() * sizeof(uint32);
        return memory;
    }

} // namespace Playerbot