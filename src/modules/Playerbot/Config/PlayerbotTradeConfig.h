/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_PLAYERBOT_TRADE_CONFIG_H
#define TRINITYCORE_PLAYERBOT_TRADE_CONFIG_H

#include "Common.h"
#include <string>
#include <vector>

namespace Playerbot
{
    class PlayerbotTradeConfig
    {
    public:
        // Load configuration from file
        static void Load();
        static void Reload();

        // Trading enabled
        static bool IsTradeEnabled() { return m_tradeEnabled; }
        static void SetTradeEnabled(bool enabled) { m_tradeEnabled = enabled; }

        // Auto-accept settings
        static bool IsTradeAutoAcceptEnabled() { return m_tradeAutoAccept; }
        static bool IsTradeAutoAcceptGroupEnabled() { return m_tradeAutoAcceptGroup; }
        static bool IsTradeAutoAcceptGuildEnabled() { return m_tradeAutoAcceptGuild; }
        static bool IsTradeAutoAcceptOwnerEnabled() { return m_tradeAutoAcceptOwner; }
        static bool IsTradeAutoAcceptWhitelistEnabled() { return m_tradeAutoAcceptWhitelist; }

        // Trade limits
        static uint32 GetTradeUpdateInterval() { return m_tradeUpdateInterval; }
        static uint64 GetTradeMaxGoldAmount() { return m_tradeMaxGold; }
        static uint32 GetTradeMaxItemValue() { return m_tradeMaxItemValue; }
        static uint32 GetTradeMaxItems() { return m_tradeMaxItems; }
        static float GetTradeMaxDistance() { return m_tradeMaxDistance; }
        static uint32 GetTradeTimeout() { return m_tradeTimeout; }
        static uint32 GetTradeRequestTimeout() { return m_tradeRequestTimeout; }

        // Security settings
        static bool IsTradeWhitelistOnly() { return m_tradeWhitelistOnly; }
        static uint8 GetTradeSecurityLevel() { return m_tradeSecurityLevel; }
        static float GetTradeValueTolerance() { return m_tradeValueTolerance; }
        static bool IsTradeScamProtectionEnabled() { return m_tradeScamProtection; }

        // Loot distribution
        static bool IsLootDistributionEnabled() { return m_lootDistributionEnabled; }
        static bool IsNeedGreedEnabled() { return m_needGreedEnabled; }
        static bool IsRoundRobinEnabled() { return m_roundRobinEnabled; }
        static bool IsLootBySpecEnabled() { return m_lootBySpecEnabled; }

        // Item evaluation
        static float GetItemQualityMultiplier(uint8 quality);
        static float GetItemLevelMultiplier(uint32 level);
        static uint32 GetItemBaseValue(uint32 itemClass);

        // Protected items (never trade these)
        static ::std::vector<uint32> const& GetProtectedItems() { return m_protectedItems; }
        static bool IsItemProtected(uint32 itemEntry);
        static void AddProtectedItem(uint32 itemEntry);
        static void RemoveProtectedItem(uint32 itemEntry);

        // Logging
        static bool IsTradeLoggingEnabled() { return m_tradeLoggingEnabled; }
        static bool IsDetailedLoggingEnabled() { return m_detailedLoggingEnabled; }
        static bool IsStatisticsTrackingEnabled() { return m_statisticsTrackingEnabled; }

    private:
        // Trading settings
        static bool m_tradeEnabled;
        static bool m_tradeAutoAccept;
        static bool m_tradeAutoAcceptGroup;
        static bool m_tradeAutoAcceptGuild;
        static bool m_tradeAutoAcceptOwner;
        static bool m_tradeAutoAcceptWhitelist;

        // Trade limits
        static uint32 m_tradeUpdateInterval;
        static uint64 m_tradeMaxGold;
        static uint32 m_tradeMaxItemValue;
        static uint32 m_tradeMaxItems;
        static float m_tradeMaxDistance;
        static uint32 m_tradeTimeout;
        static uint32 m_tradeRequestTimeout;

        // Security
        static bool m_tradeWhitelistOnly;
        static uint8 m_tradeSecurityLevel;
        static float m_tradeValueTolerance;
        static bool m_tradeScamProtection;

        // Loot distribution
        static bool m_lootDistributionEnabled;
        static bool m_needGreedEnabled;
        static bool m_roundRobinEnabled;
        static bool m_lootBySpecEnabled;

        // Protected items
        static ::std::vector<uint32> m_protectedItems;

        // Logging
        static bool m_tradeLoggingEnabled;
        static bool m_detailedLoggingEnabled;
        static bool m_statisticsTrackingEnabled;
    };

    // Configuration string for playerbots.conf
    inline ::std::string GetTradeConfigString()
    {
        return R"(
###################################################################################################
# TRADE SYSTEM CONFIGURATION
###################################################################################################

# Enable bot trading system
Playerbot.Trade.Enable = 1

# Auto-accept trade requests from specific sources
Playerbot.Trade.AutoAccept.Enable = 1
Playerbot.Trade.AutoAccept.Group = 1
Playerbot.Trade.AutoAccept.Guild = 0
Playerbot.Trade.AutoAccept.Owner = 1
Playerbot.Trade.AutoAccept.Whitelist = 1

# Trade update interval in milliseconds
Playerbot.Trade.UpdateInterval = 1000

# Maximum gold amount per trade (in copper, 10000g = 100000000)
Playerbot.Trade.MaxGold = 100000000

# Maximum single item value (in copper)
Playerbot.Trade.MaxItemValue = 10000000

# Maximum number of items per trade
Playerbot.Trade.MaxItems = 6

# Maximum trade distance in yards
Playerbot.Trade.MaxDistance = 10.0

# Trade timeout in milliseconds
Playerbot.Trade.Timeout = 60000

# Trade request timeout in milliseconds
Playerbot.Trade.RequestTimeout = 30000

# Security level (0=None, 1=Basic, 2=Standard, 3=Strict)
Playerbot.Trade.SecurityLevel = 2

# Only allow trades with whitelisted players
Playerbot.Trade.WhitelistOnly = 0

# Trade value tolerance (0.0-1.0, how unbalanced trades can be)
Playerbot.Trade.ValueTolerance = 0.3

# Enable scam protection
Playerbot.Trade.ScamProtection = 1

# Loot distribution settings
Playerbot.Trade.LootDistribution.Enable = 1
Playerbot.Trade.LootDistribution.NeedGreed = 1
Playerbot.Trade.LootDistribution.RoundRobin = 0
Playerbot.Trade.LootDistribution.BySpec = 1

# Protected items (comma-separated item IDs that bots will never trade)
# Example: 19019,22726,23577 (Thunderfury, Atiesh, Warglaive)
Playerbot.Trade.ProtectedItems = ""

# Logging settings
Playerbot.Trade.Logging.Enable = 1
Playerbot.Trade.Logging.Detailed = 0
Playerbot.Trade.Logging.Statistics = 1
)";
    }
}

#endif // TRINITYCORE_PLAYERBOT_TRADE_CONFIG_H