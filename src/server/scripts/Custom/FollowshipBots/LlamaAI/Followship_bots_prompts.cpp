/*
 * This file is part of the Stefal WoW Project.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "Followship_bots_prompts.h"
#include "Followship_bots_llamaAI.h"

#include "AI/Followship_bots_ai_base.h"
#include "AI/Followship_bots_priest.h"
#include "Config/Followship_bots_config.h"
#include "Followship_bots_mgr.h"
#include "Followship_bots_utils.h"

#include "Handlers/Followship_bots_chatter_handler.h"

#include "DB2Stores.h"
#include "Log.h"
#include "StringFormat.h"

#include <mutex>
#include <random>
#include <thread>

namespace FSBLlamaPrompts
{
    static std::string PickIntroSet(std::string const& botName, std::string const& zoneName)
    {
        char const* introSets[] =
        {
            "- Vary how you introduce yourself. Examples: 'My name is...', 'I am called...', '{}, at your service...', 'Folks around here know me as...'",
            "- Vary how you introduce yourself. Examples: 'They call me {}...', '{} is my name...', 'You may call me {}...', 'I go by the name {}...'",
            "- Vary how you introduce yourself. Examples: 'Greetings, I am {}...', 'Well met! {} here...', 'The name\'s {}...', '{} - that\'s the name they gave me...'",
            "- Vary how you introduce yourself. Examples: 'A pleasure. I am {}...', 'If you must know, I am {}...', '{}, wanderer of {z}...', 'Some know me as {}...'"
        };

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, 3);
        std::string result = introSets[dist(gen)];

        size_t pos = 0;
        while ((pos = result.find("{}", pos)) != std::string::npos)
        {
            result.replace(pos, 2, botName);
            pos += botName.length();
        }

        pos = 0;
        while ((pos = result.find("{z}", pos)) != std::string::npos)
        {
            result.replace(pos, 3, zoneName);
            pos += zoneName.length();
        }

        return result;
    }

    void DispatchBotInfoRequest(Creature* bot)
    {
        if (!bot)
            return;

        if (!FSBLlamaAI::IsEnabled())
        {
            bot->Say(FSB_GOSSIP_ITEM_PRIEST_INFO, LANG_UNIVERSAL);
            return;
        }

        auto* ai = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!ai)
        {
            TC_LOG_WARN("scripts.fsb.llama", "FSB LlamaAI: could not get AI for bot {}, falling back to hardcoded string.", bot->GetName());
            bot->Say(FSB_GOSSIP_ITEM_PRIEST_INFO, LANG_UNIVERSAL);
            return;
        }

        uint32 entry = bot->GetEntry();
        std::string const& botName = bot->GetName();
        FSB_Class botClass = FSBMgr::Get()->GetBotClassForEntry(entry);
        FSB_Race botRace = FSBMgr::Get()->GetBotRaceForEntry(entry);
        FSB_ChatterType chatterType = FSBMgr::Get()->GetBotChatterTypeForEntry(entry);
        Gender botGender = FSBMgr::Get()->GetBotGenderForEntry(entry);

        uint32 zoneId = bot->GetZoneId();
        AreaTableEntry const* areaEntry = sAreaTableStore.LookupEntry(zoneId);
        std::string zoneName = areaEntry ? areaEntry->AreaName[LOCALE_enUS] : "Azeroth";

        /* using FSBUtils::ChatterTypeToString / GenderToString */

        std::string chosenIntro = PickIntroSet(botName, zoneName);

        std::string systemPrompt = Trinity::StringFormat(
            "You are a character in World of Warcraft named {}.\n"
            "You are a {} {} {} currently in {}.\n"
            "Your personality is {}.\n\n"
            "Rules:\n"
            "- Reply in first person, staying fully in the game world.\n"
            "- NEVER refer to yourself as an NPC, bot, AI, or game character.\n"
            "{}\n"
            "- Do not copy the examples above word-for-word. Invent your own creative way of introducing yourself.\n"
            "- Keep your reply to 2 or 3 sentences but never more.\n"
            "- The current zone ({}) should naturally appear in your reply.",
            botName,
            FSBUtils::GenderToString(botGender),
            FSBUtils::BotRaceToString(botRace),
            FSBUtils::BotClassToString(botClass),
            zoneName,
            FSBUtils::ChatterTypeToString(chatterType),
            chosenIntro,
            zoneName);

        ai->llamaFallbackAction = [bot]() {
            bot->Say(FSB_GOSSIP_ITEM_PRIEST_INFO, LANG_UNIVERSAL);
        };
        ai->pendingLlamaState = std::make_unique<FSB_BaseAI::LlamaRequestState>();
        auto* state = ai->pendingLlamaState.get();
        TC_LOG_INFO("scripts.fsb.llama", "FSB LlamaAI: dispatched async request for bot {}", botName);
        std::thread([systemPrompt, state]() {
            std::string result = FSBLlamaAI::GetBotResponse(systemPrompt,
                "Tell me something about yourself!");
            std::lock_guard<std::mutex> lock(state->mutex);
            state->result = std::move(result);
            state->ready = true;
        }).detach();
    }

    void DispatchBotRoleAcknowledge(Creature* bot)
    {
        if (!bot)
            return;

        if (!FSBLlamaAI::IsEnabled())
        {
            FSBChatter::DemandTimedReply(bot, nullptr, FSB_ChatterCategory::botAcknowledge, FSB_ReplyType::Say, FSB_ChatterSource::None);
            return;
        }

        auto* ai = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!ai)
        {
            TC_LOG_WARN("scripts.fsb.llama", "FSB LlamaAI: could not get AI for bot {}, falling back to hardcoded chatter.", bot->GetName());
            FSBChatter::DemandTimedReply(bot, nullptr, FSB_ChatterCategory::botAcknowledge, FSB_ReplyType::Say, FSB_ChatterSource::None);
            return;
        }

        uint32 entry = bot->GetEntry();
        std::string const& botName = bot->GetName();
        FSB_Class botClass = FSBMgr::Get()->GetBotClassForEntry(entry);
        FSB_Race botRace = FSBMgr::Get()->GetBotRaceForEntry(entry);
        FSB_ChatterType chatterType = FSBMgr::Get()->GetBotChatterTypeForEntry(entry);
        Gender botGender = FSBMgr::Get()->GetBotGenderForEntry(entry);
        FSB_Roles botRole = FSBMgr::Get()->GetRole(bot);

        uint32 zoneId = bot->GetZoneId();
        AreaTableEntry const* areaEntry = sAreaTableStore.LookupEntry(zoneId);
        std::string zoneName = areaEntry ? areaEntry->AreaName[LOCALE_enUS] : "Azeroth";

        Player* owner = FSBMgr::Get()->GetBotOwner(bot);
        std::string ownerName = owner ? owner->GetName() : "commander";

        /* using FSBUtils::ChatterTypeToString / GenderToString */

        std::string seedLine = FSBChatter::GetRandomReply(bot, nullptr, FSB_ChatterCategory::botAcknowledge, FSB_ChatterType::None, 0, 0);

        std::string systemPrompt = Trinity::StringFormat(
            "You are a character in World of Warcraft named {}.\n"
            "You are a {} {} {} currently in {}.\n"
            "Your personality is {}.\n\n"
            "Rules:\n"
            "- Reply in first person, staying fully in the game world.\n"
            "- NEVER refer to yourself as an NPC, bot, AI, or game character.\n"
            "- Keep your reply to 1 sentence only.\n"
            "- Do not copy the example below word-for-word. Invent your own creative variation.\n"
            "- The current zone ({}) may naturally appear in your reply.",
            botName,
            FSBUtils::GenderToString(botGender),
            FSBUtils::BotRaceToString(botRace),
            FSBUtils::BotClassToString(botClass),
            zoneName,
            FSBUtils::ChatterTypeToString(chatterType),
            zoneName);

        std::string userMessage = Trinity::StringFormat(
            "{} just assigned you the {} role. Acknowledge the command with a brief, creative response relevant to your personality. "
            "Example style (do not copy): \"{}\"",
            ownerName,
            FSBUtils::BotRoleToString(botRole),
            seedLine.empty() ? "On it, commander." : seedLine);

        ai->llamaFallbackAction = [bot]() {
            FSBChatter::DemandTimedReply(bot, nullptr, FSB_ChatterCategory::botAcknowledge, FSB_ReplyType::Say, FSB_ChatterSource::None);
        };
        ai->pendingLlamaState = std::make_unique<FSB_BaseAI::LlamaRequestState>();
        auto* state = ai->pendingLlamaState.get();
        TC_LOG_INFO("scripts.fsb.llama", "FSB LlamaAI: dispatched role acknowledge for bot {}", botName);
        std::thread([systemPrompt, userMessage, state]() {
            std::string result = FSBLlamaAI::GetBotResponse(systemPrompt, userMessage);
            std::lock_guard<std::mutex> lock(state->mutex);
            state->result = std::move(result);
            state->ready = true;
        }).detach();
    }
}
