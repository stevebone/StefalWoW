/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * MOVEMENT REQUEST
 *
 * Represents a request for bot movement from any PlayerBot subsystem.
 *
 * Design Pattern: Value Object
 * Purpose: Encapsulate all information needed to execute a movement request
 *
 * Key Features:
 * - Type-safe variant for different movement types
 * - Spatial-temporal hashing for fast deduplication
 * - Source system tracking for diagnostics
 * - Thread ID tracking for thread-safety verification
 *
 * Usage:
 *   MovementRequest req(PlayerBotMovementPriority::BOSS_MECHANIC, "Avoiding void zone");
 *   req.SetPointMovement(safePosition, true, 0.0f, std::nullopt);
 *   arbiter->RequestMovement(req);
 */

#ifndef PLAYERBOT_MOVEMENT_REQUEST_H
#define PLAYERBOT_MOVEMENT_REQUEST_H

#include "Define.h"
#include "MovementPriorityMapper.h"
#include "Position.h"
#include "ObjectGuid.h"
#include "Optional.h"
#include "Duration.h"
#include "MovementDefines.h"
#include <string>
#include <variant>
#include <chrono>

namespace Playerbot
{

/**
 * Movement request type
 *
 * Corresponds to MotionMaster API methods:
 * - POINT      → MovePoint()
 * - CHASE      → MoveChase()
 * - FOLLOW     → MoveFollow()
 * - IDLE       → MoveIdle()
 * - JUMP       → MoveJump()
 * - CHARGE     → MoveCharge()
 * - KNOCKBACK  → MoveKnockbackFrom()
 * - CUSTOM     → LaunchMoveSpline()
 * - RANDOM     → MoveRandom() [NEW: TrinityCore 12.0 - natural idle wandering]
 * - PATH       → MovePath() [NEW: TrinityCore 12.0 - waypoint-based navigation]
 */
enum class MovementRequestType : uint8
{
    NONE = 0,
    POINT,          // Move to specific position
    CHASE,          // Chase target unit (melee combat)
    FOLLOW,         // Follow target unit (group following)
    IDLE,           // Stop moving, stand idle
    JUMP,           // Jump to position
    CHARGE,         // Charge to position (warrior, etc.)
    KNOCKBACK,      // Knockback from origin
    CUSTOM,         // Custom spline movement
    RANDOM,         // [NEW] Wander randomly around a point (natural idle behavior)
    PATH            // [NEW] Follow a waypoint path (quest routes, dungeon pathing)
};

/**
 * Point movement parameters (MovePoint)
 */
struct PointMovementParams
{
    Position targetPos;                         // Target position
    bool generatePath = true;                   // Use pathfinding
    Optional<float> finalOrient;                // Final orientation
    Optional<float> speed;                      // Movement speed override
    Optional<float> closeEnoughDistance;        // Arrival tolerance

    bool operator==(PointMovementParams const& other) const;
};

/**
 * Chase movement parameters (MoveChase)
 */
struct ChaseMovementParams
{
    ObjectGuid targetGuid;                      // Target to chase
    Optional<ChaseRange> range;                 // Chase range
    Optional<ChaseAngle> angle;                 // Chase angle

    bool operator==(ChaseMovementParams const& other) const;
};

/**
 * Follow movement parameters (MoveFollow)
 */
struct FollowMovementParams
{
    ObjectGuid targetGuid;                      // Target to follow
    float distance;                             // Follow distance
    Optional<ChaseAngle> angle;                 // Follow angle
    Optional<Milliseconds> duration;            // Follow duration (if timed)

    bool operator==(FollowMovementParams const& other) const;
};

/**
 * Jump movement parameters (MoveJump)
 */
struct JumpMovementParams
{
    Position targetPos;                         // Jump destination
    float speedXY;                              // Horizontal speed
    float speedZ;                               // Vertical speed
    uint32 eventId = 0;                         // Event ID

    bool operator==(JumpMovementParams const& other) const;
};

/**
 * Idle movement parameters (MoveIdle)
 */
struct IdleMovementParams
{
    // No parameters needed for idle
    bool operator==(IdleMovementParams const&) const { return true; }
};

/**
 * Random wandering movement parameters (MoveRandom)
 *
 * NEW: Leverages TrinityCore 12.0's MoveRandom() support for players.
 * Creates natural idle behavior by wandering around a center point.
 *
 * Use Cases:
 * - Town idle: Bots wander near mailbox/AH/bank naturally
 * - Waiting for group: Wander near meeting point
 * - Guard duty: Patrol randomly within defense perimeter
 * - Fishing spots: Move around fishing area
 */
struct RandomMovementParams
{
    Position centerPos;                         // Center point to wander around
    float wanderDistance = 5.0f;                // Radius of wander area (yards)
    Optional<Milliseconds> duration;            // How long to wander (empty = until interrupted)
    Optional<float> speed;                      // Movement speed override
    bool forceWalk = true;                      // Walk instead of run (more natural)

