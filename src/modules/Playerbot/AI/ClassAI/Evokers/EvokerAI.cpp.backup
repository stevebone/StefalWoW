/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "EvokerAI.h"
#include "../../Combat/CombatBehaviorIntegration.h"
#include "Player.h"
#include "Spell.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "Unit.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "Map.h"
#include "Group.h"
#include "WorldSession.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Cell.h"
#include "CellImpl.h"
#include "../BaselineRotationManager.h"
#include "Log.h"
#include "../../../Spatial/SpatialGridManager.h"
#include "../../../Spatial/SpatialGridQueryHelpers.h"  // PHASE 5F: Thread-safe queries
#include "GameTime.h"

namespace Playerbot
{

EvokerAI::EvokerAI(Player* bot) : ClassAI(bot)
{
    _currentAspect = EvokerAspect::NONE;
    _damageDealt = 0;
    _healingDone = 0;
    _essenceGenerated = 0;
    _empoweredSpellsCast = 0;
    _echoHealsPerformed = 0;

    // Initialize essence management
    _essence = EssenceInfo();
    _lastEssenceUpdate = 0;
    _essenceUpdateInterval = ESSENCE_GENERATION_RATE;

    // Initialize empowerment system
    _currentEmpoweredSpell = EmpoweredSpell();
    _lastEmpoweredSpell = 0;
    _isChannelingEmpowered = false;

    // Initialize echo system
    _lastEchoUpdate = 0;
    _echoUpdateInterval = ECHO_HEAL_INTERVAL;
    _maxEchoes = ECHO_MAX_COUNT;

    // Initialize Devastation tracking
    _burnoutStacks = 0;
    _essenceBurstStacks = 0;
    _dragonrageStacks = 0;
    _lastEternity = 0;
    _lastDisintegrate = 0;
    _eternitysSurgeReady = true;

    // Initialize Preservation tracking
    _temporalCompressionStacks = 0;
    _callOfYseraStacks = 0;
    _lastVerdantEmbrace = 0;
    _lastTemporalAnomaly = 0;

    // Initialize Augmentation tracking
    _prescientStacks = 0;
    _blisteryScalesStacks = 0;
    _lastEbon = 0;
    _lastBreathOfEons = 0;

    // Initialize aspect management
    _aspectDuration = 0;
    _lastAspectShift = 0;
    _aspectCooldown = ASPECT_SHIFT_COOLDOWN;
    _canShiftAspect = true;

    // Initialize utility tracking
    _lastSoar = 0;
    _lastHover = 0;
    _lastRescue = 0;
    _lastTimeSpiral = 0;
    _hoverRemaining = 0;
    _isHovering = false;
}

void EvokerAI::UpdateRotation(::Unit* target)
{
    if (!target || !_bot)
        return;

    // Check if bot should use baseline rotation (levels 1-9 or no spec)
    if (BaselineRotationManager::ShouldUseBaselineRotation(_bot))
    {
        static BaselineRotationManager baselineManager;
        baselineManager.HandleAutoSpecialization(_bot);

        if (baselineManager.ExecuteBaselineRotation(_bot, target))
            return;

        // Fallback: basic ranged attack
    if (!_bot->IsNonMeleeSpellCast(false))
        {
            float rangeSq = 35.0f * 35.0f; // 1225.0f
    if (_bot->GetExactDistSq(target) <= rangeSq)
            {
                _bot->AttackerStateUpdate(target);
            }
        }
        return;
    }

    UpdateEssenceManagement(target);  // DEADLOCK FIX: Pass target parameter
    UpdateEmpowermentSystem();
    UpdateAspectManagement();

    // **CombatBehaviorIntegration - 10-Priority System**
    auto* behaviors = GetCombatBehaviors();

    // Priority 1: Interrupts - Quell (Evoker's interrupt)
    if (behaviors && behaviors->ShouldInterrupt(target))
    {
        ::Unit* interruptTarget = behaviors->GetInterruptTarget();
        if (interruptTarget && CanUseAbility(SPELL_QUELL))
        {
            if (CastSpell(SPELL_QUELL, interruptTarget))
            {
                TC_LOG_DEBUG("module.playerbot.ai", "Evoker {} interrupted {} with Quell",                             _bot->GetName(), interruptTarget->GetName());
                return;
            }
        }
    }

    // Priority 2: Defensives - Obsidian Scales, Renewing Blaze
    if (behaviors && behaviors->NeedsDefensive())
    {
        float healthPct = _bot->GetHealthPct();
        if (healthPct < 30.0f && CanUseAbility(OBSIDIAN_SCALES))
        {
            if (CastSpell(OBSIDIAN_SCALES, _bot))
            {
                TC_LOG_DEBUG("module.playerbot.ai", "Evoker {} used Obsidian Scales at {}% health",
                             _bot->GetName(), healthPct);
                return;
            }
        }

        if (healthPct < 50.0f && CanUseAbility(RENEWING_BLAZE))
        {
            if (CastSpell(RENEWING_BLAZE, _bot))
            {
                TC_LOG_DEBUG("module.playerbot.ai", "Evoker {} used Renewing Blaze at {}% health",
                             _bot->GetName(), healthPct);
                return;
            }
        }

        // Verdant Embrace for Preservation - teleport to ally and heal
        EvokerSpec currentSpec = DetectSpecialization();
        if (currentSpec == EvokerSpec::PRESERVATION && healthPct < 40.0f && CanUseAbility(VERDANT_EMBRACE))
        {
            ::Unit* healTarget = GetLowestHealthAlly();
            if (healTarget && CastSpell(VERDANT_EMBRACE, healTarget))
            {
                TC_LOG_DEBUG("module.playerbot.ai", "Evoker {} used Verdant Embrace to escape",
                             
                             _bot->GetName());
                return;
            }
        }
    }

    // Priority 3: Positioning - Maintain mid-range (20-25 yards for empowered spells)
    if (behaviors && behaviors->NeedsRepositioning())
    {        Position optimalPos = behaviors->GetOptimalPosition();
        float distance = ::std::sqrt(_bot->GetExactDistSq(target)); // Calculate once from squared distance        // Too close - use Hover to gain distance
    if (distance < 15.0f && CanUseAbility(HOVER))
        {
            if (CastSpell(HOVER, _bot))
            {
                TC_LOG_DEBUG("module.playerbot.ai", "Evoker {} using Hover to reposition",
                             _bot->GetName());
                return;
            }
        }

        // Wing Buffet for knockback
    if (distance < 10.0f && CanUseAbility(WING_BUFFET))
        {
            if (CastSpell(WING_BUFFET, _bot))
            {
                TC_LOG_DEBUG("module.playerbot.ai", "Evoker {} using Wing Buffet for space",                             _bot->GetName());
                return;
            }
        }
    }

    // Priority 4: Target Switching - Switch to priority targets
    if (behaviors && behaviors->ShouldSwitchTarget())
    {
        ::Unit* priorityTarget = behaviors->GetPriorityTarget();
        if (priorityTarget && priorityTarget != target)
        {
            OnTargetChanged(priorityTarget);
            target = priorityTarget;
            TC_LOG_DEBUG("module.playerbot.ai", "Evoker {} switching to priority target {}",
                         
                         _bot->GetName(), priorityTarget->GetName());
        }
    }

    // Priority 5: Crowd Control - Sleep Walk, Landslide
    if (behaviors && behaviors->ShouldUseCrowdControl())
    {
        ::Unit* ccTarget = behaviors->GetCrowdControlTarget();
        if (ccTarget && ccTarget != target)        {
            if (CanUseAbility(SLEEP_WALK))
            {
                if (CastSpell(SLEEP_WALK, ccTarget))
                {
                    TC_LOG_DEBUG("module.playerbot.ai", "Evoker {} Sleep Walking secondary target",                                 _bot->GetName());
                    return;
                }
            }
        }
    }

    // Priority 6: AoE Decisions - Pyre, Eternity's Surge based on enemy count
    if (behaviors && behaviors->ShouldAOE())
    {        EvokerSpec currentSpec = DetectSpecialization();
        if (currentSpec == EvokerSpec::DEVASTATION)
        {
            // Pyre for AoE with Essence Burst proc
    if (_essenceBurstStacks > 0 && CanUseAbility(PYRE))
            {
                if (CastSpell(PYRE, target))
                {
                    TC_LOG_DEBUG("module.playerbot.ai", "Evoker {} using Pyre for AoE",                                 _bot->GetName());
                    return;
                }
            }            // Eternity's Surge (empowered) for AoE burst
    if (_essence.current >= 3 && CanUseAbility(ETERNITYS_SURGE))
            {
                StartEmpoweredSpell(ETERNITYS_SURGE, EmpowermentLevel::RANK_3, target);
                TC_LOG_DEBUG("module.playerbot.ai", "Evoker {} channeling Eternity's Surge (Rank 3) for AoE",                             _bot->GetName());
                return;
            }
        }        else if (currentSpec == EvokerSpec::PRESERVATION)
        {
            // Dream Breath (empowered) for AoE healing
    if (_essence.current >= 3 && CanUseAbility(DREAM_BREATH))
            {
                StartEmpoweredSpell(DREAM_BREATH, EmpowermentLevel::RANK_3, target);
                TC_LOG_DEBUG("module.playerbot.ai", "Evoker {} channeling Dream Breath (Rank 3) for AoE healing",                             _bot->GetName());
                return;
            }

            // Emerald Blossom for instant AoE heal
    if (CanUseAbility(EMERALD_BLOSSOM))
            {
                if (CastSpell(EMERALD_BLOSSOM, _bot))
                {
                    TC_LOG_DEBUG("module.playerbot.ai", "Evoker {} using Emerald Blossom for AoE healing",                                 _bot->GetName());
                    return;
                }
            }
        }
    }

    // Priority 7: Offensive Cooldowns - Dragonrage, Tip the Scales
    if (behaviors && behaviors->ShouldUseCooldowns())
    {
        EvokerSpec currentSpec = DetectSpecialization();
        if (currentSpec == EvokerSpec::DEVASTATION)
        {
            // Dragonrage - major DPS cooldown
    if (CanUseAbility(SPELL_DRAGONRAGE))
            {
                if (CastSpell(SPELL_DRAGONRAGE, _bot))
                {
                    TC_LOG_DEBUG("module.playerbot.ai", "Evoker {} activating Dragonrage",                                 _bot->GetName());
                    _dragonrageStacks = 40; // Starts at 40 stacks
                    return;
                }
            }
        }
        else if (currentSpec == EvokerSpec::PRESERVATION)
        {
            // Emerald Communion - major healing cooldown
    if (CanUseAbility(EMERALD_COMMUNION))
            {
                if (CastSpell(EMERALD_COMMUNION, _bot))
                {
                    TC_LOG_DEBUG("module.playerbot.ai", "Evoker {} activating Emerald Communion",                                 _bot->GetName());
                    return;
                }
            }
        }
    }

    // Priority 8-10: Normal Rotation
    // Note: Spec-specific rotations are now handled by the template-based refactored system
    // (DevastationEvokerRefactored, PreservationEvokerRefactored, AugmentationEvokerRefactored)
    // This legacy implementation is kept for backward compatibility only

    // Detect spec for legacy behavior
    EvokerSpec currentSpec = DetectSpecialization();

    switch (currentSpec)
    {
        case EvokerSpec::DEVASTATION:
            UpdateDevastationRotation(target);
            break;
        case EvokerSpec::PRESERVATION:
            UpdatePreservationRotation(target);
            UpdateEchoSystem();
            break;
        case EvokerSpec::AUGMENTATION:
            UpdateAugmentationRotation(target);
            break;
    }

    OptimizeResourceUsage();
}

void EvokerAI::UpdateBuffs()
{
    if (!_bot)
        return;

    // Use baseline buffs for low-level bots
    if (BaselineRotationManager::ShouldUseBaselineRotation(_bot))
    {
        static BaselineRotationManager baselineManager;
        baselineManager.ApplyBaselineBuffs(_bot);
        return;
    }

    ManageBuffs();

    // Maintain hover for mobility if needed
    if (ShouldUseHover() && CanUseAbility(HOVER))
        CastHover();

    // Shift to optimal aspect
    EvokerAspect optimalAspect = GetOptimalAspect();
    if (optimalAspect != _currentAspect && CanShiftAspect())
        ShiftToAspect(optimalAspect);
}

void EvokerAI::UpdateCooldowns(uint32 diff)
{
    ClassAI::UpdateCooldowns(diff);

    // Update essence regeneration
    _lastEssenceUpdate += diff;
    if (_lastEssenceUpdate >= _essenceUpdateInterval)
    {
        if (_essence.isRegenerating && _essence.current < _essence.maximum)
        {
            GenerateEssence(1);
            _lastEssenceUpdate = 0;
        }
    }

    // Update empowered spell channeling
    if (_isChannelingEmpowered)
        UpdateEmpoweredChanneling();

    // Update echo healing
    EvokerSpec currentSpec = DetectSpecialization();
    if (currentSpec == EvokerSpec::PRESERVATION)
    {
        _lastEchoUpdate += diff;
        if (_lastEchoUpdate >= _echoUpdateInterval)
        {
            ProcessEchoHealing();
            _lastEchoUpdate = 0;
        }
    }

    // Update hover duration
    if (_isHovering && _hoverRemaining > 0)
    {
        if (_hoverRemaining <= diff)
        {
            _hoverRemaining = 0;
            _isHovering = false;
        }
        else
        {
            _hoverRemaining -= diff;
        }
    }

    // Update aspect cooldown
    if (_lastAspectShift > 0)
    {
        _lastAspectShift = _lastAspectShift > diff ? _lastAspectShift - diff : 0;
        if (_lastAspectShift == 0)
            _canShiftAspect = true;
    }

    // Update various stack counters
    if (_burnoutStacks > 0)
    {
        static uint32 burnoutDecay = 0;
        burnoutDecay += diff;
        if (burnoutDecay >= 10000) // 10 second duration
        {
            _burnoutStacks = _burnoutStacks > 0 ? _burnoutStacks - 1 : 0;
            burnoutDecay = 0;
        }
    }
}

bool EvokerAI::CanUseAbility(uint32 spellId)
{
    if (!ClassAI::CanUseAbility(spellId))
        return false;

    if (!HasEnoughResource(spellId))
        return false;

    // Cannot cast while channeling empowered spells
    if (_isChannelingEmpowered && !IsEmpoweredSpell(spellId))
        return false;

    return true;
}

void EvokerAI::OnCombatStart(::Unit* target)
{
    ClassAI::OnCombatStart(target);

    // Shift to combat-appropriate aspect
    EvokerAspect combatAspect = GetOptimalAspect();
    if (combatAspect != _currentAspect)
        ShiftToAspect(combatAspect);
}

void EvokerAI::OnCombatEnd()
{
    ClassAI::OnCombatEnd();

    // Reset combat-specific tracking
    _burnoutStacks = 0;
    _essenceBurstStacks = 0;
    _dragonrageStacks = 0;
    _temporalCompressionStacks = 0;
    _prescientStacks = 0;

    // Process any remaining echoes
    if (!_activeEchoes.empty())
        ProcessEchoHealing();
}

bool EvokerAI::HasEnoughResource(uint32 spellId)
{
    const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(spellId, GetBot()->GetMap()->GetDifficultyID());
    if (!spellInfo)
        return false;

    // Most evoker abilities require essence
    switch (spellId)
    {        case AZURE_STRIKE:        case LIVING_FLAME:            return true; // Free abilities

        case ETERNITYS_SURGE:
        case DISINTEGRATE:
        case PYRE:
        case FIRE_BREATH:
        case EMERALD_BLOSSOM:
        case VERDANT_EMBRACE:
        case DREAM_BREATH:
        case SPIRIT_BLOOM:
        case EBON_MIGHT:
        case PRESCIENCE:
        {
            auto powerCosts = spellInfo->CalcPowerCost(GetBot(), spellInfo->GetSchoolMask());
            for (auto const& cost : powerCosts)
                if (cost.Power == POWER_MANA)
                    return HasEssence(cost.Amount);
            return true;
        }

        default:
            return true;
    }
}

void EvokerAI::ConsumeResource(uint32 spellId)
{
    const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(spellId, GetBot()->GetMap()->GetDifficultyID());
    if (!spellInfo)
        return;

    switch (spellId)
    {
        case AZURE_STRIKE:
            GenerateEssence(1); // Generates essence
            break;

        case ETERNITYS_SURGE:
        case DISINTEGRATE:
        case PYRE:
        case FIRE_BREATH:
        case EMERALD_BLOSSOM:
        case VERDANT_EMBRACE:
        case DREAM_BREATH:
        case SPIRIT_BLOOM:
        case EBON_MIGHT:
        case PRESCIENCE:
        {
            auto powerCosts = spellInfo->CalcPowerCost(GetBot(), spellInfo->GetSchoolMask());
            for (auto const& cost : powerCosts)
                if (cost.Power == POWER_MANA)
                    SpendEssence(cost.Amount);
            break;
        }
    }
}

Position EvokerAI::GetOptimalPosition(::Unit* target)
{
    if (!target)        return _bot->GetPosition();

    Position pos = _bot->GetPosition();
    float distance = ::std::sqrt(_bot->GetExactDistSq(target)); // Calculate once from squared distance
    float optimalRange = GetOptimalRange(target);

    if (distance > optimalRange || distance < optimalRange * 0.8f)
    {
        pos = target->GetPosition();
        pos.m_positionX += optimalRange * cos(target->GetOrientation() + M_PI);        pos.m_positionY += optimalRange * sin(target->GetOrientation() + M_PI);    }

    return pos;
}

float EvokerAI::GetOptimalRange(::Unit* target)
{
    if (_isChannelingEmpowered)
        return EMPOWERED_SPELL_RANGE;

    return OPTIMAL_CASTING_RANGE;
}

void EvokerAI::UpdateDevastationRotation(::Unit* target)
{
    if (!target)
        return;

    // Use empowered abilities when available
    if (!_isChannelingEmpowered)
    {
        // Eternity's Surge with empowerment
    if (_eternitysSurgeReady && CanUseAbility(ETERNITYS_SURGE))
        {
            EmpowermentLevel level = CalculateOptimalEmpowermentLevel(ETERNITYS_SURGE, target);
            CastEmpoweredEternitysSurge(target, level);
            return;
        }

        // Fire Breath for AoE situations
        ::std::vector<::Unit*> enemies = GetEmpoweredSpellTargets(FIRE_BREATH);
        if (enemies.size() >= 3 && CanUseAbility(FIRE_BREATH))
        {
            EmpowermentLevel level = CalculateOptimalEmpowermentLevel(FIRE_BREATH, target);
            CastEmpoweredFireBreath(target, level);
            return;
        }
    }

    // Non-empowered abilities
    if (HasEssence(2) && CanUseAbility(DISINTEGRATE))
    {
        CastDisintegrate(target);
        return;
    }

    if (HasEssence(3) && CanUseAbility(PYRE))
    {
        CastPyre(target);
        return;
    }

    // Living Flame for damage and essence generation
    if (CanUseAbility(LIVING_FLAME))
    {
        CastLivingFlame(target);
        return;
    }

    // Azure Strike for essence generation
    if (CanUseAbility(AZURE_STRIKE))
    {
        CastAzureStrike(target);
    }
}

void EvokerAI::UpdatePreservationRotation(::Unit* target)
{
    // Prioritize healing over damage
    UsePreservationAbilities();

    // DPS if no healing needed
    if (!GetBestHealTarget() && target)
    {
        if (CanUseAbility(LIVING_FLAME))
            CastLivingFlame(target);
        else if (CanUseAbility(AZURE_STRIKE))
            CastAzureStrike(target);
    }
}

void EvokerAI::UpdateAugmentationRotation(::Unit* target)
{
    if (!target)
        return;

    UseAugmentationAbilities(target);
    ManageAugmentationBuffs();

    // Augment allies with buffs
    ::Unit* augmentTarget = GetBestAugmentationTarget();
    if (augmentTarget)
    {
        if (CanUseAbility(EBON_MIGHT))
            CastEbonMight(augmentTarget);
        else if (CanUseAbility(PRESCIENCE))
            CastPrescience(augmentTarget);
    }

    // DPS rotation when augmentation is maintained
    if (CanUseAbility(LIVING_FLAME))
        CastLivingFlame(target);
    else if (CanUseAbility(AZURE_STRIKE))
        CastAzureStrike(target);
}

void EvokerAI::UpdateEssenceManagement(::Unit* target)
{
    // Essence regenerates naturally over time (handled in UpdateCooldowns)    // DEADLOCK FIX: Use target parameter instead of ObjectAccessor (worker thread safe)
    if (!target || !target->IsInWorld())
        return;

    // Optimize essence usage based on specialization
    if (_essence.current >= _essence.maximum * 0.9f)
    {
        EvokerSpec currentSpec = DetectSpecialization();
        // Spend excess essence
    if (currentSpec == EvokerSpec::DEVASTATION && this->CanUseAbility(DISINTEGRATE))
            this->CastDisintegrate(target);
        else if (currentSpec == EvokerSpec::PRESERVATION)
        {
            ::Unit* healTarget = this->GetBestHealTarget();
            if (healTarget && this->CanUseAbility(VERDANT_EMBRACE))
                this->CastVerdantEmbrace(healTarget);
        }
    }
}

void EvokerAI::GenerateEssence(uint32 amount)
{
    _essence.GenerateEssence(amount);
    _essenceGenerated += amount;
}

void EvokerAI::SpendEssence(uint32 amount)
{
    _essence.SpendEssence(amount);
}

bool EvokerAI::HasEssence(uint32 required)
{
    return _essence.HasEssence(required);
}

uint32 EvokerAI::GetEssence()
{
    return _essence.current;
}

uint32 EvokerAI::GetMaxEssence()
{
    return _essence.maximum;
}

float EvokerAI::GetEssencePercent()
{
    return _essence.maximum > 0 ? (float)_essence.current / _essence.maximum : 0.0f;
}

void EvokerAI::UpdateEmpowermentSystem()
{    if (_isChannelingEmpowered)
        UpdateEmpoweredChanneling();
}

void EvokerAI::StartEmpoweredSpell(uint32 spellId, EmpowermentLevel targetLevel, ::Unit* target)
{
    if (_isChannelingEmpowered)
        return;

    _currentEmpoweredSpell = EmpoweredSpell(spellId, targetLevel, target);
    _isChannelingEmpowered = true;
    _lastEmpoweredSpell = GameTime::GetGameTimeMS();
}

void EvokerAI::UpdateEmpoweredChanneling()
{
    if (!_isChannelingEmpowered)
        return;

    // Check if we should release the spell
    if (_currentEmpoweredSpell.ShouldRelease())
    {
        ReleaseEmpoweredSpell();
    }
}

void EvokerAI::ReleaseEmpoweredSpell()
{
    if (!_isChannelingEmpowered)
        return;

    // Cast the empowered spell at the achieved level
    uint32 spellId = _currentEmpoweredSpell.spellId;
    ::Unit* target = _currentEmpoweredSpell.target;

    if (target && CanUseAbility(spellId))
    {        _bot->CastSpell(CastSpellTargetArg(target), spellId);
        ConsumeResource(spellId);
        _empoweredSpellsCast++;
    }

    // Reset empowered spell state    _currentEmpoweredSpell = EmpoweredSpell();
    _isChannelingEmpowered = false;
}

EmpowermentLevel EvokerAI::CalculateOptimalEmpowermentLevel(uint32 spellId, ::Unit* target)
{
    if (!target)
        return EmpowermentLevel::RANK_1;

    // Calculate based on situation
    ::std::vector<::Unit*> targets = GetEmpoweredSpellTargets(spellId);

    if (targets.size() >= 5)
        return EmpowermentLevel::RANK_4;
    else if (targets.size() >= 3)
        return EmpowermentLevel::RANK_3;
    else if (targets.size() >= 2)
        return EmpowermentLevel::RANK_2;
    else
        return EmpowermentLevel::RANK_1;
}

void EvokerAI::UpdateEchoSystem()
{
    EvokerSpec currentSpec = DetectSpecialization();
    if (currentSpec != EvokerSpec::PRESERVATION)
        return;

    ProcessEchoHealing();
    RemoveExpiredEchoes();
}

void EvokerAI::CreateEcho(::Unit* target, uint32 healAmount, uint32 numHeals)
{
    if (!target || _activeEchoes.size() >= _maxEchoes)
        return;

    _activeEchoes.emplace_back(target, numHeals, healAmount);
}

void EvokerAI::ProcessEchoHealing()
{
    for (auto& echo : _activeEchoes)
    {        if (echo.ShouldHeal() && echo.target)
        {
            // Perform echo healing            _bot->CastSpell(CastSpellTargetArg(echo.target), ECHO);
            echo.ProcessHeal();
            _echoHealsPerformed++;
        }
    }
}

void EvokerAI::RemoveExpiredEchoes()
{
    _activeEchoes.erase(
        ::std::remove_if(_activeEchoes.begin(), _activeEchoes.end(),            [](const Echo& echo) { return echo.remainingHeals == 0 || !echo.target; }),
        _activeEchoes.end());
}
uint32 EvokerAI::GetActiveEchoCount()
{
    return static_cast<uint32>(_activeEchoes.size());
}

bool EvokerAI::ShouldCreateEcho(::Unit* target)
{
    return target && target->GetHealthPct() < 80.0f && GetActiveEchoCount() < _maxEchoes;
}

void EvokerAI::UpdateAspectManagement()
{
    // Aspect management is handled in UpdateBuffs()
}

void EvokerAI::ShiftToAspect(EvokerAspect aspect)
{
    if (!CanShiftAspect() || _currentAspect == aspect)
        return;

    uint32 aspectSpellId = 0;
    switch (aspect)
    {        case EvokerAspect::BRONZE: aspectSpellId = BRONZE_ASPECT; break;
        case EvokerAspect::AZURE: aspectSpellId = AZURE_ASPECT; break;
        case EvokerAspect::GREEN: aspectSpellId = GREEN_ASPECT; break;
        case EvokerAspect::RED: aspectSpellId = RED_ASPECT; break;
        case EvokerAspect::BLACK: aspectSpellId = BLACK_ASPECT; break;
        default: return;
    }

    if (CanUseAbility(aspectSpellId))
    {        _bot->CastSpell(CastSpellTargetArg(_bot), aspectSpellId);
        _currentAspect = aspect;
        _lastAspectShift = _aspectCooldown;
        _canShiftAspect = false;
    }
}

EvokerAspect EvokerAI::GetOptimalAspect()
{
    EvokerSpec currentSpec = DetectSpecialization();
    switch (currentSpec)
    {
        case EvokerSpec::DEVASTATION:
            return EvokerAspect::RED; // Red for damage
        case EvokerSpec::PRESERVATION:
            return EvokerAspect::GREEN; // Green for healing
        case EvokerSpec::AUGMENTATION:
            return EvokerAspect::BRONZE; // Bronze for support
        default:
            return EvokerAspect::AZURE;
    }
}

bool EvokerAI::CanShiftAspect()
{
    return _canShiftAspect && _lastAspectShift == 0;
}

::Unit* EvokerAI::GetBestHealTarget()
{
    ::Unit* lowestTarget = nullptr;
    float lowestHealth = 100.0f;

    // Check self
    if (_bot->GetHealthPct() < lowestHealth)
    {
        lowestTarget = _bot;
        lowestHealth = _bot->GetHealthPct();
    }

    // Check group members
    if (Group* group = _bot->GetGroup())    {
        float rangeSq = OPTIMAL_CASTING_RANGE * OPTIMAL_CASTING_RANGE;
        for (auto const& member : group->GetMemberSlots())
        {
            if (Player* player = ObjectAccessor::FindPlayer(member.guid))
            {
                if (player->GetHealthPct() < lowestHealth && player->GetExactDistSq(_bot) <= rangeSq)
                {
                    lowestTarget = player;
                    lowestHealth = player->GetHealthPct();
                }
            }
        }
    }

    return lowestTarget;
}

::Unit* EvokerAI::GetBestAugmentationTarget()
{
    // Find ally DPS for augmentation buffs
    if (Group* group = _bot->GetGroup())    {
        float rangeSq = OPTIMAL_CASTING_RANGE * OPTIMAL_CASTING_RANGE;
        for (auto const& member : group->GetMemberSlots())
        {
            if (Player* player = ObjectAccessor::FindPlayer(member.guid))
            {
                if (player->GetExactDistSq(_bot) <= rangeSq && player != _bot)
                {
                    // Prefer DPS classes for augmentation
                    return player;
                }
            }
        }
    }

    return nullptr;
}

::std::vector<::Unit*> EvokerAI::GetEmpoweredSpellTargets(uint32 spellId)
{
    ::std::vector<::Unit*> targets;

    ::std::list<Unit*> nearbyEnemies;
    Trinity::AnyUnitInObjectRangeCheck check(_bot, EMPOWERED_SPELL_RANGE);
    Trinity::UnitListSearcher<Trinity::AnyUnitInObjectRangeCheck> searcher(_bot, nearbyEnemies, check);
    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Map* map = _bot->GetMap();
    if (!map)
        return {};

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return {};
    }

