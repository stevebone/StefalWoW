/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * ACTIVITY EXECUTOR
 *
 * Phase 3: Humanization Core - Activity Integration
 *
 * Bridges HumanizationManager's activity sessions with actual bot managers.
 * When an activity session starts, this class triggers the corresponding
 * manager to execute real behavior.
 */

#pragma once

#include "Define.h"
#include "ActivityType.h"
#include "ObjectGuid.h"
#include <memory>
#include <functional>
#include <unordered_map>

class Player;

namespace Playerbot
{

// Forward declarations
class BotAI;
class GatheringManager;
class AuctionManager;
class ProfessionManager;
class BankingManager;
class TradeManager;
class IGameSystemsManager;

namespace Humanization
{

/**
 * @brief Result of activity execution
 */
enum class ActivityExecutionResult : uint8
{
    SUCCESS = 0,            // Activity started successfully
    FAILED_NO_MANAGER,      // Manager not available
    FAILED_PRECONDITION,    // Preconditions not met (wrong location, etc.)
    FAILED_ALREADY_ACTIVE,  // Activity already running
    FAILED_DISABLED,        // Activity type disabled
    FAILED_COOLDOWN,        // Activity on cooldown
    NOT_IMPLEMENTED         // Activity not yet implemented
};

/**
 * @brief Activity execution context
 */
struct ActivityExecutionContext
{
    ActivityType activity;
    uint32 durationMs;
    bool interruptible;
    ObjectGuid targetGuid;      // Optional target (NPC, object, etc.)
    uint32 targetEntry;         // Optional target entry ID

    ActivityExecutionContext()
        : activity(ActivityType::NONE)
        , durationMs(0)
        , interruptible(true)
        , targetEntry(0)
    {}

    explicit ActivityExecutionContext(ActivityType type, uint32 duration = 0)
        : activity(type)
        , durationMs(duration)
        , interruptible(true)
        , targetEntry(0)
    {}
};

/**
 * @brief Activity Executor
 *
 * Bridges HumanizationManager's activity tracking with actual bot behavior.
 *
 * Responsibilities:
 * - Start/stop actual bot behaviors when activity sessions begin/end
 * - Check preconditions for activities (location, resources, cooldowns)
 * - Map activity types to their corresponding manager calls
 *
 * **Integration Points:**
 * - GatheringManager: Mining, Herbalism, Skinning, Fishing
 * - AuctionManager: Auction browsing/posting
 * - ProfessionManager: Crafting
 * - BankingManager: Bank visits
 * - TradeManager: Trading
 *
 * **Performance:**
 * - Start/Stop: <1ms
 * - Precondition checks: <0.5ms
 */
class TC_GAME_API ActivityExecutor
{
public:
    /**
     * @brief Construct activity executor for a bot
     * @param bot The player bot
     */
    explicit ActivityExecutor(Player* bot);

    ~ActivityExecutor();

    // Non-copyable
    ActivityExecutor(ActivityExecutor const&) = delete;
    ActivityExecutor& operator=(ActivityExecutor const&) = delete;

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /**
     * @brief Initialize executor
     */
    void Initialize();

    /**
     * @brief Shutdown and cleanup
     */
    void Shutdown();

    /**
     * @brief Is executor initialized?
     */
    bool IsInitialized() const { return _initialized; }

    // ========================================================================
    // ACTIVITY EXECUTION
    // ========================================================================

    /**
     * @brief Start executing an activity
     * @param context Execution context with activity details
     * @return Result of execution attempt
     */
    ActivityExecutionResult StartActivity(ActivityExecutionContext const& context);

    /**
     * @brief Stop currently executing activity
     * @param activity Activity type to stop (NONE = stop all)
     */
    void StopActivity(ActivityType activity = ActivityType::NONE);

    /**
     * @brief Check if activity can be executed
     * @param activity Activity to check
     * @return true if preconditions are met
     */
    bool CanExecuteActivity(ActivityType activity) const;

    /**
     * @brief Get current executing activity
     */
    ActivityType GetCurrentActivity() const { return _currentActivity; }

    /**
     * @brief Is any activity currently executing?
     */
    bool IsExecutingActivity() const { return _currentActivity != ActivityType::NONE; }

    // ========================================================================
    // PRECONDITION CHECKS
    // ========================================================================

    /**
     * @brief Check if bot is at required location for activity
     */
    bool IsAtRequiredLocation(ActivityType activity) const;

    /**
     * @brief Check if bot has required skill for activity
     */
    bool HasRequiredSkill(ActivityType activity) const;

    /**
     * @brief Check if activity is on cooldown
     */
    bool IsOnCooldown(ActivityType activity) const;

private:
    // ========================================================================
    // ACTIVITY HANDLERS
    // ========================================================================

    // Gathering activities
    ActivityExecutionResult StartMining();
    ActivityExecutionResult StartHerbalism();
    ActivityExecutionResult StartSkinning();
    ActivityExecutionResult StartFishing();

    // City life activities
    ActivityExecutionResult StartAuctionBrowsing();
    ActivityExecutionResult StartAuctionPosting();
    ActivityExecutionResult StartBankVisit();
    ActivityExecutionResult StartVendorVisit();
    ActivityExecutionResult StartTrainerVisit();
    ActivityExecutionResult StartInnRest();
    ActivityExecutionResult StartMailboxCheck();

    // Crafting activities
    ActivityExecutionResult StartCrafting();
    ActivityExecutionResult StartDisenchanting();

    // Maintenance activities
    ActivityExecutionResult StartRepairing();
    ActivityExecutionResult StartVendoring();

    // Idle activities
    ActivityExecutionResult StartIdleBehavior();

    // Stop handlers
    void StopGatheringActivities();
    void StopCityLifeActivities();
    void StopCraftingActivities();
    void StopAllActivities();

    // ========================================================================
    // HELPER METHODS
    // ========================================================================

    /**
     * @brief Get BotAI from player
     */
    BotAI* GetBotAI() const;

    /**
     * @brief Get GameSystemsManager from BotAI
     */
    IGameSystemsManager* GetGameSystems() const;

    /**
     * @brief Check if bot is near an NPC of specific type
     */
    bool IsNearNPCType(uint32 npcFlags, float range = 30.0f) const;

    /**
     * @brief Check if bot is in a city
     */
    bool IsInCity() const;

    /**
     * @brief Record cooldown for activity
     */
    void SetActivityCooldown(ActivityType activity, uint32 cooldownMs);

    // ========================================================================
    // DATA MEMBERS
    // ========================================================================

    Player* _bot;
    bool _initialized{false};
    ActivityType _currentActivity{ActivityType::NONE};

    // Cooldown tracking (activity type -> expiry time)
    std::unordered_map<uint8, uint32> _cooldowns;
};

} // namespace Humanization
} // namespace Playerbot
