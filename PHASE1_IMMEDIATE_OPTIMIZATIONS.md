# Phase 1: Immediate Optimizations Implementation

## Overview
These optimizations can be implemented immediately with minimal risk to provide 60-70% performance improvement while more complex architectural changes are developed.

## 1. Manager Update Throttling System

### File: `src/modules/Playerbot/Core/Managers/UpdateThrottler.h`
```cpp
#pragma once

#include "Define.h"
#include <atomic>
#include <chrono>
#include <array>

namespace Playerbot
{

/**
 * @brief Intelligent update throttling based on manager priority and system load
 *
 * This system reduces update frequency for non-critical managers under load
 * while maintaining responsiveness for critical operations.
 */
class UpdateThrottler
{
public:
    enum class Priority : uint8
    {
        CRITICAL    = 0,  // Combat, movement - every frame
        HIGH        = 1,  // Quest, trade - 10 Hz (100ms)
        MEDIUM      = 2,  // Gathering, group - 2 Hz (500ms)
        LOW         = 3,  // Auction, crafting - 1 Hz (1000ms)
        BACKGROUND  = 4   // Statistics, cleanup - 0.1 Hz (10000ms)
    };

    struct ThrottleConfig
    {
        uint32 baseInterval;      // Base update interval in ms
        uint32 loadMultiplier;    // Multiplier under high load
        uint32 maxInterval;       // Maximum interval cap
        bool skipUnderLoad;       // Can skip entirely under extreme load
    };

private:
    // Configuration per priority level
    static constexpr std::array<ThrottleConfig, 5> s_configs = {{
        {0,     1, 0,     false},  // CRITICAL - never throttle
        {100,   2, 500,   false},  // HIGH - throttle to 200ms under load
        {500,   3, 2000,  false},  // MEDIUM - throttle to 1500ms under load
        {1000,  5, 10000, true},   // LOW - throttle heavily or skip
        {10000, 10, 60000, true}   // BACKGROUND - aggressive throttling
    }};

    // Per-manager state
    struct ManagerState
    {
        Priority priority;
        uint32 lastUpdate{0};
        uint32 nextUpdate{0};
        std::atomic<uint32> skipCount{0};
        std::atomic<uint64> totalUpdates{0};
    };

    std::unordered_map<std::string, ManagerState> _managerStates;
    std::atomic<float> _systemLoad{0.0f};  // 0.0 = idle, 1.0 = full load
    std::atomic<uint32> _activeBots{0};

public:
    static UpdateThrottler& Instance()
    {
        static UpdateThrottler instance;
        return instance;
    }

    /**
     * Register a manager with its priority
     */
    void RegisterManager(std::string const& managerId, Priority priority)
    {
        _managerStates[managerId] = {priority, 0, 0, {0}, {0}};
    }

    /**
     * Check if a manager should update this frame
     */
    bool ShouldUpdate(std::string const& managerId, uint32 currentTime)
    {
        auto it = _managerStates.find(managerId);
        if (it == _managerStates.end())
            return true; // Unknown managers always update (safety)

        auto& state = it->second;

        // Critical always updates
        if (state.priority == Priority::CRITICAL)
            return true;

        // Check if enough time has passed
        if (currentTime < state.nextUpdate)
        {
            state.skipCount.fetch_add(1, std::memory_order_relaxed);
            return false;
        }

        // Calculate next update time based on load
        auto& config = s_configs[static_cast<size_t>(state.priority)];
        float load = _systemLoad.load(std::memory_order_relaxed);

        // Skip entirely under extreme load if configured
        if (config.skipUnderLoad && load > 0.9f)
        {
            state.skipCount.fetch_add(1, std::memory_order_relaxed);
            return false;
        }

        // Calculate throttled interval
        uint32 interval = config.baseInterval;
        if (load > 0.5f)
        {
            interval = std::min(
                static_cast<uint32>(interval * (1.0f + load * config.loadMultiplier)),
                config.maxInterval
            );
        }

        // Schedule next update
        state.lastUpdate = currentTime;
        state.nextUpdate = currentTime + interval;
        state.totalUpdates.fetch_add(1, std::memory_order_relaxed);

        return true;
    }

    /**
     * Update system load metric (called by main update loop)
     */
    void UpdateSystemLoad(uint32 frameTime, uint32 targetFrameTime = 50)
    {
        // Simple load calculation: frameTime / targetTime
        float load = static_cast<float>(frameTime) / targetFrameTime;
        _systemLoad.store(std::clamp(load, 0.0f, 1.0f), std::memory_order_relaxed);
    }

    /**
     * Update active bot count for load calculations
     */
    void SetActiveBots(uint32 count)
    {
        _activeBots.store(count, std::memory_order_relaxed);

        // Adjust load based on bot count (>100 bots increases load factor)
        if (count > 100)
        {
            float botLoad = static_cast<float>(count - 100) / 1000.0f; // 0.1 per 100 bots
            float currentLoad = _systemLoad.load(std::memory_order_relaxed);
            _systemLoad.store(std::min(currentLoad + botLoad, 1.0f), std::memory_order_relaxed);
        }
    }

    /**
     * Get statistics for monitoring
     */
    struct Stats
    {
        uint32 activeBots;
        float systemLoad;
        std::unordered_map<std::string, uint64> managerUpdates;
        std::unordered_map<std::string, uint32> managerSkips;
    };

    Stats GetStats() const
    {
        Stats stats;
        stats.activeBots = _activeBots.load(std::memory_order_relaxed);
        stats.systemLoad = _systemLoad.load(std::memory_order_relaxed);

        for (auto const& [id, state] : _managerStates)
        {
            stats.managerUpdates[id] = state.totalUpdates.load(std::memory_order_relaxed);
            stats.managerSkips[id] = state.skipCount.load(std::memory_order_relaxed);
        }

        return stats;
    }
};

} // namespace Playerbot
```