    // Query nearby GUIDs (lock-free!)
    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        _bot->GetPosition(), EMPOWERED_SPELL_RANGE);

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
    for (Unit* enemy : nearbyEnemies)
    {
        targets.push_back(enemy);
    }

    return targets;
}

EvokerSpec EvokerAI::DetectSpecialization()
{
    Player* bot = GetBot();
    if (!bot)
        return EvokerSpec::DEVASTATION;

    // Use TrinityCore's GetPrimarySpecialization to determine specialization
    // 1467 = Devastation, 1468 = Preservation, 1473 = Augmentation
    uint32 spec = static_cast<uint32>(bot->GetPrimarySpecialization());

    switch (spec)
    {
        case 1468: // ChrSpecialization::EvokerPreservation
            return EvokerSpec::PRESERVATION;
        case 1473: // ChrSpecialization::EvokerAugmentation
            return EvokerSpec::AUGMENTATION;
        case 1467: // ChrSpecialization::EvokerDevastation
        default:
            return EvokerSpec::DEVASTATION;
    }
}

bool EvokerAI::IsEmpoweredSpell(uint32 spellId)
{
    switch (spellId)
    {
        case ETERNITYS_SURGE:
        case FIRE_BREATH:
        case DREAM_BREATH:
        case SPIRIT_BLOOM:
            return true;
        default:
            return false;
    }
}

