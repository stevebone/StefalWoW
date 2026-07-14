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

#pragma once

#include "Define.h"
#include "SharedDefines.h"

#include "Followship_bots_chatter_handler.h"

#include <array>
#include <string>

namespace FSBBattlegroundChat
{
    inline std::array<std::string, 20> const SpawnChatLines = {{
        "Hello everyone!",
        "How is everyone today?",
        "Ready to kick some [enemyTeam] ass?",
        "I am gonna rest for a bit while you lot take care of the [enemyTeam].",
        "For the [ownTeam]!!!!",
        "Let's get this win for the [ownTeam]!",
        "Time to show the [enemyTeam] what we're made of.",
        "Good luck everyone, let's make the [ownTeam] proud.",
        "I'll do my part, you do yours.",
        "Another day, another battlefield.",
        "Let's crush the [enemyTeam] and go home.",
        "The [enemyTeam] won't know what hit them.",
        "Stay focused, fight for the [ownTeam].",
        "I'm here to help secure the victory.",
        "No mercy for the [enemyTeam] today.",
        "Let's work together and win this.",
        "The [ownTeam] will prevail!",
        "I'm ready when you are.",
        "Don't let the [enemyTeam] get comfortable.",
        "Victory belongs to the [ownTeam]!"
    }};

    inline std::string FormatChatLine(std::string const& line, Team botTeam)
    {
        std::string result = line;
        std::string own = botTeam == ALLIANCE ? "Alliance" : "Horde";
        std::string enemy = botTeam == ALLIANCE ? "Horde" : "Alliance";
        FSBChatter::ReplaceAll(result, "[ownTeam]", own);
        FSBChatter::ReplaceAll(result, "[enemyTeam]", enemy);
        return result;
    }
}
