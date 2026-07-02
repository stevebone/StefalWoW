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

#include "Followship_bots_battleground_handler.h"
#include "Battleground/Followship_bots_warsong_gulch.h"

#include "Battleground.h"
#include "BattlegroundScore.h"
#include "Log.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Unit.h"

namespace FSBBattleground
{
    void SpawnBots(Battleground* battleground, BattlegroundMap* /*battlegroundMap*/)
    {
        if (!battleground)
        {
            TC_LOG_INFO("scripts.fsb.battleground", "FSBBattleground::SpawnBots: null battleground");
            return;
        }

        TC_LOG_INFO("scripts.fsb.battleground", "FSBBattleground::SpawnBots: BG type {}", battleground->GetTypeID());

        switch (battleground->GetTypeID())
        {
            case BATTLEGROUND_WS:
            case BATTLEGROUND_WG_CTF:
                WarsongGulch::SpawnBots(battleground);
                break;
            default:
                TC_LOG_DEBUG("scripts.fsb.battleground", "FSBBattleground::SpawnBots not implemented for bg type {}", battleground->GetTypeID());
                break;
        }
    }

    void HandlePlayerKilledBot(ObjectGuid killerGuid, Unit* botVictim)
    {
        if (!killerGuid || !botVictim)
            return;

        BattlegroundMap* bgMap = botVictim->GetMap()->ToBattlegroundMap();
        if (!bgMap)
            return;

        Battleground* bg = bgMap->GetBG();
        if (!bg)
            return;

        Player* killer = ObjectAccessor::GetPlayer(bgMap, killerGuid);
        if (!killer)
            return;

        bg->UpdatePlayerScore(killer, SCORE_KILLING_BLOWS, 1);
        bg->UpdatePlayerScore(killer, SCORE_HONORABLE_KILLS, 1);
    }

    void HandleBotKilledPlayer(Unit* botKiller, ObjectGuid victimGuid)
    {
        if (!botKiller || !victimGuid)
            return;

        BattlegroundMap* bgMap = botKiller->GetMap()->ToBattlegroundMap();
        if (!bgMap)
            return;

        Battleground* bg = bgMap->GetBG();
        if (!bg)
            return;

        Player* victim = ObjectAccessor::GetPlayer(bgMap, victimGuid);
        if (!victim)
            return;

        bg->UpdatePlayerScore(victim, SCORE_DEATHS, 1);
    }
}
