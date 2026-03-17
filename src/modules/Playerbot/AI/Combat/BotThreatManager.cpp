/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BotThreatManager.h"
#include "GameTime.h"
#include "Player.h"
#include "Unit.h"
#include "Creature.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "ObjectAccessor.h"
#include "Group.h"
#include "Log.h"
#include "Combat/ThreatManager.h" // TrinityCore's ThreatManager
#include "Spatial/SpatialGridQueryHelpers.h" // Thread-safe spatial grid queries
#include "Core/Events/CombatEventRouter.h"
#include "Core/Events/CombatEvent.h"
#include <algorithm>
#include <chrono>

namespace Playerbot
{

BotThreatManager::BotThreatManager(Player* bot)
    : _bot(bot), _botRole(ThreatRole::UNDEFINED), _updateInterval(DEFAULT_UPDATE_INTERVAL),
      _threatRadius(DEFAULT_THREAT_RADIUS), _lastUpdate(0), _analysisTimestamp(0), _analysisDirty(true)
{
    // Determine bot role based on class/spec
    if (_bot)
    {
        uint8 classId = _bot->GetClass();
        switch (classId)
        {
            case CLASS_WARRIOR:
            case CLASS_PALADIN:
            case CLASS_DEATH_KNIGHT:
            case CLASS_DEMON_HUNTER:
                _botRole = ThreatRole::TANK;
                break;
            case CLASS_PRIEST:
            case CLASS_SHAMAN:
            case CLASS_DRUID:
            case CLASS_MONK:
            case CLASS_EVOKER:
                _botRole = ThreatRole::HEALER; // Default, can be DPS based on spec
                break;
            default:
                _botRole = ThreatRole::DPS;
                break;
        }
    }

    // CRITICAL: Do NOT access _bot->GetName() or _bot->GetGUID() in constructor!
    // Bot may not be fully in world yet during ClassAI/BotAI construction,
    // and Player::m_name/m_guid are not initialized, causing ACCESS_VIOLATION.
    // Logging deferred to first UpdateThreat() when bot IsInWorld().

    // Phase 3: Subscribe to combat events for real-time threat tracking
    if (CombatEventRouter::Instance().IsInitialized())
    {
        CombatEventRouter::Instance().Subscribe(this);
        _subscribed = true;
        TC_LOG_DEBUG("playerbots", "BotThreatManager: Subscribed to CombatEventRouter (event-driven mode)");
    }
}

BotThreatManager::~BotThreatManager()
{
    // Phase 3: Unsubscribe from combat events
    if (_subscribed && CombatEventRouter::Instance().IsInitialized())
    {
        CombatEventRouter::Instance().Unsubscribe(this);
        _subscribed = false;
        TC_LOG_DEBUG("playerbots", "BotThreatManager: Unsubscribed from CombatEventRouter");
    }
}

void BotThreatManager::UpdateThreat(uint32 diff)
{
    if (!_bot)
        return;

    // ========================================================================
    // Phase 3 Event-Driven Architecture:
    // - Threat updates moved to event handlers (HandleDamageDealt, etc.)
    // - Update() only runs maintenance tasks at reduced frequency
    // - Maintenance includes: cleanup, distance updates, role-based adjustments
    // ========================================================================

    _maintenanceTimer += diff;

    // Run maintenance at reduced frequency (1Hz instead of 2Hz)
    // Also run if threat data is dirty from events
    if (_maintenanceTimer < MAINTENANCE_INTERVAL_MS && !_threatDataDirty)
        return;

    auto startTime = ::std::chrono::high_resolution_clock::now();

    _maintenanceTimer = 0;
    _threatDataDirty = false;

    {
        // No lock needed - threat data is per-bot instance data

        // Update threat table from TrinityCore (maintenance sync)
        UpdateThreatTable(diff);

        // Clean up stale entries
        CleanupStaleEntries();

        // Update distances and combat state (maintenance)
        UpdateDistances();
        UpdateCombatState();

        // Update role-based threat
        UpdateRoleBasedThreat();

        // Update target priorities
        UpdateTargetPriorities();

        // Mark analysis as dirty
        _analysisDirty = true;
    }

    auto endTime = ::std::chrono::high_resolution_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime);
    TrackPerformance(duration, "UpdateThreat");

    _metrics.threatCalculations++;
}

void BotThreatManager::ResetThreat()
{
    // No lock needed - threat data is per-bot instance data
    _threatMap.Clear();
    _threatHistory.Clear();
    _analysisDirty = true;

    TC_LOG_DEBUG("playerbots", "ThreatManager: Reset threat for bot {}",
                _bot ? _bot->GetName() : "null");
}

