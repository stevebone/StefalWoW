/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRINITYCORE_COMBAT_EVALUATORS_H
#define TRINITYCORE_COMBAT_EVALUATORS_H

#include "AI/Utility/UtilitySystem.h"

namespace Playerbot
{

/**
 * @brief Evaluates need to engage in combat
 * High score when enemies nearby and bot is healthy
 */
class TC_GAME_API CombatEngageEvaluator : public UtilityEvaluator
{
public:
    CombatEngageEvaluator() : UtilityEvaluator("CombatEngage", 1.0f) {}

    float Evaluate(UtilityContext const& context) const override
    {
        // No enemies = no combat
        if (context.enemiesInRange == 0)
            return 0.0f;

        // Already in combat = maintain
        if (context.inCombat)
            return 0.8f;

        // Health-based scoring (use logistic curve for smooth transition)
        float healthFactor = Logistic(context.healthPercent, 15.0f);

        // Enemy count factor (more enemies = higher priority, but cap at 5)
        float enemyFactor = Clamp(context.enemiesInRange / 5.0f, 0.0f, 1.0f);

        return healthFactor * enemyFactor;
    }
};

/**
 * @brief Evaluates need to heal allies
 * High score when allies are wounded
 */
class TC_GAME_API HealAllyEvaluator : public UtilityEvaluator
{
public:
    HealAllyEvaluator() : UtilityEvaluator("HealAlly", 1.2f) {}

    float Evaluate(UtilityContext const& context) const override
    {
        // Not a healer = can't heal
        if (context.role != UtilityContext::Role::HEALER)
            return 0.0f;

        // No mana = can't heal
        if (context.manaPercent < 0.1f)
            return 0.0f;

        // Inverse of lowest ally health (lower health = higher priority)
        float urgency = InverseLinear(context.lowestAllyHealthPercent);

        // Mana availability factor (quadratic to preserve mana when full)
        float manaFactor = Quadratic(context.manaPercent);

        return urgency * manaFactor;
    }
};

/**
 * @brief Evaluates need to maintain threat (tanks)
 * High score when tank has no aggro or enemies attacking healers
 */
class TC_GAME_API TankThreatEvaluator : public UtilityEvaluator
{
public:
    TankThreatEvaluator() : UtilityEvaluator("TankThreat", 1.5f) {}

    float Evaluate(UtilityContext const& context) const override
    {
        // Not a tank = not responsible for threat
        if (context.role != UtilityContext::Role::TANK)
            return 0.0f;

        // No enemies = no threat needed
        if (context.enemiesInRange == 0)
            return 0.0f;

        // Tank doesn't have aggro = CRITICAL
        if (!context.hasAggro)
            return 1.0f;

        // Tank has aggro = maintain moderate priority
        return 0.6f;
    }
};

/**
 * @brief Evaluates need to use defensive cooldowns
 * High score when health is low and in combat
 */
class TC_GAME_API DefensiveCooldownEvaluator : public UtilityEvaluator
{
public:
    DefensiveCooldownEvaluator() : UtilityEvaluator("DefensiveCooldown", 2.0f) {}

    float Evaluate(UtilityContext const& context) const override
    {
        // Not in combat = no need
        if (!context.inCombat)
            return 0.0f;

        // Inverse health with steep curve (panic at low health)
        // Cubic curve creates very high urgency below 30% health
        float healthUrgency = Cubic(InverseLinear(context.healthPercent));

        // Time in combat factor (longer combat = more likely to use)
        // Ramps up over 30 seconds
        float combatTimeFactor = Clamp(context.timeSinceCombatStart / 30000.0f, 0.0f, 1.0f);

        return healthUrgency * (0.7f + 0.3f * combatTimeFactor);
    }
};

/**
 * @brief Evaluates need to flee from combat
 * High score when health is critically low and outnumbered
 */
class TC_GAME_API FleeEvaluator : public UtilityEvaluator
{
public:
    FleeEvaluator() : UtilityEvaluator("Flee", 3.0f) {}

    float Evaluate(UtilityContext const& context) const override
    {
        // Not in combat = no need to flee
        if (!context.inCombat)
            return 0.0f;

        // Critically low health (<20%) = high flee priority
        if (context.healthPercent < 0.2f)
        {
            // More enemies = higher flee priority
            float enemyFactor = Clamp(context.enemiesInRange / 3.0f, 0.0f, 1.0f);
            float healthFactor = InverseLinear(context.healthPercent / 0.2f);

            return healthFactor * (0.5f + 0.5f * enemyFactor);
        }

        return 0.0f;
    }
};

/**
 * @brief Evaluates need to focus on mana regeneration
 * High score when mana is low and not in combat
 */
class TC_GAME_API ManaRegenerationEvaluator : public UtilityEvaluator
{
public:
    ManaRegenerationEvaluator() : UtilityEvaluator("ManaRegeneration", 1.0f) {}

