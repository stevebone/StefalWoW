/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PlayerbotTradeConfig.h"
#include "Config/PlayerbotConfig.h"
#include "Log.h"
#include "StringConvert.h"
#include "Util.h"
#include "SharedDefines.h"
#include <algorithm>
#include <sstream>

namespace Playerbot
{
    // Static member definitions
    bool PlayerbotTradeConfig::m_tradeEnabled = true;
    bool PlayerbotTradeConfig::m_tradeAutoAccept = true;
    bool PlayerbotTradeConfig::m_tradeAutoAcceptGroup = true;
    bool PlayerbotTradeConfig::m_tradeAutoAcceptGuild = false;
    bool PlayerbotTradeConfig::m_tradeAutoAcceptOwner = true;
    bool PlayerbotTradeConfig::m_tradeAutoAcceptWhitelist = true;

    uint32 PlayerbotTradeConfig::m_tradeUpdateInterval = 1000;
    uint64 PlayerbotTradeConfig::m_tradeMaxGold = 100000 * GOLD;
    uint32 PlayerbotTradeConfig::m_tradeMaxItemValue = 10000 * GOLD;
    uint32 PlayerbotTradeConfig::m_tradeMaxItems = 6;
    float PlayerbotTradeConfig::m_tradeMaxDistance = 10.0f;
    uint32 PlayerbotTradeConfig::m_tradeTimeout = 60000;
    uint32 PlayerbotTradeConfig::m_tradeRequestTimeout = 30000;

    bool PlayerbotTradeConfig::m_tradeWhitelistOnly = false;
    uint8 PlayerbotTradeConfig::m_tradeSecurityLevel = 2;
    float PlayerbotTradeConfig::m_tradeValueTolerance = 0.3f;
    bool PlayerbotTradeConfig::m_tradeScamProtection = true;

    bool PlayerbotTradeConfig::m_lootDistributionEnabled = true;
    bool PlayerbotTradeConfig::m_needGreedEnabled = true;
    bool PlayerbotTradeConfig::m_roundRobinEnabled = false;
    bool PlayerbotTradeConfig::m_lootBySpecEnabled = true;

    ::std::vector<uint32> PlayerbotTradeConfig::m_protectedItems;

    bool PlayerbotTradeConfig::m_tradeLoggingEnabled = true;
    bool PlayerbotTradeConfig::m_detailedLoggingEnabled = false;
    bool PlayerbotTradeConfig::m_statisticsTrackingEnabled = true;