void BotThreatManager::ClearAllThreat()
{
    ResetThreat();
}

float BotThreatManager::CalculateThreat(Unit* target) const
{
    if (!target || !_bot)
        return 0.0f;

    auto startTime = ::std::chrono::high_resolution_clock::now();

    float baseThreat = CalculateBaseThreat(target);
    float roleModifier = CalculateRoleModifier();
    float distanceModifier = CalculateDistanceModifier(target);
    float healthModifier = CalculateHealthModifier(target);
    float abilityModifier = CalculateAbilityModifier(target);

    float finalThreat = baseThreat * roleModifier * distanceModifier * healthModifier * abilityModifier;

    auto endTime = ::std::chrono::high_resolution_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime);
    const_cast<BotThreatManager*>(this)->TrackPerformance(duration, "CalculateThreat");

    return finalThreat;
}

float BotThreatManager::CalculateThreatPercent(Unit* target) const
{
    if (!target || !_bot)
        return 0.0f;

    ::ThreatManager& threatMgr = target->GetThreatManager();
    float myThreat = threatMgr.GetThreat(_bot);

    // Find maximum threat from the threat list
    float maxThreat = 0.0f;
    if (!threatMgr.IsThreatListEmpty())
    {
        auto threatList = threatMgr.GetModifiableThreatList();
        for (auto* ref : threatList)
        {
            if (ref && ref->IsOnline())
            {
                float threatValue = ref->GetThreat();
                if (threatValue > maxThreat)
                    maxThreat = threatValue;
            }
        }
    }

    if (maxThreat <= 0.0f)
        return myThreat > 0.0f ? 100.0f : 0.0f;
    return (myThreat / maxThreat) * 100.0f;
}

void BotThreatManager::UpdateThreatValue(Unit* target, float threat, ThreatType type)
{
    if (!target || !_bot)
        return;

    ObjectGuid targetGuid = target->GetGUID();
    uint32 now = GameTime::GetGameTimeMS();

    // No lock needed - threat data is per-bot instance data

    auto& info = _threatMap[targetGuid];
    info.targetGuid = targetGuid;
    info.botGuid = _bot->GetGUID();
    info.threatValue = threat;
    info.threatPercent = CalculateThreatPercent(target);
    info.type = type;
    info.lastUpdate = now;
    info.isActive = true;
    info.isInCombat = target->IsInCombat();
    info.distance = ::std::sqrt(_bot->GetExactDist2dSq(target)); // Calculate once from squared 2D distance
    info.lastPosition = target->GetPosition();
    // Update specific threat metrics based on type
    switch (type)
    {
        case ThreatType::DAMAGE:
            info.damageDealt += threat;
            info.threatGenerated += threat;
            break;
        case ThreatType::HEALING:
            info.healingDone += threat;
            info.threatGenerated += threat * 0.5f; // Healing generates less threat
            break;
        case ThreatType::AGGRO:
            info.threatGenerated += threat;
            break;
        default:
            info.threatGenerated += threat;
            break;
    }

    // Update threat history
    UpdateThreatHistory(target, threat);

    _analysisDirty = true;

    TC_LOG_TRACE("playerbots", "ThreatManager: Updated threat for bot {} on target {} - Threat: {:.2f}, Percent: {:.2f}",
                _bot->GetName(), target->GetName(), threat, info.threatPercent);
}

void BotThreatManager::ModifyThreat(Unit* target, float modifier)
{
    if (!target || !_bot)
        return;

    ObjectGuid targetGuid = target->GetGUID();
    // No lock needed - threat data is per-bot instance data

    if (_threatMap.Contains(targetGuid))
    {
        ThreatInfo& info = _threatMap[targetGuid];
        info.threatValue *= modifier;
        info.threatPercent = CalculateThreatPercent(target);
        info.lastUpdate = GameTime::GetGameTimeMS();

        if (modifier < 1.0f)
            info.threatReduced += info.threatValue * (1.0f - modifier);

        _analysisDirty = true;
    }
}

