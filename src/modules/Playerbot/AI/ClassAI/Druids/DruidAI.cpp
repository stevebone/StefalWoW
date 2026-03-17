/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "DruidAI.h"
#include "GameTime.h"
#include "BalanceDruid.h"
#include "FeralDruid.h"
#include "GuardianDruid.h"
#include "RestorationDruid.h"
#include "../BaselineRotationManager.h"
#include "../../Combat/CombatBehaviorIntegration.h"
#include "Player.h"
#include "Log.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "Unit.h"
#include "../../../Spatial/SpatialGridQueryHelpers.h"  // PHASE 5F: Thread-safe queries

namespace Playerbot
{

// Destructor must be defined in cpp file where specialization types are complete
DruidAI::~DruidAI() = default;

DruidAI::DruidAI(Player* bot) : ClassAI(bot),    _currentForm(DruidForm::HUMANOID),
    _lastFormShift(0),
    _comboPoints(0),
    _energy(100),
    _rage(0),
    _lastSwipe(0),
    _lastThrash(0),
    _hasNaturesSwiftness(false),
    _lastBarkskin(0),
    _lastSurvivalInstincts(0),
    _lastFrenziedRegen(0),
    _lastTigersFury(0),
    _lastBerserk(0),
    _lastIncarnation(0),
    _lastCelestialAlignment(0)
{
    TC_LOG_DEBUG("playerbot", "DruidAI initialized for bot {}", bot->GetGUID().GetCounter());
}

void DruidAI::UpdateRotation(::Unit* target)
{
    if (!target || !GetBot())
        return;

    // NOTE: Baseline rotation check is now handled at the dispatch level in
    // ClassAI::OnCombatUpdate(). This method is ONLY called when the bot has
    // already chosen a specialization (level 10+ with talents).

    // Update resource tracking
    UpdateResources();

    // ========================================================================
    // COMBAT BEHAVIOR INTEGRATION - Priority-based decision making
    // ========================================================================
    auto* behaviors = GetCombatBehaviors();

    // Priority 1: Handle interrupts
    if (behaviors && behaviors->ShouldInterrupt(target))
    {
        if (HandleInterrupts(target))

            return;
    }

    // Priority 2: Handle defensives
    if (behaviors && behaviors->NeedsDefensive())
    {
        if (HandleDefensives())

            return;
    }

    // Priority 3: Check for target switching
    if (behaviors && behaviors->ShouldSwitchTarget())
    {
        if (HandleTargetSwitching(target))

            return;
    }

    // Priority 4: AoE vs Single-Target decision
    if (behaviors && behaviors->ShouldAOE())
    {
        if (HandleAoERotation(target))

            return;
    }

    // Priority 5: Use major cooldowns at optimal time
    if (behaviors && behaviors->ShouldUseCooldowns())
    {
        if (HandleOffensiveCooldowns(target))

            return;
    }

    // Priority 6: Combo Point/Energy Management (Feral/Guardian)
    ChrSpecialization spec = GetBot()->GetPrimarySpecialization();
    if (spec == ChrSpecialization::DruidFeral || spec == ChrSpecialization::DruidGuardian)
    {
        HandleComboPointManagement(target);
    }

    // Priority 7: Execute normal rotation through specialization
    ExecuteSpecializationRotation(target);
}

bool DruidAI::HandleInterrupts(::Unit* target)
{
    Unit* interruptTarget = GetCombatBehaviors()->GetInterruptTarget();
    if (!interruptTarget)
        interruptTarget = target;

    if (!interruptTarget || !interruptTarget->IsNonMeleeSpellCast(false))
        return false;

    // Skull Bash - works in Cat and Bear forms
    if ((IsInForm(DruidForm::CAT) || IsInForm(DruidForm::BEAR)) &&
        CanUseAbility(SKULL_BASH_CAT))
    {
        if (CastSpell(SKULL_BASH_CAT, interruptTarget))
        {

            TC_LOG_DEBUG("module.playerbot.ai", "Druid {} interrupted {} with Skull Bash",

                         GetBot()->GetName(), interruptTarget->GetName());

            return true;
        }
    }

    // Solar Beam - Balance spec interrupt
    if (GetBot()->GetPrimarySpecialization() == ChrSpecialization::DruidBalance && CanUseAbility(SOLAR_BEAM))
    {
        if (CastSpell(SOLAR_BEAM, interruptTarget))
        {

            TC_LOG_DEBUG("module.playerbot.ai", "Druid {} interrupted {} with Solar Beam",

                         GetBot()->GetName(), interruptTarget->GetName());

            return true;
        }
    }

    // Typhoon - knockback interrupt
    float rangeSq_typhoon = 15.0f * 15.0f; // 225.0f
    if (GetBot()->GetExactDistSq(interruptTarget) <= rangeSq_typhoon && CanUseAbility(TYPHOON))
    {
        if (CastSpell(TYPHOON, interruptTarget))
        {

            TC_LOG_DEBUG("module.playerbot.ai", "Druid {} interrupted {} with Typhoon",

                         GetBot()->GetName(), interruptTarget->GetName());

            return true;
        }
    }

    // Mighty Bash - stun interrupt
    float rangeSq_bash = 5.0f * 5.0f; // 25.0f
    if (GetBot()->GetExactDistSq(interruptTarget) <= rangeSq_bash && CanUseAbility(MIGHTY_BASH))
    {
        if (CastSpell(MIGHTY_BASH, interruptTarget))
        {

            TC_LOG_DEBUG("module.playerbot.ai", "Druid {} interrupted {} with Mighty Bash",

                         GetBot()->GetName(), interruptTarget->GetName());

            return true;
        }
    }

    return false;
}

bool DruidAI::HandleDefensives()
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    float healthPercent = bot->GetHealthPct();
    uint32 currentTime = GameTime::GetGameTimeMS();

