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
 * Scripts for spells with SPELLFAMILY_WARLOCK and SPELLFAMILY_GENERIC spells used by warlock players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_warl_".
 */

#include "ScriptMgr.h"
#include "AreaTrigger.h"
#include "Creature.h"
#include "GameObject.h"
#include "ObjectAccessor.h"
#include "Pet.h"
#include "Player.h"
#include "Random.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "SpellScript.h"

#include "AreaTriggerAI.h"
#include "AreaTriggerTemplate.h"
#include "Battleground.h"
#include "SpellPackets.h"
#include "ObjectMgr.h"
#include "Object.h"
#include "CellImpl.h"
#include "DatabaseEnv.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "Unit.h"
#include "Spell.h"
#include "PetAI.h"
#include "ScriptedCreature.h"
#include "SpellHistory.h"
#include "PassiveAI.h"
#include "CombatAI.h"
#include "GridNotifiers.h"
#include "Item.h"
#include <G3D/Vector3.h>
#include <Hoff.h>



enum WarlockSpells
{
    SPELL_WARLOCK_CREATE_HEALTHSTONE                = 23517,
    SPELL_WARLOCK_DEMONIC_CIRCLE_ALLOW_CAST         = 62388,
    SPELL_WARLOCK_DEMONIC_CIRCLE_SUMMON             = 48018,
    SPELL_WARLOCK_DEMONIC_CIRCLE_TELEPORT           = 48020,
    SPELL_WARLOCK_DEVOUR_MAGIC_HEAL                 = 19658,
    SPELL_WARLOCK_DOOM_ENERGIZE                     = 193318,
    SPELL_WARLOCK_DRAIN_SOUL_ENERGIZE               = 205292,
    SPELL_WARLOCK_GLYPH_OF_DEMON_TRAINING           = 56249,
    SPELL_WARLOCK_GLYPH_OF_SOUL_SWAP                = 56226,
    SPELL_WARLOCK_GLYPH_OF_SUCCUBUS                 = 56250,
    SPELL_WARLOCK_IMMOLATE_PERIODIC                 = 157736,
    SPELL_WARLOCK_IMPROVED_HEALTH_FUNNEL_BUFF_R1    = 60955,
    SPELL_WARLOCK_IMPROVED_HEALTH_FUNNEL_BUFF_R2    = 60956,
    SPELL_WARLOCK_IMPROVED_HEALTH_FUNNEL_R1         = 18703,
    SPELL_WARLOCK_IMPROVED_HEALTH_FUNNEL_R2         = 18704,
    SPELL_WARLOCK_RAIN_OF_FIRE                      = 5740,
    SPELL_WARLOCK_RAIN_OF_FIRE_DAMAGE               = 42223,
    SPELL_WARLOCK_SEED_OF_CORRUPTION_DAMAGE         = 27285,
    SPELL_WARLOCK_SEED_OF_CORRUPTION_GENERIC        = 32865,
    SPELL_WARLOCK_SHADOW_BOLT_ENERGIZE              = 194192,
    SPELL_WARLOCK_SOULSHATTER_EFFECT                = 32835,
    SPELL_WARLOCK_SOUL_SWAP_CD_MARKER               = 94229,
    SPELL_WARLOCK_SOUL_SWAP_OVERRIDE                = 86211,
    SPELL_WARLOCK_SOUL_SWAP_MOD_COST                = 92794,
    SPELL_WARLOCK_SOUL_SWAP_DOT_MARKER              = 92795,
    SPELL_WARLOCK_UNSTABLE_AFFLICTION               = 30108,
    SPELL_WARLOCK_UNSTABLE_AFFLICTION_DISPEL        = 31117,
    SPELL_WARLOCK_SHADOWFLAME                       = 37378,
    SPELL_WARLOCK_FLAMESHADOW                       = 37379,
    SPELL_WARLOCK_SUMMON_SUCCUBUS                   = 712,
    SPELL_WARLOCK_SUMMON_INCUBUS                    = 365349,
    SPELL_WARLOCK_STRENGTHEN_PACT_SUCCUBUS          = 366323,
    SPELL_WARLOCK_STRENGTHEN_PACT_INCUBUS           = 366325,
    SPELL_WARLOCK_SUCCUBUS_PACT                     = 365360,
    SPELL_WARLOCK_INCUBUS_PACT                      = 365355,

    SPELL_WARLOCK_FEAR                              = 5782,
    SPELL_WARLOCK_FEAR_BUFF                         = 118699,
    SPELL_WARLOCK_FEAR_EFFECT                       = 118699,
    SPELL_WARLOCK_ABSOLUTE_CORRUPTION               = 196103,
    SPELL_WARLOCK_CORRUPTION_DAMAGE                 = 146739,
    SPELL_WARLOCK_PHANTOMATIC_SINGULARITY_DAMAGE    = 205246,
    SPELL_WARLOCK_HAUNT                             = 48181,
    SPELL_WARLOCK_DOOM                              = 603,
    SPELL_WARLOCK_EYE_LASER                         = 205231,
    SPELL_WARLOCK_SOULBURN                          = 74434,
    SPELL_WARLOCK_SOULBURN_UNENDING_BREATH          = 104242,
    SPELL_WARLOCK_DEMONIC_GATEWAY_PERIODIC_CHARGE   = 113901,
    SPELL_WARLOCK_DEMONIC_GATEWAY_SUMMON_GREEN      = 113886,
    SPELL_WARLOCK_DEMONIC_GATEWAY_SUMMON_PURPLE     = 113890,
    SPELL_WARLOCK_DEMONIC_GATEWAY_JUMP_GREEN        = 113896,
    SPELL_WARLOCK_DEMONIC_GATEWAY_JUMP_PURPLE       = 120729,
    SPELL_WARLOCK_DEMONIC_GATEWAY_VISUAL            = 113900,
    SPELL_WARLOCK_DEMONIC_GATEWAY_DEBUFF            = 113942,
    SPELL_WARLOCK_PLANESWALKER                      = 196675,
    SPELL_WARLOCK_PLANESWALKER_BUFF                 = 196674,
    SPELL_WARLOCK_PVP_4P_BONUS                      = 143395,
    SPELL_WARLOCK_HAND_OF_DOOM                      = 196283,
    SPELL_WARLOCK_HAND_OF_GULDAN_DAMAGE             = 86040,
    SPELL_WARLOCK_CALL_DREADSTALKERS_SUMMON         = 364750,
    SPELL_WARLOCK_IMPROVED_DREADSTALKERS            = 196272,
    SPELL_WARLOCK_WILD_IMP_SUMMON                   = 104317,
    SPELL_DEMONBOLT_ENERGIZE                        = 280127,
    SPELL_WARLOCK_DEMONIC_CALLING_TRIGGER           = 205146,
    SPELL_WARLOCK_DEMONBOLT                         = 157695,
    SPELL_WARLOCK_SHADOW_BOLT                       = 686,
    SPELL_WARLOCK_IMPLOSION_DAMAGE                  = 196278,
    SPELL_WARLOCK_IMPLOSION_JUMP                    = 205205,
    SPELL_WARLOCK_IMPENDING_DOOM                    = 196270,
    SPELL_WARLOCK_DOOM_DOUBLED                      = 218572,
    SPELL_WARLOCK_IMMOLATE                          = 348,
    SPELL_WARLOCK_BACKDRAFT_AURA                    = 196406,
    SPELL_WARLOCK_BACKDRAFT                         = 117828,
    SPELL_WARLOCK_ROARING_BLAZE                     = 205184,
    SPELL_WARLOCK_IMMOLATE_DOT                      = 157736,
    SPELL_WARLOCK_GLYPH_OF_CONFLAGRATE              = 56235,
    SPELL_WARLOCK_CONFLAGRATE                       = 17962,
    SPELL_WARLOCK_CONFLAGRATE_FIRE_AND_BRIMSTONE    = 108685,
    SPELL_WARLOCK_IMMOLATE_FIRE_AND_BRIMSTONE       = 108686,
    SPELL_WARLOCK_SOUL_FIRE                         = 6353,
    SPELL_WARLOCK_CHANNEL_DEMONFIRE_DAMAGE          = 196448,
    SPELL_WARLOCK_SOUL_CONDUIT_REFUND               = 215942,
    SPELL_SHADOW_EMBRACE                            = 32388,
    SPELL_SHADOW_EMBRACE_TARGET_DEBUFF              = 32390,
    SPELL_WARLOCK_INFERNAL_FURNACE                  = 211119,
    SPELL_WARLOCK_STOLEN_POWER                      = 211530,
    SPELL_WARLOCK_STOLEN_POWER_COUNTER              = 211529,
    SPELL_WARLOCK_STOLEN_POWER_BUFF                 = 211583,
    SPELL_WARLOCK_FEL_FIREBOLT                      = 104318,
    SPELL_INQUISITORS_GAZE                          = 386344,
    SPELL_WARLOCK_FIRE_AND_BRIMSTONE                = 196408,
    SPELL_WARLOCK_AGONY                             = 980,
    SPELL_WARLOCK_FIREBOLT_BONUS                    = 231795,
    SPELL_WARLOCK_DREADSTALKER_CHARGE               = 194247,
    SPELL_WARLOCK_SHARPENED_DREADFANGS_BUFF         = 215111,
    SPELL_WARLOCK_SHARPENED_DREADFANGS              = 211123,
};

enum MiscSpells
{
    SPELL_GEN_REPLENISHMENT                         = 57669,
    SPELL_PRIEST_SHADOW_WORD_DEATH                  = 32409
};

enum FreakzWarlockNPCs
{
    NPC_WARLOCK_DEMONIC_GATEWAY_PURPLE = 59271,
    NPC_WARLOCK_DEMONIC_GATEWAY_GREEN = 59262,
};

// 710 - Banish
class spell_warl_banish : public SpellScript
{
public:
    spell_warl_banish() {}

private:
    void HandleBanish(SpellMissInfo missInfo)
    {
        if (missInfo != SPELL_MISS_IMMUNE)
            return;

        if (Unit* target = GetHitUnit())
        {
            // Casting Banish on a banished target will remove applied aura
            if (Aura * banishAura = target->GetAura(GetSpellInfo()->Id, GetCaster()->GetGUID()))
                banishAura->Remove();
        }
    }

    void Register() override
    {
        BeforeHit += BeforeSpellHitFn(spell_warl_banish::HandleBanish);
    }
};

