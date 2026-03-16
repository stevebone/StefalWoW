/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "CollisionValidator.h"
#include "Unit.h"
#include "Map.h"
#include "Position.h"
#include "Models/ModelIgnoreFlags.h"
#include "Log.h"
#include <sstream>
#include <cmath>

bool CollisionValidator::HasLineOfSight(Unit const* unit, Position const& start, Position const& end)
{
    if (!unit || !unit->IsInWorld())
        return false;

    Map const* map = unit->GetMap();
    if (!map)
        return false;

    // Use empty PhaseShift - the HasLineOfSight overload ignores it anyway
    return HasLineOfSight(map, 0, start, end);
}

bool CollisionValidator::HasLineOfSight(Map const* map, uint32 /*phaseMask*/, Position const& start, Position const& end)
{
    if (!map)
        return false;

    // Add height offset for more realistic LOS check
    float startZ = start.GetPositionZ() + LOS_HEIGHT_OFFSET;
    float endZ = end.GetPositionZ() + LOS_HEIGHT_OFFSET;

    PhaseShift emptyPhaseShift;
    return map->isInLineOfSight(
        emptyPhaseShift,
        start.GetPositionX(), start.GetPositionY(), startZ,
        end.GetPositionX(), end.GetPositionY(), endZ,
        LINEOFSIGHT_ALL_CHECKS,
        VMAP::ModelIgnoreFlags::Nothing
    );
}

ValidationResult CollisionValidator::ValidatePathSegment(Unit const* unit, Position const& start, Position const& end)
{
    if (!unit || !unit->IsInWorld())
    {
        return ValidationResult::Failure(
            ValidationFailureReason::InvalidPosition,
            "Unit is null or not in world");
    }

    if (!HasLineOfSight(unit, start, end))
    {
        std::ostringstream oss;
        oss << "Collision detected between ("
            << start.GetPositionX() << ", " << start.GetPositionY() << ", " << start.GetPositionZ()
            << ") and ("
            << end.GetPositionX() << ", " << end.GetPositionY() << ", " << end.GetPositionZ()
            << ")";

        return ValidationResult::Failure(
            ValidationFailureReason::CollisionDetected,
            oss.str());
    }

    return ValidationResult::Success();
}

ValidationResult CollisionValidator::ValidatePath(Unit const* unit, Movement::PointsArray const& path)
{
    if (!unit || !unit->IsInWorld())
    {
        return ValidationResult::Failure(
            ValidationFailureReason::InvalidPosition,
            "Unit is null or not in world");
    }

    if (path.size() < 2)
    {
        return ValidationResult::Success(); // Empty or single-point path is valid
    }

    // Check collision between each consecutive pair of points
    for (size_t i = 0; i < path.size() - 1; ++i)
    {
        Position start(path[i].x, path[i].y, path[i].z, 0.0f);
        Position end(path[i + 1].x, path[i + 1].y, path[i + 1].z, 0.0f);

        ValidationResult segmentResult = ValidatePathSegment(unit, start, end);
        if (!segmentResult.isValid)
        {
            std::ostringstream oss;
            oss << "Path segment " << i << " collision: " << segmentResult.errorMessage;
            return ValidationResult::Failure(
                ValidationFailureReason::PathBlocked,
                oss.str());
        }
    }

    return ValidationResult::Success();
}

bool CollisionValidator::IsInsideGeometry(Unit const* unit, Position const& pos)
{
    if (!unit || !unit->IsInWorld())
        return true; // Assume inside if can't check

    return IsInsideGeometry(unit->GetMap(), unit->GetMapId(), pos);
}

bool CollisionValidator::IsInsideGeometry(Map const* map, uint32 /*mapId*/, Position const& pos)
{
    if (!map)
        return true;

    // Check if we can see in any direction from this point
    // If we can't see in any direction, we're probably inside geometry
    static const float TEST_DISTANCE = 5.0f;
    static const int NUM_DIRECTIONS = 8;

    int validDirections = 0;

    for (int i = 0; i < NUM_DIRECTIONS; ++i)
    {
        float angle = (2.0f * M_PI * i) / NUM_DIRECTIONS;
        float testX = pos.GetPositionX() + TEST_DISTANCE * std::cos(angle);
        float testY = pos.GetPositionY() + TEST_DISTANCE * std::sin(angle);

        Position testPos(testX, testY, pos.GetPositionZ(), 0.0f);

        PhaseShift emptyPhaseShift;
        if (map->isInLineOfSight(
            emptyPhaseShift,
            pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ() + LOS_HEIGHT_OFFSET,
            testX, testY, pos.GetPositionZ() + LOS_HEIGHT_OFFSET,
            LINEOFSIGHT_ALL_CHECKS,
            VMAP::ModelIgnoreFlags::Nothing))
        {
            validDirections++;
        }
    }

    // If we can see in less than half the directions, we're probably stuck
    return validDirections < NUM_DIRECTIONS / 2;
}

