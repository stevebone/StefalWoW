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

#ifndef PLAYERBOT_AI_COORDINATION_BG_SIEGE_ISLEOFCONQUESTDATA_H
#define PLAYERBOT_AI_COORDINATION_BG_SIEGE_ISLEOFCONQUESTDATA_H

#include "Define.h"
#include "Position.h"
#include <array>
#include <vector>
#include <cmath>

namespace Playerbot::Coordination::Battleground::IsleOfConquest
{

// ============================================================================
// CORE CONSTANTS
// ============================================================================

constexpr uint32 MAP_ID = 628;
constexpr char BG_NAME[] = "Isle of Conquest";
constexpr uint32 MAX_DURATION = 30 * 60 * 1000;  // 30 minutes
constexpr uint8 TEAM_SIZE = 40;
constexpr uint32 STARTING_REINFORCEMENTS = 300;
constexpr uint32 REINF_LOSS_PER_KILL = 1;
constexpr uint32 REINF_LOSS_PER_DEATH = 1;  // Same as per kill
constexpr uint32 REINF_LOSS_PER_TOWER = 50;

// ============================================================================
// OBJECTIVE IDENTIFIERS
// ============================================================================

namespace ObjectiveIds
{
    // Nodes (0-4)
    constexpr uint32 REFINERY = 0;      // Oil (vehicles)
    constexpr uint32 QUARRY = 1;        // Stone (siege)
    constexpr uint32 DOCKS = 2;         // Glaive Throwers/Catapults
    constexpr uint32 HANGAR = 3;        // Gunship
    constexpr uint32 WORKSHOP = 4;      // Siege Engines
    constexpr uint32 NODE_COUNT = 5;

    // Gates (50-55)
    constexpr uint32 GATE_ALLIANCE_FRONT = 50;
    constexpr uint32 GATE_ALLIANCE_WEST = 51;
    constexpr uint32 GATE_ALLIANCE_EAST = 52;
    constexpr uint32 GATE_HORDE_FRONT = 53;
    constexpr uint32 GATE_HORDE_WEST = 54;
    constexpr uint32 GATE_HORDE_EAST = 55;
    constexpr uint32 GATE_COUNT = 6;

    // Bosses (100-101)
    constexpr uint32 HALFORD = 100;
    constexpr uint32 AGMAR = 101;

    // Vehicle spawns (200+)
    constexpr uint32 VEHICLE_SPAWN_DOCKS = 200;
    constexpr uint32 VEHICLE_SPAWN_WORKSHOP = 201;
    constexpr uint32 VEHICLE_SPAWN_KEEP_ALLY = 202;
    constexpr uint32 VEHICLE_SPAWN_KEEP_HORDE = 203;
}

// ============================================================================
// STRATEGY CONSTANTS
// ============================================================================

namespace Strategy
{
    // Node management
    constexpr uint32 NODE_CAPTURE_TIME = 60000;           // 1 minute
    constexpr uint8 MIN_NODE_DEFENDERS = 3;
    constexpr uint8 NODE_ASSAULT_SIZE = 8;
    constexpr uint8 WORKSHOP_PRIORITY_DEFENDERS = 5;      // Workshop is key

    // Gate assault
    constexpr uint32 GATE_HP = 600000;                    // Gate health
    constexpr uint8 MIN_SIEGE_VEHICLES = 2;               // Minimum vehicles for gate assault
    constexpr uint8 OPTIMAL_SIEGE_VEHICLES = 4;

    // Boss assault
    constexpr uint8 MIN_BOSS_RAID_SIZE = 15;
    constexpr uint8 OPTIMAL_BOSS_RAID_SIZE = 25;

    // Reinforcement thresholds
    constexpr uint32 REINF_DESPERATE_THRESHOLD = 50;
    constexpr uint32 REINF_LOW_THRESHOLD = 100;
    constexpr uint32 REINF_ADVANTAGE_THRESHOLD = 50;

    // Team allocation
    constexpr uint8 OPENING_NODE_PERCENT = 70;
    constexpr uint8 SIEGE_VEHICLE_PERCENT = 25;
    constexpr uint8 BOSS_ASSAULT_PERCENT = 60;
    constexpr uint8 DEFENSE_PERCENT = 40;

    // Timing
    constexpr uint32 OPENING_PHASE_DURATION = 180000;     // 3 minutes
    constexpr uint32 STRATEGY_UPDATE_INTERVAL = 10000;
    constexpr uint32 GATE_CHECK_INTERVAL = 5000;
    constexpr uint32 VEHICLE_SPAWN_INTERVAL = 180000;     // 3 minutes
    constexpr uint32 NODE_CHECK_INTERVAL = 5000;          // 5 seconds
    constexpr uint32 VEHICLE_CHECK_INTERVAL = 10000;      // 10 seconds
    constexpr uint32 VEHICLE_SIEGE_START = 240000;        // 4 minutes into match
    constexpr uint32 DESPERATE_THRESHOLD = 50;            // Low reinforcements
}

// ============================================================================
// NODE DATA
// ============================================================================

namespace Nodes
{
    constexpr uint32 REFINERY = 0;
    constexpr uint32 QUARRY = 1;
    constexpr uint32 DOCKS = 2;
    constexpr uint32 HANGAR = 3;
    constexpr uint32 WORKSHOP = 4;
    constexpr uint32 NODE_COUNT = 5;
}

constexpr Position NODE_POSITIONS[] = {
    Position(1246.0f, -400.0f, 23.0f, 0.0f),    // Refinery (Horde side)
    Position(340.0f, -395.0f, 26.0f, 0.0f),     // Quarry (Alliance side)
    Position(736.0f, -1132.0f, 14.0f, 0.0f),    // Docks (South, center)
    Position(808.0f, -123.0f, 132.0f, 0.0f),    // Hangar (North, elevated)
    Position(773.0f, -685.0f, 9.0f, 0.0f)       // Workshop (Center)
};

// Position data structure for enterprise-grade access
struct NodePositionData
{
    float x, y, z, o;
    uint8 strategicValue;
};

namespace NodePositions
{
    constexpr std::array<NodePositionData, 5> POSITIONS = {{
        { 1246.0f, -400.0f, 23.0f, 0.0f, 6 },    // Refinery
        { 340.0f, -395.0f, 26.0f, 0.0f, 6 },     // Quarry
        { 736.0f, -1132.0f, 14.0f, 0.0f, 7 },    // Docks
        { 808.0f, -123.0f, 132.0f, 0.0f, 8 },    // Hangar
        { 773.0f, -685.0f, 9.0f, 0.0f, 9 }       // Workshop
    }};
}

struct GatePositionData
{
    float x, y, z, o;
};

namespace GatePositions
{
    constexpr std::array<GatePositionData, 6> POSITIONS = {{
        { 352.0f, -762.0f, 48.0f, 0.0f },        // Alliance Front
        { 228.0f, -820.0f, 48.0f, 0.0f },        // Alliance West
        { 478.0f, -820.0f, 48.0f, 0.0f },        // Alliance East
        { 1141.0f, -762.0f, 48.0f, 3.14f },      // Horde Front
        { 1016.0f, -820.0f, 48.0f, 3.14f },      // Horde West
        { 1266.0f, -820.0f, 48.0f, 3.14f }       // Horde East
    }};
}

// ============================================================================
// NODE DEFENSE POSITIONS (10 per node = 50 total)
// ============================================================================

namespace NodeDefense
{
    constexpr std::array<Position, 10> REFINERY = {{
        Position(1246.0f, -400.0f, 23.0f, 3.14f),   // Flag
        Position(1252.0f, -394.0f, 23.0f, 2.50f),
        Position(1240.0f, -394.0f, 23.0f, 3.80f),
        Position(1254.0f, -406.0f, 23.0f, 2.30f),
        Position(1238.0f, -406.0f, 23.0f, 4.00f),
        Position(1258.0f, -400.0f, 23.0f, 1.57f),
        Position(1234.0f, -400.0f, 23.0f, 4.71f),
        Position(1250.0f, -388.0f, 23.0f, 2.80f),
        Position(1242.0f, -412.0f, 23.0f, 3.50f),
        Position(1260.0f, -392.0f, 26.0f, 2.00f)    // Elevated
    }};

