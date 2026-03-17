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

#ifndef PLAYERBOT_AI_COORDINATION_BG_EPIC_ASHRANDATA_H
#define PLAYERBOT_AI_COORDINATION_BG_EPIC_ASHRANDATA_H

#include "Define.h"
#include "Position.h"
#include <array>
#include <vector>
#include <cmath>

namespace Playerbot::Coordination::Battleground::Ashran
{

// ============================================================================
// CORE CONSTANTS
// ============================================================================

constexpr uint32 MAP_ID = 1191;
constexpr char BG_NAME[] = "Ashran";
constexpr uint32 MAX_DURATION = 0;  // No time limit (persistent zone)
constexpr uint8 MIN_TEAM_SIZE = 10;
constexpr uint8 MAX_TEAM_SIZE = 75;

// ============================================================================
// OBJECTIVE IDENTIFIERS
// ============================================================================

namespace ObjectiveIds
{
    // Control Points (Road of Glory)
    constexpr uint32 STORMSHIELD_STRONGHOLD = 0;  // Alliance stronghold
    constexpr uint32 CROSSROADS = 1;               // Central control point
    constexpr uint32 WARSPEAR_STRONGHOLD = 2;      // Horde stronghold

    // Boss objectives
    constexpr uint32 TREMBLADE = 100;              // Grand Marshal Tremblade (Alliance leader)
    constexpr uint32 VOLRATH = 101;                // High Warlord Volrath (Horde leader)

    // Event objectives (102-109)
    constexpr uint32 EVENT_RACE_SUPREMACY = 102;
    constexpr uint32 EVENT_RING_CONQUEST = 103;
    constexpr uint32 EVENT_SEAT_OMEN = 104;
    constexpr uint32 EVENT_EMPOWERED_ORE = 105;
    constexpr uint32 EVENT_ANCIENT_ARTIFACT = 106;
    constexpr uint32 EVENT_STADIUM_RACING = 107;
    constexpr uint32 EVENT_OGRE_FIRES = 108;
    constexpr uint32 EVENT_BRUTE_ASSAULT = 109;
}

// ============================================================================
// CONTROL POINTS (Road of Glory)
// ============================================================================

namespace ControlPoints
{
    constexpr uint32 STORMSHIELD_STRONGHOLD = 0;  // Alliance base
    constexpr uint32 CROSSROADS = 1;               // Center control
    constexpr uint32 WARSPEAR_STRONGHOLD = 2;      // Horde base
    constexpr uint32 CONTROL_POINT_COUNT = 3;
}

// ============================================================================
// SIDE EVENTS
// ============================================================================

namespace Events
{
    constexpr uint32 RACE_FOR_SUPREMACY = 0;       // Racing event
    constexpr uint32 RING_OF_CONQUEST = 1;         // Arena battles
    constexpr uint32 SEAT_OF_OMEN = 2;             // Boss event
    constexpr uint32 EMPOWERED_ORE = 3;            // Mining event
    constexpr uint32 ANCIENT_ARTIFACT = 4;         // Artifact hunt
    constexpr uint32 STADIUM_RACING = 5;           // Motorcycle racing
    constexpr uint32 OGRE_FIRES = 6;               // Fire lighting
    constexpr uint32 BRUTE_ASSAULT = 7;            // NPC assault
    constexpr uint32 EVENT_COUNT = 8;

    // Event priority values (higher = more important)
    constexpr uint8 PRIORITY_SEAT_OF_OMEN = 10;    // Kills enemy boss - highest
    constexpr uint8 PRIORITY_RING_OF_CONQUEST = 8; // Strong buff
    constexpr uint8 PRIORITY_EMPOWERED_ORE = 7;    // Resource gathering
    constexpr uint8 PRIORITY_ANCIENT_ARTIFACT = 6; // Good buffs
    constexpr uint8 PRIORITY_BRUTE_ASSAULT = 5;    // NPC reinforcements
    constexpr uint8 PRIORITY_RACE_SUPREMACY = 4;   // Speed buff
    constexpr uint8 PRIORITY_STADIUM_RACING = 3;   // Fun but low value
    constexpr uint8 PRIORITY_OGRE_FIRES = 2;       // Minor buffs
}

// ============================================================================
// STRATEGY CONSTANTS
// ============================================================================

namespace Strategy
{
    // Road of Glory thresholds
    constexpr float BOSS_PUSH_THRESHOLD = 0.85f;        // Progress needed to push boss
    constexpr float DEEP_PUSH_THRESHOLD = 0.70f;        // Consider aggressive push
    constexpr float DEFENSIVE_THRESHOLD = 0.60f;        // Enemy progress triggers defense
    constexpr float CROSSROADS_CONTROL_BONUS = 0.15f;   // Bonus progress when holding crossroads

