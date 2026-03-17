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

#include "RecoveryStrategies.h"
#include "BotMovementController.h"
#include "CollisionValidator.h"
#include "GroundValidator.h"
#include "LiquidValidator.h"
#include "PositionValidator.h"
#include "Unit.h"
#include "Map.h"
#include "MotionMaster.h"
#include "Log.h"
#include <random>
#include <cmath>

Unit* RecoveryStrategies::GetOwner(BotMovementController* controller)
{
    return controller ? controller->GetOwner() : nullptr;
}

RecoveryResult RecoveryStrategies::TryRecover(BotMovementController* controller, StuckType stuckType, uint32 attemptCount)
{
    Unit* owner = GetOwner(controller);
    if (!owner || !owner->IsInWorld())
    {
        return RecoveryResult::Failure(RecoveryLevel::Level1_RecalculatePath, "Owner is null or not in world");
    }

    TC_LOG_DEBUG("movement.bot.recovery", "RecoveryStrategies: Attempting recovery for {} (attempt {}, stuck type {})",
        owner->GetName(), attemptCount, static_cast<int>(stuckType));

    // Escalate based on attempt count
    switch (attemptCount)
    {
        case 0:
        case 1:
            return Level1_RecalculatePath(controller);
        case 2:
            return Level2_BackupAndRetry(controller);
        case 3:
            return Level3_RandomNearbyPosition(controller);
        case 4:
            return Level4_TeleportToSafePosition(controller);
        default:
            return Level5_EvadeAndReset(controller);
    }
}

RecoveryResult RecoveryStrategies::Level1_RecalculatePath(BotMovementController* controller)
{
    Unit* owner = GetOwner(controller);
    if (!owner || !owner->IsInWorld())
    {
        return RecoveryResult::Failure(RecoveryLevel::Level1_RecalculatePath, "Owner invalid");
    }

    TC_LOG_DEBUG("movement.bot.recovery", "Level1: Recalculating path for {}", owner->GetName());

    // Clear current movement and let the AI recalculate
    owner->GetMotionMaster()->Clear();

    // The bot's AI should pick up and create a new path
    return RecoveryResult::Success(RecoveryLevel::Level1_RecalculatePath, "Path recalculation triggered");
}

RecoveryResult RecoveryStrategies::Level2_BackupAndRetry(BotMovementController* controller)
{
    Unit* owner = GetOwner(controller);
    if (!owner || !owner->IsInWorld())
    {
        return RecoveryResult::Failure(RecoveryLevel::Level2_BackupAndRetry, "Owner invalid");
    }

    TC_LOG_DEBUG("movement.bot.recovery", "Level2: Backing up and retrying for {}", owner->GetName());

    Position backupPos = GetBackupPosition(owner);

    if (!IsPositionSafe(owner, backupPos))
    {
        return RecoveryResult::Failure(RecoveryLevel::Level2_BackupAndRetry, "Backup position not safe");
    }

    // Move to backup position
    owner->GetMotionMaster()->Clear();
    owner->GetMotionMaster()->MovePoint(0,
        backupPos.GetPositionX(),
        backupPos.GetPositionY(),
        backupPos.GetPositionZ());

    RecoveryResult result = RecoveryResult::Success(RecoveryLevel::Level2_BackupAndRetry, "Moving to backup position");
    result.newPosition = backupPos;
    return result;
}

RecoveryResult RecoveryStrategies::Level3_RandomNearbyPosition(BotMovementController* controller)
{
    Unit* owner = GetOwner(controller);
    if (!owner || !owner->IsInWorld())
    {
        return RecoveryResult::Failure(RecoveryLevel::Level3_RandomNearbyPosition, "Owner invalid");
    }

    TC_LOG_DEBUG("movement.bot.recovery", "Level3: Trying random nearby position for {}", owner->GetName());

    Position randomPos = GetRandomNearbyPosition(owner);

    if (!IsPositionSafe(owner, randomPos))
    {
        return RecoveryResult::Failure(RecoveryLevel::Level3_RandomNearbyPosition, "Could not find safe random position");
    }

    // Move to random position
    owner->GetMotionMaster()->Clear();
    owner->GetMotionMaster()->MovePoint(0,
        randomPos.GetPositionX(),
        randomPos.GetPositionY(),
        randomPos.GetPositionZ());

    RecoveryResult result = RecoveryResult::Success(RecoveryLevel::Level3_RandomNearbyPosition, "Moving to random position");
    result.newPosition = randomPos;
    return result;
}

RecoveryResult RecoveryStrategies::Level4_TeleportToSafePosition(BotMovementController* controller)
{
    Unit* owner = GetOwner(controller);
    if (!owner || !owner->IsInWorld())
    {
        return RecoveryResult::Failure(RecoveryLevel::Level4_TeleportToSafePosition, "Owner invalid");
    }

    TC_LOG_DEBUG("movement.bot.recovery", "Level4: Teleporting to safe position for {}", owner->GetName());

    Position safePos = GetLastSafePosition(controller);

    if (!IsPositionSafe(owner, safePos))
    {
        // Try a random position instead
        safePos = GetRandomNearbyPosition(owner);
        if (!IsPositionSafe(owner, safePos))
        {
            return RecoveryResult::Failure(RecoveryLevel::Level4_TeleportToSafePosition, "Could not find safe position");
        }
    }

    // Teleport to safe position
    if (!TeleportToPosition(owner, safePos))
    {
        return RecoveryResult::Failure(RecoveryLevel::Level4_TeleportToSafePosition, "Teleport failed");
    }

    RecoveryResult result = RecoveryResult::Success(RecoveryLevel::Level4_TeleportToSafePosition, "Teleported to safe position");
    result.newPosition = safePos;
    return result;
}

