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
#include "Creature.h"
#include "CreatureAI.h"

#include "Custom_Player_Defines.h"

class player_chromie_intro : public PlayerScript
{
public:
    player_chromie_intro() : PlayerScript("player_chromie_intro") {}

    void OnQuestStatusChange(Player* player, uint32 questId) override
    {
        if (!player || player->GetQuestStatus(questId) != QUEST_STATUS_INCOMPLETE)
            return;

        for (auto const& entry : ChromieIntroQuests)
        {
            if (questId != entry.AllianceQuest && questId != entry.HordeQuest)
                continue;

            if (Creature* chromie = player->FindNearestCreature(NPC_CHROMIE, 20.0f))
                chromie->AI()->Talk(entry.TalkGroupId, player);

            break;
        }
    }
};

void AddSC_custom_player_chromie()
{
    new player_chromie_intro();
}
