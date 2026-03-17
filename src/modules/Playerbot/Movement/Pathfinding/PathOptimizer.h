/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITY_PLAYERBOT_PATH_OPTIMIZER_H
#define TRINITY_PLAYERBOT_PATH_OPTIMIZER_H

#include "../Core/MovementTypes.h"
#include "Define.h"
#include <vector>
#include <atomic>

class Map;

namespace Playerbot
{
    /**
     * @class PathOptimizer
     * @brief Optimizes movement paths for smoother and more efficient navigation
     *
     * This class provides various optimization techniques including path smoothing,
     * corner cutting, and dynamic obstacle avoidance.
     */
    class TC_GAME_API PathOptimizer
    {
    public:
        /**
         * @enum OptimizationLevel
         * @brief Different levels of path optimization
         */
        enum OptimizationLevel
        {
            OPTIMIZATION_NONE = 0,      // No optimization
            OPTIMIZATION_BASIC = 1,      // Remove redundant waypoints
            OPTIMIZATION_SMOOTH = 2,     // Smooth corners and curves
            OPTIMIZATION_AGGRESSIVE = 3  // Maximum optimization
        };

        /**
         * @brief Constructor
         */
        PathOptimizer();

        /**
         * @brief Destructor
         */
        ~PathOptimizer();

        /**
         * @brief Optimize a movement path
         * @param path Path to optimize (modified in place)
         * @param map Map for validation
         * @return True if optimization successful
         */
        bool OptimizePath(MovementPath& path, Map* map);

        /**
         * @brief Remove redundant waypoints from path
         * @param path Path to simplify
         * @param map Map for validation
         * @return Number of waypoints removed
         */
        uint32 RemoveRedundantPoints(MovementPath& path, Map* map);

        /**
         * @brief Smooth path corners
         * @param path Path to smooth
         * @param map Map for validation
         * @param smoothingFactor Smoothing intensity (0.0-1.0)
         * @return True if smoothing applied
         */
        bool SmoothPath(MovementPath& path, Map* map, float smoothingFactor = 0.5f);

        /**
         * @brief Apply corner cutting to path
         * @param path Path to optimize
         * @param map Map for validation
         * @param maxCutDistance Maximum corner cut distance
         * @return Number of corners cut
         */
        uint32 CutCorners(MovementPath& path, Map* map, float maxCutDistance = 5.0f);

        /**
         * @brief Optimize path for specific movement type
         * @param path Path to optimize
         * @param type Movement type (chase, flee, etc.)
         * @return True if optimization applied
         */
        bool OptimizeForMovementType(MovementPath& path, MovementGeneratorType type);

        /**
         * @brief Adjust path for dynamic obstacles
         * @param path Path to adjust
         * @param obstacles List of obstacle positions
         * @param avoidanceRadius Radius to maintain from obstacles
         * @return True if adjustments made
         */
        bool AdjustForObstacles(MovementPath& path, ::std::vector<Position> const& obstacles,
                               float avoidanceRadius = 2.0f);

        /**
         * @brief Optimize path for group movement
         * @param paths Multiple paths to coordinate
         * @param maintainFormation True to preserve formation
         * @return True if optimization successful
         */
        bool OptimizeGroupPaths(::std::vector<MovementPath>& paths, bool maintainFormation);

        /**
         * @brief Set optimization level
         * @param level Optimization level to use
         */
        void SetOptimizationLevel(OptimizationLevel level) { _optimizationLevel = level; }

        /**
         * @brief Get current optimization level
         * @return Current optimization level
         */
        OptimizationLevel GetOptimizationLevel() const { return _optimizationLevel; }

        /**
         * @brief Enable or disable line of sight checks
         * @param enable True to enable LOS checks
         */
        void EnableLineOfSightChecks(bool enable) { _checkLineOfSight = enable; }

        /**
         * @brief Set maximum path length
         * @param maxLength Maximum allowed path length
         */
        void SetMaxPathLength(float maxLength) { _maxPathLength = maxLength; }

        /**
         * @brief Get optimization statistics
         * @param pathsOptimized Total paths optimized
         * @param pointsRemoved Total waypoints removed
         * @param averageReduction Average path length reduction
         */
        void GetStatistics(uint32& pathsOptimized, uint32& pointsRemoved,
                         float& averageReduction) const;

        /**
         * @brief Reset optimization statistics
         */
        void ResetStatistics();

    private:
        /**
         * @brief Check if direct path between two points is valid
         * @param map Map to check
         * @param start Start position
         * @param end End position
         * @return True if direct path is valid
         */
        bool IsDirectPathValid(Map* map, Position const& start, Position const& end) const;

        /**
         * @brief Calculate path curvature at a point
         * @param prev Previous waypoint
         * @param current Current waypoint
         * @param next Next waypoint
         * @return Curvature value (0 = straight, higher = more curved)
         */
        float CalculateCurvature(Position const& prev, Position const& current,
                                Position const& next) const;

        /**
         * @brief Apply Catmull-Rom spline smoothing
         * @param path Path to smooth
         * @param tension Spline tension (0.0-1.0)
         */
        void ApplyCatmullRomSmoothing(MovementPath& path, float tension);

        /**
         * @brief Apply Douglas-Peucker simplification
         * @param path Path to simplify
         * @param epsilon Maximum deviation threshold
         */
        void ApplyDouglasPeucker(MovementPath& path, float epsilon);

        /**
         * @brief Find perpendicular distance from point to line
         * @param point Point to measure from
         * @param lineStart Start of line segment
         * @param lineEnd End of line segment
         * @return Perpendicular distance
         */
        float PerpendicularDistance(Position const& point, Position const& lineStart,
                                   Position const& lineEnd) const;

        /**
         * @brief Interpolate position between two points
         * @param start Start position
         * @param end End position
         * @param t Interpolation factor (0.0-1.0)
         * @return Interpolated position
         */
        Position InterpolatePosition(Position const& start, Position const& end, float t) const;

        /**
         * @brief Check if waypoint can be removed
         * @param path Full path
         * @param index Index of waypoint to check
         * @param map Map for validation
         * @return True if waypoint can be safely removed
         */
        bool CanRemoveWaypoint(MovementPath const& path, size_t index, Map* map) const;

        /**
         * @brief Calculate smooth turn radius
         * @param speed Movement speed
         * @param angle Turn angle in radians
         * @return Recommended turn radius
         */
        float CalculateTurnRadius(float speed, float angle) const;

        /**
         * @brief Validate optimized path
         * @param original Original path
         * @param optimized Optimized path
         * @param map Map for validation
         * @return True if optimized path is valid
         */
        bool ValidateOptimizedPath(MovementPath const& original,
                                  MovementPath const& optimized, Map* map) const;

        // Configuration
        OptimizationLevel _optimizationLevel;
        bool _checkLineOfSight;
        float _maxPathLength;
        float _minWaypointDistance;
        float _maxWaypointDistance;
        float _cornerCutThreshold;
        float _smoothingIterations;

        // Statistics
        mutable ::std::atomic<uint32> _totalPathsOptimized;
        mutable ::std::atomic<uint32> _totalPointsRemoved;
        mutable float _totalLengthReduction;  // Not atomic - fetch_add not supported for float
        mutable ::std::atomic<uint32> _failedOptimizations;
    };
}

#endif // TRINITY_PLAYERBOT_PATH_OPTIMIZER_H