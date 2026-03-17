/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "KitingManager.h"
#include "GameTime.h"
#include "Player.h"
#include "Unit.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Map.h"
#include "Log.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "../../Spatial/SpatialGridManager.h"
#include "ObjectAccessor.h"
#include "Movement/UnifiedMovementCoordinator.h"
#include "../../Movement/Arbiter/MovementPriorityMapper.h"
#include "../BotAI.h"
#include "Core/PlayerBotHelpers.h"
#include "UnitAI.h"
#include <algorithm>
#include <cmath>

namespace Playerbot
{

KitingManager::KitingManager(Player* bot)
    : _bot(bot), _kitingTarget(nullptr), _kitingActive(false), _currentState(KitingState::INACTIVE),
      _currentKitingType(KitingType::NONE), _currentWaypointIndex(0), _lastMovementTime(0),
      _lastAttackTime(0), _attackWindowStart(0), _attackWindowEnd(0), _inAttackWindow(false),
      _optimalKitingDistance(DEFAULT_OPTIMAL_DISTANCE), _minKitingDistance(DEFAULT_MIN_DISTANCE),
      _maxKitingDistance(DEFAULT_MAX_DISTANCE), _updateInterval(DEFAULT_UPDATE_INTERVAL),
      _kitingAggressiveness(DEFAULT_AGGRESSIVENESS), _predictiveKiting(true), _emergencyKiting(false),
      _availableKitingSpace(50.0f), _lastObstacleUpdate(0), _kitingStartTime(0)
{

    TC_LOG_DEBUG("playerbot.kiting", "KitingManager initialized for bot {}", _bot->GetName());
}

void KitingManager::UpdateKiting(uint32 /*diff*/)
{
    // No lock needed - kiting state is per-bot instance data

    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime - _lastMovementTime < _updateInterval && !_emergencyKiting)
        return;

    _lastMovementTime = currentTime;

    try
    {
        if (!_kitingActive)
        {
            KitingContext context;
            context.bot = _bot;
            context.currentPosition = _bot->GetPosition();
            context.currentHealth = _bot->GetHealthPct();
            context.currentMana = _bot->GetPowerPct(POWER_MANA);
            context.inCombat = _bot->IsInCombat();
            context.isMoving = _bot->isMoving();
            context.isCasting = _bot->HasUnitState(UNIT_STATE_CASTING);

            // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
            float searchRadius = 40.0f;
            Map* map = _bot->GetMap();
            if (map)
            {
                DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
                if (!spatialGrid)
                {
                    // Grid not yet created for this map - create it on demand
                    sSpatialGridManager.CreateGrid(map);
                    spatialGrid = sSpatialGridManager.GetGrid(map);
                }

                if (spatialGrid)
                {
                    // Query nearby creature GUIDs (lock-free!)
                    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
                        _bot->GetPosition(), searchRadius);

                    // Resolve GUIDs to Unit pointers and filter enemies
    for (ObjectGuid guid : nearbyGuids)
                    {
                        // SPATIAL GRID MIGRATION COMPLETE (2025-11-26):
                        // ObjectAccessor is intentionally retained - Live Unit* needed for:
                        // 1. context.threats is std::vector<Unit*> - requires live Unit*
                        // 2. IsHostileTo() faction check requires live relationship data
                        // 3. IsAlive() verification requires real-time state check
                        // The spatial grid pre-filters candidates to reduce ObjectAccessor calls.
                        ::Unit* enemy = ObjectAccessor::GetUnit(*_bot, guid);
                        if (enemy && _bot->IsHostileTo(enemy) && enemy->IsAlive())
                            context.threats.push_back(enemy);
                    }
                }
            }

            if (_bot->GetVictim())
                context.primaryTarget = _bot->GetVictim();

            KitingResult evaluation = EvaluateKitingNeed(context);
            if (evaluation.success)
            {
                ExecuteKiting(context);
            }
        }
        else
        {
            UpdateKitingState();
            ExecuteCurrentPattern();
            UpdateAttackTiming();

            if (_predictiveKiting)
            {
                UpdateThreatPredictions();
            }
        }

        UpdateKitingStatistics();
    }
    catch (const ::std::exception& e)
    {
        TC_LOG_ERROR("playerbot.kiting", "Exception in UpdateKiting for bot {}: {}", _bot->GetName(), e.what());
    }
}

