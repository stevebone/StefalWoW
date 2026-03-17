/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This file is part of the TrinityCore Playerbot Module.
 *
 * == Bot Death Recovery System ==
 *
 * Handles all bot resurrection logic:
 * - Auto-release spirit after death
 * - Corpse run navigation and resurrection
 * - Spirit healer interaction and graveyard resurrection
 * - Decision algorithm: corpse vs spirit healer
 * - Resurrection sickness management
 *
 * DESIGN PRINCIPLES:
 * - Module-only implementation (no core modifications)
 * - Uses TrinityCore Player resurrection APIs
 * - Performance: <0.01% CPU per dead bot
 * - Memory: ~1KB per bot state
 * - Thread-safe for async operations
 */

#ifndef TRINITYCORE_PLAYERBOT_DEATH_RECOVERY_MANAGER_H
#define TRINITYCORE_PLAYERBOT_DEATH_RECOVERY_MANAGER_H

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "Position.h"
#include "ObjectGuid.h"
#include <chrono>
#include <memory>
#include <atomic>
#include <mutex>

class Player;
class Creature;
class WorldLocation;

namespace Playerbot
{

class BotAI;

/**
 * @enum DeathRecoveryState
 * @brief Current state of bot death recovery process
 */
enum class DeathRecoveryState : uint8
{
    NOT_DEAD = 0,           ///< Bot is alive
    JUST_DIED,              ///< Bot just died, waiting to release spirit
    RELEASING_SPIRIT,       ///< Releasing spirit (becoming ghost)
    PENDING_TELEPORT_ACK,   ///< Waiting to complete teleport ack (prevents spell mod crash)
    GHOST_DECIDING,         ///< Ghost state, deciding corpse vs spirit healer
    RUNNING_TO_CORPSE,      ///< Running back to corpse
    AT_CORPSE,              ///< At corpse location, ready to resurrect
    FINDING_SPIRIT_HEALER,  ///< Searching for spirit healer
    MOVING_TO_SPIRIT_HEALER,///< Moving to spirit healer
    AT_SPIRIT_HEALER,       ///< At spirit healer, initiating interaction
    RESURRECTING,           ///< Resurrection in progress
    RESURRECTION_FAILED     ///< Resurrection failed, retry needed
};

/**
 * @enum ResurrectionMethod
 * @brief Method chosen for resurrection
 */
enum class ResurrectionMethod : uint8
{
    UNDECIDED = 0,
    CORPSE_RUN,         ///< Run to corpse and resurrect there
    SPIRIT_HEALER,      ///< Use spirit healer at graveyard
    BATTLE_RESURRECTION,///< Receive battle rez from another player
    AUTO_RESURRECT      ///< Forced resurrection (special cases)
};

/**
 * @struct DeathRecoveryConfig
 * @brief Configuration for death recovery behavior
 */
struct DeathRecoveryConfig
{
    uint32 autoReleaseDelayMs = 5000;           ///< Delay before auto-releasing spirit (ms)
    bool preferCorpseRun = true;                 ///< Prefer corpse run over spirit healer
    float maxCorpseRunDistance = 200.0f;         ///< Max distance for corpse run (yards)
    bool autoSpiritHealer = true;                ///< Auto-use spirit healer if corpse too far
    bool allowBattleResurrection = true;         ///< Accept battle rez from others
    uint32 navigationUpdateInterval = 500;       ///< Navigation update frequency (ms)
    uint32 corpseDistanceCheckInterval = 1000;   ///< Corpse distance check frequency (ms)
    bool skipResurrectionSickness = false;       ///< Debug: skip rez sickness (for testing)
    float spiritHealerSearchRadius = 150.0f;     ///< Search radius for spirit healer (yards)
    uint32 resurrectionTimeout = 300000;         ///< Max time in death recovery (5 min)
    bool logDebugInfo = true;                    ///< Enable debug logging

    static DeathRecoveryConfig LoadFromConfig();
};

/**
 * @struct DeathRecoveryStatistics
 * @brief Statistics tracking for death recovery
 */
struct DeathRecoveryStatistics
{
    uint32 totalDeaths = 0;
    uint32 corpseResurrections = 0;
    uint32 spiritHealerResurrections = 0;
    uint32 battleResurrections = 0;
    uint32 failedResurrections = 0;
    uint64 totalRecoveryTimeMs = 0;
    uint64 averageRecoveryTimeMs = 0;
    uint32 resurrectionsWithSickness = 0;

