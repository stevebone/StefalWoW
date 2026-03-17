/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "NavMeshInterface.h"
#include "Map.h"
#include "MMapManager.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"
#include "DetourCommon.h"
#include "PhasingHandler.h"
#include "Log.h"
#include <chrono>
#include <random>
#include <cfloat>

namespace Playerbot
{
    // Area flags from Recast
    enum NavAreaFlags
    {
        NAV_AREA_GROUND         = 0,
        NAV_AREA_WATER          = 1,
        NAV_AREA_MAGMA_SLIME    = 2,
        NAV_AREA_NO_WALK        = 3
    };

    NavMeshInterface::NavMeshInterface()
        : _totalQueries(0), _successfulQueries(0), _totalQueryTime(0),
          _maxSearchNodes(2048)
    {
        // Default search extents
        _defaultSearchExtent[0] = 5.0f;  // X
        _defaultSearchExtent[1] = 5.0f;  // Y
        _defaultSearchExtent[2] = 5.0f;  // Z

        _polyPickExtent[0] = 2.0f;
        _polyPickExtent[1] = 2.0f;
        _polyPickExtent[2] = 2.0f;
    }

    NavMeshInterface::~NavMeshInterface()
    {
        Shutdown();
    }

    bool NavMeshInterface::Initialize()
    {
        TC_LOG_INFO("playerbot.movement", "NavMeshInterface initialized");
        return true;
    }

    void NavMeshInterface::Shutdown()
    {
        // Cleanup handled by MMapManager
    }

    bool NavMeshInterface::GetGroundHeight(Map* map, float x, float y, float& z,
                                          float maxSearchDist) const
    {
        if (!map)
            return false;

        auto startTime = ::std::chrono::high_resolution_clock::now();
        _totalQueries.fetch_add(1);

        // Use TrinityCore's built-in height calculation
        // Note: We use PhasingHandler::GetEmptyPhaseShift() as we don't have player context here
        float groundZ = map->GetHeight(PhasingHandler::GetEmptyPhaseShift(), x, y, z, true, maxSearchDist);

        auto endTime = ::std::chrono::high_resolution_clock::now();
        uint64 queryTime = ::std::chrono::duration_cast<::std::chrono::microseconds>(
            endTime - startTime).count();
        _totalQueryTime.fetch_add(queryTime);

        if (groundZ > INVALID_HEIGHT)
        {
            z = groundZ;
            _successfulQueries.fetch_add(1);
            return true;
        }

        // Try with navmesh if regular height failed
        dtNavMeshQuery const* query = GetNavMeshQuery(map);
        if (!query)
            return false;

        float point[3] = { y, z, x };  // Recast uses Y-up coordinate system
        float nearestPt[3];
        uint32 polyRef;

        float searchExtent[3] = { maxSearchDist, maxSearchDist, maxSearchDist };
        if (FindNearestPoly(query, point, searchExtent, polyRef, nearestPt))
        {
            z = nearestPt[1];  // Y in Recast = Z in WoW
            _successfulQueries.fetch_add(1);
            return true;
        }

        return false;
    }

