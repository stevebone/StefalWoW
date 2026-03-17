/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * @file InstanceBotOrchestrator.h
 * @brief Master orchestrator for instance bot management
 *
 * The InstanceBotOrchestrator is the central coordinator for all instance
 * bot operations. It manages:
 *
 * 1. Bot requests from LFG, BG, and Arena systems
 * 2. Pool and JIT factory coordination
 * 3. Instance lifecycle tracking
 * 4. Bot release and recycling
 *
 * Request Flow:
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │                    INSTANCE BOT ORCHESTRATOR                            │
 * ├─────────────────────────────────────────────────────────────────────────┤
 * │                                                                         │
 * │   REQUEST SOURCES                    ORCHESTRATOR                       │
 * │   ┌─────────────┐                    ┌──────────────────────────────┐  │
 * │   │ LFG Queue   │───────────────────▶│                              │  │
 * │   └─────────────┘                    │   Request Handler            │  │
 * │   ┌─────────────┐                    │   - Validate request         │  │
 * │   │ BG Queue    │───────────────────▶│   - Calculate bots needed    │  │
 * │   └─────────────┘                    │   - Check pool availability  │  │
 * │   ┌─────────────┐                    │                              │  │
 * │   │ Arena Queue │───────────────────▶│                              │  │
 * │   └─────────────┘                    └──────────┬───────────────────┘  │
 * │                                                 │                       │
 * │                                                 ▼                       │
 * │                                      ┌──────────────────────────────┐  │
 * │   BOT SOURCES                        │   Bot Allocation Strategy    │  │
 * │   ┌─────────────┐                    │                              │  │
 * │   │ Warm Pool   │◀───── Pool OK? ────│   1. Try warm pool first     │  │
 * │   └─────────────┘                    │   2. Use JIT factory if low  │  │
 * │   ┌─────────────┐                    │   3. Hybrid for large content│  │
 * │   │ JIT Factory │◀───── Overflow ────│                              │  │
 * │   └─────────────┘                    └──────────┬───────────────────┘  │
 * │                                                 │                       │
 * │                                                 ▼                       │
 * │                                      ┌──────────────────────────────┐  │
 * │   INSTANCE MANAGEMENT                │   Bot Delivery               │  │
 * │   ┌─────────────┐                    │   - Invoke callbacks         │  │
 * │   │ Tracking    │◀───────────────────│   - Track in instance        │  │
 * │   │ - Bots      │                    │   - Monitor progress         │  │
 * │   │ - Duration  │                    └──────────────────────────────┘  │
 * │   │ - State     │                                                      │
 * │   └─────────────┘                                                      │
 * │                                                                         │
 * └─────────────────────────────────────────────────────────────────────────┘
 *
 * Thread Safety:
 * - All public methods are thread-safe
 * - Internal state protected by mutex
 * - Callbacks invoked asynchronously when possible
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "PoolSlotState.h"
#include "ContentRequirements.h"
#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Playerbot
{

// Forward declarations
class InstanceBotPool;
class JITBotFactory;
class ContentRequirementDatabase;

// ============================================================================
// REQUEST TYPES
// ============================================================================

/**
 * @brief Base request structure
 */
struct OrchestratorRequest
{
    uint32 requestId = 0;
    InstanceType type = InstanceType::Dungeon;
    uint32 contentId = 0;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::milliseconds timeout{30000};

    virtual ~OrchestratorRequest() = default;
    virtual bool IsValid() const { return contentId > 0 || requestId > 0; }
};

/**
 * @brief Request for dungeon bots
 */
struct TC_GAME_API DungeonRequest : public OrchestratorRequest
{
    ObjectGuid playerGuid;              ///< Requesting player
    uint32 dungeonId = 0;               ///< LFG dungeon ID
    uint8 playerRole = 0;               ///< Player's selected role
    uint32 playerLevel = 1;             ///< Player's level - MUST be set by caller
    Faction playerFaction = Faction::Alliance;  ///< Player's faction

    /// Called when bots are ready
    std::function<void(std::vector<ObjectGuid> const& bots)> onBotsReady;
    /// Called if request fails
    std::function<void(std::string const& error)> onFailed;

    DungeonRequest()
    {
        type = InstanceType::Dungeon;
        timeout = std::chrono::milliseconds(30000);
    }

    bool IsValid() const override
    {
        return playerGuid != ObjectGuid::Empty && dungeonId > 0;
    }
};

/**
 * @brief Request for raid bots
 */
struct TC_GAME_API RaidRequest : public OrchestratorRequest
{
    ObjectGuid leaderGuid;              ///< Raid leader
    uint32 raidId = 0;                  ///< Raid map ID
    uint32 playerLevel = 1;             ///< Raid leader's level - MUST be set by caller
    Faction playerFaction = Faction::Alliance;  ///< Raid leader's faction
    std::vector<ObjectGuid> currentGroupMembers;  ///< Existing members
    std::map<ObjectGuid, uint8> memberRoles;      ///< GUID -> role mapping

    /// Called when bots are ready
    std::function<void(std::vector<ObjectGuid> const& bots)> onBotsReady;
    /// Called if request fails
    std::function<void(std::string const& error)> onFailed;

    RaidRequest()
    {
        type = InstanceType::Raid;
        timeout = std::chrono::milliseconds(60000);
    }

    bool IsValid() const override
    {
        return leaderGuid != ObjectGuid::Empty && raidId > 0;
    }
};

/**
 * @brief Request for battleground bots (BOTH FACTIONS)
 */
struct TC_GAME_API BattlegroundRequest : public OrchestratorRequest
{
    uint32 bgTypeId = 0;                ///< Battleground type
    uint32 bracketId = 0;               ///< Bracket ID (from PVPDifficultyEntry)
    uint32 playerLevel = 1;             ///< Player's level - MUST be set by caller
    uint32 currentAlliancePlayers = 0;  ///< Current Alliance count
    uint32 currentHordePlayers = 0;     ///< Current Horde count
    Faction playerFaction = Faction::Alliance;  ///< Human player's faction

    /// Called when bots are ready (both factions)
    std::function<void(std::vector<ObjectGuid> const& alliance,
                       std::vector<ObjectGuid> const& horde)> onBotsReady;
    /// Called if request fails
    std::function<void(std::string const& error)> onFailed;

    BattlegroundRequest()
    {
        type = InstanceType::Battleground;
        timeout = std::chrono::milliseconds(120000);
    }

    bool IsValid() const override
    {
        return bgTypeId > 0;
    }
};

/**
 * @brief Request for arena bots
 */
struct TC_GAME_API ArenaRequest : public OrchestratorRequest
{
    uint32 arenaType = 0;               ///< Arena type (2, 3, 5)
    uint32 bracketId = 0;               ///< Bracket ID (from PVPDifficultyEntry)
    uint32 playerLevel = 1;             ///< Player's level - MUST be set by caller
    ObjectGuid playerGuid;              ///< Player GUID
    Faction playerFaction = Faction::Alliance;  ///< Player's faction
    std::vector<ObjectGuid> existingTeammates;  ///< Already on team
    bool needOpponents = true;          ///< Create enemy team too

    /// Called when bots are ready
    std::function<void(std::vector<ObjectGuid> const& teammates,
                       std::vector<ObjectGuid> const& opponents)> onBotsReady;
    /// Called if request fails
    std::function<void(std::string const& error)> onFailed;

    ArenaRequest()
    {
        type = InstanceType::Arena;
        timeout = std::chrono::milliseconds(15000);
    }

    bool IsValid() const override
    {
        return arenaType >= 2 && arenaType <= 5 && playerGuid != ObjectGuid::Empty;
    }
};

// ============================================================================
// INSTANCE TRACKING
// ============================================================================

/**
 * @brief Tracks an active instance with bots
 */
struct InstanceInfo
{
    uint32 instanceId = 0;              ///< Instance ID
    InstanceType type = InstanceType::Dungeon;  ///< Instance type
    uint32 contentId = 0;               ///< Content ID (dungeon/raid/bg)
    std::string contentName;            ///< Human-readable name

    std::vector<ObjectGuid> assignedBots;       ///< All bots in instance
    std::vector<ObjectGuid> allianceBots;       ///< Alliance bots (PvP)
    std::vector<ObjectGuid> hordeBots;          ///< Horde bots (PvP)

    std::chrono::system_clock::time_point startTime;  ///< When instance started
    uint32 humanPlayerCount = 0;        ///< Number of human players

    /**
     * @brief Get total bot count
     */
    uint32 GetBotCount() const
    {
        return static_cast<uint32>(assignedBots.size());
    }

    /**
     * @brief Get instance duration
     */
    std::chrono::minutes GetDuration() const
    {
        auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::minutes>(now - startTime);
    }
};

// ============================================================================
// ORCHESTRATOR CONFIGURATION
// ============================================================================

/**
 * @brief Configuration for the instance bot orchestrator
 */
struct InstanceOrchestratorConfig
{
    bool enabled = true;

    // Pool thresholds
    uint32 useOverflowThresholdPct = 80;    ///< Use JIT when pool < X% full

    // Timeouts (in milliseconds)
    uint32 dungeonTimeoutMs = 30000;
    uint32 raidTimeoutMs = 60000;
    uint32 bgTimeoutMs = 120000;
    uint32 arenaTimeoutMs = 15000;

    // Behavior
    bool preferPoolBots = true;             ///< Prefer warm pool over JIT
    bool allowPartialFill = true;           ///< Start with partial bot count
    uint32 partialFillMinPct = 60;          ///< Minimum % to start partial

    // Logging
    bool logRequests = true;
    bool logAssignments = true;

    /**
     * @brief Load from config file
     */
    void LoadFromConfig();
};

// ============================================================================
// ORCHESTRATOR STATISTICS
// ============================================================================

/**
 * @brief Statistics for the orchestrator
 */
struct OrchestratorStatistics
{
    // Request counts
    uint32 pendingRequests = 0;
    uint32 activeInstances = 0;
    uint32 botsInInstances = 0;

    // Pool status
    uint32 poolBotsAvailable = 0;
    uint32 overflowBotsActive = 0;

    // Hourly activity
    uint32 dungeonsFilledThisHour = 0;
    uint32 raidsFilledThisHour = 0;
    uint32 battlegroundsFilledThisHour = 0;
    uint32 arenasFilledThisHour = 0;

    // Success metrics
    float requestSuccessRate = 1.0f;
    std::chrono::milliseconds avgFulfillmentTime{0};

    // Timing
    std::chrono::system_clock::time_point hourStart;
};

// ============================================================================
// INSTANCE BOT ORCHESTRATOR
// ============================================================================

/**
 * @brief Master orchestrator for all instance bot operations
 *
 * Singleton class that coordinates pool, JIT factory, and queue systems
 * to provide bots for all instance content.
 */
class TC_GAME_API InstanceBotOrchestrator
{
public:
    /**
     * @brief Get singleton instance
     */
    static InstanceBotOrchestrator* Instance();

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /**
     * @brief Initialize the orchestrator
     * @return true if successful
     */
    bool Initialize();

    /**
     * @brief Shutdown and cleanup
     */
    void Shutdown();

    /**
     * @brief Update (process queues)
     * @param diff Time since last update
     */
    void Update(uint32 diff);

    /**
     * @brief Load configuration
     */
    void LoadConfig();

    // ========================================================================
    // REQUEST API (Used by LFG/BG/Arena systems)
    // ========================================================================

    /**
     * @brief Request bots for LFG dungeon
     * @param request Dungeon request
     * @return Request ID for tracking
     */
    uint32 RequestDungeonBots(DungeonRequest const& request);

    /**
     * @brief Request bots for raid
     * @param request Raid request
     * @return Request ID for tracking
     */
    uint32 RequestRaidBots(RaidRequest const& request);

    /**
     * @brief Request bots for battleground (BOTH FACTIONS)
     * @param request Battleground request
     * @return Request ID for tracking
     */
    uint32 RequestBattlegroundBots(BattlegroundRequest const& request);

    /**
     * @brief Request bots for arena
     * @param request Arena request
     * @return Request ID for tracking
     */
    uint32 RequestArenaBots(ArenaRequest const& request);

    /**
     * @brief Cancel any pending request
     * @param requestId Request to cancel
     */
    void CancelRequest(uint32 requestId);

    /**
     * @brief Cancel all pending requests for a player
     * @param playerGuid Player whose requests should be cancelled
     */
    void CancelRequestsForPlayer(ObjectGuid playerGuid);

    // ========================================================================
    // INSTANCE LIFECYCLE
    // ========================================================================

    /**
     * @brief Called when instance is created
     * @param instanceId Instance ID
     * @param type Instance type
     * @param contentId Content ID
     */
    void OnInstanceCreated(uint32 instanceId, InstanceType type, uint32 contentId);

    /**
     * @brief Called when instance ends
     * @param instanceId Instance ID
     */
    void OnInstanceEnded(uint32 instanceId);

    /**
     * @brief Called when player leaves instance
     * @param playerGuid Player who left
     * @param instanceId Instance ID
     */
    void OnPlayerLeftInstance(ObjectGuid playerGuid, uint32 instanceId);

    /**
     * @brief Remove specific bot from instance
     * @param botGuid Bot to remove
     * @param instanceId Instance ID
     */
    void RemoveBotFromInstance(ObjectGuid botGuid, uint32 instanceId);

    // ========================================================================
    // QUERIES
    // ========================================================================

    /**
     * @brief Check if enabled
     */
    bool IsEnabled() const { return _config.enabled && _initialized.load(); }

    /**
     * @brief Check if bots can be provided for content
     * @param type Instance type
     * @param contentId Content ID
     * @return true if possible
     */
    bool CanProvideBotsFor(InstanceType type, uint32 contentId) const;

    /**
     * @brief Get estimated wait time for content
     * @param type Instance type
     * @param contentId Content ID
     * @param playersAlreadyQueued Players already waiting
     * @return Estimated wait time
     */
    std::chrono::seconds GetEstimatedWaitTime(
        InstanceType type,
        uint32 contentId,
        uint32 playersAlreadyQueued = 1) const;

    /**
     * @brief Get bots currently in instance
     * @param instanceId Instance ID
     * @return Vector of bot GUIDs
     */
    std::vector<ObjectGuid> GetBotsInInstance(uint32 instanceId) const;

    /**
     * @brief Check if bot is from orchestrator
     * @param botGuid Bot to check
     * @return true if managed by orchestrator
     */
    bool IsManagedBot(ObjectGuid botGuid) const;

    /**
     * @brief Get instance info
     * @param instanceId Instance ID
     * @return Instance info or nullptr
     */
    InstanceInfo const* GetInstanceInfo(uint32 instanceId) const;

    // ========================================================================
    // STATISTICS
    // ========================================================================

    /**
     * @brief Get current statistics
     */
    OrchestratorStatistics GetStatistics() const;

    /**
     * @brief Print status report to log
     */
    void PrintStatusReport() const;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /**
     * @brief Get current configuration
     */
    InstanceOrchestratorConfig const& GetConfig() const { return _config; }

    /**
     * @brief Set configuration
     */
    void SetConfig(InstanceOrchestratorConfig const& config);

private:
    InstanceBotOrchestrator() = default;
    ~InstanceBotOrchestrator() = default;
    InstanceBotOrchestrator(InstanceBotOrchestrator const&) = delete;
    InstanceBotOrchestrator& operator=(InstanceBotOrchestrator const&) = delete;

    // ========================================================================
    // INTERNAL METHODS - Request Processing
    // ========================================================================

    /**
     * @brief Process pending dungeon requests
     */
    void ProcessDungeonRequests();

    /**
     * @brief Process pending raid requests
     */
    void ProcessRaidRequests();

    /**
     * @brief Process pending battleground requests
     */
    void ProcessBattlegroundRequests();

    /**
     * @brief Process pending arena requests
     */
    void ProcessArenaRequests();

    /**
     * @brief Fulfill a dungeon request
     */
    bool FulfillDungeonRequest(DungeonRequest& request);

    /**
     * @brief Fulfill a raid request
     */
    bool FulfillRaidRequest(RaidRequest& request);

    /**
     * @brief Fulfill a battleground request
     */
    bool FulfillBattlegroundRequest(BattlegroundRequest& request);

    /**
     * @brief Fulfill an arena request
     */
    bool FulfillArenaRequest(ArenaRequest& request);

    // ========================================================================
    // INTERNAL METHODS - Bot Selection
    // ========================================================================

    /**
     * @brief Select bots from pool
     */
    std::vector<ObjectGuid> SelectBotsFromPool(
        BotRole role,
        uint32 count,
        Faction faction,
        uint32 level,
        uint32 minGearScore);

    /**
     * @brief Create overflow bots via JIT factory
     */
    std::vector<ObjectGuid> CreateOverflowBots(
        BotRole role,
        uint32 count,
        Faction faction,
        uint32 level,
        uint32 minGearScore);

    /**
     * @brief Decide whether to use pool or JIT
     */
    bool ShouldUseOverflow(BotRole role, Faction faction, uint32 count) const;

    // ========================================================================
    // INTERNAL METHODS - Instance Management
    // ========================================================================

    /**
     * @brief Track bots in instance
     */
    void TrackBotsInInstance(
        uint32 instanceId,
        std::vector<ObjectGuid> const& bots);

    /**
     * @brief Release bots from instance
     */
    void ReleaseBotsFromInstance(uint32 instanceId);

    /**
     * @brief Process request timeouts
     */
    void ProcessTimeouts();

    // ========================================================================
    // DATA MEMBERS - Request Queues
    // ========================================================================

    std::queue<DungeonRequest> _dungeonQueue;
    std::queue<RaidRequest> _raidQueue;
    std::queue<BattlegroundRequest> _bgQueue;
    std::queue<ArenaRequest> _arenaQueue;
    mutable std::mutex _queueMutex;

    std::atomic<uint32> _nextRequestId{1};

    // ========================================================================
    // DATA MEMBERS - Instance Tracking
    // ========================================================================

    std::unordered_map<uint32, InstanceInfo> _activeInstances;
    std::unordered_set<ObjectGuid> _managedBots;
    mutable std::mutex _instanceMutex;

    // ========================================================================
    // DATA MEMBERS - Configuration
    // ========================================================================

    InstanceOrchestratorConfig _config;

    // ========================================================================
    // DATA MEMBERS - Statistics
    // ========================================================================

    mutable OrchestratorStatistics _stats;
    std::atomic<uint32> _dungeonsFilledThisHour{0};
    std::atomic<uint32> _raidsFilledThisHour{0};
    std::atomic<uint32> _bgsFilledThisHour{0};
    std::atomic<uint32> _arenasFilledThisHour{0};
    std::atomic<uint32> _requestsSucceeded{0};
    std::atomic<uint32> _requestsFailed{0};

    std::chrono::milliseconds _avgFulfillmentTime{0};
    uint32 _fulfillmentSamples = 0;
    mutable std::mutex _statsMutex;

    std::chrono::system_clock::time_point _hourStart;

    // ========================================================================
    // DATA MEMBERS - Timing
    // ========================================================================

    uint32 _updateAccumulator = 0;
    static constexpr uint32 UPDATE_INTERVAL_MS = 100;

    // ========================================================================
    // DATA MEMBERS - State
    // ========================================================================

    std::atomic<bool> _initialized{false};
};

} // namespace Playerbot

#define sInstanceBotOrchestrator Playerbot::InstanceBotOrchestrator::Instance()
