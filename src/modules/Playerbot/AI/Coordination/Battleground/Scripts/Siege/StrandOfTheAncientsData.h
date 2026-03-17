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

#ifndef PLAYERBOT_AI_COORDINATION_BG_SIEGE_STRANDOFTHEANCIENTSDATA_H
#define PLAYERBOT_AI_COORDINATION_BG_SIEGE_STRANDOFTHEANCIENTSDATA_H

#include "Define.h"
#include "Position.h"
#include <array>
#include <vector>
#include <cmath>

namespace Playerbot::Coordination::Battleground::StrandOfTheAncients
{

// ============================================================================
// CORE CONSTANTS
// ============================================================================

constexpr uint32 MAP_ID = 607;
constexpr char BG_NAME[] = "Strand of the Ancients";
constexpr uint32 MAX_DURATION = 10 * 60 * 1000;  // 10 minutes per round
constexpr uint8 TEAM_SIZE = 15;
constexpr uint32 ROUND_COUNT = 2;
constexpr uint32 PREP_TIME = 60 * 1000;  // 60 second prep phase

// Scoring constants
constexpr uint32 GATE_DESTROY_BONUS = 100;
constexpr uint32 RELIC_CAPTURE_BONUS = 500;

// ============================================================================
// STRATEGY CONSTANTS
// ============================================================================

namespace Strategy
{
    // Timing thresholds
    constexpr uint32 OPENING_PHASE_DURATION = 60 * 1000;    // First 60 seconds
    constexpr uint32 DESPERATE_TIME_THRESHOLD = 90 * 1000;  // Last 90 seconds
    constexpr uint32 DEMOLISHER_RESPAWN_TIME = 30 * 1000;   // 30 seconds
    constexpr uint32 GATE_CHECK_INTERVAL = 5 * 1000;        // Check gates every 5s
    constexpr uint32 STRATEGY_UPDATE_INTERVAL = 10 * 1000;  // Update strategy every 10s

    // Defense configuration
    constexpr uint8 MIN_GATE_DEFENDERS = 3;
    constexpr uint8 MAX_GATE_DEFENDERS = 6;
    constexpr uint8 MIN_TURRET_OPERATORS = 2;
    constexpr uint8 DEMO_KILL_SQUAD_SIZE = 4;

    // Attack configuration
    constexpr uint8 MIN_DEMO_ESCORT = 2;
    constexpr uint8 DEMO_PER_PATH = 2;
    constexpr uint8 INFANTRY_SUPPORT_PER_DEMO = 3;

    // Gate priority weights
    constexpr uint8 OUTER_GATE_PRIORITY = 7;
    constexpr uint8 MIDDLE_GATE_PRIORITY = 8;
    constexpr uint8 INNER_GATE_PRIORITY = 9;
    constexpr uint8 ANCIENT_GATE_PRIORITY = 10;

    // Health thresholds
    constexpr uint32 DEMO_CRITICAL_HEALTH = 15000;  // Focus heal below this
    constexpr uint32 GATE_LOW_HEALTH_PERCENT = 30;  // Switch focus if gate low
}

// ============================================================================
// GATE DATA
// ============================================================================

namespace Gates
{
    constexpr uint32 GREEN_JADE = 0;      // Left outer
    constexpr uint32 BLUE_SAPPHIRE = 1;   // Right outer
    constexpr uint32 RED_SUN = 2;         // Left middle
    constexpr uint32 PURPLE_AMETHYST = 3; // Right middle
    constexpr uint32 YELLOW_MOON = 4;     // Inner
    constexpr uint32 ANCIENT_GATE = 5;    // Final gate
    constexpr uint32 COUNT = 6;

    // Gate object entries
    constexpr uint32 GATE_OF_GREEN_JADE_ENTRY = 190722;
    constexpr uint32 GATE_OF_BLUE_SAPPHIRE_ENTRY = 190724;
    constexpr uint32 GATE_OF_RED_SUN_ENTRY = 190726;
    constexpr uint32 GATE_OF_PURPLE_AMETHYST_ENTRY = 190723;
    constexpr uint32 GATE_OF_YELLOW_MOON_ENTRY = 190727;
    constexpr uint32 CHAMBER_OF_ANCIENT_RELICS_ENTRY = 192549;

