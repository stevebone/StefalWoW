/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PositionManager.h"
#include "GameTime.h"
#include "Player.h"
#include "Unit.h"
#include "Map.h"
#include "MapDefines.h"
#include "Log.h"
#include "Group.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "PathGenerator.h"
#include "MoveSpline.h"
#include "PhasingHandler.h"
#include "MotionMaster.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "PhaseShift.h"
#include "SharedDefines.h"
#include "../../Spatial/SpatialGridManager.h"
#include "../../Spatial/SpatialGridQueryHelpers.h"  // PHASE 5B: Thread-safe helpers
#include "ObjectAccessor.h"
#include "../BotAI.h"
#include "Core/PlayerBotHelpers.h"

namespace Playerbot
{

PositionManager::PositionManager(Player* bot, BotThreatManager* threatManager)
    : _bot(bot), _threatManager(threatManager), _updateInterval(DEFAULT_UPDATE_INTERVAL),
      _lastUpdate(0), _positionTolerance(POSITION_TOLERANCE), _maxCandidates(MAX_CANDIDATES),
      _lastZoneUpdate(0), _lastMovePointTime(0)
{
    // CRITICAL: Do NOT access _bot->GetName() in constructor!
    // Bot may not be fully in world yet during GameSystemsManager::Initialize(),
    // and Player::m_name is not initialized, causing ACCESS_VIOLATION in fmt::buffer::append.
    // Logging with bot name deferred to first UpdatePosition() call.

    if (!_threatManager)
    {
        TC_LOG_ERROR("playerbot", "PositionManager: ThreatManager is null (bot ptr: {})",
                     _bot ? "valid" : "null");
        return;
    }

    // NOTE: Debug logging deferred - bot name not safe to access during construction
}

PositionMovementResult PositionManager::UpdatePosition(const MovementContext& context)
{
    auto startTime = ::std::chrono::steady_clock::now();
    PositionMovementResult result;
    // No lock needed - position data is per-bot instance data

    try
    {
        // FIX #3: SPELL CASTING COORDINATION - Don't move while casting
    if (_bot->IsNonMeleeSpellCast(false))
        {
            result.failureReason = "Bot is casting, movement would interrupt spell";
            TC_LOG_DEBUG("playerbot.position", "⏸ Bot {} - Movement blocked, currently casting",
                         _bot->GetName());
            return result;
        }

        uint32 currentTime = GameTime::GetGameTimeMS();
        if (currentTime - _lastUpdate < _updateInterval && !context.emergencyMode)
        {
            result.failureReason = "Update interval not reached";
            return result;
        }

        _lastUpdate = currentTime;
        UpdateAoEZones(currentTime);

        Position currentPos = _bot->GetPosition();
        result.currentPosition = currentPos;

        PositionInfo currentPosInfo = EvaluatePosition(currentPos, context);

        if (!context.emergencyMode && currentPosInfo.score >= 80.0f && currentPosInfo.priority >= MovementPriority::PRIORITY_COMBAT)
        {
            result.success = true;
            result.targetPosition = currentPos;
            result.priority = MovementPriority::PRIORITY_NORMAL;
            return result;
        }

        if (context.emergencyMode || IsInDangerZone(currentPos))
        {
            return HandleEmergencyMovement(context);
        }

        PositionMovementResult optimalResult = FindOptimalPosition(context);
        if (optimalResult.success)
        {
            float movementDistance = currentPos.GetExactDist(&optimalResult.targetPosition);
            if (movementDistance > _positionTolerance)
            {
                return ExecuteMovement(optimalResult.targetPosition, optimalResult.priority);
            }
        }

        result.success = true;
        result.targetPosition = currentPos;
        result.priority = MovementPriority::PRIORITY_NORMAL;
    }
    catch (const ::std::exception& e)
    {
        result.success = false;
        result.failureReason = "Exception during position update: " + ::std::string(e.what());
        TC_LOG_ERROR("playerbot.position", "Exception in UpdatePosition for bot {}: {}", _bot->GetName(), e.what());
    }

    auto endTime = ::std::chrono::steady_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime);
    TrackPerformance(duration, "UpdatePosition");

    return result;
}

PositionMovementResult PositionManager::FindOptimalPosition(const MovementContext& context)
{
    PositionMovementResult result;
    ::std::vector<Position> candidates = GenerateCandidatePositions(context);
    if (candidates.empty())
    {
        result.failureReason = "No candidate positions generated";
        return result;
    }

    ::std::vector<PositionInfo> evaluatedPositions = EvaluatePositions(candidates, context);
    if (evaluatedPositions.empty())
    {
        result.failureReason = "No valid positions after evaluation";
        return result;
    }

    ::std::sort(evaluatedPositions.begin(), evaluatedPositions.end(), ::std::greater<PositionInfo>());

    PositionInfo bestPosition = evaluatedPositions[0];
    result.success = true;
    result.targetPosition = bestPosition.position;
    result.priority = bestPosition.priority;
    result.estimatedTime = EstimateMovementTime(_bot->GetPosition(), bestPosition.position);
    result.pathDistance = _bot->GetExactDist(&bestPosition.position);
    TC_LOG_DEBUG("playerbot.position", "Found optimal position for bot {} at ({:.2f}, {:.2f}, {:.2f}) with score {:.2f}",
               _bot->GetName(), bestPosition.position.GetPositionX(), bestPosition.position.GetPositionY(),
               bestPosition.position.GetPositionZ(), bestPosition.score);

    return result;
}

