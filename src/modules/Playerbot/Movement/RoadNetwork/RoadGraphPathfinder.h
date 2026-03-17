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

#ifndef ROAD_GRAPH_PATHFINDER_H
#define ROAD_GRAPH_PATHFINDER_H

#include "Define.h"
#include "RoadNetworkTypes.h"
#include "Position.h"
#include <vector>

namespace Playerbot
{

// A* pathfinder on the road graph
// All methods are const/stateless - thread-safe for concurrent read access
class RoadGraphPathfinder
{
public:
    struct PathResult
    {
        bool valid = false;
        float totalCost = 0.0f;
        std::vector<uint32> nodeIndices;    // Sequence of road node IDs
    };

    // Find shortest road path between two nodes using A*
    PathResult FindPath(RoadNetworkData const& data,
                        uint32 startNode, uint32 endNode) const;

    // Expand a node-index path into world-position waypoints, including shape points
    std::vector<Position> ExpandPathToWaypoints(RoadNetworkData const& data,
                                                 PathResult const& path) const;

private:
    float Heuristic(RoadNetworkData const& data, uint32 nodeA, uint32 nodeB) const;
};

} // namespace Playerbot

#endif // ROAD_GRAPH_PATHFINDER_H