## 2. Lock-Free Read Optimizations

### File: `src/modules/Playerbot/Economy/AuctionManagerOptimized.cpp`
```cpp
// Modifications to AuctionManager to reduce lock contention

// BEFORE (Current implementation):
ItemPriceData AuctionManager::GetItemPriceData(uint32 itemId) const
{
    std::lock_guard<std::recursive_mutex> lock(_mutex); // BLOCKS ALL READS!
    auto it = _priceCache.find(itemId);
    if (it != _priceCache.end())
        return it->second;
    return ItemPriceData();
}

// AFTER (Optimized with RCU pattern):
class AuctionManager : public BehaviorManager
{
private:
    // Read-Copy-Update pattern for lock-free reads
    struct PriceCacheRCU
    {
        std::unordered_map<uint32, ItemPriceData> data;
        std::atomic<uint64> version{0};
    };

    std::atomic<PriceCacheRCU*> _priceCache{nullptr};
    PriceCacheRCU _cacheBuffers[2];  // Double buffering
    std::atomic<int> _activeBuffer{0};
    std::mutex _writeMutex;  // Only for writes (rare)

public:
    // Lock-free read (99% of operations)
    ItemPriceData GetItemPriceData(uint32 itemId) const
    {
        // No lock needed! Atomic pointer read
        auto* cache = _priceCache.load(std::memory_order_acquire);
        if (!cache)
            return ItemPriceData();

        auto it = cache->data.find(itemId);
        if (it != cache->data.end())
            return it->second;  // Copy is cheap (POD structure)

        return ItemPriceData();
    }

    // Write operations (rare - only during market scan)
    void UpdatePriceData(uint32 itemId, ItemPriceData const& data)
    {
        std::lock_guard<std::mutex> lock(_writeMutex);  // Only writers lock

        // Get inactive buffer
        int inactive = (_activeBuffer.load() + 1) % 2;
        auto& newCache = _cacheBuffers[inactive];

        // Copy current data to inactive buffer
        auto* current = _priceCache.load(std::memory_order_acquire);
        if (current)
            newCache.data = current->data;

        // Update the data
        newCache.data[itemId] = data;
        newCache.version.fetch_add(1, std::memory_order_relaxed);

        // Atomic swap to new cache
        _priceCache.store(&newCache, std::memory_order_release);
        _activeBuffer.store(inactive, std::memory_order_release);
    }
};
```

## 3. Batched Manager Updates

