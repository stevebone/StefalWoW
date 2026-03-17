# Bot Lifecycle Management - Part 4: Lifecycle Coordinator
## Week 11 Day 5: BotLifecycleMgr Implementation

### File: `src/modules/Playerbot/Lifecycle/BotLifecycleMgr.h`
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
class Player;

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
    void LogEvent(LifecycleEvent event, ObjectGuid guid, uint32 data1, uint32 data2);

private:
    // Sub-managers (not owned, managed by singleton pattern)
    BotSpawner* _spawner;
    BotScheduler* _scheduler;

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

### File: `src/modules/Playerbot/Lifecycle/BotLifecycleMgr.cpp`
```cpp
#include "BotLifecycleMgr.h"
#include "BotSpawner.h"
#include "BotScheduler.h"
#include "BotDatabasePool.h"
#include "World.h"
#include "Player.h"
#include "Log.h"

namespace Playerbot
{

BotLifecycleMgr::BotLifecycleMgr()
    : _spawner(nullptr)
    , _scheduler(nullptr)
    , _active(false)
    , _paused(false)
    , _updateTimer(0)
    , _targetPopulation(100)
    , _updateInterval(1000)
    , _dynamicPopulation(true)
{
    memset(&_stats, 0, sizeof(_stats));
}

BotLifecycleMgr::~BotLifecycleMgr()
{
    Shutdown();
}

BotLifecycleMgr* BotLifecycleMgr::instance()
{
    static BotLifecycleMgr instance;
    return &instance;
}

bool BotLifecycleMgr::Initialize()
{
    TC_LOG_INFO("playerbot", "Initializing BotLifecycleMgr - Coordinating all lifecycle systems...");

    // Get sub-manager instances
    _spawner = sBotSpawner;
    _scheduler = sBotScheduler;

    if (!_spawner || !_scheduler)
    {
        TC_LOG_ERROR("playerbot", "Failed to initialize BotLifecycleMgr - sub-managers not available");
        return false;
    }

    // Load configuration
    _targetPopulation = sConfigMgr->GetIntDefault("Playerbot.Lifecycle.TargetPopulation", 100);
    _updateInterval = sConfigMgr->GetIntDefault("Playerbot.Lifecycle.UpdateInterval", 1000);
    _dynamicPopulation = sConfigMgr->GetBoolDefault("Playerbot.Lifecycle.DynamicPopulation", true);

    // Register default event handlers
    RegisterEventHandler([this](LifecycleEvent event, ObjectGuid guid, uint32 data1, uint32 data2)
    {
        LogEvent(event, guid, data1, data2);
    });

    TC_LOG_INFO("playerbot", "BotLifecycleMgr initialized successfully");
    TC_LOG_INFO("playerbot", "  -> Target population: %u", _targetPopulation);
    TC_LOG_INFO("playerbot", "  -> Update interval: %ums", _updateInterval);
    TC_LOG_INFO("playerbot", "  -> Dynamic population: %s", _dynamicPopulation ? "enabled" : "disabled");

    return true;
}

void BotLifecycleMgr::Shutdown()
{
    TC_LOG_INFO("playerbot", "Shutting down BotLifecycleMgr...");

    // Stop lifecycle
    StopLifecycle();

    // Clear event handlers
    _eventHandlers.clear();

    TC_LOG_INFO("playerbot", "BotLifecycleMgr shutdown complete. Final stats:");
    TC_LOG_INFO("playerbot", "  -> Total created: %u", _stats.totalCreated.load());
    TC_LOG_INFO("playerbot", "  -> Total spawned: %u", _stats.totalSpawned.load());
    TC_LOG_INFO("playerbot", "  -> Total despawned: %u", _stats.totalDespawned.load());
    TC_LOG_INFO("playerbot", "  -> Peak concurrent: %u", _stats.peakConcurrent.load());
    TC_LOG_INFO("playerbot", "  -> Average session: %u seconds", _stats.averageSessionLength.load());
}

void BotLifecycleMgr::Update(uint32 diff)
{
    if (!_active || _paused)
        return;

    _updateTimer += diff;
    if (_updateTimer < _updateInterval)
        return;

    _updateTimer = 0;

    // Process lifecycle tick
    ProcessLifecycleTick();

    // Check population targets
    CheckPopulationTargets();

    // Handle inactive bots
    HandleInactiveBots();

    // Update statistics
    UpdateStatistics();
}

void BotLifecycleMgr::StartLifecycle()
{
    if (_active)
    {
        TC_LOG_WARN("playerbot", "Lifecycle already active");
        return;
    }

    TC_LOG_INFO("playerbot", "Starting bot lifecycle management...");

    _active = true;
    _paused = false;

    // Initial population spawn
    MaintainPopulation();

    TC_LOG_INFO("playerbot", "Bot lifecycle started");
}

void BotLifecycleMgr::StopLifecycle()
{
    if (!_active)
        return;

    TC_LOG_INFO("playerbot", "Stopping bot lifecycle management...");

    _active = false;

    // Despawn all bots
    if (_spawner)
    {
        _spawner->DespawnAllBots();
    }

    TC_LOG_INFO("playerbot", "Bot lifecycle stopped");
}

void BotLifecycleMgr::PauseLifecycle()
{
    _paused = true;
    TC_LOG_INFO("playerbot", "Bot lifecycle paused");
}

void BotLifecycleMgr::ResumeLifecycle()
{
    _paused = false;
    TC_LOG_INFO("playerbot", "Bot lifecycle resumed");
}

void BotLifecycleMgr::MaintainPopulation()
{
    if (!_spawner)
        return;

    uint32 currentCount = _spawner->GetTotalBotCount();
    
    if (currentCount < _targetPopulation)
    {
        uint32 toSpawn = _targetPopulation - currentCount;
        TC_LOG_INFO("playerbot", "Maintaining population: spawning %u bots (current: %u, target: %u)",
                    toSpawn, currentCount, _targetPopulation);

        // Create spawn requests
        std::vector<SpawnRequest> requests;
        for (uint32 i = 0; i < toSpawn; ++i)
        {
            SpawnRequest request;
            request.type = SpawnRequest::RANDOM;
            requests.push_back(request);
        }

        // Batch spawn
        auto guids = _spawner->SpawnBots(requests);

        // Trigger events for spawned bots
        for (auto const& guid : guids)
        {
            TriggerEvent(LifecycleEvent::BOT_SPAWNED, guid);
        }
    }
    else if (currentCount > _targetPopulation)
    {
        uint32 toDespawn = currentCount - _targetPopulation;
        TC_LOG_INFO("playerbot", "Maintaining population: despawning %u bots (current: %u, target: %u)",
                    toDespawn, currentCount, _targetPopulation);

        uint32 despawned = _spawner->DespawnBots(toDespawn);
        TC_LOG_INFO("playerbot", "Despawned %u bots", despawned);
    }
}

void BotLifecycleMgr::AdjustPopulation(uint32 targetCount)
{
    TC_LOG_INFO("playerbot", "Adjusting target population from %u to %u",
                _targetPopulation, targetCount);

    _targetPopulation = targetCount;
    MaintainPopulation();
}

void BotLifecycleMgr::BalanceZonePopulations()
{
    if (!_spawner)
        return;

    TC_LOG_DEBUG("playerbot", "Balancing zone populations...");

    _spawner->UpdatePopulationTargets();
    _spawner->SpawnToPopulationTarget();
}

void BotLifecycleMgr::RefreshInactiveBots()
{
    if (!_spawner)
        return;

    // Despawn inactive bots
    uint32 despawned = _spawner->DespawnInactiveBot();
    if (despawned > 0)
    {
        TC_LOG_DEBUG("playerbot", "Refreshed %u inactive bots", despawned);
        
        // Spawn replacements
        MaintainPopulation();
    }
}

void BotLifecycleMgr::RegisterEventHandler(LifecycleEventHandler handler)
{
    _eventHandlers.push_back(handler);
}

void BotLifecycleMgr::TriggerEvent(LifecycleEvent event, ObjectGuid guid, uint32 data1, uint32 data2)
{
    // Update statistics
    switch (event)
    {
        case LifecycleEvent::BOT_CREATED:
            _stats.totalCreated++;
            break;
        case LifecycleEvent::BOT_SPAWNED:
            _stats.totalSpawned++;
            _stats.currentlyActive++;
            if (_stats.currentlyActive > _stats.peakConcurrent)
                _stats.peakConcurrent = _stats.currentlyActive.load();
            break;
        case LifecycleEvent::BOT_DESPAWNED:
            _stats.totalDespawned++;
            if (_stats.currentlyActive > 0)
                _stats.currentlyActive--;
            break;
        case LifecycleEvent::SESSION_START:
            // Track session start time
            break;
        case LifecycleEvent::SESSION_END:
            // Calculate session duration
            _stats.totalSessionTime += data1; // data1 = session duration
            if (_stats.totalSpawned > 0)
                _stats.averageSessionLength = _stats.totalSessionTime / _stats.totalSpawned;
            break;
        default:
            break;
    }

    // Notify all handlers
    for (auto const& handler : _eventHandlers)
    {
        handler(event, guid, data1, data2);
    }
}

void BotLifecycleMgr::ProcessLifecycleTick()
{
    // This is called periodically to process lifecycle events
    // Could be used for periodic checks, cleanup, etc.
}

void BotLifecycleMgr::CheckPopulationTargets()
{
    if (_dynamicPopulation)
    {
        // Adjust population based on server activity
        // TODO: Query active player count and adjust bot population accordingly
        
        // For now, just maintain the configured target
        MaintainPopulation();
    }
}

void BotLifecycleMgr::HandleInactiveBots()
{
    // Check for bots that have been idle too long
    RefreshInactiveBots();
}

void BotLifecycleMgr::UpdateStatistics()
{
    // Update current active count
    if (_spawner)
    {
        _stats.currentlyActive = _spawner->GetTotalBotCount();
    }
}

void BotLifecycleMgr::LogEvent(LifecycleEvent event, ObjectGuid guid, uint32 data1, uint32 data2)
{
    // Store event in database for analytics
    auto stmt = sBotDBPool->GetPreparedStatement(BOT_INS_LIFECYCLE_EVENT);
    
    std::string eventType;
    switch (event)
    {
        case LifecycleEvent::BOT_CREATED: eventType = "CREATED"; break;
        case LifecycleEvent::BOT_SPAWNED: eventType = "SPAWNED"; break;
        case LifecycleEvent::BOT_DESPAWNED: eventType = "DESPAWNED"; break;
        case LifecycleEvent::BOT_DELETED: eventType = "DELETED"; break;
        case LifecycleEvent::SESSION_START: eventType = "SESSION_START"; break;
        case LifecycleEvent::SESSION_END: eventType = "SESSION_END"; break;
        case LifecycleEvent::ZONE_CHANGED: eventType = "ZONE_CHANGED"; break;
        case LifecycleEvent::LEVEL_CHANGED: eventType = "LEVEL_CHANGED"; break;
        case LifecycleEvent::ACTIVITY_CHANGED: eventType = "ACTIVITY_CHANGED"; break;
    }
    
    stmt->setString(0, eventType);
    stmt->setUInt64(1, guid.GetRawValue());
    stmt->setUInt32(2, data1);
    stmt->setUInt32(3, data2);
    
    sBotDBPool->ExecuteAsync(stmt);
}

void BotLifecycleMgr::OnServerStartup()
{
    TC_LOG_INFO("playerbot", "BotLifecycleMgr: Server startup detected");

    // Initialize sub-systems
    if (_spawner)
        _spawner->Initialize();
    
    if (_scheduler)
        _scheduler->Initialize();

    // Start lifecycle
    StartLifecycle();
}

void BotLifecycleMgr::OnServerShutdown()
{
    TC_LOG_INFO("playerbot", "BotLifecycleMgr: Server shutdown detected");

    // Stop lifecycle
    StopLifecycle();
    
    // Shutdown sub-systems
    if (_spawner)
        _spawner->Shutdown();
    
    if (_scheduler)
        _scheduler->Shutdown();
}

void BotLifecycleMgr::OnPlayerLogin(Player* player)
{
    if (!player || !_dynamicPopulation)
        return;

    // Adjust bot population based on player activity
    // Could spawn more bots in the player's zone
    
    TC_LOG_DEBUG("playerbot", "Player %s logged in, checking population balance",
                 player->GetName().c_str());

    BalanceZonePopulations();
}

void BotLifecycleMgr::OnPlayerLogout(Player* player)
{
    if (!player || !_dynamicPopulation)
        return;

    // Could reduce bots in zones with no players
    
    TC_LOG_DEBUG("playerbot", "Player %s logged out, checking population balance",
                 player->GetName().c_str());

    BalanceZonePopulations();
}

} // namespace Playerbot
```

