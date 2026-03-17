/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "EvokerAI.h"
#include "DevastationEvoker.h"
#include "PreservationEvoker.h"
#include "AugmentationEvoker.h"
#include "../../Combat/CombatBehaviorIntegration.h"
#include "Player.h"
#include "Spell.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
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

// Destructor must be defined in cpp file where specialization types are complete
EvokerAI::~EvokerAI() = default;

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

    // NOTE: Baseline rotation check is now handled at the dispatch level in
    // ClassAI::OnCombatUpdate(). This method is ONLY called when the bot has
    // already chosen a specialization (level 10+ with talents).

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

    // NOTE: Baseline buff check is now handled at the dispatch level.
    // This method is only called for level 10+ bots with talents.

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
    // QW-4 FIX: Removed static - now uses instance member _burnoutDecayTimer
    if (_burnoutStacks > 0)
    {
        _burnoutDecayTimer += diff;
        if (_burnoutDecayTimer >= 10000) // 10 second duration
        {
            _burnoutStacks = _burnoutStacks > 0 ? _burnoutStacks - 1 : 0;
            _burnoutDecayTimer = 0;
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
        pos.m_positionX += optimalRange * cos(target->GetOrientation() + static_cast<float>(M_PI));        pos.m_positionY += optimalRange * sin(target->GetOrientation() + static_cast<float>(M_PI));    }

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
// Helper function to calculate mastery bonus based on spec
static float GetEvokerMasteryBonus(Player* caster, ::Unit* target, bool isHealing)
{
    if (!caster)
        return 0.0f;

    float masteryPct = caster->GetRatingBonusValue(CR_MASTERY);
    ChrSpecializationEntry const* spec = caster->GetPrimarySpecializationEntry();
    if (!spec)
        return 0.0f;

    switch (spec->ID)
    {
        case 1467: // Devastation - Giantkiller: Bonus damage vs high health targets
            if (target && !isHealing)
            {
                float targetHealthPct = target->GetHealthPct() / 100.0f;
                return masteryPct * targetHealthPct * 0.012f;
            }
            return 0.0f;
        case 1468: // Preservation - Lifebinder: Bonus healing
            if (isHealing)
                return masteryPct * 0.01f;
            return 0.0f;
        case 1473: // Augmentation - Timewalker: Bonus to buff effects
            return masteryPct * 0.008f;
        default:
            return 0.0f;
    }
}

uint32 EvokerCalculator::CalculateAzureStrikeDamage(Player* caster, ::Unit* target)
{
    if (!caster || !target)
        return 0;

    constexpr uint32 AZURE_STRIKE_SPELL_ID = EvokerAI::AZURE_STRIKE;
    constexpr float DEFAULT_COEFFICIENT = 0.35f;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(AZURE_STRIKE_SPELL_ID, caster->GetMap()->GetDifficultyID());
    int32 spellPower = caster->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_ARCANE);

    float bonusCoefficient = DEFAULT_COEFFICIENT;
    int32 baseDamage = 0;

    if (spellInfo)
    {
        for (SpellEffectInfo const& effect : spellInfo->GetEffects())
        {
            if (effect.IsEffect(SPELL_EFFECT_SCHOOL_DAMAGE))
            {
                baseDamage = effect.CalcValue(caster, nullptr, target);
                if (effect.BonusCoefficient > 0.0f)
                    bonusCoefficient = effect.BonusCoefficient;
                break;
            }
        }
    }

    float damage = static_cast<float>(baseDamage) + (spellPower * bonusCoefficient);
    float versatility = caster->GetRatingBonusValue(CR_VERSATILITY_DAMAGE_DONE);
    damage *= (1.0f + versatility / 100.0f);

    float masteryBonus = GetEvokerMasteryBonus(caster, target, false);
    damage *= (1.0f + masteryBonus);

    if (spellInfo)
    {
        for (SpellEffectInfo const& effect : spellInfo->GetEffects())
        {
            if (effect.IsEffect(SPELL_EFFECT_SCHOOL_DAMAGE))
            {
                damage = static_cast<float>(caster->SpellDamageBonusDone(target, spellInfo,
                    static_cast<int32>(damage), SPELL_DIRECT_DAMAGE, effect, 1, nullptr, nullptr));
                break;
            }
        }
    }

    return static_cast<uint32>(std::max(0.0f, damage));
}

