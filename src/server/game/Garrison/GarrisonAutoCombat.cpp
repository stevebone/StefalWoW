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

        // Every periodic was created from a resolved GarrAutoSpell effect row, so SpellID/EffectIndex
        // are the ones the applying cast reported and the tick can be replayed against the same aura.
        AutoCombatEvent event;
        event.CasterBoardIndex = it->SourceBoardIndex;
        event.TargetBoardIndex = BoardIndex;
        event.SpellID = it->SpellID;
        event.Amount = it->Amount;
        event.EffectIndex = it->EffectIndex;
        event.CasterRole = it->SourceCasterRole;
        event.TargetOldHealth = CurrentHealth;
        event.TargetMaxHealth = MaxHealth;

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

        event.TargetNewHealth = CurrentHealth;
        event.IsPeriodicTick = true;
        event.TargetDied = event.TargetOldHealth > 0 && CurrentHealth == 0;
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

// A GarrAutoCombatant statline is a level curve: HealthBase/AttackBase are the level-1 values and
// the *GainPerLevel columns the per-level increment. Both sides of an Adventures board are built
// from the same table, so both scale the same way.
int32 GarrisonAutoCombat::ScaleHealth(GarrAutoCombatantEntry const* entry, uint32 level)
{
    int32 levelsGained = int32(std::max<uint32>(level, 1) - 1);
    return std::max(entry->HealthBase + entry->HealthGainPerLevel * levelsGained, 1);
}

int32 GarrisonAutoCombat::ScaleAttack(GarrAutoCombatantEntry const* entry, uint32 level)
{
    int32 levelsGained = int32(std::max<uint32>(level, 1) - 1);
    return std::max(entry->AttackBase + entry->AttackGainPerLevel * levelsGained, 0);
}

AutoCombatCombatant GarrisonAutoCombat::BuildFollowerCombatant(
    GarrFollowerEntry const* followerEntry,
    uint32 followerLevel, uint32 quality, uint32 itemLevelWeapon,
    uint32 itemLevelArmor, int8 boardIndex, uint64 followerDbID)
{
    AutoCombatCombatant combatant;
    combatant.BoardIndex = boardIndex;
    combatant.IsPlayerSide = true;
    combatant.FollowerDbID = followerDbID;

    // Shadowlands companions (GarrFollowerType 123) publish their entire auto-combat statline in
    // GarrAutoCombatant, reached through GarrFollower.AutoCombatantID - all 138 type-123 rows carry
    // a non-zero, non-dangling id. Rarity is already baked into the referenced row (each companion
    // points at its own statline), so no quality/item-level term belongs here.
    if (followerEntry && followerEntry->AutoCombatantID)
    {
        if (GarrAutoCombatantEntry const* statline = sGarrisonMgr.GetAutoCombatant(followerEntry->AutoCombatantID))
        {
            combatant.AutoCombatantID = statline->ID;
            combatant.MaxHealth = ScaleHealth(statline, followerLevel);
            combatant.CurrentHealth = combatant.MaxHealth;
            combatant.BaseAttack = ScaleAttack(statline, followerLevel);
            combatant.Role = statline->Role;
            combatant.AutoAttackSpellID = statline->AttackSpellID;
            combatant.PrimarySpellID = statline->AbilitySpellID;
            combatant.SecondarySpellID = statline->AbilitySpellID2;
            combatant.PassiveSpellID = statline->PassiveSpellID;
            return combatant;
        }
    }

    // WoD / Legion / War Campaign followers publish no AutoCombatantID, and none of their missions
    // carry auto-combat encounters (every GarrEncounter with an AutoCombatantID belongs to a
    // GarrTypeID 111 mission), so this branch is only reachable for a hand-built simulation. Keep
    // the pre-existing approximation there rather than leaving those followers statless.
    uint32 avgItemLevel = (itemLevelWeapon + itemLevelArmor) / 2;

    combatant.MaxHealth = 1000 + static_cast<int32>(followerLevel) * 100
        + static_cast<int32>(quality) * 200
        + static_cast<int32>(avgItemLevel) * 5;
    combatant.CurrentHealth = combatant.MaxHealth;
    combatant.BaseAttack = 50 + static_cast<int32>(followerLevel) * 10
        + static_cast<int32>(quality) * 20
        + static_cast<int32>(avgItemLevel) * 2;
    combatant.Role = AUTO_COMBAT_ROLE_MELEE;

    return combatant;
}

