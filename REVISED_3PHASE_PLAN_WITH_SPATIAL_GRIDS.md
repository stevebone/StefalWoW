# REVISED 3-PHASE ACTION PLAN
**Updated**: October 27, 2025 22:50
**Key Finding**: We ALREADY have lock-free spatial grid system in place!

---

## CURRENT SITUATION

### ✅ What We Have (GOOD)
- **Complete lock-free spatial grid system** with double-buffering
- **SpatialGridScheduler** for centralized updates (prevents deadlock)
- **49 AI files** already using spatial grid queries
- **Spatial grids updated BEFORE bot updates** (line 381-384 in BotWorldSessionMgr.cpp)

### ❌ What's Broken (BAD)
- **145 ObjectAccessor::Get calls** still exist in 43 files
- **ThreadPool synchronization barrier** blocks main thread
- **Main thread stuck at line 761** waiting for futures that never complete
- **Some bot AI code still using ObjectAccessor** instead of spatial grids

---

## ROOT CAUSE (REFINED)

The main thread hangs because:

1. **BotWorldSessionMgr::UpdateSessions()** submits 50-100 bot update tasks to ThreadPool
2. **Main thread BLOCKS** at line 761 waiting for futures to complete
3. **Some bot update tasks** call ObjectAccessor::GetUnit/GetCreature/etc
4. **ObjectAccessor acquires _objectsLock** (mutex contention)
5. **Main thread needs _objectsLock** for Map::SendObjectUpdates
6. **DEADLOCK** - main thread waiting for worker thread, worker thread waiting for main thread's lock

**The Fix**: Replace remaining 145 ObjectAccessor calls with spatial grid queries

---

## REVISED PHASE 1: Immediate Diagnosis (5 minutes)

### Goal
Identify WHICH bot AI code is calling ObjectAccessor and causing the hang

### Steps

1. **Enable TC_LOG_ERROR logging** in `worldserver.conf`:
   ```ini
   Logger.module.playerbot.session=5,Console Server
   ```

2. **Add diagnostic logging** around ObjectAccessor calls:

   Create a simple wrapper to identify blocking calls:
   ```cpp
   // In BotWorldSessionMgr.cpp, update lambda (line 576)
   auto updateLogic = [guid, botSession, diff, ...]()
   {
       TC_LOG_ERROR("module.playerbot.session", "⚠️ TASK START: {}", guid.ToString());

       auto startTime = std::chrono::steady_clock::now();

       try
       {
           botSession->Update(diff, filter);

           auto elapsed = std::chrono::steady_clock::now() - startTime;
           auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

           if (elapsedMs > 50)
           {
               TC_LOG_ERROR("module.playerbot.session",
                   "⚠️ SLOW UPDATE: Bot {} took {}ms", guid.ToString(), elapsedMs);
           }
       }
       catch (...) { /* ... */ }

       TC_LOG_ERROR("module.playerbot.session", "⚠️ TASK END: {}", guid.ToString());
   };
   ```

3. **Restart server** and watch logs for:
   - "TASK START" without matching "TASK END" (task hung)
   - "SLOW UPDATE" messages showing which bots are taking >50ms

4. **Identify the blocking bot**:
   - Note the GUID that never completes
   - Check what quest that bot is working on
   - Cross-reference with QUEST_INVESTIGATION_REPORT.md

### Expected Findings

Based on quest report, likely culprits:
- **Bots using quest items** (Quest 24470, 24471, 28813)
- **Bots searching for kill targets** (Quest 8326, 26389)

These probably call ObjectAccessor to find creatures/objects.

---

## REVISED PHASE 2: Quick Fix (15 minutes - FASTER NOW)

### Goal
Stop the main thread from freezing by removing synchronization barrier

### Option A: Fire-and-Forget (RECOMMENDED)

**Current code (BLOCKING)** - Lines 684-691, 722-783:
```cpp
// Submit task and save future
auto future = Performance::GetThreadPool().Submit(taskPriority, updateLogic);
futures.push_back(std::move(future));
futureGuids.push_back(guid);

// Wait for ALL futures (BLOCKS MAIN THREAD)
for (size_t i = 0; i < futures.size(); ++i)
{
    while (!completed && retries < MAX_RETRIES)
    {
        future.wait_for(TIMEOUT);  // <-- STUCK HERE AT LINE 761
    }
}
```

**Fixed code (NON-BLOCKING)**:
```cpp
// Submit task but DON'T save the future - let it complete in background
Performance::GetThreadPool().Submit(taskPriority, updateLogic);

// NO WAITING - main thread continues immediately!
// Tasks complete asynchronously and handle their own errors
```

