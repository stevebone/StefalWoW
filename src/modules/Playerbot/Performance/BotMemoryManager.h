/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include <memory>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <string>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <array>

namespace Playerbot
{

// Memory allocation categories for tracking
enum class MemoryCategory : uint8
{
    BOT_AI_STATE        = 0,  // Bot AI decision state data
    PERFORMANCE_METRICS = 1,  // Performance monitoring data
    COMBAT_DATA         = 2,  // Combat-related data structures
    MOVEMENT_DATA       = 3,  // Movement and pathfinding data
    SPELL_DATA          = 4,  // Spell casting and cooldown data
    INVENTORY_DATA      = 5,  // Inventory and equipment data
    SOCIAL_DATA         = 6,  // Guild, group, and social data
    QUEST_DATA          = 7,  // Quest and objective data
    CACHE_DATA          = 8,  // Cached game data
    TEMPORARY_DATA      = 9,  // Short-lived temporary allocations
    NETWORKING_DATA     = 10, // Network packet and session data
    DATABASE_CACHE      = 11, // Database query result cache
    UNKNOWN             = 12  // Uncategorized allocations
};

// Memory usage statistics for a single category
struct CategoryMemoryStats
{
    ::std::atomic<uint64_t> totalAllocated{0};     // Total bytes allocated
    ::std::atomic<uint64_t> totalDeallocated{0};   // Total bytes deallocated
    ::std::atomic<uint64_t> currentUsage{0};       // Current bytes in use
    ::std::atomic<uint64_t> peakUsage{0};          // Peak bytes usage
    ::std::atomic<uint32_t> allocationCount{0};    // Number of allocations
    ::std::atomic<uint32_t> deallocationCount{0};  // Number of deallocations
    ::std::atomic<uint64_t> lastAllocation{0};     // Timestamp of last allocation
    ::std::atomic<uint64_t> lastDeallocation{0};   // Timestamp of last deallocation

    void RecordAllocation(uint64_t size);
    void RecordDeallocation(uint64_t size);
    void Reset();
    double GetFragmentationRatio() const;
    double GetAllocationRate() const; // Allocations per second
};

// Per-bot memory tracking
struct BotMemoryProfile
{
    uint32_t botGuid;
    ::std::array<CategoryMemoryStats, 13> categoryStats;
    ::std::atomic<uint64_t> totalMemoryUsage{0};
    ::std::atomic<uint64_t> peakMemoryUsage{0};
    ::std::atomic<uint64_t> lastMemoryCheck{0};
    ::std::atomic<uint32_t> memoryLeakCount{0};
    ::std::atomic<uint32_t> memoryOptimizations{0};

    // Memory efficiency metrics
    ::std::atomic<double> memoryEfficiency{1.0};    // 0.0 to 1.0 (higher is better)
    ::std::atomic<double> fragmentationRatio{0.0};  // 0.0 to 1.0 (lower is better)
    ::std::atomic<double> cacheHitRatio{0.0};       // 0.0 to 1.0 (higher is better)

    BotMemoryProfile() : botGuid(0) {}
    explicit BotMemoryProfile(uint32_t guid) : botGuid(guid) {}

    uint64_t GetTotalUsage() const;
    double CalculateEfficiency() const;
    void UpdateMemoryMetrics();
};

// System-wide memory analytics
struct SystemMemoryAnalytics
{
    ::std::atomic<uint64_t> totalSystemMemory{0};
    ::std::atomic<uint64_t> totalBotMemory{0};
    ::std::atomic<uint64_t> availableSystemMemory{0};
    ::std::atomic<double> systemMemoryUsagePercent{0.0};
    ::std::atomic<double> botMemoryUsagePercent{0.0};

    // Performance impact tracking
    ::std::atomic<uint32_t> memoryPressureEvents{0};
    ::std::atomic<uint32_t> garbageCollectionEvents{0};
    ::std::atomic<uint64_t> totalGcTime{0}; // Microseconds

    // Memory optimization results
    ::std::atomic<uint64_t> memoryReclaimed{0};
    ::std::atomic<uint32_t> optimizationsPerformed{0};
    ::std::atomic<double> averageOptimizationGain{0.0};

