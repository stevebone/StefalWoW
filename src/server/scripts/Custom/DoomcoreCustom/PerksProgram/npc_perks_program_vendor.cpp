/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
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
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Creature.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "ScriptedCreature.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "PerksProgramMgr.h"
#include "Player.h"
#include "WorldSession.h"

struct go_perks_collectors_cache : public GameObjectAI
{
    go_perks_collectors_cache(GameObject* go) : GameObjectAI(go) { }

    bool OnGossipHello(Player* player) override
    {
        PerksProgramActivityLists lists = sPerksProgramMgr->GetCurrentMonthActivityLists();
        bool hasPending = false;
        
        for (int32 id : lists.ThresholdActivityIDs)
        {
            if (player->HasPerksMilestone(id))
            {
                hasPending = true;
                break;
            }
        }

        if (!hasPending)
            return true; 
            
        // placeholder until CurrencyAmount lookup is implemented
        player->GetSession()->SendPerksProgramCurrencyUpdate();
        return true;
    }
};

void AddSC_npc_perks_program_vendor()
{
    RegisterGameObjectAI(go_perks_collectors_cache);
}
