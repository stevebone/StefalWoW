/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PaladinAI.h"
#include "HolyPaladin.h"
#include "ProtectionPaladin.h"
#include "RetributionPaladin.h"
#include "ObjectAccessor.h"
#include "../BaselineRotationManager.h"
#include "../../Combat/CombatBehaviorIntegration.h"
#include "Player.h"
#include "SpellMgr.h"
#include "Log.h"
#include "CellImpl.h"
#include "GridNotifiersImpl.h"
#include "Unit.h"
#include "SpellAuras.h"
#include "Group.h"
#include "../../../Spatial/SpatialGridManager.h"
#include "../../../Spatial/SpatialGridQueryHelpers.h"  // PHASE 5F: Thread-safe queries
#include "../../../Group/RoleDefinitions.h"  // For tank detection
#include "GameTime.h"

namespace Playerbot
{

// Destructor must be defined in cpp file where specialization types are complete
PaladinAI::~PaladinAI() = default;

PaladinAI::PaladinAI(Player* bot) : ClassAI(bot)
{
    _lastBlessingTime = 0;
    _lastAuraChange = 0;
    _lastConsecration = 0;
    _lastDivineShield = 0;
    _lastLayOnHands = 0;
    _needsReposition = false;
    _shouldConserveMana = false;
    _currentSeal = 0;
    _currentAura = 0;
    _currentBlessing = 0;
    _successfulInterrupts = 0;

    TC_LOG_DEBUG("module.playerbot.ai", "PaladinAI created for player {}",
                 bot ? bot->GetName() : "null");
}

void PaladinAI::UpdateRotation(::Unit* target)
{
    if (!target || !GetBot())
        return;

    // NOTE: Baseline rotation check is now handled at the dispatch level in
    // ClassAI::OnCombatUpdate(). This method is ONLY called when the bot has
    // already chosen a specialization (level 10+ with talents).

    // ========================================================================
    // COMBAT BEHAVIOR INTEGRATION - Priority-based decision making
    // ========================================================================
    auto* behaviors = GetCombatBehaviors();

    // Priority 1: Handle interrupts (Rebuke/Hammer of Justice)
    if (behaviors && behaviors->ShouldInterrupt(target))
    {
        Unit* interruptTarget = behaviors->GetInterruptTarget();
        if (interruptTarget)
        {
            // Try Rebuke first (instant interrupt)
    if (CanUseAbility(REBUKE))
            {
                if (CastSpell(REBUKE, interruptTarget))
                {
                    RecordInterruptAttempt(interruptTarget, REBUKE, true);
                    TC_LOG_DEBUG("module.playerbot.ai", "Paladin {} interrupted {} with Rebuke",
                                 GetBot()->GetName(), interruptTarget->GetName());
                    return;
                }
            }

            // Try Hammer of Justice as secondary interrupt (stun)
    if (CanUseAbility(HAMMER_OF_JUSTICE))
            {
                if (CastSpell(HAMMER_OF_JUSTICE, interruptTarget))
                {
                    RecordInterruptAttempt(interruptTarget, HAMMER_OF_JUSTICE, true);
                    TC_LOG_DEBUG("module.playerbot.ai", "Paladin {} stunned {} with Hammer of Justice",
                                 GetBot()->GetName(), interruptTarget->GetName());
                    return;
                }
            }
        }
    }

    // Priority 2: Handle defensives (Divine Shield, Shield of Vengeance, etc.)
    if (behaviors && behaviors->NeedsDefensive())
    {
        UseDefensiveCooldowns();
        if (GetBot()->HasUnitState(UNIT_STATE_CASTING))
            return;
    }

    // Priority 3: Check for target switching
    if (behaviors && behaviors->ShouldSwitchTarget())
    {
        Unit* priorityTarget = behaviors->GetPriorityTarget();
        if (priorityTarget && priorityTarget != target)
        if (!priorityTarget)
        {
            return;
        }
        {
            OnTargetChanged(priorityTarget);
            target = priorityTarget;
                         if (!priorityTarget)
                         {
                             return;
                         }
            TC_LOG_DEBUG("module.playerbot.ai", "Paladin {} switching target to {}",
                         GetBot()->GetName(), priorityTarget->GetName());
        }
    }

    // Priority 4: AoE vs Single-Target decision
    if (behaviors && behaviors->ShouldAOE())
    {
        uint32 nearbyEnemies = GetNearbyEnemyCount(DIVINE_STORM_RADIUS);

        // Divine Storm for heavy AoE damage (Retribution)
    if (nearbyEnemies >= 3 && GetHolyPower() >= 3 && CanUseAbility(DIVINE_STORM))
        {
            if (CastSpell(DIVINE_STORM))
            {
                RecordAbilityUsage(DIVINE_STORM);
                _paladinMetrics.holyPowerSpent += 3;
                TC_LOG_DEBUG("module.playerbot.ai", "Paladin {} using Divine Storm for AoE",
                             GetBot()->GetName());
                return;
            }
        }

        // Consecration for AoE damage/threat
    if (nearbyEnemies >= 2 && CanUseAbility(CONSECRATION))
        {
            uint32 currentTime = GameTime::GetGameTimeMS();
            if (currentTime - _lastConsecration > 8000)  // 8 second duration
            {
                if (CastSpell(CONSECRATION))
                {
                    RecordAbilityUsage(CONSECRATION);
                    _lastConsecration = currentTime;
                    TC_LOG_DEBUG("module.playerbot.ai", "Paladin {} using Consecration for AoE",
                                 GetBot()->GetName());
                    return;
                }
            }
        }

        // Wake of Ashes for AoE + Holy Power generation (Retribution)
    if (nearbyEnemies >= 2 && CanUseAbility(WAKE_OF_ASHES))
        {
            if (CastSpell(WAKE_OF_ASHES))
            {
                RecordAbilityUsage(WAKE_OF_ASHES);
                _paladinMetrics.holyPowerGenerated += 5;
                TC_LOG_DEBUG("module.playerbot.ai", "Paladin {} using Wake of Ashes for AoE",
                             GetBot()->GetName());
                return;
            }
        }
    }

    // Priority 5: Use major cooldowns at optimal time
    if (behaviors && behaviors->ShouldUseCooldowns())
    {
        UseOffensiveCooldowns();
    }

    // Priority 6: Holy Power Management
    ManageHolyPower(target);
    if (GetBot()->HasUnitState(UNIT_STATE_CASTING))
        return;

    // Priority 7: Execute normal rotation
    // Use basic rotation - specialized behavior comes from talent/spec checks
    ExecuteBasicPaladinRotation(target);

    // Handle repositioning needs
    if (behaviors && behaviors->NeedsRepositioning())
    {
        Position optimalPos = behaviors->GetOptimalPosition();
        _needsReposition = true;
    }
}

void PaladinAI::UpdateBuffs()
{
    // NOTE: Baseline buff check is now handled at the dispatch level.
    // This method is only called for level 10+ bots with talents.

    // Use full paladin buff system for specialized bots
    UpdatePaladinBuffs();
}

void PaladinAI::UpdateCooldowns(uint32 diff)
{
    UpdateMetrics(diff);
    // Cooldowns are tracked via standard spell system
}

bool PaladinAI::CanUseAbility(uint32 spellId)
{
    if (!IsSpellReady(spellId) || !HasEnoughResource(spellId))
        return false;

    // Check Holy Power requirements for specific abilities
    if (!HasHolyPowerFor(spellId))        return false;

    return true;}

void PaladinAI::OnCombatStart(::Unit* target)
{
    _paladinMetrics.combatStartTime = ::std::chrono::steady_clock::now();

    TC_LOG_DEBUG("module.playerbot.ai", "PaladinAI {} entering combat with {}",
                 GetBot()->GetName(), target->GetName());

    _inCombat = true;
    _currentTarget = target->GetGUID();
    _combatTime = 0;
}

void PaladinAI::OnCombatEnd()
{
    AnalyzeCombatEffectiveness();

    _inCombat = false;
    _currentTarget = ObjectGuid::Empty;
    _combatTime = 0;

    TC_LOG_DEBUG("module.playerbot.ai", "PaladinAI {} leaving combat", GetBot()->GetName());
}

bool PaladinAI::HasEnoughResource(uint32 spellId)
{
    // Simplified resource check - detailed checks handled by spell system
    return GetBot()->GetPower(POWER_MANA) >= 100; // Conservative estimate
}

void PaladinAI::ConsumeResource(uint32 spellId)
{
    RecordAbilityUsage(spellId);
    // Resource consumption is handled by spell system
}

Position PaladinAI::GetOptimalPosition(::Unit* target)
{
    if (!target || !GetBot())
        return Position();

    // Calculate optimal position based on current spec
    return CalculateOptimalMeleePosition(target);
}

float PaladinAI::GetOptimalRange(::Unit* target)
{
    // Check spec using TrinityCore API
    ChrSpecialization spec = GetBot()->GetPrimarySpecialization();

    // Holy Paladin uses healing range
    if (spec == ChrSpecialization::PaladinHoly)
        return OPTIMAL_HEALING_RANGE;

    // Retribution and Protection use melee range
    return OPTIMAL_MELEE_RANGE;
}

void PaladinAI::ExecuteBasicPaladinRotation(::Unit* target)
{
    if (!target || !GetBot())
        return;

    // Basic rotation for paladins without specialization
    // Focus on simple holy power generation and spending

    // Generate Holy Power with Blade of Justice/Crusader Strike
    if (GetHolyPower() < 3)
    {
        // Try Blade of Justice first (better damage)
    if (CanUseAbility(BLADE_OF_JUSTICE))
        {
            if (CastSpell(BLADE_OF_JUSTICE, target))
            {
                RecordAbilityUsage(BLADE_OF_JUSTICE);
                _paladinMetrics.holyPowerGenerated += 2;
                return;
            }
        }

        // Fallback to Crusader Strike
    if (CanUseAbility(CRUSADER_STRIKE))
        {
            if (CastSpell(CRUSADER_STRIKE, target))
            {
                RecordAbilityUsage(CRUSADER_STRIKE);
                _paladinMetrics.holyPowerGenerated += 1;
                return;
            }
        }

        // Judgment for ranged Holy Power generation
    if (CanUseAbility(JUDGMENT))
        {
            if (CastSpell(JUDGMENT, target))
            {
                RecordAbilityUsage(JUDGMENT);
                _paladinMetrics.holyPowerGenerated += 1;
                return;
            }
        }
    }

    // Spend Holy Power
    if (GetHolyPower() >= 3)
    {
        // Check if we need healing
    if (GetBot()->GetHealthPct() < 50.0f && CanUseAbility(WORD_OF_GLORY))
        {
            if (CastSpell(WORD_OF_GLORY, GetBot()))
            {
                RecordAbilityUsage(WORD_OF_GLORY);
                _paladinMetrics.holyPowerSpent += 3;
                return;
            }
        }

        // Use Templar's Verdict for single target
    if (GetNearbyEnemyCount(DIVINE_STORM_RADIUS) < 2 && CanUseAbility(TEMPLARS_VERDICT))
        {
            if (CastSpell(TEMPLARS_VERDICT, target))
            {
                RecordAbilityUsage(TEMPLARS_VERDICT);
                _paladinMetrics.holyPowerSpent += 3;
                return;
            }
        }

        // Use Divine Storm for AoE
    if (GetNearbyEnemyCount(DIVINE_STORM_RADIUS) >= 2 && CanUseAbility(DIVINE_STORM))
        {
            if (CastSpell(DIVINE_STORM))
            {
                RecordAbilityUsage(DIVINE_STORM);
                _paladinMetrics.holyPowerSpent += 3;
                return;
            }
        }
    }

    // Use Hammer of Wrath on low health targets
    if (target->GetHealthPct() < 20.0f && CanUseAbility(HAMMER_OF_WRATH))
    {
        if (CastSpell(HAMMER_OF_WRATH, target))
        {
            RecordAbilityUsage(HAMMER_OF_WRATH);
            return;
        }
    }

    // Maintain Consecration for AoE/threat
    if (GetNearbyEnemyCount(CONSECRATION_RADIUS) > 0 && CanUseAbility(CONSECRATION))
    {
        uint32 currentTime = GameTime::GetGameTimeMS();
        if (currentTime - _lastConsecration > 8000)
        {
            if (CastSpell(CONSECRATION))
            {
                RecordAbilityUsage(CONSECRATION);
                _lastConsecration = currentTime;
                return;
            }
        }
    }
}

void PaladinAI::UpdatePaladinBuffs()
{
    UpdateBlessingManagement();
    UpdateAuraManagement();
    // Spec-specific buffs are handled through normal rotation
}

void PaladinAI::UseDefensiveCooldowns()
{
    if (!GetBot())
        return;

    float healthPct = GetBot()->GetHealthPct();

    // Lay on Hands at critical health
    if (healthPct < LAY_ON_HANDS_THRESHOLD && CanUseAbility(LAY_ON_HANDS))
    {
        uint32 currentTime = GameTime::GetGameTimeMS();
        if (currentTime - _lastLayOnHands > 600000)  // 10 minute cooldown
        {
            if (CastSpell(LAY_ON_HANDS, GetBot()))
            {
                RecordAbilityUsage(LAY_ON_HANDS);
                _lastLayOnHands = currentTime;
                TC_LOG_DEBUG("module.playerbot.ai", "Paladin {} activated Lay on Hands",
                             GetBot()->GetName());
                return;
            }
        }
    }

    // Divine Shield at emergency health
    if (healthPct < HEALTH_CRITICAL_THRESHOLD && CanUseAbility(DIVINE_SHIELD))
    {
        uint32 currentTime = GameTime::GetGameTimeMS();
        if (currentTime - _lastDivineShield > 300000)  // 5 minute cooldown
        {
            if (CastSpell(DIVINE_SHIELD))
            {
                RecordAbilityUsage(DIVINE_SHIELD);
                _lastDivineShield = currentTime;
                TC_LOG_DEBUG("module.playerbot.ai", "Paladin {} activated Divine Shield",
                             GetBot()->GetName());
                return;
            }
        }
    }

    // Shield of Vengeance for damage absorption
    if (healthPct < HEALTH_EMERGENCY_THRESHOLD && CanUseAbility(SHIELD_OF_VENGEANCE))
    {
        if (CastSpell(SHIELD_OF_VENGEANCE))
        {
            RecordAbilityUsage(SHIELD_OF_VENGEANCE);
            TC_LOG_DEBUG("module.playerbot.ai", "Paladin {} activated Shield of Vengeance",
                         GetBot()->GetName());
            return;
        }
    }

    // Divine Protection for damage reduction
    if (healthPct < DEFENSIVE_COOLDOWN_THRESHOLD && CanUseAbility(DIVINE_PROTECTION))
    {
        if (CastSpell(DIVINE_PROTECTION))
        {
            RecordAbilityUsage(DIVINE_PROTECTION);
            TC_LOG_DEBUG("module.playerbot.ai", "Paladin {} activated Divine Protection",
                         GetBot()->GetName());
            return;
        }
    }

    // Protection-specific defensives
    if (GetBot()->GetPrimarySpecialization() == ChrSpecialization::PaladinProtection)
    {
        // Ardent Defender
    if (healthPct < 35.0f && CanUseAbility(ARDENT_DEFENDER))
        {
            if (CastSpell(ARDENT_DEFENDER))
            {
                RecordAbilityUsage(ARDENT_DEFENDER);
                TC_LOG_DEBUG("module.playerbot.ai", "Protection Paladin {} activated Ardent Defender",
                             GetBot()->GetName());
                return;
            }
        }

        // Guardian of Ancient Kings
    if (healthPct < 50.0f && CanUseAbility(GUARDIAN_OF_ANCIENT_KINGS))
        {
            if (CastSpell(GUARDIAN_OF_ANCIENT_KINGS))
            {
                RecordAbilityUsage(GUARDIAN_OF_ANCIENT_KINGS);
                TC_LOG_DEBUG("module.playerbot.ai", "Protection Paladin {} activated Guardian of Ancient Kings",
                             GetBot()->GetName());
                return;            }
        }
    }

    // Blessing of Protection for allies
    if (IsAllyInDanger() && CanUseAbility(BLESSING_OF_PROTECTION))
    {
        // Find ally in danger
        Group* group = GetBot()->GetGroup();
        if (group)
        {
            for (GroupReference const& itr : group->GetMembers())
            {
                Player* member = itr.GetSource();
                if (member && member != GetBot() && member->GetHealthPct() < 30.0f)                {
                    if (CastSpell(BLESSING_OF_PROTECTION, member))
                    {
                        RecordAbilityUsage(BLESSING_OF_PROTECTION);
                        TC_LOG_DEBUG("module.playerbot.ai", "Paladin {} cast Blessing of Protection on {}",
                                     GetBot()->GetName(), member->GetName());
                        return;
                    }
                }
            }
        }
    }
}

void PaladinAI::UseOffensiveCooldowns()
{
    if (!GetBot())
        return;

    // Avenging Wrath for damage boost
    if (CanUseAbility(AVENGING_WRATH))
    {
        if (CastSpell(AVENGING_WRATH))
        {
            RecordAbilityUsage(AVENGING_WRATH);
            TC_LOG_DEBUG("module.playerbot.ai", "Paladin {} activated Avenging Wrath",
                         GetBot()->GetName());
        }
    }

    // Crusade for Retribution (replaces Avenging Wrath)
    if (GetBot()->GetPrimarySpecialization() == ChrSpecialization::PaladinRetribution && CanUseAbility(CRUSADE))
    {
        if (CastSpell(CRUSADE))
        {            RecordAbilityUsage(CRUSADE);
            TC_LOG_DEBUG("module.playerbot.ai", "Retribution Paladin {} activated Crusade",
                         GetBot()->GetName());
        }
    }

    // Holy Avenger for burst
    if (CanUseAbility(HOLY_AVENGER))
    {
        if (CastSpell(HOLY_AVENGER))
        {
            RecordAbilityUsage(HOLY_AVENGER);
            TC_LOG_DEBUG("module.playerbot.ai", "Paladin {} activated Holy Avenger",
                         GetBot()->GetName());
        }
    }

    // Execution Sentence on primary target
    Unit* target = GetBot()->GetSelectedUnit();
    if (target && CanUseAbility(EXECUTION_SENTENCE))
    {
        if (CastSpell(EXECUTION_SENTENCE, target))
        {
            RecordAbilityUsage(EXECUTION_SENTENCE);
            TC_LOG_DEBUG("module.playerbot.ai", "Paladin {} cast Execution Sentence on {}",
                         GetBot()->GetName(), target->GetName());
        }
    }
}

void PaladinAI::ManageHolyPower(::Unit* target)
{
    if (!target || !GetBot())
        return;

    uint32 holyPower = GetHolyPower();

    // Generate Holy Power if low
    if (holyPower < 3)
    {
        GenerateHolyPower(target);
    }
    // Spend Holy Power if capped or near cap
    else if (holyPower >= 3)
    {
        SpendHolyPower(target);
    }
}

void PaladinAI::UpdateBlessingManagement()
{
    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime - _lastBlessingTime < 30000)  // Check every 30 seconds
        return;

