/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * @file GracefulExitHandler.h
 * @brief Handles graceful exit process for retiring bots
 *
 * Manages the multi-stage process of gracefully removing a bot from the game:
 * 1. Leave guild (transfer leadership if needed)
 * 2. Clear mail (return items, delete text-only)
 * 3. Cancel auctions (return items)
 * 4. Save final state
 * 5. Logout session
 * 6. Delete character
 *
 * Thread Safety:
 * - All public methods are thread-safe
 * - Callbacks are executed on the world thread
 * - Database operations are async where possible
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "RetirementState.h"
#include "RetirementCandidate.h"
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

class Player;
class Guild;
class Mail;

namespace Playerbot
{

// Forward declarations
class BotSession;

/**
 * @brief Result of a graceful exit stage operation
 */
struct StageResult
{
    /**
     * @brief Whether the stage completed successfully
     */
    bool success = false;

    /**
     * @brief Whether to advance to next stage
     */
    bool advance = false;

    /**
     * @brief Whether to retry the current stage
     */
    bool retry = false;

    /**
     * @brief Error message if failed
     */
    std::string errorMessage;

    /**
     * @brief Number of items affected (mail, auctions, etc.)
     */
    uint32 itemsAffected = 0;

    /**
     * @brief Create success result
     */
    static StageResult Success(uint32 items = 0)
    {
        StageResult result;
        result.success = true;
        result.advance = true;
        result.itemsAffected = items;
        return result;
    }

    /**
     * @brief Create retry result
     */
    static StageResult Retry(std::string const& error)
    {
        StageResult result;
        result.success = false;
        result.retry = true;
        result.errorMessage = error;
        return result;
    }

    /**
     * @brief Create failure result (skip stage)
     */
    static StageResult Fail(std::string const& error)
    {
        StageResult result;
        result.success = false;
        result.advance = true;  // Skip to next stage
        result.errorMessage = error;
        return result;
    }

    /**
     * @brief Create not-needed result (nothing to do)
     */
    static StageResult NotNeeded()
    {
        StageResult result;
        result.success = true;
        result.advance = true;
        return result;
    }
};

/**
 * @brief Callback for stage completion
 */
using StageCallback = std::function<void(ObjectGuid botGuid, StageResult const& result)>;

/**
 * @brief Configuration for graceful exit behavior
 */
struct GracefulExitConfig
{
    // Stage timeouts (milliseconds)
    uint32 guildLeaveTimeoutMs = 5000;
    uint32 mailClearTimeoutMs = 30000;
    uint32 auctionCancelTimeoutMs = 15000;
    uint32 saveStateTimeoutMs = 10000;
    uint32 logoutTimeoutMs = 5000;
    uint32 deleteTimeoutMs = 10000;

    // Retry settings
    uint8 maxRetries = 3;
    uint32 retryDelayMs = 1000;

    // Mail handling
    bool returnMailWithItems = true;
    bool deleteTextOnlyMail = true;

    // Auction handling
    bool cancelActiveAuctions = true;
    bool waitForAuctionMail = false;  // Wait for auction return mail

    // Guild handling
    bool gracefulGuildLeave = true;
    bool transferGuildLeadershipFirst = true;

    // Character deletion
    bool permanentDelete = true;
    bool archiveCharacterFirst = false;  // Archive to backup table

    // Logging
    bool verboseLogging = true;
};

/**
 * @brief Handler for graceful bot exit process
 *
 * Singleton class managing all graceful exit operations.
 * Each stage is executed asynchronously with timeout protection.
 */
class TC_GAME_API GracefulExitHandler
{
public:
    /**
     * @brief Get singleton instance
     */
    static GracefulExitHandler* Instance();

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /**
     * @brief Initialize the handler
     * @return true if successful
     */
    bool Initialize();

    /**
     * @brief Shutdown and cleanup
     */
    void Shutdown();

    /**
     * @brief Load configuration
     */
    void LoadConfig();

    // ========================================================================
    // STAGE EXECUTION
    // ========================================================================

    /**
     * @brief Execute the current stage for a candidate
     * @param candidate The retirement candidate
     * @param callback Called when stage completes
     * @return true if stage started, false if error
     */
    bool ExecuteStage(RetirementCandidate& candidate, StageCallback callback);

    /**
     * @brief Execute a specific stage
     * @param botGuid Bot to process
     * @param stage Stage to execute
     * @param callback Called when stage completes
     * @return true if stage started
     */
    bool ExecuteStage(ObjectGuid botGuid, GracefulExitStage stage, StageCallback callback);

    /**
     * @brief Check if a stage is in progress for a bot
     * @param botGuid Bot to check
     * @return true if stage is being processed
     */
    bool IsStageInProgress(ObjectGuid botGuid) const;

    /**
     * @brief Cancel any in-progress stage for a bot
     * @param botGuid Bot to cancel
     */
    void CancelStage(ObjectGuid botGuid);

    // ========================================================================
    // INDIVIDUAL STAGE HANDLERS
    // ========================================================================