ThreatAnalysis BotThreatManager::AnalyzeThreatSituation()
{
    auto startTime = ::std::chrono::high_resolution_clock::now();

    uint32 now = GameTime::GetGameTimeMS();

    // Check if cached analysis is still valid
    if (!_analysisDirty && (now - _analysisTimestamp) < ANALYSIS_CACHE_DURATION)
    {
        return _cachedAnalysis;
    }

    // No lock needed - threat data is per-bot instance data

    ThreatAnalysis analysis;
    ::std::vector<ThreatTarget> targets;

    // Analyze each threat target
    for (const auto& [guid, info] : _threatMap)
    {
        if (!info.isActive)
            continue;

        // PHASE 5B: Thread-safe spatial grid lookup (replaces ObjectAccessor::GetUnit)
        auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, guid);
        if (!snapshot || !snapshot->IsAlive())
            continue;

        // Get actual Unit* only when needed (main thread validation)
        Unit* target = ObjectAccessor::GetUnit(*_bot, guid);
        if (!target)
            continue;

        ThreatTarget threatTarget;
        threatTarget.target = target;
        threatTarget.info = info;

        AnalyzeTargetThreat(target, threatTarget);
        ClassifyThreatPriority(threatTarget);

        targets.push_back(threatTarget);

        // Update analysis statistics
        analysis.totalThreat += threatTarget.aggregatedThreat;
        analysis.activeTargets++;

        if (threatTarget.info.priority == ThreatPriority::CRITICAL_PRIORITY)
        {
            analysis.criticalTargets++;
            analysis.emergencyResponse = true;
        }
    }

    // Sort targets by priority and threat
    ::std::sort(targets.begin(), targets.end());

    analysis.sortedTargets = ::std::move(targets);
    // Determine primary and secondary targets
    if (!analysis.sortedTargets.empty())
    {
        analysis.primaryTarget = &analysis.sortedTargets[0];
        if (analysis.sortedTargets.size() > 1)
            analysis.secondaryTarget = &analysis.sortedTargets[1];
    }

    // Calculate average threat
    if (analysis.activeTargets > 0)
        analysis.averageThreat = analysis.totalThreat / analysis.activeTargets;

    // Determine threat overload
    analysis.threatOverload = (analysis.criticalTargets > 2) ||
                              (analysis.activeTargets > 5 && analysis.averageThreat > 100.0f);

    // Cache the analysis
    _cachedAnalysis = analysis;
    _analysisTimestamp = now;
    _analysisDirty = false;

    auto endTime = ::std::chrono::high_resolution_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime);
    const_cast<BotThreatManager*>(this)->TrackPerformance(duration, "AnalyzeThreatSituation");

    _metrics.targetAnalyses++;
    return analysis;
}

::std::vector<ThreatTarget> BotThreatManager::GetSortedThreatTargets()
{
    ThreatAnalysis analysis = AnalyzeThreatSituation();
    return analysis.sortedTargets;
}

ThreatTarget* BotThreatManager::GetPrimaryThreatTarget()
{
    ThreatAnalysis analysis = AnalyzeThreatSituation();
    return analysis.primaryTarget;
}

ThreatTarget* BotThreatManager::GetSecondaryThreatTarget()
{
    ThreatAnalysis analysis = AnalyzeThreatSituation();
    return analysis.secondaryTarget;
}

void BotThreatManager::SetTargetPriority(Unit* target, ThreatPriority priority)
{
    if (!target)
        return;

    ObjectGuid targetGuid = target->GetGUID();
    // No lock needed - threat data is per-bot instance data

    auto& info = _threatMap[targetGuid];
    info.targetGuid = targetGuid;
    info.priority = priority;
    info.lastUpdate = GameTime::GetGameTimeMS();

    _analysisDirty = true;
    _metrics.priorityUpdates++;

    TC_LOG_DEBUG("playerbots", "ThreatManager: Set priority {} for target {} by bot {}",
                static_cast<uint32>(priority), target->GetName(), _bot->GetName());
}

ThreatPriority BotThreatManager::GetTargetPriority(Unit* target) const
{
    if (!target)
        return ThreatPriority::IGNORE_PRIORITY;

    ObjectGuid targetGuid = target->GetGUID();
    // No lock needed - threat data is per-bot instance data

    auto info = _threatMap.Get(targetGuid);
    if (info.has_value())
        return info->priority;

    return ThreatPriority::MODERATE_PRIORITY;
}

void BotThreatManager::UpdateTargetPriorities()
{
    // No lock needed - threat data is per-bot instance data
    for (auto& [guid, info] : _threatMap)
    {
        // PHASE 5B: Thread-safe spatial grid validation (replaces ObjectAccessor::GetUnit)
        auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, guid);
        if (!snapshot)
            continue;

        // Get actual Unit* for ThreatCalculator (requires Unit*)
        Unit* target = ObjectAccessor::GetUnit(*_bot, guid);
        if (!target)
            continue;

        ThreatPriority newPriority = ThreatCalculator::DetermineThreatPriority(target);
        if (newPriority != info.priority)
        {
            info.priority = newPriority;
            _analysisDirty = true;
        }
    }
}