    // Survival Instincts - critical health
    if (healthPercent < 30.0f &&
        currentTime > _lastSurvivalInstincts + 180000 &&
        CanUseAbility(SURVIVAL_INSTINCTS))
    {
        if (CastSpell(SURVIVAL_INSTINCTS))
        {

            _lastSurvivalInstincts = currentTime;

            TC_LOG_DEBUG("module.playerbot.ai", "Druid {} activated Survival Instincts at {}% health",

                         bot->GetName(), healthPercent);

            return true;
        }
    }    // Barkskin - moderate damage reduction
    if (healthPercent < 50.0f &&
        currentTime > _lastBarkskin + 60000 &&
        CanUseAbility(BARKSKIN))
    {
        if (CastSpell(BARKSKIN))
        {

            _lastBarkskin = currentTime;

            TC_LOG_DEBUG("module.playerbot.ai", "Druid {} activated Barkskin at {}% health",
            bot->GetName(), healthPercent);

            return true;
        }
    }

    // Frenzied Regeneration - Guardian healing
    if (bot->GetPrimarySpecialization() == ChrSpecialization::DruidGuardian &&        healthPercent < 60.0f &&
        currentTime > _lastFrenziedRegen + 30000 &&
        CanUseAbility(FRENZIED_REGENERATION))
    {
        if (CastSpell(FRENZIED_REGENERATION))
        {

            _lastFrenziedRegen = currentTime;

            TC_LOG_DEBUG("module.playerbot.ai", "Druid {} activated Frenzied Regeneration",

                         bot->GetName());

            return true;
        }
    }

    // Ironbark - Restoration defensive for allies
    if (bot->GetPrimarySpecialization() == ChrSpecialization::DruidRestoration)    {
        Unit* lowestAlly = GetLowestHealthAlly(40.0f);
        if (lowestAlly && lowestAlly->GetHealthPct() < 40.0f &&

            CanUseAbility(IRONBARK))
        {

            if (CastSpell(IRONBARK, lowestAlly))

            {

                TC_LOG_DEBUG("module.playerbot.ai", "Druid {} cast Ironbark on {}",

                             bot->GetName(), lowestAlly->GetName());

                return true;

            }
        }
    }

    // Cenarion Ward - preemptive defense
    if (bot->GetPrimarySpecialization() == ChrSpecialization::DruidRestoration &&        healthPercent < 70.0f &&
        CanUseAbility(CENARION_WARD))
    {
        if (CastSpell(CENARION_WARD, bot))
        {

            TC_LOG_DEBUG("module.playerbot.ai", "Druid {} activated Cenarion Ward",

                         bot->GetName());

            return true;
        }
    }

    return false;
}

bool DruidAI::HandleTargetSwitching(::Unit*& target)
{
    Unit* priorityTarget = GetCombatBehaviors()->GetPriorityTarget();
    if (priorityTarget && priorityTarget != target)
    {
        OnTargetChanged(priorityTarget);
        target = priorityTarget;
        TC_LOG_DEBUG("module.playerbot.ai", "Druid {} switching target to {}",

                     GetBot()->GetName(), priorityTarget->GetName());
        return true;
    }
    return false;
}

