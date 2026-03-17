# CRITICAL: Resurrection System Crash Analysis
**Date:** 2025-10-30 15:35:18  
**Severity:** CRITICAL - Production Blocker  
**Status:** REGRESSION - New bug introduced by direct resurrection changes

## Crash Summary
- **Crash Rate:** 1 crash after 27 successful resurrections (15 minutes runtime, 100 bots)
- **Production Impact:** UNACCEPTABLE - Server cannot run stably
- **Assertion Failed:** `Unit.cpp:10863 - ASSERT(m_procDeep)` in `Unit::SetCantProc()`

## What Happened
1. ✅ Bot "Cathrine" resurrected successfully via `ResurrectPlayer(0.5f, false)`
2. ✅ Health/Mana restored (119/238 HP, 180/360 Mana)  
3. ✅ Death recovery transitioned to state 0 (NOT_DEAD)
4. ❌ **CRASH during post-resurrection zone update:**
   - `ResurrectPlayer()` → `UpdateZone()` → `UpdateAreaDependentAuras()` 
   - Zone change spell cast → Proc system triggered
   - `m_procDeep` counter became zero when `SetCantProc(false)` expected non-zero
   - Assertion failure → Server crash

## Call Stack
```
Player::ResurrectPlayer (line 4407)
  └─> Player::UpdateZone
      └─> Player::UpdateArea  
          └─> Player::UpdateAreaDependentAuras
              └─> WorldObject::CastSpell (zone aura)
                  └─> Unit::ProcSkillsAndAuras
                      └─> Unit::TriggerAurasProcOnEvent
                          └─> Unit::SetCantProc [CRASH]
                              └─> ASSERT(m_procDeep) FAILED
```

## Root Cause Analysis

### The Proc Depth Counter (`m_procDeep`)
TrinityCore uses `m_procDeep` to track spell proc recursion depth:
- `SetCantProc(true)` → `++m_procDeep` (increment before procs)
- `SetCantProc(false)` → `--m_procDeep` (decrement after procs)
- **Assertion:** When decrementing, `m_procDeep` must be > 0

### Why It Failed
The counter became **unbalanced** during resurrection:
1. Some code path called `SetCantProc(true)` without matching `SetCantProc(false)`
2. OR: Some code path called `SetCantProc(false)` too many times
3. Result: Counter reached zero, next `SetCantProc(false)` → assertion failure

### Possible Causes
1. **Thread Safety Issue:** `ResurrectPlayer()` called from bot worker thread may have race conditions with proc counter
2. **State Transition Bug:** Resurrection may not properly clean up pending proc states
3. **Zone Update Bug:** Area-dependent aura application during resurrection triggers unexpected proc chains
4. **Pre-existing TrinityCore Bug:** Exposed by our specific resurrection code path

## Changes That Introduced This

### Before (Packet-Based - STABLE)
```cpp
// Old approach: Send CMSG_RECLAIM_CORPSE packet
WorldPacket packet(CMSG_RECLAIM_CORPSE);
packet << m_bot->GetGUID();
m_bot->GetSession()->HandleReclaimCorpseOpcode(packet);
```
- Processed on main thread via packet handler
- Full TrinityCore validation path
- **No crashes observed**

### After (Direct API - UNSTABLE)  
```cpp
// New approach: Direct API call from worker thread
m_bot->ResurrectPlayer(0.5f, false);
m_bot->SpawnCorpseBones();
```
- Called directly from bot worker thread (BotSession mutex protected)
- Bypasses packet handler validation
- **Crashes with m_procDeep assertion**

## Success Before Crash
- ✅ 27 successful resurrections
- ✅ 100% resurrection success rate
- ✅ No "Resurrection did not complete" failures
- ✅ Direct resurrection DOES work
- ❌ **BUT: Server crashes after ~15 minutes**

## Production Impact
**This is a CRITICAL REGRESSION:**
- Old system: Stable, no crashes (but had other issues)
- New system: Works but crashes server periodically
- **Cannot be deployed to production**

## Investigation Needed
1. ✅ Check if old packet approach had same crash → **User confirms this is NEW**
2. ❌ Investigate thread safety of `ResurrectPlayer()` from worker thread
3. ❌ Check if we need to call additional cleanup before/after resurrection
4. ❌ Review TrinityCore's packet handler for missing state management
5. ❌ Consider wrapping resurrection in main thread dispatch

## Potential Fixes (Priority Order)

### Option 1: Dispatch ResurrectPlayer to Main Thread
```cpp
// Queue resurrection to execute on main thread instead of worker thread
m_bot->GetMap()->AddToWorld([bot = m_bot]() {
    bot->ResurrectPlayer(0.5f, false);
    bot->SpawnCorpseBones();
});
```
**Pros:** Matches TrinityCore's threading model  
**Cons:** Adds complexity, may have timing issues

### Option 2: Add Proc State Cleanup
```cpp
// Clear proc state before resurrection
m_bot->RemoveAllAuras(); // Clear all auras including proc-triggering ones
m_bot->ResurrectPlayer(0.5f, false);
m_bot->SpawnCorpseBones();
```
**Pros:** Simple, might fix proc counter issues  
**Cons:** May be too aggressive, could break other systems

### Option 3: Revert to Packet-Based Approach
```cpp
// Use packet approach but with proper threading
// Queue packet to be processed on main thread
```
**Pros:** Known stable approach  
**Cons:** Defeats purpose of direct API usage

### Option 4: Wrap ResurrectPlayer in Proc Guards
```cpp
// Manually manage proc depth
bool hadProcs = m_bot->IsProcsActive();
m_bot->SetCantProc(true);  // Prevent procs during resurrection
m_bot->ResurrectPlayer(0.5f, false);
m_bot->SetCantProc(false); // Re-enable procs
m_bot->SpawnCorpseBones();
```
**Pros:** Surgical fix for specific issue  
**Cons:** May not address root cause

## Next Steps
1. Identify exact condition causing `m_procDeep` imbalance
2. Test if packet-based approach had this issue (user says NO)
3. Implement thread-safe resurrection (Option 1 most promising)
4. Add comprehensive proc state logging for debugging
5. Stress test with 100+ bots for extended periods

## Related Files
- `DeathRecoveryManager.cpp:598` - Direct resurrection call
- `Player.cpp:4407` - ResurrectPlayer() implementation  
- `Unit.cpp:10863` - SetCantProc() assertion
- `Unit.cpp:10437-10577` - Proc system implementation

## Crash Dumps
- Latest: `3f33f776e3ba+_worldserver.exe_[2025_10_30_15_35_18].txt`
- Previous: `3f33f776e3ba+_worldserver.exe_[2025_10_30_15_17_39].txt` (Freeze detector)
- Previous: `3f33f776e3ba+_worldserver.exe_[2025_10_30_13_36_1].txt` (Ghost aura assertion)

## Conclusion
The direct `ResurrectPlayer()` approach **works functionally** but has **critical stability issues** that make it unsuitable for production. We need to either:
1. Fix the thread safety/proc state issues
2. OR revert to a modified packet-based approach
3. OR find TrinityCore's intended resurrection API path

**Status:** Blocking production deployment until resolved.