// Combat ability implementations
void EvokerAI::CastAzureStrike(::Unit* target)
{
    if (!target || !CanUseAbility(AZURE_STRIKE))
        return;

    _bot->CastSpell(CastSpellTargetArg(target), AZURE_STRIKE);
    ConsumeResource(AZURE_STRIKE);
}

void EvokerAI::CastLivingFlame(::Unit* target)
{
    if (!target || !CanUseAbility(LIVING_FLAME))
        return;

    _bot->CastSpell(CastSpellTargetArg(target), LIVING_FLAME);
    ConsumeResource(LIVING_FLAME);
}

void EvokerAI::CastEmpoweredEternitysSurge(::Unit* target, EmpowermentLevel level)
{
    if (!target || !CanUseAbility(ETERNITYS_SURGE))
        return;

    StartEmpoweredSpell(ETERNITYS_SURGE, level, target);
    _eternitysSurgeReady = false;
}

void EvokerAI::CastDisintegrate(::Unit* target)
{
    if (!target || !CanUseAbility(DISINTEGRATE))
        return;

    _bot->CastSpell(CastSpellTargetArg(target), DISINTEGRATE);
    _lastDisintegrate = GameTime::GetGameTimeMS();
    ConsumeResource(DISINTEGRATE);
}

void EvokerAI::CastPyre(::Unit* target)
{
    if (!target || !CanUseAbility(PYRE))
        return;

    _bot->CastSpell(CastSpellTargetArg(target), PYRE);
    ConsumeResource(PYRE);
}