void BotThreatManager::UpdateRoleBasedThreat()
{
    // Role-based threat modifications are applied in CalculateRoleModifier()
    // This method can be used for role-specific threat behaviors
    switch (_botRole)
    {
        case ThreatRole::TANK:
            // Tanks should try to maintain aggro on all targets
    for (auto& [guid, info] : _threatMap)
            {
                if (info.threatPercent < 110.0f) // If not solidly ahead
                {
                    info.priority = static_cast<ThreatPriority>(
                        ::std::max(0, static_cast<int>(info.priority) - 1));
                }
            }
            break;

        case ThreatRole::DPS:
            // DPS should avoid pulling aggro
    for (auto& [guid, info] : _threatMap)
            {
                if (info.threatPercent > 90.0f) // Close to pulling aggro
                {
                    info.priority = static_cast<ThreatPriority>(
                        ::std::min(4, static_cast<int>(info.priority) + 1));
                }
            }
            break;

        case ThreatRole::HEALER:
            // Healers should focus on staying alive
    for (auto& [guid, info] : _threatMap)
            {
                // PHASE 5B: Thread-safe spatial grid validation (replaces ObjectAccessor::GetUnit)
                auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, guid);
                if (!snapshot)
                    continue;

                // Get actual Unit* to check victim
                Unit* target = ObjectAccessor::GetUnit(*_bot, guid);
                if (target && target->GetVictim() == _bot)
                {
                    info.priority = ThreatPriority::CRITICAL_PRIORITY;
                }
            }
            break;

        default:
            break;
    }
}

bool BotThreatManager::HasThreat(Unit* target) const
{
    if (!target)
        return false;

    ObjectGuid targetGuid = target->GetGUID();
    // No lock needed - threat data is per-bot instance data

    auto info = _threatMap.Get(targetGuid);
    return info.has_value() && info->isActive && info->threatValue > 0.0f;
}

float BotThreatManager::GetThreat(Unit* target) const
{
    if (!target)
        return 0.0f;

    ObjectGuid targetGuid = target->GetGUID();
    // No lock needed - threat data is per-bot instance data

    auto info = _threatMap.Get(targetGuid);
    if (info.has_value())
        return info->threatValue;

    return 0.0f;
}

float BotThreatManager::GetThreatPercent(Unit* target) const
{
    if (!target)
        return 0.0f;

    ObjectGuid targetGuid = target->GetGUID();
    // No lock needed - threat data is per-bot instance data

    auto info = _threatMap.Get(targetGuid);
    if (info.has_value())
        return info->threatPercent;

    return CalculateThreatPercent(target);
}

::std::vector<Unit*> BotThreatManager::GetAllThreatTargets()
{
    ::std::vector<Unit*> targets;

    // QW-3 FIX: Pre-allocate based on threat map size to avoid reallocations
    targets.reserve(_threatMap.Size());

    // No lock needed - threat data is per-bot instance data
    for (const auto& [guid, info] : _threatMap)
    {
        if (!info.isActive)
            continue;

        // PHASE 5B: Thread-safe spatial grid validation (replaces ObjectAccessor::GetUnit)
        auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, guid);
        if (!snapshot || !snapshot->IsAlive())
            continue;

        // Get actual Unit* for return vector (needed by callers)
        Unit* target = ObjectAccessor::GetUnit(*_bot, guid);
        if (target && target->IsAlive())
            targets.push_back(target);
    }

    return targets;
}

::std::vector<Unit*> BotThreatManager::GetThreatTargetsByPriority(ThreatPriority priority)
{
    ::std::vector<Unit*> targets;

    // QW-3 FIX: Pre-allocate (estimate ~30% of threats match given priority)
    targets.reserve(::std::max(_threatMap.Size() / 3, size_t(5)));

    // No lock needed - threat data is per-bot instance data
    for (const auto& [guid, info] : _threatMap)
    {
        if (!info.isActive || info.priority != priority)
            continue;

        // PHASE 5B: Thread-safe spatial grid validation (replaces ObjectAccessor::GetUnit)
        auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, guid);
        if (!snapshot || !snapshot->IsAlive())
            continue;

        // Get actual Unit* for return vector (needed by callers)
        Unit* target = ObjectAccessor::GetUnit(*_bot, guid);
        if (target && target->IsAlive())
            targets.push_back(target);
    }

    return targets;
}