bool DruidAI::HandleAoERotation(::Unit* target)
{
    if (!target)
        return false;

    uint32 currentTime = GameTime::GetGameTimeMS();
    ChrSpecialization spec = GetBot()->GetPrimarySpecialization();

    switch (spec)
    {
        case ChrSpecialization::DruidFeral:
        {
            // Ensure we're in Cat Form for Feral AoE
    if (!IsInForm(DruidForm::CAT))

            {

                if (ShiftToForm(DruidForm::CAT))

                    return true;

            }

            // Primal Wrath - combo point AoE finisher
    if (_comboPoints >= 4 && CanUseAbility(PRIMAL_WRATH))

            {

                if (CastSpell(PRIMAL_WRATH, target))

                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Druid {} using Primal Wrath for AoE",

                                 GetBot()->GetName());

                    return true;

                }

            }

            // Thrash - AoE bleed
    if (currentTime > _lastThrash + 6000 && CanUseAbility(THRASH_CAT))

            {

                if (CastSpell(THRASH_CAT, target))

                {

                    _lastThrash = currentTime;

                    TC_LOG_DEBUG("module.playerbot.ai", "Druid {} using Thrash for AoE",

                                 GetBot()->GetName());

                    return true;

                }

            }

            // Swipe - AoE builder
    if (currentTime > _lastSwipe + 3000 && CanUseAbility(SWIPE_CAT))

            {

                if (CastSpell(SWIPE_CAT, target))

                {

                    _lastSwipe = currentTime;

                    TC_LOG_DEBUG("module.playerbot.ai", "Druid {} using Swipe for AoE",

                                 GetBot()->GetName());

                    return true;

                }

            }

            break;
        }

        case ChrSpecialization::DruidGuardian:
        {
            // Ensure we're in Bear Form for Guardian AoE
    if (!IsInForm(DruidForm::BEAR))

            {

                if (ShiftToForm(DruidForm::BEAR))

                    return true;

            }

            // Thrash - primary AoE threat
    if (currentTime > _lastThrash + 6000 && CanUseAbility(THRASH_BEAR))

            {

                if (CastSpell(THRASH_BEAR, target))

                {

                    _lastThrash = currentTime;

                    TC_LOG_DEBUG("module.playerbot.ai", "Druid {} using Thrash for AoE threat",

                                 GetBot()->GetName());

                    return true;

                }

            }

            // Swipe - AoE damage
    if (currentTime > _lastSwipe + 3000 && CanUseAbility(SWIPE_BEAR))

            {

                if (CastSpell(SWIPE_BEAR, target))

                {

                    _lastSwipe = currentTime;

                    TC_LOG_DEBUG("module.playerbot.ai", "Druid {} using Swipe for AoE",

                                 GetBot()->GetName());

                    return true;

                }

            }

            break;
        }

        case ChrSpecialization::DruidBalance:
        {
            // Starfall - major AoE
    if (CanUseAbility(STARFALL))

            {

                if (CastSpell(STARFALL, target))

                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Druid {} using Starfall for AoE",

                                 GetBot()->GetName());

                    return true;

                }

            }

            // NOTE: Lunar Strike was removed in WoW 11.x (merged with Starfire)
            // Use Starfire for cleave damage instead
            if (CanUseAbility(STARFIRE))

            {

                if (CastSpell(STARFIRE, target))

                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Druid {} using Starfire for cleave",

                                 GetBot()->GetName());

                    return true;

                }

            }

            // Sunfire - spread DoT
    if (CanUseAbility(SUNFIRE))

            {

                if (CastSpell(SUNFIRE, target))

                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Druid {} spreading Sunfire",

                                 GetBot()->GetName());

                    return true;

                }

            }

            break;
        }

        case ChrSpecialization::DruidRestoration:
        {
            // Wild Growth - AoE heal
    if (CanUseAbility(WILD_GROWTH))

            {

                Unit* healTarget = GetLowestHealthAlly(40.0f);

                if (healTarget)

                {

                    if (CastSpell(WILD_GROWTH, healTarget))

                    {

                        TC_LOG_DEBUG("module.playerbot.ai", "Druid {} using Wild Growth",

                                     GetBot()->GetName());

                        return true;

                    }

                }

            }

            // Efflorescence - ground AoE heal
    if (CanUseAbility(EFFLORESCENCE))

            {

                if (CastSpell(EFFLORESCENCE, target))

                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Druid {} placing Efflorescence",

                                 GetBot()->GetName());

                    return true;

                }

            }

            break;
        }
    }

    return false;
}