    _lastBlessingTime = currentTime;

    // WoW 12.0: Most blessings are removed. Paladin now has situational blessings:
    // - Blessing of Freedom (movement utility)
    // - Blessing of Protection (physical immunity)
    // - Blessing of Sacrifice (damage sharing)
    // These are now used reactively in combat, not as persistent buffs.

    // Holy Paladin can use seasonal blessings (Holy::BLESSING_OF_SUMMER, etc.)
    // These are managed in the Holy specialization rotation.
}

void PaladinAI::UpdateAuraManagement()
{
    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime - _lastAuraChange < 10000)  // Check every 10 seconds
        return;

    // Select appropriate aura based on spec and situation
    ChrSpecialization spec = GetBot()->GetPrimarySpecialization();

    if (spec == ChrSpecialization::PaladinRetribution)
    {
        // Retribution Aura for damage reflection
    if (CanUseAbility(RETRIBUTION_AURA) && _currentAura != RETRIBUTION_AURA)
        {
            if (CastSpell(RETRIBUTION_AURA))
            {
                RecordAbilityUsage(RETRIBUTION_AURA);
                _currentAura = RETRIBUTION_AURA;
                _lastAuraChange = currentTime;
                return;
            }
        }
    }
    else if (spec == ChrSpecialization::PaladinProtection)
    {
        // Devotion Aura for damage reduction
    if (CanUseAbility(DEVOTION_AURA) && _currentAura != DEVOTION_AURA)
        {
            if (CastSpell(DEVOTION_AURA))
            {
                RecordAbilityUsage(DEVOTION_AURA);
                _currentAura = DEVOTION_AURA;
                _lastAuraChange = currentTime;
                return;
            }
        }
    }

    // Crusader Aura for movement speed (when out of combat)
    if (!_inCombat && CanUseAbility(CRUSADER_AURA) && _currentAura != CRUSADER_AURA)
    {
        if (CastSpell(CRUSADER_AURA))
        {
            RecordAbilityUsage(CRUSADER_AURA);
            _currentAura = CRUSADER_AURA;
            _lastAuraChange = currentTime;
        }
    }
}

