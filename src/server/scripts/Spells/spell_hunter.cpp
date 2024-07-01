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

/*
 * Scripts for spells with SPELLFAMILY_HUNTER, SPELLFAMILY_PET and SPELLFAMILY_GENERIC spells used by hunter players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_hun_".
 */

#include "ScriptMgr.h"
#include "CellImpl.h"
#include "GridNotifiersImpl.h"
#include "Pet.h"
#include "SpellAuraEffects.h"
#include "SpellHistory.h"
#include "SpellMgr.h"
#include "SpellScript.h"

#include "AreaTrigger.h"
#include "AreaTriggerAI.h"
#include "Cell.h"
#include "Creature.h"
#include "DB2Stores.h"
#include "GridNotifiers.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "PetPackets.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "TemporarySummon.h"
#include "Unit.h"

enum HunterSpells
{
    SPELL_HUNTER_A_MURDER_OF_CROWS_DAMAGE           = 131900,
    SPELL_HUNTER_A_MURDER_OF_CROWS_VISUAL_1         = 131637,
    SPELL_HUNTER_A_MURDER_OF_CROWS_VISUAL_2         = 131951,
    SPELL_HUNTER_A_MURDER_OF_CROWS_VISUAL_3         = 131952,
    SPELL_HUNTER_ASPECT_CHEETAH_SLOW                = 186258,
    SPELL_HUNTER_ASPECT_OF_THE_TURTLE_PACIFY_AURA   = 205769,
    SPELL_HUNTER_EXHILARATION                       = 109304,
    SPELL_HUNTER_EXHILARATION_PET                   = 128594,
    SPELL_HUNTER_EXHILARATION_R2                    = 231546,
    SPELL_HUNTER_EXPLOSIVE_SHOT_DAMAGE              = 212680,
    SPELL_HUNTER_LATENT_POISON_STACK                = 378015,
    SPELL_HUNTER_LATENT_POISON_DAMAGE               = 378016,
    SPELL_HUNTER_LATENT_POISON_INJECTORS_STACK      = 336903,
    SPELL_HUNTER_LATENT_POISON_INJECTORS_DAMAGE     = 336904,
    SPELL_HUNTER_LONE_WOLF                          = 155228,
    SPELL_HUNTER_MASTERS_CALL_TRIGGERED             = 62305,
    SPELL_HUNTER_MISDIRECTION                       = 34477,
    SPELL_HUNTER_MISDIRECTION_PROC                  = 35079,
    SPELL_HUNTER_MULTI_SHOT_FOCUS                   = 213363,
    SPELL_HUNTER_PET_LAST_STAND_TRIGGERED           = 53479,
    SPELL_HUNTER_PET_HEART_OF_THE_PHOENIX_TRIGGERED = 54114,
    SPELL_HUNTER_PET_HEART_OF_THE_PHOENIX_DEBUFF    = 55711,
    SPELL_HUNTER_POSTHASTE_INCREASE_SPEED           = 118922,
    SPELL_HUNTER_POSTHASTE_TALENT                   = 109215,
    SPELL_HUNTER_RAPID_FIRE_DAMAGE                  = 257045,
    SPELL_HUNTER_RAPID_FIRE_ENERGIZE                = 263585,
    SPELL_HUNTER_STEADY_SHOT_FOCUS                  = 77443,
    SPELL_HUNTER_T9_4P_GREATNESS                    = 68130,
    SPELL_HUNTER_T29_2P_MARKSMANSHIP_DAMAGE         = 394371,
    SPELL_ROAR_OF_SACRIFICE_TRIGGERED               = 67481,
    SPELL_DEATH_CHAKRAM_DAMAGE                      = 375893,
    SPELL_STAMPEDE_DAMAGE                           = 201594,
    SPELL_STAMPEDE_SOUND                            = 201591,
    SPELL_BLOODSHED_PROC                            = 321538,
    SPELL_VOLLEY_DAMAGE                             = 260247,
    SPELL_HUNTER_FLARE_EFFECT                       = 28822,
    SPELL_HUNTER_KILL_COMMAND                       = 34026,
    SPELL_HUNTER_KILL_COMMAND_TRIGGER               = 83381,
    SPELL_HUNTER_INTIMIDATION_STUN                  = 24394,
    SPELL_BARBED_SHOT_PLAYERAURA                    = 246152,
    SPELL_BARBED_SHOT_PETAURA                       = 272790,
    SPELL_HUNTER_WILD_CALL_AURA                     = 185791,
    SPELL_HUNTER_AURA_SHOOTING                      = 224729,
    SPELL_HUNTER_BARRAGE                            = 120360,
};

enum MiscSpells
{
    SPELL_DRAENEI_GIFT_OF_THE_NAARU                 = 59543,
};

enum DireBeastSpells
{
    DIRE_BEAST_DREAD_WASTES                         = 126216,
    DIRE_BEAST_DUNGEONS                             = 132764,
    DIRE_BEAST_EASTERN_KINGDOMS                     = 122804,
    DIRE_BEAST_JADE_FOREST                          = 121118,
    DIRE_BEAST_KALIMDOR                             = 122802,
    DIRE_BEAST_KRASARANG_WILDS                      = 122809,
    DIRE_BEAST_KUN_LAI_SUMMIT                       = 126214,
    DIRE_BEAST_NORTHREND                            = 122807,
    DIRE_BEAST_OUTLAND                              = 122806,
    DIRE_BEAST_TOWNLONG_STEPPES                     = 126215,
    DIRE_BEAST_VALE_OF_THE_ETERNAL_BLOSSOM          = 126213,
    DIRE_BEAST_VALLEY_OF_THE_FOUR_WINDS             = 122811,
};

enum MiscNpcs
{
    NPC_VOLLEY = 60942
};

// 131894 - A Murder of Crows
class spell_hun_a_murder_of_crows : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo
        ({
            SPELL_HUNTER_A_MURDER_OF_CROWS_DAMAGE,
            SPELL_HUNTER_A_MURDER_OF_CROWS_VISUAL_1,
            SPELL_HUNTER_A_MURDER_OF_CROWS_VISUAL_2,
            SPELL_HUNTER_A_MURDER_OF_CROWS_VISUAL_3
        });
    }

    void HandleDummyTick(AuraEffect const* /*aurEff*/)
    {
        Unit* target = GetTarget();

        if (Unit* caster = GetCaster())
            caster->CastSpell(target, SPELL_HUNTER_A_MURDER_OF_CROWS_DAMAGE, true);

        target->CastSpell(target, SPELL_HUNTER_A_MURDER_OF_CROWS_VISUAL_1, true);
        target->CastSpell(target, SPELL_HUNTER_A_MURDER_OF_CROWS_VISUAL_2, true);
        target->CastSpell(target, SPELL_HUNTER_A_MURDER_OF_CROWS_VISUAL_3, true);
        target->CastSpell(target, SPELL_HUNTER_A_MURDER_OF_CROWS_VISUAL_3, true); // not a mistake, it is intended to cast twice
    }

    void RemoveEffect(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_DEATH)
            if (Unit* caster = GetCaster())
                caster->GetSpellHistory()->ResetCooldown(GetId(), true);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_hun_a_murder_of_crows::HandleDummyTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        OnEffectRemove += AuraEffectRemoveFn(spell_hun_a_murder_of_crows::RemoveEffect, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// 186257 - Aspect of the Cheetah
class spell_hun_aspect_cheetah : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo
        ({
            SPELL_HUNTER_ASPECT_CHEETAH_SLOW
        });
    }

    void HandleOnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE)
            GetTarget()->CastSpell(GetTarget(), SPELL_HUNTER_ASPECT_CHEETAH_SLOW, true);
    }

    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(spell_hun_aspect_cheetah::HandleOnRemove, EFFECT_0, SPELL_AURA_MOD_INCREASE_SPEED, AURA_EFFECT_HANDLE_REAL);
    }
};

