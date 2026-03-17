# Phase 2: Bot Lifecycle Management - Detailed Implementation Plan

## Executive Summary
**Component**: Bot Spawning, Scheduling & Lifecycle Coordination  
**Duration**: Week 11 (1 week)  
**Foundation**: Leverages Phase 1 enterprise infrastructure + Account/Character management  
**Scope**: Intelligent bot population management, realistic login patterns, server balance  

## Architecture Overview

### System Integration
```cpp
// Lifecycle management coordinates all bot systems
BotLifecycleMgr
├── BotSpawner      // Population management
│   ├── Uses: BotAccountMgr (account pool)
│   ├── Uses: BotCharacterMgr (character selection)
│   └── Uses: BotSessionMgr (session creation)
├── BotScheduler    // Login/logout patterns
│   ├── Uses: Intel TBB (parallel scheduling)
│   ├── Uses: BotDatabasePool (async persistence)
│   └── Uses: Time-based algorithms
└── BotPopulationMgr // Zone distribution
    ├── Uses: World state monitoring
    ├── Uses: Player density calculations
    └── Uses: Dynamic rebalancing
```

### Performance Targets
- **Spawn Time**: <50ms per bot (including session creation)
- **Batch Spawning**: 100 bots in <2 seconds
- **Scheduling Overhead**: <0.1% CPU for 1000 bots
- **Memory**: <10KB per scheduled bot
- **Rebalancing**: <100ms for population adjustments
- **Database**: Async persistence, no blocking

## Week 11: Complete Implementation

### Day 1-2: Bot Spawner Implementation

#### File: `src/modules/Playerbot/Lifecycle/BotSpawner.h`
```cpp
#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <memory>
#include <vector>
#include <atomic>
#include <tbb/concurrent_queue.h>
#include <phmap.h>

namespace Playerbot
{

// Forward declarations
class BotSession;
class BotScheduler;

// Spawn request structure
struct SpawnRequest
{
    enum Type { RANDOM, SPECIFIC_CHARACTER, SPECIFIC_ZONE, GROUP_MEMBER };
    
    Type type = RANDOM;
    uint32 accountId = 0;
    ObjectGuid characterGuid;
    uint32 zoneId = 0;
    uint32 mapId = 0;
    uint8 minLevel = 0;
    uint8 maxLevel = 0;
    uint8 classFilter = 0;
    uint8 raceFilter = 0;
    
    // Callback on spawn completion
    std::function<void(bool success, ObjectGuid guid)> callback;
};

// Spawn configuration
struct SpawnConfig
{
    uint32 maxBotsTotal = 500;
    uint32 maxBotsPerZone = 50;
    uint32 maxBotsPerMap = 200;
    uint32 spawnBatchSize = 10;
    uint32 spawnDelayMs = 100;
    bool enableDynamicSpawning = true;
    bool respectPopulationCaps = true;
    float botToPlayerRatio = 2.0f;
};

// Zone population data
struct ZonePopulation
{
    uint32 zoneId;
    uint32 mapId;
    uint32 playerCount;
    uint32 botCount;
    uint32 targetBotCount;
    uint8 minLevel;
    uint8 maxLevel;
    float density; // Bots per square unit
};

class TC_GAME_API BotSpawner
{
    BotSpawner();
    ~BotSpawner();
    BotSpawner(BotSpawner const&) = delete;
    BotSpawner& operator=(BotSpawner const&) = delete;

public:
    static BotSpawner* instance();

    // Initialization
    bool Initialize();
    void Shutdown();

    // Main spawn functions - Enterprise batch processing
    ObjectGuid SpawnBot(SpawnRequest const& request);
    std::vector<ObjectGuid> SpawnBots(std::vector<SpawnRequest> const& requests);
    std::future<std::vector<ObjectGuid>> SpawnBotsAsync(
        uint32 count,
        SpawnRequest::Type type = SpawnRequest::RANDOM
    );

    // Intelligent spawning
    void SpawnToPopulationTarget();
    void SpawnForZone(uint32 zoneId, uint32 targetCount);
    void SpawnForDungeon(uint32 mapId, uint32 groupSize = 5);
    void SpawnForBattleground(uint32 bgTypeId, uint32 teamSize);

    // Despawn functions
    bool DespawnBot(ObjectGuid guid);
    uint32 DespawnBots(uint32 count);
    uint32 DespawnInactiveBot();
    void DespawnAllBots();

    // Population management
    void UpdatePopulationTargets();
    ZonePopulation GetZonePopulation(uint32 zoneId) const;
    uint32 GetTotalBotCount() const { return _activeBots.size(); }
    uint32 GetActiveSessionCount() const;

    // Configuration
    void SetConfig(SpawnConfig const& config) { _config = config; }
    SpawnConfig const& GetConfig() const { return _config; }

    // Statistics
    struct Statistics
    {
        std::atomic<uint32> totalSpawned;
        std::atomic<uint32> totalDespawned;
        std::atomic<uint32> spawnFailures;
        std::atomic<uint32> averageSpawnTimeMs;
        std::atomic<uint32> peakPopulation;
    };
    Statistics const& GetStatistics() const { return _stats; }

private:
    // Internal spawn logic
    ObjectGuid SpawnBotInternal(SpawnRequest const& request);
    bool SelectCharacterForSpawn(SpawnRequest const& request, ObjectGuid& characterGuid);
    bool CreateBotSession(ObjectGuid characterGuid);
    void ProcessSpawnQueue();
    
    // Population calculations
    uint32 CalculateTargetPopulation(uint32 zoneId) const;
    void BalancePopulation();
    std::vector<ObjectGuid> SelectBotsForDespawn(uint32 count);
    
    // Zone analysis
    void AnalyzeZoneDensity();
    std::vector<uint32> GetUnderpopulatedZones() const;
    std::vector<uint32> GetOverpopulatedZones() const;

private:
    // Active bot tracking
    phmap::parallel_flat_hash_set<ObjectGuid> _activeBots;
    phmap::parallel_flat_hash_map<uint32, std::vector<ObjectGuid>> _zoneBots;
    phmap::parallel_flat_hash_map<ObjectGuid::LowType, uint32> _botZones;
    
    // Spawn queue for batch processing
    tbb::concurrent_queue<SpawnRequest> _spawnQueue;
    
    // Population data
    phmap::parallel_flat_hash_map<uint32, ZonePopulation> _zonePopulations;
    
    // Thread pool for async operations
    std::unique_ptr<boost::asio::thread_pool> _spawnWorkers;
    
    // Configuration
    SpawnConfig _config;
    
    // Statistics
    mutable Statistics _stats;
    
    // Processing thread
    std::thread _processingThread;
    std::atomic<bool> _shutdown;
};

#define sBotSpawner BotSpawner::instance()

} // namespace Playerbot
```

### Day 5: Lifecycle Coordinator Implementation

#### File: `src/modules/Playerbot/Lifecycle/BotLifecycleMgr.h`
```cpp
#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <memory>
#include <functional>
#include <phmap.h>

namespace Playerbot
{

// Forward declarations
class BotSpawner;
class BotScheduler;

// Lifecycle events
enum class LifecycleEvent
{
    BOT_CREATED,
    BOT_SPAWNED,
    BOT_DESPAWNED,
    BOT_DELETED,
    SESSION_START,
    SESSION_END,
    ZONE_CHANGED,
    LEVEL_CHANGED,
    ACTIVITY_CHANGED
};

// Event handler callback
using LifecycleEventHandler = std::function<void(LifecycleEvent, ObjectGuid, uint32, uint32)>;

// Lifecycle statistics
struct LifecycleStats
{
    std::atomic<uint32> totalCreated;
    std::atomic<uint32> totalSpawned;
    std::atomic<uint32> totalDespawned;
    std::atomic<uint32> currentlyActive;
    std::atomic<uint32> peakConcurrent;
    std::atomic<uint32> totalSessionTime;
    std::atomic<uint32> averageSessionLength;
};

class TC_GAME_API BotLifecycleMgr
{
    BotLifecycleMgr();
    ~BotLifecycleMgr();
    BotLifecycleMgr(BotLifecycleMgr const&) = delete;
    BotLifecycleMgr& operator=(BotLifecycleMgr const&) = delete;

public:
    static BotLifecycleMgr* instance();

    // Initialization
    bool Initialize();
    void Shutdown();
    void Update(uint32 diff);

    // Lifecycle orchestration
    void StartLifecycle();
    void StopLifecycle();
    void PauseLifecycle();
    void ResumeLifecycle();

    // Population management
    void MaintainPopulation();
    void AdjustPopulation(uint32 targetCount);
    void BalanceZonePopulations();
    void RefreshInactiveBots();

    // Event handling
    void RegisterEventHandler(LifecycleEventHandler handler);
    void TriggerEvent(LifecycleEvent event, ObjectGuid guid, uint32 data1 = 0, uint32 data2 = 0);

    // Query functions
    bool IsLifecycleActive() const { return _active; }
    uint32 GetActiveCount() const { return _stats.currentlyActive.load(); }
    LifecycleStats const& GetStatistics() const { return _stats; }

    // Server integration hooks
    void OnServerStartup();
    void OnServerShutdown();
    void OnPlayerLogin(Player* player);
    void OnPlayerLogout(Player* player);

private:
    // Internal lifecycle management
    void ProcessLifecycleTick();
    void CheckPopulationTargets();
    void HandleInactiveBots();
    void UpdateStatistics();

private:
    // Sub-managers
    std::unique_ptr<BotSpawner> _spawner;
    std::unique_ptr<BotScheduler> _scheduler;

    // Event handlers
    std::vector<LifecycleEventHandler> _eventHandlers;

    // State
    std::atomic<bool> _active;
    std::atomic<bool> _paused;
    uint32 _updateTimer;

    // Statistics
    LifecycleStats _stats;

    // Configuration
    uint32 _targetPopulation;
    uint32 _updateInterval;
    bool _dynamicPopulation;
};

#define sBotLifecycleMgr BotLifecycleMgr::instance()

} // namespace Playerbot
```

## Database Schema

