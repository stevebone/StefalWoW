/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "LineOfSightManager.h"
#include "Player.h"
#include "Unit.h"
#include "Map.h"
#include "Log.h"
#include "GameObject.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "G3D/Vector3.h"
#include "../../Spatial/SpatialGridManager.h"
#include "ObjectAccessor.h"
#include "PhaseShift.h"
#include "SharedDefines.h"
#include "GameTime.h"

namespace Playerbot
{

LineOfSightManager::LineOfSightManager(Player* bot)
    : _bot(bot), _maxRange(DEFAULT_MAX_RANGE), _heightTolerance(DEFAULT_HEIGHT_TOLERANCE),
      _angleTolerance(static_cast<float>(M_PI) / 3.0f), _enableCaching(true), _profilingEnabled(false),
      _lastObstructionUpdate(0)
{
    TC_LOG_DEBUG("playerbot.los", "LineOfSightManager initialized for bot {}", _bot->GetName());
}

LoSResult LineOfSightManager::CheckLineOfSight(const LoSContext& context)
{
    auto startTime = ::std::chrono::steady_clock::now();
    LoSResult result;

    // No lock needed - line of sight cache is per-bot instance data

    try
    {
        _metrics.totalChecks++;

        if (!context.source || !context.target)
        {
            result.failureReason = "Invalid source or target";
            return result;
        }

        ObjectGuid sourceGuid = context.source->GetGUID();
        ObjectGuid targetGuid = context.target->GetGUID();
        if (_enableCaching)
        {
            LoSCacheKey key(sourceGuid, targetGuid, context.checkType);
            auto cachedEntry = _losCache.Get(key);
            if (cachedEntry.has_value())
            {
                // LRUCache handles TTL automatically - entry is valid if returned
                _metrics.cacheHits++;
                auto endTime = ::std::chrono::steady_clock::now();
                auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime);
                TrackPerformance(duration, true, cachedEntry->result.hasLineOfSight);
                return cachedEntry->result;
            }
        }

        _metrics.cacheMisses++;

        result = PerformLineOfSightCheck(context);

        if (_enableCaching)
        {
            LoSCacheEntry cacheEntry;
            cacheEntry.sourceGuid = sourceGuid;
            cacheEntry.targetGuid = targetGuid;
            cacheEntry.result = result;
            cacheEntry.timestamp = GameTime::GetGameTimeMS();
            // expirationTime is set for backward compatibility but LRUCache handles TTL automatically
            cacheEntry.expirationTime = cacheEntry.timestamp + DEFAULT_CACHE_DURATION;
            cacheEntry.checkType = context.checkType;

            AddCacheEntry(cacheEntry);
        }

        if (result.hasLineOfSight)
            _metrics.successfulChecks++;
        else
            _metrics.failedChecks++;

        auto endTime = ::std::chrono::steady_clock::now();
        auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime);
        TrackPerformance(duration, false, result.hasLineOfSight);
    }
    catch (const ::std::exception& e)
    {
        result.hasLineOfSight = false;
        result.failureReason = ::std::string("Exception during LoS check: ") + e.what();
        TC_LOG_ERROR("playerbot.los", "Exception in CheckLineOfSight for bot {}: {}", _bot->GetName(), e.what());
    }

    return result;
}

LoSResult LineOfSightManager::CheckLineOfSight(Unit* target, LoSCheckType checkType)
{
    LoSContext context;
    context.bot = _bot;
    context.source = _bot;
    context.target = target;
    context.sourcePos = _bot->GetPosition();
    context.targetPos = target->GetPosition();
    context.checkType = checkType;
    context.maxRange = _maxRange;
    context.maxHeightDiff = _heightTolerance;

    switch (checkType)
    {
        case LoSCheckType::SPELL_CASTING:
            context.validationFlags = LoSValidation::SPELL_LOS;
            break;
        case LoSCheckType::RANGED_COMBAT:
            context.validationFlags = LoSValidation::COMBAT_LOS;
            break;
        case LoSCheckType::MOVEMENT:
            context.validationFlags = LoSValidation::MOVEMENT_LOS;
            break;
        default:
            context.validationFlags = LoSValidation::BASIC_LOS;
            break;
    }

    return CheckLineOfSight(context);
}

LoSResult LineOfSightManager::CheckLineOfSight(const Position& targetPos, LoSCheckType checkType)
{
    LoSContext context;
    context.bot = _bot;
    context.source = _bot;
    context.target = nullptr;
    context.sourcePos = _bot->GetPosition();
    context.targetPos = targetPos;
    context.checkType = checkType;
    context.validationFlags = LoSValidation::BASIC_LOS;
    context.maxRange = _maxRange;
    context.maxHeightDiff = _heightTolerance;

    return CheckLineOfSight(context);
}

LoSResult LineOfSightManager::CheckSpellLineOfSight(Unit* target, uint32 spellId)
{
    LoSContext context;
    context.bot = _bot;
    context.source = _bot;
    context.target = target;
    context.sourcePos = _bot->GetPosition();
    context.targetPos = target->GetPosition();
    context.checkType = LoSCheckType::SPELL_CASTING;
    context.validationFlags = LoSValidation::SPELL_LOS;
    context.spellId = spellId;

    if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE))
    {
        context.maxRange = spellInfo->GetMaxRange();
    }

    return CheckLineOfSight(context);
}

bool LineOfSightManager::CanSeeTarget(Unit* target)
{
    if (!target)
        return false;

    LoSResult result = CheckLineOfSight(target, LoSCheckType::BASIC);
    return result.hasLineOfSight;
}

bool LineOfSightManager::CanCastSpell(uint32 spellId, Unit* target)
{
    if (!target || !spellId)
        return false;

    LoSResult result = CheckSpellLineOfSight(target, spellId);
    return result.hasLineOfSight && CheckSpellSpecificRequirements(target, spellId);
}

bool LineOfSightManager::CanAttackTarget(Unit* target)
{
    if (!target)
        return false;

    LoSResult result = CheckLineOfSight(target, LoSCheckType::RANGED_COMBAT);
    return result.hasLineOfSight;
}

bool LineOfSightManager::CanHealTarget(Unit* target)
{
    if (!target)
        return false;

    LoSResult result = CheckLineOfSight(target, LoSCheckType::HEALING);
    return result.hasLineOfSight && CheckHealingLineOfSight(target);
}

