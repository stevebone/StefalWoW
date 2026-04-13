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
#include "ObjectGuid.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "Position.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "TemporarySummon.h"
#include "Unit.h"
#include "Vehicle.h"

#include "Custom_TheWanderingIsle_Defines.h"

namespace Scripts::Custom::TheWanderingIsle
{
    //54958
    class npc_huo_follower : public CreatureScript
    {
    public:
        npc_huo_follower() : CreatureScript("npc_huo_follower") { }

        struct npc_huo_followerAI : public ScriptedAI
        {
            npc_huo_followerAI(Creature* creature) : ScriptedAI(creature) { }

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
                {
                    me->DespawnOrUnsummon();
                    return;
                }

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

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_huo_followerAI(creature);
        }
    };

    class npc_shanxi_quest : public CreatureScript
    {
    public:
        npc_shanxi_quest() : CreatureScript("npc_shanxi_quest") { }

        struct npc_shanxi_questAI : public ScriptedAI
        {
            npc_shanxi_questAI(Creature* creature) : ScriptedAI(creature) {}

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

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_shanxi_questAI(creature);
        }
    };

    class npc_chia_hui_autumnleaf : public CreatureScript
    {
    public:
        npc_chia_hui_autumnleaf() : CreatureScript("npc_chia_hui_autumnleaf") { }

        struct npc_chia_hui_autumnleafAI : public ScriptedAI
        {
            npc_chia_hui_autumnleafAI(Creature* creature) : ScriptedAI(creature) {}

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

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_chia_hui_autumnleafAI(creature);
        }
    };

    // 60250 - Cai
    class npc_cai : public CreatureScript
    {
    public:
        npc_cai() : CreatureScript("npc_cai") { }

        struct npc_caiAI : public ScriptedAI
        {
            npc_caiAI(Creature* creature) : ScriptedAI(creature) {}

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

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_caiAI(creature);
        }
    };

    // 60249 - Deng
    class npc_deng : public CreatureScript
    {
    public:
        npc_deng() : CreatureScript("npc_deng") { }

        struct npc_dengAI : public ScriptedAI
        {
            npc_dengAI(Creature* creature) : ScriptedAI(creature) {}

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

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_dengAI(creature);
        }
    };


    // 57638
    class npc_jojo_ironbrow_summon : public CreatureScript
    {
    public:
        npc_jojo_ironbrow_summon() : CreatureScript("npc_jojo_ironbrow_summon") { }

        struct npc_jojo_ironbrow_summonAI : public ScriptedAI
        {
            npc_jojo_ironbrow_summonAI(Creature* creature) : ScriptedAI(creature) {}

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

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_jojo_ironbrow_summonAI(creature);
        }
    };

    // 55015 aggro on frog pool
    class npc_whitefeather_crane : public CreatureScript
    {
    public:
        npc_whitefeather_crane() : CreatureScript("npc_whitefeather_crane") { }

        struct npc_whitefeather_craneAI : public ScriptedAI
        {
            npc_whitefeather_craneAI(Creature* creature) : ScriptedAI(creature) {}

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

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_whitefeather_craneAI(creature);
        }
    };

    // 55019 & 65468
    class npc_tushui_monk_on_pole : public CreatureScript
    {
    public:
        npc_tushui_monk_on_pole() : CreatureScript("npc_tushui_monk_on_pole") { }

        struct npc_tushui_monk_on_poleAI : public ScriptedAI
        {
            npc_tushui_monk_on_poleAI(Creature* creature) : ScriptedAI(creature) { }

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

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_tushui_monk_on_poleAI(creature);
        }
    };

    // 54993, 57431, 55083
    class npc_balance_pole : public CreatureScript
    {
    public:
        npc_balance_pole() : CreatureScript("npc_balance_pole") { }

        struct npc_balance_poleAI : public ScriptedAI
        {
            npc_balance_poleAI(Creature* creature) : ScriptedAI(creature)
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

                        if (passenger->HasAura(SpellsQ29661Q29663::spell_ride_vehicle_pole))
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

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_balance_poleAI(creature);
        }
    };

    // 55292 - Fang-she
    class npc_fang_she : public CreatureScript
    {
    public:
        npc_fang_she() : CreatureScript("npc_fang_she") { }

        struct npc_fang_sheAI : public ScriptedAI
        {
            npc_fang_sheAI(Creature* creature) : ScriptedAI(creature) { }

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

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_fang_sheAI(creature);
        }
    };

    class npc_shu_playing : public CreatureScript
    {
    public:
        npc_shu_playing() : CreatureScript("npc_shu_playing") { }

        struct npc_shu_playingAI : public ScriptedAI
        {
            npc_shu_playingAI(Creature* creature) : ScriptedAI(creature),
                jumpPosition(1),
                //spawnPosition(0),
                positionBefore(1),
                startAI(false) { }

            void Initialize()
            {
                jumpPosition = 1;
                positionBefore = 1;
                startAI = true;
            }

            void Reset() override
            {
                events.Reset();
                Initialize();
            }

            void MovementInform(uint32 type, uint32 id) override
            {
                if (type == EFFECT_MOTION_TYPE && id == EVENT_JUMP)
                    events.ScheduleEvent(EventsQ29679::event_shu_set_orientation, 500ms);
            }

            uint32 GetData(uint32 id) const override
            {
                if (id == Misc::SHU_DATA_JUMP_POSITION)
                    return jumpPosition;

                return false;
            }

            void UpdateAI(uint32 diff) override
            {
                if (startAI)
                {
                    events.ScheduleEvent(EventsQ29679::event_cast_jump_spell, 1s);
                    startAI = false;
                }

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EventsQ29679::event_cast_jump_spell:
                        if (urand(0, 2) != 0)
                            jumpPosition = urand(Misc::SHU_JUMP_POSITION_0, Misc::SHU_JUMP_POSITION_3);
                        else
                            jumpPosition = positionBefore;

                        if (jumpPosition == positionBefore)
                        {
                            events.CancelEvent(EventsQ29679::event_shu_set_orientation);
                            events.ScheduleEvent(EventsQ29679::event_shu_summon_water_spout, 1500ms);
                        }
                        else
                        {
                            DoCast(jumpSpells[jumpPosition]);
                            positionBefore = jumpPosition;
                        }
                        break;
                    case EventsQ29679::event_shu_set_orientation:
                        switch (jumpPosition)
                        {
                        case Misc::SHU_JUMP_POSITION_0:
                            me->SetFacingTo(1.32645f);
                            break;
                        case Misc::SHU_JUMP_POSITION_1:
                            me->SetFacingTo(5.654867f);
                            break;
                        case Misc::SHU_JUMP_POSITION_2:
                            me->SetFacingTo(2.338741f);
                            break;
                        case Misc::SHU_JUMP_POSITION_3:
                            me->SetFacingTo(4.34587f);
                            break;
                        }
                        events.ScheduleEvent(EventsQ29679::event_shu_summon_water_spout, 1500ms);
                        break;
                    case EventsQ29679::event_shu_summon_water_spout:
                    {
                        // Pick a random spawn slot within the current zone
                        uint8 randomSlot = urand(0, Misc::BUNNY_SPAWN_MAX_SLOTS - 1);

                        // Access the spawn position using zone and slot
                        Position targetPos = PositionsQ29679::ShuSpawnPositions[jumpPosition][randomSlot];

                        // Cast the summoning spell at the chosen location
                        //me->CastSpell(Position(targetPos), SPELL_SUMMON_WATER_SPOUT, true);
                        // Dynamic position spell casting summon not supported. we summon creature directly instead

                        Creature* bunny = me->SummonCreature(Npcs::npc_bunny_water_spout, targetPos, TEMPSUMMON_MANUAL_DESPAWN);
                        if (bunny)
                            DoCast(SpellsQ29678Q29679::spell_water_spout);
                        events.ScheduleEvent(EventsQ29679::event_cast_jump_spell, 6s);
                        break;
                    }
                    }
                }
            }

