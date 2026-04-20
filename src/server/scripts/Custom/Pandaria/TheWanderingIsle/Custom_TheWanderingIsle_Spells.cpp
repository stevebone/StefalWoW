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

#include "Creature.h"
#include "DB2Stores.h"
#include "EventProcessor.h"
#include "GameObject.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "SpellInfo.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "Unit.h"
#include "WorldStateMgr.h"

#include "Custom_TheWanderingIsle_Defines.h"

namespace Scripts::Custom::TheWanderingIsle
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
            return ValidateSpellInfo({ Spells::spell_fan_the_flames_throw_wood, Spells::spell_fan_the_flames_blow_air,
                Spells::spell_fan_the_flames_blow_air_big, Spells::spell_fan_the_flames_blow_air_bigger });
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
            schedule(1s, Spells::spell_fan_the_flames_throw_wood);
            schedule(2s, Spells::spell_fan_the_flames_blow_air);
            schedule(3s, Spells::spell_fan_the_flames_blow_air_big);
            schedule(4s, Spells::spell_fan_the_flames_blow_air_bigger);

            // Final credit
            player->m_Events.AddEvent(
                new LambdaBasicEvent([player]()
                    {
                        if (player->IsInWorld())
                            player->CastSpell(player, Spells::spell_fan_the_flames_credit, true);
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
            if (Unit* huo = GetHitUnit()->FindNearestCreature(Npcs::npc_huo_q29423, GetSpellInfo()->GetMaxRange(), true))
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
            dest.Relocate(PositionsQ29521::CaiSpawnPos);
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
            dest.Relocate(PositionsQ29521::DengSpawnPos);
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
                    if (target->GetEntry() != Npcs::npc_stack_of_reeds)
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
            dest.Relocate(PositionsQ29662::StackOfReedsSpawnPoint);
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
            dest.Relocate(PositionsQ29662::JojoSpawnPoint);
        }

        void Register() override
        {
            OnDestinationTargetSelect += SpellDestinationTargetSelectFn(spell_summon_jojo_ironbrow::SetDest, EFFECT_0, TARGET_DEST_NEARBY_ENTRY);
        }
    };

    // 103069
    class spell_rock_jump_a : public SpellScript
    {
        void HandleJumpDest(SpellEffIndex effIndex)
        {
            PreventHitDefaultEffect(effIndex);

            if (Unit* caster = GetCaster())
            {
                if (caster->GetPositionZ() > 92.0f)
                {
                    caster->GetMotionMaster()->MoveJump(EVENT_JUMP, PositionsQ29678::RockJumpFinal, 5.f, 5.f);
                }
                else
                {
                    if (GameObject* go = caster->FindNearestGameObject(Objects::go_rock_jump_b, 8.0f))
                        caster->GetMotionMaster()->MoveJump(EVENT_JUMP, go->GetPosition(), 5.f, 5.f);
                }
            }
        }

        void Register() override
        {
            OnEffectHit += SpellEffectFn(spell_rock_jump_a::HandleJumpDest, EFFECT_0, SPELL_EFFECT_JUMP_DEST);
        }
    };

    // 103070
    class spell_rock_jump_b : public SpellScript
    {
        void HandleJumpDest(SpellEffIndex effIndex)
        {
            PreventHitDefaultEffect(effIndex);

            if (Unit* caster = GetCaster())
                if (GameObject* go = caster->FindNearestGameObject(Objects::go_rock_jump_c, 10.0f))
                    caster->GetMotionMaster()->MoveJump(EVENT_JUMP, go->GetPosition(), 5.f, 5.f);
        }

        void Register() override
        {
            OnEffectHit += SpellEffectFn(spell_rock_jump_b::HandleJumpDest, EFFECT_0, SPELL_EFFECT_JUMP_DEST);
        }
    };

    // 103077
    class spell_rock_jump_c : public SpellScript
    {
        void HandleJumpDest(SpellEffIndex effIndex)
        {
            PreventHitDefaultEffect(effIndex);

            if (Unit* caster = GetCaster())
                if (GameObject* go = caster->FindNearestGameObject(Objects::go_rock_jump_a, 10.0f))
                    caster->GetMotionMaster()->MoveJump(EVENT_JUMP, go->GetPosition(), 5.f, 5.f);
        }

        void Register() override
        {
            OnEffectHit += SpellEffectFn(spell_rock_jump_c::HandleJumpDest, EFFECT_0, SPELL_EFFECT_JUMP_DEST);
        }
    };

    class spell_jump_to_front_right : public SpellScript
    {
        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            if (Unit* caster = GetCaster())
                caster->GetMotionMaster()->MoveJump(EVENT_JUMP, PositionsQ29679::JumpToFrontRight, 12.f, 15.f);
        }

        void Register() override
        {
            OnEffectHit += SpellEffectFn(spell_jump_to_front_right::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    class spell_jump_to_front_left : public SpellScript
    {
        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            if (Unit* caster = GetCaster())
                caster->GetMotionMaster()->MoveJump(EVENT_JUMP, PositionsQ29679::JumpToFrontLeft, 12.f, 15.f);
        }

        void Register() override
        {
            OnEffectHit += SpellEffectFn(spell_jump_to_front_left::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    class spell_jump_to_back_right : public SpellScript
    {
        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            if (Unit* caster = GetCaster())
                caster->GetMotionMaster()->MoveJump(EVENT_JUMP, PositionsQ29679::JumpToBackRight, 12.f, 15.f);
        }

        void Register() override
        {
            OnEffectHit += SpellEffectFn(spell_jump_to_back_right::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    class spell_jump_to_back_left : public SpellScript
    {
        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            if (Unit* caster = GetCaster())
                caster->GetMotionMaster()->MoveJump(EVENT_JUMP, PositionsQ29679::JumpToBackLeft, 12.f, 15.f);
        }

        void Register() override
        {
            OnEffectHit += SpellEffectFn(spell_jump_to_back_left::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    class spell_aysa_congrats_timer : public AuraScript
    {
        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (Unit* target = GetTarget())
                if (Creature* creature = target->FindNearestCreature(Npcs::npc_aysa_q29679, 70.0f, true))
                    creature->AI()->Talk(0, target);
        }

        void Register() override
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_aysa_congrats_timer::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    class spell_aysa_congrats_trigger_aura : public AuraScript
    {
        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (Unit* target = GetTarget())
                if (Creature* creature = target->FindNearestCreature(Npcs::npc_aysa_q29679, 70.0f, true))
                    creature->AI()->Talk(1, target);
        }

        void Register() override
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_aysa_congrats_trigger_aura::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    // 104450
    class spell_summon_ji_firepaw_temple : public SpellScript
    {
        void SetDest(SpellDestination& dest) const
        {
            dest.Relocate(Positions::JiTempleSpireSpawn);
        }

        void Register() override
        {
            OnDestinationTargetSelect += SpellDestinationTargetSelectFn(spell_summon_ji_firepaw_temple::SetDest, EFFECT_0, TARGET_DEST_NEARBY_ENTRY);
        }
    };

    // 118036
    class spell_summon_spirit_of_earth : public SpellScript
    {
        void SetDest(SpellDestination& dest) const
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            if (caster && caster->ToPlayer()->IsActiveQuest(Quests::quest_not_in_the_face))
                dest.Relocate(Positions::WugouSpawn);
            else dest.Relocate(caster->GetRandomNearPosition(5.f));
        }

        void Register() override
        {
            OnDestinationTargetSelect += SpellDestinationTargetSelectFn(spell_summon_spirit_of_earth::SetDest, EFFECT_0, TARGET_DEST_NEARBY_ENTRY);
        }
    };

    // left 108627
    // right 108691
    // Can't get the ropes to target correctly :(
    /*
    class spell_cart_ropes : public SpellScript
    {
        void SelectTargetDest(WorldObject*& target)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            Creature* yak1 = caster->FindNearestCreature(Npcs::npc_vehicle_ox, 10.f);
            Creature* yak2 = caster->FindNearestCreature(Npcs::npc_vehicle_ox_farmstead, 10.f);

            Unit* yak = nullptr;
            if (yak1)
                yak = yak1;
            else if (yak2)
                yak = yak2;

            if (!yak)
                return;

            target = yak;
        }

        void SelectTargetSource(WorldObject*& target)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            target = caster;
        }

        void Register() override
        {
            OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_cart_ropes::SelectTargetSource, EFFECT_0, TARGET_DEST_CASTER);
            OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_cart_ropes::SelectTargetDest, EFFECT_0, TARGET_UNIT_DEST_AREA_ENTRY);
        }
    };
    */

    // Spell 104126 Monkey Wisdom
    class spell_monkey_wisdom_text : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            if (!sBroadcastTextStore.LookupEntry(MonkeyWisdomTexts::text_monkey_wisdom_1) ||
                !sBroadcastTextStore.LookupEntry(MonkeyWisdomTexts::text_monkey_wisdom_2) ||
                !sBroadcastTextStore.LookupEntry(MonkeyWisdomTexts::text_monkey_wisdom_3) ||
                !sBroadcastTextStore.LookupEntry(MonkeyWisdomTexts::text_monkey_wisdom_4) ||
                !sBroadcastTextStore.LookupEntry(MonkeyWisdomTexts::text_monkey_wisdom_5) ||
                !sBroadcastTextStore.LookupEntry(MonkeyWisdomTexts::text_monkey_wisdom_6) ||
                !sBroadcastTextStore.LookupEntry(MonkeyWisdomTexts::text_monkey_wisdom_7) ||
                !sBroadcastTextStore.LookupEntry(MonkeyWisdomTexts::text_monkey_wisdom_8))
                return false;

            return true;
        }

        bool Load() override
        {
            return GetCaster()->GetTypeId() == TYPEID_PLAYER;
        }

        void HandleScript(SpellEffIndex /*effIndex*/)
        {
            uint32 randomText = urand(0, 7);

            GetCaster()->Talk(MonkeyWisdomTexts::text_monkey_wisdom_1 + randomText, CHAT_MSG_RAID_BOSS_WHISPER, 0.0f, GetHitPlayer());
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_monkey_wisdom_text::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    // 125699
    class spell_ruk_ruk_ooksplosions : public AuraScript
    {
        void HandleEffectPeriodic(AuraEffect const* /*aurEff*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            float radius = caster->GetBoundingRadius();
            float x, y, z;
            caster->GetClosePoint(x, y, z, radius, frand(0.0f, 3.0f), frand(0.0f, 2 * float(M_PI)));

            Position pos(x, y, z);
            CastSpellTargetArg target(pos);

            caster->CastSpell(target, Spells::spell_ookslosions_triggered, true);
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_ruk_ruk_ooksplosions::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    // 105333 - Summon Master Shang Q29787
    class spell_summon_worthy_of_passing : public SpellScript
    {
        void SetDest(SpellDestination& dest) const
        {
            dest.Relocate(PositionsQ29787::MasterShangSpawn);
        }

        void Register() override
        {
            OnDestinationTargetSelect += SpellDestinationTargetSelectFn(spell_summon_worthy_of_passing::SetDest, EFFECT_0, TARGET_DEST_NEARBY_ENTRY);
        }
    };

    // 117973 - Summon Ji at Forlorn hut
    class spell_summon_ji_forlorn_hut : public SpellScript
    {
        void SetDest(SpellDestination& dest) const
        {
            dest.Relocate(Positions::pos_spell_117973);
        }

        void Register() override
        {
            OnDestinationTargetSelect += SpellDestinationTargetSelectFn(spell_summon_ji_forlorn_hut::SetDest, EFFECT_0, TARGET_DEST_NEARBY_ENTRY);
        }
    };

    // 129341
    class spell_rescue_injured_sailor : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo(
                {
                    Spells::spell_rescue_sailor_female,
                    Spells::spell_rescue_sailor_male
                });
        }

        bool Load() override
        {
            return GetCaster() && GetCaster()->GetTypeId() == TYPEID_PLAYER;
        }

        void HandleGenderSelect(SpellEffIndex /*effIndex*/)
        {
            Player* player = GetCaster()->ToPlayer();
            if (!player)
                return;

            uint32 spellId = (player->GetGender() == GENDER_MALE)
                ? Spells::spell_rescue_sailor_male
                : Spells::spell_rescue_sailor_female;

            player->CastSpell(player, spellId, true);
        }

        void Register() override
        {
            OnEffectHit += SpellEffectFn(spell_rescue_injured_sailor::HandleGenderSelect, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    // 108806
    class aura_injured_sailor_feign_death : public AuraScript
    {
        void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* target = GetTarget();
            if (!target)
                return;

            // Retail behavior: fake death + low HP
            target->SetUnitFlag(UNIT_FLAG_PREVENT_EMOTES_FROM_CHAT_TEXT);
            target->SetUnitFlag2(UNIT_FLAG2_FEIGN_DEATH);
        }

        void Register() override
        {
            OnEffectApply += AuraEffectApplyFn(
                aura_injured_sailor_feign_death::OnApply,
                EFFECT_0,
                SPELL_AURA_DUMMY,
                AURA_EFFECT_HANDLE_REAL
            );
        }
    };

    // 117597 - Summon Ji at wreck explosion
    class spell_summon_ji_wreck_explosion : public SpellScript
    {
        void SetDest(SpellDestination& dest) const
        {
            dest.Relocate(Positions::pos_spell_117597);
        }

        void Register() override
        {
            OnDestinationTargetSelect += SpellDestinationTargetSelectFn(spell_summon_ji_wreck_explosion::SetDest, EFFECT_0, TARGET_DEST_NEARBY_ENTRY);
        }
    };

    // 118233
    class spell_turtle_healed_phase_timer : public AuraScript
    {
        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (Unit* target = GetTarget())
                if (target->IsPlayer())
                    target->CastSpell(GetTarget(), Spells::spell_turtle_healed_phase_update, true);
        }

        void Register() override
        {
            AfterEffectRemove += AuraEffectRemoveFn(spell_turtle_healed_phase_timer::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    // 117783
    class spell_healing_shenzin_su : public AuraScript
    {
        void HandleEffectPeriodic(AuraEffect const* /*aurEff*/)
        {
            if (Unit* target = GetTarget())
            {
                Player* player = target->ToPlayer();
                if (!player)
                    return;


                uint32 healers = WorldStateMgr::GetValue(Data::worldstate_healers_active, player->GetMap());

                target->ModifyPower(POWER_ALTERNATE_POWER, healers);

                if (target->GetPowerPct(POWER_ALTERNATE_POWER) >= 100)
                {
                    target->CastSpell(GetTarget(), Spells::spell_healing_shenzinsu_credit, true);
                    target->RemoveAura(GetId());
                }
            }
        }

        void HandleEffectApply(AuraEffect const*, AuraEffectHandleModes)
        {
            if (Unit* target = GetTarget())
            {
                if (Player* player = target->ToPlayer())
                {
                    player->SetMaxPower(POWER_ALTERNATE_POWER, 100);
                    player->SetPower(POWER_ALTERNATE_POWER, 0);
                }
            }
        }

        void Register() override
        {
            OnEffectApply += AuraEffectApplyFn(spell_healing_shenzin_su::HandleEffectApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_healing_shenzin_su::HandleEffectPeriodic, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    // 108897 - Pandaren Faction Choice
    class spell_pandaren_faction_choice : public SpellScript
    {

        void HandleScriptEffect(SpellEffIndex /*effIndex*/)
        {
            if (Unit* caster = GetCaster())
                if (caster->IsPlayer())
                    caster->ToPlayer()->ShowNeutralPlayerFactionSelectUI();
        }

        void Register() override
        {
            OnEffectHit += SpellEffectFn(spell_pandaren_faction_choice::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };
}

void AddSC_custom_the_wandering_isle_spells()
{
    using namespace Scripts::Custom::TheWanderingIsle;

    RegisterSpellScript(spell_fan_the_flames);
    RegisterSpellScript(spell_fan_the_flames_throw_wood_and_all_blow_air);
    RegisterSpellScript(spell_summon_fire_spirit);
    RegisterSpellScript(spell_despawn_fire_spirit);
    RegisterSpellScript(spell_summon_child_1);
    RegisterSpellScript(spell_summon_child_2);
    RegisterSpellScript(spell_jojo_headbash_filter);
    RegisterSpellScript(spell_summon_stack_of_reeds);
    RegisterSpellScript(spell_summon_jojo_ironbrow);
    RegisterSpellScript(spell_rock_jump_a);
    RegisterSpellScript(spell_rock_jump_b);
    RegisterSpellScript(spell_rock_jump_c);
    RegisterSpellScript(spell_jump_to_back_left);
    RegisterSpellScript(spell_jump_to_front_left);
    RegisterSpellScript(spell_jump_to_back_right);
    RegisterSpellScript(spell_jump_to_front_right);
    RegisterSpellScript(spell_aysa_congrats_timer);
    RegisterSpellScript(spell_aysa_congrats_trigger_aura);
    RegisterSpellScript(spell_summon_ji_firepaw_temple);
    RegisterSpellScript(spell_summon_spirit_of_earth);
    //RegisterSpellScript(spell_cart_ropes);
    RegisterSpellScript(spell_monkey_wisdom_text);
    RegisterSpellScript(spell_ruk_ruk_ooksplosions);
    RegisterSpellScript(spell_summon_worthy_of_passing);
    RegisterSpellScript(spell_summon_ji_forlorn_hut);
    RegisterSpellScript(spell_rescue_injured_sailor);
    RegisterSpellScript(aura_injured_sailor_feign_death);
    RegisterSpellScript(spell_summon_ji_wreck_explosion);
    RegisterSpellScript(spell_turtle_healed_phase_timer);
    RegisterSpellScript(spell_healing_shenzin_su);
    RegisterSpellScript(spell_pandaren_faction_choice);
}
