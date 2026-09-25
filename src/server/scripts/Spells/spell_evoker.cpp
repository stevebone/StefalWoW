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
 * Scripts for spells with SPELLFAMILY_EVOKER and SPELLFAMILY_GENERIC spells used by evoker players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_evo_".
 */

#include "AreaTrigger.h"
#include "AreaTriggerAI.h"
#include "Containers.h"
#include "DB2Stores.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellHistory.h"
#include "SpellMgr.h"
#include "PathGenerator.h"
#include "SpellScript.h"
#include "TaskScheduler.h"

enum EvokerSpells
{
    SPELL_EVOKER_AZURE_ESSENCE_BURST            = 375721,
	SPELL_EVOKER_AZURE_SWEEP_BUFF               = 1265871,
    SPELL_EVOKER_AZURE_SWEEP                    = 1265872,
    SPELL_EVOKER_ETERNITY_SURGE_VISUAL          = 359090,
    SPELL_EVOKER_RISING_FURY_TALENT             = 1271687,
    SPELL_EVOKER_RISING_FURY                    = 1271783,
    SPELL_EVOKER_RISEN_FURY_TALENT              = 1271788,
    SPELL_EVOKER_RISEN_FURY                     = 1271799,
    SPELL_EVOKER_SCINTILLATION                  = 370821,
    SPELL_EVOKER_SCINTILLATION_PROC             = 431192,
    SPELL_EVOKER_TWIN_FLAME                     = 1265979,
    SPELL_EVOKER_TWIN_FLAME_DAMAGE              = 1265980,
    SPELL_EVOKER_TWIN_FLAME_HEAL                = 1265991,
    SPELL_EVOKER_DISINTEGRATE_DAMAGE            = 356995,
	SPELL_EVOKER_DRAGONRAGE                     = 375087,
    SPELL_EVOKER_DRAGONRAGE_TRIGGER             = 375088,
    SPELL_EVOKER_BLAST_FURNACE                  = 375510,
    SPELL_EVOKER_BLESSING_OF_THE_BRONZE_DK      = 381732,
    SPELL_EVOKER_BLESSING_OF_THE_BRONZE_DH      = 381741,
    SPELL_EVOKER_BLESSING_OF_THE_BRONZE_DRUID   = 381746,
    SPELL_EVOKER_BLESSING_OF_THE_BRONZE_EVOKER  = 381748,
    SPELL_EVOKER_BLESSING_OF_THE_BRONZE_HUNTER  = 381749,
    SPELL_EVOKER_BLESSING_OF_THE_BRONZE_MAGE    = 381750,
    SPELL_EVOKER_BLESSING_OF_THE_BRONZE_MONK    = 381751,
    SPELL_EVOKER_BLESSING_OF_THE_BRONZE_PALADIN = 381752,
    SPELL_EVOKER_BLESSING_OF_THE_BRONZE_PRIEST  = 381753,
    SPELL_EVOKER_BLESSING_OF_THE_BRONZE_ROGUE   = 381754,
    SPELL_EVOKER_BLESSING_OF_THE_BRONZE_SHAMAN  = 381756,
    SPELL_EVOKER_BLESSING_OF_THE_BRONZE_WARLOCK = 381757,
    SPELL_EVOKER_BLESSING_OF_THE_BRONZE_WARRIOR = 381758,
    SPELL_EVOKER_BURNOUT                        = 375802,
    SPELL_EVOKER_CALL_OF_YSERA_TALENT           = 373834,
    SPELL_EVOKER_CALL_OF_YSERA                  = 373835,
    SPELL_EVOKER_CAUSALITY                      = 375777,
    SPELL_EVOKER_DISINTEGRATE                   = 356995,
    SPELL_EVOKER_EMERALD_BLOSSOM_HEAL           = 355916,
    SPELL_EVOKER_ENERGIZING_FLAME               = 400006,
    SPELL_EVOKER_ESSENCE_BURST                  = 359618,
    SPELL_EVOKER_FIRESTORM_DAMAGE               = 369374,
    SPELL_EVOKER_ETERNITY_SURGE                 = 359073,
	SPELL_EVOKER_LANDSLIDE                      = 358385,
    SPELL_EVOKER_LANDSLIDE_ROOT                 = 355689,
    SPELL_EVOKER_ETERNITY_SURGE_BLUE            = 382411,
    SPELL_EVOKER_ETERNITY_SURGE_DAMAGE          = 359077,
    SPELL_EVOKER_ETERNITYS_SPAN                 = 375757,
    SPELL_EVOKER_FIRE_BREATH                    = 357208,
    SPELL_EVOKER_FIRE_BREATH_DAMAGE             = 357209,
    SPELL_EVOKER_GLIDE_KNOCKBACK                = 358736,
    SPELL_EVOKER_HOVER                          = 358267,
    SPELL_EVOKER_LIVING_FLAME                   = 361469,
    SPELL_EVOKER_LIVING_FLAME_DAMAGE            = 361500,
    SPELL_EVOKER_LIVING_FLAME_HEAL              = 361509,
    SPELL_EVOKER_PANACEA_HEAL                   = 387763,
    SPELL_EVOKER_PANACEA_TALENT                 = 387761,
    SPELL_EVOKER_PERMEATING_CHILL_TALENT        = 370897,
    SPELL_EVOKER_PYRE_DAMAGE                    = 357212,
	SPELL_EVOKER_CONSUME_FLAME                  = 444088,
    SPELL_EVOKER_CONSUME_FLAME_DAMAGE           = 444089,
    SPELL_EVOKER_FIRE_BREATH_DOT                = 357209,
    SPELL_EVOKER_RUBY_EMBERS                    = 365937,
    SPELL_EVOKER_RUBY_ESSENCE_BURST             = 376872,
    SPELL_EVOKER_SCOURING_FLAME                 = 378438,
    SPELL_EVOKER_SNAPFIRE                       = 370818,
    SPELL_EVOKER_SOAR_RACIAL                    = 369536,
    SPELL_EVOKER_VERDANT_EMBRACE_HEAL           = 361195,
    SPELL_EVOKER_VERDANT_EMBRACE_JUMP           = 373514,
    SPELL_VISAGE                                = 372014,
    SPELL_ALTERED_FORM                          = 97709,
    SPELL_HATRED                                = 118328,
    SPELL_SKYWARD_ASCENT                        = 376744,
    SPELL_SURGE_FORWARD                         = 376743,
	SPELL_EVOKER_DEEP_BREATH                    = 357210,
    SPELL_EVOKER_DEEP_BREATH_INFLIGHT           = 362010,
};

enum EvokerSpellLabels
{
    SPELL_LABEL_EVOKER_BLUE                 = 1465,
};

enum EvokerSpellVisuals
{
    SPELL_VISUAL_KIT_EVOKER_VERDANT_EMBRACE_JUMP    = 152557,
};

