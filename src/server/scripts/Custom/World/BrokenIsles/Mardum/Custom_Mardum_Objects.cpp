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

#include "Conversation.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "Player.h"
#include "ScriptMgr.h"

#include "Custom_Mardum_Defines.h"

namespace Scripts::Custom::Mardum
{
    /*######
    ## 244439 Legion Communicator #1
    ## CMSG_GAMEOBJECT_REPORT_USE -> talking-head conversation 558, then the
    ## gameobject is marked in use (not interactable) and despawns 1s later,
    ## respawning after 5 minutes.
    ######*/

    struct go_legion_communicator : public GameObjectAI
    {
        go_legion_communicator(GameObject* go) : GameObjectAI(go) { }

        bool OnReportUse(Player* player) override
        {
            if (_used)
                return true;

            _used = true;

            // Read the objective before awarding the credit that completes it.
            if (!player->IsQuestObjectiveComplete(Quests::AssaultOnMardum, Objectives::LegionCommunicator1))
                Conversation::CreateConversation(Conversations::LegionCommunicatorReport, player, *player, player->GetGUID());

            player->KilledMonsterCredit(Creatures::LegionCommunicator1);
            me->SetFlag(GO_FLAG_IN_USE);
            me->DespawnOrUnsummon(5min, 1s);
            return false;
        }

    private:
        bool _used = false;
    };
}

void AddSC_custom_mardum_objects()
{
    using namespace Scripts::Custom::Mardum;
    RegisterGameObjectAI(go_legion_communicator);
}
