/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_RESOURCE_EVENTS_H
#define PLAYERBOT_RESOURCE_EVENTS_H

#include "Define.h"
#include "ObjectGuid.h"
#include <chrono>
#include <string>

namespace Playerbot
{

enum class ResourceEventType : uint8
{
    HEALTH_UPDATE = 0,
    POWER_UPDATE,
    BREAK_TARGET,
    MAX_RESOURCE_EVENT
};

enum class ResourceEventPriority : uint8
{
    CRITICAL = 0,
    HIGH = 1,
    MEDIUM = 2,
    LOW = 3,
    BATCH = 4
};

enum class Powers : uint8
{
    POWER_MANA = 0,
    POWER_RAGE = 1,
    POWER_FOCUS = 2,
    POWER_ENERGY = 3,
    POWER_RUNIC_POWER = 6
};

struct ResourceEvent
{
    using EventType = ResourceEventType;
    using Priority = ResourceEventPriority;

    ResourceEventType type;
    ResourceEventPriority priority;
    ObjectGuid playerGuid;
    Powers powerType;
    int32 amount;
    int32 maxAmount;
    bool isRegen;
    std::chrono::steady_clock::time_point timestamp;
    std::chrono::steady_clock::time_point expiryTime;

    bool IsValid() const;
    bool IsExpired() const
    {
        return std::chrono::steady_clock::now() >= expiryTime;
    }
    std::string ToString() const;

    bool operator<(ResourceEvent const& other) const
    {
        return priority > other.priority;
    }

    // Helper constructors
    static ResourceEvent PowerChanged(ObjectGuid player, Powers type, int32 amt, int32 max);
    static ResourceEvent PowerRegen(ObjectGuid player, Powers type, int32 amt);
    static ResourceEvent PowerDrained(ObjectGuid player, Powers type, int32 amt);
};

} // namespace Playerbot

#endif // PLAYERBOT_RESOURCE_EVENTS_H
