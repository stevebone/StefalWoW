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

#include "StuckState.h"
#include "MovementStateMachine.h"
#include "BotMovementController.h"
#include "Unit.h"
#include "Log.h"

void StuckState::OnEnter(MovementStateMachine* sm, MovementState* prevState)
{
    _recoveryAttempts = 0;
    _stuckDuration = 0;
    _recoveryTimer = 0;

    // Remember previous state to return to on recovery
    if (prevState)
        _previousStateType = prevState->GetType();
    else
        _previousStateType = MovementStateType::Idle;

    Unit* owner = GetOwner(sm);
    TC_LOG_WARN("movement.bot.state", "StuckState: Bot {} entered stuck state (was in {} state)",
        owner ? owner->GetName() : "Unknown",
        prevState ? prevState->GetName() : "null");
}

void StuckState::OnExit(MovementStateMachine* sm, MovementState* nextState)
{
    Unit* owner = GetOwner(sm);
    TC_LOG_DEBUG("movement.bot.state", "StuckState: Bot {} recovered after {}ms and {} attempts",
        owner ? owner->GetName() : "Unknown",
        _stuckDuration,
        _recoveryAttempts);
}

void StuckState::Update(MovementStateMachine* sm, uint32 diff)
{
    _stuckDuration += diff;
    _recoveryTimer += diff;

    // Check if we've been stuck too long
    if (_stuckDuration >= MAX_STUCK_DURATION)
    {
        Unit* owner = GetOwner(sm);
        TC_LOG_ERROR("movement.bot.state", "StuckState: Bot {} stuck for too long ({}ms), forcing recovery",
            owner ? owner->GetName() : "Unknown",
            _stuckDuration);

        // Force transition back (evade)
        TransitionToAppropriateState(sm);
        return;
    }

    // Try recovery at intervals
    if (_recoveryTimer >= RECOVERY_ATTEMPT_INTERVAL)
    {
        _recoveryTimer = 0;
        _recoveryAttempts++;

        if (TryRecovery(sm))
        {
            TC_LOG_DEBUG("movement.bot.state", "StuckState: Recovery attempt {} succeeded",
                _recoveryAttempts);
            TransitionToAppropriateState(sm);
            return;
        }
        else
        {
            TC_LOG_DEBUG("movement.bot.state", "StuckState: Recovery attempt {} failed ({}/{} max)",
                _recoveryAttempts, _recoveryAttempts, MAX_RECOVERY_ATTEMPTS);
        }

        // Check if max attempts reached
        if (_recoveryAttempts >= MAX_RECOVERY_ATTEMPTS)
        {
            Unit* owner = GetOwner(sm);
            TC_LOG_WARN("movement.bot.state", "StuckState: Bot {} exhausted recovery attempts, forcing transition",
                owner ? owner->GetName() : "Unknown");
            TransitionToAppropriateState(sm);
        }
    }
}

bool StuckState::TryRecovery(MovementStateMachine* sm)
{
    // Recovery will be handled by RecoveryStrategies in Phase 4
    // For now, just check if environment has changed
    Unit* owner = GetOwner(sm);
    if (!owner || !owner->IsInWorld())
        return false;

    // Simple recovery check: see if we can detect appropriate state now
    MovementStateType detectedState = sm->DetectAppropriateState();

    // If detected state is not Stuck, consider recovery successful
    return detectedState != MovementStateType::Stuck;
}

void StuckState::TransitionToAppropriateState(MovementStateMachine* sm)
{
    MovementStateType appropriateState = sm->DetectAppropriateState();

    // Don't transition back to stuck
    if (appropriateState == MovementStateType::Stuck)
        appropriateState = _previousStateType;

    // If still stuck-like, default to idle
    if (appropriateState == MovementStateType::Stuck)
        appropriateState = MovementStateType::Idle;

    TransitionTo(sm, appropriateState);
}
