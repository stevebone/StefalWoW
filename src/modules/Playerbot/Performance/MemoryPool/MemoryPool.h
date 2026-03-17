/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * Phase 5: Performance Optimization - MemoryPool System
 *
 * Production-grade memory pool for bot object allocation
 * - Fixed-size block allocation with minimal fragmentation
 * - Thread-local caching for lock-free allocations
 * - Automatic defragmentation during low-activity periods
 * - Comprehensive memory tracking per bot
 *
 * Performance Targets:
 * - <100ns allocation latency (thread-local cache hit)
 * - <1% memory fragmentation
 * - >95% thread-local cache hit rate
 * - Zero memory leaks over continuous operation
 */

#ifndef PLAYERBOT_MEMORYPOOL_H
#define PLAYERBOT_MEMORYPOOL_H

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include <memory>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <array>
#include <unordered_map>
#include <chrono>
#include "ObjectGuid.h"

namespace Playerbot {
namespace Performance {

/**
 * @brief Thread-safe memory pool with thread-local caching
 *
 * Template parameter T must be the type to allocate
 * Uses fixed-size blocks for predictable performance
 *
 * Thread-local cache provides lock-free fast path
 * Falls back to shared free list on cache miss
 */
template<typename T>
class alignas(64) MemoryPool
{
public:
    struct Configuration
    {
        size_t initialCapacity{1000};
        size_t maxCapacity{10000};
        size_t chunkSize{100};           // Blocks per chunk allocation
        bool enableThreadCache{true};
        bool enableDefragmentation{false}; // Disabled by default for simplicity
        ::std::chrono::seconds defragInterval{60};
    };

private:
    struct alignas(alignof(T)) Block
    {
        union {
            T object;
            Block* next;
        };

        Block() : next(nullptr) {}
        ~Block() {}
    };

    struct Chunk
    {
        ::std::unique_ptr<Block[]> blocks;
        size_t capacity;
        Chunk* next{nullptr};

        Chunk(size_t cap) : blocks(::std::make_unique<Block[]>(cap)), capacity(cap) {}
    };

    // Thread-local cache for lock-free allocations
    struct ThreadCache
    {
        static constexpr size_t CACHE_SIZE = 32;
        ::std::array<Block*, CACHE_SIZE> cache{};
        size_t count{0};

        Block* TryPop()
        {
            if (count > 0)
                return cache[--count];
            return nullptr;
        }

        bool TryPush(Block* block)
        {
            if (count < CACHE_SIZE)
            {
                cache[count++] = block;
                return true;
            }
            return false;
        }
    };

    // Pool state
    alignas(64) ::std::atomic<Block*> _freeList{nullptr};
    alignas(64) ::std::atomic<Chunk*> _chunks{nullptr};
    alignas(64) ::std::atomic<size_t> _totalCapacity{0};
    alignas(64) ::std::atomic<size_t> _usedBlocks{0};
    alignas(64) ::std::atomic<size_t> _peakUsage{0};

    Configuration _config;
    Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BEHAVIOR_MANAGER> _chunkMutex;

    // Thread-local cache
    static thread_local ThreadCache _threadCache;

public:
    explicit MemoryPool(Configuration config = {});
    ~MemoryPool();

    /**
     * @brief Allocate object with perfect forwarding
     * @param args Arguments to forward to T's constructor
     * @return Pointer to constructed object
     */
    template<typename... Args>
    T* Allocate(Args&&... args);

    /**
     * @brief Deallocate object
     * @param ptr Pointer to object to deallocate
     */
    void Deallocate(T* ptr);

    /**
     * @brief Get current usage metrics
     */
    size_t GetUsedBlocks() const { return _usedBlocks.load(::std::memory_order_relaxed); }
    size_t GetFreeBlocks() const { return _totalCapacity.load() - _usedBlocks.load(); }
    size_t GetPeakUsage() const { return _peakUsage.load(::std::memory_order_relaxed); }
    size_t GetMemoryUsage() const { return _totalCapacity.load() * sizeof(Block); }

private:
    void AllocateChunk();
    Block* AllocateFromFreeList();
    void ReturnToFreeList(Block* block);
};

/**
 * @brief Global bot memory manager
 *
 * Centralized memory management for all bot-related allocations
 * Tracks memory usage per bot and globally
 */
class BotMemoryManager
{
private:
    struct BotMemoryUsage
    {
        ::std::atomic<size_t> totalMemory{0};
        ::std::chrono::steady_clock::time_point lastUpdate;
    };

    ::std::unordered_map<ObjectGuid, BotMemoryUsage> _botMemoryUsage;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BEHAVIOR_MANAGER> _usageMutex;

    ::std::atomic<size_t> _totalAllocated{0};
    ::std::atomic<size_t> _maxMemory{1ULL << 30}; // 1GB default

public:
    static BotMemoryManager& Instance();

    /**
     * @brief Track memory allocation for a bot
     */
    void TrackAllocation(ObjectGuid guid, size_t size);

    /**
     * @brief Track memory deallocation for a bot
     */
    void TrackDeallocation(ObjectGuid guid, size_t size);

    /**
     * @brief Check if under memory pressure
     */
    bool IsUnderMemoryPressure() const
    {
        return _totalAllocated.load() > _maxMemory.load() * 0.9;
    }

    /**
     * @brief Get total allocated memory
     */
    size_t GetTotalAllocated() const
    {
        return _totalAllocated.load(::std::memory_order_relaxed);
    }

    /**
     * @brief Set maximum memory limit
     */
    void SetMaxMemory(size_t bytes) { _maxMemory.store(bytes); }
};

} // namespace Performance
} // namespace Playerbot

#endif // PLAYERBOT_MEMORYPOOL_H
