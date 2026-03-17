# BotSessionMgr Complete Implementation Specification

## Overview
High-performance session manager with Intel TBB work-stealing thread pool, parallel hashmap, and enterprise-grade optimizations for managing thousands of concurrent bot sessions.

## Class Declaration

### Header File: `src/modules/Playerbot/Session/BotSessionMgr.h`

```cpp
/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#ifndef BOT_SESSION_MGR_H
#define BOT_SESSION_MGR_H

#include "Define.h"
#include "BotSession.h"
#include <parallel_hashmap/phmap.h>
#include <tbb/concurrent_vector.h>
#include <tbb/task_group.h>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <boost/lockfree/stack.hpp>
#include <atomic>
#include <shared_mutex>
#include <thread>
#include <span>

/**
 * CRITICAL IMPLEMENTATION REQUIREMENTS:
 *
 * 1. CONCURRENCY REQUIREMENTS:
 *    - MUST use phmap::parallel_flat_hash_map (8+ submaps)
 *    - MUST use Intel TBB work-stealing thread pool
 *    - MUST support 100-5000 concurrent sessions
 *    - MUST achieve linear scaling to 500 bots
 *
 * 2. BATCH PROCESSING REQUIREMENTS:
 *    - MUST group sessions in batches of 64
 *    - MUST use parallel batch updates with TBB
 *    - MUST implement SIMD optimization where applicable
 *    - MUST achieve < 0.05% CPU per bot
 *
 * 3. MEMORY MANAGEMENT REQUIREMENTS:
 *    - MUST use session pool with pre-allocation (1000 sessions)
 *    - MUST implement hibernation manager
 *    - MUST defragment memory every 60 seconds
 *    - MUST achieve 99% memory reduction for hibernated sessions
 *
 * 4. PERFORMANCE MONITORING REQUIREMENTS:
 *    - MUST track active/hibernated session counts
 *    - MUST monitor packets per second globally
 *    - MUST track total memory usage
 *    - MUST calculate average CPU percentage
 */
class TC_GAME_API BotSessionMgr final
{
public:
    // Singleton with thread-safe lazy initialization
    static BotSessionMgr* instance()
    {
        static BotSessionMgr instance;
        return &instance;
    }

    // Initialization with Intel TBB thread pool
    bool Initialize();
    void Shutdown();

    // === HIGH-PERFORMANCE SESSION MANAGEMENT ===

    // Session pool operations with object pooling
    BotSession* CreateSession(uint32 accountId);
    void ReleaseSession(uint32 accountId);
    BotSession* GetSession(uint32 accountId) const;

    // Batch session operations
    std::vector<BotSession*> CreateSessionBatch(std::vector<uint32> const& accountIds);
    void ReleaseSessionBatch(std::vector<uint32> const& accountIds);

    // === PARALLEL UPDATE SYSTEM ===

    // Intel TBB parallel session updates
    void UpdateAllSessions(uint32 diff);

    // Work-stealing batch processing
    void ProcessSessionBatch(std::span<BotSession*> batch, uint32 diff);

    // === MEMORY OPTIMIZATION SYSTEM ===

    // Hibernation management for memory efficiency
    void HibernateInactiveSessions();
    void ReactivateSessionsForMap(uint32 mapId);
    void ReactivateSessionsForGroup(uint32 groupId);

    // Memory defragmentation and cleanup
    void DefragmentMemory();
    void CollectGarbage();
    size_t GetTotalMemoryUsage() const;

    // === PERFORMANCE MONITORING ===

    struct GlobalMetrics {
        std::atomic<uint32> activeSessions{0};
        std::atomic<uint32> hibernatedSessions{0};
        std::atomic<uint64> totalPacketsPerSecond{0};
        std::atomic<uint64> totalMemoryMB{0};
        std::atomic<float> avgCpuPercent{0.0f};
        std::atomic<uint32> updateCyclesPerSecond{0};
        std::atomic<uint32> hibernationCyclesPerSecond{0};
        std::atomic<uint64> totalUpdatesProcessed{0};
        std::atomic<uint64> totalBatchesProcessed{0};
    };

    GlobalMetrics const& GetGlobalMetrics() const { return _globalMetrics; }

    // === CONFIGURATION MANAGEMENT ===

    void SetMaxConcurrentSessions(uint32 max) { _maxConcurrentSessions.store(max); }
    void SetUpdateBatchSize(uint32 size) { _updateBatchSize.store(size); }
    void SetHibernationThreshold(uint32 minutes) { _hibernationThresholdMs.store(minutes * 60000); }
    void SetWorkerThreadCount(uint32 count);

    // === STATISTICS AND MONITORING ===

    uint32 GetActiveSessionCount() const;
    uint32 GetHibernatedSessionCount() const;
    float GetSessionUpdateRate() const;
    float GetMemoryUsagePercent() const;
    float GetCpuUsagePercent() const;

    // Session distribution statistics
    std::map<uint32, uint32> GetSessionsByMap() const;
    std::vector<uint32> GetMostActiveAccounts(uint32 count = 10) const;

private:
    BotSessionMgr() = default;
    ~BotSessionMgr() = default;

    // Prevent copying
    BotSessionMgr(BotSessionMgr const&) = delete;
    BotSessionMgr& operator=(BotSessionMgr const&) = delete;

    // === HIGH-PERFORMANCE DATA STRUCTURES ===

    // Parallel hashmap with 8 submaps for maximum concurrency
    phmap::parallel_flat_hash_map<uint32, std::unique_ptr<BotSession>,
                                   phmap::priv::hash_default_hash<uint32>,
                                   phmap::priv::hash_default_eq<uint32>,
                                   std::allocator<std::pair<const uint32, std::unique_ptr<BotSession>>>,
                                   8> _sessions;

    // Session pool with pre-allocation for zero-allocation at runtime
    struct SessionPool {
        static constexpr size_t POOL_SIZE = 1000;
        static constexpr size_t BATCH_ALLOCATION_SIZE = 100;

        // Pre-allocated session storage
        std::vector<std::unique_ptr<BotSession>> _preallocatedSessions;

        // Lock-free stack for available sessions
        boost::lockfree::stack<BotSession*> _availableSessions{POOL_SIZE};

        // Pool statistics
        std::atomic<size_t> _allocatedCount{0};
        std::atomic<size_t> _availableCount{0};

        bool Initialize();
        BotSession* Acquire(uint32 accountId);
        void Release(BotSession* session);
        void Cleanup();

        size_t GetAllocatedCount() const { return _allocatedCount.load(); }
        size_t GetAvailableCount() const { return _availableCount.load(); }
    };
    SessionPool _sessionPool;

    // === INTEL TBB THREAD POOL ===

    // Work-stealing task group for parallel processing
    std::unique_ptr<tbb::task_group> _updateTaskGroup;

    // Worker thread configuration
    std::atomic<uint32> _workerThreadCount{std::thread::hardware_concurrency()};

    // === BATCH PROCESSING SYSTEM ===

    // Batch configuration
    static constexpr size_t DEFAULT_BATCH_SIZE = 64;
    static constexpr size_t MAX_BATCH_SIZE = 256;
    std::atomic<uint32> _updateBatchSize{DEFAULT_BATCH_SIZE};

    // Update queue for session batching
    tbb::concurrent_vector<BotSession*> _updateQueue;
    tbb::concurrent_vector<BotSession*> _hibernationQueue;

    // === HIBERNATION SYSTEM ===

    struct HibernationManager {
        std::atomic<uint32> _hibernationThresholdMs{300000}; // 5 minutes
        std::atomic<uint32> _lastHibernationCycle{0};
        std::atomic<size_t> _totalHibernatedMemory{0};

        // Hibernation candidate tracking
        tbb::concurrent_vector<uint32> _hibernationCandidates;

        void UpdateCandidates(BotSession* session);
        void ProcessHibernationBatch(std::span<uint32> accountIds);
        size_t GetMemorySaved() const { return _totalHibernatedMemory.load(); }
    };
    HibernationManager _hibernationMgr;

    // === PERFORMANCE MONITORING ===

    mutable GlobalMetrics _globalMetrics;

    // Performance tracking
    std::chrono::steady_clock::time_point _lastMetricsUpdate;
    std::chrono::steady_clock::time_point _lastDefragmentation;
    std::atomic<uint32> _updateCycleCounter{0};

    // CPU usage tracking
    std::atomic<uint64> _totalCpuTimeUs{0};
    std::chrono::steady_clock::time_point _cpuTrackingStart;

    // === CONFIGURATION ===

    std::atomic<uint32> _maxConcurrentSessions{5000};
    std::atomic<uint32> _hibernationThresholdMs{300000}; // 5 minutes
    std::atomic<bool> _enableSIMDOptimizations{true};
    std::atomic<bool> _enableAutoDefragmentation{true};

    // === INTERNAL HIGH-PERFORMANCE METHODS ===

    // Parallel update implementation
    void ParallelUpdateBatch(std::span<BotSession*> sessions, uint32 diff);

    // SIMD-optimized operations
    void SimdUpdateSessionMetrics(std::span<BotSession*> sessions);
    void SimdClearSessionBuffers(std::span<BotSession*> sessions);

    // Memory management
    void UpdateGlobalMetrics();
    void CollectSessionStatistics();
    void OptimizeMemoryLayout();

    // Hibernation operations
    void IdentifyHibernationCandidates();
    void ProcessHibernationQueue();

    // Thread pool management
    void InitializeThreadPool();
    void ShutdownThreadPool();

    // Internal utility methods
    inline bool ShouldDefragmentMemory() const;
    inline bool ShouldUpdateMetrics() const;
    inline void UpdateCpuMetrics(std::chrono::microseconds cpuTime);
};

// Global accessor
#define sBotSessionMgr BotSessionMgr::instance()

#endif // BOT_SESSION_MGR_H
```

