/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * @file QueueShortageSubscriber.h
 * @brief EventBus subscriber for queue shortage events
 *
 * This singleton subscribes to queue shortage events from multiple sources:
 * - QueueStatePoller (periodic polling)
 * - PlayerbotBGScript (human player joins)
 * - Packet handlers (queue status updates)
 *
 * When a shortage is detected, it triggers JIT bot creation through the
 * JITBotFactory. This provides a unified handler for shortage events
 * regardless of their source.
 *
 * Architecture:
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │                    QUEUE SHORTAGE SUBSCRIBER                            │
 * ├─────────────────────────────────────────────────────────────────────────┤
 * │                                                                         │
 * │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐                 │
 * │  │QueueState    │  │PlayerbotBG   │  │ Packet       │                 │
 * │  │Poller        │  │Script        │  │ Handlers     │                 │
 * │  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘                 │
 * │         │                 │                 │                          │
 * │         │ BG_QUEUE_       │ BG_QUEUE_       │ BG_QUEUE_                │
 * │         │ SHORTAGE        │ SHORTAGE        │ UPDATE                   │
 * │         │                 │                 │                          │
 * │         ▼                 ▼                 ▼                          │
 * │  ┌──────────────────────────────────────────────────────────────────┐ │
 * │  │              EventDispatcher::Subscribe()                        │ │
 * │  └─────────────────────────────┬────────────────────────────────────┘ │
 * │                                │                                      │
 * │                                ▼                                      │
 * │  ┌──────────────────────────────────────────────────────────────────┐ │
 * │  │          QueueShortageSubscriber::OnEvent()                      │ │
 * │  │  - Deduplicates shortage events (throttling)                     │ │
 * │  │  - Validates shortage data                                       │ │
 * │  │  - Submits JIT requests to factory                               │ │
 * │  └─────────────────────────────┬────────────────────────────────────┘ │
 * │                                │                                      │
 * │                                ▼                                      │
 * │  ┌──────────────────────────────────────────────────────────────────┐ │
 * │  │              JITBotFactory::SubmitRequest()                      │ │
 * │  └──────────────────────────────────────────────────────────────────┘ │
 * │                                                                         │
 * └─────────────────────────────────────────────────────────────────────────┘
 */

#pragma once

#include "Define.h"
#include "Core/Events/QueueEventData.h"
#include "Core/Events/BotEventTypes.h"
#include "Threading/LockHierarchy.h"
#include <unordered_map>
#include <chrono>
#include <atomic>

namespace Playerbot
{

/**
 * @brief Subscribes to queue shortage events and triggers JIT bot creation
 *
 * This singleton listens for BG_QUEUE_SHORTAGE, LFG_QUEUE_SHORTAGE, and
 * ARENA_QUEUE_SHORTAGE events from the EventDispatcher and coordinates
 * JIT bot creation through JITBotFactory.
 *
 * Thread Safety:
 * - OnEvent() is thread-safe
 * - Uses OrderedRecursiveMutex for internal state protection
 * - Throttling uses atomic operations for performance
 */
class TC_GAME_API QueueShortageSubscriber
{
private:
    QueueShortageSubscriber();
    ~QueueShortageSubscriber();

public:
    /**
     * @brief Get singleton instance
     */
    static QueueShortageSubscriber* Instance();

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /**
     * @brief Initialize and subscribe to events
     */
    bool Initialize();

    /**
     * @brief Unsubscribe and cleanup
     */
    void Shutdown();

    /**
     * @brief Periodic update (process deferred work)
     */
    void Update(uint32 diff);

    /**
     * @brief Handle shortage events from EventDispatcher
     */
    void OnEvent(StateMachine::EventType type, Events::BotEvent const& event);

    // ========================================================================
    // SHORTAGE HANDLING
    // ========================================================================

    /**
     * @brief Handle BG queue shortage event
     * @param data Shortage event data
     */
    void HandleBGShortage(Events::QueueShortageEventData const* data);

    /**
     * @brief Handle LFG queue shortage event
     * @param data Shortage event data
     */
    void HandleLFGShortage(Events::QueueShortageEventData const* data);

    /**
     * @brief Handle Arena queue shortage event
     * @param data Shortage event data
     */
    void HandleArenaShortage(Events::QueueShortageEventData const* data);

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /**
     * @brief Set minimum time between handling same queue shortage
     * @param seconds Throttle time in seconds
     */
    void SetEventThrottleTime(uint32 seconds) { _eventThrottleSeconds = seconds; }

    /**
     * @brief Get current throttle time
     */
    uint32 GetEventThrottleTime() const { return _eventThrottleSeconds; }

    /**
     * @brief Enable or disable the subscriber
     */
    void SetEnabled(bool enable) { _enabled = enable; }

    /**
     * @brief Check if subscriber is enabled
     */
    bool IsEnabled() const { return _enabled; }

    // ========================================================================
    // STATISTICS
    // ========================================================================

    struct Statistics
    {
        uint32 eventsReceived;
        uint32 eventsProcessed;
        uint32 eventsThrottled;
        uint32 bgRequestsSubmitted;
        uint32 lfgRequestsSubmitted;
        uint32 arenaRequestsSubmitted;
    };

    /**
     * @brief Get current statistics
     */
    Statistics GetStatistics() const;

    /**
     * @brief Reset statistics
     */
    void ResetStatistics();

private:
    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /**
     * @brief Check if this queue shortage was handled recently
     * @param queueKey Unique key for the queue
     * @return true if should throttle (skip handling)
     */
    bool IsEventThrottled(uint64 queueKey) const;

    /**
     * @brief Record event handling time for throttling
     * @param queueKey Unique key for the queue
     */
    void RecordEventHandled(uint64 queueKey);

    /**
     * @brief Generate unique key for queue
     */
    static uint64 MakeQueueKey(Events::ContentType type, uint32 contentId, uint32 bracketId);

    /**
     * @brief Submit JIT factory request for BG
     */
    void SubmitBGRequest(Events::QueueShortageEventData const& data);

    /**
     * @brief Submit JIT factory request for LFG
     */
    void SubmitLFGRequest(Events::QueueShortageEventData const& data);

    /**
     * @brief Submit JIT factory request for Arena
     */
    void SubmitArenaRequest(Events::QueueShortageEventData const& data);

    // ========================================================================
    // DATA MEMBERS
    // ========================================================================

    mutable OrderedRecursiveMutex<LockOrder::QUEUE_MONITOR> _mutex;

    // Event throttling - last handled time per queue
    std::unordered_map<uint64, std::chrono::steady_clock::time_point> _lastEventTime;

    // Configuration
    std::atomic<bool> _enabled;
    uint32 _eventThrottleSeconds;

    // Statistics
    std::atomic<uint32> _eventsReceived;
    std::atomic<uint32> _eventsProcessed;
    std::atomic<uint32> _eventsThrottled;
    std::atomic<uint32> _bgRequestsSubmitted;
    std::atomic<uint32> _lfgRequestsSubmitted;
    std::atomic<uint32> _arenaRequestsSubmitted;

    // Constants
    static constexpr uint32 DEFAULT_EVENT_THROTTLE_SECONDS = 15;

    // Singleton
    QueueShortageSubscriber(QueueShortageSubscriber const&) = delete;
    QueueShortageSubscriber& operator=(QueueShortageSubscriber const&) = delete;
};

} // namespace Playerbot

#define sQueueShortageSubscriber Playerbot::QueueShortageSubscriber::Instance()
