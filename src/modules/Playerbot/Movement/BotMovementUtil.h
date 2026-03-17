/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * BotMovementUtil - Centralized Movement Deduplication Utility
 *
 * CRITICAL: ALL bot movement MUST use these functions to prevent
 * the infinite movement cancellation bug (60+ MovePoint calls/second)
 *
 * ROOT CAUSE: Direct MovePoint() calls every frame cancel previous movement
 * SOLUTION: Check if already moving before issuing new movement command
 */

#pragma once

#include "Player.h"
#include "MotionMaster.h"
#include "Position.h"
#include "MoveSpline.h"
#include "Map.h"
#include "PhaseShift.h"

namespace Playerbot
{

/**
 * BotMovementUtil - Centralized movement functions with deduplication
 *
 * USAGE:
 *   Instead of: bot->GetMotionMaster()->MovePoint(0, destination);
 *   Use:        BotMovementUtil::MoveToPosition(bot, destination);
 *
 * This prevents movement command spam and allows movements to complete.
 *
 * Z-VALUE CORRECTION:
 *   All position calculation functions MUST use CorrectPositionToGround()
 *   to prevent bots from falling through the ground or hovering above terrain.
 *   This is CRITICAL for proper bot navigation.
 */
class TC_GAME_API BotMovementUtil
{
public:
    // ========================================================================
    // Z-VALUE CORRECTION FUNCTIONS
    // ========================================================================

    /**
     * Correct a position's Z coordinate to actual ground level
     *
     * CRITICAL: This function MUST be called after calculating any position
     * that will be used for bot movement. Failure to correct Z values causes
     * bots to fall through the ground or hover above terrain.
     *
     * @param bot Player bot (used for map and phase information)
     * @param pos Position to correct (modified in place)
     * @param heightOffset Small offset above ground to prevent clipping (default 0.5f)
     * @return true if Z was corrected successfully, false if correction failed
     *
     * USAGE EXAMPLE:
     *   Position destination;
     *   destination.Relocate(targetX, targetY, someZ);
     *   BotMovementUtil::CorrectPositionToGround(bot, destination);
     *   // destination.m_positionZ is now at actual ground level
     */
    static bool CorrectPositionToGround(Player* bot, Position& pos, float heightOffset = 0.5f);

    /**
     * Correct a position's Z coordinate using a specific map
     *
     * Use this overload when you have a Map pointer but no bot, or when
     * you need to check a position on a different map than the bot's current map.
     *
     * @param map Map to use for height calculation
     * @param phaseShift Phase information for visibility
     * @param pos Position to correct (modified in place)
     * @param heightOffset Small offset above ground to prevent clipping (default 0.5f)
     * @return true if Z was corrected successfully, false if correction failed
     */
    static bool CorrectPositionToGroundWithMap(Map* map, PhaseShift const& phaseShift,
                                                Position& pos, float heightOffset = 0.5f);

    /**
     * Get the ground height at a specific position
     *
     * @param bot Player bot (used for map and phase information)
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Starting Z for height search (searches downward from this point)
     * @return Ground height, or INVALID_HEIGHT if no valid ground found
     */
    static float GetGroundHeight(Player* bot, float x, float y, float z);

    /**
     * Check if a position has valid ground beneath it
     *
     * @param bot Player bot (used for map and phase information)
     * @param pos Position to check
     * @param maxHeightDifference Maximum acceptable height difference (default 10.0f)
     * @return true if position has valid ground within acceptable range
     */
    static bool HasValidGround(Player* bot, Position const& pos, float maxHeightDifference = 10.0f);

    // ========================================================================
    // MOVEMENT FUNCTIONS
    // ========================================================================

    /**
     * Move bot to specific position with deduplication
     *
     * @param bot Player bot to move
     * @param destination Target position
     * @param pointId Movement point ID (default 0)
     * @param minDistanceChange Only re-issue movement if destination changed by this much (default 0.5 yards)
     * @return true if movement initiated or already in progress
     */
    static bool MoveToPosition(Player* bot, Position const& destination, uint32 pointId = 0, float minDistanceChange = 0.5f);

    /**
     * Move bot to target (position only, no chasing)
     *
     * @param bot Player bot to move
     * @param target Target to move to
     * @param pointId Movement point ID (default 0)
     * @param minDistanceChange Only re-issue if destination changed by this much
     * @return true if movement initiated or already in progress
     */
    static bool MoveToTarget(Player* bot, WorldObject* target, uint32 pointId = 0, float minDistanceChange = 0.5f);

    /**
     * Move bot to within specified distance of unit (for interaction, quest givers, etc.)
     *
     * @param bot Player bot to move
     * @param unit Target unit to approach
     * @param distance Desired distance from unit (e.g., 5.0f for quest giver interaction)
     * @param pointId Movement point ID (default 0)
     * @return true if movement initiated, already in range, or already moving there
     */
    static bool MoveToUnit(Player* bot, Unit* unit, float distance, uint32 pointId = 0);

