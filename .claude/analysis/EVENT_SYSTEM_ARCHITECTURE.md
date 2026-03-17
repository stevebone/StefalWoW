# Event System Architecture - EventDispatcher vs GenericEventBus

**Status**: Active Architecture
**Created**: 2025-02-05
**Purpose**: Clarify the distinction between two complementary event systems

---

## Executive Summary

The Playerbot module uses **TWO DISTINCT EVENT SYSTEMS** that serve different architectural purposes:

| System | Purpose | Event Types | Processing Location |
|--------|---------|-------------|---------------------|
| **EventDispatcher** | Manager-level state machine events | `StateMachine::EventType` (450+ types) | `GameSystemsManager::UpdateManagers()` |
| **GenericEventBus<T>** | Domain-level granular game mechanics | 12 domain-specific types | `PlayerbotModule::Update()` |

**CRITICAL**: These are **NOT interchangeable** - they operate at different architectural layers with different event types.

---

## System 1: EventDispatcher (Manager-Level Events)

### Purpose
Centralized event routing system for **high-level state machine transitions** and **manager coordination** (Phase 7 architecture).

### Event Type
Uses `BotEvent` with `StateMachine::EventType` enum (450+ event types defined in `BotStateTypes.h`).

### Architecture
```
BotEvent (StateMachine::EventType)
    ↓
EventDispatcher::Dispatch()
    ↓
EventDispatcher::ProcessQueue(100)  [in GameSystemsManager]
    ↓
IManagerBase::OnEvent()  [specific managers subscribe]
```

### Event Categories
- **Lifecycle**: BOT_CREATED, BOT_LOGIN, PLAYER_LEVEL_UP
- **Group Management**: GROUP_JOINED, GROUP_LEFT, LEADER_CHANGED
- **Trade/Economy**: TRADE_INITIATED, AUCTION_WON, GOLD_SPENT
- **Combat (High-Level)**: COMBAT_STARTED, DAMAGE_TAKEN
- **Queue Management**: BG_QUEUE_SHORTAGE, LFG_QUEUE_SHORTAGE
- **Instance/PvP**: BOSS_ENGAGED, ARENA_MATCH_STARTED

### Current Subscribers
1. **TradeManager** (11 events):
   - TRADE_INITIATED, TRADE_ACCEPTED, TRADE_CANCELLED
   - TRADE_ITEM_ADDED, TRADE_GOLD_ADDED
   - GOLD_RECEIVED, GOLD_SPENT, LOW_GOLD_WARNING
   - VENDOR_PURCHASE, VENDOR_SALE, REPAIR_COST

2. **AuctionManager** (5 events):
   - AUCTION_BID_PLACED, AUCTION_WON, AUCTION_OUTBID
   - AUCTION_EXPIRED, AUCTION_SOLD

3. **CombatStateManager** (2 events):
   - DAMAGE_TAKEN
   - GROUP_MEMBER_ATTACKED

### Active Dispatch Call Sites (5 total)
```cpp
// 1. BotAI.cpp:873 - Group join detection
Events::BotEvent evt(StateMachine::EventType::GROUP_JOINED, bot->GetGUID());
eventDispatcher->Dispatch(std::move(evt));

// 2. BotAI.cpp:888 - Group leave detection
Events::BotEvent evt(StateMachine::EventType::GROUP_LEFT, bot->GetGUID());
eventDispatcher->Dispatch(std::move(evt));

// 3. BotSession.cpp:2468 - Reboot group detection
_ai->GetEventDispatcher()->Dispatch(std::move(evt));

// 4. PlayerbotEventScripts.cpp:126 - Generic script events
dispatcher->Dispatch(event);

// 5. (No other active dispatches found - system is intentionally minimal)
```

### Processing
```cpp
// GameSystemsManager.cpp:474
if (_eventDispatcher)
{
    uint32 eventsProcessed = _eventDispatcher->ProcessQueue(100);

    if (eventsProcessed > 0)
        TC_LOG_TRACE("Processed {} events this cycle", eventsProcessed);
}
```

### Subscription Pattern
```cpp
// GameSystemsManager.cpp - SubscribeManagersToEvents()
if (_tradeManager && _eventDispatcher)
{
    _eventDispatcher->Subscribe(StateMachine::EventType::TRADE_INITIATED, _tradeManager.get());
    _eventDispatcher->Subscribe(StateMachine::EventType::TRADE_ACCEPTED, _tradeManager.get());
    // ... 9 more subscriptions
}
```

