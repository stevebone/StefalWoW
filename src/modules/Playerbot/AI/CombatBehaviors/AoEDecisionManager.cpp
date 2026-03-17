/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "AoEDecisionManager.h"
#include "GameTime.h"
#include "AI/BotAI.h"
#include "Player.h"
#include "Unit.h"
#include "Creature.h"
#include "Spell.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "Log.h"
#include "Timer.h"
#include "SpellHistory.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "../../Spatial/SpatialGridQueryHelpers.h" // Thread-safe spatial grid queries
#include "CellImpl.h"
#include "UpdateFields.h"
#include "../../Spatial/SpatialGridManager.h"
#include "../../Group/GroupRoleEnums.h" // Centralized role detection utilities
#include <algorithm>
#include <cmath>

namespace Playerbot
{

namespace
{
    // Role Detection using centralized utilities from GroupRoleEnums.h
    // These use TrinityCore's ChrSpecializationEntry for accurate role detection
    bool IsTank(Player const* p) { return IsPlayerTank(p); }
    bool IsHealer(Player const* p) { return IsPlayerHealer(p); }
    bool IsDPS(Player const* p) { return IsPlayerDPS(p); }

    // AoE spell categories for different classes
    enum AoESpells : uint32
    {
        // Warrior
        THUNDER_CLAP = 6343,
        WHIRLWIND = 1680,
        BLADESTORM = 46924,
        SHOCKWAVE = 46968,

        // Paladin
        CONSECRATION = 48819,
        DIVINE_STORM = 53385,
        HAMMER_OF_THE_RIGHTEOUS = 53595,

        // Hunter
        VOLLEY = 58434,
        EXPLOSIVE_TRAP = 49067,
        MULTI_SHOT = 49048,

        // Rogue
        FAN_OF_KNIVES = 51723,
        BLADE_FLURRY = 13877,

        // Priest
        HOLY_NOVA = 48078,
        MIND_SEAR = 53023,
        SHADOW_WORD_DEATH = 48158,

        // Shaman
        CHAIN_LIGHTNING = 49271,
        FIRE_NOVA = 61657,
        MAGMA_TOTEM = 58734,
        THUNDERSTORM = 59159,

        // Mage
        ARCANE_EXPLOSION = 42921,
        BLIZZARD = 42940,
        FLAMESTRIKE = 42926,
        CONE_OF_COLD = 42931,
        FROST_NOVA = 42917,
        DRAGONS_BREATH = 42950,

        // Warlock
        RAIN_OF_FIRE = 47820,
        HELLFIRE = 47823,
        SHADOWFURY = 47847,
        SEED_OF_CORRUPTION = 47836,

        // Druid
        HURRICANE = 48467,
        SWIPE_BEAR = 48562,
        STARFALL = 53201,