    bool NavMeshInterface::GetRandomPosition(Map* map, Position const& center,
                                            float radius, Position& result) const
    {
        if (!map || radius <= 0)
            return false;

        _totalQueries.fetch_add(1);

        dtNavMeshQuery const* query = GetNavMeshQuery(map);
        if (!query)
            return false;

        float centerPoint[3];
        WorldToNav(center, centerPoint);

        // Find nearest poly to center
        uint32 centerPolyRef;
        float nearestPt[3];
        if (!FindNearestPoly(query, centerPoint, _defaultSearchExtent, centerPolyRef, nearestPt))
            return false;

        // Generate random point
        static thread_local ::std::mt19937 gen(::std::chrono::steady_clock::now().time_since_epoch().count());
        static thread_local ::std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        // Simple random function for Detour
        auto frand = []() -> float { return dist(gen); };

        float randomPt[3];
        dtPolyRef randomPolyRef;
        dtStatus status = query->findRandomPointAroundCircle(centerPolyRef, centerPoint,
            radius, nullptr, frand, &randomPolyRef, randomPt);

        if (dtStatusSucceed(status))
        {
            NavToWorld(randomPt, result);
            _successfulQueries.fetch_add(1);
            return true;
        }

        // Fallback: simple random position
        float angle = dist(gen) * 2.0f * static_cast<float>(M_PI);
        float distance = dist(gen) * radius;
        result.m_positionX = center.GetPositionX() + distance * cos(angle);
        result.m_positionY = center.GetPositionY() + distance * sin(angle);
        result.m_positionZ = center.GetPositionZ();

        // Adjust height
        GetGroundHeight(map, result.m_positionX, result.m_positionY, result.m_positionZ);

        return true;
    }

    bool NavMeshInterface::GetNearestPosition(Map* map, Position const& position,
                                             Position& result, float searchDist) const
    {
        if (!map)
            return false;

        _totalQueries.fetch_add(1);

        dtNavMeshQuery const* query = GetNavMeshQuery(map);
        if (!query)
            return false;

        float point[3];
        WorldToNav(position, point);

        uint32 polyRef;
        float nearestPt[3];
        float searchExtent[3] = { searchDist, searchDist, searchDist };

        if (FindNearestPoly(query, point, searchExtent, polyRef, nearestPt))
        {
            NavToWorld(nearestPt, result);
            _successfulQueries.fetch_add(1);
            return true;
        }

        return false;
    }

    float NavMeshInterface::GetPathDistance(Map* map, Position const& start,
                                           Position const& end) const
    {
        if (!map)
            return -1.0f;

        _totalQueries.fetch_add(1);

        dtNavMeshQuery const* query = GetNavMeshQuery(map);
        if (!query)
            return -1.0f;

        float startPoint[3], endPoint[3];
        WorldToNav(start, startPoint);
        WorldToNav(end, endPoint);

        // Find nearest polys
        uint32 startPolyRef, endPolyRef;
        float nearestStart[3], nearestEnd[3];

        if (!FindNearestPoly(query, startPoint, _defaultSearchExtent, startPolyRef, nearestStart) ||
            !FindNearestPoly(query, endPoint, _defaultSearchExtent, endPolyRef, nearestEnd))
        {
            return -1.0f;
        }

        // Find path
        dtPolyRef pathPolys[256];
        int pathPolyCount = 0;
        dtStatus pathFindStatus = query->findPath(startPolyRef, endPolyRef,
            nearestStart, nearestEnd, nullptr, pathPolys, &pathPolyCount, 256);

        if (dtStatusFailed(pathFindStatus) || pathPolyCount == 0)
            return -1.0f;

        // Calculate path length
        float pathLength = 0.0f;
        float straightPath[256 * 3];
        unsigned char straightPathFlags[256];
        dtPolyRef straightPathPolys[256];
        int straightPathCount = 0;

        dtStatus status = query->findStraightPath(nearestStart, nearestEnd,
            pathPolys, pathPolyCount, straightPath, straightPathFlags,
            straightPathPolys, &straightPathCount, 256, DT_STRAIGHTPATH_ALL_CROSSINGS);

        if (dtStatusSucceed(status) && straightPathCount > 1)
        {
            for (int i = 1; i < straightPathCount; ++i)
            {
                float* p1 = &straightPath[(i - 1) * 3];
                float* p2 = &straightPath[i * 3];
                float dx = p2[0] - p1[0];
                float dy = p2[1] - p1[1];
                float dz = p2[2] - p1[2];
                pathLength += sqrt(dx * dx + dy * dy + dz * dz);
            }
            _successfulQueries.fetch_add(1);
            return pathLength;
        }

        return -1.0f;
    }

