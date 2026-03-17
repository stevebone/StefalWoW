/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2021+ WarheadCore <https://github.com/AzerothCore/WarheadCore>
 * Copyright (C) 2025+ TrinityCore Playerbot Integration
 *
 * Enterprise-Grade Twin Peaks Data Header
 * ~550 lines - Complete positional data for CTF coordination
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_AI_COORDINATION_BG_CTF_TWINPEAKSDATA_H
#define PLAYERBOT_AI_COORDINATION_BG_CTF_TWINPEAKSDATA_H

#include "Define.h"
#include "Position.h"
#include <vector>
#include <array>

namespace Playerbot::Coordination::Battleground::TwinPeaks
{

// ============================================================================
// MAP INFORMATION
// ============================================================================

constexpr uint32 MAP_ID = 726;
constexpr char BG_NAME[] = "Twin Peaks";
constexpr uint32 MAX_SCORE = 3;
constexpr uint32 MAX_DURATION = 25 * 60 * 1000;  // 25 minutes
constexpr uint8 TEAM_SIZE = 10;

// ============================================================================
// STRATEGY CONSTANTS
// ============================================================================

namespace Strategy
{
    // Defense settings
    constexpr uint8 MIN_FLAG_DEFENDERS = 2;          // Minimum defenders at flag
    constexpr uint8 OPTIMAL_FLAG_DEFENDERS = 3;      // Optimal defense squad
    constexpr uint8 MAX_FLAG_DEFENDERS = 4;          // Maximum before overkill

    // Flag carrier escort
    constexpr uint8 MIN_FC_ESCORT = 2;               // Minimum escort for FC
    constexpr uint8 OPTIMAL_FC_ESCORT = 3;           // Optimal escort size
    constexpr uint8 MAX_FC_ESCORT = 5;               // Maximum before diminishing returns

    // Offense settings
    constexpr uint8 MIN_OFFENSE = 3;                 // Minimum offensive players
    constexpr uint8 OPTIMAL_OFFENSE = 5;             // Optimal offense for flag grab
    constexpr uint8 RETRIEVAL_TEAM_SIZE = 4;         // Team to retrieve dropped flag

    // Timing thresholds (milliseconds)
    constexpr uint32 OPENING_PHASE = 90000;          // First 90 seconds
    constexpr uint32 MID_GAME_THRESHOLD = 300000;    // After 5 minutes
    constexpr uint32 LATE_GAME_THRESHOLD = 900000;   // Last 10 minutes
    constexpr uint32 DESPERATE_SCORE_DIFF = 2;       // Score difference for desperate mode

    // Distance thresholds
    constexpr float ESCORT_FORMATION_RADIUS = 15.0f;  // Formation spread around FC
    constexpr float AMBUSH_TRIGGER_DISTANCE = 30.0f;  // Distance to trigger ambush
    constexpr float WATER_SPEED_PENALTY = 0.5f;       // Speed multiplier in water
    constexpr float BRIDGE_CHOKEPOINT_RADIUS = 20.0f; // Bridge engagement area