AutoCombatCombatant GarrisonAutoCombat::BuildEnemyCombatant(
    GarrAutoCombatantEntry const* entry, uint32 level, int8 boardIndex)
{
    AutoCombatCombatant combatant;

    combatant.AutoCombatantID = entry->ID;
    combatant.MaxHealth = ScaleHealth(entry, level);
    combatant.CurrentHealth = combatant.MaxHealth;
    combatant.BaseAttack = ScaleAttack(entry, level);
    combatant.BoardIndex = boardIndex;
    combatant.Role = entry->Role;
    combatant.AutoAttackSpellID = entry->AttackSpellID;
    combatant.PrimarySpellID = entry->AbilitySpellID;
    combatant.SecondarySpellID = entry->AbilitySpellID2;
    combatant.PassiveSpellID = entry->PassiveSpellID;
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

    // Each option is only spent if it actually produced replay events, so an ability built entirely
    // out of effect kinds this build cannot simulate falls through to the next slot instead of
    // silently costing the combatant its turn.
    auto tryCast = [&](int32 spellID)
    {
        if (spellID == 0)
            return false;
        if (combatant.SpellCooldowns.find(spellID) != combatant.SpellCooldowns.end())
            return false;
        return ResolveSpell(combatant, spellID, allies, enemies, round);
    };

    // Healers prioritize healing wounded allies
    if (combatant.Role == AUTO_COMBAT_ROLE_HEAL_SUPPORT)
    {
        AutoCombatCombatant* woundedAlly = FindLowestHPAlive(allies);
        if (woundedAlly && woundedAlly->CurrentHealth < woundedAlly->MaxHealth)
            if (tryCast(combatant.PrimarySpellID))
                return;
    }

    if (tryCast(combatant.PrimarySpellID))
        return;

    // GarrAutoCombatant publishes a second ability slot; use it while the first is on cooldown
    // rather than dropping straight to the auto-attack.
    if (tryCast(combatant.SecondarySpellID))
        return;

    // Fall back to the combatant's own auto-attack. GarrAutoCombatant.AttackSpellID is a real
    // GarrAutoSpell (all 355 references in 68275 resolve, and every one of them is built out of
    // Effect 1 DealAutoDamage rows), so this is where an auto-attack's spell id comes from.
    if (tryCast(combatant.AutoAttackSpellID))
        return;

    // Eight GarrAutoCombatant rows in 68275 publish AttackSpellID 0 (1, 212, 216-219, 292, 293; row 1
    // is the JasonTest statline the five DNT companions 1203-1207 and encounter 2546 point at). There
    // is no spell to attribute a swing to, and an event without a GarrAutoSpell faults the client's
    // combat log, so the combatant simply does nothing this round.
    TC_LOG_DEBUG("garrison", "GarrisonAutoCombat: combatant {} (board {}) had no usable auto-combat "
        "spell this round; no event emitted", combatant.AutoCombatantID, combatant.BoardIndex);
}

