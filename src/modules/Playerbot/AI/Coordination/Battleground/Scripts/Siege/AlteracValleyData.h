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

#ifndef PLAYERBOT_AI_COORDINATION_BG_SIEGE_ALTERACVALLEYDATA_H
#define PLAYERBOT_AI_COORDINATION_BG_SIEGE_ALTERACVALLEYDATA_H

#include "Define.h"
#include "Position.h"
#include <array>
#include <vector>
#include <cmath>

namespace Playerbot::Coordination::Battleground::AlteracValley
{

// ============================================================================
// CORE CONSTANTS
// ============================================================================

constexpr uint32 MAP_ID = 30;
constexpr char BG_NAME[] = "Alterac Valley";
constexpr uint32 MAX_DURATION = 0;  // No time limit (until boss kill or reinforcements deplete)
constexpr uint8 TEAM_SIZE = 40;
constexpr uint32 STARTING_REINFORCEMENTS = 600;
constexpr uint32 REINF_LOSS_PER_DEATH = 1;
constexpr uint32 REINF_LOSS_PER_TOWER = 75;
constexpr uint32 REINF_GAIN_PER_CAPTAIN = 100;

// ============================================================================
// OBJECTIVE IDENTIFIERS
// ============================================================================

namespace ObjectiveIds
{
    // Towers (0-7)
    constexpr uint32 DUN_BALDAR_NORTH = 0;
    constexpr uint32 DUN_BALDAR_SOUTH = 1;
    constexpr uint32 ICEWING_BUNKER = 2;
    constexpr uint32 STONEHEARTH_BUNKER = 3;
    constexpr uint32 TOWER_POINT = 4;
    constexpr uint32 ICEBLOOD_TOWER = 5;
    constexpr uint32 EAST_FROSTWOLF = 6;
    constexpr uint32 WEST_FROSTWOLF = 7;
    constexpr uint32 TOWER_COUNT = 8;

    // Graveyards (50-56)
    constexpr uint32 GY_STORMPIKE = 50;
    constexpr uint32 GY_STORMPIKE_AID = 51;
    constexpr uint32 GY_STONEHEARTH = 52;
    constexpr uint32 GY_SNOWFALL = 53;
    constexpr uint32 GY_ICEBLOOD = 54;
    constexpr uint32 GY_FROSTWOLF = 55;
    constexpr uint32 GY_FROSTWOLF_HUT = 56;
    constexpr uint32 GY_COUNT = 7;

    // Bosses (100-101)
    constexpr uint32 VANNDAR = 100;
    constexpr uint32 DREKTHAR = 101;

    // Captains (110-111)
    constexpr uint32 BALINDA = 110;
    constexpr uint32 GALVANGAR = 111;

    // Mines (120-121)
    constexpr uint32 IRONDEEP_MINE = 120;
    constexpr uint32 COLDTOOTH_MINE = 121;
}

// ============================================================================
// STRATEGY CONSTANTS
// ============================================================================

namespace Strategy
{
    // Tower management
    constexpr uint32 TOWER_BURN_TIME = 240000;          // 4 minutes to burn
    constexpr uint8 MIN_TOWER_DEFENDERS = 2;            // Minimum to hold tower
    constexpr uint8 TOWER_ASSAULT_SIZE = 5;             // Players to assault a tower
    constexpr uint8 BOSS_TOWER_THRESHOLD = 2;           // Max towers for boss viability

    // Graveyard management
    constexpr uint32 GY_CAPTURE_TIME = 240000;          // 4 minutes to capture
    constexpr uint8 MIN_GY_DEFENDERS = 2;
    constexpr uint8 GY_ASSAULT_SIZE = 5;

    // Boss assault
    constexpr uint8 MIN_BOSS_RAID_SIZE = 20;            // Minimum for boss pull
    constexpr uint8 OPTIMAL_BOSS_RAID_SIZE = 30;        // Optimal raid size
    constexpr float BOSS_WARMASTERS_PER_TOWER = 1.0f;   // Extra warmasters per standing tower

    // Reinforcement thresholds
    constexpr uint32 REINF_DESPERATE_THRESHOLD = 100;   // Below this = desperate
    constexpr uint32 REINF_LOW_THRESHOLD = 200;         // Below this = aggressive
    constexpr uint32 REINF_ADVANTAGE_THRESHOLD = 100;   // Lead to consider aggressive

    // Team allocation percentages
    constexpr uint8 OPENING_OFFENSE_PERCENT = 70;
    constexpr uint8 OPENING_DEFENSE_PERCENT = 30;
    constexpr uint8 TOWER_BURN_OFFENSE = 60;
    constexpr uint8 BOSS_ASSAULT_OFFENSE = 85;
    constexpr uint8 DEFENSE_MODE_OFFENSE = 30;

    // Timing
    constexpr uint32 OPENING_PHASE_DURATION = 180000;   // First 3 minutes
    constexpr uint32 STRATEGY_UPDATE_INTERVAL = 10000;  // 10 seconds
    constexpr uint32 TOWER_CHECK_INTERVAL = 5000;       // 5 seconds

    // Tower strategy thresholds
    constexpr uint8 TOWER_BURN_THRESHOLD = 2;           // Burn towers if enemy has more than this
}

// ============================================================================
// BOSS DATA
// ============================================================================

namespace Bosses
{
    // Alliance Boss - Vanndar Stormpike (in Dun Baldar)
    constexpr uint32 VANNDAR_ENTRY = 11948;
    constexpr float VANNDAR_X = -1370.0f;
    constexpr float VANNDAR_Y = -219.0f;
    constexpr float VANNDAR_Z = 98.0f;
    constexpr float VANNDAR_O = 0.0f;