// 186265 - Aspect of the Turtle
class spell_hun_aspect_of_the_turtle : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_HUNTER_ASPECT_OF_THE_TURTLE_PACIFY_AURA });
    }

    void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->CastSpell(GetTarget(), SPELL_HUNTER_ASPECT_OF_THE_TURTLE_PACIFY_AURA, true);
    }

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->RemoveAurasDueToSpell(SPELL_HUNTER_ASPECT_OF_THE_TURTLE_PACIFY_AURA);
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(spell_hun_aspect_of_the_turtle::OnApply, EFFECT_0, SPELL_AURA_MOD_ATTACKER_MELEE_HIT_CHANCE, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(spell_hun_aspect_of_the_turtle::OnRemove, EFFECT_0, SPELL_AURA_MOD_ATTACKER_MELEE_HIT_CHANCE, AURA_EFFECT_HANDLE_REAL);
    }
};

// 378750 - Cobra Sting
class spell_hun_cobra_sting : public AuraScript
{
    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellEffect({ { spellInfo->Id, EFFECT_1 } });
    }

    bool RollProc(AuraEffect const* /*aurEff*/, ProcEventInfo& /*procInfo*/)
    {
        return roll_chance_i(GetEffect(EFFECT_1)->GetAmount());
    }

    void Register() override
    {
        DoCheckEffectProc += AuraCheckEffectProcFn(spell_hun_cobra_sting::RollProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

// 109304 - Exhilaration
class spell_hun_exhilaration : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_HUNTER_EXHILARATION_R2, SPELL_HUNTER_LONE_WOLF });
    }

    void HandleOnHit()
    {
        if (GetCaster()->HasAura(SPELL_HUNTER_EXHILARATION_R2) && !GetCaster()->HasAura(SPELL_HUNTER_LONE_WOLF))
            GetCaster()->CastSpell(nullptr, SPELL_HUNTER_EXHILARATION_PET, true);
    }

    void Register() override
    {
        OnHit += SpellHitFn(spell_hun_exhilaration::HandleOnHit);
    }
};

// 212431 - Explosive Shot
class spell_hun_explosive_shot : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_HUNTER_EXPLOSIVE_SHOT_DAMAGE });
    }

    void HandlePeriodic(AuraEffect const* /*aurEff*/)
    {
        if (Unit* caster = GetCaster())
            caster->CastSpell(GetTarget(), SPELL_HUNTER_EXPLOSIVE_SHOT_DAMAGE, true);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_hun_explosive_shot::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }
};

// 212658 - Hunting Party
class spell_hun_hunting_party : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
        {
            SPELL_HUNTER_EXHILARATION,
            SPELL_HUNTER_EXHILARATION_PET
        });
    }

    void HandleProc(AuraEffect* aurEff, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();
        GetTarget()->GetSpellHistory()->ModifyCooldown(SPELL_HUNTER_EXHILARATION, -Seconds(aurEff->GetAmount()));
        GetTarget()->GetSpellHistory()->ModifyCooldown(SPELL_HUNTER_EXHILARATION_PET, -Seconds(aurEff->GetAmount()));
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_hun_hunting_party::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// 53478 - Last Stand Pet
class spell_hun_last_stand_pet : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_HUNTER_PET_LAST_STAND_TRIGGERED });
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        CastSpellExtraArgs args(TRIGGERED_FULL_MASK);
        args.AddSpellBP0(caster->CountPctFromMaxHealth(30));
        caster->CastSpell(caster, SPELL_HUNTER_PET_LAST_STAND_TRIGGERED, args);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_hun_last_stand_pet::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 378016 - Latent Poison
class spell_hun_latent_poison_damage : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_HUNTER_LATENT_POISON_STACK });
    }

    void CalculateDamage()
    {
        if (Aura* stack = GetHitUnit()->GetAura(SPELL_HUNTER_LATENT_POISON_STACK, GetCaster()->GetGUID()))
        {
            SetHitDamage(GetHitDamage() * stack->GetStackAmount());
            stack->Remove();
        }
    }

    void Register() override
    {
        OnHit += SpellHitFn(spell_hun_latent_poison_damage::CalculateDamage);
    }
};

// 19434 - Aimed Shot
// 186270 - Raptor Strike
// 217200 - Barbed Shot
// 259387 - Mongoose Bite
class spell_hun_latent_poison_trigger : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_HUNTER_LATENT_POISON_STACK, SPELL_HUNTER_LATENT_POISON_DAMAGE });
    }

    void TriggerDamage()
    {
        if (GetHitUnit()->HasAura(SPELL_HUNTER_LATENT_POISON_STACK, GetCaster()->GetGUID()))
            GetCaster()->CastSpell(GetHitUnit(), SPELL_HUNTER_LATENT_POISON_DAMAGE, GetSpell());
    }

    void Register() override
    {
        AfterHit += SpellHitFn(spell_hun_latent_poison_trigger::TriggerDamage);
    }
};

// 336904 - Latent Poison Injectors
class spell_hun_latent_poison_injectors_damage : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_HUNTER_LATENT_POISON_INJECTORS_STACK });
    }

    void CalculateDamage()
    {
        if (Aura* stack = GetHitUnit()->GetAura(SPELL_HUNTER_LATENT_POISON_INJECTORS_STACK, GetCaster()->GetGUID()))
        {
            SetHitDamage(GetHitDamage() * stack->GetStackAmount());
            stack->Remove();
        }
    }

    void Register() override
    {
        OnHit += SpellHitFn(spell_hun_latent_poison_injectors_damage::CalculateDamage);
    }
};

// 186270 - Raptor Strike
// 259387 - Mongoose Bite
class spell_hun_latent_poison_injectors_trigger : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_HUNTER_LATENT_POISON_INJECTORS_STACK, SPELL_HUNTER_LATENT_POISON_INJECTORS_DAMAGE });
    }

    void TriggerDamage()
    {
        if (GetHitUnit()->HasAura(SPELL_HUNTER_LATENT_POISON_INJECTORS_STACK, GetCaster()->GetGUID()))
            GetCaster()->CastSpell(GetHitUnit(), SPELL_HUNTER_LATENT_POISON_INJECTORS_DAMAGE, GetSpell());
    }

    void Register() override
    {
        AfterHit += SpellHitFn(spell_hun_latent_poison_injectors_trigger::TriggerDamage);
    }
};