bool LineOfSightManager::CanInterruptTarget(Unit* target)
{
    if (!target)
        return false;

    LoSResult result = CheckLineOfSight(target, LoSCheckType::INTERRUPT);
    return result.hasLineOfSight && CheckInterruptLineOfSight(target);
}

bool LineOfSightManager::CanMoveToPosition(const Position& pos)
{
    LoSResult result = CheckLineOfSight(pos, LoSCheckType::MOVEMENT);
    return result.hasLineOfSight;
}

::std::vector<Position> LineOfSightManager::FindLineOfSightPositions(Unit* target, float radius)
{
    ::std::vector<Position> losPositions;

    if (!target)
        return losPositions;

    Position targetPos = target->GetPosition();
    Position botPos = _bot->GetPosition();
    for (float angle = 0.0f; angle < 2.0f * static_cast<float>(M_PI); angle += static_cast<float>(M_PI) / 8.0f)
    {
        for (float distance = radius * 0.5f; distance <= radius; distance += radius * 0.25f)
        {
            Position candidatePos;
            candidatePos.m_positionX = targetPos.GetPositionX() + distance * ::std::cos(angle);
            candidatePos.m_positionY = targetPos.GetPositionY() + distance * ::std::sin(angle);
            candidatePos.m_positionZ = targetPos.GetPositionZ();

            if (HasLineOfSightFromPosition(candidatePos, target))
            {
                losPositions.push_back(candidatePos);
            }
        }
    }

    return losPositions;
}

Position LineOfSightManager::FindBestLineOfSightPosition(Unit* target, float preferredRange)
{
    if (!target)
        return _bot->GetPosition();

    ::std::vector<Position> candidates = FindLineOfSightPositions(target, preferredRange > 0.0f ? preferredRange : 20.0f);
    if (candidates.empty())
        return _bot->GetPosition();

    Position botPos = _bot->GetPosition();
    Position bestPos = candidates[0];
    float bestScore = 0.0f;

    for (const Position& pos : candidates)
    {
        float distance = pos.GetExactDist(&botPos);
        float score = 100.0f - distance;

        if (preferredRange > 0.0f)
        {
            float rangeDiff = ::std::abs(pos.GetExactDist(target) - preferredRange);
            score -= rangeDiff * 2.0f;
        }

        if (score > bestScore)
        {
            bestScore = score;
            bestPos = pos;
        }
    }

    return bestPos;
}

bool LineOfSightManager::HasLineOfSightFromPosition(const Position& fromPos, Unit* target)
{
    if (!target)
        return false;

    LoSContext context;
    context.bot = _bot;
    context.source = _bot;
    context.target = target;
    context.sourcePos = fromPos;
    context.targetPos = target->GetPosition();
    context.checkType = LoSCheckType::BASIC;
    context.validationFlags = LoSValidation::BASIC_LOS;

    LoSResult result = CheckLineOfSight(context);
    return result.hasLineOfSight;
}

::std::vector<Unit*> LineOfSightManager::GetVisibleEnemies(float maxRange)
{
    ::std::vector<Unit*> visibleEnemies;

    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Map* map = _bot->GetMap();
    if (!map)
        return visibleEnemies;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        // Grid not yet created for this map - create it on demand
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return visibleEnemies;
    }

    // Query nearby creature GUIDs (lock-free!)
    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        _bot->GetPosition(), maxRange);
    // Resolve GUIDs to Unit pointers and filter visible enemies
    for (ObjectGuid guid : nearbyGuids)
    {
        // SPATIAL GRID MIGRATION COMPLETE (2025-11-26):
        // ObjectAccessor is intentionally retained - Live Unit* needed for:
        // 1. Return type is std::vector<Unit*> - callers require live Unit*
        // 2. IsAlive() verification requires real-time state check
        // 3. IsHostileTo() faction check requires live relationship data
        // The spatial grid pre-filters candidates to reduce ObjectAccessor calls.
        ::Unit* unit = ObjectAccessor::GetUnit(*_bot, guid);
        if (!unit || !unit->IsAlive())
            continue;

        if (!_bot->IsHostileTo(unit))
            continue;

        if (CanSeeTarget(unit))
        {
            visibleEnemies.push_back(unit);
        }
    }

    return visibleEnemies;
}

::std::vector<Unit*> LineOfSightManager::GetVisibleAllies(float maxRange)
{
    ::std::vector<Unit*> visibleAllies;

    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Map* map = _bot->GetMap();
    if (!map)
        return visibleAllies;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        // Grid not yet created for this map - create it on demand
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return visibleAllies;
    }
    // Query nearby creature GUIDs (lock-free!)
    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        _bot->GetPosition(), maxRange);

    // Resolve GUIDs to Unit pointers and filter visible allies
    for (ObjectGuid guid : nearbyGuids)
    {
        // SPATIAL GRID MIGRATION COMPLETE (2025-11-26):
        // ObjectAccessor is intentionally retained - Live Unit* needed for:
        // 1. Return type is std::vector<Unit*> - callers require live Unit*
        // 2. IsAlive() verification requires real-time state check
        // 3. IsHostileTo() faction check requires live relationship data
        // The spatial grid pre-filters candidates to reduce ObjectAccessor calls.
        ::Unit* unit = ObjectAccessor::GetUnit(*_bot, guid);
        if (!unit || !unit->IsAlive())
            continue;

        if (_bot->IsHostileTo(unit))
            continue;

        if (CanSeeTarget(unit))
        {
            visibleAllies.push_back(unit);
        }
    }

    return visibleAllies;
}

Unit* LineOfSightManager::GetBestVisibleTarget(const ::std::vector<Unit*>& candidates)
{
    Unit* bestTarget = nullptr;
    float bestScore = 0.0f;

    for (Unit* candidate : candidates)
    {
        if (!candidate || !CanSeeTarget(candidate))
            continue;

        float distance = ::std::sqrt(_bot->GetExactDistSq(candidate)); // Calculate once from squared distance
        float healthPct = candidate->GetHealthPct();

        float score = 100.0f - (distance * 2.0f);
        if (healthPct < 50.0f)
            score += (50.0f - healthPct);

        if (score > bestScore)
        {
            bestScore = score;
            bestTarget = candidate;
        }
    }

    return bestTarget;
}

