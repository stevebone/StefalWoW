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

#ifndef TRINITY_STUCKDETECTOR_H
#define TRINITY_STUCKDETECTOR_H

#include "Define.h"
#include "BotMovementDefines.h"
#include "Position.h"
#include <deque>

class BotMovementController;
class Unit;

struct StuckInfo
{
    bool isStuck = false;
    StuckType type = StuckType::None;
    uint32 stuckDuration = 0;
    uint32 stuckStartTime = 0;
    Position stuckPosition;
    uint32 recoveryAttempts = 0;
};

class TC_GAME_API StuckDetector
{
public:
    explicit StuckDetector(BotMovementController* controller);
    ~StuckDetector() = default;

    StuckDetector(StuckDetector const&) = delete;
    StuckDetector& operator=(StuckDetector const&) = delete;

    // Update stuck detection
    void Update(uint32 diff);

    // Record events that may indicate being stuck
    void RecordPosition(Position const& pos);
    void RecordPathFailure();
    void RecordCollision();
    void RecordProgress(uint32 waypointIndex);

    // Reset stuck state
    void Reset();
    void ClearHistory();

    // Query stuck state
    bool IsStuck() const { return _stuckInfo.isStuck; }
    StuckType GetStuckType() const { return _stuckInfo.type; }
    uint32 GetStuckDuration() const { return _stuckInfo.stuckDuration; }
    StuckInfo const& GetStuckInfo() const { return _stuckInfo; }

    // Increment recovery attempts
    void IncrementRecoveryAttempts() { _stuckInfo.recoveryAttempts++; }
    uint32 GetRecoveryAttempts() const { return _stuckInfo.recoveryAttempts; }

    // Configuration
    void SetPositionThreshold(uint32 thresholdMs) { _positionThresholdMs = thresholdMs; }
    void SetDistanceThreshold(float distance) { _distanceThreshold = distance; }
    void SetPathFailureThreshold(uint32 count) { _pathFailureThreshold = count; }
    void SetCollisionThreshold(uint32 count) { _collisionThreshold = count; }

private:
    BotMovementController* _controller;
    StuckInfo _stuckInfo;

    // Position tracking
    std::deque<PositionSnapshot> _positionHistory;
    uint32 _lastProgressTime = 0;
    uint32 _lastWaypointIndex = 0;

    // Failure tracking
    uint32 _consecutivePathFailures = 0;
    uint32 _consecutiveCollisions = 0;

    // Timers
    uint32 _totalTimePassed = 0;
    uint32 _positionCheckTimer = 0;

    // Configuration thresholds
    uint32 _positionThresholdMs = 3000;     // 3 seconds without movement
    float _distanceThreshold = 2.0f;         // Less than 2 yards movement
    uint32 _pathFailureThreshold = 3;        // 3 consecutive path failures
    uint32 _collisionThreshold = 5;          // 5 consecutive collisions
    uint32 _progressThresholdMs = 5000;      // 5 seconds without waypoint progress

    static constexpr uint32 POSITION_CHECK_INTERVAL = 500;   // Check every 500ms
    static constexpr size_t MAX_POSITION_HISTORY = 20;

    // Detection methods
    bool DetectPositionStuck();
    bool DetectProgressStuck();
    bool DetectPathFailureStuck();
    bool DetectCollisionStuck();

    // Set stuck state
    void SetStuck(StuckType type);
    void ClearStuck();

    // Get owner unit
    Unit* GetOwner() const;
};

#endif // TRINITY_STUCKDETECTOR_H