bool DruidAI::HandleOffensiveCooldowns(::Unit* target)
{
    if (!target)
        return false;

    uint32 currentTime = GameTime::GetGameTimeMS();
    ChrSpecialization spec = GetBot()->GetPrimarySpecialization();

    switch (spec)
    {
        case ChrSpecialization::DruidFeral:
        {
            // Tiger's Fury - energy and damage boost
    if (currentTime > _lastTigersFury + 30000 &&

                _energy < 40 &&

                CanUseAbility(TIGERS_FURY))

            {

                if (CastSpell(TIGERS_FURY))

                {

                    _lastTigersFury = currentTime;

                    TC_LOG_DEBUG("module.playerbot.ai", "Druid {} activated Tiger's Fury",

                                 GetBot()->GetName());

                    return true;

                }

            }

            // Berserk - major DPS cooldown
    if (currentTime > _lastBerserk + 180000 &&

                CanUseAbility(BERSERK_CAT))

            {

                if (CastSpell(BERSERK_CAT))

                {

                    _lastBerserk = currentTime;

                    TC_LOG_DEBUG("module.playerbot.ai", "Druid {} activated Berserk",

                                 GetBot()->GetName());

                    return true;

                }

            }

            // Incarnation: King of the Jungle
    if (currentTime > _lastIncarnation + 180000 &&

                CanUseAbility(INCARNATION_KING))

            {

                if (CastSpell(INCARNATION_KING))

                {

                    _lastIncarnation = currentTime;

                    TC_LOG_DEBUG("module.playerbot.ai", "Druid {} activated Incarnation: King of the Jungle",

                                 GetBot()->GetName());

                    return true;

                }

            }

            break;
        }

        case ChrSpecialization::DruidGuardian:
        {
            // Berserk - rage generation and defense
    if (currentTime > _lastBerserk + 180000 &&

                CanUseAbility(BERSERK_BEAR))

            {

                if (CastSpell(BERSERK_BEAR))

                {

                    _lastBerserk = currentTime;

                    TC_LOG_DEBUG("module.playerbot.ai", "Druid {} activated Berserk (Bear)",

                                 GetBot()->GetName());

                    return true;

                }

            }

            // Incarnation: Guardian of Ursoc
    if (currentTime > _lastIncarnation + 180000 &&

                CanUseAbility(INCARNATION_GUARDIAN))

            {

                if (CastSpell(INCARNATION_GUARDIAN))

                {

                    _lastIncarnation = currentTime;

                    TC_LOG_DEBUG("module.playerbot.ai", "Druid {} activated Incarnation: Guardian of Ursoc",

                                 GetBot()->GetName());

                    return true;

                }

            }

            break;
        }

        case ChrSpecialization::DruidBalance:
        {
            // Celestial Alignment - major DPS window
    if (currentTime > _lastCelestialAlignment + 180000 &&

                CanUseAbility(CELESTIAL_ALIGNMENT))

            {

                if (CastSpell(CELESTIAL_ALIGNMENT))

                {

                    _lastCelestialAlignment = currentTime;

                    TC_LOG_DEBUG("module.playerbot.ai", "Druid {} activated Celestial Alignment",

                                 GetBot()->GetName());

                    return true;

                }

            }

            // Incarnation: Chosen of Elune
    if (currentTime > _lastIncarnation + 180000 &&

                CanUseAbility(INCARNATION_BALANCE))

            {

                if (CastSpell(INCARNATION_BALANCE))

                {

                    _lastIncarnation = currentTime;

                    TC_LOG_DEBUG("module.playerbot.ai", "Druid {} activated Incarnation: Chosen of Elune",

                                 GetBot()->GetName());

                    return true;

                }

            }

            break;
        }

        case ChrSpecialization::DruidRestoration:
        {
            // Tranquility - major raid heal

            Unit* lowestAlly = GetLowestHealthAlly(40.0f);

            if (lowestAlly && lowestAlly->GetHealthPct() < 30.0f &&

                CanUseAbility(TRANQUILITY))

            {

                if (CastSpell(TRANQUILITY))

                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Druid {} channeling Tranquility",

                                 GetBot()->GetName());

                    return true;

                }

            }

            // Incarnation: Tree of Life
    if (currentTime > _lastIncarnation + 180000 &&

                CanUseAbility(INCARNATION_TREE))

            {

                if (CastSpell(INCARNATION_TREE))

                {

                    _lastIncarnation = currentTime;

                    TC_LOG_DEBUG("module.playerbot.ai", "Druid {} activated Incarnation: Tree of Life",

                                 GetBot()->GetName());

                    return true;

                }

            }

            break;
        }
    }

    return false;
}

