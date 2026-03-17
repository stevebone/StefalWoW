/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BGSpatialQueryCache.h"
#include "Battleground.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "SpellAuraEffects.h"
#include "Timer.h"
#include "Log.h"
#include <algorithm>
#include <cmath>

namespace Playerbot
{

// ============================================================================
// CONSTRUCTOR / LIFECYCLE
// ============================================================================

BGSpatialQueryCache::BGSpatialQueryCache(Battleground* bg, uint32 faction)
    : _battleground(bg)
    , _faction(faction)
{
}

void BGSpatialQueryCache::Initialize()
{
    // Pre-allocate for typical BG size
    _playerSnapshots.reserve(MAX_PLAYERS_PER_BG);
    _friendlyPlayers.reserve(MAX_PLAYERS_PER_BG / 2);
    _enemyPlayers.reserve(MAX_PLAYERS_PER_BG / 2);

    // Initial update
    _lastUpdateTime = getMSTime();
    _lastFlagScanTime = _lastUpdateTime;

    TC_LOG_DEBUG("playerbots.bg.spatial",
        "BGSpatialQueryCache: Initialized for BG {} faction {}",
        _battleground ? _battleground->GetInstanceID() : 0, _faction);
}

void BGSpatialQueryCache::Clear()
{
    _playerSnapshots.clear();
    _spatialCells.clear();
    _friendlyPlayers.clear();
    _enemyPlayers.clear();
    _friendlyFC = ObjectGuid::Empty;
    _enemyFC = ObjectGuid::Empty;
}

// ============================================================================
// UPDATE
// ============================================================================

void BGSpatialQueryCache::Update(uint32 diff)
{
    if (!_battleground)
        return;

    _timeSinceUpdate += diff;
    _timeSinceFlagScan += diff;

    // Update player snapshots at configured interval
    if (_timeSinceUpdate >= CACHE_UPDATE_INTERVAL_MS)
    {
        _timeSinceUpdate = 0;
        _lastUpdateTime = getMSTime();

        // Clear transient data
        _friendlyPlayers.clear();
        _enemyPlayers.clear();

        // Track which GUIDs are still in BG
        std::unordered_set<ObjectGuid> activeGuids;

        // Update snapshots from BG player list
        for (auto const& itr : _battleground->GetPlayers())
        {
            Player* player = ObjectAccessor::FindPlayer(itr.first);
            if (!player || !player->IsInWorld())
                continue;

            ObjectGuid guid = player->GetGUID();
            activeGuids.insert(guid);

            // Update or create snapshot
            BGPlayerSnapshot& snapshot = _playerSnapshots[guid];
            UpdatePlayerSnapshot(player, snapshot);

            // Categorize by faction
            if (player->GetBGTeam() == _faction)
                _friendlyPlayers.push_back(guid);
            else
                _enemyPlayers.push_back(guid);
        }

        // Remove snapshots for players who left BG
        for (auto it = _playerSnapshots.begin(); it != _playerSnapshots.end();)
        {
            if (activeGuids.find(it->first) == activeGuids.end())
                it = _playerSnapshots.erase(it);
            else
                ++it;
        }

        // Rebuild spatial cells with new positions
        RebuildSpatialCells();
    }

    // Update flag carriers at configured interval (more frequent)
    if (_timeSinceFlagScan >= FLAG_SCAN_INTERVAL_MS)
    {
        _timeSinceFlagScan = 0;
        _lastFlagScanTime = getMSTime();
        UpdateFlagCarriers();
    }
}

void BGSpatialQueryCache::UpdatePlayerSnapshot(Player* player, BGPlayerSnapshot& snapshot)
{
    if (!player)
        return;

    // Identity
    snapshot.guid = player->GetGUID();
    snapshot.faction = player->GetFaction();
    snapshot.bgTeam = player->GetBGTeam();

    // Position
    snapshot.position = player->GetPosition();
    snapshot.orientation = player->GetOrientation();

    // State
    snapshot.health = player->GetHealth();
    snapshot.maxHealth = player->GetMaxHealth();
    snapshot.power = player->GetPower(player->GetPowerType());
    snapshot.maxPower = player->GetMaxPower(player->GetPowerType());
    snapshot.powerType = static_cast<uint8>(player->GetPowerType());
    snapshot.isAlive = player->IsAlive();
    snapshot.isInCombat = player->IsInCombat();
    snapshot.isMoving = player->isMoving();
    snapshot.isMounted = player->IsMounted();
    snapshot.isStealthed = player->HasStealthAura();

    // Combat info
    if (Unit* target = player->GetVictim())
        snapshot.targetGuid = target->GetGUID();
    else
        snapshot.targetGuid = ObjectGuid::Empty;

    // Count attackers using threat manager sorted threat list
    snapshot.attackersCount = player->GetThreatManager().GetThreatListSize();

    // Class/Role info
    snapshot.classId = player->GetClass();
    // Determine healer/tank from class and spec
    switch (snapshot.classId)
    {
        case CLASS_PRIEST:
        case CLASS_DRUID:
        case CLASS_PALADIN:
        case CLASS_SHAMAN:
        case CLASS_MONK:
        case CLASS_EVOKER:
            snapshot.isHealer = true;  // Simplified - could check spec
            break;
        default:
            snapshot.isHealer = false;
            break;
    }
    switch (snapshot.classId)
    {
        case CLASS_WARRIOR:
        case CLASS_PALADIN:
        case CLASS_DEATH_KNIGHT:
        case CLASS_MONK:
        case CLASS_DRUID:
        case CLASS_DEMON_HUNTER:
            snapshot.isTank = true;  // Simplified - could check spec
            break;
        default:
            snapshot.isTank = false;
            break;
    }

    // BG-specific flags
    snapshot.hasFlag = player->HasAura(ALLIANCE_FLAG_AURA) || player->HasAura(HORDE_FLAG_AURA);
    if (player->HasAura(ALLIANCE_FLAG_AURA))
        snapshot.flagAuraId = ALLIANCE_FLAG_AURA;
    else if (player->HasAura(HORDE_FLAG_AURA))
        snapshot.flagAuraId = HORDE_FLAG_AURA;
    else
        snapshot.flagAuraId = 0;

    snapshot.hasOrb = player->HasAura(ORB_AURA_PURPLE) || player->HasAura(ORB_AURA_ORANGE) ||
                      player->HasAura(ORB_AURA_GREEN) || player->HasAura(ORB_AURA_BLUE);

    // Timestamp
    snapshot.updateTime = _lastUpdateTime;
}

void BGSpatialQueryCache::UpdateFlagCarriers()
{
    _friendlyFC = ObjectGuid::Empty;
    _enemyFC = ObjectGuid::Empty;

    // Determine which flag aura friendly/enemy FC would have
    // Friendly FC carries ENEMY flag, Enemy FC carries OUR flag
    uint32 friendlyFCFlagAura = (_faction == ALLIANCE) ? ALLIANCE_FLAG_AURA : HORDE_FLAG_AURA;
    uint32 enemyFCFlagAura = (_faction == ALLIANCE) ? HORDE_FLAG_AURA : ALLIANCE_FLAG_AURA;

    for (auto const& [guid, snapshot] : _playerSnapshots)
    {
        if (!snapshot.hasFlag)
            continue;

        // Check if friendly or enemy based on team
        if (snapshot.bgTeam == _faction)
        {
            // Friendly player with flag = our FC
            if (snapshot.flagAuraId == friendlyFCFlagAura)
                _friendlyFC = guid;
        }
        else
        {
            // Enemy player with flag = enemy FC
            if (snapshot.flagAuraId == enemyFCFlagAura)
                _enemyFC = guid;
        }
    }

    _metrics.flagCarrierQueries.fetch_add(1, std::memory_order_relaxed);
}

void BGSpatialQueryCache::RebuildSpatialCells()
{
    _spatialCells.clear();

    for (auto const& [guid, snapshot] : _playerSnapshots)
    {
        if (!snapshot.isAlive)
            continue;  // Don't index dead players

        BGCellKey cellKey = BGCellKey::FromPosition(snapshot.position);
        _spatialCells[cellKey].Add(guid);
    }
}

// ============================================================================
// FLAG CARRIER QUERIES (O(1))
// ============================================================================

ObjectGuid BGSpatialQueryCache::GetFriendlyFlagCarrier() const
{
    _metrics.cacheHits.fetch_add(1, std::memory_order_relaxed);
    _metrics.totalQueries.fetch_add(1, std::memory_order_relaxed);
    return _friendlyFC;
}

ObjectGuid BGSpatialQueryCache::GetEnemyFlagCarrier() const
{
    _metrics.cacheHits.fetch_add(1, std::memory_order_relaxed);
    _metrics.totalQueries.fetch_add(1, std::memory_order_relaxed);
    return _enemyFC;
}

bool BGSpatialQueryCache::GetFriendlyFCPosition(Position& outPosition) const
{
    if (_friendlyFC.IsEmpty())
        return false;

    auto it = _playerSnapshots.find(_friendlyFC);
    if (it == _playerSnapshots.end())
        return false;

    outPosition = it->second.position;
    return true;
}

bool BGSpatialQueryCache::GetEnemyFCPosition(Position& outPosition) const
{
    if (_enemyFC.IsEmpty())
        return false;

    auto it = _playerSnapshots.find(_enemyFC);
    if (it == _playerSnapshots.end())
        return false;

    outPosition = it->second.position;
    return true;
}

BGPlayerSnapshot const* BGSpatialQueryCache::GetFriendlyFCSnapshot() const
{
    if (_friendlyFC.IsEmpty())
        return nullptr;

    auto it = _playerSnapshots.find(_friendlyFC);
    return it != _playerSnapshots.end() ? &it->second : nullptr;
}

BGPlayerSnapshot const* BGSpatialQueryCache::GetEnemyFCSnapshot() const
{
    if (_enemyFC.IsEmpty())
        return nullptr;

    auto it = _playerSnapshots.find(_enemyFC);
    return it != _playerSnapshots.end() ? &it->second : nullptr;
}

// ============================================================================
// PLAYER SNAPSHOT QUERIES
// ============================================================================

BGPlayerSnapshot const* BGSpatialQueryCache::GetPlayerSnapshot(ObjectGuid guid) const
{
    auto it = _playerSnapshots.find(guid);
    return it != _playerSnapshots.end() ? &it->second : nullptr;
}

std::vector<BGPlayerSnapshot const*> BGSpatialQueryCache::GetFriendlySnapshots() const
{
    std::vector<BGPlayerSnapshot const*> result;
    result.reserve(_friendlyPlayers.size());

    for (ObjectGuid guid : _friendlyPlayers)
    {
        auto it = _playerSnapshots.find(guid);
        if (it != _playerSnapshots.end())
            result.push_back(&it->second);
    }

    return result;
}

std::vector<BGPlayerSnapshot const*> BGSpatialQueryCache::GetEnemySnapshots() const
{
    std::vector<BGPlayerSnapshot const*> result;
    result.reserve(_enemyPlayers.size());

    for (ObjectGuid guid : _enemyPlayers)
    {
        auto it = _playerSnapshots.find(guid);
        if (it != _playerSnapshots.end())
            result.push_back(&it->second);
    }

    return result;
}

// ============================================================================
// SPATIAL QUERIES
// ============================================================================

std::vector<BGCellKey> BGSpatialQueryCache::GetCellsInRadius(Position const& center, float radius) const
{
    std::vector<BGCellKey> cells;

    // Calculate cell range
    int32 minCellX = static_cast<int32>(std::floor((center.GetPositionX() - radius) / BGSpatialCell::CELL_SIZE));
    int32 maxCellX = static_cast<int32>(std::floor((center.GetPositionX() + radius) / BGSpatialCell::CELL_SIZE));
    int32 minCellY = static_cast<int32>(std::floor((center.GetPositionY() - radius) / BGSpatialCell::CELL_SIZE));
    int32 maxCellY = static_cast<int32>(std::floor((center.GetPositionY() + radius) / BGSpatialCell::CELL_SIZE));

    // Only include cells that actually overlap with the circle
    for (int32 x = minCellX; x <= maxCellX; ++x)
    {
        for (int32 y = minCellY; y <= maxCellY; ++y)
        {
            BGCellKey key(x, y);
            if (CellOverlapsCircle(key, center, radius))
                cells.push_back(key);
        }
    }

    return cells;
}

bool BGSpatialQueryCache::CellOverlapsCircle(BGCellKey const& cellKey, Position const& center, float radius) const
{
    // Cell bounds
    float cellMinX = cellKey.x * BGSpatialCell::CELL_SIZE;
    float cellMaxX = cellMinX + BGSpatialCell::CELL_SIZE;
    float cellMinY = cellKey.y * BGSpatialCell::CELL_SIZE;
    float cellMaxY = cellMinY + BGSpatialCell::CELL_SIZE;

    // Find closest point on cell to circle center
    float closestX = std::clamp(center.GetPositionX(), cellMinX, cellMaxX);
    float closestY = std::clamp(center.GetPositionY(), cellMinY, cellMaxY);

    // Check if closest point is within radius
    float dx = center.GetPositionX() - closestX;
    float dy = center.GetPositionY() - closestY;
    return (dx * dx + dy * dy) <= (radius * radius);
}

std::vector<BGPlayerSnapshot const*> BGSpatialQueryCache::QueryNearbyEnemies(
    Position const& position, float radius, uint32 callerFaction) const
{
    auto startTime = std::chrono::high_resolution_clock::now();

    std::vector<BGPlayerSnapshot const*> result;
    float radiusSq = radius * radius;

    // Get overlapping cells
    std::vector<BGCellKey> cells = GetCellsInRadius(position, radius);

    for (BGCellKey const& cellKey : cells)
    {
        auto cellIt = _spatialCells.find(cellKey);
        if (cellIt == _spatialCells.end())
            continue;

        for (ObjectGuid guid : cellIt->second.players)
        {
            auto it = _playerSnapshots.find(guid);
            if (it == _playerSnapshots.end())
                continue;

            BGPlayerSnapshot const& snapshot = it->second;

            // Filter: enemies only, alive, within radius
            if (snapshot.bgTeam == callerFaction)
                continue;  // Not enemy (same faction as caller)
            if (!snapshot.isAlive)
                continue;

            float distSq = snapshot.GetDistance2D(position);
            distSq *= distSq;  // GetDistance2D returns distance, need squared
            if (distSq <= radiusSq)
                result.push_back(&snapshot);
        }
    }

    // Update metrics
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime);
    _metrics.totalQueryTimeNs.fetch_add(duration.count(), std::memory_order_relaxed);
    _metrics.nearbyPlayerQueries.fetch_add(1, std::memory_order_relaxed);
    _metrics.totalQueries.fetch_add(1, std::memory_order_relaxed);

