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
#include "Threading/LockHierarchy.h"
#include "DungeonTypes.h"  // Canonical enum definitions
#include "Player.h"
#include "Group.h"
#include "Map.h"
#include "InstanceScript.h"
#include "Position.h"
#include "GameTime.h"  // For GameTime::GetGameTimeMS()
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>
#include <atomic>
#include <mutex>
#include <chrono>

class Player;
class Group;
class Map;
class InstanceScript;
class Creature;
class GameObject;
class Unit;

namespace Playerbot
{

// DungeonRole, DungeonPhase, EncounterStrategyType, ThreatManagement
// are now defined in DungeonTypes.h

struct DungeonEncounter
{
    uint32 encounterId;
    ::std::string encounterName;
    uint32 creatureId;
    Position encounterLocation;
    ::std::vector<uint32> trashMobIds;
    ::std::vector<Position> trashLocations;
    EncounterStrategyType recommendedStrategy;
    ThreatManagement threatStrategy;
    uint32 estimatedDuration;
    float difficultyRating;
    ::std::vector<::std::string> mechanics;
    ::std::vector<::std::string> warnings;
    bool requiresSpecialPositioning;
    bool hasEnrageTimer;
    uint32 enrageTimeSeconds;

    DungeonEncounter(uint32 id, const ::std::string& name, uint32 creatureId)
        : encounterId(id), encounterName(name), creatureId(creatureId)
        , recommendedStrategy(EncounterStrategyType::BALANCED)
        , threatStrategy(ThreatManagement::STRICT_AGGRO)
        , estimatedDuration(300000), difficultyRating(5.0f)
        , requiresSpecialPositioning(false), hasEnrageTimer(false)
        , enrageTimeSeconds(0) {}
};

struct DungeonData
{
    uint32 dungeonId;
    ::std::string dungeonName;
    uint32 mapId;
    uint32 recommendedLevel;
    uint32 minLevel;
    uint32 maxLevel;
    uint32 recommendedGroupSize;
    ::std::vector<DungeonEncounter> encounters;
    ::std::vector<Position> safeSpots;
    ::std::vector<Position> dangerousAreas;
    ::std::unordered_map<uint32, ::std::string> importantNotes;
    uint32 averageCompletionTime;
    float difficultyRating;
    bool requiresQuests;
    bool hasKeyRequirement;

    DungeonData() : dungeonId(0), mapId(0), recommendedLevel(20)
        , minLevel(15), maxLevel(25), recommendedGroupSize(5)
        , averageCompletionTime(2700000), difficultyRating(5.0f)
        , requiresQuests(false), hasKeyRequirement(false) {}

    DungeonData(uint32 id, const ::std::string& name, uint32 map)
        : dungeonId(id), dungeonName(name), mapId(map), recommendedLevel(20)
        , minLevel(15), maxLevel(25), recommendedGroupSize(5)
        , averageCompletionTime(2700000), difficultyRating(5.0f)
        , requiresQuests(false), hasKeyRequirement(false) {}
};

struct GroupDungeonState
{
    uint32 groupId;
    uint32 dungeonId;
    DungeonPhase currentPhase;
    uint32 currentEncounterId;
    uint32 encountersCompleted;
    uint32 totalEncounters;
    uint32 wipeCount;
    uint32 startTime;
    uint32 lastProgressTime;
    ::std::vector<uint32> completedEncounters;
    ::std::vector<uint32> failedEncounters;
    Position lastGroupPosition;
    bool isStuck;
    uint32 stuckTime;
    EncounterStrategyType activeStrategy;

    GroupDungeonState() : groupId(0), dungeonId(0)
        , currentPhase(DungeonPhase::ENTERING), currentEncounterId(0)
        , encountersCompleted(0), totalEncounters(0), wipeCount(0)
        , startTime(0), lastProgressTime(0)
        , isStuck(false), stuckTime(0), activeStrategy(EncounterStrategyType::BALANCED) {}