    constexpr std::array<Position, 10> QUARRY = {{
        Position(340.0f, -395.0f, 26.0f, 0.0f),     // Flag
        Position(346.0f, -389.0f, 26.0f, 5.50f),
        Position(334.0f, -389.0f, 26.0f, 0.80f),
        Position(348.0f, -401.0f, 26.0f, 5.30f),
        Position(332.0f, -401.0f, 26.0f, 1.00f),
        Position(352.0f, -395.0f, 26.0f, 4.71f),
        Position(328.0f, -395.0f, 26.0f, 1.57f),
        Position(344.0f, -383.0f, 26.0f, 6.00f),
        Position(336.0f, -407.0f, 26.0f, 0.50f),
        Position(354.0f, -387.0f, 29.0f, 5.00f)     // Elevated
    }};

    constexpr std::array<Position, 10> DOCKS = {{
        Position(736.0f, -1132.0f, 14.0f, 1.57f),   // Flag
        Position(742.0f, -1126.0f, 14.0f, 1.00f),
        Position(730.0f, -1126.0f, 14.0f, 2.20f),
        Position(744.0f, -1138.0f, 14.0f, 0.80f),
        Position(728.0f, -1138.0f, 14.0f, 2.40f),
        Position(750.0f, -1132.0f, 14.0f, 0.0f),
        Position(722.0f, -1132.0f, 14.0f, 3.14f),
        Position(740.0f, -1120.0f, 14.0f, 1.30f),
        Position(732.0f, -1144.0f, 14.0f, 1.90f),
        Position(752.0f, -1124.0f, 17.0f, 0.50f)    // Pier elevated
    }};

    constexpr std::array<Position, 10> HANGAR = {{
        Position(808.0f, -123.0f, 132.0f, 4.71f),   // Flag
        Position(814.0f, -117.0f, 132.0f, 4.20f),
        Position(802.0f, -117.0f, 132.0f, 5.20f),
        Position(816.0f, -129.0f, 132.0f, 4.00f),
        Position(800.0f, -129.0f, 132.0f, 5.40f),
        Position(820.0f, -123.0f, 132.0f, 3.14f),
        Position(796.0f, -123.0f, 132.0f, 0.0f),
        Position(812.0f, -111.0f, 132.0f, 4.50f),
        Position(804.0f, -135.0f, 132.0f, 5.00f),
        Position(822.0f, -115.0f, 135.0f, 3.50f)    // Platform elevated
    }};

    constexpr std::array<Position, 10> WORKSHOP = {{
        Position(773.0f, -685.0f, 9.0f, 1.57f),     // Flag
        Position(779.0f, -679.0f, 9.0f, 1.00f),
        Position(767.0f, -679.0f, 9.0f, 2.20f),
        Position(781.0f, -691.0f, 9.0f, 0.80f),
        Position(765.0f, -691.0f, 9.0f, 2.40f),
        Position(787.0f, -685.0f, 9.0f, 0.0f),
        Position(759.0f, -685.0f, 9.0f, 3.14f),
        Position(777.0f, -673.0f, 9.0f, 1.30f),
        Position(769.0f, -697.0f, 9.0f, 1.90f),
        Position(789.0f, -677.0f, 12.0f, 0.50f)     // Ramp elevated
    }};
}

// ============================================================================
// GATE DATA
// ============================================================================

namespace Gates
{
    constexpr uint32 ALLIANCE_FRONT = 0;
    constexpr uint32 ALLIANCE_WEST = 1;
    constexpr uint32 ALLIANCE_EAST = 2;
    constexpr uint32 HORDE_FRONT = 3;
    constexpr uint32 HORDE_WEST = 4;
    constexpr uint32 HORDE_EAST = 5;
    constexpr uint32 GATE_COUNT = 6;
}

constexpr Position GATE_POSITIONS[] = {
    Position(352.0f, -762.0f, 48.0f, 0.0f),     // Alliance Front
    Position(228.0f, -820.0f, 48.0f, 0.0f),     // Alliance West
    Position(478.0f, -820.0f, 48.0f, 0.0f),     // Alliance East
    Position(1141.0f, -762.0f, 48.0f, 3.14f),   // Horde Front
    Position(1016.0f, -820.0f, 48.0f, 3.14f),   // Horde West
    Position(1266.0f, -820.0f, 48.0f, 3.14f)    // Horde East
};

// ============================================================================
// GATE APPROACH POSITIONS (8 per gate = 48 total)
// ============================================================================

namespace GateApproach
{
    // Alliance Front Gate approach (for Horde attackers)
    constexpr std::array<Position, 8> ALLIANCE_FRONT = {{
        Position(380.0f, -762.0f, 48.0f, 3.14f),    // Siege approach center
        Position(375.0f, -752.0f, 48.0f, 3.50f),    // Siege approach left
        Position(375.0f, -772.0f, 48.0f, 2.80f),    // Siege approach right
        Position(390.0f, -755.0f, 48.0f, 3.80f),    // Infantry left
        Position(390.0f, -769.0f, 48.0f, 2.50f),    // Infantry right
        Position(400.0f, -762.0f, 48.0f, 3.14f),    // Ranged center
        Position(395.0f, -745.0f, 50.0f, 4.00f),    // Elevated left
        Position(395.0f, -779.0f, 50.0f, 2.30f)     // Elevated right
    }};

