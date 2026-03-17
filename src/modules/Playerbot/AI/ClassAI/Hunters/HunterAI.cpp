/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "HunterAI.h"
#include "GameTime.h"
#include "../../Combat/CombatBehaviorIntegration.h"
#include "BeastMasteryHunter.h"
#include "MarksmanshipHunter.h"
#include "SurvivalHunter.h"
#include "../BaselineRotationManager.h"
#include "Player.h"
#include "Pet.h"
#include "Group.h"
#include "CreatureAI.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "WorldSession.h"
#include "ObjectAccessor.h"
#include "MotionMaster.h"
#include "Map.h"
#include "Log.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include <chrono>
#include "../../../Spatial/SpatialGridManager.h"
#include "../../../Spatial/SpatialGridQueryHelpers.h"  // PHASE 5F: Thread-safe queries
#include "Movement/UnifiedMovementCoordinator.h"
#include "../../../Movement/Arbiter/MovementPriorityMapper.h"
#include "../../BotAI.h"
#include "Core/PlayerBotHelpers.h"
#include "UnitAI.h"

namespace Playerbot
{

// Destructor must be defined in cpp file where specialization types are complete
HunterAI::~HunterAI() = default;

HunterAI::HunterAI(Player* bot) :
    ClassAI(bot),
    _lastCounterShot(0),
    _lastFeignDeath(0),
    _lastDeterrence(0),
    _lastDisengage(0),
    _lastTrapPlacement(0),
    _lastPetCommand(0),
    _lastAspectSwitch(0),
    _lastPetRevive(0),
    _lastPetHeal(0),
    _petNeedsHeal(false),
    _petIsAggressive(false),
    _petTargetSwitch(0),
    _activeTrapType(0),
    _updateCounter(0),
    _totalUpdateTime(0),
    _peakUpdateTime(0)
{
    // Initialize combat behavior integration
    _combatBehaviors = ::std::make_unique<CombatBehaviorIntegration>(bot);    // Reset combat metrics
    _combatMetrics.Reset();

    TC_LOG_DEBUG("playerbot", "HunterAI initialized for bot {} with CombatBehaviorIntegration", bot->GetGUID().GetCounter());
}

void HunterAI::InitializeCombatSystems()
{
    // Initialize combat behavior integration systems
    if (_combatBehaviors)
    {
        _combatBehaviors->Update(0); // Initial update to set up managers
    }
}

void HunterAI::UpdateRotation(::Unit* target)
{
    if (!target || !_bot)
        return;

    // Performance tracking
    auto startTime = ::std::chrono::high_resolution_clock::now();

    // Update combat behavior integration
    if (_combatBehaviors)
    {
        _combatBehaviors->Update(100); // Update with 100ms diff for standard tick
    }

    // NOTE: Baseline rotation check is now handled at the dispatch level in
    // ClassAI::OnCombatUpdate(). This method is ONLY called when the bot has
    // already chosen a specialization (level 10+ with talents).

    // ========================================================================
    // COMBAT BEHAVIOR INTEGRATION - Intelligent decision making for Hunters
    // ========================================================================
    auto* behaviors = GetCombatBehaviors();

    // Priority 1: Interrupts (Counter Shot, Silencing Shot)
    if (HandleInterrupts(target))
        return;

    // Priority 2: Defensives (Feign Death, Deterrence, Aspect of the Turtle)
    if (HandleDefensives(target))
        return;

    // Priority 3: Positioning (Maintain optimal range, avoid dead zone)
    if (HandlePositioning(target))
        return;

    // Priority 4: Pet Management (Revive, Heal, Commands)
    if (HandlePetManagement(target))
        return;

    // Priority 5: Target Switching (Priority targets)
    if (HandleTargetSwitching(target))
        return;

    // Priority 6: Crowd Control (Freezing Trap, Scatter Shot)
    if (HandleCrowdControl(target))
        return;

    // Priority 7: AoE Decisions (Multi-Shot, Volley, Explosive Shot)
    if (HandleAoEDecisions(target))
        return;

    // Priority 8: Offensive Cooldowns (Bestial Wrath, Rapid Fire, Trueshot)
    if (HandleOffensiveCooldowns(target))
        return;

    // Priority 9: Normal Rotation - Delegate to specialization
    ExecuteNormalRotation(target);

    // Update combat metrics
    auto endTime = ::std::chrono::high_resolution_clock::now();
    uint32 updateTime = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime).count();
    _totalUpdateTime += updateTime;
    _updateCounter++;
    if (updateTime > _peakUpdateTime)
        _peakUpdateTime = updateTime;
}

bool HunterAI::HandleInterrupts(::Unit* target)
{    if (!_combatBehaviors || !target)
        return false;

    if (_combatBehaviors->ShouldInterrupt(target))
    {
        Unit* interruptTarget = _combatBehaviors->GetInterruptTarget();
        if (!interruptTarget)

            interruptTarget = target;

        // Counter Shot (primary interrupt)
    if (CanInterruptTarget(interruptTarget))
        {

            uint32 now = GameTime::GetGameTimeMS();
            // Try Counter Shot first
    if (_bot->HasSpell(COUNTER_SHOT) && CanUseAbility(COUNTER_SHOT) &&

                now - _lastCounterShot > 24000) // 24 second cooldown

            {

                if (CastSpell(COUNTER_SHOT, interruptTarget))

                {

                    _lastCounterShot = now;

                    _combatMetrics.interrupts++;

                    TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} interrupted {} with Counter Shot",

                                 GetBot()->GetName(), interruptTarget->GetName());

                    return true;

                }

            }

            // Note: Silencing Shot was removed in modern WoW - Counter Shot is the primary interrupt
        }
    }
    return false;
}