### File: `src/modules/Playerbot/Core/Managers/BatchedUpdateSystem.h`
```cpp
#pragma once

#include "Define.h"
#include <vector>
#include <span>
#include <execution>

namespace Playerbot
{

/**
 * @brief Batches manager updates across multiple bots for cache efficiency
 */
class BatchedUpdateSystem
{
public:
    struct UpdateBatch
    {
        std::vector<BotAI*> bots;
        uint32 diff;
        uint32 updateCount{0};
    };

    /**
     * Process bot updates in batches for better cache locality
     */
    static void ProcessBotBatch(std::span<BotAI*> bots, uint32 diff)
    {
        constexpr size_t BATCH_SIZE = 16;  // Optimal for cache line

        // Group bots by update needs
        std::vector<BotAI*> questUpdates;
        std::vector<BotAI*> auctionUpdates;
        std::vector<BotAI*> gatheringUpdates;

        auto& throttler = UpdateThrottler::Instance();
        uint32 currentTime = getMSTime();

        // First pass: Determine which managers need updates
        for (auto* bot : bots)
        {
            if (!bot)
                continue;

            if (throttler.ShouldUpdate("QuestManager", currentTime))
                questUpdates.push_back(bot);

            if (throttler.ShouldUpdate("AuctionManager", currentTime))
                auctionUpdates.push_back(bot);

            if (throttler.ShouldUpdate("GatheringManager", currentTime))
                gatheringUpdates.push_back(bot);
        }

        // Batch process each manager type
        BatchProcessQuests(questUpdates, diff);
        BatchProcessAuctions(auctionUpdates, diff);
        BatchProcessGathering(gatheringUpdates, diff);
    }

private:
    /**
     * Batch process quest updates - single lock for all bots
     */
    static void BatchProcessQuests(std::vector<BotAI*> const& bots, uint32 diff)
    {
        if (bots.empty())
            return;

        // Prepare batch data
        struct QuestUpdate
        {
            Player* bot;
            std::vector<uint32> questIds;
            std::vector<QuestObjective> objectives;
        };

        std::vector<QuestUpdate> updates;
        updates.reserve(bots.size());

        // Collect all quest data first (no locks)
        for (auto* botAI : bots)
        {
            auto* bot = botAI->GetBot();
            if (!bot)
                continue;

            QuestUpdate update;
            update.bot = bot;

            // Collect active quests
            for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
            {
                uint32 questId = bot->GetQuestSlotQuestId(slot);
                if (questId != 0)
                    update.questIds.push_back(questId);
            }

            updates.push_back(std::move(update));
        }

        // Process all quest updates in single operation
        // This is where we'd have ONE lock instead of N locks
        QuestManager::BatchUpdateQuests(updates, diff);
    }

    /**
     * Batch process auction house operations
     */
    static void BatchProcessAuctions(std::vector<BotAI*> const& bots, uint32 diff)
    {
        if (bots.empty())
            return;

        // Group by auction house (faction-specific)
        std::unordered_map<uint32, std::vector<Player*>> ahGroups;

        for (auto* botAI : bots)
        {
            auto* bot = botAI->GetBot();
            if (!bot)
                continue;

            uint32 ahId = GetAuctionHouseIdForBot(bot);
            ahGroups[ahId].push_back(bot);
        }

        // Process each auction house group together
        for (auto const& [ahId, botGroup] : ahGroups)
        {
            // Single market scan for all bots in this AH
            AuctionManager::BatchScanMarket(ahId, botGroup);
        }
    }

    /**
     * Batch process gathering node detection
     */
    static void BatchProcessGathering(std::vector<BotAI*> const& bots, uint32 diff)
    {
        if (bots.empty())
            return;

        // Group by map/zone for spatial efficiency
        std::unordered_map<uint32, std::vector<Player*>> zoneGroups;

        for (auto* botAI : bots)
        {
            auto* bot = botAI->GetBot();
            if (!bot)
                continue;

            uint32 zoneId = bot->GetZoneId();
            zoneGroups[zoneId].push_back(bot);
        }

        // Process each zone group with shared node detection
        for (auto const& [zoneId, botGroup] : zoneGroups)
        {
            // One spatial query for all bots in zone
            GatheringManager::BatchDetectNodes(zoneId, botGroup);
        }
    }
};

} // namespace Playerbot
```

## 4. Integration with BotAI

