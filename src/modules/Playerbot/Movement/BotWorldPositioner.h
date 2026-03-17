/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "Define.h"
#include "SharedDefines.h"
#include "Position.h"
#include <atomic>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <optional>

class Player;
struct FactionTemplateEntry;

namespace Playerbot
{

// ============================================================================
// SPAWN POINT TYPES - Priority order for zone spawn point selection
// ============================================================================
enum class SpawnPointType : uint8
{
    INNKEEPER       = 0,    // Highest priority - safe hearthstone bind location
    FLIGHT_MASTER   = 1,    // Travel hub, always safe
    QUEST_GIVER     = 2,    // Quest hub cluster centroid
    GRAVEYARD       = 3,    // Resurrection point fallback
    CAPITAL_CITY    = 4,    // Capital city fallback
    HARDCODED       = 5     // Manually defined fallback
};

// ============================================================================
// DATABASE SPAWN POINT - Raw data from database queries
// ============================================================================
struct DbSpawnPoint
{
    uint32 creatureEntry;
    uint32 mapId;
    uint32 zoneId;
    uint32 areaId;
    float x;
    float y;
    float z;
    float orientation;
    uint16 factionTemplateId;
    SpawnPointType type;
    ::std::string npcName;

    bool IsValid() const { return mapId != 0 || (x != 0.0f && y != 0.0f); }
};

// ============================================================================
// ZONE LEVEL INFO - Quest-based level range data for a zone
// ============================================================================
struct ZoneLevelInfo
{
    uint32 zoneId;
    uint32 minLevel;
    uint32 maxLevel;
    uint32 questCount;
    float avgLevel;

    ZoneLevelInfo() : zoneId(0), minLevel(80), maxLevel(1), questCount(0), avgLevel(0.0f) {}

    void AddQuest(uint32 questMinLevel, uint32 questMaxLevel)
    {
        if (questMinLevel > 0 && questMinLevel < minLevel)
            minLevel = questMinLevel;
        if (questMaxLevel > maxLevel)
            maxLevel = questMaxLevel;
        avgLevel = (avgLevel * questCount + (questMinLevel + questMaxLevel) / 2.0f) / (questCount + 1);
        ++questCount;
    }

    bool IsValid() const { return questCount > 0 && minLevel <= maxLevel; }
};

// ============================================================================
// QUEST HUB - Clustered group of quest givers in a location
// ============================================================================
struct QuestHub
{
    uint32 mapId;
    uint32 zoneId;
    float centroidX;
    float centroidY;
    float centroidZ;
    uint32 questGiverCount;
    TeamId faction;

    QuestHub() : mapId(0), zoneId(0), centroidX(0), centroidY(0), centroidZ(0),
                 questGiverCount(0), faction(TEAM_NEUTRAL) {}

    void AddQuestGiver(float x, float y, float z)
    {
        // Running average for centroid
        centroidX = (centroidX * questGiverCount + x) / (questGiverCount + 1);
        centroidY = (centroidY * questGiverCount + y) / (questGiverCount + 1);
        centroidZ = (centroidZ * questGiverCount + z) / (questGiverCount + 1);
        ++questGiverCount;
    }

    float DistanceTo(float x, float y) const
    {
        float dx = centroidX - x;
        float dy = centroidY - y;
        return ::std::sqrt(dx * dx + dy * dy);
    }
};

// ============================================================================
// CONFIG ZONE OVERRIDE - User-defined zone configuration
// ============================================================================
struct ConfigZoneOverride
{
    uint32 zoneId;
    bool enabled;                   // false = disabled zone
    ::std::optional<float> x;       // Override coordinates
    ::std::optional<float> y;
    ::std::optional<float> z;
    ::std::optional<uint32> minLevel;
    ::std::optional<uint32> maxLevel;
    ::std::optional<TeamId> faction;

    ConfigZoneOverride() : zoneId(0), enabled(true) {}
};

/**
 * Zone Placement Data
 * Represents a spawn location for bots at specific level ranges
 */
struct ZonePlacement
{
    uint32 zoneId;          // Zone ID from AreaTable.dbc
    uint32 mapId;           // Map ID for teleportation
    float x;                // World coordinates
    float y;
    float z;
    float orientation;
    uint32 minLevel;        // Minimum level for this zone
    uint32 maxLevel;        // Maximum level for this zone
    TeamId faction;         // TEAM_ALLIANCE, TEAM_HORDE, or TEAM_NEUTRAL
    ::std::string zoneName;   // Human-readable name
    bool isStarterZone;     // True for level 1-4 zones

