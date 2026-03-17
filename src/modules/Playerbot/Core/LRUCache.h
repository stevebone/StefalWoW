/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_LRU_CACHE_H
#define PLAYERBOT_LRU_CACHE_H

#include "Define.h"
#include <list>
#include <unordered_map>
#include <optional>
#include <mutex>
#include <shared_mutex>
#include <functional>
#include <chrono>
#include <atomic>

namespace Playerbot
{

/**
 * @brief Enterprise-grade thread-safe LRU (Least Recently Used) cache
 *
 * DESIGN GOALS:
 * - O(1) lookup, insert, and eviction
 * - Thread-safe with reader/writer locks for high concurrency
 * - TTL (Time-To-Live) support for automatic expiration
 * - Memory usage tracking and statistics
 * - Configurable eviction callback
 * - Move semantics for efficient value handling
 *
 * MEMORY MANAGEMENT:
 * - Capacity-based eviction: Oldest entries removed when full
 * - TTL-based expiration: Stale entries removed on access
 * - Bulk cleanup: RemoveExpired() for periodic maintenance
 *
 * THREAD SAFETY:
 * - Multiple readers can access simultaneously (shared_lock)
 * - Writers get exclusive access (unique_lock)
 * - Safe for concurrent get/put from multiple threads
 *
 * USAGE EXAMPLE:
 * @code
 *   LRUCache<std::string, PathResult> pathCache(1000, 5min);
 *   pathCache.Put("key", PathResult{...});
 *   auto result = pathCache.Get("key");  // Returns std::optional
 * @endcode
 *
 * @tparam Key The key type (must be hashable and equality comparable)
 * @tparam Value The cached value type (should be movable for efficiency)
 * @tparam Hash Custom hash function (defaults to std::hash<Key>)
 */
template<typename Key, typename Value, typename Hash = ::std::hash<Key>>
class LRUCache
{
public:
    using TimePoint = ::std::chrono::steady_clock::time_point;
    using Duration = ::std::chrono::milliseconds;
    using EvictionCallback = ::std::function<void(Key const&, Value&)>;

    // Cache entry with metadata
    struct CacheEntry
    {
        Key key;
        Value value;
        TimePoint insertTime;
        TimePoint lastAccessTime;
        size_t accessCount{0};

        // Estimated memory usage (can be overridden via specialization)
        size_t GetMemoryBytes() const
        {
            return sizeof(CacheEntry) + sizeof(Key) + sizeof(Value);
        }
    };

    // Cache statistics
    struct Statistics
    {
        uint64_t hits{0};
        uint64_t misses{0};
        uint64_t evictions{0};
        uint64_t expirations{0};
        uint64_t insertions{0};
        size_t currentSize{0};
        size_t maxCapacity{0};
        size_t estimatedMemoryBytes{0};

        float GetHitRate() const
        {
            uint64_t total = hits + misses;
            return total > 0 ? static_cast<float>(hits) / total : 0.0f;
        }

        float GetMemoryUsageMB() const
        {
            return static_cast<float>(estimatedMemoryBytes) / (1024.0f * 1024.0f);
        }
    };

    /**
     * @brief Construct an LRU cache with specified capacity and TTL
     *
     * @param maxCapacity Maximum number of entries (0 = unlimited, not recommended)
     * @param ttl Time-to-live for entries (0 = no expiration)
     */
    explicit LRUCache(
        size_t maxCapacity = 1000,
        Duration ttl = Duration::zero())
        : _maxCapacity(maxCapacity)
        , _ttl(ttl)
    {
    }

    ~LRUCache() = default;

    // Non-copyable (contains mutex)
    LRUCache(LRUCache const&) = delete;
    LRUCache& operator=(LRUCache const&) = delete;

    // Movable
    LRUCache(LRUCache&&) = default;
    LRUCache& operator=(LRUCache&&) = default;