    // Buff priorities
    constexpr uint8 SPEED_BUFF_PRIORITY = 10;         // Highest for FC
    constexpr uint8 RESTORE_BUFF_PRIORITY = 7;        // For sustained fights
    constexpr uint8 BERSERK_BUFF_PRIORITY = 5;        // For DPS pushes
}

// ============================================================================
// FLAG POSITIONS
// ============================================================================

// Alliance flag position (Wildhammer Stronghold)
constexpr float ALLIANCE_FLAG_X = 2118.210f;
constexpr float ALLIANCE_FLAG_Y = 191.621f;
constexpr float ALLIANCE_FLAG_Z = 44.052f;
constexpr float ALLIANCE_FLAG_O = 5.741f;

// Horde flag position (Dragonmaw Fortress)
constexpr float HORDE_FLAG_X = 1578.339f;
constexpr float HORDE_FLAG_Y = 344.063f;
constexpr float HORDE_FLAG_Z = 2.419f;
constexpr float HORDE_FLAG_O = 5.235f;

inline Position GetAllianceFlagPos()
{
    return Position(ALLIANCE_FLAG_X, ALLIANCE_FLAG_Y, ALLIANCE_FLAG_Z, ALLIANCE_FLAG_O);
}

inline Position GetHordeFlagPos()
{
    return Position(HORDE_FLAG_X, HORDE_FLAG_Y, HORDE_FLAG_Z, HORDE_FLAG_O);
}

// ============================================================================
// SPAWN POSITIONS
// ============================================================================

// Alliance spawn points (Wildhammer side)
constexpr Position ALLIANCE_SPAWNS[] = {
    { 2152.63f, 158.58f, 43.32f, 5.235f },
    { 2140.63f, 158.58f, 43.32f, 5.235f },
    { 2146.63f, 152.58f, 43.32f, 5.235f },
    { 2146.63f, 164.58f, 43.32f, 5.235f },
    { 2152.63f, 164.58f, 43.32f, 5.235f }
};

// Horde spawn points (Dragonmaw side)
constexpr Position HORDE_SPAWNS[] = {
    { 1542.74f, 364.78f, -6.19f, 2.094f },
    { 1554.74f, 364.78f, -6.19f, 2.094f },
    { 1548.74f, 358.78f, -6.19f, 2.094f },
    { 1548.74f, 370.78f, -6.19f, 2.094f },
    { 1554.74f, 370.78f, -6.19f, 2.094f }
};

// ============================================================================
// GRAVEYARD POSITIONS
// ============================================================================

// Alliance graveyard (elevated position overlooking middle)
constexpr float ALLIANCE_GY_X = 2029.56f;
constexpr float ALLIANCE_GY_Y = 228.41f;
constexpr float ALLIANCE_GY_Z = 56.02f;
constexpr float ALLIANCE_GY_O = 5.235f;

// Horde graveyard (low position near fortress)
constexpr float HORDE_GY_X = 1632.29f;
constexpr float HORDE_GY_Y = 309.01f;
constexpr float HORDE_GY_Z = -6.72f;
constexpr float HORDE_GY_O = 2.094f;

inline Position GetAllianceGraveyard()
{
    return Position(ALLIANCE_GY_X, ALLIANCE_GY_Y, ALLIANCE_GY_Z, ALLIANCE_GY_O);
}

inline Position GetHordeGraveyard()
{
    return Position(HORDE_GY_X, HORDE_GY_Y, HORDE_GY_Z, HORDE_GY_O);
}

// ============================================================================
// FLAG ROOM DEFENSIVE POSITIONS (10 per base)
// ============================================================================

// Alliance flag room (Wildhammer Stronghold) - defensive positions
inline std::vector<Position> GetAllianceFlagRoomDefense()
{
    return {
        // Core flag positions
        { 2118.21f, 191.62f, 44.05f, 5.741f },  // Flag position (primary)
        { 2108.21f, 191.62f, 44.05f, 3.141f },  // In front of flag (secondary)
        { 2113.21f, 181.62f, 44.05f, 2.356f },  // Right flank
        { 2113.21f, 201.62f, 44.05f, 3.927f },  // Left flank
        { 2098.21f, 191.62f, 44.05f, 3.141f },  // Back guard

        // Upper level positions (elevated advantage)
        { 2118.21f, 185.62f, 51.05f, 5.741f },  // Upper balcony center
        { 2108.21f, 185.62f, 51.05f, 3.141f },  // Upper front
        { 2118.21f, 197.62f, 51.05f, 4.712f },  // Upper balcony side

        // Entrance defense (chokepoint control)
        { 2088.21f, 191.62f, 44.05f, 3.141f },  // Main entrance
        { 2088.21f, 181.62f, 44.05f, 2.356f }   // Right entrance angle
    };
}

// Horde flag room (Dragonmaw Fortress) - defensive positions
inline std::vector<Position> GetHordeFlagRoomDefense()
{
    return {
        // Core flag positions
        { 1578.34f, 344.06f, 2.42f, 5.235f },   // Flag position (primary)
        { 1588.34f, 344.06f, 2.42f, 0.000f },   // In front of flag (secondary)
        { 1583.34f, 334.06f, 2.42f, 5.498f },   // Right flank
        { 1583.34f, 354.06f, 2.42f, 0.785f },   // Left flank
        { 1598.34f, 344.06f, 2.42f, 0.000f },   // Back guard

        // Upper level positions (elevated advantage)
        { 1578.34f, 338.06f, 9.42f, 5.235f },   // Upper balcony center
        { 1588.34f, 338.06f, 9.42f, 0.000f },   // Upper front
        { 1578.34f, 350.06f, 9.42f, 0.785f },   // Upper balcony side

        // Entrance defense (chokepoint control)
        { 1608.34f, 344.06f, 2.42f, 0.000f },   // Main entrance
        { 1608.34f, 334.06f, 2.42f, 5.498f }    // Right entrance angle
    };
}

// ============================================================================
// MIDDLE MAP CHOKEPOINTS (15 positions)
// ============================================================================

inline std::vector<Position> GetMiddleChokepoints()
{
    return {
        // Main center (bridge crossing - CRITICAL)
        { 1848.38f, 268.38f, 8.91f, 0.0f },     // Dead center (main bridge)
        { 1848.38f, 238.38f, 8.91f, 0.0f },     // South of bridge
        { 1848.38f, 298.38f, 8.91f, 0.0f },     // North of bridge
        { 1838.38f, 268.38f, 8.91f, 0.0f },     // Bridge west approach
        { 1858.38f, 268.38f, 8.91f, 0.0f },     // Bridge east approach

        // Alliance side approach (elevated)
        { 1948.38f, 238.38f, 25.91f, 5.5f },    // Alliance hill
        { 1978.38f, 218.38f, 35.91f, 5.5f },    // Alliance high ground
        { 1918.38f, 258.38f, 18.91f, 5.5f },    // Alliance bridge approach

        // Horde side approach (lower terrain)
        { 1748.38f, 298.38f, 0.0f, 2.3f },      // Horde approach
        { 1698.38f, 328.38f, -3.0f, 2.3f },     // Horde low ground
        { 1778.38f, 278.38f, 4.0f, 2.3f },      // Horde bridge approach

        // Flanking routes
        { 1848.38f, 178.38f, 0.0f, 0.0f },      // South flank (waterfall route)
        { 1848.38f, 358.38f, 0.0f, 0.0f },      // North flank
        { 1900.38f, 318.38f, 12.0f, 4.5f },     // Northeast cliff path
        { 1798.38f, 218.38f, 2.0f, 1.5f }       // Southwest river path
    };
}

// ============================================================================
// SNIPER/OVERLOOK POSITIONS (8 elevated advantage spots)
// ============================================================================

inline std::vector<Position> GetSniperPositions()
{
    return {
        // Alliance high ground (Wildhammer cliffs)
        { 2030.21f, 178.62f, 56.05f, 5.498f },  // Cliffside overlook (best LoS)
        { 2058.21f, 205.62f, 52.05f, 5.235f },  // Alliance GY overlook
        { 1998.21f, 188.62f, 48.05f, 5.741f },  // Mid-cliff sniper spot
        { 2118.21f, 185.62f, 51.05f, 5.741f },  // Flag room balcony

        // Horde high ground (Dragonmaw fortress)
        { 1578.34f, 338.06f, 9.42f, 5.235f },   // Fortress balcony
        { 1628.34f, 358.06f, 4.42f, 0.785f },   // Fortress outer wall

        // Middle map elevated spots
        { 1848.38f, 268.38f, 18.91f, 0.0f },    // Bridge overlook
        { 1900.38f, 288.38f, 22.0f, 4.0f }      // Cliff edge mid-map
    };
}

// ============================================================================
// AMBUSH POSITIONS (faction-specific interception points)
// ============================================================================

inline std::vector<Position> GetAllianceAmbushPositions()
{
    return {
        // Bridge ambush spots (intercept Horde FC)
        { 1868.38f, 258.38f, 10.91f, 2.5f },    // East bridge entrance
        { 1878.38f, 278.38f, 12.91f, 2.8f },    // Northeast bridge approach
        { 1858.38f, 238.38f, 9.91f, 2.0f },     // Southeast bridge flank

        // River crossing intercepts
        { 1898.38f, 228.38f, 15.91f, 5.5f },    // River south ambush
        { 1918.38f, 308.38f, 18.91f, 4.0f },    // River north ambush

        // Cliff path intercepts
        { 1958.38f, 248.38f, 28.91f, 5.235f },  // Alliance cliff path
        { 1988.38f, 198.38f, 38.91f, 5.5f }     // High ground intercept
    };
}

inline std::vector<Position> GetHordeAmbushPositions()
{
    return {
        // Bridge ambush spots (intercept Alliance FC)
        { 1828.38f, 278.38f, 8.91f, 0.5f },     // West bridge entrance
        { 1818.38f, 258.38f, 6.91f, 0.2f },     // Southwest bridge approach
        { 1838.38f, 298.38f, 9.91f, 0.8f },     // Northwest bridge flank

        // River crossing intercepts
        { 1798.38f, 308.38f, 2.0f, 2.3f },      // River north ambush
        { 1778.38f, 238.38f, 0.0f, 1.5f },      // River south ambush

        // Low ground intercepts
        { 1718.38f, 318.38f, -2.0f, 2.094f },   // Horde approach path
        { 1658.38f, 338.38f, -5.0f, 2.5f }      // Near Horde base
    };
}

// ============================================================================
// FC ESCORT FORMATION POSITIONS
// ============================================================================

// Formation positions relative to FC (offset from FC position)
inline std::vector<Position> GetFCEscortFormation()
{
    return {
        // Front guards
        { 8.0f, 0.0f, 0.0f, 0.0f },             // Point man (front center)
        { 6.0f, -5.0f, 0.0f, 0.3f },            // Front right
        { 6.0f, 5.0f, 0.0f, -0.3f },            // Front left

        // Side guards
        { 0.0f, -8.0f, 0.0f, 1.57f },           // Right flank
        { 0.0f, 8.0f, 0.0f, -1.57f },           // Left flank

        // Rear guard
        { -6.0f, 0.0f, 0.0f, 3.14f },           // Rear center
        { -5.0f, -4.0f, 0.0f, 2.8f },           // Rear right
        { -5.0f, 4.0f, 0.0f, -2.8f }            // Rear left
    };
}

// ============================================================================
// BUFF POSITIONS
// ============================================================================

// Speed buff locations (critical for FC)
inline std::vector<Position> GetSpeedBuffPositions()
{
    return {
        { 2039.54f, 208.87f, 46.14f, 0.0f },    // Alliance side speed buff
        { 1660.54f, 333.87f, -6.78f, 0.0f }     // Horde side speed buff
    };
}

// Restoration buff locations (heal/mana recovery)
inline std::vector<Position> GetRestoreBuffPositions()
{
    return {
        { 1996.24f, 136.05f, 32.92f, 0.0f },    // Alliance side restore
        { 1702.65f, 384.02f, -6.58f, 0.0f }     // Horde side restore
    };
}

// Berserk buff locations (damage boost)
inline std::vector<Position> GetBerserkBuffPositions()
{
    return {
        { 1972.42f, 309.82f, 14.90f, 0.0f },    // Alliance side berserk
        { 1731.24f, 198.65f, 2.45f, 0.0f }      // Horde side berserk
    };
}

// ============================================================================
// WATER/TERRAIN FEATURES
// ============================================================================

// River zone definition (for speed calculations)
namespace RiverZone
{
    constexpr float MIN_X = 1750.0f;
    constexpr float MAX_X = 1950.0f;
    constexpr float MIN_Y = 200.0f;
    constexpr float MAX_Y = 340.0f;
    constexpr float MAX_Z = 5.0f;      // Water level
}

// River crossing points (critical chokepoints)
inline std::vector<Position> GetRiverCrossingPoints()
{
    return {
        { 1848.38f, 268.38f, 8.91f, 0.0f },     // Main bridge (safest)
        { 1798.38f, 238.38f, -2.0f, 0.0f },     // South ford (slow)
        { 1898.38f, 298.38f, -2.0f, 0.0f },     // North ford (slow)
        { 1848.38f, 178.38f, -1.0f, 0.0f }      // Waterfall route (scenic but slow)
    };
}

// Water detection helper
inline bool IsInWaterZone(float x, float y, float z)
{
    return x > RiverZone::MIN_X && x < RiverZone::MAX_X &&
           y > RiverZone::MIN_Y && y < RiverZone::MAX_Y &&
           z < RiverZone::MAX_Z;
}

// ============================================================================
// FC KITING PATHS (Multiple route options)
// ============================================================================

// Alliance FC kiting path - DIRECT ROUTE (fastest but most contested)
inline std::vector<Position> GetAllianceFCKitePathDirect()
{
    return {
        { HORDE_FLAG_X, HORDE_FLAG_Y, HORDE_FLAG_Z, HORDE_FLAG_O },
        { 1660.54f, 333.87f, -6.78f, 0.0f },   // Horde speed buff
        { 1748.38f, 298.38f, 0.0f, 0.0f },     // Horde approach
        { 1848.38f, 268.38f, 8.91f, 0.0f },    // Bridge (main chokepoint)
        { 1948.38f, 238.38f, 25.91f, 5.5f },   // Alliance hill
        { 2039.54f, 208.87f, 46.14f, 0.0f },   // Alliance speed buff
        { ALLIANCE_FLAG_X, ALLIANCE_FLAG_Y, ALLIANCE_FLAG_Z, ALLIANCE_FLAG_O }
    };
}

// Alliance FC kiting path - NORTH ROUTE (avoids main bridge)
inline std::vector<Position> GetAllianceFCKitePathNorth()
{
    return {
        { HORDE_FLAG_X, HORDE_FLAG_Y, HORDE_FLAG_Z, HORDE_FLAG_O },
        { 1608.34f, 354.06f, 2.42f, 0.785f },  // Horde fortress exit (north)
        { 1698.38f, 358.38f, -3.0f, 0.8f },    // North path
        { 1798.38f, 338.38f, 2.0f, 0.5f },     // North river approach
        { 1898.38f, 298.38f, 12.0f, 5.5f },    // Northeast cliff
        { 1958.38f, 268.38f, 28.91f, 5.5f },   // Alliance northeast approach
        { 2039.54f, 208.87f, 46.14f, 0.0f },   // Alliance speed buff
        { ALLIANCE_FLAG_X, ALLIANCE_FLAG_Y, ALLIANCE_FLAG_Z, ALLIANCE_FLAG_O }
    };
}

// Alliance FC kiting path - SOUTH ROUTE (waterfall path)
inline std::vector<Position> GetAllianceFCKitePathSouth()
{
    return {
        { HORDE_FLAG_X, HORDE_FLAG_Y, HORDE_FLAG_Z, HORDE_FLAG_O },
        { 1608.34f, 334.06f, 2.42f, 5.498f },  // Horde fortress exit (south)
        { 1698.38f, 288.38f, -3.0f, 5.5f },    // South path
        { 1778.38f, 228.38f, 0.0f, 5.2f },     // South river approach
        { 1848.38f, 178.38f, 0.0f, 0.0f },     // Waterfall crossing
        { 1948.38f, 198.38f, 25.91f, 5.5f },   // Alliance south approach
        { 1996.24f, 136.05f, 32.92f, 5.5f },   // Alliance restore buff (optional heal)
        { ALLIANCE_FLAG_X, ALLIANCE_FLAG_Y, ALLIANCE_FLAG_Z, ALLIANCE_FLAG_O }
    };
}

// Horde FC kiting path - DIRECT ROUTE
inline std::vector<Position> GetHordeFCKitePathDirect()
{
    return {
        { ALLIANCE_FLAG_X, ALLIANCE_FLAG_Y, ALLIANCE_FLAG_Z, ALLIANCE_FLAG_O },
        { 2039.54f, 208.87f, 46.14f, 0.0f },   // Alliance speed buff
        { 1948.38f, 238.38f, 25.91f, 0.0f },   // Alliance hill
        { 1848.38f, 268.38f, 8.91f, 0.0f },    // Bridge
        { 1748.38f, 298.38f, 0.0f, 0.0f },     // Horde approach
        { 1660.54f, 333.87f, -6.78f, 0.0f },   // Horde speed buff
        { HORDE_FLAG_X, HORDE_FLAG_Y, HORDE_FLAG_Z, HORDE_FLAG_O }
    };
}

// Horde FC kiting path - NORTH ROUTE
inline std::vector<Position> GetHordeFCKitePathNorth()
{
    return {
        { ALLIANCE_FLAG_X, ALLIANCE_FLAG_Y, ALLIANCE_FLAG_Z, ALLIANCE_FLAG_O },
        { 2088.21f, 201.62f, 44.05f, 3.927f }, // Alliance base exit (north)
        { 1998.38f, 268.38f, 38.91f, 2.5f },   // Alliance northeast
        { 1918.38f, 308.38f, 18.91f, 2.3f },   // North path
        { 1848.38f, 358.38f, 0.0f, 2.0f },     // North crossing
        { 1748.38f, 338.38f, -2.0f, 2.3f },    // Horde north approach
        { 1702.65f, 384.02f, -6.58f, 2.5f },   // Horde restore buff (optional heal)
        { HORDE_FLAG_X, HORDE_FLAG_Y, HORDE_FLAG_Z, HORDE_FLAG_O }
    };
}

// Horde FC kiting path - SOUTH ROUTE
inline std::vector<Position> GetHordeFCKitePathSouth()
{
    return {
        { ALLIANCE_FLAG_X, ALLIANCE_FLAG_Y, ALLIANCE_FLAG_Z, ALLIANCE_FLAG_O },
        { 2088.21f, 181.62f, 44.05f, 2.356f }, // Alliance base exit (south)
        { 1996.24f, 136.05f, 32.92f, 2.0f },   // Alliance restore buff area
        { 1918.38f, 188.38f, 18.91f, 2.5f },   // South path
        { 1848.38f, 178.38f, 0.0f, 3.14f },    // Waterfall crossing
        { 1778.38f, 238.38f, 0.0f, 2.5f },     // Horde south approach
        { 1731.24f, 198.65f, 2.45f, 2.3f },    // Horde berserk buff
        { HORDE_FLAG_X, HORDE_FLAG_Y, HORDE_FLAG_Z, HORDE_FLAG_O }
    };
}

// Legacy compatibility - default direct routes
inline std::vector<Position> GetAllianceFCKitePath()
{
    return GetAllianceFCKitePathDirect();
}

inline std::vector<Position> GetHordeFCKitePath()
{
    return GetHordeFCKitePathDirect();
}

// ============================================================================
// DISTANCE MATRIX (Pre-calculated approximate distances)
// ============================================================================

namespace Distances
{
    // Key location indices
    constexpr uint8 LOC_ALLIANCE_FLAG = 0;
    constexpr uint8 LOC_HORDE_FLAG = 1;
    constexpr uint8 LOC_ALLIANCE_GY = 2;
    constexpr uint8 LOC_HORDE_GY = 3;
    constexpr uint8 LOC_BRIDGE = 4;
    constexpr uint8 LOC_ALLIANCE_SPEED = 5;
    constexpr uint8 LOC_HORDE_SPEED = 6;
    constexpr uint8 LOC_COUNT = 7;

