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

#include "BotPointMovementGenerator.h"
#include "MoveSpline.h"
#include "MoveSplineInit.h"
#include "Unit.h"
#include "Log.h"
#include <cmath>

BotPointMovementGenerator::BotPointMovementGenerator(uint32 id, float x, float y, float z,
    bool generatePath, Optional<float> speed, Optional<float> finalOrient)
    : _movementId(id)
    , _destination(x, y, z, 0.0f)
    , _speed(speed)
    , _finalOrient(finalOrient)
    , _generatePath(generatePath)
{
}

BotPointMovementGenerator::~BotPointMovementGenerator() = default;

MovementGeneratorType BotPointMovementGenerator::GetMovementGeneratorType() const
{
    return POINT_MOTION_TYPE;
}

void BotPointMovementGenerator::Initialize(Unit* owner)
{
    if (!owner || !owner->IsInWorld())
        return;

    RemoveFlag(MOVEMENTGENERATOR_FLAG_INITIALIZATION_PENDING);
    AddFlag(MOVEMENTGENERATOR_FLAG_INITIALIZED);

    TC_LOG_DEBUG("movement.bot.generator", "BotPointMovementGenerator: Initializing for {} to ({}, {}, {})",
        owner->GetName(), _destination.GetPositionX(), _destination.GetPositionY(), _destination.GetPositionZ());

    // Validate destination before moving
    if (!ValidateDestination(owner, _destination))
    {
        TC_LOG_WARN("movement.bot.generator", "BotPointMovementGenerator: Invalid destination for {}",
            owner->GetName());
        // Continue anyway, path generation will handle it
    }

    StartMovement(owner);
}

void BotPointMovementGenerator::Reset(Unit* owner)
{
    if (!owner || !owner->IsInWorld())
        return;

    RemoveFlag(MOVEMENTGENERATOR_FLAG_DEACTIVATED);

    TC_LOG_DEBUG("movement.bot.generator", "BotPointMovementGenerator: Reset for {}", owner->GetName());

    StartMovement(owner);
}

bool BotPointMovementGenerator::Update(Unit* owner, uint32 diff)
{
    if (!owner || !owner->IsInWorld())
        return false;

    // Check if movement completed
    if (owner->movespline->Finalized())
    {
        if (HasReachedDestination(owner))
        {
            TC_LOG_DEBUG("movement.bot.generator", "BotPointMovementGenerator: {} reached destination",
                owner->GetName());
            return false; // Movement complete
        }
        else
        {
            // Movement finished but not at destination - might need to recalculate
            _recalculatePath = true;
        }
    }

    // Update path timer
    _pathUpdateTimer += diff;

    // Sync movement flags
    SyncMovementFlags(owner);

    // Handle speed change
    if (HasFlag(MOVEMENTGENERATOR_FLAG_SPEED_UPDATE_PENDING))
    {
        RemoveFlag(MOVEMENTGENERATOR_FLAG_SPEED_UPDATE_PENDING);
        _recalculatePath = true;
    }

    // Recalculate path if needed
    if (_recalculatePath || _pathUpdateTimer >= PATH_UPDATE_INTERVAL)
    {
        _pathUpdateTimer = 0;
        _recalculatePath = false;

        // Check if we need to continue
        if (!HasReachedDestination(owner))
        {
            // Validate current path
            if (!ValidateCurrentPath(owner))
            {
                TC_LOG_DEBUG("movement.bot.generator", "BotPointMovementGenerator: Path invalidated, recalculating");
                StartMovement(owner);
            }
        }
    }

    return true; // Continue movement
}

void BotPointMovementGenerator::Deactivate(Unit* owner)
{
    AddFlag(MOVEMENTGENERATOR_FLAG_DEACTIVATED);

    if (!owner)
        return;

    TC_LOG_DEBUG("movement.bot.generator", "BotPointMovementGenerator: Deactivated for {}", owner->GetName());
}

void BotPointMovementGenerator::Finalize(Unit* owner, bool active, bool movementInform)
{
    AddFlag(MOVEMENTGENERATOR_FLAG_FINALIZED);

    if (!owner)
        return;

    TC_LOG_DEBUG("movement.bot.generator", "BotPointMovementGenerator: Finalized for {} (active: {}, inform: {})",
        owner->GetName(), active, movementInform);

    if (active)
        owner->ClearUnitState(UNIT_STATE_ROAMING_MOVE);

    if (movementInform)
        MovementInform(owner);
}

void BotPointMovementGenerator::StartMovement(Unit* owner)
{
    if (!owner)
        return;

    owner->AddUnitState(UNIT_STATE_ROAMING_MOVE);

    // Generate validated path
    _currentPath = GenerateValidatedPath(owner, _destination);
    _currentWaypoint = 0;

    Movement::MoveSplineInit init(owner);

    if (_currentPath.IsValid() && _currentPath.points.size() > 1)
    {
        // Use generated path
        init.MovebyPath(_currentPath.points);

        TC_LOG_DEBUG("movement.bot.generator", "BotPointMovementGenerator: Starting movement with {} waypoints",
            _currentPath.points.size());
    }
    else
    {
        // Direct movement if path generation failed
        init.MoveTo(_destination.GetPositionX(), _destination.GetPositionY(), _destination.GetPositionZ(), _generatePath);

        TC_LOG_DEBUG("movement.bot.generator", "BotPointMovementGenerator: Using direct movement (path gen failed)");
    }

    if (_speed)
        init.SetVelocity(*_speed);

    if (_finalOrient)
        init.SetFacing(*_finalOrient);

    init.Launch();
}

bool BotPointMovementGenerator::HasReachedDestination(Unit* owner) const
{
    if (!owner)
        return false;

    float dx = owner->GetPositionX() - _destination.GetPositionX();
    float dy = owner->GetPositionY() - _destination.GetPositionY();
    float dz = owner->GetPositionZ() - _destination.GetPositionZ();
    float distSq = dx * dx + dy * dy + dz * dz;

    return distSq <= (DESTINATION_REACHED_THRESHOLD * DESTINATION_REACHED_THRESHOLD);
}

void BotPointMovementGenerator::MovementInform(Unit* owner)
{
    if (!owner)
        return;

    if (owner->GetTypeId() == TYPEID_UNIT)
    {
        if (Creature* creature = owner->ToCreature())
        {
            if (creature->AI())
                creature->AI()->MovementInform(POINT_MOTION_TYPE, _movementId);
        }
    }
}