uint32 PaladinAI::GetHolyPower() const
{
    if (!GetBot())
        return 0;

    // Holy Power is stored as POWER_HOLY_POWER
    return GetBot()->GetPower(POWER_HOLY_POWER);
}

bool PaladinAI::HasHolyPowerFor(uint32 spellId) const
{
    uint32 requiredPower = 0;

    // Determine Holy Power cost based on spell
    switch (spellId)
    {
        case TEMPLARS_VERDICT:
        case DIVINE_STORM:
        case WORD_OF_GLORY:
        case SHIELD_OF_THE_RIGHTEOUS:
            requiredPower = 3;
            break;
        case FINAL_VERDICT:
            requiredPower = 3;
            break;
        default:
            return true;  // No Holy Power requirement
    }

    return GetHolyPower() >= requiredPower;
}

void PaladinAI::GenerateHolyPower(::Unit* target)
{
    if (!target || !GetBot())
        return;

    // Wake of Ashes generates 5 Holy Power
    if (CanUseAbility(WAKE_OF_ASHES))
    {
        if (CastSpell(WAKE_OF_ASHES))
        {
            RecordAbilityUsage(WAKE_OF_ASHES);
            _paladinMetrics.holyPowerGenerated += 5;
            return;
        }
    }

    // Blade of Justice generates 2 Holy Power
    if (CanUseAbility(BLADE_OF_JUSTICE))
    {
        if (CastSpell(BLADE_OF_JUSTICE, target))
        {
            RecordAbilityUsage(BLADE_OF_JUSTICE);
            _paladinMetrics.holyPowerGenerated += 2;
            return;
        }
    }

    // Crusader Strike generates 1 Holy Power
    if (CanUseAbility(CRUSADER_STRIKE))
    {
        if (CastSpell(CRUSADER_STRIKE, target))
        {
            RecordAbilityUsage(CRUSADER_STRIKE);
            _paladinMetrics.holyPowerGenerated += 1;
            return;
        }
    }

    // Hammer of the Righteous for Protection (AoE generator, spec ID 1)
    if (GetBot()->GetPrimarySpecialization() == ChrSpecialization::PaladinProtection && CanUseAbility(HAMMER_OF_THE_RIGHTEOUS))
    {
        if (CastSpell(HAMMER_OF_THE_RIGHTEOUS, target))
        {
            RecordAbilityUsage(HAMMER_OF_THE_RIGHTEOUS);
            _paladinMetrics.holyPowerGenerated += 1;
            return;
        }
    }

    // Judgment generates 1 Holy Power
    if (CanUseAbility(JUDGMENT))
    {
        if (CastSpell(JUDGMENT, target))
        {
            RecordAbilityUsage(JUDGMENT);
            _paladinMetrics.holyPowerGenerated += 1;
            return;
        }
    }
}