uint32 EvokerCalculator::CalculateLivingFlameDamage(Player* caster, ::Unit* target)
{
    if (!caster || !target)
        return 0;

    constexpr uint32 LIVING_FLAME_SPELL_ID = EvokerAI::LIVING_FLAME;
    constexpr float SPELL_POWER_COEFFICIENT = 0.60f;
    constexpr float ATTACK_POWER_COEFFICIENT = 0.30f;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(LIVING_FLAME_SPELL_ID, caster->GetMap()->GetDifficultyID());
    int32 spellPower = caster->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_FIRE);
    float attackPower = caster->GetTotalAttackPowerValue(BASE_ATTACK);

    float bonusCoefficient = SPELL_POWER_COEFFICIENT;
    float apCoefficient = ATTACK_POWER_COEFFICIENT;
    int32 baseDamage = 0;

    if (spellInfo)
    {
        for (SpellEffectInfo const& effect : spellInfo->GetEffects())
        {
            if (effect.IsEffect(SPELL_EFFECT_SCHOOL_DAMAGE))
            {
                baseDamage = effect.CalcValue(caster, nullptr, target);
                if (effect.BonusCoefficient > 0.0f)
                    bonusCoefficient = effect.BonusCoefficient;
                if (effect.BonusCoefficientFromAP > 0.0f)
                    apCoefficient = effect.BonusCoefficientFromAP;
                break;
            }
        }
    }

    float damage = static_cast<float>(baseDamage) + (spellPower * bonusCoefficient) + (attackPower * apCoefficient);
    float versatility = caster->GetRatingBonusValue(CR_VERSATILITY_DAMAGE_DONE);
    damage *= (1.0f + versatility / 100.0f);

    float masteryBonus = GetEvokerMasteryBonus(caster, target, false);
    damage *= (1.0f + masteryBonus);

    if (spellInfo)
    {
        for (SpellEffectInfo const& effect : spellInfo->GetEffects())
        {
            if (effect.IsEffect(SPELL_EFFECT_SCHOOL_DAMAGE))
            {
                damage = static_cast<float>(caster->SpellDamageBonusDone(target, spellInfo,
                    static_cast<int32>(damage), SPELL_DIRECT_DAMAGE, effect, 1, nullptr, nullptr));
                break;
            }
        }
    }

    return static_cast<uint32>(std::max(0.0f, damage));
}

uint32 EvokerCalculator::CalculateEmpoweredSpellDamage(uint32 spellId, EmpowermentLevel level, Player* caster, ::Unit* target)
{
    if (!caster || !target || level == EmpowermentLevel::NONE)
        return 0;

    static const float EMPOWERMENT_MULTIPLIERS[] = { 1.0f, 1.0f, 1.4f, 1.8f, 2.2f };

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, caster->GetMap()->GetDifficultyID());
    if (!spellInfo)
        return 0;

    int32 spellPower = caster->SpellBaseDamageBonusDone(spellInfo->GetSchoolMask());
    int32 baseDamage = 0;
    float bonusCoefficient = 0.8f;

    for (SpellEffectInfo const& effect : spellInfo->GetEffects())
    {
        if (effect.IsEffect(SPELL_EFFECT_SCHOOL_DAMAGE))
        {
            baseDamage = effect.CalcValue(caster, nullptr, target);
            if (effect.BonusCoefficient > 0.0f)
                bonusCoefficient = effect.BonusCoefficient;
            break;
        }
    }

    float damage = static_cast<float>(baseDamage) + (spellPower * bonusCoefficient);

    uint8 levelIndex = static_cast<uint8>(level);
    if (levelIndex <= 4)
        damage *= EMPOWERMENT_MULTIPLIERS[levelIndex];

    float versatility = caster->GetRatingBonusValue(CR_VERSATILITY_DAMAGE_DONE);
    damage *= (1.0f + versatility / 100.0f);

    float masteryBonus = GetEvokerMasteryBonus(caster, target, false);
    damage *= (1.0f + masteryBonus);

    return static_cast<uint32>(std::max(0.0f, damage));
}

