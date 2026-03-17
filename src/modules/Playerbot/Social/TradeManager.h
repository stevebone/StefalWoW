/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_BOT_TRADE_MANAGER_H
#define TRINITYCORE_BOT_TRADE_MANAGER_H

#include "Common.h"
#include "AI/BehaviorManager.h"
#include "ObjectGuid.h"
#include "ItemTemplate.h"
#include "ItemDefines.h"
#include "SharedDefines.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <optional>
#include <queue>
#include <atomic>

class Player;
class Item;
class TradeData;

// Type alias for item quality
using ItemQuality = ItemQualities;

namespace Playerbot
{
    // Trade state machine states
    enum class TradeState : uint8
    {
        IDLE            = 0,    // No active trade
        INITIATING      = 1,    // Waiting for trade window to open
        ADDING_ITEMS    = 2,    // Adding items/gold to trade
        REVIEWING       = 3,    // Reviewing trade before accepting
        ACCEPTING       = 4,    // Accept button pressed, waiting for completion
        COMPLETED       = 5,    // Trade completed successfully
        CANCELLED       = 6,    // Trade was cancelled
        ERROR_STATE     = 7     // Trade encountered an error
    };

    // Trade security levels
    enum class TradeSecurity : uint8
    {
        SECURITY_NONE     = 0,    // No security checks
        SECURITY_BASIC    = 1,    // Basic ownership and group checks
        SECURITY_STANDARD = 2,    // Standard value comparison and whitelist
        SECURITY_STRICT   = 3     // Strict mode with all validations
    };

    // Trade item slot information
    struct TradeItemSlot
    {
        uint8 slot;                         // Trade slot index (0-6)
        ObjectGuid itemGuid;                // Item GUID
        uint32 itemEntry;                   // Item template ID
        uint32 itemCount;                   // Stack count
        uint32 estimatedValue;              // Estimated gold value
        bool isQuestItem;                   // Quest item flag
        bool isSoulbound;                   // Soulbound flag
        ItemQuality quality;                // Item quality (color)

        TradeItemSlot() : slot(0), itemEntry(0), itemCount(0), estimatedValue(0),
            isQuestItem(false), isSoulbound(false), quality(ITEM_QUALITY_POOR) {}
    };

    // Trade session data (renamed to avoid conflict with TradeSystem::TradeSession)
    struct TradeManagerSession
    {
        ObjectGuid traderGuid;              // Trading partner GUID
        TradeState state;                   // Current state
        uint64 offeredGold;                 // Gold we're offering
        uint64 receivedGold;                // Gold they're offering
        ::std::vector<TradeItemSlot> offeredItems;    // Items we're offering
        ::std::vector<TradeItemSlot> receivedItems;   // Items they're offering
        ::std::chrono::steady_clock::time_point startTime;    // Trade start time
        ::std::chrono::steady_clock::time_point lastUpdate;   // Last update time
        uint32 updateCount;                 // Number of updates
        bool isAccepted;                    // We accepted
        bool traderAccepted;                // They accepted
        TradeSecurity securityLevel;        // Security level for this trade

        TradeManagerSession() : state(TradeState::IDLE), offeredGold(0), receivedGold(0),
            updateCount(0), isAccepted(false), traderAccepted(false),
            securityLevel(TradeSecurity::SECURITY_STANDARD) {}

        void Reset();
        uint64 GetTotalOfferedValue() const;
        uint64 GetTotalReceivedValue() const;
        bool IsBalanced(float tolerance = 0.2f) const;
    };

    // Trade request tracking
    struct TradeRequest
    {
        ObjectGuid requesterGuid;
        ::std::chrono::steady_clock::time_point requestTime;
        bool isAutoAccept;
        ::std::string reason;

        TradeRequest() : isAutoAccept(false) {}
    };

    // Group loot distribution info
    struct GroupLootInfo
    {
        ::std::vector<Item*> items;
        ::std::unordered_map<ObjectGuid, uint32> playerPriorities;
        ::std::unordered_map<ObjectGuid, ::std::vector<uint32>> playerNeeds;
        bool useRoundRobin;
        bool considerSpec;

        GroupLootInfo() : useRoundRobin(true), considerSpec(true) {}
    };

    // Trade statistics tracking
    struct TradeStatistics
    {
        uint32 totalTrades;
        uint32 successfulTrades;
        uint32 cancelledTrades;
        uint32 failedTrades;
        uint64 totalGoldTraded;
        uint32 totalItemsTraded;
        ::std::chrono::milliseconds totalTradeTime;
        ::std::chrono::steady_clock::time_point lastTradeTime;