    // Horde Boss - Drek'Thar (in Frostwolf Keep)
    constexpr uint32 DREKTHAR_ENTRY = 11946;
    constexpr float DREKTHAR_X = -1361.0f;
    constexpr float DREKTHAR_Y = -306.0f;
    constexpr float DREKTHAR_Z = 89.0f;
    constexpr float DREKTHAR_O = 0.0f;

    // Warmaster entries (4 per boss, 1 removed per tower destroyed)
    constexpr uint32 WARMASTER_ALLIANCE_START = 14762;
    constexpr uint32 WARMASTER_HORDE_START = 14772;
}

// ============================================================================
// CAPTAIN DATA
// ============================================================================

namespace Captains
{
    // Balinda Stonehearth (Alliance Captain - in Stonehearth Outpost)
    constexpr uint32 BALINDA_ENTRY = 11949;
    constexpr float BALINDA_X = -155.0f;
    constexpr float BALINDA_Y = -87.0f;
    constexpr float BALINDA_Z = 79.0f;
    constexpr float BALINDA_O = 0.0f;

    // Galvangar (Horde Captain - in Iceblood Garrison)
    constexpr uint32 GALVANGAR_ENTRY = 11947;
    constexpr float GALVANGAR_X = -545.0f;
    constexpr float GALVANGAR_Y = -399.0f;
    constexpr float GALVANGAR_Z = 52.0f;
    constexpr float GALVANGAR_O = 0.0f;
}

// ============================================================================
// TOWER/BUNKER DATA
// ============================================================================

namespace Towers
{
    // Alliance Bunkers (Horde must destroy)
    constexpr uint32 DUN_BALDAR_NORTH = 0;
    constexpr uint32 DUN_BALDAR_SOUTH = 1;
    constexpr uint32 ICEWING_BUNKER = 2;
    constexpr uint32 STONEHEARTH_BUNKER = 3;

    // Horde Towers (Alliance must destroy)
    constexpr uint32 TOWER_POINT = 4;
    constexpr uint32 ICEBLOOD_TOWER = 5;
    constexpr uint32 EAST_FROSTWOLF = 6;
    constexpr uint32 WEST_FROSTWOLF = 7;

    constexpr uint32 COUNT = 8;
    constexpr uint32 ALLIANCE_COUNT = 4;
    constexpr uint32 HORDE_COUNT = 4;
}

// Tower positions
constexpr Position TOWER_POSITIONS[] = {
    { -1368.30f, -313.10f, 107.14f, 0.0f },  // Dun Baldar North
    { -1367.40f, -221.20f, 98.43f, 0.0f },   // Dun Baldar South
    { -173.00f, -440.00f, 33.00f, 0.0f },    // Icewing Bunker
    { -155.87f, -87.37f, 79.08f, 0.0f },     // Stonehearth Bunker
    { -570.00f, -262.00f, 75.00f, 0.0f },    // Tower Point
    { -572.00f, -359.00f, 90.00f, 0.0f },    // Iceblood Tower
    { -1302.00f, -315.00f, 113.87f, 0.0f },  // East Frostwolf Tower
    { -1297.00f, -269.00f, 114.14f, 0.0f }   // West Frostwolf Tower
};

// ============================================================================
// TOWER DEFENSE POSITIONS (8 per tower = 64 total)
// ============================================================================

namespace TowerDefense
{
    // Dun Baldar North Bunker defense
    constexpr std::array<Position, 8> DUN_BALDAR_NORTH = {{
        Position(-1370.0f, -320.0f, 107.0f, 1.57f),   // Flag position
        Position(-1375.0f, -308.0f, 107.0f, 0.80f),   // Entrance left
        Position(-1363.0f, -308.0f, 107.0f, 2.35f),   // Entrance right
        Position(-1378.0f, -318.0f, 107.0f, 0.50f),   // Corner NW
        Position(-1360.0f, -318.0f, 107.0f, 2.60f),   // Corner NE
        Position(-1368.0f, -325.0f, 107.0f, 1.57f),   // Back center
        Position(-1380.0f, -310.0f, 110.0f, 0.40f),   // Upper left
        Position(-1356.0f, -310.0f, 110.0f, 2.75f)    // Upper right
    }};

    // Dun Baldar South Bunker defense
    constexpr std::array<Position, 8> DUN_BALDAR_SOUTH = {{
        Position(-1367.0f, -228.0f, 98.0f, 1.57f),    // Flag position
        Position(-1372.0f, -215.0f, 98.0f, 0.80f),
        Position(-1362.0f, -215.0f, 98.0f, 2.35f),
        Position(-1375.0f, -225.0f, 98.0f, 0.50f),
        Position(-1359.0f, -225.0f, 98.0f, 2.60f),
        Position(-1367.0f, -235.0f, 98.0f, 1.57f),
        Position(-1377.0f, -218.0f, 101.0f, 0.40f),
        Position(-1357.0f, -218.0f, 101.0f, 2.75f)
    }};

