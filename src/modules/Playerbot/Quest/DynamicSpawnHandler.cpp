/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "DynamicSpawnHandler.h"
#include "Player.h"
#include "Creature.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "DatabaseEnv.h"
#include "DB2Stores.h"
#include "Log.h"
#include "ScriptMgr.h"
#include "Conditions/ConditionMgr.h"
#include "Map.h"
#include "QuestDef.h"
#include "GameTime.h"
#include "RestMgr.h"

namespace Playerbot
{

// ============================================================================
// STATIC MEMBER DEFINITIONS
// ============================================================================
std::unordered_map<uint32, AreaTriggerData> DynamicSpawnHandler::_areaTriggerCache;
bool DynamicSpawnHandler::_areaTriggerCacheInitialized = false;
std::unordered_map<uint32, bool> DynamicSpawnHandler::_creatureSpawnCache;
std::unordered_map<int64, uint32> DynamicSpawnHandler::_smartScriptSummonCache;

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

DynamicSpawnHandler::DynamicSpawnHandler(Player* bot)
    : _bot(bot)
{
    if (!bot)
    {
        TC_LOG_ERROR("module.playerbot.quest", "DynamicSpawnHandler: null bot!");
        return;
    }

    // Initialize static caches on first instance
    if (!_areaTriggerCacheInitialized)
    {
        InitializeStaticCaches();
    }
}

// ============================================================================
// CORE UPDATE LOOP
// ============================================================================

void DynamicSpawnHandler::Update(uint32 diff)
{
    if (!_bot || !_bot->IsInWorld())
        return;

    uint32 currentTime = GameTime::GetGameTimeMS();

    // Throttle updates
    if (currentTime - _lastUpdateTime < UPDATE_INTERVAL_MS)
        return;

    _lastUpdateTime = currentTime;

    // Process nearby area triggers
    ProcessNearbyAreaTriggers();

    // Cleanup expired spawns periodically
    CleanupExpiredSpawns();
}

// ============================================================================
// SPAWN REQUIREMENT DETECTION
// ============================================================================

bool DynamicSpawnHandler::RequiresDynamicSpawn(uint32 creatureEntry) const
{
    // Check cache first
    auto it = _creatureSpawnCache.find(creatureEntry);
    if (it != _creatureSpawnCache.end())
        return it->second;

    // Check if creature has static spawns
    bool hasStatic = HasStaticSpawn(creatureEntry);
    _creatureSpawnCache[creatureEntry] = !hasStatic;

    if (!hasStatic)
    {
        TC_LOG_DEBUG("module.playerbot.quest", "DynamicSpawnHandler: Creature {} requires dynamic spawn (no static spawns)",
                    creatureEntry);
    }

    return !hasStatic;
}

std::optional<DynamicSpawnInfo> DynamicSpawnHandler::GetSpawnInfoForObjective(uint32 questId, uint8 objectiveIndex)
{
    // Check cache
    auto questIt = _questSpawnReqs.find(questId);
    if (questIt != _questSpawnReqs.end())
    {
        for (auto& info : questIt->second)
        {
            if (info.objectiveIndex == objectiveIndex)
                return info;
        }
    }

    // Analyze quest if not cached
    auto reqs = AnalyzeQuestSpawnRequirements(questId);
    _questSpawnReqs[questId] = reqs;

    for (auto& info : reqs)
    {
        if (info.objectiveIndex == objectiveIndex)
            return info;
    }

    return std::nullopt;
}

std::vector<DynamicSpawnInfo> DynamicSpawnHandler::AnalyzeQuestSpawnRequirements(uint32 questId)
{
    std::vector<DynamicSpawnInfo> results;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return results;

    uint8 objIndex = 0;
    for (QuestObjective const& obj : quest->GetObjectives())
    {
        // Only check monster objectives (kill/interact)
        if (obj.Type == QUEST_OBJECTIVE_MONSTER && obj.ObjectID > 0)
        {
            uint32 creatureEntry = static_cast<uint32>(obj.ObjectID);

            // Check if this creature requires dynamic spawning
            if (RequiresDynamicSpawn(creatureEntry))
            {
                // Try to find the spawn mechanism
                auto spawnInfo = QuerySmartScriptsForSpawn(creatureEntry);
                if (spawnInfo)
                {
                    spawnInfo->questId = questId;
                    spawnInfo->objectiveIndex = objIndex;
                    results.push_back(*spawnInfo);

                    TC_LOG_DEBUG("module.playerbot.quest",
                                "DynamicSpawnHandler: Quest {} obj {} creature {} - found spawn trigger type {}",
                                questId, objIndex, creatureEntry, static_cast<uint8>(spawnInfo->triggerType));
                }
                else
                {
                    // Check if any area triggers are associated with this quest
                    auto areaTriggers = QueryAreaTriggersForQuest(questId);
                    for (uint32 triggerId : areaTriggers)
                    {
                        uint32 summonCreature = GetAreaTriggerSummonCreature(triggerId);
                        if (summonCreature == creatureEntry || summonCreature == 0)
                        {
                            DynamicSpawnInfo info;
                            info.questId = questId;
                            info.objectiveIndex = objIndex;
                            info.creatureEntry = creatureEntry;
                            info.triggerType = SpawnTriggerType::AREA_TRIGGER;
                            info.areaTriggerDBC = triggerId;

                            // Get trigger position from DBC
                            if (AreaTriggerEntry const* at = sAreaTriggerStore.LookupEntry(triggerId))
                            {
                                info.areaTriggerPos = Position(at->Pos.X, at->Pos.Y, at->Pos.Z);
                                info.areaTriggerRadius = at->Radius;
                            }

                            results.push_back(info);
                            TC_LOG_DEBUG("module.playerbot.quest",
                                        "DynamicSpawnHandler: Quest {} obj {} creature {} - found area trigger {}",
                                        questId, objIndex, creatureEntry, triggerId);
                            break;
                        }
                    }
                }
            }
        }
        ++objIndex;
    }

    return results;
}

// ============================================================================
// SPAWN TRIGGERING
// ============================================================================

bool DynamicSpawnHandler::TriggerSpawn(DynamicSpawnInfo& spawnInfo)
{
    if (!_bot || !_bot->IsInWorld())
        return false;

    // Check cooldown
    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime - spawnInfo.lastAttemptTime < 5000)  // 5 second cooldown between attempts
        return false;