    // Alliance West Gate approach
    constexpr std::array<Position, 8> ALLIANCE_WEST = {{
        Position(256.0f, -820.0f, 48.0f, 3.14f),
        Position(251.0f, -810.0f, 48.0f, 3.50f),
        Position(251.0f, -830.0f, 48.0f, 2.80f),
        Position(266.0f, -813.0f, 48.0f, 3.80f),
        Position(266.0f, -827.0f, 48.0f, 2.50f),
        Position(276.0f, -820.0f, 48.0f, 3.14f),
        Position(271.0f, -803.0f, 50.0f, 4.00f),
        Position(271.0f, -837.0f, 50.0f, 2.30f)
    }};

    // Alliance East Gate approach
    constexpr std::array<Position, 8> ALLIANCE_EAST = {{
        Position(506.0f, -820.0f, 48.0f, 3.14f),
        Position(501.0f, -810.0f, 48.0f, 3.50f),
        Position(501.0f, -830.0f, 48.0f, 2.80f),
        Position(516.0f, -813.0f, 48.0f, 3.80f),
        Position(516.0f, -827.0f, 48.0f, 2.50f),
        Position(526.0f, -820.0f, 48.0f, 3.14f),
        Position(521.0f, -803.0f, 50.0f, 4.00f),
        Position(521.0f, -837.0f, 50.0f, 2.30f)
    }};

    // Horde Front Gate approach (for Alliance attackers)
    constexpr std::array<Position, 8> HORDE_FRONT = {{
        Position(1113.0f, -762.0f, 48.0f, 0.0f),
        Position(1118.0f, -752.0f, 48.0f, 5.80f),
        Position(1118.0f, -772.0f, 48.0f, 0.50f),
        Position(1103.0f, -755.0f, 48.0f, 5.50f),
        Position(1103.0f, -769.0f, 48.0f, 0.80f),
        Position(1093.0f, -762.0f, 48.0f, 0.0f),
        Position(1098.0f, -745.0f, 50.0f, 5.30f),
        Position(1098.0f, -779.0f, 50.0f, 1.00f)
    }};

    // Horde West Gate approach
    constexpr std::array<Position, 8> HORDE_WEST = {{
        Position(988.0f, -820.0f, 48.0f, 0.0f),
        Position(993.0f, -810.0f, 48.0f, 5.80f),
        Position(993.0f, -830.0f, 48.0f, 0.50f),
        Position(978.0f, -813.0f, 48.0f, 5.50f),
        Position(978.0f, -827.0f, 48.0f, 0.80f),
        Position(968.0f, -820.0f, 48.0f, 0.0f),
        Position(973.0f, -803.0f, 50.0f, 5.30f),
        Position(973.0f, -837.0f, 50.0f, 1.00f)
    }};

    // Horde East Gate approach
    constexpr std::array<Position, 8> HORDE_EAST = {{
        Position(1238.0f, -820.0f, 48.0f, 0.0f),
        Position(1243.0f, -810.0f, 48.0f, 5.80f),
        Position(1243.0f, -830.0f, 48.0f, 0.50f),
        Position(1228.0f, -813.0f, 48.0f, 5.50f),
        Position(1228.0f, -827.0f, 48.0f, 0.80f),
        Position(1218.0f, -820.0f, 48.0f, 0.0f),
        Position(1223.0f, -803.0f, 50.0f, 5.30f),
        Position(1223.0f, -837.0f, 50.0f, 1.00f)
    }};
}

// ============================================================================
// BOSS DATA
// ============================================================================

namespace Bosses
{
    // Alliance Boss - High Commander Halford Wyrmbane
    constexpr uint32 HALFORD_ENTRY = 34924;
    constexpr float HALFORD_X = 224.0f;
    constexpr float HALFORD_Y = -836.0f;
    constexpr float HALFORD_Z = 60.0f;
    constexpr float HALFORD_O = 0.0f;

