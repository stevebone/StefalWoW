/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * BOT LIFECYCLE STATE MACHINE
 *
 * Enterprise-grade bot initialization system implementing Two-Phase AddToWorld pattern.
 *
 * PROBLEM SOLVED:
 * Previously, bot managers were initialized while Player data was incomplete, causing
 * crashes when managers tried to access GetName(), GetMaxPower(), GetGroup(), etc.
 * Game events (group changes, combat) would fire before managers were ready.
 *
 * SOLUTION:
 * Strict state machine ensuring AddToWorld() is only called AFTER full initialization:
 *
 *   CREATED → LOADING_DB → INITIALIZING_MANAGERS → READY → ACTIVE → REMOVING → DESTROYED
 *                                                    ↑
 *                                        AddToWorld() only allowed here
 *
 * KEY GUARANTEE:
 * - IsInWorld() = true  IMPLIES  State >= READY
 * - Managers can safely access Player data when State >= READY
 * - Events are queued until State >= ACTIVE
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "Log.h"
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <string_view>

namespace Playerbot
{

// Forward declarations
class BotAI;
class BotSession;

/**
 * @brief Bot lifecycle states with strict transition rules
 *
 * State Transitions:
 *   CREATED            → LOADING_DB             (LoginCharacter called)
 *   LOADING_DB         → INITIALIZING_MANAGERS  (DB queries complete)
 *   INITIALIZING_MANAGERS → READY               (All managers initialized)
 *   READY              → ACTIVE                 (AddToWorld completed, first UpdateAI)
 *   ACTIVE             → REMOVING               (RemoveFromWorld called)
 *   REMOVING           → DESTROYED              (Cleanup complete)
 *
 * Any state           → FAILED                  (On error)
 */
enum class BotInitState : uint8
{
    // Pre-initialization states (Player data NOT safe to access)
    CREATED = 0,              // BotSession created, Player object allocated but not loaded
    LOADING_DB = 1,           // Database queries executing (async or sync)
    INITIALIZING_MANAGERS = 2, // BotAI and managers being created

    // Transition state (Player data partially safe)
    READY = 3,                // All managers initialized, SAFE to AddToWorld()

    // Active states (Player data FULLY safe)
    ACTIVE = 4,               // In world, UpdateAI running, fully operational

    // Cleanup states
    REMOVING = 5,             // RemoveFromWorld in progress
    DESTROYED = 6,            // Fully cleaned up, awaiting deletion

    // Error state
    FAILED = 255              // Initialization failed, cannot proceed
};

/**
 * @brief Convert lifecycle state to string for logging
 */
constexpr std::string_view BotInitStateToString(BotInitState state) noexcept
{
    switch (state)
    {
        case BotInitState::CREATED:              return "CREATED";
        case BotInitState::LOADING_DB:           return "LOADING_DB";
        case BotInitState::INITIALIZING_MANAGERS: return "INITIALIZING_MANAGERS";
        case BotInitState::READY:                return "READY";
        case BotInitState::ACTIVE:               return "ACTIVE";
        case BotInitState::REMOVING:             return "REMOVING";
        case BotInitState::DESTROYED:            return "DESTROYED";
        case BotInitState::FAILED:               return "FAILED";
        default:                                       return "UNKNOWN";
    }
}

/**
 * @brief Check if state allows player data access
 */
constexpr bool IsPlayerDataSafe(BotInitState state) noexcept
{
    return state >= BotInitState::READY && state <= BotInitState::ACTIVE;
}

/**
 * @brief Check if state allows manager operations
 */
constexpr bool AreManagersOperational(BotInitState state) noexcept
{
    return state == BotInitState::ACTIVE;
}

/**
 * @brief Check if state allows adding to world
 */
constexpr bool CanAddToWorld(BotInitState state) noexcept
{
    return state == BotInitState::READY;
}

/**
 * @brief Check if bot is fully operational
 */
constexpr bool IsFullyOperational(BotInitState state) noexcept
{
    return state == BotInitState::ACTIVE;
}

/**
 * @brief Thread-safe lifecycle state manager with deferred event queue
 *
 * This class manages:
 * 1. State transitions with validation
 * 2. Deferred event queue for pre-ACTIVE events
 * 3. Performance metrics for initialization
 * 4. Comprehensive logging for debugging
 */
class TC_GAME_API BotInitStateManager
{
public:
    // Event types that can be deferred
    enum class DeferredEventType : uint8
    {
        GROUP_JOINED,
        GROUP_LEFT,
        COMBAT_START,
        COMBAT_END,
        DEATH,
        RESPAWN,
        SPELL_CAST,
        AURA_APPLIED,
        AURA_REMOVED,
        TARGET_CHANGED,
        POSITION_CHANGED,
        CUSTOM
    };

