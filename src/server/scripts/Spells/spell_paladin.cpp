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
 * Scripts for spells with SPELLFAMILY_PALADIN and SPELLFAMILY_GENERIC spells used by paladin players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_pal_".
 */

#include "ScriptMgr.h"
#include "AreaTrigger.h"
#include "AreaTriggerAI.h"
#include "CellImpl.h"
#include "CommonPredicates.h"
#include "Containers.h"
#include "DB2Stores.h"
#include "GridNotifiersImpl.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Random.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellHistory.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include <unordered_map>
#include <unordered_set>

enum PaladinSpells
{
    SPELL_PALADIN_A_JUST_REWARD_HEAL             = 469413,
    SPELL_PALADIN_ARDENT_DEFENDER_HEAL           = 66235,
    SPELL_PALADIN_ART_OF_WAR                     = 406064,
    SPELL_PALADIN_ART_OF_WAR_LEGACY              = 267344,
    SPELL_PALADIN_ART_OF_WAR_TRIGGERED           = 231843,
    SPELL_PALADIN_RIGHTEOUS_CAUSE                = 402912,
    SPELL_PALADIN_AVENGERS_SHIELD                = 31935,
    SPELL_PALADIN_AVENGING_WRATH                 = 31884,
    SPELL_PALADIN_BEACON_OF_LIGHT                = 53563,
    SPELL_PALADIN_BEACON_OF_LIGHT_HEAL           = 53652,
    SPELL_PALADIN_BLADE_OF_JUSTICE               = 184575,
    SPELL_PALADIN_BLADE_OF_VENGEANCE             = 403826,
    SPELL_PALADIN_BLESSING_OF_FREEDOM            = 1044,
    SPELL_PALADIN_BLINDING_LIGHT_EFFECT          = 105421,
    SPELL_PALADIN_CONCENTRACTION_AURA            = 19746,
    SPELL_PALADIN_CONSECRATION_DEBUFF            = 204242,
    SPELL_PALADIN_CONSECRATION                   = 26573,
    SPELL_PALADIN_CONSECRATION_DAMAGE            = 81297,
    SPELL_PALADIN_CONSECRATION_PROTECTION_AURA   = 188370,
    SPELL_PALADIN_CONSECRATED_BLADE_TALENT       = 404834, // Consecrated Blade (talent)
    SPELL_PALADIN_CONSECRATED_BLADE              = 462970, // Action bar override dummy
    SPELL_PALADIN_CONSECRATED_BLADE_ICD          = 407475, // 10s ICD
    SPELL_PALADIN_BLADE_OF_JUSTICE_AOE           = 404358,
    SPELL_PALADIN_CRUSADER_STRIKE                = 35395,
    SPELL_PALADIN_CRUSADING_STRIKES_TALENT       = 404542, // Talent: replaces auto-attacks
    SPELL_PALADIN_CRUSADING_STRIKES              = 406833, // Hidden stack aura
    SPELL_PALADIN_CRUSADING_STRIKES_DAMAGE       = 408385, // Auto-attack replacement hit
    SPELL_PALADIN_CRUSADING_STRIKES_ENERGIZE     = 406834,
    SPELL_PALADIN_TEMPLAR_STRIKES_TALENT         = 406646, // Talent: CS -> Templar combo
    SPELL_PALADIN_TEMPLAR_STRIKE                = 407480, // Templar Strike
    SPELL_PALADIN_TEMPLAR_SLASH                 = 406647, // Templar Slash (combo follow-up)
    SPELL_PALADIN_DIVINE_AUXILIARY_ENERGIZE      = 408386,
    SPELL_PALADIN_DIVINE_AUXILIARY_TALENT        = 406158,
    SPELL_PALADIN_DIVINE_PURPOSE_TRIGGERED       = 223819,
    SPELL_PALADIN_DIVINE_STEED_HUMAN             = 221883,
    SPELL_PALADIN_DIVINE_STEED_DWARF             = 276111,
    SPELL_PALADIN_DIVINE_STEED_DRAENEI           = 221887,
    SPELL_PALADIN_DIVINE_STEED_DARK_IRON_DWARF   = 276112,
    SPELL_PALADIN_DIVINE_STEED_BLOODELF          = 221886,
    SPELL_PALADIN_DIVINE_STEED_TAUREN            = 221885,
    SPELL_PALADIN_DIVINE_STEED_ZANDALARI_TROLL   = 294133,
    SPELL_PALADIN_DIVINE_STEED_LF_DRAENEI        = 363608,
    SPELL_PALADIN_DIVINE_STORM                   = 53385,
    SPELL_PALADIN_DIVINE_STORM_DAMAGE            = 224239,
    SPELL_PALADIN_SECOND_SUNRISE                 = 431474,
    SPELL_PALADIN_LIGHT_OF_DAWN                  = 85222,
    SPELL_PALADIN_HAMMER_OF_WRATH_LEGACY         = 24275,
    SPELL_PALADIN_TEMPEST_OF_THE_LIGHTBRINGER    = 383396,
    SPELL_PALADIN_TEMPEST_OF_THE_LIGHTBRINGER_AT = 383427,
    SPELL_PALADIN_EMPYREAN_POWER                 = 326732,
    SPELL_PALADIN_EMPYREAN_POWER_BUFF            = 326733,
    SPELL_PALADIN_EMPYREAN_LEGACY                = 387170,
    SPELL_PALADIN_EMPYREAN_LEGACY_BUFF           = 387178,
    SPELL_PALADIN_EMPYREAN_LEGACY_ICD            = 387441,
    SPELL_PALADIN_TEMPLARS_VERDICT               = 85256,
    SPELL_PALADIN_FINAL_VERDICT_CAST             = 383328,
    SPELL_PALADIN_JUSTICARS_VENGEANCE            = 215661,
    SPELL_PALADIN_EMPYREAL_WARD                  = 387791,
    SPELL_PALADIN_EMPYREAL_WARD_ARMOR            = 387792,
    SPELL_PALADIN_LAY_ON_HANDS                   = 633,
    SPELL_PALADIN_LAY_ON_HANDS_EMPYREAL          = 471195,
    SPELL_PALADIN_ENDURING_LIGHT                 = 40471,
    SPELL_PALADIN_ENDURING_JUDGEMENT             = 40472,
    SPELL_PALADIN_ETERNAL_FLAME                  = 156322,
    SPELL_PALADIN_EXECUTION_SENTENCE_DAMAGE      = 387113,
    SPELL_PALADIN_EXECUTION_SENTENCE_11_SECONDS  = 406919,
    SPELL_PALADIN_EXECUTION_SENTENCE_8_SECONDS   = 386579,
    SPELL_PALADIN_EXECUTIONERS_WILL              = 406940,
    SPELL_PALADIN_EXPURGATION_DOT                = 383346,
    SPELL_PALADIN_EYE_FOR_AN_EYE_TRIGGERED       = 205202,
    SPELL_PALADIN_FINAL_STAND                    = 204077,
    SPELL_PALADIN_FINAL_STAND_EFFECT             = 204079,
    SPELL_PALADIN_FINAL_VERDICT                  = 383329,
    SPELL_PALADIN_FORBEARANCE                    = 25771,
    SPELL_PALADIN_GUARDIAN_OF_ANCIENT_KINGS      = 86659,
    SPELL_PALADIN_HAMMER_OF_WRATH                   = 1241413,
    SPELL_PALADIN_HAMMER_OF_WRATH_OVERRIDE       = 1277026,
    SPELL_PALADIN_HAMMER_OF_WRATH_TALENT         = 1241288,
    SPELL_PALADIN_HAMMER_OF_JUSTICE              = 853,
    SPELL_PALADIN_HAMMER_OF_THE_RIGHTEOUS_AOE    = 88263,
    SPELL_PALADIN_HAND_OF_SACRIFICE              = 6940,
    SPELL_PALADIN_HOLY_MENDING                   = 64891,
    SPELL_PALADIN_HOLY_POWER_ARMOR               = 28790,
    SPELL_PALADIN_HOLY_POWER_ATTACK_POWER        = 28791,
    SPELL_PALADIN_HOLY_POWER_SPELL_POWER         = 28793,
    SPELL_PALADIN_HOLY_POWER_MP5                 = 28795,
    SPELL_PALADIN_HOLY_PRISM_AREA_BEAM_VISUAL    = 121551,
    SPELL_PALADIN_HOLY_PRISM_TARGET_ALLY         = 114871,
    SPELL_PALADIN_HOLY_PRISM_TARGET_ENEMY        = 114852,
    SPELL_PALADIN_HOLY_PRISM_TARGET_BEAM_VISUAL  = 114862,
    SPELL_PALADIN_HOLY_SHOCK                     = 20473,
    SPELL_PALADIN_HOLY_SHOCK_DAMAGE              = 25912,
    SPELL_PALADIN_HOLY_SHOCK_HEALING             = 25914,
    SPELL_PALADIN_HOLY_LIGHT                     = 82326,
    SPELL_PALADIN_INFUSION_OF_LIGHT_ENERGIZE     = 356717,
    SPELL_PALADIN_IMMUNE_SHIELD_MARKER           = 61988, // Serverside
    SPELL_PALADIN_ITEM_HEALING_TRANCE            = 37706,
    SPELL_PALADIN_JUDGMENT_GAIN_HOLY_POWER       = 220637,
    SPELL_PALADIN_JUDGMENT_RANK_3                = 315867,
    SPELL_PALADIN_LIGHT_HAMMER_COSMETIC          = 122257,
    SPELL_PALADIN_LIGHT_HAMMER_DAMAGE            = 114919,
    SPELL_PALADIN_LIGHT_HAMMER_HEALING           = 119952,
    SPELL_PALADIN_LIGHT_HAMMER_PERIODIC          = 114918,
    SPELL_PALADIN_RIGHTEOUS_DEFENSE_TAUNT        = 31790,
    SPELL_PALADIN_RIGHTEOUS_VERDICT_AURA         = 267611,
    SPELL_PALADIN_RADIANT_GLORY                  = 458359,
    SPELL_PALADIN_RADIANT_GLORY_PASSIVE          = 462048,
    SPELL_PALADIN_DAWNLIGHT                      = 431377,
    SPELL_PALADIN_DAWNLIGHT_EMPOWER              = 431460,
    SPELL_PALADIN_DAWNLIGHT_CHARGES              = 431522,
    SPELL_PALADIN_DAWNLIGHT_DAMAGE               = 431380,
    SPELL_PALADIN_DAWNLIGHT_HEAL                 = 431381,
    SPELL_PALADIN_DAWNLIGHT_RADIATE_META         = 431581,
    SPELL_PALADIN_DAWNLIGHT_RADIATE_DAMAGE       = 431399,
    SPELL_PALADIN_HOLY_PRISM                     = 114165,
    SPELL_PALADIN_DIVINE_TOLL                    = 375576,
    SPELL_PALADIN_SEAL_OF_RIGHTEOUSNESS          = 25742,
    SPELL_PALADIN_SHIELD_OF_THE_RIGHTEOUS_ARMOR  = 132403,
    SPELL_PALADIN_SHIELD_OF_VENGEANCE_DAMAGE     = 184689,
    SPELL_PALADIN_TEMPLAR_VERDICT_DAMAGE         = 224266,
    SPELL_PALADIN_T30_2P_HEARTFIRE_DAMAGE        = 408399,
    SPELL_PALADIN_T30_2P_HEARTFIRE_HEAL          = 408400,
    SPELL_PALADIN_WAKE_OF_ASHES                  = 255937,
    SPELL_PALADIN_WAKE_OF_ASHES_STUN             = 255941,
    SPELL_PALADIN_ZEAL_AURA                      = 269571
};

enum PaladinCovenantSpells
{
    SPELL_PALADIN_ASHEN_HALLOW                   = 316958,
    SPELL_PALADIN_ASHEN_HALLOW_DAMAGE            = 317221,
    SPELL_PALADIN_ASHEN_HALLOW_HEAL              = 317223,
    SPELL_PALADIN_ASHEN_HALLOW_ALLOW_HAMMER      = 330382
};

enum PaladinSpellVisualKit
{
    PALADIN_VISUAL_KIT_DIVINE_STORM              = 73892
};

enum PaladinSpellVisual
{
    PALADIN_VISUAL_SPELL_HOLY_SHOCK_DAMAGE       = 83731,
    PALADIN_VISUAL_SPELL_HOLY_SHOCK_DAMAGE_CRIT  = 83881,
    PALADIN_VISUAL_SPELL_HOLY_SHOCK_HEAL         = 83732,
    PALADIN_VISUAL_SPELL_HOLY_SHOCK_HEAL_CRIT    = 83880
};

enum PaladinSpellLabel
{
    SPELL_LABEL_PALADIN_T30_2P_HEARTFIRE         = 2598
};

// 469411 - A Just Reward
class spell_pal_a_just_reward : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_A_JUST_REWARD_HEAL });
    }

    static void HandleEffectProc(AuraScript const&, AuraEffect const* aurEff, ProcEventInfo const& eventInfo)
    {
        eventInfo.GetActor()->CastSpell(eventInfo.GetActionTarget(), SPELL_PALADIN_A_JUST_REWARD_HEAL, CastSpellExtraArgsInit{
            .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
            .TriggeringAura = aurEff
        });
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_pal_a_just_reward::HandleEffectProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// 31850 - Ardent Defender
class spell_pal_ardent_defender : public AuraScript
{
    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_ARDENT_DEFENDER_HEAL })
            && ValidateSpellEffect({ { spellInfo->Id, EFFECT_1 } });
    }

    void HandleAbsorb(AuraEffect const* aurEff, DamageInfo const& dmgInfo, uint32& absorbAmount)
    {
        PreventDefaultAction();

        SpellEffectValue targetHealthPercent = GetEffectInfo(EFFECT_1).CalcValue(GetTarget());
        uint64 targetHealth = int32(GetTarget()->CountPctFromMaxHealth(targetHealthPercent));
        if (GetTarget()->HealthBelowPct(targetHealthPercent))
        {
            // we are currently below desired health
            // absorb everything and heal up
            GetTarget()->CastSpell(GetTarget(), SPELL_PALADIN_ARDENT_DEFENDER_HEAL,
                CastSpellExtraArgs(aurEff)
                .AddSpellMod(SPELLVALUE_BASE_POINT0, int32(targetHealth - GetTarget()->GetHealth())));
        }
        else
        {
            // we are currently above desired health
            // just absorb enough to reach that percentage
            absorbAmount = dmgInfo.GetDamage() - int32(GetTarget()->GetHealth() - targetHealth);
        }

        Remove();
    }

    void Register() override
    {
        OnEffectAbsorb += AuraEffectAbsorbOverkillFn(spell_pal_ardent_defender::HandleAbsorb, EFFECT_2);
    }
};

namespace
{
void ResetBladeOfJustice(Unit* target, Spell const* triggeringSpell = nullptr)
{
    if (!target)
        return;

    SpellHistory* history = target->GetSpellHistory();
    auto restore = [history](uint32 spellId)
    {
        history->ResetCooldown(spellId, true);
        if (SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE))
            if (info->ChargeCategoryId)
                history->RestoreCharge(info->ChargeCategoryId);
    };