    float Evaluate(UtilityContext const& context) const override
    {
        // In combat = can't regen efficiently
        if (context.inCombat)
            return 0.0f;

        // High mana = no need to regen
        if (context.manaPercent > 0.8f)
            return 0.0f;

        // Inverse mana (lower mana = higher priority)
        return InverseLinear(context.manaPercent);
    }
};

/**
 * @brief Evaluates need for area-of-effect damage
 * High score when multiple enemies are grouped together
 */
class TC_GAME_API AoEDamageEvaluator : public UtilityEvaluator
{
public:
    AoEDamageEvaluator() : UtilityEvaluator("AoEDamage", 1.0f) {}

    float Evaluate(UtilityContext const& context) const override
    {
        // Less than 3 enemies = single target better
        if (context.enemiesInRange < 3)
            return 0.0f;

        // Scale up with enemy count (optimal at 5+ enemies)
        float enemyFactor = Clamp((context.enemiesInRange - 2) / 3.0f, 0.0f, 1.0f);

        // Health factor (need mana/rage for AoE)
        float resourceFactor = context.manaPercent > 0.3f ? 1.0f : context.manaPercent / 0.3f;

        return enemyFactor * resourceFactor;
    }
};

/**
 * @brief Evaluates need to dispel harmful effects from allies
 * High score when allies have dispellable debuffs
 *
 * Uses blackboard keys:
 * - "dispel_count_magic": Number of allies with magic debuffs
 * - "dispel_count_curse": Number of allies with curse debuffs
 * - "dispel_count_disease": Number of allies with disease debuffs
 * - "dispel_count_poison": Number of allies with poison debuffs
 * - "dispel_priority_target": GUID of highest priority dispel target
 * - "dispel_urgent": Boolean flag for urgent dispel (CC/silence effects)
 */
class TC_GAME_API DispelEvaluator : public UtilityEvaluator
{
public:
    DispelEvaluator() : UtilityEvaluator("Dispel", 1.5f) {}

    float Evaluate(UtilityContext const& context) const override
    {
        // Only healers and support can dispel
        if (context.role != UtilityContext::Role::HEALER &&
            context.role != UtilityContext::Role::SUPPORT)
            return 0.0f;

        // Not in combat = no dispel urgency
        if (!context.inCombat)
            return 0.0f;

        // No mana = can't dispel
        if (context.manaPercent < 0.05f)
            return 0.0f;

        // Check blackboard for dispellable debuff counts
        if (context.blackboard)
        {
            // Check for urgent dispels (CC/silence/incapacitate)
            bool urgentDispel = false;
            if (context.blackboard->Get("dispel_urgent", urgentDispel) && urgentDispel)
                return 1.0f;  // Maximum priority for urgent dispels

            // Count dispellable debuffs from blackboard
            uint32 magicCount = 0, curseCount = 0, diseaseCount = 0, poisonCount = 0;
            context.blackboard->Get("dispel_count_magic", magicCount);
            context.blackboard->Get("dispel_count_curse", curseCount);
            context.blackboard->Get("dispel_count_disease", diseaseCount);
            context.blackboard->Get("dispel_count_poison", poisonCount);

            uint32 totalDebuffs = magicCount + curseCount + diseaseCount + poisonCount;

            if (totalDebuffs > 0)
            {
                // Scale priority based on number of debuffs
                // 1 debuff = 0.5, 2 = 0.65, 3 = 0.75, 4+ = 0.85
                float baseScore = 0.5f + (Clamp(totalDebuffs / 8.0f, 0.0f, 0.35f));

                // Magic debuffs often more dangerous (damage, CC)
                if (magicCount > 0)
                    baseScore += 0.1f;

                // Mana availability factor
                float manaFactor = Quadratic(context.manaPercent);

                return Clamp(baseScore * manaFactor, 0.0f, 0.95f);
            }
        }

        // Fallback: moderate priority if in group during combat
        // (blackboard may not have been updated yet)
        if (context.inGroup)
            return 0.3f;

        return 0.0f;
    }
};

} // namespace Playerbot

#endif // TRINITYCORE_COMBAT_EVALUATORS_H
