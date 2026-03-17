/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "AuctionEvents.h"
#include <sstream>

namespace Playerbot
{

AuctionEvent AuctionEvent::CommandResult(ObjectGuid playerGuid, uint32 auctionId, uint32 command, uint32 errorCode)
{
    AuctionEvent event;
    event.type = AuctionEventType::AUCTION_COMMAND_RESULT;
    event.playerGuid = playerGuid;
    event.auctionId = auctionId;
    event.command = command;
    event.errorCode = errorCode;
    event.itemId = 0;
    event.itemCount = 0;
    event.bidAmount = 0;
    event.buyoutAmount = 0;
    event.timestamp = std::chrono::steady_clock::now();
    return event;
}

AuctionEvent AuctionEvent::ListReceived(ObjectGuid playerGuid, uint32 itemCount)
{
    AuctionEvent event;
    event.type = AuctionEventType::AUCTION_LIST_RECEIVED;
    event.playerGuid = playerGuid;
    event.itemCount = itemCount;
    event.auctionId = 0;
    event.itemId = 0;
    event.bidAmount = 0;
    event.buyoutAmount = 0;
    event.command = 0;
    event.errorCode = 0;
    event.timestamp = std::chrono::steady_clock::now();
    return event;
}

AuctionEvent AuctionEvent::BidPlaced(ObjectGuid playerGuid, uint32 auctionId, uint64 bidAmount)
{
    AuctionEvent event;
    event.type = AuctionEventType::AUCTION_BID_PLACED;
    event.playerGuid = playerGuid;
    event.auctionId = auctionId;
    event.bidAmount = bidAmount;
    event.itemId = 0;
    event.itemCount = 0;
    event.buyoutAmount = 0;
    event.command = 0;
    event.errorCode = 0;
    event.timestamp = std::chrono::steady_clock::now();
    return event;
}

AuctionEvent AuctionEvent::AuctionWon(ObjectGuid playerGuid, uint32 auctionId, uint32 itemId, uint64 bidAmount)
{
    AuctionEvent event;
    event.type = AuctionEventType::AUCTION_WON;
    event.playerGuid = playerGuid;
    event.auctionId = auctionId;
    event.itemId = itemId;
    event.bidAmount = bidAmount;
    event.itemCount = 0;
    event.buyoutAmount = 0;
    event.command = 0;
    event.errorCode = 0;
    event.timestamp = std::chrono::steady_clock::now();
    return event;
}

AuctionEvent AuctionEvent::Outbid(ObjectGuid playerGuid, uint32 auctionId, uint64 newBid)
{
    AuctionEvent event;
    event.type = AuctionEventType::AUCTION_OUTBID;
    event.playerGuid = playerGuid;
    event.auctionId = auctionId;
    event.bidAmount = newBid;
    event.itemId = 0;
    event.itemCount = 0;
    event.buyoutAmount = 0;
    event.command = 0;
    event.errorCode = 0;
    event.timestamp = std::chrono::steady_clock::now();
    return event;
}

AuctionEvent AuctionEvent::Expired(ObjectGuid playerGuid, uint32 auctionId, uint32 itemId)
{
    AuctionEvent event;
    event.type = AuctionEventType::AUCTION_EXPIRED;
    event.playerGuid = playerGuid;
    event.auctionId = auctionId;
    event.itemId = itemId;
    event.itemCount = 0;
    event.bidAmount = 0;
    event.buyoutAmount = 0;
    event.command = 0;
    event.errorCode = 0;
    event.timestamp = std::chrono::steady_clock::now();
    return event;
}

bool AuctionEvent::IsValid() const
{
    if (type >= AuctionEventType::MAX_AUCTION_EVENT)
        return false;
    if (playerGuid.IsEmpty())
        return false;
    return true;
}

std::string AuctionEvent::ToString() const
{
    std::ostringstream oss;
    oss << "[AuctionEvent] Type: " << static_cast<uint32>(type)
        << ", Player: " << playerGuid.ToString()
        << ", Auction: " << auctionId
        << ", Bid: " << bidAmount;
    return oss.str();
}

} // namespace Playerbot