### Changes Required

**File**: `src/modules/Playerbot/Session/BotWorldSessionMgr.cpp`

**Line 506-507** - Remove future storage:
```cpp
// BEFORE:
std::vector<std::future<void>> futures;
std::vector<ObjectGuid> futureGuids;

// AFTER:
// No future storage - fire and forget
```

**Lines 684-691** - Don't save futures:
```cpp
// BEFORE:
auto future = Performance::GetThreadPool().Submit(taskPriority, updateLogic);
futures.push_back(std::move(future));
futureGuids.push_back(guid);

// AFTER:
Performance::GetThreadPool().Submit(taskPriority, updateLogic);
// That's it - no storage, no waiting!
```

**Lines 707-783** - Remove entire synchronization barrier:
```cpp
// DELETE THESE 77 LINES:
// - WakeAllWorkers() call
// - Future waiting loop
// - Retry logic
// - Deadlock detection
```

### Benefits

- Main thread never blocks waiting for bot updates ✅
- Quest actions can execute immediately ✅
- Server won't crash from main thread hang ✅
- Reduced code complexity (77 lines deleted) ✅

### Trade-offs

- No synchronization - don't know when bot updates finish
- Can't detect disconnected sessions immediately
- Error handling is async (logged but not tracked)

**Solution**: Move session cleanup to NEXT tick instead of same tick

---

## REVISED PHASE 3: Permanent Fix (30-60 minutes - MUCH FASTER)

### Goal
Replace remaining 145 ObjectAccessor calls with spatial grid queries

### The Good News

We DON'T need to build a spatial grid system - **it's already done!**

We just need to replace blocking calls with lock-free queries.

### Step 1: Find the Worst Offenders (10 minutes)

Check which files have the MOST ObjectAccessor calls:
```bash
grep -r "ObjectAccessor::Get" src/modules/Playerbot/AI/ --include="*.cpp" -c | sort -t: -k2 -rn | head -20
```

Expected output from earlier search:
- ShamanAI.cpp: 18 calls
- BotThreatManager.cpp: 14 calls
- InterruptRotationManager.cpp: 8 calls
- ThreatCoordinator.cpp: 7 calls
- EnhancedBotAI.cpp: 6 calls
- BotAI_EventHandlers.cpp: 6 calls
- Monks/MonkAI.cpp: 6 calls

**Priority**: Fix these 7 files first (79 calls = 54% of total)

### Step 2: Check How Spatial Grids Are Used (5 minutes)

Look at files that ALREADY use spatial grids:
```bash
grep -A10 "SpatialGrid" src/modules/Playerbot/AI/Combat/TargetScanner.cpp | head -50
```

Learn the pattern:
```cpp
// BEFORE (BLOCKING):
Unit* target = ObjectAccessor::GetUnit(bot, targetGuid);

// AFTER (LOCK-FREE):
auto snapshot = sSpatialGridManager.GetSnapshot(bot->GetMapId());
Unit* target = snapshot.FindUnit(targetGuid);
```

### Step 3: Create Search-and-Replace Script (10 minutes)

Create `replace_objectaccessor.py`:
```python
#!/usr/bin/env python3
import re
import sys

# Pattern: ObjectAccessor::GetUnit(bot, guid)
# Replace: sSpatialGridManager.GetSnapshot(bot->GetMapId()).FindUnit(guid)

patterns = [
    # GetUnit
    (r'ObjectAccessor::GetUnit\((\w+),\s*([^)]+)\)',
     r'sSpatialGridManager.GetSnapshot(\1->GetMapId()).FindUnit(\2)'),

    # GetCreature
    (r'ObjectAccessor::GetCreature\((\w+),\s*([^)]+)\)',
     r'sSpatialGridManager.GetSnapshot(\1->GetMapId()).FindCreature(\2)'),

    # GetGameObject
    (r'ObjectAccessor::GetGameObject\((\w+),\s*([^)]+)\)',
     r'sSpatialGridManager.GetSnapshot(\1->GetMapId()).FindGameObject(\2)'),

    # GetPlayer
    (r'ObjectAccessor::GetPlayer\((\w+),\s*([^)]+)\)',
     r'sSpatialGridManager.GetSnapshot(\1->GetMapId()).FindPlayer(\2)'),
]

def replace_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()

    original = content

    for pattern, replacement in patterns:
        content = re.sub(pattern, replacement, content)

    if content != original:
        with open(filepath, 'w') as f:
            f.write(content)
        print(f"✅ Updated: {filepath}")
        return True

    return False

if __name__ == '__main__':
    for filepath in sys.argv[1:]:
        replace_file(filepath)
```