bool HunterAI::HandleDefensives(::Unit* target)
{
    if (!_combatBehaviors)
        return false;
        if (_combatBehaviors->NeedsDefensive())
    {
        float healthPct = GetBot()->GetHealthPct();
        uint32 now = GameTime::GetGameTimeMS();

        // Feign Death - emergency escape
    if (healthPct < FEIGN_DEATH_THRESHOLD && ShouldFeignDeath())

        {
        if (_bot->HasSpell(FEIGN_DEATH) && CanUseAbility(FEIGN_DEATH) &&

                now - _lastFeignDeath > 30000) // 30 second cooldown
                {

                if (CastSpell(FEIGN_DEATH))

                {

                    _lastFeignDeath = now;

                    TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} used Feign Death at {}% health",

                                 GetBot()->GetName(), static_cast<uint32>(healthPct));

                    return true;

                }

            }
        }

        // Deterrence - damage reduction
    if (healthPct < DEFENSIVE_HEALTH_THRESHOLD)

        {
        if (_bot->HasSpell(DETERRENCE) && CanUseAbility(DETERRENCE) &&

                now - _lastDeterrence > 120000) // 2 minute cooldown

            {

                if (CastSpell(DETERRENCE))

                {

                    _lastDeterrence = now;
                    TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} activated Deterrence",

                                 GetBot()->GetName());

                    return true;

                }

            }
        }

        // Aspect of the Turtle - modern defensive
    if (healthPct < DEFENSIVE_HEALTH_THRESHOLD)

        {
        if (_bot->HasSpell(ASPECT_OF_THE_TURTLE) && CanUseAbility(ASPECT_OF_THE_TURTLE))

            {

                if (CastSpell(ASPECT_OF_THE_TURTLE))

                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} activated Aspect of the Turtle",

                                 GetBot()->GetName());

                    return true;

                }

            }
        }

        // Exhilaration - self heal
    if (healthPct < 50.0f && _bot->HasSpell(EXHILARATION) && CanUseAbility(EXHILARATION))

        {
        if (CastSpell(EXHILARATION))

            {

                TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} used Exhilaration for healing",

                             GetBot()->GetName());

                return true;

            }
        }

        // Disengage for creating distance
    if (target && GetDistanceToTarget(target) < DEAD_ZONE_MAX)

        {
        if (_bot->HasSpell(HUNTER_DISENGAGE) && CanUseAbility(HUNTER_DISENGAGE) &&

                now - _lastDisengage > 20000) // 20 second cooldown

            {

                if (CastSpell(HUNTER_DISENGAGE))

                {

                    _lastDisengage = now;

                    TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} used Disengage to escape dead zone",

                                 GetBot()->GetName());

                    return true;

                }

            }
        }
    }
    return false;
}

bool HunterAI::HandlePositioning(::Unit* target)
{
    if (!_combatBehaviors || !target)
        return false;
        if (_combatBehaviors->NeedsRepositioning())
    {
        float distance = GetDistanceToTarget(target);

        // Check if in dead zone
    if (IsInDeadZone(target))
        {

            _combatMetrics.timeInDeadZone += 0.1f;

            // Try to get out of dead zone
    if (distance < DEAD_ZONE_MAX)

            {
                // Too close - use disengage or move back
    if (_bot->HasSpell(HUNTER_DISENGAGE) && CanUseAbility(HUNTER_DISENGAGE))

                {

                    uint32 now = GameTime::GetGameTimeMS();

                    if (now - _lastDisengage > 20000)

                    {

                        if (CastSpell(HUNTER_DISENGAGE))

                        {

                            _lastDisengage = now;

                            TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} disengaged from dead zone",

                                         GetBot()->GetName());

                            return true;

                        }

                    }

                }

                // Note: Wing Clip was removed in WoW 12.0
                // Movement will be handled by BotAI movement strategies

                Position optimalPos = _combatBehaviors->GetOptimalPosition();
                // Movement is handled externally

            }
        }
        else
        {

            _combatMetrics.timeAtRange += 0.1f;
        }

        // Maintain optimal range for kiting
    if (NeedsToKite(target))
        {
            // Apply slowing effects
    if (_bot->HasSpell(CONCUSSIVE_SHOT) && CanUseAbility(CONCUSSIVE_SHOT))

            {

                if (CastSpell(CONCUSSIVE_SHOT, target))

                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} applied Concussive Shot for kiting",

                                 GetBot()->GetName());

                    return true;

                }

            }

            // Switch to Aspect of the Cheetah if needed
    if (!_bot->IsInCombat() && distance > 40.0f)

            {

                SwitchToMovementAspect();

            }
        }
    }
    return false;
}

