/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "Position.h"
#include "GameTime.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <optional>

class Player;
class Creature;
struct AreaTriggerEntry;
class Quest;

namespace Playerbot
{

/**
 * @brief Spawn trigger type - how the spawn is activated
 */
enum class SpawnTriggerType : uint8
{
    NONE = 0,
    AREA_TRIGGER,       // DBC area trigger - bot enters a zone/region
    QUEST_ACCEPT,       // SmartAI SMART_EVENT_QUEST_ACCEPTED - accepting quest spawns NPC
    GOSSIP_SELECT,      // SmartAI SMART_EVENT_GOSSIP_SELECT - dialog option spawns NPC
    SPELL_CLICK,        // npc_spellclick_spells - clicking NPC triggers spawn
    CREATURE_AI,        // Creature script triggers spawn (complex scripted events)
    PHASE_SHIFT,        // Phasing system - NPC exists but requires phase change
};

/**
 * @brief Information about a dynamic spawn requirement for a quest
 */
struct DynamicSpawnInfo
{
    uint32 questId = 0;
    uint32 objectiveIndex = 0;
    uint32 creatureEntry = 0;          // Creature that needs to be spawned
    SpawnTriggerType triggerType = SpawnTriggerType::NONE;

    // Area trigger specific
    uint32 areaTriggerDBC = 0;         // DBC AreaTrigger ID
    Position areaTriggerPos;           // Position to move to
    float areaTriggerRadius = 0.0f;    // Radius to enter

    // SmartAI specific
    uint32 smartScriptEntry = 0;       // smart_scripts entryorguid
    uint32 smartScriptSource = 0;      // smart_scripts source_type

    // Gossip specific
    uint32 gossipNpcEntry = 0;         // NPC to talk to
    uint32 gossipMenuId = 0;           // Gossip menu
    uint32 gossipOptionId = 0;         // Gossip option to select

    // Phase specific
    uint32 requiredPhase = 0;          // Phase bot needs to be in

    // Status tracking
    bool triggered = false;
    uint32 lastAttemptTime = 0;
    uint32 attemptCount = 0;

    bool IsValid() const { return creatureEntry != 0 && triggerType != SpawnTriggerType::NONE; }
};

/**
 * @brief Cached area trigger data for quick lookup
 */
struct AreaTriggerData
{
    uint32 triggerId = 0;
    uint32 mapId = 0;
    Position position;
    float radius = 0.0f;

    // Associated quest objectives (if any)
    std::vector<uint32> questIds;

    // SmartAI scripts that trigger on this area trigger
    bool hasSmartScript = false;
    bool summonCreature = false;  // true if SMART_ACTION_SUMMON_CREATURE is used
    uint32 summonedCreatureEntry = 0;
};

/**
 * @class DynamicSpawnHandler
 * @brief Handles triggering of dynamically spawned NPCs for bot quest completion
 *
 * Many modern WoW quests spawn NPCs dynamically when players:
 * - Enter area triggers
 * - Accept quests
 * - Select gossip options
 * - Complete other objectives
 *
 * This handler enables bots to trigger these spawns by:
 * 1. Detecting when a quest requires a dynamically-spawned creature
 * 2. Finding the spawn trigger mechanism
 * 3. Executing the trigger (entering area, selecting gossip, etc.)
 * 4. Tracking spawned creatures for objective completion
 *
 * Performance: ~0.05ms per Update() call, uses caching extensively
 */
class TC_GAME_API DynamicSpawnHandler
{
public:
    explicit DynamicSpawnHandler(Player* bot);
    ~DynamicSpawnHandler() = default;

    DynamicSpawnHandler(DynamicSpawnHandler const&) = delete;
    DynamicSpawnHandler& operator=(DynamicSpawnHandler const&) = delete;

    // ========================================================================
    // CORE UPDATE LOOP
    // ========================================================================

    /**
     * @brief Main update - called from QuestStrategy::UpdateBehavior
     * @param diff Time since last update in ms
     */
    void Update(uint32 diff);

    // ========================================================================
    // SPAWN REQUIREMENT DETECTION
    // ========================================================================

    /**
     * @brief Check if a creature requires dynamic spawning
     * @param creatureEntry Creature template entry
     * @return true if creature has no static spawns or is phase-gated
     */
    bool RequiresDynamicSpawn(uint32 creatureEntry) const;

    /**
     * @brief Get spawn info for a quest objective
     * @param questId Quest ID
     * @param objectiveIndex Objective index
     * @return Optional spawn info if dynamic spawn required
     */
    std::optional<DynamicSpawnInfo> GetSpawnInfoForObjective(uint32 questId, uint8 objectiveIndex);

    /**
     * @brief Analyze a quest for dynamic spawn requirements
     * @param questId Quest to analyze
     * @return Vector of spawn infos for all objectives needing dynamic spawns
     */
    std::vector<DynamicSpawnInfo> AnalyzeQuestSpawnRequirements(uint32 questId);

    // ========================================================================
    // SPAWN TRIGGERING
    // ========================================================================

    /**
     * @brief Attempt to trigger a spawn for a quest objective
     * @param spawnInfo Spawn information from GetSpawnInfoForObjective
     * @return true if trigger action was initiated
     */
    bool TriggerSpawn(DynamicSpawnInfo& spawnInfo);

    /**
     * @brief Check and trigger area triggers the bot is near
     * @return true if any area trigger was triggered
     */
    bool ProcessNearbyAreaTriggers();

