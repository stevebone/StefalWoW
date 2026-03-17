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

#include "BotWorldPositioner.h"
#include "Player.h"
#include "Config/PlayerbotConfig.h"
#include "Log.h"
#include "World.h"
#include "MapManager.h"
#include "DatabaseEnv.h"
#include "DB2Stores.h"
#include "DBCEnums.h"
#include "ObjectMgr.h"
#include "TerrainMgr.h"
#include "PhasingHandler.h"
#include <algorithm>
#include <random>
#include <sstream>
#include <cmath>

namespace Playerbot
{

// ====================================================================
// SINGLETON INSTANCE
// ====================================================================

BotWorldPositioner* BotWorldPositioner::instance()
{
    static BotWorldPositioner instance;
    return &instance;
}

// ====================================================================
// INITIALIZATION
// ====================================================================

bool BotWorldPositioner::LoadZones()
{
    if (_initialized.load(::std::memory_order_acquire))
    {
        TC_LOG_WARN("playerbot", "BotWorldPositioner::LoadZones() - Already initialized, skipping");
        return true;
    }

    // Clear existing data
    _zones.clear();
    _zoneById.clear();
    _zonesByLevel.clear();
    _starterZonesByRace.clear();
    _allianceCapitals.clear();
    _hordeCapitals.clear();
    _zoneSpawnPoints.clear();
    _zoneBestSpawnType.clear();
    _configOverrides.clear();
    _disabledZones.clear();
    _stats = PositionerStats();

    // Step 1: Load config overrides (which zones are enabled/disabled, custom coordinates)
    LoadZonesFromConfig();

    // Step 2: Load zones from database (primary source - innkeepers, flight masters, quest hubs)
    bool dbLoadEnabled = sPlayerbotConfig->GetBool("Playerbot.Zones.LoadFromDatabase", true);
    if (dbLoadEnabled)
    {
        LoadZonesFromDatabase();
    }

    // Step 3: Apply config overrides to database-loaded zones
    if (!_configOverrides.empty())
    {
        ApplyConfigOverrides();
    }

    // Step 4: If still no zones, build hardcoded defaults as fallback
    if (_zones.empty())
    {
        TC_LOG_WARN("playerbot", "BotWorldPositioner: No zones from database, using hardcoded defaults");
        BuildDefaultZones();
    }

    // Step 5: Validate all zones
    ValidateZones();

    // Step 6: Build lookup structures
    BuildZoneCache();

    // Update statistics
    _stats.totalZones = static_cast<uint32>(_zones.size());
    for (auto const& zone : _zones)
    {
        if (zone.isStarterZone)
            ++_stats.starterZones;
        else if (zone.maxLevel <= 60)
            ++_stats.levelingZones;
        else
            ++_stats.endgameZones;
    }
    _stats.capitalCities = static_cast<uint32>(_allianceCapitals.size() + _hordeCapitals.size());

    _initialized.store(true, ::std::memory_order_release);

    TC_LOG_INFO("playerbot", "BotWorldPositioner::LoadZones() - Loaded {} zones ({} starter, {} leveling, {} endgame, {} capitals)",
        _stats.totalZones, _stats.starterZones, _stats.levelingZones, _stats.endgameZones, _stats.capitalCities);

    // Print detailed zone report if debug enabled
    if (sPlayerbotConfig->GetBool("Playerbot.Zones.DebugReport", false))
    {
        PrintZoneReport();
    }

    return true;
}

void BotWorldPositioner::ReloadZones()
{
    _initialized.store(false, ::std::memory_order_release);
    LoadZones();
}

void BotWorldPositioner::LoadZonesFromConfig()
{

    // Load disabled zones list
    // Format: Playerbot.Zones.Disabled = "zoneId1,zoneId2,zoneId3"
    ::std::string disabledZonesStr = sPlayerbotConfig->GetString("Playerbot.Zones.Disabled", "");
    if (!disabledZonesStr.empty())
    {
        ::std::stringstream ss(disabledZonesStr);
        ::std::string token;
        while (::std::getline(ss, token, ','))
        {
            try
            {
                uint32 zoneId = static_cast<uint32>(::std::stoul(token));
                _disabledZones.insert(zoneId);
                TC_LOG_DEBUG("playerbot", "BotWorldPositioner::LoadZonesFromConfig() - Zone {} disabled by config", zoneId);
            }
            catch (...)
            {
                TC_LOG_ERROR("playerbot", "BotWorldPositioner::LoadZonesFromConfig() - Invalid zone ID in disabled list: {}", token);
            }
        }
    }

    // Load custom zone overrides
    // Format: Playerbot.Zones.Override.{ZoneId} = "x,y,z,minLevel,maxLevel,faction"
    // Example: Playerbot.Zones.Override.12 = "-8949.95,-132.493,83.5312,1,10,0"
    // faction: 0=Alliance, 1=Horde, 2=Neutral
    for (uint32 zoneId = 1; zoneId < 20000; ++zoneId)
    {
        ::std::string configKey = "Playerbot.Zones.Override." + ::std::to_string(zoneId);
        ::std::string overrideStr = sPlayerbotConfig->GetString(configKey, "");

        if (overrideStr.empty())
            continue;

        ConfigZoneOverride override;
        override.zoneId = zoneId;
        override.enabled = true;

        ::std::stringstream ss(overrideStr);
        ::std::string token;
        int fieldIndex = 0;

        while (::std::getline(ss, token, ',') && fieldIndex < 6)
        {
            try
            {
                switch (fieldIndex)
                {
                    case 0: override.x = ::std::stof(token); break;
                    case 1: override.y = ::std::stof(token); break;
                    case 2: override.z = ::std::stof(token); break;
                    case 3: override.minLevel = static_cast<uint32>(::std::stoul(token)); break;
                    case 4: override.maxLevel = static_cast<uint32>(::std::stoul(token)); break;
                    case 5:
                    {
                        int factionInt = ::std::stoi(token);
                        if (factionInt == 0) override.faction = TEAM_ALLIANCE;
                        else if (factionInt == 1) override.faction = TEAM_HORDE;
                        else override.faction = TEAM_NEUTRAL;
                        break;
                    }
                }
            }
            catch (...)
            {
                TC_LOG_ERROR("playerbot", "BotWorldPositioner::LoadZonesFromConfig() - Invalid override value for zone {}: {}", zoneId, token);
            }
            ++fieldIndex;
        }

        _configOverrides[zoneId] = override;
        TC_LOG_DEBUG("playerbot", "BotWorldPositioner::LoadZonesFromConfig() - Loaded override for zone {}", zoneId);
    }

    // Load additional custom zones (not in database)
    // Format: Playerbot.Zones.Custom.{Index} = "zoneId,mapId,x,y,z,o,minLevel,maxLevel,faction,name"
    for (int i = 1; i <= 100; ++i)
    {
        ::std::string configKey = "Playerbot.Zones.Custom." + ::std::to_string(i);
        ::std::string customZoneStr = sPlayerbotConfig->GetString(configKey, "");

        if (customZoneStr.empty())
            continue;

        ZonePlacement zone;
        ::std::stringstream ss(customZoneStr);
        ::std::string token;
        int fieldIndex = 0;

        while (::std::getline(ss, token, ',') && fieldIndex < 10)
        {
            try
            {
                switch (fieldIndex)
                {
                    case 0: zone.zoneId = static_cast<uint32>(::std::stoul(token)); break;
                    case 1: zone.mapId = static_cast<uint32>(::std::stoul(token)); break;
                    case 2: zone.x = ::std::stof(token); break;
                    case 3: zone.y = ::std::stof(token); break;
                    case 4: zone.z = ::std::stof(token); break;
                    case 5: zone.orientation = ::std::stof(token); break;
                    case 6: zone.minLevel = static_cast<uint32>(::std::stoul(token)); break;
                    case 7: zone.maxLevel = static_cast<uint32>(::std::stoul(token)); break;
                    case 8:
                    {
                        int factionInt = ::std::stoi(token);
                        if (factionInt == 0) zone.faction = TEAM_ALLIANCE;
                        else if (factionInt == 1) zone.faction = TEAM_HORDE;
                        else zone.faction = TEAM_NEUTRAL;
                        break;
                    }
                    case 9: zone.zoneName = token; break;
                }
            }
            catch (...)
            {
                TC_LOG_ERROR("playerbot", "BotWorldPositioner::LoadZonesFromConfig() - Invalid custom zone field {}: {}", fieldIndex, token);
            }
            ++fieldIndex;
        }

        if (zone.zoneId > 0 && !zone.zoneName.empty())
        {
            zone.isStarterZone = IsStarterZoneByContent(zone.zoneId, zone.minLevel, zone.maxLevel);
            _zones.push_back(zone);
            TC_LOG_INFO("playerbot", "BotWorldPositioner::LoadZonesFromConfig() - Added custom zone: {} ({})", zone.zoneName, zone.zoneId);
        }
    }

    if (!_disabledZones.empty() || !_configOverrides.empty() || !_zones.empty())
    {
        TC_LOG_DEBUG("playerbot", "BotWorldPositioner::LoadZonesFromConfig() - {} disabled, {} overrides, {} custom zones",
            _disabledZones.size(), _configOverrides.size(), _zones.size());
    }
}

// ============================================================================
// DATABASE ZONE DISCOVERY - Enterprise-Grade Implementation
// ============================================================================

void BotWorldPositioner::LoadZonesFromDatabase()
{
    uint32 startTime = getMSTime();

    // Step 1: Get zone level ranges from quest data
    auto zoneLevelInfo = QueryZoneLevelRanges();

    // Step 2: Query innkeepers (highest priority spawn points)
    auto innkeepers = QueryInnkeepers();
    for (auto const& innkeeper : innkeepers)
    {
        if (_disabledZones.count(innkeeper.zoneId))
            continue;
        MergeSpawnPointIntoZone(innkeeper, zoneLevelInfo);
    }

    // Step 3: Query flight masters (second priority)
    auto flightMasters = QueryFlightMasters();
    for (auto const& fm : flightMasters)
    {
        if (_disabledZones.count(fm.zoneId))
            continue;
        MergeSpawnPointIntoZone(fm, zoneLevelInfo);
    }

    // Step 4: Query and cluster quest hubs (third priority)
    auto questHubs = QueryAndClusterQuestHubs();
    for (size_t i = 0; i < questHubs.size(); ++i)
    {
        QuestHub const& hub = questHubs[i];
        if (_disabledZones.count(hub.zoneId))
            continue;
        MergeQuestHubIntoZone(hub, zoneLevelInfo);
    }

    // Step 5: Query graveyards as fallback (fourth priority)
    auto graveyards = QueryGraveyards();
    for (auto const& gy : graveyards)
    {
        if (_disabledZones.count(gy.zoneId))
            continue;
        MergeSpawnPointIntoZone(gy, zoneLevelInfo);
    }

    // Step 6: Convert discovered zones to ZonePlacement structs

    for (auto const& [zoneId, spawnPoints] : _zoneSpawnPoints)
    {
        if (spawnPoints.empty())
            continue;

        // Select the best spawn point for this zone
        DbSpawnPoint best = SelectBestSpawnPoint(spawnPoints);
        if (!best.IsValid())
            continue;

        // Get level info for this zone
        ZoneLevelInfo levelInfo;
        auto levelIt = zoneLevelInfo.find(zoneId);
        if (levelIt != zoneLevelInfo.end())
        {
            levelInfo = levelIt->second;
        }
        else
        {
            // Default level range if no quest data
            levelInfo.minLevel = 1;
            levelInfo.maxLevel = 80;
        }

        // Create zone placement
        ZonePlacement zone;
        zone.zoneId = zoneId;
        zone.mapId = best.mapId;
        zone.x = best.x;
        zone.y = best.y;
        zone.z = best.z;
        zone.orientation = best.orientation;
        zone.minLevel = levelInfo.minLevel;
        zone.maxLevel = levelInfo.maxLevel;
        zone.faction = best.factionTemplateId > 0 ? DetermineFaction(best.factionTemplateId) : TEAM_NEUTRAL;
        zone.zoneName = GetZoneNameFromDBC(zoneId);
        zone.isStarterZone = IsStarterZoneByContent(zoneId, levelInfo.minLevel, levelInfo.maxLevel);

        // Don't add zones without a name (invalid zone IDs)
        if (zone.zoneName.empty() || zone.zoneName == "Unknown Zone")
        {
            zone.zoneName = best.npcName.empty() ? "Zone " + ::std::to_string(zoneId) : best.npcName + " Area";
        }

        _zones.push_back(zone);

        TC_LOG_DEBUG("playerbot", "BotWorldPositioner::LoadZonesFromDatabase() - Added zone {} '{}' (L{}-{}, {})",
            zone.zoneId, zone.zoneName, zone.minLevel, zone.maxLevel,
            zone.faction == TEAM_ALLIANCE ? "Alliance" : (zone.faction == TEAM_HORDE ? "Horde" : "Neutral"));
    }

    uint32 elapsed = getMSTimeDiff(startTime, getMSTime());
    TC_LOG_INFO("playerbot", "BotWorldPositioner: Discovered {} zones from database in {}ms", _zones.size(), elapsed);
}

void BotWorldPositioner::ApplyConfigOverrides()
{

    for (auto& zone : _zones)
    {
        auto overrideIt = _configOverrides.find(zone.zoneId);
        if (overrideIt == _configOverrides.end())
            continue;

        ConfigZoneOverride const& override = overrideIt->second;

        if (!override.enabled)
        {
            // Mark zone for removal (we'll remove after iteration)
            _disabledZones.insert(zone.zoneId);
            continue;
        }

        // Apply coordinate overrides
        if (override.x.has_value()) zone.x = override.x.value();
        if (override.y.has_value()) zone.y = override.y.value();
        if (override.z.has_value()) zone.z = override.z.value();

        // Apply level range overrides
        if (override.minLevel.has_value()) zone.minLevel = override.minLevel.value();
        if (override.maxLevel.has_value()) zone.maxLevel = override.maxLevel.value();

        // Apply faction override
        if (override.faction.has_value()) zone.faction = override.faction.value();

        TC_LOG_DEBUG("playerbot", "BotWorldPositioner::ApplyConfigOverrides() - Applied override to zone {}", zone.zoneId);
    }

    // Remove disabled zones
    _zones.erase(
        ::std::remove_if(_zones.begin(), _zones.end(),
            [this](ZonePlacement const& z) { return _disabledZones.count(z.zoneId) > 0; }),
        _zones.end());
}

// ============================================================================
// DATABASE QUERY IMPLEMENTATIONS
// ============================================================================

::std::vector<DbSpawnPoint> BotWorldPositioner::QueryInnkeepers()
{
    ::std::vector<DbSpawnPoint> result;

    // Query innkeepers from creature/creature_template
    // npcflag & 65536 = UNIT_NPC_FLAG_INNKEEPER
    QueryResult dbResult = WorldDatabase.Query(
        "SELECT ct.entry, ct.name, ct.faction, c.map, c.zoneId, c.areaId, "
        "c.position_x, c.position_y, c.position_z, c.orientation "
        "FROM creature c "
        "JOIN creature_template ct ON c.id = ct.entry "
        "WHERE ct.npcflag & 65536 "
        "AND c.map IN (0, 1, 530, 571, 870, 1116, 1220, 1643, 2222, 2444, 2601) " // Major continents
        "ORDER BY c.map, c.position_x");

    if (!dbResult)
    {
        TC_LOG_WARN("playerbot", "BotWorldPositioner::QueryInnkeepers() - No innkeepers found in database");
        return result;
    }

    do
    {
        Field* fields = dbResult->Fetch();

        DbSpawnPoint spawn;
        spawn.creatureEntry = fields[0].GetUInt32();
        spawn.npcName = fields[1].GetString();
        spawn.factionTemplateId = fields[2].GetUInt16();
        spawn.mapId = fields[3].GetUInt32();
        spawn.zoneId = fields[4].GetUInt32();
        spawn.areaId = fields[5].GetUInt32();
        spawn.x = fields[6].GetFloat();
        spawn.y = fields[7].GetFloat();
        spawn.z = fields[8].GetFloat();
        spawn.orientation = fields[9].GetFloat();
        spawn.type = SpawnPointType::INNKEEPER;

        // If zone not populated, try to get from areaId (fast DBC lookup)
        if (spawn.zoneId == 0 && spawn.areaId > 0)
        {
            spawn.zoneId = GetZoneIdFromAreaId(spawn.areaId);
        }

        // Skip if still no valid zone (don't use expensive coordinate lookup)
        if (spawn.zoneId == 0)
            continue;

        result.push_back(spawn);

    } while (dbResult->NextRow());

    TC_LOG_DEBUG("playerbot", "BotWorldPositioner::QueryInnkeepers() - Found {} valid innkeeper spawn points", result.size());
    return result;
}

::std::vector<DbSpawnPoint> BotWorldPositioner::QueryFlightMasters()
{
    ::std::vector<DbSpawnPoint> result;

    // Query flight masters from creature/creature_template
    // npcflag & 8192 = UNIT_NPC_FLAG_FLIGHTMASTER
    QueryResult dbResult = WorldDatabase.Query(
        "SELECT ct.entry, ct.name, ct.faction, c.map, c.zoneId, c.areaId, "
        "c.position_x, c.position_y, c.position_z, c.orientation "
        "FROM creature c "
        "JOIN creature_template ct ON c.id = ct.entry "
        "WHERE ct.npcflag & 8192 "
        "AND c.map IN (0, 1, 530, 571, 870, 1116, 1220, 1643, 2222, 2444, 2601) "
        "ORDER BY c.map, c.position_x");

    if (!dbResult)
    {
        TC_LOG_WARN("playerbot", "BotWorldPositioner::QueryFlightMasters() - No flight masters found in database");
        return result;
    }

    do
    {
        Field* fields = dbResult->Fetch();

        DbSpawnPoint spawn;
        spawn.creatureEntry = fields[0].GetUInt32();
        spawn.npcName = fields[1].GetString();
        spawn.factionTemplateId = fields[2].GetUInt16();
        spawn.mapId = fields[3].GetUInt32();
        spawn.zoneId = fields[4].GetUInt32();
        spawn.areaId = fields[5].GetUInt32();
        spawn.x = fields[6].GetFloat();
        spawn.y = fields[7].GetFloat();
        spawn.z = fields[8].GetFloat();
        spawn.orientation = fields[9].GetFloat();
        spawn.type = SpawnPointType::FLIGHT_MASTER;

        // If zone not populated, try to get from areaId (fast DBC lookup)
        if (spawn.zoneId == 0 && spawn.areaId > 0)
        {
            spawn.zoneId = GetZoneIdFromAreaId(spawn.areaId);
        }

        // Skip if still no valid zone (don't use expensive coordinate lookup)
        if (spawn.zoneId == 0)
            continue;

        result.push_back(spawn);

    } while (dbResult->NextRow());

    TC_LOG_DEBUG("playerbot", "BotWorldPositioner::QueryFlightMasters() - Found {} valid flight master spawn points", result.size());
    return result;
}

::std::vector<QuestHub> BotWorldPositioner::QueryAndClusterQuestHubs()
{
    ::std::vector<QuestHub> result;
    ::std::unordered_map<uint64, QuestHub> hubsByLocation;  // Grid cell -> hub

    TC_LOG_DEBUG("playerbot", "QueryAndClusterQuestHubs() - Starting database query...");

    // Query quest givers with their positions
    // Only query zones that have zoneId or areaId populated (skip ~75% of invalid data)
    QueryResult dbResult = WorldDatabase.Query(
        "SELECT DISTINCT ct.entry, ct.faction, c.map, c.zoneId, c.areaId, "
        "c.position_x, c.position_y, c.position_z "
        "FROM creature c "
        "JOIN creature_template ct ON c.id = ct.entry "
        "JOIN creature_queststarter cqs ON ct.entry = cqs.id "
        "WHERE c.map IN (0, 1, 530, 571, 870, 1116, 1220, 1643, 2222, 2444, 2601) "
        "AND (c.zoneId > 0 OR c.areaId > 0) "
        "ORDER BY c.map, c.zoneId, c.position_x, c.position_y");

    if (!dbResult)
    {
        TC_LOG_WARN("playerbot", "BotWorldPositioner::QueryAndClusterQuestHubs() - No quest givers found");
        return result;
    }

    TC_LOG_DEBUG("playerbot", "QueryAndClusterQuestHubs() - Query complete, processing results...");

    // Cluster quest givers by spatial proximity (100 yard grid cells)
    constexpr float CLUSTER_GRID_SIZE = 100.0f;
    uint32 rowCount = 0;

    do
    {
        Field* fields = dbResult->Fetch();
        ++rowCount;

        uint32 entry = fields[0].GetUInt32();
        uint16 factionId = fields[1].GetUInt16();
        uint32 mapId = fields[2].GetUInt32();
        uint32 zoneId = fields[3].GetUInt32();
        uint32 areaId = fields[4].GetUInt32();
        float x = fields[5].GetFloat();
        float y = fields[6].GetFloat();
        float z = fields[7].GetFloat();

        // Get zone from areaId if not populated (fast DBC lookup)
        if (zoneId == 0 && areaId > 0)
        {
            zoneId = GetZoneIdFromAreaId(areaId);
        }

        // Skip if still no valid zone (don't use expensive coordinate lookup)
        if (zoneId == 0)
            continue;

        // Calculate grid cell key (map + zone + grid coordinates)
        int32 gridX = static_cast<int32>(x / CLUSTER_GRID_SIZE);
        int32 gridY = static_cast<int32>(y / CLUSTER_GRID_SIZE);
        uint64 cellKey = (static_cast<uint64>(mapId) << 48) |
                        (static_cast<uint64>(zoneId) << 32) |
                        (static_cast<uint64>(static_cast<uint32>(gridX + 10000)) << 16) |
                        static_cast<uint64>(static_cast<uint32>(gridY + 10000));

        // Add to existing hub or create new one
        auto& hub = hubsByLocation[cellKey];
        if (hub.mapId == 0)
        {
            hub.mapId = mapId;
            hub.zoneId = zoneId;
            hub.faction = DetermineFaction(factionId);
        }
        hub.AddQuestGiver(x, y, z);

    } while (dbResult->NextRow());

    TC_LOG_DEBUG("playerbot", "QueryAndClusterQuestHubs() - Processed {} rows, found {} grid cells", rowCount, hubsByLocation.size());

    // Filter for significant hubs (2+ quest givers)
    result.reserve(hubsByLocation.size());

    for (auto const& [key, hub] : hubsByLocation)
    {
        if (hub.questGiverCount >= 2)
            result.push_back(hub);
    }

    TC_LOG_DEBUG("playerbot", "QueryAndClusterQuestHubs() - Clustered {} quest hubs from {} grid cells", result.size(), hubsByLocation.size());
    return result;
}

::std::vector<DbSpawnPoint> BotWorldPositioner::QueryGraveyards()
{
    ::std::vector<DbSpawnPoint> result;

    // Query graveyards with zone association
    QueryResult dbResult = WorldDatabase.Query(
        "SELECT wsl.ID, wsl.MapID, wsl.LocX, wsl.LocY, wsl.LocZ, wsl.Facing, "
        "gz.GhostZone, wsl.Comment "
        "FROM world_safe_locs wsl "
        "JOIN graveyard_zone gz ON wsl.ID = gz.ID "
        "WHERE wsl.MapID IN (0, 1, 530, 571, 870, 1116, 1220, 1643, 2222, 2444, 2601) "
        "ORDER BY wsl.MapID, gz.GhostZone");

    if (!dbResult)
    {
        TC_LOG_WARN("playerbot", "BotWorldPositioner::QueryGraveyards() - No graveyards found");
        return result;
    }

    do
    {
        Field* fields = dbResult->Fetch();

        DbSpawnPoint spawn;
        spawn.creatureEntry = fields[0].GetUInt32();  // Graveyard ID
        spawn.mapId = fields[1].GetUInt32();
        spawn.x = fields[2].GetFloat();
        spawn.y = fields[3].GetFloat();
        spawn.z = fields[4].GetFloat();
        spawn.orientation = fields[5].GetFloat();
        spawn.zoneId = fields[6].GetUInt32();  // GhostZone
        spawn.npcName = fields[7].GetString();  // Comment
        spawn.factionTemplateId = 0;  // Graveyards are typically faction-neutral
        spawn.type = SpawnPointType::GRAVEYARD;

        if (spawn.zoneId == 0)
            continue;

        result.push_back(spawn);

    } while (dbResult->NextRow());

    TC_LOG_DEBUG("playerbot", "BotWorldPositioner::QueryGraveyards() - Found {} graveyard spawn points", result.size());
    return result;
}

::std::unordered_map<uint32, ZoneLevelInfo> BotWorldPositioner::QueryZoneLevelRanges()
{
    ::std::unordered_map<uint32, ZoneLevelInfo> result;

    // Use DBC stores to get zone level ranges via ContentTuning
    // AreaTable has ContentTuningID which links to ContentTuning for level data
    for (AreaTableEntry const* area : sAreaTableStore)
    {
        if (!area)
            continue;

        // Get zone ID (top-level area for this entry)
        uint32 zoneId = area->ID;
        if (area->ParentAreaID > 0)
        {
            // This is a subzone, use parent as zone
            zoneId = area->ParentAreaID;
        }

        // Skip if no content tuning
        if (area->ContentTuningID == 0)
            continue;

        // Look up content tuning for level data
        ContentTuningEntry const* contentTuning = sContentTuningStore.LookupEntry(area->ContentTuningID);
        if (!contentTuning)
            continue;

        // Get level range from content tuning
        int32 minLevel = contentTuning->MinLevel;
        int32 maxLevel = contentTuning->MaxLevel;

        // Skip invalid level ranges
        if (minLevel <= 0 && maxLevel <= 0)
            continue;

        // Apply sensible defaults
        if (minLevel <= 0)
            minLevel = 1;
        if (maxLevel <= 0)
            maxLevel = minLevel;

        // Ensure minLevel <= maxLevel
        if (minLevel > maxLevel)
            ::std::swap(minLevel, maxLevel);

        // Cap at sensible values for current expansion
        minLevel = ::std::max(1, ::std::min(80, minLevel));
        maxLevel = ::std::max(minLevel, ::std::min(80, maxLevel));

        // Check if we already have this zone
        auto it = result.find(zoneId);
        if (it != result.end())
        {
            // Merge level ranges - take the widest range
            it->second.minLevel = ::std::min(it->second.minLevel, static_cast<uint32>(minLevel));
            it->second.maxLevel = ::std::max(it->second.maxLevel, static_cast<uint32>(maxLevel));
            it->second.questCount++;  // Count as additional subzone
        }
        else
        {
            ZoneLevelInfo info;
            info.zoneId = zoneId;
            info.minLevel = static_cast<uint32>(minLevel);
            info.maxLevel = static_cast<uint32>(maxLevel);
            info.questCount = 1;
            result[zoneId] = info;
        }
    }

    TC_LOG_DEBUG("playerbot", "BotWorldPositioner::QueryZoneLevelRanges() - Found level data for {} zones from DBC", result.size());
    return result;
}

// ============================================================================
// HELPER IMPLEMENTATIONS
// ============================================================================

TeamId BotWorldPositioner::DetermineFaction(uint16 factionTemplateId) const
{
    if (factionTemplateId == 0)
        return TEAM_NEUTRAL;

    // Get faction template from DBC
    FactionTemplateEntry const* factionTemplate = sFactionTemplateStore.LookupEntry(factionTemplateId);
    if (!factionTemplate)
        return TEAM_NEUTRAL;

    // Check FactionGroup flags
    // FACTION_MASK_ALLIANCE = 2, FACTION_MASK_HORDE = 4
    if (factionTemplate->FactionGroup & FACTION_MASK_ALLIANCE)
        return TEAM_ALLIANCE;
    if (factionTemplate->FactionGroup & FACTION_MASK_HORDE)
        return TEAM_HORDE;

    // Check FriendGroup for indirect faction association
    if (factionTemplate->FriendGroup & FACTION_MASK_ALLIANCE)
        return TEAM_ALLIANCE;
    if (factionTemplate->FriendGroup & FACTION_MASK_HORDE)
        return TEAM_HORDE;

    return TEAM_NEUTRAL;
}

::std::string BotWorldPositioner::GetZoneNameFromDBC(uint32 zoneId) const
{
    AreaTableEntry const* area = sAreaTableStore.LookupEntry(zoneId);
    if (!area)
        return "Unknown Zone";

    return area->AreaName[sWorld->GetDefaultDbcLocale()];
}

uint32 BotWorldPositioner::GetZoneIdFromCoordinates(uint32 /*mapId*/, float /*x*/, float /*y*/, float /*z*/) const
{
    // DISABLED: TerrainMgr coordinate lookup is too slow during startup (triggers VMap loading)
    // Use GetZoneIdFromAreaId() instead which is a fast DBC lookup
    // If truly needed at runtime, uncomment below:
    // uint32 zoneId = sTerrainMgr.GetZoneId(PhasingHandler::GetAlwaysVisiblePhaseShift(), mapId, x, y, z);
    // return zoneId;
    return 0;
}

uint32 BotWorldPositioner::GetZoneIdFromAreaId(uint32 areaId) const
{
    if (areaId == 0)
        return 0;

    // Fast DBC lookup - no disk I/O, just memory access
    AreaTableEntry const* area = sAreaTableStore.LookupEntry(areaId);
    if (!area)
        return 0;

    // If this is a subzone, return the parent zone ID
    if (area->ParentAreaID > 0)
        return area->ParentAreaID;

    // This is already a top-level zone
    return areaId;
}

bool BotWorldPositioner::IsStarterZoneByContent(uint32 zoneId, uint32 minLevel, uint32 maxLevel) const
{
    // Starter zones are level 1-10 content
    if (minLevel <= 1 && maxLevel <= 12)
        return true;

    // Known starter zone IDs (race starting areas)
    static const ::std::unordered_set<uint32> starterZoneIds = {
        // Alliance
        12,     // Elwynn Forest
        132,    // Dun Morogh
        188,    // Teldrassil
        3524,   // Azuremyst Isle
        4755,   // Gilneas
        6170,   // Northshire (subzone)
        // Horde
        14,     // Durotar
        85,     // Tirisfal Glades
        215,    // Mulgore
        3430,   // Eversong Woods
        4720,   // Lost Isles (Goblin)
        // Neutral
        5736,   // Wandering Isle (Pandaren)
    };

    return starterZoneIds.count(zoneId) > 0;
}

bool BotWorldPositioner::IsCapitalCity(uint32 zoneId) const
{
    return zoneId == ZONE_STORMWIND || zoneId == ZONE_IRONFORGE ||
           zoneId == ZONE_DARNASSUS || zoneId == ZONE_EXODAR ||
           zoneId == ZONE_ORGRIMMAR || zoneId == ZONE_THUNDER_BLUFF ||
           zoneId == ZONE_UNDERCITY || zoneId == ZONE_SILVERMOON;
}

void BotWorldPositioner::MergeSpawnPointIntoZone(DbSpawnPoint const& spawnPoint,
                                                   ::std::unordered_map<uint32, ZoneLevelInfo> const& levelInfo)
{
    (void)levelInfo;  // Reserved for future level-based spawn filtering
    uint32 zoneId = spawnPoint.zoneId;

    // Check if we should replace existing spawn point based on priority
    auto bestTypeIt = _zoneBestSpawnType.find(zoneId);
    if (bestTypeIt != _zoneBestSpawnType.end())
    {
        // Lower enum value = higher priority
        if (static_cast<uint8>(spawnPoint.type) >= static_cast<uint8>(bestTypeIt->second))
        {
            // Existing spawn point is equal or higher priority, just add to list
            _zoneSpawnPoints[zoneId].push_back(spawnPoint);
            return;
        }
    }

    // This is a higher priority spawn point
    _zoneBestSpawnType[zoneId] = spawnPoint.type;
    _zoneSpawnPoints[zoneId].push_back(spawnPoint);
}

void BotWorldPositioner::MergeQuestHubIntoZone(QuestHub const& hub,
                                                 ::std::unordered_map<uint32, ZoneLevelInfo> const& levelInfo)
{
    // Convert quest hub to DbSpawnPoint
    DbSpawnPoint spawnPoint;
    spawnPoint.creatureEntry = 0;
    spawnPoint.mapId = hub.mapId;
    spawnPoint.zoneId = hub.zoneId;
    spawnPoint.areaId = 0;
    spawnPoint.x = hub.centroidX;
    spawnPoint.y = hub.centroidY;
    spawnPoint.z = hub.centroidZ;
    spawnPoint.orientation = 0.0f;
    spawnPoint.factionTemplateId = 0;  // Will be determined by NPC faction
    spawnPoint.type = SpawnPointType::QUEST_GIVER;
    spawnPoint.npcName = "Quest Hub (" + ::std::to_string(hub.questGiverCount) + " NPCs)";

    // Try to determine faction from hub
    if (hub.faction != TEAM_NEUTRAL)
    {
        // Set a faction template ID that will map to the correct team
        // These are common faction template IDs for Alliance/Horde
        spawnPoint.factionTemplateId = (hub.faction == TEAM_ALLIANCE) ? 12 : 29;
    }

    MergeSpawnPointIntoZone(spawnPoint, levelInfo);
}

DbSpawnPoint BotWorldPositioner::SelectBestSpawnPoint(::std::vector<DbSpawnPoint> const& candidates) const
{
    if (candidates.empty())
        return DbSpawnPoint();

    // Find highest priority spawn point (lowest enum value)
    DbSpawnPoint const* best = &candidates[0];

    for (auto const& spawn : candidates)
    {
        if (static_cast<uint8>(spawn.type) < static_cast<uint8>(best->type))
        {
            best = &spawn;
        }
    }

    return *best;
}

void BotWorldPositioner::BuildDefaultZones()
{
    // ====================================================================
    // ALLIANCE STARTER ZONES (L1-10)
    // ====================================================================

    // Human - Elwynn Forest (Northshire Abbey)
    _zones.push_back({12, 0, -8949.95f, -132.493f, 83.5312f, 0.0f, 1, 10, TEAM_ALLIANCE, "Elwynn Forest", true});

    // Dwarf/Gnome - Dun Morogh (Coldridge Valley)
    _zones.push_back({132, 0, -6240.32f, 331.033f, 382.758f, 0.0f, 1, 10, TEAM_ALLIANCE, "Dun Morogh", true});

    // Night Elf - Teldrassil (Shadowglen)
    _zones.push_back({188, 1, 10311.3f, 832.463f, 1326.41f, 5.69632f, 1, 10, TEAM_ALLIANCE, "Teldrassil", true});

    // Draenei - Azuremyst Isle (Ammen Vale)
    _zones.push_back({3524, 530, -4192.62f, -12576.7f, 36.7598f, 0.0f, 1, 10, TEAM_ALLIANCE, "Azuremyst Isle", true});

    // Worgen - Gilneas (Starting zone, phased)
    _zones.push_back({4755, 654, -1676.07f, 1345.55f, 15.1353f, 0.0f, 1, 10, TEAM_ALLIANCE, "Gilneas", true});

    // Pandaren (Alliance) - Stormwind City (post-tutorial)
    _zones.push_back({1519, 0, -8833.38f, 628.628f, 94.0066f, 1.06465f, 1, 10, TEAM_ALLIANCE, "Stormwind City", true});

    // ====================================================================
    // HORDE STARTER ZONES (L1-10)
    // ====================================================================

    // Orc/Troll - Durotar (Valley of Trials)
    _zones.push_back({14, 1, -602.608f, -4262.17f, 38.9529f, 0.0f, 1, 10, TEAM_HORDE, "Durotar", true});

    // Undead - Tirisfal Glades (Deathknell)
    _zones.push_back({85, 0, 1676.71f, 1678.31f, 121.67f, 2.70526f, 1, 10, TEAM_HORDE, "Tirisfal Glades", true});

    // Tauren - Mulgore (Red Cloud Mesa)
    _zones.push_back({215, 1, -2917.58f, -257.98f, 52.9968f, 0.0f, 1, 10, TEAM_HORDE, "Mulgore", true});

    // Blood Elf - Eversong Woods (Sunstrider Isle)
    _zones.push_back({3430, 530, 10349.6f, -6357.29f, 33.4026f, 5.31605f, 1, 10, TEAM_HORDE, "Eversong Woods", true});

    // Goblin - Kezan/Lost Isles (post-tutorial -> Orgrimmar)
    _zones.push_back({1637, 1, 1574.0f, -4439.0f, 15.4449f, 1.84061f, 1, 10, TEAM_HORDE, "Orgrimmar", true});

    // Pandaren (Horde) - Orgrimmar (post-tutorial)
    _zones.push_back({1637, 1, 1574.0f, -4439.0f, 15.4449f, 1.84061f, 1, 10, TEAM_HORDE, "Orgrimmar", true});

    // ====================================================================
    // LEVELING ZONES (L10-60)
    // ====================================================================

    // Alliance Leveling
    _zones.push_back({40, 0, -9449.06f, 64.8392f, 56.3581f, 0.0f, 10, 20, TEAM_ALLIANCE, "Westfall", false});
    _zones.push_back({3, 0, -10531.7f, -1281.91f, 38.8647f, 1.56959f, 15, 25, TEAM_ALLIANCE, "Redridge Mountains", false});
    _zones.push_back({38, 0, -11209.6f, 1666.54f, 24.6974f, 1.42053f, 20, 30, TEAM_ALLIANCE, "Duskwood", false});
    _zones.push_back({4, 0, -14297.2f, 518.269f, 8.77916f, 4.4586f, 30, 40, TEAM_ALLIANCE, "Stranglethorn Vale", false});

    // Horde Leveling
    _zones.push_back({17, 1, 304.614f, -4741.87f, 10.1027f, 0.0f, 10, 20, TEAM_HORDE, "The Barrens", false});
    _zones.push_back({406, 1, 6860.03f, -4767.11f, 696.833f, 5.31605f, 15, 25, TEAM_HORDE, "Stonetalon Mountains", false});
    _zones.push_back({16, 1, 2243.0f, -2487.0f, 97.05f, 0.72f, 20, 30, TEAM_HORDE, "Ashenvale", false});
    _zones.push_back({331, 1, -7176.38f, -3782.57f, 8.36981f, 6.00393f, 30, 40, TEAM_HORDE, "Desolace", false});

    // Neutral Leveling
    _zones.push_back({8, 0, -4919.88f, -3650.25f, 301.797f, 3.926991f, 40, 50, TEAM_NEUTRAL, "Searing Gorge", false});
    _zones.push_back({28, 0, -7179.0f, -921.0f, 165.377f, 5.09599f, 45, 55, TEAM_NEUTRAL, "Western Plaguelands", false});
    _zones.push_back({139, 0, 3352.92f, -3379.03f, 144.782f, 6.25562f, 50, 60, TEAM_NEUTRAL, "Eastern Plaguelands", false});

    // ====================================================================
    // ENDGAME ZONES (L60-80) - The War Within Content
    // ====================================================================

    // Dragonflight (60-70)
    _zones.push_back({13644, 2444, 4701.0f, 4679.0f, 55.0f, 0.0f, 60, 70, TEAM_NEUTRAL, "The Waking Shores", false});
    _zones.push_back({13645, 2444, -1695.0f, 2460.0f, 293.0f, 0.0f, 60, 70, TEAM_NEUTRAL, "Ohn'ahran Plains", false});

    // The War Within (70-80)
    _zones.push_back({14771, 2601, 2400.0f, -2800.0f, 180.0f, 0.0f, 70, 80, TEAM_NEUTRAL, "Isle of Dorn", false});
    _zones.push_back({14772, 2601, 1800.0f, -3200.0f, 150.0f, 0.0f, 70, 80, TEAM_NEUTRAL, "The Ringing Deeps", false});

    // ====================================================================
    // CAPITAL CITIES (All Levels)
    // ====================================================================

    // Alliance Capitals
    _zones.push_back({1519, 0, -8833.38f, 628.628f, 94.0066f, 1.06465f, 1, 80, TEAM_ALLIANCE, "Stormwind City", false});
    _zones.push_back({1537, 0, -4918.88f, -970.009f, 501.564f, 5.42347f, 1, 80, TEAM_ALLIANCE, "Ironforge", false});
    _zones.push_back({1657, 1, 9869.91f, 2493.58f, 1315.88f, 2.42346f, 1, 80, TEAM_ALLIANCE, "Darnassus", false});
    _zones.push_back({3557, 530, -3864.92f, -11643.7f, -137.644f, 5.50862f, 1, 80, TEAM_ALLIANCE, "The Exodar", false});

    // Horde Capitals
    _zones.push_back({1637, 1, 1574.0f, -4439.0f, 15.4449f, 1.84061f, 1, 80, TEAM_HORDE, "Orgrimmar", false});
    _zones.push_back({1638, 1, -1278.0f, 71.0f, 128.159f, 2.80623f, 1, 80, TEAM_HORDE, "Thunder Bluff", false});
    _zones.push_back({1497, 0, 1633.75f, 240.167f, -43.1034f, 6.26128f, 1, 80, TEAM_HORDE, "Undercity", false});
    _zones.push_back({3487, 530, 9738.28f, -7454.19f, 13.5605f, 0.043914f, 1, 80, TEAM_HORDE, "Silvermoon City", false});

    // Neutral Capitals
    _zones.push_back({4395, 571, 5804.15f, 624.771f, 647.767f, 1.64f, 1, 80, TEAM_NEUTRAL, "Dalaran (Northrend)", false});
    _zones.push_back({6134, 870, 867.965f, 226.952f, 503.159f, 3.93849f, 1, 80, TEAM_NEUTRAL, "Vale of Eternal Blossoms", false});

    TC_LOG_INFO("playerbot", "BotWorldPositioner::BuildDefaultZones() - Built {} default zone placements", _zones.size());
}

void BotWorldPositioner::ValidateZones()
{
    uint32 invalidCount = 0;

    for (auto& zone : _zones)
    {
        // Validate level range
    if (zone.minLevel > zone.maxLevel)
        {
            TC_LOG_ERROR("playerbot", "BotWorldPositioner::ValidateZones() - Invalid level range for zone {}: {} > {}",
                zone.zoneId, zone.minLevel, zone.maxLevel);
            ::std::swap(zone.minLevel, zone.maxLevel);
            ++invalidCount;
        }

        // Validate coordinates (basic sanity check)
    if (::std::abs(zone.x) > 20000.0f || ::std::abs(zone.y) > 20000.0f || ::std::abs(zone.z) > 10000.0f)
        {
            TC_LOG_WARN("playerbot", "BotWorldPositioner::ValidateZones() - Suspicious coordinates for zone {}: ({}, {}, {})",
                zone.zoneId, zone.x, zone.y, zone.z);
        }
    }

    if (invalidCount > 0)
        TC_LOG_WARN("playerbot", "BotWorldPositioner::ValidateZones() - Fixed {} invalid zones", invalidCount);
}

void BotWorldPositioner::BuildZoneCache()
{
    // Build zone ID lookup
    for (size_t i = 0; i < _zones.size(); ++i)
    {
        _zoneById[_zones[i].zoneId] = &_zones[i];
    }

    // Build level-based lookup (every 5 levels)
    for (size_t i = 0; i < _zones.size(); ++i)
    {
        ZonePlacement const& zone = _zones[i];

        // Safety check for level range
        if (zone.minLevel > zone.maxLevel || zone.maxLevel > 100)
        {
            TC_LOG_DEBUG("playerbot", "BotWorldPositioner::BuildZoneCache() - Skipping zone {} with invalid levels {}-{}",
                zone.zoneId, zone.minLevel, zone.maxLevel);
            continue;
        }

        for (uint32 level = zone.minLevel; level <= zone.maxLevel; level += 5)
        {
            _zonesByLevel[level].push_back(&_zones[i]);
        }

        // Also add to exact level for precision
        _zonesByLevel[zone.minLevel].push_back(&_zones[i]);
        _zonesByLevel[zone.maxLevel].push_back(&_zones[i]);
    }

    // Build race-to-starter-zone mapping
    BuildRaceZoneMapping();

    // Build capital city lists
    for (size_t i = 0; i < _zones.size(); ++i)
    {
        ZonePlacement const& zone = _zones[i];
        if ((zone.zoneName.find("City") != ::std::string::npos ||
             zone.zoneName.find("Ironforge") != ::std::string::npos ||
             zone.zoneName.find("Darnassus") != ::std::string::npos ||
             zone.zoneName.find("Thunder Bluff") != ::std::string::npos ||
             zone.zoneName.find("Undercity") != ::std::string::npos) &&
            zone.minLevel == 1 && zone.maxLevel >= 70)
        {
            if (zone.faction == TEAM_ALLIANCE)
                _allianceCapitals.push_back(&_zones[i]);
            else if (zone.faction == TEAM_HORDE)
                _hordeCapitals.push_back(&_zones[i]);
        }
    }

    TC_LOG_DEBUG("playerbot", "BotWorldPositioner::BuildZoneCache() - Built cache: {} zone IDs, {} level brackets, {} capitals",
        _zoneById.size(), _zonesByLevel.size(), _allianceCapitals.size() + _hordeCapitals.size());
}

void BotWorldPositioner::BuildRaceZoneMapping()
{
    // Map races to their starter zones based on zone names
    // This is a simplified mapping - in production, use DBC data
    for (auto const& zone : _zones)
    {
        if (!zone.isStarterZone)
            continue;

        // Alliance races
    if (zone.zoneName.find("Elwynn") != ::std::string::npos)
        {
            _starterZonesByRace[RACE_HUMAN].push_back(&zone);
        }
        else if (zone.zoneName.find("Dun Morogh") != ::std::string::npos)
        {
            _starterZonesByRace[RACE_DWARF].push_back(&zone);
            _starterZonesByRace[RACE_GNOME].push_back(&zone);
        }
        else if (zone.zoneName.find("Teldrassil") != ::std::string::npos)
        {
            _starterZonesByRace[RACE_NIGHTELF].push_back(&zone);
        }
        else if (zone.zoneName.find("Azuremyst") != ::std::string::npos)
        {
            _starterZonesByRace[RACE_DRAENEI].push_back(&zone);
        }
        else if (zone.zoneName.find("Gilneas") != ::std::string::npos)
        {
            _starterZonesByRace[RACE_WORGEN].push_back(&zone);
        }

        // Horde races
        else if (zone.zoneName.find("Durotar") != ::std::string::npos)
        {
            _starterZonesByRace[RACE_ORC].push_back(&zone);
            _starterZonesByRace[RACE_TROLL].push_back(&zone);
        }
        else if (zone.zoneName.find("Tirisfal") != ::std::string::npos)
        {
            _starterZonesByRace[RACE_UNDEAD_PLAYER].push_back(&zone);
        }
        else if (zone.zoneName.find("Mulgore") != ::std::string::npos)
        {
            _starterZonesByRace[RACE_TAUREN].push_back(&zone);
        }
        else if (zone.zoneName.find("Eversong") != ::std::string::npos)
        {
            _starterZonesByRace[RACE_BLOODELF].push_back(&zone);
        }

        // Neutral/Allied races (use capital cities)
        else if (zone.zoneName.find("Stormwind") != ::std::string::npos && zone.isStarterZone)
        {
            _starterZonesByRace[RACE_PANDAREN_ALLIANCE].push_back(&zone);
        }
        else if (zone.zoneName.find("Orgrimmar") != ::std::string::npos && zone.isStarterZone)
        {
            _starterZonesByRace[RACE_PANDAREN_HORDE].push_back(&zone);
            _starterZonesByRace[RACE_GOBLIN].push_back(&zone);
        }
    }

    TC_LOG_DEBUG("playerbot", "BotWorldPositioner::BuildRaceZoneMapping() - Mapped {} races to starter zones",
        _starterZonesByRace.size());
}

// ====================================================================
// ZONE SELECTION (Thread-safe)
// ====================================================================

ZoneChoice BotWorldPositioner::SelectZone(uint32 level, TeamId faction, uint8 race)
{
    if (!IsReady())
    {
        TC_LOG_ERROR("playerbot", "BotWorldPositioner::SelectZone() - Not initialized");
        return ZoneChoice();
    }

    // L1-4: Use starter zones (race-specific)
    if (level <= 4)
    {
        return GetStarterZone(race, faction);
    }

    // L5+: Use level-appropriate zones
    return SelectByLevelRange(level, faction);
}

ZoneChoice BotWorldPositioner::GetStarterZone(uint8 race, TeamId faction)
{
    // Try race-specific starter zone first
    auto raceItr = _starterZonesByRace.find(race);
    if (raceItr != _starterZonesByRace.end() && !raceItr->second.empty())
    {
        // Random selection if multiple options
        size_t idx = rand() % raceItr->second.size();
        return ZoneChoice(raceItr->second[idx], 1.0f);
    }

    // Fallback: Use any starter zone for faction
    ::std::vector<ZonePlacement const*> candidates;
    for (auto const& zone : _zones)
    {
        if (zone.isStarterZone && zone.IsValidForFaction(faction))
        {
            candidates.push_back(&zone);
        }
    }

    if (!candidates.empty())
    {
        size_t idx = rand() % candidates.size();
        return ZoneChoice(candidates[idx], 0.8f);
    }

    // Final fallback: Capital city
    TC_LOG_WARN("playerbot", "BotWorldPositioner::GetStarterZone() - No starter zone for race {}, using capital", race);
    return GetCapitalCity(faction);
}

ZoneChoice BotWorldPositioner::SelectByLevelRange(uint32 level, TeamId faction) const
{
    // Get all zones valid for this level
    ::std::vector<ZonePlacement const*> candidates;

    for (auto const& zone : _zones)
    {
        if (zone.IsValidForLevel(level) && zone.IsValidForFaction(faction) && !zone.isStarterZone)
        {
            candidates.push_back(&zone);
        }
    }

    if (candidates.empty())
    {
        TC_LOG_WARN("playerbot", "BotWorldPositioner::SelectByLevelRange() - No zones for level {}, faction {}",
            level, faction);
        return ZoneChoice();
    }

    // Select weighted by suitability
    return SelectWeighted(candidates);
}

ZoneChoice BotWorldPositioner::SelectWeighted(::std::vector<ZonePlacement const*> const& candidates) const
{
    if (candidates.empty())
        return ZoneChoice();

    // For now, simple random selection
    // Future: Weight by current bot population density
    size_t idx = rand() % candidates.size();
    return ZoneChoice(candidates[idx], 1.0f);
}

float BotWorldPositioner::CalculateSuitability(ZonePlacement const* zone, uint32 level) const
{
    if (!zone->IsValidForLevel(level))
        return 0.0f;

    // Calculate how well the level fits within the zone's range
    uint32 range = zone->maxLevel - zone->minLevel;
    if (range == 0)
        return 1.0f;

    uint32 offset = (level > zone->minLevel) ? (level - zone->minLevel) : 0;
    float fit = 1.0f - (static_cast<float>(offset) / static_cast<float>(range));

    return ::std::max(0.0f, ::std::min(1.0f, fit));
}

ZoneChoice BotWorldPositioner::GetCapitalCity(TeamId faction)
{
    auto const& capitals = (faction == TEAM_ALLIANCE) ? _allianceCapitals : _hordeCapitals;

    if (capitals.empty())
    {
        TC_LOG_ERROR("playerbot", "BotWorldPositioner::GetCapitalCity() - No capitals for faction {}", faction);
        return ZoneChoice();
    }

    size_t idx = rand() % capitals.size();
    return ZoneChoice(capitals[idx], 0.5f);
}

::std::vector<ZonePlacement const*> BotWorldPositioner::GetValidZones(uint32 level, TeamId faction) const
{
    ::std::vector<ZonePlacement const*> result;

    for (auto const& zone : _zones)
    {
        if (zone.IsValidForLevel(level) && zone.IsValidForFaction(faction))
        {
            result.push_back(&zone);
        }
    }

    return result;
}

// ====================================================================
// TELEPORTATION (Main Thread Only)
// ====================================================================

bool BotWorldPositioner::TeleportToZone(Player* bot, ZonePlacement const* placement)
{
    if (!bot || !placement)
    {
        TC_LOG_ERROR("playerbot", "BotWorldPositioner::TeleportToZone() - Invalid parameters");
        return false;
    }

    // Validate coordinates
    if (!ValidateTeleportCoordinates(placement))
    {
        TC_LOG_ERROR("playerbot", "BotWorldPositioner::TeleportToZone() - Invalid coordinates for zone {}",
            placement->zoneId);
        ++_stats.teleportsFailed;
        return false;
    }

    // Teleport using TrinityCore API
    bool success = bot->TeleportTo(placement->mapId, placement->x, placement->y, placement->z, placement->orientation);
    if (success)
    {
        LogPlacement(bot, placement);
        ++_stats.botsPlaced;
        ++_stats.placementsPerZone[placement->zoneId];
    }
    else
    {
        TC_LOG_ERROR("playerbot", "BotWorldPositioner::TeleportToZone() - Teleport failed for bot {} to zone {}",
            bot->GetName(), placement->zoneId);
        ++_stats.teleportsFailed;
    }

    return success;
}

bool BotWorldPositioner::PlaceBot(Player* bot, uint32 level, TeamId faction, uint8 race)
{
    if (!bot)
    {
        TC_LOG_ERROR("playerbot", "BotWorldPositioner::PlaceBot() - Invalid bot pointer");
        return false;
    }

    // Select zone
    ZoneChoice choice = SelectZone(level, faction, race);
    if (!choice.IsValid())
    {
        TC_LOG_ERROR("playerbot", "BotWorldPositioner::PlaceBot() - Failed to select zone for bot {} (L{}, F{}, R{})",
            bot->GetName(), level, faction, race);
        return false;
    }

    // Teleport to zone
    return TeleportToZone(bot, choice.placement);
}

bool BotWorldPositioner::ValidateTeleportCoordinates(ZonePlacement const* placement) const
{
    if (!placement)
        return false;

    // Basic coordinate sanity checks
    if (::std::abs(placement->x) > 20000.0f || ::std::abs(placement->y) > 20000.0f)
        return false;

    if (::std::abs(placement->z) > 10000.0f)
        return false;

    // Could add map existence checks here
    // Map const* map = sMapMgr->FindMap(placement->mapId, 0);
    // if (!map) return false;

    return true;
}

void BotWorldPositioner::LogPlacement(Player* bot, ZonePlacement const* placement)
{
    TC_LOG_DEBUG("playerbot", "BotWorldPositioner::LogPlacement() - Placed bot {} (L{}) in {} (Zone {})",
        bot->GetName(), bot->GetLevel(), placement->zoneName, placement->zoneId);
}

// ====================================================================
// ZONE QUERIES
// ====================================================================

ZonePlacement const* BotWorldPositioner::GetZonePlacement(uint32 zoneId) const
{
    auto itr = _zoneById.find(zoneId);
    return (itr != _zoneById.end()) ? itr->second : nullptr;
}

::std::string BotWorldPositioner::GetZoneName(uint32 zoneId) const
{
    auto const* placement = GetZonePlacement(zoneId);
    return placement ? placement->zoneName : "Unknown Zone";
}

bool BotWorldPositioner::IsZoneValid(uint32 zoneId, uint32 level, TeamId faction) const
{
    auto const* placement = GetZonePlacement(zoneId);
    if (!placement)
        return false;

    return placement->IsValidForLevel(level) && placement->IsValidForFaction(faction);
}

// ====================================================================
// STATISTICS & DEBUGGING
// ====================================================================

void BotWorldPositioner::PrintZoneReport() const
{
    TC_LOG_INFO("playerbot", "====================================================================");
    TC_LOG_INFO("playerbot", "BOT WORLD POSITIONER - ZONE REPORT");
    TC_LOG_INFO("playerbot", "====================================================================");
    TC_LOG_INFO("playerbot", "Total Zones:         {}", _stats.totalZones);
    TC_LOG_INFO("playerbot", "  Starter Zones:     {}", _stats.starterZones);
    TC_LOG_INFO("playerbot", "  Leveling Zones:    {}", _stats.levelingZones);
    TC_LOG_INFO("playerbot", "  Endgame Zones:     {}", _stats.endgameZones);
    TC_LOG_INFO("playerbot", "  Capital Cities:    {}", _stats.capitalCities);
    TC_LOG_INFO("playerbot", "");
    TC_LOG_INFO("playerbot", "Placements:          {}", _stats.botsPlaced);
    TC_LOG_INFO("playerbot", "Failed Teleports:    {}", _stats.teleportsFailed);
    TC_LOG_INFO("playerbot", "");

    if (!_stats.placementsPerZone.empty())
    {
        TC_LOG_INFO("playerbot", "Top 10 Most Popular Zones:");
        ::std::vector<::std::pair<uint32, uint32>> sorted(_stats.placementsPerZone.begin(), _stats.placementsPerZone.end());
        ::std::sort(sorted.begin(), sorted.end(),
            [](auto const& a, auto const& b) { return a.second > b.second; });

        size_t count = 0;
        for (auto const& [zoneId, placements] : sorted)
        {
            if (count++ >= 10)
                break;

            ::std::string zoneName = GetZoneName(zoneId);
            TC_LOG_INFO("playerbot", "  {} ({}): {} bots", zoneName, zoneId, placements);
        }
    }

    TC_LOG_INFO("playerbot", "====================================================================");
}

::std::string BotWorldPositioner::GetZoneSummary() const
{
    ::std::ostringstream oss;
    oss << "BotWorldPositioner: " << _stats.totalZones << " zones ("
        << _stats.starterZones << " starter, "
        << _stats.levelingZones << " leveling, "
        << _stats.endgameZones << " endgame), "
        << _stats.botsPlaced << " bots placed, "
        << _stats.teleportsFailed << " failed";
    return oss.str();
}

} // namespace Playerbot
