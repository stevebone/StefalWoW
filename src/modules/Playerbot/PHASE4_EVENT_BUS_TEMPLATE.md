# Phase 4 Event Bus Implementation Template

## Overview
This document provides the exact template for implementing each event bus. All event buses follow the identical pattern established by **CooldownEventBus** and **AuraEventBus**.

## Architecture Pattern

### File Structure
Each event bus requires exactly 2 files:
- `{EventType}EventBus.h` (~160 lines) - Interface definition
- `{EventType}EventBus.cpp` (~440 lines) - Complete implementation

### Core Components (Identical for ALL Buses)

1. **Priority Enum** - Event processing priority
2. **Event Struct** - Event data with helper constructors
3. **Event Bus Class** - Singleton with pub/sub infrastructure
4. **Thread Safety** - Mutexes for queue and subscribers
5. **Statistics** - Atomic counters for monitoring
6. **Cleanup** - Periodic expired event removal

## Header File Template (~160 lines)

```cpp
/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_{EVENTTYPE_UPPER}_EVENT_BUS_H
#define PLAYERBOT_{EVENTTYPE_UPPER}_EVENT_BUS_H

#include "Define.h"
#include "ObjectGuid.h"
#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <atomic>

namespace Playerbot
{

class BotAI;

enum class {EventType}EventPriority : uint8
{
    CRITICAL = 0,
    HIGH = 1,
    MEDIUM = 2,
    LOW = 3,
    BATCH = 4
};

struct {EventType}Event
{
    {EventType}EventType type;
    {EventType}EventPriority priority;

    // EVENT-SPECIFIC FIELDS HERE (see field mapping below)

    std::chrono::steady_clock::time_point timestamp;
    std::chrono::steady_clock::time_point expiryTime;

    bool IsValid() const;
    bool IsExpired() const;
    std::string ToString() const;

    // Helper constructors (2-4 per event type)
    static {EventType}Event HelperConstructor1(...);
    static {EventType}Event HelperConstructor2(...);

    // Priority comparison for priority queue
    bool operator<({EventType}Event const& other) const
    {
        return priority > other.priority;
    }
};

class TC_GAME_API {EventType}EventBus
{
public:
    static {EventType}EventBus* instance();

    // Event publishing
    bool PublishEvent({EventType}Event const& event);

    // Subscription management
    bool Subscribe(BotAI* subscriber, std::vector<{EventType}EventType> const& types);
    bool SubscribeAll(BotAI* subscriber);
    void Unsubscribe(BotAI* subscriber);

    // Event processing
    uint32 ProcessEvents(uint32 diff, uint32 maxEvents = 0);
    uint32 ProcessUnitEvents(ObjectGuid unitGuid, uint32 diff);
    void ClearUnitEvents(ObjectGuid unitGuid);

    // Status queries
    uint32 GetPendingEventCount() const;
    uint32 GetSubscriberCount() const;

    // Diagnostics
    void DumpSubscribers() const;
    void DumpEventQueue() const;
    std::vector<{EventType}Event> GetQueueSnapshot() const;

    // Statistics
    struct Statistics
    {
        std::atomic<uint64_t> totalEventsPublished{0};
        std::atomic<uint64_t> totalEventsProcessed{0};
        std::atomic<uint64_t> totalEventsDropped{0};
        std::atomic<uint64_t> totalDeliveries{0};
        std::atomic<uint64_t> averageProcessingTimeUs{0};
        std::atomic<uint32_t> peakQueueSize{0};
        std::chrono::steady_clock::time_point startTime;

        void Reset();
        std::string ToString() const;
    };

    Statistics const& GetStatistics() const { return _stats; }

private:
    {EventType}EventBus();
    ~{EventType}EventBus();

    // Event delivery
    bool DeliverEvent(BotAI* subscriber, {EventType}Event const& event);
    bool ValidateEvent({EventType}Event const& event) const;
    uint32 CleanupExpiredEvents();
    void UpdateMetrics(std::chrono::microseconds processingTime);
    void LogEvent({EventType}Event const& event, std::string const& action) const;

    // Event queue
    std::priority_queue<{EventType}Event> _eventQueue;
    mutable std::mutex _queueMutex;

    // Subscriber management
    std::unordered_map<{EventType}EventType, std::vector<BotAI*>> _subscribers;
    std::vector<BotAI*> _globalSubscribers;
    mutable std::mutex _subscriberMutex;

    // Configuration
    static constexpr uint32 MAX_QUEUE_SIZE = 10000;
    static constexpr uint32 CLEANUP_INTERVAL = 30000;
    static constexpr uint32 MAX_SUBSCRIBERS_PER_EVENT = 5000;

    // Timers
    uint32 _cleanupTimer = 0;
    uint32 _metricsUpdateTimer = 0;

    // Statistics
    Statistics _stats;
    uint32 _maxQueueSize = MAX_QUEUE_SIZE;
};

} // namespace Playerbot

#endif // PLAYERBOT_{EVENTTYPE_UPPER}_EVENT_BUS_H
```

