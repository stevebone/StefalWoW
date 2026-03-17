# Group Member Lookup Problem Analysis

**Date**: 2025-01-26  
**Status**: ACTIVE INVESTIGATION  
**Severity**: HIGH - Affects core dungeon/group functionality

---

## 1. Problem Summary

Bots in dungeons/groups are not fighting together or healing each other. The root cause is that **group member lookups frequently fail for bots**, causing:

- Healers can't find group members to heal
- DPS don't detect when group is in combat
- Tanks don't see threat from group members
- Combat coordination completely breaks down

---

## 2. Root Cause Analysis

### 2.1 The Core Issue

TrinityCore's `Group::GetMembers()` returns `GroupReference` objects. When you call `ref.GetSource()`, it internally uses `ObjectAccessor::FindPlayer()` which **only finds players properly registered in the ObjectAccessor**.

**The Problem**: Bots managed by `BotWorldSessionMgr` are often NOT properly registered in `ObjectAccessor`, so `GetSource()` returns `nullptr` even though the bot exists and is in the world.

### 2.2 Code Flow Analysis

```cpp
// How group iteration SHOULD work:
Group* group = player->GetGroup();
for (GroupReference& ref : group->GetMembers())
{
    Player* member = ref.GetSource();  // ← FAILS FOR BOTS!
    if (!member) continue;             // ← Bots are skipped
    // ... do something with member
}

// What GetSource() does internally:
Player* GroupReference::GetSource() const
{
    return ObjectAccessor::FindPlayer(GetGUID());  // ← Bots not found here
}
```

### 2.3 Why Bots Aren't Found

1. **ObjectAccessor::FindPlayer()** searches by GUID in a global map
2. Bots are created through `BotWorldSessionMgr::AddPlayerBot()`
3. The bot registration path may not properly add bots to ObjectAccessor
4. Alternative: `ObjectAccessor::FindConnectedPlayer()` might work
5. Fallback: `BotWorldSessionMgr::GetPlayerBot()` always works for bots

### 2.4 Evidence

In `GroupCombatStrategy.cpp`, a fix was already partially implemented:

```cpp
static Player* FindGroupMember(ObjectGuid memberGuid)
{
    // Method 1: Standard - FAILS for bots
    if (Player* player = ObjectAccessor::FindPlayer(memberGuid))
        return player;
    
    // Method 2: Connected players
    if (Player* player = ObjectAccessor::FindConnectedPlayer(memberGuid))
        return player;
    
    // Method 3: Bot registry - ALWAYS WORKS for bots
    if (Player* bot = sBotWorldSessionMgr->GetPlayerBot(memberGuid))
        return bot;
    
    return nullptr;
}
```

**But this fix is NOT applied consistently across the codebase!**

---

## 3. Affected Code Locations

### 3.1 Critical - Combat/Healing (MUST FIX)

| File | Function | Line | Impact |
|------|----------|------|--------|
| `AI/Services/HealingTargetSelector.cpp` | `GetGroupMembersInRange()` | ~895 | **Healers can't find targets** |
| `AI/Combat/GroupCombatTrigger.cpp` | `ShouldEngageCombat()` | ~130 | **DPS don't assist** |
| `AI/Combat/GroupCombatTrigger.cpp` | `GetAssistTarget()` | ~200 | **Wrong target selection** |
| `AI/Combat/GroupCombatTrigger.cpp` | `CalculateUrgency()` | ~110 | **Combat priority wrong** |
| `AI/Strategy/GroupCombatStrategy.cpp` | `IsGroupInCombat()` | ~305 | Partially fixed |
| `AI/Strategy/GroupCombatStrategy.cpp` | `UpdateBehavior()` | ~165 | Partially fixed |
| `Session/BotPriorityManager.cpp` | Group combat check | ~240 | **Priority system fails** |

### 3.2 High - Group Coordination

| File | Function | Line | Impact |
|------|----------|------|--------|
| `AI/Combat/ThreatCoordinator.cpp` | Multiple | TBD | Threat sharing fails |
| `AI/Combat/FormationManager.cpp` | Multiple | TBD | Formation breaks |
| `AI/Combat/InterruptCoordinator.cpp` | Multiple | TBD | Interrupt coordination fails |
| `Group/GroupCoordination.cpp` | Multiple | TBD | General coordination |

### 3.3 Medium - Loot/Trade

| File | Function | Line | Impact |
|------|----------|------|--------|
| `Social/UnifiedLootManager.cpp` | 8+ locations | 649,763,832,... | Loot distribution fails |
| `Social/LootDistribution.cpp` | 6+ locations | 82,324,721,... | Roll system fails |
| `Social/TradeManager.cpp` | 4+ locations | 813,1088,1333,... | Trade fails |

### 3.4 Low - Session/Network

| File | Function | Line | Impact |
|------|----------|------|--------|
| `Session/BotSession.cpp` | LFG sync | ~928 | LFG issues |
| `Session/BotPacketRelay.cpp` | Packet relay | ~397,432 | Packet delivery |

