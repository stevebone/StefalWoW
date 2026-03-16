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

#ifndef TRINITY_POSITIONVALIDATOR_H
#define TRINITY_POSITIONVALIDATOR_H

#include "ValidationResult.h"

struct Position;
class Unit;

class TC_GAME_API PositionValidator
{
public:
    PositionValidator() = default;
    ~PositionValidator() = default;

    static ValidationResult ValidateBounds(Position const& pos);
    static ValidationResult ValidateBounds(float x, float y, float z);

    static ValidationResult ValidateMapId(uint32 mapId);

    static ValidationResult ValidatePosition(uint32 mapId, Position const& pos);
    static ValidationResult ValidatePosition(uint32 mapId, float x, float y, float z);

    static ValidationResult ValidateUnitPosition(Unit const* unit);
};

#endif
