/*
 * Copyright (C) 2025+ TrinityCore Playerbot Integration
 *
 * Enterprise-Grade Deepwind Gorge Data
 * ~650 lines - Complete strategic positioning data for hybrid node+cart BG
 */

#ifndef PLAYERBOT_AI_COORDINATION_BG_DOMINATION_DEEPWINDGORGEDATA_H
#define PLAYERBOT_AI_COORDINATION_BG_DOMINATION_DEEPWINDGORGEDATA_H

#include "Define.h"
#include "Position.h"
#include <array>
#include <vector>
#include <cmath>

namespace Playerbot::Coordination::Battleground::DeepwindGorge
{

// ============================================================================
// BASIC CONFIGURATION
// ============================================================================

constexpr uint32 MAP_ID = 1105;
constexpr char BG_NAME[] = "Deepwind Gorge";
constexpr uint32 MAX_SCORE = 1500;  // Resources to win
constexpr uint32 MAX_DURATION = 25 * 60 * 1000;  // 25 minutes
constexpr uint8 TEAM_SIZE = 10;
constexpr uint32 NODE_COUNT = 3;   // 3 mines (nodes)
constexpr uint32 CART_COUNT = 3;   // 3 mine carts
constexpr uint32 TICK_INTERVAL = 2000;
constexpr uint32 CAPTURE_TIME = 8000;  // 8 seconds to capture node
constexpr uint32 CART_CAPTURE_POINTS = 200;  // Points per cart captured

// ============================================================================
// NODE ENUMERATION
// ============================================================================

namespace Nodes
{
    constexpr uint32 PANDAREN_MINE = 0;   // North (neutral)
    constexpr uint32 GOBLIN_MINE = 1;     // West (Alliance side)
    constexpr uint32 CENTER_MINE = 2;     // East (Horde side)
}

// ============================================================================
// NODE POSITIONS
// ============================================================================

constexpr float PANDAREN_MINE_X = 1600.53f;
constexpr float PANDAREN_MINE_Y = 945.24f;
constexpr float PANDAREN_MINE_Z = 20.0f;
constexpr float PANDAREN_MINE_O = 0.0f;

constexpr float GOBLIN_MINE_X = 1447.27f;
constexpr float GOBLIN_MINE_Y = 1110.36f;
constexpr float GOBLIN_MINE_Z = 15.0f;
constexpr float GOBLIN_MINE_O = 2.36f;

constexpr float CENTER_MINE_X = 1753.79f;
constexpr float CENTER_MINE_Y = 780.12f;
constexpr float CENTER_MINE_Z = 18.0f;
constexpr float CENTER_MINE_O = 5.50f;

inline Position GetNodePosition(uint32 nodeId)
{
    switch (nodeId)
    {
        case Nodes::PANDAREN_MINE:
            return Position(PANDAREN_MINE_X, PANDAREN_MINE_Y, PANDAREN_MINE_Z, PANDAREN_MINE_O);
        case Nodes::GOBLIN_MINE:
            return Position(GOBLIN_MINE_X, GOBLIN_MINE_Y, GOBLIN_MINE_Z, GOBLIN_MINE_O);
        case Nodes::CENTER_MINE:
            return Position(CENTER_MINE_X, CENTER_MINE_Y, CENTER_MINE_Z, CENTER_MINE_O);
        default:
            return Position(0, 0, 0, 0);
    }
}

inline const char* GetNodeName(uint32 nodeId)
{
    switch (nodeId)
    {
        case Nodes::PANDAREN_MINE: return "Pandaren Mine";
        case Nodes::GOBLIN_MINE: return "Goblin Mine";
        case Nodes::CENTER_MINE: return "Center Mine";
        default: return "Unknown";
    }
}

// ============================================================================
// NODE DEFENSE POSITIONS (10 per node = 30 total)
// ============================================================================

constexpr float NODE_DEFENSE_POSITIONS[][10][4] = {
    // Pandaren Mine (Node 0) - North mine, most contested
    {
        { 1590.53f, 955.24f, 20.0f, 0.0f },    // North entrance
        { 1610.53f, 955.24f, 20.0f, 3.14f },   // North entrance east
        { 1595.53f, 935.24f, 21.0f, 4.71f },   // South side
        { 1605.53f, 935.24f, 21.0f, 4.71f },   // South side east
        { 1585.53f, 945.24f, 20.0f, 1.57f },   // West flank
        { 1615.53f, 945.24f, 20.0f, 4.71f },   // East flank
        { 1600.53f, 960.24f, 19.0f, 0.79f },   // Far north
        { 1600.53f, 930.24f, 22.0f, 3.93f },   // Far south
        { 1588.53f, 952.24f, 20.0f, 0.39f },   // NW corner
        { 1612.53f, 952.24f, 20.0f, 2.75f }    // NE corner
    },
    // Goblin Mine (Node 1) - Alliance side
    {
        { 1437.27f, 1120.36f, 15.0f, 2.36f },  // North entrance
        { 1457.27f, 1120.36f, 15.0f, 2.36f },  // North entrance east
        { 1442.27f, 1100.36f, 16.0f, 4.71f },  // South side
        { 1452.27f, 1100.36f, 16.0f, 4.71f },  // South side east
        { 1432.27f, 1110.36f, 15.0f, 1.57f },  // West flank
        { 1462.27f, 1110.36f, 15.0f, 4.71f },  // East flank
        { 1447.27f, 1125.36f, 14.0f, 0.79f },  // Far north
        { 1447.27f, 1095.36f, 17.0f, 3.93f },  // Far south
        { 1435.27f, 1117.36f, 15.0f, 1.18f },  // NW corner
        { 1459.27f, 1117.36f, 15.0f, 1.96f }   // NE corner
    },
    // Center Mine (Node 2) - Horde side
    {
        { 1743.79f, 790.12f, 18.0f, 5.50f },   // North entrance
        { 1763.79f, 790.12f, 18.0f, 5.50f },   // North entrance east
        { 1748.79f, 770.12f, 19.0f, 4.71f },   // South side
        { 1758.79f, 770.12f, 19.0f, 4.71f },   // South side east
        { 1738.79f, 780.12f, 18.0f, 1.57f },   // West flank
        { 1768.79f, 780.12f, 18.0f, 4.71f },   // East flank
        { 1753.79f, 795.12f, 17.0f, 0.79f },   // Far north
        { 1753.79f, 765.12f, 20.0f, 3.93f },   // Far south
        { 1741.79f, 787.12f, 18.0f, 0.39f },   // NW corner
        { 1765.79f, 787.12f, 18.0f, 2.75f }    // NE corner
    }
};

inline std::vector<Position> GetNodeDefensePositions(uint32 nodeId)
{
    std::vector<Position> positions;
    if (nodeId < NODE_COUNT)
    {
        for (uint8 i = 0; i < 10; ++i)
        {
            positions.emplace_back(
                NODE_DEFENSE_POSITIONS[nodeId][i][0],
                NODE_DEFENSE_POSITIONS[nodeId][i][1],
                NODE_DEFENSE_POSITIONS[nodeId][i][2],
                NODE_DEFENSE_POSITIONS[nodeId][i][3]
            );
        }
    }
    return positions;
}

// ============================================================================
// CART ENUMERATION AND TRACKS
// ============================================================================

namespace Carts
{
    constexpr uint32 CART_NORTH = 0;
    constexpr uint32 CART_CENTER = 1;
    constexpr uint32 CART_SOUTH = 2;
}

// Cart spawn positions (on tracks)
constexpr float CART_SPAWN_POSITIONS[][4] = {
    { 1550.0f, 1010.0f, 18.0f, 3.14f },   // Cart North
    { 1600.0f, 930.0f, 20.0f, 3.14f },    // Cart Center
    { 1650.0f, 850.0f, 19.0f, 3.14f }     // Cart South
};

inline Position GetCartSpawnPosition(uint32 cartId)
{
    if (cartId < CART_COUNT)
        return Position(CART_SPAWN_POSITIONS[cartId][0],
                       CART_SPAWN_POSITIONS[cartId][1],
                       CART_SPAWN_POSITIONS[cartId][2],
                       CART_SPAWN_POSITIONS[cartId][3]);
    return Position(0, 0, 0, 0);
}

// ============================================================================
// CART DEPOT POSITIONS
// ============================================================================

constexpr float ALLIANCE_CART_DEPOT_X = 1350.0f;
constexpr float ALLIANCE_CART_DEPOT_Y = 1050.0f;
constexpr float ALLIANCE_CART_DEPOT_Z = 10.0f;
constexpr float ALLIANCE_CART_DEPOT_O = 2.36f;

constexpr float HORDE_CART_DEPOT_X = 1850.0f;
constexpr float HORDE_CART_DEPOT_Y = 850.0f;
constexpr float HORDE_CART_DEPOT_Z = 12.0f;
constexpr float HORDE_CART_DEPOT_O = 5.50f;

inline Position GetCartDepotPosition(uint32 faction)
{
    if (faction == 1)  // ALLIANCE
        return Position(ALLIANCE_CART_DEPOT_X, ALLIANCE_CART_DEPOT_Y, ALLIANCE_CART_DEPOT_Z, ALLIANCE_CART_DEPOT_O);
    else  // HORDE
        return Position(HORDE_CART_DEPOT_X, HORDE_CART_DEPOT_Y, HORDE_CART_DEPOT_Z, HORDE_CART_DEPOT_O);
}

// ============================================================================
// CART DEPOT DEFENSE POSITIONS (8 per depot)
// ============================================================================

constexpr float ALLIANCE_DEPOT_DEFENSE[][4] = {
    { 1340.0f, 1060.0f, 10.0f, 2.36f },   // North guard
    { 1360.0f, 1060.0f, 10.0f, 2.36f },   // North guard east
    { 1345.0f, 1040.0f, 11.0f, 3.93f },   // South entrance
    { 1355.0f, 1040.0f, 11.0f, 3.93f },   // South entrance east
    { 1335.0f, 1050.0f, 10.0f, 1.57f },   // West flank
    { 1365.0f, 1050.0f, 10.0f, 4.71f },   // East flank
    { 1350.0f, 1065.0f, 9.0f, 0.79f },    // Far north
    { 1350.0f, 1035.0f, 12.0f, 5.50f }    // Far south
};

constexpr float HORDE_DEPOT_DEFENSE[][4] = {
    { 1840.0f, 860.0f, 12.0f, 5.50f },    // North guard
    { 1860.0f, 860.0f, 12.0f, 5.50f },    // North guard east
    { 1845.0f, 840.0f, 13.0f, 3.93f },    // South entrance
    { 1855.0f, 840.0f, 13.0f, 3.93f },    // South entrance east
    { 1835.0f, 850.0f, 12.0f, 1.57f },    // West flank
    { 1865.0f, 850.0f, 12.0f, 4.71f },    // East flank
    { 1850.0f, 865.0f, 11.0f, 0.79f },    // Far north
    { 1850.0f, 835.0f, 14.0f, 5.50f }     // Far south
};

inline std::vector<Position> GetDepotDefensePositions(uint32 faction)
{
    std::vector<Position> positions;
    const auto& data = (faction == 1) ? ALLIANCE_DEPOT_DEFENSE : HORDE_DEPOT_DEFENSE;
    for (uint8 i = 0; i < 8; ++i)
    {
        positions.emplace_back(data[i][0], data[i][1], data[i][2], data[i][3]);
    }
    return positions;
}

// ============================================================================
// CART TRACK WAYPOINTS (route from center to each depot)
// ============================================================================

constexpr uint32 MAX_TRACK_WAYPOINTS = 8;

// Track waypoints for each cart going to Alliance depot
constexpr float CART_TO_ALLIANCE_WAYPOINTS[][3][4] = {
    // Cart North to Alliance
    {
        { 1550.0f, 1010.0f, 18.0f, 3.14f },    // Start
        { 1480.0f, 1030.0f, 14.0f, 2.75f },    // Mid
        { 1350.0f, 1050.0f, 10.0f, 2.36f }     // Depot
    },
    // Cart Center to Alliance
    {
        { 1600.0f, 930.0f, 20.0f, 3.14f },     // Start
        { 1480.0f, 990.0f, 15.0f, 2.36f },     // Mid
        { 1350.0f, 1050.0f, 10.0f, 2.36f }     // Depot
    },
    // Cart South to Alliance
    {
        { 1650.0f, 850.0f, 19.0f, 3.14f },     // Start
        { 1500.0f, 950.0f, 16.0f, 2.36f },     // Mid
        { 1350.0f, 1050.0f, 10.0f, 2.36f }     // Depot
    }
};

// Track waypoints for each cart going to Horde depot
constexpr float CART_TO_HORDE_WAYPOINTS[][3][4] = {
    // Cart North to Horde
    {
        { 1550.0f, 1010.0f, 18.0f, 5.50f },    // Start
        { 1700.0f, 930.0f, 16.0f, 5.89f },     // Mid
        { 1850.0f, 850.0f, 12.0f, 5.50f }      // Depot
    },
    // Cart Center to Horde
    {
        { 1600.0f, 930.0f, 20.0f, 5.50f },     // Start
        { 1725.0f, 890.0f, 15.0f, 5.50f },     // Mid
        { 1850.0f, 850.0f, 12.0f, 5.50f }      // Depot
    },
    // Cart South to Horde
    {
        { 1650.0f, 850.0f, 19.0f, 5.50f },     // Start
        { 1750.0f, 850.0f, 14.0f, 0.0f },      // Mid
        { 1850.0f, 850.0f, 12.0f, 5.50f }      // Depot
    }
};

inline std::vector<Position> GetCartTrackToDepot(uint32 cartId, uint32 faction)
{
    std::vector<Position> waypoints;
    if (cartId >= CART_COUNT)
        return waypoints;

    const auto& track = (faction == 1) ? CART_TO_ALLIANCE_WAYPOINTS[cartId] : CART_TO_HORDE_WAYPOINTS[cartId];
    for (uint8 i = 0; i < 3; ++i)
    {
        waypoints.emplace_back(track[i][0], track[i][1], track[i][2], track[i][3]);
    }
    return waypoints;
}

// ============================================================================
// CART INTERCEPTION POSITIONS (8 total along tracks)
// ============================================================================

namespace CartInterception
{
    constexpr uint32 COUNT = 8;
}

constexpr float CART_INTERCEPTION_POSITIONS[][4] = {
    { 1480.0f, 1010.0f, 15.0f, 2.75f },   // Alliance track north
    { 1450.0f, 1030.0f, 13.0f, 2.36f },   // Alliance track mid-north
    { 1420.0f, 1040.0f, 11.0f, 2.36f },   // Alliance track approach
    { 1520.0f, 960.0f, 17.0f, 2.75f },    // Central crossing
    { 1700.0f, 900.0f, 15.0f, 5.50f },    // Horde track north
    { 1750.0f, 870.0f, 14.0f, 5.50f },    // Horde track mid
    { 1800.0f, 855.0f, 13.0f, 5.50f },    // Horde track approach
    { 1580.0f, 920.0f, 18.0f, 4.00f }     // Central ambush point
};

inline Position GetCartInterceptionPosition(uint32 index)
{
    if (index < CartInterception::COUNT)
        return Position(CART_INTERCEPTION_POSITIONS[index][0],
                       CART_INTERCEPTION_POSITIONS[index][1],
                       CART_INTERCEPTION_POSITIONS[index][2],
                       CART_INTERCEPTION_POSITIONS[index][3]);
    return Position(0, 0, 0, 0);
}

// ============================================================================
// CART ESCORT FORMATION (positions relative to cart)
// ============================================================================

namespace EscortFormation
{
    constexpr uint32 POSITION_COUNT = 6;
}

// Relative positions for escort formation (front, sides, rear)
constexpr float ESCORT_OFFSETS[][3] = {
    { 8.0f, 0.0f, 0.0f },     // Front center
    { 4.0f, 5.0f, 0.0f },     // Front right
    { 4.0f, -5.0f, 0.0f },    // Front left
    { -4.0f, 5.0f, 0.0f },    // Rear right
    { -4.0f, -5.0f, 0.0f },   // Rear left
    { -8.0f, 0.0f, 0.0f }     // Rear center
};

inline std::vector<Position> GetEscortFormation(float cartX, float cartY, float cartZ, float facing)
{
    std::vector<Position> positions;
    float cosF = std::cos(facing);
    float sinF = std::sin(facing);

    for (uint8 i = 0; i < EscortFormation::POSITION_COUNT; ++i)
    {
        float localX = ESCORT_OFFSETS[i][0];
        float localY = ESCORT_OFFSETS[i][1];

        // Rotate by facing angle
        float worldX = cartX + (localX * cosF - localY * sinF);
        float worldY = cartY + (localX * sinF + localY * cosF);

        positions.emplace_back(worldX, worldY, cartZ, facing);
    }

    return positions;
}

// ============================================================================
// CHOKEPOINTS (10 strategic positions)
// ============================================================================

namespace Chokepoints
{
    constexpr uint32 COUNT = 10;

