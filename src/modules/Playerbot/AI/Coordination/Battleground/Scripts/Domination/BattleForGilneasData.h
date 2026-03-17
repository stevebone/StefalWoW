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

#ifndef PLAYERBOT_AI_COORDINATION_BG_DOMINATION_BATTLEFORGILNEASDATA_H
#define PLAYERBOT_AI_COORDINATION_BG_DOMINATION_BATTLEFORGILNEASDATA_H

#include "Define.h"
#include "Position.h"
#include <array>
#include <vector>

namespace Playerbot::Coordination::Battleground::BattleForGilneas
{

// ============================================================================
// MAP INFORMATION
// ============================================================================

constexpr uint32 MAP_ID = 761;
constexpr char BG_NAME[] = "Battle for Gilneas";
constexpr uint32 MAX_SCORE = 2000;
constexpr uint32 MAX_DURATION = 25 * 60 * 1000; // 25 minutes
constexpr uint8 TEAM_SIZE = 10;
constexpr uint32 NODE_COUNT = 3;
constexpr uint32 TICK_INTERVAL = 2000;          // 2 seconds
constexpr uint32 CAPTURE_TIME = 8000;           // 8 seconds to capture/assault

// ============================================================================
// NODE IDENTIFIERS
// ============================================================================

namespace Nodes
{
    constexpr uint32 LIGHTHOUSE = 0;  // Alliance-side, northwest
    constexpr uint32 WATERWORKS = 1;  // Center, critical control point
    constexpr uint32 MINES = 2;       // Horde-side, southeast
}

// ============================================================================
// NODE POSITIONS
// ============================================================================

// Lighthouse (Alliance-side, northwest - elevated)
constexpr float LIGHTHOUSE_X = 1057.73f;
constexpr float LIGHTHOUSE_Y = 1278.29f;
constexpr float LIGHTHOUSE_Z = 3.19f;
constexpr float LIGHTHOUSE_O = 0.0f;

// Waterworks (Center, critical - low ground with water)
constexpr float WATERWORKS_X = 980.07f;
constexpr float WATERWORKS_Y = 948.17f;
constexpr float WATERWORKS_Z = 12.72f;
constexpr float WATERWORKS_O = 0.0f;

// Mines (Horde-side, southeast - underground)
constexpr float MINES_X = 1251.01f;
constexpr float MINES_Y = 836.59f;
constexpr float MINES_Z = -7.43f;
constexpr float MINES_O = 0.0f;

inline Position GetNodePosition(uint32 nodeId)
{
    switch (nodeId)
    {
        case Nodes::LIGHTHOUSE: return Position(LIGHTHOUSE_X, LIGHTHOUSE_Y, LIGHTHOUSE_Z, LIGHTHOUSE_O);
        case Nodes::WATERWORKS: return Position(WATERWORKS_X, WATERWORKS_Y, WATERWORKS_Z, WATERWORKS_O);
        case Nodes::MINES:      return Position(MINES_X, MINES_Y, MINES_Z, MINES_O);
        default: return Position(0, 0, 0, 0);
    }
}

inline const char* GetNodeName(uint32 nodeId)
{
    switch (nodeId)
    {
        case Nodes::LIGHTHOUSE: return "Lighthouse";
        case Nodes::WATERWORKS: return "Waterworks";
        case Nodes::MINES:      return "Mines";
        default: return "Unknown";
    }
}

// Node strategic values (1-10)
inline uint8 GetNodeStrategicValue(uint32 nodeId)
{
    switch (nodeId)
    {
        case Nodes::LIGHTHOUSE: return 7;  // Alliance home base
        case Nodes::WATERWORKS: return 10; // Center - critical control point
        case Nodes::MINES:      return 7;  // Horde home base
        default: return 5;
    }
}

// ============================================================================
// TICK POINTS TABLE
// ============================================================================

// Points per tick based on nodes controlled (BFG has different scaling)
// [0] = 0 nodes, [1] = 1 node, [2] = 2 nodes, [3] = 3 nodes
constexpr std::array<uint32, 4> TICK_POINTS = {
    0,   // 0 nodes
    1,   // 1 node - slow
    3,   // 2 nodes - moderate (2-cap strategy)
    10   // 3 nodes - fast (full control bonus)
};

// Resources per second (approximate for planning)
constexpr float RESOURCE_RATE_PER_NODE = 0.5f;  // 1 point / 2 seconds for 1 node

// Time to win calculations (seconds)
inline uint32 GetTimeToWin(uint32 currentScore, uint32 nodeCount)
{
    if (nodeCount == 0)
        return UINT32_MAX;
    uint32 remaining = MAX_SCORE - currentScore;
    uint32 pointsPerTick = TICK_POINTS[std::min(nodeCount, 3u)];
    if (pointsPerTick == 0)
        return UINT32_MAX;
    return (remaining * TICK_INTERVAL) / (pointsPerTick * 1000);
}

// ============================================================================
// SPAWN POSITIONS
// ============================================================================

// Alliance spawn (Northwest, near Lighthouse)
constexpr Position ALLIANCE_SPAWNS[] = {
    { 1052.0f, 1396.0f, 6.0f, 5.24f },   // Main spawn
    { 1047.0f, 1391.0f, 6.0f, 5.24f },
    { 1057.0f, 1401.0f, 6.0f, 5.24f },
    { 1042.0f, 1386.0f, 6.0f, 5.24f },
    { 1062.0f, 1406.0f, 6.0f, 5.24f }
};

// Horde spawn (Southeast, near Mines)
constexpr Position HORDE_SPAWNS[] = {
    { 1330.0f, 736.0f, -8.0f, 2.36f },   // Main spawn
    { 1325.0f, 731.0f, -8.0f, 2.36f },
    { 1335.0f, 741.0f, -8.0f, 2.36f },
    { 1320.0f, 726.0f, -8.0f, 2.36f },
    { 1340.0f, 746.0f, -8.0f, 2.36f }
};

// ============================================================================
// GRAVEYARD POSITIONS (one per node when controlled)
// ============================================================================

constexpr float LIGHTHOUSE_GY_X = 1058.15f;
constexpr float LIGHTHOUSE_GY_Y = 1343.65f;
constexpr float LIGHTHOUSE_GY_Z = 5.57f;

constexpr float WATERWORKS_GY_X = 978.35f;
constexpr float WATERWORKS_GY_Y = 983.47f;
constexpr float WATERWORKS_GY_Z = 5.35f;

constexpr float MINES_GY_X = 1243.41f;
constexpr float MINES_GY_Y = 763.13f;
constexpr float MINES_GY_Z = -62.42f;

inline Position GetNodeGraveyard(uint32 nodeId)
{
    switch (nodeId)
    {
        case Nodes::LIGHTHOUSE: return Position(LIGHTHOUSE_GY_X, LIGHTHOUSE_GY_Y, LIGHTHOUSE_GY_Z, 0);
        case Nodes::WATERWORKS: return Position(WATERWORKS_GY_X, WATERWORKS_GY_Y, WATERWORKS_GY_Z, 0);
        case Nodes::MINES:      return Position(MINES_GY_X, MINES_GY_Y, MINES_GY_Z, 0);
        default: return Position(0, 0, 0, 0);
    }
}

// ============================================================================
// NODE DEFENSE POSITIONS (ENTERPRISE-GRADE)
// ============================================================================

inline std::vector<Position> GetNodeDefensePositions(uint32 nodeId)
{
    switch (nodeId)
    {
        case Nodes::LIGHTHOUSE:
            return {
                // Core flag defense (elevated platform)
                { 1057.73f, 1278.29f, 3.19f, 0.0f },     // Flag position (center)
                { 1047.73f, 1278.29f, 3.19f, 3.14f },    // West
                { 1067.73f, 1278.29f, 3.19f, 0.0f },     // East
                { 1057.73f, 1268.29f, 3.19f, 1.57f },    // South (road to WW)
                { 1057.73f, 1288.29f, 3.19f, 4.71f },    // North (towards base)
                // Entrance control
                { 1070.73f, 1265.29f, 3.19f, 0.79f },    // SE entrance from WW
                { 1045.73f, 1265.29f, 3.19f, 2.36f },    // SW entrance
                { 1070.73f, 1290.29f, 5.19f, 5.50f },    // NE cliff overlook
                // Elevated lighthouse positions
                { 1055.73f, 1283.29f, 8.19f, 4.71f },    // Lighthouse stairs
                { 1060.73f, 1275.29f, 10.19f, 0.79f }    // Lighthouse top (sniper)
            };
        case Nodes::WATERWORKS:
            return {
                // Core flag defense (center - most important node)
                { 980.07f, 948.17f, 12.72f, 0.0f },      // Flag (center)
                { 970.07f, 948.17f, 12.72f, 3.14f },     // West
                { 990.07f, 948.17f, 12.72f, 0.0f },      // East
                { 980.07f, 938.17f, 12.72f, 1.57f },     // South (to Mines)
                { 980.07f, 958.17f, 12.72f, 4.71f },     // North (to LH)
                // Extra defense positions (critical node)
                { 975.07f, 943.17f, 12.72f, 2.36f },     // SW corner
                { 985.07f, 953.17f, 12.72f, 5.50f },     // NE corner
                { 975.07f, 953.17f, 12.72f, 3.93f },     // NW corner
                { 985.07f, 943.17f, 12.72f, 0.79f },     // SE corner
                // Bridge/ramp control
                { 965.07f, 948.17f, 10.72f, 3.14f },     // West bridge
                { 995.07f, 948.17f, 10.72f, 0.0f },      // East bridge
                { 980.07f, 930.17f, 15.72f, 1.57f }      // South elevated
            };
        case Nodes::MINES:
            return {
                // Core flag defense (underground)
                { 1251.01f, 836.59f, -7.43f, 0.0f },     // Flag position
                { 1241.01f, 836.59f, -7.43f, 3.14f },    // West
                { 1261.01f, 836.59f, -7.43f, 0.0f },     // East
                { 1251.01f, 826.59f, -7.43f, 1.57f },    // South (to base)
                { 1251.01f, 846.59f, -7.43f, 4.71f },    // North (to WW)
                // Mine entrance control
                { 1240.01f, 850.59f, -7.43f, 3.93f },    // NW entrance
                { 1262.01f, 822.59f, -7.43f, 0.79f },    // SE entrance
                // Tunnel positions
                { 1235.01f, 830.59f, -10.43f, 2.36f },   // West tunnel
                { 1265.01f, 842.59f, -10.43f, 5.50f },   // East tunnel
                // Outside elevated
                { 1255.01f, 860.59f, 2.57f, 4.71f }      // Mine entrance overlook
            };
        default:
            return {};
    }
}

// ============================================================================
// CHOKEPOINT POSITIONS
// ============================================================================

// Critical map chokepoints for ambushes and interception
inline std::vector<Position> GetChokepoints()
{
    return {
        // Lighthouse to Waterworks road
        { 1015.0f, 1115.0f, 8.0f, 2.36f },   // Mid-road LH->WW
        { 1035.0f, 1195.0f, 5.0f, 2.36f },   // Near Lighthouse

        // Waterworks to Mines road
        { 1115.0f, 890.0f, 0.0f, 0.79f },    // Mid-road WW->Mines
        { 1185.0f, 865.0f, -4.0f, 0.79f },   // Near Mines entrance

        // Central crossroads (critical!)
        { 1075.0f, 1020.0f, 10.0f, 1.18f },  // Center crossing north
        { 1100.0f, 950.0f, 5.0f, 0.79f },    // Center crossing south

        // Alliance base exit
        { 1055.0f, 1360.0f, 6.0f, 5.24f },   // Alliance base road

        // Horde base exit
        { 1285.0f, 780.0f, -5.0f, 2.36f }    // Horde base road
    };
}

// ============================================================================
// SNIPER/OVERLOOK POSITIONS
// ============================================================================

// High ground and line-of-sight advantage positions
inline std::vector<Position> GetSniperPositions()
{
    return {
        // Lighthouse tower (best sniper spot)
        { 1060.0f, 1275.0f, 15.0f, 1.57f },  // LH tower top - sees WW approach

        // Waterworks elevated positions
        { 990.0f, 970.0f, 18.0f, 5.50f },    // WW cliff north - sees LH road
        { 970.0f, 930.0f, 16.0f, 2.36f },    // WW cliff south - sees Mines road

        // Mines entrance overlook
        { 1255.0f, 865.0f, 5.0f, 4.71f },    // Above Mine entrance

        // Central hill
        { 1090.0f, 1000.0f, 20.0f, 3.14f },  // High ground center - sees all roads

        // Cliff overlooking WW from east
        { 1050.0f, 950.0f, 22.0f, 0.79f },   // Eastern cliff - good LOS to WW

        // Rocky outcrop between nodes
        { 1130.0f, 920.0f, 12.0f, 2.36f }    // Between WW and Mines
    };
}

// ============================================================================
// BUFF POSITIONS (Restoration Buffs)
// ============================================================================

// Health/Mana restoration buff locations
inline std::vector<Position> GetBuffPositions()
{
    return {
        // Near Waterworks (contested area)
        { 995.0f, 965.0f, 10.0f, 0.0f },     // WW east buff

        // Near Lighthouse approach
        { 1040.0f, 1220.0f, 4.0f, 0.0f },    // LH south buff

        // Near Mines approach
        { 1200.0f, 860.0f, -2.0f, 0.0f }     // Mines north buff
    };
}

// ============================================================================
// STRATEGIC ROUTE DATA
// ============================================================================

// Standard opening routes by faction
inline std::vector<uint32> GetAllianceOpeningRoute()
{
    return { Nodes::LIGHTHOUSE, Nodes::WATERWORKS };
}

inline std::vector<uint32> GetHordeOpeningRoute()
{
    return { Nodes::MINES, Nodes::WATERWORKS };
}

// 2-cap strategy routes (optimal for BFG)
inline std::vector<uint32> GetAlliance2CapRoute()
{
    // Alliance typically takes Lighthouse + Waterworks
    return { Nodes::LIGHTHOUSE, Nodes::WATERWORKS };
}

inline std::vector<uint32> GetHorde2CapRoute()
{
    // Horde typically takes Mines + Waterworks
    return { Nodes::MINES, Nodes::WATERWORKS };
}

// Node adjacency (which nodes are close to each other)
inline std::vector<uint32> GetAdjacentNodes(uint32 nodeId)
{
    switch (nodeId)
    {
        case Nodes::LIGHTHOUSE: return { Nodes::WATERWORKS };
        case Nodes::WATERWORKS: return { Nodes::LIGHTHOUSE, Nodes::MINES };
        case Nodes::MINES:      return { Nodes::WATERWORKS };
        default: return {};
    }
}

// Distance matrix between nodes (pre-calculated for pathfinding)
inline float GetNodeDistance(uint32 fromNode, uint32 toNode)
{
    // Approximate travel distances (in yards)
    static const float distances[3][3] = {
        //              LH      WW      Mines
        /* LH */    { 0.0f,   350.0f, 600.0f },
        /* WW */    { 350.0f, 0.0f,   300.0f },
        /* Mines */ { 600.0f, 300.0f, 0.0f }
    };

    if (fromNode < 3 && toNode < 3)
        return distances[fromNode][toNode];
    return 1000.0f;  // Invalid
}

// ============================================================================
// ROTATION PATHS (Node-to-Node travel routes)
// ============================================================================

// Get path from one node to another
inline std::vector<Position> GetRotationPath(uint32 fromNode, uint32 toNode)
{
    Position start = GetNodePosition(fromNode);
    Position end = GetNodePosition(toNode);

    // Add intermediate waypoints for common routes
    if (fromNode == Nodes::LIGHTHOUSE && toNode == Nodes::WATERWORKS)
    {
        return {
            start,
            { 1045.0f, 1240.0f, 4.0f, 2.36f },
            { 1025.0f, 1170.0f, 6.0f, 2.36f },
            { 1000.0f, 1060.0f, 10.0f, 2.36f },
            { 985.0f, 1000.0f, 12.0f, 2.36f },
            end
        };
    }
    else if (fromNode == Nodes::WATERWORKS && toNode == Nodes::LIGHTHOUSE)
    {
        return {
            start,
            { 985.0f, 1000.0f, 12.0f, 5.50f },
            { 1000.0f, 1060.0f, 10.0f, 5.50f },
            { 1025.0f, 1170.0f, 6.0f, 5.50f },
            { 1045.0f, 1240.0f, 4.0f, 5.50f },
            end
        };
    }
    else if (fromNode == Nodes::WATERWORKS && toNode == Nodes::MINES)
    {
        return {
            start,
            { 1020.0f, 920.0f, 10.0f, 0.79f },
            { 1080.0f, 890.0f, 5.0f, 0.79f },
            { 1150.0f, 865.0f, 0.0f, 0.79f },
            { 1210.0f, 845.0f, -4.0f, 0.79f },
            end
        };
    }
    else if (fromNode == Nodes::MINES && toNode == Nodes::WATERWORKS)
    {
        return {
            start,
            { 1210.0f, 845.0f, -4.0f, 3.93f },
            { 1150.0f, 865.0f, 0.0f, 3.93f },
            { 1080.0f, 890.0f, 5.0f, 3.93f },
            { 1020.0f, 920.0f, 10.0f, 3.93f },
            end
        };
    }
    else if (fromNode == Nodes::LIGHTHOUSE && toNode == Nodes::MINES)
    {
        // Long route through WW
        return {
            start,
            { 1025.0f, 1170.0f, 6.0f, 2.36f },
            { 985.0f, 1000.0f, 12.0f, 2.36f },
            GetNodePosition(Nodes::WATERWORKS),
            { 1080.0f, 890.0f, 5.0f, 0.79f },
            { 1210.0f, 845.0f, -4.0f, 0.79f },
            end
        };
    }
    else if (fromNode == Nodes::MINES && toNode == Nodes::LIGHTHOUSE)
    {
        // Long route through WW
        return {
            start,
            { 1210.0f, 845.0f, -4.0f, 3.93f },
            { 1080.0f, 890.0f, 5.0f, 3.93f },
            GetNodePosition(Nodes::WATERWORKS),
            { 985.0f, 1000.0f, 12.0f, 5.50f },
            { 1025.0f, 1170.0f, 6.0f, 5.50f },
            end
        };
    }
    // Default: direct route
    return { start, end };
}

// ============================================================================
// AMBUSH POSITIONS
// ============================================================================

// Positions for intercepting enemy rotations
inline std::vector<Position> GetAmbushPositions(uint32 faction)
{
    if (faction == ALLIANCE)
    {
        return {
            // Intercept Horde going to Lighthouse
            { 1020.0f, 1100.0f, 8.0f, 0.79f },
            // Intercept at WW from south
            { 1050.0f, 920.0f, 8.0f, 1.57f },
            // Intercept at central road
            { 1090.0f, 970.0f, 10.0f, 0.79f }
        };
    }
    else
    {
        return {
            // Intercept Alliance going to Mines
            { 1180.0f, 870.0f, 0.0f, 3.93f },
            // Intercept at WW from north
            { 960.0f, 980.0f, 12.0f, 4.71f },
            // Intercept at central road
            { 1070.0f, 1000.0f, 10.0f, 3.93f }
        };
    }
}

// ============================================================================
// WORLD STATES
// ============================================================================

namespace WorldStates
{
    // Resource totals
    constexpr int32 RESOURCES_ALLY = 5496;
    constexpr int32 RESOURCES_HORDE = 5497;

