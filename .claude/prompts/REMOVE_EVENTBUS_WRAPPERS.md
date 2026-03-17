# REMOVE EVENTBUS WRAPPER CLASSES

## CONTEXT

The Playerbot module has 12 EventBus wrapper classes that are pure delegation layers over `EventBus<T>::instance()`. They add zero logic - every method just forwards to the template. These wrappers exist because the original plan had DI interfaces (now deleted) that needed concrete adapter classes.

Example of what every wrapper looks like (AuctionEventBus.h):
```cpp
class AuctionEventBus {
    static AuctionEventBus* instance() { static AuctionEventBus inst; return &inst; }
    bool PublishEvent(AuctionEvent const& event) {
        return EventBus<AuctionEvent>::instance()->PublishEvent(event);  // Just forwarding!
    }
    void Subscribe(BotAI* sub, std::vector<AuctionEventType> const& types) {
        EventBus<AuctionEvent>::instance()->Subscribe(sub, types);      // Just forwarding!
    }
    // ... every method is identical forwarding
};
```

## WHAT TO DO

### Transformation Rules

For each wrapper reference, apply these mechanical replacements:

| Old Pattern | New Pattern |
|---|---|
| `#include "Xxx/XxxEventBus.h"` | `#include "Core/Events/GenericEventBus.h"` + `#include "Xxx/XxxEvents.h"` |
| `XxxEventBus::instance()->PublishEvent(event)` | `EventBus<XxxEvent>::instance()->PublishEvent(event)` |
| `XxxEventBus::instance()->Subscribe(sub, types)` | `EventBus<XxxEvent>::instance()->Subscribe(sub, types)` |
| `XxxEventBus::instance()->SubscribeAll(this)` | See SubscribeAll pattern below |
| `XxxEventBus::instance()->Unsubscribe(sub)` | `EventBus<XxxEvent>::instance()->Unsubscribe(sub)` |
| `XxxEventBus::instance()->UnsubscribeByGuid(guid)` | `EventBus<XxxEvent>::instance()->UnsubscribeByGuid(guid)` |
| `XxxEventBus::instance()->GetQueueSize()` | `EventBus<XxxEvent>::instance()->GetQueueSize()` |
| `XxxEventBus::instance()->GetSubscriberCount()` | `EventBus<XxxEvent>::instance()->GetSubscriberCount()` |
| `XxxEventBus::instance()->GetTotalEventsPublished()` | `EventBus<XxxEvent>::instance()->GetTotalEventsPublished()` |
| `XxxEventBus::instance()->GetEventCount(type)` | `EventBus<XxxEvent>::instance()->GetEventCount(type)` |
| `XxxEventBus::instance()->SubscribeCallback(h, t)` | `EventBus<XxxEvent>::instance()->SubscribeCallback(h, t)` |
| `XxxEventBus::instance()->UnsubscribeCallback(id)` | `EventBus<XxxEvent>::instance()->UnsubscribeCallback(id)` |
| `XxxEventBus::instance()->ProcessEvents(n)` | `EventBus<XxxEvent>::instance()->ProcessEvents(n)` |

### SubscribeAll Pattern

Some wrappers have `SubscribeAll()` which subscribes to all event types. Replace with inline code:
```cpp
// OLD:
GroupEventBus::instance()->SubscribeAll(this);

// NEW:
{
    std::vector<GroupEventType> allTypes;
    for (uint8 i = 0; i < static_cast<uint8>(GroupEventType::MAX_GROUP_EVENT); ++i)
        allTypes.push_back(static_cast<GroupEventType>(i));
    EventBus<GroupEvent>::instance()->Subscribe(this, allTypes);
}
```

The MAX enum value name varies per event type. Check each XxxEvents.h header for the correct name:
- GroupEventType::MAX_GROUP_EVENT
- CombatEventType::MAX_COMBAT_EVENT  
- LootEventType::MAX_LOOT_EVENT
- etc. (pattern: MAX_XXX_EVENT)

If a SubscribeAll is used in multiple places for the same event type, consider creating a small helper function in the calling file rather than duplicating the loop.

### GroupEventBus Special Cases

GroupEventBus has two extra methods not in the template:

1. `ProcessEvents(uint32 diff, uint32 maxEvents = 100)` - The `diff` parameter is ignored. Replace with `EventBus<GroupEvent>::instance()->ProcessEvents(maxEvents)`.

2. `ClearGroupEvents(ObjectGuid groupGuid)` - Body is empty (comment says "Not implemented"). Replace calls with `// TODO: Per-group event clearing not implemented` or remove the call if it's clearly a no-op.

3. `ProcessGroupEvents(ObjectGuid groupGuid, uint32 diff)` - Just calls `ProcessEvents(diff, 50)`. Replace with `EventBus<GroupEvent>::instance()->ProcessEvents(50)`.

4. `GetPendingEventCount()` - Replace with `EventBus<GroupEvent>::instance()->GetQueueSize()`.

### Files to DELETE after migration