CollisionCheckResult CollisionValidator::CheckCollision(Unit const* unit, Position const& start, Position const& end, CollisionCheckType checkType)
{
    CollisionCheckResult result;
    result.hasCollision = false;

    if (!unit || !unit->IsInWorld())
    {
        result.hasCollision = true;
        result.description = "Unit is null or not in world";
        return result;
    }

    if (checkType == CollisionCheckType::None)
        return result;

    Map const* map = unit->GetMap();
    if (!map)
    {
        result.hasCollision = true;
        result.description = "Map is null";
        return result;
    }

    float dx = end.GetPositionX() - start.GetPositionX();
    float dy = end.GetPositionY() - start.GetPositionY();
    float dz = end.GetPositionZ() - start.GetPositionZ();
    float distance = std::sqrt(dx * dx + dy * dy + dz * dz);

    if (distance < MIN_COLLISION_DISTANCE)
        return result;

    // Normalize direction
    dx /= distance;
    dy /= distance;
    dz /= distance;

    // Step through the path and check for collisions
    float stepSize = COLLISION_CHECK_STEP;
    float currentDist = 0.0f;

    Position lastValid = start;

    while (currentDist < distance)
    {
        float nextDist = std::min(currentDist + stepSize, distance);

        float testX = start.GetPositionX() + dx * nextDist;
        float testY = start.GetPositionY() + dy * nextDist;
        float testZ = start.GetPositionZ() + dz * nextDist;

        Position testPos(testX, testY, testZ, 0.0f);

        PhaseShift emptyPhaseShift;
        bool hasLOS = map->isInLineOfSight(
            emptyPhaseShift,
            lastValid.GetPositionX(), lastValid.GetPositionY(), lastValid.GetPositionZ() + LOS_HEIGHT_OFFSET,
            testX, testY, testZ + LOS_HEIGHT_OFFSET,
            LINEOFSIGHT_ALL_CHECKS,
            VMAP::ModelIgnoreFlags::Nothing
        );

        if (!hasLOS)
        {
            result.hasCollision = true;
            result.collisionPoint = lastValid;
            result.distanceToCollision = currentDist;
            result.collisionType = CollisionCheckType::LineOfSight;

            std::ostringstream oss;
            oss << "Collision at distance " << currentDist << " from start";
            result.description = oss.str();

            return result;
        }

        lastValid = testPos;
        currentDist = nextDist;
    }

    return result;
}

Position CollisionValidator::FindLastValidPosition(Unit const* unit, Position const& start, Position const& end, float stepSize)
{
    if (!unit || !unit->IsInWorld())
        return start;

    CollisionCheckResult collision = CheckCollision(unit, start, end, CollisionCheckType::All);

    if (!collision.hasCollision)
        return end;

    // If collision detected, refine the position using smaller steps
    float dx = end.GetPositionX() - start.GetPositionX();
    float dy = end.GetPositionY() - start.GetPositionY();
    float dz = end.GetPositionZ() - start.GetPositionZ();
    float distance = std::sqrt(dx * dx + dy * dy + dz * dz);

    if (distance < stepSize)
        return start;

    dx /= distance;
    dy /= distance;
    dz /= distance;

    Position lastValid = start;
    float currentDist = 0.0f;

    Map const* map = unit->GetMap();
    if (!map)
        return start;

    while (currentDist < distance)
    {
        float nextDist = std::min(currentDist + stepSize, distance);

        float testX = start.GetPositionX() + dx * nextDist;
        float testY = start.GetPositionY() + dy * nextDist;
        float testZ = start.GetPositionZ() + dz * nextDist;

        PhaseShift emptyPhaseShift;
        bool hasLOS = map->isInLineOfSight(
            emptyPhaseShift,
            lastValid.GetPositionX(), lastValid.GetPositionY(), lastValid.GetPositionZ() + LOS_HEIGHT_OFFSET,
            testX, testY, testZ + LOS_HEIGHT_OFFSET,
            LINEOFSIGHT_ALL_CHECKS,
            VMAP::ModelIgnoreFlags::Nothing
        );

        if (!hasLOS)
            break;

        lastValid = Position(testX, testY, testZ, 0.0f);
        currentDist = nextDist;
    }

    return lastValid;
}

bool CollisionValidator::WouldCollide(Unit const* unit, Position const& target)
{
    if (!unit || !unit->IsInWorld())
        return true;

    Position current = unit->GetPosition();
    return !HasLineOfSight(unit, current, target);
}