// 53271 - Masters Call
class spell_hun_masters_call : public SpellScript
{
    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellEffect({ { spellInfo->Id, EFFECT_0 } })
            && ValidateSpellInfo({ SPELL_HUNTER_MASTERS_CALL_TRIGGERED, uint32(spellInfo->GetEffect(EFFECT_0).CalcValue()) });
    }

    bool Load() override
    {
        return GetCaster()->GetTypeId() == TYPEID_PLAYER;
    }

    SpellCastResult DoCheckCast()
    {
        Guardian* pet = GetCaster()->ToPlayer()->GetGuardianPet();
        ASSERT(pet); // checked in Spell::CheckCast

        if (!pet->IsPet() || !pet->IsAlive())
            return SPELL_FAILED_NO_PET;

        // Do a mini Spell::CheckCasterAuras on the pet, no other way of doing this
        SpellCastResult result = SPELL_CAST_OK;
        uint32 const unitflag = pet->m_unitData->Flags;
        if (!pet->GetCharmerGUID().IsEmpty())
            result = SPELL_FAILED_CHARMED;
        else if (unitflag & UNIT_FLAG_STUNNED)
            result = SPELL_FAILED_STUNNED;
        else if (unitflag & UNIT_FLAG_FLEEING)
            result = SPELL_FAILED_FLEEING;
        else if (unitflag & UNIT_FLAG_CONFUSED)
            result = SPELL_FAILED_CONFUSED;

        if (result != SPELL_CAST_OK)
            return result;

        Unit* target = GetExplTargetUnit();
        if (!target)
            return SPELL_FAILED_BAD_TARGETS;

        if (!pet->IsWithinLOSInMap(target))
            return SPELL_FAILED_LINE_OF_SIGHT;

        return SPELL_CAST_OK;
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        GetCaster()->ToPlayer()->GetPet()->CastSpell(GetHitUnit(), GetEffectValue(), true);
    }

    void HandleScriptEffect(SpellEffIndex /*effIndex*/)
    {
        GetHitUnit()->CastSpell(nullptr, SPELL_HUNTER_MASTERS_CALL_TRIGGERED, true);
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_hun_masters_call::DoCheckCast);

        OnEffectHitTarget += SpellEffectFn(spell_hun_masters_call::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        OnEffectHitTarget += SpellEffectFn(spell_hun_masters_call::HandleScriptEffect, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

// 34477 - Misdirection
class spell_hun_misdirection : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_HUNTER_MISDIRECTION_PROC });
    }

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_DEFAULT || GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_INTERRUPT)
            return;

        if (!GetTarget()->HasAura(SPELL_HUNTER_MISDIRECTION_PROC))
            GetTarget()->GetThreatManager().UnregisterRedirectThreat(SPELL_HUNTER_MISDIRECTION);
    }

    void HandleProc(AuraEffect* aurEff, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();
        GetTarget()->CastSpell(GetTarget(), SPELL_HUNTER_MISDIRECTION_PROC, aurEff);
    }

    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(spell_hun_misdirection::OnRemove, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        OnEffectProc += AuraEffectProcFn(spell_hun_misdirection::HandleProc, EFFECT_1, SPELL_AURA_DUMMY);
    }
};

// 35079 - Misdirection (Proc)
class spell_hun_misdirection_proc : public AuraScript
{
    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->GetThreatManager().UnregisterRedirectThreat(SPELL_HUNTER_MISDIRECTION);
    }

    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(spell_hun_misdirection_proc::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// 2643 - Multi-Shot
class spell_hun_multi_shot : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_HUNTER_MULTI_SHOT_FOCUS });
    }

    bool Load() override
    {
        return GetCaster()->GetTypeId() == TYPEID_PLAYER;
    }

    void HandleOnHit()
    {
        // We need to check hunter's spec because it doesn't generate focus on other specs than MM
        if (GetCaster()->ToPlayer()->GetPrimarySpecialization() == ChrSpecialization::HunterMarksmanship)
            GetCaster()->CastSpell(GetCaster(), SPELL_HUNTER_MULTI_SHOT_FOCUS, true);
    }

    void Register() override
    {
        OnHit += SpellHitFn(spell_hun_multi_shot::HandleOnHit);
    }
};

// 55709 - Pet Heart of the Phoenix
class spell_hun_pet_heart_of_the_phoenix : public SpellScript
{
    bool Load() override
    {
        if (!GetCaster()->IsPet())
            return false;
        return true;
    }

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_HUNTER_PET_HEART_OF_THE_PHOENIX_TRIGGERED, SPELL_HUNTER_PET_HEART_OF_THE_PHOENIX_DEBUFF });
    }

    void HandleScript(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (Unit* owner = caster->GetOwner())
        {
            if (!caster->HasAura(SPELL_HUNTER_PET_HEART_OF_THE_PHOENIX_DEBUFF))
            {
                CastSpellExtraArgs args(TRIGGERED_FULL_MASK);
                args.AddSpellBP0(100);
                owner->CastSpell(caster, SPELL_HUNTER_PET_HEART_OF_THE_PHOENIX_TRIGGERED, args);
                caster->CastSpell(caster, SPELL_HUNTER_PET_HEART_OF_THE_PHOENIX_DEBUFF, true);
            }
        }
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_hun_pet_heart_of_the_phoenix::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

// 781 - Disengage
class spell_hun_posthaste : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_HUNTER_POSTHASTE_TALENT, SPELL_HUNTER_POSTHASTE_INCREASE_SPEED });
    }

    void HandleAfterCast()
    {
        if (GetCaster()->HasAura(SPELL_HUNTER_POSTHASTE_TALENT))
        {
            GetCaster()->RemoveMovementImpairingAuras(true);
            GetCaster()->CastSpell(GetCaster(), SPELL_HUNTER_POSTHASTE_INCREASE_SPEED, GetSpell());
        }
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_hun_posthaste::HandleAfterCast);
    }
};

// 257044 - Rapid Fire
class spell_hun_rapid_fire : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_HUNTER_RAPID_FIRE_DAMAGE });
    }

    void HandlePeriodic(AuraEffect const* /*aurEff*/)
    {
        if (Unit* caster = GetCaster())
            caster->CastSpell(GetTarget(), SPELL_HUNTER_RAPID_FIRE_DAMAGE, true);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_hun_rapid_fire::HandlePeriodic, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
    }
};

// 257045 - Rapid Fire Damage
class spell_hun_rapid_fire_damage : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_HUNTER_RAPID_FIRE_ENERGIZE });
    }

    void HandleHit(SpellEffIndex /*effIndex*/)
    {
        GetCaster()->CastSpell(nullptr, SPELL_HUNTER_RAPID_FIRE_ENERGIZE, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_hun_rapid_fire_damage::HandleHit, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

// 53480 - Roar of Sacrifice
class spell_hun_roar_of_sacrifice : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_ROAR_OF_SACRIFICE_TRIGGERED });
    }

    bool CheckProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        DamageInfo* damageInfo = eventInfo.GetDamageInfo();
        if (!damageInfo || !(damageInfo->GetSchoolMask() & aurEff->GetMiscValue()))
            return false;

        if (!GetCaster())
            return false;

        return true;
    }

    void HandleProc(AuraEffect* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        CastSpellExtraArgs args(aurEff);
        args.AddSpellBP0(CalculatePct(eventInfo.GetDamageInfo()->GetDamage(), aurEff->GetAmount()));
        eventInfo.GetActor()->CastSpell(GetCaster(), SPELL_ROAR_OF_SACRIFICE_TRIGGERED, args);
    }

    void Register() override
    {
        DoCheckEffectProc += AuraCheckEffectProcFn(spell_hun_roar_of_sacrifice::CheckProc, EFFECT_1, SPELL_AURA_DUMMY);
        OnEffectProc += AuraEffectProcFn(spell_hun_roar_of_sacrifice::HandleProc, EFFECT_1, SPELL_AURA_DUMMY);
    }
};

// 37506 - Scatter Shot
class spell_hun_scatter_shot : public SpellScript
{
    bool Load() override
    {
        return GetCaster()->GetTypeId() == TYPEID_PLAYER;
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Player* caster = GetCaster()->ToPlayer();
        // break Auto Shot and autohit
        caster->InterruptSpell(CURRENT_AUTOREPEAT_SPELL);
        caster->AttackStop();
        caster->SendAttackSwingCancelAttack();
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_hun_scatter_shot::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 56641 - Steady Shot
class spell_hun_steady_shot : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_HUNTER_STEADY_SHOT_FOCUS });
    }

    bool Load() override
    {
        return GetCaster()->GetTypeId() == TYPEID_PLAYER;
    }

    void HandleOnHit()
    {
        GetCaster()->CastSpell(GetCaster(), SPELL_HUNTER_STEADY_SHOT_FOCUS, true);
    }

    void Register() override
    {
        OnHit += SpellHitFn(spell_hun_steady_shot::HandleOnHit);
    }
};

