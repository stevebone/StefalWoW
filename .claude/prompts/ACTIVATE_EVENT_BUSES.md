# ACTIVATE DEAD EVENT BUSES - ProcessEvents Integration

## CONTEXT

The Playerbot module has a fully built event-driven system based on `GenericEventBus<T>` (template in `Core/Events/GenericEventBus.h`). There are 12 domain-specific EventBus instances that receive events via `PublishEvent()` (126 total calls from packet parsers and hooks), and `BotAI` subscribes to all 12 at initialization and has implemented handlers for each (987 LOC in `AI/BotAI_EventHandlers.cpp`).

**THE BUG:** Only `GroupEventBus` and `BotSpawnEventBus` have their `ProcessEvents()` called. The other 12 buses accumulate events in their priority queues **forever** - events are never dequeued, handlers are never called, and memory grows without bound.

## WHAT TO DO

### Step 1: Add ProcessEvents calls in PlayerbotModule.cpp

In `PlayerbotModule.cpp`, find the existing call at approximately line 412:
```cpp
// Update GroupEventBus to process pending group events
Playerbot::GroupEventBus::instance()->ProcessEvents(diff);
```

**After** this line (and after its timing measurement), add ProcessEvents calls for the 12 missing buses. Use the `EventBus<T>::instance()->ProcessEvents()` calls **directly** (not via the wrapper classes - we'll remove those later).

Add the following includes at the top of PlayerbotModule.cpp (near the existing `#include "Group/GroupEventBus.h"`):
```cpp
#include "Core/Events/GenericEventBus.h"
#include "Combat/CombatEvents.h"
#include "Loot/LootEvents.h"
#include "Quest/QuestEvents.h"
#include "Aura/AuraEvents.h"
#include "Cooldown/CooldownEvents.h"
#include "Resource/ResourceEvents.h"
#include "Social/SocialEvents.h"
#include "Auction/AuctionEvents.h"
#include "NPC/NPCEvents.h"
#include "Instance/InstanceEvents.h"
#include "Professions/ProfessionEvents.h"
```

Then add the ProcessEvents block. Model it exactly like the existing GroupEventBus pattern but process all buses in one timing block:

```cpp
// ========================================================================
// Process Domain EventBus Queues
// These EventBuses receive typed events from packet parsers and hooks,
// and dispatch them to subscribed BotAI instances via HandleEvent().
// Without ProcessEvents(), events accumulate in queues indefinitely.
// ========================================================================
{
    using namespace Playerbot;
    uint32 totalDomainEvents = 0;
    totalDomainEvents += EventBus<CombatEvent>::instance()->ProcessEvents(50);
    totalDomainEvents += EventBus<LootEvent>::instance()->ProcessEvents(50);
    totalDomainEvents += EventBus<QuestEvent>::instance()->ProcessEvents(50);
    totalDomainEvents += EventBus<AuraEvent>::instance()->ProcessEvents(30);
    totalDomainEvents += EventBus<CooldownEvent>::instance()->ProcessEvents(30);
    totalDomainEvents += EventBus<ResourceEvent>::instance()->ProcessEvents(30);
    totalDomainEvents += EventBus<SocialEvent>::instance()->ProcessEvents(30);
    totalDomainEvents += EventBus<AuctionEvent>::instance()->ProcessEvents(20);
    totalDomainEvents += EventBus<NPCEvent>::instance()->ProcessEvents(30);
    totalDomainEvents += EventBus<InstanceEvent>::instance()->ProcessEvents(20);
    totalDomainEvents += EventBus<ProfessionEvent>::instance()->ProcessEvents(20);

    if (totalDomainEvents > 0)
    {
        TC_LOG_DEBUG("module.playerbot.events",
            "PlayerbotModule: Processed {} domain events this cycle", totalDomainEvents);
    }
}
```

Add timing measurement around this block following the existing pattern in the file (t6, t7, etc - adjust numbering as needed).

**Budget rationale for maxEvents per bus:**
- Combat/Loot/Quest: 50 (high volume, time-sensitive gameplay decisions)
- Aura/Cooldown/Resource/NPC/Social: 30 (medium volume)
- Auction/Instance/Profession: 20 (low volume, less time-sensitive)
- Total max per tick: ~340 events, sufficient for 100+ bots

### Step 2: Verify the Event type includes compile

The `EventBus<T>` template requires the full event struct definition. Each bus uses a typed event:

| Template Parameter | Defined In | Already Used By |
|---|---|---|
| `CombatEvent` | `Combat/CombatEvents.h` | ParseCombatPacket_Typed.cpp |
| `LootEvent` | `Loot/LootEvents.h` | ParseLootPacket_Typed.cpp |
| `QuestEvent` | `Quest/QuestEvents.h` | ParseQuestPacket_Typed.cpp |
| `AuraEvent` | `Aura/AuraEvents.h` | ParseAuraPacket_Typed.cpp |
| `CooldownEvent` | `Cooldown/CooldownEvents.h` | ParseCooldownPacket_Typed.cpp |
| `ResourceEvent` | `Resource/ResourceEvents.h` | ParseResourcePacket_Typed.cpp |
| `SocialEvent` | `Social/SocialEvents.h` | ParseSocialPacket_Typed.cpp |
| `AuctionEvent` | `Auction/AuctionEvents.h` | ParseAuctionPacket_Typed.cpp |
| `NPCEvent` | `NPC/NPCEvents.h` | ParseNPCPacket_Typed.cpp |
| `InstanceEvent` | `Instance/InstanceEvents.h` | ParseInstancePacket_Typed.cpp |
| `ProfessionEvent` | `Professions/ProfessionEvents.h` | ProfessionEventBus existing code |

If any include fails, check the actual header filename in that directory and adjust.

### Step 3: Build and verify

```bash
cmake --build build --target Playerbot 2>&1 | head -50
```

Fix any compile errors. Common issues:
- Wrong include paths (check actual filenames)
- Missing namespace qualifiers (all event types are in `namespace Playerbot`)
- Template instantiation needing full type definition

### Step 4: Verify no queue accumulation (optional diagnostic)

Add a periodic diagnostic log (e.g., every 60 seconds) that reports queue sizes for all buses. This helps verify events are being drained:

```cpp
// Every 60 seconds, log queue health
static uint32 s_lastQueueReport = 0;
if (GameTime::GetGameTimeMS() - s_lastQueueReport > 60000)
{
    s_lastQueueReport = GameTime::GetGameTimeMS();
    
    uint32 totalQueued = 0;
    totalQueued += EventBus<CombatEvent>::instance()->GetQueueSize();
    totalQueued += EventBus<LootEvent>::instance()->GetQueueSize();
    totalQueued += EventBus<QuestEvent>::instance()->GetQueueSize();
    totalQueued += EventBus<AuraEvent>::instance()->GetQueueSize();
    totalQueued += EventBus<CooldownEvent>::instance()->GetQueueSize();
    totalQueued += EventBus<ResourceEvent>::instance()->GetQueueSize();
    totalQueued += EventBus<SocialEvent>::instance()->GetQueueSize();
    totalQueued += EventBus<AuctionEvent>::instance()->GetQueueSize();
    totalQueued += EventBus<NPCEvent>::instance()->GetQueueSize();
    totalQueued += EventBus<InstanceEvent>::instance()->GetQueueSize();
    totalQueued += EventBus<ProfessionEvent>::instance()->GetQueueSize();

    if (totalQueued > 0)
    {
        TC_LOG_INFO("module.playerbot.events",
            "EventBus queue health: {} events pending across 11 domain buses", totalQueued);
    }
}
```

## WHAT NOT TO DO

- Do NOT modify GenericEventBus.h - it works correctly
- Do NOT modify BotAI_EventHandlers.cpp - the handlers are already implemented
- Do NOT modify the ParseXxxPacket_Typed.cpp files - the publishers work correctly
- Do NOT touch BotSpawnEventBus - it has its own internal ProcessEvents loop
- Do NOT touch HostileEventBus - it uses a completely different architecture (folly::MPMCQueue)
- Do NOT remove the EventBus wrapper classes yet (that's a separate task)
- Do NOT change the EventDispatcher system (separate event path for Manager-level events)

## SUCCESS CRITERIA

1. Clean compile
2. All 11 domain EventBuses have ProcessEvents() called every world update tick
3. Events published by packet parsers are actually delivered to BotAI::HandleEvent()
4. No unbounded queue growth (verify via diagnostic log after running with bots)

## FILES TO MODIFY

- `PlayerbotModule.cpp` (add includes + ProcessEvents block)

## ARCHITECTURE REFERENCE

```
TrinityCore Server Packets
    │
    ▼
ParseXxxPacket_Typed.cpp  ──PublishEvent()──►  EventBus<T>::_eventQueue
                                                       │
                                                  ProcessEvents()  ◄── THIS IS WHAT WE'RE ADDING
                                                       │
                                                       ▼
                                               BotAI::HandleEvent(T)
                                                       │
                                                       ▼
                                              BotAI::OnXxxEvent(T)
                                              (BotAI_EventHandlers.cpp)
```
