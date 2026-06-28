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

#include "ScriptMgr.h"                 // PlayerScript
#include "Player.h"

// This script adds the needed player local flag that allows the backpack to receive the extra slots
class player_extended_backpack_slots : public PlayerScript
{
public:
    player_extended_backpack_slots() : PlayerScript("player_extended_backpack_slots") {}

    void OnLogin(Player* player, bool firstLogin) override
    {
        if (player && firstLogin)
            player->SetAccountSecured(true);
    }
};

void AddSC_custom_player_inventory()
{
    new player_extended_backpack_slots();
}
