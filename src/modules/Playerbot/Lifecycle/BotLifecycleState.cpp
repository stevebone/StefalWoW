/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * BOT LIFECYCLE STATE MACHINE - Implementation
 *
 * This file implements the enterprise-grade bot initialization system
 * with the Two-Phase AddToWorld pattern.
 *
 * Key guarantees:
 * - Thread-safe state transitions with atomic operations
 * - Deferred event queue processed in order when ACTIVE
 * - Comprehensive timing metrics for performance analysis
 * - State history for debugging initialization issues
 */

#include "BotLifecycleState.h"
#include "Log.h"
#include "ObjectGuid.h"
#include <algorithm>
#include <sstream>

namespace Playerbot
{

// ============================================================================
// BotInitStateManager Implementation
// ============================================================================

BotInitStateManager::BotInitStateManager(ObjectGuid botGuid)
    : _botGuid(botGuid)
    , _state(BotInitState::CREATED)
    , _createdTime(std::chrono::steady_clock::now())
    , _stateChangeTime(_createdTime)
    , _dbLoadStartTime()
    , _dbLoadEndTime()
    , _managerInitStartTime()
    , _managerInitEndTime()
    , _activeTime()
{
    TC_LOG_DEBUG("module.playerbot.lifecycle", "BotInitStateManager created for bot {} in state CREATED",
        _botGuid.ToString());

    // Record initial state in history
    RecordTransition(BotInitState::CREATED, BotInitState::CREATED);
}

BotInitStateManager::~BotInitStateManager()
{
    // Clean up any remaining deferred events
    std::lock_guard<std::mutex> lock(_eventQueueMutex);
    size_t remainingEvents = _deferredEvents.size();

    if (remainingEvents > 0)
    {
        TC_LOG_WARN("module.playerbot.lifecycle",
            "BotInitStateManager destroyed for bot {} with {} unprocessed deferred events",
            _botGuid.ToString(), remainingEvents);

        // Clear the queue
        std::queue<DeferredEvent> empty;
        std::swap(_deferredEvents, empty);
    }

    TC_LOG_DEBUG("module.playerbot.lifecycle", "BotInitStateManager destroyed for bot {} (final state: {})",
        _botGuid.ToString(), std::string(BotInitStateToString(_state.load())));
}

// ============================================================================
// State Transitions
// ============================================================================

bool BotInitStateManager::IsValidTransition(BotInitState from, BotInitState to) const
{
    // Always allow transition to FAILED
    if (to == BotInitState::FAILED)
        return true;

    // Define valid transitions
    switch (from)
    {
        case BotInitState::CREATED:
            return to == BotInitState::LOADING_DB;

        case BotInitState::LOADING_DB:
            return to == BotInitState::INITIALIZING_MANAGERS;

        case BotInitState::INITIALIZING_MANAGERS:
            return to == BotInitState::READY;

        case BotInitState::READY:
            return to == BotInitState::ACTIVE;

        case BotInitState::ACTIVE:
            return to == BotInitState::REMOVING;

        case BotInitState::REMOVING:
            return to == BotInitState::DESTROYED;

        case BotInitState::DESTROYED:
            return false; // Terminal state

        case BotInitState::FAILED:
            return to == BotInitState::DESTROYED; // Can only go to DESTROYED from FAILED

        default:
            return false;
    }
}

void BotInitStateManager::RecordTransition(BotInitState from, BotInitState to)
{
    std::lock_guard<std::mutex> lock(_historyMutex);

    StateTransitionRecord record;
    record.from = from;
    record.to = to;
    record.timestamp = std::chrono::steady_clock::now();

    _stateHistory.push_back(record);

    // Trim history if too large
    if (_stateHistory.size() > MAX_HISTORY_SIZE)
    {
        _stateHistory.erase(_stateHistory.begin());
    }
}

bool BotInitStateManager::TransitionTo(BotInitState newState)
{
    BotInitState currentState = _state.load(std::memory_order_acquire);

    // Check if transition is valid
    if (!IsValidTransition(currentState, newState))
    {
        TC_LOG_ERROR("module.playerbot.lifecycle",
            "Invalid state transition for bot {}: {} -> {} (rejected)",
            _botGuid.ToString(),
            std::string(BotInitStateToString(currentState)),
            std::string(BotInitStateToString(newState)));
        return false;
    }

    // Attempt atomic state change
    if (!_state.compare_exchange_strong(currentState, newState,
        std::memory_order_acq_rel, std::memory_order_acquire))
    {
        // State changed between load and compare_exchange, try again
        TC_LOG_WARN("module.playerbot.lifecycle",
            "State changed during transition attempt for bot {}, current: {}, wanted: {}",
            _botGuid.ToString(),
            std::string(BotInitStateToString(currentState)),
            std::string(BotInitStateToString(newState)));
        return false;
    }

    // Update timing
    auto now = std::chrono::steady_clock::now();
    _stateChangeTime = now;

    // Record transition in history
    RecordTransition(currentState, newState);

    TC_LOG_DEBUG("module.playerbot.lifecycle",
        "Bot {} transitioned: {} -> {} (time in previous state: {}ms)",
        _botGuid.ToString(),
        std::string(BotInitStateToString(currentState)),
        std::string(BotInitStateToString(newState)),
        GetTimeInCurrentState().count());

    return true;
}

bool BotInitStateManager::StartDatabaseLoading()
{
    if (!TransitionTo(BotInitState::LOADING_DB))
        return false;

    _dbLoadStartTime = std::chrono::steady_clock::now();

    TC_LOG_DEBUG("module.playerbot.lifecycle",
        "Bot {} started database loading",
        _botGuid.ToString());

    return true;
}

bool BotInitStateManager::StartManagerInitialization()
{
    if (!TransitionTo(BotInitState::INITIALIZING_MANAGERS))
        return false;

    _dbLoadEndTime = std::chrono::steady_clock::now();
    _managerInitStartTime = _dbLoadEndTime;

    auto dbLoadDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        _dbLoadEndTime - _dbLoadStartTime);