    restore(SPELL_PALADIN_BLADE_OF_JUSTICE);
    restore(SPELL_PALADIN_BLADE_OF_JUSTICE_AOE);

    target->CastSpell(target, SPELL_PALADIN_ART_OF_WAR_TRIGGERED, CastSpellExtraArgsInit{
        .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
        .TriggeringSpell = triggeringSpell
    });
}
}

// 267344, 406064 - Art of War
class spell_pal_art_of_war : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
        {
            SPELL_PALADIN_ART_OF_WAR_TRIGGERED,
            SPELL_PALADIN_BLADE_OF_JUSTICE,
            SPELL_PALADIN_BLADE_OF_JUSTICE_AOE
        });
    }

    bool CheckProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        // EFFECT_1 is only the crit chance bonus for 406064.
        if (aurEff->GetEffIndex() != EFFECT_0)
            return false;

        int32 chance = aurEff->GetAmountAsInt();
        if (eventInfo.GetHitMask() & PROC_HIT_CRITICAL)
            if (AuraEffect const* critBonus = GetEffect(EFFECT_1))
                chance += critBonus->GetAmountAsInt();

        return roll_chance(chance);
    }

    void HandleProc(AuraEffect* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        ResetBladeOfJustice(GetTarget(), eventInfo.GetProcSpell());
    }

    void Register() override
    {
        DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_art_of_war::CheckProc, EFFECT_0, SPELL_AURA_DUMMY);
        OnEffectProc += AuraEffectProcFn(spell_pal_art_of_war::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_art_of_war::CheckProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
        OnEffectProc += AuraEffectProcFn(spell_pal_art_of_war::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
        DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_art_of_war::CheckProc, EFFECT_1, SPELL_AURA_DUMMY);
        DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_art_of_war::CheckProc, EFFECT_1, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

// 402912 - Righteous Cause
class spell_pal_righteous_cause : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
        {
            SPELL_PALADIN_ART_OF_WAR_TRIGGERED,
            SPELL_PALADIN_BLADE_OF_JUSTICE,
            SPELL_PALADIN_BLADE_OF_JUSTICE_AOE
        });
    }

    bool CheckProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        Spell const* procSpell = eventInfo.GetProcSpell();
        if (!procSpell || !procSpell->HasPowerTypeCost(POWER_HOLY_POWER))
            return false;

        SpellInfo const* procSpellInfo = procSpell->GetSpellInfo();
        if (!procSpellInfo)
            return false;

        Optional<SpellPowerCost> holyPowerCost = procSpellInfo->CalcPowerCost(POWER_HOLY_POWER, false, eventInfo.GetActor(), eventInfo.GetSchoolMask());
        if (!holyPowerCost || holyPowerCost->Amount <= 0)
            return false;

        // Each Holy Power spent has a 6% chance to reset Blade of Justice.
        return roll_chance(aurEff->GetAmountAsInt() * holyPowerCost->Amount);
    }

    void HandleProc(AuraEffect* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        ResetBladeOfJustice(GetTarget(), eventInfo.GetProcSpell());
    }

    void Register() override
    {
        DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_righteous_cause::CheckProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
        OnEffectProc += AuraEffectProcFn(spell_pal_righteous_cause::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
        DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_righteous_cause::CheckProc, EFFECT_0, SPELL_AURA_DUMMY);
        OnEffectProc += AuraEffectProcFn(spell_pal_righteous_cause::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// 19042 - Ashen Hallow
struct areatrigger_pal_ashen_hallow : AreaTriggerAI
{
    areatrigger_pal_ashen_hallow(AreaTrigger* areatrigger) : AreaTriggerAI(areatrigger) {}

    void RefreshPeriod()
    {
        if (Unit* caster = at->GetCaster())
        {
            if (AuraEffect const* ashen = caster->GetAuraEffect(SPELL_PALADIN_ASHEN_HALLOW, EFFECT_1))
                _period = Milliseconds(ashen->GetPeriod());
        }
    }

    void OnCreate(Spell const* /*creatingSpell*/) override
    {
        RefreshPeriod();
        _refreshTimer = _period;
    }

    void OnUpdate(uint32 diff) override
    {
        _refreshTimer -= Milliseconds(diff);

        while (_refreshTimer <= 0s)
        {
            if (Unit* caster = at->GetCaster())
            {
                caster->CastSpell(at->GetPosition(), SPELL_PALADIN_ASHEN_HALLOW_HEAL);
                caster->CastSpell(at->GetPosition(), SPELL_PALADIN_ASHEN_HALLOW_DAMAGE);
            }

            RefreshPeriod();

            _refreshTimer += _period;
        }
    }

    void OnUnitEnter(Unit* unit) override
    {
        if (unit->GetGUID() == at->GetCasterGuid())
            unit->CastSpell(unit, SPELL_PALADIN_ASHEN_HALLOW_ALLOW_HAMMER, true);
    }

    void OnUnitExit(Unit* unit, AreaTriggerExitReason /*reason*/) override
    {
        if (unit->GetGUID() == at->GetCasterGuid())
            unit->RemoveAura(SPELL_PALADIN_ASHEN_HALLOW_ALLOW_HAMMER);
    }

private:
    Milliseconds _refreshTimer;
    Milliseconds _period;
};

// 248033 - Awakening
class spell_pal_awakening : public AuraScript
{
    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_AVENGING_WRATH })
            && ValidateSpellEffect({ { spellInfo->Id, EFFECT_1 } });
    }

    bool CheckProc(AuraEffect const* aurEff, ProcEventInfo& /*eventInfo*/)
    {
        return roll_chance(aurEff->GetAmount());
    }

    void HandleProc(AuraEffect* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        Milliseconds extraDuration = 0ms;
        if (AuraEffect const* durationEffect = GetEffect(EFFECT_1))
            extraDuration = duration_cast<Milliseconds>(FloatSeconds(durationEffect->GetAmount()));

        if (Aura* avengingWrath = GetTarget()->GetAura(SPELL_PALADIN_AVENGING_WRATH))
        {
            avengingWrath->SetDuration(avengingWrath->GetDuration() + extraDuration.count());
            avengingWrath->SetMaxDuration(avengingWrath->GetMaxDuration() + extraDuration.count());
        }
        else
            GetTarget()->CastSpell(GetTarget(), SPELL_PALADIN_AVENGING_WRATH,
                CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_IGNORE_SPELL_AND_CATEGORY_CD)
                    .SetTriggeringSpell(eventInfo.GetProcSpell())
                    .AddSpellMod(SPELLVALUE_DURATION, extraDuration.count()));
    }

    void Register() override
    {
        DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_awakening::CheckProc, EFFECT_0, SPELL_AURA_DUMMY);
        OnEffectProc += AuraEffectProcFn(spell_pal_awakening::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// Called by 184575 - Blade of Justice
class spell_pal_blade_of_vengeance : public SpellScript
{
    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_BLADE_OF_VENGEANCE })
            && ValidateSpellEffect({ { spellInfo->Id, EFFECT_2 } })
            && spellInfo->GetEffect(EFFECT_2).IsEffect(SPELL_EFFECT_TRIGGER_SPELL);
    }

    bool Load() override
    {
        return !GetCaster()->HasAura(SPELL_PALADIN_BLADE_OF_VENGEANCE);
    }

    static void PreventProc(SpellScript const&, WorldObject*& target)
    {
        target = nullptr;
    }

    void Register() override
    {
        OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_pal_blade_of_vengeance::PreventProc, EFFECT_2, TARGET_UNIT_TARGET_ENEMY);
    }
};

// 404358 - Blade of Justice
class spell_pal_blade_of_vengeance_aoe_target_selector : public SpellScript
{
    void RemoveExplicitTarget(std::list<WorldObject*>& targets) const
    {
        targets.remove(GetExplTargetWorldObject());
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pal_blade_of_vengeance_aoe_target_selector::RemoveExplicitTarget, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
    }
};

// Called by 184575 / 404358 - Blade of Justice
// 404834 - Consecrated Blade: cast Consecration at the target's location (ICD 407475, 10s)
class spell_pal_consecrated_blade : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
        {
            SPELL_PALADIN_CONSECRATION,
            SPELL_PALADIN_CONSECRATED_BLADE_TALENT,
            SPELL_PALADIN_CONSECRATED_BLADE_ICD
        });
    }

    bool Load() override
    {
        return GetCaster()->HasAura(SPELL_PALADIN_CONSECRATED_BLADE_TALENT)
            || GetCaster()->HasAura(SPELL_PALADIN_CONSECRATED_BLADE);
    }

    void HandleAfterCast() const
    {
        Unit* caster = GetCaster();
        Unit* target = GetExplTargetUnit();
        if (!caster || !target)
            return;

        if (caster->HasAura(SPELL_PALADIN_CONSECRATED_BLADE_ICD))
            return;

        // Consecration destinations are caster-relative; temporarily relocate so the AT spawns under the target.
        Position const home = caster->GetPosition();
        Position dest = target->GetPosition();
        caster->UpdateAllowedPositionZ(dest.m_positionX, dest.m_positionY, dest.m_positionZ);
        caster->Relocate(dest);

        caster->CastSpell(caster, SPELL_PALADIN_CONSECRATION, CastSpellExtraArgsInit{
            .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_IGNORE_SPELL_AND_CATEGORY_CD
                | TRIGGERED_IGNORE_GCD | TRIGGERED_DONT_REPORT_CAST_ERROR,
            .TriggeringSpell = GetSpell()
        });

        caster->Relocate(home);

        caster->CastSpell(caster, SPELL_PALADIN_CONSECRATED_BLADE_ICD, CastSpellExtraArgsInit{
            .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
            .TriggeringSpell = GetSpell()
        });
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_pal_consecrated_blade::HandleAfterCast);
    }
};

// 1022 - Blessing of Protection
// 204018 - Blessing of Spellwarding
class spell_pal_blessing_of_protection : public SpellScript
{
    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellInfo(
        {
            SPELL_PALADIN_FORBEARANCE,
            SPELL_PALADIN_IMMUNE_SHIELD_MARKER
        }) && spellInfo->ExcludeTargetAuraSpell == SPELL_PALADIN_IMMUNE_SHIELD_MARKER;
    }

    void TriggerForbearance() const
    {
        GetCaster()->CastSpell(GetHitUnit(), SPELL_PALADIN_FORBEARANCE, CastSpellExtraArgsInit{
            .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
            .TriggeringSpell = GetSpell()
        });
    }

    void Register() override
    {
        AfterHit += SpellHitFn(spell_pal_blessing_of_protection::TriggerForbearance);
    }
};

// 115750 - Blinding Light
class spell_pal_blinding_light : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_BLINDING_LIGHT_EFFECT });
    }

    void HandleDummy(SpellEffIndex /*effIndex*/) const
    {
        GetCaster()->CastSpell(GetHitUnit(), SPELL_PALADIN_BLINDING_LIGHT_EFFECT, CastSpellExtraArgsInit{
            .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
            .TriggeringSpell = GetSpell()
        });
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_pal_blinding_light::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 26573 - Consecration
class spell_pal_consecration : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo
        ({
            SPELL_PALADIN_CONSECRATION_DAMAGE,
            // validate for areatrigger_pal_consecration
            SPELL_PALADIN_CONSECRATION_PROTECTION_AURA,
            SPELL_PALADIN_CONSECRATION_DEBUFF
        });
    }

    void HandleEffectPeriodic(AuraEffect const* /*aurEff*/)
    {
        if (AreaTrigger* at = GetTarget()->GetAreaTrigger(SPELL_PALADIN_CONSECRATION))
            GetTarget()->CastSpell(at->GetPosition(), SPELL_PALADIN_CONSECRATION_DAMAGE);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_pal_consecration::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }
};

// 26573 - Consecration
//  9228 - AreaTriggerId
struct areatrigger_pal_consecration : AreaTriggerAI
{
    using AreaTriggerAI::AreaTriggerAI;

    void OnUnitEnter(Unit* unit) override
    {
        if (Unit* caster = at->GetCaster())
        {
            // 243597 is also being cast as protection, but CreateObject is not sent, either serverside areatrigger for this aura or unused - also no visual is seen
            if (unit == caster && caster->IsPlayer() && caster->ToPlayer()->GetPrimarySpecialization() == ChrSpecialization::PaladinProtection)
                caster->CastSpell(caster, SPELL_PALADIN_CONSECRATION_PROTECTION_AURA);

            // 204054 - Consecrated Ground slow is handled by DBC and needs no further checks
            if (caster->IsValidAttackTarget(unit))
                caster->CastSpell(unit, SPELL_PALADIN_CONSECRATION_DEBUFF);
        }
    }

    void OnUnitExit(Unit* unit, AreaTriggerExitReason /*reason*/) override
    {
        if (at->GetCasterGuid() == unit->GetGUID())
            unit->RemoveAurasDueToSpell(SPELL_PALADIN_CONSECRATION_PROTECTION_AURA, at->GetCasterGuid());

        unit->RemoveAurasDueToSpell(SPELL_PALADIN_CONSECRATION_DEBUFF, at->GetCasterGuid());
    }
};

// 196926 - Crusader Might
class spell_pal_crusader_might : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_HOLY_SHOCK });
    }

    void HandleEffectProc(AuraEffect* aurEff, ProcEventInfo& /*eventInfo*/)
    {
        GetTarget()->GetSpellHistory()->ModifyCooldown(SPELL_PALADIN_HOLY_SHOCK, Milliseconds(aurEff->GetAmountAsInt()));
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_pal_crusader_might::HandleEffectProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// 406833 - Crusading Strikes
class spell_pal_crusading_strikes : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_CRUSADING_STRIKES_ENERGIZE });
    }

    void HandleEffectProc(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        if (GetStackAmount() == 2)
        {
            GetTarget()->CastSpell(GetTarget(), SPELL_PALADIN_CRUSADING_STRIKES_ENERGIZE, aurEff);

            // this spell has weird proc order dependency set up in db2 data so we do removal manually
            Remove();
        }
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(spell_pal_crusading_strikes::HandleEffectProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
    }
};

// 406158 - Divine Auxiliary (attached to 343721 - Final Reckoning and 343527 - Execution Sentence)
class spell_pal_divine_auxiliary : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_DIVINE_AUXILIARY_ENERGIZE, SPELL_PALADIN_DIVINE_AUXILIARY_TALENT });
    }

    bool Load() override
    {
        return GetCaster()->HasAura(SPELL_PALADIN_DIVINE_AUXILIARY_TALENT);
    }

    void HandleEnergize() const
    {
        Unit* caster = GetCaster();
        caster->CastSpell(caster, SPELL_PALADIN_DIVINE_AUXILIARY_ENERGIZE, CastSpellExtraArgsInit{
            .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
            .TriggeringSpell = GetSpell()
        });
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_pal_divine_auxiliary::HandleEnergize);
    }
};

