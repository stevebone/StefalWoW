/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITY_PLAYERBOT_MOVEMENT_TYPES_H
#define TRINITY_PLAYERBOT_MOVEMENT_TYPES_H

#include "Define.h"
#include "ObjectGuid.h"
#include "Position.h"
#include <chrono>
#include <vector>
#include <memory>
#include <optional>

namespace Playerbot
{
    // FormationType from GroupFormation.h is a different enum with different values

    // Type aliases for path representations
    using PositionVector = ::std::vector<Position>;

    /**
     * @enum MovementGeneratorType
     * @brief Types of movement behaviors available to bots
     */
    enum class MovementGeneratorType : uint8
    {
        MOVEMENT_NONE           = 0,
        MOVEMENT_IDLE           = 1,
        MOVEMENT_RANDOM         = 2,
        MOVEMENT_WAYPOINT       = 3,
        MOVEMENT_FOLLOW         = 4,
        MOVEMENT_CHASE          = 5,
        MOVEMENT_FLEE           = 6,
        MOVEMENT_FORMATION      = 7,
        MOVEMENT_PATROL         = 8,
        MOVEMENT_HOME           = 9,
        MOVEMENT_POINT          = 10,
        MOVEMENT_CHARGE         = 11,
        MOVEMENT_DISTRACT       = 12,
        MOVEMENT_ASSISTANCE     = 13,
        MOVEMENT_CUSTOM         = 14,
        MOVEMENT_MAX
    };

    /**
     * @enum MovementPriority
     * @brief Priority levels for movement generators
     */
    enum class MovementPriority : uint8
    {
        PRIORITY_NONE           = 0,
        PRIORITY_NORMAL         = 1,
        PRIORITY_DISTRACT       = 2,
        PRIORITY_ASSISTANCE     = 3,
        PRIORITY_COMBAT         = 4,
        PRIORITY_FLEE           = 5,
        PRIORITY_CRITICAL       = 6
    };

    /**
     * @enum MovementResult
     * @brief Result codes for movement operations
     */
    enum class MovementResult : uint8
    {
        MOVEMENT_SUCCESS        = 0,
        MOVEMENT_IN_PROGRESS    = 1,
        MOVEMENT_FAILED         = 2,
        MOVEMENT_CANCELLED      = 3,
        MOVEMENT_UNREACHABLE    = 4,
        MOVEMENT_INVALID_DEST   = 5,
        MOVEMENT_NO_PATH        = 6,
        MOVEMENT_STUCK          = 7
    };

    /**
     * @enum PositionType
     * @brief Types of combat positions for bot positioning
     */
    enum class PositionType : uint8
    {
        MELEE_COMBAT            = 0,  // Close combat position
        RANGED_COMBAT           = 1,  // Ranged attack position
        HEALING_POSITION        = 2,  // Healer safe position
        TANK_POSITION           = 3,  // Tank defensive position
        FLANKING                = 4,  // Flanking attack position
        BEHIND_TARGET           = 5,  // Behind target for backstab
        SAFE_DISTANCE           = 6,  // Safe distance from danger
        FORMATION               = 7,  // Formation-based position
        CUSTOM                  = 8,  // Custom defined position
        // Aliases for backward compatibility (used by PositionManager, MovementIntegration)
        RANGED_DPS              = RANGED_COMBAT,   // Alias for ranged DPS positioning
        HEALING                 = HEALING_POSITION, // Alias for healer positioning
        TANKING                 = TANK_POSITION,   // Alias for tank positioning
        KITING                  = 9                // Kiting position (maintaining distance while attacking)
    };

    /**
     * @enum TerrainType
     * @brief Terrain types for movement validation
     */
    enum class TerrainType : uint8
    {
        TERRAIN_GROUND          = 0x01,
        TERRAIN_WATER           = 0x02,
        TERRAIN_SLIME           = 0x04,
        TERRAIN_LAVA            = 0x08,
        TERRAIN_INDOOR          = 0x10,
        TERRAIN_OUTDOOR         = 0x20,
        TERRAIN_AIR             = 0x40,
        TERRAIN_VMAP_GROUND     = 0x80
    };

    // Bitwise operators for TerrainType
    inline constexpr TerrainType operator&(TerrainType lhs, TerrainType rhs)
    {
        return static_cast<TerrainType>(static_cast<uint8>(lhs) & static_cast<uint8>(rhs));
    }

    inline constexpr TerrainType operator|(TerrainType lhs, TerrainType rhs)
    {
        return static_cast<TerrainType>(static_cast<uint8>(lhs) | static_cast<uint8>(rhs));
    }