    // Team allocation
    constexpr uint8 MIN_ROAD_PUSHERS = 30;              // Minimum % for road push
    constexpr uint8 MAX_EVENT_PARTICIPANTS = 25;        // Maximum % for side events
    constexpr uint8 MIN_BASE_DEFENDERS = 10;            // Minimum % defending base
    constexpr uint8 BOSS_ASSAULT_TEAM_SIZE = 35;        // % for boss kill attempt

    // Timing
    constexpr uint32 EVENT_SPAWN_INTERVAL = 300000;     // 5 minutes between events
    constexpr uint32 ROAD_UPDATE_INTERVAL = 5000;       // Check road progress every 5s
    constexpr uint32 STRATEGY_UPDATE_INTERVAL = 10000;  // Re-evaluate strategy every 10s
    constexpr uint32 BOSS_RESPAWN_TIME = 600000;        // 10 minutes after death

    // Group sizes
    constexpr uint8 MIN_ROAD_GROUP_SIZE = 5;
    constexpr uint8 OPTIMAL_ROAD_GROUP_SIZE = 15;
    constexpr uint8 MIN_EVENT_GROUP_SIZE = 5;
    constexpr uint8 MIN_BOSS_GROUP_SIZE = 20;

    // Defense response
    constexpr uint32 DEFENSE_RESPONSE_TIME = 15000;     // 15s to respond to threats
    constexpr float REINFORCE_DISTANCE = 80.0f;         // Range to call for reinforcements
}

// ============================================================================
// BOSS DATA
// ============================================================================

constexpr uint32 HIGH_WARLORD_VOLRATH = 82882;     // Alliance target (in Horde base)
constexpr uint32 GRAND_MARSHAL_TREMBLADE = 82877;  // Horde target (in Alliance base)

// Boss positions (these are the faction leaders in enemy territory)
constexpr float VOLRATH_X = 4001.0f;
constexpr float VOLRATH_Y = -4088.0f;
constexpr float VOLRATH_Z = 52.0f;
constexpr float VOLRATH_O = 0.0f;

constexpr float TREMBLADE_X = 5178.0f;
constexpr float TREMBLADE_Y = -4117.0f;
constexpr float TREMBLADE_Z = 1.0f;
constexpr float TREMBLADE_O = 3.14f;

// ============================================================================
// CONTROL POINT POSITIONS
// ============================================================================

constexpr Position CONTROL_POSITIONS[] = {
    Position(4982.0f, -4171.0f, 15.0f, 0),   // Stormshield Stronghold (Alliance base entrance)
    Position(4585.0f, -4117.0f, 32.0f, 0),   // Crossroads (center)
    Position(4188.0f, -4063.0f, 50.0f, 0)    // Warspear Stronghold (Horde base entrance)
};

// ============================================================================
// SPAWN POSITIONS
// ============================================================================

constexpr float ALLIANCE_SPAWN_X = 5200.0f;
constexpr float ALLIANCE_SPAWN_Y = -4100.0f;
constexpr float ALLIANCE_SPAWN_Z = 1.0f;
constexpr float ALLIANCE_SPAWN_O = 3.14f;

constexpr float HORDE_SPAWN_X = 3970.0f;
constexpr float HORDE_SPAWN_Y = -4100.0f;
constexpr float HORDE_SPAWN_Z = 55.0f;
constexpr float HORDE_SPAWN_O = 0.0f;

// ============================================================================
// CONTROL POINT DEFENSE POSITIONS (8-10 per point)
// ============================================================================

namespace DefensePositions
{
    // Stormshield Stronghold defense (Alliance base entrance)
    constexpr std::array<Position, 10> STORMSHIELD = {{
        Position(4990.0f, -4180.0f, 16.0f, 3.14f),  // Main gate center
        Position(4975.0f, -4165.0f, 15.0f, 2.80f),  // Gate left
        Position(5005.0f, -4175.0f, 16.0f, 3.50f),  // Gate right
        Position(4970.0f, -4185.0f, 17.0f, 2.50f),  // Elevated left
        Position(5010.0f, -4185.0f, 17.0f, 3.80f),  // Elevated right
        Position(4985.0f, -4195.0f, 15.0f, 3.14f),  // Rear center
        Position(4960.0f, -4170.0f, 14.0f, 2.20f),  // Far left flank
        Position(5020.0f, -4170.0f, 14.0f, 4.00f),  // Far right flank
        Position(4995.0f, -4160.0f, 16.0f, 3.14f),  // Forward position
        Position(4980.0f, -4200.0f, 15.0f, 3.14f)   // Deep rear
    }};

