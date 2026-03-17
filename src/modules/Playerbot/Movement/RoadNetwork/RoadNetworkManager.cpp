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

#include "RoadNetworkManager.h"
#include "Log.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace Playerbot
{

// .roadnet file header (must match the offline tool's RoadNetWriter format)
#pragma pack(push, 1)
struct RoadNetFileHeader
{
    char magic[4];
    uint32 version;
    uint32 mapId;
    uint32 nodeCount;
    uint32 edgeCount;
    uint32 shapePointCount;
    uint32 reserved1;
    uint32 reserved2;
};

struct RoadNetFileNode
{
    float x, y, z;
    uint8 flags;
    uint8 edgeCount;
    uint16 firstEdgeIdx;
};

struct RoadNetFileEdge
{
    uint32 nodeA, nodeB;
    float cost;
    uint16 shapePointOffset;
    uint16 shapePointCount;
};

struct RoadNetFileShapePoint
{
    float x, y, z;
};
#pragma pack(pop)

RoadNetworkManager* RoadNetworkManager::Instance()
{
    static RoadNetworkManager instance;
    return &instance;
}

bool RoadNetworkManager::Initialize(std::string const& roadDataPath)
{
    if (_initialized)
    {
        TC_LOG_WARN("module.playerbot", "RoadNetworkManager: Already initialized");
        return true;
    }

    TC_LOG_INFO("module.playerbot", "RoadNetworkManager: Initializing from '{}'", roadDataPath);

    if (!std::filesystem::exists(roadDataPath))
    {
        TC_LOG_WARN("module.playerbot", "RoadNetworkManager: Road data path '{}' does not exist", roadDataPath);
        _initialized = true;  // Not an error - feature just has no data
        return true;
    }

    // Scan for .roadnet files
    for (auto const& entry : std::filesystem::directory_iterator(roadDataPath))
    {
        if (entry.path().extension() != ".roadnet")
            continue;

        std::string filename = entry.path().stem().string();
        uint32 mapId = 0;
        try
        {
            mapId = std::stoul(filename);
        }
        catch (...)
        {
            TC_LOG_WARN("module.playerbot", "RoadNetworkManager: Skipping invalid filename '{}'",
                        entry.path().string());
            continue;
        }

        if (LoadMapRoadNetwork(entry.path().string(), mapId))
        {
            ++_stats.mapsLoaded;
            _stats.totalNodes += uint32(_mapData[mapId].network.nodes.size());
            _stats.totalEdges += uint32(_mapData[mapId].network.edges.size());
        }
    }

    _initialized = true;

    uint32 loadedMaps = _stats.mapsLoaded;
    uint32 loadedNodes = _stats.totalNodes;
    uint32 loadedEdges = _stats.totalEdges;
    TC_LOG_INFO("module.playerbot", "RoadNetworkManager: Loaded {} maps ({} nodes, {} edges)",
                loadedMaps, loadedNodes, loadedEdges);

    return true;
}

bool RoadNetworkManager::LoadMapRoadNetwork(std::string const& filePath, uint32 mapId)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open())
    {
        TC_LOG_ERROR("module.playerbot", "RoadNetworkManager: Cannot open '{}'", filePath);
        return false;
    }

    // Read header
    RoadNetFileHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!file)
    {
        TC_LOG_ERROR("module.playerbot", "RoadNetworkManager: Failed to read header from '{}'", filePath);
        return false;
    }

    // Validate magic
    if (std::memcmp(header.magic, "RNET", 4) != 0)
    {
        TC_LOG_ERROR("module.playerbot", "RoadNetworkManager: Invalid magic in '{}'", filePath);
        return false;
    }

    if (header.version != 1)
    {
        TC_LOG_ERROR("module.playerbot", "RoadNetworkManager: Unsupported version {} in '{}'",
                     header.version, filePath);
        return false;
    }

    if (header.nodeCount == 0)
    {
        TC_LOG_WARN("module.playerbot", "RoadNetworkManager: Empty road network in '{}'", filePath);
        return false;
    }

    MapRoadData& mapData = _mapData[mapId];
    mapData.network.mapId = mapId;

    // Read nodes
    mapData.network.nodes.resize(header.nodeCount);
    for (uint32 i = 0; i < header.nodeCount; ++i)
    {
        RoadNetFileNode fileNode;
        file.read(reinterpret_cast<char*>(&fileNode), sizeof(fileNode));
        if (!file) return false;

        RoadNode& node = mapData.network.nodes[i];
        node.x = fileNode.x;
        node.y = fileNode.y;
        node.z = fileNode.z;
        node.flags = fileNode.flags;
        node.edgeCount = fileNode.edgeCount;
        node.firstEdgeIdx = fileNode.firstEdgeIdx;
    }

    // Read edges
    mapData.network.edges.resize(header.edgeCount);
    for (uint32 i = 0; i < header.edgeCount; ++i)
    {
        RoadNetFileEdge fileEdge;
        file.read(reinterpret_cast<char*>(&fileEdge), sizeof(fileEdge));
        if (!file) return false;

        RoadEdge& edge = mapData.network.edges[i];
        edge.nodeA = fileEdge.nodeA;
        edge.nodeB = fileEdge.nodeB;
        edge.cost = fileEdge.cost;
        edge.shapePointOffset = fileEdge.shapePointOffset;
        edge.shapePointCount = fileEdge.shapePointCount;
    }

    // Read shape points
    mapData.network.shapePoints.resize(header.shapePointCount);
    for (uint32 i = 0; i < header.shapePointCount; ++i)
    {
        RoadNetFileShapePoint fileSP;
        file.read(reinterpret_cast<char*>(&fileSP), sizeof(fileSP));
        if (!file) return false;

        mapData.network.shapePoints[i] = { fileSP.x, fileSP.y, fileSP.z };
    }

    // Read adjacency list
    // Count total adjacency entries from nodes
    uint32 totalAdj = 0;
    for (auto const& node : mapData.network.nodes)
        totalAdj += node.edgeCount;

    mapData.network.adjacencyList.resize(totalAdj);
    if (totalAdj > 0)
        file.read(reinterpret_cast<char*>(mapData.network.adjacencyList.data()),
                  totalAdj * sizeof(uint32));

    // Build spatial index
    mapData.spatialIndex.Build(mapData.network);

    TC_LOG_DEBUG("module.playerbot", "RoadNetworkManager: Map {} loaded ({} nodes, {} edges)",
                 mapId, header.nodeCount, header.edgeCount);

    return true;
}