## Implementation Requirements

### Dependency Integration
```cmake
# Required in CMakeLists.txt
find_package(TBB REQUIRED)
find_package(phmap REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

target_link_libraries(playerbot-session
    PRIVATE
        TBB::tbb
        phmap::phmap
        Boost::system)

target_compile_definitions(playerbot-session
    PRIVATE
        -DTBB_USE_EXCEPTIONS=1
        -DPHMAP_USE_TBB=1)
```

### Initialization Implementation
```cpp
bool BotSessionMgr::Initialize()
{
    TC_LOG_INFO("module.playerbot.session",
        "Initializing BotSessionMgr with high-performance optimizations");

    // Initialize Intel TBB thread pool
    InitializeThreadPool();

    // Initialize session pool with pre-allocation
    if (!_sessionPool.Initialize()) {
        TC_LOG_ERROR("module.playerbot.session",
            "Failed to initialize session pool");
        return false;
    }

    // Initialize parallel hashmap with optimal submap count
    _sessions.reserve(1000); // Reserve for 1000 sessions

    // Initialize performance tracking
    _lastMetricsUpdate = std::chrono::steady_clock::now();
    _lastDefragmentation = _lastMetricsUpdate;
    _cpuTrackingStart = _lastMetricsUpdate;

    // Configure batch sizes based on CPU cores
    uint32 optimalBatchSize = std::min(
        static_cast<uint32>(std::thread::hardware_concurrency() * 16),
        static_cast<uint32>(MAX_BATCH_SIZE));
    _updateBatchSize.store(optimalBatchSize);

    TC_LOG_INFO("module.playerbot.session",
        "BotSessionMgr initialized with {} worker threads, batch size {}",
        _workerThreadCount.load(), _updateBatchSize.load());

    return true;
}
```