### When to Use EventDispatcher
Use EventDispatcher when you need to:
- ✅ Notify managers of **state machine transitions** (bot joined group, combat started)
- ✅ Coordinate **manager behavior** based on lifecycle events
- ✅ Handle **high-level game events** (auction won, quest turned in)
- ✅ Trigger **cross-manager coordination** (trade initiated affects multiple systems)

### When NOT to Use EventDispatcher
Do NOT use EventDispatcher for:
- ❌ Granular combat mechanics (spell cast, damage dealt to specific target)
- ❌ Domain-specific events with complex data (loot roll with item info, group marker with position)
- ❌ High-frequency events (aura applications, resource changes)
- ❌ BotAI-level event subscriptions (use GenericEventBus instead)

---

## System 2: GenericEventBus<T> (Domain-Level Events)

### Purpose
Type-safe, domain-specific event broadcasting system for **granular game mechanics** at the **BotAI subscription level** (Phase 11 architecture).

### Event Types
**12 separate domain-specific event buses**, each with its own event type enum:

1. **CombatEvent** (`CombatEventType`) - Granular combat mechanics
2. **GroupEvent** (`GroupEventType`) - Group membership and markers
3. **LootEvent** (`LootEventType`) - Loot rolls and distribution
4. **QuestEvent** (`QuestEventType`) - Quest progress and objectives
5. **AuraEvent** (`AuraEventType`) - Buff/debuff applications
6. **CooldownEvent** (`CooldownEventType`) - Spell cooldown tracking
7. **ResourceEvent** (`ResourceEventType`) - Mana/rage/energy changes
8. **SocialEvent** (`SocialEventType`) - Chat and social interactions
9. **AuctionEvent** (`AuctionEventType`) - Auction house operations
10. **NPCEvent** (`NPCEventType`) - NPC interactions
11. **InstanceEvent** (`InstanceEventType`) - Instance-specific mechanics
12. **ProfessionEvent** (`ProfessionEventType`) - Profession automation

### Architecture
```
Domain-Specific Event (e.g., CombatEvent)
    ↓
EventBus<CombatEvent>::instance()->PublishEvent()
    ↓
EventBus<CombatEvent>::instance()->ProcessEvents(50)  [in PlayerbotModule]
    ↓
IEventSubscriber::OnEvent()  [BotAI and other subscribers]
```

### Event Categories (Example: CombatEvent)
```cpp
enum class CombatEventType : uint8_t
{
    COMBAT_START = 0,
    COMBAT_END,
    DAMAGE_DEALT,        // Different from StateMachine::DAMAGE_TAKEN!
    HEAL_CAST,
    SPELL_CAST_SUCCESS,
    SPELL_INTERRUPTED,
    THREAT_CHANGED,
    TARGET_SWITCHED,
    // ... granular combat mechanics
    MAX_COMBAT_EVENT
};
```

### Current Subscribers
- **BotAI** subscribes to **ALL 12 domain buses** with `SubscribeAll()` pattern
- Each BotAI instance receives ALL events from ALL 12 buses

### Active Publish Call Sites
```cpp
// Network packet parsers (11 files)
// Network/ParseGroupPacket_Typed.cpp
GroupEvent event;
event.type = GroupEventType::READY_CHECK_STARTED;
EventBus<GroupEvent>::instance()->PublishEvent(event);

// Quest system
// Quest/QuestCompletion.cpp
QuestEvent questEvent;
questEvent.type = QuestEventType::QUEST_COMPLETED;
EventBus<QuestEvent>::instance()->PublishEvent(questEvent);

// Profession bridges
// Professions/ProfessionAuctionBridge.cpp
EventBus<ProfessionEvent>::instance()->SubscribeCallback(
    ProfessionEventType::CRAFTING_STARTED, callback);
```