### Modifications to `src/modules/Playerbot/AI/BotAI.cpp`

```cpp
// REPLACE lines 1714-1798 with optimized version:

void BotAI::UpdateManagers(uint32 diff)
{
    // OPTIMIZATION 1: Early exit if throttled
    static uint32 s_lastBatchUpdate = 0;
    uint32 currentTime = getMSTime();

    // Batch updates every 50ms for non-critical managers
    bool doBatchUpdate = (currentTime - s_lastBatchUpdate) >= 50;

    // OPTIMIZATION 2: Use throttler to skip updates
    auto& throttler = UpdateThrottler::Instance();

    // Update throttler with current frame performance
    static uint32 s_lastFrameTime = currentTime;
    uint32 frameTime = currentTime - s_lastFrameTime;
    s_lastFrameTime = currentTime;
    throttler.UpdateSystemLoad(frameTime);

    // OPTIMIZATION 3: Priority-based updates
    // Critical managers (combat, movement) - always update via registry
    if (_managerRegistry)
    {
        // This only updates managers marked as CRITICAL priority
        uint32 managersUpdated = _managerRegistry->UpdatePriority(
            diff,
            UpdateThrottler::Priority::CRITICAL
        );

        if (managersUpdated > 0)
        {
            TC_LOG_TRACE("module.playerbot.managers",
                "Bot {} updated {} critical managers",
                _bot->GetName(), managersUpdated);
        }
    }

    // OPTIMIZATION 4: Throttled updates for non-critical managers
    if (doBatchUpdate)
    {
        s_lastBatchUpdate = currentTime;

        // Collect bots for batch processing
        static thread_local std::vector<BotAI*> s_batchBuffer;
        s_batchBuffer.clear();
        s_batchBuffer.push_back(this);

        // Try to batch with nearby bots (same zone/group)
        // This would be implemented via a bot registry
        // For now, just process this bot

        BatchedUpdateSystem::ProcessBotBatch(s_batchBuffer, diff);
    }

    // OPTIMIZATION 5: Background tasks at low frequency
    if (throttler.ShouldUpdate("EquipmentManager", currentTime))
    {
        // Only check equipment every 10-30 seconds based on load
        EquipmentManager::instance()->AutoEquipBestGear(_bot);
    }

    if (throttler.ShouldUpdate("ProfessionManager", currentTime))
    {
        // Only update professions every 15-60 seconds based on load
        ProfessionManager::instance()->Update(_bot, diff);
    }
}
```

## 5. Performance Monitoring

### File: `src/modules/Playerbot/Performance/UpdateProfiler.h`
```cpp
#pragma once

#include "Define.h"
#include <atomic>
#include <chrono>
#include <array>

namespace Playerbot
{

/**
 * @brief Lightweight profiler for bot update performance
 */
class UpdateProfiler
{
public:
    struct ManagerStats
    {
        std::atomic<uint64> totalCalls{0};
        std::atomic<uint64> totalTimeUs{0};
        std::atomic<uint64> maxTimeUs{0};
        std::atomic<uint64> skipCount{0};

        float GetAvgTimeMs() const
        {
            uint64 calls = totalCalls.load(std::memory_order_relaxed);
            if (calls == 0)
                return 0.0f;
            return static_cast<float>(totalTimeUs.load(std::memory_order_relaxed))
                   / calls / 1000.0f;
        }
    };

private:
    std::unordered_map<std::string, ManagerStats> _stats;
    std::atomic<uint32> _activeBots{0};
    std::atomic<uint64> _frameCount{0};
    std::chrono::steady_clock::time_point _startTime;

public:
    class ScopedTimer
    {
        UpdateProfiler& _profiler;
        std::string _name;
        std::chrono::steady_clock::time_point _start;

    public:
        ScopedTimer(UpdateProfiler& profiler, std::string const& name)
            : _profiler(profiler), _name(name)
            , _start(std::chrono::steady_clock::now())
        { }

        ~ScopedTimer()
        {
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - _start);
            _profiler.RecordTime(_name, duration.count());
        }
    };

    static UpdateProfiler& Instance()
    {
        static UpdateProfiler instance;
        return instance;
    }

    void RecordTime(std::string const& manager, uint64 microseconds)
    {
        auto& stats = _stats[manager];
        stats.totalCalls.fetch_add(1, std::memory_order_relaxed);
        stats.totalTimeUs.fetch_add(microseconds, std::memory_order_relaxed);

        // Update max time
        uint64 currentMax = stats.maxTimeUs.load(std::memory_order_relaxed);
        while (microseconds > currentMax &&
               !stats.maxTimeUs.compare_exchange_weak(currentMax, microseconds))
        {
            // Retry
        }
    }

    void RecordSkip(std::string const& manager)
    {
        _stats[manager].skipCount.fetch_add(1, std::memory_order_relaxed);
    }

    void PrintReport() const
    {
        TC_LOG_INFO("module.playerbot.perf", "=== Performance Report ===");
        TC_LOG_INFO("module.playerbot.perf", "Active Bots: {}",
                    _activeBots.load(std::memory_order_relaxed));

        for (auto const& [name, stats] : _stats)
        {
            TC_LOG_INFO("module.playerbot.perf",
                        "{}: Avg={:.2f}ms Max={:.2f}ms Calls={} Skips={}",
                        name,
                        stats.GetAvgTimeMs(),
                        stats.maxTimeUs.load() / 1000.0f,
                        stats.totalCalls.load(),
                        stats.skipCount.load());
        }
    }
};

// Macro for easy profiling
#define PROFILE_SCOPE(name) \
    UpdateProfiler::ScopedTimer _timer##__LINE__(UpdateProfiler::Instance(), name)

} // namespace Playerbot
```

