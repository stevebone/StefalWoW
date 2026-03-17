/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * BotMovementUtil - Centralized Movement Deduplication Implementation
 *
 * Uses MotionMaster::MovePoint() with deduplication to prevent movement spam.
 * NOTE: We cannot use MoveSplineInit directly because it's NOT thread-safe -
 * bot AI runs on worker threads but MoveSplineInit manipulates unit->movespline
 * which is accessed by Unit::Update() on the main thread, causing crashes.
 */

#include "BotMovementUtil.h"
#include "Battleground.h"
#include "Log.h"
#include "Map.h"
#include "PhaseShift.h"

namespace Playerbot
{

// ============================================================================
// Z-VALUE CORRECTION FUNCTIONS
// ============================================================================
// CRITICAL: These functions prevent bots from falling through the ground or
// hovering above terrain. ALL position calculations MUST use these functions
// before passing positions to movement systems.
// ============================================================================

bool BotMovementUtil::CorrectPositionToGround(Player* bot, Position& pos, float heightOffset)
{
    // Validate input
    if (!bot)
    {
        TC_LOG_ERROR("module.playerbot.movement",
            "CorrectPositionToGround: NULL bot pointer");
        return false;
    }

    // CRITICAL: Use FindMap() instead of GetMap() to avoid ASSERT crash
    // GetMap() has ASSERT(m_currMap) which crashes if map is null
    // This can happen during loading, logout, or map transitions
    Map* map = bot->FindMap();
    if (!map)
    {
        TC_LOG_DEBUG("module.playerbot.movement",
            "CorrectPositionToGround: Bot {} has no map, cannot correct Z",
            bot->GetName());
        return false;
    }

    // Use bot's phase shift for proper visibility/collision
    return CorrectPositionToGroundWithMap(map, bot->GetPhaseShift(), pos, heightOffset);
}

bool BotMovementUtil::CorrectPositionToGroundWithMap(Map* map, PhaseShift const& phaseShift,
                                                     Position& pos, float heightOffset)
{
    // Validate input
    if (!map)
    {
        TC_LOG_ERROR("module.playerbot.movement",
            "CorrectPositionToGroundWithMap: NULL map pointer");
        return false;
    }

    // Store original Z for logging
    float originalZ = pos.GetPositionZ();

    // Get ground height at this X/Y position
    // Parameters:
    //   - phaseShift: visibility/phase info
    //   - x, y: position to check
    //   - z: starting height for downward search
    //   - checkVMap: true to use vmaps for indoor areas (default)
    //   - maxSearchDist: how far down to search (100.0f covers most terrain)
    float groundZ = map->GetHeight(phaseShift,
                                   pos.GetPositionX(),
                                   pos.GetPositionY(),
                                   pos.GetPositionZ() + 10.0f,  // Search from slightly above current Z
                                   true,                         // Use VMaps
                                   100.0f);                      // Max search distance

    // Check if we got a valid height
    if (groundZ <= INVALID_HEIGHT)
    {
        // No valid ground found - this can happen over water, void, etc.
        // Try searching from higher up in case we're in a deep hole
        groundZ = map->GetHeight(phaseShift,
                                 pos.GetPositionX(),
                                 pos.GetPositionY(),
                                 pos.GetPositionZ() + 50.0f,  // Search from much higher
                                 true,
                                 200.0f);                      // Larger search distance

        if (groundZ <= INVALID_HEIGHT)
        {
            TC_LOG_DEBUG("module.playerbot.movement",
                "CorrectPositionToGroundWithMap: No valid ground at ({:.1f}, {:.1f}, {:.1f}) - keeping original Z",
                pos.GetPositionX(), pos.GetPositionY(), originalZ);
            return false;
        }
    }

    // Apply the corrected Z with height offset
    float newZ = groundZ + heightOffset;

    // Calculate the correction amount for diagnostics
    float correction = std::abs(newZ - originalZ);

    // Only log if the correction was significant (> 1 yard)
    if (correction > 1.0f)
    {
        TC_LOG_DEBUG("module.playerbot.movement",
            "CorrectPositionToGroundWithMap: Z corrected by {:.1f}yd ({:.1f} -> {:.1f}) at ({:.1f}, {:.1f})",
            correction, originalZ, newZ, pos.GetPositionX(), pos.GetPositionY());
    }

    // Apply the correction
    pos.m_positionZ = newZ;

    return true;
}

float BotMovementUtil::GetGroundHeight(Player* bot, float x, float y, float z)
{
    // Validate input
    if (!bot)
        return INVALID_HEIGHT;

    // Use FindMap() instead of GetMap() to avoid ASSERT crash
    Map* map = bot->FindMap();
    if (!map)
        return INVALID_HEIGHT;

    // Get height using bot's phase shift
    return map->GetHeight(bot->GetPhaseShift(), x, y, z + 10.0f, true, 100.0f);
}

bool BotMovementUtil::HasValidGround(Player* bot, Position const& pos, float maxHeightDifference)
{
    // Validate input
    if (!bot)
        return false;

    // Get ground height at position
    float groundZ = GetGroundHeight(bot, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ());

    // Check if we got a valid height
    if (groundZ <= INVALID_HEIGHT)
        return false;

    // Check if the ground is within acceptable range
    float heightDiff = std::abs(pos.GetPositionZ() - groundZ);
    return heightDiff <= maxHeightDifference;
}

// ============================================================================
// MOVEMENT FUNCTIONS
// ============================================================================

bool BotMovementUtil::MoveToPosition(Player* bot, Position const& destination, uint32 pointId, float minDistanceChange)
{
    if (!bot)
        return false;

    // CRITICAL: Bot must be in world before any movement operations
    if (!bot->IsInWorld())
    {
        TC_LOG_DEBUG("module.playerbot", "⚠️ BotMovement: Bot {} not in world, skipping movement", bot->GetName());
        return false;
    }

    MotionMaster* mm = bot->GetMotionMaster();
    if (!mm)
    {
        TC_LOG_ERROR("module.playerbot", "BotMovementUtil::MoveToPosition: Bot {} has NULL MotionMaster", bot->GetName());
        return false;
    }

    // MINE/CAVE FIX: Use 3D distance to properly detect vertical distance
    // Previously used 2D distance which caused bots to think they were "at destination"
    // when standing at mine entrance but spawn is directly below at different Z
    float distToDestination2D = bot->GetExactDist2d(destination.GetPositionX(), destination.GetPositionY());
    float distToDestination3D = bot->GetExactDist(destination);
    float zDifference = std::abs(bot->GetPositionZ() - destination.GetPositionZ());

    // DIAGNOSTIC: Log movement attempts (DEBUG level to avoid spam)
    TC_LOG_DEBUG("module.playerbot.movement", "🔍 MoveToPosition: Bot {} dist2D={:.1f} dist3D={:.1f} zDiff={:.1f} minDist={:.1f} dest=({:.1f},{:.1f},{:.1f})",
                 bot->GetName(), distToDestination2D, distToDestination3D, zDifference, minDistanceChange,
                 destination.GetPositionX(), destination.GetPositionY(), destination.GetPositionZ());

    // MINE/CAVE FIX: Use 3D distance for "already at destination" check
    // This ensures bots will still move if there's significant vertical distance
    // even if horizontal distance is small (e.g., mine entrance above spawn inside)
    if (distToDestination3D < minDistanceChange)
    {
        TC_LOG_DEBUG("module.playerbot.movement", "✅ BotMovement: Bot {} already at destination (3D dist {:.1f}yd < {:.1f})",
                     bot->GetName(), distToDestination3D, minDistanceChange);
        return true;
    }

    // CRITICAL SAFETY CHECK: Re-verify bot is still valid before accessing MotionMaster
    // Threading issue: Bot could have been removed from world between initial check and here
    if (!bot->IsInWorld() || !bot->GetSession())
    {
        TC_LOG_DEBUG("module.playerbot.movement", "⚠️ BotMovement: Bot became invalid during MoveToPosition, aborting");
        return false;
    }

    // Check if bot is already moving
    MovementGeneratorType currentMoveType = mm->GetCurrentMovementGeneratorType(MOTION_SLOT_ACTIVE);

    // ========================================================================
    // CRITICAL FIX: Do NOT interrupt CHASE_MOTION_TYPE!
    // ========================================================================
    // When the bot is chasing a target (combat), calling MovePoint() interrupts
    // the chase and causes stuttering as SoloCombatStrategy immediately re-issues
    // MoveChase(). This creates rapid oscillation between CHASE and POINT motion.
    //
    // During combat, SoloCombatStrategy is the ONLY system that should control
    // movement. If MoveToPosition is called during chase, we should NOT interrupt.
    // ========================================================================
    if (currentMoveType == CHASE_MOTION_TYPE)
    {
        TC_LOG_DEBUG("module.playerbot.movement", "⚠️ BotMovement: Bot {} in CHASE mode - NOT INTERRUPTING chase to move to position (let combat strategy handle movement)",
                     bot->GetName());
        return false;  // Indicate that we did NOT start movement - chase continues
    }

    // Similarly, don't interrupt FOLLOW_MOTION_TYPE (following group leader)
    // CRITICAL FIX: EXCEPT in battlegrounds where BG tactics take priority over following!
    // Without this exception, bots in groups just follow the master and ignore BG objectives.
    if (currentMoveType == FOLLOW_MOTION_TYPE)
    {
        // Check if bot is in a battleground - BG tactics should override follow behavior
        if (bot->InBattleground())
        {
            ::Battleground* bg = bot->GetBattleground();
            if (bg && bg->GetStatus() == STATUS_IN_PROGRESS)
            {
                TC_LOG_DEBUG("module.playerbot.movement",
                    "🎮 BotMovement: Bot {} in FOLLOW mode BUT in active BG - INTERRUPTING follow for BG tactics",
                    bot->GetName());
                // Clear follow motion so BG movement can take over
                mm->Clear(MOTION_SLOT_ACTIVE);
                // Continue to execute the BG movement below
            }
            else
            {
                TC_LOG_DEBUG("module.playerbot.movement",
                    "⚠️ BotMovement: Bot {} in FOLLOW mode, BG not in progress - NOT INTERRUPTING",
                    bot->GetName());
                return false;
            }
        }
        else
        {
            TC_LOG_DEBUG("module.playerbot.movement",
                "⚠️ BotMovement: Bot {} in FOLLOW mode - NOT INTERRUPTING follow to move to position",
                bot->GetName());
            return false;  // Indicate that we did NOT start movement - follow continues
        }
    }

    // If already moving via spline, check if we should interrupt
    if (currentMoveType == POINT_MOTION_TYPE || currentMoveType == EFFECT_MOTION_TYPE)
    {
        // Check if the spline is still active and heading somewhere reasonable
        if (bot->movespline && !bot->movespline->Finalized())
        {
            // BUG FIX: Check if current spline is going to the CORRECT destination!
            // The spline might be going somewhere completely different (e.g., follow target)
            // We need to interrupt and redirect if the destination doesn't match
            G3D::Vector3 const& splineDestination = bot->movespline->FinalDestination();

            // Skip zero destination check (means spline not properly initialized)
            if (splineDestination != G3D::Vector3::zero())
            {
                float splineDestDist = std::sqrt(
                    (splineDestination.x - destination.GetPositionX()) * (splineDestination.x - destination.GetPositionX()) +
                    (splineDestination.y - destination.GetPositionY()) * (splineDestination.y - destination.GetPositionY()) +
                    (splineDestination.z - destination.GetPositionZ()) * (splineDestination.z - destination.GetPositionZ())
                );

                // If spline destination is close to requested destination (< 10 yards), let it continue
                if (splineDestDist < 10.0f)
                {
                    TC_LOG_DEBUG("module.playerbot.movement", "⏭ BotMovement: Bot {} spline ACTIVE going to correct destination (diff={:.1f}yd) - {:.1f}yd to dest (3D)",
                                 bot->GetName(), splineDestDist, distToDestination3D);
                    return true;
                }
                else
                {
                    // Spline is going to WRONG destination - must interrupt and redirect!
                    TC_LOG_DEBUG("module.playerbot.movement", "🔄 BotMovement: Bot {} spline going WRONG DIRECTION! Spline dest ({:.1f},{:.1f},{:.1f}) is {:.1f}yd from requested ({:.1f},{:.1f},{:.1f}) - INTERRUPTING",
                                 bot->GetName(),
                                 splineDestination.x, splineDestination.y, splineDestination.z, splineDestDist,
                                 destination.GetPositionX(), destination.GetPositionY(), destination.GetPositionZ());
                    // Fall through to start new movement
                }
            }
            else
            {
                TC_LOG_DEBUG("module.playerbot.movement", "⚠️ BotMovement: Bot {} spline has zero destination, will restart",
                             bot->GetName());
            }
        }
        else
        {
            TC_LOG_DEBUG("module.playerbot.movement", "⚠️ BotMovement: Bot {} spline FINALIZED or NULL (moveType={}), will restart - {:.1f}yd to destination (3D)",
                         bot->GetName(), static_cast<int>(currentMoveType), distToDestination3D);
        }
    }
    else
    {
        TC_LOG_DEBUG("module.playerbot.movement", "📍 BotMovement: Bot {} not in POINT/EFFECT motion (moveType={}), will start new movement",
                     bot->GetName(), static_cast<int>(currentMoveType));
    }

    // Start new movement via MotionMaster
    // NOTE: We previously used MoveSplineInit directly for smoother movement, but that caused
    // ASSERTION FAILED: Initialized() crashes in MoveSpline::updateState because MoveSplineInit
    // is NOT thread-safe - it directly manipulates unit->movespline which is also accessed by
    // Unit::Update() on the main thread.
    //
    // MotionMaster::MovePoint() is the safer approach, and we maintain deduplication above
    // by checking if a spline is already active before calling this.
    TC_LOG_DEBUG("module.playerbot.movement", "🚶 BotMovement: Bot {} STARTING MOVEMENT to ({:.2f},{:.2f},{:.2f}) - {:.1f}yd (3D)",
                 bot->GetName(), destination.GetPositionX(), destination.GetPositionY(), destination.GetPositionZ(),
                 distToDestination3D);

    // Use MotionMaster for thread-safe movement initiation
    // The deduplication check above prevents the "60+ MovePoint calls/second" bug
    mm->MovePoint(pointId, destination.GetPositionX(), destination.GetPositionY(), destination.GetPositionZ());

    return true;
}

bool BotMovementUtil::MoveToTarget(Player* bot, WorldObject* target, uint32 pointId, float minDistanceChange)
{
    if (!bot || !target)
        return false;

    Position destination;
    destination.Relocate(target->GetPositionX(), target->GetPositionY(), target->GetPositionZ());

    // CRITICAL FIX: Correct Z to actual ground level
    // Target's Z may be different from ground level at bot's approach position
    // (e.g., target flying, on a different elevation, or Z from database not matching terrain)
    CorrectPositionToGround(bot, destination);

    return MoveToPosition(bot, destination, pointId, minDistanceChange);
}

bool BotMovementUtil::MoveToUnit(Player* bot, Unit* unit, float distance, uint32 pointId)
{
    if (!bot || !unit)
        return false;

    // Check if already within desired distance
    float currentDistance = bot->GetExactDist(unit);
    if (currentDistance <= distance)
    {
        TC_LOG_DEBUG("module.playerbot", "✅ BotMovement: Bot {} already within {:.1f}yd of {} (current: {:.1f}yd)",
                     bot->GetName(), distance, unit->GetName(), currentDistance);
        return true;
    }

    // Calculate position at desired distance from unit
    // Use angle from bot to unit for approach vector
    float angle = bot->GetAbsoluteAngle(unit);

    Position destination;
    // Calculate position that is 'distance' yards from unit, on the line between bot and unit
    // NOTE: We initially use unit's Z, but it will be corrected to ground level below
    destination.Relocate(
        unit->GetPositionX() - distance * std::cos(angle),
        unit->GetPositionY() - distance * std::sin(angle),
        unit->GetPositionZ()
    );

    // CRITICAL FIX: Correct Z to actual ground level at the calculated position
    // The calculated position may be over terrain that's at a completely different Z level
    // than the unit (e.g., unit on a hill, calculated position is over a cliff)
    // Without this correction, bots can fall through the ground or hover in mid-air
    CorrectPositionToGround(bot, destination);

    TC_LOG_DEBUG("module.playerbot", "🚶 BotMovement: Bot {} approaching {} to within {:.1f}yd at ({:.2f},{:.2f},{:.2f})",
                 bot->GetName(), unit->GetName(), distance,
                 destination.GetPositionX(), destination.GetPositionY(), destination.GetPositionZ());

    return MoveToPosition(bot, destination, pointId);
}

bool BotMovementUtil::ChaseTarget(Player* bot, Unit* target, float distance)
{
    if (!bot || !target)
        return false;

    // CRITICAL: Verify bot is still in world (threading safety)
    if (!bot->IsInWorld() || !bot->GetSession())
        return false;

    MotionMaster* mm = bot->GetMotionMaster();
    if (!mm)
        return false;

    // Check if already chasing this target at this distance
    if (mm->GetCurrentMovementGeneratorType(MOTION_SLOT_ACTIVE) == CHASE_MOTION_TYPE)
    {
        float currentDistance = bot->GetExactDist(target);
        float tolerance = 2.0f;

        if (std::abs(currentDistance - distance) <= tolerance)
        {
            TC_LOG_DEBUG("module.playerbot", "⏭ BotMovement: Bot {} already chasing at optimal distance", bot->GetName());
            return true;
        }
    }

    // Start or update chase
    TC_LOG_DEBUG("module.playerbot", "🏃 BotMovement: Bot {} chasing {} at {:.1f}yd",
                 bot->GetName(), target->GetName(), distance);
    mm->MoveChase(target, distance);
    return true;
}

void BotMovementUtil::StopMovement(Player* bot)
{
    if (!bot)
        return;

    bot->StopMoving();

    if (MotionMaster* mm = bot->GetMotionMaster())
        mm->Clear();

    TC_LOG_DEBUG("module.playerbot", "⏹ BotMovement: Bot {} stopped movement", bot->GetName());
}

bool BotMovementUtil::IsMoving(Player* bot)
{
    if (!bot)
        return false;

    // CRITICAL: Verify bot is still in world (threading safety)
    if (!bot->IsInWorld())
        return false;

    // Check spline state first (most accurate)
    if (bot->movespline && !bot->movespline->Finalized())
        return true;

    // Fall back to MotionMaster check
    MotionMaster* mm = bot->GetMotionMaster();
    if (!mm)
        return false;

    MovementGeneratorType type = mm->GetCurrentMovementGeneratorType(MOTION_SLOT_ACTIVE);
    return (type == POINT_MOTION_TYPE || type == CHASE_MOTION_TYPE || type == FOLLOW_MOTION_TYPE || type == EFFECT_MOTION_TYPE);
}

bool BotMovementUtil::IsMovingToDestination(Player* bot, Position const& destination, float tolerance)
{
    if (!bot)
        return false;

    // Check if spline is active
    if (!bot->movespline || bot->movespline->Finalized())
        return false;

    // Check if destination is close to where we're heading
    float distToDestination = bot->GetExactDist2d(destination.GetPositionX(), destination.GetPositionY());
    return (distToDestination <= tolerance);
}

// ============================================================================
// NEW: TrinityCore 12.0 Movement Features - Implementation
// ============================================================================
// These methods leverage the new MoveRandom() and MovePath() player support
// added in TrinityCore 12.0 (commits 12743dd0e7, 1db1a0e57f)
// ============================================================================

bool BotMovementUtil::MoveRandomAround(Player* bot, float wanderDistance,
    Optional<Milliseconds> duration, bool forceWalk)
{
    if (!bot)
        return false;

    // CRITICAL: Bot must be in world before any movement operations
    if (!bot->IsInWorld())
    {
        TC_LOG_DEBUG("module.playerbot.movement",
            "MoveRandomAround: Bot {} not in world, skipping random movement", bot->GetName());
        return false;
    }

    // Use bot's current position as center
    Position centerPos;
    centerPos.Relocate(bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ());

    return MoveRandomAroundPosition(bot, centerPos, wanderDistance, duration, forceWalk);
}

bool BotMovementUtil::MoveRandomAroundPosition(Player* bot, Position const& centerPos,
    float wanderDistance, Optional<Milliseconds> duration, bool forceWalk)
{
    if (!bot)
        return false;

    // CRITICAL: Bot must be in world before any movement operations
    if (!bot->IsInWorld())
    {
        TC_LOG_DEBUG("module.playerbot.movement",
            "MoveRandomAroundPosition: Bot {} not in world, skipping random movement", bot->GetName());
        return false;
    }

    MotionMaster* mm = bot->GetMotionMaster();
    if (!mm)
    {
        TC_LOG_ERROR("module.playerbot.movement",
            "MoveRandomAroundPosition: Bot {} has NULL MotionMaster", bot->GetName());
        return false;
    }

    // Validate wander distance
    if (wanderDistance < 1.0f)
        wanderDistance = 1.0f;
    if (wanderDistance > 50.0f)
        wanderDistance = 50.0f;  // Reasonable max to prevent extreme wandering

    // Correct the center position Z to ground level
    Position correctedCenter = centerPos;
    CorrectPositionToGround(bot, correctedCenter);

    // Determine speed selection mode
    MovementWalkRunSpeedSelectionMode speedMode = forceWalk
        ? MovementWalkRunSpeedSelectionMode::ForceWalk
        : MovementWalkRunSpeedSelectionMode::Default;

    // Check if already doing random movement near this center
    MovementGeneratorType currentMoveType = mm->GetCurrentMovementGeneratorType(MOTION_SLOT_ACTIVE);
    if (currentMoveType == RANDOM_MOTION_TYPE)
    {
        // Already wandering - check if center is similar
        float distFromCenter = bot->GetExactDist2d(correctedCenter.GetPositionX(), correctedCenter.GetPositionY());
        if (distFromCenter <= wanderDistance)
        {
            TC_LOG_DEBUG("module.playerbot.movement",
                "MoveRandomAroundPosition: Bot {} already wandering near center ({:.1f}yd away)",
                bot->GetName(), distFromCenter);
            return true;
        }
    }

    // Don't interrupt important movement types
    if (currentMoveType == CHASE_MOTION_TYPE ||
        currentMoveType == FOLLOW_MOTION_TYPE ||
        currentMoveType == POINT_MOTION_TYPE)
    {
        TC_LOG_DEBUG("module.playerbot.movement",
            "MoveRandomAroundPosition: Bot {} has active movement (type={}), not starting wander",
            bot->GetName(), static_cast<int>(currentMoveType));
        return false;
    }

    // Use TrinityCore 12.0's MoveRandom() for players
    // NEW: This now works for players (previously creature-only)
    TC_LOG_DEBUG("module.playerbot.movement",
        "MoveRandomAroundPosition: Bot {} starting random wander (center={:.1f},{:.1f},{:.1f}, radius={:.1f}yd, walk={})",
        bot->GetName(),
        correctedCenter.GetPositionX(), correctedCenter.GetPositionY(), correctedCenter.GetPositionZ(),
        wanderDistance, forceWalk ? "yes" : "no");

    // First teleport to center if needed (only if far from center)
    float distToCenter = bot->GetExactDist(correctedCenter);
    if (distToCenter > wanderDistance * 2)
    {
        // Too far from center - move there first
        TC_LOG_DEBUG("module.playerbot.movement",
            "MoveRandomAroundPosition: Bot {} is {:.1f}yd from center, moving there first",
            bot->GetName(), distToCenter);
        mm->MovePoint(0, correctedCenter.GetPositionX(), correctedCenter.GetPositionY(),
            correctedCenter.GetPositionZ());
        return true;
    }

    // Start random movement using TrinityCore 12.0 API
    mm->MoveRandom(wanderDistance, duration, Optional<float>{}, speedMode);

    return true;
}

bool BotMovementUtil::MoveAlongPath(Player* bot, uint32 pathId, bool repeatable,
    bool forceWalk, Optional<float> speed)
{
    if (!bot)
        return false;

    // CRITICAL: Bot must be in world before any movement operations
    if (!bot->IsInWorld())
    {
        TC_LOG_DEBUG("module.playerbot.movement",
            "MoveAlongPath: Bot {} not in world, skipping path movement", bot->GetName());
        return false;
    }

    // Path ID 0 is invalid
    if (pathId == 0)
    {
        TC_LOG_ERROR("module.playerbot.movement",
            "MoveAlongPath: Bot {} - invalid pathId 0", bot->GetName());
        return false;
    }

    MotionMaster* mm = bot->GetMotionMaster();
    if (!mm)
    {
        TC_LOG_ERROR("module.playerbot.movement",
            "MoveAlongPath: Bot {} has NULL MotionMaster", bot->GetName());
        return false;
    }

    // Don't interrupt important movement types
    MovementGeneratorType currentMoveType = mm->GetCurrentMovementGeneratorType(MOTION_SLOT_ACTIVE);
    if (currentMoveType == CHASE_MOTION_TYPE || currentMoveType == FOLLOW_MOTION_TYPE)
    {
        TC_LOG_DEBUG("module.playerbot.movement",
            "MoveAlongPath: Bot {} has active chase/follow (type={}), not starting path",
            bot->GetName(), static_cast<int>(currentMoveType));
        return false;
    }

    // Check if already on the same path
    if (currentMoveType == WAYPOINT_MOTION_TYPE)
    {
        TC_LOG_DEBUG("module.playerbot.movement",
            "MoveAlongPath: Bot {} already on a waypoint path", bot->GetName());
        // Could check if same path ID, but for now just let it continue
        return true;
    }

    // Determine speed selection mode
    MovementWalkRunSpeedSelectionMode speedMode = forceWalk
        ? MovementWalkRunSpeedSelectionMode::ForceWalk
        : MovementWalkRunSpeedSelectionMode::Default;

    TC_LOG_DEBUG("module.playerbot.movement",
        "MoveAlongPath: Bot {} starting path {} (repeatable={}, walk={})",
        bot->GetName(), pathId, repeatable ? "yes" : "no", forceWalk ? "yes" : "no");

    // Use TrinityCore 12.0's MovePath() for players
    // NEW: This now works for players (previously creature-only)
    mm->MovePath(pathId, repeatable,
        Optional<Milliseconds>{},                  // duration
        speed,                                     // speed override
        speedMode,                                 // walk/run mode
        Optional<::std::pair<Milliseconds, Milliseconds>>{},  // wait time at end
        Optional<float>{},                         // wander at ends
        Optional<bool>{},                          // follow backwards
        Optional<bool>{},                          // exact spline
        true);                                     // generate path

    return true;
}

bool BotMovementUtil::IsWandering(Player* bot)
{
    if (!bot || !bot->IsInWorld())
        return false;

    MotionMaster* mm = bot->GetMotionMaster();
    if (!mm)
        return false;

    return mm->GetCurrentMovementGeneratorType(MOTION_SLOT_ACTIVE) == RANDOM_MOTION_TYPE;
}

bool BotMovementUtil::IsFollowingPath(Player* bot)
{
    if (!bot || !bot->IsInWorld())
        return false;

    MotionMaster* mm = bot->GetMotionMaster();
    if (!mm)
        return false;

    return mm->GetCurrentMovementGeneratorType(MOTION_SLOT_ACTIVE) == WAYPOINT_MOTION_TYPE;
}

void BotMovementUtil::StopWanderingOrPath(Player* bot)
{
    if (!bot || !bot->IsInWorld())
        return;

    MotionMaster* mm = bot->GetMotionMaster();
    if (!mm)
        return;

    MovementGeneratorType currentMoveType = mm->GetCurrentMovementGeneratorType(MOTION_SLOT_ACTIVE);

    if (currentMoveType == RANDOM_MOTION_TYPE)
    {
        TC_LOG_DEBUG("module.playerbot.movement",
            "StopWanderingOrPath: Bot {} stopping random movement", bot->GetName());
        mm->Remove(RANDOM_MOTION_TYPE, MOTION_SLOT_ACTIVE);
    }
    else if (currentMoveType == WAYPOINT_MOTION_TYPE)
    {
        TC_LOG_DEBUG("module.playerbot.movement",
            "StopWanderingOrPath: Bot {} stopping waypoint path", bot->GetName());
        mm->Remove(WAYPOINT_MOTION_TYPE, MOTION_SLOT_ACTIVE);
    }
}

} // namespace Playerbot
