/*
 * Copyright (C) 2025+ TrinityCore Playerbot Integration
 *
 * Enterprise-Grade Seething Shore Data
 * ~700 lines - Complete strategic positioning data for AI coordination
 */

#ifndef PLAYERBOT_AI_COORDINATION_BG_DOMINATION_SEETHINGSHOREDATA_H
#define PLAYERBOT_AI_COORDINATION_BG_DOMINATION_SEETHINGSHOREDATA_H

#include "Define.h"
#include "Position.h"
#include <array>
#include <vector>
#include <cmath>

namespace Playerbot::Coordination::Battleground::SeethingShore
{

// ============================================================================
// BASIC CONFIGURATION
// ============================================================================

constexpr uint32 MAP_ID = 1803;
constexpr char BG_NAME[] = "Seething Shore";
constexpr uint32 MAX_SCORE = 1500;  // Azerite collected to win
constexpr uint32 MAX_DURATION = 12 * 60 * 1000;  // 12 minutes
constexpr uint8 TEAM_SIZE = 10;
constexpr uint32 MAX_ACTIVE_NODES = 3;  // Only 3 nodes active at a time
constexpr uint32 TICK_INTERVAL = 1000;
constexpr uint32 CAPTURE_TIME = 6000;  // 6 seconds to capture
constexpr uint32 AZERITE_PER_NODE = 100;  // Points per node capture
constexpr uint32 NODE_RESPAWN_TIME = 15000;  // 15 seconds after capture

// ============================================================================
// SPAWN ZONE ENUMERATION
// ============================================================================

namespace SpawnZones
{
    constexpr uint32 ZONE_COUNT = 12;

