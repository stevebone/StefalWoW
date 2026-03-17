/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * @file BotCloneEngine.h
 * @brief Fast bot creation engine using template cloning
 *
 * The BotCloneEngine provides high-performance bot creation by:
 * 1. Cloning from pre-serialized templates
 * 2. Parallel batch creation
 * 3. Async creation with callbacks
 * 4. Name generation with uniqueness guarantees
 * 5. Account pool management
 *
 * Creation Flow:
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │                       BOT CLONE ENGINE                                   │
 * ├─────────────────────────────────────────────────────────────────────────┤
 * │                                                                         │
 * │   ┌─────────────────┐                                                   │
 * │   │ Clone Request   │                                                   │
 * │   │ - Template      │                                                   │
 * │   │ - Target level  │                                                   │
 * │   │ - Faction       │                                                   │
 * │   │ - Gear score    │                                                   │
 * │   └────────┬────────┘                                                   │
 * │            │                                                            │
 * │            ▼                                                            │
 * │   ┌─────────────────┐    ┌──────────────┐    ┌──────────────┐          │
 * │   │ Allocate GUID   │───▶│ Alloc Account │───▶│ Generate Name│          │
 * │   └─────────────────┘    └──────────────┘    └──────────────┘          │
 * │                                   │                                     │
 * │            ┌──────────────────────┘                                     │
 * │            │                                                            │
 * │            ▼                                                            │
 * │   ┌─────────────────┐    ┌──────────────┐    ┌──────────────┐          │
 * │   │ Create Player   │───▶│ Apply Template│───▶│ Scale Gear   │          │
 * │   └─────────────────┘    └──────────────┘    └──────────────┘          │
 * │                                   │                                     │
 * │            ┌──────────────────────┘                                     │
 * │            │                                                            │
 * │            ▼                                                            │
 * │   ┌─────────────────┐    ┌──────────────┐    ┌──────────────┐          │
 * │   │ Apply Talents   │───▶│ Setup Actions│───▶│ Save to DB   │          │
 * │   └─────────────────┘    └──────────────┘    └──────────────┘          │
 * │                                   │                                     │
 * │            ┌──────────────────────┘                                     │
 * │            │                                                            │
 * │            ▼                                                            │
 * │   ┌─────────────────┐                                                   │
 * │   │  CloneResult    │                                                   │
 * │   │ - Bot GUID      │                                                   │
 * │   │ - Creation time │                                                   │
 * │   │ - Success/fail  │                                                   │
 * │   └─────────────────┘                                                   │
 * │                                                                         │
 * └─────────────────────────────────────────────────────────────────────────┘
 *
 * Thread Safety:
 * - All public methods are thread-safe
 * - Async operations use worker thread pool
 * - Name generation is thread-safe with atomic counter
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "PoolSlotState.h"
#include "BotTemplateRepository.h"
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace Playerbot
{

// ============================================================================
// CLONE RESULT
// ============================================================================

/**
 * @brief Result of a bot clone operation
 */
struct TC_GAME_API CloneResult
{
    bool success = false;                   ///< Whether creation succeeded
    ObjectGuid botGuid;                     ///< Created bot's GUID
    uint32 accountId = 0;                   ///< Account ID used
    std::string botName;                    ///< Generated bot name
    uint8 race = 0;                         ///< Selected race
    uint8 playerClass = 0;                  ///< Class ID
    uint32 specId = 0;                      ///< Specialization ID
    BotRole role = BotRole::DPS;            ///< Combat role
    Faction faction = Faction::Alliance;    ///< Faction
    uint32 level = 80;                      ///< Character level
    uint32 gearScore = 0;                   ///< Final gear score
    std::chrono::milliseconds creationTime{0};  ///< Time taken to create
    std::string errorMessage;               ///< Error message if failed

    /**
     * @brief Check if result is valid
     */
    bool IsValid() const { return success && botGuid != ObjectGuid::Empty; }

    /**
     * @brief Get string representation for logging
     */
    std::string ToString() const;
};

// ============================================================================
// BATCH CLONE REQUEST
// ============================================================================

/**
 * @brief Request for batch bot creation
 */
struct BatchCloneRequest
{
    BotRole role = BotRole::DPS;            ///< Required role
    uint32 count = 1;                       ///< Number of bots to create
    uint32 targetLevel = 80;                ///< Target level
    Faction faction = Faction::Alliance;    ///< Target faction
    uint32 minGearScore = 0;                ///< Minimum gear score
    uint8 preferredClass = 0;               ///< Preferred class (0 = any)
    bool async = false;                     ///< Use async creation

    // Post-creation queue configuration
    uint32 dungeonIdToQueue = 0;            ///< If > 0, queue bot for this dungeon after login
    uint32 battlegroundIdToQueue = 0;       ///< If > 0, queue bot for this BG after login
    uint32 arenaTypeToQueue = 0;            ///< If > 0, queue bot for this arena type after login

    /**
     * @brief Validate request parameters
     */
    bool IsValid() const { return count > 0 && count <= 100; }
};

// ============================================================================
// BOT CLONE ENGINE
// ============================================================================

/**
 * @brief High-performance bot creation engine using template cloning
 *
 * Singleton class that handles fast bot creation by cloning from
 * pre-serialized templates.
 */
class TC_GAME_API BotCloneEngine
{
public:
    /**
     * @brief Get singleton instance
     */
    static BotCloneEngine* Instance();

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /**
     * @brief Initialize the engine
     * @return true if successful
     */
    bool Initialize();

    /**
     * @brief Shutdown and cleanup
     */
    void Shutdown();

    /**
     * @brief Update (process async queue)
     * @param diff Time since last update
     */
    void Update(uint32 diff);

    // ========================================================================
    // SYNCHRONOUS CLONING
    // ========================================================================

    /**
     * @brief Clone a bot from template (synchronous)
     * @param tmpl Template to clone from
     * @param targetLevel Target character level
     * @param faction Target faction
     * @param targetGearScore Target gear score (0 = template default)
     * @return Clone result
     */
    CloneResult CloneFromTemplate(
        BotTemplate const* tmpl,
        uint32 targetLevel,
        Faction faction,
        uint32 targetGearScore = 0);

    /**
     * @brief Clone bot with auto-selected template
     * @param role Required role
     * @param faction Target faction
     * @param targetLevel Target level
     * @param targetGearScore Target gear score
     * @return Clone result
     */
    CloneResult Clone(
        BotRole role,
        Faction faction,
        uint32 targetLevel,
        uint32 targetGearScore = 0);

    /**
     * @brief Batch clone (synchronous)
     * @param request Batch request parameters
     * @return Vector of clone results
     */
    std::vector<CloneResult> BatchClone(BatchCloneRequest const& request);

    // ========================================================================
    // ASYNCHRONOUS CLONING
    // ========================================================================

    /// Callback for single clone completion
    using CloneCallback = std::function<void(CloneResult const&)>;

    /// Callback for batch clone completion
    using BatchCloneCallback = std::function<void(std::vector<CloneResult> const&)>;

    /**
     * @brief Clone asynchronously with callback
     * @param tmpl Template to clone from
     * @param targetLevel Target level
     * @param faction Target faction
     * @param callback Completion callback
     */
    void CloneAsync(
        BotTemplate const* tmpl,
        uint32 targetLevel,
        Faction faction,
        CloneCallback callback);

    /**
     * @brief Batch clone asynchronously
     * @param request Batch request parameters
     * @param callback Completion callback
     */
    void BatchCloneAsync(
        BatchCloneRequest const& request,
        BatchCloneCallback callback);

    // ========================================================================
    // QUERIES
    // ========================================================================

    /**
     * @brief Get estimated clone time for count
     * @param count Number of bots
     * @return Estimated time in milliseconds
     */
    std::chrono::milliseconds GetEstimatedCloneTime(uint32 count) const;

    /**
     * @brief Get number of pending async operations
     */
    uint32 GetPendingCloneCount() const;

    /**
     * @brief Get clones completed this hour
     */
    uint32 GetClonesThisHour() const;

    /**
     * @brief Check if engine is busy
     */
    bool IsBusy() const;

    // ========================================================================
    // STATISTICS
    // ========================================================================

    struct Statistics
    {
        uint32 totalClonesCreated = 0;
        uint32 clonesThisHour = 0;
        uint32 failedClonesThisHour = 0;
        std::chrono::milliseconds avgCreationTime{0};
        std::chrono::milliseconds peakCreationTime{0};
        uint32 pendingAsyncOperations = 0;
        uint32 accountPoolSize = 0;
        uint32 availableAccounts = 0;
    };

    /**
     * @brief Get current statistics
     */
    Statistics GetStatistics() const;

    /**
     * @brief Print statistics to log
     */
    void PrintStatistics() const;

private:
    BotCloneEngine() = default;
    ~BotCloneEngine() = default;
    BotCloneEngine(BotCloneEngine const&) = delete;
    BotCloneEngine& operator=(BotCloneEngine const&) = delete;

    // ========================================================================
    // INTERNAL METHODS - Resource Allocation
    // ========================================================================

    /**
     * @brief Allocate a new GUID for bot
     * @return New GUID
     */
    ObjectGuid AllocateGuid();

    /**
     * @brief Allocate an account for bot
     * @return Account ID (0 if failed)
     */
    uint32 AllocateAccount();

    /**
     * @brief Release an account back to pool
     * @param accountId Account to release
     */
    void ReleaseAccount(uint32 accountId);

    /**
     * @brief Generate unique bot name
     * @param race Race ID (affects naming style)
     * @param gender Gender (affects naming style)
     * @return Unique name
     */
    std::string GenerateUniqueName(uint8 race, uint8 gender);

    // ========================================================================
    // INTERNAL METHODS - Clone Execution
    // ========================================================================

    /**
     * @brief Execute the clone operation
     * @param tmpl Template to use
     * @param targetLevel Target level
     * @param faction Target faction
     * @param targetGearScore Target gear score
     * @return Clone result
     */
    CloneResult ExecuteClone(
        BotTemplate const* tmpl,
        uint32 targetLevel,
        Faction faction,
        uint32 targetGearScore,
        uint32 dungeonIdToQueue = 0,
        uint32 battlegroundIdToQueue = 0,
        uint32 arenaTypeToQueue = 0);

    /**
     * @brief Create the Player object
     * @param guid Player GUID
     * @param accountId Account ID
     * @param name Character name
     * @param race Race ID
     * @param playerClass Class ID
     * @param gender Gender (0=male, 1=female)
     * @param level Character level
     */
    bool CreatePlayerObject(
        ObjectGuid guid, uint32 accountId,
        std::string const& name,
        uint8 race, uint8 playerClass, uint8 gender, uint32 level);

    /**
     * @brief Perform a "fast login" for a newly created bot
     * @param botGuid The bot to login
     * @return true if successful
     */
    bool FastLogin(ObjectGuid botGuid);

    // ========================================================================
    // INTERNAL METHODS - Async Processing
    // ========================================================================

    /**
     * @brief Worker thread function
     */
    void AsyncWorkerThread();

    /**
     * @brief Process pending async queue
     */
    void ProcessAsyncQueue();

    // ========================================================================
    // INTERNAL METHODS - Name Generation
    // ========================================================================

    /**
     * @brief Load name pool from database
     */
    void LoadNamePool();

    /**
     * @brief Check if name is available
     */
    bool IsNameAvailable(std::string const& name) const;

    // ========================================================================
    // DATA MEMBERS - Async Tasks
    // ========================================================================

    struct AsyncCloneTask
    {
        BotTemplate const* tmpl;
        uint32 targetLevel;
        Faction faction;
        uint32 targetGearScore;
        CloneCallback callback;
    };

    struct AsyncBatchTask
    {
        BatchCloneRequest request;
        BatchCloneCallback callback;
    };

    std::queue<AsyncCloneTask> _asyncQueue;
    std::queue<AsyncBatchTask> _asyncBatchQueue;
    mutable std::mutex _asyncMutex;
    std::unique_ptr<std::thread> _workerThread;
    std::atomic<bool> _running{false};

    // ========================================================================
    // DATA MEMBERS - Name Generation
    // ========================================================================

    std::vector<std::string> _maleNames;
    std::vector<std::string> _femaleNames;
    std::atomic<uint32> _nameIndex{0};
    std::atomic<uint32> _nameSuffix{0};
    mutable std::mutex _nameMutex;

    // ========================================================================
    // DATA MEMBERS - Account Pool (Delegated to BotAccountMgr)
    // ========================================================================
    // NOTE: Account management is now handled by BotAccountMgr
    // AllocateAccount() uses sBotAccountMgr->AcquireAccount()
    // ReleaseAccount() uses sBotAccountMgr->ReleaseAccount()

    // ========================================================================
    // DATA MEMBERS - Statistics
    // ========================================================================

    std::atomic<uint32> _totalClonesCreated{0};
    std::atomic<uint32> _clonesThisHour{0};
    std::atomic<uint32> _failedClonesThisHour{0};
    std::chrono::milliseconds _avgCreationTime{0};
    std::chrono::milliseconds _peakCreationTime{0};
    uint32 _creationTimeSamples = 0;
    std::chrono::system_clock::time_point _hourStart;
    mutable std::mutex _statsMutex;

    // ========================================================================
    // DATA MEMBERS - State
    // ========================================================================

    std::atomic<bool> _initialized{false};
};

} // namespace Playerbot

#define sBotCloneEngine Playerbot::BotCloneEngine::Instance()