    TC_LOG_DEBUG("module.playerbot.lifecycle",
        "Bot {} completed database loading ({}ms), starting manager initialization",
        _botGuid.ToString(), dbLoadDuration.count());

    return true;
}

bool BotInitStateManager::MarkReady()
{
    if (!TransitionTo(BotInitState::READY))
        return false;

    _managerInitEndTime = std::chrono::steady_clock::now();

    auto managerInitDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        _managerInitEndTime - _managerInitStartTime);

    TC_LOG_INFO("module.playerbot.lifecycle",
        "Bot {} is READY for AddToWorld() (DB load: {}ms, Manager init: {}ms, queued events: {})",
        _botGuid.ToString(),
        std::chrono::duration_cast<std::chrono::milliseconds>(_dbLoadEndTime - _dbLoadStartTime).count(),
        managerInitDuration.count(),
        GetQueuedEventCount());

    return true;
}

bool BotInitStateManager::MarkActive()
{
    if (!TransitionTo(BotInitState::ACTIVE))
        return false;

    _activeTime = std::chrono::steady_clock::now();

    auto totalInitTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        _activeTime - _createdTime);

    size_t queuedEvents = GetQueuedEventCount();

    TC_LOG_INFO("module.playerbot.lifecycle",
        "Bot {} is now ACTIVE (total init time: {}ms, deferred events to process: {})",
        _botGuid.ToString(), totalInitTime.count(), queuedEvents);

    return true;
}

bool BotInitStateManager::StartRemoval()
{
    if (!TransitionTo(BotInitState::REMOVING))
        return false;

    TC_LOG_DEBUG("module.playerbot.lifecycle",
        "Bot {} starting removal from world",
        _botGuid.ToString());

    return true;
}

