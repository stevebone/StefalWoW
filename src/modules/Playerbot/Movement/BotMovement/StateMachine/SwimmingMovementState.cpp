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

#include "SwimmingMovementState.h"
#include "MovementStateMachine.h"
#include "LiquidValidator.h"
#include "Unit.h"
#include "Log.h"

void SwimmingMovementState::OnEnter(MovementStateMachine* sm, MovementState* /*prevState*/)
{
    _underwaterTimer = 0;
    _waterCheckTimer = 0;
    _surfaceCheckTimer = 0;
    _isUnderwater = false;

    Unit* owner = GetOwner(sm);
    if (!owner)
        return;

    // Check if unit needs to breathe (most do, undead/forsaken don't)
    // For now, assume all bots need air
    _needsAir = true;

    // CRITICAL: Set MOVEMENTFLAG_SWIMMING
    if (!owner->HasUnitMovementFlag(MOVEMENTFLAG_SWIMMING))
    {
        owner->AddUnitMovementFlag(MOVEMENTFLAG_SWIMMING);
        TC_LOG_DEBUG("movement.bot.state", "SwimmingMovementState: Set MOVEMENTFLAG_SWIMMING for {}",
            owner->GetName());
    }

    TC_LOG_DEBUG("movement.bot.state", "SwimmingMovementState: Entered swimming state");
}

void SwimmingMovementState::OnExit(MovementStateMachine* sm, MovementState* /*nextState*/)
{
    Unit* owner = GetOwner(sm);
    if (owner)
    {
        // CRITICAL: Clear MOVEMENTFLAG_SWIMMING
        if (owner->HasUnitMovementFlag(MOVEMENTFLAG_SWIMMING))
        {
            owner->RemoveUnitMovementFlag(MOVEMENTFLAG_SWIMMING);
            TC_LOG_DEBUG("movement.bot.state", "SwimmingMovementState: Cleared MOVEMENTFLAG_SWIMMING for {}",
                owner->GetName());
        }
    }

    TC_LOG_DEBUG("movement.bot.state", "SwimmingMovementState: Exiting swimming state");
}

void SwimmingMovementState::Update(MovementStateMachine* sm, uint32 diff)
{
    Unit* owner = GetOwner(sm);
    if (!owner || !owner->IsInWorld())
        return;

    // Ensure swimming flag stays set while in this state
    if (!owner->HasUnitMovementFlag(MOVEMENTFLAG_SWIMMING))
    {
        owner->AddUnitMovementFlag(MOVEMENTFLAG_SWIMMING);
    }

    // Check if still in water
    _waterCheckTimer += diff;
    if (_waterCheckTimer >= WATER_CHECK_INTERVAL)
    {
        _waterCheckTimer = 0;

        if (!CheckStillInWater(sm))
        {
            TC_LOG_DEBUG("movement.bot.state", "SwimmingMovementState: No longer in water, transitioning to Ground");
            TransitionTo(sm, MovementStateType::Ground);
            return;
        }
    }

    // Update underwater status and handle breath
    UpdateUnderwaterStatus(sm, diff);

    // Check if need to surface for air
    _surfaceCheckTimer += diff;
    if (_surfaceCheckTimer >= SURFACE_CHECK_INTERVAL)
    {
        _surfaceCheckTimer = 0;

        if (CheckNeedToSurface(sm))
        {
            SurfaceForAir(sm);
        }
    }
}

bool SwimmingMovementState::CheckStillInWater(MovementStateMachine* sm)
{
    Unit* owner = GetOwner(sm);
    if (!owner || !owner->IsInWorld())
        return false;

    return LiquidValidator::IsSwimmingRequired(owner);
}

bool SwimmingMovementState::CheckNeedToSurface(MovementStateMachine* sm)
{
    if (!_needsAir)
        return false;

    if (!_isUnderwater)
        return false;

    return _underwaterTimer >= BREATH_WARNING_TIME;
}

void SwimmingMovementState::SurfaceForAir(MovementStateMachine* sm)
{
    Unit* owner = GetOwner(sm);
    if (!owner || !owner->IsInWorld())
        return;

    TC_LOG_DEBUG("movement.bot.state", "SwimmingMovementState: Bot needs to surface for air");

    // Get surface position
    Position surfacePos = LiquidValidator::GetSurfacePosition(owner);

    // Request movement to surface
    // Note: Actual movement will be handled by movement generators
    // For now, just log the intent
    TC_LOG_DEBUG("movement.bot.state", "SwimmingMovementState: Surface position at ({}, {}, {})",
        surfacePos.GetPositionX(), surfacePos.GetPositionY(), surfacePos.GetPositionZ());
}

void SwimmingMovementState::UpdateUnderwaterStatus(MovementStateMachine* sm, uint32 diff)
{
    Unit* owner = GetOwner(sm);
    if (!owner || !owner->IsInWorld())
        return;

    bool wasUnderwater = _isUnderwater;
    _isUnderwater = LiquidValidator::IsUnderwater(owner);

    if (_isUnderwater)
    {
        _underwaterTimer += diff;

        if (!wasUnderwater)
        {
            TC_LOG_DEBUG("movement.bot.state", "SwimmingMovementState: Bot went underwater");
        }

        // Log warnings at intervals
        if (_underwaterTimer >= BREATH_WARNING_TIME && _needsAir)
        {
            if ((_underwaterTimer / 5000) > ((_underwaterTimer - diff) / 5000))
            {
                TC_LOG_DEBUG("movement.bot.state", "SwimmingMovementState: Running low on breath ({}s remaining)",
                    (MAX_BREATH_TIME - _underwaterTimer) / 1000);
            }
        }
    }
    else
    {
        if (wasUnderwater)
        {
            TC_LOG_DEBUG("movement.bot.state", "SwimmingMovementState: Bot surfaced after {}ms underwater",
                _underwaterTimer);
        }
        _underwaterTimer = 0;
    }
}
