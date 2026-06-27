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

#include "Followship_bots_mail_prompts.h"
#include "Followship_bots_llamaAI.h"
#include "Followship_bots_mgr.h"
#include "Followship_bots_utils.h"

#include "DB2Stores.h"
#include "Log.h"
#include "Creature.h"
#include "Player.h"
#include "Errors.h"
#include "Common.h"

#include <rapidjson/document.h>

namespace FSBMailPrompts
{
    MailContent GenerateGoldMailContent(Creature* bot, Player* player, uint32 amount,
        std::string const& playerRequest,
        std::string const& botReply)
    {
        MailContent result;
        result.subject = "Some spare coin";
        result.body = botReply;

        if (!bot || !player || !FSBLlamaAI::IsEnabled())
            return result;

        uint32 entry = bot->GetEntry();
        FSB_Class botClass = FSBMgr::Get()->GetBotClassForEntry(entry);
        FSB_Race botRace = FSBMgr::Get()->GetBotRaceForEntry(entry);
        FSB_ChatterType botPersonality = FSBMgr::Get()->GetBotChatterTypeForEntry(entry);

        std::string botRaceStr = FSBUtils::BotRaceToString(botRace);
        std::string botClassStr = FSBUtils::BotClassToString(botClass);
        std::string botPersonalityStr = FSBUtils::ChatterTypeToString(botPersonality);

        uint32 zoneId = bot->GetZoneId();
        AreaTableEntry const* area = sAreaTableStore.LookupEntry(zoneId);
        std::string areaName = area ? area->AreaName[LOCALE_enUS] : "Unknown";

        std::string playerRaceStr = "Unknown";
        if (ChrRacesEntry const* raceEntry = sChrRacesStore.LookupEntry(player->GetRace()))
            playerRaceStr = raceEntry->Name[LOCALE_enUS];

        std::string playerClassStr = "Unknown";
        if (ChrClassesEntry const* classEntry = sChrClassesStore.LookupEntry(player->GetClass()))
            playerClassStr = classEntry->Name[LOCALE_enUS];

        uint32 level = player->GetLevel();
        std::string levelBracket;
        if (level <= 30)
            levelBracket = "low level";
        else if (level <= 60)
            levelBracket = "mid level";
        else
            levelBracket = "high level";

        std::string systemPrompt =
            "You are a World of Warcraft player named " + std::string(bot->GetName()) +
            ", a " + botRaceStr + " " + botClassStr +
            " with a " + botPersonalityStr + " personality, currently in " + areaName +
            ". You have just decided to send some spare coin/gold to a fellow adventurer via in-game mail.\n\n"
            "The recipient is " + std::string(player->GetName()) +
            ", a " + playerRaceStr + " " + playerClassStr +
            " of level " + std::to_string(level) + " (" + levelBracket + ").\n\n"
            "You MUST respond ONLY in valid JSON with exactly these fields:\n"
            "- \"subject\": a short mail subject (3 to 6 words), NO quotation marks inside.\n"
            "- \"body\": a personality relevant in-character mail body (1 to 3 sentences), NO quotation marks inside.\n\n"
            "The body MUST make sense given why the gold is being sent. Reference the player's reason naturally. "
            "Keep it brief, in-universe, and in character. Do not refer to yourself as a bot, NPC, or AI.";

        // Give the AI the actual conversation so it understands context
        std::string userPrompt =
            "The player said in chat: " + playerRequest + "\n"
            "Your chat reply was: " + botReply + "\n"
            "You are now sending " + std::to_string(amount) + " copper to " +
            std::string(player->GetName()) + " via mail.\n"
            "Write a subject and body that naturally follow from this conversation. "
            "The body should acknowledge their actual reason for asking, not invent a different one.";

        std::string aiResponse = FSBLlamaAI::GetStructuredBotResponse(systemPrompt, userPrompt);
        if (aiResponse.empty())
            return result;

        rapidjson::Document doc;
        doc.Parse(aiResponse.c_str(), aiResponse.length());

        if (doc.HasParseError() || !doc.IsObject())
        {
            TC_LOG_ERROR("scripts.fsb.llama", "FSB MailPrompts: JSON parse error: {}", aiResponse);
            return result;
        }

        if (doc.HasMember("subject") && doc["subject"].IsString())
            result.subject = doc["subject"].GetString();
        else
            TC_LOG_WARN("scripts.fsb.llama", "FSB MailPrompts: missing/invalid 'subject' field");

        if (doc.HasMember("body") && doc["body"].IsString())
            result.body = doc["body"].GetString();
        else
            TC_LOG_WARN("scripts.fsb.llama", "FSB MailPrompts: missing/invalid 'body' field");

        return result;
    }
}
