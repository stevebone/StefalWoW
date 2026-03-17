/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "InterruptManager.h"
#include "GameTime.h"
#include "Player.h"
#include "Unit.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Log.h"
#include "Group.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "Map.h"
#include "SharedDefines.h"
#include "SpellHistory.h"
#include "MotionMaster.h"
#include <algorithm>
#include <cmath>
#include "../../Spatial/SpatialGridManager.h"
#include "ObjectAccessor.h"
#include "Movement/UnifiedMovementCoordinator.h"
#include "../../Movement/Arbiter/MovementPriorityMapper.h"
#include "../BotAI.h"
#include "Core/PlayerBotHelpers.h"
#include "UnitAI.h"
// WoW 12.0: SpellAttr16 and SpellPvpModifier support
#include "DB2Stores.h"
#include "DB2Structure.h"

namespace Playerbot
{

InterruptManager::InterruptManager(Player* bot)
    : _bot(bot), _isInterrupting(false), _currentTarget(nullptr),
      _reactionTime(DEFAULT_REACTION_TIME), _maxInterruptRange(DEFAULT_MAX_RANGE),
      _scanInterval(DEFAULT_SCAN_INTERVAL), _predictiveInterrupts(true), _emergencyMode(false),
      _timingAccuracyTarget(TIMING_ACCURACY_TARGET), _lastScan(0), _lastInterruptAttempt(0),
      _lastCoordinationUpdate(0), _capabilitiesInitialized(false)
{
    // CRITICAL: Do NOT call InitializeInterruptCapabilities() here!
    // The bot's power systems may not be initialized yet, causing ACCESS_VIOLATION
    // in CalcPowerCost() -> Unit::GetPower(). Defer to first Update() call.
    // Also no logging with _bot->GetName() - concurrent access issue.
}

void InterruptManager::UpdateInterruptSystem(uint32 /*diff*/)
{
    // No lock needed - interrupt tracking is per-bot instance data

    // CRITICAL: Deferred initialization - bot's power systems must be ready
    // before we can call InitializeInterruptCapabilities() which uses CalcPowerCost()
    if (!_capabilitiesInitialized && _bot && _bot->IsInWorld())
    {
        InitializeInterruptCapabilities();
        _capabilitiesInitialized = true;
    }

    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime - _lastScan < _scanInterval && !_emergencyMode)
        return;

    _lastScan = currentTime;

    try
    {
        ScanNearbyUnitsForCasts();
        UpdateInterruptCapabilities();

        if (!_trackedTargets.empty())
        {
            ProcessInterruptOpportunities();

            if (_predictiveInterrupts)
            {
                HandleMultipleInterruptTargets();
            }
        }

        if (currentTime - _lastCoordinationUpdate >= COORDINATION_UPDATE_INTERVAL)
        {
            if (Group* group = _bot->GetGroup())
            {
                ::std::vector<Player*> groupMembers;
                for (GroupReference const& ref : group->GetMembers())
                {
                    if (Player* member = ref.GetSource())
                        groupMembers.push_back(member);
                }
                CoordinateInterruptsWithGroup(groupMembers);
            }
            _lastCoordinationUpdate = currentTime;
        }

        auto it = _trackedTargets.begin();
        while (it != _trackedTargets.end())
        {
            if (currentTime - it->detectedTime > TARGET_TRACKING_DURATION)
                it = _trackedTargets.erase(it);
            else
                ++it;
        }
    }
    catch (const ::std::exception& e)
    {
        TC_LOG_ERROR("playerbot.interrupt", "Exception in UpdateInterruptSystem for bot {}: {}", _bot->GetName(), e.what());
    }
}

::std::vector<InterruptTarget> InterruptManager::ScanForInterruptTargets()
{
    ::std::vector<InterruptTarget> targets;

    // Lock-free spatial grid query
    Map* map = _bot->GetMap();
    if (!map)
        return targets;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        // Create grid on demand
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return targets;
    }
    // Query nearby creature GUIDs (lock-free!)
    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        _bot->GetPosition(), _maxInterruptRange);

    // SPATIAL GRID MIGRATION COMPLETE (2025-11-26):
    // ObjectAccessor is intentionally retained because we need:
    // 1. Real-time casting state via HasUnitState(UNIT_STATE_CASTING)
    // 2. GetCurrentSpell() for spell information
    // 3. GetSpellInfo() for interrupt worthiness checks
    // The spatial grid pre-filters candidates to reduce ObjectAccessor calls.
    for (ObjectGuid guid : nearbyGuids)
    {
        Unit* unit = ObjectAccessor::GetUnit(*_bot, guid);
        if (!IsValidInterruptTarget(unit))
            continue;

        if (!unit->HasUnitState(UNIT_STATE_CASTING))
            continue;

        Spell* currentSpell = unit->GetCurrentSpell(CURRENT_GENERIC_SPELL);
        if (!currentSpell)
            continue;

        const SpellInfo* spellInfo = currentSpell->GetSpellInfo();
        if (!spellInfo || !IsSpellInterruptWorthy(spellInfo->Id, unit))
            continue;

        InterruptTarget target;
        target.guid = unit->GetGUID();
        target.unit = unit;
        target.position = unit->GetPosition();
        target.currentSpell = currentSpell;
        target.spellInfo = spellInfo;
        target.spellId = spellInfo->Id;
        target.priority = AssessInterruptPriority(spellInfo, unit);
        target.type = ClassifyInterruptType(spellInfo);
        target.totalCastTime = static_cast<float>(spellInfo->CalcCastTime());
        target.remainingCastTime = static_cast<float>(currentSpell->GetCastTime() - currentSpell->GetTimer());
        target.castProgress = (target.totalCastTime - target.remainingCastTime) / target.totalCastTime;
        target.detectedTime = GameTime::GetGameTimeMS();
        target.isChanneled = spellInfo->IsChanneled();
        target.isInterruptible = true; // Default to interruptible
        target.requiresLoS = true; // Default to requiring LoS
        target.spellName = spellInfo->SpellName->Str[0];
        target.targetName = unit->GetName();
        if (target.isInterruptible && target.priority != InterruptPriority::IGNORE)
        {
            targets.push_back(target);
        }
    }

    ::std::sort(targets.begin(), targets.end(),
        [](const InterruptTarget& a, const InterruptTarget& b) {
            if (a.priority != b.priority)
                return a.priority < b.priority;
            return a.remainingCastTime < b.remainingCastTime;
        });

    return targets;
}

