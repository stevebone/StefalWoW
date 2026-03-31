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
#include "EventMap.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "TemporarySummon.h"
#include "Unit.h"


#include "Custom_TheWanderingIsle_Defines.h"

namespace Scripts::TheWanderingIsle::Npcs
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

                _events.RescheduleEvent(Defines::EventsQ29423::event_delivery_huo, 20s);
            }
        }

        void MovementInform(uint32 type, uint32 pointId) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid);

            if (pointId == 1)
            {
                me->CastSpell(player, Defines::SpellsQ29423::spell_fire_form);

                _events.RescheduleEvent(Defines::EventsQ29423::event_second_huo_position, 7s);
            }
            else if (pointId == 2)
            {
                me->CastSpell(player, Defines::SpellsQ29423::spell_forcecast_fire_turn_in_statue_brazier_change);
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
                case Defines::EventsQ29423::event_delivery_huo:
                {
                    Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid);

                    if (player->GetDistance(973.311829f, 3603.419434f, 195.528030f) <= 7.0f)
                    {
                        me->GetMotionMaster()->Remove(FOLLOW_MOTION_TYPE);
                        me->SetFloating(true);
                        me->SetSpeed(MOVE_RUN, 3.0f);
                        me->GetMotionMaster()->MovePoint(1, Defines::PositionsQ29423::huoFirstPoint, true, { 6.249388f });
                    }
                    else
                        _events.RescheduleEvent(Defines::EventsQ29423::event_delivery_huo, 2s);
                    break;
                }
                case Defines::EventsQ29423::event_second_huo_position:
                {
                    me->GetMotionMaster()->MovePoint(2, Defines::PositionsQ29423::huoSecondPoint);
                }
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
            if (quest->GetQuestId() == Defines::Quests::quest_the_passion_of_shen_zin_su)
            {
                Creature* ji = player->FindNearestCreatureWithOptions(15.0f, { .CreatureId = Defines::Npcs::npc_ji_q29423, .IgnorePhases = true });
                Creature* aysa = player->FindNearestCreatureWithOptions(15.0f, { .CreatureId = Defines::Npcs::npc_aysa_q29423, .IgnorePhases = true });

                if (!ji || !aysa)
                    return;

                Talk(Defines::TalksQ29423::shanxi_talk_1, player);

                _scheduler.Schedule(10s, [this](TaskContext /*task*/)
                    {
                        Talk(Defines::TalksQ29423::shanxi_talk_2);
                    });
                _scheduler.Schedule(26s, [this](TaskContext /*task*/)
                    {
                        Talk(Defines::TalksQ29423::shanxi_talk_3);
                    });
                _scheduler.Schedule(40s, [this](TaskContext /*task*/)
                    {
                        Talk(Defines::TalksQ29423::shanxi_talk_4);
                    });
                _scheduler.Schedule(49s, [ji](TaskContext /*task*/)
                    {
                        ji->AI()->Talk(Defines::TalksQ29423::ji_talk);
                        ji->GetMotionMaster()->MovePath(Defines::PathQ29423::ji_away, false);
                        ji->DespawnOrUnsummon(20s);
                    });
                _scheduler.Schedule(52s, [this](TaskContext /*task*/)
                    {
                        Talk(Defines::TalksQ29423::shanxi_talk_5);
                    });
                _scheduler.Schedule(60s, [aysa](TaskContext /*task*/)
                    {
                        aysa->AI()->Talk(Defines::TalksQ29423::aysa_talk);
                        aysa->GetMotionMaster()->MovePath(Defines::PathQ29423::aysa_away, false);
                        aysa->DespawnOrUnsummon(20s);
                    });
                _scheduler.Schedule(63s, [this, player](TaskContext /*task*/)
                    {
                        Talk(Defines::TalksQ29423::shanxi_talk_6, player);
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
            if (spellInfo->Id != Defines::SpellsQ29423::spell_start_talk_event)
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

                me->AI()->Talk(Defines::TalksQ29423::chia_hui_autumnleaf_talk, player);

                _scheduler.Schedule(2s, [this, lin, zhen, player](TaskContext /*task*/)
                    {
                        me->SetFacingToObject(player);
                        lin->SetFacingToObject(player);
                        zhen->SetFacingToObject(player);

                        lin->AI()->Talk(Defines::TalksQ29423::brewer_lin_talk);
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
                    me->AI()->Talk(Defines::TalksQ29521::CaiTalk0);
                });
            _scheduler.Schedule(21s, [this](TaskContext /*task*/)
                {
                    me->AI()->Talk(Defines::TalksQ29521::CaiTalk1);
                });
            _scheduler.Schedule(37s, [this](TaskContext /*task*/)
                {
                    me->AI()->Talk(Defines::TalksQ29521::CaiTalk2);
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
                    Talk(Defines::TalksQ29521::DengTalk0);
                });
            _scheduler.Schedule(29s, [this](TaskContext /*task*/)
                {
                    Talk(Defines::TalksQ29521::DengTalk1);
                });
            _scheduler.Schedule(45s, [this](TaskContext /*task*/)
                {
                    TempSummon* summon = me->ToTempSummon();
                    if (!summon)
                        return;

                    Unit* summoner = summon->GetSummonerUnit();
                    if (!summoner)
                        return;

                    Talk(Defines::TalksQ29521::DengTalk2);
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
}

void AddSC_custom_the_wandering_isle_npcs()
{
    using namespace Scripts::TheWanderingIsle::Npcs;
    RegisterCreatureAI(npc_huo_follower);
    RegisterCreatureAI(npc_chia_hui_autumnleaf);
    RegisterCreatureAI(npc_shanxi_quest);
    RegisterCreatureAI(npc_deng);
    RegisterCreatureAI(npc_cai);
}