    bool operator==(RandomMovementParams const& other) const
    {
        return centerPos.GetExactDist(&other.centerPos) < 0.5f &&
               std::abs(wanderDistance - other.wanderDistance) < 0.5f &&
               forceWalk == other.forceWalk;
    }
};

/**
 * Waypoint path movement parameters (MovePath)
 *
 * NEW: Leverages TrinityCore 12.0's MovePath() support for players.
 * Allows bots to follow predefined waypoint paths for smooth navigation.
 *
 * Use Cases:
 * - Quest routes: Pre-defined paths to quest objectives
 * - Dungeon pathing: Follow tank's path through instance
 * - Patrol behavior: Guard bots patrolling an area
 * - Gathering routes: Mining/herbalism farming paths
 * - Boss mechanics: Execute precise movement patterns
 */
struct PathMovementParams
{
    uint32 pathId = 0;                          // Waypoint path ID from database/memory
    bool repeatable = false;                    // Loop the path
    Optional<Milliseconds> duration;            // Path duration limit
    Optional<float> speed;                      // Movement speed override
    bool forceWalk = false;                     // Walk instead of run
    Optional<::std::pair<Milliseconds, Milliseconds>> waitTimeAtEnd;  // Wait at path end
    Optional<float> wanderAtEnds;               // Wander distance at waypoints
    bool exactSpline = false;                   // Follow exact spline vs pathfinding
    bool generatePath = true;                   // Use pathfinding between waypoints

    bool operator==(PathMovementParams const& other) const
    {
        return pathId == other.pathId &&
               repeatable == other.repeatable &&
               exactSpline == other.exactSpline;
    }
};

/**
 * Movement request
 *
 * Immutable value object representing a single movement request.
 * Created by PlayerBot subsystems and submitted to MovementArbiter.
 *
 * Thread-Safe: Yes (immutable after construction)
 * Copyable: Yes
 * Movable: Yes
 */
class TC_GAME_API MovementRequest
{
public:
    // ========================================================================
    // CONSTRUCTION
    // ========================================================================

    /**
     * Construct empty request
     * Must call Set*Movement() before use
     */
    explicit MovementRequest(PlayerBotMovementPriority priority, ::std::string reason);

    /**
     * Construct point movement request
     */
    static MovementRequest MakePointMovement(
        PlayerBotMovementPriority priority,
        Position const& targetPos,
        bool generatePath = true,
        Optional<float> finalOrient = {},
        Optional<float> speed = {},
        Optional<float> closeEnoughDistance = {},
        ::std::string reason = "",
        ::std::string sourceSystem = "");

    /**
     * Construct chase movement request
     */
    static MovementRequest MakeChaseMovement(
        PlayerBotMovementPriority priority,
        ObjectGuid targetGuid,
        Optional<ChaseRange> range = {},
        Optional<ChaseAngle> angle = {},
        ::std::string reason = "",
        ::std::string sourceSystem = "");

    /**
     * Construct follow movement request
     */
    static MovementRequest MakeFollowMovement(
        PlayerBotMovementPriority priority,
        ObjectGuid targetGuid,
        float distance,
        Optional<ChaseAngle> angle = {},
        Optional<Milliseconds> duration = {},
        ::std::string reason = "",
        ::std::string sourceSystem = "");

    /**
     * Construct idle movement request
     */
    static MovementRequest MakeIdleMovement(
        PlayerBotMovementPriority priority,
        ::std::string reason = "",
        ::std::string sourceSystem = "");

    /**
     * Construct jump movement request
     */
    static MovementRequest MakeJumpMovement(
        PlayerBotMovementPriority priority,
        Position const& targetPos,
        float speedXY,
        float speedZ,
        uint32 eventId = 0,
        ::std::string reason = "",
        ::std::string sourceSystem = "");

    /**
     * Construct random wandering movement request
     *
     * NEW: Uses TrinityCore 12.0's MoveRandom() for natural idle behavior.
     *
     * @param priority Movement priority
     * @param centerPos Center point to wander around
     * @param wanderDistance Radius of wander area (default 5 yards)
     * @param duration How long to wander (empty = until interrupted)
     * @param forceWalk Walk instead of run (default true for natural movement)
     * @param reason Debug description
     * @param sourceSystem Source system name
     */
    static MovementRequest MakeRandomMovement(
        PlayerBotMovementPriority priority,
        Position const& centerPos,
        float wanderDistance = 5.0f,
        Optional<Milliseconds> duration = {},
        bool forceWalk = true,
        ::std::string reason = "",
        ::std::string sourceSystem = "");