## Implementation File Template (~440 lines)

```cpp
/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "{EventType}EventBus.h"
#include "BotAI.h"
#include "Player.h"
#include "Log.h"
#include "Timer.h"
#include <sstream>

namespace Playerbot
{

// ============================================================================
// {EventType}Event Helper Constructors
// ============================================================================

{EventType}Event {EventType}Event::HelperConstructor1(...)
{
    {EventType}Event event;
    event.type = {EventType}EventType::XXX;
    event.priority = {EventType}EventPriority::MEDIUM;
    // Set event-specific fields
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(30000);
    return event;
}

// Repeat for all helper constructors

bool {EventType}Event::IsValid() const
{
    if (type >= {EventType}EventType::MAX_{EVENTTYPE_UPPER}_EVENT)
        return false;
    if (timestamp.time_since_epoch().count() == 0)
        return false;
    // Add event-specific validation
    return true;
}

bool {EventType}Event::IsExpired() const
{
    return std::chrono::steady_clock::now() >= expiryTime;
}

std::string {EventType}Event::ToString() const
{
    std::ostringstream oss;
    oss << "[{EventType}Event] Type: " << static_cast<uint32>(type);
    // Add event-specific fields to string
    return oss.str();
}

// ============================================================================
// {EventType}EventBus Implementation
// ============================================================================

{EventType}EventBus::{EventType}EventBus()
{
    _stats.startTime = std::chrono::steady_clock::now();
    TC_LOG_INFO("module.playerbot.{eventtype_lower}", "{EventType}EventBus initialized");
}

{EventType}EventBus::~{EventType}EventBus()
{
    TC_LOG_INFO("module.playerbot.{eventtype_lower}", "{EventType}EventBus shutting down - Stats: {}", _stats.ToString());
}

{EventType}EventBus* {EventType}EventBus::instance()
{
    static {EventType}EventBus instance;
    return &instance;
}

bool {EventType}EventBus::PublishEvent({EventType}Event const& event)
{
    if (!ValidateEvent(event))
    {
        _stats.totalEventsDropped++;
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(_queueMutex);
        if (_eventQueue.size() >= _maxQueueSize)
        {
            _stats.totalEventsDropped++;
            return false;
        }

        _eventQueue.push(event);

        uint32 currentSize = static_cast<uint32>(_eventQueue.size());
        uint32 expectedPeak = _stats.peakQueueSize.load();
        while (currentSize > expectedPeak &&
               !_stats.peakQueueSize.compare_exchange_weak(expectedPeak, currentSize))
        {
        }
    }

    _stats.totalEventsPublished++;
    LogEvent(event, "Published");
    return true;
}

bool {EventType}EventBus::Subscribe(BotAI* subscriber, std::vector<{EventType}EventType> const& types)
{
    if (!subscriber)
        return false;

    std::lock_guard<std::mutex> lock(_subscriberMutex);

    for ({EventType}EventType type : types)
    {
        auto& subscriberList = _subscribers[type];
        if (std::find(subscriberList.begin(), subscriberList.end(), subscriber) != subscriberList.end())
            continue;
        if (subscriberList.size() >= MAX_SUBSCRIBERS_PER_EVENT)
            return false;
        subscriberList.push_back(subscriber);
    }

    return true;
}

bool {EventType}EventBus::SubscribeAll(BotAI* subscriber)
{
    if (!subscriber)
        return false;

    std::lock_guard<std::mutex> lock(_subscriberMutex);

    if (std::find(_globalSubscribers.begin(), _globalSubscribers.end(), subscriber) != _globalSubscribers.end())
        return false;

    _globalSubscribers.push_back(subscriber);
    return true;
}

void {EventType}EventBus::Unsubscribe(BotAI* subscriber)
{
    if (!subscriber)
        return;

    std::lock_guard<std::mutex> lock(_subscriberMutex);

    for (auto& [type, subscriberList] : _subscribers)
    {
        subscriberList.erase(
            std::remove(subscriberList.begin(), subscriberList.end(), subscriber),
            subscriberList.end()
        );
    }

    _globalSubscribers.erase(
        std::remove(_globalSubscribers.begin(), _globalSubscribers.end(), subscriber),
        _globalSubscribers.end()
    );
}

uint32 {EventType}EventBus::ProcessEvents(uint32 diff, uint32 maxEvents)
{
    auto startTime = std::chrono::high_resolution_clock::now();

    _cleanupTimer += diff;
    if (_cleanupTimer >= CLEANUP_INTERVAL)
    {
        CleanupExpiredEvents();
        _cleanupTimer = 0;
    }

    uint32 processedCount = 0;
    std::vector<{EventType}Event> eventsToProcess;

    {
        std::lock_guard<std::mutex> lock(_queueMutex);

        while (!_eventQueue.empty() && (maxEvents == 0 || processedCount < maxEvents))
        {
            {EventType}Event event = _eventQueue.top();
            _eventQueue.pop();

            if (event.IsExpired())
            {
                _stats.totalEventsDropped++;
                continue;
            }

            eventsToProcess.push_back(event);
            processedCount++;
        }
    }

    for ({EventType}Event const& event : eventsToProcess)
    {
        std::vector<BotAI*> subscribers;
        std::vector<BotAI*> globalSubs;

        {
            std::lock_guard<std::mutex> lock(_subscriberMutex);
            auto it = _subscribers.find(event.type);
            if (it != _subscribers.end())
                subscribers = it->second;
            globalSubs = _globalSubscribers;
        }

        for (BotAI* subscriber : subscribers)
        {
            if (DeliverEvent(subscriber, event))
                _stats.totalDeliveries++;
        }

        for (BotAI* subscriber : globalSubs)
        {
            if (DeliverEvent(subscriber, event))
                _stats.totalDeliveries++;
        }

        _stats.totalEventsProcessed++;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

    if (processedCount > 0)
        UpdateMetrics(duration);

    return processedCount;
}

uint32 {EventType}EventBus::ProcessUnitEvents(ObjectGuid unitGuid, uint32 diff)
{
    return ProcessEvents(diff, 0);
}

void {EventType}EventBus::ClearUnitEvents(ObjectGuid unitGuid)
{
    std::lock_guard<std::mutex> lock(_queueMutex);

    std::vector<{EventType}Event> remainingEvents;

    while (!_eventQueue.empty())
    {
        {EventType}Event event = _eventQueue.top();
        _eventQueue.pop();

        // EVENT-SPECIFIC: Check if event belongs to unitGuid
        if (/* event doesn't match unitGuid */)
            remainingEvents.push_back(event);
        else
            _stats.totalEventsDropped++;
    }

    for ({EventType}Event const& event : remainingEvents)
        _eventQueue.push(event);
}

uint32 {EventType}EventBus::GetPendingEventCount() const
{
    std::lock_guard<std::mutex> lock(_queueMutex);
    return static_cast<uint32>(_eventQueue.size());
}

uint32 {EventType}EventBus::GetSubscriberCount() const
{
    std::lock_guard<std::mutex> lock(_subscriberMutex);

    uint32 count = static_cast<uint32>(_globalSubscribers.size());

    for (auto const& [type, subscriberList] : _subscribers)
        count += static_cast<uint32>(subscriberList.size());

    return count;
}

bool {EventType}EventBus::DeliverEvent(BotAI* subscriber, {EventType}Event const& event)
{
    if (!subscriber)
        return false;

    try
    {
        // Phase 4: Call virtual event handler on BotAI
        subscriber->On{EventType}Event(event);
        TC_LOG_TRACE("module.playerbot.{eventtype_lower}", "{EventType}EventBus: Delivered event to subscriber");
        return true;
    }
    catch (std::exception const& e)
    {
        TC_LOG_ERROR("module.playerbot.{eventtype_lower}", "{EventType}EventBus: Exception delivering event: {}", e.what());
        return false;
    }
}

bool {EventType}EventBus::ValidateEvent({EventType}Event const& event) const
{
    return event.IsValid() && !event.IsExpired();
}

uint32 {EventType}EventBus::CleanupExpiredEvents()
{
    std::lock_guard<std::mutex> lock(_queueMutex);

    uint32 cleanedCount = 0;
    std::vector<{EventType}Event> validEvents;

    while (!_eventQueue.empty())
    {
        {EventType}Event event = _eventQueue.top();
        _eventQueue.pop();

        if (!event.IsExpired())
            validEvents.push_back(event);
        else
            cleanedCount++;
    }

    for ({EventType}Event const& event : validEvents)
        _eventQueue.push(event);

    return cleanedCount;
}

void {EventType}EventBus::UpdateMetrics(std::chrono::microseconds processingTime)
{
    uint64_t currentAvg = _stats.averageProcessingTimeUs.load();
    uint64_t newTime = processingTime.count();
    uint64_t newAvg = (currentAvg * 9 + newTime) / 10;
    _stats.averageProcessingTimeUs.store(newAvg);
}

void {EventType}EventBus::LogEvent({EventType}Event const& event, std::string const& action) const
{
    TC_LOG_TRACE("module.playerbot.{eventtype_lower}", "{EventType}EventBus: {} event - {}", action, event.ToString());
}

void {EventType}EventBus::DumpSubscribers() const
{
    std::lock_guard<std::mutex> lock(_subscriberMutex);
    TC_LOG_INFO("module.playerbot.{eventtype_lower}", "=== {EventType}EventBus Subscribers: {} global ===", _globalSubscribers.size());
}

void {EventType}EventBus::DumpEventQueue() const
{
    std::lock_guard<std::mutex> lock(_queueMutex);
    TC_LOG_INFO("module.playerbot.{eventtype_lower}", "=== {EventType}EventBus Queue: {} events ===", _eventQueue.size());
}

std::vector<{EventType}Event> {EventType}EventBus::GetQueueSnapshot() const
{
    std::lock_guard<std::mutex> lock(_queueMutex);

    std::vector<{EventType}Event> snapshot;
    std::priority_queue<{EventType}Event> tempQueue = _eventQueue;

    while (!tempQueue.empty())
    {
        snapshot.push_back(tempQueue.top());
        tempQueue.pop();
    }

    return snapshot;
}

void {EventType}EventBus::Statistics::Reset()
{
    totalEventsPublished.store(0);
    totalEventsProcessed.store(0);
    totalEventsDropped.store(0);
    totalDeliveries.store(0);
    averageProcessingTimeUs.store(0);
    peakQueueSize.store(0);
    startTime = std::chrono::steady_clock::now();
}

std::string {EventType}EventBus::Statistics::ToString() const
{
    auto now = std::chrono::steady_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);

    std::ostringstream oss;
    oss << "Published: " << totalEventsPublished.load()
        << ", Processed: " << totalEventsProcessed.load()
        << ", Dropped: " << totalEventsDropped.load()
        << ", Deliveries: " << totalDeliveries.load()
        << ", Avg Processing: " << averageProcessingTimeUs.load() << "μs"
        << ", Peak Queue: " << peakQueueSize.load()
        << ", Uptime: " << uptime.count() << "s";
    return oss.str();
}

} // namespace Playerbot
```

