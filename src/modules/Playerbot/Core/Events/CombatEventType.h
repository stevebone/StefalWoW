/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 */

#ifndef TRINITYCORE_COMBAT_EVENT_TYPE_H
#define TRINITYCORE_COMBAT_EVENT_TYPE_H

#include <cstdint>

namespace Playerbot {

/**
 * @enum CombatEventType
 * @brief Bitmask-based combat event types for efficient filtering
 *
 * Phase 3 Architecture: Event-driven combat system
 * Uses bitmasks for O(1) subscription filtering
 *
 * Categories:
 * - 0x0001-0x000F: Damage events
 * - 0x0010-0x00F0: Healing events
 * - 0x0100-0x0F00: Spell events
 * - 0x1000-0xF000: Threat events
 * - 0x10000-0xF0000: Aura events
 * - 0x100000-0xF00000: Combat state events
 * - 0x1000000-0xF000000: Unit events
 * - 0x10000000-0xF0000000: Encounter events
 */
enum class CombatEventType : uint32_t {
    NONE = 0,

    // Damage Events (0x0001 - 0x000F)
    DAMAGE_TAKEN        = 0x0001,
    DAMAGE_DEALT        = 0x0002,
    DAMAGE_ABSORBED     = 0x0004,

    // Healing Events (0x0010 - 0x00F0)
    HEALING_RECEIVED    = 0x0010,
    HEALING_DONE        = 0x0020,
    OVERHEAL            = 0x0040,

    // Spell Events (0x0100 - 0x0F00)
    SPELL_CAST_START    = 0x0100,
    SPELL_CAST_SUCCESS  = 0x0200,
    SPELL_CAST_FAILED   = 0x0400,
    SPELL_INTERRUPTED   = 0x0800,

    // Threat Events (0x1000 - 0xF000)
    THREAT_CHANGED      = 0x1000,
    TAUNT_APPLIED       = 0x2000,
    THREAT_WIPE         = 0x4000,

    // Aura Events (0x10000 - 0xF0000)
    AURA_APPLIED        = 0x10000,
    AURA_REMOVED        = 0x20000,
    AURA_REFRESHED      = 0x40000,
    AURA_STACK_CHANGED  = 0x80000,

    // Combat State Events (0x100000 - 0xF00000)
    COMBAT_STARTED      = 0x100000,
    COMBAT_ENDED        = 0x200000,

    // Unit Events (0x1000000 - 0xF000000)
    UNIT_DIED           = 0x1000000,
    UNIT_RESURRECTED    = 0x2000000,
    UNIT_TARGET_CHANGED = 0x4000000,

    // Encounter Events (0x10000000 - 0xF0000000)
    ENCOUNTER_START     = 0x10000000,
    ENCOUNTER_END       = 0x20000000,
    BOSS_PHASE_CHANGED  = 0x40000000,

