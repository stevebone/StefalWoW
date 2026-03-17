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

#ifndef TRINITY_MOVEMENTSTATE_H
#define TRINITY_MOVEMENTSTATE_H

#include "Define.h"
#include "BotMovementDefines.h"

class MovementStateMachine;
class Unit;

class TC_GAME_API MovementState
{
public:
    MovementState() = default;
    virtual ~MovementState() = default;

    MovementState(MovementState const&) = delete;
    MovementState& operator=(MovementState const&) = delete;

    // Called when entering this state
    virtual void OnEnter(MovementStateMachine* sm, MovementState* prevState) = 0;

    // Called when exiting this state
    virtual void OnExit(MovementStateMachine* sm, MovementState* nextState) = 0;

    // Called every update tick
    virtual void Update(MovementStateMachine* sm, uint32 diff) = 0;

    // Get the state type
    virtual MovementStateType GetType() const = 0;

    // Get movement flags required for this state
    virtual uint32 GetRequiredMovementFlags() const = 0;

    // Get the name of this state for debugging
    virtual char const* GetName() const = 0;

protected:
    // Helper to request state transition
    void TransitionTo(MovementStateMachine* sm, MovementStateType newState);

    // Helper to get owner unit
    Unit* GetOwner(MovementStateMachine* sm) const;
};

#endif // TRINITY_MOVEMENTSTATE_H
