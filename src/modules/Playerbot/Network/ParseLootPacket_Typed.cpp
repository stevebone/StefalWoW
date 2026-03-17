/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PlayerbotPacketSniffer.h"
#include "WorldSession.h"
#include "Player.h"
#include "Core/Events/GenericEventBus.h"
#include "Loot/LootEvents.h"
#include "LootPackets.h"
#include "Log.h"

namespace Playerbot
{

// ================================================================================================
// TYPED PACKET HANDLERS - LOOT CATEGORY
// Full implementation of all 11 loot packet handlers
// ================================================================================================

void ParseTypedLootResponse(WorldSession* session, WorldPackets::Loot::LootResponse const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    LootEvent event;
    event.type = LootEventType::LOOT_WINDOW_OPENED;
    event.priority = LootEventPriority::HIGH;
    event.looterGuid = bot->GetGUID();
    event.itemGuid = packet.Owner;  // The object being looted
    event.itemEntry = 0;
    event.itemCount = packet.Items.size();
    event.lootType = LootType::CORPSE;
    event.timestamp = ::std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + ::std::chrono::seconds(30);

    EventBus<LootEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received LOOT_RESPONSE (typed): {} items, {} copper",
        bot->GetName(), packet.Items.size(), packet.Coins);
}

void ParseTypedLootReleaseResponse(WorldSession* session, WorldPackets::Loot::LootReleaseResponse const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    LootEvent event;
    event.type = LootEventType::LOOT_WINDOW_CLOSED;
    event.priority = LootEventPriority::MEDIUM;
    event.looterGuid = bot->GetGUID();
    event.itemGuid = packet.LootObj;
    event.itemEntry = 0;
    event.itemCount = 0;
    event.lootType = LootType::CORPSE;
    event.timestamp = ::std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + ::std::chrono::seconds(5);

    EventBus<LootEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received LOOT_RELEASE_RESPONSE (typed)",
        bot->GetName());
}

void ParseTypedLootRemoved(WorldSession* session, WorldPackets::Loot::LootRemoved const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    LootEvent event;
    event.type = LootEventType::LOOT_REMOVED;
    event.priority = LootEventPriority::MEDIUM;
    event.looterGuid = bot->GetGUID();
    event.itemGuid = packet.Owner;
    event.itemEntry = 0;  // Slot ID is in packet.LootListID
    event.itemCount = 0;
    event.lootType = LootType::CORPSE;
    event.timestamp = ::std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + ::std::chrono::seconds(5);

    EventBus<LootEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received LOOT_REMOVED (typed): slot={}",
        bot->GetName(), packet.LootListID);
}

void ParseTypedLootMoneyNotify(WorldSession* session, WorldPackets::Loot::LootMoneyNotify const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    LootEvent event;
    event.type = LootEventType::LOOT_MONEY_RECEIVED;
    event.priority = LootEventPriority::MEDIUM;
    event.looterGuid = bot->GetGUID();
    event.itemGuid = ObjectGuid::Empty;
    event.itemEntry = 0;
    event.itemCount = packet.Money;  // Store copper amount in itemCount
    event.lootType = LootType::CORPSE;
    event.timestamp = ::std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + ::std::chrono::seconds(5);

    EventBus<LootEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received LOOT_MONEY_NOTIFY (typed): {} copper, soleLooter={}",
        bot->GetName(), packet.Money, packet.SoleLooter);
}

void ParseTypedStartLootRoll(WorldSession* session, WorldPackets::Loot::StartLootRoll const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    LootEvent event;
    event.type = LootEventType::LOOT_ROLL_STARTED;
    event.priority = LootEventPriority::HIGH;
    event.looterGuid = bot->GetGUID();
    event.itemGuid = packet.LootObj;
    event.itemEntry = packet.Item.Loot.ItemID;  // WoW 12.0: LootItemData has ItemInstance Loot
    event.itemCount = packet.Item.Quantity;
    event.lootType = LootType::CORPSE;
    event.timestamp = ::std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + ::std::chrono::milliseconds(packet.RollTime);

    EventBus<LootEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received START_LOOT_ROLL (typed): item={} x{}",
        bot->GetName(), packet.Item.Loot.ItemID, packet.Item.Quantity);
}