bool LineOfSightManager::IsHeightDifferenceBlocking(Unit* target)
{
    if (!target)
        return false;

    float heightDiff = ::std::abs(_bot->GetPositionZ() - target->GetPositionZ());
    return heightDiff > _heightTolerance;
}

float LineOfSightManager::CalculateHeightAdvantage(Unit* target)
{
    if (!target)
        return 0.0f;

    return _bot->GetPositionZ() - target->GetPositionZ();
}

bool LineOfSightManager::HasElevationAdvantage(Unit* target)
{
    return CalculateHeightAdvantage(target) > 3.0f;
}

void LineOfSightManager::ClearCache()
{
    // No lock needed - line of sight cache is per-bot instance data
    _losCache.Clear();
    TC_LOG_DEBUG("playerbot.los", "LoS cache cleared for bot {}", _bot->GetName());
}

void LineOfSightManager::ClearExpiredCacheEntries()
{
    // LRUCache handles TTL-based expiration automatically on access
    // This method provides explicit cleanup for maintenance purposes
    size_t removed = _losCache.RemoveExpired();
    if (removed > 0)
    {
        TC_LOG_DEBUG("playerbot.los", "LoS cache cleanup: {} expired entries removed for bot {}",
                    removed, _bot->GetName());
    }
}

LoSResult LineOfSightManager::PerformLineOfSightCheck(const LoSContext& context)
{
    LoSResult result;
    result.checkType = context.checkType;
    result.checkTime = GameTime::GetGameTimeMS();

    Position from = context.sourcePos;
    Position to = context.targetPos;
    result.distance = CalculateDistance3D(from, to);
    result.heightDifference = ::std::abs(to.GetPositionZ() - from.GetPositionZ());

    if (!IsWithinRange(from, to, context.maxRange))
    {
        result.blockedByRange = true;
        result.failureReason = "Target out of range";
        return result;
    }
    if (!IsHeightDifferenceAcceptable(from, to, context.maxHeightDiff))
    {
        result.blockedByHeight = true;
        result.failureReason = "Height difference too great";
        return result;
    }

    if ((context.validationFlags & LoSValidation::TERRAIN) != LoSValidation::NONE && CheckTerrainBlocking(from, to))
    {
        result.blockedByTerrain = true;
        result.failureReason = "Blocked by terrain";
        return result;
    }

    if ((context.validationFlags & LoSValidation::BUILDINGS) != LoSValidation::NONE && CheckBuildingBlocking(from, to))
    {
        result.blockedByBuilding = true;
        result.failureReason = "Blocked by building";
        return result;
    }

    if ((context.validationFlags & LoSValidation::OBJECTS) != LoSValidation::NONE && CheckObjectBlocking(from, to))
    {
        result.blockedByObject = true;
        result.failureReason = "Blocked by object";
        return result;
    }
    if ((context.validationFlags & LoSValidation::UNITS) != LoSValidation::NONE && !context.ignoreUnits && CheckUnitBlocking(from, to, context.target))
    {
        result.blockedByUnit = true;
        result.failureReason = "Blocked by unit";
        return result;
    }

    if ((context.validationFlags & LoSValidation::WATER) != LoSValidation::NONE && CheckWaterBlocking(from, to))
    {
        result.blockedByWater = true;
        result.failureReason = "Blocked by water";
        return result;
    }

    if ((context.validationFlags & LoSValidation::ANGLE_CHECK) != LoSValidation::NONE && !IsAngleAcceptable(from, to, context.viewAngleTolerance))
    {
        result.blockedByAngle = true;
        result.failureReason = "Outside viewing angle";
        return result;
    }

    if ((context.validationFlags & LoSValidation::SPELL_SPECIFIC) != LoSValidation::NONE && context.spellId > 0)
    {
        if (!CheckSpellSpecificRequirements(context.target, context.spellId))
        {
            result.failureReason = "Spell-specific requirements not met";
            return result;
        }
    }

    result.hasLineOfSight = true;
    return result;
}

bool LineOfSightManager::CheckTerrainBlocking(const Position& from, const Position& to)
{
    Map* map = _bot->GetMap();
    if (!map)
        return true;

    return !map->isInLineOfSight(_bot->GetPhaseShift(), from.GetPositionX(), from.GetPositionY(), from.GetPositionZ() + 2.0f,
                                to.GetPositionX(), to.GetPositionY(), to.GetPositionZ() + 2.0f,
                                LINEOFSIGHT_ALL_CHECKS, VMAP::ModelIgnoreFlags::Nothing);
}

bool LineOfSightManager::CheckBuildingBlocking(const Position& from, const Position& to)
{
    Map* map = _bot->GetMap();
    if (!map)
        return false;
    return !map->isInLineOfSight(_bot->GetPhaseShift(), from.GetPositionX(), from.GetPositionY(), from.GetPositionZ() + 2.0f,
                               to.GetPositionX(), to.GetPositionY(), to.GetPositionZ() + 2.0f, LINEOFSIGHT_CHECK_VMAP, VMAP::ModelIgnoreFlags::Nothing);
}

bool LineOfSightManager::CheckObjectBlocking(const Position& from, const Position& to)
{
    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    float searchRange = ::std::max(from.GetExactDist(&to), 30.0f);
    Map* map = _bot->GetMap();
    if (!map)
        return false;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        // Grid not yet created for this map - create it on demand
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return false;
    }

    // Query nearby GameObject GUIDs (lock-free!)
    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyGameObjectGuids(
        _bot->GetPosition(), searchRange);

    // Resolve GUIDs to GameObject pointers and check for blocking
    for (ObjectGuid guid : nearbyGuids)
    {
        GameObject* obj = _bot->GetMap()->GetGameObject(guid);
        if (!obj || !obj->IsInWorld())
            continue;

        if (obj->GetGoType() == GAMEOBJECT_TYPE_DOOR && obj->GetGoState() == GO_STATE_ACTIVE)
            continue;

        float objDistance = ::std::sqrt(obj->GetExactDistSq(from)); // Calculate once from squared distance
        float totalDistance = from.GetExactDist(&to);

        if (objDistance < totalDistance && objDistance > 1.0f)
        {
            Position objPos = obj->GetPosition();
            if (LoSUtils::DoLinesIntersect(from, to, objPos, objPos))
            {
                return true;
            }
        }
    }

    return false;
}