    // Gate health
    constexpr uint32 OUTER_GATE_HEALTH = 100000;
    constexpr uint32 MIDDLE_GATE_HEALTH = 150000;
    constexpr uint32 INNER_GATE_HEALTH = 180000;
    constexpr uint32 ANCIENT_GATE_HEALTH = 200000;

    inline uint32 GetGateHealth(uint32 gateId)
    {
        switch (gateId)
        {
            case GREEN_JADE:
            case BLUE_SAPPHIRE:
                return OUTER_GATE_HEALTH;
            case RED_SUN:
            case PURPLE_AMETHYST:
                return MIDDLE_GATE_HEALTH;
            case YELLOW_MOON:
                return INNER_GATE_HEALTH;
            case ANCIENT_GATE:
                return ANCIENT_GATE_HEALTH;
            default:
                return 0;
        }
    }

    inline uint8 GetGateTier(uint32 gateId)
    {
        switch (gateId)
        {
            case GREEN_JADE:
            case BLUE_SAPPHIRE:
                return 1;  // Outer
            case RED_SUN:
            case PURPLE_AMETHYST:
                return 2;  // Middle
            case YELLOW_MOON:
                return 3;  // Inner
            case ANCIENT_GATE:
                return 4;  // Final
            default:
                return 0;
        }
    }
}

// ============================================================================
// GRAVEYARD DATA
// ============================================================================

namespace Graveyards
{
    constexpr uint32 BEACH_GY = 0;           // Attackers spawn
    constexpr uint32 WEST_GY = 1;            // After outer gates (west)
    constexpr uint32 EAST_GY = 2;            // After outer gates (east)
    constexpr uint32 SOUTH_GY = 3;           // After middle gates
    constexpr uint32 DEFENDER_START_GY = 4;  // Defender spawn at relic
    constexpr uint32 COUNT = 5;
}

// ============================================================================
// VEHICLE DATA
// ============================================================================

namespace Vehicles
{
    // Demolisher entries
    constexpr uint32 DEMOLISHER_ENTRY = 28781;
    constexpr uint32 DEMOLISHER_HP = 50000;
    constexpr uint32 DEMOLISHER_DAMAGE_VS_GATE = 2500;  // Per shot

    // Turret entries
    constexpr uint32 TURRET_ENTRY = 27894;
    constexpr uint32 TURRET_HP = 25000;
    constexpr uint32 TURRET_DAMAGE_VS_DEMO = 1500;  // Per shot

    // Spawn points
    constexpr uint32 SPAWN_BEACH_WEST = 0;
    constexpr uint32 SPAWN_BEACH_EAST = 1;
    constexpr uint32 SPAWN_INNER_WEST = 2;
    constexpr uint32 SPAWN_INNER_EAST = 3;
    constexpr uint32 SPAWN_COUNT = 4;
}

// ============================================================================
// TITAN RELIC
// ============================================================================

namespace Relic
{
    constexpr uint32 TITAN_RELIC_ENTRY = 192834;
    constexpr float X = 836.0f;
    constexpr float Y = -24.0f;
    constexpr float Z = 94.0f;
    constexpr float O = 0.0f;
    constexpr uint32 CAPTURE_TIME = 10 * 1000;  // 10 seconds to capture
}

// ============================================================================
// WORLD STATES
// ============================================================================

namespace WorldStates
{
    // Core states
    constexpr int32 ROUND_TIME = 3564;
    constexpr int32 ATTACKER_TEAM = 3565;
    constexpr int32 GATE_DESTROYED_COUNT = 3566;

    // Gate states (damaged/destroyed)
    constexpr int32 GREEN_JADE_INTACT = 3614;
    constexpr int32 GREEN_JADE_DAMAGED = 3615;
    constexpr int32 GREEN_JADE_DESTROYED = 3616;

    constexpr int32 BLUE_SAPPHIRE_INTACT = 3617;
    constexpr int32 BLUE_SAPPHIRE_DAMAGED = 3618;
    constexpr int32 BLUE_SAPPHIRE_DESTROYED = 3619;

    constexpr int32 RED_SUN_INTACT = 3620;
    constexpr int32 RED_SUN_DAMAGED = 3621;
    constexpr int32 RED_SUN_DESTROYED = 3622;

