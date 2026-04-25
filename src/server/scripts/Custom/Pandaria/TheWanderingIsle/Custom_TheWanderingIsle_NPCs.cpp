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
#include "CreatureAIImpl.h"
#include "EventMap.h"
#include "GameObject.h"
#include "Map.h"
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
#include "WorldStateMgr.h"

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

                    _events.RescheduleEvent(Events::event_delivery_huo, 20s);
                }
            }

            void MovementInform(uint32 type, uint32 pointId) override
            {
                if (type != POINT_MOTION_TYPE)
                    return;

                Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid);

                if (pointId == 1)
                {
                    me->CastSpell(player, Spells::spell_fire_form);

                    _events.RescheduleEvent(Events::event_second_huo_position, 7s);
                }
                else if (pointId == 2)
                {
                    me->CastSpell(player, Spells::spell_forcecast_fire_turn_in_statue_brazier_change);
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
                    case Events::event_delivery_huo:
                    {
                        Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid);

                        if (player->GetDistance(973.311829f, 3603.419434f, 195.528030f) <= 7.0f)
                        {
                            me->GetMotionMaster()->Remove(FOLLOW_MOTION_TYPE);
                            me->SetFloating(true);
                            me->SetSpeed(MOVE_RUN, 3.0f);
                            me->GetMotionMaster()->MovePoint(1, Positions::huoFirstPoint, true, { 6.249388f });
                        }
                        else
                            _events.RescheduleEvent(Events::event_delivery_huo, 2s);
                        break;
                    }
                    case Events::event_second_huo_position:
                        me->GetMotionMaster()->MovePoint(2, Positions::huoSecondPoint);
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

                    _scheduler.Schedule(10s, [this](TaskContext const&)
                        {
                            Talk(TalksQ29423::shanxi_talk_2);
                        });
                    _scheduler.Schedule(26s, [this](TaskContext const&)
                        {
                            Talk(TalksQ29423::shanxi_talk_3);
                        });
                    _scheduler.Schedule(40s, [this](TaskContext const&)
                        {
                            Talk(TalksQ29423::shanxi_talk_4);
                        });
                    _scheduler.Schedule(49s, [ji](TaskContext const&)
                        {
                            ji->AI()->Talk(TalksQ29423::ji_talk);
                            ji->GetMotionMaster()->MovePath(Paths::ji_away, false);
                            ji->DespawnOrUnsummon(20s);
                        });
                    _scheduler.Schedule(52s, [this](TaskContext const&)
                        {
                            Talk(TalksQ29423::shanxi_talk_5);
                        });
                    _scheduler.Schedule(60s, [aysa](TaskContext const&)
                        {
                            aysa->AI()->Talk(TalksQ29423::aysa_talk);
                            aysa->GetMotionMaster()->MovePath(Paths::aysa_away, false);
                            aysa->DespawnOrUnsummon(20s);
                        });
                    _scheduler.Schedule(63s, [this, player](TaskContext const&)
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
                if (spellInfo->Id != Spells::spell_start_talk_event)
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

                    _scheduler.Schedule(2s, [this, lin, zhen, player](TaskContext const&)
                        {
                            me->SetFacingToObject(player);
                            lin->SetFacingToObject(player);
                            zhen->SetFacingToObject(player);

                            lin->AI()->Talk(TalksQ29423::brewer_lin_talk);
                        });
                    _scheduler.Schedule(4s, [this, lin, zhen](TaskContext const&)
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

                _scheduler.Schedule(7s, [this](TaskContext const&)
                    {
                        me->AI()->Talk(Talks::CaiTalk0);
                    });
                _scheduler.Schedule(21s, [this](TaskContext const&)
                    {
                        me->AI()->Talk(Talks::CaiTalk1);
                    });
                _scheduler.Schedule(37s, [this](TaskContext const&)
                    {
                        me->AI()->Talk(Talks::CaiTalk2);
                    });
                _scheduler.Schedule(45s, [this](TaskContext const&)
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

                _scheduler.Schedule(14s, [this](TaskContext const&)
                    {
                        Talk(Talks::DengTalk0);
                    });
                _scheduler.Schedule(29s, [this](TaskContext const&)
                    {
                        Talk(Talks::DengTalk1);
                    });
                _scheduler.Schedule(45s, [this](TaskContext const&)
                    {
                        TempSummon* summon = me->ToTempSummon();
                        if (!summon)
                            return;

                        Unit* summoner = summon->GetSummonerUnit();
                        if (!summoner)
                            return;

                        Talk(Talks::DengTalk2);
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

                    _scheduler.Schedule(1s, [this, player](TaskContext const&)
                        {
                            Talk(TalksQ29662::Jojo_Talk_0, player);
                        });
                    _scheduler.Schedule(3s, [this](TaskContext const&)
                        {
                            me->SetAIAnimKitId(0);
                        });
                    _scheduler.Schedule(4s, [this](TaskContext const&)
                        {
                            me->GetMotionMaster()->MovePoint(1, PositionsQ29662::JojoMovePoint);
                        });
                    _scheduler.Schedule(6200ms, [this](TaskContext const&)
                        {
                            me->CastSpell(me, Spells::spell_jojo_headbash_reeds_cast);
                        });
                    _scheduler.Schedule(8700ms, [this, player](TaskContext const&)
                        {
                            me->RemoveAurasDueToSpell(Spells::spell_jojo_headbash_stack_of_reeds_impact);
                            Talk(TalksQ29662::Jojo_Talk_1, player);
                        });
                    _scheduler.Schedule(14700ms, [this](TaskContext const&)
                        {
                            me->GetMotionMaster()->MovePath(Paths::path_jojo_q29662, false);
                        });
                }
            }

            void WaypointPathEnded(uint32 /*nodeId*/, uint32 pathId) override
            {
                if (pathId == Paths::path_jojo_q29662)
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
                _events.RescheduleEvent(Events::event_check_players, 1s);
            }

            void EnterCombat()
            {
                _events.CancelEvent(Events::event_check_players);
                _events.RescheduleEvent(Events::event_cast_razor_beak, 8s);
            }

            void UpdateAI(uint32 diff) override
            {
                _events.Update(diff);

                if (!UpdateVictim())
                {
                    if (_events.ExecuteEvent() == Events::event_check_players)
                    {
                        if (Player* player = me->SelectNearestPlayer(10.0f))
                        {
                            if (player->IsAlive() && !player->IsGameMaster() && player->HasAura(Spells::spell_curse_of_the_frog))
                            {
                                AttackStart(player);
                                EnterCombat();
                                return;
                            }
                        }
                        _events.RescheduleEvent(Events::event_check_players, 1s);
                    }
                    return;
                }

                if (Unit* victim = me->GetVictim())
                {
                    if (!victim->HasAura(Spells::spell_curse_of_the_frog))
                    {
                        if (me->IsInCombat())
                        {
                            EnterEvadeMode();
                            _events.RescheduleEvent(Events::event_check_players, 1s);
                        }
                        return;
                    }
                }

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case Events::event_cast_razor_beak:
                        if (Unit* target = me->GetVictim())
                        {
                            DoCast(target, Spells::spell_razor_beak);
                        }
                        _events.RescheduleEvent(Events::event_cast_razor_beak, 8s);
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
                _events.ScheduleEvent(Events::event_monk_switch_pole, 1s);
                me->RestoreFaction();
                me->SetReactState(REACT_DEFENSIVE);
            }

            void SpellHit(WorldObject* caster, SpellInfo const* spellInfo) override
            {
                if (spellInfo->Id == Spells::spell_force_cast_ride_pole)
                {
                    if (Unit* unitCaster = caster->ToUnit())
                        DoCast(unitCaster, Spells::spell_monk_ride_pole, true);
                }
            }

            void JustEnteredCombat(Unit* who) override
            {
                me->SetReactState(REACT_AGGRESSIVE);
                AttackStart(who);

                _events.ScheduleEvent(Events::event_cast_throw_rock, 0s);
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
                    case Events::event_cast_throw_rock:
                    {
                        if (victim && !me->IsWithinMeleeRange(victim) && !me->HasUnitState(UNIT_STATE_CASTING))
                            DoCast(victim, Spells::spell_throw_rock);

                        _events.ScheduleEvent(Events::event_cast_throw_rock, 2500ms);
                        break;
                    }
                    
                    case Events::event_monk_switch_pole:
                    {
                        if (!me->IsInCombat())
                        {
                            SwitchPole();
                            _events.ScheduleEvent(Events::event_monk_switch_pole, 15s, 30s);
                        }
                        break;
                    }
                    }
                }
                me->DoMeleeAttackIfReady();
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

                    if (!pole->HasAura(Spells::spell_monk_ride_pole) && !pole->HasAura(Spells::spell_ride_vehicle_pole))
                    {
                        pole->CastSpell(me, Spells::spell_force_cast_ride_pole, true);
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
                        _events.ScheduleEvent(Events::event_cast_transform, 1s);
                    else
                    {
                        if (me->GetEntry() == Npcs::npc_training_bell_pole)
                            DoCast(passenger, Spells::spell_training_bell_force_cast_ride_vehicle, true);

                        passenger->AddAura(Spells::spell_ride_vehicle_pole, passenger);
                        passenger->RemoveAurasDueToSpell(Spells::spell_curse_of_the_frog);
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
                    case Events::event_cast_transform:
                        // Transform is casted only when in frog pool
                        Unit* passenger = ObjectAccessor::GetUnit(*me, _passengerGuid);

                        if (passenger->HasAura(Spells::spell_training_bell_exclusion_aura))
                            passenger->RemoveAura(Spells::spell_training_bell_exclusion_aura);

                        if (passenger->HasAura(Spells::spell_ride_vehicle_pole))
                            passenger->RemoveAura(Spells::spell_ride_vehicle_pole);

                        if (passenger->HasAura(Spells::spell_curse_of_the_frog))
                            passenger->RemoveAura(Spells::spell_curse_of_the_frog);

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
                    if (eventId == Events::event_cast_serpent_strike)
                    {
                        DoCastVictim(128409); // Fang-she's attack spell
                        events.ScheduleEvent(Events::event_cast_serpent_strike, 3s, 5s); // repeat
                    }
                }
            }

            void JustEngagedWith(Unit* who) override // Runs every time creature gets in combat
            {
                me->EngageWithTarget(who);
                events.ScheduleEvent(Events::event_cast_serpent_strike, 3s, 5s);
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
                    events.ScheduleEvent(Events::event_shu_set_orientation, 500ms);
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
                    events.ScheduleEvent(Events::event_cast_jump_spell, 1s);
                    startAI = false;
                }

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case Events::event_cast_jump_spell:
                        if (urand(0, 2) != 0)
                            jumpPosition = urand(Misc::SHU_JUMP_POSITION_0, Misc::SHU_JUMP_POSITION_3);
                        else
                            jumpPosition = positionBefore;

                        if (jumpPosition == positionBefore)
                        {
                            events.CancelEvent(Events::event_shu_set_orientation);
                            events.ScheduleEvent(Events::event_shu_summon_water_spout, 1500ms);
                        }
                        else
                        {
                            DoCast(jumpSpells[jumpPosition]);
                            positionBefore = jumpPosition;
                        }
                        break;
                    case Events::event_shu_set_orientation:
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
                        events.ScheduleEvent(Events::event_shu_summon_water_spout, 1500ms);
                        break;
                    case Events::event_shu_summon_water_spout:
                    {
                        // Pick a random spawn slot within the current zone
                        uint8 randomSlot = urand(0, Misc::BUNNY_SPAWN_MAX_SLOTS - 1);

                        // Access the spawn position using zone and slot
                        Position targetPos = Positions::ShuSpawnPositions[jumpPosition][randomSlot];

                        // Cast the summoning spell at the chosen location
                        //me->CastSpell(Position(targetPos), SPELL_SUMMON_WATER_SPOUT, true);
                        // Dynamic position spell casting summon not supported. we summon creature directly instead

                        Creature* bunny = me->SummonCreature(Npcs::npc_bunny_water_spout, targetPos, TEMPSUMMON_MANUAL_DESPAWN);
                        if (bunny)
                            DoCast(Spells::spell_water_spout);
                        events.ScheduleEvent(Events::event_cast_jump_spell, 6s);
                        break;
                    }
                    }
                }
            }

        private:
            EventMap events;
            uint32 jumpSpells[4] = { Spells::spell_jump_front_right, Spells::spell_jump_front_left, Spells::spell_jump_back_right, Spells::spell_jump_back_left };
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
            npc_ox_cartAI(Creature* creature) : ScriptedAI(creature), _passengerGuid()
            {
                Initialize();
            }

            void Initialize()
            {
                switch (me->GetEntry())
                {
                case Npcs::npc_vehicle_ox:
                    _pathId = Paths::path_ox;
                    break;
                case Npcs::npc_vehicle_ox_farmstead:
                    _pathId = Paths::path_ox_farmstead;
                    break;
                case Npcs::npc_vehicle_ox_forest:
                    _pathId = Paths::path_ox_forest;
                    break;
                case Npcs::npc_vehicle_cart:
                    _isCart = true;
                    _pathId = Paths::path_cart;
                    _ejectNodeId = Paths::path_node_cart_remove_passenger;
                    _creditNPC = Npcs::npc_cart;
                    _questId = Quests::quest_the_source_of_livelihood;
                    _yakNPC = Npcs::npc_vehicle_ox;
                    break;
                case Npcs::npc_vehicle_cart_farmstead:
                    _isCart = true;
                    _pathId = Paths::path_cart_farmstead;
                    _ejectNodeId = Paths::path_node_cart_remove_passenger;
                    _creditNPC = Npcs::npc_cart_farmstead;
                    _questId = Quests::quest_the_spirit_and_body_of_shenzinsu;
                    _yakNPC = Npcs::npc_vehicle_ox_farmstead;
                    break;
                case Npcs::npc_vehicle_cart_forest:
                    _isCart = true;
                    _pathId = Paths::path_cart_forest;
                    _ejectNodeId = Paths::path_node_cart_forest_remove_passenger;
                    _creditNPC = Npcs::npc_cart_forest;
                    _questId = Quests::quest_new_allies;
                    _yakNPC = Npcs::npc_vehicle_ox_forest;
                    break;
                default:
                    break;
                }

                me->SetPrivateObjectOwner(ObjectGuid::Empty); // Needed otherwise FindCreature does not work
                me->SetReactState(REACT_PASSIVE);
            }

            void Reset() override
            {
                _events.Reset();
                Initialize();

                if(!_isCart)
                    _events.ScheduleEvent(Events::event_ox_cart_path_start, 1400ms); // Delay
            }

            void IsSummonedBy(WorldObject* /*summoner*/) override
            {
                //me->ToTempSummon()->SetTempSummonType(TEMPSUMMON_MANUAL_DESPAWN);
            }

            void PassengerBoarded(Unit* passenger, int8 /*seat*/, bool apply) override
            {
                Player* player = passenger->ToPlayer();
                if (apply && player)
                {
                    if (_isCart)
                    {
                        _passengerGuid = player->GetGUID(); // Store for later use (e.g., for eject)
                        me->CastSpell(player, Spells::spell_force_vehicle_ride);
                        _events.ScheduleEvent(Events::event_ox_cart_path_start, 1800ms); // Delay

                        if (player->hasQuest(_questId))
                            player->KilledMonsterCredit(_creditNPC, _passengerGuid);

                        // Rope spells are currently broken and need fixing :(
                        //_events.ScheduleEvent(Events::event_ox_cart_ropes, 1s);
                    }
                }
            }

            void WaypointReached(uint32 nodeId, uint32 /*pathId*/) override
            {
                if (_isCart && nodeId == _ejectNodeId)
                    me->CastSpell(me, Spells::spell_eject_passengers);
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
                    case Events::event_ox_cart_ropes:
                    {
                        Unit* yak = me->FindNearestCreatureWithOptions(10.f, { .CreatureId = _yakNPC, .IgnorePhases = true } );
                        if (yak)
                        {
                            me->CastSpell(yak, Spells::spell_rope_left);
                            yak->CastSpell(me, Spells::spell_rope_right);
                        }
                        break;
                    }

                    case Events::event_ox_cart_path_start:
                        me->LoadPath(_pathId);
                        me->GetMotionMaster()->MovePath(_pathId, false);
                        break;
                    default:
                        break;
                    }

                }
            }

        private:
            EventMap _events;
            ObjectGuid _passengerGuid;
            uint32 _creditNPC = 0;
            uint32 _yakNPC = 0;
            uint32 _questId = 0;
            uint32 _pathId = 0;
            uint8 _ejectNodeId = 0;
            bool _isCart = false;
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
                me->CastSpell(me, Spells::spell_shu_splash, true);

                Unit* owner = me->GetOwner();
                if (owner && owner->ToPlayer())
                    _playerGuid = owner->ToPlayer()->GetGUID();

                _events.ScheduleEvent(Events::event_shu_follower_check_player_quest, 30s);

                if (owner && owner->ToPlayer()->IsActiveQuest(Quests::quest_the_source_of_livelihood))
                    _events.ScheduleEvent(Events::event_shu_singing_pools_check_player_vehicle, 1s);
            }

            void UpdateAI(uint32 diff) override
            {
                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case Events::event_shu_follower_path_start:
                    {
                        me->GetMotionMaster()->Clear();
                        me->StopMoving();
                        me->LoadPath(Paths::path_shu_follower);
                        me->GetMotionMaster()->MovePath(Paths::path_shu_follower, false);
                        _pathStarted = true;
                        break;
                    }
                    case Events::event_shu_follower_check_player_quest:
                    {
                        if (!_pathStarted)
                        {
                            Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid);

                            if (player && player->GetQuestStatus(Quests::quest_the_source_of_livelihood) == QUEST_STATUS_REWARDED)
                            {
                                _events.ScheduleEvent(Events::event_shu_follower_path_start, 0s);
                            }
                            else
                                _events.ScheduleEvent(Events::event_shu_follower_check_player_quest, 1s);
                        }
                        break;
                    }
                    case Events::event_shu_singing_pools_check_player_vehicle:
                    {
                        if (me->GetVehicle())
                            break;

                        Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid);
                        if (!player)
                            break;

                        Vehicle* veh = player->GetVehicle();
                        if (!veh || !veh->IsVehicleInUse())
                        {
                            _events.ScheduleEvent(Events::event_shu_singing_pools_check_player_vehicle, 500ms);
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

                if (nodeId == Paths::path_node_shu_remove)
                {
                    me->SetWalk(true);
                    player->RemoveAurasDueToSpell(Spells::spell_summon_spirit_of_water);
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

                _events.ScheduleEvent(Events::event_shu_farmstead_play, 0s);
            }

            bool OnGossipSelect(Player* player, uint32 menuId, uint32 gossipListId) override
            {
                _playerGuid = player->GetGUID();

                if (menuId == Gossip::shu_farmstead_gossip_menu && gossipListId == 0)
                {
                    CloseGossipMenuFor(player);
                    //player->KilledMonsterCredit(Npcs::credit_not_in_the_face_1);
                    //player->CastSpell(player, 105891);
                    if (Creature* wugou = GetClosestCreatureWithEntry(me, Npcs::npc_wugou_farmstead, 70.0f))
                        wugou->DespawnOrUnsummon();
                    me->DespawnOrUnsummon();
                    player->CastSpell(player, Spells::spell_summon_spirits_water_earth);
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
                    case Events::event_shu_farmstead_play:
                    {
                        // Pick a random spawn slot within the current zone
                        uint8 randomSlot = urand(0, 4);

                        // Cast the summoning spell at the chosen location
                        //me->CastSpell(Position(targetPos), SPELL_SUMMON_WATER_SPOUT, true);
                        // Dynamic position spell casting summon not supported. we summon creature directly instead

                        Creature* bunny = me->SummonCreature(Npcs::npc_bunny_water_spout_farmstead, Positions::ShuFarmsteadPlay[randomSlot], TEMPSUMMON_MANUAL_DESPAWN);
                        if (bunny)
                            DoCast(Spells::spell_water_spout);
                        _events.ScheduleEvent(Events::event_shu_farmstead_play, 6s);
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
                    _events.ScheduleEvent(Events::event_shu_farmstead_path_start_1, 0s);
                else
                {
                    _events.ScheduleEvent(Events::event_try_remove_wugou_sleep, 200ms);
                    _events.ScheduleEvent(Events::event_shu_farmstead_check_player_vehicle, 1s);
                }
            }

            void UpdateAI(uint32 diff) override
            {
                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case Events::event_shu_farmstead_path_start_1:
                    {
                        if (!_path1Started)
                        {
                            me->GetMotionMaster()->Clear();
                            me->StopMoving();
                            me->LoadPath(Paths::path_shu_farmstead_1);
                            me->GetMotionMaster()->MovePath(Paths::path_shu_farmstead_1, false);
                            _path1Started = true;
                        }
                        break;
                    }
                    case Events::event_shu_farmstead_path_start_2:
                    {
                        if (!_path2Started)
                        {
                            me->StopMoving();
                            me->GetMotionMaster()->Clear();
                            me->LoadPath(Paths::path_shu_farmstead_2);
                            me->GetMotionMaster()->MovePath(Paths::path_shu_farmstead_2, false);
                            _path2Started = true;
                        }
                        break;
                    }
                    case Events::event_shu_wakes_wugou:
                    {
                        Creature* wugou = GetClosestCreatureWithEntry(me, Npcs::npc_wugou_q29774, 30.0f);
                        if (wugou)
                        {
                            me->CastSpell(wugou, Spells::spell_shu_watersplash_wugou);
                            wugou->CastSpell(me, Spells::spell_shu_watersplash);
                            wugou->RemoveAllAuras();
                            me->RemoveAllAuras();
                            wugou->SetStandState(UNIT_STAND_STATE_STAND);
                            me->CastSpell(me, Spells::spell_water_spirit_laugh);

                            Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid);
                            if (!player)
                                return;

                            me->CastSpell(player, Spells::spell_shu_watersplash_credit);
                            //player->KilledMonsterCredit(Npcs::credit_not_in_the_face_2);
                            wugou->GetMotionMaster()->MoveFollow(me, 5.0f, 5.0f);
                            wugou->DespawnOrUnsummon(30s);
                            _events.ScheduleEvent(Events::event_shu_farmstead_path_start_2, 5s);
                        }
                        break;
                    }
                    case Events::event_shu_farmstead_check_player_vehicle:
                    {
                        if (me->GetVehicle())
                            break;

                        Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid);
                        if (!player)
                            break;

                        Vehicle* veh = player->GetVehicle();
                        if (!veh || !veh->IsVehicleInUse())
                        {
                            _events.ScheduleEvent(Events::event_shu_farmstead_check_player_vehicle, 500ms);
                            break;
                        }

                        me->EnterVehicle(veh->GetBase(), -1);
                        break;
                    }
                    case Events::event_try_remove_wugou_sleep:
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
                        _events.ScheduleEvent(Events::event_try_remove_wugou_sleep, 200ms);
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
                case Paths::path_shu_farmstead_1:
                {
                    me->StopMoving();
                    me->GetMotionMaster()->Clear();

                    Creature* wugou = GetClosestCreatureWithEntry(me, Npcs::npc_wugou_q29774, 30.0f);
                    if (wugou)
                    {
                        
                        me->CastSpell(me, Spells::spell_water_spirit_laugh);
                        _events.ScheduleEvent(Events::event_shu_wakes_wugou, 5s);
                    }
                    break;
                }
                case Paths::path_shu_farmstead_2:
                {
                    me->StopMoving();
                    me->GetMotionMaster()->Clear();
                    me->CastSpell(me, Spells::spell_water_spirit_laugh);
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

                    _scheduler.Schedule(3s, [this](TaskContext const&)
                        {
                            Talk(TalksQ29775::shanxi_talk_8);
                        });
                    _scheduler.Schedule(6s, [this](TaskContext const&)
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

            void SetData(uint32 id, uint32 value) override
            {
                if (id == 1 && value == 1)
                    StartLorewalkerStory();
            }

            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case Events::event_start_dialogue:
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
                            events.ScheduleEvent(Events::event_start_dialogue, entry.delay);
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
                me->CastSpell(me, Spells::spell_ruolin_singing);
                me->HandleEmoteCommand(EMOTE_ONESHOT_YES);
                events.ScheduleEvent(Events::event_start_dialogue, 30s);
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
                events.ScheduleEvent(Events::event_rukruk_cast_aim, 10s);
                events.ScheduleEvent(Events::event_rukruk_cast_ooksplosions, 30s);
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
                    case Events::event_rukruk_cast_aim:
                        if (me->HasUnitState(UNIT_STATE_CASTING))
                        {
                            events.RescheduleEvent(Events::event_rukruk_cast_aim, 1s);
                            break;
                        }
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random))
                        {
                            me->SetFacingToObject(target);
                            CalculateSpellVisual(target);
                            DoCast(target, Spells::spell_aim);
                            events.ScheduleEvent(Events::event_rukruk_cast_aim, 15s, 25s);
                        }
                        break;
                    case Events::event_rukruk_cast_ooksplosions:
                        if (me->HasUnitState(UNIT_STATE_CASTING))
                        {
                            events.RescheduleEvent(Events::event_rukruk_cast_ooksplosions, 1s);
                            break;
                        }
                        DoCast(Spells::spell_ookslosions);
                        events.ScheduleEvent(Events::event_rukruk_cast_ooksplosions, 25s, 35s);
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
                        me->SendPlaySpellVisual(pos, Spells::spell_aim_visual, 0, 0, 0.0f, false, 2.0f);
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
                events.ScheduleEvent(Events::event_rukruk_rocket_fire, 500ms);
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
                    DoCast(Spells::spell_rocket_explosion_visual);
                    DoCast(Spells::spell_rocket_explosion_damage);
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
                    case Events::event_rukruk_rocket_fire:
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

                events.ScheduleEvent(Events::event_aysa_outside_chambers_init, 1s);
            }

            void MovementInform(uint32 type, uint32 id) override
            {
                if (type != POINT_MOTION_TYPE)
                    return;

                if (id == 1) // reached player
                {
                    Talk(Talks::aysa_chamber_of_whispers_1);
                    events.ScheduleEvent(Events::event_aysa_outside_chambers_move1, 3s);
                }

                if (id == 2) // reached outside entrance
                {
                    events.ScheduleEvent(Events::event_aysa_outside_chambers_move2, 25s);
                }

                if (id == 3) // reached half way chamber
                {
                    Talk(Talks::aysa_chamber_of_whispers_2);
                    events.ScheduleEvent(Events::event_aysa_inside_chambers_move_dafeng, 10s);
                }
            }

            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case Events::event_aysa_outside_chambers_init:
                    {
                        Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID);
                        if (!player)
                        {
                            player = me->SelectNearestPlayer(10.f);
                            if (!player)
                                return;
                        }
                        Talk(Talks::aysa_chamber_of_whispers_0);
                        me->GetMotionMaster()->MovePoint(1, player->GetRandomNearPosition(3.f));
                        break;
                    }
                        
                    case Events::event_aysa_outside_chambers_move1:
                    {
                        me->GetMotionMaster()->MovePoint(2, Positions::AysaOutsideChamber);
                        break;
                    }

                    case Events::event_aysa_outside_chambers_move2:
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
                        me->GetMotionMaster()->MovePoint(3, Positions::AysaInsideChamber);
                        break;
                    }

                    case Events::event_aysa_inside_chambers_move_dafeng:
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
                me->GetMotionMaster()->MovePath(Paths::path_zhaoren_at_chamber, true);
                events.SetPhase(Misc::ZHAO_PHASE_FLYING);
                events.ScheduleEvent(Events::event_zhao_cast_lightning, 5s);
            }

            void SpellHit(WorldObject* caster, SpellInfo const* spell) override
            {
                if (spell->Id == Spells::spell_overpacked_firework)
                    if (!me->IsInCombat())
                        if (Unit* target = caster->ToUnit())
                            me->Attack(target, true);
            }

            void MovementInform(uint32 type, uint32 id) override
            {
                if (type == POINT_MOTION_TYPE && id == 1)
                    events.ScheduleEvent(Events::event_zhao_state_stun, 0s);
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
                    events.CancelEvent(Events::event_zhao_cast_lightning);
                    events.ScheduleEvent(Events::event_zhao_move_center, 0s);
                }
                if (phase == Misc::ZHAO_PHASE_GROUNDED && HealthBelowPct(25))
                {
                    phase++;
                    events.SetPhase(Misc::ZHAO_PHASE_STAY_IN_CENTER);
                    events.CancelEvent(Events::event_zhao_cast_lightning);
                    events.ScheduleEvent(Events::event_zhao_move_center, 0s);
                }
                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case Events::event_zhao_cast_lightning:
                    {
                        auto const& threatList = me->GetThreatManager().GetUnsortedThreatList();
                        if (threatList.begin() != threatList.end())
                        {
                            for (ThreatReference const* ref : threatList)
                            {
                                if (Unit* target = ref->GetVictim())
                                {
                                    if (target->IsPlayer())
                                        DoCast(target, Spells::spell_lightning_pool);
                                }
                            }

                            events.ScheduleEvent(Events::event_zhao_cast_lightning, events.IsInPhase(Misc::ZHAO_PHASE_FLYING) ? 5s : 3500ms);

                            if (!sweepScheduled && events.IsInPhase(Misc::ZHAO_PHASE_STAY_IN_CENTER))
                            {
                                events.ScheduleEvent(Events::event_zhao_cast_sweep, 15s, 0, Misc::ZHAO_PHASE_STAY_IN_CENTER);
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
                    case Events::event_zhao_move_center:
                        me->GetMotionMaster()->MovePoint(1, Positions::ZhaoPos);
                        break;

                    case Events::event_zhao_state_stun:
                        DoCast(Spells::spell_stunned_by_fireworks);
                        events.ScheduleEvent(Events::event_zhao_cast_sweep, 12s);
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

                    case Events::event_zhao_cast_sweep:
                        events.CancelEvent(Events::event_zhao_cast_lightning);
                        DoCast(Spells::spell_serpent_sweep);
                        sweepScheduled = false;
                        events.ScheduleEvent(Events::event_zhao_cast_lightning, 3500ms, 0, Misc::ZHAO_PHASE_STAY_IN_CENTER);
                        events.ScheduleEvent(Events::event_zhao_resume_path, 5s, 0, Misc::ZHAO_PHASE_GROUNDED);
                        if (events.IsInPhase(Misc::ZHAO_PHASE_GROUNDED))
                            if (Creature* ji = ObjectAccessor::GetCreature(*me, JiGUID))
                                ji->AI()->SetData(Misc::DATA_PHASE_OOC, Misc::DATA_PHASE_OOC);
                        break;
                    case Events::event_zhao_resume_path:
                        me->GetMotionMaster()->MovePath(Paths::path_zhaoren_at_chamber, true);
                        events.SetPhase(Misc::ZHAO_PHASE_FLYING);
                        events.ScheduleEvent(Events::event_zhao_cast_lightning, 5s);
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
                me->RemoveAurasDueToSpell(Spells::spell_firework_inactive);
                events.Reset();
            }

            // Player clicks launcher ? cast 125961 on launcher
            void OnSpellClick(Unit* clicker, bool) override
            {
                clicker->CastSpell(me, Spells::spell_overpacked_firework_launcher_ping, true);
            }

            // Launcher is hit by 125961
            void SpellHit(WorldObject* caster, SpellInfo const* spellInfo) override
            {
                if (spellInfo->Id == Spells::spell_overpacked_firework_launcher_ping &&
                    state == Misc::LAUNCHER_STATE_READY)
                {
                    Unit* player = caster->ToUnit();
                    if (!player)
                        return;

                    // Cast 125970 on player (matches SAI)
                    me->CastSpell(player, Spells::spell_aicast_overpacked_fireworkd, true);

                    // Play animation
                    me->PlayOneShotAnimKitId(2538);

                    // Apply inactive aura
                    me->CastSpell(me, Spells::spell_firework_inactive, true);

                    // Ping Ji Firepaw
                    if (Creature* ji = me->FindNearestCreature(Npcs::npc_ji_q29786, 30.f))
                        me->CastSpell(ji, 104080, true);

                    // Enter USED state
                    state = Misc::LAUNCHER_STATE_USED;

                    // Periodic Ji pings every 5s
                    events.ScheduleEvent(Events::event_firework_launcher_periodic_ping, 5s);

                    // Recharge after 21s
                    events.ScheduleEvent(Events::event_firework_launcher_recharge, 21s);
                }
            }

            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case Events::event_firework_launcher_periodic_ping:
                    {
                        if (state == Misc::LAUNCHER_STATE_USED)
                        {
                            if (Creature* ji = me->FindNearestCreature(Npcs::npc_ji_q29786, 30.f))
                                me->CastSpell(ji, 104080, true);

                            events.ScheduleEvent(eventId, 5s);
                        }
                        break;
                    }

                    case Events::event_firework_launcher_recharge:
                    {
                        state = Misc::LAUNCHER_STATE_READY;
                        me->RemoveAurasDueToSpell(Spells::spell_firework_inactive);
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

                events.ScheduleEvent(Events::event_shang_talk_0, 2s);
                events.ScheduleEvent(Events::event_shang_path_0, 3s);
            }

            void MovementInform(uint32 /*type*/, uint32 pointId) override
            {
                switch (pointId)
                {
                case 7: me->SetWalk(true); Talk(Talks::shang_talk_1); break;
                case 8: Talk(Talks::shang_talk_2); break;
                case 9: Talk(Talks::shang_talk_3); break;
                case 12: me->SetFacingTo(5.48033f); break;
                }
            }

            void WaypointPathEnded(uint32 /*nodeId*/, uint32 pathId) override
            {
                if (pathId == Paths::path_zhang_1)
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

                        events.ScheduleEvent(Events::event_shang_talk_4, 2s);
                    }

                }

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case Events::event_shang_talk_0:
                        Talk(Talks::shang_talk_0);
                        break;

                    case Events::event_shang_path_0:
                        me->GetMotionMaster()->MovePath(Paths::path_zhang_0, false);
                        break;

                    case Events::event_shang_talk_4:
                        Talk(Talks::shang_talk_4);
                        me->GetMotionMaster()->MovePath(Paths::path_zhang_1, false);
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
                me->SetSpeed(MOVE_RUN, 3.0f);
                me->SetSpeed(MOVE_FLIGHT, 3.0f);
            }

            void PassengerBoarded(Unit* passenger, int8 /*seat*/, bool apply) override
            {
                if (apply && me->GetEntry() == Npcs::npc_balloon_q29791)
                {
                    if (passenger->GetTypeId() == TYPEID_PLAYER)
                    {
                        PlayerGUID = passenger->GetGUID();
                        events.ScheduleEvent(Events::event_aysa_jump_in, 2s);
                    }
                }

                if (apply && me->GetEntry() == Npcs::npc_balloon_spawned)
                {
                    if (passenger->GetTypeId() == TYPEID_PLAYER)
                    {
                        passenger->ToPlayer()->KilledMonsterCredit(Npcs::credit_the_suffering_of_shenzinsu_1);
                        //passenger->ToPlayer()->DisableMirrorTimer(FATIGUE_TIMER);
                        PlayerGUID = passenger->GetGUID();
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
                    Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID);
                    Creature* ji = ObjectAccessor::GetCreature(*me, JiGUID);
                    Creature* aysa = ObjectAccessor::GetCreature(*me, AysaGUID);
                    if (!aysa || !ji || !player)
                        return;

                    switch (nodeId)
                    {
                    case 0:
                    {
                        scheduler.Schedule(2s, [ji, player](TaskContext const&)
                            {
                                ji->AI()->Talk(0, player);
                            });

                        scheduler.Schedule(5s, [aysa](TaskContext const&)
                            {
                                aysa->AI()->Talk(0);
                            });

                        scheduler.Schedule(10s, [ji](TaskContext const&)
                            {
                                ji->AI()->Talk(1);
                            });

                        break;
                    }

                    case 1:
                    {
                        me->SetSpeed(MOVE_RUN, 4.0f);
                        me->SetSpeed(MOVE_FLIGHT, 4.0f);

                        aysa->AI()->Talk(1);

                        scheduler.Schedule(5s, [ji](TaskContext const&)
                            {
                                ji->AI()->Talk(2);
                            });

                        scheduler.Schedule(10s, [aysa](TaskContext const&)
                            {
                                aysa->AI()->Talk(2);
                            });

                        break;
                    }

                    case 3:
                    {
                        scheduler.Schedule(8s, [aysa](TaskContext const&)
                            {
                                aysa->AI()->Talk(3);
                            });

                        scheduler.Schedule(18s, [aysa](TaskContext const&)
                            {
                                aysa->AI()->Talk(4);
                            });

                        break;
                    }

                    case 4:
                    {
                        Creature* shenzinsu = me->FindNearestCreatureWithOptions(500.f, { .CreatureId = Npcs::npc_shenzinsu_bunny, .IgnorePhases = true });
                        if (shenzinsu)
                        {
                            shenzinsu->setActive(true);
                            shenzinsu->Talk(Misc::shenzinsu_bunny_text_1, CHAT_MSG_MONSTER_SAY, 500.f, player);
                            me->PlayDirectSound(27822, player);

                            scheduler.Schedule(16s, [this, player, shenzinsu](TaskContext const&)
                                {
                                    shenzinsu->Talk(Misc::shenzinsu_bunny_text_2, CHAT_MSG_MONSTER_SAY, 500.f, player);
                                    me->PlayDirectSound(27823, player);
                                });

                            scheduler.Schedule(30s, [this, player, shenzinsu](TaskContext const&)
                                {
                                    shenzinsu->Talk(Misc::shenzinsu_bunny_text_3, CHAT_MSG_MONSTER_SAY, 500.f, player);
                                    me->PlayDirectSound(27824, player);
                                });

                            scheduler.Schedule(48s, [this, player, shenzinsu](TaskContext const&)
                                {
                                    shenzinsu->Talk(Misc::shenzinsu_bunny_text_4, CHAT_MSG_MONSTER_SAY, 500.f, player);
                                    me->PlayDirectSound(27825, player);
                                });

                            scheduler.Schedule(63s, [aysa](TaskContext const&)
                                {
                                    aysa->AI()->Talk(5);
                                });

                            scheduler.Schedule(75s, [this, player, shenzinsu](TaskContext const&)
                                {
                                    shenzinsu->Talk(Misc::shenzinsu_bunny_text_5, CHAT_MSG_MONSTER_SAY, 500.f, player);
                                    me->PlayDirectSound(27826, player);
                                });

                            scheduler.Schedule(90s, [aysa](TaskContext const&)
                                {
                                    aysa->AI()->Talk(6);
                                });

                            scheduler.Schedule(100s, [this, player, shenzinsu](TaskContext const&)
                                {
                                    shenzinsu->Talk(Misc::shenzinsu_bunny_text_6, CHAT_MSG_MONSTER_SAY, 500.f, player);
                                    me->PlayDirectSound(27827, player);
                                });
                        }
                        break;
                    }

                    case 5:
                    {
                        me->SetSpeed(MOVE_RUN, 9.0f);
                        me->SetSpeed(MOVE_FLIGHT, 9.0f);
                        break;
                    }

                    case 8:
                    {
                        ji->AI()->Talk(3);

                        scheduler.Schedule(8s, [ji](TaskContext const&)
                            {
                                ji->AI()->Talk(4);
                            });

                        scheduler.Schedule(15s, [aysa](TaskContext const&)
                            {
                                aysa->AI()->Talk(7);
                            });

                        break;
                    }

                    case 11:
                    {
                        ji->AI()->Talk(5);

                        scheduler.Schedule(8s, [aysa](TaskContext const&)
                            {
                                aysa->AI()->Talk(8);
                            });

                        scheduler.Schedule(26s, [ji](TaskContext const&)
                            {
                                ji->AI()->Talk(6);
                            });

                        scheduler.Schedule(37s, [aysa](TaskContext const&)
                            {
                                aysa->AI()->Talk(9);
                            });
                        break;
                    }

                    case 17:
                    {
                        scheduler.Schedule(5s, [aysa](TaskContext const&)
                            {
                                aysa->AI()->Talk(10);
                            });
                        player->KilledMonsterCredit(Npcs::credit_the_suffering_of_shenzinsu_2);
                        break;
                    }
                    default:
                        break;
                    }
                }
            }

            void WaypointPathEnded(uint32 /*nodeId*/, uint32 pathId) override
            {
                if (pathId == 5564900)
                {
                    Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID);
                    if (player)
                    {
                        //player->EnableMirrorTimer(FATIGUE_TIMER);
                        me->CastSpell(player, Spells::spell_eject_passenger_1, true);
                        me->CastSpell(player, Spells::spell_parachute, true);
                    }
                    me->DespawnOrUnsummon(30s);
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
                    case Events::event_aysa_jump_in:
                    {
                        Creature* Aysa = me->FindNearestCreature(Npcs::npc_aysa_q29790, 10.f);
                        if (Aysa)
                        {
                            Aysa->CastSpell(me, Spells::spell_force_vehicle_ride, true);
                            events.ScheduleEvent(Events::event_spawn_balloon, 2s);
                        }
                        break;
                    }

                    case Events::event_spawn_balloon:
                    {
                        Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID);
                        Creature* spawnedBalloon = player->SummonCreature(Npcs::npc_balloon_spawned, me->GetPosition());
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

    // 56159
    class npc_aysa_mandori_village : public CreatureScript
    {
    public:
        npc_aysa_mandori_village() : CreatureScript("npc_aysa_mandori_village") { }

        struct npc_aysaAI : public ScriptedAI
        {
            npc_aysaAI(Creature* c) : ScriptedAI(c) { }

            void Reset() override
            {
                events.Reset();
            }

            void MovementInform(uint32 /*type*/, uint32 pointId) override
            {
                switch (pointId)
                {
                case 1:
                    me->GetMotionMaster()->MovePoint(2, PositionsQ29792::Aysha_point_2);
                    break;
                case 2:
                    me->GetMotionMaster()->MovePoint(3, PositionsQ29792::Aysha_point_3);
                    break;
                case 3:
                    me->GetMotionMaster()->MovePoint(4, PositionsQ29792::Aysha_point_4);
                    break;
                case 4:
                    me->GetMotionMaster()->MovePoint(5, PositionsQ29792::Aysha_point_5);
                    break;
                case 5:
                    events.ScheduleEvent(2, 2s);
                    break;
                case 6:
                {
                    scheduler.Schedule(2s, [this](TaskContext const&)
                        {
                            Talk(1);
                        });

                    scheduler.Schedule(4s, [this](TaskContext const&)
                        {
                            Talk(2);
                        });

                    Creature* ji = me->FindNearestCreature(Npcs::npc_ji_q29792, 20.f);
                    Creature* jojo = me->FindNearestCreature(Npcs::npc_jojo_q29792, 20.f);

                    if (ji && jojo)
                    {
                        scheduler.Schedule(4s, [ji](TaskContext const&)
                            {
                                ji->GetMotionMaster()->MovePoint(7, PositionsQ29792::Ji_point_gate);
                            });

                        scheduler.Schedule(6s, [ji](TaskContext const&)
                            {
                                ji->AI()->Talk(0);
                            });

                        scheduler.Schedule(10s, [ji](TaskContext const&)
                            {
                                ji->AI()->Talk(1);
                            });

                        scheduler.Schedule(12s, [this, ji](TaskContext const&)
                            {
                                me->SetWalk(false);
                                ji->SetWalk(false);
                                me->GetMotionMaster()->MovePoint(8, PositionsQ29792::Aysa_point_gate_back);
                                ji->GetMotionMaster()->MovePoint(9, PositionsQ29792::Ji_point_gate_back);
                            });

                        scheduler.Schedule(14s, [jojo](TaskContext const&)
                            {
                                jojo->GetMotionMaster()->MovePoint(10, PositionsQ29792::Jojo_point_gate);
                            });

                        scheduler.Schedule(17s, [this, ji, jojo](TaskContext const&)
                            {
                                GameObject* door = jojo->FindNearestGameObject(Objects::go_forest_door2, 30.f);
                                if (door)
                                {
                                    jojo->SetOrientation(3.085f);
                                    ji->SetOrientation(3.085f);
                                    me->SetOrientation(3.085f);

                                    jojo->CastSpell(jojo, 129297, true);
                                }
                                
                            });

                        scheduler.Schedule(19s, [this](TaskContext const&)
                            {
                                GameObject* door = me->FindNearestGameObject(Objects::go_forest_door2, 30.f);
                                if (door)
                                {
                                    door->SetGoState(GO_STATE_ACTIVE);
                                }
                            });

                        scheduler.Schedule(21s, [this](TaskContext const&)
                            {
                                Player* player = ObjectAccessor::GetPlayer(*me, PlayerGuid);
                                if (player)
                                    player->KilledMonsterCredit(Npcs::credit_forest_door_two);
                            });

                        scheduler.Schedule(23s, [this, ji, jojo](TaskContext const&)
                            {
                                ji->GetMotionMaster()->Clear();
                                jojo->GetMotionMaster()->Clear();
                                me->GetMotionMaster()->MovePoint(11, PositionsQ29792::ForlornHut);
                            });

                        scheduler.Schedule(25s, [this, ji, jojo](TaskContext const&)
                            {
                                ji->GetMotionMaster()->MoveFollow(me, 3.f, M_PI / 4);
                                jojo->GetMotionMaster()->MoveFollow(me, 3.f, -M_PI / 4);
                            });

                        scheduler.Schedule(26s, [ji](TaskContext const&)
                            {
                                ji->AI()->Talk(2);
                            });
                    }
                    break;
                }

                case 11:
                {
                    Creature* ji = me->FindNearestCreature(Npcs::npc_ji_q29792, 20.f);
                    Creature* jojo = me->FindNearestCreature(Npcs::npc_jojo_q29792, 20.f);
                    if (ji)
                        ji->DespawnOrUnsummon(60s);

                    if (jojo)
                        jojo->DespawnOrUnsummon(60s);

                    me->DespawnOrUnsummon(60s);

                    break;
                }
                }
            }

            void SetData(uint32 type, uint32 value) override
            {
                if (type == 1 && value == 1)
                {
                    events.ScheduleEvent(1, 3s);
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
                    case 1:
                    {
                        Creature* ji = me->FindNearestCreature(Npcs::npc_ji_q29792, 20.f);
                        Creature* jojo = me->FindNearestCreature(Npcs::npc_jojo_q29792, 20.f);

                        if (ji && jojo)
                        {
                            ji->GetMotionMaster()->MoveFollow(me, 3.f, M_PI / 4);
                            jojo->GetMotionMaster()->MoveFollow(me, 3.f, -M_PI / 4);
                            me->GetMotionMaster()->MovePoint(1, PositionsQ29792::Aysha_point_1);
                        }
                        break;
                    }
                    case 2:
                    {
                        Creature* ji = me->FindNearestCreature(Npcs::npc_ji_q29792, 20.f);
                        Creature* jojo = me->FindNearestCreature(Npcs::npc_jojo_q29792, 20.f);

                        me->SetWalk(true);
                        if (ji && jojo)
                        {
                            ji->GetMotionMaster()->Clear();
                            ji->SetWalk(true);
                            jojo->GetMotionMaster()->Clear();

                        }
                        me->GetMotionMaster()->MovePoint(6, PositionsQ29792::Aysha_point_gate);
                    }
                    }
                }
            }

            void SetGUID(ObjectGuid const& guid, int32 type) override
            {
                if (type == 1)
                    PlayerGuid = guid;
            }

        private:
            EventMap events;
            TaskScheduler scheduler;
            ObjectGuid PlayerGuid;
        };

        CreatureAI* GetAI(Creature* c) const override
        {
            return new npc_aysaAI(c);
        }
    };

    // 60042 Korga in the forbidden forest
    class npc_korga_hut : public CreatureScript
    {
    public:
        npc_korga_hut() : CreatureScript("npc_korga_hut") { }

        struct npc_korga_hutAI : public ScriptedAI
        {
            npc_korga_hutAI(Creature* creature) : ScriptedAI(creature) { }

            void OnQuestAccept(Player* player, Quest const* quest) override
            {
                if (quest->GetQuestId() == Quests::quest_wrecking_the_wreck)
                    PhasingHandler::OnConditionChange(player, true);
            }

            void SetData(uint32 type, uint32 value) override
            {
                if (type == 1 && value == 1)
                {
                    StartDialogue();
                }
            }

            void SetGUID(ObjectGuid const& guid, int32 type) override
            {
                if (type == 1)
                    PlayerGUID = guid;
            }

            void StartDialogue()
            {
                scheduler.Schedule(5s, [this](TaskContext const&)
                    {
                        Talk(0);
                    });

                Creature* hermit = me->FindNearestCreature(Npcs::npc_wei_palerage, 10.f);
                if (hermit)
                {
                    scheduler.Schedule(10s, [hermit](TaskContext const&)
                        {
                            hermit->AI()->Talk(0);
                        });
                }

                scheduler.Schedule(20s, [this](TaskContext const&)
                    {
                        Talk(1);
                    });

                Creature* ji = me->FindNearestCreature(Npcs::npc_ji_q29792, 20.f);
                if (ji)
                {
                    scheduler.Schedule(30s, [ji](TaskContext const&)
                        {
                            ji->AI()->Talk(3);
                        });
                }

                scheduler.Schedule(40s, [this](TaskContext const&)
                    {
                        Talk(2);
                    });

                if (ji)
                {
                    Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID);
                    if (!player)
                        return;

                    scheduler.Schedule(50s, [ji, player](TaskContext const&)
                        {
                            ji->AI()->Talk(4, player);
                        });
                }
            }

            void UpdateAI(uint32 diff) override
            {
                //events.Update(diff);
                scheduler.Update(diff);
            }

        private:
            TaskScheduler scheduler;
            ObjectGuid PlayerGUID;
        };

        CreatureAI* GetAI(Creature* c) const override
        {
            return new npc_korga_hutAI(c);
        }
    };

    // 55999
    class npc_injured_sailor_55999 : public CreatureScript
    {
    public:
        npc_injured_sailor_55999() : CreatureScript("npc_injured_sailor_55999") { }

        struct npc_injured_sailor_55999AI : public ScriptedAI
        {
            npc_injured_sailor_55999AI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() override
            {
                // Make sure it's clickable
                me->SetNpcFlag(UNIT_NPC_FLAG_SPELLCLICK);

                // Set HP to 5-10%
                uint32 pct = urand(5, 10);
                me->SetHealth(me->CountPctFromMaxHealth(pct));
            }

            void OnSpellClick(Unit* clicker, bool) override
            {
                Player* player = clicker->ToPlayer();
                if (!player)
                    return;

                if (!player->IsActiveQuest(Quests::quest_none_left_behind))
                    return;

                Vehicle* veh = player->GetVehicleKit();
                if (veh)
                {
                    Unit* sailor = veh->GetPassenger(0);
                    if (sailor)
                        return;
                }

                Creature* sailor = player->SummonCreature(Npcs::npc_rescued_sailor, player->GetPosition());
                if (sailor)
                {
                    scheduler.Schedule(400ms, [sailor, player](TaskContext const&)
                        {
                            sailor->CastSpell(player, Spells::spell_force_vehicle_ride, true);
                        });
                }
                me->DespawnOrUnsummon(500ms);
            }

            void UpdateAI(uint32 diff) override
            {
                scheduler.Update(diff);
            }

        private:
            TaskScheduler scheduler;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_injured_sailor_55999AI(creature);
        }
    };

    // 56416
    class npc_aysa_vordraka_fight : public CreatureScript
    {
    public:
        npc_aysa_vordraka_fight() : CreatureScript("npc_aysa_vordraka_fight") { }

        struct npc_aysa_vordraka_fightAI : public ScriptedAI
        {
            npc_aysa_vordraka_fightAI(Creature* creature) : ScriptedAI(creature) {}

            void Reset() override
            {
                me->SetReactState(REACT_DEFENSIVE);
            }

            void MoveInLineOfSight(Unit* who) override
            {
                if (who->IsPlayer() && who->ToPlayer()->IsActiveQuest(Quests::quest_an_ancient_evil))
                    if (Creature* creature = me->FindNearestCreature(Npcs::npc_vordraka, me->GetVisibilityRange(), true))
                    {
                        me->SetReactState(REACT_AGGRESSIVE);
                        creature->SetReactState(REACT_AGGRESSIVE);
                        AttackStart(creature);
                    }
            }

            void JustEnteredCombat(Unit* who) override
            {
                me->ReplaceAllNpcFlags(UNIT_NPC_FLAG_NONE);
                if(!who->IsPlayer())
                    who->GetAI()->AttackStart(me);
                events.ScheduleEvent(Events::event_aysa_cast_tempered_fury, 2s, 4s);
            }

            void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
            {
                if (HealthAbovePct(85))
                    damage = urand(1, 2);
                else
                    me->SetHealth(me->GetMaxHealth() * 0.85f);
            }

            void SpellHit(WorldObject* /*caster*/, SpellInfo const* spellInfo) override
            {
                if (spellInfo->Id == Spells::spell_vordraka_deep_sea_smash)
                {
                    events.CancelEvent(Events::event_aysa_cast_tempered_fury);
                    events.ScheduleEvent(Events::event_aysa_cast_combat_roll, 1s);
                }
            }

            void SetData(uint32 id, uint32 /*value*/) override
            {
                switch (id)
                {
                case Data::data_aysa_vordraka_fight_talk_1:
                    Talk(Talks::aysa_vordraka_boss_0);
                    break;
                case Data::data_aysa_vordraka_fight_talk_2:
                    Talk(Talks::aysa_vordraka_boss_1);
                    break;
                case Data::data_vordraka_summon_aggressors:
                    Talk(Talks::aysa_vordraka_boss_2);
                    break;
                case Data::data_vordraka_death:
                    Talk(Talks::aysa_vordraka_boss_4);
                    me->SetNpcFlag(UNIT_NPC_FLAG_QUESTGIVER);
                    EnterEvadeMode(EvadeReason::Other);
                    events.CancelEvent(Events::event_aysa_cast_tempered_fury);
                    break;
                }
            }

            void JustReachedHome() override
            {
                events.ScheduleEvent(Events::event_aysa_update_phases, 5s);
            }

            void OnQuestAccept(Player* player, Quest const* quest) override
            {
                if (quest->GetQuestId() == Quests::quest_risking_it_all)
                {
                    // remove this so player no longer sees Jojo at camp
                    player->RemoveAurasDueToSpell(Spells::spell_invisibility_detection_quest_5);

                    DoCast(player, Spells::spell_forcecast_summon_aysa_after_fight, true);
                }
            }

            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case Events::event_aysa_cast_tempered_fury:
                        DoCastVictim(Spells::spell_aysa_tempered_fury);
                        events.ScheduleEvent(Events::event_aysa_cast_tempered_fury, 2s, 4s);
                        break;
                    case Events::event_aysa_cast_combat_roll:
                        DoCastVictim(Spells::spell_aysa_combat_roll);
                        events.ScheduleEvent(Events::event_aysa_cast_tempered_fury, 5s, 7s);
                        break;
                    case Events::event_aysa_update_phases:
                        std::list<Player*> players;
                        me->GetPlayerListInGrid(players, me->GetVisibilityRange());
                        for (std::list<Player*>::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                        {
                            if ((*itr)->ToPlayer()->GetQuestStatus(Quests::quest_an_ancient_evil) == QUEST_STATUS_COMPLETE)
                                PhasingHandler::OnConditionChange(*itr, true); // update phases based on conditions
                        }
                        break;
                    }
                }

                me->DoMeleeAttackIfReady();
            }

        private:
            EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_aysa_vordraka_fightAI(creature);
        }
    };

    class npc_vordraka : public CreatureScript
    {
    public:
        npc_vordraka() : CreatureScript("npc_vordraka") { }

        struct npc_vordrakaAI : public ScriptedAI
        {
            npc_vordrakaAI(Creature* creature) : ScriptedAI(creature) {
                Initialize();
            }

            void Initialize()
            {
                _playerParticipating = false;
                _secondSmash = false;
                _smashAnnounced = false;
                _aggressorSummoned = 0;
            }

            void Reset() override
            {
                events.Reset();
                Initialize();
                me->setActive(true);
                me->SetReactState(REACT_PASSIVE);
            }

            void JustEnteredCombat(Unit* /*who*/) override
            {
                events.ScheduleEvent(Events::event_vordraka_cast_smash, 8s);
                events.ScheduleEvent(Events::event_vordraka_cast_rupture, 12s, 16s);
            }

            void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
            {
                if (HealthAbovePct(85) && attacker->IsCreature())
                    if (attacker->GetEntry() == Npcs::npc_aysa_vordraka_fight)
                        damage = urand(1, 2);

                if (HealthBelowPct(85) && attacker->IsCreature())
                    if (attacker->GetEntry() == Npcs::npc_aysa_vordraka_fight)
                        me->SetHealth(me->GetHealth() + damage);

                if (!_playerParticipating && attacker->ToPlayer())
                {
                    if (Creature* creature = me->FindNearestCreature(Npcs::npc_aysa_vordraka_fight, me->GetVisibilityRange(), true))
                    {
                        creature->AI()->SetData(Data::data_aysa_vordraka_fight_talk_1, Data::data_aysa_vordraka_fight_talk_1);
                        _playerParticipating = true;
                    }
                }

                if (damage >= me->GetHealth())
                {
                    auto const& threatMap = me->GetThreatManager().GetThreatenedByMeList();

                    for (auto const& pair : threatMap)
                    {
                        ThreatReference* ref = pair.second;
                        if (!ref)
                            continue;

                        Unit* unit = ref->GetVictim();
                        if (!unit)
                            continue;

                        if (Player* player = unit->ToPlayer())
                        {
                            if (player->GetQuestStatus(Quests::quest_an_ancient_evil) == QUEST_STATUS_INCOMPLETE)
                                player->KilledMonsterCredit(me->GetEntry());
                        }
                    }
                }
            }

            void JustDied(Unit* /*killer*/) override
            {
                std::list<Creature*> summonedAggressors;
                me->GetCreatureListWithEntryInGrid(summonedAggressors, Npcs::npc_deepscale_aggressor, me->GetVisibilityRange());
                for (std::list<Creature*>::const_iterator itr = summonedAggressors.begin(); itr != summonedAggressors.end(); ++itr)
                    (*itr)->ToCreature()->DespawnOrUnsummon();

                if (Creature* creature = me->FindNearestCreature(Npcs::npc_aysa_vordraka_fight, me->GetVisibilityRange(), true))
                    creature->AI()->SetData(Data::data_vordraka_death, Data::data_vordraka_death);

                std::list<Player*> playersVisibility;
                me->GetPlayerListInGrid(playersVisibility, me->GetVisibilityRange());
                for (std::list<Player*>::const_iterator itr = playersVisibility.begin(); itr != playersVisibility.end(); ++itr)
                    (*itr)->CastSpell((*itr), Spells::spell_vordraka_see_death_invis, true);

                DoCastSelf(Spells::spell_vordraka_death_invis, true);
                me->DespawnOrUnsummon(30s);
            }

            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);

                if (!UpdateVictim())
                    return;

                if (_aggressorSummoned == 0 && HealthBelowPct(60))
                {
                    events.ScheduleEvent(Events::event_vordraka_cast_summon, 0s);
                    events.ScheduleEvent(Events::event_vordraka_cast_summon, 0s);
                }

                if (_aggressorSummoned == 2 && HealthBelowPct(30))
                {
                    events.ScheduleEvent(Events::event_vordraka_cast_summon, 0s);
                    events.ScheduleEvent(Events::event_vordraka_cast_summon, 1s, 5s);
                }

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case Events::event_vordraka_cast_smash:
                        DoCastVictim(Spells::spell_vordraka_deep_sea_smash);
                        events.ScheduleEvent(Events::event_vordraka_cast_smash, 12s);
                        if (_playerParticipating && !_smashAnnounced)
                        {
                            if (_secondSmash)
                            {
                                if (Creature* creature = me->FindNearestCreature(Npcs::npc_aysa_vordraka_fight, me->GetVisibilityRange(), true))
                                {
                                    creature->AI()->SetData(Data::data_aysa_vordraka_fight_talk_2, Data::data_aysa_vordraka_fight_talk_2);
                                    _smashAnnounced = true;
                                }
                            }
                            _secondSmash = true;
                        }
                        break;
                    case Events::event_vordraka_cast_rupture:
                        DoCast(SelectTarget(SelectTargetMethod::Random, 1), Spells::spell_vordraka_rupture);
                        events.ScheduleEvent(Events::event_vordraka_cast_rupture, 12s, 16s);
                        break;
                    case Events::event_vordraka_cast_summon:
                        DoCast(me, Spells::spell_vordraka_summon_aggressor, true);
                        if (_aggressorSummoned == 0)
                            if (Creature* creature = me->FindNearestCreature(Npcs::npc_aysa_vordraka_fight, me->GetVisibilityRange(), true))
                                creature->AI()->SetData(Data::data_vordraka_summon_aggressors, Data::data_vordraka_summon_aggressors);
                        _aggressorSummoned++;
                        break;
                    }
                }

                me->DoMeleeAttackIfReady();
            }

        private:
            EventMap events;
            bool _playerParticipating;
            bool _secondSmash;
            bool _smashAnnounced;
            uint8 _aggressorSummoned;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_vordrakaAI(creature);
        }
    };

    // 60729
    class npc_aysa_explosion : public CreatureScript
    {
    public:
        npc_aysa_explosion() : CreatureScript("npc_aysa_explosion") { }

        struct npc_aysa_explosionAI : public ScriptedAI
        {
            npc_aysa_explosionAI(Creature* creature) : ScriptedAI(creature) {}

            void Reset() override
            {
                me->SetReactState(REACT_DEFENSIVE);
                events.ScheduleEvent(Events::event_aysa_explosion_intro_talk, 1s);
            }

            void IsSummonedBy(WorldObject* summoner) override
            {
                if (summoner->IsPlayer())
                    PlayerGUID = summoner->ToPlayer()->GetGUID();
            }

            void WaypointPathEnded(uint32 /*nodeId*/, uint32 pathId) override
            {
                if (pathId == 6072900)
                {
                    StartDialogue();
                }
            }

            void WaypointReached(uint32 nodeId, uint32 /*pathId*/) override
            {
                switch (nodeId)
                {
                case 7:
                {
                    Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID);
                    if (player)
                        player->CastSpell(player, Spells::spell_summon_ji_after_vordraka_fight, true);
                    break;
                }
                default:
                    break;
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
                    case Events::event_aysa_explosion_intro_talk:
                    {
                        Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID);
                        if(player)
                            Talk(0, player);

                        me->GetMotionMaster()->MovePath(6072900, false);
                        break;
                    }

                    case Events::event_aysa_explosion_leave_deck:
                    {
                        me->SetWalk(false);
                        me->GetMotionMaster()->MovePoint(1, Positions::AysaDeckMove);
                        Creature* ji = me->FindNearestCreatureWithOptions(20.f, { .CreatureId = Npcs::npc_ji_after_vordraka_fight , .IgnorePhases = true });
                        if (ji)
                        {
                            ji->SetWalk(false);
                            ji->GetMotionMaster()->MovePoint(1, Positions::JiDeckMove);

                            scheduler.Schedule(3s, [this, ji](TaskContext const&)
                                {
                                    me->GetMotionMaster()->MoveJump(EVENT_JUMP, Positions::AysaDeckJump1);
                                    ji->GetMotionMaster()->MoveJump(EVENT_JUMP, Positions::JiDeckJump1);
                                });

                            scheduler.Schedule(5s, [this, ji](TaskContext const&)
                                {
                                    me->GetMotionMaster()->MoveJump(EVENT_JUMP, Positions::AysaDeckJump2);
                                    ji->GetMotionMaster()->MoveJump(EVENT_JUMP, Positions::JiDeckJump2);
                                });

                            Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID);
                            if (player)
                            {
                                scheduler.Schedule(15s, [player](TaskContext const&)
                                    {
                                        player->CastSpell(player, Spells::spell_trigger_blackout, true);
                                    });

                                scheduler.Schedule(17s, [this, ji, player](TaskContext const&)
                                    {
                                        player->SendMovieStart(117);
                                        player->CastSpell(player, 117615, true);
                                        PhasingHandler::OnConditionChange(player, true);
                                        PhasingHandler::AddVisibleMapId(player, 975);
                                        me->DespawnOrUnsummon();
                                        ji->DespawnOrUnsummon();
                                    });
                            }
                        }

                        break;
                    }
                    }
                }
            }

            void StartDialogue()
            {
                Creature* ji = me->FindNearestCreatureWithOptions(20.f, { .CreatureId = Npcs::npc_ji_after_vordraka_fight , .IgnorePhases = true });
                Talk(1);

                if (ji)
                {
                    scheduler.Schedule(5s, [this, ji](TaskContext const&)
                        {
                            ji->SetFacingToObject(me, true);
                            ji->AI()->Talk(0);
                        });

                    scheduler.Schedule(15s, [this](TaskContext const&)
                        {
                            Talk(2);
                        });

                    scheduler.Schedule(21s, [ji](TaskContext const&)
                        {
                            ji->AI()->Talk(1);
                        });

                    scheduler.Schedule(31s, [this](TaskContext const&)
                        {
                            Talk(3);
                        });

                    scheduler.Schedule(38s, [ji](TaskContext const&)
                        {
                            ji->AI()->Talk(2);
                        });

                    scheduler.Schedule(48s, [this](TaskContext const&)
                        {
                            Talk(4);
                        });

                    scheduler.Schedule(57s, [this](TaskContext const&)
                        {
                            Talk(5);

                            events.ScheduleEvent(Events::event_aysa_explosion_leave_deck, 6s);
                        });
                }
            }

        private:
            EventMap events;
            TaskScheduler scheduler;
            ObjectGuid PlayerGUID;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_aysa_explosionAI(creature);
        }
    };

    class npc_healers_active_bunny : public CreatureScript
    {
    public:
        npc_healers_active_bunny() : CreatureScript("npc_healers_active_bunny") { }

        struct npc_healers_active_bunnyAI : public ScriptedAI
        {
            npc_healers_active_bunnyAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() override
            {
                events.Reset();
                _healersActive = 0;
                events.ScheduleEvent(Events::event_healing_bunny_spawn_adds, 5s);
                events.ScheduleEvent(Events::event_healing_bunny_ji_yell, 45s);
                events.ScheduleEvent(Events::event_healing_bunny_korga_combat, 5s);
                events.ScheduleEvent(Events::event_healing_bunny_jojo_combat, 5s);
                events.ScheduleEvent(Events::event_healing_bunny_delora_combat, 5s);
                events.ScheduleEvent(Events::event_shenzinsu_pain_shake, 45s);
            }

            uint32 updateTimer = 2000; // update every 2 seconds

            void UpdateAI(uint32 diff) override
            {
                if (updateTimer <= diff)
                {
                    Map* map = me->GetMap();
                    if (!map)
                        return;

                    _healersActive = 0;

                    std::list<Creature*> list;
                    me->GetCreatureListWithEntryInGrid(list, Npcs::npc_healer_alliance_priest, 200.0f);
                    me->GetCreatureListWithEntryInGrid(list, Npcs::npc_healer_alliance_priest2, 200.0f);
                    me->GetCreatureListWithEntryInGrid(list, Npcs::npc_healer_horde_druid, 200.0f);
                    me->GetCreatureListWithEntryInGrid(list, Npcs::npc_healer_horde_druid2, 200.0f);

                    for (Creature* c : list)
                        if (c->IsAlive() && (c->HasAura(Spells::spell_druid_wound_heal) || c->HasAura(Spells::spell_priest_wound_heal)))
                            ++_healersActive;

                    WorldStateMgr::SetValue(Data::worldstate_healers_active, _healersActive, false, map);
                    updateTimer = 2000;
                    Map::PlayerList const& playerList = me->GetMap()->GetPlayers();
                    if (playerList.empty())
                        return;

                    for (Map::PlayerList::const_iterator itr = playerList.begin(); itr != playerList.end(); ++itr)
                        if (Player* player = itr->GetSource())
                            if (player->GetAreaId() == me->GetAreaId())
                                player->SendUpdateWorldState(Data::worldstate_healers_active, _healersActive);
                }
                else
                    updateTimer -= diff;

                events.Update(diff);
                scheduler.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case Events::event_shenzinsu_pain_shake:
                    {
                        Map::PlayerList const& playerList = me->GetMap()->GetPlayers();
                        if (playerList.empty())
                            return;

                        for (Map::PlayerList::const_iterator itr = playerList.begin(); itr != playerList.end(); ++itr)
                            if (Player* player = itr->GetSource())
                                if (player->GetAreaId() == me->GetAreaId() && player->IsActiveQuest(Quests::quest_the_healing_of_shenzinsu))
                                    player->CastSpell(player, Spells::spell_shenzinsu_pain_shake, true);

                        events.ScheduleEvent(Events::event_shenzinsu_pain_shake, 45s, 60s);
                        break;
                    }

                    case Events::event_healing_bunny_korga_combat:
                    {
                        Creature* korga = me->FindNearestCreature(Npcs::npc_korga_during_healing, 200.f, true);
                        if (korga)
                        {
                            Creature* hordeCrewman = korga->FindNearestCreature(Npcs::npc_horde_crewman, 30.f, true);
                            if (hordeCrewman)
                            {
                                hordeCrewman->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);
                                hordeCrewman->GetMotionMaster()->MoveFollow(korga, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
                            }

                            scheduler.Schedule(5s, [korga](TaskContext const&)
                                {
                                    Unit* target = korga->SelectNearestTarget(20.f);
                                    if (target)
                                        korga->EngageWithTarget(target);
                                });

                            if (roll_chance(10))
                            {
                                korga->AI()->Talk(0);
                            }
                        }

                        events.ScheduleEvent(Events::event_healing_bunny_korga_combat, 35s, 45s);
                        break;
                    }

                    case Events::event_healing_bunny_delora_combat:
                    {
                        Creature* delora = me->FindNearestCreature(Npcs::npc_delora_during_healing, 200.f, true);
                        if (delora)
                        {
                            Creature* deloraCrewman = delora->FindNearestCreature(Npcs::npc_delora_crewman, 30.f, true);
                            if (deloraCrewman)
                                deloraCrewman->GetMotionMaster()->MoveFollow(delora, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);

                            scheduler.Schedule(5s, [delora](TaskContext const&)
                                {
                                    Unit* target = delora->SelectNearestTarget(20.f);
                                    if (target)
                                        delora->EngageWithTarget(target);
                                });

                            if (roll_chance(10))
                            {
                                delora->AI()->Talk(0);
                            }
                        }

                        events.ScheduleEvent(Events::event_healing_bunny_delora_combat, 35s, 45s);
                        break;
                    }

                    case Events::event_healing_bunny_jojo_combat:
                    {
                        Creature* jojo = me->FindNearestCreature(Npcs::npc_jojo_during_healing, 200.f, true);
                        if (jojo)
                        {
                            scheduler.Schedule(5s, [jojo](TaskContext const&)
                                {
                                    Unit* target = jojo->SelectNearestTarget(20.f);
                                    if (target)
                                        jojo->EngageWithTarget(target);
                                });
                        }

                        events.ScheduleEvent(Events::event_healing_bunny_jojo_combat, 35s, 45s);
                        break;
                    }

                    case Events::event_healing_bunny_ji_yell:
                    {
                        Creature* ji = me->FindNearestCreature(Npcs::npc_ji_during_healing, 100.f);
                        if (ji)
                        {
                            ji->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);
                            ji->AI()->Talk(0);
                        }

                        events.ScheduleEvent(Events::event_healing_bunny_ji_yell, 45s, 60s);
                        break;
                    }

                    case Events::event_healing_bunny_spawn_adds:
                    {
                        Creature* spawnAdd1 = ObjectAccessor::GetCreature(*me, add1);
                        Creature* spawnAdd2 = ObjectAccessor::GetCreature(*me, add2);
                        Creature* spawnAdd3 = ObjectAccessor::GetCreature(*me, add3);
                        if (!spawnAdd1 || (spawnAdd1 && !spawnAdd1->IsAlive()))
                        {
                            Creature* newAdd1 = me->SummonCreature(RAND(Npcs::npc_spawned_deepscale_ravager, Npcs::npc_spawned_deepscale_fleshripper),
                                Positions::Ravager_Spawn_1, TEMPSUMMON_DEAD_DESPAWN);

                            add1 = newAdd1->GetGUID();
                            newAdd1->SetReactState(REACT_AGGRESSIVE);
                            newAdd1->GetMotionMaster()->MoveJump(EVENT_JUMP, Positions::Ravager_Jump_1);
                        }

                        if (!spawnAdd2 || (spawnAdd2 && !spawnAdd2->IsAlive()))
                        {
                            Creature* newAdd2 = me->SummonCreature(RAND(Npcs::npc_spawned_deepscale_ravager, Npcs::npc_spawned_deepscale_fleshripper),
                                Positions::Ravager_Spawn_2, TEMPSUMMON_DEAD_DESPAWN);

                            add2 = newAdd2->GetGUID();
                            newAdd2->SetReactState(REACT_AGGRESSIVE);
                            newAdd2->GetMotionMaster()->MoveJump(EVENT_JUMP, Positions::Ravager_Jump_2);
                        }

                        if (!spawnAdd3 || (spawnAdd3 && !spawnAdd3->IsAlive()))
                        {
                            Creature* newAdd3 = me->SummonCreature(RAND(Npcs::npc_spawned_deepscale_ravager, Npcs::npc_spawned_deepscale_fleshripper),
                                Positions::Ravager_Spawn_3, TEMPSUMMON_DEAD_DESPAWN);

                            add3 = newAdd3->GetGUID();
                            newAdd3->SetReactState(REACT_AGGRESSIVE);
                            newAdd3->GetMotionMaster()->MoveJump(EVENT_JUMP, Positions::Ravager_Jump_3);
                        }

                        events.ScheduleEvent(Events::event_healing_bunny_spawn_adds, 15s);
                        break;
                    }
                    }
                }
            }

        private:
            EventMap events;
            TaskScheduler scheduler;
            uint8 _healersActive = 0;
            ObjectGuid add1;
            ObjectGuid add2;
            ObjectGuid add3;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_healers_active_bunnyAI(creature);
        }
    };

    enum HealerState
    {
        STATE_IDLE,
        STATE_MOVING_TO_WOUND,
        STATE_HEALING,
        STATE_COMBAT
    };

    class npc_shenzin_su_healer : public CreatureScript
    {
    public:
        npc_shenzin_su_healer() : CreatureScript("npc_shenzin_su_healer") { }

        struct npc_shenzin_su_healerAI : public ScriptedAI
        {
            npc_shenzin_su_healerAI(Creature* creature) : ScriptedAI(creature) { }

            HealerState state = STATE_IDLE;
            uint32 checkTimer = 1000;
            uint32 combatTimer = 2000;
            uint32 healSpell = 0;
            uint32 combatSpell1 = 0;
            uint32 combatSpell2 = 0;

            void Reset() override
            {
                state = STATE_IDLE;
                InitSpells();
                me->SetReactState(REACT_AGGRESSIVE);
                me->SetNpcFlag(UNIT_NPC_FLAG_SPELLCLICK);
            }

            void InitSpells()
            {
                switch (me->GetEntry())
                {
                case Npcs::npc_healer_alliance_priest:
                case Npcs::npc_healer_alliance_priest2:
                    healSpell = Spells::spell_priest_wound_heal;
                    combatSpell1 = Spells::spell_alliance_priest_smite;
                    combatSpell2 = Spells::spell_alliance_priest_greater_heal;
                    break;

                case Npcs::npc_healer_horde_druid:
                case Npcs::npc_healer_horde_druid2:
                    healSpell = Spells::spell_druid_wound_heal;
                    combatSpell1 = Spells::spell_horde_druid_wrath;
                    combatSpell2 = Spells::spell_horde_druid_healing_touch;
                    break;
                }
            }

            // -------------------------
            // SPELL CLICK ? START EVENT
            // -------------------------
            void OnSpellClick(Unit* clicker, bool) override
            {
                Player* player = clicker->ToPlayer();
                if (!player)
                    return;

                QuestStatus qStatus = player->GetQuestStatus(Quests::quest_the_healing_of_shenzinsu);

                if (qStatus != QUEST_STATUS_INCOMPLETE)
                    return;

                if (state != STATE_IDLE)
                    return;

                Talk(0, player); // healer line 0
                me->RemoveNpcFlag(UNIT_NPC_FLAG_SPELLCLICK);
                MoveToRandomWoundSpot();
            }

            void MoveToRandomWoundSpot()
            {
                state = STATE_MOVING_TO_WOUND;
                me->GetMotionMaster()->MovePoint(1, GetRandomWoundPosition());
            }

            Position GetRandomWoundPosition()
            {
                uint8 randomSlot = urand(0, 4);

                return me->GetRandomPoint(Positions::ShenzinsuHealerSpots[randomSlot], 3.f);
            }

            void MovementInform(uint32 type, uint32 id) override
            {
                if (type == POINT_MOTION_TYPE && id == 1)
                    StartHealing();
            }

            // -------------------------
            // HEALING LOOP
            // -------------------------
            void StartHealing()
            {
                state = STATE_HEALING;
                me->SetReactState(REACT_PASSIVE);
                me->CastSpell(me, healSpell, false);
            }

            void StopHealing()
            {
                me->InterruptNonMeleeSpells(true);
                me->RemoveAurasDueToSpell(healSpell);
            }

            // -------------------------
            // COMBAT LOOP
            // -------------------------
            void JustEnteredCombat(Unit* /*attacker*/) override
            {
                if(state == STATE_HEALING)
                    StopHealing();

                state = STATE_COMBAT;
                me->SetReactState(REACT_AGGRESSIVE);
            }

            void JustExitedCombat() override
            {
                // Resume healing
                if(!me->HasNpcFlag(UNIT_NPC_FLAG_SPELLCLICK))
                    MoveToRandomWoundSpot();
            }

            void UpdateAI(uint32 diff) override
            {
                // Healing state: ensure channel stays active
                if (state == STATE_HEALING)
                {
                    if (checkTimer <= diff)
                    {
                        if (!me->HasAura(healSpell))
                            me->CastSpell(me, healSpell, false);

                        checkTimer = 1000;
                    }
                    else
                        checkTimer -= diff;

                    return;
                }

                // Combat state: cast combat spells
                if (state == STATE_COMBAT)
                {
                    if (!UpdateVictim())
                        return;

                    if (combatTimer <= diff)
                    {
                        if(me->GetHealthPct() <= 35)
                            me->CastSpell(me, combatSpell2, false);
                        
                        else
                            me->CastSpell(me->GetVictim(), combatSpell1, false);

                        combatTimer = 2000;
                    }
                    else
                        combatTimer -= diff;

                    me->DoMeleeAttackIfReady();
                    return;
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_shenzin_su_healerAI(creature);
        }
    };

    // 56013 - Spirit of Master Shang Xi
    class npc_spirit_of_master_shang_xi_q31450 : public CreatureScript
    {
    public:
        npc_spirit_of_master_shang_xi_q31450() : CreatureScript("npc_spirit_of_master_shang_xi_q31450") {}

        struct npc_spirit_of_master_shang_xi_q31450AI : public ScriptedAI
        {
            npc_spirit_of_master_shang_xi_q31450AI(Creature* creature) : ScriptedAI(creature) { }

            void OnQuestAccept(Player* player, Quest const* quest) override
            {
                // Only handle quest 31450 - A New Fate
                if (quest->GetQuestId() != Quests::quest_a_new_fate)
                    return;

                // Start conversation sequence after 3 seconds
                me->m_Events.AddEventAtOffset([this, player]()
                    {
                        if (me && me->IsAlive())
                        {
                            // Spirit of Master Shang Xi speaks first (line 0)
                            Talk(0, player);

                            // Spirit of Master Shang Xi second line (line 1) after 4 seconds
                            me->m_Events.AddEventAtOffset([this, player]()
                                {
                                    if (me && me->IsAlive())
                                        Talk(1, player);
                                }, std::chrono::seconds(9));

                            // Aysa responds after 8 seconds
                            Creature* aysa = player->FindNearestCreature(Npcs::npc_aysa_cloudsinger_q31450, 100.0f);
                            if (aysa && aysa->IsAlive())
                            {
                                aysa->m_Events.AddEventAtOffset([aysa, this]()
                                    {
                                        aysa->AI()->Talk(0, me);
                                    }, std::chrono::seconds(20));
                            }

                            // Spirit of Master Shang Xi third line (line 2) after 8 seconds
                            me->m_Events.AddEventAtOffset([this, player]()
                                {
                                    if (me && me->IsAlive())
                                        Talk(2, player);
                                }, std::chrono::seconds(28));

                            // Ji responds after 8 seconds
                            Creature* ji = player->FindNearestCreature(Npcs::npc_ji_firepaw_q31450, 100.0f);
                            if (ji && ji->IsAlive())
                            {
                                ji->m_Events.AddEventAtOffset([ji, this]()
                                    {
                                        ji->AI()->Talk(0, me);
                                    }, std::chrono::seconds(35));
                            }

                            // Spirit of Master Shang Xi final line (line 3) after 20 seconds
                            me->m_Events.AddEventAtOffset([this, player]()
                                {
                                    if (me && me->IsAlive())
                                    {
                                        Talk(3, player);
                                    }
                                }, std::chrono::seconds(45));
                        }
                    }, std::chrono::milliseconds(3000));
            }

            bool OnGossipSelect(Player* player, uint32 /*menuId*/, uint32 gossipListId) override
            {
                if (gossipListId == 0) // Faction choice option
                {
                    player->PlayerTalkClass->ClearMenus();
                    player->PlayerTalkClass->SendCloseGossip();

                    // Cast pandaren faction choice spell
                    player->CastSpell(player, Spells::spell_pandaren_faction_choice, true);
                    return true;
                }
                return false;
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_spirit_of_master_shang_xi_q31450AI(creature);
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
    new npc_ruk_ruk();
    new npc_ruk_ruk_rocket();
    new npc_aysa_outside_chambers_of_whispers();
    new npc_zhaoren();
    new npc_firework_launcher();
    new npc_master_shang_q29787();
    new npc_shang_xi_hot_air_balloon();
    new npc_aysa_mandori_village();
    new npc_korga_hut();
    new npc_injured_sailor_55999();
    new npc_aysa_vordraka_fight();
    new npc_vordraka();
    new npc_aysa_explosion();
    new npc_healers_active_bunny();
    new npc_shenzin_su_healer();
    new npc_spirit_of_master_shang_xi_q31450();
}
