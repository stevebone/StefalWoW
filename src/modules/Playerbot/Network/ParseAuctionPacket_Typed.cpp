/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PlayerbotPacketSniffer.h"
#include "Core/Events/GenericEventBus.h"
#include "Auction/AuctionEvents.h"
#include "AuctionHousePackets.h"
#include "Player.h"
#include "WorldSession.h"
#include "Log.h"

namespace Playerbot
{

/**
 * @brief SMSG_AUCTION_COMMAND_RESULT - Auction command result (bid, buyout, create)
 */
void ParseTypedAuctionCommandResult(WorldSession* session, WorldPackets::AuctionHouse::AuctionCommandResult const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    AuctionEvent event = AuctionEvent::CommandResult(
        bot->GetGUID(),
        packet.AuctionID,
        packet.Command,
        packet.ErrorCode
    );

    EventBus<AuctionEvent>::instance()->PublishEvent(event);

    TC_LOG_TRACE("playerbot.packets", "Bot {} received AUCTION_COMMAND_RESULT (typed): auction={}, cmd={}, error={}",
        bot->GetName(), packet.AuctionID, packet.Command, packet.ErrorCode);
}

/**
 * @brief SMSG_AUCTION_LIST_BUCKETS_RESULT - Auction list received (buckets view)
 */
void ParseTypedAuctionListBucketsResult(WorldSession* session, WorldPackets::AuctionHouse::AuctionListBucketsResult const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    AuctionEvent event = AuctionEvent::ListReceived(
        bot->GetGUID(),
        static_cast<uint32>(packet.Buckets.size())
    );

    EventBus<AuctionEvent>::instance()->PublishEvent(event);

    TC_LOG_TRACE("playerbot.packets", "Bot {} received AUCTION_LIST_BUCKETS_RESULT (typed): {} buckets",
        bot->GetName(), packet.Buckets.size());
}

/**
 * @brief SMSG_AUCTION_LIST_ITEMS_RESULT - Auction list received (items view)
 */
void ParseTypedAuctionListItemsResult(WorldSession* session, WorldPackets::AuctionHouse::AuctionListItemsResult const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    AuctionEvent event = AuctionEvent::ListReceived(
        bot->GetGUID(),
        static_cast<uint32>(packet.Items.size())
    );

    EventBus<AuctionEvent>::instance()->PublishEvent(event);
    TC_LOG_TRACE("playerbot.packets", "Bot {} received AUCTION_LIST_ITEMS_RESULT (typed): {} items",
        bot->GetName(), packet.Items.size());
}

/**
 * @brief SMSG_AUCTION_WON_NOTIFICATION - Bot won an auction
 */
void ParseTypedAuctionWonNotification(WorldSession* session, WorldPackets::AuctionHouse::AuctionWonNotification const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    // Extract item ID from the notification
    uint32 itemId = packet.Info.Item.ItemID;

    AuctionEvent event = AuctionEvent::AuctionWon(
        bot->GetGUID(),
        packet.Info.AuctionID,
        itemId,
        0  // Bid amount not available in this packet
    );

    EventBus<AuctionEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received AUCTION_WON_NOTIFICATION (typed): auction={}, item={}",
        bot->GetName(), packet.Info.AuctionID, itemId);
}

/**
 * @brief SMSG_AUCTION_OUTBID_NOTIFICATION - Bot was outbid on an auction
 */
void ParseTypedAuctionOutbidNotification(WorldSession* session, WorldPackets::AuctionHouse::AuctionOutbidNotification const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    AuctionEvent event = AuctionEvent::Outbid(
        bot->GetGUID(),
        packet.Info.AuctionID,
        packet.BidAmount
    );

    EventBus<AuctionEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received AUCTION_OUTBID_NOTIFICATION (typed): auction={}, newBid={}",
        bot->GetName(), packet.Info.AuctionID, packet.BidAmount);
}

/**
 * @brief SMSG_AUCTION_CLOSED_NOTIFICATION - Auction expired or sold
 */
void ParseTypedAuctionClosedNotification(WorldSession* session, WorldPackets::AuctionHouse::AuctionClosedNotification const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    // If not sold, it expired
    if (!packet.Sold)
    {
        uint32 itemId = packet.Info.Item.ItemID;

        AuctionEvent event = AuctionEvent::Expired(
            bot->GetGUID(),
            packet.Info.AuctionID,
            itemId
        );

        EventBus<AuctionEvent>::instance()->PublishEvent(event);

        TC_LOG_DEBUG("playerbot.packets", "Bot {} received AUCTION_CLOSED_NOTIFICATION (typed): auction={}, item={}, expired={}",
            bot->GetName(), packet.Info.AuctionID, itemId, !packet.Sold);
    }
}

/**
 * @brief Register all auction packet typed handlers
 */
void RegisterAuctionPacketHandlers()
{
    // Register command result handler
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::AuctionHouse::AuctionCommandResult>(&ParseTypedAuctionCommandResult);

    // Register list result handlers
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::AuctionHouse::AuctionListBucketsResult>(&ParseTypedAuctionListBucketsResult);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::AuctionHouse::AuctionListItemsResult>(&ParseTypedAuctionListItemsResult);

    // Register notification handlers
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::AuctionHouse::AuctionWonNotification>(&ParseTypedAuctionWonNotification);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::AuctionHouse::AuctionOutbidNotification>(&ParseTypedAuctionOutbidNotification);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::AuctionHouse::AuctionClosedNotification>(&ParseTypedAuctionClosedNotification);

    TC_LOG_INFO("playerbot", "PlayerbotPacketSniffer: Registered {} Auction packet typed handlers", 6);
}

} // namespace Playerbot