    /**
     * @brief Get a value from the cache
     *
     * Returns std::nullopt if key not found or entry expired.
     * Updates access time and moves entry to front (most recently used).
     *
     * @param key The key to lookup
     * @return Optional containing value if found and valid
     */
    ::std::optional<Value> Get(Key const& key)
    {
        ::std::unique_lock lock(_mutex);

        auto mapIt = _cacheMap.find(key);
        if (mapIt == _cacheMap.end())
        {
            ++_misses;
            return ::std::nullopt;
        }

        auto listIt = mapIt->second;

        // Check TTL expiration
        if (_ttl.count() > 0)
        {
            auto now = ::std::chrono::steady_clock::now();
            auto age = ::std::chrono::duration_cast<Duration>(now - listIt->insertTime);
            if (age > _ttl)
            {
                // Entry expired - remove it
                _cacheMap.erase(mapIt);
                _cacheList.erase(listIt);
                ++_misses;
                ++_expirations;
                return ::std::nullopt;
            }
        }

        // Update access metadata
        listIt->lastAccessTime = ::std::chrono::steady_clock::now();
        ++listIt->accessCount;

        // Move to front (most recently used)
        if (listIt != _cacheList.begin())
        {
            _cacheList.splice(_cacheList.begin(), _cacheList, listIt);
        }

        ++_hits;
        return listIt->value;
    }

    /**
     * @brief Get a value without modifying access order (peek)
     *
     * Useful for checking existence without affecting LRU order.
     *
     * @param key The key to lookup
     * @return Optional containing value if found and valid
     */
    ::std::optional<Value> Peek(Key const& key) const
    {
        ::std::shared_lock lock(_mutex);

        auto mapIt = _cacheMap.find(key);
        if (mapIt == _cacheMap.end())
        {
            return ::std::nullopt;
        }

        auto listIt = mapIt->second;

        // Check TTL expiration (but don't remove - const method)
        if (_ttl.count() > 0)
        {
            auto now = ::std::chrono::steady_clock::now();
            auto age = ::std::chrono::duration_cast<Duration>(now - listIt->insertTime);
            if (age > _ttl)
            {
                return ::std::nullopt;
            }
        }

        return listIt->value;
    }

    /**
     * @brief Insert or update a value in the cache
     *
     * If key exists, updates value and moves to front.
     * If cache is full, evicts least recently used entry.
     *
     * @param key The key
     * @param value The value (will be moved)
     */
    void Put(Key const& key, Value value)
    {
        ::std::unique_lock lock(_mutex);

        auto mapIt = _cacheMap.find(key);
        if (mapIt != _cacheMap.end())
        {
            // Key exists - update value and move to front
            auto listIt = mapIt->second;
            listIt->value = ::std::move(value);
            listIt->lastAccessTime = ::std::chrono::steady_clock::now();
            ++listIt->accessCount;

            if (listIt != _cacheList.begin())
            {
                _cacheList.splice(_cacheList.begin(), _cacheList, listIt);
            }
            return;
        }

        // Evict if at capacity
        while (_maxCapacity > 0 && _cacheList.size() >= _maxCapacity)
        {
            EvictLRU();
        }

        // Insert new entry at front
        auto now = ::std::chrono::steady_clock::now();
        _cacheList.emplace_front(CacheEntry{key, ::std::move(value), now, now, 1});
        _cacheMap[key] = _cacheList.begin();
        ++_insertions;
    }

    /**
     * @brief Remove an entry from the cache
     *
     * @param key The key to remove
     * @return true if entry was removed, false if not found
     */
    bool Remove(Key const& key)
    {
        ::std::unique_lock lock(_mutex);

        auto mapIt = _cacheMap.find(key);
        if (mapIt == _cacheMap.end())
        {
            return false;
        }

        auto listIt = mapIt->second;

        // Call eviction callback if set
        if (_evictionCallback)
        {
            _evictionCallback(listIt->key, listIt->value);
        }

        _cacheList.erase(listIt);
        _cacheMap.erase(mapIt);
        return true;
    }

    /**
     * @brief Remove all expired entries
     *
     * Should be called periodically for maintenance.
     *
     * @return Number of entries removed
     */
    size_t RemoveExpired()
    {
        if (_ttl.count() == 0)
            return 0;

        ::std::unique_lock lock(_mutex);

        auto now = ::std::chrono::steady_clock::now();
        size_t removed = 0;

        // Iterate from back (oldest) to front
        auto it = _cacheList.rbegin();
        while (it != _cacheList.rend())
        {
            auto age = ::std::chrono::duration_cast<Duration>(now - it->insertTime);
            if (age <= _ttl)
            {
                // Entries are ordered by access time, so once we find
                // a non-expired entry, older entries might still be valid
                // (if accessed recently). Continue checking.
                ++it;
                continue;
            }

            // Entry expired
            auto forwardIt = ::std::next(it).base();  // Convert to forward iterator
            --forwardIt;  // Point to the element

            // Remove from map
            _cacheMap.erase(forwardIt->key);

            // Advance reverse iterator before erasing
            ++it;

            // Erase from list
            _cacheList.erase(forwardIt);

            ++removed;
            ++_expirations;
        }

        return removed;
    }

