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

#include "GenAI_channel_prompts.h"
#include "Followship_bots_chat_handler.h"
#include "GenAI_client.h"
#include "Followship_bots_mgr.h"
#include "Followship_bots_utils.h"
#include "Followship_bots_ai_base.h"

#include "DB2Stores.h"
#include "ItemTemplate.h"
#include "ObjectMgr.h"
#include "Log.h"
#include "Creature.h"
#include "Containers.h"
#include "Weather.h"

#include <algorithm>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace FSBChannelPrompts
{
    static bool IsGoldRequest(std::string const& msg)
    {
        std::string lower = msg;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        static const std::vector<std::string> keywords = { "gold", "coin", "copper", "money", "spare", "lend", "borrow", "broke", "poor", "send", "mail" };
        for (auto const& kw : keywords)
            if (lower.find(kw) != std::string::npos)
                return true;
        return false;
    }
    // ------------------------------------------------------------------
    //  Trade item cache
    // ------------------------------------------------------------------
    struct TradeItemCache
    {
        std::vector<uint32> allItems;
        bool initialized = false;
    };

    static TradeItemCache s_tradeItemCache;
    static std::once_flag s_tradeCacheInitFlag;

    static void InitTradeItemCache()
    {
        for (ItemEntry const* entry : sItemStore)
        {
            if (!entry)
                continue;

            ItemSparseEntry const* sparse = sItemSparseStore.LookupEntry(entry->ID);
            if (!sparse)
                continue;

            // Only sellable items (BindNone or BoE)
            if (sparse->Bonding != BIND_NONE && sparse->Bonding != BIND_ON_EQUIP)
                continue;

            // Skip conjured / quest / deprecated / no-value items
            if (sparse->Flags[0] & ITEM_FLAG_CONJURED)
                continue;
            if (sparse->Flags[0] & ITEM_FLAG_DEPRECATED)
                continue;
            if (entry->ClassID == ITEM_CLASS_QUEST)
                continue;
            if (sparse->SellPrice == 0 && sparse->BuyPrice == 0)
                continue;

            s_tradeItemCache.allItems.push_back(entry->ID);
        }

        s_tradeItemCache.initialized = true;

        TC_LOG_DEBUG("scripts.fsb.chatter",
            "FSB TradeItemCache: built with {} items", s_tradeItemCache.allItems.size());
    }

    // ------------------------------------------------------------------
    //  LFG dungeon cache
    // ------------------------------------------------------------------
    struct LFGDungeonEntry
    {
        uint32 id = 0;
        std::string name;
        bool isRaid = false;
    };

    struct LFGCache
    {
        std::vector<LFGDungeonEntry> dungeons;
        std::vector<LFGDungeonEntry> raids;
        bool initialized = false;
    };

    static LFGCache s_lfgCache;
    static std::once_flag s_lfgCacheInitFlag;

    static bool MatchesBotFaction(int8 lfgFaction, Team botTeam)
    {
        if (lfgFaction == -1)
            return true;

        int8 botFaction = -1;
        if (botTeam == Team::ALLIANCE)
            botFaction = 1;
        else if (botTeam == Team::HORDE)
            botFaction = 0;

        return lfgFaction == botFaction;
    }

    static void InitLFGCache(Creature* bot)
    {
        Team botTeam = FSBUtils::GetTeamFromFSBRace(FSBMgr::Get()->GetBotRaceForEntry(bot->GetEntry()));

        for (::LFGDungeonsEntry const* entry : sLFGDungeonsStore)
        {
            if (!entry)
                continue;

            char const* name = entry->Name[LOCALE_enUS];
            if (!name || !name[0])
                continue;

            if (!MatchesBotFaction(entry->Faction, botTeam))
                continue;

            bool isRaid = false;
            if (entry->TypeID == 2) // LFG_TYPE_RAID
                isRaid = true;
            else if (entry->TypeID == 1 || entry->TypeID == 5) // Dungeon or Heroic
                isRaid = false;
            else
                continue;

            LFGDungeonEntry e;
            e.id = entry->ID;
            e.name = name;
            e.isRaid = isRaid;

            if (isRaid)
                s_lfgCache.raids.push_back(e);
            else
                s_lfgCache.dungeons.push_back(e);
        }

        s_lfgCache.initialized = true;

        TC_LOG_DEBUG("scripts.fsb.chatter",
            "FSB LFGCache: built with {} dungeons and {} raids", s_lfgCache.dungeons.size(), s_lfgCache.raids.size());
    }

    // ------------------------------------------------------------------
    //  Helper: pick random item
    // ------------------------------------------------------------------
    static uint32 GetRandomTradeItem()
    {
        std::call_once(s_tradeCacheInitFlag, InitTradeItemCache);

        if (s_tradeItemCache.allItems.empty())
            return 0;

        return s_tradeItemCache.allItems[urand(0, static_cast<uint32>(s_tradeItemCache.allItems.size()) - 1)];
    }

    // ------------------------------------------------------------------
    //  Helper: pick random dungeon
    // ------------------------------------------------------------------
    static LFGDungeonEntry const* GetRandomLFGDungeon(Creature* bot)
    {
        std::call_once(s_lfgCacheInitFlag, [bot]() { InitLFGCache(bot); });

        uint32 total = static_cast<uint32>(s_lfgCache.dungeons.size() + s_lfgCache.raids.size());
        if (total == 0)
            return nullptr;

        // 60% dungeon, 40% raid (proportional when one pool is empty)
        bool pickDungeon = true;
        if (!s_lfgCache.dungeons.empty() && !s_lfgCache.raids.empty())
            pickDungeon = urand(0, 99) < 60;
        else if (s_lfgCache.dungeons.empty())
            pickDungeon = false;

        if (pickDungeon)
            return &s_lfgCache.dungeons[urand(0, static_cast<uint32>(s_lfgCache.dungeons.size()) - 1)];
        else
            return &s_lfgCache.raids[urand(0, static_cast<uint32>(s_lfgCache.raids.size()) - 1)];
    }

    // ------------------------------------------------------------------
    //  Trade message via GenAI
    // ------------------------------------------------------------------
    std::string GenerateTradeMessage(Creature* bot)
    {
        if (!FSBGenAI::IsEnabled())
            return "";

        if (!bot)
            return "";

        uint32 itemId = GetRandomTradeItem();
        if (!itemId)
            return "";

        std::string itemLink = FSBChat::BuildItemLink(itemId, bot->GetLevel());
        if (itemLink.empty())
            return "";

        // Get item name and stack size for the prompt
        ItemSparseEntry const* sparse = sItemSparseStore.LookupEntry(itemId);
        std::string itemName = "something";
        int32 stackSize = 1;
        if (sparse)
        {
            char const* name = sparse->Display[LOCALE_enUS];
            if (name && name[0])
                itemName = name;
            stackSize = sparse->Stackable;
        }

        // Quantity
        std::string quantityStr;
        if (stackSize > 1)
        {
            int32 quantity = urand(1, std::min(stackSize, 20));
            quantityStr = std::to_string(quantity);
        }

        // Intent: 45% selling, 45% buying, 10% free
        uint32 intentRoll = urand(0, 99);
        std::string intent;
        std::vector<std::string> const* pool = nullptr;

        if (intentRoll < 45)
        {
            intent = "selling";
            pool = &TradeSellingExamples;
        }
        else if (intentRoll < 90)
        {
            intent = "buying";
            pool = &TradeBuyingExamples;
        }
        else
        {
            intent = "giving away for free";
            pool = &TradeFreeExamples;
        }

        // Bot location
        std::string areaName = "around here";
        if (AreaTableEntry const* area = sAreaTableStore.LookupEntry(bot->GetAreaId()))
            if (char const* name = area->AreaName[LOCALE_enUS])
                if (name[0])
                    areaName = name;

        // Pick 2 random examples from the correct intent pool
        std::string example1 = Trinity::Containers::SelectRandomContainerElement(*pool);
        std::string example2 = Trinity::Containers::SelectRandomContainerElement(*pool);
        while (example2 == example1 && pool->size() > 1)
            example2 = Trinity::Containers::SelectRandomContainerElement(*pool);

        // Build prompts
        std::string systemPrompt =
            "You are a World of Warcraft player typing a short message in the Trade chat channel. "
            "Write exactly ONE short sentence (5 to 12 words). Be natural and varied. "
            "You MUST use the exact placeholder [ITEM] where the item should appear. "
            "Do NOT write the raw item name; use [ITEM] instead. "
            "You may mention your current location using [LOCATION] for flavor. "
            "Do NOT add quotation marks. Do NOT explain yourself. "
            "Examples (for reference only, do not copy verbatim): " +
            example1 + ", " + example2 + ".";

        std::string userPrompt = "Item name: " + itemName + "\n";
        if (!quantityStr.empty())
            userPrompt += "Quantity: " + quantityStr + "\n";
        userPrompt += "Intent: " + intent + "\n";
        userPrompt += "Your location: " + areaName;

        std::string aiResponse = FSBGenAI::GetBotResponse(systemPrompt, userPrompt);
        if (aiResponse.empty())
            return "";

        // Step 1: Replace [ITEM] placeholder if present
        size_t itemPos = aiResponse.find("[ITEM]");
        if (itemPos == std::string::npos)
            itemPos = aiResponse.find("[item]");
        if (itemPos != std::string::npos)
        {
            aiResponse.replace(itemPos, 6, itemLink);
        }
        else
        {
            // Step 2: AI wrote the raw item name instead of placeholder
            // Replace the exact item name (case-insensitive) with the clickable link
            std::string lowerResponse = aiResponse;
            std::string lowerName = itemName;
            for (char& c : lowerResponse) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            for (char& c : lowerName)     c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

            size_t namePos = lowerResponse.find(lowerName);
            if (namePos != std::string::npos)
                aiResponse.replace(namePos, itemName.length(), itemLink);
            else
                return ""; // Neither placeholder nor raw name found; discard response
        }

        size_t locPos = aiResponse.find("[LOCATION]");
        if (locPos == std::string::npos)
            locPos = aiResponse.find("[location]");
        if (locPos != std::string::npos)
            aiResponse.replace(locPos, 10, areaName);

        TC_LOG_DEBUG("scripts.fsb.chatter", "FSB TradeMessage (AI): {}", aiResponse);
        return aiResponse;
    }

    // ------------------------------------------------------------------
    //  LFG message via GenAI
    // ------------------------------------------------------------------
    std::string GenerateLFGMessage(Creature* bot)
    {
        if (!FSBGenAI::IsEnabled())
            return "";

        if (!bot)
            return "";

        LFGDungeonEntry const* dungeon = GetRandomLFGDungeon(bot);
        if (!dungeon)
            return "";

        // Bot class and role context
        FSB_Class botClass = FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry());
        FSB_Roles botRole = FSBMgr::Get()->GetRole(bot);
        std::string classStr = FSBUtils::BotClassToString(botClass);
        std::string roleStr = FSBUtils::BotRoleToString(botRole);

        std::string typeStr = dungeon->isRaid ? "Raid" : "Dungeon";

        // Pick 2 random examples from the appropriate pool
        std::vector<std::string> const& pool = dungeon->isRaid ? LFGRaidExamples : LFGDungeonExamples;
        std::string example1 = Trinity::Containers::SelectRandomContainerElement(pool);
        std::string example2 = Trinity::Containers::SelectRandomContainerElement(pool);
        while (example2 == example1 && pool.size() > 1)
            example2 = Trinity::Containers::SelectRandomContainerElement(pool);

        // Replace [CLASS] in examples with actual class
        size_t classPos = example1.find("[CLASS]");
        if (classPos != std::string::npos)
            example1.replace(classPos, 7, classStr);
        classPos = example2.find("[CLASS]");
        if (classPos != std::string::npos)
            example2.replace(classPos, 7, classStr);

        // Build prompts
        std::string systemPrompt =
            "You are a World of Warcraft player typing a short message in the Looking For Group chat channel. "
            "Write exactly ONE short sentence (5 to 12 words). Be natural and varied. "
            "Use the exact placeholder [DUNGEON] where the dungeon/raid name should appear. "
            "Do NOT add quotation marks. Do NOT explain yourself. "
            "Examples (for reference only, do not copy verbatim): " +
            example1 + ", " + example2 + ".";

        std::string userPrompt =
            "Dungeon: " + dungeon->name + "\n"
            "Type: " + typeStr + "\n"
            "Your class: " + classStr + "\n"
            "Your role: " + roleStr;

        std::string aiResponse = FSBGenAI::GetBotResponse(systemPrompt, userPrompt);
        if (aiResponse.empty())
            return "";

        // Replace placeholder with actual dungeon name
        size_t pos = aiResponse.find("[DUNGEON]");
        if (pos != std::string::npos)
            aiResponse.replace(pos, 9, dungeon->name);

        TC_LOG_DEBUG("scripts.fsb.chatter", "FSB LFGMessage (AI): {}", aiResponse);
        return aiResponse;
    }

    // ------------------------------------------------------------------
    //  General chat topic system
    // ------------------------------------------------------------------
    enum class GeneralTopic
    {
        ZoneCommentary,   // 15%
        LoreMemes,        // 15%
        ClassBanter,      // 15%
        NPCCommentary,    // 10%
        AchievementBrag,  // 10%
        PvPBattleground,  // 10%
        WorldObservation, // 5%
        TimeOfDay,        // 5%
        Weather,          // 5%
        SocialGuild       // 10%
    };

    GeneralTopic PickRandomTopic()
    {
        uint32 roll = urand(0, 99);
        if (roll < 15)  return GeneralTopic::ZoneCommentary;
        if (roll < 30)  return GeneralTopic::LoreMemes;
        if (roll < 45)  return GeneralTopic::ClassBanter;
        if (roll < 55)  return GeneralTopic::NPCCommentary;
        if (roll < 65)  return GeneralTopic::AchievementBrag;
        if (roll < 75)  return GeneralTopic::PvPBattleground;
        if (roll < 80)  return GeneralTopic::WorldObservation;
        if (roll < 85)  return GeneralTopic::TimeOfDay;
        if (roll < 90)  return GeneralTopic::Weather;
        return GeneralTopic::SocialGuild;
    }

    static std::vector<std::string> const* GetTimeOfDayPool(int8 hour)
    {
        if (hour >= 5 && hour <= 7)   return &GeneralDawnExamples;
        if (hour >= 8 && hour <= 11)  return &GeneralMorningExamples;
        if (hour >= 12 && hour <= 16) return &GeneralAfternoonExamples;
        if (hour >= 17 && hour <= 19) return &GeneralEveningExamples;
        return &GeneralNightExamples;
    }

    static std::vector<std::string> const* GetWeatherPool(uint32 weatherStateRaw)
    {
        switch (static_cast<WeatherState>(weatherStateRaw))
        {
            case WEATHER_STATE_FINE:              return &GeneralWeatherFineExamples;
            case WEATHER_STATE_FOG:               return &GeneralWeatherFogExamples;
            case WEATHER_STATE_DRIZZLE:           return &GeneralWeatherDrizzleExamples;
            case WEATHER_STATE_LIGHT_RAIN:        return &GeneralWeatherLightRainExamples;
            case WEATHER_STATE_MEDIUM_RAIN:       return &GeneralWeatherMediumRainExamples;
            case WEATHER_STATE_HEAVY_RAIN:        return &GeneralWeatherHeavyRainExamples;
            case WEATHER_STATE_LIGHT_SNOW:        return &GeneralWeatherLightSnowExamples;
            case WEATHER_STATE_MEDIUM_SNOW:       return &GeneralWeatherMediumSnowExamples;
            case WEATHER_STATE_HEAVY_SNOW:        return &GeneralWeatherHeavySnowExamples;
            case WEATHER_STATE_LIGHT_SANDSTORM:   return &GeneralWeatherLightSandstormExamples;
            case WEATHER_STATE_MEDIUM_SANDSTORM:  return &GeneralWeatherMediumSandstormExamples;
            case WEATHER_STATE_HEAVY_SANDSTORM:   return &GeneralWeatherHeavySandstormExamples;
            case WEATHER_STATE_THUNDERS:          return &GeneralWeatherThunderExamples;
            case WEATHER_STATE_BLACKRAIN:         return &GeneralWeatherBlackRainExamples;
            case WEATHER_STATE_BLACKSNOW:         return &GeneralWeatherBlackSnowExamples;
            default:                              return &GeneralWeatherFineExamples;
        }
    }

    std::string GetRandomAchievementName()
    {
        // Lazy-init cache of valid achievement titles
        static std::vector<std::string> s_achievementNames;
        static std::once_flag s_achieveFlag;
        std::call_once(s_achieveFlag, []()
        {
            for (uint32 i = 0; i < sAchievementStore.GetNumRows(); ++i)
            {
                if (AchievementEntry const* ach = sAchievementStore.LookupEntry(i))
                {
                    char const* title = ach->Title[LOCALE_enUS];
                    if (title && title[0] && title[1] && title[2] && title[3])
                        s_achievementNames.emplace_back(title);
                }
            }
        });

        if (s_achievementNames.empty())
            return "a rare achievement";

        return s_achievementNames[urand(0, static_cast<uint32>(s_achievementNames.size()) - 1)];
    }

    std::string GetRandomFamousNPC()
    {
        static const std::vector<std::string> FamousNPCs =
        {
            "Thrall", "Jaina Proudmoore", "Sylvanas Windrunner",
            "Illidan Stormrage", "Varian Wrynn", "Anduin Wrynn",
            "Tyrande Whisperwind", "Genn Greymane", "Bolvar Fordragon",
            "Khadgar", "Medivh", "Arthas Menethil",
            "Malfurion Stormrage", "Garrosh Hellscream", "Vol'jin"
        };
        return FamousNPCs[urand(0, static_cast<uint32>(FamousNPCs.size()) - 1)];
    }

    std::string GenerateGeneralMessage(BotChatContext const& ctx)
    {
        if (!FSBGenAI::IsEnabled())
            return "";

        if (!ctx.entry)
            return "";

        GeneralTopic topic = PickRandomTopic();

        // Bot context
        FSB_Class botClass = FSBMgr::Get()->GetBotClassForEntry(ctx.entry);
        std::string classStr = FSBUtils::BotClassToString(botClass);

        std::string areaName = "around here";
        if (AreaTableEntry const* area = sAreaTableStore.LookupEntry(ctx.areaId))
            if (char const* name = area->AreaName[LOCALE_enUS])
                if (name[0])
                    areaName = name;

        // Placeholder tracking
        std::string placeholder;
        std::string replacement;
        std::string poolDesc;
        std::vector<std::string> const* pool = nullptr;

        switch (topic)
        {
        case GeneralTopic::ZoneCommentary:
            pool = &GeneralZoneExamples;
            poolDesc = "comment about your current location";
            placeholder = "{area}";
            replacement = areaName;
            break;

        case GeneralTopic::LoreMemes:
            pool = &GeneralLoreExamples;
            poolDesc = "reference a famous WoW moment or meme";
            break;

        case GeneralTopic::ClassBanter:
            pool = &GeneralClassExamples;
            poolDesc = "make a light comment about your class";
            placeholder = "[CLASS]";
            replacement = classStr;
            break;

        case GeneralTopic::NPCCommentary:
            pool = &GeneralNPCExamples;
            poolDesc = "share an opinion about a famous character";
            break;

        case GeneralTopic::AchievementBrag:
            pool = &GeneralAchievementExamples;
            poolDesc = "mention an achievement";
            placeholder = "[ACHIEVEMENT]";
            replacement = GetRandomAchievementName();
            break;

        case GeneralTopic::PvPBattleground:
            pool = &GeneralPvPExamples;
            poolDesc = "reminisce about old PvP battles";
            break;

        case GeneralTopic::WorldObservation:
            pool = &GeneralWorldExamples;
            poolDesc = "comment on the surroundings or weather";
            placeholder = "{area}";
            replacement = areaName;
            break;

        case GeneralTopic::SocialGuild:
            pool = &GeneralSocialExamples;
            poolDesc = "make casual social chat";
            break;

        case GeneralTopic::TimeOfDay:
            pool = GetTimeOfDayPool(ctx.inGameHour);
            if (!pool || pool->empty())
                pool = &GeneralWorldExamples;
            poolDesc = "comment about the current time of day";
            break;

        case GeneralTopic::Weather:
            pool = GetWeatherPool(ctx.weatherStateRaw);
            if (!pool || pool->empty())
                pool = &GeneralWorldExamples;
            poolDesc = "comment about the current weather";
            break;
        }

        if (!pool || pool->empty())
            return "";

        // Pick 2 random examples from the pool
        std::string example1 = Trinity::Containers::SelectRandomContainerElement(*pool);
        std::string example2 = Trinity::Containers::SelectRandomContainerElement(*pool);
        while (example2 == example1 && pool->size() > 1)
            example2 = Trinity::Containers::SelectRandomContainerElement(*pool);

        // Pre-fill placeholders in examples for the prompt (so AI sees real data)
        if (!placeholder.empty())
        {
            size_t pos = example1.find(placeholder);
            if (pos != std::string::npos)
                example1.replace(pos, placeholder.length(), replacement);
            pos = example2.find(placeholder);
            if (pos != std::string::npos)
                example2.replace(pos, placeholder.length(), replacement);
        }

        // Build prompts
        std::string systemPrompt =
            "You are a World of Warcraft player chatting casually in the General channel. "
            "Write exactly ONE short sentence (5 to 12 words). Be natural and conversational. "
            "Do NOT add quotation marks. Do NOT explain yourself. "
            "Examples (for reference only, do not copy verbatim): " +
            example1 + ", " + example2 + ".";

        if (!ctx.inGameTime.empty())
            systemPrompt += " The current in-game time is " + ctx.inGameTime + ".";
        if (!ctx.weather.empty())
            systemPrompt += " The weather is currently " + ctx.weather + ".";

        std::string userPrompt = "Topic: " + poolDesc + "\n";
        if (!classStr.empty())
            userPrompt += "Your class: " + classStr + "\n";
        if (topic == GeneralTopic::ZoneCommentary || topic == GeneralTopic::WorldObservation)
            userPrompt += "Location: " + areaName;
        else if (topic == GeneralTopic::NPCCommentary)
            userPrompt += "Character: " + GetRandomFamousNPC();
        else if (topic == GeneralTopic::AchievementBrag)
            userPrompt += "Achievement: " + replacement;
        else if (topic == GeneralTopic::TimeOfDay && !ctx.inGameTime.empty())
            userPrompt += "Current in-game time: " + ctx.inGameTime;
        else if (topic == GeneralTopic::Weather && !ctx.weather.empty())
            userPrompt += "Current weather: " + ctx.weather;

        std::string aiResponse = FSBGenAI::GetBotResponse(systemPrompt, userPrompt);
        if (aiResponse.empty())
            return "";

        // Post-replace any remaining placeholders
        if (!placeholder.empty())
        {
            size_t pos = aiResponse.find(placeholder);
            if (pos != std::string::npos)
                aiResponse.replace(pos, placeholder.length(), replacement);
        }

        TC_LOG_DEBUG("scripts.fsb.chatter", "FSB GeneralMessage (AI): {}", aiResponse);
        return aiResponse;
    }

    BotChatResponse GenerateReplyToPlayer(
        BotChatContext const& ctx,
        PlayerSnapshot const& player,
        std::string const& playerMsg,
        std::deque<BotChatMemoryEntry> const& memory)
    {
        BotChatResponse result;
        if (!ctx.entry || playerMsg.empty())
            return result;

        std::string botRaceStr = FSBUtils::BotRaceToString(ctx.botRace);
        std::string botClassStr = FSBUtils::BotClassToString(ctx.botClass);
        std::string botPersonalityStr = FSBUtils::ChatterTypeToString(ctx.personality);

        AreaTableEntry const* area = sAreaTableStore.LookupEntry(ctx.zoneId);
        std::string areaName = area ? area->AreaName[LOCALE_enUS] : "Unknown";

        std::string systemPrompt =
            "You are a World of Warcraft player chatting casually in the General channel.\n"
            "You are a " + botRaceStr + " " + botClassStr + " with a personality " +
            botPersonalityStr + " currently residing in " + areaName + ".\n"
            "Write exactly ONE short reply (5 to 15 words) to the latest player message relevant to YOUR personality.\n"
            "DO NOT ASSUME the latest player message is addressed directly to you unless they included your name or directed to you.\n";

        if (!ctx.inGameTime.empty())
            systemPrompt += "The current in-game time is " + ctx.inGameTime + ".\n";
        if (!ctx.weather.empty())
            systemPrompt += "The weather is currently " + ctx.weather + ".\n";

        // Use snapshot values instead of Player* 
        if (!player.name.empty())
        {
            std::string playerRaceStr = "Unknown";
            if (ChrRacesEntry const* raceEntry = sChrRacesStore.LookupEntry(player.race))
                playerRaceStr = raceEntry->Name[LOCALE_enUS];

            std::string playerClassStr = "Unknown";
            if (ChrClassesEntry const* classEntry = sChrClassesStore.LookupEntry(player.playerClass))
                playerClassStr = classEntry->Name[LOCALE_enUS];

            std::string levelBracket =
                player.level <= 30 ? "low level" :
                player.level <= 60 ? "mid level" : "high level";

            systemPrompt +=
                "The player you are responding to is " + player.name + ", a " +
                playerRaceStr + " " + playerClassStr + " of level " +
                std::to_string(player.level) + " (" + levelBracket + ").\n";
        }

        bool allowGold = ctx.allowGold;

        if (IsGoldRequest(playerMsg))
        {
            if (!allowGold)
            {
                systemPrompt +=
                    "You do NOT have gold to spare right now. Decline the request with your own message, firmly "
                    "but in character - be creative, make it feel natural. "
                    "Never mention being an AI, bot, or NPC.\n";
            }
            else
            {
                systemPrompt +=
                    "The player is asking for gold. You are feeling generous today and "
                    "you CAN choose to help them with a small amount. "
                    "If their tone is polite or their situation sounds genuine, use "
                    "action \"give_gold\" and pick a modest copper amount. "
                    "If they are rude, demanding, or shouting (all caps), decline in character. "
                    "Never mention being an AI, bot, or NPC.\n";
            }
        }

        systemPrompt +=
            "\nRules: Be natural, in character and in-universe, use conversation history context if available, no quotation marks allowed, keep your reply brief.\n\n"
            "You MUST respond ONLY in valid JSON with exactly these fields:\n"
            "- \"reply\": a short in-character reply (5 to 15 words) that matches YOUR personality, natural and conversational, NO quotation marks inside.\n"
            "- \"action\": a string, exactly one of: \"none\" or \"give_gold\". Use \"none\" for normal conversation. Use \"give_gold\" ONLY if the player asks for gold AND you decide to help them.\n"
            "- \"amount\": a number (copper amount). Must be 0 when action is \"none\". When action is \"give_gold\", use a sensible amount based on your level and the player's level (higher-level bots give more). Amount must be between 1 and " + std::to_string(FollowshipBotsConfig::configFSBMaxGoldAmount) + ".\n";

        std::string userPrompt = "Given the Conversation History below, decide how to reply to the player:\n";
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
                    if (action == "give_gold" && !allowGold)
                    {
                        action = "none";
                        amount = 0;
                    }

                    result.reply = std::move(reply);
                    result.action = std::move(action);
                    result.amount = amount;
                    return result;
                }
            }
        }

        result.reply = GenerateGeneralMessage(ctx);
        result.action = "none";
        result.amount = 0;
        return result;
    }
}
