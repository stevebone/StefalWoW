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

#ifndef ROAD_NETWORK_MANAGER_H
#define ROAD_NETWORK_MANAGER_H

#include "Define.h"
#include "RoadNetworkTypes.h"
#include "RoadSpatialIndex.h"
#include "RoadGraphPathfinder.h"
#include "Position.h"
#include <atomic>
#include <string>
#include <unordered_map>
#include <vector>

namespace Playerbot
{

class RoadNetworkManager
{
public:
    static RoadNetworkManager* Instance();

    // Startup/shutdown
    bool Initialize(std::string const& roadDataPath);
    void Shutdown();

    // Core queries (all const, thread-safe)
    bool HasRoadNetwork(uint32 mapId) const;
    uint32 FindNearestRoadNode(uint32 mapId, float x, float y, float maxRange = 200.0f) const;
    Position GetRoadNodePosition(uint32 mapId, uint32 nodeId) const;

    // Road-aware path computation
    // Returns true if a road path was found and is beneficial
    // outWaypoints will contain road waypoints (start-on-road through end-on-road)
    bool CalculateRoadAwarePath(uint32 mapId,
                                Position const& start,
                                Position const& end,
                                std::vector<Position>& outWaypoints) const;

    // Configuration
    void SetEnabled(bool enabled) { _enabled = enabled; }
    void SetMinDistance(float yards) { _minDistance = yards; }
    void SetMaxDetourRatio(float ratio) { _maxDetourRatio = ratio; }
    void SetMaxEntryDistance(float yards) { _maxEntryDistance = yards; }

    bool IsEnabled() const { return _enabled; }

    // Statistics
    struct Stats
    {
        uint32 mapsLoaded = 0;
        uint32 totalNodes = 0;
        uint32 totalEdges = 0;
        std::atomic<uint32> roadPathsUsed{0};
        std::atomic<uint32> directFallbacks{0};
    };
    Stats const& GetStats() const { return _stats; }

private:
    RoadNetworkManager() = default;
    ~RoadNetworkManager() = default;
    RoadNetworkManager(RoadNetworkManager const&) = delete;
    RoadNetworkManager& operator=(RoadNetworkManager const&) = delete;

    bool LoadMapRoadNetwork(std::string const& filePath, uint32 mapId);

    struct MapRoadData
    {
        RoadNetworkData network;
        RoadSpatialIndex spatialIndex;
    };

    // Immutable after Initialize() - safe for concurrent read access
    std::unordered_map<uint32, MapRoadData> _mapData;
    RoadGraphPathfinder _pathfinder;
    mutable Stats _stats;

    bool _enabled = true;
    bool _initialized = false;
    float _minDistance = 200.0f;
    float _maxDetourRatio = 1.5f;
    float _maxEntryDistance = 200.0f;
};

} // namespace Playerbot

#define sRoadNetworkMgr Playerbot::RoadNetworkManager::Instance()

#endif // ROAD_NETWORK_MANAGER_H
