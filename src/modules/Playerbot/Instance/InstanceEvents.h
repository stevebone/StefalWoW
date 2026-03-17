/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_INSTANCE_EVENTS_H
#define PLAYERBOT_INSTANCE_EVENTS_H

#include "Define.h"
#include "ObjectGuid.h"
#include <chrono>
#include <string>
#include <vector>

namespace Playerbot
{

enum class InstanceEventType : uint8
{
    INSTANCE_RESET = 0,
    INSTANCE_RESET_FAILED,
    ENCOUNTER_FRAME_UPDATE,
    RAID_INFO_RECEIVED,
    RAID_GROUP_ONLY_WARNING,
    INSTANCE_SAVE_CREATED,
    INSTANCE_MESSAGE_RECEIVED,
    MAX_INSTANCE_EVENT
};

enum class InstanceEventPriority : uint8
{
    CRITICAL = 0,
    HIGH = 1,
    MEDIUM = 2,
    LOW = 3,
    BATCH = 4
};

struct InstanceEvent
{
    using EventType = InstanceEventType;
    using Priority = InstanceEventPriority;

    InstanceEventType type;
    ObjectGuid playerGuid;
    uint32 mapId;
    uint32 instanceId;
    uint32 encounterId;
    uint32 encounterFrame;
    uint32 errorCode;
    std::string message;
    std::vector<uint32> bossStates;  // For raid info
    std::chrono::steady_clock::time_point timestamp;
    std::chrono::steady_clock::time_point expiryTime;
    InstanceEventPriority priority{InstanceEventPriority::MEDIUM};

    bool IsValid() const;
    bool IsExpired() const
    {
        return std::chrono::steady_clock::now() >= expiryTime;
    }
    std::string ToString() const;

    // Operator< for priority queue (higher priority = lower value)
    bool operator<(const InstanceEvent& other) const
    {
        if (priority != other.priority)
            return static_cast<uint8>(priority) > static_cast<uint8>(other.priority);
        return timestamp > other.timestamp;  // Earlier events first
    }

    // Factory methods
    static InstanceEvent InstanceReset(ObjectGuid playerGuid, uint32 mapId);
    static InstanceEvent InstanceResetFailed(ObjectGuid playerGuid, uint32 mapId, uint32 errorCode);
    static InstanceEvent EncounterFrameUpdate(ObjectGuid playerGuid, uint32 encounterId, uint32 frame);
    static InstanceEvent RaidInfoReceived(ObjectGuid playerGuid, uint32 mapId, uint32 instanceId, std::vector<uint32> bossStates);
    static InstanceEvent RaidGroupOnlyWarning(ObjectGuid playerGuid);
    static InstanceEvent InstanceSaveCreated(ObjectGuid playerGuid, uint32 mapId, uint32 instanceId);
    static InstanceEvent InstanceMessageReceived(ObjectGuid playerGuid, uint32 mapId, std::string message);
};

} // namespace Playerbot

#endif // PLAYERBOT_INSTANCE_EVENTS_H