// 1515 - Tame Beast
class spell_hun_tame_beast : public SpellScript
{
    static constexpr uint32 CallPetSpellIds[MAX_ACTIVE_PETS] =
    {
        883,
        83242,
        83243,
        83244,
        83245,
    };

    SpellCastResult CheckCast()
    {
        Player* caster = GetCaster()->ToPlayer();
        if (!caster)
            return SPELL_FAILED_DONT_REPORT;

        if (!GetExplTargetUnit())
            return SPELL_FAILED_BAD_IMPLICIT_TARGETS;

        if (Creature* target = GetExplTargetUnit()->ToCreature())
        {
            if (target->GetLevelForTarget(caster) > caster->GetLevel())
                return SPELL_FAILED_HIGHLEVEL;

            // use SMSG_PET_TAME_FAILURE?
            if (!target->GetCreatureTemplate()->IsTameable(caster->CanTameExoticPets(), target->GetCreatureDifficulty()))
                return SPELL_FAILED_BAD_TARGETS;

            if (PetStable const* petStable = caster->GetPetStable())
            {
                if (petStable->CurrentPetIndex)
                    return SPELL_FAILED_ALREADY_HAVE_SUMMON;

                auto freeSlotItr = std::find_if(petStable->ActivePets.begin(), petStable->ActivePets.end(), [](Optional<PetStable::PetInfo> const& petInfo)
                {
                    return !petInfo.has_value();
                });

                if (freeSlotItr == petStable->ActivePets.end())
                {
                    caster->SendTameFailure(PetTameResult::TooMany);
                    return SPELL_FAILED_DONT_REPORT;
                }

                // Check for known Call Pet X spells
                std::size_t freeSlotIndex = std::distance(petStable->ActivePets.begin(), freeSlotItr);
                if (!caster->HasSpell(CallPetSpellIds[freeSlotIndex]))
                {
                    caster->SendTameFailure(PetTameResult::TooMany);
                    return SPELL_FAILED_DONT_REPORT;
                }
            }

            if (!caster->GetCharmedGUID().IsEmpty())
                return SPELL_FAILED_ALREADY_HAVE_CHARM;

            if (!target->GetOwnerGUID().IsEmpty())
            {
                caster->SendTameFailure(PetTameResult::CreatureAlreadyOwned);
                return SPELL_FAILED_DONT_REPORT;
            }
        }
        else
            return SPELL_FAILED_BAD_IMPLICIT_TARGETS;

        return SPELL_CAST_OK;
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_hun_tame_beast::CheckCast);
    }
};

