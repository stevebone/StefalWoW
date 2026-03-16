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

#include "MovementStateMachine.h"
#include "MovementState.h"
#include "IdleState.h"
#include "GroundMovementState.h"
#include "SwimmingMovementState.h"
#include "FallingMovementState.h"
#include "StuckState.h"
#include "BotMovementController.h"
#include "LiquidValidator.h"
#include "GroundValidator.h"
#include "Unit.h"
#include "Log.h"

MovementStateMachine::MovementStateMachine(BotMovementController* controller)
    : _controller(controller)
    , _currentState(nullptr)
    , _currentStateType(MovementStateType::Idle)
    , _pendingStateType(MovementStateType::Idle)
    , _hasPendingTransition(false)
    , _timeInCurrentState(0)
{
    InitializeStates();

    // Start in Idle state
    _currentState = GetStateInstance(MovementStateType::Idle);
    if (_currentState)
        _currentState->OnEnter(this, nullptr);
}

MovementStateMachine::~MovementStateMachine()
{
    if (_currentState)
        _currentState->OnExit(this, nullptr);
}

void MovementStateMachine::InitializeStates()
{
    _states[static_cast<size_t>(MovementStateType::Idle)] = std::make_unique<IdleState>();
    _states[static_cast<size_t>(MovementStateType::Ground)] = std::make_unique<GroundMovementState>();
    _states[static_cast<size_t>(MovementStateType::Swimming)] = std::make_unique<SwimmingMovementState>();
    _states[static_cast<size_t>(MovementStateType::Flying)] = nullptr; // Not implemented yet
    _states[static_cast<size_t>(MovementStateType::Falling)] = std::make_unique<FallingMovementState>();
    _states[static_cast<size_t>(MovementStateType::Stuck)] = std::make_unique<StuckState>();
}

MovementState* MovementStateMachine::GetStateInstance(MovementStateType type)
{
    size_t index = static_cast<size_t>(type);
    if (index >= _states.size())
        return nullptr;

    return _states[index].get();
}

void MovementStateMachine::Update(uint32 diff)
{
    // Process any pending state transition
    if (_hasPendingTransition)
        ProcessPendingTransition();

    // Update current state
    if (_currentState)
    {
        _currentState->Update(this, diff);
        _timeInCurrentState += diff;
    }

    // Apply movement flags based on current state
    ApplyStateMovementFlags();
}

void MovementStateMachine::TransitionTo(MovementStateType newState)
{
    if (newState == _currentStateType && !_hasPendingTransition)
        return;

    _pendingStateType = newState;
    _hasPendingTransition = true;

    TC_LOG_DEBUG("movement.bot.state", "MovementStateMachine: Pending transition from {} to {}",
        _currentState ? _currentState->GetName() : "null",
        static_cast<int>(newState));
}

void MovementStateMachine::ProcessPendingTransition()
{
    if (!_hasPendingTransition)
        return;

    _hasPendingTransition = false;

    MovementState* newState = GetStateInstance(_pendingStateType);
    if (!newState)
    {
        TC_LOG_ERROR("movement.bot.state", "MovementStateMachine: No state instance for type {}",
            static_cast<int>(_pendingStateType));
        return;
    }

    MovementState* oldState = _currentState;
    MovementStateType oldStateType = _currentStateType;

    TC_LOG_DEBUG("movement.bot.state", "MovementStateMachine: Transitioning from {} to {}",
        oldState ? oldState->GetName() : "null", newState->GetName());

    // Exit old state
    if (oldState)
    {
        ClearStateMovementFlags();
        oldState->OnExit(this, newState);
    }

    // Update current state
    _currentState = newState;
    _currentStateType = _pendingStateType;
    _timeInCurrentState = 0;

    // Enter new state
    _currentState->OnEnter(this, oldState);

    // Apply new state's movement flags
    ApplyStateMovementFlags();
}

Unit* MovementStateMachine::GetOwner() const
{
    return _controller ? _controller->GetOwner() : nullptr;
}

bool MovementStateMachine::IsOnGround() const
{
    Unit* owner = GetOwner();
    if (!owner || !owner->IsInWorld())
        return true;

    // Not on ground if swimming or flying
    if (IsInWater() || IsFlying())
        return false;

    // Check ground height
    float groundHeight = GroundValidator::GetGroundHeight(owner);
    if (groundHeight == INVALID_HEIGHT)
        return false;

    float heightDiff = owner->GetPositionZ() - groundHeight;
    return heightDiff < 2.0f && heightDiff > -1.0f;
}