    void UpdateSystemMetrics();
    double CalculateMemoryPressure() const;
    bool IsMemoryPressureHigh() const;
};

// Memory leak detection entry
struct MemoryLeakEntry
{
    void* address;
    uint64_t size;
    MemoryCategory category;
    uint32_t botGuid;
    uint64_t allocationTime;
    ::std::string stackTrace;
    ::std::string context;

    MemoryLeakEntry(void* addr, uint64_t sz, MemoryCategory cat, uint32_t guid, ::std::string ctx)
        : address(addr), size(sz), category(cat), botGuid(guid), context(::std::move(ctx))
    {
        allocationTime = ::std::chrono::duration_cast<::std::chrono::microseconds>(
            ::std::chrono::steady_clock::now().time_since_epoch()).count();
    }
};

// Smart pointer with memory tracking
template<typename T>
class TrackedPtr
{
public:
    TrackedPtr() : _ptr(nullptr), _category(MemoryCategory::UNKNOWN), _botGuid(0) {}

    TrackedPtr(T* ptr, MemoryCategory category, uint32_t botGuid)
        : _ptr(ptr), _category(category), _botGuid(botGuid)
    {
        if (_ptr)
            RecordAllocation();
    }

    ~TrackedPtr()
    {
        if (_ptr)
        {
            RecordDeallocation();
            delete _ptr;
        }
    }

    // Move constructor
    TrackedPtr(TrackedPtr&& other) noexcept
        : _ptr(other._ptr), _category(other._category), _botGuid(other._botGuid)
    {
        other._ptr = nullptr;
    }

    // Move assignment
    TrackedPtr& operator=(TrackedPtr&& other) noexcept
    {
        if (this != &other)
        {
            if (_ptr)
            {
                RecordDeallocation();
                delete _ptr;
            }
            _ptr = other._ptr;
            _category = other._category;
            _botGuid = other._botGuid;
            other._ptr = nullptr;
        }
        return *this;
    }

    // Disable copy
    TrackedPtr(const TrackedPtr&) = delete;
    TrackedPtr& operator=(const TrackedPtr&) = delete;

    T* get() const { return _ptr; }
    T& operator*() const { return *_ptr; }
    T* operator->() const { return _ptr; }
    explicit operator bool() const { return _ptr != nullptr; }

    void reset(T* ptr = nullptr)
    {
        if (_ptr)
        {
            RecordDeallocation();
            delete _ptr;
        }
        _ptr = ptr;
        if (_ptr)
            RecordAllocation();
    }

private:
    void RecordAllocation();
    void RecordDeallocation();

    T* _ptr;
    MemoryCategory _category;
    uint32_t _botGuid;
};

// Memory pool for frequent allocations
template<typename T, size_t PoolSize = 1024>
class MemoryPool
{
public:
    MemoryPool(MemoryCategory category, uint32_t botGuid)
        : _category(category), _botGuid(botGuid), _nextFree(0)
    {
        // Pre-allocate pool
        _pool = static_cast<T*>(::std::aligned_alloc(alignof(T), sizeof(T) * PoolSize));
        if (_pool)
        {
            // Initialize free list
            for (size_t i = 0; i < PoolSize - 1; ++i)
            {
                *reinterpret_cast<size_t*>(&_pool[i]) = i + 1;
            }
            *reinterpret_cast<size_t*>(&_pool[PoolSize - 1]) = SIZE_MAX;
        }
    }

    ~MemoryPool()
    {
        if (_pool)
        {
            ::std::free(_pool);
        }
    }

    T* Allocate()
    {
        ::std::lock_guard lock(_mutex);

        if (_nextFree == SIZE_MAX)
            return nullptr; // Pool exhausted

        T* result = &_pool[_nextFree];
        _nextFree = *reinterpret_cast<size_t*>(result);

        _allocatedCount++;
        RecordPoolAllocation();

        return result;
    }

