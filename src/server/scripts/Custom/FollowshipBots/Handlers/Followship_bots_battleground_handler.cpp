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
#include "Log.h"

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
}