    // Crossroads defense (center control point - critical)
    constexpr std::array<Position, 12> CROSSROADS = {{
        Position(4585.0f, -4117.0f, 32.0f, 0.0f),   // Center flag
        Position(4575.0f, -4107.0f, 32.0f, 5.50f),  // NW corner
        Position(4595.0f, -4107.0f, 32.0f, 0.80f),  // NE corner
        Position(4575.0f, -4127.0f, 32.0f, 4.00f),  // SW corner
        Position(4595.0f, -4127.0f, 32.0f, 2.35f),  // SE corner
        Position(4560.0f, -4117.0f, 33.0f, 4.71f),  // West road block
        Position(4610.0f, -4117.0f, 33.0f, 1.57f),  // East road block
        Position(4570.0f, -4100.0f, 34.0f, 5.20f),  // North elevated
        Position(4570.0f, -4134.0f, 34.0f, 4.20f),  // South elevated
        Position(4600.0f, -4100.0f, 34.0f, 1.00f),  // NE elevated
        Position(4600.0f, -4134.0f, 34.0f, 2.10f),  // SE elevated
        Position(4585.0f, -4090.0f, 35.0f, 0.0f)    // North overlook
    }};

    // Warspear Stronghold defense (Horde base entrance)
    constexpr std::array<Position, 10> WARSPEAR = {{
        Position(4188.0f, -4063.0f, 50.0f, 0.0f),   // Main gate center
        Position(4175.0f, -4053.0f, 50.0f, 5.50f),  // Gate left
        Position(4200.0f, -4053.0f, 50.0f, 0.80f),  // Gate right
        Position(4170.0f, -4073.0f, 51.0f, 4.50f),  // Elevated left
        Position(4205.0f, -4073.0f, 51.0f, 1.80f),  // Elevated right
        Position(4188.0f, -4080.0f, 49.0f, 0.0f),   // Rear center
        Position(4160.0f, -4063.0f, 49.0f, 4.71f),  // Far left flank
        Position(4215.0f, -4063.0f, 49.0f, 1.57f),  // Far right flank
        Position(4188.0f, -4045.0f, 51.0f, 0.0f),   // Forward position
        Position(4188.0f, -4095.0f, 49.0f, 0.0f)    // Deep rear
    }};
}

// ============================================================================
// ROAD OF GLORY POSITIONS
// ============================================================================

namespace RoadOfGlory
{
    // Main road waypoints (Alliance base to Horde base)
    inline std::vector<Position> GetWaypoints()
    {
        return {
            Position(5178.0f, -4117.0f, 1.0f, 3.14f),    // Alliance base (start)
            Position(5100.0f, -4130.0f, 5.0f, 3.30f),
            Position(5000.0f, -4150.0f, 12.0f, 3.40f),
            Position(4900.0f, -4145.0f, 18.0f, 3.20f),
            Position(4800.0f, -4135.0f, 24.0f, 3.10f),
            Position(4700.0f, -4125.0f, 28.0f, 3.14f),
            Position(4585.0f, -4117.0f, 32.0f, 3.14f),   // Crossroads (middle)
            Position(4480.0f, -4105.0f, 36.0f, 3.00f),
            Position(4380.0f, -4090.0f, 42.0f, 2.90f),
            Position(4280.0f, -4075.0f, 47.0f, 2.95f),
            Position(4188.0f, -4063.0f, 50.0f, 3.00f),   // Warspear entrance
            Position(4001.0f, -4088.0f, 52.0f, 3.14f)    // Horde base (end)
        };
    }

