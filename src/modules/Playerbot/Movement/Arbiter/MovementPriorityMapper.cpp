/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "MovementPriorityMapper.h"
#include "Log.h"
#include <sstream>

namespace Playerbot
{

// ============================================================================
// TrinityCorePriority::ToString()
// ============================================================================

::std::string TrinityCorePriority::ToString() const
{
    ::std::ostringstream oss;
    oss << "TrinityCorePriority{";

    // Priority
    oss << "priority=";
    switch (priority)
    {
        case MOTION_PRIORITY_NONE:    oss << "NONE"; break;
        case MOTION_PRIORITY_NORMAL:  oss << "NORMAL"; break;
        case MOTION_PRIORITY_HIGHEST: oss << "HIGHEST"; break;
        default:                      oss << "UNKNOWN(" << static_cast<int>(priority) << ")"; break;
    }

    // Mode
    oss << ", mode=";
    switch (mode)
    {
        case MOTION_MODE_DEFAULT:  oss << "DEFAULT"; break;
        case MOTION_MODE_OVERRIDE: oss << "OVERRIDE"; break;
        default:                   oss << "UNKNOWN(" << static_cast<int>(mode) << ")"; break;
    }

    // Tie-breaker
    oss << ", tieBreaker=" << static_cast<int>(tieBreaker);

    // Slot
    oss << ", slot=";
    switch (slot)
    {
        case MOTION_SLOT_DEFAULT: oss << "DEFAULT"; break;
        case MOTION_SLOT_ACTIVE:  oss << "ACTIVE"; break;
        default:                  oss << "UNKNOWN(" << static_cast<int>(slot) << ")"; break;
    }

    oss << "}";
    return oss.str();
}

// ============================================================================
// MovementPriorityMapper::Map() - Core Mapping Logic
// ============================================================================

TrinityCorePriority MovementPriorityMapper::Map(PlayerBotMovementPriority pbPriority)
{
    uint8 value = static_cast<uint8>(pbPriority);

    // ========================================================================
    // CRITICAL (240-255): Life-or-death emergencies
    // → HIGHEST priority, OVERRIDE mode, ACTIVE slot
    // ========================================================================
    // These movements MUST NOT be interrupted by anything.
    // Examples: Boss void zones, death recovery, emergency fleeing
    //
    // TrinityCore Behavior:
    // - HIGHEST priority beats NORMAL priority
    // - OVERRIDE mode beats DEFAULT mode at same priority
    // - Result: Always executes immediately, cancels lower priority
    if (value >= 240)
    {
        return TrinityCorePriority{
            MOTION_PRIORITY_HIGHEST,  // Highest TC priority
            MOTION_MODE_OVERRIDE,     // Override mode (beats DEFAULT)
            value,                    // Preserve original for tie-breaking
            MOTION_SLOT_ACTIVE        // Active movement slot
        };
    }

    // ========================================================================
    // VERY_HIGH (200-239): Important but can be overridden by CRITICAL
    // → HIGHEST priority, DEFAULT mode, ACTIVE slot
    // ========================================================================
    // These movements are important and should usually complete.
    // Examples: Interrupts, PvP objectives, escort quests
    //
    // TrinityCore Behavior:
    // - HIGHEST priority beats NORMAL priority
    // - DEFAULT mode - can be overridden by OVERRIDE at same priority
    // - Result: Beats normal combat positioning, but CRITICAL can override
    if (value >= 200)
    {
        return TrinityCorePriority{
            MOTION_PRIORITY_HIGHEST,
            MOTION_MODE_DEFAULT,
            value,
            MOTION_SLOT_ACTIVE
        };
    }

    // ========================================================================
    // HIGH (150-199): Combat positioning
    // → NORMAL priority, OVERRIDE mode, ACTIVE slot
    // ========================================================================
    // These movements are combat-related and should override following.
    // Examples: ClassAI combat logic, kiting, role positioning
    //
    // TrinityCore Behavior:
    // - NORMAL priority (same as following)
    // - OVERRIDE mode beats DEFAULT mode
    // - Result: Overrides following, but loses to HIGHEST priority
    if (value >= 150)
    {
        return TrinityCorePriority{
            MOTION_PRIORITY_NORMAL,
            MOTION_MODE_OVERRIDE,
            value,
            MOTION_SLOT_ACTIVE
        };
    }

    // ========================================================================
    // MEDIUM/LOW (50-149): Standard movement
    // → NORMAL priority, DEFAULT mode, ACTIVE slot
    // ========================================================================
    // These are standard movements during normal gameplay.
    // Examples: Following, questing, looting
    //
    // TrinityCore Behavior:
    // - NORMAL priority
    // - DEFAULT mode (can be overridden)
    // - Result: Standard movement behavior
    if (value >= 50)
    {
        return TrinityCorePriority{
            MOTION_PRIORITY_NORMAL,
            MOTION_MODE_DEFAULT,
            value,
            MOTION_SLOT_ACTIVE
        };
    }

    // ========================================================================
    // MINIMAL (0-49): Idle/exploration
    // → NONE priority, DEFAULT mode, DEFAULT slot
    // ========================================================================
    // These are idle behaviors when bot has nothing else to do.
    // Examples: Wandering, standing idle
    //
    // TrinityCore Behavior:
    // - NONE priority (lowest)
    // - Uses MOTION_SLOT_DEFAULT (separate slot from active movements)
    // - Result: Can always be interrupted, doesn't interfere with active movement
    return TrinityCorePriority{
        MOTION_PRIORITY_NONE,
        MOTION_MODE_DEFAULT,
        value,
        MOTION_SLOT_DEFAULT  // Note: Different slot!
    };
}

// ============================================================================
// MovementPriorityMapper::GetPriorityName()
// ============================================================================

::std::string MovementPriorityMapper::GetPriorityName(PlayerBotMovementPriority priority)
{
    switch (priority)
    {
        // CRITICAL
        case PlayerBotMovementPriority::DEATH_RECOVERY:
            return "DEATH_RECOVERY";
        case PlayerBotMovementPriority::BOSS_MECHANIC:
            return "BOSS_MECHANIC";
        case PlayerBotMovementPriority::OBSTACLE_AVOIDANCE_EMERGENCY:
            return "OBSTACLE_AVOIDANCE_EMERGENCY";
        case PlayerBotMovementPriority::EMERGENCY_DEFENSIVE:
            return "EMERGENCY_DEFENSIVE";

        // VERY_HIGH
        case PlayerBotMovementPriority::INTERRUPT_POSITIONING:
            return "INTERRUPT_POSITIONING";
        case PlayerBotMovementPriority::PVP_FLAG_CAPTURE:
            return "PVP_FLAG_CAPTURE";
        case PlayerBotMovementPriority::DUNGEON_MECHANIC:
            return "DUNGEON_MECHANIC";
        case PlayerBotMovementPriority::ESCORT_QUEST:
            return "ESCORT_QUEST";

        // HIGH
        case PlayerBotMovementPriority::COMBAT_AI:
            return "COMBAT_AI";
        case PlayerBotMovementPriority::KITING:
            return "KITING";
        case PlayerBotMovementPriority::ROLE_POSITIONING:
            return "ROLE_POSITIONING";
        case PlayerBotMovementPriority::FORMATION:
            return "FORMATION";
        case PlayerBotMovementPriority::PET_POSITIONING:
            return "PET_POSITIONING";
        case PlayerBotMovementPriority::CHARGE_INTERCEPT:
            return "CHARGE_INTERCEPT";

        // MEDIUM
        case PlayerBotMovementPriority::COMBAT_MOVEMENT_STRATEGY:
            return "COMBAT_MOVEMENT_STRATEGY";
        case PlayerBotMovementPriority::PVP_TACTICAL:
            return "PVP_TACTICAL";
        case PlayerBotMovementPriority::DUNGEON_POSITIONING:
            return "DUNGEON_POSITIONING";
        case PlayerBotMovementPriority::GROUP_COORDINATION:
            return "GROUP_COORDINATION";

        // LOW
        case PlayerBotMovementPriority::FOLLOW:
            return "FOLLOW";
        case PlayerBotMovementPriority::QUEST:
            return "QUEST";
        case PlayerBotMovementPriority::LOOT:
            return "LOOT";

        // MINIMAL
        case PlayerBotMovementPriority::EXPLORATION:
            return "EXPLORATION";
        case PlayerBotMovementPriority::IDLE:
            return "IDLE";

        default:
        {
            ::std::ostringstream oss;
            oss << "UNKNOWN_PRIORITY_" << static_cast<int>(priority);
            return oss.str();
        }
    }
}

// ============================================================================
// MovementPriorityMapper::GetPriorityDescription()
// ============================================================================

::std::string MovementPriorityMapper::GetPriorityDescription(PlayerBotMovementPriority priority)
{
    switch (priority)
    {
        // CRITICAL
        case PlayerBotMovementPriority::DEATH_RECOVERY:
            return "Retrieving corpse after death";
        case PlayerBotMovementPriority::BOSS_MECHANIC:
            return "Avoiding boss mechanics (void zones, fire, beams)";
        case PlayerBotMovementPriority::OBSTACLE_AVOIDANCE_EMERGENCY:
            return "Emergency pathfinding around obstacles";
        case PlayerBotMovementPriority::EMERGENCY_DEFENSIVE:
            return "Fleeing at critical health";

        // VERY_HIGH
        case PlayerBotMovementPriority::INTERRUPT_POSITIONING:
            return "Moving into interrupt range";
        case PlayerBotMovementPriority::PVP_FLAG_CAPTURE:
            return "Capturing battleground objectives";
        case PlayerBotMovementPriority::DUNGEON_MECHANIC:
            return "Dungeon-specific mechanic positioning";
        case PlayerBotMovementPriority::ESCORT_QUEST:
            return "Protecting escort NPC";

        // HIGH
        case PlayerBotMovementPriority::COMBAT_AI:
            return "Class-specific combat positioning";
        case PlayerBotMovementPriority::KITING:
            return "Ranged kiting from melee enemies";
        case PlayerBotMovementPriority::ROLE_POSITIONING:
            return "Tank/healer/DPS role positioning";
        case PlayerBotMovementPriority::FORMATION:
            return "Maintaining group formation";
        case PlayerBotMovementPriority::PET_POSITIONING:
            return "Hunter/Warlock pet positioning";
        case PlayerBotMovementPriority::CHARGE_INTERCEPT:
            return "Warrior charge/intercept gap closer";

        // MEDIUM
        case PlayerBotMovementPriority::COMBAT_MOVEMENT_STRATEGY:
            return "Generic combat movement positioning";
        case PlayerBotMovementPriority::PVP_TACTICAL:
            return "PvP tactical positioning";
        case PlayerBotMovementPriority::DUNGEON_POSITIONING:
            return "Dungeon pull positioning";
        case PlayerBotMovementPriority::GROUP_COORDINATION:
            return "Coordinated group movement";

        // LOW
        case PlayerBotMovementPriority::FOLLOW:
            return "Following group leader";
        case PlayerBotMovementPriority::QUEST:
            return "Moving to quest objective";
        case PlayerBotMovementPriority::LOOT:
            return "Moving to loot corpses";

        // MINIMAL
        case PlayerBotMovementPriority::EXPLORATION:
            return "Exploring/wandering";
        case PlayerBotMovementPriority::IDLE:
            return "Stationary idle";

        default:
            return "Unknown priority";
    }
}

// ============================================================================
// MovementPriorityMapper::IsValidPriority()
// ============================================================================

bool MovementPriorityMapper::IsValidPriority(uint8 value)
{
    // All values 0-255 are technically valid in the enum
    // This function exists for future validation rules
    return true;
}

// ============================================================================
// MovementPriorityMapper::GetCategoryName()
// ============================================================================

::std::string MovementPriorityMapper::GetCategoryName(PlayerBotMovementPriority priority)
{
    uint8 value = static_cast<uint8>(priority);

    if (value >= 240)
        return "CRITICAL";
    if (value >= 200)
        return "VERY_HIGH";
    if (value >= 150)
        return "HIGH";
    if (value >= 100)
        return "MEDIUM";
    if (value >= 50)
        return "LOW";
    return "MINIMAL";
}

} // namespace Playerbot
