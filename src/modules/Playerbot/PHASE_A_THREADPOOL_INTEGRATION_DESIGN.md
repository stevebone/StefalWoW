# Phase A: ThreadPool Integration Design

**Date**: 2025-10-15
**Status**: PLANNING (CLAUDE.md Workflow)
**Goal**: Integrate existing ThreadPool with BotWorldSessionMgr for 7x speedup

---

## CLAUDE.md Acknowledgment ✅

**I acknowledge the mandatory workflow:**
- ✅ **No shortcuts** - Full implementation, no stubs
- ✅ **Module-first** - All changes in `src/modules/Playerbot/`
- ✅ **Complete solution** - No TODOs, full error handling
- ✅ **TrinityCore API** - Proper API usage validation
- ✅ **Enterprise grade** - Performance, completeness, sustainability

**File Modification Hierarchy:**
1. ✅ **PREFERRED**: Module-only implementation (`Session/BotWorldSessionMgr.cpp`)
2. ❌ Not needed: No core modifications required

---

## System Status

### ThreadPool Verification ✅

**Location**: `src/modules/Playerbot/Performance/ThreadPool/`
**Files**: `ThreadPool.h` (544 lines), `ThreadPool.cpp` (648 lines)
**Status**: **PRODUCTION-READY** ✅

**Features Verified**:
```cpp
namespace Playerbot::Performance {

// Lock-free work-stealing queue (Chase-Lev algorithm)
template<typename T>
class WorkStealingQueue {
    bool Push(T item);   // Owner thread - lock-free
    bool Pop(T& item);   // Owner thread - lock-free
    bool Steal(T& item); // Any thread - lock-free
};

// 5 Priority Levels
enum class TaskPriority : uint8 {
    CRITICAL = 0,  // 0-10ms tolerance
    HIGH     = 1,  // 10-50ms tolerance
    NORMAL   = 2,  // 50-200ms tolerance
    LOW      = 3,  // 200-1000ms tolerance
    IDLE     = 4   // no time constraints
};

// Thread Pool with work stealing
class ThreadPool {
    template<typename Func, typename... Args>
    auto Submit(TaskPriority priority, Func&& func, Args&&... args)
        -> std::future<std::invoke_result_t<...>>;

    bool WaitForCompletion(std::chrono::milliseconds timeout);
    size_t GetActiveThreads() const;
    size_t GetQueuedTasks() const;
};

// Global instance (lazy init)
ThreadPool& GetThreadPool();

} // namespace Playerbot::Performance
```

**Current Usage**: ONLY in tests (`ThreadingStressTest.cpp`)
**Not integrated**: BotWorldSessionMgr uses sequential iteration

---

## Priority Mapping Design

### BotPriority → TaskPriority Mapping

```cpp
namespace Playerbot {

/**
 * @brief Map bot priority to thread pool task priority
 *
 * This mapping ensures that bot update urgency translates to
 * appropriate thread pool scheduling priority.
 */
inline Performance::TaskPriority MapBotPriorityToTaskPriority(BotPriority botPriority)
{
    switch (botPriority)
    {
        case BotPriority::EMERGENCY:
            // Dead bots, critical health (< 20%), immediate resurrection needed
            return Performance::TaskPriority::CRITICAL;

        case BotPriority::HIGH:
            // Active combat, group content, requires immediate response
            return Performance::TaskPriority::HIGH;

        case BotPriority::MEDIUM:
            // Active movement, targeting, positioning
            return Performance::TaskPriority::NORMAL;

        case BotPriority::LOW:
            // Idle, resting, background activities
            return Performance::TaskPriority::LOW;

        case BotPriority::SUSPENDED:
            // Load shedding, minimal processing
            return Performance::TaskPriority::IDLE;

        default:
            return Performance::TaskPriority::NORMAL;
    }
}

} // namespace Playerbot
```

**Rationale**:
- EMERGENCY → CRITICAL: Death recovery must be immediate
- HIGH → HIGH: Combat responsiveness critical
- MEDIUM → NORMAL: Active but not time-critical
- LOW → LOW: Can tolerate 200-1000ms delay
- SUSPENDED → IDLE: Minimal resource allocation

---

## Integration Architecture

### Current System (Sequential)

