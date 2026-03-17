/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Strategy.h"
#include "../../Quest/ObjectiveTracker.h"
#include "../../Quest/DynamicSpawnHandler.h"
#include "Movement/BotMovementUtil.h"
#include "Player.h"
#include "QuestDef.h"
#include "../../Game/QuestAcceptanceManager.h"
#include "../../Travel/TravelRouteManager.h"
#include <vector>

namespace Playerbot
{

class BotAI;
class QuestAcceptanceManager;

/**
 * @brief Quest Strategy - Drives bot movement and actions for quest objectives
 *
 * This strategy activates when bots have active quests and guides them to:
 * - Move to quest objective locations
 * - Kill quest mobs
 * - Collect quest items
 * - Explore quest areas
 * - Turn in completed quests
 *
 * Integrates with:
 * - QuestManager (quest acceptance/turn-in)
 * - ObjectiveTracker (objective prioritization)
 * - BotMovementUtil (movement execution)
 * - GroupCombatStrategy (combat during quests)
 */
class TC_GAME_API QuestStrategy : public Strategy
{
public:
    QuestStrategy();
    ~QuestStrategy() override = default;

    // Strategy interface
    void InitializeActions();
    void InitializeTriggers();
    void InitializeValues();
    bool IsActive(BotAI* ai) const;
    float GetRelevance(BotAI* ai) const;
    void UpdateBehavior(BotAI* ai, uint32 diff);

    // ========================================================================
    // GRIND FALLBACK INTEGRATION
    // ========================================================================

    /**
     * @brief Get number of consecutive quest giver search failures
     * @return Failure count (used by GrindStrategy to determine when to activate)
     */
    uint32 GetQuestGiverSearchFailures() const { return _questGiverSearchFailures; }

    /**
     * @brief Check if quest strategy has exhausted all options
     * @return true if no quests, no quest givers, and no quest hubs found
     */
    bool HasExhaustedQuestOptions() const { return _questGiverSearchFailures >= 3; }

    /**
     * @brief Reset quest search failure counter (called when quests become available)
     */
    void ResetQuestSearchFailures() { _questGiverSearchFailures = 0; }

private:
    // Quest navigation phases
    enum class QuestPhase
    {
        IDLE,                   // No quest activity
        MOVING_TO_OBJECTIVE,    // Navigating to quest area
        ENGAGING_TARGETS,       // Killing quest mobs
        COLLECTING_ITEMS,       // Looting quest items
        EXPLORING_AREA,         // Exploring for quest completion
        TURNING_IN,             // Moving to turn in quest
    };

    // Core quest execution
    void ProcessQuestObjectives(BotAI* ai);
    void SearchForQuestGivers(BotAI* ai);
    void NavigateToObjective(BotAI* ai, ObjectiveState const& objective);
    void EngageQuestTargets(BotAI* ai, ObjectiveState const& objective);
    void CollectQuestItems(BotAI* ai, ObjectiveState const& objective);
    void ExploreQuestArea(BotAI* ai, ObjectiveState const& objective);
    void TurnInQuest(BotAI* ai, uint32 questId);

    // Quest item usage system (for quests that require using items on targets)
    void UseQuestItemOnTarget(BotAI* ai, ObjectiveState const& objective);

    // ========================================================================
    // QUEST OBJECTIVE TYPE HANDLERS (complete coverage of all 22 objective types)
    // ========================================================================

    // TALKTO objectives - interact with NPC via gossip/dialog
    void TalkToNpc(BotAI* ai, ObjectiveState const& objective);

    // Currency objectives - check if bot has required currency
    void HandleCurrencyObjective(BotAI* ai, ObjectiveState const& objective);

    // Money objectives - check if bot has required gold/silver/copper
    void HandleMoneyObjective(BotAI* ai, ObjectiveState const& objective);

    // Objective analysis
    ObjectivePriority GetCurrentObjective(BotAI* ai) const;
    bool HasActiveObjectives(BotAI* ai) const;
    bool ShouldEngageTarget(BotAI* ai, ::Unit* target, ObjectiveState const& objective) const;

    // Movement helpers
    bool MoveToObjectiveLocation(BotAI* ai, Position const& location);
    bool MoveToQuestGiver(BotAI* ai, uint32 questId);
    Position GetObjectivePosition(BotAI* ai, ObjectiveState const& objective) const;

    // Target detection
    ::Unit* FindQuestTarget(BotAI* ai, ObjectiveState const& objective) const;
    GameObject* FindQuestObject(BotAI* ai, ObjectiveState const& objective) const;
    ::Item* FindQuestItem(BotAI* ai, ObjectiveState const& objective) const;

    // Quest turn-in system (multi-tier fallback with creature AND gameobject support)
    struct QuestEnderLocation
    {
        uint32 objectEntry = 0;         // Creature OR GameObject entry ID
        Position position;
        uint32 targetMapId = 0;         // Map where quest ender is located
        bool isGameObject = false;      // True if quest ender is a GameObject, false if Creature
        bool foundViaSpawn = false;     // True if found via spawn data (creature or gameobject)
        bool foundViaPOI = false;       // True if found via Quest POI
        bool requiresSearch = false;    // True if needs area search
        bool isOnDifferentMap = false;  // True if quest ender exists but on a different map

        // Helper methods for clarity
        bool IsCreature() const { return !isGameObject && objectEntry != 0; }
        bool IsGameObject() const { return isGameObject && objectEntry != 0; }
        bool IsValid() const { return objectEntry != 0; }
        bool HasValidPosition() const { return position.GetPositionX() != 0.0f || position.GetPositionY() != 0.0f || position.GetPositionZ() != 0.0f; }
        // NOTE: targetMapId=0 is valid (Eastern Kingdoms) - don't check targetMapId != 0
        bool RequiresMapTravel() const { return isOnDifferentMap && HasValidPosition(); }
    };