// 111400 - Burning Rush
class spell_warl_burning_rush : public SpellScript
{
    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellEffect({ { spellInfo->Id, EFFECT_1 } });
    }

    SpellCastResult CheckApplyAura()
    {
        Unit* caster = GetCaster();

        if (caster->GetHealthPct() <= float(GetEffectInfo(EFFECT_1).CalcValue(caster)))
        {
            SetCustomCastResultMessage(SPELL_CUSTOM_ERROR_YOU_DONT_HAVE_ENOUGH_HEALTH);
            return SPELL_FAILED_CUSTOM_ERROR;
        }

        return SPELL_CAST_OK;
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_warl_burning_rush::CheckApplyAura);
    }
};

// 111400 - Burning Rush
class spell_warl_burning_rush_aura : public AuraScript
{
    void PeriodicTick(AuraEffect const* aurEff)
    {
        if (GetTarget()->GetHealthPct() <= float(aurEff->GetAmount()))
        {
            PreventDefaultAction();
            Remove();
        }
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_warl_burning_rush_aura::PeriodicTick, EFFECT_1, SPELL_AURA_PERIODIC_DAMAGE_PERCENT);
    }
};

// 116858 - Chaos Bolt
class spell_warl_chaos_bolt : public SpellScript
{
    bool Load() override
    {
        return GetCaster()->GetTypeId() == TYPEID_PLAYER;
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        SetHitDamage(GetHitDamage() + CalculatePct(GetHitDamage(), GetCaster()->ToPlayer()->m_activePlayerData->SpellCritPercentage));
    }

    void CalcCritChance(Unit const* /*victim*/, float& critChance)
    {
        critChance = 100.0f;
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_warl_chaos_bolt::HandleDummy, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        OnCalcCritChance += SpellOnCalcCritChanceFn(spell_warl_chaos_bolt::CalcCritChance);
    }
};

// 77220 - Mastery: Chaotic Energies
class spell_warl_chaotic_energies : public AuraScript
{
    void HandleAbsorb(AuraEffect* /*aurEff*/, DamageInfo& dmgInfo, uint32& absorbAmount)
    {
        AuraEffect const* effect1 = GetEffect(EFFECT_1);
        if (!effect1 || !GetTargetApplication()->HasEffect(EFFECT_1))
        {
            PreventDefaultAction();
            return;
        }

        // You take ${$s2/3}% reduced damage
        float damageReductionPct = float(effect1->GetAmount()) / 3;
        // plus a random amount of up to ${$s2/3}% additional reduced damage
        damageReductionPct += frand(0.0f, damageReductionPct);

        absorbAmount = CalculatePct(dmgInfo.GetDamage(), damageReductionPct);
    }

    void Register() override
    {
        OnEffectAbsorb += AuraEffectAbsorbFn(spell_warl_chaotic_energies::HandleAbsorb, EFFECT_2);
    }
};

// 6201 - Create Healthstone
class spell_warl_create_healthstone : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARLOCK_CREATE_HEALTHSTONE });
    }

    bool Load() override
    {
        return GetCaster()->GetTypeId() == TYPEID_PLAYER;
    }

    void HandleScriptEffect(SpellEffIndex /*effIndex*/)
    {
        GetCaster()->CastSpell(GetCaster(), SPELL_WARLOCK_CREATE_HEALTHSTONE, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_warl_create_healthstone::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

// 108416 - Dark Pact
class spell_warl_dark_pact : public AuraScript
{
    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellEffect({ { spellInfo->Id, EFFECT_1 }, { spellInfo->Id, EFFECT_2 } });
    }

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& canBeRecalculated)
    {
        canBeRecalculated = false;
        if (Unit* caster = GetCaster())
        {
            float extraAmount = caster->SpellBaseDamageBonusDone(GetSpellInfo()->GetSchoolMask()) * 2.5f;
            int32 absorb = caster->CountPctFromCurHealth(GetEffectInfo(EFFECT_1).CalcValue(caster));
            caster->SetHealth(caster->GetHealth() - absorb);
            amount = CalculatePct(absorb, GetEffectInfo(EFFECT_2).CalcValue(caster)) + extraAmount;
        }
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_dark_pact::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
    }
};

// 48018 - Demonic Circle: Summon
class spell_warl_demonic_circle_summon : public AuraScript
{
    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes mode)
    {
        // If effect is removed by expire remove the summoned demonic circle too.
        if (!(mode & AURA_EFFECT_HANDLE_REAPPLY))
            GetTarget()->RemoveGameObject(GetId(), true);

        GetTarget()->RemoveAura(SPELL_WARLOCK_DEMONIC_CIRCLE_ALLOW_CAST);
    }

    void HandleDummyTick(AuraEffect const* /*aurEff*/)
    {
        if (GameObject* circle = GetTarget()->GetGameObject(GetId()))
        {
            // Here we check if player is in demonic circle teleport range, if so add
            // WARLOCK_DEMONIC_CIRCLE_ALLOW_CAST; allowing him to cast the WARLOCK_DEMONIC_CIRCLE_TELEPORT.
            // If not in range remove the WARLOCK_DEMONIC_CIRCLE_ALLOW_CAST.

            SpellInfo const* spellInfo = sSpellMgr->AssertSpellInfo(SPELL_WARLOCK_DEMONIC_CIRCLE_TELEPORT, GetCastDifficulty());

            if (GetTarget()->IsWithinDist(circle, spellInfo->GetMaxRange(true)))
            {
                if (!GetTarget()->HasAura(SPELL_WARLOCK_DEMONIC_CIRCLE_ALLOW_CAST))
                    GetTarget()->CastSpell(GetTarget(), SPELL_WARLOCK_DEMONIC_CIRCLE_ALLOW_CAST, true);
            }
            else
                GetTarget()->RemoveAura(SPELL_WARLOCK_DEMONIC_CIRCLE_ALLOW_CAST);
        }
    }

    void Register() override
    {
        OnEffectRemove += AuraEffectApplyFn(spell_warl_demonic_circle_summon::HandleRemove, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_warl_demonic_circle_summon::HandleDummyTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }
};

// 48020 - Demonic Circle: Teleport
class spell_warl_demonic_circle_teleport : public AuraScript
{
    void HandleTeleport(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (Player* player = GetTarget()->ToPlayer())
        {
            if (GameObject* circle = player->GetGameObject(SPELL_WARLOCK_DEMONIC_CIRCLE_SUMMON))
            {
                player->NearTeleportTo(circle->GetPositionX(), circle->GetPositionY(), circle->GetPositionZ(), circle->GetOrientation());
                player->RemoveMovementImpairingAuras(false);
            }
        }
    }

    void Register() override
    {
        OnEffectApply += AuraEffectApplyFn(spell_warl_demonic_circle_teleport::HandleTeleport, EFFECT_0, SPELL_AURA_MECHANIC_IMMUNITY, AURA_EFFECT_HANDLE_REAL);
    }
};

// 67518, 19505 - Devour Magic
class spell_warl_devour_magic : public SpellScript
{
    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellInfo({ SPELL_WARLOCK_GLYPH_OF_DEMON_TRAINING, SPELL_WARLOCK_DEVOUR_MAGIC_HEAL })
            && ValidateSpellEffect({ { spellInfo->Id, EFFECT_1 } });
    }

    void OnSuccessfulDispel(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        CastSpellExtraArgs args;
        args.TriggerFlags = TRIGGERED_FULL_MASK;
        args.AddSpellBP0(GetEffectInfo(EFFECT_1).CalcValue(caster));

        caster->CastSpell(caster, SPELL_WARLOCK_DEVOUR_MAGIC_HEAL, args);

        // Glyph of Felhunter
        if (Unit* owner = caster->GetOwner())
            if (owner->GetAura(SPELL_WARLOCK_GLYPH_OF_DEMON_TRAINING))
                owner->CastSpell(owner, SPELL_WARLOCK_DEVOUR_MAGIC_HEAL, args);
    }

    void Register() override
    {
        OnEffectSuccessfulDispel += SpellEffectFn(spell_warl_devour_magic::OnSuccessfulDispel, EFFECT_0, SPELL_EFFECT_DISPEL);
    }
};

// 603 - Doom
class spell_warl_doom : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo ({ SPELL_WARLOCK_DOOM_ENERGIZE });
    }

    void HandleEffectPeriodic(AuraEffect const* /*aurEff*/)
    {
        if (Unit* caster = GetCaster())
            caster->CastSpell(caster, SPELL_WARLOCK_DOOM_ENERGIZE, true);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_warl_doom::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
    }
};

// 198590 - Drain Soul
class spell_warl_drain_soul : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo ({ SPELL_WARLOCK_DRAIN_SOUL_ENERGIZE });
    }

    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_DEATH)
            return;

        if (Unit* caster = GetCaster())
            caster->CastSpell(caster, SPELL_WARLOCK_DRAIN_SOUL_ENERGIZE, true);
    }

    void Register() override
    {
        AfterEffectRemove += AuraEffectApplyFn(spell_warl_drain_soul::HandleRemove, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
    }
};

// 48181 - Haunt
class spell_warl_haunt : public SpellScript
{
    void HandleAfterHit()
    {
        if (Aura* aura = GetHitAura())
            if (AuraEffect* aurEff = aura->GetEffect(EFFECT_1))
                aurEff->SetAmount(CalculatePct(GetHitDamage(), aurEff->GetAmount()));
    }

    void Register() override
    {
        AfterHit += SpellHitFn(spell_warl_haunt::HandleAfterHit);
    }
};

// 755 - Health Funnel
class spell_warl_health_funnel : public AuraScript
{
    void ApplyEffect(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        Unit* target = GetTarget();
        if (caster->HasAura(SPELL_WARLOCK_IMPROVED_HEALTH_FUNNEL_R2))
            target->CastSpell(target, SPELL_WARLOCK_IMPROVED_HEALTH_FUNNEL_BUFF_R2, true);
        else if (caster->HasAura(SPELL_WARLOCK_IMPROVED_HEALTH_FUNNEL_R1))
            target->CastSpell(target, SPELL_WARLOCK_IMPROVED_HEALTH_FUNNEL_BUFF_R1, true);
    }

    void RemoveEffect(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        target->RemoveAurasDueToSpell(SPELL_WARLOCK_IMPROVED_HEALTH_FUNNEL_BUFF_R1);
        target->RemoveAurasDueToSpell(SPELL_WARLOCK_IMPROVED_HEALTH_FUNNEL_BUFF_R2);
    }

    void OnPeriodic(AuraEffect const* aurEff)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;
        //! HACK for self damage, is not blizz :/
        uint32 damage = caster->CountPctFromMaxHealth(aurEff->GetBaseAmount());

        if (Player* modOwner = caster->GetSpellModOwner())
            modOwner->ApplySpellMod(GetSpellInfo(), SpellModOp::PowerCost0, damage);

