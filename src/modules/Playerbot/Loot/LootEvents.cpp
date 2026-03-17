/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "LootEvents.h"
#include <sstream>

namespace Playerbot
{

LootEvent LootEvent::ItemLooted(ObjectGuid looter, ObjectGuid item, uint32 entry, uint32 count, LootType type)
{
    LootEvent event;
    event.type = LootEventType::LOOT_ITEM_RECEIVED;
    event.priority = LootEventPriority::MEDIUM;
    event.looterGuid = looter;
    event.itemGuid = item;
    event.itemEntry = entry;
    event.itemCount = count;
    event.lootType = type;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(30000);
    return event;
}

LootEvent LootEvent::LootRollStarted(ObjectGuid item, uint32 entry)
{
    LootEvent event;
    event.type = LootEventType::LOOT_ROLL_STARTED;
    event.priority = LootEventPriority::HIGH;
    event.looterGuid = ObjectGuid::Empty;
    event.itemGuid = item;
    event.itemEntry = entry;
    event.itemCount = 1;
    event.lootType = LootType::CORPSE;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(60000);
    return event;
}

LootEvent LootEvent::LootRollWon(ObjectGuid winner, ObjectGuid item, uint32 entry)
{
    LootEvent event;
    event.type = LootEventType::LOOT_ROLL_WON;
    event.priority = LootEventPriority::HIGH;
    event.looterGuid = winner;
    event.itemGuid = item;
    event.itemEntry = entry;
    event.itemCount = 1;
    event.lootType = LootType::CORPSE;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(10000);
    return event;
}

std::string LootEvent::ToString() const
{
    std::ostringstream oss;
    oss << "[LootEvent] Type: " << static_cast<uint32>(type)
        << ", Looter: " << looterGuid.ToString()
        << ", Item: " << itemEntry
        << " x" << itemCount;
    return oss.str();
}

} // namespace Playerbot