void EvokerAI::CastEmpoweredFireBreath(::Unit* target, EmpowermentLevel level)
{
    if (!target || !CanUseAbility(FIRE_BREATH))
        return;

    StartEmpoweredSpell(FIRE_BREATH, level, target);
}

void EvokerAI::UsePreservationAbilities()
{
    ::Unit* healTarget = GetBestHealTarget();
    if (!healTarget)
        return;

    float healthPercent = healTarget->GetHealthPct();

    // Emergency healing
    if (healthPercent < 30.0f)
    {
        if (!_isChannelingEmpowered && CanUseAbility(SPIRIT_BLOOM))
        {
            EmpowermentLevel level = CalculateOptimalEmpowermentLevel(SPIRIT_BLOOM, healTarget);
            CastEmpoweredSpiritBloom(healTarget, level);
        }
        else if (CanUseAbility(VERDANT_EMBRACE))
            CastVerdantEmbrace(healTarget);
    }
    // Moderate healing
    else if (healthPercent < 70.0f)
    {
        if (CanUseAbility(EMERALD_BLOSSOM))
            CastEmeraldBlossom();
        else if (ShouldCreateEcho(healTarget) && CanUseAbility(VERDANT_EMBRACE))
        {
            CastVerdantEmbrace(healTarget);
            CreateEcho(healTarget, 1000, 3);
        }
    }
    // Group healing
    else if (CanUseAbility(EMERALD_BLOSSOM))
    {
        CastEmeraldBlossom();
    }
}