    inline constexpr TerrainType operator^(TerrainType lhs, TerrainType rhs)
    {
        return static_cast<TerrainType>(static_cast<uint8>(lhs) ^ static_cast<uint8>(rhs));
    }

    inline constexpr TerrainType operator~(TerrainType val)
    {
        return static_cast<TerrainType>(~static_cast<uint8>(val));
    }

    inline constexpr bool HasFlag(TerrainType value, TerrainType flag)
    {
        return (static_cast<uint8>(value) & static_cast<uint8>(flag)) != 0;
    }

    /**
     * @enum PathType
     * @brief Types of paths that can be generated
     */
    enum class PathType : uint8
    {
        PATHFIND_BLANK          = 0x00,
        PATHFIND_NORMAL         = 0x01,
        PATHFIND_SHORTCUT       = 0x02,
        PATHFIND_INCOMPLETE     = 0x04,
        PATHFIND_NOPATH         = 0x08,
        PATHFIND_NOT_USING_PATH = 0x10,
        PATHFIND_SHORT          = 0x20,
        PATHFIND_FARFROMPOLY    = 0x40
    };

    /**
     * @struct PathNode
     * @brief Single waypoint in a movement path
     */
    struct PathNode
    {
        Position position;
        float speed;
        uint32 delay;
        TerrainType terrain;
        bool isSmoothed;

        PathNode() : speed(0.0f), delay(0), terrain(TerrainType::TERRAIN_GROUND), isSmoothed(false) {}
        PathNode(Position const& pos, float spd = 0.0f)
            : position(pos), speed(spd), delay(0), terrain(TerrainType::TERRAIN_GROUND), isSmoothed(false) {}
    };

    /**
     * @struct MovementPath
     * @brief Complete movement path with metadata
     */
    struct MovementPath
    {
        ::std::vector<PathNode> nodes;
        PathType pathType;
        float totalLength;
        ::std::chrono::steady_clock::time_point generatedTime;
        uint32 generationCost; // CPU microseconds to generate
        bool isOptimized;

        MovementPath() : pathType(PathType::PATHFIND_BLANK), totalLength(0.0f),
                        generationCost(0), isOptimized(false) {}

        void Clear()
        {
            nodes.clear();
            pathType = PathType::PATHFIND_BLANK;
            totalLength = 0.0f;
            isOptimized = false;
        }

        bool IsValid() const { return !nodes.empty() && pathType != PathType::PATHFIND_NOPATH; }
        size_t GetNodeCount() const { return nodes.size(); }
    };

    /**
     * @struct BasicMovementRequest
     * @brief Simple request parameters for basic movement generation
     * @note For full featured movement requests, use Movement/Arbiter/MovementRequest.h
     */
    struct BasicMovementRequest
    {
        ObjectGuid targetGuid;
        Position destination;
        float speed;
        float range;
        float angle;
        MovementGeneratorType type;
        MovementPriority priority;
        bool forceDirect;
        bool allowPartial;
        uint32 maxSearchNodes;

        BasicMovementRequest() : speed(0.0f), range(0.0f), angle(0.0f),
            type(MovementGeneratorType::MOVEMENT_NONE),
            priority(MovementPriority::PRIORITY_NORMAL),
            forceDirect(false), allowPartial(false), maxSearchNodes(3000) {}
    };

    /**
     * @struct MovementState
     * @brief Current movement state of a bot
     */
    struct MovementState
    {
        MovementGeneratorType currentType;
        MovementResult lastResult;
        Position currentPosition;
        Position targetPosition;
        ObjectGuid targetGuid;
        float currentSpeed;
        uint32 currentPathNode;
        uint32 stuckCounter;
        uint32 recalcCounter;
        ::std::chrono::steady_clock::time_point lastUpdateTime;
        ::std::chrono::steady_clock::time_point lastStuckCheck;
        bool isMoving;
        bool needsRecalc;

        MovementState() : currentType(MovementGeneratorType::MOVEMENT_IDLE),
            lastResult(MovementResult::MOVEMENT_SUCCESS),
            currentSpeed(0.0f), currentPathNode(0), stuckCounter(0),
            recalcCounter(0), isMoving(false), needsRecalc(false) {}

        void Reset()
        {
            currentType = MovementGeneratorType::MOVEMENT_IDLE;
            lastResult = MovementResult::MOVEMENT_SUCCESS;
            currentSpeed = 0.0f;
            currentPathNode = 0;
            stuckCounter = 0;
            recalcCounter = 0;
            isMoving = false;
            needsRecalc = false;
            targetGuid.Clear();
        }
    };

    /**
     * @struct FormationPosition
     * @brief Position data for formation movement
     */
    struct FormationPosition
    {
        float relativeX;
        float relativeY;
        float relativeAngle;
        float followDistance;
        float followAngle;
        uint8 slot;