    // Distance matrix (symmetric, approximate in yards)
    constexpr std::array<std::array<float, LOC_COUNT>, LOC_COUNT> MATRIX = {{
        //  A_FLAG  H_FLAG  A_GY    H_GY    BRIDGE  A_SPEED H_SPEED
        {   0.0f,   550.0f, 95.0f,  475.0f, 280.0f, 85.0f,  465.0f  },  // A_FLAG
        {   550.0f, 0.0f,   455.0f, 75.0f,  270.0f, 465.0f, 85.0f   },  // H_FLAG
        {   95.0f,  455.0f, 0.0f,   400.0f, 185.0f, 55.0f,  380.0f  },  // A_GY
        {   475.0f, 75.0f,  400.0f, 0.0f,   205.0f, 380.0f, 35.0f   },  // H_GY
        {   280.0f, 270.0f, 185.0f, 205.0f, 0.0f,   195.0f, 190.0f  },  // BRIDGE
        {   85.0f,  465.0f, 55.0f,  380.0f, 195.0f, 0.0f,   385.0f  },  // A_SPEED
        {   465.0f, 85.0f,  380.0f, 35.0f,  190.0f, 385.0f, 0.0f    }   // H_SPEED
    }};

    // Flag to flag total distance
    constexpr float FLAG_TO_FLAG = 550.0f;