    bool NavMeshInterface::IsOnNavMesh(Map* map, Position const& position,
                                      float tolerance) const
    {
        if (!map)
            return false;

        _totalQueries.fetch_add(1);

        dtNavMeshQuery const* query = GetNavMeshQuery(map);
        if (!query)
            return false;

        float point[3];
        WorldToNav(position, point);

        uint32 polyRef;
        float nearestPt[3];
        float searchExtent[3] = { tolerance, tolerance, tolerance };

        if (FindNearestPoly(query, point, searchExtent, polyRef, nearestPt))
        {
            // Check distance to nearest point
            float dx = nearestPt[0] - point[0];
            float dy = nearestPt[1] - point[1];
            float dz = nearestPt[2] - point[2];
            float dist = sqrt(dx * dx + dy * dy + dz * dz);

            if (dist <= tolerance)
            {
                _successfulQueries.fetch_add(1);
                return true;
            }
        }

        return false;
    }

    uint32 NavMeshInterface::GetAreaFlags(Map* map, Position const& position) const
    {
        if (!map)
            return 0;

        _totalQueries.fetch_add(1);

        // Check liquid status
        LiquidData liquidData;
        ZLiquidStatus liquidStatus = map->GetLiquidStatus(PhasingHandler::GetEmptyPhaseShift(),
            position.GetPositionX(), position.GetPositionY(), position.GetPositionZ(),
            {}, &liquidData);

        uint32 areaFlags = NAV_AREA_GROUND;

        if (liquidStatus != LIQUID_MAP_NO_WATER)
        {
            if (liquidData.type_flags.HasFlag(map_liquidHeaderTypeFlags::Water | map_liquidHeaderTypeFlags::Ocean))
                areaFlags = NAV_AREA_WATER;
            else if (liquidData.type_flags.HasFlag(map_liquidHeaderTypeFlags::Magma | map_liquidHeaderTypeFlags::Slime))
                areaFlags = NAV_AREA_MAGMA_SLIME;
        }

        _successfulQueries.fetch_add(1);
        return areaFlags;
    }

    bool NavMeshInterface::HasLineOfSight(Map* map, Position const& start,
                                         Position const& end) const
    {
        if (!map)
            return false;

        _totalQueries.fetch_add(1);

        dtNavMeshQuery const* query = GetNavMeshQuery(map);
        if (!query)
        {
            // Map doesn't have IsInLineOfSight in TrinityCore 12.0
            // Use simple distance check as fallback
            float distance = start.GetExactDist(&end);
            bool hasLOS = (distance < 100.0f);
            if (hasLOS)
                _successfulQueries.fetch_add(1);
            return hasLOS;
        }

        float startPoint[3], endPoint[3];
        WorldToNav(start, startPoint);
        WorldToNav(end, endPoint);

        // Find nearest polys
        uint32 startPolyRef, endPolyRef;
        float nearestStart[3], nearestEnd[3];

        if (!FindNearestPoly(query, startPoint, _polyPickExtent, startPolyRef, nearestStart) ||
            !FindNearestPoly(query, endPoint, _polyPickExtent, endPolyRef, nearestEnd))
        {
            return false;
        }

        // Raycast on navmesh
        float hitDist = FLT_MAX;
        dtPolyRef polys[256];
        int polyCount = 0;

        dtStatus status = query->raycast(startPolyRef, nearestStart, nearestEnd,
            nullptr, &hitDist, nullptr, polys, &polyCount, 256);

        if (dtStatusSucceed(status))
        {
            // If hit distance is close to 1.0, we have clear LOS
            bool hasLOS = (hitDist >= 0.99f);
            if (hasLOS)
                _successfulQueries.fetch_add(1);
            return hasLOS;
        }

        return false;
    }

