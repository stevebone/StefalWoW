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
    ## 244440 Legion Communicator #2
    ## CMSG_GAMEOBJECT_REPORT_USE -> talking-head conversation, then the
    ## gameobject is marked not selectable after use and remains spawned
    ######*/

    struct go_legion_communicator : public GameObjectAI
    {
        go_legion_communicator(GameObject* go) : GameObjectAI(go)
        {
            DetermineCommunicator();
        }

        bool OnReportUse(Player* player) override
        {
            if (_used)
                return true;

            _used = true;

            // Read the objective before awarding the credit that completes it.
            if (!player->IsQuestObjectiveComplete(Quests::AssaultOnMardum, _questObjective))
                Conversation::CreateConversation(_conversationId, player, *player, player->GetGUID());

            player->KilledMonsterCredit(_killCreditEntry);
            me->SetFlag(GO_FLAG_NOT_SELECTABLE);
            return false;
        }

    private:
        bool _used = false;
        uint32 _questObjective = 0;
        uint32 _conversationId = 0;
        uint32 _killCreditEntry = 0;

        void DetermineCommunicator()
        {
            switch (me->GetEntry())
            {
            case GameObjects::LegionCommunicator1:
                _questObjective = Objectives::LegionCommunicator1;
                _conversationId = Conversations::LegionCommunicatorReport1;
                _killCreditEntry = Creatures::LegionCommunicator1;
                break;
            case GameObjects::LegionCommunicator2:
                _questObjective = Objectives::LegionCommunicator2;
                _conversationId = Conversations::LegionCommunicatorReport2;
                _killCreditEntry = Creatures::LegionCommunicator2;
                break;
            default:
                break;
            }
        }
    };
}

void AddSC_custom_mardum_objects()
{
    using namespace Scripts::Custom::Mardum;

    RegisterGameObjectAI(go_legion_communicator);
}