    // Deferred event structure
    struct DeferredEvent
    {
        DeferredEventType type;
        ObjectGuid sourceGuid;
        ObjectGuid targetGuid;
        uint32 spellId;
        uint32 auraId;
        std::chrono::steady_clock::time_point timestamp;
        std::function<void()> callback;  // For custom events

        DeferredEvent(DeferredEventType t = DeferredEventType::CUSTOM)
            : type(t), spellId(0), auraId(0), timestamp(std::chrono::steady_clock::now()) {}
    };

    explicit BotInitStateManager(ObjectGuid botGuid);
    ~BotInitStateManager();

    // Non-copyable, non-movable
    BotInitStateManager(const BotInitStateManager&) = delete;
    BotInitStateManager& operator=(const BotInitStateManager&) = delete;
    BotInitStateManager(BotInitStateManager&&) = delete;
    BotInitStateManager& operator=(BotInitStateManager&&) = delete;

    // ========================================================================
    // STATE ACCESS
    // ========================================================================

    /**
     * @brief Get current lifecycle state (thread-safe)
     */
    BotInitState GetState() const noexcept
    {
        return _state.load(std::memory_order_acquire);
    }

    /**
     * @brief Get bot GUID
     */
    ObjectGuid GetBotGuid() const noexcept { return _botGuid; }

    /**
     * @brief Check if player data is safe to access
     */
    bool IsPlayerDataSafe() const noexcept
    {
        return Playerbot::IsPlayerDataSafe(GetState());
    }

    /**
     * @brief Check if managers are operational
     */
    bool AreManagersOperational() const noexcept
    {
        return Playerbot::AreManagersOperational(GetState());
    }

    /**
     * @brief Check if bot is fully operational
     */
    bool IsFullyOperational() const noexcept
    {
        return Playerbot::IsFullyOperational(GetState());
    }

    // ========================================================================
    // STATE TRANSITIONS
    // ========================================================================

    /**
     * @brief Transition to new state with validation
     * @return true if transition succeeded
     */
    bool TransitionTo(BotInitState newState);

    /**
     * @brief Transition to LOADING_DB state (called when DB loading starts)
     */
    bool StartDatabaseLoading();

    /**
     * @brief Transition to INITIALIZING_MANAGERS state (called when DB loading completes)
     */
    bool StartManagerInitialization();

    /**
     * @brief Transition to READY state (called when all managers are initialized)
     * This is the point where AddToWorld() becomes safe
     */
    bool MarkReady();

    /**
     * @brief Transition to ACTIVE state (called after first successful UpdateAI)
     * This flushes all deferred events
     */
    bool MarkActive();

    /**
     * @brief Transition to REMOVING state (called when RemoveFromWorld starts)
     */
    bool StartRemoval();

    /**
     * @brief Transition to DESTROYED state (called when cleanup completes)
     */
    bool MarkDestroyed();

    /**
     * @brief Transition to FAILED state (called on any error)
     */
    void MarkFailed(std::string_view reason);

    // ========================================================================
    // DEFERRED EVENT QUEUE
    // ========================================================================

    /**
     * @brief Queue an event to be processed when bot becomes ACTIVE
     * @return true if event was queued, false if bot is already ACTIVE (event should be processed immediately)
     */
    bool QueueEvent(DeferredEvent event);

    /**
     * @brief Queue a custom callback to be executed when bot becomes ACTIVE
     */
    bool QueueCallback(std::function<void()> callback);

    /**
     * @brief Get number of queued events
     */
    size_t GetQueuedEventCount() const;

    /**
     * @brief Process all queued events (called when transitioning to ACTIVE)
     * @param handler Function to process each event
     * @return Number of events processed
     */
    uint32 ProcessQueuedEvents(std::function<void(const DeferredEvent&)> handler);

    // ========================================================================
    // DIAGNOSTICS & METRICS
    // ========================================================================

    /**
     * @brief Get time spent in current state
     */
    std::chrono::milliseconds GetTimeInCurrentState() const;

    /**
     * @brief Get total initialization time (from CREATED to ACTIVE)
     */
    std::chrono::milliseconds GetTotalInitializationTime() const;

