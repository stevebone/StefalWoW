/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Common Rotation Helper Functions
 *
 * Provides shared utility methods used across all spec rotations
 * Eliminates hundreds of duplicate helper method implementations
 */

#pragma once

#include "Player.h"
#include "Unit.h"
#include "Group.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "SpellAuraEffects.h"
#include "GridNotifiers.h"
#include "CellImpl.h"
#include <vector>
#include <algorithm>

namespace Playerbot
{

// ============================================================================
// HEALTH CHECKING UTILITIES
// ============================================================================

class HealthUtils
{
public:
    /**
     * Get all group members below health threshold
     */
    static ::std::vector<Unit*> GetInjuredGroupMembers(Player* bot, float healthPct)
    {
        ::std::vector<Unit*> injured;

        if (!bot)
            return injured;

        Group* group = bot->GetGroup();
        if (!group)
        {
            // Solo - check self
            if (bot->GetHealthPct() < healthPct)
                injured.push_back(bot);
            return injured;
        }

        for (GroupReference const& ref : group->GetMembers())
        {
            if (Player* member = ref.GetSource())
            {
                if (member->IsAlive() && bot->IsInMap(member) && member->GetHealthPct() < healthPct)
                    injured.push_back(member);
            }
        }

        return injured;
    }

    /**
     * Get most injured group member
     */
    static Unit* GetMostInjured(Player* bot, float minHealthPct = 0.0f)
    {
        Unit* mostInjured = nullptr;
        float lowestPct = 100.0f;

        if (!bot)
            return nullptr;

        Group* group = bot->GetGroup();
        if (!group)
        {
            return bot->GetHealthPct() < lowestPct ? bot : nullptr;
        }

        for (GroupReference const& ref : group->GetMembers())
        {
            if (Player* member = ref.GetSource())
            {
                if (member->IsAlive() && bot->IsInMap(member))
                {
                    float healthPct = member->GetHealthPct();
                    if (healthPct < lowestPct && healthPct >= minHealthPct)
                    {
                        lowestPct = healthPct;
                        mostInjured = member;
                    }
                }
            }
        }

        return mostInjured;
    }

    /**
     * Count group members below health threshold
     */
    static uint32 CountInjured(Player* bot, float healthPct)
    {
        return static_cast<uint32>(GetInjuredGroupMembers(bot, healthPct).size());
    }

    /**
     * Get tank (lowest health member currently tanking)
     */
    static Unit* GetTank(Player* bot)
    {
        if (!bot)
            return nullptr;

        Group* group = bot->GetGroup();
        if (!group)
            return bot;

        Unit* tank = nullptr;
        float lowestPct = 100.0f;

        for (GroupReference const& ref : group->GetMembers())
        {
            if (Player* member = ref.GetSource())
            {
                if (member->IsAlive() && bot->IsInMap(member))
                {
                    // Check if currently tanking (has aggro)
                    if (Unit* victim = member->GetVictim())
                    {
                        if (victim->GetVictim() == member)
                        {
                            float healthPct = member->GetHealthPct();
                            if (healthPct < lowestPct)
                            {
                                lowestPct = healthPct;
                                tank = member;
                            }
                        }
                    }
                }
            }
        }

        return tank;
    }
};

// ============================================================================
// TARGET SELECTION UTILITIES
// ============================================================================

class TargetUtils
{
public:
    /**
     * Find best AoE target (most enemies nearby)
     */
    static Unit* GetBestAoETarget(Player* bot, Unit* currentTarget, float range)
    {
        if (!bot || !currentTarget)
            return currentTarget;

        Unit* bestTarget = currentTarget;
        uint32 maxNearby = CountEnemiesNear(currentTarget, range);

        // Check if another target has more nearby enemies
        ::std::list<Unit*> enemies;
        Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(bot, bot, 40.0f);
        Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(bot, enemies, u_check);
        Cell::VisitAllObjects(bot, searcher, 40.0f);

        for (Unit* enemy : enemies)
        {
            if (!enemy || !enemy->IsAlive())
                continue;

            uint32 nearbyCount = CountEnemiesNear(enemy, range);
            if (nearbyCount > maxNearby)
            {
                maxNearby = nearbyCount;
                bestTarget = enemy;
            }
        }

        return bestTarget;
    }