// 362969 - Azure Strike (blue)
class spell_evo_azure_strike : public SpellScript
{
    void FilterTargets(std::list<WorldObject*>& targets)
    {
        targets.remove(GetExplTargetUnit());
        Trinity::Containers::RandomResize(targets, GetEffectInfo(EFFECT_0).CalcValueAsInt(GetCaster()) - 1);
        targets.push_back(GetExplTargetUnit());
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_evo_azure_strike::FilterTargets, EFFECT_1, TARGET_UNIT_DEST_AREA_ENEMY);
    }
};

// 381732 - Blessing of the Bronze (Bronze)
// 381741 - Blessing of the Bronze (Bronze)
// 381746 - Blessing of the Bronze (Bronze)
// 381748 - Blessing of the Bronze (Bronze)
// 381749 - Blessing of the Bronze (Bronze)
// 381750 - Blessing of the Bronze (Bronze)
// 381751 - Blessing of the Bronze (Bronze)
// 381752 - Blessing of the Bronze (Bronze)
// 381753 - Blessing of the Bronze (Bronze)
// 381754 - Blessing of the Bronze (Bronze)
// 381756 - Blessing of the Bronze (Bronze)
// 381757 - Blessing of the Bronze (Bronze)
// 381758 - Blessing of the Bronze (Bronze)
class spell_evo_blessing_of_the_bronze : public SpellScript
{
    void RemoveInvalidTargets(std::list<WorldObject*>& targets) const
    {
        targets.remove_if([&](WorldObject const* target)
        {
            Unit const* unitTarget = target->ToUnit();
            if (!unitTarget)
                return true;

            switch (GetSpellInfo()->Id)
            {
                case SPELL_EVOKER_BLESSING_OF_THE_BRONZE_DK: return unitTarget->GetClass() != CLASS_DEATH_KNIGHT;
                case SPELL_EVOKER_BLESSING_OF_THE_BRONZE_DH: return unitTarget->GetClass() != CLASS_DEMON_HUNTER;
                case SPELL_EVOKER_BLESSING_OF_THE_BRONZE_DRUID: return unitTarget->GetClass() != CLASS_DRUID;
                case SPELL_EVOKER_BLESSING_OF_THE_BRONZE_EVOKER: return unitTarget->GetClass() != CLASS_EVOKER;
                case SPELL_EVOKER_BLESSING_OF_THE_BRONZE_HUNTER: return unitTarget->GetClass() != CLASS_HUNTER;
                case SPELL_EVOKER_BLESSING_OF_THE_BRONZE_MAGE: return unitTarget->GetClass() != CLASS_MAGE;
                case SPELL_EVOKER_BLESSING_OF_THE_BRONZE_MONK: return unitTarget->GetClass() != CLASS_MONK;
                case SPELL_EVOKER_BLESSING_OF_THE_BRONZE_PALADIN: return unitTarget->GetClass() != CLASS_PALADIN;
                case SPELL_EVOKER_BLESSING_OF_THE_BRONZE_PRIEST: return unitTarget->GetClass() != CLASS_PRIEST;
                case SPELL_EVOKER_BLESSING_OF_THE_BRONZE_ROGUE: return unitTarget->GetClass() != CLASS_ROGUE;
                case SPELL_EVOKER_BLESSING_OF_THE_BRONZE_SHAMAN: return unitTarget->GetClass() != CLASS_SHAMAN;
                case SPELL_EVOKER_BLESSING_OF_THE_BRONZE_WARLOCK: return unitTarget->GetClass() != CLASS_WARLOCK;
                case SPELL_EVOKER_BLESSING_OF_THE_BRONZE_WARRIOR: return unitTarget->GetClass() != CLASS_WARRIOR;
                default:
                    break;
            }
            return true;
        });
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_evo_blessing_of_the_bronze::RemoveInvalidTargets, EFFECT_ALL, TARGET_UNIT_CASTER_AREA_RAID);
    }
};

// 375801 - Burnout
class spell_evo_burnout : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_EVOKER_BURNOUT });
    }

    static bool CheckProc(AuraScript const&, AuraEffect const* aurEff, ProcEventInfo const& /*eventInfo*/)
    {
        return roll_chance(aurEff->GetAmount());
    }

    static void HandleProc(AuraScript const&, AuraEffect const* /*aurEff*/, ProcEventInfo const& eventInfo)
    {
        eventInfo.GetActor()->CastSpell(eventInfo.GetActor(), SPELL_EVOKER_BURNOUT, TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
    }

    void Register() override
    {
        DoCheckEffectProc += AuraCheckEffectProcFn(spell_evo_burnout::CheckProc, EFFECT_0, SPELL_AURA_DUMMY);
        OnEffectProc += AuraEffectProcFn(spell_evo_burnout::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// 373834 - Call of Ysera (attached to 361195 - Verdant Embrace (Green))
class spell_evo_call_of_ysera : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_EVOKER_CALL_OF_YSERA_TALENT, SPELL_EVOKER_CALL_OF_YSERA });
    }

    bool Load() override
    {
        return GetCaster()->HasAura(SPELL_EVOKER_CALL_OF_YSERA_TALENT);
    }

    void HandleCallOfYsera() const
    {
        GetCaster()->CastSpell(GetCaster(), SPELL_EVOKER_CALL_OF_YSERA, CastSpellExtraArgsInit{
            .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
            .TriggeringSpell = GetSpell()
        });
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_evo_call_of_ysera::HandleCallOfYsera);
    }
};

static constexpr std::array<uint32, 3> CausalityAffectedEmpowerSpells = { SPELL_EVOKER_ETERNITY_SURGE, SPELL_EVOKER_ETERNITY_SURGE_BLUE, SPELL_EVOKER_FIRE_BREATH };

// Called by 356995 - Disintegrate (Blue)
class spell_evo_causality_disintegrate : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellEffect({ { SPELL_EVOKER_CAUSALITY, EFFECT_1 } });
    }

    bool Load() override
    {
        return GetCaster()->HasAura(SPELL_EVOKER_CAUSALITY);
    }

    void OnTick(AuraEffect const* /*aurEff*/) const
    {
        if (AuraEffect const* causality = GetCaster()->GetAuraEffect(SPELL_EVOKER_CAUSALITY, EFFECT_0))
            for (uint32 spell : CausalityAffectedEmpowerSpells)
                GetCaster()->GetSpellHistory()->ModifyCooldown(spell, Milliseconds(causality->GetAmountAsInt()));
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_evo_causality_disintegrate::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
    }
};

