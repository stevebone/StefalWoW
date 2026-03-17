# Thread-Safe World Population System
## Lock-Free & High-Performance Design

**Status**: Design Phase
**Integration**: Uses existing Playerbot::Performance::ThreadPool
**Concurrency Model**: Lock-free readers, atomic counters, immutable caches

---

## Concurrency Strategy Overview

### Design Principles

1. **Immutable Caches**: Build once at startup, never modify (lock-free reads)
2. **Atomic Counters**: Use `std::atomic<uint32>` for distribution tracking
3. **Two-Phase Creation**: Prepare data in worker threads, apply on main thread
4. **No Shared Mutable State**: Each bot creation is independent
5. **Work-Stealing**: Leverage existing ThreadPool work-stealing queues

### Thread Safety Guarantees

✅ **Lock-Free Reads**: All caches (gear, talents, zones) are read-only
✅ **Atomic Writes**: Distribution counters use atomic operations
✅ **Thread-Local Storage**: Per-thread random number generators
✅ **Main Thread Serialization**: Player object manipulation on main thread only
✅ **Connection Pool**: Database queries use TrinityCore's thread-safe connection pool

---

## Shared State Analysis

### 1. Distribution Counters (ATOMIC)

```cpp
namespace Playerbot {

struct LevelBracket {
    uint32 minLevel;
    uint32 maxLevel;
    float targetPercentage;

    // ⚠️ SHARED STATE - Use atomic
    std::atomic<uint32> currentCount{0};

    TeamId faction;

    // Thread-safe increment
    void IncrementCount() {
        currentCount.fetch_add(1, std::memory_order_relaxed);
    }

    // Thread-safe decrement
    void DecrementCount() {
        currentCount.fetch_sub(1, std::memory_order_relaxed);
    }

    // Thread-safe read
    uint32 GetCount() const {
        return currentCount.load(std::memory_order_relaxed);
    }
};

}
```

**Justification**: Memory order `relaxed` is sufficient because:
- We don't need synchronization between different counters
- Slight delays in visibility are acceptable (±15% tolerance)
- No dependent operations that require ordering

---

### 2. Immutable Caches (LOCK-FREE READS)

```cpp
namespace Playerbot {

// Built once at initialization, never modified
class ImmutableGearCache {
public:
    // Initialize cache (called ONCE at server startup, single-threaded)
    void BuildCache();

    // Thread-safe read access (no locks needed)
    std::vector<ItemScore> const* GetItemsForSlot(
        uint32 level, EquipmentSlots slot, Classes cls, uint32 spec) const
    {
        auto key = MakeCacheKey(level, slot, cls, spec);
        auto it = _cache.find(key);
        return it != _cache.end() ? &it->second : nullptr;
    }

private:
    // ✅ IMMUTABLE after BuildCache() completes
    // No mutex needed - readers never block each other
    std::unordered_map<uint64, std::vector<ItemScore>> _cache;

    // Cache is marked const after initialization
    std::atomic<bool> _initialized{false};
};

// Global read-only cache
inline ImmutableGearCache const& GetGearCache() {
    static ImmutableGearCache cache;
    return cache;
}

}
```

**Performance**: Zero contention, zero cache-line bouncing, perfect scalability

---

### 3. Thread-Local Storage (ZERO CONTENTION)

```cpp
namespace Playerbot {

// Per-thread random number generator (no locking)
class ThreadLocalRandom {
public:
    static uint32 Rand(uint32 min, uint32 max) {
        thread_local std::mt19937 generator(std::random_device{}());
        std::uniform_int_distribution<uint32> dist(min, max);
        return dist(generator);
    }
};

// Per-thread temporary storage (no allocation overhead)
class ThreadLocalStorage {
public:
    static GearSet& GetTempGearSet() {
        thread_local GearSet tempGear;
        tempGear.items.clear();  // Reuse allocation
        return tempGear;
    }
};

}
```

