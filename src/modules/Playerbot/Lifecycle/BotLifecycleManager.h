/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#ifndef BOT_LIFECYCLE_MANAGER_H
#define BOT_LIFECYCLE_MANAGER_H

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "ObjectGuid.h"
#include <memory>
#include <unordered_map>
#include <queue>
#include <chrono>
#include <atomic>
#include <mutex>
#include <functional>

class Player;

namespace Playerbot {

class BotSession;
class BotAI;
class BotWorldEntry;

/**
 * Bot Lifecycle States
 *
 * Represents the complete lifecycle of a bot from creation to removal
 */
enum class BotLifecycleState : uint8
{
    CREATED,            // Bot session created but not logged in
    LOGGING_IN,         // Currently logging into world
    ACTIVE,             // Fully active in world
    IDLE,               // Active but not doing anything
    COMBAT,             // Engaged in combat
    QUESTING,           // Performing quest activities
    FOLLOWING,          // Following another player
    RESTING,            // Resting to recover health/mana
    LOGGING_OUT,        // In process of logging out
    OFFLINE,            // Logged out but session maintained
    TERMINATED          // Session terminated, ready for cleanup
};

/**
 * Bot Activity Information
 */
struct BotActivity
{
    enum Type
    {
        NONE,
        COMBAT,
        QUEST,
        DUNGEON,
        RAID,
        PVP,
        GATHERING,
        CRAFTING,
        TRADING,
        SOCIAL,
        TRAVEL
    };

    Type type = NONE;
    uint32 targetId = 0;
    std::string description;
    std::chrono::steady_clock::time_point startTime;
    uint32 durationMs = 0;
};

/**
 * Bot Performance Metrics
 */
struct BotPerformanceMetrics
{
    // CPU metrics
    uint64 aiUpdateTime = 0;        // Total AI update time in microseconds
    uint32 aiUpdateCount = 0;        // Number of AI updates
    float avgAiUpdateTime = 0.0f;    // Average AI update time in ms

    // Memory metrics
    size_t currentMemoryUsage = 0;   // Current memory in bytes
    size_t peakMemoryUsage = 0;      // Peak memory usage

    // Activity metrics
    uint32 actionsExecuted = 0;      // Total actions executed
    uint32 spellsCast = 0;           // Total spells cast
    uint32 itemsUsed = 0;            // Total items used
    uint32 questsCompleted = 0;      // Quests completed
    uint32 mobsKilled = 0;           // Mobs killed
    uint32 successfulEncounters = 0; // Combat encounters won
    uint32 failedEncounters = 0;     // Combat encounters lost (deaths)

    // Network metrics
    uint32 packetsReceived = 0;      // Packets received
    uint32 packetsSent = 0;          // Packets sent
    uint64 bytesReceived = 0;        // Bytes received
    uint64 bytesSent = 0;            // Bytes sent

    // Time metrics
    std::chrono::steady_clock::time_point loginTime;
    std::chrono::steady_clock::time_point lastActivityTime;
    uint64 totalActiveTime = 0;      // Total active time in seconds
    uint64 totalIdleTime = 0;        // Total idle time in seconds
};

/**
 * Individual Bot Lifecycle Controller
 */
class TC_GAME_API BotLifecycle
{
public:
    BotLifecycle(ObjectGuid botGuid, std::shared_ptr<BotSession> session);
    ~BotLifecycle();

    // === Lifecycle Management ===

    /**
     * Start the bot lifecycle (login and activate)
     */
    bool Start();

    /**
     * Stop the bot lifecycle (logout and cleanup)
     * @param immediate If true, logout immediately without proper cleanup
     */
    void Stop(bool immediate = false);

    /**
     * Update the bot lifecycle
     * @param diff Time since last update in milliseconds
     */
    void Update(uint32 diff);

    /**
     * Pause the bot (temporary inactive state)
     */
    void Pause();

    /**
     * Resume from paused state
     */
    void Resume();

    // === State Management ===

    BotLifecycleState GetState() const { return _state.load(); }
    bool IsActive() const { return _state == BotLifecycleState::ACTIVE; }
    bool IsOnline() const;

    /**
     * Transition to a new lifecycle state
     */
    bool TransitionToState(BotLifecycleState newState);

    // === Activity Management ===

    /**
     * Set the current bot activity
     */
    void SetActivity(BotActivity const& activity);

    /**
     * Get current activity
     */
    BotActivity GetCurrentActivity() const;

    /**
     * Check if bot is busy with an activity
     */
    bool IsBusy() const { return _currentActivity.type != BotActivity::NONE; }

    // === Performance Monitoring ===

    /**
     * Update performance metrics
     */
    void UpdateMetrics(uint32 aiUpdateTime);

    /**
     * Get performance metrics
     */
    BotPerformanceMetrics const& GetMetrics() const { return _metrics; }

    // === Event Handlers ===

    void OnLogin();
    void OnLogout();
    void OnEnterCombat();
    void OnLeaveCombat();
    void OnDeath();
    void OnRespawn();
    void OnQuestComplete(uint32 questId);
    void OnLevelUp(uint32 newLevel);

    // === Accessors ===

    ObjectGuid GetGuid() const { return _botGuid; }
    Player* GetPlayer() const;
    BotSession* GetSession() const { return _session.get(); }
    BotAI* GetAI() const;

private:
    // === Internal State Handlers ===