        TradeStatistics() : totalTrades(0), successfulTrades(0), cancelledTrades(0),
            failedTrades(0), totalGoldTraded(0), totalItemsTraded(0),
            totalTradeTime(0) {}

        float GetSuccessRate() const;
        ::std::chrono::milliseconds GetAverageTradeTime() const;
    };

    /**
     * TradeManager - Handles all trading, vendor, and repair activities for bots
     *
     * Inherits from BehaviorManager for throttled updates and performance optimization.
     * Manages:
     * - Player-to-player trading
     * - Vendor interactions (buying/selling)
     * - Equipment repair
     * - Consumable management
     * - Inventory optimization
     *
     * Update interval: 5000ms (5 seconds)
     */
    class TC_GAME_API TradeManager : public BehaviorManager
    {
    public:
        explicit TradeManager(Player* bot, BotAI* ai);
        ~TradeManager();

        // Core trade operations
        bool InitiateTrade(Player* target, ::std::string const& reason = "");
        bool InitiateTrade(ObjectGuid targetGuid, ::std::string const& reason = "");
        bool AcceptTradeRequest(ObjectGuid requesterGuid);
        bool DeclineTradeRequest(ObjectGuid requesterGuid);
        void CancelTrade(::std::string const& reason = "");
        bool AcceptTrade();

        // Item management
        bool AddItemToTrade(Item* item, uint8 slot = 255);
        bool AddItemsToTrade(::std::vector<Item*> const& items);
        bool RemoveItemFromTrade(uint8 slot);
        bool SetTradeGold(uint64 gold);

        // Trade window events
        void OnTradeStarted(Player* trader);
        void OnTradeStatusUpdate(TradeData* myTrade, TradeData* theirTrade);
        void OnTradeAccepted();
        void OnTradeCancelled();
        void OnTradeCompleted();

        // Fast atomic state queries (<0.001ms)
        bool IsTradingActive() const { return _isTradingActive.load(::std::memory_order_acquire); }
        bool NeedsRepair() const { return _needsRepair.load(::std::memory_order_acquire); }
        bool NeedsSupplies() const { return _needsSupplies.load(::std::memory_order_acquire); }

        // Group loot distribution
        bool DistributeLoot(::std::vector<Item*> const& items, bool useNeedGreed = true);
        bool SendItemToPlayer(Item* item, Player* recipient);
        bool RequestItemFromPlayer(uint32 itemEntry, Player* owner);

        // Trade validation and security
        bool ValidateTradeTarget(Player* target) const;
        bool ValidateTradeItems() const;
        bool ValidateTradeGold(uint64 amount) const;
        bool EvaluateTradeFairness() const;
        bool IsTradeScam() const;
        bool IsTradeSafe() const;

        // Configuration and settings
        void SetSecurityLevel(TradeSecurity level) { m_securityLevel = level; }
        TradeSecurity GetSecurityLevel() const { return m_securityLevel; }
        void SetAutoAcceptGroup(bool enable) { m_autoAcceptGroup = enable; }
        void SetAutoAcceptGuild(bool enable) { m_autoAcceptGuild = enable; }
        void SetMaxTradeValue(uint64 value) { m_maxTradeValue = value; }

        // Whitelist/Blacklist management
        void AddToWhitelist(ObjectGuid guid) { m_tradeWhitelist.insert(guid); }
        void RemoveFromWhitelist(ObjectGuid guid) { m_tradeWhitelist.erase(guid); }
        void AddToBlacklist(ObjectGuid guid) { m_tradeBlacklist.insert(guid); }
        void RemoveFromBlacklist(ObjectGuid guid) { m_tradeBlacklist.erase(guid); }
        bool IsWhitelisted(ObjectGuid guid) const { return m_tradeWhitelist.count(guid) > 0; }
        bool IsBlacklisted(ObjectGuid guid) const { return m_tradeBlacklist.count(guid) > 0; }

        // State queries
        bool IsTrading() const { return m_currentSession.state != TradeState::IDLE; }
        TradeState GetTradeState() const { return m_currentSession.state; }
        ObjectGuid GetTradingPartner() const { return m_currentSession.traderGuid; }
        TradeManagerSession const& GetCurrentSession() const { return m_currentSession; }

        // Statistics
        TradeStatistics const& GetStatistics() const { return m_statistics; }
        void ResetStatistics() { m_statistics = TradeStatistics(); }

