/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PathOptimizer.h"
#include "Map.h"
#include "Log.h"
#include <algorithm>
#include <cmath>

namespace Playerbot
{
    PathOptimizer::PathOptimizer()
        : _optimizationLevel(OPTIMIZATION_SMOOTH),
          _checkLineOfSight(true),
          _maxPathLength(200.0f),
          _minWaypointDistance(2.0f),
          _maxWaypointDistance(20.0f),
          _cornerCutThreshold(15.0f * static_cast<float>(M_PI) / 180.0f), // 15 degrees
          _smoothingIterations(3),
          _totalPathsOptimized(0),
          _totalPointsRemoved(0),
          _totalLengthReduction(0.0f),
          _failedOptimizations(0)
    {
    }

    PathOptimizer::~PathOptimizer()
    {
    }

    bool PathOptimizer::OptimizePath(MovementPath& path, Map* map)
    {
        if (!path.IsValid() || _optimizationLevel == OPTIMIZATION_NONE)
            return false;

        size_t originalSize = path.nodes.size();
        float originalLength = path.totalLength;

        bool success = false;

        switch (_optimizationLevel)
        {
        case OPTIMIZATION_BASIC:
            RemoveRedundantPoints(path, map);
            success = true;
            break;

        case OPTIMIZATION_SMOOTH:
            RemoveRedundantPoints(path, map);
            SmoothPath(path, map, 0.5f);
            success = true;
            break;

        case OPTIMIZATION_AGGRESSIVE:
            ApplyDouglasPeucker(path, 2.0f);
            RemoveRedundantPoints(path, map);
            SmoothPath(path, map, 0.7f);
            CutCorners(path, map, 5.0f);
            success = true;
            break;

        default:
            break;
        }

        if (success)
        {
            // Recalculate path length
            path.totalLength = 0.0f;
            for (size_t i = 1; i < path.nodes.size(); ++i)
            {
                path.totalLength += path.nodes[i - 1].position.GetExactDist(
                    &path.nodes[i].position);
            }

            // Update statistics
            _totalPathsOptimized.fetch_add(1);
            size_t pointsRemoved = originalSize - path.nodes.size();
            _totalPointsRemoved.fetch_add(static_cast<uint32>(pointsRemoved));
            float reduction = originalLength - path.totalLength;
            _totalLengthReduction += reduction;

            path.isOptimized = true;

            TC_LOG_DEBUG("playerbot.movement", "Optimized path: %zu -> %zu nodes, %.2f -> %.2f length",
                originalSize, path.nodes.size(), originalLength, path.totalLength);
        }
        else
        {
            _failedOptimizations.fetch_add(1);
        }

        return success;
    }

    uint32 PathOptimizer::RemoveRedundantPoints(MovementPath& path, Map* map)
    {
        if (path.nodes.size() < 3)
            return 0;

        ::std::vector<PathNode> optimized;
        optimized.reserve(path.nodes.size());

        // Always keep the start point
        optimized.push_back(path.nodes[0]);

        uint32 removed = 0;
        size_t i = 1;

        while (i < path.nodes.size() - 1)
        {
            // Check if we can skip this waypoint
    if (CanRemoveWaypoint(path, i, map))
            {
                removed++;
                i++;
            }
            else
            {
                optimized.push_back(path.nodes[i]);
                i++;
            }
        }

        // Always keep the end point
        optimized.push_back(path.nodes.back());

        path.nodes = ::std::move(optimized);
        return removed;
    }