    /**
     * @brief Clear all entries from the cache
     */
    void Clear()
    {
        ::std::unique_lock lock(_mutex);

        if (_evictionCallback)
        {
            for (auto& entry : _cacheList)
            {
                _evictionCallback(entry.key, entry.value);
            }
        }

        _cacheList.clear();
        _cacheMap.clear();
    }

    /**
     * @brief Check if cache contains a key
     */
    bool Contains(Key const& key) const
    {
        ::std::shared_lock lock(_mutex);
        return _cacheMap.find(key) != _cacheMap.end();
    }

    /**
     * @brief Get current size of the cache
     */
    size_t Size() const
    {
        ::std::shared_lock lock(_mutex);
        return _cacheList.size();
    }

    /**
     * @brief Check if cache is empty
     */
    bool Empty() const
    {
        ::std::shared_lock lock(_mutex);
        return _cacheList.empty();
    }

    /**
     * @brief Get cache statistics
     */
    Statistics GetStatistics() const
    {
        ::std::shared_lock lock(_mutex);

        Statistics stats;
        stats.hits = _hits.load(::std::memory_order_relaxed);
        stats.misses = _misses.load(::std::memory_order_relaxed);
        stats.evictions = _evictions.load(::std::memory_order_relaxed);
        stats.expirations = _expirations.load(::std::memory_order_relaxed);
        stats.insertions = _insertions.load(::std::memory_order_relaxed);
        stats.currentSize = _cacheList.size();
        stats.maxCapacity = _maxCapacity;

        // Estimate memory usage
        stats.estimatedMemoryBytes = sizeof(LRUCache);
        stats.estimatedMemoryBytes += _cacheList.size() * sizeof(CacheEntry);
        stats.estimatedMemoryBytes += _cacheMap.size() * (sizeof(Key) + sizeof(void*) + 32);  // Hash map overhead

        return stats;
    }

    /**
     * @brief Set callback for when entries are evicted
     */
    void SetEvictionCallback(EvictionCallback callback)
    {
        ::std::unique_lock lock(_mutex);
        _evictionCallback = ::std::move(callback);
    }

    /**
     * @brief Update maximum capacity (may trigger evictions)
     */
    void SetMaxCapacity(size_t newCapacity)
    {
        ::std::unique_lock lock(_mutex);
        _maxCapacity = newCapacity;

        // Evict excess entries
        while (_maxCapacity > 0 && _cacheList.size() > _maxCapacity)
        {
            EvictLRU();
        }
    }

    /**
     * @brief Update TTL for new entries
     */
    void SetTTL(Duration newTTL)
    {
        ::std::unique_lock lock(_mutex);
        _ttl = newTTL;
    }

    /**
     * @brief Get current capacity setting
     */
    size_t GetMaxCapacity() const { return _maxCapacity; }

    /**
     * @brief Get current TTL setting
     */
    Duration GetTTL() const { return _ttl; }

private:
    /**
     * @brief Evict the least recently used entry (must hold lock)
     */
    void EvictLRU()
    {
        if (_cacheList.empty())
            return;

        auto& entry = _cacheList.back();

        // Call eviction callback
        if (_evictionCallback)
        {
            _evictionCallback(entry.key, entry.value);
        }

        _cacheMap.erase(entry.key);
        _cacheList.pop_back();
        ++_evictions;
    }

    // Data structures
    ::std::list<CacheEntry> _cacheList;  // Ordered by access time (front = MRU, back = LRU)
    ::std::unordered_map<Key, typename ::std::list<CacheEntry>::iterator, Hash> _cacheMap;

    // Configuration
    size_t _maxCapacity;
    Duration _ttl;
    EvictionCallback _evictionCallback;

    // Thread safety
    mutable ::std::shared_mutex _mutex;