// 223817 - Divine Purpose
class spell_pal_divine_purpose : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_DIVINE_PURPOSE_TRIGGERED });
    }

    bool CheckProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        Spell const* procSpell = eventInfo.GetProcSpell();
        if (!procSpell)
            return false;

        if (!procSpell->HasPowerTypeCost(POWER_HOLY_POWER))
            return false;

        return roll_chance(aurEff->GetAmount());
    }

    void HandleProc(AuraEffect* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        eventInfo.GetActor()->CastSpell(eventInfo.GetActor(), SPELL_PALADIN_DIVINE_PURPOSE_TRIGGERED,
            CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS).SetTriggeringSpell(eventInfo.GetProcSpell()));
    }

    void Register() override
    {
        DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_divine_purpose::CheckProc, EFFECT_0, SPELL_AURA_DUMMY);
        OnEffectProc += AuraEffectProcFn(spell_pal_divine_purpose::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// 642 - Divine Shield
class spell_pal_divine_shield : public SpellScript
{
    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellInfo(
        {
            SPELL_PALADIN_FINAL_STAND,
            SPELL_PALADIN_FINAL_STAND_EFFECT,
            SPELL_PALADIN_FORBEARANCE,
            SPELL_PALADIN_IMMUNE_SHIELD_MARKER
        }) && spellInfo->ExcludeCasterAuraSpell == SPELL_PALADIN_IMMUNE_SHIELD_MARKER;
    }

    void HandleFinalStand()
    {
        if (GetCaster()->HasAura(SPELL_PALADIN_FINAL_STAND))
            GetCaster()->CastSpell(nullptr, SPELL_PALADIN_FINAL_STAND_EFFECT, true);
    }

    void TriggerForbearance() const
    {
        GetCaster()->CastSpell(GetHitUnit(), SPELL_PALADIN_FORBEARANCE, CastSpellExtraArgsInit{
            .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
            .TriggeringSpell = GetSpell()
        });
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_pal_divine_shield::HandleFinalStand);
        AfterHit += SpellHitFn(spell_pal_divine_shield::TriggerForbearance);
    }
};

// 190784 - Divine Steed
class spell_pal_divine_steed : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
        {
            SPELL_PALADIN_DIVINE_STEED_HUMAN,
            SPELL_PALADIN_DIVINE_STEED_DWARF,
            SPELL_PALADIN_DIVINE_STEED_DRAENEI,
            SPELL_PALADIN_DIVINE_STEED_DARK_IRON_DWARF,
            SPELL_PALADIN_DIVINE_STEED_BLOODELF,
            SPELL_PALADIN_DIVINE_STEED_TAUREN,
            SPELL_PALADIN_DIVINE_STEED_ZANDALARI_TROLL,
            SPELL_PALADIN_DIVINE_STEED_LF_DRAENEI
        });
    }

    void HandleOnCast()
    {
        Unit* caster = GetCaster();

        uint32 spellId = SPELL_PALADIN_DIVINE_STEED_HUMAN;
        switch (caster->GetRace())
        {
            case RACE_HUMAN:
                spellId = SPELL_PALADIN_DIVINE_STEED_HUMAN;
                break;
            case RACE_DWARF:
                spellId = SPELL_PALADIN_DIVINE_STEED_DWARF;
                break;
            case RACE_DRAENEI:
                spellId = SPELL_PALADIN_DIVINE_STEED_DRAENEI;
                break;
            case RACE_LIGHTFORGED_DRAENEI:
                spellId = SPELL_PALADIN_DIVINE_STEED_LF_DRAENEI;
                break;
            case RACE_DARK_IRON_DWARF:
                spellId = SPELL_PALADIN_DIVINE_STEED_DARK_IRON_DWARF;
                break;
            case RACE_BLOODELF:
                spellId = SPELL_PALADIN_DIVINE_STEED_BLOODELF;
                break;
            case RACE_TAUREN:
                spellId = SPELL_PALADIN_DIVINE_STEED_TAUREN;
                break;
            case RACE_ZANDALARI_TROLL:
                spellId = SPELL_PALADIN_DIVINE_STEED_ZANDALARI_TROLL;
                break;
            default:
                break;
        }

        caster->CastSpell(caster, spellId, true);
    }

    void Register() override
    {
        OnCast += SpellCastFn(spell_pal_divine_steed::HandleOnCast);
    }
};

namespace PaladinTempest
{
    struct WaveData
    {
        int32 EffectivenessPct = 20;
    };

    static void ApplyWaveEffectiveness(Spell const* spell, float& pctMod)
    {
        if (WaveData const* wave = std::any_cast<WaveData>(&spell->m_customArg))
            ApplyPct(pctMod, wave->EffectivenessPct);
    }
}

// 53385 - Divine Storm
class spell_pal_divine_storm : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return sSpellVisualKitStore.HasRecord(PALADIN_VISUAL_KIT_DIVINE_STORM)
            && ValidateSpellInfo(
            {
                SPELL_PALADIN_TEMPEST_OF_THE_LIGHTBRINGER,
                SPELL_PALADIN_TEMPEST_OF_THE_LIGHTBRINGER_AT,
                SPELL_PALADIN_DIVINE_STORM_DAMAGE,
                SPELL_PALADIN_EMPYREAN_POWER_BUFF
            });
    }

    void HandleOnCast()
    {
        GetCaster()->SendPlaySpellVisualKit(PALADIN_VISUAL_KIT_DIVINE_STORM, 0, 0);
    }

    void HandleDamage(SpellEffectInfo const& /*spellEffectInfo*/, Unit const* /*victim*/, int32& /*damage*/, int32& /*flatMod*/, float& pctMod) const
    {
        if (int32 const* bonusPct = std::any_cast<int32>(&GetSpell()->m_customArg))
            AddPct(pctMod, *bonusPct);
    }

    void HandleAfterCast()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        // Empyrean Power buff is consumed by the free Divine Storm.
        if (Aura* empyreanPower = caster->GetAura(SPELL_PALADIN_EMPYREAN_POWER_BUFF))
            empyreanPower->Remove();

        AuraEffect const* tempestEff = caster->GetAuraEffect(SPELL_PALADIN_TEMPEST_OF_THE_LIGHTBRINGER, EFFECT_0);
        if (!tempestEff)
            return;

        int32 const rangeYards = std::max(tempestEff->GetAmountAsInt(), 1);
        int32 effectivenessPct = 20;
        if (AuraEffect const* pctEff = caster->GetAuraEffect(SPELL_PALADIN_TEMPEST_OF_THE_LIGHTBRINGER, EFFECT_1))
            effectivenessPct = std::max(pctEff->GetAmountAsInt(), 1);

        // Projectile / wave visual (AreaTriggers 4366 / 34998).
        caster->CastSpell(caster, SPELL_PALADIN_TEMPEST_OF_THE_LIGHTBRINGER_AT, CastSpellExtraArgsInit{
            .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
            .TriggeringSpell = GetSpell()
        });

        std::list<Unit*> targets;
        Trinity::AnyUnfriendlyUnitInObjectRangeCheck check(caster, caster, float(rangeYards));
        Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(caster, targets, check);
        Cell::VisitAllObjects(caster, searcher, float(rangeYards));

        for (Unit* target : targets)
        {
            if (!caster->IsValidAttackTarget(target) || !caster->isInFront(target))
                continue;

            caster->CastSpell(target, SPELL_PALADIN_DIVINE_STORM_DAMAGE, CastSpellExtraArgsInit{
                .TriggerFlags = TRIGGERED_IGNORE_GCD | TRIGGERED_IGNORE_POWER_COST | TRIGGERED_IGNORE_CAST_IN_PROGRESS
                    | TRIGGERED_DONT_REPORT_CAST_ERROR | TRIGGERED_IGNORE_SPELL_AND_CATEGORY_CD,
                .TriggeringSpell = GetSpell(),
                .CustomArg = PaladinTempest::WaveData{ .EffectivenessPct = effectivenessPct }
            });
        }
    }

    void Register() override
    {
        OnCast += SpellCastFn(spell_pal_divine_storm::HandleOnCast);
        AfterCast += SpellCastFn(spell_pal_divine_storm::HandleAfterCast);
        CalcDamage += SpellCalcDamageFn(spell_pal_divine_storm::HandleDamage);
    }
};

// 224239 - Divine Storm (damage)
class spell_pal_divine_storm_damage : public SpellScript
{
    void HandleDamage(SpellEffectInfo const& /*spellEffectInfo*/, Unit const* /*victim*/, int32& /*damage*/, int32& /*flatMod*/, float& pctMod) const
    {
        if (int32 const* bonusPct = std::any_cast<int32>(&GetSpell()->m_customArg))
            AddPct(pctMod, *bonusPct);

        PaladinTempest::ApplyWaveEffectiveness(GetSpell(), pctMod);
    }

    void Register() override
    {
        CalcDamage += SpellCalcDamageFn(spell_pal_divine_storm_damage::HandleDamage);
    }
};

// 326732 - Empyrean Power: CS / Templar Strike / Crusading Strikes can grant a free empowered Divine Storm.
class spell_pal_empyrean_power : public AuraScript
{
    ObjectGuid _lastAttemptCastId;

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
        {
            SPELL_PALADIN_EMPYREAN_POWER_BUFF,
            SPELL_PALADIN_CRUSADER_STRIKE,
            SPELL_PALADIN_CRUSADING_STRIKES_TALENT,
            SPELL_PALADIN_CRUSADING_STRIKES_DAMAGE,
            SPELL_PALADIN_TEMPLAR_STRIKES_TALENT,
            SPELL_PALADIN_TEMPLAR_STRIKE,
            SPELL_PALADIN_TEMPLAR_SLASH
        });
    }

    static bool HasCrusadingStrikesTalent(Unit const* unit)
    {
        if (unit->HasAura(SPELL_PALADIN_CRUSADING_STRIKES_TALENT))
            return true;

        if (Player const* player = unit->ToPlayer())
            return player->HasSpell(SPELL_PALADIN_CRUSADING_STRIKES_TALENT);

        return false;
    }

    static bool IsCrusaderStrikeFamilySpell(uint32 spellId)
    {
        switch (spellId)
        {
            case SPELL_PALADIN_CRUSADER_STRIKE:
            case SPELL_PALADIN_TEMPLAR_STRIKE: // 407480 - from talent 406646
            case SPELL_PALADIN_TEMPLAR_SLASH:  // 406647 - combo follow-up
                return true;
            default:
                return false;
        }
    }

    bool CheckProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        if (aurEff->GetEffIndex() != EFFECT_0 && aurEff->GetEffIndex() != EFFECT_1)
            return false;

        Spell const* procSpell = eventInfo.GetProcSpell();
        SpellInfo const* spellInfo = eventInfo.GetSpellInfo();
        Unit* caster = GetTarget();

        int32 chance = 0;
        if (spellInfo && IsCrusaderStrikeFamilySpell(spellInfo->Id))
        {
            // EFFECT_0: Crusader Strike / Templar Strike / Templar Slash chance (15%).
            if (aurEff->GetEffIndex() != EFFECT_0)
                return false;
            chance = GetEffectInfo(EFFECT_0).CalcValueAsInt(caster);
        }
        else if (spellInfo && spellInfo->Id == SPELL_PALADIN_CRUSADING_STRIKES_DAMAGE
            && HasCrusadingStrikesTalent(caster))
        {
            // EFFECT_1 path: Crusading Strikes (404542 -> 408385) — same 15% as Crusader Strike.
            if (aurEff->GetEffIndex() != EFFECT_1)
                return false;
            chance = GetEffectInfo(EFFECT_0).CalcValueAsInt(caster);
        }
        else
            return false;

        if (chance <= 0)
            return false;

        // AoE / multi-hit: only one roll per cast. White swings have no cast id — roll each swing.
        if (procSpell)
        {
            if (procSpell->m_castId == _lastAttemptCastId)
                return false;
            _lastAttemptCastId = procSpell->m_castId;
        }

        return roll_chance(chance);
    }

    void HandleProc(AuraEffect* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        eventInfo.GetActor()->CastSpell(eventInfo.GetActor(), SPELL_PALADIN_EMPYREAN_POWER_BUFF, CastSpellExtraArgsInit{
            .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
            .TriggeringSpell = eventInfo.GetProcSpell()
        });
    }

    void Register() override
    {
        DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_empyrean_power::CheckProc, EFFECT_0, SPELL_AURA_DUMMY);
        OnEffectProc += AuraEffectProcFn(spell_pal_empyrean_power::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_empyrean_power::CheckProc, EFFECT_1, SPELL_AURA_DUMMY);
        OnEffectProc += AuraEffectProcFn(spell_pal_empyrean_power::HandleProc, EFFECT_1, SPELL_AURA_DUMMY);
    }
};

namespace PaladinSecondSunrise
{
    struct EchoData
    {
        int32 EffectivenessPct = 100;
    };

    static bool IsEchoableSpell(uint32 spellId)
    {
        switch (spellId)
        {
            case SPELL_PALADIN_DIVINE_STORM:
            case SPELL_PALADIN_HAMMER_OF_WRATH:
            case SPELL_PALADIN_HAMMER_OF_WRATH_LEGACY:
            case SPELL_PALADIN_HOLY_SHOCK:
            case SPELL_PALADIN_LIGHT_OF_DAWN:
                return true;
            default:
                return false;
        }
    }

    static void ApplyEffectiveness(Spell const* spell, float& pctMod)
    {
        if (EchoData const* echo = std::any_cast<EchoData>(&spell->m_customArg))
            ApplyPct(pctMod, echo->EffectivenessPct);
    }