    bool IsValidForLevel(uint32 level) const
    {
        return level >= minLevel && level <= maxLevel;
    }

    bool IsValidForFaction(TeamId factionTeam) const
    {
        return faction == TEAM_NEUTRAL || faction == factionTeam;
    }

    Position GetPosition() const
    {
        return Position(x, y, z, orientation);
    }
};

/**
 * Positioner Statistics
 * Tracks zone placement statistics
 */
struct PositionerStats
{
    uint32 totalPlacements{0};
    uint32 successfulPlacements{0};
    uint32 failedPlacements{0};
    uint32 zonesCached{0};
    uint32 starterZonePlacements{0};
    uint32 capitalCityFallbacks{0};
    // Zone category tracking
    uint32 totalZones{0};
    uint32 starterZones{0};
    uint32 levelingZones{0};
    uint32 endgameZones{0};
    uint32 capitalCities{0};
    // Teleport tracking
    uint32 teleportsFailed{0};
    uint32 botsPlaced{0};
    std::unordered_map<uint32, uint32> placementsPerZone;
};

/**
 * Zone Selection Result
 * Returned when selecting a zone for bot placement
 */
struct ZoneChoice
{
    ZonePlacement const* placement;
    float suitability;  // 0.0-1.0, how suitable the zone is

    ZoneChoice() : placement(nullptr), suitability(0.0f) {}
    explicit ZoneChoice(ZonePlacement const* p, float suit = 1.0f)
        : placement(p), suitability(suit) {}

    bool IsValid() const { return placement != nullptr; }
};

/**
 * Bot World Positioner - Automated Zone Placement for Bots
 *
 * Purpose: Place bots in level-appropriate zones during world population
 *
 * Features:
 * - Starter zone teleportation (L1-4 bots)
 * - Level-appropriate zone selection (L5+ bots)
 * - Faction-specific placement
 * - Race-specific starter zones
 * - Safe coordinates (verified spawn points)
 * - Immutable zone cache (lock-free reads)
 *
 * Integration:
 * - Uses TrinityCore's AreaTable.dbc for zone data
 * - Uses Player::TeleportTo() for positioning (main thread only)
 * - Compatible with ThreadPool worker threads (cache queries)
 *
 * Thread Safety:
 * - Immutable zone cache after LoadZones()
 * - Lock-free concurrent reads
 * - Atomic initialization flag
 *
 * Performance:
 * - Zone cache build: <1 second
 * - Zone selection: <0.05ms per bot
 * - Teleportation: <1ms per bot (TrinityCore API)
 *
 * Usage Workflow (Two-Phase Bot Creation):
 * 1. Worker Thread: SelectZone() - Choose zone based on level/faction
 * 2. Main Thread: TeleportToZone() - TrinityCore Player API
 *
 * Zone Categories:
 * - Starter Zones (L1-10): Race-specific starting areas
 * - Leveling Zones (L10-60): Open world questing zones
 * - Endgame Zones (L60-80): The War Within content zones
 * - Capital Cities (All levels): Faction capitals
 */
class TC_GAME_API BotWorldPositioner final 
{
public:
    static BotWorldPositioner* instance();

    // ====================================================================
    // INITIALIZATION (Called once at server startup)
    // ====================================================================

    /**
     * Load zone placements from configuration/database
     * MUST be called before any zone operations
     * Single-threaded execution required
     */
    bool LoadZones();

    /**
     * Reload zones (for hot-reload during development)
     */
    void ReloadZones();

    /**
     * Check if zones are ready
     */
    bool IsReady() const
    {
        return _initialized.load(::std::memory_order_acquire);
    }

    // ====================================================================
    // ZONE SELECTION (Thread-safe, for worker threads)
    // ====================================================================

    /**
     * Select zone for bot based on level and faction
     * Thread-safe, can be called from worker threads
     *
     * Selection Logic:
     * - L1-4: Starter zones (race-specific if available)
     * - L5-10: Starter regions
     * - L11-60: Leveling zones (weighted by level range)
     * - L61-80: Endgame zones (The War Within content)
     *
     * @param level         Bot level
     * @param faction       TEAM_ALLIANCE or TEAM_HORDE
     * @param race          Bot race (for starter zone selection)
     * @return              ZoneChoice with selected zone
     */
    ZoneChoice SelectZone(uint32 level, TeamId faction, uint8 race = 0);

