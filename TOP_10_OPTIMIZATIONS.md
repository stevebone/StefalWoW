# TOP 10 PERFORMANCE OPTIMIZATIONS
## TrinityCore Playerbot Module - Prioritized Action List

**Generated**: 2025-10-16
**Target**: Achieve 5000 concurrent bots @ <50ms/tick
**Current Baseline**: 778 bots @ 60-80ms/tick (histogram disabled)

---

## PRIORITY MATRIX

```
HIGH IMPACT ↑
            │
    #1      │  #2    #3
  (Histogram)│(Branches)(Vectors)
            │
            │  #4    #5
            │(string_view)(PrepStmt)
            │
            │  #6    #7    #10
            │(CPU Aff)(Pool)(Spatial)
            │
            │  #8    #9
            │(DeadCode)(PerfCounters)
            │
LOW IMPACT  └─────────────────────→ HIGH EFFORT
            LOW     MEDIUM     HIGH
```

---

## #1: RESTORE HISTOGRAM WITH LOCK-FREE IMPLEMENTATION
**Priority**: CRITICAL
**Effort**: 2 hours
**Impact**: 10-15% speedup at 778+ bots
**Status**: NOT IMPLEMENTED

### Problem
```cpp
// BotPerformanceMonitor.cpp (Line 237)
void BotPerformanceMonitor::RecordBotUpdateTime(uint32 microseconds)
{
    // PERFORMANCE FIX: Skip histogram recording to reduce overhead
    // Histogram adds significant mutex contention with 778+ bots
    // _histogram.RecordTime(microseconds);  // DISABLED
}
```
**Root Cause**: Mutex contention at 778 bots × 20 updates/sec = 15,560 lock acquisitions/sec

### Solution
```cpp
class LockFreeHistogram
{
private:
    static constexpr uint32 BUCKET_COUNT = 100;
    static constexpr uint32 BUCKET_SIZE_MICROS = 100; // 100μs buckets
    std::array<std::atomic<uint32>, BUCKET_COUNT> _buckets;
    std::atomic<uint32> _totalCount{0};

public:
    void RecordTime(uint32 microseconds)
    {
        uint32 bucket = microseconds / BUCKET_SIZE_MICROS;
        if (bucket >= BUCKET_COUNT)
            bucket = BUCKET_COUNT - 1;

        _buckets[bucket].fetch_add(1, std::memory_order_relaxed);
        _totalCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32 GetPercentile(uint8 percentile) const
    {
        uint32 totalCount = _totalCount.load(std::memory_order_relaxed);
        if (totalCount == 0)
            return 0;

        uint32 targetCount = (totalCount * percentile) / 100;
        uint32 cumulative = 0;

        for (uint32 i = 0; i < BUCKET_COUNT; ++i)
        {
            cumulative += _buckets[i].load(std::memory_order_relaxed);
            if (cumulative >= targetCount)
                return i * BUCKET_SIZE_MICROS;
        }

        return (BUCKET_COUNT - 1) * BUCKET_SIZE_MICROS;
    }

    void Clear()
    {
        for (auto& bucket : _buckets)
            bucket.store(0, std::memory_order_relaxed);
        _totalCount.store(0, std::memory_order_relaxed);
    }
};
```

### Files to Change
- `C:\TrinityBots\TrinityCore\src\modules\Playerbot\Session\BotPerformanceMonitor.h` (Lines 18-93)
- `C:\TrinityBots\TrinityCore\src\modules\Playerbot\Session\BotPerformanceMonitor.cpp` (Lines 18-93, 233-238)

### Testing
```cmd
# Build and run with 778 bots
# Verify histogram recording enabled in log
# Check CPU usage < 80% (down from current 90-95%)
```

---

## #2: ADD [[likely]]/[[unlikely]] BRANCH HINTS
**Priority**: HIGH
**Effort**: 1.5 hours
**Impact**: 5-10% speedup in hot loops
**Status**: NOT IMPLEMENTED

### Changes Needed

#### BotWorldSessionMgr.cpp (Line 336)
```cpp
// BEFORE:
if (!session || !session->IsBot())
{
    sessionsToRemove.push_back(guid);
    continue;
}

// AFTER:
if (!session || !session->IsBot()) [[unlikely]]
{
    sessionsToRemove.push_back(guid);
    continue;
}
```

