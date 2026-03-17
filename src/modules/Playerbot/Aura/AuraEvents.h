/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_AURA_EVENTS_H
#define PLAYERBOT_AURA_EVENTS_H

#include "Define.h"
#include "ObjectGuid.h"
#include <chrono>
#include <string>

namespace Playerbot
{

enum class AuraEventType : uint8
{
    AURA_APPLIED = 0,
    AURA_REMOVED,
    AURA_UPDATED,
    DISPEL_FAILED,
    SPELL_MODIFIER_CHANGED,
    MAX_AURA_EVENT
};

enum class AuraEventPriority : uint8
{
    CRITICAL = 0,
    HIGH = 1,
    MEDIUM = 2,
    LOW = 3,
    BATCH = 4
};

struct AuraEvent
{
    using EventType = AuraEventType;
    using Priority = AuraEventPriority;

    AuraEventType type;
    AuraEventPriority priority;
    ObjectGuid targetGuid;
    ObjectGuid casterGuid;
    uint32 spellId;
    uint32 auraSlot;
    uint8 stackCount;
    uint32 duration;
    bool isBuff;
    bool isHarmful;
    std::chrono::steady_clock::time_point timestamp;
    std::chrono::steady_clock::time_point expiryTime;

    bool IsValid() const;
    bool IsExpired() const
    {
        return std::chrono::steady_clock::now() >= expiryTime;
    }
    std::string ToString() const;

    bool operator<(AuraEvent const& other) const
    {
        return priority > other.priority;
    }

    // Helper constructors
    static AuraEvent AuraApplied(ObjectGuid target, ObjectGuid caster, uint32 spellId, uint8 stacks, bool harmful);
    static AuraEvent AuraRemoved(ObjectGuid target, uint32 spellId);
    static AuraEvent AuraUpdated(ObjectGuid target, uint32 spellId, uint8 stacks);
};

} // namespace Playerbot

#endif // PLAYERBOT_AURA_EVENTS_H