    void HandleActiveState(uint32 diff);
    void HandleIdleState(uint32 diff);
    void HandleCombatState(uint32 diff);
    void HandleQuestingState(uint32 diff);
    void HandleRestingState(uint32 diff);

    // === Idle Behavior ===

    void SelectIdleBehavior();
    void PerformIdleAction();

    // === Resource Management ===

    bool NeedsRest() const;
    bool NeedsRepair() const;
    bool NeedsVendor() const;

private:
    // Core identifiers
    ObjectGuid _botGuid;
    std::shared_ptr<BotSession> _session;

    // State management
    std::atomic<BotLifecycleState> _state;
    std::chrono::steady_clock::time_point _stateChangeTime;

    // Activity tracking
    BotActivity _currentActivity;
    std::queue<BotActivity> _activityQueue;
    

    // Performance metrics
    BotPerformanceMetrics _metrics;
    

    // Idle management
    uint32 _idleTimer;
    uint32 _nextIdleAction;
    static constexpr uint32 IDLE_ACTION_INTERVAL = 30000; // 30 seconds

    // Cleanup flag
    std::atomic<bool> _pendingCleanup{false};
};

/**
 * Global statistics for all bots
 */
struct GlobalStats
{
    uint32 totalBots{0};
    uint32 activeBots{0};
    uint32 idleBots{0};
    uint32 combatBots{0};
    uint32 questingBots{0};
    uint32 offlineBots{0};
    uint64 totalCpuTime{0};
    size_t totalMemoryUsage{0};
    float averageUpdateTime{0.0f};
    float avgAiUpdateTime{0.0f};           // Average AI update time in ms
    uint32 totalActionsExecuted{0};
    uint32 totalActionsPerSecond{0};       // Actions per second rate
    uint32 totalEncounters{0};
    float successRate{0.0f};
};

/**
 * Global Bot Lifecycle Manager
 *
 * Manages all bot lifecycles in the system
 */
class TC_GAME_API BotLifecycleManager final 
{
public:
    explicit BotLifecycleManager(Player* bot);
    ~BotLifecycleManager();
    BotLifecycleManager(BotLifecycleManager const&) = delete;
    BotLifecycleManager& operator=(BotLifecycleManager const&) = delete;

    // === IBotLifecycleManager interface implementation ===

    /**
     * Create a new bot lifecycle
     * @param botGuid The bot's GUID
     * @param session The bot's session
     * @return Shared pointer to the lifecycle controller
     */
    std::shared_ptr<BotLifecycle> CreateBotLifecycle(ObjectGuid botGuid, std::shared_ptr<BotSession> session);

    /**
     * Remove a bot lifecycle
     * @param botGuid The bot's GUID
     */
    void RemoveBotLifecycle(ObjectGuid botGuid);

    /**
     * Get a bot's lifecycle controller
     * @param botGuid The bot's GUID
     * @return Shared pointer to lifecycle, nullptr if not found
     */
    std::shared_ptr<BotLifecycle> GetBotLifecycle(ObjectGuid botGuid) const;

    /**
     * Get all active bot lifecycles
     */
    std::vector<std::shared_ptr<BotLifecycle>> GetActiveLifecycles() const;

    // === Global Updates ===

    /**
     * Update all bot lifecycles
     * @param diff Time since last update in milliseconds
     */
    void UpdateAll(uint32 diff);

    /**
     * Stop all bots
     * @param immediate If true, stop immediately without cleanup
     */
    void StopAll(bool immediate = false);

    // === Statistics ===

    /**
     * Get global statistics
     */
    GlobalStats GetGlobalStats() const;

    /**
     * Print performance report
     */
    void PrintPerformanceReport() const;

    // === Configuration ===

    /**
     * Set maximum concurrent bot logins
     */
    void SetMaxConcurrentLogins(uint32 max) { _maxConcurrentLogins = max; }

    /**
     * Set bot update interval
     */
    void SetUpdateInterval(uint32 intervalMs) { _updateInterval = intervalMs; }

    // === Event Broadcasting ===

    /**
     * Lifecycle event handler callback type
     */
    using LifecycleEventHandler = std::function<void(ObjectGuid, BotLifecycleState, BotLifecycleState)>;

    /**
     * Register for lifecycle events
     */
    void RegisterEventHandler(LifecycleEventHandler handler);

private:
    Player* _bot;

    // Bot storage
    std::unordered_map<ObjectGuid, std::shared_ptr<BotLifecycle>> _botLifecycles;
    

    // Configuration
    uint32 _maxConcurrentLogins = 10;
    uint32 _updateInterval = 100; // milliseconds

    // Statistics tracking
    
    std::chrono::steady_clock::time_point _lastStatsUpdate;
    static constexpr auto STATS_UPDATE_INTERVAL = std::chrono::seconds(1);

    // Event handlers
    std::vector<LifecycleEventHandler> _eventHandlers;
    Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_SPAWNER> _eventMutex;

    // Broadcast state change event
    void BroadcastStateChange(ObjectGuid botGuid, BotLifecycleState oldState, BotLifecycleState newState);
};

// Convenience accessor
#define sBotLifecycleMgr BotLifecycleManager::instance()

} // namespace Playerbot

#endif // BOT_LIFECYCLE_MANAGER_H