    void RecordDeath();
    void RecordResurrection(ResurrectionMethod method, uint64 recoveryTimeMs, bool hadSickness);
    void RecordFailure();
    ::std::string ToString() const;
};

/**
 * @class DeathRecoveryManager
 * @brief Manages bot resurrection after death
 *
 * COMPLETE IMPLEMENTATION - NO SHORTCUTS
 *
 * Features:
 * - Automatic spirit release with configurable delay
 * - Intelligent corpse vs spirit healer decision
 * - Corpse navigation and interaction
 * - Spirit healer detection and interaction
 * - Resurrection sickness handling
 * - Battle resurrection acceptance
 * - Performance optimized (throttled updates)
 * - Thread-safe state management
 * - Comprehensive error handling
 * - Statistics tracking
 *
 * Usage:
 *   auto manager = std::make_unique<DeathRecoveryManager>(bot, botAI);
 *   manager->OnDeath();          // Called when bot dies
 *   manager->Update(diff);       // Called every AI update
 *   manager->OnResurrection();   // Called after resurrection
 */
class TC_GAME_API DeathRecoveryManager
{
public:
    /**
     * @brief Construct death recovery manager for a bot
     * @param bot The bot player
     * @param ai The bot's AI controller
     */
    DeathRecoveryManager(Player* bot, BotAI* ai);
    ~DeathRecoveryManager();

    // Prevent copying
    DeathRecoveryManager(DeathRecoveryManager const&) = delete;
    DeathRecoveryManager& operator=(DeathRecoveryManager const&) = delete;

    // ========================================================================
    // LIFECYCLE EVENTS
    // ========================================================================

    /**
     * @brief Called when bot dies
     * Initializes death recovery process
     */
    void OnDeath();

    /**
     * @brief Called when bot resurrects
     * Cleans up death recovery state
     */
    void OnResurrection();

    /**
     * @brief Update death recovery process
     * @param diff Time since last update in milliseconds
     *
     * Handles all death recovery logic:
     * - Spirit release timing
     * - Corpse distance monitoring
     * - Navigation to corpse/spirit healer
     * - Resurrection interaction
     */
    void Update(uint32 diff);

    /**
     * @brief Reset death recovery state
     * Clears all state and returns to NOT_DEAD
     */
    void Reset();

    // ========================================================================
    // STATE QUERIES
    // ========================================================================

    /**
     * @brief Check if bot is in death recovery process
     * @return True if bot is dead and recovering
     */
    bool IsInDeathRecovery() const { return m_state != DeathRecoveryState::NOT_DEAD; }

    /**
     * @brief Get current death recovery state
     * @return Current state
     */
    DeathRecoveryState GetState() const { return m_state; }

    /**
     * @brief Get chosen resurrection method
     * @return Resurrection method
     */
    ResurrectionMethod GetResurrectionMethod() const { return m_method; }

    /**
     * @brief Get time since death (milliseconds)
     * @return Time elapsed since death
     */
    uint64 GetTimeSinceDeath() const;

    /**
     * @brief Check if bot is currently a ghost
     * @return True if bot has ghost flag
     */
    bool IsGhost() const;

    /**
     * @brief Check if resurrection is in progress
     * @return True if resurrection cast/animation active
     */
    bool IsResurrecting() const { return m_state == DeathRecoveryState::RESURRECTING; }

    /**
     * @brief Check if there's a pending resurrection waiting for main thread
     * @return True if resurrection needs to be executed on main thread
     */
    bool HasPendingMainThreadResurrection() const { return _pendingMainThreadResurrection.load(::std::memory_order_acquire); }

    /**
     * @brief Execute pending resurrection on main thread
     * MUST be called from main thread only (e.g., from Map::Update path)
     * @return True if resurrection was executed
     */
    bool ExecutePendingMainThreadResurrection();

    // ========================================================================
    // CORPSE INFORMATION
    // ========================================================================

    /**
     * @brief Get corpse location
     * @return Corpse position, or empty if no corpse
     */
    WorldLocation GetCorpseLocation() const;

    /**
     * @brief Get distance to corpse
     * @return Distance in yards, -1.0f if no corpse
     */
    float GetCorpseDistance() const;

    /**
     * @brief Check if bot is within corpse resurrection range
     * @return True if close enough to resurrect at corpse (39 yards)
     */
    bool IsInCorpseRange() const;

    // ========================================================================
    // SPIRIT HEALER INFORMATION
    // ========================================================================