Delete these 12 wrapper header files:
1. `Auction/AuctionEventBus.h` (103 LOC)
2. `Aura/AuraEventBus.h` (106 LOC)
3. `Combat/CombatEventBus.h` (125 LOC)
4. `Cooldown/CooldownEventBus.h` (106 LOC)
5. `Group/GroupEventBus.h` (135 LOC)
6. `Instance/InstanceEventBus.h` (103 LOC)
7. `Loot/LootEventBus.h` (106 LOC)
8. `NPC/NPCEventBus.h` (103 LOC)
9. `Professions/ProfessionEventBus.h` (221 LOC)
10. `Quest/QuestEventBus.h` (106 LOC)
11. `Resource/ResourceEventBus.h` (106 LOC)
12. `Social/SocialEventBus.h` (106 LOC)

Total: ~1,526 LOC deleted

### Files to NOT touch

- `Lifecycle/BotSpawnEventBus.h` + `.cpp` - Own event system, NOT a GenericEventBus wrapper
- `AI/Combat/HostileEventBus.h` - Own architecture with folly::MPMCQueue
- `Core/Events/GenericEventBus.h` - The template itself, no changes needed
- `Core/Events/EventDispatcher.h/.cpp` - Separate event system for Manager-level events

### Reference Counts Per Wrapper (for progress tracking)

| Wrapper | References | Key Files |
|---|---|---|
| AuctionEventBus | 9 | ParseAuctionPacket_Typed.cpp, BotAI_EventHandlers.cpp |
| AuraEventBus | 7 | ParseAuraPacket_Typed.cpp, BotAI_EventHandlers.cpp |
| CombatEventBus | 13 | ParseCombatPacket_Typed.cpp, BotAI_EventHandlers.cpp, CombatStateManager.cpp |
| CooldownEventBus | 8 | ParseCooldownPacket_Typed.cpp, BotAI_EventHandlers.cpp |
| GroupEventBus | 49 | PlayerBotHooks.cpp, ParseGroupPacket_*.cpp, GroupEventHandler.cpp, PlayerbotModule.cpp |
| InstanceEventBus | 10 | ParseInstancePacket_Typed.cpp, BotAI_EventHandlers.cpp |
| LootEventBus | 12 | ParseLootPacket_Typed.cpp, BotAI_EventHandlers.cpp |
| NPCEventBus | 11 | ParseNPCPacket_Typed.cpp, BotAI_EventHandlers.cpp |
| ProfessionEventBus | 8 | BotAI_EventHandlers.cpp, ProfessionAuctionBridge |
| QuestEventBus | 18 | ParseQuestPacket_Typed.cpp, QuestCompletion.cpp, BotAI_EventHandlers.cpp |
| ResourceEventBus | 6 | ParseResourcePacket_Typed.cpp, BotAI_EventHandlers.cpp |
| SocialEventBus | 9 | ParseSocialPacket_Typed.cpp, BotAI_EventHandlers.cpp |
| **TOTAL** | **~160** | |

## APPROACH

Compiler-driven: Build → Fix errors → Repeat.

1. Start by deleting all 12 wrapper .h files
2. Build - compiler will show every file that included them
3. For each error: add correct includes + apply transformation rules above
4. Repeat until clean build

## INCLUDE MAPPING

When replacing wrapper includes, you need TWO includes:

| Old Include | New Include 1 (template) | New Include 2 (event type) |
|---|---|---|
| `"Auction/AuctionEventBus.h"` | `"Core/Events/GenericEventBus.h"` | `"Auction/AuctionEvents.h"` |
| `"Aura/AuraEventBus.h"` | `"Core/Events/GenericEventBus.h"` | `"Aura/AuraEvents.h"` |
| `"Combat/CombatEventBus.h"` | `"Core/Events/GenericEventBus.h"` | `"Combat/CombatEvents.h"` |
| `"Cooldown/CooldownEventBus.h"` | `"Core/Events/GenericEventBus.h"` | `"Cooldown/CooldownEvents.h"` |
| `"Group/GroupEventBus.h"` | `"Core/Events/GenericEventBus.h"` | `"Group/GroupEvents.h"` |
| `"Instance/InstanceEventBus.h"` | `"Core/Events/GenericEventBus.h"` | `"Instance/InstanceEvents.h"` |
| `"Loot/LootEventBus.h"` | `"Core/Events/GenericEventBus.h"` | `"Loot/LootEvents.h"` |
| `"NPC/NPCEventBus.h"` | `"Core/Events/GenericEventBus.h"` | `"NPC/NPCEvents.h"` |
| `"Professions/ProfessionEventBus.h"` | `"Core/Events/GenericEventBus.h"` | `"Professions/ProfessionEvents.h"` |
| `"Quest/QuestEventBus.h"` | `"Core/Events/GenericEventBus.h"` | `"Quest/QuestEvents.h"` |
| `"Resource/ResourceEventBus.h"` | `"Core/Events/GenericEventBus.h"` | `"Resource/ResourceEvents.h"` |
| `"Social/SocialEventBus.h"` | `"Core/Events/GenericEventBus.h"` | `"Social/SocialEvents.h"` |

NOTE: If a file already includes `GenericEventBus.h`, don't add it again. Many files may already have it transitively.

## SUCCESS CRITERIA

1. Clean compile
2. All 12 wrapper .h files deleted
3. No references to deleted wrapper class names remain (except comments)
4. All EventBus calls go directly through `EventBus<T>::instance()`
