/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "InstanceEvents.h"
#include <sstream>

namespace Playerbot
{

InstanceEvent InstanceEvent::InstanceReset(ObjectGuid playerGuid, uint32 mapId)
{
    InstanceEvent event;
    event.type = InstanceEventType::INSTANCE_RESET;
    event.playerGuid = playerGuid;
    event.mapId = mapId;
    event.instanceId = 0;
    event.encounterId = 0;
    event.encounterFrame = 0;
    event.errorCode = 0;
    event.timestamp = std::chrono::steady_clock::now();
    return event;
}

InstanceEvent InstanceEvent::InstanceResetFailed(ObjectGuid playerGuid, uint32 mapId, uint32 errorCode)
{
    InstanceEvent event;
    event.type = InstanceEventType::INSTANCE_RESET_FAILED;
    event.playerGuid = playerGuid;
    event.mapId = mapId;
    event.errorCode = errorCode;
    event.instanceId = 0;
    event.encounterId = 0;
    event.encounterFrame = 0;
    event.timestamp = std::chrono::steady_clock::now();
    return event;
}

InstanceEvent InstanceEvent::EncounterFrameUpdate(ObjectGuid playerGuid, uint32 encounterId, uint32 frame)
{
    InstanceEvent event;
    event.type = InstanceEventType::ENCOUNTER_FRAME_UPDATE;
    event.playerGuid = playerGuid;
    event.encounterId = encounterId;
    event.encounterFrame = frame;
    event.mapId = 0;
    event.instanceId = 0;
    event.errorCode = 0;
    event.timestamp = std::chrono::steady_clock::now();
    return event;
}

InstanceEvent InstanceEvent::RaidInfoReceived(ObjectGuid playerGuid, uint32 mapId, uint32 instanceId, std::vector<uint32> bossStates)
{
    InstanceEvent event;
    event.type = InstanceEventType::RAID_INFO_RECEIVED;
    event.playerGuid = playerGuid;
    event.mapId = mapId;
    event.instanceId = instanceId;
    event.bossStates = std::move(bossStates);
    event.encounterId = 0;
    event.encounterFrame = 0;
    event.errorCode = 0;
    event.timestamp = std::chrono::steady_clock::now();
    return event;
}

InstanceEvent InstanceEvent::RaidGroupOnlyWarning(ObjectGuid playerGuid)
{
    InstanceEvent event;
    event.type = InstanceEventType::RAID_GROUP_ONLY_WARNING;
    event.playerGuid = playerGuid;
    event.mapId = 0;
    event.instanceId = 0;
    event.encounterId = 0;
    event.encounterFrame = 0;
    event.errorCode = 0;
    event.timestamp = std::chrono::steady_clock::now();
    return event;
}

InstanceEvent InstanceEvent::InstanceSaveCreated(ObjectGuid playerGuid, uint32 mapId, uint32 instanceId)
{
    InstanceEvent event;
    event.type = InstanceEventType::INSTANCE_SAVE_CREATED;
    event.playerGuid = playerGuid;
    event.mapId = mapId;
    event.instanceId = instanceId;
    event.encounterId = 0;
    event.encounterFrame = 0;
    event.errorCode = 0;
    event.timestamp = std::chrono::steady_clock::now();
    return event;
}

InstanceEvent InstanceEvent::InstanceMessageReceived(ObjectGuid playerGuid, uint32 mapId, std::string message)
{
    InstanceEvent event;
    event.type = InstanceEventType::INSTANCE_MESSAGE_RECEIVED;
    event.playerGuid = playerGuid;
    event.mapId = mapId;
    event.message = std::move(message);
    event.instanceId = 0;
    event.encounterId = 0;
    event.encounterFrame = 0;
    event.errorCode = 0;
    event.timestamp = std::chrono::steady_clock::now();
    return event;
}

bool InstanceEvent::IsValid() const
{
    if (type >= InstanceEventType::MAX_INSTANCE_EVENT)
        return false;

    switch (type)
    {
        case InstanceEventType::INSTANCE_RESET:
            return !playerGuid.IsEmpty() && mapId > 0;
        case InstanceEventType::INSTANCE_RESET_FAILED:
            return !playerGuid.IsEmpty() && mapId > 0;
        case InstanceEventType::ENCOUNTER_FRAME_UPDATE:
            return !playerGuid.IsEmpty();
        case InstanceEventType::RAID_INFO_RECEIVED:
            return !playerGuid.IsEmpty() && mapId > 0;
        case InstanceEventType::RAID_GROUP_ONLY_WARNING:
            return !playerGuid.IsEmpty();
        case InstanceEventType::INSTANCE_SAVE_CREATED:
            return !playerGuid.IsEmpty() && mapId > 0;
        case InstanceEventType::INSTANCE_MESSAGE_RECEIVED:
            return !playerGuid.IsEmpty();
        default:
            return false;
    }
}

std::string InstanceEvent::ToString() const
{
    std::ostringstream oss;
    oss << "InstanceEvent[";

    switch (type)
    {
        case InstanceEventType::INSTANCE_RESET:
            oss << "INSTANCE_RESET, map=" << mapId;
            break;
        case InstanceEventType::INSTANCE_RESET_FAILED:
            oss << "INSTANCE_RESET_FAILED, map=" << mapId << ", error=" << errorCode;
            break;
        case InstanceEventType::ENCOUNTER_FRAME_UPDATE:
            oss << "ENCOUNTER_FRAME, encounter=" << encounterId << ", frame=" << encounterFrame;
            break;
        case InstanceEventType::RAID_INFO_RECEIVED:
            oss << "RAID_INFO, map=" << mapId << ", instance=" << instanceId
                << ", bosses=" << bossStates.size();
            break;
        case InstanceEventType::RAID_GROUP_ONLY_WARNING:
            oss << "RAID_GROUP_ONLY";
            break;
        case InstanceEventType::INSTANCE_SAVE_CREATED:
            oss << "INSTANCE_SAVE, map=" << mapId << ", instance=" << instanceId;
            break;
        case InstanceEventType::INSTANCE_MESSAGE_RECEIVED:
            oss << "INSTANCE_MESSAGE, map=" << mapId << ", msg=" << message.substr(0, 50);
            break;
        default:
            oss << "UNKNOWN";
    }

    oss << "]";
    return oss.str();
}

} // namespace Playerbot