## Configuration Integration

### File: `conf/playerbots.conf.dist` (Lifecycle Section)
```ini
###################################################################################################
#   LIFECYCLE MANAGEMENT CONFIGURATION
#
#   Bot spawning, scheduling, and population management
###################################################################################################

#
#   Playerbot.Lifecycle.TargetPopulation
#       Target number of active bots on the server
#       Default: 100
#

Playerbot.Lifecycle.TargetPopulation = 100

#
#   Playerbot.Lifecycle.UpdateInterval
#       How often to update lifecycle (milliseconds)
#       Default: 1000
#

Playerbot.Lifecycle.UpdateInterval = 1000

#
#   Playerbot.Lifecycle.DynamicPopulation
#       Adjust bot population based on player activity
#       Default: 1 (enabled)
#

Playerbot.Lifecycle.DynamicPopulation = 1

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

## Next Parts
- [Part 1: Overview & Architecture](LIFECYCLE_PART1_OVERVIEW.md)
- [Part 2: BotSpawner Implementation](LIFECYCLE_PART2_SPAWNER.md)
- [Part 3: BotScheduler Implementation](LIFECYCLE_PART3_SCHEDULER.md)
- [Part 5: Database & Testing](LIFECYCLE_PART5_DATABASE.md)

---

**Status**: Ready for implementation
**Dependencies**: BotSpawner, BotScheduler, BotDatabasePool
**Estimated Completion**: Day 5 of Week 11