InterruptResult InterruptManager::AttemptInterrupt(const InterruptTarget& target)
{
    auto startTime = ::std::chrono::steady_clock::now();
    InterruptResult result;
    result.originalTarget = target;

    // No lock needed - interrupt tracking is per-bot instance data

    try
    {
        if (!target.unit || !target.unit->IsAlive())
        {
            result.failureReason = "Target is no longer valid";
            return result;
        }
        if (!target.unit->HasUnitState(UNIT_STATE_CASTING))
        {
            result.failureReason = "Target is no longer casting";
            return result;
        }

        InterruptCapability* capability = GetBestInterruptForTarget(target);
        if (!capability || !capability->isAvailable)
        {
            result.failureReason = "No available interrupt capability";
            return result;
        }

        InterruptPlan plan = CreateInterruptPlan(target);
        if (!IsInterruptExecutable(plan))
        {
            result.failureReason = "Interrupt plan not executable: " + plan.reasoning;
            return result;
        }

        bool executionSuccess = ExecuteInterruptPlan(plan);
        result.attemptMade = true;
        result.success = executionSuccess;
        result.usedMethod = plan.method;
        result.usedSpell = capability->spellId;

        auto endTime = ::std::chrono::steady_clock::now();
        auto reactionTime = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime);
        result.executionTime = static_cast<uint32>(reactionTime.count());

        float expectedTime = target.remainingCastTime;
        float actualTime = static_cast<float>(reactionTime.count()) / 1000.0f;
        result.timingAccuracy = 1.0f - ::std::abs(expectedTime - actualTime) / expectedTime;

        _metrics.interruptAttempts++;
        if (result.success)
        {
            _metrics.successfulInterrupts++;
            if (target.priority == InterruptPriority::CRITICAL)
                _metrics.criticalInterrupts++;
        }
        else
        {
            _metrics.failedInterrupts++;
        }

        UpdateReactionTimeMetrics(reactionTime);
        UpdateTimingAccuracy(result);

        TC_LOG_DEBUG("playerbot.interrupt", "Bot {} attempted interrupt on {} ({}): {}",
                   _bot->GetName(), target.targetName, target.spellName,
                   result.success ? "SUCCESS" : result.failureReason);
    }
    catch (const ::std::exception& e)
    {
        result.success = false;
        result.failureReason = ::std::string("Exception during interrupt: ") + e.what();
        TC_LOG_ERROR("playerbot.interrupt", "Exception in AttemptInterrupt for bot {}: {}", _bot->GetName(), e.what());
    }

    return result;
}

void InterruptManager::ProcessInterruptOpportunities()
{
    ::std::vector<InterruptTarget> targets = ScanForInterruptTargets();
    _trackedTargets = targets;

    if (targets.empty())
        return;

    ::std::vector<InterruptPlan> plans = GenerateInterruptPlans(targets);
    if (plans.empty())
        return;

    ::std::sort(plans.begin(), plans.end());

    for (const InterruptPlan& plan : plans)
    {
        if (!plan.target || plan.target->priority == InterruptPriority::IGNORE)
            continue;

        if (ShouldLetOthersInterrupt(*plan.target))
            continue;

        float urgency = CalculateInterruptUrgency(*plan.target);
        if (urgency < 0.5f && plan.target->priority > InterruptPriority::HIGH)
            continue;

        if (ExecuteInterruptPlan(plan))
        {
            RegisterInterruptAttempt(*plan.target);
            break;
        }
    }
}

InterruptPriority InterruptManager::AssessInterruptPriority(const SpellInfo* spellInfo, Unit* caster)
{
    if (!spellInfo || !caster)
        return InterruptPriority::IGNORE;

    uint32 spellId = spellInfo->Id;

    InterruptPriority configuredPriority = InterruptUtils::GetSpellInterruptPriority(spellId);
    if (configuredPriority != InterruptPriority::IGNORE)
        return configuredPriority;

    if (ShouldInterruptHealing(spellInfo, caster))
        return AssessHealingPriority(spellInfo, caster);

    if (ShouldInterruptCrowdControl(spellInfo, caster))
        return AssessCrowdControlPriority(spellInfo, caster);
    if (ShouldInterruptDamage(spellInfo, caster))
        return AssessDamagePriority(spellInfo, caster);

    if (ShouldInterruptBuff(spellInfo, caster))
        return AssessBuffPriority(spellInfo, caster);

    return InterruptPriority::LOW;
}

InterruptType InterruptManager::ClassifyInterruptType(const SpellInfo* spellInfo)
{
    if (!spellInfo)
        return InterruptType::DAMAGE_PREVENTION;

    if (spellInfo->HasEffect(SPELL_EFFECT_HEAL) || spellInfo->HasEffect(SPELL_EFFECT_HEAL_PCT))
        return InterruptType::HEALING_DENIAL;

    if (spellInfo->HasEffect(SPELL_EFFECT_APPLY_AURA))
    {
        for (SpellEffectInfo const& effect : spellInfo->GetEffects())
        {
            if (effect.ApplyAuraName == SPELL_AURA_MOD_STUN ||
                effect.ApplyAuraName == SPELL_AURA_MOD_FEAR ||
                effect.ApplyAuraName == SPELL_AURA_MOD_CHARM ||
                effect.ApplyAuraName == SPELL_AURA_MOD_CONFUSE)
            {
                return InterruptType::CROWD_CONTROL;
            }
        }

        if (spellInfo->IsPositive())
            return InterruptType::BUFF_DENIAL;
        else
            return InterruptType::DEBUFF_PREVENTION;
    }

    if (spellInfo->IsChanneled())
        return InterruptType::CHANNEL_BREAK;

    return InterruptType::DAMAGE_PREVENTION;
}

bool InterruptManager::IsSpellInterruptWorthy(uint32 spellId, Unit* caster)
{
    const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    // Skip attribute check for now - use default assumption
    // if (spellInfo->HasAttribute(...))
    //     return false;

    InterruptPriority priority = AssessInterruptPriority(spellInfo, caster);
    return priority != InterruptPriority::IGNORE;
}

