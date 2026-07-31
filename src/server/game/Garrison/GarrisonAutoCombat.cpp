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

#include "GarrisonAutoCombat.h"
#include "DB2Stores.h"
#include "GarrisonMgr.h"
#include "Log.h"
#include "Random.h"
#include <algorithm>

int32 AutoCombatCombatant::GetEffectiveAttack() const
{
    int32 attack = BaseAttack;
    for (auto const& mod : AttackModifiers)
        attack += mod.Amount;
    return std::max(attack, 0);
}

void AutoCombatCombatant::TickPeriodicEffects(AutoCombatRound& round)
{
    for (auto it = PeriodicEffects.begin(); it != PeriodicEffects.end();)
    {
        if (it->RemainingTicks <= 0)
        {
            it = PeriodicEffects.erase(it);
            continue;
        }

        AutoCombatEvent event;
        event.CasterBoardIndex = it->SourceBoardIndex;
        event.TargetBoardIndex = BoardIndex;
        event.SpellID = it->SpellID;
        event.Amount = it->Amount;

        if (it->IsDamage)
        {
            event.EffectType = AUTO_COMBAT_EFFECT_DOT;
            int32 dmg = it->Amount;
            if (ShieldAmount > 0)
            {
                int32 absorbed = std::min(ShieldAmount, dmg);
                ShieldAmount -= absorbed;
                dmg -= absorbed;
            }
            CurrentHealth -= dmg;
            if (CurrentHealth < 0)
                CurrentHealth = 0;
        }
        else
        {
            event.EffectType = AUTO_COMBAT_EFFECT_HOT;
            CurrentHealth = std::min(CurrentHealth + it->Amount, MaxHealth);
        }

        round.Events.push_back(event);
        --it->RemainingTicks;

        if (it->RemainingTicks <= 0)
            it = PeriodicEffects.erase(it);
        else
            ++it;
    }
}

void AutoCombatCombatant::TickCooldowns()
{
    for (auto it = SpellCooldowns.begin(); it != SpellCooldowns.end();)
    {
        --it->second;
        if (it->second <= 0)
            it = SpellCooldowns.erase(it);
        else
            ++it;
    }
}

void AutoCombatCombatant::TickModifiers()
{
    for (auto it = AttackModifiers.begin(); it != AttackModifiers.end();)
    {
        --it->RemainingRounds;
        if (it->RemainingRounds <= 0)
            it = AttackModifiers.erase(it);
        else
            ++it;
    }
}

AutoCombatResult GarrisonAutoCombat::SimulateCombat(
    std::vector<AutoCombatCombatant>& playerUnits,
    std::vector<AutoCombatCombatant>& enemyUnits)
{
    AutoCombatResult result;

    if (playerUnits.empty() || enemyUnits.empty())
    {
        result.PlayerWon = !playerUnits.empty();
        return result;
    }

    // Sort all combatants by board index for turn order
    std::vector<AutoCombatCombatant*> turnOrder;
    for (auto& unit : playerUnits)
        turnOrder.push_back(&unit);
    for (auto& unit : enemyUnits)
        turnOrder.push_back(&unit);

    std::sort(turnOrder.begin(), turnOrder.end(),
        [](AutoCombatCombatant const* a, AutoCombatCombatant const* b)
        {
            return a->BoardIndex < b->BoardIndex;
        });

    for (int32 roundNum = 1; roundNum <= MAX_ROUNDS; ++roundNum)
    {
        AutoCombatRound round;
        round.RoundNumber = roundNum;

        // Process periodic effects at round start
        for (auto* combatant : turnOrder)
        {
            if (combatant->IsAlive())
                combatant->TickPeriodicEffects(round);
        }

        // Check for deaths after periodic effects
        if (!IsTeamAlive(playerUnits) || !IsTeamAlive(enemyUnits))
        {
            result.CombatLog.push_back(std::move(round));
            break;
        }

        // Each combatant takes a turn in board index order
        for (auto* combatant : turnOrder)
        {
            if (!combatant->IsAlive())
                continue;

            if (combatant->IsPlayerSide)
                ProcessTurn(*combatant, playerUnits, enemyUnits, round);
            else
                ProcessTurn(*combatant, enemyUnits, playerUnits, round);

            // Check if either side is eliminated after each turn
            if (!IsTeamAlive(playerUnits) || !IsTeamAlive(enemyUnits))
                break;
        }

        // Tick cooldowns and buff durations at end of round
        for (auto* combatant : turnOrder)
        {
            if (combatant->IsAlive())
            {
                combatant->TickCooldowns();
                combatant->TickModifiers();
            }
        }

        result.CombatLog.push_back(std::move(round));

        if (!IsTeamAlive(playerUnits) || !IsTeamAlive(enemyUnits))
            break;
    }

    result.PlayerWon = IsTeamAlive(playerUnits);
    result.TotalRounds = static_cast<int32>(result.CombatLog.size());

    return result;
}

