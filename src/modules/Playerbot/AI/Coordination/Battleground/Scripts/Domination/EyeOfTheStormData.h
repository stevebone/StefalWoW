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

#ifndef PLAYERBOT_AI_COORDINATION_BG_DOMINATION_EYEOFTHESTORMDATA_H
#define PLAYERBOT_AI_COORDINATION_BG_DOMINATION_EYEOFTHESTORMDATA_H

#include "Define.h"
#include "Position.h"
#include <array>
#include <vector>

namespace Playerbot::Coordination::Battleground::EyeOfTheStorm
{

// ============================================================================
// MAP INFORMATION
// ============================================================================

constexpr uint32 MAP_ID = 566;
constexpr char BG_NAME[] = "Eye of the Storm";
constexpr uint32 MAX_SCORE = 1500;
constexpr uint32 MAX_DURATION = 25 * 60 * 1000;  // 25 minutes
constexpr uint8 TEAM_SIZE = 15;
constexpr uint32 NODE_COUNT = 4;
constexpr uint32 TICK_INTERVAL = 2000;
constexpr uint32 CAPTURE_TIME = 8000;

// Flag capture points (per node count) - CRITICAL for strategy decisions
constexpr uint32 FLAG_POINTS_0_NODES = 0;     // No points without nodes
constexpr uint32 FLAG_POINTS_1_NODE = 75;
constexpr uint32 FLAG_POINTS_2_NODES = 85;
constexpr uint32 FLAG_POINTS_3_NODES = 100;
constexpr uint32 FLAG_POINTS_4_NODES = 500;   // Huge bonus for 4-cap!

// Flag respawn time after capture
constexpr uint32 FLAG_RESPAWN_TIME = 7000;    // 7 seconds

// ============================================================================
// NODE IDENTIFIERS
// ============================================================================

namespace Nodes
{
    constexpr uint32 FEL_REAVER = 0;       // Horde-side (north)
    constexpr uint32 BLOOD_ELF = 1;        // Alliance-side (south)
    constexpr uint32 DRAENEI_RUINS = 2;    // Alliance-side (west) - closer to center
    constexpr uint32 MAGE_TOWER = 3;       // Horde-side (east) - closer to center
    constexpr uint32 CENTER_FLAG = 4;      // Special ID for center flag objective
}

// ============================================================================
// NODE POSITIONS
// ============================================================================

// Fel Reaver Ruins (Horde-side, north)
constexpr float FEL_REAVER_X = 2044.28f;
constexpr float FEL_REAVER_Y = 1729.68f;
constexpr float FEL_REAVER_Z = 1189.96f;
constexpr float FEL_REAVER_O = 0.0f;

// Blood Elf Tower (Alliance-side, south)
constexpr float BLOOD_ELF_X = 2048.71f;
constexpr float BLOOD_ELF_Y = 1393.65f;
constexpr float BLOOD_ELF_Z = 1194.05f;
constexpr float BLOOD_ELF_O = 0.0f;

// Draenei Ruins (Alliance-side, west) - strategic importance
constexpr float DRAENEI_RUINS_X = 2284.31f;
constexpr float DRAENEI_RUINS_Y = 1576.87f;
constexpr float DRAENEI_RUINS_Z = 1177.13f;
constexpr float DRAENEI_RUINS_O = 0.0f;

// Mage Tower (Horde-side, east) - strategic importance
constexpr float MAGE_TOWER_X = 1807.26f;
constexpr float MAGE_TOWER_Y = 1539.78f;
constexpr float MAGE_TOWER_Z = 1267.63f;
constexpr float MAGE_TOWER_O = 0.0f;

// Center flag position
constexpr float CENTER_FLAG_X = 2174.78f;
constexpr float CENTER_FLAG_Y = 1569.05f;
constexpr float CENTER_FLAG_Z = 1159.96f;
constexpr float CENTER_FLAG_O = 0.0f;

inline Position GetNodePosition(uint32 nodeId)
{
    switch (nodeId)
    {
        case Nodes::FEL_REAVER:     return Position(FEL_REAVER_X, FEL_REAVER_Y, FEL_REAVER_Z, FEL_REAVER_O);
        case Nodes::BLOOD_ELF:      return Position(BLOOD_ELF_X, BLOOD_ELF_Y, BLOOD_ELF_Z, BLOOD_ELF_O);
        case Nodes::DRAENEI_RUINS:  return Position(DRAENEI_RUINS_X, DRAENEI_RUINS_Y, DRAENEI_RUINS_Z, DRAENEI_RUINS_O);
        case Nodes::MAGE_TOWER:     return Position(MAGE_TOWER_X, MAGE_TOWER_Y, MAGE_TOWER_Z, MAGE_TOWER_O);
        default: return Position(0, 0, 0, 0);
    }
}

inline Position GetCenterFlagPosition()
{
    return Position(CENTER_FLAG_X, CENTER_FLAG_Y, CENTER_FLAG_Z, CENTER_FLAG_O);
}

inline const char* GetNodeName(uint32 nodeId)
{
    switch (nodeId)
    {
        case Nodes::FEL_REAVER:     return "Fel Reaver Ruins";
        case Nodes::BLOOD_ELF:      return "Blood Elf Tower";
        case Nodes::DRAENEI_RUINS:  return "Draenei Ruins";
        case Nodes::MAGE_TOWER:     return "Mage Tower";
        case Nodes::CENTER_FLAG:    return "Center Flag";
        default: return "Unknown";
    }
}

// Node strategic values (1-10)
// DR and MT are more valuable because they're closer to center
inline uint8 GetNodeStrategicValue(uint32 nodeId)
{
    switch (nodeId)
    {
        case Nodes::FEL_REAVER:     return 7;   // Horde home - farther from center
        case Nodes::BLOOD_ELF:      return 7;   // Alliance home - farther from center
        case Nodes::DRAENEI_RUINS:  return 9;   // Close to center - strategic
        case Nodes::MAGE_TOWER:     return 9;   // Close to center - strategic
        default: return 5;
    }
}

// ============================================================================
// TICK POINTS TABLE
// ============================================================================

constexpr std::array<uint32, 5> TICK_POINTS = {
    0,   // 0 nodes - no tick points
    1,   // 1 node
    2,   // 2 nodes
    5,   // 3 nodes
    10   // 4 nodes - full control
};

// Calculate flag capture points based on node control
inline uint32 GetFlagCaptureValue(uint32 nodeCount)
{
    switch (nodeCount)
    {
        case 0: return FLAG_POINTS_0_NODES;
        case 1: return FLAG_POINTS_1_NODE;
        case 2: return FLAG_POINTS_2_NODES;
        case 3: return FLAG_POINTS_3_NODES;
        case 4: return FLAG_POINTS_4_NODES;
        default: return 0;
    }
}

// ============================================================================
// SPAWN POSITIONS
// ============================================================================

// Alliance spawn (Draenei starting area)
constexpr Position ALLIANCE_SPAWNS[] = {
    { 2523.68f, 1596.59f, 1269.35f, 3.14f },
    { 2518.68f, 1591.59f, 1269.35f, 3.14f },
    { 2528.68f, 1601.59f, 1269.35f, 3.14f },
    { 2513.68f, 1586.59f, 1269.35f, 3.14f },
    { 2533.68f, 1606.59f, 1269.35f, 3.14f },
    { 2508.68f, 1581.59f, 1269.35f, 3.14f },
    { 2538.68f, 1611.59f, 1269.35f, 3.14f }
};

// Horde spawn (Blood Elf starting area)
constexpr Position HORDE_SPAWNS[] = {
    { 1803.73f, 1539.41f, 1267.63f, 0.0f },
    { 1808.73f, 1544.41f, 1267.63f, 0.0f },
    { 1798.73f, 1534.41f, 1267.63f, 0.0f },
    { 1813.73f, 1549.41f, 1267.63f, 0.0f },
    { 1793.73f, 1529.41f, 1267.63f, 0.0f },
    { 1818.73f, 1554.41f, 1267.63f, 0.0f },
    { 1788.73f, 1524.41f, 1267.63f, 0.0f }
};

// ============================================================================
// NODE DEFENSE POSITIONS (ENTERPRISE-GRADE)
// ============================================================================

inline std::vector<Position> GetNodeDefensePositions(uint32 nodeId)
{
    switch (nodeId)
    {
        case Nodes::FEL_REAVER:
            return {
                // Core flag defense
                { 2044.28f, 1729.68f, 1189.96f, 0.0f },   // Flag position
                { 2034.28f, 1729.68f, 1189.96f, 3.14f },  // West
                { 2054.28f, 1729.68f, 1189.96f, 0.0f },   // East
                { 2044.28f, 1719.68f, 1189.96f, 1.57f },  // South (toward center)
                { 2044.28f, 1739.68f, 1189.96f, 4.71f },  // North
                // Ramp/bridge control
                { 2055.28f, 1715.68f, 1188.96f, 0.79f },  // SE bridge to center
                { 2030.28f, 1740.68f, 1189.96f, 3.93f },  // NW corner
                // Elevated positions
                { 2050.28f, 1735.68f, 1195.96f, 5.50f }   // Platform overlook
            };
        case Nodes::BLOOD_ELF:
            return {
                // Core flag defense
                { 2048.71f, 1393.65f, 1194.05f, 0.0f },   // Flag position
                { 2038.71f, 1393.65f, 1194.05f, 3.14f },  // West
                { 2058.71f, 1393.65f, 1194.05f, 0.0f },   // East
                { 2048.71f, 1383.65f, 1194.05f, 1.57f },  // South
                { 2048.71f, 1403.65f, 1194.05f, 4.71f },  // North (toward center)
                // Tower positions
                { 2055.71f, 1408.65f, 1194.05f, 5.50f },  // NE bridge approach
                { 2040.71f, 1380.65f, 1194.05f, 2.36f },  // SW corner
                // Elevated
                { 2048.71f, 1400.65f, 1200.05f, 4.71f }   // Tower top
            };
        case Nodes::DRAENEI_RUINS:
            return {
                // Core flag defense - STRATEGIC NODE
                { 2284.31f, 1576.87f, 1177.13f, 0.0f },   // Flag position
                { 2274.31f, 1576.87f, 1177.13f, 3.14f },  // West (toward Alliance)
                { 2294.31f, 1576.87f, 1177.13f, 0.0f },   // East
                { 2284.31f, 1566.87f, 1177.13f, 1.57f },  // South
                { 2284.31f, 1586.87f, 1177.13f, 4.71f },  // North
                // Ruins archways - chokepoints
                { 2270.31f, 1560.87f, 1177.13f, 2.36f },  // SW archway
                { 2270.31f, 1590.87f, 1177.13f, 3.93f },  // NW archway
                { 2298.31f, 1570.87f, 1177.13f, 0.79f },  // E entrance (to center)
                // Elevated ruins positions
                { 2280.31f, 1582.87f, 1183.13f, 3.93f }   // Upper ruins platform
            };
        case Nodes::MAGE_TOWER:
            return {
                // Core flag defense - STRATEGIC NODE
                { 1807.26f, 1539.78f, 1267.63f, 0.0f },   // Flag position
                { 1797.26f, 1539.78f, 1267.63f, 3.14f },  // West
                { 1817.26f, 1539.78f, 1267.63f, 0.0f },   // East (toward Horde)
                { 1807.26f, 1529.78f, 1267.63f, 1.57f },  // South
                { 1807.26f, 1549.78f, 1267.63f, 4.71f },  // North
                // Tower positions (highest point in EOTS!)
                { 1795.26f, 1545.78f, 1267.63f, 3.93f },  // NW tower edge
                { 1815.26f, 1530.78f, 1267.63f, 0.79f },  // SE tower edge
                { 1800.26f, 1535.78f, 1270.63f, 2.36f },  // Inner tower
                // Bridge to center
                { 1830.26f, 1545.78f, 1260.63f, 0.0f }    // Bridge head (toward center)
            };
        default:
            return {};
    }
}

// ============================================================================
// CENTER FLAG AREA POSITIONS
// ============================================================================

// Positions around the center flag area (contested zone)
inline std::vector<Position> GetCenterFlagDefensePositions()
{
    return {
        // Flag position itself
        { CENTER_FLAG_X, CENTER_FLAG_Y, CENTER_FLAG_Z, 0.0f },

        // Inner ring (close to flag)
        { 2165.78f, 1569.05f, 1159.96f, 3.14f },  // West
        { 2183.78f, 1569.05f, 1159.96f, 0.0f },   // East
        { 2174.78f, 1559.05f, 1159.96f, 1.57f },  // South
        { 2174.78f, 1579.05f, 1159.96f, 4.71f },  // North

        // Outer ring (approach control)
        { 2155.78f, 1569.05f, 1159.96f, 3.14f },  // Far west (DR approach)
        { 2193.78f, 1569.05f, 1159.96f, 0.0f },   // Far east (MT approach)
        { 2174.78f, 1549.05f, 1159.96f, 1.57f },  // Far south (BE approach)
        { 2174.78f, 1589.05f, 1159.96f, 4.71f },  // Far north (FR approach)

        // Corner positions
        { 2160.78f, 1555.05f, 1159.96f, 2.36f },  // SW
        { 2188.78f, 1555.05f, 1159.96f, 0.79f },  // SE
        { 2160.78f, 1583.05f, 1159.96f, 3.93f },  // NW
        { 2188.78f, 1583.05f, 1159.96f, 5.50f }   // NE
    };
}

// ============================================================================
// BRIDGE POSITIONS (Critical EOTS feature!)
// ============================================================================

// The center of EOTS has bridges connecting to each node
inline std::vector<Position> GetBridgePositions()
{
    return {
        // Fel Reaver bridge (north)
        { 2090.0f, 1650.0f, 1175.0f, 4.71f },   // FR bridge mid
        { 2120.0f, 1610.0f, 1168.0f, 3.93f },   // FR bridge center-end

        // Blood Elf bridge (south)
        { 2100.0f, 1480.0f, 1175.0f, 1.57f },   // BE bridge mid
        { 2130.0f, 1520.0f, 1165.0f, 0.79f },   // BE bridge center-end

        // Draenei Ruins bridge (west)
        { 2230.0f, 1575.0f, 1170.0f, 0.0f },    // DR bridge mid
        { 2200.0f, 1572.0f, 1162.0f, 0.0f },    // DR bridge center-end

        // Mage Tower bridge (east)
        { 1870.0f, 1545.0f, 1230.0f, 3.14f },   // MT bridge mid
        { 1920.0f, 1550.0f, 1200.0f, 3.14f },   // MT bridge approach
        { 1970.0f, 1555.0f, 1175.0f, 3.14f }    // MT bridge center-end
    };
}

// ============================================================================
// FLAG RUNNING ROUTES (NODE -> CENTER -> NODE)
// ============================================================================

// Route from center flag to each node (for flag capture)
inline std::vector<Position> GetFlagRouteToNode(uint32 nodeId)
{
    Position flagStart = GetCenterFlagPosition();
    Position nodeEnd = GetNodePosition(nodeId);

    switch (nodeId)
    {
        case Nodes::FEL_REAVER:
            return {
                flagStart,
                { 2150.0f, 1590.0f, 1162.0f, 4.71f },    // Center north
                { 2120.0f, 1610.0f, 1168.0f, 4.71f },    // Bridge start
                { 2090.0f, 1650.0f, 1175.0f, 4.71f },    // Bridge mid
                { 2060.0f, 1690.0f, 1183.0f, 4.71f },    // Bridge end
                nodeEnd
            };
        case Nodes::BLOOD_ELF:
            return {
                flagStart,
                { 2150.0f, 1545.0f, 1162.0f, 1.57f },    // Center south
                { 2130.0f, 1520.0f, 1165.0f, 1.57f },    // Bridge start
                { 2100.0f, 1480.0f, 1175.0f, 1.57f },    // Bridge mid
                { 2070.0f, 1440.0f, 1185.0f, 1.57f },    // Bridge end
                nodeEnd
            };
        case Nodes::DRAENEI_RUINS:
            return {
                flagStart,
                { 2200.0f, 1572.0f, 1162.0f, 0.0f },     // Center west
                { 2230.0f, 1575.0f, 1170.0f, 0.0f },     // Bridge mid
                { 2255.0f, 1576.0f, 1175.0f, 0.0f },     // Bridge end
                nodeEnd
            };
        case Nodes::MAGE_TOWER:
            return {
                flagStart,
                { 2140.0f, 1565.0f, 1162.0f, 3.14f },    // Center east
                { 2050.0f, 1560.0f, 1168.0f, 3.14f },    // Approach
                { 1970.0f, 1555.0f, 1175.0f, 3.14f },    // Bridge
                { 1900.0f, 1548.0f, 1220.0f, 3.14f },    // Ramp up
                nodeEnd
            };
        default:
            return { flagStart, nodeEnd };
    }
}

// Optimal flag running route from center to nearest controlled node
inline std::vector<uint32> GetFlagCapturePriority(uint32 faction)
{
    if (faction == ALLIANCE)
    {
        // Alliance prefers: DR (closest) > BE > MT > FR
        return { Nodes::DRAENEI_RUINS, Nodes::BLOOD_ELF, Nodes::MAGE_TOWER, Nodes::FEL_REAVER };
    }
    else
    {
        // Horde prefers: MT (closest) > FR > DR > BE
        return { Nodes::MAGE_TOWER, Nodes::FEL_REAVER, Nodes::DRAENEI_RUINS, Nodes::BLOOD_ELF };
    }
}

// ============================================================================
// ESCORT FORMATION POSITIONS
// ============================================================================

// Positions around a flag carrier for escort duty
inline std::vector<Position> GetEscortFormation(const Position& fcPosition, uint8 escortCount)
{
    std::vector<Position> formation;
    float x = fcPosition.GetPositionX();
    float y = fcPosition.GetPositionY();
    float z = fcPosition.GetPositionZ();
    float o = fcPosition.GetOrientation();

    // Front guard (blocks incoming enemies)
    if (escortCount >= 1)
        formation.emplace_back(x + 5.0f * cos(o), y + 5.0f * sin(o), z, o);

    // Flanking guards
    if (escortCount >= 2)
        formation.emplace_back(x + 3.0f * cos(o + 1.57f), y + 3.0f * sin(o + 1.57f), z, o);
    if (escortCount >= 3)
        formation.emplace_back(x + 3.0f * cos(o - 1.57f), y + 3.0f * sin(o - 1.57f), z, o);

    // Rear guard (catches chasers)
    if (escortCount >= 4)
        formation.emplace_back(x - 5.0f * cos(o), y - 5.0f * sin(o), z, o + 3.14f);

    // Additional rear flanks
    if (escortCount >= 5)
        formation.emplace_back(x - 3.0f * cos(o + 1.57f), y - 3.0f * sin(o + 1.57f), z, o + 3.14f);
    if (escortCount >= 6)
        formation.emplace_back(x - 3.0f * cos(o - 1.57f), y - 3.0f * sin(o - 1.57f), z, o + 3.14f);

    return formation;
}

// ============================================================================
// SNIPER/ELEVATED POSITIONS
// ============================================================================

inline std::vector<Position> GetSniperPositions()
{
    return {
        // Mage Tower (HIGHEST point - best sniper spot in EOTS)
        { 1807.26f, 1539.78f, 1275.63f, 3.14f },   // MT top - sees entire map

        // Blood Elf Tower elevated
        { 2048.71f, 1393.65f, 1205.05f, 4.71f },   // BE tower top

        // Draenei Ruins upper level
        { 2280.31f, 1582.87f, 1185.13f, 0.0f },    // DR elevated platform

        // Fel Reaver platform
        { 2050.28f, 1735.68f, 1198.96f, 1.57f },   // FR elevated

        // Bridge overlooks (good for intercepting flag runners)
        { 2090.0f, 1650.0f, 1180.0f, 1.57f },      // FR bridge high point
        { 1920.0f, 1550.0f, 1210.0f, 3.14f }       // MT bridge high point
    };
}

// ============================================================================
// STRATEGIC ROUTES
// ============================================================================

inline std::vector<uint32> GetAllianceOpeningRoute()
{
    // Alliance should take Blood Elf first (home), then Draenei Ruins (strategic)
    return { Nodes::BLOOD_ELF, Nodes::DRAENEI_RUINS };
}

inline std::vector<uint32> GetHordeOpeningRoute()
{
    // Horde should take Fel Reaver first (home), then Mage Tower (strategic)
    return { Nodes::FEL_REAVER, Nodes::MAGE_TOWER };
}

// 4-cap rush routes (aggressive)
inline std::vector<uint32> GetAlliance4CapRoute()
{
    return { Nodes::BLOOD_ELF, Nodes::DRAENEI_RUINS, Nodes::FEL_REAVER, Nodes::MAGE_TOWER };
}

inline std::vector<uint32> GetHorde4CapRoute()
{
    return { Nodes::FEL_REAVER, Nodes::MAGE_TOWER, Nodes::BLOOD_ELF, Nodes::DRAENEI_RUINS };
}

// Node adjacency (for rotation planning)
inline std::vector<uint32> GetAdjacentNodes(uint32 nodeId)
{
    switch (nodeId)
    {
        case Nodes::FEL_REAVER:     return { Nodes::MAGE_TOWER, Nodes::DRAENEI_RUINS };  // Adjacent via center
        case Nodes::BLOOD_ELF:      return { Nodes::DRAENEI_RUINS, Nodes::MAGE_TOWER };
        case Nodes::DRAENEI_RUINS:  return { Nodes::BLOOD_ELF, Nodes::FEL_REAVER };
        case Nodes::MAGE_TOWER:     return { Nodes::FEL_REAVER, Nodes::BLOOD_ELF };
        default: return {};
    }
}

// Node-to-node travel distance (approximate yards via shortest path)
inline float GetNodeDistance(uint32 fromNode, uint32 toNode)
{
    static const float distances[4][4] = {
        //           FR      BE      DR      MT
        /* FR */   { 0.0f,   340.0f, 280.0f, 260.0f },
        /* BE */   { 340.0f, 0.0f,   250.0f, 270.0f },
        /* DR */   { 280.0f, 250.0f, 0.0f,   480.0f },
        /* MT */   { 260.0f, 270.0f, 480.0f, 0.0f }
    };

    if (fromNode < 4 && toNode < 4)
        return distances[fromNode][toNode];
    return 500.0f;  // Invalid/far
}

// Distance from node to center flag
inline float GetDistanceToCenter(uint32 nodeId)
{
    switch (nodeId)
    {
        case Nodes::FEL_REAVER:     return 180.0f;  // Far from center
        case Nodes::BLOOD_ELF:      return 175.0f;  // Far from center
        case Nodes::DRAENEI_RUINS:  return 110.0f;  // Close to center!
        case Nodes::MAGE_TOWER:     return 130.0f;  // Close to center
        default: return 200.0f;
    }
}

// ============================================================================
// WORLD STATES
// ============================================================================

namespace WorldStates
{
    // Node states
    constexpr int32 FEL_REAVER_ALLIANCE = 2722;
    constexpr int32 FEL_REAVER_HORDE = 2723;
    constexpr int32 FEL_REAVER_NEUTRAL = 2724;

