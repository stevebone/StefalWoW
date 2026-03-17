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

#include "BotMovementController.h"
#include "MovementStateMachine.h"
#include "StuckDetector.h"
#include "RecoveryStrategies.h"
#include "ValidatedPathGenerator.h"
#include "LiquidValidator.h"
#include "GroundValidator.h"
#include "Unit.h"
#include "Position.h"
#include "MotionMaster.h"
#include "Log.h"

BotMovementController::BotMovementController(Unit* owner)
    : _owner(owner)
    , _totalTimePassed(0)
    , _positionRecordTimer(0)
    , _stateSyncTimer(0)
{
    if (_owner)
    {
        // Initialize state machine
        _stateMachine = std::make_unique<MovementStateMachine>(this);

        // Initialize stuck detector
        _stuckDetector = std::make_unique<StuckDetector>(this);

        // Initialize path generator
        _pathGenerator = std::make_unique<ValidatedPathGenerator>(_owner);

        // Record initial position
        RecordPosition();

        TC_LOG_DEBUG("movement.bot", "BotMovementController: Created for {}", _owner->GetName());
    }
}

BotMovementController::~BotMovementController()
{
    TC_LOG_DEBUG("movement.bot", "BotMovementController: Destroyed for {}",
        _owner ? _owner->GetName() : "null");
}

void BotMovementController::Update(uint32 diff)
{
    if (!_owner || !_owner->IsInWorld())
        return;

    _totalTimePassed += diff;

    // Update position history
    UpdatePositionHistory(diff);

    // Update state machine
    UpdateStateMachine(diff);

    // Update stuck detection
    UpdateStuckDetection(diff);

    // Handle stuck state if detected
    if (IsStuck())
    {
        HandleStuckState();
    }

    // Periodic state synchronization
    _stateSyncTimer += diff;
    if (_stateSyncTimer >= STATE_SYNC_INTERVAL)
    {
        _stateSyncTimer = 0;
        SyncMovementFlags();
    }
}

void BotMovementController::UpdateStateMachine(uint32 diff)
{
    if (_stateMachine)
    {
        // Check for automatic state transitions based on environment
        UpdateStateTransitions();

        // Update current state
        _stateMachine->Update(diff);
    }
}

void BotMovementController::UpdateStateTransitions()
{
    if (!_stateMachine)
        return;

    MovementStateType currentState = _stateMachine->GetCurrentStateType();
    MovementStateType appropriateState = DetermineAppropriateState();

    if (appropriateState != currentState)
    {
        TC_LOG_DEBUG("movement.bot.state",
            "BotMovementController: Auto-transition for {} from {} to {}",
            _owner ? _owner->GetName() : "null",
            static_cast<int>(currentState),
            static_cast<int>(appropriateState));

        _stateMachine->TransitionTo(appropriateState);
    }
}

MovementStateType BotMovementController::DetermineAppropriateState() const
{
    if (!_owner || !_owner->IsInWorld())
        return MovementStateType::Idle;

    // Priority order: Stuck > Swimming > Falling > Ground > Idle

    // Check if stuck (highest priority)
    if (IsStuck())
        return MovementStateType::Stuck;

    // Check if in water (requires swimming)
    if (LiquidValidator::IsSwimmingRequired(_owner))
        return MovementStateType::Swimming;

    // Check if falling (not on ground and not in flight)
    if (_stateMachine && !_stateMachine->IsOnGround() &&
        !_owner->HasUnitState(UNIT_STATE_IN_FLIGHT))
        return MovementStateType::Falling;

    // Check if moving on ground
    if (_owner->isMoving())
        return MovementStateType::Ground;

    // Default to idle
    return MovementStateType::Idle;
}

void BotMovementController::UpdateStuckDetection(uint32 diff)
{
    if (_stuckDetector)
    {
        _stuckDetector->Update(diff);
    }
}

void BotMovementController::UpdatePositionHistory(uint32 diff)
{
    _positionRecordTimer += diff;

    if (_positionRecordTimer >= POSITION_RECORD_INTERVAL)
    {
        _positionRecordTimer = 0;
        RecordPosition();
    }
}

void BotMovementController::SyncMovementFlags()
{
    if (!_owner || !_owner->IsInWorld())
        return;

    // Let state machine handle flag synchronization
    if (_stateMachine)
    {
        _stateMachine->ApplyStateMovementFlags();
    }
}

