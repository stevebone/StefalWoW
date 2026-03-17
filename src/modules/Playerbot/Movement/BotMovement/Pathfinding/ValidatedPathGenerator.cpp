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

#include "ValidatedPathGenerator.h"
#include "CollisionValidator.h"
#include "LiquidValidator.h"
#include "GroundValidator.h"
#include "PositionValidator.h"
#include "Object.h"
#include "Unit.h"
#include "Map.h"
#include "Log.h"
#include <sstream>
#include <cmath>

ValidatedPathGenerator::ValidatedPathGenerator(WorldObject const* owner)
    : _pathGenerator(owner)
    , _owner(owner)
    , _validationLevel(ValidationLevel::Standard)
{
}

ValidatedPathGenerator::~ValidatedPathGenerator() = default;

ValidatedPath ValidatedPathGenerator::CalculateValidatedPath(Position const& dest, bool forceDest)
{
    return CalculateValidatedPath(dest.GetPositionX(), dest.GetPositionY(), dest.GetPositionZ(), forceDest);
}

ValidatedPath ValidatedPathGenerator::CalculateValidatedPath(float destX, float destY, float destZ, bool forceDest)
{
    ValidatedPath result;

    if (!_owner || !_owner->IsInWorld())
    {
        result.validationResult = ValidationResult::Failure(
            ValidationFailureReason::InvalidPosition,
            "Owner is null or not in world");
        return result;
    }

    Position dest(destX, destY, destZ, 0.0f);

    // Stage 1: Validate destination
    if (_validationLevel >= ValidationLevel::Basic)
    {
        result.validationResult = ValidateDestination(dest);
        if (!result.validationResult.isValid)
        {
            TC_LOG_DEBUG("movement.bot", "ValidatedPathGenerator: Destination validation failed: {}",
                result.validationResult.errorMessage);
            return result;
        }
    }

    // Stage 2: Generate path using underlying PathGenerator
    bool pathResult = _pathGenerator.CalculatePath(destX, destY, destZ, forceDest);

    result.pathType = _pathGenerator.GetPathType();
    result.points = _pathGenerator.GetPath();

    if (!pathResult || result.pathType == PATHFIND_NOPATH)
    {
        result.validationResult = ValidationResult::Failure(
            ValidationFailureReason::DestinationUnreachable,
            "PathGenerator failed to find path");
        return result;
    }

    // Stage 3: Validate path segments for collisions
    if (_validationLevel >= ValidationLevel::Standard)
    {
        Unit const* unit = _owner->ToUnit();
        if (unit)
        {
            result.validationResult = ValidatePathSegments(result.points);
            if (!result.validationResult.isValid)
            {
                TC_LOG_DEBUG("movement.bot", "ValidatedPathGenerator: Path segment validation failed: {}",
                    result.validationResult.errorMessage);
                return result;
            }
        }
    }

    // Stage 4: Validate environment transitions (water, etc.)
    if (_validationLevel >= ValidationLevel::Strict)
    {
        result.validationResult = ValidateEnvironmentTransitions(result.points);
        if (!result.validationResult.isValid)
        {
            TC_LOG_DEBUG("movement.bot", "ValidatedPathGenerator: Environment transition validation failed: {}",
                result.validationResult.errorMessage);
            return result;
        }
    }

    // Check for water in path
    result.requiresSwimming = PathContainsWater(result.points);
    result.containsWaterTransition = result.requiresSwimming;

    // Success
    result.validationResult = ValidationResult::Success();

    TC_LOG_DEBUG("movement.bot", "ValidatedPathGenerator: Path calculated with {} points, type 0x{:X}, swimming: {}",
        result.points.size(), static_cast<uint32>(result.pathType), result.requiresSwimming);

    return result;
}

ValidatedPath ValidatedPathGenerator::CalculateValidatedPath(Position const& start, Position const& dest, bool forceDest)
{
    ValidatedPath result;

    if (!_owner || !_owner->IsInWorld())
    {
        result.validationResult = ValidationResult::Failure(
            ValidationFailureReason::InvalidPosition,
            "Owner is null or not in world");
        return result;
    }

    // Validate start position
    if (_validationLevel >= ValidationLevel::Basic)
    {
        Unit const* unit = _owner->ToUnit();
        if (unit)
        {
            ValidationResult startValidation = PositionValidator::ValidatePosition(unit->GetMapId(), start);
            if (!startValidation.isValid)
            {
                result.validationResult = startValidation;
                return result;
            }
        }
    }

    // Generate path from specific start
    bool pathResult = _pathGenerator.CalculatePath(
        start.GetPositionX(), start.GetPositionY(), start.GetPositionZ(),
        dest.GetPositionX(), dest.GetPositionY(), dest.GetPositionZ(),
        forceDest);

    result.pathType = _pathGenerator.GetPathType();
    result.points = _pathGenerator.GetPath();

    if (!pathResult || result.pathType == PATHFIND_NOPATH)
    {
        result.validationResult = ValidationResult::Failure(
            ValidationFailureReason::DestinationUnreachable,
            "PathGenerator failed to find path from specified start");
        return result;
    }

    // Apply same validation stages
    if (_validationLevel >= ValidationLevel::Standard)
    {
        Unit const* unit = _owner->ToUnit();
        if (unit)
        {
            result.validationResult = ValidatePathSegments(result.points);
            if (!result.validationResult.isValid)
                return result;
        }
    }

    if (_validationLevel >= ValidationLevel::Strict)
    {
        result.validationResult = ValidateEnvironmentTransitions(result.points);
        if (!result.validationResult.isValid)
            return result;
    }

    result.requiresSwimming = PathContainsWater(result.points);
    result.containsWaterTransition = result.requiresSwimming;
    result.validationResult = ValidationResult::Success();

    return result;
}