// Called by 357212 - Pyre (Red)
class spell_evo_causality_pyre : public SpellScript
{
    static constexpr int64 TargetLimit = 5;

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellEffect({ { SPELL_EVOKER_CAUSALITY, EFFECT_1 } });
    }

    bool Load() override
    {
        return GetCaster()->HasAura(SPELL_EVOKER_CAUSALITY);
    }

    void HandleCooldown() const
    {
        AuraEffect const* causality = GetCaster()->GetAuraEffect(SPELL_EVOKER_CAUSALITY, EFFECT_1);
        if (!causality)
            return;

        Milliseconds cooldownReduction = Milliseconds(std::min(GetUnitTargetCountForEffect(EFFECT_0), TargetLimit) * causality->GetAmountAsInt());
        for (uint32 spell : CausalityAffectedEmpowerSpells)
            GetCaster()->GetSpellHistory()->ModifyCooldown(spell, cooldownReduction);
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_evo_causality_pyre::HandleCooldown);
    }
};

// 370455 - Charged Blast
class spell_evo_charged_blast : public AuraScript
{
    bool CheckProc(ProcEventInfo& procInfo)
    {
        return procInfo.GetSpellInfo() && procInfo.GetSpellInfo()->HasLabel(SPELL_LABEL_EVOKER_BLUE);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_evo_charged_blast::CheckProc);
    }
};

// 355913 - Emerald Blossom
struct at_evo_emerald_blossom : AreaTriggerAI
{
    using AreaTriggerAI::AreaTriggerAI;

    void OnRemove() override
    {
        if (Unit* caster = at->GetCaster())
            caster->CastSpell(at->GetPosition(), SPELL_EVOKER_EMERALD_BLOSSOM_HEAL, TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
    }
};

// 355916 - Emerald Blossom (Green)
class spell_evo_emerald_blossom_heal : public SpellScript
{
    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellEffect({ { spellInfo->Id, EFFECT_1 } });
    }

    void FilterTargets(std::list<WorldObject*>& targets) const
    {
        uint32 const maxTargets = uint32(GetSpellInfo()->GetEffect(EFFECT_1).CalcValueAsInt(GetCaster()));
        Trinity::SelectRandomInjuredTargets(targets, maxTargets, true);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_evo_emerald_blossom_heal::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ALLY);
    }
};

// Called by 362969 - Azure Strike
// Called by 361469 - Living Flame (Red)
class spell_evo_essence_burst_trigger : public SpellScript
{
public:
    explicit spell_evo_essence_burst_trigger(uint32 talentAuraId) : _talentAuraId(talentAuraId) { }

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ _talentAuraId, SPELL_EVOKER_ESSENCE_BURST, SPELL_EVOKER_DRAGONRAGE });
    }

    bool Load() override
    {
		if (GetCaster()->HasAura(SPELL_EVOKER_DRAGONRAGE))
            return true;
		
        AuraEffect const* aurEff = GetCaster()->GetAuraEffect(_talentAuraId, EFFECT_0);
        return aurEff && roll_chance(aurEff->GetAmount());
    }

    void HandleEssenceBurst() const
    {
        GetCaster()->CastSpell(GetCaster(), SPELL_EVOKER_ESSENCE_BURST, CastSpellExtraArgsInit{
            .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
            .TriggeringSpell = GetSpell()
        });
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_evo_essence_burst_trigger::HandleEssenceBurst);
    }

    uint32 _talentAuraId;
};

// 357208 Fire Breath (Red)
// 382266 Fire Breath (Red)
class spell_evo_fire_breath : public SpellScript
{
public:
    struct data
    {
        int32 EmpowerLevel;
    };

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_EVOKER_FIRE_BREATH_DAMAGE, SPELL_EVOKER_BLAST_FURNACE });
    }

    void OnComplete(int32 completedStageCount) const
    {
        SpellEffectValue dotTicks = 10 - (completedStageCount - 1) * 3;
        if (AuraEffect const* blastFurnace = GetCaster()->GetAuraEffect(SPELL_EVOKER_BLAST_FURNACE, EFFECT_0))
            dotTicks += blastFurnace->GetAmount() / 2;

        GetCaster()->CastSpell(GetCaster(), SPELL_EVOKER_FIRE_BREATH_DAMAGE, CastSpellExtraArgs()
            .SetTriggeringSpell(GetSpell())
            .SetTriggerFlags(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
            .AddSpellMod(SPELLVALUE_DURATION_PCT, 100 * dotTicks)
            .SetCustomArg(data{ .EmpowerLevel = completedStageCount }));
    }

    void Register() override
    {
        OnEmpowerCompleted += SpellOnEmpowerStageCompletedFn(spell_evo_fire_breath::OnComplete);
    }
};

// 357209 Fire Breath (Red)
class spell_evo_fire_breath_damage : public SpellScript
{
    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellEffect({ { spellInfo->Id, EFFECT_2 } })
            && spellInfo->GetEffect(EFFECT_2).IsAura(SPELL_AURA_MOD_SILENCE); // validate we are removing the correct effect
    }

    void AddBonusUpfrontDamage(SpellEffectInfo const& /*spellEffectInfo*/, Unit const* victim, int32& /*damage*/, int32& flatMod, float& /*pctMod*/) const
    {
        spell_evo_fire_breath::data const* params = std::any_cast<spell_evo_fire_breath::data>(&GetSpell()->m_customArg);
        if (!params)
            return;

        if (AuraEffect const* fireBreath = victim->GetAuraEffect(GetSpellInfo()->Id, EFFECT_1, GetCaster()->GetGUID()))
            flatMod += fireBreath->GetEstimatedAmount().value_or(fireBreath->GetAmount()) * (params->EmpowerLevel - 1) * 3;
    }

    void RemoveUnusedEffect(std::list<WorldObject*>& targets) const
    {
        targets.clear();
    }

    void Register() override
    {
        CalcDamage += SpellCalcDamageFn(spell_evo_fire_breath_damage::AddBonusUpfrontDamage);
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_evo_fire_breath_damage::RemoveUnusedEffect, EFFECT_2, TARGET_UNIT_CONE_CASTER_TO_DEST_ENEMY);
    }
};

// 369372 - Firestorm (Red)
struct at_evo_firestorm : AreaTriggerAI
{
    using AreaTriggerAI::AreaTriggerAI;

    struct extra_create_data
    {
        float SnapshotDamageMultipliers = 1.0f;
    };

    static extra_create_data& GetOrCreateExtraData(Spell* firestorm)
    {
        if (firestorm->m_customArg.type() != typeid(extra_create_data))
            return firestorm->m_customArg.emplace<extra_create_data>();

        return *std::any_cast<extra_create_data>(&firestorm->m_customArg);
    }

