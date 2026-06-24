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

#include "Followship_bots_channel_prompts.h"
#include "Followship_bots_chat_handler.h"
#include "Followship_bots_llamaAI.h"
#include "Followship_bots_mgr.h"
#include "Followship_bots_utils.h"

#include "DB2Stores.h"
#include "ItemTemplate.h"
#include "ObjectMgr.h"
#include "Log.h"
#include "Creature.h"
#include "Containers.h"

#include <mutex>
#include <unordered_map>
#include <vector>

namespace FSBChannelPrompts
{
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
        uint32 id;
        std::string name;
        bool isRaid;
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
        Team botTeam = FSBUtils::GetTeamFromFSBRace(bot);

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
    //  Trade message via LlamaAI
    // ------------------------------------------------------------------
    std::string GenerateTradeMessage(Creature* bot)
    {
        if (!FSBLlamaAI::IsEnabled())
            return "";

        if (!bot)
            return "";

        uint32 itemId = GetRandomTradeItem();
        if (!itemId)
            return "";

        std::string itemLink = FSBChat::BuildItemLink(itemId);
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

        // 10% chance for "free" intent
        bool isFree = urand(0, 99) < 10;
        std::string intent = isFree ? "giving away for free" : "selling";

        // Bot location
        std::string areaName = "around here";
        if (AreaTableEntry const* area = sAreaTableStore.LookupEntry(bot->GetAreaId()))
            if (area->ZoneName && area->ZoneName[0])
                areaName = area->ZoneName;

        // Pick 2 random examples from the correct intent pool
        std::vector<std::string> const& pool = isFree ? TradeFreeExamples : TradeSellingExamples;
        std::string example1 = Trinity::Containers::SelectRandomContainerElement(pool);
        std::string example2 = Trinity::Containers::SelectRandomContainerElement(pool);
        while (example2 == example1 && pool.size() > 1)
            example2 = Trinity::Containers::SelectRandomContainerElement(pool);

        // Build prompts
        std::string systemPrompt =
            "You are a World of Warcraft player typing a short message in the Trade chat channel. "
            "Write exactly ONE short sentence (5 to 12 words). Be natural and varied. "
            "Use the exact placeholder [ITEM] where the item should appear. "
            "You may mention your current location for flavor. "
            "Do NOT add quotation marks. Do NOT explain yourself. "
            "Examples (for reference only, do not copy verbatim): " +
            example1 + ", " + example2 + ".";

        std::string userPrompt = "Item name: " + itemName + "\n";
        if (!quantityStr.empty())
            userPrompt += "Quantity: " + quantityStr + "\n";
        userPrompt += "Intent: " + intent + "\n";
        userPrompt += "Your location: " + areaName;

        std::string aiResponse = FSBLlamaAI::GetBotResponse(systemPrompt, userPrompt);
        if (aiResponse.empty())
            return "";

        // Replace placeholders
        size_t pos = aiResponse.find("[ITEM]");
        if (pos != std::string::npos)
            aiResponse.replace(pos, 6, itemLink);

        pos = aiResponse.find("[LOCATION]");
        if (pos != std::string::npos)
            aiResponse.replace(pos, 10, areaName);

        TC_LOG_DEBUG("scripts.fsb.chatter", "FSB TradeMessage (AI): {}", aiResponse);
        return aiResponse;
    }

    // ------------------------------------------------------------------
    //  LFG message via LlamaAI
    // ------------------------------------------------------------------
    std::string GenerateLFGMessage(Creature* bot)
    {
        if (!FSBLlamaAI::IsEnabled())
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

        std::string aiResponse = FSBLlamaAI::GetBotResponse(systemPrompt, userPrompt);
        if (aiResponse.empty())
            return "";

        // Replace placeholder with actual dungeon name
        size_t pos = aiResponse.find("[DUNGEON]");
        if (pos != std::string::npos)
            aiResponse.replace(pos, 9, dungeon->name);

        TC_LOG_DEBUG("scripts.fsb.chatter", "FSB LFGMessage (AI): {}", aiResponse);
        return aiResponse;
    }
}