void PaladinAI::SpendHolyPower(::Unit* target)
{
    if (!target || !GetBot())
        return;

    uint32 holyPower = GetHolyPower();
    if (holyPower < 3)
        return;

    // Protection: Shield of the Righteous for mitigation (spec ID 1)
    if (GetBot()->GetPrimarySpecialization() == ChrSpecialization::PaladinProtection)
    {
        if (CanUseAbility(SHIELD_OF_THE_RIGHTEOUS))
        {
            if (CastSpell(SHIELD_OF_THE_RIGHTEOUS))
            {
                RecordAbilityUsage(SHIELD_OF_THE_RIGHTEOUS);
                _paladinMetrics.holyPowerSpent += 3;
                return;
            }
        }
    }

    // Holy: Word of Glory for healing (spec ID 0 or low health)
    if (GetBot()->GetPrimarySpecialization() == ChrSpecialization::PaladinHoly || GetBot()->GetHealthPct() < 50.0f)
    {
        if (CanUseAbility(WORD_OF_GLORY))
        {
            // Heal self or lowest health ally
            Unit* healTarget = GetBot();
            if (IsAllyInDanger())
            {
                // Find lowest health ally
                Group* group = GetBot()->GetGroup();
                if (group)
                {
                    float lowestHealth = 100.0f;
                    for (GroupReference const& itr : group->GetMembers())
                    {
                        Player* member = itr.GetSource();
                        if (member && member->GetHealthPct() < lowestHealth)
                        {
                            lowestHealth = member->GetHealthPct();
                            healTarget = member;
                        }
                    }
                }
            }

            if (CastSpell(WORD_OF_GLORY, healTarget))
            {
                RecordAbilityUsage(WORD_OF_GLORY);
                _paladinMetrics.holyPowerSpent += 3;
                return;
            }
        }
    }

    // Retribution: Damage abilities
    uint32 nearbyEnemies = GetNearbyEnemyCount(DIVINE_STORM_RADIUS);

    // Divine Storm for AoE
    if (nearbyEnemies >= 2 && CanUseAbility(DIVINE_STORM))
    {
        if (CastSpell(DIVINE_STORM))
        {
            RecordAbilityUsage(DIVINE_STORM);
            _paladinMetrics.holyPowerSpent += 3;
            return;
        }
    }

    // Templar's Verdict for single target
    if (CanUseAbility(TEMPLARS_VERDICT))
    {
        if (CastSpell(TEMPLARS_VERDICT, target))
        {
            RecordAbilityUsage(TEMPLARS_VERDICT);
            _paladinMetrics.holyPowerSpent += 3;
            return;
        }
    }

    // Final Verdict (if talented)
    if (CanUseAbility(FINAL_VERDICT))
    {
        if (CastSpell(FINAL_VERDICT, target))
        {
            RecordAbilityUsage(FINAL_VERDICT);
            _paladinMetrics.holyPowerSpent += 3;
            return;
        }
    }
}