    // Chokepoints along the road (12 positions)
    constexpr std::array<Position, 12> CHOKEPOINTS = {{
        Position(5080.0f, -4135.0f, 8.0f, 3.30f),    // Alliance approach 1
        Position(4950.0f, -4148.0f, 15.0f, 3.35f),   // Alliance approach 2
        Position(4850.0f, -4140.0f, 21.0f, 3.20f),   // East road narrow
        Position(4750.0f, -4130.0f, 26.0f, 3.10f),   // Before crossroads east
        Position(4650.0f, -4122.0f, 30.0f, 3.08f),   // Crossroads approach east
        Position(4585.0f, -4117.0f, 32.0f, 0.0f),    // Crossroads center (critical)
        Position(4520.0f, -4110.0f, 34.0f, 2.96f),   // Crossroads approach west
        Position(4430.0f, -4098.0f, 39.0f, 2.90f),   // Before crossroads west
        Position(4330.0f, -4082.0f, 44.0f, 2.88f),   // West road narrow
        Position(4235.0f, -4070.0f, 48.0f, 2.92f),   // Horde approach 1
        Position(4120.0f, -4075.0f, 51.0f, 3.00f),   // Horde approach 2
        Position(4050.0f, -4082.0f, 52.0f, 3.10f)    // Before Horde base
    }};

    // Ambush positions along the road (faction-specific)
    namespace AmbushPositions
    {
        // Alliance ambush spots (eastern road)
        constexpr std::array<Position, 6> ALLIANCE = {{
            Position(5050.0f, -4160.0f, 10.0f, 3.14f),  // East cliff edge
            Position(4920.0f, -4170.0f, 18.0f, 3.00f),  // Behind rocks east
            Position(4820.0f, -4160.0f, 23.0f, 3.14f),  // Hill overlook east
            Position(4720.0f, -4150.0f, 27.0f, 2.90f),  // Road bend east
            Position(4650.0f, -4145.0f, 31.0f, 2.80f),  // Crossroads approach
            Position(4600.0f, -4135.0f, 33.0f, 2.50f)   // Near crossroads
        }};

        // Horde ambush spots (western road)
        constexpr std::array<Position, 6> HORDE = {{
            Position(4100.0f, -4095.0f, 51.0f, 0.0f),   // West cliff edge
            Position(4180.0f, -4085.0f, 50.0f, 0.20f),  // Behind rocks west
            Position(4280.0f, -4095.0f, 46.0f, 0.0f),   // Hill overlook west
            Position(4380.0f, -4110.0f, 41.0f, 0.30f),  // Road bend west
            Position(4480.0f, -4125.0f, 37.0f, 0.50f),  // Crossroads approach
            Position(4550.0f, -4130.0f, 34.0f, 0.70f)   // Near crossroads
        }};
    }
}

// ============================================================================
// SNIPER/OVERLOOK POSITIONS
// ============================================================================

namespace SniperPositions
{
    constexpr std::array<Position, 8> POSITIONS = {{
        Position(5150.0f, -4090.0f, 12.0f, 3.50f),   // Alliance base overlook
        Position(4950.0f, -4180.0f, 22.0f, 2.80f),   // Eastern hill
        Position(4800.0f, -4090.0f, 28.0f, 4.00f),   // Northern ridge east
        Position(4585.0f, -4080.0f, 38.0f, 3.14f),   // Crossroads tower
        Position(4585.0f, -4155.0f, 37.0f, 3.14f),   // Crossroads south cliff
        Position(4350.0f, -4050.0f, 50.0f, 2.50f),   // Northern ridge west
        Position(4200.0f, -4100.0f, 54.0f, 0.30f),   // Western hill
        Position(4030.0f, -4060.0f, 56.0f, 0.80f)    // Horde base overlook
    }};
}

// ============================================================================
// EVENT POSITIONS
// ============================================================================

namespace EventPositions
{
    // Race for Supremacy (Racing event)
    constexpr Position RACE_START = Position(4700.0f, -4250.0f, 28.0f, 0);
    constexpr Position RACE_FINISH = Position(4400.0f, -4200.0f, 40.0f, 0);
    constexpr std::array<Position, 4> RACE_DEFENSE = {{
        Position(4550.0f, -4230.0f, 33.0f, 0),
        Position(4600.0f, -4240.0f, 31.0f, 0),
        Position(4500.0f, -4220.0f, 36.0f, 0),
        Position(4450.0f, -4210.0f, 38.0f, 0)
    }};

