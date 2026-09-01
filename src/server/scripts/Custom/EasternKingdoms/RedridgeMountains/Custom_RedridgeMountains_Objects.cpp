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
#include "CreatureAI.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "Player.h"
#include "ScriptMgr.h"

#include <vector>
#include <random>

#include "Custom_RedridgeMountains_Defines.h"

namespace Scripts::EasternKingdoms::RedridgeMountains
{
    /*######
    ## 204403 Chain Lever
    ######*/

    struct go_chain_lever : public GameObjectAI
    {
        go_chain_lever(GameObject* go) : GameObjectAI(go) { }

        bool OnReportUse(Player* /*player*/) override
        {
            if (Creature* danforth = me->FindNearestCreature(Creatures::DanforthCaptured, 50.0f))
                danforth->AI()->SetData(1, 1);

            return true;
        }
    };

    /*######
    ## 204441/204442/204435 Blackrock Holding Pen
    ######*/

    struct go_blackrock_holding_pen : public GameObjectAI
    {
        go_blackrock_holding_pen(GameObject* go) : GameObjectAI(go) { }

        bool OnReportUse(Player* player) override
        {
            player->KilledMonsterCredit(Creatures::PrisonerOfWarKillCredit);

            std::vector<Creature*> citizens;

            std::list<Creature*> maleList;
            me->GetCreatureListWithEntryInGrid(maleList, Creatures::KidnappedRedridgeCitizenMale, 5.0f);
            for (Creature* creature : maleList)
                citizens.push_back(creature);

            std::list<Creature*> femaleList;
            me->GetCreatureListWithEntryInGrid(femaleList, Creatures::KidnappedRedridgeCitizenFemale, 5.0f);
            for (Creature* creature : femaleList)
                citizens.push_back(creature);

            for (Creature* creature : citizens)
                creature->AI()->SetData(1, 1);

            if (!citizens.empty())
            {
                size_t randomIndex = urand(0, citizens.size() - 1);
                citizens[randomIndex]->AI()->SetData(2, 2);
            }

            return true;
        }
    };
}

void AddSC_custom_redridge_mountains_objects()
{
    using namespace Scripts::EasternKingdoms::RedridgeMountains;

    RegisterGameObjectAI(go_chain_lever);
    RegisterGameObjectAI(go_blackrock_holding_pen);
}
