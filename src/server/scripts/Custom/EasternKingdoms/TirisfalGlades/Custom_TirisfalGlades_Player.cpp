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

#include "Chat.h"
#include "ScriptMgr.h"
#include "Player.h"

#include "Custom_TirisfalGlades_Defines.h"

class player_tirisfal_glades_darnell_handler : public PlayerScript
{
public:
    player_tirisfal_glades_darnell_handler() : PlayerScript("player_tirisfal_glades_darnell_handler") { }

    void OnLogin(Player* player, bool /*firstLogin*/) override
    {
        //bool needToDespawnDarnell = false;

        if (player->IsInWorld() && (player->IsActiveQuest(QUEST_THE_SHADOW_GRAVE) || player->IsActiveQuest(QUEST_RECRUITMENT)))
            player->CastSpell(player, SPELL_DEATHKNELL_SUMMON_DARNELL, true);

        //if (player->GetQuestStatus(QUEST_THE_SHADOW_GRAVE) == QUEST_STATUS_COMPLETE || player->GetQuestStatus(QUEST_RECRUITMENT) == QUEST_STATUS_REWARDED)
        //    needToDespawnDarnell = true;

        /* Despawn should happen on quest status change and NOT at login
        else if (needToDespawnDarnell)
        {
            std::list<TempSummon*> summons;
            player->GetAllMinionsByEntry(summons, NPC_DEATHKNELL_DARNELL);

            for (TempSummon* darnell : summons)
            {
                if (!darnell || !darnell->IsAlive())
                    continue;

                // add here a call to Darnells AI to go random point and despawn
                // for now we despawn directly
                darnell->DespawnOrUnsummon(5s);
            }
        }
        */
    }
};

void AddSC_custom_tirisfal_glades_player()
{
    new player_tirisfal_glades_darnell_handler();
}