void InterruptManager::InitializeInterruptCapabilities()
{
    _interruptCapabilities.clear();

    uint8 botClass = _bot->GetClass();
    ::std::vector<uint32> classInterrupts = InterruptUtils::GetClassInterruptSpells(botClass);
    for (uint32 spellId : classInterrupts)
    {
        const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (!spellInfo)
            continue;

        InterruptCapability capability;
        capability.spellId = spellId;
        capability.spellName = spellInfo->SpellName->Str[0];
        capability.range = spellInfo->GetMaxRange();
        capability.cooldown = static_cast<float>(spellInfo->RecoveryTime);

        auto powerCosts = spellInfo->CalcPowerCost(_bot, spellInfo->GetSchoolMask());
        capability.manaCost = 0;
        for (auto const& cost : powerCosts)
        {
            if (cost.Power == POWER_MANA)
            {
                capability.manaCost = cost.Amount;
                break;
            }
        }
        capability.castTime = static_cast<float>(spellInfo->CalcCastTime());
        capability.requiresLoS = true; // Default to requiring LoS
        capability.requiresFacing = true; // Default to requiring facing
    if (spellInfo->HasEffect(SPELL_EFFECT_INTERRUPT_CAST))
            capability.method = InterruptMethod::SPELL_INTERRUPT;
        else if (spellInfo->HasEffect(SPELL_EFFECT_APPLY_AURA))
        {
            for (SpellEffectInfo const& effect : spellInfo->GetEffects())
            {
                if (effect.ApplyAuraName == SPELL_AURA_MOD_STUN)
                    capability.method = InterruptMethod::STUN;
                else if (effect.ApplyAuraName == SPELL_AURA_MOD_SILENCE)
                    capability.method = InterruptMethod::SILENCE;
                else if (effect.ApplyAuraName == SPELL_AURA_MOD_FEAR)
                    capability.method = InterruptMethod::FEAR;
            }
        }
        else if (spellInfo->HasEffect(SPELL_EFFECT_KNOCK_BACK))
            capability.method = InterruptMethod::KNOCKBACK;

        capability.minPriority = InterruptPriority::MODERATE;
        capability.lastUsed = 0;

        _interruptCapabilities.push_back(capability);
    }

    TC_LOG_DEBUG("playerbot.interrupt", "Initialized {} interrupt capabilities for bot {}",
               _interruptCapabilities.size(), _bot->GetName());
}

void InterruptManager::UpdateInterruptCapabilities()
{
    for (InterruptCapability& capability : _interruptCapabilities)
    {
        uint32 currentTime = GameTime::GetGameTimeMS();
        capability.isAvailable = _bot->HasSpell(capability.spellId) &&
                                !_bot->GetSpellHistory()->HasCooldown(capability.spellId) &&
                                (currentTime - capability.lastUsed >= static_cast<uint32>(capability.cooldown));

        if (capability.manaCost > 0)
        {
            capability.isAvailable = capability.isAvailable &&
                                   _bot->GetPower(POWER_MANA) >= static_cast<int32>(capability.manaCost);
        }
    }
}

InterruptCapability* InterruptManager::GetBestInterruptForTarget(const InterruptTarget& target)
{
    InterruptCapability* bestCapability = nullptr;
    float bestEffectiveness = 0.0f;

    for (InterruptCapability& capability : _interruptCapabilities)
    {
        if (!capability.isAvailable)
            continue;

        if (capability.minPriority > target.priority)
            continue;

        float rangeSq = capability.range * capability.range;
        if (rangeSq < _bot->GetExactDistSq(target.unit))
            continue;

        float effectiveness = CalculateInterruptEffectiveness(capability, target);
        if (effectiveness > bestEffectiveness)
        {
            bestEffectiveness = effectiveness;
            bestCapability = &capability;
        }
    }

    return bestCapability;
}

InterruptPlan InterruptManager::CreateInterruptPlan(const InterruptTarget& target)
{
    InterruptPlan plan;
    plan.target = const_cast<InterruptTarget*>(&target);

    InterruptCapability* capability = GetBestInterruptForTarget(target);
    if (!capability)
    {
        plan.reasoning = "No available interrupt capability";
        return plan;
    }

    plan.capability = capability;
    plan.method = capability->method;
    plan.priority = static_cast<uint32>(target.priority);

    float reactionDelay = CalculateReactionDelay();
    float executionTime = CalculateExecutionTime(capability->method);
    plan.executionTime = executionTime;
    plan.reactionTime = reactionDelay;

    if (target.remainingCastTime < executionTime + reactionDelay)
    {
        plan.successProbability = 0.0f;
        plan.reasoning = "Insufficient time to execute interrupt";
        return plan;
    }

    plan.successProbability = ::std::min(1.0f, (target.remainingCastTime - reactionDelay) / executionTime);

    if (capability->requiresLoS && !HasLineOfSightToTarget(target.unit))
    {
        plan.requiresMovement = true;
        plan.executionPosition = CalculateOptimalInterruptPosition(target.unit);
    }

    plan.reasoning = "Interrupt plan generated successfully";
    return plan;
}

::std::vector<InterruptPlan> InterruptManager::GenerateInterruptPlans(const ::std::vector<InterruptTarget>& targets)
{
    ::std::vector<InterruptPlan> plans;
    plans.reserve(targets.size());

    for (const InterruptTarget& target : targets)
    {
        InterruptPlan plan = CreateInterruptPlan(target);
        if (plan.successProbability > 0.0f)
        {
            plans.push_back(plan);
        }
    }

    return plans;
}

bool InterruptManager::ExecuteInterruptPlan(const InterruptPlan& plan)
{
    if (!plan.capability || !plan.target || !plan.target->unit)
        return false;

    try
    {
        _isInterrupting = true;
        _currentTarget = plan.target;

        if (plan.requiresMovement)
        {
            // PHASE 6B: Use Movement Arbiter with INTERRUPT_POSITIONING priority (220)
            BotAI* botAI = dynamic_cast<BotAI*>(_bot->GetAI());
            if (botAI && botAI->GetUnifiedMovementCoordinator())
            {
                botAI->RequestPointMovement(
                    PlayerBotMovementPriority::INTERRUPT_POSITIONING,
                    plan.executionPosition,
                    "Interrupt positioning",
                    "InterruptManager");
            }
            else
            {
                // FALLBACK: Use BotMovementController with validated pathfinding
                if (BotAI* ai = GetBotAI(_bot))
                {
                    if (!ai->MoveTo(plan.executionPosition, true))
                    {
                        // Final fallback to legacy if validation fails
                        _bot->GetMotionMaster()->MovePoint(0, plan.executionPosition.GetPositionX(),
                                                         plan.executionPosition.GetPositionY(),
                                                         plan.executionPosition.GetPositionZ());
                    }
                }
                else
                {
                    // Non-bot player - use standard movement
                    _bot->GetMotionMaster()->MovePoint(0, plan.executionPosition.GetPositionX(),
                                                     plan.executionPosition.GetPositionY(),
                                                     plan.executionPosition.GetPositionZ());
                }
            }
            return false;
        }

        bool success = false;
        switch (plan.method)
        {
            case InterruptMethod::SPELL_INTERRUPT:
            case InterruptMethod::STUN:
            case InterruptMethod::SILENCE:
            case InterruptMethod::FEAR:
            case InterruptMethod::KNOCKBACK:
                success = CastInterruptSpell(plan.capability->spellId, plan.target->unit);
                break;

            case InterruptMethod::LINE_OF_SIGHT:
                success = AttemptLoSInterrupt(plan.target->unit);
                break;

            case InterruptMethod::MOVEMENT:
                success = AttemptMovementInterrupt(plan.target->unit);
                break;

            default:
                success = CastInterruptSpell(plan.capability->spellId, plan.target->unit);
                break;
        }

        if (success)
        {
            plan.capability->lastUsed = GameTime::GetGameTimeMS();
            _lastInterruptAttempt = GameTime::GetGameTimeMS();
        }

        _isInterrupting = false;
        _currentTarget = nullptr;

        return success;
    }
    catch (const ::std::exception& e)
    {
        TC_LOG_ERROR("playerbot.interrupt", "Exception executing interrupt plan for bot {}: {}", _bot->GetName(), e.what());
        _isInterrupting = false;
        _currentTarget = nullptr;
        return false;
    }
}