    void OnCreate(Spell const* creatingSpell) override
    {
        _damageSpellCustomArg = creatingSpell->m_customArg;

        _scheduler.Schedule(0ms, [this](TaskContext& task)
        {
            FloatMilliseconds period = 2s;
            if (Unit* caster = at->GetCaster())
            {
                period *= *caster->m_unitData->ModCastingSpeed;
                caster->CastSpell(at->GetPosition(), SPELL_EVOKER_FIRESTORM_DAMAGE, CastSpellExtraArgsInit{
                    .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
                    .CustomArg = _damageSpellCustomArg
                });
            }

            task.Repeat(duration_cast<Milliseconds>(period));
        });
    }

    void OnUpdate(uint32 diff) override
    {
        _scheduler.Update(diff);
    }

private:
    TaskScheduler _scheduler;
    std::any _damageSpellCustomArg;
};

// 358733 - Glide (Racial)
class spell_evo_glide : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_EVOKER_GLIDE_KNOCKBACK, SPELL_EVOKER_HOVER, SPELL_EVOKER_SOAR_RACIAL });
    }

    SpellCastResult CheckCast()
    {
        Unit* caster = GetCaster();

        if (!caster->IsFalling())
            return SPELL_FAILED_NOT_ON_GROUND;

        return SPELL_CAST_OK;
    }

    void HandleCast()
    {
        Player* caster = GetCaster()->ToPlayer();
        if (!caster)
            return;

        caster->CastSpell(caster, SPELL_EVOKER_GLIDE_KNOCKBACK, true);

        caster->GetSpellHistory()->StartCooldown(sSpellMgr->AssertSpellInfo(SPELL_EVOKER_HOVER, GetCastDifficulty()), 0, nullptr, false, 250ms);
        caster->GetSpellHistory()->StartCooldown(sSpellMgr->AssertSpellInfo(SPELL_EVOKER_SOAR_RACIAL, GetCastDifficulty()), 0, nullptr, false, 250ms);
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_evo_glide::CheckCast);
        OnCast += SpellCastFn(spell_evo_glide::HandleCast);
    }
};

// 361469 - Living Flame (Red)
class spell_evo_living_flame : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo ({ SPELL_EVOKER_LIVING_FLAME_DAMAGE, SPELL_EVOKER_LIVING_FLAME_HEAL, SPELL_EVOKER_ENERGIZING_FLAME });
    }

    void HandleHitTarget(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        Unit* hitUnit = GetHitUnit();
        if (caster->IsValidAssistTarget(hitUnit))
            caster->CastSpell(hitUnit, SPELL_EVOKER_LIVING_FLAME_HEAL, true);
        else
            caster->CastSpell(hitUnit, SPELL_EVOKER_LIVING_FLAME_DAMAGE, true);
    }

    void HandleLaunchTarget(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (caster->IsValidAssistTarget(GetHitUnit()))
            return;

        if (AuraEffect* auraEffect = caster->GetAuraEffect(SPELL_EVOKER_ENERGIZING_FLAME, EFFECT_0))
        {
            int32 manaCost = GetSpell()->GetPowerTypeCostAmount(POWER_MANA).value_or(0);
            if (manaCost != 0)
                GetCaster()->ModifyPower(POWER_MANA, CalculatePct(manaCost, auraEffect->GetAmount()));
        }
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_evo_living_flame::HandleHitTarget, EFFECT_0, SPELL_EFFECT_DUMMY);
        OnEffectLaunchTarget += SpellEffectFn(spell_evo_living_flame::HandleLaunchTarget, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 387761 Panacea (Green) (attached to 355913 - Emerald Blossom (Green) and 360995 - Verdant Embrace (Green))
class spell_evo_panacea : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_EVOKER_PANACEA_TALENT, SPELL_EVOKER_PANACEA_HEAL });
    }

    bool Load() override
    {
        return GetCaster()->HasAura(SPELL_EVOKER_PANACEA_TALENT);
    }

    void HandlePanacea() const
    {
        GetCaster()->CastSpell(GetCaster(), SPELL_EVOKER_PANACEA_HEAL, CastSpellExtraArgsInit{
            .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
            .TriggeringSpell = GetSpell()
        });
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_evo_panacea::HandlePanacea);
    }
};

// 381773 - Permeating Chill
class spell_evo_permeating_chill : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_EVOKER_PERMEATING_CHILL_TALENT });
    }

    bool CheckProc(ProcEventInfo& procInfo)
    {
        SpellInfo const* spellInfo = procInfo.GetSpellInfo();
        if (!spellInfo)
            return false;

        if (!spellInfo->HasLabel(SPELL_LABEL_EVOKER_BLUE))
            return false;

        if (!procInfo.GetActor()->HasAura(SPELL_EVOKER_PERMEATING_CHILL_TALENT))
            if (!spellInfo->IsAffected(SPELLFAMILY_EVOKER, { 0x40, 0, 0, 0 })) // disintegrate
                return false;

        return true;
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_evo_permeating_chill::CheckProc);
    }
};

// 1271783 - Rising Fury
class spell_evo_rising_fury : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_EVOKER_RISING_FURY });
    }

    void OnTick(AuraEffect const* /*aurEff*/) const
    {
        GetTarget()->CastSpell(GetTarget(), SPELL_EVOKER_RISING_FURY,
            CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR));
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_evo_rising_fury::OnTick, EFFECT_3, SPELL_AURA_PERIODIC_DUMMY);
    }
};

// 1271799 - Risen Fury
class spell_evo_risen_fury : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_EVOKER_ESSENCE_BURST });
    }

    void OnTick(AuraEffect const* /*aurEff*/) const
    {
        GetTarget()->CastSpell(GetTarget(), SPELL_EVOKER_ESSENCE_BURST,
            CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR));
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_evo_risen_fury::OnTick, EFFECT_3, SPELL_AURA_PERIODIC_DUMMY);
    }
};