        SpellNonMeleeDamage damageInfo(caster, caster, GetSpellInfo(), GetAura()->GetSpellVisual(), GetSpellInfo()->SchoolMask, GetAura()->GetCastId());
        damageInfo.periodicLog = true;
        damageInfo.damage = damage;
        caster->DealSpellDamage(&damageInfo, false);
        caster->SendSpellNonMeleeDamageLog(&damageInfo);
    }

    void Register() override
    {
        OnEffectApply += AuraEffectApplyFn(spell_warl_health_funnel::ApplyEffect, EFFECT_0, SPELL_AURA_OBS_MOD_HEALTH, AURA_EFFECT_HANDLE_REAL);
        OnEffectRemove += AuraEffectRemoveFn(spell_warl_health_funnel::RemoveEffect, EFFECT_0, SPELL_AURA_OBS_MOD_HEALTH, AURA_EFFECT_HANDLE_REAL);
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_warl_health_funnel::OnPeriodic, EFFECT_0, SPELL_AURA_OBS_MOD_HEALTH);
    }
};

// 6262 - Healthstone
class spell_warl_healthstone_heal : public SpellScript
{
    void HandleOnHit()
    {
        int32 heal = int32(CalculatePct(GetCaster()->GetCreateHealth(), GetHitHeal()));
        SetHitHeal(heal);
    }

    void Register() override
    {
        OnHit += SpellHitFn(spell_warl_healthstone_heal::HandleOnHit);
    }
};

// 348 - Immolate
class spell_warl_immolate : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARLOCK_IMMOLATE_PERIODIC});
    }

    void HandleOnEffectHit(SpellEffIndex /*effIndex*/)
    {
        GetCaster()->CastSpell(GetHitUnit(), SPELL_WARLOCK_IMMOLATE_PERIODIC, GetSpell());
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_warl_immolate::HandleOnEffectHit, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

// 366330 - Random Sayaad
class spell_warl_random_sayaad : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo
        ({
            SPELL_WARLOCK_SUCCUBUS_PACT,
            SPELL_WARLOCK_INCUBUS_PACT
        });
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();

        caster->RemoveAurasDueToSpell(SPELL_WARLOCK_SUCCUBUS_PACT);
        caster->RemoveAurasDueToSpell(SPELL_WARLOCK_INCUBUS_PACT);

        Player* player = GetCaster()->ToPlayer();
        if (!player)
            return;

        if (Pet* pet = player->GetPet())
        {
            if (pet->IsPetSayaad())
                pet->DespawnOrUnsummon();
        }
    }

    void Register() override
    {
        OnEffectHit += SpellEffectFn(spell_warl_random_sayaad::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 366323 - Strengthen Pact - Succubus
// 366325 - Strengthen Pact - Incubus
// 366222 - Summon Sayaad
class spell_warl_sayaad_precast_disorientation : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ PET_SUMMONING_DISORIENTATION });
    }

    // Note: this is a special case in which the warlock's minion pet must also cast Summon Disorientation at the beginning since this is only handled by SPELL_EFFECT_SUMMON_PET in Spell::CheckCast.
    void OnPrecast() override
    {
        Player* player = GetCaster()->ToPlayer();
        if (!player)
            return;

        if (Pet* pet = player->GetPet())
            pet->CastSpell(pet, PET_SUMMONING_DISORIENTATION, CastSpellExtraArgs(TRIGGERED_FULL_MASK)
                .SetOriginalCaster(pet->GetGUID())
                .SetTriggeringSpell(GetSpell()));
    }

    void Register() override
    {
    }
};

// 6358 - Seduction (Special Ability)
class spell_warl_seduction : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARLOCK_GLYPH_OF_SUCCUBUS, SPELL_PRIEST_SHADOW_WORD_DEATH });
    }

    void HandleScriptEffect(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (Unit* target = GetHitUnit())
        {
            if (caster->GetOwner() && caster->GetOwner()->HasAura(SPELL_WARLOCK_GLYPH_OF_SUCCUBUS))
            {
                target->RemoveAurasByType(SPELL_AURA_PERIODIC_DAMAGE, ObjectGuid::Empty, target->GetAura(SPELL_PRIEST_SHADOW_WORD_DEATH)); // SW:D shall not be removed.
                target->RemoveAurasByType(SPELL_AURA_PERIODIC_DAMAGE_PERCENT);
                target->RemoveAurasByType(SPELL_AURA_PERIODIC_LEECH);
            }
        }
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_warl_seduction::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_APPLY_AURA);
    }
};

// 27285 - Seed of Corruption
class spell_warl_seed_of_corruption : public SpellScript
{
    void FilterTargets(std::list<WorldObject*>& targets)
    {
        if (GetExplTargetUnit())
            targets.remove(GetExplTargetUnit());
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_warl_seed_of_corruption::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
    }
};

// 27243 - Seed of Corruption
class spell_warl_seed_of_corruption_dummy : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARLOCK_SEED_OF_CORRUPTION_DAMAGE });
    }

    void CalculateBuffer(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        amount = caster->SpellBaseDamageBonusDone(GetSpellInfo()->GetSchoolMask()) * GetEffectInfo(EFFECT_0).CalcValue(caster) / 100;
    }

    void HandleProc(AuraEffect* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        DamageInfo* damageInfo = eventInfo.GetDamageInfo();
        if (!damageInfo || !damageInfo->GetDamage())
            return;

        int32 amount = aurEff->GetAmount() - damageInfo->GetDamage();
        if (amount > 0)
        {
            const_cast<AuraEffect*>(aurEff)->SetAmount(amount);
            if (!GetTarget()->HealthBelowPctDamaged(1, damageInfo->GetDamage()))
                return;
        }

        Remove();

        Unit* caster = GetCaster();
        if (!caster)
            return;

        caster->CastSpell(eventInfo.GetActionTarget(), SPELL_WARLOCK_SEED_OF_CORRUPTION_DAMAGE, aurEff);
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_seed_of_corruption_dummy::CalculateBuffer, EFFECT_2, SPELL_AURA_DUMMY);
        OnEffectProc += AuraEffectProcFn(spell_warl_seed_of_corruption_dummy::HandleProc, EFFECT_2, SPELL_AURA_DUMMY);
    }
};

// 32863 - Seed of Corruption
// 36123 - Seed of Corruption
// 38252 - Seed of Corruption
// 39367 - Seed of Corruption
// 44141 - Seed of Corruption
// 70388 - Seed of Corruption
// Monster spells, triggered only on amount drop (not on death)
class spell_warl_seed_of_corruption_generic : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARLOCK_SEED_OF_CORRUPTION_GENERIC });
    }

    void HandleProc(AuraEffect* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        DamageInfo* damageInfo = eventInfo.GetDamageInfo();
        if (!damageInfo || !damageInfo->GetDamage())
            return;

        int32 amount = aurEff->GetAmount() - damageInfo->GetDamage();
        if (amount > 0)
        {
            const_cast<AuraEffect*>(aurEff)->SetAmount(amount);
            return;
        }

        Remove();

        Unit* caster = GetCaster();
        if (!caster)
            return;

        caster->CastSpell(eventInfo.GetActionTarget(), SPELL_WARLOCK_SEED_OF_CORRUPTION_GENERIC, aurEff);
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_warl_seed_of_corruption_generic::HandleProc, EFFECT_1, SPELL_AURA_DUMMY);
    }
};

// 686 - Shadow Bolt
class spell_warl_shadow_bolt : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo ({ SPELL_WARLOCK_SHADOW_BOLT_ENERGIZE });
    }

    void HandleAfterCast()
    {
        GetCaster()->CastSpell(GetCaster(), SPELL_WARLOCK_SHADOW_BOLT_ENERGIZE, true);
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_warl_shadow_bolt::HandleAfterCast);
    }
};