    uint32 NavMeshInterface::FindSmoothPath(Map* map, ::std::vector<Position> const& path,
                                           ::std::vector<Position>& smoothPath,
                                           uint32 maxSmoothPoints) const
    {
        if (!map || path.size() < 2)
            return 0;

        _totalQueries.fetch_add(1);

        dtNavMeshQuery const* query = GetNavMeshQuery(map);
        if (!query)
        {
            smoothPath = path;
            return static_cast<uint32>(path.size());
        }

        // Convert path to navmesh coordinates
        ::std::vector<float> navPath;
        navPath.reserve(path.size() * 3);
        for (auto const& pos : path)
        {
            float point[3];
            WorldToNav(pos, point);
            navPath.push_back(point[0]);
            navPath.push_back(point[1]);
            navPath.push_back(point[2]);
        }

        // String pulling algorithm for smooth path
        smoothPath.clear();
        smoothPath.reserve(maxSmoothPoints);

        // Add first point
        smoothPath.push_back(path[0]);

        size_t currentIndex = 0;
        while (currentIndex < path.size() - 1)
        {
            size_t furthestVisible = currentIndex + 1;

            // Find furthest visible point
    for (size_t i = currentIndex + 2; i < path.size(); ++i)
            {
                if (HasLineOfSight(map, path[currentIndex], path[i]))
                {
                    furthestVisible = i;
                }
                else
                {
                    break;
                }
            }

            smoothPath.push_back(path[furthestVisible]);
            currentIndex = furthestVisible;

            if (smoothPath.size() >= maxSmoothPoints)
                break;
        }

        // Ensure end point is included
    if (smoothPath.back().GetExactDist(&path.back()) > 0.1f)
            smoothPath.push_back(path.back());

        _successfulQueries.fetch_add(1);
        return static_cast<uint32>(smoothPath.size());
    }

    float NavMeshInterface::GetSlope(Map* map, Position const& position) const
    {
        if (!map)
            return 0.0f;

        _totalQueries.fetch_add(1);

        // Sample points around position
        float sampleDist = 1.0f;
        float h1 = position.GetPositionZ();
        float h2 = position.GetPositionZ();
        float h3 = position.GetPositionZ();
        float h4 = position.GetPositionZ();

        GetGroundHeight(map, position.GetPositionX() + sampleDist,
                       position.GetPositionY(), h1);
        GetGroundHeight(map, position.GetPositionX() - sampleDist,
                       position.GetPositionY(), h2);
        GetGroundHeight(map, position.GetPositionX(),
                       position.GetPositionY() + sampleDist, h3);
        GetGroundHeight(map, position.GetPositionX(),
                       position.GetPositionY() - sampleDist, h4);

        // Calculate slope from height differences
        float dx = (h1 - h2) / (2 * sampleDist);
        float dy = (h3 - h4) / (2 * sampleDist);
        float slope = atan(sqrt(dx * dx + dy * dy));

        _successfulQueries.fetch_add(1);
        return slope;
    }

    bool NavMeshInterface::IsWalkableArea(uint32 areaFlags) const
    {
        return areaFlags != NAV_AREA_NO_WALK && areaFlags != NAV_AREA_MAGMA_SLIME;
    }

    bool NavMeshInterface::IsWaterArea(uint32 areaFlags) const
    {
        return areaFlags == NAV_AREA_WATER;
    }

    bool NavMeshInterface::GetPositionInDirection(Map* map, Position const& origin,
                                                 float direction, float distance,
                                                 Position& result) const
    {
        if (!map)
            return false;

        _totalQueries.fetch_add(1);

        // Calculate target position
        result.m_positionX = origin.GetPositionX() + distance * cos(direction);
        result.m_positionY = origin.GetPositionY() + distance * sin(direction);
        result.m_positionZ = origin.GetPositionZ();

        // Find nearest valid position on navmesh
        Position nearest;
        if (GetNearestPosition(map, result, nearest, 5.0f))
        {
            result = nearest;
            _successfulQueries.fetch_add(1);
            return true;
        }

        // Fallback: adjust height at calculated position
    if (GetGroundHeight(map, result.m_positionX, result.m_positionY, result.m_positionZ))
        {
            _successfulQueries.fetch_add(1);
            return true;
        }

        return false;
    }