// 375087 - Dragonrage
class spell_evo_dragonrage : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_EVOKER_RISING_FURY_TALENT, SPELL_EVOKER_RISING_FURY, SPELL_EVOKER_RISEN_FURY_TALENT, SPELL_EVOKER_RISEN_FURY });
    }

    void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/) const
    {
        Unit* target = GetTarget();
        if (target->HasAura(SPELL_EVOKER_RISING_FURY_TALENT))
            target->CastSpell(target, SPELL_EVOKER_RISING_FURY,
                CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR));
    }

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/) const
    {
        Unit* target = GetTarget();

        if (target->HasAura(SPELL_EVOKER_RISEN_FURY_TALENT))
        {
            if (Aura const* risingFury = target->GetAura(SPELL_EVOKER_RISING_FURY))
            {
                int32 stacks = risingFury->GetStackAmount();
                AuraEffect const* risenEff = target->GetAuraEffect(SPELL_EVOKER_RISEN_FURY_TALENT, EFFECT_0);
                int32 durationMs = stacks * (risenEff ? risenEff->GetAmount() : 4000);
                target->CastSpell(target, SPELL_EVOKER_RISEN_FURY,
                    CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
                        .AddSpellMod(SPELLVALUE_DURATION, durationMs));
            }
        }

        target->RemoveAura(SPELL_EVOKER_RISING_FURY);
    }

    void Register() override
    {
        OnEffectApply += AuraEffectApplyFn(spell_evo_dragonrage::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        OnEffectRemove += AuraEffectRemoveFn(spell_evo_dragonrage::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// 375088 - Dragonrage (trigger)
class spell_evo_dragonrage_trigger : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_EVOKER_PYRE_DAMAGE });
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        int32 maxTargets = GetEffectInfo().CalcValue(caster);

        std::list<Unit*> targets;
        Trinity::AnyUnfriendlyUnitInObjectRangeCheck check(caster, caster, GetSpellInfo()->GetMaxRange(false, caster));
        Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(caster, targets, check);
        Cell::VisitAllObjects(caster, searcher, GetSpellInfo()->GetMaxRange(false, caster));

        Trinity::Containers::RandomResize(targets, uint32(maxTargets));

        for (Unit* target : targets)
            caster->CastSpell(target->GetPosition(), SPELL_EVOKER_PYRE_DAMAGE,
                CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
                    .SetTriggeringSpell(GetSpell()));
    }

    void Register() override
    {
        OnEffectHit += SpellEffectFn(spell_evo_dragonrage_trigger::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 359618 - Essence Burst
class spell_evo_essence_burst_consumed : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_EVOKER_TWIN_FLAME, SPELL_EVOKER_TWIN_FLAME_DAMAGE, SPELL_EVOKER_TWIN_FLAME_HEAL });
    }

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_DEFAULT)
            return;

        Unit* caster = GetTarget();
        if (!caster->HasAura(SPELL_EVOKER_TWIN_FLAME))
            return;

        Unit* target = caster->GetVictim();
        if (!target)
            target = ObjectAccessor::GetUnit(*caster, caster->GetTarget());
        if (!target)
            return;

        Player const* player = caster->ToPlayer();
        uint32 twinSpell = (player && player->GetPrimarySpecialization() == ChrSpecialization(1468))
            ? SPELL_EVOKER_TWIN_FLAME_HEAL
            : SPELL_EVOKER_TWIN_FLAME_DAMAGE;

        caster->CastSpell(target, twinSpell,
            CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR));
    }

    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(spell_evo_essence_burst_consumed::OnRemove, EFFECT_0, SPELL_AURA_ADD_PCT_MODIFIER, AURA_EFFECT_HANDLE_REAL);
    }
};

// 393568 - Pyre
class spell_evo_pyre : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_EVOKER_PYRE_DAMAGE });
    }

    void HandleDamage(SpellEffIndex /*effIndex*/)
    {
        GetCaster()->CastSpell(GetHitUnit()->GetPosition(), SPELL_EVOKER_PYRE_DAMAGE, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_evo_pyre::HandleDamage, EFFECT_0, SPELL_EFFECT_DUMMY);
      }
};

static void ConsumeFireBreath(Unit* caster, Unit* target, int32 consumeMs)
{
    AuraEffect const* consumeFlame = caster->GetAuraEffect(SPELL_EVOKER_CONSUME_FLAME, EFFECT_3);
    if (!consumeFlame)
        return;

    Aura* fireBreath = target->GetAura(SPELL_EVOKER_FIRE_BREATH_DOT, caster->GetGUID());
    if (!fireBreath)
        return;

    int32 remaining = fireBreath->GetDuration();
    if (remaining <= 0)
        return;

    AuraEffect const* dotEffect = fireBreath->GetEffect(EFFECT_1);
    if (!dotEffect)
        return;

    float tickAmount = dotEffect->GetEstimatedAmount().value_or(float(dotEffect->GetAmount()));
    int32 detonationPct = consumeFlame->GetAmount();

    int32 consumed = std::min(consumeMs, remaining);
    fireBreath->ModDuration(-consumed);
    if (fireBreath->GetDuration() <= 0)
        fireBreath->Remove();

    static constexpr int32 FIRE_BREATH_PERIOD = 2000;
    float ticks = float(consumed) / float(FIRE_BREATH_PERIOD);
    int32 damage = int32(ticks * tickAmount * float(detonationPct) / 100.0f);
    if (damage <= 0)
        return;

    caster->CastSpell(target, SPELL_EVOKER_CONSUME_FLAME_DAMAGE,
        CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
            .SetCustomArg(damage));
}

// 357212 - Pyre damage
class spell_evo_pyre_damage : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_EVOKER_CONSUME_FLAME, SPELL_EVOKER_CONSUME_FLAME_DAMAGE });
    }

    void HandleHit(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (!caster->HasAura(SPELL_EVOKER_CONSUME_FLAME))
            return;

        AuraEffect const* consumeEff = caster->GetAuraEffect(SPELL_EVOKER_CONSUME_FLAME, EFFECT_2);
        if (!consumeEff)
            return;

        ConsumeFireBreath(caster, GetHitUnit(), consumeEff->GetAmount());
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_evo_pyre_damage::HandleHit, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

// 361500 Living Flame (Red)
// 361509 Living Flame (Red)
class spell_evo_ruby_embers : public SpellScript
{
    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellInfo({ SPELL_EVOKER_RUBY_EMBERS })
            && ValidateSpellEffect({ { spellInfo->Id, EFFECT_1 } })
            && spellInfo->GetEffect(EFFECT_1).IsEffect(SPELL_EFFECT_APPLY_AURA)
            && spellInfo->GetEffect(EFFECT_1).ApplyAuraPeriod != 0;
    }

    bool Load() override
    {
        return !GetCaster()->HasAura(SPELL_EVOKER_RUBY_EMBERS);
    }

    static void PreventPeriodic(SpellScript const&, WorldObject*& target)
    {
        target = nullptr;
    }

    void Register() override
    {
        OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_evo_ruby_embers::PreventPeriodic, EFFECT_1,
            m_scriptSpellId == SPELL_EVOKER_LIVING_FLAME_DAMAGE ? TARGET_UNIT_TARGET_ENEMY : TARGET_UNIT_TARGET_ALLY);
    }
};

// 357209 Fire Breath (Red)
class spell_evo_scouring_flame : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_EVOKER_SCOURING_FLAME });
    }

    void HandleScouringFlame(std::list<WorldObject*>& targets) const
    {
        if (!GetCaster()->HasAura(SPELL_EVOKER_SCOURING_FLAME))
            targets.clear();
    }

    void CalcDispelCount(SpellEffIndex /*effIndex*/)
    {
        if (spell_evo_fire_breath::data const* params = std::any_cast<spell_evo_fire_breath::data>(&GetSpell()->m_customArg))
            SetEffectValue(params->EmpowerLevel);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_evo_scouring_flame::HandleScouringFlame, EFFECT_3, TARGET_UNIT_CONE_CASTER_TO_DEST_ENEMY);
        OnEffectHitTarget += SpellEffectFn(spell_evo_scouring_flame::CalcDispelCount, EFFECT_3, SPELL_EFFECT_DISPEL);
    }
};

