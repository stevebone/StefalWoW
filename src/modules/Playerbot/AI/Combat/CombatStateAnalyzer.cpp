/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "CombatStateAnalyzer.h"
#include "Player.h"
#include "Group.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellHistory.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "Timer.h"
#include "GameTime.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "Unit.h"
#include "Creature.h"
#include "ObjectMgr.h"
#include "MotionMaster.h"
#include "Log.h"
#include <algorithm>
#include <numeric>
#include "../../Spatial/SpatialGridManager.h"
#include "ObjectAccessor.h"

namespace Playerbot
{

CombatStateAnalyzer::CombatStateAnalyzer(Player* bot) :
    _bot(bot),
    _currentSituation(CombatSituation::NORMAL),
    _previousSituation(CombatSituation::NORMAL),
    _situationChanged(false),
    _timeSinceSituationChange(0),
    _historyIndex(0),
    _lastSnapshotTime(0),
    _updateTimer(0),
    _lastUpdateTime(0),
    _totalUpdateTime(0),
    _updateCount(0),
    _detailedLogging(false),
    _enemyCacheTime(0),
    _mainTankCache(nullptr),
    _mainHealerCache(nullptr),
    _roleCacheTime(0)
{
    _currentMetrics.Reset();
    ClearHistory();
}

CombatStateAnalyzer::~CombatStateAnalyzer() = default;

void CombatStateAnalyzer::Update(uint32 diff)
{
    uint32 startTime = GameTime::GetGameTimeMS();

    _updateTimer += diff;
    _timeSinceSituationChange += diff;

    // Update metrics every 100ms for responsiveness
    if (_updateTimer >= 100)
    {
        UpdateMetrics(_updateTimer);
        _updateTimer = 0;

        // Record snapshot every 500ms for trend analysis
    if (GameTime::GetGameTimeMS() - _lastSnapshotTime >= 500)
        {
            RecordSnapshot();
            _lastSnapshotTime = GameTime::GetGameTimeMS();
        }

        // Analyze situation
        CombatSituation newSituation = DetermineSituation();
        if (newSituation != _currentSituation)
        {
            _previousSituation = _currentSituation;
            _currentSituation = newSituation;
            _situationChanged = true;
            _timeSinceSituationChange = 0;

            if (_detailedLogging)
                TC_LOG_DEBUG("bot.playerbot", "Combat situation changed from {} to {} for bot {}",
                                                            static_cast<uint32>(_previousSituation), static_cast<uint32>(_currentSituation), _bot->GetName());
        }
        else
        {
            _situationChanged = false;
        }

        // Analyze trends
        AnalyzeCombatTrends();

        // Update boss mechanics
        DetectBossMechanics();
        UpdateBossTimers(diff);
    }

    // Track performance
    _lastUpdateTime = GameTime::GetGameTimeMS() - startTime;
    _totalUpdateTime += _lastUpdateTime;
    _updateCount++;

    // Prune old data periodically
    if (_updateCount % 100 == 0)
                PruneOldData();
}

void CombatStateAnalyzer::UpdateMetrics(uint32 diff)
{
        if (!_bot || !_bot->IsInCombat())
    {
        _currentMetrics.Reset();
        return;
    }

    // Update combat duration
    _currentMetrics.combatDuration += diff;

    // Update personal metrics
    _currentMetrics.personalHealthPercent = _bot->GetHealthPct();
    _currentMetrics.manaPercent = _bot->GetPowerPct(POWER_MANA);

    Powers powerType = _bot->GetPowerType();
    if (powerType != POWER_MANA)
        _currentMetrics.energyPercent = _bot->GetPowerPct(powerType);

    // Check debuffs
    _currentMetrics.isStunned = _bot->HasUnitState(UNIT_STATE_STUNNED);
    _currentMetrics.isSilenced = _bot->IsSilenced(SPELL_SCHOOL_MASK_MAGIC);
    _currentMetrics.isRooted = _bot->HasUnitState(UNIT_STATE_ROOT);
        // Update group metrics
    UpdateGroupMetrics();

    // Update enemy metrics
    UpdateEnemyMetrics();

    // Update positioning metrics
    UpdatePositioningMetrics();

    // Update threat data
    UpdateThreatData();

    // Calculate DPS metrics using damage tracking
    uint32 now = GameTime::GetGameTimeMS();

    // Track damage taken based on health changes
    uint32 currentHealth = _bot->GetHealth();
    if (_lastHealthValue > 0 && currentHealth < _lastHealthValue)
    {
        uint32 damageTaken = _lastHealthValue - currentHealth;
        _totalDamageTaken += damageTaken;
        _currentMetrics.lastDamageTime = now;

        // Record in damage history
        DamageRecord& record = _damageHistory[_damageHistoryIndex % DAMAGE_HISTORY_SIZE];
        record.timestamp = now;
        record.damage = damageTaken;
        record.targetGuid = _bot->GetGUID();
        record.isIncoming = true;
        _damageHistoryIndex++;
    }
    _lastHealthValue = currentHealth;

    // Calculate DPS over the window period
    if (_combatStartTime == 0)
        _combatStartTime = now;

    uint32 combatDuration = now - _combatStartTime;
    if (combatDuration > 0)
    {
        // Personal DPS - estimate from target health changes
        float personalDamageDealt = 0.0f;
        if (Unit* target = _bot->GetVictim())
        {
            // Track enemy health changes as our damage contribution
            // This is an estimate assuming we're the primary damage dealer
            personalDamageDealt = static_cast<float>(_totalDamageDealt);
        }

        float durationSeconds = static_cast<float>(::std::min(combatDuration, DPS_WINDOW_MS)) / 1000.0f;
        if (durationSeconds > 0.0f)
        {
            _currentMetrics.personalDPS = personalDamageDealt / durationSeconds;
            _currentMetrics.incomingDPS = static_cast<float>(_totalDamageTaken) / durationSeconds;
        }

        // Group DPS - sum of all group members' estimated DPS
        _currentMetrics.groupDPS = _currentMetrics.personalDPS;  // Start with personal
        if (Group* group = _bot->GetGroup())
        {
            for (GroupReference const& groupRef : group->GetMembers())
            {
                if (Player* member = groupRef.GetSource())
                {
                    if (member != _bot && member->IsInCombat())
                    {
                        // Estimate member DPS based on their primary stat and level
                        // This is a heuristic since we can't track their actual damage
                        float memberDPS = member->GetTotalAttackPowerValue(BASE_ATTACK) * 0.5f;
                        _currentMetrics.groupDPS += memberDPS;
                    }
                }
            }
        }
    }
}

void CombatStateAnalyzer::UpdateGroupMetrics()
{
    Group* group = _bot->GetGroup();
        if (!group)
    {
        _currentMetrics.averageGroupHealth = _currentMetrics.personalHealthPercent;
                _currentMetrics.lowestGroupHealth = _currentMetrics.personalHealthPercent;
        _currentMetrics.tankAlive = true;
        _currentMetrics.healerAlive = true;
        return;
    }

    float totalHealth = 0.0f;
    float lowestHealth = 100.0f;
    uint32 memberCount = 0;
        bool hasTank = false;
    bool hasHealer = false;

    for (GroupReference const& groupRef : group->GetMembers())
    {
        Player* member = groupRef.GetSource();
                if (!member || !member->IsAlive())
            continue;

        float healthPct = member->GetHealthPct();
        totalHealth += healthPct;
        lowestHealth = ::std::min(lowestHealth, healthPct);
        memberCount++;

        // Simple role detection based on class
        Classes memberClass = static_cast<Classes>(member->GetClass());
                switch (memberClass)
        {
            case CLASS_WARRIOR:
            case CLASS_DEATH_KNIGHT:
                hasTank = true;
                break;
            case CLASS_PALADIN:
                hasTank = true;
                hasHealer = true;
                break;
            case CLASS_PRIEST:
            case CLASS_DRUID:
            case CLASS_SHAMAN:
                hasHealer = true;
                break;
            default:
                break;
        }
    }

    if (memberCount > 0)
    {
        _currentMetrics.averageGroupHealth = totalHealth / memberCount;
        _currentMetrics.lowestGroupHealth = lowestHealth;
        }

    _currentMetrics.tankAlive = hasTank;
    _currentMetrics.healerAlive = hasHealer;

    // Calculate group spread
    _currentMetrics.groupSpread = CalculateGroupSpread();
}

void CombatStateAnalyzer::UpdateEnemyMetrics()
{
    _currentMetrics.enemyCount = 0;
    _currentMetrics.eliteCount = 0;
    _currentMetrics.bossCount = 0;
    _currentMetrics.nearestEnemyDistance = 100.0f;
    _currentMetrics.furthestEnemyDistance = 0.0f;
    _currentMetrics.hasRangedEnemies = false;

    // Clear enemy cache if too old
    if (GameTime::GetGameTimeMS() - _enemyCacheTime > 500)
    {
        _enemyCache.clear();
        _enemyCacheTime = GameTime::GetGameTimeMS();
                            // Lock-free spatial grid query
        Map* map = _bot->GetMap();
                                    if (map)
                {
            DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
                        if (!spatialGrid)
            {
                // Create grid on demand
                sSpatialGridManager.CreateGrid(map);
                spatialGrid = sSpatialGridManager.GetGrid(map);
                        }

            if (spatialGrid)
            {
                // Query nearby creature GUIDs (lock-free!)
                ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
                    _bot->GetPosition(), 50.0f);

                // SPATIAL GRID MIGRATION COMPLETE (2025-11-26):
                // ObjectAccessor is intentionally retained because we need:
                // 1. Real-time IsAlive() check (creature may have died since snapshot)
                // 2. IsInCombatWith() for live combat state verification
                // 3. ToCreature() for creature-specific checks (IsElite, IsDungeonBoss)
                // The spatial grid pre-filters candidates to reduce ObjectAccessor calls.
    for (ObjectGuid guid : nearbyGuids)
                {
                    Unit* enemy = ObjectAccessor::GetUnit(*_bot, guid);
                                                            if (!enemy || !enemy->IsAlive() || !enemy->IsInCombatWith(_bot))
                        continue;

                    if (!_bot->IsHostileTo(enemy))
                        continue;

                    _enemyCache.push_back(enemy);
                    _currentMetrics.enemyCount++;

                    float distance = ::std::sqrt(_bot->GetExactDistSq(enemy)); // Calculate once from squared distance
                    _currentMetrics.nearestEnemyDistance = ::std::min(_currentMetrics.nearestEnemyDistance, distance);
                    _currentMetrics.furthestEnemyDistance = ::std::max(_currentMetrics.furthestEnemyDistance, distance);

                    if (enemy->GetTypeId() == TYPEID_UNIT)
                                        {
                        Creature* creature = enemy->ToCreature();
                                                                                                                        if (creature->IsElite())
                            _currentMetrics.eliteCount++;
                        if (creature->IsDungeonBoss())
                            _currentMetrics.bossCount++;
                    }

                    // Check if enemy is ranged (simplified check)
    if (distance > 10.0f && enemy->IsInCombatWith(_bot))
                        _currentMetrics.hasRangedEnemies = true;
                }
            }
        }
    }
    else
    {
        // Use cached data
        _currentMetrics.enemyCount = _enemyCache.size();
        for (Unit* enemy : _enemyCache)
                    {
            if (!enemy || !enemy->IsAlive())
                continue;

            float distance = ::std::sqrt(_bot->GetExactDistSq(enemy)); // Calculate once from squared distance
            _currentMetrics.nearestEnemyDistance = ::std::min(_currentMetrics.nearestEnemyDistance, distance);
            _currentMetrics.furthestEnemyDistance = ::std::max(_currentMetrics.furthestEnemyDistance, distance);
        }
    }