        FormationPosition() : relativeX(0.0f), relativeY(0.0f), relativeAngle(0.0f),
            followDistance(2.0f), followAngle(0.0f), slot(0) {}

        FormationPosition(float x, float y, float angle, uint8 slotId)
            : relativeX(x), relativeY(y), relativeAngle(angle),
            followDistance(sqrt(x*x + y*y)), followAngle(atan2(y, x)), slot(slotId) {}
    };

    // FormationType removed - use definition from AI/Combat/FormationManager.h to avoid duplication

    /**
     * @struct MovementMetrics
     * @brief Performance metrics for movement system
     */
    struct MovementMetrics
    {
        uint32 pathsGenerated;
        uint32 pathsOptimized;
        uint32 pathsCached;
        uint32 cacheHits;
        uint32 cacheMisses;
        uint32 stuckDetections;
        uint32 recalculations;
        uint64 totalPathNodes;
        uint64 totalCpuMicros;
        float averagePathLength;
        float averageNodeCount;
        float cacheHitRate;

        MovementMetrics() { Reset(); }

        void Reset()
        {
            pathsGenerated = 0;
            pathsOptimized = 0;
            pathsCached = 0;
            cacheHits = 0;
            cacheMisses = 0;
            stuckDetections = 0;
            recalculations = 0;
            totalPathNodes = 0;
            totalCpuMicros = 0;
            averagePathLength = 0.0f;
            averageNodeCount = 0.0f;
            cacheHitRate = 0.0f;
        }

        void UpdateAverages()
        {
            if (pathsGenerated > 0)
            {
                averageNodeCount = static_cast<float>(totalPathNodes) / pathsGenerated;
                averagePathLength = averagePathLength / pathsGenerated;
            }
            if (cacheHits + cacheMisses > 0)
            {
                cacheHitRate = static_cast<float>(cacheHits) / (cacheHits + cacheMisses) * 100.0f;
            }
        }
    };

    // Constants for movement system tuning
    namespace MovementConstants
    {
        // Update frequencies (milliseconds)
        constexpr uint32 UPDATE_INTERVAL_COMBAT        = 100;
        constexpr uint32 UPDATE_INTERVAL_NORMAL        = 250;
        constexpr uint32 UPDATE_INTERVAL_IDLE          = 1000;
        constexpr uint32 UPDATE_INTERVAL_FAR           = 2000;

        // Distance thresholds
        constexpr float DISTANCE_NEAR                  = 10.0f;
        constexpr float DISTANCE_MEDIUM                = 30.0f;
        constexpr float DISTANCE_FAR                   = 60.0f;
        constexpr float DISTANCE_VERY_FAR              = 100.0f;

        // Movement thresholds
        constexpr float REACHED_THRESHOLD              = 0.5f;
        constexpr float RECALC_THRESHOLD               = 5.0f;
        constexpr float STUCK_THRESHOLD                = 2.0f;
        constexpr uint32 STUCK_CHECK_INTERVAL          = 1000; // ms
        constexpr uint32 MAX_STUCK_COUNTER             = 5;

        // Path generation
        constexpr uint32 MAX_PATH_NODES                = 74;
        constexpr uint32 BOT_SMOOTH_PATH_STEPS         = 2;  // Renamed to avoid conflict with PathGenerator.h
        constexpr float PATH_OPTIMIZATION_ANGLE        = 15.0f; // degrees
        constexpr uint32 PATH_CACHE_SIZE               = 100;
        constexpr uint32 PATH_CACHE_DURATION           = 5000; // ms

        // Formation defaults
        constexpr float FORMATION_SPREAD               = 3.0f;
        constexpr float FORMATION_FOLLOW_DIST          = 5.0f;
        constexpr float FORMATION_MAX_DIST             = 20.0f;

        // Performance tuning
        constexpr uint32 MAX_BOTS_PER_UPDATE           = 50;
        constexpr uint32 PATH_GENERATION_BUDGET        = 1000; // microseconds
        constexpr float CPU_THROTTLE_THRESHOLD         = 0.1f; // 0.1% CPU per bot
    }

    // Forward declarations for cross-references
    class MovementGenerator;
    class MovementManager;
    class PathfindingAdapter;
    class PathOptimizer;
    class MovementValidator;
    class NavMeshInterface;

    using MovementGeneratorPtr = ::std::shared_ptr<MovementGenerator>;
    using MovementPathPtr = ::std::shared_ptr<MovementPath>;
}

#endif // TRINITY_PLAYERBOT_MOVEMENT_TYPES_H