    // Ring of Conquest (Arena battles)
    constexpr Position RING_CENTER = Position(4750.0f, -4300.0f, 25.0f, 0);
    constexpr std::array<Position, 6> RING_POSITIONS = {{
        Position(4760.0f, -4290.0f, 25.0f, 2.80f),  // North entrance
        Position(4740.0f, -4290.0f, 25.0f, 0.35f),  // North entrance left
        Position(4760.0f, -4310.0f, 25.0f, 3.50f),  // South entrance
        Position(4740.0f, -4310.0f, 25.0f, 5.90f),  // South entrance left
        Position(4770.0f, -4300.0f, 25.0f, 3.14f),  // East side
        Position(4730.0f, -4300.0f, 25.0f, 0.0f)    // West side
    }};

    // Seat of Omen (Boss event - high priority)
    constexpr Position OMEN_CENTER = Position(4400.0f, -4250.0f, 35.0f, 0);
    constexpr std::array<Position, 8> OMEN_POSITIONS = {{
        Position(4410.0f, -4240.0f, 35.0f, 3.50f),  // Tank position
        Position(4390.0f, -4240.0f, 35.0f, 5.90f),  // Tank position 2
        Position(4420.0f, -4250.0f, 35.0f, 3.14f),  // Melee right
        Position(4380.0f, -4250.0f, 35.0f, 0.0f),   // Melee left
        Position(4430.0f, -4260.0f, 36.0f, 2.80f),  // Ranged right
        Position(4370.0f, -4260.0f, 36.0f, 0.35f),  // Ranged left
        Position(4400.0f, -4270.0f, 36.0f, 3.14f),  // Healer center
        Position(4400.0f, -4230.0f, 35.0f, 3.14f)   // Back position
    }};

    // Empowered Ore (Mining event)
    constexpr Position ORE_CENTER = Position(4650.0f, -3950.0f, 30.0f, 0);
    constexpr std::array<Position, 4> ORE_NODES = {{
        Position(4660.0f, -3940.0f, 30.0f, 0),
        Position(4640.0f, -3940.0f, 30.0f, 0),
        Position(4660.0f, -3960.0f, 30.0f, 0),
        Position(4640.0f, -3960.0f, 30.0f, 0)
    }};

    // Ancient Artifact (Artifact hunt)
    constexpr Position ARTIFACT_CENTER = Position(4500.0f, -4000.0f, 34.0f, 0);
    constexpr std::array<Position, 5> ARTIFACT_SPAWNS = {{
        Position(4510.0f, -3990.0f, 34.0f, 0),
        Position(4490.0f, -3990.0f, 34.0f, 0),
        Position(4520.0f, -4010.0f, 34.0f, 0),
        Position(4480.0f, -4010.0f, 34.0f, 0),
        Position(4500.0f, -4020.0f, 34.0f, 0)
    }};

    // Stadium Racing (Motorcycle racing)
    constexpr Position STADIUM_CENTER = Position(4800.0f, -4350.0f, 22.0f, 0);
    constexpr std::array<Position, 4> STADIUM_POSITIONS = {{
        Position(4810.0f, -4340.0f, 22.0f, 0),
        Position(4790.0f, -4340.0f, 22.0f, 0),
        Position(4810.0f, -4360.0f, 22.0f, 0),
        Position(4790.0f, -4360.0f, 22.0f, 0)
    }};

    // Ogre Fires (Fire lighting)
    constexpr Position FIRES_CENTER = Position(4350.0f, -3900.0f, 42.0f, 0);
    constexpr std::array<Position, 6> FIRE_LOCATIONS = {{
        Position(4360.0f, -3890.0f, 42.0f, 0),
        Position(4340.0f, -3890.0f, 42.0f, 0),
        Position(4370.0f, -3900.0f, 42.0f, 0),
        Position(4330.0f, -3900.0f, 42.0f, 0),
        Position(4360.0f, -3910.0f, 42.0f, 0),
        Position(4340.0f, -3910.0f, 42.0f, 0)
    }};

