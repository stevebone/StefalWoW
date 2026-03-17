# Performance Tuning Guide - Phase 3
## Optimization Strategies for Bot AI Systems

## Overview

This document provides comprehensive performance tuning strategies for Phase 3 bot AI systems, including profiling techniques, optimization patterns, and performance monitoring frameworks.

## Performance Targets

### System-Wide Performance Goals

```cpp
// src/modules/Playerbot/Performance/PerformanceTargets.h

namespace Performance {
    // Per-bot resource limits
    constexpr uint32 MAX_AI_UPDATE_MICROSECONDS = 100;        // 0.1ms per update
    constexpr uint32 MAX_PATHFINDING_MILLISECONDS = 10;       // 10ms per path
    constexpr size_t MAX_MEMORY_PER_BOT_BYTES = 10485760;     // 10MB
    constexpr uint32 MAX_DATABASE_QUERY_MILLISECONDS = 10;    // 10ms per query
    
    // System-wide limits
    constexpr float MAX_CPU_USAGE_PERCENT = 80.0f;
    constexpr size_t MAX_TOTAL_MEMORY_BYTES = 4294967296;     // 4GB
    constexpr uint32 MAX_CONCURRENT_PATHFINDING = 10;
    constexpr uint32 MAX_CONCURRENT_DATABASE_QUERIES = 50;
    
    // Scalability targets
    struct ScalabilityTargets {
        uint32 botCount;
        float expectedCPU;
        size_t expectedMemory;
    };
    
    constexpr ScalabilityTargets SCALABILITY_TARGETS[] = {
        {100,  10.0f, 1073741824},   // 100 bots:  10% CPU, 1GB RAM
        {500,  40.0f, 2147483648},   // 500 bots:  40% CPU, 2GB RAM
        {1000, 70.0f, 3221225472},   // 1000 bots: 70% CPU, 3GB RAM
        {5000, 90.0f, 4294967296}    // 5000 bots: 90% CPU, 4GB RAM
    };
}
```

## CPU Optimization

### AI Update Optimization

```cpp
// src/modules/Playerbot/Performance/AIOptimizer.h

class AIOptimizer {
public:
    // Update frequency management
    struct UpdateFrequency {
        uint32 combatUpdateMs = 100;      // 10 updates/second in combat
        uint32 normalUpdateMs = 500;      // 2 updates/second normally
        uint32 idleUpdateMs = 2000;       // 0.5 updates/second when idle
        uint32 outOfRangeUpdateMs = 5000; // 0.2 updates/second when far
    };
    
    // Batch processing
    template<typename Func>
    static void BatchProcess(std::vector<Player*>& bots, Func func, size_t batchSize = 64) {
        tbb::parallel_for(
            tbb::blocked_range<size_t>(0, bots.size(), batchSize),
            [&](const tbb::blocked_range<size_t>& range) {
                for (size_t i = range.begin(); i != range.end(); ++i) {
                    func(bots[i]);
                }
            }
        );
    }
    
    // Priority-based updates
    class PriorityUpdateQueue {
    public:
        struct BotUpdate {
            Player* bot;
            uint32 priority;
            uint32 nextUpdateTime;
            
            bool operator<(const BotUpdate& other) const {
                return nextUpdateTime > other.nextUpdateTime;
            }
        };
        
        void Schedule(Player* bot) {
            BotUpdate update;
            update.bot = bot;
            update.priority = CalculatePriority(bot);
            update.nextUpdateTime = getMSTime() + GetUpdateInterval(update.priority);
            
            _queue.push(update);
        }
        
        std::vector<Player*> GetBotsToUpdate(uint32 maxCount) {
            std::vector<Player*> bots;
            uint32 now = getMSTime();
            
            while (!_queue.empty() && bots.size() < maxCount) {
                const BotUpdate& update = _queue.top();
                if (update.nextUpdateTime > now)
                    break;
                
                bots.push_back(update.bot);
                _queue.pop();
                
                // Reschedule
                Schedule(update.bot);
            }
            
            return bots;
        }
        
    private:
        std::priority_queue<BotUpdate> _queue;
        
        uint32 CalculatePriority(Player* bot) {
            uint32 priority = 100;
            
            // Higher priority for combat
            if (bot->IsInCombat())
                priority = 0;
            
            // Higher priority if near players
            if (HasNearbyPlayers(bot, 100.0f))
                priority = std::min(priority, 10u);
            
            // Lower priority if idle
            if (IsIdle(bot))
                priority = 200;
            
            return priority;
        }
        
        uint32 GetUpdateInterval(uint32 priority) {
            if (priority == 0) return 100;       // Combat: 100ms
            if (priority < 50) return 250;       // Active: 250ms
            if (priority < 100) return 500;      // Normal: 500ms
            if (priority < 200) return 1000;     // Low: 1s
            return 5000;                         // Idle: 5s
        }
    };
};

// Optimized AI update loop
class OptimizedAIController {
public:
    void UpdateAllBots(uint32 diff) {
        // Get bots that need updating
        auto botsToUpdate = _updateQueue.GetBotsToUpdate(100);
        
        // Batch process updates
        AIOptimizer::BatchProcess(botsToUpdate, 
            [diff](Player* bot) {
                // Profile update time
                auto start = std::chrono::high_resolution_clock::now();
                
                bot->AI()->UpdateAI(diff);
                
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>
                               (end - start).count();
                
                if (duration > Performance::MAX_AI_UPDATE_MICROSECONDS) {
                    TC_LOG_WARN("performance", 
                        "Bot %s AI update took %uμs (limit: %uμs)",
                        bot->GetName().c_str(), duration,
                        Performance::MAX_AI_UPDATE_MICROSECONDS);
                }
            });
    }
    
private:
    AIOptimizer::PriorityUpdateQueue _updateQueue;
};
```