// Called by 368847 - Firestorm (Red)
class spell_evo_snapfire : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellEffect({ { SPELL_EVOKER_SNAPFIRE, EFFECT_1 } });
    }

    bool Load() override
    {
        return GetCaster()->HasAura(SPELL_EVOKER_SNAPFIRE);
    }

    void OnPrecast() override
    {
        if (AuraEffect const* snapfire = GetCaster()->GetAuraEffect(SPELL_EVOKER_SNAPFIRE, EFFECT_1))
            if (GetSpell()->m_appliedMods.contains(snapfire->GetBase()))
                AddPct(at_evo_firestorm::GetOrCreateExtraData(GetSpell()).SnapshotDamageMultipliers, snapfire->GetAmount());
    }

    void Register() override { }
};

// Called by 369374 - Firestorm (Red)
class spell_evo_snapfire_bonus_damage : public SpellScript
{
    void CalculateDamageBonus(SpellEffectInfo const& /*spellEffectInfo*/, Unit* /*victim*/, int32& /*damage*/, int32& /*flatMod*/, float& pctMod) const
    {
        if (at_evo_firestorm::extra_create_data const* bonus = std::any_cast<at_evo_firestorm::extra_create_data>(&GetSpell()->m_customArg))
            pctMod *= bonus->SnapshotDamageMultipliers;
    }

    void Register() override
    {
        CalcDamage += SpellCalcDamageFn(spell_evo_snapfire_bonus_damage::CalculateDamageBonus);
    }
};

// 360995 - Verdant Embrace (Green)
class spell_evo_verdant_embrace : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_EVOKER_VERDANT_EMBRACE_HEAL, SPELL_EVOKER_VERDANT_EMBRACE_JUMP })
            && sSpellVisualKitStore.HasRecord(SPELL_VISUAL_KIT_EVOKER_VERDANT_EMBRACE_JUMP);
    }

    void HandleLaunchTarget(SpellEffIndex /*effIndex*/) const
    {
        Unit* caster = GetCaster();
        Unit* target = GetHitUnit();
        CastSpellExtraArgs args;
        args.SetTriggerFlags(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
        args.SetTriggeringSpell(GetSpell());

        if (target != caster)
        {
            caster->CastSpell(target, SPELL_EVOKER_VERDANT_EMBRACE_JUMP, args);
            caster->SendPlaySpellVisualKit(SPELL_VISUAL_KIT_EVOKER_VERDANT_EMBRACE_JUMP, 0, 0);
        }
        else
            caster->CastSpell(caster, SPELL_EVOKER_VERDANT_EMBRACE_HEAL, args);
    }

    void Register() override
    {
        OnEffectLaunchTarget += SpellEffectFn(spell_evo_verdant_embrace::HandleLaunchTarget, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 396557 - Verdant Embrace
class spell_evo_verdant_embrace_trigger_heal : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_EVOKER_VERDANT_EMBRACE_HEAL });
    }

    void HandleHitTarget(SpellEffIndex /*effIndex*/) const
    {
        GetHitUnit()->CastSpell(GetExplTargetUnit(), SPELL_EVOKER_VERDANT_EMBRACE_HEAL, CastSpellExtraArgsInit{
            .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
            .TriggeringSpell = GetSpell()
        });
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_evo_verdant_embrace_trigger_heal::HandleHitTarget, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 369536 - Soar
class spell_evo_soar : public SpellScript
{
    void HandleOnCast()
    {
        Unit* caster = GetCaster();
        caster->GetMotionMaster()->MoveJump(EVENT_JUMP, Position(caster->GetPositionX(), caster->GetPositionY(), caster->GetPositionZ() + 30.0f), 20.0f, {}, 10.0f);
    }
    void HandleAfterCast()
    {
        Player* caster = GetCaster()->ToPlayer();

        caster->CastSpell(caster, 430747, true);

        float SURGE_SPEED = 30.0f;

        float destX = caster->GetPositionX() + SURGE_SPEED * std::cos(caster->GetOrientation());
        float destY = caster->GetPositionY() + SURGE_SPEED * std::sin(caster->GetOrientation());
        float destZ = caster->GetPositionZ() + SURGE_SPEED * std::tan(caster->m_movementInfo.pitch);

        caster->AddMoveImpulse(Position(destX - caster->GetPositionX(), destY - caster->GetPositionY(), destZ - caster->GetPositionZ()));
    }
    void Register() override
    {
        OnCast += SpellCastFn(spell_evo_soar::HandleOnCast);
        AfterCast += SpellCastFn(spell_evo_soar::HandleAfterCast);
    }
};

// 351239 - Visage (Racial)
class spell_evo_cosmic_visage : public SpellScript
{
    void HandleOnCast()
    {
        Unit* caster = GetCaster();

        if (caster->HasAura(SPELL_VISAGE))
        {
            // Dracthyr Form
            caster->RemoveAurasDueToSpell(SPELL_VISAGE);
            caster->CastSpell(caster, SPELL_ALTERED_FORM, true);
            caster->SendPlaySpellVisual(caster, SPELL_HATRED, 0, 0, 60, false);
            caster->SetDisplayId(108590);
        }
        else
        {
            // Visage Form
            if (caster->HasAura(SPELL_ALTERED_FORM))
                caster->RemoveAurasDueToSpell(SPELL_ALTERED_FORM);

            caster->CastSpell(caster, SPELL_VISAGE, true);
            caster->SendPlaySpellVisual(caster, SPELL_HATRED, 0, 0, 60, false);
            caster->SetDisplayId(104597);
        }
    }

    void Register()
    {
        OnCast += SpellCastFn(spell_evo_cosmic_visage::HandleOnCast);
    }
};

// 359073 - Eternity Surge (3-stage)
class spell_evo_eternity_surge_base : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_EVOKER_ETERNITY_SURGE_DAMAGE, SPELL_EVOKER_ETERNITYS_SPAN, SPELL_EVOKER_AZURE_SWEEP_BUFF, SPELL_EVOKER_ETERNITY_SURGE_VISUAL });
    }

    void OnComplete(int32 completedStageCount) const
    {
        if (_fired)
            return;
        _fired = true;

        Unit* caster = GetCaster();
        Unit* primaryTarget = GetExplTargetUnit();
        if (!primaryTarget)
            return;

        int32 totalHits = completedStageCount;
        if (caster->HasAura(SPELL_EVOKER_ETERNITYS_SPAN))
            totalHits *= 2;

        caster->CastSpell(caster, SPELL_EVOKER_ETERNITY_SURGE_VISUAL,
            CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
                .SetTriggeringSpell(GetSpell()));

        caster->CastSpell(primaryTarget, SPELL_EVOKER_ETERNITY_SURGE_DAMAGE, CastSpellExtraArgs()
            .SetTriggeringSpell(GetSpell())
            .SetTriggerFlags(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
            .SetCustomArg(totalHits));

        caster->CastSpell(caster, SPELL_EVOKER_AZURE_SWEEP_BUFF,
            CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
                .SetTriggeringSpell(GetSpell()));
    }

    mutable bool _fired = false;

    void Register() override
    {
        OnEmpowerCompleted += SpellOnEmpowerStageCompletedFn(spell_evo_eternity_surge_base::OnComplete);
    }
};