    _currentMetrics.isInMelee = (_currentMetrics.nearestEnemyDistance <= 5.0f);
}

void CombatStateAnalyzer::UpdatePositioningMetrics()
{
    // Update distances to key roles
    Player* tank = GetMainTank();
    Player* healer = GetMainHealer();

    if (tank && tank != _bot)
        _currentMetrics.distanceToTank = ::std::sqrt(_bot->GetExactDistSq(tank)); // Calculate once from squared distance
    else
        _currentMetrics.distanceToTank = 0.0f;

    if (healer && healer != _bot)
        _currentMetrics.distanceToHealer = ::std::sqrt(_bot->GetExactDistSq(healer)); // Calculate once from squared distance
    else
        _currentMetrics.distanceToHealer = 0.0f;

    // Check if position is safe (simplified - would need actual mechanic detection)
    _currentMetrics.isPositioningSafe = !IsInVoidZone();
}

void CombatStateAnalyzer::UpdateThreatData()
{
    _currentMetrics.hasAggro = false;

    // Check threat on all enemies
    for (Unit* enemy : _enemyCache)
            {
        if (!enemy || !enemy->IsAlive())
            continue;

        if (enemy->GetTarget() == _bot->GetGUID())
                {
            _currentMetrics.hasAggro = true;
            break;
        }
    }
}