// 67151 - Item - Hunter T9 4P Bonus (Steady Shot)
class spell_hun_t9_4p_bonus : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_HUNTER_T9_4P_GREATNESS });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        if (eventInfo.GetActor()->GetTypeId() == TYPEID_PLAYER && eventInfo.GetActor()->ToPlayer()->GetPet())
            return true;
        return false;
    }

    void HandleProc(AuraEffect* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        Unit* caster = eventInfo.GetActor();

        caster->CastSpell(caster->ToPlayer()->GetPet(), SPELL_HUNTER_T9_4P_GREATNESS, aurEff);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_hun_t9_4p_bonus::CheckProc);
        OnEffectProc += AuraEffectProcFn(spell_hun_t9_4p_bonus::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

// 394366 - Find The Mark
class spell_hun_t29_2p_marksmanship_bonus : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellEffect({ { SPELL_HUNTER_T29_2P_MARKSMANSHIP_DAMAGE, EFFECT_0 } })
            && sSpellMgr->AssertSpellInfo(SPELL_HUNTER_T29_2P_MARKSMANSHIP_DAMAGE, DIFFICULTY_NONE)->GetMaxTicks();
    }

    void HandleProc(AuraEffect* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Unit* caster = eventInfo.GetActor();
        uint32 ticks = sSpellMgr->AssertSpellInfo(SPELL_HUNTER_T29_2P_MARKSMANSHIP_DAMAGE, DIFFICULTY_NONE)->GetMaxTicks();
        uint32 damage = CalculatePct(eventInfo.GetDamageInfo()->GetOriginalDamage(), aurEff->GetAmount()) / ticks;

        caster->CastSpell(eventInfo.GetActionTarget(), SPELL_HUNTER_T29_2P_MARKSMANSHIP_DAMAGE, CastSpellExtraArgs(aurEff)
            .SetTriggeringSpell(eventInfo.GetProcSpell())
            .AddSpellMod(SPELLVALUE_BASE_POINT0, damage));
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_hun_t29_2p_marksmanship_bonus::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// Tar Trap (not activated) - 187698
// AreaTriggerID - 4435
class at_hun_tar_trap_not_activated : public AreaTriggerEntityScript
{
public:

    at_hun_tar_trap_not_activated() : AreaTriggerEntityScript("at_hun_tar_trap_not_activated") { }

    struct at_hun_tar_trap_not_activatedAI : AreaTriggerAI
    {
        int32 timeInterval;

        enum UsedSpells
        {
            SPELL_HUNTER_ACTIVATE_TAR_TRAP = 187700
        };

        at_hun_tar_trap_not_activatedAI(AreaTrigger* areatrigger) : AreaTriggerAI(areatrigger)
        {
            timeInterval = 200;
        }

        void OnCreate(Spell const* /*creatingSpell*/) override
        {
            Unit* caster = at->GetCaster();

            if (!caster)
                return;

            if (!caster->ToPlayer())
                return;

            for (auto itr : at->GetInsideUnits())
            {
                Unit* target = ObjectAccessor::GetUnit(*caster, itr);
                if (!caster->IsFriendlyTo(target))
                    if (TempSummon* tempSumm = caster->SummonCreature(WORLD_TRIGGER, at->GetPosition(), TEMPSUMMON_TIMED_DESPAWN, 1min))
                    {
                        tempSumm->SetFaction(caster->GetFaction());
                        tempSumm->SetSummonerGUID(caster->GetGUID());
                        PhasingHandler::InheritPhaseShift(tempSumm, caster);
                        caster->CastSpell(tempSumm, SPELL_HUNTER_ACTIVATE_TAR_TRAP, true);
                        at->Remove();
                    }
            }
        }

        void OnUnitEnter(Unit* unit) override
        {
            Unit* caster = at->GetCaster();

            if (!caster || !unit)
                return;

            if (!caster->ToPlayer())
                return;

            if (!caster->IsFriendlyTo(unit))
            {
                if (TempSummon* tempSumm = caster->SummonCreature(WORLD_TRIGGER, at->GetPosition(), TEMPSUMMON_TIMED_DESPAWN, 1min))
                {
                    tempSumm->SetFaction(caster->GetFaction());
                    tempSumm->SetSummonerGUID(caster->GetGUID());
                    PhasingHandler::InheritPhaseShift(tempSumm, caster);
                    caster->CastSpell(tempSumm, SPELL_HUNTER_ACTIVATE_TAR_TRAP, true);
                    at->Remove();
                }
            }
        }
    };

    AreaTriggerAI* GetAI(AreaTrigger* areatrigger) const override
    {
        return new at_hun_tar_trap_not_activatedAI(areatrigger);
    }
};

// Tar Trap (activated) - 187700
// AreaTriggerID - 4436
class at_hun_tar_trap_activated : public AreaTriggerEntityScript
{
public:
    at_hun_tar_trap_activated() : AreaTriggerEntityScript("at_hun_tar_trap_activated") { }

    struct at_hun_tar_trap_activatedAI : AreaTriggerAI
    {
        int32 timeInterval;

        enum UsedSpells
        {
            SPELL_HUNTER_TAR_TRAP_SLOW = 135299
        };

        at_hun_tar_trap_activatedAI(AreaTrigger* areatrigger) : AreaTriggerAI(areatrigger)
        {
            timeInterval = 200;
        }

        void OnCreate(Spell const* /*creatingSpell*/) override
        {
            Unit* caster = at->GetCaster();

            if (!caster)
                return;

            if (!caster->ToPlayer())
                return;

            for (auto itr : at->GetInsideUnits())
            {
                Unit* target = ObjectAccessor::GetUnit(*caster, itr);
                if (!caster->IsFriendlyTo(target))
                {
                    caster->CastSpell(target, SPELL_HUNTER_TAR_TRAP_SLOW, true);
                }
            }
        }

        void OnUnitEnter(Unit* unit) override
        {
            Unit* caster = at->GetCaster();

            if (!caster || !unit)
                return;

            if (!caster->ToPlayer())
                return;

            if (!caster->IsFriendlyTo(unit))
            {
                caster->CastSpell(unit, SPELL_HUNTER_TAR_TRAP_SLOW, true);
            }
        }

        void OnUnitExit(Unit* unit) override
        {
            Unit* caster = at->GetCaster();

            if (!caster || !unit)
                return;

            if (!caster->ToPlayer())
                return;

            if (unit->HasAura(SPELL_HUNTER_TAR_TRAP_SLOW) && unit->GetAura(SPELL_HUNTER_TAR_TRAP_SLOW)->GetCaster() == caster)
                unit->RemoveAura(SPELL_HUNTER_TAR_TRAP_SLOW);
        }

        void OnRemove() override
        {
            Unit* caster = at->GetCaster();

            if (!caster)
                return;

            if (!caster->ToPlayer())
                return;

            for (auto itr : at->GetInsideUnits())
            {
                Unit* target = ObjectAccessor::GetUnit(*caster, itr);
                if (target->HasAura(SPELL_HUNTER_TAR_TRAP_SLOW) && target->GetAura(SPELL_HUNTER_TAR_TRAP_SLOW)->GetCaster() == caster)
                    target->RemoveAura(SPELL_HUNTER_TAR_TRAP_SLOW);
            }
        }
    };

    AreaTriggerAI* GetAI(AreaTrigger* areatrigger) const override
    {
        return new at_hun_tar_trap_activatedAI(areatrigger);
    }
};

// Freezing Trap - 187650
// AreaTriggerID - 4424
class at_hun_freezing_trap : public AreaTriggerEntityScript
{
public:

    at_hun_freezing_trap() : AreaTriggerEntityScript("at_hun_freezing_trap") { }

    struct at_hun_freezing_trapAI : AreaTriggerAI
    {
        int32 timeInterval;

        enum UsedSpells
        {
            SPELL_HUNTER_FREEZING_TRAP_STUN = 3355
        };

        at_hun_freezing_trapAI(AreaTrigger* areatrigger) : AreaTriggerAI(areatrigger)
        {
            timeInterval = 200;
        }

        void OnCreate(Spell const* /*creatingSpell*/) override
        {
            Unit* caster = at->GetCaster();

            if (!caster)
                return;

            if (!caster->ToPlayer())
                return;

            for (auto itr : at->GetInsideUnits())
            {
                Unit* target = ObjectAccessor::GetUnit(*caster, itr);
                if (!caster->IsFriendlyTo(target))
                {
                    caster->CastSpell(target, SPELL_HUNTER_FREEZING_TRAP_STUN, true);
                    at->Remove();
                    return;
                }
            }
        }

        void OnUnitEnter(Unit* unit) override
        {
            Unit* caster = at->GetCaster();

            if (!caster || !unit)
                return;

            if (!caster->ToPlayer())
                return;

            if (!caster->IsFriendlyTo(unit))
            {
                caster->CastSpell(unit, SPELL_HUNTER_FREEZING_TRAP_STUN, true);
                at->Remove();
                return;
            }
        }
    };

    AreaTriggerAI* GetAI(AreaTrigger* areatrigger) const override
    {
        return new at_hun_freezing_trapAI(areatrigger);
    }
};

// Flare - 1543
// AreaTriggerID - 510
class at_hun_flare : public AreaTriggerEntityScript
{
public:
    at_hun_flare() : AreaTriggerEntityScript("at_hun_flare") {}

    struct at_hun_flareAI : AreaTriggerAI
    {
        at_hun_flareAI(AreaTrigger* areatrigger) : AreaTriggerAI(areatrigger) { }

        void OnCreate(Spell const* /*creatingSpell*/) override
        {
            Unit* caster = at->GetCaster();
            if (!caster)
                return;

            if (caster->GetTypeId() != TYPEID_PLAYER)
                return;

            if (TempSummon* tempSumm = caster->SummonCreature(WORLD_TRIGGER, at->GetPosition(), TEMPSUMMON_TIMED_DESPAWN, 200ms))
            {
                tempSumm->SetFaction(caster->GetFaction());
                tempSumm->SetSummonerGUID(caster->GetGUID());
                PhasingHandler::InheritPhaseShift(tempSumm, caster);
                caster->CastSpell(tempSumm, SPELL_HUNTER_FLARE_EFFECT, true);
            }
        }
    };

    AreaTriggerAI* GetAI(AreaTrigger* areatrigger) const override
    {
        return new at_hun_flareAI(areatrigger);
    }
};

// Kill Command - 34026
class spell_hun_kill_command : public SpellScriptLoader
{
public:
    spell_hun_kill_command() : SpellScriptLoader("spell_hun_kill_command") { }

    class spell_hun_kill_command_SpellScript : public SpellScript
    {
        enum sspell {
            AnimalInstinctsReduction = 232646,
            AspectoftheBeast = 191384,
            BestialFerocity = 191413,
            BestialTenacity = 191414,
            BestialCunning = 191397,
            SpikedCollar = 53184,
            GreatStamina = 61688,
            Cornered = 53497
        };
        bool Validate(SpellInfo const* /*SpellEntry*/) override
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_HUNTER_KILL_COMMAND, DIFFICULTY_NONE))
                return false;
            return true;
        }

        SpellCastResult CheckCastMeet()
        {
            Unit* pet = GetCaster()->GetGuardianPet();
            Unit* petTarget = GetExplTargetUnit();

            if (!pet || pet->isDead())
                return SPELL_FAILED_NO_PET;

            // pet has a target and target is within 5 yards and target is in line of sight
            if (!petTarget || !pet->IsWithinDist(petTarget, 40.0f, true) || !petTarget->IsWithinLOSInMap(pet))
                return SPELL_FAILED_DONT_REPORT;

            if (pet->HasAuraType(SPELL_AURA_MOD_STUN) || pet->HasAuraType(SPELL_AURA_MOD_CONFUSE) || pet->HasAuraType(SPELL_AURA_MOD_SILENCE) ||
                pet->HasAuraType(SPELL_AURA_MOD_FEAR) || pet->HasAuraType(SPELL_AURA_MOD_FEAR_2))
                return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;

            return SPELL_CAST_OK;
        }

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            if (GetCaster()->IsPlayer())
            {
                if (Unit* pet = GetCaster()->GetGuardianPet())
                {
                    if (!pet)
                        return;

                    if (!GetExplTargetUnit())
                        return;
                    Unit* target = GetExplTargetUnit();
                    Player* player = GetCaster()->ToPlayer();

                    pet->CastSpell(GetExplTargetUnit(), SPELL_HUNTER_KILL_COMMAND_TRIGGER, true);

                    if (pet->GetVictim())
                    {
                        pet->AttackStop();
                        pet->ToCreature()->AI()->AttackStart(GetExplTargetUnit());
                    }
                    else
                        pet->ToCreature()->AI()->AttackStart(GetExplTargetUnit());
                    //pet->CastSpell(GetExplTargetUnit(), SPELL_HUNTER_KILL_COMMAND_CHARGE, true);

                   //191384 Aspect of the Beast
                    if (GetCaster()->HasAura(AspectoftheBeast))
                    {
                        if (pet->HasAura(SpikedCollar))
                            player->CastSpell(target, BestialFerocity, true);
                        if (pet->HasAura(GreatStamina))
                            pet->CastSpell(pet, BestialTenacity, true);
                        if (pet->HasAura(Cornered))
                            player->CastSpell(target, BestialCunning, true);
                    }
                }
            }
        }

        void Register() override
        {
            OnCheckCast += SpellCheckCastFn(spell_hun_kill_command_SpellScript::CheckCastMeet);
            OnEffectHit += SpellEffectFn(spell_hun_kill_command_SpellScript::HandleDummy, EFFECT_1, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_hun_kill_command_SpellScript();
    }
};