---

## Two-Phase Bot Creation (Thread-Safe)

### Phase 1: Worker Thread (Parallel, Lock-Free)

Compute-intensive operations that don't touch Player object:

```cpp
struct BotCreationPlan {
    // Input
    uint32 accountId;
    std::string name;
    Races race;
    Classes cls;
    Gender gender;
    uint32 targetLevel;
    uint32 spec1;
    uint32 spec2;

    // Output (computed in worker thread)
    GearSet spec1Gear;
    GearSet spec2Gear;
    std::vector<uint32> talents1;
    std::vector<uint32> talents2;
    std::vector<uint32> bags;
    WorldLocation spawnLocation;

    // Status
    bool preparationComplete{false};
    std::string errorMessage;
};

// ✅ THREAD-SAFE: No shared mutable state
BotCreationPlan PrepareBotData(uint32 accountId, uint32 targetLevel) {
    BotCreationPlan plan;

    // 1. Select spec (uses immutable cache)
    plan.spec1 = SelectSpecialization(cls, 1);
    plan.spec2 = targetLevel >= 10 ? SelectSpecialization(cls, 2) : 0;

    // 2. Query gear (database connection pool is thread-safe)
    plan.spec1Gear = BuildGearSet(cls, plan.spec1, targetLevel);
    if (plan.spec2 > 0)
        plan.spec2Gear = BuildGearSet(cls, plan.spec2, targetLevel);

    // 3. Select talents (uses immutable cache)
    plan.talents1 = GetTalentLoadout(cls, plan.spec1, targetLevel);
    if (plan.spec2 > 0)
        plan.talents2 = GetTalentLoadout(cls, plan.spec2, targetLevel);

    // 4. Select spawn location (uses immutable cache)
    plan.spawnLocation = SelectSpawnLocation(targetLevel, faction);

    // 5. Select bags
    plan.bags = SelectBags(targetLevel);

    plan.preparationComplete = true;
    return plan;
}
```

### Phase 2: Main Thread (Serialized)

Player object manipulation (MUST be main thread):

```cpp
// ⚠️ MAIN THREAD ONLY
void ApplyBotPlan(BotCreationPlan const& plan) {
    // 1. Create character (TrinityCore API - main thread only)
    ObjectGuid botGuid = CreateCharacter(plan.accountId, plan.name, ...);
    Player* bot = ObjectAccessor::FindPlayer(botGuid);

    if (!bot) {
        LOG_ERROR("Failed to create bot");
        return;
    }

    // 2. Apply level (TrinityCore API - main thread only)
    if (plan.targetLevel >= 5) {
        bot->GiveLevel(plan.targetLevel);
        bot->InitTalentForLevel();
        bot->SetXP(0);
    }

    // 3. Apply spec & talents (TrinityCore API - main thread only)
    ApplySpecialization(bot, plan.spec1);
    ApplyTalents(bot, plan.talents1);

    if (plan.spec2 > 0) {
        ActivateSpecialization(bot, plan.spec2);
        ApplyTalents(bot, plan.talents2);
        ActivateSpecialization(bot, plan.spec1);  // Switch back
    }

    // 4. Equip gear (TrinityCore API - main thread only)
    EquipGearSet(bot, plan.spec1Gear);
    if (plan.spec2 > 0)
        AddGearToBags(bot, plan.spec2Gear);

    // 5. Add bags & consumables
    AddBags(bot, plan.bags);

    // 6. Teleport
    bot->TeleportTo(plan.spawnLocation);

    // 7. Save
    bot->SaveToDB(false, false);

    // 8. ✅ Update distribution counter (atomic)
    GetBracketForLevel(plan.targetLevel, faction)->IncrementCount();

    LOG_INFO("Bot '{}' created successfully", plan.name);
}
```

---

## ThreadPool Integration

### Batch Bot Creation (Parallel)