    spawnInfo.lastAttemptTime = currentTime;
    spawnInfo.attemptCount++;

    switch (spawnInfo.triggerType)
    {
        case SpawnTriggerType::AREA_TRIGGER:
            return TriggerAreaTrigger(spawnInfo.areaTriggerDBC);

        case SpawnTriggerType::QUEST_ACCEPT:
            // Quest accept triggers happen automatically when quest is accepted
            // Nothing to do here - the OnQuestAccept hook handles this
            return true;

        case SpawnTriggerType::GOSSIP_SELECT:
            // Would need to interact with NPC and select gossip option
            // This is complex and would require bot gossip handling
            TC_LOG_DEBUG("module.playerbot.quest",
                        "DynamicSpawnHandler: GOSSIP_SELECT spawn trigger not yet implemented for creature {}",
                        spawnInfo.creatureEntry);
            return false;

        case SpawnTriggerType::PHASE_SHIFT:
            // Phase-based spawns require the bot to meet phase conditions
            // The bot should already be in the correct phase if they have the quest
            TC_LOG_DEBUG("module.playerbot.quest",
                        "DynamicSpawnHandler: PHASE_SHIFT spawn - creature {} should be visible if phase conditions met",
                        spawnInfo.creatureEntry);
            return true;

        default:
            TC_LOG_DEBUG("module.playerbot.quest",
                        "DynamicSpawnHandler: Unsupported spawn trigger type {} for creature {}",
                        static_cast<uint8>(spawnInfo.triggerType), spawnInfo.creatureEntry);
            return false;
    }
}

