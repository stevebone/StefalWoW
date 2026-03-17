# MAIN THREAD BLOCKING - ROOT CAUSE ANALYSIS
**Generated**: October 27, 2025 22:45
**Issue**: Main thread stuck in BotWorldSessionMgr::UpdateSessions at line 761
**Impact**: Server hangs, quests cannot execute, crashes after ~10 minutes

---

## EXECUTIVE SUMMARY

The main thread is **BLOCKED** waiting for ThreadPool futures to complete in `BotWorldSessionMgr::UpdateSessions()`. One or more bot update tasks are never completing, causing the main thread to hang indefinitely in a retry loop.

**Critical Finding**: The stack trace shows the main thread stuck at **line 761**, which is INSIDE the future.wait_for() retry loop. This proves that at least one future is NOT completing even after multiple wake attempts.

---

## STACK TRACE ANALYSIS

```
worldserver.exe!Playerbot::BotWorldSessionMgr::UpdateSessions(unsigned int diff) Line 761
worldserver.exe!PlayerbotModule::OnWorldUpdate(unsigned int diff) Line 357
worldserver.exe!Trinity::ModuleUpdateManager::Update(unsigned int diff) Line 71
worldserver.exe!World::Update(unsigned int diff) Line 2346
worldserver.exe!WorldUpdateLoop() Line 598
```

**Line 761** in BotWorldSessionMgr.cpp:
```cpp
760:    "⚠️ Future {} of {} (bot {}) not ready after {}ms, waking workers (attempt {}/{})",
761:    i + 1, futures.size(), futureGuids[i].ToString(), retries * 100, retries, MAX_RETRIES);
```

This is the **WARNING LOG** inside the retry loop (lines 722-783).

---

## ROOT CAUSE: FUTURE NEVER COMPLETING

### The Blocking Code (Lines 722-783)

```cpp
for (size_t i = 0; i < futures.size(); ++i)
{
    auto& future = futures[i];
    bool completed = false;
    uint32 retries = 0;
    constexpr uint32 MAX_RETRIES = 5;  // 5 * 100ms = 500ms max wait
    constexpr auto TIMEOUT = std::chrono::milliseconds(100);

    while (!completed && retries < MAX_RETRIES)  // <-- MAIN THREAD STUCK HERE
    {
        auto status = future.wait_for(TIMEOUT);

        if (status == std::future_status::ready)
        {
            completed = true;
            future.get();
        }
        else
        {
            ++retries;

            if (retries % 2 == 0)
            {
                TC_LOG_WARN(...);  // <-- LINE 761 where main thread is stuck
                Performance::GetThreadPool().WakeAllWorkers();
            }
        }
    }

    if (!completed)  // <-- Should log DEADLOCK DETECTED but never reaches here
    {
        TC_LOG_FATAL("⚠️ DEADLOCK DETECTED: Future {} did not complete!");
    }
}
```

### Why This Blocks

1. **Main thread submits bot update tasks** to ThreadPool (lines 684-691)
2. **Each task returns a std::future<void>**
3. **Main thread MUST WAIT** for ALL futures to complete (synchronization barrier)
4. **If ANY future never completes**, main thread hangs forever in the retry loop
5. **The retry loop has MAX_RETRIES = 5** (500ms total), but code is stuck at retry attempt logging

---

## WHY FUTURES DON'T COMPLETE

Looking at the bot update task lambda (lines 576-677):

```cpp
auto updateLogic = [guid, botSession, diff, currentTime, ...]()
{
    TC_LOG_ERROR("⚠️ DEBUG: TASK START for bot {}", guid.ToString());

    try
    {
        // Create PacketFilter
        BotPacketFilter filter(botSession.get());

        // Update session - THIS IS WHERE THE TASK CAN HANG
        bool updateResult = botSession->Update(diff, filter);

        // ... priority adjustments ...
    }
    catch (...)
    {
        // Error handling
    }

    TC_LOG_ERROR("⚠️ DEBUG: TASK END for bot {}", guid.ToString());
};
```

### Potential Blocking Points in botSession->Update():

1. **ObjectAccessor calls** (known to cause deadlocks)
2. **Database queries** (synchronous queries block worker threads)
3. **Spatial grid locks** (if grid system uses locks)
4. **Map locks** (Map::Update() calls)
5. **Player state locks** (Player::Update() calls)

---

## EVIDENCE FROM LOGS

### No DEBUG Logging
The code has extensive DEBUG logging at lines 578-676:
```cpp
TC_LOG_ERROR("module.playerbot.session", "⚠️ DEBUG: TASK START for bot {}", guid.ToString());
TC_LOG_ERROR("module.playerbot.session", "⚠️ DEBUG: Creating PacketFilter...");
TC_LOG_ERROR("module.playerbot.session", "⚠️ DEBUG: About to call botSession->Update()...");
TC_LOG_ERROR("module.playerbot.session", "⚠️ DEBUG: botSession->Update() returned...");
```

**Finding**: NO DEBUG messages in Server.log
**Conclusion**: Either:
- Logging level is set too high (ERROR logs disabled)
- Tasks are never executing at all
- Tasks are stuck before reaching TASK START logging

