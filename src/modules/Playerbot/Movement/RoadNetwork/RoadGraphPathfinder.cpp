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

#include "RoadGraphPathfinder.h"
#include <cmath>
#include <queue>
#include <unordered_map>
#include <limits>

namespace Playerbot
{

float RoadGraphPathfinder::Heuristic(RoadNetworkData const& data,
                                      uint32 nodeA, uint32 nodeB) const
{
    float dx = data.nodes[nodeA].x - data.nodes[nodeB].x;
    float dy = data.nodes[nodeA].y - data.nodes[nodeB].y;
    return std::sqrt(dx * dx + dy * dy);
}

RoadGraphPathfinder::PathResult RoadGraphPathfinder::FindPath(
    RoadNetworkData const& data, uint32 startNode, uint32 endNode) const
{
    PathResult result;

    if (startNode >= data.nodes.size() || endNode >= data.nodes.size())
        return result;

    if (startNode == endNode)
    {
        result.valid = true;
        result.totalCost = 0.0f;
        result.nodeIndices.push_back(startNode);
        return result;
    }

    // A* search
    using PQEntry = std::pair<float, uint32>;  // (fCost, nodeIdx)
    std::priority_queue<PQEntry, std::vector<PQEntry>, std::greater<PQEntry>> openSet;

    std::unordered_map<uint32, float> gScore;
    std::unordered_map<uint32, uint32> cameFrom;

    gScore[startNode] = 0.0f;
    openSet.push({ Heuristic(data, startNode, endNode), startNode });

    uint32 nodesExpanded = 0;
    static constexpr uint32 MAX_EXPANSION = 100000;

    while (!openSet.empty() && nodesExpanded < MAX_EXPANSION)
    {
        auto [fCost, current] = openSet.top();
        openSet.pop();

        if (current == endNode)
        {
            // Reconstruct path
            result.valid = true;
            result.totalCost = gScore[endNode];

            uint32 node = endNode;
            while (node != startNode)
            {
                result.nodeIndices.push_back(node);
                node = cameFrom[node];
            }
            result.nodeIndices.push_back(startNode);

            // Reverse to get start->end order
            std::reverse(result.nodeIndices.begin(), result.nodeIndices.end());
            return result;
        }

        // Skip if we've already found a better path to this node
        float currentG = gScore[current];
        if (fCost - Heuristic(data, current, endNode) > currentG + 0.001f)
            continue;

        ++nodesExpanded;

        // Expand neighbors via adjacency list
        RoadNode const& node = data.nodes[current];
        for (uint32 i = 0; i < node.edgeCount; ++i)
        {
            uint32 adjListIdx = node.firstEdgeIdx + i;
            if (adjListIdx >= data.adjacencyList.size())
                break;

            uint32 edgeIdx = data.adjacencyList[adjListIdx];
            if (edgeIdx >= data.edges.size())
                continue;

            RoadEdge const& edge = data.edges[edgeIdx];
            uint32 neighbor = edge.GetOtherNode(current);

            if (neighbor >= data.nodes.size())
                continue;

            float tentativeG = currentG + edge.cost;

            auto it = gScore.find(neighbor);
            if (it != gScore.end() && tentativeG >= it->second)
                continue;

            gScore[neighbor] = tentativeG;
            cameFrom[neighbor] = current;

            float fScore = tentativeG + Heuristic(data, neighbor, endNode);
            openSet.push({ fScore, neighbor });
        }
    }

    return result;  // No path found
}

std::vector<Position> RoadGraphPathfinder::ExpandPathToWaypoints(
    RoadNetworkData const& data, PathResult const& path) const
{
    std::vector<Position> waypoints;

    if (!path.valid || path.nodeIndices.size() < 2)
    {
        if (path.valid && path.nodeIndices.size() == 1)
            waypoints.push_back(data.nodes[path.nodeIndices[0]].GetPosition());
        return waypoints;
    }

    // First waypoint: start node position
    waypoints.push_back(data.nodes[path.nodeIndices[0]].GetPosition());

    // For each pair of consecutive nodes in the path, find the connecting edge
    // and insert its shape points
    for (uint32 i = 0; i + 1 < path.nodeIndices.size(); ++i)
    {
        uint32 nodeA = path.nodeIndices[i];
        uint32 nodeB = path.nodeIndices[i + 1];

        // Find the edge connecting nodeA and nodeB
        RoadNode const& node = data.nodes[nodeA];
        for (uint32 j = 0; j < node.edgeCount; ++j)
        {
            uint32 adjIdx = node.firstEdgeIdx + j;
            if (adjIdx >= data.adjacencyList.size())
                break;

            uint32 edgeIdx = data.adjacencyList[adjIdx];
            if (edgeIdx >= data.edges.size())
                continue;

            RoadEdge const& edge = data.edges[edgeIdx];
            if ((edge.nodeA == nodeA && edge.nodeB == nodeB) ||
                (edge.nodeA == nodeB && edge.nodeB == nodeA))
            {
                // Insert shape points
                bool reversed = (edge.nodeA == nodeB);
                if (!reversed)
                {
                    for (uint32 sp = 0; sp < edge.shapePointCount; ++sp)
                    {
                        uint32 spIdx = edge.shapePointOffset + sp;
                        if (spIdx < data.shapePoints.size())
                        {
                            auto const& pt = data.shapePoints[spIdx];
                            waypoints.push_back(Position(pt.x, pt.y, pt.z));
                        }
                    }
                }
                else
                {
                    // Reverse shape point order when traversing edge backwards
                    for (int32 sp = int32(edge.shapePointCount) - 1; sp >= 0; --sp)
                    {
                        uint32 spIdx = edge.shapePointOffset + uint32(sp);
                        if (spIdx < data.shapePoints.size())
                        {
                            auto const& pt = data.shapePoints[spIdx];
                            waypoints.push_back(Position(pt.x, pt.y, pt.z));
                        }
                    }
                }
                break;
            }
        }

        // Add the destination node position
        waypoints.push_back(data.nodes[nodeB].GetPosition());
    }

    return waypoints;
}

} // namespace Playerbot