void DruidAI::HandleComboPointManagement(::Unit* target)
{
    if (!target)
        return;

    // Only relevant for Feral spec
    if (GetBot()->GetPrimarySpecialization() != ChrSpecialization::DruidFeral)
        return;

    // Ensure we're in Cat Form
    if (!IsInForm(DruidForm::CAT))
    {
        ShiftToForm(DruidForm::CAT);
        return;
    }

    // At max combo points, use a finisher
    if (_comboPoints >= 5)
    {
        // Rip - maintain bleed
    if (!HasAura(RIP, target) && CanUseAbility(RIP))
        {

            if (CastSpell(RIP, target))

            {

                TC_LOG_DEBUG("module.playerbot.ai", "Druid {} applied Rip with {} combo points",

                             GetBot()->GetName(), _comboPoints);

                return;

            }
        }

        // Savage Roar - maintain buff
    if (!HasAura(SAVAGE_ROAR) && CanUseAbility(SAVAGE_ROAR))
        {

            if (CastSpell(SAVAGE_ROAR))

            {

                TC_LOG_DEBUG("module.playerbot.ai", "Druid {} refreshed Savage Roar",

                             GetBot()->GetName());

                return;

            }
        }

        // Ferocious Bite - dump combo points
    if (CanUseAbility(FEROCIOUS_BITE))
        {

            if (CastSpell(FEROCIOUS_BITE, target))

            {

                TC_LOG_DEBUG("module.playerbot.ai", "Druid {} used Ferocious Bite",

                             GetBot()->GetName());

                return;

            }
        }
    }

    // Build combo points
    if (_comboPoints < 5)
    {
        // Rake - maintain bleed and build CP
    if (!HasAura(RAKE, target) && CanUseAbility(RAKE))
        {

            if (CastSpell(RAKE, target))

            {

                TC_LOG_DEBUG("module.playerbot.ai", "Druid {} applied Rake",

                             GetBot()->GetName());

                return;

            }
        }

        // Shred - primary builder from behind
    if (CanUseAbility(SHRED))
        {

            if (CastSpell(SHRED, target))

            {

                TC_LOG_DEBUG("module.playerbot.ai", "Druid {} used Shred",

                             GetBot()->GetName());

                return;

            }
        }
    }
}