    // Icewing Bunker defense
    constexpr std::array<Position, 8> ICEWING = {{
        Position(-173.0f, -447.0f, 33.0f, 1.57f),     // Flag position
        Position(-178.0f, -435.0f, 33.0f, 0.80f),
        Position(-168.0f, -435.0f, 33.0f, 2.35f),
        Position(-180.0f, -445.0f, 33.0f, 0.50f),
        Position(-166.0f, -445.0f, 33.0f, 2.60f),
        Position(-173.0f, -455.0f, 33.0f, 1.57f),
        Position(-183.0f, -438.0f, 36.0f, 0.40f),
        Position(-163.0f, -438.0f, 36.0f, 2.75f)
    }};

    // Stonehearth Bunker defense
    constexpr std::array<Position, 8> STONEHEARTH = {{
        Position(-155.0f, -94.0f, 79.0f, 1.57f),      // Flag position
        Position(-160.0f, -82.0f, 79.0f, 0.80f),
        Position(-150.0f, -82.0f, 79.0f, 2.35f),
        Position(-163.0f, -92.0f, 79.0f, 0.50f),
        Position(-147.0f, -92.0f, 79.0f, 2.60f),
        Position(-155.0f, -102.0f, 79.0f, 1.57f),
        Position(-165.0f, -85.0f, 82.0f, 0.40f),
        Position(-145.0f, -85.0f, 82.0f, 2.75f)
    }};

    // Tower Point defense
    constexpr std::array<Position, 8> TOWER_POINT = {{
        Position(-570.0f, -269.0f, 75.0f, 1.57f),     // Flag position
        Position(-575.0f, -257.0f, 75.0f, 0.80f),
        Position(-565.0f, -257.0f, 75.0f, 2.35f),
        Position(-578.0f, -267.0f, 75.0f, 0.50f),
        Position(-562.0f, -267.0f, 75.0f, 2.60f),
        Position(-570.0f, -277.0f, 75.0f, 1.57f),
        Position(-580.0f, -260.0f, 78.0f, 0.40f),
        Position(-560.0f, -260.0f, 78.0f, 2.75f)
    }};

    // Iceblood Tower defense
    constexpr std::array<Position, 8> ICEBLOOD = {{
        Position(-572.0f, -366.0f, 90.0f, 1.57f),     // Flag position
        Position(-577.0f, -354.0f, 90.0f, 0.80f),
        Position(-567.0f, -354.0f, 90.0f, 2.35f),
        Position(-580.0f, -364.0f, 90.0f, 0.50f),
        Position(-564.0f, -364.0f, 90.0f, 2.60f),
        Position(-572.0f, -374.0f, 90.0f, 1.57f),
        Position(-582.0f, -357.0f, 93.0f, 0.40f),
        Position(-562.0f, -357.0f, 93.0f, 2.75f)
    }};

    // East Frostwolf Tower defense
    constexpr std::array<Position, 8> EAST_FROSTWOLF = {{
        Position(-1302.0f, -322.0f, 113.0f, 1.57f),   // Flag position
        Position(-1307.0f, -310.0f, 113.0f, 0.80f),
        Position(-1297.0f, -310.0f, 113.0f, 2.35f),
        Position(-1310.0f, -320.0f, 113.0f, 0.50f),
        Position(-1294.0f, -320.0f, 113.0f, 2.60f),
        Position(-1302.0f, -330.0f, 113.0f, 1.57f),
        Position(-1312.0f, -313.0f, 116.0f, 0.40f),
        Position(-1292.0f, -313.0f, 116.0f, 2.75f)
    }};

    // West Frostwolf Tower defense
    constexpr std::array<Position, 8> WEST_FROSTWOLF = {{
        Position(-1297.0f, -276.0f, 114.0f, 1.57f),   // Flag position
        Position(-1302.0f, -264.0f, 114.0f, 0.80f),
        Position(-1292.0f, -264.0f, 114.0f, 2.35f),
        Position(-1305.0f, -274.0f, 114.0f, 0.50f),
        Position(-1289.0f, -274.0f, 114.0f, 2.60f),
        Position(-1297.0f, -284.0f, 114.0f, 1.57f),
        Position(-1307.0f, -267.0f, 117.0f, 0.40f),
        Position(-1287.0f, -267.0f, 117.0f, 2.75f)
    }};
}

// ============================================================================
// GRAVEYARD DATA
// ============================================================================

namespace Graveyards
{
    constexpr uint32 STORMPIKE_GY = 0;
    constexpr uint32 STORMPIKE_AID_STATION = 1;
    constexpr uint32 STONEHEARTH_GY = 2;
    constexpr uint32 SNOWFALL_GY = 3;          // Neutral, capturable
    constexpr uint32 ICEBLOOD_GY = 4;
    constexpr uint32 FROSTWOLF_GY = 5;
    constexpr uint32 FROSTWOLF_RELIEF_HUT = 6;
    constexpr uint32 COUNT = 7;
}

constexpr Position GRAVEYARD_POSITIONS[] = {
    { -1404.80f, -309.10f, 89.94f, 0.0f },   // Stormpike GY
    { -1361.62f, -220.67f, 98.94f, 0.0f },   // Stormpike Aid Station
    { -172.50f, -136.00f, 79.00f, 0.0f },    // Stonehearth GY
    { -203.00f, -112.00f, 78.00f, 0.0f },    // Snowfall GY
    { -545.00f, -399.00f, 52.00f, 0.0f },    // Iceblood GY
    { -1082.00f, -346.00f, 55.00f, 0.0f },   // Frostwolf GY
    { -1402.40f, -307.70f, 89.44f, 0.0f }    // Frostwolf Relief Hut
};

// ============================================================================
// GRAVEYARD DEFENSE POSITIONS (6 per graveyard = 42 total)
// ============================================================================

namespace GraveyardDefense
{
    constexpr std::array<Position, 6> STORMPIKE = {{
        Position(-1404.0f, -302.0f, 90.0f, 1.57f),
        Position(-1412.0f, -309.0f, 90.0f, 0.0f),
        Position(-1396.0f, -309.0f, 90.0f, 3.14f),
        Position(-1404.0f, -316.0f, 90.0f, 4.71f),
        Position(-1410.0f, -302.0f, 90.0f, 0.80f),
        Position(-1398.0f, -316.0f, 90.0f, 3.90f)
    }};

