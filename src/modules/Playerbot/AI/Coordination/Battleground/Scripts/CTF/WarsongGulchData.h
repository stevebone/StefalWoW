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

#ifndef PLAYERBOT_AI_COORDINATION_BG_CTF_WARSONGGULCHDATA_H
#define PLAYERBOT_AI_COORDINATION_BG_CTF_WARSONGGULCHDATA_H

#include "Define.h"
#include "Position.h"

namespace Playerbot::Coordination::Battleground::WarsongGulch
{

// ============================================================================
// MAP INFORMATION
// ============================================================================

constexpr uint32 MAP_ID = 489;
constexpr uint32 MAP_ID_REMAKE = 2106;  // Warsong Gulch (Remake) - uses same logic

constexpr char BG_NAME[] = "Warsong Gulch";
constexpr uint32 MAX_SCORE = 3;
constexpr uint32 MAX_DURATION = 25 * 60 * 1000;  // 25 minutes
constexpr uint8 TEAM_SIZE = 10;

// ============================================================================
// FLAG POSITIONS
// ============================================================================

// Alliance flag position (Silverwing Hold)
constexpr float ALLIANCE_FLAG_X = 1540.423f;
constexpr float ALLIANCE_FLAG_Y = 1481.325f;
constexpr float ALLIANCE_FLAG_Z = 351.818f;
constexpr float ALLIANCE_FLAG_O = 3.089f;

// Horde flag position (Warsong base)
constexpr float HORDE_FLAG_X = 916.023f;
constexpr float HORDE_FLAG_Y = 1433.805f;
constexpr float HORDE_FLAG_Z = 346.037f;
constexpr float HORDE_FLAG_O = 0.017f;

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

// Alliance spawn points
constexpr Position ALLIANCE_SPAWNS[] = {
    { 1519.530f, 1481.870f, 352.020f, 3.141f },
    { 1519.530f, 1476.870f, 352.020f, 3.141f },
    { 1519.530f, 1486.870f, 352.020f, 3.141f },
    { 1523.530f, 1481.870f, 352.020f, 3.141f },
    { 1515.530f, 1481.870f, 352.020f, 3.141f }
};

// Horde spawn points
constexpr Position HORDE_SPAWNS[] = {
    { 933.989f, 1433.720f, 345.537f, 0.000f },
    { 933.989f, 1428.720f, 345.537f, 0.000f },
    { 933.989f, 1438.720f, 345.537f, 0.000f },
    { 929.989f, 1433.720f, 345.537f, 0.000f },
    { 937.989f, 1433.720f, 345.537f, 0.000f }
};

// ============================================================================
// GRAVEYARD POSITIONS
// ============================================================================

// Alliance graveyard
constexpr float ALLIANCE_GY_X = 1415.33f;
constexpr float ALLIANCE_GY_Y = 1554.79f;
constexpr float ALLIANCE_GY_Z = 343.156f;
constexpr float ALLIANCE_GY_O = 3.141f;

// Horde graveyard
constexpr float HORDE_GY_X = 1029.14f;
constexpr float HORDE_GY_Y = 1387.49f;
constexpr float HORDE_GY_Z = 340.836f;
constexpr float HORDE_GY_O = 0.0f;

inline Position GetAllianceGraveyard()
{
    return Position(ALLIANCE_GY_X, ALLIANCE_GY_Y, ALLIANCE_GY_Z, ALLIANCE_GY_O);
}

inline Position GetHordeGraveyard()
{
    return Position(HORDE_GY_X, HORDE_GY_Y, HORDE_GY_Z, HORDE_GY_O);
}

// ============================================================================
// FLAG ROOM DEFENSIVE POSITIONS
// ============================================================================

// Alliance flag room (Silverwing Hold) - defensive positions
inline std::vector<Position> GetAllianceFlagRoomDefense()
{
    return {
        // Main flag room
        { 1539.22f, 1480.03f, 352.018f, 3.089f },  // Flag position
        { 1531.87f, 1481.35f, 352.017f, 3.141f },  // In front of flag
        { 1535.51f, 1473.97f, 352.017f, 2.356f },  // Right side
        { 1535.51f, 1488.03f, 352.017f, 3.927f },  // Left side
        { 1525.51f, 1481.35f, 352.017f, 3.141f },  // Further back

        // Balcony positions
        { 1540.43f, 1479.53f, 362.018f, 3.089f },  // Upper balcony center
        { 1537.26f, 1472.14f, 361.017f, 2.356f },  // Upper balcony right
        { 1537.26f, 1490.14f, 361.017f, 3.927f },  // Upper balcony left

        // Tunnel entrance defense
        { 1505.22f, 1493.97f, 352.017f, 4.712f },  // Left tunnel entrance
        { 1505.22f, 1468.03f, 352.017f, 1.571f },  // Right tunnel entrance

        // Ramp entrance
        { 1492.54f, 1457.15f, 342.963f, 0.785f },  // Ramp bottom
        { 1507.04f, 1456.85f, 352.013f, 1.571f }   // Ramp top
    };
}

// Horde flag room (Warsong base) - defensive positions
inline std::vector<Position> GetHordeFlagRoomDefense()
{
    return {
        // Main flag room
        { 916.98f, 1434.14f, 346.037f, 0.017f },   // Flag position
        { 924.06f, 1433.82f, 345.903f, 0.000f },   // In front of flag
        { 920.27f, 1426.54f, 345.903f, 5.498f },   // Right side
        { 920.27f, 1441.46f, 345.903f, 0.785f },   // Left side
        { 930.27f, 1433.82f, 345.903f, 0.000f },   // Further back

        // Balcony positions
        { 918.26f, 1433.53f, 355.903f, 0.017f },   // Upper balcony center
        { 921.04f, 1426.14f, 355.903f, 5.498f },   // Upper balcony right
        { 921.04f, 1441.14f, 355.903f, 0.785f },   // Upper balcony left

        // Tunnel entrance defense
        { 949.98f, 1420.97f, 345.903f, 5.498f },   // Right tunnel entrance
        { 949.98f, 1446.03f, 345.903f, 0.785f },   // Left tunnel entrance

        // Ramp entrance
        { 963.18f, 1457.35f, 336.963f, 3.927f },   // Ramp bottom
        { 948.68f, 1458.65f, 345.903f, 4.712f }    // Ramp top
    };
}

// ============================================================================
// MIDDLE MAP POSITIONS
// ============================================================================

// Center chokepoints
inline std::vector<Position> GetMiddleChokepoints()
{
    return {
        // Main center field
        { 1227.24f, 1475.43f, 307.485f, 3.141f },  // Dead center
        { 1227.24f, 1445.43f, 306.485f, 3.141f },  // Center south
        { 1227.24f, 1505.43f, 307.485f, 3.141f },  // Center north

        // Alliance side of middle
        { 1316.15f, 1492.42f, 317.074f, 3.927f },  // Alliance approach
        { 1298.84f, 1525.04f, 313.485f, 4.712f },  // Alliance flank north
        { 1308.54f, 1445.87f, 314.963f, 2.356f },  // Alliance flank south

        // Horde side of middle
        { 1136.87f, 1456.54f, 315.743f, 0.785f },  // Horde approach
        { 1153.18f, 1388.96f, 311.485f, 1.571f },  // Horde flank south
        { 1145.88f, 1524.13f, 312.485f, 5.498f },  // Horde flank north

        // Tunnel center points
        { 1125.54f, 1524.13f, 316.743f, 0.000f },  // North tunnel middle
        { 1330.84f, 1388.96f, 317.074f, 3.141f }   // South tunnel middle
    };
}

// ============================================================================
// BUFF POSITIONS
// ============================================================================

// Speed buff locations
inline std::vector<Position> GetSpeedBuffPositions()
{
    return {
        { 1449.93f, 1470.71f, 342.634f, 0.0f },    // Alliance side speed buff
        { 1005.17f, 1447.95f, 335.903f, 0.0f }     // Horde side speed buff
    };
}

// Restoration buff locations (heal/mana)
inline std::vector<Position> GetRestoreBuffPositions()
{
    return {
        { 1317.51f, 1550.85f, 313.234f, 0.0f },    // Alliance side restore
        { 1110.45f, 1351.09f, 316.518f, 0.0f }     // Horde side restore
    };
}

// Berserk buff locations
inline std::vector<Position> GetBerserkBuffPositions()
{
    return {
        { 1320.09f, 1378.79f, 314.753f, 0.0f },    // Alliance side berserk
        { 1139.69f, 1560.29f, 306.843f, 0.0f }     // Horde side berserk
    };
}

// ============================================================================
// TUNNEL POSITIONS
// ============================================================================

// Alliance tunnel entrance
constexpr float ALLIANCE_TUNNEL_X = 1449.97f;
constexpr float ALLIANCE_TUNNEL_Y = 1468.85f;
constexpr float ALLIANCE_TUNNEL_Z = 342.634f;

// Horde tunnel entrance
constexpr float HORDE_TUNNEL_X = 1005.21f;
constexpr float HORDE_TUNNEL_Y = 1450.93f;
constexpr float HORDE_TUNNEL_Z = 335.903f;

// ============================================================================
// STRATEGIC POSITIONS
// ============================================================================

// Sniper/LOS positions
inline std::vector<Position> GetSniperPositions()
{
    return {
        // Alliance high ground
        { 1540.43f, 1479.53f, 362.018f, 3.089f },  // Alliance balcony
        { 1495.84f, 1456.85f, 352.013f, 1.571f },  // Alliance ramp top

        // Horde high ground
        { 918.26f, 1433.53f, 355.903f, 0.017f },   // Horde balcony
        { 960.68f, 1458.65f, 345.903f, 4.712f },   // Horde ramp top

        // Middle elevated
        { 1227.24f, 1475.43f, 315.485f, 3.141f }   // Center rock
    };
}

// FC kiting path (Alliance)
inline std::vector<Position> GetAllianceFCKitePath()
{
    return {
        { 1540.42f, 1481.33f, 351.818f, 3.089f },  // Flag room
        { 1500.54f, 1481.35f, 352.017f, 3.141f },  // Exit flag room
        { 1449.93f, 1470.71f, 342.634f, 5.498f },  // Speed buff
        { 1316.15f, 1492.42f, 317.074f, 3.927f },  // Mid field approach
        { 1227.24f, 1475.43f, 307.485f, 3.141f },  // Center
        { 1136.87f, 1456.54f, 315.743f, 0.785f },  // Horde approach
        { 1005.17f, 1447.95f, 335.903f, 0.785f },  // Horde speed buff
        { 916.02f, 1433.80f, 346.037f, 0.017f }    // Horde flag (cap point)
    };
}

// FC kiting path (Horde)
inline std::vector<Position> GetHordeFCKitePath()
{
    return {
        { 916.02f, 1433.80f, 346.037f, 0.017f },   // Flag room
        { 955.54f, 1433.82f, 345.903f, 0.000f },   // Exit flag room
        { 1005.17f, 1447.95f, 335.903f, 5.498f },  // Speed buff
        { 1136.87f, 1456.54f, 315.743f, 2.356f },  // Mid field approach
        { 1227.24f, 1475.43f, 307.485f, 0.000f },  // Center
        { 1316.15f, 1492.42f, 317.074f, 0.785f },  // Alliance approach
        { 1449.93f, 1470.71f, 342.634f, 0.785f },  // Alliance speed buff
        { 1540.42f, 1481.33f, 351.818f, 3.089f }   // Alliance flag (cap point)
    };
}

// ============================================================================
// WORLD STATES
// ============================================================================

namespace WorldStates
{
    // Flag states
    constexpr int32 ALLIANCE_FLAG_STATE = 1545;    // 1 = at base, 2 = picked up, 3 = dropped
    constexpr int32 HORDE_FLAG_STATE = 1546;

    // Score
    constexpr int32 ALLIANCE_FLAG_CAPTURES = 1581;
    constexpr int32 HORDE_FLAG_CAPTURES = 1582;

    // Timer
    constexpr int32 TIME_REMAINING = 4248;

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
    constexpr uint32 ALLIANCE_FLAG = 179830;
    constexpr uint32 HORDE_FLAG = 179831;

    // Doors
    constexpr uint32 ALLIANCE_DOOR_1 = 179918;
    constexpr uint32 ALLIANCE_DOOR_2 = 179919;
    constexpr uint32 HORDE_DOOR_1 = 179916;
    constexpr uint32 HORDE_DOOR_2 = 179917;
}

// ============================================================================
// OBJECTIVE IDS (for script use)
// ============================================================================

namespace ObjectiveIds
{
    constexpr uint32 ALLIANCE_FLAG = 1;
    constexpr uint32 HORDE_FLAG = 2;
}

} // namespace Playerbot::Coordination::Battleground::WarsongGulch

#endif // PLAYERBOT_AI_COORDINATION_BG_CTF_WARSONGGULCHDATA_H