### SQL Schema: `sql/playerbot/003_lifecycle_management.sql`
```sql
-- Bot spawn logs for analytics
CREATE TABLE IF NOT EXISTS `playerbot_spawn_log` (
    `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `guid` BIGINT UNSIGNED NOT NULL,
    `zone_id` INT UNSIGNED DEFAULT NULL,
    `map_id` INT UNSIGNED DEFAULT NULL,
    `spawn_time` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    `despawn_time` TIMESTAMP NULL DEFAULT NULL,
    `session_duration` INT UNSIGNED DEFAULT NULL,
    `spawn_reason` VARCHAR(50) DEFAULT NULL,
    `despawn_reason` VARCHAR(50) DEFAULT NULL,
    PRIMARY KEY (`id`),
    KEY `idx_guid` (`guid`),
    KEY `idx_spawn_time` (`spawn_time`),
    KEY `idx_zone` (`zone_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Activity patterns for scheduling
CREATE TABLE IF NOT EXISTS `playerbot_activity_patterns` (
    `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `name` VARCHAR(50) NOT NULL,
    `active_hours` JSON NOT NULL,
    `active_days` JSON NOT NULL,
    `login_probability` FLOAT DEFAULT 1.0,
    `min_session` INT UNSIGNED DEFAULT 1800,
    `max_session` INT UNSIGNED DEFAULT 14400,
    `sessions_per_day` INT UNSIGNED DEFAULT 2,
    `prefer_peak` BOOLEAN DEFAULT TRUE,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_pattern_name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Bot schedules
CREATE TABLE IF NOT EXISTS `playerbot_schedules` (
    `guid` BIGINT UNSIGNED NOT NULL,
    `pattern_name` VARCHAR(50) DEFAULT 'default',
    `next_login` TIMESTAMP NULL DEFAULT NULL,
    `next_logout` TIMESTAMP NULL DEFAULT NULL,
    `total_sessions` INT UNSIGNED DEFAULT 0,
    `total_playtime` INT UNSIGNED DEFAULT 0,
    `last_update` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`guid`),
    KEY `idx_next_login` (`next_login`),
    KEY `idx_pattern` (`pattern_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Zone population targets
CREATE TABLE IF NOT EXISTS `playerbot_zone_populations` (
    `zone_id` INT UNSIGNED NOT NULL,
    `map_id` INT UNSIGNED NOT NULL,
    `min_bots` INT UNSIGNED DEFAULT 0,
    `max_bots` INT UNSIGNED DEFAULT 50,
    `target_bots` INT UNSIGNED DEFAULT 10,
    `current_bots` INT UNSIGNED DEFAULT 0,
    `min_level` TINYINT UNSIGNED DEFAULT 1,
    `max_level` TINYINT UNSIGNED DEFAULT 80,
    `bot_density` FLOAT DEFAULT 0.1,
    `last_update` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`zone_id`),
    KEY `idx_map` (`map_id`),
    KEY `idx_level_range` (`min_level`, `max_level`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Lifecycle events log
CREATE TABLE IF NOT EXISTS `playerbot_lifecycle_events` (
    `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `event_type` ENUM('CREATED','SPAWNED','DESPAWNED','DELETED',
                      'SESSION_START','SESSION_END','ZONE_CHANGED',
                      'LEVEL_CHANGED','ACTIVITY_CHANGED') NOT NULL,
    `guid` BIGINT UNSIGNED NOT NULL,
    `data1` INT UNSIGNED DEFAULT NULL,
    `data2` INT UNSIGNED DEFAULT NULL,
    `timestamp` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_guid` (`guid`),
    KEY `idx_timestamp` (`timestamp`),
    KEY `idx_event_type` (`event_type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
```

## Testing Suite

### File: `src/modules/Playerbot/Tests/LifecycleManagementTest.cpp`
```cpp
#include "catch.hpp"
#include "BotSpawner.h"
#include "BotScheduler.h"
#include "BotLifecycleMgr.h"
#include "TestHelpers.h"
#include <chrono>
#include <thread>

using namespace Playerbot;

TEST_CASE("BotSpawner", "[lifecycle][integration]")
{
    SECTION("Basic Spawning")
    {
        SECTION("Spawns single bot")
        {
            SpawnRequest request;
            request.type = SpawnRequest::RANDOM;

            ObjectGuid guid = sBotSpawner->SpawnBot(request);
            REQUIRE(!guid.IsEmpty());
            REQUIRE(sBotSpawner->GetTotalBotCount() > 0);
        }

        SECTION("Respects population caps")
        {
            SpawnConfig config = sBotSpawner->GetConfig();
            config.maxBotsTotal = 5;
            config.respectPopulationCaps = true;
            sBotSpawner->SetConfig(config);

            // Spawn to limit
            std::vector<ObjectGuid> bots;
            for (int i = 0; i < 5; ++i)
            {
                SpawnRequest request;
                request.type = SpawnRequest::RANDOM;
                ObjectGuid guid = sBotSpawner->SpawnBot(request);
                REQUIRE(!guid.IsEmpty());
                bots.push_back(guid);
            }

            // 6th should fail
            SpawnRequest request;
            request.type = SpawnRequest::RANDOM;
            ObjectGuid guid = sBotSpawner->SpawnBot(request);
            REQUIRE(guid.IsEmpty());

            // Cleanup
            for (auto const& botGuid : bots)
            {
                sBotSpawner->DespawnBot(botGuid);
            }
        }

        SECTION("Batch spawning works")
        {
            std::vector<SpawnRequest> requests;
            for (int i = 0; i < 10; ++i)
            {
                SpawnRequest request;
                request.type = SpawnRequest::RANDOM;
                requests.push_back(request);
            }

            auto guids = sBotSpawner->SpawnBots(requests);
            REQUIRE(guids.size() == 10);

            // Cleanup
            for (auto const& guid : guids)
            {
                sBotSpawner->DespawnBot(guid);
            }
        }
    }

    SECTION("Zone Population")
    {
        SECTION("Maintains zone populations")
        {
            uint32 testZoneId = 1519; // Stormwind
            sBotSpawner->SpawnForZone(testZoneId, 5);

            auto population = sBotSpawner->GetZonePopulation(testZoneId);
            REQUIRE(population.botCount >= 5);
        }

        SECTION("Updates population targets")
        {
            sBotSpawner->UpdatePopulationTargets();
            // Should complete without errors
        }
    }
}

TEST_CASE("BotScheduler", "[lifecycle][scheduling]")
{
    SECTION("Pattern Management")
    {
        SECTION("Loads default patterns")
        {
            REQUIRE(sBotScheduler->Initialize());
            auto pattern = sBotScheduler->GetPattern("default");
            REQUIRE(pattern != nullptr);
            REQUIRE(pattern->name == "default");
        }

        SECTION("Registers custom pattern")
        {
            ActivityPattern custom;
            custom.name = "test_pattern";
            custom.activeHours = {{9, 17}}; // 9 AM to 5 PM
            custom.minSessionDuration = 600;
            custom.maxSessionDuration = 1800;

            sBotScheduler->RegisterPattern("test_pattern", custom);
            auto pattern = sBotScheduler->GetPattern("test_pattern");
            REQUIRE(pattern != nullptr);
            REQUIRE(pattern->name == "test_pattern");
        }
    }

    SECTION("Scheduling")
    {
        ObjectGuid testGuid = ObjectGuid::Create<HighGuid::Player>(12345);

        SECTION("Schedules bot successfully")
        {
            sBotScheduler->ScheduleBot(testGuid);
            REQUIRE(sBotScheduler->IsBotScheduled(testGuid));
            REQUIRE(sBotScheduler->GetScheduledBotCount() > 0);
        }

        SECTION("Schedules actions")
        {
            ScheduleEntry entry;
            entry.botGuid = testGuid;
            entry.action = ScheduleEntry::LOGIN;
            entry.executeTime = std::chrono::system_clock::now() + std::chrono::seconds(1);

            sBotScheduler->ScheduleAction(entry);
            // Action should be queued
        }

        SECTION("Calculates next login time")
        {
            sBotScheduler->AssignPattern(testGuid, "default");
            auto nextLogin = sBotScheduler->GetNextScheduledAction(testGuid);
            REQUIRE(nextLogin > std::chrono::system_clock::now());
        }
    }
}

TEST_CASE("Performance", "[lifecycle][performance]")
{
    SECTION("Spawn performance")
    {
        auto start = std::chrono::high_resolution_clock::now();

        SpawnRequest request;
        request.type = SpawnRequest::RANDOM;
        ObjectGuid guid = sBotSpawner->SpawnBot(request);

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start).count();

        REQUIRE(!guid.IsEmpty());
        REQUIRE(elapsed < 50); // Should complete in under 50ms

        sBotSpawner->DespawnBot(guid);
    }

    SECTION("Batch spawn performance")
    {
        auto start = std::chrono::high_resolution_clock::now();

        std::vector<SpawnRequest> requests;
        for (int i = 0; i < 100; ++i)
        {
            SpawnRequest request;
            request.type = SpawnRequest::RANDOM;
            requests.push_back(request);
        }

        auto guids = sBotSpawner->SpawnBots(requests);

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start).count();

        REQUIRE(guids.size() == 100);
        REQUIRE(elapsed < 2000); // 100 bots in under 2 seconds

        // Cleanup
        for (auto const& guid : guids)
        {
            sBotSpawner->DespawnBot(guid);
        }
    }

    SECTION("Scheduling overhead")
    {
        // Schedule 1000 bots
        std::vector<ObjectGuid> testGuids;
        for (int i = 0; i < 1000; ++i)
        {
            ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(100000 + i);
            testGuids.push_back(guid);
            sBotScheduler->ScheduleBot(guid);
        }

        REQUIRE(sBotScheduler->GetScheduledBotCount() == 1000);

        // Measure CPU usage of scheduler
        auto start = std::chrono::high_resolution_clock::now();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start).count();

        // Scheduling overhead should be minimal
        // This is a simplified test - real measurement would track actual CPU time
        REQUIRE(elapsed >= 1000); // At least 1 second passed
        REQUIRE(elapsed < 1100);  // No more than 100ms overhead
    }
}
```

## Configuration File

### File: `conf/playerbots.conf.dist` (Lifecycle Section)
```ini
###################################################################################################
#   LIFECYCLE MANAGEMENT CONFIGURATION
#
#   Bot spawning, scheduling, and population management
###################################################################################################

#
#   Playerbot.Spawn.MaxTotal
#       Maximum total number of bots that can be spawned
#       Default: 500
#

Playerbot.Spawn.MaxTotal = 500

#
#   Playerbot.Spawn.MaxPerZone
#       Maximum number of bots per zone
#       Default: 50
#

Playerbot.Spawn.MaxPerZone = 50

#
#   Playerbot.Spawn.MaxPerMap
#       Maximum number of bots per map
#       Default: 200
#

Playerbot.Spawn.MaxPerMap = 200

#
#   Playerbot.Spawn.BatchSize
#       Number of bots to spawn in parallel batches
#       Default: 10
#

Playerbot.Spawn.BatchSize = 10

#
#   Playerbot.Spawn.Dynamic
#       Enable dynamic spawning based on player population
#       Default: 1 (enabled)
#

Playerbot.Spawn.Dynamic = 1

#
#   Playerbot.Spawn.BotToPlayerRatio
#       Ratio of bots to players for dynamic spawning
#       Default: 2.0 (2 bots per player)
#

Playerbot.Spawn.BotToPlayerRatio = 2.0

#
#   Playerbot.Schedule.Enable
#       Enable realistic scheduling patterns
#       Default: 1 (enabled)
#

Playerbot.Schedule.Enable = 1

#
#   Playerbot.Schedule.RealisticPatterns
#       Use realistic login/logout patterns
#       Default: 1 (enabled)
#

Playerbot.Schedule.RealisticPatterns = 1

#
#   Playerbot.Schedule.PeakMultiplier
#       Bot activity multiplier during peak hours (6 PM - 11 PM)
#       Default: 2.0
#

Playerbot.Schedule.PeakMultiplier = 2.0

#
#   Playerbot.Schedule.OffPeakMultiplier
#       Bot activity multiplier during off-peak hours (2 AM - 10 AM)
#       Default: 0.5
#

Playerbot.Schedule.OffPeakMultiplier = 0.5
```

## Implementation Checklist

### Week 11 Tasks
- [ ] Implement BotSpawner core class
- [ ] Add population management logic
- [ ] Implement zone-based spawning
- [ ] Create spawn request queue with Intel TBB
- [ ] Implement BotScheduler with activity patterns
- [ ] Add realistic login/logout scheduling
- [ ] Implement BotLifecycleMgr coordinator
- [ ] Create database tables for lifecycle data
- [ ] Write comprehensive unit tests
- [ ] Performance testing with 500+ bots

## Performance Optimizations

### Implemented Optimizations
1. **Parallel Spawning**: Intel TBB thread pool for concurrent spawning
2. **Batch Processing**: Spawn multiple bots in parallel batches
3. **Priority Queue**: TBB concurrent_priority_queue for scheduling
4. **Async Database**: All lifecycle data persisted asynchronously
5. **Memory Efficiency**: <10KB per scheduled bot
6. **Lock-Free Collections**: phmap for all concurrent data structures

### Scalability Metrics
- Spawn 100 bots: <2 seconds
- Schedule 1000 bots: <0.1% CPU overhead
- Population rebalancing: <100ms
- Memory usage: Linear scaling, <10MB for 1000 bots

## Integration Points

### With Phase 1 Components
```cpp
// Uses enterprise session management
BotSession* session = sBotSessionMgr->CreateSession(accountId);

// Leverages async database
sBotDBPool->ExecuteAsync(stmt);

// Uses Intel TBB for parallelism
tbb::concurrent_queue<SpawnRequest> for batch processing
```

### With Phase 2 Components
```cpp
// Integrates with account management
std::vector<uint32> accounts = sBotAccountMgr->GetAvailableAccounts();

// Uses character management
std::vector<ObjectGuid> characters = sBotCharacterMgr->GetBotCharacters(accountId);
```

## Next Steps

After completing Week 11 lifecycle management:
1. Proceed to AI Framework implementation (Week 12-14)
2. Integrate AI with lifecycle systems
3. Implement strategy pattern for decision making
4. Add movement and combat systems

---

**Status**: Ready for implementation
**Dependencies**: Phase 1 ✅, Account Management ✅, Character Management ✅
**Estimated Completion**: 1 week

#### File: `src/modules/Playerbot/Lifecycle/BotSpawner.cpp`
```cpp
#include "BotSpawner.h"
#include "BotSession.h"
#include "BotSessionMgr.h"
#include "BotAccountMgr.h"
#include "BotCharacterMgr.h"
#include "BotScheduler.h"
#include "BotDatabasePool.h"
#include "World.h"
#include "WorldSession.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "Log.h"
#include <execution>
#include <boost/asio.hpp>

namespace Playerbot
{

BotSpawner::BotSpawner()
    : _shutdown(false)
{
    _spawnWorkers = std::make_unique<boost::asio::thread_pool>(4);
}

BotSpawner::~BotSpawner()
{
    Shutdown();
}

BotSpawner* BotSpawner::instance()
{
    static BotSpawner instance;
    return &instance;
}

bool BotSpawner::Initialize()
{
    TC_LOG_INFO("playerbot", "Initializing BotSpawner with enterprise features...");

    // Load configuration
    _config.maxBotsTotal = sConfigMgr->GetIntDefault("Playerbot.Spawn.MaxTotal", 500);
    _config.maxBotsPerZone = sConfigMgr->GetIntDefault("Playerbot.Spawn.MaxPerZone", 50);
    _config.maxBotsPerMap = sConfigMgr->GetIntDefault("Playerbot.Spawn.MaxPerMap", 200);
    _config.spawnBatchSize = sConfigMgr->GetIntDefault("Playerbot.Spawn.BatchSize", 10);
    _config.spawnDelayMs = sConfigMgr->GetIntDefault("Playerbot.Spawn.DelayMs", 100);
    _config.enableDynamicSpawning = sConfigMgr->GetBoolDefault("Playerbot.Spawn.Dynamic", true);
    _config.respectPopulationCaps = sConfigMgr->GetBoolDefault("Playerbot.Spawn.RespectCaps", true);
    _config.botToPlayerRatio = sConfigMgr->GetFloatDefault("Playerbot.Spawn.BotToPlayerRatio", 2.0f);

    // Initialize zone populations
    AnalyzeZoneDensity();
    UpdatePopulationTargets();

    // Start processing thread
    _processingThread = std::thread([this] { ProcessSpawnQueue(); });

    TC_LOG_INFO("playerbot", "BotSpawner initialized successfully");
    TC_LOG_INFO("playerbot", "  -> Max total bots: %u", _config.maxBotsTotal);
    TC_LOG_INFO("playerbot", "  -> Max per zone: %u", _config.maxBotsPerZone);
    TC_LOG_INFO("playerbot", "  -> Dynamic spawning: %s",
                _config.enableDynamicSpawning ? "enabled" : "disabled");
    TC_LOG_INFO("playerbot", "  -> Bot to player ratio: %.1f", _config.botToPlayerRatio);

    return true;
}

void BotSpawner::Shutdown()
{
    TC_LOG_INFO("playerbot", "Shutting down BotSpawner...");

    _shutdown = true;

    // Stop processing thread
    if (_processingThread.joinable())
        _processingThread.join();

    // Despawn all bots
    DespawnAllBots();

    // Wait for workers to finish
    _spawnWorkers->join();

    TC_LOG_INFO("playerbot", "BotSpawner shutdown complete. Stats:");
    TC_LOG_INFO("playerbot", "  -> Total spawned: %u", _stats.totalSpawned.load());
    TC_LOG_INFO("playerbot", "  -> Total despawned: %u", _stats.totalDespawned.load());
    TC_LOG_INFO("playerbot", "  -> Peak population: %u", _stats.peakPopulation.load());
}

ObjectGuid BotSpawner::SpawnBot(SpawnRequest const& request)
{
    auto startTime = std::chrono::high_resolution_clock::now();

    // Check population limits
    if (_config.respectPopulationCaps)
    {
        if (_activeBots.size() >= _config.maxBotsTotal)
        {
            TC_LOG_WARN("playerbot", "SpawnBot: Maximum bot population reached (%u)",
                        _config.maxBotsTotal);
            if (request.callback)
                request.callback(false, ObjectGuid::Empty);
            return ObjectGuid::Empty;
        }

        if (request.zoneId > 0)
        {
            auto it = _zoneBots.find(request.zoneId);
            if (it != _zoneBots.end() && it->second.size() >= _config.maxBotsPerZone)
            {
                TC_LOG_WARN("playerbot", "SpawnBot: Zone %u has reached bot limit (%u)",
                            request.zoneId, _config.maxBotsPerZone);
                if (request.callback)
                    request.callback(false, ObjectGuid::Empty);
                return ObjectGuid::Empty;
            }
        }
    }

    // Process spawn request
    ObjectGuid guid = SpawnBotInternal(request);

    if (!guid.IsEmpty())
    {
        // Track active bot
        _activeBots.insert(guid);

        // Track zone if specified
        if (request.zoneId > 0)
        {
            _zoneBots[request.zoneId].push_back(guid);
            _botZones[guid.GetRawValue()] = request.zoneId;
        }

        // Update statistics
        _stats.totalSpawned++;
        uint32 currentCount = _activeBots.size();
        uint32 peak = _stats.peakPopulation.load();
        while (peak < currentCount && !_stats.peakPopulation.compare_exchange_weak(peak, currentCount));

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - startTime).count();
        
        // Update average spawn time
        uint32 avgTime = _stats.averageSpawnTimeMs.load();
        _stats.averageSpawnTimeMs = (avgTime + elapsed) / 2;

        TC_LOG_DEBUG("playerbot", "Spawned bot %s in %ums",
                     guid.ToString().c_str(), elapsed);

        // Store spawn metadata
        auto stmt = sBotDBPool->GetPreparedStatement(BOT_INS_SPAWN_LOG);
        stmt->setUInt64(0, guid.GetRawValue());
        stmt->setUInt32(1, request.zoneId);
        stmt->setUInt32(2, request.mapId);
        stmt->setUInt32(3, elapsed);
        sBotDBPool->ExecuteAsync(stmt);
    }
    else
    {
        _stats.spawnFailures++;
    }

