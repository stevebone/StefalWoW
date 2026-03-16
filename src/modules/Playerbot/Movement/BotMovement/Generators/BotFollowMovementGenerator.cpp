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

#include "BotFollowMovementGenerator.h"
#include "MoveSpline.h"
#include "MoveSplineInit.h"
#include "Unit.h"
#include "Creature.h"
#include "Player.h"
#include "Log.h"
#include <cmath>

BotFollowMovementGenerator::BotFollowMovementGenerator(Unit* target, float range, Optional<ChaseAngle> angle,
    Optional<Milliseconds> duration)
    : AbstractFollower(target)
    , _range(range)
    , _angle(angle)
    , _checkTimer(CHECK_INTERVAL)
{
    if (duration)
        _duration.emplace(*duration);
}

BotFollowMovementGenerator::~BotFollowMovementGenerator() = default;

void BotFollowMovementGenerator::Initialize(Unit* owner)
{
    if (!owner)
        return;

    RemoveFlag(MOVEMENTGENERATOR_FLAG_INITIALIZATION_PENDING);
    AddFlag(MOVEMENTGENERATOR_FLAG_INITIALIZED);

    owner->AddUnitState(UNIT_STATE_FOLLOW);

    UpdatePetSpeed(owner);

    Unit* target = GetTarget();
    if (target)
    {
        TC_LOG_DEBUG("movement.bot.generator", "BotFollowMovementGenerator: {} following {} at range {}",
            owner->GetName(), target->GetName(), _range);
    }

    _lastTargetPosition.reset();
    StartFollowing(owner);
}

void BotFollowMovementGenerator::Reset(Unit* owner)
{
    if (!owner)
        return;

    RemoveFlag(MOVEMENTGENERATOR_FLAG_DEACTIVATED);

    owner->AddUnitState(UNIT_STATE_FOLLOW);

    UpdatePetSpeed(owner);

    _lastTargetPosition.reset();
    StartFollowing(owner);
}

bool BotFollowMovementGenerator::Update(Unit* owner, uint32 diff)
{
    if (!owner || !owner->IsInWorld())
        return false;

    // Check duration
    if (_duration)
    {
        _duration->Update(diff);
        if (_duration->Passed())
        {
            TC_LOG_DEBUG("movement.bot.generator", "BotFollowMovementGenerator: Duration expired for {}",
                owner->GetName());
            return false;
        }
    }

    // Check target validity
    Unit* target = GetTarget();
    if (!target || !target->IsInWorld())
    {
        TC_LOG_DEBUG("movement.bot.generator", "BotFollowMovementGenerator: Target lost for {}",
            owner->GetName());
        return false;
    }

    // Update check timer
    _checkTimer.Update(diff);
    if (!_checkTimer.Passed())
    {
        // Sync movement flags every update
        SyncMovementFlags(owner);
        return true;
    }
    _checkTimer.Reset(CHECK_INTERVAL);

    // Check if target has moved significantly
    bool needsRecalculation = false;

    if (_lastTargetPosition)
    {
        float dx = target->GetPositionX() - _lastTargetPosition->GetPositionX();
        float dy = target->GetPositionY() - _lastTargetPosition->GetPositionY();
        float dz = target->GetPositionZ() - _lastTargetPosition->GetPositionZ();
        float distMoved = std::sqrt(dx * dx + dy * dy + dz * dz);

        if (distMoved >= MIN_TARGET_MOVE_DISTANCE)
        {
            needsRecalculation = true;
        }
    }
    else
    {
        needsRecalculation = true;
    }

    // Check if already in range
    if (IsWithinRange(owner))
    {
        // Stop if already in range and target hasn't moved
        if (!needsRecalculation && owner->movespline->Finalized())
        {
            // Face target
            owner->SetFacingToObject(target);
            return true;
        }
    }
    else
    {
        // Not in range, need to move
        needsRecalculation = true;
    }

    // Speed update needed
    if (HasFlag(MOVEMENTGENERATOR_FLAG_SPEED_UPDATE_PENDING))
    {
        RemoveFlag(MOVEMENTGENERATOR_FLAG_SPEED_UPDATE_PENDING);
        UpdatePetSpeed(owner);
        needsRecalculation = true;
    }

    // Recalculate path if needed
    if (needsRecalculation || owner->movespline->Finalized())
    {
        _lastTargetPosition = target->GetPosition();
        StartFollowing(owner);
    }

    // Sync movement flags
    SyncMovementFlags(owner);

    return true;
}