### Processing
```cpp
// PlayerbotModule.cpp - Domain EventBus processing
uint32 totalDomainEvents = 0;
{
    using namespace Playerbot;
    totalDomainEvents += EventBus<CombatEvent>::instance()->ProcessEvents(50);
    totalDomainEvents += EventBus<LootEvent>::instance()->ProcessEvents(50);
    totalDomainEvents += EventBus<QuestEvent>::instance()->ProcessEvents(50);
    totalDomainEvents += EventBus<AuraEvent>::instance()->ProcessEvents(25);
    totalDomainEvents += EventBus<CooldownEvent>::instance()->ProcessEvents(25);
    totalDomainEvents += EventBus<ResourceEvent>::instance()->ProcessEvents(25);
    totalDomainEvents += EventBus<SocialEvent>::instance()->ProcessEvents(20);
    totalDomainEvents += EventBus<AuctionEvent>::instance()->ProcessEvents(20);
    totalDomainEvents += EventBus<NPCEvent>::instance()->ProcessEvents(20);
    totalDomainEvents += EventBus<InstanceEvent>::instance()->ProcessEvents(30);
    totalDomainEvents += EventBus<GroupEvent>::instance()->ProcessEvents(30);
    totalDomainEvents += EventBus<ProfessionEvent>::instance()->ProcessEvents(20);
}
```

### Subscription Pattern
```cpp
// BotAI_EventHandlers.cpp - Subscribe to ALL events in a domain
{
    std::vector<CombatEventType> allCombatTypes;
    for (uint8 i = 0; i < static_cast<uint8>(CombatEventType::MAX_COMBAT_EVENT); ++i)
        allCombatTypes.push_back(static_cast<CombatEventType>(i));
    EventBus<CombatEvent>::instance()->Subscribe(this, allCombatTypes);
}

// Repeat for all 12 domain buses
```

### When to Use GenericEventBus
Use GenericEventBus when you need to:
- ✅ Broadcast **granular game mechanics** (spell cast on specific target, loot item rolled)
- ✅ Publish **domain-specific events** with complex data structures
- ✅ Allow **BotAI-level subscriptions** (each bot subscribes individually)
- ✅ Handle **high-frequency events** (aura applications, resource changes)
- ✅ Enable **type-safe event handling** with compile-time checks

### When NOT to Use GenericEventBus
Do NOT use GenericEventBus for:
- ❌ State machine transitions (use EventDispatcher)
- ❌ Manager-level coordination (use EventDispatcher)
- ❌ Lifecycle events (use EventDispatcher)
- ❌ Events that need to route to specific managers only (use EventDispatcher)

---

## Critical Differences

### Event Type Comparison

**THESE ARE DIFFERENT EVENTS:**

| EventDispatcher | GenericEventBus | Difference |
|----------------|-----------------|------------|
| `StateMachine::EventType::GROUP_JOINED` | `GroupEventType::MEMBER_JOINED` | State transition vs. domain event |
| `StateMachine::EventType::DAMAGE_TAKEN` | `CombatEventType::DAMAGE_DEALT` | Bot received damage vs. bot dealt damage |
| `StateMachine::EventType::COMBAT_STARTED` | `CombatEventType::COMBAT_START` | High-level state vs. granular mechanic |
| `StateMachine::EventType::TRADE_INITIATED` | N/A (no Trade domain bus) | Manager-level only |

### Architectural Layer Comparison

```
┌─────────────────────────────────────────────────────────────┐
│ MANAGER LAYER (Phase 7)                                     │
│                                                              │
│  EventDispatcher                                            │
│  ├─ StateMachine::EventType (450+ types)                    │
│  ├─ IManagerBase subscribers (3 managers)                   │
│  ├─ ProcessQueue(100) in GameSystemsManager                 │
│  └─ Use case: Manager coordination, state transitions       │
│                                                              │
└─────────────────────────────────────────────────────────────┘
                           ▼
┌─────────────────────────────────────────────────────────────┐
│ DOMAIN LAYER (Phase 11)                                     │
│                                                              │
│  GenericEventBus<T>                                         │
│  ├─ 12 domain-specific event types                          │
│  ├─ IEventSubscriber subscribers (BotAI + others)           │
│  ├─ ProcessEvents(budget) in PlayerbotModule                │
│  └─ Use case: Granular game mechanics, BotAI subscriptions  │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

## Usage Guidelines

### Scenario 1: Bot Joins a Group

**EventDispatcher** (State Machine Transition):
```cpp
// BotAI.cpp - Notify managers of state change
Events::BotEvent evt(StateMachine::EventType::GROUP_JOINED, bot->GetGUID());
eventDispatcher->Dispatch(std::move(evt));

