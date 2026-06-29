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

#include "GenAI_chat_prompts.h"

#include "Creature.h"
#include "GenAI_client.h"
#include "Config/Followship_bots_config.h"
#include "Player.h"

#include "DB2Stores.h"
#include "Log.h"

namespace FSBNpcPrompts
{
    BotChatResponse GenerateNpcReply(NpcChatContext const& ctx, PlayerSnapshot const& player, std::string const& playerMsg,
        std::deque<BotChatMemoryEntry> const& memory)
    {
        BotChatResponse result;
        if (!ctx.entry || playerMsg.empty())
            return result;

        AreaTableEntry const* zone = sAreaTableStore.LookupEntry(ctx.zoneId);
        std::string zoneName = zone ? zone->AreaName[LOCALE_enUS] : "Unknown";

        AreaTableEntry const* area = sAreaTableStore.LookupEntry(ctx.areaId);
        std::string areaName = area ? area->AreaName[LOCALE_enUS] : zoneName;

        std::string systemPrompt =
            "You are a World of Warcraft NPC named " + ctx.npcName + ".\n"
            "You currently reside in " + areaName + ", " + zoneName + ".\n"
            "A player is speaking to you directly. You are a normal inhabitant of this world.\n"
            "Respond naturally and in character. Keep your reply brief (5 to 15 words).\n"
            "Never mention being an AI, bot, or NPC.\n"
            "Do not use quotation marks inside your reply.\n\n"
            "You MUST respond ONLY in valid JSON with exactly these fields:\n"
            "- \"reply\": a short in-character reply (5 to 15 words), natural and conversational.\n"
            "- \"action\": always \"none\".\n"
            "- \"amount\": always 0.\n";

        if (!player.name.empty())
        {
            std::string playerRaceStr = "Unknown";
            if (ChrRacesEntry const* raceEntry = sChrRacesStore.LookupEntry(player.race))
                playerRaceStr = raceEntry->Name[LOCALE_enUS];

            std::string playerClassStr = "Unknown";
            if (ChrClassesEntry const* classEntry = sChrClassesStore.LookupEntry(player.playerClass))
                playerClassStr = classEntry->Name[LOCALE_enUS];

            systemPrompt +=
                "The player speaking to you is " + player.name + ", a " +
                playerRaceStr + " " + playerClassStr + " of level " +
                std::to_string(player.level) + ".\n";
        }

        std::string userPrompt = "Given the Conversation History below, reply to the latest player message:\n";
        for (auto const& entry : memory)
            userPrompt += entry.senderName + ": " + entry.message + "\n";

        userPrompt += player.name + ": " + playerMsg + "\n";

        if (FollowshipBotsConfig::configFSBGenAIEnabled)
        {
            std::string aiResponse = FSBGenAI::GetStructuredBotResponse(systemPrompt, userPrompt);
            if (!aiResponse.empty())
            {
                std::string reply;
                std::string action;
                uint32 amount = 0;
                if (FSBGenAI::ParseStructuredResponse(aiResponse, reply, action, amount))
                {
                    result.reply = std::move(reply);
                    result.action = std::move(action);
                    result.amount = amount;
                    return result;
                }
            }
        }

        result.reply = "Greetings, traveler.";
        result.action = "none";
        result.amount = 0;
        return result;
    }

    NpcChatContext BuildNpcContext(Creature* npc)
    {
        NpcChatContext ctx;
        if (!npc)
            return ctx;

        ctx.entry = npc->GetEntry();
        ctx.npcName = npc->GetName();
        ctx.zoneId = npc->GetZoneId();
        ctx.areaId = npc->GetAreaId();
        return ctx;
    }

    PlayerSnapshot BuildPlayerSnapshot(Player* player)
    {
        PlayerSnapshot snap;
        if (!player)
            return snap;

        snap.name = player->GetName();
        snap.race = player->GetRace();
        snap.playerClass = player->GetClass();
        snap.level = player->GetLevel();
        return snap;
    }
}
