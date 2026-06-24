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

#include <string>
#include <vector>

class Creature;

namespace FSBChannelPrompts
{
    inline const std::vector<std::string> TradeSellingExamples =
    {
        "WTS [ITEM] PST",
        "Anyone need [ITEM]?",
        "Offloading [ITEM] come find me",
        "Want [ITEM]? Message me",
        "For sale: [ITEM]",
        "Cheap [ITEM] going fast",
        "Grab [ITEM] before I vendor it",
        "Need [ITEM]? Hit me up",
        "Selling [ITEM] in [LOCATION]",
        "[ITEM] available, first come first serve",
        "WTT [ITEM] for something cool",
        "Clearing out [ITEM]"
    };

    inline const std::vector<std::string> TradeFreeExamples =
    {
        "Giving away [ITEM] for free",
        "Free [ITEM] near the mailbox",
        "Who wants [ITEM]?",
        "Take [ITEM] off my hands",
        "[ITEM] going free first come first serve",
        "Want [ITEM]? It's free"
    };

    inline const std::vector<std::string> LFGDungeonExamples =
    {
        "LFM [DUNGEON]",
        "LFG [DUNGEON] PST",
        "Need group for [DUNGEON]",
        "Forming group for [DUNGEON]",
        "Anyone up for [DUNGEON]?",
        "Running [DUNGEON] need more",
        "Looking for more for [DUNGEON]",
        "[CLASS] looking for [DUNGEON] group",
        "[DUNGEON] run forming now",
        "Join me for [DUNGEON]",
        "[DUNGEON] - need healer and dps",
        "Speed run [DUNGEON] anyone?"
    };

    inline const std::vector<std::string> LFGRaidExamples =
    {
        "LFR [DUNGEON]",
        "Looking for raid for [DUNGEON]",
        "Need raiders for [DUNGEON]",
        "Forming raid for [DUNGEON]",
        "Assembling raid for [DUNGEON]",
        "[DUNGEON] raid group forming",
        "[DUNGEON] - recruiting dps and healers",
        "Raid [DUNGEON] tonight need more",
        "[CLASS] lf raid for [DUNGEON]",
        "Anyone wanna raid [DUNGEON]?",
        "[DUNGEON] progression run forming",
        "Join the [DUNGEON] raid"
    };

    std::string GenerateTradeMessage(Creature* bot);
    std::string GenerateLFGMessage(Creature* bot);
}