uint32 BotThreatManager::GetThreatTargetCount() const
{
    // No lock needed - threat data is per-bot instance data

    uint32 count = 0;
    for (const auto& [guid, info] : _threatMap)
    {
        if (info.isActive)
            count++;
    }

    return count;
}

bool BotThreatManager::IsInThreatEmergency() const
{
    ThreatAnalysis analysis = const_cast<BotThreatManager*>(this)->AnalyzeThreatSituation();
    return analysis.emergencyResponse || analysis.threatOverload;
}

::std::vector<Unit*> BotThreatManager::GetEmergencyTargets()
{
    return GetThreatTargetsByPriority(ThreatPriority::CRITICAL_PRIORITY);
}

void BotThreatManager::HandleThreatEmergency()
{
    TC_LOG_WARN("playerbots", "ThreatManager: Bot {} is handling threat emergency",
                _bot ? _bot->GetName() : "null");

    // Implementation would depend on bot role and available abilities
    // This is a hook for emergency threat responses
}

// Event handlers
void BotThreatManager::OnDamageDealt(Unit* target, uint32 damage)
{
    if (!target || damage == 0)
        return;

    float threat = ThreatCalculator::CalculateDamageThreat(damage);
    UpdateThreatValue(target, threat, ThreatType::DAMAGE);
}

void BotThreatManager::OnHealingDone(Unit* target, uint32 healing)
{
    if (!target || healing == 0)
        return;

    float threat = ThreatCalculator::CalculateHealingThreat(healing);
    UpdateThreatValue(target, threat, ThreatType::HEALING);
}

void BotThreatManager::OnSpellInterrupt(Unit* target)
{
    if (!target)
        return;

    ObjectGuid targetGuid = target->GetGUID();
    // No lock needed - threat data is per-bot instance data

    if (_threatMap.Contains(targetGuid))
    {
        ThreatInfo& info = _threatMap[targetGuid];
        info.spellsInterrupted++;
        info.abilitiesUsed++;
    }
}
void BotThreatManager::OnTauntUsed(Unit* target)
{
    if (!target)
        return;

    float threat = 1000.0f; // Taunt generates significant threat
    UpdateThreatValue(target, threat, ThreatType::AGGRO);
}

void BotThreatManager::OnThreatRedirect(Unit* from, Unit* to, float amount)
{
    if (!from || !to)
        return;

    // Reduce threat on 'from' and add to 'to'
    ModifyThreat(from, 1.0f - (amount / 100.0f));
    UpdateThreatValue(to, amount, ThreatType::AGGRO);
}

// Private implementation methods

float BotThreatManager::CalculateBaseThreat(Unit* target) const
{
    if (!target || !_bot)
        return 0.0f;

    // Get base threat from TrinityCore's threat manager
    ::ThreatManager& threatMgr = target->GetThreatManager();
    return threatMgr.GetThreat(_bot);
}

float BotThreatManager::CalculateRoleModifier() const
{
    switch (_botRole)
    {
        case ThreatRole::TANK:
            return 1.5f; // Tanks want high threat
        case ThreatRole::DPS:
            return 1.0f; // Normal threat
        case ThreatRole::HEALER:
            return 0.8f; // Healers want lower threat
        case ThreatRole::SUPPORT:
            return 0.9f; // Support wants slightly lower threat
        default:
            return 1.0f;
    }
}

float BotThreatManager::CalculateDistanceModifier(Unit* target) const
{
    if (!target || !_bot)
        return 1.0f;

    float distance = _bot->GetDistance2d(target);
    // Closer targets are more threatening
    if (distance < 5.0f)
        return 1.5f;
    else if (distance < 15.0f)
        return 1.2f;
    else if (distance < 30.0f)
        return 1.0f;
    else
        return 0.8f;
}

float BotThreatManager::CalculateHealthModifier(Unit* target) const
{
    if (!target)
        return 1.0f;

    float healthPct = target->GetHealthPct();

    // Low health targets are higher priority for finishing
    if (healthPct < 20.0f)
        return 1.4f;
    else if (healthPct < 50.0f)
        return 1.2f;
    else
        return 1.0f;
}

