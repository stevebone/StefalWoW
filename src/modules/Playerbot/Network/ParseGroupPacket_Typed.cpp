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
#include "../Group/GroupEvents.h"
#include "PartyPackets.h"
#include "Log.h"

namespace Playerbot
{

// ================================================================================================
// TYPED PACKET HANDLERS
// These handlers receive TYPED packet objects before serialization (WoW 12.0 Solution)
// ================================================================================================

/**
 * Ready Check Started - Typed Handler
 * Receives full typed packet with all data accessible
 */
void ParseTypedReadyCheckStarted(WorldSession* session, WorldPackets::Party::ReadyCheckStarted const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    GroupEvent event;
    event.type = GroupEventType::READY_CHECK_STARTED;
    event.priority = EventPriority::HIGH;
    event.groupGuid = packet.PartyGUID;
    event.sourceGuid = packet.InitiatorGUID;
    event.data1 = static_cast<uint32>(Milliseconds(packet.Duration).count());  // Duration in milliseconds (WoW 12.0 API)
    event.data2 = static_cast<uint32>(packet.PartyIndex);
    event.timestamp = ::std::chrono::steady_clock::now();

    EventBus<GroupEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received READY_CHECK_STARTED (typed): initiator={}, duration={}ms, partyIndex={}",
        bot->GetName(), packet.InitiatorGUID.ToString(), Milliseconds(packet.Duration).count(), packet.PartyIndex);
}

/**
 * Ready Check Response - Typed Handler
 */
void ParseTypedReadyCheckResponse(WorldSession* session, WorldPackets::Party::ReadyCheckResponse const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    GroupEvent event;
    event.type = GroupEventType::READY_CHECK_RESPONSE;
    event.priority = EventPriority::MEDIUM;
    event.groupGuid = packet.PartyGUID;
    event.targetGuid = packet.Player;
    event.data1 = packet.IsReady ? 1 : 0;
    event.timestamp = ::std::chrono::steady_clock::now();

    EventBus<GroupEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received READY_CHECK_RESPONSE (typed): player={}, ready={}",
        bot->GetName(), packet.Player.ToString(), packet.IsReady ? "YES" : "NO");
}

/**
 * Ready Check Completed - Typed Handler
 */
void ParseTypedReadyCheckCompleted(WorldSession* session, WorldPackets::Party::ReadyCheckCompleted const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    GroupEvent event;
    event.type = GroupEventType::READY_CHECK_COMPLETED;
    event.priority = EventPriority::MEDIUM;
    event.groupGuid = packet.PartyGUID;
    event.data1 = 0;  // WoW 12.0: ReadyCount no longer in packet
    event.data2 = 0;  // WoW 12.0: NotReadyCount no longer in packet
    event.timestamp = ::std::chrono::steady_clock::now();

    EventBus<GroupEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received READY_CHECK_COMPLETED (typed): partyGuid={}",
        bot->GetName(), packet.PartyGUID.ToString());
}

/**
 * Raid Target Update Single - Typed Handler
 */
void ParseTypedRaidTargetUpdateSingle(WorldSession* session, WorldPackets::Party::SendRaidTargetUpdateSingle const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    GroupEvent event;
    event.type = GroupEventType::TARGET_ICON_CHANGED;
    event.priority = EventPriority::HIGH;
    event.targetGuid = packet.Target;
    event.sourceGuid = packet.ChangedBy;
    event.data1 = static_cast<uint32>(packet.Symbol);
    event.data2 = static_cast<uint32>(packet.PartyIndex);
    event.timestamp = ::std::chrono::steady_clock::now();

    EventBus<GroupEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received RAID_TARGET_UPDATE_SINGLE (typed): target={}, symbol={}, changedBy={}",
        bot->GetName(), packet.Target.ToString(), packet.Symbol, packet.ChangedBy.ToString());
}

/**
 * Raid Target Update All - Typed Handler
 */
void ParseTypedRaidTargetUpdateAll(WorldSession* session, WorldPackets::Party::SendRaidTargetUpdateAll const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    // Publish individual events for each target icon (WoW 12.0: vector of pairs)
    for (auto const& [symbolIndex, targetGuid] : packet.TargetIcons)
    {
        if (!targetGuid.IsEmpty())  // Check if target is valid
        {
            GroupEvent event;
            event.type = GroupEventType::TARGET_ICON_CHANGED;
            event.priority = EventPriority::MEDIUM;
            event.targetGuid = targetGuid;
            event.data1 = static_cast<uint32>(symbolIndex);  // Explicit symbol index from pair
            event.data2 = static_cast<uint32>(packet.PartyIndex);
            event.timestamp = ::std::chrono::steady_clock::now();

            EventBus<GroupEvent>::instance()->PublishEvent(event);
        }
    }

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received RAID_TARGET_UPDATE_ALL (typed): partyIndex={}",
        bot->GetName(), packet.PartyIndex);
}

/**
 * Group New Leader - Typed Handler
 */
void ParseTypedGroupNewLeader(WorldSession* session, WorldPackets::Party::GroupNewLeader const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    GroupEvent event;
    event.type = GroupEventType::LEADER_CHANGED;
    event.priority = EventPriority::HIGH;
    event.targetGuid = ObjectGuid::Empty;  // New leader GUID might be in packet data
    event.data1 = static_cast<uint32>(packet.PartyIndex);
    event.timestamp = ::std::chrono::steady_clock::now();

    EventBus<GroupEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received GROUP_NEW_LEADER (typed): partyIndex={}",
        bot->GetName(), packet.PartyIndex);
}

// ================================================================================================
// HANDLER REGISTRATION
// Called from PlayerbotPacketSniffer::Initialize()
// ================================================================================================

void RegisterGroupPacketHandlers()
{
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Party::ReadyCheckStarted>(&ParseTypedReadyCheckStarted);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Party::ReadyCheckResponse>(&ParseTypedReadyCheckResponse);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Party::ReadyCheckCompleted>(&ParseTypedReadyCheckCompleted);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Party::SendRaidTargetUpdateSingle>(&ParseTypedRaidTargetUpdateSingle);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Party::SendRaidTargetUpdateAll>(&ParseTypedRaidTargetUpdateAll);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Party::GroupNewLeader>(&ParseTypedGroupNewLeader);

    TC_LOG_INFO("playerbot", "PlayerbotPacketSniffer: Registered {} Group packet typed handlers", 6);
}

} // namespace Playerbot