    // Max resources
    constexpr int32 MAX_RESOURCES = 5498;

    // Node state world states
    constexpr int32 LIGHTHOUSE_ALLIANCE = 5480;
    constexpr int32 LIGHTHOUSE_HORDE = 5481;
    constexpr int32 LIGHTHOUSE_ALLIANCE_CONTROLLED = 5482;
    constexpr int32 LIGHTHOUSE_HORDE_CONTROLLED = 5483;

    constexpr int32 WATERWORKS_ALLIANCE = 5484;
    constexpr int32 WATERWORKS_HORDE = 5485;
    constexpr int32 WATERWORKS_ALLIANCE_CONTROLLED = 5486;
    constexpr int32 WATERWORKS_HORDE_CONTROLLED = 5487;

    constexpr int32 MINES_ALLIANCE = 5488;
    constexpr int32 MINES_HORDE = 5489;
    constexpr int32 MINES_ALLIANCE_CONTROLLED = 5490;
    constexpr int32 MINES_HORDE_CONTROLLED = 5491;

    // Occupied bases count
    constexpr int32 OCCUPIED_BASES_ALLY = 5492;
    constexpr int32 OCCUPIED_BASES_HORDE = 5493;
}

// ============================================================================
// GAME OBJECTS
// ============================================================================

namespace GameObjects
{
    // Node banner objects
    constexpr uint32 LIGHTHOUSE_BANNER = 208522;
    constexpr uint32 WATERWORKS_BANNER = 208523;
    constexpr uint32 MINES_BANNER = 208524;