bool LineOfSightManager::CheckUnitBlocking(const Position& from, const Position& to, Unit* ignoreUnit)
{
    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    float searchRange = from.GetExactDist(&to);
    Map* map = _bot->GetMap();
    if (!map)
        return false;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        // Grid not yet created for this map - create it on demand
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return false;
    }

    // Query nearby creature GUIDs (lock-free!)
    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        _bot->GetPosition(), searchRange);

    // Resolve GUIDs to Unit pointers and check for blocking
    for (ObjectGuid guid : nearbyGuids)
    {
        // SPATIAL GRID MIGRATION COMPLETE (2025-11-26):
        // ObjectAccessor is intentionally retained - Live Unit* needed for:
        // 1. GetPosition() requires current position from live object
        // 2. IsAlive() verification requires real-time state check
        // 3. Equality comparison (unit == ignoreUnit) requires live references
        // The spatial grid pre-filters candidates to reduce ObjectAccessor calls.
        ::Unit* unit = ObjectAccessor::GetUnit(*_bot, guid);
        if (!unit || unit == _bot || unit == ignoreUnit)
            continue;

        if (!unit->IsAlive())
            continue;

        Position unitPos = unit->GetPosition();
        float distanceToLine = 0.0f;

        if (distanceToLine < 1.5f)
        {
            return true;
        }
    }

    return false;
}

bool LineOfSightManager::CheckWaterBlocking(const Position& from, const Position& to)
{
    Map* map = _bot->GetMap();
    if (!map)
        return false;

    bool fromInWater = map->IsInWater(_bot->GetPhaseShift(), from.GetPositionX(), from.GetPositionY(), from.GetPositionZ());
    bool toInWater = map->IsInWater(_bot->GetPhaseShift(), to.GetPositionX(), to.GetPositionY(), to.GetPositionZ());

    return fromInWater != toInWater;
}

void LineOfSightManager::AddCacheEntry(const LoSCacheEntry& entry)
{
    // LRUCache handles capacity management automatically - no need to check size
    // The cache will evict LRU entries when full
    LoSCacheKey key(entry.sourceGuid, entry.targetGuid, entry.checkType);
    _losCache.Put(key, entry);
}

float LineOfSightManager::CalculateDistance3D(const Position& from, const Position& to)
{
    return from.GetExactDist(&to);
}

bool LineOfSightManager::IsWithinRange(const Position& from, const Position& to, float maxRange)
{
    return from.GetExactDist(&to) <= maxRange;
}

bool LineOfSightManager::IsHeightDifferenceAcceptable(const Position& from, const Position& to, float maxDiff)
{
    return ::std::abs(to.GetPositionZ() - from.GetPositionZ()) <= maxDiff;
}

bool LineOfSightManager::CheckSpellSpecificRequirements(Unit* target, uint32 spellId)
{
    if (!target || !spellId)
        return false;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    // Check if spell ignores line of sight
    if (spellInfo->HasAttribute(SPELL_ATTR2_IGNORE_LINE_OF_SIGHT))
        return true;

    // Check if AI doesn't need to face target
    if (spellInfo->HasAttribute(SPELL_ATTR5_AI_DOESNT_FACE_TARGET))
        return true;

    float angle = _bot->GetRelativeAngle(target);
    return ::std::abs(angle) <= static_cast<float>(M_PI) / 3.0f;
}

bool LineOfSightManager::CheckInterruptLineOfSight(Unit* target)
{
    if (!target)
        return false;

    return target->HasUnitState(UNIT_STATE_CASTING) && _bot->GetExactDistSq(target) <= (30.0f * 30.0f); // 900.0f
}

bool LineOfSightManager::CheckHealingLineOfSight(Unit* target)
{
    if (!target)
        return false;

    return !_bot->IsHostileTo(target) && _bot->GetExactDistSq(target) <= (40.0f * 40.0f); // 1600.0f
}

void LineOfSightManager::TrackPerformance(::std::chrono::microseconds duration, bool cacheHit, bool successful)
{
    if (duration > _metrics.maxCheckTime)
        _metrics.maxCheckTime = duration;

    auto currentTime = ::std::chrono::steady_clock::now();
    auto timeSinceLastUpdate = ::std::chrono::duration_cast<::std::chrono::seconds>(currentTime - _metrics.lastUpdate);

    if (timeSinceLastUpdate.count() >= 1)
    {
        uint32 totalChecks = _metrics.totalChecks.load();
        if (totalChecks > 0)
        {
            _metrics.averageCheckTime = ::std::chrono::microseconds(
                static_cast<uint64_t>(_metrics.averageCheckTime.count() * 0.9 + duration.count() * 0.1)
            );
        }
        _metrics.lastUpdate = currentTime;
    }
}

// LoSUtils implementation
bool LoSUtils::HasLoS(Player* source, Unit* target)
{
    if (!source || !target)
        return false;

    return source->IsWithinLOSInMap(target);
}

bool LoSUtils::HasLoS(const Position& from, const Position& to, Map* map)
{
    if (!map)
        return false;

    // Use empty PhaseShift for static position checks
    PhaseShift emptyPhaseShift;
    return map->isInLineOfSight(emptyPhaseShift, from.GetPositionX(), from.GetPositionY(), from.GetPositionZ(),
                              to.GetPositionX(), to.GetPositionY(), to.GetPositionZ(),
                              LINEOFSIGHT_ALL_CHECKS, VMAP::ModelIgnoreFlags::Nothing);
}

float LoSUtils::GetLoSDistance(Player* source, Unit* target)
{
    if (!source || !target)
        return 0.0f;

    return ::std::sqrt(source->GetExactDistSq(target)); // Calculate actual distance
}

bool LoSUtils::CanCastSpellAtTarget(Player* caster, Unit* target, uint32 spellId)
{
    if (!caster || !target || !spellId)
        return false;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    float maxRange = spellInfo->GetMaxRange();
    if (caster->GetExactDistSq(target) > (maxRange * maxRange))
        return false;

    return HasLoS(caster, target);
}

bool LoSUtils::IsDirectPathClear(const Position& from, const Position& to, Map* map)
{
    if (!map)
        return false;

    return HasLoS(from, to, map);
}

