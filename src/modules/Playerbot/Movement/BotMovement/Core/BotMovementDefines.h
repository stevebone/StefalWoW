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

#ifndef TRINITY_BOTMOVEMENTDEFINES_H
#define TRINITY_BOTMOVEMENTDEFINES_H

#include "Common.h"
#include "Position.h"

struct PositionSnapshot
{
    Position pos;
    uint32 timestamp;
    
    PositionSnapshot() : pos(), timestamp(0) { }
    PositionSnapshot(Position const& p, uint32 time) : pos(p), timestamp(time) { }
};

enum class MovementStateType : uint8
{
    Idle = 0,
    Ground,
    Swimming,
    Flying,
    Falling,
    Stuck
};

enum class ValidationFailureReason : uint8
{
    None = 0,
    InvalidPosition,
    OutOfBounds,
    InvalidMapId,
    NoGroundHeight,
    VoidPosition,
    CollisionDetected,
    PathBlocked,
    DestinationUnreachable,
    LiquidDanger,
    UnsafeTerrain
};

enum class StuckType : uint8
{
    None = 0,
    PositionStuck,
    ProgressStuck,
    CollisionStuck,
    PathFailureStuck
};

enum class RecoveryLevel : uint8
{
    Level1_RecalculatePath = 1,
    Level2_BackupAndRetry = 2,
    Level3_RandomNearbyPosition = 3,
    Level4_TeleportToSafePosition = 4,
    Level5_EvadeAndReset = 5
};

enum class ValidationLevel : uint8
{
    None = 0,
    Basic,
    Standard,
    Strict
};

#endif