void CombatStateAnalyzer::UpdateBossTimers(uint32 /*diff*/)
{
    // Update known mechanic cooldowns
    for (BossMechanic& mechanic : _knownMechanics)
    {
        if (mechanic.lastSeen > 0 && GameTime::GetGameTimeMS() - mechanic.lastSeen < mechanic.cooldown)
        {
            // Mechanic is on cooldown
            continue;
        }
    }

    // Simple enrage timer estimation (would need actual boss data)
    if (_currentMetrics.bossCount > 0 && _currentMetrics.combatDuration > 0)
    {
                // Assume 10 minute enrage timer for bosses
        uint32 typicalEnrageTime = 10 * 60 * 1000; // 10 minutes in ms
    if (_currentMetrics.combatDuration < typicalEnrageTime)
            _currentMetrics.enrageTimer = typicalEnrageTime - _currentMetrics.combatDuration;
        else
            _currentMetrics.enrageTimer = 0;
    }
}

void CombatStateAnalyzer::DetectBossMechanics()
{
    // Scan for boss spell casts and detect dangerous mechanics
    uint32 now = GameTime::GetGameTimeMS();

    // Clean up expired ground effects
    _detectedGroundEffects.erase(
        std::remove_if(_detectedGroundEffects.begin(), _detectedGroundEffects.end(),
            [now](const GroundEffectInfo& effect) { return now > effect.expiryTime; }),
        _detectedGroundEffects.end()
    );

    // Scan enemies for dangerous spell casts
    for (Unit* enemy : _enemyCache)
    {
        if (!enemy || enemy->GetTypeId() != TYPEID_UNIT)
            continue;

        Creature* creature = enemy->ToCreature();
        if (!creature)
            continue;

        // Check if creature is casting
        if (Spell* currentSpell = creature->GetCurrentSpell(CURRENT_GENERIC_SPELL))
        {
            SpellInfo const* spellInfo = currentSpell->GetSpellInfo();
            if (!spellInfo)
                continue;

            // Check for area effect spells (potential void zones)
            bool isAreaEffect = spellInfo->HasEffect(SPELL_EFFECT_PERSISTENT_AREA_AURA) ||
                               spellInfo->HasEffect(SPELL_EFFECT_APPLY_AREA_AURA_ENEMY) ||
                               spellInfo->HasEffect(SPELL_EFFECT_CREATE_AREATRIGGER);

            if (isAreaEffect)
            {
                // Register as a known mechanic if from a boss
                if (creature->IsDungeonBoss())
                {
                    bool alreadyKnown = false;
                    for (const auto& mechanic : _knownMechanics)
                    {
                        if (mechanic.spellId == spellInfo->Id)
                        {
                            alreadyKnown = true;
                            break;
                        }
                    }

                    if (!alreadyKnown)
                    {
                        BossMechanic mechanic;
                        mechanic.spellId = spellInfo->Id;
                        mechanic.name = std::string((*spellInfo->SpellName)[LOCALE_enUS]);
                        mechanic.castTime = spellInfo->CastTimeEntry ? spellInfo->CastTimeEntry->Base : 0;
                        mechanic.cooldown = 0;  // Would need to track from usage patterns
                        mechanic.lastSeen = now;
                        mechanic.requiresMovement = isAreaEffect;
                        mechanic.requiresInterrupt = spellInfo->HasChannelInterruptFlag(SpellAuraInterruptFlags::Action);
                        mechanic.requiresDefensive = spellInfo->HasEffect(SPELL_EFFECT_SCHOOL_DAMAGE);
                        _knownMechanics.push_back(mechanic);
                    }
                }
            }

            // Check for interruptible dangerous casts
            if (spellInfo->HasChannelInterruptFlag(SpellAuraInterruptFlags::Action) && spellInfo->HasEffect(SPELL_EFFECT_SCHOOL_DAMAGE))
            {
                // Track recent mechanic casts for pattern detection
                _recentMechanicCasts.push_back(spellInfo->Id);
                if (_recentMechanicCasts.size() > 20)
                    _recentMechanicCasts.erase(_recentMechanicCasts.begin());
            }
        }
    }
}

void CombatStateAnalyzer::AnalyzeCombatTrends()
{
    // Analyze health trend over last 5 seconds
    if (_historyIndex < 5)
                return; // Not enough data

        float healthTrend = 0.0f;
        float dpsTrend = 0.0f;

    for (uint32 i = 0; i < 5; ++i)
    {
        uint32 idx = (_historyIndex - i) % 10;
        healthTrend += _history[idx].metrics.averageGroupHealth;
        dpsTrend += _history[idx].metrics.groupDPS;
    }

    healthTrend /= 5.0f;
    dpsTrend /= 5.0f;

    // Store trends for use in decision making
    // These would be member variables in production
}

CombatSituation CombatStateAnalyzer::DetermineSituation() const
{
    // Priority order for situation determination
    if (CheckForWipe())
        return CombatSituation::WIPE_IMMINENT;

    if (CheckForTankDeath())
        return CombatSituation::TANK_DEAD;

    if (CheckForHealerDeath())
        return CombatSituation::HEALER_DEAD;

    if (CheckForKiteNeed())
        return CombatSituation::KITE;

    if (CheckForDefensiveNeed())
        return CombatSituation::DEFENSIVE;

    if (CheckForBurstNeed())
        return CombatSituation::BURST_NEEDED;

    if (CheckForSpreadNeed())
        return CombatSituation::SPREAD;

    if (CheckForStackNeed())
        return CombatSituation::STACK;

    if (CheckForAOESituation())
        return CombatSituation::AOE_HEAVY;

    return CombatSituation::NORMAL;
}

bool CombatStateAnalyzer::CheckForAOESituation() const
{
    // AOE situation if 4+ enemies or 3+ in melee range
    if (_currentMetrics.enemyCount >= 4)
        return true;

    uint32 meleeCount = 0;
    for (Unit* enemy : _enemyCache)
    {
        if (enemy && enemy->IsAlive() && _bot->GetExactDistSq(enemy) <= (8.0f * 8.0f)) // 64.0f
                    meleeCount++;
    }

    return meleeCount >= 3;
}

bool CombatStateAnalyzer::CheckForBurstNeed() const
{
    // Need burst if boss is enraging soon or health is getting low
    if (_currentMetrics.enrageTimer > 0 && _currentMetrics.enrageTimer < 30000)
        return true;

    // Need burst if boss is below 30% (execute phase)
    for (Unit* enemy : _enemyCache)
            {
        if (!enemy || !enemy->IsAlive())
            continue;

        if (enemy->GetTypeId() == TYPEID_UNIT)
        {
            Creature* creature = enemy->ToCreature();
            if (creature->IsDungeonBoss() && creature->GetHealthPct() < 30.0f)
                return true;
        }
    }

    return false;
}

