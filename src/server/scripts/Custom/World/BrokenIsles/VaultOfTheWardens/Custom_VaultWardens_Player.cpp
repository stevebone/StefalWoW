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

#include "EventProcessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestDef.h"
#include "ScriptMgr.h"

#include "Custom_VaultWardens_Defines.h"

namespace Scripts::Custom::VaultOfTheWardens
{
    // 39742 - Vault of the Wardens auto-rewards on completion (bonus quest, no
    // questgiver turn-in). Scheduled for the same reason as the Mardum assault
    // quest: RewardQuest -> RemoveActiveQuest erases the quest's
    // m_questObjectiveStatus entries while UpdateQuestObjectiveProgress is
    // still iterating them.
    class player_vault_wardens_auto_reward : public PlayerScript
    {
    public:
        player_vault_wardens_auto_reward() : PlayerScript("player_vault_wardens_auto_reward") { }

        void OnQuestStatusChange(Player* player, uint32 questId) override
        {
            if (questId != Quests::VaultOfTheWardensQuest || player->GetQuestStatus(questId) != QUEST_STATUS_COMPLETE)
                return;

            player->m_Events.AddEventAtOffset([player]()
            {
                // a second COMPLETE status change may have already queued/run
                // the reward
                if (player->GetQuestStatus(Quests::VaultOfTheWardensQuest) != QUEST_STATUS_COMPLETE)
                    return;

                if (Quest const* quest = sObjectMgr->GetQuestTemplate(Quests::VaultOfTheWardensQuest))
                    player->RewardQuest(quest, LootItemType::Item, 0, player);
            }, 2s);
        }
    };
}

void AddSC_custom_vault_wardens_player()
{
    using namespace Scripts::Custom::VaultOfTheWardens;

    new player_vault_wardens_auto_reward();
}
