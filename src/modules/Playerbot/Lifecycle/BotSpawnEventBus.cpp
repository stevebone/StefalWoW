/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BotSpawnEventBus.h"
#include "BotSpawner.h"
#include "Logging/Log.h"
#include <algorithm>
#include <chrono>

namespace Playerbot
{

// Static member definitions removed - now inline static in header to fix DLL export issues

BotSpawnEventBus::BotSpawnEventBus()
    : _lastProcessing(::std::chrono::steady_clock::now())
{
}

BotSpawnEventBus* BotSpawnEventBus::instance()
{
    ::std::lock_guard lock(_instanceMutex);
    if (!_instance)
        _instance = ::std::unique_ptr<BotSpawnEventBus>(new BotSpawnEventBus());
    return _instance.get();
}

bool BotSpawnEventBus::Initialize()
{
    TC_LOG_INFO("module.playerbot.events",
        "Initializing BotSpawnEventBus for event-driven spawning architecture");

    ResetStats();
    _lastProcessing = ::std::chrono::steady_clock::now();
    _processingEnabled.store(true);

    TC_LOG_INFO("module.playerbot.events",
        "BotSpawnEventBus initialized - Max Queue: {}, Batch Size: {}, Processing Interval: {}ms",
        _maxQueueSize, _batchSize, PROCESSING_INTERVAL_MS);

    return true;
}

void BotSpawnEventBus::Shutdown()
{
    TC_LOG_INFO("module.playerbot.events", "Shutting down BotSpawnEventBus");

    _processingEnabled.store(false);

    // Process remaining events
    ProcessEvents();

    // Log final stats
    auto const& stats = GetStats();
    float avgProcessingTime = stats.GetAverageProcessingTimeUs();
    TC_LOG_INFO("module.playerbot.events",
        "Final Event Statistics - Published: {}, Processed: {}, Dropped: {}, Avg Processing: {}μs",
        stats.eventsPublished, stats.eventsProcessed,
        stats.eventsDropped, avgProcessingTime);

    // Clear subscriptions
    {
        ::std::lock_guard lock(_subscriptionMutex);
        _subscriptions.clear();
    }

    // Clear queue
    {
        ::std::lock_guard lock(_queueMutex);
        ::std::queue<QueuedEvent> empty;
        _eventQueue.swap(empty);
    }
}

void BotSpawnEventBus::Update(uint32 /*diff*/)
{
    auto now = ::std::chrono::steady_clock::now();
    auto elapsed = ::std::chrono::duration_cast<::std::chrono::milliseconds>(now - _lastProcessing);

    if (elapsed.count() >= PROCESSING_INTERVAL_MS)
    {
        ProcessEvents();
        _lastProcessing = now;
    }
}

// === EVENT PUBLISHING ===

void BotSpawnEventBus::PublishEvent(::std::shared_ptr<BotSpawnEvent> event)
{
    if (!event || !_processingEnabled.load())
        return;

    // Assign unique event ID
    event->eventId = GenerateEventId();

    // Check if event should be dropped (performance protection)
    if (ShouldDropEvent(event))
    {
        _stats.eventsDropped.fetch_add(1);
        TC_LOG_WARN("module.playerbot.events",
            "Dropping event {} due to queue overload or rate limiting",
            static_cast<uint32>(event->type));
        return;
    }

    // Queue the event
    {
        ::std::lock_guard lock(_queueMutex);
        if (_eventQueue.size() >= _maxQueueSize)
        {
            // Drop oldest event to make room
            _eventQueue.pop();
            _stats.eventsDropped.fetch_add(1);
        }

        QueuedEvent queuedEvent;
        queuedEvent.event = event;
        queuedEvent.priority = GetEventPriority(event->type);
        queuedEvent.queueTime = ::std::chrono::steady_clock::now();

        _eventQueue.push(queuedEvent);
        _stats.queuedEvents.store(_eventQueue.size());
    }

    _stats.eventsPublished.fetch_add(1);
}

void BotSpawnEventBus::PublishSpawnRequest(SpawnRequest const& request, ::std::function<void(bool, ObjectGuid)> callback)
{
    auto event = ::std::make_shared<SpawnRequestEvent>(request, ::std::move(callback));
    PublishEvent(event);
}

void BotSpawnEventBus::PublishCharacterSelected(ObjectGuid characterGuid, SpawnRequest const& request)
{
    auto event = ::std::make_shared<CharacterSelectedEvent>(characterGuid, request);
    PublishEvent(event);
}

void BotSpawnEventBus::PublishSessionCreated(::std::shared_ptr<BotSession> session, SpawnRequest const& request)
{
    auto event = ::std::make_shared<SessionCreatedEvent>(session, request);
    PublishEvent(event);
}

void BotSpawnEventBus::PublishSpawnCompleted(ObjectGuid botGuid, bool success, ::std::string const& details)
{
    auto event = ::std::make_shared<SpawnCompletedEvent>(botGuid, success, details);
    PublishEvent(event);
}

void BotSpawnEventBus::PublishPopulationChanged(uint32 zoneId, uint32 oldCount, uint32 newCount)
{
    auto event = ::std::make_shared<PopulationChangedEvent>(zoneId, oldCount, newCount);
    PublishEvent(event);
}

// === EVENT SUBSCRIPTION ===

BotSpawnEventBus::HandlerId BotSpawnEventBus::Subscribe(BotSpawnEventType eventType, EventHandler handler)
{
    ::std::lock_guard lock(_subscriptionMutex);

    HandlerId id = _nextHandlerId.fetch_add(1);

    EventSubscription subscription;
    subscription.id = id;
    subscription.eventType = eventType;
    subscription.handler = ::std::move(handler);
    subscription.isGlobal = false;

    _subscriptions.push_back(subscription);

    TC_LOG_DEBUG("module.playerbot.events",
        "Subscribed handler {} to event type {}", id, static_cast<uint32>(eventType));

    return id;
}

BotSpawnEventBus::HandlerId BotSpawnEventBus::SubscribeToAll(EventHandler handler)
{
    ::std::lock_guard lock(_subscriptionMutex);

    HandlerId id = _nextHandlerId.fetch_add(1);

    EventSubscription subscription;
    subscription.id = id;
    subscription.eventType = BotSpawnEventType::SPAWN_REQUESTED; // Unused for global
    subscription.handler = ::std::move(handler);
    subscription.isGlobal = true;

    _subscriptions.push_back(subscription);

    TC_LOG_DEBUG("module.playerbot.events", "Subscribed global handler {}", id);

    return id;
}

void BotSpawnEventBus::Unsubscribe(HandlerId handlerId)
{
    ::std::lock_guard lock(_subscriptionMutex);

    auto it = ::std::remove_if(_subscriptions.begin(), _subscriptions.end(),
        [handlerId](EventSubscription const& sub) {
            return sub.id == handlerId;
        });

    if (it != _subscriptions.end())
    {
        _subscriptions.erase(it, _subscriptions.end());
        TC_LOG_DEBUG("module.playerbot.events", "Unsubscribed handler {}", handlerId);
    }
}

// === EVENT PROCESSING ===

void BotSpawnEventBus::ProcessEvents()
{
    if (!_processingEnabled.load())
        return;

    uint32 processed = 0;
    auto processingStart = ::std::chrono::high_resolution_clock::now();

    // Process events in batches for better performance
    while (processed < _batchSize)
    {
        QueuedEvent queuedEvent;

        // Get next event from queue
        {
            ::std::lock_guard lock(_queueMutex);
            if (_eventQueue.empty())
                break;

            queuedEvent = _eventQueue.front();
            _eventQueue.pop();
            _stats.queuedEvents.store(_eventQueue.size());
        }

        // Process the event
        ProcessEventInternal(queuedEvent.event);
        processed++;
    }

    // Record processing performance
    if (processed > 0)
    {
        auto processingEnd = ::std::chrono::high_resolution_clock::now();
        auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(processingEnd - processingStart);
        RecordEventProcessing(duration.count());

        _stats.eventsProcessed.fetch_add(processed);
    }
}

void BotSpawnEventBus::ProcessEventsOfType(BotSpawnEventType eventType)
{
    ::std::vector<QueuedEvent> eventsToProcess;
    ::std::vector<QueuedEvent> eventsToKeep;

    // Extract events of specific type
    {
        ::std::lock_guard lock(_queueMutex);

        while (!_eventQueue.empty())
        {
            QueuedEvent queuedEvent = _eventQueue.front();
            _eventQueue.pop();

            if (queuedEvent.event->type == eventType)
                eventsToProcess.push_back(queuedEvent);
            else
                eventsToKeep.push_back(queuedEvent);
        }

        // Put back non-matching events
    for (auto const& event : eventsToKeep)
            _eventQueue.push(event);

        _stats.queuedEvents.store(_eventQueue.size());
    }

    // Process extracted events
    for (auto const& queuedEvent : eventsToProcess)
    {
        ProcessEventInternal(queuedEvent.event);
        _stats.eventsProcessed.fetch_add(1);
    }
}

void BotSpawnEventBus::ProcessEventInternal(::std::shared_ptr<BotSpawnEvent> event)
{
    auto start = ::std::chrono::high_resolution_clock::now();

    try
    {
        NotifySubscribers(event);
    }
    catch (::std::exception const& ex)
    {
        TC_LOG_ERROR("module.playerbot.events",
            "Exception processing event {}: {}", static_cast<uint32>(event->type), ex.what());
    }
    catch (...)
    {
        TC_LOG_ERROR("module.playerbot.events",
            "Unknown exception processing event {}", static_cast<uint32>(event->type));
    }

    auto end = ::std::chrono::high_resolution_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(end - start);
    RecordEventProcessing(duration.count());
}

void BotSpawnEventBus::NotifySubscribers(::std::shared_ptr<BotSpawnEvent> event)
{
    ::std::lock_guard lock(_subscriptionMutex);

    for (auto const& subscription : _subscriptions)
    {
        if (subscription.isGlobal || subscription.eventType == event->type)
        {
            try
            {
                subscription.handler(event);
            }
            catch (::std::exception const& ex)
            {
                TC_LOG_ERROR("module.playerbot.events",
                    "Exception in event handler {}: {}", subscription.id, ex.what());
            }
            catch (...)
            {
                TC_LOG_ERROR("module.playerbot.events",
                    "Unknown exception in event handler {}", subscription.id);
            }
        }
    }
}

// === HELPER METHODS ===

uint32 BotSpawnEventBus::GetEventPriority(BotSpawnEventType eventType) const
{
    switch (eventType)
    {
        case BotSpawnEventType::PERFORMANCE_ALERT:
            return 1; // Highest priority
        case BotSpawnEventType::SPAWN_FAILED:
            return 2;
        case BotSpawnEventType::SPAWN_REQUESTED:
            return 3;
        case BotSpawnEventType::CHARACTER_SELECTED:
            return 4;
        case BotSpawnEventType::SESSION_CREATED:
            return 5;
        case BotSpawnEventType::SPAWN_COMPLETED:
            return 6;
        case BotSpawnEventType::POPULATION_CHANGED:
            return 7; // Lowest priority
        default:
            return 5; // Default medium priority
    }
}

bool BotSpawnEventBus::ShouldDropEvent(::std::shared_ptr<BotSpawnEvent> event) const
{
    // Drop events if queue is near capacity
    uint32 currentQueueSize = _stats.queuedEvents.load();
    if (currentQueueSize >= _maxQueueSize * 0.9f)
    {
        // Only keep high-priority events when queue is nearly full
        uint32 priority = GetEventPriority(event->type);
        return priority > 3; // Drop medium/low priority events
    }

    return false;
}

void BotSpawnEventBus::RecordEventProcessing(uint64 processingTimeUs)
{
    _stats.totalProcessingTimeUs.fetch_add(processingTimeUs);
}

EventStats const& BotSpawnEventBus::GetStats() const
{
    // Update snapshot from atomic internal stats
    _statsSnapshot.eventsPublished = _stats.eventsPublished.load();
    _statsSnapshot.eventsProcessed = _stats.eventsProcessed.load();
    _statsSnapshot.eventsDropped = _stats.eventsDropped.load();
    _statsSnapshot.totalProcessingTimeUs = _stats.totalProcessingTimeUs.load();
    _statsSnapshot.queuedEvents = _stats.queuedEvents.load();

    return _statsSnapshot;
}

void BotSpawnEventBus::ResetStats()
{
    _stats.eventsPublished.store(0);
    _stats.eventsProcessed.store(0);
    _stats.eventsDropped.store(0);
    _stats.totalProcessingTimeUs.store(0);
    _stats.queuedEvents.store(0);
}

bool BotSpawnEventBus::IsHealthy() const
{
    uint32 queuedEvents = _stats.queuedEvents.load();
    float avgProcessingTime = _stats.GetAverageProcessingTimeUs();

    // Health checks for event bus performance
    return queuedEvents < _maxQueueSize * 0.8f &&      // Queue not overwhelmed
           avgProcessingTime < 1000.0f &&               // Processing under 1ms average
           _processingEnabled.load();                    // Processing enabled
}

} // namespace Playerbot