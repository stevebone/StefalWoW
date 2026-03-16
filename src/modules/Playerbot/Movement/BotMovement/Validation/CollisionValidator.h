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

#ifndef TRINITY_COLLISIONVALIDATOR_H
#define TRINITY_COLLISIONVALIDATOR_H

#include "Define.h"
#include "ValidationResult.h"
#include "MoveSplineInitArgs.h"
#include <vector>

struct Position;
class Unit;
class Map;

enum class CollisionCheckType : uint8
{
    None            = 0x00,
    LineOfSight     = 0x01,
    VMapCollision   = 0x02,
    DynamicObjects  = 0x04,
    All             = 0xFF
};

struct CollisionCheckResult
{
    bool hasCollision = false;
    Position collisionPoint;
    float distanceToCollision = 0.0f;
    CollisionCheckType collisionType = CollisionCheckType::None;
    std::string description;
};

class TC_GAME_API CollisionValidator
{
public:
    CollisionValidator() = default;
    ~CollisionValidator() = default;

    // Check line of sight between two points
    static bool HasLineOfSight(Unit const* unit, Position const& start, Position const& end);
    static bool HasLineOfSight(Map const* map, uint32 phaseMask, Position const& start, Position const& end);

    // Check if a path segment is collision-free
    static ValidationResult ValidatePathSegment(Unit const* unit, Position const& start, Position const& end);

    // Check collision along entire path
    static ValidationResult ValidatePath(Unit const* unit, Movement::PointsArray const& path);

    // Check if position is inside geometry (stuck in wall)
    static bool IsInsideGeometry(Unit const* unit, Position const& pos);
    static bool IsInsideGeometry(Map const* map, uint32 mapId, Position const& pos);

    // Detailed collision check with collision point
    static CollisionCheckResult CheckCollision(Unit const* unit, Position const& start, Position const& end, CollisionCheckType checkType = CollisionCheckType::All);

    // Find the last valid position before collision
    static Position FindLastValidPosition(Unit const* unit, Position const& start, Position const& end, float stepSize = 1.0f);

    // Check if movement from current position to target would collide
    static bool WouldCollide(Unit const* unit, Position const& target);

private:
    // Height offset for LOS checks to account for unit height
    static constexpr float LOS_HEIGHT_OFFSET = 2.0f;

    // Minimum distance to consider a collision significant
    static constexpr float MIN_COLLISION_DISTANCE = 0.5f;

    // Step size for iterative collision checking
    static constexpr float COLLISION_CHECK_STEP = 2.0f;
};

#endif // TRINITY_COLLISIONVALIDATOR_H