// 86121 - Soul Swap
class spell_warl_soul_swap : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
        {
            SPELL_WARLOCK_GLYPH_OF_SOUL_SWAP,
            SPELL_WARLOCK_SOUL_SWAP_CD_MARKER,
            SPELL_WARLOCK_SOUL_SWAP_OVERRIDE
        });
    }

    void HandleHit(SpellEffIndex /*effIndex*/)
    {
        GetCaster()->CastSpell(GetCaster(), SPELL_WARLOCK_SOUL_SWAP_OVERRIDE, true);
        GetHitUnit()->CastSpell(GetCaster(), SPELL_WARLOCK_SOUL_SWAP_DOT_MARKER, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_warl_soul_swap::HandleHit, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

// 86211 - Soul Swap Override - Also acts as a dot container
class spell_warl_soul_swap_override : public AuraScript
{
    //! Forced to, pure virtual functions must have a body when linking
    void Register() override { }

public:
    void AddDot(uint32 id)                          { _dotList.push_back(id); }
    std::list<uint32> const GetDotList() const      { return _dotList; }
    Unit* GetOriginalSwapSource() const             { return _swapCaster; }
    void SetOriginalSwapSource(Unit* victim)        { _swapCaster = victim; }

private:
    std::list<uint32> _dotList;
    Unit* _swapCaster = nullptr;
};

//! Soul Swap Copy Spells - 92795 - Simply copies spell IDs.
class spell_warl_soul_swap_dot_marker : public SpellScript
{
    void HandleHit(SpellEffIndex /*effIndex*/)
    {
        Unit* swapVictim = GetCaster();
        Unit* warlock    = GetHitUnit();
        if (!warlock || !swapVictim)
            return;

        Unit::AuraApplicationMap const& appliedAuras = swapVictim->GetAppliedAuras();
        spell_warl_soul_swap_override* swapSpellScript = nullptr;
        if (Aura* swapOverrideAura = warlock->GetAura(SPELL_WARLOCK_SOUL_SWAP_OVERRIDE))
            swapSpellScript = swapOverrideAura->GetScript<spell_warl_soul_swap_override>();

        if (!swapSpellScript)
            return;

        flag128 classMask = GetEffectInfo().SpellClassMask;

        for (Unit::AuraApplicationMap::const_iterator itr = appliedAuras.begin(); itr != appliedAuras.end(); ++itr)
        {
            SpellInfo const* spellProto = itr->second->GetBase()->GetSpellInfo();
            if (itr->second->GetBase()->GetCaster() == warlock)
                if (spellProto->SpellFamilyName == SPELLFAMILY_WARLOCK && (spellProto->SpellFamilyFlags & classMask))
                    swapSpellScript->AddDot(itr->first);
        }

        swapSpellScript->SetOriginalSwapSource(swapVictim);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_warl_soul_swap_dot_marker::HandleHit, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 86213 - Soul Swap Exhale
class spell_warl_soul_swap_exhale : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARLOCK_SOUL_SWAP_MOD_COST, SPELL_WARLOCK_SOUL_SWAP_OVERRIDE });
    }

    SpellCastResult CheckCast()
    {
        Unit* currentTarget = GetExplTargetUnit();
        Unit* swapTarget = nullptr;
        if (Aura const* swapOverride = GetCaster()->GetAura(SPELL_WARLOCK_SOUL_SWAP_OVERRIDE))
            if (spell_warl_soul_swap_override* swapScript = swapOverride->GetScript<spell_warl_soul_swap_override>())
                swapTarget = swapScript->GetOriginalSwapSource();

        // Soul Swap Exhale can't be cast on the same target than Soul Swap
        if (swapTarget && currentTarget && swapTarget == currentTarget)
            return SPELL_FAILED_BAD_TARGETS;

        return SPELL_CAST_OK;
    }

    void OnEffectHit(SpellEffIndex /*effIndex*/)
    {
        GetCaster()->CastSpell(GetCaster(), SPELL_WARLOCK_SOUL_SWAP_MOD_COST, true);
        bool hasGlyph = GetCaster()->HasAura(SPELL_WARLOCK_GLYPH_OF_SOUL_SWAP);

        std::list<uint32> dotList;
        Unit* swapSource = nullptr;
        if (Aura const* swapOverride = GetCaster()->GetAura(SPELL_WARLOCK_SOUL_SWAP_OVERRIDE))
        {
            spell_warl_soul_swap_override* swapScript = swapOverride->GetScript<spell_warl_soul_swap_override>();
            if (!swapScript)
                return;
            dotList = swapScript->GetDotList();
            swapSource = swapScript->GetOriginalSwapSource();
        }

        if (dotList.empty())
            return;

        for (std::list<uint32>::const_iterator itr = dotList.begin(); itr != dotList.end(); ++itr)
        {
            GetCaster()->AddAura(*itr, GetHitUnit());
            if (!hasGlyph && swapSource)
                swapSource->RemoveAurasDueToSpell(*itr);
        }

        // Remove Soul Swap Exhale buff
        GetCaster()->RemoveAurasDueToSpell(SPELL_WARLOCK_SOUL_SWAP_OVERRIDE);

        if (hasGlyph) // Add a cooldown on Soul Swap if caster has the glyph
            GetCaster()->CastSpell(GetCaster(), SPELL_WARLOCK_SOUL_SWAP_CD_MARKER, false);
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_warl_soul_swap_exhale::CheckCast);
        OnEffectHitTarget += SpellEffectFn(spell_warl_soul_swap_exhale::OnEffectHit, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

// 29858 - Soulshatter
class spell_warl_soulshatter : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARLOCK_SOULSHATTER_EFFECT });
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (Unit* target = GetHitUnit())
            if (target->GetThreatManager().IsThreatenedBy(caster, true))
                caster->CastSpell(target, SPELL_WARLOCK_SOULSHATTER_EFFECT, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_warl_soulshatter::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 366323 - Strengthen Pact - Succubus
class spell_warl_strengthen_pact_succubus : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo
        ({
            SPELL_WARLOCK_SUCCUBUS_PACT,
            SPELL_WARLOCK_SUMMON_SUCCUBUS
        });
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();

        caster->CastSpell(nullptr, SPELL_WARLOCK_SUCCUBUS_PACT, TRIGGERED_FULL_MASK);
        caster->CastSpell(nullptr, SPELL_WARLOCK_SUMMON_SUCCUBUS, TRIGGERED_FULL_MASK);
    }

    void Register() override
    {
        OnEffectHit += SpellEffectFn(spell_warl_strengthen_pact_succubus::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 366325 - Strengthen Pact - Incubus
class spell_warl_strengthen_pact_incubus : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo
        ({
            SPELL_WARLOCK_INCUBUS_PACT,
            SPELL_WARLOCK_SUMMON_INCUBUS
        });
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();

        caster->CastSpell(nullptr, SPELL_WARLOCK_INCUBUS_PACT, TRIGGERED_FULL_MASK);
        caster->CastSpell(nullptr, SPELL_WARLOCK_SUMMON_INCUBUS, TRIGGERED_FULL_MASK);
    }

    void Register() override
    {
        OnEffectHit += SpellEffectFn(spell_warl_strengthen_pact_incubus::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 366222 - Summon Sayaad
class spell_warl_summon_sayaad : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo
        ({
            SPELL_WARLOCK_SUMMON_SUCCUBUS,
            SPELL_WARLOCK_SUMMON_INCUBUS
        });
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        GetCaster()->CastSpell(nullptr, roll_chance_i(50) ? SPELL_WARLOCK_SUMMON_SUCCUBUS : SPELL_WARLOCK_SUMMON_INCUBUS, TRIGGERED_FULL_MASK);
    }

    void Register() override
    {
        OnEffectHit += SpellEffectFn(spell_warl_summon_sayaad::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 37377 - Shadowflame
// 39437 - Shadowflame Hellfire and RoF
template <uint32 Trigger>
class spell_warl_t4_2p_bonus : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ Trigger });
    }

    void HandleProc(AuraEffect* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        Unit* caster = eventInfo.GetActor();
        caster->CastSpell(caster, Trigger, aurEff);
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_warl_t4_2p_bonus::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// 30108, 34438, 34439, 35183 - Unstable Affliction
class spell_warl_unstable_affliction : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARLOCK_UNSTABLE_AFFLICTION_DISPEL });
    }

    void HandleDispel(DispelInfo* dispelInfo)
    {
        if (Unit* caster = GetCaster())
        {
            if (AuraEffect const* aurEff = GetEffect(EFFECT_1))
            {
                if (Unit* target = dispelInfo->GetDispeller()->ToUnit())
                {
                    int32 bp = aurEff->GetAmount();
                    bp = target->SpellDamageBonusTaken(caster, aurEff->GetSpellInfo(), bp, DOT);
                    bp *= 9;

                    // backfire damage and silence
                    CastSpellExtraArgs args(aurEff);
                    args.AddSpellBP0(bp);
                    caster->CastSpell(target, SPELL_WARLOCK_UNSTABLE_AFFLICTION_DISPEL, args);
                }
            }
        }
    }

    void Register() override
    {
        AfterDispel += AuraDispelFn(spell_warl_unstable_affliction::HandleDispel);
    }
};

// 5740 - Rain of Fire
/// Updated 7.1.5
class spell_warl_rain_of_fire : public AuraScript
{
    void HandleDummyTick(AuraEffect const* /*aurEff*/)
    {
        std::vector<AreaTrigger*> rainOfFireAreaTriggers = GetTarget()->GetAreaTriggers(SPELL_WARLOCK_RAIN_OF_FIRE);
        GuidUnorderedSet targetsInRainOfFire;

        for (AreaTrigger* rainOfFireAreaTrigger : rainOfFireAreaTriggers)
        {
            GuidUnorderedSet const& insideTargets = rainOfFireAreaTrigger->GetInsideUnits();
            targetsInRainOfFire.insert(insideTargets.begin(), insideTargets.end());
        }

        for (ObjectGuid insideTargetGuid : targetsInRainOfFire)
            if (Unit* insideTarget = ObjectAccessor::GetUnit(*GetTarget(), insideTargetGuid))
                if (!GetTarget()->IsFriendlyTo(insideTarget))
                    GetTarget()->CastSpell(insideTarget, SPELL_WARLOCK_RAIN_OF_FIRE_DAMAGE, true);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_warl_rain_of_fire::HandleDummyTick, EFFECT_3, SPELL_AURA_PERIODIC_DUMMY);
    }
};

//5782 - Fear
class spell_warl_fear : public SpellScriptLoader
{
public:
    spell_warl_fear() : SpellScriptLoader("spell_warl_fear") {}

    class spell_warl_fear_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_warl_fear_SpellScript);

        bool  Validate(SpellInfo const* /*spellInfo*/) override
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_WARLOCK_FEAR, DIFFICULTY_NONE))
                return false;
            if (!sSpellMgr->GetSpellInfo(SPELL_WARLOCK_FEAR_BUFF, DIFFICULTY_NONE))
                return false;
            return true;
        }

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            Unit* target = GetExplTargetUnit();
            if (!target)
                return;

            caster->CastSpell(target, SPELL_WARLOCK_FEAR_BUFF, true);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_warl_fear_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_warl_fear_SpellScript();
    }
};

//204730 - Fear (effect)
class spell_warl_fear_buff : public SpellScriptLoader
{
public:
    spell_warl_fear_buff() : SpellScriptLoader("spell_warl_fear_buff") {}

    class spell_warl_fear_buff_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_warl_fear_buff_SpellScript);

        bool  Validate(SpellInfo const* /*spellInfo*/) override
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_WARLOCK_FEAR_BUFF, DIFFICULTY_NONE))
                return false;
            return true;
        }

        void HandleAfterHit()
        {
            if (Aura* aura = GetHitAura())
            {
                aura->SetMaxDuration(20 * IN_MILLISECONDS);
                aura->SetDuration(20 * IN_MILLISECONDS);
                aura->RefreshDuration();
            }
        }

        void Register() override
        {
            AfterHit += SpellHitFn(spell_warl_fear_buff_SpellScript::HandleAfterHit);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_warl_fear_buff_SpellScript();
    }
};

//146739 - Corruption effect
class spell_warl_corruption_effect : public AuraScript
{
    PrepareAuraScript(spell_warl_corruption_effect);

    bool  Validate(SpellInfo const* /*spellInfo*/) override
    {
        if (!sSpellMgr->GetSpellInfo(SPELL_WARLOCK_ABSOLUTE_CORRUPTION, DIFFICULTY_NONE))
            return false;
        return true;
    }

    void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        Unit* caster = GetCaster();
        if (!target || !caster)
            return;

        //If the target is a player, only cast for the time said in ABSOLUTE_CORRUPTION
        if (caster->HasAura(SPELL_WARLOCK_ABSOLUTE_CORRUPTION))
            GetAura()->SetDuration(target->GetTypeId() == TYPEID_PLAYER ? sSpellMgr->GetSpellInfo(SPELL_WARLOCK_ABSOLUTE_CORRUPTION, DIFFICULTY_NONE)->GetEffect(EFFECT_0).BasePoints * IN_MILLISECONDS : 60 * 60 * IN_MILLISECONDS); //If not player, 1 hour
    }

    /*
    Removes the aura if the caster is null, far away or dead.
    */
    void HandlePeriodic(AuraEffect const* /*aurEff*/)
    {
        Unit* target = GetTarget();
        Unit* caster = GetCaster();
        if (!target)
            return;

        if (!caster)
        {
            target->RemoveAura(SPELL_WARLOCK_CORRUPTION_DAMAGE);
            return;
        }

        if (caster->isDead())
            target->RemoveAura(SPELL_WARLOCK_CORRUPTION_DAMAGE);

        if (!caster->IsInRange(target, 0, 80))
            target->RemoveAura(SPELL_WARLOCK_CORRUPTION_DAMAGE);
    }

    void Register() override
    {
        OnEffectApply += AuraEffectApplyFn(spell_warl_corruption_effect::HandleApply, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_warl_corruption_effect::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
    }
};