    constexpr int32 BLOOD_ELF_ALLIANCE = 2725;
    constexpr int32 BLOOD_ELF_HORDE = 2726;
    constexpr int32 BLOOD_ELF_NEUTRAL = 2727;

    constexpr int32 DRAENEI_RUINS_ALLIANCE = 2728;
    constexpr int32 DRAENEI_RUINS_HORDE = 2729;
    constexpr int32 DRAENEI_RUINS_NEUTRAL = 2730;

    constexpr int32 MAGE_TOWER_ALLIANCE = 2731;
    constexpr int32 MAGE_TOWER_HORDE = 2732;
    constexpr int32 MAGE_TOWER_NEUTRAL = 2733;

    // Resources
    constexpr int32 RESOURCES_ALLY = 2749;
    constexpr int32 RESOURCES_HORDE = 2750;

    // Flag state
    constexpr int32 FLAG_STATE = 2757;

    // Flag state values
    constexpr int32 FLAG_STATE_NEUTRAL = 0;
    constexpr int32 FLAG_STATE_ALLIANCE_TAKEN = 1;
    constexpr int32 FLAG_STATE_HORDE_TAKEN = 2;
    constexpr int32 FLAG_STATE_WAIT_RESPAWN = 3;

    // Node counts
    constexpr int32 ALLIANCE_NODES = 2752;
    constexpr int32 HORDE_NODES = 2753;
}

// ============================================================================
// GAME OBJECTS
// ============================================================================

namespace GameObjects
{
    // Node point objects
    constexpr uint32 FEL_REAVER_TOWER_CAP = 184083;
    constexpr uint32 BLOOD_ELF_TOWER_CAP = 184082;
    constexpr uint32 DRAENEI_RUINS_CAP = 184081;
    constexpr uint32 MAGE_TOWER_CAP = 184080;

