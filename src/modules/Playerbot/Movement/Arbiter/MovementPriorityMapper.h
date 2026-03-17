/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * MOVEMENT PRIORITY MAPPER
 *
 * Enterprise-grade adapter that maps PlayerBot's granular 0-255 priority system
 * to TrinityCore's 3-level priority system (NONE, NORMAL, HIGHEST).
 *
 * Design Pattern: Adapter Pattern
 * Purpose: Bridge PlayerBot movement priorities to TrinityCore MotionMaster API
 *
 * Architecture:
 * - PlayerBot: 16+ priority categories (0-255 granular)
 * - TrinityCore: 3 priority levels + 2 modes + tie-breaker
 * - Mapping: Intelligent categorization based on urgency
 *
 * Priority Mapping Strategy:
 * - CRITICAL (240+)   → HIGHEST + OVERRIDE (emergency, must complete)
 * - VERY_HIGH (200+)  → HIGHEST + DEFAULT (important, can be overridden)
 * - HIGH (150+)       → NORMAL + OVERRIDE (combat, overrides following)
 * - MEDIUM/LOW (50+)  → NORMAL + DEFAULT (standard movement)
 * - MINIMAL (0-49)    → NONE + DEFAULT (idle, uses MOTION_SLOT_DEFAULT)
 */

#ifndef PLAYERBOT_MOVEMENT_PRIORITY_MAPPER_H
#define PLAYERBOT_MOVEMENT_PRIORITY_MAPPER_H

#include "Define.h"
#include "MovementDefines.h"
#include <string>

namespace Playerbot
{

/**
 * PlayerBot granular movement priorities (0-255)
 *
 * Categories organized by urgency:
 * - CRITICAL: Life-or-death situations
 * - VERY_HIGH: Must complete (interrupts, objectives)
 * - HIGH: Combat positioning
 * - MEDIUM: Tactical movement
 * - LOW: Out-of-combat behavior
 * - MINIMAL: Idle/exploration
 */
enum class PlayerBotMovementPriority : uint8
{
    // ========================================================================
    // CRITICAL (240-255): Life-or-death situations
    // Maps to: MOTION_PRIORITY_HIGHEST + MOTION_MODE_OVERRIDE
    // ========================================================================
    DEATH_RECOVERY = 255,               // Retrieving corpse after death
    BOSS_MECHANIC = 250,                // Boss void zones, fire, beams
    OBSTACLE_AVOIDANCE_EMERGENCY = 245, // Emergency pathfinding around obstacles
    EMERGENCY_DEFENSIVE = 240,          // Fleeing at critical HP

    // ========================================================================
    // VERY_HIGH (200-239): Important, must complete
    // Maps to: MOTION_PRIORITY_HIGHEST + MOTION_MODE_DEFAULT
    // ========================================================================
    INTERRUPT_POSITIONING = 220,        // Moving into interrupt range
    PVP_FLAG_CAPTURE = 210,             // Battleground flag/node capture
    DUNGEON_MECHANIC = 205,             // Dungeon-specific mechanics
    ESCORT_QUEST = 200,                 // Escort NPC protection

    // ========================================================================
    // HIGH (150-199): Combat positioning
    // Maps to: MOTION_PRIORITY_NORMAL + MOTION_MODE_OVERRIDE
    // ========================================================================
    COMBAT_AI = 180,                    // Class-specific combat logic
    KITING = 175,                       // Ranged kiting from melee
    ROLE_POSITIONING = 170,             // Tank/healer/dps positioning
    FORMATION = 160,                    // Group formation in combat
    PET_POSITIONING = 155,              // Hunter/Warlock pet control
    CHARGE_INTERCEPT = 150,             // Warrior charge/intercept

    // ========================================================================
    // MEDIUM (100-149): Tactical movement
    // Maps to: MOTION_PRIORITY_NORMAL + MOTION_MODE_DEFAULT
    // ========================================================================
    COMBAT_MOVEMENT_STRATEGY = 130,     // Generic combat movement
    PVP_TACTICAL = 120,                 // PvP tactical positioning
    TACTICAL_POSITIONING = 115,         // General tactical combat positioning
    DUNGEON_POSITIONING = 110,          // Dungeon pull positioning
    GROUP_COORDINATION = 100,           // Coordinated group movement

