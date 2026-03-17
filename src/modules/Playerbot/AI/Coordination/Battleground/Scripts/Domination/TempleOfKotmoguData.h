/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2021+ WarheadCore <https://github.com/AzerothCore/WarheadCore>
 * Copyright (C) 2025+ TrinityCore Playerbot Integration
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_AI_COORDINATION_BG_DOMINATION_TEMPLEOFKOTMOGUDATA_H
#define PLAYERBOT_AI_COORDINATION_BG_DOMINATION_TEMPLEOFKOTMOGUDATA_H

#include "Define.h"
#include "Position.h"
#include <array>
#include <vector>

namespace Playerbot::Coordination::Battleground::TempleOfKotmogu
{

// ============================================================================
// MAP INFORMATION
// ============================================================================

constexpr uint32 MAP_ID = 998;
constexpr char BG_NAME[] = "Temple of Kotmogu";
constexpr uint32 MAX_SCORE = 1500;
constexpr uint32 MAX_DURATION = 25 * 60 * 1000; // 25 minutes
constexpr uint8 TEAM_SIZE = 10;
constexpr uint32 ORB_COUNT = 4;
constexpr uint32 TICK_INTERVAL = 2000;          // 2 seconds

// ============================================================================
// ORB IDENTIFIERS
// ============================================================================

namespace Orbs
{
    constexpr uint32 ORANGE = 0;  // Northeast corner
    constexpr uint32 BLUE = 1;    // Northwest corner
    constexpr uint32 GREEN = 2;   // Southeast corner
    constexpr uint32 PURPLE = 3;  // Southwest corner
}

// ============================================================================
// ORB POSITIONS (Corner spawns)
// CRITICAL: These must match TrinityCore's battleground_temple_of_kotmogu.cpp
// The Z level is ~13 (arena floor), NOT 29 (some elevated area)
// ============================================================================

constexpr float ORANGE_ORB_X = 1850.1666f;
constexpr float ORANGE_ORB_Y = 1250.1180f;
constexpr float ORANGE_ORB_Z = 13.2056f;
constexpr float ORANGE_ORB_O = 3.93f;  // Facing center

constexpr float BLUE_ORB_X = 1716.9479f;
constexpr float BLUE_ORB_Y = 1250.0173f;
constexpr float BLUE_ORB_Z = 13.3266f;
constexpr float BLUE_ORB_O = 5.50f;  // Facing center

constexpr float GREEN_ORB_X = 1716.8923f;
constexpr float GREEN_ORB_Y = 1416.6180f;
constexpr float GREEN_ORB_Z = 13.2056f;
constexpr float GREEN_ORB_O = 0.79f;  // Facing center

constexpr float PURPLE_ORB_X = 1850.2170f;
constexpr float PURPLE_ORB_Y = 1416.8229f;
constexpr float PURPLE_ORB_Z = 13.3382f;
constexpr float PURPLE_ORB_O = 2.36f;  // Facing center

// Array for iteration
constexpr float ORB_POSITIONS[][4] = {
    { ORANGE_ORB_X, ORANGE_ORB_Y, ORANGE_ORB_Z, ORANGE_ORB_O },  // Orange
    { BLUE_ORB_X, BLUE_ORB_Y, BLUE_ORB_Z, BLUE_ORB_O },          // Blue
    { GREEN_ORB_X, GREEN_ORB_Y, GREEN_ORB_Z, GREEN_ORB_O },      // Green
    { PURPLE_ORB_X, PURPLE_ORB_Y, PURPLE_ORB_Z, PURPLE_ORB_O }   // Purple
};

inline Position GetOrbPosition(uint32 orbId)
{
    if (orbId < ORB_COUNT)
        return Position(ORB_POSITIONS[orbId][0], ORB_POSITIONS[orbId][1],
            ORB_POSITIONS[orbId][2], ORB_POSITIONS[orbId][3]);
    return Position(0, 0, 0, 0);
}

inline const char* GetOrbName(uint32 orbId)
{
    switch (orbId)
    {
        case Orbs::ORANGE: return "Orange Orb";
        case Orbs::BLUE:   return "Blue Orb";
        case Orbs::GREEN:  return "Green Orb";
        case Orbs::PURPLE: return "Purple Orb";
        default: return "Unknown Orb";
    }
}

// Orb strategic values (all equal - any orb is good)
inline uint8 GetOrbStrategicValue(uint32 orbId)
{
    return 8;  // All orbs have equal value
}

// ============================================================================
// CENTER ZONE (Bonus points area)
// ============================================================================

// Geometric center of the arena computed from the 4 orb positions:
//   X = (1850.17+1716.95+1716.89+1850.22)/4 = 1783.56
//   Y = (1250.12+1250.02+1416.62+1416.82)/4 = 1333.40
// Confirmed by healing buff midpoint: (1783.0, 1333.5)
constexpr float CENTER_X = 1783.5f;
constexpr float CENTER_Y = 1333.4f;
constexpr float CENTER_Z = 13.0f;
constexpr float CENTER_O = 0.0f;
constexpr float CENTER_RADIUS = 25.0f;  // Distance for "center" bonus

inline Position GetCenterPosition()
{
    return Position(CENTER_X, CENTER_Y, CENTER_Z, CENTER_O);
}

inline bool IsInCenterZone(float x, float y)
{
    float dx = x - CENTER_X;
    float dy = y - CENTER_Y;
    return (dx * dx + dy * dy) <= (CENTER_RADIUS * CENTER_RADIUS);
}

// ============================================================================
// POINT VALUES
// ============================================================================

// Base points per tick based on orb count (outside center)
constexpr uint32 POINTS_BASE = 3;           // Per orb, outside center
constexpr uint32 POINTS_CENTER = 5;         // Per orb, in center
constexpr uint32 POINTS_CENTER_BONUS = 10;  // Additional bonus per orb in center

// Points scaling: outside = 3 per orb, center = 5 + 10 bonus = 15 per orb
constexpr std::array<uint32, 5> TICK_POINTS_OUTSIDE = { 0, 3, 6, 9, 12 };  // 0-4 orbs
constexpr std::array<uint32, 5> TICK_POINTS_CENTER = { 0, 15, 30, 45, 60 };  // Center bonus

// ============================================================================
// SPAWN POSITIONS
// ============================================================================

// Alliance spawn (East side)
constexpr Position ALLIANCE_SPAWNS[] = {
    { 1790.0f, 1312.0f, 35.0f, 3.14f },
    { 1795.0f, 1307.0f, 35.0f, 3.14f },
    { 1785.0f, 1317.0f, 35.0f, 3.14f },
    { 1795.0f, 1317.0f, 35.0f, 3.14f },
    { 1785.0f, 1307.0f, 35.0f, 3.14f }
};

// Horde spawn (West side)
constexpr Position HORDE_SPAWNS[] = {
    { 1674.0f, 1263.0f, 35.0f, 0.0f },
    { 1669.0f, 1268.0f, 35.0f, 0.0f },
    { 1679.0f, 1258.0f, 35.0f, 0.0f },
    { 1669.0f, 1258.0f, 35.0f, 0.0f },
    { 1679.0f, 1268.0f, 35.0f, 0.0f }
};

// ============================================================================
// ORB DEFENSE POSITIONS (ENTERPRISE-GRADE)
// ============================================================================

inline std::vector<Position> GetOrbDefensePositions(uint32 orbId)
{
    Position orbPos = GetOrbPosition(orbId);
    float x = orbPos.GetPositionX();
    float y = orbPos.GetPositionY();
    float z = orbPos.GetPositionZ();

    switch (orbId)
    {
        case Orbs::ORANGE:  // NE corner
            return {
                // Core orb defense
                { x, y, z, 3.93f },           // Orb position
                { x - 5.0f, y, z, 3.14f },    // West
                { x, y + 5.0f, z, 4.71f },    // North
                { x + 5.0f, y, z, 0.0f },     // East (wall)
                { x, y - 5.0f, z, 1.57f },    // South
                // Corner defense
                { x - 8.0f, y - 8.0f, z, 2.36f },  // Toward center
                // Platform edge
                { x - 3.0f, y + 3.0f, z + 2.0f, 3.93f },  // Elevated
                { x - 3.0f, y - 3.0f, z + 2.0f, 2.36f }   // Elevated south
            };
        case Orbs::BLUE:  // NW corner
            return {
                // Core orb defense
                { x, y, z, 5.50f },
                { x + 5.0f, y, z, 0.0f },     // East
                { x, y - 5.0f, z, 1.57f },    // South
                { x - 5.0f, y, z, 3.14f },    // West (wall)
                { x, y + 5.0f, z, 4.71f },    // North (wall)
                // Corner defense
                { x + 8.0f, y - 8.0f, z, 0.79f },  // Toward center
                // Platform edge
                { x + 3.0f, y - 3.0f, z + 2.0f, 0.79f },
                { x + 3.0f, y + 3.0f, z + 2.0f, 5.50f }
            };
        case Orbs::GREEN:  // SE corner
            return {
                // Core orb defense
                { x, y, z, 0.79f },
                { x - 5.0f, y, z, 3.14f },    // West (wall)
                { x, y + 5.0f, z, 4.71f },    // North
                { x + 5.0f, y, z, 0.0f },     // East
                { x, y - 5.0f, z, 1.57f },    // South (wall)
                // Corner defense
                { x + 8.0f, y + 8.0f, z, 5.50f },  // Toward center
                // Platform edge
                { x + 3.0f, y + 3.0f, z + 2.0f, 0.79f },
                { x - 3.0f, y + 3.0f, z + 2.0f, 2.36f }
            };
        case Orbs::PURPLE:  // SW corner
            return {
                // Core orb defense
                { x, y, z, 2.36f },
                { x + 5.0f, y, z, 0.0f },     // East
                { x, y + 5.0f, z, 4.71f },    // North (wall)
                { x - 5.0f, y, z, 3.14f },    // West (wall)
                { x, y - 5.0f, z, 1.57f },    // South
                // Corner defense
                { x + 8.0f, y - 8.0f, z, 0.79f },  // Toward center
                // Platform edge
                { x + 3.0f, y - 3.0f, z + 2.0f, 0.79f },
                { x + 3.0f, y + 3.0f, z + 2.0f, 2.36f }
            };
        default:
            return {};
    }
}

// ============================================================================
// CENTER ZONE POSITIONS
// ============================================================================

inline std::vector<Position> GetCenterDefensePositions()
{
    return {
        // Center core
        { CENTER_X, CENTER_Y, CENTER_Z, 0.0f },          // Dead center
        { CENTER_X + 10.0f, CENTER_Y, CENTER_Z, 0.0f },  // East
        { CENTER_X - 10.0f, CENTER_Y, CENTER_Z, 3.14f }, // West
        { CENTER_X, CENTER_Y + 10.0f, CENTER_Z, 4.71f }, // North
        { CENTER_X, CENTER_Y - 10.0f, CENTER_Z, 1.57f }, // South

        // Inner ring (optimal positions)
        { CENTER_X + 8.0f, CENTER_Y + 8.0f, CENTER_Z, 5.50f },   // NE
        { CENTER_X - 8.0f, CENTER_Y + 8.0f, CENTER_Z, 3.93f },   // NW
        { CENTER_X + 8.0f, CENTER_Y - 8.0f, CENTER_Z, 0.79f },   // SE
        { CENTER_X - 8.0f, CENTER_Y - 8.0f, CENTER_Z, 2.36f },   // SW

        // Outer ring (edge of center zone)
        { CENTER_X + 15.0f, CENTER_Y, CENTER_Z + 2.0f, 0.0f },   // E elevated
        { CENTER_X - 15.0f, CENTER_Y, CENTER_Z + 2.0f, 3.14f },  // W elevated
        { CENTER_X, CENTER_Y + 15.0f, CENTER_Z + 2.0f, 4.71f }   // N elevated
    };
}

// ============================================================================
// ORB CARRIER ROUTES (To Center)
// ============================================================================

inline std::vector<Position> GetOrbCarrierRoute(uint32 orbId)
{
    Position orbPos = GetOrbPosition(orbId);
    Position center = GetCenterPosition();

    // Routes go from orb corner positions across the arena floor to center.
    // All waypoints at Z≈13 (arena floor). Pathfinding handles terrain details.
    switch (orbId)
    {
        case Orbs::ORANGE:  // NE corner (1850, 1250) → Center (1784, 1333)
            return {
                orbPos,
                { 1820.0f, 1290.0f, 13.0f, 3.93f },  // Midway toward center
                center
            };
        case Orbs::BLUE:  // NW corner (1717, 1250) → Center (1784, 1333)
            return {
                orbPos,
                { 1750.0f, 1290.0f, 13.0f, 0.79f },  // Midway toward center
                center
            };
        case Orbs::GREEN:  // SE corner (1717, 1417) → Center (1784, 1333)
            return {
                orbPos,
                { 1750.0f, 1375.0f, 13.0f, 5.50f },  // Midway toward center
                center
            };
        case Orbs::PURPLE:  // SW corner (1850, 1417) → Center (1784, 1333)
            return {
                orbPos,
                { 1820.0f, 1375.0f, 13.0f, 2.36f },  // Midway toward center
                center
            };
        default:
            return { orbPos, center };
    }
}

// ============================================================================
// ESCORT FORMATION POSITIONS
// ============================================================================

// Dynamic positions around an orb carrier
inline std::vector<Position> GetEscortFormation(float carrierX, float carrierY, float carrierZ)
{
    return {
        // Close escort (melee range)
        { carrierX + 3.0f, carrierY, carrierZ, 0.0f },       // Right
        { carrierX - 3.0f, carrierY, carrierZ, 3.14f },      // Left
        { carrierX, carrierY + 3.0f, carrierZ, 4.71f },      // Behind
        { carrierX, carrierY - 3.0f, carrierZ, 1.57f },      // Front

        // Outer escort (ranged)
        { carrierX + 8.0f, carrierY + 5.0f, carrierZ, 5.50f },  // NE
        { carrierX - 8.0f, carrierY + 5.0f, carrierZ, 3.93f },  // NW
        { carrierX + 8.0f, carrierY - 5.0f, carrierZ, 0.79f },  // SE
        { carrierX - 8.0f, carrierY - 5.0f, carrierZ, 2.36f }   // SW
    };
}

// ============================================================================
// CHOKEPOINT POSITIONS
// ============================================================================

inline std::vector<Position> GetChokepoints()
{
    return {
        // Temple entrances
        { 1750.0f, 1220.0f, 20.0f, 0.0f },    // NE entrance ramp
        { 1750.0f, 1355.0f, 20.0f, 0.0f },    // NW entrance ramp
        { 1715.0f, 1220.0f, 20.0f, 0.0f },    // SE entrance ramp
        { 1715.0f, 1355.0f, 20.0f, 0.0f },    // SW entrance ramp

        // Center approaches
        { 1745.0f, 1287.0f, 16.0f, 3.14f },   // Center from East
        { 1720.0f, 1287.0f, 16.0f, 0.0f },    // Center from West
        { 1732.0f, 1310.0f, 16.0f, 4.71f },   // Center from North
        { 1732.0f, 1265.0f, 16.0f, 1.57f },   // Center from South

        // Bridge chokes (between orbs)
        { 1782.0f, 1287.0f, 28.0f, 3.14f },   // East bridge (Orange-Blue)
        { 1680.0f, 1287.0f, 28.0f, 0.0f }     // West bridge (Green-Purple)
    };
}

// ============================================================================
// SNIPER/OVERLOOK POSITIONS
// ============================================================================

inline std::vector<Position> GetSniperPositions()
{
    return {
        // Elevated temple platforms
        { 1784.0f, 1287.0f, 32.0f, 3.14f },   // East high platform
        { 1680.0f, 1287.0f, 32.0f, 0.0f },    // West high platform

        // Corner overlooks
        { 1780.0f, 1205.0f, 32.0f, 3.93f },   // NE overlook
        { 1780.0f, 1370.0f, 32.0f, 5.50f },   // NW overlook
        { 1685.0f, 1205.0f, 32.0f, 0.79f },   // SE overlook
        { 1685.0f, 1370.0f, 32.0f, 2.36f }    // SW overlook
    };
}

// ============================================================================
// BUFF POSITIONS
// ============================================================================

inline std::vector<Position> GetBuffPositions()
{
    return {
        // Power-ups near orb spawns
        { 1765.0f, 1210.0f, 27.0f, 0.0f },    // Near Orange
        { 1765.0f, 1365.0f, 27.0f, 0.0f },    // Near Blue
        { 1700.0f, 1210.0f, 27.0f, 0.0f },    // Near Green
        { 1700.0f, 1365.0f, 27.0f, 0.0f }     // Near Purple
    };
}

// ============================================================================
// AMBUSH POSITIONS
// ============================================================================

inline std::vector<Position> GetAmbushPositions(uint32 faction)
{
    if (faction == ALLIANCE)
    {
        return {
            // Intercept Horde going to Orange/Green
            { 1740.0f, 1230.0f, 18.0f, 3.14f },
            // Intercept at center from West
            { 1720.0f, 1287.0f, 15.0f, 0.0f },
            // Intercept at South
            { 1732.0f, 1250.0f, 14.0f, 4.71f }
        };
    }
    else
    {
        return {
            // Intercept Alliance going to Blue/Purple
            { 1725.0f, 1345.0f, 18.0f, 0.0f },
            // Intercept at center from East
            { 1745.0f, 1287.0f, 15.0f, 3.14f },
            // Intercept at North
            { 1732.0f, 1320.0f, 14.0f, 1.57f }
        };
    }
}

// ============================================================================
// DISTANCE MATRIX
// ============================================================================

// Approximate distances between orb spawns
inline float GetOrbDistance(uint32 fromOrb, uint32 toOrb)
{
    static const float distances[4][4] = {
        //         Orange  Blue    Green   Purple
        /* O */  { 0.0f,   175.0f, 105.0f, 195.0f },
        /* B */  { 175.0f, 0.0f,   195.0f, 105.0f },
        /* G */  { 105.0f, 195.0f, 0.0f,   175.0f },
        /* P */  { 195.0f, 105.0f, 175.0f, 0.0f }
    };

    if (fromOrb < 4 && toOrb < 4)
        return distances[fromOrb][toOrb];
    return 500.0f;  // Invalid
}

// Distance from orb to center
inline float GetOrbToCenterDistance(uint32 orbId)
{
    static const float distances[] = { 100.0f, 100.0f, 100.0f, 100.0f };  // All equidistant
    return orbId < 4 ? distances[orbId] : 500.0f;
}

// ============================================================================
// WORLD STATES
// ============================================================================

namespace WorldStates
{
    // Scores
    constexpr int32 SCORE_ALLY = 6303;
    constexpr int32 SCORE_HORDE = 6304;
    constexpr int32 MAX_SCORE = 6305;

