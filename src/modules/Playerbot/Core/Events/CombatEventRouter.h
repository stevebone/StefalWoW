/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 */

#ifndef TRINITYCORE_COMBAT_EVENT_ROUTER_H
#define TRINITYCORE_COMBAT_EVENT_ROUTER_H

#include "CombatEventType.h"
#include "CombatEvent.h"
#include "ICombatEventSubscriber.h"
#include <vector>
#include <unordered_map>
#include <shared_mutex>
#include <queue>
#include <atomic>
#include <mutex>

namespace Playerbot {

/**
 * @class CombatEventRouter
 * @brief Singleton event dispatcher for combat events
 *
 * Phase 3 Architecture: Central hub for combat event distribution
 *
 * Features:
 * - Bitmask-based subscriber filtering
 * - Priority-ordered event delivery
 * - Immediate dispatch (Dispatch) for time-critical events
 * - Queued dispatch (QueueEvent) for batched processing
 * - Thread-safe subscription management
 * - Performance statistics
 *
 * Dispatch Modes:
 * 1. **Dispatch()** - Immediate delivery, call from main thread only
 *    Use for: SPELL_CAST_START (needs immediate interrupt response)
 *
 * 2. **QueueEvent()** - Thread-safe queuing, processed on main thread
 *    Use for: Most events (damage, healing, auras, etc.)
 *
 * Usage:
 * @code
 * // Subscribe (in component initialization)
 * CombatEventRouter::Instance().Subscribe(this);
 *
 * // Dispatch event (in TrinityCore hooks)
 * auto event = CombatEvent::CreateSpellCastStart(caster, spell);
 * CombatEventRouter::Instance().Dispatch(event);  // Immediate
 *
 * auto damageEvent = CombatEvent::CreateDamageTaken(victim, attacker, 100);
 * CombatEventRouter::Instance().QueueEvent(damageEvent);  // Queued
 *
 * // Process queue (in World::Update)
 * CombatEventRouter::Instance().ProcessQueuedEvents();
 *
 * // Unsubscribe (in component shutdown)
 * CombatEventRouter::Instance().Unsubscribe(this);
 * @endcode
 *
 * Performance:
 * - O(1) event type filtering (bitmask)
 * - O(n) subscriber iteration (sorted by priority)
 * - Lock-free statistics
 * - ~0.01ms per event dispatch (typical)
 */
class CombatEventRouter {
public:
    /**
     * @brief Get singleton instance
     *
     * Thread-safe Meyer's singleton
     */
    static CombatEventRouter& Instance();

    // Prevent copying
    CombatEventRouter(const CombatEventRouter&) = delete;
    CombatEventRouter& operator=(const CombatEventRouter&) = delete;

    // ====================================================================
    // SUBSCRIPTION MANAGEMENT
    // ====================================================================

    /**
     * @brief Subscribe to events using subscriber's GetSubscribedEventTypes()
     *
     * @param subscriber Subscriber to register (must not be null)
     *
     * Thread Safety: Yes (mutex-protected)
     */
    void Subscribe(ICombatEventSubscriber* subscriber);

    /**
     * @brief Subscribe to specific event types
     *
     * @param subscriber Subscriber to register
     * @param eventTypes Bitmask of event types to subscribe to
     *
     * Thread Safety: Yes (mutex-protected)
     */
    void Subscribe(ICombatEventSubscriber* subscriber, CombatEventType eventTypes);

    /**
     * @brief Unsubscribe from all events
     *
     * @param subscriber Subscriber to unregister
     *
     * Thread Safety: Yes (mutex-protected)
     */
    void Unsubscribe(ICombatEventSubscriber* subscriber);

    /**
     * @brief Unsubscribe all subscribers
     *
     * Thread Safety: Yes (mutex-protected)
     */
    void UnsubscribeAll();

    // ====================================================================
    // EVENT DISPATCH
    // ====================================================================

    /**
     * @brief Dispatch event immediately (synchronous)
     *
     * @param event Event to dispatch
     *
     * IMPORTANT: Call from main thread only!
     *
     * Use for time-critical events like SPELL_CAST_START
     * where immediate response is required for interrupts.
     *
     * Thread Safety: No (main thread only)
     */
    void Dispatch(const CombatEvent& event);

    /**
     * @brief Queue event for later processing (async)
     *
     * @param event Event to queue
     *
     * Thread-safe. Events are processed in ProcessQueuedEvents().
     *
     * Thread Safety: Yes (mutex-protected queue)
     */
    void QueueEvent(CombatEvent event);

    /**
     * @brief Process all queued events
     *
     * Call from World::Update() on main thread.
     * Processes all events queued since last call.
     *
     * Thread Safety: Should be called from main thread only
     */
    void ProcessQueuedEvents();

    /**
     * @brief Dispatch event asynchronously (alias for QueueEvent)
     *
     * @param event Event to queue
     */
    void DispatchAsync(CombatEvent event);

    // ====================================================================
    // STATISTICS
    // ====================================================================

    /**
     * @brief Get total events dispatched (immediate + queued)
     */
    uint64 GetTotalEventsDispatched() const { return _totalEventsDispatched.load(); }

