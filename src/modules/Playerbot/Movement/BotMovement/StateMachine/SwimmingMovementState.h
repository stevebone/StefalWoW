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

#ifndef TRINITY_SWIMMINGMOVEMENTSTATE_H
#define TRINITY_SWIMMINGMOVEMENTSTATE_H

#include "MovementState.h"
#include "UnitDefines.h"

class TC_GAME_API SwimmingMovementState : public MovementState
{
public:
    SwimmingMovementState() = default;
    ~SwimmingMovementState() override = default;

    void OnEnter(MovementStateMachine* sm, MovementState* prevState) override;
    void OnExit(MovementStateMachine* sm, MovementState* nextState) override;
    void Update(MovementStateMachine* sm, uint32 diff) override;

    MovementStateType GetType() const override { return MovementStateType::Swimming; }
    uint32 GetRequiredMovementFlags() const override { return MOVEMENTFLAG_SWIMMING; }
    char const* GetName() const override { return "Swimming"; }

private:
    uint32 _underwaterTimer = 0;
    uint32 _waterCheckTimer = 0;
    uint32 _surfaceCheckTimer = 0;
    bool _needsAir = true;
    bool _isUnderwater = false;

    static constexpr uint32 WATER_CHECK_INTERVAL = 200;     // Check every 200ms
    static constexpr uint32 SURFACE_CHECK_INTERVAL = 1000;  // Check every 1s
    static constexpr uint32 BREATH_WARNING_TIME = 45000;    // Warn at 45s underwater
    static constexpr uint32 MAX_BREATH_TIME = 60000;        // Max 60s underwater

    // Check if still in water
    bool CheckStillInWater(MovementStateMachine* sm);

    // Check if need to surface for air
    bool CheckNeedToSurface(MovementStateMachine* sm);

    // Surface for air
    void SurfaceForAir(MovementStateMachine* sm);

    // Update underwater tracking
    void UpdateUnderwaterStatus(MovementStateMachine* sm, uint32 diff);
};

#endif // TRINITY_SWIMMINGMOVEMENTSTATE_H