    GroupDungeonState(uint32 gId, uint32 dId) : groupId(gId), dungeonId(dId)
        , currentPhase(DungeonPhase::ENTERING), currentEncounterId(0)
        , encountersCompleted(0), totalEncounters(0), wipeCount(0)
        , startTime(GameTime::GetGameTimeMS()), lastProgressTime(GameTime::GetGameTimeMS())
        , isStuck(false), stuckTime(0), activeStrategy(EncounterStrategyType::BALANCED) {}
};

/**
 * @brief Non-atomic snapshot of dungeon metrics (copyable)
 */
struct DungeonMetrics
{
    uint32 dungeonsAttempted{0};
    uint32 dungeonsCompleted{0};
    uint32 dungeonsFailed{0};
    uint32 encountersCompleted{0};
    uint32 encountersFailed{0};
    uint32 totalWipes{0};
    uint32 averageCompletionTimeMs{0};
    uint32 lastDungeonTime{0};
    float averageCompletionTime{0.0f};
    uint32 encounterWipes{0};

    DungeonMetrics() = default;

    void Reset()
    {
        dungeonsAttempted = 0;
        dungeonsCompleted = 0;
        dungeonsFailed = 0;
        encountersCompleted = 0;
        encountersFailed = 0;
        totalWipes = 0;
        averageCompletionTimeMs = 0;
        lastDungeonTime = 0;
        averageCompletionTime = 0.0f;
        encounterWipes = 0;
    }
};

/**
 * @brief Atomic dungeon metrics for thread-safe operations
 */
struct AtomicDungeonMetrics
{
    ::std::atomic<uint32> dungeonsAttempted{0};
    ::std::atomic<uint32> dungeonsCompleted{0};
    ::std::atomic<uint32> dungeonsFailed{0};
    ::std::atomic<uint32> encountersCompleted{0};
    ::std::atomic<uint32> encountersFailed{0};
    ::std::atomic<uint32> totalWipes{0};
    ::std::atomic<uint32> averageCompletionTimeMs{0};
    ::std::atomic<uint32> lastDungeonTime{0};
    ::std::atomic<float> averageCompletionTime{0.0f};
    ::std::atomic<uint32> encounterWipes{0};

    AtomicDungeonMetrics() = default;

    DungeonMetrics GetSnapshot() const
    {
        DungeonMetrics snapshot;
        snapshot.dungeonsAttempted = dungeonsAttempted.load();
        snapshot.dungeonsCompleted = dungeonsCompleted.load();
        snapshot.dungeonsFailed = dungeonsFailed.load();
        snapshot.encountersCompleted = encountersCompleted.load();
        snapshot.encountersFailed = encountersFailed.load();
        snapshot.totalWipes = totalWipes.load();
        snapshot.averageCompletionTimeMs = averageCompletionTimeMs.load();
        snapshot.lastDungeonTime = lastDungeonTime.load();
        snapshot.averageCompletionTime = averageCompletionTime.load();
        snapshot.encounterWipes = encounterWipes.load();
        return snapshot;
    }
};

class TC_GAME_API DungeonBehavior final 
{
public:
    static DungeonBehavior* instance();

    // Core dungeon management
    bool EnterDungeon(Group* group, uint32 dungeonId);
    void UpdateDungeonProgress(Group* group);
    void HandleDungeonCompletion(Group* group);
    void HandleDungeonWipe(Group* group);

    // Encounter management
    void StartEncounter(Group* group, uint32 encounterId);
    void UpdateEncounter(Group* group, uint32 encounterId);
    void CompleteEncounter(Group* group, uint32 encounterId);
    void HandleEncounterWipe(Group* group, uint32 encounterId);

    // Role-specific behavior coordination
    void CoordinateTankBehavior(Player* tank, const DungeonEncounter& encounter);
    void CoordinateHealerBehavior(Player* healer, const DungeonEncounter& encounter);
    void CoordinateDpsBehavior(Player* dps, const DungeonEncounter& encounter);
    void CoordinateCrowdControlBehavior(Player* cc, const DungeonEncounter& encounter);

    // Movement and positioning
    void UpdateGroupPositioning(Group* group, const DungeonEncounter& encounter);
    void HandleSpecialPositioning(Group* group, uint32 encounterId);
    Position GetOptimalPosition(Player* player, DungeonRole role, const DungeonEncounter& encounter);
    void AvoidDangerousAreas(Player* player, const ::std::vector<Position>& dangerousAreas);

