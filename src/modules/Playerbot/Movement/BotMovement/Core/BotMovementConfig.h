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

#ifndef TRINITYCORE_BOT_MOVEMENT_CONFIG_H
#define TRINITYCORE_BOT_MOVEMENT_CONFIG_H

#include "Define.h"
#include "Duration.h"
#include "BotMovementDefines.h"

class TC_GAME_API BotMovementConfig
{
public:
    BotMovementConfig() = default;
    ~BotMovementConfig() = default;

    void Load();
    void Reload();

    bool IsEnabled() const { return _enabled; }
    ValidationLevel GetValidationLevel() const { return _validationLevel; }

    // Validation toggles
    bool IsGroundValidationEnabled() const { return _groundValidation; }
    bool IsCollisionValidationEnabled() const { return _collisionValidation; }
    bool IsLiquidValidationEnabled() const { return _liquidValidation; }

    // Stuck detection
    bool IsStuckDetectionEnabled() const { return _stuckDetectionEnabled; }
    Milliseconds GetStuckPositionThreshold() const { return _stuckPosThreshold; }
    float GetStuckDistanceThreshold() const { return _stuckDistThreshold; }
    uint32 GetMaxRecoveryAttempts() const { return _maxRecoveryAttempts; }

    // Path cache
    bool IsPathCacheEnabled() const { return _pathCacheEnabled; }
    uint32 GetPathCacheSize() const { return _pathCacheSize; }
    Seconds GetPathCacheTTL() const { return _pathCacheTTL; }

    // Debug
    uint32 GetDebugLogLevel() const { return _debugLogLevel; }
    bool ShouldLogStateChanges() const { return _logStateChanges; }
    bool ShouldLogValidationFailures() const { return _logValidationFailures; }

private:
    bool _enabled = true;
    ValidationLevel _validationLevel = ValidationLevel::Standard;

    // Validation toggles
    bool _groundValidation = true;
    bool _collisionValidation = true;
    bool _liquidValidation = true;

    // Stuck detection
    bool _stuckDetectionEnabled = true;
    Milliseconds _stuckPosThreshold = Milliseconds(3000);
    float _stuckDistThreshold = 2.0f;
    uint32 _maxRecoveryAttempts = 5;

    // Path cache
    bool _pathCacheEnabled = true;
    uint32 _pathCacheSize = 1000;
    Seconds _pathCacheTTL = Seconds(60);

    // Debug
    uint32 _debugLogLevel = 2;
    bool _logStateChanges = false;
    bool _logValidationFailures = true;
};

#endif