    bool PathOptimizer::SmoothPath(MovementPath& path, Map* map, float smoothingFactor)
    {
        if (path.nodes.size() < 3)
            return false;

        smoothingFactor = ::std::clamp(smoothingFactor, 0.0f, 1.0f);

        for (uint32 iteration = 0; iteration < _smoothingIterations; ++iteration)
        {
            ::std::vector<PathNode> smoothed;
            smoothed.reserve(path.nodes.size());

            // Keep first node unchanged
            smoothed.push_back(path.nodes[0]);

            // Smooth intermediate nodes
    for (size_t i = 1; i < path.nodes.size() - 1; ++i)
            {
                Position const& prev = path.nodes[i - 1].position;
                Position const& curr = path.nodes[i].position;
                Position const& next = path.nodes[i + 1].position;

                // Calculate smoothed position
                Position smoothPos;
                smoothPos.m_positionX = curr.GetPositionX() * (1.0f - smoothingFactor) +
                    (prev.GetPositionX() + next.GetPositionX()) * 0.5f * smoothingFactor;
                smoothPos.m_positionY = curr.GetPositionY() * (1.0f - smoothingFactor) +
                    (prev.GetPositionY() + next.GetPositionY()) * 0.5f * smoothingFactor;
                smoothPos.m_positionZ = curr.GetPositionZ() * (1.0f - smoothingFactor) +
                    (prev.GetPositionZ() + next.GetPositionZ()) * 0.5f * smoothingFactor;

                // Validate the smoothed position
    if (!map || IsDirectPathValid(map, prev, smoothPos) &&
                    IsDirectPathValid(map, smoothPos, next))
                {
                    PathNode smoothedNode = path.nodes[i];
                    smoothedNode.position = smoothPos;
                    smoothedNode.isSmoothed = true;
                    smoothed.push_back(smoothedNode);
                }
                else
                {
                    // Keep original if smoothed position is invalid
                    smoothed.push_back(path.nodes[i]);
                }
            }

            // Keep last node unchanged
            smoothed.push_back(path.nodes.back());

            path.nodes = ::std::move(smoothed);
        }

        return true;
    }

    uint32 PathOptimizer::CutCorners(MovementPath& path, Map* map, float maxCutDistance)
    {
        if (path.nodes.size() < 3)
            return 0;

        uint32 cornersCut = 0;
        ::std::vector<PathNode> optimized;
        optimized.reserve(path.nodes.size());

        optimized.push_back(path.nodes[0]);

        for (size_t i = 1; i < path.nodes.size() - 1; ++i)
        {
            Position const& prev = path.nodes[i - 1].position;
            Position const& curr = path.nodes[i].position;
            Position const& next = path.nodes[i + 1].position;

            // Calculate the angle at this waypoint
            float angle1 = atan2(curr.GetPositionY() - prev.GetPositionY(),
                                curr.GetPositionX() - prev.GetPositionX());
            float angle2 = atan2(next.GetPositionY() - curr.GetPositionY(),
                                next.GetPositionX() - curr.GetPositionX());

            float angleDiff = ::std::abs(Position::NormalizeOrientation(angle2 - angle1));

            // Check if this is a sharp corner worth cutting
    if (angleDiff > _cornerCutThreshold && angleDiff < static_cast<float>(M_PI) - _cornerCutThreshold)
            {
                // Calculate cut position
                float cutFactor = 0.3f; // Cut 30% into the corner
                Position cutPos;
                cutPos.m_positionX = prev.GetPositionX() * cutFactor +
                    next.GetPositionX() * (1.0f - cutFactor);
                cutPos.m_positionY = prev.GetPositionY() * cutFactor +
                    next.GetPositionY() * (1.0f - cutFactor);
                cutPos.m_positionZ = prev.GetPositionZ() * cutFactor +
                    next.GetPositionZ() * (1.0f - cutFactor);

                // Check if cut is valid
                float cutDist = curr.GetExactDist(&cutPos);
                if (cutDist <= maxCutDistance && IsDirectPathValid(map, prev, cutPos) &&
                    IsDirectPathValid(map, cutPos, next))
                {
                    PathNode cutNode = path.nodes[i];
                    cutNode.position = cutPos;
                    cutNode.isSmoothed = true;
                    optimized.push_back(cutNode);
                    cornersCut++;
                }
                else
                {
                    optimized.push_back(path.nodes[i]);
                }
            }
            else
            {
                optimized.push_back(path.nodes[i]);
            }
        }

        optimized.push_back(path.nodes.back());
        path.nodes = ::std::move(optimized);

        return cornersCut;
    }

