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

#ifndef TRINITY_BOTMOVEMENTGENERATORBASE_H
#define TRINITY_BOTMOVEMENTGENERATORBASE_H

#include "Define.h"
#include "MovementGenerator.h"
#include "Position.h"
#include "ValidatedPathGenerator.h"
#include <memory>

class BotMovementController;
class MovementStateMachine;
class StuckDetector;
class Unit;

class TC_GAME_API BotMovementGeneratorBase
{
public:
    BotMovementGeneratorBase();
    virtual ~BotMovementGeneratorBase();

    BotMovementGeneratorBase(BotMovementGeneratorBase const&) = delete;
    BotMovementGeneratorBase& operator=(BotMovementGeneratorBase const&) = delete;

protected:
    // Validation helpers
    bool ValidateDestination(Unit* owner, Position const& dest);
    bool ValidateCurrentPath(Unit* owner);

    // Path generation with validation
    ValidatedPath GenerateValidatedPath(Unit* owner, Position const& dest);

    // Movement flag synchronization
    void SyncMovementFlags(Unit* owner);

    // State machine integration
    void UpdateStateMachine(Unit* owner, uint32 diff);
    MovementStateMachine* GetStateMachine(Unit* owner) const;

    // Stuck detection integration
    void UpdateStuckDetection(Unit* owner, uint32 diff);
    StuckDetector* GetStuckDetector(Unit* owner) const;
    void RecordProgress(Unit* owner, uint32 waypointIndex);

    // Controller access
    BotMovementController* GetController(Unit* owner) const;

    // Current path info
    ValidatedPath _currentPath;
    uint32 _currentWaypoint = 0;
    uint32 _pathUpdateTimer = 0;

    static constexpr uint32 PATH_UPDATE_INTERVAL = 1000;     // Recalculate path every 1s
    static constexpr float WAYPOINT_REACHED_THRESHOLD = 1.5f; // Within 1.5 yards = reached
};

#endif // TRINITY_BOTMOVEMENTGENERATORBASE_H