## Event-Specific Field Mapping

### LootEventBus
**Fields:**
- `ObjectGuid looterGuid`
- `ObjectGuid itemGuid`
- `uint32 itemEntry`
- `uint32 itemCount`
- `LootType lootType`

**Helper Constructors:**
- `LootEvent::ItemLooted(ObjectGuid looter, ObjectGuid item, uint32 entry, uint32 count)`
- `LootEvent::LootRollStarted(ObjectGuid item, uint32 entry)`
- `LootEvent::LootRollWon(ObjectGuid winner, ObjectGuid item, uint32 entry)`

**ClearUnitEvents:** Check `looterGuid == unitGuid`

### QuestEventBus
**Fields:**
- `ObjectGuid playerGuid`
- `uint32 questId`
- `uint32 objectiveId`
- `int32 objectiveCount`
- `QuestState state`

**Helper Constructors:**
- `QuestEvent::QuestAccepted(ObjectGuid player, uint32 questId)`
- `QuestEvent::QuestCompleted(ObjectGuid player, uint32 questId)`
- `QuestEvent::ObjectiveProgress(ObjectGuid player, uint32 questId, uint32 objId, int32 count)`

**ClearUnitEvents:** Check `playerGuid == unitGuid`

### ResourceEventBus
**Fields:**
- `ObjectGuid playerGuid`
- `Powers powerType`
- `int32 amount`
- `int32 maxAmount`
- `bool isRegen`

