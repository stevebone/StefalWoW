/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ResourceEvents.h"
#include <sstream>

namespace Playerbot
{

ResourceEvent ResourceEvent::PowerChanged(ObjectGuid player, Powers type, int32 amt, int32 max)
{
    ResourceEvent event;
    event.type = ResourceEventType::POWER_UPDATE;
    event.priority = ResourceEventPriority::MEDIUM;
    event.playerGuid = player;
    event.powerType = type;
    event.amount = amt;
    event.maxAmount = max;
    event.isRegen = false;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(5000);
    return event;
}

ResourceEvent ResourceEvent::PowerRegen(ObjectGuid player, Powers type, int32 amt)
{
    ResourceEvent event;
    event.type = ResourceEventType::POWER_UPDATE;
    event.priority = ResourceEventPriority::LOW;
    event.playerGuid = player;
    event.powerType = type;
    event.amount = amt;
    event.maxAmount = 0;
    event.isRegen = true;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(2000);
    return event;
}

ResourceEvent ResourceEvent::PowerDrained(ObjectGuid player, Powers type, int32 amt)
{
    ResourceEvent event;
    event.type = ResourceEventType::POWER_UPDATE;
    event.priority = ResourceEventPriority::HIGH;
    event.playerGuid = player;
    event.powerType = type;
    event.amount = -amt;
    event.maxAmount = 0;
    event.isRegen = false;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(10000);
    return event;
}

bool ResourceEvent::IsValid() const
{
    if (type >= ResourceEventType::MAX_RESOURCE_EVENT)
        return false;
    if (timestamp.time_since_epoch().count() == 0)
        return false;
    if (playerGuid.IsEmpty())
        return false;
    return true;
}

std::string ResourceEvent::ToString() const
{
    std::ostringstream oss;
    oss << "[ResourceEvent] Type: " << static_cast<uint32>(type)
        << ", Player: " << playerGuid.ToString()
        << ", Power: " << static_cast<uint32>(powerType)
        << ", Amount: " << amount;
    return oss.str();
}

} // namespace Playerbot