**File**: `src/modules/Playerbot/Session/BotWorldSessionMgr.cpp`
**Function**: `BotWorldSessionMgr::UpdateSessions(uint32 diff)` (lines 251-473)

**Current Flow**:
```cpp
void BotWorldSessionMgr::UpdateSessions(uint32 diff)
{
    // PHASE 1: Collect sessions (mutex protected)
    {
        std::lock_guard<std::mutex> lock(_sessionsMutex);
        for (auto& [guid, session] : _botSessions) {
            if (ShouldUpdateThisTick(guid))
                sessionsToUpdate.push_back({guid, session});
        }
    } // Release mutex

    // PHASE 2: Update sessions SEQUENTIALLY (BOTTLENECK!)
    for (auto& [guid, session] : sessionsToUpdate) {
        session->Update(diff);  // ~1ms per bot
        // Adaptive priority adjustment...
    }
    // 145 bots × 1ms = 145ms (could be 145ms ÷ 8 threads = 18ms!)

    // PHASE 3: Cleanup
    // PHASE 4: Enterprise monitoring
}
```

**Problem**: Sequential iteration with 145 bots takes 145ms
**Solution**: Parallel execution with 8 threads = 18ms (7x speedup)

### Target System (Parallel ThreadPool)

**Modified Flow**:
```cpp
void BotWorldSessionMgr::UpdateSessions(uint32 diff)
{
    using namespace Performance;

    // PHASE 1: Collect sessions (unchanged)
    {
        std::lock_guard<std::mutex> lock(_sessionsMutex);
        // ... collect sessionsToUpdate ...
    }

    // PHASE 2: Submit tasks to ThreadPool (PARALLEL!)
    std::vector<std::future<void>> futures;
    futures.reserve(sessionsToUpdate.size());

    for (auto& [guid, botSession] : sessionsToUpdate) {
        // Get bot priority
        BotPriority botPriority = sBotPriorityMgr->GetPriority(guid);
        TaskPriority taskPriority = MapBotPriorityToTaskPriority(botPriority);

        // Submit to thread pool
        auto future = GetThreadPool().Submit(taskPriority, [&, guid, diff]() {
            // Validate session still valid
            if (!botSession || !botSession->IsActive())
                return;

            try {
                // Create PacketFilter
                class BotPacketFilter : public PacketFilter {
                public:
                    explicit BotPacketFilter(WorldSession* session) : PacketFilter(session) {}
                    virtual ~BotPacketFilter() override = default;
                    bool Process(WorldPacket*) override { return true; }
                    bool ProcessUnsafe() const override { return true; }
                } filter(botSession.get());

                // Update bot session
                if (!botSession->Update(diff, filter))
                    return; // Failed

                // Adaptive priority adjustment
                if (_enterpriseMode && botSession->IsLoginComplete()) {
                    Player* bot = botSession->GetPlayer();
                    if (bot && bot->IsInWorld()) {
                        // Adaptive frequency logic...
                        AutoAdjustPriority(bot, guid, currentTime);
                    }
                }
            }
            catch (std::exception const& e) {
                TC_LOG_ERROR("module.playerbot.session",
                    "Exception updating bot {}: {}", guid.ToString(), e.what());
                sBotHealthCheck->RecordError(guid, "UpdateException");
            }
        });

        futures.push_back(std::move(future));
    }

    // PHASE 2.5: SYNCHRONIZATION BARRIER - Wait for all tasks
    for (auto& future : futures) {
        try {
            future.get(); // Wait for completion + exception propagation
        }
        catch (std::exception const& e) {
            TC_LOG_ERROR("module.playerbot.session",
                "ThreadPool task exception: {}", e.what());
        }
    }

    // PHASE 3: Cleanup (unchanged)
    // PHASE 4: Enterprise monitoring (unchanged)
}
```

---

## Performance Analysis

### Current Performance (Sequential)

**Configuration**: 896 bots, update spreading active
**Metrics**:
```
Bots per tick: 145 bots
Tick time: 3ms
Per-bot time: 3ms ÷ 145 = 0.021ms
```

**Why so fast?** Previous optimizations:
1. Update spreading (no spikes)
2. Priority-based skipping (750 bots skipped)
3. Adaptive frequency (idle bots updated rarely)
4. Smart hysteresis (prevent thrashing)