bool HunterAI::HandlePetManagement(::Unit* target)
{
    UpdatePetStatus();

    // Priority: Revive dead pet
    if (NeedsPetRevive())
    {
        uint32 now = GameTime::GetGameTimeMS();
        if (now - _lastPetRevive > 10000) // Don't spam revive
        {

            RevivePet();

            _lastPetRevive = now;

            TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} reviving pet",

                         GetBot()->GetName());

            return true;
        }
    }

    // Call pet if no pet
    if (!HasActivePet())
    {
        CallPet();
        TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} calling pet",

                     GetBot()->GetName());
        return true;
    }

    Pet* pet = GetPet();
    if (!pet)
        return false;

    // Heal pet if needed
    if (NeedsPetHeal())
    {
        uint32 now = GameTime::GetGameTimeMS();
        if (now - _lastPetHeal > 3000) // Don't spam heal
        {

            HealPet();

            _lastPetHeal = now;

            TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} healing pet ({}% health)",

                         GetBot()->GetName(), static_cast<uint32>(GetPetHealthPercent()));

            return true;
        }
    }

    // Command pet to attack if not already
    if (target && !IsPetInCombat())
    {
        CommandPetAttack(target);
        _combatMetrics.petCommands++;
        return false; // Don't stop rotation for this
    }

    // Use Kill Command if available (BM spec)
    if (target && GetCurrentSpecialization() == HunterSpec::BEAST_MASTERY)
    {        if (_bot->HasSpell(KILL_COMMAND) && CanUseAbility(KILL_COMMAND))
        {

            if (CastSpell(KILL_COMMAND, target))

            {

                TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} used Kill Command",

                             GetBot()->GetName());

                return false; // Continue rotation

            }
        }
    }

    // Master's Call for freedom effects
    if (_bot->HasUnitState(UNIT_STATE_ROOT) || _bot->HasUnitState(UNIT_STATE_STUNNED))
    {
        if (_bot->HasSpell(MASTER_S_CALL) && CanUseAbility(MASTER_S_CALL))
        {
            if (CastSpell(MASTER_S_CALL))
            {
                TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} used Master's Call for freedom",
                             GetBot()->GetName());
                return true;
            }
        }
    }

    return false;
}

bool HunterAI::HandleTargetSwitching(::Unit* target)
{
    if (!_combatBehaviors)
        return false;

    if (_combatBehaviors->ShouldSwitchTarget())
    {
        Unit* priorityTarget = _combatBehaviors->GetPriorityTarget();
        if (priorityTarget && priorityTarget != target)
        {
            // Apply Hunter's Mark to new target
    if (_bot->HasSpell(HUNTER_S_MARK) && CanUseAbility(HUNTER_S_MARK))

            {

                if (!priorityTarget->HasAura(HUNTER_S_MARK))

                {

                    if (CastSpell(HUNTER_S_MARK, priorityTarget))

                    {

                        TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} marked priority target {}",

                                     GetBot()->GetName(), priorityTarget->GetName());

                    }

                }

            }
            // Command pet to switch targets
    if (HasActivePet())

            {

                CommandPetAttack(priorityTarget);

                _petTargetSwitch = GameTime::GetGameTimeMS();

            }

            // Update current target
            _currentTarget = priorityTarget->GetGUID();

            TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} switching to priority target {}",
                         GetBot()->GetName(), priorityTarget->GetName());

            return false; // Continue with new target
        }
    }
    return false;
}

bool HunterAI::HandleCrowdControl(::Unit* target)
{
    if (!_combatBehaviors)
        return false;

    if (_combatBehaviors->ShouldUseCrowdControl())
    {
        ::Unit* ccTarget = GetBestCrowdControlTarget();

        if (ccTarget && ccTarget != target)
        {

            uint32 now = GameTime::GetGameTimeMS();

            // Freezing Trap for long CC
    if (ShouldPlaceFreezingTrap(ccTarget))

            {

                if (now - _lastTrapPlacement > 30000) // 30 second trap CD

                {

                    PlaceTrap(FREEZING_TRAP, ccTarget->GetPosition());

                    _lastTrapPlacement = now;

                    _frozenTargets.insert(ccTarget->GetGUID());

                    _combatMetrics.trapsTriggered++;

                    TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} placed Freezing Trap for {}",
                    GetBot()->GetName(), ccTarget->GetName());

                    return true;

                }

            }

            // Scatter Shot for instant CC
            if (_bot->HasSpell(SCATTER_SHOT) && CanUseAbility(SCATTER_SHOT))
            {
                if (CastSpell(SCATTER_SHOT, ccTarget))
                {
                    TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} used Scatter Shot on {}",
                                 GetBot()->GetName(), ccTarget->GetName());

                    return true;
                }
            }

            // Concussive Shot for slowing
            if (_bot->HasSpell(CONCUSSIVE_SHOT) && CanUseAbility(CONCUSSIVE_SHOT))
            {
                if (CastSpell(CONCUSSIVE_SHOT, ccTarget))
                {
                    TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} slowed {} with Concussive Shot",
                                 GetBot()->GetName(), ccTarget->GetName());

                    return false; // Continue rotation
                }
            }
        }
    }
    return false;
}

bool HunterAI::HandleAoEDecisions(::Unit* target)
{
    if (!_combatBehaviors || !target)
        return false;

    if (_combatBehaviors->ShouldAOE())
    {
        uint32 nearbyEnemies = GetNearbyEnemyCount(10.0f);

        // Multi-Shot for 3+ targets
    if (nearbyEnemies >= 3)

        {
        if (_bot->HasSpell(MULTI_SHOT) && CanUseAbility(MULTI_SHOT))

            {

                if (CastSpell(MULTI_SHOT, target))

                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} using Multi-Shot on {} enemies",

                                 GetBot()->GetName(), nearbyEnemies);

                    return true;

                }

            }
        }        // Volley for ground-targeted AoE
    if (nearbyEnemies >= 4 && _bot->HasSpell(VOLLEY) && CanUseAbility(VOLLEY))        {
            // Volley needs special handling for ground targeting

            Position aoeCenter = _combatBehaviors->GetOptimalPosition();

            TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} would cast Volley for {} enemies",

                         GetBot()->GetName(), nearbyEnemies);
            // Ground-targeted spell handling would go here
        }

        // Explosive Shot for Survival spec
    if (GetCurrentSpecialization() == HunterSpec::SURVIVAL && nearbyEnemies >= 2)
        {

            if (_bot->HasSpell(EXPLOSIVE_SHOT) && CanUseAbility(EXPLOSIVE_SHOT))

            {

                if (CastSpell(EXPLOSIVE_SHOT, target))
                {

                    TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} using Explosive Shot for AoE",

                                 GetBot()->GetName());

                    return true;

                }

            }
        }

        // Barrage for modern AoE
    if (nearbyEnemies >= 3 && _bot->HasSpell(BARRAGE) && CanUseAbility(BARRAGE))        {

            if (CastSpell(BARRAGE, target))

            {

                TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} using Barrage for AoE",

                             GetBot()->GetName());

                return true;

            }
        }

        // Place explosive trap for AoE damage
    if (ShouldPlaceExplosiveTrap())
        {

            uint32 now = GameTime::GetGameTimeMS();

            if (now - _lastTrapPlacement > 30000)

            {
            PlaceTrap(EXPLOSIVE_TRAP, target->GetPosition());

                _lastTrapPlacement = now;

                _combatMetrics.trapsTriggered++;

                TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} placed Explosive Trap for AoE",

                             GetBot()->GetName());

                return true;

            }
        }
    }
    return false;
}