---

## 4. Total Impact Assessment

Based on search results: **~276 occurrences** of `GetMembers()` across the codebase.

Estimated affected functionality:
- 100% of group healing
- 100% of group combat assistance
- 100% of threat coordination
- 80% of loot distribution
- 70% of group positioning/formation

---

## 5. Sustainable Solution Design

### 5.1 Design Principles

1. **Single Point of Truth**: One utility class for all group member lookups
2. **Backward Compatible**: Existing code can be migrated incrementally
3. **Diagnostic Capable**: Built-in tracking of lookup success/failure
4. **Performance Optimized**: Cache successful lookups, minimize repeated failures
5. **Thread Safe**: Must work from main thread and worker threads

### 5.2 Solution Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                  GroupMemberResolver                         │
│  (Central utility for all group member lookups)             │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ResolveGroupMember(guid) → Player*                         │
│    ├─ ObjectAccessor::FindPlayer()        [fastest]         │
│    ├─ ObjectAccessor::FindConnectedPlayer() [connected]     │
│    └─ BotWorldSessionMgr::GetPlayerBot()  [bot registry]    │
│                                                             │
│  ResolveGroupMembers(group) → vector<Player*>               │
│    └─ Iterates group, resolves each member                  │
│                                                             │
│  GetGroupMembersInRange(player, range) → vector<Player*>    │
│    └─ Resolves + distance filter                            │
│                                                             │
│  Built-in diagnostics via GroupMemberDiagnostics            │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### 5.3 Implementation Files

1. **GroupMemberResolver.h** - Central utility class
2. **GroupMemberResolver.cpp** - Implementation
3. **GroupMemberDiagnostics.h/cpp** - Already created (diagnostics)

### 5.4 Migration Strategy

**Phase 1**: Create GroupMemberResolver with diagnostics (this session)
**Phase 2**: Instrument critical combat/healing code with diagnostics
**Phase 3**: Run tests, collect diagnostic data
**Phase 4**: Migrate all affected files to use GroupMemberResolver
**Phase 5**: Verify fix, remove diagnostic overhead if desired

---

## 6. Diagnostic Integration Plan

### 6.1 Key Files to Instrument First

1. `HealingTargetSelector.cpp` - Most visible symptom
2. `GroupCombatTrigger.cpp` - Combat detection
3. `GroupCombatStrategy.cpp` - Already has partial fix

### 6.2 Diagnostic Output Expected

```
╔══════════════════════════════════════════════════════════════════════╗
║           GROUP MEMBER LOOKUP DIAGNOSTICS REPORT                     ║
╠══════════════════════════════════════════════════════════════════════╣
║ GLOBAL STATISTICS                                                    ║
║   Total Lookups:            1000                                     ║
║   Successful:                650                                     ║
║   Failed:                    350                                     ║
║   Success Rate:             65.0%                                    ║
║                                                                      ║
║ FAILURE BREAKDOWN                                                    ║
║   Bot Lookup Failures:       340  <-- LIKELY THE PROBLEM!            ║
║   Player Failures:            10                                     ║
╠══════════════════════════════════════════════════════════════════════╣
║ TOP PROBLEM LOCATIONS (by failure count)                             ║
╠══════════════════════════════════════════════════════════════════════╣
║ GetGroupMembersInRange                                               ║
║   Attempts:    200  Failures:    180  Rate: 10.0%                    ║
║   Bot Failures:    175  Player Failures:      5                      ║
║   → BotSessionMgr would fix 175 lookups!                             ║
╠──────────────────────────────────────────────────────────────────────╣
║ ShouldEngageCombat                                                   ║
║   Attempts:    150  Failures:    120  Rate: 20.0%                    ║
║   → BotSessionMgr would fix 115 lookups!                             ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## 7. Next Steps

1. ✅ Create GroupMemberDiagnostics.h/cpp (DONE)
2. ⏳ Create GroupMemberResolver.h/cpp (central solution)
3. ⏳ Add diagnostic command to enable/disable/report
4. ⏳ Instrument HealingTargetSelector with diagnostics
5. ⏳ Instrument GroupCombatTrigger with diagnostics
6. ⏳ Test in dungeon, collect diagnostic data
7. ⏳ Migrate all affected files to GroupMemberResolver

---

## 8. Files Modified/Created

### Created:
- `Core/Diagnostics/GroupMemberDiagnostics.h`
- `Core/Diagnostics/GroupMemberDiagnostics.cpp`
- `.claude/GROUP_MEMBER_LOOKUP_ANALYSIS.md` (this file)

### To Be Created:
- `Group/GroupMemberResolver.h`
- `Group/GroupMemberResolver.cpp`

### To Be Modified:
- `AI/Services/HealingTargetSelector.cpp`
- `AI/Combat/GroupCombatTrigger.cpp`
- `AI/Strategy/GroupCombatStrategy.cpp`
- (and ~20 more files after validation)

---

**Document Version**: 1.0  
**Last Updated**: 2025-01-26