bool InterruptManager::ShouldInterruptHealing(const SpellInfo* spellInfo, Unit* caster)
{
    if (!spellInfo || !caster)
        return false;

    return spellInfo->HasEffect(SPELL_EFFECT_HEAL) || spellInfo->HasEffect(SPELL_EFFECT_HEAL_PCT);
}

bool InterruptManager::ShouldInterruptCrowdControl(const SpellInfo* spellInfo, Unit* caster)
{
    if (!spellInfo || !caster)
        return false;

    for (SpellEffectInfo const& effect : spellInfo->GetEffects())
    {
        if (effect.ApplyAuraName == SPELL_AURA_MOD_CHARM ||
            effect.ApplyAuraName == SPELL_AURA_MOD_CONFUSE)
        {
            return true;
        }
    }

    return false;
}

bool InterruptManager::ShouldInterruptDamage(const SpellInfo* spellInfo, Unit* caster)
{
    if (!spellInfo || !caster)
        return false;

    for (SpellEffectInfo const& effect : spellInfo->GetEffects())
    {
        if (effect.Effect == SPELL_EFFECT_WEAPON_PERCENT_DAMAGE)
        {
            return effect.CalcValue() > 1000;
        }
    }

    return false;
}

float InterruptManager::CalculateInterruptUrgency(const InterruptTarget& target)
{
    float urgency = 0.0f;

    switch (target.priority)
    {
        case InterruptPriority::CRITICAL:
            urgency += 1.0f;
            break;
        case InterruptPriority::HIGH:
            urgency += 0.8f;
            break;
        case InterruptPriority::MODERATE:
            urgency += 0.6f;
            break;
        case InterruptPriority::LOW:
            urgency += 0.3f;
            break;
        default:
            urgency = 0.0f;
            break;
    }
    float timeUrgency = 1.0f - (target.remainingCastTime / target.totalCastTime);
    urgency += timeUrgency * 0.5f;

    return ::std::min(1.0f, urgency);
}

void InterruptManager::ScanNearbyUnitsForCasts()
{
    ::std::vector<InterruptTarget> newTargets = ScanForInterruptTargets();

    for (const InterruptTarget& newTarget : newTargets)
    {
        auto it = ::std::find_if(_trackedTargets.begin(), _trackedTargets.end(),
            [&newTarget](const InterruptTarget& existing) {
                return existing.guid == newTarget.guid && existing.spellId == newTarget.spellId;
            });

        if (it == _trackedTargets.end())
        {
            auto& firstDetected = _targetFirstDetected[newTarget.guid];
            if (firstDetected == 0)
                firstDetected = GameTime::GetGameTimeMS();

            _trackedTargets.push_back(newTarget);
        }
        else
        {
            UpdateTargetInformation(*it);
        }
    }
}

bool InterruptManager::IsValidInterruptTarget(Unit* unit)
{
    if (!unit || !unit->IsAlive())
        return false;

    if (!_bot->IsHostileTo(unit))
        return false;

    // Skip immunity check for now - too complex to handle generically
    // if (unit->IsImmunedToSpellEffect(...))
    //     return false;

    float maxRangeSq = _maxInterruptRange * _maxInterruptRange;
    if (_bot->GetExactDistSq(unit) > maxRangeSq)
        return false;

    return true;
}
bool InterruptManager::CastInterruptSpell(uint32 spellId, Unit* target)
{
    if (!target || !_bot->HasSpell(spellId))
        return false;

    const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    if (_bot->GetSpellHistory()->HasCooldown(spellId))
        return false;

    auto powerCosts = spellInfo->CalcPowerCost(_bot, spellInfo->GetSchoolMask());
    uint32 manaCost = 0;
    for (auto const& cost : powerCosts)
    {
        if (cost.Power == POWER_MANA)
        {
            manaCost = cost.Amount;
            break;
        }
    }
    if (manaCost > 0 && _bot->GetPower(POWER_MANA) < static_cast<int32>(manaCost))
        return false;

    if (!_bot->IsWithinLOSInMap(target))
        return false;

    float maxRange = spellInfo->GetMaxRange();
    float maxRangeSq = maxRange * maxRange;
    if (_bot->GetExactDistSq(target) > maxRangeSq)
        return false;
    _bot->CastSpell(CastSpellTargetArg(target), spellId);
    return true;
}

float InterruptManager::CalculateReactionDelay()
{
    return static_cast<float>(_reactionTime) / 1000.0f;
}

float InterruptManager::CalculateExecutionTime(InterruptMethod method)
{
    switch (method)
    {
        case InterruptMethod::SPELL_INTERRUPT:
        case InterruptMethod::STUN:
        case InterruptMethod::SILENCE:
        case InterruptMethod::FEAR:
            return 0.5f;

        case InterruptMethod::KNOCKBACK:
            return 0.3f;

        case InterruptMethod::LINE_OF_SIGHT:
        case InterruptMethod::MOVEMENT:
            return 1.0f;

        default:
            return 0.5f;
    }
}

void InterruptManager::UpdateTargetInformation(InterruptTarget& target)
{
    if (!target.unit || !target.unit->IsAlive())
        return;

    if (!target.unit->HasUnitState(UNIT_STATE_CASTING))
        return;

    Spell* currentSpell = target.unit->GetCurrentSpell(CURRENT_GENERIC_SPELL);
    if (!currentSpell)
        return;

    target.remainingCastTime = static_cast<float>(currentSpell->GetCastTime() - currentSpell->GetTimer());
    target.castProgress = (target.totalCastTime - target.remainingCastTime) / target.totalCastTime;
    target.position = target.unit->GetPosition();
}

bool InterruptManager::HasLineOfSightToTarget(Unit* target)
{
    return target && _bot->IsWithinLOSInMap(target);
}