    // Horde Boss - Overlord Agmar
    constexpr uint32 AGMAR_ENTRY = 34922;
    constexpr float AGMAR_X = 1296.0f;
    constexpr float AGMAR_Y = -765.0f;
    constexpr float AGMAR_Z = 70.0f;
    constexpr float AGMAR_O = 3.14f;
}

constexpr uint32 HIGH_COMMANDER_HALFORD = Bosses::HALFORD_ENTRY;
constexpr uint32 OVERLORD_AGMAR = Bosses::AGMAR_ENTRY;

constexpr float HALFORD_X = Bosses::HALFORD_X;
constexpr float HALFORD_Y = Bosses::HALFORD_Y;
constexpr float HALFORD_Z = Bosses::HALFORD_Z;
constexpr float AGMAR_X = Bosses::AGMAR_X;
constexpr float AGMAR_Y = Bosses::AGMAR_Y;
constexpr float AGMAR_Z = Bosses::AGMAR_Z;

// ============================================================================
// BOSS ROOM POSITIONS (12 per boss = 24 total)
// ============================================================================

// Defense position data structure
struct DefensePositionData
{
    float x, y, z, o;
};

// Node defense positions as arrays of structured data
namespace NodeDefense
{
    constexpr std::array<DefensePositionData, 10> POSITIONS[] = {
        // REFINERY
        {{
            { 1246.0f, -400.0f, 23.0f, 3.14f },
            { 1252.0f, -394.0f, 23.0f, 2.50f },
            { 1240.0f, -394.0f, 23.0f, 3.80f },
            { 1254.0f, -406.0f, 23.0f, 2.30f },
            { 1238.0f, -406.0f, 23.0f, 4.00f },
            { 1258.0f, -400.0f, 23.0f, 1.57f },
            { 1234.0f, -400.0f, 23.0f, 4.71f },
            { 1250.0f, -388.0f, 23.0f, 2.80f },
            { 1242.0f, -412.0f, 23.0f, 3.50f },
            { 1260.0f, -392.0f, 26.0f, 2.00f }
        }},
        // QUARRY
        {{
            { 340.0f, -395.0f, 26.0f, 0.0f },
            { 346.0f, -389.0f, 26.0f, 5.50f },
            { 334.0f, -389.0f, 26.0f, 0.80f },
            { 348.0f, -401.0f, 26.0f, 5.30f },
            { 332.0f, -401.0f, 26.0f, 1.00f },
            { 352.0f, -395.0f, 26.0f, 4.71f },
            { 328.0f, -395.0f, 26.0f, 1.57f },
            { 344.0f, -383.0f, 26.0f, 6.00f },
            { 336.0f, -407.0f, 26.0f, 0.50f },
            { 354.0f, -387.0f, 29.0f, 5.00f }
        }},
        // DOCKS
        {{
            { 736.0f, -1132.0f, 14.0f, 1.57f },
            { 742.0f, -1126.0f, 14.0f, 1.00f },
            { 730.0f, -1126.0f, 14.0f, 2.20f },
            { 744.0f, -1138.0f, 14.0f, 0.80f },
            { 728.0f, -1138.0f, 14.0f, 2.40f },
            { 750.0f, -1132.0f, 14.0f, 0.0f },
            { 722.0f, -1132.0f, 14.0f, 3.14f },
            { 740.0f, -1120.0f, 14.0f, 1.30f },
            { 732.0f, -1144.0f, 14.0f, 1.90f },
            { 752.0f, -1124.0f, 17.0f, 0.50f }
        }},
        // HANGAR
        {{
            { 808.0f, -123.0f, 132.0f, 4.71f },
            { 814.0f, -117.0f, 132.0f, 4.20f },
            { 802.0f, -117.0f, 132.0f, 5.20f },
            { 816.0f, -129.0f, 132.0f, 4.00f },
            { 800.0f, -129.0f, 132.0f, 5.40f },
            { 820.0f, -123.0f, 132.0f, 3.14f },
            { 796.0f, -123.0f, 132.0f, 0.0f },
            { 812.0f, -111.0f, 132.0f, 4.50f },
            { 804.0f, -135.0f, 132.0f, 5.00f },
            { 822.0f, -115.0f, 135.0f, 3.50f }
        }},
        // WORKSHOP
        {{
            { 773.0f, -685.0f, 9.0f, 1.57f },
            { 779.0f, -679.0f, 9.0f, 1.00f },
            { 767.0f, -679.0f, 9.0f, 2.20f },
            { 781.0f, -691.0f, 9.0f, 0.80f },
            { 765.0f, -691.0f, 9.0f, 2.40f },
            { 787.0f, -685.0f, 9.0f, 0.0f },
            { 759.0f, -685.0f, 9.0f, 3.14f },
            { 777.0f, -673.0f, 9.0f, 1.30f },
            { 769.0f, -697.0f, 9.0f, 1.90f },
            { 789.0f, -677.0f, 12.0f, 0.50f }
        }}
    };
}

// Gate approach positions as arrays
namespace GateApproach
{
    constexpr std::array<DefensePositionData, 8> POSITIONS[] = {
        // ALLIANCE_FRONT
        {{
            { 380.0f, -762.0f, 48.0f, 3.14f },
            { 375.0f, -752.0f, 48.0f, 3.50f },
            { 375.0f, -772.0f, 48.0f, 2.80f },
            { 390.0f, -755.0f, 48.0f, 3.80f },
            { 390.0f, -769.0f, 48.0f, 2.50f },
            { 400.0f, -762.0f, 48.0f, 3.14f },
            { 395.0f, -745.0f, 50.0f, 4.00f },
            { 395.0f, -779.0f, 50.0f, 2.30f }
        }},
        // ALLIANCE_WEST
        {{
            { 256.0f, -820.0f, 48.0f, 3.14f },
            { 251.0f, -810.0f, 48.0f, 3.50f },
            { 251.0f, -830.0f, 48.0f, 2.80f },
            { 266.0f, -813.0f, 48.0f, 3.80f },
            { 266.0f, -827.0f, 48.0f, 2.50f },
            { 276.0f, -820.0f, 48.0f, 3.14f },
            { 271.0f, -803.0f, 50.0f, 4.00f },
            { 271.0f, -837.0f, 50.0f, 2.30f }
        }},
        // ALLIANCE_EAST
        {{
            { 506.0f, -820.0f, 48.0f, 3.14f },
            { 501.0f, -810.0f, 48.0f, 3.50f },
            { 501.0f, -830.0f, 48.0f, 2.80f },
            { 516.0f, -813.0f, 48.0f, 3.80f },
            { 516.0f, -827.0f, 48.0f, 2.50f },
            { 526.0f, -820.0f, 48.0f, 3.14f },
            { 521.0f, -803.0f, 50.0f, 4.00f },
            { 521.0f, -837.0f, 50.0f, 2.30f }
        }},
        // HORDE_FRONT
        {{
            { 1113.0f, -762.0f, 48.0f, 0.0f },
            { 1118.0f, -752.0f, 48.0f, 5.80f },
            { 1118.0f, -772.0f, 48.0f, 0.50f },
            { 1103.0f, -755.0f, 48.0f, 5.50f },
            { 1103.0f, -769.0f, 48.0f, 0.80f },
            { 1093.0f, -762.0f, 48.0f, 0.0f },
            { 1098.0f, -745.0f, 50.0f, 5.30f },
            { 1098.0f, -779.0f, 50.0f, 1.00f }
        }},
        // HORDE_WEST
        {{
            { 988.0f, -820.0f, 48.0f, 0.0f },
            { 993.0f, -810.0f, 48.0f, 5.80f },
            { 993.0f, -830.0f, 48.0f, 0.50f },
            { 978.0f, -813.0f, 48.0f, 5.50f },
            { 978.0f, -827.0f, 48.0f, 0.80f },
            { 968.0f, -820.0f, 48.0f, 0.0f },
            { 973.0f, -803.0f, 50.0f, 5.30f },
            { 973.0f, -837.0f, 50.0f, 1.00f }
        }},
        // HORDE_EAST
        {{
            { 1238.0f, -820.0f, 48.0f, 0.0f },
            { 1243.0f, -810.0f, 48.0f, 5.80f },
            { 1243.0f, -830.0f, 48.0f, 0.50f },
            { 1228.0f, -813.0f, 48.0f, 5.50f },
            { 1228.0f, -827.0f, 48.0f, 0.80f },
            { 1218.0f, -820.0f, 48.0f, 0.0f },
            { 1223.0f, -803.0f, 50.0f, 5.30f },
            { 1223.0f, -837.0f, 50.0f, 1.00f }
        }}
    };
}

// Boss room positions namespace
namespace BossRoom
{
    constexpr std::array<DefensePositionData, 12> HALFORD_ROOM = {{
        { 232.0f, -836.0f, 60.0f, 3.14f },
        { 228.0f, -830.0f, 60.0f, 3.50f },
        { 236.0f, -830.0f, 60.0f, 2.80f },
        { 226.0f, -842.0f, 60.0f, 3.80f },
        { 238.0f, -842.0f, 60.0f, 2.50f },
        { 220.0f, -826.0f, 60.0f, 4.00f },
        { 244.0f, -826.0f, 60.0f, 2.20f },
        { 216.0f, -840.0f, 60.0f, 3.14f },
        { 248.0f, -840.0f, 60.0f, 3.14f },
        { 232.0f, -852.0f, 60.0f, 3.14f },
        { 212.0f, -848.0f, 60.0f, 3.50f },
        { 252.0f, -848.0f, 60.0f, 2.80f }
    }};

