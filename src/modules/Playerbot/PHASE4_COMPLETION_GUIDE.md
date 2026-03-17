# Phase 4 Event Handler Integration - Completion Guide

## Current Status: 7/11 Event Buses Complete (64%)

### ✅ Fully Implemented (7/11)
1. **GroupEventBus** - DeliverEvent calls `subscriber->OnGroupEvent(event)`
2. **CombatEventBus** - DeliverEvent calls `subscriber->OnCombatEvent(event)`
3. **CooldownEventBus** - Full pub/sub (~440 lines)
4. **AuraEventBus** - Full pub/sub (~440 lines)
5. **LootEventBus** - Full pub/sub (~600 lines)
6. **QuestEventBus** - Full pub/sub (~600 lines)
7. **ResourceEventBus** - Full pub/sub (~430 lines) ✅ JUST COMPLETED

### 🔄 Remaining (4/11)
8. **SocialEventBus** - Files exist, need template application
9. **AuctionEventBus** - Files exist, need template application
10. **NPCEventBus** - Files exist, need template application
11. **InstanceEventBus** - Files exist, need template application

## Quick Completion Instructions

Each remaining bus follows the EXACT same pattern. Use **PHASE4_EVENT_BUS_TEMPLATE.md** as your guide.

### Step-by-Step for Each Bus

#### 1. SocialEventBus

**Header Fields (from template):**
```cpp
struct SocialEvent
{
    SocialEventType type;
    SocialEventPriority priority;
    ObjectGuid playerGuid;
    ObjectGuid targetGuid;
    std::string message;
    ChatMsg chatType;
    Language language;
    std::chrono::steady_clock::time_point timestamp;
    std::chrono::steady_clock::time_point expiryTime;

    static SocialEvent ChatReceived(ObjectGuid player, ObjectGuid target, std::string msg, ChatMsg type);
    static SocialEvent WhisperReceived(ObjectGuid player, ObjectGuid target, std::string msg);
    static SocialEvent GroupInvite(ObjectGuid player, ObjectGuid inviter);
};
```

**ClearUnitEvents check:** `playerGuid == unitGuid || targetGuid == unitGuid`
**DeliverEvent calls:** `subscriber->OnSocialEvent(event)`

#### 2. AuctionEventBus

**Header Fields:**
```cpp
struct AuctionEvent
{
    AuctionEventType type;
    AuctionEventPriority priority;
    ObjectGuid playerGuid;
    uint32 auctionId;
    uint32 itemEntry;
    uint64 bidAmount;
    uint64 buyoutAmount;
    AuctionAction action;
    std::chrono::steady_clock::time_point timestamp;
    std::chrono::steady_clock::time_point expiryTime;

    static AuctionEvent ItemListed(ObjectGuid player, uint32 auctionId, uint32 item, uint64 bid, uint64 buyout);
    static AuctionEvent BidPlaced(ObjectGuid player, uint32 auctionId, uint64 amount);
    static AuctionEvent AuctionWon(ObjectGuid player, uint32 auctionId, uint32 item);
};
```

**ClearUnitEvents check:** `playerGuid == unitGuid`
**DeliverEvent calls:** `subscriber->OnAuctionEvent(event)`

#### 3. NPCEventBus

**Header Fields:**
```cpp
struct NPCEvent
{
    NPCEventType type;
    NPCEventPriority priority;
    ObjectGuid playerGuid;
    ObjectGuid npcGuid;
    uint32 npcEntry;
    NPCInteraction interactionType;
    uint32 gossipMenuId;
    std::chrono::steady_clock::time_point timestamp;
    std::chrono::steady_clock::time_point expiryTime;

    static NPCEvent NPCInteraction(ObjectGuid player, ObjectGuid npc, uint32 entry, NPCInteraction type);
    static NPCEvent GossipSelect(ObjectGuid player, ObjectGuid npc, uint32 menuId);
    static NPCEvent VendorInteraction(ObjectGuid player, ObjectGuid vendor, uint32 entry);
};
```

**ClearUnitEvents check:** `playerGuid == unitGuid`
**DeliverEvent calls:** `subscriber->OnNPCEvent(event)`

#### 4. InstanceEventBus

**Header Fields:**
```cpp
struct InstanceEvent
{
    InstanceEventType type;
    InstanceEventPriority priority;
    ObjectGuid playerGuid;
    uint32 mapId;
    uint32 instanceId;
    Difficulty difficulty;
    InstanceAction action;
    std::chrono::steady_clock::time_point timestamp;
    std::chrono::steady_clock::time_point expiryTime;

    static InstanceEvent PlayerEntered(ObjectGuid player, uint32 mapId, uint32 instanceId, Difficulty diff);
    static InstanceEvent PlayerLeft(ObjectGuid player, uint32 mapId, uint32 instanceId);
    static InstanceEvent BossKilled(ObjectGuid player, uint32 mapId, uint32 bossEntry);
};
```

