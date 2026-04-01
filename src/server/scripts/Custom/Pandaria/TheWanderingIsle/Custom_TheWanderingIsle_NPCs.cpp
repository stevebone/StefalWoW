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

#include "Containers.h"
#include "Creature.h"
#include "EventMap.h"
#include "GameObject.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "Position.h"
#include "ScriptedCreature.h"
#include "SpellInfo.h"
#include "TemporarySummon.h"
#include "Unit.h"
#include "Vehicle.h"

#include "Custom_TheWanderingIsle_Defines.h"

namespace Scripts::Custom::TheWanderingIsle
{
    //54958
    struct npc_huo_follower : public ScriptedAI
    {
        npc_huo_follower(Creature* creature) : ScriptedAI(creature) {}

        void IsSummonedBy(WorldObject* summoner) override
        {
            if (Player* player = summoner->ToPlayer())
            {
                _playerGuid = player->GetGUID();

                me->SetFloating(false);
                me->SetSpeed(MOVE_RUN, 7.0f);
                me->GetMotionMaster()->MoveFollow(player, 3.0f);

                _events.RescheduleEvent(EventsQ29423::event_delivery_huo, 20s);
            }
        }

        void MovementInform(uint32 type, uint32 pointId) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid);

            if (pointId == 1)
            {
                me->CastSpell(player, SpellsQ29423::spell_fire_form);

                _events.RescheduleEvent(EventsQ29423::event_second_huo_position, 7s);
            }
            else if (pointId == 2)
            {
                me->CastSpell(player, SpellsQ29423::spell_forcecast_fire_turn_in_statue_brazier_change);
                PhasingHandler::OnConditionChange(player);
                me->DespawnOrUnsummon();
            }
        }

        void UpdateAI(uint32 diff) override
        {
            Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid);
            if (!player || !player->IsInWorld())
                me->DespawnOrUnsummon();

            _events.Update(diff);
            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EventsQ29423::event_delivery_huo:
                {
                    Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid);

                    if (player->GetDistance(973.311829f, 3603.419434f, 195.528030f) <= 7.0f)
                    {
                        me->GetMotionMaster()->Remove(FOLLOW_MOTION_TYPE);
                        me->SetFloating(true);
                        me->SetSpeed(MOVE_RUN, 3.0f);
                        me->GetMotionMaster()->MovePoint(1, PositionsQ29423::huoFirstPoint, true, { 6.249388f });
                    }
                    else
                        _events.RescheduleEvent(EventsQ29423::event_delivery_huo, 2s);
                    break;
                }
                case EventsQ29423::event_second_huo_position:
                    me->GetMotionMaster()->MovePoint(2, PositionsQ29423::huoSecondPoint);
                    break;
                default:
                    break;
                }
            }
        }
    private:
        ObjectGuid _playerGuid;

        EventMap _events;
    };

    struct npc_shanxi_quest : public ScriptedAI
    {
        npc_shanxi_quest(Creature* creature) : ScriptedAI(creature) {}

        void OnQuestReward(Player* player, Quest const* quest, LootItemType /*type*/, uint32 /*opt*/) override
        {
            if (quest->GetQuestId() == Quests::quest_the_passion_of_shen_zin_su)
            {
                Creature* ji = player->FindNearestCreatureWithOptions(15.0f, { .CreatureId = Npcs::npc_ji_q29423, .IgnorePhases = true });
                Creature* aysa = player->FindNearestCreatureWithOptions(15.0f, { .CreatureId = Npcs::npc_aysa_q29423, .IgnorePhases = true });

                if (!ji || !aysa)
                    return;

                Talk(TalksQ29423::shanxi_talk_1, player);

                _scheduler.Schedule(10s, [this](TaskContext /*task*/)
                    {
                        Talk(TalksQ29423::shanxi_talk_2);
                    });
                _scheduler.Schedule(26s, [this](TaskContext /*task*/)
                    {
                        Talk(TalksQ29423::shanxi_talk_3);
                    });
                _scheduler.Schedule(40s, [this](TaskContext /*task*/)
                    {
                        Talk(TalksQ29423::shanxi_talk_4);
                    });
                _scheduler.Schedule(49s, [ji](TaskContext /*task*/)
                    {
                        ji->AI()->Talk(TalksQ29423::ji_talk);
                        ji->GetMotionMaster()->MovePath(PathQ29423::ji_away, false);
                        ji->DespawnOrUnsummon(20s);
                    });
                _scheduler.Schedule(52s, [this](TaskContext /*task*/)
                    {
                        Talk(TalksQ29423::shanxi_talk_5);
                    });
                _scheduler.Schedule(60s, [aysa](TaskContext /*task*/)
                    {
                        aysa->AI()->Talk(TalksQ29423::aysa_talk);
                        aysa->GetMotionMaster()->MovePath(PathQ29423::aysa_away, false);
                        aysa->DespawnOrUnsummon(20s);
                    });
                _scheduler.Schedule(63s, [this, player](TaskContext /*task*/)
                    {
                        Talk(TalksQ29423::shanxi_talk_6, player);
                    });
            }
        }

        void UpdateAI(uint32 diff) override
        {
            _scheduler.Update(diff);
        }

    private:
        TaskScheduler _scheduler;
    };

    struct npc_chia_hui_autumnleaf : public ScriptedAI
    {
        npc_chia_hui_autumnleaf(Creature* creature) : ScriptedAI(creature) {}

        void SpellHit(WorldObject* caster, SpellInfo const* spellInfo) override
        {
            if (spellInfo->Id != SpellsQ29423::spell_start_talk_event)
                return;

            if (Player* player = caster->ToPlayer())
            {
                Creature* lin = me->FindNearestCreatureWithOptions(15.0f, { .StringId = "Brewer_Lin_Talk_Event", .IgnorePhases = true });
                Creature* zhen = me->FindNearestCreatureWithOptions(15.0f, { .StringId = "Brewer_Zhen_Talk_Event", .IgnorePhases = true });

                if (!lin || !zhen)
                    return;

                me->SetFacingToObject(player);
                lin->SetFacingToObject(player);
                zhen->SetFacingToObject(player);

                me->AI()->Talk(TalksQ29423::chia_hui_autumnleaf_talk, player);

                _scheduler.Schedule(2s, [this, lin, zhen, player](TaskContext /*task*/)
                    {
                        me->SetFacingToObject(player);
                        lin->SetFacingToObject(player);
                        zhen->SetFacingToObject(player);

                        lin->AI()->Talk(TalksQ29423::brewer_lin_talk);
                    });
                _scheduler.Schedule(4s, [this, lin, zhen](TaskContext /*task*/)
                    {
                        me->SetFacingTo(2.381570f);
                        lin->SetFacingTo(5.455640f);
                        zhen->SetFacingTo(4.906480f);
                    });
            }
        }

        void UpdateAI(uint32 diff) override
        {
            _scheduler.Update(diff);
        }

    private:
        TaskScheduler _scheduler;
    };

    // 60250 - Cai
    struct npc_cai : public ScriptedAI
    {
        npc_cai(Creature* creature) : ScriptedAI(creature) {}

        void IsSummonedBy(WorldObject* summoner) override
        {
            Player* player = summoner->ToPlayer();
            if (!player)
                return;

            me->GetMotionMaster()->MoveFollow(player, 2.0f, M_PI / 4);

            _scheduler.Schedule(7s, [this](TaskContext /*task*/)
                {
                    me->AI()->Talk(TalksQ29521::CaiTalk0);
                });
            _scheduler.Schedule(21s, [this](TaskContext /*task*/)
                {
                    me->AI()->Talk(TalksQ29521::CaiTalk1);
                });
            _scheduler.Schedule(37s, [this](TaskContext /*task*/)
                {
                    me->AI()->Talk(TalksQ29521::CaiTalk2);
                });
            _scheduler.Schedule(45s, [this](TaskContext /*task*/)
                {
                    TempSummon* summon = me->ToTempSummon();
                    if (!summon)
                        return;

                    Unit* summoner = summon->GetSummonerUnit();
                    if (!summoner)
                        return;

                    me->GetMotionMaster()->MoveFleeing(summoner);
                    me->DespawnOrUnsummon(3s);
                });
        }

        void UpdateAI(uint32 diff) override
        {
            _scheduler.Update(diff);
        }

    private:
        TaskScheduler _scheduler;
    };

    // 60249 - Deng
    struct npc_deng : public ScriptedAI
    {
        npc_deng(Creature* creature) : ScriptedAI(creature) {}

        void IsSummonedBy(WorldObject* summoner) override
        {
            Player* player = summoner->ToPlayer();
            if (!player)
                return;

            me->GetMotionMaster()->MoveFollow(player, 2.0f, M_PI / 2);

            _scheduler.Schedule(14s, [this](TaskContext /*task*/)
                {
                    Talk(TalksQ29521::DengTalk0);
                });
            _scheduler.Schedule(29s, [this](TaskContext /*task*/)
                {
                    Talk(TalksQ29521::DengTalk1);
                });
            _scheduler.Schedule(45s, [this](TaskContext /*task*/)
                {
                    TempSummon* summon = me->ToTempSummon();
                    if (!summon)
                        return;

                    Unit* summoner = summon->GetSummonerUnit();
                    if (!summoner)
                        return;

                    Talk(TalksQ29521::DengTalk2);
                    me->GetMotionMaster()->MoveFleeing(summoner);
                    me->DespawnOrUnsummon(3s);
                });
        }

        void UpdateAI(uint32 diff) override
        {
            _scheduler.Update(diff);
        }

    private:
        TaskScheduler _scheduler;
    };


    // 57638
    struct npc_jojo_ironbrow_summon : public ScriptedAI
    {
        npc_jojo_ironbrow_summon(Creature* creature) : ScriptedAI(creature) {}

        void IsSummonedBy(WorldObject* summoner) override
        {
            if (Player* player = summoner->ToPlayer())
            {
                _playerGuid = player->GetGUID();

                me->SetAIAnimKitId(Misc::Jojo_AiAnimKitID);

                _scheduler.Schedule(1s, [this, player](TaskContext /*task*/)
                    {
                        Talk(TalksQ29662::Jojo_Talk_0, player);
                    });
                _scheduler.Schedule(3s, [this](TaskContext /*task*/)
                    {
                        me->SetAIAnimKitId(0);
                    });
                _scheduler.Schedule(4s, [this](TaskContext /*task*/)
                    {
                        me->GetMotionMaster()->MovePoint(1, PositionsQ29662::JojoMovePoint);
                    });
                _scheduler.Schedule(6200ms, [this](TaskContext /*task*/)
                    {
                        me->CastSpell(me, SpellsQ29662::spell_jojo_headbash_reeds_cast);
                    });
                _scheduler.Schedule(8700ms, [this, player](TaskContext /*task*/)
                    {
                        me->RemoveAurasDueToSpell(SpellsQ29662::spell_jojo_headbash_stack_of_reeds_impact);
                        Talk(TalksQ29662::Jojo_Talk_1, player);
                    });
                _scheduler.Schedule(14700ms, [this](TaskContext /*task*/)
                    {
                        me->GetMotionMaster()->MovePath(PathQ29662::path_jojo, false);
                    });
            }
        }

        void WaypointPathEnded(uint32 /*nodeId*/, uint32 pathId) override
        {
            if (pathId == PathQ29662::path_jojo)
            {
                me->DespawnOrUnsummon();
            }
        }

        void UpdateAI(uint32 diff) override
        {
            _scheduler.Update(diff);
        }

    private:
        ObjectGuid _playerGuid;
        TaskScheduler _scheduler;
    };

    // 55015 aggro on frog pool
    struct npc_whitefeather_crane : public ScriptedAI
    {
        npc_whitefeather_crane(Creature* creature) : ScriptedAI(creature) {}

        void Reset() override
        {
            _events.Reset();
            _events.RescheduleEvent(EventsQ29662::event_check_players, 1s);
        }

        void EnterCombat()
        {
            _events.CancelEvent(EventsQ29662::event_check_players);
            _events.RescheduleEvent(EventsQ29662::event_cast_razor_beak, 8s);
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            if (!UpdateVictim())
            {
                if (_events.ExecuteEvent() == EventsQ29662::event_check_players)
                {
                    if (Player* player = me->SelectNearestPlayer(10.0f))
                    {
                        if (player->IsAlive() && !player->IsGameMaster() && player->HasAura(SpellsQ29662::spell_curse_of_the_frog))
                        {
                            AttackStart(player);
                            EnterCombat();
                            return;
                        }
                    }
                    _events.RescheduleEvent(EventsQ29662::event_check_players, 1s);
                }
                return;
            }

            if (Unit* victim = me->GetVictim())
            {
                if (!victim->HasAura(SpellsQ29662::spell_curse_of_the_frog))
                {
                    if (me->IsInCombat())
                    {
                        EnterEvadeMode();
                        _events.RescheduleEvent(EventsQ29662::event_check_players, 1s);
                    }
                    return;
                }
            }

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EventsQ29662::event_cast_razor_beak:
                    if (Unit* target = me->GetVictim())
                    {
                        DoCast(target, SpellsQ29662::spell_razor_beak);
                    }
                    _events.RescheduleEvent(EventsQ29662::event_cast_razor_beak, 8s);
                    break;
                }
            }

            me->DoMeleeAttackIfReady();
        }
    private:
        EventMap _events;
    };

    // 55019 & 65468
    struct npc_tushui_monk_on_pole : public ScriptedAI
    {
        npc_tushui_monk_on_pole(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            _events.ScheduleEvent(EventsQ29661Q29663::event_monk_switch_pole, 1s);
            me->RestoreFaction();
            me->SetReactState(REACT_DEFENSIVE);
        }

        void SpellHit(WorldObject* caster, SpellInfo const* spellInfo) override
        {
            if (spellInfo->Id == SpellsQ29661Q29663::spell_force_cast_ride_pole)
            {
                if (Unit* unitCaster = caster->ToUnit())
                    DoCast(unitCaster, SpellsQ29661Q29663::spell_monk_ride_pole, true);
            }
        }

        void JustEnteredCombat(Unit* who) override
        {
            me->SetReactState(REACT_AGGRESSIVE);
            AttackStart(who);

            _events.ScheduleEvent(EventsQ29661Q29663::event_cast_throw_rock, 0s);
        }

        void MovementInform(uint32 type, uint32 pointId) override
        {
            if (type != EFFECT_MOTION_TYPE)
                return;

            if (pointId == EVENT_JUMP)
            {
                me->GetMotionMaster()->Clear();          // stop all movement
                me->StopMoving();
                me->KillSelf(false);
            }
        }

        void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (damage >= me->GetHealth())
            {
                damage = 0; // Prevent lethal damage
                me->SetHealth(10);
                _events.Reset();
                me->RemoveAllAuras();
                me->SetFaction(35);
                me->SetUnitFlag(UnitFlags(UNIT_FLAG_CAN_SWIM | UNIT_FLAG_IMMUNE_TO_PC));
                me->AttackStop();
                attacker->AttackStop();
                me->ExitVehicle();
                attacker->ToPlayer()->KilledMonsterCredit(Npcs::npc_monk_on_pole_1);
                Position dest = me->GetNearPosition(3.0f, 0.0f);
                me->GetMotionMaster()->MoveJump(EVENT_JUMP, dest, 10.0f, 10.0f);
                me->DespawnOrUnsummon(5s);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            Unit* victim = nullptr;
            if (Unit* rawVictim = me->GetVictim())
            {
                if (rawVictim->IsVehicle() && rawVictim->GetVehicleKit()->GetPassenger(0))
                    victim = rawVictim->GetVehicleKit()->GetPassenger(0);
                else
                    victim = rawVictim;
            }

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EventsQ29661Q29663::event_cast_throw_rock:
                {
                    if (victim && !me->IsWithinMeleeRange(victim))
                        DoCast(victim, SpellsQ29661Q29663::spell_throw_rock);
                }
                _events.ScheduleEvent(EventsQ29661Q29663::event_cast_throw_rock, 2500ms);
                break;
                case EventsQ29661Q29663::event_monk_switch_pole:
                    if (!me->IsInCombat())
                    {
                        SwitchPole();
                        _events.ScheduleEvent(EventsQ29661Q29663::event_monk_switch_pole, 15s, 30s);
                    }
                    break;
                }
            }
            AttackStart(victim);
        }

    private:
        EventMap _events;

        void SwitchPole()
        {
            std::vector<Creature*> poles;
            me->GetCreatureListWithEntryInGrid(poles, Npcs::npc_training_pole_1, 5.0f);
            me->GetCreatureListWithEntryInGrid(poles, Npcs::npc_training_pole_1, 5.0f);

            if (poles.empty())
                return;

            Trinity::Containers::RandomShuffle(poles);

            for (Creature* pole : poles)
            {
                if (!me->IsWithinDist2d(pole, 5.0f))
                    continue;

                if (!pole->HasAura(SpellsQ29661Q29663::spell_monk_ride_pole) && !pole->HasAura(SpellsQ29661Q29663::spell_ride_vehicle_pole))
                {
                    pole->CastSpell(me, SpellsQ29661Q29663::spell_force_cast_ride_pole, true);
                    break;
                }
            }
        }

        void AttackStart(Unit* who) override
        {
            if (!who)
                return;

            // If the attacker is riding a pole, target the rider
            if (who->IsVehicle() && who->GetVehicleKit())
            {
                if (Unit* rider = who->GetVehicleKit()->GetPassenger(0))
                {
                    me->Attack(rider, true);
                    return;
                }
            }

            me->Attack(who, true);
        }
    };

    // 54993, 57431, 55083
    struct npc_balance_pole : public ScriptedAI
    {
        npc_balance_pole(Creature* creature) : ScriptedAI(creature)
        {
            me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_NON_ATTACKABLE_2);
            me->SetNpcFlag(UNIT_NPC_FLAG_SPELLCLICK | UNIT_NPC_FLAG_PLAYER_VEHICLE);
        }

        void PassengerBoarded(Unit* passenger, int8 /*seat*/, bool apply) override
        {
            if (passenger->GetTypeId() == TYPEID_PLAYER)
            {
                _passengerGuid = passenger->GetGUID();
                if (!apply)
                    _events.ScheduleEvent(EventsQ29661Q29663::event_cast_transform, 1s);
                else
                {
                    if (me->GetEntry() == Npcs::npc_training_bell_pole)
                        DoCast(passenger, SpellsQ29661Q29663::spell_training_bell_force_cast_ride_vehicle, true);

                    passenger->AddAura(SpellsQ29661Q29663::spell_ride_vehicle_pole, passenger);
                    passenger->RemoveAurasDueToSpell(SpellsQ29661Q29663::spell_curse_of_the_frog);
                }
            }
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EventsQ29661Q29663::event_cast_transform:
                    // Transform is casted only when in frog pool
                    Unit* passenger = ObjectAccessor::GetUnit(*me, _passengerGuid);

                    if (passenger->HasAura(SpellsQ29661Q29663::spell_training_bell_exclusion_aura))
                        passenger->RemoveAura(SpellsQ29661Q29663::spell_training_bell_exclusion_aura);

                    if(passenger->HasAura(SpellsQ29661Q29663::spell_ride_vehicle_pole))
                        passenger->RemoveAura(SpellsQ29661Q29663::spell_ride_vehicle_pole);

                    if (passenger->HasAura(SpellsQ29661Q29663::spell_curse_of_the_frog))
                        passenger->RemoveAura(SpellsQ29661Q29663::spell_curse_of_the_frog);
                    _passengerGuid.Clear(); // ? Clear after you've finished handling the passenger
                    break;
                }
            }
        }

    private:
        EventMap _events;
        ObjectGuid _passengerGuid;
    };

    // 55292 - Fang-she
    struct npc_fang_she : public ScriptedAI
    {
        npc_fang_she(Creature* creature) : ScriptedAI(creature) { }

        EventMap events;

        void Reset() override
        {
            events.Reset();
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                if (eventId == EventsQ29677::event_cast_serpent_strike)
                {
                    DoCastVictim(128409); // Fang-she's attack spell
                    events.ScheduleEvent(EventsQ29677::event_cast_serpent_strike, 3s, 5s); // repeat
                }
            }
        }

        void JustEngagedWith(Unit* who) override // Runs every time creature gets in combat
        {
            me->EngageWithTarget(who);
            events.ScheduleEvent(EventsQ29677::event_cast_serpent_strike, 3s, 5s);
        }

        void JustDied(Unit* killer) override
        {
            Player* player = killer->ToPlayer();
            if (!player)
                return;

            // Only activate the pearl if the player has the quest
            if (player->GetQuestStatus(Quests::quest_the_sun_pearl) == QUEST_STATUS_INCOMPLETE)
            {
                if (GameObject* pearl = me->FindNearestGameObject(Objects::go_ancient_clam, 20.0f))
                {
                    pearl->RemoveFlag(GO_FLAG_INTERACT_COND);
                    pearl->SetDynamicFlag(GO_DYNFLAG_LO_ACTIVATE);
                }
            }
        }
    };
}

void AddSC_custom_the_wandering_isle_npcs()
{
    using namespace Scripts::Custom::TheWanderingIsle;
    RegisterCreatureAI(npc_huo_follower);
    RegisterCreatureAI(npc_chia_hui_autumnleaf);
    RegisterCreatureAI(npc_shanxi_quest);
    RegisterCreatureAI(npc_deng);
    RegisterCreatureAI(npc_cai);
    RegisterCreatureAI(npc_whitefeather_crane);
    RegisterCreatureAI(npc_jojo_ironbrow_summon);
    RegisterCreatureAI(npc_tushui_monk_on_pole);
    RegisterCreatureAI(npc_balance_pole);
    RegisterCreatureAI(npc_fang_she);
}