    /**
     * Construct waypoint path movement request
     *
     * NEW: Uses TrinityCore 12.0's MovePath() for waypoint-based navigation.
     *
     * @param priority Movement priority
     * @param pathId Waypoint path ID
     * @param repeatable Loop the path
     * @param duration Path duration limit
     * @param speed Movement speed override
     * @param forceWalk Walk instead of run
     * @param reason Debug description
     * @param sourceSystem Source system name
     */
    static MovementRequest MakePathMovement(
        PlayerBotMovementPriority priority,
        uint32 pathId,
        bool repeatable = false,
        Optional<Milliseconds> duration = {},
        Optional<float> speed = {},
        bool forceWalk = false,
        ::std::string reason = "",
        ::std::string sourceSystem = "");

    // ========================================================================
    // ACCESSORS
    // ========================================================================

    uint64 GetRequestId() const { return _requestId; }
    PlayerBotMovementPriority GetPriority() const { return _priority; }
    MovementRequestType GetType() const { return _type; }
    ::std::string const& GetReason() const { return _reason; }
    ::std::string const& GetSourceSystem() const { return _sourceSystem; }
    uint32 GetSourceThreadId() const { return _sourceThreadId; }
    uint32 GetTimestamp() const { return _timestamp; }
    uint32 GetExpectedDuration() const { return _expectedDuration; }
    bool CanBeInterrupted() const { return _allowInterrupt; }

    // Type-specific parameter getters (throws if wrong type)
    PointMovementParams const& GetPointParams() const;
    ChaseMovementParams const& GetChaseParams() const;
    FollowMovementParams const& GetFollowParams() const;
    JumpMovementParams const& GetJumpParams() const;
    IdleMovementParams const& GetIdleParams() const;
    RandomMovementParams const& GetRandomParams() const;
    PathMovementParams const& GetPathParams() const;

    // Safe parameter getters (returns Optional)
    Optional<PointMovementParams> TryGetPointParams() const;
    Optional<ChaseMovementParams> TryGetChaseParams() const;
    Optional<FollowMovementParams> TryGetFollowParams() const;
    Optional<RandomMovementParams> TryGetRandomParams() const;
    Optional<PathMovementParams> TryGetPathParams() const;

    // ========================================================================
    // SETTERS (Fluent Interface)
    // ========================================================================

    MovementRequest& SetSourceSystem(::std::string sourceSystem);
    MovementRequest& SetExpectedDuration(uint32 durationMs);
    MovementRequest& SetAllowInterrupt(bool allow);

    // ========================================================================
    // DEDUPLICATION
    // ========================================================================

    /**
     * Get spatial-temporal hash for fast duplicate detection
     *
     * Hash is based on:
     * - Request type
     * - Priority
     * - Spatial position (5-yard grid for POINT)
     * - Target GUID (for CHASE/FOLLOW)
     *
     * Returns: 64-bit hash suitable for map lookup
     *
     * Performance: O(1)
     * Thread-Safe: Yes (const method)
     */
    uint64 GetSpatialTemporalHash() const;

    /**
     * Check if this request is a duplicate of another
     *
     * Considers:
     * - Same type
     * - Same priority
     * - Spatial proximity (< 0.3 yards for POINT)
     * - Same target (for CHASE/FOLLOW)
     *
     * Returns: true if duplicate
     *
     * Performance: O(1)
     * Thread-Safe: Yes (const methods)
     */
    bool IsDuplicateOf(MovementRequest const& other) const;

    // ========================================================================
    // DEBUG
    // ========================================================================

    /**
     * Get debug string representation
     */
    ::std::string ToString() const;

    /**
     * Comparison operators for testing
     */
    bool operator==(MovementRequest const& other) const;
    bool operator!=(MovementRequest const& other) const { return !(*this == other); }

private:
    // ========================================================================
    // DATA MEMBERS
    // ========================================================================

    // Identity
    uint64 _requestId;                          // Unique ID (generated)
    PlayerBotMovementPriority _priority;        // Request priority
    MovementRequestType _type;                  // Request type
    ::std::string _reason;                        // Debug description

    // Source tracking
    ::std::string _sourceSystem;                  // System that made request
    uint32 _sourceThreadId;                     // Thread ID (for thread safety checks)

    // Timing
    uint32 _timestamp;                          // GameTime::GetGameTimeMS() when created
    uint32 _expectedDuration;                   // Expected duration (ms)
    bool _allowInterrupt;                       // Can be interrupted?

    // Type-specific parameters
    using Params = ::std::variant<
        ::std::monostate,                         // Uninitialized
        PointMovementParams,
        ChaseMovementParams,
        FollowMovementParams,
        JumpMovementParams,
        IdleMovementParams,
        RandomMovementParams,                     // NEW: TrinityCore 12.0 MoveRandom()
        PathMovementParams                        // NEW: TrinityCore 12.0 MovePath()
    >;

    Params _params;

    // Request ID generator
    static uint64 GenerateRequestId();

    // Helper: Get current thread ID
    static uint32 GetCurrentThreadId();
};

} // namespace Playerbot

#endif // PLAYERBOT_MOVEMENT_REQUEST_H