    void PlayerbotTradeConfig::Load()
    {
        TC_LOG_INFO("bot.trade", "Loading Playerbot Trade Configuration...");

        // Trading enabled
        m_tradeEnabled = sPlayerbotConfig->GetBool("Playerbot.Trade.Enable", true);

        // Auto-accept settings
        m_tradeAutoAccept = sPlayerbotConfig->GetBool("Playerbot.Trade.AutoAccept.Enable", true);
        m_tradeAutoAcceptGroup = sPlayerbotConfig->GetBool("Playerbot.Trade.AutoAccept.Group", true);
        m_tradeAutoAcceptGuild = sPlayerbotConfig->GetBool("Playerbot.Trade.AutoAccept.Guild", false);
        m_tradeAutoAcceptOwner = sPlayerbotConfig->GetBool("Playerbot.Trade.AutoAccept.Owner", true);
        m_tradeAutoAcceptWhitelist = sPlayerbotConfig->GetBool("Playerbot.Trade.AutoAccept.Whitelist", true);

        // Trade limits
        m_tradeUpdateInterval = sPlayerbotConfig->GetInt("Playerbot.Trade.UpdateInterval", 1000);
        m_tradeMaxGold = sPlayerbotConfig->GetInt("Playerbot.Trade.MaxGold", 100000 * GOLD);
        m_tradeMaxItemValue = sPlayerbotConfig->GetInt("Playerbot.Trade.MaxItemValue", 10000 * GOLD);
        m_tradeMaxItems = sPlayerbotConfig->GetInt("Playerbot.Trade.MaxItems", 6);
        m_tradeMaxDistance = sPlayerbotConfig->GetFloat("Playerbot.Trade.MaxDistance", 10.0f);
        m_tradeTimeout = sPlayerbotConfig->GetInt("Playerbot.Trade.Timeout", 60000);
        m_tradeRequestTimeout = sPlayerbotConfig->GetInt("Playerbot.Trade.RequestTimeout", 30000);

        // Security
        m_tradeWhitelistOnly = sPlayerbotConfig->GetBool("Playerbot.Trade.WhitelistOnly", false);
        m_tradeSecurityLevel = sPlayerbotConfig->GetInt("Playerbot.Trade.SecurityLevel", 2);
        m_tradeValueTolerance = sPlayerbotConfig->GetFloat("Playerbot.Trade.ValueTolerance", 0.3f);
        m_tradeScamProtection = sPlayerbotConfig->GetBool("Playerbot.Trade.ScamProtection", true);

        // Loot distribution
        m_lootDistributionEnabled = sPlayerbotConfig->GetBool("Playerbot.Trade.LootDistribution.Enable", true);
        m_needGreedEnabled = sPlayerbotConfig->GetBool("Playerbot.Trade.LootDistribution.NeedGreed", true);
        m_roundRobinEnabled = sPlayerbotConfig->GetBool("Playerbot.Trade.LootDistribution.RoundRobin", false);
        m_lootBySpecEnabled = sPlayerbotConfig->GetBool("Playerbot.Trade.LootDistribution.BySpec", true);

        // Protected items
        m_protectedItems.clear();
        ::std::string protectedItemsStr = sPlayerbotConfig->GetString("Playerbot.Trade.ProtectedItems", "");
        if (!protectedItemsStr.empty())
        {
            ::std::stringstream ss(protectedItemsStr);
            ::std::string token;
            while (::std::getline(ss, token, ','))
            {
                // Trim whitespace
                token.erase(0, token.find_first_not_of(" \t"));
                token.erase(token.find_last_not_of(" \t") + 1);

                if (!token.empty())
                {
                    if (Optional<uint32> itemId = Trinity::StringTo<uint32>(token))
                    {
                        m_protectedItems.push_back(*itemId);
                        TC_LOG_DEBUG("bot.trade", "Added protected item: {}", *itemId);
                    }
                }
            }
        }

        // Logging
        m_tradeLoggingEnabled = sPlayerbotConfig->GetBool("Playerbot.Trade.Logging.Enable", true);
        m_detailedLoggingEnabled = sPlayerbotConfig->GetBool("Playerbot.Trade.Logging.Detailed", false);
        m_statisticsTrackingEnabled = sPlayerbotConfig->GetBool("Playerbot.Trade.Logging.Statistics", true);

        // Validate configuration
    if (m_tradeUpdateInterval < 100)
        {
            TC_LOG_WARN("bot.trade", "Trade update interval too low ({}ms), setting to 100ms", m_tradeUpdateInterval);
            m_tradeUpdateInterval = 100;
        }

        if (m_tradeUpdateInterval > 10000)
        {
            TC_LOG_WARN("bot.trade", "Trade update interval too high ({}ms), setting to 10000ms", m_tradeUpdateInterval);
            m_tradeUpdateInterval = 10000;
        }

        if (m_tradeMaxDistance < 1.0f)
        {
            TC_LOG_WARN("bot.trade", "Trade max distance too low ({}), setting to 1.0", m_tradeMaxDistance);
            m_tradeMaxDistance = 1.0f;
        }

        if (m_tradeMaxDistance > 100.0f)
        {
            TC_LOG_WARN("bot.trade", "Trade max distance too high ({}), setting to 100.0", m_tradeMaxDistance);
            m_tradeMaxDistance = 100.0f;
        }

        if (m_tradeSecurityLevel > 3)
        {
            TC_LOG_WARN("bot.trade", "Invalid trade security level ({}), setting to 2 (Standard)", m_tradeSecurityLevel);
            m_tradeSecurityLevel = 2;
        }

        if (m_tradeValueTolerance < 0.0f || m_tradeValueTolerance > 1.0f)
        {
            TC_LOG_WARN("bot.trade", "Invalid trade value tolerance ({}), setting to 0.3", m_tradeValueTolerance);
            m_tradeValueTolerance = 0.3f;
        }

        TC_LOG_INFO("bot.trade", "Playerbot Trade Configuration loaded successfully:");
        TC_LOG_INFO("bot.trade", "  - Trade Enabled: {}", m_tradeEnabled ? "Yes" : "No");
        TC_LOG_INFO("bot.trade", "  - Security Level: {}", m_tradeSecurityLevel);
        TC_LOG_INFO("bot.trade", "  - Auto-Accept: Group={}, Guild={}, Owner={}, Whitelist={}",
            m_tradeAutoAcceptGroup ? "Yes" : "No",
            m_tradeAutoAcceptGuild ? "Yes" : "No",
            m_tradeAutoAcceptOwner ? "Yes" : "No",
            m_tradeAutoAcceptWhitelist ? "Yes" : "No");
        TC_LOG_INFO("bot.trade", "  - Max Gold: {}g", m_tradeMaxGold / GOLD);
        TC_LOG_INFO("bot.trade", "  - Max Item Value: {}g", m_tradeMaxItemValue / GOLD);
        TC_LOG_INFO("bot.trade", "  - Protected Items: {} items", m_protectedItems.size());
    }

