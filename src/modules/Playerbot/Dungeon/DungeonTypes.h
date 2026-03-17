/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"

namespace Playerbot
{

/**
 * @brief Player role in dungeon
 *
 * Canonical definition used across all dungeon systems.
 * Do NOT define DungeonRole elsewhere - include this header instead.
 */
enum class DungeonRole : uint8
{
    TANK            = 0,
    HEALER          = 1,
    MELEE_DPS       = 2,
    RANGED_DPS      = 3,
    CROWD_CONTROL   = 4,
    SUPPORT         = 5
};

/**
 * @brief Current phase of dungeon progression
 */
enum class DungeonPhase : uint8
{
    ENTERING        = 0,
    CLEARING_TRASH  = 1,
    BOSS_ENCOUNTER  = 2,
    LOOTING         = 3,
    RESTING         = 4,
    COMPLETED       = 5,
    WIPED           = 6
};

/**
 * @brief Strategy type for encounter execution
 */
enum class EncounterStrategyType : uint8
{
    CONSERVATIVE    = 0,  // Safe, methodical approach
    AGGRESSIVE      = 1,  // Fast, high-risk approach
    BALANCED        = 2,  // Moderate risk/reward
    ADAPTIVE        = 3,  // Adapt based on group performance
    SPEED_RUN       = 4,  // Maximum speed completion
    LEARNING        = 5   // Educational approach for new content
};

/**
 * @brief Threat management approach
 */
enum class ThreatManagement : uint8
{
    STRICT_AGGRO    = 0,  // Maintain strict threat control
    LOOSE_AGGRO     = 1,  // Allow some threat variation
    BURN_STRATEGY   = 2,  // Ignore threat, focus DPS
    TANK_SWAP       = 3,  // Coordinate tank swapping
    OFF_TANK        = 4   // Off-tank specific mechanics
};

/**
 * @brief Mechanic types for dungeon encounters
 */
enum class MechanicType : uint8
{
    INTERRUPT       = 0,  // Spell interrupts
    GROUND_AVOID    = 1,  // Ground effect avoidance
    ADD_PRIORITY    = 2,  // Add kill priority
    POSITIONING     = 3,  // Player positioning
    DISPEL          = 4,  // Dispel mechanics
    MOVEMENT        = 5,  // Movement mechanics
    TANK_SWAP       = 6,  // Tank swap mechanics
    SPREAD          = 7,  // Spread out mechanic
    STACK           = 8   // Stack together mechanic
};

} // namespace Playerbot