bool DynamicSpawnHandler::ProcessNearbyAreaTriggers()
{
    if (!_bot || !_bot->IsInWorld())
        return false;

    bool triggered = false;
    uint32 currentTime = GameTime::GetGameTimeMS();

    // Iterate through all area triggers in DBC and check if bot is inside
    for (uint32 i = 1; i < sAreaTriggerStore.GetNumRows(); ++i)
    {
        AreaTriggerEntry const* at = sAreaTriggerStore.LookupEntry(i);
        if (!at)
            continue;

        // Skip if we triggered this recently
        auto cooldownIt = _triggeredAreaTriggers.find(i);
        if (cooldownIt != _triggeredAreaTriggers.end())
        {
            if (currentTime - cooldownIt->second < AREATRIGGER_COOLDOWN_MS)
                continue;
        }

        // Check if bot is in this area trigger
        if (_bot->IsInAreaTrigger(at))
        {
            // Check conditions
            if (!sConditionMgr->IsObjectMeetingNotGroupedConditions(CONDITION_SOURCE_TYPE_AREATRIGGER_CLIENT_TRIGGERED, at->ID, _bot))
                continue;

            TC_LOG_DEBUG("module.playerbot.quest",
                        "DynamicSpawnHandler: Bot {} entered area trigger {}",
                        _bot->GetName(), at->ID);

            // Execute the trigger
            ExecuteAreaTrigger(at, true);
            _triggeredAreaTriggers[i] = currentTime;
            triggered = true;
        }
    }

    return triggered;
}

bool DynamicSpawnHandler::TriggerAreaTrigger(uint32 triggerId)
{
    if (!_bot || !_bot->IsInWorld())
        return false;

    AreaTriggerEntry const* at = sAreaTriggerStore.LookupEntry(triggerId);
    if (!at)
    {
        TC_LOG_WARN("module.playerbot.quest",
                   "DynamicSpawnHandler: Area trigger {} not found in DBC", triggerId);
        return false;
    }

    // Check if bot is actually in the trigger area
    if (!_bot->IsInAreaTrigger(at))
    {
        TC_LOG_DEBUG("module.playerbot.quest",
                    "DynamicSpawnHandler: Bot {} not in area trigger {} - needs to move there first",
                    _bot->GetName(), triggerId);
        return false;
    }

    // Check conditions
    if (!sConditionMgr->IsObjectMeetingNotGroupedConditions(CONDITION_SOURCE_TYPE_AREATRIGGER_CLIENT_TRIGGERED, at->ID, _bot))
    {
        TC_LOG_DEBUG("module.playerbot.quest",
                    "DynamicSpawnHandler: Bot {} doesn't meet conditions for area trigger {}",
                    _bot->GetName(), triggerId);
        return false;
    }

    ExecuteAreaTrigger(at, true);
    return true;
}

// ============================================================================
// AREA TRIGGER DETECTION
// ============================================================================

std::vector<uint32> DynamicSpawnHandler::GetNearbyAreaTriggers(float radius) const
{
    std::vector<uint32> results;

    if (!_bot || !_bot->IsInWorld())
        return results;

    Position botPos = _bot->GetPosition();
    uint32 botMapId = _bot->GetMapId();

    // Check all area triggers in DBC
    for (uint32 i = 1; i < sAreaTriggerStore.GetNumRows(); ++i)
    {
        AreaTriggerEntry const* at = sAreaTriggerStore.LookupEntry(i);
        if (!at)
            continue;

        // Skip if on different map
        if (at->ContinentID != botMapId)
            continue;

        // Check distance
        float dist = botPos.GetExactDist(Position(at->Pos.X, at->Pos.Y, at->Pos.Z));
        if (dist <= radius + at->Radius)
        {
            results.push_back(i);
        }
    }

    return results;
}