    static void TryEcho(Unit* caster, Spell const* procSpell, AuraEffect const* aurEff)
    {
        if (!caster || !procSpell || !aurEff)
            return;

        Aura const* aura = aurEff->GetBase();
        if (!aura)
            return;

        // Use CalcValue so Ret aura 1258011 (+5% chance / +20% effectiveness via label 4716) applies.
        int32 effectivenessPct = aura->GetSpellInfo()->GetEffect(EFFECT_1).CalcValueAsInt(caster);

        uint32 const spellId = procSpell->GetSpellInfo()->Id;
        ObjectGuid targetGuid;
        switch (spellId)
        {
            case SPELL_PALADIN_DIVINE_STORM:
            case SPELL_PALADIN_LIGHT_OF_DAWN:
                targetGuid = caster->GetGUID();
                break;
            default:
                if (Unit* target = procSpell->m_targets.GetUnitTarget())
                    targetGuid = target->GetGUID();
                else
                    targetGuid = caster->GetGUID();
                break;
        }

        caster->m_Events.AddEventAtOffset([caster, targetGuid, spellId, effectivenessPct]()
        {
            Unit* target = ObjectAccessor::GetUnit(*caster, targetGuid);
            if (!target)
                return;

            caster->CastSpell(target, spellId, CastSpellExtraArgsInit{
                .TriggerFlags = TRIGGERED_IGNORE_GCD | TRIGGERED_IGNORE_POWER_COST | TRIGGERED_IGNORE_CAST_IN_PROGRESS
                    | TRIGGERED_DONT_REPORT_CAST_ERROR | TRIGGERED_IGNORE_SPELL_AND_CATEGORY_CD,
                .CustomArg = EchoData{ .EffectivenessPct = effectivenessPct }
            });
        }, 200ms);
    }
}
// 431474 - Second Sunrise: DS/HoW (Ret) and Holy Shock/Light of Dawn (Holy)
// have a chance to cast again at reduced effectiveness.
class spell_pal_second_sunrise : public AuraScript
{
    ObjectGuid _lastAttemptCastId;

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
        {
            SPELL_PALADIN_DIVINE_STORM,
            SPELL_PALADIN_HAMMER_OF_WRATH,
            SPELL_PALADIN_HAMMER_OF_WRATH_LEGACY,
            SPELL_PALADIN_HOLY_SHOCK,
            SPELL_PALADIN_LIGHT_OF_DAWN
        });
    }

    bool CheckProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        if (aurEff->GetEffIndex() != EFFECT_0)
            return false;

        Spell const* procSpell = eventInfo.GetProcSpell();
        if (!procSpell || !PaladinSecondSunrise::IsEchoableSpell(procSpell->GetSpellInfo()->Id))
            return false;

        // Echoes must not echo again.
        if (std::any_cast<PaladinSecondSunrise::EchoData>(&procSpell->m_customArg))
            return false;

        // Divine Storm / Light of Dawn hit multiple targets; only roll once per cast.
        if (procSpell->m_castId == _lastAttemptCastId)
            return false;
        _lastAttemptCastId = procSpell->m_castId;

        // CalcValue includes Ret flat mods (15+5=20 chance on Ret; base 15 on Holy).
        return roll_chance(GetEffectInfo(EFFECT_0).CalcValueAsInt(GetTarget()));
    }

    void HandleProc(AuraEffect* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        PaladinSecondSunrise::TryEcho(GetTarget(), eventInfo.GetProcSpell(), aurEff);
    }

    void Register() override
    {
        DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_second_sunrise::CheckProc, EFFECT_0, SPELL_AURA_DUMMY);
        OnEffectProc += AuraEffectProcFn(spell_pal_second_sunrise::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_second_sunrise::CheckProc, EFFECT_1, SPELL_AURA_DUMMY);
    }
};

// Applies Second Sunrise echo effectiveness to damage/healing spells.
class spell_pal_second_sunrise_effectiveness : public SpellScript
{
    void HandleDamage(SpellEffectInfo const& /*spellEffectInfo*/, Unit const* /*victim*/, int32& /*damage*/, int32& /*flatMod*/, float& pctMod) const
    {
        PaladinSecondSunrise::ApplyEffectiveness(GetSpell(), pctMod);
    }

    void HandleHealing(SpellEffectInfo const& /*spellEffectInfo*/, Unit const* /*victim*/, int32& /*healing*/, int32& /*flatMod*/, float& pctMod) const
    {
        PaladinSecondSunrise::ApplyEffectiveness(GetSpell(), pctMod);
    }

    void Register() override
    {
        CalcDamage += SpellCalcDamageFn(spell_pal_second_sunrise_effectiveness::HandleDamage);
        CalcHealing += SpellCalcHealingFn(spell_pal_second_sunrise_effectiveness::HandleHealing);
    }
};

namespace PaladinEmpyreanLegacy
{
    static bool IsAvengingWrathSpell(uint32 spellId)
    {
        switch (spellId)
        {
            case SPELL_PALADIN_AVENGING_WRATH: // 31884
            case 231895: // Avenging Wrath / Crusade variants
            case 384376:
            case 454351:
            case 454373:
                return true;
            default:
                return false;
        }
    }

    static bool IsSingleTargetHolyPowerSpender(uint32 spellId)
    {
        switch (spellId)
        {
            case SPELL_PALADIN_TEMPLARS_VERDICT:      // 85256
            case SPELL_PALADIN_FINAL_VERDICT_CAST:    // 383328
            case SPELL_PALADIN_JUSTICARS_VENGEANCE:   // 215661
                return true;
            default:
                return false;
        }
    }

    static void TryTriggerDivineStorm(Unit* caster, Spell const* triggeringSpell)
    {
        if (!caster)
            return;

        Aura* buff = caster->GetAura(SPELL_PALADIN_EMPYREAN_LEGACY_BUFF);
        if (!buff)
            return;

        int32 bonusPct = 25;
        if (AuraEffect const* aurEff = buff->GetEffect(EFFECT_0))
            bonusPct = aurEff->GetAmountAsInt();

        buff->Remove();

        caster->CastSpell(caster, SPELL_PALADIN_DIVINE_STORM, CastSpellExtraArgsInit{
            .TriggerFlags = TRIGGERED_IGNORE_GCD | TRIGGERED_IGNORE_POWER_COST | TRIGGERED_IGNORE_CAST_IN_PROGRESS
                | TRIGGERED_DONT_REPORT_CAST_ERROR | TRIGGERED_IGNORE_SPELL_AND_CATEGORY_CD,
            .TriggeringSpell = triggeringSpell,
            .CustomArg = bonusPct
        });
    }
}

// 387170 - Empyrean Legacy
class spell_pal_empyrean_legacy : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
        {
            SPELL_PALADIN_EMPYREAN_LEGACY_BUFF,
            SPELL_PALADIN_EMPYREAN_LEGACY_ICD,
            SPELL_PALADIN_DIVINE_STORM
        });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        SpellInfo const* spellInfo = eventInfo.GetSpellInfo();
        if (!spellInfo || !PaladinEmpyreanLegacy::IsAvengingWrathSpell(spellInfo->Id))
            return false;

        Unit* actor = eventInfo.GetActor();
        if (!actor || actor->HasAura(SPELL_PALADIN_EMPYREAN_LEGACY_ICD))
            return false;

        return true;
    }

    void HandleProc(AuraEffect* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        Unit* actor = eventInfo.GetActor();
        if (!actor)
            return;

        // Default action applies 387178; also start the talent ICD.
        actor->CastSpell(actor, SPELL_PALADIN_EMPYREAN_LEGACY_ICD, CastSpellExtraArgsInit{
            .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
            .TriggeringSpell = eventInfo.GetProcSpell()
        });
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_pal_empyrean_legacy::CheckProc);
        OnEffectProc += AuraEffectProcFn(spell_pal_empyrean_legacy::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

// 387178 - Empyrean Legacy (buff): next ST Holy Power spender casts empowered Divine Storm
class spell_pal_empyrean_legacy_buff : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
        {
            SPELL_PALADIN_DIVINE_STORM,
            SPELL_PALADIN_TEMPLARS_VERDICT,
            SPELL_PALADIN_FINAL_VERDICT_CAST,
            SPELL_PALADIN_JUSTICARS_VENGEANCE
        });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        SpellInfo const* spellInfo = eventInfo.GetSpellInfo();
        return spellInfo && PaladinEmpyreanLegacy::IsSingleTargetHolyPowerSpender(spellInfo->Id);
    }

    void HandleProc(AuraEffect* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        PaladinEmpyreanLegacy::TryTriggerDivineStorm(GetTarget(), eventInfo.GetProcSpell());
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_pal_empyrean_legacy_buff::CheckProc);
        OnEffectProc += AuraEffectProcFn(spell_pal_empyrean_legacy_buff::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        OnEffectProc += AuraEffectProcFn(spell_pal_empyrean_legacy_buff::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

// Called by ST Holy Power spenders - fallback if 387178 has no usable proc flags
class spell_pal_empyrean_legacy_spender : public SpellScript
{
    bool Load() override
    {
        return GetCaster()->HasAura(SPELL_PALADIN_EMPYREAN_LEGACY_BUFF);
    }

    void HandleAfterCast() const
    {
        PaladinEmpyreanLegacy::TryTriggerDivineStorm(GetCaster(), GetSpell());
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_pal_empyrean_legacy_spender::HandleAfterCast);
    }
};

// 387791 - Empyreal Ward
class spell_pal_empyreal_ward : public AuraScript
{
    static bool IsLayOnHandsSpell(uint32 spellId)
    {
        switch (spellId)
        {
            case SPELL_PALADIN_LAY_ON_HANDS:          // 633
            case SPELL_PALADIN_LAY_ON_HANDS_EMPYREAL: // 471195
                return true;
            default:
                return false;
        }
    }

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
        {
            SPELL_PALADIN_EMPYREAL_WARD_ARMOR,
            SPELL_PALADIN_LAY_ON_HANDS,
            SPELL_PALADIN_LAY_ON_HANDS_EMPYREAL
        });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        SpellInfo const* spellInfo = eventInfo.GetSpellInfo();
        return spellInfo && IsLayOnHandsSpell(spellInfo->Id);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_pal_empyreal_ward::CheckProc);
    }
};

namespace PaladinHammerOfWrath
{
    static constexpr std::array<uint32, 4> AvengingWrathAuraIds =
    {
        SPELL_PALADIN_AVENGING_WRATH,
        231895, // Avenging Wrath (Holy)
        454351, // Avenging Wrath
        454373  // Avenging Wrath
    };

    static constexpr std::array<uint32, 5> JudgmentSpellIds =
    {
        20271,  // Judgment
        275773, // Judgment (Retribution)
        275779, // Judgment (Protection)
        406957, // Judgment
        197277  // Judgment
    };

    static bool HasHammerOfWrathTalent(Unit const* unit)
    {
        if (unit->HasAura(SPELL_PALADIN_HAMMER_OF_WRATH_TALENT))
            return true;

        if (Player const* player = unit->ToPlayer())
            return player->HasSpell(SPELL_PALADIN_HAMMER_OF_WRATH_TALENT);

        return false;
    }

    static bool HasAvengingWrathAura(Unit const* unit, uint32 excludeAuraId = 0)
    {
        for (uint32 auraId : AvengingWrathAuraIds)
        {
            if (excludeAuraId && auraId == excludeAuraId)
                continue;

            if (unit->HasAura(auraId))
                return true;
        }

        return false;
    }

    static uint32 GetPrimaryJudgmentSpell(Player const* player)
    {
        for (uint32 judgmentSpellId : JudgmentSpellIds)
            if (player->HasSpell(judgmentSpellId))
                return judgmentSpellId;

        return 0;
    }

    static uint32 GetSpecJudgmentSpell(Player const* player)
    {
        for (uint32 judgmentSpellId : JudgmentSpellIds)
        {
            if (judgmentSpellId == 20271)
                continue;

            if (player->HasSpell(judgmentSpellId))
                return judgmentSpellId;
        }

        return 0;
    }

    static bool IsJudgmentSpell(uint32 spellId)
    {
        for (uint32 judgmentSpellId : JudgmentSpellIds)
            if (judgmentSpellId == spellId)
                return true;

        return false;
    }

    static bool HasPermanentSpell(Player* player, uint32 spellId)
    {
        PlayerSpellMap::iterator itr = player->GetSpellMap().find(spellId);
        return itr != player->GetSpellMap().end()
            && itr->second.state != PLAYERSPELL_REMOVED
            && itr->second.state != PLAYERSPELL_TEMPORARY;
    }

    static std::unordered_map<ObjectGuid, std::unordered_set<uint32>> SupercededJudgments;
    static std::unordered_map<ObjectGuid, uint32> SuppressedSpecJudgments;
    static std::unordered_map<ObjectGuid, std::unordered_map<uint8, uint32>> SwappedActionButtons;
    static std::unordered_set<ObjectGuid> ActiveJudgmentOverrides;

    static void SuppressSpecJudgmentOverride(Player* player)
    {
        uint32 const specJudgmentSpellId = GetSpecJudgmentSpell(player);
        if (!specJudgmentSpellId)
            return;

        ObjectGuid const guid = player->GetGUID();
        if (SuppressedSpecJudgments.contains(guid))
            return;

        player->RemoveOverrideSpell(20271, specJudgmentSpellId);
        SuppressedSpecJudgments.emplace(guid, specJudgmentSpellId);
    }

    static void RestoreSpecJudgmentOverride(Player* player)
    {
        auto itr = SuppressedSpecJudgments.find(player->GetGUID());
        if (itr == SuppressedSpecJudgments.end())
            return;

        uint32 const specJudgmentSpellId = itr->second;
        SuppressedSpecJudgments.erase(itr);

        player->AddOverrideSpell(20271, specJudgmentSpellId);
        player->SendSupercededSpell(20271, specJudgmentSpellId);
    }

    // Reason 0: client does not validate that the spell is known (needed for temporary HoW).
    static void SyncActionButtons(Player* player)
    {
        player->SendActionButtons(0);
    }

    static void SwapActionBarJudgmentToHammerOfWrath(Player* player, uint32 hammerOfWrathSpellId)
    {
        ObjectGuid const guid = player->GetGUID();
        std::unordered_map<uint8, uint32>& swapped = SwappedActionButtons[guid];

        for (auto const& [button, actionButton] : player->GetActionButtons())
        {
            if (actionButton.uState == ACTIONBUTTON_DELETED)
                continue;

            if (actionButton.GetType() != ACTION_BUTTON_SPELL)
                continue;

            uint32 const action = actionButton.GetAction();
            if (!IsJudgmentSpell(action))
                continue;

            swapped.try_emplace(button, action);
            player->AddActionButton(button, hammerOfWrathSpellId, ACTION_BUTTON_SPELL);
        }

        if (!swapped.empty())
            SyncActionButtons(player);
    }

    static void RestoreActionBarJudgment(Player* player, uint32 hammerOfWrathSpellId)
    {
        ObjectGuid const guid = player->GetGUID();
        auto swappedItr = SwappedActionButtons.find(guid);
        bool changed = false;

        if (swappedItr != SwappedActionButtons.end())
        {
            for (auto const& [button, spellId] : swappedItr->second)
            {
                if (player->AddActionButton(button, spellId, ACTION_BUTTON_SPELL))
                    changed = true;
            }

            SwappedActionButtons.erase(swappedItr);
        }

        // Fallback if tracked swaps were lost (script reload / desync): put Judgment back
        // on bar slots that still hold temporary Hammer of Wrath only.
        if (!changed && !HasPermanentSpell(player, hammerOfWrathSpellId))
        {
            uint32 const restoreSpellId = GetSpecJudgmentSpell(player);
            uint32 const judgmentSpellId = restoreSpellId ? restoreSpellId : GetPrimaryJudgmentSpell(player);
            if (judgmentSpellId)
            {
                for (auto const& [button, actionButton] : player->GetActionButtons())
                {
                    if (actionButton.uState == ACTIONBUTTON_DELETED)
                        continue;

                    if (actionButton.GetType() != ACTION_BUTTON_SPELL)
                        continue;

                    if (actionButton.GetAction() != hammerOfWrathSpellId)
                        continue;

                    if (player->AddActionButton(button, judgmentSpellId, ACTION_BUTTON_SPELL))
                        changed = true;
                }
            }
        }

        if (changed)
            SyncActionButtons(player);
    }