bool PaladinAI::ShouldBuildHolyPower() const
{
    return GetHolyPower() < 3;
}

bool PaladinAI::IsInMeleeRange(::Unit* target) const
{
    if (!target || !GetBot())
        return false;

    return GetBot()->GetDistance(target) <= OPTIMAL_MELEE_RANGE;
}

bool PaladinAI::CanInterrupt(::Unit* target) const
{
    if (!target || !GetBot())
        return false;

    // Check if target is casting and interruptible
    return target->HasUnitState(UNIT_STATE_CASTING);
}

uint32 PaladinAI::GetNearbyEnemyCount(float range) const
{
    if (!GetBot())
        return 0;

    uint32 count = 0;
    ::std::list<Unit*> targets;
    Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(GetBot(), GetBot(), range);
    Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(GetBot(), targets, u_check);
    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Map* map = GetBot()->GetMap();
    if (!map)
        return false;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return false;
    }

    // Query nearby GUIDs (lock-free!)
    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        GetBot()->GetPosition(), range);

    // Process results (replace old searcher logic)
    for (ObjectGuid guid : nearbyGuids)
    {
        // PHASE 5F: Thread-safe spatial grid validation
        auto snapshot_entity = SpatialGridQueryHelpers::FindCreatureByGuid(GetBot(), guid);

        Creature* entity = nullptr;
        if (snapshot_entity)
        {

        } snapshot_entity = SpatialGridQueryHelpers::FindCreatureByGuid(GetBot(), guid);
 entity = nullptr;
 if (snapshot_entity)
 {
 }
        if (!entity)
            continue;
        // Original filtering logic from searcher goes here
    }
    // End of spatial grid fix
    for (auto& target : targets)
    {
        if (GetBot()->IsValidAttackTarget(target))
            count++;
    }

    return count;
}

