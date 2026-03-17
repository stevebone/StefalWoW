/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 */

#ifndef TRINITYCORE_ICOMBAT_EVENT_SUBSCRIBER_H
#define TRINITYCORE_ICOMBAT_EVENT_SUBSCRIBER_H

#include "CombatEventType.h"
#include "Define.h"
#include <cstdint>

namespace Playerbot {

struct CombatEvent;

/**
 * @class ICombatEventSubscriber
 * @brief Interface for combat event subscribers
 *
 * Phase 3 Architecture: Subscriber interface for event-driven combat system
 *
 * Components that want to receive combat events implement this interface
 * and register with CombatEventRouter.
 *
 * Features:
 * - Bitmask-based subscription for efficient filtering
 * - Priority ordering for event delivery
 * - Optional per-event filtering via ShouldReceiveEvent()
 *
 * Usage Example:
 * @code
 * class InterruptCoordinator  {
 * public:
 *     void OnCombatEvent(const CombatEvent& event) override {
 *         if (event.type == CombatEventType::SPELL_CAST_START) {
 *             HandleSpellCastStart(event);
 *         }
 *     }
 *
 *     CombatEventType GetSubscribedEventTypes() const override {
 *         return CombatEventType::SPELL_CAST_START |
 *                CombatEventType::SPELL_INTERRUPTED;
 *     }
 *
 *     int32 GetEventPriority() const override { return 100; }  // High priority
 * };
 *
 * // In initialization
 * CombatEventRouter::Instance().Subscribe(this);
 *
 * // In shutdown
 * CombatEventRouter::Instance().Unsubscribe(this);
 * @endcode
 *
 * Priority Guidelines:
 * - 200+: Critical systems (emergency responses)
 * - 100-199: High priority (interrupt coordination)
 * - 50-99: Normal priority (threat tracking)
 * - 0-49: Low priority (statistics, logging)
 * - <0: Background processing
 */
class ICombatEventSubscriber {
public:
    virtual ~ICombatEventSubscriber() = default;

    /**
     * @brief Called when a subscribed event occurs
     *
     * @param event The combat event data
     *
     * IMPORTANT:
     * - Keep handler fast (<1ms typical)
     * - Don't block or do heavy processing
     * - For heavy processing, queue work for later
     *
     * Thread Safety: Called from main thread (world update)
     */
    virtual void OnCombatEvent(const CombatEvent& event) = 0;

    /**
     * @brief Return bitmask of event types this subscriber wants
     *
     * @return Bitmask of CombatEventType values
     *
     * Use bitwise OR to combine multiple types:
     * @code
     * return CombatEventType::SPELL_CAST_START |
     *        CombatEventType::SPELL_INTERRUPTED |
     *        CombatEventType::DAMAGE_TAKEN;
     * @endcode
     *
     * Or use convenience masks:
     * @code
     * return CombatEventType::ALL_SPELL;  // All spell events
     * return CombatEventType::ALL_EVENTS; // All events
     * @endcode
     */
    virtual CombatEventType GetSubscribedEventTypes() const = 0;

    /**
     * @brief Priority for event delivery (higher = earlier)
     *
     * @return Priority value (default 0)
     *
     * Events are delivered to higher-priority subscribers first.
     * This allows critical systems (like interrupt coordination)
     * to process events before less time-sensitive systems.
     *
     * Priority Guidelines:
     * - 200+: Critical (emergency shutdown, safety)
     * - 100-199: High (interrupts, immediate responses)
     * - 50-99: Normal (threat, damage tracking)
     * - 0-49: Low (statistics, analytics)
     * - <0: Background (logging, history)
     */
    virtual int32 GetEventPriority() const { return 0; }

    /**
     * @brief Optional per-event filter
     *
     * @param event Event to check
     * @return true to receive event, false to skip
     *
     * Called after type filtering, before OnCombatEvent.
     * Use for filtering by source/target GUID, range, etc.
     *
     * Default returns true (receive all matching events).
     *
     * Example: Filter to only receive events for a specific bot:
     * @code
     * bool ShouldReceiveEvent(const CombatEvent& event) const override {
     *     return event.target == _myBotGuid || event.source == _myBotGuid;
     * }
     * @endcode
     *
     * Keep this fast - it's called for every matching event!
     */
    virtual bool ShouldReceiveEvent(const CombatEvent& event) const {
        (void)event; // Suppress unused parameter warning
        return true;
    }

    /**
     * @brief Get subscriber name for logging/debugging
     *
     * @return Human-readable name
     *
     * Override to provide meaningful identification in logs.
     */
    virtual const char* GetSubscriberName() const { return "Unknown"; }
};

} // namespace Playerbot

#endif // TRINITYCORE_ICOMBAT_EVENT_SUBSCRIBER_H