bool CombatStateAnalyzer::CheckForDefensiveNeed() const
{
    // Defensive if taking heavy damage or health is low
    if (_currentMetrics.personalHealthPercent < 40.0f)
        return true;

    if (_currentMetrics.averageGroupHealth < 50.0f)
        return true;

    if (_currentMetrics.incomingDPS > 0 &&
        _currentMetrics.personalHealthPercent < 70.0f)
        return true;

    return false;
}

bool CombatStateAnalyzer::CheckForSpreadNeed() const
{
    // Check for spread mechanics based on enemy spell casts and group proximity
    // Spread is needed when:
    // 1. Group is clustered together
    // 2. Enemy is casting an AOE spell
    // 3. Multiple group members are being hit by chain damage

    if (_currentMetrics.groupSpread < 5.0f && _currentMetrics.enemyCount > 0)
    {
        // Check for area effect spells being cast
        for (Unit* enemy : _enemyCache)
        {
            if (!enemy || enemy->GetTypeId() != TYPEID_UNIT)
                continue;

            Creature* creature = enemy->ToCreature();
            if (!creature)
                continue;

            // Check if creature is casting an AOE spell
            if (Spell* currentSpell = creature->GetCurrentSpell(CURRENT_GENERIC_SPELL))
            {
                SpellInfo const* spellInfo = currentSpell->GetSpellInfo();
                if (spellInfo)
                {
                    // Check for area effect targeting
                    if (spellInfo->HasEffect(SPELL_EFFECT_SCHOOL_DAMAGE) ||
                        spellInfo->HasEffect(SPELL_EFFECT_PERSISTENT_AREA_AURA) ||
                        spellInfo->HasEffect(SPELL_EFFECT_APPLY_AREA_AURA_ENEMY))
                    {
                        // Check if this targets multiple people (chain or area)
                        for (SpellEffectInfo const& effect : spellInfo->GetEffects())
                        {
                            if (effect.TargetA.GetTarget() == TARGET_UNIT_TARGET_ENEMY &&
                                effect.ChainTargets > 1)
                            {
                                return true;  // Chain spell - need to spread
                            }
                            // Check TargetA and TargetB radius entries for large AOE
                            if ((effect.TargetARadiusEntry && effect.TargetARadiusEntry->RadiusMax > 5.0f) ||
                                (effect.TargetBRadiusEntry && effect.TargetBRadiusEntry->RadiusMax > 5.0f))
                            {
                                return true;  // Large AOE - need to spread
                            }
                        }
                    }
                }
            }
        }
    }

    // Also spread if we detect multiple group members taking simultaneous damage
    // (indicates cleave or chain mechanics)
    if (_currentMetrics.groupSpread < 8.0f &&
        _currentMetrics.averageGroupHealth < 80.0f &&
        _currentMetrics.incomingDPS > 0)
    {
        // Check if multiple members took recent damage
        uint32 recentlyDamagedCount = 0;
        uint32 now = GameTime::GetGameTimeMS();
        if (Group* group = _bot->GetGroup())
        {
            for (GroupReference const& groupRef : group->GetMembers())
            {
                if (Player* member = groupRef.GetSource())
                {
                    // If member health dropped recently, count them
                    if (member->GetHealthPct() < 95.0f && member->IsInCombat())
                        recentlyDamagedCount++;
                }
            }
        }
        // If 3+ members are taking damage while clustered, spread out
        if (recentlyDamagedCount >= 3)
            return true;
    }

    return false;
}

bool CombatStateAnalyzer::CheckForStackNeed() const
{
    // Check for stack mechanics based on healer efficiency and group spread
    // Stack is needed when:
    // 1. Group is too spread out for efficient healing
    // 2. Healer mana is low and needs to conserve with group heals
    // 3. Boss mechanic requires stacking (shared damage)

    // Stack if group is too spread and healer is struggling
    if (_currentMetrics.groupSpread > 15.0f && _currentMetrics.averageGroupHealth < 70.0f)
    {
        return true;
    }

    // Stack if healer is low on mana - group heals are more efficient when stacked
    if (_currentMetrics.healerAlive && _currentMetrics.groupSpread > 10.0f)
    {
        // Check healer mana
        if (Player* healer = GetMainHealer())
        {
            if (healer->GetPower(POWER_MANA) * 100 / healer->GetMaxPower(POWER_MANA) < 40)
            {
                return true;  // Low mana healer - stack for efficient group heals
            }
        }
    }

    // Check for shared damage mechanics (boss casting damage split spell)
    for (Unit* enemy : _enemyCache)
    {
        if (!enemy || enemy->GetTypeId() != TYPEID_UNIT)
            continue;

        Creature* creature = enemy->ToCreature();
        if (!creature || !creature->IsDungeonBoss())
            continue;

        if (Spell* currentSpell = creature->GetCurrentSpell(CURRENT_GENERIC_SPELL))
        {
            SpellInfo const* spellInfo = currentSpell->GetSpellInfo();
            if (spellInfo)
            {
                // Check for damage split/shared damage mechanics
                for (SpellEffectInfo const& effect : spellInfo->GetEffects())
                {
                    // Shared damage mechanics often have specific aura types
                    if (effect.ApplyAuraName == SPELL_AURA_SPLIT_DAMAGE_PCT ||
                        effect.ApplyAuraName == SPELL_AURA_SHARE_DAMAGE_PCT)
                    {
                        return true;  // Damage split mechanic - stack up
                    }
                }
            }
        }
    }

    return false;
}

bool CombatStateAnalyzer::CheckForKiteNeed() const
{
    // Kite if we have aggro and shouldn't tank
    Classes botClass = static_cast<Classes>(_bot->GetClass());
        bool canTank = (botClass == CLASS_WARRIOR || botClass == CLASS_PALADIN ||
                    botClass == CLASS_DEATH_KNIGHT || botClass == CLASS_DRUID);

    if (!canTank && _currentMetrics.hasAggro && _currentMetrics.enemyCount > 0)
        return true;

    // Kite if enemy is too dangerous in melee
    if (_currentMetrics.isInMelee && _currentMetrics.personalHealthPercent < 30.0f)
        return true;

    return false;
}

bool CombatStateAnalyzer::CheckForTankDeath() const
{
    // Tank is dead if no tank alive and we have elite/boss enemies
    return !_currentMetrics.tankAlive && (_currentMetrics.eliteCount > 0 || _currentMetrics.bossCount > 0);
}