RecoveryResult RecoveryStrategies::Level5_EvadeAndReset(BotMovementController* controller)
{
    Unit* owner = GetOwner(controller);
    if (!owner || !owner->IsInWorld())
    {
        return RecoveryResult::Failure(RecoveryLevel::Level5_EvadeAndReset, "Owner invalid");
    }

    TC_LOG_DEBUG("movement.bot.recovery", "Level5: Evading and resetting for {}", owner->GetName());

    // Clear all movement
    owner->GetMotionMaster()->Clear();

    // Stop combat if in combat
    if (owner->IsInCombat())
    {
        // Note: For bots we don't actually evade, just disengage movement
        TC_LOG_DEBUG("movement.bot.recovery", "Level5: Stopping combat movement for {}", owner->GetName());
    }

    // This is the last resort - always succeeds
    return RecoveryResult::Success(RecoveryLevel::Level5_EvadeAndReset, "Movement reset complete");
}

Position RecoveryStrategies::GetBackupPosition(Unit* unit)
{
    if (!unit)
        return Position();

    // Calculate position behind the unit
    float orientation = unit->GetOrientation();
    float backOrientation = orientation + M_PI; // Opposite direction

    float x = unit->GetPositionX() + BACKUP_DISTANCE * std::cos(backOrientation);
    float y = unit->GetPositionY() + BACKUP_DISTANCE * std::sin(backOrientation);
    float z = unit->GetPositionZ();

    // Get proper ground height
    if (Map* map = unit->GetMap())
    {
        float groundZ = map->GetHeight(unit->GetPhaseShift(), x, y, z, true);
        if (groundZ != INVALID_HEIGHT)
            z = groundZ;
    }

    return Position(x, y, z, orientation);
}

Position RecoveryStrategies::GetRandomNearbyPosition(Unit* unit)
{
    if (!unit)
        return Position();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * M_PI);
    std::uniform_real_distribution<float> distDist(3.0f, RANDOM_SEARCH_RADIUS);

    Map* map = unit->GetMap();
    if (!map)
        return Position();

    // Try multiple random positions
    for (uint32 i = 0; i < MAX_RANDOM_ATTEMPTS; ++i)
    {
        float angle = angleDist(gen);
        float dist = distDist(gen);

        float x = unit->GetPositionX() + dist * std::cos(angle);
        float y = unit->GetPositionY() + dist * std::sin(angle);
        float z = unit->GetPositionZ();

        // Get ground height at this position
        float groundZ = map->GetHeight(unit->GetPhaseShift(), x, y, z, true);
        if (groundZ == INVALID_HEIGHT)
            continue;

        Position testPos(x, y, groundZ + 0.5f, unit->GetOrientation());

        if (IsPositionSafe(unit, testPos))
        {
            return testPos;
        }
    }

    // Return current position if nothing found
    return unit->GetPosition();
}

Position RecoveryStrategies::GetLastSafePosition(BotMovementController* controller)
{
    if (!controller)
        return Position();

    Unit* owner = controller->GetOwner();
    if (!owner)
        return Position();

    // Check position history for a safe position
    auto const& history = controller->GetPositionHistory();

    // Look for a position that's safe
    for (auto it = history.rbegin(); it != history.rend(); ++it)
    {
        if (IsPositionSafe(owner, it->pos))
        {
            TC_LOG_DEBUG("movement.bot.recovery", "Found safe position from history at ({}, {}, {})",
                it->pos.GetPositionX(), it->pos.GetPositionY(), it->pos.GetPositionZ());
            return it->pos;
        }
    }

    // If no safe position in history, return current position
    return owner->GetPosition();
}

bool RecoveryStrategies::IsPositionSafe(Unit* unit, Position const& pos)
{
    if (!unit || !unit->IsInWorld())
        return false;

    // Check position validation
    ValidationResult validation = PositionValidator::ValidatePosition(unit->GetMapId(), pos);
    if (!validation.isValid)
        return false;

    // Check if position is in void
    if (GroundValidator::IsVoidPosition(unit))
        return false;

    // Check if in dangerous liquid
    LiquidInfo liquidInfo = LiquidValidator::GetLiquidInfoAt(unit->GetMap(), pos);
    if (liquidInfo.isDangerous)
        return false;

    // Check if stuck in geometry
    if (CollisionValidator::IsInsideGeometry(unit, pos))
        return false;

    return true;
}

bool RecoveryStrategies::TeleportToPosition(Unit* unit, Position const& pos)
{
    if (!unit || !unit->IsInWorld())
        return false;

    // Use NearTeleportTo for same-map teleport
    unit->NearTeleportTo(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation());

    TC_LOG_DEBUG("movement.bot.recovery", "Teleported {} to ({}, {}, {})",
        unit->GetName(), pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ());

    return true;
}
