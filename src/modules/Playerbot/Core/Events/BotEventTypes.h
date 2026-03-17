/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRINITYCORE_BOT_EVENT_TYPES_H
#define TRINITYCORE_BOT_EVENT_TYPES_H

#include "Define.h"
#include "Core/StateMachine/BotStateTypes.h" // For EventType enum
#include "ObjectGuid.h"
#include "Common.h"
#include "Timer.h"
#include "GameTime.h"  // PHASE 0 - Quick Win #3: For GameTime::GetGameTimeMS()
#include <string>
#include <functional>
#include <memory>
#include <chrono>
#include <variant>
#include <any>

namespace Playerbot
{
namespace Events
{
    // Forward declarations for Phase 4 implementation
    class BotEventSystem;
    class EventObserver;
    class EventDispatcher;
    class EventQueue;
    class EventFilter;
    class EventHistory;

    /**
     * @struct BotEvent
     * @brief Base event structure (Phase 1 skeleton, Phase 4 full implementation)
     *
     * This is a minimal implementation for Phase 1. Phase 4 will add:
     * - Event priority queues with weighted scheduling
     * - Event filtering and pattern matching
     * - Event batching for performance optimization
     * - Async event dispatch with thread pools
     * - Event correlation and causality tracking
     * - Event persistence and replay capabilities
     *
     * Note: EventDataVariant is defined in BotEventData.h and can be added
     * as a member when that header is included.
     */
    struct TC_GAME_API BotEvent
    {
        // Core event data
        StateMachine::EventType type;
        ObjectGuid sourceGuid;
        ObjectGuid targetGuid;
        uint64 timestamp;

        // Event payload (Phase 4 will use variant for type safety)
        ::std::string data;

        // Specialized event data (Phase 4 - type-safe event data)
        // NOTE: Use std::any for now to avoid circular dependency
        // Files that include BotEventData.h can cast this to EventDataVariant
        ::std::any eventData;

        // Event metadata (Phase 4 will expand)
        uint32 eventId = 0;           // Unique event identifier
        uint8 priority = 100;          // Event priority (0-255)
        bool processed = false;        // Processing status

        // Default constructor (required for std::vector)
        BotEvent()
            : type(StateMachine::EventType::BOT_CREATED)
            , sourceGuid(ObjectGuid::Empty)
            , targetGuid(ObjectGuid::Empty)
            , timestamp(GameTime::GetGameTimeMS())
            , data("")
        {
        }

        // Constructor for basic events
        BotEvent(StateMachine::EventType t, ObjectGuid source = ObjectGuid::Empty)
            : type(t)
            , sourceGuid(source)
            , targetGuid(ObjectGuid::Empty)
            , timestamp(GameTime::GetGameTimeMS())
            , data("")
        {
            static ::std::atomic<uint32> s_eventIdCounter{1};
            eventId = s_eventIdCounter.fetch_add(1, ::std::memory_order_relaxed);
        }

        // Constructor with target
        BotEvent(StateMachine::EventType t, ObjectGuid source, ObjectGuid target)
            : type(t)
            , sourceGuid(source)
            , targetGuid(target)
            , timestamp(GameTime::GetGameTimeMS())
            , data("")
        {
            static ::std::atomic<uint32> s_eventIdCounter{1};
            eventId = s_eventIdCounter.fetch_add(1, ::std::memory_order_relaxed);
        }

        virtual ~BotEvent() = default;

        // Helper methods for event classification
        bool IsLifecycleEvent() const { return static_cast<uint16_t>(type) < 32; }
        bool IsGroupEvent() const { return static_cast<uint16_t>(type) >= 32 && static_cast<uint16_t>(type) < 64; }
        bool IsCombatEvent() const { return static_cast<uint16_t>(type) >= 64 && static_cast<uint16_t>(type) < 96; }
        bool IsMovementEvent() const { return static_cast<uint16_t>(type) >= 96 && static_cast<uint16_t>(type) < 128; }
        bool IsQuestEvent() const { return static_cast<uint16_t>(type) >= 128 && static_cast<uint16_t>(type) < 160; }
        bool IsTradeEvent() const { return static_cast<uint16_t>(type) >= 160 && static_cast<uint16_t>(type) < 192; }
        bool IsLootEvent() const { return static_cast<uint16_t>(type) >= 200 && static_cast<uint16_t>(type) < 231; }
        bool IsAuraEvent() const { return static_cast<uint16_t>(type) >= 231 && static_cast<uint16_t>(type) < 261; }
        bool IsDeathEvent() const { return static_cast<uint16_t>(type) >= 261 && static_cast<uint16_t>(type) < 276; }
        bool IsInstanceEvent() const { return static_cast<uint16_t>(type) >= 276 && static_cast<uint16_t>(type) < 301; }
        bool IsPvPEvent() const { return static_cast<uint16_t>(type) >= 301 && static_cast<uint16_t>(type) < 321; }
        bool IsResourceEvent() const { return static_cast<uint16_t>(type) >= 321 && static_cast<uint16_t>(type) < 341; }
        bool IsWarWithinEvent() const { return static_cast<uint16_t>(type) >= 341 && static_cast<uint16_t>(type) < 371; }
        bool IsSocialEvent() const { return static_cast<uint16_t>(type) >= 371 && static_cast<uint16_t>(type) < 391; }
        bool IsEquipmentEvent() const { return static_cast<uint16_t>(type) >= 391 && static_cast<uint16_t>(type) < 411; }
        bool IsEnvironmentalEvent() const { return static_cast<uint16_t>(type) >= 411 && static_cast<uint16_t>(type) < 426; }
        bool IsCustomEvent() const { return static_cast<uint16_t>(type) >= 1000; }