KitingResult KitingManager::EvaluateKitingNeed(const KitingContext& context)
{
    KitingResult result;

    if (context.threats.empty() || !context.inCombat)
    {
        result.failureReason = "No threats or not in combat";
        return result;
    }

    KitingTrigger triggers = EvaluateKitingTriggers(context);
    if (triggers == KitingTrigger::NONE)
    {
        result.failureReason = "No kiting triggers activated";
        return result;
    }

    Unit* primaryThreat = context.primaryTarget ? context.primaryTarget : context.threats[0];
    if (!IsKiteable(primaryThreat))
    {
        result.failureReason = "Primary threat is not kiteable";
        return result;
    }
    float currentDistance = context.currentPosition.GetExactDist(primaryThreat);
    float optimalDistance = GetOptimalKitingDistance(primaryThreat);

    if (currentDistance >= optimalDistance * 0.9f && !(triggers & KitingTrigger::EMERGENCY))
    {
        result.failureReason = "Already at optimal distance";
        return result;
    }

    KitingType optimalType = SelectOptimalKitingType(context);
    if (optimalType == KitingType::NONE)
    {
        result.failureReason = "No suitable kiting type found";
        return result;
    }

    Position kitingPosition = CalculateKitingPosition(primaryThreat, optimalType);
    if (!IsPositionSafe(kitingPosition, context.threats))
    {
        result.failureReason = "Kiting position is not safe";
        return result;
    }

    result.success = true;
    result.usedType = optimalType;
    result.nextPosition = kitingPosition;
    result.safetyImprovement = CalculateSafetyRating(kitingPosition, context.threats) -
                              CalculateSafetyRating(context.currentPosition, context.threats);

    TC_LOG_DEBUG("playerbot.kiting", "Bot {} evaluated kiting need: {} (triggers: {})",
               _bot->GetName(), result.success ? "REQUIRED" : "NOT_REQUIRED", static_cast<uint32>(triggers));

    return result;
}

KitingResult KitingManager::ExecuteKiting(const KitingContext& context)
{
    auto startTime = ::std::chrono::steady_clock::now();
    KitingResult result;

    try
    {
        KitingType kitingType = SelectOptimalKitingType(context);
        _currentKitingType = kitingType;
        _currentPattern = GenerateKitingPattern(kitingType, context);
        _kitingTarget = context.primaryTarget;
        _kitingActive = true;
        _currentState = KitingState::POSITIONING;
        _kitingStartTime = GameTime::GetGameTimeMS();

        switch (kitingType)
        {
            case KitingType::CIRCULAR_KITING:
                result = ExecuteCircularKiting(context);
                break;

            case KitingType::LINE_KITING:
                result = ExecuteLineKiting(context);
                break;

            case KitingType::STUTTER_STEP:
                result = ExecuteStutterStep(context);
                break;

            case KitingType::HIT_AND_RUN:
                result = ExecuteHitAndRun(context);
                break;

            case KitingType::FIGURE_EIGHT:
                result = ExecuteFigureEight(context);
                break;

            default:
                result = ExecuteCircularKiting(context);
                break;
        }

        if (result.success)
        {
            _metrics.kitingActivations++;
            _metrics.successfulKites++;

            TC_LOG_DEBUG("playerbot.kiting", "Bot {} started kiting with type {} against {}",
                       _bot->GetName(), static_cast<uint32>(kitingType),
                       _kitingTarget ? _kitingTarget->GetName() : "unknown");
        }
        else
        {
            _metrics.failedKites++;
            StopKiting();
        }

        auto endTime = ::std::chrono::steady_clock::now();
        auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime);
        TrackPerformance(duration, "ExecuteKiting");
    }
    catch (const ::std::exception& e)
    {
        result.success = false;
        result.failureReason = ::std::string("Exception during kiting execution: ") + e.what();
        TC_LOG_ERROR("playerbot.kiting", "Exception in ExecuteKiting for bot {}: {}", _bot->GetName(), e.what());
    }

    return result;
}

void KitingManager::StopKiting()
{
    // No lock needed - kiting state is per-bot instance data
    if (!_kitingActive)
        return;

    _kitingActive = false;
    _currentState = KitingState::INACTIVE;
    _currentKitingType = KitingType::NONE;
    _kitingTarget = nullptr;
    _kitingWaypoints.clear();
    _currentWaypointIndex = 0;

    if (_kitingStartTime > 0)
    {
        uint32 duration = GameTime::GetGameTimeMS() - _kitingStartTime;
        if (duration > _metrics.maxKitingDuration.count() / 1000)
        {
            _metrics.maxKitingDuration = ::std::chrono::microseconds(duration * 1000);
        }
        _kitingStartTime = 0;
    }
    TC_LOG_DEBUG("playerbot.kiting", "Bot {} stopped kiting", _bot->GetName());
}