    // Center flag
    constexpr uint32 CENTER_FLAG = 184141;

    // Doors
    constexpr uint32 ALLIANCE_DOOR = 184719;
    constexpr uint32 HORDE_DOOR = 184720;
}

// ============================================================================
// SPELLS
// ============================================================================

namespace Spells
{
    // Flag-related
    constexpr uint32 NETHERSTORM_FLAG = 34976;   // Carrying the flag aura
    constexpr uint32 NETHERSTORM_FLAG_VISUAL = 35774; // Visual effect

    // Movement buffs (important for flag running)
    constexpr uint32 SPEED_BOOST = 23451;  // Speed buff from node control
}

// ============================================================================
// STRATEGY CONSTANTS
// ============================================================================

namespace Strategy
{
    // Minimum nodes before focusing on flag
    constexpr uint8 MIN_NODES_FOR_FLAG = 2;

    // Ideal nodes for flag running (massive point bonus at 4)
    constexpr uint8 IDEAL_NODES_FOR_FLAG = 3;

    // Minimum escort for flag carrier
    constexpr uint8 MIN_FLAG_ESCORT = 2;

    // Optimal escort for flag carrier
    constexpr uint8 OPTIMAL_FLAG_ESCORT = 4;

    // Defenders per node
    constexpr uint8 MIN_NODE_DEFENDERS = 2;
    constexpr uint8 STRATEGIC_NODE_DEFENDERS = 3;  // DR and MT

    // Time thresholds
    constexpr uint32 FLAG_FOCUS_TIME = 5 * 60 * 1000;  // Last 5 min - flag important

    // Score thresholds
    constexpr uint32 DEFENSIVE_THRESHOLD = 1200;  // 80% - turtle
    constexpr uint32 FLAG_RUSH_THRESHOLD = 300;   // Far behind - need flag caps
}

} // namespace Playerbot::Coordination::Battleground::EyeOfTheStorm

#endif // PLAYERBOT_AI_COORDINATION_BG_DOMINATION_EYEOFTHESTORMDATA_H