    /**
     * Count enemies within range of a unit
     */
    static uint32 CountEnemiesNear(Unit* center, float range)
    {
        if (!center)
            return 0;

        uint32 count = 0;
        ::std::list<Unit*> enemies;
        Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(center, center, range);
        Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(center, enemies, u_check);
        Cell::VisitAllObjects(center, searcher, range);

        for (Unit* enemy : enemies)
        {
            if (enemy && enemy->IsAlive())
                ++count;
        }

        return count;
    }

    /**
     * Find target missing specific debuff
     */
    static Unit* GetTargetMissingDebuff(Player* bot, uint32 spellId, float maxRange = 40.0f)
    {
        if (!bot)
            return nullptr;

        ::std::list<Unit*> enemies;
        Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(bot, bot, maxRange);
        Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(bot, enemies, u_check);
        Cell::VisitAllObjects(bot, searcher, maxRange);

        for (Unit* enemy : enemies)
        {
            if (enemy && enemy->IsAlive() && !enemy->HasAura(spellId))
                return enemy;
        }

        return nullptr;
    }

    /**
     * Check if target is priority (boss, elite, high threat)
     */
    static bool IsPriorityTarget(Unit* target)
    {
        if (!target)
            return false;

        // Boss or elite
        if (Creature* creature = target->ToCreature())
        {
            if (creature->isWorldBoss() || creature->GetCreatureType() == CREATURE_TYPE_HUMANOID)
                return true;
        }

        // High health target
        if (target->GetMaxHealth() > 1000000)
            return true;

        return false;
    }
};

// ============================================================================
// DISTANCE AND POSITIONING UTILITIES
// ============================================================================

class RotationPositionUtils
{
public:
    /**
     * Check if bot is in melee range of target
     */
    static bool IsInMeleeRange(Player* bot, Unit* target, float extraRange = 0.0f)
    {
        if (!bot || !target)
            return false;

        float meleeRange = bot->GetMeleeRange(target) + extraRange;
        return bot->GetDistance(target) <= meleeRange;
    }

    /**
     * Check if bot is behind target (for backstab, etc.)
     */
    static bool IsBehindTarget(Player* bot, Unit* target)
    {
        if (!bot || !target)
            return false;

        return target->IsWithinMeleeRange(bot) && target->HasInArc(static_cast<float>(M_PI), bot);
    }

    /**
     * Check if target is in range
     */
    static bool IsInRange(Player* bot, Unit* target, float minRange, float maxRange)
    {
        if (!bot || !target)
            return false;

        float distance = bot->GetDistance(target);
        return distance >= minRange && distance <= maxRange;
    }

    /**
     * Get distance to target
     */
    static float GetDistance(Player* bot, Unit* target)
    {
        if (!bot || !target)
            return 1000.0f; // Very far

        return bot->GetDistance(target);
    }

    /**
     * Check if group members are stacked (for AoE heal placement)
     */
    static bool AreGroupMembersStacked(Player* bot, Unit* centerPoint, float stackRange)
    {
        if (!bot || !centerPoint)
            return false;

        Group* group = bot->GetGroup();
        if (!group)
            return false;

        uint32 nearbyCount = 0;

        for (GroupReference const& ref : group->GetMembers())
        {
            if (Player* member = ref.GetSource())
            {
                if (member->IsAlive() && bot->IsInMap(member))
                {
                    if (member->GetDistance(centerPoint) <= stackRange)
                        ++nearbyCount;
                }
            }
        }

        return nearbyCount >= 3; // At least 3 members stacked
    }
};

// ============================================================================
// RESOURCE UTILITIES
// ============================================================================

class ResourceUtils
{
public:
    /**
     * Get resource percent (0-100)
     */
    static float GetResourcePercent(Player* bot, Powers powerType)
    {
        if (!bot)
            return 0.0f;

        uint32 current = bot->GetPower(powerType);
        uint32 max = bot->GetMaxPower(powerType);

        if (max == 0)
            return 0.0f;

        return (static_cast<float>(current) / static_cast<float>(max)) * 100.0f;
    }

