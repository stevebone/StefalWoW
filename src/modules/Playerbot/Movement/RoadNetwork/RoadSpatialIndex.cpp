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

#include "RoadSpatialIndex.h"
#include <cmath>
#include <limits>

namespace Playerbot
{

uint32 RoadSpatialIndex::WorldToGrid(float coord) const
{
    // World coords range roughly from -MAP_HALF_SIZE to +MAP_HALF_SIZE
    // Shift to 0..MAP_HALF_SIZE*2, then divide by cell size
    float shifted = coord + MAP_HALF_SIZE;
    if (shifted < 0.0f) shifted = 0.0f;
    uint32 cell = static_cast<uint32>(shifted / CELL_SIZE);
    return std::min(cell, GRID_SIZE - 1);
}

void RoadSpatialIndex::Build(RoadNetworkData const& data)
{
    Clear();

    _cells.resize(GRID_SIZE * GRID_SIZE);

    for (uint32 i = 0; i < static_cast<uint32>(data.nodes.size()); ++i)
    {
        uint32 gx = WorldToGrid(data.nodes[i].x);
        uint32 gy = WorldToGrid(data.nodes[i].y);
        _cells[GridIndex(gx, gy)].push_back(i);
    }

    _built = true;
}

uint32 RoadSpatialIndex::FindNearestNode(RoadNetworkData const& data,
                                          float x, float y, float maxRange) const
{
    if (!_built || data.nodes.empty())
        return UINT32_MAX;

    uint32 centerGX = WorldToGrid(x);
    uint32 centerGY = WorldToGrid(y);

    float bestDistSq = std::numeric_limits<float>::max();
    uint32 bestNode = UINT32_MAX;
    float maxRangeSq = maxRange * maxRange;

    // Expand search in concentric rings from center cell
    uint32 maxRingCells = static_cast<uint32>(std::ceil(maxRange / CELL_SIZE)) + 1;

    for (uint32 ring = 0; ring <= maxRingCells; ++ring)
    {
        // Early termination: if the minimum possible distance from this ring
        // exceeds our best find, we can stop
        float ringMinDist = (ring > 0) ? (ring - 1) * CELL_SIZE : 0.0f;
        if (ringMinDist * ringMinDist > bestDistSq)
            break;

        // Iterate all cells in this ring
        int32 minGX = std::max<int32>(0, int32(centerGX) - int32(ring));
        int32 maxGX = std::min<int32>(int32(GRID_SIZE) - 1, int32(centerGX) + int32(ring));
        int32 minGY = std::max<int32>(0, int32(centerGY) - int32(ring));
        int32 maxGY = std::min<int32>(int32(GRID_SIZE) - 1, int32(centerGY) + int32(ring));

        for (int32 gy = minGY; gy <= maxGY; ++gy)
        {
            for (int32 gx = minGX; gx <= maxGX; ++gx)
            {
                // Only process cells on the ring border (not interior, already processed)
                if (ring > 0 &&
                    gx > minGX && gx < maxGX &&
                    gy > minGY && gy < maxGY)
                    continue;

                uint32 cellIdx = GridIndex(uint32(gx), uint32(gy));
                for (uint32 nodeIdx : _cells[cellIdx])
                {
                    float dx = data.nodes[nodeIdx].x - x;
                    float dy = data.nodes[nodeIdx].y - y;
                    float distSq = dx * dx + dy * dy;

                    if (distSq < bestDistSq && distSq <= maxRangeSq)
                    {
                        bestDistSq = distSq;
                        bestNode = nodeIdx;
                    }
                }
            }
        }
    }

    return bestNode;
}

void RoadSpatialIndex::Clear()
{
    _cells.clear();
    _built = false;
}

} // namespace Playerbot
