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

#ifndef PLAYERBOT_AI_COORDINATION_BG_DOMINATION_ARATHIBASINDATA_H
#define PLAYERBOT_AI_COORDINATION_BG_DOMINATION_ARATHIBASINDATA_H

#include "Define.h"
#include "Position.h"
#include <array>
#include <vector>

namespace Playerbot::Coordination::Battleground::ArathiBasin
{

// ============================================================================
// MAP INFORMATION
// ============================================================================

constexpr uint32 MAP_ID = 529;
constexpr char BG_NAME[] = "Arathi Basin";
constexpr uint32 MAX_SCORE = 1500;             // Changed from 1600/2000 in various patches
constexpr uint32 MAX_DURATION = 25 * 60 * 1000; // 25 minutes
constexpr uint8 TEAM_SIZE = 15;
constexpr uint32 NODE_COUNT = 5;
constexpr uint32 TICK_INTERVAL = 2000;          // 2 seconds
constexpr uint32 CAPTURE_TIME = 8000;           // 8 seconds to capture/assault

// ============================================================================
// NODE IDENTIFIERS
// ============================================================================

namespace Nodes
{
    constexpr uint32 STABLES = 0;
    constexpr uint32 BLACKSMITH = 1;
    constexpr uint32 FARM = 2;
    constexpr uint32 GOLD_MINE = 3;
    constexpr uint32 LUMBER_MILL = 4;
}

// ============================================================================
// NODE POSITIONS
// ============================================================================

// Stables (Alliance-side, north)
constexpr float STABLES_X = 1166.785f;
constexpr float STABLES_Y = 1200.132f;
constexpr float STABLES_Z = -56.70f;
constexpr float STABLES_O = 0.0f;

// Blacksmith (Center, critical)
constexpr float BLACKSMITH_X = 977.017f;
constexpr float BLACKSMITH_Y = 1046.534f;
constexpr float BLACKSMITH_Z = -44.80f;
constexpr float BLACKSMITH_O = 0.0f;

// Farm (Horde-side, south)
constexpr float FARM_X = 806.218f;
constexpr float FARM_Y = 874.217f;
constexpr float FARM_Z = -55.99f;
constexpr float FARM_O = 0.0f;

// Gold Mine (Horde-side, east)
constexpr float GOLD_MINE_X = 1146.923f;
constexpr float GOLD_MINE_Y = 848.277f;
constexpr float GOLD_MINE_Z = -110.52f;
constexpr float GOLD_MINE_O = 0.0f;

// Lumber Mill (Alliance-side, west - high ground)
constexpr float LUMBER_MILL_X = 856.141f;
constexpr float LUMBER_MILL_Y = 1148.902f;
constexpr float LUMBER_MILL_Z = 11.18f;
constexpr float LUMBER_MILL_O = 0.0f;

inline Position GetNodePosition(uint32 nodeId)
{
    switch (nodeId)
    {
        case Nodes::STABLES:     return Position(STABLES_X, STABLES_Y, STABLES_Z, STABLES_O);
        case Nodes::BLACKSMITH:  return Position(BLACKSMITH_X, BLACKSMITH_Y, BLACKSMITH_Z, BLACKSMITH_O);
        case Nodes::FARM:        return Position(FARM_X, FARM_Y, FARM_Z, FARM_O);
        case Nodes::GOLD_MINE:   return Position(GOLD_MINE_X, GOLD_MINE_Y, GOLD_MINE_Z, GOLD_MINE_O);
        case Nodes::LUMBER_MILL: return Position(LUMBER_MILL_X, LUMBER_MILL_Y, LUMBER_MILL_Z, LUMBER_MILL_O);
        default: return Position(0, 0, 0, 0);
    }
}

inline const char* GetNodeName(uint32 nodeId)
{
    switch (nodeId)
    {
        case Nodes::STABLES:     return "Stables";
        case Nodes::BLACKSMITH:  return "Blacksmith";
        case Nodes::FARM:        return "Farm";
        case Nodes::GOLD_MINE:   return "Gold Mine";
        case Nodes::LUMBER_MILL: return "Lumber Mill";
        default: return "Unknown";
    }
}

// Node strategic values (1-10)
inline uint8 GetNodeStrategicValue(uint32 nodeId)
{
    switch (nodeId)
    {
        case Nodes::STABLES:     return 7;  // Alliance home base
        case Nodes::BLACKSMITH:  return 10; // Center - critical control point
        case Nodes::FARM:        return 7;  // Horde home base
        case Nodes::GOLD_MINE:   return 6;  // Distant, lower value
        case Nodes::LUMBER_MILL: return 8;  // High ground advantage
        default: return 5;
    }
}

// ============================================================================
// TICK POINTS TABLE
// ============================================================================

// Points per tick based on nodes controlled
// [0] = 0 nodes, [1] = 1 node, etc.
constexpr std::array<uint32, 6> TICK_POINTS = {
    0,   // 0 nodes
    10,  // 1 node
    10,  // 2 nodes
    10,  // 3 nodes
    10,  // 4 nodes
    30   // 5 nodes (full control bonus)
};

// Resources per second (approximate for planning)
constexpr float RESOURCE_RATE_PER_NODE = 5.0f;  // 10 points / 2 seconds

// Time to win calculations (seconds)
inline uint32 GetTimeToWin(uint32 currentScore, uint32 nodeCount)
{
    if (nodeCount == 0)
        return UINT32_MAX;
    uint32 remaining = MAX_SCORE - currentScore;
    uint32 pointsPerTick = TICK_POINTS[std::min(nodeCount, 5u)];
    if (pointsPerTick == 0)
        return UINT32_MAX;
    return (remaining * TICK_INTERVAL) / (pointsPerTick * 1000);
}

// ============================================================================
// SPAWN POSITIONS
// ============================================================================

// Alliance spawn (Trollbane Hall)
constexpr Position ALLIANCE_SPAWNS[] = {
    { 1285.96f, 1281.62f, -15.67f, 0.7f },
    { 1280.96f, 1276.62f, -15.67f, 0.7f },
    { 1290.96f, 1286.62f, -15.67f, 0.7f },
    { 1275.96f, 1271.62f, -15.67f, 0.7f },
    { 1295.96f, 1291.62f, -15.67f, 0.7f }
};

// Horde spawn (Defiler's Den)
constexpr Position HORDE_SPAWNS[] = {
    { 686.57f, 683.04f, -12.59f, 0.7f },
    { 691.57f, 688.04f, -12.59f, 0.7f },
    { 681.57f, 678.04f, -12.59f, 0.7f },
    { 696.57f, 693.04f, -12.59f, 0.7f },
    { 676.57f, 673.04f, -12.59f, 0.7f }
};

// ============================================================================
// GRAVEYARD POSITIONS (one per node when controlled)
// ============================================================================

constexpr float STABLES_GY_X = 1237.64f;
constexpr float STABLES_GY_Y = 1212.19f;
constexpr float STABLES_GY_Z = -57.74f;

constexpr float BLACKSMITH_GY_X = 1016.49f;
constexpr float BLACKSMITH_GY_Y = 1062.50f;
constexpr float BLACKSMITH_GY_Z = -44.64f;

constexpr float FARM_GY_X = 809.67f;
constexpr float FARM_GY_Y = 842.91f;
constexpr float FARM_GY_Z = -56.11f;

constexpr float GOLD_MINE_GY_X = 1104.35f;
constexpr float GOLD_MINE_GY_Y = 819.79f;
constexpr float GOLD_MINE_GY_Z = -111.06f;

constexpr float LUMBER_MILL_GY_X = 847.68f;
constexpr float LUMBER_MILL_GY_Y = 1176.47f;
constexpr float LUMBER_MILL_GY_Z = 12.22f;

inline Position GetNodeGraveyard(uint32 nodeId)
{
    switch (nodeId)
    {
        case Nodes::STABLES:     return Position(STABLES_GY_X, STABLES_GY_Y, STABLES_GY_Z, 0);
        case Nodes::BLACKSMITH:  return Position(BLACKSMITH_GY_X, BLACKSMITH_GY_Y, BLACKSMITH_GY_Z, 0);
        case Nodes::FARM:        return Position(FARM_GY_X, FARM_GY_Y, FARM_GY_Z, 0);
        case Nodes::GOLD_MINE:   return Position(GOLD_MINE_GY_X, GOLD_MINE_GY_Y, GOLD_MINE_GY_Z, 0);
        case Nodes::LUMBER_MILL: return Position(LUMBER_MILL_GY_X, LUMBER_MILL_GY_Y, LUMBER_MILL_GY_Z, 0);
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
        case Nodes::STABLES:
            return {
                // Core flag defense
                { 1166.78f, 1200.13f, -56.70f, 0.0f },   // Flag position
                { 1156.78f, 1200.13f, -56.70f, 3.14f },  // North
                { 1176.78f, 1200.13f, -56.70f, 0.0f },   // South
                { 1166.78f, 1190.13f, -56.70f, 1.57f },  // East (road to BS)
                { 1166.78f, 1210.13f, -56.70f, 4.71f },  // West (road to LM)
                // Entrance control
                { 1175.78f, 1185.13f, -56.70f, 0.79f },  // SE entrance
                { 1155.78f, 1215.13f, -56.70f, 3.93f },  // NW entrance
                // Elevated positions
                { 1170.78f, 1205.13f, -54.00f, 2.36f }   // Elevated overlook
            };
        case Nodes::BLACKSMITH:
            return {
                // Core flag defense (most important node)
                { 977.02f, 1046.53f, -44.80f, 0.0f },    // Flag (center)
                { 967.02f, 1046.53f, -44.80f, 3.14f },   // North
                { 987.02f, 1046.53f, -44.80f, 0.0f },    // South
                { 977.02f, 1036.53f, -44.80f, 1.57f },   // East
                { 977.02f, 1056.53f, -44.80f, 4.71f },   // West
                // Extra defense positions (critical node)
                { 972.02f, 1041.53f, -44.80f, 2.36f },   // NE corner
                { 982.02f, 1051.53f, -44.80f, 5.50f },   // SW corner
                { 972.02f, 1051.53f, -44.80f, 3.93f },   // NW corner
                { 982.02f, 1041.53f, -44.80f, 0.79f },   // SE corner
                // Bridge/ramp control
                { 960.02f, 1046.53f, -44.80f, 3.14f },   // North road
                { 994.02f, 1046.53f, -44.80f, 0.0f },    // South road
                { 977.02f, 1020.53f, -44.80f, 1.57f },   // East road to GM
                { 977.02f, 1072.53f, -44.80f, 4.71f }    // West road to LM
            };
        case Nodes::FARM:
            return {
                // Core flag defense
                { 806.22f, 874.22f, -55.99f, 0.0f },     // Flag position
                { 796.22f, 874.22f, -55.99f, 3.14f },    // North
                { 816.22f, 874.22f, -55.99f, 0.0f },     // South
                { 806.22f, 864.22f, -55.99f, 1.57f },    // East
                { 806.22f, 884.22f, -55.99f, 4.71f },    // West
                // Farm building positions
                { 820.22f, 890.22f, -55.99f, 5.50f },    // Barn corner
                { 790.22f, 860.22f, -55.99f, 2.36f },    // Windmill side
                // Entrance chokes
                { 815.22f, 860.22f, -55.99f, 0.79f }     // Road to BS
            };
        case Nodes::GOLD_MINE:
            return {
                // Core flag defense (inside mine)
                { 1146.92f, 848.28f, -110.52f, 0.0f },   // Flag position
                { 1136.92f, 848.28f, -110.52f, 3.14f },  // North
                { 1156.92f, 848.28f, -110.52f, 0.0f },   // South
                { 1146.92f, 838.28f, -110.52f, 1.57f },  // East
                { 1146.92f, 858.28f, -110.52f, 4.71f },  // West
                // Mine entrance control
                { 1130.92f, 830.28f, -110.52f, 2.36f },  // Mine entrance
                { 1160.92f, 865.28f, -105.52f, 5.50f },  // Ramp top
                // Outside positions
                { 1110.92f, 835.28f, -90.52f, 2.36f }    // Outside overlook
            };
        case Nodes::LUMBER_MILL:
            return {
                // Core flag defense (elevated)
                { 856.14f, 1148.90f, 11.18f, 0.0f },     // Flag position
                { 846.14f, 1148.90f, 11.18f, 3.14f },    // North
                { 866.14f, 1148.90f, 11.18f, 0.0f },     // South
                { 856.14f, 1138.90f, 11.18f, 1.57f },    // East
                { 856.14f, 1158.90f, 11.18f, 4.71f },    // West
                // Cliff edge positions (high ground advantage!)
                { 861.14f, 1143.90f, 11.18f, 0.79f },    // SE cliff - overlooks BS
                { 851.14f, 1153.90f, 11.18f, 3.93f },    // NW cliff - overlooks Stables
                { 866.14f, 1158.90f, 15.18f, 5.50f },    // High platform
                // Ramp defense
                { 840.14f, 1135.90f, 5.18f, 2.36f },     // Ramp bottom
                { 850.14f, 1140.90f, 9.18f, 2.36f }      // Ramp mid
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
        // Stables to Blacksmith road
        { 1070.0f, 1125.0f, -55.0f, 3.93f },  // Mid-road ST->BS
        { 1120.0f, 1175.0f, -56.0f, 3.93f },  // Near Stables

        // Blacksmith to Farm road
        { 890.0f, 960.0f, -50.0f, 2.36f },    // Mid-road BS->Farm
        { 850.0f, 920.0f, -53.0f, 2.36f },    // Near Farm

        // Blacksmith to Gold Mine road
        { 1060.0f, 945.0f, -80.0f, 0.79f },   // Mid-road BS->GM
        { 1100.0f, 895.0f, -100.0f, 0.79f },  // Near GM entrance

        // Blacksmith to Lumber Mill road
        { 915.0f, 1095.0f, -20.0f, 3.14f },   // Mid-road BS->LM
        { 885.0f, 1120.0f, 0.0f, 3.93f },     // Near LM ramp base

        // Stables to Lumber Mill road
        { 1010.0f, 1175.0f, -30.0f, 4.71f },  // Mid-road ST->LM

        // Farm to Gold Mine road
        { 975.0f, 860.0f, -70.0f, 0.0f },     // Mid-road Farm->GM

        // Center crossroads (critical!)
        { 980.0f, 1000.0f, -48.0f, 0.0f },    // True center

        // Alliance base exit
        { 1220.0f, 1250.0f, -35.0f, 3.93f },  // Alliance base road

        // Horde base exit
        { 750.0f, 740.0f, -30.0f, 0.79f }     // Horde base road
    };
}

// ============================================================================
// SNIPER/OVERLOOK POSITIONS
// ============================================================================

// High ground and line-of-sight advantage positions
inline std::vector<Position> GetSniperPositions()
{
    return {
        // Lumber Mill overlooks (best sniper spots in AB)
        { 850.0f, 1140.0f, 15.0f, 0.79f },    // LM cliff - sees BS, Stables approach
        { 865.0f, 1155.0f, 18.0f, 5.50f },    // LM high platform - sees Farm road

        // Blacksmith elevated positions
        { 985.0f, 1060.0f, -40.0f, 5.50f },   // BS elevated south
        { 965.0f, 1035.0f, -40.0f, 0.79f },   // BS elevated north

        // Gold Mine entrance overlook
        { 1110.0f, 830.0f, -90.0f, 2.36f },   // Outside GM cave

        // Stables hill
        { 1180.0f, 1215.0f, -52.0f, 3.93f },  // Stables hill north

        // Farm windmill area
        { 795.0f, 860.0f, -50.0f, 2.36f }     // Farm elevated
    };
}

// ============================================================================
// BUFF POSITIONS (Restoration Buffs)
// ============================================================================

// Health/Mana restoration buff locations
inline std::vector<Position> GetBuffPositions()
{
    return {
        // Near Blacksmith (contested area)
        { 990.0f, 1008.0f, -45.0f, 0.0f },    // BS east buff

        // Near Gold Mine entrance
        { 1080.0f, 870.0f, -95.0f, 0.0f },    // GM approach buff

        // Near Lumber Mill base
        { 870.0f, 1100.0f, -15.0f, 0.0f },    // LM base buff

        // Stables approach
        { 1130.0f, 1165.0f, -55.0f, 0.0f },   // Stables south buff

        // Farm approach
        { 840.0f, 910.0f, -55.0f, 0.0f }      // Farm north buff
    };
}

// ============================================================================
// STRATEGIC ROUTE DATA
// ============================================================================

// Standard opening routes by faction
inline std::vector<uint32> GetAllianceOpeningRoute()
{
    return { Nodes::STABLES, Nodes::BLACKSMITH, Nodes::LUMBER_MILL };
}

inline std::vector<uint32> GetHordeOpeningRoute()
{
    return { Nodes::FARM, Nodes::BLACKSMITH, Nodes::GOLD_MINE };
}

// Fast 5-cap rush routes (aggressive strategy)
inline std::vector<uint32> GetAlliance5CapRoute()
{
    // Rush Stables -> BS -> LM -> Farm -> GM
    return { Nodes::STABLES, Nodes::BLACKSMITH, Nodes::LUMBER_MILL, Nodes::FARM, Nodes::GOLD_MINE };
}

inline std::vector<uint32> GetHorde5CapRoute()
{
    // Rush Farm -> BS -> GM -> LM -> Stables
    return { Nodes::FARM, Nodes::BLACKSMITH, Nodes::GOLD_MINE, Nodes::LUMBER_MILL, Nodes::STABLES };
}

// Node adjacency (which nodes are close to each other)
inline std::vector<uint32> GetAdjacentNodes(uint32 nodeId)
{
    switch (nodeId)
    {
        case Nodes::STABLES:     return { Nodes::BLACKSMITH, Nodes::LUMBER_MILL };
        case Nodes::BLACKSMITH:  return { Nodes::STABLES, Nodes::FARM, Nodes::GOLD_MINE, Nodes::LUMBER_MILL };
        case Nodes::FARM:        return { Nodes::BLACKSMITH, Nodes::GOLD_MINE };
        case Nodes::GOLD_MINE:   return { Nodes::FARM, Nodes::BLACKSMITH };
        case Nodes::LUMBER_MILL: return { Nodes::STABLES, Nodes::BLACKSMITH };
        default: return {};
    }
}

// Distance matrix between nodes (pre-calculated for pathfinding)
inline float GetNodeDistance(uint32 fromNode, uint32 toNode)
{
    // Approximate travel distances (in yards)
    static const float distances[5][5] = {
        //           ST      BS      Farm    GM      LM
        /* ST */   { 0.0f,   200.0f, 400.0f, 360.0f, 180.0f },
        /* BS */   { 200.0f, 0.0f,   180.0f, 200.0f, 150.0f },
        /* Farm */ { 400.0f, 180.0f, 0.0f,   170.0f, 350.0f },
        /* GM */   { 360.0f, 200.0f, 170.0f, 0.0f,   380.0f },
        /* LM */   { 180.0f, 150.0f, 350.0f, 380.0f, 0.0f }
    };

    if (fromNode < 5 && toNode < 5)
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
    if (fromNode == Nodes::STABLES && toNode == Nodes::BLACKSMITH)
    {
        return {
            start,
            { 1120.0f, 1175.0f, -56.0f, 3.93f },
            { 1070.0f, 1125.0f, -55.0f, 3.93f },
            { 1020.0f, 1085.0f, -50.0f, 3.93f },
            end
        };
    }
    else if (fromNode == Nodes::BLACKSMITH && toNode == Nodes::STABLES)
    {
        return {
            start,
            { 1020.0f, 1085.0f, -50.0f, 0.79f },
            { 1070.0f, 1125.0f, -55.0f, 0.79f },
            { 1120.0f, 1175.0f, -56.0f, 0.79f },
            end
        };
    }
    else if (fromNode == Nodes::BLACKSMITH && toNode == Nodes::LUMBER_MILL)
    {
        return {
            start,
            { 940.0f, 1075.0f, -35.0f, 3.93f },
            { 900.0f, 1110.0f, -10.0f, 3.93f },
            { 870.0f, 1130.0f, 5.0f, 3.93f },
            end
        };
    }
    else if (fromNode == Nodes::BLACKSMITH && toNode == Nodes::FARM)
    {
        return {
            start,
            { 940.0f, 1010.0f, -48.0f, 2.36f },
            { 890.0f, 960.0f, -50.0f, 2.36f },
            { 850.0f, 920.0f, -53.0f, 2.36f },
            end
        };
    }
    else if (fromNode == Nodes::BLACKSMITH && toNode == Nodes::GOLD_MINE)
    {
        return {
            start,
            { 1010.0f, 1010.0f, -55.0f, 0.79f },
            { 1060.0f, 945.0f, -80.0f, 0.79f },
            { 1100.0f, 895.0f, -100.0f, 0.79f },
            end
        };
    }
    else if (fromNode == Nodes::FARM && toNode == Nodes::GOLD_MINE)
    {
        return {
            start,
            { 850.0f, 860.0f, -60.0f, 0.79f },
            { 920.0f, 855.0f, -75.0f, 0.0f },
            { 1020.0f, 850.0f, -90.0f, 0.0f },
            { 1100.0f, 848.0f, -105.0f, 0.0f },
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
            // Intercept Horde going to Stables
            { 1100.0f, 1150.0f, -56.0f, 2.36f },
            // Intercept at BS from south
            { 950.0f, 1020.0f, -46.0f, 1.57f },
            // Intercept at LM ramp
            { 865.0f, 1125.0f, 0.0f, 2.36f }
        };
    }
    else
    {
        return {
            // Intercept Alliance going to Farm
            { 850.0f, 920.0f, -53.0f, 5.50f },
            // Intercept at BS from north
            { 1000.0f, 1070.0f, -46.0f, 4.71f },
            // Intercept at GM entrance
            { 1120.0f, 860.0f, -100.0f, 3.93f }
        };
    }
}

// ============================================================================
// WORLD STATES
// ============================================================================

namespace WorldStates
{
    // Node state world states (show icon on map)
    constexpr int32 STABLES_ICON = 1842;
    constexpr int32 STABLES_ALLIANCE = 1767;
    constexpr int32 STABLES_HORDE = 1768;
    constexpr int32 STABLES_HORDE_CONTROLLED = 1769;
    constexpr int32 STABLES_ALLIANCE_CONTROLLED = 1770;

    constexpr int32 BLACKSMITH_ICON = 1846;
    constexpr int32 BLACKSMITH_ALLIANCE = 1772;
    constexpr int32 BLACKSMITH_HORDE = 1773;
    constexpr int32 BLACKSMITH_HORDE_CONTROLLED = 1774;
    constexpr int32 BLACKSMITH_ALLIANCE_CONTROLLED = 1775;

    constexpr int32 FARM_ICON = 1845;
    constexpr int32 FARM_ALLIANCE = 1801;
    constexpr int32 FARM_HORDE = 1802;
    constexpr int32 FARM_HORDE_CONTROLLED = 1803;
    constexpr int32 FARM_ALLIANCE_CONTROLLED = 1804;

    constexpr int32 GOLD_MINE_ICON = 1843;
    constexpr int32 GOLD_MINE_ALLIANCE = 1782;
    constexpr int32 GOLD_MINE_HORDE = 1783;
    constexpr int32 GOLD_MINE_HORDE_CONTROLLED = 1784;
    constexpr int32 GOLD_MINE_ALLIANCE_CONTROLLED = 1785;

    constexpr int32 LUMBER_MILL_ICON = 1844;
    constexpr int32 LUMBER_MILL_ALLIANCE = 1792;
    constexpr int32 LUMBER_MILL_HORDE = 1793;
    constexpr int32 LUMBER_MILL_HORDE_CONTROLLED = 1794;
    constexpr int32 LUMBER_MILL_ALLIANCE_CONTROLLED = 1795;

    // Resource totals
    constexpr int32 RESOURCES_ALLY = 1776;
    constexpr int32 RESOURCES_HORDE = 1777;

    // Max resources
    constexpr int32 MAX_RESOURCES = 1780;

    // Occupied bases count
    constexpr int32 OCCUPIED_BASES_ALLY = 1778;
    constexpr int32 OCCUPIED_BASES_HORDE = 1779;
}

// ============================================================================
// GAME OBJECTS
// ============================================================================

namespace GameObjects
{
    // Node banner objects
    constexpr uint32 STABLES_BANNER = 180087;
    constexpr uint32 BLACKSMITH_BANNER = 180088;
    constexpr uint32 FARM_BANNER = 180089;
    constexpr uint32 GOLD_MINE_BANNER = 180090;
    constexpr uint32 LUMBER_MILL_BANNER = 180091;

    // Aura objects (show control state)
    constexpr uint32 ALLIANCE_BANNER = 180058;
    constexpr uint32 HORDE_BANNER = 180059;
    constexpr uint32 NEUTRAL_BANNER = 180060;

    // Doors
    constexpr uint32 ALLIANCE_DOOR = 180255;
    constexpr uint32 HORDE_DOOR = 180256;
}

// ============================================================================
// SPELLS
// ============================================================================

namespace Spells
{
    constexpr uint32 HONORABLE_DEFENDER_25 = 21235;  // +25% honor when defending
    constexpr uint32 HONORABLE_DEFENDER_50 = 21236;  // +50% honor (2+ defenders)

    // Node assault/capture related
    constexpr uint32 ASSAULT_BANNER = 23932;   // Channeled spell for assaulting
}

// ============================================================================
// STRATEGY CONSTANTS
// ============================================================================

namespace Strategy
{
    // Minimum defenders per node for 3-cap strategy
    constexpr uint8 MIN_DEFENDERS_PER_NODE = 2;

    // Blacksmith always needs extra defenders
    constexpr uint8 BS_EXTRA_DEFENDERS = 2;

    // Time to rotate between nodes (milliseconds)
    constexpr uint32 ROTATION_INTERVAL = 15000;

    // Time to respond to node under attack
    constexpr uint32 DEFENSE_RESPONSE_TIME = 5000;

    // Minimum players to send for an assault
    constexpr uint8 MIN_ASSAULT_FORCE = 3;

    // Score threshold for switching to defensive
    constexpr uint32 DEFENSIVE_THRESHOLD = 1200;  // 80% of max score

    // Score threshold for desperation all-in
    constexpr uint32 DESPERATION_THRESHOLD = 300;  // Far behind
}

} // namespace Playerbot::Coordination::Battleground::ArathiBasin

#endif // PLAYERBOT_AI_COORDINATION_BG_DOMINATION_ARATHIBASINDATA_H