    constexpr int32 PURPLE_AMETHYST_INTACT = 3623;
    constexpr int32 PURPLE_AMETHYST_DAMAGED = 3624;
    constexpr int32 PURPLE_AMETHYST_DESTROYED = 3625;

    constexpr int32 YELLOW_MOON_INTACT = 3626;
    constexpr int32 YELLOW_MOON_DAMAGED = 3627;
    constexpr int32 YELLOW_MOON_DESTROYED = 3628;

    constexpr int32 ANCIENT_GATE_INTACT = 3629;
    constexpr int32 ANCIENT_GATE_DAMAGED = 3630;
    constexpr int32 ANCIENT_GATE_DESTROYED = 3631;

    // Round info
    constexpr int32 ROUND_NUMBER = 3632;
    constexpr int32 ROUND_1_TIME = 3633;
}

// ============================================================================
// GATE POSITIONS
// ============================================================================

constexpr Position GATE_POSITIONS[] = {
    Position(1411.0f, -108.0f, 61.0f, 3.14f),   // Green Jade (Left outer)
    Position(1411.0f, 53.0f, 61.0f, 3.14f),     // Blue Sapphire (Right outer)
    Position(1232.0f, -182.0f, 66.0f, 3.14f),   // Red Sun (Left middle)
    Position(1232.0f, 133.0f, 66.0f, 3.14f),    // Purple Amethyst (Right middle)
    Position(1095.0f, -24.0f, 67.0f, 3.14f),    // Yellow Moon (Inner)
    Position(878.0f, -24.0f, 79.0f, 3.14f)      // Ancient Gate (Final)
};

// ============================================================================
// GRAVEYARD POSITIONS
// ============================================================================

constexpr Position GRAVEYARD_POSITIONS[] = {
    Position(1597.0f, -106.0f, 8.0f, 3.14f),    // Beach GY
    Position(1338.0f, -298.0f, 32.0f, 3.14f),   // West GY
    Position(1338.0f, 245.0f, 32.0f, 3.14f),    // East GY
    Position(1119.0f, -24.0f, 67.0f, 3.14f),    // South GY
    Position(830.0f, -24.0f, 93.0f, 3.14f)      // Defender Start GY
};

// ============================================================================
// DEMOLISHER SPAWN POSITIONS
// ============================================================================

constexpr Position DEMOLISHER_SPAWNS[] = {
    Position(1611.0f, -117.0f, 8.0f, 3.14f),    // Beach West
    Position(1611.0f, 61.0f, 8.0f, 3.14f),      // Beach East
    Position(1353.0f, -317.0f, 35.0f, 3.14f),   // Inner West
    Position(1353.0f, 260.0f, 35.0f, 3.14f)     // Inner East
};

// ============================================================================
// DEFENSE POSITION DATA STRUCTURE
// ============================================================================

struct DefensePositionData
{
    float x, y, z, o;
};

// ============================================================================
// GATE DEFENSE POSITIONS (8 per gate = 48 total)
// ============================================================================

namespace GateDefense
{
    // Green Jade Gate defense positions
    constexpr std::array<DefensePositionData, 8> GREEN_JADE = {{
        { 1395.0f, -115.0f, 61.0f, 0.0f },    // Front center
        { 1390.0f, -100.0f, 61.0f, 5.80f },   // Front right
        { 1390.0f, -130.0f, 61.0f, 0.50f },   // Front left
        { 1405.0f, -108.0f, 63.0f, 0.0f },    // Elevated center
        { 1400.0f, -95.0f, 63.0f, 5.50f },    // Elevated right
        { 1400.0f, -125.0f, 63.0f, 0.80f },   // Elevated left
        { 1420.0f, -108.0f, 61.0f, 3.14f },   // Behind gate
        { 1385.0f, -108.0f, 61.0f, 0.0f }     // Far front
    }};