        // Death Knight
        DEATH_AND_DECAY = 49938,
        BLOOD_BOIL = 49941,
        HOWLING_BLAST = 51411
    };
}

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

AoEDecisionManager::AoEDecisionManager(BotAI* ai) :
    _ai(ai),
    _bot(ai ? ai->GetBot() : nullptr),
    _lastCacheUpdate(0),
    _lastClusterUpdate(0),
    _minAoETargets(3),
    _aggressiveAoE(false),
    _smartTargeting(true),
    _currentStrategy(SINGLE_TARGET),
    _lastEfficiencyCalc(0),
    _cachedEfficiency(0.0f)
{
}

AoEDecisionManager::~AoEDecisionManager() = default;

// ============================================================================
// CORE UPDATE
// ============================================================================

void AoEDecisionManager::Update(uint32 /*diff*/)
{
    if (!_bot || !_bot->IsAlive() || !_bot->IsInCombat())
    {
        _targetCache.clear();
        _clusters.clear();
        _currentStrategy = SINGLE_TARGET;
        return;
    }

    uint32 now = GameTime::GetGameTimeMS();

    // Update target cache periodically
    if (now - _lastCacheUpdate > CACHE_UPDATE_INTERVAL)
    {
        UpdateTargetCache();
        _lastCacheUpdate = now;
    }

    // Update clustering less frequently
    if (now - _lastClusterUpdate > CLUSTER_UPDATE_INTERVAL)
    {
        CalculateClusters();
        _lastClusterUpdate = now;
    }

    // Determine current strategy
    uint32 targetCount = GetTargetCount();
    if (targetCount >= 8)
        _currentStrategy = AOE_FULL;
    else if (targetCount >= 5)
        _currentStrategy = _aggressiveAoE ? AOE_FULL : AOE_LIGHT;
    else if (targetCount >= 3)
        _currentStrategy = AOE_LIGHT;
    else if (targetCount >= 2)
        _currentStrategy = CLEAVE;
    else
        _currentStrategy = SINGLE_TARGET;

    // Adjust based on role
    if (IsTank(_bot))
    {
        // Tanks should use AoE more aggressively for threat
    if (targetCount >= 2)
            _currentStrategy = ::std::max(_currentStrategy, static_cast<AoEStrategy>(CLEAVE));
    }
    else if (IsHealer(_bot))
    {
        // Healers should be conservative with AoE
    if (_currentStrategy > CLEAVE)
            _currentStrategy = CLEAVE;
    }
}

// ============================================================================
// AOE STRATEGY
// ============================================================================

AoEDecisionManager::AoEStrategy AoEDecisionManager::GetOptimalStrategy() const
{
    return _currentStrategy;
}

uint32 AoEDecisionManager::GetTargetCount(float range) const
{
    if (!_bot)
        return 0;

    uint32 count = 0;

    // PHASE 5B: Thread-safe spatial grid query (replaces QueryNearbyCreatureGuids + ObjectAccessor)
    auto hostileSnapshots = SpatialGridQueryHelpers::FindHostileCreaturesInRange(_bot, range, true);

    // Count valid targets using snapshots (lock-free)
    for (auto const& snapshot : hostileSnapshots)
    {
        // Get Unit* for additional validation
        // SPATIAL GRID MIGRATION COMPLETE (2025-11-26):
        // ObjectAccessor is intentionally retained - Live Unit* needed for:
        // 1. IsValidAttackTarget() requires live target validation logic
        // 2. ToCreature() type conversion requires live object
        // 3. IsCritter()/IsTotem() require real-time creature state
        // The spatial grid pre-filters candidates to reduce ObjectAccessor calls.
        ::Unit* unit = ObjectAccessor::GetUnit(*_bot, snapshot.guid);
        if (!unit || !_bot->IsValidAttackTarget(unit))
            continue;

        if (unit->GetTypeId() == TYPEID_UNIT)
        {
            Creature* creature = unit->ToCreature();
            if (creature && (creature->IsCritter() || creature->IsTotem()))
                continue;
        }

        ++count;
    }

    return count;
}

bool AoEDecisionManager::ShouldUseAoE(uint32 minTargets) const
{
    uint32 actualMin = ::std::max(minTargets, _minAoETargets);

    // Role-based adjustments
    if (IsTank(_bot))
        actualMin = ::std::max(2u, actualMin - 1);  // Tanks need AoE for threat
    else if (IsHealer(_bot))
        actualMin = actualMin + 1;  // Healers should be conservative

    return GetTargetCount() >= actualMin;
}

// ============================================================================
// TARGET CLUSTERING
// ============================================================================

::std::vector<AoEDecisionManager::TargetCluster> AoEDecisionManager::FindTargetClusters(float maxRange) const
{
    ::std::vector<TargetCluster> clusters;
    if (!_bot)
        return clusters;

    // Build spatial grid
    _spatialGrid.clear();
    float maxRangeSq = maxRange * maxRange;
    for (auto const& [guid, info] : _targetCache)
    {
        if (_bot->GetExactDistSq(info.position) > maxRangeSq)
            continue;

        uint32 key = GetGridKey(info.position);
        _spatialGrid[key].targets.push_back(guid);
    }

    // Find clusters using DBSCAN-like algorithm
    ::std::unordered_set<ObjectGuid> processed;
    for (auto const& [guid, info] : _targetCache)
    {
        if (processed.count(guid) > 0)
            continue;

        TargetCluster cluster;
        cluster.center = info.position;

        // Find all targets within cluster radius (8 yards default)
        float clusterRadius = 8.0f;
        ::std::vector<ObjectGuid> neighbors = GetGridNeighbors(info.position, clusterRadius);

        if (neighbors.size() < 2)  // Need at least 2 targets for a cluster
            continue;

        float sumX = 0, sumY = 0, sumZ = 0;
        float sumHealth = 0;

        for (ObjectGuid const& neighborGuid : neighbors)
        {
            auto it = _targetCache.find(neighborGuid);
            if (it == _targetCache.end())
                continue;

            cluster.targets.push_back(neighborGuid);
            sumX += it->second.position.GetPositionX();
            sumY += it->second.position.GetPositionY();
            sumZ += it->second.position.GetPositionZ();
            sumHealth += it->second.healthPercent;

            if (it->second.isElite)
                cluster.hasElite = true;

            processed.insert(neighborGuid);
        }

        // Calculate cluster center and properties
        cluster.targetCount = cluster.targets.size();
        cluster.center.m_positionX = sumX / cluster.targetCount;
        cluster.center.m_positionY = sumY / cluster.targetCount;
        cluster.center.m_positionZ = sumZ / cluster.targetCount;
        cluster.avgHealthPercent = sumHealth / cluster.targetCount;
        cluster.radius = clusterRadius;

        clusters.push_back(cluster);
    }

    // Sort clusters by target count (descending)
    ::std::sort(clusters.begin(), clusters.end(),
        [](TargetCluster const& a, TargetCluster const& b) {
            return a.targetCount > b.targetCount;
        });

    return clusters;
}

Position AoEDecisionManager::GetBestAoEPosition(uint32 spellId) const
{
    if (!_bot)
        return Position();

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return _bot->GetPosition();

    float radius = spellInfo->GetMaxRange(false);
    if (radius <= 0)
        radius = 30.0f;  // Default search radius

    // Find clusters
    ::std::vector<TargetCluster> clusters = FindTargetClusters(radius);
    if (clusters.empty())
        return _bot->GetPosition();

    // Return center of largest cluster
    return clusters[0].center;
}

// ============================================================================
// CLEAVE OPTIMIZATION
// ============================================================================

float AoEDecisionManager::GetCleavePriority() const
{
    uint32 targetCount = GetTargetCount(8.0f);
    if (targetCount < 2)
        return 0.0f;

    // Base priority on target count
    float priority = ::std::min(1.0f, targetCount / 5.0f);

    // Adjust for role
    if (IsTank(_bot))
        priority = ::std::min(1.0f, priority * 1.5f);  // Tanks need cleave for threat
    else if (IsHealer(_bot))
        priority *= 0.5f;  // Healers should focus on healing

    // Check average health
    float avgHealth = 0.0f;
    uint32 validTargets = 0;
    float rangeSq = 8.0f * 8.0f; // 64.0f
    for (auto const& [guid, info] : _targetCache)
    {
        if (_bot->GetExactDistSq(info.position) <= rangeSq)
        {
            avgHealth += info.healthPercent;
            ++validTargets;
        }
    }

    if (validTargets > 0)
    {
        avgHealth /= validTargets;
        // Reduce priority if targets are low health (will die soon anyway)
    if (avgHealth < 30.0f)
            priority *= 0.5f;
    }

    return priority;
}

float AoEDecisionManager::GetBestCleaveAngle(float coneAngle) const
{
    if (!_bot || !_bot->GetVictim())
        return _bot ? _bot->GetOrientation() : 0.0f;

    float bestAngle = _bot->GetAbsoluteAngle(_bot->GetVictim());
    uint32 maxTargets = 1;

    // Test different angles
    for (float testAngle = 0; testAngle < 2.0f * static_cast<float>(M_PI); testAngle += static_cast<float>(M_PI) / 8.0f)
    {
        uint32 targetsInCone = 0;

        for (auto const& [guid, info] : _targetCache)
        {
            float angleToTarget = _bot->GetAbsoluteAngle(&info.position);
            float angleDiff = ::std::abs(angleToTarget - testAngle);

            // Normalize angle difference
    if (angleDiff > static_cast<float>(M_PI))
                angleDiff = 2.0f * static_cast<float>(M_PI) - angleDiff;

            if (angleDiff <= coneAngle / 2)
                ++targetsInCone;
        }

        if (targetsInCone > maxTargets)
        {
            maxTargets = targetsInCone;
            bestAngle = testAngle;
        }
    }

    return bestAngle;
}

// ============================================================================
// AOE EFFICIENCY
// ============================================================================

float AoEDecisionManager::CalculateAoEEfficiency(uint32 targets, float spellRadius) const
{
    if (targets == 0)
        return 0.0f;

    // Cache efficiency calculation
    uint32 now = GameTime::GetGameTimeMS();
    if (now - _lastEfficiencyCalc < 1000)
        return _cachedEfficiency;

    // Base efficiency on target count vs optimal AoE targets
    float efficiency = 0.0f;

    if (targets >= 5)
        efficiency = 1.0f;  // Maximum efficiency at 5+ targets
    else if (targets >= 3)
        efficiency = 0.7f + (targets - 3) * 0.15f;
    else if (targets >= 2)
        efficiency = 0.4f + (targets - 2) * 0.3f;
    else
        efficiency = 0.2f;  // Minimum efficiency for single target

    // Adjust for spell radius (larger radius = slightly less efficient)
    if (spellRadius > 20.0f)
        efficiency *= 0.9f;
    else if (spellRadius > 30.0f)
        efficiency *= 0.8f;

    // Role adjustments
    if (IsTank(_bot))
        efficiency *= 1.2f;  // Tanks benefit more from AoE
    else if (IsHealer(_bot))
        efficiency *= 0.7f;  // Healers should conserve mana

    _cachedEfficiency = ::std::min(1.0f, efficiency);
    _lastEfficiencyCalc = now;

    return _cachedEfficiency;
}

bool AoEDecisionManager::IsHealthSufficientForAoE(float avgHealthPercent) const
{
    // Don't waste AoE on nearly dead targets
    if (IsTank(_bot))
        return avgHealthPercent > 20.0f;  // Tanks need threat regardless
    else
        return avgHealthPercent > 35.0f;  // Others should be more selective
}

float AoEDecisionManager::CalculateResourceEfficiency(uint32 aoeSpellId, uint32 singleTargetSpellId) const
{
    if (!_bot)
        return 0.0f;

    SpellInfo const* aoeInfo = sSpellMgr->GetSpellInfo(aoeSpellId, DIFFICULTY_NONE);
    SpellInfo const* stInfo = sSpellMgr->GetSpellInfo(singleTargetSpellId, DIFFICULTY_NONE);

    if (!aoeInfo || !stInfo)
        return 1.0f;

    // Compare mana/energy costs
    auto aoeCosts = aoeInfo->CalcPowerCost(_bot, aoeInfo->GetSchoolMask());
    auto stCosts = stInfo->CalcPowerCost(_bot, stInfo->GetSchoolMask());

    uint32 aoeCost = aoeCosts.empty() ? 0 : aoeCosts[0].Amount;
    uint32 stCost = stCosts.empty() ? 0 : stCosts[0].Amount;

    if (aoeCost == 0 || stCost == 0)
        return 1.0f;

    uint32 targetCount = GetTargetCount();
    if (targetCount < 2)
        return 0.5f;  // Single target always favors ST spell

    // Full implementation for AoE damage calculation using spell coefficients
    // Current behavior: Assumes AoE spells deal 60% of single-target damage per target hit
    // Full implementation should:
    // - Query actual spell damage coefficients from SpellInfo
    // - Account for AoE damage falloff/diminishing returns per target
    // - Calculate damage based on spell effects (SPELL_EFFECT_SCHOOL_DAMAGE)
    // - Factor in target resistances and armor mitigation
    // - Consider spell-specific mechanics (e.g., chain vs splash)
    // Reference: SpellInfo::GetEffect(), spell damage calculation formulas
    // Calculate damage per resource point
    float aoeDamagePerResource = (0.6f * targetCount) / aoeCost;
    float stDamagePerResource = 1.0f / stCost;

    return aoeDamagePerResource / stDamagePerResource;
}

// ============================================================================
// DOT SPREADING
// ============================================================================

::std::vector<Unit*> AoEDecisionManager::GetDoTSpreadTargets(uint32 maxTargets) const
{
    ::std::vector<Unit*> targets;
    if (!_bot)
        return targets;

    // Build priority list
    struct DoTTarget
    {
        Unit* unit;
        float priority;
    };
    ::std::vector<DoTTarget> candidates;

    for (auto const& [guid, info] : _targetCache)
    {
        // PHASE 5B: Thread-safe spatial grid validation (replaces ObjectAccessor::GetUnit)
        auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, guid);
        if (!snapshot || !snapshot->IsAlive())
            continue;

        // Get Unit* for additional validation

        // Calculate priority
        float priority = 100.0f;

        // Prioritize targets without DoTs
    if (!info.hasDot)
            priority += 50.0f;

        // Prioritize high health targets
        priority += info.healthPercent * 0.5f;

        // Prioritize elites
    if (info.isElite)
            priority += 30.0f;

        // Deprioritize distant targets (use snapshot position for lock-free calculation)
        float distance = ::std::sqrt(_bot->GetExactDistSq(snapshot->position)); // Calculate once from squared distance
        priority -= distance * 2.0f;

        // SPATIAL GRID MIGRATION COMPLETE (2025-11-26):
        // ObjectAccessor is intentionally retained - Live Unit* needed for:
        // 1. Return type is std::vector<Unit*> - callers require live Unit*
        // 2. DoT spreading logic requires live aura verification
        // The spatial grid provides position/health data, ObjectAccessor handles DoT state.
        Unit* unit = ObjectAccessor::GetUnit(*_bot, snapshot->guid);
        if (!unit) continue;
        candidates.push_back({unit, priority});
    }