// Kill Command (Damage) - 83381
class spell_hun_kill_command_proc : public SpellScriptLoader
{
public:
    spell_hun_kill_command_proc() : SpellScriptLoader("spell_hun_kill_command_proc") { }

    class spell_hun_kill_command_proc_SpellScript : public SpellScript
    {
        void HandleDamage(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            Unit* owner = caster->GetOwner();
            Unit* target = GetExplTargetUnit();

            // (1.5 * (rap * 3) * bmMastery * lowNerf * (1 + versability))
            int32 dmg = 4.5f * owner->m_unitData->RangedAttackPower;
            int32 lowNerf = std::min(int32(owner->GetLevel()), 20) * 0.05f;

            if (Player const* ownerPlayer = owner->ToPlayer())
                dmg = AddPct(dmg, ownerPlayer->m_activePlayerData->Mastery);

            dmg *= lowNerf;

            dmg = caster->SpellDamageBonusDone(target, GetSpellInfo(), dmg, SPELL_DIRECT_DAMAGE, GetEffectInfo(EFFECT_0));
            dmg = target->SpellDamageBonusTaken(caster, GetSpellInfo(), dmg, SPELL_DIRECT_DAMAGE);

            SetHitDamage(dmg);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_hun_kill_command_proc_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_hun_kill_command_proc_SpellScript();
    }
};

// Intimidation - 19577
class spell_hun_intimidation : public SpellScript
{
    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        Unit* target = caster->ToPlayer()->GetSelectedUnit();
        if (!caster || !target)
            return;