AutoCombatCombatant GarrisonAutoCombat::BuildFollowerCombatant(
    uint32 followerLevel, uint32 quality, uint32 itemLevelWeapon,
    uint32 itemLevelArmor, int8 boardIndex, uint64 followerDbID)
{
    AutoCombatCombatant combatant;

    // No DB2 publishes follower auto-combat stats directly (GarrAutoCombatant only
    // covers enemies via GarrEncounterID). The formula below is an empirical scaling
    // calibrated so that quality/level-equivalent followers fall within the 1000-3000
    // HP range typical of GarrAutoCombatant enemy entries; tune against sniffed
    // adventure outcomes when available.
    uint32 avgItemLevel = (itemLevelWeapon + itemLevelArmor) / 2;

    combatant.MaxHealth = 1000 + static_cast<int32>(followerLevel) * 100
        + static_cast<int32>(quality) * 200
        + static_cast<int32>(avgItemLevel) * 5;
    combatant.CurrentHealth = combatant.MaxHealth;
    combatant.BaseAttack = 50 + static_cast<int32>(followerLevel) * 10
        + static_cast<int32>(quality) * 20
        + static_cast<int32>(avgItemLevel) * 2;
    combatant.BoardIndex = boardIndex;
    combatant.IsPlayerSide = true;
    combatant.FollowerDbID = followerDbID;
    combatant.Role = AUTO_COMBAT_ROLE_DPS;

    return combatant;
}

AutoCombatCombatant GarrisonAutoCombat::BuildEnemyCombatant(
    GarrAutoCombatantEntry const* entry)
{
    AutoCombatCombatant combatant;

    combatant.AutoCombatantID = entry->ID;
    combatant.CurrentHealth = entry->Health;
    combatant.MaxHealth = entry->MaxHealth;
    combatant.BaseAttack = entry->Attack;
    combatant.BoardIndex = static_cast<int8>(entry->BoardIndex);
    combatant.Role = entry->Role;
    combatant.AutoAttackSpellID = entry->AutoAttackSpellID;
    combatant.PrimarySpellID = entry->GarrAutoSpellID;
    combatant.Flags = entry->Flags;
    combatant.IsPlayerSide = false;

    return combatant;
}

void GarrisonAutoCombat::ProcessTurn(
    AutoCombatCombatant& combatant,
    std::vector<AutoCombatCombatant>& allies,
    std::vector<AutoCombatCombatant>& enemies,
    AutoCombatRound& round)
{
    if (!combatant.IsAlive())
        return;

    // Healers prioritize healing wounded allies
    if (combatant.Role == AUTO_COMBAT_ROLE_HEALER)
    {
        AutoCombatCombatant* woundedAlly = FindLowestHPAlive(allies);
        if (woundedAlly && woundedAlly->CurrentHealth < woundedAlly->MaxHealth)
        {
            // Try primary heal spell if available and not on cooldown
            if (combatant.PrimarySpellID != 0
                && combatant.SpellCooldowns.find(combatant.PrimarySpellID) == combatant.SpellCooldowns.end())
            {
                ResolveSpell(combatant, combatant.PrimarySpellID, allies, enemies, round);
                return;
            }
        }
    }

    // Try primary spell if available and not on cooldown
    if (combatant.PrimarySpellID != 0
        && combatant.SpellCooldowns.find(combatant.PrimarySpellID) == combatant.SpellCooldowns.end())
    {
        ResolveSpell(combatant, combatant.PrimarySpellID, allies, enemies, round);
        return;
    }

    // Fall back to auto-attack
    if (combatant.AutoAttackSpellID != 0)
    {
        ResolveSpell(combatant, combatant.AutoAttackSpellID, allies, enemies, round);
        return;
    }

    // Last resort: basic attack on first alive enemy
    AutoCombatCombatant* target = nullptr;
    for (auto& enemy : enemies)
    {
        if (enemy.IsAlive())
        {
            target = &enemy;
            break;
        }
    }

    if (target)
    {
        int32 damage = combatant.GetEffectiveAttack();
        ApplyDamage(*target, damage, combatant.BoardIndex, 0,
            AUTO_COMBAT_EFFECT_DAMAGE, round);
    }
}

