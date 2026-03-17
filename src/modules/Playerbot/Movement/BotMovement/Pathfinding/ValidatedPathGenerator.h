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

#ifndef TRINITY_VALIDATEDPATHGENERATOR_H
#define TRINITY_VALIDATEDPATHGENERATOR_H

#include "Define.h"
#include "PathGenerator.h"
#include "ValidationResult.h"
#include "BotMovementDefines.h"
#include "MoveSplineInitArgs.h"
#include <memory>

class WorldObject;
class Unit;
struct Position;

struct ValidatedPath
{
    Movement::PointsArray points;
    PathType pathType = PATHFIND_BLANK;
    ValidationResult validationResult;
    bool requiresSwimming = false;
    bool containsWaterTransition = false;

    bool IsValid() const { return validationResult.isValid && pathType != PATHFIND_NOPATH; }
    bool IsComplete() const { return (pathType & PATHFIND_NORMAL) != 0; }
    bool IsPartial() const { return (pathType & PATHFIND_INCOMPLETE) != 0; }
};

class TC_GAME_API ValidatedPathGenerator
{
public:
    explicit ValidatedPathGenerator(WorldObject const* owner);
    ~ValidatedPathGenerator();

    ValidatedPathGenerator(ValidatedPathGenerator const&) = delete;
    ValidatedPathGenerator& operator=(ValidatedPathGenerator const&) = delete;

    // Calculate and validate a path to destination
    ValidatedPath CalculateValidatedPath(Position const& dest, bool forceDest = false);
    ValidatedPath CalculateValidatedPath(float destX, float destY, float destZ, bool forceDest = false);

    // Calculate path from specific start position
    ValidatedPath CalculateValidatedPath(Position const& start, Position const& dest, bool forceDest = false);

    // Get underlying PathGenerator for advanced usage
    PathGenerator& GetPathGenerator() { return _pathGenerator; }
    PathGenerator const& GetPathGenerator() const { return _pathGenerator; }

    // Configuration
    void SetValidationLevel(ValidationLevel level) { _validationLevel = level; }
    ValidationLevel GetValidationLevel() const { return _validationLevel; }

    void SetUseStraightPath(bool use) { _pathGenerator.SetUseStraightPath(use); }
    void SetPathLengthLimit(float distance) { _pathGenerator.SetPathLengthLimit(distance); }
    void SetUseRaycast(bool use) { _pathGenerator.SetUseRaycast(use); }

    // Get last calculated path info
    Movement::PointsArray const& GetPath() const { return _pathGenerator.GetPath(); }
    PathType GetPathType() const { return _pathGenerator.GetPathType(); }
    float GetPathLength() const { return _pathGenerator.GetPathLength(); }

private:
    PathGenerator _pathGenerator;
    WorldObject const* _owner;
    ValidationLevel _validationLevel;

    // Validation stages
    ValidationResult ValidateDestination(Position const& dest);
    ValidationResult ValidatePathSegments(Movement::PointsArray const& path);
    ValidationResult ValidateEnvironmentTransitions(Movement::PointsArray const& path);

    // Path optimization
    void OptimizePath(Movement::PointsArray& path);

    // Check for water in path
    bool PathContainsWater(Movement::PointsArray const& path) const;
};

#endif // TRINITY_VALIDATEDPATHGENERATOR_H
