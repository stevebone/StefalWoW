/*
 * This file is part of the Stefal WoW Project.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "Followship_bots_chatter_prompts.h"
#include "Followship_bots_llamaAI.h"

#include "AI/Followship_bots_ai_base.h"
#include "AI/Followship_bots_priest.h"
#include "Config/Followship_bots_config.h"
#include "Followship_bots_mgr.h"
#include "Followship_bots_utils.h"

#include "Handlers/Followship_bots_chatter_handler.h"
#include "ObjectAccessor.h"

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

    static std::string BuildStandardSystemPrompt(Creature* bot)
    {
        uint32 entry = bot->GetEntry();
        std::string const& botName = bot->GetName();
        FSB_Class botClass = FSBMgr::Get()->GetBotClassForEntry(entry);
        FSB_Race botRace = FSBMgr::Get()->GetBotRaceForEntry(entry);
        FSB_ChatterType chatterType = FSBMgr::Get()->GetBotChatterTypeForEntry(entry);
        Gender botGender = FSBMgr::Get()->GetBotGenderForEntry(entry);

        uint32 zoneId = bot->GetZoneId();
        AreaTableEntry const* areaEntry = sAreaTableStore.LookupEntry(zoneId);
        std::string zoneName = areaEntry ? areaEntry->AreaName[LOCALE_enUS] : "Azeroth";

        return Trinity::StringFormat(
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

        FSB_Roles botRole = FSBMgr::Get()->GetRole(bot);

        Player* owner = FSBMgr::Get()->GetBotOwner(bot);
        std::string ownerName = owner ? owner->GetName() : "commander";

        std::string seedLine = FSBChatter::GetRandomReply(bot, nullptr, FSB_ChatterCategory::botAcknowledge, FSB_ChatterType::None, 0, 0);

        std::string systemPrompt = BuildStandardSystemPrompt(bot);

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
        TC_LOG_INFO("scripts.fsb.llama", "FSB LlamaAI: dispatched role acknowledge for bot {}", bot->GetName());
        std::thread([systemPrompt, userMessage, state]() {
            std::string result = FSBLlamaAI::GetBotResponse(systemPrompt, userMessage);
            std::lock_guard<std::mutex> lock(state->mutex);
            state->result = std::move(result);
            state->ready = true;
        }).detach();
    }

    static FSB_ChatterCategory GetAcknowledgeCategory(FSB_AcknowledgeContext context)
    {
        switch (context)
        {
            case FSB_AcknowledgeContext::StayCommand:    return FSB_ChatterCategory::botStay;
            case FSB_AcknowledgeContext::FollowCommand:  return FSB_ChatterCategory::botFollow;
            default:                                      return FSB_ChatterCategory::botAcknowledge;
        }
    }

    static char const* GetAcknowledgeDescription(FSB_AcknowledgeContext context)
    {
        switch (context)
        {
            case FSB_AcknowledgeContext::FollowDistanceClose:  return "follow at close distance";
            case FSB_AcknowledgeContext::FollowDistanceNormal: return "follow at normal distance";
            case FSB_AcknowledgeContext::FollowDistanceWide:   return "follow at wide distance";
            case FSB_AcknowledgeContext::FollowAngleFront:     return "follow from the front";
            case FSB_AcknowledgeContext::FollowAngleBehind:    return "follow from behind";
            case FSB_AcknowledgeContext::FollowAngleRight:   return "follow from the right";
            case FSB_AcknowledgeContext::FollowAngleLeft:    return "follow from the left";
            case FSB_AcknowledgeContext::StayCommand:        return "stay here and hold position";
            case FSB_AcknowledgeContext::FollowCommand:      return "follow again";
            default:                                         return "obey the command";
        }
    }

    void DispatchBotAcknowledge(Creature* bot, FSB_AcknowledgeContext context)
    {
        if (!bot)
            return;

        FSB_ChatterCategory category = GetAcknowledgeCategory(context);

        if (!FSBLlamaAI::IsEnabled())
        {
            FSBChatter::DemandTimedReply(bot, nullptr, category, FSB_ReplyType::Say, FSB_ChatterSource::None);
            return;
        }

        auto* ai = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!ai)
        {
            TC_LOG_WARN("scripts.fsb.llama", "FSB LlamaAI: could not get AI for bot {}, falling back to hardcoded chatter.", bot->GetName());
            FSBChatter::DemandTimedReply(bot, nullptr, category, FSB_ReplyType::Say, FSB_ChatterSource::None);
            return;
        }

        Player* owner = FSBMgr::Get()->GetBotOwner(bot);
        std::string ownerName = owner ? owner->GetName() : "commander";

        std::string seedLine = FSBChatter::GetRandomReply(bot, nullptr, category, FSB_ChatterType::None, 0, 0);

        std::string systemPrompt = BuildStandardSystemPrompt(bot);

        std::string userMessage = Trinity::StringFormat(
            "{} just told you to {}. Acknowledge the command with a brief, creative response relevant to your personality. "
            "Example style (do not copy): \"{}\"",
            ownerName,
            GetAcknowledgeDescription(context),
            seedLine.empty() ? "On it, commander." : seedLine);

        ai->llamaFallbackAction = [bot, category]() {
            FSBChatter::DemandTimedReply(bot, nullptr, category, FSB_ReplyType::Say, FSB_ChatterSource::None);
        };
        ai->pendingLlamaState = std::make_unique<FSB_BaseAI::LlamaRequestState>();
        auto* state = ai->pendingLlamaState.get();
        TC_LOG_INFO("scripts.fsb.llama", "FSB LlamaAI: dispatched acknowledge for bot {}", bot->GetName());
        std::thread([systemPrompt, userMessage, state]() {
            std::string result = FSBLlamaAI::GetBotResponse(systemPrompt, userMessage);
            std::lock_guard<std::mutex> lock(state->mutex);
            state->result = std::move(result);
            state->ready = true;
        }).detach();
    }

    void DispatchBotDismissed(Creature* bot)
    {
        if (!bot)
            return;

        if (!FSBLlamaAI::IsEnabled())
        {
            FSBChatter::DemandBotChatter(bot, nullptr, FSB_ChatterCategory::botDismissed);
            return;
        }

        auto* ai = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!ai)
        {
            TC_LOG_WARN("scripts.fsb.llama", "FSB LlamaAI: could not get AI for bot {}, falling back to hardcoded chatter.", bot->GetName());
            FSBChatter::DemandBotChatter(bot, nullptr, FSB_ChatterCategory::botDismissed);
            return;
        }

        Player* owner = FSBMgr::Get()->GetBotOwner(bot);
        std::string ownerName = owner ? owner->GetName() : "commander";

        std::string seedLine = FSBChatter::GetRandomReply(bot, nullptr, FSB_ChatterCategory::botDismissed, FSB_ChatterType::None, 0, 0);

        std::string systemPrompt = BuildStandardSystemPrompt(bot);

        std::string userMessage = Trinity::StringFormat(
            "{} is dismissing you from service. Respond with a brief, creative farewell relevant to your personality. "
            "Example style (do not copy): \"{}\"",
            ownerName,
            seedLine.empty() ? "Time's up. I'm off - good luck out there." : seedLine);

        ai->llamaFallbackAction = [bot]() {
            FSBChatter::DemandBotChatter(bot, nullptr, FSB_ChatterCategory::botDismissed);
        };
        ai->pendingLlamaState = std::make_unique<FSB_BaseAI::LlamaRequestState>();
        auto* state = ai->pendingLlamaState.get();
        TC_LOG_INFO("scripts.fsb.llama", "FSB LlamaAI: dispatched dismissal for bot {}", bot->GetName());
        std::thread([systemPrompt, userMessage, state]() {
            std::string result = FSBLlamaAI::GetBotResponse(systemPrompt, userMessage);
            std::lock_guard<std::mutex> lock(state->mutex);
            state->result = std::move(result);
            state->ready = true;
        }).detach();
    }

    void DispatchBotHired(Creature* bot, uint32 durationHours)
    {
        if (!bot)
            return;

        bool isPermanent = (durationHours == 0);
        FSB_ChatterCategory category = isPermanent ? FSB_ChatterCategory::botHiredPermanent : FSB_ChatterCategory::botHire;

        if (!FSBLlamaAI::IsEnabled())
        {
            if (isPermanent)
                FSBChatter::DemandTimedReply(bot, nullptr, category, FSB_ReplyType::Say, FSB_ChatterSource::None);
            else
            {
                std::string chatter = FSBChatter::GetRandomReply(bot, nullptr, category, FSB_ChatterType::None, 0, durationHours);
                if (!chatter.empty())
                    bot->Say(chatter, LANG_UNIVERSAL);
            }
            return;
        }

        auto* ai = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!ai)
        {
            TC_LOG_WARN("scripts.fsb.llama", "FSB LlamaAI: could not get AI for bot {}, falling back to hardcoded chatter.", bot->GetName());
            if (isPermanent)
                FSBChatter::DemandTimedReply(bot, nullptr, category, FSB_ReplyType::Say, FSB_ChatterSource::None);
            else
            {
                std::string chatter = FSBChatter::GetRandomReply(bot, nullptr, category, FSB_ChatterType::None, 0, durationHours);
                if (!chatter.empty())
                    bot->Say(chatter, LANG_UNIVERSAL);
            }
            return;
        }

        Player* owner = FSBMgr::Get()->GetBotOwner(bot);
        std::string ownerName = owner ? owner->GetName() : "commander";

        std::string seedLine = FSBChatter::GetRandomReply(bot, nullptr, category, FSB_ChatterType::None, 0, isPermanent ? 0 : durationHours);

        std::string systemPrompt = BuildStandardSystemPrompt(bot);

        std::string userMessage;
        if (isPermanent)
        {
            userMessage = Trinity::StringFormat(
                "{} just hired you as a permanent companion. Greet them with a brief, creative response relevant to your personality. "
                "Example style (do not copy): \"{}\"",
                ownerName,
                seedLine.empty() ? "Looks like we're in this together now." : seedLine);
        }
        else
        {
            std::string durationStr = durationHours == 1 ? "1 hour" : std::to_string(durationHours) + " hours";
            userMessage = Trinity::StringFormat(
                "{} just hired you for {}. Greet them with a brief, creative response relevant to your personality. "
                "Example style (do not copy): \"{}\"",
                ownerName,
                durationStr,
                seedLine.empty() ? "Alright, let's band together." : seedLine);
        }

        ai->llamaFallbackAction = [bot, category, durationHours]() {
            if (category == FSB_ChatterCategory::botHiredPermanent)
                FSBChatter::DemandTimedReply(bot, nullptr, category, FSB_ReplyType::Say, FSB_ChatterSource::None);
            else
            {
                std::string chatter = FSBChatter::GetRandomReply(bot, nullptr, category, FSB_ChatterType::None, 0, durationHours);
                if (!chatter.empty())
                    bot->Say(chatter, LANG_UNIVERSAL);
            }
        };
        ai->pendingLlamaState = std::make_unique<FSB_BaseAI::LlamaRequestState>();
        auto* state = ai->pendingLlamaState.get();
        TC_LOG_INFO("scripts.fsb.llama", "FSB LlamaAI: dispatched hire for bot {}", bot->GetName());
        std::thread([systemPrompt, userMessage, state]() {
            std::string result = FSBLlamaAI::GetBotResponse(systemPrompt, userMessage);
            std::lock_guard<std::mutex> lock(state->mutex);
            state->result = std::move(result);
            state->ready = true;
        }).detach();
    }

    void DispatchBotRecovery(Creature* bot, uint32 spellId)
    {
        if (!bot)
            return;

        Player* owner = FSBMgr::Get()->GetBotOwner(bot);
        FSB_ChatterCategory category = owner ? FSB_ChatterCategory::botOOCRecoveryHired : FSB_ChatterCategory::botOOCRecovery;

        if (!FSBLlamaAI::IsEnabled())
        {
            FSBChatter::DemandBotChatter(bot, nullptr, category, FSB_ReplyType::Say, FSB_ChatterSource::None, spellId);
            return;
        }

        auto* ai = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!ai)
        {
            TC_LOG_WARN("scripts.fsb.llama", "FSB LlamaAI: could not get AI for bot {}, falling back to hardcoded chatter.", bot->GetName());
            FSBChatter::DemandBotChatter(bot, nullptr, category, FSB_ReplyType::Say, FSB_ChatterSource::None, spellId);
            return;
        }

        std::string ownerName = owner ? owner->GetName() : "commander";

        std::string seedLine = FSBChatter::GetRandomReply(bot, nullptr, category, FSB_ChatterType::None, spellId, 0);

        std::string systemPrompt = BuildStandardSystemPrompt(bot);

        std::string spellName = spellId ? FSBSpellsUtils::GetSpellName(spellId) : "a moment of rest";

        std::string userMessage;
        if (owner)
        {
            userMessage = Trinity::StringFormat(
                "You are recovering after a tough battle, low on strength or energy, and use {} to catch your breath. "
                "Address {} with a brief, creative comment about it relevant to your personality. "
                "Example style (do not copy): \"{}\"",
                spellName,
                ownerName,
                seedLine.empty() ? "Hey, I really need a break..." : seedLine);
        }
        else
        {
            userMessage = Trinity::StringFormat(
                "You are recovering after a tough battle, low on strength or energy, and use {} to catch your breath. "
                "Make a brief, creative comment about it relevant to your personality. "
                "Example style (do not copy): \"{}\"",
                spellName,
                seedLine.empty() ? "Okay, deep breaths. I can do this." : seedLine);
        }

        ai->llamaFallbackAction = [bot, category, spellId]() {
            FSBChatter::DemandBotChatter(bot, nullptr, category, FSB_ReplyType::Say, FSB_ChatterSource::None, spellId);
        };
        ai->pendingLlamaState = std::make_unique<FSB_BaseAI::LlamaRequestState>();
        auto* state = ai->pendingLlamaState.get();
        TC_LOG_INFO("scripts.fsb.llama", "FSB LlamaAI: dispatched recovery for bot {}", bot->GetName());
        std::thread([systemPrompt, userMessage, state]() {
            std::string result = FSBLlamaAI::GetBotResponse(systemPrompt, userMessage);
            std::lock_guard<std::mutex> lock(state->mutex);
            state->result = std::move(result);
            state->ready = true;
        }).detach();
    }

    void DispatchBotBuff(Creature* bot, ObjectGuid targetGuid, uint32 spellId)
    {
        if (!bot)
            return;

        bool isSelfBuff = targetGuid == bot->GetGUID();
        FSB_ChatterCategory category = isSelfBuff ? FSB_ChatterCategory::botBuffSelf : FSB_ChatterCategory::botBuffTarget;
        FSB_ChatterType chatterType = FSB_ChatterType::None;
        if (!isSelfBuff)
            chatterType = FSBMgr::Get()->GetBotChatterTypeForEntry(bot->GetEntry());

        if (!FSBLlamaAI::IsEnabled())
        {
            if (isSelfBuff)
                FSBChatter::DemandBotChatter(bot, nullptr, category, FSB_ReplyType::Say, FSB_ChatterSource::None, spellId);
            else
            {
                Unit* target = ObjectAccessor::GetUnit(*bot, targetGuid);
                FSBChatter::DemandBotChatter(bot, target, category, FSB_ReplyType::Say, FSB_ChatterSource::Bot, spellId);
            }
            return;
        }

        auto* ai = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!ai)
        {
            TC_LOG_WARN("scripts.fsb.llama", "FSB LlamaAI: could not get AI for bot {}, falling back to hardcoded chatter.", bot->GetName());
            if (isSelfBuff)
                FSBChatter::DemandBotChatter(bot, nullptr, category, FSB_ReplyType::Say, FSB_ChatterSource::None, spellId);
            else
            {
                Unit* target = ObjectAccessor::GetUnit(*bot, targetGuid);
                FSBChatter::DemandBotChatter(bot, target, category, FSB_ReplyType::Say, FSB_ChatterSource::Bot, spellId);
            }
            return;
        }

        std::string seedLine = FSBChatter::GetRandomReply(bot, nullptr, category, chatterType, spellId, 0);

        std::string systemPrompt = BuildStandardSystemPrompt(bot);

        std::string spellName = spellId ? FSBSpellsUtils::GetSpellName(spellId) : "a spell";

        std::string userMessage;
        if (isSelfBuff)
        {
            userMessage = Trinity::StringFormat(
                "You just cast a beneficial {} on yourself. Make a brief, creative comment about it relevant to your personality. "
                "Example style (do not copy): \"{}\"",
                spellName,
                seedLine.empty() ? "Am gonna be needing this!" : seedLine);
        }
        else
        {
            Unit* targetUnit = ObjectAccessor::GetUnit(*bot, targetGuid);
            std::string targetName = targetUnit ? targetUnit->GetName() : "your ally";
            userMessage = Trinity::StringFormat(
                "You just cast a beneficial {} on {}. Make a brief, creative comment about it relevant to your personality. "
                "Example style (do not copy): \"{}\"",
                spellName,
                targetName,
                seedLine.empty() ? "Hey, no need to thank me!" : seedLine);
        }

        if (isSelfBuff)
        {
            ai->llamaFallbackAction = [bot, category, spellId]() {
                FSBChatter::DemandBotChatter(bot, nullptr, category, FSB_ReplyType::Say, FSB_ChatterSource::None, spellId);
            };
        }
        else
        {
            ai->llamaFallbackAction = [bot, targetGuid, category, spellId]() {
                Unit* target = ObjectAccessor::GetUnit(*bot, targetGuid);
                FSBChatter::DemandBotChatter(bot, target, category, FSB_ReplyType::Say, FSB_ChatterSource::Bot, spellId);
            };
        }
        ai->pendingLlamaState = std::make_unique<FSB_BaseAI::LlamaRequestState>();
        auto* state = ai->pendingLlamaState.get();
        TC_LOG_INFO("scripts.fsb.llama", "FSB LlamaAI: dispatched buff for bot {}", bot->GetName());
        std::thread([systemPrompt, userMessage, state]() {
            std::string result = FSBLlamaAI::GetBotResponse(systemPrompt, userMessage);
            std::lock_guard<std::mutex> lock(state->mutex);
            state->result = std::move(result);
            state->ready = true;
        }).detach();
    }

    void DispatchBotPotion(Creature* bot, uint32 spellId, bool isManaPotion)
    {
        if (!bot)
            return;

        FSB_ChatterCategory category = isManaPotion ? FSB_ChatterCategory::botCombatMana : FSB_ChatterCategory::botCombatHealth;

        if (!FSBLlamaAI::IsEnabled())
        {
            FSBChatter::DemandBotChatter(bot, nullptr, category, FSB_ReplyType::Say, FSB_ChatterSource::None, spellId);
            return;
        }

        auto* ai = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!ai)
        {
            TC_LOG_WARN("scripts.fsb.llama", "FSB LlamaAI: could not get AI for bot {}, falling back to hardcoded chatter.", bot->GetName());
            FSBChatter::DemandBotChatter(bot, nullptr, category, FSB_ReplyType::Say, FSB_ChatterSource::None, spellId);
            return;
        }

        std::string seedLine = FSBChatter::GetRandomReply(bot, nullptr, category, FSB_ChatterType::None, spellId, 0);

        std::string systemPrompt = BuildStandardSystemPrompt(bot);

        std::string spellName = spellId ? FSBSpellsUtils::GetSpellName(spellId) : "a potion";

        std::string userMessage;
        if (isManaPotion)
        {
            userMessage = Trinity::StringFormat(
                "You are in the middle of combat and your mana is dangerously low. "
                "You quickly drink {} to restore your magical energy. "
                "Make a brief, desperate but personality-relevant comment about it. "
                "Example style (do not copy): \"{}\"",
                spellName,
                seedLine.empty() ? "Ugh, my brain's running on empty!" : seedLine);
        }
        else
        {
            userMessage = Trinity::StringFormat(
                "You are in the middle of combat and your health is critically low. "
                "You quickly drink {} to stay alive. "
                "Make a brief, desperate but personality-relevant comment about it. "
                "Example style (do not copy): \"{}\"",
                spellName,
                seedLine.empty() ? "This is my last chance..." : seedLine);
        }

        ai->llamaFallbackAction = [bot, category, spellId]() {
            FSBChatter::DemandBotChatter(bot, nullptr, category, FSB_ReplyType::Say, FSB_ChatterSource::None, spellId);
        };
        ai->pendingLlamaState = std::make_unique<FSB_BaseAI::LlamaRequestState>();
        auto* state = ai->pendingLlamaState.get();
        TC_LOG_INFO("scripts.fsb.llama", "FSB LlamaAI: dispatched potion for bot {}", bot->GetName());
        std::thread([systemPrompt, userMessage, state]() {
            std::string result = FSBLlamaAI::GetBotResponse(systemPrompt, userMessage);
            std::lock_guard<std::mutex> lock(state->mutex);
            state->result = std::move(result);
            state->ready = true;
        }).detach();
    }

    void DispatchBotHeal(Creature* bot, ObjectGuid targetGuid, uint32 spellId)
    {
        if (!bot)
            return;

        bool isSelfHeal = targetGuid == bot->GetGUID();
        FSB_ChatterCategory category = isSelfHeal ? FSB_ChatterCategory::botHealSelf : FSB_ChatterCategory::botHealTarget;

        if (!FSBLlamaAI::IsEnabled())
        {
            if (isSelfHeal)
                FSBChatter::DemandBotChatter(bot, nullptr, category, FSB_ReplyType::Say, FSB_ChatterSource::None, spellId);
            else
            {
                Unit* target = ObjectAccessor::GetUnit(*bot, targetGuid);
                FSBChatter::DemandBotChatter(bot, target, category, FSB_ReplyType::Say, FSB_ChatterSource::Bot, spellId);
            }
            return;
        }

        auto* ai = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!ai)
        {
            TC_LOG_WARN("scripts.fsb.llama", "FSB LlamaAI: could not get AI for bot {}, falling back to hardcoded chatter.", bot->GetName());
            if (isSelfHeal)
                FSBChatter::DemandBotChatter(bot, nullptr, category, FSB_ReplyType::Say, FSB_ChatterSource::None, spellId);
            else
            {
                Unit* target = ObjectAccessor::GetUnit(*bot, targetGuid);
                FSBChatter::DemandBotChatter(bot, target, category, FSB_ReplyType::Say, FSB_ChatterSource::Bot, spellId);
            }
            return;
        }

        std::string seedLine = FSBChatter::GetRandomReply(bot, nullptr, category, FSB_ChatterType::None, spellId, 0);

        std::string systemPrompt = BuildStandardSystemPrompt(bot);

        std::string spellName = spellId ? FSBSpellsUtils::GetSpellName(spellId) : "healing magic";
        bool inCombat = bot->IsInCombat();

        std::string userMessage;
        if (isSelfHeal)
        {
            if (inCombat)
            {
                userMessage = Trinity::StringFormat(
                    "You are in the middle of combat and your health is dangerously low. "
                    "You use {} on yourself to stay alive. "
                    "Make a brief, urgent but personality-relevant comment about it. "
                    "Example style (do not copy): \"{}\"",
                    spellName,
                    seedLine.empty() ? "Not done yet." : seedLine);
            }
            else
            {
                userMessage = Trinity::StringFormat(
                    "You take a moment to heal yourself with {} while things are calm. "
                    "Make a brief, casual but personality-relevant comment about it. "
                    "Example style (do not copy): \"{}\"",
                    spellName,
                    seedLine.empty() ? "All patched up and good to go." : seedLine);
            }
        }
        else
        {
            Unit* targetUnit = ObjectAccessor::GetUnit(*bot, targetGuid);
            std::string targetName = targetUnit ? targetUnit->GetName() : "your ally";
            if (inCombat)
            {
                userMessage = Trinity::StringFormat(
                    "You are in the middle of combat and {} is wounded. "
                    "You quickly cast {} on them to keep them standing. "
                    "Make a brief, urgent but personality-relevant comment about it. "
                    "Example style (do not copy): \"{}\"",
                    targetName,
                    spellName,
                    seedLine.empty() ? "Hold still, I'm trying to fix whatever that was." : seedLine);
            }
            else
            {
                userMessage = Trinity::StringFormat(
                    "You take a moment to heal {} with {} while things are calm. "
                    "Make a brief, casual but personality-relevant comment about it. "
                    "Example style (do not copy): \"{}\"",
                    targetName,
                    spellName,
                    seedLine.empty() ? "Ah, just what you needed!" : seedLine);
            }
        }

        if (isSelfHeal)
        {
            ai->llamaFallbackAction = [bot, category, spellId]() {
                FSBChatter::DemandBotChatter(bot, nullptr, category, FSB_ReplyType::Say, FSB_ChatterSource::None, spellId);
            };
        }
        else
        {
            ai->llamaFallbackAction = [bot, targetGuid, category, spellId]() {
                Unit* target = ObjectAccessor::GetUnit(*bot, targetGuid);
                FSBChatter::DemandBotChatter(bot, target, category, FSB_ReplyType::Say, FSB_ChatterSource::Bot, spellId);
            };
        }
        ai->pendingLlamaState = std::make_unique<FSB_BaseAI::LlamaRequestState>();
        auto* state = ai->pendingLlamaState.get();
        TC_LOG_INFO("scripts.fsb.llama", "FSB LlamaAI: dispatched heal for bot {}", bot->GetName());
        std::thread([systemPrompt, userMessage, state]() {
            std::string result = FSBLlamaAI::GetBotResponse(systemPrompt, userMessage);
            std::lock_guard<std::mutex> lock(state->mutex);
            state->result = std::move(result);
            state->ready = true;
        }).detach();
    }

    void DispatchBotCombatSpell(Creature* bot, ObjectGuid targetGuid, uint32 spellId)
    {
        if (!bot)
            return;

        FSB_ChatterCategory category = FSB_ChatterCategory::botCombatSpell;

        if (!FSBLlamaAI::IsEnabled())
        {
            Unit* target = ObjectAccessor::GetUnit(*bot, targetGuid);
            FSBChatter::DemandBotChatter(bot, target, category, FSB_ReplyType::Say, FSB_ChatterSource::Bot, spellId);
            return;
        }

        auto* ai = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!ai)
        {
            TC_LOG_WARN("scripts.fsb.llama", "FSB LlamaAI: could not get AI for bot {}, falling back to hardcoded chatter.", bot->GetName());
            Unit* target = ObjectAccessor::GetUnit(*bot, targetGuid);
            FSBChatter::DemandBotChatter(bot, target, category, FSB_ReplyType::Say, FSB_ChatterSource::Bot, spellId);
            return;
        }

        std::string seedLine = FSBChatter::GetRandomReply(bot, nullptr, category, FSB_ChatterType::None, spellId, 0);

        std::string systemPrompt = BuildStandardSystemPrompt(bot);

        std::string spellName = spellId ? FSBSpellsUtils::GetSpellName(spellId) : "a spell";
        Unit* targetUnit = ObjectAccessor::GetUnit(*bot, targetGuid);
        std::string targetName = targetUnit ? targetUnit->GetName() : "your foe";

        std::string userMessage = Trinity::StringFormat(
            "You are in the middle of combat and cast offensively {} on {}. "
            "Make a brief, urgent but personality-relevant comment about it. "
            "Example style (do not copy): \"{}\"",
            spellName,
            targetName,
            seedLine.empty() ? "Take this!" : seedLine);

        ai->llamaFallbackAction = [bot, targetGuid, category, spellId]() {
            Unit* target = ObjectAccessor::GetUnit(*bot, targetGuid);
            FSBChatter::DemandBotChatter(bot, target, category, FSB_ReplyType::Say, FSB_ChatterSource::Bot, spellId);
        };
        ai->pendingLlamaState = std::make_unique<FSB_BaseAI::LlamaRequestState>();
        auto* state = ai->pendingLlamaState.get();
        TC_LOG_INFO("scripts.fsb.llama", "FSB LlamaAI: dispatched combat spell for bot {}", bot->GetName());
        std::thread([systemPrompt, userMessage, state]() {
            std::string result = FSBLlamaAI::GetBotResponse(systemPrompt, userMessage);
            std::lock_guard<std::mutex> lock(state->mutex);
            state->result = std::move(result);
            state->ready = true;
        }).detach();
    }

    void DispatchBotTargetKilled(Creature* bot, ObjectGuid victimGuid)
    {
        if (!bot)
            return;

        Player* owner = FSBMgr::Get()->GetBotOwner(bot);
        bool isHired = (owner != nullptr);
        FSB_ChatterCategory category = isHired ? FSB_ChatterCategory::targetKilledHired : FSB_ChatterCategory::targetKilled;

        if (!FSBLlamaAI::IsEnabled())
        {
            Unit* victim = ObjectAccessor::GetUnit(*bot, victimGuid);
            FSBChatter::DemandBotChatter(bot, victim, category);
            return;
        }

        auto* ai = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!ai)
        {
            TC_LOG_WARN("scripts.fsb.llama", "FSB LlamaAI: could not get AI for bot {}, falling back to hardcoded chatter.", bot->GetName());
            Unit* victim = ObjectAccessor::GetUnit(*bot, victimGuid);
            FSBChatter::DemandBotChatter(bot, victim, category);
            return;
        }

        std::string seedLine = FSBChatter::GetRandomReply(bot, nullptr, category, FSB_ChatterType::None, 0, 0);

        std::string systemPrompt = BuildStandardSystemPrompt(bot);

        Unit* victimUnit = ObjectAccessor::GetUnit(*bot, victimGuid);
        std::string victimName = victimUnit ? victimUnit->GetName() : "your foe";

        std::string userMessage;
        if (isHired)
        {
            std::string ownerName = owner->GetName();
            userMessage = Trinity::StringFormat(
                "You just landed the final blow on {}. You are fighting alongside your companion {}. "
                "Make a brief, personality-relevant comment about the kill. "
                "Example style (do not copy): \"{}\"",
                victimName,
                ownerName,
                seedLine.empty() ? "That's one less problem to worry about." : seedLine);
        }
        else
        {
            userMessage = Trinity::StringFormat(
                "You just landed the final blow on {}. "
                "Make a brief, personality-relevant comment about the kill. "
                "Example style (do not copy): \"{}\"",
                victimName,
                seedLine.empty() ? "Rest in pieces." : seedLine);
        }

        ai->llamaFallbackAction = [bot, victimGuid, category]() {
            Unit* victim = ObjectAccessor::GetUnit(*bot, victimGuid);
            FSBChatter::DemandBotChatter(bot, victim, category);
        };
        ai->pendingLlamaState = std::make_unique<FSB_BaseAI::LlamaRequestState>();
        auto* state = ai->pendingLlamaState.get();
        TC_LOG_INFO("scripts.fsb.llama", "FSB LlamaAI: dispatched target killed for bot {}", bot->GetName());
        std::thread([systemPrompt, userMessage, state]() {
            std::string result = FSBLlamaAI::GetBotResponse(systemPrompt, userMessage);
            std::lock_guard<std::mutex> lock(state->mutex);
            state->result = std::move(result);
            state->ready = true;
        }).detach();
    }

    void DispatchBotDeath(Creature* bot, ObjectGuid killerGuid)
    {
        if (!bot)
            return;

        Player* owner = FSBMgr::Get()->GetBotOwner(bot);
        bool isHired = (owner != nullptr);
        FSB_ChatterCategory category = isHired ? FSB_ChatterCategory::botDeathHired : FSB_ChatterCategory::botDeath;

        if (!FSBLlamaAI::IsEnabled())
        {
            Unit* killer = ObjectAccessor::GetUnit(*bot, killerGuid);
            FSBChatter::DemandBotChatter(bot, killer, category, FSB_ReplyType::Yell, FSB_ChatterSource::None);
            return;
        }

        auto* ai = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!ai)
        {
            TC_LOG_WARN("scripts.fsb.llama", "FSB LlamaAI: could not get AI for bot {}, falling back to hardcoded chatter.", bot->GetName());
            Unit* killer = ObjectAccessor::GetUnit(*bot, killerGuid);
            FSBChatter::DemandBotChatter(bot, killer, category, FSB_ReplyType::Yell, FSB_ChatterSource::None);
            return;
        }

        std::string seedLine = FSBChatter::GetRandomReply(bot, nullptr, category, FSB_ChatterType::None, 0, 0);

        std::string systemPrompt = BuildStandardSystemPrompt(bot);

        Unit* killerUnit = ObjectAccessor::GetUnit(*bot, killerGuid);
        std::string killerName = killerUnit ? killerUnit->GetName() : "your foe";

        std::string userMessage;
        if (isHired)
        {
            std::string ownerName = owner->GetName();
            userMessage = Trinity::StringFormat(
                "You just died at the hands of {}. {} was relying on you. "
                "Make a brief, personality-relevant dying comment. "
                "Example style (do not copy): \"{}\"",
                killerName,
                ownerName,
                seedLine.empty() ? "Well, this seems suboptimal." : seedLine);
        }
        else
        {
            userMessage = Trinity::StringFormat(
                "You just died at the hands of {}. "
                "Make a brief, personality-relevant dying comment. "
                "Example style (do not copy): \"{}\"",
                killerName,
                seedLine.empty() ? "Ah yes, the sweet embrace of death." : seedLine);
        }

        ai->llamaFallbackAction = [bot, killerGuid, category]() {
            Unit* killer = ObjectAccessor::GetUnit(*bot, killerGuid);
            FSBChatter::DemandBotChatter(bot, killer, category, FSB_ReplyType::Yell, FSB_ChatterSource::None);
        };
        ai->pendingLlamaState = std::make_unique<FSB_BaseAI::LlamaRequestState>();
        ai->pendingLlamaState->replyType = FSB_ReplyType::Yell;
        auto* state = ai->pendingLlamaState.get();
        TC_LOG_INFO("scripts.fsb.llama", "FSB LlamaAI: dispatched death for bot {}", bot->GetName());
        std::thread([systemPrompt, userMessage, state]() {
            std::string result = FSBLlamaAI::GetBotResponse(systemPrompt, userMessage);
            std::lock_guard<std::mutex> lock(state->mutex);
            state->result = std::move(result);
            state->ready = true;
        }).detach();
    }

    void DispatchBotRevived(Creature* bot)
    {
        if (!bot)
            return;

        FSB_ChatterCategory category = FSB_ChatterCategory::botRevived;

        if (!FSBLlamaAI::IsEnabled())
        {
            FSBChatter::DemandTimedReply(bot, nullptr, category, FSB_ReplyType::Say, FSB_ChatterSource::Bot);
            return;
        }

        auto* ai = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!ai)
        {
            TC_LOG_WARN("scripts.fsb.llama", "FSB LlamaAI: could not get AI for bot {}, falling back to hardcoded chatter.", bot->GetName());
            FSBChatter::DemandTimedReply(bot, nullptr, category, FSB_ReplyType::Say, FSB_ChatterSource::Bot);
            return;
        }

        std::string seedLine = FSBChatter::GetRandomReply(bot, nullptr, category, FSB_ChatterType::None, 0, 0);

        std::string systemPrompt = BuildStandardSystemPrompt(bot);

        std::string userMessage = Trinity::StringFormat(
            "You were just resurrected and are back on your feet. "
            "Make a brief, personality-relevant comment about being revived. "
            "Example style (do not copy): \"{}\"",
            seedLine.empty() ? "Back among the living." : seedLine);

        ai->llamaFallbackAction = [bot, category]() {
            FSBChatter::DemandTimedReply(bot, nullptr, category, FSB_ReplyType::Say, FSB_ChatterSource::Bot);
        };
        ai->pendingLlamaState = std::make_unique<FSB_BaseAI::LlamaRequestState>();
        auto* state = ai->pendingLlamaState.get();
        TC_LOG_INFO("scripts.fsb.llama", "FSB LlamaAI: dispatched revived for bot {}", bot->GetName());
        std::thread([systemPrompt, userMessage, state]() {
            std::string result = FSBLlamaAI::GetBotResponse(systemPrompt, userMessage);
            std::lock_guard<std::mutex> lock(state->mutex);
            state->result = std::move(result);
            state->ready = true;
        }).detach();
    }

    void DispatchBotRevivedTarget(Creature* healer, ObjectGuid targetGuid)
    {
        if (!healer)
            return;

        FSB_ChatterCategory category = FSB_ChatterCategory::botRevivedTarget;

        if (!FSBLlamaAI::IsEnabled())
        {
            Unit* target = ObjectAccessor::GetUnit(*healer, targetGuid);
            FSBChatter::DemandBotChatter(healer, target, category, FSB_ReplyType::Say, FSB_ChatterSource::None, 0);
            return;
        }

        auto* ai = dynamic_cast<FSB_BaseAI*>(healer->AI());
        if (!ai)
        {
            TC_LOG_WARN("scripts.fsb.llama", "FSB LlamaAI: could not get AI for healer {}, falling back to hardcoded chatter.", healer->GetName());
            Unit* target = ObjectAccessor::GetUnit(*healer, targetGuid);
            FSBChatter::DemandBotChatter(healer, target, category, FSB_ReplyType::Say, FSB_ChatterSource::None, 0);
            return;
        }

        std::string seedLine = FSBChatter::GetRandomReply(healer, nullptr, category, FSB_ChatterType::None, 0, 0);

        std::string systemPrompt = BuildStandardSystemPrompt(healer);

        Unit* targetUnit = ObjectAccessor::GetUnit(*healer, targetGuid);
        std::string targetName = targetUnit ? targetUnit->GetName() : "your ally";

        std::string userMessage = Trinity::StringFormat(
            "You just resurrected {} back to life. "
            "Make a brief, personality-relevant comment about the revival. "
            "Example style (do not copy): \"{}\"",
            targetName,
            seedLine.empty() ? "Rise again, friend." : seedLine);

        ai->llamaFallbackAction = [healer, targetGuid, category]() {
            Unit* target = ObjectAccessor::GetUnit(*healer, targetGuid);
            FSBChatter::DemandBotChatter(healer, target, category, FSB_ReplyType::Say, FSB_ChatterSource::None, 0);
        };
        ai->pendingLlamaState = std::make_unique<FSB_BaseAI::LlamaRequestState>();
        auto* state = ai->pendingLlamaState.get();
        TC_LOG_INFO("scripts.fsb.llama", "FSB LlamaAI: dispatched revived target for healer {}", healer->GetName());
        std::thread([systemPrompt, userMessage, state]() {
            std::string result = FSBLlamaAI::GetBotResponse(systemPrompt, userMessage);
            std::lock_guard<std::mutex> lock(state->mutex);
            state->result = std::move(result);
            state->ready = true;
        }).detach();
    }

    void DispatchBotRevivedSelf(Creature* bot)
    {
        if (!bot)
            return;

        FSB_ChatterCategory category = FSB_ChatterCategory::botRevivedSelf;

        if (!FSBLlamaAI::IsEnabled())
        {
            FSBChatter::DemandTimedReply(bot, nullptr, category, FSB_ReplyType::Say, FSB_ChatterSource::Bot);
            return;
        }

        auto* ai = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!ai)
        {
            TC_LOG_WARN("scripts.fsb.llama", "FSB LlamaAI: could not get AI for bot {}, falling back to hardcoded chatter.", bot->GetName());
            FSBChatter::DemandTimedReply(bot, nullptr, category, FSB_ReplyType::Say, FSB_ChatterSource::Bot);
            return;
        }

        std::string seedLine = FSBChatter::GetRandomReply(bot, nullptr, category, FSB_ChatterType::None, 0, 0);

        std::string systemPrompt = BuildStandardSystemPrompt(bot);

        std::string userMessage = Trinity::StringFormat(
            "You just used your own power to resurrect yourself from death. "
            "Make a brief, proud personality-relevant comment about your self-revival. "
            "Example style (do not copy): \"{}\"",
            seedLine.empty() ? "I don't wait for resurrections. I MAKE them." : seedLine);

        ai->llamaFallbackAction = [bot, category]() {
            FSBChatter::DemandTimedReply(bot, nullptr, category, FSB_ReplyType::Say, FSB_ChatterSource::Bot);
        };
        ai->pendingLlamaState = std::make_unique<FSB_BaseAI::LlamaRequestState>();
        auto* state = ai->pendingLlamaState.get();
        TC_LOG_INFO("scripts.fsb.llama", "FSB LlamaAI: dispatched revived self for bot {}", bot->GetName());
        std::thread([systemPrompt, userMessage, state]() {
            std::string result = FSBLlamaAI::GetBotResponse(systemPrompt, userMessage);
            std::lock_guard<std::mutex> lock(state->mutex);
            state->result = std::move(result);
            state->ready = true;
        }).detach();
    }
}
