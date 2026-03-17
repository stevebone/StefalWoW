/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * ACTIVITY SESSION MANAGER
 *
 * Phase 3: Humanization Core
 *
 * Manages activity sessions for a single bot.
 * Handles session lifecycle, transitions, and history.
 */

#pragma once

#include "Define.h"
#include "ActivitySession.h"
#include "../Core/PersonalityProfile.h"
#include "../Core/HumanizationConfig.h"
#include "../Core/ActivityExecutor.h"
#include "ObjectGuid.h"
#include <memory>
#include <vector>
#include <queue>
#include <atomic>

class Player;

namespace Playerbot
{
namespace Humanization
{

/**
 * @brief Session transition request
 */
struct SessionTransitionRequest
{
    ActivityType targetActivity;
    uint32 targetDurationMs;
    bool immediate;             // Skip current session wrap-up
    std::string reason;

    SessionTransitionRequest()
        : targetActivity(ActivityType::NONE)
        , targetDurationMs(0)
        , immediate(false)
    {}

    SessionTransitionRequest(ActivityType target, uint32 duration, bool immed = false, std::string const& rsn = "")
        : targetActivity(target)
        , targetDurationMs(duration)
        , immediate(immed)
        , reason(rsn)
    {}
};

/**
 * @brief Session history entry (lightweight)
 */
struct SessionHistoryEntry
{
    uint64 sessionId;
    ActivityType activityType;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;
    SessionEndReason endReason;
    uint32 durationMs;
    uint32 progressValue;

    SessionHistoryEntry()
        : sessionId(0)
        , activityType(ActivityType::NONE)
        , endReason(SessionEndReason::NONE)
        , durationMs(0)
        , progressValue(0)
    {}
};

/**
 * @brief Activity Session Manager
 *
 * Per-bot manager that handles:
 * - Current session lifecycle
 * - Session transitions
 * - Activity selection based on personality
 * - Session history tracking
 * - Break scheduling
 */
class TC_GAME_API ActivitySessionManager
{
public:
    /**
     * @brief Construct session manager for a bot
     * @param bot The player bot this manager serves
     */
    explicit ActivitySessionManager(Player* bot);

    ~ActivitySessionManager();

    // Non-copyable
    ActivitySessionManager(ActivitySessionManager const&) = delete;
    ActivitySessionManager& operator=(ActivitySessionManager const&) = delete;

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /**
     * @brief Initialize the session manager
     */
    void Initialize();

    /**
     * @brief Update session state
     * @param diff Time since last update in milliseconds
     */
    void Update(uint32 diff);

    /**
     * @brief Shutdown and cleanup
     */
    void Shutdown();

    // ========================================================================
    // CURRENT SESSION
    // ========================================================================

    /**
     * @brief Get current session (if any)
     * @return Pointer to current session, nullptr if none
     */
    ActivitySession* GetCurrentSession() const { return _currentSession.get(); }

    /**
     * @brief Check if there is an active session
     */
    bool HasActiveSession() const;

    /**
     * @brief Get current activity type
     * @return Current activity, or NONE if no session
     */
    ActivityType GetCurrentActivity() const;

    /**
     * @brief Get current activity category
     */
    ActivityCategory GetCurrentCategory() const;

    /**
     * @brief Is the bot currently on a break?
     */
    bool IsOnBreak() const { return _isOnBreak; }

    // ========================================================================
    // SESSION CONTROL
    // ========================================================================

    /**
     * @brief Start a new session
     * @param activity Activity to start
     * @param durationMs Duration in milliseconds (0 = use default)
     * @return true if session started successfully
     */
    bool StartSession(ActivityType activity, uint32 durationMs = 0);

    /**
     * @brief End the current session
     * @param reason Why the session is ending
     */
    void EndSession(SessionEndReason reason = SessionEndReason::MANUAL_STOP);

    /**
     * @brief Pause the current session
     * @param reason Why pausing
     * @return true if paused successfully
     */
    bool PauseSession(std::string const& reason = "");

    /**
     * @brief Resume the current session
     * @return true if resumed successfully
     */
    bool ResumeSession();

    /**
     * @brief Request a transition to another activity
     * @param request Transition request details
     * @return true if request was queued
     */
    bool RequestTransition(SessionTransitionRequest const& request);

    /**
     * @brief Force immediate transition (interrupts current session)
     * @param activity Activity to transition to
     * @param durationMs Duration in milliseconds
     */
    void ForceTransition(ActivityType activity, uint32 durationMs = 0);

    // ========================================================================
    // BREAK MANAGEMENT
    // ========================================================================

    /**
     * @brief Start a break
     * @param durationMs Break duration (0 = use personality default)
     * @return true if break started
     */
    bool StartBreak(uint32 durationMs = 0);

    /**
     * @brief End the current break early
     */
    void EndBreak();

