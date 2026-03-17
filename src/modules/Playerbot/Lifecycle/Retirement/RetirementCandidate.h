/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * @file RetirementCandidate.h
 * @brief Retirement candidate data structure
 *
 * Holds all information about a bot that is in the retirement process,
 * including timing information, state tracking, and audit data.
 *
 * Thread Safety:
 * - This is a data structure; thread safety is managed by caller
 * - Use appropriate locking when accessing from multiple threads
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "RetirementState.h"
#include "../BotLifecycleState.h"
#include "Character/ZoneLevelHelper.h"
#include <chrono>
#include <string>

namespace Playerbot
{

/**
 * @brief Represents a bot in the retirement queue
 *
 * Contains all information needed to manage a bot through the retirement
 * process, from initial queueing through final deletion.
 */
struct TC_GAME_API RetirementCandidate
{
    // ========================================================================
    // IDENTIFICATION
    // ========================================================================

    /**
     * @brief The bot's GUID
     */
    ObjectGuid botGuid;

    /**
     * @brief Bot's character name (for logging/audit)
     */
    std::string botName;

    /**
     * @brief Bot's level at time of queueing
     */
    uint32 levelAtQueue = 0;

    /**
     * @brief Bot's class (for statistics)
     */
    uint8 botClass = 0;

    /**
     * @brief Bot's race (for statistics)
     */
    uint8 botRace = 0;

    // ========================================================================
    // STATE
    // ========================================================================

    /**
     * @brief Current retirement state
     */
    RetirementState state = RetirementState::None;

    /**
     * @brief Current graceful exit stage (when state == Exiting)
     */
    GracefulExitStage exitStage = GracefulExitStage::None;

    /**
     * @brief Reason for cancellation (if cancelled)
     */
    RetirementCancelReason cancelReason = RetirementCancelReason::None;

    // ========================================================================
    // BRACKET INFORMATION
    // ========================================================================

    /**
     * @brief Level bracket index at queue time (0-3)
     */
    uint8 bracketAtQueue = 0;

    /**
     * @brief Expansion tier at queue time
     */
    ExpansionTier tierAtQueue = ExpansionTier::Starting;

    // ========================================================================
    // TIMING
    // ========================================================================

    /**
     * @brief When the bot was added to retirement queue
     */
    std::chrono::system_clock::time_point queuedAt;

    /**
     * @brief When cooling period ends (eligible for deletion)
     */
    std::chrono::system_clock::time_point coolingEndsAt;

    /**
     * @brief When graceful exit started
     */
    std::chrono::system_clock::time_point exitStartedAt;

    /**
     * @brief When current exit stage started
     */
    std::chrono::system_clock::time_point stageStartedAt;

    /**
     * @brief When retirement was completed or cancelled
     */
    std::chrono::system_clock::time_point completedAt;

    // ========================================================================
    // SCORES AND METRICS
    // ========================================================================

    /**
     * @brief Protection score at time of queueing
     */
    float protectionScoreAtQueue = 0.0f;

    /**
     * @brief Retirement priority score (higher = retire sooner)
     *
     * Calculated from:
     * - Inverse protection score
     * - Bracket overpopulation
     * - Time in bracket
     * - Playtime
     */
    float retirementPriority = 0.0f;

    /**
     * @brief Bot's total playtime in minutes at queue time
     */
    uint32 playtimeMinutesAtQueue = 0;

    /**
     * @brief Time spent in current bracket at queue time (seconds)
     */
    uint32 timeInBracketAtQueue = 0;

    /**
     * @brief Number of player interactions at queue time
     */
    uint32 interactionCountAtQueue = 0;

    // ========================================================================
    // REASON TRACKING
    // ========================================================================

    /**
     * @brief Human-readable reason for retirement
     */
    std::string retirementReason;

    /**
     * @brief Additional notes (for audit trail)
     */
    std::string notes;

    // ========================================================================
    // GRACEFUL EXIT TRACKING
    // ========================================================================

    /**
     * @brief Guild GUID if bot was in a guild
     */
    ObjectGuid guildGuid;

    /**
     * @brief Whether guild leaving was needed and completed
     */
    bool guildLeftSuccessfully = false;