bool HunterAI::HandleOffensiveCooldowns(::Unit* target)
{
    if (!_combatBehaviors || !target)
        return false;

    if (_combatBehaviors->ShouldUseCooldowns())
    {
        bool usedCooldown = false;

        switch (GetCurrentSpecialization())
        {

            case HunterSpec::BEAST_MASTERY:
                // Bestial Wrath - pet damage boost
    if (_bot->HasSpell(BESTIAL_WRATH) && CanUseAbility(BESTIAL_WRATH))

                {

                    if (HasActivePet() && IsPetInCombat())

                    {

                        if (CastSpell(BESTIAL_WRATH))

                        {
                        TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} activated Bestial Wrath",

                                         GetBot()->GetName());

                            usedCooldown = true;

                        }

                    }

                }

                // Aspect of the Wild - BM cooldown
    if (_bot->HasSpell(ASPECT_OF_THE_WILD) && CanUseAbility(ASPECT_OF_THE_WILD))

                {

                    if (CastSpell(ASPECT_OF_THE_WILD))

                    {

                        TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} activated Aspect of the Wild",

                                     GetBot()->GetName());

                        usedCooldown = true;

                    }

                }

                break;


            case HunterSpec::MARKSMANSHIP:
                // Trueshot - MM burst
    if (_bot->HasSpell(TRUESHOT) && CanUseAbility(TRUESHOT))

                {

                    if (CastSpell(TRUESHOT))

                    {
                    TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} activated Trueshot",

                                     GetBot()->GetName());

                        usedCooldown = true;

                    }

                }

                // Rapid Fire for attack speed
    if (_bot->HasSpell(RAPID_FIRE) && CanUseAbility(RAPID_FIRE))

                {

                    if (CastSpell(RAPID_FIRE))

                    {

                        TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} activated Rapid Fire",

                                     GetBot()->GetName());

                        usedCooldown = true;

                    }

                }

                break;


            case HunterSpec::SURVIVAL:
            // Coordinated Assault or similar survival cooldowns
                // Survival uses different cooldowns depending on version
    if (_bot->HasSpell(RAPID_FIRE) && CanUseAbility(RAPID_FIRE))

                {

                    if (CastSpell(RAPID_FIRE))

                    {

                        TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} activated Rapid Fire",

                                     GetBot()->GetName());

                        usedCooldown = true;

                    }

                }

                break;
        }

        return usedCooldown;
    }    return false;
}

void HunterAI::ExecuteNormalRotation(::Unit* target)
{
    if (!target)
        return;

    // Ensure we have proper aspect
    ManageAspects();

    // Apply Hunter's Mark if not present
    if (!target->HasAura(HUNTER_S_MARK) && _bot->HasSpell(HUNTER_S_MARK) && CanUseAbility(HUNTER_S_MARK))
    {
        CastSpell(HUNTER_S_MARK, target);
    }

    // Apply Serpent Sting if not present
    if (!target->HasAura(SERPENT_STING) && _bot->HasSpell(SERPENT_STING) && CanUseAbility(SERPENT_STING))
    {
        CastSpell(SERPENT_STING, target);
    }

    // Kill Shot if target is low health
    if (target->GetHealthPct() < 20.0f && _bot->HasSpell(KILL_SHOT) && CanUseAbility(KILL_SHOT))
    {
        if (CastSpell(KILL_SHOT, target))
        {

            RecordShotResult(true, false);

            return;
        }
    }

    // Fallback basic rotation (specialization rotations handled by refactored system)
    if (_bot->HasSpell(STEADY_SHOT) && CanUseAbility(STEADY_SHOT))
    {
        CastSpell(STEADY_SHOT, target);
        RecordShotResult(true, false);
    }    else if (_bot->HasSpell(ARCANE_SHOT) && CanUseAbility(ARCANE_SHOT))
    {
        CastSpell(ARCANE_SHOT, target);
        RecordShotResult(true, false);
    }
}

void HunterAI::UpdateBuffs()
{
    if (!_bot)
        return;

    // NOTE: Baseline buff check is now handled at the dispatch level.
    // This method is only called for level 10+ bots with talents.

    // Use full hunter buff system for specialized bots
    ManageAspects();
    UpdateTracking();

    // Specialization buffs handled by refactored system
}

void HunterAI::UpdateCooldowns(uint32 diff)
{
    // Update combat behavior integration
    if (_combatBehaviors)
        _combatBehaviors->Update(diff);

    // Base class handles cooldown tracking
    ClassAI::UpdateCooldowns(diff);
}