    static void ApplyJudgmentOverride(Player* player, uint32 hammerOfWrathSpellId)
    {
        if (!player || !hammerOfWrathSpellId)
            return;

        ObjectGuid const guid = player->GetGUID();
        if (ActiveJudgmentOverrides.contains(guid))
            return;

        if (!HasPermanentSpell(player, hammerOfWrathSpellId))
            player->AddTemporarySpell(hammerOfWrathSpellId);

        SuppressSpecJudgmentOverride(player);

        std::unordered_set<uint32> superceded;

        auto doSupercede = [&](uint32 judgmentSpellId)
        {
            if (!judgmentSpellId || !superceded.insert(judgmentSpellId).second)
                return;

            player->AddOverrideSpell(judgmentSpellId, hammerOfWrathSpellId);
            player->SendSupercededSpell(judgmentSpellId, hammerOfWrathSpellId);
        };

        // Prefer the Judgment actually placed on the action bar — client swaps by exact spell id.
        for (auto const& [button, actionButton] : player->GetActionButtons())
        {
            if (actionButton.uState == ACTIONBUTTON_DELETED)
                continue;

            if (actionButton.GetType() != ACTION_BUTTON_SPELL)
                continue;

            uint32 const action = actionButton.GetAction();
            if (IsJudgmentSpell(action))
                doSupercede(action);
        }

        if (superceded.empty())
        {
            if (uint32 specJudgment = GetSpecJudgmentSpell(player))
                doSupercede(specJudgment);
            else
                doSupercede(GetPrimaryJudgmentSpell(player));
        }

        SwapActionBarJudgmentToHammerOfWrath(player, hammerOfWrathSpellId);

        // HoW must be usable immediately when it replaces Judgment (shared CD / leftover Judgment CD).
        SpellHistory* history = player->GetSpellHistory();
        history->ResetCooldown(hammerOfWrathSpellId, true);
        if (SpellInfo const* howInfo = sSpellMgr->GetSpellInfo(hammerOfWrathSpellId, DIFFICULTY_NONE))
            if (howInfo->ChargeCategoryId)
                history->RestoreCharge(howInfo->ChargeCategoryId);

        for (uint32 judgmentSpellId : superceded)
            history->ResetCooldown(judgmentSpellId, true);

        SupercededJudgments[guid] = std::move(superceded);
        ActiveJudgmentOverrides.insert(guid);
    }

    static void RemoveJudgmentOverride(Player* player, uint32 hammerOfWrathSpellId)
    {
        if (!player || !hammerOfWrathSpellId)
            return;

        ObjectGuid const guid = player->GetGUID();
        bool const hadActive = ActiveJudgmentOverrides.erase(guid) > 0;
        bool const hadSwaps = SwappedActionButtons.contains(guid);
        bool const hadSupercedes = SupercededJudgments.contains(guid);
        if (!hadActive && !hadSwaps && !hadSupercedes)
            return;

        // Undo client supersede first, then restore the physical action-bar slot.
        // Restoring the button while Judgment→HoW supersede is still active leaves HoW on the bar.
        auto itr = SupercededJudgments.find(guid);
        std::unordered_set<uint32> superceded;
        if (itr != SupercededJudgments.end())
        {
            superceded = std::move(itr->second);
            SupercededJudgments.erase(itr);
        }

        if (superceded.empty())
        {
            if (uint32 specJudgment = GetSpecJudgmentSpell(player))
                superceded.insert(specJudgment);
            else if (uint32 judgmentSpellId = GetPrimaryJudgmentSpell(player))
                superceded.insert(judgmentSpellId);
        }

        for (uint32 judgmentSpellId : superceded)
        {
            player->RemoveOverrideSpell(judgmentSpellId, hammerOfWrathSpellId);
            player->SendSupercededSpell(hammerOfWrathSpellId, judgmentSpellId);
        }

        RestoreActionBarJudgment(player, hammerOfWrathSpellId);

        if (!HasPermanentSpell(player, hammerOfWrathSpellId))
            player->RemoveTemporarySpell(hammerOfWrathSpellId);

        RestoreSpecJudgmentOverride(player);

        // Spec restore may change which Judgment the client shows — refresh bar once more.
        SyncActionButtons(player);
    }

    static void UpdateOverride(Unit* unit, uint32 excludeAuraId = 0)
    {
        if (!unit)
            return;

        Player* player = unit->ToPlayer();
        bool const shouldOverride = HasHammerOfWrathTalent(unit) && HasAvengingWrathAura(unit, excludeAuraId);

        if (shouldOverride)
        {
            if (player)
                ApplyJudgmentOverride(player, SPELL_PALADIN_HAMMER_OF_WRATH);
        }
        else if (player)
        {
            RemoveJudgmentOverride(player, SPELL_PALADIN_HAMMER_OF_WRATH);
        }
    }
}

namespace PaladinRadiantGlory
{
    static constexpr std::array<uint32, 4> AvengingWrathAuraIds =
    {
        SPELL_PALADIN_AVENGING_WRATH,
        231895, // Avenging Wrath (Holy)
        454351, // Avenging Wrath
        454373  // Avenging Wrath
    };

    static constexpr int32 WakeOfAshesDurationMs = 8000;
    static constexpr int32 HolyPowerProcDurationMs = 6000;
    static constexpr double AccumulatorChancePerHolyPower = 0.075;
    static constexpr uint32 ProcInternalCooldownMs = 500;

    static std::unordered_map<ObjectGuid, double> Accumulator;
    static std::unordered_map<ObjectGuid, uint32> LastProcMs;

    static bool HasRadiantGlory(Unit const* unit)
    {
        if (unit->HasAura(SPELL_PALADIN_RADIANT_GLORY) || unit->HasAura(SPELL_PALADIN_RADIANT_GLORY_PASSIVE))
            return true;

        if (Player const* player = unit->ToPlayer())
            return player->HasSpell(SPELL_PALADIN_RADIANT_GLORY) || player->HasSpell(SPELL_PALADIN_RADIANT_GLORY_PASSIVE);

        return false;
    }

    static uint32 GetAvengingWrathSpellId(Player const* player)
    {
        if (!player)
            return SPELL_PALADIN_AVENGING_WRATH;

        if (player->HasSpell(454351))
            return 454351;
        if (player->HasSpell(454373))
            return 454373;
        if (player->HasSpell(231895))
            return 231895;

        return SPELL_PALADIN_AVENGING_WRATH;
    }

    // Radiant Glory base (WoA 8s / HP proc 6s) + duration mods such as Divine Wrath (406872: +3s/+4s).
    static int32 CalcAvengingWrathDurationMs(Unit* unit, uint32 awSpellId, int32 baseDurationMs)
    {
        int32 durationMs = baseDurationMs;
        if (Player* player = unit->ToPlayer())
            if (SpellInfo const* awSpellInfo = sSpellMgr->GetSpellInfo(awSpellId, DIFFICULTY_NONE))
                player->ApplySpellMod(awSpellInfo, SpellModOp::Duration, durationMs);

        return std::max(durationMs, 0);
    }

    static void TriggerOrExtendAvengingWrath(Unit* unit, Spell const* triggeringSpell, int32 baseDurationMs)
    {
        if (!unit || !baseDurationMs || !HasRadiantGlory(unit))
            return;

        uint32 awSpellId = GetAvengingWrathSpellId(unit->ToPlayer());
        int32 const durationMs = CalcAvengingWrathDurationMs(unit, awSpellId, baseDurationMs);
        if (!durationMs)
            return;

        for (uint32 auraId : AvengingWrathAuraIds)
        {
            if (Aura* activeWrath = unit->GetAura(auraId))
            {
                activeWrath->SetDuration(activeWrath->GetDuration() + durationMs);
                activeWrath->SetMaxDuration(activeWrath->GetMaxDuration() + durationMs);
                PaladinHammerOfWrath::UpdateOverride(unit);
                return;
            }
        }

        unit->CastSpell(unit, awSpellId, CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_IGNORE_SPELL_AND_CATEGORY_CD)
            .SetTriggeringSpell(triggeringSpell)
            .AddSpellMod(SPELLVALUE_DURATION, durationMs));

        PaladinHammerOfWrath::UpdateOverride(unit);
    }

    static void TriggerWakeOfAshesAvengingWrath(Unit* unit, Spell const* triggeringSpell)
    {
        TriggerOrExtendAvengingWrath(unit, triggeringSpell, WakeOfAshesDurationMs);
    }

    static void TryProcFromHolyPowerSpend(Player* player, Spell const* triggeringSpell, int32 holyPowerSpent)
    {
        if (!player || !triggeringSpell || holyPowerSpent <= 0 || !HasRadiantGlory(player))
            return;

        ObjectGuid const guid = player->GetGUID();
        uint32 const now = getMSTime();

        if (auto itr = LastProcMs.find(guid); itr != LastProcMs.end() && getMSTimeDiff(itr->second, now) < ProcInternalCooldownMs)
            return;

        double& accumulator = Accumulator[guid];
        accumulator += frand(0.0f, static_cast<float>(AccumulatorChancePerHolyPower * holyPowerSpent));
        if (accumulator < 1.0)
            return;

        accumulator -= 1.0;
        LastProcMs[guid] = now;

        TriggerOrExtendAvengingWrath(player, triggeringSpell, HolyPowerProcDurationMs);
    }
}

namespace PaladinDawnlight
{
    static bool HasDawnlightTalent(Unit const* unit)
    {
        if (unit->HasAura(SPELL_PALADIN_DAWNLIGHT))
            return true;

        if (Player const* player = unit->ToPlayer())
            return player->HasSpell(SPELL_PALADIN_DAWNLIGHT);

        return false;
    }

    static int32 GetChargeCount(Unit const* unit)
    {
        if (AuraEffect const* aurEff = unit->GetAuraEffect(SPELL_PALADIN_DAWNLIGHT, EFFECT_0))
            return std::max(aurEff->GetAmountAsInt(), 1);

        return 2;
    }

    static void GrantCharges(Unit* unit, Spell const* triggeringSpell = nullptr)
    {
        if (!unit || !HasDawnlightTalent(unit))
            return;

        int32 const stacks = GetChargeCount(unit);
        if (Aura* existing = unit->GetAura(SPELL_PALADIN_DAWNLIGHT_CHARGES))
        {
            existing->SetStackAmount(stacks);
            existing->RefreshDuration();
            return;
        }

        unit->CastSpell(unit, SPELL_PALADIN_DAWNLIGHT_CHARGES, CastSpellExtraArgs(TRIGGERED_FULL_MASK)
            .SetTriggeringSpell(triggeringSpell)
            .AddSpellMod(SPELLVALUE_AURA_STACK, stacks));
    }

    static void ApplyDawnlight(Unit* caster, Unit* target, Spell const* triggeringSpell)
    {
        if (!caster || !target)
            return;

        uint32 const dawnlightSpellId = target->IsFriendlyTo(caster)
            ? SPELL_PALADIN_DAWNLIGHT_HEAL
            : SPELL_PALADIN_DAWNLIGHT_DAMAGE;

        caster->CastSpell(target, dawnlightSpellId, CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
            .SetTriggeringSpell(triggeringSpell));
    }
}

// 431377 - Dawnlight (talent): DBC has broad proc flags but only dummy effects.
class spell_pal_dawnlight : public AuraScript
{
    bool CheckProc(ProcEventInfo& /*eventInfo*/)
    {
        return false;
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_pal_dawnlight::CheckProc);
    }
};

// 431460 - Dawnlight empower helper: must NOT grant charges from every hit.
class spell_pal_dawnlight_empower : public AuraScript
{
    bool CheckProc(ProcEventInfo& /*eventInfo*/)
    {
        return false;
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_pal_dawnlight_empower::CheckProc);
    }
};

// 431581 - Dawnlight radiate metadata (no automatic procs).
class spell_pal_dawnlight_radiate_meta : public AuraScript
{
    bool CheckProc(ProcEventInfo& /*eventInfo*/)
    {
        return false;
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_pal_dawnlight_radiate_meta::CheckProc);
    }
};

// 431522 - Dawnlight charges: next Holy Power spenders apply Dawnlight.
class spell_pal_dawnlight_charges : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_DAWNLIGHT_DAMAGE, SPELL_PALADIN_DAWNLIGHT_HEAL });
    }

    bool CheckProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        Spell const* procSpell = eventInfo.GetProcSpell();
        if (!procSpell || !procSpell->HasPowerTypeCost(POWER_HOLY_POWER))
            return false;

        return eventInfo.GetActionTarget() != nullptr;
    }

    void HandleProc(AuraEffect* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        Unit* caster = GetTarget();
        Unit* target = eventInfo.GetActionTarget();
        if (!caster || !target)
            return;

        PaladinDawnlight::ApplyDawnlight(caster, target, eventInfo.GetProcSpell());

        if (GetStackAmount() <= 1)
            Remove();
        else
            ModStackAmount(-1);
    }

    void Register() override
    {
        DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_dawnlight_charges::CheckProc, EFFECT_0, SPELL_AURA_DUMMY);
        OnEffectProc += AuraEffectProcFn(spell_pal_dawnlight_charges::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// 431380 - Dawnlight (damage): radiate a portion of each tick to nearby enemies.
class spell_pal_dawnlight_damage : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_DAWNLIGHT_RADIATE_DAMAGE, SPELL_PALADIN_DAWNLIGHT_RADIATE_META });
    }

    void HandlePeriodic(AuraEffect const* aurEff)
    {
        Unit* caster = GetCaster();
        Unit* target = GetTarget();
        if (!caster || !target)
            return;

        AuraEffect const* radiateEff = caster->GetAuraEffect(SPELL_PALADIN_DAWNLIGHT_RADIATE_META, EFFECT_0);
        if (!radiateEff)
            return;

        int32 const radiatePct = radiateEff->GetAmountAsInt();
        if (radiatePct <= 0)
            return;

        int32 const radiateDamage = CalculatePct(aurEff->GetAmountAsInt(), radiatePct);
        if (radiateDamage <= 0)
            return;

        caster->CastSpell(target, SPELL_PALADIN_DAWNLIGHT_RADIATE_DAMAGE, CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
            .AddSpellMod(SPELLVALUE_BASE_POINT0, radiateDamage));
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_pal_dawnlight_damage::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
    }
};

class spell_pal_dawnlight_cast_trigger : public PlayerScript
{
public:
    spell_pal_dawnlight_cast_trigger() : PlayerScript("spell_pal_dawnlight_cast_trigger") { }

    void OnSpellCast(Player* player, Spell* spell, bool /*skipCheck*/) override
    {
        if (!player || !spell)
            return;

        switch (spell->GetSpellInfo()->Id)
        {
            case SPELL_PALADIN_WAKE_OF_ASHES:
            case SPELL_PALADIN_HOLY_PRISM:
            case SPELL_PALADIN_DIVINE_TOLL:
                PaladinDawnlight::GrantCharges(player, spell);
                break;
            default:
                break;
        }
    }
};

// 462048 - Radiant Glory (passive proc logic)
class spell_pal_radiant_glory : public PlayerScript
{
public:
    spell_pal_radiant_glory() : PlayerScript("spell_pal_radiant_glory") { }