PositionMovementResult PositionManager::ExecuteMovement(const Position& targetPos, MovementPriority priority)
{
    PositionMovementResult result;
    result.targetPosition = targetPos;
    result.priority = priority;

    if (!CanReachPosition(targetPos))
    {
        result.failureReason = "Target position unreachable";
        return result;
    }

    Position currentPos = _bot->GetPosition();
    float distance = currentPos.GetExactDist(&targetPos);

    if (distance <= _positionTolerance)
    {
        result.success = true;
        result.pathDistance = 0.0f;
        result.estimatedTime = 0.0f;
        return result;
    }

    ::std::vector<Position> waypoints = CalculateWaypoints(currentPos, targetPos);
    result.waypoints = waypoints;
    result.pathDistance = 0.0f;

    for (size_t i = 1; i < waypoints.size(); ++i)
    {
        result.pathDistance += waypoints[i-1].GetExactDist(&waypoints[i]);
    }

    result.estimatedTime = EstimateMovementTime(currentPos, targetPos);

    if (priority <= MovementPriority::PRIORITY_CRITICAL)
    {
        result.requiresSprint = true;
    }

    float heightDiff = ::std::abs(targetPos.GetPositionZ() - currentPos.GetPositionZ());
    if (heightDiff > 3.0f)
    {
        result.requiresJump = true;
    }

    // FIX #1: DUPLICATE MOVEMENT PREVENTION - Check if already moving to same position
    // This prevents the infinite movement cancellation bug (60+ MovePoint calls/second)
    uint32 currentTime = GameTime::GetGameTimeMS();
    float distanceToLastTarget = _lastTargetPosition.GetExactDist(&targetPos);

    if (distanceToLastTarget < _positionTolerance && (currentTime - _lastMovePointTime) < 500)
    {
        // Already moving to same destination - don't re-issue command within 500ms
        result.success = true;
        result.failureReason = "Already moving to target position";
        TC_LOG_DEBUG("playerbot.position", "⏭ Bot {} - Duplicate movement prevented, already moving to ({:.2f}, {:.2f}, {:.2f})",
                     _bot->GetName(), targetPos.GetPositionX(), targetPos.GetPositionY(), targetPos.GetPositionZ());

        return result;
    }

    // Issue new movement command with validated pathfinding
    if (BotAI* ai = GetBotAI(_bot))
    {
        if (!ai->MoveTo(targetPos, true))
        {
            // Fallback to legacy if validation fails
            _bot->GetMotionMaster()->MovePoint(0, targetPos.GetPositionX(), targetPos.GetPositionY(), targetPos.GetPositionZ());
        }
    }
    else
    {
        // Non-bot player - use standard movement
        _bot->GetMotionMaster()->MovePoint(0, targetPos.GetPositionX(), targetPos.GetPositionY(), targetPos.GetPositionZ());
    }

    // FIX #5: SPEED CONTROL SYSTEM - Apply sprint for critical/emergency movement
    if (result.requiresSprint)
    {
        // Increase movement speed for urgent repositioning
        // Note: TrinityCore handles speed through auras/spells, not direct modification
        // This is a marker for future sprint ability integration
        TC_LOG_DEBUG("playerbot.position", " Bot {} - Sprint required for urgent movement (priority: {})",
                     _bot->GetName(), static_cast<uint8>(priority));

        // Future: Trigger sprint ability here if available
        // For now, just log the requirement - ClassAI should handle sprint abilities
    }

    // Update tracking state
    _lastTargetPosition = targetPos;
    _lastMovePointTime = currentTime;

    result.success = true;
    _metrics.movementCommands++;

    TC_LOG_DEBUG("playerbot.position", "Bot {} moving to position ({:.2f}, {:.2f}, {:.2f}), distance: {:.2f}",
               _bot->GetName(), targetPos.GetPositionX(), targetPos.GetPositionY(), targetPos.GetPositionZ(), distance);

    return result;
}

PositionInfo PositionManager::EvaluatePosition(const Position& pos, const MovementContext& context)
{
    auto startTime = ::std::chrono::steady_clock::now();
    PositionInfo info;
    info.position = pos;
    info.evaluationTime = GameTime::GetGameTimeMS();

    if (!ValidatePosition(pos, context.validationFlags))
    {
        info.score = 0.0f;
        info.priority = MovementPriority::PRIORITY_NONE;
        return info;
    }

    float totalScore = 0.0f;

    totalScore += CalculateDistanceScore(pos, context) * context.weights.distanceWeight;
    totalScore += CalculateSafetyScore(pos, context) * context.weights.safetyWeight;
    totalScore += CalculateLineOfSightScore(pos, context) * context.weights.losWeight;
    totalScore += CalculateAngleScore(pos, context) * context.weights.angleWeight;
    totalScore += CalculateGroupScore(pos, context) * context.weights.groupWeight;
    totalScore += CalculateEscapeScore(pos, context) * context.weights.escapeWeight;

    info.score = ::std::max(0.0f, totalScore);
    info.distanceToTarget = context.target ? pos.GetExactDist(context.target) : 0.0f;
    info.hasLineOfSight = context.target ? _bot->IsWithinLOSInMap(context.target) : true;
    info.isOptimalRange = (info.distanceToTarget >= context.preferredRange * 0.8f &&
                          info.distanceToTarget <= context.preferredRange * 1.2f);
    info.safetyRating = CalculateSafetyScore(pos, context);
    info.movementCost = CalculateMovementCost(_bot->GetPosition(), pos);
    if (info.score >= 90.0f)
        info.priority = MovementPriority::PRIORITY_COMBAT;
    else if (info.score >= 70.0f)
        info.priority = MovementPriority::PRIORITY_FLEE;
    else if (info.score >= 50.0f)
        info.priority = MovementPriority::PRIORITY_NORMAL;
    else
        info.priority = MovementPriority::PRIORITY_NONE;

    if (IsInDangerZone(pos))
    {
        info.priority = MovementPriority::PRIORITY_CRITICAL;
        info.score *= 0.1f;
    }

    auto endTime = ::std::chrono::steady_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime);
    TrackPerformance(duration, "EvaluatePosition");

    _metrics.positionEvaluations++;
    return info;
}

::std::vector<PositionInfo> PositionManager::EvaluatePositions(const ::std::vector<Position>& positions, const MovementContext& context)
{
    ::std::vector<PositionInfo> results;
    results.reserve(positions.size());

    for (const Position& pos : positions)
    {
        PositionInfo info = EvaluatePosition(pos, context);
        if (info.score > 0.0f)
        {
            results.push_back(info);
        }
    }

    return results;
}

::std::vector<Position> PositionManager::GenerateCandidatePositions(const MovementContext& context)
{
    ::std::vector<Position> candidates;

    if (!context.target)
    {
        return candidates;
    }

    Position targetPos = context.target->GetPosition();
    // Variables for potential position calculations
    (void)context.maxRange; // Silence unused warning
    switch (context.desiredType)
    {
        case PositionType::MELEE_COMBAT:
            candidates = GenerateCircularPositions(targetPos, 4.0f, 8);
            break;

        case PositionType::RANGED_DPS:
            {
                // QW-3 FIX: Pre-allocate for both rings (12+12=24 positions)
                candidates.reserve(24);
                ::std::vector<Position> innerRing = GenerateCircularPositions(targetPos, context.preferredRange * 0.8f, 12);
                ::std::vector<Position> outerRing = GenerateCircularPositions(targetPos, context.preferredRange * 1.2f, 12);
                candidates.insert(candidates.end(), innerRing.begin(), innerRing.end());
                candidates.insert(candidates.end(), outerRing.begin(), outerRing.end());
            }
            break;

        case PositionType::HEALING:
            {
                if (!context.groupMembers.empty())
                {
                    Position groupCenter = PositionUtils::CalculateGroupCenter(context.groupMembers);
                    candidates = GenerateCircularPositions(groupCenter, 20.0f, 16);
                }
                else
                {
                    candidates = GenerateCircularPositions(targetPos, 25.0f, 12);
                }
            }
            break;

        case PositionType::KITING:
            {
                Position currentPos = _bot->GetPosition();
                float angle = PositionUtils::CalculateAngleBetween(targetPos, currentPos);
                candidates = GenerateArcPositions(targetPos, context.preferredRange, angle - static_cast<float>(M_PI) / 3.0f, angle + static_cast<float>(M_PI) / 3.0f, 8);
            }
            break;

        case PositionType::FLANKING:
            {
                float targetAngle = context.target->GetOrientation();
                float leftFlankAngle = PositionUtils::NormalizeAngle(targetAngle + static_cast<float>(M_PI) / 2.0f);
                float rightFlankAngle = PositionUtils::NormalizeAngle(targetAngle - static_cast<float>(M_PI) / 2.0f);

                candidates.push_back(PositionUtils::CalculatePositionAtAngle(targetPos, 6.0f, leftFlankAngle));
                candidates.push_back(PositionUtils::CalculatePositionAtAngle(targetPos, 6.0f, rightFlankAngle));
            }
            break;

        case PositionType::TANKING:
            {
                float targetAngle = context.target->GetOrientation();
                float frontAngle = PositionUtils::NormalizeAngle(targetAngle + static_cast<float>(M_PI));
                candidates = GenerateArcPositions(targetPos, 5.0f, frontAngle - static_cast<float>(M_PI) / 6.0f, frontAngle + static_cast<float>(M_PI) / 6.0f, 6);
            }
            break;

        default:
            candidates = GenerateCircularPositions(targetPos, context.preferredRange, 12);
            break;
    }

    if (candidates.size() > _maxCandidates)
    {
        candidates.resize(_maxCandidates);
    }

    return candidates;
}