    // Blue Sapphire Gate defense positions
    constexpr std::array<DefensePositionData, 8> BLUE_SAPPHIRE = {{
        { 1395.0f, 60.0f, 61.0f, 0.0f },      // Front center
        { 1390.0f, 75.0f, 61.0f, 5.80f },     // Front right
        { 1390.0f, 45.0f, 61.0f, 0.50f },     // Front left
        { 1405.0f, 53.0f, 63.0f, 0.0f },      // Elevated center
        { 1400.0f, 68.0f, 63.0f, 5.50f },     // Elevated right
        { 1400.0f, 38.0f, 63.0f, 0.80f },     // Elevated left
        { 1420.0f, 53.0f, 61.0f, 3.14f },     // Behind gate
        { 1385.0f, 53.0f, 61.0f, 0.0f }       // Far front
    }};

    // Red Sun Gate defense positions
    constexpr std::array<DefensePositionData, 8> RED_SUN = {{
        { 1220.0f, -185.0f, 66.0f, 0.0f },    // Front center
        { 1215.0f, -170.0f, 66.0f, 5.80f },   // Front right
        { 1215.0f, -200.0f, 66.0f, 0.50f },   // Front left
        { 1228.0f, -182.0f, 68.0f, 0.0f },    // Elevated center
        { 1225.0f, -168.0f, 68.0f, 5.50f },   // Elevated right
        { 1225.0f, -196.0f, 68.0f, 0.80f },   // Elevated left
        { 1245.0f, -182.0f, 66.0f, 3.14f },   // Behind gate
        { 1210.0f, -182.0f, 66.0f, 0.0f }     // Far front
    }};

    // Purple Amethyst Gate defense positions
    constexpr std::array<DefensePositionData, 8> PURPLE_AMETHYST = {{
        { 1220.0f, 140.0f, 66.0f, 0.0f },     // Front center
        { 1215.0f, 155.0f, 66.0f, 5.80f },    // Front right
        { 1215.0f, 125.0f, 66.0f, 0.50f },    // Front left
        { 1228.0f, 133.0f, 68.0f, 0.0f },     // Elevated center
        { 1225.0f, 148.0f, 68.0f, 5.50f },    // Elevated right
        { 1225.0f, 118.0f, 68.0f, 0.80f },    // Elevated left
        { 1245.0f, 133.0f, 66.0f, 3.14f },    // Behind gate
        { 1210.0f, 133.0f, 66.0f, 0.0f }      // Far front
    }};

    // Yellow Moon Gate defense positions
    constexpr std::array<DefensePositionData, 8> YELLOW_MOON = {{
        { 1085.0f, -24.0f, 67.0f, 0.0f },     // Front center
        { 1080.0f, -10.0f, 67.0f, 5.80f },    // Front right
        { 1080.0f, -38.0f, 67.0f, 0.50f },    // Front left
        { 1092.0f, -24.0f, 69.0f, 0.0f },     // Elevated center
        { 1088.0f, -12.0f, 69.0f, 5.50f },    // Elevated right
        { 1088.0f, -36.0f, 69.0f, 0.80f },    // Elevated left
        { 1105.0f, -24.0f, 67.0f, 3.14f },    // Behind gate
        { 1070.0f, -24.0f, 67.0f, 0.0f }      // Far front
    }};

    // Ancient Gate (Chamber) defense positions
    constexpr std::array<DefensePositionData, 8> ANCIENT_GATE = {{
        { 865.0f, -24.0f, 79.0f, 0.0f },      // Front center
        { 860.0f, -10.0f, 79.0f, 5.80f },     // Front right
        { 860.0f, -38.0f, 79.0f, 0.50f },     // Front left
        { 872.0f, -24.0f, 81.0f, 0.0f },      // Elevated center
        { 868.0f, -12.0f, 81.0f, 5.50f },     // Elevated right
        { 868.0f, -36.0f, 81.0f, 0.80f },     // Elevated left
        { 885.0f, -24.0f, 79.0f, 3.14f },     // Behind gate
        { 850.0f, -24.0f, 79.0f, 0.0f }       // Far front
    }};

    // Array of all gate defense positions
    constexpr std::array<DefensePositionData, 8> POSITIONS[] = {
        GREEN_JADE[0], GREEN_JADE[1], GREEN_JADE[2], GREEN_JADE[3],
        GREEN_JADE[4], GREEN_JADE[5], GREEN_JADE[6], GREEN_JADE[7]
    };
}

// ============================================================================
// TURRET POSITIONS (Near each gate)
// ============================================================================

namespace Turrets
{
    // Turrets defending outer gates
    constexpr std::array<Position, 4> OUTER_GATE_TURRETS = {{
        Position(1430.0f, -125.0f, 65.0f, 3.14f),  // Green Jade - South turret
        Position(1430.0f, -90.0f, 65.0f, 3.14f),   // Green Jade - North turret
        Position(1430.0f, 70.0f, 65.0f, 3.14f),    // Blue Sapphire - South turret
        Position(1430.0f, 35.0f, 65.0f, 3.14f)     // Blue Sapphire - North turret
    }};