### Projected Performance (ThreadPool)

**With 8 worker threads**:
```
Bots per tick: 145 bots
Threads: 8 workers
Bots per thread: 145 ÷ 8 = 18 bots
Time per thread: 18 × 0.021ms = 0.38ms
Speedup: 3ms ÷ 0.38ms = 7.9x
```

**Expected Result**: 3ms → 0.4ms (7x speedup)

### Scaling Projection

**Current** (896 bots, sequential):
```
145 bots/tick × 0.021ms = 3ms
```

**With ThreadPool** (896 bots):
```
(145 ÷ 8) × 0.021ms = 0.38ms
```

**Scaled to 5000 bots**:
```
Bots per tick: 5000 ÷ 50 (spread) = 100 bots/tick
With ThreadPool: (100 ÷ 8) × 0.021ms = 0.26ms
```

**Scaled to 10,000 bots**:
```
Bots per tick: 10000 ÷ 50 (spread) = 200 bots/tick
With ThreadPool: (200 ÷ 8) × 0.021ms = 0.53ms
```

**Conclusion**: ThreadPool enables **10,000+ bot scaling** at reasonable performance

---

## Thread Safety Analysis

### Data Race Concerns

**1. BotSession::Update()**
- **Status**: Thread-safe ✅
- **Rationale**: Each BotSession is independent, no shared mutable state
- **Caveat**: Must not access `_botSessions` map during update (already handled)

**2. BotPriorityManager**
- **Status**: Thread-safe ✅
- **Design**: Uses `std::mutex` on all map access
- **Methods**:
  - `GetPriority()` - mutex protected ✅
  - `SetPriority()` - mutex protected ✅
  - `AutoAdjustPriority()` - mutex protected ✅

**3. BotPerformanceMonitor**
- **Status**: Thread-safe ✅
- **Design**: Atomic operations for metrics
- **Methods**:
  - `RecordBotUpdateTime()` - atomic ✅
  - `GetMetrics()` - atomic reads ✅

**4. BotHealthCheck**
- **Status**: Thread-safe ✅
- **Design**: Mutex protected error tracking

**5. Shared Container (`sessionsToUpdate`)**
- **Status**: Safe ✅
- **Rationale**: Read-only after collection phase, futures keep sessions alive

### Lifetime Management

**Problem**: `shared_ptr<BotSession>` must remain valid during parallel execution

**Solution**: Capture `shared_ptr` by value in lambda
```cpp
// WRONG - Captures reference (dangling after sessionsToUpdate destroyed)
[&, guid, diff]() { botSession->Update(diff); }

// CORRECT - Captures shared_ptr by value (increments refcount)
[botSession, guid, diff]() { botSession->Update(diff); }
```

**Revised Lambda Capture**:
```cpp
auto future = GetThreadPool().Submit(taskPriority,
    [botSession, guid, diff, this]() {  // Capture shared_ptr by value!
        // botSession refcount incremented, safe from destruction
        botSession->Update(diff);
        // ...
    });
```

---

## Error Handling Strategy

### Exception Safety

**ThreadPool Behavior**:
- Exceptions in tasks are stored in `std::future`
- `future.get()` rethrows exception
- Pool remains operational after exception

**Strategy**:
```cpp
// Inside task lambda
try {
    botSession->Update(diff);
    // ... other operations ...
}
catch (std::exception const& e) {
    TC_LOG_ERROR("module.playerbot.session",
        "Bot {} update exception: {}", guid.ToString(), e.what());
    sBotHealthCheck->RecordError(guid, "UpdateException");
    // Don't rethrow - let other bots continue
}
catch (...) {
    TC_LOG_ERROR("module.playerbot.session",
        "Bot {} unknown exception", guid.ToString());
    sBotHealthCheck->RecordError(guid, "UnknownException");
}

// In main thread (future.get())
try {
    future.get();
}
catch (std::exception const& e) {
    // Should not reach here if lambda catches all exceptions
    TC_LOG_ERROR("module.playerbot.session",
        "ThreadPool task exception: {}", e.what());
}
```

### Disconnection Handling