    /**
     * @brief Get time spent in each phase
     */
    struct InitializationMetrics
    {
        std::chrono::milliseconds dbLoadTime{0};
        std::chrono::milliseconds managerInitTime{0};
        std::chrono::milliseconds addToWorldTime{0};
        std::chrono::milliseconds totalTime{0};
        uint32 queuedEventCount{0};
        bool succeeded{false};
        std::string failureReason;
    };

    InitializationMetrics GetMetrics() const;

    /**
     * @brief Get state transition history for debugging
     */
    std::string GetStateHistory() const;

private:
    // Validate state transition
    bool IsValidTransition(BotInitState from, BotInitState to) const;

    // Record state transition for history
    void RecordTransition(BotInitState from, BotInitState to);

    // Bot identification
    ObjectGuid _botGuid;

    // Thread-safe state
    std::atomic<BotInitState> _state{BotInitState::CREATED};

    // Deferred event queue
    mutable std::mutex _eventQueueMutex;
    std::queue<DeferredEvent> _deferredEvents;

    // Timing metrics
    std::chrono::steady_clock::time_point _createdTime;
    std::chrono::steady_clock::time_point _stateChangeTime;
    std::chrono::steady_clock::time_point _dbLoadStartTime;
    std::chrono::steady_clock::time_point _dbLoadEndTime;
    std::chrono::steady_clock::time_point _managerInitStartTime;
    std::chrono::steady_clock::time_point _managerInitEndTime;
    std::chrono::steady_clock::time_point _activeTime;

    // State history for debugging (last 10 transitions)
    struct StateTransitionRecord
    {
        BotInitState from;
        BotInitState to;
        std::chrono::steady_clock::time_point timestamp;
    };
    mutable std::mutex _historyMutex;
    std::vector<StateTransitionRecord> _stateHistory;
    static constexpr size_t MAX_HISTORY_SIZE = 10;

    // Failure tracking
    std::string _failureReason;
};

/**
 * @brief RAII guard for safe player data access
 *
 * Usage:
 *   if (auto guard = BotInitGuard::TryCreate(lifecycleManager))
 *   {
 *       // Safe to access player->GetName(), GetMaxPower(), etc.
 *   }
 *   else
 *   {
 *       // Bot not ready, cannot access player data
 *   }
 */
class TC_GAME_API BotInitGuard
{
public:
    /**
     * @brief Try to create a guard for player data access
     * @return nullptr if bot is not in a safe state
     */
    static std::unique_ptr<BotInitGuard> TryCreate(const BotInitStateManager* manager);

    /**
     * @brief Check if guard is valid (player data safe)
     */
    explicit operator bool() const noexcept { return _valid; }

    /**
     * @brief Get the lifecycle state at guard creation
     */
    BotInitState GetState() const noexcept { return _state; }

private:
    explicit BotInitGuard(const BotInitStateManager* manager, BotInitState state);

    const BotInitStateManager* _manager;
    BotInitState _state;
    bool _valid;
};

/**
 * @brief Macro for safe player data access with early return
 *
 * Usage:
 *   BOT_INIT_CHECK(manager);
 *   // Only reaches here if bot is in safe state
 *   player->GetName(); // Safe
 */
#define BOT_INIT_CHECK(manager) \
    do { \
        if (!(manager) || !::Playerbot::IsPlayerDataSafe((manager)->GetState())) \
        { \
            TC_LOG_TRACE("module.playerbot.lifecycle", "Bot lifecycle check failed at {}:{}", __FILE__, __LINE__); \
            return; \
        } \
    } while (0)

/**
 * @brief Macro for safe player data access with custom return value
 */
#define BOT_INIT_CHECK_RETURN(manager, retval) \
    do { \
        if (!(manager) || !::Playerbot::IsPlayerDataSafe((manager)->GetState())) \
        { \
            TC_LOG_TRACE("module.playerbot.lifecycle", "Bot lifecycle check failed at {}:{}", __FILE__, __LINE__); \
            return (retval); \
        } \
    } while (0)

/**
 * @brief Macro for safe manager operations (requires ACTIVE state)
 */
#define BOT_INIT_MANAGER_CHECK(manager) \
    do { \
        if (!(manager) || !::Playerbot::AreManagersOperational((manager)->GetState())) \
        { \
            TC_LOG_TRACE("module.playerbot.lifecycle", "Bot manager check failed at {}:{}", __FILE__, __LINE__); \
            return; \
        } \
    } while (0)

} // namespace Playerbot
