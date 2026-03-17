/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_COMBAT_STATE_ANALYZER_H
#define TRINITYCORE_COMBAT_STATE_ANALYZER_H

#include "Define.h"
#include "ObjectGuid.h"
#include "Position.h"
#include <vector>
#include <array>
#include <memory>
#include <functional>
#include <string>

class Player;
class Unit;
class Creature;
class Spell;
class SpellInfo;
class Group;

namespace Playerbot
{
    // Combat situation types for tactical decision making
    enum class CombatSituation : uint8
    {
        NORMAL          = 0,    // Standard combat, no special considerations
        AOE_HEAVY       = 1,    // Many enemies, prioritize AOE abilities
        BURST_NEEDED    = 2,    // Need high damage quickly (enrage, phase transition)
        DEFENSIVE       = 3,    // High incoming damage, prioritize survivability
        SPREAD          = 4,    // Players need to spread out (void zones, chain damage)
        STACK           = 5,    // Players need to stack up (shared damage, healing efficiency)
        KITE            = 6,    // Need to keep distance from enemies
        TANK_DEAD       = 7,    // Main tank down, emergency tanking needed
        HEALER_DEAD     = 8,    // Healer down, self-preservation priority
        WIPE_IMMINENT   = 9     // Combat going badly, consider escape or last stand
    };

    // Detailed combat metrics for analysis
    struct CombatMetrics
    {
        // Damage metrics
        float groupDPS;                     // Total group DPS
        float personalDPS;                  // Bot's personal DPS
        float incomingDPS;                  // Damage taken per second
        float burstDamage;                  // Spike damage in last 2 seconds
        uint32 lastDamageTime;              // Time of last damage taken

        // Health and resources
        float averageGroupHealth;           // Average health percentage of group
        float lowestGroupHealth;            // Lowest health member percentage
        float personalHealthPercent;        // Bot's health percentage
        float manaPercent;                  // Bot's mana percentage
        float energyPercent;                // Bot's energy/rage/etc percentage

        // Enemy information
        uint32 enemyCount;                  // Number of active enemies
        uint32 eliteCount;                  // Number of elite enemies
        uint32 bossCount;                   // Number of boss enemies
        float nearestEnemyDistance;         // Distance to nearest enemy
        float furthestEnemyDistance;        // Distance to furthest engaged enemy
        bool hasRangedEnemies;              // Are there ranged attackers

        // Positioning metrics
        float groupSpread;                  // How spread out the group is
        float distanceToTank;               // Distance to main tank
        float distanceToHealer;             // Distance to nearest healer
        bool isInMelee;                     // Currently in melee range
        bool isPositioningSafe;             // Not standing in bad stuff

        // Status flags
        bool tankAlive;                     // Is main tank alive
        bool healerAlive;                   // Is at least one healer alive
        bool hasAggro;                      // Bot has threat on something
        bool isStunned;                     // Bot is stunned/incapacitated
        bool isSilenced;                    // Bot is silenced
        bool isRooted;                      // Bot is rooted/snared

        // Timing information
        uint32 combatDuration;              // How long in combat (ms)
        uint32 timeSinceLastHeal;           // Time since last heal received (ms)
        uint32 timeSinceLastDeath;          // Time since last group member death (ms)
        uint32 enrageTimer;                 // Estimated time to enrage (ms)

        CombatMetrics() { Reset(); }
        void Reset()
        {
            groupDPS = 0.0f;
            personalDPS = 0.0f;
            incomingDPS = 0.0f;
            burstDamage = 0.0f;
            lastDamageTime = 0;
            averageGroupHealth = 100.0f;
            lowestGroupHealth = 100.0f;
            personalHealthPercent = 100.0f;
            manaPercent = 100.0f;
            energyPercent = 100.0f;
            enemyCount = 0;
            eliteCount = 0;
            bossCount = 0;
            nearestEnemyDistance = 0.0f;
            furthestEnemyDistance = 0.0f;
            hasRangedEnemies = false;
            groupSpread = 0.0f;
            distanceToTank = 0.0f;
            distanceToHealer = 0.0f;
            isInMelee = false;
            isPositioningSafe = true;
            tankAlive = true;
            healerAlive = true;
            hasAggro = false;
            isStunned = false;
            isSilenced = false;
            isRooted = false;
            combatDuration = 0;
            timeSinceLastHeal = 0;
            timeSinceLastDeath = 0;
            enrageTimer = 0;
        }
    };