void DruidAI::ExecuteSpecializationRotation(::Unit* target)
{
    if (!target)
        return;

    // Execute basic rotation based on spec
    ChrSpecialization spec = GetBot()->GetPrimarySpecialization();
    switch (spec)
    {
        case ChrSpecialization::DruidFeral:
        {
            // Ensure Cat Form
    if (!IsInForm(DruidForm::CAT))

            {

                ShiftToForm(DruidForm::CAT);

                return;

            }

            // Basic Feral rotation
    if (!HasAura(RAKE, target) && CanUseAbility(RAKE))

            {

                CastSpell(RAKE, target);

                return;

            }


            if (_comboPoints >= 5)

            {

                if (!HasAura(RIP, target) && CanUseAbility(RIP))

                {

                    CastSpell(RIP, target);

                    return;

                }

                if (CanUseAbility(FEROCIOUS_BITE))

                {

                    CastSpell(FEROCIOUS_BITE, target);

                    return;

                }

            }


            if (CanUseAbility(SHRED))

            {

                CastSpell(SHRED, target);

                return;

            }

            break;
        }

        case ChrSpecialization::DruidGuardian:
        {
            // Ensure Bear Form
    if (!IsInForm(DruidForm::BEAR))

            {

                ShiftToForm(DruidForm::BEAR);

                return;

            }

            // Basic Guardian rotation
    if (CanUseAbility(MANGLE_BEAR))

            {

                CastSpell(MANGLE_BEAR, target);

                return;

            }


            if (!HasAura(THRASH_BEAR, target) && CanUseAbility(THRASH_BEAR))

            {

                CastSpell(THRASH_BEAR, target);

                return;

            }


            if (CanUseAbility(MAUL))

            {

                CastSpell(MAUL, target);

                return;

            }


            if (CanUseAbility(SWIPE_BEAR))

            {

                CastSpell(SWIPE_BEAR, target);

                return;

            }

            break;
        }

        case ChrSpecialization::DruidBalance:
        {
            // Ensure Moonkin Form if available
    if (!IsInForm(DruidForm::MOONKIN) && CanUseAbility(MOONKIN_FORM))

            {

                ShiftToForm(DruidForm::MOONKIN);

                return;

            }

            // Basic Balance rotation
    if (!HasAura(MOONFIRE, target) && CanUseAbility(MOONFIRE))

            {

                CastSpell(MOONFIRE, target);

                return;

            }


            if (!HasAura(SUNFIRE, target) && CanUseAbility(SUNFIRE))

            {

                CastSpell(SUNFIRE, target);

                return;

            }


            if (CanUseAbility(STARSURGE))

            {

                CastSpell(STARSURGE, target);

                return;

            }


            // NOTE: Solar Wrath and Lunar Strike were removed in WoW 11.x
            // They were merged with Wrath and Starfire respectively
            // Use Wrath as the primary filler
            if (CanUseAbility(WRATH))

            {

                CastSpell(WRATH, target);

                return;

            }

            break;
        }

        case ChrSpecialization::DruidRestoration:
        {
            // Basic Restoration rotation - heal allies

            Unit* healTarget = GetLowestHealthAlly(40.0f);

            if (healTarget)

            {

                if (healTarget->GetHealthPct() < 30.0f && CanUseAbility(SWIFTMEND))

                {

                    CastSpell(SWIFTMEND, healTarget);

                    return;

                }


                if (!HasAura(REJUVENATION, healTarget) && CanUseAbility(REJUVENATION))

                {

                    CastSpell(REJUVENATION, healTarget);

                    return;

                }


                if (!HasAura(LIFEBLOOM, healTarget) && CanUseAbility(LIFEBLOOM))

                {

                    CastSpell(LIFEBLOOM, healTarget);

                    return;

                }


                if (healTarget->GetHealthPct() < 50.0f && CanUseAbility(REGROWTH))

                {

                    CastSpell(REGROWTH, healTarget);

                    return;

                }


                if (healTarget->GetHealthPct() < 70.0f && CanUseAbility(HEALING_TOUCH))

                {

                    CastSpell(HEALING_TOUCH, healTarget);

                    return;

                }

            }

            // If no healing needed, do some damage
    if (!HasAura(MOONFIRE, target) && CanUseAbility(MOONFIRE))

            {

                CastSpell(MOONFIRE, target);

                return;

            }


            if (CanUseAbility(WRATH))

            {

                CastSpell(WRATH, target);

                return;

            }

            break;
        }
    }
}

void DruidAI::UpdateBuffs()
{
    Player* bot = GetBot();
    if (!bot)
        return;

    // NOTE: Baseline buff check is now handled at the dispatch level.
    // This method is only called for level 10+ bots with talents.

    // Spec-specific buff logic based on current specialization
    ChrSpecialization spec = bot->GetPrimarySpecialization();

    switch (spec)
    {
        case ChrSpecialization::DruidBalance:
            // Balance druids should be in Moonkin Form for combat
            if (!bot->IsInCombat())
            {
                // Out of combat: ensure Moonkin Form for DPS
                if (!IsInForm(DruidForm::MOONKIN) && CanUseAbility(MOONKIN_FORM))
                    ShiftToForm(DruidForm::MOONKIN);
            }
            // Apply Mark of the Wild if not present (universal druid buff)
            if (!bot->HasAura(MARK_OF_THE_WILD) && CanUseAbility(MARK_OF_THE_WILD))
                CastSpell(MARK_OF_THE_WILD, bot);
            break;

        case ChrSpecialization::DruidFeral:
            // Feral druids should be in Cat Form for melee DPS
            if (!bot->IsInCombat())
            {
                // Out of combat: ensure Cat Form ready
                if (!IsInForm(DruidForm::CAT) && CanUseAbility(CAT_FORM))
                    ShiftToForm(DruidForm::CAT);
            }
            // Apply Mark of the Wild if not present
            if (!bot->HasAura(MARK_OF_THE_WILD) && CanUseAbility(MARK_OF_THE_WILD))
                CastSpell(MARK_OF_THE_WILD, bot);
            break;

        case ChrSpecialization::DruidGuardian:
            // Guardian druids should be in Bear Form for tanking
            if (!bot->IsInCombat())
            {
                // Out of combat: ensure Bear Form ready for tanking
                if (!IsInForm(DruidForm::BEAR) && CanUseAbility(BEAR_FORM))
                    ShiftToForm(DruidForm::BEAR);
            }
            // Apply Mark of the Wild if not present
            if (!bot->HasAura(MARK_OF_THE_WILD) && CanUseAbility(MARK_OF_THE_WILD))
                CastSpell(MARK_OF_THE_WILD, bot);
            break;

        case ChrSpecialization::DruidRestoration:
            // Restoration druids stay in caster form for healing
            // They might use Tree of Life form during heavy healing phases (handled in rotation)
            // Apply Mark of the Wild if not present
            if (!bot->HasAura(MARK_OF_THE_WILD) && CanUseAbility(MARK_OF_THE_WILD))
                CastSpell(MARK_OF_THE_WILD, bot);
            break;

        default:
            // No specialization or unknown - apply basic buffs
            if (!bot->HasAura(MARK_OF_THE_WILD) && CanUseAbility(MARK_OF_THE_WILD))
                CastSpell(MARK_OF_THE_WILD, bot);
            break;
    }
}