KitingType KitingManager::SelectOptimalKitingType(const KitingContext& context)
{
    if (context.threats.empty())
        return KitingType::NONE;

    Unit* primaryThreat = context.primaryTarget ? context.primaryTarget : context.threats[0];
    float distance = context.currentPosition.GetExactDist(primaryThreat);

    if (_emergencyKiting || (context.triggers & KitingTrigger::EMERGENCY))
    {
        return KitingType::TACTICAL_RETREAT;
    }

    if (context.threats.size() >= 3)
    {
        return KitingType::CIRCULAR_KITING;
    }

    if (distance < _minKitingDistance)
    {
        return KitingType::STUTTER_STEP;
    }

    if (context.availableSpace >= 30.0f)
    {
        return KitingType::FIGURE_EIGHT;
    }

    uint8 botClass = _bot->GetClass();
    switch (botClass)
    {
        case CLASS_HUNTER:
        case CLASS_MAGE:
        case CLASS_WARLOCK:
            return KitingType::CIRCULAR_KITING;

        case CLASS_PRIEST:
        case CLASS_SHAMAN:
            return KitingType::LINE_KITING;

        case CLASS_DRUID:
            return KitingType::HIT_AND_RUN;

        default:
            return KitingType::STUTTER_STEP;
    }
}

KitingPattern KitingManager::GenerateKitingPattern(KitingType type, const KitingContext& context)
{
    KitingPattern pattern;
    pattern.type = type;
    pattern.optimalDistance = _optimalKitingDistance;
    pattern.minDistance = _minKitingDistance;
    pattern.maxDistance = _maxKitingDistance;
    pattern.maintainLoS = true;

    Unit* target = context.primaryTarget ? context.primaryTarget :
                  (!context.threats.empty() ? context.threats[0] : nullptr);

    if (!target)
        return pattern;

    switch (type)
    {
        case KitingType::CIRCULAR_KITING:
            pattern.waypoints = GenerateCircularWaypoints(target, _optimalKitingDistance, 8);
            pattern.attackWindow = 2.5f;
            pattern.movementWindow = 1.0f;
            pattern.description = "Circular kiting around target";
            break;
        case KitingType::LINE_KITING:
            {
                Position direction = FindSafeKitingDirection(context.threats);
                pattern.waypoints = GenerateLineWaypoints(context.currentPosition, direction, _optimalKitingDistance);
                pattern.attackWindow = 3.0f;
                pattern.movementWindow = 1.5f;
                pattern.description = "Linear retreat kiting";
            }
            break;

        case KitingType::STUTTER_STEP:
            {
                Position direction = FindSafeKitingDirection(context.threats);
                pattern.waypoints = GenerateLineWaypoints(context.currentPosition, direction, _minKitingDistance);
                pattern.attackWindow = 1.5f;
                pattern.movementWindow = 0.5f;
                pattern.description = "Stutter step kiting";
            }
            break;

        case KitingType::FIGURE_EIGHT:
            pattern.waypoints = GenerateFigureEightWaypoints(target, _optimalKitingDistance);
            pattern.attackWindow = 2.0f;
            pattern.movementWindow = 1.2f;
            pattern.description = "Figure-8 kiting pattern";
            break;

        case KitingType::HIT_AND_RUN:
            {
                Position retreatPos = GetRetreatPosition(context.threats, _optimalKitingDistance);
                pattern.waypoints = { context.currentPosition, retreatPos };
                pattern.attackWindow = 1.0f;
                pattern.movementWindow = 2.0f;
                pattern.description = "Hit and run tactics";
            }
            break;

        default:
            pattern.waypoints = GenerateCircularWaypoints(target, _optimalKitingDistance, 6);
            pattern.description = "Default kiting pattern";
            break;
    }
    return pattern;
}

bool KitingManager::ShouldMaintainDistance(Unit* target)
{
    if (!target || !_bot->IsHostileTo(target))
        return false;

    return _bot->GetExactDistSq(target) < (_optimalKitingDistance * _optimalKitingDistance);
}

float KitingManager::GetOptimalKitingDistance(Unit* target)
{
    if (!target)
        return _optimalKitingDistance;

    uint8 botClass = _bot->GetClass();
    float baseDistance = KitingUtils::GetClassKitingRange(botClass);

    if (target->GetTypeId() == TYPEID_UNIT)
    {
        if (target->GetUnitMovementFlags() & MOVEMENTFLAG_WALKING)
            baseDistance *= 0.8f;

        if (target->HasUnitState(UNIT_STATE_CASTING))
            baseDistance *= 1.2f;
    }

    return ::std::max(baseDistance, _minKitingDistance);
}