## Implementation Steps

1. **Add UpdateThrottler** (30 minutes)
   - Create new header/source files
   - Register all managers with appropriate priorities
   - Integrate with ManagerRegistry

2. **Implement RCU Pattern** (2 hours)
   - Modify AuctionManager::GetItemPriceData()
   - Modify GatheringManager node detection
   - Test lock-free reads

3. **Add Batched Updates** (2 hours)
   - Create BatchedUpdateSystem
   - Modify BotAI::UpdateManagers()
   - Test with multiple bots

4. **Integrate Profiler** (1 hour)
   - Add profiling points
   - Create performance dashboard
   - Set up alerts for slow operations

5. **Testing & Tuning** (2 hours)
   - Load test with 100, 500, 1000 bots
   - Adjust throttle thresholds
   - Verify no functionality regression

## Expected Results

### Before Optimization:
- 100 bots: 50-100ms update time
- 600+ mutex operations per frame
- Linear scaling with bot count

### After Phase 1:
- 100 bots: 15-25ms update time (70% improvement)
- <200 mutex operations per frame (66% reduction)
- Sub-linear scaling with intelligent throttling
- 500 bots: 75-125ms (viable for testing)

## Configuration

Add to `worldserver.conf`:
```ini
# Playerbot Performance Optimization
Playerbot.Performance.UpdateThrottling = 1
Playerbot.Performance.BatchSize = 16
Playerbot.Performance.AdaptiveLoad = 1
Playerbot.Performance.ProfileUpdates = 1

# Manager Priority Configuration
Playerbot.Manager.Quest.Priority = HIGH
Playerbot.Manager.Trade.Priority = HIGH
Playerbot.Manager.Gathering.Priority = MEDIUM
Playerbot.Manager.Auction.Priority = LOW
Playerbot.Manager.Group.Priority = MEDIUM
Playerbot.Manager.Equipment.Priority = BACKGROUND
Playerbot.Manager.Profession.Priority = BACKGROUND

# Throttle Thresholds
Playerbot.Performance.LoadThreshold = 0.5
Playerbot.Performance.CriticalLoadThreshold = 0.9
Playerbot.Performance.MaxSkipCount = 10
```

## Monitoring

Use these commands to monitor performance:
```
.playerbot perf show       - Show current performance stats
.playerbot perf reset      - Reset performance counters
.playerbot throttle show   - Show throttle status
.playerbot throttle set [manager] [priority] - Adjust manager priority
```

## Next Steps

After Phase 1 is stable and showing improvements, proceed to:
- **Phase 2**: Message-passing architecture
- **Phase 3**: Work-stealing task system
- **Phase 4**: Advanced SIMD optimizations

The immediate optimizations provide relief while we build the more complex lock-free architecture needed for 5000+ bots.