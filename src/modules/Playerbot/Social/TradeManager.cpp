/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "TradeManager.h"
#include "BotAI.h"
#include "Player.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "ItemDefines.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "TradeData.h"
#include "Group.h"
#include "Guild.h"
#include "Chat.h"
#include "Log.h"
#include "World.h"
#include "SpellMgr.h"
#include "DB2Stores.h"
#include "Bag.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "SharedDefines.h"
#include "../Group/GroupMemberResolver.h"
#include "../Core/Diagnostics/GroupMemberDiagnostics.h"
#include "Config/PlayerbotTradeConfig.h"
#include <algorithm>
#include <numeric>

namespace Playerbot
{
    // TradeManagerSession implementation
    void TradeManagerSession::Reset()
    {
        traderGuid.Clear();
        state = TradeState::IDLE;
        offeredGold = 0;
        receivedGold = 0;
        offeredItems.clear();
        receivedItems.clear();
        updateCount = 0;
        isAccepted = false;
        traderAccepted = false;
    }

    uint64 TradeManagerSession::GetTotalOfferedValue() const
    {
        uint64 totalValue = offeredGold;
        for (auto const& slot : offeredItems)

            totalValue += slot.estimatedValue * slot.itemCount;
        return totalValue;
    }

    uint64 TradeManagerSession::GetTotalReceivedValue() const
    {
        uint64 totalValue = receivedGold;
        for (auto const& slot : receivedItems)

            totalValue += slot.estimatedValue * slot.itemCount;
        return totalValue;
    }

    bool TradeManagerSession::IsBalanced(float tolerance) const
    {
        uint64 offeredValue = GetTotalOfferedValue();
        uint64 receivedValue = GetTotalReceivedValue();

        if (offeredValue == 0 && receivedValue == 0)

            return true;

        if (offeredValue == 0 || receivedValue == 0)

            return false;

        float ratio = static_cast<float>(::std::min(offeredValue, receivedValue)) /

                     static_cast<float>(::std::max(offeredValue, receivedValue));

        return ratio >= (1.0f - tolerance);
    }

    // TradeStatistics implementation
    float TradeStatistics::GetSuccessRate() const
    {
        if (totalTrades == 0)

            return 0.0f;
        return static_cast<float>(successfulTrades) / static_cast<float>(totalTrades);
    }

    ::std::chrono::milliseconds TradeStatistics::GetAverageTradeTime() const
    {
        if (successfulTrades == 0)

            return ::std::chrono::milliseconds(0);
        return totalTradeTime / successfulTrades;
    }

    // TradeManager implementation
    TradeManager::TradeManager(Player* bot, BotAI* ai) :
        BehaviorManager(bot, ai, PlayerbotTradeConfig::GetTradeUpdateInterval(), "TradeManager"),
        m_securityLevel(static_cast<TradeSecurity>(PlayerbotTradeConfig::GetTradeSecurityLevel())),
        m_autoAcceptGroup(PlayerbotTradeConfig::IsTradeAutoAcceptGroupEnabled()),
        m_autoAcceptGuild(PlayerbotTradeConfig::IsTradeAutoAcceptGuildEnabled()),
        m_autoAcceptWhitelist(PlayerbotTradeConfig::IsTradeAutoAcceptWhitelistEnabled()),
        m_maxTradeValue(PlayerbotTradeConfig::GetTradeMaxGoldAmount()),
        m_maxTradeDistance(PlayerbotTradeConfig::GetTradeMaxDistance()),
        m_updateTimer(0)
    {
        m_lastUpdateTime = ::std::chrono::steady_clock::now();

        // Load protected items from config
        for (uint32 itemId : PlayerbotTradeConfig::GetProtectedItems())
            m_protectedItems.insert(itemId);

        TC_LOG_DEBUG("bot.trade", "TradeManager created with config: SecurityLevel={}, AutoAcceptGroup={}, MaxTradeValue={}",
            static_cast<uint8>(m_securityLevel), m_autoAcceptGroup, m_maxTradeValue);
    }

    TradeManager::~TradeManager()
    {
        if (IsTrading())

            CancelTrade("Bot shutting down");
    }

    bool TradeManager::OnInitialize()
    {
        // Initialize trade state
        ResetTradeSession();
        m_updateTimer = 0;
        m_lastUpdateTime = ::std::chrono::steady_clock::now();

        // Clear any pending transfers
    while (!m_pendingTransfers.empty())

            m_pendingTransfers.pop();

        // Clear pending requests
        m_pendingRequests.clear();

        // Reset statistics for this session
        m_statistics = TradeStatistics();

        // Set initial atomic states
        _isTradingActive.store(false, ::std::memory_order_release);
        _needsRepair.store(false, ::std::memory_order_release);
        _needsSupplies.store(false, ::std::memory_order_release);

        TC_LOG_DEBUG("bot.trade", "TradeManager initialized for bot {}",

            GetBot() ? GetBot()->GetName() : "unknown");

        return true;
    }

    void TradeManager::OnShutdown()
    {
        // Cancel any ongoing trade
    if (IsTrading())

            CancelTrade("Manager shutting down");

        // Clear pending transfers
    while (!m_pendingTransfers.empty())

            m_pendingTransfers.pop();

        // Clear pending requests
        m_pendingRequests.clear();

        // Clear distribution plan
        m_currentDistribution.reset();

        // Reset session
        ResetTradeSession();

        TC_LOG_DEBUG("bot.trade", "TradeManager shut down for bot {}",

            GetBot() ? GetBot()->GetName() : "unknown");
    }