#### BotWorldSessionMgr.cpp (Line 563)
```cpp
// BEFORE:
if (!botSession || !botSession->IsActive())
{
    disconnectedSessions.push_back(guid);
    continue;
}

// AFTER:
if (!botSession || !botSession->IsActive()) [[unlikely]]
{
    disconnectedSessions.push_back(guid);
    continue;
}
```

#### GridQueryProcessor.cpp (Line 420)
```cpp
// BEFORE:
if (!creature || !creature->IsAlive())
    continue;

// AFTER:
if (!creature || !creature->IsAlive()) [[unlikely]]
    continue;
```

#### BotAI.cpp (Line 681)
```cpp
// BEFORE:
if (strategy && strategy->IsActive(this))
{
    activeStrategies.push_back(strategy);
}

// AFTER:
if (strategy && strategy->IsActive(this)) [[likely]]
{
    activeStrategies.push_back(strategy);
}
```

#### QuestManager.cpp (Line 289)
```cpp
// BEFORE:
if (!quest)
    return false;

// AFTER:
if (!quest) [[unlikely]]
    return false;
```

### Full Change List (20 locations)
1. `BotWorldSessionMgr.cpp`: Lines 336, 563, 351, 353, 364, 366, 370
2. `GridQueryProcessor.cpp`: Lines 420, 422, 532, 534, 621, 623
3. `BotAI.cpp`: Lines 350, 681, 886, 888
4. `QuestManager.cpp`: Lines 289, 293, 298

### Testing
```cmd
# Compile with MSVC /O2 optimization
# Profile with Intel VTune → Microarchitecture Exploration
# Verify branch prediction accuracy > 95%
```

---

## #3: RESERVE VECTOR/MAP CAPACITY
**Priority**: MEDIUM
**Effort**: 0.5 hours
**Impact**: 3-5% reduction in memory allocations
**Status**: NOT IMPLEMENTED

### Changes

#### BotWorldSessionMgr.cpp (Line 328)
```cpp
// BEFORE:
sessionsToUpdate.reserve(200); // Reserve for typical load

// AFTER:
sessionsToUpdate.reserve(_botSessions.size()); // Reserve exact size
```

#### GridQueryProcessor.cpp (Constructor)
```cpp
// ADD to constructor:
GridQueryProcessor::GridQueryProcessor()
{
    _resultCache.reserve(_config.maxCacheSize); // Pre-reserve 10,000 entries
    // NOTE: Don't log here - logging system may not be initialized yet
}
```

### Testing
```cmd
# Profile with Visual Studio Memory Profiler
# Compare "Allocations" count before/after
# Expected: 20-30% reduction in vector/map reallocations
```

---

## #4: USE std::string_view FOR LOGGING
**Priority**: MEDIUM
**Effort**: 2 hours
**Impact**: 2-3% reduction in string allocations
**Status**: NOT IMPLEMENTED

### Pattern
```cpp
// BEFORE:
TC_LOG_ERROR("module.playerbot", "Bot {} error", _bot->GetName());
// Problem: GetName() returns std::string by value → copy

// AFTER:
#define BOT_NAME_VIEW(_bot) std::string_view((_bot)->GetName().c_str(), (_bot)->GetName().length())
TC_LOG_ERROR("module.playerbot", "Bot {} error", BOT_NAME_VIEW(_bot));
// Solution: string_view wraps c_str → no copy
```

### Files to Change (~50 locations)
- `BotAI.cpp`: Lines 116, 156, 239-257, 309, 545, 808, 817, 827, 838
- `BotWorldSessionMgr.cpp`: Lines 63, 116, 186, 191, 223, 238, 356, 593
- `QuestManager.cpp`: Lines 239, 362, 382, 462, 496, 512
- `GridQueryProcessor.cpp`: Lines 78, 90, 144, 274

### Testing
```cmd
# Profile with Visual Studio Memory Profiler
# Filter by "std::string" allocations
# Expected: 40-50% reduction in string allocations during logging
```

---

## #5: REPLACE STRING CONCATENATION WITH PREPARED STATEMENT
**Priority**: MEDIUM
**Effort**: 1 hour
**Impact**: 10-20% faster database queries
**Status**: NOT IMPLEMENTED

### Change

