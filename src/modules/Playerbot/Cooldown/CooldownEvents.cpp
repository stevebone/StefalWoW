/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "CooldownEvents.h"
#include <sstream>

namespace Playerbot
{

CooldownEvent CooldownEvent::SpellCooldownStart(ObjectGuid caster, uint32 spellId, uint32 cooldownMs)
{
    CooldownEvent event;
    event.type = CooldownEventType::SPELL_COOLDOWN_START;
    event.priority = CooldownEventPriority::MEDIUM;
    event.casterGuid = caster;
    event.spellId = spellId;
    event.itemId = 0;
    event.category = 0;
    event.cooldownMs = cooldownMs;
    event.modRateMs = 0;
    event.majorCDTier = MajorCooldownTier::NONE;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(cooldownMs + 5000);
    return event;
}

CooldownEvent CooldownEvent::SpellCooldownClear(ObjectGuid caster, uint32 spellId)
{
    CooldownEvent event;
    event.type = CooldownEventType::SPELL_COOLDOWN_CLEAR;
    event.priority = CooldownEventPriority::HIGH;
    event.casterGuid = caster;
    event.spellId = spellId;
    event.itemId = 0;
    event.category = 0;
    event.cooldownMs = 0;
    event.modRateMs = 0;
    event.majorCDTier = MajorCooldownTier::NONE;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(5000);
    return event;
}

CooldownEvent CooldownEvent::ItemCooldownStart(ObjectGuid caster, uint32 itemId, uint32 cooldownMs)
{
    CooldownEvent event;
    event.type = CooldownEventType::ITEM_COOLDOWN_START;
    event.priority = CooldownEventPriority::MEDIUM;
    event.casterGuid = caster;
    event.spellId = 0;
    event.itemId = itemId;
    event.category = 0;
    event.cooldownMs = cooldownMs;
    event.modRateMs = 0;
    event.majorCDTier = MajorCooldownTier::NONE;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(cooldownMs + 5000);
    return event;
}

CooldownEvent CooldownEvent::MajorCDUsed(ObjectGuid caster, uint32 spellId, MajorCooldownTier tier, uint32 cooldownMs)
{
    CooldownEvent event;
    event.type = CooldownEventType::MAJOR_CD_USED;
    event.priority = CooldownEventPriority::CRITICAL;  // Major CDs are high priority
    event.casterGuid = caster;
    event.spellId = spellId;
    event.itemId = 0;
    event.category = 0;
    event.cooldownMs = cooldownMs;
    event.modRateMs = 0;
    event.majorCDTier = tier;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(5000);  // Short expiry for coordination
    return event;
}

CooldownEvent CooldownEvent::MajorCDAvailable(ObjectGuid caster, uint32 spellId, MajorCooldownTier tier)
{
    CooldownEvent event;
    event.type = CooldownEventType::MAJOR_CD_AVAILABLE;
    event.priority = CooldownEventPriority::HIGH;
    event.casterGuid = caster;
    event.spellId = spellId;
    event.itemId = 0;
    event.category = 0;
    event.cooldownMs = 0;
    event.modRateMs = 0;
    event.majorCDTier = tier;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(5000);
    return event;
}

bool CooldownEvent::IsValid() const
{
    if (type >= CooldownEventType::MAX_COOLDOWN_EVENT)
        return false;
    if (timestamp.time_since_epoch().count() == 0)
        return false;
    if (casterGuid.IsEmpty())
        return false;
    return true;
}

std::string CooldownEvent::ToString() const
{
    std::ostringstream oss;
    oss << "[CooldownEvent] Type: " << static_cast<uint32>(type)
        << ", Caster: " << casterGuid.ToString()
        << ", Spell: " << spellId
        << ", Item: " << itemId
        << ", Duration: " << cooldownMs << "ms";
    if (majorCDTier != MajorCooldownTier::NONE)
        oss << ", Tier: " << static_cast<uint32>(majorCDTier);
    return oss.str();
}

} // namespace Playerbot