    constexpr uint32 NORTH_BRIDGE = 0;
    constexpr uint32 PANDAREN_ENTRANCE = 1;
    constexpr uint32 GOBLIN_ROAD = 2;
    constexpr uint32 CENTER_CROSSING = 3;
    constexpr uint32 SOUTH_PASS = 4;
    constexpr uint32 ALLIANCE_APPROACH = 5;
    constexpr uint32 HORDE_APPROACH = 6;
    constexpr uint32 CART_JUNCTION = 7;
    constexpr uint32 MINE_RIDGE = 8;
    constexpr uint32 DEPOT_CORRIDOR = 9;
}

constexpr float CHOKEPOINT_POSITIONS[][4] = {
    { 1570.0f, 980.0f, 19.0f, 2.36f },    // 0 - North Bridge
    { 1600.0f, 965.0f, 20.0f, 0.0f },     // 1 - Pandaren Entrance
    { 1470.0f, 1080.0f, 14.0f, 2.36f },   // 2 - Goblin Road
    { 1600.0f, 880.0f, 19.0f, 3.14f },    // 3 - Center Crossing
    { 1720.0f, 810.0f, 17.0f, 5.50f },    // 4 - South Pass
    { 1400.0f, 1070.0f, 12.0f, 2.36f },   // 5 - Alliance Approach
    { 1800.0f, 820.0f, 14.0f, 5.50f },    // 6 - Horde Approach
    { 1520.0f, 960.0f, 18.0f, 2.75f },    // 7 - Cart Junction
    { 1650.0f, 900.0f, 18.0f, 3.93f },    // 8 - Mine Ridge
    { 1500.0f, 1000.0f, 16.0f, 2.36f }    // 9 - Depot Corridor
};

inline Position GetChokepointPosition(uint32 chokepointId)
{
    if (chokepointId < Chokepoints::COUNT)
        return Position(CHOKEPOINT_POSITIONS[chokepointId][0],
                       CHOKEPOINT_POSITIONS[chokepointId][1],
                       CHOKEPOINT_POSITIONS[chokepointId][2],
                       CHOKEPOINT_POSITIONS[chokepointId][3]);
    return Position(0, 0, 0, 0);
}

inline const char* GetChokepointName(uint32 chokepointId)
{
    static const char* names[] = {
        "North Bridge", "Pandaren Entrance", "Goblin Road", "Center Crossing",
        "South Pass", "Alliance Approach", "Horde Approach", "Cart Junction",
        "Mine Ridge", "Depot Corridor"
    };
    return chokepointId < Chokepoints::COUNT ? names[chokepointId] : "Unknown";
}

// ============================================================================
// SNIPER POSITIONS (6 elevated spots)
// ============================================================================

namespace SniperSpots
{
    constexpr uint32 COUNT = 6;

