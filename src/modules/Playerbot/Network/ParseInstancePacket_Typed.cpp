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
#include "Instance/InstanceEvents.h"
#include "InstancePackets.h"
#include "Player.h"
#include "WorldSession.h"
#include "Log.h"

namespace Playerbot
{

/**
 * @brief SMSG_INSTANCE_RESET - Instance has been reset
 */
void ParseTypedInstanceReset(WorldSession* session, WorldPackets::Instance::InstanceReset const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    InstanceEvent event = InstanceEvent::InstanceReset(
        bot->GetGUID(),
        packet.MapID
    );

    EventBus<InstanceEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received INSTANCE_RESET (typed): map={}",
        bot->GetName(), packet.MapID);
}

/**
 * @brief SMSG_INSTANCE_RESET_FAILED - Instance reset failed
 */
void ParseTypedInstanceResetFailed(WorldSession* session, WorldPackets::Instance::InstanceResetFailed const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    InstanceEvent event = InstanceEvent::InstanceResetFailed(
        bot->GetGUID(),
        packet.MapID,
        packet.ResetFailedReason
    );

    EventBus<InstanceEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received INSTANCE_RESET_FAILED (typed): map={}, reason={}",
        bot->GetName(), packet.MapID, packet.ResetFailedReason);
}

/**
 * @brief SMSG_INSTANCE_ENCOUNTER_ENGAGE_UNIT - Boss encounter frame update
 */
void ParseTypedInstanceEncounterEngageUnit(WorldSession* session, WorldPackets::Instance::InstanceEncounterEngageUnit const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    InstanceEvent event = InstanceEvent::EncounterFrameUpdate(
        bot->GetGUID(),
        0,  // Encounter ID not in packet
        packet.TargetFramePriority
    );

    EventBus<InstanceEvent>::instance()->PublishEvent(event);

    TC_LOG_TRACE("playerbot.packets", "Bot {} received INSTANCE_ENCOUNTER_ENGAGE_UNIT (typed): unit={}, priority={}",
        bot->GetName(), packet.Unit.ToString(), packet.TargetFramePriority);
}

/**
 * @brief SMSG_INSTANCE_INFO - Raid instance info received
 */
void ParseTypedInstanceInfo(WorldSession* session, WorldPackets::Instance::InstanceInfo const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    // Process each instance lock
    for (auto const& lock : packet.LockList)
    {
        // Extract boss states from completed mask
        ::std::vector<uint32> bossStates;
        uint32 mask = lock.CompletedMask;
        for (uint32 i = 0; i < 32 && mask; ++i)
        {
            if (mask & (1 << i))
                bossStates.push_back(i);
            mask &= ~(1 << i);
        }

        InstanceEvent event = InstanceEvent::RaidInfoReceived(
            bot->GetGUID(),
            lock.MapID,
            static_cast<uint32>(lock.InstanceID),
            ::std::move(bossStates)
        );

        EventBus<InstanceEvent>::instance()->PublishEvent(event);
    }

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received INSTANCE_INFO (typed): {} locks",
        bot->GetName(), packet.LockList.size());
}

/**
 * @brief SMSG_RAID_GROUP_ONLY - Raid group only warning
 */
void ParseTypedRaidGroupOnly(WorldSession* session, WorldPackets::Instance::RaidGroupOnly const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    InstanceEvent event = InstanceEvent::RaidGroupOnlyWarning(
        bot->GetGUID()
    );

    EventBus<InstanceEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received RAID_GROUP_ONLY (typed): delay={}, reason={}",
        bot->GetName(), packet.Delay, packet.Reason);
}

/**
 * @brief SMSG_INSTANCE_SAVE_CREATED - Instance save created
 */
void ParseTypedInstanceSaveCreated(WorldSession* session, WorldPackets::Instance::InstanceSaveCreated const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    // Note: This packet doesn't include map/instance IDs, use current map
    uint32 mapId = bot->GetMapId();
    InstanceEvent event = InstanceEvent::InstanceSaveCreated(
        bot->GetGUID(),
        mapId,
        0  // Instance ID not in packet
    );

    EventBus<InstanceEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received INSTANCE_SAVE_CREATED (typed): gm={}",
        bot->GetName(), packet.Gm);
}

/**
 * @brief SMSG_RAID_INSTANCE_MESSAGE - Raid instance message received
 */
void ParseTypedRaidInstanceMessage(WorldSession* session, WorldPackets::Instance::RaidInstanceMessage const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    InstanceEvent event = InstanceEvent::InstanceMessageReceived(
        bot->GetGUID(),
        packet.MapID,
        ::std::string(packet.WarningMessage)
    );

    EventBus<InstanceEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received RAID_INSTANCE_MESSAGE (typed): map={}, type={}, msg={}",
        bot->GetName(), packet.MapID, packet.Type, packet.WarningMessage);
}

/**
 * @brief Register all instance packet typed handlers
 */
void RegisterInstancePacketHandlers()
{
    // Register instance reset handlers
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Instance::InstanceReset>(&ParseTypedInstanceReset);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Instance::InstanceResetFailed>(&ParseTypedInstanceResetFailed);

    // Register encounter frame handler
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Instance::InstanceEncounterEngageUnit>(&ParseTypedInstanceEncounterEngageUnit);

    // Register instance info handler
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Instance::InstanceInfo>(&ParseTypedInstanceInfo);

    // Register raid group handler
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Instance::RaidGroupOnly>(&ParseTypedRaidGroupOnly);

    // Register instance save handler
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Instance::InstanceSaveCreated>(&ParseTypedInstanceSaveCreated);

    // Register instance message handler
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Instance::RaidInstanceMessage>(&ParseTypedRaidInstanceMessage);

    TC_LOG_INFO("playerbot", "PlayerbotPacketSniffer: Registered {} Instance packet typed handlers", 7);
}

} // namespace Playerbot
