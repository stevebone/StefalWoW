/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "AuraEvents.h"
#include <sstream>

namespace Playerbot
{

AuraEvent AuraEvent::AuraApplied(ObjectGuid target, ObjectGuid caster, uint32 spellId, uint8 stacks, bool harmful)
{
    AuraEvent event;
    event.type = AuraEventType::AURA_APPLIED;
    event.priority = harmful ? AuraEventPriority::HIGH : AuraEventPriority::MEDIUM;
    event.targetGuid = target;
    event.casterGuid = caster;
    event.spellId = spellId;
    event.auraSlot = 0;
    event.stackCount = stacks;
    event.duration = 0;
    event.isBuff = !harmful;
    event.isHarmful = harmful;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(30000);
    return event;
}

AuraEvent AuraEvent::AuraRemoved(ObjectGuid target, uint32 spellId)
{
    AuraEvent event;
    event.type = AuraEventType::AURA_REMOVED;
    event.priority = AuraEventPriority::MEDIUM;
    event.targetGuid = target;
    event.casterGuid = ObjectGuid::Empty;
    event.spellId = spellId;
    event.auraSlot = 0;
    event.stackCount = 0;
    event.duration = 0;
    event.isBuff = false;
    event.isHarmful = false;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(10000);
    return event;
}

AuraEvent AuraEvent::AuraUpdated(ObjectGuid target, uint32 spellId, uint8 stacks)
{
    AuraEvent event;
    event.type = AuraEventType::AURA_UPDATED;
    event.priority = AuraEventPriority::LOW;
    event.targetGuid = target;
    event.casterGuid = ObjectGuid::Empty;
    event.spellId = spellId;
    event.auraSlot = 0;
    event.stackCount = stacks;
    event.duration = 0;
    event.isBuff = false;
    event.isHarmful = false;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(10000);
    return event;
}

bool AuraEvent::IsValid() const
{
    if (type >= AuraEventType::MAX_AURA_EVENT)
        return false;
    if (timestamp.time_since_epoch().count() == 0)
        return false;
    if (targetGuid.IsEmpty())
        return false;
    return true;
}

std::string AuraEvent::ToString() const
{
    std::ostringstream oss;
    oss << "[AuraEvent] Type: " << static_cast<uint32>(type)
        << ", Target: " << targetGuid.ToString()
        << ", Spell: " << spellId
        << ", Stacks: " << static_cast<uint32>(stackCount)
        << ", Harmful: " << isHarmful;
    return oss.str();
}

} // namespace Playerbot