uint32 EvokerCalculator::CalculateEmeraldBlossomHealing(Player* caster)
{
    if (!caster)
        return 0;

    constexpr uint32 EMERALD_BLOSSOM_SPELL_ID = EvokerAI::EMERALD_BLOSSOM;
    constexpr float DEFAULT_COEFFICIENT = 1.15f;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(EMERALD_BLOSSOM_SPELL_ID, caster->GetMap()->GetDifficultyID());
    int32 spellPower = caster->SpellBaseHealingBonusDone(SPELL_SCHOOL_MASK_NATURE);

    float bonusCoefficient = DEFAULT_COEFFICIENT;
    int32 baseHealing = 0;

    if (spellInfo)
    {
        for (SpellEffectInfo const& effect : spellInfo->GetEffects())
        {
            if (effect.IsEffect(SPELL_EFFECT_HEAL))
            {
                baseHealing = effect.CalcValue(caster, nullptr, nullptr);
                if (effect.BonusCoefficient > 0.0f)
                    bonusCoefficient = effect.BonusCoefficient;
                break;
            }
        }
    }

    float healing = static_cast<float>(baseHealing) + (spellPower * bonusCoefficient);

    // Apply versatility
    float versatility = caster->GetRatingBonusValue(CR_VERSATILITY_DAMAGE_DONE);
    healing *= (1.0f + versatility / 100.0f);

    // Apply Preservation mastery (Lifebinder)
    float masteryBonus = GetEvokerMasteryBonus(caster, nullptr, true);
    healing *= (1.0f + masteryBonus);

    // Apply critical strike chance (average contribution)
    float critChance = caster->GetRatingBonusValue(CR_CRIT_SPELL) / 100.0f;
    healing *= (1.0f + critChance * 0.5f);

    // Apply spell healing bonus modifier from gear/buffs
    if (spellInfo)
    {
        for (SpellEffectInfo const& effect : spellInfo->GetEffects())
        {
            if (effect.IsEffect(SPELL_EFFECT_HEAL))
            {
                healing = static_cast<float>(caster->SpellHealingBonusDone(caster, spellInfo,
                    static_cast<int32>(healing), HEAL, effect, 1, nullptr, nullptr));
                break;
            }
        }
    }

    return static_cast<uint32>(std::max(0.0f, healing));
}

uint32 EvokerCalculator::CalculateVerdantEmbraceHealing(Player* caster, ::Unit* target)
{
    if (!caster)
        return 0;

    constexpr uint32 VERDANT_EMBRACE_SPELL_ID = EvokerAI::VERDANT_EMBRACE;
    constexpr float DEFAULT_COEFFICIENT = 2.85f;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(VERDANT_EMBRACE_SPELL_ID, caster->GetMap()->GetDifficultyID());
    int32 spellPower = caster->SpellBaseHealingBonusDone(SPELL_SCHOOL_MASK_NATURE);

    float bonusCoefficient = DEFAULT_COEFFICIENT;
    int32 baseHealing = 0;

    if (spellInfo)
    {
        for (SpellEffectInfo const& effect : spellInfo->GetEffects())
        {
            if (effect.IsEffect(SPELL_EFFECT_HEAL))
            {
                baseHealing = effect.CalcValue(caster, nullptr, target);
                if (effect.BonusCoefficient > 0.0f)
                    bonusCoefficient = effect.BonusCoefficient;
                break;
            }
        }
    }

    float healing = static_cast<float>(baseHealing) + (spellPower * bonusCoefficient);

    // Apply versatility
    float versatility = caster->GetRatingBonusValue(CR_VERSATILITY_DAMAGE_DONE);
    healing *= (1.0f + versatility / 100.0f);

    // Apply Preservation mastery (Lifebinder) - scales with target missing health
    float masteryBonus = GetEvokerMasteryBonus(caster, target, true);
    if (target)
    {
        float missingHealthPct = (100.0f - target->GetHealthPct()) / 100.0f;
        masteryBonus *= (1.0f + missingHealthPct * 0.5f);  // Up to 50% bonus on low health targets
    }
    healing *= (1.0f + masteryBonus);

    // Apply critical strike chance
    float critChance = caster->GetRatingBonusValue(CR_CRIT_SPELL) / 100.0f;
    healing *= (1.0f + critChance * 0.5f);

    // Apply spell healing bonus modifier
    if (spellInfo && target)
    {
        for (SpellEffectInfo const& effect : spellInfo->GetEffects())
        {
            if (effect.IsEffect(SPELL_EFFECT_HEAL))
            {
                healing = static_cast<float>(caster->SpellHealingBonusDone(target, spellInfo,
                    static_cast<int32>(healing), HEAL, effect, 1, nullptr, nullptr));
                break;
            }
        }
    }

    return static_cast<uint32>(std::max(0.0f, healing));
}