    constexpr std::array<Position, 6> STORMPIKE_AID = {{
        Position(-1361.0f, -213.0f, 99.0f, 1.57f),
        Position(-1369.0f, -220.0f, 99.0f, 0.0f),
        Position(-1353.0f, -220.0f, 99.0f, 3.14f),
        Position(-1361.0f, -228.0f, 99.0f, 4.71f),
        Position(-1367.0f, -213.0f, 99.0f, 0.80f),
        Position(-1355.0f, -228.0f, 99.0f, 3.90f)
    }};

    constexpr std::array<Position, 6> STONEHEARTH = {{
        Position(-172.0f, -129.0f, 79.0f, 1.57f),
        Position(-180.0f, -136.0f, 79.0f, 0.0f),
        Position(-164.0f, -136.0f, 79.0f, 3.14f),
        Position(-172.0f, -143.0f, 79.0f, 4.71f),
        Position(-178.0f, -129.0f, 79.0f, 0.80f),
        Position(-166.0f, -143.0f, 79.0f, 3.90f)
    }};

    constexpr std::array<Position, 6> SNOWFALL = {{
        Position(-203.0f, -105.0f, 78.0f, 1.57f),
        Position(-211.0f, -112.0f, 78.0f, 0.0f),
        Position(-195.0f, -112.0f, 78.0f, 3.14f),
        Position(-203.0f, -119.0f, 78.0f, 4.71f),
        Position(-209.0f, -105.0f, 78.0f, 0.80f),
        Position(-197.0f, -119.0f, 78.0f, 3.90f)
    }};

    constexpr std::array<Position, 6> ICEBLOOD = {{
        Position(-545.0f, -392.0f, 52.0f, 1.57f),
        Position(-553.0f, -399.0f, 52.0f, 0.0f),
        Position(-537.0f, -399.0f, 52.0f, 3.14f),
        Position(-545.0f, -406.0f, 52.0f, 4.71f),
        Position(-551.0f, -392.0f, 52.0f, 0.80f),
        Position(-539.0f, -406.0f, 52.0f, 3.90f)
    }};

    constexpr std::array<Position, 6> FROSTWOLF = {{
        Position(-1082.0f, -339.0f, 55.0f, 1.57f),
        Position(-1090.0f, -346.0f, 55.0f, 0.0f),
        Position(-1074.0f, -346.0f, 55.0f, 3.14f),
        Position(-1082.0f, -353.0f, 55.0f, 4.71f),
        Position(-1088.0f, -339.0f, 55.0f, 0.80f),
        Position(-1076.0f, -353.0f, 55.0f, 3.90f)
    }};

    constexpr std::array<Position, 6> FROSTWOLF_HUT = {{
        Position(-1402.0f, -300.0f, 89.0f, 1.57f),
        Position(-1410.0f, -307.0f, 89.0f, 0.0f),
        Position(-1394.0f, -307.0f, 89.0f, 3.14f),
        Position(-1402.0f, -314.0f, 89.0f, 4.71f),
        Position(-1408.0f, -300.0f, 89.0f, 0.80f),
        Position(-1396.0f, -314.0f, 89.0f, 3.90f)
    }};
}

// ============================================================================
// BOSS ROOM POSITIONS
// ============================================================================

namespace BossRoomPositions
{
    // Vanndar Stormpike raid positions (Dun Baldar)
    constexpr std::array<Position, 12> VANNDAR_RAID = {{
        Position(-1367.0f, -210.0f, 98.0f, 4.71f),   // Main tank
        Position(-1373.0f, -215.0f, 98.0f, 5.20f),   // Off tank
        Position(-1360.0f, -215.0f, 98.0f, 4.20f),   // Melee 1
        Position(-1375.0f, -220.0f, 98.0f, 5.50f),   // Melee 2
        Position(-1358.0f, -220.0f, 98.0f, 3.90f),   // Melee 3
        Position(-1378.0f, -225.0f, 98.0f, 5.80f),   // Melee 4
        Position(-1355.0f, -225.0f, 98.0f, 3.60f),   // Melee 5
        Position(-1380.0f, -230.0f, 98.0f, 0.0f),    // Ranged left
        Position(-1352.0f, -230.0f, 98.0f, 3.14f),   // Ranged right
        Position(-1370.0f, -235.0f, 98.0f, 4.71f),   // Ranged center
        Position(-1385.0f, -235.0f, 98.0f, 0.50f),   // Healer left
        Position(-1347.0f, -235.0f, 98.0f, 2.60f)    // Healer right
    }};