    // Turrets defending middle gates
    constexpr std::array<Position, 4> MIDDLE_GATE_TURRETS = {{
        Position(1250.0f, -200.0f, 70.0f, 3.14f),  // Red Sun - South turret
        Position(1250.0f, -165.0f, 70.0f, 3.14f),  // Red Sun - North turret
        Position(1250.0f, 150.0f, 70.0f, 3.14f),   // Purple Amethyst - South turret
        Position(1250.0f, 115.0f, 70.0f, 3.14f)    // Purple Amethyst - North turret
    }};

    // Turrets defending inner gate
    constexpr std::array<Position, 2> INNER_GATE_TURRETS = {{
        Position(1110.0f, -45.0f, 71.0f, 3.14f),   // Yellow Moon - South turret
        Position(1110.0f, -5.0f, 71.0f, 3.14f)     // Yellow Moon - North turret
    }};

    // Turrets defending ancient gate
    constexpr std::array<Position, 2> ANCIENT_GATE_TURRETS = {{
        Position(895.0f, -45.0f, 83.0f, 3.14f),    // Ancient - South turret
        Position(895.0f, -5.0f, 83.0f, 3.14f)      // Ancient - North turret
    }};
}

// ============================================================================
// BEACH POSITIONS (Attacker staging areas)
// ============================================================================

namespace Beach
{
    // Beach landing positions
    constexpr std::array<Position, 8> LANDING_POSITIONS = {{
        Position(1600.0f, -100.0f, 8.0f, 3.14f),
        Position(1600.0f, -80.0f, 8.0f, 3.14f),
        Position(1600.0f, -60.0f, 8.0f, 3.14f),
        Position(1600.0f, -40.0f, 8.0f, 3.14f),
        Position(1600.0f, -20.0f, 8.0f, 3.14f),
        Position(1600.0f, 0.0f, 8.0f, 3.14f),
        Position(1600.0f, 20.0f, 8.0f, 3.14f),
        Position(1600.0f, 40.0f, 8.0f, 3.14f)
    }};