    bool FindQuestEnderLocation(BotAI* ai, uint32 questId, QuestEnderLocation& location);
    bool NavigateToQuestEnder(BotAI* ai, QuestEnderLocation const& location);
    bool CheckForQuestEnderInRange(BotAI* ai, QuestEnderLocation const& location);
    bool CheckForCreatureQuestEnderInRange(BotAI* ai, uint32 creatureEntry);
    bool CheckForGameObjectQuestEnderInRange(BotAI* ai, uint32 gameobjectEntry);
    bool CompleteQuestTurnIn(BotAI* ai, uint32 questId, ::Unit* questEnder);
    bool CompleteQuestTurnInAtGameObject(BotAI* ai, uint32 questId, GameObject* questEnder);

    // State management
    QuestPhase _currentPhase;
    uint32 _phaseTimer;
    uint32 _lastObjectiveUpdate;
    ObjectGuid _currentTargetGuid;
    uint32 _currentQuestId;
    uint32 _currentObjectiveIndex;
    Position _currentObjectivePosition;

    // Quest giver search throttling (prevent log spam)
    uint32 _lastQuestGiverSearchTime;
    uint32 _questGiverSearchFailures;

    // Quest area wandering (for respawn waiting)
    uint32 _lastWanderTime;
    uint32 _currentWanderPointIndex;
    ::std::vector<Position> _questAreaWanderPoints;

    // Performance tracking
    uint32 _objectivesCompleted;
    uint32 _questsCompleted;
    uint32 _averageObjectiveTime;

    // Quest acceptance manager (enterprise-grade auto-acceptance)
    ::std::unique_ptr<QuestAcceptanceManager> _acceptanceManager;

    // Helper methods for area wandering
    bool ShouldWanderInQuestArea(BotAI* ai, ObjectiveState const& objective) const;
    void InitializeQuestAreaWandering(BotAI* ai, ObjectiveState const& objective);
    void WanderInQuestArea(BotAI* ai);

    // Item loot source detection
    bool IsItemFromCreatureLoot(uint32 itemId) const;

    // NPC interaction detection (distinguish "talk to" NPCs from "killable neutral" mobs)
    bool RequiresSpellClickInteraction(uint32 creatureEntry) const;

    // Quest item usage tracking (prevent recasting on same/despawned targets)
    // Key: questId, Value: set of used target GUIDs
    ::std::unordered_map<uint32, ::std::set<ObjectGuid>> _usedQuestItemTargets;
    uint32 _lastQuestItemCastTime = 0;
    static constexpr uint32 QUEST_ITEM_CAST_COOLDOWN_MS = 5000; // 5 second cooldown between casts

    // Failed quest turn-in tracking (prevent infinite loops on quests missing required items)
    // Key: questId, Value: consecutive failure count
    ::std::unordered_map<uint32, uint32> _questTurnInFailures;
    static constexpr uint32 MAX_QUEST_TURNIN_FAILURES = 3; // Abandon quest after 3 consecutive failures

    // Failed quest objective tracking (prevent infinite loops on unreachable objectives)
    // Key: questId << 8 | objectiveIndex, Value: {failureCount, lastFailureTime}
    struct ObjectiveFailureInfo
    {
        uint32 failureCount = 0;
        uint32 lastFailureTime = 0;
    };
    ::std::unordered_map<uint32, ObjectiveFailureInfo> _questObjectiveFailures;
    static constexpr uint32 MAX_QUEST_OBJECTIVE_FAILURES = 10; // Skip objective after 10 consecutive failures
    static constexpr uint32 OBJECTIVE_FAILURE_RESET_TIME_MS = 300000; // Reset failures after 5 minutes of not trying

    // Helper to get/set objective failure key
    static uint32 MakeObjectiveKey(uint32 questId, uint8 objectiveIndex) { return (questId << 8) | objectiveIndex; }
    uint32 GetObjectiveFailures(uint32 questId, uint8 objectiveIndex);
    void IncrementObjectiveFailures(uint32 questId, uint8 objectiveIndex);
    void ResetObjectiveFailures(uint32 questId, uint8 objectiveIndex);
    bool IsObjectiveBlacklisted(uint32 questId, uint8 objectiveIndex);

    // ========================================================================
    // PERSISTENT TRAVEL MANAGER (for multi-step transport journeys)
    // ========================================================================
    // Required for ship/zeppelin boarding - state must persist across update ticks
    ::std::unique_ptr<TravelRouteManager> _travelManager;
    uint32 _lastTravelQuestId = 0;  // Track which quest initiated the travel

    // ========================================================================
    // DYNAMIC SPAWN HANDLER (for quests with dynamically-spawned NPCs)
    // ========================================================================
    // Enables bots to trigger spawns via area triggers, gossip, etc.
    ::std::unique_ptr<DynamicSpawnHandler> _dynamicSpawnHandler;

    // Helper to check/trigger dynamic spawns for objectives
    bool TryTriggerDynamicSpawn(BotAI* ai, ObjectiveState const& objective);

    // ========================================================================
    // TRAVEL FAILURE COOLDOWN (prevent infinite back-and-forth loops)
    // ========================================================================
    // Key: questId, Value: {lastFailureTime, failureCount}
    struct TravelFailureInfo
    {
        uint32 lastFailureTime = 0;
        uint32 failureCount = 0;
    };
    ::std::unordered_map<uint32, TravelFailureInfo> _travelFailures;
    static constexpr uint32 TRAVEL_FAILURE_COOLDOWN_MS = 300000;  // 5 minute cooldown after failure
    static constexpr uint32 MAX_TRAVEL_FAILURES = 3;              // Give up after 3 failures
};

} // namespace Playerbot