    // Trash mob handling
    void HandleTrashMobs(Group* group, const ::std::vector<uint32>& trashMobIds);
    void PullTrashGroup(Group* group, const ::std::vector<Unit*>& trashMobs);
    void AssignTrashTargets(Group* group, const ::std::vector<Unit*>& trashMobs);
    void ExecuteTrashStrategy(Group* group, const ::std::vector<Unit*>& trashMobs);

    // Boss encounter strategies
    void ExecuteBossStrategy(Group* group, const DungeonEncounter& encounter);
    void HandleBossMechanics(Group* group, uint32 encounterId, const ::std::string& mechanic);
    void AdaptToEncounterPhase(Group* group, uint32 encounterId, uint32 phase);
    void HandleEnrageTimer(Group* group, const DungeonEncounter& encounter);

    // Threat and aggro management
    void ManageGroupThreat(Group* group, const DungeonEncounter& encounter);
    void HandleTankSwap(Group* group, Player* currentTank, Player* newTank);
    void ManageThreatMeters(Group* group);
    void HandleThreatEmergency(Group* group, Player* player);

    // Healing and damage coordination
    void CoordinateGroupHealing(Group* group, const DungeonEncounter& encounter);
    void CoordinateGroupDamage(Group* group, const DungeonEncounter& encounter);
    void HandleHealingEmergency(Group* group);
    void OptimizeDamageOutput(Group* group, const DungeonEncounter& encounter);

    // Crowd control and utility
    void CoordinateCrowdControl(Group* group, const ::std::vector<Unit*>& targets);
    void HandleCrowdControlBreaks(Group* group, Unit* target);
    void ManageGroupUtilities(Group* group, const DungeonEncounter& encounter);
    void HandleSpecialAbilities(Group* group, uint32 encounterId);

    // Loot and rewards management
    void HandleEncounterLoot(Group* group, uint32 encounterId);
    void DistributeLoot(Group* group, const ::std::vector<uint32>& lootItems);
    void HandleNeedGreedPass(Group* group, uint32 itemId, Player* player);
    void OptimizeLootDistribution(Group* group);

    // Performance monitoring and adaptation (DungeonMetrics defined in IDungeonBehavior.h interface)
    DungeonMetrics GetGroupDungeonMetrics(uint32 groupId);
    DungeonMetrics GetGlobalDungeonMetrics();

    // Dungeon-specific strategies
    void LoadDungeonData();
    DungeonData GetDungeonData(uint32 dungeonId);
    DungeonEncounter GetEncounterData(uint32 encounterId);
    void UpdateDungeonStrategy(Group* group, EncounterStrategyType strategy);

    // Error handling and recovery
    void HandleDungeonError(Group* group, const ::std::string& error);
    void RecoverFromWipe(Group* group);
    void HandlePlayerDisconnection(Group* group, Player* disconnectedPlayer);
    void HandleGroupDisbandInDungeon(Group* group);

    // Role determination
    DungeonRole DeterminePlayerRole(Player* player);

    // State access
    GroupDungeonState GetGroupDungeonState(uint32 groupId) const;

    // Configuration and settings
    void SetEncounterStrategy(uint32 groupId, EncounterStrategyType strategy);
    EncounterStrategyType GetEncounterStrategy(uint32 groupId);
    void SetThreatManagement(uint32 groupId, ThreatManagement management);
    void EnableAdaptiveBehavior(uint32 groupId, bool enable);

    // Update and maintenance
    void Update(uint32 diff);
    void UpdateGroupDungeon(Group* group, uint32 diff);
    void CleanupInactiveDungeons();

private:
    DungeonBehavior();
    ~DungeonBehavior() = default;

    // Core data structures
    ::std::unordered_map<uint32, DungeonData> _dungeonDatabase; // dungeonId -> data
    ::std::unordered_map<uint32, GroupDungeonState> _groupDungeonStates; // groupId -> state
    ::std::unordered_map<uint32, AtomicDungeonMetrics> _groupMetrics;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BEHAVIOR_MANAGER> _dungeonMutex;

    // Encounter tracking
    ::std::unordered_map<uint32, ::std::vector<DungeonEncounter>> _dungeonEncounters; // dungeonId -> encounters
    ::std::unordered_map<uint32, uint32> _encounterProgress; // groupId -> currentEncounterId
    ::std::unordered_map<uint32, uint32> _encounterStartTime; // groupId -> startTime