float BotThreatManager::CalculateAbilityModifier(Unit* target) const
{
    if (!target)
        return 1.0f;

    float modifier = 1.0f;

    // Casting targets are higher priority
    if (target->HasUnitState(UNIT_STATE_CASTING))
        modifier *= 1.3f;

    // Targets attacking healers are higher priority
    if (target->GetVictim() && target->GetVictim()->IsPlayer())
    {
        Player* victim = target->GetVictim()->ToPlayer();
        if (victim && (victim->GetClass() == CLASS_PRIEST ||
                      victim->GetClass() == CLASS_PALADIN ||
                      victim->GetClass() == CLASS_SHAMAN ||
                      victim->GetClass() == CLASS_DRUID))
        {
            modifier *= 1.4f;
        }
    }

    return modifier;
}

void BotThreatManager::AnalyzeTargetThreat(Unit* target, ThreatTarget& threatTarget)
{
    if (!target)
        return;

    threatTarget.aggregatedThreat = threatTarget.info.threatValue;
    threatTarget.averageThreatPercent = threatTarget.info.threatPercent;
    threatTarget.botsInCombat = target->IsInCombat() ? 1 : 0;
    threatTarget.requiresAttention =
        (threatTarget.info.priority == ThreatPriority::CRITICAL_PRIORITY) ||
        (threatTarget.info.threatPercent > EMERGENCY_THREAT_THRESHOLD);
}

void BotThreatManager::ClassifyThreatPriority(ThreatTarget& threatTarget)
{
    Unit* target = threatTarget.target;
    if (!target)
        return;

    ThreatPriority priority = ThreatPriority::MODERATE_PRIORITY;

    // Classify based on creature type and abilities
    if (target->GetVictim() == _bot)
        priority = ThreatPriority::HIGH_PRIORITY;

    if (target->HasUnitState(UNIT_STATE_CASTING))
        priority = ThreatPriority::HIGH_PRIORITY;

    if (target->GetHealthPct() < 20.0f)
        priority = ThreatPriority::HIGH_PRIORITY;

    // Emergency situations
    if (threatTarget.info.threatPercent > EMERGENCY_THREAT_THRESHOLD)
        priority = ThreatPriority::CRITICAL_PRIORITY;

    if (_bot->GetHealthPct() < 30.0f && target->GetVictim() == _bot)
        priority = ThreatPriority::CRITICAL_PRIORITY;

    threatTarget.info.priority = priority;
}

void BotThreatManager::UpdateThreatHistory(Unit* target, float threat)
{
    if (!target)
        return;

    ObjectGuid targetGuid = target->GetGUID();
    // BoundedHistory automatically evicts oldest entries when at capacity
    // Use SetMaxSize to ensure proper capacity if not already set
    if (!_threatHistory.Contains(targetGuid))
    {
        BoundedHistory<float> history(THREAT_HISTORY_SIZE);
        history.Push(threat);
        _threatHistory.Insert(targetGuid, ::std::move(history));
    }
    else
    {
        _threatHistory[targetGuid].Push(threat);
    }
}

void BotThreatManager::UpdateThreatTable(uint32 /*diff*/)
{
    uint32 now = GameTime::GetGameTimeMS();

    for (auto& [guid, info] : _threatMap)
    {
        // PHASE 5B: Thread-safe spatial grid validation (replaces ObjectAccessor::GetUnit)
        auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, guid);
        if (!snapshot)
        {
            info.isActive = false;
            continue;
        }

        // Get actual Unit* for threat calculation methods (they require Unit*)
        Unit* target = ObjectAccessor::GetUnit(*_bot, guid);
        if (!target)
        {
            info.isActive = false;
            continue;
        }

        // Update current threat values
        info.threatValue = CalculateThreat(target);
        info.threatPercent = CalculateThreatPercent(target);
        info.distance = ::std::sqrt(_bot->GetExactDist2dSq(target)); // Calculate once from squared 2D distance
        info.isInCombat = target->IsInCombat();
        info.lastPosition = target->GetPosition();
        info.lastUpdate = now;
    }
}

void BotThreatManager::CleanupStaleEntries()
{
    uint32 now = GameTime::GetGameTimeMS();
    const uint32 STALE_THRESHOLD = 30000; // 30 seconds

    // Collect GUIDs to remove (can't modify during iteration)
    ::std::vector<ObjectGuid> toRemove;
    for (auto const& [guid, info] : _threatMap)
    {
        if ((now - info.lastUpdate) > STALE_THRESHOLD)
        {
            toRemove.push_back(guid);
        }
    }

    // Remove stale entries
    for (ObjectGuid const& guid : toRemove)
    {
        _threatHistory.Remove(guid);
        _threatMap.Remove(guid);
        _analysisDirty = true;
    }
}

