# ThreadPool Deadlock Analysis - October 20, 2025

## Executive Summary

**Issue**: 24+ worker threads are stuck in `_Primitive_wait_for` (condition variable wait) despite having completed nearly all tasks (1145/1146). The threads are sleeping but not waking up when new work arrives.

**Root Cause**: Race condition in the wake-up mechanism between `WorkerThread::Sleep()` and `WorkerThread::Wake()` causing lost wake signals.

**Impact**: Complete server deadlock after processing initial bot batch, preventing all further bot updates.

## Thread State Analysis

### Critical Observations
```
Worker Thread State (Thread 25652):
- _sleeping: true
- _running: true
- _wakeMutex: unlocked
- _wakeCv: waiting
- tasksCompleted: 31
- totalIdleTime: 144,762,054,475 (144 seconds!)
- stealAttempts: 78,990
- stealSuccesses: 23

ThreadPool State:
- totalSubmitted: 1146
- totalCompleted: 1145
- totalFailed: 0
- Difference: 1 task (likely stuck or lost)

All Local Queues: EMPTY
- Queue[0]: _bottom=0, _top=0 (empty)
- Queue[1]: _bottom=3, _top=3 (empty)
- Queue[2]: _bottom=1, _top=1 (empty)
- Queue[3]: _bottom=11, _top=11 (empty)
- Queue[4]: _bottom=0, _top=0 (empty)
```

## Root Cause Identification

### Primary Issue: Lost Wake Signal Race Condition

The deadlock occurs due to a classic lost wake-up problem in the sleep/wake mechanism:

**Location**: `ThreadPool.cpp`, lines 389-413 (Sleep method) and 359-366 (Wake method)

#### The Race Condition Sequence

```cpp
// Thread A (Worker sleeping)         // Thread B (Submitter waking)
// ================================   // ================================
                                      Wake() called {
                                        if (_sleeping.load()) // false, not sleeping yet
                                          return;  // Wake signal lost!
                                      }

_sleeping.store(true);

unique_lock lock(_wakeMutex);
_wakeCv.wait_for(...);  // Sleeps forever, wake already missed
```

### The Problematic Code

**WorkerThread::Sleep()** (lines 389-413):
```cpp
void WorkerThread::Sleep()
{
    if (!_running.load() || _pool->IsShuttingDown())
        return;

    _sleeping.store(true, std::memory_order_relaxed);  // PROBLEM: Set BEFORE lock

    std::unique_lock<std::mutex> lock(_wakeMutex, std::try_to_lock);
    if (lock.owns_lock())
    {
        _wakeCv.wait_for(lock, timeout, [this]() {
            return !_running.load() || _pool->IsShuttingDown();
        });
    }
    else
    {
        std::this_thread::yield();
    }

    _sleeping.store(false, std::memory_order_relaxed);
}
```

**WorkerThread::Wake()** (lines 359-366):
```cpp
void WorkerThread::Wake()
{
    if (_sleeping.load(std::memory_order_relaxed))  // PROBLEM: Check outside lock
    {
        std::lock_guard<std::mutex> lock(_wakeMutex);
        _wakeCv.notify_one();
    }
}
```

### Secondary Issue: Work Stealing Inefficiency

78,990 steal attempts with only 23 successes (0.03% success rate) indicates:
1. Workers are waking up and trying to steal but finding no work
2. This creates unnecessary CPU usage and contention
3. The steal algorithm doesn't back off properly

### Tertiary Issue: Missing Task

1146 submitted - 1145 completed = 1 missing task
- Either stuck in a queue that appears empty due to memory ordering issues
- Or lost due to a race condition in the Submit path

## Why Previous Fixes Didn't Solve This

The previous fixes addressed:
1. **60s hang**: Fixed thread creation blocking ✓
2. **Configuration validation**: Fixed numThreads=0 crash ✓
3. **Early logging deadlock**: Fixed logger initialization ✓
4. **CPU affinity blocking**: Disabled SetThreadAffinityMask ✓

But they didn't fix the fundamental sleep/wake race condition that's causing this new deadlock.

## Proposed Solutions

### Solution 1: Fix the Wake Signal Race (IMMEDIATE FIX)

**Correct Sleep Implementation**:
```cpp
void WorkerThread::Sleep()
{
    if (!_running.load() || _pool->IsShuttingDown())
        return;

    std::unique_lock<std::mutex> lock(_wakeMutex);
    _sleeping.store(true, std::memory_order_relaxed);  // Set AFTER lock

    // Double-check work availability before sleeping
    if (HasWork()) {
        _sleeping.store(false, std::memory_order_relaxed);
        return;
    }

    _wakeCv.wait_for(lock, _pool->GetConfiguration().workerSleepTime, [this]() {
        return !_sleeping.load() || !_running.load() || _pool->IsShuttingDown() || HasWork();
    });

    _sleeping.store(false, std::memory_order_relaxed);
}
```