    // Zone IDs
    constexpr uint32 NORTH_BEACH = 0;
    constexpr uint32 NORTHWEST_HILL = 1;
    constexpr uint32 NORTHEAST_ROCKS = 2;
    constexpr uint32 WEST_CLIFF = 3;
    constexpr uint32 EAST_SHORE = 4;
    constexpr uint32 CENTER_NORTH = 5;
    constexpr uint32 WEST_SOUTH = 6;
    constexpr uint32 CENTER = 7;
    constexpr uint32 EAST_BEACH = 8;
    constexpr uint32 SOUTHWEST_HILL = 9;
    constexpr uint32 SOUTH_CENTER = 10;
    constexpr uint32 SOUTHEAST = 11;
}

// ============================================================================
// ZONE CENTER POSITIONS
// ============================================================================

// Main zone center coordinates where Azerite spawns
constexpr float ZONE_POSITIONS[][4] = {
    { -1863.0f, 2112.0f, 5.0f, 3.14f },    // Zone 0 - North Beach
    { -1938.0f, 2027.0f, 18.0f, 5.50f },   // Zone 1 - Northwest Hill
    { -1783.0f, 2148.0f, 3.0f, 0.78f },    // Zone 2 - Northeast Rocks
    { -1998.0f, 1942.0f, 22.0f, 4.71f },   // Zone 3 - West Cliff (elevated)
    { -1703.0f, 2083.0f, 6.0f, 1.57f },    // Zone 4 - East Shore
    { -1858.0f, 1987.0f, 10.0f, 3.14f },   // Zone 5 - Center North
    { -1923.0f, 1857.0f, 25.0f, 4.00f },   // Zone 6 - West South (elevated)
    { -1773.0f, 1918.0f, 8.0f, 2.36f },    // Zone 7 - Center (most contested)
    { -1643.0f, 1998.0f, 4.0f, 0.00f },    // Zone 8 - East Beach
    { -1888.0f, 1772.0f, 28.0f, 3.93f },   // Zone 9 - Southwest Hill (highest)
    { -1728.0f, 1833.0f, 12.0f, 2.36f },   // Zone 10 - South Center
    { -1588.0f, 1913.0f, 6.0f, 0.79f }     // Zone 11 - Southeast
};

inline Position GetZoneCenter(uint32 zoneId)
{
    if (zoneId < SpawnZones::ZONE_COUNT)
        return Position(ZONE_POSITIONS[zoneId][0], ZONE_POSITIONS[zoneId][1],
                       ZONE_POSITIONS[zoneId][2], ZONE_POSITIONS[zoneId][3]);
    return Position(0, 0, 0, 0);
}

inline const char* GetZoneName(uint32 zoneId)
{
    static const char* names[] = {
        "North Beach", "Northwest Hill", "Northeast Rocks", "West Cliff",
        "East Shore", "Center North", "West South", "Center",
        "East Beach", "Southwest Hill", "South Center", "Southeast"
    };
    return zoneId < SpawnZones::ZONE_COUNT ? names[zoneId] : "Unknown";
}

// ============================================================================
// ZONE CLUSTERING - WHICH ZONES ARE ADJACENT
// ============================================================================

// Each zone has a list of adjacent zone IDs for rapid rotation
constexpr uint32 MAX_ADJACENT_ZONES = 5;

constexpr uint32 ZONE_ADJACENCY[][MAX_ADJACENT_ZONES + 1] = {
    // { count, adjacent zones... }
    { 4, SpawnZones::NORTHWEST_HILL, SpawnZones::NORTHEAST_ROCKS, SpawnZones::CENTER_NORTH, SpawnZones::EAST_SHORE },          // 0 - North Beach
    { 4, SpawnZones::NORTH_BEACH, SpawnZones::WEST_CLIFF, SpawnZones::CENTER_NORTH, SpawnZones::WEST_SOUTH },                   // 1 - Northwest Hill
    { 3, SpawnZones::NORTH_BEACH, SpawnZones::EAST_SHORE, SpawnZones::CENTER_NORTH },                                          // 2 - Northeast Rocks
    { 4, SpawnZones::NORTHWEST_HILL, SpawnZones::CENTER_NORTH, SpawnZones::WEST_SOUTH, SpawnZones::SOUTHWEST_HILL },           // 3 - West Cliff
    { 4, SpawnZones::NORTH_BEACH, SpawnZones::NORTHEAST_ROCKS, SpawnZones::CENTER, SpawnZones::EAST_BEACH },                   // 4 - East Shore
    { 5, SpawnZones::NORTH_BEACH, SpawnZones::NORTHWEST_HILL, SpawnZones::WEST_CLIFF, SpawnZones::CENTER, SpawnZones::WEST_SOUTH }, // 5 - Center North
    { 5, SpawnZones::NORTHWEST_HILL, SpawnZones::WEST_CLIFF, SpawnZones::CENTER_NORTH, SpawnZones::CENTER, SpawnZones::SOUTHWEST_HILL }, // 6 - West South
    { 5, SpawnZones::CENTER_NORTH, SpawnZones::EAST_SHORE, SpawnZones::WEST_SOUTH, SpawnZones::SOUTH_CENTER, SpawnZones::EAST_BEACH }, // 7 - Center (hub)
    { 4, SpawnZones::EAST_SHORE, SpawnZones::CENTER, SpawnZones::SOUTH_CENTER, SpawnZones::SOUTHEAST },                        // 8 - East Beach
    { 4, SpawnZones::WEST_CLIFF, SpawnZones::WEST_SOUTH, SpawnZones::CENTER, SpawnZones::SOUTH_CENTER },                       // 9 - Southwest Hill
    { 5, SpawnZones::CENTER, SpawnZones::EAST_BEACH, SpawnZones::SOUTHWEST_HILL, SpawnZones::SOUTHEAST, SpawnZones::WEST_SOUTH }, // 10 - South Center
    { 3, SpawnZones::EAST_BEACH, SpawnZones::SOUTH_CENTER, SpawnZones::CENTER }                                                // 11 - Southeast
};

inline std::vector<uint32> GetAdjacentZones(uint32 zoneId)
{
    std::vector<uint32> adjacent;
    if (zoneId < SpawnZones::ZONE_COUNT)
    {
        uint32 count = ZONE_ADJACENCY[zoneId][0];
        for (uint32 i = 1; i <= count; ++i)
            adjacent.push_back(ZONE_ADJACENCY[zoneId][i]);
    }
    return adjacent;
}

// ============================================================================
// ZONE DEFENSE POSITIONS (8 per zone = 96 total)
// ============================================================================

// Defense positions around each zone (indexed by zone ID)
constexpr float ZONE_DEFENSE_POSITIONS[][8][4] = {
    // Zone 0 - North Beach (defensive positions along beach)
    {
        { -1853.0f, 2122.0f, 5.0f, 3.14f },   // North edge
        { -1873.0f, 2122.0f, 5.0f, 3.14f },   // North edge west
        { -1843.0f, 2107.0f, 5.0f, 4.71f },   // East flank
        { -1883.0f, 2107.0f, 5.0f, 1.57f },   // West flank
        { -1858.0f, 2097.0f, 6.0f, 3.93f },   // South cover
        { -1868.0f, 2097.0f, 6.0f, 2.36f },   // South cover west
        { -1848.0f, 2117.0f, 4.0f, 0.79f },   // Beach edge
        { -1878.0f, 2117.0f, 4.0f, 5.50f }    // Beach edge west
    },
    // Zone 1 - Northwest Hill (elevated defensive positions)
    {
        { -1928.0f, 2037.0f, 20.0f, 5.50f },  // Summit north
        { -1948.0f, 2037.0f, 18.0f, 5.50f },  // Summit north west
        { -1923.0f, 2017.0f, 19.0f, 4.71f },  // East slope
        { -1953.0f, 2017.0f, 17.0f, 1.57f },  // West slope
        { -1938.0f, 2007.0f, 16.0f, 3.14f },  // South edge
        { -1928.0f, 2022.0f, 19.0f, 0.00f },  // Central high ground
        { -1948.0f, 2022.0f, 18.0f, 3.14f },  // Central west
        { -1938.0f, 2042.0f, 17.0f, 5.50f }   // Far north overlook
    },
    // Zone 2 - Northeast Rocks (rocky terrain positions)
    {
        { -1773.0f, 2158.0f, 4.0f, 0.78f },   // North rock
        { -1793.0f, 2158.0f, 3.0f, 0.78f },   // North rock west
        { -1768.0f, 2143.0f, 4.0f, 1.57f },   // East outcrop
        { -1798.0f, 2143.0f, 3.0f, 4.71f },   // West outcrop
        { -1778.0f, 2133.0f, 4.0f, 3.14f },   // South cover
        { -1788.0f, 2133.0f, 3.0f, 2.36f },   // South cover west
        { -1763.0f, 2153.0f, 5.0f, 0.00f },   // Far east rock
        { -1803.0f, 2153.0f, 3.0f, 3.14f }    // Far west rock
    },
    // Zone 3 - West Cliff (highest elevation defensive positions)
    {
        { -1988.0f, 1952.0f, 24.0f, 4.71f },  // Cliff edge north
        { -2008.0f, 1952.0f, 22.0f, 4.71f },  // Cliff edge north west
        { -1983.0f, 1937.0f, 23.0f, 5.50f },  // East cliff
        { -2013.0f, 1937.0f, 21.0f, 1.57f },  // West cliff
        { -1998.0f, 1927.0f, 22.0f, 3.14f },  // South cliff
        { -1993.0f, 1947.0f, 23.0f, 0.00f },  // Central plateau
        { -2003.0f, 1947.0f, 22.0f, 3.14f },  // Central west
        { -1998.0f, 1957.0f, 23.0f, 4.71f }   // Northern overlook
    },
    // Zone 4 - East Shore (beach/water defensive positions)
    {
        { -1693.0f, 2093.0f, 7.0f, 1.57f },   // Shore north
        { -1713.0f, 2093.0f, 6.0f, 1.57f },   // Shore north west
        { -1688.0f, 2078.0f, 7.0f, 0.79f },   // East waterline
        { -1718.0f, 2078.0f, 6.0f, 4.71f },   // West waterline
        { -1698.0f, 2068.0f, 6.0f, 3.14f },   // South beach
        { -1708.0f, 2068.0f, 6.0f, 2.36f },   // South beach west
        { -1683.0f, 2088.0f, 7.0f, 0.00f },   // Far east shore
        { -1723.0f, 2088.0f, 6.0f, 3.14f }    // Far west shore
    },
    // Zone 5 - Center North (transitional defensive positions)
    {
        { -1848.0f, 1997.0f, 11.0f, 3.14f },  // North edge
        { -1868.0f, 1997.0f, 10.0f, 3.14f },  // North edge west
        { -1843.0f, 1982.0f, 11.0f, 4.71f },  // East side
        { -1873.0f, 1982.0f, 10.0f, 1.57f },  // West side
        { -1858.0f, 1972.0f, 10.0f, 3.14f },  // South cover
        { -1853.0f, 1987.0f, 11.0f, 0.00f },  // Central east
        { -1863.0f, 1987.0f, 10.0f, 3.14f },  // Central west
        { -1858.0f, 2002.0f, 10.0f, 5.50f }   // Far north
    },
    // Zone 6 - West South (elevated defensive positions)
    {
        { -1913.0f, 1867.0f, 26.0f, 4.00f },  // Hilltop north
        { -1933.0f, 1867.0f, 24.0f, 4.00f },  // Hilltop north west
        { -1908.0f, 1852.0f, 26.0f, 5.50f },  // East slope
        { -1938.0f, 1852.0f, 24.0f, 1.57f },  // West slope
        { -1923.0f, 1842.0f, 24.0f, 3.14f },  // South edge
        { -1918.0f, 1857.0f, 26.0f, 0.00f },  // Central high
        { -1928.0f, 1857.0f, 25.0f, 3.14f },  // Central west
        { -1923.0f, 1872.0f, 25.0f, 4.71f }   // Northern ridge
    },
    // Zone 7 - Center (most contested - comprehensive defense)
    {
        { -1763.0f, 1928.0f, 9.0f, 2.36f },   // North edge
        { -1783.0f, 1928.0f, 8.0f, 2.36f },   // North edge west
        { -1758.0f, 1913.0f, 9.0f, 1.57f },   // East flank
        { -1788.0f, 1913.0f, 8.0f, 4.71f },   // West flank
        { -1768.0f, 1903.0f, 9.0f, 3.14f },   // South edge
        { -1778.0f, 1903.0f, 8.0f, 3.93f },   // South edge west
        { -1768.0f, 1923.0f, 9.0f, 0.79f },   // NE corner
        { -1778.0f, 1923.0f, 8.0f, 5.50f }    // NW corner
    },
    // Zone 8 - East Beach (coastal defensive positions)
    {
        { -1633.0f, 2008.0f, 5.0f, 0.00f },   // Beach north
        { -1653.0f, 2008.0f, 4.0f, 0.00f },   // Beach north west
        { -1628.0f, 1993.0f, 5.0f, 0.79f },   // East tide line
        { -1658.0f, 1993.0f, 4.0f, 4.71f },   // West edge
        { -1638.0f, 1983.0f, 5.0f, 3.14f },   // South beach
        { -1648.0f, 1983.0f, 4.0f, 2.36f },   // South beach west
        { -1623.0f, 2003.0f, 5.0f, 0.00f },   // Far east water
        { -1663.0f, 2003.0f, 4.0f, 3.14f }    // Far west edge
    },
    // Zone 9 - Southwest Hill (highest point - sniper positions)
    {
        { -1878.0f, 1782.0f, 30.0f, 3.93f },  // Summit north
        { -1898.0f, 1782.0f, 28.0f, 3.93f },  // Summit north west
        { -1873.0f, 1767.0f, 29.0f, 5.50f },  // East ridge
        { -1903.0f, 1767.0f, 27.0f, 1.57f },  // West ridge
        { -1888.0f, 1757.0f, 27.0f, 3.14f },  // South overlook
        { -1883.0f, 1772.0f, 29.0f, 0.00f },  // Central high
        { -1893.0f, 1772.0f, 28.0f, 3.14f },  // Central west
        { -1888.0f, 1787.0f, 29.0f, 4.71f }   // Northern peak
    },
    // Zone 10 - South Center (transitional positions)
    {
        { -1718.0f, 1843.0f, 13.0f, 2.36f },  // North edge
        { -1738.0f, 1843.0f, 12.0f, 2.36f },  // North edge west
        { -1713.0f, 1828.0f, 13.0f, 1.57f },  // East side
        { -1743.0f, 1828.0f, 12.0f, 4.71f },  // West side
        { -1723.0f, 1818.0f, 12.0f, 3.14f },  // South edge
        { -1733.0f, 1818.0f, 12.0f, 3.93f },  // South edge west
        { -1718.0f, 1838.0f, 13.0f, 0.79f },  // NE position
        { -1738.0f, 1838.0f, 12.0f, 5.50f }   // NW position
    },
    // Zone 11 - Southeast (coastal transition)
    {
        { -1578.0f, 1923.0f, 7.0f, 0.79f },   // North beach
        { -1598.0f, 1923.0f, 6.0f, 0.79f },   // North beach west
        { -1573.0f, 1908.0f, 7.0f, 0.00f },   // East shore
        { -1603.0f, 1908.0f, 6.0f, 3.14f },   // West edge
        { -1583.0f, 1898.0f, 7.0f, 3.93f },   // South edge
        { -1593.0f, 1898.0f, 6.0f, 2.36f },   // South edge west
        { -1568.0f, 1918.0f, 8.0f, 0.00f },   // Far east
        { -1608.0f, 1918.0f, 6.0f, 3.14f }    // Far west
    }
};

inline std::vector<Position> GetZoneDefensePositions(uint32 zoneId)
{
    std::vector<Position> positions;
    if (zoneId < SpawnZones::ZONE_COUNT)
    {
        for (uint8 i = 0; i < 8; ++i)
        {
            positions.emplace_back(
                ZONE_DEFENSE_POSITIONS[zoneId][i][0],
                ZONE_DEFENSE_POSITIONS[zoneId][i][1],
                ZONE_DEFENSE_POSITIONS[zoneId][i][2],
                ZONE_DEFENSE_POSITIONS[zoneId][i][3]
            );
        }
    }
    return positions;
}

// ============================================================================
// CHOKEPOINTS (15 inter-zone transition points)
// ============================================================================

namespace Chokepoints
{
    constexpr uint32 COUNT = 15;