    // Brute Assault (NPC assault)
    constexpr Position BRUTE_CENTER = Position(4200.0f, -4200.0f, 48.0f, 0);
    constexpr std::array<Position, 6> BRUTE_POSITIONS = {{
        Position(4210.0f, -4190.0f, 48.0f, 3.50f),  // Front line
        Position(4190.0f, -4190.0f, 48.0f, 5.90f),  // Front line
        Position(4220.0f, -4200.0f, 48.0f, 3.14f),  // Flank right
        Position(4180.0f, -4200.0f, 48.0f, 0.0f),   // Flank left
        Position(4210.0f, -4210.0f, 48.0f, 2.80f),  // Support right
        Position(4190.0f, -4210.0f, 48.0f, 0.35f)   // Support left
    }};
}

// ============================================================================
// BOSS APPROACH ROUTES
// ============================================================================

namespace BossRoutes
{
    // Route to attack Tremblade (for Horde attacking Alliance leader)
    inline std::vector<Position> GetTrembladeApproach()
    {
        return {
            Position(4585.0f, -4117.0f, 32.0f, 0),      // From crossroads
            Position(4700.0f, -4125.0f, 28.0f, 1.57f),
            Position(4850.0f, -4140.0f, 21.0f, 1.40f),
            Position(5000.0f, -4150.0f, 12.0f, 1.30f),
            Position(5100.0f, -4130.0f, 5.0f, 1.20f),
            Position(5178.0f, -4117.0f, 1.0f, 1.57f)    // Tremblade position
        };
    }

    // Route to attack Volrath (for Alliance attacking Horde leader)
    inline std::vector<Position> GetVolrathApproach()
    {
        return {
            Position(4585.0f, -4117.0f, 32.0f, 0),      // From crossroads
            Position(4430.0f, -4098.0f, 39.0f, 4.71f),
            Position(4280.0f, -4075.0f, 47.0f, 4.60f),
            Position(4140.0f, -4060.0f, 50.0f, 4.50f),
            Position(4050.0f, -4082.0f, 52.0f, 4.40f),
            Position(4001.0f, -4088.0f, 52.0f, 4.71f)   // Volrath position
        };
    }

    // Boss room positions (for organizing the raid)
    namespace RaidPositions
    {
        // Tremblade raid positions
        constexpr std::array<Position, 10> TREMBLADE_RAID = {{
            Position(5175.0f, -4105.0f, 1.0f, 4.71f),   // Main tank
            Position(5175.0f, -4130.0f, 1.0f, 1.57f),   // Off tank
            Position(5165.0f, -4110.0f, 1.0f, 4.20f),   // Melee 1
            Position(5165.0f, -4125.0f, 1.0f, 2.10f),   // Melee 2
            Position(5185.0f, -4110.0f, 1.0f, 5.20f),   // Melee 3
            Position(5185.0f, -4125.0f, 1.0f, 1.10f),   // Melee 4
            Position(5155.0f, -4117.0f, 2.0f, 3.14f),   // Ranged center
            Position(5145.0f, -4105.0f, 2.0f, 3.80f),   // Ranged left
            Position(5145.0f, -4130.0f, 2.0f, 2.50f),   // Ranged right
            Position(5140.0f, -4117.0f, 2.0f, 3.14f)    // Healers
        }};

        // Volrath raid positions
        constexpr std::array<Position, 10> VOLRATH_RAID = {{
            Position(4005.0f, -4075.0f, 52.0f, 1.57f),  // Main tank
            Position(4005.0f, -4100.0f, 52.0f, 4.71f),  // Off tank
            Position(4015.0f, -4080.0f, 52.0f, 2.10f),  // Melee 1
            Position(4015.0f, -4095.0f, 52.0f, 4.20f),  // Melee 2
            Position(3995.0f, -4080.0f, 52.0f, 1.10f),  // Melee 3
            Position(3995.0f, -4095.0f, 52.0f, 5.20f),  // Melee 4
            Position(4025.0f, -4088.0f, 52.0f, 0.0f),   // Ranged center
            Position(4035.0f, -4075.0f, 52.0f, 0.80f),  // Ranged left
            Position(4035.0f, -4100.0f, 52.0f, 5.50f),  // Ranged right
            Position(4040.0f, -4088.0f, 52.0f, 0.0f)    // Healers
        }};
    }
}

// ============================================================================
// DISTANCE MATRIX (pre-calculated distances)
// ============================================================================

namespace DistanceMatrix
{
    // Control point to control point distances
    constexpr float STORMSHIELD_TO_CROSSROADS = 450.0f;
    constexpr float CROSSROADS_TO_WARSPEAR = 420.0f;
    constexpr float STORMSHIELD_TO_WARSPEAR = 870.0f;