void ParseTypedLootRoll(WorldSession* session, WorldPackets::Loot::LootRollBroadcast const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    LootEvent event;
    event.type = LootEventType::LOOT_ROLL_CAST;
    event.priority = LootEventPriority::MEDIUM;
    event.looterGuid = packet.Player;  // The player who rolled
    event.itemGuid = packet.LootObj;
    event.itemEntry = packet.Item.Loot.ItemID;
    event.itemCount = packet.Roll;  // Store roll value in itemCount
    event.lootType = LootType::CORPSE;
    event.timestamp = ::std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + ::std::chrono::seconds(5);

    EventBus<LootEvent>::instance()->PublishEvent(event);
    TC_LOG_DEBUG("playerbot.packets", "Bot {} received LOOT_ROLL (typed): player={}, rollType={}, roll={}",
        bot->GetName(), packet.Player.ToString(), static_cast<uint32>(packet.RollType), packet.Roll);
}

void ParseTypedLootRollWon(WorldSession* session, WorldPackets::Loot::LootRollWon const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    LootEvent event;
    event.type = LootEventType::LOOT_ROLL_WON;
    event.priority = LootEventPriority::HIGH;
    event.looterGuid = packet.Winner;  // The player who won
    event.itemGuid = packet.LootObj;
    event.itemEntry = packet.Item.Loot.ItemID;  // WoW 12.0: LootItemData has ItemInstance Loot
    event.itemCount = packet.Item.Quantity;
    event.lootType = LootType::CORPSE;
    event.timestamp = ::std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + ::std::chrono::seconds(10);

    EventBus<LootEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received LOOT_ROLL_WON (typed): winner={}, item={}, roll={}",
        bot->GetName(), packet.Winner.ToString(), packet.Item.Loot.ItemID, packet.Roll);
}

void ParseTypedLootAllPassed(WorldSession* session, WorldPackets::Loot::LootAllPassed const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    LootEvent event;
    event.type = LootEventType::LOOT_ALL_PASSED;
    event.priority = LootEventPriority::MEDIUM;
    event.looterGuid = bot->GetGUID();
    event.itemGuid = packet.LootObj;
    event.itemEntry = packet.Item.Loot.ItemID;  // WoW 12.0: LootItemData has ItemInstance Loot
    event.itemCount = packet.Item.Quantity;
    event.lootType = LootType::CORPSE;
    event.timestamp = ::std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + ::std::chrono::seconds(10);

    EventBus<LootEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received LOOT_ALL_PASSED (typed): item={}",
        bot->GetName(), packet.Item.Loot.ItemID);
}

void ParseTypedMasterLootCandidateList(WorldSession* session, WorldPackets::Loot::MasterLootCandidateList const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    LootEvent event;
    event.type = LootEventType::MASTER_LOOT_LIST;
    event.priority = LootEventPriority::MEDIUM;
    event.looterGuid = bot->GetGUID();
    event.itemGuid = packet.LootObj;
    event.itemEntry = 0;
    event.itemCount = packet.Players.size();
    event.lootType = LootType::CORPSE;
    event.timestamp = ::std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + ::std::chrono::seconds(10);

    EventBus<LootEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received MASTER_LOOT_LIST (typed): {} candidates",
        bot->GetName(), packet.Players.size());
}

void ParseTypedLootList(WorldSession* session, WorldPackets::Loot::LootList const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received LOOT_LIST (typed): owner={}, master={}",
        bot->GetName(), packet.Owner.ToString(),
        packet.Master.has_value() ? packet.Master.value().ToString() : "none");
}

// ================================================================================================
// HANDLER REGISTRATION
// ================================================================================================

void RegisterLootPacketHandlers()
{
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Loot::LootResponse>(&ParseTypedLootResponse);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Loot::LootReleaseResponse>(&ParseTypedLootReleaseResponse);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Loot::LootRemoved>(&ParseTypedLootRemoved);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Loot::LootMoneyNotify>(&ParseTypedLootMoneyNotify);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Loot::StartLootRoll>(&ParseTypedStartLootRoll);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Loot::LootRollBroadcast>(&ParseTypedLootRoll);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Loot::LootRollWon>(&ParseTypedLootRollWon);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Loot::LootAllPassed>(&ParseTypedLootAllPassed);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Loot::MasterLootCandidateList>(&ParseTypedMasterLootCandidateList);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Loot::LootList>(&ParseTypedLootList);

    TC_LOG_INFO("playerbot", "PlayerbotPacketSniffer: Registered {} Loot packet typed handlers", 10);
}

} // namespace Playerbot