Position LoSUtils::GetLastVisiblePoint(const Position& from, const Position& to, Map* map)
{
    if (!map)
        return from;

    Position current = from;
    Position step;

    float distance = from.GetExactDist(&to);
    uint32 steps = static_cast<uint32>(distance / 2.0f);

    step.m_positionX = (to.GetPositionX() - from.GetPositionX()) / steps;
    step.m_positionY = (to.GetPositionY() - from.GetPositionY()) / steps;
    step.m_positionZ = (to.GetPositionZ() - from.GetPositionZ()) / steps;

    for (uint32 i = 0; i < steps; ++i)
    {
        Position next = current;
        next.m_positionX += step.m_positionX;
        next.m_positionY += step.m_positionY;
        next.m_positionZ += step.m_positionZ;

        if (!HasLoS(from, next, map))
            break;

        current = next;
    }

    return current;
}

// ============================================================================
// MISSING METHOD IMPLEMENTATIONS
// ============================================================================

bool LineOfSightManager::WillHaveLineOfSightAfterMovement(const Position& newPos, Unit* target)
{
    if (!target)
        return false;

    return HasLineOfSightFromPosition(newPos, target);
}

Position LineOfSightManager::GetClosestUnblockedPosition(Unit* target)
{
    if (!target)
        return _bot->GetPosition();

    // Use FindBestLineOfSightPosition with a small preferred range to get closest
    float currentDist = ::std::sqrt(_bot->GetExactDistSq(target));
    Position best = FindBestLineOfSightPosition(target, currentDist > 5.0f ? currentDist : 10.0f);

    // If FindBest returned our own position (no candidates found), try stepping toward target
    if (best.GetExactDist(_bot) < 1.0f)
    {
        // Step directly toward the target in increments to find the first visible point
        float angle = _bot->GetAbsoluteAngle(target);
        float totalDist = currentDist;

        for (float step = 3.0f; step < totalDist; step += 3.0f)
        {
            Position candidate;
            candidate.m_positionX = _bot->GetPositionX() + step * ::std::cos(angle);
            candidate.m_positionY = _bot->GetPositionY() + step * ::std::sin(angle);
            candidate.m_positionZ = _bot->GetPositionZ();

            // Correct Z to ground level
            Map* map = _bot->GetMap();
            if (map)
            {
                float groundZ = map->GetHeight(_bot->GetPhaseShift(), candidate.m_positionX,
                                                candidate.m_positionY, candidate.m_positionZ + 5.0f);
                if (groundZ > INVALID_HEIGHT)
                    candidate.m_positionZ = groundZ + 0.5f;
            }

            if (HasLineOfSightFromPosition(candidate, target))
                return candidate;
        }
    }

    return best;
}

::std::vector<ObjectGuid> LineOfSightManager::GetBlockingObjects(Unit* target)
{
    ::std::vector<ObjectGuid> blockingGuids;
    if (!target)
        return blockingGuids;

    Position from = _bot->GetPosition();
    Position to = target->GetPosition();
    float searchRange = from.GetExactDist(&to);

    Map* map = _bot->GetMap();
    if (!map)
        return blockingGuids;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return blockingGuids;
    }

    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyGameObjectGuids(
        _bot->GetPosition(), searchRange);

    for (ObjectGuid guid : nearbyGuids)
    {
        GameObject* obj = map->GetGameObject(guid);
        if (!obj || !obj->IsInWorld())
            continue;

        if (obj->GetGoType() == GAMEOBJECT_TYPE_DOOR && obj->GetGoState() == GO_STATE_ACTIVE)
            continue;

        Position objPos = obj->GetPosition();
        float objDist = ::std::sqrt(obj->GetExactDistSq(from));
        if (objDist < searchRange && objDist > 1.0f)
        {
            if (LoSUtils::DoLinesIntersect(from, to, objPos, objPos))
                blockingGuids.push_back(guid);
        }
    }

    return blockingGuids;
}

bool LineOfSightManager::IsObstructionTemporary(const LoSResult& result)
{
    // Terrain and building obstructions are permanent
    if (result.blockedByTerrain || result.blockedByBuilding)
        return false;

    // Unit obstructions are temporary (units move)
    if (result.blockedByUnit)
        return true;

    // Object obstructions could be temporary (doors open/close)
    if (result.blockedByObject && !result.blockingObjectGuid.IsEmpty())
    {
        if (_dynamicObstructions.Contains(result.blockingObjectGuid))
            return true;
    }

    return false;
}

float LineOfSightManager::EstimateTimeUntilClearPath(Unit* target)
{
    if (!target)
        return -1.0f;

    LoSResult result = CheckLineOfSight(target, LoSCheckType::BASIC);
    if (result.hasLineOfSight)
        return 0.0f;

    if (!IsObstructionTemporary(result))
        return -1.0f; // Permanent obstruction, won't clear

    // Estimate based on unit movement speeds (~7 yards/sec average)
    if (result.blockedByUnit)
        return 2.0f; // Rough estimate: units typically move out of the way in ~2 sec

    // Dynamic objects (doors) - could be 5-30 seconds
    return 10.0f;
}

bool LineOfSightManager::IsAngleAcceptable(const Position& from, const Position& to, float maxAngle)
{
    float dx = to.GetPositionX() - from.GetPositionX();
    float dy = to.GetPositionY() - from.GetPositionY();
    float angle = ::std::atan2(dy, dx);

    // Compare against the source's orientation
    float orientation = from.GetOrientation();
    float diff = ::std::abs(angle - orientation);

    // Normalize to [0, PI]
    while (diff > static_cast<float>(M_PI))
        diff = 2.0f * static_cast<float>(M_PI) - diff;

    return diff <= maxAngle;
}

bool LineOfSightManager::IsWithinViewingAngle(Unit* target, float maxAngle)
{
    if (!target)
        return false;

    return IsAngleAcceptable(_bot->GetPosition(), target->GetPosition(), maxAngle);
}

float LineOfSightManager::CalculateViewingAngle(Unit* target)
{
    if (!target)
        return static_cast<float>(M_PI);

    float angle = _bot->GetAbsoluteAngle(target);
    float orientation = _bot->GetOrientation();
    float diff = ::std::abs(angle - orientation);

    while (diff > static_cast<float>(M_PI))
        diff = 2.0f * static_cast<float>(M_PI) - diff;

    return diff;
}