Position PositionManager::FindMeleePosition(Unit* target, bool preferBehind)
{
    if (!target)
        return _bot->GetPosition();

    Position targetPos = target->GetPosition();
    float targetAngle = target->GetOrientation();
    if (preferBehind)
    {
        float behindAngle = PositionUtils::NormalizeAngle(targetAngle + static_cast<float>(M_PI));
        return PositionUtils::CalculatePositionAtAngle(targetPos, 3.5f, behindAngle);
    }
    else
    {
        ::std::vector<Position> candidates = GenerateCircularPositions(targetPos, 4.0f, 8);

        MovementContext context;
        context.bot = _bot;
        context.target = target;
        context.desiredType = PositionType::MELEE_COMBAT;
        context.validationFlags = PositionValidation::COMBAT;

        PositionInfo bestPosition;
        for (const Position& pos : candidates)
        {
            PositionInfo info = EvaluatePosition(pos, context);
            if (info.score > bestPosition.score)
            {
                bestPosition = info;
            }
        }

        return bestPosition.position;
    }
}

Position PositionManager::FindRangedPosition(Unit* target, float preferredRange)
{
    if (!target)
        return _bot->GetPosition();

    Position targetPos = target->GetPosition();
    ::std::vector<Position> candidates = GenerateCircularPositions(targetPos, preferredRange, 16);
    MovementContext context;
    context.bot = _bot;
    context.target = target;
    context.desiredType = PositionType::RANGED_DPS;
    context.preferredRange = preferredRange;
    context.validationFlags = PositionValidation::SAFE;

    PositionInfo bestPosition;
    for (const Position& pos : candidates)
    {
        PositionInfo info = EvaluatePosition(pos, context);
        if (info.score > bestPosition.score)
        {
            bestPosition = info;
        }
    }

    return bestPosition.position;
}

Position PositionManager::FindKitingPosition(Unit* threat, float minDistance)
{
    if (!threat)
        return _bot->GetPosition();

    Position currentPos = _bot->GetPosition();
    Position threatPos = threat->GetPosition();

    float currentDistance = currentPos.GetExactDist(&threatPos);
    if (currentDistance >= minDistance * 1.2f)
    {
        return currentPos;
    }

    float escapeAngle = PositionUtils::CalculateAngleBetween(threatPos, currentPos);

    ::std::vector<Position> escapePositions;
    for (int i = -2; i <= 2; ++i)
    {
        float angle = PositionUtils::NormalizeAngle(escapeAngle + (i * static_cast<float>(M_PI) / 6.0f));
        Position escapePos = PositionUtils::CalculatePositionAtAngle(threatPos, minDistance * 1.5f, angle);
        escapePositions.push_back(escapePos);
    }

    MovementContext context;
    context.bot = _bot;
    context.target = threat;
    context.desiredType = PositionType::KITING;
    context.preferredRange = minDistance * 1.5f;
    context.validationFlags = PositionValidation::BASIC;

    PositionInfo bestPosition;
    for (const Position& pos : escapePositions)
    {
        PositionInfo info = EvaluatePosition(pos, context);
        if (info.score > bestPosition.score)
        {
            bestPosition = info;
        }
    }

    return bestPosition.position;
}

Position PositionManager::FindTankPosition(Unit* target)
{
    if (!target)
        return _bot->GetPosition();

    // Tank should be in front of the target, facing it away from group
    float angle = target->GetOrientation();
    // Position slightly to the side to avoid frontal cone attacks
    angle += 0.2f; // Slight offset

    // Tank positioning distance (5 yards - melee range)
    float distance = 5.0f;

    Position tankPos = PositionUtils::CalculatePositionAtAngle(target->GetPosition(), distance, angle);

    // Validate position is reachable
    if (!ValidatePosition(tankPos, PositionValidation::BASIC))
    {
        // Try alternative angles if primary position invalid
        for (float offsetAngle : {-0.2f, 0.4f, -0.4f})
        {
            Position altPos = PositionUtils::CalculatePositionAtAngle(target->GetPosition(), distance, angle + offsetAngle);
            if (ValidatePosition(altPos, PositionValidation::BASIC))
                return altPos;
        }
    }

    return tankPos;
}