    /**
     * @brief Find nearest spirit healer
     * @return Spirit healer creature, nullptr if none found
     */
    Creature* FindNearestSpiritHealer() const;

    /**
     * @brief Get current spirit healer target
     * @return Spirit healer GUID, Empty if none
     */
    ObjectGuid GetSpiritHealerGuid() const { return m_spiritHealerGuid; }

    /**
     * @brief Check if spirit healer interaction is available
     * @return True if can interact with spirit healer
     */
    bool CanInteractWithSpiritHealer() const;

    // ========================================================================
    // RESURRECTION CONTROL
    // ========================================================================

    /**
     * @brief Manually trigger resurrection at corpse
     * @return True if resurrection initiated
     */
    bool TriggerCorpseResurrection();

    /**
     * @brief Manually trigger spirit healer resurrection
     * @return True if resurrection initiated
     */
    bool TriggerSpiritHealerResurrection();

    /**
     * @brief Accept battle resurrection from another player
     * @param casterGuid The player offering resurrection
     * @param spellId The resurrection spell ID
     * @return True if accepted
     */
    bool AcceptBattleResurrection(ObjectGuid casterGuid, uint32 spellId);

    /**
     * @brief Force immediate resurrection (admin/debug)
     * @param method Method to use
     * @return True if successful
     */
    bool ForceResurrection(ResurrectionMethod method);

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /**
     * @brief Set death recovery configuration
     * @param config New configuration
     */
    void SetConfig(DeathRecoveryConfig const& config);

    /**
     * @brief Get current configuration
     * @return Configuration reference
     */
    DeathRecoveryConfig const& GetConfig() const { return m_config; }

    /**
     * @brief Reload configuration from config file
     */
    void ReloadConfig();

    // ========================================================================
    // STATISTICS
    // ========================================================================

    /**
     * @brief Get death recovery statistics
     * @return Statistics reference
     */
    DeathRecoveryStatistics const& GetStatistics() const { return m_stats; }

    /**
     * @brief Reset statistics counters
     */
    void ResetStatistics();

    /**
     * @brief Log current statistics
     */
    void LogStatistics() const;

private:
    // ========================================================================
    // STATE MACHINE HANDLERS
    // ========================================================================

    void HandleJustDied(uint32 diff);
    void HandleReleasingSpirit(uint32 diff);
    void HandlePendingTeleportAck(uint32 diff);
    void HandleGhostDeciding(uint32 diff);
    void HandleRunningToCorpse(uint32 diff);
    void HandleAtCorpse(uint32 diff);
    void HandleFindingSpiritHealer(uint32 diff);
    void HandleMovingToSpiritHealer(uint32 diff);
    void HandleAtSpiritHealer(uint32 diff);
    void HandleResurrecting(uint32 diff);
    void HandleResurrectionFailed(uint32 diff);

    // ========================================================================
    // DECISION LOGIC
    // ========================================================================

    /**
     * @brief Decide resurrection method (corpse vs spirit healer)
     * Uses configuration and game state to make optimal decision
     */
    void DecideResurrectionMethod();

    /**
     * @brief Check if corpse run is viable
     * @return True if corpse is reachable and preferable
     */
    bool ShouldDoCorpseRun() const;

    /**
     * @brief Check if spirit healer should be used
     * @return True if spirit healer is better option
     */
    bool ShouldUseSpiritHealer() const;

    /**
     * @brief Check for special resurrection cases
     * @return True if special handling needed (battleground, arena, etc)
     */
    bool CheckSpecialResurrectionCases();

    // ========================================================================
    // RESURRECTION EXECUTION
    // ========================================================================

    /**
     * @brief Release spirit and become ghost
     * @return True if successful
     */
    bool ExecuteReleaseSpirit();

    /**
     * @brief Navigate to corpse location
     * @return True if navigation started
     */
    bool NavigateToCorpse();

    /**
     * @brief Interact with corpse to resurrect
     * @return True if resurrection initiated
     */
    bool InteractWithCorpse();

    /**
     * @brief Navigate to spirit healer
     * @return True if navigation started
     */
    bool NavigateToSpiritHealer();

    /**
     * @brief Interact with spirit healer (gossip)
     * @return True if interaction initiated
     */
    bool InteractWithSpiritHealer();

    /**
     * @brief Execute graveyard resurrection via TrinityCore API
     * @return True if successful
     */
    bool ExecuteGraveyardResurrection();

    // ========================================================================
    // VALIDATION & ERROR HANDLING
    // ========================================================================