**Helper Constructors:**
- `ResourceEvent::PowerChanged(ObjectGuid player, Powers type, int32 amt, int32 max)`
- `ResourceEvent::PowerRegen(ObjectGuid player, Powers type, int32 amt)`
- `ResourceEvent::PowerDrained(ObjectGuid player, Powers type, int32 amt)`

**ClearUnitEvents:** Check `playerGuid == unitGuid`

### SocialEventBus
**Fields:**
- `ObjectGuid playerGuid`
- `ObjectGuid targetGuid`
- `std::string message`
- `ChatMsg chatType`
- `Language language`

**Helper Constructors:**
- `SocialEvent::ChatReceived(ObjectGuid player, ObjectGuid target, std::string msg, ChatMsg type)`
- `SocialEvent::WhisperReceived(ObjectGuid player, ObjectGuid target, std::string msg)`
- `SocialEvent::GroupInvite(ObjectGuid player, ObjectGuid inviter)`

**ClearUnitEvents:** Check `playerGuid == unitGuid || targetGuid == unitGuid`

### AuctionEventBus
**Fields:**
- `ObjectGuid playerGuid`
- `uint32 auctionId`
- `uint32 itemEntry`
- `uint64 bidAmount`
- `uint64 buyoutAmount`
- `AuctionAction action`