    /**
     * Check if we have enough resource for spell
     */
    static bool HasEnoughResource(Player* bot, Powers powerType, uint32 amount)
    {
        if (!bot)
            return false;

        return bot->GetPower(powerType) >= amount;
    }

    /**
     * Check if resource is below threshold (need to conserve)
     */
    static bool IsLowResource(Player* bot, Powers powerType, float threshold = 20.0f)
    {
        return GetResourcePercent(bot, powerType) < threshold;
    }
};

// ============================================================================
// BUFF/DEBUFF UTILITIES
// ============================================================================

class AuraUtils
{
public:
    /**
     * Check if any of the given auras are active
     */
    static bool HasAnyAura(Unit* unit, const ::std::vector<uint32>& spellIds)
    {
        if (!unit)
            return false;

        for (uint32 spellId : spellIds)
        {
            if (unit->HasAura(spellId))
                return true;
        }

        return false;
    }

    /**
     * Get aura stack count
     */
    static uint32 GetAuraStacks(Unit* unit, uint32 spellId)
    {
        if (!unit)
            return 0;

        if (Aura* aura = unit->GetAura(spellId))
            return aura->GetStackAmount();

        return 0;
    }

    /**
     * Get aura remaining time
     */
    static uint32 GetAuraRemainingTime(Unit* unit, uint32 spellId)
    {
        if (!unit)
            return 0;

        if (Aura* aura = unit->GetAura(spellId))
            return aura->GetDuration();

        return 0;
    }

    /**
     * Count group members with buff
     */
    static uint32 CountGroupMembersWithBuff(Player* bot, uint32 spellId)
    {
        if (!bot)
            return 0;

        Group* group = bot->GetGroup();
        if (!group)
            return bot->HasAura(spellId) ? 1 : 0;

        uint32 count = 0;

        for (GroupReference const& ref : group->GetMembers())
        {
            if (Player* member = ref.GetSource())
            {
                if (member->IsAlive() && bot->IsInMap(member) && member->HasAura(spellId))
                    ++count;
            }
        }

        return count;
    }
};

// ============================================================================
// COMBAT STATE UTILITIES
// ============================================================================

class CombatUtils
{
public:
    /**
     * Check if in execute range (target below 20% or 35% health)
     */
    static bool IsExecutePhase(Unit* target, float threshold = 20.0f)
    {
        if (!target)
            return false;

        return target->GetHealthPct() <= threshold;
    }

    /**
     * Check if in burn phase (boss below 35% typically)
     */
    static bool IsBurnPhase(Unit* target)
    {
        return IsExecutePhase(target, 35.0f);
    }

    /**
     * Check if we should use AoE (3+ targets typically)
     */
    static bool ShouldUseAoE(uint32 enemyCount, uint32 threshold = 3)
    {
        return enemyCount >= threshold;
    }

    /**
     * Check if target is casting and can be interrupted
     */
    static bool IsInterruptible(Unit* target)
    {
        if (!target)
            return false;

        if (Spell* currentSpell = target->GetCurrentSpell(CURRENT_GENERIC_SPELL))
        {
            if (SpellInfo const* spellInfo = currentSpell->GetSpellInfo())
                return spellInfo->PreventionType == SPELL_PREVENTION_TYPE_SILENCE;
        }

        return false;
    }

    /**
     * Get current threat level (0 = no threat, 100 = high threat)
     */
    static float GetThreatLevel(Player* bot)
    {
        if (!bot)
            return 0.0f;

        Unit* victim = bot->GetVictim();
        if (!victim)
            return 0.0f;

        // Simplified threat calculation
        if (victim->GetVictim() == bot)
            return 100.0f; // We have aggro

        return 50.0f; // Moderate threat
    }
};

} // namespace Playerbot