    // Statistics (atomic for thread-safe reads without lock)
    mutable ::std::atomic<uint64_t> _hits{0};
    mutable ::std::atomic<uint64_t> _misses{0};
    mutable ::std::atomic<uint64_t> _evictions{0};
    mutable ::std::atomic<uint64_t> _expirations{0};
    mutable ::std::atomic<uint64_t> _insertions{0};
};

/**
 * @brief Bounded history container with automatic eviction
 *
 * Simple wrapper around std::vector with maximum size enforcement.
 * When full, oldest entries are automatically removed.
 *
 * USAGE:
 * @code
 *   BoundedHistory<DamageEntry> history(100);
 *   history.Push(DamageEntry{...});  // Removes oldest if at capacity
 * @endcode
 *
 * @tparam T The element type
 */
template<typename T>
class BoundedHistory
{
public:
    explicit BoundedHistory(size_t maxSize = 100)
        : _maxSize(maxSize)
    {
        _data.reserve(::std::min(maxSize, size_t(100)));  // Don't over-allocate
    }

    void Push(T value)
    {
        if (_data.size() >= _maxSize && _maxSize > 0)
        {
            // Remove oldest (front)
            _data.erase(_data.begin());
        }
        _data.push_back(::std::move(value));
    }

    void Clear() { _data.clear(); }
    size_t Size() const { return _data.size(); }
    bool Empty() const { return _data.empty(); }
    size_t MaxSize() const { return _maxSize; }

    void SetMaxSize(size_t newMax)
    {
        _maxSize = newMax;
        // Trim if needed
        while (_data.size() > _maxSize)
        {
            _data.erase(_data.begin());
        }
    }

    // Iterators
    auto begin() { return _data.begin(); }
    auto end() { return _data.end(); }
    auto begin() const { return _data.begin(); }
    auto end() const { return _data.end(); }
    auto rbegin() { return _data.rbegin(); }
    auto rend() { return _data.rend(); }
    auto rbegin() const { return _data.rbegin(); }
    auto rend() const { return _data.rend(); }

    // Access
    T& operator[](size_t index) { return _data[index]; }
    T const& operator[](size_t index) const { return _data[index]; }
    T& Back() { return _data.back(); }
    T const& Back() const { return _data.back(); }
    T& Front() { return _data.front(); }
    T const& Front() const { return _data.front(); }

    // Underlying data
    ::std::vector<T>& Data() { return _data; }
    ::std::vector<T> const& Data() const { return _data; }

private:
    ::std::vector<T> _data;
    size_t _maxSize;
};

/**
 * @brief Bounded map with LRU-based eviction (simplified version)
 *
 * Simplified version of LRUCache for cases where full LRU semantics
 * aren't needed but size limiting is required.
 *
 * @tparam K Key type
 * @tparam V Value type
 */
template<typename K, typename V>
class BoundedMap
{
public:
    explicit BoundedMap(size_t maxSize = 1000)
        : _maxSize(maxSize)
    {
    }

    void Insert(K key, V value)
    {
        // If at capacity and key doesn't exist, remove a random entry
        if (_data.size() >= _maxSize && _data.find(key) == _data.end())
        {
            // Simple eviction: remove first entry (not true LRU but O(1))
            if (!_data.empty())
            {
                _data.erase(_data.begin());
            }
        }
        _data[::std::move(key)] = ::std::move(value);
    }

    ::std::optional<V> Get(K const& key) const
    {
        auto it = _data.find(key);
        if (it != _data.end())
            return it->second;
        return ::std::nullopt;
    }

    bool Contains(K const& key) const
    {
        return _data.find(key) != _data.end();
    }

    void Remove(K const& key)
    {
        _data.erase(key);
    }

    void Clear() { _data.clear(); }
    size_t Size() const { return _data.size(); }
    bool Empty() const { return _data.empty(); }

    V& operator[](K const& key) { return _data[key]; }

    // Iterators
    auto begin() { return _data.begin(); }
    auto end() { return _data.end(); }
    auto begin() const { return _data.begin(); }
    auto end() const { return _data.end(); }

private:
    ::std::unordered_map<K, V> _data;
    size_t _maxSize;
};

} // namespace Playerbot

#endif // PLAYERBOT_LRU_CACHE_H
