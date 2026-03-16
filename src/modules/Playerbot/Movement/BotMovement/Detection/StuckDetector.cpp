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

#include "StuckDetector.h"
#include "BotMovementController.h"
#include "Unit.h"
#include "Log.h"
#include <cmath>

StuckDetector::StuckDetector(BotMovementController* controller)
    : _controller(controller)
{
}

Unit* StuckDetector::GetOwner() const
{
    return _controller ? _controller->GetOwner() : nullptr;
}

void StuckDetector::Update(uint32 diff)
{
    _totalTimePassed += diff;
    _positionCheckTimer += diff;

    // Update stuck duration if stuck
    if (_stuckInfo.isStuck)
    {
        _stuckInfo.stuckDuration = _totalTimePassed - _stuckInfo.stuckStartTime;
    }

    // Periodic position check
    if (_positionCheckTimer >= POSITION_CHECK_INTERVAL)
    {
        _positionCheckTimer = 0;

        Unit* owner = GetOwner();
        if (owner && owner->IsInWorld())
        {
            RecordPosition(owner->GetPosition());
        }

        // Run detection checks
        if (!_stuckInfo.isStuck)
        {
            if (DetectPositionStuck())
            {
                SetStuck(StuckType::PositionStuck);
            }
            else if (DetectProgressStuck())
            {
                SetStuck(StuckType::ProgressStuck);
            }
            else if (DetectPathFailureStuck())
            {
                SetStuck(StuckType::PathFailureStuck);
            }
            else if (DetectCollisionStuck())
            {
                SetStuck(StuckType::CollisionStuck);
            }
        }
    }
}

void StuckDetector::RecordPosition(Position const& pos)
{
    PositionSnapshot snapshot(pos, _totalTimePassed);
    _positionHistory.push_back(snapshot);

    // Maintain max history size
    while (_positionHistory.size() > MAX_POSITION_HISTORY)
    {
        _positionHistory.pop_front();
    }
}

void StuckDetector::RecordPathFailure()
{
    _consecutivePathFailures++;

    TC_LOG_DEBUG("movement.bot.stuck", "StuckDetector: Path failure recorded ({} consecutive)",
        _consecutivePathFailures);
}

void StuckDetector::RecordCollision()
{
    _consecutiveCollisions++;

    TC_LOG_DEBUG("movement.bot.stuck", "StuckDetector: Collision recorded ({} consecutive)",
        _consecutiveCollisions);
}

void StuckDetector::RecordProgress(uint32 waypointIndex)
{
    if (waypointIndex != _lastWaypointIndex)
    {
        _lastWaypointIndex = waypointIndex;
        _lastProgressTime = _totalTimePassed;

        // Reset collision count on progress
        _consecutiveCollisions = 0;
        _consecutivePathFailures = 0;

        // If we were stuck, we might be recovered
        if (_stuckInfo.isStuck &&
            (_stuckInfo.type == StuckType::ProgressStuck ||
             _stuckInfo.type == StuckType::PositionStuck))
        {
            TC_LOG_DEBUG("movement.bot.stuck", "StuckDetector: Progress detected, clearing stuck state");
            ClearStuck();
        }
    }
}

void StuckDetector::Reset()
{
    ClearStuck();
    _positionHistory.clear();
    _consecutivePathFailures = 0;
    _consecutiveCollisions = 0;
    _lastProgressTime = _totalTimePassed;
}

void StuckDetector::ClearHistory()
{
    _positionHistory.clear();
    Reset();
}

bool StuckDetector::DetectPositionStuck()
{
    if (_positionHistory.size() < 2)
        return false;

    // Get oldest and newest positions
    PositionSnapshot const& oldest = _positionHistory.front();
    PositionSnapshot const& newest = _positionHistory.back();

    // Check if enough time has passed
    uint32 timeSpan = newest.timestamp - oldest.timestamp;
    if (timeSpan < _positionThresholdMs)
        return false;

    // Calculate distance moved
    float dx = newest.pos.GetPositionX() - oldest.pos.GetPositionX();
    float dy = newest.pos.GetPositionY() - oldest.pos.GetPositionY();
    float dz = newest.pos.GetPositionZ() - oldest.pos.GetPositionZ();
    float distanceMoved = std::sqrt(dx * dx + dy * dy + dz * dz);

    // If moved less than threshold over the time period, we're stuck
    if (distanceMoved < _distanceThreshold)
    {
        TC_LOG_DEBUG("movement.bot.stuck", "StuckDetector: Position stuck detected - moved only {} yards in {}ms",
            distanceMoved, timeSpan);
        return true;
    }

    return false;
}

bool StuckDetector::DetectProgressStuck()
{
    // Check if we haven't made waypoint progress in a while
    if (_lastProgressTime == 0)
        return false;

    uint32 timeSinceProgress = _totalTimePassed - _lastProgressTime;
    if (timeSinceProgress >= _progressThresholdMs)
    {
        TC_LOG_DEBUG("movement.bot.stuck", "StuckDetector: Progress stuck detected - no waypoint progress for {}ms",
            timeSinceProgress);
        return true;
    }

    return false;
}

bool StuckDetector::DetectPathFailureStuck()
{
    if (_consecutivePathFailures >= _pathFailureThreshold)
    {
        TC_LOG_DEBUG("movement.bot.stuck", "StuckDetector: Path failure stuck detected - {} consecutive failures",
            _consecutivePathFailures);
        return true;
    }

    return false;
}

bool StuckDetector::DetectCollisionStuck()
{
    if (_consecutiveCollisions >= _collisionThreshold)
    {
        TC_LOG_DEBUG("movement.bot.stuck", "StuckDetector: Collision stuck detected - {} consecutive collisions",
            _consecutiveCollisions);
        return true;
    }

    return false;
}

void StuckDetector::SetStuck(StuckType type)
{
    if (_stuckInfo.isStuck)
        return; // Already stuck

    Unit* owner = GetOwner();

    _stuckInfo.isStuck = true;
    _stuckInfo.type = type;
    _stuckInfo.stuckStartTime = _totalTimePassed;
    _stuckInfo.stuckDuration = 0;
    _stuckInfo.recoveryAttempts = 0;

    if (owner)
    {
        _stuckInfo.stuckPosition = owner->GetPosition();
    }

    TC_LOG_WARN("movement.bot.stuck", "StuckDetector: Bot {} is now STUCK (type: {})",
        owner ? owner->GetName() : "Unknown",
        static_cast<int>(type));
}

void StuckDetector::ClearStuck()
{
    if (!_stuckInfo.isStuck)
        return;

    Unit* owner = GetOwner();

    TC_LOG_DEBUG("movement.bot.stuck", "StuckDetector: Bot {} is no longer stuck (was stuck for {}ms)",
        owner ? owner->GetName() : "Unknown",
        _stuckInfo.stuckDuration);

    _stuckInfo.isStuck = false;
    _stuckInfo.type = StuckType::None;
    _stuckInfo.stuckDuration = 0;
    _stuckInfo.stuckStartTime = 0;
    _stuckInfo.recoveryAttempts = 0;
}