    constexpr std::array<DefensePositionData, 12> AGMAR_ROOM = {{
        { 1288.0f, -765.0f, 70.0f, 0.0f },
        { 1292.0f, -759.0f, 70.0f, 0.50f },
        { 1284.0f, -759.0f, 70.0f, 5.80f },
        { 1294.0f, -771.0f, 70.0f, 0.80f },
        { 1282.0f, -771.0f, 70.0f, 5.50f },
        { 1300.0f, -755.0f, 70.0f, 1.00f },
        { 1276.0f, -755.0f, 70.0f, 5.30f },
        { 1304.0f, -769.0f, 70.0f, 0.0f },
        { 1272.0f, -769.0f, 70.0f, 0.0f },
        { 1288.0f, -781.0f, 70.0f, 0.0f },
        { 1308.0f, -777.0f, 70.0f, 5.80f },
        { 1268.0f, -777.0f, 70.0f, 0.50f }
    }};
}

namespace BossRoomPositions
{
    // Halford raid positions (Alliance Keep)
    constexpr std::array<Position, 12> HALFORD_RAID = {{
        Position(232.0f, -836.0f, 60.0f, 3.14f),    // Main tank
        Position(228.0f, -830.0f, 60.0f, 3.50f),    // Off tank
        Position(236.0f, -830.0f, 60.0f, 2.80f),    // Melee 1
        Position(226.0f, -842.0f, 60.0f, 3.80f),    // Melee 2
        Position(238.0f, -842.0f, 60.0f, 2.50f),    // Melee 3
        Position(220.0f, -826.0f, 60.0f, 4.00f),    // Melee 4
        Position(244.0f, -826.0f, 60.0f, 2.20f),    // Melee 5
        Position(216.0f, -840.0f, 60.0f, 3.14f),    // Ranged left
        Position(248.0f, -840.0f, 60.0f, 3.14f),    // Ranged right
        Position(232.0f, -852.0f, 60.0f, 3.14f),    // Ranged back
        Position(212.0f, -848.0f, 60.0f, 3.50f),    // Healer left
        Position(252.0f, -848.0f, 60.0f, 2.80f)     // Healer right
    }};

    // Agmar raid positions (Horde Keep)
    constexpr std::array<Position, 12> AGMAR_RAID = {{
        Position(1288.0f, -765.0f, 70.0f, 0.0f),    // Main tank
        Position(1292.0f, -759.0f, 70.0f, 0.50f),   // Off tank
        Position(1284.0f, -759.0f, 70.0f, 5.80f),   // Melee 1
        Position(1294.0f, -771.0f, 70.0f, 0.80f),   // Melee 2
        Position(1282.0f, -771.0f, 70.0f, 5.50f),   // Melee 3
        Position(1300.0f, -755.0f, 70.0f, 1.00f),   // Melee 4
        Position(1276.0f, -755.0f, 70.0f, 5.30f),   // Melee 5
        Position(1304.0f, -769.0f, 70.0f, 0.0f),    // Ranged left
        Position(1272.0f, -769.0f, 70.0f, 0.0f),    // Ranged right
        Position(1288.0f, -781.0f, 70.0f, 0.0f),    // Ranged back
        Position(1308.0f, -777.0f, 70.0f, 5.80f),   // Healer left
        Position(1268.0f, -777.0f, 70.0f, 0.50f)    // Healer right
    }};
}

// ============================================================================
// SPAWN POSITIONS
// ============================================================================

constexpr Position ALLIANCE_SPAWNS[] = {
    Position(303.0f, -857.0f, 48.0f, 1.57f),
    Position(345.0f, -857.0f, 48.0f, 1.57f),
    Position(387.0f, -857.0f, 48.0f, 1.57f),
    Position(324.0f, -865.0f, 48.0f, 1.57f),
    Position(366.0f, -865.0f, 48.0f, 1.57f)
};

constexpr Position HORDE_SPAWNS[] = {
    Position(1147.0f, -745.0f, 48.0f, 4.71f),
    Position(1189.0f, -745.0f, 48.0f, 4.71f),
    Position(1231.0f, -745.0f, 48.0f, 4.71f),
    Position(1168.0f, -737.0f, 48.0f, 4.71f),
    Position(1210.0f, -737.0f, 48.0f, 4.71f)
};

// ============================================================================
// STRATEGIC POSITIONS
// ============================================================================

// Strategic position data structure with metadata
struct StrategicPositionData
{
    const char* name;
    float x, y, z;
    uint8 strategicValue;  // 1-10 importance rating
};

namespace StrategicPositions
{
    // Chokepoints with metadata
    constexpr std::array<StrategicPositionData, 12> CHOKEPOINTS = {{
        { "Workshop Center", 773.0f, -685.0f, 9.0f, 9 },
        { "Workshop North Path", 793.0f, -480.0f, 12.0f, 7 },
        { "Workshop South Path", 793.0f, -890.0f, 12.0f, 7 },
        { "West Midfield", 500.0f, -685.0f, 20.0f, 6 },
        { "East Midfield", 1000.0f, -685.0f, 20.0f, 6 },
        { "Alliance Keep Approach", 352.0f, -720.0f, 40.0f, 8 },
        { "Horde Keep Approach", 1141.0f, -720.0f, 40.0f, 8 },
        { "Quarry-Hangar Path", 550.0f, -500.0f, 25.0f, 5 },
        { "Refinery-Hangar Path", 960.0f, -500.0f, 25.0f, 5 },
        { "Quarry-Docks Path", 600.0f, -950.0f, 18.0f, 5 },
        { "Refinery-Docks Path", 900.0f, -950.0f, 18.0f, 5 },
        { "Hangar Platform", 793.0f, -123.0f, 100.0f, 8 }
    }};