    /**
     * @brief Trigger a specific area trigger for the bot
     * @param triggerId DBC AreaTrigger ID
     * @return true if trigger was successful
     */
    bool TriggerAreaTrigger(uint32 triggerId);

    // ========================================================================
    // AREA TRIGGER DETECTION
    // ========================================================================

    /**
     * @brief Get area triggers near the bot
     * @param radius Search radius (default: 50 yards)
     * @return Vector of nearby area trigger IDs
     */
    std::vector<uint32> GetNearbyAreaTriggers(float radius = 50.0f) const;

    /**
     * @brief Check if bot is inside an area trigger
     * @param triggerId AreaTrigger ID to check
     * @return true if bot is within trigger bounds
     */
    bool IsInAreaTrigger(uint32 triggerId) const;

    /**
     * @brief Get position to move to for triggering an area trigger
     * @param triggerId AreaTrigger ID
     * @return Position at center of trigger, or bot position if not found
     */
    Position GetAreaTriggerPosition(uint32 triggerId) const;

    // ========================================================================
    // SPAWNED CREATURE TRACKING
    // ========================================================================

    /**
     * @brief Check if a creature was spawned by this handler
     * @param guid Creature GUID
     * @return true if creature was spawned by bot interaction
     */
    bool IsSpawnedCreature(ObjectGuid guid) const;

    /**
     * @brief Track a newly spawned creature
     * @param guid Creature GUID
     * @param entry Creature entry
     * @param questId Quest that triggered spawn
     */
    void TrackSpawnedCreature(ObjectGuid guid, uint32 entry, uint32 questId);

    /**
     * @brief Get spawned creatures for a quest
     * @param questId Quest ID
     * @return Vector of spawned creature GUIDs
     */
    std::vector<ObjectGuid> GetSpawnedCreaturesForQuest(uint32 questId) const;

    // ========================================================================
    // CACHING AND PERFORMANCE
    // ========================================================================

    /**
     * @brief Preload spawn data for all bot quests
     * Call once after bot logs in or accepts new quests
     */
    void PreloadQuestSpawnData();

    /**
     * @brief Clear cached data for a completed quest
     * @param questId Quest ID to clear
     */
    void ClearQuestCache(uint32 questId);

    /**
     * @brief Clear all cached spawn data
     */
    void ClearAllCaches();

private:
    Player* _bot;

    // ========================================================================
    // STATIC CACHES (shared across all handlers)
    // ========================================================================

    // Area trigger cache: triggerId -> AreaTriggerData
    static std::unordered_map<uint32, AreaTriggerData> _areaTriggerCache;
    static bool _areaTriggerCacheInitialized;

    // Creature spawn check cache: creatureEntry -> hasDynamicSpawn
    static std::unordered_map<uint32, bool> _creatureSpawnCache;

    // SmartScript summon cache: entryorguid -> summonedCreatureEntry
    static std::unordered_map<int64, uint32> _smartScriptSummonCache;

    // ========================================================================
    // INSTANCE DATA
    // ========================================================================

    // Quest spawn requirements: questId -> vector of spawn infos
    std::unordered_map<uint32, std::vector<DynamicSpawnInfo>> _questSpawnReqs;

    // Triggered area triggers (don't re-trigger): triggerId -> lastTriggerTime
    std::unordered_map<uint32, uint32> _triggeredAreaTriggers;
    static constexpr uint32 AREATRIGGER_COOLDOWN_MS = 60000;  // 1 minute cooldown

    // Spawned creatures: GUID -> {entry, questId, spawnTime}
    struct SpawnedCreatureInfo
    {
        uint32 entry = 0;
        uint32 questId = 0;
        uint32 spawnTime = 0;
    };
    std::unordered_map<ObjectGuid, SpawnedCreatureInfo> _spawnedCreatures;

    // Update throttling
    uint32 _lastUpdateTime = 0;
    static constexpr uint32 UPDATE_INTERVAL_MS = 1000;  // Check every 1 second

    // ========================================================================
    // INTERNAL HELPERS
    // ========================================================================

    /**
     * @brief Initialize static caches (called once per server)
     */
    static void InitializeStaticCaches();

    /**
     * @brief Query smart_scripts for summon actions
     * @param creatureEntry Creature we're looking for
     * @return Optional spawn info if found in smart_scripts
     */
    std::optional<DynamicSpawnInfo> QuerySmartScriptsForSpawn(uint32 creatureEntry);

    /**
     * @brief Query areatrigger_involvedrelation for quest triggers
     * @param questId Quest ID
     * @return Vector of area trigger IDs
     */
    std::vector<uint32> QueryAreaTriggersForQuest(uint32 questId) const;

    /**
     * @brief Check if creature has any static spawn points
     * @param creatureEntry Creature entry
     * @return true if creature exists in creature table
     */
    bool HasStaticSpawn(uint32 creatureEntry) const;

    /**
     * @brief Get SmartScript summon info for an area trigger
     * @param triggerId AreaTrigger ID
     * @return Summoned creature entry or 0 if none
     */
    uint32 GetAreaTriggerSummonCreature(uint32 triggerId) const;

    /**
     * @brief Execute the actual area trigger script for bot
     * @param atEntry AreaTrigger entry from DBC
     * @param entered true if entering, false if leaving
     */
    void ExecuteAreaTrigger(AreaTriggerEntry const* atEntry, bool entered);

    /**
     * @brief Clean up expired spawned creatures
     */
    void CleanupExpiredSpawns();

    // Spawn expiry time (creatures despawn after this)
    static constexpr uint32 SPAWN_EXPIRY_MS = 300000;  // 5 minutes
};

} // namespace Playerbot
