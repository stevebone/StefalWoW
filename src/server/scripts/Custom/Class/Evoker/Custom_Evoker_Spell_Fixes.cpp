/*
 * This file is part of the Stefal WoW Project.
 * It is designed to work exclusively with the TrinityCore framework.
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
 * This code is provided for personal and educational use within the
 * Stefal WoW Project. It is not intended for commercial distribution,
 * resale, or any form of monetization.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "Unit.h"
#include "Player.h"
#include "Creature.h"
#include "MotionMaster.h"
#include "MovementGenerator.h"
#include "ObjectAccessor.h"
#include "Object.h"
#include "Position.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

#include "Containers.h"
#include "Cell.h"
#include "GridNotifiers.h"
#include "EventMap.h"

#include "Custom_Evoker_Spell_Defines.h"

namespace Scripts::Custom::Evoker
{
    // Helpers
    static void ConsumeFireBreath(Unit* caster, Unit* target, int32 consumeMs)
    {
        AuraEffect const* consumeFlame = caster->GetAuraEffect(Spells::spell_evoker_consume_flame, EFFECT_3);
        if (!consumeFlame)
            return;

        Aura* fireBreath = target->GetAura(Spells::spell_evoker_fire_breath_dot, caster->GetGUID());
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
        int32 current = fireBreath->GetDuration();
        int32 newDuration = current - consumed;

        fireBreath->SetDuration(newDuration);

        if (newDuration <= 0)
            fireBreath->Remove();

        static constexpr int32 FIRE_BREATH_PERIOD = 2000;
        float ticks = float(consumed) / float(FIRE_BREATH_PERIOD);
        int32 damage = int32(ticks * tickAmount * float(detonationPct) / 100.0f);
        if (damage <= 0)
            return;

        caster->CastSpell(target, Spells::spell_evoker_consume_flame_damage,
            CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
            .SetCustomArg(damage));
    }

    // 1265872 - Azure Sweep
    class spell_evo_azure_sweep : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::spell_evoker_azure_sweep });
        }

        void FilterTargets(std::list<WorldObject*>& targets) const
        {
            uint32 maxTargets = uint32(GetEffectInfo(EFFECT_0).CalcValue(GetCaster()));
            if (targets.size() > maxTargets)
                Trinity::Containers::RandomResize(targets, maxTargets);
        }

        void ConsumeCharge()
        {
            if (Aura* buff = GetCaster()->GetAura(Spells::spell_evoker_azure_sweep_buff))
                buff->ModStackAmount(-1);
        }

        void Register() override
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_evo_azure_sweep::FilterTargets, EFFECT_1, TARGET_UNIT_DEST_AREA_ENEMY);
            AfterCast += SpellCastFn(spell_evo_azure_sweep::ConsumeCharge);
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

    // 351239 - Visage (Racial)
    class spell_evo_cosmic_visage : public SpellScript
    {
        void HandleOnCast()
        {
            Unit* caster = GetCaster();

            if (caster->HasAura(Spells::spell_evoker_visage))
            {
                caster->RemoveAurasDueToSpell(Spells::spell_evoker_visage);
                caster->CastSpell(caster, Spells::spell_evoker_altered_form, true);
                caster->SendPlaySpellVisual(caster, Spells::spell_evoker_hatred, 0, 0, 60, false);
                caster->SetDisplayId(108590);
            }
            else
            {
                if (caster->HasAura(Spells::spell_evoker_altered_form))
                    caster->RemoveAurasDueToSpell(Spells::spell_evoker_altered_form);

                caster->CastSpell(caster, Spells::spell_evoker_visage, true);
                caster->SendPlaySpellVisual(caster, Spells::spell_evoker_hatred, 0, 0, 60, false);
                caster->SetDisplayId(104597);
            }
        }

        void Register()
        {
            OnCast += SpellCastFn(spell_evo_cosmic_visage::HandleOnCast);
        }
    };

    // 357210 - Deep Breath (Hackfix) known issues: can cast other spells during the animation
    class spell_evo_deep_breath : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::spell_evoker_deep_breath, Spells::spell_evoker_deep_breath_inflight });
        }

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            WorldLocation const* destPtr = GetExplTargetDest();
            if (!destPtr)
                return;
            WorldLocation dest = *destPtr;

            // Fly to destination - hackfixes 362010's JUMP_CHARGE (effect 254)
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
                    caster->RemoveAurasDueToSpell(Spells::spell_evoker_deep_breath);
                    caster->AddAura(Spells::spell_evoker_deep_breath_inflight, caster);
                }, 800ms);

            caster->m_Events.AddEventAtOffset([caster]()
                {
                    if (caster->IsAlive())
                        caster->RemoveAurasDueToSpell(Spells::spell_evoker_deep_breath_inflight);
                }, travelMs + 800ms);
        }

        void Register() override
        {
            OnEffectHit += SpellEffectFn(spell_evo_deep_breath::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    // 356995 - Disintegrate
    class spell_evo_disintegrate : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::spell_evoker_consume_flame, Spells::spell_evoker_consume_flame_damage });
        }

        void OnTick(AuraEffect const* /*aurEff*/) const
        {
            Unit* caster = GetCaster();
            if (!caster || !caster->HasAura(Spells::spell_evoker_consume_flame))
                return;

            AuraEffect const* consumeEff = caster->GetAuraEffect(Spells::spell_evoker_consume_flame, EFFECT_1);
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

    // 375087 - Dragonrage
    class spell_evo_dragonrage : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::spell_evoker_rising_fury_talent, Spells::spell_evoker_rising_fury,
                Spells::spell_evoker_risen_fury_talent, Spells::spell_evoker_risen_fury });
        }

        void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/) const
        {
            Unit* target = GetTarget();
            if (target->HasAura(Spells::spell_evoker_rising_fury_talent))
                target->CastSpell(target, Spells::spell_evoker_rising_fury,
                    CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR));
        }

        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/) const
        {
            Unit* target = GetTarget();

            if (target->HasAura(Spells::spell_evoker_risen_fury_talent))
            {
                if (Aura const* risingFury = target->GetAura(Spells::spell_evoker_rising_fury))
                {
                    int32 stacks = risingFury->GetStackAmount();
                    AuraEffect const* risenEff = target->GetAuraEffect(Spells::spell_evoker_risen_fury_talent, EFFECT_0);
                    int32 durationMs = stacks * (risenEff ? risenEff->GetAmount() : 4000);
                    target->CastSpell(target, Spells::spell_evoker_risen_fury,
                        CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
                        .AddSpellMod(SPELLVALUE_DURATION, durationMs));
                }
            }

            target->RemoveAura(Spells::spell_evoker_rising_fury);
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
            return ValidateSpellInfo({ Spells::spell_evoker_pyre_damage });
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
                caster->CastSpell(target->GetPosition(), Spells::spell_evoker_pyre_damage,
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
            return ValidateSpellInfo({ Spells::spell_evoker_twin_flame, Spells::spell_evoker_twin_flame_damage, Spells::spell_evoker_twin_flame_heal });
        }

        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_DEFAULT)
                return;

            Unit* caster = GetTarget();
            if (!caster->HasAura(Spells::spell_evoker_twin_flame))
                return;

            Unit* target = caster->GetVictim();
            if (!target)
                target = ObjectAccessor::GetUnit(*caster, caster->GetTarget());
            if (!target)
                return;

            Player const* player = caster->ToPlayer();
            uint32 twinSpell = (player && player->GetPrimarySpecialization() == ChrSpecialization(1468))
                ? Spells::spell_evoker_twin_flame_heal
                : Spells::spell_evoker_twin_flame_damage;

            caster->CastSpell(target, twinSpell,
                CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR));
        }

        void Register() override
        {
            AfterEffectRemove += AuraEffectRemoveFn(spell_evo_essence_burst_consumed::OnRemove, EFFECT_0, SPELL_AURA_ADD_PCT_MODIFIER, AURA_EFFECT_HANDLE_REAL);
        }
    };

    // 358385 - Landslide
    class spell_evo_landslide : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::spell_evoker_landslide_root });
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

                caster->CastSpell(Position(x, y, z, angle), Spells::spell_evoker_landslide_root,
                    CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
                    .SetTriggeringSpell(GetSpell()));
            }
        }

        void Register() override
        {
            AfterCast += SpellCastFn(spell_evo_landslide::HandleCast);
        }
    };

    // 357212 - Pyre damage
    class spell_evo_pyre_damage : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::spell_evoker_consume_flame, Spells::spell_evoker_consume_flame_damage });
        }

        void HandleHit(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            if (!caster->HasAura(Spells::spell_evoker_consume_flame))
                return;

            AuraEffect const* consumeEff = caster->GetAuraEffect(Spells::spell_evoker_consume_flame, EFFECT_2);
            if (!consumeEff)
                return;

            ConsumeFireBreath(caster, GetHitUnit(), consumeEff->GetAmount());
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_evo_pyre_damage::HandleHit, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        }
    };
    
    // 1271799 - Risen Fury
    class spell_evo_risen_fury : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::spell_evoker_essence_burst });
        }

        void OnTick(AuraEffect const* /*aurEff*/) const
        {
            GetTarget()->CastSpell(GetTarget(), Spells::spell_evoker_essence_burst,
                CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR));
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_evo_risen_fury::OnTick, EFFECT_3, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    // 1271783 - Rising Fury
    class spell_evo_rising_fury : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::spell_evoker_rising_fury });
        }

        void OnTick(AuraEffect const* /*aurEff*/) const
        {
            GetTarget()->CastSpell(GetTarget(), Spells::spell_evoker_rising_fury,
                CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR));
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_evo_rising_fury::OnTick, EFFECT_3, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    // 382411 - Eternity Surge (4-stage)
    class spell_evo_eternity_surge : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::spell_evoker_eternity_surge_damage, Spells::spell_evoker_eternitys_span,
                Spells::spell_evoker_azure_sweep_buff, Spells::spell_evoker_eternity_surge_visual });
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
            if (caster->HasAura(Spells::spell_evoker_eternitys_span))
                totalHits *= 2;

            caster->CastSpell(caster, Spells::spell_evoker_eternity_surge_visual,
                CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
                .SetTriggeringSpell(GetSpell()));

            caster->CastSpell(primaryTarget, Spells::spell_evoker_eternity_surge_damage, CastSpellExtraArgs()
                .SetTriggeringSpell(GetSpell())
                .SetTriggerFlags(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
                .SetCustomArg(totalHits));

            caster->CastSpell(caster, Spells::spell_evoker_azure_sweep_buff,
                CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
                .SetTriggeringSpell(GetSpell()));
        }

        mutable bool _fired = false;

        void Register() override
        {
            OnEmpowerCompleted += SpellOnEmpowerStageCompletedFn(spell_evo_eternity_surge::OnComplete);
        }
    };

    // 359073 - Eternity Surge (3-stage)
    class spell_evo_eternity_surge_base : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::spell_evoker_eternity_surge_damage, Spells::spell_evoker_eternitys_span,
                Spells::spell_evoker_azure_sweep_buff, Spells::spell_evoker_eternity_surge_visual });
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
            if (caster->HasAura(Spells::spell_evoker_eternitys_span))
                totalHits *= 2;

            caster->CastSpell(caster, Spells::spell_evoker_eternity_surge_visual,
                CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
                .SetTriggeringSpell(GetSpell()));

            caster->CastSpell(primaryTarget, Spells::spell_evoker_eternity_surge_damage, CastSpellExtraArgs()
                .SetTriggeringSpell(GetSpell())
                .SetTriggerFlags(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
                .SetCustomArg(totalHits));

            caster->CastSpell(caster, Spells::spell_evoker_azure_sweep_buff,
                CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
                .SetTriggeringSpell(GetSpell()));
        }

        mutable bool _fired = false;

        void Register() override
        {
            OnEmpowerCompleted += SpellOnEmpowerStageCompletedFn(spell_evo_eternity_surge_base::OnComplete);
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

    // 370821 - Scintillation
    class spell_evo_scintillation : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::spell_evoker_scintillation_proc, Spells::spell_evoker_eternity_surge_visual });
        }

        bool CheckProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo) const
        {
            SpellInfo const* spellInfo = eventInfo.GetSpellInfo();
            return spellInfo && spellInfo->Id == Spells::spell_evoker_disintegrate_damage;
        }

        void HandleProc(AuraEffect* aurEff, ProcEventInfo& eventInfo)
        {
            Unit* caster = GetTarget();
            Unit* target = eventInfo.GetActionTarget();
            if (!target)
                return;

            if (!roll_chance(GetEffect(EFFECT_1)->GetAmount()))
                return;

            caster->CastSpell(caster, Spells::spell_evoker_eternity_surge_visual,
                CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR));

            float powerPct = aurEff->GetAmount() / 100.0f;
            caster->CastSpell(target, Spells::spell_evoker_scintillation_proc,
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
}

void AddSC_custom_evoker_spell_fixes()
{
    using namespace Scripts::Custom::Evoker;

    RegisterSpellScript(spell_evo_azure_sweep);
    RegisterSpellScript(spell_evo_consume_flame_damage);
    RegisterSpellScript(spell_evo_cosmic_visage);
    RegisterSpellScript(spell_evo_deep_breath);
    RegisterSpellScript(spell_evo_disintegrate);
    RegisterSpellScript(spell_evo_dragonrage);
    RegisterSpellScript(spell_evo_dragonrage_trigger);
    RegisterSpellScript(spell_evo_essence_burst_consumed);
    RegisterSpellScript(spell_evo_eternity_surge);
    RegisterSpellScript(spell_evo_eternity_surge_base);
    RegisterSpellScript(spell_evo_eternity_surge_damage);
    RegisterSpellScript(spell_evo_landslide);
    RegisterSpellScript(spell_evo_pyre_damage);
    RegisterSpellScript(spell_evo_risen_fury);
    RegisterSpellScript(spell_evo_rising_fury);
    RegisterSpellScript(spell_evo_scintillation);
    RegisterSpellScript(spell_evo_scintillation_proc);
    RegisterSpellScript(spell_evo_soar);
}
