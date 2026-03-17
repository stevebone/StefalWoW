# Resurrection Bug Investigation - Complete Analysis
## Date: 2025-10-30 07:50

---

## Investigation Summary

**Duration**: 2+ hours extensive investigation
**Bugs Found**: 3 critical bugs
**Fixes Applied**: 3 fixes
**Status**: Partially working (ghost detection ✅, packet processing ❌)

---

## Bug #1: Ghost Detection at Login ✅ FIXED

### Problem
Bots that died before server restart loaded in ghost form (1 HP, Ghost aura, PLAYER_FLAGS_GHOST) but resurrection never triggered because:
```cpp
// OLD CODE (line 1113):
if (player->isDead())  // Only checks DEAD/CORPSE states, misses ALIVE+GHOST!
```

### Root Cause
- `isDead()` = checks `m_deathState == DEAD || CORPSE`
- Ghosts have `m_deathState == ALIVE` with `PLAYER_FLAGS_GHOST` flag
- Function returned FALSE for ghosts!

### Fix Applied
**File**: `src/modules/Playerbot/Session/BotSession.cpp` (line 1116)
```cpp
// FIXED CODE:
if (player->isDead() || player->HasPlayerFlag(PLAYER_FLAGS_GHOST))
```

### Result
✅ Ghosts ARE now detected at login
✅ DeathRecoveryManager::OnDeath() IS called
✅ Resurrection sequence initiates

---

## Bug #2: Missing WorldSession::Update() Call ❌ CRITICAL

### Problem
BotSession overrides `Update()` but NEVER calls parent class `WorldSession::Update()`!

**Impact**: Packets queued via `QueuePacket()` are **NEVER processed**!

### Evidence Chain
1. **Bots queue resurrection packet**:
   ```cpp
   // DeathRecoveryManager.cpp:1233
   WorldPacket* packet = new WorldPacket(CMSG_RECLAIM_CORPSE, 16);
   m_bot->GetSession()->QueuePacket(packet);  // ← Queues to _incomingPackets
   ```

2. **BotSession processes queue**:
   ```cpp
   // BotSession.cpp:854
   WorldSession::QueuePacket(packet.get());  // ← Re-queues to parent class
   ```

3. **But WorldSession::Update() is NEVER called**:
   ```cpp
   // BotSession.cpp:768 (OLD CODE)
   return true; // ← Returns without calling WorldSession::Update()!
   ```

4. **Result**: HandleReclaimCorpse is NEVER invoked!

### Fix Applied
**File**: `src/modules/Playerbot/Session/BotSession.cpp` (line 773)
```cpp
// CRITICAL FIX: Call WorldSession::Update() to process queued packets
try {
    WorldSession::Update(diff, updater);
}
catch (std::exception const& e) {
    TC_LOG_ERROR("module.playerbot.session", "Exception in WorldSession::Update for account {}: {}", GetAccountId(), e.what());
}
```

### Expected Result
✅ WorldSession::Update() processes packet queue
✅ HandleReclaimCorpse is called
✅ Resurrection completes

### Actual Result (TESTING REQUIRED)
❓ Handler not being called in logs (may be logging config issue)
❓ Need to verify with in-game testing

---

## Bug #3: No Diagnostic Logging

### Problem
HandleReclaimCorpse has NO logging, making debugging impossible.

### Fix Applied
**File**: `src/server/game/Handlers/MiscHandler.cpp` (lines 419-477)

**Added comprehensive diagnostics**:
```cpp
TC_LOG_ERROR("playerbot.death", "🔍 RECLAIM_CORPSE: {} - Handler called...");
TC_LOG_ERROR("playerbot.death", "❌ RECLAIM_CORPSE: {} - FAILED: [reason]");
TC_LOG_ERROR("playerbot.death", "✅ RECLAIM_CORPSE: {} - ALL CHECKS PASSED!");
TC_LOG_ERROR("playerbot.death", "✅ RECLAIM_CORPSE: {} - ResurrectPlayer() called!");
```

**Shows which checks fail**:
1. IsAlive check
2. InArena check
3. PLAYER_FLAGS_GHOST check
4. Corpse existence check
5. Ghost time delay check
6. Distance check (39 yard radius)

### Current Issue
**No diagnostic logs appearing** despite code being deployed!

Possible causes:
1. Log level filtering (playerbot.death not configured)
2. WorldSession::Update() not being called properly
3. Packet not reaching handler for other reasons

---

## Complete Evidence

### Binary Deployment
```bash
build/bin/RelWithDebInfo/worldserver.exe  # 46M  Okt 30 07:46
M:/Wplayerbot/worldserver.exe             # 46M  Okt 30 07:46
```
✅ Timestamps match - binary IS deployed

### Packet Queuing (Working)
```
Bot Boone queued CMSG_RECLAIM_CORPSE packet (distance: 38.5y, deathState: 3)
Bot Cathrine queued CMSG_RECLAIM_CORPSE packet (distance: 38.0y, deathState: 3)
```
✅ Packets ARE being queued

### Handler Invocation (NOT Working)
```bash
grep "RECLAIM_CORPSE" /m/Wplayerbot/logs/Server.log
# 0 results
```
❌ Handler NEVER called (or logs not appearing)

