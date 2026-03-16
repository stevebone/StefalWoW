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

#ifndef TRINITY_GROUNDMOVEMENTSTATE_H
#define TRINITY_GROUNDMOVEMENTSTATE_H

#include "MovementState.h"
#include "Position.h"

class TC_GAME_API GroundMovementState : public MovementState
{
public:
    GroundMovementState() = default;
    ~GroundMovementState() override = default;

    void OnEnter(MovementStateMachine* sm, MovementState* prevState) override;
    void OnExit(MovementStateMachine* sm, MovementState* nextState) override;
    void Update(MovementStateMachine* sm, uint32 diff) override;

    MovementStateType GetType() const override { return MovementStateType::Ground; }
    uint32 GetRequiredMovementFlags() const override;
    char const* GetName() const override { return "Ground"; }

private:
    uint32 _edgeCheckTimer = 0;
    uint32 _waterCheckTimer = 0;
    Position _lastPosition;

    static constexpr uint32 EDGE_CHECK_INTERVAL = 100;   // Check every 100ms
    static constexpr uint32 WATER_CHECK_INTERVAL = 200;  // Check every 200ms
    static constexpr float EDGE_DETECTION_DISTANCE = 3.0f; // Look 3 yards ahead
    static constexpr float EDGE_HEIGHT_THRESHOLD = 5.0f;   // Consider it an edge if drop > 5 yards

    // Check if there's an edge (cliff) in front of the unit
    bool CheckForEdge(MovementStateMachine* sm);

    // Check if we're entering water
    bool CheckForWater(MovementStateMachine* sm);

    // Check if we're falling
    bool CheckForFalling(MovementStateMachine* sm);

    // Get position ahead of unit based on facing direction
    Position GetPositionAhead(Unit* unit, float distance) const;
};

#endif // TRINITY_GROUNDMOVEMENTSTATE_H