void EvokerAI::CastEmeraldBlossom()
{
    if (!CanUseAbility(EMERALD_BLOSSOM))
        return;

    _bot->CastSpell(CastSpellTargetArg(_bot), EMERALD_BLOSSOM);
    ConsumeResource(EMERALD_BLOSSOM);
}

void EvokerAI::CastVerdantEmbrace(::Unit* target)
{
    if (!target || !CanUseAbility(VERDANT_EMBRACE))
        return;

    _bot->CastSpell(CastSpellTargetArg(target), VERDANT_EMBRACE);
    _lastVerdantEmbrace = GameTime::GetGameTimeMS();
    ConsumeResource(VERDANT_EMBRACE);
}

void EvokerAI::CastEmpoweredDreamBreath(::Unit* target, EmpowermentLevel level)
{
    if (!target || !CanUseAbility(DREAM_BREATH))
        return;

    StartEmpoweredSpell(DREAM_BREATH, level, target);
}

void EvokerAI::CastEmpoweredSpiritBloom(::Unit* target, EmpowermentLevel level)
{
    if (!target || !CanUseAbility(SPIRIT_BLOOM))
        return;

    StartEmpoweredSpell(SPIRIT_BLOOM, level, target);
}

void EvokerAI::UseAugmentationAbilities(::Unit* target)
{
    ::Unit* augmentTarget = GetBestAugmentationTarget();
    if (!augmentTarget)
        return;

    if (CanUseAbility(EBON_MIGHT))
        CastEbonMight(augmentTarget);
    else if (CanUseAbility(PRESCIENCE))
        CastPrescience(augmentTarget);
}