bool CombatStateAnalyzer::CheckForHealerDeath() const
{
    // Healer dead situation
    return !_currentMetrics.healerAlive && _currentMetrics.averageGroupHealth < 60.0f;
}

bool CombatStateAnalyzer::CheckForWipe() const
{
    // Wipe imminent if most of group is dead or about to die
    if (_currentMetrics.averageGroupHealth < 20.0f)
        return true;

    // Wipe if tank dead and boss still has high health
    if (!_currentMetrics.tankAlive && _currentMetrics.bossCount > 0)
    {
        for (Unit* enemy : _enemyCache)
        {
            if (enemy && enemy->GetTypeId() == TYPEID_UNIT)
            {
                Creature* creature = enemy->ToCreature();
                if (creature && creature->IsDungeonBoss() && creature->GetHealthPct() > 50.0f)
                    return true;
            }
        }
    }

    return false;
}

bool CombatStateAnalyzer::IsWipeImminent() const
{
    return _currentSituation == CombatSituation::WIPE_IMMINENT;
}

bool CombatStateAnalyzer::NeedsBurst() const
{
    return _currentSituation == CombatSituation::BURST_NEEDED ||
           (_currentMetrics.enrageTimer > 0 && _currentMetrics.enrageTimer < 20000);
}

bool CombatStateAnalyzer::NeedsDefensive() const
{
    return _currentSituation == CombatSituation::DEFENSIVE ||
           _currentMetrics.personalHealthPercent < 50.0f ||
           (_currentMetrics.hasAggro && _currentMetrics.eliteCount > 0);
}

bool CombatStateAnalyzer::NeedsEmergencyHealing() const
{
    return _currentMetrics.personalHealthPercent < 30.0f ||
           _currentMetrics.lowestGroupHealth < 25.0f;
}

bool CombatStateAnalyzer::ShouldRetreat() const
{
    return IsWipeImminent() ||
           (_currentMetrics.personalHealthPercent < 20.0f && !_currentMetrics.healerAlive);
}

bool CombatStateAnalyzer::ShouldUseConsumables() const
{
    // Use consumables in critical situations or boss fights
    return IsWipeImminent() || NeedsBurst() ||
                      (_currentMetrics.bossCount > 0 && _currentMetrics.averageGroupHealth < 50.0f);
}

bool CombatStateAnalyzer::NeedsToSpread() const
{
    return _currentSituation == CombatSituation::SPREAD;
}

bool CombatStateAnalyzer::NeedsToStack() const
{
    return _currentSituation == CombatSituation::STACK;
}

bool CombatStateAnalyzer::NeedsToKite() const
{
    return _currentSituation == CombatSituation::KITE;
}

bool CombatStateAnalyzer::NeedsToMoveOut() const
{
    // Move out if in danger zone or spreading
    return !_currentMetrics.isPositioningSafe || NeedsToSpread();
}

float CombatStateAnalyzer::GetSafeDistance() const
{
    if (NeedsToSpread())
        return 10.0f; // Spread distance
    if (NeedsToStack())
        return 3.0f; // Stack distance
    if (NeedsToKite())
        return 20.0f; // Kite distance

        return 5.0f; // Default safe distance
}

Position CombatStateAnalyzer::GetSafePosition() const
{
    // Calculate safe position based on situation
    Position pos;
    pos.Relocate(_bot->GetPositionX(), _bot->GetPositionY(), _bot->GetPositionZ(), _bot->GetOrientation());

    if (NeedsToSpread())
    {
        // Move away from group center
    if (Group* group = _bot->GetGroup())
                {
            float centerX = 0, centerY = 0, centerZ = 0;
            uint32 count = 0;

            for (GroupReference const& groupRef : group->GetMembers())
            {
                if (Player* member = groupRef.GetSource())
                {
                    if (member != _bot && member->IsAlive())
                                        {
                        Position memberPos = member->GetPosition();
                                                centerX += memberPos.GetPositionX();
                        centerY += memberPos.GetPositionY();
                        centerZ += memberPos.GetPositionZ();
                                        count++;
                    }
                }
            }

            if (count > 0)
            {
                centerX /= count;
                centerY /= count;
                centerZ /= count;

                // Move away from center
                float angle = _bot->GetRelativeAngle(centerX, centerY);
                float newX = _bot->GetPositionX() + cos(angle + static_cast<float>(M_PI)) * 10.0f;
                                float newY = _bot->GetPositionY() + sin(angle + static_cast<float>(M_PI)) * 10.0f;
                                                            pos.Relocate(newX, newY, _bot->GetPositionZ());
            }
        }
    }
    else if (NeedsToStack())
    {
        // Move to tank position
    if (Player* tank = GetMainTank())
                            {
            pos.Relocate(tank->GetPositionX(), tank->GetPositionY(), tank->GetPositionZ());
        }
        }

    return pos;
}

float CombatStateAnalyzer::GetMetricTrend(::std::function<float(const CombatMetrics&)> selector) const
{
    if (_historyIndex < 2)
        return 0.0f;

    float recent = selector(_currentMetrics);
    float previous = selector(_history[(_historyIndex - 1) % 10].metrics);

    return recent - previous;
}

bool CombatStateAnalyzer::IsMetricDeclining(::std::function<float(const CombatMetrics&)> selector, float threshold) const
{
    float trend = GetMetricTrend(selector);
    return trend < -threshold;
}

bool CombatStateAnalyzer::IsMetricImproving(::std::function<float(const CombatMetrics&)> selector, float threshold) const
{
    float trend = GetMetricTrend(selector);
    return trend > threshold;
}

uint32 CombatStateAnalyzer::GetPriorityTargetCount() const
{
    uint32 count = 0;
    for (Unit* enemy : _enemyCache)
    {
        if (!enemy || !enemy->IsAlive())
            continue;

        // Priority targets are elites, bosses, or low health enemies
    if (enemy->GetTypeId() == TYPEID_UNIT)
        {
            Creature* creature = enemy->ToCreature();
            if (creature->IsElite() || creature->IsDungeonBoss() || creature->GetHealthPct() < 30.0f)
                count++;
        }
    }
    return count;
}

::std::vector<Unit*> CombatStateAnalyzer::GetNearbyEnemies(float range) const
{
    ::std::vector<Unit*> result;
    float rangeSq = range * range;
    for (Unit* enemy : _enemyCache)
    {
        if (enemy && enemy->IsAlive() && _bot->GetExactDistSq(enemy) <= rangeSq)
                    result.push_back(enemy);
    }
    return result;
}

