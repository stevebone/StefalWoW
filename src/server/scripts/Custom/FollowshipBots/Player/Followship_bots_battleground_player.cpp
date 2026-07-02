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

#include "../Handlers/Followship_bots_battleground_handler.h"

#include "Battleground.h"
#include "BattlegroundMgr.h"
#include "BattlegroundScript.h"
#include "Map.h"
#include "Player.h"
#include "ScriptMgr.h"

class followship_bots_battleground_player : public PlayerScript
{
public:
    followship_bots_battleground_player() : PlayerScript("followship_bots_battleground_player") { }

    void OnMapChanged(Player* player) override
    {
        TC_LOG_INFO("scripts.fsb.battleground", "FSB BG Player: OnMapChanged for player {} map {}", player->GetName(), player->GetMapId());

        if (!sBattlegroundMgr->IsFSBOverrideEnabled())
        {
            TC_LOG_INFO("scripts.fsb.battleground", "FSB BG Player: FSB override not enabled, aborting spawn");
            return;
        }

        BattlegroundMap* battlegroundMap = player->GetMap()->ToBattlegroundMap();
        if (!battlegroundMap)
        {
            TC_LOG_INFO("scripts.fsb.battleground", "FSB BG Player: Not a battleground map, aborting spawn");
            return;
        }

        Battleground* battleground = battlegroundMap->GetBG();
        if (!battleground)
        {
            TC_LOG_INFO("scripts.fsb.battleground", "FSB BG Player: No battleground instance, aborting spawn");
            return;
        }

        BattlegroundScript* script = battlegroundMap->GetBattlegroundScript();
        if (!script)
        {
            TC_LOG_INFO("scripts.fsb.battleground", "FSB BG Player: No battleground script, aborting spawn");
            return;
        }

        if (script->GetData(FSBBattleground::BotsSpawnedDataId) != 0)
        {
            TC_LOG_INFO("scripts.fsb.battleground", "FSB BG Player: Bots already spawned for this BG, aborting");
            return;
        }

        TC_LOG_INFO("scripts.fsb.battleground", "FSB BG Player: Spawning bots for BG type {}", battleground->GetTypeID());
        script->SetData(FSBBattleground::BotsSpawnedDataId, 1);
        FSBBattleground::SpawnBots(battleground, battlegroundMap);
    }
};

void AddSC_followship_bots_battleground_player()
{
    new followship_bots_battleground_player();
}