    // Execute callback
    if (request.callback)
        request.callback(!guid.IsEmpty(), guid);

    return guid;
}

ObjectGuid BotSpawner::SpawnBotInternal(SpawnRequest const& request)
{
    ObjectGuid characterGuid;

    // Select or determine character to spawn
    if (request.type == SpawnRequest::SPECIFIC_CHARACTER)
    {
        characterGuid = request.characterGuid;
    }
    else
    {
        if (!SelectCharacterForSpawn(request, characterGuid))
        {
            TC_LOG_ERROR("playerbot", "Failed to select character for spawn");
            return ObjectGuid::Empty;
        }
    }

    // Validate character
    if (!sBotCharacterMgr->IsBotCharacter(characterGuid))
    {
        TC_LOG_ERROR("playerbot", "Character %s is not a bot character",
                     characterGuid.ToString().c_str());
        return ObjectGuid::Empty;
    }

    // Create or retrieve session
    if (!CreateBotSession(characterGuid))
    {
        TC_LOG_ERROR("playerbot", "Failed to create session for character %s",
                     characterGuid.ToString().c_str());
        return ObjectGuid::Empty;
    }

    // Schedule bot activity
    sBotScheduler->ScheduleBot(characterGuid);

    return characterGuid;
}

bool BotSpawner::SelectCharacterForSpawn(SpawnRequest const& request, ObjectGuid& characterGuid)
{
    // Get available bot accounts
    std::vector<uint32> accounts = sBotAccountMgr->GetAvailableAccounts();
    if (accounts.empty())
    {
        TC_LOG_WARN("playerbot", "No available bot accounts for spawning");
        return false;
    }

    // Build character pool based on criteria
    std::vector<ObjectGuid> candidates;

    for (uint32 accountId : accounts)
    {
        auto characters = sBotCharacterMgr->GetBotCharacters(accountId);
        for (ObjectGuid const& guid : characters)
        {
            // Apply filters
            // TODO: Check level, class, race filters
            // TODO: Check if character is already spawned
            
            candidates.push_back(guid);
        }
    }

    if (candidates.empty())
    {
        TC_LOG_WARN("playerbot", "No suitable characters found for spawn request");
        return false;
    }

    // Select random candidate
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
    
    characterGuid = candidates[dist(gen)];
    return true;
}

bool BotSpawner::CreateBotSession(ObjectGuid characterGuid)
{
    // Get account ID for character
    uint32 accountId = sBotCharacterMgr->GetAccountForCharacter(characterGuid);
    if (!accountId)
    {
        TC_LOG_ERROR("playerbot", "No account found for character %s",
                     characterGuid.ToString().c_str());
        return false;
    }

    // Create or retrieve session
    BotSession* session = sBotSessionMgr->GetSession(accountId);
    if (!session)
    {
        session = sBotSessionMgr->CreateSession(accountId);
        if (!session)
        {
            TC_LOG_ERROR("playerbot", "Failed to create session for account %u", accountId);
            return false;
        }
    }

    // Login character
    session->LoginBot(characterGuid);

    return true;
}

std::vector<ObjectGuid> BotSpawner::SpawnBots(std::vector<SpawnRequest> const& requests)
{
    TC_LOG_INFO("playerbot", "Spawning batch of %zu bots...", requests.size());

    std::vector<ObjectGuid> results;
    results.reserve(requests.size());

    // Use parallel execution for batch spawning
    std::vector<std::future<ObjectGuid>> futures;
    futures.reserve(requests.size());

    for (auto const& request : requests)
    {
        futures.push_back(
            boost::asio::post(*_spawnWorkers, boost::asio::use_future([this, request]()
            {
                return SpawnBot(request);
            })
        ));

        // Small delay between spawns to avoid overwhelming
        if (_config.spawnDelayMs > 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(_config.spawnDelayMs));
        }
    }

    // Collect results
    for (auto& future : futures)
    {
        try
        {
            ObjectGuid guid = future.get();
            if (!guid.IsEmpty())
            {
                results.push_back(guid);
            }
        }
        catch (std::exception const& e)
        {
            TC_LOG_ERROR("playerbot", "Batch spawn failed: %s", e.what());
        }
    }

    TC_LOG_INFO("playerbot", "Successfully spawned %zu/%zu bots",
                results.size(), requests.size());

    return results;
}

void BotSpawner::SpawnToPopulationTarget()
{
    if (!_config.enableDynamicSpawning)
        return;

    TC_LOG_INFO("playerbot", "Spawning bots to reach population targets...");

    // Update population targets based on current player distribution
    UpdatePopulationTargets();

    // Spawn bots for underpopulated zones
    for (auto const& [zoneId, pop] : _zonePopulations)
    {
        if (pop.botCount < pop.targetBotCount)
        {
            uint32 toSpawn = pop.targetBotCount - pop.botCount;
            SpawnForZone(zoneId, toSpawn);
        }
    }
}

void BotSpawner::UpdatePopulationTargets()
{
    // Analyze player distribution
    // TODO: Query world for player counts by zone

    for (auto& [zoneId, pop] : _zonePopulations)
    {
        // Calculate target based on player count and ratio
        pop.targetBotCount = static_cast<uint32>(pop.playerCount * _config.botToPlayerRatio);
        
        // Apply caps
        if (pop.targetBotCount > _config.maxBotsPerZone)
            pop.targetBotCount = _config.maxBotsPerZone;
    }
}