#### BotWorldSessionMgr.cpp (Lines 783-784)
```cpp
// BEFORE:
std::string query = "SELECT guid, name, account FROM characters WHERE guid = " + std::to_string(playerGuid.GetCounter());
QueryResult result = CharacterDatabase.Query(query.c_str());

// AFTER:
CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_BASIC);
stmt->setUInt64(0, playerGuid.GetCounter());
QueryResult result = CharacterDatabase.Query(stmt);
```

### Note
May need to add `CHAR_SEL_CHARACTER_BASIC` to `CharacterDatabase.h` prepared statements enum:
```cpp
enum CharacterDatabaseStatements
{
    // ... existing statements
    CHAR_SEL_CHARACTER_BASIC, // SELECT guid, name, account FROM characters WHERE guid = ?
    // ... more statements
};
```

### Testing
```cmd
# Enable MySQL query log
# Compare query execution time before/after
# Expected: 10-20% faster (prepared statement uses cached plan)
```

---

## #6: ENABLE CPU AFFINITY (OPTIONAL, REQUIRES ADMIN)
**Priority**: LOW
**Effort**: 0.5 hours (configuration only)
**Impact**: 5-10% improvement in cache locality
**Status**: CONFIGURATION CHANGE NEEDED

### Change

#### ThreadPool.h (Line 366)
```cpp
// BEFORE:
bool enableCpuAffinity = false; // Disabled by default (requires admin on Windows)

// AFTER:
bool enableCpuAffinity = true; // ENABLED

// Note: Requires running worldserver.exe as Administrator on Windows
// SetThreadAffinityMask() requires elevated privileges
```

### Testing
```cmd
# Run worldserver.exe as Administrator
# Verify in Task Manager → Details → worldserver.exe → Set Affinity
# Should see worker threads pinned to specific CPU cores
```

### Documentation Update
Add to `playerbots.conf`:
```ini
# Enable CPU affinity for worker threads (requires Administrator privileges on Windows)
# When enabled, each worker thread is pinned to a specific CPU core
# This improves cache locality but requires running worldserver as admin
# Default: 0 (disabled)
Playerbot.Performance.EnableCPUAffinity = 0
```

---

## #7: IMPLEMENT OBJECT POOL FOR BotSession (CONDITIONAL)
**Priority**: LOW (CONDITIONAL)
**Effort**: 3 hours
**Impact**: 50% reduction in allocation overhead IF churn rate > 10 sessions/sec
**Status**: NOT IMPLEMENTED
**Recommendation**: SKIP - Only if profiling shows high allocation rate

### When to Implement
Profile first with Visual Studio Memory Profiler:
```cmd
# Check "Allocations" view
# Filter by "BotSession"
# If allocation rate > 10/sec AND high churn (frequent login/logout):
#   → Implement object pool
# Otherwise:
#   → Skip (current allocation overhead is acceptable)
```

### Implementation (Only if Needed)
```cpp
class BotSessionPool
{
    std::vector<std::unique_ptr<BotSession>> _pool;
    std::queue<BotSession*> _available;
    std::mutex _mutex;

public:
    BotSession* Acquire()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_available.empty())
        {
            auto session = std::make_unique<BotSession>();
            BotSession* ptr = session.get();
            _pool.push_back(std::move(session));
            return ptr;
        }
        BotSession* session = _available.front();
        _available.pop();
        return session;
    }

    void Release(BotSession* session)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        session->Reset(); // Clear state
        _available.push(session);
    }
};
```

---

## #8: REMOVE DEAD PreScanCache CODE
**Priority**: LOW
**Effort**: 0.5 hours
**Impact**: Code cleanliness only (no performance impact)
**Status**: CLEANUP TASK

### Files to Remove
- `C:\TrinityBots\TrinityCore\src\modules\Playerbot\Combat\PreScanCache.cpp` (entire file)
- `C:\TrinityBots\TrinityCore\src\modules\Playerbot\Combat\PreScanCache.h` (entire file)

