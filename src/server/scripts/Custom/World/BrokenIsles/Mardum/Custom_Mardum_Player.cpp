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

#include "ObjectMgr.h"
#include "Player.h"
#include "QuestDef.h"
#include "ScriptMgr.h"

#include "Custom_Mardum_Defines.h"

namespace Scripts::Custom::Mardum
{
    // 39279 - Assault on Mardum (bonus objective, auto-granted to Demon Hunters entering Mardum)
    class player_mardum_assault_bonus_objective : public PlayerScript
    {
    public:
        player_mardum_assault_bonus_objective() : PlayerScript("player_mardum_assault_bonus_objective") { }

        void OnLogin(Player* player, bool firstLogin) override
        {
            if (firstLogin)
                TryGrant(player);
        }

        // Covers DHs that did not spawn directly in Mardum (e.g. Void Elf DH starts on map 1865)
        // and reach it later via teleport/hearth/quest travel.
        void OnMapChanged(Player* player) override
        {
            TryGrant(player);
        }

    private:
        void TryGrant(Player* player)
        {
            if (player->GetClass() != CLASS_DEMON_HUNTER || player->GetMapId() != Maps::Mardum)
                return;

            // QUEST_STATUS_NONE only when never taken (or abandoned) - skips in-log, complete and rewarded
            if (player->GetQuestStatus(Quests::AssaultOnMardum) != QUEST_STATUS_NONE)
                return;

            if (Quest const* quest = sObjectMgr->GetQuestTemplate(Quests::AssaultOnMardum))
                if (player->CanAddQuest(quest, false))
                    player->AddQuest(quest, nullptr);
        }
    };
}

void AddSC_custom_mardum_player()
{
    using namespace Scripts::Custom::Mardum;
    new player_mardum_assault_bonus_objective();
}
