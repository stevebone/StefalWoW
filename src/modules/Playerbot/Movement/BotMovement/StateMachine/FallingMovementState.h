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

#ifndef TRINITY_FALLINGMOVEMENTSTATE_H
#define TRINITY_FALLINGMOVEMENTSTATE_H

#include "MovementState.h"
#include "UnitDefines.h"

class TC_GAME_API FallingMovementState : public MovementState
{
public:
    FallingMovementState() = default;
    ~FallingMovementState() override = default;

    void OnEnter(MovementStateMachine* sm, MovementState* prevState) override;
    void OnExit(MovementStateMachine* sm, MovementState* nextState) override;
    void Update(MovementStateMachine* sm, uint32 diff) override;

    MovementStateType GetType() const override { return MovementStateType::Falling; }
    uint32 GetRequiredMovementFlags() const override { return MOVEMENTFLAG_FALLING; }
    char const* GetName() const override { return "Falling"; }

private:
    float _fallStartHeight = 0.0f;
    uint32 _fallDuration = 0;
    uint32 _landingCheckTimer = 0;

    static constexpr uint32 LANDING_CHECK_INTERVAL = 50;     // Check every 50ms
    static constexpr float LANDING_HEIGHT_THRESHOLD = 1.5f;  // Within 1.5 yards of ground = landed
    static constexpr float GRAVITY = 19.29f;                 // WoW gravity constant (yards/s^2)
    static constexpr float SAFE_FALL_HEIGHT = 14.5f;         // No damage below this
    static constexpr float FATAL_FALL_HEIGHT = 65.0f;        // Fatal above this

    // Check if landed on ground
    bool CheckForLanding(MovementStateMachine* sm);

    // Check if landed in water
    bool CheckForWaterLanding(MovementStateMachine* sm);

    // Calculate expected fall damage
    float CalculateFallDamage(float fallHeight) const;

    // Apply falling physics
    void ApplyFallingPhysics(MovementStateMachine* sm, uint32 diff);
};

#endif // TRINITY_FALLINGMOVEMENTSTATE_H