    /**
     * @brief Validate bot and AI are valid
     * @return True if valid, logs error if not
     */
    bool ValidateBotState() const;

    /**
     * @brief Check if resurrection timed out
     * @return True if exceeded timeout
     */
    bool IsResurrectionTimedOut() const;

    /**
     * @brief Handle resurrection failure
     * @param reason Failure reason for logging
     */
    void HandleResurrectionFailure(::std::string const& reason);

    /**
     * @brief Transition to new state with logging
     * @param newState New state to transition to
     * @param reason Reason for transition
     */
    void TransitionToState(DeathRecoveryState newState, ::std::string const& reason);

    // ========================================================================
    // HELPER METHODS
    // ========================================================================

    /**
     * @brief Update corpse distance cache
     */
    void UpdateCorpseDistance() const;

    /**
     * @brief Check if bot is in instance/battleground
     * @return True if in special zone
     */
    bool IsInSpecialZone() const;

    /**
     * @brief Get resurrection sickness spell ID
     * @return Spell ID (15007)
     */
    uint32 GetResurrectionSicknessSpellId() const { return 15007; }

    /**
     * @brief Check if bot will get resurrection sickness
     * @return True if level > 10 and using spirit healer
     */
    bool WillReceiveResurrectionSickness() const;

    /**
     * @brief Log debug information
     * @param message Message to log
     */
    void LogDebug(::std::string const& message) const;

private:
    // Core references
    Player* m_bot;
    BotAI* m_ai;

    // State management
    ::std::atomic<DeathRecoveryState> m_state;
    ResurrectionMethod m_method;
    ::std::chrono::steady_clock::time_point m_deathTime;
    ::std::chrono::steady_clock::time_point m_lastStateTransition;

    // Corpse tracking
    WorldLocation m_corpseLocation;
    mutable float m_corpseDistance;
    mutable ::std::chrono::steady_clock::time_point m_lastCorpseDistanceCheck;

    // Spirit healer tracking
    ObjectGuid m_spiritHealerGuid;
    WorldLocation m_spiritHealerLocation;

    // Navigation tracking
    ::std::chrono::steady_clock::time_point m_lastNavigationUpdate;
    bool m_navigationActive;

    // Timers
    uint32 m_releaseTimer;                  ///< Time until spirit release
    uint32 m_stateTimer;                    ///< Generic state timer
    uint32 m_retryTimer;                    ///< Retry delay after failure
    uint32 m_retryCount;                    ///< Number of retry attempts

    // Configuration
    DeathRecoveryConfig m_config;

    // Statistics
    DeathRecoveryStatistics m_stats;

    // Thread safety
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_SPAWNER> m_mutex;

    // GHOST AURA FIX: Resurrection synchronization to prevent duplicate aura application
    mutable ::std::timed_mutex _resurrectionMutex;    ///< Prevents concurrent resurrection attempts with timeout support
    ::std::atomic<bool> _resurrectionInProgress{false};         ///< Resurrection is currently executing
    ::std::atomic<uint64> _lastResurrectionAttemptMs{0}; ///< Last resurrection attempt timestamp (for debouncing)

    // THREAD SAFETY FIX: Main thread resurrection execution
    // SendSpiritResurrect() modifies player state and MUST run on main thread
    // Worker threads set this flag, main thread executes the resurrection
    ::std::atomic<bool> _pendingMainThreadResurrection{false};

    // SPELL MOD CRASH FIX: Deferred teleport ack to prevent Spell.cpp:603 crash
    ::std::chrono::steady_clock::time_point m_teleportAckTime;  ///< Time when teleport ack was deferred
    bool m_needsTeleportAck;                                  ///< True if teleport ack needs to be processed

    // Constants
    static constexpr float CORPSE_RESURRECTION_RANGE = 39.0f;      ///< WoW corpse rez range
    static constexpr float SPIRIT_HEALER_INTERACTION_RANGE = 10.0f; ///< Spirit healer range
    static constexpr uint32 MAX_RETRY_ATTEMPTS = 5;                ///< Max resurrection retries
    static constexpr uint32 RETRY_DELAY_MS = 5000;                 ///< Delay between retries
    static constexpr uint32 RESURRECTION_DEBOUNCE_MS = 500;        ///< Minimum time between resurrection attempts
};

} // namespace Playerbot

#endif // TRINITYCORE_PLAYERBOT_DEATH_RECOVERY_MANAGER_H