void GarrisonAutoCombat::ResolveSpell(
    AutoCombatCombatant& caster, int32 spellID,
    std::vector<AutoCombatCombatant>& allies,
    std::vector<AutoCombatCombatant>& enemies,
    AutoCombatRound& round)
{
    GarrAutoSpellEntry const* spell = sGarrAutoSpellStore.LookupEntry(spellID);
    if (!spell)
    {
        // Unknown spell — fall back to basic attack
        for (auto& enemy : enemies)
        {
            if (enemy.IsAlive())
            {
                ApplyDamage(enemy, caster.GetEffectiveAttack(),
                    caster.BoardIndex, 0, AUTO_COMBAT_EFFECT_DAMAGE, round);
                break;
            }
        }
        return;
    }

    // Get spell effects
    std::vector<GarrAutoSpellEffectEntry const*> const* effects =
        sGarrisonMgr.GetAutoSpellEffects(spellID);

    if (!effects || effects->empty())
    {
        // Spell with no effects — treat as single-target damage using caster attack
        for (auto& enemy : enemies)
        {
            if (enemy.IsAlive())
            {
                ApplyDamage(enemy, caster.GetEffectiveAttack(),
                    caster.BoardIndex, spellID, AUTO_COMBAT_EFFECT_DAMAGE, round);
                break;
            }
        }
    }
    else
    {
        // Resolve each effect
        for (GarrAutoSpellEffectEntry const* effect : *effects)
        {
            std::vector<AutoCombatCombatant*> targets =
                SelectTargets(caster, effect->Targets, allies, enemies);

            for (AutoCombatCombatant* target : targets)
            {
                if (target && target->IsAlive())
                    ResolveEffect(caster, effect, *target, spellID, round);
            }
        }
    }

    // Apply cooldown
    if (spell->Cooldown > 0)
        caster.SpellCooldowns[spellID] = spell->Cooldown;
}

