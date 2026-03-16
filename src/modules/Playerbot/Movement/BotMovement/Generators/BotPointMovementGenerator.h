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

#ifndef TRINITY_BOTPOINTMOVEMENTGENERATOR_H
#define TRINITY_BOTPOINTMOVEMENTGENERATOR_H

#include "BotMovementGeneratorBase.h"
#include "MovementGenerator.h"
#include "Position.h"
#include "Optional.h"

class Unit;

class TC_GAME_API BotPointMovementGenerator : public MovementGenerator, protected BotMovementGeneratorBase
{
public:
    explicit BotPointMovementGenerator(uint32 id, float x, float y, float z,
        bool generatePath = true, Optional<float> speed = {},
        Optional<float> finalOrient = {});

    ~BotPointMovementGenerator() override;

    BotPointMovementGenerator(BotPointMovementGenerator const&) = delete;
    BotPointMovementGenerator& operator=(BotPointMovementGenerator const&) = delete;

    MovementGeneratorType GetMovementGeneratorType() const override;

    void Initialize(Unit* owner) override;
    void Reset(Unit* owner) override;
    bool Update(Unit* owner, uint32 diff) override;
    void Deactivate(Unit* owner) override;
    void Finalize(Unit* owner, bool active, bool movementInform) override;

    void UnitSpeedChanged() override { AddFlag(MOVEMENTGENERATOR_FLAG_SPEED_UPDATE_PENDING); }

    uint32 GetId() const { return _movementId; }
    Position const& GetDestination() const { return _destination; }

private:
    void MovementInform(Unit* owner);
    void StartMovement(Unit* owner);
    bool HasReachedDestination(Unit* owner) const;

    uint32 _movementId;
    Position _destination;
    Optional<float> _speed;
    Optional<float> _finalOrient;
    bool _generatePath;
    bool _recalculatePath = false;

    static constexpr float DESTINATION_REACHED_THRESHOLD = 0.5f;
};

#endif // TRINITY_BOTPOINTMOVEMENTGENERATOR_H
