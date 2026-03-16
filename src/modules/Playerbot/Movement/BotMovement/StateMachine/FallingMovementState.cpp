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

#include "FallingMovementState.h"
#include "MovementStateMachine.h"
#include "LiquidValidator.h"
#include "GroundValidator.h"
#include "Unit.h"
#include "Log.h"
#include <cmath>

void FallingMovementState::OnEnter(MovementStateMachine* sm, MovementState* /*prevState*/)
{
    _fallDuration = 0;
    _landingCheckTimer = 0;

    Unit* owner = GetOwner(sm);
    if (!owner)
        return;

    // Record fall start height
    _fallStartHeight = owner->GetPositionZ();

    // CRITICAL: Set MOVEMENTFLAG_FALLING
    if (!owner->HasUnitMovementFlag(MOVEMENTFLAG_FALLING))
    {
        owner->AddUnitMovementFlag(MOVEMENTFLAG_FALLING);
        TC_LOG_DEBUG("movement.bot.state", "FallingMovementState: Set MOVEMENTFLAG_FALLING for {}",
            owner->GetName());
    }

    TC_LOG_DEBUG("movement.bot.state", "FallingMovementState: Entered falling state at height {}",
        _fallStartHeight);
}

void FallingMovementState::OnExit(MovementStateMachine* sm, MovementState* nextState)
{
    Unit* owner = GetOwner(sm);
    if (owner)
    {
        // CRITICAL: Clear MOVEMENTFLAG_FALLING
        if (owner->HasUnitMovementFlag(MOVEMENTFLAG_FALLING))
        {
            owner->RemoveUnitMovementFlag(MOVEMENTFLAG_FALLING);
            TC_LOG_DEBUG("movement.bot.state", "FallingMovementState: Cleared MOVEMENTFLAG_FALLING for {}",
                owner->GetName());
        }

        // Calculate and log fall damage if landing on ground
        if (nextState && nextState->GetType() == MovementStateType::Ground)
        {
            float fallHeight = _fallStartHeight - owner->GetPositionZ();
            if (fallHeight > SAFE_FALL_HEIGHT)
            {
                float damage = CalculateFallDamage(fallHeight);
                TC_LOG_DEBUG("movement.bot.state", "FallingMovementState: Fall of {} yards would deal {} damage",
                    fallHeight, damage);
            }
        }
    }

    TC_LOG_DEBUG("movement.bot.state", "FallingMovementState: Exiting falling state after {}ms",
        _fallDuration);
}

void FallingMovementState::Update(MovementStateMachine* sm, uint32 diff)
{
    Unit* owner = GetOwner(sm);
    if (!owner || !owner->IsInWorld())
        return;

    _fallDuration += diff;

    // Ensure falling flag stays set while in this state
    if (!owner->HasUnitMovementFlag(MOVEMENTFLAG_FALLING))
    {
        owner->AddUnitMovementFlag(MOVEMENTFLAG_FALLING);
    }

    // Check for landing
    _landingCheckTimer += diff;
    if (_landingCheckTimer >= LANDING_CHECK_INTERVAL)
    {
        _landingCheckTimer = 0;

        // Check for water landing first
        if (CheckForWaterLanding(sm))
        {
            TC_LOG_DEBUG("movement.bot.state", "FallingMovementState: Landed in water, transitioning to Swimming");
            TransitionTo(sm, MovementStateType::Swimming);
            return;
        }

        // Check for ground landing
        if (CheckForLanding(sm))
        {
            float fallHeight = _fallStartHeight - owner->GetPositionZ();
            TC_LOG_DEBUG("movement.bot.state", "FallingMovementState: Landed on ground after falling {} yards",
                fallHeight);
            TransitionTo(sm, MovementStateType::Ground);
            return;
        }
    }

    // Apply falling physics (gravity)
    ApplyFallingPhysics(sm, diff);
}

bool FallingMovementState::CheckForLanding(MovementStateMachine* sm)
{
    Unit* owner = GetOwner(sm);
    if (!owner || !owner->IsInWorld())
        return false;

    // Check if in water (water landing handled separately)
    if (LiquidValidator::IsSwimmingRequired(owner))
        return false;

    // Get ground height
    float groundHeight = GroundValidator::GetGroundHeight(owner);
    if (groundHeight == INVALID_HEIGHT)
        return false;

    // Check distance to ground
    float heightAboveGround = owner->GetPositionZ() - groundHeight;
    return heightAboveGround <= LANDING_HEIGHT_THRESHOLD;
}

bool FallingMovementState::CheckForWaterLanding(MovementStateMachine* sm)
{
    Unit* owner = GetOwner(sm);
    if (!owner || !owner->IsInWorld())
        return false;

    return LiquidValidator::IsSwimmingRequired(owner);
}

float FallingMovementState::CalculateFallDamage(float fallHeight) const
{
    if (fallHeight <= SAFE_FALL_HEIGHT)
        return 0.0f;

    if (fallHeight >= FATAL_FALL_HEIGHT)
        return 100.0f; // Percentage of max health

    // Linear interpolation between safe and fatal
    float damagePercent = ((fallHeight - SAFE_FALL_HEIGHT) / (FATAL_FALL_HEIGHT - SAFE_FALL_HEIGHT)) * 100.0f;
    return std::min(damagePercent, 100.0f);
}

void FallingMovementState::ApplyFallingPhysics(MovementStateMachine* sm, uint32 diff)
{
    Unit* owner = GetOwner(sm);
    if (!owner || !owner->IsInWorld())
        return;

    // Note: TrinityCore handles actual falling physics via spline movement
    // This method is here for future enhancements or custom falling behavior

    // Calculate current fall velocity using physics
    // v = g * t (where g = gravity, t = time falling)
    float fallTimeSeconds = static_cast<float>(_fallDuration) / 1000.0f;
    float currentVelocity = GRAVITY * fallTimeSeconds;

    // Log for debugging
    if (_fallDuration % 500 == 0) // Every 500ms
    {
        float currentHeight = owner->GetPositionZ();
        float fallDistance = _fallStartHeight - currentHeight;

        TC_LOG_DEBUG("movement.bot.state", "FallingMovementState: Falling for {}ms, velocity {}y/s, dropped {} yards",
            _fallDuration, currentVelocity, fallDistance);
    }
}