    return result;
}

std::vector<BGPlayerSnapshot const*> BGSpatialQueryCache::QueryNearbyAllies(
    Position const& position, float radius, uint32 callerFaction) const
{
    auto startTime = std::chrono::high_resolution_clock::now();

    std::vector<BGPlayerSnapshot const*> result;
    float radiusSq = radius * radius;

    std::vector<BGCellKey> cells = GetCellsInRadius(position, radius);

    for (BGCellKey const& cellKey : cells)
    {
        auto cellIt = _spatialCells.find(cellKey);
        if (cellIt == _spatialCells.end())
            continue;

        for (ObjectGuid guid : cellIt->second.players)
        {
            auto it = _playerSnapshots.find(guid);
            if (it == _playerSnapshots.end())
                continue;

            BGPlayerSnapshot const& snapshot = it->second;

            // Filter: allies only, alive, within radius
            if (snapshot.bgTeam != callerFaction)
                continue;  // Not ally (different faction from caller)
            if (!snapshot.isAlive)
                continue;

            float distSq = snapshot.GetDistance2D(position);
            distSq *= distSq;
            if (distSq <= radiusSq)
                result.push_back(&snapshot);
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime);
    _metrics.totalQueryTimeNs.fetch_add(duration.count(), std::memory_order_relaxed);
    _metrics.nearbyPlayerQueries.fetch_add(1, std::memory_order_relaxed);
    _metrics.totalQueries.fetch_add(1, std::memory_order_relaxed);

    return result;
}

std::vector<BGPlayerSnapshot const*> BGSpatialQueryCache::QueryNearbyPlayers(
    Position const& position, float radius, ObjectGuid excludeGuid) const
{
    std::vector<BGPlayerSnapshot const*> result;
    float radiusSq = radius * radius;

    std::vector<BGCellKey> cells = GetCellsInRadius(position, radius);

    for (BGCellKey const& cellKey : cells)
    {
        auto cellIt = _spatialCells.find(cellKey);
        if (cellIt == _spatialCells.end())
            continue;

        for (ObjectGuid guid : cellIt->second.players)
        {
            if (guid == excludeGuid)
                continue;

            auto it = _playerSnapshots.find(guid);
            if (it == _playerSnapshots.end())
                continue;

            BGPlayerSnapshot const& snapshot = it->second;
            if (!snapshot.isAlive)
                continue;

            float distSq = snapshot.GetDistance2D(position);
            distSq *= distSq;
            if (distSq <= radiusSq)
                result.push_back(&snapshot);
        }
    }

    _metrics.nearbyPlayerQueries.fetch_add(1, std::memory_order_relaxed);
    _metrics.totalQueries.fetch_add(1, std::memory_order_relaxed);

    return result;
}

BGPlayerSnapshot const* BGSpatialQueryCache::GetNearestEnemy(
    Position const& position, float maxRadius, uint32 callerFaction,
    ObjectGuid excludeGuid, float* outDistance) const
{
    auto startTime = std::chrono::high_resolution_clock::now();

    BGPlayerSnapshot const* nearest = nullptr;
    float nearestDistSq = maxRadius * maxRadius;

    // Sort cells by distance from center for early exit optimization
    BGCellKey centerCell = BGCellKey::FromPosition(position);

    // Search in expanding rings from center cell
    // Ring 0: center cell only
    // Ring 1: 8 surrounding cells
    // etc.
    int32 maxRing = static_cast<int32>(std::ceil(maxRadius / BGSpatialCell::CELL_SIZE));

    for (int32 ring = 0; ring <= maxRing; ++ring)
    {
        // Minimum distance to any point in this ring
        float ringMinDist = (ring > 0) ? (ring - 1) * BGSpatialCell::CELL_SIZE : 0.0f;

        // If we already found something closer than anything in this ring, stop
        if (nearest && ringMinDist * ringMinDist >= nearestDistSq)
            break;

        // Check all cells in this ring
        for (int32 dx = -ring; dx <= ring; ++dx)
        {
            for (int32 dy = -ring; dy <= ring; ++dy)
            {
                // Only process cells on the ring border (not interior, already processed)
                if (ring > 0 && std::abs(dx) < ring && std::abs(dy) < ring)
                    continue;

                BGCellKey cellKey(centerCell.x + dx, centerCell.y + dy);
                auto cellIt = _spatialCells.find(cellKey);
                if (cellIt == _spatialCells.end())
                    continue;

                for (ObjectGuid guid : cellIt->second.players)
                {
                    if (guid == excludeGuid)
                        continue;

                    auto it = _playerSnapshots.find(guid);
                    if (it == _playerSnapshots.end())
                        continue;

                    BGPlayerSnapshot const& snapshot = it->second;

                    // Filter: enemies only, alive
                    if (snapshot.bgTeam == callerFaction)
                        continue;  // Same faction as caller = not enemy
                    if (!snapshot.isAlive)
                        continue;

                    float dist = snapshot.GetDistance(position);
                    float distSq = dist * dist;
                    if (distSq < nearestDistSq)
                    {
                        nearestDistSq = distSq;
                        nearest = &snapshot;
                        if (outDistance)
                            *outDistance = dist;
                    }
                }
            }
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime);
    _metrics.totalQueryTimeNs.fetch_add(duration.count(), std::memory_order_relaxed);
    _metrics.nearestEnemyQueries.fetch_add(1, std::memory_order_relaxed);
    _metrics.totalQueries.fetch_add(1, std::memory_order_relaxed);

    return nearest;
}

BGPlayerSnapshot const* BGSpatialQueryCache::GetNearestAlly(
    Position const& position, float maxRadius, uint32 callerFaction,
    ObjectGuid excludeGuid, float* outDistance) const
{
    BGPlayerSnapshot const* nearest = nullptr;
    float nearestDistSq = maxRadius * maxRadius;

    BGCellKey centerCell = BGCellKey::FromPosition(position);
    int32 maxRing = static_cast<int32>(std::ceil(maxRadius / BGSpatialCell::CELL_SIZE));

    for (int32 ring = 0; ring <= maxRing; ++ring)
    {
        float ringMinDist = (ring > 0) ? (ring - 1) * BGSpatialCell::CELL_SIZE : 0.0f;
        if (nearest && ringMinDist * ringMinDist >= nearestDistSq)
            break;

        for (int32 dx = -ring; dx <= ring; ++dx)
        {
            for (int32 dy = -ring; dy <= ring; ++dy)
            {
                if (ring > 0 && std::abs(dx) < ring && std::abs(dy) < ring)
                    continue;

                BGCellKey cellKey(centerCell.x + dx, centerCell.y + dy);
                auto cellIt = _spatialCells.find(cellKey);
                if (cellIt == _spatialCells.end())
                    continue;

                for (ObjectGuid guid : cellIt->second.players)
                {
                    if (guid == excludeGuid)
                        continue;

                    auto it = _playerSnapshots.find(guid);
                    if (it == _playerSnapshots.end())
                        continue;

                    BGPlayerSnapshot const& snapshot = it->second;

                    if (snapshot.bgTeam != callerFaction)
                        continue;  // Different faction from caller = not ally
                    if (!snapshot.isAlive)
                        continue;

                    float dist = snapshot.GetDistance(position);
                    float distSq = dist * dist;
                    if (distSq < nearestDistSq)
                    {
                        nearestDistSq = distSq;
                        nearest = &snapshot;
                        if (outDistance)
                            *outDistance = dist;
                    }
                }
            }
        }
    }

    _metrics.totalQueries.fetch_add(1, std::memory_order_relaxed);
    return nearest;
}

// ============================================================================
// SPECIALIZED QUERIES
// ============================================================================

std::vector<BGPlayerSnapshot const*> BGSpatialQueryCache::QueryNearbyEnemyHealers(
    Position const& position, float radius, uint32 callerFaction) const
{
    std::vector<BGPlayerSnapshot const*> result;

    auto enemies = QueryNearbyEnemies(position, radius, callerFaction);
    for (auto const* snapshot : enemies)
    {
        if (snapshot->isHealer)
            result.push_back(snapshot);
    }

    return result;
}

std::vector<BGPlayerSnapshot const*> BGSpatialQueryCache::QueryNearbyFriendlyHealers(
    Position const& position, float radius, uint32 callerFaction) const
{
    std::vector<BGPlayerSnapshot const*> result;

    auto allies = QueryNearbyAllies(position, radius, callerFaction);
    for (auto const* snapshot : allies)
    {
        if (snapshot->isHealer)
            result.push_back(snapshot);
    }

    return result;
}

std::vector<BGPlayerSnapshot const*> BGSpatialQueryCache::GetPlayersAttacking(ObjectGuid targetGuid) const
{
    std::vector<BGPlayerSnapshot const*> result;

    for (auto const& [guid, snapshot] : _playerSnapshots)
    {
        if (snapshot.targetGuid == targetGuid && snapshot.isAlive)
            result.push_back(&snapshot);
    }

    return result;
}

uint32 BGSpatialQueryCache::CountEnemiesInRadius(Position const& position, float radius, uint32 callerFaction) const
{
    uint32 count = 0;
    float radiusSq = radius * radius;

    std::vector<BGCellKey> cells = GetCellsInRadius(position, radius);

    for (BGCellKey const& cellKey : cells)
    {
        auto cellIt = _spatialCells.find(cellKey);
        if (cellIt == _spatialCells.end())
            continue;

        for (ObjectGuid guid : cellIt->second.players)
        {
            auto it = _playerSnapshots.find(guid);
            if (it == _playerSnapshots.end())
                continue;

            BGPlayerSnapshot const& snapshot = it->second;
            if (snapshot.bgTeam == callerFaction)
                continue;  // Same faction as caller = not enemy
            if (!snapshot.isAlive)
                continue;

            float distSq = snapshot.GetDistance2D(position);
            distSq *= distSq;
            if (distSq <= radiusSq)
                ++count;
        }
    }

    return count;
}

uint32 BGSpatialQueryCache::CountAlliesInRadius(Position const& position, float radius, uint32 callerFaction) const
{
    uint32 count = 0;
    float radiusSq = radius * radius;

    std::vector<BGCellKey> cells = GetCellsInRadius(position, radius);

    for (BGCellKey const& cellKey : cells)
    {
        auto cellIt = _spatialCells.find(cellKey);
        if (cellIt == _spatialCells.end())
            continue;

        for (ObjectGuid guid : cellIt->second.players)
        {
            auto it = _playerSnapshots.find(guid);
            if (it == _playerSnapshots.end())
                continue;

            BGPlayerSnapshot const& snapshot = it->second;
            if (snapshot.bgTeam != callerFaction)
                continue;  // Different faction from caller = not ally
            if (!snapshot.isAlive)
                continue;

            float distSq = snapshot.GetDistance2D(position);
            distSq *= distSq;
            if (distSq <= radiusSq)
                ++count;
        }
    }

    return count;
}

// ============================================================================
// METRICS / LOGGING
// ============================================================================

void BGSpatialQueryCache::LogPerformanceSummary() const
{
    TC_LOG_INFO("playerbots.bg.spatial",
        "BGSpatialQueryCache Performance Summary:\n"
        "  Total Queries: {}\n"
        "  Cache Hit Rate: {:.1f}%\n"
        "  Avg Query Time: {:.2f} us\n"
        "  Flag Carrier Queries: {}\n"
        "  Nearby Player Queries: {}\n"
        "  Nearest Enemy Queries: {}\n"
        "  Cached Players: {}\n"
        "  Active Cells: {}",
        _metrics.totalQueries.load(),
        _metrics.GetCacheHitRate() * 100.0f,
        _metrics.GetAverageQueryTimeUs(),
        _metrics.flagCarrierQueries.load(),
        _metrics.nearbyPlayerQueries.load(),
        _metrics.nearestEnemyQueries.load(),
        _playerSnapshots.size(),
        _spatialCells.size());
}

} // namespace Playerbot
