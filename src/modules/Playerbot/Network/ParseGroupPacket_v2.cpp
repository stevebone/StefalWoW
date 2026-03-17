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
#include "WorldPacket.h"
#include "Player.h"
#include "GroupEventBus.h"
#include "Log.h"

namespace Playerbot
{

/**
 * TEMPORARY IMPLEMENTATION: Opcode-Only Detection
 *
 * This version detects WHICH packets are sent but doesn't parse packet contents.
 * This is because WoW 12.0 ServerPacket classes only have Write() methods, not Read() methods.
 *
 * Full packet parsing requires hooking BEFORE packet serialization to access typed packet objects.
 * See PLAYERBOT_PACKET_SNIFFER_STATUS.md for details.
 */

void PlayerbotPacketSniffer::ParseGroupPacket(WorldSession* session, WorldPacket const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    OpcodeServer opcode = static_cast<OpcodeServer>(packet.GetOpcode());

    // Opcode-only detection - just log that we received the packet
    switch (opcode)
    {
        case SMSG_READY_CHECK_STARTED:
        {
            GroupEvent event;
            event.type = GroupEventType::READY_CHECK_STARTED;
            event.priority = EventPriority::HIGH;
            event.timestamp = ::std::chrono::steady_clock::now();
            EventBus<GroupEvent>::instance()->PublishEvent(event);

            TC_LOG_DEBUG("playerbot.packets", "Bot {} - READY_CHECK_STARTED detected", bot->GetName());
            break;
        }

        case SMSG_READY_CHECK_RESPONSE:
        {
            GroupEvent event;
            event.type = GroupEventType::READY_CHECK_RESPONSE;
            event.priority = EventPriority::NORMAL;
            event.timestamp = ::std::chrono::steady_clock::now();
            EventBus<GroupEvent>::instance()->PublishEvent(event);

            TC_LOG_DEBUG("playerbot.packets", "Bot {} - READY_CHECK_RESPONSE detected", bot->GetName());
            break;
        }

        case SMSG_READY_CHECK_COMPLETED:
        {
            GroupEvent event;
            event.type = GroupEventType::READY_CHECK_COMPLETED;
            event.priority = EventPriority::NORMAL;
            event.timestamp = ::std::chrono::steady_clock::now();
            EventBus<GroupEvent>::instance()->PublishEvent(event);

            TC_LOG_DEBUG("playerbot.packets", "Bot {} - READY_CHECK_COMPLETED detected", bot->GetName());
            break;
        }

        case SMSG_RAID_MARKERS_CHANGED:
        {
            GroupEvent event;
            event.type = GroupEventType::WORLD_MARKER_CHANGED;
            event.priority = EventPriority::LOW;
            event.timestamp = ::std::chrono::steady_clock::now();
            EventBus<GroupEvent>::instance()->PublishEvent(event);

            TC_LOG_DEBUG("playerbot.packets", "Bot {} - RAID_MARKERS_CHANGED detected", bot->GetName());
            break;
        }

        case SMSG_GROUP_NEW_LEADER:
        {
            GroupEvent event;
            event.type = GroupEventType::LEADER_CHANGED;
            event.priority = GroupEventType::HIGH;
            event.timestamp = ::std::chrono::steady_clock::now();
            EventBus<GroupEvent>::instance()->PublishEvent(event);

            TC_LOG_DEBUG("playerbot.packets", "Bot {} - GROUP_NEW_LEADER detected", bot->GetName());
            break;
        }

        case SMSG_PARTY_UPDATE:
        {
            GroupEvent event;
            event.type = GroupEventType::MEMBER_JOINED;
            event.priority = EventPriority::NORMAL;
            event.timestamp = ::std::chrono::steady_clock::now();
            EventBus<GroupEvent>::instance()->PublishEvent(event);

            TC_LOG_DEBUG("playerbot.packets", "Bot {} - PARTY_UPDATE detected", bot->GetName());
            break;
        }

        case SMSG_PARTY_MEMBER_FULL_STATE:
        case SMSG_PARTY_MEMBER_PARTIAL_STATE:
        {
            GroupEvent event;
            event.type = GroupEventType::MEMBER_STATS_CHANGED;
            event.priority = EventPriority::LOW;
            event.timestamp = ::std::chrono::steady_clock::now();
            EventBus<GroupEvent>::instance()->PublishEvent(event);

            TC_LOG_DEBUG("playerbot.packets", "Bot {} - PARTY_MEMBER_STATE detected", bot->GetName());
            break;
        }

        case SMSG_GROUP_DESTROYED:
        {
            GroupEvent event;
            event.type = GroupEventType::GROUP_DISBANDED;
            event.priority = EventPriority::HIGH;
            event.timestamp = ::std::chrono::steady_clock::now();
            EventBus<GroupEvent>::instance()->PublishEvent(event);

            TC_LOG_DEBUG("playerbot.packets", "Bot {} - GROUP_DESTROYED detected", bot->GetName());
            break;
        }

        case SMSG_GROUP_DECLINE:
        {
            GroupEvent event;
            event.type = GroupEventType::INVITE_DECLINED;
            event.priority = EventPriority::NORMAL;
            event.timestamp = ::std::chrono::steady_clock::now();
            EventBus<GroupEvent>::instance()->PublishEvent(event);

            TC_LOG_DEBUG("playerbot.packets", "Bot {} - GROUP_DECLINE detected", bot->GetName());
            break;
        }

        default:
            TC_LOG_DEBUG("playerbot.packets", "Bot {} - Unhandled group packet: {}",
                bot->GetName(), static_cast<uint32>(opcode));
            break;
    }
}

} // namespace Playerbot