### No DEADLOCK DETECTED Messages
The code should log `DEADLOCK DETECTED` after 500ms timeout, but it never appears in logs.
**Conclusion**: Main thread is stuck IN THE RETRY LOOP but never completes even a single retry cycle.

### No ThreadPool Saturation Warnings
Lines 554-560 should log if ThreadPool is saturated:
```cpp
if (queuedTasks > 100 || activeWorkers > busyThreshold)
{
    TC_LOG_WARN("ThreadPool saturated - skipping bot updates this tick");
    return;
}
```

**Finding**: NO saturation warnings in logs
**Conclusion**: ThreadPool is accepting tasks but not completing them

---

## CRITICAL ISSUE: SYNCHRONIZATION BARRIER DEADLOCK

The current architecture creates a **SYNCHRONIZATION BARRIER** where:

1. Main thread submits 50-100 bot update tasks to ThreadPool
2. Main thread BLOCKS waiting for ALL futures to complete
3. If ANY task blocks on ObjectAccessor, Map, or other locked resource
4. Main thread cannot proceed
5. Quest actions never execute
6. Server eventually crashes

### The Cascading Failure

```
Bot Update Task → Calls BotAI::UpdateAI()
                → Calls ObjectAccessor::GetUnit()
                → Acquires _objectsLock
                → Main thread waiting at line 761
                → Map::SendObjectUpdates needs _objectsLock
                → DEADLOCK
                → Server crash
```

---

## IMMEDIATE FIXES REQUIRED

### Fix #1: Enable DEBUG Logging
Add to `worldserver.conf`:
```
Logger.module.playerbot.session=5,Console Server
```
This will show us where tasks are actually blocking.

### Fix #2: Remove Synchronization Barrier (HIGH PRIORITY)
**Current (BLOCKING)**:
```cpp
for (auto& future : futures)
{
    future.wait_for(timeout);  // Main thread BLOCKS
}
```

**Solution A - Fire and Forget**:
```cpp
// Don't wait for futures - let them complete asynchronously
// Store futures in a deque and check only completed ones
std::deque<std::pair<ObjectGuid, std::future<void>>> _pendingFutures;
```

**Solution B - Timeout and Continue**:
```cpp
// Give tasks 1 tick to complete, if not, skip them and move on
for (auto& future : futures)
{
    if (future.wait_for(0ms) == std::future_status::ready)
        future.get();
    // Otherwise, future completes in background and we move on
}
```

### Fix #3: Remove ObjectAccessor from Bot Updates (CRITICAL)
Replace ALL ObjectAccessor calls in bot AI with:
- Spatial grid snapshots (lock-free)
- Cached Player pointers
- Event-driven updates instead of polling

### Fix #4: Timeout Individual Bot Updates
Add timeout to each bot's Update() call:
```cpp
auto future = std::async(std::launch::async, [&]() {
    return botSession->Update(diff, filter);
});

if (future.wait_for(50ms) == std::future_status::timeout)
{
    TC_LOG_ERROR("Bot {} update timeout - skipping");
    return;  // Don't block entire system for one slow bot
}
```

---

## RECOMMENDED ACTION PLAN

### Phase 1: Immediate Diagnosis (5 minutes)
1. ✅ Enable DEBUG logging in worldserver.conf
2. ✅ Restart server and capture first 100 log lines
3. ✅ Identify which bot GUID is blocking
4. ✅ Identify which code path is blocking (TASK START vs TASK END)

### Phase 2: Quick Fix (30 minutes)
1. ✅ Remove synchronization barrier - fire-and-forget futures
2. ✅ Add 50ms timeout per bot update
3. ✅ Log and skip slow bots instead of blocking main thread

### Phase 3: Permanent Fix (2-4 hours)
1. ✅ Audit ALL bot AI code for ObjectAccessor calls
2. ✅ Replace with spatial grid snapshots
3. ✅ Convert to event-driven architecture (no polling)
4. ✅ Implement per-bot health monitoring

---

## TESTING RECOMMENDATIONS

After implementing fixes:
1. Monitor `Server.log` for "TASK START" and "TASK END" messages
2. Verify all futures complete within 50ms
3. Confirm no "ThreadPool saturated" warnings
4. Validate quest actions execute (items used, targets engaged)
5. Run server for 30+ minutes without crashes

---

## APPENDIX: ThreadPool Architecture Issues

### Current Submission Logic (Lines 684-691)
```cpp
BotPriority botPriority = sBotPriorityMgr->GetPriority(guid);
Performance::TaskPriority taskPriority = MapBotPriorityToTaskPriority(botPriority);

auto future = Performance::GetThreadPool().Submit(taskPriority, updateLogic);
futures.push_back(std::move(future));
futureGuids.push_back(guid);
```

### Problem
- Submitting 50-100 tasks in rapid succession
- Each task can block on external resources
- Main thread MUST wait for ALL tasks
- One slow/blocked task stops entire system

### Better Architecture
```cpp
// Don't block main thread - process completions asynchronously
Performance::GetThreadPool().Submit(taskPriority, [guid, botSession, diff]() {
    bool result = botSession->Update(diff, filter);

    if (!result)
    {
        // Queue for cleanup on NEXT tick
        sBotWorldSessionMgr->QueueForRemoval(guid);
    }
});
// Main thread continues immediately - no waiting!
```

---

**Report End**