    // Orb states
    constexpr int32 ORANGE_ORB_STATE = 6306;
    constexpr int32 BLUE_ORB_STATE = 6307;
    constexpr int32 GREEN_ORB_STATE = 6308;
    constexpr int32 PURPLE_ORB_STATE = 6309;

    // Orb holder faction
    constexpr int32 ORANGE_ORB_HOLDER = 6310;
    constexpr int32 BLUE_ORB_HOLDER = 6311;
    constexpr int32 GREEN_ORB_HOLDER = 6312;
    constexpr int32 PURPLE_ORB_HOLDER = 6313;
}

// ============================================================================
// GAME OBJECTS
// ============================================================================

namespace GameObjects
{
    // Orb objects - MUST match TrinityCore's battleground_temple_of_kotmogu.cpp
    constexpr uint32 BLUE_ORB = 212091;
    constexpr uint32 PURPLE_ORB = 212092;
    constexpr uint32 GREEN_ORB = 212093;
    constexpr uint32 ORANGE_ORB = 212094;

    // Doors
    constexpr uint32 ALLIANCE_DOOR = 212686;
    constexpr uint32 HORDE_DOOR = 212687;
}

// ============================================================================
// SPELLS
// ============================================================================

namespace Spells
{
    // Orb possession auras
    constexpr uint32 ORANGE_ORB_AURA = 121175;
    constexpr uint32 BLUE_ORB_AURA = 121176;
    constexpr uint32 GREEN_ORB_AURA = 121177;
    constexpr uint32 PURPLE_ORB_AURA = 121178;