// 234153 - Drain Life
class spell_warlock_drain_life : public AuraScript
{
    PrepareAuraScript(spell_warlock_drain_life);

    void PeriodicTick(AuraEffect const* /*aurEff*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_warlock_drain_life::PeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_LEECH);
    }
};

// 205179
class aura_warl_phantomatic_singularity : public AuraScript
{
    PrepareAuraScript(aura_warl_phantomatic_singularity);

    void OnTick(const AuraEffect* /*aurEff*/)
    {
        if (Unit* caster = GetCaster())
            caster->CastSpell(GetTarget()->GetPosition(), SPELL_WARLOCK_PHANTOMATIC_SINGULARITY_DAMAGE, true);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(aura_warl_phantomatic_singularity::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_LEECH);
    }
};

// 48181 - Haunt
class aura_warl_haunt : public AuraScript
{
    PrepareAuraScript(aura_warl_haunt);

    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* caster = GetCaster();
        if (!caster || GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_DEATH)
            return;

        caster->GetSpellHistory()->ResetCooldown(SPELL_WARLOCK_HAUNT, true);
    }

    void Register() override
    {
        OnEffectRemove += AuraEffectApplyFn(aura_warl_haunt::HandleRemove, EFFECT_1, SPELL_AURA_MOD_SCHOOL_MASK_DAMAGE_FROM_CASTER, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
    }
};

// Summon Darkglare - 205180
class spell_warlock_summon_darkglare : public SpellScript
{
    PrepareSpellScript(spell_warlock_summon_darkglare);

    void HandleOnHitTarget(SpellEffIndex /*effIndex*/)
    {
        if (Unit* target = GetHitUnit())
        {
            Player::AuraEffectList effectList = target->GetAuraEffectsByType(SPELL_AURA_PERIODIC_DAMAGE);
            /*
                        // crash here
                        for (AuraEffect* effect : effectList)
                            if (Aura* aura = effect->GetBase())
                                aura->ModDuration(8 * IN_MILLISECONDS);
            */
        }
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_warlock_summon_darkglare::HandleOnHitTarget, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

// Darkglare - 103673
class npc_pet_warlock_darkglare : public CreatureScript
{
public:
    npc_pet_warlock_darkglare() : CreatureScript("npc_pet_warlock_darkglare") {}

    struct npc_pet_warlock_darkglare_PetAI : public PetAI
    {
        npc_pet_warlock_darkglare_PetAI(Creature* creature) : PetAI(creature) {}

        void UpdateAI(uint32 /*diff*/) override
        {
            Unit* owner = me->GetOwner();
            if (!owner)
                return;

            std::list<Unit*> targets;
            owner->GetAttackableUnitListInRange(targets, 100.0f);
            targets.remove_if(Trinity::UnitAuraCheck(false, SPELL_WARLOCK_DOOM, owner->GetGUID()));
            if (!targets.empty())
                me->CastSpell(targets.front(), SPELL_WARLOCK_EYE_LASER, CastSpellExtraArgs(TRIGGERED_NONE).SetOriginalCaster(owner->GetGUID()));
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_pet_warlock_darkglare_PetAI(creature);
    }
};

// 5697 - Unending Breath
class spell_warlock_unending_breath : public SpellScriptLoader
{
public:
    spell_warlock_unending_breath() : SpellScriptLoader("spell_warlock_unending_breath") { }

    class spell_warlock_unending_breath_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_warlock_unending_breath_SpellScript);

        void HandleHit(SpellEffIndex effIndex)
        {
            PreventHitDefaultEffect(effIndex);
            Unit* caster = GetCaster();
            if (Unit* target = GetHitUnit())
                if (caster->HasAura(SPELL_WARLOCK_SOULBURN))
                    caster->CastSpell(target, SPELL_WARLOCK_SOULBURN_UNENDING_BREATH, true);
        }

        void Register() override
        {
            OnEffectLaunchTarget += SpellEffectFn(spell_warlock_unending_breath_SpellScript::HandleHit, EFFECT_0, SPELL_EFFECT_APPLY_AURA);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_warlock_unending_breath_SpellScript();
    }
};

// Demonic Gateway - 111771
class spell_warl_demonic_gateway : public SpellScriptLoader
{
public:
    spell_warl_demonic_gateway() : SpellScriptLoader("spell_warl_demonic_gateway") { }

    class spell_warl_demonic_gateway_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_warl_demonic_gateway_SpellScript);

        void HandleLaunch(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();

            // despawn all other gateways
            std::list<Creature*> targets1, targets2;
            caster->GetCreatureListWithEntryInGrid(targets1, NPC_WARLOCK_DEMONIC_GATEWAY_GREEN, 200.0f);
            caster->GetCreatureListWithEntryInGrid(targets2, NPC_WARLOCK_DEMONIC_GATEWAY_PURPLE, 200.0f);
            targets1.insert(targets1.end(), targets2.begin(), targets2.end());
            for (auto target : targets1)
            {
                if (target->GetOwnerGUID() != caster->GetGUID())
                    continue;
                target->DespawnOrUnsummon(100ms); // despawn at next tick
            }

            if (WorldLocation const* dest = GetExplTargetDest()) {
                caster->CastSpell(caster, SPELL_WARLOCK_DEMONIC_GATEWAY_SUMMON_PURPLE, true);
                caster->CastSpell(*dest, SPELL_WARLOCK_DEMONIC_GATEWAY_SUMMON_GREEN, true);
            }
        }

        SpellCastResult CheckRequirement()
        {
            // don't allow during Arena Preparation
            if (GetCaster()->HasAura(SPELL_ARENA_PREPARATION))
                return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;

            // check if player can reach the location
            Spell* spell = GetSpell();
            if (spell->m_targets.HasDst())
            {
                Position pos;
                pos = spell->m_targets.GetDst()->_position.GetPosition();
                Unit* caster = GetCaster();

                if (caster->GetPositionZ() + 6.0f < pos.GetPositionZ() ||
                    caster->GetPositionZ() - 6.0f > pos.GetPositionZ())
                    return SPELL_FAILED_NOPATH;
            }

            return SPELL_CAST_OK;
        }

        void HandleVisual(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            WorldLocation const* dest = GetExplTargetDest();
            if (!caster || !dest)
                return;

            Position pos = dest->GetPosition();

            caster->SendPlaySpellVisual(pos, 63644, 0, 0, 2.0f);
        }

        void Register() override
        {
            OnEffectLaunch += SpellEffectFn(spell_warl_demonic_gateway_SpellScript::HandleVisual, EFFECT_0, SPELL_EFFECT_SUMMON);
            OnEffectLaunch += SpellEffectFn(spell_warl_demonic_gateway_SpellScript::HandleLaunch, EFFECT_1, SPELL_EFFECT_DUMMY);
            OnCheckCast += SpellCheckCastFn(spell_warl_demonic_gateway_SpellScript::CheckRequirement);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_warl_demonic_gateway_SpellScript();
    }
};

class npc_warl_demonic_gateway : public CreatureScript
{
public:
    npc_warl_demonic_gateway() : CreatureScript("npc_warl_demonic_gateway") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_warl_demonic_gatewayAI(creature);
    }

    struct npc_warl_demonic_gatewayAI : public CreatureAI
    {
        npc_warl_demonic_gatewayAI(Creature* creature) : CreatureAI(creature) { }

        EventMap events;
        bool firstTick = true;

        void UpdateAI(uint32 /*diff*/) override
        {
            if (firstTick)
            {
                me->CastSpell(me, SPELL_WARLOCK_DEMONIC_GATEWAY_VISUAL, true);

                //todo me->SetInteractSpellId(SPELL_WARLOCK_DEMONIC_GATEWAY_ACTIVATE);
                me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE);
                me->SetNpcFlag(UNIT_NPC_FLAG_SPELLCLICK);
                me->SetReactState(ReactStates::REACT_PASSIVE);
                me->SetControlled(true, UNIT_STATE_ROOT);

                firstTick = false;
            }
        }

        void OnSpellClick(Unit* player, bool /*result*/) override
        {
            // don't allow using the gateway while having specific auras
            uint32 aurasToCheck[4] = { 121164, 121175, 121176, 121177 }; // Orbs of Power @ Temple of Kotmogu
            for (auto auraToCheck : aurasToCheck)
                if (player->HasAura(auraToCheck))
                    return;

            TeleportTarget(player, true);
            return;
        }

        void TeleportTarget(Unit* target, bool allowAnywhere)
        {
            Unit* owner = me->GetOwner();
            if (!owner)
                return;

            // only if target stepped through the portal
            if (!allowAnywhere && me->GetDistance2d(target) > 1.0f)
                return;
            // check if target wasn't recently teleported
            if (target->HasAura(SPELL_WARLOCK_DEMONIC_GATEWAY_DEBUFF))
                return;
            // only if in same party
            if (!target->IsInRaidWith(owner))
                return;
            // not allowed while CC'ed
            if (!target->CanFreeMove())
                return;

            uint32 otherGateway = me->GetEntry() == NPC_WARLOCK_DEMONIC_GATEWAY_GREEN ? NPC_WARLOCK_DEMONIC_GATEWAY_PURPLE : NPC_WARLOCK_DEMONIC_GATEWAY_GREEN;
            uint32 teleportSpell = me->GetEntry() == NPC_WARLOCK_DEMONIC_GATEWAY_GREEN ? SPELL_WARLOCK_DEMONIC_GATEWAY_JUMP_GREEN : SPELL_WARLOCK_DEMONIC_GATEWAY_JUMP_PURPLE;
            std::list<Creature*> gateways;
            me->GetCreatureListWithEntryInGrid(gateways, otherGateway, 100.0f);
            for (auto gateway : gateways)
            {
                if (gateway->GetOwnerGUID() != me->GetOwnerGUID())
                    continue;

                target->CastSpell(gateway, teleportSpell, true);
                if (target->HasAura(SPELL_WARLOCK_PLANESWALKER))
                    target->CastSpell(target, SPELL_WARLOCK_PLANESWALKER_BUFF, true);
                // Item - Warlock PvP Set 4P Bonus: "Your allies can use your Demonic Gateway again 15 sec sooner"
                if (int32 amount = owner->GetAuraEffectAmount(SPELL_WARLOCK_PVP_4P_BONUS, EFFECT_0))
                    if (Aura* aura = target->GetAura(SPELL_WARLOCK_DEMONIC_GATEWAY_DEBUFF))
                        aura->SetDuration(aura->GetDuration() - amount * IN_MILLISECONDS);
                break;
            }
        }
    };
};