    // Historical data point for trend analysis
    struct MetricsSnapshot
    {
        CombatMetrics metrics;
        uint32 timestamp;
        CombatSituation situation;
    };

    // Threat analysis data
    struct ThreatData
    {
        ObjectGuid targetGuid;
        float threatValue;
        bool isTanking;
        uint32 position;  // Position in threat list (1 = highest)

        ThreatData() : threatValue(0.0f), isTanking(false), position(0) {}
    };

    // Boss mechanic detection
    struct BossMechanic
    {
        uint32 spellId;
        ::std::string name;
        uint32 castTime;
        uint32 cooldown;
        uint32 lastSeen;
        bool requiresInterrupt;
        bool requiresMovement;
        bool requiresDefensive;
    };

    // Combat state analyzer for tactical decision making
    class CombatStateAnalyzer
    {
    public:
        explicit CombatStateAnalyzer(Player* bot);
        ~CombatStateAnalyzer();

        // Main update function
        void Update(uint32 diff);

        // Situation analysis
        CombatSituation AnalyzeSituation() const { return _currentSituation; }
        bool HasSituationChanged() const { return _situationChanged; }
        uint32 GetTimeSinceSituationChange() const { return _timeSinceSituationChange; }

        // Critical state checks
        bool IsWipeImminent() const;
        bool NeedsBurst() const;
        bool NeedsDefensive() const;
        bool NeedsEmergencyHealing() const;
        bool ShouldRetreat() const;
        bool ShouldUseConsumables() const;

        // Positioning requirements
        bool NeedsToSpread() const;
        bool NeedsToStack() const;
        bool NeedsToKite() const;
        bool NeedsToMoveOut() const;
        float GetSafeDistance() const;
        Position GetSafePosition() const;

        // Metrics access
        const CombatMetrics& GetCurrentMetrics() const { return _currentMetrics; }
        float GetMetricTrend(::std::function<float(const CombatMetrics&)> selector) const;
        bool IsMetricDeclining(::std::function<float(const CombatMetrics&)> selector, float threshold = 0.1f) const;
        bool IsMetricImproving(::std::function<float(const CombatMetrics&)> selector, float threshold = 0.1f) const;

        // Enemy analysis
        uint32 GetPriorityTargetCount() const;
        ::std::vector<Unit*> GetNearbyEnemies(float range = 40.0f) const;
        Unit* GetMostDangerousEnemy() const;
        bool HasCleaveTargets() const;
        bool ShouldFocusAdd() const;

        // Group analysis
        Player* GetLowestHealthAlly() const;
        Player* GetMainTank() const;
        Player* GetMainHealer() const;
        bool IsGroupHealthCritical() const;
        bool IsGroupManaLow() const;
        float GetGroupSurvivabilityScore() const;

        // Threat management
        ::std::vector<ThreatData> GetThreatList() const;
        bool IsAboutToGetAggro() const;
        bool ShouldDropThreat() const;
        float GetThreatPercentage(Unit* target) const;

        // Boss mechanic handling
        void RegisterBossMechanic(const BossMechanic& mechanic);
        bool IsBossMechanicIncoming(uint32& spellId, uint32& timeUntil) const;
        bool ShouldInterruptCast(Unit* caster, uint32 spellId) const;
        uint32 GetEstimatedEnrageTime() const;
        bool IsBossEnraging() const;

