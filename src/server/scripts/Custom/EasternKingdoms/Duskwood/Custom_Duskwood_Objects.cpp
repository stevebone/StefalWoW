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
#include "GameObject.h"
#include "GameObjectAI.h"
#include "ScriptMgr.h"

#include "Custom_Duskwood_Defines.h"

namespace Scripts::EasternKingdoms::Duskwood
{
    /*######
    ## go_mound_of_loose_dirt
    ######*/

    struct go_mound_of_loose_dirt : public GameObjectAI
    {
        go_mound_of_loose_dirt(GameObject* go) : GameObjectAI(go) { }

        bool OnReportUse(Player* /*player*/) override
        {
            if (Creature* worgen = me->FindNearestCreature(Creatures::LurkingWorgen, 30.0f))
                worgen->AI()->SetData(Data::DataPounce, 1);

            return true;
        }
    };
}

void AddSC_custom_duskwood_objects()
{
    using namespace Scripts::EasternKingdoms::Duskwood;

    RegisterGameObjectAI(go_mound_of_loose_dirt);
}