void EvokerAI::CastEbonMight(::Unit* target)
{
    if (!target || !CanUseAbility(EBON_MIGHT))
        return;

    _bot->CastSpell(CastSpellTargetArg(target), EBON_MIGHT);
    _lastEbon = GameTime::GetGameTimeMS();
    ConsumeResource(EBON_MIGHT);
}

void EvokerAI::CastPrescience(::Unit* target)
{
    if (!target || !CanUseAbility(PRESCIENCE))
        return;

    _bot->CastSpell(CastSpellTargetArg(target), PRESCIENCE);
    ConsumeResource(PRESCIENCE);
}

void EvokerAI::CastHover()
{
    if (!CanUseAbility(HOVER))
        return;

    _bot->CastSpell(CastSpellTargetArg(_bot), HOVER);
    _isHovering = true;
    _hoverRemaining = 8000; // 8 seconds
}

bool EvokerAI::ShouldUseHover()
{
    // Use hover for mobility when repositioning or under threat
    return !_isHovering && (_bot->IsInCombat() && _bot->GetHealthPct() < 50.0f);
}

void EvokerAI::ManageBuffs()
{
    // Maintain aspect and other buffs
}

void EvokerAI::ManageAugmentationBuffs()
{
    // Manage augmentation buff timers and priorities
}

void EvokerAI::OptimizeResourceUsage()
{
    // DEADLOCK FIX: This function is deprecated - essence optimization now handled in UpdateEssenceManagement
    // which receives target parameter from UpdateRotation (thread-safe)
    // Keeping function for backward compatibility but not implementing Map access from worker thread
}

void EvokerAI::RecordDamageDealt(uint32 damage, ::Unit* target)
{
    _damageDealt += damage;
}

void EvokerAI::RecordHealingDone(uint32 amount, ::Unit* target)
{
    _healingDone += amount;
}

void EvokerAI::RecordEchoHealing(uint32 amount)
{
    _healingDone += amount;
    _echoHealsPerformed++;
}

// Utility class implementations
uint32 EvokerCalculator::CalculateAzureStrikeDamage(Player* caster, ::Unit* target)
{
    // DESIGN NOTE: Calculate Azure Strike damage for Evoker bots
    // Returns 600 as baseline damage value
    // Full implementation should:
    // - Query spell power from caster stats
    // - Apply spell power coefficient from SpellInfo
    // - Factor in target resistances and armor
    // - Apply mastery and versatility modifiers
    // Reference: WoW 11.2 Azure Strike (spell ID varies by spec)
    return 600;
}

uint32 EvokerCalculator::CalculateLivingFlameDamage(Player* caster, ::Unit* target)
{
    // DESIGN NOTE: Calculate Living Flame damage for Evoker bots
    // Returns 800 as baseline damage value
    // Full implementation should:
    // - Query spell power and attack power from caster
    // - Apply dual-scaling coefficient (spell + attack power)
    // - Factor in target resistances
    // - Apply Devastation mastery (Giantkiller) bonus if applicable
    // Reference: WoW 11.2 Living Flame (spell ID 361469)
    return 800;
}

uint32 EvokerCalculator::CalculateEmpoweredSpellDamage(uint32 spellId, EmpowermentLevel level, Player* caster, ::Unit* target)
{
    uint32 baseDamage = 1000;
    return baseDamage * (static_cast<uint32>(level) + 1); // Scales with empowerment level
}

uint32 EvokerCalculator::CalculateEmeraldBlossomHealing(Player* caster)
{
    // DESIGN NOTE: Calculate Emerald Blossom healing for Preservation Evoker bots
    // Returns 800 as baseline healing value
    // Full implementation should:
    // - Query spell power and versatility from caster
    // - Apply healing coefficient from SpellInfo
    // - Factor in Preservation mastery (Lifebinder) modifier
    // - Apply critical strike and multistrike bonuses
    // - Consider Echo interaction for duplicate healing
    // Reference: WoW 11.2 Emerald Blossom (spell ID 355913)
    return 800;
}

