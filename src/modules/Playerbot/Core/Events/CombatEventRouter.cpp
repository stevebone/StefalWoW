/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 */

#include "CombatEventRouter.h"
#include "GameTime.h"
#include "Log.h"
#include <algorithm>
#include <array>

namespace Playerbot {

CombatEventRouter& CombatEventRouter::Instance() {
    static CombatEventRouter instance;
    return instance;
}

CombatEventRouter::CombatEventRouter() {
    TC_LOG_DEBUG("playerbot.events.combat", "CombatEventRouter: Constructing");
}

CombatEventRouter::~CombatEventRouter() {
    TC_LOG_DEBUG("playerbot.events.combat", "CombatEventRouter: Destructing - {} events dispatched, {} queued",
        _totalEventsDispatched.load(), _totalEventsQueued.load());
}

void CombatEventRouter::Initialize() {
    if (_initialized) {
        return;
    }

    TC_LOG_INFO("playerbot.events.combat", "CombatEventRouter: Initializing event-driven combat system");
    _initialized = true;
}

void CombatEventRouter::Shutdown() {
    if (!_initialized) {
        return;
    }

    TC_LOG_INFO("playerbot.events.combat", "CombatEventRouter: Shutting down - {} total events, {} subscribers",
        _totalEventsDispatched.load(), _allSubscribers.size());

    UnsubscribeAll();

    // Clear queue
    {
        ::std::lock_guard lock(_queueMutex);
        while (!_eventQueue.empty()) {
            _eventQueue.pop();
        }
    }

    _initialized = false;
}

void CombatEventRouter::Subscribe(ICombatEventSubscriber* subscriber) {
    if (!subscriber) {
        return;
    }
    Subscribe(subscriber, subscriber->GetSubscribedEventTypes());
}

void CombatEventRouter::Subscribe(ICombatEventSubscriber* subscriber, CombatEventType eventTypes) {
    if (!subscriber || eventTypes == CombatEventType::NONE) {
        return;
    }

    ::std::unique_lock lock(_subscriberMutex);

    // Check if already subscribed
    auto it = ::std::find(_allSubscribers.begin(), _allSubscribers.end(), subscriber);
    if (it != _allSubscribers.end()) {
        // Update subscription mask
        _subscriberMasks[subscriber] = _subscriberMasks[subscriber] | eventTypes;
    } else {
        // New subscriber
        _allSubscribers.push_back(subscriber);
        _subscriberMasks[subscriber] = eventTypes;
    }

    // Add to per-type lists
    // Iterate through all possible single-bit event types
    for (uint32 i = 0; i < 32; ++i) {
        CombatEventType eventType = static_cast<CombatEventType>(1u << i);
        if (HasFlag(eventTypes, eventType)) {
            auto& subs = _subscribers[eventType];
            if (::std::find(subs.begin(), subs.end(), subscriber) == subs.end()) {
                subs.push_back(subscriber);
                // Sort by priority (higher priority first)
                ::std::sort(subs.begin(), subs.end(),
                    [](ICombatEventSubscriber* a, ICombatEventSubscriber* b) {
                        return a->GetEventPriority() > b->GetEventPriority();
                    });
            }
        }
    }

    if (_loggingEnabled) {
        TC_LOG_DEBUG("playerbot.events.combat", "CombatEventRouter: Subscriber '{}' registered for types 0x{:08X}",
            subscriber->GetSubscriberName(), static_cast<uint32>(eventTypes));
    }
}

void CombatEventRouter::Unsubscribe(ICombatEventSubscriber* subscriber) {
    if (!subscriber) {
        return;
    }

    ::std::unique_lock lock(_subscriberMutex);

    // Remove from all per-type lists
    for (auto& [eventType, subs] : _subscribers) {
        subs.erase(::std::remove(subs.begin(), subs.end(), subscriber), subs.end());
    }

    // Remove from all subscribers list
    _allSubscribers.erase(
        ::std::remove(_allSubscribers.begin(), _allSubscribers.end(), subscriber),
        _allSubscribers.end());

    // Remove from mask tracking
    _subscriberMasks.erase(subscriber);

    if (_loggingEnabled) {
        TC_LOG_DEBUG("playerbot.events.combat", "CombatEventRouter: Subscriber '{}' unsubscribed",
            subscriber->GetSubscriberName());
    }
}

void CombatEventRouter::UnsubscribeAll() {
    ::std::unique_lock lock(_subscriberMutex);

    _subscribers.clear();
    _allSubscribers.clear();
    _subscriberMasks.clear();

    TC_LOG_DEBUG("playerbot.events.combat", "CombatEventRouter: All subscribers removed");
}

void CombatEventRouter::Dispatch(const CombatEvent& event) {
    if (event.type == CombatEventType::NONE) {
        return;
    }

    if (!_initialized) {
        return;
    }

    DispatchToSubscribers(event);
    ++_totalEventsDispatched;

    // PERFORMANCE FIX: Lock-free per-type statistics update
    // Uses relaxed memory order - stats don't need strict ordering
    uint32 bitIndex = GetEventTypeBitIndex(event.type);
    if (bitIndex < MAX_EVENT_TYPE_BITS) {
        _eventsByTypeLockFree[bitIndex].fetch_add(1, ::std::memory_order_relaxed);
    }

    if (_loggingEnabled) {
        TC_LOG_TRACE("playerbot.events.combat", "CombatEventRouter: Dispatched {} (source: {}, target: {})",
            CombatEventTypeToString(event.type),
            event.source.ToString(), event.target.ToString());
    }
}

void CombatEventRouter::QueueEvent(CombatEvent event) {
    if (event.type == CombatEventType::NONE) {
        return;
    }

    if (!_initialized) {
        return;
    }

    ::std::lock_guard lock(_queueMutex);

    if (_eventQueue.size() >= _maxQueueSize) {
        if (_dropOldestOnOverflow) {
            _eventQueue.pop();  // Drop oldest
            ++_totalEventsDropped;
        } else {
            ++_totalEventsDropped;
            return;  // Drop newest (don't queue)
        }
    }

    // Set timestamp if not already set
    if (event.timestamp == 0) {
        event.timestamp = GameTime::GetGameTimeMS();
    }

    _eventQueue.push(::std::move(event));
    ++_totalEventsQueued;
}

void CombatEventRouter::ProcessQueuedEvents() {
    if (!_initialized) {
        return;
    }

    // Swap queue to minimize lock time
    ::std::queue<CombatEvent> eventsToProcess;
    {
        ::std::lock_guard lock(_queueMutex);
        ::std::swap(eventsToProcess, _eventQueue);
    }

    // Process all events
    while (!eventsToProcess.empty()) {
        Dispatch(eventsToProcess.front());
        eventsToProcess.pop();
    }
}

void CombatEventRouter::DispatchAsync(CombatEvent event) {
    QueueEvent(::std::move(event));
}

void CombatEventRouter::DispatchToSubscribers(const CombatEvent& event) {
    ::std::shared_lock lock(_subscriberMutex);

    // Find subscribers for this event type
    auto it = _subscribers.find(event.type);
    if (it == _subscribers.end()) {
        return;
    }

    // Dispatch to each subscriber (already sorted by priority)
    for (ICombatEventSubscriber* subscriber : it->second) {
        if (subscriber && subscriber->ShouldReceiveEvent(event)) {
            try {
                subscriber->OnCombatEvent(event);
            } catch (const ::std::exception& e) {
                TC_LOG_ERROR("playerbot.events.combat",
                    "CombatEventRouter: Exception in subscriber '{}' handling {}: {}",
                    subscriber->GetSubscriberName(),
                    CombatEventTypeToString(event.type), e.what());
            }
        }
    }
}

::std::vector<ICombatEventSubscriber*> CombatEventRouter::GetSubscribersForEvent(CombatEventType type) {
    ::std::shared_lock lock(_subscriberMutex);

    auto it = _subscribers.find(type);
    if (it != _subscribers.end()) {
        return it->second;
    }
    return {};
}

size_t CombatEventRouter::GetSubscriberCount() const {
    ::std::shared_lock lock(_subscriberMutex);
    return _allSubscribers.size();
}

size_t CombatEventRouter::GetQueueSize() const {
    ::std::lock_guard lock(_queueMutex);
    return _eventQueue.size();
}

uint64 CombatEventRouter::GetEventsDispatchedByType(CombatEventType type) const {
    // PERFORMANCE FIX: Lock-free stats read - no mutex needed
    uint32 bitIndex = GetEventTypeBitIndex(type);
    if (bitIndex < MAX_EVENT_TYPE_BITS) {
        return _eventsByTypeLockFree[bitIndex].load(::std::memory_order_relaxed);
    }
    return 0;
}

void CombatEventRouter::SortSubscribers() {
    ::std::unique_lock lock(_subscriberMutex);

    for (auto& [eventType, subs] : _subscribers) {
        ::std::sort(subs.begin(), subs.end(),
            [](ICombatEventSubscriber* a, ICombatEventSubscriber* b) {
                return a->GetEventPriority() > b->GetEventPriority();
            });
    }
}

} // namespace Playerbot