### Resurrection Status (Failing)
```
⏳ Bot Boone waiting for resurrection... (30.0s elapsed, IsAlive=false)
Bot Boone resurrection failed: Resurrection did not complete
```
❌ Bots timeout after 30 seconds

---

## Files Modified

### 1. BotSession.cpp (2 changes)
**Line 1116**: Ghost detection at login
```cpp
if (player->isDead() || player->HasPlayerFlag(PLAYER_FLAGS_GHOST))
```

**Line 773**: WorldSession::Update() call
```cpp
WorldSession::Update(diff, updater);
```

### 2. MiscHandler.cpp (1 change)
**Lines 419-477**: Comprehensive diagnostic logging

---

## Testing Required

### Verification Steps
1. ✅ Binary deployed (verified: 07:46 timestamp)
2. ✅ Server started with new binary
3. ❓ WorldSession::Update() being called?
4. ❓ HandleReclaimCorpse being invoked?
5. ❓ Bots resurrecting?

### Debug Commands
```bash
# Check if handler is called
grep "RECLAIM_CORPSE" /m/Wplayerbot/logs/Server.log

# Check WorldSession::Update errors
grep "Exception in WorldSession::Update" /m/Wplayerbot/logs/Playerbot.log

# Check bot health status
grep -E "Bot.*health=" /m/Wplayerbot/logs/Playerbot.log | tail -20
```

### In-Game Testing
1. Select a dead bot character
2. Check if still in ghost form (gray screen)
3. Wait 30+ seconds for resurrection attempt
4. Check if bot comes back to life
5. Verify health returns to 100%

---

## Next Investigation Steps

### If Handler Still Not Called

**Option 1: Verify WorldSession::Update() is called**
Add temporary logging:
```cpp
// BotSession.cpp line 773
TC_LOG_ERROR("module.playerbot.session", "🔍 CALLING WorldSession::Update() for account {}", GetAccountId());
WorldSession::Update(diff, updater);
TC_LOG_ERROR("module.playerbot.session", "✅ WorldSession::Update() COMPLETED for account {}", GetAccountId());
```

**Option 2: Check packet filter**
```cpp
// Check if PacketFilter is blocking CMSG_RECLAIM_CORPSE
// WorldSession.h - check updater parameter
```

**Option 3: Add packet processing logs**
```cpp
// WorldSession.cpp - add logging in Update() to show packets being processed
```

### If Handler IS Called But Failing

Check which validation fails:
1. IsAlive check - should pass (bots are dead)
2. InArena check - should pass (not in arena)
3. PLAYER_FLAGS_GHOST check - should pass (bots have flag)
4. Corpse check - **may fail** (no corpse?)
5. Ghost time delay - **may fail** (30-second delay?)
6. Distance check - **may fail** (39 yard max, bots at 37.9-38.8y)

---

## Known Issues

### Distance Edge Case
Bots at 37.9-38.8 yards, limit is 39 yards.
**Status**: Should pass, but very close to limit.

### Corpse Existence
Bots loaded from DB in ghost form may not have corpse objects.
**Status**: Need to investigate corpse creation during death.

### Ghost Time Delay
30-second delay after death before resurrection allowed.
**Status**: Bots that died hours ago should have expired delay.

---

## Success Criteria

Resurrection is working when:
1. ✅ Ghost detection at login fires
2. ✅ CMSG_RECLAIM_CORPSE packet queued
3. ❓ WorldSession::Update() processes packet
4. ❓ HandleReclaimCorpse is invoked
5. ❓ All validation checks pass
6. ❓ ResurrectPlayer() is called
7. ❓ Bot returns to life (IsAlive=true)
8. ❓ Health returns to 100%
9. ❓ Combat/quest systems work

---

## Commit Message (When Complete)

```
fix(playerbot): Fix ghost resurrection after server restart

Problem 1: Ghost Detection
  Bots in ghost form (ALIVE + PLAYER_FLAGS_GHOST) not detected at login
  because isDead() only checks DEAD/CORPSE states.

Solution 1:
  Check both isDead() and HasPlayerFlag(PLAYER_FLAGS_GHOST) at login
  to trigger DeathRecoveryManager for ghosts.

Problem 2: Packet Processing
  BotSession::Update() overrides parent but never calls WorldSession::Update()
  causing queued packets (CMSG_RECLAIM_CORPSE) to never be processed.

Solution 2:
  Call WorldSession::Update(diff, updater) after bot AI update to process
  queued packets and invoke handlers like HandleReclaimCorpse.

Problem 3: No Diagnostics
  HandleReclaimCorpse had no logging, making debugging impossible.

Solution 3:
  Added comprehensive diagnostic logging showing which validation checks
  pass/fail during resurrection attempts.

Files Modified:
  - src/modules/Playerbot/Session/BotSession.cpp (lines 1116, 773)
  - src/server/game/Handlers/MiscHandler.cpp (lines 419-477)

Testing:
  - Ghost detection confirmed working (logs show detection)
  - Packet processing needs verification (handler logs not appearing)
  - In-game testing required to confirm resurrection works
```

---

**Investigation Status**: ONGOING
**Confidence**: 80% - Fixes are correct but testing incomplete
**Blocker**: Handler logs not appearing (logging config or other issue)
**Next Step**: In-game testing or add more diagnostic logging

