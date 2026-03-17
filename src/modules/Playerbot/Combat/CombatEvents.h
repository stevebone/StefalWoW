/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_COMBAT_EVENTS_H
#define PLAYERBOT_COMBAT_EVENTS_H

#include "Define.h"
#include "ObjectGuid.h"
#include <chrono>
#include <string>

namespace Playerbot
{

/**
 * @enum CombatEventType
 * @brief Categorizes all combat-related events that bots must handle
 */
enum class CombatEventType : uint8
{
    // Spell casting
    SPELL_CAST_START = 0,       // Spell cast begins (for interrupts)
    SPELL_CAST_GO,              // Spell cast completes (for positioning)
    SPELL_CAST_FAILED,          // Spell cast failed
    SPELL_CAST_DELAYED,         // Spell cast delayed (pushback)

    // Spell damage and healing
    SPELL_DAMAGE_DEALT,         // Spell damage dealt
    SPELL_DAMAGE_TAKEN,         // Spell damage received
    SPELL_HEAL_DEALT,           // Healing done
    SPELL_HEAL_TAKEN,           // Healing received
    SPELL_ENERGIZE,             // Resource gain (mana, rage, etc.)

    // Periodic effects
    PERIODIC_DAMAGE,            // DoT tick
    PERIODIC_HEAL,              // HoT tick
    PERIODIC_ENERGIZE,          // Periodic resource gain

    // Spell interruption
    SPELL_INTERRUPTED,          // Spell was interrupted
    SPELL_INTERRUPT_FAILED,     // Interrupt attempt failed

    // Dispel and cleanse
    SPELL_DISPELLED,            // Aura was dispelled
    SPELL_DISPEL_FAILED,        // Dispel attempt failed
    SPELL_STOLEN,               // Aura was stolen (Spellsteal)

    // Melee combat
    ATTACK_START,               // Melee combat initiated
    ATTACK_STOP,                // Melee combat ended
    ATTACK_SWING,               // Melee swing (damage)

    // Attack errors
    ATTACK_ERROR_NOT_IN_RANGE,  // Target out of range
    ATTACK_ERROR_BAD_FACING,    // Wrong facing
    ATTACK_ERROR_DEAD_TARGET,   // Target is dead
    ATTACK_ERROR_CANT_ATTACK,   // Can't attack target

    // Threat and aggro
    AI_REACTION,                // NPC aggro change
    THREAT_UPDATE,              // Threat value changed
    THREAT_TRANSFER,            // Threat redirected

    // Crowd control
    CC_APPLIED,                 // CC effect applied
    CC_BROKEN,                  // CC effect broken
    CC_IMMUNE,                  // Target immune to CC

    // Absorb and shields
    SPELL_ABSORB,               // Damage absorbed
    SHIELD_BROKEN,              // Shield depleted

    // Combat state
    COMBAT_ENTERED,             // Entered combat
    COMBAT_LEFT,                // Left combat
    EVADE_START,                // NPC evading

    // Spell school lockout
    SPELL_LOCKOUT,              // School locked from interrupts

    MAX_COMBAT_EVENT
};

/**
 * @enum EventPriority
 * @brief Defines processing priority for combat events
 */
enum class CombatEventPriority : uint8
{
    CRITICAL = 0,   // Interrupts, CC breaks - process immediately
    HIGH = 1,       // Damage, healing, threat - process within 50ms
    MEDIUM = 2,     // DoT/HoT ticks - process within 200ms
    LOW = 3,        // Combat state changes - process within 500ms
    BATCH = 4       // Periodic updates - batch process
};

/**
 * @struct CombatEvent
 * @brief Encapsulates all data for a combat-related event
 */
struct CombatEvent
{
    using EventType = CombatEventType;
    using Priority = CombatEventPriority;

    CombatEventType type;
    CombatEventPriority priority;

    ObjectGuid casterGuid;      // Who cast the spell / initiated action
    ObjectGuid targetGuid;      // Who was targeted
    ObjectGuid victimGuid;      // Who was affected (for AoE, may differ from target)

    uint32 spellId;             // Spell ID (0 for melee)
    int32 amount;               // Damage/healing/resource amount
    uint32 schoolMask;          // Spell school mask
    uint32 flags;               // Combat flags (crit, resist, block, etc.)

    std::chrono::steady_clock::time_point timestamp;
    std::chrono::steady_clock::time_point expiryTime;

    /**
     * @brief Check if event is valid
     * @return true if event has valid data
     */
    bool IsValid() const;

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
     *       Events with same priority are ordered by timestamp (earlier first)
     */
    bool operator<(CombatEvent const& other) const
    {
        if (priority != other.priority)
            return priority > other.priority;
        return timestamp > other.timestamp;
    }

    // ========================================================================
    // HELPER CONSTRUCTORS
    // ========================================================================

    /**
     * @brief Create a spell cast start event
     * @param caster Who is casting the spell
     * @param target Spell target
     * @param spellId Spell ID
     * @param castTime Cast time in milliseconds
     * @return Constructed CombatEvent
     */
    static CombatEvent SpellCastStart(ObjectGuid caster, ObjectGuid target, uint32 spellId, uint32 castTime);

    /**
     * @brief Create a spell cast complete event
     * @param caster Who cast the spell
     * @param target Spell target
     * @param spellId Spell ID
     * @return Constructed CombatEvent
     */
    static CombatEvent SpellCastGo(ObjectGuid caster, ObjectGuid target, uint32 spellId);

    /**
     * @brief Create a spell damage event
     * @param caster Spell caster
     * @param victim Damage victim
     * @param spellId Spell ID
     * @param damage Amount of damage
     * @param school Spell school mask
     * @return Constructed CombatEvent
     */
    static CombatEvent SpellDamage(ObjectGuid caster, ObjectGuid victim, uint32 spellId, int32 damage, uint32 school);

    /**
     * @brief Create a spell heal event
     * @param caster Spell caster
     * @param target Heal target
     * @param spellId Spell ID
     * @param heal Amount of healing
     * @return Constructed CombatEvent
     */
    static CombatEvent SpellHeal(ObjectGuid caster, ObjectGuid target, uint32 spellId, int32 heal);

    /**
     * @brief Create a spell interrupt event
     * @param interrupter Who interrupted
     * @param victim Whose spell was interrupted
     * @param interruptedSpell Interrupted spell ID
     * @param interruptSpell Interrupt spell ID
     * @return Constructed CombatEvent
     */
    static CombatEvent SpellInterrupt(ObjectGuid interrupter, ObjectGuid victim, uint32 interruptedSpell, uint32 interruptSpell);

    /**
     * @brief Create an attack start event
     * @param attacker Who started attacking
     * @param victim Attack target
     * @return Constructed CombatEvent
     */
    static CombatEvent AttackStart(ObjectGuid attacker, ObjectGuid victim);

    /**
     * @brief Create an attack stop event
     * @param attacker Who stopped attacking
     * @param victim Former target
     * @param nowDead Whether victim is now dead
     * @return Constructed CombatEvent
     */
    static CombatEvent AttackStop(ObjectGuid attacker, ObjectGuid victim, bool nowDead);

    /**
     * @brief Create a threat update event
     * @param unit Unit whose threat changed
     * @param victim Threat target
     * @param threatChange Amount of threat change
     * @return Constructed CombatEvent
     */
    static CombatEvent ThreatUpdate(ObjectGuid unit, ObjectGuid victim, int32 threatChange);
};

} // namespace Playerbot

#endif // PLAYERBOT_COMBAT_EVENTS_H