    // Convenience masks for subscription
    ALL_DAMAGE          = DAMAGE_TAKEN | DAMAGE_DEALT | DAMAGE_ABSORBED,
    ALL_HEALING         = HEALING_RECEIVED | HEALING_DONE | OVERHEAL,
    ALL_SPELL           = SPELL_CAST_START | SPELL_CAST_SUCCESS | SPELL_CAST_FAILED | SPELL_INTERRUPTED,
    ALL_THREAT          = THREAT_CHANGED | TAUNT_APPLIED | THREAT_WIPE,
    ALL_AURA            = AURA_APPLIED | AURA_REMOVED | AURA_REFRESHED | AURA_STACK_CHANGED,
    ALL_COMBAT_STATE    = COMBAT_STARTED | COMBAT_ENDED,
    ALL_UNIT            = UNIT_DIED | UNIT_RESURRECTED | UNIT_TARGET_CHANGED,
    ALL_ENCOUNTER       = ENCOUNTER_START | ENCOUNTER_END | BOSS_PHASE_CHANGED,
    ALL_EVENTS          = 0xFFFFFFFF
};

// Enable bitwise OR operator
inline CombatEventType operator|(CombatEventType a, CombatEventType b) {
    return static_cast<CombatEventType>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

// Enable bitwise AND operator
inline CombatEventType operator&(CombatEventType a, CombatEventType b) {
    return static_cast<CombatEventType>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

// Enable bitwise OR assignment
inline CombatEventType& operator|=(CombatEventType& a, CombatEventType b) {
    a = a | b;
    return a;
}

// Enable bitwise AND assignment
inline CombatEventType& operator&=(CombatEventType& a, CombatEventType b) {
    a = a & b;
    return a;
}

// Enable bitwise NOT operator
inline CombatEventType operator~(CombatEventType a) {
    return static_cast<CombatEventType>(~static_cast<uint32_t>(a));
}

/**
 * @brief Check if a mask contains a specific flag
 *
 * @param mask Bitmask to check
 * @param flag Flag to look for
 * @return true if flag is present in mask
 *
 * Usage:
 * @code
 * if (HasFlag(subscribedTypes, CombatEventType::SPELL_CAST_START)) {
 *     // Handle spell cast start
 * }
 * @endcode
 */
inline bool HasFlag(CombatEventType mask, CombatEventType flag) {
    return (static_cast<uint32_t>(mask) & static_cast<uint32_t>(flag)) != 0;
}

/**
 * @brief Convert event type to string for logging
 *
 * @param type Event type
 * @return String representation
 */
inline const char* CombatEventTypeToString(CombatEventType type) {
    switch (type) {
        case CombatEventType::NONE:               return "NONE";
        case CombatEventType::DAMAGE_TAKEN:       return "DAMAGE_TAKEN";
        case CombatEventType::DAMAGE_DEALT:       return "DAMAGE_DEALT";
        case CombatEventType::DAMAGE_ABSORBED:    return "DAMAGE_ABSORBED";
        case CombatEventType::HEALING_RECEIVED:   return "HEALING_RECEIVED";
        case CombatEventType::HEALING_DONE:       return "HEALING_DONE";
        case CombatEventType::OVERHEAL:           return "OVERHEAL";
        case CombatEventType::SPELL_CAST_START:   return "SPELL_CAST_START";
        case CombatEventType::SPELL_CAST_SUCCESS: return "SPELL_CAST_SUCCESS";
        case CombatEventType::SPELL_CAST_FAILED:  return "SPELL_CAST_FAILED";
        case CombatEventType::SPELL_INTERRUPTED:  return "SPELL_INTERRUPTED";
        case CombatEventType::THREAT_CHANGED:     return "THREAT_CHANGED";
        case CombatEventType::TAUNT_APPLIED:      return "TAUNT_APPLIED";
        case CombatEventType::THREAT_WIPE:        return "THREAT_WIPE";
        case CombatEventType::AURA_APPLIED:       return "AURA_APPLIED";
        case CombatEventType::AURA_REMOVED:       return "AURA_REMOVED";
        case CombatEventType::AURA_REFRESHED:     return "AURA_REFRESHED";
        case CombatEventType::AURA_STACK_CHANGED: return "AURA_STACK_CHANGED";
        case CombatEventType::COMBAT_STARTED:     return "COMBAT_STARTED";
        case CombatEventType::COMBAT_ENDED:       return "COMBAT_ENDED";
        case CombatEventType::UNIT_DIED:          return "UNIT_DIED";
        case CombatEventType::UNIT_RESURRECTED:   return "UNIT_RESURRECTED";
        case CombatEventType::UNIT_TARGET_CHANGED:return "UNIT_TARGET_CHANGED";
        case CombatEventType::ENCOUNTER_START:    return "ENCOUNTER_START";
        case CombatEventType::ENCOUNTER_END:      return "ENCOUNTER_END";
        case CombatEventType::BOSS_PHASE_CHANGED: return "BOSS_PHASE_CHANGED";
        default:                                  return "UNKNOWN";
    }
}

} // namespace Playerbot

#endif // TRINITYCORE_COMBAT_EVENT_TYPE_H