bool GarrisonAutoCombat::TranslateSpellEffect(uint8 dbEffect, uint8& simulatedEffect)
{
    switch (dbEffect)
    {
        case GARR_AUTO_SPELL_EFFECT_DEAL_AUTO_DAMAGE:
        case GARR_AUTO_SPELL_EFFECT_DEAL_DAMAGE:
            simulatedEffect = AUTO_COMBAT_EFFECT_DAMAGE;
            return true;
        case GARR_AUTO_SPELL_EFFECT_HEAL:
        case GARR_AUTO_SPELL_EFFECT_HEAL_ALT:
            simulatedEffect = AUTO_COMBAT_EFFECT_HEAL;
            return true;
        case GARR_AUTO_SPELL_EFFECT_DOT:
            simulatedEffect = AUTO_COMBAT_EFFECT_DOT;
            return true;
        case GARR_AUTO_SPELL_EFFECT_HOT:
            simulatedEffect = AUTO_COMBAT_EFFECT_HOT;
            return true;
        default:
            return false;
    }
}

bool GarrisonAutoCombat::ResolveSpell(
    AutoCombatCombatant& caster, int32 spellID,
    std::vector<AutoCombatCombatant>& allies,
    std::vector<AutoCombatCombatant>& enemies,
    AutoCombatRound& round)
{
    if (spellID == 0)
        return false;

    // No fallback swing here any more. The only thing a caster could be credited with when the spell
    // does not resolve is a hit with no GarrAutoSpell behind it, and the replay cannot render that:
    // AdventuresCombatLogMixin:AddCombatEvent feeds combatLogEvent.spellID straight into
    // C_Garrison.GetCombatLogSpellInfo and indexes the result, so a zero id faults the combat log.
    GarrAutoSpellEntry const* spell = sGarrAutoSpellStore.LookupEntry(spellID);
    if (!spell)
    {
        TC_LOG_DEBUG("garrison", "GarrisonAutoCombat: combatant {} references GarrAutoSpell {} which "
            "does not exist; skipping", caster.AutoCombatantID, spellID);
        return false;
    }

    std::vector<GarrAutoSpellEffectEntry const*> const* effects =
        sGarrisonMgr.GetAutoSpellEffects(spellID);

    if (!effects || effects->empty())
    {
        TC_LOG_DEBUG("garrison", "GarrisonAutoCombat: GarrAutoSpell {} has no GarrAutoSpellEffect rows; "
            "skipping", spellID);
        return false;
    }

    std::size_t const eventsBefore = round.Events.size();

    for (GarrAutoSpellEffectEntry const* effect : *effects)
    {
        uint8 simulatedEffect = 0;
        if (!TranslateSpellEffect(effect->Effect, simulatedEffect))
        {
            TC_LOG_DEBUG("garrison", "GarrisonAutoCombat: GarrAutoSpell {} effect index {} uses "
                "GarrAutoSpellEffect.Effect {}, which this build does not simulate; no event emitted "
                "for it", spellID, effect->EffectIndex, effect->Effect);
            continue;
        }

        std::vector<AutoCombatCombatant*> targets =
            SelectTargets(caster, effect->TargetType, allies, enemies);

        for (AutoCombatCombatant* target : targets)
            if (target && target->IsAlive())
                ResolveEffect(caster, effect, *target, spellID, simulatedEffect, round);
    }

    if (round.Events.size() == eventsBefore)
        return false;

    // Only a cast that actually happened goes on cooldown.
    if (spell->Cooldown > 0)
        caster.SpellCooldowns[spellID] = spell->Cooldown;

    return true;
}