        // Utility functions
        uint32 EstimateItemValue(Item* item) const;
        uint32 EstimateItemValue(uint32 itemEntry, uint32 count = 1) const;
        bool CanTradeItem(Item* item) const;
        uint8 GetNextFreeTradeSlot() const;
        ::std::vector<Item*> GetTradableItems() const;

    protected:
        // BehaviorManager interface - runs every 5 seconds
        void OnUpdate(uint32 elapsed);
        bool OnInitialize();
        void OnShutdown();
        void OnEventInternal(Events::BotEvent const& event);

    private:
        // Internal state management
        void SetTradeState(TradeState newState);
        void ResetTradeSession();
        void UpdateTradeWindow();
        bool ProcessTradeUpdate(uint32 diff);

        // Item evaluation
        uint32 CalculateItemBaseValue(ItemTemplate const* itemTemplate) const;
        float GetItemQualityMultiplier(ItemQuality quality) const;
        float GetItemLevelMultiplier(uint32 itemLevel) const;
        bool IsItemNeededByBot(uint32 itemEntry) const;
        bool IsItemUsableByBot(Item* item) const;

        // Group distribution logic
        Player* SelectBestRecipient(Item* item, ::std::vector<Player*> const& candidates) const;
        uint32 CalculateItemPriority(Item* item, Player* player) const;
        bool CanPlayerUseItem(Item* item, Player* player) const;
        void BuildLootDistributionPlan(GroupLootInfo& distribution);

        // Security validations
        bool ValidateTradeDistance(Player* trader) const;
        bool ValidateTradePermissions(Player* trader) const;
        bool ValidateItemOwnership(Item* item) const;
        bool CheckForScamPatterns() const;
        bool CheckValueBalance() const;

        // Trade execution
        bool ExecuteTrade();
        void ProcessTradeCompletion();
        void ProcessTradeCancellation(::std::string const& reason);
        void HandleTradeError(::std::string const& error);

        // Logging and debugging
        void LogTradeAction(::std::string const& action, ::std::string const& details = "") const;
        void LogTradeItem(Item* item, bool offered);
        void LogTradeCompletion(bool success);

    private:
        // Atomic state flags for fast queries
        ::std::atomic<bool> _isTradingActive{false};
        ::std::atomic<bool> _needsRepair{false};
        ::std::atomic<bool> _needsSupplies{false};

        // Current trade session
        TradeManagerSession m_currentSession;                         // Active trade data
        ::std::unordered_map<ObjectGuid, TradeRequest> m_pendingRequests;    // Pending trade requests

        // Configuration
        TradeSecurity m_securityLevel;                         // Security level
        bool m_autoAcceptGroup;                                // Auto-accept group trades
        bool m_autoAcceptGuild;                                // Auto-accept guild trades
        bool m_autoAcceptWhitelist;                            // Auto-accept whitelist trades
        uint64 m_maxTradeValue;                                // Maximum trade value
        uint32 m_maxTradeDistance;                             // Maximum trade distance

        // Security lists
        ::std::unordered_set<ObjectGuid> m_tradeWhitelist;      // Trusted traders
        ::std::unordered_set<ObjectGuid> m_tradeBlacklist;      // Blocked traders
        ::std::unordered_set<uint32> m_protectedItems;          // Items to never trade

        // Group loot distribution
        ::std::unique_ptr<GroupLootInfo> m_currentDistribution;    // Active distribution
        ::std::queue<::std::pair<Item*, ObjectGuid>> m_pendingTransfers;    // Pending item transfers

        // Performance tracking
        TradeStatistics m_statistics;                          // Trade statistics
        ::std::chrono::steady_clock::time_point m_lastUpdateTime;    // Last update timestamp
        uint32 m_updateTimer;                                  // Update timer

        // Constants
        static constexpr uint32 TRADE_UPDATE_INTERVAL = 1000;  // 1 second
        static constexpr uint32 TRADE_TIMEOUT = 60000;         // 60 seconds
        static constexpr uint32 TRADE_REQUEST_TIMEOUT = 30000; // 30 seconds
        static constexpr float MAX_TRADE_DISTANCE_YARDS = 10.0f;  // 10 yards
        static constexpr uint8 MAX_TRADE_ITEMS = 6;           // 6 item slots + gold
        static constexpr float SCAM_VALUE_THRESHOLD = 0.1f;    // 10% value difference
    };
}

#endif // TRINITYCORE_BOT_TRADE_MANAGER_H