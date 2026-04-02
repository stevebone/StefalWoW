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

#include "Player.h"
#include "ScriptMgr.h"

#include "Custom_TheWanderingIsle_Defines.h"

namespace Scripts::Custom::TheWanderingIsle
{
    // 102632 PlayerScript On Relog
    class player_quest_29423_summon_on_relog : public PlayerScript
    {
    public:
        player_quest_29423_summon_on_relog() : PlayerScript("player_quest_29423_summon_on_relog") {}

        void OnLogin(Player* player, bool /*loginFirst*/)
        {
            if (player->GetQuestStatus(Quests::quest_the_passion_of_shen_zin_su) != QUEST_STATUS_INCOMPLETE)
                return;

            player->CastSpell(player, SpellsQ29423::spell_summon_spirit_of_fire_on_relog, true);
        }
    };

    // 103538 PlayerScript On Relog
    class player_quest_29680_summon_on_relog : public PlayerScript
    {
    public:
        player_quest_29680_summon_on_relog() : PlayerScript("player_quest_29680_summon_on_relog") {}

        void OnLogin(Player* player, bool /*loginFirst*/)
        {
            if (player->IsInWorld() && player->IsActiveQuest(Quests::quest_the_source_of_livelihood))
                player->CastSpell(player, SpellsQ29678Q29679::spell_summon_spirit_of_water, true);
        }
    };

    // adds a cooldown per player to the area trigger
    player_singing_pools_memory::player_singing_pools_memory() = default;

    bool player_singing_pools_memory::CanTrigger(Player* player)
    {
        time_t now = time(nullptr);
        auto& t = lastTrigger[player->GetGUID()];

        if (now - t < 120)
            return false;

        t = now;
        return true;
    }

    player_singing_pools_memory g_singingPoolsMemory;
}

void AddSC_custom_the_wandering_isle_player()
{
    using namespace Scripts::Custom::TheWanderingIsle;
    new player_quest_29423_summon_on_relog();
    new player_quest_29680_summon_on_relog();
}