    bool TradeManager::InitiateTrade(Player* target, ::std::string const& reason)
    {
        if (!target)

            return false;

        return InitiateTrade(target->GetGUID(), reason);
    }

    bool TradeManager::InitiateTrade(ObjectGuid targetGuid, ::std::string const& reason)
    {
        if (!GetBot() || !targetGuid)

            return false;

        // Check if already trading
    if (IsTrading())
        {

            LogTradeAction("INITIATE_FAILED", "Already in trade");

            return false;
        }

        // Find target player
        Player* target = ObjectAccessor::FindPlayer(targetGuid);
        if (!target)
        {

            LogTradeAction("INITIATE_FAILED", "Target not found");

            return false;
        }

        // Validate target
    if (!ValidateTradeTarget(target))
        {

            LogTradeAction("INITIATE_FAILED", "Target validation failed");

            return false;
        }

        // Check distance
    if (!ValidateTradeDistance(target))
        {

            LogTradeAction("INITIATE_FAILED", "Target too far away");

            return false;
        }

        // Check if target is already trading
    if (target->GetTradeData())
        {

            LogTradeAction("INITIATE_FAILED", "Target already trading");

            return false;
        }
        // Initialize trade session
        ResetTradeSession();
        m_currentSession.traderGuid = targetGuid;
        m_currentSession.startTime = ::std::chrono::steady_clock::now();
        SetTradeState(TradeState::INITIATING);

        // Initiate trade (this will create the trade data internally and send packets)
        GetBot()->InitiateTrade(target);

        LogTradeAction("INITIATE", "Trade initiated with " + target->GetName() + " - " + reason);
        return true;
    }

    bool TradeManager::AcceptTradeRequest(ObjectGuid requesterGuid)
    {
        if (!GetBot() || !requesterGuid)

            return false;

        // Check if already trading
    if (IsTrading())
        {

            LogTradeAction("ACCEPT_REQUEST_FAILED", "Already in trade");

            return false;
        }

        // Find requester
        Player* requester = ObjectAccessor::FindPlayer(requesterGuid);
        if (!requester)
        {

            LogTradeAction("ACCEPT_REQUEST_FAILED", "Requester not found");

            return false;
        }

        // Check for pending request
        auto it = m_pendingRequests.find(requesterGuid);
        if (it == m_pendingRequests.end())
        {

            LogTradeAction("ACCEPT_REQUEST_FAILED", "No pending request from player");

            return false;
        }

        // Check request timeout
        auto now = ::std::chrono::steady_clock::now();
        auto elapsed = ::std::chrono::duration_cast<::std::chrono::milliseconds>(

            now - it->second.requestTime).count();

        if (elapsed > static_cast<int64>(PlayerbotTradeConfig::GetTradeRequestTimeout()))
        {
            m_pendingRequests.erase(it);
            LogTradeAction("ACCEPT_REQUEST_FAILED", "Request timed out");
            return false;
        }

        // Accept the trade request
        m_pendingRequests.erase(it);
        return InitiateTrade(requester, "Accepting trade request");
    }

    bool TradeManager::DeclineTradeRequest(ObjectGuid requesterGuid)
    {
        auto it = m_pendingRequests.find(requesterGuid);
        if (it != m_pendingRequests.end())
        {

            m_pendingRequests.erase(it);

            LogTradeAction("DECLINE_REQUEST", "Declined trade from " + requesterGuid.ToString());

            return true;
        }
        return false;
    }

    void TradeManager::CancelTrade(::std::string const& reason)
    {
        if (!IsTrading())

            return;

        ProcessTradeCancellation(reason);
    }

    bool TradeManager::AcceptTrade()
    {
        if (!GetBot() || !IsTrading())

            return false;

        TradeData* myTrade = GetBot()->GetTradeData();
        if (!myTrade)

            return false;

        // Validate trade before accepting
    if (m_securityLevel >= TradeSecurity::SECURITY_BASIC)
        {

            if (!ValidateTradeItems())

            {

                LogTradeAction("ACCEPT_FAILED", "Item validation failed");

                CancelTrade("Invalid items in trade");

                return false;

            }


            if (!IsTradeSafe())

            {

                LogTradeAction("ACCEPT_FAILED", "Trade safety check failed");

                CancelTrade("Unsafe trade detected");

                return false;

            }
        }
        // Check for scam (skip for owner trades)
    if (m_securityLevel >= TradeSecurity::SECURITY_STANDARD)
        {
            // Don't check scam for trades with owner

            bool isOwnerTrade = false;

            if (Player* trader = ObjectAccessor::FindPlayer(m_currentSession.traderGuid))

            {
                // Check if trader is the bot's master/owner (would need to be set elsewhere)
                // For now, we'll allow trades with group leader as owner

                Group* group = GetBot()->GetGroup();

                if (group && group->GetLeaderGUID() == trader->GetGUID())

                    isOwnerTrade = true;

            }


            if (!isOwnerTrade && IsTradeScam())

            {

                LogTradeAction("ACCEPT_FAILED", "Potential scam detected");

                CancelTrade("Potential scam detected");

                return false;

            }
        }

        // Check fairness
    if (m_securityLevel >= TradeSecurity::SECURITY_STRICT)
        {

            if (!EvaluateTradeFairness())

            {

                LogTradeAction("ACCEPT_FAILED", "Trade not fair");

                CancelTrade("Unfair trade");

                return false;

            }
        }

        // Set accepted
        myTrade->SetAccepted(true, false);
        m_currentSession.isAccepted = true;
        SetTradeState(TradeState::ACCEPTING);

        // Check if both accepted
        TradeData* theirTrade = myTrade->GetTraderData();
        if (theirTrade && theirTrade->IsAccepted())
        {

            ProcessTradeCompletion();
        }

        LogTradeAction("ACCEPT", "Trade accepted");
        return true;
    }