uint32 EvokerCalculator::CalculateEchoHealing(Player* caster, ::Unit* target)
{
    if (!caster)
        return 0;

    constexpr uint32 ECHO_SPELL_ID = EvokerAI::ECHO;
    constexpr float ECHO_BASE_COEFFICIENT = 0.30f;  // Echo duplicates 30% of original heal

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(ECHO_SPELL_ID, caster->GetMap()->GetDifficultyID());
    int32 spellPower = caster->SpellBaseHealingBonusDone(SPELL_SCHOOL_MASK_NATURE);

    float bonusCoefficient = ECHO_BASE_COEFFICIENT;
    int32 baseHealing = 0;

    if (spellInfo)
    {
        for (SpellEffectInfo const& effect : spellInfo->GetEffects())
        {
            if (effect.IsEffect(SPELL_EFFECT_HEAL) || effect.IsEffect(SPELL_EFFECT_HEAL_PCT))
            {
                baseHealing = effect.CalcValue(caster, nullptr, target);
                if (effect.BonusCoefficient > 0.0f)
                    bonusCoefficient = effect.BonusCoefficient;
                break;
            }
        }
    }

    float healing = static_cast<float>(baseHealing) + (spellPower * bonusCoefficient);

    // Apply versatility
    float versatility = caster->GetRatingBonusValue(CR_VERSATILITY_DAMAGE_DONE);
    healing *= (1.0f + versatility / 100.0f);

    // Apply Preservation mastery (Lifebinder)
    float masteryBonus = GetEvokerMasteryBonus(caster, target, true);
    healing *= (1.0f + masteryBonus);

    // Echo healing is reduced when target has multiple Echoes (diminishing returns)
    if (target && target->HasAura(ECHO_SPELL_ID, caster->GetGUID()))
    {
        // Simple diminishing returns - assume some reduction for existing echo
        healing *= 0.9f;
    }

    // Apply spell healing bonus
    if (spellInfo && target)
    {
        for (SpellEffectInfo const& effect : spellInfo->GetEffects())
        {
            if (effect.IsEffect(SPELL_EFFECT_HEAL) || effect.IsEffect(SPELL_EFFECT_HEAL_PCT))
            {
                healing = static_cast<float>(caster->SpellHealingBonusDone(target, spellInfo,
                    static_cast<int32>(healing), HEAL, effect, 1, nullptr, nullptr));
                break;
            }
        }
    }

    return static_cast<uint32>(std::max(0.0f, healing));
}