**ClearUnitEvents check:** `playerGuid == unitGuid`
**DeliverEvent calls:** `subscriber->OnInstanceEvent(event)`

## Implementation Checklist (Per Bus)

Use this for each of the 4 remaining buses:

### Header File (.h)
- [ ] Read existing stub file
- [ ] Replace with template structure
- [ ] Add event-specific fields from above
- [ ] Add helper constructor declarations
- [ ] Ensure all includes (Define.h, ObjectGuid.h, chrono, queue, mutex, atomic)
- [ ] Verify TC_GAME_API on class declaration
- [ ] Check priority enum and operator< implementation

### Implementation File (.cpp)
- [ ] Read existing stub file
- [ ] Replace with template structure
- [ ] Implement 2-3 helper constructors with appropriate priorities
- [ ] Set expiry times (10-60 seconds based on priority)
- [ ] Implement IsValid(), IsExpired(), ToString()
- [ ] Implement full pub/sub infrastructure
- [ ] **CRITICAL**: DeliverEvent calls `subscriber->On{EventType}Event(event)`
- [ ] ClearUnitEvents uses correct guid field
- [ ] Log category: `module.playerbot.{eventtype_lower}`

### Verification
- [ ] Header ~160-175 lines
- [ ] Implementation ~430-440 lines
- [ ] All template checklist items complete
- [ ] NO SHORTCUTS - full implementation
- [ ] DeliverEvent handler name matches BotAI virtual method

## Template Reference Files

**Perfect Examples to Copy From:**
- `src/modules/Playerbot/Resource/ResourceEventBus.h` (just completed)
- `src/modules/Playerbot/Resource/ResourceEventBus.cpp` (just completed)
- `src/modules/Playerbot/Quest/QuestEventBus.h`
- `src/modules/Playerbot/Quest/QuestEventBus.cpp`

**Template Document:**
- `src/modules/Playerbot/PHASE4_EVENT_BUS_TEMPLATE.md`

## After Completing All 4 Buses

### 1. Update CMakeLists.txt

Add all new files to the appropriate CMakeLists.txt:

```cmake
# In src/modules/Playerbot/CMakeLists.txt

# Event Bus Implementations
${CMAKE_CURRENT_SOURCE_DIR}/Aura/AuraEventBus.cpp
${CMAKE_CURRENT_SOURCE_DIR}/Cooldown/CooldownEventBus.cpp
${CMAKE_CURRENT_SOURCE_DIR}/Loot/LootEventBus.cpp
${CMAKE_CURRENT_SOURCE_DIR}/Quest/QuestEventBus.cpp
${CMAKE_CURRENT_SOURCE_DIR}/Resource/ResourceEventBus.cpp
${CMAKE_CURRENT_SOURCE_DIR}/Social/SocialEventBus.cpp
${CMAKE_CURRENT_SOURCE_DIR}/Auction/AuctionEventBus.cpp
${CMAKE_CURRENT_SOURCE_DIR}/NPC/NPCEventBus.cpp
${CMAKE_CURRENT_SOURCE_DIR}/Instance/InstanceEventBus.cpp

# BotAI Event Handlers
${CMAKE_CURRENT_SOURCE_DIR}/AI/BotAI_EventHandlers.cpp
```

### 2. Test Compilation

```bash
cd build
cmake ..
make -j4  # or MSBuild on Windows
```

### 3. Final Commit

```bash
git add -A
git commit -m "[PlayerBot] Phase 4 COMPLETE: All 11 Event Buses Fully Implemented

MILESTONE: Phase 4 Event Handler Integration 100% Complete
- ✅ 11/11 Event Buses with full pub/sub infrastructure
- ✅ BotAI virtual handlers + default implementations
- ✅ Thread-safe subscription management
- ✅ Priority queue event processing
- ✅ Statistics tracking and diagnostics

Template-driven implementation ensures:
- NO SHORTCUTS - Full ~600 lines per bus
- Consistent architecture across all buses
- Enterprise-grade quality and performance
- Complete error handling and logging

Phase 4 Event Handler Integration: COMPLETE ✅"
```

## Success Criteria

Phase 4 is complete when:
- ✅ All 11 event buses have full pub/sub infrastructure
- ✅ Each bus ~600 lines (header + implementation)
- ✅ DeliverEvent calls appropriate BotAI virtual handler
- ✅ Thread-safe with mutex protection
- ✅ Priority queue processing
- ✅ Statistics tracking
- ✅ All files compile without errors
- ✅ CMakeLists.txt updated
- ✅ Final commit with comprehensive summary

## Estimated Time to Complete

- SocialEventBus: ~15 minutes (following template)
- AuctionEventBus: ~15 minutes (following template)
- NPCEventBus: ~15 minutes (following template)
- InstanceEventBus: ~15 minutes (following template)
- CMakeLists.txt: ~5 minutes
- Test compilation: ~10 minutes
- Final commit: ~5 minutes

**Total: ~1.5 hours to complete Phase 4**

The template makes it straightforward - just apply event-specific fields and verify the checklist!