    // Strategy and adaptation
    ::std::unordered_map<uint32, EncounterStrategyType> _groupStrategies; // groupId -> strategy
    ::std::unordered_map<uint32, ThreatManagement> _groupThreatManagement; // groupId -> threat management
    ::std::unordered_map<uint32, bool> _adaptiveBehaviorEnabled; // groupId -> enabled

    // Performance tracking
    AtomicDungeonMetrics _globalMetrics;

    // Helper functions
    void InitializeDungeonDatabase();
    void LoadClassicDungeons();
    void LoadBurningCrusadeDungeons();
    void LoadWrathDungeons();
    void LoadCataclysmDungeons();
    void LoadPandariaDungeons();
    void LoadDraenorDungeons();
    void LoadLegionDungeons();
    void LoadBfADungeons();
    void LoadShadowlandsDungeons();
    void LoadDragonflightDungeons();

    // Encounter-specific implementations
    void HandleDeadminesStrategy(Group* group, uint32 encounterId);
    void HandleWailingCavernsStrategy(Group* group, uint32 encounterId);
    void HandleShadowfangKeepStrategy(Group* group, uint32 encounterId);
    void HandleStormwindStockadeStrategy(Group* group, uint32 encounterId);
    void HandleRazorfenKraulStrategy(Group* group, uint32 encounterId);
    void HandleBlackfathomDeepsStrategy(Group* group, uint32 encounterId);

    // Role coordination helpers
    void AssignTankTargets(Player* tank, const ::std::vector<Unit*>& enemies);
    void PrioritizeHealingTargets(Player* healer, const ::std::vector<Player*>& groupMembers);
    void AssignDpsTargets(Player* dps, const ::std::vector<Unit*>& enemies);
    void CoordinateInterrupts(Group* group, Unit* target);

    // Movement and positioning algorithms
    Position CalculateTankPosition(const DungeonEncounter& encounter, const ::std::vector<Unit*>& enemies);
    Position CalculateHealerPosition(const DungeonEncounter& encounter, const ::std::vector<Player*>& groupMembers);
    Position CalculateDpsPosition(const DungeonEncounter& encounter, Unit* target);
    void UpdateGroupFormation(Group* group, const DungeonEncounter& encounter);

    // Combat coordination
    void InitiatePull(Group* group, const ::std::vector<Unit*>& enemies);
    void ManageCombatPriorities(Group* group, const DungeonEncounter& encounter);
    void HandleCombatPhaseTransition(Group* group, uint32 encounterId, uint32 newPhase);
    void CoordinateCooldownUsage(Group* group, const DungeonEncounter& encounter);

    // Performance analysis
    void AnalyzeGroupPerformance(Group* group, const DungeonEncounter& encounter);
    void AdaptStrategyBasedOnPerformance(Group* group);
    void UpdateEncounterDifficulty(uint32 encounterId, float performanceRating);
    void LogDungeonEvent(uint32 groupId, const ::std::string& event, const ::std::string& details = "");

    // Constants
    static constexpr uint32 DUNGEON_UPDATE_INTERVAL = 1000; // 1 second
    static constexpr uint32 ENCOUNTER_TIMEOUT = 1800000; // 30 minutes
    static constexpr uint32 STUCK_DETECTION_TIME = 120000; // 2 minutes
    static constexpr float THREAT_WARNING_THRESHOLD = 0.8f; // 80% of tank threat
    static constexpr float THREAT_EMERGENCY_THRESHOLD = 1.1f; // 110% of tank threat
    static constexpr uint32 WIPE_RECOVERY_TIME = 30000; // 30 seconds
    static constexpr float MIN_GROUP_HEALTH_THRESHOLD = 0.3f; // 30% health for emergencies
    static constexpr uint32 MAX_ENCOUNTER_RETRIES = 3;
    static constexpr float POSITIONING_TOLERANCE = 5.0f; // 5 yards
    static constexpr uint32 COMBAT_PREPARATION_TIME = 10000; // 10 seconds
    static constexpr float ENRAGE_WARNING_THRESHOLD = 0.1f; // 10% time remaining
};

} // namespace Playerbot