        // Priority classification (for event dispatcher)
        bool IsCriticalEvent() const { return IsLootEvent() || IsAuraEvent() || IsDeathEvent() || IsResourceEvent(); }
        bool IsHighPriorityEvent() const { return IsInstanceEvent() || IsWarWithinEvent() || IsEnvironmentalEvent(); }
        bool IsMediumPriorityEvent() const { return IsPvPEvent() || IsEquipmentEvent(); }
        bool IsLowPriorityEvent() const { return IsSocialEvent(); }
    };

    /**
     * @brief Event callback function type
     *
     * Phase 4 will expand this to support:
     * - Priority-based callbacks with weighted execution
     * - Filter predicates for conditional execution
     * - Async execution with futures/promises
     * - Callback chaining and composition
     * - Error handling and retry policies
     */
    using EventCallback = ::std::function<void(const BotEvent&)>;

    /**
     * @brief Event filter predicate
     * Phase 4 will implement complex filtering logic
     */
    using EventPredicate = ::std::function<bool(const BotEvent&)>;

    /**
     * @brief Event observer interface (Phase 4 will implement)
     *
     * Phase 4 implementation will include:
     * - Subscription management
     * - Event pattern matching
     * - Priority-based notification
     * - Async event handling
     */
    class TC_GAME_API IEventObserver
    {
    public:
        virtual ~IEventObserver() = default;

        // Core observer methods
        virtual void OnEvent(const BotEvent& event) = 0;
        virtual bool ShouldReceiveEvent(const BotEvent& /*event*/) const { return true; }
        virtual uint8 GetObserverPriority() const { return 100; }
    };

    /**
     * @brief Specialized event types (Phase 1 type aliases, Phase 4 full classes)
     *
     * Phase 4 will convert these to full classes with:
     * - Type-safe event data
     * - Specialized event handlers
     * - Event-specific validation
     * - Custom serialization
     */

    // Group management events
    using GroupEvent = BotEvent;
    using GroupJoinEvent = BotEvent;
    using GroupLeaveEvent = BotEvent;
    using LeaderChangeEvent = BotEvent;

    // Combat events
    using CombatEvent = BotEvent;
    using DamageEvent = BotEvent;
    using HealEvent = BotEvent;
    using ThreatEvent = BotEvent;

    // Movement events
    using MovementEvent = BotEvent;
    using PositionUpdateEvent = BotEvent;
    using PathingEvent = BotEvent;
    using StuckEvent = BotEvent;

    // Quest events
    using QuestEvent = BotEvent;
    using QuestProgressEvent = BotEvent;
    using QuestCompleteEvent = BotEvent;

    // Trade and economy events
    using TradeEvent = BotEvent;
    using AuctionEvent = BotEvent;
    using MailEvent = BotEvent;

    /**
     * @brief Event priority constants
     * Used for event queue ordering in Phase 4
     */
    enum class EventPriority : uint8_t
    {
        LOWEST   = 0,
        LOW      = 50,
        NORMAL   = 100,
        HIGH     = 150,
        HIGHEST  = 200,
        CRITICAL = 255
    };

    /**
     * @brief Event processing result
     * Phase 4 will use for event handler feedback
     */
    enum class EventResult : uint8_t
    {
        HANDLED,           // Event was processed successfully
        NOT_HANDLED,       // Event was not processed by this handler
        CONSUME,           // Stop propagating event to other handlers
        ERROR_OCCURRED,    // PHASE 0 - Quick Win #3: Renamed from ERROR to avoid Windows.h macro conflict
        DEFER              // Defer processing to next update cycle
    };

} // namespace Events
} // namespace Playerbot

// Phase 4 TODO LIST:
// ==================
// 1. Implement BotEventSystem singleton class
// 2. Create EventDispatcher with priority queue management
// 3. Implement EventObserver registration and lifecycle
// 4. Add event filtering and pattern matching system
// 5. Create event batching for performance optimization
// 6. Implement async event processing with thread pools
// 7. Add event history tracking with circular buffers
// 8. Create event correlation and causality analysis
// 9. Implement event persistence for debugging/replay
// 10. Add performance metrics and event statistics
// 11. Create specialized event classes for each event type
// 12. Implement event serialization for network/storage
// 13. Add event validation and sanitization
// 14. Create event testing framework
// 15. Implement event replay and simulation capabilities

#endif // TRINITYCORE_BOT_EVENT_TYPES_H