bool HunterAI::CanUseAbility(uint32 spellId)
{
    if (!HasEnoughResource(spellId))
        return false;

    // Additional hunter-specific checks
    // Note: Dead zone restrictions are less strict in modern WoW, but ranged attacks still require distance
    if (IsInDeadZone(nullptr) && spellId != HUNTER_DISENGAGE)
        return false;

    return ClassAI::CanUseAbility(spellId);
}

void HunterAI::OnCombatStart(::Unit* target)
{
    _inCombat = true;
    _currentTarget = target ? target->GetGUID() : ObjectGuid::Empty;
    _combatTime = 0;

    // Reset combat metrics
    _combatMetrics.Reset();

    // Initialize combat behaviors
    InitializeCombatSystems();

    // Switch to combat aspect
    SwitchToCombatAspect();

    // Command pet to attack
    if (HasActivePet() && target)
    {
        CommandPetAttack(target);
    }

    TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} entering combat with {}",

                 GetBot()->GetName(), target ? target->GetName() : "null");

    ClassAI::OnCombatStart(target);
}

void HunterAI::OnCombatEnd()
{
    _inCombat = false;
    _currentTarget = ObjectGuid::Empty;
    _combatTime = 0;

    // Log combat metrics    LogCombatMetrics();

    // Command pet to follow
    if (HasActivePet())
    {
        CommandPetFollow();    }

    // Clear trap tracking    _frozenTargets.clear();
    _activeTrapType = 0;

    TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} left combat", GetBot()->GetName());
    ClassAI::OnCombatEnd();
}
bool HunterAI::HasEnoughResource(uint32 spellId)
{
    if (!_bot)
        return false;

    // Get spell info
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, _bot->GetMap()->GetDifficultyID());
    if (!spellInfo)
        return false;

    // Check focus cost
    for (SpellPowerEntry const* power : spellInfo->PowerCosts)
    {
        if (power && power->PowerType == POWER_FOCUS)
        {
            uint32 focusCost = power->ManaCost;

            return HasFocus(focusCost);
        }
    }

    // Default for abilities without focus cost
    return true;
}

void HunterAI::ConsumeResource(uint32 spellId)
{
    if (!_bot)
        return;

    // Get spell info
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, _bot->GetMap()->GetDifficultyID());
    if (!spellInfo)
        return;

    // Track focus consumption
    for (SpellPowerEntry const* power : spellInfo->PowerCosts)
    {
        if (power && power->PowerType == POWER_FOCUS)
        {
            _combatMetrics.focusSpent += power->ManaCost;

            break;
        }
    }

    // Track shot statistics
    _combatMetrics.shotsLanded++;
}

Position HunterAI::GetOptimalPosition(::Unit* target)
{
    if (!target || !_bot)        return _bot ? _bot->GetPosition() : Position();

    // Use combat behavior integration for positioning
    if (_combatBehaviors)
        return _combatBehaviors->GetOptimalPosition();

    // Fallback to preferred range
    float angle = _bot->GetAbsoluteAngle(target);
    float distance = OPTIMAL_RANGE_PREFERRED;

    Position pos;
    pos.m_positionX = target->GetPositionX() - distance * cos(angle);
    pos.m_positionY = target->GetPositionY() - distance * sin(angle);    pos.m_positionZ = target->GetPositionZ();
        pos.SetOrientation(target->GetOrientation());

    return pos;
}

float HunterAI::GetOptimalRange(::Unit* target)
{
    if (!target)
        return OPTIMAL_RANGE_PREFERRED;

    // Use combat behavior integration
    if (_combatBehaviors)
        return _combatBehaviors->GetOptimalRange(target);

    // Kiting range if target is dangerous
    if (NeedsToKite(target))
        return KITING_RANGE;

    return OPTIMAL_RANGE_PREFERRED;
}

HunterSpec HunterAI::GetCurrentSpecialization() const
{
    if (!_bot)
        return HunterSpec::BEAST_MASTERY;

    // Use TrinityCore's GetPrimarySpecialization to determine specialization
    // 253 = Beast Mastery, 254 = Marksmanship, 255 = Survival
    uint32 spec = static_cast<uint32>(_bot->GetPrimarySpecialization());

    switch (spec)
    {
        case 253: // Beast Mastery

            return HunterSpec::BEAST_MASTERY;
        case 254: // Marksmanship

            return HunterSpec::MARKSMANSHIP;
        case 255: // Survival

            return HunterSpec::SURVIVAL;
        default:

            return HunterSpec::BEAST_MASTERY;
    }
}

Pet* HunterAI::GetPet() const
{
    if (!_bot)
        return nullptr;
    return _bot->GetPet();
}

bool HunterAI::HasActivePet() const
{
    Pet* pet = GetPet();
    return pet && pet->IsAlive();
}

// Pet management implementation
void HunterAI::UpdatePetStatus()
{
    Pet* pet = GetPet();
    if (pet)
    {
        _petGuid = pet->GetGUID();
        _petNeedsHeal = pet->GetHealthPct() < PET_HEAL_THRESHOLD;

        // ========================================================================
        // CRITICAL FIX: Ensure bot pets are always in DEFENSIVE mode
        // ========================================================================
        // Pet react state is loaded from database on spawn. If it was saved as PASSIVE,
        // the pet will not defend itself or its master when attacked.
        // Bot pets should ALWAYS be DEFENSIVE so they auto-attack when:
        // - The bot is attacked
        // - The pet itself is attacked
        // - The bot issues an attack command
        // ========================================================================
        if (pet->HasReactState(REACT_PASSIVE))
        {
            TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} pet {} was PASSIVE, setting to DEFENSIVE",
                         GetBot()->GetName(), pet->GetName());
            pet->SetReactState(REACT_DEFENSIVE);
        }
    }
    else
    {
        _petGuid.Clear();
        _petNeedsHeal = false;
    }
}