    // Orb power stacking buff (increases damage taken)
    constexpr uint32 ORB_POWER_STACK = 121225;

    // Center zone buff
    constexpr uint32 CENTER_ZONE_AURA = 121219;
}

// ============================================================================
// STRATEGY CONSTANTS
// ============================================================================

namespace Strategy
{
    // Minimum escorts per orb carrier
    constexpr uint8 MIN_ESCORT_SIZE = 2;

    // Maximum escorts (don't over-commit)
    constexpr uint8 MAX_ESCORT_SIZE = 4;

    // Center push threshold (when to move to center)
    constexpr uint8 CENTER_PUSH_ORB_COUNT = 2;  // Push center when we have 2+ orbs

    // Hold time before center push (build up orbs first)
    constexpr uint32 INITIAL_HOLD_TIME = 30000;  // 30 seconds

    // Orb respawn time
    constexpr uint32 ORB_RESPAWN_TIME = 30000;

    // Time to consider center push
    constexpr uint32 CENTER_PUSH_INTERVAL = 20000;

    // Rotation interval for orb defense
    constexpr uint32 ROTATION_INTERVAL = 15000;

    // Score threshold for defensive play
    constexpr uint32 DEFENSIVE_THRESHOLD = 1200;  // 80% of max

    // Score threshold for desperation
    constexpr uint32 DESPERATION_THRESHOLD = 400;

    // Damage vulnerability per orb stack
    constexpr float DAMAGE_TAKEN_PER_STACK = 0.1f;  // +10% per 30 seconds

    // Opening phase duration
    constexpr uint32 OPENING_PHASE_DURATION = 60000;

    // Mid-game phase
    constexpr uint32 MID_GAME_START = 60000;
    constexpr uint32 MID_GAME_END = 1200000;

    // Late game
    constexpr uint32 LATE_GAME_START = 1200000;
}

} // namespace Playerbot::Coordination::Battleground::TempleOfKotmogu

#endif // PLAYERBOT_AI_COORDINATION_BG_DOMINATION_TEMPLEOFKOTMOGUDATA_H
