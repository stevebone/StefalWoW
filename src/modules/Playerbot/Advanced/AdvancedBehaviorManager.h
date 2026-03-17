/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_BOT_ADVANCED_BEHAVIOR_MANAGER_H
#define TRINITYCORE_BOT_ADVANCED_BEHAVIOR_MANAGER_H

#include "Define.h"
#include "ObjectGuid.h"
#include "Position.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <memory>
#include <chrono>

class Player;
class Creature;
class GameObject;
class Map;
class InstanceScript;

namespace Playerbot
{
    class BotAI;

    /**
     * AdvancedBehaviorManager - Advanced PvE/PvP behaviors for PlayerBots
     *
     * Handles complex game scenarios including:
     * - Dungeon mechanics and boss fights
     * - PvP battleground strategies
     * - World event participation
     * - Achievement hunting
     */
    class AdvancedBehaviorManager
    {
    public:
        explicit AdvancedBehaviorManager(Player* bot, BotAI* ai);
        ~AdvancedBehaviorManager();

        // Core lifecycle
        void Initialize();
        void Update(uint32 diff);
        void Reset();
        void Shutdown();

        // Dungeon System
        enum class DungeonRole : uint8
        {
            TANK,
            HEALER,
            DPS,
            UNDEFINED
        };

        struct DungeonStrategy
        {
            uint32 dungeonId;
            ::std::string dungeonName;
            uint32 recommendedLevel;
            uint32 maxPlayers;
            ::std::vector<uint32> bossEntries;
            ::std::unordered_map<uint32, ::std::string> bossStrategies;
        };

        bool EnterDungeon(uint32 dungeonId);
        bool LeaveDungeon();
        bool IsInDungeon() const;
        DungeonStrategy const* GetCurrentDungeonStrategy() const;
        void ExecuteDungeonStrategy();

        // Boss mechanics
        struct BossMechanic
        {
            ::std::string mechanicName;
            ::std::string description;
            uint32 spellId;
            ::std::string response;           // How to respond
            uint32 priority;                // Priority level (1-10)
        };

        void HandleBossMechanic(Creature* boss, uint32 spellId);
        void AvoidDangerZone(Position const& center, float radius);
        void InterruptBossCast(Creature* boss, uint32 spellId);
        void DispelBossDebuff(uint32 spellId);
        void MoveToBossSafeSpot(Creature* boss);

        // Trash mechanics
        void HandleTrashPull();
        void PrioritizeCrowdControl(::std::vector<Creature*> const& mobs);
        void HandlePatrolAvoidance();

        // PvP System
        enum class BattlegroundType : uint8
        {
            NONE,
            WARSONG_GULCH,
            ARATHI_BASIN,
            ALTERAC_VALLEY,
            EYE_OF_THE_STORM,
            STRAND_OF_THE_ANCIENTS,
            ISLE_OF_CONQUEST,
            RANDOM_BG
        };

        struct BattlegroundStrategy
        {
            BattlegroundType type;
            ::std::string strategyName;
            ::std::vector<::std::string> objectives;
            ::std::unordered_map<::std::string, Position> keyLocations;
        };

        bool QueueForBattleground(BattlegroundType type);
        bool LeaveBattleground();
        bool IsInBattleground() const;
        void ExecuteBattlegroundStrategy();

        // Battleground objectives
        void DefendBase(GameObject* flag);
        void AttackBase(GameObject* flag);
        void EscortFlagCarrier(Player* carrier);
        void ReturnFlag();
        void CaptureObjective(GameObject* objective);

        // PvP combat
        void FocusPvPTarget(Player* target);
        void CallForBackup(Position const& location);
        void UseDefensiveCooldowns();
        void PrioritizeHealers();
        void PrioritizeFlagCarriers();

        // World Events
        enum class WorldEventType : uint8
        {
            NONE,
            BREWFEST,
            HALLOWS_END,
            WINTER_VEIL,
            LUNAR_FESTIVAL,
            LOVE_IS_IN_THE_AIR,
            NOBLEGARDEN,
            CHILDRENS_WEEK,
            MIDSUMMER,
            HARVEST_FESTIVAL,
            PILGRIMS_BOUNTY,
            DAY_OF_THE_DEAD,
            DARKMOON_FAIRE
        };

