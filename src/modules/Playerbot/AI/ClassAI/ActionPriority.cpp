/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ActionPriority.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "Log.h"
#include "Creature.h"
#include <algorithm>
#include "../../Spatial/SpatialGridQueryHelpers.h"  // PHASE 5F: Thread-safe queries

namespace Playerbot
{

ActionPriorityQueue::ActionPriorityQueue() : _size(0)
{
}

void ActionPriorityQueue::AddAction(uint32 spellId, ActionPriority priority, float score, ::Unit* target)
{
    if (!spellId)
        return;

    PrioritizedAction action(spellId, priority, score, target);

    if (IsActionValid(action))
    {
        _queue.push(action);
        _size.fetch_add(1);

        TC_LOG_DEBUG("playerbot.actionqueue", "Added action: spell={}, priority={}, score={:.2f}",
                     spellId, static_cast<uint8>(priority), score);
    }
}

bool ActionPriorityQueue::GetNextAction(PrioritizedAction& action)
{
    if (!_queue.empty())
    {
        action = _queue.top();
        _queue.pop();
        _size.fetch_sub(1);

        // Validate the action is still relevant
    if (action.IsValid())
        {
            TC_LOG_DEBUG("playerbot.actionqueue", "Retrieved action: spell={}, priority={}, score={:.2f}",
                         action.spellId, static_cast<uint8>(action.priority), action.score);
            return true;
        }
        else
        {
            TC_LOG_DEBUG("playerbot.actionqueue", "Discarded stale action: spell={}", action.spellId);
            // Try next action
            return GetNextAction(action);
        }
    }

    return false;
}

bool ActionPriorityQueue::PeekNextAction(PrioritizedAction& action) const
{
    // TBB doesn't have try_peek, so we'll implement a workaround
    PrioritizedAction tempAction;
    if (!_queue.empty())
    {
        tempAction = _queue.top();
        action = tempAction;
        // Put it back
        const_cast<ActionPriorityQueue*>(this)->_queue.push(tempAction);
        return tempAction.IsValid();
    }
    return false;
}

bool ActionPriorityQueue::HasActions() const
{
    return _size.load() > 0;
}

size_t ActionPriorityQueue::Size() const
{
    return _size.load();
}

void ActionPriorityQueue::Clear()
{
    while (!_queue.empty())
    {
        _queue.pop();
        _size.fetch_sub(1);
    }

    TC_LOG_DEBUG("playerbot.actionqueue", "Cleared action queue");
}

void ActionPriorityQueue::CleanupOldActions(uint32 maxAgeMs)
{
    ::std::vector<PrioritizedAction> validActions;
    PrioritizedAction action;

    // Extract all actions and filter valid ones
    while (!_queue.empty())
    {
        action = _queue.top();
        _queue.pop();
        _size.fetch_sub(1);
        if (action.IsValid(maxAgeMs))
        {
            validActions.push_back(action);
        }
    }

    // Re-add valid actions
    for (const auto& validAction : validActions)
    {
        _queue.push(validAction);
        _size.fetch_add(1);
    }

    TC_LOG_DEBUG("playerbot.actionqueue", "Cleaned up action queue: {} valid actions remaining", validActions.size());
}

void ActionPriorityQueue::AddActions(const ::std::vector<PrioritizedAction>& actions)
{
    for (const auto& action : actions)
    {
        if (IsActionValid(action))
        {
            _queue.push(action);
            _size.fetch_add(1);
        }
    }

    TC_LOG_DEBUG("playerbot.actionqueue", "Added {} actions to queue", actions.size());
}

::std::vector<PrioritizedAction> ActionPriorityQueue::GetActionsByPriority(ActionPriority priority) const
{
    ::std::vector<PrioritizedAction> result;
    ::std::vector<PrioritizedAction> allActions;
    PrioritizedAction action;

    // Extract all actions
    while (!const_cast<ActionPriorityQueue*>(this)->_queue.empty())
    {
        action = const_cast<ActionPriorityQueue*>(this)->_queue.top();
        const_cast<ActionPriorityQueue*>(this)->_queue.pop();
        const_cast<ActionPriorityQueue*>(this)->_size.fetch_sub(1);
        allActions.push_back(action);

        if (action.priority == priority && action.IsValid())
        {
            result.push_back(action);
        }
    }

    // Re-add all actions
    for (const auto& act : allActions)
    {
        const_cast<ActionPriorityQueue*>(this)->_queue.push(act);
        const_cast<ActionPriorityQueue*>(this)->_size.fetch_add(1);
    }

    return result;
}

bool ActionPriorityQueue::ContainsSpell(uint32 spellId) const
{
    ::std::vector<PrioritizedAction> allActions;
    PrioritizedAction action;
    bool found = false;

    // Extract all actions and check for spell
    while (!const_cast<ActionPriorityQueue*>(this)->_queue.empty())
    {
        action = const_cast<ActionPriorityQueue*>(this)->_queue.top();
        const_cast<ActionPriorityQueue*>(this)->_queue.pop();
        const_cast<ActionPriorityQueue*>(this)->_size.fetch_sub(1);
        allActions.push_back(action);

        if (action.spellId == spellId && action.IsValid())
        {
            found = true;
        }
    }

    // Re-add all actions
    for (const auto& act : allActions)
    {
        const_cast<ActionPriorityQueue*>(this)->_queue.push(act);
        const_cast<ActionPriorityQueue*>(this)->_size.fetch_add(1);
    }

    return found;
}

bool ActionPriorityQueue::IsActionValid(const PrioritizedAction& action) const
{
    // Check spell exists
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(action.spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    // Check target validity if specified
    if (action.target && !action.target->IsInWorld())
        return false;

    return true;
}

// ActionPriorityHelper implementation

PrioritizedAction ActionPriorityHelper::CreateEmergencyHeal(uint32 spellId, ::Unit* target, float healthPct)
{
    float score = 100.0f - healthPct; // Lower health = higher score
    score *= GetHealthPriorityMultiplier(healthPct);

    return PrioritizedAction(spellId, ActionPriority::EMERGENCY, score, target);
}

PrioritizedAction ActionPriorityHelper::CreateEmergencyDefensive(uint32 spellId, float threatLevel)
{
    float score = threatLevel * 100.0f;

    return PrioritizedAction(spellId, ActionPriority::EMERGENCY, score);
}

PrioritizedAction ActionPriorityHelper::CreateEmergencyEscape(uint32 spellId, ::Unit* target)
{
    float score = 100.0f; // High priority for escape abilities

    return PrioritizedAction(spellId, ActionPriority::EMERGENCY, score, target);
}

PrioritizedAction ActionPriorityHelper::CreateInterrupt(uint32 spellId, ::Unit* target, uint32 enemySpellId)
{
    float score = CalculateInterruptScore(target, enemySpellId);

    return PrioritizedAction(spellId, ActionPriority::INTERRUPT, score, target);
}

PrioritizedAction ActionPriorityHelper::CreateBurst(uint32 spellId, ::Unit* target, float damageModifier)
{
    float score = CalculateDamageScore(target, spellId) * damageModifier;

    return PrioritizedAction(spellId, ActionPriority::BURST, score, target);
}

PrioritizedAction ActionPriorityHelper::CreateRotation(uint32 spellId, ::Unit* target, float rotationPriority)
{
    float score = rotationPriority;

    return PrioritizedAction(spellId, ActionPriority::ROTATION, score, target);
}

PrioritizedAction ActionPriorityHelper::CreateBuff(uint32 spellId, ::Unit* target, uint32 remainingDuration)
{
    float score = CalculateBuffScore(target, spellId);

    // Higher priority if buff is about to expire
    if (remainingDuration < 30000) // Less than 30 seconds
    {
        score += (30000 - remainingDuration) / 1000.0f;
    }

    return PrioritizedAction(spellId, ActionPriority::BUFF, score, target);
}

PrioritizedAction ActionPriorityHelper::CreateMovement(uint32 spellId, ::Unit* target, float distance)
{
    float score = GetDistancePriorityMultiplier(distance);

    return PrioritizedAction(spellId, ActionPriority::MOVEMENT, score, target);
}

float ActionPriorityHelper::CalculateHealScore(::Unit* target)
{
    if (!target)
        return 0.0f;

    float healthPct = target->GetHealthPct();
    float score = 100.0f - healthPct;

    // Multiply by health priority curve
    score *= GetHealthPriorityMultiplier(healthPct);

    return score;
}

float ActionPriorityHelper::CalculateDamageScore(::Unit* target, uint32 spellId)
{
    if (!target)
        return 0.0f;

    float score = 50.0f; // Base damage score

    // Consider target's health - prioritize low health targets
    float healthPct = target->GetHealthPct();
    if (healthPct < 35.0f)
    {
        score += (35.0f - healthPct) * 2.0f; // Execute range bonus
    }

    // Consider spell power/effectiveness
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (spellInfo)
    {
        // Higher score for higher damage spells (approximation)
    if (spellInfo->GetMaxRange() > 5.0f) // Ranged spell
            score += 10.0f;

        if (spellInfo->RecoveryTime > 10000) // Long cooldown = powerful
            score += 20.0f;
    }

    return score;
}

float ActionPriorityHelper::CalculateBuffScore(::Unit* target, uint32 spellId)
{
    if (!target)
        return 0.0f;

    float score = 30.0f; // Base buff score

    // Check if target already has this buff
    if (target->HasAura(spellId))
    {
        score = 5.0f; // Lower priority for refreshing existing buffs
    }

    return score;
}

float ActionPriorityHelper::CalculateInterruptScore(::Unit* target, uint32 enemySpellId)
{
    if (!target)
        return 0.0f;

    float score = 80.0f; // Base interrupt score

    // Analyze enemy spell to determine interrupt priority
    if (enemySpellId != 0)
    {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(enemySpellId, DIFFICULTY_NONE);
        if (spellInfo)
        {
            // HIGH PRIORITY: Healing spells (prevent enemy healing)
    if (spellInfo->HasEffect(SPELL_EFFECT_HEAL) ||
                spellInfo->HasEffect(SPELL_EFFECT_HEAL_PCT))
            {
                score += 40.0f; // Total: 120
            }

            // HIGH PRIORITY: Crowd control spells (prevent CC on our team)
    if (spellInfo->HasAura(SPELL_AURA_MOD_STUN) ||
                spellInfo->HasAura(SPELL_AURA_MOD_FEAR) ||
                spellInfo->HasAura(SPELL_AURA_MOD_CHARM) ||
                spellInfo->HasAura(SPELL_AURA_MOD_CONFUSE) ||
                spellInfo->HasAura(SPELL_AURA_MOD_PACIFY))
            {
                score += 35.0f; // Total: 115
            }

            // MEDIUM-HIGH PRIORITY: Long cast time spells (usually powerful)
            uint32 castTime = spellInfo->CalcCastTime();
            if (castTime >= 3000) // 3+ seconds
                score += 25.0f; // Total: 105
            else if (castTime >= 2000) // 2+ seconds
                score += 15.0f; // Total: 95

            // MEDIUM PRIORITY: AoE damage spells
    if (spellInfo->IsTargetingArea())
                score += 20.0f; // Total: 100+

            // MEDIUM PRIORITY: DoT spells (less urgent than direct damage)
    if (spellInfo->HasAura(SPELL_AURA_PERIODIC_DAMAGE) ||
                spellInfo->HasAura(SPELL_AURA_PERIODIC_DAMAGE_PERCENT))
                score += 10.0f; // Total: 90

            // LOWER PRIORITY: Buffs (less critical to interrupt)
    if (spellInfo->IsPositive())
                score -= 20.0f; // Total: 60

            // LOWER PRIORITY: Instant cast (harder to interrupt, less impact)
    if (castTime == 0)
                score -= 30.0f; // Total: 50

            // BONUS: Elite/Boss mobs have more dangerous spells
    if (Creature const* creature = target->ToCreature())
            {
                if (creature->isWorldBoss() || creature->IsDungeonBoss())
                    score += 30.0f; // Boss spells are high priority
                else if (creature->IsElite())
                    score += 15.0f; // Elite spells are medium-high priority
            }
        }
    }

    // Ensure score stays in reasonable range
    return ::std::max(0.0f, ::std::min(score, 200.0f));
}

float ActionPriorityHelper::GetHealthPriorityMultiplier(float healthPct)
{
    if (healthPct < 10.0f)
        return 10.0f; // Critical
    else if (healthPct < 30.0f)
        return 3.0f;  // Very low
    else if (healthPct < 50.0f)
        return 2.0f;  // Low
    else if (healthPct < 80.0f)
        return 1.5f;  // Medium
    else
        return 1.0f;  // High
}

float ActionPriorityHelper::GetThreatPriorityMultiplier(::Unit* unit)
{
    if (!unit)
        return 1.0f;

    // Calculate priority multiplier based on unit's role and current threat situation
    float multiplier = 1.0f;

    Player* player = unit->ToPlayer();
    if (player)
    {
        // Get player's role based on spec
        ChrSpecialization spec = player->GetPrimarySpecialization();
        bool isTank = false;
        bool isHealer = false;

        // Determine role from specialization
    switch (player->GetClass())
        {
            case CLASS_WARRIOR:
                isTank = (spec == ChrSpecialization::WarriorProtection);
                break;
            case CLASS_PALADIN:
                isTank = (spec == ChrSpecialization::PaladinProtection);
                isHealer = (spec == ChrSpecialization::PaladinHoly);
                break;
            case CLASS_DEATH_KNIGHT:
                isTank = (spec == ChrSpecialization::DeathKnightBlood);
                break;
            case CLASS_DRUID:
                isTank = (spec == ChrSpecialization::DruidGuardian);
                isHealer = (spec == ChrSpecialization::DruidRestoration);
                break;
            case CLASS_MONK:
                isTank = (spec == ChrSpecialization::MonkBrewmaster);
                isHealer = (spec == ChrSpecialization::MonkMistweaver);
                break;
            case CLASS_DEMON_HUNTER:
                isTank = (spec == ChrSpecialization::DemonHunterVengeance);
                break;
            case CLASS_PRIEST:
            case CLASS_SHAMAN:
                isHealer = (spec == ChrSpecialization::PriestDiscipline ||
                           spec == ChrSpecialization::PriestHoly ||
                           spec == ChrSpecialization::ShamanRestoration);
                break;
            default:
                break;
        }

        // TANK PRIORITY: Tanks should have higher priority for threat generation
        // Multiply by threat level to determine urgency
    if (isTank)
        {
            // Check if tank is currently tanking (has aggro)
            ThreatManager& threatMgr = unit->GetThreatManager();
            if (threatMgr.GetCurrentVictim())
            {
                // Tank is actively tanking - high threat priority
                multiplier = 2.5f;
            }
            else
            {
                // Tank needs to establish threat
                multiplier = 2.0f;
            }
        }
        // HEALER PRIORITY: Healers should avoid threat
        else if (isHealer)
        {
            // Healers should have reduced threat priority (avoid pulling aggro)
            multiplier = 0.5f;
        }
        // DPS PRIORITY: Standard threat management
        else
        {
            // DPS should be aware of threat but not prioritize it heavily
            multiplier = 1.0f;

            // Check if DPS is about to pull aggro (threat too high)
            ThreatManager& threatMgr = unit->GetThreatManager();
            // If threat is approaching tank's threat, reduce priority
            // (to avoid accidental pulls)
            Unit* victim = threatMgr.GetCurrentVictim();
            if (victim && victim != unit)
            {
                float myThreat = threatMgr.GetThreat(unit);
                float tankThreat = threatMgr.GetThreat(victim);

                if (tankThreat > 0.0f)
                {
                    float threatPercent = (myThreat / tankThreat) * 100.0f;
                    if (threatPercent > 90.0f)
                    {
                        // About to pull aggro - reduce priority
                        multiplier = 0.7f;
                    }
                    else if (threatPercent > 110.0f)
                    {
                        // Already pulled aggro - very low priority
                        multiplier = 0.3f;
                    }
                }
            }
        }
    }
    else if (Creature const* creature = unit->ToCreature())
    {
        // Pets and guardians should follow similar rules to their owners
    if (creature->IsPet() || creature->IsGuardian())
        {
            Unit* owner = creature->GetOwner();
            if (owner)
            {
                // Recursively calculate owner's threat priority
                return GetThreatPriorityMultiplier(owner);
            }
        }
    }

    // Clamp multiplier to reasonable range
    return ::std::max(0.1f, ::std::min(multiplier, 5.0f));
}

float ActionPriorityHelper::GetDistancePriorityMultiplier(float distance)
{
    if (distance > 40.0f)
        return 5.0f;  // Very far
    else if (distance > 20.0f)
        return 3.0f;  // Far
    else if (distance > 10.0f)
        return 2.0f;  // Medium
    else
        return 1.0f;  // Close
}

// ActionPool implementation

ActionPool& ActionPool::Instance()
{
    static ActionPool instance;
    return instance;
}

::std::unique_ptr<PrioritizedAction> ActionPool::Acquire()
{
    ::std::lock_guard lock(_poolMutex);

    if (!_pool.empty())
    {
        auto action = ::std::move(_pool.back());
        _pool.pop_back();
        return action;
    }

    return ::std::make_unique<PrioritizedAction>();
}

void ActionPool::Release(::std::unique_ptr<PrioritizedAction> action)
{
    if (!action)
        return;

    ::std::lock_guard lock(_poolMutex);

    if (_pool.size() < MAX_POOL_SIZE)
    {
        // Reset the action
        *action = PrioritizedAction();
        _pool.push_back(::std::move(action));
    }
    // If pool is full, just let the unique_ptr destroy the object
}

void ActionPool::Cleanup()
{
    ::std::lock_guard lock(_poolMutex);

    // Keep only half the pool size to free up memory
    if (_pool.size() > MAX_POOL_SIZE / 2)
    {
        _pool.resize(MAX_POOL_SIZE / 2);
    }
}

} // namespace Playerbot