    // Drek'Thar raid positions (Frostwolf Keep)
    constexpr std::array<Position, 12> DREKTHAR_RAID = {{
        Position(-1358.0f, -296.0f, 89.0f, 1.57f),   // Main tank
        Position(-1364.0f, -300.0f, 89.0f, 2.10f),   // Off tank
        Position(-1352.0f, -300.0f, 89.0f, 1.10f),   // Melee 1
        Position(-1366.0f, -305.0f, 89.0f, 2.40f),   // Melee 2
        Position(-1350.0f, -305.0f, 89.0f, 0.80f),   // Melee 3
        Position(-1368.0f, -310.0f, 89.0f, 2.70f),   // Melee 4
        Position(-1348.0f, -310.0f, 89.0f, 0.50f),   // Melee 5
        Position(-1370.0f, -315.0f, 89.0f, 3.14f),   // Ranged left
        Position(-1346.0f, -315.0f, 89.0f, 0.0f),    // Ranged right
        Position(-1361.0f, -320.0f, 89.0f, 1.57f),   // Ranged center
        Position(-1375.0f, -320.0f, 89.0f, 3.50f),   // Healer left
        Position(-1341.0f, -320.0f, 89.0f, 5.80f)    // Healer right
    }};
}

// ============================================================================
// SPAWN POSITIONS
// ============================================================================

constexpr Position ALLIANCE_SPAWNS[] = {
    { 873.98f, -491.79f, 96.54f, 3.14f },
    { 869.98f, -496.79f, 96.54f, 3.14f },
    { 878.98f, -486.79f, 96.54f, 3.14f },
    { 864.98f, -501.79f, 96.54f, 3.14f },
    { 883.98f, -481.79f, 96.54f, 3.14f }
};

constexpr Position HORDE_SPAWNS[] = {
    { -1437.00f, -610.00f, 51.16f, 0.0f },
    { -1442.00f, -605.00f, 51.16f, 0.0f },
    { -1432.00f, -615.00f, 51.16f, 0.0f },
    { -1447.00f, -600.00f, 51.16f, 0.0f },
    { -1427.00f, -620.00f, 51.16f, 0.0f }
};

// ============================================================================
// STRATEGIC POSITIONS
// ============================================================================

namespace StrategicPositions
{
    // Chokepoints (key map control points)
    constexpr std::array<Position, 10> CHOKEPOINTS = {{
        Position(-257.00f, -282.00f, 6.00f, 0.0f),     // Field of Strife center
        Position(-200.00f, -350.00f, 10.00f, 0.0f),    // Field of Strife south
        Position(-300.00f, -220.00f, 8.00f, 0.0f),     // Field of Strife north
        Position(-520.00f, -350.00f, 52.00f, 0.0f),    // Iceblood Garrison area
        Position(-168.00f, -130.00f, 79.00f, 0.0f),    // Stonehearth Outpost area
        Position(619.00f, -60.00f, 41.00f, 0.0f),      // Dun Baldar Bridge
        Position(-1230.00f, -340.00f, 60.00f, 0.0f),   // Frostwolf Keep entrance
        Position(-700.00f, -330.00f, 50.00f, 0.0f),    // Iceblood choke
        Position(-50.00f, -200.00f, 35.00f, 0.0f),     // Stonehearth approach
        Position(400.00f, -350.00f, 60.00f, 0.0f)      // Alliance bridge approach
    }};

    // Sniper/overlook positions
    constexpr std::array<Position, 8> SNIPER_POSITIONS = {{
        Position(-1380.00f, -325.00f, 115.00f, 0.0f),  // Dun Baldar North overlook
        Position(-180.00f, -450.00f, 40.00f, 0.0f),    // Icewing overlook
        Position(-160.00f, -95.00f, 85.00f, 0.0f),     // Stonehearth overlook
        Position(-575.00f, -370.00f, 98.00f, 0.0f),    // Iceblood overlook
        Position(-1310.00f, -320.00f, 120.00f, 0.0f),  // East Frostwolf overlook
        Position(-1305.00f, -275.00f, 120.00f, 0.0f),  // West Frostwolf overlook
        Position(-250.00f, -300.00f, 20.00f, 0.0f),    // Field of Strife hill
        Position(-1090.00f, -360.00f, 65.00f, 0.0f)    // Frostwolf GY overlook
    }};

    // Ambush positions (faction-specific)
    namespace Ambush
    {
        constexpr std::array<Position, 6> ALLIANCE = {{
            Position(-220.00f, -250.00f, 15.00f, 0.0f),   // Field approach
            Position(-100.00f, -150.00f, 70.00f, 0.0f),   // Stonehearth road
            Position(-480.00f, -320.00f, 55.00f, 0.0f),   // Before Iceblood
            Position(-800.00f, -350.00f, 52.00f, 0.0f),   // Iceblood to Frostwolf
            Position(-1150.00f, -340.00f, 58.00f, 0.0f),  // Frostwolf approach
            Position(-1280.00f, -290.00f, 70.00f, 0.0f)   // Keep entrance
        }};

        constexpr std::array<Position, 6> HORDE = {{
            Position(-280.00f, -310.00f, 10.00f, 0.0f),   // Field approach
            Position(-350.00f, -200.00f, 20.00f, 0.0f),   // North field
            Position(-130.00f, -180.00f, 75.00f, 0.0f),   // Before Stonehearth
            Position(200.00f, -250.00f, 55.00f, 0.0f),    // Stonehearth to bridge
            Position(500.00f, -150.00f, 50.00f, 0.0f),    // Bridge approach
            Position(700.00f, -80.00f, 45.00f, 0.0f)      // Dun Baldar entrance
        }};
    }
}

// ============================================================================
// MINE DATA
// ============================================================================

namespace Mines
{
    constexpr Position IRONDEEP_MINE = { 900.00f, -365.00f, 61.00f, 0.0f };
    constexpr Position COLDTOOTH_MINE = { -1093.00f, -271.00f, 54.00f, 0.0f };