```cpp
namespace Playerbot {

class BotLevelManager {
public:
    // Submit batch of bots for creation
    void CreateBotsAsync(uint32 count) {
        auto& threadPool = GetThreadPool(ThreadPoolType::GENERAL);

        for (uint32 i = 0; i < count; ++i) {
            // Phase 1: Submit to worker thread
            threadPool.Submit(
                []() {
                    // Worker thread: prepare bot data
                    BotCreationPlan plan = PrepareBotData(accountId, targetLevel);

                    // Phase 2: Schedule main-thread application
                    ScheduleMainThreadTask([plan]() {
                        ApplyBotPlan(plan);
                    });
                },
                TaskPriority::NORMAL
            );
        }
    }

private:
    // Queue for main-thread tasks
    std::queue<std::function<void()>> _mainThreadTasks;
    std::mutex _mainThreadMutex;
};

}
```

### Main Thread Update Loop

```cpp
void BotLevelManager::Update(uint32 diff) {
    // Process pending bot applications (main thread)
    std::unique_lock<std::mutex> lock(_mainThreadMutex);

    // Process up to 10 bots per update to avoid lag
    constexpr uint32 MAX_PER_UPDATE = 10;
    uint32 processed = 0;

    while (!_mainThreadTasks.empty() && processed < MAX_PER_UPDATE) {
        auto task = std::move(_mainThreadTasks.front());
        _mainThreadTasks.pop();

        lock.unlock();  // Release lock while executing
        task();         // Apply bot plan (main thread)
        lock.lock();    // Re-acquire for next iteration

        ++processed;
    }
}
```

---

## Lock-Free Distribution Queries

```cpp
class BotLevelDistribution {
public:
    // ✅ THREAD-SAFE: Reads atomic counters
    std::vector<uint32> GetOverpopulatedBrackets(TeamId faction) const {
        std::vector<uint32> result;

        uint32 totalBots = GetTotalBotCount(faction);

        for (size_t i = 0; i < _brackets[faction].size(); ++i) {
            auto const& bracket = _brackets[faction][i];

            float currentPct = bracket.GetCount() / (float)totalBots;
            float targetPct = bracket.targetPercentage;

            // ±15% tolerance
            if (currentPct > targetPct * 1.15f) {
                result.push_back(i);
            }
        }

        return result;
    }

    // ✅ THREAD-SAFE: Atomic read
    uint32 GetTotalBotCount(TeamId faction) const {
        uint32 total = 0;
        for (auto const& bracket : _brackets[faction]) {
            total += bracket.GetCount();  // Atomic read
        }
        return total;
    }

private:
    // Brackets stored in vector (stable addresses, no reallocation)
    std::array<std::vector<LevelBracket>, 2> _brackets;  // [ALLIANCE][HORDE]
};
```

---

## Immutable Cache Building

### Startup Initialization (Single-Threaded)

```cpp
void BotGearFactory::Initialize() {
    LOG_INFO("playerbot", "Building immutable gear cache...");

    auto startTime = std::chrono::steady_clock::now();

    // ✅ SINGLE-THREADED BUILD
    for (uint32 level = 1; level <= 80; ++level) {
        for (uint32 cls = CLASS_WARRIOR; cls < MAX_CLASSES; ++cls) {
            for (uint32 spec = 0; spec < 4; ++spec) {  // Max 4 specs per class
                for (uint32 slot = 0; slot < EQUIPMENT_SLOT_END; ++slot) {

                    // Query database (expensive)
                    auto items = QueryItemsForSlot(level, slot, cls, spec);

                    // Store in cache
                    uint64 key = MakeCacheKey(level, slot, cls, spec);
                    _cache[key] = std::move(items);
                }
            }
        }
    }

    _initialized.store(true, std::memory_order_release);

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime
    );

    LOG_INFO("playerbot", "Gear cache built in {}ms - {} entries",
             elapsed.count(), _cache.size());
}

// Access (lock-free read)
std::vector<ItemScore> const* BotGearFactory::GetCachedItems(
    uint32 level, EquipmentSlots slot, Classes cls, uint32 spec) const
{
    // Wait for initialization (happens once at startup)
    while (!_initialized.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    uint64 key = MakeCacheKey(level, slot, cls, spec);
    auto it = _cache.find(key);
    return it != _cache.end() ? &it->second : nullptr;
}
```