EmpowermentLevel EvokerCalculator::GetOptimalEmpowermentLevel(uint32 spellId, Player* caster, ::Unit* target)
{
    if (!caster || !target)
        return EmpowermentLevel::RANK_1;

    // Get combat urgency factors
    float targetHealthPct = target->GetHealthPct();
    float casterHealthPct = caster->GetHealthPct();
    bool inDanger = casterHealthPct < 40.0f;
    bool targetDying = targetHealthPct < 20.0f;

    // Fast response if caster in danger or target about to die
    if (inDanger || targetDying)
        return EmpowermentLevel::RANK_1;

    // Get spec to determine healing vs damage priority
    ChrSpecializationEntry const* spec = caster->GetPrimarySpecializationEntry();
    bool isHealer = spec && spec->ID == 1468;  // Preservation

    // For healers, check group health state
    if (isHealer)
    {
        uint32 criticalAllies = 0;
        uint32 injuredAllies = 0;

        if (Group* group = caster->GetGroup())
        {
            for (auto const& member : group->GetMemberSlots())
            {
                if (Player* player = ObjectAccessor::FindPlayer(member.guid))
                {
                    float hp = player->GetHealthPct();
                    if (hp < 30.0f)
                        criticalAllies++;
                    else if (hp < 70.0f)
                        injuredAllies++;
                }
            }
        }

        // Emergency: fast heal
        if (criticalAllies >= 2)
            return EmpowermentLevel::RANK_1;

        // Multiple injured: medium empowerment for throughput
        if (injuredAllies >= 3)
            return EmpowermentLevel::RANK_2;

        // Light damage: max empowerment for efficiency
        if (injuredAllies >= 1)
            return EmpowermentLevel::RANK_3;

        // No urgency: full empowerment
        return EmpowermentLevel::RANK_4;
    }

    // For DPS specs, consider AoE target count
    uint32 nearbyEnemies = 0;
    float rangeSq = 30.0f * 30.0f;

    // Count enemies in range for AoE evaluation
    if (Map* map = caster->GetMap())
    {
        for (auto& itr : map->GetPlayers())
        {
            if (Unit* unit = itr.GetSource())
            {
                if (unit->IsHostileTo(caster) && unit->GetExactDistSq(target) <= rangeSq)
                    nearbyEnemies++;
            }
        }
    }

    // Large AoE: max empowerment for cleave value
    if (nearbyEnemies >= 5)
        return EmpowermentLevel::RANK_4;

    // Medium group: good empowerment
    if (nearbyEnemies >= 3)
        return EmpowermentLevel::RANK_3;

    // Small group: balanced empowerment
    if (nearbyEnemies >= 2)
        return EmpowermentLevel::RANK_2;

    // Single target: still use RANK_2 for decent damage
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
    if (!caster)
        return 1.0f;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, caster->GetMap()->GetDifficultyID());
    if (!spellInfo)
        return 1.0f;

    // Get essence cost
    uint32 essenceCost = 0;
    auto powerCosts = spellInfo->CalcPowerCost(caster, spellInfo->GetSchoolMask());
    for (auto const& cost : powerCosts)
    {
        if (cost.Power == POWER_ESSENCE || cost.Power == POWER_MANA)
        {
            essenceCost = cost.Amount;
            break;
        }
    }

    // Generator spells have infinite efficiency
    if (essenceCost == 0)
        return 100.0f;

    // Calculate base damage/healing value
    float spellValue = 0.0f;
    int32 spellPower = caster->SpellBaseDamageBonusDone(spellInfo->GetSchoolMask());

    for (SpellEffectInfo const& effect : spellInfo->GetEffects())
    {
        if (effect.IsEffect(SPELL_EFFECT_SCHOOL_DAMAGE) || effect.IsEffect(SPELL_EFFECT_HEAL))
        {
            float baseValue = static_cast<float>(effect.CalcValue(caster, nullptr, nullptr));
            float coefficient = effect.BonusCoefficient > 0.0f ? effect.BonusCoefficient : 0.5f;
            spellValue = baseValue + (spellPower * coefficient);
            break;
        }
    }

    // Calculate efficiency = value per essence point
    float efficiency = spellValue / static_cast<float>(essenceCost);

    // Bonus efficiency for spells that hit multiple targets
    switch (spellId)
    {
        case EvokerAI::PYRE:
        case EvokerAI::FIRE_BREATH:
        case EvokerAI::ETERNITYS_SURGE:
        case EvokerAI::EMERALD_BLOSSOM:
        case EvokerAI::DREAM_BREATH:
        case EvokerAI::SPIRIT_BLOOM:
            efficiency *= 1.5f;  // AoE bonus
            break;
        case EvokerAI::DISINTEGRATE:
            efficiency *= 1.2f;  // Channeled bonus (sustained damage)
            break;
    }

    // Normalize to 0-100 scale where 50 is average
    return std::min(100.0f, efficiency / 100.0f * 50.0f);
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
    if (!caster || !target)
        return 0;

    Player* targetPlayer = target->ToPlayer();
    if (!targetPlayer)
        return 50;  // Non-player targets get base efficiency

    uint32 efficiency = 0;

    // Get target's spec for role determination
    ChrSpecializationEntry const* targetSpec = targetPlayer->GetPrimarySpecializationEntry();
    uint32 specId = targetSpec ? targetSpec->ID : 0;

    // Role-based priority (DPS > Tank > Healer for damage buffs)
    bool isDPS = false;
    bool isTank = false;
    bool isHealer = false;

    // Determine role from spec
    if (targetSpec)
    {
        uint32 role = targetSpec->Role;
        isDPS = (role == 0);      // SPEC_ROLE_DPS
        isTank = (role == 1);     // SPEC_ROLE_TANK
        isHealer = (role == 2);   // SPEC_ROLE_HEALER
    }

    // Base efficiency by role
    if (isDPS)
        efficiency = 100;
    else if (isTank)
        efficiency = 60;
    else if (isHealer)
        efficiency = 40;
    else
        efficiency = 50;

    // Modify by target's current DPS potential (approximated by attack power + spell power)
    float attackPower = targetPlayer->GetTotalAttackPowerValue(BASE_ATTACK);
    float spellPower = static_cast<float>(targetPlayer->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_ALL));
    float totalPower = std::max(attackPower, spellPower);

    // Scale efficiency by power level (higher geared players benefit more)
    float powerMultiplier = std::min(2.0f, totalPower / 5000.0f);
    efficiency = static_cast<uint32>(efficiency * powerMultiplier);

    // Check for existing buff to avoid overwriting
    switch (spellId)
    {
        case EvokerAI::EBON_MIGHT:
            if (target->HasAura(395152))  // Ebon Might aura
                efficiency = efficiency / 2;  // Reduced value if already buffed
            break;
        case EvokerAI::PRESCIENCE:
            if (target->HasAura(410089))  // Prescience aura
                efficiency = efficiency / 2;
            break;
    }

    // Bonus for targets with active cooldowns (Lust, Trinkets, etc.)
    if (target->HasAura(2825) ||   // Bloodlust
        target->HasAura(32182) ||  // Heroism
        target->HasAura(80353))    // Time Warp
    {
        efficiency = static_cast<uint32>(efficiency * 1.5f);
    }

    return std::min(200u, efficiency);  // Cap at 200
}

