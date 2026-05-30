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
#include "GameObject.h"
#include "GameObjectAI.h"
#include "InstanceScript.h"
#include "Item.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "Spell.h"
#include "WorldSession.h"

#include "Custom_Instance_Deadmines.h"

namespace Scripts::EasternKingdoms::Deadmines
{
    struct go_defias_cannon : public GameObjectAI
    {
        go_defias_cannon(GameObject* go) : GameObjectAI(go) { }

        bool OnReportUse(Player* player) override
        {
            InstanceScript* instance = me->GetInstanceScript();

            if (!instance)
                return false;

            if (!player->HasItemCount(Items::DefiasGunpowder, 1))
                return false;

            instance->SetData(CannonEvent::DATA_EVENT, CannonEvent::STATE_CANNON_GUNPOWDER_USED);
            return true;
        }
    };
}

void AddSC_custom_deadmines_objects()
{
    using namespace Scripts::EasternKingdoms::Deadmines;

    RegisterGameObjectAI(go_defias_cannon);
}