bool BotInitStateManager::MarkDestroyed()
{
    if (!TransitionTo(BotInitState::DESTROYED))
        return false;

    auto metrics = GetMetrics();

    TC_LOG_INFO("module.playerbot.lifecycle",
        "Bot {} destroyed. Lifetime metrics - DB: {}ms, Managers: {}ms, Total: {}ms, Events: {}",
        _botGuid.ToString(),
        metrics.dbLoadTime.count(),
        metrics.managerInitTime.count(),
        metrics.totalTime.count(),
        metrics.queuedEventCount);

    return true;
}

void BotInitStateManager::MarkFailed(std::string_view reason)
{
    BotInitState previousState = _state.exchange(BotInitState::FAILED, std::memory_order_acq_rel);

    _failureReason = std::string(reason);

    RecordTransition(previousState, BotInitState::FAILED);

    TC_LOG_ERROR("module.playerbot.lifecycle",
        "Bot {} FAILED during state {}: {}",
        _botGuid.ToString(),
        std::string(BotInitStateToString(previousState)),
        _failureReason);
}

// ============================================================================
// Deferred Event Queue
// ============================================================================

bool BotInitStateManager::QueueEvent(DeferredEvent event)
{
    BotInitState currentState = GetState();

    // If already ACTIVE, don't queue - process immediately
    if (currentState == BotInitState::ACTIVE)
    {
        return false; // Indicates caller should process immediately
    }

    // If in error or terminal states, don't queue
    if (currentState == BotInitState::FAILED ||
        currentState == BotInitState::DESTROYED ||
        currentState == BotInitState::REMOVING)
    {
        TC_LOG_WARN("module.playerbot.lifecycle",
            "Bot {} cannot queue event in state {} - discarding",
            _botGuid.ToString(),
            std::string(BotInitStateToString(currentState)));
        return true; // Don't process immediately either
    }

    // Queue the event
    {
        std::lock_guard<std::mutex> lock(_eventQueueMutex);
        _deferredEvents.push(std::move(event));
    }

    TC_LOG_TRACE("module.playerbot.lifecycle",
        "Bot {} queued deferred event (type: {}, total queued: {})",
        _botGuid.ToString(),
        static_cast<uint8>(event.type),
        GetQueuedEventCount());

    return true;
}

bool BotInitStateManager::QueueCallback(std::function<void()> callback)
{
    DeferredEvent event(DeferredEventType::CUSTOM);
    event.callback = std::move(callback);
    return QueueEvent(std::move(event));
}

size_t BotInitStateManager::GetQueuedEventCount() const
{
    std::lock_guard<std::mutex> lock(_eventQueueMutex);
    return _deferredEvents.size();
}

uint32 BotInitStateManager::ProcessQueuedEvents(std::function<void(const DeferredEvent&)> handler)
{
    if (GetState() != BotInitState::ACTIVE)
    {
        TC_LOG_WARN("module.playerbot.lifecycle",
            "Bot {} attempted to process events but is not ACTIVE (state: {})",
            _botGuid.ToString(),
            std::string(BotInitStateToString(GetState())));
        return 0;
    }

    uint32 processedCount = 0;

    // Process all queued events
    std::queue<DeferredEvent> eventsToProcess;
    {
        std::lock_guard<std::mutex> lock(_eventQueueMutex);
        std::swap(eventsToProcess, _deferredEvents);
    }

    auto processStartTime = std::chrono::steady_clock::now();

    while (!eventsToProcess.empty())
    {
        const DeferredEvent& event = eventsToProcess.front();

        try
        {
            // If it's a custom callback, execute it
            if (event.type == DeferredEventType::CUSTOM && event.callback)
            {
                event.callback();
            }
            else
            {
                // Otherwise call the handler
                handler(event);
            }

            ++processedCount;
        }
        catch (const std::exception& e)
        {
            TC_LOG_ERROR("module.playerbot.lifecycle",
                "Bot {} exception processing deferred event type {}: {}",
                _botGuid.ToString(),
                static_cast<uint8>(event.type),
                e.what());
        }

        eventsToProcess.pop();
    }

    auto processEndTime = std::chrono::steady_clock::now();
    auto processDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        processEndTime - processStartTime);

    if (processedCount > 0)
    {
        TC_LOG_DEBUG("module.playerbot.lifecycle",
            "Bot {} processed {} deferred events in {}ms",
            _botGuid.ToString(), processedCount, processDuration.count());
    }

    return processedCount;
}