        struct WorldEvent
        {
            WorldEventType type;
            ::std::string eventName;
            uint32 startTime;
            uint32 endTime;
            ::std::vector<uint32> questIds;
            ::std::vector<uint32> vendorIds;
            bool isActive;
        };

        bool ParticipateInWorldEvent(WorldEventType type);
        void CompleteEventQuests(WorldEventType type);
        void VisitEventVendors(WorldEventType type);
        ::std::vector<WorldEvent> GetActiveEvents() const;

        // Achievement System
        struct Achievement
        {
            uint32 achievementId;
            ::std::string name;
            ::std::string description;
            uint32 points;
            ::std::vector<::std::string> criteria;
            bool isCompleted;
        };

        void PursueAchievement(uint32 achievementId);
        ::std::vector<Achievement> GetPursuitAchievements() const;
        void CheckAchievementProgress(uint32 achievementId);
        void PrioritizeAchievements();

        // Exploration and Discovery
        void ExploreZone(uint32 zoneId);
        void DiscoverFlightPaths();
        void FindRareSpawns();
        void CollectTreasures();

        // Rare Spawns
        struct RareSpawn
        {
            uint32 entry;
            ::std::string name;
            Position lastKnownPosition;
            uint32 respawnTime;
            uint32 lastSeenTime;
            bool isElite;
            uint32 level;
        };

        void TrackRareSpawn(Creature* rare);
        ::std::vector<RareSpawn> GetTrackedRares() const;
        bool ShouldEngageRare(Creature* rare) const;

        // Treasure Hunting
        struct Treasure
        {
            ObjectGuid guid;
            uint32 entry;
            Position position;
            uint32 discoveredTime;
            bool isLooted;
        };

        void FindNearbyTreasures();
        bool LootTreasure(GameObject* treasure);
        ::std::vector<Treasure> GetDiscoveredTreasures() const;

        // Mount and Pet Collection
        void CollectMounts();
        void CollectPets();
        void BattlePets();

        // Configuration
        bool IsEnabled() const { return m_enabled; }
        void SetEnabled(bool enabled) { m_enabled = enabled; }

        void SetDungeonEnabled(bool enable) { m_dungeonEnabled = enable; }
        void SetPvPEnabled(bool enable) { m_pvpEnabled = enable; }
        void SetEventEnabled(bool enable) { m_eventEnabled = enable; }
        void SetAchievementHunting(bool enable) { m_achievementHunting = enable; }
        void SetRareHunting(bool enable) { m_rareHunting = enable; }

        // Statistics
        struct Statistics
        {
            uint32 dungeonsCompleted = 0;
            uint32 bossesKilled = 0;
            uint32 battlegroundsWon = 0;
            uint32 battlegroundsLost = 0;
            uint32 objectivesCaptured = 0;
            uint32 eventsParticipated = 0;
            uint32 achievementsEarned = 0;
            uint32 raresKilled = 0;
            uint32 treasuresLooted = 0;
        };

        Statistics const& GetStatistics() const { return m_stats; }

        // Performance metrics
        float GetCPUUsage() const { return m_cpuUsage; }
        size_t GetMemoryUsage() const;

    private:
        // Dungeon logic
        void UpdateDungeonBehavior(uint32 diff);
        void LoadDungeonStrategies();
        DungeonStrategy* GetDungeonStrategy(uint32 dungeonId);
        void AnalyzeDungeonComposition();
        void AssignDungeonRole();

        // Boss fight coordination
        struct ActiveBossFight
        {
            Creature* boss;
            uint32 bossEntry;
            uint32 startTime;
            uint32 phase;
            ::std::vector<BossMechanic> activeMechanics;
        };

        ::std::unique_ptr<ActiveBossFight> m_currentBossFight;
        void StartBossFight(Creature* boss);
        void EndBossFight(bool victory);
        void UpdateBossFight(uint32 diff);
        void AdvanceBossPhase();

