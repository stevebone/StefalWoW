/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BGPositionDiscovery.h"
#include "Battleground.h"
#include "BattlegroundMgr.h"
#include "GameObject.h"
#include "Log.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "PathGenerator.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "CellImpl.h"
#include "GridNotifiersImpl.h"

namespace Playerbot::Coordination::Battleground
{

BGPositionDiscovery::BGPositionDiscovery(::Battleground* bg)
    : _battleground(bg)
{
}

bool BGPositionDiscovery::Initialize()
{
    if (!_battleground)
    {
        TC_LOG_ERROR("playerbots.bg.discovery", "BGPositionDiscovery: NULL battleground");
        return false;
    }

    _map = _battleground->FindBgMap();
    if (!_map)
    {
        TC_LOG_WARN("playerbots.bg.discovery",
            "BGPositionDiscovery: BG {} has no map yet, will retry later",
            _battleground->GetInstanceID());
        return false;
    }

    TC_LOG_INFO("playerbots.bg.discovery",
        "BGPositionDiscovery: Initialized for BG {} on map {}",
        _battleground->GetInstanceID(), _map->GetId());

    return true;
}

// ============================================================================
// POSITION VALIDATION
// ============================================================================

PositionValidation BGPositionDiscovery::ValidatePosition(Player* referenceBot, Position const& position,
                                                         float maxCorrectionDistance) const
{
    PositionValidation result;
    result.correctedPosition = position;

    // Step 1: Check if we have a valid map
    if (!_map)
    {
        result.failureReason = "No BG map available";
        return result;
    }

    // Step 2: Correct Z to ground level
    if (!CorrectZToGround(result.correctedPosition))
    {
        result.failureReason = "No valid ground at position";

        // Try to find nearest navmesh point
        auto nearestNav = GetNearestNavmeshPoint(position, maxCorrectionDistance);
        if (nearestNav)
        {
            result.correctedPosition = *nearestNav;
            result.isValid = true;
            result.distanceToOriginal = position.GetExactDist(result.correctedPosition);
            TC_LOG_DEBUG("playerbots.bg.discovery",
                "BGPositionDiscovery: Corrected position ({:.1f},{:.1f},{:.1f}) -> ({:.1f},{:.1f},{:.1f}), dist={:.1f}",
                position.GetPositionX(), position.GetPositionY(), position.GetPositionZ(),
                result.correctedPosition.GetPositionX(), result.correctedPosition.GetPositionY(),
                result.correctedPosition.GetPositionZ(), result.distanceToOriginal);
        }
        else
        {
            return result;  // Completely invalid
        }
    }
    else
    {
        result.isValid = true;
        result.distanceToOriginal = position.GetPositionZ() != result.correctedPosition.GetPositionZ()
            ? std::abs(position.GetPositionZ() - result.correctedPosition.GetPositionZ())
            : 0.0f;
    }

    // Step 3: Check navmesh reachability (if we have a reference bot)
    if (referenceBot && referenceBot->IsInWorld())
    {
        if (IsPathReachable(referenceBot->GetPosition(), result.correctedPosition))
        {
            result.isReachable = true;
        }
        else
        {
            result.failureReason = "Position not reachable via navmesh";

            // Try to find nearest reachable point
            auto nearestNav = GetNearestNavmeshPoint(result.correctedPosition, maxCorrectionDistance);
            if (nearestNav && IsPathReachable(referenceBot->GetPosition(), *nearestNav))
            {
                result.correctedPosition = *nearestNav;
                result.isReachable = true;
                result.distanceToOriginal = position.GetExactDist(result.correctedPosition);
            }
        }
    }
    else
    {
        // No reference bot - just check if on navmesh
        result.isReachable = IsOnNavmesh(result.correctedPosition);
    }

    return result;
}

std::vector<Position> BGPositionDiscovery::ValidatePositions(Player* referenceBot,
                                                             std::vector<Position> const& positions) const
{
    std::vector<Position> validated;
    validated.reserve(positions.size());

    for (auto const& pos : positions)
    {
        auto validation = ValidatePosition(referenceBot, pos);
        if (validation.isValid && (validation.isReachable || !referenceBot))
        {
            validated.push_back(validation.correctedPosition);
        }
    }

    TC_LOG_DEBUG("playerbots.bg.discovery",
        "BGPositionDiscovery: Validated {} of {} positions",
        validated.size(), positions.size());

    return validated;
}

bool BGPositionDiscovery::IsOnNavmesh(Position const& position) const
{
    if (!_map)
        return false;

    // Use map height check as a navmesh indicator
    float height = _map->GetHeight(PhasingHandler::GetEmptyPhaseShift(),
                                   position.GetPositionX(),
                                   position.GetPositionY(),
                                   position.GetPositionZ() + 10.0f,
                                   true, 100.0f);

    return height > INVALID_HEIGHT;
}

std::optional<Position> BGPositionDiscovery::GetNearestNavmeshPoint(Position const& position,
                                                                    float searchRadius) const
{
    if (!_map)
        return std::nullopt;

    // Try spiral search pattern to find valid ground
    constexpr float stepSize = 2.0f;
    const int maxSteps = static_cast<int>(searchRadius / stepSize);

    for (int radius = 1; radius <= maxSteps; ++radius)
    {
        float r = radius * stepSize;
        // Check 8 directions at this radius
        for (int dir = 0; dir < 8; ++dir)
        {
            float angle = dir * M_PI / 4.0f;
            float x = position.GetPositionX() + r * std::cos(angle);
            float y = position.GetPositionY() + r * std::sin(angle);

            float height = _map->GetHeight(PhasingHandler::GetEmptyPhaseShift(),
                                          x, y, position.GetPositionZ() + 20.0f,
                                          true, 50.0f);

            if (height > INVALID_HEIGHT)
            {
                Position result;
                result.Relocate(x, y, height + 0.5f, position.GetOrientation());
                return result;
            }
        }
    }

    return std::nullopt;
}

// ============================================================================
// DYNAMIC GAME OBJECT DISCOVERY
// ============================================================================

std::vector<DiscoveredPOI> BGPositionDiscovery::DiscoverGameObjects(std::vector<uint32> const& entries,
                                                                    std::string const& namePrefix) const
{
    std::vector<DiscoveredPOI> discovered;

    uint32 index = 0;
    for (uint32 entry : entries)
    {
        auto poi = DiscoverGameObject(entry, namePrefix + " " + std::to_string(index++));
        if (poi)
        {
            discovered.push_back(*poi);
        }
    }

    TC_LOG_INFO("playerbots.bg.discovery",
        "BGPositionDiscovery: Discovered {} of {} {} game objects",
        discovered.size(), entries.size(), namePrefix.c_str());

    return discovered;
}

std::optional<DiscoveredPOI> BGPositionDiscovery::DiscoverGameObject(uint32 entry, std::string const& name) const
{
    if (!_map || !_battleground)
        return std::nullopt;

    // Try to find a player on the map to use FindNearestGameObject
    Player* searchPlayer = nullptr;
    ::Battleground::BattlegroundPlayerMap const& players = _battleground->GetPlayers();
    for (auto const& itr : players)
    {
        Player* player = ObjectAccessor::FindPlayer(itr.first);
        if (player && player->IsInWorld() && player->FindMap() == _map)
        {
            searchPlayer = player;
            break;
        }
    }

    GameObject* go = nullptr;

    if (searchPlayer)
    {
        // Use player's FindNearestGameObject with large search radius
        go = searchPlayer->FindNearestGameObject(entry, 500.0f);
    }

    if (!go)
    {
        TC_LOG_DEBUG("playerbots.bg.discovery",
            "BGPositionDiscovery: Could not find game object entry {} ({}) on map {}",
            entry, name.c_str(), _map->GetId());
        return std::nullopt;
    }

    // Create discovered POI
    DiscoveredPOI poi;
    poi.id = entry;
    poi.name = name;
    poi.position.Relocate(go->GetPositionX(), go->GetPositionY(), go->GetPositionZ(), go->GetOrientation());
    poi.gameObjectEntry = entry;
    poi.gameObjectGuid = go->GetGUID();
    poi.isDynamic = true;

    // Validate the position
    if (CorrectZToGround(poi.position))
    {
        poi.isValidated = true;
    }

    TC_LOG_INFO("playerbots.bg.discovery",
        "BGPositionDiscovery: Discovered {} at ({:.1f},{:.1f},{:.1f})",
        name.c_str(), poi.position.GetPositionX(), poi.position.GetPositionY(), poi.position.GetPositionZ());

    return poi;
}

GameObject* BGPositionDiscovery::FindNearestGameObject(uint32 entry, Position const& nearPosition,
                                                       float searchRadius) const
{
    if (!_map || !_battleground)
        return nullptr;

    // Find a player to use for searching
    ::Battleground::BattlegroundPlayerMap const& players = _battleground->GetPlayers();
    for (auto const& itr : players)
    {
        Player* player = ObjectAccessor::FindPlayer(itr.first);
        if (player && player->IsInWorld() && player->FindMap() == _map)
        {
            // Check if player is close to search position
            float dist = player->GetExactDist2d(nearPosition.GetPositionX(), nearPosition.GetPositionY());
            if (dist < searchRadius)
            {
                return player->FindNearestGameObject(entry, searchRadius);
            }
        }
    }

    return nullptr;
}

// ============================================================================
// SPAWN POINT DISCOVERY
// ============================================================================

std::vector<Position> BGPositionDiscovery::GetSpawnPositions(uint32 faction) const
{
    std::vector<Position> spawns;

    if (!_battleground)
        return spawns;

    // Get team start position from WorldSafeLocsEntry
    TeamId teamId = (faction == ALLIANCE) ? TEAM_ALLIANCE : TEAM_HORDE;
    WorldSafeLocsEntry const* safeLoc = _battleground->GetTeamStartPosition(teamId);

    if (safeLoc)
    {
        Position pos;
        pos.Relocate(safeLoc->Loc.GetPositionX(),
                    safeLoc->Loc.GetPositionY(),
                    safeLoc->Loc.GetPositionZ(),
                    safeLoc->Loc.GetOrientation());

        // Validate and add
        if (_map)
        {
            CorrectZToGround(pos);
        }
        spawns.push_back(pos);

        // Generate some spread positions around the main spawn
        for (int i = 0; i < 4; ++i)
        {
            float angle = i * M_PI / 2.0f;
            Position spreadPos;
            spreadPos.Relocate(pos.GetPositionX() + 3.0f * std::cos(angle),
                              pos.GetPositionY() + 3.0f * std::sin(angle),
                              pos.GetPositionZ(),
                              pos.GetOrientation());

            if (_map)
            {
                CorrectZToGround(spreadPos);
            }
            spawns.push_back(spreadPos);
        }

        TC_LOG_DEBUG("playerbots.bg.discovery",
            "BGPositionDiscovery: Got {} spawn positions for faction {}",
            spawns.size(), faction == ALLIANCE ? "Alliance" : "Horde");
    }
    else
    {
        TC_LOG_WARN("playerbots.bg.discovery",
            "BGPositionDiscovery: No WorldSafeLoc for faction {} in BG {}",
            faction == ALLIANCE ? "Alliance" : "Horde", _battleground->GetInstanceID());
    }

    return spawns;
}

std::vector<Position> BGPositionDiscovery::GetGraveyardPositions(uint32 faction) const
{
    // For most BGs, graveyards are the same as spawns
    return GetSpawnPositions(faction);
}

// ============================================================================
// CACHED DISCOVERIES
// ============================================================================

DiscoveredPOI const* BGPositionDiscovery::GetDiscoveredPOI(uint32 poiId) const
{
    auto it = _discoveredPOIs.find(poiId);
    return it != _discoveredPOIs.end() ? &it->second : nullptr;
}

void BGPositionDiscovery::CachePOI(DiscoveredPOI const& poi)
{
    _discoveredPOIs[poi.id] = poi;
}

void BGPositionDiscovery::ClearCache()
{
    _discoveredPOIs.clear();
}

// ============================================================================
// DIAGNOSTICS
// ============================================================================

void BGPositionDiscovery::LogDiscoveryStatus() const
{
    TC_LOG_INFO("playerbots.bg.discovery",
        "BGPositionDiscovery Status: Map={}, Cached POIs={}",
        _map ? _map->GetId() : 0, _discoveredPOIs.size());

    for (auto const& kv : _discoveredPOIs)
    {
        TC_LOG_INFO("playerbots.bg.discovery",
            "  POI {}: {} at ({:.1f},{:.1f},{:.1f}) validated={} dynamic={}",
            kv.first, kv.second.name.c_str(),
            kv.second.position.GetPositionX(), kv.second.position.GetPositionY(), kv.second.position.GetPositionZ(),
            kv.second.isValidated ? "yes" : "no", kv.second.isDynamic ? "yes" : "no");
    }
}

// ============================================================================
// PRIVATE HELPERS
// ============================================================================

bool BGPositionDiscovery::CorrectZToGround(Position& pos) const
{
    if (!_map)
        return false;

    float groundZ = _map->GetHeight(PhasingHandler::GetEmptyPhaseShift(),
                                   pos.GetPositionX(),
                                   pos.GetPositionY(),
                                   pos.GetPositionZ() + 20.0f,  // Search from above
                                   true, 100.0f);

    if (groundZ <= INVALID_HEIGHT)
        return false;

    pos.m_positionZ = groundZ + 0.5f;  // Small offset above ground
    return true;
}

bool BGPositionDiscovery::IsPathReachable(Position const& from, Position const& to) const
{
    if (!_map || !_battleground)
        return false;

    // Find any player on the map to use for pathfinding
    Player* anyPlayer = nullptr;
    ::Battleground::BattlegroundPlayerMap const& players = _battleground->GetPlayers();
    for (auto const& itr : players)
    {
        anyPlayer = ObjectAccessor::FindPlayer(itr.first);
        if (anyPlayer && anyPlayer->IsInWorld() && anyPlayer->FindMap() == _map)
            break;
        anyPlayer = nullptr;
    }

    if (!anyPlayer)
    {
        // No player available - assume reachable if both positions have valid ground
        return true;
    }

    // Create PathGenerator and check path
    PathGenerator path(anyPlayer);
    bool pathResult = path.CalculatePath(from.GetPositionX(), from.GetPositionY(), from.GetPositionZ(),
                                         to.GetPositionX(), to.GetPositionY(), to.GetPositionZ());

    if (!pathResult)
        return false;

    PathType pathType = path.GetPathType();

    // Check if path is valid
    if (pathType & PATHFIND_NOPATH)
        return false;

    if (pathType & PATHFIND_INCOMPLETE)
    {
        // Partial path - check if we get reasonably close
        G3D::Vector3 const& actualEnd = path.GetActualEndPosition();
        float distToTarget = std::sqrt(
            (actualEnd.x - to.GetPositionX()) * (actualEnd.x - to.GetPositionX()) +
            (actualEnd.y - to.GetPositionY()) * (actualEnd.y - to.GetPositionY()) +
            (actualEnd.z - to.GetPositionZ()) * (actualEnd.z - to.GetPositionZ())
        );

        // Consider reachable if we get within 5 yards
        return distToTarget < 5.0f;
    }

    return true;
}

} // namespace Playerbot::Coordination::Battleground
