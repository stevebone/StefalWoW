/*
 * Copyright (C) 2025+ TrinityCore Playerbot Integration
 *
 * Enterprise-Grade Silvershard Mines Data Header
 * ~550 lines - Complete positional data for mine cart coordination
 */

#ifndef PLAYERBOT_AI_COORDINATION_BG_RESOURCERACE_SILVERSHARDMINESDATA_H
#define PLAYERBOT_AI_COORDINATION_BG_RESOURCERACE_SILVERSHARDMINESDATA_H

#include "Define.h"
#include "Position.h"
#include <array>
#include <vector>

namespace Playerbot::Coordination::Battleground::SilvershardMines
{

// ============================================================================
// MAP INFORMATION
// ============================================================================

constexpr uint32 MAP_ID = 727;
constexpr char BG_NAME[] = "Silvershard Mines";
constexpr uint32 MAX_SCORE = 1600;
constexpr uint32 MAX_DURATION = 25 * 60 * 1000;  // 25 minutes
constexpr uint8 TEAM_SIZE = 10;
constexpr uint32 CART_COUNT = 3;
constexpr uint32 POINTS_PER_CAPTURE = 200;
constexpr uint32 TICK_INTERVAL = 1000;

// ============================================================================
// STRATEGY CONSTANTS
// ============================================================================

namespace Strategy
{
    // Cart escort settings
    constexpr uint8 MIN_CART_ESCORT = 2;           // Minimum escort per cart
    constexpr uint8 OPTIMAL_CART_ESCORT = 3;       // Optimal escort size
    constexpr uint8 MAX_CART_ESCORT = 4;           // Maximum before overkill

    // Defense settings
    constexpr uint8 MIN_DEPOT_DEFENDERS = 1;       // Minimum at depot
    constexpr uint8 OPTIMAL_DEPOT_DEFENDERS = 2;   // Optimal depot defense

    // Interception settings
    constexpr uint8 INTERCEPT_TEAM_SIZE = 3;       // Size of intercept team
    constexpr float INTERCEPT_DISTANCE = 50.0f;    // Distance to set up intercept

    // Timing thresholds (milliseconds)
    constexpr uint32 OPENING_PHASE = 60000;        // First 60 seconds
    constexpr uint32 MID_GAME_THRESHOLD = 300000;  // After 5 minutes
    constexpr uint32 LATE_GAME_THRESHOLD = 600000; // Last 10 minutes
    constexpr uint32 SCORE_DESPERATE_DIFF = 400;   // Points behind for desperate

    // Intersection decision time
    constexpr uint32 INTERSECTION_DECISION_TIME = 5000;  // 5 seconds to decide

    // Cart priority multipliers
    constexpr float LAVA_PRIORITY = 1.0f;          // Longest track
    constexpr float UPPER_PRIORITY = 1.2f;         // Medium track
    constexpr float DIAMOND_PRIORITY = 1.5f;       // Shortest/fastest track

    // Escort formation radius
    constexpr float ESCORT_FORMATION_RADIUS = 8.0f;  // Close formation for carts
    constexpr float INTERCEPTION_RANGE = 30.0f;      // Range to intercept