    // Aura objects (show control state)
    constexpr uint32 ALLIANCE_BANNER = 180058;
    constexpr uint32 HORDE_BANNER = 180059;
    constexpr uint32 NEUTRAL_BANNER = 180060;

    // Doors
    constexpr uint32 ALLIANCE_DOOR = 208480;
    constexpr uint32 HORDE_DOOR = 208484;
}

// ============================================================================
// SPELLS
// ============================================================================

namespace Spells
{
    constexpr uint32 HONORABLE_DEFENDER_25 = 21235;  // +25% honor when defending
    constexpr uint32 HONORABLE_DEFENDER_50 = 21236;  // +50% honor (2+ defenders)

    // Node assault/capture related
    constexpr uint32 ASSAULT_BANNER = 86746;   // Channeled spell for assaulting (BFG specific)
}

// ============================================================================
// STRATEGY CONSTANTS
// ============================================================================

namespace Strategy
{
    // Minimum defenders per node for 2-cap strategy
    constexpr uint8 MIN_DEFENDERS_PER_NODE = 2;

    // Waterworks always needs extra defenders (critical center node)
    constexpr uint8 WW_EXTRA_DEFENDERS = 2;

    // Time to rotate between nodes (milliseconds)
    constexpr uint32 ROTATION_INTERVAL = 15000;

    // Time to respond to node under attack
    constexpr uint32 DEFENSE_RESPONSE_TIME = 5000;

    // Minimum players to send for an assault
    constexpr uint8 MIN_ASSAULT_FORCE = 3;

    // Score threshold for switching to defensive
    constexpr uint32 DEFENSIVE_THRESHOLD = 1600;  // 80% of max score

    // Score threshold for desperation all-in
    constexpr uint32 DESPERATION_THRESHOLD = 500;  // Far behind

    // BFG-specific: 2-cap is optimal (3 points/tick vs 10 for 3-cap)
    constexpr uint8 OPTIMAL_NODE_COUNT = 2;

    // Opening rush timing
    constexpr uint32 OPENING_PHASE_DURATION = 60000;  // First minute

    // Mid-game phase
    constexpr uint32 MID_GAME_START = 60000;
    constexpr uint32 MID_GAME_END = 1200000;  // 20 minutes

    // Late game (desperate measures)
    constexpr uint32 LATE_GAME_START = 1200000;
}

} // namespace Playerbot::Coordination::Battleground::BattleForGilneas

#endif // PLAYERBOT_AI_COORDINATION_BG_DOMINATION_BATTLEFORGILNEASDATA_H