    void OnSpellCast(Player* player, Spell* spell, bool /*skipCheck*/) override
    {
        if (!player || !spell || !PaladinRadiantGlory::HasRadiantGlory(player))
            return;

        if (!spell->HasPowerTypeCost(POWER_HOLY_POWER))
            return;

        SpellInfo const* spellInfo = spell->GetSpellInfo();
        if (!spellInfo)
            return;

        Optional<SpellPowerCost> holyPowerCost = spellInfo->CalcPowerCost(POWER_HOLY_POWER, false, player, spellInfo->GetSchoolMask(), spell);
        if (!holyPowerCost || holyPowerCost->Amount <= 0)
            return;

        PaladinRadiantGlory::TryProcFromHolyPowerSpend(player, spell, holyPowerCost->Amount);
    }
};

// 1241288 - Hammer of Wrath (talent)
class spell_pal_hammer_of_wrath_talent : public AuraScript
{
    bool CheckProc(AuraEffect const* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
    {
        return false;
    }

    void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        PaladinHammerOfWrath::UpdateOverride(GetTarget());
    }

    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        PaladinHammerOfWrath::UpdateOverride(GetTarget());
    }

    void Register() override
    {
        DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_hammer_of_wrath_talent::CheckProc, EFFECT_0, SPELL_AURA_DUMMY);
        AfterEffectApply += AuraEffectApplyFn(spell_pal_hammer_of_wrath_talent::HandleApply, EFFECT_0, SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(spell_pal_hammer_of_wrath_talent::HandleRemove, EFFECT_0, SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
    }
};

// Avenging Wrath - apply/remove Hammer of Wrath override when talent is known
class spell_pal_hammer_of_wrath_avenging_wrath : public AuraScript
{
    void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        PaladinHammerOfWrath::UpdateOverride(GetTarget());
    }

    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        // Ignore this AW aura while removing — HasAura can still see it mid-unapply on some paths.
        PaladinHammerOfWrath::UpdateOverride(GetTarget(), GetId());
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(spell_pal_hammer_of_wrath_avenging_wrath::HandleApply, EFFECT_FIRST_FOUND, SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(spell_pal_hammer_of_wrath_avenging_wrath::HandleRemove, EFFECT_FIRST_FOUND, SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
    }
};

// 383344 - Expurgation
class spell_pal_expurgation : public AuraScript
{
    static bool IsBladeOfJusticeSpell(uint32 spellId)
    {
        switch (spellId)
        {
            case SPELL_PALADIN_BLADE_OF_JUSTICE: // 184575
            case SPELL_PALADIN_BLADE_OF_JUSTICE_AOE: // 404358
                return true;
            default:
                return false;
        }
    }

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_EXPURGATION_DOT, SPELL_PALADIN_BLADE_OF_JUSTICE });
    }

    bool CheckProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        SpellInfo const* spellInfo = eventInfo.GetSpellInfo();
        return spellInfo && IsBladeOfJusticeSpell(spellInfo->Id);
    }

    void Register() override
    {
        DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_expurgation::CheckProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// 156322 - Eternal Flame
class spell_pal_eternal_flame : public SpellScript
{
    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellEffect({ { spellInfo->Id, EFFECT_2 } });
    }

    void CalculateHealing(SpellEffectInfo const& /*effectInfo*/, Unit const* victim, int32& /*healing*/, int32& /*flatMod*/, float& pctMod) const
    {
        Unit* caster = GetCaster();
        if (victim == caster)
            AddPct(pctMod, GetEffectInfo(EFFECT_2).CalcValue(caster));
    }

    void Register() override
    {
        CalcHealing += SpellCalcHealingFn(spell_pal_eternal_flame::CalculateHealing);
    }
};

class spell_pal_eternal_flame_aura : public AuraScript
{
    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellEffect({ { spellInfo->Id, EFFECT_2 } });
    }

    void CalculateHealing(AuraEffect const* /*aurEff*/, Unit const* victim, int32& /*healing*/, int32& /*flatMod*/, float& pctMod) const
    {
        Unit* caster = GetCaster();
        if (victim == caster)
            AddPct(pctMod, GetEffectInfo(EFFECT_2).CalcValue(caster));
    }

    void Register() override
    {
        DoEffectCalcDamageAndHealing += AuraEffectCalcHealingFn(spell_pal_eternal_flame_aura::CalculateHealing, EFFECT_0, SPELL_AURA_PERIODIC_HEAL);
    }
};

// 343527 - Execution Sentence
class spell_pal_execution_sentence : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
        {
            SPELL_PALADIN_EXECUTION_SENTENCE_DAMAGE,
            SPELL_PALADIN_EXECUTIONERS_WILL,
            SPELL_PALADIN_EXECUTION_SENTENCE_11_SECONDS,
            SPELL_PALADIN_EXECUTION_SENTENCE_8_SECONDS
        });
    }

    void HandleVisual(SpellEffIndex /*effIndex*/) const
    {
        uint32 visualSpellId = GetCaster()->HasAura(SPELL_PALADIN_EXECUTIONERS_WILL)
            ? SPELL_PALADIN_EXECUTION_SENTENCE_11_SECONDS
            : SPELL_PALADIN_EXECUTION_SENTENCE_8_SECONDS;
        GetCaster()->CastSpell(GetHitUnit(), visualSpellId,
            CastSpellExtraArgsInit{
                .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
                .TriggeringSpell = GetSpell()
            });
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_pal_execution_sentence::HandleVisual, EFFECT_0, SPELL_EFFECT_APPLY_AURA);
    }
};

class spell_pal_execution_sentence_aura : public AuraScript
{
    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellEffect({ { spellInfo->Id, EFFECT_1 } })
            && spellInfo->GetEffect(EFFECT_1).IsAura();
    }

    void HandleProc(AuraEffect* aurEff, ProcEventInfo const& eventInfo) const
    {
        if (DamageInfo const* damageInfo = eventInfo.GetDamageInfo())
            aurEff->ChangeAmount(aurEff->GetAmount() + CalculatePct(damageInfo->GetDamage(), GetEffect(EFFECT_1)->GetAmount()));
    }

    void AfterRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/) const
    {
        SpellEffectValue amount = aurEff->GetAmount();
        if (!amount || GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_EXPIRE)
            return;

        if (Unit* caster = GetCaster())
            caster->CastSpell(GetTarget(), SPELL_PALADIN_EXECUTION_SENTENCE_DAMAGE, CastSpellExtraArgsInit{
                .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
                .TriggeringAura = aurEff,
                .SpellValueOverrides = { { SPELLVALUE_BASE_POINT0, amount } }
            });
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_pal_execution_sentence_aura::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        AfterEffectRemove += AuraEffectRemoveFn(spell_pal_execution_sentence_aura::AfterRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// 205191 - Eye for an Eye
class spell_pal_eye_for_an_eye : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_EYE_FOR_AN_EYE_TRIGGERED });
    }

    void HandleEffectProc(AuraEffect* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        GetTarget()->CastSpell(eventInfo.GetActor(), SPELL_PALADIN_EYE_FOR_AN_EYE_TRIGGERED, true);
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_pal_eye_for_an_eye::HandleEffectProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// 383328 - Final Verdict
class spell_pal_final_verdict : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_FINAL_VERDICT });
    }

    void HandleDummy(SpellEffIndex /*effIndex*/) const
    {
        if (!roll_chance(GetEffectValue()))
            return;

        Unit* caster = GetCaster();
        caster->CastSpell(caster, SPELL_PALADIN_FINAL_VERDICT, CastSpellExtraArgsInit{
            .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
            .TriggeringSpell = GetSpell()
        });
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_pal_final_verdict::HandleDummy, EFFECT_1, SPELL_EFFECT_DUMMY);
    }
};

// 234299 - Fist of Justice
class spell_pal_fist_of_justice : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_HAMMER_OF_JUSTICE });
    }

    bool CheckEffectProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        if (Spell const* procSpell = eventInfo.GetProcSpell())
            return procSpell->HasPowerTypeCost(POWER_HOLY_POWER);

        return false;
    }

    void HandleEffectProc(AuraEffect* aurEff, ProcEventInfo& /*procInfo*/)
    {
        int32 value = aurEff->GetAmount() / 10;

        GetTarget()->GetSpellHistory()->ModifyCooldown(SPELL_PALADIN_HAMMER_OF_JUSTICE, Seconds(-value));
    }

    void Register() override
    {
        DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_fist_of_justice::CheckEffectProc, EFFECT_0, SPELL_AURA_DUMMY);
        OnEffectProc += AuraEffectProcFn(spell_pal_fist_of_justice::HandleEffectProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// -85043 - Grand Crusader
class spell_pal_grand_crusader : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_AVENGERS_SHIELD });
    }

    bool CheckProc(ProcEventInfo& /*eventInfo*/)
    {
        return GetTarget()->GetTypeId() == TYPEID_PLAYER;
    }

    void HandleEffectProc(AuraEffect* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
    {
        GetTarget()->GetSpellHistory()->ResetCooldown(SPELL_PALADIN_AVENGERS_SHIELD, true);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_pal_grand_crusader::CheckProc);
        OnEffectProc += AuraEffectProcFn(spell_pal_grand_crusader::HandleEffectProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

// 54968 - Glyph of Holy Light
class spell_pal_glyph_of_holy_light : public SpellScript
{
    void FilterTargets(std::list<WorldObject*>& targets)
    {
        uint32 const maxTargets = GetSpellInfo()->MaxAffectedTargets;

        if (targets.size() > maxTargets)
        {
            targets.sort(Trinity::Predicates::HealthPctOrderPred());
            targets.resize(maxTargets);
        }
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pal_glyph_of_holy_light::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ALLY);
    }
};

// 53595 - Hammer of the Righteous
struct spell_pal_hammer_of_the_righteous : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo
        ({
            SPELL_PALADIN_CONSECRATION_PROTECTION_AURA,
            SPELL_PALADIN_HAMMER_OF_THE_RIGHTEOUS_AOE
        });
    }

    void HandleAoEHit(SpellEffIndex /*effIndex*/)
    {
        if (GetCaster()->HasAura(SPELL_PALADIN_CONSECRATION_PROTECTION_AURA))
            GetCaster()->CastSpell(GetHitUnit(), SPELL_PALADIN_HAMMER_OF_THE_RIGHTEOUS_AOE);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_pal_hammer_of_the_righteous::HandleAoEHit, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

// 6940 - Hand of Sacrifice
class spell_pal_hand_of_sacrifice : public AuraScript
{
public:
    spell_pal_hand_of_sacrifice()
    {
        remainingAmount = 0;
    }

private:
    int32 remainingAmount;

    bool Load() override
    {
        if (Unit* caster = GetCaster())
        {
            remainingAmount = caster->GetMaxHealth();
            return true;
        }
        return false;
    }

    void Split(AuraEffect* /*aurEff*/, DamageInfo & /*dmgInfo*/, uint32 & splitAmount)
    {
        remainingAmount -= splitAmount;

        if (remainingAmount <= 0)
        {
            GetTarget()->RemoveAura(SPELL_PALADIN_HAND_OF_SACRIFICE);
        }
    }

    void Register() override
    {
        OnEffectSplit += AuraEffectSplitFn(spell_pal_hand_of_sacrifice::Split, EFFECT_0);
    }
};

// 54149 - Infusion of Light
class spell_pal_infusion_of_light : public AuraScript
{
    static constexpr flag128 HolyLightSpellClassMask = { 0, 0, 0x400 };

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_INFUSION_OF_LIGHT_ENERGIZE });
    }

    bool CheckFlashOfLightProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        return eventInfo.GetProcSpell() && eventInfo.GetProcSpell()->m_appliedMods.find(GetAura()) != eventInfo.GetProcSpell()->m_appliedMods.end();
    }

    bool CheckHolyLightProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        return eventInfo.GetSpellInfo() && eventInfo.GetSpellInfo()->IsAffected(SPELLFAMILY_PALADIN, HolyLightSpellClassMask);
    }

    void HandleProc(AuraEffect* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        eventInfo.GetActor()->CastSpell(eventInfo.GetActor(), SPELL_PALADIN_INFUSION_OF_LIGHT_ENERGIZE,
            CastSpellExtraArgs(TRIGGERED_FULL_MASK).SetTriggeringSpell(eventInfo.GetProcSpell()));
    }

    void Register() override
    {
        DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_infusion_of_light::CheckFlashOfLightProc, EFFECT_0, SPELL_AURA_ADD_PCT_MODIFIER);
        DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_infusion_of_light::CheckFlashOfLightProc, EFFECT_2, SPELL_AURA_ADD_FLAT_MODIFIER);

        DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_infusion_of_light::CheckHolyLightProc, EFFECT_1, SPELL_AURA_DUMMY);
        OnEffectProc += AuraEffectProcFn(spell_pal_infusion_of_light::HandleProc, EFFECT_1, SPELL_AURA_DUMMY);
    }
};

// 327193 - Moment of Glory
class spell_pal_moment_of_glory : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_AVENGERS_SHIELD });
    }

    void HandleOnHit()
    {
        GetCaster()->GetSpellHistory()->ResetCooldown(SPELL_PALADIN_AVENGERS_SHIELD);
    }

    void Register() override
    {
        OnHit += SpellHitFn(spell_pal_moment_of_glory::HandleOnHit);
    }
};

// 20271/275779/275773 - Judgement (Retribution/Protection/Holy)
class spell_pal_judgment : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo
        ({
            SPELL_PALADIN_JUDGMENT_RANK_3,
            SPELL_PALADIN_JUDGMENT_GAIN_HOLY_POWER
        });
    }

    bool Load() override
    {
        return GetCaster()->HasSpell(SPELL_PALADIN_JUDGMENT_RANK_3);
    }

    void HandleOnCast() const
    {
        Unit* caster = GetCaster();
        caster->CastSpell(caster, SPELL_PALADIN_JUDGMENT_GAIN_HOLY_POWER, CastSpellExtraArgsInit{
            .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
            .TriggeringSpell = GetSpell()
        });
    }

    void Register() override
    {
        OnCast += SpellCastFn(spell_pal_judgment::HandleOnCast);
    }
};

// 215661 - Justicar's Vengeance
class spell_pal_justicars_vengeance : public SpellScript
{
    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellEffect({ { spellInfo->Id, EFFECT_1 } });
    }

    void HandleDamage(SpellEffectInfo const& /*spellEffectInfo*/, Unit const* victim, int32& /*damage*/, int32& /*flatMod*/, float& pctMod) const
    {
        if (victim->HasUnitState(UNIT_STATE_STUNNED))
            AddPct(pctMod, GetEffectInfo(EFFECT_1).CalcValue(GetCaster()));
    }

    void Register() override
    {
        CalcDamage += SpellCalcDamageFn(spell_pal_justicars_vengeance::HandleDamage);
    }
};