// TradeManager receives event and adjusts behavior:
// - Enable group gold sharing
// - Adjust vendor purchase logic for group needs
```

**GenericEventBus** (Domain Events):
```cpp
// Network/ParseGroupPacket_Typed.cpp - Notify subscribers of group details
GroupEvent event;
event.type = GroupEventType::MEMBER_JOINED;
event.memberGuid = newMemberGuid;
event.memberName = "PlayerName";
EventBus<GroupEvent>::instance()->PublishEvent(event);

// BotAI receives event and updates group roster cache
```

**Both are needed** - they serve different purposes at different layers!

### Scenario 2: Bot Takes Damage

**EventDispatcher** (State Machine Event):
```cpp
// Notify CombatStateManager of damage taken (state transition)
Events::BotEvent evt(StateMachine::EventType::DAMAGE_TAKEN, attackerGuid);
evt.data = std::to_string(damage) + ":" + std::to_string(absorbed);
eventDispatcher->Dispatch(std::move(evt));

// CombatStateManager triggers EnterCombatWith(attacker)
```

**GenericEventBus** (Domain Event):
```cpp
// Broadcast granular combat details to all subscribers
CombatEvent combatEvent;
combatEvent.type = CombatEventType::DAMAGE_RECEIVED;
combatEvent.sourceGuid = attackerGuid;
combatEvent.targetGuid = botGuid;
combatEvent.spellId = spellId;
combatEvent.damageAmount = damage;
combatEvent.schoolMask = schoolMask;
EventBus<CombatEvent>::instance()->PublishEvent(combatEvent);

// BotAI updates threat calculations, AI decision making
```

**Both are needed** - EventDispatcher for state management, GenericEventBus for combat AI!

### Scenario 3: Auction Won

**EventDispatcher ONLY**:
```cpp
// High-level manager notification
Events::BotEvent evt(StateMachine::EventType::AUCTION_WON, ObjectGuid::Empty);
evt.data = std::to_string(auctionId);
eventDispatcher->Dispatch(std::move(evt));

// AuctionManager adjusts bidding strategy and gold tracking
```

**GenericEventBus** (AuctionEvent):
```cpp
// IF we need granular auction details for BotAI subscriptions
AuctionEvent auctionEvent;
auctionEvent.type = AuctionEventType::AUCTION_WON;
auctionEvent.auctionId = auctionId;
auctionEvent.itemEntry = itemEntry;
auctionEvent.bidAmount = bidAmount;
EventBus<AuctionEvent>::instance()->PublishEvent(auctionEvent);

// BotAI can react if needed (currently AuctionEventBus has no subscribers)
```

**Typically only EventDispatcher** - AuctionManager handles auction logic, BotAI doesn't need granular details.

---

## Common Mistakes to Avoid

### ❌ WRONG: Using EventDispatcher for granular combat
```cpp
// DON'T DO THIS - EventDispatcher is not for high-frequency granular events
Events::BotEvent evt(StateMachine::EventType::SPELL_CAST_SUCCESS, targetGuid);
evt.data = std::to_string(spellId);
eventDispatcher->Dispatch(std::move(evt));  // WRONG LAYER!
```

**✅ CORRECT: Use GenericEventBus**
```cpp
CombatEvent combatEvent;
combatEvent.type = CombatEventType::SPELL_CAST_SUCCESS;
combatEvent.spellId = spellId;
combatEvent.targetGuid = targetGuid;
EventBus<CombatEvent>::instance()->PublishEvent(combatEvent);
```

### ❌ WRONG: Using GenericEventBus for state transitions
```cpp
// DON'T DO THIS - GenericEventBus is not for manager coordination
GroupEvent event;
event.type = GroupEventType::GROUP_DISBANDED;  // This is a domain event!
EventBus<GroupEvent>::instance()->PublishEvent(event);

// TradeManager won't receive this - it subscribes to EventDispatcher!
```

**✅ CORRECT: Use EventDispatcher**
```cpp
Events::BotEvent evt(StateMachine::EventType::GROUP_DISBANDED, botGuid);
eventDispatcher->Dispatch(std::move(evt));
// TradeManager receives event and clears group-related state
```

### ❌ WRONG: Confusing similar event names
```cpp
// These are DIFFERENT events at DIFFERENT layers!

// Manager-level state machine event (EventDispatcher)
StateMachine::EventType::GROUP_JOINED  // "Bot joined a group"

// Domain-level membership event (GenericEventBus)
GroupEventType::MEMBER_JOINED          // "A player joined the group"