    bool TradeManager::AddItemToTrade(Item* item, uint8 slot)
    {
        if (!GetBot() || !item || !IsTrading())

            return false;

        TradeData* myTrade = GetBot()->GetTradeData();
        if (!myTrade)

            return false;

        // Validate item can be traded
    if (!CanTradeItem(item))
        {

            LogTradeAction("ADD_ITEM_FAILED", "Item cannot be traded: " + ::std::to_string(item->GetEntry()));

            return false;
        }

        // Find free slot if not specified
    if (slot == 255)
        {

            slot = GetNextFreeTradeSlot();

            if (slot >= MAX_TRADE_ITEMS)

            {

                LogTradeAction("ADD_ITEM_FAILED", "No free trade slots");

                return false;

            }
        }

        // Add item to trade
        myTrade->SetItem(TradeSlots(slot), item, false);

        // Update session
        TradeItemSlot itemSlot;
        itemSlot.slot = slot;
        itemSlot.itemGuid = item->GetGUID();
        itemSlot.itemEntry = item->GetEntry();
        itemSlot.itemCount = item->GetCount();
        itemSlot.estimatedValue = EstimateItemValue(item);
        itemSlot.isQuestItem = (item->GetTemplate()->GetStartQuest() != 0);
        itemSlot.isSoulbound = item->IsSoulBound() || item->GetTemplate()->HasFlag(ITEM_FLAG_IS_BOUND_TO_ACCOUNT);
        itemSlot.quality = ItemQuality(item->GetTemplate()->GetQuality());

        m_currentSession.offeredItems.push_back(itemSlot);
        SetTradeState(TradeState::ADDING_ITEMS);

        LogTradeItem(item, true);
        return true;
    }

    bool TradeManager::AddItemsToTrade(::std::vector<Item*> const& items)
    {
        if (!GetBot() || items.empty() || !IsTrading())

            return false;

        uint32 addedCount = 0;
        for (Item* item : items)
        {

            if (AddItemToTrade(item))

                addedCount++;

            // Check if all slots are full
    if (GetNextFreeTradeSlot() >= MAX_TRADE_ITEMS)

                break;
        }

        return addedCount > 0;
    }

    bool TradeManager::RemoveItemFromTrade(uint8 slot)
    {
        if (!GetBot() || !IsTrading() || slot >= MAX_TRADE_ITEMS)

            return false;

        TradeData* myTrade = GetBot()->GetTradeData();
        if (!myTrade)

            return false;

        // Clear the slot
        myTrade->SetItem(TradeSlots(slot), nullptr, false);

        // Remove from session
        auto it = ::std::remove_if(m_currentSession.offeredItems.begin(),

                                 m_currentSession.offeredItems.end(),

                                 [slot](TradeItemSlot const& item) { return item.slot == slot; });

        if (it != m_currentSession.offeredItems.end())
        {

            m_currentSession.offeredItems.erase(it, m_currentSession.offeredItems.end());

            LogTradeAction("REMOVE_ITEM", "Removed item from slot " + ::std::to_string(slot));

            return true;
        }

        return false;
    }

    bool TradeManager::SetTradeGold(uint64 gold)
    {
        if (!GetBot() || !IsTrading())

            return false;

        TradeData* myTrade = GetBot()->GetTradeData();
        if (!myTrade)

            return false;

        // Validate gold amount
    if (!ValidateTradeGold(gold))
        {

            LogTradeAction("SET_GOLD_FAILED", "Invalid gold amount: " + ::std::to_string(gold));

            return false;
        }

        // Check bot has enough gold
    if (gold > GetBot()->GetMoney())
        {

            LogTradeAction("SET_GOLD_FAILED", "Insufficient gold");

            return false;
        }
        // Set gold in trade
        myTrade->SetMoney(gold);
        m_currentSession.offeredGold = gold;
        SetTradeState(TradeState::ADDING_ITEMS);

        LogTradeAction("SET_GOLD", "Set gold to " + ::std::to_string(gold));
        return true;
    }

    void TradeManager::OnTradeStarted(Player* trader)
    {
        if (!trader)

            return;

        m_currentSession.traderGuid = trader->GetGUID();
        SetTradeState(TradeState::ADDING_ITEMS);
        m_currentSession.startTime = ::std::chrono::steady_clock::now();

        LogTradeAction("TRADE_STARTED", "Trade window opened with " + trader->GetName());
    }