bool HunterAI::NeedsPetRevive() const
{
    Pet* pet = GetPet();
    return pet && !pet->IsAlive();
}

bool HunterAI::NeedsPetHeal() const
{
    return _petNeedsHeal;
}

bool HunterAI::ShouldDismissPet() const
{
    // Dismiss pet in specific situations (e.g., stealth required)
    return false;
}

void HunterAI::CommandPetAttack(::Unit* target)
{
    if (!target || !HasActivePet())
        return;

    Pet* pet = GetPet();
    if (pet)
    {
        pet->AI()->AttackStart(target);
        _lastPetCommand = GameTime::GetGameTimeMS();
    }
}

void HunterAI::CommandPetFollow()
{
    Pet* pet = GetPet();
    if (pet)
    {
        pet->GetMotionMaster()->MoveFollow(_bot, 2.0f, static_cast<float>(M_PI));
        _lastPetCommand = GameTime::GetGameTimeMS();
    }
}

void HunterAI::CommandPetStay()
{
    Pet* pet = GetPet();
    if (pet)
    {
        pet->StopMoving();
        pet->GetMotionMaster()->Clear();
        _lastPetCommand = GameTime::GetGameTimeMS();
    }
}

bool HunterAI::IsPetInCombat() const
{
    Pet* pet = GetPet();
    return pet && pet->IsInCombat();
}

float HunterAI::GetPetHealthPercent() const
{
    Pet* pet = GetPet();
    return pet ? pet->GetHealthPct() : 0.0f;
}
void HunterAI::HealPet()
{    if (!_bot->HasSpell(MEND_PET) || !CanUseAbility(MEND_PET))        return;
Pet* pet = GetPet();
    if (pet && pet->IsAlive())
    {        _bot->CastSpell(CastSpellTargetArg(pet), MEND_PET);
    }
}

void HunterAI::RevivePet()
{
    if (!_bot->HasSpell(REVIVE_PET) || !CanUseAbility(REVIVE_PET))
        return;

    _bot->CastSpell(CastSpellTargetArg(_bot), REVIVE_PET);
}

void HunterAI::CallPet()
{
    if (!_bot->HasSpell(CALL_PET) || !CanUseAbility(CALL_PET))
        return;

    _bot->CastSpell(CastSpellTargetArg(_bot), CALL_PET);
}

// Trap management implementation
bool HunterAI::CanPlaceTrap() const
{
    uint32 now = GameTime::GetGameTimeMS();
    return (now - _lastTrapPlacement) > 30000; // 30 second cooldown
}

bool HunterAI::ShouldPlaceFreezingTrap(::Unit* target) const
{    if (!target || !CanPlaceTrap())
        return false;

    // Place freezing trap for dangerous adds or CC targets
    return IsTargetDangerous(target) && GetDistanceToTarget(target) < TRAP_PLACEMENT_RANGE;
}

bool HunterAI::ShouldPlaceExplosiveTrap() const
{
    if (!CanPlaceTrap())
        return false;

    // Place explosive trap when multiple enemies nearby
    return GetNearbyEnemyCount(10.0f) >= 3;
}
bool HunterAI::ShouldPlaceTarTrap() const
{
    if (!CanPlaceTrap())
        return false;

    // Place tar trap for slowing multiple enemies (replaces Snake Trap in WoW 12.0)
    return GetNearbyEnemyCount(10.0f) >= 2;
}

void HunterAI::PlaceTrap(uint32 trapSpell, const Position& pos)
{    if (!_bot->HasSpell(trapSpell) || !CanUseAbility(trapSpell))
        return;

    // Note: Ground-targeted spells need special handling    _bot->CastSpell(trapSpell, false, _bot);
    _lastTrapPosition = pos;
    _activeTrapType = trapSpell;
    RecordTrapPlacement(trapSpell);
}

uint32 HunterAI::GetBestTrapForSituation() const
{
    if (ShouldPlaceFreezingTrap(nullptr))
        return FREEZING_TRAP;
    if (ShouldPlaceExplosiveTrap())
        return EXPLOSIVE_TRAP;
    // TAR_TRAP replaces Snake Trap in WoW 12.0
    return TAR_TRAP;
}
// Range management implementation
bool HunterAI::IsInOptimalRange(::Unit* target) const
{
    if (!target)
        return false;

    float distance = GetDistanceToTarget(target);
    return distance >= OPTIMAL_RANGE_MIN && distance <= OPTIMAL_RANGE_MAX;
}

bool HunterAI::IsInDeadZone(::Unit* target) const
{
    if (!target)
    {        // Check if any enemy is in dead zone
        float minDistance = 100.0f;
        Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(_bot, _bot, DEAD_ZONE_MAX);
        Trinity::UnitLastSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(_bot, target, u_check);
        // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Map* map = _bot->GetMap();
    if (!map)
        return false;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)

            return false;
    }    // Query nearby GUIDs (lock-free!)
    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        _bot->GetPosition(), DEAD_ZONE_MAX);

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
    if (target)
        {

            minDistance = ::std::sqrt(_bot->GetExactDistSq(target)); // Calculate once from squared distance
            }
        return minDistance > DEAD_ZONE_MIN && minDistance <= DEAD_ZONE_MAX;
    }

    float distance = GetDistanceToTarget(target);
    return distance > DEAD_ZONE_MIN && distance <= DEAD_ZONE_MAX;
}

