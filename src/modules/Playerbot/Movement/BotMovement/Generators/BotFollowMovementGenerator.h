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

#ifndef TRINITY_BOTFOLLOWMOVEMENTGENERATOR_H
#define TRINITY_BOTFOLLOWMOVEMENTGENERATOR_H

#include "BotMovementGeneratorBase.h"
#include "AbstractFollower.h"
#include "MovementGenerator.h"
#include "Position.h"
#include "Timer.h"
#include "Optional.h"

class Unit;

class TC_GAME_API BotFollowMovementGenerator : public MovementGenerator, public AbstractFollower, protected BotMovementGeneratorBase
{
public:
    explicit BotFollowMovementGenerator(Unit* target, float range, Optional<ChaseAngle> angle,
        Optional<Milliseconds> duration = {});

    ~BotFollowMovementGenerator() override;

    BotFollowMovementGenerator(BotFollowMovementGenerator const&) = delete;
    BotFollowMovementGenerator& operator=(BotFollowMovementGenerator const&) = delete;

    MovementGeneratorType GetMovementGeneratorType() const override { return FOLLOW_MOTION_TYPE; }

    void Initialize(Unit* owner) override;
    void Reset(Unit* owner) override;
    bool Update(Unit* owner, uint32 diff) override;
    void Deactivate(Unit* owner) override;
    void Finalize(Unit* owner, bool active, bool movementInform) override;

    void UnitSpeedChanged() override { _lastTargetPosition.reset(); }

private:
    static constexpr uint32 CHECK_INTERVAL = 100;            // Check every 100ms
    static constexpr float MIN_TARGET_MOVE_DISTANCE = 3.0f;  // Recalculate if target moved > 3 yards
    static constexpr float FOLLOW_RANGE_TOLERANCE = 1.0f;

    void UpdatePetSpeed(Unit* owner);
    bool IsWithinRange(Unit* owner) const;
    Position GetFollowPosition(Unit* owner) const;
    void StartFollowing(Unit* owner);

    float const _range;
    Optional<ChaseAngle const> _angle;

    TimeTracker _checkTimer;
    Optional<TimeTracker> _duration;
    Optional<Position> _lastTargetPosition;
};

#endif // TRINITY_BOTFOLLOWMOVEMENTGENERATOR_H