    // Chokepoint indices
    constexpr uint32 NORTH_RIDGE = 0;
    constexpr uint32 NORTHWEST_PATH = 1;
    constexpr uint32 NORTHEAST_PASSAGE = 2;
    constexpr uint32 WEST_DESCENT = 3;
    constexpr uint32 EAST_CLIMB = 4;
    constexpr uint32 CENTER_NORTH_BRIDGE = 5;
    constexpr uint32 CENTER_SOUTH_BRIDGE = 6;
    constexpr uint32 WEST_SOUTH_PASS = 7;
    constexpr uint32 EAST_SOUTH_PASS = 8;
    constexpr uint32 SOUTHWEST_RIDGE = 9;
    constexpr uint32 SOUTHEAST_BEACH = 10;
    constexpr uint32 CENTRAL_CROSSROADS = 11;
    constexpr uint32 HORDE_APPROACH = 12;
    constexpr uint32 ALLIANCE_APPROACH = 13;
    constexpr uint32 MID_FIELD = 14;
}

constexpr float CHOKEPOINT_POSITIONS[][4] = {
    { -1863.0f, 2070.0f, 8.0f, 3.14f },    // 0 - North Ridge (between North Beach and Center North)
    { -1968.0f, 2000.0f, 15.0f, 5.00f },   // 1 - Northwest Path (NW Hill to West Cliff)
    { -1743.0f, 2113.0f, 5.0f, 1.00f },    // 2 - Northeast Passage (NE Rocks to East Shore)
    { -1983.0f, 1890.0f, 20.0f, 4.50f },   // 3 - West Descent (West Cliff to West South)
    { -1658.0f, 2050.0f, 6.0f, 1.20f },    // 4 - East Climb (East Shore to East Beach)
    { -1813.0f, 1950.0f, 9.0f, 2.80f },    // 5 - Center North Bridge (Center North to Center)
    { -1753.0f, 1873.0f, 10.0f, 2.50f },   // 6 - Center South Bridge (Center to South Center)
    { -1903.0f, 1810.0f, 22.0f, 3.80f },   // 7 - West South Pass (West South to SW Hill)
    { -1663.0f, 1958.0f, 6.0f, 1.00f },    // 8 - East South Pass (East Beach to Southeast)
    { -1858.0f, 1810.0f, 18.0f, 3.50f },   // 9 - Southwest Ridge (SW Hill to South Center)
    { -1618.0f, 1948.0f, 5.0f, 0.50f },    // 10 - Southeast Beach (Southeast to East Beach)
    { -1813.0f, 1893.0f, 9.0f, 2.70f },    // 11 - Central Crossroads (major intersection)
    { -1983.0f, 2100.0f, 10.0f, 4.20f },   // 12 - Horde Approach (Horde spawn route)
    { -1628.0f, 1838.0f, 5.0f, 0.80f },    // 13 - Alliance Approach (Alliance spawn route)
    { -1813.0f, 1950.0f, 9.0f, 2.36f }     // 14 - Mid Field (center of map)
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
        "North Ridge", "Northwest Path", "Northeast Passage", "West Descent",
        "East Climb", "Center North Bridge", "Center South Bridge", "West South Pass",
        "East South Pass", "Southwest Ridge", "Southeast Beach", "Central Crossroads",
        "Horde Approach", "Alliance Approach", "Mid Field"
    };
    return chokepointId < Chokepoints::COUNT ? names[chokepointId] : "Unknown";
}