    // Demolisher escort formation (around demo)
    constexpr std::array<DefensePositionData, 6> ESCORT_FORMATION = {{
        { 5.0f, 5.0f, 0.0f, 0.0f },     // Front right offset
        { 5.0f, -5.0f, 0.0f, 0.0f },    // Front left offset
        { -3.0f, 8.0f, 0.0f, 0.0f },    // Side right offset
        { -3.0f, -8.0f, 0.0f, 0.0f },   // Side left offset
        { -8.0f, 3.0f, 0.0f, 0.0f },    // Rear right offset
        { -8.0f, -3.0f, 0.0f, 0.0f }    // Rear left offset
    }};
}

// ============================================================================
// STRATEGIC POSITION DATA STRUCTURE
// ============================================================================

struct StrategicPositionData
{
    const char* name;
    float x, y, z;
    uint8 strategicValue;  // 1-10 importance rating
};

// ============================================================================
// CHOKEPOINTS
// ============================================================================

namespace Chokepoints
{
    constexpr std::array<StrategicPositionData, 10> POSITIONS = {{
        { "Beach Narrows West", 1550.0f, -130.0f, 10.0f, 7 },
        { "Beach Narrows East", 1550.0f, 80.0f, 10.0f, 7 },
        { "Green Jade Approach", 1450.0f, -108.0f, 55.0f, 8 },
        { "Blue Sapphire Approach", 1450.0f, 53.0f, 55.0f, 8 },
        { "West Courtyard", 1300.0f, -220.0f, 40.0f, 7 },
        { "East Courtyard", 1300.0f, 175.0f, 40.0f, 7 },
        { "Red Sun Approach", 1280.0f, -182.0f, 60.0f, 8 },
        { "Purple Amethyst Approach", 1280.0f, 133.0f, 60.0f, 8 },
        { "Inner Courtyard", 1150.0f, -24.0f, 64.0f, 9 },
        { "Relic Chamber Entrance", 920.0f, -24.0f, 85.0f, 10 }
    }};
}

// ============================================================================
// SNIPER POSITIONS
// ============================================================================

namespace SniperPositions
{
    constexpr std::array<StrategicPositionData, 8> POSITIONS = {{
        { "Beach Cliff West", 1580.0f, -160.0f, 25.0f, 6 },
        { "Beach Cliff East", 1580.0f, 100.0f, 25.0f, 6 },
        { "Outer Wall West", 1440.0f, -140.0f, 68.0f, 8 },
        { "Outer Wall East", 1440.0f, 90.0f, 68.0f, 8 },
        { "Middle Wall West", 1260.0f, -210.0f, 72.0f, 8 },
        { "Middle Wall East", 1260.0f, 165.0f, 72.0f, 8 },
        { "Inner Wall", 1120.0f, -60.0f, 73.0f, 9 },
        { "Relic Overlook", 880.0f, -50.0f, 95.0f, 9 }
    }};
}

// ============================================================================
// AMBUSH POSITIONS (for defenders intercepting demos)
// ============================================================================

namespace AmbushPositions
{
    constexpr std::array<StrategicPositionData, 8> DEFENDER_AMBUSH = {{
        { "Beach Ambush West", 1520.0f, -150.0f, 12.0f, 7 },
        { "Beach Ambush East", 1520.0f, 100.0f, 12.0f, 7 },
        { "Outer Path Ambush West", 1380.0f, -150.0f, 50.0f, 8 },
        { "Outer Path Ambush East", 1380.0f, 100.0f, 50.0f, 8 },
        { "Middle Path Ambush West", 1210.0f, -150.0f, 60.0f, 8 },
        { "Middle Path Ambush East", 1210.0f, 100.0f, 60.0f, 8 },
        { "Inner Path Ambush", 1050.0f, -24.0f, 65.0f, 9 },
        { "Relic Defense Point", 850.0f, -24.0f, 90.0f, 10 }
    }};
}

// ============================================================================
// DEMOLISHER ATTACK ROUTES
// ============================================================================

namespace DemolisherRoutes
{
    // Left path: Beach -> Green Jade -> Red Sun -> Yellow Moon -> Ancient
    constexpr std::array<Position, 8> LEFT_PATH = {{
        Position(1600.0f, -100.0f, 8.0f, 3.14f),    // Beach spawn
        Position(1500.0f, -110.0f, 25.0f, 3.14f),   // Beach approach
        Position(1420.0f, -108.0f, 55.0f, 3.14f),   // Green Jade target
        Position(1350.0f, -150.0f, 45.0f, 3.14f),   // Post Green Jade
        Position(1250.0f, -182.0f, 62.0f, 3.14f),   // Red Sun target
        Position(1170.0f, -100.0f, 65.0f, 3.14f),   // Post Red Sun
        Position(1100.0f, -24.0f, 67.0f, 3.14f),    // Yellow Moon target
        Position(920.0f, -24.0f, 79.0f, 3.14f)      // Ancient Gate target
    }};

