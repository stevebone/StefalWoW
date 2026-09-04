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

#include "CreatureAI.h"
#include "Log.h"
#include "Pet.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "SpellScript.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "Unit.h"
#include "ObjectAccessor.h"

#include "Custom_Warlock_Defines.h"

namespace Scripts::Custom::Warlock
{
    // 265187 - Summon Demonic Tyrant
    class spell_warlock_summon_demonic_tyrant : public SpellScript
    {
        void HandleEffect0(SpellEffIndex effIndex)
        {
            Unit* caster = GetCaster();

            if (!caster)
                return;

            if (caster->HasAura(Spells::AntoranArmaments))
            {
                PreventHitEffect(effIndex);
                PreventHitDefaultEffect(effIndex);
            }
        }

        void HandleEffect3(SpellEffIndex effIndex)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            if (!caster->HasAura(Spells::AntoranArmaments))
            {
                PreventHitEffect(effIndex);
                PreventHitDefaultEffect(effIndex);
            }
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_warlock_summon_demonic_tyrant::HandleEffect0, EFFECT_0, SPELL_EFFECT_SUMMON);
            OnEffectHitTarget += SpellEffectFn(spell_warlock_summon_demonic_tyrant::HandleEffect3, EFFECT_3, SPELL_EFFECT_SUMMON);
        }
    };

    struct DreadstalkerCastContext
    {
        ObjectGuid TargetGuid;
        uint8 SpawnIndex = 0;
        uint32 ExistingDreadstalkers = 0;
    };

    std::unordered_map<ObjectGuid, DreadstalkerCastContext> s_dreadstalkerCastContext;

    void TeleportDreadstalkerToTarget(Unit* dreadstalker, Unit* target, uint8 spawnIndex)
    {
        if (!dreadstalker || !target)
            return;

        float const angle = float(spawnIndex % 8) * (M_PI / 4.0f) + frand(0.0f, float(M_PI / 4.0f));
        Position const pos = target->GetNearPosition(3.0f, angle);
        dreadstalker->NearTeleportTo(pos, false);

        if (Creature* creature = dreadstalker->ToCreature())
            if (creature->AI())
                creature->AI()->AttackStart(target);
    }

    // 104316 - Call Dreadstalkers
    class spell_warlock_call_dreadstalkers : public SpellScript
    {
        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            for (int32 i = 0; i < GetEffectValue(); ++i)
                caster->CastSpell(caster, Spells::CallDreadstalkersSummon, true);
        }

        void HandleCast()
        {
            Unit* caster = GetCaster();
            Unit* target = GetExplTargetUnit();
            if (!caster || !target)
                return;

            uint32 existingDreadstalkers = 0;
            if (Player* player = caster->ToPlayer())
            {
                for (Unit* unit : player->m_Controlled)
                    if (unit->GetEntry() == Creatures::WarlockDreadstalker)
                        ++existingDreadstalkers;
            }

            s_dreadstalkerCastContext[caster->GetGUID()] = { target->GetGUID(), 0, existingDreadstalkers };
        }

        void HandleAfterCast()
        {
            Unit* caster = GetCaster();
            Unit* target = GetExplTargetUnit();
            if (!caster || !target)
                return;

            if (Aura* aura = caster->GetAura(Spells::RippedThroughThePortal))
            {
                if (AuraEffect const* effect = aura->GetEffect(0))
                {
                    if (roll_chance(effect->GetAmount()))
                        caster->CastSpell(caster, Spells::CallDreadstalkersSummon, true);
                }
            }

            ObjectGuid const casterGuid = caster->GetGUID();
            ObjectGuid const targetGuid = target->GetGUID();
            uint32 const existingDreadstalkers = s_dreadstalkerCastContext[casterGuid].ExistingDreadstalkers;

            caster->m_Events.AddEventAtOffset([casterGuid, targetGuid, existingDreadstalkers]()
                {
                    Player* owner = ObjectAccessor::FindPlayer(casterGuid);
                    if (!owner)
                    {
                        s_dreadstalkerCastContext.erase(casterGuid);
                        return;
                    }

                    Unit* destTarget = ObjectAccessor::GetUnit(*owner, targetGuid);
                    if (!destTarget)
                    {
                        s_dreadstalkerCastContext.erase(casterGuid);
                        return;
                    }

                    uint8 index = 0;
                    uint32 skipped = 0;
                    for (Unit* unit : owner->m_Controlled)
                    {
                        if (unit->GetEntry() != Creatures::WarlockDreadstalker)
                            continue;

                        if (skipped < existingDreadstalkers)
                        {
                            ++skipped;
                            continue;
                        }

                        TeleportDreadstalkerToTarget(unit, destTarget, index);
                        ++index;
                    }

                    s_dreadstalkerCastContext.erase(casterGuid);
                }, 50ms);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_warlock_call_dreadstalkers::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            OnCast += SpellCastFn(spell_warlock_call_dreadstalkers::HandleCast);
            AfterCast += SpellCastFn(spell_warlock_call_dreadstalkers::HandleAfterCast);
        }
    };

    // 364750 - Call Dreadstalkers Summon
    class spell_warlock_call_dreadstalkers_summon : public SpellScript
    {
        void HandleSummon(SpellEffIndex /*effIndex*/)
        {
            Creature* summon = GetHitCreature();
            Unit* owner = GetOriginalCaster();
            if (!summon || !owner)
                return;

            auto itr = s_dreadstalkerCastContext.find(owner->GetGUID());
            if (itr == s_dreadstalkerCastContext.end())
                return;

            Unit* target = ObjectAccessor::GetUnit(*owner, itr->second.TargetGuid);
            if (!target)
                return;

            TeleportDreadstalkerToTarget(summon, target, itr->second.SpawnIndex);
            ++itr->second.SpawnIndex;
        }

        void Register() override
        {
            OnEffectHit += SpellEffectFn(spell_warlock_call_dreadstalkers_summon::HandleSummon, EFFECT_0, SPELL_EFFECT_SUMMON);
        }
    };

    // 1245089 - Avatar of Destruction
    class spell_warl_avatar_of_destruction : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::SoulFire, Spells::SummonOverfiend });
        }

        bool CheckProc(ProcEventInfo& eventInfo)
        {
            SpellInfo const* spellInfo = eventInfo.GetSpellInfo();
            if (!spellInfo)
                return false;

            return spellInfo->Id == Spells::SoulFire;
        }

        void HandleProc(ProcEventInfo& /*eventInfo*/)
        {
            Unit* caster = GetTarget();
            if (!caster)
                return;

            caster->CastSpell(caster, Spells::SummonOverfiend, CastSpellExtraArgsInit{
                .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR
                });
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_warl_avatar_of_destruction::CheckProc);
            OnProc += AuraProcFn(spell_warl_avatar_of_destruction::HandleProc);
        }
    };

    void AddSoulLeechAbsorb(Unit* unit, uint32 spellId, int32 addAbsorb, float maxPerc)
    {
        if (!unit || addAbsorb <= 0)
            return;

        float const maxAbsorb = float(unit->CountPctFromMaxHealth(maxPerc));

        if (AuraEffect* auraEff = unit->GetAuraEffect(spellId, EFFECT_0))
        {
            float allAbsorb = float(auraEff->GetAmount()) + float(addAbsorb);
            if (allAbsorb > maxAbsorb)
                allAbsorb = maxAbsorb;

            auraEff->SetAmount(int32(allAbsorb));
            if (AuraApplication* app = unit->GetAuraApplication(spellId))
                app->ClientUpdate();
        }
        else
        {
            int32 bp0 = int32(std::min<float>(float(addAbsorb), maxAbsorb));
            unit->CastSpell(unit, spellId,
                CastSpellExtraArgs(TRIGGERED_FULL_MASK).AddSpellBP0(bp0));
        }
    }

    // 6262 - Healthstone (Soulburn empowerment)
    class spell_warl_healthstone_soulburn : public SpellScript
    {
        bool _empowered = false;

        bool Load() override
        {
            if (Unit* caster = GetCaster())
                _empowered = caster->HasAura(Spells::SoulburnBuff);
            return true;
        }

        void HandleOnHit()
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            int32 heal = int32(CalculatePct(caster->GetCreateHealth(), GetHitHeal()));

            if (_empowered)
            {
                AddPct(heal, Spells::SoulburnHealthstoneHealPct);
                caster->CastSpell(caster, Spells::SoulburnHealthstoneBuff, CastSpellExtraArgs(TRIGGERED_FULL_MASK));
            }

            SetHitHeal(heal);
        }

        void Register() override
        {
            OnHit += SpellHitFn(spell_warl_healthstone_soulburn::HandleOnHit);
        }
    };

    // 387630 - Soulburn: Drain Life (hidden proc aura)
    class spell_warl_soulburn_drain_life_absorb : public AuraScript
    {
        bool CheckProc(ProcEventInfo& eventInfo)
        {
            HealInfo* healInfo = eventInfo.GetHealInfo();
            if (!healInfo || !healInfo->GetHeal())
                return false;

            SpellInfo const* spellInfo = eventInfo.GetSpellInfo();
            return spellInfo && spellInfo->Id == Spells::DrainLife;
        }

        void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();

            HealInfo* healInfo = eventInfo.GetHealInfo();
            if (!healInfo)
                return;

            AddSoulLeechAbsorb(GetTarget(), Spells::SoulburnDrainLifeAbsorb, int32(healInfo->GetHeal()), float(Spells::SoulburnDrainLifeAbsorbMaxPct));
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_warl_soulburn_drain_life_absorb::CheckProc);
            OnEffectProc += AuraEffectProcFn(spell_warl_soulburn_drain_life_absorb::HandleProc, EFFECT_0, SPELL_AURA_ANY);
        }
    };

    // 234153 - Drain Life (Soulburn empowerment)
    class spell_warl_drain_life_soulburn : public SpellScript
    {
        bool _empowered = false;

        bool Load() override
        {
            if (Unit* caster = GetCaster())
                _empowered = caster->HasAura(Spells::SoulburnBuff);
            return true;
        }

        void HandleOnCast()
        {
            Unit* caster = GetCaster();
            if (!caster || !_empowered)
                return;

            caster->CastSpell(caster, Spells::SoulburnDrainLifeBuff, CastSpellExtraArgs(TRIGGERED_FULL_MASK));
        }

        void Register() override
        {
            OnCast += SpellCastFn(spell_warl_drain_life_soulburn::HandleOnCast);
        }
    };

    // 111771 - Demonic Gateway
    class spell_warl_demonic_gateway : public SpellScript
    {
        int32 CalcCastTime(int32 castTime) override
        {
            if (castTime <= 0)
                return castTime;

            Unit* caster = GetCaster();
            if (caster && caster->HasAura(Spells::SoulburnBuff))
                return 0;

            return castTime;
        }

        SpellCastResult CheckRequirement()
        {
            Unit* caster = GetCaster();
            if (!caster)
                return SPELL_FAILED_DONT_REPORT;

            if (caster->HasAura(Spells::ArenaPreparation))
                return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;

            Spell* spell = GetSpell();
            if (spell->m_targets.HasDst())
            {
                Position pos = spell->m_targets.GetDst()->_position.GetPosition();
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

            caster->SendPlaySpellVisual(dest->GetPosition(), 63644, 0, 0, 2.0f);
        }

        void HandleLaunch(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            std::vector<Creature*> gateways;
            caster->GetCreatureListWithEntryInGrid(gateways, Creatures::DemonicGatewayGreen, 200.0f);

            std::vector<Creature*> purpleGateways;
            caster->GetCreatureListWithEntryInGrid(purpleGateways, Creatures::DemonicGatewayPurple, 200.0f);
            gateways.insert(gateways.end(), purpleGateways.begin(), purpleGateways.end());

            for (Creature* gateway : gateways)
            {
                if (gateway->IsInWorld() && gateway->GetOwnerGUID() == caster->GetGUID())
                    gateway->DespawnOrUnsummon(100ms);
            }

            if (WorldLocation const* dest = GetExplTargetDest())
            {
                Position pos = dest->GetPosition();
                caster->CastSpell(caster, Spells::DemonicGatewaySummonPurple, true);
                caster->CastSpell(pos, Spells::DemonicGatewaySummonGreen, true);
            }
        }

        void Register() override
        {
            OnEffectLaunch += SpellEffectFn(spell_warl_demonic_gateway::HandleVisual, EFFECT_0, SPELL_EFFECT_SUMMON);
            OnEffectLaunch += SpellEffectFn(spell_warl_demonic_gateway::HandleLaunch, EFFECT_1, SPELL_EFFECT_DUMMY);
            OnCheckCast += SpellCheckCastFn(spell_warl_demonic_gateway::CheckRequirement);
        }
    };

    // 48020 - Demonic Circle: Teleport
    class spell_warl_demonic_circle_teleport : public AuraScript
    {
        void HandleTeleport(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Player* player = GetTarget()->ToPlayer();
            if (!player)
                return;

            GameObject* circle = player->GetGameObject(Spells::DemonicCircleSummon);
            if (!circle)
                return;

            player->NearTeleportTo(circle->GetPositionX(), circle->GetPositionY(), circle->GetPositionZ(), circle->GetOrientation());
            player->RemoveMovementImpairingAuras(false);

            if (player->HasAura(Spells::SoulburnBuff))
                player->CastSpell(player, Spells::SoulburnDemonicCircle, CastSpellExtraArgs(TRIGGERED_FULL_MASK));
        }

        void Register() override
        {
            OnEffectApply += AuraEffectApplyFn(spell_warl_demonic_circle_teleport::HandleTeleport, EFFECT_0, SPELL_AURA_MECHANIC_IMMUNITY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    // 108370 - Soul Leech
    class spell_warl_soul_leech : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::SoulLeechShield });
        }

        bool CheckProc(ProcEventInfo& eventInfo)
        {
            Unit* owner = GetTarget();
            Unit* actor = eventInfo.GetActor();
            if (!owner || !actor)
                return false;

            if (actor != owner && actor->GetOwnerGUID() != owner->GetGUID())
                return false;

            DamageInfo* damageInfo = eventInfo.GetDamageInfo();
            if (!damageInfo || !damageInfo->GetDamage())
                return false;

            SpellInfo const* spellInfo = eventInfo.GetSpellInfo();
            if (spellInfo && spellInfo->IsAffectingArea())
                return false;

            return true;
        }

        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();

            Unit* target = GetTarget();
            DamageInfo* damageInfo = eventInfo.GetDamageInfo();
            if (!target || !damageInfo)
                return;

            Player* player = target->ToPlayer();
            if (!player)
                player = ObjectAccessor::FindPlayer(target->GetOwnerGUID());
            if (!player)
                return;

            int32 const addAbsorb = CalculatePct(damageInfo->GetDamage(), aurEff->GetAmount());

            float maxPerc = 5.0f;
            if (AuraEffect const* capEff = GetAura()->GetEffect(EFFECT_1))
                maxPerc = float(capEff->GetAmount());

            AddSoulLeechAbsorb(player, Spells::SoulLeechShield, addAbsorb, maxPerc);

            if (Pet* pet = player->GetPet())
                AddSoulLeechAbsorb(pet, Spells::SoulLeechShield, addAbsorb, maxPerc);
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_warl_soul_leech::CheckProc);
            OnEffectProc += AuraEffectProcFn(spell_warl_soul_leech::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    // 219272 - Demon Skin
    class spell_warl_demon_skin : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::SoulLeech, Spells::SoulLeechShield });
        }

        void CalcPeriodic(AuraEffect const* /*aurEff*/, bool& isPeriodic, int32& amplitude)
        {
            isPeriodic = true;
            if (amplitude <= 0)
                amplitude = 1 * IN_MILLISECONDS;
        }

        void OnTick(AuraEffect const* aurEff)
        {
            Unit* target = GetTarget();
            if (!target)
                return;

            Aura* soulLeech = target->GetAura(Spells::SoulLeech);
            if (!soulLeech)
                return;

            Player* player = target->ToPlayer();
            if (!player)
                player = ObjectAccessor::FindPlayer(target->GetOwnerGUID());
            if (!player)
                return;

            float const perc = float(aurEff->GetAmount()) / 10.0f;
            int32 const addAbsorb = int32(CalculatePct(target->GetMaxHealth(), perc));

            float maxPerc = 5.0f;
            if (AuraEffect const* capEff = soulLeech->GetEffect(EFFECT_1))
                maxPerc = float(capEff->GetAmount());

            AddSoulLeechAbsorb(player, Spells::SoulLeechShield, addAbsorb, maxPerc);

            if (Pet* pet = player->GetPet())
                AddSoulLeechAbsorb(pet, Spells::SoulLeechShield, addAbsorb, maxPerc);
        }

        void Register() override
        {
            DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(spell_warl_demon_skin::CalcPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_warl_demon_skin::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }
    };
}

void AddSC_custom_warlock_spell_fixes()
{
    using namespace Scripts::Custom::Warlock;

    RegisterSpellScript(spell_warl_avatar_of_destruction);
    RegisterSpellScript(spell_warlock_call_dreadstalkers);
    RegisterSpellScript(spell_warlock_call_dreadstalkers_summon);
    RegisterSpellScript(spell_warlock_summon_demonic_tyrant);
    RegisterSpellScript(spell_warl_healthstone_soulburn);
    RegisterSpellScript(spell_warl_soulburn_drain_life_absorb);
    RegisterSpellScript(spell_warl_drain_life_soulburn);
    RegisterSpellScript(spell_warl_demonic_gateway);
    RegisterSpellScript(spell_warl_demonic_circle_teleport);
    RegisterSpellScript(spell_warl_soul_leech);
    RegisterSpellScript(spell_warl_demon_skin);
}