// ============================================================================
// SNIPER POSITIONS (8 elevated advantage points)
// ============================================================================

namespace SniperSpots
{
    constexpr uint32 COUNT = 8;

    constexpr uint32 WEST_CLIFF_OVERLOOK = 0;
    constexpr uint32 SOUTHWEST_PEAK = 1;
    constexpr uint32 NORTHWEST_SUMMIT = 2;
    constexpr uint32 WEST_SOUTH_RIDGE = 3;
    constexpr uint32 SOUTH_CENTER_HIGH = 4;
    constexpr uint32 EAST_SHORE_ROCKS = 5;
    constexpr uint32 CENTER_ELEVATION = 6;
    constexpr uint32 NORTHEAST_OUTCROP = 7;
}

constexpr float SNIPER_POSITIONS[][4] = {
    { -2008.0f, 1942.0f, 25.0f, 0.79f },   // 0 - West Cliff Overlook (highest western point)
    { -1893.0f, 1762.0f, 32.0f, 5.50f },   // 1 - Southwest Peak (highest point on map)
    { -1943.0f, 2042.0f, 22.0f, 5.50f },   // 2 - Northwest Summit (overlooks north beach)
    { -1933.0f, 1847.0f, 28.0f, 0.00f },   // 3 - West South Ridge (central western control)
    { -1738.0f, 1823.0f, 16.0f, 5.50f },   // 4 - South Center High (southern elevated point)
    { -1678.0f, 2093.0f, 10.0f, 4.71f },   // 5 - East Shore Rocks (eastern elevation)
    { -1773.0f, 1908.0f, 12.0f, 3.14f },   // 6 - Center Elevation (central high ground)
    { -1768.0f, 2158.0f, 8.0f, 3.93f }     // 7 - Northeast Outcrop (northeastern rocks)
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
        "West Cliff Overlook", "Southwest Peak", "Northwest Summit", "West South Ridge",
        "South Center High", "East Shore Rocks", "Center Elevation", "Northeast Outcrop"
    };
    return sniperId < SniperSpots::COUNT ? names[sniperId] : "Unknown";
}