**Helper Constructors:**
- `AuctionEvent::ItemListed(ObjectGuid player, uint32 auctionId, uint32 item, uint64 bid, uint64 buyout)`
- `AuctionEvent::BidPlaced(ObjectGuid player, uint32 auctionId, uint64 amount)`
- `AuctionEvent::AuctionWon(ObjectGuid player, uint32 auctionId, uint32 item)`

**ClearUnitEvents:** Check `playerGuid == unitGuid`

### NPCEventBus
**Fields:**
- `ObjectGuid playerGuid`
- `ObjectGuid npcGuid`
- `uint32 npcEntry`
- `NPCInteraction interactionType`
- `uint32 gossipMenuId`

**Helper Constructors:**
- `NPCEvent::NPCInteraction(ObjectGuid player, ObjectGuid npc, uint32 entry, NPCInteraction type)`
- `NPCEvent::GossipSelect(ObjectGuid player, ObjectGuid npc, uint32 menuId)`
- `NPCEvent::VendorInteraction(ObjectGuid player, ObjectGuid vendor, uint32 entry)`

**ClearUnitEvents:** Check `playerGuid == unitGuid`

### InstanceEventBus
**Fields:**
- `ObjectGuid playerGuid`
- `uint32 mapId`
- `uint32 instanceId`
- `Difficulty difficulty`
- `InstanceAction action`

**Helper Constructors:**
- `InstanceEvent::PlayerEntered(ObjectGuid player, uint32 mapId, uint32 instanceId, Difficulty diff)`
- `InstanceEvent::PlayerLeft(ObjectGuid player, uint32 mapId, uint32 instanceId)`
- `InstanceEvent::BossKilled(ObjectGuid player, uint32 mapId, uint32 bossEntry)`

**ClearUnitEvents:** Check `playerGuid == unitGuid`

## Implementation Checklist (Per Bus)