uint32 EvokerCalculator::CalculateVerdantEmbraceHealing(Player* caster, ::Unit* target)
{
    // DESIGN NOTE: Calculate Verdant Embrace healing for Preservation Evoker bots
    // Returns 1200 as baseline healing value
    // Full implementation should:
    // - Query spell power from caster stats
    // - Apply instant heal coefficient
    // - Factor in Preservation mastery (Lifebinder) multiplier
    // - Apply critical strike and versatility bonuses
    // - Consider target's missing health for smart healing
    // - Track Echo application for follow-up heals
    // Reference: WoW 11.2 Verdant Embrace (spell ID 360995)
    return 1200;
}

uint32 EvokerCalculator::CalculateEchoHealing(Player* caster, ::Unit* target)
{
    // DESIGN NOTE: Calculate Echo healing for Preservation Evoker bots
    // Returns 300 as baseline periodic healing value
    // Full implementation should:
    // - Query spell power from caster stats
    // - Apply Echo coefficient (percentage of original heal)
    // - Factor in number of active Echoes on target
    // - Apply Preservation mastery and versatility
    // - Consider Echo duration and remaining heal count
    // Reference: WoW 11.2 Echo (spell ID 364343) - signature Preservation mechanic
    return 300;
}

EmpowermentLevel EvokerCalculator::GetOptimalEmpowermentLevel(uint32 spellId, Player* caster, ::Unit* target)
{
    // DESIGN NOTE: Determine optimal empowerment level for Evoker empowered spells
    // Returns RANK_2 as balanced default empowerment
    // Full implementation should:
    // - Analyze current essence availability
    // - Calculate damage/healing scaling per rank
    // - Factor in number of targets for AoE spells
    // - Consider combat urgency (use higher rank for burst)
    // - Balance channel time vs immediate need
    // Reference: WoW 11.2 Empowered spells (Fire Breath, Eternity's Surge, Dream Breath, Spirit Bloom)
    return EmpowermentLevel::RANK_2;
}

uint32 EvokerCalculator::CalculateEmpowermentChannelTime(EmpowermentLevel level)
{
    return static_cast<uint32>(level) * 1000; // 1 second per rank
}

float EvokerCalculator::CalculateEmpowermentEfficiency(uint32 spellId, EmpowermentLevel level, Player* caster)
{
    return 1.0f + static_cast<float>(level) * 0.25f; // 25% per rank
}

uint32 EvokerCalculator::CalculateEssenceGeneration(uint32 spellId, Player* caster)
{
    switch (spellId)
    {
        case EvokerAI::AZURE_STRIKE: return 1;
        default: return 0;
    }
}

float EvokerCalculator::CalculateEssenceEfficiency(uint32 spellId, Player* caster)
{
    // DESIGN NOTE: Calculate essence efficiency (damage or healing per essence point)
    // Returns 1.0f as neutral efficiency baseline
    // Full implementation should:
    // - Calculate total damage/healing from spell
    // - Divide by essence cost to get efficiency ratio
    // - Factor in AoE target count for cleave spells
    // - Consider cooldown availability and uptime
    // - Weight spenders vs generators differently
    // Reference: WoW 11.2 Essence resource system (max 5-6 based on talents)
    return 1.0f;
}

bool EvokerCalculator::ShouldConserveEssence(Player* caster, uint32 currentEssence)
{
    return currentEssence < 2; // Conserve when below 2 essence
}

uint32 EvokerCalculator::CalculateOptimalEchoTargets(Player* caster, const ::std::vector<::Unit*>& allies)
{
    return ::std::min(static_cast<uint32>(allies.size()), 8u);
}

bool EvokerCalculator::ShouldCreateEcho(Player* caster, ::Unit* target)
{
    return target && target->GetHealthPct() < 80.0f;
}

uint32 EvokerCalculator::CalculateEchoValue(Player* caster, ::Unit* target)
{
    return target ? (100 - static_cast<uint32>(target->GetHealthPct())) : 0;
}

uint32 EvokerCalculator::CalculateBuffEfficiency(uint32 spellId, Player* caster, ::Unit* target)
{
    // DESIGN NOTE: Calculate buff efficiency for Augmentation Evoker support abilities
    // Returns 100 as baseline efficiency score
    // Full implementation should:
    // - Query target's DPS potential and role
    // - Calculate stat buff value (Ebon Might, Prescience)
    // - Factor in target's gear and primary stat scaling
    // - Prioritize high-DPS allies for maximum raid benefit
    // - Consider buff duration and overlap prevention
    // Reference: WoW 11.2 Augmentation spec buffs (Ebon Might, Prescience, Blistering Scales)
    return 100;
}

::Unit* EvokerCalculator::GetOptimalAugmentationTarget(Player* caster, const ::std::vector<::Unit*>& allies)
{
    // DESIGN NOTE: Select optimal ally target for Augmentation Evoker buffs
    // Returns first ally as fallback selection
    // Full implementation should:
    // - Calculate DPS potential for each ally
    // - Prioritize DPS specs over tanks/healers
    // - Check for existing buff duration to avoid overwriting
    // - Consider ally proximity and combat engagement
    // - Favor allies with cooldowns active for maximum value
    // Reference: WoW 11.2 Augmentation targeting priority (melee > ranged > support)
    return allies.empty() ? nullptr : allies[0];
}

void EvokerCalculator::CacheEvokerData()
{
    // Cache implementation
}

// EssenceManager implementation
EssenceManager::EssenceManager(EvokerAI* owner) : _owner(owner), _lastUpdate(0), _updateInterval(1500)
{
}

void EssenceManager::Update(uint32 diff)
{
    _lastUpdate += diff;
    if (_lastUpdate >= _updateInterval)
    {
        UpdateEssenceRegeneration();
        _lastUpdate = 0;
    }
}

void EssenceManager::GenerateEssence(uint32 amount)
{
    _essence.GenerateEssence(amount);
}

void EssenceManager::SpendEssence(uint32 amount)
{
    _essence.SpendEssence(amount);
}

bool EssenceManager::HasEssence(uint32 required) const
{
    return _essence.HasEssence(required);
}

uint32 EssenceManager::GetEssence() const
{
    return _essence.current;
}

float EssenceManager::GetEssencePercent() const
{
    return _essence.maximum > 0 ? (float)_essence.current / _essence.maximum : 0.0f;
}

void EssenceManager::UpdateEssenceRegeneration()
{
    if (_essence.isRegenerating && _essence.current < _essence.maximum)
    {
        GenerateEssence(1);
    }
}