    constexpr uint32 PANDAREN_OVERLOOK = 0;
    constexpr uint32 GOBLIN_CLIFF = 1;
    constexpr uint32 CENTER_RIDGE = 2;
    constexpr uint32 ALLIANCE_HIGH = 3;
    constexpr uint32 HORDE_HIGH = 4;
    constexpr uint32 CART_OVERVIEW = 5;
}

constexpr float SNIPER_POSITIONS[][4] = {
    { 1610.0f, 960.0f, 25.0f, 3.14f },    // 0 - Pandaren Overlook
    { 1430.0f, 1120.0f, 22.0f, 5.50f },   // 1 - Goblin Cliff
    { 1770.0f, 795.0f, 24.0f, 1.57f },    // 2 - Center Ridge
    { 1370.0f, 1080.0f, 18.0f, 5.50f },   // 3 - Alliance High Ground
    { 1830.0f, 830.0f, 20.0f, 2.36f },    // 4 - Horde High Ground
    { 1570.0f, 930.0f, 24.0f, 3.93f }     // 5 - Cart Overview (central)
};

inline Position GetSniperPosition(uint32 sniperId)
{
    if (sniperId < SniperSpots::COUNT)
        return Position(SNIPER_POSITIONS[sniperId][0], SNIPER_POSITIONS[sniperId][1],
                       SNIPER_POSITIONS[sniperId][2], SNIPER_POSITIONS[sniperId][3]);
    return Position(0, 0, 0, 0);
}

inline const char* GetSniperSpotName(uint32 sniperId)
{
    static const char* names[] = {
        "Pandaren Overlook", "Goblin Cliff", "Center Ridge",
        "Alliance High Ground", "Horde High Ground", "Cart Overview"
    };
    return sniperId < SniperSpots::COUNT ? names[sniperId] : "Unknown";
}

// ============================================================================
// SPAWN POSITIONS
// ============================================================================

constexpr float ALLIANCE_SPAWN_X = 1350.0f;
constexpr float ALLIANCE_SPAWN_Y = 1100.0f;
constexpr float ALLIANCE_SPAWN_Z = 10.0f;
constexpr float ALLIANCE_SPAWN_O = 5.50f;

constexpr float HORDE_SPAWN_X = 1850.0f;
constexpr float HORDE_SPAWN_Y = 800.0f;
constexpr float HORDE_SPAWN_Z = 12.0f;
constexpr float HORDE_SPAWN_O = 2.36f;

inline Position GetSpawnPosition(uint32 faction)
{
    if (faction == 1)  // ALLIANCE
        return Position(ALLIANCE_SPAWN_X, ALLIANCE_SPAWN_Y, ALLIANCE_SPAWN_Z, ALLIANCE_SPAWN_O);
    else  // HORDE
        return Position(HORDE_SPAWN_X, HORDE_SPAWN_Y, HORDE_SPAWN_Z, HORDE_SPAWN_O);
}

// ============================================================================
// DISTANCE MATRIX (nodes + depots = 5 points)
// ============================================================================

constexpr float DISTANCE_MATRIX[5][5] = {
    //  Pan     Gob     Cen     ADepot  HDepot
    { 0.0f,    200.0f, 230.0f, 260.0f, 290.0f },  // Pandaren Mine
    { 200.0f,  0.0f,   350.0f, 80.0f,  480.0f },  // Goblin Mine
    { 230.0f,  350.0f, 0.0f,   440.0f, 120.0f },  // Center Mine
    { 260.0f,  80.0f,  440.0f, 0.0f,   550.0f },  // Alliance Depot
    { 290.0f,  480.0f, 120.0f, 550.0f, 0.0f }     // Horde Depot
};

inline float GetPointDistance(uint32 pointA, uint32 pointB)
{
    if (pointA < 5 && pointB < 5)
        return DISTANCE_MATRIX[pointA][pointB];
    return 9999.0f;
}

// ============================================================================
// AMBUSH POSITIONS (faction-specific)
// ============================================================================

namespace AmbushSpots
{
    constexpr uint32 ALLIANCE_COUNT = 5;
    constexpr uint32 HORDE_COUNT = 5;
}

constexpr float ALLIANCE_AMBUSH_POSITIONS[][4] = {
    { 1500.0f, 970.0f, 17.0f, 5.50f },    // Cart intercept
    { 1550.0f, 920.0f, 19.0f, 5.50f },    // Central ambush
    { 1480.0f, 1050.0f, 14.0f, 5.50f },   // Goblin road
    { 1600.0f, 890.0f, 19.0f, 4.71f },    // Center approach
    { 1430.0f, 1090.0f, 13.0f, 5.50f }    // Near Goblin Mine
};

constexpr float HORDE_AMBUSH_POSITIONS[][4] = {
    { 1700.0f, 880.0f, 16.0f, 2.36f },    // Cart intercept
    { 1650.0f, 920.0f, 18.0f, 2.36f },    // Central ambush
    { 1720.0f, 830.0f, 17.0f, 2.36f },    // Center road
    { 1600.0f, 960.0f, 20.0f, 1.57f },    // Pandaren approach
    { 1770.0f, 800.0f, 18.0f, 2.36f }     // Near Center Mine
};

inline std::vector<Position> GetAmbushPositions(uint32 faction)
{
    std::vector<Position> positions;
    const auto& data = (faction == 1) ? ALLIANCE_AMBUSH_POSITIONS : HORDE_AMBUSH_POSITIONS;
    uint32 count = (faction == 1) ? AmbushSpots::ALLIANCE_COUNT : AmbushSpots::HORDE_COUNT;

    for (uint32 i = 0; i < count; ++i)
    {
        positions.emplace_back(data[i][0], data[i][1], data[i][2], data[i][3]);
    }
    return positions;
}

// ============================================================================
// TICK POINTS TABLE (resources per controlled node)
// ============================================================================

constexpr std::array<uint32, 4> TICK_POINTS = { 0, 1, 3, 10 };  // 0, 1, 2, 3 nodes

inline uint32 GetTickPoints(uint32 controlledNodes)
{
    if (controlledNodes < TICK_POINTS.size())
        return TICK_POINTS[controlledNodes];
    return TICK_POINTS.back();
}

// ============================================================================
// WORLD STATES
// ============================================================================

namespace WorldStates
{
    constexpr int32 RESOURCES_ALLY = 6446;
    constexpr int32 RESOURCES_HORDE = 6447;
    constexpr int32 PANDAREN_STATE = 6448;
    constexpr int32 GOBLIN_STATE = 6449;
    constexpr int32 CENTER_STATE = 6450;
}

// ============================================================================
// GAME OBJECTS
// ============================================================================

namespace GameObjects
{
    constexpr uint32 ALLIANCE_BANNER = 220164;
    constexpr uint32 HORDE_BANNER = 220165;
    constexpr uint32 NEUTRAL_BANNER = 220166;
    constexpr uint32 MINE_CART_BASE = 220170;
}

// ============================================================================
// STRATEGY CONSTANTS
// ============================================================================

namespace Strategy
{
    // Node control
    constexpr uint8 MIN_NODE_DEFENDERS = 2;
    constexpr uint8 MAX_NODE_DEFENDERS = 4;
    constexpr uint8 PANDAREN_EXTRA_DEFENDERS = 1;  // Critical central node