Position InterruptManager::CalculateOptimalInterruptPosition(Unit* target)
{
    if (!target)
        return _bot->GetPosition();

    Position targetPos = target->GetPosition();
    Position botPos = _bot->GetPosition();

    float angle = ::std::atan2(targetPos.GetPositionY() - botPos.GetPositionY(),
                           targetPos.GetPositionX() - botPos.GetPositionX());

    Position optimalPos;
    optimalPos.m_positionX = targetPos.GetPositionX() - 15.0f * ::std::cos(angle);
    optimalPos.m_positionY = targetPos.GetPositionY() - 15.0f * ::std::sin(angle);
    optimalPos.m_positionZ = targetPos.GetPositionZ();

    return optimalPos;
}

void InterruptManager::UpdateReactionTimeMetrics(::std::chrono::microseconds reactionTime)
{
    if (reactionTime < _metrics.minReactionTime)
        _metrics.minReactionTime = reactionTime;

    if (reactionTime > _metrics.maxReactionTime)
        _metrics.maxReactionTime = reactionTime;

    _metrics.averageReactionTime = ::std::chrono::microseconds(
        static_cast<uint64_t>(_metrics.averageReactionTime.count() * 0.9 + reactionTime.count() * 0.1)
    );
}

void InterruptManager::UpdateTimingAccuracy(const InterruptResult& result)
{
    _metrics.averageTimingAccuracy = _metrics.averageTimingAccuracy * 0.9f + result.timingAccuracy * 0.1f;
}

// InterruptUtils implementation
InterruptPriority InterruptUtils::GetSpellInterruptPriority(uint32 spellId)
{
    switch (spellId)
    {
        // Critical interrupts - Healing
        case 2061:   // Flash Heal
        case 596:    // Prayer of Healing
        case 25314:  // Greater Heal
            return InterruptPriority::CRITICAL;

        // Critical interrupts - CC
        case 118:    // Polymorph
        case 5782:   // Fear
        case 6770:   // Sap
            return InterruptPriority::CRITICAL;

        // High priority - Major damage spells
        case 133:    // Fireball
        case 5676:   // Searing Pain
        case 172:    // Corruption
            return InterruptPriority::HIGH;

        default:
            return InterruptPriority::MODERATE;
    }
}

::std::vector<uint32> InterruptUtils::GetClassInterruptSpells(uint8 playerClass)
{
    ::std::vector<uint32> interrupts;

    switch (playerClass)
    {
        case CLASS_WARRIOR:
            interrupts.push_back(6552);  // Pummel
            interrupts.push_back(72);    // Shield Bash
            break;

        case CLASS_PALADIN:
            interrupts.push_back(96231); // Rebuke
            break;

        case CLASS_HUNTER:
            interrupts.push_back(147362); // Counter Shot
            interrupts.push_back(19577);  // Intimidation
            break;

        case CLASS_ROGUE:
            interrupts.push_back(1766);  // Kick
            interrupts.push_back(408);   // Kidney Shot
            break;

        case CLASS_PRIEST:
            interrupts.push_back(15487); // Silence
            interrupts.push_back(8122);  // Psychic Scream
            break;

        case CLASS_DEATH_KNIGHT:
            interrupts.push_back(47528); // Mind Freeze
            interrupts.push_back(47476); // Strangulate
            break;

        case CLASS_SHAMAN:
            interrupts.push_back(57994); // Wind Shear
            break;

        case CLASS_MAGE:
            interrupts.push_back(2139);  // Counterspell
            break;

        case CLASS_WARLOCK:
            interrupts.push_back(19647); // Spell Lock
            interrupts.push_back(6789);  // Death Coil
            break;

        case CLASS_MONK:
            interrupts.push_back(116705); // Spear Hand Strike
            break;

        case CLASS_DRUID:
            interrupts.push_back(78675);  // Solar Beam
            interrupts.push_back(16979);  // Feral Charge - Bear
            break;

        case CLASS_DEMON_HUNTER:
            interrupts.push_back(183752); // Disrupt
            break;

        case CLASS_EVOKER:
            interrupts.push_back(351338); // Quell
            break;
    }

    return interrupts;
}

bool InterruptUtils::CanClassInterrupt(uint8 playerClass)
{
    ::std::vector<uint32> interrupts = GetClassInterruptSpells(playerClass);
    return !interrupts.empty();
}

float InterruptUtils::GetClassInterruptRange(uint8 playerClass)
{
    switch (playerClass)
    {
        case CLASS_WARRIOR:
        case CLASS_PALADIN:
        case CLASS_ROGUE:
        case CLASS_DEATH_KNIGHT:
        case CLASS_MONK:
        case CLASS_DEMON_HUNTER:
            return 5.0f;  // Melee range

        case CLASS_HUNTER:
        case CLASS_MAGE:
        case CLASS_WARLOCK:
        case CLASS_SHAMAN:
        case CLASS_EVOKER:
            return 30.0f; // Ranged

        case CLASS_PRIEST:
        case CLASS_DRUID:
            return 20.0f; // Medium range

        default:
            return 15.0f;
    }
}

// Missing method implementations for linker

float InterruptManager::CalculateInterruptEffectiveness(const InterruptCapability& capability, const InterruptTarget& target)
{
    float effectiveness = 1.0f;

    // Higher effectiveness for more critical targets
    switch (target.priority)
    {
        case InterruptPriority::CRITICAL:
            effectiveness = 1.0f;
            break;
        case InterruptPriority::HIGH:
            effectiveness = 0.8f;
            break;
        case InterruptPriority::MODERATE:
            effectiveness = 0.6f;
            break;
        case InterruptPriority::LOW:
            effectiveness = 0.4f;
            break;
        default:
            effectiveness = 0.1f;
            break;
    }

    // Adjust for cast time remaining
    float timeWindow = target.remainingCastTime;
    if (timeWindow > capability.castTime)
        effectiveness *= 1.0f;
    else
        effectiveness *= 0.5f; // Risky timing

    return effectiveness;
}

InterruptPriority InterruptManager::AssessHealingPriority(const SpellInfo* spellInfo, Unit* caster)
{
    if (!spellInfo || !caster)
        return InterruptPriority::IGNORE;

    // High priority for healing spells
    if (spellInfo->HasEffect(SPELL_EFFECT_HEAL))
        return InterruptPriority::HIGH;

    return InterruptPriority::MODERATE;
}

InterruptPriority InterruptManager::AssessDamagePriority(const SpellInfo* spellInfo, Unit* caster)
{
    if (!spellInfo || !caster)
        return InterruptPriority::IGNORE;

    // Check for high damage spells
    for (SpellEffectInfo const& effect : spellInfo->GetEffects())
    {
        if (effect.Effect == SPELL_EFFECT_SCHOOL_DAMAGE)
        {
            if (effect.CalcValue() > 2000)
                return InterruptPriority::HIGH;
            else if (effect.CalcValue() > 1000)
                return InterruptPriority::MODERATE;
        }
    }

    return InterruptPriority::LOW;
}