    /**
     * @brief Number of mail items that needed clearing
     */
    uint32 mailItemsCleared = 0;

    /**
     * @brief Number of auctions that needed cancelling
     */
    uint32 auctionsCancelled = 0;

    /**
     * @brief Whether bot was successfully logged out
     */
    bool loggedOutSuccessfully = false;

    /**
     * @brief Whether character was successfully deleted
     */
    bool characterDeleted = false;

    // ========================================================================
    // RETRY AND ERROR TRACKING
    // ========================================================================

    /**
     * @brief Number of retry attempts for current stage
     */
    uint8 stageRetryCount = 0;

    /**
     * @brief Maximum retries before forced skip
     */
    static constexpr uint8 MAX_STAGE_RETRIES = 3;

    /**
     * @brief Last error message (if any)
     */
    std::string lastError;

    // ========================================================================
    // UTILITY METHODS
    // ========================================================================

    /**
     * @brief Default constructor
     */
    RetirementCandidate() = default;

    /**
     * @brief Construct with bot GUID
     */
    explicit RetirementCandidate(ObjectGuid guid)
        : botGuid(guid)
        , queuedAt(std::chrono::system_clock::now())
    {
    }

    /**
     * @brief Check if cooling period has expired
     */
    bool IsCoolingExpired() const
    {
        return std::chrono::system_clock::now() >= coolingEndsAt;
    }

    /**
     * @brief Get remaining cooling time
     * @return Remaining time, or zero if expired
     */
    std::chrono::seconds GetRemainingCoolingTime() const
    {
        auto now = std::chrono::system_clock::now();
        if (now >= coolingEndsAt)
            return std::chrono::seconds(0);

        return std::chrono::duration_cast<std::chrono::seconds>(coolingEndsAt - now);
    }

    /**
     * @brief Get time since queued
     */
    std::chrono::seconds GetTimeInQueue() const
    {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now() - queuedAt);
    }