// Hand of Gul'Dan - 105174
class spell_warl_hand_of_guldan : public SpellScript
{
    PrepareSpellScript(spell_warl_hand_of_guldan);

    void HandleOnHit()
    {
        if (Unit* caster = GetCaster())
        {
            if (Unit* target = GetHitUnit())
            {
                int32 nrofsummons = 1;
                nrofsummons += caster->GetPower(POWER_SOUL_SHARDS);
                if (nrofsummons > 4)
                    nrofsummons = 4;

                int8 offsetX[4]{ 0, 0, 1, 1 };
                int8 offsetY[4]{ 0, 1, 0, 1 };

                for (int i = 0; i < nrofsummons; i++)
                    caster->CastSpell(Position(target->GetPositionX() + offsetX[i], target->GetPositionY() + offsetY[i], target->GetPositionZ()), 104317, true);
                caster->CastSpell(target, SPELL_WARLOCK_HAND_OF_GULDAN_DAMAGE, true);
            }
        }
    }

    void Register() override
    {
        OnHit += SpellHitFn(spell_warl_hand_of_guldan::HandleOnHit);
    }
};

// Hand of Guldan damage - 86040
class spell_warl_hand_of_guldan_damage : public SpellScriptLoader
{
public:
    spell_warl_hand_of_guldan_damage() : SpellScriptLoader("spell_warl_hand_of_guldan_damage") { }

    class spell_warl_hand_of_guldan_damage_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_warl_hand_of_guldan_damage_SpellScript);

    public:
        spell_warl_hand_of_guldan_damage_SpellScript()
        {
            soulshards = 1;
        }

    private:

        bool Load() override
        {
            soulshards += GetCaster()->GetPower(POWER_SOUL_SHARDS);
            if (soulshards > 4)
            {
                GetCaster()->SetPower(POWER_SOUL_SHARDS, 1);
                soulshards = 4;

            }
            else
                GetCaster()->SetPower(POWER_SOUL_SHARDS, 0);
            return true;
        }

        uint32 soulshards;

        void HandleOnHit(SpellEffIndex /*effIndex*/)
        {
            if (Unit* caster = GetCaster())
            {
                if (Unit* target = GetHitUnit())
                {
                    uint32 dmg = GetHitDamage();
                    SetHitDamage(dmg * soulshards);

                    if (caster->HasAura(SPELL_WARLOCK_HAND_OF_DOOM))
                        caster->CastSpell(target, SPELL_WARLOCK_DOOM, true);
                }
            }
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_warl_hand_of_guldan_damage_SpellScript::HandleOnHit, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_warl_hand_of_guldan_damage_SpellScript();
    }
};

// Call Dreadstalkers - 104316
class spell_warlock_call_dreadstalkers : public SpellScriptLoader
{
public:
    spell_warlock_call_dreadstalkers() : SpellScriptLoader("spell_warlock_call_dreadstalkers") {}

    class spell_warlock_call_dreadstalkers_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_warlock_call_dreadstalkers_SpellScript);

        void HandleHit(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            for (int32 i = 0; i < GetEffectValue(); ++i)
                caster->CastSpell(caster, SPELL_WARLOCK_CALL_DREADSTALKERS_SUMMON, true);

            Player* player = caster->ToPlayer();
            if (!player)
                return;

            // Check if player has aura with ID 387485
            if (Aura* aura = caster->GetAura(387485))
            {
                auto effect = aura->GetEffect(0);

                if (roll_chance_i(effect->GetBaseAmount()))
                    caster->CastSpell(caster, SPELL_WARLOCK_CALL_DREADSTALKERS_SUMMON, true);
            }
        }

        void HandleAfterCast()
        {
            Unit* caster = GetCaster();
            Unit* target = GetExplTargetUnit();
            if (!caster || !target)
                return;

            std::list<Creature*> dreadstalkers;
            caster->GetCreatureListWithEntryInGrid(dreadstalkers, 98035);
            for (auto it = dreadstalkers.begin(); it != dreadstalkers.end(); ++it)
            {
                Creature* dreadstalker = *it;
                if (dreadstalker && dreadstalker->GetOwner() == caster)
                {
                    int index = std::distance(dreadstalkers.begin(), it);
                    Position TeleportPos = Hoff::GetTargetFollowPosition(dreadstalker->GetOwner(), static_cast<EFollowAngle>(7 - index), 3.f);
                    dreadstalker->NearTeleportTo(TeleportPos, false);

                    dreadstalker->SetLevel(caster->GetLevel());
                    dreadstalker->SetMaxHealth(caster->GetMaxHealth() / 3);
                    dreadstalker->SetHealth(caster->GetHealth() / 3);

                    dreadstalker->AI()->AttackStart(target);
                    caster->GetThreatManager().AddThreat(target, 9999999.f);
                }
            }

            if (uint32 impsToSummon = caster->GetAuraEffectAmount(SPELL_WARLOCK_IMPROVED_DREADSTALKERS, EFFECT_0))
                for (uint32 i = 0; i < impsToSummon; ++i)
                    caster->CastSpell(target->GetRandomNearPosition(3.f), SPELL_WARLOCK_WILD_IMP_SUMMON, true);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_warlock_call_dreadstalkers_SpellScript::HandleHit, EFFECT_0, SPELL_EFFECT_DUMMY);
            AfterCast += SpellCastFn(spell_warlock_call_dreadstalkers_SpellScript::HandleAfterCast);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_warlock_call_dreadstalkers_SpellScript();
    }
};

// Dreadstalker - 98035
class npc_warlock_dreadstalker : public CreatureScript
{
public:
    npc_warlock_dreadstalker() : CreatureScript("npc_warlock_dreadstalker") {}

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_warlock_dreadstalkerAI(creature);
    }

    struct npc_warlock_dreadstalkerAI : public ScriptedAI
    {
        npc_warlock_dreadstalkerAI(Creature* creature) : ScriptedAI(creature) {}

        bool firstTick = true;

        void UpdateAI(uint32 /*diff*/) override
        {
            if (firstTick)
            {
                Unit* owner = me->GetOwner();
                if (!me->GetOwner() || !me->GetOwner()->ToPlayer())
                    return;

                me->SetMaxHealth(owner->CountPctFromMaxHealth(40));
                me->SetHealth(me->GetMaxHealth());

                if (Unit* target = owner->ToPlayer()->GetSelectedUnit())
                    me->CastSpell(target, SPELL_WARLOCK_DREADSTALKER_CHARGE, true);

                firstTick = false;

                me->CastSpell(me, SPELL_WARLOCK_SHARPENED_DREADFANGS_BUFF, SPELLVALUE_BASE_POINT0);
                owner->GetAuraEffectAmount(SPELL_WARLOCK_SHARPENED_DREADFANGS, EFFECT_0), me, true;
            }

            UpdateVictim();
        }
    };
};

// Eye Laser - 205231
class spell_warl_eye_laser : public SpellScriptLoader
{
public:
    spell_warl_eye_laser() : SpellScriptLoader("spell_warl_eye_laser") {}

    class spell_warl_eye_laser_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_warl_eye_laser_SpellScript);

        void HandleTargets(std::list<WorldObject*>& targets)
        {
            Unit* caster = GetOriginalCaster();
            if (!caster)
                return;
            targets.clear();
            Trinity::AllWorldObjectsInRange check(caster, 100.f);
            Trinity::WorldObjectListSearcher<Trinity::AllWorldObjectsInRange> search(caster, targets, check);
            Cell::VisitAllObjects(caster, search, 100.f);
            targets.remove_if(Trinity::UnitAuraCheck(false, SPELL_WARLOCK_DOOM, caster->GetGUID()));
        }

        void Register() override
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_warl_eye_laser_SpellScript::HandleTargets, EFFECT_0, TARGET_UNIT_TARGET_ENEMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_warl_eye_laser_SpellScript();
    }
};

// 264178 - Demonbolt
class spell_warlock_demonbolt_new : public SpellScriptLoader
{
public:
    spell_warlock_demonbolt_new() : SpellScriptLoader("spell_warlock_demonbolt_new") { }

    class spell_warlock_demonbolt_new_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_warlock_demonbolt_new_SpellScript);

        void HandleHit(SpellEffIndex /*effIndex*/)
        {
            if (GetCaster())
            {
                GetCaster()->CastSpell(GetCaster(), SPELL_DEMONBOLT_ENERGIZE, true);
                GetCaster()->CastSpell(GetCaster(), SPELL_DEMONBOLT_ENERGIZE, true);
            }
        }

        void Register() override
        {
            OnEffectHit += SpellEffectFn(spell_warlock_demonbolt_new_SpellScript::HandleHit, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_warlock_demonbolt_new_SpellScript();
    }
};

// Demonic Calling - 205145
class spell_warl_demonic_calling : public SpellScriptLoader
{
public:
    spell_warl_demonic_calling() : SpellScriptLoader("spell_warl_demonic_calling") {}

    class spell_warl_demonic_calling_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_warl_demonic_calling_AuraScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_WARLOCK_DEMONIC_CALLING_TRIGGER, DIFFICULTY_NONE))
                return false;
            return true;
        }

        bool CheckProc(ProcEventInfo& eventInfo)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return false;
            if (eventInfo.GetSpellInfo() && (eventInfo.GetSpellInfo()->Id == SPELL_WARLOCK_DEMONBOLT || eventInfo.GetSpellInfo()->Id == SPELL_WARLOCK_SHADOW_BOLT) && roll_chance_i(20))
                caster->CastSpell(caster, SPELL_WARLOCK_DEMONIC_CALLING_TRIGGER, true);
            return false;
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_warl_demonic_calling_AuraScript::CheckProc);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_warl_demonic_calling_AuraScript();
    }
};

// Demonbolt - 157695
class spell_warl_demonbolt : public SpellScript
{
    PrepareSpellScript(spell_warl_demonbolt);

    int32 _summons = 0;

    void HandleHit(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        Unit* target = GetHitUnit();
        if (!caster || !target)
            return;

        int32 damage = GetHitDamage();
        AddPct(damage, _summons * 20);
        SetHitDamage(damage);
    }

    void CountSummons(std::list<WorldObject*>& targets)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        for (WorldObject* wo : targets)
        {
            if (!wo->ToCreature())
                continue;
            if (wo->ToCreature()->GetOwner() != caster)
                continue;
            if (wo->ToCreature()->GetCreatureType() != CREATURE_TYPE_DEMON)
                continue;

            _summons++;
        }

        targets.clear();
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_warl_demonbolt::HandleHit, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_warl_demonbolt::CountSummons, EFFECT_2, TARGET_UNIT_CASTER_AND_SUMMONS);
    }
};