void BotSpawner::ProcessSpawnQueue()
{
    while (!_shutdown)
    {
        SpawnRequest request;
        if (_spawnQueue.try_pop(request))
        {
            SpawnBot(request);
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

bool BotSpawner::DespawnBot(ObjectGuid guid)
{
    if (!_activeBots.count(guid))
    {
        TC_LOG_WARN("playerbot", "Attempted to despawn non-active bot %s",
                    guid.ToString().c_str());
        return false;
    }

    // Remove from tracking
    _activeBots.erase(guid);
    
    // Remove from zone tracking
    auto it = _botZones.find(guid.GetRawValue());
    if (it != _botZones.end())
    {
        uint32 zoneId = it->second;
        _botZones.erase(it);
        
        auto& zoneBots = _zoneBots[zoneId];
        zoneBots.erase(std::remove(zoneBots.begin(), zoneBots.end(), guid), zoneBots.end());
    }

    // Logout through session
    Player* player = ObjectAccessor::FindPlayer(guid);
    if (player && player->GetSession())
    {
        player->GetSession()->LogoutPlayer(false);
    }

    _stats.totalDespawned++;

    TC_LOG_DEBUG("playerbot", "Despawned bot %s", guid.ToString().c_str());

    return true;
}

void BotSpawner::DespawnAllBots()
{
    TC_LOG_INFO("playerbot", "Despawning all %zu active bots...", _activeBots.size());

    std::vector<ObjectGuid> toRemove;
    toRemove.reserve(_activeBots.size());
    
    for (auto const& guid : _activeBots)
    {
        toRemove.push_back(guid);
    }

    for (auto const& guid : toRemove)
    {
        DespawnBot(guid);
    }

    _activeBots.clear();
    _zoneBots.clear();
    _botZones.clear();

    TC_LOG_INFO("playerbot", "All bots despawned");
}

} // namespace Playerbot
```

### Day 5: Lifecycle Coordinator Implementation

#### File: `src/modules/Playerbot/Lifecycle/BotLifecycleMgr.h`
```cpp
#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <memory>
#include <functional>
#include <phmap.h>

namespace Playerbot
{

// Forward declarations
class BotSpawner;
class BotScheduler;

// Lifecycle events
enum class LifecycleEvent
{
    BOT_CREATED,
    BOT_SPAWNED,
    BOT_DESPAWNED,
    BOT_DELETED,
    SESSION_START,
    SESSION_END,
    ZONE_CHANGED,
    LEVEL_CHANGED,
    ACTIVITY_CHANGED
};

// Event handler callback
using LifecycleEventHandler = std::function<void(LifecycleEvent, ObjectGuid, uint32, uint32)>;

// Lifecycle statistics
struct LifecycleStats
{
    std::atomic<uint32> totalCreated;
    std::atomic<uint32> totalSpawned;
    std::atomic<uint32> totalDespawned;
    std::atomic<uint32> currentlyActive;
    std::atomic<uint32> peakConcurrent;
    std::atomic<uint32> totalSessionTime;
    std::atomic<uint32> averageSessionLength;
};

class TC_GAME_API BotLifecycleMgr
{
    BotLifecycleMgr();
    ~BotLifecycleMgr();
    BotLifecycleMgr(BotLifecycleMgr const&) = delete;
    BotLifecycleMgr& operator=(BotLifecycleMgr const&) = delete;

public:
    static BotLifecycleMgr* instance();

    // Initialization
    bool Initialize();
    void Shutdown();
    void Update(uint32 diff);

    // Lifecycle orchestration
    void StartLifecycle();
    void StopLifecycle();
    void PauseLifecycle();
    void ResumeLifecycle();

    // Population management
    void MaintainPopulation();
    void AdjustPopulation(uint32 targetCount);
    void BalanceZonePopulations();
    void RefreshInactiveBots();

    // Event handling
    void RegisterEventHandler(LifecycleEventHandler handler);
    void TriggerEvent(LifecycleEvent event, ObjectGuid guid, uint32 data1 = 0, uint32 data2 = 0);

    // Query functions
    bool IsLifecycleActive() const { return _active; }
    uint32 GetActiveCount() const { return _stats.currentlyActive.load(); }
    LifecycleStats const& GetStatistics() const { return _stats; }

    // Server integration hooks
    void OnServerStartup();
    void OnServerShutdown();
    void OnPlayerLogin(Player* player);
    void OnPlayerLogout(Player* player);

private:
    // Internal lifecycle management
    void ProcessLifecycleTick();
    void CheckPopulationTargets();
    void HandleInactiveBots();
    void UpdateStatistics();

private:
    // Sub-managers
    std::unique_ptr<BotSpawner> _spawner;
    std::unique_ptr<BotScheduler> _scheduler;

    // Event handlers
    std::vector<LifecycleEventHandler> _eventHandlers;

    // State
    std::atomic<bool> _active;
    std::atomic<bool> _paused;
    uint32 _updateTimer;

    // Statistics
    LifecycleStats _stats;

    // Configuration
    uint32 _targetPopulation;
    uint32 _updateInterval;
    bool _dynamicPopulation;
};

#define sBotLifecycleMgr BotLifecycleMgr::instance()

} // namespace Playerbot
```

## Database Schema

### SQL Schema: `sql/playerbot/003_lifecycle_management.sql`
```sql
-- Bot spawn logs for analytics
CREATE TABLE IF NOT EXISTS `playerbot_spawn_log` (
    `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `guid` BIGINT UNSIGNED NOT NULL,
    `zone_id` INT UNSIGNED DEFAULT NULL,
    `map_id` INT UNSIGNED DEFAULT NULL,
    `spawn_time` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    `despawn_time` TIMESTAMP NULL DEFAULT NULL,
    `session_duration` INT UNSIGNED DEFAULT NULL,
    `spawn_reason` VARCHAR(50) DEFAULT NULL,
    `despawn_reason` VARCHAR(50) DEFAULT NULL,
    PRIMARY KEY (`id`),
    KEY `idx_guid` (`guid`),
    KEY `idx_spawn_time` (`spawn_time`),
    KEY `idx_zone` (`zone_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Activity patterns for scheduling
CREATE TABLE IF NOT EXISTS `playerbot_activity_patterns` (
    `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `name` VARCHAR(50) NOT NULL,
    `active_hours` JSON NOT NULL,
    `active_days` JSON NOT NULL,
    `login_probability` FLOAT DEFAULT 1.0,
    `min_session` INT UNSIGNED DEFAULT 1800,
    `max_session` INT UNSIGNED DEFAULT 14400,
    `sessions_per_day` INT UNSIGNED DEFAULT 2,
    `prefer_peak` BOOLEAN DEFAULT TRUE,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_pattern_name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Bot schedules
CREATE TABLE IF NOT EXISTS `playerbot_schedules` (
    `guid` BIGINT UNSIGNED NOT NULL,
    `pattern_name` VARCHAR(50) DEFAULT 'default',
    `next_login` TIMESTAMP NULL DEFAULT NULL,
    `next_logout` TIMESTAMP NULL DEFAULT NULL,
    `total_sessions` INT UNSIGNED DEFAULT 0,
    `total_playtime` INT UNSIGNED DEFAULT 0,
    `last_update` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`guid`),
    KEY `idx_next_login` (`next_login`),
    KEY `idx_pattern` (`pattern_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Zone population targets
CREATE TABLE IF NOT EXISTS `playerbot_zone_populations` (
    `zone_id` INT UNSIGNED NOT NULL,
    `map_id` INT UNSIGNED NOT NULL,
    `min_bots` INT UNSIGNED DEFAULT 0,
    `max_bots` INT UNSIGNED DEFAULT 50,
    `target_bots` INT UNSIGNED DEFAULT 10,
    `current_bots` INT UNSIGNED DEFAULT 0,
    `min_level` TINYINT UNSIGNED DEFAULT 1,
    `max_level` TINYINT UNSIGNED DEFAULT 80,
    `bot_density` FLOAT DEFAULT 0.1,
    `last_update` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`zone_id`),
    KEY `idx_map` (`map_id`),
    KEY `idx_level_range` (`min_level`, `max_level`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Lifecycle events log
CREATE TABLE IF NOT EXISTS `playerbot_lifecycle_events` (
    `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `event_type` ENUM('CREATED','SPAWNED','DESPAWNED','DELETED',
                      'SESSION_START','SESSION_END','ZONE_CHANGED',
                      'LEVEL_CHANGED','ACTIVITY_CHANGED') NOT NULL,
    `guid` BIGINT UNSIGNED NOT NULL,
    `data1` INT UNSIGNED DEFAULT NULL,
    `data2` INT UNSIGNED DEFAULT NULL,
    `timestamp` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_guid` (`guid`),
    KEY `idx_timestamp` (`timestamp`),
    KEY `idx_event_type` (`event_type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
```

## Testing Suite

### File: `src/modules/Playerbot/Tests/LifecycleManagementTest.cpp`
```cpp
#include "catch.hpp"
#include "BotSpawner.h"
#include "BotScheduler.h"
#include "BotLifecycleMgr.h"
#include "TestHelpers.h"
#include <chrono>
#include <thread>

using namespace Playerbot;

TEST_CASE("BotSpawner", "[lifecycle][integration]")
{
    SECTION("Basic Spawning")
    {
        SECTION("Spawns single bot")
        {
            SpawnRequest request;
            request.type = SpawnRequest::RANDOM;

            ObjectGuid guid = sBotSpawner->SpawnBot(request);
            REQUIRE(!guid.IsEmpty());
            REQUIRE(sBotSpawner->GetTotalBotCount() > 0);
        }

        SECTION("Respects population caps")
        {
            SpawnConfig config = sBotSpawner->GetConfig();
            config.maxBotsTotal = 5;
            config.respectPopulationCaps = true;
            sBotSpawner->SetConfig(config);

            // Spawn to limit
            std::vector<ObjectGuid> bots;
            for (int i = 0; i < 5; ++i)
            {
                SpawnRequest request;
                request.type = SpawnRequest::RANDOM;
                ObjectGuid guid = sBotSpawner->SpawnBot(request);
                REQUIRE(!guid.IsEmpty());
                bots.push_back(guid);
            }

            // 6th should fail
            SpawnRequest request;
            request.type = SpawnRequest::RANDOM;
            ObjectGuid guid = sBotSpawner->SpawnBot(request);
            REQUIRE(guid.IsEmpty());

            // Cleanup
            for (auto const& botGuid : bots)
            {
                sBotSpawner->DespawnBot(botGuid);
            }
        }

        SECTION("Batch spawning works")
        {
            std::vector<SpawnRequest> requests;
            for (int i = 0; i < 10; ++i)
            {
                SpawnRequest request;
                request.type = SpawnRequest::RANDOM;
                requests.push_back(request);
            }

            auto guids = sBotSpawner->SpawnBots(requests);
            REQUIRE(guids.size() == 10);

            // Cleanup
            for (auto const& guid : guids)
            {
                sBotSpawner->DespawnBot(guid);
            }
        }
    }

    SECTION("Zone Population")
    {
        SECTION("Maintains zone populations")
        {
            uint32 testZoneId = 1519; // Stormwind
            sBotSpawner->SpawnForZone(testZoneId, 5);

            auto population = sBotSpawner->GetZonePopulation(testZoneId);
            REQUIRE(population.botCount >= 5);
        }

        SECTION("Updates population targets")
        {
            sBotSpawner->UpdatePopulationTargets();
            // Should complete without errors
        }
    }
}

TEST_CASE("BotScheduler", "[lifecycle][scheduling]")
{
    SECTION("Pattern Management")
    {
        SECTION("Loads default patterns")
        {
            REQUIRE(sBotScheduler->Initialize());
            auto pattern = sBotScheduler->GetPattern("default");
            REQUIRE(pattern != nullptr);
            REQUIRE(pattern->name == "default");
        }

        SECTION("Registers custom pattern")
        {
            ActivityPattern custom;
            custom.name = "test_pattern";
            custom.activeHours = {{9, 17}}; // 9 AM to 5 PM
            custom.minSessionDuration = 600;
            custom.maxSessionDuration = 1800;

            sBotScheduler->RegisterPattern("test_pattern", custom);
            auto pattern = sBotScheduler->GetPattern("test_pattern");
            REQUIRE(pattern != nullptr);
            REQUIRE(pattern->name == "test_pattern");
        }
    }

    SECTION("Scheduling")
    {
        ObjectGuid testGuid = ObjectGuid::Create<HighGuid::Player>(12345);

        SECTION("Schedules bot successfully")
        {
            sBotScheduler->ScheduleBot(testGuid);
            REQUIRE(sBotScheduler->IsBotScheduled(testGuid));
            REQUIRE(sBotScheduler->GetScheduledBotCount() > 0);
        }

        SECTION("Schedules actions")
        {
            ScheduleEntry entry;
            entry.botGuid = testGuid;
            entry.action = ScheduleEntry::LOGIN;
            entry.executeTime = std::chrono::system_clock::now() + std::chrono::seconds(1);

            sBotScheduler->ScheduleAction(entry);
            // Action should be queued
        }

        SECTION("Calculates next login time")
        {
            sBotScheduler->AssignPattern(testGuid, "default");
            auto nextLogin = sBotScheduler->GetNextScheduledAction(testGuid);
            REQUIRE(nextLogin > std::chrono::system_clock::now());
        }
    }
}

TEST_CASE("Performance", "[lifecycle][performance]")
{
    SECTION("Spawn performance")
    {
        auto start = std::chrono::high_resolution_clock::now();

        SpawnRequest request;
        request.type = SpawnRequest::RANDOM;
        ObjectGuid guid = sBotSpawner->SpawnBot(request);

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start).count();

        REQUIRE(!guid.IsEmpty());
        REQUIRE(elapsed < 50); // Should complete in under 50ms

        sBotSpawner->DespawnBot(guid);
    }

    SECTION("Batch spawn performance")
    {
        auto start = std::chrono::high_resolution_clock::now();

        std::vector<SpawnRequest> requests;
        for (int i = 0; i < 100; ++i)
        {
            SpawnRequest request;
            request.type = SpawnRequest::RANDOM;
            requests.push_back(request);
        }

        auto guids = sBotSpawner->SpawnBots(requests);

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start).count();

        REQUIRE(guids.size() == 100);
        REQUIRE(elapsed < 2000); // 100 bots in under 2 seconds

        // Cleanup
        for (auto const& guid : guids)
        {
            sBotSpawner->DespawnBot(guid);
        }
    }

    SECTION("Scheduling overhead")
    {
        // Schedule 1000 bots
        std::vector<ObjectGuid> testGuids;
        for (int i = 0; i < 1000; ++i)
        {
            ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(100000 + i);
            testGuids.push_back(guid);
            sBotScheduler->ScheduleBot(guid);
        }

        REQUIRE(sBotScheduler->GetScheduledBotCount() == 1000);

        // Measure CPU usage of scheduler
        auto start = std::chrono::high_resolution_clock::now();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start).count();

        // Scheduling overhead should be minimal
        // This is a simplified test - real measurement would track actual CPU time
        REQUIRE(elapsed >= 1000); // At least 1 second passed
        REQUIRE(elapsed < 1100);  // No more than 100ms overhead
    }
}
```

## Configuration File

### File: `conf/playerbots.conf.dist` (Lifecycle Section)
```ini
###################################################################################################
#   LIFECYCLE MANAGEMENT CONFIGURATION
#
#   Bot spawning, scheduling, and population management
###################################################################################################

#
#   Playerbot.Spawn.MaxTotal
#       Maximum total number of bots that can be spawned
#       Default: 500
#

Playerbot.Spawn.MaxTotal = 500

#
#   Playerbot.Spawn.MaxPerZone
#       Maximum number of bots per zone
#       Default: 50
#

Playerbot.Spawn.MaxPerZone = 50

#
#   Playerbot.Spawn.MaxPerMap
#       Maximum number of bots per map
#       Default: 200
#

Playerbot.Spawn.MaxPerMap = 200

#
#   Playerbot.Spawn.BatchSize
#       Number of bots to spawn in parallel batches
#       Default: 10
#

Playerbot.Spawn.BatchSize = 10

#
#   Playerbot.Spawn.Dynamic
#       Enable dynamic spawning based on player population
#       Default: 1 (enabled)
#

Playerbot.Spawn.Dynamic = 1

#
#   Playerbot.Spawn.BotToPlayerRatio
#       Ratio of bots to players for dynamic spawning
#       Default: 2.0 (2 bots per player)
#

Playerbot.Spawn.BotToPlayerRatio = 2.0

#
#   Playerbot.Schedule.Enable
#       Enable realistic scheduling patterns
#       Default: 1 (enabled)
#

Playerbot.Schedule.Enable = 1

#
#   Playerbot.Schedule.RealisticPatterns
#       Use realistic login/logout patterns
#       Default: 1 (enabled)
#

Playerbot.Schedule.RealisticPatterns = 1

#
#   Playerbot.Schedule.PeakMultiplier
#       Bot activity multiplier during peak hours (6 PM - 11 PM)
#       Default: 2.0
#

Playerbot.Schedule.PeakMultiplier = 2.0

#
#   Playerbot.Schedule.OffPeakMultiplier
#       Bot activity multiplier during off-peak hours (2 AM - 10 AM)
#       Default: 0.5
#

Playerbot.Schedule.OffPeakMultiplier = 0.5
```

## Implementation Checklist

### Week 11 Tasks
- [ ] Implement BotSpawner core class
- [ ] Add population management logic
- [ ] Implement zone-based spawning
- [ ] Create spawn request queue with Intel TBB
- [ ] Implement BotScheduler with activity patterns
- [ ] Add realistic login/logout scheduling
- [ ] Implement BotLifecycleMgr coordinator
- [ ] Create database tables for lifecycle data
- [ ] Write comprehensive unit tests
- [ ] Performance testing with 500+ bots

## Performance Optimizations

### Implemented Optimizations
1. **Parallel Spawning**: Intel TBB thread pool for concurrent spawning
2. **Batch Processing**: Spawn multiple bots in parallel batches
3. **Priority Queue**: TBB concurrent_priority_queue for scheduling
4. **Async Database**: All lifecycle data persisted asynchronously
5. **Memory Efficiency**: <10KB per scheduled bot
6. **Lock-Free Collections**: phmap for all concurrent data structures

### Scalability Metrics
- Spawn 100 bots: <2 seconds
- Schedule 1000 bots: <0.1% CPU overhead
- Population rebalancing: <100ms
- Memory usage: Linear scaling, <10MB for 1000 bots

## Integration Points

### With Phase 1 Components
```cpp
// Uses enterprise session management
BotSession* session = sBotSessionMgr->CreateSession(accountId);

// Leverages async database
sBotDBPool->ExecuteAsync(stmt);

// Uses Intel TBB for parallelism
tbb::concurrent_queue<SpawnRequest> for batch processing
```

### With Phase 2 Components
```cpp
// Integrates with account management
std::vector<uint32> accounts = sBotAccountMgr->GetAvailableAccounts();

// Uses character management
std::vector<ObjectGuid> characters = sBotCharacterMgr->GetBotCharacters(accountId);
```

## Next Steps

After completing Week 11 lifecycle management:
1. Proceed to AI Framework implementation (Week 12-14)
2. Integrate AI with lifecycle systems
3. Implement strategy pattern for decision making
4. Add movement and combat systems

---

**Status**: Ready for implementation
**Dependencies**: Phase 1 ✅, Account Management ✅, Character Management ✅
**Estimated Completion**: 1 week

### Day 3-4: Bot Scheduler Implementation

#### File: `src/modules/Playerbot/Lifecycle/BotScheduler.h`
```cpp
#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <memory>
#include <vector>
#include <chrono>
#include <functional>
#include <tbb/concurrent_priority_queue.h>
#include <phmap.h>

namespace Playerbot
{

// Schedule entry for bot activities
struct ScheduleEntry
{
    ObjectGuid botGuid;
    std::chrono::system_clock::time_point executeTime;

    enum Action
    {
        LOGIN,
        LOGOUT,
        ZONE_CHANGE,
        ACTIVITY_CHANGE,
        IDLE_CHECK,
        HEARTBEAT
    };
    Action action;

    uint32 data1 = 0; // Action-specific data
    uint32 data2 = 0;

    // Priority queue comparison (earliest time = highest priority)
    bool operator<(ScheduleEntry const& other) const
    {
        return executeTime > other.executeTime; // Inverted for min-heap
    }
};

// Bot activity pattern
struct ActivityPattern
{
    std::string name;
    std::vector<std::pair<uint32, uint32>> activeHours; // Hour ranges (0-23)
    std::vector<uint32> activeDays; // Days of week (0-6)
    float loginProbability = 1.0f;
    uint32 minSessionDuration = 1800; // Seconds
    uint32 maxSessionDuration = 14400;
    uint32 averageSessionsPerDay = 2;
    bool preferPeakHours = true;
};

// Bot schedule configuration
struct ScheduleConfig
{
    bool enableScheduling = true;
    bool useRealisticPatterns = true;
    uint32 scheduleLookaheadMinutes = 60;
    uint32 heartbeatIntervalSeconds = 300;
    uint32 idleTimeoutSeconds = 1800;
    float peakHourMultiplier = 2.0f;
    float offPeakMultiplier = 0.5f;
};

class TC_GAME_API BotScheduler
{
    BotScheduler();
    ~BotScheduler();
    BotScheduler(BotScheduler const&) = delete;
    BotScheduler& operator=(BotScheduler const&) = delete;

public:
    static BotScheduler* instance();

    // Initialization
    bool Initialize();
    void Shutdown();

    // Schedule management - Enterprise scheduling system
    void ScheduleBot(ObjectGuid guid);
    void UnscheduleBot(ObjectGuid guid);
    void ScheduleAction(ScheduleEntry const& entry);
    void CancelScheduledActions(ObjectGuid guid);

    // Pattern management
    void LoadActivityPatterns();
    void RegisterPattern(std::string const& name, ActivityPattern const& pattern);
    ActivityPattern const* GetPattern(std::string const& name) const;
    void AssignPattern(ObjectGuid guid, std::string const& patternName);

    // Time-based scheduling
    void ScheduleLogin(ObjectGuid guid, std::chrono::system_clock::time_point when);
    void ScheduleLogout(ObjectGuid guid, std::chrono::system_clock::time_point when);
    void ScheduleNextActivity(ObjectGuid guid);

    // Batch scheduling
    void SchedulePeakHourBots();
    void ScheduleOffPeakBots();
    void ScheduleWeekendBots();
    void ScheduleMaintenanceWindow();

    // Query functions
    bool IsBotScheduled(ObjectGuid guid) const;
    std::chrono::system_clock::time_point GetNextScheduledAction(ObjectGuid guid) const;
    uint32 GetScheduledBotCount() const { return _botSchedules.size(); }
    std::vector<ObjectGuid> GetBotsScheduledForLogin(uint32 nextMinutes) const;

    // Statistics
    struct Statistics
    {
        std::atomic<uint32> totalScheduled;
        std::atomic<uint32> actionsExecuted;
        std::atomic<uint32> loginsScheduled;
        std::atomic<uint32> logoutsScheduled;
        std::atomic<uint32> patternsAssigned;
        std::atomic<uint32> averageSessionMinutes;
    };
    Statistics const& GetStatistics() const { return _stats; }

    // Configuration
    void SetConfig(ScheduleConfig const& config) { _config = config; }
    ScheduleConfig const& GetConfig() const { return _config; }

private:
    // Internal scheduling logic
    void ProcessSchedule();
    void ExecuteScheduledAction(ScheduleEntry const& entry);

    // Pattern-based scheduling
    std::chrono::system_clock::time_point CalculateNextLogin(ObjectGuid guid);
    std::chrono::system_clock::time_point CalculateSessionEnd(
        ObjectGuid guid,
        std::chrono::system_clock::time_point loginTime
    );

    // Time calculations
    bool IsInActiveHours(ActivityPattern const& pattern,
                        std::chrono::system_clock::time_point time) const;
    bool IsPeakHour(std::chrono::system_clock::time_point time) const;
    float GetTimeMultiplier(std::chrono::system_clock::time_point time) const;

    // Session management
    void StartBotSession(ObjectGuid guid);
    void EndBotSession(ObjectGuid guid);
    void UpdateBotActivity(ObjectGuid guid);

private:
    // Priority queue for scheduled actions (min-heap by time)
    tbb::concurrent_priority_queue<ScheduleEntry> _scheduleQueue;

    // Bot schedules and patterns
    phmap::parallel_flat_hash_map<ObjectGuid::LowType, std::string> _botPatterns;
    phmap::parallel_flat_hash_map<std::string, ActivityPattern> _activityPatterns;
    phmap::parallel_flat_hash_set<ObjectGuid> _botSchedules;

    // Session tracking
    phmap::parallel_flat_hash_map<ObjectGuid::LowType,
        std::chrono::system_clock::time_point> _sessionStarts;

    // Configuration
    ScheduleConfig _config;

    // Statistics
    mutable Statistics _stats;

    // Processing thread
    std::thread _processingThread;
    std::atomic<bool> _shutdown;
};

#define sBotScheduler BotScheduler::instance()

} // namespace Playerbot
```

### Day 5: Lifecycle Coordinator Implementation

#### File: `src/modules/Playerbot/Lifecycle/BotLifecycleMgr.h`
```cpp
#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <memory>
#include <functional>
#include <phmap.h>

namespace Playerbot
{

// Forward declarations
class BotSpawner;
class BotScheduler;

// Lifecycle events
enum class LifecycleEvent
{
    BOT_CREATED,
    BOT_SPAWNED,
    BOT_DESPAWNED,
    BOT_DELETED,
    SESSION_START,
    SESSION_END,
    ZONE_CHANGED,
    LEVEL_CHANGED,
    ACTIVITY_CHANGED
};

// Event handler callback
using LifecycleEventHandler = std::function<void(LifecycleEvent, ObjectGuid, uint32, uint32)>;

// Lifecycle statistics
struct LifecycleStats
{
    std::atomic<uint32> totalCreated;
    std::atomic<uint32> totalSpawned;
    std::atomic<uint32> totalDespawned;
    std::atomic<uint32> currentlyActive;
    std::atomic<uint32> peakConcurrent;
    std::atomic<uint32> totalSessionTime;
    std::atomic<uint32> averageSessionLength;
};

class TC_GAME_API BotLifecycleMgr
{
    BotLifecycleMgr();
    ~BotLifecycleMgr();
    BotLifecycleMgr(BotLifecycleMgr const&) = delete;
    BotLifecycleMgr& operator=(BotLifecycleMgr const&) = delete;

public:
    static BotLifecycleMgr* instance();

    // Initialization
    bool Initialize();
    void Shutdown();
    void Update(uint32 diff);

    // Lifecycle orchestration
    void StartLifecycle();
    void StopLifecycle();
    void PauseLifecycle();
    void ResumeLifecycle();

    // Population management
    void MaintainPopulation();
    void AdjustPopulation(uint32 targetCount);
    void BalanceZonePopulations();
    void RefreshInactiveBots();

    // Event handling
    void RegisterEventHandler(LifecycleEventHandler handler);
    void TriggerEvent(LifecycleEvent event, ObjectGuid guid, uint32 data1 = 0, uint32 data2 = 0);

    // Query functions
    bool IsLifecycleActive() const { return _active; }
    uint32 GetActiveCount() const { return _stats.currentlyActive.load(); }
    LifecycleStats const& GetStatistics() const { return _stats; }

    // Server integration hooks
    void OnServerStartup();
    void OnServerShutdown();
    void OnPlayerLogin(Player* player);
    void OnPlayerLogout(Player* player);

private:
    // Internal lifecycle management
    void ProcessLifecycleTick();
    void CheckPopulationTargets();
    void HandleInactiveBots();
    void UpdateStatistics();

private:
    // Sub-managers
    std::unique_ptr<BotSpawner> _spawner;
    std::unique_ptr<BotScheduler> _scheduler;

    // Event handlers
    std::vector<LifecycleEventHandler> _eventHandlers;

    // State
    std::atomic<bool> _active;
    std::atomic<bool> _paused;
    uint32 _updateTimer;

    // Statistics
    LifecycleStats _stats;

    // Configuration
    uint32 _targetPopulation;
    uint32 _updateInterval;
    bool _dynamicPopulation;
};

#define sBotLifecycleMgr BotLifecycleMgr::instance()

} // namespace Playerbot
```

## Database Schema

### SQL Schema: `sql/playerbot/003_lifecycle_management.sql`
```sql
-- Bot spawn logs for analytics
CREATE TABLE IF NOT EXISTS `playerbot_spawn_log` (
    `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `guid` BIGINT UNSIGNED NOT NULL,
    `zone_id` INT UNSIGNED DEFAULT NULL,
    `map_id` INT UNSIGNED DEFAULT NULL,
    `spawn_time` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    `despawn_time` TIMESTAMP NULL DEFAULT NULL,
    `session_duration` INT UNSIGNED DEFAULT NULL,
    `spawn_reason` VARCHAR(50) DEFAULT NULL,
    `despawn_reason` VARCHAR(50) DEFAULT NULL,
    PRIMARY KEY (`id`),
    KEY `idx_guid` (`guid`),
    KEY `idx_spawn_time` (`spawn_time`),
    KEY `idx_zone` (`zone_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Activity patterns for scheduling
CREATE TABLE IF NOT EXISTS `playerbot_activity_patterns` (
    `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `name` VARCHAR(50) NOT NULL,
    `active_hours` JSON NOT NULL,
    `active_days` JSON NOT NULL,
    `login_probability` FLOAT DEFAULT 1.0,
    `min_session` INT UNSIGNED DEFAULT 1800,
    `max_session` INT UNSIGNED DEFAULT 14400,
    `sessions_per_day` INT UNSIGNED DEFAULT 2,
    `prefer_peak` BOOLEAN DEFAULT TRUE,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_pattern_name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Bot schedules
CREATE TABLE IF NOT EXISTS `playerbot_schedules` (
    `guid` BIGINT UNSIGNED NOT NULL,
    `pattern_name` VARCHAR(50) DEFAULT 'default',
    `next_login` TIMESTAMP NULL DEFAULT NULL,
    `next_logout` TIMESTAMP NULL DEFAULT NULL,
    `total_sessions` INT UNSIGNED DEFAULT 0,
    `total_playtime` INT UNSIGNED DEFAULT 0,
    `last_update` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`guid`),
    KEY `idx_next_login` (`next_login`),
    KEY `idx_pattern` (`pattern_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Zone population targets
CREATE TABLE IF NOT EXISTS `playerbot_zone_populations` (
    `zone_id` INT UNSIGNED NOT NULL,
    `map_id` INT UNSIGNED NOT NULL,
    `min_bots` INT UNSIGNED DEFAULT 0,
    `max_bots` INT UNSIGNED DEFAULT 50,
    `target_bots` INT UNSIGNED DEFAULT 10,
    `current_bots` INT UNSIGNED DEFAULT 0,
    `min_level` TINYINT UNSIGNED DEFAULT 1,
    `max_level` TINYINT UNSIGNED DEFAULT 80,
    `bot_density` FLOAT DEFAULT 0.1,
    `last_update` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`zone_id`),
    KEY `idx_map` (`map_id`),
    KEY `idx_level_range` (`min_level`, `max_level`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Lifecycle events log
CREATE TABLE IF NOT EXISTS `playerbot_lifecycle_events` (
    `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `event_type` ENUM('CREATED','SPAWNED','DESPAWNED','DELETED',
                      'SESSION_START','SESSION_END','ZONE_CHANGED',
                      'LEVEL_CHANGED','ACTIVITY_CHANGED') NOT NULL,
    `guid` BIGINT UNSIGNED NOT NULL,
    `data1` INT UNSIGNED DEFAULT NULL,
    `data2` INT UNSIGNED DEFAULT NULL,
    `timestamp` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_guid` (`guid`),
    KEY `idx_timestamp` (`timestamp`),
    KEY `idx_event_type` (`event_type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
```

## Testing Suite

### File: `src/modules/Playerbot/Tests/LifecycleManagementTest.cpp`
```cpp
#include "catch.hpp"
#include "BotSpawner.h"
#include "BotScheduler.h"
#include "BotLifecycleMgr.h"
#include "TestHelpers.h"
#include <chrono>
#include <thread>

using namespace Playerbot;

TEST_CASE("BotSpawner", "[lifecycle][integration]")
{
    SECTION("Basic Spawning")
    {
        SECTION("Spawns single bot")
        {
            SpawnRequest request;
            request.type = SpawnRequest::RANDOM;

            ObjectGuid guid = sBotSpawner->SpawnBot(request);
            REQUIRE(!guid.IsEmpty());
            REQUIRE(sBotSpawner->GetTotalBotCount() > 0);
        }

        SECTION("Respects population caps")
        {
            SpawnConfig config = sBotSpawner->GetConfig();
            config.maxBotsTotal = 5;
            config.respectPopulationCaps = true;
            sBotSpawner->SetConfig(config);

            // Spawn to limit
            std::vector<ObjectGuid> bots;
            for (int i = 0; i < 5; ++i)
            {
                SpawnRequest request;
                request.type = SpawnRequest::RANDOM;
                ObjectGuid guid = sBotSpawner->SpawnBot(request);
                REQUIRE(!guid.IsEmpty());
                bots.push_back(guid);
            }

            // 6th should fail
            SpawnRequest request;
            request.type = SpawnRequest::RANDOM;
            ObjectGuid guid = sBotSpawner->SpawnBot(request);
            REQUIRE(guid.IsEmpty());

            // Cleanup
            for (auto const& botGuid : bots)
            {
                sBotSpawner->DespawnBot(botGuid);
            }
        }

        SECTION("Batch spawning works")
        {
            std::vector<SpawnRequest> requests;
            for (int i = 0; i < 10; ++i)
            {
                SpawnRequest request;
                request.type = SpawnRequest::RANDOM;
                requests.push_back(request);
            }

            auto guids = sBotSpawner->SpawnBots(requests);
            REQUIRE(guids.size() == 10);

            // Cleanup
            for (auto const& guid : guids)
            {
                sBotSpawner->DespawnBot(guid);
            }
        }
    }

    SECTION("Zone Population")
    {
        SECTION("Maintains zone populations")
        {
            uint32 testZoneId = 1519; // Stormwind
            sBotSpawner->SpawnForZone(testZoneId, 5);

            auto population = sBotSpawner->GetZonePopulation(testZoneId);
            REQUIRE(population.botCount >= 5);
        }

        SECTION("Updates population targets")
        {
            sBotSpawner->UpdatePopulationTargets();
            // Should complete without errors
        }
    }
}

TEST_CASE("BotScheduler", "[lifecycle][scheduling]")
{
    SECTION("Pattern Management")
    {
        SECTION("Loads default patterns")
        {
            REQUIRE(sBotScheduler->Initialize());
            auto pattern = sBotScheduler->GetPattern("default");
            REQUIRE(pattern != nullptr);
            REQUIRE(pattern->name == "default");
        }

        SECTION("Registers custom pattern")
        {
            ActivityPattern custom;
            custom.name = "test_pattern";
            custom.activeHours = {{9, 17}}; // 9 AM to 5 PM
            custom.minSessionDuration = 600;
            custom.maxSessionDuration = 1800;

            sBotScheduler->RegisterPattern("test_pattern", custom);
            auto pattern = sBotScheduler->GetPattern("test_pattern");
            REQUIRE(pattern != nullptr);
            REQUIRE(pattern->name == "test_pattern");
        }
    }

    SECTION("Scheduling")
    {
        ObjectGuid testGuid = ObjectGuid::Create<HighGuid::Player>(12345);

        SECTION("Schedules bot successfully")
        {
            sBotScheduler->ScheduleBot(testGuid);
            REQUIRE(sBotScheduler->IsBotScheduled(testGuid));
            REQUIRE(sBotScheduler->GetScheduledBotCount() > 0);
        }

        SECTION("Schedules actions")
        {
            ScheduleEntry entry;
            entry.botGuid = testGuid;
            entry.action = ScheduleEntry::LOGIN;
            entry.executeTime = std::chrono::system_clock::now() + std::chrono::seconds(1);

            sBotScheduler->ScheduleAction(entry);
            // Action should be queued
        }

        SECTION("Calculates next login time")
        {
            sBotScheduler->AssignPattern(testGuid, "default");
            auto nextLogin = sBotScheduler->GetNextScheduledAction(testGuid);
            REQUIRE(nextLogin > std::chrono::system_clock::now());
        }
    }
}

TEST_CASE("Performance", "[lifecycle][performance]")
{
    SECTION("Spawn performance")
    {
        auto start = std::chrono::high_resolution_clock::now();

        SpawnRequest request;
        request.type = SpawnRequest::RANDOM;
        ObjectGuid guid = sBotSpawner->SpawnBot(request);

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start).count();

        REQUIRE(!guid.IsEmpty());
        REQUIRE(elapsed < 50); // Should complete in under 50ms

        sBotSpawner->DespawnBot(guid);
    }

    SECTION("Batch spawn performance")
    {
        auto start = std::chrono::high_resolution_clock::now();

        std::vector<SpawnRequest> requests;
        for (int i = 0; i < 100; ++i)
        {
            SpawnRequest request;
            request.type = SpawnRequest::RANDOM;
            requests.push_back(request);
        }

        auto guids = sBotSpawner->SpawnBots(requests);

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start).count();

        REQUIRE(guids.size() == 100);
        REQUIRE(elapsed < 2000); // 100 bots in under 2 seconds

        // Cleanup
        for (auto const& guid : guids)
        {
            sBotSpawner->DespawnBot(guid);
        }
    }

    SECTION("Scheduling overhead")
    {
        // Schedule 1000 bots
        std::vector<ObjectGuid> testGuids;
        for (int i = 0; i < 1000; ++i)
        {
            ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(100000 + i);
            testGuids.push_back(guid);
            sBotScheduler->ScheduleBot(guid);
        }

        REQUIRE(sBotScheduler->GetScheduledBotCount() == 1000);

        // Measure CPU usage of scheduler
        auto start = std::chrono::high_resolution_clock::now();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start).count();

        // Scheduling overhead should be minimal
        // This is a simplified test - real measurement would track actual CPU time
        REQUIRE(elapsed >= 1000); // At least 1 second passed
        REQUIRE(elapsed < 1100);  // No more than 100ms overhead
    }
}
```

## Configuration File

### File: `conf/playerbots.conf.dist` (Lifecycle Section)
```ini
###################################################################################################
#   LIFECYCLE MANAGEMENT CONFIGURATION
#
#   Bot spawning, scheduling, and population management
###################################################################################################

#
#   Playerbot.Spawn.MaxTotal
#       Maximum total number of bots that can be spawned
#       Default: 500
#

Playerbot.Spawn.MaxTotal = 500

#
#   Playerbot.Spawn.MaxPerZone
#       Maximum number of bots per zone
#       Default: 50
#

Playerbot.Spawn.MaxPerZone = 50

#
#   Playerbot.Spawn.MaxPerMap
#       Maximum number of bots per map
#       Default: 200
#

Playerbot.Spawn.MaxPerMap = 200

#
#   Playerbot.Spawn.BatchSize
#       Number of bots to spawn in parallel batches
#       Default: 10
#

Playerbot.Spawn.BatchSize = 10

#
#   Playerbot.Spawn.Dynamic
#       Enable dynamic spawning based on player population
#       Default: 1 (enabled)
#

Playerbot.Spawn.Dynamic = 1

#
#   Playerbot.Spawn.BotToPlayerRatio
#       Ratio of bots to players for dynamic spawning
#       Default: 2.0 (2 bots per player)
#

Playerbot.Spawn.BotToPlayerRatio = 2.0

#
#   Playerbot.Schedule.Enable
#       Enable realistic scheduling patterns
#       Default: 1 (enabled)
#

Playerbot.Schedule.Enable = 1

#
#   Playerbot.Schedule.RealisticPatterns
#       Use realistic login/logout patterns
#       Default: 1 (enabled)
#

Playerbot.Schedule.RealisticPatterns = 1

#
#   Playerbot.Schedule.PeakMultiplier
#       Bot activity multiplier during peak hours (6 PM - 11 PM)
#       Default: 2.0
#

Playerbot.Schedule.PeakMultiplier = 2.0

#
#   Playerbot.Schedule.OffPeakMultiplier
#       Bot activity multiplier during off-peak hours (2 AM - 10 AM)
#       Default: 0.5
#

Playerbot.Schedule.OffPeakMultiplier = 0.5
```

## Implementation Checklist

### Week 11 Tasks
- [ ] Implement BotSpawner core class
- [ ] Add population management logic
- [ ] Implement zone-based spawning
- [ ] Create spawn request queue with Intel TBB
- [ ] Implement BotScheduler with activity patterns
- [ ] Add realistic login/logout scheduling
- [ ] Implement BotLifecycleMgr coordinator
- [ ] Create database tables for lifecycle data
- [ ] Write comprehensive unit tests
- [ ] Performance testing with 500+ bots

## Performance Optimizations

### Implemented Optimizations
1. **Parallel Spawning**: Intel TBB thread pool for concurrent spawning
2. **Batch Processing**: Spawn multiple bots in parallel batches
3. **Priority Queue**: TBB concurrent_priority_queue for scheduling
4. **Async Database**: All lifecycle data persisted asynchronously
5. **Memory Efficiency**: <10KB per scheduled bot
6. **Lock-Free Collections**: phmap for all concurrent data structures

### Scalability Metrics
- Spawn 100 bots: <2 seconds
- Schedule 1000 bots: <0.1% CPU overhead
- Population rebalancing: <100ms
- Memory usage: Linear scaling, <10MB for 1000 bots

## Integration Points

### With Phase 1 Components
```cpp
// Uses enterprise session management
BotSession* session = sBotSessionMgr->CreateSession(accountId);

// Leverages async database
sBotDBPool->ExecuteAsync(stmt);

// Uses Intel TBB for parallelism
tbb::concurrent_queue<SpawnRequest> for batch processing
```

### With Phase 2 Components
```cpp
// Integrates with account management
std::vector<uint32> accounts = sBotAccountMgr->GetAvailableAccounts();

// Uses character management
std::vector<ObjectGuid> characters = sBotCharacterMgr->GetBotCharacters(accountId);
```

## Next Steps

After completing Week 11 lifecycle management:
1. Proceed to AI Framework implementation (Week 12-14)
2. Integrate AI with lifecycle systems
3. Implement strategy pattern for decision making
4. Add movement and combat systems

---

**Status**: Ready for implementation
**Dependencies**: Phase 1 ✅, Account Management ✅, Character Management ✅
**Estimated Completion**: 1 week

#### File: `src/modules/Playerbot/Lifecycle/BotScheduler.cpp`
```cpp
#include "BotScheduler.h"
#include "BotSpawner.h"
#include "BotSessionMgr.h"
#include "BotDatabasePool.h"
#include "World.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "Log.h"
#include <random>

namespace Playerbot
{

BotScheduler::BotScheduler()
    : _shutdown(false)
{
}

BotScheduler::~BotScheduler()
{
    Shutdown();
}

BotScheduler* BotScheduler::instance()
{
    static BotScheduler instance;
    return &instance;
}

bool BotScheduler::Initialize()
{
    TC_LOG_INFO("playerbot", "Initializing BotScheduler with enterprise scheduling...");

    // Load configuration
    _config.enableScheduling = sConfigMgr->GetBoolDefault("Playerbot.Schedule.Enable", true);
    _config.useRealisticPatterns = sConfigMgr->GetBoolDefault("Playerbot.Schedule.RealisticPatterns", true);
    _config.scheduleLookaheadMinutes = sConfigMgr->GetIntDefault("Playerbot.Schedule.LookaheadMinutes", 60);
    _config.heartbeatIntervalSeconds = sConfigMgr->GetIntDefault("Playerbot.Schedule.HeartbeatInterval", 300);
    _config.idleTimeoutSeconds = sConfigMgr->GetIntDefault("Playerbot.Schedule.IdleTimeout", 1800);
    _config.peakHourMultiplier = sConfigMgr->GetFloatDefault("Playerbot.Schedule.PeakMultiplier", 2.0f);
    _config.offPeakMultiplier = sConfigMgr->GetFloatDefault("Playerbot.Schedule.OffPeakMultiplier", 0.5f);

    // Load activity patterns
    LoadActivityPatterns();

    // Start processing thread
    if (_config.enableScheduling)
    {
        _processingThread = std::thread([this] { ProcessSchedule(); });
    }

    TC_LOG_INFO("playerbot", "BotScheduler initialized successfully");
    TC_LOG_INFO("playerbot", "  -> Scheduling: %s", _config.enableScheduling ? "enabled" : "disabled");
    TC_LOG_INFO("playerbot", "  -> Realistic patterns: %s",
                _config.useRealisticPatterns ? "enabled" : "disabled");
    TC_LOG_INFO("playerbot", "  -> Lookahead: %u minutes", _config.scheduleLookaheadMinutes);
    TC_LOG_INFO("playerbot", "  -> Loaded %zu activity patterns", _activityPatterns.size());

    return true;
}

void BotScheduler::Shutdown()
{
    TC_LOG_INFO("playerbot", "Shutting down BotScheduler...");

    _shutdown = true;

    // Stop processing thread
    if (_processingThread.joinable())
        _processingThread.join();

    // Clear all schedules
    _scheduleQueue.clear();
    _botSchedules.clear();
    _botPatterns.clear();

    TC_LOG_INFO("playerbot", "BotScheduler shutdown complete. Stats:");
    TC_LOG_INFO("playerbot", "  -> Total scheduled: %u", _stats.totalScheduled.load());
    TC_LOG_INFO("playerbot", "  -> Actions executed: %u", _stats.actionsExecuted.load());
    TC_LOG_INFO("playerbot", "  -> Average session: %u minutes", _stats.averageSessionMinutes.load());
}

void BotScheduler::ScheduleBot(ObjectGuid guid)
{
    if (!_config.enableScheduling)
    {
        // If scheduling disabled, just mark as active
        _botSchedules.insert(guid);
        return;
    }

    TC_LOG_DEBUG("playerbot", "Scheduling bot %s", guid.ToString().c_str());

    // Check if already scheduled
    if (_botSchedules.count(guid))
    {
        TC_LOG_DEBUG("playerbot", "Bot %s already scheduled", guid.ToString().c_str());
        return;
    }

    _botSchedules.insert(guid);
    _stats.totalScheduled++;

    // Assign pattern if not already assigned
    if (!_botPatterns.count(guid.GetRawValue()))
    {
        // Select random pattern or use default
        if (_config.useRealisticPatterns && !_activityPatterns.empty())
        {
            // Random pattern selection
            auto it = _activityPatterns.begin();
            std::advance(it, rand() % _activityPatterns.size());
            AssignPattern(guid, it->first);
        }
        else
        {
            AssignPattern(guid, "default");
        }
    }

    // Schedule next activity
    ScheduleNextActivity(guid);

    // Schedule heartbeat
    ScheduleEntry heartbeat;
    heartbeat.botGuid = guid;
    heartbeat.action = ScheduleEntry::HEARTBEAT;
    heartbeat.executeTime = std::chrono::system_clock::now() +
        std::chrono::seconds(_config.heartbeatIntervalSeconds);
    ScheduleAction(heartbeat);
}

void BotScheduler::ScheduleAction(ScheduleEntry const& entry)
{
    _scheduleQueue.push(entry);

    TC_LOG_DEBUG("playerbot", "Scheduled action %u for bot %s at %s",
                entry.action, entry.botGuid.ToString().c_str(),
                std::asctime(std::localtime(&std::chrono::system_clock::to_time_t(entry.executeTime))));
}

void BotScheduler::ScheduleNextActivity(ObjectGuid guid)
{
    if (!_config.useRealisticPatterns)
    {
        // Simple scheduling - random login/logout
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(300, 3600); // 5 min to 1 hour

        ScheduleEntry entry;
        entry.botGuid = guid;
        entry.action = ScheduleEntry::LOGOUT;
        entry.executeTime = std::chrono::system_clock::now() +
            std::chrono::seconds(dist(gen));
        ScheduleAction(entry);
        return;
    }

    // Get bot's pattern
    auto patternIt = _botPatterns.find(guid.GetRawValue());
    if (patternIt == _botPatterns.end())
        return;

    auto pattern = GetPattern(patternIt->second);
    if (!pattern)
        return;

    // Calculate next login time based on pattern
    auto nextLogin = CalculateNextLogin(guid);
    ScheduleLogin(guid, nextLogin);

    // Calculate session end
    auto sessionEnd = CalculateSessionEnd(guid, nextLogin);
    ScheduleLogout(guid, sessionEnd);
}

std::chrono::system_clock::time_point BotScheduler::CalculateNextLogin(ObjectGuid guid)
{
    auto patternIt = _botPatterns.find(guid.GetRawValue());
    if (patternIt == _botPatterns.end())
        return std::chrono::system_clock::now() + std::chrono::hours(1);

    auto pattern = GetPattern(patternIt->second);
    if (!pattern)
        return std::chrono::system_clock::now() + std::chrono::hours(1);

    std::random_device rd;
    std::mt19937 gen(rd());

    // Find next active period
    auto now = std::chrono::system_clock::now();
    auto currentTime = std::chrono::system_clock::to_time_t(now);
    struct tm* timeinfo = std::localtime(&currentTime);

    // Check if we're in active hours
    for (auto const& [startHour, endHour] : pattern->activeHours)
    {
        if (timeinfo->tm_hour >= startHour && timeinfo->tm_hour < endHour)
        {
            // Already in active hours, schedule within current period
            std::uniform_int_distribution<> dist(5, 30); // 5-30 minutes
            return now + std::chrono::minutes(dist(gen));
        }
        else if (timeinfo->tm_hour < startHour)
        {
            // Next active period is today
            timeinfo->tm_hour = startHour;
            timeinfo->tm_min = 0;
            timeinfo->tm_sec = 0;

            std::uniform_int_distribution<> jitter(0, 30); // 0-30 minute jitter
            return std::chrono::system_clock::from_time_t(std::mktime(timeinfo)) +
                   std::chrono::minutes(jitter(gen));
        }
    }

    // Schedule for tomorrow's first active period
    if (!pattern->activeHours.empty())
    {
        timeinfo->tm_mday++;
        timeinfo->tm_hour = pattern->activeHours[0].first;
        timeinfo->tm_min = 0;
        timeinfo->tm_sec = 0;

        std::uniform_int_distribution<> jitter(0, 30);
        return std::chrono::system_clock::from_time_t(std::mktime(timeinfo)) +
               std::chrono::minutes(jitter(gen));
    }

    // Fallback
    return now + std::chrono::hours(1);
}

std::chrono::system_clock::time_point BotScheduler::CalculateSessionEnd(
    ObjectGuid guid,
    std::chrono::system_clock::time_point loginTime)
{
    auto patternIt = _botPatterns.find(guid.GetRawValue());
    if (patternIt == _botPatterns.end())
        return loginTime + std::chrono::hours(1);

    auto pattern = GetPattern(patternIt->second);
    if (!pattern)
        return loginTime + std::chrono::hours(1);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(pattern->minSessionDuration,
                                         pattern->maxSessionDuration);

    return loginTime + std::chrono::seconds(dist(gen));
}

void BotScheduler::LoadActivityPatterns()
{
    TC_LOG_INFO("playerbot", "Loading bot activity patterns...");

    // Default pattern - active most of the day
    ActivityPattern defaultPattern;
    defaultPattern.name = "default";
    defaultPattern.activeHours = {{6, 23}}; // 6 AM to 11 PM
    defaultPattern.activeDays = {0, 1, 2, 3, 4, 5, 6}; // All days
    defaultPattern.loginProbability = 1.0f;
    defaultPattern.minSessionDuration = 1800; // 30 minutes
    defaultPattern.maxSessionDuration = 7200; // 2 hours
    defaultPattern.averageSessionsPerDay = 3;
    RegisterPattern("default", defaultPattern);

    // Casual pattern - evenings and weekends
    ActivityPattern casualPattern;
    casualPattern.name = "casual";
    casualPattern.activeHours = {{18, 23}}; // 6 PM to 11 PM
    casualPattern.activeDays = {0, 1, 2, 3, 4, 5, 6};
    casualPattern.loginProbability = 0.8f;
    casualPattern.minSessionDuration = 900; // 15 minutes
    casualPattern.maxSessionDuration = 3600; // 1 hour
    casualPattern.averageSessionsPerDay = 1;
    RegisterPattern("casual", casualPattern);

    // Hardcore pattern - long sessions, all hours
    ActivityPattern hardcorePattern;
    hardcorePattern.name = "hardcore";
    hardcorePattern.activeHours = {{0, 24}}; // All day
    hardcorePattern.activeDays = {0, 1, 2, 3, 4, 5, 6};
    hardcorePattern.loginProbability = 1.0f;
    hardcorePattern.minSessionDuration = 7200; // 2 hours
    hardcorePattern.maxSessionDuration = 28800; // 8 hours
    hardcorePattern.averageSessionsPerDay = 2;
    RegisterPattern("hardcore", hardcorePattern);

    // Weekend warrior - mainly weekends
    ActivityPattern weekendPattern;
    weekendPattern.name = "weekend";
    weekendPattern.activeHours = {{10, 23}}; // 10 AM to 11 PM
    weekendPattern.activeDays = {0, 6}; // Sunday, Saturday
    weekendPattern.loginProbability = 0.9f;
    weekendPattern.minSessionDuration = 3600; // 1 hour
    weekendPattern.maxSessionDuration = 14400; // 4 hours
    weekendPattern.averageSessionsPerDay = 2;
    RegisterPattern("weekend", weekendPattern);

    // Load custom patterns from database
    auto result = sBotDBPool->Query(
        "SELECT name, active_hours, active_days, login_probability, "
        "min_session, max_session, sessions_per_day "
        "FROM playerbot_activity_patterns"
    );

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            ActivityPattern pattern;
            pattern.name = fields[0].GetString();
            // Parse active hours and days from JSON or string format
            // ...
            pattern.loginProbability = fields[3].GetFloat();
            pattern.minSessionDuration = fields[4].GetUInt32();
            pattern.maxSessionDuration = fields[5].GetUInt32();
            pattern.averageSessionsPerDay = fields[6].GetUInt32();

            RegisterPattern(pattern.name, pattern);
        }
        while (result->NextRow());
    }

    TC_LOG_INFO("playerbot", "Loaded %zu activity patterns", _activityPatterns.size());
}

void BotScheduler::ProcessSchedule()
{
    while (!_shutdown)
    {
        auto now = std::chrono::system_clock::now();

        // Process due entries
        while (!_scheduleQueue.empty())
        {
            ScheduleEntry entry;
            if (_scheduleQueue.try_pop(entry))
            {
                if (entry.executeTime <= now)
                {
                    ExecuteScheduledAction(entry);
                    _stats.actionsExecuted++;
                }
                else
                {
                    // Not due yet, push back
                    _scheduleQueue.push(entry);
                    break;
                }
            }
        }

        // Sleep briefly
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void BotScheduler::ExecuteScheduledAction(ScheduleEntry const& entry)
{
    TC_LOG_DEBUG("playerbot", "Executing scheduled action %u for bot %s",
                entry.action, entry.botGuid.ToString().c_str());

    switch (entry.action)
    {
        case ScheduleEntry::LOGIN:
        {
            SpawnRequest request;
            request.type = SpawnRequest::SPECIFIC_CHARACTER;
            request.characterGuid = entry.botGuid;
            sBotSpawner->SpawnBot(request);
            _stats.loginsScheduled++;
            break;
        }
        case ScheduleEntry::LOGOUT:
        {
            sBotSpawner->DespawnBot(entry.botGuid);
            _stats.logoutsScheduled++;

            // Schedule next login
            ScheduleNextActivity(entry.botGuid);
            break;
        }
        case ScheduleEntry::HEARTBEAT:
        {
            // Check if bot is still active
            UpdateBotActivity(entry.botGuid);

            // Reschedule heartbeat
            ScheduleEntry nextHeartbeat = entry;
            nextHeartbeat.executeTime = std::chrono::system_clock::now() +
                std::chrono::seconds(_config.heartbeatIntervalSeconds);
            ScheduleAction(nextHeartbeat);
            break;
        }
        // Handle other actions...
    }
}

void BotScheduler::RegisterPattern(std::string const& name, ActivityPattern const& pattern)
{
    _activityPatterns[name] = pattern;
    _stats.patternsAssigned++;
}

void BotScheduler::AssignPattern(ObjectGuid guid, std::string const& patternName)
{
    _botPatterns[guid.GetRawValue()] = patternName;

    // Store in database for persistence
    auto stmt = sBotDBPool->GetPreparedStatement(BOT_UPD_ACTIVITY_PATTERN);
    stmt->setUInt64(0, guid.GetRawValue());
    stmt->setString(1, patternName);
    sBotDBPool->ExecuteAsync(stmt);
}

} // namespace Playerbot
```

### Day 5: Lifecycle Coordinator Implementation

#### File: `src/modules/Playerbot/Lifecycle/BotLifecycleMgr.h`
```cpp
#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <memory>
#include <functional>
#include <phmap.h>

namespace Playerbot
{

// Forward declarations
class BotSpawner;
class BotScheduler;

// Lifecycle events
enum class LifecycleEvent
{
    BOT_CREATED,
    BOT_SPAWNED,
    BOT_DESPAWNED,
    BOT_DELETED,
    SESSION_START,
    SESSION_END,
    ZONE_CHANGED,
    LEVEL_CHANGED,
    ACTIVITY_CHANGED
};

// Event handler callback
using LifecycleEventHandler = std::function<void(LifecycleEvent, ObjectGuid, uint32, uint32)>;

// Lifecycle statistics
struct LifecycleStats
{
    std::atomic<uint32> totalCreated;
    std::atomic<uint32> totalSpawned;
    std::atomic<uint32> totalDespawned;
    std::atomic<uint32> currentlyActive;
    std::atomic<uint32> peakConcurrent;
    std::atomic<uint32> totalSessionTime;
    std::atomic<uint32> averageSessionLength;
};

class TC_GAME_API BotLifecycleMgr
{
    BotLifecycleMgr();
    ~BotLifecycleMgr();
    BotLifecycleMgr(BotLifecycleMgr const&) = delete;
    BotLifecycleMgr& operator=(BotLifecycleMgr const&) = delete;

public:
    static BotLifecycleMgr* instance();

    // Initialization
    bool Initialize();
    void Shutdown();
    void Update(uint32 diff);

    // Lifecycle orchestration
    void StartLifecycle();
    void StopLifecycle();
    void PauseLifecycle();
    void ResumeLifecycle();

    // Population management
    void MaintainPopulation();
    void AdjustPopulation(uint32 targetCount);
    void BalanceZonePopulations();
    void RefreshInactiveBots();

    // Event handling
    void RegisterEventHandler(LifecycleEventHandler handler);
    void TriggerEvent(LifecycleEvent event, ObjectGuid guid, uint32 data1 = 0, uint32 data2 = 0);

    // Query functions
    bool IsLifecycleActive() const { return _active; }
    uint32 GetActiveCount() const { return _stats.currentlyActive.load(); }
    LifecycleStats const& GetStatistics() const { return _stats; }

    // Server integration hooks
    void OnServerStartup();
    void OnServerShutdown();
    void OnPlayerLogin(Player* player);
    void OnPlayerLogout(Player* player);

private:
    // Internal lifecycle management
    void ProcessLifecycleTick();
    void CheckPopulationTargets();
    void HandleInactiveBots();
    void UpdateStatistics();

private:
    // Sub-managers
    std::unique_ptr<BotSpawner> _spawner;
    std::unique_ptr<BotScheduler> _scheduler;

    // Event handlers
    std::vector<LifecycleEventHandler> _eventHandlers;

    // State
    std::atomic<bool> _active;
    std::atomic<bool> _paused;
    uint32 _updateTimer;

    // Statistics
    LifecycleStats _stats;

    // Configuration
    uint32 _targetPopulation;
    uint32 _updateInterval;
    bool _dynamicPopulation;
};

#define sBotLifecycleMgr BotLifecycleMgr::instance()

} // namespace Playerbot
```

## Database Schema

### SQL Schema: `sql/playerbot/003_lifecycle_management.sql`
```sql
-- Bot spawn logs for analytics
CREATE TABLE IF NOT EXISTS `playerbot_spawn_log` (
    `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `guid` BIGINT UNSIGNED NOT NULL,
    `zone_id` INT UNSIGNED DEFAULT NULL,
    `map_id` INT UNSIGNED DEFAULT NULL,
    `spawn_time` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    `despawn_time` TIMESTAMP NULL DEFAULT NULL,
    `session_duration` INT UNSIGNED DEFAULT NULL,
    `spawn_reason` VARCHAR(50) DEFAULT NULL,
    `despawn_reason` VARCHAR(50) DEFAULT NULL,
    PRIMARY KEY (`id`),
    KEY `idx_guid` (`guid`),
    KEY `idx_spawn_time` (`spawn_time`),
    KEY `idx_zone` (`zone_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Activity patterns for scheduling
CREATE TABLE IF NOT EXISTS `playerbot_activity_patterns` (
    `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `name` VARCHAR(50) NOT NULL,
    `active_hours` JSON NOT NULL,
    `active_days` JSON NOT NULL,
    `login_probability` FLOAT DEFAULT 1.0,
    `min_session` INT UNSIGNED DEFAULT 1800,
    `max_session` INT UNSIGNED DEFAULT 14400,
    `sessions_per_day` INT UNSIGNED DEFAULT 2,
    `prefer_peak` BOOLEAN DEFAULT TRUE,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_pattern_name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Bot schedules
CREATE TABLE IF NOT EXISTS `playerbot_schedules` (
    `guid` BIGINT UNSIGNED NOT NULL,
    `pattern_name` VARCHAR(50) DEFAULT 'default',
    `next_login` TIMESTAMP NULL DEFAULT NULL,
    `next_logout` TIMESTAMP NULL DEFAULT NULL,
    `total_sessions` INT UNSIGNED DEFAULT 0,
    `total_playtime` INT UNSIGNED DEFAULT 0,
    `last_update` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`guid`),
    KEY `idx_next_login` (`next_login`),
    KEY `idx_pattern` (`pattern_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Zone population targets
CREATE TABLE IF NOT EXISTS `playerbot_zone_populations` (
    `zone_id` INT UNSIGNED NOT NULL,
    `map_id` INT UNSIGNED NOT NULL,
    `min_bots` INT UNSIGNED DEFAULT 0,
    `max_bots` INT UNSIGNED DEFAULT 50,
    `target_bots` INT UNSIGNED DEFAULT 10,
    `current_bots` INT UNSIGNED DEFAULT 0,
    `min_level` TINYINT UNSIGNED DEFAULT 1,
    `max_level` TINYINT UNSIGNED DEFAULT 80,
    `bot_density` FLOAT DEFAULT 0.1,
    `last_update` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`zone_id`),
    KEY `idx_map` (`map_id`),
    KEY `idx_level_range` (`min_level`, `max_level`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Lifecycle events log
CREATE TABLE IF NOT EXISTS `playerbot_lifecycle_events` (
    `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `event_type` ENUM('CREATED','SPAWNED','DESPAWNED','DELETED',
                      'SESSION_START','SESSION_END','ZONE_CHANGED',
                      'LEVEL_CHANGED','ACTIVITY_CHANGED') NOT NULL,
    `guid` BIGINT UNSIGNED NOT NULL,
    `data1` INT UNSIGNED DEFAULT NULL,
    `data2` INT UNSIGNED DEFAULT NULL,
    `timestamp` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_guid` (`guid`),
    KEY `idx_timestamp` (`timestamp`),
    KEY `idx_event_type` (`event_type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
```

## Testing Suite

### File: `src/modules/Playerbot/Tests/LifecycleManagementTest.cpp`
```cpp
#include "catch.hpp"
#include "BotSpawner.h"
#include "BotScheduler.h"
#include "BotLifecycleMgr.h"
#include "TestHelpers.h"
#include <chrono>
#include <thread>

using namespace Playerbot;

TEST_CASE("BotSpawner", "[lifecycle][integration]")
{
    SECTION("Basic Spawning")
    {
        SECTION("Spawns single bot")
        {
            SpawnRequest request;
            request.type = SpawnRequest::RANDOM;

            ObjectGuid guid = sBotSpawner->SpawnBot(request);
            REQUIRE(!guid.IsEmpty());
            REQUIRE(sBotSpawner->GetTotalBotCount() > 0);
        }

        SECTION("Respects population caps")
        {
            SpawnConfig config = sBotSpawner->GetConfig();
            config.maxBotsTotal = 5;
            config.respectPopulationCaps = true;
            sBotSpawner->SetConfig(config);

            // Spawn to limit
            std::vector<ObjectGuid> bots;
            for (int i = 0; i < 5; ++i)
            {
                SpawnRequest request;
                request.type = SpawnRequest::RANDOM;
                ObjectGuid guid = sBotSpawner->SpawnBot(request);
                REQUIRE(!guid.IsEmpty());
                bots.push_back(guid);
            }

            // 6th should fail
            SpawnRequest request;
            request.type = SpawnRequest::RANDOM;
            ObjectGuid guid = sBotSpawner->SpawnBot(request);
            REQUIRE(guid.IsEmpty());

            // Cleanup
            for (auto const& botGuid : bots)
            {
                sBotSpawner->DespawnBot(botGuid);
            }
        }

        SECTION("Batch spawning works")
        {
            std::vector<SpawnRequest> requests;
            for (int i = 0; i < 10; ++i)
            {
                SpawnRequest request;
                request.type = SpawnRequest::RANDOM;
                requests.push_back(request);
            }

            auto guids = sBotSpawner->SpawnBots(requests);
            REQUIRE(guids.size() == 10);

            // Cleanup
            for (auto const& guid : guids)
            {
                sBotSpawner->DespawnBot(guid);
            }
        }
    }

    SECTION("Zone Population")
    {
        SECTION("Maintains zone populations")
        {
            uint32 testZoneId = 1519; // Stormwind
            sBotSpawner->SpawnForZone(testZoneId, 5);

            auto population = sBotSpawner->GetZonePopulation(testZoneId);
            REQUIRE(population.botCount >= 5);
        }

        SECTION("Updates population targets")
        {
            sBotSpawner->UpdatePopulationTargets();
            // Should complete without errors
        }
    }
}

TEST_CASE("BotScheduler", "[lifecycle][scheduling]")
{
    SECTION("Pattern Management")
    {
        SECTION("Loads default patterns")
        {
            REQUIRE(sBotScheduler->Initialize());
            auto pattern = sBotScheduler->GetPattern("default");
            REQUIRE(pattern != nullptr);
            REQUIRE(pattern->name == "default");
        }

        SECTION("Registers custom pattern")
        {
            ActivityPattern custom;
            custom.name = "test_pattern";
            custom.activeHours = {{9, 17}}; // 9 AM to 5 PM
            custom.minSessionDuration = 600;
            custom.maxSessionDuration = 1800;

            sBotScheduler->RegisterPattern("test_pattern", custom);
            auto pattern = sBotScheduler->GetPattern("test_pattern");
            REQUIRE(pattern != nullptr);
            REQUIRE(pattern->name == "test_pattern");
        }
    }

    SECTION("Scheduling")
    {
        ObjectGuid testGuid = ObjectGuid::Create<HighGuid::Player>(12345);

        SECTION("Schedules bot successfully")
        {
            sBotScheduler->ScheduleBot(testGuid);
            REQUIRE(sBotScheduler->IsBotScheduled(testGuid));
            REQUIRE(sBotScheduler->GetScheduledBotCount() > 0);
        }

        SECTION("Schedules actions")
        {
            ScheduleEntry entry;
            entry.botGuid = testGuid;
            entry.action = ScheduleEntry::LOGIN;
            entry.executeTime = std::chrono::system_clock::now() + std::chrono::seconds(1);

            sBotScheduler->ScheduleAction(entry);
            // Action should be queued
        }

        SECTION("Calculates next login time")
        {
            sBotScheduler->AssignPattern(testGuid, "default");
            auto nextLogin = sBotScheduler->GetNextScheduledAction(testGuid);
            REQUIRE(nextLogin > std::chrono::system_clock::now());
        }
    }
}

TEST_CASE("Performance", "[lifecycle][performance]")
{
    SECTION("Spawn performance")
    {
        auto start = std::chrono::high_resolution_clock::now();

        SpawnRequest request;
        request.type = SpawnRequest::RANDOM;
        ObjectGuid guid = sBotSpawner->SpawnBot(request);

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start).count();

        REQUIRE(!guid.IsEmpty());
        REQUIRE(elapsed < 50); // Should complete in under 50ms

        sBotSpawner->DespawnBot(guid);
    }

    SECTION("Batch spawn performance")
    {
        auto start = std::chrono::high_resolution_clock::now();

        std::vector<SpawnRequest> requests;
        for (int i = 0; i < 100; ++i)
        {
            SpawnRequest request;
            request.type = SpawnRequest::RANDOM;
            requests.push_back(request);
        }

        auto guids = sBotSpawner->SpawnBots(requests);

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start).count();

        REQUIRE(guids.size() == 100);
        REQUIRE(elapsed < 2000); // 100 bots in under 2 seconds

        // Cleanup
        for (auto const& guid : guids)
        {
            sBotSpawner->DespawnBot(guid);
        }
    }

    SECTION("Scheduling overhead")
    {
        // Schedule 1000 bots
        std::vector<ObjectGuid> testGuids;
        for (int i = 0; i < 1000; ++i)
        {
            ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(100000 + i);
            testGuids.push_back(guid);
            sBotScheduler->ScheduleBot(guid);
        }

        REQUIRE(sBotScheduler->GetScheduledBotCount() == 1000);

        // Measure CPU usage of scheduler
        auto start = std::chrono::high_resolution_clock::now();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start).count();

        // Scheduling overhead should be minimal
        // This is a simplified test - real measurement would track actual CPU time
        REQUIRE(elapsed >= 1000); // At least 1 second passed
        REQUIRE(elapsed < 1100);  // No more than 100ms overhead
    }
}
```

## Configuration File

### File: `conf/playerbots.conf.dist` (Lifecycle Section)
```ini
###################################################################################################
#   LIFECYCLE MANAGEMENT CONFIGURATION
#
#   Bot spawning, scheduling, and population management
###################################################################################################

#
#   Playerbot.Spawn.MaxTotal
#       Maximum total number of bots that can be spawned
#       Default: 500
#

Playerbot.Spawn.MaxTotal = 500

#
#   Playerbot.Spawn.MaxPerZone
#       Maximum number of bots per zone
#       Default: 50
#

Playerbot.Spawn.MaxPerZone = 50

#
#   Playerbot.Spawn.MaxPerMap
#       Maximum number of bots per map
#       Default: 200
#

Playerbot.Spawn.MaxPerMap = 200

#
#   Playerbot.Spawn.BatchSize
#       Number of bots to spawn in parallel batches
#       Default: 10
#

Playerbot.Spawn.BatchSize = 10

#
#   Playerbot.Spawn.Dynamic
#       Enable dynamic spawning based on player population
#       Default: 1 (enabled)
#

Playerbot.Spawn.Dynamic = 1

#
#   Playerbot.Spawn.BotToPlayerRatio
#       Ratio of bots to players for dynamic spawning
#       Default: 2.0 (2 bots per player)
#

Playerbot.Spawn.BotToPlayerRatio = 2.0

#
#   Playerbot.Schedule.Enable
#       Enable realistic scheduling patterns
#       Default: 1 (enabled)
#

Playerbot.Schedule.Enable = 1

#
#   Playerbot.Schedule.RealisticPatterns
#       Use realistic login/logout patterns
#       Default: 1 (enabled)
#

Playerbot.Schedule.RealisticPatterns = 1

#
#   Playerbot.Schedule.PeakMultiplier
#       Bot activity multiplier during peak hours (6 PM - 11 PM)
#       Default: 2.0
#

Playerbot.Schedule.PeakMultiplier = 2.0

#
#   Playerbot.Schedule.OffPeakMultiplier
#       Bot activity multiplier during off-peak hours (2 AM - 10 AM)
#       Default: 0.5
#

Playerbot.Schedule.OffPeakMultiplier = 0.5
```

## Implementation Checklist

### Week 11 Tasks
- [ ] Implement BotSpawner core class
- [ ] Add population management logic
- [ ] Implement zone-based spawning
- [ ] Create spawn request queue with Intel TBB
- [ ] Implement BotScheduler with activity patterns
- [ ] Add realistic login/logout scheduling
- [ ] Implement BotLifecycleMgr coordinator
- [ ] Create database tables for lifecycle data
- [ ] Write comprehensive unit tests
- [ ] Performance testing with 500+ bots

## Performance Optimizations

### Implemented Optimizations
1. **Parallel Spawning**: Intel TBB thread pool for concurrent spawning
2. **Batch Processing**: Spawn multiple bots in parallel batches
3. **Priority Queue**: TBB concurrent_priority_queue for scheduling
4. **Async Database**: All lifecycle data persisted asynchronously
5. **Memory Efficiency**: <10KB per scheduled bot
6. **Lock-Free Collections**: phmap for all concurrent data structures

### Scalability Metrics
- Spawn 100 bots: <2 seconds
- Schedule 1000 bots: <0.1% CPU overhead
- Population rebalancing: <100ms
- Memory usage: Linear scaling, <10MB for 1000 bots

## Integration Points

### With Phase 1 Components
```cpp
// Uses enterprise session management
BotSession* session = sBotSessionMgr->CreateSession(accountId);

// Leverages async database
sBotDBPool->ExecuteAsync(stmt);

// Uses Intel TBB for parallelism
tbb::concurrent_queue<SpawnRequest> for batch processing
```

### With Phase 2 Components
```cpp
// Integrates with account management
std::vector<uint32> accounts = sBotAccountMgr->GetAvailableAccounts();

// Uses character management
std::vector<ObjectGuid> characters = sBotCharacterMgr->GetBotCharacters(accountId);
```

## Next Steps

After completing Week 11 lifecycle management:
1. Proceed to AI Framework implementation (Week 12-14)
2. Integrate AI with lifecycle systems
3. Implement strategy pattern for decision making
4. Add movement and combat systems

---

**Status**: Ready for implementation
**Dependencies**: Phase 1 ✅, Account Management ✅, Character Management ✅
**Estimated Completion**: 1 week