---

## Performance Characteristics

### Scalability Analysis

**Worker Thread Load (Parallel)**:
```
Per bot preparation:
- Gear selection: ~50 database queries (200ms)
- Talent selection: ~5ms (cache lookup)
- Zone selection: ~1ms (cache lookup)
Total: ~205ms per bot

With 8 worker threads:
- 8 bots prepared simultaneously
- Throughput: ~39 bots/second preparation
```

**Main Thread Load (Serialized)**:
```
Per bot application:
- Character creation: ~10ms
- Level-up: ~5ms (TrinityCore native)
- Gear equipping: ~20ms (15 items)
- Teleport: ~5ms
Total: ~40ms per bot

Main thread capacity:
- 25 bots/second application rate
- Bottleneck: main thread (not worker threads)
```

**Optimization**: Main thread is the bottleneck, not worker threads. This is acceptable because:
1. Creating 25 bots/second = 1500 bots/minute is MORE than sufficient
2. Main thread operations are inherently serial (TrinityCore API requirement)
3. We can throttle bot creation to avoid lag spikes

---

## Memory Safety

### Cache-Line Bouncing Prevention

```cpp
// ✅ Aligned to cache line boundary (64 bytes)
struct alignas(64) LevelBracket {
    uint32 minLevel;
    uint32 maxLevel;
    float targetPercentage;
    TeamId faction;

    // Atomic on separate cache line to avoid false sharing
    alignas(64) std::atomic<uint32> currentCount{0};
};
```

**Why**: Prevents false sharing when multiple threads read different brackets simultaneously.

---

## Database Connection Safety

### TrinityCore Connection Pool (Already Thread-Safe)

```cpp
// ✅ THREAD-SAFE: TrinityCore manages connection pool
QueryResult result = WorldDatabase.Query(
    "SELECT entry FROM item_template WHERE ..."
);

// Each worker thread gets its own connection from pool
// No manual synchronization needed
```

---

## Error Handling (Thread-Safe)

```cpp
struct BotCreationPlan {
    std::atomic<bool> hasError{false};
    std::string errorMessage;  // Written once, read once (safe)

    void SetError(std::string const& msg) {
        errorMessage = msg;
        hasError.store(true, std::memory_order_release);
    }

    bool HasError() const {
        return hasError.load(std::memory_order_acquire);
    }
};

// In worker thread
void PrepareBotData(BotCreationPlan& plan) {
    try {
        // ... preparation ...
    }
    catch (std::exception const& e) {
        plan.SetError(e.what());
        return;
    }
}

// In main thread
void ApplyBotPlan(BotCreationPlan const& plan) {
    if (plan.HasError()) {
        LOG_ERROR("Bot creation failed: {}", plan.errorMessage);
        return;
    }

    // ... apply ...
}
```

---

## Configuration

```conf
###############################################################################
# THREAD POOL CONFIGURATION
###############################################################################

# Number of worker threads for bot creation
# 0 = auto-detect (num CPUs - 1)
# Recommended: 4-8 threads
Playerbot.Population.WorkerThreads = 0

# Max bots to create per batch
# Higher = faster initial population, but may cause lag spikes
Playerbot.Population.BatchSize = 50

# Max bots to apply per main-thread update
# Higher = faster, but may cause frame drops
# Recommended: 5-10
Playerbot.Population.MaxApplyPerUpdate = 10

# Task priority for bot creation
# Options: CRITICAL, HIGH, NORMAL, LOW, IDLE
# Recommended: NORMAL (don't block critical game systems)
Playerbot.Population.TaskPriority = NORMAL
```