### Files to Update
- `C:\TrinityBots\TrinityCore\src\modules\Playerbot\AI\BotAI.cpp` (Line 31 - remove #include)
- `C:\TrinityBots\TrinityCore\src\modules\Playerbot\CMakeLists.txt` (remove PreScanCache.cpp reference)

### Reason
PreScanCache was disabled due to ThreadPool deadlocks:
```cpp
// BotAI.cpp (Lines 90-100)
// DEADLOCK FIX: PerformPreScan() calls Cell::VisitAllObjects() which deadlocks on worker threads!
// BotAI constructor is called from worker threads during bot session creation.
// Pre-scan cache has been DISABLED to prevent ThreadPool deadlocks.
// Bots must now use GridQueryProcessor async queries instead of PreScanCache.
```

All functionality replaced by `GridQueryProcessor` async system.

---

## #9: ADD WINDOWS PERFORMANCE COUNTER INTEGRATION
**Priority**: LOW
**Effort**: 4 hours
**Impact**: Better profiling integration (no direct performance improvement)
**Status**: NOT IMPLEMENTED

### Implementation
```cpp
#include <Pdh.h>
#include <PdhMsg.h>
#pragma comment(lib, "Pdh.lib")

class WindowsPerformanceCounters
{
private:
    PDH_HQUERY _query = nullptr;
    PDH_HCOUNTER _cpuCounter = nullptr;
    PDH_HCOUNTER _memoryCounter = nullptr;
    PDH_HCOUNTER _threadCounter = nullptr;

public:
    bool Initialize()
    {
        if (PdhOpenQuery(nullptr, 0, &_query) != ERROR_SUCCESS)
            return false;

        // Add CPU usage counter
        PdhAddCounter(_query, L"\\Processor(_Total)\\% Processor Time", 0, &_cpuCounter);

        // Add memory usage counter
        PdhAddCounter(_query, L"\\Process(worldserver)\\Working Set - Private", 0, &_memoryCounter);

        // Add thread count counter
        PdhAddCounter(_query, L"\\Process(worldserver)\\Thread Count", 0, &_threadCounter);

        PdhCollectQueryData(_query); // Initial collection
        return true;
    }

    void Update()
    {
        PdhCollectQueryData(_query);

        PDH_FMT_COUNTERVALUE cpuValue;
        PdhGetFormattedCounterValue(_cpuCounter, PDH_FMT_DOUBLE, nullptr, &cpuValue);

        PDH_FMT_COUNTERVALUE memValue;
        PdhGetFormattedCounterValue(_memoryCounter, PDH_FMT_LARGE, nullptr, &memValue);

        PDH_FMT_COUNTERVALUE threadValue;
        PdhGetFormattedCounterValue(_threadCounter, PDH_FMT_LONG, nullptr, &threadValue);

        TC_LOG_INFO("playerbot.performance",
            "Windows Counters - CPU: {:.1f}%, Memory: {} MB, Threads: {}",
            cpuValue.doubleValue,
            memValue.largeValue / (1024 * 1024),
            threadValue.longValue);
    }

    ~WindowsPerformanceCounters()
    {
        if (_query)
            PdhCloseQuery(_query);
    }
};
```

### Integration
Add to `BotPerformanceMonitor`:
```cpp
// In BotPerformanceMonitor.h
#ifdef _WIN32
    std::unique_ptr<WindowsPerformanceCounters> _winPerfCounters;
#endif

// In BotPerformanceMonitor::Initialize()
#ifdef _WIN32
    _winPerfCounters = std::make_unique<WindowsPerformanceCounters>();
    _winPerfCounters->Initialize();
#endif

// In BotPerformanceMonitor::LogPerformanceReport()
#ifdef _WIN32
    _winPerfCounters->Update();
#endif
```

---

## #10: OPTIMIZE SPATIAL BATCHING WITH PERSISTENT CACHE
**Priority**: LOW
**Effort**: 2 hours
**Impact**: 20-30% reduction in duplicate grid queries
**Status**: NOT IMPLEMENTED

### Current Limitation
Spatial batching only shares results within same tick (Lines 254-284)
```cpp
// GridQueryProcessor::ProcessQueries()
std::unordered_map<uint64, SpatialBatch> tickBatches; // CLEARED every tick!
```

### Improvement
Extend spatial batching across multiple ticks with TTL:
```cpp
struct SpatialBatchCache
{
    GridQueryResult result;
    uint32 expirationTime;
    std::vector<ObjectGuid> subscribers;
};

// Add to GridQueryProcessor class:
std::unordered_map<uint64, SpatialBatchCache> _persistentBatches;
std::mutex _persistentBatchesMutex;

// In ProcessQueries():
{
    std::lock_guard<std::mutex> lock(_persistentBatchesMutex);
    auto batchIt = _persistentBatches.find(query.spatialKey);
    if (batchIt != _persistentBatches.end() && batchIt->second.expirationTime > currentTime)
    {
        // Reuse cached batch result (no Cell::VisitAllObjects call!)
        StoreResult(query.requesterId, query.type, batchIt->second.result);
        batchIt->second.subscribers.push_back(query.requesterId);
        continue;
    }
}

// After query execution:
{
    std::lock_guard<std::mutex> lock(_persistentBatchesMutex);
    SpatialBatchCache batch;
    batch.result = result;
    batch.expirationTime = currentTime + 200; // 200ms TTL
    batch.subscribers.push_back(query.requesterId);
    _persistentBatches[query.spatialKey] = batch;
}

// Periodic cleanup (every 10 ticks):
if (_tickCounter % 10 == 0)
{
    std::lock_guard<std::mutex> lock(_persistentBatchesMutex);
    for (auto it = _persistentBatches.begin(); it != _persistentBatches.end();)
    {
        if (it->second.expirationTime <= currentTime)
            it = _persistentBatches.erase(it);
        else
            ++it;
    }
}
```

### Testing
```cmd
# Profile with Visual Studio CPU Profiler
# Count Cell::VisitAllObjects() calls before/after
# Expected: 20-30% reduction (5-10 bots in same area share results)
```

---

## IMPLEMENTATION ROADMAP

### Week 1: Quick Wins (4 hours)
- [x] Day 1: Implement #2 (Branch hints) - 1.5 hrs
- [x] Day 2: Implement #3 (Vector reserves) - 0.5 hrs
- [x] Day 3-4: Implement #4 (string_view) - 2 hrs

**Expected Result**: 10-15% speedup, easy to test, low risk

### Week 2: Critical Fixes (3 hours)
- [x] Day 1-2: Implement #1 (Lock-free histogram) - 2 hrs
- [x] Day 3: Implement #5 (Prepared statements) - 1 hr

**Expected Result**: Restore histogram, 5% additional speedup

### Week 3: Optional Enhancements (2 hours)
- [x] Day 1: Implement #6 (CPU affinity) - 0.5 hrs
- [x] Day 2: Implement #10 (Spatial batching) - 2 hrs
- [x] Day 3: Testing and validation

**Expected Result**: 5% additional speedup, better cache locality

### Week 4: Polish & Cleanup (5 hours)
- [x] Day 1: Implement #8 (Remove dead code) - 0.5 hrs
- [x] Day 2-3: Implement #9 (Windows counters) - 4 hrs
- [x] Day 4: Final testing and documentation - 0.5 hrs

**Expected Result**: Clean codebase, production-ready monitoring

---

## EXPECTED PERFORMANCE IMPROVEMENTS

### Before Optimizations (Baseline)
- **100 bots**: ~8-10ms/tick
- **500 bots**: ~40-50ms/tick
- **778 bots**: ~60-80ms/tick (histogram disabled)
- **5000 bots**: ~300-400ms/tick (estimated, exceeds target)

### After All 10 Optimizations
- **100 bots**: ~6-8ms/tick (20-25% improvement)
- **500 bots**: ~30-40ms/tick (20% improvement)
- **778 bots**: ~45-60ms/tick (25% improvement, histogram restored)
- **5000 bots**: ~240-320ms/tick (20% improvement)

### To Achieve 5000 Bot Target (<50ms/tick)
**Additional Work Required**: Distribute priority filtering to ThreadPool workers
**Effort**: 6-8 hours
**Expected Result**: ~45-60ms/tick for 5000 bots

---

## TESTING CHECKLIST

### After Each Optimization
- [ ] Compile without warnings (MSVC /W4 /WX)
- [ ] Run unit tests (if available)
- [ ] Start worldserver with 100 bots
- [ ] Monitor log for errors/warnings
- [ ] Profile with Visual Studio (CPU + Memory)
- [ ] Verify expected improvement
- [ ] Document actual improvement in this file

### Final Validation (All Optimizations Applied)
- [ ] 100 bots: Verify <10ms/tick
- [ ] 500 bots: Verify <50ms/tick
- [ ] 778 bots: Verify <70ms/tick with histogram enabled
- [ ] Check memory usage stable (no leaks)
- [ ] Check CPU usage <80% average
- [ ] Run for 1 hour without crashes
- [ ] Create git commit with before/after metrics

---

**Last Updated**: 2025-10-16
**Status**: READY FOR IMPLEMENTATION
**Total Estimated Effort**: 14 hours (excluding #7 conditional optimization)
**Expected Overall Improvement**: 20-30% performance gain