// 382411 - Eternity Surge (4-stage)
class spell_evo_eternity_surge : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_EVOKER_ETERNITY_SURGE_DAMAGE, SPELL_EVOKER_ETERNITYS_SPAN, SPELL_EVOKER_AZURE_SWEEP_BUFF, SPELL_EVOKER_ETERNITY_SURGE_VISUAL });
    }

    void OnComplete(int32 completedStageCount) const
    {
        if (_fired)
            return;
        _fired = true;

        Unit* caster = GetCaster();
        Unit* primaryTarget = GetExplTargetUnit();
        if (!primaryTarget)
            return;

        int32 totalHits = completedStageCount;
        if (caster->HasAura(SPELL_EVOKER_ETERNITYS_SPAN))
            totalHits *= 2;

        caster->CastSpell(caster, SPELL_EVOKER_ETERNITY_SURGE_VISUAL,
            CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
                .SetTriggeringSpell(GetSpell()));

        caster->CastSpell(primaryTarget, SPELL_EVOKER_ETERNITY_SURGE_DAMAGE, CastSpellExtraArgs()
            .SetTriggeringSpell(GetSpell())
            .SetTriggerFlags(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
            .SetCustomArg(totalHits));

        caster->CastSpell(caster, SPELL_EVOKER_AZURE_SWEEP_BUFF,
            CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
                .SetTriggeringSpell(GetSpell()));
    }
	
	mutable bool _fired = false;
	
    void Register() override
    {
        OnEmpowerCompleted += SpellOnEmpowerStageCompletedFn(spell_evo_eternity_surge::OnComplete);
    }
};

// 444089 - Consume Flame damage
class spell_evo_consume_flame_damage : public SpellScript
{
    void ApplyFlatDamage(SpellEffectInfo const& /*effInfo*/, Unit* /*victim*/, int32& /*damage*/, int32& flatMod, float& /*pctMod*/) const
    {
        if (int32 const* flat = std::any_cast<int32>(&GetSpell()->m_customArg))
            flatMod += *flat;
    }

    void Register() override
    {
        CalcDamage += SpellCalcDamageFn(spell_evo_consume_flame_damage::ApplyFlatDamage);
    }
};

// 356995 - Disintegrate
class spell_evo_disintegrate : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_EVOKER_CONSUME_FLAME, SPELL_EVOKER_CONSUME_FLAME_DAMAGE });
    }

    void OnTick(AuraEffect const* /*aurEff*/) const
    {
        Unit* caster = GetCaster();
        if (!caster || !caster->HasAura(SPELL_EVOKER_CONSUME_FLAME))
            return;

        AuraEffect const* consumeEff = caster->GetAuraEffect(SPELL_EVOKER_CONSUME_FLAME, EFFECT_1);
        if (!consumeEff)
            return;

        if (Unit* target = GetUnitOwner())
            ConsumeFireBreath(caster, target, consumeEff->GetAmount());
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_evo_disintegrate::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
    }
};

// 370821 - Scintillation
class spell_evo_scintillation : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_EVOKER_SCINTILLATION_PROC, SPELL_EVOKER_ETERNITY_SURGE_VISUAL });
    }

    bool CheckProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo) const
    {
        SpellInfo const* spellInfo = eventInfo.GetSpellInfo();
        return spellInfo && spellInfo->Id == SPELL_EVOKER_DISINTEGRATE_DAMAGE;
    }

    void HandleProc(AuraEffect* aurEff, ProcEventInfo& eventInfo)
    {
        Unit* caster = GetTarget();
        Unit* target = eventInfo.GetActionTarget();
        if (!target)
            return;

        if (!roll_chance(GetEffect(EFFECT_1)->GetAmount()))
            return;

        caster->CastSpell(caster, SPELL_EVOKER_ETERNITY_SURGE_VISUAL,
            CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR));

        float powerPct = aurEff->GetAmount() / 100.0f;
        caster->CastSpell(target, SPELL_EVOKER_SCINTILLATION_PROC,
            CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
                .SetCustomArg(powerPct));
    }

    void Register() override
    {
        DoCheckEffectProc += AuraCheckEffectProcFn(spell_evo_scintillation::CheckProc, EFFECT_0, SPELL_AURA_DUMMY);
        OnEffectProc += AuraEffectProcFn(spell_evo_scintillation::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// 431192 - Eternity Surge (Scintillation proc)
class spell_evo_scintillation_proc : public SpellScript
{
    void ApplyPowerPct(SpellEffectInfo const& /*effInfo*/, Unit* /*victim*/, int32& /*damage*/, int32& /*flatMod*/, float& pctMod) const
    {
        if (float const* pct = std::any_cast<float>(&GetSpell()->m_customArg))
            pctMod *= *pct;
    }

    void Register() override
    {
        CalcDamage += SpellCalcDamageFn(spell_evo_scintillation_proc::ApplyPowerPct);
    }
};

// 359077 - Eternity Surge damage
class spell_evo_eternity_surge_damage : public SpellScript
{
    void FilterChainTargets(std::list<WorldObject*>& targets) const
    {
        int32 const* totalHits = std::any_cast<int32>(&GetSpell()->m_customArg);
        if (!totalHits || *totalHits <= 1)
        {
            targets.clear();
            return;
        }
        if ((int32)targets.size() > *totalHits - 1)
            Trinity::Containers::RandomResize(targets, uint32(*totalHits - 1));
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_evo_eternity_surge_damage::FilterChainTargets, EFFECT_1, TARGET_UNIT_TARGET_ENEMY);
    }
};

// 1265872 - Azure Sweep
class spell_evo_azure_sweep : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_EVOKER_AZURE_SWEEP });
    }

    void FilterTargets(std::list<WorldObject*>& targets) const
    {
        uint32 maxTargets = uint32(GetEffectInfo(EFFECT_0).CalcValue(GetCaster()));
        if (targets.size() > maxTargets)
            Trinity::Containers::RandomResize(targets, maxTargets);
    }

    void ConsumeCharge()
    {
        if (Aura* buff = GetCaster()->GetAura(SPELL_EVOKER_AZURE_SWEEP_BUFF))
            buff->ModStackAmount(-1);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_evo_azure_sweep::FilterTargets, EFFECT_1, TARGET_UNIT_DEST_AREA_ENEMY);
        AfterCast += SpellCastFn(spell_evo_azure_sweep::ConsumeCharge);
    }
};