void DruidAI::UpdateCooldowns(uint32 diff)
{
    // Cooldown tracking is handled automatically by TrinityCore spell system
    // No manual tracking needed here
}

bool DruidAI::CanUseAbility(uint32 spellId)
{
    if (!GetBot() || !IsSpellReady(spellId))
        return false;

    if (!HasEnoughResource(spellId))
        return false;

    // Check form requirements
    const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(spellId, GetBot()->GetMap()->GetDifficultyID());
    if (spellInfo)
    {        // Check if spell requires specific form
    if (spellInfo->Stances)
        {

            bool canCastInForm = false;

            uint32 currentFormMask = 0;


            if (IsInForm(DruidForm::CAT))

                currentFormMask = 1 << 1; // Cat form bit

            else if (IsInForm(DruidForm::BEAR))

                currentFormMask = 1 << 0; // Bear form bit

            else if (IsInForm(DruidForm::MOONKIN))

                currentFormMask = 1 << 4; // Moonkin form bit

            else if (IsInForm(DruidForm::TREE_OF_LIFE))

                currentFormMask = 1 << 5; // Tree form bit
    if (spellInfo->Stances & currentFormMask)

                canCastInForm = true;


            if (!canCastInForm && spellInfo->StancesNot == 0)

                return false; // Required form not active
        }
    }

    return true;
}

void DruidAI::OnCombatStart(::Unit* target)
{
    if (!target || !GetBot())
        return;

    TC_LOG_DEBUG("playerbot", "DruidAI {} entering combat with {}",

                 GetBot()->GetName(), target->GetName());

    _inCombat = true;
    _currentTarget = target->GetGUID();
    _combatTime = 0;

    // Update resources
    UpdateResources();
}

void DruidAI::OnCombatEnd()
{
    _inCombat = false;
    _currentTarget = ObjectGuid::Empty;
    _combatTime = 0;

    TC_LOG_DEBUG("playerbot", "DruidAI {} leaving combat", GetBot()->GetName());
}

bool DruidAI::HasEnoughResource(uint32 spellId)
{
    if (!GetBot())
        return false;

    // Get spell info
    const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(spellId, GetBot()->GetMap()->GetDifficultyID());
    if (!spellInfo)
        return false;

    // Check power cost based on current form
    Powers powerType = GetBot()->GetPowerType();
    auto powerCosts = spellInfo->CalcPowerCost(GetBot(), spellInfo->GetSchoolMask());

    // Find the cost for the current power type
    int32 cost = 0;
    for (const auto& powerCost : powerCosts)
    {
        if (powerCost.Power == powerType)
        {

            cost = powerCost.Amount;

            break;
        }
    }

    if (cost <= 0)
        return true; // No cost

    return GetBot()->GetPower(powerType) >= uint32(cost);
}

void DruidAI::ConsumeResource(uint32 spellId)
{
    if (!GetBot())
        return;

    // Resource consumption is handled automatically by TrinityCore spell system
}

Position DruidAI::GetOptimalPosition(::Unit* target)
{
    if (!GetBot() || !target)
        return GetBot() ? GetBot()->GetPosition() : Position();

    // Default position handling - melee specs get close, ranged stay back
    ChrSpecialization spec = GetBot()->GetPrimarySpecialization();
    if (spec == ChrSpecialization::DruidFeral || spec == ChrSpecialization::DruidGuardian)  // Feral or Guardian - melee
    {
        // Get behind target for Feral, face target for Guardian
        return GetBot()->GetPosition();
    }

    return GetBot()->GetPosition();
}