    // Cart value assessment
    constexpr float CONTROLLED_CART_VALUE = 10.0f;
    constexpr float CONTESTED_CART_VALUE = 15.0f;   // Higher priority to secure
    constexpr float ENEMY_CART_VALUE = 12.0f;       // Value to contest enemy cart
}

// ============================================================================
// TRACK DEFINITIONS
// ============================================================================

namespace Tracks
{
    constexpr uint32 LAVA = 0;      // Bottom track (through lava area)
    constexpr uint32 UPPER = 1;     // Upper track (elevated)
    constexpr uint32 DIAMOND = 2;   // Diamond track (starts later, shortest)
    constexpr uint32 TRACK_COUNT = 3;
}

// ============================================================================
// CART SPAWN POSITIONS
// ============================================================================

constexpr Position CART_SPAWN_POSITIONS[] = {
    Position(830.0f, 190.0f, 387.0f, 0),   // Lava cart spawn (center-bottom)
    Position(746.0f, 305.0f, 402.0f, 0),   // Upper cart spawn (alliance side elevated)
    Position(933.0f, 305.0f, 404.0f, 0)    // Diamond cart spawn (horde side elevated)
};

// ============================================================================
// DEPOT POSITIONS
// ============================================================================

constexpr Position ALLIANCE_DEPOT = Position(580.0f, 200.0f, 380.0f, 0);
constexpr Position HORDE_DEPOT = Position(1050.0f, 200.0f, 380.0f, 0);

// Depot defense positions (8 per depot)
inline std::vector<Position> GetAllianceDepotDefense()
{
    return {
        // Inner defense (around depot)
        { 580.0f, 200.0f, 380.0f, 0.0f },      // Depot center
        { 590.0f, 195.0f, 380.0f, 5.5f },      // Front right
        { 590.0f, 205.0f, 380.0f, 0.8f },      // Front left
        { 570.0f, 200.0f, 380.0f, 3.14f },     // Back center

        // Outer defense (approach coverage)
        { 610.0f, 190.0f, 382.0f, 5.5f },      // Lava track approach
        { 605.0f, 225.0f, 385.0f, 0.8f },      // Upper track approach
        { 595.0f, 210.0f, 382.0f, 0.3f },      // Mid approach
        { 585.0f, 180.0f, 380.0f, 5.0f }       // South flank
    };
}

inline std::vector<Position> GetHordeDepotDefense()
{
    return {
        // Inner defense (around depot)
        { 1050.0f, 200.0f, 380.0f, 3.14f },    // Depot center
        { 1040.0f, 195.0f, 380.0f, 2.5f },     // Front right
        { 1040.0f, 205.0f, 380.0f, 3.9f },     // Front left
        { 1060.0f, 200.0f, 380.0f, 0.0f },     // Back center

        // Outer defense (approach coverage)
        { 1020.0f, 210.0f, 382.0f, 2.5f },     // Diamond track approach
        { 1025.0f, 185.0f, 380.0f, 2.3f },     // Lava track approach
        { 1035.0f, 200.0f, 382.0f, 3.14f },    // Mid approach
        { 1045.0f, 220.0f, 380.0f, 3.5f }      // North flank
    };
}

// ============================================================================
// INTERSECTION POSITIONS
// ============================================================================

namespace Intersections
{
    constexpr uint32 LAVA_UPPER = 0;      // Where lava and upper tracks can merge
    constexpr uint32 UPPER_DIAMOND = 1;   // Where upper and diamond tracks can merge
    constexpr uint32 INTERSECTION_COUNT = 2;
}

constexpr Position INTERSECTION_POSITIONS[] = {
    Position(746.0f, 248.0f, 395.0f, 0),   // Lava-Upper intersection
    Position(846.0f, 305.0f, 403.0f, 0)    // Upper-Diamond intersection
};

// Intersection control positions (positions to hold intersection)
inline std::vector<Position> GetLavaUpperIntersectionPositions()
{
    return {
        { 746.0f, 248.0f, 395.0f, 0.0f },      // Center
        { 756.0f, 258.0f, 396.0f, 5.5f },      // Northeast
        { 736.0f, 258.0f, 396.0f, 0.8f },      // Northwest
        { 746.0f, 238.0f, 394.0f, 4.7f },      // South
        { 756.0f, 238.0f, 394.0f, 5.2f },      // Southeast
        { 736.0f, 238.0f, 394.0f, 1.1f }       // Southwest
    };
}

inline std::vector<Position> GetUpperDiamondIntersectionPositions()
{
    return {
        { 846.0f, 305.0f, 403.0f, 0.0f },      // Center
        { 856.0f, 315.0f, 404.0f, 5.5f },      // Northeast
        { 836.0f, 315.0f, 404.0f, 0.8f },      // Northwest
        { 846.0f, 295.0f, 402.0f, 4.7f },      // South
        { 856.0f, 295.0f, 402.0f, 5.2f },      // Southeast
        { 836.0f, 295.0f, 402.0f, 1.1f }       // Southwest
    };
}

// ============================================================================
// SPAWN POSITIONS
// ============================================================================

constexpr float ALLIANCE_SPAWN_X = 570.0f;
constexpr float ALLIANCE_SPAWN_Y = 215.0f;
constexpr float ALLIANCE_SPAWN_Z = 380.0f;

constexpr float HORDE_SPAWN_X = 1058.0f;
constexpr float HORDE_SPAWN_Y = 215.0f;
constexpr float HORDE_SPAWN_Z = 380.0f;

constexpr Position ALLIANCE_SPAWNS[] = {
    { 570.0f, 215.0f, 380.0f, 0.0f },
    { 565.0f, 210.0f, 380.0f, 0.0f },
    { 575.0f, 210.0f, 380.0f, 0.0f },
    { 565.0f, 220.0f, 380.0f, 0.0f },
    { 575.0f, 220.0f, 380.0f, 0.0f }
};

constexpr Position HORDE_SPAWNS[] = {
    { 1058.0f, 215.0f, 380.0f, 3.14f },
    { 1053.0f, 210.0f, 380.0f, 3.14f },
    { 1063.0f, 210.0f, 380.0f, 3.14f },
    { 1053.0f, 220.0f, 380.0f, 3.14f },
    { 1063.0f, 220.0f, 380.0f, 3.14f }
};

inline Position GetSpawnPosition(uint32 faction)
{
    if (faction == 1)  // ALLIANCE
        return Position(ALLIANCE_SPAWN_X, ALLIANCE_SPAWN_Y, ALLIANCE_SPAWN_Z, 0);
    else
        return Position(HORDE_SPAWN_X, HORDE_SPAWN_Y, HORDE_SPAWN_Z, 3.14f);
}

// ============================================================================
// TRACK WAYPOINTS (Complete paths for each track)
// ============================================================================

// Lava Track - Bottom route, longest track
inline std::vector<Position> GetLavaTrackWaypoints()
{
    return {
        Position(830.0f, 190.0f, 387.0f, 0),   // Start
        Position(800.0f, 190.0f, 386.0f, 0),   // Waypoint 1
        Position(780.0f, 190.0f, 385.0f, 0),   // Waypoint 2
        Position(746.0f, 195.0f, 387.0f, 0),   // Near intersection
        Position(720.0f, 195.0f, 385.0f, 0),   // Waypoint 3
        Position(680.0f, 198.0f, 383.0f, 0),   // Waypoint 4
        Position(660.0f, 200.0f, 382.0f, 0),   // Waypoint 5
        Position(620.0f, 200.0f, 381.0f, 0),   // Approaching depot
        Position(580.0f, 200.0f, 380.0f, 0)    // Alliance depot
    };
}

// Lava Track - Horde direction
inline std::vector<Position> GetLavaTrackWaypointsHorde()
{
    return {
        Position(830.0f, 190.0f, 387.0f, 0),   // Start
        Position(860.0f, 190.0f, 386.0f, 0),   // Waypoint 1
        Position(900.0f, 192.0f, 385.0f, 0),   // Waypoint 2
        Position(940.0f, 195.0f, 383.0f, 0),   // Waypoint 3
        Position(980.0f, 198.0f, 382.0f, 0),   // Waypoint 4
        Position(1010.0f, 200.0f, 381.0f, 0),  // Approaching depot
        Position(1050.0f, 200.0f, 380.0f, 0)   // Horde depot
    };
}

// Upper Track - Alliance side elevated
inline std::vector<Position> GetUpperTrackWaypoints()
{
    return {
        Position(746.0f, 305.0f, 402.0f, 0),   // Start
        Position(730.0f, 300.0f, 400.0f, 0),   // Waypoint 1
        Position(700.0f, 295.0f, 398.0f, 0),   // Waypoint 2
        Position(670.0f, 280.0f, 395.0f, 0),   // Waypoint 3
        Position(650.0f, 260.0f, 392.0f, 0),   // Waypoint 4
        Position(620.0f, 240.0f, 388.0f, 0),   // Waypoint 5
        Position(600.0f, 230.0f, 385.0f, 0),   // Waypoint 6
        Position(580.0f, 200.0f, 380.0f, 0)    // Alliance depot
    };
}

// Upper Track - toward Horde (after intersection)
inline std::vector<Position> GetUpperTrackWaypointsHorde()
{
    return {
        Position(746.0f, 305.0f, 402.0f, 0),   // Start
        Position(780.0f, 305.0f, 403.0f, 0),   // Waypoint 1
        Position(820.0f, 305.0f, 403.0f, 0),   // Toward intersection
        Position(846.0f, 305.0f, 403.0f, 0),   // Intersection
        Position(880.0f, 305.0f, 403.0f, 0),   // Past intersection
        Position(920.0f, 290.0f, 400.0f, 0),   // Waypoint 2
        Position(960.0f, 260.0f, 395.0f, 0),   // Waypoint 3
        Position(1000.0f, 230.0f, 388.0f, 0),  // Waypoint 4
        Position(1050.0f, 200.0f, 380.0f, 0)   // Horde depot
    };
}

// Diamond Track - Horde side, shortest track
inline std::vector<Position> GetDiamondTrackWaypoints()
{
    return {
        Position(933.0f, 305.0f, 404.0f, 0),   // Start
        Position(960.0f, 290.0f, 402.0f, 0),   // Waypoint 1
        Position(980.0f, 280.0f, 400.0f, 0),   // Waypoint 2
        Position(1010.0f, 250.0f, 395.0f, 0),  // Waypoint 3
        Position(1030.0f, 230.0f, 388.0f, 0),  // Waypoint 4
        Position(1040.0f, 220.0f, 385.0f, 0),  // Waypoint 5
        Position(1050.0f, 200.0f, 380.0f, 0)   // Horde depot
    };
}

// Diamond Track - toward Alliance (reversed)
inline std::vector<Position> GetDiamondTrackWaypointsAlliance()
{
    return {
        Position(933.0f, 305.0f, 404.0f, 0),   // Start
        Position(900.0f, 305.0f, 404.0f, 0),   // Toward intersection
        Position(846.0f, 305.0f, 403.0f, 0),   // Intersection
        Position(800.0f, 300.0f, 400.0f, 0),   // Past intersection
        Position(750.0f, 280.0f, 395.0f, 0),   // Waypoint 1
        Position(700.0f, 260.0f, 390.0f, 0),   // Waypoint 2
        Position(650.0f, 230.0f, 385.0f, 0),   // Waypoint 3
        Position(580.0f, 200.0f, 380.0f, 0)    // Alliance depot
    };
}

// ============================================================================
// CHOKEPOINTS (Track interception positions)
// ============================================================================

inline std::vector<Position> GetTrackChokepoints()
{
    return {
        // Lava track chokepoints
        { 780.0f, 190.0f, 385.0f, 0.0f },      // Lava mid-alliance
        { 830.0f, 190.0f, 387.0f, 0.0f },      // Lava center (spawn)
        { 900.0f, 192.0f, 385.0f, 0.0f },      // Lava mid-horde

        // Upper track chokepoints
        { 700.0f, 295.0f, 398.0f, 0.0f },      // Upper alliance approach
        { 746.0f, 305.0f, 402.0f, 0.0f },      // Upper spawn
        { 820.0f, 305.0f, 403.0f, 0.0f },      // Upper center

        // Diamond track chokepoints
        { 880.0f, 305.0f, 403.0f, 0.0f },      // Diamond alliance approach
        { 933.0f, 305.0f, 404.0f, 0.0f },      // Diamond spawn
        { 1010.0f, 250.0f, 395.0f, 0.0f },     // Diamond horde approach

        // Intersection chokepoints
        { 746.0f, 248.0f, 395.0f, 0.0f },      // Lava-Upper intersection
        { 846.0f, 305.0f, 403.0f, 0.0f }       // Upper-Diamond intersection
    };
}

// ============================================================================
// SNIPER/OVERLOOK POSITIONS (Elevated positions with track visibility)
// ============================================================================

inline std::vector<Position> GetSniperPositions()
{
    return {
        // Alliance side elevated
        { 620.0f, 250.0f, 395.0f, 5.5f },      // Alliance depot overlook
        { 680.0f, 280.0f, 400.0f, 5.5f },      // Upper track overlook (alliance)

        // Horde side elevated
        { 1000.0f, 260.0f, 398.0f, 2.5f },     // Horde depot overlook
        { 950.0f, 280.0f, 402.0f, 2.5f },      // Diamond track overlook

        // Center elevated
        { 810.0f, 320.0f, 410.0f, 4.7f },      // Center map overlook (best visibility)
        { 846.0f, 320.0f, 410.0f, 4.7f }       // Intersection overlook
    };
}

// ============================================================================
// CART ESCORT FORMATION
// ============================================================================

// Formation positions relative to cart (offset from cart position)
inline std::vector<Position> GetCartEscortFormation()
{
    return {
        // Front escorts (direction of movement)
        { 6.0f, 0.0f, 0.0f, 0.0f },            // Point (front center)
        { 4.0f, -3.0f, 0.0f, 0.3f },           // Front right
        { 4.0f, 3.0f, 0.0f, -0.3f },           // Front left

        // Side escorts
        { 0.0f, -5.0f, 0.0f, 1.57f },          // Right flank
        { 0.0f, 5.0f, 0.0f, -1.57f },          // Left flank

        // Rear escort
        { -4.0f, 0.0f, 0.0f, 3.14f }           // Rear guard
    };
}

// ============================================================================
// AMBUSH POSITIONS (Faction-specific interception points)
// ============================================================================

inline std::vector<Position> GetAllianceAmbushPositions()
{
    return {
        // Near Horde cart spawns
        { 920.0f, 300.0f, 404.0f, 2.5f },      // Diamond track ambush
        { 860.0f, 190.0f, 387.0f, 2.5f },      // Lava track ambush (horde side)

        // Intersection ambushes
        { 860.0f, 305.0f, 403.0f, 2.5f },      // Upper-Diamond junction
        { 760.0f, 248.0f, 395.0f, 2.5f },      // Lava-Upper junction

        // Mid-track intercepts
        { 750.0f, 300.0f, 402.0f, 2.5f },      // Upper track mid
        { 720.0f, 195.0f, 385.0f, 2.5f }       // Lava track mid
    };
}

inline std::vector<Position> GetHordeAmbushPositions()
{
    return {
        // Near Alliance cart spawns
        { 760.0f, 305.0f, 402.0f, 5.5f },      // Upper track ambush
        { 800.0f, 190.0f, 386.0f, 5.5f },      // Lava track ambush (alliance side)

        // Intersection ambushes
        { 830.0f, 305.0f, 403.0f, 5.5f },      // Upper-Diamond junction
        { 730.0f, 248.0f, 395.0f, 5.5f },      // Lava-Upper junction

        // Mid-track intercepts
        { 880.0f, 305.0f, 403.0f, 5.5f },      // Diamond track mid
        { 940.0f, 195.0f, 383.0f, 5.5f }       // Lava track mid
    };
}

// ============================================================================
// TRACK TIMING
// ============================================================================

constexpr uint32 LAVA_TRACK_TIME = 90000;      // 90 seconds (longest)
constexpr uint32 UPPER_TRACK_TIME = 75000;     // 75 seconds
constexpr uint32 DIAMOND_TRACK_TIME = 60000;   // 60 seconds (shortest)

// Track distance estimates (yards)
constexpr float LAVA_TRACK_LENGTH = 470.0f;
constexpr float UPPER_TRACK_LENGTH = 380.0f;
constexpr float DIAMOND_TRACK_LENGTH = 320.0f;

// ============================================================================
// DISTANCE MATRIX (Key locations)
// ============================================================================

namespace Distances
{
    constexpr uint8 LOC_ALLIANCE_DEPOT = 0;
    constexpr uint8 LOC_HORDE_DEPOT = 1;
    constexpr uint8 LOC_LAVA_SPAWN = 2;
    constexpr uint8 LOC_UPPER_SPAWN = 3;
    constexpr uint8 LOC_DIAMOND_SPAWN = 4;
    constexpr uint8 LOC_INTERSECTION_1 = 5;
    constexpr uint8 LOC_INTERSECTION_2 = 6;
    constexpr uint8 LOC_COUNT = 7;

