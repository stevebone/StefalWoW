/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRINITY_PATHCACHE_H
#define TRINITY_PATHCACHE_H

#include "Define.h"
#include "PathGenerator.h"
#include "MoveSplineInitArgs.h"
#include <list>
#include <unordered_map>
#include <mutex>
#include <optional>

struct CachedPath
{
    Movement::PointsArray points;
    PathType pathType = PATHFIND_BLANK;
    uint32 timestamp = 0;
    bool requiresSwimming = false;

    bool IsValid() const { return pathType != PATHFIND_BLANK && pathType != PATHFIND_NOPATH; }
};

struct PathCacheKey
{
    uint32 mapId = 0;
    float startX = 0.0f;
    float startY = 0.0f;
    float startZ = 0.0f;
    float endX = 0.0f;
    float endY = 0.0f;
    float endZ = 0.0f;

    bool operator==(PathCacheKey const& other) const;
};

struct PathCacheKeyHash
{
    std::size_t operator()(PathCacheKey const& key) const;
};

struct PathCacheMetrics
{
    uint64 hits = 0;
    uint64 misses = 0;
    uint64 insertions = 0;
    uint64 evictions = 0;
    uint32 currentSize = 0;

    float GetHitRate() const
    {
        uint64 total = hits + misses;
        return total > 0 ? static_cast<float>(hits) / total : 0.0f;
    }

    void Reset()
    {
        hits = 0;
        misses = 0;
        insertions = 0;
        evictions = 0;
    }
};

class TC_GAME_API PathCache
{
public:
    PathCache();
    explicit PathCache(uint32 maxSize, uint32 ttlSeconds);
    ~PathCache() = default;

    PathCache(PathCache const&) = delete;
    PathCache& operator=(PathCache const&) = delete;

    // Try to get a cached path
    std::optional<CachedPath> Get(PathCacheKey const& key);

    // Store a path in the cache
    void Put(PathCacheKey const& key, CachedPath const& path);

    // Store a path from PathGenerator
    void Put(uint32 mapId, Position const& start, Position const& end,
             Movement::PointsArray const& points, PathType pathType, bool requiresSwimming = false);

    // Try to get a cached path using positions
    std::optional<CachedPath> Get(uint32 mapId, Position const& start, Position const& end);

    // Clear all cached paths
    void Clear();

    // Clear expired entries
    void ClearExpired();

    // Configuration
    void SetMaxSize(uint32 size) { _maxSize = size; }
    uint32 GetMaxSize() const { return _maxSize; }

    void SetTTL(uint32 ttlSeconds) { _ttlMs = ttlSeconds * 1000; }
    uint32 GetTTL() const { return _ttlMs / 1000; }

    // Metrics
    PathCacheMetrics GetMetrics() const;
    void ResetMetrics();

    // Current size
    uint32 Size() const;

private:
    // LRU list - most recently used at front
    using CacheList = std::list<std::pair<PathCacheKey, CachedPath>>;
    using CacheMap = std::unordered_map<PathCacheKey, CacheList::iterator, PathCacheKeyHash>;

    CacheList _cacheList;
    CacheMap _cacheMap;
    mutable std::mutex _mutex;

    uint32 _maxSize;
    uint32 _ttlMs;

    PathCacheMetrics _metrics;

    // Quantize position for cache key (reduce precision for better cache hits)
    static float QuantizePosition(float value);

    // Create key from positions
    static PathCacheKey MakeKey(uint32 mapId, Position const& start, Position const& end);

    // Evict least recently used entry
    void EvictLRU();

    // Move entry to front of LRU list
    void MoveToFront(CacheMap::iterator it);

    // Check if entry is expired
    bool IsExpired(CachedPath const& path) const;
};

#endif // TRINITY_PATHCACHE_H
