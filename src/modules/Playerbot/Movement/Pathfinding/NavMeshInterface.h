/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITY_PLAYERBOT_NAVMESH_INTERFACE_H
#define TRINITY_PLAYERBOT_NAVMESH_INTERFACE_H

#include "../Core/MovementTypes.h"
#include "Define.h"
#include <atomic>
#include <memory>

class Map;
class dtNavMesh;
class dtNavMeshQuery;

namespace Playerbot
{
    /**
     * @class NavMeshInterface
     * @brief Interface to TrinityCore's navigation mesh system
     *
     * This class provides a wrapper around the Recast/Detour navigation mesh
     * functionality, offering optimized queries and utilities for bot movement.
     */
    class TC_GAME_API NavMeshInterface
    {
    public:
        /**
         * @brief Constructor
         */
        NavMeshInterface();

        /**
         * @brief Destructor
         */
        ~NavMeshInterface();

        /**
         * @brief Initialize the navigation interface
         * @return True if initialization successful
         */
        bool Initialize();

        /**
         * @brief Shutdown and cleanup
         */
        void Shutdown();

        /**
         * @brief Query ground height at position
         * @param map The map
         * @param x X coordinate
         * @param y Y coordinate
         * @param z Z coordinate (output)
         * @param maxSearchDist Maximum search distance
         * @return True if ground found
         */
        bool GetGroundHeight(Map* map, float x, float y, float& z,
                           float maxSearchDist = 10.0f) const;

        /**
         * @brief Find random position around point
         * @param map The map
         * @param center Center position
         * @param radius Search radius
         * @param result Output position
         * @return True if position found
         */
        bool GetRandomPosition(Map* map, Position const& center,
                              float radius, Position& result) const;

        /**
         * @brief Find nearest position on navmesh
         * @param map The map
         * @param position Input position
         * @param result Nearest navmesh position
         * @param searchDist Search distance
         * @return True if position found
         */
        bool GetNearestPosition(Map* map, Position const& position,
                               Position& result, float searchDist = 10.0f) const;

        /**
         * @brief Calculate distance along navmesh
         * @param map The map
         * @param start Start position
         * @param end End position
         * @return Distance along navmesh, or -1 if no path
         */
        float GetPathDistance(Map* map, Position const& start,
                            Position const& end) const;

        /**
         * @brief Check if position is on navmesh
         * @param map The map
         * @param position Position to check
         * @param tolerance Position tolerance
         * @return True if on navmesh
         */
        bool IsOnNavMesh(Map* map, Position const& position,
                        float tolerance = 2.0f) const;

        /**
         * @brief Get area flags at position
         * @param map The map
         * @param position Position to query
         * @return Area flags (water, slime, etc.)
         */
        uint32 GetAreaFlags(Map* map, Position const& position) const;

        /**
         * @brief Check line of sight on navmesh
         * @param map The map
         * @param start Start position
         * @param end End position
         * @return True if line of sight exists
         */
        bool HasLineOfSight(Map* map, Position const& start,
                          Position const& end) const;

        /**
         * @brief Find smooth path corner positions
         * @param map The map
         * @param path Raw path points
         * @param smoothPath Output smooth path
         * @param maxSmoothPoints Maximum smooth points
         * @return Number of smooth points
         */
        uint32 FindSmoothPath(Map* map, ::std::vector<Position> const& path,
                            ::std::vector<Position>& smoothPath,
                            uint32 maxSmoothPoints = 20) const;

        /**
         * @brief Get slope at position
         * @param map The map
         * @param position Position to check
         * @return Slope angle in radians
         */
        float GetSlope(Map* map, Position const& position) const;

        /**
         * @brief Check if area is walkable
         * @param areaFlags Area flags to check
         * @return True if walkable
         */
        bool IsWalkableArea(uint32 areaFlags) const;

        /**
         * @brief Check if area is water
         * @param areaFlags Area flags to check
         * @return True if water area
         */
        bool IsWaterArea(uint32 areaFlags) const;

        /**
         * @brief Get nearest walkable position in direction
         * @param map The map
         * @param origin Starting position
         * @param direction Direction to search
         * @param distance Search distance
         * @param result Output position
         * @return True if position found
         */
        bool GetPositionInDirection(Map* map, Position const& origin,
                                   float direction, float distance,
                                   Position& result) const;

        /**
         * @brief Calculate avoidance position
         * @param map The map
         * @param position Current position
         * @param avoidPos Position to avoid
         * @param avoidRadius Avoidance radius
         * @param result Output avoidance position
         * @return True if avoidance position found
         */
        bool CalculateAvoidancePosition(Map* map, Position const& position,
                                       Position const& avoidPos,
                                       float avoidRadius, Position& result) const;

        /**
         * @brief Get query statistics
         * @param queries Total queries performed
         * @param hits Successful queries
         * @param avgTime Average query time in microseconds
         */
        void GetStatistics(uint32& queries, uint32& hits, uint32& avgTime) const;

        /**
         * @brief Reset statistics
         */
        void ResetStatistics();

    private:
        /**
         * @brief Get or load navmesh for map
         * @param map The map
         * @return Navigation mesh or nullptr
         */
        dtNavMesh const* GetNavMesh(Map* map) const;

        /**
         * @brief Get or create query object for map
         * @param map The map
         * @return Navigation query or nullptr
         */
        dtNavMeshQuery const* GetNavMeshQuery(Map* map) const;

        /**
         * @brief Convert world position to navmesh position
         * @param worldPos World position
         * @param navPos Navmesh position (output)
         */
        void WorldToNav(Position const& worldPos, float* navPos) const;

        /**
         * @brief Convert navmesh position to world position
         * @param navPos Navmesh position
         * @param worldPos World position (output)
         */
        void NavToWorld(float const* navPos, Position& worldPos) const;

        /**
         * @brief Find nearest poly on navmesh
         * @param query Query object
         * @param point Point to search from
         * @param extents Search extents
         * @param polyRef Output poly reference
         * @param nearestPt Output nearest point
         * @return True if poly found
         */
        bool FindNearestPoly(dtNavMeshQuery const* query, float const* point,
                           float const* extents, uint32& polyRef,
                           float* nearestPt) const;

        // Statistics
        mutable ::std::atomic<uint32> _totalQueries;
        mutable ::std::atomic<uint32> _successfulQueries;
        mutable ::std::atomic<uint64> _totalQueryTime;

        // Configuration
        float _defaultSearchExtent[3];
        float _polyPickExtent[3];
        uint32 _maxSearchNodes;
    };
}

#endif // TRINITY_PLAYERBOT_NAVMESH_INTERFACE_H