### Step 4: Apply to Top 7 Files (5 minutes)

```bash
python3 replace_objectaccessor.py \
    src/modules/Playerbot/AI/ClassAI/Shamans/ShamanAI.cpp \
    src/modules/Playerbot/AI/Combat/BotThreatManager.cpp \
    src/modules/Playerbot/AI/CombatBehaviors/InterruptRotationManager.cpp \
    src/modules/Playerbot/AI/Combat/ThreatCoordinator.cpp \
    src/modules/Playerbot/AI/EnhancedBotAI.cpp \
    src/modules/Playerbot/AI/BotAI_EventHandlers.cpp \
    src/modules/Playerbot/AI/ClassAI/Monks/MonkAI.cpp
```

This fixes 54% of blocking calls in 5 minutes!

### Step 5: Build and Test (10 minutes)

```bash
cmake --build build --target playerbot -j8
```

If compilation succeeds:
- Restart server
- Check logs for reduced "SLOW UPDATE" messages
- Verify quests progress (items used, targets engaged)

### Step 6: Apply to Remaining 36 Files (20 minutes)

```bash
# Generate list of all files with ObjectAccessor
grep -r "ObjectAccessor::Get" src/modules/Playerbot/AI/ --include="*.cpp" -l > objectaccessor_files.txt

# Apply replacements to all
python3 replace_objectaccessor.py $(cat objectaccessor_files.txt)

# Rebuild
cmake --build build --target playerbot -j8
```

This completes the permanent fix!

---

## PHASE 3 ALTERNATIVE: Manual Replacement Example

If you prefer manual replacement, here's the pattern for ONE file:

### Example: ShamanAI.cpp (18 calls)

**File**: `src/modules/Playerbot/AI/ClassAI/Shamans/ShamanAI.cpp`

**Line ~345** (example):
```cpp
// BEFORE:
Unit* enemy = ObjectAccessor::GetUnit(bot, _currentTarget);
if (!enemy || !enemy->IsAlive())
    return;

// AFTER:
auto snapshot = sSpatialGridManager.GetSnapshot(bot->GetMapId());
Unit* enemy = snapshot.FindUnit(_currentTarget);
if (!enemy || !enemy->IsAlive())
    return;
```

**Add include** at top of file:
```cpp
#include "../Spatial/SpatialGridManager.h"
```

Repeat for all 18 calls in this file (takes ~10 minutes manually).

---

## TIMELINE COMPARISON

### Original Plan (No Spatial Grids)
- Phase 1: 5 minutes
- Phase 2: 30 minutes
- Phase 3: 2-4 hours (building spatial grid system from scratch)
- **Total: 3-4.5 hours**

### Revised Plan (With Existing Spatial Grids)
- Phase 1: 5 minutes
- Phase 2: 15 minutes
- Phase 3: 30-60 minutes (just replacing calls)
- **Total: 50-80 minutes**

**Time Savings: 2-3.5 hours!** 🎉

---

## SUCCESS CRITERIA

After completing all 3 phases:

### Phase 1 Success
- [ ] DEBUG logging enabled
- [ ] Can see "TASK START" and "TASK END" in logs
- [ ] Identified which bot GUID is blocking
- [ ] Know which quest is causing the hang

### Phase 2 Success
- [ ] Main thread never blocks at line 761
- [ ] Server runs for 30+ minutes without crash
- [ ] Quest actions execute (items used, targets engaged)
- [ ] Bots complete at least ONE quest

### Phase 3 Success
- [ ] 0 ObjectAccessor calls in bot AI code
- [ ] All bot updates complete in <50ms
- [ ] No "SLOW UPDATE" warnings in logs
- [ ] Multiple quests completing successfully
- [ ] Quest reward selection system testable

---

## IMPLEMENTATION ORDER

1. **DO PHASE 2 FIRST** (15 minutes)
   - Removes synchronization barrier
   - Gets server stable immediately
   - Allows quest testing

2. **DO PHASE 1 SECOND** (5 minutes)
   - With stable server, can now diagnose safely
   - Identify which bots/quests are slow
   - Prioritize Phase 3 replacements

3. **DO PHASE 3 LAST** (30-60 minutes)
   - Fix identified slow paths first
   - Replace remaining calls systematically
   - Test after each batch of replacements

**Total Time: 50-80 minutes to complete fix**

---

**Report End**