    // Sniper positions with metadata
    constexpr std::array<StrategicPositionData, 8> SNIPER_POSITIONS = {{
        { "Hangar Tower", 808.0f, -150.0f, 140.0f, 9 },
        { "Alliance Keep Wall", 352.0f, -740.0f, 60.0f, 8 },
        { "Horde Keep Wall", 1141.0f, -740.0f, 60.0f, 8 },
        { "Workshop Overlook", 773.0f, -660.0f, 15.0f, 7 },
        { "Docks Overlook", 740.0f, -1100.0f, 20.0f, 7 },
        { "Quarry Hill", 340.0f, -370.0f, 35.0f, 6 },
        { "Refinery Hill", 1246.0f, -375.0f, 30.0f, 6 },
        { "Mid-Map Elevation", 793.0f, -300.0f, 50.0f, 7 }
    }};

    // Vehicle staging areas
    constexpr std::array<DefensePositionData, 6> ALLIANCE_VEHICLE_STAGING = {{
        { 280.0f, -850.0f, 48.0f, 0.0f },
        { 290.0f, -850.0f, 48.0f, 0.0f },
        { 300.0f, -850.0f, 48.0f, 0.0f },
        { 285.0f, -860.0f, 48.0f, 0.0f },
        { 295.0f, -860.0f, 48.0f, 0.0f },
        { 305.0f, -860.0f, 48.0f, 0.0f }
    }};

    constexpr std::array<DefensePositionData, 6> HORDE_VEHICLE_STAGING = {{
        { 1190.0f, -750.0f, 48.0f, 0.0f },
        { 1200.0f, -750.0f, 48.0f, 0.0f },
        { 1210.0f, -750.0f, 48.0f, 0.0f },
        { 1195.0f, -740.0f, 48.0f, 0.0f },
        { 1205.0f, -740.0f, 48.0f, 0.0f },
        { 1215.0f, -740.0f, 48.0f, 0.0f }
    }};

    // Ambush positions for each faction
    constexpr std::array<DefensePositionData, 6> ALLIANCE_AMBUSH = {{
        { 450.0f, -720.0f, 30.0f, 0.0f },
        { 500.0f, -550.0f, 25.0f, 0.0f },
        { 500.0f, -850.0f, 25.0f, 0.0f },
        { 650.0f, -685.0f, 15.0f, 0.0f },
        { 600.0f, -400.0f, 30.0f, 0.0f },
        { 600.0f, -1000.0f, 20.0f, 0.0f }
    }};

    constexpr std::array<DefensePositionData, 6> HORDE_AMBUSH = {{
        { 1050.0f, -720.0f, 30.0f, 0.0f },
        { 1000.0f, -550.0f, 25.0f, 0.0f },
        { 1000.0f, -850.0f, 25.0f, 0.0f },
        { 900.0f, -685.0f, 15.0f, 0.0f },
        { 950.0f, -400.0f, 30.0f, 0.0f },
        { 950.0f, -1000.0f, 20.0f, 0.0f }
    }};

    // Parachute drop positions (for Hangar control)
    constexpr std::array<DefensePositionData, 4> ALLIANCE_PARACHUTE_DROP = {{
        { 240.0f, -840.0f, 65.0f, 0.0f },
        { 220.0f, -830.0f, 65.0f, 0.0f },
        { 260.0f, -830.0f, 65.0f, 0.0f },
        { 240.0f, -820.0f, 65.0f, 0.0f }
    }};

    constexpr std::array<DefensePositionData, 4> HORDE_PARACHUTE_DROP = {{
        { 1280.0f, -765.0f, 75.0f, 0.0f },
        { 1260.0f, -755.0f, 75.0f, 0.0f },
        { 1300.0f, -755.0f, 75.0f, 0.0f },
        { 1280.0f, -745.0f, 75.0f, 0.0f }
    }};
}

// ============================================================================
// VEHICLE DATA
// ============================================================================

namespace Vehicles
{
    constexpr uint32 GLAIVE_THROWER = 34802;
    constexpr uint32 CATAPULT = 34793;
    constexpr uint32 DEMOLISHER = 34775;
    constexpr uint32 SIEGE_ENGINE = 34776;        // Generic (use SIEGE_ENGINE_A/H for specific)
    constexpr uint32 SIEGE_ENGINE_A = 34776;      // Alliance
    constexpr uint32 SIEGE_ENGINE_H = 34777;      // Horde
    constexpr uint32 GUNSHIP = 0;                 // Not a vehicle you enter

    // Vehicle stats
    constexpr uint32 GLAIVE_HP = 100000;
    constexpr uint32 CATAPULT_HP = 75000;
    constexpr uint32 DEMOLISHER_HP = 150000;
    constexpr uint32 SIEGE_ENGINE_HP = 500000;

