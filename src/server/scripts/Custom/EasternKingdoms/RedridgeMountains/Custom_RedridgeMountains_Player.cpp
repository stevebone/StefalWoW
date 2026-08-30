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

#include "Custom_RedridgeMountains_Defines.h"

namespace Scripts::EasternKingdoms::RedridgeMountains
{
    class player_summon_messner_on_relog : public PlayerScript
    {
    public:
        player_summon_messner_on_relog() : PlayerScript("player_summon_messner_on_relog") { }

        void OnLogin(Player* player, bool /*firstLogin*/) override
        {
            if (player->HasAura(Spells::SummonMessner))
                player->CastSpell(player, Spells::SummonMessner, true);
        }
    };

    class player_summon_jorgensen_on_relog : public PlayerScript
    {
    public:
        player_summon_jorgensen_on_relog() : PlayerScript("player_summon_jorgensen_on_relog") { }

        void OnLogin(Player* player, bool /*firstLogin*/) override
        {
            if (player->HasAura(Spells::SummonJorgensen))
                player->CastSpell(player, Spells::SummonJorgensen, true);
        }
    };

    class player_summon_krakauer_on_relog : public PlayerScript
    {
    public:
        player_summon_krakauer_on_relog() : PlayerScript("player_summon_krakauer_on_relog") { }

        void OnLogin(Player* player, bool /*firstLogin*/) override
        {
            if (player->HasAura(Spells::SummonKrakauer))
                player->CastSpell(player, Spells::SummonKrakauer, true);
        }
    };

    class player_summon_danforth_on_relog : public PlayerScript
    {
    public:
        player_summon_danforth_on_relog() : PlayerScript("player_summon_danforth_on_relog") { }

        void OnLogin(Player* player, bool /*firstLogin*/) override
        {
            if (player->HasAura(Spells::SummonDanforth))
                player->CastSpell(player, Spells::SummonDanforth, true);
        }
    };
}

void AddSC_custom_redridge_mountains_player()
{
    using namespace Scripts::EasternKingdoms::RedridgeMountains;

    new player_summon_messner_on_relog();
    new player_summon_jorgensen_on_relog();
    new player_summon_krakauer_on_relog();
    new player_summon_danforth_on_relog();
}