    void Deallocate(T* ptr)
    {
        if (!ptr || ptr < _pool || ptr >= _pool + PoolSize)
            return; // Invalid pointer

        ::std::lock_guard lock(_mutex);

        size_t index = ptr - _pool;
        *reinterpret_cast<size_t*>(ptr) = _nextFree;
        _nextFree = index;

        _deallocatedCount++;
        RecordPoolDeallocation();
    }

    size_t GetAllocatedCount() const { return _allocatedCount; }
    size_t GetAvailableCount() const { return PoolSize - _allocatedCount + _deallocatedCount; }
    double GetUtilization() const { return static_cast<double>(_allocatedCount - _deallocatedCount) / PoolSize; }

private:
    void RecordPoolAllocation();
    void RecordPoolDeallocation();

    T* _pool;
    MemoryCategory _category;
    uint32_t _botGuid;
    size_t _nextFree;
    ::std::atomic<size_t> _allocatedCount{0};
    ::std::atomic<size_t> _deallocatedCount{0};
    mutable ::std::recursive_mutex _mutex;
};

// Main memory management system
class TC_GAME_API BotMemoryManager
{
public:
    static BotMemoryManager& Instance()
    {
        static BotMemoryManager instance;
        return instance;
    }

    // Initialization and shutdown
    bool Initialize();
    void Shutdown();
    bool IsEnabled() const { return _enabled.load(); }

    // Bot registration
    void RegisterBot(uint32_t botGuid);
    void UnregisterBot(uint32_t botGuid);

    // Memory tracking
    void RecordAllocation(void* address, uint64_t size, MemoryCategory category, uint32_t botGuid, const ::std::string& context = "");
    void RecordDeallocation(void* address, uint64_t size, MemoryCategory category, uint32_t botGuid);

    // Memory optimization
    void OptimizeBotMemory(uint32_t botGuid);
    void PerformGarbageCollection();
    void CompactMemory();
    uint64_t ReclaimUnusedMemory();

    // Memory analysis
    BotMemoryProfile GetBotMemoryProfile(uint32_t botGuid) const;
    SystemMemoryAnalytics GetSystemAnalytics() const;
    ::std::vector<uint32_t> GetHighMemoryUsageBots(uint32_t count = 10) const;

    // Leak detection
    void DetectMemoryLeaks();
    ::std::vector<MemoryLeakEntry> GetSuspectedLeaks() const;
    void ReportMemoryLeaks() const;

    // Cache management
    void FlushCache(MemoryCategory category);
    void FlushBotCache(uint32_t botGuid);
    void OptimizeCacheSize();

    // Memory pressure handling
    void HandleMemoryPressure();
    bool IsMemoryPressureHigh() const;
    void SetMemoryPressureThreshold(double threshold) { _memoryPressureThreshold = threshold; }

    // Configuration
    void SetEnabled(bool enabled) { _enabled.store(enabled); }
    void SetOptimizationEnabled(bool enabled) { _optimizationEnabled.store(enabled); }
    void SetLeakDetectionEnabled(bool enabled) { _leakDetectionEnabled.store(enabled); }

    // Reporting
    void GenerateMemoryReport(::std::string& report, uint32_t botGuid = 0) const;
    void GenerateLeakReport(::std::string& report) const;
    void GenerateOptimizationReport(::std::string& report) const;

    // Memory allocation helpers
    template<typename T>
    TrackedPtr<T> AllocateTracked(MemoryCategory category, uint32_t botGuid, const ::std::string& context = "")
    {
        T* ptr = new T();
        RecordAllocation(ptr, sizeof(T), category, botGuid, context);
        return TrackedPtr<T>(ptr, category, botGuid);
    }

    template<typename T>
    ::std::shared_ptr<MemoryPool<T>> GetPool(MemoryCategory category, uint32_t botGuid)
    {
        ::std::lock_guard lock(_poolsMutex);

        auto key = ::std::make_pair(static_cast<uint8_t>(category), botGuid);
        auto it = _memoryPools.find(key);

        if (it == _memoryPools.end())
        {
            auto pool = ::std::make_shared<MemoryPool<T>>(category, botGuid);
            _memoryPools[key] = pool;
            return pool;
        }

        return ::std::static_pointer_cast<MemoryPool<T>>(it->second);
    }

