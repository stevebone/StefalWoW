/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "TerrainCache.h"
#include "Log.h"

namespace Playerbot
{

TerrainCache::TerrainCache(Map* map)
    : _map(map)
{
    ASSERT(map, "TerrainCache requires valid Map pointer");

    TC_LOG_INFO("playerbot.spatial",
        "TerrainCache created for map {} ({}), grid size: {}x{}, memory: ~{} KB",
        map->GetId(), map->GetMapName(), GRID_SIZE, GRID_SIZE,
        (GRID_SIZE * GRID_SIZE * sizeof(TerrainData)) / 1024);
}

::std::pair<uint32, uint32> TerrainCache::GetCellCoords(Position const& pos) const
{
    // Convert world coordinates to grid cell indices
    // Map coordinate system:
    //   - Origin: (0, 0) at map center
    //   - Range: -17066.67 to +17066.67 in both X and Y (34133.33 total)
    //   - Grid: 512×512 cells, each cell is 66.6666 yards (34133.33 / 512 = 66.6666)

    // Offset position to 0-based coordinate system
    float offsetX = pos.GetPositionX() + 17066.67f;
    float offsetY = pos.GetPositionY() + 17066.67f;

    // Calculate cell indices
    uint32 cellX = static_cast<uint32>(offsetX / CELL_SIZE);
    uint32 cellY = static_cast<uint32>(offsetY / CELL_SIZE);

    // Clamp to valid range (handle edge cases where position is outside map bounds)
    cellX = ::std::min(cellX, GRID_SIZE - 1);
    cellY = ::std::min(cellY, GRID_SIZE - 1);

    return {cellX, cellY};
}

TerrainCache::TerrainData TerrainCache::GetTerrainData(Position const& pos, PhaseShift const& phaseShift)
{
    auto [cellX, cellY] = GetCellCoords(pos);
    auto& cachedData = _grid[cellX][cellY];

    // FAST PATH: Check if cached data is valid and not expired
    if (cachedData.isValid && !cachedData.IsExpired())
    {
        ++_stats.hits;
        return cachedData;
    }

    // SLOW PATH: Cache miss - query TrinityCore Map API
    ++_stats.misses;

    TerrainData freshData;

    // Query height (VMAP + NavMesh lookup)
    // Parameters:
    //   - phaseShift: Phase-aware query (different phases can have different terrain)
    //   - x, y, z: Position to query
    //   - vmap: true = check VMAP collision data
    //   - maxSearchDist: 50.0f default (search up to 50 yards for ground)
    freshData.height = _map->GetHeight(
        phaseShift,
        pos.GetPositionX(),
        pos.GetPositionY(),
        pos.GetPositionZ(),
        true,   // Use VMAP
        50.0f   // Max search distance
    );

    // Query water level
    // Returns Z coordinate of water surface, or VMAP_INVALID_HEIGHT_VALUE if no water
    freshData.waterLevel = _map->GetWaterLevel(
        phaseShift,
        pos.GetPositionX(),
        pos.GetPositionY()
    );

    // Query detailed liquid status
    // Provides complete liquid information (type, depth, flags)
    LiquidData liquidData;
    freshData.liquidStatus = _map->GetLiquidStatus(
        phaseShift,
        pos.GetPositionX(),
        pos.GetPositionY(),
        pos.GetPositionZ(),
        Optional<map_liquidHeaderTypeFlags>(),  // Optional empty - check all liquid types
        &liquidData,  // LiquidData output parameter
        0.0f  // collisionHeight - check at entity position level
    );

    // Mark as valid and timestamp
    freshData.isValid = true;
    freshData.timestamp = ::std::chrono::steady_clock::now();

    // Update cache
    if (cachedData.isValid)
        ++_stats.evictions;  // Replacing existing entry

    _grid[cellX][cellY] = freshData;

    return freshData;
}

void TerrainCache::WarmCache(::std::vector<Position> const& positions, PhaseShift const& phaseShift)
{
    uint32 populated = 0;

    for (auto const& pos : positions)
    {
        // Query each position to populate cache
        GetTerrainData(pos, phaseShift);
        ++populated;
    }

    TC_LOG_INFO("playerbot.spatial",
        "TerrainCache warmed with {} positions for map {} ({})",
        populated, _map->GetId(), _map->GetMapName());
}

void TerrainCache::InvalidateCell(uint32 x, uint32 y)
{
    if (x < GRID_SIZE && y < GRID_SIZE)
    {
        _grid[x][y].isValid = false;
        ++_stats.evictions;

        TC_LOG_DEBUG("playerbot.spatial",
            "TerrainCache invalidated cell ({}, {}) for map {}",
            x, y, _map->GetId());
    }
    else
    {
        TC_LOG_WARN("playerbot.spatial",
            "TerrainCache InvalidateCell called with out-of-bounds coordinates ({}, {}) for map {}",
            x, y, _map->GetId());
    }
}

void TerrainCache::Clear()
{
    uint32 invalidated = 0;

    // Invalidate all cells
    for (auto& row : _grid)
    {
        for (auto& cell : row)
        {
            if (cell.isValid)
            {
                cell.isValid = false;
                ++invalidated;
            }
        }
    }

    _stats.evictions += invalidated;

    TC_LOG_INFO("playerbot.spatial",
        "TerrainCache cleared for map {} ({}), {} entries invalidated",
        _map->GetId(), _map->GetMapName(), invalidated);
}

} // namespace Playerbot