uint32 PaladinAI::GetNearbyAllyCount(float range) const
{
    if (!GetBot())
        return 0;

    uint32 count = 0;
    ::std::list<Unit*> allies;
    Trinity::AnyFriendlyUnitInObjectRangeCheck u_check(GetBot(), GetBot(), range);
    Trinity::UnitListSearcher<Trinity::AnyFriendlyUnitInObjectRangeCheck> searcher(GetBot(), allies, u_check);
    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Map* map = GetBot()->GetMap();
    if (!map)
        return false;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return false;
    }

    // Query nearby GUIDs (lock-free!)
    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        GetBot()->GetPosition(), range);

    // Process results (replace old searcher logic)
    for (ObjectGuid guid : nearbyGuids)
    {
        // PHASE 5F: Thread-safe spatial grid validation
        auto snapshot_entity = SpatialGridQueryHelpers::FindCreatureByGuid(GetBot(), guid);

        Creature* entity = nullptr;
        if (snapshot_entity)
        {

        } snapshot_entity = SpatialGridQueryHelpers::FindCreatureByGuid(GetBot(), guid);
 entity = nullptr;
 if (snapshot_entity)
 {
 }
        if (!entity)
            continue;
        // Original filtering logic from searcher goes here
    }
    // End of spatial grid fix

    return allies.size();
}

