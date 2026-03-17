/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "ObjectGuid.h"
#include "Lifecycle/SpawnRequest.h"
#include <functional>
#include <memory>
#include <vector>
#include <mutex>
#include <atomic>
#include <queue>

namespace Playerbot
{

// Forward declarations
class BotSession;

/**
 * @brief Event types for bot spawning workflow
 */
enum class BotSpawnEventType : uint32
{
    SPAWN_REQUESTED = 1,
    CHARACTER_SELECTED = 2,
    SESSION_CREATED = 3,
    SPAWN_COMPLETED = 4,
    SPAWN_FAILED = 5,
    POPULATION_CHANGED = 6,
    PERFORMANCE_ALERT = 7
};

/**
 * @brief Base class for all bot spawn events
 */
struct BotSpawnEvent
{
    BotSpawnEventType type;
    ::std::chrono::steady_clock::time_point timestamp;
    uint64 eventId;

    BotSpawnEvent(BotSpawnEventType eventType)
        : type(eventType), timestamp(::std::chrono::steady_clock::now()), eventId(0) {}

    virtual ~BotSpawnEvent() = default;
};

/**
 * @brief Specific event types
 */
struct SpawnRequestEvent : public BotSpawnEvent
{
    SpawnRequest request;
    ::std::function<void(bool, ObjectGuid)> callback;

    SpawnRequestEvent(SpawnRequest req, ::std::function<void(bool, ObjectGuid)> cb)
        : BotSpawnEvent(BotSpawnEventType::SPAWN_REQUESTED), request(::std::move(req)), callback(::std::move(cb)) {}
};

struct CharacterSelectedEvent : public BotSpawnEvent
{
    ObjectGuid characterGuid;
    SpawnRequest originalRequest;

    CharacterSelectedEvent(ObjectGuid guid, SpawnRequest req)
        : BotSpawnEvent(BotSpawnEventType::CHARACTER_SELECTED), characterGuid(guid), originalRequest(::std::move(req)) {}
};

struct SessionCreatedEvent : public BotSpawnEvent
{
    ::std::shared_ptr<BotSession> session;
    SpawnRequest originalRequest;

    SessionCreatedEvent(::std::shared_ptr<BotSession> sess, SpawnRequest req)
        : BotSpawnEvent(BotSpawnEventType::SESSION_CREATED), session(::std::move(sess)), originalRequest(::std::move(req)) {}
};

struct SpawnCompletedEvent : public BotSpawnEvent
{
    ObjectGuid botGuid;
    bool success;
    ::std::string details;

    SpawnCompletedEvent(ObjectGuid guid, bool succeeded, ::std::string info = "")
        : BotSpawnEvent(BotSpawnEventType::SPAWN_COMPLETED), botGuid(guid), success(succeeded), details(::std::move(info)) {}
};

struct PopulationChangedEvent : public BotSpawnEvent
{
    uint32 zoneId;
    uint32 oldBotCount;
    uint32 newBotCount;

    PopulationChangedEvent(uint32 zone, uint32 oldCount, uint32 newCount)
        : BotSpawnEvent(BotSpawnEventType::POPULATION_CHANGED), zoneId(zone), oldBotCount(oldCount), newBotCount(newCount) {}
};

// Public-facing event statistics (copyable snapshot)
struct EventStats
{
    uint64 eventsPublished{0};
    uint64 eventsProcessed{0};
    uint64 eventsDropped{0};
    uint64 totalProcessingTimeUs{0};
    uint32 queuedEvents{0};
    float averageProcessingTimeUs{0.0f};

    // Calculate average processing time from totals
    float GetAverageProcessingTimeUs() const
    {
        return eventsProcessed > 0 ? static_cast<float>(totalProcessingTimeUs) / eventsProcessed : 0.0f;
    }
};

// Internal statistics with atomic members for thread-safe tracking
struct InternalEventStats
{
    ::std::atomic<uint64> eventsPublished{0};
    ::std::atomic<uint64> eventsProcessed{0};
    ::std::atomic<uint64> eventsDropped{0};
    ::std::atomic<uint64> totalProcessingTimeUs{0};
    ::std::atomic<uint32> queuedEvents{0};

    // Delete copy constructor and assignment operator for atomic members
    InternalEventStats() = default;
    InternalEventStats(InternalEventStats const&) = delete;
    InternalEventStats& operator=(InternalEventStats const&) = delete;

