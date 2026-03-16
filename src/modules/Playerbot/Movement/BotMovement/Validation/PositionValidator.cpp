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

#include "PositionValidator.h"
#include "Position.h"
#include "Unit.h"
#include "GridDefines.h"
#include "MapManager.h"
#include <sstream>

ValidationResult PositionValidator::ValidateBounds(Position const& pos)
{
    return ValidateBounds(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ());
}

ValidationResult PositionValidator::ValidateBounds(float x, float y, float z)
{
    if (!Trinity::IsValidMapCoord(x, y, z))
    {
        std::ostringstream oss;
        oss << "Position out of bounds: (" << x << ", " << y << ", " << z << ")";
        return ValidationResult::Failure(ValidationFailureReason::OutOfBounds, oss.str());
    }

    return ValidationResult::Success();
}

ValidationResult PositionValidator::ValidateMapId(uint32 mapId)
{
    if (!MapManager::IsValidMAP(mapId))
    {
        std::ostringstream oss;
        oss << "Invalid map ID: " << mapId;
        return ValidationResult::Failure(ValidationFailureReason::InvalidMapId, oss.str());
    }

    return ValidationResult::Success();
}

ValidationResult PositionValidator::ValidatePosition(uint32 mapId, Position const& pos)
{
    return ValidatePosition(mapId, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ());
}

ValidationResult PositionValidator::ValidatePosition(uint32 mapId, float x, float y, float z)
{
    ValidationResult mapResult = ValidateMapId(mapId);
    if (!mapResult.isValid)
        return mapResult;

    ValidationResult boundsResult = ValidateBounds(x, y, z);
    if (!boundsResult.isValid)
        return boundsResult;

    return ValidationResult::Success();
}

ValidationResult PositionValidator::ValidateUnitPosition(Unit const* unit)
{
    if (!unit)
    {
        return ValidationResult::Failure(
            ValidationFailureReason::InvalidPosition,
            "Unit is null");
    }

    if (!unit->IsInWorld())
    {
        return ValidationResult::Failure(
            ValidationFailureReason::InvalidPosition,
            "Unit is not in world");
    }

    return ValidatePosition(unit->GetMapId(), *unit);
}