bool KitingManager::IsAtOptimalKitingDistance(Unit* target)
{
    if (!target)
        return false;

    float distance = ::std::sqrt(_bot->GetExactDistSq(target)); // Calculate once from squared distance
    float optimal = GetOptimalKitingDistance(target);

    return distance >= optimal * 0.9f && distance <= optimal * 1.1f;
}

KitingResult KitingManager::ExecuteCircularKiting(const KitingContext& context)
{
    KitingResult result;
    result.usedType = KitingType::CIRCULAR_KITING;

    Unit* target = context.primaryTarget;
    if (!target)
    {
        result.failureReason = "No target for circular kiting";
        return result;
    }

    Position targetPos = target->GetPosition();
    float angle = ::std::atan2(_bot->GetPositionY() - targetPos.GetPositionY(),
                           _bot->GetPositionX() - targetPos.GetPositionX());

    angle += static_cast<float>(M_PI) / 4.0f;
    Position kitingPos = GetCircularKitingPosition(target, angle);

    if (!IsPositionSafe(kitingPos, context.threats))
    {
        angle += static_cast<float>(M_PI) / 2.0f;
        kitingPos = GetCircularKitingPosition(target, angle);
    }

    if (ExecuteMovementToPosition(kitingPos))
    {
        result.success = true;
        result.nextPosition = kitingPos;
        _currentState = KitingState::KITING;
    }
    else
    {
        result.failureReason = "Failed to move to kiting position";
    }

    return result;
}

KitingResult KitingManager::ExecuteLineKiting(const KitingContext& context)
{
    KitingResult result;
    result.usedType = KitingType::LINE_KITING;

    Position safeDirection = FindSafeKitingDirection(context.threats);
    Position retreatPos = GetRetreatPosition(context.threats, _optimalKitingDistance);

    if (ExecuteMovementToPosition(retreatPos))
    {
        result.success = true;
        result.nextPosition = retreatPos;
        _currentState = KitingState::KITING;
    }
    else
    {
        result.failureReason = "Failed to execute line kiting";
    }

    return result;
}

KitingResult KitingManager::ExecuteStutterStep(const KitingContext& context)
{
    KitingResult result;
    result.usedType = KitingType::STUTTER_STEP;

    Position currentPos = _bot->GetPosition();
    Position retreatPos = GetRetreatPosition(context.threats, 5.0f);

    if (ExecuteMovementToPosition(retreatPos))
    {
        result.success = true;
        result.nextPosition = retreatPos;
        _currentState = KitingState::KITING;

        uint32 currentTime = GameTime::GetGameTimeMS();
        _attackWindowStart = currentTime + 500;
        _attackWindowEnd = _attackWindowStart + 1500;
    }
    else
    {
        result.failureReason = "Failed to execute stutter step";
    }

    return result;
}

KitingResult KitingManager::ExecuteHitAndRun(const KitingContext& context)
{
    KitingResult result;
    result.usedType = KitingType::HIT_AND_RUN;
    if (_currentState == KitingState::ATTACKING)
    {
        Position retreatPos = GetRetreatPosition(context.threats, _optimalKitingDistance);
        if (ExecuteMovementToPosition(retreatPos))
        {
            result.success = true;
            result.nextPosition = retreatPos;
            _currentState = KitingState::RETREATING;
        }
    }
    else
    {
        Position attackPos = GetAttackPosition(context.primaryTarget);
        if (ExecuteMovementToPosition(attackPos))
        {
            result.success = true;
            result.nextPosition = attackPos;
            _currentState = KitingState::ATTACKING;
        }
    }

    return result;
}

KitingResult KitingManager::ExecuteFigureEight(const KitingContext& context)
{
    KitingResult result;
    result.usedType = KitingType::FIGURE_EIGHT;

    Unit* target = context.primaryTarget;
    if (!target)
    {
        result.failureReason = "No target for figure-8 kiting";
        return result;
    }

    if (_kitingWaypoints.empty())
    {
        _kitingWaypoints = GenerateFigureEightWaypoints(target, _optimalKitingDistance);
        _currentWaypointIndex = 0;
    }

    if (_currentWaypointIndex < _kitingWaypoints.size())
    {
        Position nextWaypoint = _kitingWaypoints[_currentWaypointIndex];

        if (ExecuteMovementToPosition(nextWaypoint))
        {
            if (_bot->GetExactDistSq(nextWaypoint) <= (2.0f * 2.0f)) // 4.0f
            {
                _currentWaypointIndex = (_currentWaypointIndex + 1) % _kitingWaypoints.size();
            }

            result.success = true;
            result.nextPosition = nextWaypoint;
            _currentState = KitingState::KITING;
        }
    }

    return result;
}