    /**
     * Get starter zone for specific race
     * Used for L1-4 bots
     *
     * @param race          Player race
     * @param faction       Faction (for validation)
     * @return              ZoneChoice for starter zone
     */
    ZoneChoice GetStarterZone(uint8 race, TeamId faction);

    /**
     * Get all zones valid for level and faction
     * Useful for debugging and validation
     */
    ::std::vector<ZonePlacement const*> GetValidZones(uint32 level, TeamId faction) const;

    /**
     * Get random capital city for faction
     * Fallback option when no other zone is suitable
     */
    ZoneChoice GetCapitalCity(TeamId faction);

    // ====================================================================
    // TELEPORTATION (MAIN THREAD ONLY - Player API)
    // ====================================================================

    /**
     * Teleport bot to selected zone
     * MUST be called from main thread (Player API)
     *
     * Workflow:
     * 1. Get zone coordinates from placement
     * 2. Call Player::TeleportTo()
     * 3. Log successful placement
     *
     * NOTE: Call AFTER bot is fully initialized (gear, talents, etc.)
     *
     * @param bot           Bot player object
     * @param placement     Zone placement from SelectZone()
     * @return              True if successful
     */
    bool TeleportToZone(Player* bot, ZonePlacement const* placement);

    /**
     * Complete workflow: Select zone + teleport in one call
     * MUST be called from main thread
     *
     * @param bot           Bot player object
     * @param level         Bot level
     * @param faction       Bot faction
     * @param race          Bot race
     * @return              True if successful
     */
    bool PlaceBot(Player* bot, uint32 level, TeamId faction, uint8 race);

    // ====================================================================
    // ZONE QUERIES (Thread-safe, lock-free cache access)
    // ====================================================================

    /**
     * Get zone placement by zone ID
     * Thread-safe, returns cached placement
     */
    ZonePlacement const* GetZonePlacement(uint32 zoneId) const;

    /**
     * Get zone name by zone ID
     */
    ::std::string GetZoneName(uint32 zoneId) const;

    /**
     * Check if zone is valid for level/faction
     */
    bool IsZoneValid(uint32 zoneId, uint32 level, TeamId faction) const;

    // ====================================================================
    // STATISTICS & DEBUGGING
    // ====================================================================

    PositionerStats GetStats() const { return _stats; }
    void PrintZoneReport() const;
    ::std::string GetZoneSummary() const;

private:
    BotWorldPositioner() = default;
    ~BotWorldPositioner() = default;
    BotWorldPositioner(BotWorldPositioner const&) = delete;
    BotWorldPositioner& operator=(BotWorldPositioner const&) = delete;

    // ====================================================================
    // ZONE MANAGEMENT
    // ====================================================================

    void LoadZonesFromConfig();
    void LoadZonesFromDatabase();
    void BuildDefaultZones();      // Fallback if config is empty
    void ValidateZones();
    void BuildZoneCache();         // Build lookup structures
    void ApplyConfigOverrides();   // Apply user config overrides

    // ====================================================================
    // DATABASE ZONE DISCOVERY (Enterprise-Grade Implementation)
    // ====================================================================

    /**
     * Query innkeepers from creature/creature_template tables
     * Innkeepers provide the safest spawn points (hearthstone bind locations)
     */
    ::std::vector<DbSpawnPoint> QueryInnkeepers();

    /**
     * Query flight masters from creature/creature_template tables
     * Flight masters are at travel hubs - always safe locations
     */
    ::std::vector<DbSpawnPoint> QueryFlightMasters();

    /**
     * Query quest givers and cluster them into quest hubs
     * Uses spatial clustering to find town/camp centers
     */
    ::std::vector<QuestHub> QueryAndClusterQuestHubs();

    /**
     * Query graveyards as fallback spawn points
     * Uses world_safe_locs joined with graveyard_zone
     */
    ::std::vector<DbSpawnPoint> QueryGraveyards();

    /**
     * Get zone level ranges from quest_template
     * Determines appropriate level range for each zone based on quest content
     */
    ::std::unordered_map<uint32, ZoneLevelInfo> QueryZoneLevelRanges();

    /**
     * Determine faction from FactionTemplate DBC entry
     * Uses FactionGroup flags: FACTION_MASK_ALLIANCE (2), FACTION_MASK_HORDE (4)
     */
    TeamId DetermineFaction(uint16 factionTemplateId) const;

    /**
     * Get zone name from AreaTable DBC
     * Returns human-readable zone name for logging
     */
    ::std::string GetZoneNameFromDBC(uint32 zoneId) const;