void BotThreatManager::UpdateDistances()
{
    for (auto& [guid, info] : _threatMap)
    {
        // PHASE 5B: Thread-safe spatial grid query (replaces ObjectAccessor::GetUnit)
        auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, guid);
        if (!snapshot)
            continue;

        // Use snapshot data directly (lock-free)
        info.distance = ::std::sqrt(_bot->GetExactDistSq(snapshot->position)); // Calculate once from squared 3D distance
        info.lastPosition = snapshot->position;
    }
}

void BotThreatManager::UpdateCombatState()
{
    for (auto& [guid, info] : _threatMap)
    {
        // PHASE 5B: Thread-safe spatial grid query (replaces ObjectAccessor::GetUnit)
        auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, guid);
        if (!snapshot)
            continue;

        // Use snapshot data directly (lock-free)
        info.isInCombat = snapshot->isInCombat;
    }
}

void BotThreatManager::TrackPerformance(::std::chrono::microseconds duration, const ::std::string& operation)
{
    _metrics.maxAnalysisTime = ::std::max(_metrics.maxAnalysisTime, duration);

    // Update moving average
    static ::std::chrono::microseconds totalTime{0};
    static uint32 samples = 0;

    totalTime += duration;
    samples++;

    if (samples >= 100) // Reset every 100 samples
    {
        _metrics.averageAnalysisTime = totalTime / samples;
        totalTime = ::std::chrono::microseconds{0};
        samples = 0;
    }
}

// ThreatCalculator implementation

float ThreatCalculator::CalculateDamageThreat(uint32 damage, float modifier)
{
    return static_cast<float>(damage) * modifier;
}

float ThreatCalculator::CalculateHealingThreat(uint32 healing, float modifier)
{
    return static_cast<float>(healing) * modifier;
}

float ThreatCalculator::CalculateSpellThreat(uint32 spellId, float modifier)
{
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return 0.0f;

    // Base threat calculation based on spell effects
    float baseThreat = 100.0f; // Default threat value

    // Adjust based on spell school and effects
    for (auto const& effect : spellInfo->GetEffects())
    {
        switch (effect.Effect)
        {
            case SPELL_EFFECT_SCHOOL_DAMAGE:
                baseThreat += effect.CalcValue() * 0.5f;
                break;
            case SPELL_EFFECT_HEAL:
                baseThreat += effect.CalcValue() * 0.3f;
                break;
            case SPELL_EFFECT_APPLY_AURA:
                baseThreat += 50.0f;
                break;
            default:
                break;
        }
    }

    return baseThreat * modifier;
}

ThreatPriority ThreatCalculator::DetermineThreatPriority(Unit* target)
{
    if (!target)
        return ThreatPriority::IGNORE_PRIORITY;

    // Base priority on creature type and abilities
    if (target->GetCreatureType() == CREATURE_TYPE_HUMANOID)
    {
        if (target->HasUnitState(UNIT_STATE_CASTING))
            return ThreatPriority::HIGH_PRIORITY;
    }

    if (target->GetHealthPct() < 20.0f)
        return ThreatPriority::HIGH_PRIORITY;

    return ThreatPriority::MODERATE_PRIORITY;
}

float ThreatCalculator::GetClassThreatModifier(uint8 classId)
{
    switch (classId)
    {
        case CLASS_WARRIOR:
        case CLASS_PALADIN:
        case CLASS_DEATH_KNIGHT:
            return 1.3f; // Tanks generate more threat
        case CLASS_ROGUE:
        case CLASS_HUNTER:
            return 0.9f; // Stealth/ranged classes generate less
        case CLASS_MAGE:
        case CLASS_WARLOCK:
            return 1.1f; // Casters generate moderate threat
        default:
            return 1.0f;
    }
}

bool ThreatCalculator::IsValidThreatTarget(Unit* target)
{
    if (!target)
        return false;

    if (!target->IsAlive())
        return false;

    if (target->HasUnitFlag(UNIT_FLAG_NON_ATTACKABLE))
        return false;

    if (Creature* creature = target->ToCreature())
        if (creature->IsInEvadeMode())
            return false;

    return true;
}

// ============================================================================
// ICombatEventSubscriber Implementation (Phase 3 Event-Driven Architecture)
// ============================================================================

CombatEventType BotThreatManager::GetSubscribedEventTypes() const
{
    // Subscribe to all events that affect threat tracking:
    // - DAMAGE_DEALT: Bot deals damage, generates threat
    // - DAMAGE_TAKEN: Bot takes damage, enemy has threat on bot
    // - HEALING_DONE: Healing generates threat
    // - THREAT_CHANGED: Direct threat table updates
    // - UNIT_DIED: Remove dead units from tracking
    return CombatEventType::DAMAGE_DEALT |
           CombatEventType::DAMAGE_TAKEN |
           CombatEventType::HEALING_DONE |
           CombatEventType::THREAT_CHANGED |
           CombatEventType::UNIT_DIED;
}