::std::vector<KitingTarget> KitingManager::AnalyzeThreats(const ::std::vector<Unit*>& enemies)
{
    ::std::vector<KitingTarget> threats;
    threats.reserve(enemies.size());

    Position botPos = _bot->GetPosition();
    for (Unit* enemy : enemies)
    {
        if (!enemy || !enemy->IsAlive() || !_bot->IsHostileTo(enemy))
            continue;

        KitingTarget threat;
        threat.guid = enemy->GetGUID();
        threat.unit = enemy;
        threat.position = enemy->GetPosition();
        threat.distance = botPos.GetExactDist(&threat.position);
        threat.isMoving = enemy->isMoving();
        threat.isCasting = enemy->HasUnitState(UNIT_STATE_CASTING);
        threat.name = enemy->GetName();
        threat.lastUpdate = GameTime::GetGameTimeMS();

        if (threat.isMoving)
        {
            threat.velocity.m_positionX = enemy->GetSpeedXY() * ::std::cos(enemy->GetOrientation());
            threat.velocity.m_positionY = enemy->GetSpeedXY() * ::std::sin(enemy->GetOrientation());
            threat.relativeSpeed = CalculateRelativeSpeed(enemy);
        }

        threat.threatLevel = enemy->GetThreatManager().GetThreat(_bot);
        threats.push_back(threat);
    }

    ::std::sort(threats.begin(), threats.end(),
        [](const KitingTarget& a, const KitingTarget& b) {
            return a.distance < b.distance;
        });

    return threats;
}

bool KitingManager::IsKiteable(Unit* target)
{
    if (!target || !target->IsAlive())
        return false;

    if (target->HasUnitState(UNIT_STATE_ROOT))
        return true;

    if (target->HasUnitState(UNIT_STATE_STUNNED))
        return true;

    float targetSpeed = target->GetSpeedXY();
    float botSpeed = _bot->GetSpeedXY();
    return botSpeed >= targetSpeed * 0.9f;
}

Position KitingManager::CalculateKitingPosition(Unit* target, KitingType type)
{
    if (!target)
        return _bot->GetPosition();

    Position targetPos = target->GetPosition();
    Position botPos = _bot->GetPosition();

    switch (type)
    {
        case KitingType::CIRCULAR_KITING:
            {
                float angle = ::std::atan2(botPos.GetPositionY() - targetPos.GetPositionY(),
                                       botPos.GetPositionX() - targetPos.GetPositionX());
                return GetCircularKitingPosition(target, angle + static_cast<float>(M_PI) / 6.0f);
            }

        case KitingType::LINE_KITING:
        case KitingType::TACTICAL_RETREAT:
            return GetRetreatPosition({target}, _optimalKitingDistance);

        default:
            return GetCircularKitingPosition(target, 0.0f);
    }
}

Position KitingManager::FindSafeKitingDirection(const ::std::vector<Unit*>& threats)
{
    Position botPos = _bot->GetPosition();
    Position safeDirection(0.0f, 0.0f, 0.0f);

    for (Unit* threat : threats)
    {
        if (!threat)
            continue;

        Position threatPos = threat->GetPosition();
        float angle = ::std::atan2(botPos.GetPositionY() - threatPos.GetPositionY(),
                               botPos.GetPositionX() - threatPos.GetPositionX());

        safeDirection.m_positionX += ::std::cos(angle);
        safeDirection.m_positionY += ::std::sin(angle);
    }

    float length = ::std::sqrt(safeDirection.m_positionX * safeDirection.m_positionX +
                           safeDirection.m_positionY * safeDirection.m_positionY);

    if (length > 0.0f)
    {
        safeDirection.m_positionX /= length;
        safeDirection.m_positionY /= length;
    }
    else
    {
        safeDirection.m_positionX = 1.0f;
        safeDirection.m_positionY = 0.0f;
    }

    return safeDirection;
}

Position KitingManager::GetCircularKitingPosition(Unit* target, float angle)
{
    if (!target)
        return _bot->GetPosition();

    Position targetPos = target->GetPosition();
    Position kitingPos;

    kitingPos.m_positionX = targetPos.GetPositionX() + _optimalKitingDistance * ::std::cos(angle);
    kitingPos.m_positionY = targetPos.GetPositionY() + _optimalKitingDistance * ::std::sin(angle);
    kitingPos.m_positionZ = targetPos.GetPositionZ();

    return kitingPos;
}