void GarrisonAutoCombat::ResolveEffect(
    AutoCombatCombatant& caster, GarrAutoSpellEffectEntry const* effect,
    AutoCombatCombatant& target, uint32 spellID,
    AutoCombatRound& round)
{
    // Scale effect amount by caster attack
    int32 amount = static_cast<int32>(effect->Amount);
    if (amount == 0)
        amount = caster.GetEffectiveAttack();

    switch (effect->EffectType)
    {
        case AUTO_COMBAT_EFFECT_DAMAGE:
            ApplyDamage(target, amount, caster.BoardIndex, spellID,
                AUTO_COMBAT_EFFECT_DAMAGE, round);
            break;

        case AUTO_COMBAT_EFFECT_HEAL:
            ApplyHealing(target, amount, caster.BoardIndex, spellID, round);
            break;

        case AUTO_COMBAT_EFFECT_BUFF_ATTACK:
        {
            AutoCombatAttackModifier mod;
            mod.Amount = amount;
            mod.RemainingRounds = effect->Period > 0 ? effect->Period : 3;
            target.AttackModifiers.push_back(mod);

            AutoCombatEvent event;
            event.CasterBoardIndex = caster.BoardIndex;
            event.TargetBoardIndex = target.BoardIndex;
            event.SpellID = spellID;
            event.Amount = amount;
            event.EffectType = AUTO_COMBAT_EFFECT_BUFF_ATTACK;
            round.Events.push_back(event);
            break;
        }

        case AUTO_COMBAT_EFFECT_DEBUFF_ATTACK:
        {
            AutoCombatAttackModifier mod;
            mod.Amount = -amount;
            mod.RemainingRounds = effect->Period > 0 ? effect->Period : 3;
            target.AttackModifiers.push_back(mod);

            AutoCombatEvent event;
            event.CasterBoardIndex = caster.BoardIndex;
            event.TargetBoardIndex = target.BoardIndex;
            event.SpellID = spellID;
            event.Amount = -amount;
            event.EffectType = AUTO_COMBAT_EFFECT_DEBUFF_ATTACK;
            round.Events.push_back(event);
            break;
        }

        case AUTO_COMBAT_EFFECT_SHIELD:
        {
            target.ShieldAmount += amount;

            AutoCombatEvent event;
            event.CasterBoardIndex = caster.BoardIndex;
            event.TargetBoardIndex = target.BoardIndex;
            event.SpellID = spellID;
            event.Amount = amount;
            event.EffectType = AUTO_COMBAT_EFFECT_SHIELD;
            round.Events.push_back(event);
            break;
        }

        case AUTO_COMBAT_EFFECT_AOE_DAMAGE:
            // Already handled by target selection returning all enemies
            ApplyDamage(target, amount, caster.BoardIndex, spellID,
                AUTO_COMBAT_EFFECT_AOE_DAMAGE, round);
            break;

        case AUTO_COMBAT_EFFECT_HOT:
        {
            AutoCombatPeriodicEffect periodic;
            periodic.SpellID = spellID;
            periodic.Amount = amount;
            periodic.RemainingTicks = effect->Period > 0 ? effect->Period : 3;
            periodic.IsDamage = false;
            periodic.SourceBoardIndex = caster.BoardIndex;
            target.PeriodicEffects.push_back(periodic);

            AutoCombatEvent event;
            event.CasterBoardIndex = caster.BoardIndex;
            event.TargetBoardIndex = target.BoardIndex;
            event.SpellID = spellID;
            event.Amount = amount;
            event.EffectType = AUTO_COMBAT_EFFECT_HOT;
            round.Events.push_back(event);
            break;
        }

        case AUTO_COMBAT_EFFECT_DOT:
        {
            AutoCombatPeriodicEffect periodic;
            periodic.SpellID = spellID;
            periodic.Amount = amount;
            periodic.RemainingTicks = effect->Period > 0 ? effect->Period : 3;
            periodic.IsDamage = true;
            periodic.SourceBoardIndex = caster.BoardIndex;
            target.PeriodicEffects.push_back(periodic);

            AutoCombatEvent event;
            event.CasterBoardIndex = caster.BoardIndex;
            event.TargetBoardIndex = target.BoardIndex;
            event.SpellID = spellID;
            event.Amount = amount;
            event.EffectType = AUTO_COMBAT_EFFECT_DOT;
            round.Events.push_back(event);
            break;
        }

        default:
            TC_LOG_DEBUG("garrison", "GarrisonAutoCombat: Unknown effect type {} for spell {}",
                effect->EffectType, spellID);
            break;
    }
}

