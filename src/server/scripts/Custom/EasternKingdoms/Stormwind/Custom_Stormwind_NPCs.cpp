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
#include "CreatureAIImpl.h"
#include "EventMap.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "TaskScheduler.h"

#include "Custom_Stormwind_Defines.h"

namespace Scripts::EasternKingdoms::StormwindCity
{
    // 61796 - King Varian (Spawned for Quest: The Alliance Way)
    struct npc_king_varian_61796 : public ScriptedAI
    {
        npc_king_varian_61796(Creature* creature) : ScriptedAI(creature) {}

        void Reset() override
        {
            _events.Reset();
            _events.ScheduleEvent(Events::KingVarianSpawnedStartPath, 2s);
            _events.ScheduleEvent(Events::KingVarianSpawnedInitialTalk, 5s);
        }

        void SetGUID(ObjectGuid const& guid, int32 id) override
        {
            if (id == 1)
                _playerGUID = guid;
        }

        void WaypointReached(uint32 nodeId, uint32 /*pathId*/) override
        {
            Player* player = ObjectAccessor::GetPlayer(*me, _playerGUID);
            if (!player)
            {
                me->Say("No player found", LANG_UNIVERSAL);
                return;
            }

            if (nodeId == 2)
            {
                Talk(1);/*
                me->m_Events.AddEventAtOffset([this]()
                    {
                        if (me && me->IsAlive())
                            Talk(1);
                    }, std::chrono::seconds(3)); */
            }

            if (nodeId == 3)
            {
                me->m_Events.AddEventAtOffset([this]()
                    {
                        if (me && me->IsAlive())
                            Talk(2);
                    }, std::chrono::seconds(5));
            }

            if (nodeId == 5)
            {
                Talk(3);

                Creature* aysa = player->FindNearestCreature(Creatures::AysaSpawnedAtGate, 30.f);
                if (aysa && aysa->IsAlive())
                {
                    aysa->m_Events.AddEventAtOffset([aysa]()
                        {
                            if (aysa && aysa->IsAlive())
                                aysa->AI()->Talk(5);
                        }, std::chrono::seconds(6));
                }

                me->m_Events.AddEventAtOffset([this]()
                    {
                        if (me && me->IsAlive())
                            Talk(4);
                    }, std::chrono::seconds(9));
            }

            //if (nodeId == 6)
            //    Talk(4);

            if (nodeId == 7)
                Talk(5);

            if (nodeId == 9)
            {
                Talk(6);

                Creature* aysa = player->FindNearestCreature(Creatures::AysaSpawnedAtGate, 20.f);
                if (aysa && aysa->IsAlive())
                {
                    aysa->m_Events.AddEventAtOffset([aysa]()
                        {
                            if (aysa && aysa->IsAlive())
                                aysa->AI()->Talk(6);
                        }, std::chrono::seconds(6));
                }
            }

            if (nodeId == 10)
                Talk(7);

            if (nodeId == 12)
                Talk(8);

            if (nodeId == 14)
                Talk(9);

            if (nodeId == 15)
                Talk(10);

            if (nodeId == 16)
                Talk(11);

            if (nodeId == 19)
                Talk(12);

            if (nodeId == 21)
            {
                Talk(13);

                Creature* aysa = player->FindNearestCreature(Creatures::AysaSpawnedAtGate, 20.f);
                if (aysa && aysa->IsAlive())
                {
                    aysa->m_Events.AddEventAtOffset([aysa]()
                        {
                            if (aysa && aysa->IsAlive())
                                aysa->AI()->Talk(7);
                        }, std::chrono::seconds(5));
                }
            }
        }

        void WaypointPathEnded(uint32 /*nodeId*/, uint32 /*pathId*/) override
        {
            if (Player* player = ObjectAccessor::GetPlayer(*me, _playerGUID))
            {
                player->KilledMonsterCredit(Creatures::CreditWalkWithKingVarian);
                me->m_Events.AddEventAtOffset([this, player]()
                    {
                        if (me && me->IsAlive())
                            Talk(14, player);
                    }, std::chrono::seconds(5));
            }
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                case Events::KingVarianSpawnedStartPath:
                    me->GetMotionMaster()->MovePath(Paths::StormwindKeepKingVarian, false);
                    break;

                case Events::KingVarianSpawnedInitialTalk:
                    Talk(0);
                    break;
                }
            }
        }

    private:
        EventMap _events;
        ObjectGuid _playerGUID;
    };

    // 61834 - Alyn Black
    struct npc_alyn_black : public ScriptedAI
    {
        npc_alyn_black(Creature* creature) : ScriptedAI(creature) {}

        void SetData(uint32 id, uint32 value) override
        {
            if (id == 1 && value == 1)
            {
                _scheduler.Schedule(5s, [this](TaskContext /*context*/)
                    {
                        if (me && me->IsAlive())
                            Talk(0);
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

    // 61840 - Naanae
    struct npc_naanae : public ScriptedAI
    {
        npc_naanae(Creature* creature) : ScriptedAI(creature) {}

        void SetData(uint32 id, uint32 value) override
        {
            if (id == 1 && value == 1)
            {
                _scheduler.Schedule(5s, [this](TaskContext /*context*/)
                    {
                        if (me && me->IsAlive())
                            Talk(0);
                    });

                _scheduler.Schedule(10s, [this](TaskContext /*context*/)
                    {
                        if (me && me->IsAlive())
                            Talk(1);
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
}

void AddSC_custom_stormwind_npcs()
{
    using namespace Scripts::EasternKingdoms::StormwindCity;

    RegisterCreatureAI(npc_king_varian_61796);
    //RegisterCreatureAI(npc_alyn_black);
    //RegisterCreatureAI(npc_naanae);
}