Position KitingManager::GetRetreatPosition(const ::std::vector<Unit*>& threats, float distance)
{
    Position safeDirection = FindSafeKitingDirection(threats);
    Position botPos = _bot->GetPosition();

    Position retreatPos;
    retreatPos.m_positionX = botPos.GetPositionX() + safeDirection.m_positionX * distance;
    retreatPos.m_positionY = botPos.GetPositionY() + safeDirection.m_positionY * distance;
    retreatPos.m_positionZ = botPos.GetPositionZ();

    return retreatPos;
}

bool KitingManager::CanAttackWhileKiting()
{
    float optimalDistance = GetOptimalKitingDistance(_kitingTarget);
    return IsInAttackWindow() && _kitingTarget &&
           _bot->IsWithinLOSInMap(_kitingTarget) &&
           _bot->GetExactDistSq(_kitingTarget) <= (optimalDistance * optimalDistance);
}

Position KitingManager::GetAttackPosition(Unit* target)
{
    if (!target)
        return _bot->GetPosition();

    float attackRange = ::std::max(5.0f, GetOptimalKitingDistance(target) * 0.8f);
    Position targetPos = target->GetPosition();
    Position botPos = _bot->GetPosition();

    float angle = ::std::atan2(targetPos.GetPositionY() - botPos.GetPositionY(),
                           targetPos.GetPositionX() - botPos.GetPositionX());

    Position attackPos;
    attackPos.m_positionX = targetPos.GetPositionX() - attackRange * ::std::cos(angle);
    attackPos.m_positionY = targetPos.GetPositionY() - attackRange * ::std::sin(angle);
    attackPos.m_positionZ = targetPos.GetPositionZ();

    return attackPos;
}

KitingTrigger KitingManager::EvaluateKitingTriggers(const KitingContext& context)
{
    KitingTrigger triggers = KitingTrigger::NONE;

    if (context.primaryTarget)
    {
        float distance = context.currentPosition.GetExactDist(context.primaryTarget);
        if (distance < _minKitingDistance)
            triggers |= KitingTrigger::DISTANCE_TOO_CLOSE;
    }

    if (context.currentHealth < 50.0f)
        triggers |= KitingTrigger::LOW_HEALTH;

    if (context.threats.size() >= 3)
        triggers |= KitingTrigger::MULTIPLE_ENEMIES;

    if (context.isCasting)
        triggers |= KitingTrigger::CASTING_INTERRUPT;

    uint8 botClass = _bot->GetClass();
    if (botClass == CLASS_HUNTER || botClass == CLASS_MAGE || botClass == CLASS_WARLOCK)
        triggers |= KitingTrigger::FORMATION_ROLE;

    return triggers;
}

bool KitingManager::ShouldActivateKiting(const KitingContext& context)
{
    KitingTrigger triggers = EvaluateKitingTriggers(context);
    return triggers != KitingTrigger::NONE;
}

void KitingManager::UpdateKitingState()
{
    if (!_kitingActive || !_kitingTarget)
    {
        StopKiting();
        return;
    }

    if (!_kitingTarget->IsAlive() || !_bot->IsHostileTo(_kitingTarget))
    {
        StopKiting();
        return;
    }

    float distance = ::std::sqrt(_bot->GetExactDistSq(_kitingTarget)); // Calculate once from squared distance
    if (distance > _maxKitingDistance)
    {
        _currentState = KitingState::REPOSITIONING;
    }
    else if (distance >= _optimalKitingDistance * 0.9f)
    {
        _currentState = KitingState::KITING;
    }
}

void KitingManager::ExecuteCurrentPattern()
{
    if (!_kitingActive || _kitingWaypoints.empty())
        return;

    if (_currentWaypointIndex >= _kitingWaypoints.size())
        _currentWaypointIndex = 0;

    Position targetWaypoint = _kitingWaypoints[_currentWaypointIndex];

    if (_bot->GetExactDistSq(targetWaypoint) <= (2.0f * 2.0f)) // 4.0f
    {
        _currentWaypointIndex++;
        if (_currentWaypointIndex >= _kitingWaypoints.size())
        {
            if (_currentKitingType == KitingType::CIRCULAR_KITING ||
                _currentKitingType == KitingType::FIGURE_EIGHT)
            {
                _currentWaypointIndex = 0;
            }
            else
            {
                StopKiting();
                return;
            }
        }
    }
    else
    {
        ExecuteMovementToPosition(targetWaypoint);
    }
}