void EssenceManager::OptimizeEssenceUsage()
{
    // Optimization logic
}

bool EssenceManager::ShouldConserveEssence() const
{
    return GetEssencePercent() < 0.3f;
}

uint32 EssenceManager::GetOptimalEssenceLevel() const
{
    return _essence.maximum / 2; // Keep around 50%
}

// EmpowermentController implementation
EmpowermentController::EmpowermentController(EvokerAI* owner) : _owner(owner), _lastUpdate(0)
{
}

void EmpowermentController::Update(uint32 diff)
{
    if (IsChanneling())
    {
        UpdateChanneling();
    }
}

void EmpowermentController::StartEmpoweredSpell(uint32 spellId, EmpowermentLevel targetLevel, ::Unit* target)
{
    _currentSpell = EmpoweredSpell(spellId, targetLevel, target);
}

void EmpowermentController::UpdateChanneling()
{
    UpdateEmpowermentLevel();

    if (ShouldReleaseSpell())
        ReleaseSpell();
}

bool EmpowermentController::ShouldReleaseSpell()
{
    return _currentSpell.ShouldRelease();
}

void EmpowermentController::ReleaseSpell()
{
    // Release the empowered spell
    _currentSpell = EmpoweredSpell();
}

bool EmpowermentController::IsChanneling() const
{
    return _currentSpell.isChanneling;
}

EmpowermentLevel EmpowermentController::GetCurrentLevel() const
{
    return _currentSpell.currentLevel;
}

uint32 EmpowermentController::GetChannelTime() const
{
    return _currentSpell.GetChannelTime();
}

uint32 EmpowermentController::GetSpellId() const
{
    return _currentSpell.spellId;
}

EmpowermentLevel EmpowermentController::CalculateOptimalLevel(uint32 spellId, ::Unit* target)
{
    // DESIGN NOTE: Calculate optimal empowerment rank for channeled spell
    // Returns RANK_2 as balanced default empowerment
    // Full implementation should:
    // - Analyze current combat situation urgency
    // - Calculate targets hit by AoE cone/radius
    // - Balance channel time vs immediate threat response
    // - Consider essence availability for follow-up casts
    // - Factor in movement requirements and interrupt risk
    // Reference: WoW 11.2 Empowerment system (1-4 ranks, ~1 sec per rank)
    return EmpowermentLevel::RANK_2;
}

bool EmpowermentController::ShouldEmpowerSpell(uint32 spellId)
{
    // DESIGN NOTE: Determine whether to cast spell at empowered level
    // Returns true as default behavior to always use empowerment
    // Full implementation should:
    // - Check if spell is on empowered spell list (Fire Breath, Eternity's Surge, etc.)
    // - Verify sufficient essence available for higher ranks
    // - Consider combat mobility requirements (avoid channeling during movement)
    // - Evaluate interrupt risk from enemy casters
    // - Balance empowerment vs quick-cast instant abilities
    // Reference: WoW 11.2 Empowered spells require channeling and cannot be used while moving
    return true;
}

void EmpowermentController::UpdateEmpowermentLevel()
{
    // Update empowerment level based on channel time
    uint32 channelTime = GetChannelTime();
    if (channelTime >= 4000)
        _currentSpell.currentLevel = EmpowermentLevel::RANK_4;
    else if (channelTime >= 3000)
        _currentSpell.currentLevel = EmpowermentLevel::RANK_3;
    else if (channelTime >= 2000)
        _currentSpell.currentLevel = EmpowermentLevel::RANK_2;
    else if (channelTime >= 1000)
        _currentSpell.currentLevel = EmpowermentLevel::RANK_1;
}

uint32 EmpowermentController::GetRequiredChannelTime(EmpowermentLevel level)
{
    return static_cast<uint32>(level) * 1000;
}

float EmpowermentController::CalculateEmpowermentValue(EmpowermentLevel level)
{
    return 1.0f + static_cast<float>(level) * 0.25f;
}

// EchoController implementation
EchoController::EchoController(EvokerAI* owner) : _owner(owner), _lastUpdate(0), _maxEchoes(8)
{
}

void EchoController::Update(uint32 diff)
{
    _lastUpdate += diff;
    if (_lastUpdate >= 2000) // Update every 2 seconds
    {
        ProcessEchoHealing();
        RemoveExpiredEchoes();
        _lastUpdate = 0;
    }
}

void EchoController::CreateEcho(::Unit* target, uint32 healAmount, uint32 numHeals)
{
    if (_echoes.size() >= _maxEchoes)
        return;

    _echoes.emplace_back(target, numHeals, healAmount);
}

void EchoController::ProcessEchoHealing()
{
    for (auto& echo : _echoes)
    {
        if (echo.ShouldHeal())
        {
            echo.ProcessHeal();
        }
    }
}

void EchoController::RemoveExpiredEchoes()
{
    _echoes.erase(
        ::std::remove_if(_echoes.begin(), _echoes.end(),
            [](const Echo& echo) { return echo.remainingHeals == 0; }),
        _echoes.end());
}

uint32 EchoController::GetActiveEchoCount() const
{
    return static_cast<uint32>(_echoes.size());
}

bool EchoController::HasEcho(::Unit* target) const
{
    return ::std::any_of(_echoes.begin(), _echoes.end(),
        [target](const Echo& echo) { return echo.target == target; });
}

bool EchoController::ShouldCreateEcho(::Unit* target) const
{
    return target && !HasEcho(target) && target->GetHealthPct() < 80.0f;
}

::Unit* EchoController::GetBestEchoTarget() const
{
    // DESIGN NOTE: Select best ally target for Echo application (Preservation mechanic)
    // Returns nullptr as default (no valid target)
    // Full implementation should:
    // - Scan group members for missing Echo debuff
    // - Prioritize tanks and melee DPS (high sustained damage intake)
    // - Avoid targets at full health (wasted healing)
    // - Check for incoming damage patterns
    // - Consider target's role and expected damage taken
    // Reference: WoW 11.2 Echo (spell ID 364343) - applies HoT that duplicates certain heals
    return nullptr;
}

void EchoController::UpdateEchoStates()
{
    // Update echo states
}

bool EchoController::IsEchoExpired(const Echo& echo) const
{
    return echo.remainingHeals == 0 || !echo.target;
}

uint32 EchoController::CalculateEchoValue(::Unit* target) const
{
    return target ? (100 - static_cast<uint32>(target->GetHealthPct())) : 0;
}

void EchoController::OptimizeEchoPlacement()
{
    // Echo optimization logic
}

// Static member definitions moved to header with inline to fix DLL linkage (C2491)

} // namespace Playerbot