### Combat Optimization

```cpp
// src/modules/Playerbot/Performance/CombatOptimizer.h

class CombatOptimizer {
public:
    // Ability caching
    struct CachedAbility {
        uint32 spellId;
        bool canCast;
        uint32 cooldownEnd;
        uint32 resource;
        float range;
        uint32 lastCheck;
    };
    
    class AbilityCache {
    public:
        bool CanCastCached(Player* bot, uint32 spellId, Unit* target) {
            uint64 key = GetKey(bot->GetGUID(), spellId);
            auto it = _cache.find(key);
            
            uint32 now = getMSTime();
            
            // Cache hit and still valid
            if (it != _cache.end() && now - it->second.lastCheck < 100) {
                return it->second.canCast && now >= it->second.cooldownEnd;
            }
            
            // Cache miss or expired - recalculate
            CachedAbility ability;
            ability.spellId = spellId;
            ability.canCast = bot->HasSpell(spellId) && 
                            bot->GetSpellCooldownDelay(spellId) == 0;
            ability.cooldownEnd = bot->GetSpellCooldownEnd(spellId);
            ability.resource = CalculateResourceCost(bot, spellId);
            ability.range = GetSpellRange(spellId);
            ability.lastCheck = now;
            
            _cache[key] = ability;
            return ability.canCast;
        }
        
        void InvalidateBot(ObjectGuid guid) {
            // Remove all cache entries for this bot
            for (auto it = _cache.begin(); it != _cache.end();) {
                if ((it->first >> 32) == guid.GetRawValue()) {
                    it = _cache.erase(it);
                } else {
                    ++it;
                }
            }
        }
        
    private:
        phmap::flat_hash_map<uint64, CachedAbility> _cache;
        
        uint64 GetKey(ObjectGuid guid, uint32 spellId) {
            return (uint64(guid.GetRawValue()) << 32) | spellId;
        }
    };
    
    // Target caching
    class TargetCache {
    public:
        Unit* GetBestTargetCached(Player* bot) {
            auto it = _targetCache.find(bot->GetGUID());
            
            uint32 now = getMSTime();
            if (it != _targetCache.end() && now - it->second.lastUpdate < 500) {
                if (IsValidTarget(it->second.target)) {
                    return it->second.target;
                }
            }
            
            // Recalculate best target
            Unit* newTarget = CalculateBestTarget(bot);
            
            CachedTarget cached;
            cached.target = newTarget;
            cached.lastUpdate = now;
            _targetCache[bot->GetGUID()] = cached;
            
            return newTarget;
        }
        
    private:
        struct CachedTarget {
            Unit* target;
            uint32 lastUpdate;
        };
        
        phmap::flat_hash_map<ObjectGuid, CachedTarget> _targetCache;
    };
};
```

