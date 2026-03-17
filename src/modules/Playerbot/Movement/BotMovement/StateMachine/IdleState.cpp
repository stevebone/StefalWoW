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

#include "IdleState.h"
#include "MovementStateMachine.h"
#include "Unit.h"
#include "Log.h"

void IdleState::OnEnter(MovementStateMachine* sm, MovementState* /*prevState*/)
{
    _environmentCheckTimer = 0;

    TC_LOG_DEBUG("movement.bot.state", "IdleState: Entered idle state");

    // Ensure no movement flags are set
    Unit* owner = GetOwner(sm);
    if (owner)
    {
        // Stop any ongoing movement
        // Note: Don't call MotionMaster here to avoid circular dependencies
    }
}

void IdleState::OnExit(MovementStateMachine* sm, MovementState* /*nextState*/)
{
    TC_LOG_DEBUG("movement.bot.state", "IdleState: Exiting idle state");
}

void IdleState::Update(MovementStateMachine* sm, uint32 diff)
{
    _environmentCheckTimer += diff;

    if (_environmentCheckTimer >= ENVIRONMENT_CHECK_INTERVAL)
    {
        _environmentCheckTimer = 0;

        // Check if we should transition to a different state based on environment
        if (sm->IsInWater())
        {
            TC_LOG_DEBUG("movement.bot.state", "IdleState: Detected water, transitioning to Swimming");
            TransitionTo(sm, MovementStateType::Swimming);
            return;
        }

        if (sm->IsFalling())
        {
            TC_LOG_DEBUG("movement.bot.state", "IdleState: Detected falling, transitioning to Falling");
            TransitionTo(sm, MovementStateType::Falling);
            return;
        }
    }
}