    // Average run time (seconds) at normal speed
    constexpr float AVG_FLAG_RUN_TIME = 45.0f;

    // With speed buff bonus
    constexpr float SPEED_BUFF_RUN_TIME = 32.0f;
}

// ============================================================================
// WORLD STATES
// ============================================================================

namespace WorldStates
{
    // Flag states
    constexpr int32 ALLIANCE_FLAG_STATE = 5546;
    constexpr int32 HORDE_FLAG_STATE = 5547;

    // Score
    constexpr int32 ALLIANCE_FLAG_CAPTURES = 5548;
    constexpr int32 HORDE_FLAG_CAPTURES = 5549;

    // Timer
    constexpr int32 TIME_REMAINING = 5550;

    // State values
    constexpr int32 FLAG_STATE_AT_BASE = 1;
    constexpr int32 FLAG_STATE_PICKED_UP = 2;
    constexpr int32 FLAG_STATE_DROPPED = 3;
}

// ============================================================================
// GAME OBJECTS
// ============================================================================

namespace GameObjects
{
    // Flags
    constexpr uint32 ALLIANCE_FLAG = 179830;  // Same model as WSG
    constexpr uint32 HORDE_FLAG = 179831;

    // Doors
    constexpr uint32 ALLIANCE_DOOR = 206655;
    constexpr uint32 HORDE_DOOR = 206656;

