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

#ifndef ROAD_NETWORK_TYPES_H
#define ROAD_NETWORK_TYPES_H

#include "Define.h"
#include "Position.h"
#include <vector>

namespace Playerbot
{

struct RoadNode
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    uint8 flags = 0;
    uint8 edgeCount = 0;
    uint32 firstEdgeIdx = 0;

    enum Flags : uint8
    {
        FLAG_JUNCTION = 0x01,
        FLAG_ENDPOINT = 0x02,
        FLAG_BRIDGE   = 0x04,
        FLAG_TOWN     = 0x08
    };

    Position GetPosition() const { return Position(x, y, z); }
};

struct RoadEdge
{
    uint32 nodeA = 0;
    uint32 nodeB = 0;
    float cost = 0.0f;
    uint32 shapePointOffset = 0;
    uint32 shapePointCount = 0;

    uint32 GetOtherNode(uint32 node) const { return node == nodeA ? nodeB : nodeA; }
};

struct RoadShapePoint
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct RoadNetworkData
{
    uint32 mapId = 0;
    std::vector<RoadNode> nodes;
    std::vector<RoadEdge> edges;
    std::vector<RoadShapePoint> shapePoints;
    std::vector<uint32> adjacencyList;

    bool IsValid() const { return !nodes.empty(); }
};

} // namespace Playerbot

#endif // ROAD_NETWORK_TYPES_H
