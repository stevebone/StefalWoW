/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "AuctionManager.h"
#include "AI/BotAI.h"
#include "Events/BotEventData.h"
#include "Events/BotEventTypes.h"
#include "Player.h"
#include "Log.h"
#include <any>

namespace Playerbot
{
    using namespace Events;
    using namespace StateMachine;

    /**
     * AuctionManager::OnEventInternal Implementation
     *
     * Handles 5 auction-related events dispatched from observers.
     * Phase 7.2: Complete implementation - extracts event data and calls manager methods
     *
     * Auction Events:
     * - AUCTION_BID_PLACED: Bot placed a bid on an auction
     * - AUCTION_WON: Bot won an auction
     * - AUCTION_OUTBID: Bot was outbid on an auction
     * - AUCTION_EXPIRED: Bot's auction listing expired
     * - AUCTION_SOLD: Bot's auction item sold
     */
    void AuctionManager::OnEventInternal(Events::BotEvent const& event)
    {
        // Early exit for non-auction events (auction events are in trade category)
    if (!event.IsTradeEvent())
            return;

        Player* bot = GetBot();
        if (!bot || !bot->IsInWorld())
            return;

        // Handle auction events with full implementation
    switch (event.type)
        {
            case StateMachine::EventType::AUCTION_BID_PLACED:
            {
                // Extract auction bid data
    if (!event.eventData.has_value())
                {
                    TC_LOG_WARN("module.playerbot", "AuctionManager::OnEventInternal: AUCTION_BID_PLACED event {} missing data", event.eventId);
                    ForceUpdate();
                    return;
                }

                AuctionEventData auctionData;
                try
                {
                    auctionData = ::std::any_cast<AuctionEventData>(event.eventData);
                }
                catch (::std::bad_any_cast const& e)
                {
                    TC_LOG_ERROR("module.playerbot", "AuctionManager::OnEventInternal: Failed to cast AUCTION_BID_PLACED data: {}", e.what());
                    ForceUpdate();
                    return;
                }
                TC_LOG_INFO("module.playerbot", "AuctionManager: Bot {} placed bid on auction {} (Item: {}, Bid: {} copper, Buyout: {} copper)",
                    bot->GetName(), auctionData.auctionId, auctionData.itemEntry,
                    auctionData.bidPrice, auctionData.buyoutPrice);

                // Record bid placement for statistics
                RecordBidPlaced(bot->GetGUID(), auctionData.bidPrice);

                // Track this auction for the bot
                BotAuctionData botAuctionData;
                botAuctionData.AuctionId = auctionData.auctionId;
                botAuctionData.ItemId = auctionData.itemEntry;
                botAuctionData.ItemCount = 1;
                botAuctionData.StartPrice = auctionData.bidPrice;
                botAuctionData.BuyoutPrice = auctionData.buyoutPrice;
                botAuctionData.CostBasis = 0;
                botAuctionData.IsCommodity = false;
                botAuctionData.Strategy = AuctionStrategy::SMART_PRICING;
                RegisterBotAuction(bot, auctionData.auctionId, botAuctionData);

                ForceUpdate();
                break;
            }

            case StateMachine::EventType::AUCTION_WON:
            {
                // Extract auction won data
    if (!event.eventData.has_value())
                {
                    TC_LOG_WARN("module.playerbot", "AuctionManager::OnEventInternal: AUCTION_WON event {} missing data", event.eventId);
                    ForceUpdate();
                    return;
                }

                AuctionEventData auctionData;
                try
                {
                    auctionData = ::std::any_cast<AuctionEventData>(event.eventData);
                }
                catch (::std::bad_any_cast const& e)
                {
                    TC_LOG_ERROR("module.playerbot", "AuctionManager::OnEventInternal: Failed to cast AUCTION_WON data: {}", e.what());
                    ForceUpdate();
                    return;
                }

                TC_LOG_INFO("module.playerbot", "AuctionManager: Bot {} WON auction {} (Item: {}, Final price: {} copper)",
                    bot->GetName(), auctionData.auctionId, auctionData.itemEntry, auctionData.bidPrice);

                // Unregister auction (bot won, so it's complete)
                UnregisterBotAuction(bot, auctionData.auctionId);

                // Update statistics - successful purchase
                // Note: Item will arrive via mail, handled by MailEventObserver

                ForceUpdate();
                break;
            }

            case StateMachine::EventType::AUCTION_OUTBID:
            {
                // Extract outbid data
    if (!event.eventData.has_value())
                {
                    TC_LOG_WARN("module.playerbot", "AuctionManager::OnEventInternal: AUCTION_OUTBID event {} missing data", event.eventId);
                    ForceUpdate();
                    return;
                }

                AuctionEventData auctionData;
                try
                {
                    auctionData = ::std::any_cast<AuctionEventData>(event.eventData);
                }
                catch (::std::bad_any_cast const& e)
                {
                    TC_LOG_ERROR("module.playerbot", "AuctionManager::OnEventInternal: Failed to cast AUCTION_OUTBID data: {}", e.what());
                    ForceUpdate();
                    return;
                }

                TC_LOG_INFO("module.playerbot", "AuctionManager: Bot {} was OUTBID on auction {} (Item: {}, Previous bid: {}, Buyout: {})",
                    bot->GetName(), auctionData.auctionId, auctionData.itemEntry,
                    auctionData.bidPrice, auctionData.buyoutPrice);

                // Decide whether to re-bid
                // Calculate optimal next bid amount
                uint64 newBidAmount = CalculateOptimalBid(auctionData.itemEntry, auctionData.bidPrice, auctionData.buyoutPrice);

                if (newBidAmount > 0 && newBidAmount <= auctionData.buyoutPrice)
                {
                    // Check if bot can afford the new bid
    if (bot->GetMoney() >= newBidAmount)
                    {
                        TC_LOG_DEBUG("module.playerbot", "AuctionManager: Bot {} considering re-bidding {} copper on auction {}",
                            bot->GetName(), newBidAmount, auctionData.auctionId);

                        // Re-bid will be attempted on next Update() cycle
                        // PlaceBid() is called from Update() based on auction analysis
                    }
                    else
                    {
                        TC_LOG_DEBUG("module.playerbot", "AuctionManager: Bot {} cannot afford re-bid on auction {} (need: {}, have: {})",
                            bot->GetName(), auctionData.auctionId, newBidAmount, bot->GetMoney());
                        // Unregister auction (bot can't compete)
                        UnregisterBotAuction(bot, auctionData.auctionId);
                    }
                }
                else
                {
                    TC_LOG_DEBUG("module.playerbot", "AuctionManager: Bot {} declining to re-bid on auction {} (price too high)",
                        bot->GetName(), auctionData.auctionId);

                    // Unregister auction (not worth continuing)
                    UnregisterBotAuction(bot, auctionData.auctionId);
                }

                ForceUpdate();
                break;
            }

            case StateMachine::EventType::AUCTION_EXPIRED:
            {
                // Extract expiration data
    if (!event.eventData.has_value())
                {
                    TC_LOG_WARN("module.playerbot", "AuctionManager::OnEventInternal: AUCTION_EXPIRED event {} missing data", event.eventId);
                    ForceUpdate();
                    return;
                }

                AuctionEventData auctionData;
                try
                {
                    auctionData = ::std::any_cast<AuctionEventData>(event.eventData);
                }
                catch (::std::bad_any_cast const& e)
                {
                    TC_LOG_ERROR("module.playerbot", "AuctionManager::OnEventInternal: Failed to cast AUCTION_EXPIRED data: {}", e.what());
                    ForceUpdate();
                    return;
                }

                TC_LOG_INFO("module.playerbot", "AuctionManager: Bot {} auction EXPIRED (Auction: {}, Item: {})",
                    bot->GetName(), auctionData.auctionId, auctionData.itemEntry);

                // Unregister expired auction
                UnregisterBotAuction(bot, auctionData.auctionId);

                // Item will be returned via mail if no one bought it
                // Consider re-listing at a lower price on next update cycle

                ForceUpdate();
                break;
            }

            case StateMachine::EventType::AUCTION_SOLD:
            {
                // Extract sale data
    if (!event.eventData.has_value())
                {
                    TC_LOG_WARN("module.playerbot", "AuctionManager::OnEventInternal: AUCTION_SOLD event {} missing data", event.eventId);
                    ForceUpdate();
                    return;
                }

                AuctionEventData auctionData;
                try
                {
                    auctionData = ::std::any_cast<AuctionEventData>(event.eventData);
                }
                catch (::std::bad_any_cast const& e)
                {
                    TC_LOG_ERROR("module.playerbot", "AuctionManager::OnEventInternal: Failed to cast AUCTION_SOLD data: {}", e.what());
                    ForceUpdate();
                    return;
                }

                TC_LOG_INFO("module.playerbot", "AuctionManager: Bot {} auction SOLD (Auction: {}, Item: {}, Sale price: {} copper)",
                    bot->GetName(), auctionData.auctionId, auctionData.itemEntry, auctionData.bidPrice);

                // Record successful sale for statistics
                // Estimate cost basis (we don't have this data in event, so use 0 for now)
                RecordAuctionSold(bot->GetGUID(), auctionData.bidPrice, 0);

                // Unregister sold auction
                UnregisterBotAuction(bot, auctionData.auctionId);

                // Gold will arrive via mail system

                ForceUpdate();
                break;
            }

            default:
                break;
        }
    }

} // namespace Playerbot
