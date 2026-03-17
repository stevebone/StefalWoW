/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_COOLDOWN_EVENTS_H
#define PLAYERBOT_COOLDOWN_EVENTS_H

#include "Define.h"
#include "ObjectGuid.h"
#include <chrono>
#include <string>

namespace Playerbot
{

/**
 * @brief Cooldown event types
 */
enum class CooldownEventType : uint8
{
    SPELL_COOLDOWN_START = 0,
    SPELL_COOLDOWN_CLEAR,
    SPELL_COOLDOWN_MODIFY,
    SPELL_COOLDOWNS_CLEAR_ALL,
    ITEM_COOLDOWN_START,
    CATEGORY_COOLDOWN_START,
    // Major CD tracking for raid/group coordination
    MAJOR_CD_USED,          // A major raid/group cooldown was used
    MAJOR_CD_AVAILABLE,     // A major raid/group cooldown is available again
    MAX_COOLDOWN_EVENT
};

/**
 * @brief Major cooldown tiers for prioritization
 *
 * Used to categorize raid/group cooldowns by impact level.
 * Higher tiers should be saved for more dangerous situations.
 */
enum class MajorCooldownTier : uint8
{
    NONE = 0,               // Not a major cooldown
    RAID_OFFENSIVE,         // Bloodlust, Power Infusion, Innervate
    RAID_DEFENSIVE,         // Rallying Cry, Spirit Link, Darkness, AMZ
    EXTERNAL_MAJOR,         // Guardian Spirit, Pain Suppression, Ironbark, Life Cocoon
    EXTERNAL_MODERATE,      // Blessing of Sacrifice, Vigilance
    PERSONAL_MAJOR,         // Iceblock, Divine Shield, etc.
    RESURRECTION,           // Battle Res (Rebirth, Soulstone, etc.)
};

/**
 * @brief Cooldown event priorities
 */
enum class CooldownEventPriority : uint8
{
    CRITICAL = 0,
    HIGH = 1,
    MEDIUM = 2,
    LOW = 3,
    BATCH = 4
};

/**
 * @brief Cooldown event structure
 */
struct CooldownEvent
{
    using EventType = CooldownEventType;
    using Priority = CooldownEventPriority;

    CooldownEventType type;
    CooldownEventPriority priority;
    ObjectGuid casterGuid;
    uint32 spellId;
    uint32 itemId;
    uint32 category;
    uint32 cooldownMs;
    int32 modRateMs;
    MajorCooldownTier majorCDTier;  // For MAJOR_CD_USED/MAJOR_CD_AVAILABLE events
    std::chrono::steady_clock::time_point timestamp;
    std::chrono::steady_clock::time_point expiryTime;

    bool IsValid() const;
    bool IsExpired() const
    {
        return std::chrono::steady_clock::now() >= expiryTime;
    }
    std::string ToString() const;

    bool operator<(CooldownEvent const& other) const
    {
        return priority > other.priority;
    }

    // Helper constructors for basic cooldown events
    static CooldownEvent SpellCooldownStart(ObjectGuid caster, uint32 spellId, uint32 cooldownMs);
    static CooldownEvent SpellCooldownClear(ObjectGuid caster, uint32 spellId);
    static CooldownEvent ItemCooldownStart(ObjectGuid caster, uint32 itemId, uint32 cooldownMs);

    // Helper constructors for major cooldown coordination
    static CooldownEvent MajorCDUsed(ObjectGuid caster, uint32 spellId, MajorCooldownTier tier, uint32 cooldownMs);
    static CooldownEvent MajorCDAvailable(ObjectGuid caster, uint32 spellId, MajorCooldownTier tier);
};

} // namespace Playerbot

#endif // PLAYERBOT_COOLDOWN_EVENTS_H
