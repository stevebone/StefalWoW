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
#include "Player.h"
#include "PhasingHandler.h"

#include "Custom_TheWanderingIsle_Defines.h"

namespace Scripts::TheWanderingIsle::Quests
{
    // 29422
    class quest_29422_huo_the_spirit_of_fire : public QuestScript
    {
    public:
        quest_29422_huo_the_spirit_of_fire() : QuestScript("quest_29422_huo_the_spirit_of_fire") {}

        void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
        {
            switch (newStatus)
            {
            case QUEST_STATUS_NONE:
            case QUEST_STATUS_INCOMPLETE:
            case QUEST_STATUS_COMPLETE:
                PhasingHandler::OnConditionChange(player);
                break;
            default:
                break;
            }
        }
    };
}

void AddSC_custom_the_wandering_isle_quests()
{
    using namespace Scripts::TheWanderingIsle::Quests;
    new quest_29422_huo_the_spirit_of_fire();
}
