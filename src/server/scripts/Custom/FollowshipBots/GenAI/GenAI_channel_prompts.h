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

#include <deque>
#include <string>
#include <vector>

class Creature;
class Player;

#include "Followship_bots_defines.h"
#include "GenAI/GenAI_chat_memory.h"

namespace FSBChannelPrompts
{
    struct BotChatContext
    {
        uint32 entry = 0;
        uint8 level = 0;
        uint32 zoneId = 0;
        uint32 areaId = 0;
        FSB_Class botClass = FSB_Class::None;
        FSB_Race botRace = FSB_Race::None;
        FSB_ChatterType personality = FSB_ChatterType::None;
        FSB_Roles role = FSB_Roles::FSB_ROLE_NONE;
        uint32 goldGivenCount = 0;
        uint32 lastGoldGiveTime = 0;
        std::string botName;
        bool allowGold = false;
    };
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

    inline const std::vector<std::string> TradeBuyingExamples =
    {
        "WTB [ITEM] PST",
        "Wanna buy [ITEM]",
        "Need [ITEM] ASAP",
        "Anyone have [ITEM]?",
        "Anyone selling [ITEM]?",
        "Buying [ITEM] hit me up",
        "Looking for [ITEM]",
        "LF [ITEM] will pay well",
        "Need [ITEM] send price",
        "WTB [ITEM] in [LOCATION]"
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

    // General chat example pools
    inline const std::vector<std::string> GeneralZoneExamples =
    {
        "{area} sure is lively today",
        "I love the atmosphere in {area}",
        "{area} never gets old",
        "Wish {area} had a better inn",
        "Can't wait to leave {area}",
        "The guards in {area} are on edge lately",
        "{area} looks different at night",
        "Anyone else love the music in {area}?"
    };

    inline const std::vector<std::string> GeneralLoreExamples =
    {
        "Remember the Corrupted Blood incident?",
        "At least I have chicken...",
        "50 DKP minus! Classic wipe",
        "Did someone say Thunderfury?",
        "Leeroy Jenkins would charge this mob",
        "Illegal Danish was peak WoW cinema",
        "The AQ gate opening lag was unreal",
        "Bubblehearth was the ultimate escape",
        "The South Park episode was spot on",
        "Thunderfury, Blessed Blade of the Windseeker",
        "Many whelps, handle it!",
        "Arcanite Reaper... HOOOOO!"
    };

    inline const std::vector<std::string> GeneralClassExamples =
    {
        "Being a [CLASS] is tough sometimes",
        "[CLASS] problems: running out of mana",
        "Every [CLASS] needs a healer friend",
        "I rolled [CLASS] because it looked cool",
        "[CLASS] main since vanilla",
        "Being [CLASS] is underrated",
        "[CLASS] life chose me",
        "Anyone else [CLASS] here?"
    };

    inline const std::vector<std::string> GeneralNPCExamples =
    {
        "Thrall really let the Horde down in BfA",
        "Jaina has been through so much",
        "Sylvanas did nothing wrong... or did she?",
        "Illidan was right all along",
        "Varian's sacrifice still hits hard",
        "Anduin needs to grow a spine",
        "Tyrande went full vengeance mode",
        "Genn Greymane needs to chill",
        "Bolvar as the Lich King was wild",
        "Khadgar's beard has its own fan club"
    };

    inline const std::vector<std::string> GeneralPvPExamples =
    {
        "Alterac Valley lasted 8 hours minimum",
        "Southshore vs Tarren Mill was true world PvP",
        "High Warlord grind was brutal",
        "Grand Marshal title meant no sleep",
        "Warsong Gulch flag running was an art",
        "Nothing beats a good world PvP brawl",
        "Arathi Basin was my favorite battleground",
        "Eye of the Storm was chaos incarnate"
    };

    inline const std::vector<std::string> GeneralSocialExamples =
    {
        "Anyone else miss old Azeroth?",
        "What's your favorite expansion?",
        "Leveling an alt is surprisingly fun",
        "The economy is weird right now",
        "Guild recruitment is impossible these days",
        "Anyone want to group up for quests?",
        "Vanilla was better, fight me",
        "The new patch changed everything"
    };

    inline const std::vector<std::string> GeneralWorldExamples =
    {
        "It's getting dark, better watch the roads",
        "This rain never stops in {area}",
        "Beautiful morning in {area}",
        "The stars look amazing tonight",
        "Fog is rolling in through {area}",
        "Perfect weather for grinding in {area}",
        "Getting chilly as the sun sets"
    };

    inline const std::vector<std::string> GeneralAchievementExamples =
    {
        "Finally got [ACHIEVEMENT], took forever",
        "Anyone else working toward [ACHIEVEMENT]?",
        "[ACHIEVEMENT] was harder than it looks",
        "Proud owner of [ACHIEVEMENT]",
        "[ACHIEVEMENT] complete, what's next?",
        "Grinding for [ACHIEVEMENT] all week"
    };

    struct BotChatResponse
    {
        std::string reply;
        std::string action = "none";
        uint32 amount = 0;
    };

    struct PlayerSnapshot
    {
        std::string name;
        uint8       race = 0;
        uint8       playerClass = 0;
        uint32      level = 0;
    };

    std::string GenerateTradeMessage(Creature* bot);
    std::string GenerateLFGMessage(Creature* bot);
    std::string GenerateGeneralMessage(BotChatContext const& ctx);
    BotChatResponse GenerateReplyToPlayer(BotChatContext const& ctx, PlayerSnapshot const& player, std::string const& playerMsg,
        std::deque<BotChatMemoryEntry> const& memory);
}
