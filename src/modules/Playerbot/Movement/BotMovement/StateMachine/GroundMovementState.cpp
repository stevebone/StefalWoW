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

#include "GroundMovementState.h"
#include "MovementStateMachine.h"
#include "LiquidValidator.h"
#include "GroundValidator.h"
#include "Unit.h"
#include "Map.h"
#include "Log.h"
#include <cmath>

void GroundMovementState::OnEnter(MovementStateMachine* sm, MovementState* /*prevState*/)
{
    _edgeCheckTimer = 0;
    _waterCheckTimer = 0;

    Unit* owner = GetOwner(sm);
    if (owner)
        _lastPosition = owner->GetPosition();

    TC_LOG_DEBUG("movement.bot.state", "GroundMovementState: Entered ground movement state");
}

void GroundMovementState::OnExit(MovementStateMachine* sm, MovementState* /*nextState*/)
{
    TC_LOG_DEBUG("movement.bot.state", "GroundMovementState: Exiting ground movement state");
}

void GroundMovementState::Update(MovementStateMachine* sm, uint32 diff)
{
    Unit* owner = GetOwner(sm);
    if (!owner || !owner->IsInWorld())
        return;

    // Check for falling first (highest priority)
    if (CheckForFalling(sm))
    {
        TC_LOG_DEBUG("movement.bot.state", "GroundMovementState: Detected falling, transitioning");
        TransitionTo(sm, MovementStateType::Falling);
        return;
    }

    // Check for water
    _waterCheckTimer += diff;
    if (_waterCheckTimer >= WATER_CHECK_INTERVAL)
    {
        _waterCheckTimer = 0;

        if (CheckForWater(sm))
        {
            TC_LOG_DEBUG("movement.bot.state", "GroundMovementState: Detected water, transitioning to Swimming");
            TransitionTo(sm, MovementStateType::Swimming);
            return;
        }
    }

    // Check for edge (cliff detection)
    _edgeCheckTimer += diff;
    if (_edgeCheckTimer >= EDGE_CHECK_INTERVAL)
    {
        _edgeCheckTimer = 0;

        if (CheckForEdge(sm))
        {
            TC_LOG_DEBUG("movement.bot.state", "GroundMovementState: Detected edge ahead");
            // Note: Edge detection doesn't transition - it's used by path validation
            // to stop movement before falling off cliffs
        }
    }

    // Update last position
    _lastPosition = owner->GetPosition();
}

uint32 GroundMovementState::GetRequiredMovementFlags() const
{
    // Ground movement may use forward flag if moving
    return 0; // Base flags, actual movement flags set by movement generators
}

bool GroundMovementState::CheckForEdge(MovementStateMachine* sm)
{
    Unit* owner = GetOwner(sm);
    if (!owner || !owner->IsInWorld())
        return false;

    Map* map = owner->GetMap();
    if (!map)
        return false;

    // Get position ahead of the unit
    Position aheadPos = GetPositionAhead(owner, EDGE_DETECTION_DISTANCE);

    // Get ground height at current position and ahead
    float currentGroundHeight = GroundValidator::GetGroundHeight(owner);
    if (currentGroundHeight == INVALID_HEIGHT)
        return false;

    // Get ground height at ahead position
    float aheadGroundHeight = map->GetHeight(
        owner->GetPhaseShift(),
        aheadPos.GetPositionX(),
        aheadPos.GetPositionY(),
        owner->GetPositionZ(),
        true
    );

    if (aheadGroundHeight == INVALID_HEIGHT)
    {
        // No ground ahead - this is definitely an edge!
        TC_LOG_DEBUG("movement.bot.state", "GroundMovementState: No ground detected ahead - edge!");
        return true;
    }

    // Check for significant height drop
    float heightDrop = currentGroundHeight - aheadGroundHeight;
    if (heightDrop > EDGE_HEIGHT_THRESHOLD)
    {
        TC_LOG_DEBUG("movement.bot.state", "GroundMovementState: Edge detected - {} yard drop",
            heightDrop);
        return true;
    }

    return false;
}

bool GroundMovementState::CheckForWater(MovementStateMachine* sm)
{
    Unit* owner = GetOwner(sm);
    if (!owner || !owner->IsInWorld())
        return false;

    return LiquidValidator::IsSwimmingRequired(owner);
}

bool GroundMovementState::CheckForFalling(MovementStateMachine* sm)
{
    Unit* owner = GetOwner(sm);
    if (!owner || !owner->IsInWorld())
        return false;

    // Check if we're in water - can't fall in water
    if (LiquidValidator::IsSwimmingRequired(owner))
        return false;

    // Get ground height
    float groundHeight = GroundValidator::GetGroundHeight(owner);
    if (groundHeight == INVALID_HEIGHT)
        return false;

    // Check if significantly above ground
    float heightAboveGround = owner->GetPositionZ() - groundHeight;

    // Consider falling if more than 3 yards above ground
    return heightAboveGround > 3.0f;
}

Position GroundMovementState::GetPositionAhead(Unit* unit, float distance) const
{
    if (!unit)
        return Position();

    float orientation = unit->GetOrientation();
    float x = unit->GetPositionX() + distance * std::cos(orientation);
    float y = unit->GetPositionY() + distance * std::sin(orientation);
    float z = unit->GetPositionZ();

    return Position(x, y, z, orientation);
}
