/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PriestAI.h"
#include "DisciplinePriest.h"
#include "HolyPriest.h"
#include "ShadowPriest.h"
#include "Player.h"
#include "Unit.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "Map.h"
#include "Group.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "SpellAuras.h"
#include "MotionMaster.h"
#include "../BaselineRotationManager.h"
#include "../../Combat/CombatBehaviorIntegration.h"
#include <algorithm>
#include "../../../Spatial/SpatialGridQueryHelpers.h"  // PHASE 5F: Thread-safe queries
#include "Movement/UnifiedMovementCoordinator.h"
#include "../../../Movement/Arbiter/MovementPriorityMapper.h"
#include "../../BotAI.h"
#include "Core/PlayerBotHelpers.h"
#include "UnitAI.h"
#include "GameTime.h"

namespace Playerbot
{

// Namespace aliases for central spell registry
namespace PriestCommon = WoW120Spells::Priest::Common;
namespace PriestDiscipline = WoW120Spells::Priest::Discipline;
namespace PriestHoly = WoW120Spells::Priest::HolyPriest;
namespace PriestShadow = WoW120Spells::Priest::Shadow;

// Talent IDs for specialization detection
// NOTE: Renamed to avoid conflict with PriestTalents namespace in PriestTalentEnhancements.h
enum PriestTalentIds
{
    // Holy talents
    TALENT_SPIRIT_OF_REDEMPTION = 20711,
    TALENT_CIRCLE_OF_HEALING = 34861,
    TALENT_GUARDIAN_SPIRIT = 47788,

    // Discipline talents
    TALENT_POWER_INFUSION = 10060,
    TALENT_PAIN_SUPPRESSION = 33206,
    TALENT_PENANCE = 47540,