    void TradeManager::OnTradeStatusUpdate(TradeData* myTrade, TradeData* theirTrade)
    {
        if (!myTrade || !theirTrade)

            return;

        // Update received items
        m_currentSession.receivedItems.clear();
        for (uint8 slot = 0; slot < MAX_TRADE_ITEMS; ++slot)
        {

            Item* item = theirTrade->GetItem(TradeSlots(slot));

            if (!item)

                continue;


            TradeItemSlot itemSlot;

            itemSlot.slot = slot;

            itemSlot.itemGuid = item->GetGUID();

            itemSlot.itemEntry = item->GetEntry();

            itemSlot.itemCount = item->GetCount();

            itemSlot.estimatedValue = EstimateItemValue(item);

            itemSlot.isQuestItem = (item->GetTemplate()->GetStartQuest() != 0);

            itemSlot.isSoulbound = item->IsSoulBound() || item->GetTemplate()->HasFlag(ITEM_FLAG_IS_BOUND_TO_ACCOUNT);

            itemSlot.quality = ItemQuality(item->GetTemplate()->GetQuality());


            m_currentSession.receivedItems.push_back(itemSlot);
        }

        // Update received gold
        m_currentSession.receivedGold = theirTrade->GetMoney();

        // Update acceptance status
        m_currentSession.traderAccepted = theirTrade->IsAccepted();

        // Update state
    if (m_currentSession.state == TradeState::ADDING_ITEMS)

            SetTradeState(TradeState::REVIEWING);

        m_currentSession.updateCount++;
        m_currentSession.lastUpdate = ::std::chrono::steady_clock::now();
    }

    void TradeManager::OnTradeAccepted()
    {
        SetTradeState(TradeState::ACCEPTING);
        LogTradeAction("TRADE_ACCEPTED", "Both parties accepted");
    }

    void TradeManager::OnTradeCancelled()
    {
        ProcessTradeCancellation("Trade cancelled by other party");
    }

    void TradeManager::OnTradeCompleted()
    {
        ProcessTradeCompletion();
    }

    void TradeManager::OnUpdate(uint32 elapsed)
    {
        m_updateTimer += elapsed;

        // Process pending trade requests
        auto now = ::std::chrono::steady_clock::now();
        for (auto it = m_pendingRequests.begin(); it != m_pendingRequests.end();)
        {

            auto elapsed = ::std::chrono::duration_cast<::std::chrono::milliseconds>(

                now - it->second.requestTime).count();

            if (elapsed > static_cast<int64>(PlayerbotTradeConfig::GetTradeRequestTimeout()))
            {
                LogTradeAction("REQUEST_TIMEOUT", "Request from " + it->first.ToString() + " timed out");
                it = m_pendingRequests.erase(it);
            }
            else
            {
                // Auto-accept logic
                if (it->second.isAutoAccept)

                {

                    AcceptTradeRequest(it->first);

                    it = m_pendingRequests.erase(it);

                }

                else

                    ++it;

            }
        }

        // Update current trade
    if (IsTrading())
        {

            if (m_updateTimer >= TRADE_UPDATE_INTERVAL)

            {

                m_updateTimer = 0;

                ProcessTradeUpdate(elapsed);

            }

            // Check for timeout
            auto elapsed = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
                now - m_currentSession.startTime).count();

            if (elapsed > static_cast<int64>(PlayerbotTradeConfig::GetTradeTimeout()))
            {
                LogTradeAction("TRADE_TIMEOUT", "Trade timed out after " + ::std::to_string(elapsed) + "ms");
                CancelTrade("Trade timeout");
            }
        }