    // Damage vs gates
    constexpr uint32 GLAIVE_GATE_DAMAGE = 2000;
    constexpr uint32 CATAPULT_GATE_DAMAGE = 1500;
    constexpr uint32 DEMOLISHER_GATE_DAMAGE = 5000;
    constexpr uint32 SIEGE_ENGINE_GATE_DAMAGE = 10000;
}

// ============================================================================
// WORLD STATES
// ============================================================================

namespace WorldStates
{
    // Reinforcements
    constexpr int32 REINF_ALLY = 4221;
    constexpr int32 REINF_HORDE = 4222;

    // Node states - Alliance controlled
    constexpr int32 REFINERY_ALLY = 4287;
    constexpr int32 QUARRY_ALLY = 4289;
    constexpr int32 DOCKS_ALLY = 4291;
    constexpr int32 HANGAR_ALLY = 4293;
    constexpr int32 WORKSHOP_ALLY = 4295;

    // Node states - Horde controlled
    constexpr int32 REFINERY_HORDE = 4288;
    constexpr int32 QUARRY_HORDE = 4290;
    constexpr int32 DOCKS_HORDE = 4292;
    constexpr int32 HANGAR_HORDE = 4294;
    constexpr int32 WORKSHOP_HORDE = 4296;

    // Node states - Neutral (using estimated values)
    constexpr int32 REFINERY_NEUTRAL = 4297;
    constexpr int32 QUARRY_NEUTRAL = 4298;
    constexpr int32 DOCKS_NEUTRAL = 4299;
    constexpr int32 HANGAR_NEUTRAL = 4300;
    constexpr int32 WORKSHOP_NEUTRAL = 4301;

    // Gate states
    constexpr int32 GATE_A_FRONT = 4317;
    constexpr int32 GATE_A_WEST = 4318;
    constexpr int32 GATE_A_EAST = 4319;
    constexpr int32 GATE_H_FRONT = 4320;
    constexpr int32 GATE_H_WEST = 4321;
    constexpr int32 GATE_H_EAST = 4322;
}

// Node world state helper structure
struct NodeWorldStateInfo
{
    int32 allyControlled;
    int32 hordeControlled;
};

constexpr std::array<NodeWorldStateInfo, 5> NODE_WORLD_STATES = {{
    { WorldStates::REFINERY_ALLY, WorldStates::REFINERY_HORDE },
    { WorldStates::QUARRY_ALLY, WorldStates::QUARRY_HORDE },
    { WorldStates::DOCKS_ALLY, WorldStates::DOCKS_HORDE },
    { WorldStates::HANGAR_ALLY, WorldStates::HANGAR_HORDE },
    { WorldStates::WORKSHOP_ALLY, WorldStates::WORKSHOP_HORDE }
}};

// Gate world states array
constexpr std::array<int32, 6> GATE_WORLD_STATES = {{
    WorldStates::GATE_A_FRONT,
    WorldStates::GATE_A_WEST,
    WorldStates::GATE_A_EAST,
    WorldStates::GATE_H_FRONT,
    WorldStates::GATE_H_WEST,
    WorldStates::GATE_H_EAST
}};

// ============================================================================
// DISTANCE MATRIX
// ============================================================================

namespace DistanceMatrix
{
    // Spawn to key objectives
    constexpr float ALLY_TO_WORKSHOP = 350.0f;
    constexpr float HORDE_TO_WORKSHOP = 350.0f;
    constexpr float ALLY_TO_HANGAR = 750.0f;
    constexpr float HORDE_TO_HANGAR = 750.0f;
    constexpr float ALLY_TO_DOCKS = 480.0f;
    constexpr float HORDE_TO_DOCKS = 480.0f;
    constexpr float ALLY_TO_QUARRY = 120.0f;
    constexpr float HORDE_TO_REFINERY = 120.0f;

    // Boss distances
    constexpr float WORKSHOP_TO_ALLY_GATE = 420.0f;
    constexpr float WORKSHOP_TO_HORDE_GATE = 420.0f;