InterruptPriority InterruptManager::AssessCrowdControlPriority(const SpellInfo* spellInfo, Unit* caster)
{
    if (!spellInfo || !caster)
        return InterruptPriority::IGNORE;

    // Critical priority for crowd control
    for (SpellEffectInfo const& effect : spellInfo->GetEffects())
    {
        if (effect.ApplyAuraName == SPELL_AURA_MOD_STUN ||
            effect.ApplyAuraName == SPELL_AURA_MOD_FEAR ||
            effect.ApplyAuraName == SPELL_AURA_MOD_CHARM)
        {
            return InterruptPriority::CRITICAL;
        }
    }

    return InterruptPriority::LOW;
}

InterruptPriority InterruptManager::AssessBuffPriority(const SpellInfo* spellInfo, Unit* caster)
{
    if (!spellInfo || !caster)
        return InterruptPriority::IGNORE;
    // Moderate priority for buffs
    if (spellInfo->IsPositive())
        return InterruptPriority::MODERATE;

    return InterruptPriority::LOW;
}

void InterruptManager::HandleMultipleInterruptTargets()
{
    // Simple implementation - handle one target at a time for now
    if (_trackedTargets.empty())
        return;

    // Process the highest priority target
    auto highestPriorityTarget = ::std::min_element(_trackedTargets.begin(), _trackedTargets.end(),
        [](const InterruptTarget& a, const InterruptTarget& b) {
            return a.priority < b.priority;
        });

    if (highestPriorityTarget != _trackedTargets.end())
    {
        AttemptInterrupt(*highestPriorityTarget);
    }
}

void InterruptManager::RegisterInterruptAttempt(const InterruptTarget& target)
{
    _lastInterruptAttempt = GameTime::GetGameTimeMS();

    // Record the attempt in group data if in group
    if (Group* group = _bot->GetGroup())
    {
        _groupInterruptClaims[target.guid] = GameTime::GetGameTimeMS() + 5000; // 5 second claim
    }
}

bool InterruptManager::ShouldLetOthersInterrupt(const InterruptTarget& target)
{
    // Check if another group member has already claimed this target
    auto it = _groupInterruptClaims.find(target.guid);
    if (it != _groupInterruptClaims.end())
    {
        uint32 currentTime = GameTime::GetGameTimeMS();
        if (currentTime < it->second)
        {
            return true; // Someone else is handling it
        }
    }

    return false;
}

void InterruptManager::CoordinateInterruptsWithGroup(const ::std::vector<Player*>& groupMembers)
{
    if (groupMembers.empty())
        return;

    // Simple round-robin assignment for now
    uint32 myIndex = 0;
    for (size_t i = 0; i < groupMembers.size(); ++i)
    {
        if (groupMembers[i] == _bot)
        {
            myIndex = static_cast<uint32>(i);
            break;
        }
    }

    // Store coordination data
    _groupData.rotationIndex = myIndex;
    _groupData.lastRotationUpdate = GameTime::GetGameTimeMS();
}

bool InterruptManager::ShouldInterruptBuff(const SpellInfo* spellInfo, Unit* caster)
{
    if (!spellInfo || !caster)
        return false;

    // Interrupt beneficial spells on enemies
    return spellInfo->IsPositive() && _bot->IsHostileTo(caster);
}

bool InterruptManager::IsInterruptExecutable(const InterruptPlan& plan)
{
    if (!plan.target || !plan.capability)
        return false;

    // Check if we have enough time to execute
    if (plan.target->remainingCastTime < plan.executionTime)
        return false;

    // Check if capability is available
    if (!plan.capability->isAvailable)
        return false;

    return plan.successProbability > 0.3f; // At least 30% chance of success
}

bool InterruptManager::AttemptLoSInterrupt(Unit* target)
{
    // LoS interrupt - break line of sight with caster to interrupt spell
    if (!target || !_bot)
        return false;

    // Check if target is casting
    if (!target->HasUnitState(UNIT_STATE_CASTING))
    {
        TC_LOG_TRACE("module.playerbot.interrupt",
            "AttemptLoSInterrupt: Target {} is not casting",
            target->GetName());
        return false;
    }

    Spell* currentSpell = target->GetCurrentSpell(CURRENT_GENERIC_SPELL);
    if (!currentSpell)
    {
        // Try channeled spell
        currentSpell = target->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
    }

    if (!currentSpell)
    {
        TC_LOG_TRACE("module.playerbot.interrupt",
            "AttemptLoSInterrupt: Could not get current spell for {}",
            target->GetName());
        return false;
    }

    SpellInfo const* spellInfo = currentSpell->GetSpellInfo();
    if (!spellInfo)
        return false;

    // Check if spell ignores LoS - if so, LoS interrupt won't work
    if (spellInfo->HasAttribute(SPELL_ATTR2_IGNORE_LINE_OF_SIGHT))
    {
        TC_LOG_TRACE("module.playerbot.interrupt",
            "AttemptLoSInterrupt: Spell {} ignores LoS, cannot interrupt via positioning",
            spellInfo->SpellName->Str[0]);
        return false;
    }

    // Calculate remaining cast time
    float castTimeRemainingMs = static_cast<float>(currentSpell->GetTimer());
    float castTimeRemainingSec = castTimeRemainingMs / 1000.0f;

    // Find a cover position that breaks LoS
    Position coverPos;
    if (!FindCoverPosition(target, coverPos))
    {
        TC_LOG_TRACE("module.playerbot.interrupt",
            "AttemptLoSInterrupt: Cannot find cover position for bot {}",
            _bot->GetName());
        return false;
    }

    // Calculate time needed to reach cover
    float distToCover = _bot->GetDistance2d(coverPos.GetPositionX(), coverPos.GetPositionY());
    float runSpeed = _bot->GetSpeed(MOVE_RUN);
    float timeToReachSec = distToCover / runSpeed;

    // Add buffer time for movement latency
    constexpr float MOVEMENT_BUFFER_SEC = 0.5f;
    timeToReachSec += MOVEMENT_BUFFER_SEC;

    if (timeToReachSec >= castTimeRemainingSec)
    {
        TC_LOG_TRACE("module.playerbot.interrupt",
            "AttemptLoSInterrupt: Bot {} cannot reach cover in time ({:.1f}s vs {:.1f}s remaining)",
            _bot->GetName(), timeToReachSec, castTimeRemainingSec);
        return false;
    }

    // Move to cover using Movement Arbiter if available
    BotAI* botAI = dynamic_cast<BotAI*>(_bot->GetAI());
    if (botAI && botAI->GetUnifiedMovementCoordinator())
    {
        botAI->RequestPointMovement(
            PlayerBotMovementPriority::INTERRUPT_POSITIONING,
            coverPos,
            "LoS interrupt - breaking line of sight",
            "InterruptManager");

        TC_LOG_DEBUG("module.playerbot.interrupt",
            "AttemptLoSInterrupt: Bot {} moving to cover at ({:.1f}, {:.1f}, {:.1f}) to break LoS with {}",
            _bot->GetName(), coverPos.GetPositionX(), coverPos.GetPositionY(), coverPos.GetPositionZ(),
            target->GetName());
    }
    else
    {
        // Fallback: Use BotMovementController with validated pathfinding
        if (BotAI* ai = GetBotAI(_bot))
        {
            if (!ai->MoveTo(coverPos, true))
            {
                // Final fallback to legacy if validation fails
                _bot->GetMotionMaster()->MovePoint(0, coverPos.GetPositionX(), coverPos.GetPositionY(), coverPos.GetPositionZ());
            }
        }
        else
        {
            // Non-bot player - use standard movement
            _bot->GetMotionMaster()->MovePoint(0, coverPos.GetPositionX(), coverPos.GetPositionY(), coverPos.GetPositionZ());
        }

        TC_LOG_DEBUG("module.playerbot.interrupt",
            "AttemptLoSInterrupt: Bot {} (fallback) moving to cover for LoS interrupt",
            _bot->GetName());
    }

    return true;
}