        private:
            EventMap events;
            uint32 jumpSpells[4] = { SpellsQ29678Q29679::spell_jump_front_right, SpellsQ29678Q29679::spell_jump_front_left, SpellsQ29678Q29679::spell_jump_back_right, SpellsQ29678Q29679::spell_jump_back_left };
            uint8 jumpPosition;
            //uint8 spawnPosition;
            uint8 positionBefore;
            bool startAI;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_shu_playingAI(creature);
        }
    };

    class npc_ox_cart : public CreatureScript
    {
    public:
        npc_ox_cart() : CreatureScript("npc_ox_cart") { }

        struct npc_ox_cartAI : public ScriptedAI
        {
            npc_ox_cartAI(Creature* creature) : ScriptedAI(creature), _passengerGuid() { }

            void Reset() override
            {
                _events.Reset();

                if (me->GetEntry() == Npcs::npc_vehicle_ox || me->GetEntry() == Npcs::npc_vehicle_ox_farmstead)
                {
                    me->SetReactState(REACT_PASSIVE);
                    //_events.ScheduleEvent(EventsOxCart::event_ox_cart_ropes, 500ms);
                    _events.ScheduleEvent(EventsOxCart::event_ox_cart_path_start, 1400ms); // Delay of 0.5 seconds
                }
            }

            void IsSummonedBy(WorldObject* /*summoner*/) override
            {
                me->ToTempSummon()->SetTempSummonType(TEMPSUMMON_MANUAL_DESPAWN);
            }

            void PassengerBoarded(Unit* passenger, int8 /*seat*/, bool apply) override
            {
                if (apply && passenger->GetTypeId() == TYPEID_PLAYER)
                {
                    if (me->GetEntry() == Npcs::npc_vehicle_cart || me->GetEntry() == Npcs::npc_vehicle_cart_farmstead)
                    {
                        _passengerGuid = passenger->GetGUID(); // Store for later use (e.g., for eject)
                        me->CastSpell(passenger, SpellsCartOx::spell_force_vehicle_ride);
                        _events.ScheduleEvent(EventsOxCart::event_ox_cart_path_start, 1800ms); // Delay
                    }
                }
            }

            void WaypointReached(uint32 nodeId, uint32 /*pathId*/) override
            {
                if(nodeId == PathOxCart::path_node_remove_passenger)
                    if (me->GetEntry() == Npcs::npc_vehicle_cart || me->GetEntry() == Npcs::npc_vehicle_cart_farmstead)
                        me->CastSpell(me, SpellsCartOx::spell_eject_passengers);
            }

            void WaypointPathEnded(uint32 /*nodeId*/, uint32 /*pathId*/) override
            {
                me->DespawnOrUnsummon(1s);
            }

            void UpdateAI(uint32 diff) override
            {
                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        /*
                    case EventsOxCart::event_ox_cart_ropes:
                    {
                        Unit* yak = nullptr;
                        if (me->GetEntry() == Npcs::npc_vehicle_cart || me->GetEntry() == Npcs::npc_vehicle_cart_farmstead)
                        {
                            if (Unit* yak1 = me->FindNearestCreature(Npcs::npc_vehicle_ox, 10.f))
                                yak = yak1;
                            else if (Unit* yak2 = me->FindNearestCreature(Npcs::npc_vehicle_ox_farmstead, 10.f))
                                yak = yak2;

                            if (yak)
                            {
                                me->CastSpell(yak, SpellsCartOx::spell_rope_left);
                                me->CastSpell(yak, SpellsCartOx::spell_rope_right);
                            }
                            
                        }
                        break;
                    }*/
                    case EventsOxCart::event_ox_cart_path_start:
                        if (me->GetEntry() == Npcs::npc_vehicle_cart)
                        {
                            me->LoadPath(PathOxCart::path_cart);
                            me->GetMotionMaster()->MovePath(PathOxCart::path_cart, false);
                        }
                        else if (me->GetEntry() == Npcs::npc_vehicle_ox)
                        {
                            me->LoadPath(PathOxCart::path_ox);
                            me->GetMotionMaster()->MovePath(PathOxCart::path_ox, false);
                        }
                        else if (me->GetEntry() == Npcs::npc_vehicle_cart_farmstead)
                        {
                            me->LoadPath(PathOxCart::path_cart_farmstead);
                            me->GetMotionMaster()->MovePath(PathOxCart::path_cart_farmstead, false);
                        }
                        else if (me->GetEntry() == Npcs::npc_vehicle_ox_farmstead)
                        {
                            me->LoadPath(PathOxCart::path_ox_farmstead);
                            me->GetMotionMaster()->MovePath(PathOxCart::path_ox_farmstead, false);
                        }
                        break;
                    default:
                        break;
                    }

                }
            }

        private:
            EventMap _events;
            ObjectGuid _passengerGuid;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_ox_cartAI(creature);
        }
    };

    class npc_shu_follower : public CreatureScript
    {
    public:
        npc_shu_follower() : CreatureScript("npc_shu_follower") { }

        struct npc_shu_followerAI : public ScriptedAI
        {
            npc_shu_followerAI(Creature* creature) : ScriptedAI(creature), _playerGuid(), _pathStarted(false) { }

            void Reset() override
            {
                _events.Reset();
                _pathStarted = false;

                me->SetWalk(false);
                me->CastSpell(me, SpellsQ29680::spell_shu_splash, true);

                Unit* owner = me->GetOwner();
                if (owner && owner->ToPlayer())
                    _playerGuid = owner->ToPlayer()->GetGUID();

                _events.ScheduleEvent(EventsQ29680::event_shu_follower_check_player_quest, 30s);

                if (owner && owner->ToPlayer()->IsActiveQuest(Quests::quest_the_source_of_livelihood))
                    _events.ScheduleEvent(EventsQ29680::event_shu_singing_pools_check_player_vehicle, 1s);
            }

            void UpdateAI(uint32 diff) override
            {
                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EventsQ29680::event_shu_follower_path_start:
                    {
                        me->GetMotionMaster()->Clear();
                        me->StopMoving();
                        me->LoadPath(PathQ29680::path_shu_follower);
                        me->GetMotionMaster()->MovePath(PathQ29680::path_shu_follower, false);
                        _pathStarted = true;
                        break;
                    }
                    case EventsQ29680::event_shu_follower_check_player_quest:
                    {
                        if (!_pathStarted)
                        {
                            Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid);

                            if (player && player->GetQuestStatus(Quests::quest_the_source_of_livelihood) == QUEST_STATUS_REWARDED)
                            {
                                _events.ScheduleEvent(EventsQ29680::event_shu_follower_path_start, 0s);
                            }
                            else
                                _events.ScheduleEvent(EventsQ29680::event_shu_follower_check_player_quest, 1s);
                        }
                        break;
                    }
                    case EventsQ29680::event_shu_singing_pools_check_player_vehicle:
                    {
                        if (me->GetVehicle())
                            break;

                        Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid);
                        if (!player)
                            break;

                        Vehicle* veh = player->GetVehicle();
                        if (!veh || !veh->IsVehicleInUse())
                        {
                            _events.ScheduleEvent(EventsQ29680::event_shu_singing_pools_check_player_vehicle, 500ms);
                            break;
                        }

                        me->EnterVehicle(veh->GetBase(), -1);
                        break;
                    }
                    default:
                        break;
                    }

                }
            }

            void WaypointReached(uint32 nodeId, uint32 /*pathId*/) override
            {
                Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid);
                if (!player)
                    return;

                if (nodeId == PathQ29680::path_node_shu_remove)
                {
                    me->SetWalk(true);
                    player->RemoveAurasDueToSpell(SpellsQ29678Q29679::spell_summon_spirit_of_water);
                    player->CastSpell(player, 104018, true);
                }
            }

            void WaypointPathEnded(uint32 /*nodeId*/, uint32 /*pathId*/) override
            {
                me->DespawnOrUnsummon(30s);
            }

        private:
            EventMap _events;
            ObjectGuid _playerGuid;
            bool _pathStarted;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_shu_followerAI(creature);
        }
    };

    // 55556
    class npc_shu_at_farmstead_pool : public CreatureScript
    {
    public:
        npc_shu_at_farmstead_pool() : CreatureScript("npc_shu_at_farmstead_pool") { }

        struct npc_shu_at_farmstead_poolAI : public ScriptedAI
        {
            npc_shu_at_farmstead_poolAI(Creature* creature) : ScriptedAI(creature), _playerGuid(), _npcFlagSet(false) {}

            void Reset() override
            {
                _events.Reset();
                _npcFlagSet = false;

                me->RemoveNpcFlag(UNIT_NPC_FLAG_GOSSIP);

                _events.ScheduleEvent(EventsQ29774::event_shu_farmstead_play, 0s);
            }

            bool OnGossipSelect(Player* player, uint32 menuId, uint32 gossipListId) override
            {
                _playerGuid = player->GetGUID();

                if (menuId == Misc::shu_farmstead_gossip_menu && gossipListId == 0)
                {
                    CloseGossipMenuFor(player);
                    //player->KilledMonsterCredit(Npcs::credit_not_in_the_face_1);
                    //player->CastSpell(player, 105891);
                    if (Creature* wugou = GetClosestCreatureWithEntry(me, Npcs::npc_wugou_farmstead, 70.0f))
                        wugou->DespawnOrUnsummon();
                    me->DespawnOrUnsummon();
                    player->CastSpell(player, SpellsQ29774::spell_summon_spirits_water_earth);
                    return true;
                }
                return false;
            }

            uint32 CheckQuestTimer = 1000; // 1 second

            void UpdateAI(uint32 diff) override
            {
                if (CheckQuestTimer <= diff)
                {
                    if (Player* player = me->SelectNearestPlayer(50.0f))
                    {
                        if (!_npcFlagSet && player->GetQuestStatus(Quests::quest_not_in_the_face) == QUEST_STATUS_INCOMPLETE)
                        {
                            if (!me->HasNpcFlag(UNIT_NPC_FLAG_GOSSIP))
                            {
                                me->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
                                _npcFlagSet = true;
                            }
                        }
                    }

                    CheckQuestTimer = 1000; // reset
                }
                else
                    CheckQuestTimer -= diff;


                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EventsQ29774::event_shu_farmstead_play:
                    {
                        // Pick a random spawn slot within the current zone
                        uint8 randomSlot = urand(0, 4);

                        // Cast the summoning spell at the chosen location
                        //me->CastSpell(Position(targetPos), SPELL_SUMMON_WATER_SPOUT, true);
                        // Dynamic position spell casting summon not supported. we summon creature directly instead

                        Creature* bunny = me->SummonCreature(Npcs::npc_bunny_water_spout_farmstead, PositionsQ29774::ShuFarmsteadPlayPosition[randomSlot], TEMPSUMMON_MANUAL_DESPAWN);
                        if (bunny)
                            DoCast(SpellsQ29678Q29679::spell_water_spout);
                        _events.ScheduleEvent(EventsQ29774::event_shu_farmstead_play, 6s);
                        break;
                    }
                    default:
                        break;
                    }

                }
            }
        private:
            EventMap _events;
            ObjectGuid _playerGuid;
            bool _npcFlagSet;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_shu_at_farmstead_poolAI(creature);
        }
    };

    // 55558
    class npc_shu_at_farmstead_play : public CreatureScript
    {
    public:
        npc_shu_at_farmstead_play() : CreatureScript("npc_shu_at_farmstead_play") { }

        struct npc_shu_at_farmsteadAI : public ScriptedAI
        {
            npc_shu_at_farmsteadAI(Creature* creature) : ScriptedAI(creature), _playerGuid(), _path1Started(false), _path2Started(false) {}

            void Reset() override
            {
                _path1Started = false;
                _path2Started = false;
                _events.Reset();
                _playerGuid = me->GetOwnerGUID();
                Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid);
                if (player && player->IsActiveQuest(Quests::quest_not_in_the_face))
                    _events.ScheduleEvent(EventsQ29774::event_shu_farmstead_path_start_1, 0s);
                else
                {
                    _events.ScheduleEvent(EventsQ29774::event_try_remove_wugou_sleep, 200ms);
                    _events.ScheduleEvent(EventsQ29774::event_shu_farmstead_check_player_vehicle, 1s);
                }
            }

            void UpdateAI(uint32 diff) override
            {
                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EventsQ29774::event_shu_farmstead_path_start_1:
                    {
                        if (!_path1Started)
                        {
                            me->GetMotionMaster()->Clear();
                            me->StopMoving();
                            me->LoadPath(PathQ29774::path_shu_farmstead_1);
                            me->GetMotionMaster()->MovePath(PathQ29774::path_shu_farmstead_1, false);
                            _path1Started = true;
                        }
                        break;
                    }
                    case EventsQ29774::event_shu_farmstead_path_start_2:
                    {
                        if (!_path2Started)
                        {
                            me->StopMoving();
                            me->GetMotionMaster()->Clear();
                            me->LoadPath(PathQ29774::path_shu_farmstead_2);
                            me->GetMotionMaster()->MovePath(PathQ29774::path_shu_farmstead_2, false);
                            _path2Started = true;
                        }
                        break;
                    }
                    case EventsQ29774::event_shu_wakes_wugou:
                    {
                        Creature* wugou = GetClosestCreatureWithEntry(me, Npcs::npc_wugou_q29774, 30.0f);
                        if (wugou)
                        {
                            me->CastSpell(wugou, SpellsQ29774::spell_shu_watersplash_wugou);
                            wugou->CastSpell(me, SpellsQ29774::spell_shu_watersplash);
                            wugou->RemoveAllAuras();
                            me->RemoveAllAuras();
                            wugou->SetStandState(UNIT_STAND_STATE_STAND);
                            me->CastSpell(me, SpellsQ29774::spell_water_spirit_laugh);

                            Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid);
                            if (!player)
                                return;

                            me->CastSpell(player, SpellsQ29774::spell_shu_watersplash_credit);
                            //player->KilledMonsterCredit(Npcs::credit_not_in_the_face_2);
                            wugou->GetMotionMaster()->MoveFollow(me, 5.0f, 5.0f);
                            wugou->DespawnOrUnsummon(30s);
                            _events.ScheduleEvent(EventsQ29774::event_shu_farmstead_path_start_2, 5s);
                        }
                        break;
                    }
                    case EventsQ29774::event_shu_farmstead_check_player_vehicle:
                    {
                        if (me->GetVehicle())
                            break;

                        Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid);
                        if (!player)
                            break;

                        Vehicle* veh = player->GetVehicle();
                        if (!veh || !veh->IsVehicleInUse())
                        {
                            _events.ScheduleEvent(EventsQ29774::event_shu_farmstead_check_player_vehicle, 500ms);
                            break;
                        }

                        me->EnterVehicle(veh->GetBase(), -1);
                        break;
                    }
                    case EventsQ29774::event_try_remove_wugou_sleep:
                    {
                        if (Creature* wugou = GetClosestCreatureWithEntry(me, Npcs::npc_wugou_q29774, 20.0f))
                        {
                            if (wugou->GetOwnerGUID() == _playerGuid)
                            {
                                wugou->RemoveAllAuras();
                                break;
                            }
                        }

                        // If not found yet, try again shortly
                        _events.ScheduleEvent(EventsQ29774::event_try_remove_wugou_sleep, 200ms);
                        break;
                    }
                    default:
                        break;
                    }

                }
            }

            void WaypointPathEnded(uint32 /*nodeId*/, uint32 pathId) override
            {
                switch (pathId)
                {
                case PathQ29774::path_shu_farmstead_1:
                {
                    me->StopMoving();
                    me->GetMotionMaster()->Clear();

                    Creature* wugou = GetClosestCreatureWithEntry(me, Npcs::npc_wugou_q29774, 30.0f);
                    if (wugou)
                    {
                        
                        me->CastSpell(me, SpellsQ29774::spell_water_spirit_laugh);
                        _events.ScheduleEvent(EventsQ29774::event_shu_wakes_wugou, 5s);
                    }
                    break;
                }
                case PathQ29774::path_shu_farmstead_2:
                {
                    me->StopMoving();
                    me->GetMotionMaster()->Clear();
                    me->CastSpell(me, SpellsQ29774::spell_water_spirit_laugh);
                    me->GetMotionMaster()->MovePoint(1, me->GetRandomNearPosition(30.0f));
                    me->DespawnOrUnsummon(15s);
                    break;
                }
                default:
                    break;
                }
            }

        private:
            EventMap _events;
            ObjectGuid _playerGuid;
            bool _path1Started;
            bool _path2Started;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_shu_at_farmsteadAI(creature);
        }
    };

    class npc_shanxi_quest2 : public CreatureScript
    {
    public:
        npc_shanxi_quest2() : CreatureScript("npc_shanxi_quest2") { }

        struct npc_shanxi_questAI : public ScriptedAI
        {
            npc_shanxi_questAI(Creature* creature) : ScriptedAI(creature) {}

            void OnQuestReward(Player* /*player*/, Quest const* quest, LootItemType /*type*/, uint32 /*opt*/) override
            {
                if (quest->GetQuestId() == Quests::quest_the_spirit_and_body_of_shenzinsu)
                {
                    Talk(TalksQ29775::shanxi_talk_7);

                    _scheduler.Schedule(3s, [this](TaskContext /*task*/)
                        {
                            Talk(TalksQ29775::shanxi_talk_8);
                        });
                    _scheduler.Schedule(6s, [this](TaskContext /*task*/)
                        {
                            Talk(TalksQ29775::shanxi_talk_9);
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

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_shanxi_questAI(creature);
        }
    };

    class npc_lorewalker_ruolin : public CreatureScript
    {
    public:
        npc_lorewalker_ruolin() : CreatureScript("npc_lorewalker_ruolin") { }

        struct npc_lorewalker_ruolinAI : public ScriptedAI
        {
            npc_lorewalker_ruolinAI(Creature* creature) : ScriptedAI(creature), dialogueIndex(0) { }

            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EventsLorewalker::event_start_dialogue:
                    {
                        // safety: ensure index in range
                        if (dialogueIndex >= dialogueSequence.size())
                        {
                            StoryInProgress = false;
                            break;
                        }

                        const LorewalkerDialogueEntry& entry = dialogueSequence[dialogueIndex];

                        if (Creature* speaker = GetClosestCreatureWithEntry(me, entry.npcEntry, 20.0f))
                            speaker->AI()->Talk(entry.talkId);

                        // prepare for next step
                        ++dialogueIndex;

                        if (entry.isFinal)
                        {
                            StoryInProgress = false;
                            // optionally reset events or schedule a cooldown
                            events.Reset();
                        }
                        else
                        {
                            // schedule next dialogue step using the entry delay
                            events.ScheduleEvent(EventsLorewalker::event_start_dialogue, entry.delay);
                        }
                        break;
                    }
                    default:
                        break;
                    }
                }
            }

            void StartLorewalkerStory()
            {
                if (StoryInProgress)
                    return;

                events.Reset();
                dialogueIndex = 0;
                StoryInProgress = true;
                me->CastSpell(me, SpellsMisc::spell_ruolin_singing);
                me->HandleEmoteCommand(EMOTE_ONESHOT_YES);
                events.ScheduleEvent(EventsLorewalker::event_start_dialogue, 30s);
            }

        private:
            EventMap events;
            size_t dialogueIndex;
            bool StoryInProgress = false;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_lorewalker_ruolinAI(creature);
        }
    };

    const std::vector<LorewalkerDialogueEntry> dialogueSequence =
    {
        { Npcs::npc_lorewalker_amai, 39, 4s },
        { Npcs::npc_lorewalker_amai, 0, 4s },
        { Npcs::npc_lorewalker_hao, 0, 3s },
        { Npcs::npc_lorewalker_amai, 1, 4s },
        { Npcs::npc_lorewalker_nan, 0, 3s },
        { Npcs::npc_lorewalker_amai, 2, 4s },
        { Npcs::npc_lorewalker_amai, 3, 4s },
        { Npcs::npc_lorewalker_amai, 4, 4s },
        { Npcs::npc_lorewalker_yin, 0, 3s },
        { Npcs::npc_lorewalker_amai, 5, 4s },
        { Npcs::npc_lorewalker_amai, 6, 4s },
        { Npcs::npc_lorewalker_amai, 7, 4s },
        { Npcs::npc_lorewalker_hao, 1, 3s },
        { Npcs::npc_lorewalker_amai, 8, 4s },
        { Npcs::npc_lorewalker_nan, 1, 3s },
        { Npcs::npc_lorewalker_hao, 2, 3s },
        { Npcs::npc_lorewalker_amai, 9, 4s },
        { Npcs::npc_lorewalker_hao, 3, 3s },
        { Npcs::npc_lorewalker_amai, 10, 4s },
        { Npcs::npc_lorewalker_amai, 11, 4s },
        { Npcs::npc_lorewalker_nan, 2, 3s },
        { Npcs::npc_lorewalker_hao, 4, 3s },
        { Npcs::npc_lorewalker_amai, 12, 4s },
        { Npcs::npc_lorewalker_amai, 13, 4s },
        { Npcs::npc_lorewalker_hao, 5, 3s },
        { Npcs::npc_lorewalker_nan, 3, 3s },
        { Npcs::npc_lorewalker_amai, 14, 4s },
        { Npcs::npc_lorewalker_amai, 15, 4s },
        { Npcs::npc_lorewalker_yin, 1, 3s },
        { Npcs::npc_lorewalker_amai, 16, 4s },
        { Npcs::npc_lorewalker_amai, 17, 4s },
        { Npcs::npc_lorewalker_yin, 2, 3s },
        { Npcs::npc_lorewalker_amai, 18, 4s },
        { Npcs::npc_lorewalker_amai, 19, 4s },
        { Npcs::npc_lorewalker_nan, 4, 3s },
        { Npcs::npc_lorewalker_amai, 20, 4s },
        { Npcs::npc_lorewalker_hao, 6, 3s },
        { Npcs::npc_lorewalker_amai, 21, 4s },
        { Npcs::npc_lorewalker_yin, 3, 3s },
        { Npcs::npc_lorewalker_amai, 22, 4s },
        { Npcs::npc_lorewalker_nan, 5, 3s },
        { Npcs::npc_lorewalker_amai, 23, 4s },
        { Npcs::npc_lorewalker_hao, 7, 3s },
        { Npcs::npc_lorewalker_nan, 6, 3s },
        { Npcs::npc_lorewalker_hao, 8, 3s },
        { Npcs::npc_lorewalker_yin, 4, 3s },
        { Npcs::npc_lorewalker_amai, 24, 4s },
        { Npcs::npc_lorewalker_amai, 25, 4s },
        { Npcs::npc_lorewalker_amai, 26, 4s },
        { Npcs::npc_lorewalker_amai, 27, 4s },
        { Npcs::npc_lorewalker_amai, 28, 4s },
        { Npcs::npc_lorewalker_amai, 29, 4s },
        { Npcs::npc_lorewalker_amai, 30, 4s },
        { Npcs::npc_lorewalker_amai, 31, 4s },
        { Npcs::npc_lorewalker_hao, 9, 3s },
        { Npcs::npc_lorewalker_nan, 7, 3s },
        { Npcs::npc_lorewalker_amai, 32, 4s },
        { Npcs::npc_lorewalker_hao, 10, 3s },
        { Npcs::npc_lorewalker_amai, 33, 4s },
        { Npcs::npc_lorewalker_amai, 34, 4s },
        { Npcs::npc_lorewalker_yin, 5, 3s },
        { Npcs::npc_lorewalker_amai, 35, 4s },
        { Npcs::npc_lorewalker_amai, 36, 4s },
        { Npcs::npc_lorewalker_amai, 37, 4s },
        { Npcs::npc_lorewalker_amai, 38, 5s, true }
    };

    // Area Trigger 8287
    class at_lorewalker_zan : public AreaTriggerScript
    {
    public:
        at_lorewalker_zan() : AreaTriggerScript("at_lorewalker_zan") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/) override
        {
            if (player->IsAlive())
            {
                Creature* zan = GetClosestCreatureWithEntry(player, Npcs::npc_lorewalker_zan, 30.0f);
                if (zan)
                {
                    zan->AI()->Talk(TalksLorewalker::lorewalker_zan_0, player);

                    Creature* ruolin = zan->FindNearestCreatureWithOptions(100.f, { .CreatureId = Npcs::npc_lorewalker_ruolin, .IgnorePhases = true });
                    if (ruolin)
                    {
                        auto* ai = CAST_AI(npc_lorewalker_ruolin::npc_lorewalker_ruolinAI, ruolin->AI());
                        if (ai)
                            ai->StartLorewalkerStory();

                    }
                }

                return true;
            }
            return false;
        }
    };

    class npc_ruk_ruk : public CreatureScript
    {
    public:
        npc_ruk_ruk() : CreatureScript("npc_ruk_ruk") { }

        struct npc_ruk_rukAI : public ScriptedAI
        {
            npc_ruk_rukAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() override
            {
                events.Reset();
            }

            void JustEngagedWith(Unit* /*who*/) override
            {
                events.ScheduleEvent(EventsRukRuk::event_rukruk_cast_aim, 10s);
                events.ScheduleEvent(EventsRukRuk::event_rukruk_cast_ooksplosions, 30s);
            }

            Position GetRocketTargetPos() const
            {
                return pos;
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EventsRukRuk::event_rukruk_cast_aim:
                        if (me->HasUnitState(UNIT_STATE_CASTING))
                        {
                            events.RescheduleEvent(EventsRukRuk::event_rukruk_cast_aim, 1s);
                            break;
                        }
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random))
                        {
                            me->SetFacingToObject(target);
                            CalculateSpellVisual(target);
                            DoCast(target, SpellsRukRuk::spell_aim);
                            events.ScheduleEvent(EventsRukRuk::event_rukruk_cast_aim, 15s, 25s);
                        }
                        break;
                    case EventsRukRuk::event_rukruk_cast_ooksplosions:
                        if (me->HasUnitState(UNIT_STATE_CASTING))
                        {
                            events.RescheduleEvent(EventsRukRuk::event_rukruk_cast_ooksplosions, 1s);
                            break;
                        }
                        DoCast(SpellsRukRuk::spell_ookslosions);
                        events.ScheduleEvent(EventsRukRuk::event_rukruk_cast_ooksplosions, 25s, 35s);
                        break;
                    }
                }
                me->DoMeleeAttackIfReady();
            }

        private:
            EventMap events;
            Position pos;

            void CalculateSpellVisual(Unit* target)
            {
                float ori = me->GetOrientation();
                float z = me->GetPositionZ();
                float targetDist = target->GetExactDist(me->GetPosition());

                for (int radius = 1; ; radius++)
                {
                    if (radius <= ceilf(targetDist))
                    {
                        float x = me->GetPositionX() + radius * cos(ori);
                        float y = me->GetPositionY() + radius * sin(ori);
                        me->UpdateGroundPositionZ(x, y, z);
                        pos = { x, y, z };
                        me->SendPlaySpellVisual(pos, SpellsRukRuk::spell_aim_visual, 0, 0, 0.0f, false, 2.0f);
                    }
                    else
                        break;
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_ruk_rukAI(creature);
        }
    };

    class npc_ruk_ruk_rocket : public CreatureScript
    {
    public:
        npc_ruk_ruk_rocket() : CreatureScript("npc_ruk_ruk_rocket") { }

        struct npc_ruk_ruk_rocketAI : public ScriptedAI
        {
            npc_ruk_ruk_rocketAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() override
            {
                me->SetDisplayFromModel(0);
                events.ScheduleEvent(EventsRukRuk::event_rukruk_rocket_fire, 500ms);
            }

            void IsSummonedBy(WorldObject* summonerWO) override
            {
                if (Creature* summoner = summonerWO->ToCreature())
                {
                    if (auto* rukRukAI = CAST_AI(npc_ruk_ruk::npc_ruk_rukAI, summoner->AI()))
                    {
                        rocketTargetPos = rukRukAI->GetRocketTargetPos();

                        if (me->GetExactDist2d(rocketTargetPos) > 30.0f)
                            RecalculateTargetPos();
                    }
                }
            }

            void MovementInform(uint32 /*type*/, uint32 id) override
            {
                if (id == 1)
                {
                    DoCast(SpellsRukRuk::spell_rocket_explosion_visual);
                    DoCast(SpellsRukRuk::spell_rocket_explosion_damage);
                    me->DespawnOrUnsummon(1s);
                }
            }

            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EventsRukRuk::event_rukruk_rocket_fire:
                        me->GetMotionMaster()->MovePoint(1, rocketTargetPos);
                        break;
                    }
                }
            }

            void RecalculateTargetPos()
            {
                float ori = me->GetOrientation();
                float x = me->GetPositionX() + 30 * cos(ori);
                float y = me->GetPositionY() + 30 * sin(ori);
                float z = me->GetPositionZ();
                me->UpdateGroundPositionZ(x, y, z);
                rocketTargetPos = { x, y, z };
            }

        private:
            EventMap events;
            Position rocketTargetPos;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_ruk_ruk_rocketAI(creature);
        }
    };

    // 55744 Aysa outside chambers of whispers
    class npc_aysa_outside_chambers_of_whispers : public CreatureScript
    {
    public:
        npc_aysa_outside_chambers_of_whispers() : CreatureScript("npc_aysa_outside_chambers_of_whispers") { }

        struct npc_aysaAI : public ScriptedAI
        {
            npc_aysaAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() override
            {
                events.Reset();
                Unit* owner = me->GetOwner();
                if (owner && owner->ToPlayer())
                    PlayerGUID = owner->ToPlayer()->GetGUID();

                events.ScheduleEvent(EventsZhaorenEvent::event_aysa_outside_chambers_init, 1s);
            }

            void MovementInform(uint32 type, uint32 id) override
            {
                if (type != POINT_MOTION_TYPE)
                    return;

                if (id == 1) // reached player
                {
                    Talk(TalksZhaorenEvent::aysa_chamber_of_whispers_1);
                    events.ScheduleEvent(EventsZhaorenEvent::event_aysa_outside_chambers_move1, 3s);
                }

                if (id == 2) // reached outside entrance
                {
                    events.ScheduleEvent(EventsZhaorenEvent::event_aysa_outside_chambers_move2, 25s);
                }

                if (id == 3) // reached half way chamber
                {
                    Talk(TalksZhaorenEvent::aysa_chamber_of_whispers_2);
                    events.ScheduleEvent(EventsZhaorenEvent::event_aysa_inside_chambers_move_dafeng, 10s);
                }
            }

            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EventsZhaorenEvent::event_aysa_outside_chambers_init:
                    {
                        Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID);
                        if (!player)
                        {
                            player = me->SelectNearestPlayer(10.f);
                            if (!player)
                                return;
                        }
                        Talk(TalksZhaorenEvent::aysa_chamber_of_whispers_0);
                        me->GetMotionMaster()->MovePoint(1, player->GetRandomNearPosition(3.f));
                        break;
                    }
                        
                    case EventsZhaorenEvent::event_aysa_outside_chambers_move1:
                    {
                        me->GetMotionMaster()->MovePoint(2, PositionsZhaorenEvent::AysaOutsideChamber);
                        break;
                    }

                    case EventsZhaorenEvent::event_aysa_outside_chambers_move2:
                    {
                        std::list<Creature*> list;
                        GetCreatureListWithEntryInGrid(list, me, Npcs::npc_chamber_winds, 100.0f);

                        for (Creature* creature : list)
                        {
                            if (creature)
                            {
                                creature->SetRespawnTime(5);
                                creature->DespawnOrUnsummon();
                            }
                        }
                        me->GetMotionMaster()->MovePoint(3, PositionsZhaorenEvent::AysaInsideChamber);
                        break;
                    }

                    case EventsZhaorenEvent::event_aysa_inside_chambers_move_dafeng:
                    {
                        Creature* aysaDafeng = me->FindNearestCreatureWithOptions(100.f, { .CreatureId = Npcs::npc_aysa_q29785, .IgnorePhases = true });
                        if (aysaDafeng)
                        {
                            std::list<Creature*> list;
                            GetCreatureListWithEntryInGrid(list, me, Npcs::npc_chamber_winds, 100.0f);

                            for (Creature* creature : list)
                            {
                                if (creature)
                                {
                                    creature->SetRespawnTime(5);
                                    creature->DespawnOrUnsummon();
                                }
                            }

                            me->GetMotionMaster()->MovePoint(4, aysaDafeng->GetPosition());
                            me->DespawnOrUnsummon(25s);
                        }
                    }
                    }
                }
            }

        private:
            EventMap events;
            ObjectGuid PlayerGUID;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_aysaAI(creature);
        }
    };

    //55786
    class npc_zhaoren : public CreatureScript
    {
    public:
        npc_zhaoren() : CreatureScript("npc_zhaoren") { }

        struct npc_zhaorenAI : public ScriptedAI
        {
            npc_zhaorenAI(Creature* creature) : ScriptedAI(creature), phase(0), sweepScheduled(false) { }

        private:
            uint8 phase;
            bool sweepScheduled;
            EventMap events;
            ObjectGuid JiGUID;
            ObjectGuid AysaGUID;

            void Reset() override
            {
                events.Reset();
                me->setActive(true);
                me->SetReactState(REACT_AGGRESSIVE);
                phase = 0;
                sweepScheduled = false;
            }

            void JustEngagedWith(Unit* /*who*/) override
            {
                if (Creature* ji = me->FindNearestCreature(Npcs::npc_ji_q29786, me->GetVisibilityRange(), true))
                {
                    ji->SetReactState(REACT_AGGRESSIVE);
                    ji->AI()->AttackStart(me);
                    JiGUID = ji->GetGUID();
                }

                if (Creature* aysa = me->FindNearestCreature(Npcs::npc_aysa_q29786, me->GetVisibilityRange(), true))
                {
                    aysa->SetReactState(REACT_AGGRESSIVE);
                    aysa->AI()->AttackStart(me);
                    AysaGUID = aysa->GetGUID();
                }

                me->GetMotionMaster()->Clear();
                me->GetMotionMaster()->MovePath(PathZhaorenEvent::path_zhaoren_at_chamber, true);
                events.SetPhase(Misc::ZHAO_PHASE_FLYING);
                events.ScheduleEvent(EventsZhaorenEvent::event_zhao_cast_lightning, 5s);
            }

            void SpellHit(WorldObject* caster, SpellInfo const* spell) override
            {
                if (spell->Id == SpellsZhaorenEvent::spell_overpacked_firework)
                    if (!me->IsInCombat())
                        if (Unit* target = caster->ToUnit())
                            me->Attack(target, true);
            }

            void MovementInform(uint32 type, uint32 id) override
            {
                if (type == POINT_MOTION_TYPE && id == 1)
                    events.ScheduleEvent(EventsZhaorenEvent::event_zhao_state_stun, 0s);
            }

            void KilledUnit(Unit* who) override
            {
                if (who->IsPlayer())
                {
                    if (me->GetThreatManager().IsThreatListEmpty(true))
                    {
                        if (Creature* ji = ObjectAccessor::GetCreature(*me, JiGUID))
                            ji->DespawnOrUnsummon(5s);

                        if (Creature* aysa = ObjectAccessor::GetCreature(*me, AysaGUID))
                            aysa->DespawnOrUnsummon(5s);
                        me->DespawnOrUnsummon(10s);

                    }
                }
            }

            void JustDied(Unit* /*killer*/) override
            {
                if (Creature* dafeng = me->FindNearestCreature(Npcs::npc_dafeng_q29786, me->GetVisibilityRange(), true))
                    dafeng->AI()->SetData(Misc::DATA_ZHAOREN_DEATH, Misc::DATA_ZHAOREN_DEATH);

                me->DespawnOrUnsummon(4s);
            }

            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);
                if (phase == 0 && HealthBelowPct(85))
                {
                    phase++;
                    if (Creature* aysha = ObjectAccessor::GetCreature(*me, AysaGUID))
                        aysha->AI()->SetData(Misc::EVENT_DATA_1, Misc::EVENT_DATA_1);
                }
                if (phase == Misc::ZHAO_PHASE_FLYING && HealthBelowPct(75))
                {
                    phase++;
                    events.SetPhase(Misc::ZHAO_PHASE_GROUNDED);
                    events.CancelEvent(EventsZhaorenEvent::event_zhao_cast_lightning);
                    events.ScheduleEvent(EventsZhaorenEvent::event_zhao_move_center, 0s);
                }
                if (phase == Misc::ZHAO_PHASE_GROUNDED && HealthBelowPct(25))
                {
                    phase++;
                    events.SetPhase(Misc::ZHAO_PHASE_STAY_IN_CENTER);
                    events.CancelEvent(EventsZhaorenEvent::event_zhao_cast_lightning);
                    events.ScheduleEvent(EventsZhaorenEvent::event_zhao_move_center, 0s);
                }
                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EventsZhaorenEvent::event_zhao_cast_lightning:
                    {
                        auto const& threatList = me->GetThreatManager().GetUnsortedThreatList();
                        if (threatList.begin() != threatList.end())
                        {
                            for (ThreatReference const* ref : threatList)
                            {
                                if (Unit* target = ref->GetVictim())
                                {
                                    if (target->IsPlayer())
                                        DoCast(target, SpellsZhaorenEvent::spell_lightning_pool);
                                }
                            }

                            events.ScheduleEvent(EventsZhaorenEvent::event_zhao_cast_lightning, events.IsInPhase(Misc::ZHAO_PHASE_FLYING) ? 5s : 3500ms);

                            if (!sweepScheduled && events.IsInPhase(Misc::ZHAO_PHASE_STAY_IN_CENTER))
                            {
                                events.ScheduleEvent(EventsZhaorenEvent::event_zhao_cast_sweep, 15s, 0, Misc::ZHAO_PHASE_STAY_IN_CENTER);
                                sweepScheduled = true;
                            }
                        }
                        else
                        {
                            if (Creature* ji = ObjectAccessor::GetCreature(*me, JiGUID))
                                ji->DespawnOrUnsummon(5s);
                            if (Creature* aysa = ObjectAccessor::GetCreature(*me, AysaGUID))
                                aysa->DespawnOrUnsummon(5s);
                            me->DespawnOrUnsummon(10s);
                        }
                        break;
                    }
                    case EventsZhaorenEvent::event_zhao_move_center:
                        me->GetMotionMaster()->MovePoint(1, PositionsZhaorenEvent::ZhaoPos);
                        break;

                    case EventsZhaorenEvent::event_zhao_state_stun:
                        DoCast(SpellsZhaorenEvent::spell_stunned_by_fireworks);
                        events.ScheduleEvent(EventsZhaorenEvent::event_zhao_cast_sweep, 12s);
                        if (Creature* aysa = ObjectAccessor::GetCreature(*me, AysaGUID))
                        {
                            if (phase == Misc::ZHAO_PHASE_GROUNDED)
                                aysa->AI()->SetData(Misc::DATA_COMBAT, Misc::DATA_COMBAT);
                            else if (phase == Misc::ZHAO_PHASE_STAY_IN_CENTER)
                                aysa->AI()->SetData(Misc::DATA_AYSA_TALK_3, Misc::DATA_AYSA_TALK_3);
                        }
                        if (Creature* ji = ObjectAccessor::GetCreature(*me, JiGUID))
                            ji->AI()->SetData(Misc::DATA_COMBAT, Misc::DATA_COMBAT);
                        break;

                    case EventsZhaorenEvent::event_zhao_cast_sweep:
                        events.CancelEvent(EventsZhaorenEvent::event_zhao_cast_lightning);
                        DoCast(SpellsZhaorenEvent::spell_serpent_sweep);
                        sweepScheduled = false;
                        events.ScheduleEvent(EventsZhaorenEvent::event_zhao_cast_lightning, 3500ms, 0, Misc::ZHAO_PHASE_STAY_IN_CENTER);
                        events.ScheduleEvent(EventsZhaorenEvent::event_zhao_resume_path, 5s, 0, Misc::ZHAO_PHASE_GROUNDED);
                        if (events.IsInPhase(Misc::ZHAO_PHASE_GROUNDED))
                            if (Creature* ji = ObjectAccessor::GetCreature(*me, JiGUID))
                                ji->AI()->SetData(Misc::DATA_PHASE_OOC, Misc::DATA_PHASE_OOC);
                        break;
                    case EventsZhaorenEvent::event_zhao_resume_path:
                        me->GetMotionMaster()->MovePath(PathZhaorenEvent::path_zhaoren_at_chamber, true);
                        events.SetPhase(Misc::ZHAO_PHASE_FLYING);
                        events.ScheduleEvent(EventsZhaorenEvent::event_zhao_cast_lightning, 5s);
                        break;
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_zhaorenAI(creature);
        }
    };

    // 64507
    class npc_firework_launcher : public CreatureScript
    {
    public:
        npc_firework_launcher() : CreatureScript("npc_firework_launcher") { }

        struct npc_firework_launcherAI : public ScriptedAI
        {
            npc_firework_launcherAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() override
            {
                state = Misc::LAUNCHER_STATE_READY;
                me->RemoveAurasDueToSpell(SpellsZhaorenEvent::spell_firework_inactive);
                events.Reset();
            }

            // Player clicks launcher ? cast 125961 on launcher
            void OnSpellClick(Unit* clicker, bool) override
            {
                clicker->CastSpell(me, SpellsZhaorenEvent::spell_overpacked_firework_launcher_ping, true);
            }

            // Launcher is hit by 125961
            void SpellHit(WorldObject* caster, SpellInfo const* spellInfo) override
            {
                if (spellInfo->Id == SpellsZhaorenEvent::spell_overpacked_firework_launcher_ping &&
                    state == Misc::LAUNCHER_STATE_READY)
                {
                    Unit* player = caster->ToUnit();
                    if (!player)
                        return;

                    // Cast 125970 on player (matches SAI)
                    me->CastSpell(player, SpellsZhaorenEvent::spell_aicast_overpacked_fireworkd, true);

                    // Play animation
                    me->PlayOneShotAnimKitId(2538);

                    // Apply inactive aura
                    me->CastSpell(me, SpellsZhaorenEvent::spell_firework_inactive, true);

                    // Ping Ji Firepaw
                    if (Creature* ji = me->FindNearestCreature(Npcs::npc_ji_q29786, 30.f))
                        me->CastSpell(ji, 104080, true);

                    // Enter USED state
                    state = Misc::LAUNCHER_STATE_USED;

                    // Periodic Ji pings every 5s
                    events.ScheduleEvent(EventsZhaorenEvent::event_firework_launcher_periodic_ping, 5s);

                    // Recharge after 21s
                    events.ScheduleEvent(EventsZhaorenEvent::event_firework_launcher_recharge, 21s);
                }
            }

            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EventsZhaorenEvent::event_firework_launcher_periodic_ping:
                    {
                        if (state == Misc::LAUNCHER_STATE_USED)
                        {
                            if (Creature* ji = me->FindNearestCreature(Npcs::npc_ji_q29786, 30.f))
                                me->CastSpell(ji, 104080, true);

                            events.ScheduleEvent(eventId, 5s);
                        }
                        break;
                    }

                    case EventsZhaorenEvent::event_firework_launcher_recharge:
                    {
                        state = Misc::LAUNCHER_STATE_READY;
                        me->RemoveAurasDueToSpell(SpellsZhaorenEvent::spell_firework_inactive);
                        break;
                    }
                    }
                }
            }

            bool CanBeUsed() const
            {
                return state == Misc::LAUNCHER_STATE_READY;
            }

        private:
            uint8 state = Misc::LAUNCHER_STATE_NONE;
            EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_firework_launcherAI(creature);
        }
    };

    // 56159
    class npc_master_shang_q29787 : public CreatureScript
    {
    public:
        npc_master_shang_q29787() : CreatureScript("npc_master_shang_q29787") { }

        struct npc_master_shangAI : public ScriptedAI
        {
            npc_master_shangAI(Creature* c) : ScriptedAI(c) { }

            void Reset() override
            {
                events.Reset();

                PlayerGUID = me->SelectNearestPlayer(5.f)->GetGUID();

                Creature* shang_q29786 = me->FindNearestCreature(Npcs::npc_shang_q29786, 5.f);
                if (shang_q29786)
                    shang_q29786->DespawnOrUnsummon();

                events.ScheduleEvent(EventsQ29787::event_shang_talk_0, 2s);
                events.ScheduleEvent(EventsQ29787::event_shang_path_0, 3s);
            }

            void MovementInform(uint32 /*type*/, uint32 pointId) override
            {
                switch (pointId)
                {
                case 7: me->SetWalk(true); Talk(TalksQ29787::shang_talk_1); break;
                case 8: Talk(TalksQ29787::shang_talk_2); break;
                case 9: Talk(TalksQ29787::shang_talk_3); break;
                case 12: me->SetFacingTo(5.48033f); break;
                }
            }

            void WaypointPathEnded(uint32 /*nodeId*/, uint32 pathId) override
            {
                if (pathId == PathQ29787::path_zhang_1)
                {
                    me->SetFacingTo(4.53786f);
                    Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID);
                    if (player)
                    {
                        player->RemoveAurasDueToSpell(SpellsQ29787::spell_summon_shang_q29787);
                        PhasingHandler::AddPhase(player, 1527, true);
                    }


                    me->DespawnOrUnsummon();
                }
            }

            void UpdateAI(uint32 diff) override
            {
                if (!guardianDead)
                {
                    Creature* guardian = me->FindNearestCreature(Npcs::npc_guardian_q29787, 20.f, false);
                    if (guardian && !guardian->IsAlive())
                    {
                        guardianDead = true;
                        GameObject* wall = me->FindNearestGameObject(Objects::go_spirit_wall, 50.f, true);
                        if (wall)
                        {
                            wall->SetRespawnTime(60);
                            wall->DespawnOrUnsummon();
                        }

                        events.ScheduleEvent(EventsQ29787::event_shang_talk_4, 2s);
                    }

                }

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EventsQ29787::event_shang_talk_0:
                        Talk(TalksQ29787::shang_talk_0);
                        break;

                    case EventsQ29787::event_shang_path_0:
                        me->GetMotionMaster()->MovePath(PathQ29787::path_zhang_0, false);
                        break;

                    case EventsQ29787::event_shang_talk_4:
                        Talk(TalksQ29787::shang_talk_4);
                        me->GetMotionMaster()->MovePath(PathQ29787::path_zhang_1, false);
                        break;
                    }
                }
            }

        private:
            EventMap events;
            ObjectGuid PlayerGUID;
            bool guardianDead = false;
        };

        CreatureAI* GetAI(Creature* c) const override
        {
            return new npc_master_shangAI(c);
        }
    };

    // 55918 & 55649
    class npc_shang_xi_hot_air_balloon : public CreatureScript
    {
    public:
        npc_shang_xi_hot_air_balloon() : CreatureScript("npc_shang_xi_hot_air_balloon") { }

        struct npc_shang_xi_hot_air_balloonAI : public ScriptedAI
        {
            npc_shang_xi_hot_air_balloonAI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetNpcFlag(UNIT_NPC_FLAG_SPELLCLICK);
                me->SetUnitFlag(UNIT_FLAG_UNINTERACTIBLE);
            }

            void Reset() override
            {
                events.Reset();
                me->SetCanFly(true);
                me->SetWalk(false);
                me->SetSpeed(MOVE_RUN, 1.0f);
                me->SetSpeed(MOVE_FLIGHT, 1.0f);
            }

            void PassengerBoarded(Unit* passenger, int8 /*seat*/, bool apply) override
            {
                if (apply && me->GetEntry() == Npcs::npc_balloon_q29791)
                {
                    if (passenger->GetTypeId() == TYPEID_PLAYER)
                    {
                        PlayerGUID = passenger->GetGUID();
                        events.ScheduleEvent(EventsBalloonEvent::event_aysa_jump_in, 2s);
                    }
                }

                if (apply && me->GetEntry() == Npcs::npc_balloon_spawned)
                {
                    if (passenger->GetTypeId() == TYPEID_PLAYER)
                    {
                        passenger->ToPlayer()->KilledMonsterCredit(Npcs::credit_the_suffering_of_shenzinsu_1);
                        passenger->ToPlayer()->DisableMirrorTimer(FATIGUE_TIMER);
                        PhasingHandler::OnConditionChange(passenger, true);
                    }

                    else
                    {
                        // Get GUIDs of Aysa and Ji when they board in
                        if (Creature* creature = passenger->ToCreature())
                        {
                            if(creature->GetEntry() == Npcs::npc_aysa_q29791)
                                AysaGUID = creature->GetGUID();

                            if (creature->GetEntry() == Npcs::npc_ji_q29791)
                                JiGUID = creature->GetGUID();
                        }
                    }
                }
            }

            void WaypointReached(uint32 nodeId, uint32 /*pathId*/) override
            {
                if (me->GetEntry() == Npcs::npc_balloon_spawned)
                {
                    Creature* ji = ObjectAccessor::GetCreature(*me, JiGUID);
                    Creature* aysa = ObjectAccessor::GetCreature(*me, AysaGUID);

                    switch (nodeId)
                    {
                    case 0:
                    {
                        Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID);
                        if (!player || !aysa || !ji)
                            break;

                        scheduler.Schedule(2s, [ji, player](TaskContext /*task*/)
                            {
                                ji->AI()->Talk(0, player);
                            });

                        scheduler.Schedule(5s, [aysa](TaskContext /*task*/)
                            {
                                aysa->AI()->Talk(0);
                            });

                        scheduler.Schedule(10s, [ji](TaskContext /*task*/)
                            {
                                ji->AI()->Talk(1);
                            });

                        break;
                    }

                    case 1:
                    {
                        me->SetSpeed(MOVE_RUN, 2.0f);
                        me->SetSpeed(MOVE_FLIGHT, 2.0f);

                        if (!aysa || !ji)
                            break;

                        scheduler.Schedule(1s, [aysa](TaskContext /*task*/)
                            {
                                aysa->AI()->Talk(1);
                            });

                        scheduler.Schedule(5s, [ji](TaskContext /*task*/)
                            {
                                ji->AI()->Talk(2);
                            });

                        scheduler.Schedule(8s, [aysa](TaskContext /*task*/)
                            {
                                aysa->AI()->Talk(2);
                            });

                        break;
                    }

                    case 3:
                    {
                        Creature* shenzinsu = me->FindNearestCreatureWithOptions(500.f, { .CreatureId = Npcs::npc_shenzinsu_q29791, .IgnorePhases = true });
                        if (!shenzinsu || !aysa)
                            break;

                        aysa->CastSpell(shenzinsu, 114888, true);

                        scheduler.Schedule(7s, [aysa](TaskContext /*task*/)
                            {
                                aysa->AI()->Talk(3);
                            });

                        break;
                    }

                    case 4:
                    {
                        Creature* shenzinsu = me->FindNearestCreatureWithOptions(500.f, { .CreatureId = Npcs::npc_shenzinsu_q29791, .IgnorePhases = true });
                        if (shenzinsu)
                        {
                            me->CastSpell(shenzinsu, 114898, true);
                            me->CastSpell(shenzinsu, 106759, true);

                            scheduler.Schedule(2s, [shenzinsu](TaskContext /*task*/)
                                {
                                    shenzinsu->AI()->Talk(0);
                                });

                            scheduler.Schedule(7s, [shenzinsu](TaskContext /*task*/)
                                {
                                    shenzinsu->AI()->Talk(1);
                                });

                        }
                        break;
                    }

                    case 5:
                    {
                        break;
                    }
                    default:
                        break;
                    }
                }
            }

            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);
                scheduler.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EventsBalloonEvent::event_aysa_jump_in:
                    {
                        Creature* Aysa = me->FindNearestCreature(Npcs::npc_aysa_q29790, 10.f);
                        if (Aysa)
                        {
                            Vehicle* vehicle = me->GetVehicleKit();
                            Aysa->EnterVehicle(vehicle->GetBase(), 1);
                            events.ScheduleEvent(EventsBalloonEvent::event_spawn_balloon, 2s);
                        }
                        break;
                    }

                    case EventsBalloonEvent::event_spawn_balloon:
                    {
                        Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID);
                        Creature* spawnedBalloon = player->SummonCreature(Npcs::npc_balloon_spawned, player->GetPosition());
                        if (spawnedBalloon)
                        {
                            BalloonGUID = spawnedBalloon->GetGUID();
                            Vehicle* vehicle = spawnedBalloon->GetVehicleKit();
                            player->EnterVehicle(vehicle->GetBase(), -1);
                            spawnedBalloon->LoadPath(5564900);
                            spawnedBalloon->GetMotionMaster()->MovePath(5564900, false);
                        }
                        break;
                    }
                    
                    }
                }
            }

        private:
            ObjectGuid  PlayerGUID;
            ObjectGuid  JiGUID;
            ObjectGuid  AysaGUID;
            ObjectGuid  BalloonGUID;
            EventMap    events;
            TaskScheduler scheduler;
        };

        CreatureAI* GetAI(Creature* c) const override
        {
            return new npc_shang_xi_hot_air_balloonAI(c);
        }
    };
}

void AddSC_custom_the_wandering_isle_npcs()
{
    using namespace Scripts::Custom::TheWanderingIsle;

    new npc_huo_follower();
    new npc_chia_hui_autumnleaf();
    new npc_shanxi_quest();
    new npc_deng();
    new npc_cai();
    new npc_whitefeather_crane();
    new npc_jojo_ironbrow_summon();
    new npc_tushui_monk_on_pole();
    new npc_balance_pole();
    new npc_fang_she();
    new npc_shu_playing();
    new npc_ox_cart();
    new npc_shu_follower();
    new npc_shu_at_farmstead_pool();
    new npc_shu_at_farmstead_play();
    new npc_shanxi_quest2();
    new npc_lorewalker_ruolin();
    new at_lorewalker_zan();
    new npc_ruk_ruk();
    new npc_ruk_ruk_rocket();
    new npc_aysa_outside_chambers_of_whispers();
    new npc_zhaoren();
    new npc_firework_launcher();
    new npc_master_shang_q29787();
    new npc_shang_xi_hot_air_balloon();
}