float DruidAI::GetOptimalRange(::Unit* target)
{
    // Default range based on spec
    ChrSpecialization spec = GetBot()->GetPrimarySpecialization();
    switch (spec)
    {
        case ChrSpecialization::DruidFeral:
        case ChrSpecialization::DruidGuardian:

            return 5.0f; // Melee range
        case ChrSpecialization::DruidBalance:
        case ChrSpecialization::DruidRestoration:

            return 30.0f; // Casting range
        default:

            return 25.0f; // Safe default
    }
}

// Form management helpers
bool DruidAI::IsInForm(DruidForm form) const
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    switch (form)
    {
        case DruidForm::CAT:

            return bot->HasAura(CAT_FORM);
        case DruidForm::BEAR:

            return bot->HasAura(BEAR_FORM);
        case DruidForm::MOONKIN:

            return bot->HasAura(MOONKIN_FORM);
        case DruidForm::TREE_OF_LIFE:

            return bot->HasAura(TREE_OF_LIFE);
        case DruidForm::TRAVEL:

            return bot->HasAura(TRAVEL_FORM);
        case DruidForm::HUMANOID:

            return !bot->HasAuraType(SPELL_AURA_MOD_SHAPESHIFT);
        default:

            return false;
    }
}

bool DruidAI::ShiftToForm(DruidForm form)
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    // Already in desired form
    if (IsInForm(form))
        return false;

    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime < _lastFormShift + 1500) // GCD for form shifting
        return false;

    uint32 spellId = 0;
    switch (form)
    {
        case DruidForm::CAT:

            spellId = CAT_FORM;

            break;
        case DruidForm::BEAR:

            spellId = BEAR_FORM;

            break;
        case DruidForm::MOONKIN:

            spellId = MOONKIN_FORM;

            break;
        case DruidForm::TREE_OF_LIFE:

            spellId = TREE_OF_LIFE;
            break;

        case DruidForm::TRAVEL:
        spellId = TRAVEL_FORM;

            break;
            case DruidForm::HUMANOID:
            // Cancel current form
    if (bot->HasAuraType(SPELL_AURA_MOD_SHAPESHIFT))

            {
            bot->RemoveAurasByType(SPELL_AURA_MOD_SHAPESHIFT);

                _lastFormShift = currentTime;

                _currentForm = DruidForm::HUMANOID;

                return true;

            }

            return false;
        default:

            return false;
    }

    if (spellId && CanUseAbility(spellId))
    {
        if (CastSpell(spellId))
        {

            _lastFormShift = currentTime;

            _currentForm = form;

            return true;
        }
    }    return false;
}
DruidAI::DruidForm DruidAI::GetCurrentForm() const
{    return _currentForm;
}

bool DruidAI::CanShiftToForm(DruidForm form) const
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    switch (form)
    {
        case DruidForm::CAT:

            return bot->HasSpell(CAT_FORM);
        case DruidForm::BEAR:

            return bot->HasSpell(BEAR_FORM);
        case DruidForm::MOONKIN:

            return bot->HasSpell(MOONKIN_FORM);
        case DruidForm::TREE_OF_LIFE:

            return bot->HasSpell(TREE_OF_LIFE);
        case DruidForm::TRAVEL:

            return bot->HasSpell(TRAVEL_FORM);
        case DruidForm::HUMANOID:

            return true; // Can always shift to humanoid
        default:

            return false;
    }
}

// Resource helpers
void DruidAI::UpdateResources()
{
    Player* bot = GetBot();
    if (!bot)
        return;

    // Update based on current form
    Powers powerType = bot->GetPowerType();
    switch (powerType)
    {
        case POWER_ENERGY:

            _energy = bot->GetPower(POWER_ENERGY);
            // Combo points are stored as a separate power type

            _comboPoints = bot->GetPower(POWER_COMBO_POINTS);
            break;
        case POWER_RAGE:

            _rage = bot->GetPower(POWER_RAGE);
            break;
        case POWER_MANA:
            // Mana is tracked via GetPower directly

            break;
        default:

            break;
    }
}

bool DruidAI::HasEnoughEnergy(uint32 amount) const
{
    return _energy >= amount;
}

bool DruidAI::HasEnoughRage(uint32 amount) const
{
    return _rage >= amount;
}

bool DruidAI::HasEnoughMana(uint32 amount) const
{
    Player* bot = GetBot();
    if (!bot)
        return false;
    return bot->GetPower(POWER_MANA) >= amount;
}

uint32 DruidAI::GetComboPoints() const
{
    return _comboPoints;
}

} // namespace Playerbot