// They have different meanings and different subscribers!
```

---

## Performance Considerations

### EventDispatcher
- **Low frequency**: Only 5 active Dispatch calls in entire codebase
- **Budget**: ProcessQueue(100) events per update
- **Minimal overhead**: Designed for state transitions, not high-frequency events

### GenericEventBus
- **Medium-to-high frequency**: Packet parsers, quest updates, combat events
- **Budget**: Per-domain budgets (20-50 events per bus per update)
- **12 separate queues**: Each domain bus has independent processing

### Total Event Budget per Frame
```
EventDispatcher:        100 events/frame
GenericEventBus (12):   ~350 events/frame total
  - CombatEvent:         50
  - LootEvent:           50
  - QuestEvent:          50
  - AuraEvent:           25
  - CooldownEvent:       25
  - ResourceEvent:       25
  - SocialEvent:         20
  - AuctionEvent:        20
  - NPCEvent:            20
  - InstanceEvent:       30
  - GroupEvent:          30
  - ProfessionEvent:     20
```

---

## Migration Guidance

### Question: Should EventDispatcher be consolidated into GenericEventBus?

**Answer: NO - Keep both systems separate.**

**Reasons:**
1. **Different event types**: StateMachine::EventType (450+ types) vs. 12 domain-specific types
2. **Different layers**: Manager-level vs. Domain-level
3. **Different subscribers**: IManagerBase vs. IEventSubscriber interfaces
4. **Different purposes**: State coordination vs. granular mechanics
5. **Phase architecture**: EventDispatcher is Phase 7, GenericEventBus is Phase 11
6. **Minimal overhead**: Only 5 Dispatch calls - no performance reason to consolidate

### If You Need to Add a New Event

**Use EventDispatcher IF:**
- Event represents a **state machine transition** (bot logged in, group joined)
- Event needs to **coordinate manager behavior** (all managers adjust to new state)
- Event is **lifecycle-related** (bot created, destroyed, teleported)
- Event should route to **specific managers only** (TradeManager, AuctionManager)

**Use GenericEventBus IF:**
- Event represents **granular game mechanics** (spell cast, item looted)
- Event contains **domain-specific data** (spell ID, item entry, position)
- Event should broadcast to **all BotAI subscribers**
- Event is **high-frequency** (>10 per second per bot)
- Event fits into existing **domain categories** (Combat, Loot, Quest, etc.)

**Create New Domain Bus IF:**
- You have a **new domain** not covered by existing 12 buses
- You need **50+ granular events** for this domain
- Domain has **complex event data structures**
- Domain needs **independent event budget** for performance

---

## Code References

### EventDispatcher Files
- **Definition**: `src/modules/Playerbot/Core/Events/EventDispatcher.h`
- **Event Types**: `src/modules/Playerbot/Core/StateMachine/BotStateTypes.h` (StateMachine::EventType)
- **Event Data**: `src/modules/Playerbot/Core/Events/BotEventTypes.h` (BotEvent struct)
- **Processing**: `src/modules/Playerbot/Core/Managers/GameSystemsManager.cpp:474`
- **Subscriptions**: `src/modules/Playerbot/Core/Managers/GameSystemsManager.cpp:393`

### GenericEventBus Files
- **Definition**: `src/modules/Playerbot/Core/Events/GenericEventBus.h`
- **Event Types**: `src/modules/Playerbot/*/Events.h` (12 domain-specific headers)
- **Processing**: `src/modules/Playerbot/PlayerbotModule.cpp` (ProcessEvents block)
- **Subscriptions**: `src/modules/Playerbot/AI/BotAI_EventHandlers.cpp`
- **Publishing**: `src/modules/Playerbot/Network/Parse*Packet_Typed.cpp` (11 files)

---

## Conclusion

**EventDispatcher** and **GenericEventBus** are **complementary systems** operating at different architectural layers:

- **EventDispatcher**: Manager-level state machine coordination (Phase 7)
- **GenericEventBus**: Domain-level granular game mechanics (Phase 11)

**DO NOT consolidate** - they serve fundamentally different purposes with different event types, subscribers, and architectural responsibilities.

When implementing new features, **use both systems appropriately**:
- State transitions and manager coordination → EventDispatcher
- Granular game mechanics and BotAI subscriptions → GenericEventBus

---

**Questions?** See `GameSystemsManager.cpp` for EventDispatcher usage examples, or `BotAI_EventHandlers.cpp` for GenericEventBus subscription patterns.