    // Statistics and monitoring
    uint64_t GetTotalMemoryUsage() const;
    uint64_t GetBotMemoryUsage(uint32_t botGuid) const;
    double GetMemoryEfficiency(uint32_t botGuid) const;
    uint32_t GetActiveAllocations() const;

private:
    BotMemoryManager() = default;
    ~BotMemoryManager() = default;

    // Internal optimization methods
    void OptimizeMemoryLayout();
    void ConsolidateFragmentedMemory();
    void PerformMemoryDefragmentation();

    // Background processing
    void PerformMemoryMaintenance();
    void UpdateMemoryStatistics();
    void MonitorMemoryUsage();

    // Leak detection implementation
    void ScanForLeaks();
    void AnalyzeAllocationPatterns();
    void ValidatePointers();

    // System integration
    void UpdateSystemMemoryInfo();
    void HandleOutOfMemory();
    void EmergencyMemoryCleanup();

    // Configuration
    ::std::atomic<bool> _enabled{false};
    ::std::atomic<bool> _optimizationEnabled{true};
    ::std::atomic<bool> _leakDetectionEnabled{true};
    ::std::atomic<bool> _shutdownRequested{false};

    // Memory tracking
    mutable ::std::recursive_mutex _profilesMutex;
    ::std::unordered_map<uint32_t, BotMemoryProfile> _botProfiles;

    mutable ::std::recursive_mutex _allocationsMutex;
    ::std::unordered_map<void*, MemoryLeakEntry> _activeAllocations;

    // System analytics
    mutable ::std::recursive_mutex _systemAnalyticsMutex;
    SystemMemoryAnalytics _systemAnalytics;

    // Memory pools
    mutable ::std::recursive_mutex _poolsMutex;
    ::std::unordered_map<::std::pair<uint8_t, uint32_t>, ::std::shared_ptr<void>,
                      ::std::hash<::std::pair<uint8_t, uint32_t>>> _memoryPools;

    // Background processing
    ::std::thread _maintenanceThread;
    ::std::condition_variable _maintenanceCondition;
    ::std::recursive_mutex _maintenanceMutex;

    // Configuration
    ::std::atomic<double> _memoryPressureThreshold{0.8}; // 80% usage
    ::std::atomic<uint64_t> _maxBotMemoryUsage{104857600}; // 100MB per bot
    ::std::atomic<uint64_t> _garbageCollectionInterval{300000000}; // 5 minutes
    ::std::atomic<uint64_t> _leakDetectionInterval{600000000}; // 10 minutes

    // Performance tracking
    ::std::atomic<uint64_t> _totalOptimizations{0};
    ::std::atomic<uint64_t> _totalMemoryReclaimed{0};
    ::std::atomic<uint64_t> _lastOptimization{0};
    ::std::atomic<uint64_t> _lastLeakDetection{0};

    // Constants
    static constexpr uint64_t DEFAULT_MAINTENANCE_INTERVAL_US = 60000000; // 60 seconds
    static constexpr uint64_t EMERGENCY_CLEANUP_THRESHOLD = 0x40000000; // 1GB
    static constexpr uint32_t MAX_LEAK_ENTRIES = 10000;
    static constexpr double FRAGMENTATION_THRESHOLD = 0.3; // 30%
};

// Convenience macros for memory management
#define sMemoryManager BotMemoryManager::Instance()

#define ALLOCATE_TRACKED(Type, category, botGuid, context) \
    sMemoryManager.AllocateTracked<Type>(category, botGuid, context)

#define RECORD_ALLOCATION(ptr, size, category, botGuid, context) \
    if (sMemoryManager.IsEnabled()) \
        sMemoryManager.RecordAllocation(ptr, size, category, botGuid, context)

#define RECORD_DEALLOCATION(ptr, size, category, botGuid) \
    if (sMemoryManager.IsEnabled()) \
        sMemoryManager.RecordDeallocation(ptr, size, category, botGuid)

#define GET_MEMORY_POOL(Type, category, botGuid) \
    sMemoryManager.GetPool<Type>(category, botGuid)

} // namespace Playerbot