    /**
     * Get zone ID from coordinates using TerrainMgr (SLOW - triggers VMap loading)
     * @deprecated Use GetZoneIdFromAreaId instead for startup queries
     */
    uint32 GetZoneIdFromCoordinates(uint32 mapId, float x, float y, float z) const;

    /**
     * Get zone ID from area ID using fast DBC lookup (no disk I/O)
     * Returns parent zone if areaId is a subzone
     */
    uint32 GetZoneIdFromAreaId(uint32 areaId) const;

    /**
     * Check if zone is a starter zone (level 1-10 content)
     */
    bool IsStarterZoneByContent(uint32 zoneId, uint32 minLevel, uint32 maxLevel) const;

    /**
     * Check if zone is a capital city
     */
    bool IsCapitalCity(uint32 zoneId) const;

    /**
     * Merge spawn point into existing zone or create new zone
     * Handles spawn point priority: Innkeeper > Flight Master > Quest Hub > Graveyard
     */
    void MergeSpawnPointIntoZone(DbSpawnPoint const& spawnPoint,
                                  ::std::unordered_map<uint32, ZoneLevelInfo> const& levelInfo);

    /**
     * Merge quest hub into zones
     */
    void MergeQuestHubIntoZone(QuestHub const& hub,
                                ::std::unordered_map<uint32, ZoneLevelInfo> const& levelInfo);

    /**
     * Select best spawn point for a zone from multiple candidates
     * Priority: Innkeeper > Flight Master > Quest Hub Centroid > Graveyard
     */
    DbSpawnPoint SelectBestSpawnPoint(::std::vector<DbSpawnPoint> const& candidates) const;

    // ====================================================================
    // ZONE SELECTION HELPERS
    // ====================================================================

    ZoneChoice SelectByLevelRange(uint32 level, TeamId faction) const;
    ZoneChoice SelectWeighted(::std::vector<ZonePlacement const*> const& candidates) const;
    float CalculateSuitability(ZonePlacement const* zone, uint32 level) const;

    // ====================================================================
    // RACE TO ZONE MAPPING (Starter Zones)
    // ====================================================================

    void BuildRaceZoneMapping();
    uint32 GetStarterZoneForRace(uint8 race) const;

    // ====================================================================
    // TELEPORTATION HELPERS
    // ====================================================================

    bool ValidateTeleportCoordinates(ZonePlacement const* placement) const;
    void LogPlacement(Player* bot, ZonePlacement const* placement);

    // ====================================================================
    // DATA STORAGE
    // ====================================================================

    // Master zone list (immutable after LoadZones)
    ::std::vector<ZonePlacement> _zones;

    // Quick lookups (immutable after BuildZoneCache)
    ::std::unordered_map<uint32, ZonePlacement const*> _zoneById;           // zoneId -> placement
    ::std::unordered_map<uint32, ::std::vector<ZonePlacement const*>> _zonesByLevel;  // level -> placements
    ::std::unordered_map<uint8, ::std::vector<ZonePlacement const*>> _starterZonesByRace;  // race -> placements

    // Faction-specific lookups
    ::std::vector<ZonePlacement const*> _allianceCapitals;
    ::std::vector<ZonePlacement const*> _hordeCapitals;

    // Database-discovered zone data (used during loading)
    ::std::unordered_map<uint32, ::std::vector<DbSpawnPoint>> _zoneSpawnPoints;  // zoneId -> spawn points
    ::std::unordered_map<uint32, SpawnPointType> _zoneBestSpawnType;              // zoneId -> best spawn type found

    // Config overrides
    ::std::unordered_map<uint32, ConfigZoneOverride> _configOverrides;
    ::std::unordered_set<uint32> _disabledZones;

    // Known capital city zone IDs (for fallback and special handling)
    static constexpr uint32 ZONE_STORMWIND = 1519;
    static constexpr uint32 ZONE_IRONFORGE = 1537;
    static constexpr uint32 ZONE_DARNASSUS = 1657;
    static constexpr uint32 ZONE_EXODAR = 3557;
    static constexpr uint32 ZONE_ORGRIMMAR = 1637;
    static constexpr uint32 ZONE_THUNDER_BLUFF = 1638;
    static constexpr uint32 ZONE_UNDERCITY = 1497;
    static constexpr uint32 ZONE_SILVERMOON = 3487;

    // Statistics
    PositionerStats _stats;

    // Initialization flag
    ::std::atomic<bool> _initialized{false};
};

} // namespace Playerbot

#define sBotWorldPositioner Playerbot::BotWorldPositioner::instance()