void GarrisonAutoCombat::ResolveEffect(
    AutoCombatCombatant& caster, GarrAutoSpellEffectEntry const* effect,
    AutoCombatCombatant& target, uint32 spellID, uint8 simulatedEffect,
    AutoCombatRound& round)
{
    // GarrAutoSpellEffect.Points is a coefficient, not a flat amount - "Double Strike ... dealing a
    // $s1 and then $s2 Physical damage" (GarrAutoSpell 4) carries 0.75 and 0.5 - but what it is a
    // coefficient *of* is not published consistently across the effect kinds (GarrAutoSpell 8
    // "Hawk Punch" carries 10 on the same kind that GarrAutoSpell 11 "Auto Attack" carries 1). The
    // pre-existing approximation is kept unchanged rather than replaced by a guessed formula; the
    // magnitudes are a separate open item from the event layout this change fixes.
    int32 amount = static_cast<int32>(effect->Points);
    if (amount == 0)
        amount = caster.GetEffectiveAttack();

    bool const isAutoAttack = effect->Effect == GARR_AUTO_SPELL_EFFECT_DEAL_AUTO_DAMAGE;

    switch (simulatedEffect)
    {
        case AUTO_COMBAT_EFFECT_DAMAGE:
            ApplyDamage(target, amount, caster, spellID, AUTO_COMBAT_EFFECT_DAMAGE,
                effect->EffectIndex, isAutoAttack, round);
            break;

        case AUTO_COMBAT_EFFECT_HEAL:
            ApplyHealing(target, amount, caster, spellID, effect->EffectIndex, round);
            break;

        case AUTO_COMBAT_EFFECT_HOT:
        case AUTO_COMBAT_EFFECT_DOT:
        {
            bool const isDamage = simulatedEffect == AUTO_COMBAT_EFFECT_DOT;

            AutoCombatPeriodicEffect periodic;
            periodic.SpellID = spellID;
            periodic.Amount = amount;
            periodic.RemainingTicks = effect->Period > 0 ? effect->Period : 3;
            periodic.IsDamage = isDamage;
            periodic.SourceBoardIndex = caster.BoardIndex;
            periodic.EffectIndex = effect->EffectIndex;
            periodic.SourceCasterRole = caster.Role;
            target.PeriodicEffects.push_back(periodic);

            // The application itself is an aura on the client, not a hit; the ticks that follow are the
            // PeriodicDamage/PeriodicHeal events, produced by TickPeriodicEffects.
            AutoCombatEvent event;
            event.CasterBoardIndex = caster.BoardIndex;
            event.TargetBoardIndex = target.BoardIndex;
            event.SpellID = spellID;
            event.Amount = amount;
            event.EffectType = isDamage ? AUTO_COMBAT_EFFECT_DOT : AUTO_COMBAT_EFFECT_HOT;
            // Health is untouched by this effect, but the replay still needs the bar values.
            event.TargetOldHealth = target.CurrentHealth;
            event.TargetNewHealth = target.CurrentHealth;
            event.TargetMaxHealth = target.MaxHealth;
            event.CasterRole = caster.Role;
            event.EffectIndex = effect->EffectIndex;
            round.Events.push_back(event);
            break;
        }

        default:
            // Unreachable: TranslateSpellEffect only produces the four cases above.
            break;
    }
}

std::vector<AutoCombatCombatant*> GarrisonAutoCombat::SelectTargets(
    AutoCombatCombatant& caster, uint8 targetMask,
    std::vector<AutoCombatCombatant>& allies,
    std::vector<AutoCombatCombatant>& enemies)
{
    std::vector<AutoCombatCombatant*> targets;

    // Bit 0 on its own is the caster (GarrAutoSpell 17 effect 1, "healing himself", carries mask 1).
    if (targetMask == GARR_AUTO_TARGET_ENEMY_TEAM)
    {
        targets.push_back(&caster);
        return targets;
    }

    bool const hitsEnemies = (targetMask & GARR_AUTO_TARGET_ENEMY_TEAM) != 0;
    std::vector<AutoCombatCombatant>& team = hitsEnemies ? enemies : allies;

    // Near and far together is the whole team ("all enemies" = 1|2|4). A row restriction (8 melee row,
    // 16 ranged row) also names a group rather than one combatant - "all enemies in melee range",
    // "all enemies at range", "all ranged allies" - and since the simulation does not model board rows
    // it resolves to the whole team rather than to an invented subset.
    bool const wholeTeam = ((targetMask & GARR_AUTO_TARGET_NEAR) && (targetMask & GARR_AUTO_TARGET_FAR))
        || (targetMask & GARR_AUTO_TARGET_MELEE_ROW) != 0
        || (targetMask & GARR_AUTO_TARGET_RANGED_ROW) != 0;

    if (wholeTeam)
    {
        for (AutoCombatCombatant& combatant : team)
            if (combatant.IsAlive())
                targets.push_back(&combatant);

        return targets;
    }

    // A single pick. Which one - closest or farthest - is a board-position question the simulation
    // cannot answer, so it takes the first living member in board order for an enemy and the most
    // wounded for an ally, which is what the previous code did and keeps healers useful.
    if (hitsEnemies)
    {
        for (AutoCombatCombatant& enemy : team)
        {
            if (enemy.IsAlive())
            {
                targets.push_back(&enemy);
                break;
            }
        }
    }
    else if (AutoCombatCombatant* wounded = FindLowestHPAlive(team))
        targets.push_back(wounded);

    return targets;
}