### Header File Checklist
- [ ] Copyright header (2024 TrinityCore)
- [ ] Include guards with correct event type name
- [ ] All required includes (Define.h, ObjectGuid.h, chrono, etc.)
- [ ] Namespace Playerbot opening/closing
- [ ] BotAI forward declaration
- [ ] Priority enum (CRITICAL=0, HIGH=1, MEDIUM=2, LOW=3, BATCH=4)
- [ ] Event struct with all event-specific fields
- [ ] Event struct: IsValid(), IsExpired(), ToString() declarations
- [ ] Event struct: 2-4 static helper constructors
- [ ] Event struct: operator< for priority queue
- [ ] EventBus class declaration with TC_GAME_API
- [ ] Static instance() method
- [ ] PublishEvent, Subscribe, SubscribeAll, Unsubscribe methods
- [ ] ProcessEvents, ProcessUnitEvents, ClearUnitEvents methods
- [ ] GetPendingEventCount, GetSubscriberCount methods
- [ ] DumpSubscribers, DumpEventQueue, GetQueueSnapshot methods
- [ ] Statistics struct with atomic counters
- [ ] GetStatistics() const method
- [ ] Private constructor/destructor
- [ ] Private DeliverEvent, ValidateEvent, CleanupExpiredEvents methods
- [ ] Private UpdateMetrics, LogEvent methods
- [ ] Private _eventQueue (priority_queue)
- [ ] Private _queueMutex (mutable mutex)
- [ ] Private _subscribers (unordered_map)
- [ ] Private _globalSubscribers (vector)
- [ ] Private _subscriberMutex (mutable mutex)
- [ ] Private constants (MAX_QUEUE_SIZE=10000, CLEANUP_INTERVAL=30000, MAX_SUBSCRIBERS_PER_EVENT=5000)
- [ ] Private _cleanupTimer, _metricsUpdateTimer
- [ ] Private _stats, _maxQueueSize

### Implementation File Checklist
- [ ] Copyright header (2024 TrinityCore)
- [ ] Include EventBus.h, BotAI.h, Player.h, Log.h, Timer.h, sstream
- [ ] Namespace Playerbot opening/closing
- [ ] Helper constructor 1 implementation
- [ ] Helper constructor 2 implementation
- [ ] Additional helper constructors (if applicable)
- [ ] IsValid() implementation with type range check
- [ ] IsExpired() implementation
- [ ] ToString() implementation with event-specific fields
- [ ] Constructor with stats.startTime and TC_LOG_INFO
- [ ] Destructor with stats logging
- [ ] instance() static local variable singleton
- [ ] PublishEvent with validation, queue size check, push, peak tracking, stats increment
- [ ] Subscribe with null check, lock, duplicate check, max subscriber check, push
- [ ] SubscribeAll with null check, lock, duplicate check, push
- [ ] Unsubscribe with null check, lock, erase from all subscribers
- [ ] ProcessEvents with cleanup timer, event extraction loop, delivery loop, metrics update
- [ ] ProcessUnitEvents (just calls ProcessEvents)
- [ ] ClearUnitEvents with event-specific guid check
- [ ] GetPendingEventCount with lock
- [ ] GetSubscriberCount with lock and accumulation
- [ ] DeliverEvent calling subscriber->OnXXXEvent(event) in try/catch
- [ ] ValidateEvent calling IsValid() && !IsExpired()
- [ ] CleanupExpiredEvents with expiry filtering
- [ ] UpdateMetrics with exponential moving average
- [ ] LogEvent with TC_LOG_TRACE
- [ ] DumpSubscribers with TC_LOG_INFO
- [ ] DumpEventQueue with TC_LOG_INFO
- [ ] GetQueueSnapshot with temp queue copy
- [ ] Statistics::Reset() implementation
- [ ] Statistics::ToString() implementation with uptime calculation

## Replacement Variables

For each event bus, replace these placeholders:

- `{EventType}` → Loot, Quest, Resource, Social, Auction, NPC, Instance
- `{EVENTTYPE_UPPER}` → LOOT, QUEST, RESOURCE, SOCIAL, AUCTION, NPC, INSTANCE
- `{eventtype_lower}` → loot, quest, resource, social, auction, npc, instance
- Event-specific fields from field mapping section
- Helper constructor parameters and logic

## Success Criteria

Each event bus implementation is complete when:
1. ✅ Header file matches template exactly (~160 lines)
2. ✅ Implementation file matches template exactly (~440 lines)
3. ✅ All checklist items verified
4. ✅ Event-specific fields correctly integrated
5. ✅ Helper constructors match event semantics
6. ✅ ClearUnitEvents uses correct guid field
7. ✅ DeliverEvent calls correct BotAI handler (On{EventType}Event)
8. ✅ Log category uses correct module name (module.playerbot.{eventtype_lower})
9. ✅ Compiles without errors
10. ✅ NO SHORTCUTS - full implementation

## Next Steps

Using this template, implement all 7 remaining event buses in order:
1. LootEventBus
2. QuestEventBus
3. ResourceEventBus
4. SocialEventBus
5. AuctionEventBus
6. NPCEventBus
7. InstanceEventBus

Each implementation should take the template, apply event-specific field mapping, and ensure all checklist items are complete.
