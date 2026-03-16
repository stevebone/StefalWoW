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

#ifndef TRINITY_IDLESTATE_H
#define TRINITY_IDLESTATE_H

#include "MovementState.h"

class TC_GAME_API IdleState : public MovementState
{
public:
    IdleState() = default;
    ~IdleState() override = default;

    void OnEnter(MovementStateMachine* sm, MovementState* prevState) override;
    void OnExit(MovementStateMachine* sm, MovementState* nextState) override;
    void Update(MovementStateMachine* sm, uint32 diff) override;

    MovementStateType GetType() const override { return MovementStateType::Idle; }
    uint32 GetRequiredMovementFlags() const override { return 0; }
    char const* GetName() const override { return "Idle"; }

private:
    uint32 _environmentCheckTimer = 0;
    static constexpr uint32 ENVIRONMENT_CHECK_INTERVAL = 500; // Check every 500ms
};

#endif // TRINITY_IDLESTATE_H
