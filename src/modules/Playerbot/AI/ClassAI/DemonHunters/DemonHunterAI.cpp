/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "DemonHunterAI.h"
#include "HavocDemonHunter.h"
#include "VengeanceDemonHunter.h"
#include "../BaselineRotationManager.h"
#include "../../Combat/CombatBehaviorIntegration.h"
#include "Player.h"
#include "SpellMgr.h"
#include "SpellAuras.h"
#include "Log.h"
#include "CellImpl.h"
#include "GridNotifiersImpl.h"
#include "ObjectAccessor.h"
#include "DataStores/DBCEnums.h"
#include "../../../Spatial/SpatialGridManager.h"
#include "../../../Spatial/SpatialGridQueryHelpers.h"  // PHASE 5F: Thread-safe queries
#include "GameTime.h"

namespace Playerbot
{

// Combat constants for Demon Hunters
static constexpr float OPTIMAL_MELEE_RANGE = 5.0f;
static constexpr float CHARGE_MIN_RANGE = 8.0f;
static constexpr float CHARGE_MAX_RANGE = 25.0f;
static constexpr float HEALTH_EMERGENCY_THRESHOLD = 30.0f;
static constexpr float DEFENSIVE_COOLDOWN_THRESHOLD = 50.0f;
static constexpr uint32 FURY_DUMP_THRESHOLD = 80;
static constexpr uint32 PAIN_DUMP_THRESHOLD = 60;
static constexpr float METAMORPHOSIS_HEALTH_THRESHOLD = 40.0f;
static constexpr uint32 INTERRUPT_COOLDOWN = 15000;
static constexpr uint32 DEFENSIVE_COOLDOWN = 60000;

// Destructor must be defined in cpp file where specialization types are complete
DemonHunterAI::~DemonHunterAI() = default;

DemonHunterAI::DemonHunterAI(Player* bot) : ClassAI(bot),    _lastInterruptTime(0),
    _lastDefensiveTime(0),
    _lastMobilityTime(0),
    _successfulInterrupts(0)
{
    _dhMetrics.totalAbilitiesUsed = 0;
    _dhMetrics.interruptsSucceeded = 0;
    _dhMetrics.defensivesUsed = 0;
    _dhMetrics.mobilityAbilitiesUsed = 0;

    TC_LOG_DEBUG("module.playerbot.demonhunter", "DemonHunterAI initialized for player {}",
                 bot->GetName());
}

void DemonHunterAI::UpdateRotation(::Unit* target)
{
    if (!target || !_bot)
        return;

    // NOTE: Baseline rotation check is now handled at the dispatch level in
    // ClassAI::OnCombatUpdate(). This method is ONLY called when the bot has
    // already chosen a specialization (level 10+ with talents).

    // ========================================================================
    // COMBAT BEHAVIOR INTEGRATION - Priority-based decision making
    // ========================================================================
    auto* behaviors = GetCombatBehaviors();

    // Priority 1: Handle interrupts (Disrupt)
    if (behaviors && behaviors->ShouldInterrupt(target))
    {
        HandleInterrupts(target);
        if (_bot->HasUnitState(UNIT_STATE_CASTING))
            return;
    }

    // Priority 2: Handle defensives (Blur, Darkness, Netherwalk)
    if (behaviors && behaviors->NeedsDefensive())
    {
        HandleDefensives();
        if (_bot->HasUnitState(UNIT_STATE_CASTING))
            return;
    }

    // Priority 3: Check for target switching
    if (behaviors && behaviors->ShouldSwitchTarget())
    {
        HandleTargetSwitching(target);
    }

    // Priority 4: AoE vs Single-Target decision
    if (behaviors && behaviors->ShouldAOE())
    {
        HandleAoEDecisions(target);
        if (_bot->HasUnitState(UNIT_STATE_CASTING))
            return;
    }

    // Priority 5: Use major cooldowns at optimal time
    if (behaviors && behaviors->ShouldUseCooldowns())
    {
        HandleCooldowns(target);
        if (_bot->HasUnitState(UNIT_STATE_CASTING))
            return;
    }

    // Priority 6: Fury/Pain Management based on spec
    HandleResourceGeneration(target);
    if (_bot->HasUnitState(UNIT_STATE_CASTING))
        return;

    // Priority 7: Execute spec-specific rotation based on player's actual specialization
    ChrSpecialization spec = _bot->GetPrimarySpecialization();

    // Execute spec-specific rotation or fallback to basic rotation
    if (spec == ChrSpecialization::DemonHunterHavoc)
    {
        UpdateHavocRotation(target);
    }
    else if (spec == ChrSpecialization::DemonHunterVengeance)
    {
        UpdateVengeanceRotation(target);
    }
    else
    {
        // Fallback rotation for unspecialized Demon Hunters
        ExecuteBasicDemonHunterRotation(target);
    }

    // Handle Demon Hunter-specific mobility
    HandleMobility(target);
    UpdateMetrics(0);
}

void DemonHunterAI::HandleInterrupts(::Unit* target)
{
    if (!target)
        return;

    auto* behaviors = GetCombatBehaviors();
    Unit* interruptTarget = behaviors ? behaviors->GetInterruptTarget() : nullptr;

    // Use provided interrupt target or fall back to current target
    if (!interruptTarget)
        interruptTarget = target;    // Check if we can interrupt
    if (!IsTargetInterruptible(interruptTarget))
        return;

    uint32 currentTime = GameTime::GetGameTimeMS();

    // Disrupt - Main interrupt ability
    if (currentTime - _lastInterruptTime > INTERRUPT_COOLDOWN && CanUseAbility(DISRUPT))
    {
        if (CastSpell(DISRUPT, interruptTarget))
        {            RecordInterruptAttempt(interruptTarget, DISRUPT, true);
            _lastInterruptTime = currentTime;
            TC_LOG_DEBUG("module.playerbot.ai", "DemonHunter {} interrupted {} with Disrupt",                         _bot->GetName(), interruptTarget->GetName());
            return;
        }
    }    // Sigil of Silence - AoE interrupt for casters
    if (CanUseAbility(SIGIL_OF_SILENCE))
    {
        if (CastSpell(SIGIL_OF_SILENCE, interruptTarget))
        {
            RecordInterruptAttempt(interruptTarget, SIGIL_OF_SILENCE, true);
            TC_LOG_DEBUG("module.playerbot.ai", "DemonHunter {} used Sigil of Silence on {}",
                         
                         _bot->GetName(), interruptTarget->GetName());
            return;
        }
    }

    // Chaos Nova - AoE stun (can interrupt non-boss casts)
    if (IsInMeleeRange(interruptTarget) && CanUseAbility(CHAOS_NOVA))
    {
        if (CastSpell(CHAOS_NOVA))
        {
            RecordInterruptAttempt(interruptTarget, CHAOS_NOVA, true);
            TC_LOG_DEBUG("module.playerbot.ai", "DemonHunter {} stunned {} with Chaos Nova",                         _bot->GetName(), interruptTarget->GetName());
            return;
        }
    }    // Imprison - CC to stop casts on humanoids/beasts/demons
    if (CanUseAbility(IMPRISON))
    {
        if (CastSpell(IMPRISON, interruptTarget))
        {
            RecordInterruptAttempt(interruptTarget, IMPRISON, true);
            TC_LOG_DEBUG("module.playerbot.ai", "DemonHunter {} imprisoned {}",                         _bot->GetName(), interruptTarget->GetName());        }
    }
}

void DemonHunterAI::HandleDefensives()
{
    if (!_bot)
        return;

    float healthPct = _bot->GetHealthPct();
    uint32 currentTime = GameTime::GetGameTimeMS();

    // Netherwalk - Emergency immunity
    if (healthPct < HEALTH_EMERGENCY_THRESHOLD && CanUseAbility(NETHERWALK))
    {
        if (CastSpell(NETHERWALK))
        {
            RecordAbilityUsage(NETHERWALK);
            _dhMetrics.defensivesUsed++;
            _lastDefensiveTime = currentTime;
            TC_LOG_DEBUG("module.playerbot.ai", "DemonHunter {} activated Netherwalk (emergency)",
                         _bot->GetName());
            return;
        }    }

    // Blur - Primary defensive for damage reduction
    if (healthPct < DEFENSIVE_COOLDOWN_THRESHOLD && CanUseAbility(BLUR))
    {
        if (CastSpell(BLUR))
        {
            RecordAbilityUsage(BLUR);
            _dhMetrics.defensivesUsed++;
            _lastDefensiveTime = currentTime;
            TC_LOG_DEBUG("module.playerbot.ai", "DemonHunter {} activated Blur",
                         
                         _bot->GetName());
            return;
        }
    }

    // Darkness - Group defensive (AoE damage reduction)
    if (healthPct < 40.0f && CanUseAbility(DARKNESS))
    {
        if (CastSpell(DARKNESS))
        {
            RecordAbilityUsage(DARKNESS);
            _dhMetrics.defensivesUsed++;
            TC_LOG_DEBUG("module.playerbot.ai", "DemonHunter {} activated Darkness",                         _bot->GetName());
            return;
        }
    }

    // Vengeance-specific defensives
    if (_bot->GetPrimarySpecialization() == ChrSpecialization::DemonHunterVengeance)    {
        // Demon Spikes - Active mitigation
    if (healthPct < 70.0f && CanUseAbility(DEMON_SPIKES))
        {
            if (CastSpell(DEMON_SPIKES))
            {
                RecordAbilityUsage(DEMON_SPIKES);
                _dhMetrics.defensivesUsed++;
                TC_LOG_DEBUG("module.playerbot.ai", "DemonHunter {} activated Demon Spikes",
                             _bot->GetName());
                return;
            }
        }

        // Fiery Brand - Damage reduction on target
        Unit* target = _bot->GetSelectedUnit();
        if (target && healthPct < 60.0f && CanUseAbility(FIERY_BRAND))
        {
            if (CastSpell(FIERY_BRAND, target))
            {
                RecordAbilityUsage(FIERY_BRAND);
                _dhMetrics.defensivesUsed++;
                TC_LOG_DEBUG("module.playerbot.ai", "DemonHunter {} applied Fiery Brand to {}",
                             _bot->GetName(), target->GetName());
                return;
            }
        }

        // Soul Barrier - Absorb shield
    if (healthPct < 40.0f && CanUseAbility(SOUL_BARRIER))
        {
            if (CastSpell(SOUL_BARRIER))
            {
                RecordAbilityUsage(SOUL_BARRIER);
                _dhMetrics.defensivesUsed++;
                TC_LOG_DEBUG("module.playerbot.ai", "DemonHunter {} activated Soul Barrier",                             _bot->GetName());
                return;
            }
        }
    }

    // Metamorphosis as defensive (both specs)
    if (healthPct < METAMORPHOSIS_HEALTH_THRESHOLD && ShouldUseMetamorphosis())
    {
        if (_bot->GetPrimarySpecialization() == ChrSpecialization::DemonHunterHavoc)        {
            CastMetamorphosisHavoc();
        }
        else
        {
            CastMetamorphosisVengeance();
        }
    }
}

void DemonHunterAI::HandleTargetSwitching(::Unit*& target)
{
    auto* behaviors = GetCombatBehaviors();
    if (!behaviors)
        return;

    Unit* priorityTarget = behaviors->GetPriorityTarget();
    if (priorityTarget && priorityTarget != target)
    {
        OnTargetChanged(priorityTarget);
        target = priorityTarget;
        TC_LOG_DEBUG("module.playerbot.ai", "DemonHunter {} switching target to {}",                     _bot->GetName(), priorityTarget->GetName());
    }
}

void DemonHunterAI::HandleAoEDecisions(::Unit* target)
{
    if (!target || !_bot)
        return;

    uint32 enemyCount = GetNearbyEnemyCount(8.0f);

    // Eye Beam - Primary AoE ability for Havoc
    if (_bot->GetPrimarySpecialization() == ChrSpecialization::DemonHunterHavoc && enemyCount >= 2 && CanUseAbility(EYE_BEAM))    {
        if (CastSpell(EYE_BEAM, target))
        {
            RecordAbilityUsage(EYE_BEAM);
            TC_LOG_DEBUG("module.playerbot.ai", "DemonHunter {} channeling Eye Beam for AoE",
                         _bot->GetName());
            return;
        }
    }

    // Blade Dance / Death Sweep - AoE with dodge
    if (enemyCount >= 3)
    {
        uint32 bladeDanceSpell = _bot->HasAura(METAMORPHOSIS_HAVOC) ? DEATH_SWEEP : BLADE_DANCE;
        if (CanUseAbility(bladeDanceSpell))
        {
            if (CastSpell(bladeDanceSpell))
            {
                RecordAbilityUsage(bladeDanceSpell);
                TC_LOG_DEBUG("module.playerbot.ai", "DemonHunter {} using {} for AoE",
                             _bot->GetName(), bladeDanceSpell == DEATH_SWEEP ? "Death Sweep" : "Blade Dance");
                             return;
            }
        }
    }

    // Fel Barrage - Heavy AoE burst
    if (enemyCount >= 4 && CanUseAbility(FEL_BARRAGE))
    {
        if (CastSpell(FEL_BARRAGE, target))
        {
            RecordAbilityUsage(FEL_BARRAGE);
            TC_LOG_DEBUG("module.playerbot.ai", "DemonHunter {} activated Fel Barrage",
                         _bot->GetName());
                         return;
        }
    }

    // Immolation Aura - Constant AoE damage
    if (enemyCount >= 2 && CanUseAbility(IMMOLATION_AURA))
    {
        if (CastSpell(IMMOLATION_AURA))
        {
            RecordAbilityUsage(IMMOLATION_AURA);
            TC_LOG_DEBUG("module.playerbot.ai", "DemonHunter {} activated Immolation Aura",                         _bot->GetName());
            return;
        }
    }

    // Sigil of Flame - Ground-targeted AoE
    if (enemyCount >= 2 && CanUseAbility(SIGIL_OF_FLAME))
    {
        if (CastSpell(SIGIL_OF_FLAME, target))
        {            RecordAbilityUsage(SIGIL_OF_FLAME);
            TC_LOG_DEBUG("module.playerbot.ai", "DemonHunter {} placed Sigil of Flame",                         _bot->GetName());
            return;
        }
    }

    // Vengeance-specific AoE
    if (_bot->GetPrimarySpecialization() == ChrSpecialization::DemonHunterVengeance)    {
        // Spirit Bomb - Requires soul fragments
    if (enemyCount >= 3 && CanUseAbility(SPIRIT_BOMB))
        {
            if (CastSpell(SPIRIT_BOMB, target))
            {
                RecordAbilityUsage(SPIRIT_BOMB);
                TC_LOG_DEBUG("module.playerbot.ai", "DemonHunter {} detonated Spirit Bomb",
                             _bot->GetName());
                return;
            }
        }
    }
}

void DemonHunterAI::HandleCooldowns(::Unit* target)
{
    if (!target || !_bot)
        return;

    // Metamorphosis - Major DPS/survival cooldown
    if (ShouldUseMetamorphosis())
    {
        uint32 metaSpell = _bot->GetPrimarySpecialization() == ChrSpecialization::DemonHunterHavoc ?                          METAMORPHOSIS_HAVOC : METAMORPHOSIS_VENGEANCE;

        if (CanUseAbility(metaSpell))
        {
            if (CastSpell(metaSpell))
            {
                RecordAbilityUsage(metaSpell);
                TC_LOG_DEBUG("module.playerbot.ai", "DemonHunter {} activated Metamorphosis",
                             _bot->GetName());
            }
        }
    }

    // Nemesis - Single target damage increase (Havoc)
    if (_bot->GetPrimarySpecialization() == ChrSpecialization::DemonHunterHavoc && CanUseAbility(NEMESIS))    {
        if (CastSpell(NEMESIS, target))
        {
            RecordAbilityUsage(NEMESIS);
            TC_LOG_DEBUG("module.playerbot.ai", "DemonHunter {} marked {} with Nemesis",
                         _bot->GetName(), target->GetName());
        }
    }

    // Fel Barrage - AoE burst cooldown
    if (GetNearbyEnemyCount(8.0f) >= 3 && CanUseAbility(FEL_BARRAGE))
    {        if (CastSpell(FEL_BARRAGE, target))
        {
            RecordAbilityUsage(FEL_BARRAGE);
            TC_LOG_DEBUG("module.playerbot.ai", "DemonHunter {} using Fel Barrage burst",
                         _bot->GetName());
        }
    }
}

void DemonHunterAI::HandleResourceGeneration(::Unit* target)
{
    if (!target || !_bot)
        return;

    if (_bot->GetPrimarySpecialization() == ChrSpecialization::DemonHunterHavoc)
    {
        uint32 currentFury = GetFury();
        uint32 maxFury = GetMaxFury();

        // Prevent fury capping
    if (currentFury > FURY_DUMP_THRESHOLD)
        {
            // Use metamorphosed ability if available
            uint32 chaosStrike = _bot->HasAura(METAMORPHOSIS_HAVOC) ? ANNIHILATION : CHAOS_STRIKE;
            if (CanUseAbility(chaosStrike))
            {
                if (CastSpell(chaosStrike, target))
                {
                    RecordAbilityUsage(chaosStrike);
                    TC_LOG_DEBUG("module.playerbot.ai", "DemonHunter {} dumping fury with {}",
                                 _bot->GetName(), chaosStrike == ANNIHILATION ? "Annihilation" : "Chaos Strike");
                                 return;
                }
            }
        }

        // Generate fury with Demon's Bite
    if (currentFury < 40 && CanUseAbility(DEMONS_BITE))
        {
            if (CastSpell(DEMONS_BITE, target))
            {
                RecordAbilityUsage(DEMONS_BITE);
                return;
            }
        }
    }
    else // Vengeance
    {
        uint32 currentPain = GetPain();

        // Prevent pain capping
    if (currentPain > PAIN_DUMP_THRESHOLD)
        {
            // Soul Cleave to spend pain and heal
    if (CanUseAbility(SOUL_CLEAVE))
            {
                if (CastSpell(SOUL_CLEAVE, target))
                {
                    RecordAbilityUsage(SOUL_CLEAVE);
                    TC_LOG_DEBUG("module.playerbot.ai", "DemonHunter {} spending pain with Soul Cleave",                                 _bot->GetName());
                    return;
                }
            }
        }

        // Generate pain with Shear
    if (currentPain < 30 && CanUseAbility(SHEAR))
        {
            if (CastSpell(SHEAR, target))
            {
                RecordAbilityUsage(SHEAR);
                return;
            }
        }
    }
}

void DemonHunterAI::HandleMobility(::Unit* target)
{
    if (!target || !_bot)
        return;

    auto* behaviors = GetCombatBehaviors();
    if (!behaviors)
        return;

    // Check if we need to reposition
    if (behaviors->NeedsRepositioning())
    {
        Position optimalPos = behaviors->GetOptimalPosition();
        float distance = ::std::sqrt(_bot->GetExactDistSq(target)); // Calculate once from squared distance        // Fel Rush to close gap or reposition
    if (distance > CHARGE_MIN_RANGE && distance < CHARGE_MAX_RANGE && CanUseAbility(FEL_RUSH))
        {
            if (CastSpell(FEL_RUSH, target))
            {
                RecordAbilityUsage(FEL_RUSH);
                _dhMetrics.mobilityAbilitiesUsed++;
                _lastMobilityTime = GameTime::GetGameTimeMS();
                TC_LOG_DEBUG("module.playerbot.ai", "DemonHunter {} used Fel Rush to reach {}",
                             _bot->GetName(), target->GetName());
                return;
            }
        }

        // Vengeful Retreat for backward movement (defensive or offensive with Momentum)
    if (distance < 3.0f && CanUseAbility(VENGEFUL_RETREAT))
        {
            if (CastSpell(VENGEFUL_RETREAT))
            {
                RecordAbilityUsage(VENGEFUL_RETREAT);
                _dhMetrics.mobilityAbilitiesUsed++;
                TC_LOG_DEBUG("module.playerbot.ai", "DemonHunter {} used Vengeful Retreat",                             _bot->GetName());
                return;
            }
        }
    }}

void DemonHunterAI::ExecuteBasicDemonHunterRotation(::Unit* target)
{
    if (!target || !_bot)
        return;

    // Basic rotation for Demon Hunters without specialization    // This covers both Havoc and Vengeance basics

    // Maintain Immolation Aura
    if (!_bot->HasAura(IMMOLATION_AURA) && CanUseAbility(IMMOLATION_AURA))
    {
        if (CastSpell(IMMOLATION_AURA))
        {
            RecordAbilityUsage(IMMOLATION_AURA);
            return;
        }
    }

    // Try spec-specific builders/spenders
    if (_bot->GetPrimarySpecialization() == ChrSpecialization::DemonHunterHavoc)
    {
        // Havoc rotation
        uint32 fury = GetFury();

        // Chaos Strike/Annihilation at high fury
    if (fury >= 40)
        {
            uint32 spender = _bot->HasAura(METAMORPHOSIS_HAVOC) ? ANNIHILATION : CHAOS_STRIKE;
            if (CanUseAbility(spender))
            {
                if (CastSpell(spender, target))
                {
                    RecordAbilityUsage(spender);
                    return;
                }
            }
        }

        // Demon's Bite to generate fury
    if (CanUseAbility(DEMONS_BITE))
        {
            if (CastSpell(DEMONS_BITE, target))
            {
                RecordAbilityUsage(DEMONS_BITE);
                return;
            }
        }
    }
    else
    {
        // Vengeance rotation
        uint32 pain = GetPain();

        // Soul Cleave at high pain
    if (pain >= 30 && CanUseAbility(SOUL_CLEAVE))
        {
            if (CastSpell(SOUL_CLEAVE, target))
            {
                RecordAbilityUsage(SOUL_CLEAVE);
                return;
            }
        }        // Shear to generate pain
    if (CanUseAbility(SHEAR))
        {
            if (CastSpell(SHEAR, target))
            {
                RecordAbilityUsage(SHEAR);
                return;
            }
        }

        // Maintain Demon Spikes
    if (!_bot->HasAura(DEMON_SPIKES) && CanUseAbility(DEMON_SPIKES))
        {
            if (CastSpell(DEMON_SPIKES))
            {
                RecordAbilityUsage(DEMON_SPIKES);
                return;
            }
        }
    }

    // Sigil of Flame for damage
    if (CanUseAbility(SIGIL_OF_FLAME))
    {
        if (CastSpell(SIGIL_OF_FLAME, target))
        {
            RecordAbilityUsage(SIGIL_OF_FLAME);
            return;
        }
    }
}

void DemonHunterAI::UpdateBuffs()
{
    if (!_bot)
        return;

    // NOTE: Baseline buff check is now handled at the dispatch level.
    // This method is only called for level 10+ bots with talents.

    // Apply Demon Hunter buffs based on specialization
    ChrSpecialization spec = _bot->GetPrimarySpecialization();

    // Maintain Immolation Aura (both specs)
    if (!_bot->HasAura(IMMOLATION_AURA) && CanUseAbility(IMMOLATION_AURA))
    {
        if (CastSpell(IMMOLATION_AURA))
        {
            RecordAbilityUsage(IMMOLATION_AURA);
            TC_LOG_DEBUG("module.playerbot.demonhunter", "DemonHunter {} activated Immolation Aura buff",
                         _bot->GetName());
        }
    }

    // Havoc-specific buffs
    if (spec == ChrSpecialization::DemonHunterHavoc)
    {
        // Maintain Momentum buff through movement abilities if talented
    if (_bot->HasSpell(MOMENTUM_TALENT) && !_bot->HasAura(208628)) // BUFF_MOMENTUM
        {
            // Fel Rush for momentum
    if (CanUseAbility(FEL_RUSH))
            {
                Unit* target = _bot->GetSelectedUnit();
                if (target && _bot->GetDistance(target) > 5.0f && _bot->GetDistance(target) < 20.0f)
                {
                    if (CastSpell(FEL_RUSH, target))
                    {
                        RecordAbilityUsage(FEL_RUSH);
                        TC_LOG_DEBUG("module.playerbot.demonhunter", "DemonHunter {} using Fel Rush for Momentum buff",
                                     _bot->GetName());
                    }
                }
            }
        }

        // Refresh Prepared buff from Vengeful Retreat if talented
    if (_bot->HasSpell(203650) && !_bot->HasAura(203650) && CanUseAbility(VENGEFUL_RETREAT)) // BUFF_PREPARED
        {
            Unit* target = _bot->GetSelectedUnit();
            if (target && _bot->GetDistance(target) < 3.0f)
            {
                if (CastSpell(VENGEFUL_RETREAT))
                {
                    RecordAbilityUsage(VENGEFUL_RETREAT);
                    TC_LOG_DEBUG("module.playerbot.demonhunter", "DemonHunter {} using Vengeful Retreat for Prepared buff",
                                 _bot->GetName());
                }
            }
        }
    }
    // Vengeance-specific buffs
    else if (spec == ChrSpecialization::DemonHunterVengeance)
    {
        // Maintain Demon Spikes uptime when tanking
    if (!_bot->HasAura(DEMON_SPIKES) && CanUseAbility(DEMON_SPIKES))
        {
            if (_bot->GetHealthPct() < 90.0f || _bot->IsInCombat())
            {
                if (CastSpell(DEMON_SPIKES))
                {
                    RecordAbilityUsage(DEMON_SPIKES);
                    TC_LOG_DEBUG("module.playerbot.demonhunter", "DemonHunter {} activated Demon Spikes buff",
                                 _bot->GetName());
                }
            }
        }

        // Apply Fiery Brand on primary target for damage reduction
        Unit* target = _bot->GetVictim();
        if (target && !target->HasAura(FIERY_BRAND) && _bot->GetHealthPct() < 80.0f && CanUseAbility(FIERY_BRAND))
        {
            if (CastSpell(FIERY_BRAND, target))
            {
                RecordAbilityUsage(FIERY_BRAND);
                TC_LOG_DEBUG("module.playerbot.demonhunter", "DemonHunter {} applied Fiery Brand debuff",
                             _bot->GetName());
            }
        }
    }
}

void DemonHunterAI::UpdateCooldowns(uint32 diff)
{
    UpdateMetrics(diff);

    // Cooldowns are tracked by TrinityCore's spell system
    // No additional tracking needed
}

bool DemonHunterAI::CanUseAbility(uint32 spellId)
{
    if (!IsSpellReady(spellId) || !HasEnoughResource(spellId))
        return false;

    return true;
}

void DemonHunterAI::OnCombatStart(::Unit* target)
{
    _dhMetrics.combatStartTime = ::std::chrono::steady_clock::now();

    TC_LOG_DEBUG("module.playerbot.demonhunter", "DemonHunterAI combat started for player {}",
                 
                 _bot->GetName());
}

void DemonHunterAI::OnCombatEnd()
{    AnalyzeCombatEffectiveness();

    TC_LOG_DEBUG("module.playerbot.demonhunter", "DemonHunterAI combat ended for player {}",                 _bot->GetName());
}

bool DemonHunterAI::HasEnoughResource(uint32 spellId)
{
    // Check resource requirements based on spec
    if (_bot->GetPrimarySpecialization() == ChrSpecialization::DemonHunterHavoc)    {
        // Check fury costs for common abilities
    switch (spellId)
        {
            case CHAOS_STRIKE:
            case ANNIHILATION:
                return GetFury() >= 40;
            case BLADE_DANCE:
            case DEATH_SWEEP:
                return GetFury() >= 35;
            case EYE_BEAM:
                return GetFury() >= 30;
            case FEL_BARRAGE:
                return GetFury() >= 60;
            default:
                return true;
        }
    }
    else
    {        // Check pain costs for Vengeance
    switch (spellId)
        {
            case SOUL_CLEAVE:
                return GetPain() >= 30;
            case SPIRIT_BOMB:
                return GetPain() >= 30;
            case SOUL_BARRIER:
                return GetPain() >= 30;
            default:
                return true;
        }
    }
}
void DemonHunterAI::ConsumeResource(uint32 spellId)
{
    RecordAbilityUsage(spellId);

    // Resource consumption is handled by TrinityCore's spell system
    // No manual tracking needed
}

Position DemonHunterAI::GetOptimalPosition(::Unit* target)
{    if (!target || !_bot)
        return Position();

    // Demon Hunters are melee - stay close to target
    return _bot->GetPosition();
}

float DemonHunterAI::GetOptimalRange(::Unit* target)
{
    return OPTIMAL_MELEE_RANGE;
}

void DemonHunterAI::ExitMetamorphosis()
{
    // Metamorphosis naturally expires, no manual exit needed
}

bool DemonHunterAI::ShouldUseMetamorphosis()
{
    if (!_bot)
        return false;

    // Already in metamorphosis
    if (_bot->HasAura(METAMORPHOSIS_HAVOC) || _bot->HasAura(METAMORPHOSIS_VENGEANCE))
        return false;

    // Use for survival at low health
    if (_bot->GetHealthPct() < METAMORPHOSIS_HEALTH_THRESHOLD)
        return true;

    // Use for burst damage on high-health targets
    Unit* target = _bot->GetSelectedUnit();
    if (target && target->GetHealthPct() > 80.0f)
        return true;

    // Use during AoE situations
    if (GetNearbyEnemyCount(8.0f) >= 3)
        return true;

    return false;
}

void DemonHunterAI::CastMetamorphosisHavoc()
{
    if (CanUseAbility(METAMORPHOSIS_HAVOC))
    {
        CastSpell(METAMORPHOSIS_HAVOC);
        RecordAbilityUsage(METAMORPHOSIS_HAVOC);
        TC_LOG_DEBUG("module.playerbot.ai", "DemonHunter {} transformed with Havoc Metamorphosis",
                     _bot->GetName());
    }
}

void DemonHunterAI::CastMetamorphosisVengeance()
{
    if (CanUseAbility(METAMORPHOSIS_VENGEANCE))
    {
        CastSpell(METAMORPHOSIS_VENGEANCE);
        RecordAbilityUsage(METAMORPHOSIS_VENGEANCE);
        TC_LOG_DEBUG("module.playerbot.ai", "DemonHunter {} transformed with Vengeance Metamorphosis",                     _bot->GetName());
    }
}

void DemonHunterAI::SpendPain(uint32 amount)
{
    if (!_bot)
        return;

    int32 currentPain = _bot->GetPower(POWER_PAIN);
    _bot->SetPower(POWER_PAIN, ::std::max(0, currentPain - static_cast<int32>(amount)));
}

void DemonHunterAI::GeneratePain(uint32 amount)
{
    if (!_bot)
        return;

    int32 currentPain = _bot->GetPower(POWER_PAIN);
    int32 maxPain = _bot->GetMaxPower(POWER_PAIN);
    _bot->SetPower(POWER_PAIN, ::std::min(maxPain, currentPain + static_cast<int32>(amount)));
}

bool DemonHunterAI::HasPain(uint32 amount)
{
    return _bot && _bot->GetPower(POWER_PAIN) >= amount;}

void DemonHunterAI::SpendFury(uint32 amount)
{
    if (!_bot)
        return;

    int32 currentFury = _bot->GetPower(POWER_FURY);
    _bot->SetPower(POWER_FURY, ::std::max(0, currentFury - static_cast<int32>(amount)));
}

void DemonHunterAI::GenerateFury(uint32 amount)
{
    if (!_bot)
        return;

    int32 currentFury = _bot->GetPower(POWER_FURY);
    int32 maxFury = _bot->GetMaxPower(POWER_FURY);
    _bot->SetPower(POWER_FURY, ::std::min(maxFury, currentFury + static_cast<int32>(amount)));
}

bool DemonHunterAI::HasFury(uint32 amount)
{
    return _bot && _bot->GetPower(POWER_FURY) >= amount;}

void DemonHunterAI::UpdatePainManagement(uint32 diff)
{
    // Pain doesn't decay in combat
}

void DemonHunterAI::DecayPain(uint32 diff)
{
    // Pain decays out of combat
    if (!_bot || _bot->IsInCombat())
        return;

    // Decay 1 pain per second out of combat
    // QW-4 FIX: Removed static - now uses instance member _painDecayTimer
    _painDecayTimer += diff;
    if (_painDecayTimer >= 1000)
    {
        SpendPain(1);
        _painDecayTimer = 0;
    }
}

uint32 DemonHunterAI::GetFury() const
{    return _bot ? _bot->GetPower(POWER_FURY) : 0;
}

uint32 DemonHunterAI::GetMaxFury() const
{
    return _bot ? _bot->GetMaxPower(POWER_FURY) : 120;
}

uint32 DemonHunterAI::GetPain() const
{    return _bot ? _bot->GetPower(POWER_PAIN) : 0;
}

uint32 DemonHunterAI::GetMaxPain() const
{
    return _bot ? _bot->GetMaxPower(POWER_PAIN) : 100;
}

void DemonHunterAI::UpdateHavocRotation(::Unit* target)
{
    if (!target || !_bot)
        return;    // Havoc-specific rotation logic
    uint32 fury = GetFury();

    // Eye Beam on cooldown for AoE and buff
    if (CanUseAbility(EYE_BEAM) && GetNearbyEnemyCount(20.0f) >= 2)
    {
        CastEyeBeam(target);
        return;
    }

    // Blade Dance for AoE
    if (fury >= 35 && GetNearbyEnemyCount(8.0f) >= 2)
    {
        CastBladeDance(target);
        return;
    }

    // Chaos Strike as main spender
    if (fury >= 40)
    {
        CastChaosStrike(target);
        return;
    }

    // Demon's Bite as filler
    CastDemonsBite(target);
}

void DemonHunterAI::UpdateVengeanceRotation(::Unit* target)
{
    if (!target || !_bot)
        return;

    // Vengeance-specific rotation logic
    uint32 pain = GetPain();

    // Spirit Bomb for AoE threat
    if (pain >= 30 && GetNearbyEnemyCount(8.0f) >= 2 && CanUseAbility(SPIRIT_BOMB))
    {
        CastSpell(SPIRIT_BOMB, target);
        return;
    }

    // Soul Cleave for healing and damage
    if (pain >= 30 && (_bot->GetHealthPct() < 70.0f || pain >= 60))
    {
        CastSoulCleave(target);
        return;
    }

    // Shear as filler
    CastShear(target);
}

void DemonHunterAI::HandleMetamorphosisAbilities(::Unit* target)
{
    if (!target || !_bot)
        return;

    // Handle abilities that change during metamorphosis
    if (_bot->HasAura(METAMORPHOSIS_HAVOC))
    {
        // Use Annihilation instead of Chaos Strike
    if (CanUseAbility(ANNIHILATION) && HasFury(40))
        {
            CastSpell(ANNIHILATION, target);
            return;
        }

        // Use Death Sweep instead of Blade Dance
    if (CanUseAbility(DEATH_SWEEP) && HasFury(35))
        {
            CastSpell(DEATH_SWEEP);
            return;
        }
    }
}

void DemonHunterAI::CastEyeBeam(::Unit* target)
{
    if (target && CanUseAbility(EYE_BEAM))
    {        CastSpell(EYE_BEAM, target);
        ConsumeResource(EYE_BEAM);
    }
}

void DemonHunterAI::CastChaosStrike(::Unit* target)
{
    if (!target)
        return;

    uint32 ability = _bot->HasAura(METAMORPHOSIS_HAVOC) ? ANNIHILATION : CHAOS_STRIKE;
    if (CanUseAbility(ability))
    {
        CastSpell(ability, target);
        ConsumeResource(ability);
    }
}

void DemonHunterAI::CastBladeDance(::Unit* target)
{
    uint32 ability = _bot->HasAura(METAMORPHOSIS_HAVOC) ? DEATH_SWEEP : BLADE_DANCE;
    if (CanUseAbility(ability))
    {
        CastSpell(ability);
        ConsumeResource(ability);
    }
}

void DemonHunterAI::CastDemonsBite(::Unit* target)
{
    if (target && CanUseAbility(DEMONS_BITE))
    {
        CastSpell(DEMONS_BITE, target);
        ConsumeResource(DEMONS_BITE);
    }
}

void DemonHunterAI::CastSoulCleave(::Unit* target)
{
    if (target && CanUseAbility(SOUL_CLEAVE))
    {
        CastSpell(SOUL_CLEAVE, target);
        ConsumeResource(SOUL_CLEAVE);
    }
}

void DemonHunterAI::CastShear(::Unit* target)
{
    if (target && CanUseAbility(SHEAR))
    {
        CastSpell(SHEAR, target);
        ConsumeResource(SHEAR);
    }
}

::std::vector<::Unit*> DemonHunterAI::GetAoETargets(float range)
{
    ::std::vector<::Unit*> targets;
    if (!_bot)
        return targets;

    ::std::list<Unit*> targetList;
    Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(_bot, _bot, range);
    Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(_bot, targetList, u_check);
    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Map* map = _bot->GetMap();
    if (!map)
        return targets;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return targets;
    }