        // PvP logic
        void UpdatePvPBehavior(uint32 diff);
        void LoadBattlegroundStrategies();
        BattlegroundStrategy* GetBattlegroundStrategy(BattlegroundType type);
        void AnalyzeBattlegroundSituation();
        Player* SelectPvPTarget();

        // Event logic
        void UpdateEventBehavior(uint32 diff);
        void LoadWorldEvents();
        void UpdateEventStatus();
        bool IsEventActive(WorldEventType type) const;

        // Achievement logic
        void UpdateAchievementProgress(uint32 diff);
        ::std::vector<uint32> m_pursuingAchievements;
        void CalculateAchievementPriority();

        // Exploration logic
        void UpdateExploration(uint32 diff);
        ::std::unordered_set<uint32> m_exploredZones;
        ::std::unordered_set<uint32> m_discoveredFlightPaths;

        // Rare spawn tracking
        ::std::unordered_map<uint32, RareSpawn> m_trackedRares;
        void UpdateRareTracking(uint32 diff);
        void ScanForRares();

        // Treasure tracking
        ::std::vector<Treasure> m_discoveredTreasures;
        void UpdateTreasureHunting(uint32 diff);
        void ScanForTreasures();

        // Danger zone tracking
        struct DangerZone
        {
            Position center;
            float radius;
            uint32 expiryTime;
            uint32 damagePerSecond;
        };

        ::std::vector<DangerZone> m_dangerZones;
        void UpdateDangerZones(uint32 diff);
        bool IsInDangerZone(Position const& pos) const;
        Position FindSafePosition(Position const& currentPos) const;

        // Statistics tracking
        void RecordDungeonComplete();
        void RecordBossKill(Creature* boss);
        void RecordBattlegroundResult(bool victory);
        void RecordObjectiveCapture();
        void RecordEventParticipation(WorldEventType type);
        void RecordAchievement(uint32 achievementId);
        void RecordRareKill(Creature* rare);
        void RecordTreasureLoot(GameObject* treasure);

        // Performance tracking
        void StartPerformanceTimer();
        void EndPerformanceTimer();
        void UpdatePerformanceMetrics();

    private:
        Player* m_bot;
        BotAI* m_ai;
        bool m_enabled;

        // Configuration
        bool m_dungeonEnabled;
        bool m_pvpEnabled;
        bool m_eventEnabled;
        bool m_achievementHunting;
        bool m_rareHunting;

        // Current state
        DungeonRole m_dungeonRole;
        BattlegroundType m_currentBattleground;
        WorldEventType m_activeEvent;

        // Strategy databases
        ::std::unordered_map<uint32, DungeonStrategy> m_dungeonStrategies;
        ::std::unordered_map<BattlegroundType, BattlegroundStrategy> m_bgStrategies;
        ::std::vector<WorldEvent> m_worldEvents;

        // Update intervals
        uint32 m_dungeonUpdateInterval;
        uint32 m_pvpUpdateInterval;
        uint32 m_eventUpdateInterval;
        uint32 m_achievementUpdateInterval;
        uint32 m_explorationUpdateInterval;
        uint32 m_rareUpdateInterval;

        // Last update times
        uint32 m_lastDungeonUpdate;
        uint32 m_lastPvPUpdate;
        uint32 m_lastEventUpdate;
        uint32 m_lastAchievementUpdate;
        uint32 m_lastExplorationUpdate;
        uint32 m_lastRareUpdate;

        // Statistics
        Statistics m_stats;

        // Performance metrics
        ::std::chrono::high_resolution_clock::time_point m_performanceStart;
        ::std::chrono::microseconds m_lastUpdateDuration;
        ::std::chrono::microseconds m_totalUpdateTime;
        uint32 m_updateCount;
        float m_cpuUsage;
    };

} // namespace Playerbot

#endif // TRINITYCORE_BOT_ADVANCED_BEHAVIOR_MANAGER_H