Position PositionManager::FindHealerPosition(const std::vector<Player*>& groupMembers)
{
    if (!_bot)
        return Position();

    // Healers should be at medium range (18 yards), central to the group
    float healerDistance = 18.0f;

    // Find the group center
    Position groupCenter;
    if (!groupMembers.empty())
    {
        float sumX = 0.0f, sumY = 0.0f, sumZ = 0.0f;
        uint32 count = 0;

        for (Player* member : groupMembers)
        {
            if (member && member != _bot)
            {
                sumX += member->GetPositionX();
                sumY += member->GetPositionY();
                sumZ += member->GetPositionZ();
                ++count;
            }
        }

        if (count > 0)
        {
            groupCenter.Relocate(sumX / count, sumY / count, sumZ / count);
        }
        else
        {
            groupCenter = _bot->GetPosition();
        }
    }
    else
    {
        groupCenter = _bot->GetPosition();
    }

    // Use spatial grid to find optimal position with LOS to most allies
    Map* map = _bot->GetMap();
    Position bestPos = groupCenter;
    int maxVisibleAllies = 0;

    if (map)
    {
        DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
        {
            sSpatialGridManager.CreateGrid(map);
            spatialGrid = sSpatialGridManager.GetGrid(map);
        }

        if (spatialGrid)
        {
            // Query nearby players (lock-free!)
            std::vector<DoubleBufferedSpatialGrid::PlayerSnapshot> nearbyPlayers =
                spatialGrid->QueryNearbyPlayers(_bot->GetPosition(), 40.0f);

            // Test different positions around the group center
            for (float testAngle = 0; testAngle < 2.0f * static_cast<float>(M_PI); testAngle += static_cast<float>(M_PI) / 4.0f)
            {
                Position testPos = PositionUtils::CalculatePositionAtAngle(groupCenter, healerDistance, testAngle);

                if (!ValidatePosition(testPos, PositionValidation::BASIC))
                    continue;

                int visibleAllies = 0;

                // Count visible allies using snapshot positions
                for (auto const& snapshot : nearbyPlayers)
                {
                    if (snapshot.guid == _bot->GetGUID())
                        continue; // Skip self

                    // Simple distance check for LOS proxy
                    float dx = testPos.GetPositionX() - snapshot.position.GetPositionX();
                    float dy = testPos.GetPositionY() - snapshot.position.GetPositionY();
                    float dz = testPos.GetPositionZ() - snapshot.position.GetPositionZ();
                    float distSq = dx * dx + dy * dy + dz * dz;

                    // Healers need to be within 40 yards of allies
                    if (distSq < 40.0f * 40.0f)
                        ++visibleAllies;
                }

                if (visibleAllies > maxVisibleAllies)
                {
                    maxVisibleAllies = visibleAllies;
                    bestPos = testPos;
                }
            }
        }
    }

    return bestPos;
}

Position PositionManager::FindDpsPosition(Unit* target, PositionType type)
{
    if (!target)
        return _bot->GetPosition();

    // Route to appropriate DPS positioning based on type
    switch (type)
    {
        case PositionType::MELEE_COMBAT:
        case PositionType::FLANKING:  // Flanking positioning for melee DPS (behind target)
            return FindMeleePosition(target, true); // Prefer behind for DPS

        case PositionType::RANGED_DPS:  // Handles both ranged DPS and casters
            return FindRangedPosition(target, 25.0f); // Standard ranged distance

        default:
            // Default to ranged for unknown types
            return FindRangedPosition(target, 20.0f);
    }
}

bool PositionManager::IsPositionSafe(const Position& pos, const MovementContext& context)
{
    if (IsInDangerZone(pos))
        return false;

    for (Unit* enemy : context.nearbyEnemies)
    {
        if (!enemy)
            continue;

        float distance = pos.GetExactDist(enemy);
        if (distance < 5.0f && enemy->IsHostileTo(_bot))
        {
            return false;
        }
    }
    return ValidatePosition(pos, PositionValidation::SAFE);
}

bool PositionManager::IsInDangerZone(const Position& pos)
{
    uint32 currentTime = GameTime::GetGameTimeMS();

    for (const AoEZone& zone : _activeZones)
    {
        if (zone.IsPositionInDanger(pos, currentTime))
        {
            return true;
        }
    }

    return false;
}

Position PositionManager::FindSafePosition(const Position& fromPos, float minDistance)
{
    ::std::vector<Position> safePositions = GenerateCircularPositions(fromPos, minDistance, 16);

    for (const Position& pos : safePositions)
    {
        if (!IsInDangerZone(pos) && ValidatePosition(pos, PositionValidation::BASIC))
        {
            return pos;
        }
    }
    ::std::vector<Position> farPositions = GenerateCircularPositions(fromPos, minDistance * 2.0f, 16);
    for (const Position& pos : farPositions)
    {
        if (!IsInDangerZone(pos) && ValidatePosition(pos, PositionValidation::BASIC))
        {
            return pos;
        }
    }

    return fromPos;
}

void PositionManager::RegisterAoEZone(const AoEZone& zone)
{
    // No lock needed - position data is per-bot instance data
    _activeZones.push_back(zone);

    TC_LOG_DEBUG("playerbot.position", "Registered AoE zone for bot {} at ({:.2f}, {:.2f}) radius {:.2f}",
               _bot->GetName(), zone.center.GetPositionX(), zone.center.GetPositionY(), zone.radius);
}

void PositionManager::UpdateAoEZones(uint32 currentTime)
{
    if (currentTime - _lastZoneUpdate < 1000)  // Update every second
        return;

    ClearExpiredZones(currentTime);
    _lastZoneUpdate = currentTime;
}

void PositionManager::ClearExpiredZones(uint32 currentTime)
{
    _activeZones.erase(
        ::std::remove_if(_activeZones.begin(), _activeZones.end(),
            [currentTime](const AoEZone& zone) {
                return !zone.isActive || currentTime > zone.startTime + zone.duration;
            }),
        _activeZones.end()
    );
}

bool PositionManager::ValidatePosition(const Position& pos, PositionValidation flags)
{
    if ((static_cast<uint32>(flags) & static_cast<uint32>(PositionValidation::WALKABLE)) && !IsWalkablePosition(pos))
        return false;

    if (static_cast<uint32>(flags) & static_cast<uint32>(PositionValidation::STABLE_GROUND))
    {
        Map* map = _bot->GetMap();
        if (!map || !PositionUtils::IsPositionOnGround(pos, map))
            return false;
    }

    if (static_cast<uint32>(flags) & static_cast<uint32>(PositionValidation::NO_OBSTACLES))
    {
        if (!PositionUtils::CanWalkStraightLine(_bot->GetPosition(), pos, _bot->GetMap()))
            return false;
    }

    if ((static_cast<uint32>(flags) & static_cast<uint32>(PositionValidation::AVOID_AOE)) && IsInDangerZone(pos))
        return false;

    return true;
}

bool PositionManager::IsWalkablePosition(const Position& pos)
{
    Map* map = _bot->GetMap();
    if (!map)
        return false;

    return map->IsInWater(_bot->GetPhaseShift(), pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ()) == false &&
           PositionUtils::IsPositionOnGround(pos, map);
}

float PositionManager::CalculateMovementCost(const Position& from, const Position& to)
{
    float distance = from.GetExactDist(&to);
    float heightDiff = ::std::abs(to.GetPositionZ() - from.GetPositionZ());

    float cost = distance;
    if (heightDiff > 2.0f)
        cost += heightDiff * 2.0f;  // Penalty for elevation changes
    if (!PositionUtils::CanWalkStraightLine(from, to, _bot->GetMap()))
        cost *= 1.5f;  // Penalty for indirect paths

    return cost;
}

PositionMovementResult PositionManager::HandleEmergencyMovement(const MovementContext& /* context */)
{
    _metrics.emergencyMoves++;

    Position emergencyPos = FindEmergencyEscapePosition();

    PositionMovementResult result;
    result.priority = MovementPriority::PRIORITY_CRITICAL;
    result.requiresSprint = true;

    return ExecuteMovement(emergencyPos, MovementPriority::PRIORITY_CRITICAL);
}