// ============================================================================
// BUFF/RESTORATION POSITIONS (5 locations)
// ============================================================================

namespace BuffSpots
{
    constexpr uint32 COUNT = 5;
}

constexpr float BUFF_POSITIONS[][4] = {
    { -1858.0f, 2050.0f, 7.0f, 3.14f },    // North area buff
    { -1963.0f, 1942.0f, 18.0f, 4.71f },   // West area buff
    { -1678.0f, 2043.0f, 6.0f, 1.57f },    // East area buff
    { -1813.0f, 1873.0f, 10.0f, 2.36f },   // Central buff
    { -1718.0f, 1793.0f, 8.0f, 3.14f }     // South area buff
};

inline Position GetBuffPosition(uint32 buffId)
{
    if (buffId < BuffSpots::COUNT)
        return Position(BUFF_POSITIONS[buffId][0], BUFF_POSITIONS[buffId][1],
                       BUFF_POSITIONS[buffId][2], BUFF_POSITIONS[buffId][3]);
    return Position(0, 0, 0, 0);
}

// ============================================================================
// SPAWN POSITIONS
// ============================================================================

constexpr float ALLIANCE_SPAWN_X = -1573.0f;
constexpr float ALLIANCE_SPAWN_Y = 1758.0f;
constexpr float ALLIANCE_SPAWN_Z = 4.0f;
constexpr float ALLIANCE_SPAWN_O = 1.57f;

