/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
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

#include "PortalDatabase.h"
#include "CellImpl.h"
#include "DatabaseEnv.h"
#include "DB2Stores.h"
#include "GameObjectData.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Timer.h"
#include <algorithm>

namespace Playerbot
{

// ============================================================================
// PortalInfo Implementation
// ============================================================================

bool PortalInfo::CanPlayerUse(Player const* player) const
{
    if (!player)
        return false;

    // Check faction restrictions
    if (faction == PortalFaction::ALLIANCE && player->GetTeam() != ALLIANCE)
        return false;
    if (faction == PortalFaction::HORDE && player->GetTeam() != HORDE)
        return false;

    // Check level requirements
    uint8 playerLevel = player->GetLevel();
    if (minLevel > 0 && playerLevel < minLevel)
        return false;
    if (maxLevel > 0 && playerLevel > maxLevel)
        return false;

    // Check quest requirement
    if (requiredQuestId != 0)
    {
        if (!player->GetQuestRewardStatus(requiredQuestId))
            return false;
    }

    // Check skill requirement
    if (requiredSkillId != 0)
    {
        if (player->GetSkillValue(static_cast<SkillType>(requiredSkillId)) < requiredSkillValue)
            return false;
    }

    return true;
}

float PortalInfo::GetDistanceFrom(Player const* player) const
{
    if (!player || player->GetMapId() != sourceMapId)
        return std::numeric_limits<float>::max();

    return player->GetDistance(sourcePosition);
}

bool PortalInfo::IsOnSameMap(Player const* player) const
{
    return player && player->GetMapId() == sourceMapId;
}

// ============================================================================
// PortalDatabase Implementation
// ============================================================================

PortalDatabase& PortalDatabase::Instance()
{
    static PortalDatabase instance;
    return instance;
}

bool PortalDatabase::Initialize()
{
    if (_initialized)
        return true;

    uint32 startTime = getMSTime();

    TC_LOG_INFO("module.playerbot.travel", "PortalDatabase: Initializing portal database...");

    // Load portals from database
    uint32 portalCount = LoadPortalsFromDB();
    TC_LOG_INFO("module.playerbot.travel", "PortalDatabase: Loaded {} portal GameObjects", portalCount);

    // Load destinations for all portals
    uint32 destinationCount = LoadDestinations();
    TC_LOG_INFO("module.playerbot.travel", "PortalDatabase: Resolved {} portal destinations", destinationCount);

    // Build spatial indexes
    BuildIndexes();

    // Validate data integrity
    ValidateData();

    // Calculate memory usage
    _memoryUsage = sizeof(PortalDatabase);
    _memoryUsage += _portals.size() * sizeof(PortalInfo);
    _memoryUsage += _portals.capacity() * sizeof(PortalInfo);
    for (auto const& [mapId, indices] : _portalsBySourceMap)
        _memoryUsage += sizeof(mapId) + indices.capacity() * sizeof(size_t);
    for (auto const& [zoneId, indices] : _portalsBySourceZone)
        _memoryUsage += sizeof(zoneId) + indices.capacity() * sizeof(size_t);
    for (auto const& [mapId, indices] : _portalsByDestination)
        _memoryUsage += sizeof(mapId) + indices.capacity() * sizeof(size_t);
    _memoryUsage += _portalByEntry.size() * (sizeof(uint32) + sizeof(size_t));

    _stats.loadTimeMs = GetMSTimeDiffToNow(startTime);
    _initialized = true;

    TC_LOG_INFO("module.playerbot.travel",
        "PortalDatabase: Initialization complete in {} ms. {} portals, {} KB memory",
        _stats.loadTimeMs, _stats.totalPortals, _memoryUsage / 1024);

    return true;
}

bool PortalDatabase::Reload()
{
    std::unique_lock<OrderedSharedMutex<LockOrder::QUEST_MANAGER>> lock(_mutex);

    TC_LOG_INFO("module.playerbot.travel", "PortalDatabase: Reloading portal database...");

    // Clear existing data
    _portals.clear();
    _portalsBySourceMap.clear();
    _portalsBySourceZone.clear();
    _portalsByDestination.clear();
    _portalByEntry.clear();
    _stats = Statistics();
    _initialized = false;

    // Reinitialize
    lock.unlock();
    return Initialize();
}

uint32 PortalDatabase::LoadPortalsFromDB()
{
    uint32 count = 0;

    // Query gameobject + gameobject_template for portal-type objects
    // GAMEOBJECT_TYPE_SPELLCASTER (22) - spell in data0
    // GAMEOBJECT_TYPE_GOOBER (10) - spell in data10
    QueryResult result = WorldDatabase.Query(R"SQL(
        SELECT
            gt.entry,
            gt.name,
            gt.type,
            CASE gt.type
                WHEN 22 THEN gt.Data0   -- SPELLCASTER: spell in data0
                WHEN 10 THEN gt.Data10  -- GOOBER: spell in data10
                ELSE 0
            END AS spell_id,
            go.map,
            go.position_x,
            go.position_y,
            go.position_z,
            go.orientation,
            go.spawntimesecs,
            go.zoneId,
            go.areaId
        FROM gameobject_template gt
        INNER JOIN gameobject go ON gt.entry = go.id
        WHERE gt.type IN (22, 10)
        AND (
            (gt.type = 22 AND gt.Data0 > 0)
            OR (gt.type = 10 AND gt.Data10 > 0)
        )
        ORDER BY gt.entry
    )SQL");

    if (!result)
    {
        TC_LOG_WARN("module.playerbot.travel", "PortalDatabase: No portal GameObjects found in database");
        return 0;
    }

    // Reserve space for efficiency
    _portals.reserve(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();
        std::string name = fields[1].GetString();
        uint32 goType = fields[2].GetUInt32();
        uint32 spellId = fields[3].GetUInt32();
        uint32 mapId = fields[4].GetUInt16();
        float posX = fields[5].GetFloat();
        float posY = fields[6].GetFloat();
        float posZ = fields[7].GetFloat();
        float orientation = fields[8].GetFloat();
        int32 spawnTime = fields[9].GetInt32();
        uint32 zoneId = fields[10].GetUInt16();
        uint32 areaId = fields[11].GetUInt16();

        // Verify spell is a teleport spell
        if (spellId == 0 || !IsTeleportSpell(spellId))
            continue;

        // Check for duplicate entries (same entry can spawn multiple times)
        auto existingIt = _portalByEntry.find(entry);
        if (existingIt != _portalByEntry.end())
        {
            // Skip duplicate spawn, we already have this portal type
            continue;
        }

        PortalInfo portal;
        portal.portalId = static_cast<uint32>(_portals.size());
        portal.gameObjectEntry = entry;
        portal.name = std::move(name);
        portal.teleportSpellId = spellId;

        // Source location — zone/area come directly from the gameobject table,
        // avoiding expensive TerrainMgr::LoadTerrain() calls during startup
        portal.sourceMapId = mapId;
        portal.sourcePosition.Relocate(posX, posY, posZ, orientation);
        portal.sourceZoneId = zoneId;
        portal.sourceAreaId = areaId;

        // Determine faction and type
        DeterminePortalFaction(portal, entry);

        // Portal is active if spawn time > 0 (or = 0 for always-spawned)
        portal.isActive = (spawnTime >= 0);
        portal.requiresInteraction = (goType == GAMEOBJECT_TYPE_GOOBER);

        _portals.push_back(std::move(portal));
        _portalByEntry[entry] = _portals.size() - 1;
        ++count;

    } while (result->NextRow());

    return count;
}

uint32 PortalDatabase::LoadDestinations()
{
    uint32 resolved = 0;

    for (PortalInfo& portal : _portals)
    {
        if (portal.teleportSpellId == 0)
            continue;

        // Try to get destination from SpellMgr
        SpellTargetPosition const* targetPos = sSpellMgr->GetSpellTargetPosition(
            portal.teleportSpellId, EFFECT_0);

        if (targetPos)
        {
            portal.destinationMapId = targetPos->GetMapId();
            portal.destinationPosition.Relocate(
                targetPos->GetPositionX(),
                targetPos->GetPositionY(),
                targetPos->GetPositionZ(),
                targetPos->GetOrientation());

            // Get destination name from map entry
            if (MapEntry const* mapEntry = sMapStore.LookupEntry(portal.destinationMapId))
            {
                portal.destinationName = mapEntry->MapName[DEFAULT_LOCALE];
            }

            // Classify portal type now that we have destination
            ClassifyPortalType(portal);

            ++resolved;
        }
        else
        {
            // Check EFFECT_1 and EFFECT_2 as fallback
            for (uint8 effIdx = 1; effIdx <= 2; ++effIdx)
            {
                targetPos = sSpellMgr->GetSpellTargetPosition(
                    portal.teleportSpellId, static_cast<SpellEffIndex>(effIdx));
                if (targetPos)
                {
                    portal.destinationMapId = targetPos->GetMapId();
                    portal.destinationPosition.Relocate(
                        targetPos->GetPositionX(),
                        targetPos->GetPositionY(),
                        targetPos->GetPositionZ(),
                        targetPos->GetOrientation());
                    portal.spellEffectIndex = effIdx;

                    ClassifyPortalType(portal);
                    ++resolved;
                    break;
                }
            }
        }
    }

    // Update statistics
    _stats.totalPortals = static_cast<uint32>(_portals.size());

    // Count unique destinations
    std::set<uint32> uniqueDestinations;
    for (PortalInfo const& portal : _portals)
    {
        if (portal.destinationMapId != 0)
            uniqueDestinations.insert(portal.destinationMapId);
    }
    _stats.uniqueDestinations = static_cast<uint32>(uniqueDestinations.size());

    return resolved;
}

void PortalDatabase::DeterminePortalFaction(PortalInfo& portal, uint32 /*gameObjectEntry*/)
{
    // Determine faction based on zone/area
    // This is a heuristic - specific zones are faction-specific

    // Alliance capital zones
    static const std::set<uint32> allianceZones = {
        1519,   // Stormwind City
        1537,   // Ironforge
        1657,   // Darnassus
        3557,   // The Exodar
        4395,   // Dalaran (neutral but Alliance-friendly portals)
    };

    // Horde capital zones
    static const std::set<uint32> hordeZones = {
        1637,   // Orgrimmar
        1638,   // Thunder Bluff
        1497,   // Undercity
        3487,   // Silvermoon City
    };

    if (allianceZones.find(portal.sourceZoneId) != allianceZones.end())
    {
        portal.faction = PortalFaction::ALLIANCE;
        ++_stats.alliancePortals;
    }
    else if (hordeZones.find(portal.sourceZoneId) != hordeZones.end())
    {
        portal.faction = PortalFaction::HORDE;
        ++_stats.hordePortals;
    }
    else
    {
        portal.faction = PortalFaction::NEUTRAL;
        ++_stats.neutralPortals;
    }
}

void PortalDatabase::ClassifyPortalType(PortalInfo& portal)
{
    // Check destination map type
    if (MapEntry const* destMap = sMapStore.LookupEntry(portal.destinationMapId))
    {
        // Instance portals
        if (destMap->IsDungeon() || destMap->IsRaid())
        {
            portal.type = PortalType::DUNGEON_PORTAL;
            ++_stats.dungeonPortals;
            return;
        }

        // Battleground portals
        if (destMap->IsBattleground() || destMap->IsBattleArena())
        {
            portal.type = PortalType::BATTLEGROUND_PORTAL;
            return;
        }
    }

    // Expansion-specific portals (Dark Portal, etc.)
    static const std::set<uint32> expansionPortalMaps = {
        530,    // Outland
        571,    // Northrend
        870,    // Pandaria
        1116,   // Draenor
        1220,   // Broken Isles
        1642,   // Zandalar
        1643,   // Kul Tiras
        2222,   // Dragon Isles
        2552,   // Khaz Algar
    };

    if (expansionPortalMaps.find(portal.destinationMapId) != expansionPortalMaps.end() ||
        expansionPortalMaps.find(portal.sourceMapId) != expansionPortalMaps.end())
    {
        portal.type = PortalType::EXPANSION_PORTAL;
        ++_stats.expansionPortals;
        return;
    }

    // Default to static portal
    portal.type = PortalType::STATIC_PORTAL;
    ++_stats.staticPortals;
}

void PortalDatabase::BuildIndexes()
{
    // Clear existing indexes
    _portalsBySourceMap.clear();
    _portalsBySourceZone.clear();
    _portalsByDestination.clear();

    // Build indexes
    for (size_t i = 0; i < _portals.size(); ++i)
    {
        PortalInfo const& portal = _portals[i];

        // Index by source map
        _portalsBySourceMap[portal.sourceMapId].push_back(i);

        // Index by source zone (if known)
        if (portal.sourceZoneId != 0)
            _portalsBySourceZone[portal.sourceZoneId].push_back(i);

        // Index by destination map
        if (portal.destinationMapId != 0)
            _portalsByDestination[portal.destinationMapId].push_back(i);
    }

    // Count maps with portals
    _stats.mapsWithPortals = static_cast<uint32>(_portalsBySourceMap.size());

    TC_LOG_DEBUG("module.playerbot.travel",
        "PortalDatabase: Built indexes - {} source maps, {} zones, {} destinations",
        _portalsBySourceMap.size(), _portalsBySourceZone.size(), _portalsByDestination.size());
}

void PortalDatabase::ValidateData()
{
    uint32 invalidCount = 0;

    for (PortalInfo& portal : _portals)
    {
        // Check for invalid source position
        if (portal.sourcePosition.GetPositionX() == 0.0f &&
            portal.sourcePosition.GetPositionY() == 0.0f &&
            portal.sourcePosition.GetPositionZ() == 0.0f)
        {
            TC_LOG_WARN("module.playerbot.travel",
                "PortalDatabase: Portal '{}' (entry {}) has invalid source position",
                portal.name, portal.gameObjectEntry);
            portal.isActive = false;
            ++invalidCount;
        }

        // Check for missing destination
        if (portal.destinationMapId == 0)
        {
            TC_LOG_DEBUG("module.playerbot.travel",
                "PortalDatabase: Portal '{}' (entry {}) has no destination (spell {} not in spell_target_position)",
                portal.name, portal.gameObjectEntry, portal.teleportSpellId);
            portal.isActive = false;
            ++invalidCount;
        }
    }

    if (invalidCount > 0)
    {
        TC_LOG_WARN("module.playerbot.travel",
            "PortalDatabase: {} portals marked inactive due to validation failures", invalidCount);
    }
}

bool PortalDatabase::IsTeleportSpell(uint32 spellId) const
{
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    // Check for teleport effects
    for (SpellEffectInfo const& effect : spellInfo->GetEffects())
    {
        switch (effect.Effect)
        {
            case SPELL_EFFECT_TELEPORT_UNITS:
            case SPELL_EFFECT_TELEPORT_UNITS_FACE_CASTER:
            case SPELL_EFFECT_TELEPORT_TO_RETURN_POINT:
            case SPELL_EFFECT_TELEPORT_WITH_SPELL_VISUAL_KIT_LOADING_SCREEN:
            case SPELL_EFFECT_TELEPORT_GRAVEYARD:
            case SPELL_EFFECT_TELEPORT_TO_LFG_DUNGEON:
                return true;
            default:
                break;
        }
    }

    // Also check if spell has target position defined
    if (sSpellMgr->GetSpellTargetPosition(spellId, EFFECT_0))
        return true;
    if (sSpellMgr->GetSpellTargetPosition(spellId, EFFECT_1))
        return true;
    if (sSpellMgr->GetSpellTargetPosition(spellId, EFFECT_2))
        return true;

    return false;
}

// ============================================================================
// Query Methods
// ============================================================================

std::vector<PortalInfo const*> PortalDatabase::GetPortalsToMap(
    Player const* player,
    uint32 destinationMapId,
    uint32 maxCount) const
{
    std::shared_lock<OrderedSharedMutex<LockOrder::QUEST_MANAGER>> lock(_mutex);

    std::vector<PortalInfo const*> result;

    if (!player)
        return result;

    uint32 playerMapId = player->GetMapId();

    // Find portals on player's map that go to destination
    auto sourceMapIt = _portalsBySourceMap.find(playerMapId);
    if (sourceMapIt == _portalsBySourceMap.end())
        return result;

    // Collect accessible portals
    for (size_t idx : sourceMapIt->second)
    {
        PortalInfo const& portal = _portals[idx];

        // Check destination
        if (portal.destinationMapId != destinationMapId)
            continue;

        // Check access
        if (!portal.isActive || !portal.CanPlayerUse(player))
            continue;

        result.push_back(&portal);
    }

    // Sort by distance
    std::sort(result.begin(), result.end(),
        [player](PortalInfo const* a, PortalInfo const* b)
        {
            return a->GetDistanceFrom(player) < b->GetDistanceFrom(player);
        });

    // Limit results
    if (result.size() > maxCount)
        result.resize(maxCount);

    return result;
}

std::vector<PortalInfo const*> PortalDatabase::GetPortalsOnMap(uint32 mapId) const
{
    std::shared_lock<OrderedSharedMutex<LockOrder::QUEST_MANAGER>> lock(_mutex);

    std::vector<PortalInfo const*> result;

    auto it = _portalsBySourceMap.find(mapId);
    if (it == _portalsBySourceMap.end())
        return result;

    result.reserve(it->second.size());
    for (size_t idx : it->second)
    {
        if (_portals[idx].isActive)
            result.push_back(&_portals[idx]);
    }

    return result;
}

std::vector<PortalInfo const*> PortalDatabase::GetPortalsInZone(uint32 zoneId) const
{
    std::shared_lock<OrderedSharedMutex<LockOrder::QUEST_MANAGER>> lock(_mutex);

    std::vector<PortalInfo const*> result;

    auto it = _portalsBySourceZone.find(zoneId);
    if (it == _portalsBySourceZone.end())
        return result;

    result.reserve(it->second.size());
    for (size_t idx : it->second)
    {
        if (_portals[idx].isActive)
            result.push_back(&_portals[idx]);
    }

    return result;
}

PortalInfo const* PortalDatabase::GetNearestPortalToDestination(
    Player const* player,
    uint32 destinationMapId) const
{
    auto portals = GetPortalsToMap(player, destinationMapId, 1);
    return portals.empty() ? nullptr : portals[0];
}

PortalInfo const* PortalDatabase::GetBestPortalForDestination(
    Player const* player,
    uint32 destinationMapId,
    Position const& destinationPos) const
{
    std::shared_lock<OrderedSharedMutex<LockOrder::QUEST_MANAGER>> lock(_mutex);

    if (!player)
        return nullptr;

    uint32 playerMapId = player->GetMapId();
    PortalInfo const* bestPortal = nullptr;
    float bestScore = std::numeric_limits<float>::max();

    auto sourceMapIt = _portalsBySourceMap.find(playerMapId);
    if (sourceMapIt == _portalsBySourceMap.end())
        return nullptr;

    for (size_t idx : sourceMapIt->second)
    {
        PortalInfo const& portal = _portals[idx];

        if (portal.destinationMapId != destinationMapId)
            continue;

        if (!portal.isActive || !portal.CanPlayerUse(player))
            continue;

        // Calculate score: distance to portal + distance from portal destination to final destination
        float distToPortal = portal.GetDistanceFrom(player);
        float distFromPortalDest = portal.destinationPosition.GetExactDist(&destinationPos);

        // Weight the portal destination distance more heavily (we want to arrive close to target)
        float score = distToPortal + (distFromPortalDest * 2.0f);

        if (score < bestScore)
        {
            bestScore = score;
            bestPortal = &portal;
        }
    }

    return bestPortal;
}

PortalInfo const* PortalDatabase::GetPortalByEntry(uint32 gameObjectEntry) const
{
    std::shared_lock<OrderedSharedMutex<LockOrder::QUEST_MANAGER>> lock(_mutex);

    auto it = _portalByEntry.find(gameObjectEntry);
    if (it == _portalByEntry.end())
        return nullptr;

    return &_portals[it->second];
}

PortalInfo const* PortalDatabase::GetPortalById(uint32 portalId) const
{
    std::shared_lock<OrderedSharedMutex<LockOrder::QUEST_MANAGER>> lock(_mutex);

    if (portalId >= _portals.size())
        return nullptr;

    return &_portals[portalId];
}

// ============================================================================
// Dynamic Portal Detection
// ============================================================================

std::vector<GameObject*> PortalDatabase::FindNearbyPortalObjects(
    Player const* player,
    float searchRadius) const
{
    std::vector<GameObject*> result;

    if (!player || !player->GetMap())
        return result;

    // Search for nearby GameObjects
    std::list<GameObject*> nearbyObjects;
    Trinity::GameObjectInRangeCheck check(player->GetPositionX(), player->GetPositionY(),
        player->GetPositionZ(), searchRadius);
    Trinity::GameObjectListSearcher<Trinity::GameObjectInRangeCheck> searcher(player, nearbyObjects, check);
    Cell::VisitGridObjects(player, searcher, searchRadius);

    for (GameObject* go : nearbyObjects)
    {
        if (!go || !go->isSpawned())
            continue;

        GameObjectTemplate const* goTemplate = go->GetGOInfo();
        if (!goTemplate)
            continue;

        // Check for portal types
        bool isPortal = false;
        uint32 spellId = 0;

        switch (goTemplate->type)
        {
            case GAMEOBJECT_TYPE_SPELLCASTER:
                spellId = goTemplate->spellCaster.spell;
                isPortal = IsTeleportSpell(spellId);
                break;
            case GAMEOBJECT_TYPE_GOOBER:
                spellId = goTemplate->goober.spell;
                isPortal = IsTeleportSpell(spellId);
                break;
            default:
                break;
        }

        if (isPortal)
            result.push_back(go);
    }

    return result;
}

std::optional<WorldLocation> PortalDatabase::GetPortalDestination(uint32 spellId, uint8 effIndex) const
{
    SpellTargetPosition const* targetPos = sSpellMgr->GetSpellTargetPosition(
        spellId, static_cast<SpellEffIndex>(effIndex));

    if (!targetPos)
        return std::nullopt;

    return WorldLocation(
        targetPos->GetMapId(),
        targetPos->GetPositionX(),
        targetPos->GetPositionY(),
        targetPos->GetPositionZ(),
        targetPos->GetOrientation());
}

// ============================================================================
// Statistics
// ============================================================================

size_t PortalDatabase::GetPortalCount() const
{
    std::shared_lock<OrderedSharedMutex<LockOrder::QUEST_MANAGER>> lock(_mutex);
    return _portals.size();
}

size_t PortalDatabase::GetMemoryUsage() const
{
    std::shared_lock<OrderedSharedMutex<LockOrder::QUEST_MANAGER>> lock(_mutex);
    return _memoryUsage;
}

PortalDatabase::Statistics PortalDatabase::GetStatistics() const
{
    std::shared_lock<OrderedSharedMutex<LockOrder::QUEST_MANAGER>> lock(_mutex);
    return _stats;
}

} // namespace Playerbot