Unit* CombatStateAnalyzer::GetMostDangerousEnemy() const
{
    Unit* mostDangerous = nullptr;
    float highestDanger = 0.0f;

    for (Unit* enemy : _enemyCache)
            {
        if (!enemy || !enemy->IsAlive())
            continue;

        float danger = 1.0f;

        // Bosses are most dangerous
    if (enemy->GetTypeId() == TYPEID_UNIT)
                {
            Creature* creature = enemy->ToCreature();
                                                                                    if (creature->IsDungeonBoss())
                danger *= 10.0f;
            else if (creature->IsElite())
                danger *= 5.0f;
        }

        // Enemies targeting us are dangerous
    if (enemy->GetTarget() == _bot->GetGUID())
                            danger *= 3.0f;

        // Close enemies are dangerous
        float distance = ::std::sqrt(_bot->GetExactDistSq(enemy)); // Calculate once from squared distance
    if (distance < 5.0f)
            danger *= 2.0f;

        // Low health enemies are priority targets
    if (enemy->GetHealthPct() < 30.0f)
            danger *= 1.5f;

        if (danger > highestDanger)
        {
            highestDanger = danger;
            mostDangerous = enemy;
        }
    }

    return mostDangerous;
}

bool CombatStateAnalyzer::HasCleaveTargets() const
{
    // Check if multiple enemies are in cleave range (8 yards)
    uint32 cleaveCount = 0;
    for (Unit* enemy : _enemyCache)
    {
        if (enemy && enemy->IsAlive() && _bot->GetExactDistSq(enemy) <= (8.0f * 8.0f)) // 64.0f
                    cleaveCount++;
    }
    return cleaveCount >= 2;
}

bool CombatStateAnalyzer::ShouldFocusAdd() const
{
    // Focus adds if they're dangerous or numerous
    return _currentMetrics.enemyCount > 1 &&
           (_currentMetrics.eliteCount > 0 || _currentMetrics.enemyCount >= 3);
}

Player* CombatStateAnalyzer::GetLowestHealthAlly() const
{
    Player* lowest = _bot;
    float lowestHealth = _bot->GetHealthPct();

    if (Group* group = _bot->GetGroup())
        {
        for (GroupReference const& groupRef : group->GetMembers())
        {
            Player* member = groupRef.GetSource();
                        if (!member || !member->IsAlive())
                continue;

            float health = member->GetHealthPct();
            if (health < lowestHealth)
            {
                lowestHealth = health;
                lowest = member;
            }
        }
    }

    return lowest;
}

Player* CombatStateAnalyzer::GetMainTank() const
{
    // Cache for performance
    if (_mainTankCache && GameTime::GetGameTimeMS() - _roleCacheTime < 1000)
        return _mainTankCache;

    _mainTankCache = nullptr;
    _roleCacheTime = GameTime::GetGameTimeMS();

    if (Group* group = _bot->GetGroup())
        {
        for (GroupReference const& groupRef : group->GetMembers())
        {
            Player* member = groupRef.GetSource();
                        if (!member || !member->IsAlive())
                continue;

            Classes memberClass = static_cast<Classes>(member->GetClass());
                        if (memberClass == CLASS_WARRIOR || memberClass == CLASS_PALADIN ||
                memberClass == CLASS_DEATH_KNIGHT)
            {
                // Simple tank detection - would need role assignment in production
                _mainTankCache = member;
                break;
            }
        }
    }

    return _mainTankCache;
}

Player* CombatStateAnalyzer::GetMainHealer() const
{
    // Cache for performance
    if (_mainHealerCache && GameTime::GetGameTimeMS() - _roleCacheTime < 1000)
        return _mainHealerCache;

    _mainHealerCache = nullptr;

    if (Group* group = _bot->GetGroup())
    {
        for (GroupReference const& groupRef : group->GetMembers())
        {
            Player* member = groupRef.GetSource();
            if (!member || !member->IsAlive())
                continue;

            Classes memberClass = static_cast<Classes>(member->GetClass());
                        if (memberClass == CLASS_PRIEST || memberClass == CLASS_DRUID ||
                memberClass == CLASS_SHAMAN || memberClass == CLASS_PALADIN)
            {
                // Simple healer detection - would need role assignment in production
                _mainHealerCache = member;
                                break;
            }
        }
    }

    return _mainHealerCache;
}

bool CombatStateAnalyzer::IsGroupHealthCritical() const
{
    return _currentMetrics.averageGroupHealth < 40.0f || _currentMetrics.lowestGroupHealth < 20.0f;
}

bool CombatStateAnalyzer::IsGroupManaLow() const
{
    if (Group* group = _bot->GetGroup())
        {
        uint32 lowManaCount = 0;
        uint32 manaUsers = 0;

        for (GroupReference const& groupRef : group->GetMembers())
        {
            Player* member = groupRef.GetSource();
                        if (!member || !member->IsAlive())
                continue;

            if (member->GetMaxPower(POWER_MANA) > 0)
            {
                manaUsers++;
                if (member->GetPowerPct(POWER_MANA) < 30.0f)
                    lowManaCount++;
                        }
        }

                return manaUsers > 0 && lowManaCount >= manaUsers / 2;
        }

    return _currentMetrics.manaPercent < 30.0f;
}

float CombatStateAnalyzer::GetGroupSurvivabilityScore() const
{
    float score = 100.0f;

    // Health factor
    score *= (_currentMetrics.averageGroupHealth / 100.0f);

    // Tank/healer alive factors
    if (!_currentMetrics.tankAlive)
        score *= 0.5f;
    if (!_currentMetrics.healerAlive)
        score *= 0.6f;

        // Enemy danger factor
    if (_currentMetrics.bossCount > 0)
        score *= 0.8f;
    if (_currentMetrics.eliteCount > 2)
        score *= 0.7f;

    // Positioning factor
    if (!_currentMetrics.isPositioningSafe)
        score *= 0.9f;

        return ::std::max(0.0f, score);
}

::std::vector<ThreatData> CombatStateAnalyzer::GetThreatList() const
{
    ::std::vector<ThreatData> threatList;

    // Would need actual threat API access here
    // This is a simplified version
    for (Unit* enemy : _enemyCache)
            {
        if (!enemy || !enemy->IsAlive())
            continue;

        ThreatData data;
        data.targetGuid = enemy->GetGUID();
                data.isTanking = (enemy->GetTarget() == _bot->GetGUID());
                data.threatValue = data.isTanking ? 100.0f : 0.0f;
        data.position = data.isTanking ? 1 : 2;

        threatList.push_back(data);
    }

    return threatList;
}

bool CombatStateAnalyzer::IsAboutToGetAggro() const
{
    // Simplified - would need actual threat percentage calculation
    return false;
}

