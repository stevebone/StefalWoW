# Bot Lifecycle Management - Part 2: BotSpawner Implementation
## Week 11 Day 1-2: Bot Spawner

### File: `src/modules/Playerbot/Lifecycle/BotSpawner.h`
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

### File: `src/modules/Playerbot/Lifecycle/BotSpawner.cpp`
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

## Next Parts
- [Part 1: Overview & Architecture](LIFECYCLE_PART1_OVERVIEW.md)
- [Part 3: BotScheduler Implementation](LIFECYCLE_PART3_SCHEDULER.md)
- [Part 4: Lifecycle Coordinator](LIFECYCLE_PART4_COORDINATOR.md)
- [Part 5: Database & Testing](LIFECYCLE_PART5_DATABASE.md)

---

**Status**: Ready for implementation
**Dependencies**: BotAccountMgr, BotCharacterMgr, BotSessionMgr
**Estimated Completion**: Day 1-2 of Week 11