bool DynamicSpawnHandler::IsInAreaTrigger(uint32 triggerId) const
{
    if (!_bot)
        return false;

    AreaTriggerEntry const* at = sAreaTriggerStore.LookupEntry(triggerId);
    if (!at)
        return false;

    return _bot->IsInAreaTrigger(at);
}

Position DynamicSpawnHandler::GetAreaTriggerPosition(uint32 triggerId) const
{
    AreaTriggerEntry const* at = sAreaTriggerStore.LookupEntry(triggerId);
    if (at)
        return Position(at->Pos.X, at->Pos.Y, at->Pos.Z);

    if (_bot)
        return _bot->GetPosition();

    return Position();
}

// ============================================================================
// SPAWNED CREATURE TRACKING
// ============================================================================

bool DynamicSpawnHandler::IsSpawnedCreature(ObjectGuid guid) const
{
    return _spawnedCreatures.find(guid) != _spawnedCreatures.end();
}

void DynamicSpawnHandler::TrackSpawnedCreature(ObjectGuid guid, uint32 entry, uint32 questId)
{
    SpawnedCreatureInfo info;
    info.entry = entry;
    info.questId = questId;
    info.spawnTime = GameTime::GetGameTimeMS();
    _spawnedCreatures[guid] = info;

    TC_LOG_DEBUG("module.playerbot.quest",
                "DynamicSpawnHandler: Tracking spawned creature {} entry {} for quest {}",
                guid.ToString(), entry, questId);
}

std::vector<ObjectGuid> DynamicSpawnHandler::GetSpawnedCreaturesForQuest(uint32 questId) const
{
    std::vector<ObjectGuid> results;
    for (auto const& pair : _spawnedCreatures)
    {
        if (pair.second.questId == questId)
            results.push_back(pair.first);
    }
    return results;
}

// ============================================================================
// CACHING AND PERFORMANCE
// ============================================================================

void DynamicSpawnHandler::PreloadQuestSpawnData()
{
    if (!_bot)
        return;

    // Iterate through bot's active quests
    for (uint16 i = 0; i < MAX_QUEST_LOG_SIZE; ++i)
    {
        uint32 questId = _bot->GetQuestSlotQuestId(i);
        if (questId == 0)
            continue;

        // Analyze and cache
        if (_questSpawnReqs.find(questId) == _questSpawnReqs.end())
        {
            auto reqs = AnalyzeQuestSpawnRequirements(questId);
            if (!reqs.empty())
            {
                _questSpawnReqs[questId] = reqs;
                TC_LOG_DEBUG("module.playerbot.quest",
                            "DynamicSpawnHandler: Preloaded {} dynamic spawn reqs for quest {}",
                            reqs.size(), questId);
            }
        }
    }
}

void DynamicSpawnHandler::ClearQuestCache(uint32 questId)
{
    _questSpawnReqs.erase(questId);

    // Remove spawned creatures for this quest
    for (auto it = _spawnedCreatures.begin(); it != _spawnedCreatures.end(); )
    {
        if (it->second.questId == questId)
            it = _spawnedCreatures.erase(it);
        else
            ++it;
    }
}

void DynamicSpawnHandler::ClearAllCaches()
{
    _questSpawnReqs.clear();
    _triggeredAreaTriggers.clear();
    _spawnedCreatures.clear();
}

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