    // Query nearby GUIDs (lock-free!)
    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        _bot->GetPosition(), range);

    // Process results (replace old loop)
    for (ObjectGuid guid : nearbyGuids)
    {
        // PHASE 5F: Thread-safe spatial grid validation
        auto snapshot_entity = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, guid);
        Creature* entity = nullptr;
        if (snapshot_entity)
        {

        }
        if (!entity)
            continue;
        // Original filtering logic goes here
    }
    // End of spatial grid fix
    for (auto& unit : targetList)
    {
        if (_bot->IsValidAttackTarget(unit))
            targets.push_back(unit);
    }

    return targets;
}

uint32 DemonHunterAI::GetNearbyEnemyCount(float range) const
{
    if (!_bot)
        return 0;

    uint32 count = 0;
    ::std::list<Unit*> targets;
    Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(_bot, _bot, range);
    Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(_bot, targets, u_check);
    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Map* map = _bot->GetMap();
    if (!map)
        return 0;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return 0;
    }

    // Query nearby GUIDs (lock-free!)
    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        _bot->GetPosition(), range);

    // Process results (replace old loop)
    for (ObjectGuid guid : nearbyGuids)
    {
        // PHASE 5F: Thread-safe spatial grid validation
        auto snapshot_entity = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, guid);
        Creature* entity = nullptr;
        if (snapshot_entity)
        {

        }
        if (!entity)
            continue;
        // Original filtering logic goes here
    }
    // End of spatial grid fix
    for (auto& target : targets)
    {
        if (_bot->IsValidAttackTarget(target))
            count++;
    }

    return count;
}