    // Sort by priority
    ::std::sort(candidates.begin(), candidates.end(),
        [](DoTTarget const& a, DoTTarget const& b) {
            return a.priority > b.priority;
        });

    // Return top N targets
    for (size_t i = 0; i < ::std::min(static_cast<size_t>(maxTargets), candidates.size()); ++i)
        targets.push_back(candidates[i].unit);

    return targets;
}

bool AoEDecisionManager::NeedsDoTRefresh(Unit* target, uint32 dotSpellId) const
{
    if (!target || !_bot)
        return false;

    // Check if target has the DoT
    if (Aura const* dot = target->GetAura(dotSpellId, _bot->GetGUID()))
    {
        // Refresh if less than 30% duration remaining
        int32 duration = dot->GetDuration();
        int32 maxDuration = dot->GetMaxDuration();

        if (maxDuration > 0)
        {
            float remainingPercent = (float)duration / (float)maxDuration;
            return remainingPercent < 0.3f;
        }
    }

    return true;  // No DoT present
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void AoEDecisionManager::SetMinimumAoETargets(uint32 count)
{
    _minAoETargets = ::std::max(2u, count);
}

void AoEDecisionManager::SetAoEAggression(bool aggressive)
{
    _aggressiveAoE = aggressive;
}

void AoEDecisionManager::SetSmartTargeting(bool enabled)
{
    _smartTargeting = enabled;
}

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

void AoEDecisionManager::UpdateTargetCache()
{
    if (!_bot)
        return;

    uint32 now = GameTime::GetGameTimeMS();

    // Remove stale entries
    for (auto it = _targetCache.begin(); it != _targetCache.end();)
    {
        if (now - it->second.lastUpdateTime > 5000)  // 5 second timeout
            it = _targetCache.erase(it);
        else
            ++it;
    }

    // PHASE 5B: Thread-safe spatial grid query (replaces QueryNearbyCreatureGuids + ObjectAccessor)
    float searchRadius = 40.0f;
    auto hostileSnapshots = SpatialGridQueryHelpers::FindHostileCreaturesInRange(_bot, searchRadius, true);

    // Update cache with hostile snapshots
    for (auto const& snapshot : hostileSnapshots)
    {
        // Get Unit* for additional validation
        // SPATIAL GRID MIGRATION COMPLETE (2025-11-26):
        // ObjectAccessor is intentionally retained - Live Unit* needed for:
        // 1. IsValidAoETarget() requires live target validation logic
        // 2. ToCreature()->IsElite() requires real-time creature state
        // 3. GetThreatManager() requires live threat table access
        // The spatial grid provides health/position data, ObjectAccessor handles live validation.
        ::Unit* unit = ObjectAccessor::GetUnit(*_bot, snapshot.guid);
        if (!unit || !IsValidAoETarget(unit))
            continue;

        TargetInfo info;
        info.guid = snapshot.guid;
        info.position = snapshot.position;
        info.healthPercent = (snapshot.maxHealth > 0) ?
            (float(snapshot.health) / float(snapshot.maxHealth)) * 100.0f : 0.0f;
        info.isElite = unit->GetTypeId() == TYPEID_UNIT &&
                       unit->ToCreature()->IsElite();
        info.hasDot = false;  // Would check for specific DoTs here
        info.threatLevel = unit->GetThreatManager().GetThreatListSize();
        info.lastUpdateTime = now;

        _targetCache[info.guid] = info;
    }
}

void AoEDecisionManager::CalculateClusters()
{
    _clusters = FindTargetClusters(30.0f);
}

float AoEDecisionManager::ScoreAoEPosition(Position const& pos, float radius) const
{
    if (!_bot)
        return 0.0f;

    float score = 0.0f;
    uint32 targetsHit = 0;

    for (auto const& [guid, info] : _targetCache)
    {
        float distance = pos.GetExactDist(&info.position);
        if (distance <= radius)
        {
            ++targetsHit;

            // Higher score for hitting more important targets
            float targetScore = 1.0f;
            if (info.isElite)
                targetScore *= 2.0f;
            if (info.healthPercent > 50.0f)
                targetScore *= 1.5f;
            score += targetScore;
        }
    }

    // Penalize positions too far from bot
    float botDistance = ::std::sqrt(_bot->GetExactDistSq(pos)); // Calculate once from squared distance
    if (botDistance > 30.0f)
        score *= 0.5f;
    else if (botDistance > 20.0f)
        score *= 0.8f;

    return score;
}

bool AoEDecisionManager::IsValidAoETarget(Unit* unit) const
{
    if (!unit || !_bot)
        return false;

    if (!unit->IsAlive())
        return false;

    if (!_bot->IsValidAttackTarget(unit))
        return false;

    // Skip critters and totems
    if (unit->GetTypeId() == TYPEID_UNIT)
    {
        Creature* creature = unit->ToCreature();
        if (creature && (creature->IsCritter() || creature->IsTotem()))
            return false;
    }

    // Skip targets that are crowd controlled
    if (unit->HasAuraType(SPELL_AURA_MOD_CONFUSE) ||
        unit->HasAuraType(SPELL_AURA_MOD_FEAR) ||
        unit->HasAuraType(SPELL_AURA_MOD_STUN))
        return false;

    return true;
}

uint32 AoEDecisionManager::GetRoleAoEThreshold() const
{
    if (!_bot)
        return 3;

    if (IsTank(_bot))
        return 2;  // Tanks need AoE for threat
    else if (IsHealer(_bot))
        return 4;  // Healers should be conservative
    else
        return 3;  // DPS standard threshold
}

uint32 AoEDecisionManager::GetGridKey(Position const& pos) const
{
    int32 gridX = static_cast<int32>(pos.GetPositionX() / GRID_SIZE);
    int32 gridY = static_cast<int32>(pos.GetPositionY() / GRID_SIZE);

    // Simple hash for 2D grid coordinates
    return (gridX & 0xFFFF) | ((gridY & 0xFFFF) << 16);
}

::std::vector<ObjectGuid> AoEDecisionManager::GetGridNeighbors(Position const& pos, float radius) const
{
    ::std::vector<ObjectGuid> neighbors;

    int32 gridRadius = static_cast<int32>(::std::ceil(radius / GRID_SIZE));
    int32 centerX = static_cast<int32>(pos.GetPositionX() / GRID_SIZE);
    int32 centerY = static_cast<int32>(pos.GetPositionY() / GRID_SIZE);

    for (int32 x = centerX - gridRadius; x <= centerX + gridRadius; ++x)
    {
        for (int32 y = centerY - gridRadius; y <= centerY + gridRadius; ++y)
        {
            uint32 key = (x & 0xFFFF) | ((y & 0xFFFF) << 16);
            auto it = _spatialGrid.find(key);
            if (it != _spatialGrid.end())
            {
                for (ObjectGuid const& guid : it->second.targets)
                {
                    auto targetIt = _targetCache.find(guid);
                    if (targetIt != _targetCache.end())
                    {
                        if (pos.GetExactDist(&targetIt->second.position) <= radius)
                            neighbors.push_back(guid);
                    }
                }
            }
        }
    }

    return neighbors;
}

} // namespace Playerbot