::Unit* EvokerCalculator::GetOptimalAugmentationTarget(Player* caster, const ::std::vector<::Unit*>& allies)
{
    if (!caster || allies.empty())
        return nullptr;

    ::Unit* bestTarget = nullptr;
    uint32 bestScore = 0;

    for (::Unit* ally : allies)
    {
        if (!ally || ally == caster || !ally->IsAlive())
            continue;

        Player* allyPlayer = ally->ToPlayer();
        if (!allyPlayer)
            continue;

        uint32 score = 0;

        // Get spec for role determination
        ChrSpecializationEntry const* spec = allyPlayer->GetPrimarySpecializationEntry();
        if (spec)
        {
            uint32 role = spec->Role;
            // Role priority: DPS (100) > Tank (40) > Healer (20)
            if (role == 0)       // DPS
                score += 100;
            else if (role == 1)  // Tank
                score += 40;
            else if (role == 2)  // Healer
                score += 20;
        }

        // Melee vs Ranged bonus (melee often does more damage with uptime)
        float distance = std::sqrt(ally->GetExactDistSq(caster->GetVictim()));
        if (distance < 8.0f && caster->GetVictim())
            score += 20;  // Melee bonus

        // Power scaling (higher geared = more value from buffs)
        float attackPower = allyPlayer->GetTotalAttackPowerValue(BASE_ATTACK);
        float spellPower = static_cast<float>(allyPlayer->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_ALL));
        float totalPower = std::max(attackPower, spellPower);
        score += static_cast<uint32>(totalPower / 100.0f);  // +1 per 100 power

        // Cooldown active bonus
        if (ally->HasAura(2825) ||   // Bloodlust
            ally->HasAura(32182) ||  // Heroism
            ally->HasAura(80353))    // Time Warp
        {
            score += 50;
        }

        // Penalty for already having Augmentation buffs
        if (ally->HasAura(395152))  // Ebon Might
            score = score / 2;
        if (ally->HasAura(410089))  // Prescience
            score = score / 2;

        // Penalty for low health (might die, wasting buff)
        if (ally->GetHealthPct() < 30.0f)
            score = score / 3;

        // In combat bonus
        if (ally->IsInCombat())
            score += 10;

        if (score > bestScore)
        {
            bestScore = score;
            bestTarget = ally;
        }
    }

    // Fallback to first alive DPS if no clear winner
    if (!bestTarget)
    {
        for (::Unit* ally : allies)
        {
            if (ally && ally != caster && ally->IsAlive())
            {
                bestTarget = ally;
                break;
            }
        }
    }

    return bestTarget;
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
    if (!_owner || !target)
        return EmpowermentLevel::RANK_1;

    Player* caster = _owner->GetBot();
    if (!caster)
        return EmpowermentLevel::RANK_1;

    // Emergency situations: fast cast
    if (caster->GetHealthPct() < 30.0f)
        return EmpowermentLevel::RANK_1;

    // Check for interrupt threats
    bool hasInterruptThreat = false;
    float rangeSq = 30.0f * 30.0f;

    if (Map* map = caster->GetMap())
    {
        // Check nearby enemies for caster mobs
        for (auto& pair : map->GetPlayers())
        {
            if (Player* player = pair.GetSource())
            {
                if (player->IsHostileTo(caster) && player->GetExactDistSq(caster) <= rangeSq)
                {
                    // Check if enemy is casting (potential interrupt)
                    if (player->IsNonMeleeSpellCast(false))
                    {
                        hasInterruptThreat = true;
                        break;
                    }
                }
            }
        }
    }

    // High interrupt risk: fast cast
    if (hasInterruptThreat)
        return EmpowermentLevel::RANK_1;

    // Get player's current power level (essence approximation through mana)
    uint32 currentPowerPct = static_cast<uint32>(caster->GetPowerPct(POWER_MANA));

    // Low resources: conserve with lower empowerment
    if (currentPowerPct <= 30)
        return EmpowermentLevel::RANK_2;

    // Count nearby enemies for AoE evaluation
    uint32 targetCount = 1;
    ::std::list<Unit*> nearbyEnemies;
    Trinity::AnyUnfriendlyUnitInObjectRangeCheck check(caster, caster, 30.0f);
    Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(caster, nearbyEnemies, check);
    Cell::VisitAllObjects(caster, searcher, 30.0f);
    targetCount = static_cast<uint32>(nearbyEnemies.size());

    // Scale empowerment with target count
    if (targetCount >= 5)
        return EmpowermentLevel::RANK_4;
    else if (targetCount >= 3)
        return EmpowermentLevel::RANK_3;
    else if (targetCount >= 2)
        return EmpowermentLevel::RANK_2;

    // Check spec via player's primary specialization
    ChrSpecializationEntry const* spec = caster->GetPrimarySpecializationEntry();
    if (spec && spec->ID == 1468)  // Preservation
    {
        // Healer: higher empowerment for throughput
        return EmpowermentLevel::RANK_3;
    }

    // Default: balanced empowerment
    return EmpowermentLevel::RANK_2;
}

