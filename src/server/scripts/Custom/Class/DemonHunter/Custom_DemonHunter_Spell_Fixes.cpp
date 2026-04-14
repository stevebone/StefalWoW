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

#include "AreaTriggerAI.h"
#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "Unit.h"
#include "Player.h"
#include "Creature.h"
#include "Map.h"
#include "MotionMaster.h"
#include "MovementGenerator.h"
#include "ObjectAccessor.h"
#include "Object.h"
#include "Position.h"
#include "SpellHistory.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

#include "Containers.h"
#include "Cell.h"
#include "GridNotifiers.h"
#include "EventMap.h"
#include "TaskScheduler.h"

#include "Custom_DemonHunter_Spell_Defines.h"

namespace Scripts::Custom::DemonHunter
{
    class spell_dh_feast_of_souls_tracker : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::spell_dh_feast_of_souls_buff });
        }

        void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            GetTarget()->RemoveAura(Spells::spell_dh_feast_of_souls_buff);
        }

    public:
        void AddStack(Unit* unit)
        {
            Player* player = unit ? unit->ToPlayer() : nullptr;
            if (!player)
                return;

            // Compute effective duration: 10s base + Sweet Suffering flat modifier
            int32 stackDuration = 10000;
            if (SpellInfo const* buffInfo = sSpellMgr->GetSpellInfo(Spells::spell_dh_feast_of_souls_buff, DIFFICULTY_NONE))
                player->ApplySpellMod(buffInfo, SpellModOp::Duration, stackDuration);
            stackDuration = std::max(stackDuration, 1000);

            if (Aura* buff = player->GetAura(Spells::spell_dh_feast_of_souls_buff))
            {
                buff->SetStackAmount(std::min<uint8>(buff->GetStackAmount() + 1, 20));
                buff->SetMaxDuration(stackDuration);
                buff->SetDuration(stackDuration);
            }
            else
            {
                player->CastSpell(player, Spells::spell_dh_feast_of_souls_buff,
                    TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
                if (Aura* buff = player->GetAura(Spells::spell_dh_feast_of_souls_buff))
                {
                    buff->SetMaxDuration(stackDuration);
                    buff->SetDuration(stackDuration);
                }
            }

            player->m_Events.AddEventAtOffset([playerGUID = player->GetGUID()]()
                {
                    if (Player* p = ObjectAccessor::FindConnectedPlayer(playerGUID))
                        if (Aura* buff = p->GetAura(Spells::spell_dh_feast_of_souls_buff))
                        {
                            uint8 stacks = buff->GetStackAmount();
                            if (stacks <= 1)
                                p->RemoveAura(Spells::spell_dh_feast_of_souls_buff);
                            else
                                buff->SetStackAmount(stacks - 1);
                        }
                }, Milliseconds(stackDuration));
        }

        void Register() override
        {
            AfterEffectRemove += AuraEffectRemoveFn(spell_dh_feast_of_souls_tracker::HandleRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    // Helpers
    static void CollectSoulFragments(Unit* caster, uint32 maxCount, Spell const* triggeringSpell = nullptr)
    {
        std::vector<AreaTrigger*> fragments = caster->GetAreaTriggers(Spells::spell_dh_soul_fragment_devourer);
        uint32 collected = 0;
        for (AreaTrigger* at : fragments)
        {
            if (collected >= maxCount)
                break;

            CastSpellExtraArgs args(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
            if (triggeringSpell)
                args.SetTriggeringSpell(triggeringSpell);
            caster->CastSpell(at->GetPosition(), Spells::spell_dh_consume_soul_devourer, args);

            if (caster->HasAura(Spells::spell_dh_void_metamorphosis_passive) && !caster->HasAura(Spells::spell_dh_void_metamorphosis_buff))
                caster->CastSpell(caster, Spells::spell_dh_void_metamorphosis_counter,
                    TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
            if (caster->HasAura(Spells::spell_dh_void_metamorphosis_buff) && caster->HasAura(Spells::spell_dh_collapsing_star_passive))
                caster->CastSpell(caster, Spells::spell_dh_collapsing_star_counter,
                    TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
            if (caster->HasAura(Spells::spell_dh_void_metamorphosis_buff) && caster->HasAura(Spells::spell_dh_emptiness_passive))
                caster->CastSpell(caster, Spells::spell_dh_emptiness_buff,
                    TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
            if (Aura* fos = caster->GetAura(Spells::spell_dh_feast_of_souls_passive))
                if (auto* script = fos->GetScript<spell_dh_feast_of_souls_tracker>())
                    script->AddStack(caster);

            at->Remove();
            ++collected;
        }
    }

    // 1253304 - Voidfall passive: Consume hit has 35% chance to grant a stack of 1256301 (max 3)
    class spell_dh_voidfall_passive : public AuraScript
    {
        uint32 _meteorCount = 0;

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::spell_dh_voidfall_stack });
        }

        static bool CheckProc(AuraScript const& script, ProcEventInfo const& eventInfo)
        {
            if (!eventInfo.GetSpellInfo())
                return false;
            int32 chance = static_cast<spell_dh_voidfall_passive const&>(script).GetEffect(EFFECT_2)->GetAmount();
            return roll_chance(chance);
        }

        void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo const& /*eventInfo*/) const
        {
            GetTarget()->CastSpell(GetTarget(), Spells::spell_dh_voidfall_stack,
                TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
        }

    public:
        uint32 IncrementMeteorCount() { return ++_meteorCount; }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_dh_voidfall_passive::CheckProc);
            OnEffectProc += AuraEffectProcFn(spell_dh_voidfall_passive::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    class event_dh_dark_matter_meteor : public BasicEvent
    {
    public:
        event_dh_dark_matter_meteor(Unit* caster, Position dest, uint32 spellId)
            : _casterGUID(caster->GetGUID()), _dest(dest), _spellId(spellId) {
        }

        bool Execute(uint64 /*execTime*/, uint32 /*diff*/) override
        {
            if (Player* caster = ObjectAccessor::FindConnectedPlayer(_casterGUID))
                caster->CastSpell(_dest, _spellId,
                    TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
            return true;
        }

    private:
        ObjectGuid _casterGUID;
        Position _dest;
        uint32 _spellId;
    };

    static void SetVoidMetaReapOverride(Unit* target, bool enable)
    {
        if (Aura* metaBuff = target->GetAura(Spells::spell_dh_void_metamorphosis_buff))
            for (AuraEffect* eff : metaBuff->GetAuraEffects())
                if (eff->GetAuraType() == SPELL_AURA_OVERRIDE_ACTIONBAR_SPELLS && uint32(eff->GetMiscValue()) == Spells::spell_dh_reap)
                {
                    eff->ChangeAmount(enable ? Spells::spell_dh_cull : 0);
                    break;
                }
    }

    static void TryFireVoidfallMeteors(Unit* caster, Unit* target)
    {
        Aura* vfAura = caster->GetAura(Spells::spell_dh_voidfall_passive);
        if (!vfAura)
            return;
        Aura* stack = caster->GetAura(Spells::spell_dh_voidfall_stack);
        if (!stack || stack->GetStackAmount() < 3)
            return;

        bool isVengeance = caster->HasAura(Spells::spell_dh_vengeance_spec) && !caster->HasAura(Spells::spell_dh_devourer_spec);
        bool meteoricFall = caster->HasAura(Spells::spell_dh_meteoric_fall_passive);
        uint32 meteorsToFire = meteoricFall ? stack->GetStackAmount() : 1;
        stack->ModStackAmount(-int32(meteorsToFire));

        bool hasWorldKiller = caster->HasAura(Spells::spell_dh_world_killer_passive);
        auto* vfScript = hasWorldKiller ? vfAura->GetScript<spell_dh_voidfall_passive>() : nullptr;

        for (uint32 i = 0; i < meteorsToFire; ++i)
        {
            uint32 meteorCount = 1;
            if (vfScript)
                meteorCount = vfScript->IncrementMeteorCount();

            bool bigMeteor = hasWorldKiller && (meteorCount % 3 == 0);
            uint32 meteorSpell;
            if (bigMeteor)
                meteorSpell = isVengeance ? Spells::spell_dh_world_killer_meteor_vengeance : Spells::spell_dh_world_killer_meteor_havoc;
            else
                meteorSpell = isVengeance ? Spells::spell_dh_voidfall_meteor_vengeance : Spells::spell_dh_voidfall_meteor_havoc;

            caster->CastSpell(target, meteorSpell,
                TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);

            if (bigMeteor && isVengeance)
                caster->CastSpell(caster, Spells::spell_dh_consume_soul_spawn,
                    TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);

            if (caster->HasAura(Spells::spell_dh_final_hour_passive))
                caster->CastSpell(caster, Spells::spell_dh_final_hour_buff,
                    TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
        }
    }

    //201427 - Annihilation
    class spell_dh_annihilation : public SpellScript
    {

        void HandleHit(SpellMissInfo /*missInfo*/)
        {
            if (Unit* caster = GetCaster())
            {
                Unit* target = caster->GetVictim();
                if (!target)
                    return;

                float attackPower = caster->GetTotalAttackPowerValue(BASE_ATTACK) + 28.7f;
                float damage = GetHitDamage();

                SetHitDamage(damage + attackPower);

                if (roll_chance(20))
                    caster->ModifyPower(POWER_FURY, +20);
            }
        }

        void Register() override
        {
            BeforeHit += BeforeSpellHitFn(spell_dh_annihilation::HandleHit);
        }
    };

    // Awaken the Demon - 205598
    class spell_dh_awaken_the_demon : public AuraScript
    {
        void HandleProc(AuraEffect* aurEff, ProcEventInfo& eventInfo)
        {
            Unit* caster = GetCaster();
            if (!caster || !eventInfo.GetDamageInfo())
                return;

            // Ensure EFFECT_1 and EFFECT_2 exist
            if (!GetSpellInfo()->GetEffect(EFFECT_1).IsEffect() ||
                !GetSpellInfo()->GetEffect(EFFECT_2).IsEffect())
                return;

            uint64 threshold1 = caster->CountPctFromMaxHealth(aurEff->GetBaseAmount());
            uint64 threshold2 = caster->CountPctFromMaxHealth(GetSpellInfo()->GetEffect(EFFECT_1).BasePoints);
            int32 duration = GetSpellInfo()->GetEffect(EFFECT_2).BasePoints;

            uint32 damage = eventInfo.GetDamageInfo()->GetDamage();
            uint64 newHealth = caster->GetHealth() - damage;

            // First threshold: trigger CD + extend Meta or apply Meta
            if (newHealth < threshold1)
            {
                if (caster->HasAura(Spells::spell_dh_awaken_the_demon_within_cd))
                    return;

                caster->CastSpell(caster, Spells::spell_dh_awaken_the_demon_within_cd, true);

                if (Aura* aur = caster->GetAura(Spells::spell_dh_metamorphosis_transform))
                {
                    int32 newDuration = std::min(
                        aur->GetDuration() + duration * IN_MILLISECONDS,
                        aur->GetMaxDuration()
                    );
                    aur->SetDuration(newDuration);
                    return;
                }

                if (Aura* aur = caster->AddAura(Spells::spell_dh_metamorphosis_transform, caster))
                    aur->SetDuration(duration * IN_MILLISECONDS);

                return;
            }

            // Second threshold: extend Meta only when crossing downward
            if (caster->GetHealth() > threshold2 && newHealth < threshold2)
            {
                if (Aura* aur = caster->GetAura(Spells::spell_dh_metamorphosis_transform))
                {
                    int32 newDuration = std::min(
                        aur->GetDuration() + duration * IN_MILLISECONDS,
                        aur->GetMaxDuration()
                    );
                    aur->SetDuration(newDuration);
                }
            }
        }

        void Register() override
        {
            OnEffectProc += AuraEffectProcFn(
                spell_dh_awaken_the_demon::HandleProc,
                EFFECT_0,
                SPELL_AURA_DUMMY
            );
        }
    };

    // 203753 - Blade Turning
    class spell_dh_blade_turning : public AuraScript
    {

        bool CheckProc(ProcEventInfo& eventInfo)
        {
            if (eventInfo.GetHitMask() & PROC_HIT_PARRY)
                return true;
            return false;
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_dh_blade_turning::CheckProc);
        }
    };

    // Bloodlet - 206473
    class spell_dh_bloodlet : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::spell_dh_bloodlet_dot });
        }

        bool CheckProc(ProcEventInfo& eventInfo)
        {
            if (eventInfo.GetSpellInfo()->Id == Spells::spell_dh_throw_glaive)
                return true;
            return false;
        }

        void HandleProc(AuraEffect* /*aurEff*/, ProcEventInfo& eventInfo)
        {
            Unit* caster = GetCaster();
            Unit* target = eventInfo.GetActionTarget();
            if (!caster || !target || !eventInfo.GetDamageInfo() || !GetSpellInfo()->GetEffect(EFFECT_0).IsEffect())
                return;

            int32 basePoints = GetSpellInfo()->GetEffect(EFFECT_0).BasePoints;
            int32 dmg = (eventInfo.GetDamageInfo()->GetDamage() * (float)basePoints) / 100.f;
            float dmgPerTick = (float)dmg / 5.f;

            // Any remaining damage must be added
            if (AuraEffect* dot = target->GetAuraEffect(Spells::spell_dh_bloodlet_dot, EFFECT_0, caster->GetGUID()))
            {
                dmgPerTick += (dot->GetAmount() * (dot->GetTotalTicks() - dot->GetTickNumber())) / 5;
            }

            CastSpellExtraArgs args;
            args.AddSpellBP0(dmgPerTick);
            args.SetTriggerFlags(TRIGGERED_FULL_MASK);
            caster->CastSpell(target, Spells::spell_dh_bloodlet_dot, args);
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_dh_bloodlet::CheckProc);
            OnEffectProc += AuraEffectProcFn(spell_dh_bloodlet::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    // 206475 - Chaos Cleave
    class spell_dh_chaos_cleave : public AuraScript
    {

        void OnProc(AuraEffect* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();
            Unit* caster = GetCaster();
            if (!caster || !eventInfo.GetDamageInfo())
                return;

            int32 damage = CalculatePct(eventInfo.GetDamageInfo()->GetDamage(), aurEff->GetAmount());
            caster->CastSpell(caster, Spells::spell_dh_chaos_cleave_proc, CastSpellExtraArgs(TRIGGERED_FULL_MASK).AddSpellBP0(damage));
        }

        void Register() override
        {
            OnEffectProc += AuraEffectProcFn(spell_dh_chaos_cleave::OnProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
        }
    };

    // 162794 - Chaos Strike
    // 201427 - Annihilation
    class spell_dh_chaos_strike_energize : public SpellScript
    {
        bool _didCrit = false; // per-cast state

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({
                Spells::spell_dh_chaos_strike_energize
                });
        }

        void HandleCast()
        {
            Unit* caster = GetCaster();
            Unit* target = GetExplTargetUnit();
            if (!caster || !target)
                return;

            // Determine crit chance once per cast
            float critChance = caster->GetUnitCriticalChanceAgainst(BASE_ATTACK, target);
            _didCrit = roll_chance(critChance);

            // Fire energize spell
            caster->CastSpell(nullptr, Spells::spell_dh_chaos_strike_energize, true);
        }

        void Register() override
        {
            BeforeCast += SpellCastFn(spell_dh_chaos_strike_energize::HandleCast);
        }
    };

    // 1217607 - Void Metamorphosis (active buff)
    class spell_dh_void_metamorphosis_buff : public AuraScript
    {
        void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            SetMaxDuration(-1);
            SetDuration(-1);

            Unit* target = GetTarget();
            target->RemoveAura(Spells::spell_dh_void_metamorphosis_counter);

            if (target->HasAura(Spells::spell_dh_mass_acceleration_passive))
            {
                int32 stacks = 0;

                if (AuraEffect* eff = target->GetAuraEffect(Spells::spell_dh_mass_acceleration_passive, EFFECT_0))
                    stacks = eff->GetAmount();

                for (int32 i = 0; i < stacks; ++i)
                {
                    target->CastSpell(
                        target,
                        Spells::spell_dh_voidfall_stack,
                        TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR
                    );
                }

                target->GetSpellHistory()->ResetCooldown(Spells::spell_dh_reap, true);
            }

            if (target->HasAura(Spells::spell_dh_dark_matter_passive))
                _darkMatterReady = true;

            if (target->HasAura(Spells::spell_dh_midnight_soul_erupt))
            {
                int32 fragments = 0;

                if (AuraEffect* eff = target->GetAuraEffect(Spells::spell_dh_midnight_soul_erupt, EFFECT_0))
                    fragments = eff->GetAmount();

                for (int32 i = 0; i < fragments; ++i)
                {
                    target->CastSpell(
                        target,
                        Spells::spell_dh_consume_soul_spawn,
                        TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR
                    );
                }

                target->CastSpell(
                    target,
                    Spells::spell_dh_collapsing_star_override,
                    TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR
                );
            }
        }


        bool _darkMatterReady = false;
        float _drainAmount = 10.0f;

    public:
        bool ConsumeDarkMatter()
        {
            if (!_darkMatterReady)
                return false;
            _darkMatterReady = false;
            return true;
        }

    private:
        void HandlePeriodic(AuraEffect const* /*aurEff*/)
        {
            Unit* target = GetTarget();

            if (target->HasAura(Spells::spell_dh_void_ray))
                return;

            // Sniff: 20 Fury/sec base, +2.5/sec per tick (EFFECT_10 BP=25 / 10)
            // Collapsing Star channel reduces drain by 70%
            float drain = _drainAmount;
            if (target->FindCurrentSpellBySpellId(Spells::spell_dh_collapsing_star))
                drain *= 0.3f;
            target->ModifyPower(POWER_FURY, -int32(drain));
            _drainAmount += 1.5f;

            if (target->GetPower(POWER_FURY) <= 0)
            {
                target->SetPower(POWER_FURY, 0);
                Remove();
            }
        }

        void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* target = GetTarget();

            if (target->HasAura(Spells::spell_dh_rolling_torment_passive))
            {
                uint32 unusedFragments = uint32(target->GetAreaTriggers(Spells::spell_dh_soul_fragment_devourer).size());

                if (Aura const* csCounter = target->GetAura(Spells::spell_dh_collapsing_star_counter))
                    unusedFragments += uint32(csCounter->GetStackAmount());

                if (unusedFragments > 0)
                {
                    int32 furyPerFragment = 0;

                    if (AuraEffect* eff = target->GetAuraEffect(Spells::spell_dh_rolling_torment_passive, EFFECT_0))
                        furyPerFragment = eff->GetAmount();

                    target->ModifyPower(POWER_FURY, furyPerFragment * int32(unusedFragments));

                    for (uint32 i = 0; i < unusedFragments; ++i)
                    {
                        target->CastSpell(
                            target,
                            Spells::spell_dh_rolling_torment_buff,
                            TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR
                        );
                    }
                }
            }

            target->RemoveAura(Spells::spell_dh_void_metamorphosis_counter);
            target->RemoveAura(Spells::spell_dh_collapsing_star_counter);
            target->RemoveAura(Spells::spell_dh_collapsing_star_override);
            target->RemoveAura(Spells::spell_dh_emptiness_buff);
        }

        void PreventBreakableCC(AuraEffect* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
        {
            PreventDefaultAction();
        }

        void Register() override
        {
            AfterEffectApply += AuraEffectApplyFn(spell_dh_void_metamorphosis_buff::HandleApply, EFFECT_6, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_dh_void_metamorphosis_buff::HandlePeriodic, EFFECT_6, SPELL_AURA_PERIODIC_DUMMY);
            AfterEffectRemove += AuraEffectRemoveFn(spell_dh_void_metamorphosis_buff::HandleRemove, EFFECT_6, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
            OnEffectProc += AuraEffectProcFn(spell_dh_void_metamorphosis_buff::PreventBreakableCC, EFFECT_11, SPELL_AURA_TRANSFORM);
            OnEffectProc += AuraEffectProcFn(spell_dh_void_metamorphosis_buff::PreventBreakableCC, EFFECT_12, SPELL_AURA_TRANSFORM);
        }
    };

    // 1221150 - Collapsing Star
    class spell_dh_collapsing_star : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::spell_dh_collapsing_star_damage, Spells::spell_dh_meteor_shower_devourer, Spells::spell_dh_meteor_shower_vengeance });
        }

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            Unit* target = GetHitUnit();
            if (!target)
                return;

            caster->CastSpell(target, Spells::spell_dh_collapsing_star_damage,
                CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
                .SetTriggeringSpell(GetSpell()));

            if (caster->HasAura(Spells::spell_dh_collapsing_star_fragments))
            {
                for (uint32 i = 0; i < 3; ++i)
                    caster->CastSpell(caster, Spells::spell_dh_consume_soul_spawn,
                        CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
                        .SetTriggeringSpell(GetSpell()));
            }

            // Dark Matter: first Collapsing Star after Void Meta fires meteor shower
            if (Aura* metaBuff = caster->GetAura(Spells::spell_dh_void_metamorphosis_buff))
            {
                if (auto* metaScript = metaBuff->GetScript<spell_dh_void_metamorphosis_buff>())
                {
                    if (metaScript->ConsumeDarkMatter())
                    {
                        bool isVengeance = caster->HasAura(Spells::spell_dh_vengeance_spec) &&
                            !caster->HasAura(Spells::spell_dh_devourer_spec);

                        uint32 showerSpell = isVengeance
                            ? Spells::spell_dh_meteor_shower_vengeance
                            : Spells::spell_dh_meteor_shower_devourer;

                        int32 count = 0;

                        if (AuraEffect* eff = caster->GetAuraEffect(Spells::spell_dh_dark_matter_passive, EFFECT_0))
                            count = eff->GetAmount();

                        for (int32 i = 0; i < count; ++i)
                        {
                            float angle = frand(0.0f, float(2 * M_PI));
                            float dist = frand(0.0f, 8.0f);

                            Position dest = target->GetPosition();
                            dest.m_positionX += dist * std::cos(angle);
                            dest.m_positionY += dist * std::sin(angle);

                            caster->m_Events.AddEventAtOffset(
                                new event_dh_dark_matter_meteor(caster, dest, showerSpell),
                                Milliseconds(i * 500)
                            );
                        }
                    }
                }
            }

            caster->RemoveAura(Spells::spell_dh_collapsing_star_counter);
            caster->RemoveAura(Spells::spell_dh_collapsing_star_override);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_dh_collapsing_star::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    // 1221167 - Collapsing Star (passive talent marker)
    class spell_dh_collapsing_star_counter : public AuraScript
    {
        void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (GetStackAmount() >= 30)
                GetTarget()->CastSpell(GetTarget(), Spells::spell_dh_collapsing_star_override,
                    TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
        }

        void Register() override
        {
            AfterEffectApply += AuraEffectApplyFn(spell_dh_collapsing_star_counter::HandleApply, EFFECT_0, SPELL_AURA_SET_ACTION_BUTTON_SPELL_COUNT, AURA_EFFECT_HANDLE_REAPPLY);
        }
    };

    // 210047 - Consume Soul missile
    class spell_dh_consume_soul_missile : public SpellScript
    {
        void HandleHit(SpellEffIndex effIndex)
        {
            PreventHitDefaultEffect(effIndex);
            Unit* caster = GetCaster();
            if (!caster)
                return;

            int32 spellToCast = GetSpellValue()->EffectBasePoints[0];
            caster->CastSpell(caster, spellToCast, true);
        }

        void Register() override
        {
            OnEffectHit += SpellEffectFn(spell_dh_consume_soul_missile::HandleHit, EFFECT_1, SPELL_EFFECT_TRIGGER_MISSILE);
        }
    };

    // 1245453 - Cull (enhanced Reap during Void Metamorphosis)
    class spell_dh_cull : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::spell_dh_cull_damage, Spells::spell_dh_soul_fragment_devourer, Spells::spell_dh_consume_soul_devourer });
        }

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            Unit* target = GetHitUnit();

            if (target)
                caster->CastSpell(target, Spells::spell_dh_cull_damage,
                    CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
                    .SetTriggeringSpell(GetSpell()));

            caster->CastSpell(caster, Spells::spell_dh_reap_fury,
                CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
                .SetTriggeringSpell(GetSpell()));

            CollectSoulFragments(caster, 4, GetSpell());

            if (target)
                TryFireVoidfallMeteors(caster, target);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_dh_cull::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    // 209426 - Darkness absorb
    class spell_dh_darkness_absorb : public AuraScript
    {

        void CalculateAmount(AuraEffect const* /*aurEff*/, SpellEffectValue& amount, bool& /*canBeRecalculated*/)
        {
            amount = -1;
        }

        void OnAbsorb(AuraEffect* /*aurEff*/, DamageInfo& dmgInfo, uint32& absorbAmount)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            // Base chance from the spell
            int32 chance = GetSpellInfo()->GetEffect(EFFECT_1).BasePoints;

            // Add Cover of Darkness bonus
            if (AuraEffect* coverEff = caster->GetAuraEffect(Spells::spell_dh_cover_of_darkness, EFFECT_0))
                chance += coverEff->GetAmount();

            if (roll_chance(chance))
                absorbAmount = dmgInfo.GetDamage();
        }

        void Register() override
        {
            OnEffectAbsorb += AuraEffectAbsorbFn(spell_dh_darkness_absorb::OnAbsorb, EFFECT_0);
            DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_dh_darkness_absorb::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
        }
    };

    // 236189 - Demonic Infusion
    class spell_dh_demonic_infusion : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            // We must ensure Demon Spikes exists, because we access its ChargeCategoryId
            return ValidateSpellInfo({
                Spells::spell_dh_demon_spikes
                });
        }

        void HandleCast()
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            SpellInfo const* demonSpikes = sSpellMgr->GetSpellInfo(Spells::spell_dh_demon_spikes, DIFFICULTY_NONE);
            if (!demonSpikes)
                return;

            uint32 chargeCategory = demonSpikes->ChargeCategoryId;

            caster->GetSpellHistory()->ResetCharges(chargeCategory);
            caster->CastSpell(caster, Spells::spell_dh_demon_spikes, true);
            caster->GetSpellHistory()->ResetCharges(chargeCategory);
        }

        void Register() override
        {
            OnCast += SpellCastFn(spell_dh_demonic_infusion::HandleCast);
        }
    };

    // 205411 - Desperate Instints
    class spell_dh_desperate_instincts : public AuraScript
    {

        void OnProc(AuraEffect* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();
            Unit* caster = GetCaster();
            if (!caster || !eventInfo.GetDamageInfo())
                return;

            if (caster->GetSpellHistory()->HasCooldown(Spells::spell_dh_blur))
                return;

            uint32 triggerOnHealth = caster->CountPctFromMaxHealth(aurEff->GetAmount());
            uint32 currentHealth = caster->GetHealth();
            // Just falling below threshold
            if (currentHealth > triggerOnHealth && (currentHealth - eventInfo.GetDamageInfo()->GetDamage()) <= triggerOnHealth)
                caster->CastSpell(caster, Spells::spell_dh_blur, false);
        }

        void Register() override
        {
            OnEffectProc += AuraEffectProcFn(spell_dh_desperate_instincts::OnProc, EFFECT_0, SPELL_AURA_TRIGGER_SPELL_ON_HEALTH_PCT);
        }
    };

    // 1217610 - Devour (enhanced Consume during Void Metamorphosis)
    class spell_dh_devour : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::spell_dh_reap, Spells::spell_dh_consume_soul_spawn, Spells::spell_dh_shattered_soul_devourer_lesser_right });
        }

        void HandleAfterCast()
        {
            Unit* caster = GetCaster();
            Unit* target = GetExplTargetUnit();
            if (!target)
                return;

            target->CastSpell(caster, Spells::spell_dh_consume_soul_spawn, CastSpellExtraArgsInit{
                .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
                .TriggeringSpell = GetSpell()
                });
            target->CastSpell(caster, Spells::spell_dh_shattered_soul_devourer_lesser_right, CastSpellExtraArgsInit{
                .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
                .TriggeringSpell = GetSpell()
                });

            caster->GetSpellHistory()->ModifyCooldown(Spells::spell_dh_reap, Milliseconds(-1000));
            caster->GetSpellHistory()->ModifyCooldown(Spells::spell_dh_cull, Milliseconds(-1000));
            caster->GetSpellHistory()->ModifyCooldown(Spells::spell_dh_eradicate, Milliseconds(-1000));
        }

        void Register() override
        {
            AfterCast += SpellCastFn(spell_dh_devour::HandleAfterCast);
        }
    };

    // 1265768 - Doomsayer buff: next harmful spell fires 3 meteors at target
    class spell_dh_doomsayer_buff : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::spell_dh_voidfall_meteor_vengeance, Spells::spell_dh_voidfall_meteor_havoc });
        }

        void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo const& eventInfo) const
        {
            Unit* caster = GetTarget();
            Unit* target = eventInfo.GetActionTarget();
            if (!target)
                return;

            bool isVengeance = caster->HasAura(Spells::spell_dh_vengeance_spec) && !caster->HasAura(Spells::spell_dh_devourer_spec);
            int32 meteors = 0;

            if (AuraEffect* eff = caster->GetAuraEffect(Spells::spell_dh_doomsayer_passive, EFFECT_0))
                meteors = eff->GetAmount();

            auto* vfAura = caster->GetAura(Spells::spell_dh_voidfall_passive);
            auto* vfScript = (vfAura && caster->HasAura(Spells::spell_dh_world_killer_passive)) ? vfAura->GetScript<spell_dh_voidfall_passive>() : nullptr;

            for (int32 i = 0; i < meteors; ++i)
            {
                uint32 meteorCount = 1;
                if (vfScript)
                    meteorCount = vfScript->IncrementMeteorCount();

                bool bigMeteor = vfScript && (meteorCount % 3 == 0);
                uint32 meteorSpell;
                if (bigMeteor)
                    meteorSpell = isVengeance ? Spells::spell_dh_world_killer_meteor_vengeance : Spells::spell_dh_world_killer_meteor_havoc;
                else
                    meteorSpell = isVengeance ? Spells::spell_dh_voidfall_meteor_vengeance : Spells::spell_dh_voidfall_meteor_havoc;

                caster->CastSpell(target, meteorSpell,
                    TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);

                if (caster->HasAura(Spells::spell_dh_final_hour_passive))
                    caster->CastSpell(caster, Spells::spell_dh_final_hour_buff,
                        TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
            }
        }

        void Register() override
        {
            OnEffectProc += AuraEffectProcFn(spell_dh_doomsayer_buff::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    // 1253676 - Doomsayer passive: tracks combat entry time for the 10s window
    class spell_dh_doomsayer_passive : public AuraScript
    {
        TimePoint _combatEnterTime = TimePoint::min();

        void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo const& /*eventInfo*/)
        {
            if (_combatEnterTime == TimePoint::min())
                _combatEnterTime = GameTime::Now();
            // Reset if previous combat ended (gap > window means new fight)
            else if (std::chrono::duration_cast<Milliseconds>(GameTime::Now() - _combatEnterTime).count() > 30000)
                _combatEnterTime = GameTime::Now();
        }

    public:
        bool IsWithinCombatWindow(Unit* target) const
        {
            if (_combatEnterTime == TimePoint::min())
                return false;

            int32 multiplier = 0;

            if (AuraEffect* eff = target->GetAuraEffect(Spells::spell_dh_doomsayer_passive, EFFECT_1))
                multiplier = eff->GetAmount();

            int64 windowMs = int64(multiplier) * 5000;

            return std::chrono::duration_cast<Milliseconds>(GameTime::Now() - _combatEnterTime).count() <= windowMs;
        }

        void Register() override
        {
            OnEffectProc += AuraEffectProcFn(spell_dh_doomsayer_passive::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    // 1242504 - Emptiness (0.25% haste per stack, up to 100 stacks = 25%)
    class spell_dh_emptiness_buff : public AuraScript
    {
        float _appliedHaste = 0.0f;

        void HandleCalcAmount(AuraEffect const* /*aurEff*/, SpellEffectValue& amount, bool& canBeRecalculated)
        {
            amount = 0;
            canBeRecalculated = false;
        }

        void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* target = GetTarget();
            float newHaste = 0.25f * float(GetStackAmount());
            if (_appliedHaste > 0.0f)
            {
                target->ApplyCastTimePercentMod(_appliedHaste, false);
                target->ApplyAttackTimePercentMod(BASE_ATTACK, _appliedHaste, false);
                target->ApplyAttackTimePercentMod(OFF_ATTACK, _appliedHaste, false);
                target->ApplyAttackTimePercentMod(RANGED_ATTACK, _appliedHaste, false);
            }
            target->ApplyCastTimePercentMod(newHaste, true);
            target->ApplyAttackTimePercentMod(BASE_ATTACK, newHaste, true);
            target->ApplyAttackTimePercentMod(OFF_ATTACK, newHaste, true);
            target->ApplyAttackTimePercentMod(RANGED_ATTACK, newHaste, true);
            _appliedHaste = newHaste;
        }

        void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (_appliedHaste > 0.0f)
            {
                Unit* target = GetTarget();
                target->ApplyCastTimePercentMod(_appliedHaste, false);
                target->ApplyAttackTimePercentMod(BASE_ATTACK, _appliedHaste, false);
                target->ApplyAttackTimePercentMod(OFF_ATTACK, _appliedHaste, false);
                target->ApplyAttackTimePercentMod(RANGED_ATTACK, _appliedHaste, false);
                _appliedHaste = 0.0f;
            }
        }

        void Register() override
        {
            DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_dh_emptiness_buff::HandleCalcAmount, EFFECT_0, SPELL_AURA_MELEE_SLOW);
            AfterEffectApply += AuraEffectApplyFn(spell_dh_emptiness_buff::HandleApply, EFFECT_0, SPELL_AURA_MELEE_SLOW, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
            AfterEffectRemove += AuraEffectRemoveFn(spell_dh_emptiness_buff::HandleRemove, EFFECT_0, SPELL_AURA_MELEE_SLOW, AURA_EFFECT_HANDLE_REAL);
        }
    };

    // 1261684
    class spell_dh_entropy : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::spell_dh_consume_soul_spawn, Spells::spell_dh_void_metamorphosis_counter });
        }

        void HandlePeriodic(AuraEffect const* /*aurEff*/)
        {
            Unit* target = GetTarget();

            if (target->IsInCombat())
            {
                target->CastSpell(target, Spells::spell_dh_consume_soul_spawn,
                    TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
            }
            else if (target->HasAura(Spells::spell_dh_void_metamorphosis_passive) && !target->HasAura(Spells::spell_dh_void_metamorphosis_buff))
            {
                if (Aura* counter = target->GetAura(Spells::spell_dh_void_metamorphosis_counter))
                {
                    if (counter->GetStackAmount() < 25)
                        target->CastSpell(target, Spells::spell_dh_void_metamorphosis_counter,
                            TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
                }
                else
                    target->CastSpell(target, Spells::spell_dh_void_metamorphosis_counter,
                        TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
            }
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_dh_entropy::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    // 1225826 - Eradicate (replaces Reap/Cull via Moment of Craving)
    class spell_dh_eradicate : public SpellScript
    {
        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            Unit* target = GetHitUnit();
            if (!target)
                return;

            uint32 damageSpell = caster->HasAura(Spells::spell_dh_void_metamorphosis_buff)
                ? Spells::spell_dh_eradicate_damage_meta : Spells::spell_dh_eradicate_damage;
            float angle = caster->GetOrientation();
            Position dest = caster->GetPosition();
            dest.m_positionX += 25.0f * std::cos(angle);
            dest.m_positionY += 25.0f * std::sin(angle);
            caster->CastSpell(dest, damageSpell,
                CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
                .SetTriggeringSpell(GetSpell()));

            caster->CastSpell(caster, Spells::spell_dh_reap_fury,
                CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
                .SetTriggeringSpell(GetSpell()));

            CollectSoulFragments(caster, 10, GetSpell());
            TryFireVoidfallMeteors(caster, target);

            SetVoidMetaReapOverride(caster, true);
            caster->RemoveAura(Spells::spell_dh_moment_of_craving_buff);
            caster->RemoveAura(Spells::spell_dh_eradicate_override);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_dh_eradicate::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    // 1225827/1279200 - Eradicate damage
    class spell_dh_eradicate_damage : public SpellScript
    {
        void FilterTargets(std::list<WorldObject*>& targets)
        {
            Unit* caster = GetCaster();
            targets.remove_if([&](WorldObject* target) -> bool
                {
                    return !caster->IsWithinDist(target, 25.0f) || !caster->HasInArc(float(M_PI) / 2.0f, target);
                });
        }

        void Register() override
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_dh_eradicate_damage::FilterTargets, EFFECT_0, TARGET_UNIT_LINE_CASTER_TO_DEST_ENEMY);
        }
    };

    class event_dh_eye_of_leotheras : public BasicEvent
    {
    public:
        event_dh_eye_of_leotheras(Unit* caster, Unit* target, int32 bp)
            : _caster(caster), _target(target), _bp(bp) { }

        bool Execute(uint64 /*execTime*/, uint32 /*diff*/) override
        {
            if (_caster && _target)
            {
                _caster->CastSpell(
                    _target,
                    Spells::spell_dh_eye_of_leotheras_dmg,
                    CastSpellExtraArgs(TRIGGERED_FULL_MASK).AddSpellBP0(_bp)
                );
            }
            return true;
        }

    private:
        Unit* _caster;
        Unit* _target;
        int32 _bp;
    };

    // Eye of leotheras - 206649
    class spell_dh_eye_of_leotheras : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            if (!sSpellMgr->GetSpellInfo(Spells::spell_dh_eye_of_leotheras_dmg, DIFFICULTY_NONE))
                return false;

            return true;
        }

        bool HandleProc(ProcEventInfo& eventInfo)
        {
            Unit* caster = GetCaster();
            WorldObject* target = GetAura()->GetOwner();
            if (!caster || !target || !eventInfo.GetSpellInfo() || !caster->ToPlayer())
                return false;
            Unit* unitTarget = target->ToUnit();
            if (!unitTarget || eventInfo.GetSpellInfo()->IsPositive())
                return false;

            if (AuraEffect* aurEff = GetAura()->GetEffect(EFFECT_0))
            {
                int32 bp = aurEff->GetAmount();
                GetAura()->RefreshDuration();
                caster->m_Events.AddEvent(
                    new event_dh_eye_of_leotheras(caster, unitTarget, bp),
                    caster->m_Events.CalculateTime(100ms)
                );

                return true;
            }
            return false;
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_dh_eye_of_leotheras::HandleProc);
        }
    };

    // Fel Barrage - 211053
    class spell_dh_fel_barrage : public AuraScript
    {
        int32 _charges = 1;

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            if (!sSpellMgr->GetSpellInfo(Spells::spell_dh_fel_barrage, DIFFICULTY_NONE) ||
                !sSpellMgr->GetSpellInfo(Spells::spell_dh_fel_barrage_dmg, DIFFICULTY_NONE))
                return false;
            return true;
        }

        bool Load() override
        {
            Unit* caster = GetCaster();
            if (!caster || !GetSpellInfo())
                return false;

            uint32 chargeCategoryId = GetSpellInfo()->ChargeCategoryId;
            while (caster->GetSpellHistory()->HasCharge(chargeCategoryId))
            {
                caster->GetSpellHistory()->ConsumeCharge(chargeCategoryId);
                _charges++;
            }
            return true;
        }

        void HandleTrigger(AuraEffect const* /*aurEff*/)
        {
            Unit* caster = GetCaster();
            Unit* target = GetTarget();
            if (!caster || !target)
                return;

            CastSpellExtraArgs args;
            args.AddSpellBP0(_charges);
            args.SetTriggerFlags(TRIGGERED_FULL_MASK);
            caster->CastSpell(target, Spells::spell_dh_fel_barrage_dmg, args);
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_dh_fel_barrage::HandleTrigger, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    // Fel Barrage Damage - 211052
    class spell_dh_fel_barrage_damage : public SpellScript
    {
        void HandleHit()
        {
            int32 chargesUsed = GetSpellValue()->EffectBasePoints[0];
            int32 dmg = GetHitDamage();
            SetHitDamage(float(dmg * chargesUsed) / 5.f);
        }

        void Register() override
        {
            OnHit += SpellHitFn(spell_dh_fel_barrage_damage::HandleHit);
        }
    };

    // Fel Barrage aura - 222703
    class spell_dh_fel_barrage_aura : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::spell_dh_blade_dance, Spells::spell_dh_death_sweep,
                Spells::spell_dh_chaos_strike_mh, Spells::spell_dh_annihilation_mh, Spells::spell_dh_fel_barrage_dmg });
        }

        bool CheckProc(ProcEventInfo& eventInfo)
        {
            // Blade Dance, Chaos Strike and Annihilation have many damagers,
            // so we accept only 1 of those, and we remove the others
            // Also we remove fel barrage itself too.
            if (!eventInfo.GetSpellInfo())
                return false;

            //Blade Dance    //Chaos Strike   //Fel Barrage
            std::vector<uint32> removeSpellIds{ Spells::spell_dh_blade_dance, Spells::spell_dh_death_sweep, Spells::spell_dh_chaos_strike_mh, Spells::spell_dh_annihilation_mh, Spells::spell_dh_fel_barrage_dmg };
            return std::find(removeSpellIds.begin(), removeSpellIds.end(), eventInfo.GetSpellInfo()->Id) == removeSpellIds.end();
        }

        void HandleProc(AuraEffect* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
        {
            PreventDefaultAction();

            Unit* caster = GetCaster();
            if (!caster)
                return;

            int32 chargeCatId = sSpellMgr->GetSpellInfo(Spells::spell_dh_fel_barrage, DIFFICULTY_NONE)->ChargeCategoryId;
            if (chargeCatId > 0)
                caster->GetSpellHistory()->RestoreCharge(chargeCatId);
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_dh_fel_barrage_aura::CheckProc);
            OnEffectProc += AuraEffectProcFn(spell_dh_fel_barrage_aura::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    // 212105 - Fel Devastation damage
    class spell_dh_fel_devastation_damage : public AuraScript
    {
        void PeriodicTick(AuraEffect const* aurEff)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            if (aurEff->GetTickNumber() == 1)
                return;

            caster->CastSpell(caster, Spells::spell_dh_fel_devastation_dmg, true);
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_dh_fel_devastation_damage::PeriodicTick, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        }
    };

    // Fel Eruption - 211881
    class spell_dh_fel_eruption : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::spell_dh_fel_eruption_damage });
        }

        void HandleOnHit()
        {
            Unit* caster = GetCaster();
            Unit* target = GetExplTargetUnit();
            if (!caster || !target)
                return;
            caster->CastSpell(target, Spells::spell_dh_fel_eruption_damage, CastSpellExtraArgs(TRIGGERED_FULL_MASK).AddSpellMod(SPELLVALUE_BASE_POINT1, 1));
        }

        void HandleHit(SpellMissInfo missInfo)
        {
            Unit* caster = GetCaster();
            Unit* target = GetExplTargetUnit();
            if (!caster || !target)
                return;

            if (missInfo == SPELL_MISS_IMMUNE || missInfo == SPELL_MISS_IMMUNE2)
                caster->CastSpell(target, Spells::spell_dh_fel_eruption_damage, CastSpellExtraArgs(TRIGGERED_FULL_MASK).AddSpellMod(SPELLVALUE_BASE_POINT1, 2));
        }

        void Register() override
        {
            BeforeHit += BeforeSpellHitFn(spell_dh_fel_eruption::HandleHit);
            OnHit += SpellHitFn(spell_dh_fel_eruption::HandleOnHit);
        }
    };

    // 206966 - Fel Lance
    class spell_dh_fel_lance : public SpellScript
    {
        void HandleHit(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            Unit* target = GetHitUnit();
            if (!caster || !target)
                return;

            int32 pct = GetSpellInfo()->GetEffect(EFFECT_0).BasePoints;
            SetHitDamage(GetHitDamage() + target->CountPctFromMaxHealth(pct));
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_dh_fel_lance::HandleHit, EFFECT_1, SPELL_EFFECT_DUMMY);
        }
    };

    // 344865 - Fel Rush (specless version used in Mardum)
    class spell_dh_fel_rush_specless : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            // Must validate the real Fel Rush spell we cast
            return ValidateSpellInfo({
                Spells::spell_dh_fel_rush
                });
        }

        void HandleOnCast()
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            // Fire the real Fel Rush
            caster->CastSpell(nullptr, Spells::spell_dh_fel_rush, true);
        }

        void Register() override
        {
            OnCast += SpellCastFn(spell_dh_fel_rush_specless::HandleOnCast);
        }
    };

    // Fel Rush - 195072
    class spell_dh_fel_rush : public SpellScript
    {

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            if (!sSpellMgr->GetSpellInfo(Spells::spell_dh_fel_rush_ground, DIFFICULTY_NONE))
                return false;
            if (!sSpellMgr->GetSpellInfo(Spells::spell_dh_fel_rush_water_air, DIFFICULTY_NONE))
                return false;
            return true;
        }

        void HandleDashGround(SpellEffIndex /*effIndex*/)
        {
            if (Unit* caster = GetCaster())
            {
                if (!caster->IsFalling() || caster->IsInWater())
                {
                    caster->RemoveAurasDueToSpell(Spells::spell_dh_glide);
                    caster->CastSpell(caster, Spells::spell_dh_fel_rush_ground, true);
                    caster->CastSpell(caster, Spells::spell_dh_fel_rush_damage, TRIGGERED_FULL_DEBUG_MASK);

                    if (caster->HasAura(Spells::spell_dh_momentum))
                        caster->CastSpell(nullptr, Spells::spell_dh_momentum_buff, true);
                }
                caster->GetSpellHistory()->AddCooldown(GetSpellInfo()->Id, 0, std::chrono::milliseconds(750));
            }
        }

        void HandleDashAir(SpellEffIndex /*effIndex*/)
        {
            if (Unit* caster = GetCaster())
            {
                if (caster->IsFalling())
                {
                    caster->RemoveAurasDueToSpell(Spells::spell_dh_glide);
                    caster->SetDisableGravity(true);
                    caster->CastSpell(caster, Spells::spell_dh_fel_rush_water_air, true);
                    caster->CastSpell(caster, Spells::spell_dh_fel_rush_damage, TRIGGERED_FULL_DEBUG_MASK);

                    if (caster->HasAura(Spells::spell_dh_momentum))
                        caster->CastSpell(nullptr, Spells::spell_dh_momentum_buff, true);

                    caster->GetSpellHistory()->AddCooldown(GetSpellInfo()->Id, 0, std::chrono::milliseconds(750));
                }
            }
        }

        void HandleCast()
        {
            if (Unit* caster = GetCaster())
                if (caster->HasAura(427794))
                    caster->CastSpell(caster, 427793, true);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_dh_fel_rush::HandleDashGround, EFFECT_0, SPELL_EFFECT_DUMMY);
            OnEffectHitTarget += SpellEffectFn(spell_dh_fel_rush::HandleDashAir, EFFECT_1, SPELL_EFFECT_DUMMY);
        }
    };

    // 197923
    class spell_dh_fel_rush_dash : public SpellScript
    {
        void PreventTrigger(SpellEffIndex effIndex)
        {
            PreventHitEffect(effIndex);
        }

        void Register() override
        {
            OnEffectLaunch += SpellEffectFn(spell_dh_fel_rush_dash::PreventTrigger, EFFECT_6, SPELL_EFFECT_TRIGGER_SPELL);
            OnEffectHit += SpellEffectFn(spell_dh_fel_rush_dash::PreventTrigger, EFFECT_6, SPELL_EFFECT_TRIGGER_SPELL);
        }
    };

    // Fel Rush air - 197923
    class spell_dh_fel_rush_dash_aura : public AuraScript
    {
        void AfterRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (Unit* caster = GetCaster())
                caster->m_Events.AddEventAtOffset(
                    [caster]()
                    {
                        if (!caster->HasAura(Spells::spell_dh_fel_rush_water_air))
                            caster->SetDisableGravity(false);
                    },
                    100ms
                );
        }

        void CalcSpeed(AuraEffect const* /*aurEff*/, SpellEffectValue& amount, bool& /*canBeRecalculated*/)
        {
            amount = 1250;
            RefreshDuration();
        }

        void Register() override
        {
            DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_dh_fel_rush_dash_aura::CalcSpeed, EFFECT_1, SPELL_AURA_MOD_SPEED_NO_CONTROL);
            DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_dh_fel_rush_dash_aura::CalcSpeed, EFFECT_3, SPELL_AURA_MOD_MINIMUM_SPEED);
            AfterEffectRemove += AuraEffectRemoveFn(spell_dh_fel_rush_dash_aura::AfterRemove, EFFECT_9, SPELL_AURA_MOD_MINIMUM_SPEED_RATE, AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK);
        }
    };

    // Fel Rush Damage 223107
    class spell_dh_fel_rush_damage : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo(
                {
                    Spells::spell_dh_fel_mastery,
                    Spells::spell_dh_fel_mastery_fury,
                    Spells::spell_dh_fel_rush_water_air
                }
            );
        }

        bool targetHit;

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            targets.remove(GetCaster());
        }

        void CountTargets(std::list<WorldObject*>& targets)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            targets.clear();

            std::list<Unit*> units;
            Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(caster, caster, 25.0f);
            Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(caster, units, u_check);
            Cell::VisitAllObjects(caster, searcher, 25.0f);

            units.remove_if([caster](Unit* unit)
                {
                    return !caster->HasInLine(unit, 6.f, caster->GetObjectScale());
                });

            for (Unit* unit : units)
                targets.push_back(unit);

            targetHit = !targets.empty();
        }

        void HandleCast()
        {
            if (Unit* caster = GetCaster())
                if (caster->HasAura(Spells::spell_dh_fel_mastery) && targetHit)
                    caster->CastSpell(caster, Spells::spell_dh_fel_mastery_fury, true);
        }

        void HandleOnHit()
        {
            if (GetCaster() && GetHitUnit())
            {
                int32 attackPower = GetCaster()->m_unitData->AttackPower / static_cast<float>(100) * 25.3f;
                SetHitDamage(attackPower);
            }
        }

        void Register() override
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_dh_fel_rush_damage::FilterTargets, EFFECT_0, TARGET_UNIT_RECT_CASTER_ENEMY);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_dh_fel_rush_damage::CountTargets, EFFECT_0, TARGET_UNIT_RECT_CASTER_ENEMY);
            OnCast += SpellCastFn(spell_dh_fel_rush_damage::HandleCast);
            OnHit += SpellHitFn(spell_dh_fel_rush_damage::HandleOnHit);
        }
    };

    // Fel Rush air - 197922
    class spell_dh_fel_rush_dash_ground : public AuraScript
    {
        void AfterRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (Unit* caster = GetCaster())
            {
                caster->SetDisableGravity(false);
            }
        }

        void CalcSpeed(AuraEffect const* /*aurEff*/, SpellEffectValue& amount, bool& /*canBeRecalculated*/)
        {
            amount = 1250;
            RefreshDuration();
        }

        void Register() override
        {
            DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_dh_fel_rush_dash_ground::CalcSpeed, EFFECT_1, SPELL_AURA_MOD_SPEED_NO_CONTROL);
            DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_dh_fel_rush_dash_ground::CalcSpeed, EFFECT_3, SPELL_AURA_MOD_MINIMUM_SPEED);
            AfterEffectRemove += AuraEffectRemoveFn(spell_dh_fel_rush_dash_ground::AfterRemove, EFFECT_6, SPELL_AURA_MOD_MINIMUM_SPEED_RATE, AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK);
        }
    };

    // 209795 - Fracture
    class spell_dh_fracture : public SpellScript
    {
        void HandleHit(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            // (Your logic goes here)
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(
                spell_dh_fracture::HandleHit,
                EFFECT_0,
                SPELL_EFFECT_DUMMY
            );
        }
    };

    // 224509 - Frailty - Spirit Bomb healing accumulator
    class spell_dh_frailty : public AuraScript
    {
        // Per-aura stored accumulated damage
        uint32 _frailtyDamage = 0;

        void OnProc(AuraEffect* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();

            Unit* caster = GetCaster();
            if (!caster || caster != eventInfo.GetActor() || !eventInfo.GetDamageInfo())
                return;

            // Add % of damage taken to accumulator
            uint32 damage = CalculatePct(eventInfo.GetDamageInfo()->GetDamage(), aurEff->GetAmount());
            _frailtyDamage += damage;
        }

        void PeriodicTick(AuraEffect const* /*aurEff*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            if (_frailtyDamage > 0)
            {
                caster->CastSpell(
                    caster,
                    Spells::spell_dh_spirit_bomb_heal,
                    CastSpellExtraArgs(TRIGGERED_FULL_MASK).AddSpellBP0(_frailtyDamage)
                );

                _frailtyDamage = 0;
            }
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(
                spell_dh_frailty::PeriodicTick,
                EFFECT_0,
                SPELL_AURA_PERIODIC_DUMMY
            );

            OnEffectProc += AuraEffectProcFn(
                spell_dh_frailty::OnProc,
                EFFECT_0,
                SPELL_AURA_PERIODIC_DUMMY
            );
        }
    };

    // 1239123 - Hungering Slash
    class spell_dh_hungering_slash : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::spell_dh_hungering_slash_damage, Spells::spell_dh_hungering_slash_fury,
                Spells::spell_dh_hungering_slash_voidstep, Spells::spell_dh_vengeful_retreat_trigger });
        }

        void HandleAfterCast() const
        {
            Unit* caster = GetCaster();

            caster->CastSpell(caster, Spells::spell_dh_hungering_slash_damage,
                CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
                .SetTriggeringSpell(GetSpell()));

            caster->CastSpell(caster, Spells::spell_dh_hungering_slash_fury,
                CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
                .SetTriggeringSpell(GetSpell()));

            uint32 maxFragments = GetSpellInfo()->GetEffect(EFFECT_0).CalcValue(caster);
            std::list<Unit*> nearbyEnemies;
            Trinity::AnyUnfriendlyUnitInObjectRangeCheck check(caster, caster, 20.0f);
            Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(caster, nearbyEnemies, check);
            Cell::VisitAllObjects(caster, searcher, 20.0f);
            nearbyEnemies.sort(Trinity::ObjectDistanceOrderPred(caster));

            uint32 shattered = 0;
            for (Unit* enemy : nearbyEnemies)
            {
                if (shattered >= maxFragments)
                    break;
                enemy->CastSpell(caster, Spells::spell_dh_consume_soul_spawn,
                    CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
                    .SetTriggeringSpell(GetSpell()));
                ++shattered;
            }

            caster->CastSpell(caster, 444929, CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR).SetTriggeringSpell(GetSpell()));

            if (Aura* freeCastAura = caster->GetAura(444929))
            {
                freeCastAura->SetDuration(6000);
                freeCastAura->SetMaxDuration(6000);
            }

            caster->CastSpell(caster, Spells::spell_dh_hungering_slash_voidstep,
                CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
                .SetTriggeringSpell(GetSpell()));

            caster->RemoveAurasDueToSpell(Spells::spell_dh_hungering_slash_override);
        }

        void Register() override
        {
            AfterCast += SpellCastFn(spell_dh_hungering_slash::HandleAfterCast);
        }
    };

    // Immolation Aura - 258920
    class spell_dh_immolation_aura : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({
                Spells::spell_dh_cleansed_by_flame,
                Spells::spell_dh_cleansed_by_flame_dispel,
                Spells::spell_dh_fallout,
                Spells::spell_dh_shattered_souls_havoc
                });
        }

        void HandleCast()
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            if (caster->HasAura(Spells::spell_dh_cleansed_by_flame))
                caster->CastSpell(caster, Spells::spell_dh_cleansed_by_flame_dispel, true);
        }

        void Register() override
        {
            OnCast += SpellCastFn(spell_dh_immolation_aura::HandleCast);
        }
    };

    // Immolation Aura damage - 258922
    class spell_dh_immolation_aura_damage : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({
                Spells::spell_dh_charred_flesh,
                Spells::spell_dh_fiery_brand_debuff_rank_2,
                Spells::spell_dh_fiery_brand_debuff_rank_1
                });
        }

        void HandleHit(SpellEffIndex /*effIndex*/)
        {
            Unit* target = GetHitUnit();
            Unit* caster = GetCaster();
            if (!target || !caster)
                return;

            if (caster->HasAura(Spells::spell_dh_charred_flesh))
            {
                AuraEffect* charredFleshEff = caster->GetAuraEffect(Spells::spell_dh_charred_flesh, EFFECT_0);
                if (!charredFleshEff)
                    return;

                int32 durationMod = charredFleshEff->GetAmount();
                if (!durationMod)
                    return;

                for (uint32 spellId : {
                    Spells::spell_dh_fiery_brand_debuff_rank_2,
                        Spells::spell_dh_fiery_brand_debuff_rank_1 })
                {
                    if (Aura* fieryBrand = target->GetAura(spellId))
                    {
                        int32 newDuration = fieryBrand->GetDuration() + durationMod;
                        int32 newMaxDuration = fieryBrand->GetMaxDuration() + durationMod;

                        fieryBrand->SetDuration(newDuration);
                        fieryBrand->SetMaxDuration(newMaxDuration);
                    }
                }
            }
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(
                spell_dh_immolation_aura_damage::HandleHit,
                EFFECT_0,
                SPELL_EFFECT_SCHOOL_DAMAGE
            );
        }
    };

    class event_dh_infernal_strike : public BasicEvent
    {
    public:
        explicit event_dh_infernal_strike(Unit* caster) : _caster(caster) {}

        bool Execute(uint64 /*execTime*/, uint32 /*diff*/) override
        {
            if (_caster)
            {
                _caster->CastSpell(_caster, Spells::spell_dh_infernal_strike_impact_damage, true);

                if (_caster->HasAura(Spells::spell_dh_rain_of_chaos))
                    _caster->CastSpell(_caster, Spells::spell_dh_rain_of_chaos_impact, true);

                if (_caster->HasAura(Spells::spell_dh_abyssal_strike))
                    _caster->CastSpell(_caster, Spells::spell_dh_sigil_of_flame_flame_crash, true);
            }

            return true;
        }

    private:
        Unit* _caster;
    };

    // Infernal Strike - 189110
    class spell_dh_infernal_strike : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({
                Spells::spell_dh_infernal_strike_jump,
                Spells::spell_dh_infernal_strike_impact_damage
                });
        }

        void HandleOnHit()
        {
            Unit* caster = GetCaster();
            WorldLocation const* dest = GetHitDest();
            Unit* target = GetHitUnit();
            if (!caster || !dest || !target)
                return;

            if (target->IsHostileTo(caster))
            {
                caster->CastSpell(
                    Position(dest->GetPositionX(), dest->GetPositionY(), dest->GetPositionZ()),
                    Spells::spell_dh_infernal_strike_jump,
                    true
                );

                caster->CastSpell(caster, Spells::spell_dh_infernal_strike_visual, true);
            }
        }

        void HandleOnCast()
        {
            if (Unit* caster = GetCaster())
                caster->m_Events.AddEvent(new event_dh_infernal_strike(caster),
                    caster->m_Events.CalculateTime(750ms));
        }

        void Register() override
        {
            OnHit += SpellHitFn(spell_dh_infernal_strike::HandleOnHit);
            OnCast += SpellCastFn(spell_dh_infernal_strike::HandleOnCast);
        }
    };

    // 206891 - Intimidated
    class spell_dh_intimidated : public AuraScript
    {

        void OnProc(AuraEffect* /*aurEff*/, ProcEventInfo& eventInfo)
        {
            Unit* attacker = eventInfo.GetActor();
            WorldObject* auraOwner = GetAura()->GetOwner();
            if (!attacker || !auraOwner)
                return;

            if (attacker == GetCaster())
            {
                RefreshDuration();
                return;
            }

            if (uniqueTargets.size() >= 4 || !auraOwner->ToUnit())
                return;

            if (uniqueTargets.find(attacker->GetGUID()) == uniqueTargets.end())
            {
                attacker->CastSpell(auraOwner->ToUnit(), GetSpellInfo()->Id, true);
                uniqueTargets.insert(attacker->GetGUID());
            }
        }

        void Register() override
        {
            OnEffectProc += AuraEffectProcFn(spell_dh_intimidated::OnProc, EFFECT_0, SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN);
        }

    private:
        GuidSet uniqueTargets;
    };

    // Jagged Spikes - 208796
    class spell_dh_jagged_spikes : public AuraScript
    {
        void HandleProc(AuraEffect* /*aurEff*/, ProcEventInfo& eventInfo)
        {
            Unit* caster = GetCaster();
            Unit* target = eventInfo.GetActor();
            if (!caster || !eventInfo.GetDamageInfo())
                return;

            if (caster->IsFriendlyTo(target))
                return;

            AuraEffect* jaggedEff = caster->GetAuraEffect(Spells::spell_dh_jagged_spikes, EFFECT_0);
            int32 pct = jaggedEff ? jaggedEff->GetAmount() : 0;

            int32 damage = eventInfo.GetDamageInfo()->GetDamage();
            ApplyPct(damage, pct);

            caster->CastSpell(
                target,
                Spells::spell_dh_jagged_spikes_dmg,
                CastSpellExtraArgs(TRIGGERED_FULL_MASK).AddSpellBP0(damage)
            );
        }

        void Register() override
        {
            OnEffectProc += AuraEffectProcFn(
                spell_dh_jagged_spikes::HandleProc,
                EFFECT_0,
                SPELL_AURA_DUMMY
            );
        }
    };

    // 203704 - Mana Break
    class spell_dh_mana_break : public SpellScript
    {
        void HandleHit(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            Unit* target = GetHitUnit();
            if (!caster || !target)
                return;

            // Base damage from EFFECT_1
            int32 damage = GetSpellInfo()->GetEffect(EFFECT_1).BasePoints;

            // Mana percentage scaling
            float manaPct = target->GetPowerPct(POWER_MANA);
            if (manaPct >= 1.f)
            {
                float bonus = ((100.f - manaPct) / 10.f) * GetSpellInfo()->GetEffect(EFFECT_2).BasePoints;
                damage += int32(bonus);
            }

            // NOTE: Original script overwrites the damage entirely here.
            // This fixes the original behavior.
            damage = std::max<int32>(damage, int32(target->CountPctFromMaxHealth(5)));

            SetHitDamage(damage);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(
                spell_dh_mana_break::HandleHit,
                EFFECT_0,
                SPELL_EFFECT_SCHOOL_DAMAGE
            );
        }
    };

    // Master of the Glaive - 203556
    class spell_dh_master_of_the_glaive : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::spell_dh_throw_glaive });
        }

        bool CheckProc(ProcEventInfo& eventInfo)
        {
            if (eventInfo.GetSpellInfo()->Id == Spells::spell_dh_throw_glaive)
                return true;
            return false;
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_dh_master_of_the_glaive::CheckProc);
        }
    };

    // Metamorphosis (Havoc) - 191427
    class spell_dh_metamorphosis : public SpellScript
    {

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            if (!sSpellMgr->GetSpellInfo(Spells::spell_dh_metamorphosis_transform, DIFFICULTY_NONE) ||
                !sSpellMgr->GetSpellInfo(Spells::spell_dh_metamorphosis, DIFFICULTY_NONE) ||
                !sSpellMgr->GetSpellInfo(Spells::spell_dh_metamorphosis_impact_damage, DIFFICULTY_NONE))
                return false;
            return true;
        }

        void HandleDummy()
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            Player* player = caster->ToPlayer();
            if (!player)
                return;

            if (const WorldLocation* dest = GetExplTargetDest())
            {
                player->CastSpell(*dest, Spells::spell_dh_metamorphosis, true);
                player->CastSpell(Position(dest->GetPositionX(), dest->GetPositionY(), dest->GetPositionZ()), Spells::spell_dh_metamorphosis_impact_damage, true);
            }

            if (player->HasAura(Spells::spell_dh_demon_reborn)) // Remove CD of Eye Beam, Chaos Nova and Blur
            {
                player->GetSpellHistory()->ResetCooldown(Spells::spell_dh_chaos_nova, true);
                player->GetSpellHistory()->ResetCooldown(Spells::spell_dh_blur, true);
                player->GetSpellHistory()->AddCooldown(Spells::spell_dh_blur, 0, std::chrono::minutes(1));
                player->GetSpellHistory()->ResetCooldown(Spells::spell_dh_blur, true);
                player->GetSpellHistory()->ResetCooldown(Spells::spell_dh_eye_beam, true);
            }
        }

        void Register() override
        {
            BeforeCast += SpellCastFn(spell_dh_metamorphosis::HandleDummy);
        }
    };

    // 162264 - Metamorphosis (Vengeance) - Demonic Origins buff handler
    class spell_dh_metamorphosis_buffs : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            // We must ensure these spells exist because we remove/cast them
            return ValidateSpellInfo({
                Spells::spell_dh_demonic_origins,
                Spells::spell_dh_demonic_origins_buff
                });
        }

        void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            // Remove the Demonic Origins buff when Meta is applied
            caster->RemoveAura(Spells::spell_dh_demonic_origins_buff);
        }

        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            // Re-apply Demonic Origins buff when Meta ends (if the talent is active)
            if (caster->HasAura(Spells::spell_dh_demonic_origins))
                caster->CastSpell(caster, Spells::spell_dh_demonic_origins_buff, true);
        }

        void Register() override
        {
            OnEffectApply += AuraEffectApplyFn(
                spell_dh_metamorphosis_buffs::OnApply,
                EFFECT_0,
                SPELL_AURA_TRANSFORM,
                AURA_EFFECT_HANDLE_REAL
            );

            OnEffectRemove += AuraEffectRemoveFn(
                spell_dh_metamorphosis_buffs::OnRemove,
                EFFECT_0,
                SPELL_AURA_TRANSFORM,
                AURA_EFFECT_HANDLE_REAL
            );
        }
    };

    // 1250088 - Midnight: Collapsing Star crit damage += SpellCrit% * EFFECT_2_BP%; recalculated every 1s via PERIODIC_DUMMY
    class spell_dh_midnight_passive : public AuraScript
    {
        void HandlePeriodic(AuraEffect const* /*aurEff*/)
        {
            Player* player = GetTarget()->ToPlayer();
            if (!player)
                return;

            float critPct = *player->m_activePlayerData->SpellCritPercentage;
            int32 scalingPct = GetEffect(EFFECT_2)->GetAmount();
            int32 newAmount = int32(critPct * float(scalingPct) / 100.0f);
            GetEffect(EFFECT_0)->ChangeAmount(newAmount);
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_dh_midnight_passive::HandlePeriodic, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    // Nemesis - 206491
    class spell_dh_nemesis : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo(
                {
                    Spells::spell_dh_nemesis_aberrations,
                    Spells::spell_dh_nemesis_beasts,
                    Spells::spell_dh_nemesis_critters,
                    Spells::spell_dh_nemesis_demons,
                    Spells::spell_dh_nemesis_dragonkin,
                    Spells::spell_dh_nemesis_elementals,
                    Spells::spell_dh_nemesis_giants,
                    Spells::spell_dh_nemesis_humanoids,
                    Spells::spell_dh_nemesis_mechanicals,
                    Spells::spell_dh_nemesis_undead
                });
        }

        void HandleAfterRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (!GetTargetApplication())
                return;

            if (GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_DEATH)
                return;

            Unit* target = GetTargetApplication()->GetTarget();
            uint32 type = target->GetCreatureType();
            int32 dur = GetTargetApplication()->GetBase()->GetDuration();
            Unit* caster = GetAura()->GetCaster();

            if (!caster || !target)
                return;

            uint32 spellId = 0;

            switch (type)
            {
            case CREATURE_TYPE_ABERRATION:
                spellId = Spells::spell_dh_nemesis_aberrations;
                break;
            case CREATURE_TYPE_BEAST:
                spellId = Spells::spell_dh_nemesis_beasts;
                break;
            case CREATURE_TYPE_CRITTER:
                spellId = Spells::spell_dh_nemesis_critters;
                break;
            case CREATURE_TYPE_DEMON:
                spellId = Spells::spell_dh_nemesis_demons;
                break;
            case CREATURE_TYPE_DRAGONKIN:
                spellId = Spells::spell_dh_nemesis_dragonkin;
                break;
            case CREATURE_TYPE_ELEMENTAL:
                spellId = Spells::spell_dh_nemesis_elementals;
                break;
            case CREATURE_TYPE_GIANT:
                spellId = Spells::spell_dh_nemesis_giants;
                break;
            case CREATURE_TYPE_HUMANOID:
                spellId = Spells::spell_dh_nemesis_humanoids;
                break;
            case CREATURE_TYPE_MECHANICAL:
                spellId = Spells::spell_dh_nemesis_mechanicals;
                break;
            case CREATURE_TYPE_UNDEAD:
                spellId = Spells::spell_dh_nemesis_undead;
                break;
            default:
                break;
            }

            if (spellId)
                if (Aura* aur = caster->AddAura(spellId, caster))
                    aur->SetDuration(dur);
        }

        void Register() override
        {
            AfterEffectRemove += AuraEffectRemoveFn(
                spell_dh_nemesis::HandleAfterRemove,
                EFFECT_0,
                SPELL_AURA_MOD_SCHOOL_MASK_DAMAGE_FROM_CASTER,
                AURA_EFFECT_HANDLE_REAL
            );
        }
    };

    // Nether Bond - 207810
    class spell_dh_nether_bond : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({
                Spells::spell_dh_nether_bond_damage,
                Spells::spell_dh_nether_bond_periodic
                });
        }

        void HandleDummy()
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            caster->CastSpell(caster, Spells::spell_dh_nether_bond_periodic, true);
        }

        void Register() override
        {
            OnCast += SpellCastFn(spell_dh_nether_bond::HandleDummy);
        }
    };

    // Nether Bond periodic - 207811
    class spell_dh_nether_bond_periodic : public AuraScript
    {
        Unit* m_BondUnit = nullptr;

        void HandlePeriodic(AuraEffect const* /*aurEff*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            if (!m_BondUnit)
                m_BondUnit = GetBondUnit();

            if (!m_BondUnit)
                return;

            int32 casterHealBp = 0;
            int32 casterDamageBp = 0;
            int32 targetHealBp = 0;
            int32 targetDamageBp = 0;

            float casterHp = caster->GetHealthPct();
            float targetHp = m_BondUnit->GetHealthPct();
            float healthPct = (casterHp + targetHp) / 2.0f;

            if (casterHp < targetHp)
            {
                casterHealBp = caster->CountPctFromMaxHealth(healthPct) - caster->GetHealth();
                targetDamageBp = m_BondUnit->GetHealth() - m_BondUnit->CountPctFromMaxHealth(healthPct);
            }
            else
            {
                casterDamageBp = caster->GetHealth() - caster->CountPctFromMaxHealth(healthPct);
                targetHealBp = m_BondUnit->CountPctFromMaxHealth(healthPct) - m_BondUnit->GetHealth();
            }

            caster->CastSpell(
                caster,
                Spells::spell_dh_nether_bond_damage,
                CastSpellExtraArgs(TRIGGERED_FULL_MASK)
                .AddSpellMod(SPELLVALUE_BASE_POINT0, casterDamageBp)
                .AddSpellMod(SPELLVALUE_BASE_POINT1, casterHealBp)
            );

            caster->CastSpell(
                m_BondUnit,
                Spells::spell_dh_nether_bond_damage,
                CastSpellExtraArgs(TRIGGERED_FULL_MASK)
                .AddSpellMod(SPELLVALUE_BASE_POINT0, targetDamageBp)
                .AddSpellMod(SPELLVALUE_BASE_POINT1, targetHealBp)
            );
        }

        Unit* GetBondUnit()
        {
            Unit* caster = GetCaster();
            if (!caster)
                return nullptr;

            std::list<Unit*> units;
            Trinity::AnyUnitInObjectRangeCheck check(caster, 100.0f);
            Trinity::UnitListSearcher<Trinity::AnyUnitInObjectRangeCheck> search(caster, units, check);
            Cell::VisitAllObjects(caster, search, 100.0f);

            for (Unit* u : units)
                if (u->HasAura(Spells::spell_dh_nether_bond, caster->GetGUID()))
                    return u;

            return nullptr;
        }

        void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            m_BondUnit = GetBondUnit();
        }

        void Register() override
        {
            AfterEffectApply += AuraEffectApplyFn(
                spell_dh_nether_bond_periodic::HandleApply,
                EFFECT_0,
                SPELL_AURA_PERIODIC_DUMMY,
                AURA_EFFECT_HANDLE_REAL
            );

            OnEffectPeriodic += AuraEffectPeriodicFn(
                spell_dh_nether_bond_periodic::HandlePeriodic,
                EFFECT_0,
                SPELL_AURA_PERIODIC_DUMMY
            );
        }
    };

    // 185244 - Pain
    class spell_dh_pain : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({
                Spells::spell_dh_reward_pain
                });
        }

        void OnProc(AuraEffect* /*aurEff*/, ProcEventInfo& eventInfo)
        {
            Unit* caster = GetCaster();
            if (!caster || !eventInfo.GetDamageInfo())
                return;

            // Ignore positive spells (heals, buffs)
            if (eventInfo.GetSpellInfo() && eventInfo.GetSpellInfo()->IsPositive())
                return;

            int32 damageTaken = eventInfo.GetDamageInfo()->GetDamage();
            if (damageTaken <= 0)
                return;

            // Pain gained = 50% of damage taken relative to max HP
            float painAmount = (50.0f * float(damageTaken)) / float(caster->GetMaxHealth());

            caster->CastSpell(
                caster,
                Spells::spell_dh_reward_pain,
                CastSpellExtraArgs(TRIGGERED_FULL_MASK).AddSpellBP0(painAmount)
            );
        }

        void Register() override
        {
            OnEffectProc += AuraEffectProcFn(
                spell_dh_pain::OnProc,
                EFFECT_0,
                SPELL_AURA_MOD_POWER_DISPLAY
            );
        }
    };

    // 203650 - Prepared
    class spell_dh_prepared : public AuraScript
    {

        void PeriodicTick(AuraEffect const* aurEff)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            caster->ModifyPower(POWER_FURY, aurEff->GetAmount() / 10.f);
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_dh_prepared::PeriodicTick, EFFECT_0, SPELL_AURA_MOD_POWER_REGEN);
        }
    };

    class event_dh_rain_from_above_slowfall : public BasicEvent
    {
    public:
        event_dh_rain_from_above_slowfall(Unit* caster)
            : _caster(caster) { }

        bool Execute(uint64 /*execTime*/, uint32 /*diff*/) override
        {
            if (_caster)
                _caster->CastSpell(_caster, Spells::spell_dh_rain_from_above_slowfall, true);

            return true;
        }

    private:
        Unit* _caster;
    };

    // 206803 - Rain from Above
    class spell_dh_rain_from_above : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({
                Spells::spell_dh_rain_from_above_slowfall
                });
        }

        void HandleCast()
        {
            Unit* caster = GetCaster();
            if (!caster || !caster->ToPlayer())
                return;

            // Schedule slowfall 1.75 seconds later
            caster->m_Events.AddEvent(
                new event_dh_rain_from_above_slowfall(caster),
                caster->m_Events.CalculateTime(1750ms)
            );
        }

        void Register() override
        {
            AfterCast += SpellCastFn(spell_dh_rain_from_above::HandleCast);
        }
    };

    class event_dh_razor_spikes : public BasicEvent
    {
    public:
        event_dh_razor_spikes(Unit* caster, Unit* target)
            : _caster(caster), _target(target) { }

        bool Execute(uint64 /*execTime*/, uint32 /*diff*/) override
        {
            if (_caster && _target)
                _caster->CastSpell(_target, Spells::spell_dh_razor_spikes, true);

            return true;
        }

    private:
        Unit* _caster;
        Unit* _target;
    };

    // Razor Spikes - 209400
    class spell_dh_razor_spikes : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({
                Spells::spell_dh_razor_spikes
                });
        }

        bool CheckProc(ProcEventInfo& eventInfo)
        {
            DamageInfo* damageInfo = eventInfo.GetDamageInfo();
            if (!damageInfo)
                return false;

            if (damageInfo->GetAttackType() == BASE_ATTACK ||
                damageInfo->GetAttackType() == OFF_ATTACK)
            {
                Unit* caster = damageInfo->GetAttacker();
                Unit* target = damageInfo->GetVictim();
                if (!caster || !target || !caster->ToPlayer())
                    return false;

                if (!caster->IsValidAttackTarget(target))
                    return false;

                if (caster->HasAura(Spells::spell_dh_demon_spikes))
                {
                    caster->m_Events.AddEvent(
                        new event_dh_razor_spikes(caster, target),
                        caster->m_Events.CalculateTime(750ms)
                    );
                }
                return true;
            }
            return false;
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_dh_razor_spikes::CheckProc);
        }
    };

    // 1226019 - Reap (Devourer)
    class spell_dh_reap : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::spell_dh_reap_damage, Spells::spell_dh_reap_fury, Spells::spell_dh_soul_fragment_devourer,
                Spells::spell_dh_consume_soul_devourer, Spells::spell_dh_voidfall_meteor_vengeance, Spells::spell_dh_voidfall_meteor_havoc,
                Spells::spell_dh_world_killer_meteor_vengeance, Spells::spell_dh_world_killer_meteor_havoc });
        }

        void HandleCast()
        {
            Unit* caster = GetCaster();
            Unit* target = GetExplTargetUnit();
            if (!target)
                return;

            caster->CastSpell(target, Spells::spell_dh_reap_damage,
                CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
                .SetTriggeringSpell(GetSpell()));

            caster->CastSpell(caster, Spells::spell_dh_reap_fury,
                CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
                .SetTriggeringSpell(GetSpell()));

            CollectSoulFragments(caster, 4, GetSpell());
            TryFireVoidfallMeteors(caster, target);
        }

        void Register() override
        {
            AfterCast += SpellCastFn(spell_dh_reap::HandleCast);
        }
    };

    struct auraData
    {
        auraData(uint32 id, ObjectGuid casterGUID) : m_id(id), m_casterGuid(casterGUID) {}
        uint32 m_id;
        ObjectGuid m_casterGuid;
    };

    // Reverse Magic - 205604
    class spell_dh_reverse_magic : public SpellScript
    {
        void HandleScript()
        {
            Unit* player = GetCaster();
            if (!player || !player->ToPlayer())
                return;

            Unit* _player = player->ToPlayer();

            std::list<Unit*> allies;
            Trinity::AnyFriendlyUnitInObjectRangeCheck check(_player, _player, 10.f, true);
            Trinity::UnitListSearcher<Trinity::AnyFriendlyUnitInObjectRangeCheck> searcher(_player, allies, check);
            Cell::VisitAllObjects(_player, searcher, 10.f);

            for (Unit* unit : allies)
            {
                std::set<auraData*> auraListToRemove;
                Unit::AuraApplicationMap AuraList = unit->GetAppliedAuras();
                for (Unit::AuraApplicationMap::iterator iter = AuraList.begin(); iter != AuraList.end(); ++iter)
                {
                    Aura* aura = iter->second->GetBase();
                    if (!aura)
                        continue;

                    Unit* caster = aura->GetCaster();
                    if (!caster || caster->GetGUID() == unit->GetGUID())
                        continue;

                    if (!caster->IsWithinDist(unit, 40.0f))
                        continue;

                    if (aura->GetSpellInfo()->IsPositive())
                        continue;

                    if (aura->GetSpellInfo()->Dispel != DISPEL_MAGIC)
                        continue;

                    if (Creature* creature = caster->ToCreature())
                    {
                        if (creature->GetCreatureTemplate()->Classification == CreatureClassifications::Obsolete)
                            continue;
                        if (creature->GetCreatureTemplate()->Classification == CreatureClassifications::Elite && creature->GetMap()->IsDungeon())
                            continue;
                    }

                    if (Aura* targetAura = unit->AddAura(aura->GetSpellInfo()->Id, caster))
                    {
                        for (int i = 0; i < MAX_SPELL_EFFECTS; ++i)
                        {
                            targetAura->SetMaxDuration(aura->GetMaxDuration());
                            targetAura->SetDuration(aura->GetDuration());
                            if (targetAura->GetEffect(i) && aura->GetEffect(i))
                            {
                                AuraEffect* auraEffect = unit->GetAuraEffect(aura->GetSpellInfo()->Id, i);
                                if (!auraEffect)
                                    continue;

                                int32 amount = auraEffect->GetAmount();

                                if (auraEffect->GetAuraType() == SPELL_AURA_PERIODIC_DAMAGE ||
                                    auraEffect->GetAuraType() == SPELL_AURA_PERIODIC_DAMAGE_PERCENT)
                                    amount = caster->SpellDamageBonusDone(unit, aura->GetSpellInfo(), amount, DOT, aura->GetSpellInfo()->GetEffects().at(i), auraEffect->GetBase()->GetStackAmount());

                                targetAura->GetEffect(i)->SetAmount(amount);
                                targetAura->GetEffect(i)->SetPeriodicTimer(auraEffect->GetPeriodicTimer());
                            }
                        }
                        targetAura->SetNeedClientUpdateForTargets();
                    }

                    auraListToRemove.insert(new auraData(aura->GetSpellInfo()->Id, caster->GetGUID()));
                }

                for (auto aura : auraListToRemove)
                {
                    unit->RemoveAura(aura->m_id, aura->m_casterGuid);
                    delete aura;
                }

                auraListToRemove.clear();
            }
        }

        void Register() override
        {
            OnCast += SpellCastFn(spell_dh_reverse_magic::HandleScript);
        }
    };

    // 203783 - Shear proc
    class spell_dh_shear_proc : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::spell_dh_shattered_soul_lesser_right,
                Spells::spell_dh_shattered_soul_lesser_left,
                Spells::spell_dh_shatter_the_souls,
                Spells::spell_dh_felblade,
                Spells::spell_dh_shear });
        }

        void OnProc(AuraEffect* /*aurEff*/, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();

            Unit* caster = GetCaster();
            if (!caster || !eventInfo.GetSpellInfo())
                return;

            SpellInfo const* spell = eventInfo.GetSpellInfo();

            // ============================================================
            // EFFECT 1: Soul Fragment proc (Shear only)
            // ============================================================
            if (spell->Id == Spells::spell_dh_shear)
            {
                int32 soulChance = 15; // base Shear chance

                // Shatter the Souls bonus (only below 50% HP)
                if (caster->HealthBelowPct(50))
                {
                    if (AuraEffect* shatterEff = caster->GetAuraEffect(Spells::spell_dh_shatter_the_souls, EFFECT_0))
                        soulChance += shatterEff->GetAmount(); // +5% per rank
                }

                if (roll_chance(soulChance))
                {
                    // Spawn a Lesser Soul Fragment (retail-accurate)
                    ShatterLesserSoulFragment(*this, eventInfo);
                }
            }

            // ============================================================
            // EFFECT 2: Felblade cooldown reset
            // ============================================================
            if (caster->GetSpellHistory()->HasCooldown(Spells::spell_dh_felblade))
            {
                // Passive aura stores the Felblade reset chance in EFFECT_3
                AuraEffect* shearEff = caster->GetAuraEffect(Spells::spell_dh_shear_passive, EFFECT_3);
                int32 resetChance = shearEff ? shearEff->GetAmount() : 0;

                if (roll_chance(resetChance))
                    caster->GetSpellHistory()->ResetCooldown(Spells::spell_dh_felblade);
            }
        }

        static void ShatterLesserSoulFragment(AuraScript const&, ProcEventInfo const& procEvent)
        {
            procEvent.GetActionTarget()->CastSpell(procEvent.GetActor(),
                Trinity::Containers::SelectRandomContainerElement(std::array{ Spells::spell_dh_shattered_soul_lesser_right, Spells::spell_dh_shattered_soul_lesser_left }),
                TRIGGERED_DONT_REPORT_CAST_ERROR);
        }

        void Register() override
        {
            OnEffectProc += AuraEffectProcFn(
                spell_dh_shear_proc::OnProc,
                EFFECT_0,
                SPELL_AURA_PROC_TRIGGER_SPELL
            );
        }
    };

    // Solitude - 211509
    class spell_dh_solitude : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({
                Spells::spell_dh_solitude_buff
                });
        }

        void HandlePeriodic(AuraEffect const* /*aurEff*/)
        {
            PreventDefaultAction();

            Unit* caster = GetCaster();
            if (!caster || !GetSpellInfo()->GetEffect(EFFECT_1).IsEffect())
                return;

            int32 range = GetSpellInfo()->GetEffect(EFFECT_1).BasePoints;

            std::list<Unit*> allies;
            Trinity::AnyFriendlyUnitInObjectRangeCheck check(caster, caster, range, true);
            Trinity::UnitListSearcher<Trinity::AnyFriendlyUnitInObjectRangeCheck> searcher(caster, allies, check);
            Cell::VisitAllObjects(caster, searcher, range);

            allies.remove(caster);

            if (allies.empty() && !caster->HasAura(Spells::spell_dh_solitude_buff))
            {
                caster->CastSpell(caster, Spells::spell_dh_solitude_buff, true);
            }
            else if (!allies.empty())
            {
                caster->RemoveAurasDueToSpell(Spells::spell_dh_solitude_buff);
            }
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(
                spell_dh_solitude::HandlePeriodic,
                EFFECT_0,
                SPELL_AURA_PERIODIC_DUMMY
            );
        }
    };

    // Soul Barrier - 263648
    class spell_dh_soul_barrier : public AuraScript
    {
        void CalcAmount(AuraEffect const* /*aurEff*/, SpellEffectValue& amount, bool& /*canBeRecalculated*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            if (Player* player = caster->ToPlayer())
            {
                float coeff = amount / 100.f;
                float soulShardCoeff = GetSpellInfo()->GetEffect(EFFECT_1).BasePoints / 100.f;
                int32 ap = player->GetTotalAttackPowerValue(BASE_ATTACK);

                amount = coeff * ap;

                std::vector<std::vector<AreaTrigger*>> fragments;
                fragments.push_back(caster->GetAreaTriggers(Spells::spell_dh_shattered_souls_havoc));
                fragments.push_back(caster->GetAreaTriggers(Spells::spell_dh_shattered_souls_demon));
                fragments.push_back(caster->GetAreaTriggers(Spells::spell_dh_lesser_soul_shard));
                float range = 25.f;

                for (auto vec : fragments)
                {
                    for (AreaTrigger* at : vec)
                    {
                        if (!caster->IsWithinDist(at, range))
                            continue;

                        if (TempSummon* tempSumm = caster->SummonCreature(
                            WORLD_TRIGGER,
                            at->GetPositionX(), at->GetPositionY(), at->GetPositionZ(),
                            0, TEMPSUMMON_TIMED_DESPAWN, 100ms))
                        {
                            tempSumm->SetFaction(caster->GetFaction());
                            tempSumm->SetCreatorGUID(caster->GetGUID());

                            int32 bp = 0;
                            switch (at->GetTemplate()->Id.Id)
                            {
                            case 6007:
                            case 5997:
                                bp = Spells::spell_dh_soul_fragment_heal_vengeance;
                                break;
                            case 6710:
                                bp = Spells::spell_dh_consume_soul_heal;
                                break;
                            }

                            caster->CastSpell(
                                tempSumm,
                                Spells::spell_dh_consume_soul_vengeance_shattered,
                                CastSpellExtraArgs(TRIGGERED_FULL_MASK).AddSpellBP0(bp));

                            if (at->GetTemplate()->Id.Id == 6007)
                                caster->CastSpell(caster, Spells::spell_dh_soul_fragment_demon_bonus, true);

                            if (caster->HasAura(Spells::spell_dh_feed_the_demon))
                                caster->GetSpellHistory()->ModifyCooldown(
                                    Spells::spell_dh_demon_spikes,
                                    Milliseconds(-1000),
                                    false);

                            if (caster->HasAura(Spells::spell_dh_painbringer))
                                caster->CastSpell(caster, Spells::spell_dh_painbringer_buff, true);

                            amount += soulShardCoeff * ap;

                            at->SetDuration(0);
                        }
                    }
                }
            }

            if (AuraApplication* app = caster->GetAuraApplication(Spells::spell_dh_soul_barrier))
                app->ClientUpdate();
        }

        void HandleAbsorb(AuraEffect* aurEff, DamageInfo& dmgInfo, uint32& absorbAmount)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            uint32 threshold = caster->GetTotalAttackPowerValue(BASE_ATTACK) * 0.4914f;

            if (absorbAmount < dmgInfo.GetDamage())
                aurEff->SetAmount(absorbAmount + threshold);

            if (AuraApplication* app = caster->GetAuraApplication(Spells::spell_dh_soul_barrier))
                app->ClientUpdate();
        }

        void Register() override
        {
            DoEffectCalcAmount += AuraEffectCalcAmountFn(
                spell_dh_soul_barrier::CalcAmount,
                EFFECT_0,
                SPELL_AURA_SCHOOL_ABSORB);

            OnEffectAbsorb += AuraEffectAbsorbFn(
                spell_dh_soul_barrier::HandleAbsorb,
                EFFECT_0);
        }
    };

    // Soul Cleave - 228477
    class spell_dh_soul_cleave : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo(
                {
                    Spells::spell_dh_feast_of_souls,
                    Spells::spell_dh_feast_of_souls_periodic_heal,
                    Spells::spell_dh_shattered_souls_havoc,
                    Spells::spell_dh_shattered_souls_demon,
                    Spells::spell_dh_lesser_soul_shard,
                    Spells::spell_dh_soul_fragment_heal_vengeance,
                    Spells::spell_dh_consume_soul_heal,
                    Spells::spell_dh_consume_soul_vengeance_shattered,
                    Spells::spell_dh_soul_fragment_demon_bonus,
                    Spells::spell_dh_feed_the_demon,
                    Spells::spell_dh_demon_spikes,
                    Spells::spell_dh_painbringer,
                    Spells::spell_dh_painbringer_buff,
                    Spells::spell_dh_soul_barrier
                });
        }

        void HandleHeal(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            if (caster->GetTypeId() != TYPEID_PLAYER)
                return;

            if (caster->HasAura(Spells::spell_dh_feast_of_souls))
                caster->CastSpell(caster, Spells::spell_dh_feast_of_souls_periodic_heal, true);
        }

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            std::vector<std::vector<AreaTrigger*>> fragments;
            fragments.push_back(caster->GetAreaTriggers(Spells::spell_dh_shattered_souls_havoc));
            fragments.push_back(caster->GetAreaTriggers(Spells::spell_dh_shattered_souls_demon));
            fragments.push_back(caster->GetAreaTriggers(Spells::spell_dh_lesser_soul_shard));

            int32 range = GetEffectInfo().BasePoints;

            for (auto vec : fragments)
            {
                for (AreaTrigger* at : vec)
                {
                    if (!caster->IsWithinDist(at, range))
                        continue;

                    if (TempSummon* tempSumm = caster->SummonCreature(WORLD_TRIGGER,
                        at->GetPositionX(), at->GetPositionY(), at->GetPositionZ(),
                        0, TEMPSUMMON_TIMED_DESPAWN, 100ms))
                    {
                        tempSumm->SetFaction(caster->GetFaction());
                        tempSumm->SetCreatorGUID(caster->GetGUID());

                        int32 bp = 0;
                        switch (at->GetTemplate()->Id.Id)
                        {
                        case 6007:
                        case 5997:
                            bp = Spells::spell_dh_soul_fragment_heal_vengeance;
                            break;
                        case 6710:
                            bp = Spells::spell_dh_consume_soul_heal;
                            break;
                        }

                        caster->CastSpell(tempSumm,
                            Spells::spell_dh_consume_soul_vengeance_shattered,
                            CastSpellExtraArgs(TRIGGERED_FULL_MASK).AddSpellBP0(bp));

                        if (at->GetTemplate()->Id.Id == 6007)
                            caster->CastSpell(caster, Spells::spell_dh_soul_fragment_demon_bonus, true);

                        if (caster->HasAura(Spells::spell_dh_feed_the_demon))
                        {
                            // Reduce Demon Spikes cooldown by 1 second (including its category cooldown)
                            caster->GetSpellHistory()->ModifyCooldown(
                                Spells::spell_dh_demon_spikes,
                                -1s,
                                false
                            );
                        }

                        if (caster->HasAura(Spells::spell_dh_painbringer))
                            caster->CastSpell(caster, Spells::spell_dh_painbringer_buff, true);

                        if (AuraEffect* soulBarrier = caster->GetAuraEffect(Spells::spell_dh_soul_barrier, EFFECT_0))
                        {
                            int32 amount = soulBarrier->GetAmount() +
                                (float(sSpellMgr->GetSpellInfo(Spells::spell_dh_soul_barrier, DIFFICULTY_NONE)
                                    ->GetEffect(EFFECT_1).BasePoints) / 100.f)
                                * caster->GetTotalAttackPowerValue(BASE_ATTACK);

                            soulBarrier->SetAmount(amount);
                        }

                        at->SetDuration(0);
                    }
                }
            }
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_dh_soul_cleave::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            OnEffectHitTarget += SpellEffectFn(spell_dh_soul_cleave::HandleHeal, EFFECT_3, SPELL_EFFECT_HEAL);
        }
    };

    // Soul cleave damage - 228478
    class spell_dh_soul_cleave_damage : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo(
                {
                    Spells::spell_dh_gluttony_buff
                });
        }

        int32 m_ExtraSpellCost{};
        float m_Modifier = 1.0f;

        void HandleOnCast()
        {
            // Compute the modifier once per cast
            m_Modifier = ((float)m_ExtraSpellCost + 300.0f) / 600.0f;
        }

        void HandleOnHit()
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            float dmg = GetHitDamage() * 2;
            dmg *= m_Modifier;
            SetHitDamage(dmg);
        }

        void HandleDamage(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            int32 dmg = GetHitDamage() * 2;
            dmg = float(dmg) * (((float)m_ExtraSpellCost + 300.0f) / 600.0f);
            SetHitDamage(dmg);

            caster->SetPower(POWER_PAIN, caster->GetPower(POWER_PAIN) - m_ExtraSpellCost);
            caster->ToPlayer()->SetPower(POWER_PAIN, caster->GetPower(POWER_PAIN) - m_ExtraSpellCost);

            if (caster->HasAura(Spells::spell_dh_gluttony_buff))
                caster->RemoveAurasDueToSpell(Spells::spell_dh_gluttony_buff);
        }

        void Register() override
        {
            BeforeCast += SpellCastFn(spell_dh_soul_cleave_damage::HandleOnCast);
            OnEffectHitTarget += SpellEffectFn(spell_dh_soul_cleave_damage::HandleDamage,
                EFFECT_1, SPELL_EFFECT_WEAPON_PERCENT_DAMAGE);
        }
    };

    // Soul Fragment healing effects - 178963, 203794, 228532
    class spell_dh_soul_fragment_heals : public SpellScript
    {
        void HandleHit(SpellEffIndex effIndex)
        {
            // If the caster does NOT have Demonic Appetite, prevent the default heal effect
            if (!GetCaster()->HasAura(Spells::spell_dh_demonic_appetite))
                PreventHitDefaultEffect(effIndex);
        }

        void HandleHeal(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            // Feast on the Souls cooldown reduction
            AuraEffect* feastEff = caster->GetAuraEffect(Spells::spell_dh_feast_on_the_souls, EFFECT_0);
            if (!feastEff)
                return;

            int32 reductionTime = feastEff->GetAmount();   // seconds
            if (reductionTime <= 0)
                return;

            // Apply cooldown reduction to Chaos Nova and Eye Beam
            caster->GetSpellHistory()->ModifyCooldown(
                Spells::spell_dh_chaos_nova,
                Seconds(-reductionTime),
                false
            );

            caster->GetSpellHistory()->ModifyCooldown(
                Spells::spell_dh_eye_beam,
                Seconds(-reductionTime),
                false
            );
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(
                spell_dh_soul_fragment_heals::HandleHeal,
                EFFECT_0,
                SPELL_AURA_ANY
            );

            OnEffectLaunchTarget += SpellEffectFn(
                spell_dh_soul_fragment_heals::HandleHit,
                EFFECT_1,
                SPELL_EFFECT_TRIGGER_SPELL
            );

            OnEffectLaunch += SpellEffectFn(
                spell_dh_soul_fragment_heals::HandleHit,
                EFFECT_1,
                SPELL_EFFECT_TRIGGER_SPELL
            );
        }
    };

    // 1241937 - Soul Immolation: periodic dummy spawns soul fragments + grants Fury each tick
    class spell_dh_soul_immolation : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::spell_dh_consume_soul_spawn, Spells::spell_dh_soul_immolation_fury });
        }

        void HandlePeriodic(AuraEffect const* aurEff)
        {
            Unit* target = GetTarget();
            int32 totalFragments = aurEff->GetAmount();
            if (int32(aurEff->GetTickNumber()) <= totalFragments)
                target->CastSpell(target, Spells::spell_dh_consume_soul_spawn,
                    TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);

            target->CastSpell(target, Spells::spell_dh_soul_immolation_fury,
                TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_dh_soul_immolation::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    // 218679 - Spirit Bomb
    class spell_dh_spirit_bomb : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({
                Spells::spell_dh_spirit_bomb_damage,
                Spells::spell_dh_lesser_soul_shard,
                Spells::spell_dh_shattered_souls_havoc,
                Spells::spell_dh_shattered_souls_demon
                });
        }

        bool TryCastDamage(Unit* caster, Unit* target, uint32 atSpellId)
        {
            if (AreaTrigger* at = caster->GetAreaTrigger(atSpellId))
            {
                caster->CastSpell(target, Spells::spell_dh_spirit_bomb_damage, true);
                at->Remove();
                return true;
            }
            return false;
        }

        SpellCastResult CheckCast()
        {
            Unit* caster = GetCaster();
            if (!caster)
                return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;

            // Must have at least one soul fragment AT
            if (!caster->GetAreaTrigger(Spells::spell_dh_lesser_soul_shard) &&
                !caster->GetAreaTrigger(Spells::spell_dh_shattered_souls_havoc) &&
                !caster->GetAreaTrigger(Spells::spell_dh_shattered_souls_demon))
            {
                return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
            }

            return SPELL_CAST_OK;
        }

        void HandleHit()
        {
            Unit* caster = GetCaster();
            Unit* target = GetHitUnit();
            if (!caster || !target)
                return;

            // Try each soul fragment type in order
            for (uint32 spellId : {
                Spells::spell_dh_lesser_soul_shard,
                    Spells::spell_dh_shattered_souls_havoc,
                    Spells::spell_dh_shattered_souls_demon })
            {
                if (TryCastDamage(caster, target, spellId))
                    break;
            }
        }

        void Register() override
        {
            OnHit += SpellHitFn(spell_dh_spirit_bomb::HandleHit);
            OnCheckCast += SpellCheckCastFn(spell_dh_spirit_bomb::CheckCast);
        }
    };

    // 1246556 - Spontaneous Immolation: 5% chance on damaging spell cast to trigger Soul Immolation
    class spell_dh_spontaneous_immolation : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::spell_dh_spontaneous_immolation_buff });
        }

        static bool CheckProc(AuraScript const& script, ProcEventInfo const& eventInfo)
        {
            if (!eventInfo.GetSpellInfo())
                return false;
            int32 chance = static_cast<spell_dh_spontaneous_immolation const&>(script).GetEffect(EFFECT_0)->GetAmount();
            return roll_chance(chance);
        }

        void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo const& /*eventInfo*/) const
        {
            GetTarget()->CastSpell(GetTarget(), Spells::spell_dh_spontaneous_immolation_buff,
                TRIGGERED_FULL_MASK | TRIGGERED_SUPPRESS_CASTER_ANIM);
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_dh_spontaneous_immolation::CheckProc);
            OnEffectProc += AuraEffectProcFn(spell_dh_spontaneous_immolation::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    // 1246169 - The Hunt damage
    class spell_dh_the_hunt_damage : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::spell_dh_hungering_slash_passive, Spells::spell_dh_hungering_slash_override });
        }

        void HandleAfterCast() const
        {
            Unit* caster = GetCaster();
            if (caster->HasAura(Spells::spell_dh_hungering_slash_passive))
                caster->CastSpell(caster, Spells::spell_dh_hungering_slash_override,
                    TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
        }

        void Register() override
        {
            AfterCast += SpellCastFn(spell_dh_the_hunt_damage::HandleAfterCast);
        }
    };

    // 258881 - Trail of Ruin
    class spell_dh_trail_of_ruin : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            // We must validate Blade Dance because we read its TriggerSpell
            return ValidateSpellInfo({
                Spells::spell_dh_blade_dance
                });
        }

        bool CheckProc(ProcEventInfo& eventInfo)
        {
            SpellInfo const* procSpell = eventInfo.GetSpellInfo();
            if (!procSpell)
                return false;

            SpellInfo const* bladeDance = sSpellMgr->GetSpellInfo(Spells::spell_dh_blade_dance, DIFFICULTY_NONE);
            if (!bladeDance)
                return false;

            uint32 trigger = bladeDance->GetEffect(EFFECT_0).TriggerSpell;
            if (!trigger)
                return false;

            return procSpell->Id == trigger;
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_dh_trail_of_ruin::CheckProc);
        }
    };

    // 213480 - Unending Hatred
    class spell_dh_unending_hatred : public AuraScript
    {
        bool CheckProc(ProcEventInfo& eventInfo)
        {
            DamageInfo* dmg = eventInfo.GetDamageInfo();
            if (!dmg)
                return false;

            // Only trigger from magic damage
            return (dmg->GetSchoolMask() & SPELL_SCHOOL_MASK_MAGIC) != 0;
        }

        void HandleProc(ProcEventInfo const& eventInfo)
        {
            Player* caster = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
            if (!caster)
                return;

            DamageInfo* dmg = eventInfo.GetDamageInfo();
            if (!dmg)
                return;

            int32 points = GetPointsGained(caster, dmg->GetDamage());
            if (points <= 0)
                return;

            // Havoc ? Fury, Vengeance ? Pain
            if (caster->GetPrimarySpecialization() == ChrSpecialization::DemonHunterHavoc)
                caster->EnergizeBySpell(caster, GetSpellInfo(), points, POWER_FURY);
            else if (caster->GetPrimarySpecialization() == ChrSpecialization::DemonHunterVengeance)
                caster->EnergizeBySpell(caster, GetSpellInfo(), points, POWER_PAIN);
        }

        int32 GetPointsGained(Player* caster, uint32 damage)
        {
            // Percent of max HP / 2
            float pct = (float(damage) / float(caster->GetMaxHealth())) * 50.f;

            int32 maxGain = GetSpellInfo()->GetEffect(EFFECT_0).BasePoints;

            // Original script logic:
            if (pct > maxGain)
                return maxGain;
            if (pct < 1.f)
                return 1;

            return int32(pct);
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_dh_unending_hatred::CheckProc);
            OnProc += AuraProcFn(spell_dh_unending_hatred::HandleProc);
        }
    };

    // 1256305/1256306/1256617/1256619 - Voidfall/World Killer meteor damage: apply Catastrophe DoT if passive present
    class spell_dh_voidfall_meteor_damage : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::spell_dh_catastrophe_dot });
        }

        void HandleHit(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetOriginalCaster();
            if (!caster)
                caster = GetCaster();

            Unit* target = GetHitUnit();
            if (!caster || !target)
                return;

            if (!caster->HasAura(Spells::spell_dh_catastrophe_passive))
                return;

            AuraEffect* eff = caster->GetAuraEffect(Spells::spell_dh_catastrophe_passive, EFFECT_1);
            if (!eff)
                return;

            int32 pct = eff->GetAmount();
            int32 dotDmg = int32(GetHitDamage() * pct / 100) / 4;

            caster->CastSpell(
                target,
                Spells::spell_dh_catastrophe_dot,
                CastSpellExtraArgs(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR)
                .AddSpellMod(SPELLVALUE_BASE_POINT0, dotDmg)
            );
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_dh_voidfall_meteor_damage::HandleHit, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        }
    };

    // 473728 - Void Ray
    class spell_dh_void_ray : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({
                Spells::spell_dh_void_ray_damage,
                Spells::spell_dh_doomsayer_buff
                });
        }

        void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* target = GetTarget();
            if (!target->HasAura(Spells::spell_dh_doomsayer_passive))
                return;

            if (!target->IsInCombat())
            {
                target->CastSpell(
                    target,
                    Spells::spell_dh_doomsayer_buff,
                    TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR
                );
            }
            else if (Aura const* passive = target->GetAura(Spells::spell_dh_doomsayer_passive))
            {
                if (auto* script = passive->GetScript<spell_dh_doomsayer_passive>())
                    if (script->IsWithinCombatWindow(target))
                        target->CastSpell(
                            target,
                            Spells::spell_dh_doomsayer_buff,
                            TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR
                        );
            }
        }

        void HandleEffectPeriodic(AuraEffect const* aurEff) const
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            TriggerCastFlags flags = TriggerCastFlags(
                TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR
            );

            if (caster->HasAura(Spells::spell_dh_void_metamorphosis_buff))
                flags = TriggerCastFlags(flags | TRIGGERED_IGNORE_POWER_COST);

            caster->CastSpell(
                nullptr,
                Spells::spell_dh_void_ray_damage,
                CastSpellExtraArgsInit{
                    .TriggerFlags = flags,
                    .TriggeringAura = aurEff
                }
            );
        }

        void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_EXPIRE)
                return;

            Unit* target = GetTarget();

            if (target->HasAura(Spells::spell_dh_meteoric_rise_passive))
            {
                target->CastSpell(
                    target,
                    Spells::spell_dh_voidfall_stack,
                    TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR
                );
            }

            // Moment of Craving: reset Reap CD + apply buff (+6 fragments + Eradicate override)
            if (target->HasAura(Spells::spell_dh_moment_of_craving_passive))
            {
                target->GetSpellHistory()->ResetCooldown(Spells::spell_dh_reap, true);
                target->GetSpellHistory()->ResetCooldown(Spells::spell_dh_cull, true);
                target->GetSpellHistory()->ResetCooldown(Spells::spell_dh_eradicate, true);

                target->CastSpell(
                    target,
                    Spells::spell_dh_moment_of_craving_buff,
                    TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR
                );

                target->CastSpell(
                    target,
                    Spells::spell_dh_eradicate_override,
                    TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR
                );

                SetVoidMetaReapOverride(target, false);
            }
        }

        void Register() override
        {
            AfterEffectApply += AuraEffectApplyFn(
                spell_dh_void_ray::HandleApply,
                EFFECT_0,
                SPELL_AURA_PERIODIC_TRIGGER_SPELL,
                AURA_EFFECT_HANDLE_REAL
            );

            OnEffectPeriodic += AuraEffectPeriodicFn(
                spell_dh_void_ray::HandleEffectPeriodic,
                EFFECT_0,
                SPELL_AURA_PERIODIC_TRIGGER_SPELL
            );

            AfterEffectRemove += AuraEffectRemoveFn(
                spell_dh_void_ray::HandleRemove,
                EFFECT_0,
                SPELL_AURA_PERIODIC_TRIGGER_SPELL,
                AURA_EFFECT_HANDLE_REAL
            );
        }
    };

    // 1225789 - Void Metamorphosis (stack counter)
    class spell_dh_void_metamorphosis_counter : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::spell_dh_can_metamorphosis });
        }

        void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (GetStackAmount() >= static_cast<int32>(GetSpellInfo()->StackAmount))
                GetTarget()->CastSpell(GetTarget(), Spells::spell_dh_can_metamorphosis,
                    TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
        }

        void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            GetTarget()->RemoveAura(Spells::spell_dh_can_metamorphosis);
        }

        void Register() override
        {
            AfterEffectApply += AuraEffectApplyFn(spell_dh_void_metamorphosis_counter::HandleApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAPPLY);
            AfterEffectRemove += AuraEffectRemoveFn(spell_dh_void_metamorphosis_counter::HandleRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    // 205629 - Demonic Trample
    // MiscId - 6482
    struct at_dh_demonic_trample : AreaTriggerAI
    {
        at_dh_demonic_trample(AreaTrigger* areatrigger) : AreaTriggerAI(areatrigger) {}

        void OnUnitEnter(Unit* unit) override
        {
            Unit* caster = at->GetCaster();

            if (!caster || !unit)
                return;

            if (caster->IsValidAttackTarget(unit))
            {
                caster->CastSpell(unit, Spells::spell_dh_demonic_trample_stun, true);
                caster->CastSpell(unit, Spells::spell_dh_demonic_trample_dmg, true);
            }
        }
    };

    // 235903 - Mana Rift
    // MiscId - 9695
    struct at_dh_mana_rift : AreaTriggerAI
    {
        at_dh_mana_rift(AreaTrigger* areatrigger) : AreaTriggerAI(areatrigger) {}

        void OnUnitExit(Unit* unit, AreaTriggerExitReason /*reason*/) override
        {
            Unit* caster = at->GetCaster();
            if (!caster || !unit)
                return;

            SpellInfo const* spellProto = sSpellMgr->GetSpellInfo(Spells::spell_dh_mana_rift, DIFFICULTY_NONE);
            if (!spellProto)
                return;

            if (at->IsRemoved())
            {
                if (caster->IsValidAttackTarget(unit))
                {
                    int32 hpBp = unit->CountPctFromMaxHealth(spellProto->GetEffect(EFFECT_1).BasePoints);
                    int32 manaBp = unit->CountPctFromMaxPower(POWER_MANA, spellProto->GetEffect(EFFECT_2).BasePoints);
                    CastSpellExtraArgs args;
                    args.AddSpellMod(SPELLVALUE_BASE_POINT0, hpBp);
                    args.AddSpellMod(SPELLVALUE_BASE_POINT0, manaBp);
                    args.SetTriggerFlags(TRIGGERED_FULL_MASK);
                    caster->CastSpell(unit, Spells::spell_dh_mana_rift_damage_power_burn, args);
                }
            }
        }
    };

    // 209693 - Shattered Souls, 209788 - Shattered Souls and 1223412 - Soul Fragment
// Id - 3680, 6659 and 36671
    template<uint32 SpellId>
    struct at_dh_shattered_souls : public AreaTriggerAI
    {
        using AreaTriggerAI::AreaTriggerAI;

        uint32 _spawnDelay = 500;

        void OnUpdate(uint32 diff) override
        {
            if (!_spawnDelay)
                return;

            if (_spawnDelay > diff)
            {
                _spawnDelay -= diff;
                return;
            }

            _spawnDelay = 0;

            // Check if caster is already inside now that delay expired
            if (Unit* caster = at->GetCaster())
                if (at->GetInsideUnits().count(caster->GetGUID()))
                    Collect(caster);
        }

        void OnUnitEnter(Unit* unit) override
        {
            if (_spawnDelay)
                return;

            Collect(unit);
        }

        void Collect(Unit* unit)
        {
            unit->CastSpell(at->GetPosition(), SpellId,
                TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);

            if (unit->HasAura(Spells::spell_dh_void_metamorphosis_passive) &&
                !unit->HasAura(Spells::spell_dh_void_metamorphosis_buff))
            {
                unit->CastSpell(unit, Spells::spell_dh_void_metamorphosis_counter,
                    TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
            }

            if (unit->HasAura(Spells::spell_dh_void_metamorphosis_buff) &&
                unit->HasAura(Spells::spell_dh_collapsing_star_passive))
            {
                unit->CastSpell(unit, Spells::spell_dh_collapsing_star_counter,
                    TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
            }

            if (unit->HasAura(Spells::spell_dh_void_metamorphosis_buff) &&
                unit->HasAura(Spells::spell_dh_emptiness_passive))
            {
                unit->CastSpell(unit, Spells::spell_dh_emptiness_buff,
                    TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
            }

            if (Aura* fos = unit->GetAura(Spells::spell_dh_feast_of_souls_passive))
                if (auto* script = fos->GetScript<spell_dh_feast_of_souls_tracker>())
                    script->AddStack(unit);

            at->Remove();
        }

        void OnInitialize() override
        {
            if (Unit* caster = at->GetCaster())
            {
                if (caster->HasAura(Spells::spell_dh_shattered_souls_vengeance))
                    caster->CastSpell(caster, Spells::spell_dh_soul_fragment_counter,
                        TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
                else if (caster->HasAura(Spells::spell_dh_shattered_souls_devourer))
                    caster->CastSpell(caster, Spells::spell_dh_soul_fragments_devourer_counter,
                        TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
            }
        }

        void OnRemove() override
        {
            if (Unit* caster = at->GetCaster())
            {
                caster->RemoveAuraFromStack(Spells::spell_dh_soul_fragment_counter);
                caster->RemoveAuraFromStack(Spells::spell_dh_soul_fragments_devourer_counter);
            }
        }
    };

    using at_dh_shattered_souls_devourer = at_dh_shattered_souls<Spells::spell_dh_consume_soul_devourer>;
    using at_dh_shattered_souls_havoc_demon = at_dh_shattered_souls<Spells::spell_dh_consume_soul_havoc_demon>;
    using at_dh_shattered_souls_havoc_lesser = at_dh_shattered_souls<Spells::spell_dh_consume_soul_havoc_lesser>;
    using at_dh_shattered_souls_havoc_shattered = at_dh_shattered_souls<Spells::spell_dh_consume_soul_havoc_shattered>;
    using at_dh_shattered_souls_vengeance_demon = at_dh_shattered_souls<Spells::spell_dh_consume_soul_vengeance_demon>;
    using at_dh_shattered_souls_vengeance_lesser = at_dh_shattered_souls<Spells::spell_dh_consume_soul_vengeance_lesser>;
    using at_dh_shattered_souls_vengeance_shattered = at_dh_shattered_souls<Spells::spell_dh_consume_soul_vengeance_shattered>;
}

void AddSC_custom_demonhunter_spell_fixes()
{
    using namespace Scripts::Custom::DemonHunter;

    RegisterSpellScript(spell_dh_annihilation);
    RegisterSpellScript(spell_dh_awaken_the_demon);
    RegisterSpellScript(spell_dh_blade_turning);
    RegisterSpellScript(spell_dh_bloodlet);
    RegisterSpellScript(spell_dh_chaos_cleave);
    RegisterSpellScript(spell_dh_chaos_strike_energize);
    RegisterSpellScript(spell_dh_collapsing_star);
    RegisterSpellScript(spell_dh_collapsing_star_counter);
    RegisterSpellScript(spell_dh_consume_soul_missile);
    RegisterSpellScript(spell_dh_cull);
    RegisterSpellScript(spell_dh_darkness_absorb);
    RegisterSpellScript(spell_dh_demonic_infusion);
    RegisterSpellScript(spell_dh_desperate_instincts);
    RegisterSpellScript(spell_dh_devour);
    RegisterSpellScript(spell_dh_doomsayer_buff);
    RegisterSpellScript(spell_dh_doomsayer_passive);
    RegisterSpellScript(spell_dh_emptiness_buff);
    RegisterSpellScript(spell_dh_entropy);
    RegisterSpellScript(spell_dh_eradicate);
    RegisterSpellScript(spell_dh_eradicate_damage);
    RegisterSpellScript(spell_dh_eye_of_leotheras);
    RegisterSpellScript(spell_dh_feast_of_souls_tracker);
    RegisterSpellScript(spell_dh_fel_barrage);
    RegisterSpellScript(spell_dh_fel_barrage_aura);
    RegisterSpellScript(spell_dh_fel_barrage_damage);
    RegisterSpellScript(spell_dh_fel_devastation_damage);
    RegisterSpellScript(spell_dh_fel_eruption);
    RegisterSpellScript(spell_dh_fel_lance);
    RegisterSpellScript(spell_dh_fel_rush_specless);
    RegisterSpellScript(spell_dh_fel_rush);
    RegisterSpellAndAuraScriptPair(spell_dh_fel_rush_dash, spell_dh_fel_rush_dash_aura);
    RegisterSpellScript(spell_dh_fel_rush_damage);
    RegisterSpellScript(spell_dh_fel_rush_dash_ground);
    RegisterSpellScript(spell_dh_fracture);
    RegisterSpellScript(spell_dh_frailty);
    RegisterSpellScript(spell_dh_hungering_slash);
    RegisterSpellScript(spell_dh_immolation_aura);
    RegisterSpellScript(spell_dh_immolation_aura_damage);
    RegisterSpellScript(spell_dh_infernal_strike);
    RegisterSpellScript(spell_dh_intimidated);
    RegisterSpellScript(spell_dh_jagged_spikes);
    RegisterSpellScript(spell_dh_mana_break);
    RegisterSpellScript(spell_dh_master_of_the_glaive);
    RegisterSpellScript(spell_dh_metamorphosis);
    RegisterSpellScript(spell_dh_metamorphosis_buffs);
    RegisterSpellScript(spell_dh_midnight_passive);
    RegisterSpellScript(spell_dh_nemesis);
    RegisterSpellScript(spell_dh_nether_bond);
    RegisterSpellScript(spell_dh_nether_bond_periodic);
    RegisterSpellScript(spell_dh_pain);
    RegisterSpellScript(spell_dh_prepared);
    RegisterSpellScript(spell_dh_rain_from_above);
    RegisterSpellScript(spell_dh_razor_spikes);
    RegisterSpellScript(spell_dh_reap);
    RegisterSpellScript(spell_dh_reverse_magic);
    RegisterSpellScript(spell_dh_shear_proc);
    RegisterSpellScript(spell_dh_solitude);
    RegisterSpellScript(spell_dh_soul_barrier);
    RegisterSpellScript(spell_dh_soul_cleave);
    RegisterSpellScript(spell_dh_soul_cleave_damage);
    RegisterSpellScript(spell_dh_soul_fragment_heals);
    RegisterSpellScript(spell_dh_soul_immolation);
    RegisterSpellScript(spell_dh_spirit_bomb);
    RegisterSpellScript(spell_dh_spontaneous_immolation);
    RegisterSpellScript(spell_dh_the_hunt_damage);
    RegisterSpellScript(spell_dh_trail_of_ruin);
    RegisterSpellScript(spell_dh_unending_hatred);
    RegisterSpellScript(spell_dh_void_metamorphosis_buff);
    RegisterSpellScript(spell_dh_void_metamorphosis_counter);
    RegisterSpellScript(spell_dh_void_ray);
    RegisterSpellScript(spell_dh_voidfall_meteor_damage);
    RegisterSpellScript(spell_dh_voidfall_passive);

    RegisterAreaTriggerAI(at_dh_demonic_trample);
    RegisterAreaTriggerAI(at_dh_mana_rift);
    RegisterAreaTriggerAI(at_dh_shattered_souls_devourer);
    RegisterAreaTriggerAI(at_dh_shattered_souls_havoc_demon);
    RegisterAreaTriggerAI(at_dh_shattered_souls_havoc_lesser);
    RegisterAreaTriggerAI(at_dh_shattered_souls_havoc_shattered);
    RegisterAreaTriggerAI(at_dh_shattered_souls_vengeance_demon);
    RegisterAreaTriggerAI(at_dh_shattered_souls_vengeance_lesser);
    RegisterAreaTriggerAI(at_dh_shattered_souls_vengeance_shattered);
}