constexpr float HORDE_SPAWN_X = -2053.0f;
constexpr float HORDE_SPAWN_Y = 2172.0f;
constexpr float HORDE_SPAWN_Z = 8.0f;
constexpr float HORDE_SPAWN_O = 4.71f;

inline Position GetSpawnPosition(uint32 faction)
{
    if (faction == 1)  // ALLIANCE
        return Position(ALLIANCE_SPAWN_X, ALLIANCE_SPAWN_Y, ALLIANCE_SPAWN_Z, ALLIANCE_SPAWN_O);
    else  // HORDE
        return Position(HORDE_SPAWN_X, HORDE_SPAWN_Y, HORDE_SPAWN_Z, HORDE_SPAWN_O);
}

// ============================================================================
// DISTANCE MATRIX (12x12 pre-calculated zone distances)
// ============================================================================

constexpr float ZONE_DISTANCE_MATRIX[12][12] = {
    //  NB     NWH    NER    WC     ES     CN     WS     C      EB     SWH    SC     SE
    {   0.0f, 105.0f, 90.0f, 215.0f, 165.0f, 125.0f, 270.0f, 210.0f, 240.0f, 350.0f, 305.0f, 325.0f },  // North Beach
    { 105.0f,   0.0f, 175.0f, 105.0f, 260.0f, 90.0f, 180.0f, 200.0f, 340.0f, 270.0f, 285.0f, 395.0f },  // Northwest Hill
    {  90.0f, 175.0f,   0.0f, 285.0f, 85.0f, 175.0f, 315.0f, 235.0f, 180.0f, 405.0f, 330.0f, 260.0f },  // Northeast Rocks
    { 215.0f, 105.0f, 285.0f,   0.0f, 345.0f, 155.0f, 115.0f, 250.0f, 420.0f, 185.0f, 295.0f, 475.0f },  // West Cliff
    { 165.0f, 260.0f, 85.0f, 345.0f,   0.0f, 185.0f, 315.0f, 175.0f, 95.0f, 400.0f, 270.0f, 170.0f },  // East Shore
    { 125.0f, 90.0f, 175.0f, 155.0f, 185.0f,   0.0f, 145.0f, 110.0f, 250.0f, 235.0f, 190.0f, 310.0f },  // Center North
    { 270.0f, 180.0f, 315.0f, 115.0f, 315.0f, 145.0f,   0.0f, 160.0f, 340.0f, 95.0f, 170.0f, 385.0f },  // West South
    { 210.0f, 200.0f, 235.0f, 250.0f, 175.0f, 110.0f, 160.0f,   0.0f, 160.0f, 200.0f, 95.0f, 215.0f },  // Center
    { 240.0f, 340.0f, 180.0f, 420.0f, 95.0f, 250.0f, 340.0f, 160.0f,   0.0f, 380.0f, 175.0f, 85.0f },  // East Beach
    { 350.0f, 270.0f, 405.0f, 185.0f, 400.0f, 235.0f, 95.0f, 200.0f, 380.0f,   0.0f, 135.0f, 425.0f },  // Southwest Hill
    { 305.0f, 285.0f, 330.0f, 295.0f, 270.0f, 190.0f, 170.0f, 95.0f, 175.0f, 135.0f,   0.0f, 195.0f },  // South Center
    { 325.0f, 395.0f, 260.0f, 475.0f, 170.0f, 310.0f, 385.0f, 215.0f, 85.0f, 425.0f, 195.0f,   0.0f }   // Southeast
};

inline float GetZoneDistance(uint32 zoneA, uint32 zoneB)
{
    if (zoneA < SpawnZones::ZONE_COUNT && zoneB < SpawnZones::ZONE_COUNT)
        return ZONE_DISTANCE_MATRIX[zoneA][zoneB];
    return 9999.0f;
}

// ============================================================================
// ZONE PRIORITY BY FACTION (distance from spawn = priority)
// ============================================================================

// Alliance priority (closer zones = lower index = higher priority)
constexpr uint32 ALLIANCE_ZONE_PRIORITY[] = {
    SpawnZones::SOUTHEAST,       // 0 - Closest to Alliance spawn
    SpawnZones::EAST_BEACH,      // 1
    SpawnZones::SOUTH_CENTER,    // 2
    SpawnZones::CENTER,          // 3
    SpawnZones::EAST_SHORE,      // 4
    SpawnZones::NORTHEAST_ROCKS, // 5
    SpawnZones::CENTER_NORTH,    // 6
    SpawnZones::NORTH_BEACH,     // 7
    SpawnZones::WEST_SOUTH,      // 8
    SpawnZones::SOUTHWEST_HILL,  // 9
    SpawnZones::NORTHWEST_HILL,  // 10
    SpawnZones::WEST_CLIFF       // 11 - Furthest from Alliance
};

