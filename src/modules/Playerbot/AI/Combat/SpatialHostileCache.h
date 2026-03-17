/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * Enterprise-Grade Spatial Hostile Cache for 5000+ Bot Scalability
 *
 * Architecture Features:
 * - Lock-free reads via RCU (Read-Copy-Update) pattern
 * - Zone-based partitioning to reduce lock contention
 * - Worker thread updates to avoid main thread blocking
 * - Memory-efficient hostile tracking with object pooling
 */

#ifndef TRINITYCORE_SPATIAL_HOSTILE_CACHE_H
#define TRINITYCORE_SPATIAL_HOSTILE_CACHE_H

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "ObjectGuid.h"
#include "Position.h"
#include <atomic>
#include <memory>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <shared_mutex>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/pool/object_pool.hpp>

class Unit;
class Creature;
class Player;

namespace Playerbot
{

// Forward declarations
class HostileEntry;
class CellCache;
class ZoneCache;

// Configuration constants optimized for 5000 bots
constexpr uint32 CACHE_UPDATE_INTERVAL_MS = 100;      // Zone cache refresh rate
constexpr uint32 CELL_SIZE = 50.0f;                   // Yards per cell
constexpr uint32 CELLS_PER_ZONE = 256;                // 16x16 grid
constexpr uint32 MAX_HOSTILES_PER_CELL = 32;          // Pre-allocated
constexpr uint32 BOT_LOCAL_CACHE_SIZE = 16;           // Per-bot cache entries
constexpr uint32 WORKER_BATCH_SIZE = 100;             // Updates per batch

/**
 * Hostile entity data optimized for cache efficiency
 * Size: 64 bytes (fits in single cache line)
 */
struct HostileEntry
{
    ObjectGuid guid;                    // 16 bytes
    float x, y, z;                      // 12 bytes
    uint32 level;                       // 4 bytes
    uint32 entryId;                     // 4 bytes
    uint8 rank;                         // 1 byte (elite, rare, etc)
    uint8 threatLevel;                  // 1 byte (0-255 scale)
    uint16 hostilityFlags;              // 2 bytes
    uint32 lastUpdateTime;              // 4 bytes
    uint32 cellIndex;                   // 4 bytes
    // Padding to 64 bytes
    uint8 _padding[16];

    bool IsValid() const { return guid && lastUpdateTime > 0; }
    float GetDistanceSq(float px, float py, float pz) const
    {
        float dx = x - px;
        float dy = y - py;
        float dz = z - pz;
        return dx*dx + dy*dy + dz*dz;
    }
};

/**
 * Cell-level cache for spatial queries
 * Lock-free read access with atomic pointer swapping
 */
class CellCache
{
public:
    CellCache();
    ~CellCache();

    // Thread-safe read (lock-free)
    ::std::vector<HostileEntry> GetHostiles() const;

    // Writer thread only
    void UpdateHostiles(::std::vector<HostileEntry>&& hostiles);

    uint32 GetLastUpdateTime() const { return _lastUpdate.load(); }
    bool IsStale(uint32 currentTime) const;

private:
    // RCU pattern: atomic pointer to immutable data
    mutable ::std::atomic<::std::vector<HostileEntry>*> _hostiles;
    ::std::atomic<uint32> _lastUpdate;
    ::std::atomic<uint32> _version;

    // Memory pool for vector recycling
    static thread_local boost::object_pool<::std::vector<HostileEntry>> s_vectorPool;
};

/**
 * Zone-level hostile cache with spatial indexing
 */
class ZoneCache
{
public:
    explicit ZoneCache(uint32 zoneId);
    ~ZoneCache();

    // Query interface (thread-safe, lock-free reads)
    ::std::vector<HostileEntry> FindHostilesInRange(
        float x, float y, float z, float range,
        uint32 maxResults = 100) const;

    // Update interface (writer thread only)
    void BeginUpdate();
    void AddHostile(Unit* unit);
    void CommitUpdate();

