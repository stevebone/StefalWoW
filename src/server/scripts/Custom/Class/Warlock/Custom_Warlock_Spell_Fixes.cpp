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

#include "Log.h"
#include "ScriptMgr.h"
#include "SpellScript.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "Unit.h"

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
}

void AddSC_custom_warlock_spell_fixes()
{
    using namespace Scripts::Custom::Warlock;

    RegisterSpellScript(spell_warl_avatar_of_destruction);
    RegisterSpellScript(spell_warlock_call_dreadstalkers);
    RegisterSpellScript(spell_warlock_call_dreadstalkers_summon);
    RegisterSpellScript(spell_warlock_summon_demonic_tyrant);
}
