/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * @file RetirementState.h
 * @brief Retirement state definitions for bot lifecycle management
 *
 * Defines the states a bot can be in during the retirement process.
 * The retirement process follows a staged approach:
 *
 * 1. NONE      - Bot is not in retirement queue
 * 2. PENDING   - Bot marked for retirement, awaiting processing
 * 3. COOLING   - In cooling period (can be rescued by protection)
 * 4. PREPARING - Preparing for graceful exit (saving state)
 * 5. EXITING   - Graceful exit in progress (leaving guild, etc.)
 * 6. CANCELLED - Rescued from retirement (protection gained)
 * 7. COMPLETED - Successfully retired and deleted
 *
 * Thread Safety:
 * - All enum operations are inherently thread-safe
 * - State transitions should be protected by mutex in caller
 */

#pragma once

#include "Define.h"
#include <string>

namespace Playerbot
{

/**
 * @brief States in the retirement lifecycle
 *
 * These states represent the progression of a bot through retirement.
 * State transitions are managed by BotRetirementManager.
 */
enum class RetirementState : uint8
{
    /**
     * @brief Not in retirement queue
     *
     * The bot is active and not scheduled for retirement.
     * This is the default state for all bots.
     */
    None = 0,

    /**
     * @brief Marked for retirement, awaiting processing
     *
     * The bot has been identified for retirement but processing
     * has not yet begun. This is a transient state.
     *
     * Transition from: None
     * Transition to: Cooling, Cancelled
     */
    Pending = 1,

    /**
     * @brief In cooling period (can be rescued)
     *
     * The bot is in a grace period where it can be rescued
     * if protection is gained (e.g., player adds to friends,
     * invites to guild, groups with the bot).
     *
     * Default cooling period: 7 days
     * Configurable via: Playerbot.Lifecycle.Retirement.CoolingPeriodDays
     *
     * Transition from: Pending
     * Transition to: Preparing, Cancelled
     */
    Cooling = 2,

    /**
     * @brief Preparing for graceful exit
     *
     * The cooling period has expired. The system is now
     * preparing for graceful exit by saving important state
     * and notifying relevant systems.
     *
     * Transition from: Cooling
     * Transition to: Exiting
     */
    Preparing = 3,

    /**
     * @brief Graceful exit in progress
     *
     * The bot is actively exiting the game world:
     * 1. Leaving guild (if member)
     * 2. Clearing mail (delete/return)
     * 3. Cancelling auctions (return items)
     * 4. Saving final state
     * 5. Logging out
     * 6. Character deletion
     *
     * This is a point of no return - cannot be cancelled.
     *
     * Transition from: Preparing
     * Transition to: Completed
     */
    Exiting = 4,

    /**
     * @brief Rescued from retirement
     *
     * The bot gained protection during the cooling period
     * and has been removed from the retirement queue.
     *
     * This state is temporary - the bot will return to None
     * after the rescue is processed.
     *
     * Transition from: Pending, Cooling
     * Transition to: None
     */
    Cancelled = 5,

    /**
     * @brief Successfully retired and deleted
     *
     * Terminal state. The bot has been deleted from the database.
     * This state is recorded for audit purposes only.
     *
     * Transition from: Exiting
     * Transition to: (none - terminal)
     */
    Completed = 6,

    /**
     * @brief Maximum enum value for iteration
     */
    Max
};

/**
 * @brief Graceful exit stages within Exiting state
 *
 * These sub-stages track progress through the graceful exit process.
 * Each stage must complete before proceeding to the next.
 */
enum class GracefulExitStage : uint8
{
    /**
     * @brief Not in graceful exit
     */
    None = 0,

    /**
     * @brief Leaving guild (if member)
     *
     * Bot gracefully leaves guild with system message.
     * If guild master, leadership is transferred first.
     */
    LeavingGuild = 1,

    /**
     * @brief Clearing mail
     *
     * Mail with items is returned to sender.
     * Mail without items is deleted.
     */
    ClearingMail = 2,

    /**
     * @brief Cancelling auctions
     *
     * Active auctions are cancelled.
     * Items are returned via mail (will be deleted).
     * Gold from sold auctions is processed normally.
     */
    CancellingAuctions = 3,

    /**
     * @brief Saving final state
     *
     * Any final state saving before deletion.
     * Statistics are recorded for audit.
     */
    SavingState = 4,

    /**
     * @brief Logging out the bot session
     *
     * Bot session is properly terminated.
     * All references are cleaned up.
     */
    LoggingOut = 5,

    /**
     * @brief Deleting character from database
     *
     * Final step - character is deleted.
     * This is irreversible.
     */
    DeletingCharacter = 6,

    /**
     * @brief Graceful exit complete
     *
     * All stages completed successfully.
     */
    Complete = 7,

    /**
     * @brief Maximum enum value for iteration
     */
    Max
};

/**
 * @brief Retirement cancellation reasons
 */
enum class RetirementCancelReason : uint8
{
    /**
     * @brief No cancellation (still in queue)
     */
    None = 0,

    /**
     * @brief Bot joined a guild
     */
    JoinedGuild = 1,

    /**
     * @brief Player added bot to friend list
     */
    AddedToFriendList = 2,

    /**
     * @brief Player grouped with bot
     */
    GroupedWithPlayer = 3,

    /**
     * @brief Player interacted with bot (trade, whisper, etc.)
     */
    PlayerInteraction = 4,