void DynamicSpawnHandler::InitializeStaticCaches()
{
    TC_LOG_INFO("module.playerbot.quest", "DynamicSpawnHandler: Initializing static caches...");

    // Load area trigger data from DBC
    uint32 atCount = 0;
    for (uint32 i = 1; i < sAreaTriggerStore.GetNumRows(); ++i)
    {
        AreaTriggerEntry const* at = sAreaTriggerStore.LookupEntry(i);
        if (!at)
            continue;

        AreaTriggerData data;
        data.triggerId = at->ID;
        data.mapId = at->ContinentID;
        data.position = Position(at->Pos.X, at->Pos.Y, at->Pos.Z);
        data.radius = at->Radius;

        // Check for associated quests
        if (std::unordered_set<uint32> const* quests = sObjectMgr->GetQuestsForAreaTrigger(at->ID))
        {
            for (uint32 questId : *quests)
                data.questIds.push_back(questId);
        }

        _areaTriggerCache[at->ID] = data;
        ++atCount;
    }

    // Cache SmartScript summon data
    // Query: Find all SMART_ACTION_SUMMON_CREATURE (12) triggered by SMART_EVENT_AREATRIGGER_ENTER (46)
    QueryResult result = WorldDatabase.Query(
        "SELECT entryorguid, source_type, action_param1 FROM smart_scripts "
        "WHERE event_type = 46 AND action_type = 12");  // AREATRIGGER_ENTER + SUMMON_CREATURE

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            int64 entry = fields[0].GetInt64();
            uint32 creatureEntry = fields[2].GetUInt32();

            _smartScriptSummonCache[entry] = creatureEntry;
        } while (result->NextRow());
    }

    TC_LOG_INFO("module.playerbot.quest",
                "DynamicSpawnHandler: Cached {} area triggers, {} SmartScript summons",
                atCount, _smartScriptSummonCache.size());

    _areaTriggerCacheInitialized = true;
}

std::optional<DynamicSpawnInfo> DynamicSpawnHandler::QuerySmartScriptsForSpawn(uint32 creatureEntry)
{
    // Query smart_scripts for any script that summons this creature
    // Check SMART_EVENT_QUEST_ACCEPTED (47) and SMART_EVENT_AREATRIGGER_ENTER (46)

    QueryResult result = WorldDatabase.PQuery(
        "SELECT entryorguid, source_type, event_type FROM smart_scripts "
        "WHERE action_type = 12 AND action_param1 = {} "  // SMART_ACTION_SUMMON_CREATURE
        "ORDER BY source_type, entryorguid",
        creatureEntry);

    if (!result)
        return std::nullopt;

    do
    {
        Field* fields = result->Fetch();
        int64 entry = fields[0].GetInt64();
        uint32 sourceType = fields[1].GetUInt32();
        uint32 eventType = fields[2].GetUInt32();

        DynamicSpawnInfo info;
        info.creatureEntry = creatureEntry;
        info.smartScriptEntry = static_cast<uint32>(std::abs(entry));
        info.smartScriptSource = sourceType;

        // Determine trigger type based on event
        switch (eventType)
        {
            case 46:  // SMART_EVENT_AREATRIGGER_ENTER
                info.triggerType = SpawnTriggerType::AREA_TRIGGER;
                // For areatrigger scripts, entryorguid IS the areatrigger ID
                if (sourceType == 2)  // SMART_SCRIPT_TYPE_AREATRIGGER
                    info.areaTriggerDBC = static_cast<uint32>(entry);
                break;

            case 47:  // SMART_EVENT_QUEST_ACCEPTED
                info.triggerType = SpawnTriggerType::QUEST_ACCEPT;
                break;

            case 62:  // SMART_EVENT_GOSSIP_HELLO
            case 63:  // SMART_EVENT_GOSSIP_SELECT
                info.triggerType = SpawnTriggerType::GOSSIP_SELECT;
                info.gossipNpcEntry = static_cast<uint32>(std::abs(entry));
                break;

            default:
                info.triggerType = SpawnTriggerType::CREATURE_AI;
                break;
        }

        // Return first match
        return info;

    } while (result->NextRow());

    return std::nullopt;
}

std::vector<uint32> DynamicSpawnHandler::QueryAreaTriggersForQuest(uint32 questId) const
{
    std::vector<uint32> results;

    // Check cached area triggers
    for (auto const& pair : _areaTriggerCache)
    {
        for (uint32 qId : pair.second.questIds)
        {
            if (qId == questId)
            {
                results.push_back(pair.first);
                break;
            }
        }
    }

    return results;
}

