/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * Event-Driven Hostile Detection System
 * Replaces polling with reactive event processing
 */

#ifndef TRINITYCORE_HOSTILE_EVENT_BUS_H
#define TRINITYCORE_HOSTILE_EVENT_BUS_H

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "ObjectGuid.h"
#include <atomic>
#include <functional>
#include <memory>
#include <boost/lockfree/queue.hpp>
#include <folly/MPMCQueue.h>
#include <vector>

namespace Playerbot
{

// Event types for hostile state changes
enum class HostileEventType : uint8
{
    SPAWN,              // New hostile spawned
    DESPAWN,            // Hostile despawned/died
    AGGRO_GAINED,       // Hostile aggroed on player/bot
    AGGRO_LOST,         // Hostile lost aggro
    POSITION_UPDATE,    // Hostile moved significantly
    THREAT_CHANGE,      // Threat table updated
    COMBAT_START,       // Hostile entered combat
    COMBAT_END          // Hostile left combat
};

/**
 * Lightweight event structure (32 bytes)
 * Optimized for cache efficiency
 */
struct HostileEvent
{
    HostileEventType type;          // 1 byte
    uint8 priority;                  // 1 byte (0-255, higher = more important)
    uint16 zoneId;                   // 2 bytes
    uint32 timestamp;                // 4 bytes
    ObjectGuid hostileGuid;          // 16 bytes
    ObjectGuid targetGuid;           // 8 bytes (compressed if bot)

    bool IsHighPriority() const { return priority >= 200; }
};

/**
 * Lock-free event bus for hostile notifications
 * Uses MPMC queue for multi-producer, multi-consumer pattern
 */
class HostileEventBus
{
public:
    static HostileEventBus& Instance();

    // Producer interface (called from game events)
    void PublishEvent(const HostileEvent& event);
    void PublishSpawn(ObjectGuid hostile, uint32 zoneId);
    void PublishDespawn(ObjectGuid hostile, uint32 zoneId);
    void PublishAggro(ObjectGuid hostile, ObjectGuid target, uint32 zoneId);
    void PublishThreatChange(ObjectGuid hostile, ObjectGuid target, float threat);

    // Consumer interface (called by cache updater)
    bool TryConsumeEvent(HostileEvent& event);
    size_t ConsumeEvents(::std::vector<HostileEvent>& events, size_t maxCount = 100);

    // Subscription for specific zones
    using EventHandler = ::std::function<void(const HostileEvent&)>;
    void Subscribe(uint32 zoneId, EventHandler handler);
    void Unsubscribe(uint32 zoneId);

    // Statistics
    struct BusStats
    {
        uint64 totalEvents;
        uint64 eventsProcessed;
        uint64 eventsDropped;
        uint32 queueSize;
        uint32 subscriberCount;
    };

    BusStats GetStatistics() const;

private:
    HostileEventBus();
    ~HostileEventBus();

    // Lock-free MPMC queue (supports 10k events)
    folly::MPMCQueue<HostileEvent> _eventQueue;

    // Zone subscriptions (read-heavy, write-rare)
    mutable Playerbot::OrderedSharedMutex<Playerbot::LockOrder::BOT_AI_STATE> _subscriberMutex;
    ::std::unordered_map<uint32, ::std::vector<EventHandler>> _subscribers;

    // Statistics
    ::std::atomic<uint64> _totalEvents{0};
    ::std::atomic<uint64> _eventsProcessed{0};
    ::std::atomic<uint64> _eventsDropped{0};

    // Deleted operations
    HostileEventBus(const HostileEventBus&) = delete;
    HostileEventBus& operator=(const HostileEventBus&) = delete;
};

/**
 * Integration hooks for TrinityCore events
 */
class HostileEventHooks
{
public:
    // Called from Creature::Create
    static void OnCreatureSpawn(Creature* creature);

    // Called from Creature::RemoveFromWorld
    static void OnCreatureDespawn(Creature* creature);

    // Called from ThreatManager::AddThreat
    static void OnThreatUpdate(Unit* hostile, Unit* target, float threat);

    // Called from Unit::SetInCombatWith
    static void OnCombatStateChange(Unit* unit, bool inCombat);

    // Called from Unit::UpdatePosition
    static void OnPositionUpdate(Unit* unit, float x, float y, float z);

private:
    static bool IsRelevantHostile(Unit* unit);
    static uint32 GetUnitZone(Unit* unit);
};

} // namespace Playerbot

#endif // TRINITYCORE_HOSTILE_EVENT_BUS_H