Position PositionManager::FindEmergencyEscapePosition()
{
    Position currentPos = _bot->GetPosition();
    ::std::vector<Position> escapePositions = GenerateCircularPositions(currentPos, EMERGENCY_DISTANCE, 12);

    for (const Position& pos : escapePositions)
    {
        if (!IsInDangerZone(pos) && ValidatePosition(pos, PositionValidation::BASIC))
        {
            return pos;
        }
    }

    return currentPos;
}

float PositionManager::CalculateDistanceScore(const Position& pos, const MovementContext& context)
{
    if (!context.target)
        return 50.0f;

    float distance = pos.GetExactDist(context.target);
    float optimalDistance = context.preferredRange;

    if (distance <= optimalDistance * 1.2f && distance >= optimalDistance * 0.8f)
        return 100.0f;

    float deviation = ::std::abs(distance - optimalDistance) / optimalDistance;
    return ::std::max(0.0f, 100.0f - (deviation * 100.0f));
}

float PositionManager::CalculateSafetyScore(const Position& pos, const MovementContext& context)
{
    float score = 100.0f;

    if (IsInDangerZone(pos))
        score -= 80.0f;

    for (Unit* enemy : context.nearbyEnemies)
    {
        if (!enemy)
            continue;

        float distance = pos.GetExactDist(enemy);
        if (distance < 5.0f)
            score -= 30.0f;
        else if (distance < 10.0f)
            score -= 15.0f;
    }

    return ::std::max(0.0f, score);
}

float PositionManager::CalculateLineOfSightScore(const Position& /* pos */, const MovementContext& context)
{
    if (!context.target)
        return 100.0f;

    if (_bot->IsWithinLOSInMap(context.target))
        return 100.0f;
    else
        return 0.0f;
}

float PositionManager::CalculateAngleScore(const Position& pos, const MovementContext& context)
{
    if (!context.target)
        return 50.0f;

    float score = 50.0f;
    Position targetPos = context.target->GetPosition();
    float targetAngle = context.target->GetOrientation();
    float positionAngle = PositionUtils::CalculateAngleBetween(targetPos, pos);

    switch (context.desiredType)
    {
        case PositionType::MELEE_COMBAT:
        case PositionType::FLANKING:
            {
                float behindAngle = PositionUtils::NormalizeAngle(targetAngle + static_cast<float>(M_PI));
                float angleDiff = ::std::abs(PositionUtils::NormalizeAngle(positionAngle - behindAngle));
                if (angleDiff < static_cast<float>(M_PI) / 6.0f)  // Within 30 degrees behind
                    score += 30.0f;
            }
            break;

        case PositionType::TANKING:
            {
                float frontAngle = targetAngle;
                float angleDiff = ::std::abs(PositionUtils::NormalizeAngle(positionAngle - frontAngle));
                if (angleDiff < static_cast<float>(M_PI) / 6.0f)  // Within 30 degrees in front
                    score += 30.0f;
            }
            break;

        default:
            score = 50.0f;
            break;
    }
    return score;
}

float PositionManager::CalculateGroupScore(const Position& pos, const MovementContext& context)
{
    if (context.groupMembers.empty())
        return 50.0f;

    Position groupCenter = PositionUtils::CalculateGroupCenter(context.groupMembers);
    float distanceToGroup = pos.GetExactDist(&groupCenter);

    float optimalDistance = GetOptimalGroupDistance(context.botRole);

    if (distanceToGroup <= optimalDistance * 1.2f && distanceToGroup >= optimalDistance * 0.8f)
        return 100.0f;

    float deviation = ::std::abs(distanceToGroup - optimalDistance) / optimalDistance;
    return ::std::max(0.0f, 100.0f - (deviation * 50.0f));
}

float PositionManager::GetOptimalGroupDistance(ThreatRole role)
{
    switch (role)
    {
        case ThreatRole::TANK:
            return 8.0f;
        case ThreatRole::HEALER:
            return 25.0f;
        case ThreatRole::DPS:
            return 15.0f;
        case ThreatRole::SUPPORT:
            return 20.0f;
        default:
            return 15.0f;
    }
}

::std::vector<Position> PositionManager::GenerateCircularPositions(const Position& center, float radius, uint32 count)
{
    ::std::vector<Position> positions;
    positions.reserve(count);

    for (uint32 i = 0; i < count; ++i)
    {
        float angle = (2.0f * static_cast<float>(M_PI) * i) / count;
        Position pos = PositionUtils::CalculatePositionAtAngle(center, radius, angle);
        positions.push_back(pos);
    }
    return positions;
}

::std::vector<Position> PositionManager::GenerateArcPositions(const Position& center, float radius, float startAngle, float endAngle, uint32 count)
{
    ::std::vector<Position> positions;
    positions.reserve(count);

    float angleStep = (endAngle - startAngle) / (count - 1);

    for (uint32 i = 0; i < count; ++i)
    {
        float angle = startAngle + (angleStep * i);
        Position pos = PositionUtils::CalculatePositionAtAngle(center, radius, angle);
        positions.push_back(pos);
    }

    return positions;
}

float PositionManager::EstimateMovementTime(const Position& from, const Position& to)
{
    float distance = from.GetExactDist(&to);
    float baseSpeed = _bot->GetSpeed(MOVE_RUN);

    return distance / baseSpeed;
}

bool PositionManager::CanReachPosition(const Position& pos)
{
    return ValidatePosition(pos, static_cast<PositionValidation>(
        static_cast<uint32>(PositionValidation::WALKABLE) |
        static_cast<uint32>(PositionValidation::NO_OBSTACLES)));
}

::std::vector<Position> PositionManager::CalculateWaypoints(const Position& from, const Position& to)
{
    ::std::vector<Position> waypoints;
    waypoints.push_back(from);
    waypoints.push_back(to);

    return waypoints;
}

void PositionManager::TrackPerformance(::std::chrono::microseconds duration, const ::std::string& /* operation */)
{
    if (duration > _metrics.maxEvaluationTime)
        _metrics.maxEvaluationTime = duration;

    auto currentTime = ::std::chrono::steady_clock::now();
    auto timeSinceLastUpdate = ::std::chrono::duration_cast<::std::chrono::seconds>(currentTime - _metrics.lastUpdate);

    if (timeSinceLastUpdate.count() >= 1)
    {
        uint32 totalEvaluations = _metrics.positionEvaluations.load();
        if (totalEvaluations > 0)
        {
            _metrics.averageEvaluationTime = ::std::chrono::microseconds(
                static_cast<uint64_t>(_metrics.averageEvaluationTime.count() * 0.9 + duration.count() * 0.1)
            );
        }
        _metrics.lastUpdate = currentTime;
    }
}