    float GetAverageProcessingTimeUs() const
    {
        uint64 processed = eventsProcessed.load();
        return processed > 0 ? static_cast<float>(totalProcessingTimeUs.load()) / processed : 0.0f;
    }
};

/**
 * @class BotSpawnEventBus
 * @brief Event-driven architecture for bot spawning workflow
 *
 * ARCHITECTURE REFACTORING: Implements event-driven spawning to decouple
 * components and improve scalability for 5000 concurrent bots.
 *
 * Benefits:
 * - Loose coupling between spawning components
 * - Async event processing for high throughput
 * - Easy to add new features without modifying existing code
 * - Built-in event logging and monitoring
 * - Scalable to thousands of concurrent spawn operations
 *
 * Performance Features:
 * - Lock-free event queuing where possible
 * - Batched event processing
 * - Priority-based event handling
 * - Automatic event deduplication
 * - Memory-efficient event storage
 */
class TC_GAME_API BotSpawnEventBus final 
{
public:
    using EventHandler = ::std::function<void(::std::shared_ptr<BotSpawnEvent>)>;

    BotSpawnEventBus();
    ~BotSpawnEventBus() = default;

    // Singleton access
    static BotSpawnEventBus* instance();

    // Lifecycle
    bool Initialize();
    void Shutdown();
    void Update(uint32 diff);

    // === EVENT PUBLISHING ===
    void PublishEvent(::std::shared_ptr<BotSpawnEvent> event);

    // Convenience methods for common events
    void PublishSpawnRequest(SpawnRequest const& request, ::std::function<void(bool, ObjectGuid)> callback);
    void PublishCharacterSelected(ObjectGuid characterGuid, SpawnRequest const& request);
    void PublishSessionCreated(::std::shared_ptr<BotSession> session, SpawnRequest const& request);
    void PublishSpawnCompleted(ObjectGuid botGuid, bool success, ::std::string const& details = "");
    void PublishPopulationChanged(uint32 zoneId, uint32 oldCount, uint32 newCount);

    // === EVENT SUBSCRIPTION ===
    using HandlerId = uint64;

    HandlerId Subscribe(BotSpawnEventType eventType, EventHandler handler);
    HandlerId SubscribeToAll(EventHandler handler);
    void Unsubscribe(HandlerId handlerId);

    // === EVENT PROCESSING ===
    void ProcessEvents();
    void ProcessEventsOfType(BotSpawnEventType eventType);

    // === PERFORMANCE AND MONITORING ===
    EventStats const& GetStats() const;
    void ResetStats();

    // === CONFIGURATION ===
    void SetMaxQueueSize(uint32 maxSize) { _maxQueueSize = maxSize; }
    void SetBatchSize(uint32 batchSize) { _batchSize = batchSize; }
    void SetProcessingEnabled(bool enabled) { _processingEnabled = enabled; }

    uint32 GetQueuedEventCount() const { return _stats.queuedEvents.load(); }
    bool IsHealthy() const;

private:
    // Event queue management
    struct QueuedEvent
    {
        ::std::shared_ptr<BotSpawnEvent> event;
        uint32 priority = 0;
        ::std::chrono::steady_clock::time_point queueTime;
    };

    ::std::queue<QueuedEvent> _eventQueue;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_SPAWNER> _queueMutex;

    // Event handlers
    struct EventSubscription
    {
        HandlerId id;
        BotSpawnEventType eventType;
        EventHandler handler;
        bool isGlobal = false;
    };

    ::std::vector<EventSubscription> _subscriptions;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_SPAWNER> _subscriptionMutex;
    ::std::atomic<HandlerId> _nextHandlerId{1};

    // Event processing
    void ProcessEventInternal(::std::shared_ptr<BotSpawnEvent> event);
    void NotifySubscribers(::std::shared_ptr<BotSpawnEvent> event);

    uint32 GetEventPriority(BotSpawnEventType eventType) const;
    bool ShouldDropEvent(::std::shared_ptr<BotSpawnEvent> event) const;

    // Performance tracking
    mutable InternalEventStats _stats;
    mutable EventStats _statsSnapshot;
    void RecordEventProcessing(uint64 processingTimeUs);

    // Configuration
    uint32 _maxQueueSize = 10000;
    uint32 _batchSize = 100;
    ::std::atomic<bool> _processingEnabled{true};

    // Event ID generation
    ::std::atomic<uint64> _nextEventId{1};
    uint64 GenerateEventId() { return _nextEventId.fetch_add(1); }

    // Timing
    ::std::chrono::steady_clock::time_point _lastProcessing;
    static constexpr uint32 PROCESSING_INTERVAL_MS = 10; // 10ms

    // Singleton
    inline static ::std::unique_ptr<BotSpawnEventBus> _instance;
    inline static Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_SPAWNER> _instanceMutex;

    // Non-copyable
    BotSpawnEventBus(BotSpawnEventBus const&) = delete;
    BotSpawnEventBus& operator=(BotSpawnEventBus const&) = delete;
};

// Convenience macros for event publishing
#define PUBLISH_SPAWN_EVENT(event) sBotSpawnEventBus->PublishEvent(::std::make_shared<event>)
#define SUBSCRIBE_SPAWN_EVENT(eventType, handler) sBotSpawnEventBus->Subscribe(eventType, handler)

#define sBotSpawnEventBus BotSpawnEventBus::instance()

} // namespace Playerbot