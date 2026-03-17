/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_OBJECT_POOL_H
#define PLAYERBOT_OBJECT_POOL_H

#include <memory>
#include <vector>
#include <mutex>
#include <atomic>
#include "Log.h"
#include "Threading/LockHierarchy.h"

namespace Playerbot
{

/**
 * @brief Thread-safe object pool for reducing heap allocations in hot paths
 *
 * This pool uses a simple stack-based allocation strategy with mutex protection.
 * For maximum performance, objects are not destroyed until pool shutdown.
 *
 * Performance Targets:
 * - Allocation: O(1) with mutex lock (faster than malloc)
 * - Deallocation: O(1) with mutex lock
 * - Memory: Pre-allocated chunks reduce fragmentation
 *
 * @tparam T Object type to pool (must be default constructible)
 * @tparam ChunkSize Number of objects to allocate per chunk
 */
template<typename T, size_t ChunkSize = 64>
class ObjectPool
{
public:
    /**
     * @brief Construct object pool with initial capacity
     * @param initialCapacity Initial number of objects to pre-allocate
     */
    explicit ObjectPool(size_t initialCapacity = ChunkSize)
        : _totalAllocated(0)
        , _totalReused(0)
        , _currentPooled(0)
        , _peakPooled(0)
    {
        // Pre-allocate first chunk
        if (initialCapacity > 0)
        {
            AllocateChunk(initialCapacity);
        }
    }

    /**
     * @brief Destructor - releases all pooled objects
     */
    ~ObjectPool()
    {
        ::std::lock_guard lock(_poolMutex);

        // Log pool statistics before shutdown
        TC_LOG_INFO("module.playerbot.pool",
            "ObjectPool<{}> shutdown - Allocated: {}, Reused: {}, Peak: {}, Final: {}",
            typeid(T).name(),
            _totalAllocated.load(),
            _totalReused.load(),
            _peakPooled.load(),
            _currentPooled.load());

        // Clear all pooled objects
        _pool.clear();
        _chunks.clear();
    }

    /**
     * @brief Acquire object from pool (or allocate new if pool empty)
     * @return Unique pointer with custom deleter that returns to pool
     */
    ::std::unique_ptr<T, ::std::function<void(T*)>> Acquire()
    {
        ::std::lock_guard lock(_poolMutex);

        T* obj = nullptr;

        if (!_pool.empty())
        {
            // Reuse from pool
            obj = _pool.back();
            _pool.pop_back();
            _currentPooled.fetch_sub(1, ::std::memory_order_relaxed);
            _totalReused.fetch_add(1, ::std::memory_order_relaxed);
        }
        else
        {
            // Allocate new object
            obj = AllocateObject();
            _totalAllocated.fetch_add(1, ::std::memory_order_relaxed);
        }

        // Return with custom deleter that returns to pool
        return ::std::unique_ptr<T, ::std::function<void(T*)>>(
            obj,
            [this](T* ptr) { this->Release(ptr); }
        );
    }

    /**
     * @brief Get pool statistics
     */
    struct Stats
    {
        uint64_t totalAllocated;
        uint64_t totalReused;
        uint32_t currentPooled;
        uint32_t peakPooled;
        float reuseRate; // Percentage of reused vs total acquisitions
    };

    Stats GetStats() const
    {
        Stats stats;
        stats.totalAllocated = _totalAllocated.load(::std::memory_order_relaxed);
        stats.totalReused = _totalReused.load(::std::memory_order_relaxed);
        stats.currentPooled = _currentPooled.load(::std::memory_order_relaxed);
        stats.peakPooled = _peakPooled.load(::std::memory_order_relaxed);

        uint64_t total = stats.totalAllocated + stats.totalReused;
        stats.reuseRate = total > 0 ? (float(stats.totalReused) / float(total) * 100.0f) : 0.0f;

        return stats;
    }

    /**
     * @brief Preallocate objects to avoid allocation during runtime
     * @param count Number of objects to preallocate
     */
    void Reserve(size_t count)
    {
        ::std::lock_guard lock(_poolMutex);

        size_t needed = count > _pool.size() ? (count - _pool.size()) : 0;
        if (needed > 0)
        {
            AllocateChunk(needed);
        }
    }

    /**
     * @brief Shrink pool to release unused memory
     * @param targetSize Target pool size (0 = release all)
     */
    void Shrink(size_t targetSize = 0)
    {
        ::std::lock_guard lock(_poolMutex);

        while (_pool.size() > targetSize && !_pool.empty())
        {
            _pool.pop_back();
            _currentPooled.fetch_sub(1, ::std::memory_order_relaxed);
        }
    }

private:
    /**
     * @brief Release object back to pool
     * @param obj Object to return
     */
    void Release(T* obj)
    {
        if (!obj)
            return;

        ::std::lock_guard lock(_poolMutex);

        // Reset object state (placement new with default constructor)
        obj->~T();
        new (obj) T();

        // Return to pool
        _pool.push_back(obj);

        uint32_t current = _currentPooled.fetch_add(1, ::std::memory_order_relaxed) + 1;

        // Update peak
        uint32_t peak = _peakPooled.load(::std::memory_order_relaxed);
        while (current > peak && !_peakPooled.compare_exchange_weak(peak, current, ::std::memory_order_relaxed)) {}
    }

    /**
     * @brief Allocate new object from chunk
     * @return Raw pointer to new object
     */
    T* AllocateObject()
    {
        // Allocate new chunk
        auto chunk = ::std::make_unique<::std::vector<T>>(1);
        T* obj = &(*chunk)[0];
        _chunks.push_back(::std::move(chunk));
        return obj;
    }

    /**
     * @brief Allocate chunk of objects
     * @param count Number of objects in chunk
     */
    void AllocateChunk(size_t count)
    {
        auto chunk = ::std::make_unique<::std::vector<T>>(count);

        for (size_t i = 0; i < count; ++i)
        {
            _pool.push_back(&(*chunk)[i]);
            _currentPooled.fetch_add(1, ::std::memory_order_relaxed);
        }

        _chunks.push_back(::std::move(chunk));
    }

    Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BEHAVIOR_MANAGER> _poolMutex;                              // Pool access synchronization
    ::std::vector<T*> _pool;                              // Available objects
    ::std::vector<::std::unique_ptr<::std::vector<T>>> _chunks; // Allocated memory chunks

    ::std::atomic<uint64_t> _totalAllocated;              // Total new allocations
    ::std::atomic<uint64_t> _totalReused;                 // Total pool reuses
    ::std::atomic<uint32_t> _currentPooled;               // Current objects in pool
    ::std::atomic<uint32_t> _peakPooled;                  // Peak pool size
};

} // namespace Playerbot

#endif // PLAYERBOT_OBJECT_POOL_H