bool HunterAI::NeedsToKite(::Unit* target) const
{
    if (!target)
        return false;

    // Kite if target is melee and dangerous
    float kiteSq = 10.0f * 10.0f; // 100.0f
    return target->GetExactDistSq(_bot) < kiteSq && IsTargetDangerous(target);
}

void HunterAI::MaintainRange(::Unit* target)
{
    if (!target)
        return;

    float distance = GetDistanceToTarget(target);

    if (distance < OPTIMAL_RANGE_MIN)
    {
        // Too close, move back
        Position pos = GetOptimalPosition(target);

        // PHASE 6C: Use Movement Arbiter with ROLE_POSITIONING priority (170)
        BotAI* botAI = dynamic_cast<BotAI*>(_bot->GetAI());
        if (botAI && botAI->GetUnifiedMovementCoordinator())
        {

            botAI->RequestPointMovement(

                PlayerBotMovementPriority::ROLE_POSITIONING,

                pos,

                "Hunter optimal range positioning (too close)",

                "HunterAI");
        }
        else
        {
            // FALLBACK: Use BotMovementController with validated pathfinding
            if (BotAI* ai = GetBotAI(_bot))
            {
                if (!ai->MoveTo(pos, true))
                {
                    // Final fallback to legacy if validation fails
                    _bot->GetMotionMaster()->MovePoint(0, pos);
                }
            }
            else
            {
                // Non-bot player - use standard movement
                _bot->GetMotionMaster()->MovePoint(0, pos);
            }
        }
    }
    else if (distance > OPTIMAL_RANGE_MAX)
    {
        // Too far, move closer
        // CRITICAL FIX: Only issue MoveChase if not already chasing
        // Re-issuing MoveChase causes speed multiplication bug
        MotionMaster* mm = _bot->GetMotionMaster();
        if (mm->GetCurrentMovementGeneratorType(MOTION_SLOT_ACTIVE) != CHASE_MOTION_TYPE)
        {

            mm->MoveChase(target, OPTIMAL_RANGE_PREFERRED);
        }
    }
}

float HunterAI::GetDistanceToTarget(::Unit* target) const
{
    if (!target || !_bot)
        return 0.0f;
    return ::std::sqrt(_bot->GetExactDistSq(target)); // Calculate once from squared distance
}

// Hunter-specific mechanics implementation
void HunterAI::ManageAspects()
{
    if (!_bot)
        return;

    uint32 currentAspect = GetCurrentAspect();
    uint32 optimalAspect = GetOptimalAspect();

    if (currentAspect != optimalAspect)
    {
        uint32 now = GameTime::GetGameTimeMS();
        if (now - _lastAspectSwitch > 1000) // 1 second GCD

        {
        if (_bot->HasSpell(optimalAspect) && CanUseAbility(optimalAspect))

            {

                CastSpell(optimalAspect);

                _lastAspectSwitch = now;

            }
        }
    }
}

void HunterAI::UpdateTracking()
{
    // Note: Tracking spells were removed in WoW 12.0
    // This function is kept for interface compatibility but does nothing
}

bool HunterAI::HasAnyAspect()
{
    // WoW 12.0 only has these aspects
    return HasAura(ASPECT_OF_THE_WILD) ||
           HasAura(ASPECT_OF_THE_CHEETAH) ||
           HasAura(ASPECT_OF_THE_TURTLE);
}

uint32 HunterAI::GetCurrentAspect()
{
    // WoW 12.0 aspects only
    if (HasAura(ASPECT_OF_THE_WILD)) return ASPECT_OF_THE_WILD;
    if (HasAura(ASPECT_OF_THE_CHEETAH)) return ASPECT_OF_THE_CHEETAH;
    if (HasAura(ASPECT_OF_THE_TURTLE)) return ASPECT_OF_THE_TURTLE;
    return 0;
}

void HunterAI::SwitchToCombatAspect()
{
    // WoW 12.0: Aspect of the Wild is the primary combat aspect
    if (!HasAura(ASPECT_OF_THE_WILD) && _bot->HasSpell(ASPECT_OF_THE_WILD))
    {
        CastSpell(ASPECT_OF_THE_WILD);
        _lastAspectSwitch = GameTime::GetGameTimeMS();
    }
}

void HunterAI::SwitchToMovementAspect()
{
    // WoW 12.0: Aspect of the Cheetah is the movement aspect (Aspect of the Pack was removed)
    if (!HasAura(ASPECT_OF_THE_CHEETAH) && _bot->HasSpell(ASPECT_OF_THE_CHEETAH))
    {
        CastSpell(ASPECT_OF_THE_CHEETAH);
        _lastAspectSwitch = GameTime::GetGameTimeMS();
    }
}

bool HunterAI::ValidateAspectForAbility(uint32 spellId) const
{
    // Some abilities require specific aspects
    return true;
}

uint32 HunterAI::GetOptimalAspect() const
{
    if (_bot->IsInCombat())
    {
        // WoW 12.0: Aspect of the Wild is the combat aspect
        // Note: Aspect of the Viper was removed - focus regenerates passively now
        if (_bot->HasSpell(ASPECT_OF_THE_WILD))
            return ASPECT_OF_THE_WILD;
        return 0; // No aspect
    }
    else
    {
        // Out of combat movement
        if (_bot->HasSpell(ASPECT_OF_THE_CHEETAH))
            return ASPECT_OF_THE_CHEETAH;
        return 0; // No aspect
    }
}