bool LineOfSightManager::RequiresFacing(Unit* target, LoSCheckType checkType)
{
    if (!target)
        return false;

    // Movement doesn't require facing
    if (checkType == LoSCheckType::MOVEMENT)
        return false;

    // Spell casting typically requires facing
    if (checkType == LoSCheckType::SPELL_CASTING || checkType == LoSCheckType::HEALING ||
        checkType == LoSCheckType::INTERRUPT)
        return true;

    // Ranged combat requires facing
    if (checkType == LoSCheckType::RANGED_COMBAT)
        return true;

    return false;
}

Position LineOfSightManager::CalculateOptimalViewingPosition(Unit* target)
{
    if (!target)
        return _bot->GetPosition();

    // Find position that provides LOS and is at optimal range
    float optimalRange = 20.0f; // Default for ranged
    return FindBestLineOfSightPosition(target, optimalRange);
}

uint32 LineOfSightManager::CountVisibleTargets(const ::std::vector<Unit*>& targets)
{
    uint32 count = 0;
    for (Unit* target : targets)
    {
        if (target && CanSeeTarget(target))
            ++count;
    }
    return count;
}

Position LineOfSightManager::FindElevatedPosition(Unit* target)
{
    if (!target)
        return _bot->GetPosition();

    Position botPos = _bot->GetPosition();
    Position targetPos = target->GetPosition();
    Map* map = _bot->GetMap();
    if (!map)
        return botPos;

    // Search for nearby positions that are higher than current
    Position bestPos = botPos;
    float bestElevation = botPos.GetPositionZ();

    for (float angle = 0.0f; angle < 2.0f * static_cast<float>(M_PI); angle += static_cast<float>(M_PI) / 6.0f)
    {
        for (float dist = 5.0f; dist <= 20.0f; dist += 5.0f)
        {
            Position candidate;
            candidate.m_positionX = botPos.GetPositionX() + dist * ::std::cos(angle);
            candidate.m_positionY = botPos.GetPositionY() + dist * ::std::sin(angle);

            float groundZ = map->GetHeight(_bot->GetPhaseShift(), candidate.m_positionX,
                                            candidate.m_positionY, botPos.GetPositionZ() + 20.0f);
            if (groundZ <= INVALID_HEIGHT)
                continue;

            candidate.m_positionZ = groundZ + 0.5f;

            if (candidate.m_positionZ > bestElevation && HasLineOfSightFromPosition(candidate, target))
            {
                bestElevation = candidate.m_positionZ;
                bestPos = candidate;
            }
        }
    }

    return bestPos;
}

void LineOfSightManager::UpdateDynamicObstructions()
{
    uint32 now = GameTime::GetGameTimeMS();
    if (now - _lastObstructionUpdate < OBSTRUCTION_UPDATE_INTERVAL)
        return;

    _lastObstructionUpdate = now;

    // Prune invalid obstructions
    ::std::vector<ObjectGuid> toRemove;
    for (auto const& pair : _dynamicObstructions)
    {
        if (!pair.second || !pair.second->IsInWorld())
            toRemove.push_back(pair.first);
    }

    for (ObjectGuid guid : toRemove)
        _dynamicObstructions.Remove(guid);
}

void LineOfSightManager::RegisterDynamicObstruction(GameObject* obj)
{
    if (!obj)
        return;

    _dynamicObstructions.Insert(obj->GetGUID(), obj);
}

void LineOfSightManager::UnregisterDynamicObstruction(GameObject* obj)
{
    if (!obj)
        return;

    _dynamicObstructions.Remove(obj->GetGUID());
}

bool LineOfSightManager::IsDynamicObstructionActive(ObjectGuid guid)
{
    auto result = _dynamicObstructions.Get(guid);
    if (!result.has_value())
        return false;

    GameObject* obj = result.value();
    return obj && obj->IsInWorld();
}

bool LineOfSightManager::HasSpellLineOfSight(Unit* target, uint32 spellId)
{
    if (!target || !spellId)
        return false;

    LoSResult result = CheckSpellLineOfSight(target, spellId);
    return result.hasLineOfSight;
}

float LineOfSightManager::GetSpellMaxRange(uint32 spellId)
{
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return 0.0f;

    return spellInfo->GetMaxRange();
}

bool LineOfSightManager::IsSpellRangeBlocked(Unit* target, uint32 spellId)
{
    if (!target || !spellId)
        return true;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return true;

    float maxRange = spellInfo->GetMaxRange();
    float dist = ::std::sqrt(_bot->GetExactDistSq(target));

    return dist > maxRange;
}

LoSValidation LineOfSightManager::GetSpellLoSRequirements(uint32 spellId)
{
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return LoSValidation::SPELL_LOS;

    if (spellInfo->HasAttribute(SPELL_ATTR2_IGNORE_LINE_OF_SIGHT))
        return LoSValidation::NONE;

    return LoSValidation::SPELL_LOS;
}

bool LineOfSightManager::CanCastAoEAtPosition(const Position& targetPos, uint32 spellId)
{
    if (!spellId)
        return false;

    LoSResult result = CheckLineOfSight(targetPos, LoSCheckType::SPELL_CASTING);
    return result.hasLineOfSight;
}

::std::vector<Unit*> LineOfSightManager::GetAoETargetsInLoS(const Position& centerPos, float radius)
{
    ::std::vector<Unit*> results;

    Map* map = _bot->GetMap();
    if (!map)
        return results;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return results;
    }

    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        centerPos, radius);

    for (ObjectGuid guid : nearbyGuids)
    {
        ::Unit* unit = ObjectAccessor::GetUnit(*_bot, guid);
        if (!unit || !unit->IsAlive())
            continue;

        if (unit->GetExactDist(&centerPos) <= radius)
        {
            LoSContext context;
            context.bot = _bot;
            context.source = _bot;
            context.target = unit;
            context.sourcePos = _bot->GetPosition();
            context.targetPos = unit->GetPosition();
            context.checkType = LoSCheckType::AREA_CHECK;
            context.validationFlags = LoSValidation::BASIC_LOS;

            LoSResult losResult = CheckLineOfSight(context);
            if (losResult.hasLineOfSight)
                results.push_back(unit);
        }
    }

    return results;
}