bool InterruptManager::FindCoverPosition(Unit* target, Position& outPos)
{
    if (!target || !_bot)
        return false;

    Position botPos = _bot->GetPosition();
    Position targetPos = target->GetPosition();

    // Strategy 1: Move behind the bot's current position relative to target
    // This creates distance and potentially breaks LoS with terrain
    float angleFromTarget = target->GetAbsoluteAngle(&botPos);

    // Try multiple distances to find valid cover
    constexpr float COVER_DISTANCES[] = { 15.0f, 20.0f, 25.0f, 10.0f };
    constexpr int NUM_ANGLES_TO_TRY = 8;

    Map* map = _bot->GetMap();
    if (!map)
        return false;

    for (float distance : COVER_DISTANCES)
    {
        // Try different angles around the retreat direction
        for (int i = 0; i < NUM_ANGLES_TO_TRY; ++i)
        {
            // Spread angles: 0, +22.5, -22.5, +45, -45, +67.5, -67.5, +90
            float angleOffset = (i == 0) ? 0.0f :
                               ((i % 2 == 1) ? 1.0f : -1.0f) * (((i + 1) / 2) * M_PI / 8.0f);

            float testAngle = angleFromTarget + angleOffset;

            Position testPos;
            testPos.m_positionX = botPos.GetPositionX() + distance * ::std::cos(testAngle);
            testPos.m_positionY = botPos.GetPositionY() + distance * ::std::sin(testAngle);
            testPos.m_positionZ = botPos.GetPositionZ();

            // Get proper ground height
            float groundZ = map->GetHeight(_bot->GetPhaseShift(),
                                          testPos.GetPositionX(),
                                          testPos.GetPositionY(),
                                          testPos.GetPositionZ() + 5.0f);

            if (groundZ != INVALID_HEIGHT)
            {
                testPos.m_positionZ = groundZ;
            }

            // Check if this position breaks LoS with target
            if (!target->IsWithinLOS(testPos.GetPositionX(), testPos.GetPositionY(),
                                    testPos.GetPositionZ() + 2.0f))  // +2 for character height
            {
                // This position breaks LoS - check if it's reachable
                if (_bot->IsWithinLOS(testPos.GetPositionX(), testPos.GetPositionY(),
                                     testPos.GetPositionZ() + 2.0f))
                {
                    outPos = testPos;
                    TC_LOG_TRACE("module.playerbot.interrupt",
                        "FindCoverPosition: Found cover at ({:.1f}, {:.1f}, {:.1f}) at distance {:.1f}",
                        testPos.GetPositionX(), testPos.GetPositionY(), testPos.GetPositionZ(), distance);
                    return true;
                }
            }
        }
    }

    // Strategy 2: If no natural cover found, try to use world geometry
    // Check for nearby pillars, walls, or other objects
    // This is a simplified approach - ideally would query static obstacles

    // Try perpendicular movement (strafe left/right)
    float perpAngle1 = angleFromTarget + M_PI / 2.0f;
    float perpAngle2 = angleFromTarget - M_PI / 2.0f;

    for (float distance = 10.0f; distance <= 30.0f; distance += 5.0f)
    {
        for (float perpAngle : { perpAngle1, perpAngle2 })
        {
            Position testPos;
            testPos.m_positionX = botPos.GetPositionX() + distance * ::std::cos(perpAngle);
            testPos.m_positionY = botPos.GetPositionY() + distance * ::std::sin(perpAngle);
            testPos.m_positionZ = botPos.GetPositionZ();

            float groundZ = map->GetHeight(_bot->GetPhaseShift(),
                                          testPos.GetPositionX(),
                                          testPos.GetPositionY(),
                                          testPos.GetPositionZ() + 5.0f);

            if (groundZ != INVALID_HEIGHT)
            {
                testPos.m_positionZ = groundZ;
            }

            if (!target->IsWithinLOS(testPos.GetPositionX(), testPos.GetPositionY(),
                                    testPos.GetPositionZ() + 2.0f))
            {
                if (_bot->IsWithinLOS(testPos.GetPositionX(), testPos.GetPositionY(),
                                     testPos.GetPositionZ() + 2.0f))
                {
                    outPos = testPos;
                    TC_LOG_TRACE("module.playerbot.interrupt",
                        "FindCoverPosition: Found perpendicular cover at ({:.1f}, {:.1f}, {:.1f})",
                        testPos.GetPositionX(), testPos.GetPositionY(), testPos.GetPositionZ());
                    return true;
                }
            }
        }
    }

    TC_LOG_TRACE("module.playerbot.interrupt",
        "FindCoverPosition: No cover position found for bot {} against {}",
        _bot->GetName(), target->GetName());

    return false;
}