    // Mine control positions
    constexpr std::array<Position, 4> IRONDEEP_CONTROL = {{
        Position(905.00f, -360.00f, 61.00f, 0.0f),
        Position(895.00f, -370.00f, 61.00f, 0.0f),
        Position(910.00f, -370.00f, 61.00f, 0.0f),
        Position(890.00f, -360.00f, 61.00f, 0.0f)
    }};

    constexpr std::array<Position, 4> COLDTOOTH_CONTROL = {{
        Position(-1088.00f, -266.00f, 54.00f, 0.0f),
        Position(-1098.00f, -276.00f, 54.00f, 0.0f),
        Position(-1083.00f, -276.00f, 54.00f, 0.0f),
        Position(-1098.00f, -266.00f, 54.00f, 0.0f)
    }};
}

// ============================================================================
// WORLD STATES
// ============================================================================

namespace WorldStates
{
    // Reinforcements
    constexpr int32 REINF_ALLY = 3127;
    constexpr int32 REINF_HORDE = 3128;

    // Tower states - Alliance Bunkers
    constexpr int32 DB_NORTH_ALLY = 1326;
    constexpr int32 DB_NORTH_HORDE = 1327;
    constexpr int32 DB_SOUTH_ALLY = 1325;
    constexpr int32 DB_SOUTH_HORDE = 1324;
    constexpr int32 IW_BUNKER_ALLY = 1329;
    constexpr int32 IW_BUNKER_HORDE = 1330;
    constexpr int32 SH_BUNKER_ALLY = 1331;
    constexpr int32 SH_BUNKER_HORDE = 1332;

    // Tower states - Horde Towers
    constexpr int32 TOWER_POINT_HORDE = 1377;
    constexpr int32 TOWER_POINT_ALLY = 1378;
    constexpr int32 IB_TOWER_HORDE = 1379;
    constexpr int32 IB_TOWER_ALLY = 1380;
    constexpr int32 EF_TOWER_HORDE = 1381;
    constexpr int32 EF_TOWER_ALLY = 1382;
    constexpr int32 WF_TOWER_HORDE = 1383;
    constexpr int32 WF_TOWER_ALLY = 1384;

    // Graveyard states
    constexpr int32 SNOWFALL_NEUTRAL = 1966;
    constexpr int32 SNOWFALL_ALLY = 1341;
    constexpr int32 SNOWFALL_HORDE = 1342;
    constexpr int32 STONEHEARTH_ALLY = 1301;
    constexpr int32 ICEBLOOD_HORDE = 1346;
    constexpr int32 FROSTWOLF_HORDE = 1348;
}

// ============================================================================
// DISTANCE MATRIX
// ============================================================================

namespace DistanceMatrix
{
    // Spawn to key objectives
    constexpr float ALLY_SPAWN_TO_BOSS = 2400.0f;       // To Drek'Thar
    constexpr float HORDE_SPAWN_TO_BOSS = 2200.0f;      // To Vanndar

    // Boss room distances
    constexpr float VANNDAR_TO_DB_NORTH = 95.0f;
    constexpr float VANNDAR_TO_DB_SOUTH = 15.0f;
    constexpr float DREKTHAR_TO_EF_TOWER = 60.0f;
    constexpr float DREKTHAR_TO_WF_TOWER = 45.0f;

    // Tower distances from spawn
    constexpr float ALLY_TO_STONEHEARTH = 1050.0f;
    constexpr float ALLY_TO_ICEWING = 750.0f;
    constexpr float HORDE_TO_TOWER_POINT = 900.0f;
    constexpr float HORDE_TO_ICEBLOOD = 870.0f;

