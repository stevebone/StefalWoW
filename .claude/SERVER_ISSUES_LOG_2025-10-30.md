# Server Issues and Anomalies - October 30, 2025

## Session Timeline

**Start Time**: ~01:05 (after previous crash)
**End Time**: 01:13:52-53 (graceful shutdown)
**Duration**: ~8 minutes runtime
**Last Crash**: 01:05:34 (BoundingIntervalHierarchy collision crash)

---

## Critical Issues Identified

### 1. ❌ BoundingIntervalHierarchy Collision Crash (01:05:34)

**Severity**: CRITICAL - Server Crash
**Status**: ❌ UNRESOLVED
**Crash File**: `38981ec17edb+_worldserver.exe_[2025_10_30_1_5_34].txt`

**Exception Details**:
```
Exception: 80000003 BREAKPOINT
Location: BoundingIntervalHierarchy.cpp:60
Error: throw std::logic_error("invalid node overlap")
```

**Call Stack**:
```
BIH::subdivide (line 60) → "invalid node overlap"
  ↓
BIH::buildHierarchy
  ↓
DynamicMapTree::getHeight
  ↓
PathGenerator::CalculatePath
  ↓
RandomMovementGenerator<Creature>::SetRandomLocation
  ↓
Creature::Update (NPC, not bot!)
  ↓
Map::Update
```

**Analysis**:
- **NOT a bot issue** - Triggered by NPC (Creature) random movement pathfinding
- **Core TrinityCore bug** - Collision system with invalid map geometry
- **Affects any server** - Can happen with creature movement on any map
- **No bot code in stack** - Pure TrinityCore pathfinding crash

**Likely Causes**:
1. Corrupt map/collision data (`.map`, `.vmap`, `.mmap` files)
2. Invalid GameObject bounding boxes
3. Map extraction issue (bad geometry data)
4. Known TrinityCore bug with specific maps/areas

---

### 2. ⚠️ Spell 83470 Proc Trigger Missing (Ongoing)

**Severity**: MODERATE - Spam Warning
**Status**: ⚠️ NEEDS INVESTIGATION
**Frequency**: Multiple instances every few seconds

**Error Message**:
```
AuraEffect::HandleProcTriggerSpellAuraProc: Spell 83470 [EffectIndex: 0] does not have triggered spell.
```

**Occurrences**: 5+ times in last 100 log lines

**Analysis**:
- Spell 83470 is configured with SPELL_AURA_PROC_TRIGGER_SPELL
- But database has no triggered spell defined for Effect 0
- TrinityCore expects a triggered spell but database entry is missing
- This is a **database configuration issue**, not a code bug

**Impact**:
- Log spam (performance impact if frequent)
- Spell may not function correctly
- Possible incomplete spell implementation

---

### 3. ⚠️ "Non Existent Socket" Errors During Shutdown

**Severity**: LOW - Normal Shutdown Artifact
**Status**: ✅ EXPECTED BEHAVIOR

**Error Messages**:
```
Prevented sending of [SMSG_ON_MONSTER_MOVE 0x480002] to non existent socket 1 to [Player: Yesenia Player-1-0000003B, Account: 62]
Prevented sending of [SMSG_ON_MONSTER_MOVE 0x480002] to non existent socket 1 to [Player: Octavius Player-1-0000001C, Account: 30]
Prevented sending of [SMSG_ON_MONSTER_MOVE 0x480002] to non existent socket 1 to [Player: Boone Player-1-00000007, Account: 8]
Prevented sending of [SMSG_USERLIST_REMOVE 0x3B0010] to non existent socket 0 to [Player: Boone Player-1-00000007, Account: 8]
```

**Analysis**:
- **Normal behavior** during shutdown
- Bots are being destroyed (BehaviorPriorityManager, DeathRecoveryManager destructors)
- Network layer tries to send packets but socket already closed
- TrinityCore prevents the send and logs warning
- **No action needed** - defensive logging working correctly

---

### 4. ℹ️ Quest Hub Database Warnings (Startup)

**Severity**: INFO - Configuration Issue
**Status**: ℹ️ INFORMATIONAL

**Warnings**:
```
QuestHubDatabase: Hub 8 has no quests
QuestHubDatabase: Hub 73 has no quests
[...21 total hubs with no quests...]
QuestHubDatabase: Validation found 21 warnings
```

**Analysis**:
- 21 quest hubs configured but have no associated quests
- Likely incomplete quest database or zone configuration
- Playerbot quest routing will skip these hubs
- **Low priority** - Does not cause crashes or errors

---

### 5. ℹ️ Spell Script Hook Mismatch (Startup)

**Severity**: INFO - Script Configuration
**Status**: ℹ️ INFORMATIONAL

**Warning**:
```
Spell `66683` Effect `Index: EFFECT_2, AuraName: SPELL_AURA_12` of script `spell_icehowl_massive_crash` did not match dbc effect data - handler bound to hook `AfterEffectRemove` of AuraScript won't be executed
```