### High-Performance Update Implementation
```cpp
void BotSessionMgr::UpdateAllSessions(uint32 diff)
{
    auto updateStartTime = std::chrono::high_resolution_clock::now();

    // Collect active sessions for parallel processing
    _updateQueue.clear();

    // Collect all active sessions (lock-free read from parallel hashmap)
    _sessions.for_each([this](auto const& pair) {
        BotSession* session = pair.second.get();
        if (session && session->IsActive()) {
            _updateQueue.push_back(session);
        }
    });

    size_t totalSessions = _updateQueue.size();
    if (totalSessions == 0) return;

    // Process sessions in parallel batches using Intel TBB
    size_t batchSize = _updateBatchSize.load();
    size_t numBatches = (totalSessions + batchSize - 1) / batchSize;

    // Intel TBB parallel_for with blocked_range for optimal work distribution
    tbb::parallel_for(
        tbb::blocked_range<size_t>(0, numBatches),
        [this, diff, batchSize, totalSessions](const tbb::blocked_range<size_t>& range) {
            for (size_t batchIdx = range.begin(); batchIdx != range.end(); ++batchIdx) {
                size_t startIdx = batchIdx * batchSize;
                size_t endIdx = std::min(startIdx + batchSize, totalSessions);

                if (startIdx < endIdx) {
                    std::span<BotSession*> batch(_updateQueue.data() + startIdx, endIdx - startIdx);
                    ProcessSessionBatch(batch, diff);
                }
            }
        }
    );

    // Update global metrics
    auto updateEndTime = std::chrono::high_resolution_clock::now();
    auto updateDuration = std::chrono::duration_cast<std::chrono::microseconds>(
        updateEndTime - updateStartTime);

    UpdateCpuMetrics(updateDuration);
    _updateCycleCounter.fetch_add(1, std::memory_order_relaxed);
    _globalMetrics.totalUpdatesProcessed.fetch_add(totalSessions, std::memory_order_relaxed);
    _globalMetrics.totalBatchesProcessed.fetch_add(numBatches, std::memory_order_relaxed);

    // Periodic maintenance operations
    if (ShouldUpdateMetrics()) {
        UpdateGlobalMetrics();
    }

    if (ShouldDefragmentMemory()) {
        DefragmentMemory();
    }

    // Process hibernation candidates
    ProcessHibernationQueue();
}
```