bool CombatStateAnalyzer::ShouldDropThreat() const
{
    // Drop threat if we're not a tank and have aggro on dangerous enemies
    Classes botClass = static_cast<Classes>(_bot->GetClass());
        bool canTank = (botClass == CLASS_WARRIOR || botClass == CLASS_PALADIN ||
                    botClass == CLASS_DEATH_KNIGHT || botClass == CLASS_DRUID);

return !canTank && _currentMetrics.hasAggro &&
           (_currentMetrics.eliteCount > 0 || _currentMetrics.bossCount > 0);
}

float CombatStateAnalyzer::GetThreatPercentage(Unit* target) const
{
    if (!target)
        return 0.0f;

    // Would need actual threat API
    return target->GetTarget() == _bot->GetGUID() ? 100.0f : 50.0f;
    }

void CombatStateAnalyzer::RegisterBossMechanic(const BossMechanic& mechanic)
{
    _knownMechanics.push_back(mechanic);
}

bool CombatStateAnalyzer::IsBossMechanicIncoming(uint32& spellId, uint32& timeUntil) const
{
    for (const BossMechanic& mechanic : _knownMechanics)
    {
        if (mechanic.lastSeen > 0)
        {
            uint32 nextCast = mechanic.lastSeen + mechanic.cooldown;
            if (GameTime::GetGameTimeMS() < nextCast)
            {
                spellId = mechanic.spellId;
                timeUntil = nextCast - GameTime::GetGameTimeMS();
                return timeUntil < 3000; // Mechanic incoming in next 3 seconds
            }
        }
    }
    return false;
}

bool CombatStateAnalyzer::ShouldInterruptCast(Unit* caster, uint32 spellId) const
{
    if (!caster)
        return false;

    // Check known mechanics
    for (const BossMechanic& mechanic : _knownMechanics)
    {
        if (mechanic.spellId == spellId && mechanic.requiresInterrupt)
            return true;
    }

    // Check spell info for interruptible dangerous spells
    if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE))
    {
        // Interrupt heals on enemies
    if (spellInfo->HasEffect(SPELL_EFFECT_HEAL))
            return true;

        // Interrupt crowd control
    if (spellInfo->HasAura(SPELL_AURA_MOD_STUN) ||
            spellInfo->HasAura(SPELL_AURA_MOD_FEAR))
            return true;
    }

    return false;
}

uint32 CombatStateAnalyzer::GetEstimatedEnrageTime() const
{
    return _currentMetrics.enrageTimer;
}

bool CombatStateAnalyzer::IsBossEnraging() const
{
    return _currentMetrics.enrageTimer > 0 && _currentMetrics.enrageTimer < 10000;
}

uint32 CombatStateAnalyzer::GetAverageUpdateTime() const
{
    if (_updateCount == 0)
        return 0;
    return _totalUpdateTime / _updateCount;
}

CombatMetrics CombatStateAnalyzer::GetAverageMetrics(uint32 periodMs) const
{
    CombatMetrics average;
    uint32 count = 0;
    uint32 now = GameTime::GetGameTimeMS();

    for (const MetricsSnapshot& snapshot : _history)
    {
        if (snapshot.timestamp == 0)
            continue;

        if (now - snapshot.timestamp <= periodMs)
        {
            average.groupDPS += snapshot.metrics.groupDPS;
            average.personalDPS += snapshot.metrics.personalDPS;
            average.averageGroupHealth += snapshot.metrics.averageGroupHealth;
            average.enemyCount += snapshot.metrics.enemyCount;
            count++;
        }
    }

    if (count > 0)
    {
        average.groupDPS /= count;
        average.personalDPS /= count;
        average.averageGroupHealth /= count;
        average.enemyCount /= count;
    }

    return average;
}

float CombatStateAnalyzer::GetDPSTrend() const
{
    if (_historyIndex < 2)
        return 0.0f;

    float recent = _currentMetrics.groupDPS;
    float older = _history[(_historyIndex - 5) % 10].metrics.groupDPS;

    return recent - older;
}

float CombatStateAnalyzer::GetHealthTrend() const
{
    if (_historyIndex < 2)
        return 0.0f;

    float recent = _currentMetrics.averageGroupHealth;
    float older = _history[(_historyIndex - 5) % 10].metrics.averageGroupHealth;

    return recent - older;
}

bool CombatStateAnalyzer::IsBeingKited() const
{
    // Check if we're being kited (enemy staying at range)
    if (!_currentMetrics.hasAggro)
        return false;

    for (Unit* enemy : _enemyCache)
            {
        if (!enemy || !enemy->IsAlive())
            continue;

        if (enemy->GetTarget() == _bot->GetGUID())
                {
            float distance = ::std::sqrt(_bot->GetExactDistSq(enemy)); // Calculate once from squared distance
    if (distance > 15.0f && distance < 40.0f)
                return true;
        }
    }

    return false;
}

bool CombatStateAnalyzer::IsBeingFocused() const
{
    // Check if multiple enemies are targeting us
    uint32 targetingUs = 0;
    for (Unit* enemy : _enemyCache)
    {
        if (enemy && enemy->IsAlive() && enemy->GetTarget() == _bot->GetGUID())
                    targetingUs++;
    }

    return targetingUs >= 2;
}