ValidationResult ValidatedPathGenerator::ValidateDestination(Position const& dest)
{
    if (!_owner || !_owner->IsInWorld())
    {
        return ValidationResult::Failure(
            ValidationFailureReason::InvalidPosition,
            "Owner is null or not in world");
    }

    Unit const* unit = _owner->ToUnit();
    if (!unit)
        return ValidationResult::Success(); // Non-unit WorldObjects skip validation

    // Check position bounds
    ValidationResult boundsResult = PositionValidator::ValidatePosition(unit->GetMapId(), dest);
    if (!boundsResult.isValid)
        return boundsResult;

    // Check if destination is in dangerous liquid
    LiquidInfo liquidInfo = LiquidValidator::GetLiquidInfoAt(unit->GetMap(), dest);
    if (liquidInfo.isDangerous)
    {
        return ValidationResult::Failure(
            ValidationFailureReason::LiquidDanger,
            "Destination is in dangerous liquid (magma/slime)");
    }

    return ValidationResult::Success();
}

ValidationResult ValidatedPathGenerator::ValidatePathSegments(Movement::PointsArray const& path)
{
    if (path.size() < 2)
        return ValidationResult::Success();

    Unit const* unit = _owner->ToUnit();
    if (!unit)
        return ValidationResult::Success();

    // Check collision along each path segment
    for (size_t i = 0; i < path.size() - 1; ++i)
    {
        Position start(path[i].x, path[i].y, path[i].z, 0.0f);
        Position end(path[i + 1].x, path[i + 1].y, path[i + 1].z, 0.0f);

        // Check line of sight
        if (!CollisionValidator::HasLineOfSight(unit, start, end))
        {
            std::ostringstream oss;
            oss << "Collision detected at path segment " << i
                << " from (" << start.GetPositionX() << ", " << start.GetPositionY() << ")"
                << " to (" << end.GetPositionX() << ", " << end.GetPositionY() << ")";

            return ValidationResult::Failure(
                ValidationFailureReason::PathBlocked,
                oss.str());
        }
    }

    return ValidationResult::Success();
}

ValidationResult ValidatedPathGenerator::ValidateEnvironmentTransitions(Movement::PointsArray const& path)
{
    if (path.size() < 2)
        return ValidationResult::Success();

    Unit const* unit = _owner->ToUnit();
    if (!unit)
        return ValidationResult::Success();

    // Check for dangerous liquid transitions
    for (size_t i = 0; i < path.size() - 1; ++i)
    {
        Position from(path[i].x, path[i].y, path[i].z, 0.0f);
        Position to(path[i + 1].x, path[i + 1].y, path[i + 1].z, 0.0f);

        ValidationResult liquidResult = LiquidValidator::ValidateLiquidPath(unit, from, to);
        if (!liquidResult.isValid)
            return liquidResult;
    }

    return ValidationResult::Success();
}

void ValidatedPathGenerator::OptimizePath(Movement::PointsArray& path)
{
    if (path.size() < 3)
        return;

    // Remove redundant waypoints that are in a straight line
    Movement::PointsArray optimized;
    optimized.push_back(path[0]);

    for (size_t i = 1; i < path.size() - 1; ++i)
    {
        G3D::Vector3 const& prev = optimized.back();
        G3D::Vector3 const& curr = path[i];
        G3D::Vector3 const& next = path[i + 1];

        // Calculate direction vectors
        float dx1 = curr.x - prev.x;
        float dy1 = curr.y - prev.y;
        float dz1 = curr.z - prev.z;

        float dx2 = next.x - curr.x;
        float dy2 = next.y - curr.y;
        float dz2 = next.z - curr.z;

        // Normalize
        float len1 = std::sqrt(dx1 * dx1 + dy1 * dy1 + dz1 * dz1);
        float len2 = std::sqrt(dx2 * dx2 + dy2 * dy2 + dz2 * dz2);

        if (len1 > 0.01f && len2 > 0.01f)
        {
            dx1 /= len1; dy1 /= len1; dz1 /= len1;
            dx2 /= len2; dy2 /= len2; dz2 /= len2;

            // Check if directions are different enough to keep the point
            float dot = dx1 * dx2 + dy1 * dy2 + dz1 * dz2;
            if (dot < 0.99f) // Not collinear
            {
                optimized.push_back(curr);
            }
        }
        else
        {
            optimized.push_back(curr);
        }
    }

    optimized.push_back(path.back());
    path = std::move(optimized);
}

bool ValidatedPathGenerator::PathContainsWater(Movement::PointsArray const& path) const
{
    if (!_owner || !_owner->IsInWorld())
        return false;

    Map const* map = _owner->GetMap();
    if (!map)
        return false;

    for (auto const& point : path)
    {
        Position pos(point.x, point.y, point.z, 0.0f);
        LiquidInfo liquidInfo = LiquidValidator::GetLiquidInfoAt(map, pos);

        if (liquidInfo.ShouldSwim())
            return true;
    }

    return false;
}