    // Total map length (spawn to spawn)
    constexpr float MAP_LENGTH = 3500.0f;
}

// ============================================================================
// RUSH ROUTES
// ============================================================================

// Alliance to Horde boss rush route
inline std::vector<Position> GetAllianceRushRoute()
{
    return {
        Position(873.98f, -491.79f, 96.54f, 0.0f),      // Alliance spawn
        Position(400.0f, -350.0f, 60.0f, 0.0f),         // Bridge approach
        Position(-168.00f, -130.00f, 79.00f, 0.0f),     // Stonehearth bypass
        Position(-520.00f, -350.00f, 52.00f, 0.0f),     // Iceblood area
        Position(-1082.00f, -346.00f, 55.00f, 0.0f),    // Frostwolf GY
        Position(-1230.00f, -340.00f, 60.00f, 0.0f),    // Keep entrance
        Position(Bosses::DREKTHAR_X, Bosses::DREKTHAR_Y, Bosses::DREKTHAR_Z, 0.0f)
    };
}

// Horde to Alliance boss rush route
inline std::vector<Position> GetHordeRushRoute()
{
    return {
        Position(-1437.00f, -610.00f, 51.16f, 0.0f),    // Horde spawn
        Position(-1082.00f, -346.00f, 55.00f, 0.0f),    // Frostwolf GY
        Position(-520.00f, -350.00f, 52.00f, 0.0f),     // Iceblood area
        Position(-168.00f, -130.00f, 79.00f, 0.0f),     // Stonehearth bypass
        Position(400.0f, -200.0f, 55.0f, 0.0f),         // Dun Baldar approach
        Position(619.00f, -60.00f, 41.00f, 0.0f),       // Dun Baldar Bridge
        Position(Bosses::VANNDAR_X, Bosses::VANNDAR_Y, Bosses::VANNDAR_Z, 0.0f)
    };
}

// Tower burn routes
inline std::vector<Position> GetAllianceTowerBurnRoute()
{
    return {
        Position(-570.00f, -262.00f, 75.00f, 0.0f),     // Tower Point first
        Position(-572.00f, -359.00f, 90.00f, 0.0f),     // Iceblood Tower
        Position(-1302.00f, -315.00f, 113.87f, 0.0f),   // East Frostwolf
        Position(-1297.00f, -269.00f, 114.14f, 0.0f)    // West Frostwolf
    };
}

inline std::vector<Position> GetHordeTowerBurnRoute()
{
    return {
        Position(-155.87f, -87.37f, 79.08f, 0.0f),      // Stonehearth Bunker first
        Position(-173.00f, -440.00f, 33.00f, 0.0f),     // Icewing Bunker
        Position(-1367.40f, -221.20f, 98.43f, 0.0f),    // Dun Baldar South
        Position(-1368.30f, -313.10f, 107.14f, 0.0f)    // Dun Baldar North
    };
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

inline const char* GetTowerName(uint32 towerId)
{
    switch (towerId)
    {
        case Towers::DUN_BALDAR_NORTH:  return "Dun Baldar North Bunker";
        case Towers::DUN_BALDAR_SOUTH:  return "Dun Baldar South Bunker";
        case Towers::ICEWING_BUNKER:    return "Icewing Bunker";
        case Towers::STONEHEARTH_BUNKER: return "Stonehearth Bunker";
        case Towers::TOWER_POINT:       return "Tower Point";
        case Towers::ICEBLOOD_TOWER:    return "Iceblood Tower";
        case Towers::EAST_FROSTWOLF:    return "East Frostwolf Tower";
        case Towers::WEST_FROSTWOLF:    return "West Frostwolf Tower";
        default: return "Unknown Tower";
    }
}

inline const char* GetGraveyardName(uint32 gyId)
{
    switch (gyId)
    {
        case Graveyards::STORMPIKE_GY:         return "Stormpike Graveyard";
        case Graveyards::STORMPIKE_AID_STATION: return "Stormpike Aid Station";
        case Graveyards::STONEHEARTH_GY:       return "Stonehearth Graveyard";
        case Graveyards::SNOWFALL_GY:          return "Snowfall Graveyard";
        case Graveyards::ICEBLOOD_GY:          return "Iceblood Graveyard";
        case Graveyards::FROSTWOLF_GY:         return "Frostwolf Graveyard";
        case Graveyards::FROSTWOLF_RELIEF_HUT: return "Frostwolf Relief Hut";
        default: return "Unknown Graveyard";
    }
}

inline bool IsAllianceTower(uint32 towerId)
{
    return towerId <= Towers::STONEHEARTH_BUNKER;
}

inline bool IsHordeTower(uint32 towerId)
{
    return towerId >= Towers::TOWER_POINT && towerId <= Towers::WEST_FROSTWOLF;
}

inline Position GetTowerPosition(uint32 towerId)
{
    if (towerId < Towers::COUNT)
        return TOWER_POSITIONS[towerId];
    return Position(0, 0, 0, 0);
}

inline Position GetGraveyardPosition(uint32 gyId)
{
    if (gyId < Graveyards::COUNT)
        return GRAVEYARD_POSITIONS[gyId];
    return Position(0, 0, 0, 0);
}

inline Position GetVanndarPosition()
{
    return Position(Bosses::VANNDAR_X, Bosses::VANNDAR_Y, Bosses::VANNDAR_Z, Bosses::VANNDAR_O);
}

inline Position GetDrekTharPosition()
{
    return Position(Bosses::DREKTHAR_X, Bosses::DREKTHAR_Y, Bosses::DREKTHAR_Z, Bosses::DREKTHAR_O);
}

inline Position GetBalindaPosition()
{
    return Position(Captains::BALINDA_X, Captains::BALINDA_Y, Captains::BALINDA_Z, Captains::BALINDA_O);
}

inline Position GetGalvangarPosition()
{
    return Position(Captains::GALVANGAR_X, Captains::GALVANGAR_Y, Captains::GALVANGAR_Z, Captains::GALVANGAR_O);
}

inline std::vector<Position> GetTowerDefensePositions(uint32 towerId)
{
    std::vector<Position> positions;
    switch (towerId)
    {
        case Towers::DUN_BALDAR_NORTH:
            for (const auto& pos : TowerDefense::DUN_BALDAR_NORTH) positions.push_back(pos);
            break;
        case Towers::DUN_BALDAR_SOUTH:
            for (const auto& pos : TowerDefense::DUN_BALDAR_SOUTH) positions.push_back(pos);
            break;
        case Towers::ICEWING_BUNKER:
            for (const auto& pos : TowerDefense::ICEWING) positions.push_back(pos);
            break;
        case Towers::STONEHEARTH_BUNKER:
            for (const auto& pos : TowerDefense::STONEHEARTH) positions.push_back(pos);
            break;
        case Towers::TOWER_POINT:
            for (const auto& pos : TowerDefense::TOWER_POINT) positions.push_back(pos);
            break;
        case Towers::ICEBLOOD_TOWER:
            for (const auto& pos : TowerDefense::ICEBLOOD) positions.push_back(pos);
            break;
        case Towers::EAST_FROSTWOLF:
            for (const auto& pos : TowerDefense::EAST_FROSTWOLF) positions.push_back(pos);
            break;
        case Towers::WEST_FROSTWOLF:
            for (const auto& pos : TowerDefense::WEST_FROSTWOLF) positions.push_back(pos);
            break;
    }
    return positions;
}

inline std::vector<Position> GetGraveyardDefensePositions(uint32 gyId)
{
    std::vector<Position> positions;
    switch (gyId)
    {
        case Graveyards::STORMPIKE_GY:
            for (const auto& pos : GraveyardDefense::STORMPIKE) positions.push_back(pos);
            break;
        case Graveyards::STORMPIKE_AID_STATION:
            for (const auto& pos : GraveyardDefense::STORMPIKE_AID) positions.push_back(pos);
            break;
        case Graveyards::STONEHEARTH_GY:
            for (const auto& pos : GraveyardDefense::STONEHEARTH) positions.push_back(pos);
            break;
        case Graveyards::SNOWFALL_GY:
            for (const auto& pos : GraveyardDefense::SNOWFALL) positions.push_back(pos);
            break;
        case Graveyards::ICEBLOOD_GY:
            for (const auto& pos : GraveyardDefense::ICEBLOOD) positions.push_back(pos);
            break;
        case Graveyards::FROSTWOLF_GY:
            for (const auto& pos : GraveyardDefense::FROSTWOLF) positions.push_back(pos);
            break;
        case Graveyards::FROSTWOLF_RELIEF_HUT:
            for (const auto& pos : GraveyardDefense::FROSTWOLF_HUT) positions.push_back(pos);
            break;
    }
    return positions;
}

inline std::vector<Position> GetChokepoints()
{
    std::vector<Position> positions;
    for (const auto& pos : StrategicPositions::CHOKEPOINTS)
        positions.push_back(pos);
    return positions;
}

inline std::vector<Position> GetSniperPositions()
{
    std::vector<Position> positions;
    for (const auto& pos : StrategicPositions::SNIPER_POSITIONS)
        positions.push_back(pos);
    return positions;
}

inline std::vector<Position> GetAmbushPositions(uint32 faction)
{
    std::vector<Position> positions;
    if (faction == 1)  // ALLIANCE
    {
        for (const auto& pos : StrategicPositions::Ambush::ALLIANCE)
            positions.push_back(pos);
    }
    else
    {
        for (const auto& pos : StrategicPositions::Ambush::HORDE)
            positions.push_back(pos);
    }
    return positions;
}

inline std::vector<Position> GetBossRaidPositions(uint32 targetFaction)
{
    std::vector<Position> positions;
    if (targetFaction == 1)  // Attacking Alliance (Vanndar)
    {
        for (const auto& pos : BossRoomPositions::VANNDAR_RAID)
            positions.push_back(pos);
    }
    else  // Attacking Horde (Drek'Thar)
    {
        for (const auto& pos : BossRoomPositions::DREKTHAR_RAID)
            positions.push_back(pos);
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

// Faction-based route helpers
inline std::vector<Position> GetRushRoute(uint32 faction)
{
    return faction == 1 ? GetAllianceRushRoute() : GetHordeRushRoute();
}

inline std::vector<Position> GetTowerBurnRoute(uint32 faction)
{
    return faction == 1 ? GetAllianceTowerBurnRoute() : GetHordeTowerBurnRoute();
}

inline Position GetCaptainPosition(uint32 faction)
{
    return faction == 1 ? GetBalindaPosition() : GetGalvangarPosition();
}

// Tower world state helper structure
struct TowerWorldStateInfo
{
    int32 allyControlled;
    int32 hordeControlled;
    int32 destroyed;
};

// Tower world states array (indexed by tower ID)
constexpr std::array<TowerWorldStateInfo, 8> TOWER_WORLD_STATES = {{
    { WorldStates::DB_NORTH_ALLY, WorldStates::DB_NORTH_HORDE, 0 },   // Dun Baldar North
    { WorldStates::DB_SOUTH_ALLY, WorldStates::DB_SOUTH_HORDE, 0 },   // Dun Baldar South
    { WorldStates::IW_BUNKER_ALLY, WorldStates::IW_BUNKER_HORDE, 0 }, // Icewing Bunker
    { WorldStates::SH_BUNKER_ALLY, WorldStates::SH_BUNKER_HORDE, 0 }, // Stonehearth Bunker
    { WorldStates::TOWER_POINT_ALLY, WorldStates::TOWER_POINT_HORDE, 0 },  // Tower Point
    { WorldStates::IB_TOWER_ALLY, WorldStates::IB_TOWER_HORDE, 0 },   // Iceblood Tower
    { WorldStates::EF_TOWER_ALLY, WorldStates::EF_TOWER_HORDE, 0 },   // East Frostwolf
    { WorldStates::WF_TOWER_ALLY, WorldStates::WF_TOWER_HORDE, 0 }    // West Frostwolf
}};

// Forward-declared boss entries at namespace level for convenience
constexpr uint32 VANNDAR_ENTRY = Bosses::VANNDAR_ENTRY;
constexpr uint32 DREKTHAR_ENTRY = Bosses::DREKTHAR_ENTRY;

} // namespace Playerbot::Coordination::Battleground::AlteracValley

#endif // PLAYERBOT_AI_COORDINATION_BG_SIEGE_ALTERACVALLEYDATA_H
