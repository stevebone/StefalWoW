/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * ACTIVITY SESSION
 *
 * Phase 3: Humanization Core
 *
 * Represents a single activity session - what the bot is doing,
 * how long it's been doing it, and when it should transition.
 */

#pragma once

#include "Define.h"
#include "../Core/ActivityType.h"
#include "ObjectGuid.h"
#include <chrono>
#include <string>
#include <vector>

namespace Playerbot
{
namespace Humanization
{

/**
 * @brief Session state
 */
enum class SessionState : uint8
{
    NOT_STARTED = 0,    // Session hasn't begun
    ACTIVE,             // Session is in progress
    PAUSED,             // Session is paused (break, interrupt)
    COMPLETING,         // Session is wrapping up
    COMPLETED,          // Session finished normally
    INTERRUPTED,        // Session was interrupted
    FAILED              // Session failed
};

/**
 * @brief Session completion reason
 */
enum class SessionEndReason : uint8
{
    NONE = 0,           // Session not ended
    DURATION_EXPIRED,   // Normal completion - time limit reached
    GOAL_ACHIEVED,      // Goal was accomplished
    BAGS_FULL,          // Can't continue - bags are full
    INTERRUPTED_COMBAT, // Interrupted by combat
    INTERRUPTED_USER,   // Interrupted by user action
    INTERRUPTED_DEATH,  // Bot died
    LOCATION_CHANGED,   // Bot moved away from session area
    RESOURCE_DEPLETED,  // No more resources (nodes, mobs)
    TRANSITION,         // Natural transition to another activity
    MANUAL_STOP,        // Manually stopped
    SESSION_ERROR       // Some error occurred
};

/**
 * @brief Session progress checkpoint
 */
struct SessionCheckpoint
{
    std::chrono::steady_clock::time_point timestamp;
    std::string description;
    uint32 progressValue;

    SessionCheckpoint()
        : timestamp(std::chrono::steady_clock::now())
        , progressValue(0)
    {}

    SessionCheckpoint(std::string const& desc, uint32 value)
        : timestamp(std::chrono::steady_clock::now())
        , description(desc)
        , progressValue(value)
    {}
};

/**
 * @brief Activity session data
 *
 * Tracks a single activity session for a bot.
 * Immutable after creation except for state updates.
 */
class TC_GAME_API ActivitySession
{
public:
    /**
     * @brief Construct a new activity session
     * @param botGuid GUID of the bot
     * @param activity Type of activity
     * @param targetDurationMs Target duration in milliseconds
     */
    ActivitySession(ObjectGuid botGuid, ActivityType activity, uint32 targetDurationMs);

    /**
     * @brief Default constructor for containers
     */
    ActivitySession();

    ~ActivitySession() = default;

    // ========================================================================
    // IDENTITY
    // ========================================================================

    /**
     * @brief Get session ID (unique per session)
     */
    uint64 GetSessionId() const { return _sessionId; }

    /**
     * @brief Get bot GUID
     */
    ObjectGuid GetBotGuid() const { return _botGuid; }

    /**
     * @brief Get activity type
     */
    ActivityType GetActivityType() const { return _activityType; }

    /**
     * @brief Get activity category
     */
    ActivityCategory GetCategory() const { return GetActivityCategory(_activityType); }

    // ========================================================================
    // STATE
    // ========================================================================

    /**
     * @brief Get current session state
     */
    SessionState GetState() const { return _state; }

    /**
     * @brief Is the session active?
     */
    bool IsActive() const { return _state == SessionState::ACTIVE; }

    /**
     * @brief Is the session paused?
     */
    bool IsPaused() const { return _state == SessionState::PAUSED; }

    /**
     * @brief Has the session ended?
     */
    bool HasEnded() const
    {
        return _state == SessionState::COMPLETED ||
               _state == SessionState::INTERRUPTED ||
               _state == SessionState::FAILED;
    }

    /**
     * @brief Get end reason (if ended)
     */
    SessionEndReason GetEndReason() const { return _endReason; }

    // ========================================================================
    // TIMING
    // ========================================================================

    /**
     * @brief Get session start time
     */
    std::chrono::steady_clock::time_point GetStartTime() const { return _startTime; }

    /**
     * @brief Get session end time (if ended)
     */
    std::chrono::steady_clock::time_point GetEndTime() const { return _endTime; }

    /**
     * @brief Get target duration in milliseconds
     */
    uint32 GetTargetDurationMs() const { return _targetDurationMs; }

    /**
     * @brief Get elapsed time in milliseconds
     */
    uint32 GetElapsedMs() const;