**Correct Wake Implementation**:
```cpp
void WorkerThread::Wake()
{
    std::lock_guard<std::mutex> lock(_wakeMutex);  // Lock FIRST
    _sleeping.store(false, std::memory_order_relaxed);  // Clear flag under lock
    _wakeCv.notify_one();  // Always notify
}
```

### Solution 2: Add Work Availability Check

```cpp
bool WorkerThread::HasWork() const
{
    for (size_t i = 0; i < static_cast<size_t>(TaskPriority::COUNT); ++i)
    {
        if (!_localQueues[i].Empty())
            return true;
    }
    return false;
}
```

### Solution 3: Periodic Wake-All Safety Net

In `ThreadPool::Submit()`:
```cpp
// After submitting task
worker->Wake();

// Safety net: Wake all sleeping workers if many tasks pending
if (GetQueuedTasks() > _workers.size() * 2) {
    for (auto& w : _workers) {
        w->Wake();
    }
}
```

### Solution 4: Fix Work Stealing Backoff

```cpp
bool WorkerThread::TryStealTask()
{
    // Exponential backoff for failed attempts
    uint32 backoffMs = 1;

    for (uint32 attempt = 0; attempt < maxAttempts; ++attempt)
    {
        // ... steal attempt code ...

        if (!success && attempt < maxAttempts - 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(backoffMs));
            backoffMs *= 2;  // Exponential backoff
        }
    }
}
```

## Implementation Plan

### Step 1: Fix Sleep/Wake Race (CRITICAL)
1. Modify `WorkerThread::Sleep()` to set `_sleeping` flag AFTER acquiring lock
2. Modify `WorkerThread::Wake()` to acquire lock BEFORE checking `_sleeping`
3. Add `HasWork()` check before sleeping
4. Clear `_sleeping` flag in Wake under lock

### Step 2: Add Debugging
1. Add atomic counter for wake calls sent vs received
2. Log when threads go to sleep and wake up
3. Track task submission and completion

### Step 3: Implement Safety Nets
1. Periodic wake-all when tasks are queued
2. Maximum sleep timeout (e.g., 100ms)
3. Work availability double-check before sleep

### Step 4: Optimize Work Stealing
1. Implement exponential backoff
2. Reduce max steal attempts
3. Add success rate tracking

## Testing Strategy

### Unit Tests
```cpp
TEST(ThreadPool, NoLostWakeups) {
    ThreadPool pool(4);
    std::atomic<int> counter{0};

    // Submit 1000 tasks rapidly
    for (int i = 0; i < 1000; ++i) {
        pool.Submit(TaskPriority::NORMAL, [&counter]() {
            counter.fetch_add(1);
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        });
    }

    // Wait for completion
    pool.WaitForCompletion(std::chrono::seconds(5));

    EXPECT_EQ(counter.load(), 1000);  // All tasks must complete
}
```

### Integration Tests
1. Spawn 100 bots, verify all update
2. Rapid submit/wait cycles
3. Stress test with varying priorities
4. Long-running tasks mixed with short tasks

## Files to Modify

1. **src/modules/Playerbot/Performance/ThreadPool/ThreadPool.cpp**
   - Lines 389-413: Fix Sleep() method
   - Lines 359-366: Fix Wake() method
   - Lines 298-351: Improve TryStealTask() backoff

2. **src/modules/Playerbot/Performance/ThreadPool/ThreadPool.h**
   - Add HasWork() method declaration
   - Add wake counter metrics

## Immediate Actions Required

1. **Apply the Sleep/Wake fix** - This is the critical fix that will resolve the deadlock
2. **Add work availability check** - Prevents sleeping when work is available
3. **Implement wake-all safety net** - Ensures no tasks get stuck
4. **Deploy and monitor** - Watch for any remaining deadlock scenarios

## Expected Outcome

After applying these fixes:
- Worker threads will reliably wake when work is submitted
- No lost wake signals due to race conditions
- Reduced CPU usage from failed steal attempts
- Complete elimination of the deadlock condition
- All 1146 tasks will complete successfully

## Risk Assessment

**Low Risk**: The changes are localized to the ThreadPool sleep/wake mechanism
**High Impact**: Fixes a critical deadlock affecting all bot processing
**Backwards Compatible**: No API changes, only internal synchronization improvements

## Conclusion

The deadlock is caused by a classic lost wake-up race condition in the ThreadPool's sleep/wake mechanism. The `_sleeping` flag is set before acquiring the mutex, allowing Wake() to check and return before the thread actually waits on the condition variable. The fix involves proper synchronization of the sleep/wake mechanism with the flag updates happening under mutex protection.

This is a critical bug that must be fixed immediately as it causes complete server lockup after initial bot processing.