void RoadNetworkManager::Shutdown()
{
    _mapData.clear();
    _initialized = false;
    TC_LOG_INFO("module.playerbot", "RoadNetworkManager: Shutdown complete");
}

bool RoadNetworkManager::HasRoadNetwork(uint32 mapId) const
{
    return _enabled && _mapData.count(mapId) > 0;
}

uint32 RoadNetworkManager::FindNearestRoadNode(uint32 mapId, float x, float y, float maxRange) const
{
    auto it = _mapData.find(mapId);
    if (it == _mapData.end())
        return UINT32_MAX;

    return it->second.spatialIndex.FindNearestNode(it->second.network, x, y, maxRange);
}

Position RoadNetworkManager::GetRoadNodePosition(uint32 mapId, uint32 nodeId) const
{
    auto it = _mapData.find(mapId);
    if (it == _mapData.end() || nodeId >= it->second.network.nodes.size())
        return Position();

    return it->second.network.nodes[nodeId].GetPosition();
}

bool RoadNetworkManager::CalculateRoadAwarePath(uint32 mapId,
                                                 Position const& start,
                                                 Position const& end,
                                                 std::vector<Position>& outWaypoints) const
{
    outWaypoints.clear();

    if (!_enabled)
        return false;

    auto it = _mapData.find(mapId);
    if (it == _mapData.end())
        return false;

    MapRoadData const& mapData = it->second;

    // Check minimum distance threshold
    float directDist = start.GetExactDist2d(end);
    if (directDist < _minDistance)
    {
        _stats.directFallbacks.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    // Find nearest road nodes to start and end
    uint32 startNode = mapData.spatialIndex.FindNearestNode(
        mapData.network, start.GetPositionX(), start.GetPositionY(), _maxEntryDistance);

    uint32 endNode = mapData.spatialIndex.FindNearestNode(
        mapData.network, end.GetPositionX(), end.GetPositionY(), _maxEntryDistance);

    if (startNode == UINT32_MAX || endNode == UINT32_MAX)
    {
        _stats.directFallbacks.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    if (startNode == endNode)
    {
        _stats.directFallbacks.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    // Check if the detour to reach the road is too far
    Position startRoadPos = mapData.network.nodes[startNode].GetPosition();
    Position endRoadPos = mapData.network.nodes[endNode].GetPosition();

    float startEntryDist = start.GetExactDist2d(startRoadPos);
    float endEntryDist = end.GetExactDist2d(endRoadPos);

    // If entry/exit distance is a large fraction of the total trip, skip
    if (startEntryDist + endEntryDist > directDist * 0.8f)
    {
        _stats.directFallbacks.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    // Find road path
    RoadGraphPathfinder::PathResult roadPath = _pathfinder.FindPath(mapData.network, startNode, endNode);
    if (!roadPath.valid)
    {
        _stats.directFallbacks.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    // Check detour ratio
    float totalRoadDist = roadPath.totalCost + startEntryDist + endEntryDist;
    if (totalRoadDist > directDist * _maxDetourRatio)
    {
        _stats.directFallbacks.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    // Expand to waypoints
    outWaypoints = _pathfinder.ExpandPathToWaypoints(mapData.network, roadPath);

    if (outWaypoints.empty())
    {
        _stats.directFallbacks.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    _stats.roadPathsUsed.fetch_add(1, std::memory_order_relaxed);
    return true;
}

} // namespace Playerbot