    bool PathOptimizer::OptimizeForMovementType(MovementPath& path, MovementGeneratorType type)
    {
        if (!path.IsValid())
            return false;

        switch (type)
        {
        case MovementGeneratorType::MOVEMENT_CHASE:
            // For chasing, prioritize direct paths
            _optimizationLevel = OPTIMIZATION_AGGRESSIVE;
            _cornerCutThreshold = 30.0f * static_cast<float>(M_PI) / 180.0f; // More aggressive corner cutting
            break;

        case MovementGeneratorType::MOVEMENT_FLEE:
            // For fleeing, prioritize quick escape over optimization
            _optimizationLevel = OPTIMIZATION_BASIC;
            break;

        case MovementGeneratorType::MOVEMENT_FOLLOW:
            // For following, use smooth paths
            _optimizationLevel = OPTIMIZATION_SMOOTH;
            break;

        case MovementGeneratorType::MOVEMENT_FORMATION:
            // For formation, maintain predictable paths
            _optimizationLevel = OPTIMIZATION_BASIC;
            _cornerCutThreshold = 10.0f * static_cast<float>(M_PI) / 180.0f; // Less aggressive
            break;

        default:
            _optimizationLevel = OPTIMIZATION_SMOOTH;
            break;
        }

        return true;
    }

    bool PathOptimizer::AdjustForObstacles(MovementPath& path, ::std::vector<Position> const& obstacles,
                                          float avoidanceRadius)
    {
        if (path.nodes.empty() || obstacles.empty())
            return false;

        bool adjusted = false;

        for (auto& node : path.nodes)
        {
            for (auto const& obstacle : obstacles)
            {
                float dist = node.position.GetExactDist(&obstacle);
                if (dist < avoidanceRadius)
                {
                    // Move waypoint away from obstacle
                    float angle = atan2(node.position.GetPositionY() - obstacle.GetPositionY(),
                                      node.position.GetPositionX() - obstacle.GetPositionX());
                    float pushDistance = avoidanceRadius - dist + 0.5f;

                    node.position.m_positionX += pushDistance * cos(angle);
                    node.position.m_positionY += pushDistance * sin(angle);
                    adjusted = true;
                }
            }
        }

        return adjusted;
    }

    bool PathOptimizer::OptimizeGroupPaths(::std::vector<MovementPath>& paths, bool maintainFormation)
    {
        if (paths.empty())
            return false;

        if (maintainFormation)
        {
            // Find the median path length
            ::std::vector<float> lengths;
            for (auto const& path : paths)
                lengths.push_back(path.totalLength);

            ::std::sort(lengths.begin(), lengths.end());
            float medianLength = lengths[lengths.size() / 2];

            // Adjust all paths to similar length for formation maintenance
    for (auto& path : paths)
            {
                if (::std::abs(path.totalLength - medianLength) > 5.0f)
                {
                    // Add delay points or adjust speed to synchronize arrival
                    float speedAdjustment = path.totalLength / medianLength;
                    for (auto& node : path.nodes)
                    {
                        node.speed *= speedAdjustment;
                    }
                }
            }
        }
        else
        {
            // Optimize each path independently
    for (auto& path : paths)
            {
                OptimizePath(path, nullptr);
            }
        }

        return true;
    }

    void PathOptimizer::GetStatistics(uint32& pathsOptimized, uint32& pointsRemoved,
                                     float& averageReduction) const
    {
        pathsOptimized = _totalPathsOptimized.load();
        pointsRemoved = _totalPointsRemoved.load();
        float totalReduction = _totalLengthReduction;
        averageReduction = pathsOptimized > 0 ? totalReduction / pathsOptimized : 0.0f;
    }

    void PathOptimizer::ResetStatistics()
    {
        _totalPathsOptimized.store(0);
        _totalPointsRemoved.store(0);
        _totalLengthReduction = 0.0f;
        _failedOptimizations.store(0);
    }

    bool PathOptimizer::IsDirectPathValid(Map* map, Position const& start, Position const& end) const
    {
        if (!map || !_checkLineOfSight)
            return true;

        // Simplified validation - just check distance
        // Real LOS checking would require PhaseShift which we don't have in this context
        float distance = start.GetExactDist(&end);
        if (distance > 100.0f) // Too far, probably not direct path
            return false;

        // Check height difference
        float heightDiff = ::std::abs(start.GetPositionZ() - end.GetPositionZ());
        if (heightDiff > 20.0f) // Too steep
            return false;

        return true;
    }