## Memory Optimization

### Object Pooling

```cpp
// src/modules/Playerbot/Performance/ObjectPooling.h

template<typename T>
class ThreadSafeObjectPool {
public:
    ThreadSafeObjectPool(size_t initialSize = 100, size_t maxSize = 10000)
        : _maxSize(maxSize) {
        for (size_t i = 0; i < initialSize; ++i) {
            _available.push(std::make_unique<T>());
        }
    }
    
    T* Acquire() {
        std::unique_lock<std::mutex> lock(_mutex);
        
        if (_available.empty()) {
            if (_totalAllocated < _maxSize) {
                _totalAllocated++;
                return new T();
            } else {
                // Wait for object to become available
                _condition.wait(lock, [this] { return !_available.empty(); });
            }
        }
        
        T* obj = _available.front().release();
        _available.pop();
        _inUse.insert(obj);
        
        return obj;
    }
    
    void Release(T* obj) {
        if (!obj) return;
        
        std::unique_lock<std::mutex> lock(_mutex);
        
        if (_inUse.erase(obj)) {
            obj->Reset();  // Object must implement Reset()
            _available.push(std::unique_ptr<T>(obj));
            _condition.notify_one();
        } else {
            delete obj;  // Not from pool
        }
    }
    
    size_t GetPoolSize() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _available.size() + _inUse.size();
    }
    
    size_t GetAvailable() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _available.size();
    }
    
private:
    mutable std::mutex _mutex;
    std::condition_variable _condition;
    std::queue<std::unique_ptr<T>> _available;
    std::unordered_set<T*> _inUse;
    size_t _maxSize;
    size_t _totalAllocated = 0;
};

// Pooled objects
struct PooledAction {
    uint32 spellId = 0;
    Unit* target = nullptr;
    float priority = 0.0f;
    
    void Reset() {
        spellId = 0;
        target = nullptr;
        priority = 0.0f;
    }
};

struct PooledPathNode {
    G3D::Vector3 position;
    float g = 0, h = 0;
    PooledPathNode* parent = nullptr;
    
    void Reset() {
        position = G3D::Vector3();
        g = h = 0;
        parent = nullptr;
    }
};

// Global pools
namespace Pools {
    static ThreadSafeObjectPool<PooledAction> ActionPool(1000, 10000);
    static ThreadSafeObjectPool<PooledPathNode> PathNodePool(10000, 100000);
    static ThreadSafeObjectPool<PathfindingManager::Path> PathPool(100, 1000);
}
```

### Memory-Efficient Data Structures

```cpp
// src/modules/Playerbot/Performance/EfficientStructures.h

// Compact bot state (32 bytes instead of potentially hundreds)
struct CompactBotState {
    uint32 health : 20;        // 0-1048575 (enough for health)
    uint32 power : 20;         // 0-1048575 (enough for mana/rage/etc)
    uint32 targetGuid : 24;    // Lower 24 bits of target GUID
    
    uint8 level;
    uint8 class_ : 4;
    uint8 spec : 4;
    
    uint8 role : 2;            // 0-3 (tank/healer/dps)
    uint8 inCombat : 1;
    uint8 isMoving : 1;
    uint8 isCasting : 1;
    uint8 isDead : 1;
    uint8 needsHelp : 1;
    uint8 hasAggro : 1;
    
    uint16 zoneId;
    
    // 32 bytes total
};

// Spatial index for efficient range queries
class SpatialIndex {
public:
    void Insert(Player* bot) {
        Position pos = bot->GetPosition();
        uint32 cellX = uint32(pos.GetPositionX() / CELL_SIZE);
        uint32 cellY = uint32(pos.GetPositionY() / CELL_SIZE);
        uint64 cellKey = (uint64(cellX) << 32) | cellY;
        
        _cells[cellKey].push_back(bot);
    }
    
    std::vector<Player*> GetNearby(Position center, float range) {
        std::vector<Player*> nearby;
        
        uint32 cellRange = uint32(range / CELL_SIZE) + 1;
        uint32 centerX = uint32(center.GetPositionX() / CELL_SIZE);
        uint32 centerY = uint32(center.GetPositionY() / CELL_SIZE);
        
        for (uint32 dx = 0; dx <= cellRange; ++dx) {
            for (uint32 dy = 0; dy <= cellRange; ++dy) {
                CheckCell(centerX + dx, centerY + dy, center, range, nearby);
                if (dx > 0) CheckCell(centerX - dx, centerY + dy, center, range, nearby);
                if (dy > 0) CheckCell(centerX + dx, centerY - dy, center, range, nearby);
                if (dx > 0 && dy > 0) CheckCell(centerX - dx, centerY - dy, center, range, nearby);
            }
        }
        
        return nearby;
    }
    
private:
    static constexpr float CELL_SIZE = 100.0f;  // 100 yard cells
    phmap::flat_hash_map<uint64, std::vector<Player*>> _cells;
    
    void CheckCell(uint32 x, uint32 y, Position center, float range, 
                  std::vector<Player*>& result) {
        uint64 cellKey = (uint64(x) << 32) | y;
        auto it = _cells.find(cellKey);
        if (it == _cells.end())
            return;
        
        for (Player* bot : it->second) {
            if (bot->GetDistance(center) <= range) {
                result.push_back(bot);
            }
        }
    }
};
```

