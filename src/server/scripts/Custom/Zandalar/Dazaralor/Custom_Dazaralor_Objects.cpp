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
  
#include "GameObject.h"  
#include "GameObjectAI.h"  
#include "Player.h"  
#include "ScriptMgr.h"  
#include "QuestDef.h"  
  
#include "Custom_Dazaralor_Defines.h"  
  
namespace Scripts::Custom::Dazaralor  
{  
    // GO 78687 - Portal to Orgrimmar (quest 55137 "The Throne of Zuldazar")  
    struct go_dazaralor_orgrimmar_portal : public GameObjectAI  
    {  
        go_dazaralor_orgrimmar_portal(GameObject* go) : GameObjectAI(go) { }  
  
        bool OnReportUse(Player* player) override  
        {  
            if (!player)  
                return false;  
  
            if (player->GetQuestStatus(Quests::ForTheHorde) == QUEST_STATUS_INCOMPLETE)
                player->UpdateQuestObjectiveProgress(QUEST_OBJECTIVE_CRITERIA_TREE, Objectives::PortalToOrgrimmarTaken, 1);  
  
            // Teleport to the Gates of Orgrimmar  
            player->CastSpell(player, Spells::PortalOrgrimmar, true);  
  
            return true;  
        }  
    };  
}  
  
void AddSC_custom_dazaralor_objects()  
{  
    using namespace Scripts::Custom::Dazaralor;  
  
    RegisterGameObjectAI(go_dazaralor_orgrimmar_portal);  
}