    // Cart priority
    constexpr float CART_PRIORITY_THRESHOLD = 0.5f;  // When to prioritize carts
    constexpr uint8 MIN_CART_ESCORT = 3;
    constexpr uint8 OPTIMAL_CART_ESCORT = 5;

    // Timing
    constexpr uint32 NODE_ROTATION_INTERVAL = 20000;
    constexpr uint32 CART_CHECK_INTERVAL = 5000;
    constexpr uint32 DEFENSE_RESPONSE_TIME = 12000;

    // Score thresholds
    constexpr uint32 LEADING_THRESHOLD = 200;
    constexpr uint32 DESPERATE_THRESHOLD = 400;

    // Phase timing (ms)
    constexpr uint32 OPENING_PHASE = 90000;        // First 90 seconds
    constexpr uint32 MID_GAME_END = 900000;        // 15 minutes
    constexpr uint32 LATE_GAME_START = 900001;     // After 15 minutes

    // Optimal strategy
    constexpr uint8 OPTIMAL_NODE_COUNT = 2;  // Control 2 of 3 nodes
    constexpr float NODE_VS_CART_BALANCE = 0.6f;  // 60% nodes, 40% carts
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

inline float CalculateDistance(float x1, float y1, float x2, float y2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    return std::sqrt(dx * dx + dy * dy);
}

inline uint32 GetNearestNode(float x, float y)
{
    uint32 nearest = 0;
    float minDist = 99999.0f;

    for (uint32 i = 0; i < NODE_COUNT; ++i)
    {
        Position pos = GetNodePosition(i);
        float dist = CalculateDistance(x, y, pos.GetPositionX(), pos.GetPositionY());
        if (dist < minDist)
        {
            minDist = dist;
            nearest = i;
        }
    }
    return nearest;
}

inline bool IsNearAllianceDepot(float x, float y, float threshold = 50.0f)
{
    return CalculateDistance(x, y, ALLIANCE_CART_DEPOT_X, ALLIANCE_CART_DEPOT_Y) < threshold;
}

inline bool IsNearHordeDepot(float x, float y, float threshold = 50.0f)
{
    return CalculateDistance(x, y, HORDE_CART_DEPOT_X, HORDE_CART_DEPOT_Y) < threshold;
}

// ============================================================================
// CART TRACKING STRUCTURE
// ============================================================================

struct CartState
{
    uint32 id;
    Position position;
    float progress;        // 0.0 to 1.0 (capture progress)
    uint32 controllingFaction;  // 0 = neutral, 1 = Alliance, 2 = Horde
    bool contested;
    bool active;
};

} // namespace Playerbot::Coordination::Battleground::DeepwindGorge

#endif