## Database Optimization

### Query Optimization

```cpp
// src/modules/Playerbot/Performance/DatabaseOptimizer.h

class DatabaseOptimizer {
public:
    // Prepared statement caching
    class PreparedStatementCache {
    public:
        PlayerbotDatabasePreparedStatement* GetCached(uint32 index) {
            auto it = _cache.find(index);
            if (it != _cache.end() && !it->second.inUse) {
                it->second.inUse = true;
                return it->second.stmt;
            }
            
            // Create new statement
            auto stmt = new PlayerbotDatabasePreparedStatement(index);
            _cache[index] = {stmt, true};
            return stmt;
        }
        
        void Release(PlayerbotDatabasePreparedStatement* stmt) {
            for (auto& [index, cached] : _cache) {
                if (cached.stmt == stmt) {
                    cached.stmt->Clear();
                    cached.inUse = false;
                    return;
                }
            }
        }
        
    private:
        struct CachedStatement {
            PlayerbotDatabasePreparedStatement* stmt;
            bool inUse;
        };
        
        phmap::flat_hash_map<uint32, CachedStatement> _cache;
    };
    
    // Result caching
    template<typename Key, typename Result>
    class ResultCache {
    public:
        bool Get(const Key& key, Result& result) {
            auto it = _cache.find(key);
            if (it == _cache.end())
                return false;
            
            uint32 now = getMSTime();
            if (now - it->second.timestamp > _ttlMs) {
                _cache.erase(it);
                return false;
            }
            
            result = it->second.data;
            _hits++;
            return true;
        }
        
        void Set(const Key& key, const Result& result) {
            CachedResult cached;
            cached.data = result;
            cached.timestamp = getMSTime();
            
            _cache[key] = cached;
            _misses++;
            
            // Cleanup old entries periodically
            if (++_accessCount % 1000 == 0) {
                Cleanup();
            }
        }
        
        float GetHitRate() const {
            uint32 total = _hits + _misses;
            return total > 0 ? float(_hits) / total : 0.0f;
        }
        
    private:
        struct CachedResult {
            Result data;
            uint32 timestamp;
        };
        
        phmap::flat_hash_map<Key, CachedResult> _cache;
        uint32 _ttlMs = 30000;  // 30 second TTL
        uint32 _hits = 0;
        uint32 _misses = 0;
        uint32 _accessCount = 0;
        
        void Cleanup() {
            uint32 now = getMSTime();
            for (auto it = _cache.begin(); it != _cache.end();) {
                if (now - it->second.timestamp > _ttlMs) {
                    it = _cache.erase(it);
                } else {
                    ++it;
                }
            }
        }
    };
};

// Async database operations
class AsyncDatabaseWorker {
public:
    using Callback = std::function<void(QueryResult)>;
    
    void ExecuteAsync(const char* sql, Callback callback) {
        _workQueue.push([sql, callback]() {
            QueryResult result = PlayerbotDatabase.Query(sql);
            
            // Queue callback for main thread
            _callbackQueue.push([callback, result]() {
                callback(result);
            });
        });
    }
    
    void ProcessCallbacks() {
        Callback callback;
        while (_callbackQueue.try_pop(callback)) {
            callback();
        }
    }
    
private:
    tbb::concurrent_queue<std::function<void()>> _workQueue;
    tbb::concurrent_queue<Callback> _callbackQueue;
    std::thread _worker{[this]() { WorkerThread(); }};
    
    void WorkerThread() {
        std::function<void()> work;
        while (true) {
            if (_workQueue.try_pop(work)) {
                work();
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }
};
```