bool InterruptManager::AttemptMovementInterrupt(Unit* target)
{
    // Simple movement interrupt - get out of range
    if (!target)
        return false;

    Position targetPos = target->GetPosition();
    Position botPos = _bot->GetPosition();

    // Move away from target
    float angle = ::std::atan2(botPos.GetPositionY() - targetPos.GetPositionY(),
                           botPos.GetPositionX() - targetPos.GetPositionX());

    Position movePos;
    movePos.m_positionX = botPos.GetPositionX() + 10.0f * ::std::cos(angle);
    movePos.m_positionY = botPos.GetPositionY() + 10.0f * ::std::sin(angle);
    movePos.m_positionZ = botPos.GetPositionZ();

    // PHASE 6B: Use Movement Arbiter with INTERRUPT_POSITIONING priority (220)
    BotAI* botAI = dynamic_cast<BotAI*>(_bot->GetAI());
    if (botAI && botAI->GetUnifiedMovementCoordinator())
    {
        botAI->RequestPointMovement(
            PlayerBotMovementPriority::INTERRUPT_POSITIONING,
            movePos,
            "Movement interrupt - get out of range",
            "InterruptManager");
    }
    else
    {
        // FALLBACK: Use BotMovementController with validated pathfinding
        if (BotAI* ai = GetBotAI(_bot))
        {
            if (!ai->MoveTo(movePos, true))
            {
                // Final fallback to legacy if validation fails
                _bot->GetMotionMaster()->MovePoint(0, movePos.GetPositionX(), movePos.GetPositionY(), movePos.GetPositionZ());
            }
        }
        else
        {
            // Non-bot player - use standard movement
            _bot->GetMotionMaster()->MovePoint(0, movePos.GetPositionX(), movePos.GetPositionY(), movePos.GetPositionZ());
        }
    }
    return true;
}

// Compatibility interface implementations for CombatBehaviorIntegration
bool InterruptManager::HasUrgentInterrupt() const
{
    // Simple check - would scan for urgent interrupt targets
    return false;
}

Unit* InterruptManager::GetInterruptTarget()
{
    auto targets = const_cast<InterruptManager*>(this)->ScanForInterruptTargets();
    if (targets.empty())
        return nullptr;

    // Return highest priority target
    return targets.front().unit;
}

bool InterruptManager::ShouldInterrupt(Unit* target)
{
    if (!target || !target->HasUnitState(UNIT_STATE_CASTING))
        return false;

    Spell const* spell = target->GetCurrentSpell(CURRENT_GENERIC_SPELL);
    if (!spell)
        spell = target->GetCurrentSpell(CURRENT_CHANNELED_SPELL);

    if (!spell)
        return false;

    return IsSpellInterruptWorthy(spell->GetSpellInfo()->Id, target);
}

bool InterruptManager::ShouldInterruptOwnCast()
{
    // Check if bot should interrupt own cast for more urgent action
    return false;
}

bool InterruptManager::IsCastDangerous(Unit* caster)
{
    if (!caster || !caster->HasUnitState(UNIT_STATE_CASTING))
        return false;

    Spell const* spell = caster->GetCurrentSpell(CURRENT_GENERIC_SPELL);
    if (!spell)
        spell = caster->GetCurrentSpell(CURRENT_CHANNELED_SPELL);

    if (!spell)
        return false;

    auto priority = AssessInterruptPriority(spell->GetSpellInfo(), caster);
    return priority == InterruptPriority::CRITICAL || priority == InterruptPriority::HIGH;
}

bool InterruptManager::IsCastHighPriority(Unit* caster)
{
    if (!caster || !caster->HasUnitState(UNIT_STATE_CASTING))
        return false;

    Spell const* spell = caster->GetCurrentSpell(CURRENT_GENERIC_SPELL);
    if (!spell)
        spell = caster->GetCurrentSpell(CURRENT_CHANNELED_SPELL);

    if (!spell)
        return false;

    auto priority = AssessInterruptPriority(spell->GetSpellInfo(), caster);
    return priority == InterruptPriority::HIGH;
}

void InterruptManager::Reset()
{
    // Reset interrupt manager state
    // Clear any tracked state variables that exist in the private section
    // Note: Actual member variables need to be checked in the header
}

// ============================================================================
// INTERRUPT UTILS - STATIC METHODS
// ============================================================================

// WoW 12.0: SpellAttr16 Infrastructure
// These methods provide infrastructure for checking SpellAttr16 flags.
// All 32 flags are currently UNK (undocumented) in WoW 12.0, but this
// infrastructure is ready for when flags become documented.

bool InterruptUtils::HasAnySpellAttr16(uint32 spellId)
{
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    // Check if any SpellAttr16 bits are set by checking each bit
    for (uint32 bit = 0; bit < 32; ++bit)
    {
        SpellAttr16 attr = static_cast<SpellAttr16>(1u << bit);
        if (spellInfo->HasAttribute(attr))
            return true;
    }

    return false;
}

bool InterruptUtils::HasSpellAttr16(uint32 spellId, SpellAttr16 attribute)
{
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    return spellInfo->HasAttribute(attribute);
}

// WoW 12.0: SpellPvpModifier Support
// Access PvpMultiplier from SpellEffectEntry for PvP interrupt priority assessment

float InterruptUtils::GetPvPInterruptMultiplier(uint32 spellId, uint8 effectIndex)
{
    // Access SpellEffectEntry from DB2 store
    for (SpellEffectEntry const* effectEntry : sSpellEffectStore)
    {
        if (effectEntry && effectEntry->SpellID == static_cast<int32>(spellId) &&
            effectEntry->EffectIndex == effectIndex)
        {
            // Return PvpMultiplier if valid, otherwise 1.0 (no modification)
            return effectEntry->PvpMultiplier > 0.0f ? effectEntry->PvpMultiplier : 1.0f;
        }
    }

    return 1.0f;
}

bool InterruptUtils::IsPvPHighPriorityInterrupt(uint32 spellId)
{
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    // Check each effect's PvP multiplier
    // Spells with multiplier > 0.9 (less than 10% reduction) are particularly
    // dangerous in PvP and should be high priority for interruption
    for (uint8 i = 0; i < spellInfo->GetEffects().size(); ++i)
    {
        float mult = GetPvPInterruptMultiplier(spellId, i);
        if (mult > 0.9f && mult > 0.0f)
        {
            // This effect has minimal PvP reduction - high priority interrupt
            SpellEffectInfo const& effect = spellInfo->GetEffect(static_cast<SpellEffIndex>(i));

            // Check if this is a damage or healing effect
            SpellEffectName effectType = effect.Effect;
            switch (effectType)
            {
                case SPELL_EFFECT_SCHOOL_DAMAGE:
                case SPELL_EFFECT_HEAL:
                case SPELL_EFFECT_HEAL_PCT:
                case SPELL_EFFECT_WEAPON_DAMAGE:
                case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
                case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
                case SPELL_EFFECT_DAMAGE_FROM_MAX_HEALTH_PCT:
                case SPELL_EFFECT_HEAL_MAX_HEALTH:
                    // High damage/healing with minimal PvP reduction = high priority
                    TC_LOG_DEBUG("playerbot.interrupt",
                        "IsPvPHighPriorityInterrupt: Spell {} effect {} has PvpMultiplier {:.2f} - HIGH PRIORITY",
                        spellId, i, mult);
                    return true;
                default:
                    break;
            }
        }
    }

    return false;
}

} // namespace Playerbot