bool CombatStateAnalyzer::IsInVoidZone() const
{
    // Check for dangerous ground effects using detected effects and auras
    Position botPos = _bot->GetPosition();

    // Check tracked ground effects from DetectBossMechanics
    for (const auto& effect : _detectedGroundEffects)
    {
        if (GameTime::GetGameTimeMS() < effect.expiryTime)
        {
            float dist = botPos.GetExactDist2d(effect.center);
            if (dist <= effect.radius)
                return true;
        }
    }

    // Check for harmful auras that indicate standing in bad stuff
    // These are common "void zone" type effects that deal periodic damage
    Unit::AuraApplicationMap const& auras = _bot->GetAppliedAuras();
    for (auto const& [auraId, aurApp] : auras)
    {
        Aura const* aura = aurApp->GetBase();
        if (!aura)
            continue;

        SpellInfo const* spellInfo = aura->GetSpellInfo();
        if (!spellInfo)
            continue;

        // Check if this is a harmful area aura (typically void zones)
        if (!spellInfo->IsPositive())
        {
            for (SpellEffectInfo const& effect : spellInfo->GetEffects())
            {
                // Check for periodic damage auras from area sources
                if (effect.ApplyAuraName == SPELL_AURA_PERIODIC_DAMAGE ||
                    effect.ApplyAuraName == SPELL_AURA_PERIODIC_DAMAGE_PERCENT)
                {
                    // Check if it's from a creature (enemy) or dynamic object (ground effect)
                    Unit* caster = aura->GetCaster();
                    if (caster && caster->GetTypeId() == TYPEID_UNIT && _bot->IsHostileTo(caster))
                    {
                        // If the spell creates persistent area aura, we're in a void zone
                        if (spellInfo->HasEffect(SPELL_EFFECT_PERSISTENT_AREA_AURA) ||
                            spellInfo->HasEffect(SPELL_EFFECT_CREATE_AREATRIGGER))
                        {
                            return true;
                        }

                        // Check if this aura was applied very recently (indicates we just stepped in)
                        if (aura->GetDuration() > 0 && aura->GetMaxDuration() - aura->GetDuration() < 1000)
                        {
                            // Freshly applied damage over time from enemy - likely void zone
                            return true;
                        }
                    }
                }
            }
        }
    }

    // Check for nearby dynamic objects (ground-based effects)
    // This catches desecrations, death and decay, etc.
    // Note: In production, would iterate DynamicObjects in range
    // For now, check if we're taking rapid periodic damage while stationary
    if (_currentMetrics.incomingDPS > 0 && !_bot->isMoving())
    {
        // If taking significant damage while standing still, probably in bad
        float healthLossRate = _currentMetrics.incomingDPS / static_cast<float>(_bot->GetMaxHealth());
        if (healthLossRate > 0.05f)  // Losing more than 5% health per second
        {
            return true;
        }
    }

    return false;
}

bool CombatStateAnalyzer::HasDebuffRequiringDispel() const
{
    // Check for dispellable debuffs
    Unit::AuraApplicationMap const& auras = _bot->GetAppliedAuras();
    for (auto const& [auraId, aurApp] : auras)
    {
        Aura const* aura = aurApp->GetBase();
        if (!aura)
            continue;

        SpellInfo const* spellInfo = aura->GetSpellInfo();
        if (!spellInfo)
            continue;

        // Check if it's a debuff that can be dispelled
    if (!spellInfo->IsPositive() && spellInfo->Dispel != DISPEL_NONE)
            return true;
    }

    return false;
}

bool CombatStateAnalyzer::IsPhaseTransition() const
{
    // Detect boss phase transitions
    for (Unit* enemy : _enemyCache)
    {
        if (!enemy || enemy->GetTypeId() != TYPEID_UNIT)
                    continue;

        Creature* creature = enemy->ToCreature();
                        if (creature->IsDungeonBoss())
        {
            // Common phase transition health percentages
            float health = creature->GetHealthPct();
            if (::std::abs(health - 75.0f) < 2.0f ||
                ::std::abs(health - 50.0f) < 2.0f ||
                ::std::abs(health - 25.0f) < 2.0f)
            {
                return true;
            }
        }
    }

    return false;
}

float CombatStateAnalyzer::CalculateGroupSpread() const
{
    if (Group* group = _bot->GetGroup())
        {
        ::std::vector<Position> positions;
        for (GroupReference const& groupRef : group->GetMembers())
        {
            if (Player* member = groupRef.GetSource())
                                                    {
                if (member->IsAlive())
                {
                    Position memberPos;
                    memberPos.Relocate(member->GetPositionX(), member->GetPositionY(), member->GetPositionZ());
                    positions.push_back(memberPos);
                }
            }
        }

        if (positions.size() < 2)
            return 0.0f;

        float totalDistance = 0.0f;
        uint32 comparisons = 0;

        for (size_t i = 0; i < positions.size(); ++i)
        {
            for (size_t j = i + 1; j < positions.size(); ++j)
            {
                float dist = positions[i].GetExactDist(&positions[j]);
                totalDistance += dist;
                comparisons++;
            }
        }

        return comparisons > 0 ? totalDistance / comparisons : 0.0f;
    }

    return 0.0f;
}

float CombatStateAnalyzer::CalculateDangerScore() const
{
    float danger = 0.0f;

    // Enemy factors
    danger += _currentMetrics.enemyCount * 10.0f;
    danger += _currentMetrics.eliteCount * 30.0f;
    danger += _currentMetrics.bossCount * 100.0f;

    // Health factors
    danger += (100.0f - _currentMetrics.averageGroupHealth) * 2.0f;
    danger += (100.0f - _currentMetrics.personalHealthPercent) * 1.5f;

    // Status factors
    if (!_currentMetrics.tankAlive)
        danger += 50.0f;
    if (!_currentMetrics.healerAlive)
        danger += 40.0f;
    if (_currentMetrics.hasAggro)
        danger += 20.0f;

    return danger;
}

bool CombatStateAnalyzer::IsUnitDangerous(Unit* unit) const
{
    if (!unit)
        return false;

    if (unit->GetTypeId() == TYPEID_UNIT)
    {
        Creature* creature = unit->ToCreature();
                        if (creature->IsDungeonBoss())
            return true;
        if (creature->IsElite())
            return true;
    }

    // Unit is dangerous if it can kill us quickly
    if (unit->GetTarget() == _bot->GetGUID())
            return true;

    return false;
}

void CombatStateAnalyzer::RecordSnapshot()
{
    MetricsSnapshot snapshot;
    snapshot.metrics = _currentMetrics;
    snapshot.timestamp = GameTime::GetGameTimeMS();
    snapshot.situation = _currentSituation;

    _history[_historyIndex % 10] = snapshot;
    _historyIndex++;
}

void CombatStateAnalyzer::PruneOldData()
{
    // Clean up old mechanic casts
    uint32 now = GameTime::GetGameTimeMS();
    _recentMechanicCasts.erase(
        ::std::remove_if(_recentMechanicCasts.begin(), _recentMechanicCasts.end(),
            [now](uint32 castTime) { return now - castTime > 30000; }),
        _recentMechanicCasts.end()
    );
}

void CombatStateAnalyzer::Reset()
{
    _currentMetrics.Reset();
    _currentSituation = CombatSituation::NORMAL;
    _previousSituation = CombatSituation::NORMAL;
    _situationChanged = false;
    _timeSinceSituationChange = 0;
    _updateTimer = 0;
    _enemyCache.clear();
    _enemyCacheTime = 0;
    _mainTankCache = nullptr;
    _mainHealerCache = nullptr;
    _roleCacheTime = 0;
    ClearHistory();
}

void CombatStateAnalyzer::ClearHistory()
{
    for (auto& snapshot : _history)
    {
        snapshot.metrics.Reset();
        snapshot.timestamp = 0;
        snapshot.situation = CombatSituation::NORMAL;
    }
    _historyIndex = 0;
    _lastSnapshotTime = 0;
}

} // namespace Playerbot