    // Right path: Beach -> Blue Sapphire -> Purple Amethyst -> Yellow Moon -> Ancient
    constexpr std::array<Position, 8> RIGHT_PATH = {{
        Position(1600.0f, 50.0f, 8.0f, 3.14f),      // Beach spawn
        Position(1500.0f, 55.0f, 25.0f, 3.14f),     // Beach approach
        Position(1420.0f, 53.0f, 55.0f, 3.14f),     // Blue Sapphire target
        Position(1350.0f, 100.0f, 45.0f, 3.14f),    // Post Blue Sapphire
        Position(1250.0f, 133.0f, 62.0f, 3.14f),    // Purple Amethyst target
        Position(1170.0f, 50.0f, 65.0f, 3.14f),     // Post Purple Amethyst
        Position(1100.0f, -24.0f, 67.0f, 3.14f),    // Yellow Moon target
        Position(920.0f, -24.0f, 79.0f, 3.14f)      // Ancient Gate target
    }};
}

// ============================================================================
// RELIC ROOM POSITIONS
// ============================================================================

namespace RelicRoom
{
    // Positions around the Titan Relic for attackers
    constexpr std::array<DefensePositionData, 10> ATTACK_POSITIONS = {{
        { 840.0f, -24.0f, 94.0f, 3.14f },     // Direct approach
        { 845.0f, -15.0f, 94.0f, 3.50f },     // Right side
        { 845.0f, -33.0f, 94.0f, 2.80f },     // Left side
        { 850.0f, -24.0f, 94.0f, 3.14f },     // Slightly back center
        { 852.0f, -10.0f, 94.0f, 3.70f },     // Back right
        { 852.0f, -38.0f, 94.0f, 2.60f },     // Back left
        { 830.0f, -18.0f, 94.0f, 3.30f },     // Forward right
        { 830.0f, -30.0f, 94.0f, 3.00f },     // Forward left
        { 825.0f, -24.0f, 94.0f, 3.14f },     // Deep forward
        { 860.0f, -24.0f, 94.0f, 3.14f }      // Entrance guard
    }};