    // Buffs (powerups)
    constexpr uint32 SPEED_BUFF = 179871;
    constexpr uint32 RESTORE_BUFF = 179904;
    constexpr uint32 BERSERK_BUFF = 179905;
}

// ============================================================================
// OBJECTIVE IDS
// ============================================================================

namespace ObjectiveIds
{
    constexpr uint32 ALLIANCE_FLAG = 1;
    constexpr uint32 HORDE_FLAG = 2;
}

// ============================================================================
// TERRAIN FEATURES
// ============================================================================

namespace Terrain
{
    // Bridge bounds (main chokepoint)
    constexpr float BRIDGE_MIN_X = 1838.0f;
    constexpr float BRIDGE_MAX_X = 1858.0f;
    constexpr float BRIDGE_MIN_Y = 258.0f;
    constexpr float BRIDGE_MAX_Y = 278.0f;
    constexpr float BRIDGE_Z = 8.91f;

    // Alliance stronghold bounds
    constexpr float ALLIANCE_BASE_MIN_X = 2078.0f;
    constexpr float ALLIANCE_BASE_MAX_X = 2158.0f;
    constexpr float ALLIANCE_BASE_MIN_Y = 148.0f;
    constexpr float ALLIANCE_BASE_MAX_Y = 208.0f;