bool EmpowermentController::ShouldEmpowerSpell(uint32 spellId)
{
    if (!_owner)
        return false;

    Player* caster = _owner->GetBot();
    if (!caster)
        return false;

    // Check if spell is empowerable
    switch (spellId)
    {
        case EvokerAI::FIRE_BREATH:
        case EvokerAI::ETERNITYS_SURGE:
        case EvokerAI::DREAM_BREATH:
        case EvokerAI::SPIRIT_BLOOM:
            break;  // Valid empowered spells
        default:
            return false;  // Not an empowered spell
    }

    // Don't empower if currently moving
    if (caster->isMoving())
        return false;

    // Don't empower if already channeling
    if (_currentSpell.isChanneling)
        return false;

    // Don't empower in emergency situations
    if (caster->GetHealthPct() < 20.0f)
        return false;

    // Check for nearby enemy casters (interrupt risk)
    bool highInterruptRisk = false;
    ::std::list<Unit*> nearbyEnemies;
    Trinity::AnyUnfriendlyUnitInObjectRangeCheck check(caster, caster, 8.0f);
    Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(caster, nearbyEnemies, check);
    Cell::VisitAllObjects(caster, searcher, 8.0f);

    if (!nearbyEnemies.empty())
        highInterruptRisk = true;

    // Avoid empowerment if high interrupt risk
    ChrSpecializationEntry const* spec = caster->GetPrimarySpecializationEntry();
    uint32 specId = spec ? spec->ID : 0;

    if (highInterruptRisk)
    {
        // Healer spec might still need to empower for healing throughput
        if (specId != 1468)  // Not Preservation
            return false;
    }

    // Spec-specific logic
    switch (specId)
    {
        case 1467:  // Devastation
            // DPS should empower for damage, but not during movement phases
            return !caster->isMoving();

        case 1468:  // Preservation
            // Healer should empower for throughput unless emergency
            if (Group* group = caster->GetGroup())
            {
                uint32 criticalCount = 0;
                for (auto const& member : group->GetMemberSlots())
                {
                    if (Player* player = ObjectAccessor::FindPlayer(member.guid))
                    {
                        if (player->GetHealthPct() < 30.0f)
                            criticalCount++;
                    }
                }
                // Don't empower if multiple critically injured (need fast heals)
                if (criticalCount >= 2)
                    return false;
            }
            return true;

        case 1473:  // Augmentation
            // Augmentation typically uses instant casts
            return false;

        default:
            return true;
    }
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
    if (!_owner)
        return nullptr;

    Player* caster = _owner->GetBot();
    if (!caster)
        return nullptr;

    constexpr uint32 ECHO_SPELL_ID = EvokerAI::ECHO;

    ::Unit* bestTarget = nullptr;
    uint32 bestScore = 0;

    // Check group members
    if (Group* group = caster->GetGroup())
    {
        float rangeSq = 40.0f * 40.0f;

        for (auto const& member : group->GetMemberSlots())
        {
            Player* player = ObjectAccessor::FindPlayer(member.guid);
            if (!player || !player->IsAlive())
                continue;

            // Skip if out of range
            if (player->GetExactDistSq(caster) > rangeSq)
                continue;

            // Skip if already has Echo
            if (player->HasAura(ECHO_SPELL_ID, caster->GetGUID()))
                continue;

            // Skip full health targets (Echo healing wasted)
            if (player->GetHealthPct() > 95.0f)
                continue;

            uint32 score = 0;

            // Health deficit score (lower health = higher priority)
            float healthDeficit = 100.0f - player->GetHealthPct();
            score += static_cast<uint32>(healthDeficit);

            // Role priority
            ChrSpecializationEntry const* spec = player->GetPrimarySpecializationEntry();
            if (spec)
            {
                uint32 role = spec->Role;
                if (role == 1)       // Tank - highest priority (constant damage intake)
                    score += 50;
                else if (role == 0)  // DPS - medium priority
                    score += 30;
                // Healers lowest priority (self-sustaining)
            }

            // Melee bonus (more likely to take damage)
            if (player->GetVictim())
            {
                float distToTarget = std::sqrt(player->GetExactDistSq(player->GetVictim()));
                if (distToTarget < 8.0f)
                    score += 20;  // Melee range
            }

            // In combat bonus
            if (player->IsInCombat())
                score += 10;

            // Check for harmful auras (simplified - just count them)
            uint32 debuffCount = 0;
            Unit::AuraApplicationMap const& auras = player->GetAppliedAuras();
            for (auto const& auraPair : auras)
            {
                if (auraPair.second && auraPair.second->GetBase())
                {
                    SpellInfo const* spellInfo = auraPair.second->GetBase()->GetSpellInfo();
                    if (spellInfo && !spellInfo->IsPositive())
                        debuffCount++;
                }
            }
            score += debuffCount * 5;

            if (score > bestScore)
            {
                bestScore = score;
                bestTarget = player;
            }
        }
    }

    // If no group, consider self
    if (!bestTarget && caster->GetHealthPct() < 90.0f && !caster->HasAura(ECHO_SPELL_ID))
        bestTarget = caster;

    return bestTarget;
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