### Session Pool Implementation
```cpp
bool BotSessionMgr::SessionPool::Initialize()
{
    TC_LOG_INFO("module.playerbot.session",
        "Initializing session pool with {} pre-allocated sessions", POOL_SIZE);

    try {
        // Pre-allocate sessions to avoid runtime allocation
        _preallocatedSessions.reserve(POOL_SIZE);

        for (size_t i = 0; i < POOL_SIZE; ++i) {
            // Create session with placeholder account ID
            auto session = std::make_unique<BotSession>(0);
            BotSession* rawPtr = session.get();

            _preallocatedSessions.push_back(std::move(session));
            _availableSessions.push(rawPtr);
        }

        _availableCount.store(POOL_SIZE);
        TC_LOG_INFO("module.playerbot.session",
            "Session pool initialized successfully with {} available sessions",
            _availableCount.load());

        return true;
    }
    catch (std::exception const& e) {
        TC_LOG_ERROR("module.playerbot.session",
            "Failed to initialize session pool: {}", e.what());
        return false;
    }
}

BotSession* BotSessionMgr::SessionPool::Acquire(uint32 accountId)
{
    BotSession* session = nullptr;

    if (_availableSessions.pop(session)) {
        // Reinitialize session with new account ID
        // Note: This requires a Reset method in BotSession
        session->Reset(accountId);

        _allocatedCount.fetch_add(1, std::memory_order_relaxed);
        _availableCount.fetch_sub(1, std::memory_order_relaxed);

        return session;
    }

    // Pool exhausted - this should not happen with proper sizing
    TC_LOG_WARN("module.playerbot.session",
        "Session pool exhausted, creating session on-demand for account {}",
        accountId);

    return new BotSession(accountId);
}

void BotSessionMgr::SessionPool::Release(BotSession* session)
{
    if (!session) return;

    // Clean up session state
    session->Cleanup();

    // Return to pool if there's space
    if (_availableSessions.push(session)) {
        _allocatedCount.fetch_sub(1, std::memory_order_relaxed);
        _availableCount.fetch_add(1, std::memory_order_relaxed);
    } else {
        // Pool is full - delete the session
        delete session;
    }
}
```

### Hibernation System Implementation
```cpp
void BotSessionMgr::HibernateInactiveSessions()
{
    auto hibernationStart = std::chrono::high_resolution_clock::now();

    // Collect hibernation candidates
    _hibernationMgr._hibernationCandidates.clear();

    _sessions.for_each([this](auto const& pair) {
        BotSession* session = pair.second.get();
        if (session && session->ShouldHibernate()) {
            _hibernationMgr._hibernationCandidates.push_back(pair.first);
        }
    });

    size_t candidateCount = _hibernationMgr._hibernationCandidates.size();
    if (candidateCount == 0) return;

    // Process hibernation in parallel batches
    constexpr size_t HIBERNATION_BATCH_SIZE = 32;
    size_t numBatches = (candidateCount + HIBERNATION_BATCH_SIZE - 1) / HIBERNATION_BATCH_SIZE;

    tbb::parallel_for(
        tbb::blocked_range<size_t>(0, numBatches),
        [this, candidateCount](const tbb::blocked_range<size_t>& range) {
            for (size_t batchIdx = range.begin(); batchIdx != range.end(); ++batchIdx) {
                size_t startIdx = batchIdx * HIBERNATION_BATCH_SIZE;
                size_t endIdx = std::min(startIdx + HIBERNATION_BATCH_SIZE, candidateCount);

                if (startIdx < endIdx) {
                    std::span<uint32> batch(
                        _hibernationMgr._hibernationCandidates.data() + startIdx,
                        endIdx - startIdx);
                    _hibernationMgr.ProcessHibernationBatch(batch);
                }
            }
        }
    );

    auto hibernationEnd = std::chrono::high_resolution_clock::now();
    auto hibernationDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        hibernationEnd - hibernationStart);

    TC_LOG_DEBUG("module.playerbot.session",
        "Hibernated {} sessions in {}ms",
        candidateCount, hibernationDuration.count());

    _globalMetrics.hibernationCyclesPerSecond.fetch_add(1, std::memory_order_relaxed);
}
```

## Performance Requirements Validation