bool BotMovementController::MoveToPosition(Position const& dest, bool forceDest)
{
    if (!_owner || !_owner->IsInWorld())
        return false;

    if (!_pathGenerator)
        return false;

    // Generate validated path
    ValidatedPath path = _pathGenerator->CalculateValidatedPath(dest, forceDest);

    if (!path.IsValid())
    {
        TC_LOG_DEBUG("movement.bot", "BotMovementController: Path validation failed for {}: {}",
            _owner->GetName(), path.validationResult.errorMessage);

        if (_stuckDetector)
            _stuckDetector->RecordPathFailure();

        return false;
    }

    // Clear current movement
    _owner->GetMotionMaster()->Clear();

    // Start new movement
    _owner->GetMotionMaster()->MovePoint(0,
        dest.GetPositionX(),
        dest.GetPositionY(),
        dest.GetPositionZ());

    // Transition to appropriate state based on path
    if (_stateMachine)
    {
        if (path.requiresSwimming)
        {
            _stateMachine->TransitionTo(MovementStateType::Swimming);
        }
        else
        {
            _stateMachine->TransitionTo(MovementStateType::Ground);
        }
    }

    return true;
}

bool BotMovementController::MoveFollow(Unit* target, float distance, float angle)
{
    if (!_owner || !_owner->IsInWorld() || !target)
        return false;

    // Clear current movement
    _owner->GetMotionMaster()->Clear();

    // Start follow movement
    _owner->GetMotionMaster()->MoveFollow(target, distance, angle);

    // Transition to appropriate state
    if (_stateMachine)
    {
        if (LiquidValidator::IsSwimmingRequired(_owner))
        {
            _stateMachine->TransitionTo(MovementStateType::Swimming);
        }
        else
        {
            _stateMachine->TransitionTo(MovementStateType::Ground);
        }
    }

    return true;
}

MovementStateType BotMovementController::GetCurrentState() const
{
    if (_stateMachine)
        return _stateMachine->GetCurrentStateType();

    return MovementStateType::Idle;
}

bool BotMovementController::IsStuck() const
{
    if (_stuckDetector)
        return _stuckDetector->IsStuck();

    return false;
}

bool BotMovementController::IsMoving() const
{
    if (!_owner)
        return false;

    return _owner->isMoving();
}

bool BotMovementController::IsInWater() const
{
    if (_stateMachine)
        return _stateMachine->IsInWater();

    if (!_owner)
        return false;

    return LiquidValidator::IsSwimmingRequired(_owner);
}

bool BotMovementController::IsFalling() const
{
    if (_stateMachine)
        return _stateMachine->IsFalling();

    return false;
}

Position const* BotMovementController::GetLastPosition() const
{
    if (_positionHistory.empty())
        return nullptr;

    return &_positionHistory.back().pos;
}

void BotMovementController::RecordPosition()
{
    if (!_owner)
        return;

    Position currentPos = _owner->GetPosition();
    _positionHistory.emplace_back(currentPos, _totalTimePassed);

    if (_positionHistory.size() > MAX_POSITION_HISTORY)
        _positionHistory.pop_front();

    // Also record to stuck detector
    if (_stuckDetector)
    {
        _stuckDetector->RecordPosition(currentPos);
    }
}

void BotMovementController::TriggerStuckRecovery()
{
    if (!_stuckDetector || !_stuckDetector->IsStuck())
        return;

    HandleStuckState();
}

void BotMovementController::ClearStuckState()
{
    if (_stuckDetector)
    {
        _stuckDetector->Reset();
    }

    // Transition out of stuck state
    if (_stateMachine && _stateMachine->GetCurrentStateType() == MovementStateType::Stuck)
    {
        _stateMachine->SyncWithEnvironment();
    }
}

void BotMovementController::SyncWithEnvironment()
{
    if (_stateMachine)
    {
        _stateMachine->SyncWithEnvironment();
    }
}

void BotMovementController::HandleStuckState()
{
    if (!_stuckDetector || !_owner)
        return;

    StuckInfo const& stuckInfo = _stuckDetector->GetStuckInfo();

    TC_LOG_DEBUG("movement.bot", "BotMovementController: Handling stuck state for {} (type: {}, duration: {}ms, attempts: {})",
        _owner->GetName(),
        static_cast<int>(stuckInfo.type),
        stuckInfo.stuckDuration,
        stuckInfo.recoveryAttempts);

    // Attempt recovery
    RecoveryResult result = RecoveryStrategies::TryRecover(
        this,
        stuckInfo.type,
        stuckInfo.recoveryAttempts);

    if (result.success)
    {
        TC_LOG_DEBUG("movement.bot", "BotMovementController: Recovery succeeded for {} (level {}): {}",
            _owner->GetName(),
            static_cast<int>(result.levelUsed),
            result.message);

        _stuckDetector->Reset();

        // Transition out of stuck state
        if (_stateMachine)
        {
            _stateMachine->SyncWithEnvironment();
        }
    }
    else
    {
        TC_LOG_DEBUG("movement.bot", "BotMovementController: Recovery failed for {} (level {}): {}",
            _owner->GetName(),
            static_cast<int>(result.levelUsed),
            result.message);

        // Increment recovery attempts
        _stuckDetector->IncrementRecoveryAttempts();

        // Transition to stuck state if not already
        if (_stateMachine && _stateMachine->GetCurrentStateType() != MovementStateType::Stuck)
        {
            _stateMachine->TransitionTo(MovementStateType::Stuck);
        }
    }
}