    /**
     * Chase target at specific distance with deduplication
     *
     * @param bot Player bot to move
     * @param target Target to chase
     * @param distance Distance to maintain from target
     * @return true if chase initiated or already in progress
     */
    static bool ChaseTarget(Player* bot, Unit* target, float distance);

    /**
     * Stop bot movement immediately
     *
     * @param bot Player bot to stop
     */
    static void StopMovement(Player* bot);

    /**
     * Check if bot is currently moving
     *
     * @param bot Player bot to check
     * @return true if bot has active movement
     */
    static bool IsMoving(Player* bot);

    /**
     * Check if bot is moving to specific destination
     *
     * @param bot Player bot to check
     * @param destination Target position
     * @param tolerance Distance tolerance (default 1.0 yard)
     * @return true if bot is already moving to this destination
     */
    static bool IsMovingToDestination(Player* bot, Position const& destination, float tolerance = 1.0f);

    // ========================================================================
    // NEW: TrinityCore 12.0 Movement Features
    // ========================================================================
    // These methods leverage the new MoveRandom() and MovePath() player support
    // added in TrinityCore 12.0 (commits 12743dd0e7, 1db1a0e57f)
    // ========================================================================

    /**
     * Start random wandering around current position
     *
     * NEW: Uses TrinityCore 12.0's MoveRandom() for players.
     * Creates natural idle behavior by wandering around a center point.
     *
     * Use Cases:
     * - Town idle: Bots wander near mailbox/AH/bank naturally
     * - Waiting for group: Wander near meeting point
     * - Guard duty: Patrol randomly within defense perimeter
     * - Fishing spots: Move around fishing area
     *
     * @param bot Player bot to move
     * @param wanderDistance Radius of wander area in yards (default 5.0)
     * @param duration How long to wander (default 30 seconds, empty = until interrupted)
     * @param forceWalk Walk instead of run (default true for natural movement)
     * @return true if random movement initiated
     *
     * Example:
     *   // Natural town idle behavior
     *   BotMovementUtil::MoveRandomAround(bot, 5.0f, 30s, true);
     *
     *   // Guard patrol behavior (larger radius)
     *   BotMovementUtil::MoveRandomAround(bot, 15.0f, {}, true);
     */
    static bool MoveRandomAround(Player* bot, float wanderDistance = 5.0f,
        Optional<Milliseconds> duration = Milliseconds(30000), bool forceWalk = true);

    /**
     * Start random wandering around a specific position
     *
     * Same as MoveRandomAround() but with custom center point.
     *
     * @param bot Player bot to move
     * @param centerPos Center point to wander around
     * @param wanderDistance Radius of wander area in yards
     * @param duration How long to wander
     * @param forceWalk Walk instead of run
     * @return true if random movement initiated
     *
     * Example:
     *   // Wander around a quest giver while waiting
     *   Position questGiverPos = questGiver->GetPosition();
     *   BotMovementUtil::MoveRandomAroundPosition(bot, questGiverPos, 8.0f, 60s, true);
     */
    static bool MoveRandomAroundPosition(Player* bot, Position const& centerPos,
        float wanderDistance = 5.0f, Optional<Milliseconds> duration = Milliseconds(30000),
        bool forceWalk = true);

    /**
     * Follow a waypoint path by ID
     *
     * NEW: Uses TrinityCore 12.0's MovePath() for players.
     * Allows bots to follow predefined waypoint paths for smooth navigation.
     *
     * Use Cases:
     * - Quest routes: Pre-defined paths to quest objectives
     * - Dungeon pathing: Follow tank's path through instance
     * - Patrol behavior: Guard bots patrolling an area
     * - Gathering routes: Mining/herbalism farming paths
     * - Boss mechanics: Execute precise movement patterns
     *
     * @param bot Player bot to move
     * @param pathId Waypoint path ID from waypoint_path table or BotWaypointPathManager
     * @param repeatable Loop the path when reaching the end
     * @param forceWalk Walk instead of run
     * @param speed Optional speed override
     * @return true if path movement initiated
     *
     * Example:
     *   // Follow a quest route path
     *   BotMovementUtil::MoveAlongPath(bot, 12345, false, false);
     *
     *   // Patrol path that loops
     *   BotMovementUtil::MoveAlongPath(bot, 12346, true, true);
     */
    static bool MoveAlongPath(Player* bot, uint32 pathId, bool repeatable = false,
        bool forceWalk = false, Optional<float> speed = {});

    /**
     * Check if bot is currently in random wandering mode
     *
     * @param bot Player bot to check
     * @return true if bot is randomly wandering
     */
    static bool IsWandering(Player* bot);

    /**
     * Check if bot is currently following a waypoint path
     *
     * @param bot Player bot to check
     * @return true if bot is following a path
     */
    static bool IsFollowingPath(Player* bot);

    /**
     * Stop random wandering or path following
     *
     * Specifically clears RANDOM_MOTION_TYPE or WAYPOINT_MOTION_TYPE
     * without affecting other movement types.
     *
     * @param bot Player bot to stop
     */
    static void StopWanderingOrPath(Player* bot);
};

} // namespace Playerbot
