# Group Event System - BotAI Integration Guide

**Purpose**: This guide explains how to integrate the Group Event System with BotAI when it becomes available.

---

## 📋 Prerequisites

Before integrating, ensure:
- ✅ Group Event System is compiled (GroupEventBus, GroupStateMachine, GroupEventHandler, PlayerbotGroupScript)
- ✅ BotAI class exists with Update() method
- ✅ Bots are managed by BotSpawner or similar lifecycle manager

---

## 🔧 Integration Steps

### Step 1: Subscribe to Group Events (BotAI Constructor)

Add event subscriptions in `BotAI::BotAI()` constructor:

```cpp
// In BotAI.cpp constructor
#include "Group/GroupEventBus.h"

BotAI::BotAI(Player* bot) : _bot(bot)
{
    // ... existing initialization ...

    // Subscribe to group events
    auto eventBus = Playerbot::GroupEventBus::instance();

    // Subscribe to member join/leave events
    eventBus->Subscribe(_bot->GetGUID(), Playerbot::GroupEventType::MEMBER_JOINED,
        [this](Playerbot::GroupEvent const& event) {
            OnGroupMemberJoined(event);
        });

    eventBus->Subscribe(_bot->GetGUID(), Playerbot::GroupEventType::MEMBER_LEFT,
        [this](Playerbot::GroupEvent const& event) {
            OnGroupMemberLeft(event);
        });

    // Subscribe to leader change events
    eventBus->Subscribe(_bot->GetGUID(), Playerbot::GroupEventType::LEADER_CHANGED,
        [this](Playerbot::GroupEvent const& event) {
            OnGroupLeaderChanged(event);
        });

    // Subscribe to loot method changes
    eventBus->Subscribe(_bot->GetGUID(), Playerbot::GroupEventType::LOOT_METHOD_CHANGED,
        [this](Playerbot::GroupEvent const& event) {
            OnLootMethodChanged(event);
        });

    // Subscribe to raid conversion
    eventBus->Subscribe(_bot->GetGUID(), Playerbot::GroupEventType::RAID_CONVERTED,
        [this](Playerbot::GroupEvent const& event) {
            OnRaidConverted(event);
        });

    // Subscribe to difficulty changes
    eventBus->Subscribe(_bot->GetGUID(), Playerbot::GroupEventType::DIFFICULTY_CHANGED,
        [this](Playerbot::GroupEvent const& event) {
            OnDifficultyChanged(event);
        });

    // Add more subscriptions as needed...
}
```

### Step 2: Unsubscribe on Destruction (BotAI Destructor)

```cpp
// In BotAI.cpp destructor
BotAI::~BotAI()
{
    // Unsubscribe from all events
    Playerbot::GroupEventBus::instance()->UnsubscribeAll(_bot->GetGUID());

    // ... existing cleanup ...
}
```

### Step 3: Enable Per-Bot Group Polling (BotAI::Update)

```cpp
// In BotAI::Update()
#include "Group/PlayerbotGroupScript.h"

void BotAI::Update(uint32 diff)
{
    // ... existing update logic ...

    // Poll group state changes (if bot is in a group)
    if (Group* group = _bot->GetGroup())
    {
        Playerbot::PlayerbotGroupScript::PollGroupStateChanges(group, diff);
    }

    // ... rest of update logic ...
}
```

### Step 4: Implement Event Handler Methods

Add handler methods to BotAI class:

```cpp
// In BotAI.h
class BotAI
{
private:
    // Group event handlers
    void OnGroupMemberJoined(Playerbot::GroupEvent const& event);
    void OnGroupMemberLeft(Playerbot::GroupEvent const& event);
    void OnGroupLeaderChanged(Playerbot::GroupEvent const& event);
    void OnLootMethodChanged(Playerbot::GroupEvent const& event);
    void OnRaidConverted(Playerbot::GroupEvent const& event);
    void OnDifficultyChanged(Playerbot::GroupEvent const& event);
    // Add more as needed...
};

// In BotAI.cpp
void BotAI::OnGroupMemberJoined(Playerbot::GroupEvent const& event)
{
    TC_LOG_DEBUG("bot.ai", "BotAI: Group member joined - Group: {}, Member: {}",
        event.groupGuid.ToString(), event.targetGuid.ToString());

    // Example: Update formation when new member joins
    if (_combatCoordinator)
        _combatCoordinator->UpdateFormation();
}

void BotAI::OnGroupMemberLeft(Playerbot::GroupEvent const& event)
{
    TC_LOG_DEBUG("bot.ai", "BotAI: Group member left - Group: {}, Member: {}",
        event.groupGuid.ToString(), event.targetGuid.ToString());

    // Example: Adjust tactics when member leaves
    if (_combatCoordinator)
        _combatCoordinator->RecalculateGroupComposition();
}

void BotAI::OnGroupLeaderChanged(Playerbot::GroupEvent const& event)
{
    TC_LOG_INFO("bot.ai", "BotAI: Group leader changed - New leader: {}",
        event.targetGuid.ToString());

    // Example: Update assist target if leader changed
    if (event.targetGuid == _bot->GetGUID())
    {
        // Bot is now the leader
        TC_LOG_INFO("bot.ai", "BotAI: Bot {} is now group leader", _bot->GetName());
    }
}

void BotAI::OnLootMethodChanged(Playerbot::GroupEvent const& event)
{
    TC_LOG_DEBUG("bot.ai", "BotAI: Loot method changed - Method: {}", event.data1);

    // Example: Adjust looting behavior based on loot method
    // data1 contains the new LootMethod value
}

void BotAI::OnRaidConverted(Playerbot::GroupEvent const& event)
{
    bool isRaid = (event.data1 != 0);
    TC_LOG_INFO("bot.ai", "BotAI: Group converted to {} - Group: {}",
        isRaid ? "RAID" : "PARTY", event.groupGuid.ToString());

    // Example: Switch to raid formations if converted to raid
    if (isRaid && _combatCoordinator)
        _combatCoordinator->SwitchToRaidFormation();
}

void BotAI::OnDifficultyChanged(Playerbot::GroupEvent const& event)
{
    TC_LOG_DEBUG("bot.ai", "BotAI: Difficulty changed - Difficulty: {}", event.data1);

    // Example: Adjust AI aggressiveness based on difficulty
    // data1 contains the new difficulty ID
}
```

---

## 🎯 Event Types and Data

### Available Events (via hooks + polling)

| Event Type | Trigger | data1 | data2 | targetGuid | Description |
|-----------|---------|-------|-------|------------|-------------|
| MEMBER_JOINED | OnAddMember hook | - | - | Member GUID | New member joined group |
| MEMBER_LEFT | OnRemoveMember hook | RemoveMethod | - | Member GUID | Member left group |
| LEADER_CHANGED | OnChangeLeader hook | - | - | New leader GUID | Leadership changed |
| GROUP_DISBANDED | OnDisband hook | - | - | - | Group disbanded |
| LOOT_METHOD_CHANGED | Polling | LootMethod | - | - | Loot method changed |
| LOOT_THRESHOLD_CHANGED | Polling | ItemQualities | - | - | Loot threshold changed |
| MASTER_LOOTER_CHANGED | Polling | - | - | Master looter GUID | Master looter changed |
| DIFFICULTY_CHANGED | Polling | Difficulty ID | - | - | Instance difficulty changed |
| RAID_CONVERTED | Polling | 1=raid, 0=party | - | - | Party/raid conversion |
| SUBGROUP_CHANGED | Polling | New subgroup | - | Member GUID | Member subgroup changed |

### Unavailable Events (TrinityCore limitations)

- ❌ TARGET_ICON_CHANGED (no Group::GetTargetIcon() accessor)
- ❌ READY_CHECK_* (no Group::IsReadyCheckActive() accessor)

**Workaround**: Implement packet sniffing for SMSG_RAID_TARGET_UPDATE and SMSG_READY_CHECK packets

---

## 📊 Performance Considerations

### Polling Overhead
- **Per-bot polling**: ~200 CPU cycles per poll
- **Poll interval**: 100-500ms recommended (configurable in BotAI::Update)
- **500 bots**: ~0.03% CPU @ 3GHz with 100ms polling

### Memory Usage
- **GroupState cache**: ~150 bytes per group
- **Event queue**: ~64 bytes per queued event
- **Total**: <10MB for 500 bot groups