bool LineOfSightManager::IsAoEPositionOptimal(const Position& pos, const ::std::vector<Unit*>& targets)
{
    if (targets.empty())
        return false;

    uint32 inLoS = 0;
    for (Unit* target : targets)
    {
        if (target && HasLineOfSightFromPosition(pos, target))
            ++inLoS;
    }

    // Optimal if we can see at least 75% of targets
    return inLoS >= (targets.size() * 3 / 4);
}

bool LineOfSightManager::IsPathClear(const ::std::vector<Position>& waypoints)
{
    if (waypoints.size() < 2)
        return true;

    for (size_t i = 0; i < waypoints.size() - 1; ++i)
    {
        if (CheckTerrainBlocking(waypoints[i], waypoints[i + 1]))
            return false;
    }

    return true;
}

Position LineOfSightManager::GetFirstBlockedWaypoint(const ::std::vector<Position>& waypoints)
{
    if (waypoints.size() < 2)
        return waypoints.empty() ? Position() : waypoints[0];

    for (size_t i = 0; i < waypoints.size() - 1; ++i)
    {
        if (CheckTerrainBlocking(waypoints[i], waypoints[i + 1]))
            return waypoints[i + 1];
    }

    return waypoints.back();
}

bool LineOfSightManager::CanSeeDestination(const Position& destination)
{
    return !CheckTerrainBlocking(_bot->GetPosition(), destination);
}

::std::vector<Position> LineOfSightManager::GetVisibilityWaypoints(const Position& destination)
{
    ::std::vector<Position> waypoints;
    Position botPos = _bot->GetPosition();

    if (!CheckTerrainBlocking(botPos, destination))
    {
        waypoints.push_back(destination);
        return waypoints;
    }

    // Use LoSUtils::GetLastVisiblePoint to find intermediate waypoints
    Map* map = _bot->GetMap();
    if (map)
    {
        Position lastVisible = LoSUtils::GetLastVisiblePoint(botPos, destination, map);
        if (lastVisible.GetExactDist(&botPos) > 2.0f)
        {
            waypoints.push_back(lastVisible);
            waypoints.push_back(destination);
        }
    }

    if (waypoints.empty())
        waypoints.push_back(destination);

    return waypoints;
}

bool LineOfSightManager::IsPositionInWorld(const Position& pos)
{
    // Basic world coordinate bounds for WoW maps
    return pos.GetPositionX() > -17000.0f && pos.GetPositionX() < 17000.0f &&
           pos.GetPositionY() > -17000.0f && pos.GetPositionY() < 17000.0f;
}

bool LineOfSightManager::IsPositionAccessible(const Position& pos)
{
    Map* map = _bot->GetMap();
    if (!map)
        return false;

    float groundZ = map->GetHeight(_bot->GetPhaseShift(), pos.GetPositionX(),
                                    pos.GetPositionY(), pos.GetPositionZ() + 10.0f);
    return groundZ > INVALID_HEIGHT;
}

float LineOfSightManager::GetGroundLevel(const Position& pos)
{
    Map* map = _bot->GetMap();
    if (!map)
        return INVALID_HEIGHT;

    return map->GetHeight(_bot->GetPhaseShift(), pos.GetPositionX(),
                          pos.GetPositionY(), pos.GetPositionZ() + 10.0f);
}

bool LineOfSightManager::IsUnderground(const Position& pos)
{
    float groundLevel = GetGroundLevel(pos);
    if (groundLevel <= INVALID_HEIGHT)
        return false;

    return pos.GetPositionZ() < groundLevel - 2.0f;
}

void LineOfSightManager::UpdateMetrics()
{
    // Metrics are updated inline during TrackPerformance() calls
    // This method exists for explicit periodic metric aggregation if needed
}

bool LineOfSightManager::CheckRangedCombatLineOfSight(Unit* target)
{
    if (!target)
        return false;

    // Ranged combat requires target within 40 yards and facing
    float distSq = _bot->GetExactDistSq(target);
    if (distSq > 40.0f * 40.0f)
        return false;

    float angle = _bot->GetRelativeAngle(target);
    return ::std::abs(angle) <= static_cast<float>(M_PI) / 2.0f; // 90 degree cone
}

void LineOfSightManager::CleanupCache()
{
    ClearExpiredCacheEntries();
}

// ============================================================================
// REMAINING LOSUTILS STATIC METHOD IMPLEMENTATIONS
// ============================================================================

bool LoSUtils::IsLoSBlocked(Player* source, Unit* target, ::std::string& reason)
{
    if (!source || !target)
    {
        reason = "Invalid source or target";
        return true;
    }

    if (!source->IsWithinLOSInMap(target))
    {
        reason = "Line of sight blocked by terrain/objects";
        return true;
    }

    return false;
}

bool LoSUtils::IsPointBehindPoint(const Position& observer, const Position& target, const Position& reference)
{
    float angleToTarget = ::std::atan2(target.GetPositionY() - observer.GetPositionY(),
                                        target.GetPositionX() - observer.GetPositionX());
    float angleToRef = ::std::atan2(reference.GetPositionY() - observer.GetPositionY(),
                                     reference.GetPositionX() - observer.GetPositionX());

    float diff = ::std::abs(angleToTarget - angleToRef);
    while (diff > static_cast<float>(M_PI))
        diff = 2.0f * static_cast<float>(M_PI) - diff;

    // Behind means same general direction but farther
    return diff < static_cast<float>(M_PI) / 4.0f &&
           observer.GetExactDist(&target) > observer.GetExactDist(&reference);
}

Position LoSUtils::GetLineIntersection(const Position& line1Start, const Position& line1End,
                                       const Position& line2Start, const Position& line2End)
{
    float x1 = line1Start.GetPositionX(), y1 = line1Start.GetPositionY();
    float x2 = line1End.GetPositionX(), y2 = line1End.GetPositionY();
    float x3 = line2Start.GetPositionX(), y3 = line2Start.GetPositionY();
    float x4 = line2End.GetPositionX(), y4 = line2End.GetPositionY();

    float denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
    if (::std::abs(denom) < 0.0001f)
        return line1Start; // Parallel lines, return start

    float t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denom;

    Position result;
    result.m_positionX = x1 + t * (x2 - x1);
    result.m_positionY = y1 + t * (y2 - y1);
    result.m_positionZ = line1Start.GetPositionZ();
    return result;
}