    /**
     * @brief Get time spent in current exit stage
     */
    std::chrono::milliseconds GetTimeInCurrentStage() const
    {
        if (state != RetirementState::Exiting)
            return std::chrono::milliseconds(0);

        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - stageStartedAt);
    }

    /**
     * @brief Check if current stage has timed out
     */
    bool IsStageTimedOut() const
    {
        if (state != RetirementState::Exiting)
            return false;

        uint32 expectedTime = EstimateStageTime(exitStage);
        uint32 timeout = expectedTime * 3;  // 3x expected time

        return GetTimeInCurrentStage().count() > timeout;
    }

    /**
     * @brief Advance to next exit stage
     */
    void AdvanceExitStage()
    {
        exitStage = GetNextExitStage(exitStage);
        stageStartedAt = std::chrono::system_clock::now();
        stageRetryCount = 0;
        lastError.clear();
    }

    /**
     * @brief Record stage error
     */
    void RecordError(std::string const& error)
    {
        lastError = error;
        ++stageRetryCount;
    }

    /**
     * @brief Check if max retries exceeded
     */
    bool HasExceededRetries() const
    {
        return stageRetryCount >= MAX_STAGE_RETRIES;
    }

    /**
     * @brief Start cooling period
     * @param coolingDays Number of days in cooling period
     */
    void StartCooling(uint32 coolingDays)
    {
        state = RetirementState::Cooling;
        coolingEndsAt = queuedAt + std::chrono::hours(coolingDays * 24);
    }

    /**
     * @brief Start graceful exit process
     */
    void StartGracefulExit()
    {
        state = RetirementState::Exiting;
        exitStage = GracefulExitStage::LeavingGuild;
        exitStartedAt = std::chrono::system_clock::now();
        stageStartedAt = exitStartedAt;
    }

    /**
     * @brief Cancel retirement
     * @param reason Why retirement was cancelled
     */
    void Cancel(RetirementCancelReason reason)
    {
        state = RetirementState::Cancelled;
        cancelReason = reason;
        completedAt = std::chrono::system_clock::now();
    }

    /**
     * @brief Complete retirement
     */
    void Complete()
    {
        state = RetirementState::Completed;
        exitStage = GracefulExitStage::Complete;
        completedAt = std::chrono::system_clock::now();
    }

    /**
     * @brief Get total graceful exit duration
     */
    std::chrono::milliseconds GetGracefulExitDuration() const
    {
        if (state != RetirementState::Completed && state != RetirementState::Exiting)
            return std::chrono::milliseconds(0);

        auto endTime = (state == RetirementState::Completed) ? completedAt : std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(endTime - exitStartedAt);
    }

    /**
     * @brief Get summary for logging
     */
    std::string GetSummary() const
    {
        std::string summary = "Bot " + botName + " [" + botGuid.ToString() + "] ";
        summary += "State: " + RetirementStateToString(state);

        if (state == RetirementState::Exiting)
            summary += " Stage: " + GracefulExitStageToString(exitStage);

        if (state == RetirementState::Cooling)
        {
            auto remaining = GetRemainingCoolingTime();
            summary += " Cooling: " + std::to_string(remaining.count()) + "s remaining";
        }

        if (state == RetirementState::Cancelled)
            summary += " Reason: " + RetirementCancelReasonToString(cancelReason);

        return summary;
    }

    /**
     * @brief Validate candidate data
     * @return Error message if invalid, empty string if valid
     */
    std::string Validate() const
    {
        if (!botGuid)
            return "Invalid bot GUID";

        if (state == RetirementState::None)
            return "Candidate not initialized";

        if (state == RetirementState::Exiting && exitStage == GracefulExitStage::None)
            return "Exiting state with no exit stage";

        return "";
    }

    // ========================================================================
    // COMPARISON FOR PRIORITY QUEUE
    // ========================================================================

    /**
     * @brief Compare for priority queue (higher priority = process first)
     */
    bool operator<(RetirementCandidate const& other) const
    {
        // Different states have different priorities
        if (state != other.state)
        {
            // Process active exits first, then cooling, then pending
            static const std::array<int, static_cast<size_t>(RetirementState::Max)> statePriority = {
                0,  // None
                1,  // Pending
                2,  // Cooling
                3,  // Preparing
                4,  // Exiting
                0,  // Cancelled
                0   // Completed
            };
            return statePriority[static_cast<size_t>(state)] >
                   statePriority[static_cast<size_t>(other.state)];
        }

        // Within same state, use retirement priority
        return retirementPriority > other.retirementPriority;
    }
};

/**
 * @brief Statistics about retirement processing
 */
struct RetirementStatistics
{
    // Current queue state
    uint32 pendingCount = 0;
    uint32 coolingCount = 0;
    uint32 preparingCount = 0;
    uint32 exitingCount = 0;

    // Completed statistics
    uint32 completedThisHour = 0;
    uint32 completedToday = 0;
    uint32 completedThisWeek = 0;
    uint32 completedTotal = 0;

    // Cancelled statistics
    uint32 cancelledThisHour = 0;
    uint32 cancelledToday = 0;
    uint32 cancelledTotal = 0;

    // By bracket
    std::array<uint32, 4> queuedByBracket = {0, 0, 0, 0};
    std::array<uint32, 4> completedByBracket = {0, 0, 0, 0};

    // Timing
    float avgCoolingDays = 0.0f;
    float avgGracefulExitSeconds = 0.0f;

    // Rate limiting
    uint32 maxPerHour = 0;
    uint32 processedThisHour = 0;

    // Error tracking
    uint32 failedStages = 0;
    uint32 forceSkippedStages = 0;

    // Last update
    std::chrono::system_clock::time_point lastUpdate;

    /**
     * @brief Get total queue size
     */
    uint32 GetTotalQueueSize() const
    {
        return pendingCount + coolingCount + preparingCount + exitingCount;
    }

    /**
     * @brief Check if rate limit allows more processing
     */
    bool CanProcessMore() const
    {
        return processedThisHour < maxPerHour;
    }

    /**
     * @brief Get remaining capacity this hour
     */
    uint32 GetRemainingCapacity() const
    {
        return (maxPerHour > processedThisHour) ? (maxPerHour - processedThisHour) : 0;
    }
};

} // namespace Playerbot