    // Positions for defenders protecting the relic
    constexpr std::array<DefensePositionData, 10> DEFENSE_POSITIONS = {{
        { 836.0f, -24.0f, 94.0f, 0.0f },      // On relic
        { 836.0f, -15.0f, 94.0f, 5.80f },     // Relic right
        { 836.0f, -33.0f, 94.0f, 0.50f },     // Relic left
        { 845.0f, -24.0f, 94.0f, 0.0f },      // Forward center
        { 850.0f, -10.0f, 94.0f, 5.50f },     // Forward right
        { 850.0f, -38.0f, 94.0f, 0.80f },     // Forward left
        { 860.0f, -24.0f, 94.0f, 0.0f },      // Entrance
        { 865.0f, -15.0f, 94.0f, 5.30f },     // Entrance right
        { 865.0f, -33.0f, 94.0f, 1.00f },     // Entrance left
        { 825.0f, -24.0f, 94.0f, 0.0f }       // Behind relic
    }};
}

// ============================================================================
// ATTACK PATH ENUMERATION
// ============================================================================

enum class AttackPath
{
    LEFT,   // Green Jade -> Red Sun -> Yellow Moon
    RIGHT,  // Blue Sapphire -> Purple Amethyst -> Yellow Moon
    SPLIT   // Both paths simultaneously
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

inline Position GetGatePosition(uint32 gateId)
{
    if (gateId < Gates::COUNT)
        return GATE_POSITIONS[gateId];
    return Position(0, 0, 0, 0);
}

inline Position GetGraveyardPosition(uint32 gyId)
{
    if (gyId < Graveyards::COUNT)
        return GRAVEYARD_POSITIONS[gyId];
    return Position(0, 0, 0, 0);
}

inline Position GetDemolisherSpawn(uint32 spawnId)
{
    if (spawnId < Vehicles::SPAWN_COUNT)
        return DEMOLISHER_SPAWNS[spawnId];
    return Position(0, 0, 0, 0);
}

inline Position GetRelicPosition()
{
    return Position(Relic::X, Relic::Y, Relic::Z, Relic::O);
}

inline const char* GetGateName(uint32 gateId)
{
    switch (gateId)
    {
        case Gates::GREEN_JADE: return "Gate of the Green Jade";
        case Gates::BLUE_SAPPHIRE: return "Gate of the Blue Sapphire";
        case Gates::RED_SUN: return "Gate of the Red Sun";
        case Gates::PURPLE_AMETHYST: return "Gate of the Purple Amethyst";
        case Gates::YELLOW_MOON: return "Gate of the Yellow Moon";
        case Gates::ANCIENT_GATE: return "Chamber of Ancient Relics";
        default: return "Unknown Gate";
    }
}

inline const char* GetGraveyardName(uint32 gyId)
{
    switch (gyId)
    {
        case Graveyards::BEACH_GY: return "Beach Graveyard";
        case Graveyards::WEST_GY: return "West Graveyard";
        case Graveyards::EAST_GY: return "East Graveyard";
        case Graveyards::SOUTH_GY: return "South Graveyard";
        case Graveyards::DEFENDER_START_GY: return "Defender Graveyard";
        default: return "Unknown Graveyard";
    }
}

// Gate dependencies - which gate must be destroyed first
inline std::vector<uint32> GetGateDependencies(uint32 gateId)
{
    switch (gateId)
    {
        case Gates::RED_SUN:
            return { Gates::GREEN_JADE };
        case Gates::PURPLE_AMETHYST:
            return { Gates::BLUE_SAPPHIRE };
        case Gates::YELLOW_MOON:
            return { Gates::RED_SUN, Gates::PURPLE_AMETHYST };  // Either one
        case Gates::ANCIENT_GATE:
            return { Gates::YELLOW_MOON };
        default:
            return {};  // Outer gates have no dependencies
    }
}

inline std::vector<Position> GetGateDefensePositions(uint32 gateId)
{
    std::vector<Position> positions;
    const std::array<DefensePositionData, 8>* defenseArray = nullptr;

    switch (gateId)
    {
        case Gates::GREEN_JADE: defenseArray = &GateDefense::GREEN_JADE; break;
        case Gates::BLUE_SAPPHIRE: defenseArray = &GateDefense::BLUE_SAPPHIRE; break;
        case Gates::RED_SUN: defenseArray = &GateDefense::RED_SUN; break;
        case Gates::PURPLE_AMETHYST: defenseArray = &GateDefense::PURPLE_AMETHYST; break;
        case Gates::YELLOW_MOON: defenseArray = &GateDefense::YELLOW_MOON; break;
        case Gates::ANCIENT_GATE: defenseArray = &GateDefense::ANCIENT_GATE; break;
        default: return positions;
    }

    for (const auto& pos : *defenseArray)
        positions.push_back(Position(pos.x, pos.y, pos.z, pos.o));

    return positions;
}

inline std::vector<Position> GetChokepoints()
{
    std::vector<Position> positions;
    for (const auto& pos : Chokepoints::POSITIONS)
        positions.push_back(Position(pos.x, pos.y, pos.z, 0.0f));
    return positions;
}

inline std::vector<Position> GetSniperPositions()
{
    std::vector<Position> positions;
    for (const auto& pos : SniperPositions::POSITIONS)
        positions.push_back(Position(pos.x, pos.y, pos.z, 0.0f));
    return positions;
}

inline std::vector<Position> GetDemolisherRoute(AttackPath path)
{
    std::vector<Position> route;
    if (path == AttackPath::LEFT)
    {
        for (const auto& pos : DemolisherRoutes::LEFT_PATH)
            route.push_back(pos);
    }
    else
    {
        for (const auto& pos : DemolisherRoutes::RIGHT_PATH)
            route.push_back(pos);
    }
    return route;
}

inline std::vector<Position> GetRelicAttackPositions()
{
    std::vector<Position> positions;
    for (const auto& pos : RelicRoom::ATTACK_POSITIONS)
        positions.push_back(Position(pos.x, pos.y, pos.z, pos.o));
    return positions;
}

inline std::vector<Position> GetRelicDefensePositions()
{
    std::vector<Position> positions;
    for (const auto& pos : RelicRoom::DEFENSE_POSITIONS)
        positions.push_back(Position(pos.x, pos.y, pos.z, pos.o));
    return positions;
}

inline std::vector<Position> GetAmbushPositions()
{
    std::vector<Position> positions;
    for (const auto& pos : AmbushPositions::DEFENDER_AMBUSH)
        positions.push_back(Position(pos.x, pos.y, pos.z, 0.0f));
    return positions;
}

inline std::vector<Position> GetEscortFormation(const Position& demoPos)
{
    std::vector<Position> positions;
    for (const auto& offset : Beach::ESCORT_FORMATION)
    {
        positions.push_back(Position(
            demoPos.GetPositionX() + offset.x,
            demoPos.GetPositionY() + offset.y,
            demoPos.GetPositionZ() + offset.z,
            demoPos.GetOrientation()
        ));
    }
    return positions;
}

inline float CalculateDistance(float x1, float y1, float z1, float x2, float y2, float z2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dz = z2 - z1;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

} // namespace Playerbot::Coordination::Battleground::StrandOfTheAncients

#endif // PLAYERBOT_AI_COORDINATION_BG_SIEGE_STRANDOFTHEANCIENTSDATA_H