// 114165 - Holy Prism
class spell_pal_holy_prism : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo ({ SPELL_PALADIN_HOLY_PRISM_TARGET_ALLY, SPELL_PALADIN_HOLY_PRISM_TARGET_ENEMY, SPELL_PALADIN_HOLY_PRISM_TARGET_BEAM_VISUAL });
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        if (GetCaster()->IsFriendlyTo(GetHitUnit()))
            GetCaster()->CastSpell(GetHitUnit(), SPELL_PALADIN_HOLY_PRISM_TARGET_ALLY, true);
        else
            GetCaster()->CastSpell(GetHitUnit(), SPELL_PALADIN_HOLY_PRISM_TARGET_ENEMY , true);

        GetCaster()->CastSpell(GetHitUnit(), SPELL_PALADIN_HOLY_PRISM_TARGET_BEAM_VISUAL, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_pal_holy_prism::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 114852 - Holy Prism (Damage)
// 114871 - Holy Prism (Heal)
class spell_pal_holy_prism_selector : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo ({ SPELL_PALADIN_HOLY_PRISM_TARGET_ALLY, SPELL_PALADIN_HOLY_PRISM_AREA_BEAM_VISUAL });
    }

    void SaveTargetGuid(SpellEffIndex /*effIndex*/)
    {
        _targetGUID = GetHitUnit()->GetGUID();
    }

    void FilterTargets(std::list<WorldObject*>& targets)
    {
        uint8 const maxTargets = 5;

        if (targets.size() > maxTargets)
        {
            if (GetSpellInfo()->Id == SPELL_PALADIN_HOLY_PRISM_TARGET_ALLY)
            {
                targets.sort(Trinity::Predicates::HealthPctOrderPred());
                targets.resize(maxTargets);
            }
            else
                Trinity::Containers::RandomResize(targets, maxTargets);
        }

        _sharedTargets = targets;
    }

    void ShareTargets(std::list<WorldObject*>& targets)
    {
        targets = _sharedTargets;
    }

    void HandleScript(SpellEffIndex /*effIndex*/)
    {
        if (Unit* initialTarget = ObjectAccessor::GetUnit(*GetCaster(), _targetGUID))
            initialTarget->CastSpell(GetHitUnit(), SPELL_PALADIN_HOLY_PRISM_AREA_BEAM_VISUAL, true);
    }

    void Register() override
    {
        if (m_scriptSpellId == SPELL_PALADIN_HOLY_PRISM_TARGET_ENEMY)
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pal_holy_prism_selector::FilterTargets, EFFECT_1, TARGET_UNIT_DEST_AREA_ALLY);
        else if (m_scriptSpellId == SPELL_PALADIN_HOLY_PRISM_TARGET_ALLY)
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pal_holy_prism_selector::FilterTargets, EFFECT_1, TARGET_UNIT_DEST_AREA_ENEMY);

        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pal_holy_prism_selector::ShareTargets, EFFECT_2, TARGET_UNIT_DEST_AREA_ENTRY);

        OnEffectHitTarget += SpellEffectFn(spell_pal_holy_prism_selector::SaveTargetGuid, EFFECT_0, SPELL_EFFECT_ANY);
        OnEffectHitTarget += SpellEffectFn(spell_pal_holy_prism_selector::HandleScript, EFFECT_2, SPELL_EFFECT_SCRIPT_EFFECT);
    }

private:
    std::list<WorldObject*> _sharedTargets;
    ObjectGuid _targetGUID;
};

// 20473 - Holy Shock
class spell_pal_holy_shock : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo
        ({
            SPELL_PALADIN_HOLY_SHOCK,
            SPELL_PALADIN_HOLY_SHOCK_HEALING,
            SPELL_PALADIN_HOLY_SHOCK_DAMAGE
        });
    }

    SpellCastResult CheckCast()
    {
        Unit* caster = GetCaster();

        if (Unit* target = GetExplTargetUnit())
        {
            if (!caster->IsFriendlyTo(target))
            {
                if (!caster->IsValidAttackTarget(target))
                    return SPELL_FAILED_BAD_TARGETS;

                if (!caster->isInFront(target))
                    return SPELL_FAILED_UNIT_NOT_INFRONT;
            }
        }
        else
            return SPELL_FAILED_BAD_TARGETS;

        return SPELL_CAST_OK;
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();

        if (Unit* unitTarget = GetHitUnit())
        {
            uint32 const shockId = caster->IsFriendlyTo(unitTarget)
                ? SPELL_PALADIN_HOLY_SHOCK_HEALING
                : SPELL_PALADIN_HOLY_SHOCK_DAMAGE;

            // Forward CustomArg so Second Sunrise effectiveness reaches damage/heal spells.
            caster->CastSpell(unitTarget, shockId, CastSpellExtraArgsInit{
                .TriggerFlags = TRIGGERED_FULL_MASK,
                .TriggeringSpell = GetSpell(),
                .CustomArg = GetSpell()->m_customArg
            });
        }
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_pal_holy_shock::CheckCast);
        OnEffectHitTarget += SpellEffectFn(spell_pal_holy_shock::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 25912 - Holy Shock
class spell_pal_holy_shock_damage_visual : public SpellScript
{
    bool Validate(SpellInfo const*) override
    {
        return sSpellVisualStore.HasRecord(PALADIN_VISUAL_SPELL_HOLY_SHOCK_DAMAGE)
            && sSpellVisualStore.HasRecord(PALADIN_VISUAL_SPELL_HOLY_SHOCK_DAMAGE_CRIT);
    }

    void PlayVisual()
    {
        GetCaster()->SendPlaySpellVisual(GetHitUnit(), IsHitCrit() ? PALADIN_VISUAL_SPELL_HOLY_SHOCK_DAMAGE_CRIT : PALADIN_VISUAL_SPELL_HOLY_SHOCK_DAMAGE, 0, 0, 0.0f, false);
    }

    void Register() override
    {
        AfterHit += SpellHitFn(spell_pal_holy_shock_damage_visual::PlayVisual);
    }
};

// 25914 - Holy Shock
class spell_pal_holy_shock_heal_visual : public SpellScript
{
    bool Validate(SpellInfo const*) override
    {
        return sSpellVisualStore.HasRecord(PALADIN_VISUAL_SPELL_HOLY_SHOCK_HEAL)
            && sSpellVisualStore.HasRecord(PALADIN_VISUAL_SPELL_HOLY_SHOCK_HEAL_CRIT);
    }

    void PlayVisual()
    {
        GetCaster()->SendPlaySpellVisual(GetHitUnit(), IsHitCrit() ? PALADIN_VISUAL_SPELL_HOLY_SHOCK_HEAL_CRIT : PALADIN_VISUAL_SPELL_HOLY_SHOCK_HEAL, 0, 0, 0.0f, false);
    }

    void Register() override
    {
        AfterHit += SpellHitFn(spell_pal_holy_shock_heal_visual::PlayVisual);
    }
};

// 37705 - Healing Discount
class spell_pal_item_healing_discount : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_ITEM_HEALING_TRANCE });
    }

    void HandleProc(AuraEffect* aurEff, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();
        GetTarget()->CastSpell(GetTarget(), SPELL_PALADIN_ITEM_HEALING_TRANCE, aurEff);
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_pal_item_healing_discount::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

// 40470 - Paladin Tier 6 Trinket
class spell_pal_item_t6_trinket : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
        {
            SPELL_PALADIN_ENDURING_LIGHT,
            SPELL_PALADIN_ENDURING_JUDGEMENT
        });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        SpellInfo const* spellInfo = eventInfo.GetSpellInfo();
        if (!spellInfo)
            return false;

        // Holy Light & Flash of Light
        if (spellInfo->SpellFamilyFlags[0] & 0xC0000000)
        {
            if (!roll_chance(15))
                return false;

            _triggeredSpellId = SPELL_PALADIN_ENDURING_LIGHT;
            return true;
        }
        // Judgements
        else if (spellInfo->SpellFamilyFlags[0] & 0x00800000)
        {
            if (!roll_chance(50))
                return false;

            _triggeredSpellId = SPELL_PALADIN_ENDURING_JUDGEMENT;
            return true;
        }

        return false;
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        eventInfo.GetActor()->CastSpell(eventInfo.GetActionTarget(), _triggeredSpellId, aurEff);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_pal_item_t6_trinket::CheckProc);
        OnEffectProc += AuraEffectProcFn(spell_pal_item_t6_trinket::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }

    uint32 _triggeredSpellId = 0;
};

// 633 - Lay on Hands
// 471195 - Lay on Hands (from 387791 - Empyreal Ward)
class spell_pal_lay_on_hands : public SpellScript
{
    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_FORBEARANCE })
            && spellInfo->ExcludeTargetAuraSpell == SPELL_PALADIN_IMMUNE_SHIELD_MARKER;
    }

    void TriggerForbearance() const
    {
        GetCaster()->CastSpell(GetHitUnit(), SPELL_PALADIN_FORBEARANCE, CastSpellExtraArgsInit{
            .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
            .TriggeringSpell = GetSpell()
        });
    }

    void Register() override
    {
        AfterHit += SpellHitFn(spell_pal_lay_on_hands::TriggerForbearance);
    }
};

// 53651 - Light's Beacon - Beacon of Light
class spell_pal_light_s_beacon : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_BEACON_OF_LIGHT, SPELL_PALADIN_BEACON_OF_LIGHT_HEAL });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        if (!eventInfo.GetActionTarget())
            return false;
        if (eventInfo.GetActionTarget()->HasAura(SPELL_PALADIN_BEACON_OF_LIGHT, eventInfo.GetActor()->GetGUID()))
            return false;
        HealInfo* healInfo = eventInfo.GetHealInfo();
        return healInfo && healInfo->GetHeal();
    }

    void HandleProc(AuraEffect* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        SpellEffectValue heal = CalculatePct(eventInfo.GetHealInfo()->GetHeal(), aurEff->GetAmount());

        Unit::AuraList const& auras = GetCaster()->GetSingleCastAuras();
        for (Unit::AuraList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
        {
            if ((*itr)->GetId() == SPELL_PALADIN_BEACON_OF_LIGHT)
            {
                std::vector<AuraApplication*> applications;
                (*itr)->GetApplicationVector(applications);
                if (!applications.empty())
                {
                    CastSpellExtraArgs args(aurEff);
                    args.AddSpellMod(SPELLVALUE_BASE_POINT0, heal);
                    eventInfo.GetActor()->CastSpell(applications.front()->GetTarget(), SPELL_PALADIN_BEACON_OF_LIGHT_HEAL, args);
                }
                return;
            }
        }
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_pal_light_s_beacon::CheckProc);
        OnEffectProc += AuraEffectProcFn(spell_pal_light_s_beacon::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// 122773 - Light's Hammer
class spell_pal_light_hammer_init_summon : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo
        ({
            SPELL_PALADIN_LIGHT_HAMMER_COSMETIC,
            SPELL_PALADIN_LIGHT_HAMMER_PERIODIC
        });
    }

    void InitSummon()
    {
         for (SpellLogEffectGenericVictimParams const& summonedObject : GetSpell()->GetExecuteLogEffectTargets(SPELL_EFFECT_SUMMON, &SpellLogEffect::GenericVictimTargets))
         {
             if (Unit* hammer = ObjectAccessor::GetUnit(*GetCaster(), summonedObject.Victim))
             {
                 hammer->CastSpell(hammer, SPELL_PALADIN_LIGHT_HAMMER_COSMETIC,
                     CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS).SetTriggeringSpell(GetSpell()));
                 hammer->CastSpell(hammer, SPELL_PALADIN_LIGHT_HAMMER_PERIODIC,
                     CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS).SetTriggeringSpell(GetSpell()));
             }
         }
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_pal_light_hammer_init_summon::InitSummon);
    }
};

// 114918 - Light's Hammer (Periodic)
class spell_pal_light_hammer_periodic : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo
        ({
            SPELL_PALADIN_LIGHT_HAMMER_HEALING,
            SPELL_PALADIN_LIGHT_HAMMER_DAMAGE
        });
    }

    void HandleEffectPeriodic(AuraEffect const* /*aurEff*/)
    {
        Unit* lightHammer = GetTarget();
        if (Unit* originalCaster = lightHammer->GetOwner())
        {
            originalCaster->CastSpell(lightHammer->GetPosition(), SPELL_PALADIN_LIGHT_HAMMER_DAMAGE, TRIGGERED_IGNORE_CAST_IN_PROGRESS);
            originalCaster->CastSpell(lightHammer->GetPosition(), SPELL_PALADIN_LIGHT_HAMMER_HEALING, TRIGGERED_IGNORE_CAST_IN_PROGRESS);
        }
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_pal_light_hammer_periodic::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }
};

// 204074 - Righteous Protector
class spell_pal_righteous_protector : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_AVENGING_WRATH, SPELL_PALADIN_GUARDIAN_OF_ANCIENT_KINGS });
    }

    bool CheckEffectProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        if (SpellInfo const* procSpell = eventInfo.GetSpellInfo())
            _baseHolyPowerCost = procSpell->CalcPowerCost(POWER_HOLY_POWER, false, eventInfo.GetActor(), eventInfo.GetSchoolMask());
        else
            _baseHolyPowerCost.reset();

        return _baseHolyPowerCost.has_value();
    }

    void HandleEffectProc(AuraEffect* aurEff, ProcEventInfo& /*eventInfo*/)
    {
        int32 value = aurEff->GetAmountAsInt() * 100 * _baseHolyPowerCost->Amount;

        GetTarget()->GetSpellHistory()->ModifyCooldown(SPELL_PALADIN_AVENGING_WRATH, Milliseconds(-value));
        GetTarget()->GetSpellHistory()->ModifyCooldown(SPELL_PALADIN_GUARDIAN_OF_ANCIENT_KINGS, Milliseconds(-value));
    }

    void Register() override
    {
        DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_righteous_protector::CheckEffectProc, EFFECT_0, SPELL_AURA_DUMMY);
        OnEffectProc += AuraEffectProcFn(spell_pal_righteous_protector::HandleEffectProc, EFFECT_0, SPELL_AURA_DUMMY);
    }

    Optional<SpellPowerCost> _baseHolyPowerCost;
};

// 267610 - Righteous Verdict
class spell_pal_righteous_verdict : public AuraScript
{
    bool Validate(SpellInfo const* /*spellEntry*/) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_RIGHTEOUS_VERDICT_AURA });
    }

    void HandleEffectProc(AuraEffect* /*aurEff*/, ProcEventInfo& procInfo)
    {
        procInfo.GetActor()->CastSpell(procInfo.GetActor(), SPELL_PALADIN_RIGHTEOUS_VERDICT_AURA, true);
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_pal_righteous_verdict::HandleEffectProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// 85804 - Selfless Healer
class spell_pal_selfless_healer : public AuraScript
{
    bool CheckEffectProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        if (Spell const* procSpell = eventInfo.GetProcSpell())
            return procSpell->HasPowerTypeCost(POWER_HOLY_POWER);

        return false;
    }

    void Register() override
    {
        DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_selfless_healer::CheckEffectProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

// 53600 - Shield of the Righteous
class spell_pal_shield_of_the_righteous : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_SHIELD_OF_THE_RIGHTEOUS_ARMOR });
    }

    void HandleArmor()
    {
        GetCaster()->CastSpell(GetCaster(), SPELL_PALADIN_SHIELD_OF_THE_RIGHTEOUS_ARMOR, true);
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_pal_shield_of_the_righteous::HandleArmor);
    }
};

