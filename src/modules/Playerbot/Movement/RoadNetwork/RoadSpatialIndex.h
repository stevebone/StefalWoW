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

#ifndef ROAD_SPATIAL_INDEX_H
#define ROAD_SPATIAL_INDEX_H

#include "Define.h"
#include "RoadNetworkTypes.h"
#include <vector>

namespace Playerbot
{

// Uniform 2D grid spatial index for O(1) nearest road node queries
// Covers the full continent range: 34,133 yards per axis
class RoadSpatialIndex
{
public:
    // Build spatial index from road network data
    void Build(RoadNetworkData const& data);

    // Find the nearest road node to (x, y) within maxRange yards
    // Returns node index, or UINT32_MAX if none found
    uint32 FindNearestNode(RoadNetworkData const& data,
                           float x, float y, float maxRange) const;

    void Clear();

    bool IsBuilt() const { return _built; }

private:
    static constexpr float CELL_SIZE = 100.0f;  // 100-yard grid cells
    static constexpr float MAP_HALF_SIZE = 17066.66f;  // 32 * 533.33
    static constexpr uint32 GRID_SIZE = 342;  // ceil(34133 / 100)

    uint32 WorldToGrid(float coord) const;
    uint32 GridIndex(uint32 gx, uint32 gy) const { return gy * GRID_SIZE + gx; }

    // Each cell stores indices of road nodes that fall within it
    std::vector<std::vector<uint32>> _cells;
    bool _built = false;
};

} // namespace Playerbot

#endif // ROAD_SPATIAL_INDEX_H