    // Map dimensions
    constexpr float MAP_WIDTH = 1400.0f;
    constexpr float MAP_HEIGHT = 1100.0f;
}

// ============================================================================
// SIEGE ROUTES
// ============================================================================

inline std::vector<Position> GetAllianceSiegeRoute()
{
    return {
        Position(773.0f, -685.0f, 9.0f, 0.0f),      // Workshop
        Position(950.0f, -685.0f, 15.0f, 0.0f),     // East mid
        Position(1050.0f, -720.0f, 30.0f, 0.0f),    // Horde keep approach
        Position(1141.0f, -762.0f, 48.0f, 0.0f)     // Horde front gate
    };
}

inline std::vector<Position> GetHordeSiegeRoute()
{
    return {
        Position(773.0f, -685.0f, 9.0f, 0.0f),      // Workshop
        Position(550.0f, -685.0f, 15.0f, 0.0f),     // West mid
        Position(450.0f, -720.0f, 30.0f, 0.0f),     // Alliance keep approach
        Position(352.0f, -762.0f, 48.0f, 0.0f)      // Alliance front gate
    };
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

inline Position GetNodePosition(uint32 nodeId)
{
    if (nodeId < Nodes::NODE_COUNT)
        return NODE_POSITIONS[nodeId];
    return Position(0, 0, 0, 0);
}

inline Position GetGatePosition(uint32 gateId)
{
    if (gateId < Gates::GATE_COUNT)
        return GATE_POSITIONS[gateId];
    return Position(0, 0, 0, 0);
}

inline const char* GetNodeName(uint32 nodeId)
{
    switch (nodeId)
    {
        case Nodes::REFINERY: return "Refinery";
        case Nodes::QUARRY: return "Quarry";
        case Nodes::DOCKS: return "Docks";
        case Nodes::HANGAR: return "Hangar";
        case Nodes::WORKSHOP: return "Workshop";
        default: return "Unknown";
    }
}

inline const char* GetGateName(uint32 gateId)
{
    switch (gateId)
    {
        case Gates::ALLIANCE_FRONT: return "Alliance Front Gate";
        case Gates::ALLIANCE_WEST: return "Alliance West Gate";
        case Gates::ALLIANCE_EAST: return "Alliance East Gate";
        case Gates::HORDE_FRONT: return "Horde Front Gate";
        case Gates::HORDE_WEST: return "Horde West Gate";
        case Gates::HORDE_EAST: return "Horde East Gate";
        default: return "Unknown Gate";
    }
}

inline bool IsAllianceGate(uint32 gateId)
{
    return gateId <= Gates::ALLIANCE_EAST;
}

inline bool IsHordeGate(uint32 gateId)
{
    return gateId >= Gates::HORDE_FRONT && gateId <= Gates::HORDE_EAST;
}

inline Position GetHalfordPosition()
{
    return Position(Bosses::HALFORD_X, Bosses::HALFORD_Y, Bosses::HALFORD_Z, Bosses::HALFORD_O);
}

inline Position GetAgmarPosition()
{
    return Position(Bosses::AGMAR_X, Bosses::AGMAR_Y, Bosses::AGMAR_Z, Bosses::AGMAR_O);
}

inline std::vector<uint32> GetVehiclesFromNode(uint32 nodeId)
{
    switch (nodeId)
    {
        case Nodes::DOCKS: return { Vehicles::GLAIVE_THROWER, Vehicles::CATAPULT };
        case Nodes::WORKSHOP: return { Vehicles::DEMOLISHER, Vehicles::SIEGE_ENGINE_A };
        default: return {};
    }
}

inline std::vector<Position> GetNodeDefensePositions(uint32 nodeId)
{
    std::vector<Position> positions;
    switch (nodeId)
    {
        case Nodes::REFINERY:
            for (const auto& pos : NodeDefense::REFINERY) positions.push_back(pos);
            break;
        case Nodes::QUARRY:
            for (const auto& pos : NodeDefense::QUARRY) positions.push_back(pos);
            break;
        case Nodes::DOCKS:
            for (const auto& pos : NodeDefense::DOCKS) positions.push_back(pos);
            break;
        case Nodes::HANGAR:
            for (const auto& pos : NodeDefense::HANGAR) positions.push_back(pos);
            break;
        case Nodes::WORKSHOP:
            for (const auto& pos : NodeDefense::WORKSHOP) positions.push_back(pos);
            break;
    }
    return positions;
}

inline std::vector<Position> GetGateApproachPositions(uint32 gateId)
{
    std::vector<Position> positions;
    switch (gateId)
    {
        case Gates::ALLIANCE_FRONT:
            for (const auto& pos : GateApproach::ALLIANCE_FRONT) positions.push_back(pos);
            break;
        case Gates::ALLIANCE_WEST:
            for (const auto& pos : GateApproach::ALLIANCE_WEST) positions.push_back(pos);
            break;
        case Gates::ALLIANCE_EAST:
            for (const auto& pos : GateApproach::ALLIANCE_EAST) positions.push_back(pos);
            break;
        case Gates::HORDE_FRONT:
            for (const auto& pos : GateApproach::HORDE_FRONT) positions.push_back(pos);
            break;
        case Gates::HORDE_WEST:
            for (const auto& pos : GateApproach::HORDE_WEST) positions.push_back(pos);
            break;
        case Gates::HORDE_EAST:
            for (const auto& pos : GateApproach::HORDE_EAST) positions.push_back(pos);
            break;
    }
    return positions;
}

inline std::vector<Position> GetBossRaidPositions(uint32 targetFaction)
{
    std::vector<Position> positions;
    if (targetFaction == 1)  // Attacking Alliance (Halford)
    {
        for (const auto& pos : BossRoomPositions::HALFORD_RAID)
            positions.push_back(pos);
    }
    else  // Attacking Horde (Agmar)
    {
        for (const auto& pos : BossRoomPositions::AGMAR_RAID)
            positions.push_back(pos);
    }
    return positions;
}

inline std::vector<Position> GetChokepoints()
{
    std::vector<Position> positions;
    for (const auto& pos : StrategicPositions::CHOKEPOINTS)
        positions.push_back(Position(pos.x, pos.y, pos.z, 0.0f));
    return positions;
}

inline std::vector<Position> GetSniperPositions()
{
    std::vector<Position> positions;
    for (const auto& pos : StrategicPositions::SNIPER_POSITIONS)
        positions.push_back(Position(pos.x, pos.y, pos.z, 0.0f));
    return positions;
}

inline std::vector<Position> GetAmbushPositions(uint32 faction)
{
    std::vector<Position> positions;
    if (faction == 1)  // ALLIANCE
    {
        for (const auto& pos : StrategicPositions::ALLIANCE_AMBUSH)
            positions.push_back(Position(pos.x, pos.y, pos.z, pos.o));
    }
    else
    {
        for (const auto& pos : StrategicPositions::HORDE_AMBUSH)
            positions.push_back(Position(pos.x, pos.y, pos.z, pos.o));
    }
    return positions;
}

inline std::vector<Position> GetVehicleStagingPositions(uint32 faction)
{
    std::vector<Position> positions;
    if (faction == 1)  // ALLIANCE
    {
        for (const auto& pos : StrategicPositions::ALLIANCE_VEHICLE_STAGING)
            positions.push_back(Position(pos.x, pos.y, pos.z, pos.o));
    }
    else
    {
        for (const auto& pos : StrategicPositions::HORDE_VEHICLE_STAGING)
            positions.push_back(Position(pos.x, pos.y, pos.z, pos.o));
    }
    return positions;
}

inline std::vector<Position> GetSiegeRoute(uint32 faction)
{
    return faction == 1 ? GetAllianceSiegeRoute() : GetHordeSiegeRoute();
}

// Siege route to specific gate
inline std::vector<StrategicPositionData> GetSiegeRoute(uint32 attackingFaction, uint32 targetGate)
{
    std::vector<StrategicPositionData> route;

    // Get base route based on attacking faction
    auto baseRoute = (attackingFaction == 1) ? GetAllianceSiegeRoute() : GetHordeSiegeRoute();

    for (const auto& pos : baseRoute)
    {
        route.push_back({"Siege Waypoint", pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), 5});
    }

    return route;
}

inline float CalculateDistance(float x1, float y1, float z1, float x2, float y2, float z2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dz = z2 - z1;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

} // namespace Playerbot::Coordination::Battleground::IsleOfConquest

#endif // PLAYERBOT_AI_COORDINATION_BG_SIEGE_ISLEOFCONQUESTDATA_H
