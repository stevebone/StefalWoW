/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "GroupEvents.h"
#include "Position.h"
#include <sstream>

namespace Playerbot
{

// Static helper functions for event type names
namespace
{
    const char* GetEventTypeName(GroupEventType type)
    {
        switch (type)
        {
            case GroupEventType::MEMBER_JOINED: return "MEMBER_JOINED";
            case GroupEventType::MEMBER_LEFT: return "MEMBER_LEFT";
            case GroupEventType::LEADER_CHANGED: return "LEADER_CHANGED";
            case GroupEventType::GROUP_DISBANDED: return "GROUP_DISBANDED";
            case GroupEventType::RAID_CONVERTED: return "RAID_CONVERTED";
            case GroupEventType::READY_CHECK_STARTED: return "READY_CHECK_STARTED";
            case GroupEventType::READY_CHECK_RESPONSE: return "READY_CHECK_RESPONSE";
            case GroupEventType::READY_CHECK_COMPLETED: return "READY_CHECK_COMPLETED";
            case GroupEventType::TARGET_ICON_CHANGED: return "TARGET_ICON_CHANGED";
            case GroupEventType::RAID_MARKER_CHANGED: return "RAID_MARKER_CHANGED";
            case GroupEventType::ASSIST_TARGET_CHANGED: return "ASSIST_TARGET_CHANGED";
            case GroupEventType::LOOT_METHOD_CHANGED: return "LOOT_METHOD_CHANGED";
            case GroupEventType::LOOT_THRESHOLD_CHANGED: return "LOOT_THRESHOLD_CHANGED";
            case GroupEventType::MASTER_LOOTER_CHANGED: return "MASTER_LOOTER_CHANGED";
            case GroupEventType::SUBGROUP_CHANGED: return "SUBGROUP_CHANGED";
            case GroupEventType::ASSISTANT_CHANGED: return "ASSISTANT_CHANGED";
            case GroupEventType::MAIN_TANK_CHANGED: return "MAIN_TANK_CHANGED";
            case GroupEventType::MAIN_ASSIST_CHANGED: return "MAIN_ASSIST_CHANGED";
            case GroupEventType::DIFFICULTY_CHANGED: return "DIFFICULTY_CHANGED";
            case GroupEventType::INSTANCE_LOCK_MESSAGE: return "INSTANCE_LOCK_MESSAGE";
            case GroupEventType::PING_RECEIVED: return "PING_RECEIVED";
            case GroupEventType::COMMAND_RESULT: return "COMMAND_RESULT";
            case GroupEventType::STATE_UPDATE_REQUIRED: return "STATE_UPDATE_REQUIRED";
            case GroupEventType::ERROR_OCCURRED: return "ERROR_OCCURRED";
            default: return "UNKNOWN";
        }
    }