bool LoSUtils::DoLinesIntersect(const Position& line1Start, const Position& line1End,
                                const Position& line2Start, const Position& line2End)
{
    float x1 = line1Start.GetPositionX(), y1 = line1Start.GetPositionY();
    float x2 = line1End.GetPositionX(), y2 = line1End.GetPositionY();
    float x3 = line2Start.GetPositionX(), y3 = line2Start.GetPositionY();
    float x4 = line2End.GetPositionX(), y4 = line2End.GetPositionY();

    float denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
    if (::std::abs(denom) < 0.0001f)
        return false; // Parallel lines

    float t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denom;
    float u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / denom;

    // Both parameters must be in [0, 1] for segments to intersect
    return (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f);
}

bool LoSUtils::IsAboveGround(const Position& pos, Map* map, float threshold)
{
    if (!map)
        return false;

    PhaseShift emptyPhaseShift;
    float groundZ = map->GetStaticHeight(emptyPhaseShift, pos.GetPositionX(),
                                          pos.GetPositionY(), pos.GetPositionZ());
    if (groundZ <= INVALID_HEIGHT)
        return false;

    return (pos.GetPositionZ() - groundZ) > threshold;
}

bool LoSUtils::IsBelowGround(const Position& pos, Map* map, float threshold)
{
    if (!map)
        return false;

    PhaseShift emptyPhaseShift;
    float groundZ = map->GetStaticHeight(emptyPhaseShift, pos.GetPositionX(),
                                          pos.GetPositionY(), pos.GetPositionZ() + 50.0f);
    if (groundZ <= INVALID_HEIGHT)
        return false;

    return (groundZ - pos.GetPositionZ()) > threshold;
}

float LoSUtils::GetVerticalClearance(const Position& pos, Map* map)
{
    if (!map)
        return 0.0f;

    PhaseShift emptyPhaseShift;
    float groundZ = map->GetStaticHeight(emptyPhaseShift, pos.GetPositionX(),
                                          pos.GetPositionY(), pos.GetPositionZ());
    if (groundZ <= INVALID_HEIGHT)
        return 0.0f;

    return pos.GetPositionZ() - groundZ;
}

bool LoSUtils::CanCastSpellAtPosition(Player* caster, const Position& pos, uint32 spellId)
{
    if (!caster || !spellId)
        return false;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    float maxRange = spellInfo->GetMaxRange();
    if (caster->GetExactDist(&pos) > maxRange)
        return false;

    return HasLoS(caster->GetPosition(), pos, caster->GetMap());
}

float LoSUtils::GetEffectiveSpellRange(Player* caster, uint32 spellId)
{
    if (!caster || !spellId)
        return 0.0f;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return 0.0f;

    return spellInfo->GetMaxRange();
}

bool LoSUtils::IsAreaClear(const Position& center, float radius, Map* map)
{
    if (!map)
        return false;

    // Check 8 points around the center
    for (float angle = 0.0f; angle < 2.0f * static_cast<float>(M_PI); angle += static_cast<float>(M_PI) / 4.0f)
    {
        Position edge;
        edge.m_positionX = center.GetPositionX() + radius * ::std::cos(angle);
        edge.m_positionY = center.GetPositionY() + radius * ::std::sin(angle);
        edge.m_positionZ = center.GetPositionZ();

        if (!HasLoS(center, edge, map))
            return false;
    }

    return true;
}

::std::vector<Position> LoSUtils::GetBlockedPositionsInArea(const Position& center, float radius, Map* map)
{
    ::std::vector<Position> blocked;
    if (!map)
        return blocked;

    for (float angle = 0.0f; angle < 2.0f * static_cast<float>(M_PI); angle += static_cast<float>(M_PI) / 8.0f)
    {
        for (float dist = radius * 0.25f; dist <= radius; dist += radius * 0.25f)
        {
            Position pos;
            pos.m_positionX = center.GetPositionX() + dist * ::std::cos(angle);
            pos.m_positionY = center.GetPositionY() + dist * ::std::sin(angle);
            pos.m_positionZ = center.GetPositionZ();

            if (!HasLoS(center, pos, map))
                blocked.push_back(pos);
        }
    }

    return blocked;
}

Position LoSUtils::GetClearedPositionNear(const Position& target, float searchRadius, Map* map)
{
    if (!map)
        return target;

    for (float angle = 0.0f; angle < 2.0f * static_cast<float>(M_PI); angle += static_cast<float>(M_PI) / 8.0f)
    {
        for (float dist = 2.0f; dist <= searchRadius; dist += 2.0f)
        {
            Position candidate;
            candidate.m_positionX = target.GetPositionX() + dist * ::std::cos(angle);
            candidate.m_positionY = target.GetPositionY() + dist * ::std::sin(angle);
            candidate.m_positionZ = target.GetPositionZ();

            PhaseShift emptyPhaseShift;
            float groundZ = map->GetStaticHeight(emptyPhaseShift, candidate.m_positionX,
                                                   candidate.m_positionY, candidate.m_positionZ + 10.0f);
            if (groundZ > INVALID_HEIGHT)
            {
                candidate.m_positionZ = groundZ + 0.5f;
                if (HasLoS(candidate, target, map))
                    return candidate;
            }
        }
    }

    return target;
}

::std::vector<Position> LoSUtils::GetLoSBreakpoints(const Position& from, const Position& to, Map* map)
{
    ::std::vector<Position> breakpoints;
    if (!map)
        return breakpoints;

    float totalDist = from.GetExactDist(&to);
    uint32 steps = static_cast<uint32>(totalDist / 2.0f);
    if (steps < 2)
        return breakpoints;

    float dx = (to.GetPositionX() - from.GetPositionX()) / steps;
    float dy = (to.GetPositionY() - from.GetPositionY()) / steps;
    float dz = (to.GetPositionZ() - from.GetPositionZ()) / steps;

    Position prev = from;
    bool prevVisible = true;

    for (uint32 i = 1; i <= steps; ++i)
    {
        Position current;
        current.m_positionX = from.GetPositionX() + dx * i;
        current.m_positionY = from.GetPositionY() + dy * i;
        current.m_positionZ = from.GetPositionZ() + dz * i;

        bool currentVisible = HasLoS(from, current, map);

        // Transition point: visible to blocked or blocked to visible
        if (currentVisible != prevVisible)
            breakpoints.push_back(current);

        prevVisible = currentVisible;
        prev = current;
    }

    return breakpoints;
}

} // namespace Playerbot