float PositionManager::CalculateEscapeScore(const Position& pos, const MovementContext& context)
{
    if (!_bot)
        return 0.0f;

    float score = 0.0f;

    // Base score for distance from current position
    float currentDistance = _bot->GetPosition().GetExactDist(&pos);
    score += ::std::min(currentDistance * 10.0f, 50.0f); // Max 50 points for distance

    // Lock-free spatial grid query for nearby enemies
    Map* map = _bot->GetMap();
    if (map)
    {
        DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
        {
            // Create grid on demand
            sSpatialGridManager.CreateGrid(map);
            spatialGrid = sSpatialGridManager.GetGrid(map);
        }

        if (spatialGrid)
        {
            // PHASE 5B: Thread-safe spatial grid query (replaces QueryNearbyCreatureGuids + ObjectAccessor)
            auto hostileSnapshots = SpatialGridQueryHelpers::FindHostileCreaturesInRange(_bot, 30.0f, true);

            // Check distance from nearby enemies
            float minEnemyDistance = 1000.0f;

            // Use snapshots for position calculations (lock-free!)
            for (auto const& snapshot : hostileSnapshots)
            {
                // Validate with Unit* for IsHostileTo check
                // SPATIAL GRID MIGRATION COMPLETE (2025-11-26):
                // ObjectAccessor is intentionally retained - Live Unit* needed for:
                // 1. IsHostileTo() faction check requires live relationship data
                // 2. Threat level assessment requires current threat table state
                // The spatial grid provides position data, ObjectAccessor handles faction checks.
                Unit* enemy = ObjectAccessor::GetUnit(*_bot, snapshot.guid);
                if (!enemy || !_bot->IsHostileTo(enemy))
                    continue;

                float enemyDistance = pos.GetExactDist(snapshot.position);
                minEnemyDistance = ::std::min(minEnemyDistance, enemyDistance);

                // Higher score for positions farther from enemies
    if (enemyDistance > 0.0f)
                    score += ::std::min(enemyDistance * 5.0f, 30.0f); // Max 30 points per enemy
            }

            // Bonus for getting to safe range
    if (minEnemyDistance > 15.0f)
                score += 20.0f;
        }
    }

    // Check for line of sight to group members (we want to maintain LOS)
    if (Group* group = _bot->GetGroup())
    {
        for (GroupReference const& itr : group->GetMembers())
        {
            Player* member = itr.GetSource();
            if (member && member != _bot && member->IsInWorld())
            {
                if (_bot->IsWithinLOSInMap(member))
                    score += 5.0f;
            }
        }
    }

    // Penalty for positions in AoE zones
    if (IsInDangerZone(pos))
    {
        score -= 40.0f;
        TC_LOG_TRACE("module.playerbot.position",
            "ScorePositionForCombat: Position ({:.1f}, {:.1f}) is in AoE danger zone, -40 penalty",
            pos.GetPositionX(), pos.GetPositionY());
    }

    // Check for nearby dynamic objects (ground effects, AoE spells)
    if (_bot && _bot->GetMap())
    {
        // Scan for hostile ground effects near this position
        float aoePenalty = CalculateAoEThreat(pos);
        if (aoePenalty > 0.0f)
        {
            score -= aoePenalty;
            TC_LOG_TRACE("module.playerbot.position",
                "ScorePositionForCombat: Position ({:.1f}, {:.1f}) near AoE, -{:.1f} penalty",
                pos.GetPositionX(), pos.GetPositionY(), aoePenalty);
        }
    }

    // Penalty for positions too close to walls/obstacles
    MovementContext tempContext;
    if (!IsPositionSafe(pos, tempContext))
        score -= 20.0f;

    return ::std::max(score, 0.0f);
}

// PositionUtils implementation
Position PositionUtils::CalculatePositionAtAngle(const Position& center, float distance, float angle)
{
    Position result;
    result.m_positionX = center.GetPositionX() + distance * ::std::cos(angle);
    result.m_positionY = center.GetPositionY() + distance * ::std::sin(angle);
    result.m_positionZ = center.GetPositionZ();
    result.SetOrientation(angle);

    return result;
}

float PositionUtils::CalculateAngleBetween(const Position& from, const Position& to)
{
    float dx = to.GetPositionX() - from.GetPositionX();
    float dy = to.GetPositionY() - from.GetPositionY();

    return ::std::atan2(dy, dx);
}

float PositionUtils::NormalizeAngle(float angle)
{
    while (angle > static_cast<float>(M_PI))
        angle -= 2.0f * static_cast<float>(M_PI);
    while (angle < -static_cast<float>(M_PI))
        angle += 2.0f * static_cast<float>(M_PI);

    return angle;
}

bool PositionUtils::IsInMeleeRange(Player* bot, Unit* target)
{
    if (!bot || !target)
        return false;

    return bot->GetExactDistSq(target) <= (5.0f * 5.0f); // 25.0f
}

bool PositionUtils::IsInOptimalRange(Player* bot, Unit* target, PositionType type)
{
    if (!bot || !target)
        return false;

    float distSq = bot->GetExactDistSq(target);

    switch (type)
    {
        case PositionType::MELEE_COMBAT:
            return distSq <= (5.0f * 5.0f); // 25.0f
        case PositionType::RANGED_DPS:
            return distSq >= (20.0f * 20.0f) && distSq <= (40.0f * 40.0f); // 400.0f - 1600.0f
        case PositionType::HEALING:
            return distSq <= (40.0f * 40.0f); // 1600.0f
        case PositionType::KITING:
            return distSq >= (15.0f * 15.0f); // 225.0f
        default:
            return true;
    }
}

Position PositionUtils::CalculateGroupCenter(const ::std::vector<Player*>& players)
{
    if (players.empty())
        return Position();

    float totalX = 0.0f, totalY = 0.0f, totalZ = 0.0f;

    for (Player* player : players)
    {
        if (player)
        {
            totalX += player->GetPositionX();
            totalY += player->GetPositionY();
            totalZ += player->GetPositionZ();
        }
    }

    Position center;
    center.m_positionX = totalX / players.size();
    center.m_positionY = totalY / players.size();
    center.m_positionZ = totalZ / players.size();

    return center;
}

bool PositionUtils::IsPositionOnGround(const Position& pos, Map* map)
{
    if (!map)
        return false;

    float groundZ = map->GetHeight(PhasingHandler::GetEmptyPhaseShift(), pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ());
    return ::std::abs(pos.GetPositionZ() - groundZ) <= 2.0f;
}

