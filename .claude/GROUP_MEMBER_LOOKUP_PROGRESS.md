# Group Member Lookup Fix - Progress Report

**Date**: 2025-01-26  
**Status**: IN PROGRESS

---

## Completed Work

### 1. Diagnostic System Created ✅

**Files Created:**
- `Core/Diagnostics/GroupMemberDiagnostics.h` - Header mit API
- `Core/Diagnostics/GroupMemberDiagnostics.cpp` - Implementation

**Features:**
- Tracks all group member lookup attempts
- Records success/failure by method (ObjectAccessor, ConnectedPlayer, BotSessionMgr)
- Identifies which callers have the highest failure rates
- Generates detailed reports

### 2. Central Solution Created ✅

**Files Created:**
- `Group/GroupMemberResolver.h` - Central utility API
- `Group/GroupMemberResolver.cpp` - Implementation

**Key Methods:**
```cpp
// Core lookup with fallback chain
static Player* ResolveMember(ObjectGuid guid);

// Get all members (guaranteed non-null)
static std::vector<Player*> GetGroupMembers(Group* group);

// Combat helpers
static bool IsGroupInCombat(Group* group, Player* exclude = nullptr);
static std::vector<Player*> GetGroupMembersNeedingHealing(Group* group, float healthThreshold);

// Role helpers
static std::vector<Player*> GetGroupTanks(Group* group);
static std::vector<Player*> GetGroupHealers(Group* group);
```

### 3. Chat Commands Added ✅

**Commands:**
- `.bot diag` - Show status and quick stats
- `.bot diag enable` - Enable diagnostic tracking
- `.bot diag disable` - Disable diagnostics
- `.bot diag report` - Show detailed report
- `.bot diag reset` - Reset statistics
- `.bot diag verbose <on/off>` - Toggle verbose logging

### 4. Critical Files Partially Fixed ✅

**HealingTargetSelector.cpp:**
- `GetGroupMembersInRange()` - FIXED ✅

**GroupCombatTrigger.cpp:**
- `CalculateUrgency()` - FIXED ✅
- 6+ additional locations - NEED FIX ⚠️

---

## Remaining Work

### High Priority - GroupCombatTrigger.cpp

The following functions still use broken `GetMembers()` pattern:

| Line | Function | Impact |
|------|----------|--------|
| ~181 | ShouldEngageCombat | Combat detection fails |
| ~251 | GetAssistTarget | Wrong target selection |
| ~326 | GetAssistTarget | Nearest target calc |
| ~374 | UpdateGroupCombatInfo | Combat info wrong |
| ~460 | IsTargetEngaged | Target check fails |
| ~483 | IsGroupInCombat | Combat detection |
| ~547 | CountMembersAttacking | Count wrong |

### Medium Priority - Other Files

Based on analysis, these files need fixing:

1. **AI/Combat/ThreatCoordinator.cpp** - Threat sharing
2. **AI/Combat/FormationManager.cpp** - Formation positioning
3. **AI/Combat/InterruptCoordinator.cpp** - Interrupt coordination
4. **Session/BotPriorityManager.cpp** - Priority calculation
5. **Social/UnifiedLootManager.cpp** - 8+ locations
6. **Social/LootDistribution.cpp** - 6+ locations
7. **Social/TradeManager.cpp** - 4+ locations
8. **Session/BotSession.cpp** - LFG handling
9. **Session/BotPacketRelay.cpp** - Packet relay

---

## How to Test

### 1. Build the Project
```bash
cmake --build build --config RelWithDebInfo
```

### 2. Enable Diagnostics
```
.bot diag enable
```

### 3. Run Dungeon Test
- Queue for LFG with bot group
- Enter dungeon
- Engage enemies
- Observe healing and combat behavior

### 4. Check Report
```
.bot diag report
```

The report will show:
- Which lookups failed
- Whether failures are for bots
- Which callers have problems
- Which fallback method would have worked

---

## Expected Diagnostic Output

```
╔══════════════════════════════════════════════════════════════════════╗
║           GROUP MEMBER LOOKUP DIAGNOSTICS REPORT                     ║
╠══════════════════════════════════════════════════════════════════════╣
║ GLOBAL STATISTICS                                                    ║
║   Total Lookups:            500                                      ║
║   Failed:                   200                                      ║
║   Bot Lookup Failures:      195  <-- CONFIRMS THE PROBLEM!           ║
╠══════════════════════════════════════════════════════════════════════╣
║ TOP PROBLEM LOCATIONS                                                ║
║ ShouldEngageCombat          Failures: 80   → BotSessionMgr would fix ║
║ GetAssistTarget             Failures: 60   → BotSessionMgr would fix ║
║ IsGroupInCombat             Failures: 40   → BotSessionMgr would fix ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Next Steps

1. **Test current fixes** - Run dungeon to verify HealingTargetSelector fix works
2. **Complete GroupCombatTrigger.cpp** - Fix remaining 6 locations
3. **Fix other combat files** - ThreatCoordinator, FormationManager, etc.
4. **Validate with diagnostics** - Use `.bot diag report` to confirm fixes
5. **Migrate remaining files** - Loot, Trade, Session files

---

## Files Modified This Session

### Created:
- `Core/Diagnostics/GroupMemberDiagnostics.h`
- `Core/Diagnostics/GroupMemberDiagnostics.cpp`
- `Group/GroupMemberResolver.h`
- `Group/GroupMemberResolver.cpp`
- `.claude/GROUP_MEMBER_LOOKUP_ANALYSIS.md`
- `.claude/GROUP_MEMBER_LOOKUP_PROGRESS.md` (this file)

### Modified:
- `Commands/PlayerbotCommands.h` - Added diag command declarations
- `Commands/PlayerbotCommands.cpp` - Added diag command implementations
- `AI/Services/HealingTargetSelector.cpp` - Fixed GetGroupMembersInRange
- `AI/Combat/GroupCombatTrigger.cpp` - Fixed CalculateUrgency (partial)

---

**Document Version**: 1.0  
**Last Updated**: 2025-01-26