**Current**: Collect disconnected sessions, remove after loop
**ThreadPool**: Same strategy, but use atomic flag or concurrent queue

**Option 1: Concurrent Queue** (Recommended)
```cpp
#include <tbb/concurrent_queue.h>

tbb::concurrent_queue<ObjectGuid> disconnectedSessions;

// In task lambda
if (update_failed) {
    disconnectedSessions.push(guid);
}

// After all futures complete
ObjectGuid guid;
while (disconnectedSessions.try_pop(guid)) {
    std::lock_guard<std::mutex> lock(_sessionsMutex);
    _botSessions.erase(guid);
    sBotPriorityMgr->RemoveBot(guid);
}
```

**Option 2: Atomic Flag + Vector** (Simpler, if TBB not available)
```cpp
std::mutex disconnectMutex;
std::vector<ObjectGuid> disconnectedSessions;

// In task lambda
if (update_failed) {
    std::lock_guard<std::mutex> lock(disconnectMutex);
    disconnectedSessions.push_back(guid);
}

// After all futures complete
std::lock_guard<std::mutex> lock(_sessionsMutex);
for (ObjectGuid const& guid : disconnectedSessions) {
    _botSessions.erase(guid);
    sBotPriorityMgr->RemoveBot(guid);
}
```

---

## Implementation Plan

### Step 1: Add Priority Mapping Function

**File**: `src/modules/Playerbot/Session/BotWorldSessionMgr.cpp`
**Location**: Before `BotWorldSessionMgr::UpdateSessions()`

```cpp
namespace {

// Helper function for mapping bot priority to task priority
Performance::TaskPriority MapBotPriorityToTaskPriority(BotPriority botPriority)
{
    switch (botPriority)
    {
        case BotPriority::EMERGENCY:
            return Performance::TaskPriority::CRITICAL;
        case BotPriority::HIGH:
            return Performance::TaskPriority::HIGH;
        case BotPriority::MEDIUM:
            return Performance::TaskPriority::NORMAL;
        case BotPriority::LOW:
            return Performance::TaskPriority::LOW;
        case BotPriority::SUSPENDED:
            return Performance::TaskPriority::IDLE;
        default:
            return Performance::TaskPriority::NORMAL;
    }
}

} // anonymous namespace
```

### Step 2: Add ThreadPool Header

**File**: `src/modules/Playerbot/Session/BotWorldSessionMgr.cpp`
**Location**: Top of file (after existing includes)

```cpp
#include "../Performance/ThreadPool/ThreadPool.h"
#include <tbb/concurrent_queue.h> // For disconnection handling
```

### Step 3: Modify UpdateSessions Function

**File**: `src/modules/Playerbot/Session/BotWorldSessionMgr.cpp`
**Function**: `BotWorldSessionMgr::UpdateSessions(uint32 diff)` (lines 251-473)

**Changes**:
1. After session collection (Phase 1), submit tasks to ThreadPool
2. Wait for all futures to complete (synchronization barrier)
3. Use concurrent queue for disconnection tracking
4. Preserve enterprise monitoring hooks

### Step 4: Add Configuration

**File**: `src/modules/Playerbot/conf/playerbots.conf.dist`
**New Settings**:

```ini
###################################################################################################
# THREAD POOL SETTINGS
###################################################################################################

# Enable ThreadPool for parallel bot updates (0 = disabled, 1 = enabled)
# Provides 7x speedup with 8 threads
Playerbot.ThreadPool.Enable = 1

# Number of worker threads (default: hardware_concurrency - 2)
# Set to 0 to use default
Playerbot.ThreadPool.WorkerCount = 0

# Enable work-stealing for automatic load balancing (0 = disabled, 1 = enabled)
Playerbot.ThreadPool.EnableWorkStealing = 1

# Enable CPU affinity for cache locality (0 = disabled, 1 = enabled)
# Requires elevated permissions on Windows
Playerbot.ThreadPool.EnableCpuAffinity = 0

# Maximum steal attempts before sleeping (default: 3)
Playerbot.ThreadPool.MaxStealAttempts = 3
```

### Step 5: Add ThreadPool Initialization

**File**: `src/modules/Playerbot/Session/BotWorldSessionMgr.cpp`
**Function**: `BotWorldSessionMgr::Initialize()`