// 184662 - Shield of Vengeance
class spell_pal_shield_of_vengeance : public AuraScript
{
    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_SHIELD_OF_VENGEANCE_DAMAGE }) && ValidateSpellEffect({ { spellInfo->Id, EFFECT_1 } });
    }

    void CalculateAmount(AuraEffect const* /*aurEff*/, SpellEffectValue& amount, bool& /*canBeRecalculated*/)
    {
        amount = CalculatePct(GetUnitOwner()->GetMaxHealth(), GetEffectInfo(EFFECT_1).CalcValue());
        if (Player const* player = GetUnitOwner()->ToPlayer())
            AddPct(amount, player->GetRatingBonusValue(CR_VERSATILITY_DAMAGE_DONE) + player->GetTotalAuraModifier(SPELL_AURA_MOD_VERSATILITY));

        _initialAmount = amount;
    }

    void HandleRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->CastSpell(GetTarget(), SPELL_PALADIN_SHIELD_OF_VENGEANCE_DAMAGE,
            CastSpellExtraArgs(TRIGGERED_FULL_MASK).AddSpellMod(SPELLVALUE_BASE_POINT0, _initialAmount - aurEff->GetAmount()));
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_pal_shield_of_vengeance::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
        OnEffectRemove += AuraEffectApplyFn(spell_pal_shield_of_vengeance::HandleRemove, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB, AURA_EFFECT_HANDLE_REAL);
    }

    SpellEffectValue _initialAmount = 0;
};

// 469304 - Steed of Liberty
class spell_pal_steed_of_liberty : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_BLESSING_OF_FREEDOM });
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo const& /*eventInfo*/) const
    {
        Unit* target = GetTarget();
        target->CastSpell(target, SPELL_PALADIN_BLESSING_OF_FREEDOM, CastSpellExtraArgsInit{
            .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
            .TriggeringAura = aurEff,
            .SpellValueOverrides = { { SPELLVALUE_DURATION, aurEff->GetAmountAsInt() } }
        });
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_pal_steed_of_liberty::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// 85256 - Templar's Verdict
class spell_pal_templar_s_verdict : public SpellScript
{
    bool Validate(SpellInfo const* /*spellEntry*/) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_TEMPLAR_VERDICT_DAMAGE });
    }

    void HandleHitTarget(SpellEffIndex /*effIndex*/)
    {
        GetCaster()->CastSpell(GetHitUnit(), SPELL_PALADIN_TEMPLAR_VERDICT_DAMAGE, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_pal_templar_s_verdict::HandleHitTarget, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 28789 - Holy Power
class spell_pal_t3_6p_bonus : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
        {
            SPELL_PALADIN_HOLY_POWER_ARMOR,
            SPELL_PALADIN_HOLY_POWER_ATTACK_POWER,
            SPELL_PALADIN_HOLY_POWER_SPELL_POWER,
            SPELL_PALADIN_HOLY_POWER_MP5
        });
    }

    void HandleProc(AuraEffect* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        uint32 spellId;
        Unit* caster = eventInfo.GetActor();
        Unit* target = eventInfo.GetActionTarget();

        switch (target->GetClass())
        {
            case CLASS_PALADIN:
            case CLASS_PRIEST:
            case CLASS_SHAMAN:
            case CLASS_DRUID:
                spellId = SPELL_PALADIN_HOLY_POWER_MP5;
                break;
            case CLASS_MAGE:
            case CLASS_WARLOCK:
                spellId = SPELL_PALADIN_HOLY_POWER_SPELL_POWER;
                break;
            case CLASS_HUNTER:
            case CLASS_ROGUE:
                spellId = SPELL_PALADIN_HOLY_POWER_ATTACK_POWER;
                break;
            case CLASS_WARRIOR:
                spellId = SPELL_PALADIN_HOLY_POWER_ARMOR;
                break;
            default:
                return;
        }

        caster->CastSpell(target, spellId, aurEff);
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_pal_t3_6p_bonus::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// 64890 - Item - Paladin T8 Holy 2P Bonus
class spell_pal_t8_2p_bonus : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellEffect({ { SPELL_PALADIN_HOLY_MENDING, EFFECT_0 } })
            && sSpellMgr->AssertSpellInfo(SPELL_PALADIN_HOLY_MENDING, DIFFICULTY_NONE)->GetEffect(EFFECT_0).GetPeriodicTickCount() > 0;
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        HealInfo* healInfo = eventInfo.GetHealInfo();
        return healInfo && healInfo->GetHeal();
    }

    void HandleProc(AuraEffect* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Unit* caster = eventInfo.GetActor();
        Unit* target = eventInfo.GetActionTarget();

        SpellEffectInfo const& hotEffect = sSpellMgr->AssertSpellInfo(SPELL_PALADIN_HOLY_MENDING, GetCastDifficulty())->GetEffect(EFFECT_0);
        SpellEffectValue amount = CalculatePct(static_cast<SpellEffectValue>(eventInfo.GetHealInfo()->GetHeal()), aurEff->GetAmount());

        amount /= hotEffect.GetPeriodicTickCount();

        CastSpellExtraArgs args(aurEff);
        args.AddSpellBP0(amount);
        caster->CastSpell(target, SPELL_PALADIN_HOLY_MENDING, args);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_pal_t8_2p_bonus::CheckProc);
        OnEffectProc += AuraEffectProcFn(spell_pal_t8_2p_bonus::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// 405547 - Paladin Protection 10.1 Class Set 2pc
class spell_pal_t30_2p_protection_bonus : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellEffect({ { SPELL_PALADIN_T30_2P_HEARTFIRE_DAMAGE, EFFECT_0 } })
            && sSpellMgr->AssertSpellInfo(SPELL_PALADIN_T30_2P_HEARTFIRE_DAMAGE, DIFFICULTY_NONE)->GetEffect(EFFECT_0).GetPeriodicTickCount() > 0;
    }

    void HandleProc(AuraEffect* aurEff, ProcEventInfo& procInfo)
    {
        PreventDefaultAction();

        Unit* caster = procInfo.GetActor();
        uint32 ticks = sSpellMgr->AssertSpellInfo(SPELL_PALADIN_T30_2P_HEARTFIRE_DAMAGE, DIFFICULTY_NONE)->GetEffect(EFFECT_0).GetPeriodicTickCount();
        SpellEffectValue damage = CalculatePct(procInfo.GetDamageInfo()->GetOriginalDamage(), aurEff->GetAmount()) / ticks;

        caster->CastSpell(procInfo.GetActionTarget(), SPELL_PALADIN_T30_2P_HEARTFIRE_DAMAGE, CastSpellExtraArgs(aurEff)
            .SetTriggeringSpell(procInfo.GetProcSpell())
            .AddSpellMod(SPELLVALUE_BASE_POINT0, damage));
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_pal_t30_2p_protection_bonus::HandleProc, EFFECT_1, SPELL_AURA_DUMMY);
    }
};

// 408461 - Heartfire
class spell_pal_t30_2p_protection_bonus_heal : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_T30_2P_HEARTFIRE_HEAL });
    }

    bool CheckProc(AuraEffect const* /*aurEff*/, ProcEventInfo& procInfo)
    {
        return procInfo.GetDamageInfo() && procInfo.GetSpellInfo() && procInfo.GetSpellInfo()->HasLabel(SPELL_LABEL_PALADIN_T30_2P_HEARTFIRE);
    }

    void HandleProc(AuraEffect* aurEff, ProcEventInfo& procInfo)
    {
        GetTarget()->CastSpell(GetTarget(), SPELL_PALADIN_T30_2P_HEARTFIRE_HEAL, CastSpellExtraArgs(aurEff)
            .SetTriggeringSpell(procInfo.GetProcSpell())
            .AddSpellMod(SPELLVALUE_BASE_POINT0, procInfo.GetDamageInfo()->GetOriginalDamage()));
    }

    void Register() override
    {
        DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_t30_2p_protection_bonus_heal::CheckProc, EFFECT_0, SPELL_AURA_DUMMY);
        OnEffectProc += AuraEffectProcFn(spell_pal_t30_2p_protection_bonus_heal::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// 255937 - Wake of Ashes
class spell_pal_wake_of_ashes : public SpellScript
{
    bool Validate(SpellInfo const* /*spellEntry*/) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_WAKE_OF_ASHES_STUN, SPELL_PALADIN_RADIANT_GLORY, SPELL_PALADIN_RADIANT_GLORY_PASSIVE, SPELL_PALADIN_AVENGING_WRATH });
    }

    void HandleCast()
    {
        Unit* caster = GetCaster();
        PaladinRadiantGlory::TriggerWakeOfAshesAvengingWrath(caster, GetSpell());
    }

    void HandleHitTarget(SpellEffIndex /*effIndex*/) const
    {
        Unit* target = GetHitUnit();

        if (target->GetCreatureType() == CREATURE_TYPE_DEMON || target->GetCreatureType() == CREATURE_TYPE_UNDEAD)
            GetCaster()->CastSpell(target, SPELL_PALADIN_WAKE_OF_ASHES_STUN, CastSpellExtraArgsInit{
                .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
                .TriggeringSpell = GetSpell()
            });
    }

    void Register() override
    {
        OnCast += SpellCastFn(spell_pal_wake_of_ashes::HandleCast);
        OnEffectHitTarget += SpellEffectFn(spell_pal_wake_of_ashes::HandleHitTarget, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

// 269569 - Zeal
class spell_pal_zeal : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_PALADIN_ZEAL_AURA });
    }

    void HandleEffectProc(AuraEffect* aurEff, ProcEventInfo& /*procInfo*/)
    {
        Unit* target = GetTarget();
        target->CastSpell(target, SPELL_PALADIN_ZEAL_AURA, CastSpellExtraArgs(TRIGGERED_FULL_MASK).AddSpellMod(SPELLVALUE_AURA_STACK, aurEff->GetAmountAsInt()));

        PreventDefaultAction();
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_pal_zeal::HandleEffectProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

void AddSC_paladin_spell_scripts()
{
    new spell_pal_radiant_glory();
    new spell_pal_dawnlight_cast_trigger();
    RegisterSpellScript(spell_pal_dawnlight);
    RegisterSpellScript(spell_pal_dawnlight_empower);
    RegisterSpellScript(spell_pal_dawnlight_radiate_meta);
    RegisterSpellScript(spell_pal_dawnlight_charges);
    RegisterSpellScript(spell_pal_dawnlight_damage);
    RegisterSpellScript(spell_pal_a_just_reward);
    RegisterSpellScript(spell_pal_ardent_defender);
    RegisterSpellScript(spell_pal_art_of_war);
    RegisterSpellScript(spell_pal_righteous_cause);
    RegisterAreaTriggerAI(areatrigger_pal_ashen_hallow);
    RegisterSpellScript(spell_pal_awakening);
    RegisterSpellScript(spell_pal_blade_of_vengeance);
    RegisterSpellScript(spell_pal_blade_of_vengeance_aoe_target_selector);
    RegisterSpellScript(spell_pal_consecrated_blade);
    RegisterSpellScript(spell_pal_blessing_of_protection);
    RegisterSpellScript(spell_pal_blinding_light);
    RegisterSpellScript(spell_pal_crusader_might);
    RegisterSpellScript(spell_pal_crusading_strikes);
    RegisterSpellScript(spell_pal_consecration);
    RegisterAreaTriggerAI(areatrigger_pal_consecration);
    RegisterSpellScript(spell_pal_divine_auxiliary);
    RegisterSpellScript(spell_pal_divine_purpose);
    RegisterSpellScript(spell_pal_divine_shield);
    RegisterSpellScript(spell_pal_divine_steed);
    RegisterSpellScript(spell_pal_divine_storm);
    RegisterSpellScript(spell_pal_divine_storm_damage);
    RegisterSpellScript(spell_pal_empyrean_power);
    RegisterSpellScript(spell_pal_second_sunrise);
    RegisterSpellScript(spell_pal_second_sunrise_effectiveness);
    RegisterSpellScript(spell_pal_empyrean_legacy);
    RegisterSpellScript(spell_pal_empyrean_legacy_buff);
    RegisterSpellScript(spell_pal_empyrean_legacy_spender);
    RegisterSpellScript(spell_pal_empyreal_ward);
    RegisterSpellScript(spell_pal_expurgation);
    RegisterSpellAndAuraScriptPair(spell_pal_eternal_flame, spell_pal_eternal_flame_aura);
    RegisterSpellAndAuraScriptPair(spell_pal_execution_sentence, spell_pal_execution_sentence_aura);
    RegisterSpellScript(spell_pal_eye_for_an_eye);
    RegisterSpellScript(spell_pal_final_verdict);
    RegisterSpellScript(spell_pal_fist_of_justice);
    RegisterSpellScript(spell_pal_glyph_of_holy_light);
    RegisterSpellScript(spell_pal_grand_crusader);
    RegisterSpellScript(spell_pal_hammer_of_wrath_avenging_wrath);
    RegisterSpellScript(spell_pal_hammer_of_wrath_talent);
    RegisterSpellScript(spell_pal_hammer_of_the_righteous);
    RegisterSpellScript(spell_pal_hand_of_sacrifice);
    RegisterSpellScript(spell_pal_infusion_of_light);
    RegisterSpellScript(spell_pal_moment_of_glory);
    RegisterSpellScript(spell_pal_judgment);
    RegisterSpellScript(spell_pal_justicars_vengeance);
    RegisterSpellScript(spell_pal_holy_prism);
    RegisterSpellScript(spell_pal_holy_prism_selector);
    RegisterSpellScript(spell_pal_holy_shock);
    RegisterSpellScript(spell_pal_holy_shock_damage_visual);
    RegisterSpellScript(spell_pal_holy_shock_heal_visual);
    RegisterSpellScript(spell_pal_item_healing_discount);
    RegisterSpellScript(spell_pal_item_t6_trinket);
    RegisterSpellScript(spell_pal_lay_on_hands);
    RegisterSpellScript(spell_pal_light_s_beacon);
    RegisterSpellScript(spell_pal_light_hammer_init_summon);
    RegisterSpellScript(spell_pal_light_hammer_periodic);
    RegisterSpellScript(spell_pal_righteous_protector);
    RegisterSpellScript(spell_pal_righteous_verdict);
    RegisterSpellScript(spell_pal_selfless_healer);
    RegisterSpellScript(spell_pal_shield_of_the_righteous);
    RegisterSpellScript(spell_pal_shield_of_vengeance);
    RegisterSpellScript(spell_pal_steed_of_liberty);
    RegisterSpellScript(spell_pal_templar_s_verdict);
    RegisterSpellScript(spell_pal_t3_6p_bonus);
    RegisterSpellScript(spell_pal_t8_2p_bonus);
    RegisterSpellScript(spell_pal_t30_2p_protection_bonus);
    RegisterSpellScript(spell_pal_t30_2p_protection_bonus_heal);
    RegisterSpellScript(spell_pal_wake_of_ashes);
    RegisterSpellScript(spell_pal_zeal);
}