---

## Testing & Validation

### Thread Safety Tests

```cpp
// Unit test: Concurrent distribution queries
TEST(BotLevelDistribution, ConcurrentReads) {
    BotLevelDistribution dist;
    dist.Initialize();

    // Spawn 100 threads reading concurrently
    std::vector<std::thread> threads;
    for (int i = 0; i < 100; ++i) {
        threads.emplace_back([&dist]() {
            for (int j = 0; j < 1000; ++j) {
                auto overpopulated = dist.GetOverpopulatedBrackets(TEAM_ALLIANCE);
                auto count = dist.GetTotalBotCount(TEAM_ALLIANCE);
            }
        });
    }

    for (auto& t : threads)
        t.join();

    // Should complete without crashes or data races
}

// Stress test: Create 1000 bots in parallel
TEST(BotLevelManager, ParallelCreation) {
    BotLevelManager mgr;
    mgr.CreateBotsAsync(1000);

    // Wait for completion
    while (mgr.GetPendingCreations() > 0) {
        mgr.Update(100);  // Main thread update
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    EXPECT_EQ(mgr.GetCreatedBots(), 1000);
}
```

---

## Performance Monitoring

```cpp
struct PopulationMetrics {
    std::atomic<uint64> totalBotsCreated{0};
    std::atomic<uint64> totalPreparationTime{0};  // microseconds
    std::atomic<uint64> totalApplicationTime{0};  // microseconds
    std::atomic<uint32> peakPendingCreations{0};

    double GetAveragePreparationTime() const {
        return totalPreparationTime.load() / (double)totalBotsCreated.load();
    }

    double GetAverageApplicationTime() const {
        return totalApplicationTime.load() / (double)totalBotsCreated.load();
    }
};

// Log every 100 bots
void LogMetrics() {
    LOG_INFO("playerbot",
             "Population Metrics:\n"
             "  Total Bots: {}\n"
             "  Avg Prep Time: {:.2f}ms\n"
             "  Avg Apply Time: {:.2f}ms\n"
             "  Peak Pending: {}",
             _metrics.totalBotsCreated.load(),
             _metrics.GetAveragePreparationTime() / 1000.0,
             _metrics.GetAverageApplicationTime() / 1000.0,
             _metrics.peakPendingCreations.load());
}
```

---

## Summary: Thread Safety Guarantees

| Component | Strategy | Contention | Performance |
|-----------|----------|------------|-------------|
| **Distribution Counters** | `std::atomic<uint32>` | Zero | Perfect |
| **Gear Cache** | Immutable (read-only) | Zero | Perfect |
| **Talent Cache** | Immutable (read-only) | Zero | Perfect |
| **Zone Cache** | Immutable (read-only) | Zero | Perfect |
| **Bot Preparation** | Thread-local, no sharing | Zero | Perfect |
| **Bot Application** | Main thread only | N/A | Sufficient |
| **Database Queries** | Connection pool | Low | Good |
| **Main Thread Queue** | Mutex (short critical section) | Low | Good |

**Overall**: Lock-free for 95% of operations, minimal locking only where required.

---

## Implementation Checklist

- [ ] Replace bracket `currentCount` with `std::atomic<uint32>`
- [ ] Build immutable caches at startup (single-threaded)
- [ ] Implement `PrepareBotData()` function (worker thread)
- [ ] Implement `ApplyBotPlan()` function (main thread)
- [ ] Add main-thread task queue with mutex
- [ ] Integrate with existing `ThreadPool` via `Submit()`
- [ ] Add metrics tracking (atomic counters)
- [ ] Add thread safety tests
- [ ] Add stress tests (1000+ bots)
- [ ] Add performance monitoring
- [ ] Document thread safety in code comments

---

**Author**: Claude Code
**Date**: 2025-10-18
**Version**: 1.0
**Status**: Ready for Implementation
