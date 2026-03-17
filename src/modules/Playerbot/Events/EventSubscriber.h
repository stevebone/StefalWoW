/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_EVENT_SUBSCRIBER_H
#define PLAYERBOT_EVENT_SUBSCRIBER_H

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include <functional>
#include <vector>
#include <memory>
#include <mutex>

namespace Playerbot
{

/**
 * @brief Thread-safe subscriber management for event buses
 *
 * Template-based subscriber system supporting:
 * - Multiple subscribers per event type
 * - Thread-safe subscription/unsubscription
 * - Automatic cleanup via RAII
 * - Event filtering predicates
 *
 * @tparam EventType The event struct type (GroupEvent, CombatEvent, etc.)
 */
template<typename EventType>
class EventSubscriberManager
{
public:
    using EventHandler = ::std::function<void(EventType const&)>;
    using EventPredicate = ::std::function<bool(EventType const&)>;

    /**
     * @brief Subscription handle for RAII-based cleanup
     */
    class SubscriptionHandle
    {
    public:
        SubscriptionHandle(EventSubscriberManager* manager, uint32 subscriptionId)
            : _manager(manager), _subscriptionId(subscriptionId) {}

        ~SubscriptionHandle()
        {
            if (_manager && _subscriptionId != 0)
                _manager->Unsubscribe(_subscriptionId);
        }

        // Non-copyable, movable
        SubscriptionHandle(SubscriptionHandle const&) = delete;
        SubscriptionHandle& operator=(SubscriptionHandle const&) = delete;
        SubscriptionHandle(SubscriptionHandle&& other) noexcept
            : _manager(other._manager), _subscriptionId(other._subscriptionId)
        {
            other._manager = nullptr;
            other._subscriptionId = 0;
        }

        uint32 GetId() const { return _subscriptionId; }

    private:
        EventSubscriberManager* _manager;
        uint32 _subscriptionId;
    };

    /**
     * @brief Subscribe to events with optional filtering
     *
     * @param handler Function to call when event occurs
     * @param predicate Optional filter (return true to receive event)
     * @return Subscription handle for automatic cleanup
     */
    ::std::shared_ptr<SubscriptionHandle> Subscribe(EventHandler handler, EventPredicate predicate = nullptr)
    {
        ::std::lock_guard lock(_mutex);

        Subscriber sub;
        sub.id = ++_nextSubscriptionId;
        sub.handler = ::std::move(handler);
        sub.predicate = ::std::move(predicate);

        _subscribers.push_back(::std::move(sub));

        return ::std::make_shared<SubscriptionHandle>(this, sub.id);
    }

    /**
     * @brief Unsubscribe by ID
     */
    void Unsubscribe(uint32 subscriptionId)
    {
        ::std::lock_guard lock(_mutex);

        _subscribers.erase(
            ::std::remove_if(_subscribers.begin(), _subscribers.end(),
                [subscriptionId](Subscriber const& sub) { return sub.id == subscriptionId; }),
            _subscribers.end());
    }

    /**
     * @brief Publish event to all matching subscribers
     *
     * @param event The event to publish
     */
    void PublishEvent(EventType const& event)
    {
        ::std::lock_guard lock(_mutex);

        for (auto const& subscriber : _subscribers)
        {
            // Apply predicate filter if present
            if (subscriber.predicate && !subscriber.predicate(event))
                continue;

            // Invoke handler
            subscriber.handler(event);
        }
    }

    /**
     * @brief Get subscriber count
     */
    size_t GetSubscriberCount() const
    {
        ::std::lock_guard lock(_mutex);
        return _subscribers.size();
    }

    /**
     * @brief Clear all subscribers
     */
    void ClearSubscribers()
    {
        ::std::lock_guard lock(_mutex);
        _subscribers.clear();
    }

private:
    struct Subscriber
    {
        uint32 id;
        EventHandler handler;
        EventPredicate predicate;
    };

    ::std::vector<Subscriber> _subscribers;
    uint32 _nextSubscriptionId = 0;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BEHAVIOR_MANAGER> _mutex;
};

} // namespace Playerbot

#endif // PLAYERBOT_EVENT_SUBSCRIBER_H
