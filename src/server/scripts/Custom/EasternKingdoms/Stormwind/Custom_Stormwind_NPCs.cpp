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
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "TaskScheduler.h"

#include "Custom_Stormwind_Defines.h"

namespace Scripts::EasternKingdoms::StormwindCity
{
    // 61836 - Moni Widdlesprock
    struct npc_moni_widdlesprock : public ScriptedAI
    {
        npc_moni_widdlesprock(Creature* creature) : ScriptedAI(creature) {}

        void SetData(uint32 id, uint32 value) override
        {
            if (id == 1 && value == 1)
            {
                Talk(0);
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

    // Creature
    //RegisterCreatureAI(npc_moni_widdlesprock);
    //RegisterCreatureAI(npc_alyn_black);
    //RegisterCreatureAI(npc_naanae);
}