### Optimization Tips
1. **Poll only when in group**: Check `_bot->GetGroup()` before polling
2. **Adjust poll interval**: Use higher interval (500ms) for idle bots
3. **Batch event processing**: GroupEventBus processes events in batches
4. **Unsubscribe unused events**: Only subscribe to events you actually use

---

## 🧪 Testing the Integration

### Test 1: Member Join/Leave
```cpp
// Create test scenario
Bot1->InvitePlayerToGroup(Bot2);
Bot2->AcceptGroupInvite();
// Verify: OnGroupMemberJoined() called for both bots

Bot2->LeaveGroup();
// Verify: OnGroupMemberLeft() called for Bot1
```

### Test 2: Loot Method Change
```cpp
Group* group = Bot1->GetGroup();
group->SetLootMethod(LootMethod::MASTER_LOOT);
// Wait for next poll cycle (100ms)
// Verify: OnLootMethodChanged() called for all group bots
```

### Test 3: Raid Conversion
```cpp
Group* group = Bot1->GetGroup();
for (int i = 0; i < 5; i++)
    group->AddMember(TestBot[i]);
group->ConvertToRaid();
// Verify: OnRaidConverted() called with data1 = 1
```

---

## 🔍 Debugging

### Enable Debug Logging
```cpp
// In worldserver.conf
Logger.Group = 4, Console Server  # Enable group event logging
Logger.BotAI = 4, Console Server  # Enable bot AI logging
```

### Event Bus Statistics
```cpp
// Get event statistics
auto stats = Playerbot::GroupEventBus::instance()->GetStatistics();
TC_LOG_INFO("bot.ai", "Event Bus Stats:\n"
    "  Total Published: {}\n"
    "  Total Delivered: {}\n"
    "  Pending: {}\n"
    "  Avg Process Time: {} μs",
    stats.totalPublished, stats.totalDelivered, stats.pendingCount, stats.avgProcessTimeUs);
```

### Dump Event Queue
```cpp
// Dump pending events for debugging
Playerbot::GroupEventBus::instance()->DumpEventQueue();
```

---

## ✅ Integration Checklist

Before deploying, verify:
- [ ] BotAI subscribes to events in constructor
- [ ] BotAI unsubscribes in destructor
- [ ] BotAI::Update() calls PollGroupStateChanges()
- [ ] All event handlers implemented
- [ ] Event handlers update bot behavior appropriately
- [ ] Performance testing with 100+ bots
- [ ] Memory leak testing (no leaked subscriptions)
- [ ] Event delivery tested for all event types

---

## 🚀 Advanced Usage

### Custom Event Priorities
```cpp
// Publish custom high-priority event
Playerbot::GroupEvent event;
event.type = Playerbot::GroupEventType::MEMBER_JOINED;
event.priority = Playerbot::EventPriority::CRITICAL; // Process immediately
event.groupGuid = group->GetGUID();
event.targetGuid = newMember->GetGUID();
Playerbot::GroupEventBus::instance()->PublishEvent(event);
```

### Event Filtering
```cpp
// Subscribe with filter predicate
eventBus->Subscribe(_bot->GetGUID(), Playerbot::GroupEventType::MEMBER_JOINED,
    [this](Playerbot::GroupEvent const& event) {
        // Only handle if this is MY group
        if (event.groupGuid == _bot->GetGroup()->GetGUID())
            OnGroupMemberJoined(event);
    });
```

### Batch Event Processing
```cpp
// Process multiple events at once
std::vector<Playerbot::GroupEvent> events;
eventBus->GetPendingEvents(_bot->GetGUID(), events, 10); // Get up to 10 events
for (auto const& event : events)
{
    switch (event.type)
    {
        case Playerbot::GroupEventType::MEMBER_JOINED:
            OnGroupMemberJoined(event);
            break;
        // ... handle other events ...
    }
}
```

---

## 📚 Related Documentation

- **GROUP_EVENT_SYSTEM_STATUS.md** - System implementation status
- **Group/GroupEventBus.h** - Event bus API documentation
- **Group/GroupStateMachine.h** - State machine documentation
- **Group/GroupEventHandler.h** - Event handler implementations
- **Group/PlayerbotGroupScript.h** - TrinityCore integration