std::vector<AutoCombatCombatant*> GarrisonAutoCombat::SelectTargets(
    AutoCombatCombatant& caster, uint8 targetType,
    std::vector<AutoCombatCombatant>& allies,
    std::vector<AutoCombatCombatant>& enemies)
{
    std::vector<AutoCombatCombatant*> targets;

    switch (targetType)
    {
        case AUTO_COMBAT_TARGET_SELF:
            targets.push_back(&caster);
            break;

        case AUTO_COMBAT_TARGET_SINGLE_ENEMY:
        {
            // Target first alive enemy (by board index)
            for (auto& enemy : enemies)
            {
                if (enemy.IsAlive())
                {
                    targets.push_back(&enemy);
                    break;
                }
            }
            break;
        }

        case AUTO_COMBAT_TARGET_ALL_ENEMIES:
            for (auto& enemy : enemies)
                if (enemy.IsAlive())
                    targets.push_back(&enemy);
            break;

        case AUTO_COMBAT_TARGET_SINGLE_ALLY:
        {
            // Prefer wounded ally
            AutoCombatCombatant* wounded = FindLowestHPAlive(allies);
            if (wounded)
                targets.push_back(wounded);
            break;
        }

        case AUTO_COMBAT_TARGET_ALL_ALLIES:
            for (auto& ally : allies)
                if (ally.IsAlive())
                    targets.push_back(&ally);
            break;

        case AUTO_COMBAT_TARGET_LOWEST_HP_ALLY:
        {
            AutoCombatCombatant* lowest = FindLowestHPAlive(allies);
            if (lowest)
                targets.push_back(lowest);
            break;
        }

        case AUTO_COMBAT_TARGET_HIGHEST_HP_ENEMY:
        {
            AutoCombatCombatant* highest = FindHighestHPAlive(enemies);
            if (highest)
                targets.push_back(highest);
            break;
        }

        case AUTO_COMBAT_TARGET_RANDOM_ENEMY:
        {
            std::vector<AutoCombatCombatant*> alive;
            for (auto& enemy : enemies)
                if (enemy.IsAlive())
                    alive.push_back(&enemy);
            if (!alive.empty())
                targets.push_back(alive[urand(0, static_cast<uint32>(alive.size()) - 1)]);
            break;
        }

        default:
            // Fall back to single enemy
            for (auto& enemy : enemies)
            {
                if (enemy.IsAlive())
                {
                    targets.push_back(&enemy);
                    break;
                }
            }
            break;
    }

    return targets;
}

void GarrisonAutoCombat::ApplyDamage(
    AutoCombatCombatant& target, int32 amount,
    int8 casterBoardIndex, uint32 spellID, uint8 effectType,
    AutoCombatRound& round)
{
    int32 damage = amount;

    // Absorb shields first
    if (target.ShieldAmount > 0)
    {
        int32 absorbed = std::min(target.ShieldAmount, damage);
        target.ShieldAmount -= absorbed;
        damage -= absorbed;
    }

    target.CurrentHealth -= damage;
    if (target.CurrentHealth < 0)
        target.CurrentHealth = 0;

    AutoCombatEvent event;
    event.CasterBoardIndex = casterBoardIndex;
    event.TargetBoardIndex = target.BoardIndex;
    event.SpellID = spellID;
    event.Amount = amount;
    event.EffectType = effectType;
    round.Events.push_back(event);
}

void GarrisonAutoCombat::ApplyHealing(
    AutoCombatCombatant& target, int32 amount,
    int8 casterBoardIndex, uint32 spellID,
    AutoCombatRound& round)
{
    target.CurrentHealth = std::min(target.CurrentHealth + amount, target.MaxHealth);

    AutoCombatEvent event;
    event.CasterBoardIndex = casterBoardIndex;
    event.TargetBoardIndex = target.BoardIndex;
    event.SpellID = spellID;
    event.Amount = amount;
    event.EffectType = AUTO_COMBAT_EFFECT_HEAL;
    round.Events.push_back(event);
}

bool GarrisonAutoCombat::IsTeamAlive(std::vector<AutoCombatCombatant> const& team)
{
    for (auto const& combatant : team)
        if (combatant.IsAlive())
            return true;
    return false;
}

AutoCombatCombatant* GarrisonAutoCombat::FindLowestHPAlive(std::vector<AutoCombatCombatant>& team)
{
    AutoCombatCombatant* lowest = nullptr;
    float lowestPct = 2.0f;

    for (auto& combatant : team)
    {
        if (!combatant.IsAlive())
            continue;

        float pct = static_cast<float>(combatant.CurrentHealth) /
            static_cast<float>(std::max(combatant.MaxHealth, 1));

        if (pct < lowestPct)
        {
            lowestPct = pct;
            lowest = &combatant;
        }
    }

    return lowest;
}

AutoCombatCombatant* GarrisonAutoCombat::FindHighestHPAlive(std::vector<AutoCombatCombatant>& team)
{
    AutoCombatCombatant* highest = nullptr;
    int32 highestHP = -1;

    for (auto& combatant : team)
    {
        if (!combatant.IsAlive())
            continue;

        if (combatant.CurrentHealth > highestHP)
        {
            highestHP = combatant.CurrentHealth;
            highest = &combatant;
        }
    }

    return highest;
}