## Monitoring and Profiling

### Performance Monitoring Framework

```cpp
// src/modules/Playerbot/Performance/PerformanceMonitor.h

class PerformanceMonitor {
public:
    struct Metrics {
        // Timing metrics
        uint32 avgAIUpdateUs;
        uint32 maxAIUpdateUs;
        uint32 avgPathfindingMs;
        uint32 maxPathfindingMs;
        
        // Resource metrics
        float cpuUsagePercent;
        size_t memoryUsedBytes;
        size_t peakMemoryBytes;
        
        // Throughput metrics
        uint32 updatesPerSecond;
        uint32 actionsPerSecond;
        uint32 queriesPerSecond;
        
        // Cache metrics
        float abilityCacheHitRate;
        float targetCacheHitRate;
        float pathCacheHitRate;
        float databaseCacheHitRate;
    };
    
    static PerformanceMonitor& Instance() {
        static PerformanceMonitor instance;
        return instance;
    }
    
    // Recording methods
    void RecordAIUpdate(uint32 microseconds) {
        _aiUpdateTimes.push_back(microseconds);
        if (_aiUpdateTimes.size() > 1000)
            _aiUpdateTimes.pop_front();
        
        UpdateAIMetrics();
    }
    
    void RecordPathfinding(uint32 milliseconds) {
        _pathfindingTimes.push_back(milliseconds);
        if (_pathfindingTimes.size() > 100)
            _pathfindingTimes.pop_front();
    }
    
    // Reporting
    Metrics GetMetrics() const {
        Metrics m;
        
        // AI timing
        if (!_aiUpdateTimes.empty()) {
            uint32 sum = 0;
            m.maxAIUpdateUs = 0;
            for (uint32 time : _aiUpdateTimes) {
                sum += time;
                m.maxAIUpdateUs = std::max(m.maxAIUpdateUs, time);
            }
            m.avgAIUpdateUs = sum / _aiUpdateTimes.size();
        }
        
        // Memory
        m.memoryUsedBytes = GetProcessMemory();
        m.peakMemoryBytes = _peakMemory;
        
        // CPU
        m.cpuUsagePercent = GetProcessCPU();
        
        return m;
    }
    
    void LogReport() {
        Metrics m = GetMetrics();
        
        TC_LOG_INFO("performance", 
            "=== Bot Performance Report ===\n"
            "AI Update: avg=%uμs max=%uμs\n"
            "Pathfinding: avg=%ums max=%ums\n"
            "CPU: %.1f%%\n"
            "Memory: %zuMB (peak: %zuMB)\n"
            "Updates/sec: %u\n"
            "Cache hit rates: ability=%.1f%% target=%.1f%% path=%.1f%%",
            m.avgAIUpdateUs, m.maxAIUpdateUs,
            m.avgPathfindingMs, m.maxPathfindingMs,
            m.cpuUsagePercent,
            m.memoryUsedBytes / 1048576, m.peakMemoryBytes / 1048576,
            m.updatesPerSecond,
            m.abilityCacheHitRate * 100, m.targetCacheHitRate * 100,
            m.pathCacheHitRate * 100);
    }
    
private:
    std::deque<uint32> _aiUpdateTimes;
    std::deque<uint32> _pathfindingTimes;
    size_t _peakMemory = 0;
    
    void UpdateAIMetrics() {
        size_t currentMem = GetProcessMemory();
        _peakMemory = std::max(_peakMemory, currentMem);
    }
    
    size_t GetProcessMemory() const {
#ifdef _WIN32
        PROCESS_MEMORY_COUNTERS pmc;
        GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
        return pmc.WorkingSetSize;
#else
        // Linux implementation
        std::ifstream status("/proc/self/status");
        std::string line;
        while (std::getline(status, line)) {
            if (line.substr(0, 6) == "VmRSS:") {
                return std::stoull(line.substr(6)) * 1024;
            }
        }
        return 0;
#endif
    }
    
    float GetProcessCPU() const {
        static auto lastTime = std::chrono::steady_clock::now();
        static clock_t lastCPU = clock();
        
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>
                      (now - lastTime).count();
        
        clock_t currentCPU = clock();
        clock_t cpuUsed = currentCPU - lastCPU;
        
        lastTime = now;
        lastCPU = currentCPU;
        
        return (float(cpuUsed) / CLOCKS_PER_SEC) / (elapsed / 1000.0f) * 100.0f;
    }
};

// Profiling macros
#define PROFILE_AI_UPDATE(bot) \
    ProfileScope _profile([](uint32 us) { \
        PerformanceMonitor::Instance().RecordAIUpdate(us); \
    })

#define PROFILE_PATHFINDING() \
    ProfileScope _profile([](uint32 us) { \
        PerformanceMonitor::Instance().RecordPathfinding(us / 1000); \
    })

class ProfileScope {
public:
    ProfileScope(std::function<void(uint32)> callback) 
        : _callback(callback) {
        _start = std::chrono::high_resolution_clock::now();
    }
    
    ~ProfileScope() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>
                       (end - _start).count();
        _callback(duration);
    }
    
private:
    std::chrono::high_resolution_clock::time_point _start;
    std::function<void(uint32)> _callback;
};
```