    // Spawn to control point distances
    constexpr float ALLIANCE_SPAWN_TO_STORMSHIELD = 220.0f;
    constexpr float ALLIANCE_SPAWN_TO_CROSSROADS = 650.0f;
    constexpr float ALLIANCE_SPAWN_TO_WARSPEAR = 1070.0f;
    constexpr float HORDE_SPAWN_TO_WARSPEAR = 220.0f;
    constexpr float HORDE_SPAWN_TO_CROSSROADS = 600.0f;
    constexpr float HORDE_SPAWN_TO_STORMSHIELD = 1020.0f;

    // Road length
    constexpr float TOTAL_ROAD_LENGTH = 1200.0f;
}

// ============================================================================
// WORLD STATES
// ============================================================================

namespace WorldStates
{
    constexpr int32 ROAD_PROGRESS_ALLY = 8671;
    constexpr int32 ROAD_PROGRESS_HORDE = 8672;
    constexpr int32 ACTIVE_EVENT = 8673;
    constexpr int32 TREMBLADE_HEALTH = 8674;
    constexpr int32 VOLRATH_HEALTH = 8675;
    constexpr int32 CROSSROADS_CONTROL = 8676;
}

// ============================================================================
// ARTIFACT BUFFS (items that grant temp bonuses)
// ============================================================================

namespace Artifacts
{
    constexpr uint32 BOOK_OF_MEDIVH = 114205;    // +15% damage
    constexpr uint32 NECROTIC_ORB = 114203;      // +10% health
    constexpr uint32 RACING_FLAG = 114207;       // +30% speed
    constexpr uint32 ANCIENT_RELIC = 114209;     // +10% all stats
}

// ============================================================================
// HONOR/CONQUEST POINTS
// ============================================================================

constexpr uint32 BOSS_KILL_HONOR = 500;
constexpr uint32 EVENT_WIN_HONOR = 150;
constexpr uint32 KILL_HONOR = 15;
constexpr uint32 CONTROL_POINT_HONOR = 25;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

inline Position GetSpawnPosition(uint32 faction)
{
    if (faction == 1)  // ALLIANCE
        return Position(ALLIANCE_SPAWN_X, ALLIANCE_SPAWN_Y, ALLIANCE_SPAWN_Z, ALLIANCE_SPAWN_O);
    else
        return Position(HORDE_SPAWN_X, HORDE_SPAWN_Y, HORDE_SPAWN_Z, HORDE_SPAWN_O);
}

inline Position GetControlPosition(uint32 pointId)
{
    if (pointId < ControlPoints::CONTROL_POINT_COUNT)
        return CONTROL_POSITIONS[pointId];
    return Position(0, 0, 0, 0);
}

inline const char* GetControlPointName(uint32 pointId)
{
    switch (pointId)
    {
        case ControlPoints::STORMSHIELD_STRONGHOLD: return "Stormshield Stronghold";
        case ControlPoints::CROSSROADS: return "Crossroads";
        case ControlPoints::WARSPEAR_STRONGHOLD: return "Warspear Stronghold";
        default: return "Unknown";
    }
}

inline const char* GetEventName(uint32 eventId)
{
    switch (eventId)
    {
        case Events::RACE_FOR_SUPREMACY: return "Race for Supremacy";
        case Events::RING_OF_CONQUEST: return "Ring of Conquest";
        case Events::SEAT_OF_OMEN: return "Seat of Omen";
        case Events::EMPOWERED_ORE: return "Empowered Ore";
        case Events::ANCIENT_ARTIFACT: return "Ancient Artifact";
        case Events::STADIUM_RACING: return "Stadium Racing";
        case Events::OGRE_FIRES: return "Ogre Fires";
        case Events::BRUTE_ASSAULT: return "Brute Assault";
        default: return "Unknown Event";
    }
}

inline uint8 GetEventPriority(uint32 eventId)
{
    switch (eventId)
    {
        case Events::SEAT_OF_OMEN: return Events::PRIORITY_SEAT_OF_OMEN;
        case Events::RING_OF_CONQUEST: return Events::PRIORITY_RING_OF_CONQUEST;
        case Events::EMPOWERED_ORE: return Events::PRIORITY_EMPOWERED_ORE;
        case Events::ANCIENT_ARTIFACT: return Events::PRIORITY_ANCIENT_ARTIFACT;
        case Events::BRUTE_ASSAULT: return Events::PRIORITY_BRUTE_ASSAULT;
        case Events::RACE_FOR_SUPREMACY: return Events::PRIORITY_RACE_SUPREMACY;
        case Events::STADIUM_RACING: return Events::PRIORITY_STADIUM_RACING;
        case Events::OGRE_FIRES: return Events::PRIORITY_OGRE_FIRES;
        default: return 0;
    }
}

inline Position GetTrembladePosition()
{
    return Position(TREMBLADE_X, TREMBLADE_Y, TREMBLADE_Z, TREMBLADE_O);
}

inline Position GetVolrathPosition()
{
    return Position(VOLRATH_X, VOLRATH_Y, VOLRATH_Z, VOLRATH_O);
}

inline Position GetEventCenter(uint32 eventId)
{
    switch (eventId)
    {
        case Events::RACE_FOR_SUPREMACY: return EventPositions::RACE_START;
        case Events::RING_OF_CONQUEST: return EventPositions::RING_CENTER;
        case Events::SEAT_OF_OMEN: return EventPositions::OMEN_CENTER;
        case Events::EMPOWERED_ORE: return EventPositions::ORE_CENTER;
        case Events::ANCIENT_ARTIFACT: return EventPositions::ARTIFACT_CENTER;
        case Events::STADIUM_RACING: return EventPositions::STADIUM_CENTER;
        case Events::OGRE_FIRES: return EventPositions::FIRES_CENTER;
        case Events::BRUTE_ASSAULT: return EventPositions::BRUTE_CENTER;
        default: return GetControlPosition(ControlPoints::CROSSROADS);
    }
}

inline std::vector<Position> GetControlPointDefensePositions(uint32 pointId)
{
    std::vector<Position> positions;
    switch (pointId)
    {
        case ControlPoints::STORMSHIELD_STRONGHOLD:
            for (const auto& pos : DefensePositions::STORMSHIELD)
                positions.push_back(pos);
            break;
        case ControlPoints::CROSSROADS:
            for (const auto& pos : DefensePositions::CROSSROADS)
                positions.push_back(pos);
            break;
        case ControlPoints::WARSPEAR_STRONGHOLD:
            for (const auto& pos : DefensePositions::WARSPEAR)
                positions.push_back(pos);
            break;
    }
    return positions;
}

inline float GetRoadProgressFromPosition(float x)
{
    // Calculate progress based on X coordinate
    // Alliance base is at ~5200, Horde base is at ~4000
    float progress = (5200.0f - x) / DistanceMatrix::TOTAL_ROAD_LENGTH;
    return std::max(0.0f, std::min(1.0f, progress));
}

inline std::vector<Position> GetAmbushPositions(uint32 faction)
{
    std::vector<Position> positions;
    if (faction == 1)  // ALLIANCE
    {
        for (const auto& pos : RoadOfGlory::AmbushPositions::ALLIANCE)
            positions.push_back(pos);
    }
    else
    {
        for (const auto& pos : RoadOfGlory::AmbushPositions::HORDE)
            positions.push_back(pos);
    }
    return positions;
}

inline std::vector<Position> GetRoadOfGloryWaypoints()
{
    return RoadOfGlory::GetWaypoints();
}

inline std::vector<Position> GetSniperPositions()
{
    std::vector<Position> positions;
    for (const auto& pos : SniperPositions::POSITIONS)
        positions.push_back(pos);
    return positions;
}

inline std::vector<Position> GetRoadChokepoints()
{
    std::vector<Position> positions;
    for (const auto& pos : RoadOfGlory::CHOKEPOINTS)
        positions.push_back(pos);
    return positions;
}

inline float CalculateDistance(float x1, float y1, float z1, float x2, float y2, float z2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dz = z2 - z1;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

} // namespace Playerbot::Coordination::Battleground::Ashran

#endif // PLAYERBOT_AI_COORDINATION_BG_EPIC_ASHRANDATA_H