// 358385 - Landslide
class spell_evo_landslide : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_EVOKER_LANDSLIDE_ROOT });
    }

    void HandleCast()
    {
        Unit* caster = GetCaster();
        WorldLocation const* dest = GetExplTargetDest();
        if (!dest)
            return;

        float dist = caster->GetExactDist(*dest);
        if (dist < 0.5f)
            return;

        float angle = std::atan2(dest->GetPositionY() - caster->GetPositionY(), dest->GetPositionX() - caster->GetPositionX());

        // 355689 EFFECT_0 TargetBRadius = 6y, so space casts ~6y apart to fill the path
        static constexpr float SPACING = 6.0f;
        int32 numCasts = std::max(1, static_cast<int32>(dist / SPACING));
        float stepX = (dest->GetPositionX() - caster->GetPositionX()) / dist * SPACING;
        float stepY = (dest->GetPositionY() - caster->GetPositionY()) / dist * SPACING;
        float stepZ = (dest->GetPositionZ() - caster->GetPositionZ()) / dist * SPACING;

        for (int32 i = 1; i <= numCasts; ++i)
        {
            float x = caster->GetPositionX() + stepX * i;
            float y = caster->GetPositionY() + stepY * i;
            float z = caster->GetPositionZ() + stepZ * i;

            caster->CastSpell(Position(x, y, z, angle), SPELL_EVOKER_LANDSLIDE_ROOT,
                CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
                    .SetTriggeringSpell(GetSpell()));
        }
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_evo_landslide::HandleCast);
    }
};

// 357210 - Deep Breath (Hackfix) known issues: can cast other spells during the animation
class spell_evo_deep_breath : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_EVOKER_DEEP_BREATH, SPELL_EVOKER_DEEP_BREATH_INFLIGHT });
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        WorldLocation const* destPtr = GetExplTargetDest();
        if (!destPtr)
            return;
        WorldLocation dest = *destPtr;

        // Fly to destination ? hackfixes 362010's JUMP_CHARGE (effect 254)
        constexpr float Speed = 19.0f;
        float distance = caster->GetExactDist2d(dest.GetPositionX(), dest.GetPositionY());
        Milliseconds travelMs{ uint32((distance / Speed) * 1000.0f) };

        caster->m_Events.AddEventAtOffset([caster, dest]()
        {
            if (caster->IsAlive())
                caster->GetMotionMaster()->MoveCharge(dest.GetPositionX(), dest.GetPositionY(), dest.GetPositionZ(), Speed, EVENT_CHARGE);
        }, 800ms);

        caster->m_Events.AddEventAtOffset([caster]()
        {
            if (!caster->IsAlive())
                return;
            caster->RemoveAurasDueToSpell(SPELL_EVOKER_DEEP_BREATH);
            caster->AddAura(SPELL_EVOKER_DEEP_BREATH_INFLIGHT, caster);
        }, 800ms);

        caster->m_Events.AddEventAtOffset([caster]()
        {
            if (caster->IsAlive())
                caster->RemoveAurasDueToSpell(SPELL_EVOKER_DEEP_BREATH_INFLIGHT);
        }, travelMs + 800ms);
    }

    void Register() override
    {
        OnEffectHit += SpellEffectFn(spell_evo_deep_breath::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

void AddSC_evoker_spell_scripts()
{
    RegisterSpellScript(spell_evo_azure_strike);
    RegisterSpellScript(spell_evo_blessing_of_the_bronze);
    RegisterSpellScript(spell_evo_burnout);
    RegisterSpellScript(spell_evo_call_of_ysera);
    RegisterSpellScript(spell_evo_causality_disintegrate);
    RegisterSpellScript(spell_evo_causality_pyre);
    RegisterSpellScript(spell_evo_charged_blast);
    RegisterAreaTriggerAI(at_evo_emerald_blossom);
    RegisterSpellScript(spell_evo_emerald_blossom_heal);
    RegisterSpellScriptWithArgs(spell_evo_essence_burst_trigger, "spell_evo_azure_essence_burst", SPELL_EVOKER_AZURE_ESSENCE_BURST);
    RegisterSpellScriptWithArgs(spell_evo_essence_burst_trigger, "spell_evo_ruby_essence_burst", SPELL_EVOKER_RUBY_ESSENCE_BURST);
    RegisterAreaTriggerAI(at_evo_firestorm);
    RegisterSpellScript(spell_evo_fire_breath);
    RegisterSpellScript(spell_evo_fire_breath_damage);
    RegisterSpellScript(spell_evo_glide);
    RegisterSpellScript(spell_evo_living_flame);
    RegisterSpellScript(spell_evo_panacea);
    RegisterSpellScript(spell_evo_permeating_chill);
    RegisterSpellScript(spell_evo_pyre);
	RegisterSpellScript(spell_evo_pyre_damage);
    RegisterSpellScript(spell_evo_consume_flame_damage);
    RegisterSpellScript(spell_evo_ruby_embers);
    RegisterSpellScript(spell_evo_scouring_flame);
    RegisterSpellScript(spell_evo_snapfire);
    RegisterSpellScript(spell_evo_snapfire_bonus_damage);
    RegisterSpellScript(spell_evo_verdant_embrace);
    RegisterSpellScript(spell_evo_verdant_embrace_trigger_heal);

    //new + doom fixes
    RegisterSpellScript(spell_evo_soar);
    RegisterSpellScript(spell_evo_cosmic_visage);
	RegisterSpellScript(spell_evo_scintillation);
    RegisterSpellScript(spell_evo_scintillation_proc);
    RegisterSpellScript(spell_evo_eternity_surge_base);
    RegisterSpellScript(spell_evo_eternity_surge);
	RegisterSpellScript(spell_evo_eternity_surge_damage);
    RegisterSpellScript(spell_evo_azure_sweep);
    RegisterSpellScript(spell_evo_landslide);
    RegisterSpellScript(spell_evo_deep_breath);
    RegisterSpellScript(spell_evo_risen_fury);
    RegisterSpellScript(spell_evo_rising_fury);
    RegisterSpellScript(spell_evo_dragonrage);
    RegisterSpellScript(spell_evo_dragonrage_trigger);
    RegisterSpellScript(spell_evo_essence_burst_consumed);
    RegisterSpellScript(spell_evo_disintegrate);
}
