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

#ifndef TRINITY_RECOVERYSTRATEGIES_H
#define TRINITY_RECOVERYSTRATEGIES_H

#include "Define.h"
#include "BotMovementDefines.h"
#include "Position.h"

class BotMovementController;
class Unit;

struct RecoveryResult
{
    bool success = false;
    RecoveryLevel levelUsed = RecoveryLevel::Level1_RecalculatePath;
    std::string message;
    Position newPosition;

    static RecoveryResult Success(RecoveryLevel level, std::string msg = "")
    {
        RecoveryResult result;
        result.success = true;
        result.levelUsed = level;
        result.message = std::move(msg);
        return result;
    }

    static RecoveryResult Failure(RecoveryLevel level, std::string msg = "")
    {
        RecoveryResult result;
        result.success = false;
        result.levelUsed = level;
        result.message = std::move(msg);
        return result;
    }
};

class TC_GAME_API RecoveryStrategies
{
public:
    // Main recovery entry point - tries recovery based on attempt count
    static RecoveryResult TryRecover(BotMovementController* controller, StuckType stuckType, uint32 attemptCount);

    // Individual recovery strategies
    static RecoveryResult Level1_RecalculatePath(BotMovementController* controller);
    static RecoveryResult Level2_BackupAndRetry(BotMovementController* controller);
    static RecoveryResult Level3_RandomNearbyPosition(BotMovementController* controller);
    static RecoveryResult Level4_TeleportToSafePosition(BotMovementController* controller);
    static RecoveryResult Level5_EvadeAndReset(BotMovementController* controller);

private:
    // Configuration
    static constexpr float BACKUP_DISTANCE = 5.0f;           // Yards to back up
    static constexpr float RANDOM_SEARCH_RADIUS = 10.0f;     // Radius for random position
    static constexpr uint32 SAFE_POSITION_LOOKBACK = 10000;  // Look back 10s for safe position
    static constexpr uint32 MAX_RANDOM_ATTEMPTS = 8;         // Max attempts to find random valid pos

    // Helper methods
    static Unit* GetOwner(BotMovementController* controller);
    static Position GetBackupPosition(Unit* unit);
    static Position GetRandomNearbyPosition(Unit* unit);
    static Position GetLastSafePosition(BotMovementController* controller);
    static bool IsPositionSafe(Unit* unit, Position const& pos);
    static bool TeleportToPosition(Unit* unit, Position const& pos);
};

#endif // TRINITY_RECOVERYSTRATEGIES_H