        caster->CastSpell(target, SPELL_HUNTER_INTIMIDATION_STUN, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_hun_intimidation::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 19574 - Bestial Wrath
class spell_hun_bestial_wrath : public SpellScript
{
    void OnActivate()
    {
        if (Unit* caster = GetCaster())
        {
            if (Player* player = caster->ToPlayer())
            {
                if (Pet* pet = player->GetPet())
                    pet->AddAura(19574, pet);
            }
        }
    }

    void Register() override
    {
        OnCast += SpellCastFn(spell_hun_bestial_wrath::OnActivate);
    }
};

//217200 - Barbed Shot
class spell_hun_barbed_shot : public SpellScriptLoader
{
public:
    spell_hun_barbed_shot() : SpellScriptLoader("spell_hun_barbed_shot") { }

    class spell_hun_barbed_shot_SpellScript : public SpellScript
    {
        void HandleOnCast()
        {
            if (Unit* caster = GetCaster())
            {
                caster->CastSpell(caster, SPELL_BARBED_SHOT_PLAYERAURA, true);

                if (caster->IsPlayer())
                {
                    if (Unit* pet = caster->GetGuardianPet())
                    {
                        if (!pet)
                            return;

                        caster->CastSpell(pet, SPELL_BARBED_SHOT_PETAURA, true);
                    }
                }
            }
        }

        void Register() override
        {
            OnCast += SpellCastFn(spell_hun_barbed_shot_SpellScript::HandleOnCast);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_hun_barbed_shot_SpellScript();
    }
};

// Dire Beast - 120679
class spell_hun_dire_beast : public SpellScriptLoader
{
public:
    spell_hun_dire_beast() : SpellScriptLoader("spell_hun_dire_beast") { }

    class spell_hun_dire_beast_SpellScript : public SpellScript
    {
        void HandleOnHit()
        {
            if (Player* player = GetCaster()->ToPlayer())
            {
                if (Unit* target = GetHitUnit())
                {
                    // Summon's skin is different function of Map or Zone ID
                    switch (player->GetZoneId())
                    {
                    case 5785: // The Jade Forest
                        player->CastSpell(target, DIRE_BEAST_JADE_FOREST, true);
                        break;
                    case 5805: // Valley of the Four Winds
                        player->CastSpell(target, DIRE_BEAST_VALLEY_OF_THE_FOUR_WINDS, true);
                        break;
                    case 5840: // Vale of Eternal Blossoms
                        player->CastSpell(target, DIRE_BEAST_VALE_OF_THE_ETERNAL_BLOSSOM, true);
                        break;
                    case 5841: // Kun-Lai Summit
                        player->CastSpell(target, DIRE_BEAST_KUN_LAI_SUMMIT, true);
                        break;
                    case 5842: // Townlong Steppes
                        player->CastSpell(target, DIRE_BEAST_TOWNLONG_STEPPES, true);
                        break;
                    case 6134: // Krasarang Wilds
                        player->CastSpell(target, DIRE_BEAST_KRASARANG_WILDS, true);
                        break;
                    case 6138: // Dread Wastes
                        player->CastSpell(target, DIRE_BEAST_DREAD_WASTES, true);
                        break;
                    default:
                    {
                        switch (player->GetMapId())
                        {
                        case 0: // Eastern Kingdoms
                            player->CastSpell(target, DIRE_BEAST_EASTERN_KINGDOMS, true);
                            break;
                        case 1: // Kalimdor
                            player->CastSpell(target, DIRE_BEAST_KALIMDOR, true);
                            break;
                        case 8: // Outland
                            player->CastSpell(target, DIRE_BEAST_OUTLAND, true);
                            break;
                        case 10: // Northrend
                            player->CastSpell(target, DIRE_BEAST_NORTHREND, true);
                            break;
                        default:
                            if (player->GetMap()->IsDungeon())
                                player->CastSpell(target, DIRE_BEAST_DUNGEONS, true);
                            else // Default
                                player->CastSpell(target, DIRE_BEAST_KALIMDOR, true);
                            break;
                        }
                        break;
                    }
                    }
                }
            }
        }

        void HandleAfterCast()
        {
            if (Player* player = GetCaster()->ToPlayer())
            {
                if (player->HasAura(SPELL_HUNTER_WILD_CALL_AURA))
                    player->RemoveAurasDueToSpell(SPELL_HUNTER_WILD_CALL_AURA);
            }
        }

        void Register() override
        {
            OnHit += SpellHitFn(spell_hun_dire_beast_SpellScript::HandleOnHit);
            AfterCast += SpellCastFn(spell_hun_dire_beast_SpellScript::HandleAfterCast);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_hun_dire_beast_SpellScript();
    }
};

// Binding Shot - 109248
// AreaTriggerID - 1524
class at_hun_binding_shot : public AreaTriggerEntityScript
{
public:

    at_hun_binding_shot() : AreaTriggerEntityScript("at_hun_binding_shot") { }

    struct at_hun_binding_shotAI : AreaTriggerAI
    {
        enum UsedSpells
        {
            SPELL_HUNTER_BINDING_SHOT_AURA = 117405,
            SPELL_HUNTER_BINDING_SHOT_STUN = 117526,
            SPELL_HUNTER_BINDING_SHOT_IMMUNE = 117553,
            SPELL_HUNTER_BINDING_SHOT_VISUAL_1 = 118306,
            SPELL_HUNDER_BINDING_SHOT_VISUAL_2 = 117614
        };

        at_hun_binding_shotAI(AreaTrigger* areatrigger) : AreaTriggerAI(areatrigger) { }

        void OnUnitEnter(Unit* unit) override
        {
            Unit* caster = at->GetCaster();

            if (!caster)
                return;

            if (!unit)
                return;

            if (!caster->IsFriendlyTo(unit))
            {
                unit->CastSpell(unit, SPELL_HUNTER_BINDING_SHOT_AURA, true);
            }
        }

        void OnUnitExit(Unit* unit) override
        {
            if (!unit || !at->GetCaster())
                return;

            Position pos = at->GetPosition();

            // Need to check range also, since when the trigger is removed, this get called as well.
            if (unit->HasAura(SPELL_HUNTER_BINDING_SHOT_AURA) && unit->GetExactDist(&pos) >= 5.0f)
            {
                unit->RemoveAura(SPELL_HUNTER_BINDING_SHOT_AURA);
                at->GetCaster()->CastSpell(unit, SPELL_HUNTER_BINDING_SHOT_STUN, true);
            }
        }
    };

    AreaTriggerAI* GetAI(AreaTrigger* areatrigger) const override
    {
        return new at_hun_binding_shotAI(areatrigger);
    }
};

// 321530 - Bloodshed - Work half
class spell_hun_bloodshed : public SpellScript
{
    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        Unit* target = caster->ToPlayer()->GetSelectedUnit();
        Player* player = caster->ToPlayer();
        Pet* pet = player->GetPet();

        if (!caster || !player || !target || !pet)
            return;

        if (pet->GetVictim())
        {
            pet->AttackStop();
            pet->ToCreature()->AI()->AttackStart(GetExplTargetUnit());
            pet->CastSpell(GetExplTargetUnit(), SPELL_BLOODSHED_PROC, true);
        }
        else
            pet->ToCreature()->AI()->AttackStart(GetExplTargetUnit());
            pet->CastSpell(GetExplTargetUnit(), SPELL_BLOODSHED_PROC, true);

        
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_hun_bloodshed::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 260243 - Volley (Aura)
class spell_hun_volley_aura : public AuraScript
{
public:
    void SetVisualDummy(TempSummon* summon)
    {
        _visualDummy = summon->GetGUID();
        _dest = summon->GetPosition();
    }

private:
    void HandleEffectPeriodic(AuraEffect const* aurEff)
    {
        GetTarget()->CastSpell(_dest, SPELL_VOLLEY_DAMAGE, aurEff);
    }

    void HandleEffecRemoved(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (Creature* summon = ObjectAccessor::GetCreature(*GetTarget(), _visualDummy))
            summon->DespawnOrUnsummon();
    }

    void Register() override
    {
        OnEffectRemove += AuraEffectRemoveFn(spell_hun_volley_aura::HandleEffecRemoved, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_hun_volley_aura::HandleEffectPeriodic, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
    }

    ObjectGuid _visualDummy;
    Position _dest;
};

// 260243 - Volley
class spell_hun_volley : public SpellScript
{
    void InitializeVisualStalker()
    {
        if (Aura* aura = GetHitAura())
        {
            if (WorldLocation const* dest = GetExplTargetDest())
            {
                Milliseconds duration = Milliseconds(GetSpellInfo()->CalcDuration(GetOriginalCaster()));
                TempSummon* summon = GetCaster()->GetMap()->SummonCreature(NPC_VOLLEY, *dest, nullptr, duration, GetOriginalCaster());
                if (!summon)
                    return;

                if (spell_hun_volley_aura* script = aura->GetScript<spell_hun_volley_aura>())
                    script->SetVisualDummy(summon);
            }
        }
    }

    void Register() override
    {
        AfterHit += SpellHitFn(spell_hun_volley::InitializeVisualStalker);
    }
};

// 186387 - Bursting Shot
class spell_bursting_shot : public SpellScriptLoader
{
public:
    spell_bursting_shot() : SpellScriptLoader("spell_bursting_shot") { }

    class spell_bursting_shot_SpellScript : public SpellScript
    {
        void HandleAfterHit()
        {
            if (Unit* caster = GetCaster())
                caster->CastSpell(GetHitUnit(), SPELL_HUNTER_AURA_SHOOTING, true);
        }

        void Register() override
        {
            AfterHit += SpellHitFn(spell_bursting_shot_SpellScript::HandleAfterHit);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_bursting_shot_SpellScript();
    }
};

// Steel Trap - 162488
// AreaTriggerID - 2392
class at_hun_steel_trap : public AreaTriggerEntityScript
{
public:

    at_hun_steel_trap() : AreaTriggerEntityScript("at_hun_steel_trap") { }

    struct at_hun_steel_trapAI : AreaTriggerAI
    {
        int32 timeInterval;

        enum UsedSpells
        {
            SPELL_HUNTER_STEEL_TRAP_STUN = 162480,
            SPELL_HUNTER_STEEL_TRAP_BLOOD = 162487
        };

        at_hun_steel_trapAI(AreaTrigger* areatrigger) : AreaTriggerAI(areatrigger)
        {
            timeInterval = 200;
        }

        void OnCreate(Spell const* /*creatingSpell*/) override
        {

            Unit* caster = at->GetCaster();

            if (!caster)
                return;

            if (!caster->ToPlayer())
                return;

            for (auto itr : at->GetInsideUnits())
            {
                Unit* target = ObjectAccessor::GetUnit(*caster, itr);
                if (!caster->IsFriendlyTo(target))
                {
                    caster->CastSpell(target, SPELL_HUNTER_STEEL_TRAP_STUN, true);
                    caster->CastSpell(target, SPELL_HUNTER_STEEL_TRAP_BLOOD, true);
                    at->Remove();
                    return;
                }
            }
        }

        void OnUnitEnter(Unit* unit) override
        {
            Unit* caster = at->GetCaster();

            if (!caster || !unit)
                return;

            if (!caster->ToPlayer())
                return;

            if (caster->HasAura(212574)) // Nesingwary's Trapping Treads
                caster->CastSpell(caster, 212575, true);

            if (caster->HasAura(199543)) // Expert Trapper
                caster->CastSpell(unit, 201199, true);

            if (!caster->IsFriendlyTo(unit))
            {
                caster->CastSpell(unit, SPELL_HUNTER_STEEL_TRAP_STUN, true);
                caster->CastSpell(unit, SPELL_HUNTER_STEEL_TRAP_BLOOD, true);
                at->Remove();
                return;
            }
        }
    };

    AreaTriggerAI* GetAI(AreaTrigger* areatrigger) const override
    {
        return new at_hun_steel_trapAI(areatrigger);
    }
};

// Barrage - 120361
class spell_hun_barrage : public SpellScriptLoader
{
public:
    spell_hun_barrage() : SpellScriptLoader("spell_hun_barrage") { }

    class spell_hun_barrage_SpellScript : public SpellScript
    {
        void CheckLOS(std::list<WorldObject*>& targets)
        {
            if (targets.empty())
                return;

            Unit* caster = GetCaster();
            if (!caster)
                return;

            targets.remove_if([caster](WorldObject* objects) -> bool
            {
                if (!objects)
                    return true;

                if (!objects->IsWithinLOSInMap(caster))
                    return true;

                if (objects->ToUnit() && !caster->IsValidAttackTarget(objects->ToUnit()))
                    return true;

                return false;
            });
        }

        void HandleOnHit()
        {
            Player* player = GetCaster()->ToPlayer();
            Unit* target = GetHitUnit();

            if (!player || !target)
                return;

            float minDamage = 0.0f;
            float maxDamage = 0.0f;

            player->CalculateMinMaxDamage(RANGED_ATTACK, true, true, minDamage, maxDamage);

            int32 dmg = (minDamage + maxDamage) / 2 * 0.8f;

            if (!target->HasAura(SPELL_HUNTER_BARRAGE, player->GetGUID()))
                dmg /= 2;

            dmg = player->SpellDamageBonusDone(target, GetSpellInfo(), dmg, SPELL_DIRECT_DAMAGE, GetEffectInfo(EFFECT_0));
            dmg = target->SpellDamageBonusTaken(player, GetSpellInfo(), dmg, SPELL_DIRECT_DAMAGE);

            // Barrage now deals only 80% of normal damage against player-controlled targets.
            if (target->GetSpellModOwner())
                dmg = CalculatePct(dmg, 80);

            SetHitDamage(dmg);
        }

        void Register() override
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_hun_barrage_SpellScript::CheckLOS, EFFECT_0, TARGET_UNIT_CONE_ENEMY_24);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_hun_barrage_SpellScript::CheckLOS, EFFECT_1, TARGET_UNIT_CONE_ENEMY_24);
            OnHit += SpellHitFn(spell_hun_barrage_SpellScript::HandleOnHit);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_hun_barrage_SpellScript();
    }
};

// 269751 - Flanking Strike
class spell_hun_flanking_strike : public SpellScript
{
    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        Unit* target = caster->ToPlayer()->GetSelectedUnit();
        Player* player = caster->ToPlayer();
        Pet* pet = player->GetPet();

        if (!caster || !player || !target || !pet)
            return;

        WorldLocation pTarget = target->GetWorldLocation();

        float speedXY, speedZ;
        speedZ = 1.8f;
        speedXY = player->GetExactDist2d(&pTarget) * 10.0f / speedZ;
        player->GetMotionMaster()->MoveJump(pTarget, speedXY, speedZ, EVENT_JUMP);
        pet->GetMotionMaster()->MoveJump(pTarget, speedXY, speedZ, EVENT_JUMP);

        caster->CastSpell(target, 269752, true);
        pet->CastSpell(target, 269752, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_hun_flanking_strike::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 360966 - Spearhead
class spell_hun_spearhead : public SpellScript
{
    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        //Unit* target = caster->ToPlayer()->GetSelectedUnit();
        Player* player = caster->ToPlayer();
        Pet* pet = player->GetPet();

        if (!caster || !player || !pet)
            return;

        caster->CastSpell(GetHitUnit(), 360972, true);
        caster->CastSpell(GetHitUnit(), 389881, true);
        pet->CastSpell(GetHitUnit(), 360972, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_hun_spearhead::HandleDummy, EFFECT_5, SPELL_EFFECT_DUMMY);
    }
};

// Exposive Trap - 236775
// AreaTriggerID - 9810
class at_hun_explosive_trap : public AreaTriggerEntityScript
{
public:

    at_hun_explosive_trap() : AreaTriggerEntityScript("at_hun_explosive_trap") { }

    struct at_hun_explosive_trapAI : AreaTriggerAI
    {
        int32 timeInterval;

        enum UsedSpells
        {
            SPELL_HUNTER_EXPLOSIVE_TRAP_DAMAGE = 236777
        };

        at_hun_explosive_trapAI(AreaTrigger* areatrigger) : AreaTriggerAI(areatrigger)
        {
            timeInterval = 200;
        }

        void OnCreate(Spell const* /*creatingSpell*/) override
        {
            Unit* caster = at->GetCaster();

            if (!caster)
                return;

            if (!caster->ToPlayer())
                return;

            for (auto itr : at->GetInsideUnits())
            {
                Unit* target = ObjectAccessor::GetUnit(*caster, itr);
                if (!caster->IsFriendlyTo(target))
                {
                    if (TempSummon* tempSumm = caster->SummonCreature(WORLD_TRIGGER, at->GetPosition(), TEMPSUMMON_TIMED_DESPAWN, 200ms))
                    {
                        tempSumm->SetFaction(caster->GetFaction());
                        tempSumm->SetSummonerGUID(caster->GetGUID());
                        PhasingHandler::InheritPhaseShift(tempSumm, caster);
                        caster->CastSpell(tempSumm, SPELL_HUNTER_EXPLOSIVE_TRAP_DAMAGE, true);
                        at->Remove();
                    }
                }
            }
        }

        void OnUnitEnter(Unit* unit) override
        {
            Unit* caster = at->GetCaster();

            if (!caster || !unit)
                return;

            if (!caster->ToPlayer())
                return;

            if (!caster->IsFriendlyTo(unit))
            {
                if (TempSummon* tempSumm = caster->SummonCreature(WORLD_TRIGGER, at->GetPosition(), TEMPSUMMON_TIMED_DESPAWN, 200ms))
                {
                    tempSumm->SetFaction(caster->GetFaction());
                    tempSumm->SetSummonerGUID(caster->GetGUID());
                    PhasingHandler::InheritPhaseShift(tempSumm, caster);
                    caster->CastSpell(tempSumm, SPELL_HUNTER_EXPLOSIVE_TRAP_DAMAGE, true);
                    at->Remove();
                }
            }
        }
    };

    AreaTriggerAI* GetAI(AreaTrigger* areatrigger) const override
    {
        return new at_hun_explosive_trapAI(areatrigger);
    }
};

void AddSC_hunter_spell_scripts()
{
    RegisterSpellScript(spell_hun_a_murder_of_crows);
    RegisterSpellScript(spell_hun_aspect_cheetah);
    RegisterSpellScript(spell_hun_aspect_of_the_turtle);
    RegisterSpellScript(spell_hun_cobra_sting);
    RegisterSpellScript(spell_hun_exhilaration);
    RegisterSpellScript(spell_hun_explosive_shot);
    RegisterSpellScript(spell_hun_hunting_party);
    RegisterSpellScript(spell_hun_last_stand_pet);
    RegisterSpellScript(spell_hun_latent_poison_damage);
    RegisterSpellScript(spell_hun_latent_poison_trigger);
    RegisterSpellScript(spell_hun_latent_poison_injectors_damage);
    RegisterSpellScript(spell_hun_latent_poison_injectors_trigger);
    RegisterSpellScript(spell_hun_masters_call);
    RegisterSpellScript(spell_hun_misdirection);
    RegisterSpellScript(spell_hun_misdirection_proc);
    RegisterSpellScript(spell_hun_multi_shot);
    RegisterSpellScript(spell_hun_pet_heart_of_the_phoenix);
    RegisterSpellScript(spell_hun_posthaste);
    RegisterSpellScript(spell_hun_rapid_fire);
    RegisterSpellScript(spell_hun_rapid_fire_damage);
    RegisterSpellScript(spell_hun_roar_of_sacrifice);
    RegisterSpellScript(spell_hun_scatter_shot);
    RegisterSpellScript(spell_hun_steady_shot);
    RegisterSpellScript(spell_hun_tame_beast);
    RegisterSpellScript(spell_hun_t9_4p_bonus);
    RegisterSpellScript(spell_hun_t29_2p_marksmanship_bonus);

    //new
    new at_hun_tar_trap_activated();
    new at_hun_tar_trap_not_activated();
    new at_hun_freezing_trap();
    new at_hun_flare();
    new spell_hun_kill_command();
    new spell_hun_kill_command_proc();
    RegisterSpellScript(spell_hun_intimidation);
    RegisterSpellScript(spell_hun_bestial_wrath);
    new spell_hun_barbed_shot();
    new spell_hun_dire_beast();
    new at_hun_binding_shot();
    RegisterSpellScript(spell_hun_bloodshed); //work half
    RegisterSpellAndAuraScriptPair(spell_hun_volley, spell_hun_volley_aura);
    new spell_bursting_shot();
    new at_hun_steel_trap();
    new spell_hun_barrage();
    RegisterSpellScript(spell_hun_flanking_strike);
    RegisterSpellScript(spell_hun_spearhead);
    new at_hun_explosive_trap();
}
