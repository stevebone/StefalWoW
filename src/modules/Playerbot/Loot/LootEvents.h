/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_LOOT_EVENTS_H
#define PLAYERBOT_LOOT_EVENTS_H

#include "Define.h"
#include "ObjectGuid.h"
#include <chrono>
#include <string>

namespace Playerbot
{

/**
 * @brief Loot event types
 */
enum class LootEventType : uint8
{
    LOOT_WINDOW_OPENED = 0,
    LOOT_WINDOW_CLOSED,
    LOOT_ITEM_RECEIVED,
    LOOT_MONEY_RECEIVED,
    LOOT_REMOVED,
    LOOT_SLOT_CHANGED,
    LOOT_ROLL_STARTED,
    LOOT_ROLL_CAST,
    LOOT_ROLL_WON,
    LOOT_ALL_PASSED,
    MASTER_LOOT_LIST,
    MAX_LOOT_EVENT
};

/**
 * @brief Loot event priorities
 */
enum class LootEventPriority : uint8
{
    CRITICAL = 0,
    HIGH = 1,
    MEDIUM = 2,
    LOW = 3,
    BATCH = 4
};

/**
 * @brief Loot types
 */
enum class LootType : uint8
{
    CORPSE = 0,
    PICKPOCKETING,
    FISHING,
    DISENCHANTING,
    SKINNING,
    PROSPECTING,
    MILLING,
    ITEM,
    MAIL,
    INSIGNIA
};

/**
 * @brief Loot event structure
 *
 * Represents a single loot-related event that can be published and processed
 * by the LootEventBus system.
 */
struct LootEvent
{
    using EventType = LootEventType;
    using Priority = LootEventPriority;

    LootEventType type;
    LootEventPriority priority;
    ObjectGuid looterGuid;
    ObjectGuid itemGuid;
    uint32 itemEntry;
    uint32 itemCount;
    LootType lootType;
    std::chrono::steady_clock::time_point timestamp;
    std::chrono::steady_clock::time_point expiryTime;

    /**
     * @brief Check if event is valid
     * @return true if event has valid data
     */
    bool IsValid() const
    {
        if (type >= LootEventType::MAX_LOOT_EVENT)
            return false;
        if (timestamp.time_since_epoch().count() == 0)
            return false;
        if (looterGuid.IsEmpty() && type != LootEventType::LOOT_ROLL_STARTED)
            return false;
        return true;
    }

    /**
     * @brief Check if event has expired
     * @return true if current time >= expiry time
     */
    bool IsExpired() const
    {
        return std::chrono::steady_clock::now() >= expiryTime;
    }

    /**
     * @brief Convert event to string for logging
     * @return String representation of event
     */
    std::string ToString() const;

    /**
     * @brief Priority comparison for priority queue
     * @note Lower priority value = higher priority (CRITICAL > HIGH > MEDIUM > LOW)
     */
    bool operator<(LootEvent const& other) const
    {
        return priority > other.priority;
    }

    // ========================================================================
    // HELPER CONSTRUCTORS
    // ========================================================================

    /**
     * @brief Create an item looted event
     * @param looter Player who looted the item
     * @param item Item GUID
     * @param entry Item entry ID
     * @param count Item count
     * @param type Loot type
     * @return Constructed LootEvent
     */
    static LootEvent ItemLooted(ObjectGuid looter, ObjectGuid item, uint32 entry, uint32 count, LootType type);

    /**
     * @brief Create a loot roll started event
     * @param item Item GUID
     * @param entry Item entry ID
     * @return Constructed LootEvent
     */
    static LootEvent LootRollStarted(ObjectGuid item, uint32 entry);

    /**
     * @brief Create a loot roll won event
     * @param winner Player who won the roll
     * @param item Item GUID
     * @param entry Item entry ID
     * @return Constructed LootEvent
     */
    static LootEvent LootRollWon(ObjectGuid winner, ObjectGuid item, uint32 entry);
};

} // namespace Playerbot

#endif // PLAYERBOT_LOOT_EVENTS_H