// ============================================================================
// Diagnostics & Metrics
// ============================================================================

std::chrono::milliseconds BotInitStateManager::GetTimeInCurrentState() const
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - _stateChangeTime);
}

std::chrono::milliseconds BotInitStateManager::GetTotalInitializationTime() const
{
    BotInitState currentState = GetState();

    if (currentState >= BotInitState::ACTIVE)
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            _activeTime - _createdTime);
    }

    // Still initializing
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - _createdTime);
}

BotInitStateManager::InitializationMetrics BotInitStateManager::GetMetrics() const
{
    InitializationMetrics metrics;

    BotInitState currentState = GetState();

    // Calculate DB load time
    if (_dbLoadEndTime > _dbLoadStartTime)
    {
        metrics.dbLoadTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            _dbLoadEndTime - _dbLoadStartTime);
    }

    // Calculate manager init time
    if (_managerInitEndTime > _managerInitStartTime)
    {
        metrics.managerInitTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            _managerInitEndTime - _managerInitStartTime);
    }

    // Calculate AddToWorld time (from READY to ACTIVE)
    if (currentState >= BotInitState::ACTIVE && _activeTime > _managerInitEndTime)
    {
        metrics.addToWorldTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            _activeTime - _managerInitEndTime);
    }

    // Total time
    metrics.totalTime = GetTotalInitializationTime();

    // Queued events
    metrics.queuedEventCount = static_cast<uint32>(GetQueuedEventCount());

    // Success status
    metrics.succeeded = (currentState >= BotInitState::ACTIVE &&
                         currentState != BotInitState::FAILED);

    // Failure reason if applicable
    if (currentState == BotInitState::FAILED)
    {
        metrics.failureReason = _failureReason;
    }

    return metrics;
}

std::string BotInitStateManager::GetStateHistory() const
{
    std::lock_guard<std::mutex> lock(_historyMutex);

    std::ostringstream oss;
    oss << "State history for bot " << _botGuid.ToString() << ":\n";

    for (size_t i = 0; i < _stateHistory.size(); ++i)
    {
        const auto& record = _stateHistory[i];
        auto timeSinceCreate = std::chrono::duration_cast<std::chrono::milliseconds>(
            record.timestamp - _createdTime);

        oss << "  [" << i << "] +" << timeSinceCreate.count() << "ms: "
            << std::string(BotInitStateToString(record.from))
            << " -> "
            << std::string(BotInitStateToString(record.to))
            << "\n";
    }

    return oss.str();
}

// ============================================================================
// BotInitGuard Implementation
// ============================================================================

BotInitGuard::BotInitGuard(const BotInitStateManager* manager, BotInitState state)
    : _manager(manager)
    , _state(state)
    , _valid(Playerbot::IsPlayerDataSafe(state))
{
}

std::unique_ptr<BotInitGuard> BotInitGuard::TryCreate(const BotInitStateManager* manager)
{
    if (!manager)
    {
        return nullptr;
    }

    BotInitState state = manager->GetState();

    if (!Playerbot::IsPlayerDataSafe(state))
    {
        TC_LOG_TRACE("module.playerbot.lifecycle",
            "BotInitGuard::TryCreate failed for bot {} (state: {})",
            manager->GetBotGuid().ToString(),
            std::string(BotInitStateToString(state)));
        return nullptr;
    }

    // Use a raw pointer and wrap it since constructor is private
    return std::unique_ptr<BotInitGuard>(new BotInitGuard(manager, state));
}

} // namespace Playerbot