bool BotThreatManager::ShouldReceiveEvent(const CombatEvent& event) const
{
    if (!_bot)
        return false;

    ObjectGuid botGuid = _bot->GetGUID();

    // Only receive events involving this specific bot
    if (!event.source.IsEmpty() && event.source == botGuid)
        return true;

    if (!event.target.IsEmpty() && event.target == botGuid)
        return true;

    return false;
}

void BotThreatManager::OnCombatEvent(const CombatEvent& event)
{
    // Route event to appropriate handler based on type
    switch (event.type)
    {
        case CombatEventType::DAMAGE_DEALT:
            HandleDamageDealt(event);
            break;

        case CombatEventType::DAMAGE_TAKEN:
            HandleDamageTaken(event);
            break;

        case CombatEventType::HEALING_DONE:
            HandleHealingDone(event);
            break;

        case CombatEventType::THREAT_CHANGED:
            HandleThreatChanged(event);
            break;

        case CombatEventType::UNIT_DIED:
            HandleUnitDied(event);
            break;

        default:
            // Ignore unhandled event types
            break;
    }
}

void BotThreatManager::HandleDamageDealt(const CombatEvent& event)
{
    // When bot deals damage, update threat on the target
    if (!_bot || event.source != _bot->GetGUID())
        return;

    if (event.target.IsEmpty() || event.amount == 0)
        return;

    // Find target unit
    Unit* target = ObjectAccessor::GetUnit(*_bot, event.target);
    if (!target)
        return;

    // Use existing OnDamageDealt method
    OnDamageDealt(target, event.amount);

    // Mark data as dirty
    _threatDataDirty = true;
}

void BotThreatManager::HandleDamageTaken(const CombatEvent& event)
{
    // When bot takes damage, the attacker has threat on the bot
    if (!_bot || event.target != _bot->GetGUID())
        return;

    if (event.source.IsEmpty())
        return;

    // The attacker is generating threat by attacking us
    // This is mainly for tracking who is attacking the bot

    // Mark data as dirty - we may need to recalculate priorities
    _threatDataDirty = true;
    _analysisDirty = true;
}

void BotThreatManager::HandleHealingDone(const CombatEvent& event)
{
    // When bot heals, it generates threat on all enemies in combat
    if (!_bot || event.source != _bot->GetGUID())
        return;

    if (event.amount == 0)
        return;

    // Healing generates threat - find target if specified
    Unit* target = nullptr;
    if (!event.target.IsEmpty())
    {
        target = ObjectAccessor::GetUnit(*_bot, event.target);
    }

    if (target)
    {
        // Use existing OnHealingDone method
        OnHealingDone(target, event.amount);
    }

    // Mark data as dirty
    _threatDataDirty = true;
}

void BotThreatManager::HandleThreatChanged(const CombatEvent& event)
{
    // Direct threat table update from TrinityCore ThreatManager
    if (!_bot)
        return;

    ObjectGuid botGuid = _bot->GetGUID();

    // Only care if the bot is on someone's threat list
    if (event.source != botGuid)
        return;

    // event.target is the enemy whose threat table changed
    if (event.target.IsEmpty())
        return;

    // Update our tracking to match the new threat value
    Unit* target = ObjectAccessor::GetUnit(*_bot, event.target);
    if (!target)
        return;

    // Update threat value directly
    float threatDelta = event.newThreat - event.oldThreat;
    if (::std::abs(threatDelta) > 0.1f)
    {
        // Significant threat change
        UpdateThreatValue(target, event.newThreat, ThreatType::DAMAGE);
        _threatDataDirty = true;
        _analysisDirty = true;
    }
}

void BotThreatManager::HandleUnitDied(const CombatEvent& event)
{
    // When a unit dies, remove it from threat tracking
    if (event.source.IsEmpty())
        return;

    // Check if this unit was in our threat map
    if (_threatMap.Contains(event.source))
    {
        _threatMap.Remove(event.source);
        _threatHistory.Remove(event.source);
        _threatDataDirty = true;
        _analysisDirty = true;

        TC_LOG_DEBUG("playerbots", "BotThreatManager: Removed dead unit {} from threat tracking",
            event.source.ToString());
    }
}

} // namespace Playerbot