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

#ifndef TRINITY_STUCKSTATE_H
#define TRINITY_STUCKSTATE_H

#include "MovementState.h"

class TC_GAME_API StuckState : public MovementState
{
public:
    StuckState() = default;
    ~StuckState() override = default;

    void OnEnter(MovementStateMachine* sm, MovementState* prevState) override;
    void OnExit(MovementStateMachine* sm, MovementState* nextState) override;
    void Update(MovementStateMachine* sm, uint32 diff) override;

    MovementStateType GetType() const override { return MovementStateType::Stuck; }
    uint32 GetRequiredMovementFlags() const override { return 0; }
    char const* GetName() const override { return "Stuck"; }

    // Get current recovery attempt count
    uint32 GetRecoveryAttempts() const { return _recoveryAttempts; }

    // Get time spent in stuck state
    uint32 GetStuckDuration() const { return _stuckDuration; }

private:
    uint32 _recoveryAttempts = 0;
    uint32 _stuckDuration = 0;
    uint32 _recoveryTimer = 0;
    MovementStateType _previousStateType = MovementStateType::Idle;

    static constexpr uint32 RECOVERY_ATTEMPT_INTERVAL = 2000;  // Try recovery every 2s
    static constexpr uint32 MAX_RECOVERY_ATTEMPTS = 5;
    static constexpr uint32 MAX_STUCK_DURATION = 30000;        // Max 30s in stuck state

    // Attempt recovery
    bool TryRecovery(MovementStateMachine* sm);

    // Transition back to appropriate state
    void TransitionToAppropriateState(MovementStateMachine* sm);
};

#endif // TRINITY_STUCKSTATE_H