        // Performance metrics
        uint32 GetUpdateTime() const { return _lastUpdateTime; }
        uint32 GetAverageUpdateTime() const;
        void EnableDetailedLogging(bool enable) { _detailedLogging = enable; }

        // Historical analysis
        const ::std::array<MetricsSnapshot, 10>& GetHistory() const { return _history; }
        CombatMetrics GetAverageMetrics(uint32 periodMs = 5000) const;
        float GetDPSTrend() const;
        float GetHealthTrend() const;

        // Special case detection
        bool IsBeingKited() const;
        bool IsBeingFocused() const;
        bool IsInVoidZone() const;
        bool HasDebuffRequiringDispel() const;
        bool IsPhaseTransition() const;

        // Reset and cleanup
        void Reset();
        void ClearHistory();

    private:
        // Internal update functions
        void UpdateMetrics(uint32 diff);
        void UpdateGroupMetrics();
        void UpdateEnemyMetrics();
        void UpdatePositioningMetrics();
        void UpdateThreatData();
        void UpdateBossTimers(uint32 diff);
        void DetectBossMechanics();
        void AnalyzeCombatTrends();

        // Situation determination
        CombatSituation DetermineSituation() const;
        bool CheckForAOESituation() const;
        bool CheckForBurstNeed() const;
        bool CheckForDefensiveNeed() const;
        bool CheckForSpreadNeed() const;
        bool CheckForStackNeed() const;
        bool CheckForKiteNeed() const;
        bool CheckForTankDeath() const;
        bool CheckForHealerDeath() const;
        bool CheckForWipe() const;

        // Helper functions
        float CalculateGroupSpread() const;
        float CalculateDangerScore() const;
        bool IsUnitDangerous(Unit* unit) const;
        void RecordSnapshot();
        void PruneOldData();

        // Member variables
        Player* _bot;
        CombatMetrics _currentMetrics;
        CombatSituation _currentSituation;
        CombatSituation _previousSituation;
        bool _situationChanged;
        uint32 _timeSinceSituationChange;

        // Historical tracking
        ::std::array<MetricsSnapshot, 10> _history;
        uint32 _historyIndex;
        uint32 _lastSnapshotTime;

        // Boss mechanics tracking
        ::std::vector<BossMechanic> _knownMechanics;
        ::std::vector<uint32> _recentMechanicCasts;

        // Performance tracking
        uint32 _updateTimer;
        uint32 _lastUpdateTime;
        uint32 _totalUpdateTime;
        uint32 _updateCount;
        bool _detailedLogging;

        // Caches (refreshed each update)
        mutable ::std::vector<Unit*> _enemyCache;
        mutable uint32 _enemyCacheTime;
        mutable Player* _mainTankCache;
        mutable Player* _mainHealerCache;
        mutable uint32 _roleCacheTime;

        // Damage tracking for DPS calculation
        struct DamageRecord
        {
            uint32 timestamp;
            uint32 damage;
            ObjectGuid sourceGuid;
            ObjectGuid targetGuid;
            bool isIncoming;  // true = damage to bot, false = damage from bot
        };

        static constexpr uint32 DAMAGE_HISTORY_SIZE = 60;  // ~1 second at 60 updates
        static constexpr uint32 DPS_WINDOW_MS = 5000;      // 5-second DPS window
        ::std::array<DamageRecord, DAMAGE_HISTORY_SIZE> _damageHistory;
        uint32 _damageHistoryIndex{0};
        uint32 _lastHealthValue{0};
        uint32 _combatStartTime{0};

        // Total damage tracking for DPS calculation
        uint64 _totalDamageDealt{0};
        uint64 _totalDamageTaken{0};
        uint32 _lastDamageTrackTime{0};

        // Ground effect tracking
        struct GroundEffectInfo
        {
            uint32 spellId;
            Position center;
            float radius;
            uint32 expiryTime;
        };
        ::std::vector<GroundEffectInfo> _detectedGroundEffects;
    };

} // namespace Playerbot

#endif // TRINITYCORE_COMBAT_STATE_ANALYZER_H