    /**
     * @brief Get remaining break time in milliseconds
     */
    uint32 GetRemainingBreakMs() const;

    /**
     * @brief Should the bot take a break now?
     * @return true if break is recommended
     */
    bool ShouldTakeBreak() const;

    // ========================================================================
    // PERSONALITY
    // ========================================================================

    /**
     * @brief Set the bot's personality profile
     * @param profile Personality profile to use
     */
    void SetPersonality(PersonalityProfile const& profile);

    /**
     * @brief Get the bot's personality profile
     */
    PersonalityProfile const& GetPersonality() const { return _personality; }

    /**
     * @brief Get mutable reference to personality
     */
    PersonalityProfile& GetPersonalityMutable() { return _personality; }

    // ========================================================================
    // ACTIVITY SELECTION
    // ========================================================================

    /**
     * @brief Select next activity based on personality and context
     * @return Recommended next activity
     */
    ActivityType SelectNextActivity() const;

    /**
     * @brief Get available activities for current context
     * @return Vector of available activity types
     */
    std::vector<ActivityType> GetAvailableActivities() const;

    /**
     * @brief Calculate session duration for an activity
     * @param activity Activity type
     * @return Duration in milliseconds
     */
    uint32 CalculateSessionDuration(ActivityType activity) const;

    // ========================================================================
    // HISTORY
    // ========================================================================

    /**
     * @brief Get session history
     * @param maxEntries Maximum entries to return (0 = all)
     * @return Vector of history entries (newest first)
     */
    std::vector<SessionHistoryEntry> GetHistory(uint32 maxEntries = 0) const;

    /**
     * @brief Get total time spent on an activity category
     * @param category Activity category
     * @return Total time in milliseconds
     */
    uint32 GetTotalTimeInCategory(ActivityCategory category) const;

    /**
     * @brief Get session count for today
     */
    uint32 GetTodaySessionCount() const { return _todaySessionCount; }

    /**
     * @brief Clear session history
     */
    void ClearHistory();

    // ========================================================================
    // METRICS
    // ========================================================================

    struct SessionMetrics
    {
        std::atomic<uint32> totalSessions{0};
        std::atomic<uint32> completedSessions{0};
        std::atomic<uint32> interruptedSessions{0};
        std::atomic<uint32> failedSessions{0};
        std::atomic<uint32> totalBreaks{0};
        std::atomic<uint64> totalActiveTimeMs{0};
        std::atomic<uint64> totalBreakTimeMs{0};

        void Reset()
        {
            totalSessions = 0;
            completedSessions = 0;
            interruptedSessions = 0;
            failedSessions = 0;
            totalBreaks = 0;
            totalActiveTimeMs = 0;
            totalBreakTimeMs = 0;
        }
    };

    SessionMetrics const& GetMetrics() const { return _metrics; }

private:
    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /**
     * @brief Process pending transition requests
     */
    void ProcessTransitionQueue();

    /**
     * @brief Check if current session should end
     */
    void CheckSessionExpiry();

    /**
     * @brief Check if break should end
     */
    void CheckBreakExpiry();

    /**
     * @brief Archive current session to history
     */
    void ArchiveCurrentSession();

    /**
     * @brief Get default duration for an activity
     */
    uint32 GetDefaultDuration(ActivityType activity) const;

    /**
     * @brief Check if activity is valid for current context
     */
    bool IsActivityAvailable(ActivityType activity) const;

    // ========================================================================
    // DATA MEMBERS
    // ========================================================================

    // Bot reference
    Player* _bot;
    ObjectGuid _botGuid;

    // Current session
    std::unique_ptr<ActivitySession> _currentSession;

    // Activity executor (executes real behavior)
    std::unique_ptr<ActivityExecutor> _activityExecutor;

    // Session transitions (handles activity flow logic)
    std::unique_ptr<class SessionTransitions> _sessionTransitions;

    // Personality
    PersonalityProfile _personality;

    // Transition queue
    std::queue<SessionTransitionRequest> _transitionQueue;

    // Break state
    bool _isOnBreak{false};
    std::chrono::steady_clock::time_point _breakStartTime;
    uint32 _breakDurationMs{0};

    // History
    std::vector<SessionHistoryEntry> _history;
    static constexpr uint32 MAX_HISTORY_SIZE = 100;

    // Daily tracking
    uint32 _todaySessionCount{0};
    std::chrono::system_clock::time_point _lastDayReset;

    // Timing
    uint32 _updateTimer{0};
    static constexpr uint32 UPDATE_INTERVAL = 1000;  // 1 second

    // Time since last activity
    uint32 _idleTimeMs{0};

    // Metrics
    SessionMetrics _metrics;

    // Initialization flag
    bool _initialized{false};
};

} // namespace Humanization
} // namespace Playerbot