::std::vector<Position> KitingManager::GenerateCircularWaypoints(Unit* target, float radius, uint32 points)
{
    ::std::vector<Position> waypoints;
    if (!target)
        return waypoints;

    Position centerPos = target->GetPosition();
    waypoints.reserve(points);

    for (uint32 i = 0; i < points; ++i)
    {
        float angle = (2.0f * static_cast<float>(M_PI) * i) / points;
        Position waypoint;
        waypoint.m_positionX = centerPos.GetPositionX() + radius * ::std::cos(angle);
        waypoint.m_positionY = centerPos.GetPositionY() + radius * ::std::sin(angle);
        waypoint.m_positionZ = centerPos.GetPositionZ();
        waypoints.push_back(waypoint);
    }

    return waypoints;
}

::std::vector<Position> KitingManager::GenerateFigureEightWaypoints(Unit* target, float radius)
{
    ::std::vector<Position> waypoints;
    if (!target)
        return waypoints;

    Position centerPos = target->GetPosition();
    uint32 points = 16;

    for (uint32 i = 0; i < points; ++i)
    {
        float t = (2.0f * static_cast<float>(M_PI) * i) / points;
        float x = radius * ::std::sin(t);
        float y = radius * ::std::sin(t) * ::std::cos(t);

        Position waypoint;
        waypoint.m_positionX = centerPos.GetPositionX() + x;
        waypoint.m_positionY = centerPos.GetPositionY() + y;
        waypoint.m_positionZ = centerPos.GetPositionZ();
        waypoints.push_back(waypoint);
    }

    return waypoints;
}

bool KitingManager::ExecuteMovementToPosition(const Position& target)
{
    try
    {
        // PHASE 6B: Use Movement Arbiter with KITING priority (175)
        BotAI* botAI = dynamic_cast<BotAI*>(_bot->GetAI());
        if (botAI && botAI->GetUnifiedMovementCoordinator())
        {
            bool accepted = botAI->RequestPointMovement(
                PlayerBotMovementPriority::KITING,
                target,
                "Kiting movement",
                "KitingManager");

            if (!accepted)
            {
                TC_LOG_TRACE("playerbot.movement.arbiter",
                    "KitingManager: Kiting movement rejected for {} - higher priority active",
                    _bot->GetName());
            }
            return accepted;
        }
        else
        {
            // FALLBACK: Use BotMovementController with validated pathfinding
            if (BotAI* ai = GetBotAI(_bot))
            {
                if (ai->MoveTo(target, true))
                {
                    return true;
                }
                else
                {
                    // Final fallback to legacy if validation fails
                    _bot->GetMotionMaster()->MovePoint(0, target.GetPositionX(), target.GetPositionY(), target.GetPositionZ());
                    return true;
                }
            }
            else
            {
                // Non-bot player - use standard movement
                _bot->GetMotionMaster()->MovePoint(0, target.GetPositionX(), target.GetPositionY(), target.GetPositionZ());
                return true;
            }
        }
    }
    catch (const ::std::exception& e)
    {
        TC_LOG_ERROR("playerbot.kiting", "Failed to execute movement for bot {}: {}", _bot->GetName(), e.what());
        return false;
    }
}

bool KitingManager::IsPositionSafe(const Position& pos, const ::std::vector<Unit*>& threats)
{
    for (Unit* threat : threats)
    {
        if (!threat)
            continue;

        float distance = pos.GetExactDist(threat);
        if (distance < _minKitingDistance * 0.8f)
            return false;
    }

    return true;
}

float KitingManager::CalculateSafetyRating(const Position& pos, const ::std::vector<Unit*>& threats)
{
    float safetyRating = 100.0f;

    for (Unit* threat : threats)
    {
        if (!threat)
            continue;

        float distance = pos.GetExactDist(threat);
        float safeDistance = _minKitingDistance;

        if (distance < safeDistance)
        {
            safetyRating -= (safeDistance - distance) * 10.0f;
        }
    }

    return ::std::max(0.0f, safetyRating);
}

void KitingManager::UpdateAttackTiming()
{
    uint32 currentTime = GameTime::GetGameTimeMS();
    _inAttackWindow = currentTime >= _attackWindowStart && currentTime <= _attackWindowEnd;

    if (currentTime > _attackWindowEnd)
    {
        _attackWindowStart = currentTime + static_cast<uint32>(_currentPattern.movementWindow * 1000);
        _attackWindowEnd = _attackWindowStart + static_cast<uint32>(_currentPattern.attackWindow * 1000);
    }
}

