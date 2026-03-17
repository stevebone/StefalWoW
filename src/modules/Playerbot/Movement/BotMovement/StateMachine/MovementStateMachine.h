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

#ifndef TRINITY_MOVEMENTSTATEMACHINE_H
#define TRINITY_MOVEMENTSTATEMACHINE_H

#include "Define.h"
#include "BotMovementDefines.h"
#include <memory>
#include <array>

class Unit;
class MovementState;
class BotMovementController;

class TC_GAME_API MovementStateMachine
{
public:
    explicit MovementStateMachine(BotMovementController* controller);
    ~MovementStateMachine();

    MovementStateMachine(MovementStateMachine const&) = delete;
    MovementStateMachine& operator=(MovementStateMachine const&) = delete;

    // Update the state machine
    void Update(uint32 diff);

    // Request transition to a new state
    void TransitionTo(MovementStateType newState);

    // Get current state
    MovementStateType GetCurrentStateType() const { return _currentStateType; }
    MovementState* GetCurrentState() const { return _currentState; }

    // Get owner unit
    Unit* GetOwner() const;

    // Get controller
    BotMovementController* GetController() const { return _controller; }

    // Environment queries
    bool IsOnGround() const;
    bool IsInWater() const;
    bool IsFalling() const;
    bool IsFlying() const;

    // Get time in current state
    uint32 GetTimeInCurrentState() const { return _timeInCurrentState; }

    // State flag helpers
    void ApplyStateMovementFlags();
    void ClearStateMovementFlags();

    // Detect appropriate state based on current environment
    MovementStateType DetectAppropriateState() const;

    // Force state synchronization with environment
    void SyncWithEnvironment();

private:
    BotMovementController* _controller;
    MovementState* _currentState;
    MovementStateType _currentStateType;
    MovementStateType _pendingStateType;
    bool _hasPendingTransition;
    uint32 _timeInCurrentState;

    // State instances (reused, not recreated on each transition)
    std::array<std::unique_ptr<MovementState>, 6> _states;

    void InitializeStates();
    MovementState* GetStateInstance(MovementStateType type);
    void ProcessPendingTransition();

    // Movement flag manipulation
    void SetMovementFlag(uint32 flag, bool set);
    void RemoveAllStateFlags();
};

#endif // TRINITY_MOVEMENTSTATEMACHINE_H