    // ========================================================================
    // LOW (50-99): Out-of-combat behavior
    // Maps to: MOTION_PRIORITY_NORMAL + MOTION_MODE_DEFAULT
    // ========================================================================
    FOLLOW = 70,                        // Following group leader
    QUEST = 50,                         // Quest objective movement
    LOOT = 40,                          // Moving to loot corpses

    // ========================================================================
    // MINIMAL (0-49): Idle/exploration
    // Maps to: MOTION_PRIORITY_NONE + MOTION_MODE_DEFAULT
    // Uses MOTION_SLOT_DEFAULT instead of MOTION_SLOT_ACTIVE
    // ========================================================================
    EXPLORATION = 20,                   // Exploring/wandering
    IDLE = 0                            // Stationary idle
};

/**
 * TrinityCore priority mapping result
 *
 * Contains:
 * - priority: TrinityCore's 3-level priority (NONE/NORMAL/HIGHEST)
 * - mode: TrinityCore's mode (DEFAULT/OVERRIDE)
 * - tieBreaker: Original PlayerBot priority for fine-grained ordering
 * - slot: Which MotionMaster slot to use (DEFAULT/ACTIVE)
 */
struct TrinityCorePriority
{
    MovementGeneratorPriority priority;  // NONE=0, NORMAL=1, HIGHEST=2
    MovementGeneratorMode mode;          // DEFAULT=0, OVERRIDE=1
    uint8 tieBreaker;                    // Original 0-255 priority
    MovementSlot slot;                   // DEFAULT or ACTIVE

    // Comparison operators for testing
    bool operator==(TrinityCorePriority const& other) const
    {
        return priority == other.priority &&
               mode == other.mode &&
               tieBreaker == other.tieBreaker &&
               slot == other.slot;
    }

    bool operator!=(TrinityCorePriority const& other) const
    {
        return !(*this == other);
    }

    // Debug string
    ::std::string ToString() const;
};

/**
 * Movement Priority Mapper
 *
 * Stateless utility class that maps PlayerBot priorities to TrinityCore.
 * All methods are static and thread-safe.
 *
 * Usage:
 *   auto tc = MovementPriorityMapper::Map(PlayerBotMovementPriority::BOSS_MECHANIC);
 *   // tc.priority = MOTION_PRIORITY_HIGHEST
 *   // tc.mode = MOTION_MODE_OVERRIDE
 *   // tc.tieBreaker = 250
 *   // tc.slot = MOTION_SLOT_ACTIVE
 */
class TC_GAME_API MovementPriorityMapper
{
public:
    /**
     * Map PlayerBot priority to TrinityCore priority
     *
     * @param pbPriority PlayerBot priority (0-255)
     * @return TrinityCore priority mapping
     *
     * Thread-Safe: Yes (pure function, no state)
     * Performance: O(1) - simple range checks
     */
    static TrinityCorePriority Map(PlayerBotMovementPriority pbPriority);

    /**
     * Get human-readable name for PlayerBot priority
     *
     * @param priority PlayerBot priority
     * @return String name (e.g., "BOSS_MECHANIC")
     */
    static ::std::string GetPriorityName(PlayerBotMovementPriority priority);

    /**
     * Get human-readable description for PlayerBot priority
     *
     * @param priority PlayerBot priority
     * @return Description string
     */
    static ::std::string GetPriorityDescription(PlayerBotMovementPriority priority);

    /**
     * Validate that priority value is within valid range
     *
     * @param value Raw priority value (0-255)
     * @return true if valid
     */
    static bool IsValidPriority(uint8 value);

    /**
     * Get priority category name
     *
     * @param priority PlayerBot priority
     * @return Category name ("CRITICAL", "VERY_HIGH", etc.)
     */
    static ::std::string GetCategoryName(PlayerBotMovementPriority priority);

private:
    // Private constructor - utility class, no instances
    MovementPriorityMapper() = delete;
};

} // namespace Playerbot

#endif // PLAYERBOT_MOVEMENT_PRIORITY_MAPPER_H