// Horde priority (closer zones = lower index = higher priority)
constexpr uint32 HORDE_ZONE_PRIORITY[] = {
    SpawnZones::NORTHWEST_HILL,  // 0 - Closest to Horde spawn
    SpawnZones::NORTH_BEACH,     // 1
    SpawnZones::WEST_CLIFF,      // 2
    SpawnZones::NORTHEAST_ROCKS, // 3
    SpawnZones::CENTER_NORTH,    // 4
    SpawnZones::WEST_SOUTH,      // 5
    SpawnZones::CENTER,          // 6
    SpawnZones::EAST_SHORE,      // 7
    SpawnZones::SOUTHWEST_HILL,  // 8
    SpawnZones::SOUTH_CENTER,    // 9
    SpawnZones::EAST_BEACH,      // 10
    SpawnZones::SOUTHEAST        // 11 - Furthest from Horde
};

inline uint32 GetZonePriorityRank(uint32 zoneId, uint32 faction)
{
    const uint32* priority = (faction == 1) ? ALLIANCE_ZONE_PRIORITY : HORDE_ZONE_PRIORITY;
    for (uint32 i = 0; i < SpawnZones::ZONE_COUNT; ++i)
    {
        if (priority[i] == zoneId)
            return i;
    }
    return SpawnZones::ZONE_COUNT;
}

// ============================================================================
// AMBUSH POSITIONS (faction-specific interception)
// ============================================================================

namespace AmbushSpots
{
    constexpr uint32 ALLIANCE_COUNT = 6;
    constexpr uint32 HORDE_COUNT = 6;
}

// Alliance ambush positions (intercept Horde moving south)
constexpr float ALLIANCE_AMBUSH_POSITIONS[][4] = {
    { -1813.0f, 1923.0f, 9.0f, 5.50f },    // Central intercept
    { -1723.0f, 1868.0f, 11.0f, 5.50f },   // South approach
    { -1653.0f, 1968.0f, 5.0f, 4.71f },    // East flank
    { -1793.0f, 1993.0f, 9.0f, 5.50f },    // North-central
    { -1693.0f, 1893.0f, 8.0f, 4.71f },    // Southeast passage
    { -1618.0f, 1858.0f, 6.0f, 5.00f }     // Deep alliance territory
};

// Horde ambush positions (intercept Alliance moving north)
constexpr float HORDE_AMBUSH_POSITIONS[][4] = {
    { -1873.0f, 2007.0f, 9.0f, 2.36f },    // Central intercept
    { -1918.0f, 1927.0f, 15.0f, 2.00f },   // West approach
    { -1943.0f, 2047.0f, 18.0f, 2.36f },   // Northwest flank
    { -1833.0f, 2057.0f, 7.0f, 2.50f },    // North-central
    { -1893.0f, 1882.0f, 20.0f, 2.36f },   // Southwest passage
    { -1978.0f, 2090.0f, 12.0f, 2.00f }    // Deep horde territory
};

inline std::vector<Position> GetAmbushPositions(uint32 faction)
{
    std::vector<Position> positions;
    if (faction == 1)  // ALLIANCE ambushes
    {
        for (uint32 i = 0; i < AmbushSpots::ALLIANCE_COUNT; ++i)
        {
            positions.emplace_back(
                ALLIANCE_AMBUSH_POSITIONS[i][0], ALLIANCE_AMBUSH_POSITIONS[i][1],
                ALLIANCE_AMBUSH_POSITIONS[i][2], ALLIANCE_AMBUSH_POSITIONS[i][3]
            );
        }
    }
    else  // HORDE ambushes
    {
        for (uint32 i = 0; i < AmbushSpots::HORDE_COUNT; ++i)
        {
            positions.emplace_back(
                HORDE_AMBUSH_POSITIONS[i][0], HORDE_AMBUSH_POSITIONS[i][1],
                HORDE_AMBUSH_POSITIONS[i][2], HORDE_AMBUSH_POSITIONS[i][3]
            );
        }
    }
    return positions;
}

// ============================================================================
// ZONE-TO-ZONE ROUTES (multi-waypoint paths)
// ============================================================================

// Route waypoint structure
struct RouteWaypoint
{
    float x, y, z;
};

// Maximum waypoints per route
constexpr uint32 MAX_ROUTE_WAYPOINTS = 6;

// Key routes between strategic zones
// Route format: { waypointCount, {x,y,z}, {x,y,z}, ... }
// Center (7) to Southwest Hill (9) - important diagonal route
constexpr float ROUTE_CENTER_TO_SWHILL[][3] = {
    { -1773.0f, 1918.0f, 8.0f },    // Start: Center
    { -1803.0f, 1888.0f, 10.0f },   // Waypoint 1
    { -1843.0f, 1848.0f, 14.0f },   // Waypoint 2
    { -1873.0f, 1808.0f, 22.0f },   // Waypoint 3
    { -1888.0f, 1772.0f, 28.0f }    // End: SW Hill
};

// Alliance spawn to Center - main Alliance push route
constexpr float ROUTE_ALLY_SPAWN_TO_CENTER[][3] = {
    { -1573.0f, 1758.0f, 4.0f },    // Start: Alliance spawn
    { -1613.0f, 1798.0f, 6.0f },    // Waypoint 1
    { -1663.0f, 1848.0f, 8.0f },    // Waypoint 2
    { -1713.0f, 1883.0f, 9.0f },    // Waypoint 3
    { -1773.0f, 1918.0f, 8.0f }     // End: Center
};