    // Approximate distances in yards
    constexpr std::array<std::array<float, LOC_COUNT>, LOC_COUNT> MATRIX = {{
        //  A_DEPOT H_DEPOT LAVA    UPPER   DIAMOND INTER1  INTER2
        {   0.0f,   470.0f, 250.0f, 200.0f, 400.0f, 170.0f, 280.0f },  // A_DEPOT
        {   470.0f, 0.0f,   220.0f, 350.0f, 120.0f, 300.0f, 200.0f },  // H_DEPOT
        {   250.0f, 220.0f, 0.0f,   120.0f, 150.0f, 90.0f,  120.0f },  // LAVA
        {   200.0f, 350.0f, 120.0f, 0.0f,   200.0f, 60.0f,  100.0f },  // UPPER
        {   400.0f, 120.0f, 150.0f, 200.0f, 0.0f,   180.0f, 90.0f  },  // DIAMOND
        {   170.0f, 300.0f, 90.0f,  60.0f,  180.0f, 0.0f,   100.0f },  // INTER1
        {   280.0f, 200.0f, 120.0f, 100.0f, 90.0f,  100.0f, 0.0f   }   // INTER2
    }};
}

// ============================================================================
// WORLD STATES
// ============================================================================

namespace WorldStates
{
    constexpr int32 SCORE_ALLY = 6308;
    constexpr int32 SCORE_HORDE = 6309;
    constexpr int32 CART1_STATE = 6310;   // Lava cart
    constexpr int32 CART2_STATE = 6311;   // Upper cart
    constexpr int32 CART3_STATE = 6312;   // Diamond cart