    /**
     * @brief Stage 1: Leave guild
     * @param botGuid Bot to process
     * @return Stage result
     */
    StageResult HandleLeaveGuild(ObjectGuid botGuid);

    /**
     * @brief Stage 2: Clear mail
     * @param botGuid Bot to process
     * @return Stage result
     */
    StageResult HandleClearMail(ObjectGuid botGuid);

    /**
     * @brief Stage 3: Cancel auctions
     * @param botGuid Bot to process
     * @return Stage result
     */
    StageResult HandleCancelAuctions(ObjectGuid botGuid);

    /**
     * @brief Stage 4: Save state
     * @param botGuid Bot to process
     * @return Stage result
     */
    StageResult HandleSaveState(ObjectGuid botGuid);

    /**
     * @brief Stage 5: Logout bot
     * @param botGuid Bot to process
     * @return Stage result
     */
    StageResult HandleLogout(ObjectGuid botGuid);

    /**
     * @brief Stage 6: Delete character
     * @param botGuid Bot to process
     * @return Stage result
     */
    StageResult HandleDeleteCharacter(ObjectGuid botGuid);

    // ========================================================================
    // UTILITY METHODS
    // ========================================================================

    /**
     * @brief Get bot's Player object (may be nullptr if not online)
     * @param botGuid Bot to find
     * @return Player pointer or nullptr
     */
    Player* GetBotPlayer(ObjectGuid botGuid) const;

    /**
     * @brief Check if bot is currently logged in
     * @param botGuid Bot to check
     * @return true if bot is logged in
     */
    bool IsBotLoggedIn(ObjectGuid botGuid) const;

    /**
     * @brief Check if bot has pending mail
     * @param botGuid Bot to check
     * @return Number of pending mail items
     */
    uint32 GetPendingMailCount(ObjectGuid botGuid) const;

    /**
     * @brief Check if bot has active auctions
     * @param botGuid Bot to check
     * @return Number of active auctions
     */
    uint32 GetActiveAuctionCount(ObjectGuid botGuid) const;

    /**
     * @brief Check if bot is in a guild
     * @param botGuid Bot to check
     * @return Guild GUID or empty
     */
    ObjectGuid GetBotGuild(ObjectGuid botGuid) const;

    /**
     * @brief Check if bot is guild leader
     * @param botGuid Bot to check
     * @return true if guild master
     */
    bool IsBotGuildLeader(ObjectGuid botGuid) const;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /**
     * @brief Get current configuration
     */
    GracefulExitConfig const& GetConfig() const { return _config; }

    /**
     * @brief Set configuration
     */
    void SetConfig(GracefulExitConfig const& config);

    // ========================================================================
    // STATISTICS
    // ========================================================================

    /**
     * @brief Get number of in-progress exits
     */
    uint32 GetInProgressCount() const;

    /**
     * @brief Get total stages processed
     */
    uint64 GetTotalStagesProcessed() const { return _totalStagesProcessed; }

    /**
     * @brief Get total stages failed
     */
    uint64 GetTotalStagesFailed() const { return _totalStagesFailed; }

private:
    GracefulExitHandler() = default;
    ~GracefulExitHandler() = default;
    GracefulExitHandler(GracefulExitHandler const&) = delete;
    GracefulExitHandler& operator=(GracefulExitHandler const&) = delete;

    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /**
     * @brief Execute stage dispatch
     */
    StageResult DispatchStage(ObjectGuid botGuid, GracefulExitStage stage);

    /**
     * @brief Transfer guild leadership to another member
     */
    bool TransferGuildLeadership(Player* bot);

    /**
     * @brief Clear a single mail
     */
    bool ClearMail(Player* bot, Mail* mail);

    /**
     * @brief Cancel a single auction
     */
    bool CancelAuction(ObjectGuid botGuid, uint32 auctionId);

    /**
     * @brief Archive character to backup table
     */
    bool ArchiveCharacter(ObjectGuid botGuid);

    /**
     * @brief Delete character from database
     */
    bool DeleteCharacterFromDB(ObjectGuid botGuid);

    /**
     * @brief Log stage result
     */
    void LogStageResult(ObjectGuid botGuid, GracefulExitStage stage, StageResult const& result);

    // ========================================================================
    // DATA MEMBERS
    // ========================================================================

    // Configuration
    GracefulExitConfig _config;

    // In-progress stage tracking
    struct InProgressStage
    {
        ObjectGuid botGuid;
        GracefulExitStage stage;
        StageCallback callback;
        std::chrono::steady_clock::time_point startTime;
    };
    std::unordered_map<ObjectGuid, InProgressStage> _inProgressStages;
    mutable std::mutex _stageMutex;

    // Statistics
    std::atomic<uint64> _totalStagesProcessed{0};
    std::atomic<uint64> _totalStagesFailed{0};

    // Initialization state
    std::atomic<bool> _initialized{false};
};

} // namespace Playerbot

#define sGracefulExitHandler Playerbot::GracefulExitHandler::Instance()