bool DynamicSpawnHandler::HasStaticSpawn(uint32 creatureEntry) const
{
    // Query creature table for any spawns of this creature
    QueryResult result = WorldDatabase.PQuery(
        "SELECT guid FROM creature WHERE id = {} LIMIT 1", creatureEntry);

    return result != nullptr;
}

uint32 DynamicSpawnHandler::GetAreaTriggerSummonCreature(uint32 triggerId) const
{
    // Check SmartScript cache
    auto it = _smartScriptSummonCache.find(static_cast<int64>(triggerId));
    if (it != _smartScriptSummonCache.end())
        return it->second;

    return 0;
}

void DynamicSpawnHandler::ExecuteAreaTrigger(AreaTriggerEntry const* atEntry, bool entered)
{
    if (!_bot || !atEntry)
        return;

    TC_LOG_DEBUG("module.playerbot.quest",
                "DynamicSpawnHandler: Bot {} triggering area trigger {} (entered={})",
                _bot->GetName(), atEntry->ID, entered);

    // Call script manager - this is what the real client packet handler does
    if (sScriptMgr->OnAreaTrigger(_bot, atEntry, entered))
        return;

    // Process quest objectives
    if (_bot->IsAlive() && entered)
    {
        if (std::unordered_set<uint32> const* quests = sObjectMgr->GetQuestsForAreaTrigger(atEntry->ID))
        {
            for (uint32 questId : *quests)
            {
                Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
                uint16 slot = _bot->FindQuestSlot(questId);

                if (quest && slot < MAX_QUEST_LOG_SIZE && _bot->GetQuestStatus(questId) == QUEST_STATUS_INCOMPLETE)
                {
                    for (QuestObjective const& obj : quest->GetObjectives())
                    {
                        if (obj.Type != QUEST_OBJECTIVE_AREATRIGGER)
                            continue;

                        if (!_bot->IsQuestObjectiveCompletable(slot, quest, obj))
                            continue;

                        if (_bot->IsQuestObjectiveComplete(slot, quest, obj))
                            continue;

                        if (obj.ObjectID != -1 && obj.ObjectID != static_cast<int32>(atEntry->ID))
                            continue;

                        _bot->SetQuestObjectiveData(obj, 1);
                        _bot->SendQuestUpdateAddCreditSimple(obj);

                        TC_LOG_DEBUG("module.playerbot.quest",
                                    "DynamicSpawnHandler: Bot {} completed area trigger objective for quest {}",
                                    _bot->GetName(), questId);
                        break;
                    }

                    if (quest->HasFlag(QUEST_FLAGS_COMPLETION_AREA_TRIGGER))
                        _bot->AreaExploredOrEventHappens(questId);

                    if (_bot->CanCompleteQuest(questId))
                        _bot->CompleteQuest(questId);
                }
            }
        }
    }

    // Handle tavern/inn triggers
    if (sObjectMgr->IsTavernAreaTrigger(atEntry->ID))
    {
        if (entered)
            _bot->GetRestMgr().SetRestFlag(REST_FLAG_IN_TAVERN);
        else
            _bot->GetRestMgr().RemoveRestFlag(REST_FLAG_IN_TAVERN);
    }
}

void DynamicSpawnHandler::CleanupExpiredSpawns()
{
    uint32 currentTime = GameTime::GetGameTimeMS();

    for (auto it = _spawnedCreatures.begin(); it != _spawnedCreatures.end(); )
    {
        if (currentTime - it->second.spawnTime > SPAWN_EXPIRY_MS)
        {
            TC_LOG_DEBUG("module.playerbot.quest",
                        "DynamicSpawnHandler: Removing expired spawn tracking for creature {}",
                        it->first.ToString());
            it = _spawnedCreatures.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // Also cleanup old area trigger cooldowns
    for (auto it = _triggeredAreaTriggers.begin(); it != _triggeredAreaTriggers.end(); )
    {
        if (currentTime - it->second > AREATRIGGER_COOLDOWN_MS * 2)
            it = _triggeredAreaTriggers.erase(it);
        else
            ++it;
    }
}

} // namespace Playerbot