class ImplosionDamageEvent : public BasicEvent
{
public:
    ImplosionDamageEvent(Unit* caster, Unit* target) : _caster(caster), _target(target) { }

    bool Execute(uint64 /*execTime*/, uint32 /*diff*/) override
    {
        if (_caster && _target)
        {
            _caster->CastSpell(_target, SPELL_WARLOCK_IMPLOSION_DAMAGE, true);
            _target->ToCreature()->DisappearAndDie();
        }
        return true;
    }
private:
    Unit* _caster;
    Unit* _target;
};

// 196277 - Implosion
class spell_warl_implosion : public SpellScriptLoader
{
public:
    spell_warl_implosion() : SpellScriptLoader("spell_warl_implosion") { }

    class spell_warl_implosion_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_warl_implosion_SpellScript);

        void HandleHit(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            Unit* target = GetHitUnit();
            if (!caster || !target)
                return;

            std::list<Creature*> imps;
            caster->GetCreatureListWithEntryInGrid(imps, 55659); // Wild Imps
            for (Creature* imp : imps)
            {
                if (imp->ToTempSummon()->GetSummoner() == caster)
                {
                    imp->VariableStorage.Set("ForceUpdateTimers", true);
                    imp->CastSpell(target, SPELL_WARLOCK_IMPLOSION_JUMP, true);
                    imp->GetMotionMaster()->MoveJump(*target, 300.f, 1.f, EVENT_JUMP);
                    ObjectGuid casterGuid = caster->GetGUID();
                    caster->GetScheduler().Schedule(500ms, [imp, casterGuid](TaskContext /*context*/)
                    {
                        imp->CastSpell(imp, SPELL_WARLOCK_IMPLOSION_DAMAGE, CastSpellExtraArgs(TRIGGERED_FULL_MASK).SetOriginalCaster(casterGuid));
                        imp->DisappearAndDie();
                    });
                }
            }
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_warl_implosion_SpellScript::HandleHit, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_warl_implosion_SpellScript();
    }
};

// 603 - Doom
class spell_warlock_doom : public SpellScriptLoader
{
public:
    spell_warlock_doom() : SpellScriptLoader("spell_warlock_doom") { }

    class spell_warlock_doom_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_warlock_doom_AuraScript);

        void PeriodicTick(AuraEffect const* aurEff)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            caster->CastSpell(caster, SPELL_WARLOCK_DOOM_ENERGIZE, true);
            if (caster->HasAura(SPELL_WARLOCK_IMPENDING_DOOM))
                caster->CastSpell(GetTarget(), SPELL_WARLOCK_WILD_IMP_SUMMON, true);

            if (caster->HasAura(SPELL_WARLOCK_DOOM_DOUBLED) && roll_chance_i(25))
                GetEffect(EFFECT_0)->SetAmount(aurEff->GetAmount() * 2);
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_warlock_doom_AuraScript::PeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_warlock_doom_AuraScript();
    }
};

// 17962 - Conflagrate
class spell_warl_conflagrate : public SpellScript
{
    PrepareSpellScript(spell_warl_conflagrate);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARLOCK_IMMOLATE });
    }

    void HandleHit(SpellEffIndex /*effindex*/)
    {
        Unit* caster = GetCaster();
        Unit* target = GetHitUnit();
        if (!caster || !target)
            return;

        if (caster->HasAura(SPELL_WARLOCK_BACKDRAFT_AURA))
            caster->CastSpell(caster, SPELL_WARLOCK_BACKDRAFT, true);

        caster->ModifyPower(POWER_SOUL_SHARDS, 7.5f);

        if (caster->HasAura(SPELL_WARLOCK_ROARING_BLAZE))
        {
            if (Aura* aur = target->GetAura(SPELL_WARLOCK_IMMOLATE_DOT, caster->GetGUID()))
            {
                if (AuraEffect* aurEff = aur->GetEffect(EFFECT_0))
                {
                    int32 damage = aurEff->GetAmount();
                    aurEff->SetAmount(AddPct(damage, 25));
                    aur->SetNeedClientUpdateForTargets();
                }
            }
        }
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_warl_conflagrate::HandleHit, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

class spell_warl_conflagrate_aura : public SpellScript
{
    PrepareSpellScript(spell_warl_conflagrate_aura);

    void HandleOnHit()
    {
        if (Player* _player = GetCaster()->ToPlayer())
        {
            if (Unit* target = GetHitUnit())
            {
                if (!target->HasAura(SPELL_WARLOCK_IMMOLATE) && !_player->HasAura(SPELL_WARLOCK_GLYPH_OF_CONFLAGRATE))
                    if (target->GetAura(SPELL_WARLOCK_CONFLAGRATE))
                        target->RemoveAura(SPELL_WARLOCK_CONFLAGRATE);

                if (!target->HasAura(SPELL_WARLOCK_IMMOLATE_FIRE_AND_BRIMSTONE))
                    if (target->GetAura(SPELL_WARLOCK_CONFLAGRATE_FIRE_AND_BRIMSTONE))
                        target->RemoveAura(SPELL_WARLOCK_CONFLAGRATE_FIRE_AND_BRIMSTONE);
            }
        }
    }

    void Register() override
    {
        OnHit += SpellHitFn(spell_warl_conflagrate_aura::HandleOnHit);
    }
};

// 6353 - Soul Fire
class spell_warlock_soul_fire : public SpellScriptLoader
{
public:
    spell_warlock_soul_fire() : SpellScriptLoader("spell_warlock_soul_fire") { }

    class spell_warlock_soul_fire_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_warlock_soul_fire_SpellScript);

        void HandleHit(SpellEffIndex /*effIndex*/)
        {
            if (GetCaster())
                GetCaster()->ModifyPower(POWER_SOUL_SHARDS, +40);

            //TODO: Improve it later
            GetCaster()->GetSpellHistory()->ModifyCooldown(SPELL_WARLOCK_SOUL_FIRE, Seconds(-2000));
        }

        void Register() override
        {
            OnEffectHit += SpellEffectFn(spell_warlock_soul_fire_SpellScript::HandleHit, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_warlock_soul_fire_SpellScript();
    }
};

// Channel Demonfire - 196447
class spell_warl_channel_demonfire : public SpellScriptLoader
{
public:
    spell_warl_channel_demonfire() : SpellScriptLoader("spell_warl_channel_demonfire") {}

    class spell_warl_channel_demonfire_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_warl_channel_demonfire_AuraScript);

        void HandlePeriodic(AuraEffect const* /*aurEff*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            std::list<Unit*> enemies;
            Trinity::AnyUnfriendlyUnitInObjectRangeCheck check(caster, caster, 100.f);
            Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(caster, enemies, check);
            Cell::VisitAllObjects(caster, searcher, 100.f);
            enemies.remove_if(Trinity::UnitAuraCheck(false, SPELL_WARLOCK_IMMOLATE_DOT, caster->GetGUID()));
            if (enemies.empty())
                return;

            Unit* target = Trinity::Containers::SelectRandomContainerElement(enemies);
            caster->CastSpell(target, SPELL_WARLOCK_CHANNEL_DEMONFIRE_DAMAGE, true);
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_warl_channel_demonfire_AuraScript::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_warl_channel_demonfire_AuraScript();
    }
};

// Cataclysm - 152108
class spell_warl_cataclysm : public SpellScriptLoader
{
public:
    spell_warl_cataclysm() : SpellScriptLoader("spell_warl_cataclysm") {}

    class spell_warl_cataclysm_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_warl_cataclysm_SpellScript);

        void HandleHit(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            Unit* target = GetHitUnit();
            if (!caster || !target)
                return;
            if (!caster->ToPlayer())
                return;

            caster->CastSpell(target, SPELL_WARLOCK_IMMOLATE_DOT, true);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_warl_cataclysm_SpellScript::HandleHit, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_warl_cataclysm_SpellScript();
    }
};

// Soul Conduit - 215941
class spell_warl_soul_conduit : public SpellScriptLoader
{
public:
    spell_warl_soul_conduit() : SpellScriptLoader("spell_warl_soul_conduit") {}

    class spell_warl_soul_conduit_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_warl_soul_conduit_AuraScript);

        int32 refund = 0;

        bool CheckProc(ProcEventInfo& eventInfo)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return false;
            if (eventInfo.GetActor() && eventInfo.GetActor() != caster)
                return false;

            if (Spell const* spell = eventInfo.GetProcSpell())
            {
                std::vector<SpellPowerCost> const& costs = spell->GetPowerCost();
                auto costData = std::find_if(costs.begin(), costs.end(), [](SpellPowerCost const& cost) { return cost.Power == POWER_MANA && cost.Amount > 0; });
                if (costData == costs.end())
                    return false;

                refund = costData->Amount;
                return true;
            }

            return false;
        }

        void HandleProc(AuraEffect* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            if (roll_chance_i(GetSpellInfo()->GetEffect(EFFECT_0).BasePoints))
                caster->CastSpell(caster, SPELL_WARLOCK_SOUL_CONDUIT_REFUND, CastSpellExtraArgs(TRIGGERED_FULL_MASK).AddSpellBP0(refund));
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_warl_soul_conduit_AuraScript::CheckProc);
            OnEffectProc += AuraEffectProcFn(spell_warl_soul_conduit_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_warl_soul_conduit_AuraScript();
    }
};

//232670
class spell_warr_shadowbolt_affliction : public SpellScript
{
    PrepareSpellScript(spell_warr_shadowbolt_affliction);

    void HandleOnHit()
    {
        Unit* caster = GetCaster();
        Unit* target = GetHitUnit();
        if (!caster || !target)
            return;

        if (caster->HasAura(SPELL_SHADOW_EMBRACE))
            caster->AddAura(SPELL_SHADOW_EMBRACE_TARGET_DEBUFF, target);
    }

    void Register() override
    {
        OnHit += SpellHitFn(spell_warr_shadowbolt_affliction::HandleOnHit);
    }
};

// 104318 - Fel Firebolt @ Wild Imp
class spell_warlock_fel_firebolt_wild_imp : public SpellScriptLoader
{
public:
    spell_warlock_fel_firebolt_wild_imp() : SpellScriptLoader("spell_warlock_fel_firebolt_wild_imp") { }

    class spell_warlock_fel_firebolt_wild_imp_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_warlock_fel_firebolt_wild_imp_SpellScript);

        void HandleHit(SpellEffIndex /*effIndex*/)
        {
            // "Increases damage dealt by your Wild Imps' Firebolt by 10%."
            if (Unit* owner = GetCaster()->GetOwner())
            {
                if (uint32 pct = owner->GetAuraEffectAmount(SPELL_WARLOCK_INFERNAL_FURNACE, EFFECT_0))
                    SetHitDamage(GetHitDamage() + CalculatePct(GetHitDamage(), pct));

                if (owner->HasAura(SPELL_WARLOCK_STOLEN_POWER))
                {
                    if (Aura* aur = owner->AddAura(SPELL_WARLOCK_STOLEN_POWER_COUNTER, owner))
                    {
                        if (aur->GetStackAmount() == 100)
                        {
                            owner->CastSpell(owner, SPELL_WARLOCK_STOLEN_POWER_BUFF, true);
                            aur->Remove();
                        }
                    }
                }
            }
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_warlock_fel_firebolt_wild_imp_SpellScript::HandleHit, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_warlock_fel_firebolt_wild_imp_SpellScript();
    }
};