```cpp
bool BotWorldSessionMgr::Initialize()
{
    // ... existing code ...

    // Initialize ThreadPool if enabled
    if (PlayerbotConfig::Get("Playerbot.ThreadPool.Enable", true))
    {
        try {
            Performance::GetThreadPool(); // Lazy initialization
            TC_LOG_INFO("module.playerbot.session",
                "ThreadPool initialized with {} worker threads",
                Performance::GetThreadPool().GetWorkerCount());
        }
        catch (std::exception const& e) {
            TC_LOG_ERROR("module.playerbot.session",
                "Failed to initialize ThreadPool: {}", e.what());
            return false;
        }
    }

    return true;
}
```

---

## Testing Strategy

### Phase 1: Unit Tests

**Test**: Priority mapping correctness
```cpp
TEST(BotWorldSessionMgr, PriorityMapping)
{
    EXPECT_EQ(MapBotPriorityToTaskPriority(BotPriority::EMERGENCY),
              Performance::TaskPriority::CRITICAL);
    EXPECT_EQ(MapBotPriorityToTaskPriority(BotPriority::HIGH),
              Performance::TaskPriority::HIGH);
    // ... etc
}
```

### Phase 2: Integration Tests

**Test**: 100 bots with ThreadPool
1. Spawn 100 bots
2. Measure tick time over 100 ticks
3. Verify all bots update correctly
4. Check for race conditions (use ThreadSanitizer)

### Phase 3: Stress Tests

**Test**: 896 bots with ThreadPool
1. Spawn 896 bots (current production load)
2. Measure tick time over 1000 ticks
3. Verify 7x speedup (3ms → 0.4ms)
4. Profile thread utilization (expect >90%)

### Phase 4: Scaling Tests

**Test**: 5000 bots with ThreadPool
1. Spawn 5000 bots (target scale)
2. Measure tick time, CPU usage
3. Verify < 10ms per tick
4. Check for memory leaks

---

## Risk Assessment

| Risk | Severity | Mitigation |
|------|----------|------------|
| Race conditions | HIGH | Thread-safe BotPriorityManager, capture shared_ptr by value |
| Deadlocks | MEDIUM | No mutex held during task execution |
| Memory leaks | MEDIUM | Use shared_ptr, verify refcount management |
| Exception propagation | LOW | Catch all exceptions in lambda, log errors |
| Performance regression | LOW | Fallback: disable ThreadPool via config |
| Build dependency (TBB) | LOW | TBB already used in BotWorldSessionMgrOptimized.h |

---

## Success Criteria

### Performance Metrics

- [x] Current baseline: 3ms per tick (896 bots)
- [ ] With ThreadPool: < 0.5ms per tick (7x speedup)
- [ ] Thread utilization: > 90%
- [ ] CPU usage: < 50% (8 threads)
- [ ] No performance regression

### Quality Metrics

- [ ] Zero race conditions (verified with ThreadSanitizer)
- [ ] Zero deadlocks (stress tested 10,000 ticks)
- [ ] Zero memory leaks (verified with Valgrind/Dr. Memory)
- [ ] All bots update correctly
- [ ] Enterprise monitoring preserved

### Scalability Metrics

- [ ] 896 bots: < 0.5ms per tick
- [ ] 5000 bots: < 3ms per tick
- [ ] 10000 bots: < 6ms per tick

---

## Rollback Plan

**If ThreadPool integration fails or causes issues:**

1. **Configuration Disable**:
   ```ini
   Playerbot.ThreadPool.Enable = 0
   ```

2. **Code Fallback**:
   ```cpp
   if (PlayerbotConfig::Get("Playerbot.ThreadPool.Enable", true)) {
       // Use ThreadPool (new code)
   } else {
       // Use sequential iteration (old code)
   }
   ```

3. **Build Flag** (if necessary):
   ```cmake
   option(ENABLE_PLAYERBOT_THREADPOOL "Enable ThreadPool" ON)
   ```

---

**Status**: DESIGN COMPLETE - Ready for Implementation
**Next Step**: Implement ThreadPool integration in UpdateSessions
**Expected Completion**: 1-2 days
**Expected Gain**: 7x speedup (3ms → 0.4ms)
