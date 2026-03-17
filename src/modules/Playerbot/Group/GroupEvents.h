/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_GROUP_EVENTS_H
#define PLAYERBOT_GROUP_EVENTS_H

#include "Define.h"
#include "ObjectGuid.h"
#include <chrono>
#include <string>

// Forward declarations
struct Position;

namespace Playerbot
{

/**
 * @enum GroupEventType
 * @brief Categorizes all group-related events that bots must handle
 */
enum class GroupEventType : uint8
{
    // Core group lifecycle events
    MEMBER_JOINED = 0,          // New member added to group
    MEMBER_LEFT,                // Member removed from group (left/kicked)
    LEADER_CHANGED,             // Group leadership transferred
    GROUP_DISBANDED,            // Group completely disbanded
    RAID_CONVERTED,             // Party converted to raid (or vice versa)

    // Ready check system
    READY_CHECK_STARTED,        // Ready check initiated by leader
    READY_CHECK_RESPONSE,       // Member responded to ready check
    READY_CHECK_COMPLETED,      // Ready check finished (all responded or timeout)

    // Combat coordination
    TARGET_ICON_CHANGED,        // Raid target icon assigned/cleared
    RAID_MARKER_CHANGED,        // World raid marker placed/removed (legacy name)
    WORLD_MARKER_CHANGED,       // World raid marker placed/removed (new name for clarity)
    ASSIST_TARGET_CHANGED,      // Main assist target changed

    // Loot and distribution
    LOOT_METHOD_CHANGED,        // Group loot method modified
    LOOT_THRESHOLD_CHANGED,     // Item quality threshold changed
    MASTER_LOOTER_CHANGED,      // Master looter assigned

    // Raid organization
    SUBGROUP_CHANGED,           // Member moved to different subgroup
    ASSISTANT_CHANGED,          // Member promoted/demoted as assistant
    MAIN_TANK_CHANGED,          // Main tank assigned/cleared
    MAIN_ASSIST_CHANGED,        // Main assist assigned/cleared

    // Instance and difficulty
    DIFFICULTY_CHANGED,         // Instance difficulty modified
    INSTANCE_LOCK_MESSAGE,      // Instance lock/reset notification

    // Communication
    PING_RECEIVED,              // Ping notification (unit or location)
    COMMAND_RESULT,             // Group command execution result

    // Status updates
    GROUP_LIST_UPDATE,          // Group member list updated
    MEMBER_STATS_CHANGED,       // Member health/mana/stats changed
    INVITE_DECLINED,            // Group invite was declined

    // Internal events
    STATE_UPDATE_REQUIRED,      // Full state synchronization needed
    ERROR_OCCURRED,             // Error in group operation

    MAX_EVENT_TYPE
};

/**
 * @enum EventPriority
 * @brief Defines processing priority for group events
 */
enum class EventPriority : uint8
{
    CRITICAL = 0,   // Process immediately (disbanding, errors)
    HIGH = 1,       // Process within 100ms (combat events, ready checks)
    MEDIUM = 2,     // Process within 500ms (loot changes, role changes)
    LOW = 3,        // Process within 1000ms (cosmetic updates)
    BATCH = 4       // Batch process with others
};

/**
 * @struct GroupEvent
 * @brief Encapsulates all data for a group-related event
 */
struct GroupEvent
{
    using EventType = GroupEventType;
    using Priority = EventPriority;

    GroupEventType type;
    EventPriority priority;
    ObjectGuid groupGuid;           // Group involved in event
    ObjectGuid sourceGuid;          // Event originator (player/leader)
    ObjectGuid targetGuid;          // Event target (affected player/unit)

    uint32 data1;                   // Event-specific data 1
    uint32 data2;                   // Event-specific data 2
    uint64 data3;                   // Event-specific data 3 (64-bit for positions)

    std::chrono::steady_clock::time_point timestamp;
    std::chrono::steady_clock::time_point expiryTime;

    // Comparison operator for priority queue (higher priority = lower value)
    bool operator<(GroupEvent const& other) const
    {
        if (priority != other.priority)
            return priority > other.priority; // Lower enum value = higher priority

        return timestamp > other.timestamp; // Earlier events first within same priority
    }

    // Validation
    bool IsValid() const;
    bool IsExpired() const
    {
        return std::chrono::steady_clock::now() >= expiryTime;
    }
    std::string ToString() const;

    // Helper constructors for common event types
    static GroupEvent MemberJoined(ObjectGuid groupGuid, ObjectGuid memberGuid);
    static GroupEvent MemberLeft(ObjectGuid groupGuid, ObjectGuid memberGuid, uint32 removeMethod);
    static GroupEvent LeaderChanged(ObjectGuid groupGuid, ObjectGuid newLeaderGuid);
    static GroupEvent GroupDisbanded(ObjectGuid groupGuid);
    static GroupEvent ReadyCheckStarted(ObjectGuid groupGuid, ObjectGuid initiatorGuid, uint32 durationMs);
    static GroupEvent TargetIconChanged(ObjectGuid groupGuid, uint8 icon, ObjectGuid targetGuid);
    static GroupEvent RaidMarkerChanged(ObjectGuid groupGuid, uint32 markerId, Position const& position);
    static GroupEvent LootMethodChanged(ObjectGuid groupGuid, uint8 lootMethod);
    static GroupEvent DifficultyChanged(ObjectGuid groupGuid, int16 difficulty);  // WoW 12.0: Difficulty is int16
};

} // namespace Playerbot

#endif // PLAYERBOT_GROUP_EVENTS_H