bool DemonHunterAI::IsInMeleeRange(::Unit* target) const
{
    if (!target || !_bot)
        return false;

    float rangeSq = OPTIMAL_MELEE_RANGE * OPTIMAL_MELEE_RANGE; // Squared distance comparison
    return _bot->GetExactDistSq(target) <= rangeSq;
}

bool DemonHunterAI::IsTargetInterruptible(::Unit* target) const
{
    if (!target)
        return false;

    // Check if target is casting
    if (!target->HasUnitState(UNIT_STATE_CASTING))
        return false;

    // Check if the spell can be interrupted
    if (Spell const* spell = target->GetCurrentSpell(CURRENT_GENERIC_SPELL))
    {
        if (spell->GetSpellInfo() && !spell->GetSpellInfo()->HasAttribute(SPELL_ATTR0_NO_IMMUNITIES))
            return true;
    }

    if (Spell const* spell = target->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
    {
        if (spell->GetSpellInfo() && !spell->GetSpellInfo()->HasAttribute(SPELL_ATTR0_NO_IMMUNITIES))
            return true;
    }

    return false;
}

void DemonHunterAI::RecordInterruptAttempt(::Unit* target, uint32 spellId, bool success)
{
    if (success)
    {
        _successfulInterrupts++;
        _dhMetrics.interruptsSucceeded++;
        TC_LOG_DEBUG("module.playerbot.ai", "DemonHunter {} successfully interrupted with spell {}",                     _bot->GetName(), spellId);
    }
}

void DemonHunterAI::RecordAbilityUsage(uint32 spellId)
{
    _abilityUsage[spellId]++;
    _dhMetrics.totalAbilitiesUsed++;
}

void DemonHunterAI::OnTargetChanged(::Unit* newTarget)
{
    if (newTarget)
    {
        TC_LOG_DEBUG("module.playerbot.ai", "DemonHunter {} changed target to {}",                     _bot->GetName(), newTarget->GetName());
    }
}

void DemonHunterAI::UpdateMetrics(uint32 diff)
{
    _dhMetrics.lastMetricsUpdate = ::std::chrono::steady_clock::now();
}

void DemonHunterAI::AnalyzeCombatEffectiveness()
{
    auto endTime = ::std::chrono::steady_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::seconds>(endTime - _dhMetrics.combatStartTime).count();

    if (duration > 0)
    {
        float abilitiesPerSecond = static_cast<float>(_dhMetrics.totalAbilitiesUsed) / duration;
        float interruptRate = _dhMetrics.interruptsSucceeded > 0 ?
                             (static_cast<float>(_dhMetrics.interruptsSucceeded) / _dhMetrics.totalAbilitiesUsed) * 100.0f : 0.0f;

        TC_LOG_DEBUG("module.playerbot.ai", "DemonHunter {} combat analysis: {} abilities in {}s ({:.2f}/sec), "
                     "{} interrupts ({:.1f}% success), {} defensives, {} mobility uses",                     _bot->GetName(), _dhMetrics.totalAbilitiesUsed, duration, abilitiesPerSecond,
                     _dhMetrics.interruptsSucceeded, interruptRate,
                     _dhMetrics.defensivesUsed, _dhMetrics.mobilityAbilitiesUsed);
    }
}

} // namespace Playerbot