    float PathOptimizer::CalculateCurvature(Position const& prev, Position const& current,
                                           Position const& next) const
    {
        // Calculate vectors
        float v1x = current.GetPositionX() - prev.GetPositionX();
        float v1y = current.GetPositionY() - prev.GetPositionY();
        float v2x = next.GetPositionX() - current.GetPositionX();
        float v2y = next.GetPositionY() - current.GetPositionY();

        // Normalize vectors
        float len1 = sqrt(v1x * v1x + v1y * v1y);
        float len2 = sqrt(v2x * v2x + v2y * v2y);

        if (len1 < 0.001f || len2 < 0.001f)
            return 0.0f;

        v1x /= len1;
        v1y /= len1;
        v2x /= len2;
        v2y /= len2;

        // Calculate dot product (cosine of angle)
        float dot = v1x * v2x + v1y * v2y;

        // Curvature is related to the angle change
        return 1.0f - dot; // 0 = straight, 2 = 180 degree turn
    }

    void PathOptimizer::ApplyCatmullRomSmoothing(MovementPath& path, float tension)
    {
        if (path.nodes.size() < 4)
            return;

        ::std::vector<PathNode> smoothed;
        smoothed.reserve(path.nodes.size() * 2); // May add intermediate points

        smoothed.push_back(path.nodes[0]);

        for (size_t i = 0; i < path.nodes.size() - 1; ++i)
        {
            Position const& p0 = (i > 0) ? path.nodes[i - 1].position : path.nodes[i].position;
            Position const& p1 = path.nodes[i].position;
            Position const& p2 = path.nodes[i + 1].position;
            Position const& p3 = (i < path.nodes.size() - 2) ?
                path.nodes[i + 2].position : path.nodes[i + 1].position;

            // Add intermediate points using Catmull-Rom spline
            const int steps = 3;
            for (int step = 1; step < steps; ++step)
            {
                float t = static_cast<float>(step) / steps;
                float t2 = t * t;
                float t3 = t2 * t;

                // Catmull-Rom spline formula
                float x = 0.5f * ((2 * p1.GetPositionX()) +
                    (-p0.GetPositionX() + p2.GetPositionX()) * t +
                    (2 * p0.GetPositionX() - 5 * p1.GetPositionX() + 4 * p2.GetPositionX() - p3.GetPositionX()) * t2 +
                    (-p0.GetPositionX() + 3 * p1.GetPositionX() - 3 * p2.GetPositionX() + p3.GetPositionX()) * t3);

                float y = 0.5f * ((2 * p1.GetPositionY()) +
                    (-p0.GetPositionY() + p2.GetPositionY()) * t +
                    (2 * p0.GetPositionY() - 5 * p1.GetPositionY() + 4 * p2.GetPositionY() - p3.GetPositionY()) * t2 +
                    (-p0.GetPositionY() + 3 * p1.GetPositionY() - 3 * p2.GetPositionY() + p3.GetPositionY()) * t3);

                float z = 0.5f * ((2 * p1.GetPositionZ()) +
                    (-p0.GetPositionZ() + p2.GetPositionZ()) * t +
                    (2 * p0.GetPositionZ() - 5 * p1.GetPositionZ() + 4 * p2.GetPositionZ() - p3.GetPositionZ()) * t2 +
                    (-p0.GetPositionZ() + 3 * p1.GetPositionZ() - 3 * p2.GetPositionZ() + p3.GetPositionZ()) * t3);

                PathNode interpolated;
                interpolated.position.Relocate(x, y, z);
                interpolated.isSmoothed = true;
                smoothed.push_back(interpolated);
            }

            if (i < path.nodes.size() - 2)
                smoothed.push_back(path.nodes[i + 1]);
        }

        smoothed.push_back(path.nodes.back());
        path.nodes = ::std::move(smoothed);
    }

    void PathOptimizer::ApplyDouglasPeucker(MovementPath& path, float epsilon)
    {
        if (path.nodes.size() < 3)
            return;

        ::std::vector<bool> keep(path.nodes.size(), false);
        keep[0] = true;
        keep[path.nodes.size() - 1] = true;

        // Recursive simplification
        ::std::function<void(size_t, size_t)> simplify = [&](size_t start, size_t end)
        {
            float maxDist = 0.0f;
            size_t index = 0;

            for (size_t i = start + 1; i < end; ++i)
            {
                float dist = PerpendicularDistance(path.nodes[i].position,
                    path.nodes[start].position, path.nodes[end].position);
                if (dist > maxDist)
                {
                    maxDist = dist;
                    index = i;
                }
            }

            if (maxDist > epsilon)
            {
                keep[index] = true;
                simplify(start, index);
                simplify(index, end);
            }
        };

        simplify(0, path.nodes.size() - 1);

        // Keep only marked nodes
        ::std::vector<PathNode> simplified;
        for (size_t i = 0; i < path.nodes.size(); ++i)
        {
            if (keep[i])
                simplified.push_back(path.nodes[i]);
        }

        path.nodes = ::std::move(simplified);
    }