void BotFollowMovementGenerator::Deactivate(Unit* owner)
{
    AddFlag(MOVEMENTGENERATOR_FLAG_DEACTIVATED);

    if (!owner)
        return;

    owner->ClearUnitState(UNIT_STATE_FOLLOW_MOVE);

    TC_LOG_DEBUG("movement.bot.generator", "BotFollowMovementGenerator: Deactivated for {}", owner->GetName());
}

void BotFollowMovementGenerator::Finalize(Unit* owner, bool active, bool /*movementInform*/)
{
    AddFlag(MOVEMENTGENERATOR_FLAG_FINALIZED);

    if (!owner)
        return;

    owner->ClearUnitState(UNIT_STATE_FOLLOW | UNIT_STATE_FOLLOW_MOVE);

    TC_LOG_DEBUG("movement.bot.generator", "BotFollowMovementGenerator: Finalized for {} (active: {})",
        owner->GetName(), active);
}

void BotFollowMovementGenerator::UpdatePetSpeed(Unit* owner)
{
    if (!owner)
        return;

    Unit* target = GetTarget();
    if (!target)
        return;

    // Match speed with target for smooth following
    if (owner->IsPet() && target->GetTypeId() == TYPEID_PLAYER)
    {
        if (Player* player = target->ToPlayer())
        {
            owner->UpdateSpeed(MOVE_RUN);
            owner->UpdateSpeed(MOVE_WALK);
            owner->UpdateSpeed(MOVE_SWIM);
        }
    }
}

bool BotFollowMovementGenerator::IsWithinRange(Unit* owner) const
{
    if (!owner)
        return false;

    Unit* target = GetTarget();
    if (!target)
        return true; // No target, consider in range

    float dist = owner->GetDistance(target);
    return dist <= (_range + FOLLOW_RANGE_TOLERANCE);
}

Position BotFollowMovementGenerator::GetFollowPosition(Unit* owner) const
{
    Unit* target = GetTarget();
    if (!target || !owner)
        return Position();

    float angle = target->GetOrientation();

    if (_angle)
    {
        // Use specified angle relative to target's orientation
        angle += _angle->RelativeAngle;
    }
    else
    {
        // Default: follow behind
        angle += M_PI; // 180 degrees behind
    }

    // Normalize angle
    while (angle > 2 * M_PI)
        angle -= 2 * M_PI;
    while (angle < 0)
        angle += 2 * M_PI;

    float x = target->GetPositionX() + _range * std::cos(angle);
    float y = target->GetPositionY() + _range * std::sin(angle);
    float z = target->GetPositionZ();

    // Get proper ground height
    if (Map* map = target->GetMap())
    {
        float groundZ = map->GetHeight(target->GetPhaseShift(), x, y, z, true);
        if (groundZ != INVALID_HEIGHT)
            z = groundZ;
    }

    return Position(x, y, z, target->GetOrientation());
}

void BotFollowMovementGenerator::StartFollowing(Unit* owner)
{
    if (!owner)
        return;

    Unit* target = GetTarget();
    if (!target)
        return;

    // Get follow position
    Position followPos = GetFollowPosition(owner);

    // Validate destination
    if (!ValidateDestination(owner, followPos))
    {
        // Fall back to moving directly toward target
        followPos = target->GetPosition();
    }

    owner->AddUnitState(UNIT_STATE_FOLLOW_MOVE);

    // Generate validated path
    _currentPath = GenerateValidatedPath(owner, followPos);
    _currentWaypoint = 0;

    Movement::MoveSplineInit init(owner);

    if (_currentPath.IsValid() && _currentPath.points.size() > 1)
    {
        // Use generated path
        init.MovebyPath(_currentPath.points);

        TC_LOG_DEBUG("movement.bot.generator", "BotFollowMovementGenerator: Starting follow with {} waypoints",
            _currentPath.points.size());
    }
    else
    {
        // Direct movement
        init.MoveTo(followPos.GetPositionX(), followPos.GetPositionY(), followPos.GetPositionZ(), true);

        TC_LOG_DEBUG("movement.bot.generator", "BotFollowMovementGenerator: Using direct follow movement");
    }

    init.SetWalk(target->IsWalking());
    init.Launch();
}