    const char* GetPriorityName(EventPriority priority)
    {
        switch (priority)
        {
            case EventPriority::CRITICAL: return "CRITICAL";
            case EventPriority::HIGH: return "HIGH";
            case EventPriority::MEDIUM: return "MEDIUM";
            case EventPriority::LOW: return "LOW";
            case EventPriority::BATCH: return "BATCH";
            default: return "UNKNOWN";
        }
    }
}

GroupEvent GroupEvent::MemberJoined(ObjectGuid groupGuid, ObjectGuid memberGuid)
{
    GroupEvent event;
    event.type = GroupEventType::MEMBER_JOINED;
    event.priority = EventPriority::HIGH;
    event.groupGuid = groupGuid;
    event.sourceGuid = memberGuid;
    event.targetGuid = memberGuid;
    event.data1 = 0;
    event.data2 = 0;
    event.data3 = 0;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(30000); // 30 second TTL
    return event;
}

GroupEvent GroupEvent::MemberLeft(ObjectGuid groupGuid, ObjectGuid memberGuid, uint32 removeMethod)
{
    GroupEvent event;
    event.type = GroupEventType::MEMBER_LEFT;
    event.priority = EventPriority::HIGH;
    event.groupGuid = groupGuid;
    event.sourceGuid = memberGuid;
    event.targetGuid = memberGuid;
    event.data1 = removeMethod; // RemoveMethod enum
    event.data2 = 0;
    event.data3 = 0;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(30000);
    return event;
}

GroupEvent GroupEvent::LeaderChanged(ObjectGuid groupGuid, ObjectGuid newLeaderGuid)
{
    GroupEvent event;
    event.type = GroupEventType::LEADER_CHANGED;
    event.priority = EventPriority::HIGH;
    event.groupGuid = groupGuid;
    event.sourceGuid = newLeaderGuid;
    event.targetGuid = newLeaderGuid;
    event.data1 = 0;
    event.data2 = 0;
    event.data3 = 0;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(30000);
    return event;
}

GroupEvent GroupEvent::GroupDisbanded(ObjectGuid groupGuid)
{
    GroupEvent event;
    event.type = GroupEventType::GROUP_DISBANDED;
    event.priority = EventPriority::CRITICAL;
    event.groupGuid = groupGuid;
    event.sourceGuid = ObjectGuid::Empty;
    event.targetGuid = ObjectGuid::Empty;
    event.data1 = 0;
    event.data2 = 0;
    event.data3 = 0;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(10000); // 10 second TTL (critical)
    return event;
}

GroupEvent GroupEvent::ReadyCheckStarted(ObjectGuid groupGuid, ObjectGuid initiatorGuid, uint32 durationMs)
{
    GroupEvent event;
    event.type = GroupEventType::READY_CHECK_STARTED;
    event.priority = EventPriority::HIGH;
    event.groupGuid = groupGuid;
    event.sourceGuid = initiatorGuid;
    event.targetGuid = ObjectGuid::Empty;
    event.data1 = durationMs;
    event.data2 = 0;
    event.data3 = 0;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(durationMs + 5000); // Duration + 5s grace
    return event;
}

GroupEvent GroupEvent::TargetIconChanged(ObjectGuid groupGuid, uint8 icon, ObjectGuid targetGuid)
{
    GroupEvent event;
    event.type = GroupEventType::TARGET_ICON_CHANGED;
    event.priority = EventPriority::HIGH;
    event.groupGuid = groupGuid;
    event.sourceGuid = ObjectGuid::Empty;
    event.targetGuid = targetGuid;
    event.data1 = icon;
    event.data2 = 0;
    event.data3 = 0;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(20000);
    return event;
}

GroupEvent GroupEvent::RaidMarkerChanged(ObjectGuid groupGuid, uint32 markerId, Position const& position)
{
    GroupEvent event;
    event.type = GroupEventType::RAID_MARKER_CHANGED;
    event.priority = EventPriority::LOW;
    event.groupGuid = groupGuid;
    event.sourceGuid = ObjectGuid::Empty;
    event.targetGuid = ObjectGuid::Empty;
    event.data1 = markerId;
    // Pack position into data2 and data3 (not perfect but works for our needs)
    event.data2 = *reinterpret_cast<uint32 const*>(&position.m_positionX);
    event.data3 = (static_cast<uint64_t>(*reinterpret_cast<uint32 const*>(&position.m_positionY)) << 32) |
                  *reinterpret_cast<uint32 const*>(&position.m_positionZ);
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(60000); // 60 second TTL
    return event;
}

GroupEvent GroupEvent::LootMethodChanged(ObjectGuid groupGuid, uint8 lootMethod)
{
    GroupEvent event;
    event.type = GroupEventType::LOOT_METHOD_CHANGED;
    event.priority = EventPriority::MEDIUM;
    event.groupGuid = groupGuid;
    event.sourceGuid = ObjectGuid::Empty;
    event.targetGuid = ObjectGuid::Empty;
    event.data1 = lootMethod;
    event.data2 = 0;
    event.data3 = 0;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(30000);
    return event;
}

GroupEvent GroupEvent::DifficultyChanged(ObjectGuid groupGuid, int16 difficulty)
{
    GroupEvent event;
    event.type = GroupEventType::DIFFICULTY_CHANGED;
    event.priority = EventPriority::HIGH;
    event.groupGuid = groupGuid;
    event.sourceGuid = ObjectGuid::Empty;
    event.targetGuid = ObjectGuid::Empty;
    event.data1 = difficulty;
    event.data2 = 0;
    event.data3 = 0;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(20000);
    return event;
}

bool GroupEvent::IsValid() const
{
    // Group GUID must be valid (unless it's an error event)
    if (type != GroupEventType::ERROR_OCCURRED && groupGuid.IsEmpty())
        return false;

    // Event type must be within valid range
    if (type >= GroupEventType::MAX_EVENT_TYPE)
        return false;

    // Timestamp must be valid
    if (timestamp.time_since_epoch().count() == 0)
        return false;

    return true;
}

std::string GroupEvent::ToString() const
{
    std::ostringstream oss;
    oss << "[GroupEvent] "
        << "Type: " << GetEventTypeName(type)
        << ", Priority: " << GetPriorityName(priority)
        << ", Group: " << groupGuid.ToString()
        << ", Source: " << sourceGuid.ToString()
        << ", Target: " << targetGuid.ToString()
        << ", Data: " << data1 << "/" << data2 << "/" << data3;
    return oss.str();
}

} // namespace Playerbot