bool MovementStateMachine::IsInWater() const
{
    Unit* owner = GetOwner();
    if (!owner || !owner->IsInWorld())
        return false;

    return LiquidValidator::IsSwimmingRequired(owner);
}

bool MovementStateMachine::IsFalling() const
{
    Unit* owner = GetOwner();
    if (!owner || !owner->IsInWorld())
        return false;

    // Check if significantly above ground without being in water or flying
    if (IsInWater() || IsFlying())
        return false;

    float groundHeight = GroundValidator::GetGroundHeight(owner);
    if (groundHeight == INVALID_HEIGHT)
        return false;

    float heightAboveGround = owner->GetPositionZ() - groundHeight;
    return heightAboveGround > 3.0f; // Consider falling if more than 3 yards above ground
}

bool MovementStateMachine::IsFlying() const
{
    Unit* owner = GetOwner();
    if (!owner || !owner->IsInWorld())
        return false;

    // Check if unit has flying capability and is using it
    return owner->IsFlying() || owner->HasUnitMovementFlag(MOVEMENTFLAG_CAN_FLY);
}

void MovementStateMachine::ApplyStateMovementFlags()
{
    if (!_currentState)
        return;

    Unit* owner = GetOwner();
    if (!owner || !owner->IsInWorld())
        return;

    uint32 requiredFlags = _currentState->GetRequiredMovementFlags();

    // Handle swimming flag specially
    if (_currentStateType == MovementStateType::Swimming)
    {
        if (!owner->HasUnitMovementFlag(MOVEMENTFLAG_SWIMMING))
        {
            SetMovementFlag(MOVEMENTFLAG_SWIMMING, true);
            TC_LOG_DEBUG("movement.bot.state", "MovementStateMachine: Set MOVEMENTFLAG_SWIMMING");
        }
    }
    else
    {
        if (owner->HasUnitMovementFlag(MOVEMENTFLAG_SWIMMING))
        {
            SetMovementFlag(MOVEMENTFLAG_SWIMMING, false);
            TC_LOG_DEBUG("movement.bot.state", "MovementStateMachine: Cleared MOVEMENTFLAG_SWIMMING");
        }
    }

    // Handle falling flag
    if (_currentStateType == MovementStateType::Falling)
    {
        if (!owner->HasUnitMovementFlag(MOVEMENTFLAG_FALLING))
        {
            SetMovementFlag(MOVEMENTFLAG_FALLING, true);
            TC_LOG_DEBUG("movement.bot.state", "MovementStateMachine: Set MOVEMENTFLAG_FALLING");
        }
    }
    else
    {
        if (owner->HasUnitMovementFlag(MOVEMENTFLAG_FALLING))
        {
            SetMovementFlag(MOVEMENTFLAG_FALLING, false);
            TC_LOG_DEBUG("movement.bot.state", "MovementStateMachine: Cleared MOVEMENTFLAG_FALLING");
        }
    }
}

void MovementStateMachine::ClearStateMovementFlags()
{
    RemoveAllStateFlags();
}

void MovementStateMachine::SetMovementFlag(uint32 flag, bool set)
{
    Unit* owner = GetOwner();
    if (!owner)
        return;

    if (set)
        owner->AddUnitMovementFlag(static_cast<MovementFlags>(flag));
    else
        owner->RemoveUnitMovementFlag(static_cast<MovementFlags>(flag));
}

void MovementStateMachine::RemoveAllStateFlags()
{
    Unit* owner = GetOwner();
    if (!owner)
        return;

    // Remove state-specific flags
    owner->RemoveUnitMovementFlag(MOVEMENTFLAG_SWIMMING);
    owner->RemoveUnitMovementFlag(MOVEMENTFLAG_FALLING);
}

MovementStateType MovementStateMachine::DetectAppropriateState() const
{
    // Priority order: Falling > Swimming > Ground > Idle

    if (IsFalling())
        return MovementStateType::Falling;

    if (IsInWater())
        return MovementStateType::Swimming;

    if (IsOnGround())
        return MovementStateType::Ground;

    return MovementStateType::Idle;
}

void MovementStateMachine::SyncWithEnvironment()
{
    MovementStateType appropriateState = DetectAppropriateState();

    if (appropriateState != _currentStateType)
    {
        TC_LOG_DEBUG("movement.bot.state", "MovementStateMachine: Environment sync - transitioning from {} to {}",
            static_cast<int>(_currentStateType), static_cast<int>(appropriateState));
        TransitionTo(appropriateState);
    }
}
