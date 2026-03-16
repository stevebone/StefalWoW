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

#include "PathCache.h"
#include "Position.h"
#include "GameTime.h"
#include <cmath>

bool PathCacheKey::operator==(PathCacheKey const& other) const
{
    return mapId == other.mapId &&
           startX == other.startX &&
           startY == other.startY &&
           startZ == other.startZ &&
           endX == other.endX &&
           endY == other.endY &&
           endZ == other.endZ;
}

std::size_t PathCacheKeyHash::operator()(PathCacheKey const& key) const
{
    std::size_t h = 0;
    h ^= std::hash<uint32>()(key.mapId) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= std::hash<float>()(key.startX) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= std::hash<float>()(key.startY) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= std::hash<float>()(key.startZ) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= std::hash<float>()(key.endX) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= std::hash<float>()(key.endY) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= std::hash<float>()(key.endZ) + 0x9e3779b9 + (h << 6) + (h >> 2);
    return h;
}

PathCache::PathCache()
    : _maxSize(1000)
    , _ttlMs(60000) // 60 seconds default
{
}

PathCache::PathCache(uint32 maxSize, uint32 ttlSeconds)
    : _maxSize(maxSize)
    , _ttlMs(ttlSeconds * 1000)
{
}

float PathCache::QuantizePosition(float value)
{
    // Quantize to 2 decimal places (0.01 yard precision)
    return std::floor(value * 100.0f) / 100.0f;
}

PathCacheKey PathCache::MakeKey(uint32 mapId, Position const& start, Position const& end)
{
    PathCacheKey key;
    key.mapId = mapId;
    key.startX = QuantizePosition(start.GetPositionX());
    key.startY = QuantizePosition(start.GetPositionY());
    key.startZ = QuantizePosition(start.GetPositionZ());
    key.endX = QuantizePosition(end.GetPositionX());
    key.endY = QuantizePosition(end.GetPositionY());
    key.endZ = QuantizePosition(end.GetPositionZ());
    return key;
}

std::optional<CachedPath> PathCache::Get(PathCacheKey const& key)
{
    std::lock_guard<std::mutex> lock(_mutex);

    auto it = _cacheMap.find(key);
    if (it == _cacheMap.end())
    {
        _metrics.misses++;
        return std::nullopt;
    }

    CachedPath& cached = it->second->second;

    // Check if expired
    if (IsExpired(cached))
    {
        _cacheList.erase(it->second);
        _cacheMap.erase(it);
        _metrics.evictions++;
        _metrics.currentSize = static_cast<uint32>(_cacheList.size());
        _metrics.misses++;
        return std::nullopt;
    }

    // Move to front of LRU list
    MoveToFront(it);

    _metrics.hits++;
    return cached;
}

std::optional<CachedPath> PathCache::Get(uint32 mapId, Position const& start, Position const& end)
{
    PathCacheKey key = MakeKey(mapId, start, end);
    return Get(key);
}

void PathCache::Put(PathCacheKey const& key, CachedPath const& path)
{
    std::lock_guard<std::mutex> lock(_mutex);

    // Check if key already exists
    auto it = _cacheMap.find(key);
    if (it != _cacheMap.end())
    {
        // Update existing entry and move to front
        it->second->second = path;
        MoveToFront(it);
        return;
    }

    // Evict if at capacity
    while (_cacheList.size() >= _maxSize)
    {
        EvictLRU();
    }

    // Insert new entry at front
    _cacheList.push_front({key, path});
    _cacheMap[key] = _cacheList.begin();

    _metrics.insertions++;
    _metrics.currentSize = static_cast<uint32>(_cacheList.size());
}

void PathCache::Put(uint32 mapId, Position const& start, Position const& end,
                    Movement::PointsArray const& points, PathType pathType, bool requiresSwimming)
{
    PathCacheKey key = MakeKey(mapId, start, end);

    CachedPath cached;
    cached.points = points;
    cached.pathType = pathType;
    cached.timestamp = GameTime::GetGameTimeMS();
    cached.requiresSwimming = requiresSwimming;

    Put(key, cached);
}

void PathCache::Clear()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _cacheList.clear();
    _cacheMap.clear();
    _metrics.currentSize = 0;
}

void PathCache::ClearExpired()
{
    std::lock_guard<std::mutex> lock(_mutex);

    auto it = _cacheList.begin();
    while (it != _cacheList.end())
    {
        if (IsExpired(it->second))
        {
            _cacheMap.erase(it->first);
            it = _cacheList.erase(it);
            _metrics.evictions++;
        }
        else
        {
            ++it;
        }
    }

    _metrics.currentSize = static_cast<uint32>(_cacheList.size());
}

PathCacheMetrics PathCache::GetMetrics() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _metrics;
}

void PathCache::ResetMetrics()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _metrics.Reset();
    _metrics.currentSize = static_cast<uint32>(_cacheList.size());
}

uint32 PathCache::Size() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return static_cast<uint32>(_cacheList.size());
}

void PathCache::EvictLRU()
{
    // Called with lock already held
    if (_cacheList.empty())
        return;

    // Remove from back (least recently used)
    auto& back = _cacheList.back();
    _cacheMap.erase(back.first);
    _cacheList.pop_back();

    _metrics.evictions++;
    _metrics.currentSize = static_cast<uint32>(_cacheList.size());
}

void PathCache::MoveToFront(CacheMap::iterator it)
{
    // Called with lock already held
    // Splice the element to the front
    _cacheList.splice(_cacheList.begin(), _cacheList, it->second);
}

bool PathCache::IsExpired(CachedPath const& path) const
{
    uint32 currentTime = GameTime::GetGameTimeMS();
    return (currentTime - path.timestamp) > _ttlMs;
}