    /**
     * @brief Admin manually protected the bot
     */
    AdminProtected = 5,

    /**
     * @brief Bot received mail from player
     */
    ReceivedMail = 6,

    /**
     * @brief Bot participated in auction
     */
    AuctionActivity = 7,

    /**
     * @brief System error or shutdown
     */
    SystemError = 8,

    /**
     * @brief Maximum enum value for iteration
     */
    Max
};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * @brief Get string representation of retirement state
 * @param state The state to convert
 * @return Human-readable state name
 */
inline std::string RetirementStateToString(RetirementState state)
{
    switch (state)
    {
        case RetirementState::None:       return "None";
        case RetirementState::Pending:    return "Pending";
        case RetirementState::Cooling:    return "Cooling";
        case RetirementState::Preparing:  return "Preparing";
        case RetirementState::Exiting:    return "Exiting";
        case RetirementState::Cancelled:  return "Cancelled";
        case RetirementState::Completed:  return "Completed";
        default:                          return "Unknown";
    }
}

/**
 * @brief Get string representation of graceful exit stage
 * @param stage The stage to convert
 * @return Human-readable stage name
 */
inline std::string GracefulExitStageToString(GracefulExitStage stage)
{
    switch (stage)
    {
        case GracefulExitStage::None:               return "None";
        case GracefulExitStage::LeavingGuild:       return "LeavingGuild";
        case GracefulExitStage::ClearingMail:       return "ClearingMail";
        case GracefulExitStage::CancellingAuctions: return "CancellingAuctions";
        case GracefulExitStage::SavingState:        return "SavingState";
        case GracefulExitStage::LoggingOut:         return "LoggingOut";
        case GracefulExitStage::DeletingCharacter:  return "DeletingCharacter";
        case GracefulExitStage::Complete:           return "Complete";
        default:                                    return "Unknown";
    }
}

/**
 * @brief Get string representation of cancel reason
 * @param reason The reason to convert
 * @return Human-readable reason
 */
inline std::string RetirementCancelReasonToString(RetirementCancelReason reason)
{
    switch (reason)
    {
        case RetirementCancelReason::None:              return "None";
        case RetirementCancelReason::JoinedGuild:       return "Joined Guild";
        case RetirementCancelReason::AddedToFriendList: return "Added to Friend List";
        case RetirementCancelReason::GroupedWithPlayer: return "Grouped with Player";
        case RetirementCancelReason::PlayerInteraction: return "Player Interaction";
        case RetirementCancelReason::AdminProtected:    return "Admin Protected";
        case RetirementCancelReason::ReceivedMail:      return "Received Mail";
        case RetirementCancelReason::AuctionActivity:   return "Auction Activity";
        case RetirementCancelReason::SystemError:       return "System Error";
        default:                                        return "Unknown";
    }
}

/**
 * @brief Check if state allows cancellation
 * @param state The state to check
 * @return true if the bot can still be rescued
 */
inline bool CanCancelRetirement(RetirementState state)
{
    switch (state)
    {
        case RetirementState::Pending:
        case RetirementState::Cooling:
            return true;
        default:
            return false;
    }
}

/**
 * @brief Check if state is a terminal state
 * @param state The state to check
 * @return true if no further transitions possible
 */
inline bool IsTerminalState(RetirementState state)
{
    switch (state)
    {
        case RetirementState::Cancelled:
        case RetirementState::Completed:
            return true;
        default:
            return false;
    }
}

/**
 * @brief Check if state is an active retirement state
 * @param state The state to check
 * @return true if bot is in retirement process
 */
inline bool IsInRetirement(RetirementState state)
{
    switch (state)
    {
        case RetirementState::Pending:
        case RetirementState::Cooling:
        case RetirementState::Preparing:
        case RetirementState::Exiting:
            return true;
        default:
            return false;
    }
}

/**
 * @brief Get next graceful exit stage
 * @param current Current stage
 * @return Next stage in sequence
 */
inline GracefulExitStage GetNextExitStage(GracefulExitStage current)
{
    if (current >= GracefulExitStage::Complete)
        return GracefulExitStage::Complete;

    return static_cast<GracefulExitStage>(static_cast<uint8>(current) + 1);
}

/**
 * @brief Calculate estimated time for graceful exit stage
 * @param stage The stage to estimate
 * @return Estimated milliseconds for stage completion
 */
inline uint32 EstimateStageTime(GracefulExitStage stage)
{
    switch (stage)
    {
        case GracefulExitStage::LeavingGuild:       return 1000;   // 1 second
        case GracefulExitStage::ClearingMail:       return 5000;   // 5 seconds
        case GracefulExitStage::CancellingAuctions: return 3000;   // 3 seconds
        case GracefulExitStage::SavingState:        return 2000;   // 2 seconds
        case GracefulExitStage::LoggingOut:         return 1000;   // 1 second
        case GracefulExitStage::DeletingCharacter:  return 2000;   // 2 seconds
        default:                                    return 0;
    }
}

/**
 * @brief Get total estimated graceful exit time
 * @return Total milliseconds for full graceful exit
 */
inline uint32 GetTotalGracefulExitTime()
{
    uint32 total = 0;
    for (uint8 i = static_cast<uint8>(GracefulExitStage::LeavingGuild);
         i < static_cast<uint8>(GracefulExitStage::Complete); ++i)
    {
        total += EstimateStageTime(static_cast<GracefulExitStage>(i));
    }
    return total;
}

} // namespace Playerbot