bool PaladinAI::IsAllyInDanger() const
{
    if (!GetBot())
        return false;

    Group* group = GetBot()->GetGroup();
    if (!group)
        return false;

    for (GroupReference const& itr : group->GetMembers())
    {
        Player* member = itr.GetSource();
        if (member && member != GetBot() && member->GetHealthPct() < 40.0f)
            return true;
    }

    return false;
}

bool PaladinAI::ShouldUseLayOnHands() const
{
    if (!GetBot())        return false;    // Use on self if critical
    if (GetBot()->GetHealthPct() < LAY_ON_HANDS_THRESHOLD)        return true;

    // Use on tank if critical
    Group* group = GetBot()->GetGroup();
    if (group)
    {
        for (GroupReference const& itr : group->GetMembers())
        {
            Player* member = itr.GetSource();
            if (!member || member == GetBot())
                continue;

            // Check if member is a tank using RoleDefinitions
            uint8 classId = member->GetClass();
            uint8 specId = static_cast<uint8>(member->GetPrimarySpecialization());
            GroupRole memberRole = RoleDefinitions::GetPrimaryRole(classId, specId);

            // Prioritize tanks for Lay on Hands (they keep the group alive)
            if (memberRole == GroupRole::TANK && member->GetHealthPct() < LAY_ON_HANDS_THRESHOLD)
                return true;
        }
    }

    return false;
}

