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
#include "SpellInfo.h"
#include "SpellScript.h"
#include "Player.h"
#include "Creature.h"
#include "ObjectAccessor.h"
#include "EventProcessor.h"

#include "Custom_TheWanderingIsle_Defines.h"

namespace Scripts::TheWanderingIsle::Spells
{
    // 109090, 109095, 109105, 109109
    class spell_fan_the_flames_throw_wood_and_all_blow_air : public SpellScript
    {
        void SelectTarget(WorldObject*& target)
        {
            Player* caster = GetCaster()->ToPlayer();
            if (!caster)
                return;

            Creature* huo = caster->FindNearestCreatureWithOptions(10.0f, { .StringId = "Huo_Pre_Ignition" });

            if (!huo)
                return;

            target = huo;
        }

        void Register() override
        {
            OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_fan_the_flames_throw_wood_and_all_blow_air::SelectTarget, EFFECT_0, TARGET_UNIT_NEARBY_ENTRY);
        }
    };

    // 102522
    class spell_fan_the_flames : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Defines::SpellsQ29422::spell_fan_the_flames_throw_wood, Defines::SpellsQ29422::spell_fan_the_flames_blow_air,
                Defines::SpellsQ29422::spell_fan_the_flames_blow_air_big, Defines::SpellsQ29422::spell_fan_the_flames_blow_air_bigger });
        }

        void HandleAfterCast()
        {
            Player* player = GetCaster()->ToPlayer();
            if (!player)
                return;

            Creature* huo = player->FindNearestCreatureWithOptions(10.0f, { .StringId = "Huo_Pre_Ignition" });
            if (!huo)
                return;

            auto schedule = [&](Milliseconds delay, uint32 spellId)
                {
                    player->m_Events.AddEvent(
                        new LambdaBasicEvent([player, huo, spellId]()
                            {
                                if (player->IsInWorld() && huo->IsInWorld())
                                    player->CastSpell(huo, spellId, true);
                            }),
                        player->m_Events.CalculateTime(delay)
                    );
                };

            // Sequence
            schedule(1s, Defines::SpellsQ29422::spell_fan_the_flames_throw_wood);
            schedule(2s, Defines::SpellsQ29422::spell_fan_the_flames_blow_air);
            schedule(3s, Defines::SpellsQ29422::spell_fan_the_flames_blow_air_big);
            schedule(4s, Defines::SpellsQ29422::spell_fan_the_flames_blow_air_bigger);

            // Final credit
            player->m_Events.AddEvent(
                new LambdaBasicEvent([player]()
                    {
                        if (player->IsInWorld())
                            player->CastSpell(player, Defines::SpellsQ29422::spell_fan_the_flames_credit, true);
                    }),
                player->m_Events.CalculateTime(7s)
            );
        }

        void Register() override
        {
            AfterCast += SpellCastFn(spell_fan_the_flames::HandleAfterCast);
        }   
    };

    // 128700
    class spell_summon_fire_spirit : public SpellScript
    {
        void SelectTarget(WorldObject*& target)
        {
            Player* caster = GetCaster()->ToPlayer();
            if (!caster)
                return;

            Creature* huo = caster->FindNearestCreatureWithOptions(15.0f, { .StringId = "Huo_Pre_Ignition", .IgnorePhases = true });

            if (!huo)
                return;

            target = huo;
        }

        void Register() override
        {
            OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_summon_fire_spirit::SelectTarget, EFFECT_0, TARGET_DEST_NEARBY_ENTRY);
        }
    };

    // 109178
    class spell_despawn_fire_spirit : public SpellScript
    {
        void HandleHitTarget(SpellEffIndex /*effIndex*/)
        {
            if (Unit* huo = GetHitUnit()->FindNearestCreature(Defines::Npcs::npc_huo_q29423, GetSpellInfo()->GetMaxRange(), true))
                huo->ToCreature()->DespawnOrUnsummon();
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_despawn_fire_spirit::HandleHitTarget, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    // 116190 - Summon Child 1
    class spell_summon_child_1 : public SpellScript
    {
        void SetDest(SpellDestination& dest) const
        {
            dest.Relocate(Defines::PositionsQ29521::CaiSpawnPos);
        }

        void Register() override
        {
            OnDestinationTargetSelect += SpellDestinationTargetSelectFn(spell_summon_child_1::SetDest, EFFECT_0, TARGET_DEST_NEARBY_ENTRY);
        }
    };

    // 116191 - Summon Child 2
    class spell_summon_child_2 : public SpellScript
    {
        void SetDest(SpellDestination& dest) const
        {
            dest.Relocate(Defines::PositionsQ29521::DengSpawnPos);
        }

        void Register() override
        {
            OnDestinationTargetSelect += SpellDestinationTargetSelectFn(spell_summon_child_2::SetDest, EFFECT_0, TARGET_DEST_NEARBY_ENTRY);
        }
    };

    // 108798 spell_jojo_headbash_stack_of_reeds_impact
    class spell_jojo_headbash_filter : public SpellScript
    {
        void FilterTargets(std::list<WorldObject*>& targets)
        {
            targets.remove_if([](WorldObject* target)
                {
                    if (target->GetEntry() != Defines::Npcs::npc_stack_of_reeds)
                    {
                        return true;
                    }
                    return false;
                });
        }

        void Register() override
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_jojo_headbash_filter::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENTRY);
        }
    };

    // 108786
    class spell_summon_stack_of_reeds : public SpellScript
    {

        void SetDest(SpellDestination& dest) const
        {
            dest.Relocate(Defines::PositionsQ29662::StackOfReedsSpawnPoint);
        }

        void Register() override
        {
            OnDestinationTargetSelect += SpellDestinationTargetSelectFn(spell_summon_stack_of_reeds::SetDest, EFFECT_0, TARGET_DEST_NEARBY_ENTRY);
        }
    };

    // 108808
    class spell_summon_jojo_ironbrow : public SpellScript
    {

        void SetDest(SpellDestination& dest) const
        {
            dest.Relocate(Defines::PositionsQ29662::JojoSpawnPoint);
        }

        void Register() override
        {
            OnDestinationTargetSelect += SpellDestinationTargetSelectFn(spell_summon_jojo_ironbrow::SetDest, EFFECT_0, TARGET_DEST_NEARBY_ENTRY);
        }
    };
}

void AddSC_custom_the_wandering_isle_spells()
{
    using namespace Scripts::TheWanderingIsle::Spells;
    RegisterSpellScript(spell_fan_the_flames);
    RegisterSpellScript(spell_fan_the_flames_throw_wood_and_all_blow_air);
    RegisterSpellScript(spell_summon_fire_spirit);
    RegisterSpellScript(spell_despawn_fire_spirit);
    RegisterSpellScript(spell_summon_child_1);
    RegisterSpellScript(spell_summon_child_2);
    RegisterSpellScript(spell_jojo_headbash_filter);
    RegisterSpellScript(spell_summon_stack_of_reeds);
    RegisterSpellScript(spell_summon_jojo_ironbrow);
}