        // Process pending item transfers
    if (!m_pendingTransfers.empty() && !IsTrading())
        {

            auto& transfer = m_pendingTransfers.front();

            Player* recipient = ObjectAccessor::FindPlayer(transfer.second);

            if (recipient && transfer.first)

            {

                if (SendItemToPlayer(transfer.first, recipient))

                {

                    LogTradeAction("ITEM_TRANSFER", "Sent item " +

                        ::std::to_string(transfer.first->GetEntry()) + " to " + recipient->GetName());

                }

            }


            m_pendingTransfers.pop();
        }
    }
    bool TradeManager::DistributeLoot(::std::vector<Item*> const& items, bool useNeedGreed)
    {
        if (!GetBot() || items.empty())

            return false;

        Group* group = GetBot()->GetGroup();
        if (!group)
        {

            LogTradeAction("DISTRIBUTE_FAILED", "Bot not in group");

            return false;
        }

        // Create distribution plan
        m_currentDistribution = ::std::make_unique<GroupLootInfo>();
        m_currentDistribution->items = items;
        m_currentDistribution->useRoundRobin = !useNeedGreed;
        m_currentDistribution->considerSpec = useNeedGreed;

        // Build distribution plan
        BuildLootDistributionPlan(*m_currentDistribution);

        // Execute distribution
        uint32 distributedCount = 0;
        for (Item* item : items)
        {

            if (!item)

                continue;

            // Find best recipient

            ::std::vector<Player*> candidates;

            // FIXED: Use GroupMemberResolver instead of ObjectAccessor::FindPlayer
            for (Group::MemberSlot const& member : group->GetMemberSlots())

            {

                if (Player* player = GroupMemberResolver::ResolveMember(member.guid))

                {

                    if (player != GetBot() && CanPlayerUseItem(item, player))

                        candidates.push_back(player);

                }

            }


            if (candidates.empty())

                continue;


            Player* recipient = SelectBestRecipient(item, candidates);

            if (recipient)

            {

                m_pendingTransfers.push({item, recipient->GetGUID()});

                distributedCount++;

            }
        }

        LogTradeAction("DISTRIBUTE_LOOT", "Distributed " + ::std::to_string(distributedCount) +

            " items to group");
        return distributedCount > 0;
    }
    bool TradeManager::SendItemToPlayer(Item* item, Player* recipient)
    {
        if (!item || !recipient || !GetBot())

            return false;

        // Check if we can trade with recipient
    if (!ValidateTradeTarget(recipient))

            return false;

        // Initiate trade
    if (!InitiateTrade(recipient, "Sending item"))

            return false;
        // Add item to trade
    if (!AddItemToTrade(item))
        {

            CancelTrade("Failed to add item");

            return false;
        }

        // Auto-accept after a short delay
        m_currentSession.isAccepted = true;

        return true;
    }
    bool TradeManager::RequestItemFromPlayer(uint32 itemEntry, Player* owner)
    {
        if (!owner || !GetBot() || itemEntry == 0)

            return false;

        // Check if owner has the item
        bool hasItem = false;
        for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
        {

            if (Item* item = owner->GetItemByPos(INVENTORY_SLOT_BAG_0, i))

            {

                if (item->GetEntry() == itemEntry)

                {

                    hasItem = true;

                    break;

                }

            }
        }

        if (!hasItem)
        {

            LogTradeAction("REQUEST_ITEM_FAILED", "Owner doesn't have item " + ::std::to_string(itemEntry));

            return false;
        }

        // Initiate trade
        return InitiateTrade(owner, "Requesting item " + ::std::to_string(itemEntry));
    }

    bool TradeManager::ValidateTradeTarget(Player* target) const
    {
        if (!target || !GetBot())

            return false;

        // Check blacklist
    if (IsBlacklisted(target->GetGUID()))
        {

            LogTradeAction("VALIDATE_TARGET_FAILED", "Target is blacklisted");

            return false;
        }

        // Check permissions based on security level
    if (m_securityLevel >= TradeSecurity::SECURITY_BASIC)
        {
            // Must be in same group or guild

            Group* myGroup = GetBot()->GetGroup();

            Group* theirGroup = target->GetGroup();

            bool sameGroup = myGroup && theirGroup && myGroup == theirGroup;

            bool sameGuild = GetBot()->GetGuildId() &&

                           GetBot()->GetGuildId() == target->GetGuildId();

            if (!sameGroup && !sameGuild && !IsWhitelisted(target->GetGUID()))

            {

                LogTradeAction("VALIDATE_TARGET_FAILED", "Target not in group/guild/whitelist");

                return false;

            }
        }

        // Check if target is the group leader (acting as owner)
        Group* myGroup = GetBot()->GetGroup();
        if (myGroup && myGroup->GetLeaderGUID() == target->GetGUID())

            return true;

        // Check whitelist requirement
    if (m_securityLevel >= TradeSecurity::SECURITY_STRICT)
        {

            if (!IsWhitelisted(target->GetGUID()))

            {

                LogTradeAction("VALIDATE_TARGET_FAILED", "Strict mode - target not whitelisted");

                return false;

            }
        }

        return true;
    }

    bool TradeManager::ValidateTradeItems() const
    {
        if (!GetBot())

            return false;
        TradeData* myTrade = GetBot()->GetTradeData();
        if (!myTrade)

            return true;

        // Check each offered item
    for (uint8 slot = 0; slot < MAX_TRADE_ITEMS; ++slot)
        {

            Item* item = myTrade->GetItem(TradeSlots(slot));

            if (!item)

                continue;

            // Validate ownership
    if (!ValidateItemOwnership(item))

            {

                LogTradeAction("VALIDATE_ITEMS_FAILED", "Invalid ownership for item " +

                    ::std::to_string(item->GetEntry()));

                return false;

            }

            // Check if item is protected
    if (m_protectedItems.count(item->GetEntry()) > 0)

            {

                LogTradeAction("VALIDATE_ITEMS_FAILED", "Protected item " +

                    ::std::to_string(item->GetEntry()));

                return false;

            }

            // Check item value against max

            uint32 value = EstimateItemValue(item);

            if (value > m_maxTradeValue)

            {

                LogTradeAction("VALIDATE_ITEMS_FAILED", "Item value exceeds maximum: " +

                    ::std::to_string(value));

                return false;

            }
        }

        return true;
    }

    bool TradeManager::ValidateTradeGold(uint64 amount) const
    {
        if (!GetBot())

            return false;

        // Check against bot's money
    if (amount > GetBot()->GetMoney())

            return false;

        // Check against max trade value
    if (amount > m_maxTradeValue)

            return false;

        return true;
    }

    bool TradeManager::EvaluateTradeFairness() const
    {
        if (m_securityLevel == TradeSecurity::SECURITY_NONE)

            return true;

        // Allow one-sided trades from group leader (acting as owner)
        Group* group = GetBot()->GetGroup();
        if (group && group->GetLeaderGUID() == m_currentSession.traderGuid)

            return true;

        // Allow one-sided trades from other group members
        // FIXED: Use GroupMemberResolver instead of ObjectAccessor::FindPlayer
    if (group)
        {

            for (Group::MemberSlot const& member : group->GetMemberSlots())

            {

                Player* memberPlayer = GroupMemberResolver::ResolveMember(member.guid);

                if (memberPlayer && memberPlayer->GetGUID() == m_currentSession.traderGuid)

                    return true;

            }
        }

        // Check value balance
        return m_currentSession.IsBalanced(0.3f); // 30% tolerance
    }

    bool TradeManager::IsTradeScam() const
    {
        if (m_securityLevel < TradeSecurity::SECURITY_STANDARD)

            return false;

        return CheckForScamPatterns();
    }

    bool TradeManager::IsTradeSafe() const
    {
        // Check for dangerous patterns
    if (IsTradeScam())

            return false;

        // Check item validity
    if (!ValidateTradeItems())

            return false;
        // Check gold validity
    if (!ValidateTradeGold(m_currentSession.offeredGold))

            return false;

        return true;
    }

    uint32 TradeManager::EstimateItemValue(Item* item) const
    {
        if (!item)

            return 0;

        return EstimateItemValue(item->GetEntry(), item->GetCount());
    }
    uint32 TradeManager::EstimateItemValue(uint32 itemEntry, uint32 count) const
    {
        ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemEntry);
        if (!itemTemplate)

            return 0;

        uint32 baseValue = CalculateItemBaseValue(itemTemplate);
        float qualityMult = GetItemQualityMultiplier(ItemQuality(itemTemplate->GetQuality()));
        float levelMult = GetItemLevelMultiplier(itemTemplate->GetBaseItemLevel());

        return uint32(baseValue * qualityMult * levelMult * count);
    }

    bool TradeManager::CanTradeItem(Item* item) const
    {
        if (!item || !GetBot())

            return false;

        ItemTemplate const* proto = item->GetTemplate();
        if (!proto)

            return false;

        // Check if item is bound (soulbound items cannot be traded)
    if (item->IsSoulBound())

            return false;

        // Check if item is conjured (using correct 3.3.5a flag)
    if (proto->HasFlag(ITEM_FLAG_CONJURED))

            return false;

        // Check if it's a quest item (quest items usually have StartQuest)
    if (proto->GetStartQuest() != 0)

            return false;

        // Additional quest item check via bonding
    if (proto->GetBonding() == BIND_QUEST)

            return false;

        // Validate ownership
    if (!ValidateItemOwnership(item))

            return false;

        return true;
    }

    uint8 TradeManager::GetNextFreeTradeSlot() const
    {
        if (!GetBot())

            return MAX_TRADE_ITEMS;

        TradeData* myTrade = GetBot()->GetTradeData();
        if (!myTrade)

            return 0;

        for (uint8 slot = 0; slot < MAX_TRADE_ITEMS; ++slot)
        {

            if (!myTrade->GetItem(TradeSlots(slot)))

                return slot;
        }

        return MAX_TRADE_ITEMS;
    }

    ::std::vector<Item*> TradeManager::GetTradableItems() const
    {
        ::std::vector<Item*> tradableItems;
        if (!GetBot())

            return tradableItems;
        // Check main inventory
    for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
        {

            if (Item* item = GetBot()->GetItemByPos(INVENTORY_SLOT_BAG_0, i))

            {

                if (CanTradeItem(item))

                    tradableItems.push_back(item);

            }
        }

        // Check bags
    for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
        {

            if (Bag* bag = GetBot()->GetBagByPos(i))

            {

                for (uint32 j = 0; j < bag->GetBagSize(); ++j)

                {

                    if (Item* item = bag->GetItemByPos(j))

                    {

                        if (CanTradeItem(item))

                            tradableItems.push_back(item);

                    }

                }

            }
        }

        return tradableItems;
    }

    void TradeManager::SetTradeState(TradeState newState)
    {
        TradeState oldState = m_currentSession.state;
        m_currentSession.state = newState;

        LogTradeAction("STATE_CHANGE", "State changed from " + ::std::to_string(uint8(oldState)) +

            " to " + ::std::to_string(uint8(newState)));
    }

    void TradeManager::ResetTradeSession()
    {
        m_currentSession.Reset();
        m_updateTimer = 0;
    }

    void TradeManager::UpdateTradeWindow()
    {
        if (!GetBot() || !IsTrading())

            return;

        TradeData* myTrade = GetBot()->GetTradeData();
        TradeData* theirTrade = myTrade ? myTrade->GetTraderData() : nullptr;

        if (myTrade && theirTrade)
        {

            OnTradeStatusUpdate(myTrade, theirTrade);
        }
    }

    bool TradeManager::ProcessTradeUpdate(uint32 diff)
    {
        if (!IsTrading())

            return false;

        UpdateTradeWindow();

        // Auto-accept logic based on state
    if (m_currentSession.state == TradeState::REVIEWING)
        {
            // Check if we should auto-accept

            bool shouldAutoAccept = false;

            // Auto-accept from group leader (acting as owner)

            Group* group = GetBot()->GetGroup();

            if (group && group->GetLeaderGUID() == m_currentSession.traderGuid)

                shouldAutoAccept = true;

            // Auto-accept from group members
            // FIXED: Use GroupMemberResolver instead of ObjectAccessor::FindPlayer
    if (m_autoAcceptGroup && !shouldAutoAccept)

            {

                Group* group = GetBot()->GetGroup();

                if (group)

                {

                    for (Group::MemberSlot const& member : group->GetMemberSlots())

                    {

                        Player* memberPlayer = GroupMemberResolver::ResolveMember(member.guid);

                        if (memberPlayer && memberPlayer->GetGUID() == m_currentSession.traderGuid)

                        {

                            shouldAutoAccept = true;

                            break;

                        }

                    }

                }

            }

            // Auto-accept from guild members
    if (m_autoAcceptGuild && !shouldAutoAccept)

            {

                Player* trader = ObjectAccessor::FindPlayer(m_currentSession.traderGuid);

                if (trader && GetBot()->GetGuildId() &&

                    GetBot()->GetGuildId() == trader->GetGuildId())

                {

                    shouldAutoAccept = true;

                }

            }

            // Auto-accept from whitelist
    if (m_autoAcceptWhitelist && !shouldAutoAccept)

            {

                if (IsWhitelisted(m_currentSession.traderGuid))

                    shouldAutoAccept = true;

            }


            if (shouldAutoAccept && !m_currentSession.isAccepted)

            {
                // Small delay before auto-accepting

                auto elapsed = ::std::chrono::duration_cast<::std::chrono::milliseconds>(

                    ::std::chrono::steady_clock::now() - m_currentSession.lastUpdate).count();


                if (elapsed > 2000) // 2 second delay

                {

                    AcceptTrade();

                }

            }
        }

        return true;
    }

    uint32 TradeManager::CalculateItemBaseValue(ItemTemplate const* itemTemplate) const
    {
        if (!itemTemplate)

            return 0;

        // Use vendor price as base
        uint32 baseValue = itemTemplate->GetSellPrice();

        // If no sell price, estimate based on item level and quality
    if (baseValue == 0)
        {

            baseValue = itemTemplate->GetBaseItemLevel() * 100;

            // Adjust by item class
    switch (itemTemplate->GetClass())

            {

                case ITEM_CLASS_WEAPON:

                    baseValue *= 3;

                    break;

                case ITEM_CLASS_ARMOR:

                    baseValue *= 2;

                    break;

                case ITEM_CLASS_CONSUMABLE:

                    baseValue /= 2;

                    break;

                case ITEM_CLASS_TRADE_GOODS:

                    baseValue /= 3;

                    break;

            }
        }

        return baseValue;
    }

    float TradeManager::GetItemQualityMultiplier(ItemQuality quality) const
    {
        // Delegate to centralized config for consistency across all trade systems
        return PlayerbotTradeConfig::GetItemQualityMultiplier(static_cast<uint8>(quality));
    }

    float TradeManager::GetItemLevelMultiplier(uint32 itemLevel) const
    {
        // Delegate to centralized config for consistency across all trade systems
        return PlayerbotTradeConfig::GetItemLevelMultiplier(itemLevel);
    }

    bool TradeManager::IsItemNeededByBot(uint32 itemEntry) const
    {
        if (!GetBot() || !GetAI())

            return false;

        ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemEntry);
        if (!itemTemplate)

            return false;

        // Check if bot can use the item
    if (!GetBot()->CanUseItem(itemTemplate))

            return false;

        // Check if it's an upgrade
        // This would need more complex logic based on bot's current gear
        // For now, return true if usable
        return true;
    }

    bool TradeManager::IsItemUsableByBot(Item* item) const
    {
        if (!item || !GetBot())

            return false;

        return GetBot()->CanUseItem(item) == EQUIP_ERR_OK;
    }

    Player* TradeManager::SelectBestRecipient(Item* item, ::std::vector<Player*> const& candidates) const
    {
        if (candidates.empty() || !item)

            return nullptr;

        Player* bestCandidate = nullptr;
        uint32 highestPriority = 0;

        for (Player* candidate : candidates)
        {

            if (!candidate)

                continue;


            uint32 priority = CalculateItemPriority(item, candidate);

            if (priority > highestPriority)

            {

                highestPriority = priority;

                bestCandidate = candidate;

            }
        }

        return bestCandidate;
    }

    uint32 TradeManager::CalculateItemPriority(Item* item, Player* player) const
    {
        if (!item || !player)

            return 0;

        uint32 priority = 0;

        // Check if player can use item
    if (player->CanUseItem(item) != EQUIP_ERR_OK)

            return 0;

        ItemTemplate const* itemTemplate = item->GetTemplate();
        if (!itemTemplate)

            return 0;

        // Base priority on item quality
        priority += uint32(itemTemplate->GetQuality()) * 100;

        // Check if it's an upgrade
        uint16 slot = itemTemplate->GetInventoryType();
        if (slot < EQUIPMENT_SLOT_END)
        {

            if (Item* currentItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))

            {

                if (itemTemplate->GetBaseItemLevel() > currentItem->GetTemplate()->GetBaseItemLevel())

                    priority += 500;

            }

            else

            {
                // Empty slot, high priority

                priority += 1000;

            }
        }

        // Check class match (3.3.5a doesn't have spec-specific items)
        // Could check talent trees but that's complex, so just use class
    if ((itemTemplate->GetAllowableClass() & player->GetClassMask()) != 0)

            priority += 200;

        return priority;
    }

    bool TradeManager::CanPlayerUseItem(Item* item, Player* player) const
    {
        if (!item || !player)

            return false;

        return player->CanUseItem(item) == EQUIP_ERR_OK;
    }

    void TradeManager::BuildLootDistributionPlan(GroupLootInfo& distribution)
    {
        if (!GetBot())

            return;

        Group* group = GetBot()->GetGroup();
        if (!group)

            return;

        // Build player priorities
        // FIXED: Use GroupMemberResolver instead of ObjectAccessor::FindPlayer
    for (Group::MemberSlot const& member : group->GetMemberSlots())
        {

            if (Player* player = GroupMemberResolver::ResolveMember(member.guid))

            {

                distribution.playerPriorities[player->GetGUID()] = 0;
                // Build needs list
    for (Item* item : distribution.items)

                {

                    if (item && IsItemNeededByBot(item->GetEntry()))

                    {

                        distribution.playerNeeds[player->GetGUID()].push_back(item->GetEntry());

                    }

                }

            }
        }
    }

    bool TradeManager::ValidateTradeDistance(Player* trader) const
    {
        if (!trader || !GetBot())

            return false;

        return GetBot()->GetDistance(trader) <= m_maxTradeDistance;
    }

    bool TradeManager::ValidateTradePermissions(Player* trader) const
    {
        return ValidateTradeTarget(trader);
    }

    bool TradeManager::ValidateItemOwnership(Item* item) const
    {
        if (!item || !GetBot())

            return false;

        return item->GetOwnerGUID() == GetBot()->GetGUID();
    }

    bool TradeManager::CheckForScamPatterns() const
    {
        // Check for common scam patterns

        // Pattern 1: High value given for nothing
    if (m_currentSession.GetTotalOfferedValue() > 1000 * GOLD &&

            m_currentSession.GetTotalReceivedValue() < 100 * GOLD)
        {

            return true;
        }

        // Pattern 2: Suspicious item combinations
        bool hasExpensiveItem = false;
        bool hasJunkItem = false;

        for (auto const& item : m_currentSession.offeredItems)
        {

            if (item.quality >= ITEM_QUALITY_EPIC)

                hasExpensiveItem = true;

            if (item.quality == ITEM_QUALITY_POOR)

                hasJunkItem = true;
        }

        // Offering epic+ items with junk is suspicious
    if (hasExpensiveItem && hasJunkItem)

            return true;

        return false;
    }

    bool TradeManager::CheckValueBalance() const
    {
        // Use value tolerance from config (scam protection threshold)
        return m_currentSession.IsBalanced(PlayerbotTradeConfig::GetTradeValueTolerance());
    }

    bool TradeManager::ExecuteTrade()
    {
        if (!GetBot() || !IsTrading())

            return false;

        TradeData* myTrade = GetBot()->GetTradeData();
        if (!myTrade)

            return false;

        // Final validation
    if (!IsTradeSafe())
        {

            CancelTrade("Trade failed safety check");

            return false;
        }

        // Set accepted
        myTrade->SetAccepted(true, true);
        return true;
    }

    void TradeManager::ProcessTradeCompletion()
    {
        auto endTime = ::std::chrono::steady_clock::now();
        auto duration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(

            endTime - m_currentSession.startTime);

        // Update statistics
        m_statistics.totalTrades++;
        m_statistics.successfulTrades++;
        m_statistics.totalGoldTraded += m_currentSession.offeredGold;
        m_statistics.totalItemsTraded += static_cast<uint32>(m_currentSession.offeredItems.size());
        m_statistics.totalTradeTime += duration;
        m_statistics.lastTradeTime = endTime;

        LogTradeCompletion(true);
        SetTradeState(TradeState::COMPLETED);

        // Trade data is cleaned up by TradeCancel/trade completion
        // No need to manually set it in 3.3.5a

        ResetTradeSession();
    }

    void TradeManager::ProcessTradeCancellation(::std::string const& reason)
    {
        // Update statistics
        m_statistics.totalTrades++;
        m_statistics.cancelledTrades++;

        LogTradeAction("TRADE_CANCELLED", reason);
        SetTradeState(TradeState::CANCELLED);

        // Send cancel to other party
    if (GetBot())
        {

            GetBot()->TradeCancel(true);
            // TradeCancel handles cleaning up m_trade internally
        }

        ResetTradeSession();
    }

    void TradeManager::HandleTradeError(::std::string const& error)
    {
        m_statistics.totalTrades++;
        m_statistics.failedTrades++;

        LogTradeAction("TRADE_ERROR", error);
        SetTradeState(TradeState::ERROR_STATE);

        CancelTrade(error);
    }

    void TradeManager::LogTradeAction(::std::string const& action, ::std::string const& details) const
    {
        if (!GetBot())

            return;

        TC_LOG_DEBUG("bot.trade", "Bot {} - {}: {}",

            GetBot()->GetName(), action, details);
    }

    void TradeManager::LogTradeItem(Item* item, bool offered)
    {
        if (!item || !GetBot())

            return;

        ItemTemplate const* itemTemplate = item->GetTemplate();
        if (!itemTemplate)

            return;

        TC_LOG_DEBUG("bot.trade", "Bot {} - {} item: {} x{} (value: {})",

            GetBot()->GetName(),

            offered ? "Offering" : "Receiving",

            itemTemplate->GetDefaultLocaleName(),

            item->GetCount(),

            EstimateItemValue(item));
    }

    void TradeManager::LogTradeCompletion(bool success)
    {
        if (!GetBot())

            return;

        if (success)
        {

            TC_LOG_INFO("bot.trade", "Bot {} completed trade - Gave: {} gold, {} items | Received: {} gold, {} items",

                GetBot()->GetName(),

                m_currentSession.offeredGold,

                m_currentSession.offeredItems.size(),

                m_currentSession.receivedGold,

                m_currentSession.receivedItems.size());
        }
        else
        {

            TC_LOG_INFO("bot.trade", "Bot {} failed trade with {}",

                GetBot()->GetName(),

                m_currentSession.traderGuid.ToString());
        }
    }
}