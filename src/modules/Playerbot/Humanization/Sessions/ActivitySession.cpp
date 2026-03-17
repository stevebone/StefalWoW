/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ActivitySession.h"
#include "Log.h"
#include <sstream>
#include <iomanip>

namespace Playerbot
{
namespace Humanization
{

// Static member initialization
std::atomic<uint64> ActivitySession::_nextSessionId{1};

ActivitySession::ActivitySession()
    : _sessionId(0)
    , _botGuid()
    , _activityType(ActivityType::NONE)
{
}

ActivitySession::ActivitySession(ObjectGuid botGuid, ActivityType activity, uint32 targetDurationMs)
    : _sessionId(GenerateSessionId())
    , _botGuid(botGuid)
    , _activityType(activity)
    , _targetDurationMs(targetDurationMs)
{
}

uint64 ActivitySession::GenerateSessionId()
{
    return _nextSessionId.fetch_add(1, std::memory_order_relaxed);
}

uint32 ActivitySession::GetElapsedMs() const
{
    if (_state == SessionState::NOT_STARTED)
        return 0;

    auto endPoint = (_state == SessionState::COMPLETED ||
                     _state == SessionState::INTERRUPTED ||
                     _state == SessionState::FAILED)
                    ? _endTime
                    : std::chrono::steady_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        endPoint - _startTime).count();

    // Subtract pause time
    return static_cast<uint32>(elapsed) - _totalPauseMs;
}

uint32 ActivitySession::GetRemainingMs() const
{
    if (_state == SessionState::NOT_STARTED)
        return _targetDurationMs;

    if (HasEnded())
        return 0;

    uint32 elapsed = GetElapsedMs();
    uint32 totalTarget = _targetDurationMs + _totalExtendedMs;

    return elapsed >= totalTarget ? 0 : totalTarget - elapsed;
}

float ActivitySession::GetProgress() const
{
    if (_targetDurationMs == 0)
        return 0.0f;

    uint32 totalTarget = _targetDurationMs + _totalExtendedMs;
    return static_cast<float>(GetElapsedMs()) / static_cast<float>(totalTarget);
}

float ActivitySession::GetGoalProgress() const
{
    if (_goalValue == 0)
        return 0.0f;

    return static_cast<float>(_progressValue) / static_cast<float>(_goalValue);
}

bool ActivitySession::Extend(uint32 additionalMs)
{
    if (HasEnded())
        return false;

    _totalExtendedMs += additionalMs;
    _extensionCount++;

    AddCheckpoint("Session extended", additionalMs);

    TC_LOG_DEBUG("playerbots.humanization", "Session {} extended by {}ms (total extensions: {})",
        _sessionId, additionalMs, _extensionCount);

    return true;
}

bool ActivitySession::Start()
{
    if (_state != SessionState::NOT_STARTED)
        return false;

    _state = SessionState::ACTIVE;
    _startTime = std::chrono::steady_clock::now();

    AddCheckpoint("Session started", 0);

    TC_LOG_DEBUG("playerbots.humanization", "Session {} started: {} for {}ms",
        _sessionId, GetActivityName(_activityType), _targetDurationMs);

    return true;
}

bool ActivitySession::Pause(std::string const& reason)
{
    if (_state != SessionState::ACTIVE)
        return false;

    _state = SessionState::PAUSED;
    _pauseStartTime = std::chrono::steady_clock::now();

    std::string checkpoint = "Session paused";
    if (!reason.empty())
        checkpoint += ": " + reason;

    AddCheckpoint(checkpoint, 0);

    TC_LOG_DEBUG("playerbots.humanization", "Session {} paused: {}",
        _sessionId, reason);

    return true;
}

bool ActivitySession::Resume()
{
    if (_state != SessionState::PAUSED)
        return false;

    auto now = std::chrono::steady_clock::now();
    auto pauseDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - _pauseStartTime).count();

    _totalPauseMs += static_cast<uint32>(pauseDuration);
    _state = SessionState::ACTIVE;

    AddCheckpoint("Session resumed", static_cast<uint32>(pauseDuration));

    TC_LOG_DEBUG("playerbots.humanization", "Session {} resumed (was paused for {}ms)",
        _sessionId, pauseDuration);

    return true;
}

void ActivitySession::Complete(SessionEndReason reason)
{
    if (HasEnded())
        return;

    _state = SessionState::COMPLETED;
    _endReason = reason;
    _endTime = std::chrono::steady_clock::now();

    AddCheckpoint("Session completed", static_cast<uint32>(reason));

    TC_LOG_DEBUG("playerbots.humanization", "Session {} completed (reason: {}, duration: {}ms)",
        _sessionId, static_cast<uint8>(reason), GetElapsedMs());
}

void ActivitySession::Interrupt(SessionEndReason reason)
{
    if (HasEnded())
        return;

    // Resume first if paused
    if (_state == SessionState::PAUSED)
        Resume();

    _state = SessionState::INTERRUPTED;
    _endReason = reason;
    _endTime = std::chrono::steady_clock::now();

    AddCheckpoint("Session interrupted", static_cast<uint32>(reason));

    TC_LOG_DEBUG("playerbots.humanization", "Session {} interrupted (reason: {}, duration: {}ms)",
        _sessionId, static_cast<uint8>(reason), GetElapsedMs());
}

void ActivitySession::Fail(SessionEndReason reason)
{
    if (HasEnded())
        return;

    _state = SessionState::FAILED;
    _endReason = reason;
    _endTime = std::chrono::steady_clock::now();

    AddCheckpoint("Session failed", static_cast<uint32>(reason));

    TC_LOG_DEBUG("playerbots.humanization", "Session {} failed (reason: {}, duration: {}ms)",
        _sessionId, static_cast<uint8>(reason), GetElapsedMs());
}

void ActivitySession::AddCheckpoint(std::string const& description, uint32 value)
{
    _checkpoints.emplace_back(description, value);
}

void ActivitySession::SetMetadata(std::string const& key, std::string const& value)
{
    _metadata[key] = value;
}

std::string ActivitySession::GetMetadata(std::string const& key) const
{
    auto it = _metadata.find(key);
    return it != _metadata.end() ? it->second : "";
}

std::string ActivitySession::ToString() const
{
    std::ostringstream ss;

    ss << "Session #" << _sessionId << " [";

    switch (_state)
    {
        case SessionState::NOT_STARTED: ss << "NOT_STARTED"; break;
        case SessionState::ACTIVE:      ss << "ACTIVE"; break;
        case SessionState::PAUSED:      ss << "PAUSED"; break;
        case SessionState::COMPLETING:  ss << "COMPLETING"; break;
        case SessionState::COMPLETED:   ss << "COMPLETED"; break;
        case SessionState::INTERRUPTED: ss << "INTERRUPTED"; break;
        case SessionState::FAILED:      ss << "FAILED"; break;
    }

    ss << "] " << GetActivityName(_activityType);

    if (_state != SessionState::NOT_STARTED)
    {
        ss << " - " << GetElapsedMs() << "ms elapsed";
        if (!HasEnded())
            ss << " / " << GetRemainingMs() << "ms remaining";
    }

    if (_goalValue > 0)
    {
        ss << " (" << _progressValue << "/" << _goalValue << " = "
           << std::fixed << std::setprecision(1) << (GetGoalProgress() * 100.0f) << "%)";
    }

    return ss.str();
}

} // namespace Humanization
} // namespace Playerbot