Position PositionManager::PredictTargetPosition(Unit* target, float timeAhead)
{
    if (!target)
        return Position();

    // Get current position and movement info
    Position currentPos = target->GetPosition();

    // Calculate predicted position based on movement speed
    // Simple prediction: if target has movement speed, predict forward movement

    // Calculate predicted position based on current velocity
    float speed = target->GetSpeed(MOVE_RUN); // Get movement speed
    if (speed <= 0.0f)
        return currentPos;

    // Get current facing direction
    float orientation = target->GetOrientation();

    // Calculate movement distance
    float distance = speed * timeAhead;

    // Project position forward based on current facing direction
    Position predictedPos;
    predictedPos.m_positionX = currentPos.GetPositionX() + distance * ::std::cos(orientation);
    predictedPos.m_positionY = currentPos.GetPositionY() + distance * ::std::sin(orientation);
    predictedPos.m_positionZ = currentPos.GetPositionZ();
    predictedPos.SetOrientation(orientation);

    // Validate the predicted position (check if walkable)
    if (_bot && _bot->GetMap())
    {
        float groundZ = _bot->GetMap()->GetHeight(PhasingHandler::GetEmptyPhaseShift(),
                                                 predictedPos.GetPositionX(),
                                                 predictedPos.GetPositionY(),
                                                 predictedPos.GetPositionZ());

        // If ground height is reasonable, use it
    if (::std::abs(groundZ - predictedPos.GetPositionZ()) <= 10.0f)
            predictedPos.m_positionZ = groundZ;
    }

    return predictedPos;
}

bool PositionUtils::CanWalkStraightLine(const Position& from, const Position& to, Map* map)
{
    if (!map)
        return false;

    // Use TrinityCore API properly with PhaseShift
    // We need a PhaseShift - for now, create a default one or get from a WorldObject
    PhaseShift phaseShift;
    return map->isInLineOfSight(
        phaseShift, from.GetPositionX(), from.GetPositionY(), from.GetPositionZ(),
                               to.GetPositionX(), to.GetPositionY(), to.GetPositionZ(),
                               LINEOFSIGHT_ALL_CHECKS, VMAP::ModelIgnoreFlags::Nothing);
}


Position PositionManager::FindRangePosition(Unit* target, float minRange, float maxRange, float preferredAngle)
{
    if (!_bot || !target)
        return Position();

    std::lock_guard<Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE>> lock(_mutex);

    Position targetPos = target->GetPosition();
    float preferredRange = (minRange + maxRange) / 2.0f;

    float angle = preferredAngle;
    if (preferredAngle == 0.0f)
        angle = target->GetOrientation() + static_cast<float>(M_PI);

    Position candidatePos;
    candidatePos.Relocate(
        targetPos.GetPositionX() + std::cos(angle) * preferredRange,
        targetPos.GetPositionY() + std::sin(angle) * preferredRange,
        targetPos.GetPositionZ()
    );

    if (IsWalkablePosition(candidatePos))
        return candidatePos;

    for (int i = 1; i <= 8; ++i)
    {
        float testAngle = angle + (i * static_cast<float>(M_PI) / 4.0f);
        candidatePos.Relocate(
            targetPos.GetPositionX() + std::cos(testAngle) * preferredRange,
            targetPos.GetPositionY() + std::sin(testAngle) * preferredRange,
            targetPos.GetPositionZ()
        );
        if (IsWalkablePosition(candidatePos))
            return candidatePos;
    }

    return _bot->GetPosition();
}

Position PositionManager::FindHealingPosition(const ::std::vector<Player*>& allies)
{
    if (!_bot || allies.empty())
        return _bot ? _bot->GetPosition() : Position();

    std::lock_guard<Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE>> lock(_mutex);

    float sumX = 0.0f, sumY = 0.0f, sumZ = 0.0f;
    uint32 validAllies = 0;

    for (const auto* ally : allies)
    {
        if (ally && ally->IsAlive())
        {
            sumX += ally->GetPositionX();
            sumY += ally->GetPositionY();
            sumZ += ally->GetPositionZ();
            validAllies++;
        }
    }

    if (validAllies == 0)
        return _bot->GetPosition();

    Position centerPos;
    centerPos.Relocate(sumX / validAllies, sumY / validAllies, sumZ / validAllies);

    float safeOffset = 5.0f;
    Position healPos;
    healPos.Relocate(centerPos.GetPositionX() - safeOffset, centerPos.GetPositionY(), centerPos.GetPositionZ());

    if (IsWalkablePosition(healPos) && !IsInDangerZone(healPos))
        return healPos;

    return centerPos;
}

Position PositionManager::FindSupportPosition(const ::std::vector<Player*>& groupMembers)
{
    if (!_bot || groupMembers.empty())
        return _bot ? _bot->GetPosition() : Position();

    std::lock_guard<Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE>> lock(_mutex);

    float sumX = 0.0f, sumY = 0.0f, sumZ = 0.0f;
    uint32 validMembers = 0;

    for (const auto* member : groupMembers)
    {
        if (member && member->IsAlive())
        {
            sumX += member->GetPositionX();
            sumY += member->GetPositionY();
            sumZ += member->GetPositionZ();
            validMembers++;
        }
    }

    if (validMembers == 0)
        return _bot->GetPosition();

    Position supportPos;
    supportPos.Relocate(sumX / validMembers, sumY / validMembers, sumZ / validMembers);
    return supportPos;
}

Position PositionManager::FindEscapePosition(const ::std::vector<Unit*>& threats)
{
    if (!_bot || threats.empty())
        return _bot ? _bot->GetPosition() : Position();

    std::lock_guard<Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE>> lock(_mutex);

    float sumX = 0.0f, sumY = 0.0f;
    uint32 validThreats = 0;

    for (const auto* threat : threats)
    {
        if (threat && threat->IsAlive())
        {
            sumX += threat->GetPositionX();
            sumY += threat->GetPositionY();
            validThreats++;
        }
    }

    if (validThreats == 0)
        return _bot->GetPosition();

    float threatCenterX = sumX / validThreats;
    float threatCenterY = sumY / validThreats;

    float dx = _bot->GetPositionX() - threatCenterX;
    float dy = _bot->GetPositionY() - threatCenterY;
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist < 0.1f) { dx = 1.0f; dy = 0.0f; dist = 1.0f; }

    float escapeDistance = 15.0f;
    Position escapePos;
    escapePos.Relocate(_bot->GetPositionX() + (dx / dist) * escapeDistance,
                       _bot->GetPositionY() + (dy / dist) * escapeDistance,
                       _bot->GetPositionZ());

    if (IsWalkablePosition(escapePos))
        return escapePos;

    return FindEmergencyEscapePosition();
}

::std::vector<AoEZone> PositionManager::GetActiveZones() const
{
    std::lock_guard<Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE>> lock(_mutex);

    ::std::vector<AoEZone> activeZones;
    for (const auto& zone : _activeZones)
    {
        if (zone.isActive)
            activeZones.push_back(zone);
    }
    return activeZones;
}

bool PositionManager::HasLineOfSight(const Position& from, const Position& to)
{
    if (!_bot) return false;
    Map* map = _bot->GetMap();
    if (!map) return false;

    return map->isInLineOfSight(
        _bot->GetPhaseShift(),
        from.GetPositionX(), from.GetPositionY(), from.GetPositionZ() + 2.0f,
        to.GetPositionX(), to.GetPositionY(), to.GetPositionZ() + 2.0f,
        LINEOFSIGHT_ALL_CHECKS, VMAP::ModelIgnoreFlags::Nothing);
}