// Horde spawn to Center - main Horde push route
constexpr float ROUTE_HORDE_SPAWN_TO_CENTER[][3] = {
    { -2053.0f, 2172.0f, 8.0f },    // Start: Horde spawn
    { -1988.0f, 2112.0f, 10.0f },   // Waypoint 1
    { -1923.0f, 2052.0f, 12.0f },   // Waypoint 2
    { -1863.0f, 1987.0f, 10.0f },   // Waypoint 3
    { -1813.0f, 1950.0f, 9.0f },    // Waypoint 4
    { -1773.0f, 1918.0f, 8.0f }     // End: Center
};

// ============================================================================
// WORLD STATES
// ============================================================================

namespace WorldStates
{
    constexpr int32 AZERITE_ALLY = 13231;
    constexpr int32 AZERITE_HORDE = 13232;
    constexpr int32 MATCH_TIME = 13229;
    constexpr int32 NODES_ACTIVE = 13230;
}

// ============================================================================
// GAME OBJECTS
// ============================================================================

namespace GameObjects
{
    constexpr uint32 AZERITE_NODE_BASE = 281102;  // Base entry for azerite nodes
    constexpr uint32 ALLIANCE_BANNER = 281110;
    constexpr uint32 HORDE_BANNER = 281111;
    constexpr uint32 NEUTRAL_BANNER = 281112;
}

// ============================================================================
// STRATEGY CONSTANTS
// ============================================================================

namespace Strategy
{
    // Team composition per zone
    constexpr uint8 MIN_CAPTURE_TEAM = 3;          // Minimum players to capture a node
    constexpr uint8 OPTIMAL_CAPTURE_TEAM = 5;      // Optimal capture team size
    constexpr uint8 MAX_DEFENDERS_PER_ZONE = 4;    // Maximum defenders after capture

    // Timing
    constexpr uint32 RESPONSE_TIME = 12000;        // Expected time to reach a new node (ms)
    constexpr uint32 ROTATION_INTERVAL = 8000;     // How often to reassess positions (ms)
    constexpr uint32 NODE_SPAWN_ANTICIPATION = 3000; // Pre-position time before node spawns

    // Split decision thresholds
    constexpr float SPLIT_DISTANCE_THRESHOLD = 200.0f;  // Distance to justify splitting team
    constexpr uint8 MIN_SPLIT_TEAM_SIZE = 3;            // Minimum players per split group

    // Score thresholds
    constexpr uint32 LEADING_THRESHOLD = 300;      // Score lead to play defensive
    constexpr uint32 DESPERATE_THRESHOLD = 400;    // Score deficit for ALL_IN

    // Zone value modifiers
    constexpr float CONTESTED_ZONE_VALUE = 1.5f;   // Multiplier for actively contested zones
    constexpr float UNCONTESTED_ZONE_VALUE = 0.8f; // Multiplier for uncontested zones

    // Time phases (ms)
    constexpr uint32 OPENING_PHASE = 60000;        // First 60 seconds
    constexpr uint32 MID_GAME_END = 480000;        // 8 minutes
    constexpr uint32 LATE_GAME_START = 480001;     // After 8 minutes
}

// ============================================================================
// DYNAMIC NODE TRACKING
// ============================================================================

struct AzeriteNode
{
    uint32 id;
    uint32 zoneId;
    Position position;
    bool active;
    uint32 spawnTime;
    uint32 capturedByFaction;  // 0 = uncaptured, 1 = Alliance, 2 = Horde
    bool contested;
    float captureProgress;     // 0.0 to 1.0
};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

inline float CalculateDistance(float x1, float y1, float x2, float y2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    return std::sqrt(dx * dx + dy * dy);
}

inline uint32 GetNearestZone(float x, float y)
{
    uint32 nearestZone = 0;
    float minDist = 99999.0f;

    for (uint32 i = 0; i < SpawnZones::ZONE_COUNT; ++i)
    {
        float dist = CalculateDistance(x, y, ZONE_POSITIONS[i][0], ZONE_POSITIONS[i][1]);
        if (dist < minDist)
        {
            minDist = dist;
            nearestZone = i;
        }
    }
    return nearestZone;
}

inline bool IsElevatedZone(uint32 zoneId)
{
    // Zones with significant elevation advantage
    return zoneId == SpawnZones::WEST_CLIFF ||
           zoneId == SpawnZones::SOUTHWEST_HILL ||
           zoneId == SpawnZones::NORTHWEST_HILL ||
           zoneId == SpawnZones::WEST_SOUTH;
}

inline float GetZoneElevation(uint32 zoneId)
{
    if (zoneId < SpawnZones::ZONE_COUNT)
        return ZONE_POSITIONS[zoneId][2];
    return 0.0f;
}

} // namespace Playerbot::Coordination::Battleground::SeethingShore

#endif