// Wild Imp - 99739
struct npc_pet_warlock_wild_imp : public PetAI
{
    npc_pet_warlock_wild_imp(Creature* creature) : PetAI(creature)
    {
        if (Unit* owner = me->GetOwner())
        {
            me->SetLevel(owner->GetLevel());
            me->SetMaxHealth(owner->GetMaxHealth() / 3);
            me->SetHealth(owner->GetHealth() / 3);
        }
    }

    void UpdateAI(uint32 /*diff*/) override
    {
        Unit* owner = me->GetOwner();
        if (!owner)
            return;

        Unit* target = GetTarget();
        ObjectGuid newtargetGUID = owner->GetTarget();
        if (newtargetGUID.IsEmpty() || newtargetGUID == _targetGUID)
        {
            CastSpellOnTarget(owner, target);
            return;
        }

        if (Unit* newTarget = ObjectAccessor::GetUnit(*me, newtargetGUID))
            if (target != newTarget && me->IsValidAttackTarget(newTarget))
                target = newTarget;

        CastSpellOnTarget(owner, target);
    }

private:
    Unit* GetTarget() const
    {
        return ObjectAccessor::GetUnit(*me, _targetGUID);
    }

    void CastSpellOnTarget(Unit* owner, Unit* target)
    {
        if (target && me->IsValidAttackTarget(target))
        {
            _targetGUID = target->GetGUID();
            me->CastSpell(target, SPELL_WARLOCK_FEL_FIREBOLT, CastSpellExtraArgs(TRIGGERED_NONE).SetOriginalCaster(owner->GetGUID()));
        }
    }

    ObjectGuid _targetGUID;
};

// Inquisitor's Gaze - 386344
class spell_warlock_inquisitors_gaze : public SpellScript
{
    PrepareSpellScript(spell_warlock_inquisitors_gaze);

    void HandleOnHit(SpellEffIndex /*effIndex*/)
    {
        if (Unit* target = GetHitUnit())
        {
            int32 damage = (GetCaster()->SpellBaseDamageBonusDone(GetSpellInfo()->GetSchoolMask()) * 15 * 16) / 100;
            GetCaster()->CastSpell(target, SPELL_INQUISITORS_GAZE, &damage);
        }
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_warlock_inquisitors_gaze::HandleOnHit, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// Incinerate - 29722
class spell_warl_incinerate : public SpellScript
{
    PrepareSpellScript(spell_warl_incinerate);

    void HandleOnHitMainTarget(SpellEffIndex /*effIndex*/)
    {
        GetCaster()->ModifyPower(POWER_SOUL_SHARDS, 5.0f);
    }

    void HandleOnHitTarget(SpellEffIndex /*effIndex*/)
    {
        if (Unit* target = GetHitUnit())
            if (!GetCaster()->HasAura(SPELL_WARLOCK_FIRE_AND_BRIMSTONE))
                if (target != GetExplTargetUnit())
                    PreventHitDamage();
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_warl_incinerate::HandleOnHitMainTarget, EFFECT_0, SPELL_EFFECT_DUMMY);
        OnEffectHitTarget += SpellEffectFn(spell_warl_incinerate::HandleOnHitTarget, EFFECT_1, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

// 980 - Agony
class spell_warlock_agony : public SpellScriptLoader
{
public:
    spell_warlock_agony() : SpellScriptLoader("spell_warlock_agony") {}

    class spell_warlock_agony_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_warlock_agony_AuraScript);

        void HandleDummyPeriodic(AuraEffect const* auraEffect)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            float soulShardAgonyTick = caster->VariableStorage.GetValue<float>("SoulShardAgonyTick", frand(0.0f, 99.0f));
            soulShardAgonyTick += 16.0f;

            if (soulShardAgonyTick >= 100.0f)
            {
                soulShardAgonyTick = frand(0.0f, 99.0f);

                if (Player* player = GetCaster()->ToPlayer())
                    if (player->GetPower(POWER_SOUL_SHARDS) < player->GetMaxPower(POWER_SOUL_SHARDS))
                        player->SetPower(POWER_SOUL_SHARDS, player->GetPower(POWER_SOUL_SHARDS) + 10);
            }

            caster->VariableStorage.Set("SoulShardAgonyTick", soulShardAgonyTick);

            // If we have more than maxStackAmount, dont do anything
            if (GetStackAmount() >= auraEffect->GetBase()->GetMaxStackAmount())
                return;

            SetStackAmount(GetStackAmount() + 1);
        }

        void OnRemove(const AuraEffect* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            // If last agony removed, remove tick counter
            if (Unit* caster = GetCaster())
                if (!caster->GetOwnedAura(SPELL_WARLOCK_AGONY))
                    caster->VariableStorage.Remove("SoulShardAgonyTick");
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_warlock_agony_AuraScript::HandleDummyPeriodic, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
            AfterEffectRemove += AuraEffectRemoveFn(spell_warlock_agony_AuraScript::OnRemove, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_warlock_agony_AuraScript();
    }
};

// 3110 - Firebolt
class spell_warlock_imp_firebolt : public SpellScriptLoader
{
public:
    spell_warlock_imp_firebolt() : SpellScriptLoader("spell_warlock_imp_firebolt") { }

    class spell_warlock_imp_firebolt_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_warlock_imp_firebolt_SpellScript);

        void HandleHit(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            Unit* target = GetHitUnit();
            if (!caster || !caster->GetOwner() || !target)
                return;

            Unit* owner = caster->GetOwner();
            int32 damage = GetHitDamage();
            if (target->HasAura(SPELL_WARLOCK_IMMOLATE_DOT, owner->GetGUID()))
                AddPct(damage, owner->GetAuraEffectAmount(SPELL_WARLOCK_FIREBOLT_BONUS, EFFECT_0));

            SetHitDamage(damage);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_warlock_imp_firebolt_SpellScript::HandleHit, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_warlock_imp_firebolt_SpellScript();
    }
};

void AddSC_warlock_spell_scripts()
{
    RegisterSpellScript(spell_warl_banish);
    RegisterSpellAndAuraScriptPair(spell_warl_burning_rush, spell_warl_burning_rush_aura);
    RegisterSpellScript(spell_warl_chaos_bolt);
    RegisterSpellScript(spell_warl_chaotic_energies);
    RegisterSpellScript(spell_warl_create_healthstone);
    RegisterSpellScript(spell_warl_dark_pact);
    RegisterSpellScript(spell_warl_demonic_circle_summon);
    RegisterSpellScript(spell_warl_demonic_circle_teleport);
    RegisterSpellScript(spell_warl_devour_magic);
    RegisterSpellScript(spell_warl_doom);
    RegisterSpellScript(spell_warl_drain_soul);
    RegisterSpellScript(spell_warl_haunt);
    RegisterSpellScript(spell_warl_health_funnel);
    RegisterSpellScript(spell_warl_healthstone_heal);
    RegisterSpellScript(spell_warl_immolate);
    RegisterSpellScript(spell_warl_random_sayaad);
    RegisterSpellScript(spell_warl_sayaad_precast_disorientation);
    RegisterSpellScript(spell_warl_seduction);
    RegisterSpellScript(spell_warl_seed_of_corruption);
    RegisterSpellScript(spell_warl_seed_of_corruption_dummy);
    RegisterSpellScript(spell_warl_seed_of_corruption_generic);
    RegisterSpellScript(spell_warl_shadow_bolt);
    RegisterSpellScript(spell_warl_soul_swap);
    RegisterSpellScript(spell_warl_soul_swap_dot_marker);
    RegisterSpellScript(spell_warl_soul_swap_exhale);
    RegisterSpellScript(spell_warl_soul_swap_override);
    RegisterSpellScript(spell_warl_soulshatter);
    RegisterSpellScript(spell_warl_strengthen_pact_succubus);
    RegisterSpellScript(spell_warl_strengthen_pact_incubus);
    RegisterSpellScript(spell_warl_summon_sayaad);
    RegisterSpellScriptWithArgs(spell_warl_t4_2p_bonus<SPELL_WARLOCK_FLAMESHADOW>, "spell_warl_t4_2p_bonus_shadow");
    RegisterSpellScriptWithArgs(spell_warl_t4_2p_bonus<SPELL_WARLOCK_SHADOWFLAME>, "spell_warl_t4_2p_bonus_fire");
    RegisterSpellScript(spell_warl_unstable_affliction);
    RegisterSpellScript(spell_warl_rain_of_fire);

    //Fix
    new spell_warl_fear();
    new spell_warl_fear_buff();
    RegisterSpellScript(spell_warl_corruption_effect);
    RegisterSpellScript(spell_warlock_drain_life);
    RegisterSpellScript(aura_warl_phantomatic_singularity);
    RegisterSpellScript(aura_warl_haunt);
    RegisterSpellScript(spell_warlock_summon_darkglare);
    new npc_pet_warlock_darkglare();
    new spell_warlock_unending_breath();
    new spell_warl_demonic_gateway();
    new npc_warl_demonic_gateway();
    RegisterSpellScript(spell_warl_hand_of_guldan);
    new spell_warl_hand_of_guldan_damage();
    new spell_warlock_call_dreadstalkers();
    new npc_warlock_dreadstalker();
    new spell_warl_eye_laser();
    new spell_warlock_demonbolt_new();
    new spell_warl_demonic_calling();
    RegisterSpellScript(spell_warl_demonbolt);
    new spell_warl_implosion();
    new spell_warlock_doom();
    RegisterSpellScript(spell_warl_conflagrate);
    RegisterSpellScript(spell_warl_conflagrate_aura);
    new spell_warlock_soul_fire();
    new spell_warl_channel_demonfire();
    new spell_warl_cataclysm();
    new spell_warl_soul_conduit();
    RegisterSpellScript(spell_warr_shadowbolt_affliction);
    new spell_warlock_fel_firebolt_wild_imp();
    RegisterCreatureAI(npc_pet_warlock_wild_imp);
    new spell_warlock_inquisitors_gaze();
    RegisterSpellScript(spell_warl_incinerate);
    new spell_warlock_agony();
    new spell_warlock_imp_firebolt();
}