    /**
     * @brief Get total events queued
     */
    uint64 GetTotalEventsQueued() const { return _totalEventsQueued.load(); }

    /**
     * @brief Get current subscriber count
     */
    size_t GetSubscriberCount() const;

    /**
     * @brief Get current queue size
     */
    size_t GetQueueSize() const;

    /**
     * @brief Get events dispatched per type
     */
    uint64 GetEventsDispatchedByType(CombatEventType type) const;

    // ====================================================================
    // CONFIGURATION
    // ====================================================================

    /**
     * @brief Set maximum queue size
     *
     * @param maxSize Maximum events in queue
     *
     * Default: 10000
     */
    void SetMaxQueueSize(size_t maxSize) { _maxQueueSize = maxSize; }

    /**
     * @brief Set overflow behavior
     *
     * @param drop true = drop oldest, false = drop newest
     */
    void SetDropOldestOnOverflow(bool drop) { _dropOldestOnOverflow = drop; }

    /**
     * @brief Enable/disable event logging
     */
    void SetLoggingEnabled(bool enabled) { _loggingEnabled = enabled; }

    /**
     * @brief Check if initialized
     */
    bool IsInitialized() const { return _initialized; }

    /**
     * @brief Initialize router (called on module load)
     */
    void Initialize();

    /**
     * @brief Shutdown router (called on module unload)
     */
    void Shutdown();

private:
    CombatEventRouter();
    ~CombatEventRouter();

    // ====================================================================
    // INTERNAL
    // ====================================================================

    /**
     * @brief Dispatch to all matching subscribers
     */
    void DispatchToSubscribers(const CombatEvent& event);

    /**
     * @brief Get subscribers for event type
     */
    ::std::vector<ICombatEventSubscriber*> GetSubscribersForEvent(CombatEventType type);

    /**
     * @brief Sort subscribers by priority
     */
    void SortSubscribers();

    // ====================================================================
    // SUBSCRIBER STORAGE
    // ====================================================================

    // Per-type subscriber lists (for fast lookup)
    ::std::unordered_map<CombatEventType, ::std::vector<ICombatEventSubscriber*>> _subscribers;

    // All subscribers (for quick unsubscribe and iteration)
    ::std::vector<ICombatEventSubscriber*> _allSubscribers;

    // Subscriber subscription masks (for validation)
    ::std::unordered_map<ICombatEventSubscriber*, CombatEventType> _subscriberMasks;

    // Thread safety for subscriptions
    mutable ::std::shared_mutex _subscriberMutex;

    // ====================================================================
    // EVENT QUEUE
    // ====================================================================

    ::std::queue<CombatEvent> _eventQueue;
    mutable ::std::mutex _queueMutex;

    // ====================================================================
    // STATISTICS (Lock-Free)
    // ====================================================================
    // PERFORMANCE FIX: Changed from mutex-protected map to lock-free array
    // The event types are bitmasks, so we use bit position as array index.
    // Max bit position is 30 (BOSS_PHASE_CHANGED = 0x40000000), so 32 slots suffice.

    ::std::atomic<uint64> _totalEventsDispatched{0};
    ::std::atomic<uint64> _totalEventsQueued{0};
    ::std::atomic<uint64> _totalEventsDropped{0};

    // Lock-free per-type counters - index is bit position of event type
    // Uses relaxed memory order for performance (stats don't need strict ordering)
    static constexpr size_t MAX_EVENT_TYPE_BITS = 32;
    ::std::array<::std::atomic<uint64>, MAX_EVENT_TYPE_BITS> _eventsByTypeLockFree{};

    // Helper to get bit position (index) from event type bitmask
    static inline uint32 GetEventTypeBitIndex(CombatEventType type) {
        uint32 val = static_cast<uint32>(type);
        if (val == 0) return 0;
        // Count trailing zeros to get bit position
        uint32 index = 0;
        while ((val & 1) == 0) { val >>= 1; ++index; }
        return index;
    }

    // ====================================================================
    // CONFIGURATION
    // ====================================================================

    size_t _maxQueueSize = 10000;
    bool _dropOldestOnOverflow = true;
    bool _loggingEnabled = false;
    bool _initialized = false;
};

// ====================================================================
// CONVENIENCE MACROS FOR TRINITYCORE HOOKS
// ====================================================================

/**
 * @brief Dispatch combat event immediately
 *
 * Use for SPELL_CAST_START and other time-critical events
 */
#define DISPATCH_COMBAT_EVENT(event) \
    Playerbot::CombatEventRouter::Instance().Dispatch(event)

/**
 * @brief Queue combat event for later processing
 *
 * Use for most events (damage, healing, auras, etc.)
 */
#define QUEUE_COMBAT_EVENT(event) \
    Playerbot::CombatEventRouter::Instance().QueueEvent(event)

/**
 * @brief Check if combat event router is initialized
 */
#define COMBAT_EVENTS_ENABLED() \
    Playerbot::CombatEventRouter::Instance().IsInitialized()

} // namespace Playerbot

#endif // TRINITYCORE_COMBAT_EVENT_ROUTER_H