    /**
     * @brief Get remaining time in milliseconds
     * @return 0 if session should end
     */
    uint32 GetRemainingMs() const;

    /**
     * @brief Get elapsed time as percentage of target
     * @return 0.0 to 1.0 (can exceed 1.0 if extended)
     */
    float GetProgress() const;

    /**
     * @brief Is the session over its target duration?
     */
    bool IsOvertime() const { return GetElapsedMs() > _targetDurationMs; }

    /**
     * @brief Get total pause time in milliseconds
     */
    uint32 GetTotalPauseMs() const { return _totalPauseMs; }

    // ========================================================================
    // EXTENSIONS
    // ========================================================================

    /**
     * @brief Get number of extensions applied
     */
    uint32 GetExtensionCount() const { return _extensionCount; }

    /**
     * @brief Extend the session duration
     * @param additionalMs Additional time in milliseconds
     * @return true if extended successfully
     */
    bool Extend(uint32 additionalMs);

    /**
     * @brief Get total extended time in milliseconds
     */
    uint32 GetTotalExtendedMs() const { return _totalExtendedMs; }

    // ========================================================================
    // STATE TRANSITIONS
    // ========================================================================

    /**
     * @brief Start the session
     * @return true if successfully started
     */
    bool Start();

    /**
     * @brief Pause the session
     * @param reason Description of why paused
     * @return true if successfully paused
     */
    bool Pause(std::string const& reason = "");

    /**
     * @brief Resume the session from pause
     * @return true if successfully resumed
     */
    bool Resume();

    /**
     * @brief Complete the session normally
     * @param reason Why the session completed
     */
    void Complete(SessionEndReason reason = SessionEndReason::DURATION_EXPIRED);

    /**
     * @brief Interrupt the session
     * @param reason Why the session was interrupted
     */
    void Interrupt(SessionEndReason reason);

    /**
     * @brief Mark the session as failed
     * @param reason Why the session failed
     */
    void Fail(SessionEndReason reason);

    // ========================================================================
    // PROGRESS TRACKING
    // ========================================================================

    /**
     * @brief Add a progress checkpoint
     * @param description What happened
     * @param value Numeric value (context-dependent)
     */
    void AddCheckpoint(std::string const& description, uint32 value = 0);

    /**
     * @brief Get all checkpoints
     */
    std::vector<SessionCheckpoint> const& GetCheckpoints() const { return _checkpoints; }

    /**
     * @brief Set custom progress value
     * @param value Context-dependent progress value
     */
    void SetProgressValue(uint32 value) { _progressValue = value; }

    /**
     * @brief Get custom progress value
     */
    uint32 GetProgressValue() const { return _progressValue; }

    /**
     * @brief Set goal value for progress tracking
     */
    void SetGoalValue(uint32 value) { _goalValue = value; }

    /**
     * @brief Get goal value
     */
    uint32 GetGoalValue() const { return _goalValue; }

    /**
     * @brief Get goal completion percentage
     */
    float GetGoalProgress() const;

    // ========================================================================
    // METADATA
    // ========================================================================

    /**
     * @brief Set additional session data
     * @param key Data key
     * @param value Data value
     */
    void SetMetadata(std::string const& key, std::string const& value);

    /**
     * @brief Get additional session data
     * @param key Data key
     * @return Value or empty string if not found
     */
    std::string GetMetadata(std::string const& key) const;

    // ========================================================================
    // SERIALIZATION
    // ========================================================================

    /**
     * @brief Get a summary string for logging
     */
    std::string ToString() const;

private:
    // Generate unique session ID
    static uint64 GenerateSessionId();

    // Identity
    uint64 _sessionId;
    ObjectGuid _botGuid;
    ActivityType _activityType;

    // State
    SessionState _state{SessionState::NOT_STARTED};
    SessionEndReason _endReason{SessionEndReason::NONE};

    // Timing
    std::chrono::steady_clock::time_point _startTime;
    std::chrono::steady_clock::time_point _endTime;
    std::chrono::steady_clock::time_point _pauseStartTime;
    uint32 _targetDurationMs{0};
    uint32 _totalPauseMs{0};
    uint32 _extensionCount{0};
    uint32 _totalExtendedMs{0};

    // Progress
    std::vector<SessionCheckpoint> _checkpoints;
    uint32 _progressValue{0};
    uint32 _goalValue{0};

    // Metadata
    std::unordered_map<std::string, std::string> _metadata;

    // Static session ID counter
    static std::atomic<uint64> _nextSessionId;
};

} // namespace Humanization
} // namespace Playerbot