void GarrisonAutoCombat::ApplyDamage(
    AutoCombatCombatant& target, int32 amount,
    AutoCombatCombatant const& caster, uint32 spellID, uint8 effectType,
    uint8 effectIndex, bool isAutoAttack, AutoCombatRound& round)
{
    // Guard rather than assert: an event whose spellID is 0 makes the client's combat log index a nil
    // AutoCombatSpellInfo and error out mid-replay, so the hit is dropped instead.
    if (spellID == 0)
    {
        TC_LOG_ERROR("garrison", "GarrisonAutoCombat: refused to log damage from combatant {} onto "
            "board index {} with no GarrAutoSpell behind it", caster.AutoCombatantID, target.BoardIndex);
        return;
    }

    int32 damage = amount;
    int32 const oldHealth = target.CurrentHealth;

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
    event.CasterBoardIndex = caster.BoardIndex;
    event.TargetBoardIndex = target.BoardIndex;
    event.SpellID = spellID;
    event.Amount = amount;
    event.EffectType = effectType;
    event.EffectIndex = effectIndex;
    event.TargetOldHealth = oldHealth;
    event.TargetNewHealth = target.CurrentHealth;
    event.TargetMaxHealth = target.MaxHealth;
    event.CasterRole = caster.Role;
    // Sourced from the effect row (GarrAutoSpellEffect.Effect 1 DealAutoDamage), not from a missing
    // spell id. The combatant's own AttackSpellID agreeing is a second, weaker signal.
    event.IsAutoAttack = isAutoAttack || int32(spellID) == caster.AutoAttackSpellID;
    event.TargetDied = oldHealth > 0 && target.CurrentHealth == 0;
    round.Events.push_back(event);
}

void GarrisonAutoCombat::ApplyHealing(
    AutoCombatCombatant& target, int32 amount,
    AutoCombatCombatant const& caster, uint32 spellID, uint8 effectIndex,
    AutoCombatRound& round)
{
    if (spellID == 0)
    {
        TC_LOG_ERROR("garrison", "GarrisonAutoCombat: refused to log healing from combatant {} onto "
            "board index {} with no GarrAutoSpell behind it", caster.AutoCombatantID, target.BoardIndex);
        return;
    }

    int32 const oldHealth = target.CurrentHealth;
    target.CurrentHealth = std::min(target.CurrentHealth + amount, target.MaxHealth);

    AutoCombatEvent event;
    event.CasterBoardIndex = caster.BoardIndex;
    event.TargetBoardIndex = target.BoardIndex;
    event.SpellID = spellID;
    event.Amount = amount;
    event.EffectType = AUTO_COMBAT_EFFECT_HEAL;
    event.EffectIndex = effectIndex;
    event.CasterRole = caster.Role;
    event.TargetOldHealth = oldHealth;
    event.TargetNewHealth = target.CurrentHealth;
    event.TargetMaxHealth = target.MaxHealth;
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