    // Shadow talents
    TALENT_SHADOWFORM = 15473,
    TALENT_VAMPIRIC_EMBRACE = 15286,
    TALENT_VAMPIRIC_TOUCH = 34914,
    TALENT_DISPERSION = 47585
};

// ============================================================================
// CONSTRUCTOR/DESTRUCTOR
// ============================================================================

PriestAI::PriestAI(Player* bot) : ClassAI(bot),    _manaSpent(0),
    _healingDone(0),
    _damageDealt(0),
    _playersHealed(0),
    _damagePrevented(0),
    _lastDispel(0),
    _lastFearWard(0),
    _lastPsychicScream(0),
    _lastInnerFire(0),
    _lastSilence(0),
    _lastMassDispel(0),
    _lastDesperatePrayer(0),
    _lastFade(0)
{
    TC_LOG_DEBUG("module.playerbot.ai", "PriestAI created for player {}",

                 bot ? bot->GetName() : "null");
}

PriestAI::~PriestAI() = default;

// ============================================================================
// UPDATE ROTATION - Core delegation logic
// ============================================================================

void PriestAI::UpdateRotation(::Unit* target)
{
    if (!GetBot() || !target)
        return;

    // NOTE: Baseline rotation check is now handled at the dispatch level in
    // ClassAI::OnCombatUpdate(). This method is ONLY called when the bot has
    // already chosen a specialization (level 10+ with talents).

    // Update shared priest mechanics
    UpdatePriestBuffs();

    // ========================================================================
    // COMBAT BEHAVIOR INTEGRATION - Full priority system (EXECUTES FIRST)
    // ========================================================================

    // Execute combat behavior priorities - if any return true, they handled the action
    if (HandleCombatBehaviorPriorities(target))
        return;

    // Track combat metrics
    if (GetBot()->IsInCombat())
    {
        _damageDealt += CalculateDamageDealt(target);
        _healingDone += CalculateHealingDone();
        _manaSpent += CalculateManaUsage();
    }
}

void PriestAI::UpdateBuffs()
{
    if (!GetBot())
        return;

    // NOTE: Baseline buff check is now handled at the dispatch level.
    // This method is only called for level 10+ bots with talents.

    UpdatePriestBuffs();
    UpdateFortitudeBuffs();
}

void PriestAI::UpdateCooldowns(uint32 diff)
{
    // Base cooldown tracking is handled by ClassAI
    ClassAI::UpdateCooldowns(diff);
}

bool PriestAI::CanUseAbility(uint32 spellId)
{
    if (!GetBot())
        return false;

    // Check basic requirements
    if (!this->IsSpellReady(spellId) || !HasEnoughResource(spellId))
        return false;

    // Check priest-specific conditions
    if (IsHealingSpell(spellId) && ShouldConserveMana())
        return false;

    return true;
}

void PriestAI::OnCombatStart(::Unit* target)
{
    if (!GetBot() || !target)
        return;

    TC_LOG_DEBUG("module.playerbot.ai", "Priest {} entering combat with {}",

                 GetBot()->GetName(), target->GetName());

    // Initialize combat tracking
    _manaSpent = 0;
    _healingDone = 0;
    _damageDealt = 0;
    _playersHealed = 0;
}

void PriestAI::OnCombatEnd()
{
    if (!GetBot())
        return;

    TC_LOG_DEBUG("module.playerbot.ai", "Priest {} leaving combat. Metrics - Healing: {}, Damage: {}, Mana Used: {}, Players Healed: {}",

                 GetBot()->GetName(), _healingDone, _damageDealt, _manaSpent, _playersHealed);

    // Post-combat healing
    HealGroupMembers();

    // Reapply buffs if needed
    UpdatePriestBuffs();

    // Log performance metrics
    AnalyzeHealingEfficiency();
}

bool PriestAI::HasEnoughResource(uint32 spellId)
{
    if (!GetBot())
        return false;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    // Check mana cost
    auto powerCosts = spellInfo->CalcPowerCost(GetBot(), spellInfo->GetSchoolMask());
    for (auto const& cost : powerCosts)
    {
        if (cost.Power == POWER_MANA && GetBot()->GetPower(POWER_MANA) < int32(cost.Amount))

            return false;
    }

    // Check for mana conservation mode
    if (ShouldConserveMana() && !IsEmergencyHeal(spellId))
    {
        float manaPercent = GetManaPercent();
        if (manaPercent < MANA_CONSERVATION_THRESHOLD * 100.0f)

            return false;
    }

    return true;
}

void PriestAI::ConsumeResource(uint32 spellId)
{
    if (!GetBot())
        return;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return;

    // Track mana consumption
    auto powerCosts = spellInfo->CalcPowerCost(GetBot(), spellInfo->GetSchoolMask());
    for (auto const& cost : powerCosts)
    {
        if (cost.Power == POWER_MANA)

            _manaSpent += cost.Amount;
    }

    // Track specific spell usage
    if (IsHealingSpell(spellId))
    {
        _playersHealed++;
    }
    else if (IsDamageSpell(spellId))
    {
        _damageDealt += 100; // Simplified
    }
}

Position PriestAI::GetOptimalPosition(::Unit* target)
{
    if (!GetBot() || !target)
        return Position();

    // Default positioning for priests - maintain range
    float optimalRange = GetOptimalRange(target);
    float angle = GetBot()->GetAbsoluteAngle(target);

    // Position behind and to the side for safety
    angle += static_cast<float>(M_PI) / 4.0f; // 45 degrees offset

    float x = target->GetPositionX() - optimalRange * ::std::cos(angle);
    float y = target->GetPositionY() - optimalRange * ::std::sin(angle);    float z = target->GetPositionZ();    return Position(x, y, z);
}

float PriestAI::GetOptimalRange(::Unit* target)
{
    if (!GetBot() || !target)
        return OPTIMAL_HEALING_RANGE;

    // Default range based on current specialization
    ChrSpecialization currentSpec = GetBot()->GetPrimarySpecialization();

    // Shadow priest prefers DPS range
    if (currentSpec == ChrSpecialization::PriestShadow)
        return OPTIMAL_DPS_RANGE;

    // Discipline (spec 0) and Holy (spec 1) prefer healing range
    return OPTIMAL_HEALING_RANGE;
}

// ============================================================================
// COMBAT BEHAVIOR PRIORITIES - Main dispatch handler
// ============================================================================

bool PriestAI::HandleCombatBehaviorPriorities(::Unit* target)
{
    if (!GetBot() || !target)
        return false;

    auto* behaviors = GetCombatBehaviors();
    if (!behaviors)
        return ExecuteNormalRotation(target);

    // Priority 1: Interrupts (Silence for Shadow spec)
    if (HandleInterruptPriority(target))
        return true;

    // Priority 2: Defensives (Dispersion, Desperate Prayer, Fade)
    if (HandleDefensivePriority())
        return true;

    // Priority 3: Positioning - Maintain max range for safety
    if (HandlePositioningPriority(target))
        return true;

    // Priority 4: Dispel - Mass Dispel, Purify for group members
    if (HandleDispelPriority())
        return true;

    // Priority 5: Target Switching - Priority targets
    if (HandleTargetSwitchPriority(target))
        return true;

    // Priority 6: Crowd Control - Psychic Scream for emergencies
    if (HandleCrowdControlPriority(target))
        return true;

    // Priority 7: AoE Decisions - Delegate to spec
    if (HandleAoEPriority(target))
        return true;

    // Priority 8: Offensive Cooldowns - Delegate to spec
    if (HandleCooldownPriority(target))
        return true;

    // Priority 9: Resource Management - Mana conservation
    if (HandleResourceManagement())
        return true;

    // Priority 10: Normal Rotation - Delegate to spec
    return ExecuteNormalRotation(target);
}

// ============================================================================
// PRIORITY HANDLER METHODS (10 handlers)
// ============================================================================

bool PriestAI::HandleInterruptPriority(::Unit* target)
{
    if (!GetBot() || !target)
        return false;

    auto* behaviors = GetCombatBehaviors();
    if (!behaviors || !behaviors->ShouldInterrupt(target))
        return false;

    // Shadow priests have Silence
    ChrSpecialization currentSpec = GetBot()->GetPrimarySpecialization();
    if (currentSpec == ChrSpecialization::PriestShadow)
    {
        Unit* interruptTarget = behaviors->GetInterruptTarget();
        if (!interruptTarget)

            interruptTarget = target;

        if (interruptTarget && interruptTarget->IsNonMeleeSpellCast(false))
        {
            // Use Silence
    if (this->IsSpellReady(SILENCE) && GameTime::GetGameTimeMS() - _lastSilence > 45000)

            {

                if (this->CastSpell(SILENCE, interruptTarget))

                {

                    _lastSilence = GameTime::GetGameTimeMS();

                    TC_LOG_DEBUG("module.playerbot.ai", "Priest {} silenced {}",

                                 GetBot()->GetName(), interruptTarget->GetName());

                    return true;

                }

            }
        }
    }

    return false;
}

bool PriestAI::HandleDefensivePriority()
{
    if (!GetBot())
        return false;

    auto* behaviors = GetCombatBehaviors();
    if (!behaviors || !behaviors->NeedsDefensive())
        return false;

    float healthPct = GetBot()->GetHealthPct();

    // Emergency: Dispersion for Shadow (90% damage reduction)
    ChrSpecialization currentSpec = GetBot()->GetPrimarySpecialization();
    if (currentSpec == ChrSpecialization::PriestShadow && healthPct < 20.0f)
    {
        if (this->IsSpellReady(DISPERSION))
        {

            if (this->CastSpell(DISPERSION, GetBot()))

            {

                TC_LOG_DEBUG("module.playerbot.ai", "Priest {} used Dispersion (emergency)", GetBot()->GetName());

                return true;

            }
        }
    }

    // Desperate Prayer (self-heal + damage reduction)
    if (healthPct < 30.0f && GameTime::GetGameTimeMS() - _lastDesperatePrayer > 90000)
    {
        if (this->IsSpellReady(DESPERATE_PRAYER))
        {

            if (this->CastSpell(DESPERATE_PRAYER, GetBot()))

            {

                _lastDesperatePrayer = GameTime::GetGameTimeMS();

                TC_LOG_DEBUG("module.playerbot.ai", "Priest {} used Desperate Prayer", GetBot()->GetName());

                return true;

            }
        }
    }

    // Fade (threat reduction)
    if (healthPct < 50.0f && GetBot()->GetThreatManager().GetThreatListSize() > 0)
    {
        if (GameTime::GetGameTimeMS() - _lastFade > 30000)
        {

            CastFade();

            return true;
        }
    }

    return false;
}

bool PriestAI::HandlePositioningPriority(::Unit* target)
{
    if (!GetBot() || !target)
        return false;

    auto* behaviors = GetCombatBehaviors();
    if (!behaviors || !behaviors->NeedsRepositioning())
        return false;

    float currentDistance = GetBot()->GetDistance(target);
    float optimalRange = GetOptimalRange(target);

    // Use Psychic Scream if enemies are too close
    if (currentDistance < 8.0f && GameTime::GetGameTimeMS() - _lastPsychicScream > PSYCHIC_SCREAM_COOLDOWN)
    {
        CastPsychicScream();
        return true;
    }

    // Maintain optimal range
    if (currentDistance < optimalRange * 0.8f || currentDistance > optimalRange * 1.2f)
    {
        Position optimalPos = GetOptimalPosition(target);
        if (optimalPos.IsPositionValid())
        {
            // PHASE 6C: Use Movement Arbiter with ROLE_POSITIONING priority (170)

            BotAI* botAI = dynamic_cast<BotAI*>(GetBot()->GetAI());

            if (botAI && botAI->GetUnifiedMovementCoordinator())

            {

                botAI->RequestPointMovement(

                    PlayerBotMovementPriority::ROLE_POSITIONING,

                    optimalPos,

                    "Priest optimal range positioning",

                    "PriestAI");

            }

            else

            {
                // FALLBACK: Use BotMovementController with validated pathfinding
                if (BotAI* ai = GetBotAI(GetBot()))
                {
                    if (!ai->MoveTo(optimalPos, true))
                    {
                        // Final fallback to legacy if validation fails
                        GetBot()->GetMotionMaster()->MovePoint(0, optimalPos);
                    }
                }
                else
                {
                    // Non-bot player - use standard movement
                    GetBot()->GetMotionMaster()->MovePoint(0, optimalPos);
                }
            }

            return true;
        }
    }

    return false;
}

bool PriestAI::HandleDispelPriority()
{
    if (!GetBot())
        return false;

    // Check cooldown
    if (GameTime::GetGameTimeMS() - _lastDispel < DISPEL_COOLDOWN)
        return false;    // Check if there are dispellable debuffs in the group
    ::Unit* dispelTarget = GetBestDispelTarget();
    if (dispelTarget)
    {
        CastDispelMagic();
        return true;
    }

    return false;
}

bool PriestAI::HandleTargetSwitchPriority(::Unit*& target)
{
    if (!GetBot() || !target)
        return false;

    auto* behaviors = GetCombatBehaviors();
    if (!behaviors)
        return false;

    ::Unit* priorityTarget = behaviors->GetPriorityTarget();
    if (priorityTarget && priorityTarget != target)
    {
        target = priorityTarget;
        TC_LOG_DEBUG("module.playerbot.ai", "Priest {} switching to priority target {}",

                     GetBot()->GetName(), target->GetName());
        return true;
    }

    return false;
}

bool PriestAI::HandleCrowdControlPriority(::Unit* target)
{
    if (!GetBot() || !target)
        return false;

    auto* behaviors = GetCombatBehaviors();
    if (!behaviors || !behaviors->ShouldUseCrowdControl())
        return false;

    // Use Psychic Scream for emergency crowd control
    if (GetBot()->GetHealthPct() < 40.0f && GameTime::GetGameTimeMS() - _lastPsychicScream > PSYCHIC_SCREAM_COOLDOWN)
    {
        CastPsychicScream();
        return true;
    }

    return false;
}

bool PriestAI::HandleAoEPriority(::Unit* target)
{
    if (!GetBot() || !target)
        return false;

    auto* behaviors = GetCombatBehaviors();
    if (!behaviors || !behaviors->ShouldAOE())
        return false;

    // AoE handling is done in normal rotation
    return false;
}

bool PriestAI::HandleCooldownPriority(::Unit* target)
{
    if (!GetBot() || !target)
        return false;

    auto* behaviors = GetCombatBehaviors();
    if (!behaviors || !behaviors->ShouldUseCooldowns())
        return false;

    // Cooldown usage is handled in normal rotation
    return false;
}

bool PriestAI::HandleResourceManagement()
{
    if (!GetBot())
        return false;

    // Check if we need mana regeneration
    if (ShouldConserveMana())
    {
        OptimizeManaUsage();
    }

    return false; // Don't block rotation, just optimize
}

bool PriestAI::ExecuteNormalRotation(::Unit* target)
{
    if (!GetBot() || !target)
        return false;

    // Use BaselineRotationManager for rotation execution
    // QW-4 FIX: Use per-instance rotation manager instead of static
    return _rotationManager.ExecuteBaselineRotation(GetBot(), target);
}


// ============================================================================
// SHARED UTILITIES (70 lines)
// ============================================================================

void PriestAI::UpdatePriestBuffs()
{
    if (!GetBot())
        return;

    // Maintain Power Word: Fortitude
    if (!GetBot()->HasAura(POWER_WORD_FORTITUDE))
    {
        CastPowerWordFortitude();
    }

    // Maintain Power Word: Shield if not on cooldown (Weakened Soul check)
    if (GetBot()->GetHealthPct() < 80.0f && !GetBot()->HasAura(POWER_WORD_SHIELD))
    {
        if (this->IsSpellReady(POWER_WORD_SHIELD))
        {
            this->CastSpell(POWER_WORD_SHIELD, GetBot());
        }
    }

    // Note: Inner Fire, Divine Spirit, and Shadow Protection were removed in WoW 12.0
}

void PriestAI::CastInnerFire()
{
    // Inner Fire was removed in WoW 12.0 - this method is kept for backward compatibility
    // but does nothing in modern WoW
    (void)_lastInnerFire; // Suppress unused variable warning
}

void PriestAI::UpdateFortitudeBuffs()
{
    if (!GetBot())
        return;

    // Check group members for fortitude
    if (Group* group = GetBot()->GetGroup())
    {
        uint32 unbuffedCount = CountUnbuffedGroupMembers(POWER_WORD_FORTITUDE);
        if (unbuffedCount > 0)
        {

            CastPowerWordFortitude();
        }
    }
}

void PriestAI::CastPowerWordFortitude()
{
    if (!GetBot() || !this->IsSpellReady(POWER_WORD_FORTITUDE))
        return;

    this->CastSpell(POWER_WORD_FORTITUDE, GetBot());
}

bool PriestAI::HasEnoughMana(uint32 amount)
{
    return GetBot() && GetBot()->GetPower(POWER_MANA) >= int32(amount);
}

uint32 PriestAI::GetMana()
{
    return GetBot() ? GetBot()->GetPower(POWER_MANA) : 0;
}

uint32 PriestAI::GetMaxMana()
{
    return GetBot() ? GetBot()->GetMaxPower(POWER_MANA) : 0;
}

float PriestAI::GetManaPercent()
{
    uint32 maxMana = GetMaxMana();
    return maxMana > 0 ? (float(GetMana()) / float(maxMana) * 100.0f) : 0.0f;
}

void PriestAI::OptimizeManaUsage()
{
    if (!GetBot())
        return;

    float manaPercent = GetManaPercent();

    // Use mana regeneration abilities
    if (manaPercent < 20.0f)
    {
        UseManaRegeneration();
    }
}

bool PriestAI::ShouldConserveMana()
{
    return GetManaPercent() < MANA_CONSERVATION_THRESHOLD * 100.0f;
}

void PriestAI::UseManaRegeneration()
{
    if (!GetBot())
        return;

    // Use Symbol of Hope if available (Holy spec only, replaces old Hymn of Hope)
    if (this->IsSpellReady(SYMBOL_OF_HOPE))
    {
        this->CastSpell(SYMBOL_OF_HOPE, GetBot());
    }
}

void PriestAI::CastPsychicScream()
{
    if (!GetBot() || !this->IsSpellReady(PSYCHIC_SCREAM))
        return;

    if (GameTime::GetGameTimeMS() - _lastPsychicScream < PSYCHIC_SCREAM_COOLDOWN)
        return;

    if (this->CastSpell(PSYCHIC_SCREAM, GetBot()))
    {
        _lastPsychicScream = GameTime::GetGameTimeMS();
    }
}

void PriestAI::CastFade()
{
    if (!GetBot() || !this->IsSpellReady(FADE))
        return;

    if (this->CastSpell(FADE, GetBot()))
    {
        _lastFade = GameTime::GetGameTimeMS();
    }
}

void PriestAI::CastDispelMagic()
{
    if (!GetBot())
        return;

    ::Unit* target = GetBestDispelTarget();
    if (target && this->IsSpellReady(DISPEL_MAGIC))
    {
        if (GameTime::GetGameTimeMS() - _lastDispel > DISPEL_COOLDOWN)
        {

            if (this->CastSpell(DISPEL_MAGIC, target))

            {

                _lastDispel = GameTime::GetGameTimeMS();

            }
        }
    }
}

void PriestAI::CastFearWard()
{
    // Fear Ward was removed in WoW 12.0 - this method is kept for backward compatibility
    // but does nothing in modern WoW
    (void)_lastFearWard; // Suppress unused variable warning
}
void PriestAI::CastDesperatePrayer()
{
    if (!GetBot() || !this->IsSpellReady(DESPERATE_PRAYER))
        return;

    if (this->CastSpell(DESPERATE_PRAYER, GetBot()))
    {
        _lastDesperatePrayer = GameTime::GetGameTimeMS();
    }
}

::Unit* PriestAI::GetBestHealTarget()
{
    if (!GetBot())
        return nullptr;

    ::Unit* lowestHealthTarget = nullptr;
    float lowestHealthPct = 100.0f;

    // Check self first
    if (GetBot()->GetHealthPct() < EMERGENCY_HEALTH_THRESHOLD * 100.0f)
        return GetBot();

    // Check group members
    if (Group* group = GetBot()->GetGroup())
    {
        for (GroupReference const& ref : group->GetMembers())
        {

            Player* player = ref.GetSource();
            if (!player || !player->IsAlive())

                continue;


            float healthPct = player->GetHealthPct();

            if (healthPct < lowestHealthPct && GetBot()->GetDistance(player) <= OPTIMAL_HEALING_RANGE)

            {

                lowestHealthPct = healthPct;

                lowestHealthTarget = player;

            }
        }
    }

    // Return target if they need healing
    if (lowestHealthTarget && lowestHealthPct < 80.0f)
        return lowestHealthTarget;

    return nullptr;
}

::Unit* PriestAI::GetBestDispelTarget()
{
    if (!GetBot())
        return nullptr;

    // Priority: Self > Tank > Healer > DPS

    // Check self for dispellable debuffs
    if (HasDispellableDebuff(GetBot()))
        return GetBot();

    // Check group members
    if (Group* group = GetBot()->GetGroup())
    {
        // First pass - tanks
    for (GroupReference const& ref : group->GetMembers())
        {

            Player* player = ref.GetSource();

            if (player && IsTank(player) && HasDispellableDebuff(player))

                return player;
        }

        // Second pass - healers
    for (GroupReference const& ref : group->GetMembers())
        {

            Player* player = ref.GetSource();

            if (player && IsHealer(player) && HasDispellableDebuff(player))

                return player;
        }

        // Third pass - any member
    for (GroupReference const& ref : group->GetMembers())
        {

            Player* player = ref.GetSource();
            if (player && HasDispellableDebuff(player))

                return player;
        }
    }

    return nullptr;
}

::Unit* PriestAI::FindGroupTank()
{
    if (!GetBot())
        return nullptr;

    if (Group* group = GetBot()->GetGroup())
    {
        for (GroupReference const& ref : group->GetMembers())
        {

            Player* player = ref.GetSource();

            if (player && IsTank(player))

                return player;
        }
    }

    return nullptr;
}

::Unit* PriestAI::GetLowestHealthAlly(float maxRange)
{
    if (!GetBot())
        return nullptr;

    ::Unit* lowestHealthTarget = nullptr;
    float lowestHealthPct = 100.0f;

    // Check self first
    if (GetBot()->GetHealthPct() < EMERGENCY_HEALTH_THRESHOLD * 100.0f)
        return GetBot();

    // Check group members
    if (Group* group = GetBot()->GetGroup())
    {
        for (GroupReference const& ref : group->GetMembers())
        {

            Player* player = ref.GetSource();
            if (!player || !player->IsAlive())
            continue;


            float healthPct = player->GetHealthPct();
            if (healthPct < lowestHealthPct && GetBot()->GetDistance(player) <= maxRange)

            {

                lowestHealthPct = healthPct;

                lowestHealthTarget = player;
                }
        }
    }

    // Return target if they need healing
    if (lowestHealthTarget && lowestHealthPct < 80.0f)
        return lowestHealthTarget;

    return nullptr;
}

bool PriestAI::HasDispellableDebuff(::Unit* unit)
{
    if (!unit)
        return false;

    // Check for magic debuffs
    Unit::AuraApplicationMap const& auras = unit->GetAppliedAuras();
    for (auto const& itr : auras)
    {
        Aura const* aura = itr.second->GetBase();
        if (aura && aura->GetSpellInfo()->Dispel == DISPEL_MAGIC && !itr.second->IsPositive())

            return true;
    }

    return false;
}

bool PriestAI::IsTank(::Unit* unit)
{
    if (!unit)
        return false;

    if (Player* player = unit->ToPlayer())    {
        uint8 playerClass = player->GetClass();
        return (playerClass == CLASS_WARRIOR || playerClass == CLASS_PALADIN || playerClass == CLASS_DEATH_KNIGHT);
    }

    return false;
}

bool PriestAI::IsHealer(::Unit* unit)
{
    if (!unit)
        return false;

    if (Player* player = unit->ToPlayer())
    {
        uint8 playerClass = player->GetClass();
        return (playerClass == CLASS_PRIEST || playerClass == CLASS_DRUID ||

                playerClass == CLASS_SHAMAN || playerClass == CLASS_PALADIN);
    }

    return false;
}

uint32 PriestAI::CountUnbuffedGroupMembers(uint32 spellId)
{
    if (!GetBot())
        return 0;

    uint32 count = 0;
    if (Group* group = GetBot()->GetGroup())
    {
        for (GroupReference const& ref : group->GetMembers())
        {

            Player* player = ref.GetSource();

            if (player && !player->HasAura(spellId))

                count++;
        }
    }

    return count;
}

bool PriestAI::IsHealingSpell(uint32 spellId)
{
    // Common healing spell IDs - Using central registry (WoW 12.0)
    return (spellId == PriestHoly::HOLY_WORD_SERENITY ||    // 2050 - Holy Word: Serenity
            spellId == PriestCommon::HEAL ||                 // 2060 - Heal
            spellId == PriestCommon::FLASH_HEAL ||           // 2061 - Flash Heal
            spellId == PriestCommon::RENEW ||                // 139 - Renew
            spellId == PriestHoly::PRAYER_OF_HEALING ||      // 596 - Prayer of Healing
            spellId == PriestCommon::PRAYER_OF_MENDING ||    // 33076 - Prayer of Mending
            spellId == PriestDiscipline::PENANCE ||          // 47540 - Penance
            spellId == PriestDiscipline::SHADOW_MEND ||      // 186263 - Shadow Mend
            spellId == PriestCommon::CIRCLE_OF_HEALING ||    // 204883 - Circle of Healing
            spellId == PriestHoly::HOLY_WORD_SANCTIFY ||     // 34861 - Holy Word: Sanctify
            spellId == PriestCommon::POWER_WORD_SHIELD);     // 17 - Power Word: Shield
}

bool PriestAI::IsDamageSpell(uint32 spellId)
{
    // Common damage spell IDs - Using central registry (WoW 12.0)
    return (spellId == PriestCommon::SHADOW_WORD_PAIN ||     // 589 - Shadow Word: Pain
            spellId == PriestCommon::MIND_BLAST ||           // 8092 - Mind Blast
            spellId == PriestCommon::MIND_FLAY ||            // 15407 - Mind Flay
            spellId == PriestCommon::VAMPIRIC_TOUCH ||       // 34914 - Vampiric Touch
            spellId == PriestCommon::SMITE ||                // 585 - Smite
            spellId == PriestCommon::HOLY_FIRE ||            // 14914 - Holy Fire
            spellId == PriestCommon::SHADOW_WORD_DEATH ||    // 32379 - Shadow Word: Death
            spellId == PriestCommon::DEVOURING_PLAGUE ||     // 335467 - Devouring Plague
            spellId == PriestShadow::VOID_BOLT ||            // 205448 - Void Bolt
            spellId == PriestShadow::VOID_ERUPTION);         // 228260 - Void Eruption
}

bool PriestAI::IsEmergencyHeal(uint32 spellId)
{
    // Fast heals for emergencies - Using central registry (WoW 12.0)
    return (spellId == PriestCommon::FLASH_HEAL ||           // 2061 - Flash Heal
            spellId == PriestCommon::DESPERATE_PRAYER ||     // 19236 - Desperate Prayer
            spellId == PriestHoly::GUARDIAN_SPIRIT ||        // 47788 - Guardian Spirit
            spellId == PriestDiscipline::PAIN_SUPPRESSION || // 33206 - Pain Suppression
            spellId == PriestCommon::POWER_WORD_SHIELD);     // 17 - Power Word: Shield
}

bool PriestAI::ShouldUseShadowProtection()
{
    // Would need encounter-specific logic
    return false;
}

uint32 PriestAI::CalculateDamageDealt(::Unit* target) const
{
    // Simplified damage tracking
    return 0;
}

uint32 PriestAI::CalculateHealingDone() const
{
    // Simplified healing tracking
    return 0;
}

uint32 PriestAI::CalculateManaUsage() const
{
    // Simplified mana tracking
    return 0;
}

void PriestAI::AnalyzeHealingEfficiency()
{
    // Performance analysis - could be expanded
}

void PriestAI::HealGroupMembers()
{
    // Post-combat group healing
    ::Unit* healTarget = GetBestHealTarget();
    if (healTarget)
    {
        // Execute healing rotation via BaselineRotationManager
        // QW-4 FIX: Use per-instance rotation manager instead of static
        _rotationManager.ExecuteBaselineRotation(GetBot(), healTarget);
    }
}

} // namespace Playerbot
