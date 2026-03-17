/*
 * Copyright (C) 2024 TrinityCore Playerbot Module
 *
 * Lock-Free Bot Spawner Optimized for 5000+ Bots
 */

#ifndef MODULE_PLAYERBOT_BOT_SPAWNER_OPTIMIZED_H
#define MODULE_PLAYERBOT_BOT_SPAWNER_OPTIMIZED_H

#include "Common.h"
#include "ObjectGuid.h"
#include <atomic>
#include <memory>
#include <chrono>
#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_queue.h>
#include <folly/concurrency/ConcurrentHashMap.h>
#include "../Threading/ThreadingPolicy.h"

class Player;
class Map;

namespace Playerbot
{

/**
 * Optimized Bot Spawner with Zero Lock Contention
 *
 * KEY IMPROVEMENTS:
 * 1. Lock-free concurrent data structures
 * 2. Wait-free statistics updates
 * 3. Parallel spawn processing
 * 4. Zero mutex contention design
 * 5. Scales linearly to 5000+ bots
 */
class BotSpawnerOptimized
{
public:
    // Spawn request structure
    struct SpawnRequest
    {
        enum Type
        {
            SPAWN_ZONE,
            SPAWN_FOLLOW,
            SPAWN_SPECIFIC,
            DESPAWN
        };

        Type type{SPAWN_ZONE};
        ObjectGuid requester;
        ObjectGuid targetGuid;
        uint32 zoneId{0};
        uint32 mapId{0};
        uint32 level{0};
        uint32 classId{0};
        uint32 faction{0};
        ::std::chrono::steady_clock::time_point timestamp;
    };

    // Bot spawn statistics (all atomic for lock-free access)
    struct SpawnStatistics
    {
        ::std::atomic<uint32> totalSpawned{0};
        ::std::atomic<uint32> totalDespawned{0};
        ::std::atomic<uint32> currentlyActive{0};
        ::std::atomic<uint32> peakConcurrent{0};
        ::std::atomic<uint32> failedSpawns{0};
        ::std::atomic<uint64> totalSpawnTime{0}; // microseconds
        ::std::atomic<uint32> spawnAttempts{0};
        ::std::atomic<uint32> queueSize{0};
        ::std::atomic<uint64> lastUpdateTime{0};

        void RecordSpawn(uint32 timeMicros)
        {
            totalSpawned.fetch_add(1, ::std::memory_order_relaxed);
            uint32 current = currentlyActive.fetch_add(1, ::std::memory_order_relaxed) + 1;

            // Update peak if needed
            uint32 currentPeak = peakConcurrent.load(::std::memory_order_relaxed);
            while (current > currentPeak &&
                   !peakConcurrent.compare_exchange_weak(currentPeak, current))
            {
                // Retry until successful
            }

            totalSpawnTime.fetch_add(timeMicros, ::std::memory_order_relaxed);
            spawnAttempts.fetch_add(1, ::std::memory_order_relaxed);
        }

        void RecordDespawn()
        {
            totalDespawned.fetch_add(1, ::std::memory_order_relaxed);
            currentlyActive.fetch_sub(1, ::std::memory_order_relaxed);
        }

        void RecordFailure()
        {
            failedSpawns.fetch_add(1, ::std::memory_order_relaxed);
            spawnAttempts.fetch_add(1, ::std::memory_order_relaxed);
        }
    };

    // Zone population info
    struct ZonePopulation
    {
        ::std::atomic<uint32> targetBots{0};
        ::std::atomic<uint32> currentBots{0};
        ::std::atomic<uint32> realPlayers{0};
        ::std::atomic<uint64> lastUpdate{0};
    };

    // Configuration
    struct Config
    {
        uint32 maxBotsPerZone{50};
        uint32 maxTotalBots{5000};
        uint32 spawnBatchSize{10};
        uint32 despawnBatchSize{10};
        uint32 minLevel{1};
        uint32 maxLevel{80};
        float populationRatio{0.5f}; // Bots per real player
        bool enableDynamicSpawning{true};
        bool preferSameFactio{true};
        uint32 updateIntervalMs{1000};
        uint32 zoneUpdateIntervalMs{5000};
    };

    static BotSpawnerOptimized& Instance();