    float PathOptimizer::PerpendicularDistance(Position const& point, Position const& lineStart,
                                              Position const& lineEnd) const
    {
        float dx = lineEnd.GetPositionX() - lineStart.GetPositionX();
        float dy = lineEnd.GetPositionY() - lineStart.GetPositionY();

        float mag = sqrt(dx * dx + dy * dy);
        if (mag < 0.001f)
            return point.GetExactDist(&lineStart);

        float u = ((point.GetPositionX() - lineStart.GetPositionX()) * dx +
                  (point.GetPositionY() - lineStart.GetPositionY()) * dy) / (mag * mag);

        if (u < 0.0f)
            return point.GetExactDist(&lineStart);
        else if (u > 1.0f)
            return point.GetExactDist(&lineEnd);

        float px = lineStart.GetPositionX() + u * dx;
        float py = lineStart.GetPositionY() + u * dy;

        return sqrt(pow(point.GetPositionX() - px, 2) + pow(point.GetPositionY() - py, 2));
    }

    Position PathOptimizer::InterpolatePosition(Position const& start, Position const& end, float t) const
    {
        Position result;
        result.m_positionX = start.GetPositionX() + t * (end.GetPositionX() - start.GetPositionX());
        result.m_positionY = start.GetPositionY() + t * (end.GetPositionY() - start.GetPositionY());
        result.m_positionZ = start.GetPositionZ() + t * (end.GetPositionZ() - start.GetPositionZ());
        return result;
    }

    bool PathOptimizer::CanRemoveWaypoint(MovementPath const& path, size_t index, Map* map) const
    {
        if (index == 0 || index >= path.nodes.size() - 1)
            return false;

        Position const& prev = path.nodes[index - 1].position;
        Position const& next = path.nodes[index + 1].position;

        // Check if direct path is valid
    if (!IsDirectPathValid(map, prev, next))
            return false;

        // Check if removing this waypoint doesn't create too long segment
        float directDist = prev.GetExactDist(&next);
        if (directDist > _maxWaypointDistance)
            return false;

        // Check curvature change
    if (index > 1 && index < path.nodes.size() - 2)
        {
            float curvatureBefore = CalculateCurvature(path.nodes[index - 2].position,
                path.nodes[index - 1].position, path.nodes[index].position);
            float curvatureAfter = CalculateCurvature(path.nodes[index].position,
                path.nodes[index + 1].position, path.nodes[index + 2].position);

            // Don't remove if it would create sudden curvature change
    if (::std::abs(curvatureAfter - curvatureBefore) > 0.5f)
                return false;
        }

        return true;
    }

    float PathOptimizer::CalculateTurnRadius(float speed, float angle) const
    {
        // Simplified turn radius calculation
        // In reality, this would depend on unit's turn rate and physics
        float speedFactor = ::std::max(1.0f, speed / 7.0f); // 7.0 is normal run speed
        return speedFactor * 2.0f / sin(angle / 2.0f);
    }

    bool PathOptimizer::ValidateOptimizedPath(MovementPath const& original,
                                             MovementPath const& optimized, Map* map) const
    {
        if (optimized.nodes.empty())
            return false;

        // Check that start and end points match
    if (original.nodes.front().position.GetExactDist(&optimized.nodes.front().position) > 0.1f ||
            original.nodes.back().position.GetExactDist(&optimized.nodes.back().position) > 0.1f)
        {
            return false;
        }

        // Verify all segments are valid
    for (size_t i = 1; i < optimized.nodes.size(); ++i)
        {
            if (!IsDirectPathValid(map, optimized.nodes[i - 1].position,
                optimized.nodes[i].position))
            {
                return false;
            }
        }

        // Check that total length didn't increase significantly
    if (optimized.totalLength > original.totalLength * 1.1f)
        {
            return false;
        }

        return true;
    }
}