    uint32 GetZoneId() const { return _zoneId; }
    uint32 GetHostileCount() const { return _totalHostiles.load(); }
    bool NeedsUpdate(uint32 currentTime) const;

private:
    uint32 GetCellIndex(float x, float y) const;
    void GetCellsInRange(float x, float y, float range,
                        ::std::vector<uint32>& cells) const;

    const uint32 _zoneId;
    ::std::atomic<uint32> _totalHostiles;
    ::std::atomic<uint32> _lastFullUpdate;

    // Spatial index: 16x16 grid of cells
    ::std::unique_ptr<CellCache> _cells[CELLS_PER_ZONE];

    // Staging area for updates (writer thread only)
    ::std::unordered_map<uint32, ::std::vector<HostileEntry>> _stagingCells;

    // Zone boundaries for cell calculation
    float _minX, _maxX, _minY, _maxY;
    float _cellWidth, _cellHeight;
};

/**
 * Global spatial hostile cache manager
 * Coordinates all zone caches and worker threads
 */
class SpatialHostileCache
{
public:
    static SpatialHostileCache& Instance();

    // Lifecycle
    void Initialize();
    void Shutdown();

    // Bot query interface (main thread, lock-free)
    ::std::vector<HostileEntry> FindHostilesForBot(
        Player* bot, float range,
        uint32 maxResults = 50);

    // Update scheduling (called from world update)
    void ScheduleZoneUpdate(uint32 zoneId);
    void ProcessUpdates(uint32 diff);

    // Statistics
    struct CacheStats
    {
        uint32 totalQueries;
        uint32 cacheHits;
        uint32 cacheMisses;
        uint32 avgQueryTimeUs;
        uint32 totalZones;
        uint32 totalHostiles;
        uint32 updateBacklog;
        float cacheHitRate;
    };

    CacheStats GetStatistics() const;

private:
    SpatialHostileCache();
    ~SpatialHostileCache();

    // Worker thread operations
    void WorkerThreadMain();
    void ProcessZoneUpdate(uint32 zoneId);
    void ScanZoneForHostiles(ZoneCache* cache);

    // Cache management
    ZoneCache* GetOrCreateZoneCache(uint32 zoneId);
    void PruneInactiveZones();

    // Zone caches with RCU protection
    mutable Playerbot::OrderedSharedMutex<Playerbot::LockOrder::SPATIAL_GRID> _zoneCacheMutex;
    ::std::unordered_map<uint32, ::std::unique_ptr<ZoneCache>> _zoneCaches;

    // Update queue (lock-free SPSC)
    boost::lockfree::spsc_queue<uint32, boost::lockfree::capacity<1024>> _updateQueue;

    // Worker thread management
    ::std::atomic<bool> _running;
    ::std::unique_ptr<::std::thread> _workerThread;

    // Statistics tracking
    mutable ::std::atomic<uint32> _totalQueries;
    mutable ::std::atomic<uint32> _cacheHits;
    mutable ::std::atomic<uint32> _cacheMisses;
    mutable ::std::atomic<uint64> _totalQueryTimeUs;

    // Memory pools
    boost::object_pool<HostileEntry> _entryPool;

    // Deleted operations
    SpatialHostileCache(const SpatialHostileCache&) = delete;
    SpatialHostileCache& operator=(const SpatialHostileCache&) = delete;
};

/**
 * Bot-local hostile cache with LRU eviction
 * Reduces repeated queries for same targets
 */
class BotLocalHostileCache
{
public:
    explicit BotLocalHostileCache(Player* bot);
    ~BotLocalHostileCache();

    // Query with local caching
    ::std::vector<HostileEntry> GetHostilesInRange(float range);

    // Cache management
    void InvalidateCache();
    void OnCombatStart();
    void OnCombatEnd();

private:
    struct CacheEntry
    {
        float range;
        uint32 timestamp;
        ::std::vector<HostileEntry> hostiles;
    };

    Player* _bot;
    ::std::vector<CacheEntry> _cache;
    uint32 _lastAccessTime;
    bool _inCombat;

    // LRU tracking
    void EvictOldestEntry();
    CacheEntry* FindCacheEntry(float range);
};

} // namespace Playerbot

#endif // TRINITYCORE_SPATIAL_HOSTILE_CACHE_H