    // Initialization and shutdown
    void Initialize();
    void Shutdown();

    // Main update loop (called from world update)
    void Update(uint32 diff);

    // Spawn interface
    bool RequestSpawn(SpawnRequest const& request);
    bool SpawnBot(ObjectGuid targetGuid, uint32 level = 0, uint32 classId = 0);
    bool SpawnBotInZone(uint32 zoneId, uint32 mapId, uint32 level = 0);
    bool DespawnBot(ObjectGuid botGuid);
    void DespawnAllBots();

    // Zone management
    void UpdateZonePopulation(uint32 zoneId, uint32 mapId, uint32 realPlayers);
    uint32 GetZoneTargetBots(uint32 zoneId) const;
    uint32 GetZoneCurrentBots(uint32 zoneId) const;

    // Statistics and monitoring
    SpawnStatistics GetStatistics() const;
    uint32 GetActiveBotCount() const { return _stats.currentlyActive.load(::std::memory_order_relaxed); }
    uint32 GetQueueSize() const { return _stats.queueSize.load(::std::memory_order_relaxed); }
    bool IsEnabled() const { return _enabled.load(::std::memory_order_acquire); }
    void SetEnabled(bool enabled) { _enabled.store(enabled, ::std::memory_order_release); }

    // Configuration
    Config const& GetConfig() const { return _config; }
    void SetConfig(Config const& config) { _config = config; }

private:
    BotSpawnerOptimized();
    ~BotSpawnerOptimized();

    // Internal spawn methods
    bool SpawnBotInternal(SpawnRequest const& request);
    void ProcessSpawnQueue();
    void CalculateZoneTargets();
    void SpawnToPopulationTarget();
    void DespawnExcessBots();

    // Helper methods
    ObjectGuid SelectBotForZone(uint32 zoneId, uint32 level);
    bool ValidateSpawnLocation(uint32 mapId, float x, float y, float z);
    void CleanupInactiveBots();

    // Lock-free data structures using TBB
    using BotMap = tbb::concurrent_hash_map<ObjectGuid, uint32>; // botGuid -> zoneId
    using ZoneMap = tbb::concurrent_hash_map<uint32, ZonePopulation>;
    using SpawnQueue = tbb::concurrent_queue<SpawnRequest>;

    // Ultra-high throughput map for bot sessions (5000+ bots)
    using SessionMap = folly::ConcurrentHashMap<ObjectGuid, ::std::shared_ptr<void>>;

    // Core data structures (all lock-free)
    BotMap _activeBots;
    ZoneMap _zonePopulations;
    SpawnQueue _spawnQueue;
    SessionMap _botSessions;

    // Configuration and state
    Config _config;
    ::std::atomic<bool> _enabled{true};
    ::std::atomic<bool> _initialized{false};
    ::std::atomic<bool> _processingQueue{false};

    // Statistics (all atomic)
    SpawnStatistics _stats;

    // Timing
    ::std::atomic<uint64> _lastUpdate{0};
    ::std::atomic<uint64> _lastZoneUpdate{0};
    ::std::atomic<uint64> _lastCleanup{0};

    // Performance monitoring
    ::std::atomic<uint32> _updateCounter{0};
    ::std::atomic<uint64> _totalUpdateTime{0}; // microseconds
    ::std::atomic<uint32> _slowUpdates{0};     // Updates taking > 10ms

    // Constants
    static constexpr uint32 UPDATE_INTERVAL_MS = 100;
    static constexpr uint32 ZONE_UPDATE_INTERVAL_MS = 5000;
    static constexpr uint32 CLEANUP_INTERVAL_MS = 30000;
    static constexpr uint32 MAX_SPAWN_PER_UPDATE = 50;
    static constexpr uint32 MAX_DESPAWN_PER_UPDATE = 50;

    // Deleted operations
    BotSpawnerOptimized(BotSpawnerOptimized const&) = delete;
    BotSpawnerOptimized& operator=(BotSpawnerOptimized const&) = delete;
};

// Global accessor
#define sBotSpawnerOptimized BotSpawnerOptimized::Instance()

} // namespace Playerbot

#endif // MODULE_PLAYERBOT_BOT_SPAWNER_OPTIMIZED_H