/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * Phase 5: Performance Optimization - MemoryPool Implementation
 */

#include "MemoryPool.h"
#include "Log.h"
#include <algorithm>

namespace Playerbot {
namespace Performance {

// Thread-local cache definition
template<typename T>
thread_local typename MemoryPool<T>::ThreadCache MemoryPool<T>::_threadCache;

template<typename T>
MemoryPool<T>::MemoryPool(Configuration config)
    : _config(config)
{
    // Allocate initial capacity
    AllocateChunk();
}

template<typename T>
MemoryPool<T>::~MemoryPool()
{
    // Clean up chunks
    Chunk* current = _chunks.load();
    while (current)
    {
        Chunk* next = current->next;
        delete current;
        current = next;
    }
}

template<typename T>
template<typename... Args>
T* MemoryPool<T>::Allocate(Args&&... args)
{
    Block* block = nullptr;

    // Try thread-local cache first (lock-free fast path)
    if (_config.enableThreadCache)
    {
        block = _threadCache.TryPop();
    }

    // Fall back to shared free list
    if (!block)
    {
        block = AllocateFromFreeList();
    }

    // Allocate new chunk if needed
    if (!block)
    {
        AllocateChunk();
        block = AllocateFromFreeList();
    }

    if (!block)
    {
        throw ::std::bad_alloc();
    }

    // Construct object
    new (&block->object) T(::std::forward<Args>(args)...);

    // Update metrics
    size_t used = _usedBlocks.fetch_add(1, ::std::memory_order_relaxed) + 1;
    size_t peak = _peakUsage.load(::std::memory_order_relaxed);
    while (used > peak && !_peakUsage.compare_exchange_weak(peak, used)) {}

    return &block->object;
}

template<typename T>
void MemoryPool<T>::Deallocate(T* ptr)
{
    if (!ptr)
        return;

    // Destruct object
    ptr->~T();

    // Get block
    Block* block = reinterpret_cast<Block*>(ptr);

    // Try thread-local cache first
    if (_config.enableThreadCache && _threadCache.TryPush(block))
    {
        _usedBlocks.fetch_sub(1, ::std::memory_order_relaxed);
        return;
    }

    // Return to shared free list
    ReturnToFreeList(block);
    _usedBlocks.fetch_sub(1, ::std::memory_order_relaxed);
}

template<typename T>
void MemoryPool<T>::AllocateChunk()
{
    ::std::lock_guard lock(_chunkMutex);

    size_t currentCapacity = _totalCapacity.load(::std::memory_order_relaxed);
    if (currentCapacity >= _config.maxCapacity)
    {
        return; // At capacity limit
    }

    size_t chunkSize = ::std::min(_config.chunkSize, _config.maxCapacity - currentCapacity);

    // Create new chunk
    Chunk* newChunk = new Chunk(chunkSize);

    // Add blocks to free list
    for (size_t i = 0; i < chunkSize; ++i)
    {
        ReturnToFreeList(&newChunk->blocks[i]);
    }

    // Add chunk to list
    Chunk* oldHead = _chunks.load(::std::memory_order_relaxed);
    newChunk->next = oldHead;
    _chunks.store(newChunk, ::std::memory_order_release);

    _totalCapacity.fetch_add(chunkSize, ::std::memory_order_relaxed);
}

template<typename T>
typename MemoryPool<T>::Block* MemoryPool<T>::AllocateFromFreeList()
{
    Block* head = _freeList.load(::std::memory_order_acquire);

    while (head)
    {
        Block* next = head->next;
        if (_freeList.compare_exchange_weak(head, next,
            ::std::memory_order_release,
            ::std::memory_order_acquire))
        {
            return head;
        }
    }

    return nullptr;
}

template<typename T>
void MemoryPool<T>::ReturnToFreeList(Block* block)
{
    Block* head = _freeList.load(::std::memory_order_relaxed);
    do
    {
        block->next = head;
    } while (!_freeList.compare_exchange_weak(head, block,
        ::std::memory_order_release,
        ::std::memory_order_relaxed));
}

// ============================================================================
// BotMemoryManager Implementation
// ============================================================================

BotMemoryManager& BotMemoryManager::Instance()
{
    static BotMemoryManager instance;
    return instance;
}

void BotMemoryManager::TrackAllocation(ObjectGuid guid, size_t size)
{
    _totalAllocated.fetch_add(size, ::std::memory_order_relaxed);

    ::std::lock_guard lock(_usageMutex);
    auto& usage = _botMemoryUsage[guid];
    usage.totalMemory.fetch_add(size, ::std::memory_order_relaxed);
    usage.lastUpdate = ::std::chrono::steady_clock::now();
}

void BotMemoryManager::TrackDeallocation(ObjectGuid guid, size_t size)
{
    _totalAllocated.fetch_sub(size, ::std::memory_order_relaxed);

    ::std::lock_guard lock(_usageMutex);
    auto it = _botMemoryUsage.find(guid);
    if (it != _botMemoryUsage.end())
    {
        it->second.totalMemory.fetch_sub(size, ::std::memory_order_relaxed);
        it->second.lastUpdate = ::std::chrono::steady_clock::now();
    }
}

} // namespace Performance
} // namespace Playerbot