    // Horde fortress bounds
    constexpr float HORDE_BASE_MIN_X = 1538.0f;
    constexpr float HORDE_BASE_MAX_X = 1618.0f;
    constexpr float HORDE_BASE_MIN_Y = 324.0f;
    constexpr float HORDE_BASE_MAX_Y = 384.0f;
}

// Position helpers
inline bool IsOnBridge(float x, float y)
{
    return x >= Terrain::BRIDGE_MIN_X && x <= Terrain::BRIDGE_MAX_X &&
           y >= Terrain::BRIDGE_MIN_Y && y <= Terrain::BRIDGE_MAX_Y;
}

inline bool IsInAllianceBase(float x, float y)
{
    return x >= Terrain::ALLIANCE_BASE_MIN_X && x <= Terrain::ALLIANCE_BASE_MAX_X &&
           y >= Terrain::ALLIANCE_BASE_MIN_Y && y <= Terrain::ALLIANCE_BASE_MAX_Y;
}

inline bool IsInHordeBase(float x, float y)
{
    return x >= Terrain::HORDE_BASE_MIN_X && x <= Terrain::HORDE_BASE_MAX_X &&
           y >= Terrain::HORDE_BASE_MIN_Y && y <= Terrain::HORDE_BASE_MAX_Y;
}

} // namespace Playerbot::Coordination::Battleground::TwinPeaks

#endif // PLAYERBOT_AI_COORDINATION_BG_CTF_TWINPEAKSDATA_H