## Configuration

### Performance Configuration

```ini
# playerbots.conf - Performance section

###################################################################################################
# PLAYERBOT PERFORMANCE CONFIGURATION
#
#    Playerbot.Performance.MaxBotsPerUpdate
#        Maximum number of bots to update per server tick
#        Default: 100
#
#    Playerbot.Performance.EnableBatching
#        Enable batch processing of bot updates using TBB
#        Default: 1
#
#    Playerbot.Performance.BatchSize
#        Number of bots to process in each parallel batch
#        Default: 64
#
#    Playerbot.Performance.EnableCaching
#        Enable ability and target caching
#        Default: 1
#
#    Playerbot.Performance.CacheTTL
#        Cache time-to-live in milliseconds
#        Default: 100
#
#    Playerbot.Performance.EnableProfiling
#        Enable performance profiling and metrics collection
#        Default: 0
#
#    Playerbot.Performance.ProfileReportInterval
#        Interval in seconds between performance reports
#        Default: 60
#
#    Playerbot.Performance.MemoryLimit
#        Maximum memory usage for bot system in MB
#        Default: 4096
#
#    Playerbot.Performance.EnableObjectPooling
#        Enable object pooling for frequently allocated objects
#        Default: 1
#
#    Playerbot.Performance.PathfindingThreads
#        Number of threads for parallel pathfinding
#        Default: 4
#
#    Playerbot.Performance.DatabaseConnectionPool
#        Size of dedicated database connection pool for bots
#        Default: 10
#
###################################################################################################

Playerbot.Performance.MaxBotsPerUpdate = 100
Playerbot.Performance.EnableBatching = 1
Playerbot.Performance.BatchSize = 64
Playerbot.Performance.EnableCaching = 1
Playerbot.Performance.CacheTTL = 100
Playerbot.Performance.EnableProfiling = 0
Playerbot.Performance.ProfileReportInterval = 60
Playerbot.Performance.MemoryLimit = 4096
Playerbot.Performance.EnableObjectPooling = 1
Playerbot.Performance.PathfindingThreads = 4
Playerbot.Performance.DatabaseConnectionPool = 10
```

## Next Steps

1. **Implement Performance Monitor** - Metrics collection
2. **Add Object Pooling** - Memory optimization
3. **Create Cache System** - Reduce redundant calculations
4. **Add Profiling** - Identify bottlenecks
5. **Optimize Database** - Async queries and caching

---

**Status**: Ready for Implementation
**Dependencies**: All Phase 3 systems
**Estimated Time**: Ongoing throughout Phase 3