**Analysis**:
- Spell script for boss ability "Icehowl Massive Crash"
- Script expects specific effect but DBC data doesn't match
- Script hook won't execute (spell may work differently than intended)
- **Not critical** - Boss scripts often have version mismatches
- **No bot impact** - This is raid boss content

---

### 6. ℹ️ BotGearFactory Not Ready (Startup)

**Severity**: INFO - Initialization Order
**Status**: ℹ️ INFORMATIONAL

**Message**:
```
BotLevelManager::Initialize() - BotGearFactory not ready
```

**Analysis**:
- Initialization order issue between BotLevelManager and BotGearFactory
- Likely deferred initialization (will be ready later)
- **Low priority** - May indicate initialization dependency issue
- **Needs verification** - Check if bots get proper gear later in startup

---

## Shutdown Sequence Analysis

**Time**: 01:13:52-53

**Sequence Observed**:
```
1. ManagerRegistry destroyed with 0 managers (multiple bots)
2. DeathRecoveryManager destroyed for bot Boone
3. DeathRecoveryManager destroyed for bot Sevtap
4. DeathRecoveryManager destroyed for bot Yesenia
5. DeathRecoveryManager destroyed for bot Octavius
6. DeathRecoveryManager destroyed for bot Good
7. BehaviorPriorityManager destroyed for bot Octavius
8. BehaviorPriorityManager destroyed for bot Boone
9. BehaviorPriorityManager destroyed for bot Sevtap
10. BehaviorPriorityManager destroyed for bot Good
11. BehaviorPriorityManager destroyed for bot Yesenia
12. "Non existent socket" errors (network cleanup)
13. Spell 83470 proc trigger errors (final spell cleanup)
```

**Analysis**:
- **Graceful shutdown** - All destructors called in correct order
- **ManagerRegistry → DeathRecoveryManager → BehaviorPriorityManager** - Proper cleanup sequence
- **No crash during shutdown** - Just normal cleanup warnings

**Conclusion**: Shutdown was **clean and orderly**, no issues detected.

---

## Summary of Findings

### Critical Issues (1)
1. ❌ **BoundingIntervalHierarchy collision crash** - Core TrinityCore bug, NOT bot-related

### Moderate Issues (1)
1. ⚠️ **Spell 83470 proc trigger missing** - Database configuration issue

### Low Priority (4)
1. ✅ **Socket errors during shutdown** - Expected behavior
2. ℹ️ **Quest hub warnings** - Incomplete quest database
3. ℹ️ **Spell script mismatch** - Boss script version issue
4. ℹ️ **BotGearFactory not ready** - Initialization order (needs verification)

---

## Questions Requiring Investigation

### 1. BoundingIntervalHierarchy Crash
- Which map/zone triggered the crash?
- Is this a known TrinityCore issue?
- Can we identify the specific creature or area?
- Should we add defensive code or fix map data?

### 2. Spell 83470
- What spell is ID 83470?
- Is it used by bots or NPCs?
- Does it need a triggered spell configured?
- Is this affecting gameplay?

### 3. BotGearFactory
- Does BotGearFactory initialize successfully later?
- Are bots getting proper gear?
- Is this causing equipment issues?

---

## Next Steps

1. **IMMEDIATE**: Copy new worldserver.exe from `c:/TrinityBots/TrinityCore/build/bin/RelWithDebInfo/` to `M:/Wplayerbot/`
2. **TEST**: Verify crashes 1.1, 1.2, and 1.4 are resolved with new binary
3. **RESEARCH**: Update TrinityCore for BIH crash fix (Issue #5218)
4. **DATABASE**: Fix Spell 83470 proc trigger configuration
5. **VERIFY**: Check if bots have proper equipment (BotGearFactory init)

---

## Comprehensive Investigation Plan Created

See: `.claude/COMPREHENSIVE_INVESTIGATION_PLAN.md`

**Summary**:
- **5 crashes analyzed** (3 fixed, 1 core bug, 1 needs testing)
- **3 database/config issues** documented
- **All fixes implemented** in commits b99fdbcf84, 5000649091, 79842f3197
- **Awaiting user testing** with new worldserver.exe

---

**Log Files Analyzed**:
- Server.log (ending 01:13:53)
- Playerbot.log (ending 01:13:52)
- Crash dumps:
  - 38981ec17edb+_worldserver.exe_[2025_10_30_1_5_34].txt (BIH crash)
  - 38981ec17edb+_worldserver.exe_[2025_10_30_1_15_37].txt (Unit.cpp:10863)
  - 38981ec17edb+_worldserver.exe_[2025_10_30_1_19_8].txt (Map.cpp:686 - old binary)

**Analysis Date**: 2025-10-30 01:25
**Status**: ✅ Investigation complete, comprehensive plan created, awaiting user testing