    // Cart state values
    constexpr int32 CART_NEUTRAL = 0;
    constexpr int32 CART_ALLIANCE = 1;
    constexpr int32 CART_HORDE = 2;
    constexpr int32 CART_CONTESTED = 3;
}

// ============================================================================
// GAME OBJECTS
// ============================================================================

namespace GameObjects
{
    constexpr uint32 CART_LAVA = 220478;
    constexpr uint32 CART_UPPER = 220479;
    constexpr uint32 CART_DIAMOND = 220480;

    constexpr uint32 DEPOT_ALLIANCE = 220481;
    constexpr uint32 DEPOT_HORDE = 220482;
}

// ============================================================================
// OBJECTIVE IDS
// ============================================================================

namespace ObjectiveIds
{
    constexpr uint32 CART_LAVA = 0;
    constexpr uint32 CART_UPPER = 1;
    constexpr uint32 CART_DIAMOND = 2;

    constexpr uint32 INTERSECTION_LAVA_UPPER = 50;
    constexpr uint32 INTERSECTION_UPPER_DIAMOND = 51;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

inline Position GetCartSpawnPosition(uint32 cartId)
{
    if (cartId < CART_COUNT)
        return CART_SPAWN_POSITIONS[cartId];
    return Position(0, 0, 0, 0);
}

inline Position GetIntersectionPosition(uint32 intersectionId)
{
    if (intersectionId < Intersections::INTERSECTION_COUNT)
        return INTERSECTION_POSITIONS[intersectionId];
    return Position(0, 0, 0, 0);
}

inline const char* GetTrackName(uint32 trackId)
{
    switch (trackId)
    {
        case Tracks::LAVA: return "Lava Track";
        case Tracks::UPPER: return "Upper Track";
        case Tracks::DIAMOND: return "Diamond Track";
        default: return "Unknown Track";
    }
}

inline const char* GetCartName(uint32 cartId)
{
    switch (cartId)
    {
        case 0: return "Lava Cart";
        case 1: return "Upper Cart";
        case 2: return "Diamond Cart";
        default: return "Unknown Cart";
    }
}

inline uint32 GetTrackTime(uint32 trackId)
{
    switch (trackId)
    {
        case Tracks::LAVA: return LAVA_TRACK_TIME;
        case Tracks::UPPER: return UPPER_TRACK_TIME;
        case Tracks::DIAMOND: return DIAMOND_TRACK_TIME;
        default: return UPPER_TRACK_TIME;
    }
}

inline float GetTrackPriority(uint32 trackId)
{
    switch (trackId)
    {
        case Tracks::LAVA: return Strategy::LAVA_PRIORITY;
        case Tracks::UPPER: return Strategy::UPPER_PRIORITY;
        case Tracks::DIAMOND: return Strategy::DIAMOND_PRIORITY;
        default: return 1.0f;
    }
}

// Get track waypoints based on direction
inline std::vector<Position> GetTrackWaypoints(uint32 trackId, bool towardAlliance)
{
    switch (trackId)
    {
        case Tracks::LAVA:
            return towardAlliance ? GetLavaTrackWaypoints() : GetLavaTrackWaypointsHorde();
        case Tracks::UPPER:
            return towardAlliance ? GetUpperTrackWaypoints() : GetUpperTrackWaypointsHorde();
        case Tracks::DIAMOND:
            return towardAlliance ? GetDiamondTrackWaypointsAlliance() : GetDiamondTrackWaypoints();
        default:
            return {};
    }
}

} // namespace Playerbot::Coordination::Battleground::SilvershardMines

#endif // PLAYERBOT_AI_COORDINATION_BG_RESOURCERACE_SILVERSHARDMINESDATA_H