Position PositionManager::FindFormationPosition(const ::std::vector<Player*>& groupMembers, PositionType formationType)
{
    if (!_bot || groupMembers.empty())
        return _bot ? _bot->GetPosition() : Position();

    std::lock_guard<Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE>> lock(_mutex);

    float sumX = 0.0f, sumY = 0.0f, sumZ = 0.0f;
    uint32 validMembers = 0;

    for (const auto* member : groupMembers)
    {
        if (member && member->IsAlive())
        {
            sumX += member->GetPositionX();
            sumY += member->GetPositionY();
            sumZ += member->GetPositionZ();
            validMembers++;
        }
    }

    if (validMembers == 0)
        return _bot->GetPosition();

    Position center;
    center.Relocate(sumX / validMembers, sumY / validMembers, sumZ / validMembers);

    float offset = 3.0f;
    switch (formationType)
    {
        case PositionType::MELEE_COMBAT: offset = 2.0f; break;
        case PositionType::RANGED_DPS: offset = 8.0f; break;
        case PositionType::HEALING: offset = 5.0f; break;
        default: offset = 4.0f; break;
    }

    size_t myIndex = 0;
    for (size_t i = 0; i < groupMembers.size(); ++i)
    {
        if (groupMembers[i] == _bot) { myIndex = i; break; }
    }

    float angle = (2.0f * static_cast<float>(M_PI) * static_cast<float>(myIndex)) / static_cast<float>(validMembers);
    Position formationPos;
    formationPos.Relocate(center.GetPositionX() + std::cos(angle) * offset,
                         center.GetPositionY() + std::sin(angle) * offset,
                         center.GetPositionZ());
    return formationPos;
}

bool PositionManager::ShouldMaintainGroupProximity()
{
    if (!_bot) return false;
    std::lock_guard<Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE>> lock(_mutex);
    if (_bot->IsInCombat()) return true;
    Group* group = _bot->GetGroup();
    return group != nullptr;
}

bool PositionManager::ShouldStrafe(Unit* target)
{
    if (!_bot || !target) return false;
    std::lock_guard<Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE>> lock(_mutex);

    if (target->IsNonMeleeSpellCast(false))
    {
        if (Spell* spell = target->GetCurrentSpell(CURRENT_GENERIC_SPELL))
        {
            if (Unit* spellTarget = spell->m_targets.GetUnitTarget())
            {
                if (spellTarget == _bot) return true;
            }
        }
    }

    if (IsInDangerZone(_bot->GetPosition())) return true;
    return false;
}

bool PositionManager::ShouldCircleStrafe(Unit* target)
{
    if (!_bot || !target) return false;
    std::lock_guard<Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE>> lock(_mutex);
    if (!target->IsNonMeleeSpellCast(false)) return false;
    float distance = _bot->GetExactDist(target);
    return distance < 8.0f;
}

Position PositionManager::CalculateStrafePosition(Unit* target, bool strafeLeft)
{
    if (!_bot || !target)
        return _bot ? _bot->GetPosition() : Position();

    std::lock_guard<Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE>> lock(_mutex);

    float currentAngle = _bot->GetAbsoluteAngle(target);
    float strafeAngle = strafeLeft ? (currentAngle + static_cast<float>(M_PI) / 4.0f) : (currentAngle - static_cast<float>(M_PI) / 4.0f);
    float distance = _bot->GetExactDist(target);

    Position strafePos;
    strafePos.Relocate(target->GetPositionX() + std::cos(strafeAngle) * distance,
                       target->GetPositionY() + std::sin(strafeAngle) * distance,
                       _bot->GetPositionZ());

    if (IsWalkablePosition(strafePos)) return strafePos;
    return _bot->GetPosition();
}

bool PositionManager::IsInEmergencyPosition()
{
    if (!_bot) return false;
    std::lock_guard<Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE>> lock(_mutex);
    if (IsInDangerZone(_bot->GetPosition())) return true;
    if (_bot->GetHealthPct() < 20.0f) return true;
    return false;
}

void PositionManager::RecordPositionSuccess(const Position& /* pos */, PositionType /* type */)
{
    _metrics.positionEvaluations++;
}

void PositionManager::RecordPositionFailure(const Position& /* pos */, const ::std::string& /* reason */)
{
    _metrics.positionEvaluations++;
}

float PositionManager::GetPositionSuccessRate(const Position& /* pos */, float /* radius */)
{
    return 0.5f;
}

float PositionManager::CalculateAoEThreat(const Position& pos)
{
    if (!_bot || !_bot->GetMap())
        return 0.0f;

    float totalThreat = 0.0f;
    uint32 currentTime = GameTime::GetGameTimeMS();

    // Check registered AoE zones for proximity threat
    for (const AoEZone& zone : _activeZones)
    {
        if (!zone.isActive || currentTime > zone.startTime + zone.duration)
            continue;

        float distToCenter = pos.GetExactDist(&zone.center);

        // If inside the zone, max threat
        if (distToCenter <= zone.radius)
        {
            totalThreat += zone.damageRating * 10.0f;  // Heavy penalty for being inside
        }
        // If near the edge, scaled threat
        else if (distToCenter <= zone.radius + 5.0f)
        {
            float edgeProximity = 1.0f - ((distToCenter - zone.radius) / 5.0f);
            totalThreat += zone.damageRating * edgeProximity * 5.0f;  // Scaled penalty near edge
        }
    }

    // Scan for dynamic objects (ground effects) that aren't in our registered zones
    Map* map = _bot->GetMap();

    // Check for lava/slime/water hazards at position
    LiquidData liquidData;
    ZLiquidStatus liquidStatus = map->GetLiquidStatus(
        _bot->GetPhaseShift(),
        pos.GetPositionX(),
        pos.GetPositionY(),
        pos.GetPositionZ() + 2.0f,
        {},  // All liquid types
        &liquidData
    );

    if (liquidStatus != LIQUID_MAP_NO_WATER)
    {
        // Check for dangerous liquids
        if (liquidData.type_flags.HasFlag(map_liquidHeaderTypeFlags::Magma) ||
            liquidData.type_flags.HasFlag(map_liquidHeaderTypeFlags::Slime))
        {
            totalThreat += 50.0f;  // Very high threat for lava/slime
        }
        else if (liquidData.type_flags.HasFlag(map_liquidHeaderTypeFlags::Water) ||
                 liquidData.type_flags.HasFlag(map_liquidHeaderTypeFlags::Ocean))
        {
            // Deep water is dangerous for non-swimming classes
            if (liquidData.depth_level > 2.0f)
            {
                totalThreat += 10.0f;  // Moderate threat for deep water
            }
        }
    }

    return totalThreat;
}

} // namespace Playerbot