    void PlayerbotTradeConfig::Reload()
    {
        TC_LOG_INFO("bot.trade", "Reloading Playerbot Trade Configuration...");
        Load();
    }

    float PlayerbotTradeConfig::GetItemQualityMultiplier(uint8 quality)
    {
        switch (quality)
        {
            case 0: return 0.5f;   // Poor (Gray)
            case 1: return 1.0f;   // Common (White)
            case 2: return 2.5f;   // Uncommon (Green)
            case 3: return 5.0f;   // Rare (Blue)
            case 4: return 10.0f;  // Epic (Purple)
            case 5: return 25.0f;  // Legendary (Orange)
            case 6: return 50.0f;  // Artifact (Light Gold)
            case 7: return 100.0f; // Heirloom
            default: return 1.0f;
        }
    }

    float PlayerbotTradeConfig::GetItemLevelMultiplier(uint32 level)
    {
        if (level == 0)
            return 1.0f;

        // Progressive multiplier based on item level brackets
    if (level <= 60)
            return 1.0f;
        else if (level <= 70)
            return 1.5f;
        else if (level <= 80)
            return 2.0f;
        else if (level <= 85)
            return 2.5f;
        else if (level <= 90)
            return 3.0f;
        else if (level <= 100)
            return 4.0f;
        else
            return 5.0f;
    }

    uint32 PlayerbotTradeConfig::GetItemBaseValue(uint32 itemClass)
    {
        switch (itemClass)
        {
            case 0:  return 100;    // Consumable
            case 1:  return 50;     // Container
            case 2:  return 500;    // Weapon
            case 4:  return 300;    // Armor
            case 5:  return 50;     // Reagent
            case 6:  return 100;    // Projectile
            case 7:  return 75;     // Trade Goods
            case 9:  return 150;    // Recipe
            case 11: return 25;     // Quiver
            case 12: return 100;    // Quest
            case 13: return 25;     // Key
            case 15: return 200;    // Miscellaneous
            case 16: return 250;    // Glyph
            default: return 100;
        }
    }

    bool PlayerbotTradeConfig::IsItemProtected(uint32 itemEntry)
    {
        return ::std::find(m_protectedItems.begin(), m_protectedItems.end(), itemEntry) != m_protectedItems.end();
    }

    void PlayerbotTradeConfig::AddProtectedItem(uint32 itemEntry)
    {
        if (!IsItemProtected(itemEntry))
        {
            m_protectedItems.push_back(itemEntry);
            TC_LOG_INFO("bot.trade", "Added item {} to protected items list", itemEntry);
        }
    }

    void PlayerbotTradeConfig::RemoveProtectedItem(uint32 itemEntry)
    {
        auto it = ::std::find(m_protectedItems.begin(), m_protectedItems.end(), itemEntry);
        if (it != m_protectedItems.end())
        {
            m_protectedItems.erase(it);
            TC_LOG_INFO("bot.trade", "Removed item {} from protected items list", itemEntry);
        }
    }

    // Helper function
    char const* GetSecurityLevelName(uint8 level)
    {
        switch (level)
        {
            case 0: return "None";
            case 1: return "Basic";
            case 2: return "Standard";
            case 3: return "Strict";
            default: return "Unknown";
        }
    }
}