### CPU Performance Tests
```cpp
// MANDATORY: CPU usage per bot must be < 0.05%
TEST(BotSessionMgrPerformanceTest, CpuUsagePerBot) {
    auto* mgr = sBotSessionMgr;

    // Create 100 sessions
    std::vector<uint32> accountIds;
    for (uint32 i = 10000; i < 10100; ++i) {
        accountIds.push_back(i);
    }

    auto sessions = mgr->CreateSessionBatch(accountIds);

    // Measure CPU usage over 1000 update cycles
    auto startTime = std::chrono::high_resolution_clock::now();

    for (int cycle = 0; cycle < 1000; ++cycle) {
        mgr->UpdateAllSessions(100); // 100ms per cycle
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto totalCpuTime = std::chrono::duration_cast<std::chrono::microseconds>(
        endTime - startTime);

    double cpuPercentPerBot = (totalCpuTime.count() / 1000000.0) / 100.0; // 100 seconds total, 100 bots

    // REQUIREMENT: < 0.05% CPU per bot
    EXPECT_LT(cpuPercentPerBot, 0.0005);

    mgr->ReleaseSessionBatch(accountIds);
}
```

### Memory Performance Tests
```cpp
// MANDATORY: Memory usage must scale linearly
TEST(BotSessionMgrPerformanceTest, LinearMemoryScaling) {
    auto* mgr = sBotSessionMgr;

    std::vector<size_t> sessionCounts = {50, 100, 200, 400};
    std::vector<size_t> memoryUsages;

    for (size_t sessionCount : sessionCounts) {
        size_t baselineMemory = GetProcessMemoryUsage();

        // Create sessions
        std::vector<uint32> accountIds;
        for (uint32 i = 0; i < sessionCount; ++i) {
            accountIds.push_back(20000 + i);
        }

        auto sessions = mgr->CreateSessionBatch(accountIds);

        size_t sessionMemory = GetProcessMemoryUsage() - baselineMemory;
        memoryUsages.push_back(sessionMemory);

        mgr->ReleaseSessionBatch(accountIds);
    }

    // Verify linear scaling (R² > 0.95)
    double correlation = CalculateCorrelation(sessionCounts, memoryUsages);
    EXPECT_GT(correlation, 0.95);
}
```

### Concurrency Performance Tests
```cpp
// MANDATORY: Must support 1000+ concurrent sessions
TEST(BotSessionMgrPerformanceTest, HighConcurrency) {
    auto* mgr = sBotSessionMgr;

    constexpr size_t SESSION_COUNT = 1000;
    std::vector<uint32> accountIds;
    for (uint32 i = 0; i < SESSION_COUNT; ++i) {
        accountIds.push_back(30000 + i);
    }

    // Create all sessions
    auto startTime = std::chrono::high_resolution_clock::now();
    auto sessions = mgr->CreateSessionBatch(accountIds);
    auto creationTime = std::chrono::high_resolution_clock::now();

    // Run update cycles
    for (int cycle = 0; cycle < 100; ++cycle) {
        mgr->UpdateAllSessions(100);
    }
    auto updateTime = std::chrono::high_resolution_clock::now();

    // Cleanup
    mgr->ReleaseSessionBatch(accountIds);
    auto cleanupTime = std::chrono::high_resolution_clock::now();

    // Performance requirements
    auto totalCreationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        creationTime - startTime).count();
    auto avgUpdateMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        updateTime - creationTime).count() / 100;

    // REQUIREMENTS
    EXPECT_LT(totalCreationMs, 5000);  // < 5 seconds to create 1000 sessions
    EXPECT_LT(avgUpdateMs, 100);       // < 100ms per update cycle
}
```

## Integration Points

### World Server Integration
```cpp
#ifdef PLAYERBOT_ENABLED
void World::Update(uint32 diff) {
    // ... existing World update code ...

    // Update bot sessions if enabled
    if (sPlayerbotConfig->GetBool("Playerbot.Enable", false)) {
        sBotSessionMgr->UpdateAllSessions(diff);
    }
}
#endif
```

### Configuration Integration
```cpp
// In playerbots.conf
Playerbot.SessionMgr.MaxConcurrentSessions = 5000
Playerbot.SessionMgr.UpdateBatchSize = 64
Playerbot.SessionMgr.WorkerThreads = 0  # 0 = auto-detect
Playerbot.SessionMgr.HibernationThresholdMinutes = 5
Playerbot.SessionMgr.EnableSIMD = 1
Playerbot.SessionMgr.EnableAutoDefragmentation = 1
```

**ENTERPRISE-GRADE, HIGH-PERFORMANCE IMPLEMENTATION. ALL DEPENDENCIES REQUIRED.**