bool KitingManager::IsInAttackWindow()
{
    return _inAttackWindow;
}

float KitingManager::CalculateRelativeSpeed(Unit* target)
{
    if (!target)
        return 0.0f;

    float targetSpeed = target->GetSpeedXY();
    float botSpeed = _bot->GetSpeedXY();

    return botSpeed - targetSpeed;
}

void KitingManager::TrackPerformance(::std::chrono::microseconds duration, const ::std::string& operation)
{
    if (duration > _metrics.maxKitingDuration)
        _metrics.maxKitingDuration = duration;

    auto currentTime = ::std::chrono::steady_clock::now();
    auto timeSinceLastUpdate = ::std::chrono::duration_cast<::std::chrono::seconds>(currentTime - _metrics.lastUpdate);

    if (timeSinceLastUpdate.count() >= 1)
    {
        _metrics.averageKitingDuration = ::std::chrono::microseconds(
            static_cast<uint64_t>(_metrics.averageKitingDuration.count() * 0.9 + duration.count() * 0.1)
        );
        _metrics.lastUpdate = currentTime;
    }
}

void KitingManager::UpdateKitingStatistics()
{
    if (!_kitingActive || !_kitingTarget)
        return;

    float currentDistance = ::std::sqrt(_bot->GetExactDistSq(_kitingTarget)); // Calculate once from squared distance
    _metrics.averageDistanceMaintained = _metrics.averageDistanceMaintained * 0.95f + currentDistance * 0.05f;

    float optimalDistance = GetOptimalKitingDistance(_kitingTarget);
    if (currentDistance >= optimalDistance * 0.9f && currentDistance <= optimalDistance * 1.1f)
    {
        _metrics.optimalDistanceRatio = _metrics.optimalDistanceRatio * 0.95f + 1.0f * 0.05f;
    }
    else
    {
        _metrics.optimalDistanceRatio = _metrics.optimalDistanceRatio * 0.95f;
    }
}

// KitingUtils implementation
float KitingUtils::CalculateOptimalKitingDistance(Player* bot, Unit* target)
{
    if (!bot || !target)
        return 20.0f;

    uint8 botClass = bot->GetClass();
    return GetClassKitingRange(botClass);
}

float KitingUtils::GetClassKitingRange(uint8 playerClass)
{
    switch (playerClass)
    {
        case CLASS_HUNTER:
            return 30.0f;
        case CLASS_MAGE:
        case CLASS_WARLOCK:
            return 25.0f;
        case CLASS_PRIEST:
        case CLASS_SHAMAN:
            return 20.0f;
        case CLASS_DRUID:
            return 15.0f;
        default:
            return 10.0f;
    }
}

bool KitingUtils::CanClassKiteEffectively(uint8 playerClass)
{
    switch (playerClass)
    {
        case CLASS_HUNTER:
        case CLASS_MAGE:
        case CLASS_WARLOCK:
        case CLASS_PRIEST:
        case CLASS_SHAMAN:
        case CLASS_DRUID:
            return true;
        default:
            return false;
    }
}

Position KitingUtils::FindBestKitingDirection(Player* bot, const ::std::vector<Unit*>& threats)
{
    if (!bot || threats.empty())
        return Position();

    Position botPos = bot->GetPosition();
    Position resultDirection(0.0f, 0.0f, 0.0f);

    for (Unit* threat : threats)
    {
        if (!threat)
            continue;

        Position threatPos = threat->GetPosition();
        float angle = ::std::atan2(botPos.GetPositionY() - threatPos.GetPositionY(),
                               botPos.GetPositionX() - threatPos.GetPositionX());

        resultDirection.m_positionX += ::std::cos(angle);
        resultDirection.m_positionY += ::std::sin(angle);
    }

    float length = ::std::sqrt(resultDirection.m_positionX * resultDirection.m_positionX +
                           resultDirection.m_positionY * resultDirection.m_positionY);

    if (length > 0.0f)
    {
        resultDirection.m_positionX /= length;
        resultDirection.m_positionY /= length;
    }

    return resultDirection;
}

bool KitingUtils::IsPositionGoodForKiting(const Position& pos, Player* bot, const ::std::vector<Unit*>& threats)
{
    if (!bot)
        return false;

    for (Unit* threat : threats)
    {
        if (!threat)
            continue;

        float distance = pos.GetExactDist(threat);
        if (distance < 10.0f)
            return false;
    }

    return true;
}

} // namespace Playerbot