Position PaladinAI::CalculateOptimalMeleePosition(::Unit* target)
{
    if (!target || !GetBot())
        return Position();

    // Get behind target for Retribution DPS
    float angle = target->GetOrientation() + static_cast<float>(M_PI);  // Behind target
    if (GetBot()->GetPrimarySpecialization() == ChrSpecialization::PaladinProtection)  // Protection spec ID
        angle = target->GetOrientation();  // Face target for tanking

    float x = target->GetPositionX() + cos(angle) * OPTIMAL_MELEE_RANGE;
    float y = target->GetPositionY() + sin(angle) * OPTIMAL_MELEE_RANGE;
    float z = target->GetPositionZ();

    return Position(x, y, z, angle);
}

bool PaladinAI::IsValidTarget(::Unit* target)
{
    return target && target->IsAlive() && GetBot()->IsValidAttackTarget(target);
}

void PaladinAI::RecordAbilityUsage(uint32 spellId)
{
    _abilityUsage[spellId]++;
    _paladinMetrics.totalAbilitiesUsed++;
}

void PaladinAI::RecordInterruptAttempt(::Unit* target, uint32 spellId, bool success)
{
    if (success)
    {
        _successfulInterrupts++;
        TC_LOG_DEBUG("module.playerbot.ai", "Paladin {} successfully interrupted with spell {}",
                     GetBot()->GetName(), spellId);
    }
}

void PaladinAI::AnalyzeCombatEffectiveness()
{
    if (_paladinMetrics.holyPowerGenerated > 0)
    {
        _paladinMetrics.holyPowerEfficiency =
            static_cast<float>(_paladinMetrics.holyPowerSpent) / _paladinMetrics.holyPowerGenerated;
    }

    TC_LOG_DEBUG("module.playerbot.ai",
                 "Paladin {} combat stats - Abilities: {}, HP Generated: {}, HP Spent: {}, Efficiency: {:.2f}",
                 GetBot()->GetName(),
                 _paladinMetrics.totalAbilitiesUsed,
                 _paladinMetrics.holyPowerGenerated,
                 _paladinMetrics.holyPowerSpent,
                 _paladinMetrics.holyPowerEfficiency);
}

void PaladinAI::UpdateMetrics(uint32 diff)
{
    _paladinMetrics.lastMetricsUpdate = ::std::chrono::steady_clock::now();
}

float PaladinAI::CalculateHolyPowerEfficiency()
{
    if (_paladinMetrics.holyPowerGenerated == 0)
        return 0.0f;

    return static_cast<float>(_paladinMetrics.holyPowerSpent) / _paladinMetrics.holyPowerGenerated;
}

} // namespace Playerbot