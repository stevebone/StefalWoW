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
#include "GameTime.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestDef.h"
#include "Weather.h"
#include "WowTime.h"

#include "DB2Stores.h"
#include "Log.h"

#include "Utils/Followship_bots_utils.h"

#include <algorithm>
#include <vector>

namespace FSBNpcPrompts
{
    static bool PlayerIsOfferingHelp(std::string const& msg)
    {
        std::string lower = msg;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        static const std::vector<std::string> helpKeywords =
        {
            "help", "assist", "need a hand", "need hand", "give you a hand",
            "what's wrong", "whats wrong", "what is wrong",
            "what do you need", "what you need", "anything i can do",
            "anything i can help", "can i help", "work for me",
            "got work", "any work", "you seem troubled", "you look troubled",
            "what's the matter", "whats the matter", "are you okay",
            "you okay", "trouble", "quest", "task", "job for me",
            "do for you", "fetch", "bring", "collect", "gather"
        };

        for (auto const& kw : helpKeywords)
            if (lower.find(kw) != std::string::npos)
                return true;

        return false;
    }

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
            "You are a World of Warcraft NPC named " + ctx.npcName + ".\n";

        if (!ctx.subName.empty())
            systemPrompt += "You are also known as: " + ctx.subName + ".\n";

        systemPrompt +=
            "You currently reside in " + areaName + ", " + zoneName + ".\n";

        if (!ctx.inGameTime.empty())
            systemPrompt += "The current in-game time is " + ctx.inGameTime + ".\n";

        if (!ctx.weather.empty())
            systemPrompt += "The weather here is currently " + ctx.weather + ".\n";

        systemPrompt +=
            "A player is speaking to you directly. You are a normal inhabitant of this world.\n"
            "Never mention being an AI, bot, or NPC.\n"
            "Do not use quotation marks inside your reply.\n";

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

        bool playerOfferingHelp = ctx.hasQuest && PlayerIsOfferingHelp(playerMsg);

        if (ctx.hasQuest && playerOfferingHelp)
        {
            systemPrompt +=
                "\nPRIVATE QUEST CONTEXT (you know this, the player has offered to help):\n"
                "- quest_title: \"" + ctx.questTitle + "\"\n"
                "- quest_description: \"" + ctx.questDescription + "\"\n"
                "\nThe player has offered to help or asked what you need. Briefly and naturally describe the task in 2-3 sentences using the quest context above.\n";
        }

        systemPrompt +=
            "\nYou MUST respond ONLY in valid JSON with exactly these fields, IN THIS ORDER:\n"
            "- \"wants_help\": " + std::string(playerOfferingHelp ? "true" : "false") + " (pre-determined by the system).\n"
            "- \"reply\": a short in-character reply (10 to 15 words), natural and conversational, NO quotation marks inside.\n";

        std::string userPrompt = "Given the Conversation History below, reply to the latest player message:\n";
        for (auto const& entry : memory)
            userPrompt += entry.senderName + ": " + entry.message + "\n";

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

    NpcChatContext BuildNpcContext(Creature* npc, Player* player)
    {
        NpcChatContext ctx;
        if (!npc)
            return ctx;

        ctx.entry = npc->GetEntry();
        ctx.npcName = npc->GetName();
        ctx.zoneId = npc->GetZoneId();
        ctx.areaId = npc->GetAreaId();

        if (CreatureTemplate const* tmpl = npc->GetCreatureTemplate())
            ctx.subName = tmpl->SubName;

        if (WowTime const* wowTime = GameTime::GetWowTime())
        {
            int8 hour = wowTime->GetHour();
            int8 minute = wowTime->GetMinute();
            if (hour >= 0 && minute >= 0)
            {
                char buf[16];
                snprintf(buf, sizeof(buf), "%02d:%02d", hour, minute);
                ctx.inGameTime = buf;
            }
        }

        if (Map* map = npc->GetMap())
        {
            WeatherState ws = map->GetZoneWeather(npc->GetZoneId());
            ctx.weather = FSBUtils::WeatherStateToText(ws);
        }

        if (player)
        {
            for (uint32 questId : sObjectMgr->GetCreatureQuestRelations(npc->GetEntry()))
            {
                Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
                if (!quest)
                    continue;

                if (player->GetQuestStatus(questId) != QUEST_STATUS_NONE)
                    continue;

                if (!player->CanTakeQuest(quest, false))
                    continue;

                ctx.hasQuest = true;
                ctx.questTitle = quest->GetLogTitle();
                ctx.questDescription = quest->GetQuestDescription();
                break;
            }
        }

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