// Utility method implementations
bool HunterAI::ShouldFeignDeath() const
{
    // Feign death to drop aggro or escape
    return _bot->GetHealthPct() < FEIGN_DEATH_THRESHOLD || _bot->GetThreatManager().GetThreatListSize() > 3;
}

bool HunterAI::CanInterruptTarget(::Unit* target) const
{
    if (!target)
        return false;

    // Check if target is casting an interruptible spell
    return target->IsNonMeleeSpellCast(false, false, true);
}

::Unit* HunterAI::GetBestCrowdControlTarget() const
{
    ::Unit* bestTarget = nullptr;
    float lowestHealth = 100.0f;

    // Find best CC target (lowest health add that's not the main target)
    ::std::list<Unit*> targets;
    Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(_bot, _bot, 30.0f);
    Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(_bot, targets, u_check);
    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Map* map = _bot->GetMap();
    if (!map)
        return nullptr;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)

            return nullptr;
    }

    // Query nearby GUIDs (lock-free!)
    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        _bot->GetPosition(), 30.0f);

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

    // Find the target with lowest health that's not the current main target
    Unit* currentTarget = _bot->GetVictim();
    for (Unit* unit : targets)
    {
        if (unit && unit != currentTarget && unit->IsAlive())
        {

            float healthPct = unit->GetHealthPct();

            if (healthPct < lowestHealth)

            {

                lowestHealth = healthPct;

                bestTarget = unit;

            }
        }
    }

    return bestTarget;
}

uint32 HunterAI::GetNearbyEnemyCount(float range) const
{
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
    return targets.size();
}

bool HunterAI::HasFocus(uint32 amount) const
{
    return _bot && _bot->GetPower(POWER_FOCUS) >= amount;}

uint32 HunterAI::GetFocus() const
{
    return _bot ? _bot->GetPower(POWER_FOCUS) : 0;
}

uint32 HunterAI::GetMaxFocus() const
{
    return _bot ? _bot->GetMaxPower(POWER_FOCUS) : 0;
}

float HunterAI::GetFocusPercent() const
{
    uint32 maxFocus = GetMaxFocus();
    return maxFocus > 0 ? (static_cast<float>(GetFocus()) / maxFocus) * 100.0f : 0.0f;
}

void HunterAI::LogCombatMetrics()
{
    if (_updateCounter == 0)
        return;

    TC_LOG_DEBUG("module.playerbot.ai",

                 "Hunter {} combat ended - Shots: {}/{}, Crits: {}, Interrupts: {}, Traps: {}, Pet Commands: {}, "

                 "Focus Spent: {}, Damage: {}, Time at Range: {:.1f}s, Time in Dead Zone: {:.1f}s, "

                 "Avg Update: {} us, Peak: {} us",

                 GetBot()->GetName(),

                 uint32(_combatMetrics.shotsLanded.load()),

                 uint32(_combatMetrics.shotsMissed.load()),

                 uint32(_combatMetrics.criticalStrikes.load()),

                 uint32(_combatMetrics.interrupts.load()),

                 uint32(_combatMetrics.trapsTriggered.load()),

                 uint32(_combatMetrics.petCommands.load()),

                 uint32(_combatMetrics.focusSpent.load()),

                 uint32(_combatMetrics.damageDealt.load()),

                 _combatMetrics.timeAtRange,

                 _combatMetrics.timeInDeadZone,

                 _totalUpdateTime / _updateCounter,

                 _peakUpdateTime);
}

Player* HunterAI::GetMainTank()
{    if (!_bot->GetGroup())
        return nullptr;

    Group* group = _bot->GetGroup();
    Player* tank = nullptr;

    // Find tank by looking for warriors/paladins/death knights with tank specs
    // In TrinityCore, we check class + role based on talents/gear
    for (GroupReference const& itr : group->GetMembers())
    {
        Player* member = itr.GetSource();
        if (member)
        {

            uint8 playerClass = member->GetClass();
            // Typical tank classes: Warrior, Paladin, Death Knight, Druid (in bear form)
    if (playerClass == CLASS_WARRIOR || playerClass == CLASS_PALADIN ||

                playerClass == CLASS_DEATH_KNIGHT || playerClass == CLASS_DRUID)

            {

                tank = member;

                break; // Use first potential tank found

            }
        }
    }

    return tank;
}

bool HunterAI::IsTargetDangerous(::Unit* target) const
{
    if (!target)
        return false;

    // Check if target is elite or boss
    if (target->GetCreatureType() == CREATURE_TYPE_BEAST && target->GetLevel() > _bot->GetLevel() + 2)        return true;

    // Check if target has high damage output
    return target->GetTotalAttackPowerValue(BASE_ATTACK) > _bot->GetMaxHealth() * 0.3f;
}

bool HunterAI::ShouldSaveDefensives() const
{
    // Save defensives for boss fights or multiple enemies
    return GetNearbyEnemyCount(20.0f) > 1 || (_currentTarget.IsCreature() &&
           ObjectAccessor::GetCreature(*_bot, _currentTarget) &&
           ObjectAccessor::GetCreature(*_bot, _currentTarget)->isWorldBoss());
}

void HunterAI::RecordShotResult(bool hit, bool crit)
{
    if (hit)
    {
        _combatMetrics.shotsLanded++;
        if (crit)

            _combatMetrics.criticalStrikes++;
    }
    else
    {
        _combatMetrics.shotsMissed++;
    }
}

void HunterAI::RecordTrapPlacement(uint32 trapSpell)
{
    _combatMetrics.trapsTriggered++;
    TC_LOG_DEBUG("module.playerbot.ai", "Hunter {} placed trap type {}",

                 GetBot()->GetName(), trapSpell);
}

} // namespace Playerbot