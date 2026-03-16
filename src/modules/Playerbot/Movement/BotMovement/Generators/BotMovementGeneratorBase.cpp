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

#include "BotMovementGeneratorBase.h"
#include "BotMovementManager.h"
#include "BotMovementController.h"
#include "MovementStateMachine.h"
#include "StuckDetector.h"
#include "CollisionValidator.h"
#include "GroundValidator.h"
#include "LiquidValidator.h"
#include "PositionValidator.h"
#include "Unit.h"
#include "Log.h"

BotMovementGeneratorBase::BotMovementGeneratorBase()
{
}

BotMovementGeneratorBase::~BotMovementGeneratorBase() = default;

BotMovementController* BotMovementGeneratorBase::GetController(Unit* owner) const
{
    if (!owner)
        return nullptr;

    return sBotMovementManager->GetControllerForUnit(owner);
}

MovementStateMachine* BotMovementGeneratorBase::GetStateMachine(Unit* owner) const
{
    BotMovementController* controller = GetController(owner);
    // Note: Controller would need to expose state machine - for now return null
    return nullptr;
}

StuckDetector* BotMovementGeneratorBase::GetStuckDetector(Unit* owner) const
{
    BotMovementController* controller = GetController(owner);
    // Note: Controller would need to expose stuck detector - for now return null
    return nullptr;
}

bool BotMovementGeneratorBase::ValidateDestination(Unit* owner, Position const& dest)
{
    if (!owner || !owner->IsInWorld())
        return false;

    // Validate position bounds
    ValidationResult posResult = PositionValidator::ValidatePosition(owner->GetMapId(), dest);
    if (!posResult.isValid)
    {
        TC_LOG_DEBUG("movement.bot.generator", "BotMovementGeneratorBase: Destination validation failed: {}",
            posResult.errorMessage);
        return false;
    }

    // Check for dangerous liquid
    LiquidInfo liquidInfo = LiquidValidator::GetLiquidInfoAt(owner->GetMap(), dest);
    if (liquidInfo.isDangerous)
    {
        TC_LOG_DEBUG("movement.bot.generator", "BotMovementGeneratorBase: Destination is in dangerous liquid");
        return false;
    }

    // Check line of sight from current position
    if (CollisionValidator::WouldCollide(owner, dest))
    {
        TC_LOG_DEBUG("movement.bot.generator", "BotMovementGeneratorBase: No line of sight to destination");
        // This is OK - PathGenerator should handle it
    }

    return true;
}

bool BotMovementGeneratorBase::ValidateCurrentPath(Unit* owner)
{
    if (!owner || !owner->IsInWorld())
        return false;

    if (!_currentPath.IsValid())
        return false;

    // Validate remaining path segments
    if (_currentWaypoint < _currentPath.points.size())
    {
        // Just check next segment
        Position current = owner->GetPosition();
        G3D::Vector3 const& next = _currentPath.points[_currentWaypoint];
        Position nextPos(next.x, next.y, next.z, 0.0f);

        return CollisionValidator::HasLineOfSight(owner, current, nextPos);
    }

    return true;
}

ValidatedPath BotMovementGeneratorBase::GenerateValidatedPath(Unit* owner, Position const& dest)
{
    ValidatedPath result;

    if (!owner || !owner->IsInWorld())
    {
        result.validationResult = ValidationResult::Failure(
            ValidationFailureReason::InvalidPosition,
            "Owner is null or not in world");
        return result;
    }

    ValidatedPathGenerator pathGen(owner);
    result = pathGen.CalculateValidatedPath(dest);

    if (result.IsValid())
    {
        TC_LOG_DEBUG("movement.bot.generator", "BotMovementGeneratorBase: Generated path with {} points",
            result.points.size());
    }
    else
    {
        TC_LOG_DEBUG("movement.bot.generator", "BotMovementGeneratorBase: Path generation failed: {}",
            result.validationResult.errorMessage);
    }

    return result;
}

void BotMovementGeneratorBase::SyncMovementFlags(Unit* owner)
{
    if (!owner || !owner->IsInWorld())
        return;

    // Check if in water and set swimming flag
    if (LiquidValidator::IsSwimmingRequired(owner))
    {
        if (!owner->HasUnitMovementFlag(MOVEMENTFLAG_SWIMMING))
        {
            owner->AddUnitMovementFlag(MOVEMENTFLAG_SWIMMING);
            TC_LOG_DEBUG("movement.bot.generator", "BotMovementGeneratorBase: Set MOVEMENTFLAG_SWIMMING");
        }
    }
    else
    {
        if (owner->HasUnitMovementFlag(MOVEMENTFLAG_SWIMMING))
        {
            owner->RemoveUnitMovementFlag(MOVEMENTFLAG_SWIMMING);
            TC_LOG_DEBUG("movement.bot.generator", "BotMovementGeneratorBase: Cleared MOVEMENTFLAG_SWIMMING");
        }
    }

    // Check if falling
    float groundHeight = GroundValidator::GetGroundHeight(owner);
    if (groundHeight != INVALID_HEIGHT)
    {
        float heightAboveGround = owner->GetPositionZ() - groundHeight;
        bool shouldBeFalling = heightAboveGround > 3.0f && !LiquidValidator::IsSwimmingRequired(owner);

        if (shouldBeFalling && !owner->HasUnitMovementFlag(MOVEMENTFLAG_FALLING))
        {
            owner->AddUnitMovementFlag(MOVEMENTFLAG_FALLING);
            TC_LOG_DEBUG("movement.bot.generator", "BotMovementGeneratorBase: Set MOVEMENTFLAG_FALLING");
        }
        else if (!shouldBeFalling && owner->HasUnitMovementFlag(MOVEMENTFLAG_FALLING))
        {
            owner->RemoveUnitMovementFlag(MOVEMENTFLAG_FALLING);
            TC_LOG_DEBUG("movement.bot.generator", "BotMovementGeneratorBase: Cleared MOVEMENTFLAG_FALLING");
        }
    }
}

void BotMovementGeneratorBase::UpdateStateMachine(Unit* owner, uint32 diff)
{
    // State machine update would be handled by controller
    // This is a hook point for generators that want to influence state
}

void BotMovementGeneratorBase::UpdateStuckDetection(Unit* owner, uint32 diff)
{
    // Stuck detection update would be handled by controller
    // This is a hook point for generators that want to contribute to detection
}

void BotMovementGeneratorBase::RecordProgress(Unit* owner, uint32 waypointIndex)
{
    StuckDetector* detector = GetStuckDetector(owner);
    if (detector)
    {
        detector->RecordProgress(waypointIndex);
    }
}