    bool NavMeshInterface::CalculateAvoidancePosition(Map* map, Position const& position,
                                                     Position const& avoidPos,
                                                     float avoidRadius, Position& result) const
    {
        if (!map)
            return false;

        _totalQueries.fetch_add(1);

        // Calculate direction away from avoidance position
        float angle = atan2(position.GetPositionY() - avoidPos.GetPositionY(),
                           position.GetPositionX() - avoidPos.GetPositionX());

        // Try multiple angles if direct opposite is blocked
    for (int i = 0; i < 8; ++i)
        {
            float tryAngle = angle + (i % 2 == 0 ? i/2 * static_cast<float>(M_PI)/4 : -i/2 * static_cast<float>(M_PI)/4);
            tryAngle = Position::NormalizeOrientation(tryAngle);

            if (GetPositionInDirection(map, position, tryAngle, avoidRadius, result))
            {
                // Check if we're far enough from avoidance position
    if (result.GetExactDist(&avoidPos) >= avoidRadius)
                {
                    _successfulQueries.fetch_add(1);
                    return true;
                }
            }
        }

        return false;
    }

    void NavMeshInterface::GetStatistics(uint32& queries, uint32& hits, uint32& avgTime) const
    {
        queries = _totalQueries.load();
        hits = _successfulQueries.load();
        uint64 totalTime = _totalQueryTime.load();
        avgTime = queries > 0 ? static_cast<uint32>(totalTime / queries) : 0;
    }

    void NavMeshInterface::ResetStatistics()
    {
        _totalQueries.store(0);
        _successfulQueries.store(0);
        _totalQueryTime.store(0);
    }

    dtNavMesh const* NavMeshInterface::GetNavMesh(Map* map) const
    {
        if (!map)
            return nullptr;

        MMAP::MMapManager* mmapManager = MMAP::MMapManager::instance();
        if (!mmapManager)
            return nullptr;

        // GetNavMesh now requires (mapId, instanceId) - use map's InstanceId
        return mmapManager->GetNavMesh(map->GetId(), map->GetInstanceId());
    }

    dtNavMeshQuery const* NavMeshInterface::GetNavMeshQuery(Map* map) const
    {
        if (!map)
            return nullptr;

        MMAP::MMapManager* mmapManager = MMAP::MMapManager::instance();
        if (!mmapManager)
            return nullptr;

        // GetNavMeshQuery takes 3 arguments: meshMapId, instanceMapId, instanceId
        return mmapManager->GetNavMeshQuery(map->GetId(), map->GetId(), map->GetInstanceId());
    }

    void NavMeshInterface::WorldToNav(Position const& worldPos, float* navPos) const
    {
        // Recast uses Y-up coordinate system
        // WoW uses Z-up coordinate system
        navPos[0] = worldPos.GetPositionY();  // WoW Y -> Recast X
        navPos[1] = worldPos.GetPositionZ();  // WoW Z -> Recast Y
        navPos[2] = worldPos.GetPositionX();  // WoW X -> Recast Z
    }

    void NavMeshInterface::NavToWorld(float const* navPos, Position& worldPos) const
    {
        // Convert back from Recast to WoW coordinates
        worldPos.m_positionX = navPos[2];  // Recast Z -> WoW X
        worldPos.m_positionY = navPos[0];  // Recast X -> WoW Y
        worldPos.m_positionZ = navPos[1];  // Recast Y -> WoW Z
    }

    bool NavMeshInterface::FindNearestPoly(dtNavMeshQuery const* query, float const* point,
                                          float const* extents, uint32& polyRef,
                                          float* nearestPt) const
    {
        if (!query)
            return false;

        dtPolyRef ref = 0;
        dtStatus status = query->findNearestPoly(point, extents, nullptr, &ref, nearestPt);

        if (dtStatusSucceed(status) && ref != 0)
        {
            polyRef = ref;
            return true;
        }

        return false;
    }
}