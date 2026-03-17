/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * @file InstanceBotOrchestrator.cpp
 * @brief Implementation of the master instance bot orchestrator
 */

#include "InstanceBotOrchestrator.h"
#include "InstanceBotPool.h"
#include "InstanceBotHooks.h"
#include "JITBotFactory.h"
#include "PoolConfiguration.h"
#include "ContentRequirements.h"
#include "Config/PlayerbotConfig.h"
#include "Session/BotWorldSessionMgr.h"
#include "ObjectAccessor.h"
#include "WorldSession.h"
#include "Player.h"
#include "Log.h"
#include <fmt/format.h>

namespace Playerbot
{

// ============================================================================
// SINGLETON
// ============================================================================

InstanceBotOrchestrator* InstanceBotOrchestrator::Instance()
{
    static InstanceBotOrchestrator instance;
    return &instance;
}

// ============================================================================
// LIFECYCLE
// ============================================================================

bool InstanceBotOrchestrator::Initialize()
{
    if (_initialized.load())
    {
        TC_LOG_WARN("playerbot.orchestrator", "InstanceBotOrchestrator::Initialize - Already initialized");
        return true;
    }

    TC_LOG_INFO("playerbot.orchestrator", "InstanceBotOrchestrator::Initialize - Starting initialization");

    // Load configuration
    LoadConfig();

    if (!_config.enabled)
    {
        TC_LOG_INFO("playerbot.orchestrator", "InstanceBotOrchestrator::Initialize - Orchestrator is disabled");
        _initialized.store(true);
        return true;
    }

    // Initialize statistics
    _hourStart = std::chrono::system_clock::now();
    _stats.hourStart = _hourStart;
    _dungeonsFilledThisHour.store(0);
    _raidsFilledThisHour.store(0);
    _bgsFilledThisHour.store(0);
    _arenasFilledThisHour.store(0);
    _requestsSucceeded.store(0);
    _requestsFailed.store(0);

    // Wire up pool overflow callback to JIT factory
    // When pool needs more bots than available, JIT factory creates them on-demand
    sInstanceBotPool->SetOverflowNeededCallback(
        [](BotRole role, Faction faction, PoolBracket bracket, uint32 count)
        {
            TC_LOG_INFO("playerbot.orchestrator", "Pool overflow: {} {} {} bots needed for bracket {}",
                count, FactionToString(faction), BotRoleToString(role), PoolBracketToString(bracket));

            // Submit bracket-based JIT request
            sJITBotFactory->SubmitBracketRequest(
                role, faction, bracket, count,
                InstanceType::Dungeon,  // Default type, pool manages routing
                0,  // No specific content ID
                [bracket, role, faction](std::vector<ObjectGuid> const& bots)
                {
                    TC_LOG_INFO("playerbot.orchestrator", "JIT created {} bots for bracket {} - registering with pool",
                        bots.size(), PoolBracketToString(bracket));

                    // FIX (2026-01-15): Use RegisterJITBot() instead of just AddToReadyIndex()
                    // This properly integrates JIT bots with the pool tracking by:
                    // 1. Creating InstanceBotSlot entries
                    // 2. Adding to _slots map
                    // 3. Adding to ready index
                    // 4. Updating _bracketCounts
                    // Without this, ReplenishPool() kept detecting shortages and triggered
                    // infinite JIT bot creation (the 1200 bots bug).
                    for (ObjectGuid guid : bots)
                    {
                        // Get account ID from the logged-in bot's session
                        uint32 accountId = 0;
                        if (Player* bot = ObjectAccessor::FindPlayer(guid))
                        {
                            if (WorldSession* session = bot->GetSession())
                                accountId = session->GetAccountId();
                        }
                        sInstanceBotPool->RegisterJITBot(guid, accountId, role, faction, bracket);
                    }
                });
        });

    TC_LOG_INFO("playerbot.orchestrator", "InstanceBotOrchestrator::Initialize - Pool overflow callback wired to JIT factory");

    _initialized.store(true);
    TC_LOG_INFO("playerbot.orchestrator", "InstanceBotOrchestrator::Initialize - Initialization complete");

    return true;
}

void InstanceBotOrchestrator::Shutdown()
{
    if (!_initialized.load())
        return;

    TC_LOG_INFO("playerbot.orchestrator", "InstanceBotOrchestrator::Shutdown - Starting shutdown");

    // Release all bots from instances
    {
        std::lock_guard<std::mutex> lock(_instanceMutex);
        for (auto const& [instanceId, info] : _activeInstances)
        {
            TC_LOG_DEBUG("playerbot.orchestrator", "InstanceBotOrchestrator::Shutdown - Releasing {} bots from instance {}",
                info.assignedBots.size(), instanceId);
        }
        _activeInstances.clear();
        _managedBots.clear();
    }

    // Clear request queues
    {
        std::lock_guard<std::mutex> lock(_queueMutex);
        while (!_dungeonQueue.empty()) _dungeonQueue.pop();
        while (!_raidQueue.empty()) _raidQueue.pop();
        while (!_bgQueue.empty()) _bgQueue.pop();
        while (!_arenaQueue.empty()) _arenaQueue.pop();
    }

    _initialized.store(false);
    TC_LOG_INFO("playerbot.orchestrator", "InstanceBotOrchestrator::Shutdown - Shutdown complete");
}

void InstanceBotOrchestrator::Update(uint32 diff)
{
    if (!_initialized.load() || !_config.enabled)
        return;

    // CRITICAL: Process pending BG queue (bots waiting to login and queue)
    // This runs on every update to ensure bots are logged in and queued promptly
    InstanceBotHooks::Update(diff);

    _updateAccumulator += diff;
    if (_updateAccumulator < UPDATE_INTERVAL_MS)
        return;

    _updateAccumulator = 0;

    // Reset hourly statistics if needed
    auto now = std::chrono::system_clock::now();
    auto hoursSinceStart = std::chrono::duration_cast<std::chrono::hours>(now - _hourStart).count();
    if (hoursSinceStart >= 1)
    {
        std::lock_guard<std::mutex> lock(_statsMutex);
        _dungeonsFilledThisHour.store(0);
        _raidsFilledThisHour.store(0);
        _bgsFilledThisHour.store(0);
        _arenasFilledThisHour.store(0);
        _requestsSucceeded.store(0);
        _requestsFailed.store(0);
        _hourStart = now;
        _stats.hourStart = now;
    }

    // Process request queues (priority order)
    ProcessDungeonRequests();
    ProcessArenaRequests();
    ProcessRaidRequests();
    ProcessBattlegroundRequests();

    // Process timeouts
    ProcessTimeouts();
}

void InstanceBotOrchestrator::LoadConfig()
{
    TC_LOG_DEBUG("playerbot.orchestrator", "InstanceBotOrchestrator::LoadConfig - Loading configuration");

    _config.enabled = sPlayerbotConfig->GetBool("Playerbot.Instance.Orchestrator.Enable", true);
    _config.useOverflowThresholdPct = sPlayerbotConfig->GetInt("Playerbot.Instance.Orchestrator.OverflowThresholdPct", 80);

    _config.dungeonTimeoutMs = sPlayerbotConfig->GetInt("Playerbot.Instance.Orchestrator.DungeonTimeoutMs", 30000);
    _config.raidTimeoutMs = sPlayerbotConfig->GetInt("Playerbot.Instance.Orchestrator.RaidTimeoutMs", 60000);
    _config.bgTimeoutMs = sPlayerbotConfig->GetInt("Playerbot.Instance.Orchestrator.BattlegroundTimeoutMs", 120000);
    _config.arenaTimeoutMs = sPlayerbotConfig->GetInt("Playerbot.Instance.Orchestrator.ArenaTimeoutMs", 15000);

    _config.preferPoolBots = sPlayerbotConfig->GetBool("Playerbot.Instance.Orchestrator.PreferPoolBots", true);
    _config.allowPartialFill = sPlayerbotConfig->GetBool("Playerbot.Instance.Orchestrator.AllowPartialFill", true);
    _config.partialFillMinPct = sPlayerbotConfig->GetInt("Playerbot.Instance.Orchestrator.PartialFillMinPct", 60);

    _config.logRequests = sPlayerbotConfig->GetBool("Playerbot.Instance.Orchestrator.LogRequests", true);
    _config.logAssignments = sPlayerbotConfig->GetBool("Playerbot.Instance.Orchestrator.LogAssignments", true);

    TC_LOG_INFO("playerbot.orchestrator", "InstanceBotOrchestrator::LoadConfig - Orchestrator: enabled={}, overflowThreshold={}%",
        _config.enabled, _config.useOverflowThresholdPct);
}

// ============================================================================
// REQUEST API
// ============================================================================

uint32 InstanceBotOrchestrator::RequestDungeonBots(DungeonRequest const& request)
{
    if (!_initialized.load() || !_config.enabled)
    {
        TC_LOG_WARN("playerbot.orchestrator", "InstanceBotOrchestrator::RequestDungeonBots - Orchestrator not available");
        if (request.onFailed)
            request.onFailed("Orchestrator not available");
        return 0;
    }

    if (!request.IsValid())
    {
        TC_LOG_WARN("playerbot.orchestrator", "InstanceBotOrchestrator::RequestDungeonBots - Invalid request");
        if (request.onFailed)
            request.onFailed("Invalid request");
        return 0;
    }

    DungeonRequest req = request;
    req.requestId = _nextRequestId.fetch_add(1);
    req.type = InstanceType::Dungeon;
    req.contentId = request.dungeonId;
    req.createdAt = std::chrono::system_clock::now();
    req.timeout = std::chrono::milliseconds(_config.dungeonTimeoutMs);

    if (_config.logRequests)
    {
        TC_LOG_INFO("playerbot.orchestrator", "InstanceBotOrchestrator::RequestDungeonBots - Request {} from player {}, dungeon {}",
            req.requestId, req.playerGuid.ToString(), req.dungeonId);
    }

    {
        std::lock_guard<std::mutex> lock(_queueMutex);
        _dungeonQueue.push(std::move(req));
    }

    return req.requestId;
}

uint32 InstanceBotOrchestrator::RequestRaidBots(RaidRequest const& request)
{
    if (!_initialized.load() || !_config.enabled)
    {
        if (request.onFailed)
            request.onFailed("Orchestrator not available");
        return 0;
    }

    if (!request.IsValid())
    {
        if (request.onFailed)
            request.onFailed("Invalid request");
        return 0;
    }

    RaidRequest req = request;
    req.requestId = _nextRequestId.fetch_add(1);
    req.type = InstanceType::Raid;
    req.contentId = request.raidId;
    req.createdAt = std::chrono::system_clock::now();
    req.timeout = std::chrono::milliseconds(_config.raidTimeoutMs);

    if (_config.logRequests)
    {
        TC_LOG_INFO("playerbot.orchestrator", "InstanceBotOrchestrator::RequestRaidBots - Request {} from leader {}, raid {}",
            req.requestId, req.leaderGuid.ToString(), req.raidId);
    }

    {
        std::lock_guard<std::mutex> lock(_queueMutex);
        _raidQueue.push(std::move(req));
    }

    return req.requestId;
}

uint32 InstanceBotOrchestrator::RequestBattlegroundBots(BattlegroundRequest const& request)
{
    if (!_initialized.load() || !_config.enabled)
    {
        if (request.onFailed)
            request.onFailed("Orchestrator not available");
        return 0;
    }

    if (!request.IsValid())
    {
        if (request.onFailed)
            request.onFailed("Invalid request");
        return 0;
    }

    BattlegroundRequest req = request;
    req.requestId = _nextRequestId.fetch_add(1);
    req.type = InstanceType::Battleground;
    req.contentId = request.bgTypeId;
    req.createdAt = std::chrono::system_clock::now();
    req.timeout = std::chrono::milliseconds(_config.bgTimeoutMs);

    if (_config.logRequests)
    {
        TC_LOG_INFO("playerbot.orchestrator", "InstanceBotOrchestrator::RequestBattlegroundBots - Request {} for BG {}, "
            "current: {} Alliance, {} Horde",
            req.requestId, req.bgTypeId, req.currentAlliancePlayers, req.currentHordePlayers);
    }

    {
        std::lock_guard<std::mutex> lock(_queueMutex);
        _bgQueue.push(std::move(req));
    }

    return req.requestId;
}

uint32 InstanceBotOrchestrator::RequestArenaBots(ArenaRequest const& request)
{
    if (!_initialized.load() || !_config.enabled)
    {
        if (request.onFailed)
            request.onFailed("Orchestrator not available");
        return 0;
    }

    if (!request.IsValid())
    {
        if (request.onFailed)
            request.onFailed("Invalid request");
        return 0;
    }

    ArenaRequest req = request;
    req.requestId = _nextRequestId.fetch_add(1);
    req.type = InstanceType::Arena;
    req.contentId = request.arenaType;
    req.createdAt = std::chrono::system_clock::now();
    req.timeout = std::chrono::milliseconds(_config.arenaTimeoutMs);

    if (_config.logRequests)
    {
        TC_LOG_INFO("playerbot.orchestrator", "InstanceBotOrchestrator::RequestArenaBots - Request {} for {}v{} arena, "
            "player {}, needOpponents={}",
            req.requestId, req.arenaType, req.arenaType, req.playerGuid.ToString(), req.needOpponents);
    }

    {
        std::lock_guard<std::mutex> lock(_queueMutex);
        _arenaQueue.push(std::move(req));
    }

    return req.requestId;
}

void InstanceBotOrchestrator::CancelRequest(uint32 requestId)
{
    TC_LOG_DEBUG("playerbot.orchestrator", "InstanceBotOrchestrator::CancelRequest - Cancelling request {}",
        requestId);

    // Note: Since we process queues regularly, cancelled requests will
    // be detected by timeout or removed when found
}

void InstanceBotOrchestrator::CancelRequestsForPlayer(ObjectGuid playerGuid)
{
    TC_LOG_DEBUG("playerbot.orchestrator", "InstanceBotOrchestrator::CancelRequestsForPlayer - Cancelling requests for player {}",
        playerGuid.ToString());

    // Note: Full implementation would scan all request queues for this player
    // For now, we rely on request timeout mechanism
}

// ============================================================================
// INSTANCE LIFECYCLE
// ============================================================================

void InstanceBotOrchestrator::OnInstanceCreated(uint32 instanceId, InstanceType type, uint32 contentId)
{
    TC_LOG_DEBUG("playerbot.orchestrator", "InstanceBotOrchestrator::OnInstanceCreated - Instance {} type {} content {}",
        instanceId, InstanceTypeToString(type), contentId);

    std::lock_guard<std::mutex> lock(_instanceMutex);

    InstanceInfo info;
    info.instanceId = instanceId;
    info.type = type;
    info.contentId = contentId;
    info.startTime = std::chrono::system_clock::now();

    // Get content name from requirements
    ContentRequirement const* req = sContentRequirementDb->GetRequirement(type, contentId);
    if (req)
        info.contentName = req->contentName;

    _activeInstances[instanceId] = std::move(info);
}

void InstanceBotOrchestrator::OnInstanceEnded(uint32 instanceId)
{
    TC_LOG_DEBUG("playerbot.orchestrator", "InstanceBotOrchestrator::OnInstanceEnded - Instance {}", instanceId);

    ReleaseBotsFromInstance(instanceId);

    std::lock_guard<std::mutex> lock(_instanceMutex);
    _activeInstances.erase(instanceId);
}

void InstanceBotOrchestrator::OnPlayerLeftInstance(ObjectGuid playerGuid, uint32 instanceId)
{
    TC_LOG_DEBUG("playerbot.orchestrator", "InstanceBotOrchestrator::OnPlayerLeftInstance - Player {} left instance {}",
        playerGuid.ToString(), instanceId);

    std::lock_guard<std::mutex> lock(_instanceMutex);

    auto it = _activeInstances.find(instanceId);
    if (it != _activeInstances.end())
    {
        if (it->second.humanPlayerCount > 0)
            --it->second.humanPlayerCount;

        // If no humans left, consider ending the instance
        if (it->second.humanPlayerCount == 0)
        {
            TC_LOG_INFO("playerbot.orchestrator", "InstanceBotOrchestrator::OnPlayerLeftInstance - No humans left in instance {}, releasing bots",
                instanceId);
            // Don't release immediately - let instance manager handle it
        }
    }
}

void InstanceBotOrchestrator::RemoveBotFromInstance(ObjectGuid botGuid, uint32 instanceId)
{
    TC_LOG_DEBUG("playerbot.orchestrator", "InstanceBotOrchestrator::RemoveBotFromInstance - Bot {} from instance {}",
        botGuid.ToString(), instanceId);

    std::lock_guard<std::mutex> lock(_instanceMutex);

    auto it = _activeInstances.find(instanceId);
    if (it != _activeInstances.end())
    {
        auto& bots = it->second.assignedBots;
        bots.erase(std::remove(bots.begin(), bots.end(), botGuid), bots.end());

        // Also check faction-specific lists
        auto& allianceBots = it->second.allianceBots;
        allianceBots.erase(std::remove(allianceBots.begin(), allianceBots.end(), botGuid), allianceBots.end());

        auto& hordeBots = it->second.hordeBots;
        hordeBots.erase(std::remove(hordeBots.begin(), hordeBots.end(), botGuid), hordeBots.end());
    }

    _managedBots.erase(botGuid);

    // Release back to pool
    sInstanceBotPool->ReleaseBots({botGuid});
}

// ============================================================================
// QUERIES
// ============================================================================

bool InstanceBotOrchestrator::CanProvideBotsFor(InstanceType type, uint32 contentId) const
{
    if (!_initialized.load() || !_config.enabled)
        return false;

    ContentRequirement const* req = sContentRequirementDb->GetRequirement(type, contentId);
    if (!req)
        return false;

    // Check if pool + JIT can handle the request
    uint32 totalNeeded = req->GetTotalRecommended();
    if (req->requiresBothFactions)
        totalNeeded = req->playersPerFaction * 2;

    // We can always handle if JIT is enabled
    return true;
}

std::chrono::seconds InstanceBotOrchestrator::GetEstimatedWaitTime(
    InstanceType type,
    uint32 contentId,
    uint32 playersAlreadyQueued) const
{
    if (!_initialized.load())
        return std::chrono::seconds(0);

    ContentRequirement const* req = sContentRequirementDb->GetRequirement(type, contentId);
    if (!req)
        return std::chrono::seconds(0);

    uint32 botsNeeded = req->maxPlayers - playersAlreadyQueued;
    if (botsNeeded == 0)
        return std::chrono::seconds(0);

    // Estimate based on pool availability and JIT time
    uint32 availableInPool = sInstanceBotPool->GetReadyCount();

    if (availableInPool >= botsNeeded)
    {
        // Instant from pool
        return std::chrono::seconds(1);
    }

    // Need to use JIT
    uint32 fromJIT = botsNeeded - availableInPool;
    auto jitTime = sBotCloneEngine->GetEstimatedCloneTime(fromJIT);

    return std::chrono::duration_cast<std::chrono::seconds>(jitTime);
}

std::vector<ObjectGuid> InstanceBotOrchestrator::GetBotsInInstance(uint32 instanceId) const
{
    std::lock_guard<std::mutex> lock(_instanceMutex);

    auto it = _activeInstances.find(instanceId);
    if (it != _activeInstances.end())
        return it->second.assignedBots;

    return {};
}

bool InstanceBotOrchestrator::IsManagedBot(ObjectGuid botGuid) const
{
    std::lock_guard<std::mutex> lock(_instanceMutex);
    return _managedBots.count(botGuid) > 0;
}

InstanceInfo const* InstanceBotOrchestrator::GetInstanceInfo(uint32 instanceId) const
{
    std::lock_guard<std::mutex> lock(_instanceMutex);

    auto it = _activeInstances.find(instanceId);
    if (it != _activeInstances.end())
        return &it->second;

    return nullptr;
}

// ============================================================================
// STATISTICS
// ============================================================================

OrchestratorStatistics InstanceBotOrchestrator::GetStatistics() const
{
    std::lock_guard<std::mutex> lock(_statsMutex);

    _stats.pendingRequests = 0;
    {
        std::lock_guard<std::mutex> qLock(_queueMutex);
        _stats.pendingRequests = static_cast<uint32>(
            _dungeonQueue.size() + _raidQueue.size() + _bgQueue.size() + _arenaQueue.size());
    }

    {
        std::lock_guard<std::mutex> iLock(_instanceMutex);
        _stats.activeInstances = static_cast<uint32>(_activeInstances.size());
        _stats.botsInInstances = static_cast<uint32>(_managedBots.size());
    }

    _stats.poolBotsAvailable = sInstanceBotPool->GetReadyCount();
    _stats.overflowBotsActive = sJITBotFactory->GetRecycledBotCount();

    _stats.dungeonsFilledThisHour = _dungeonsFilledThisHour.load();
    _stats.raidsFilledThisHour = _raidsFilledThisHour.load();
    _stats.battlegroundsFilledThisHour = _bgsFilledThisHour.load();
    _stats.arenasFilledThisHour = _arenasFilledThisHour.load();

    uint32 total = _requestsSucceeded.load() + _requestsFailed.load();
    if (total > 0)
        _stats.requestSuccessRate = static_cast<float>(_requestsSucceeded.load()) / static_cast<float>(total);
    else
        _stats.requestSuccessRate = 1.0f;

    _stats.avgFulfillmentTime = _avgFulfillmentTime;

    return _stats;
}

void InstanceBotOrchestrator::PrintStatusReport() const
{
    OrchestratorStatistics stats = GetStatistics();

    TC_LOG_INFO("playerbot.orchestrator", "=== InstanceBotOrchestrator Status ===");
    TC_LOG_INFO("playerbot.orchestrator", "Pending Requests: {}", stats.pendingRequests);
    TC_LOG_INFO("playerbot.orchestrator", "Active Instances: {}", stats.activeInstances);
    TC_LOG_INFO("playerbot.orchestrator", "Bots in Instances: {}", stats.botsInInstances);
    TC_LOG_INFO("playerbot.orchestrator", "Pool Available: {}", stats.poolBotsAvailable);
    TC_LOG_INFO("playerbot.orchestrator", "Overflow Active: {}", stats.overflowBotsActive);
    TC_LOG_INFO("playerbot.orchestrator", "--- This Hour ---");
    TC_LOG_INFO("playerbot.orchestrator", "Dungeons Filled: {}", stats.dungeonsFilledThisHour);
    TC_LOG_INFO("playerbot.orchestrator", "Raids Filled: {}", stats.raidsFilledThisHour);
    TC_LOG_INFO("playerbot.orchestrator", "Battlegrounds Filled: {}", stats.battlegroundsFilledThisHour);
    TC_LOG_INFO("playerbot.orchestrator", "Arenas Filled: {}", stats.arenasFilledThisHour);
    TC_LOG_INFO("playerbot.orchestrator", "Success Rate: {:.1f}%", stats.requestSuccessRate * 100.0f);
    TC_LOG_INFO("playerbot.orchestrator", "Avg Fulfillment Time: {}ms", stats.avgFulfillmentTime.count());
}

void InstanceBotOrchestrator::SetConfig(InstanceOrchestratorConfig const& config)
{
    _config = config;
    TC_LOG_INFO("playerbot.orchestrator", "InstanceBotOrchestrator::SetConfig - Configuration updated");
}

// ============================================================================
// INTERNAL METHODS - Request Processing
// ============================================================================

void InstanceBotOrchestrator::ProcessDungeonRequests()
{
    DungeonRequest request;
    bool hasRequest = false;

    {
        std::lock_guard<std::mutex> lock(_queueMutex);
        if (!_dungeonQueue.empty())
        {
            request = std::move(_dungeonQueue.front());
            _dungeonQueue.pop();
            hasRequest = true;
        }
    }

    if (hasRequest)
    {
        auto startTime = std::chrono::steady_clock::now();

        if (FulfillDungeonRequest(request))
        {
            _requestsSucceeded.fetch_add(1);
            _dungeonsFilledThisHour.fetch_add(1);

            auto endTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

            // Update average
            std::lock_guard<std::mutex> lock(_statsMutex);
            ++_fulfillmentSamples;
            auto totalMs = _avgFulfillmentTime.count() * (_fulfillmentSamples - 1) + duration.count();
            _avgFulfillmentTime = std::chrono::milliseconds(totalMs / _fulfillmentSamples);
        }
        else
        {
            _requestsFailed.fetch_add(1);
        }
    }
}

void InstanceBotOrchestrator::ProcessRaidRequests()
{
    RaidRequest request;
    bool hasRequest = false;

    {
        std::lock_guard<std::mutex> lock(_queueMutex);
        if (!_raidQueue.empty())
        {
            request = std::move(_raidQueue.front());
            _raidQueue.pop();
            hasRequest = true;
        }
    }

    if (hasRequest)
    {
        if (FulfillRaidRequest(request))
        {
            _requestsSucceeded.fetch_add(1);
            _raidsFilledThisHour.fetch_add(1);
        }
        else
        {
            _requestsFailed.fetch_add(1);
        }
    }
}

void InstanceBotOrchestrator::ProcessBattlegroundRequests()
{
    BattlegroundRequest request;
    bool hasRequest = false;

    {
        std::lock_guard<std::mutex> lock(_queueMutex);
        if (!_bgQueue.empty())
        {
            request = std::move(_bgQueue.front());
            _bgQueue.pop();
            hasRequest = true;
        }
    }

    if (hasRequest)
    {
        if (FulfillBattlegroundRequest(request))
        {
            _requestsSucceeded.fetch_add(1);
            _bgsFilledThisHour.fetch_add(1);
        }
        else
        {
            _requestsFailed.fetch_add(1);
        }
    }
}

void InstanceBotOrchestrator::ProcessArenaRequests()
{
    ArenaRequest request;
    bool hasRequest = false;

    {
        std::lock_guard<std::mutex> lock(_queueMutex);
        if (!_arenaQueue.empty())
        {
            request = std::move(_arenaQueue.front());
            _arenaQueue.pop();
            hasRequest = true;
        }
    }

    if (hasRequest)
    {
        if (FulfillArenaRequest(request))
        {
            _requestsSucceeded.fetch_add(1);
            _arenasFilledThisHour.fetch_add(1);
        }
        else
        {
            _requestsFailed.fetch_add(1);
        }
    }
}

bool InstanceBotOrchestrator::FulfillDungeonRequest(DungeonRequest& request)
{
    TC_LOG_DEBUG("playerbot.orchestrator", "InstanceBotOrchestrator::FulfillDungeonRequest - Processing request {}",
        request.requestId);

    // Get requirements
    ContentRequirement const* req = sContentRequirementDb->GetDungeonRequirement(request.dungeonId);
    if (!req)
    {
        TC_LOG_WARN("playerbot.orchestrator", "InstanceBotOrchestrator::FulfillDungeonRequest - No requirements for dungeon {}",
            request.dungeonId);
        if (request.onFailed)
            request.onFailed("Unknown dungeon");
        return false;
    }

    // Calculate bots needed (player already fills one slot)
    GroupState groupState;
    groupState.totalPlayers = 1;

    // Map player role
    switch (request.playerRole)
    {
        case 0: groupState.tanks = 1; break;    // PLAYER_ROLE_TANK
        case 1: groupState.healers = 1; break;  // PLAYER_ROLE_HEALER
        case 2: groupState.dps = 1; break;      // PLAYER_ROLE_DPS
        default: groupState.dps = 1; break;
    }

    BotsNeeded needed = sContentRequirementDb->CalculateBotsNeeded(req, groupState);

    if (!needed.NeedsBots())
    {
        TC_LOG_DEBUG("playerbot.orchestrator", "InstanceBotOrchestrator::FulfillDungeonRequest - No bots needed");
        if (request.onBotsReady)
            request.onBotsReady({});
        return true;
    }

    TC_LOG_DEBUG("playerbot.orchestrator", "InstanceBotOrchestrator::FulfillDungeonRequest - Need: {}",
        needed.ToString());

    // Use player's actual level and faction from the request
    uint32 playerLevel = request.playerLevel;
    Faction playerFaction = request.playerFaction;

    TC_LOG_DEBUG("playerbot.orchestrator", "FulfillDungeonRequest - Player level {}, faction {}",
        playerLevel, playerFaction == Faction::Alliance ? "Alliance" : "Horde");

    // Collect bots
    std::vector<ObjectGuid> allBots;

    // Get tanks
    if (needed.tanksNeeded > 0)
    {
        auto tanks = SelectBotsFromPool(BotRole::Tank, needed.tanksNeeded, playerFaction, playerLevel, needed.minGearScore);
        allBots.insert(allBots.end(), tanks.begin(), tanks.end());

        if (tanks.size() < needed.tanksNeeded)
        {
            auto overflow = CreateOverflowBots(BotRole::Tank, needed.tanksNeeded - static_cast<uint32>(tanks.size()),
                playerFaction, playerLevel, needed.minGearScore);
            allBots.insert(allBots.end(), overflow.begin(), overflow.end());
        }
    }

    // Get healers
    if (needed.healersNeeded > 0)
    {
        auto healers = SelectBotsFromPool(BotRole::Healer, needed.healersNeeded, playerFaction, playerLevel, needed.minGearScore);
        allBots.insert(allBots.end(), healers.begin(), healers.end());

        if (healers.size() < needed.healersNeeded)
        {
            auto overflow = CreateOverflowBots(BotRole::Healer, needed.healersNeeded - static_cast<uint32>(healers.size()),
                playerFaction, playerLevel, needed.minGearScore);
            allBots.insert(allBots.end(), overflow.begin(), overflow.end());
        }
    }

    // Get DPS
    if (needed.dpsNeeded > 0)
    {
        auto dps = SelectBotsFromPool(BotRole::DPS, needed.dpsNeeded, playerFaction, playerLevel, needed.minGearScore);
        allBots.insert(allBots.end(), dps.begin(), dps.end());

        if (dps.size() < needed.dpsNeeded)
        {
            auto overflow = CreateOverflowBots(BotRole::DPS, needed.dpsNeeded - static_cast<uint32>(dps.size()),
                playerFaction, playerLevel, needed.minGearScore);
            allBots.insert(allBots.end(), overflow.begin(), overflow.end());
        }
    }

    // Check if we got enough bots
    if (allBots.size() < needed.totalNeeded)
    {
        if (!_config.allowPartialFill ||
            (allBots.size() * 100 / needed.totalNeeded) < _config.partialFillMinPct)
        {
            TC_LOG_WARN("playerbot.orchestrator", "InstanceBotOrchestrator::FulfillDungeonRequest - "
                "Could not get enough bots: {}/{}", allBots.size(), needed.totalNeeded);

            // Release the bots we got
            sInstanceBotPool->ReleaseBots(allBots);

            if (request.onFailed)
                request.onFailed("Not enough bots available");
            return false;
        }
    }

    // Track bots and mark them as instance bots
    {
        std::lock_guard<std::mutex> lock(_instanceMutex);
        for (auto const& guid : allBots)
        {
            _managedBots.insert(guid);
            // Mark as instance bot - enables 60s idle timeout
            sBotWorldSessionMgr->MarkAsInstanceBot(guid);
        }
    }

    if (_config.logAssignments)
    {
        TC_LOG_INFO("playerbot.orchestrator", "InstanceBotOrchestrator::FulfillDungeonRequest - "
            "Assigned {} bots for dungeon {}", allBots.size(), request.dungeonId);
    }

    // Invoke callback
    if (request.onBotsReady)
        request.onBotsReady(allBots);

    return true;
}

bool InstanceBotOrchestrator::FulfillRaidRequest(RaidRequest& request)
{
    TC_LOG_DEBUG("playerbot.orchestrator", "InstanceBotOrchestrator::FulfillRaidRequest - Processing request {}",
        request.requestId);

    ContentRequirement const* req = sContentRequirementDb->GetRaidRequirement(request.raidId);
    if (!req)
    {
        if (request.onFailed)
            request.onFailed("Unknown raid");
        return false;
    }

    // Calculate current group state
    GroupState groupState;
    groupState.totalPlayers = static_cast<uint32>(request.currentGroupMembers.size());

    for (auto const& [guid, role] : request.memberRoles)
    {
        switch (role)
        {
            case 0: ++groupState.tanks; break;
            case 1: ++groupState.healers; break;
            case 2: ++groupState.dps; break;
        }
    }

    BotsNeeded needed = sContentRequirementDb->CalculateBotsNeeded(req, groupState);

    if (!needed.NeedsBots())
    {
        if (request.onBotsReady)
            request.onBotsReady({});
        return true;
    }

    // For raids, use JIT factory for large requests
    FactoryRequest factoryReq;
    factoryReq.instanceType = InstanceType::Raid;
    factoryReq.contentId = request.raidId;
    factoryReq.playerLevel = request.playerLevel;
    factoryReq.playerFaction = request.playerFaction;
    factoryReq.tanksNeeded = needed.tanksNeeded;
    factoryReq.healersNeeded = needed.healersNeeded;
    factoryReq.dpsNeeded = needed.dpsNeeded;
    factoryReq.minGearScore = needed.minGearScore;
    factoryReq.onComplete = [this, request](std::vector<ObjectGuid> const& bots) mutable {
        // Track bots and mark them as instance bots
        {
            std::lock_guard<std::mutex> lock(_instanceMutex);
            for (auto const& guid : bots)
            {
                _managedBots.insert(guid);
                // Mark as instance bot - enables 60s idle timeout
                sBotWorldSessionMgr->MarkAsInstanceBot(guid);
            }
        }

        if (request.onBotsReady)
            request.onBotsReady(bots);
    };
    factoryReq.onFailed = [request](std::string const& error) {
        if (request.onFailed)
            request.onFailed(error);
    };

    sJITBotFactory->SubmitRequest(std::move(factoryReq));

    return true;
}

bool InstanceBotOrchestrator::FulfillBattlegroundRequest(BattlegroundRequest& request)
{
    TC_LOG_DEBUG("playerbot.orchestrator", "InstanceBotOrchestrator::FulfillBattlegroundRequest - Processing request {}",
        request.requestId);

    ContentRequirement const* req = sContentRequirementDb->GetBattlegroundRequirement(request.bgTypeId);
    if (!req)
    {
        if (request.onFailed)
            request.onFailed("Unknown battleground");
        return false;
    }

    GroupState groupState;
    groupState.alliancePlayers = request.currentAlliancePlayers;
    groupState.hordePlayers = request.currentHordePlayers;
    groupState.leaderFaction = request.playerFaction;

    BotsNeeded needed = sContentRequirementDb->CalculateBotsNeeded(req, groupState);

    TC_LOG_INFO("playerbot.orchestrator", "InstanceBotOrchestrator::FulfillBattlegroundRequest - "
        "BG {}: Need {} Alliance, {} Horde",
        request.bgTypeId, needed.allianceNeeded, needed.hordeNeeded);

    // For large BGs (40v40), MUST use JIT factory
    FactoryRequest factoryReq;
    factoryReq.instanceType = InstanceType::Battleground;
    factoryReq.contentId = request.bgTypeId;
    factoryReq.playerLevel = request.playerLevel;
    factoryReq.playerFaction = request.playerFaction;
    factoryReq.allianceNeeded = needed.allianceNeeded;
    factoryReq.hordeNeeded = needed.hordeNeeded;
    factoryReq.minGearScore = needed.minGearScore;

    // Note: The onComplete callback for PvP needs both faction lists
    // JITBotFactory will need to be extended to support this properly
    // For now, we'll collect both in one vector and split by faction later

    factoryReq.onComplete = [this, request, needed](std::vector<ObjectGuid> const& bots) mutable {
        // Split bots by faction
        // In production, JIT factory would track this
        std::vector<ObjectGuid> allianceBots;
        std::vector<ObjectGuid> hordeBots;

        // For now, assume first N are Alliance, rest are Horde
        for (size_t i = 0; i < bots.size(); ++i)
        {
            if (i < needed.allianceNeeded)
                allianceBots.push_back(bots[i]);
            else
                hordeBots.push_back(bots[i]);
        }

        // Track all bots and mark them as instance bots
        {
            std::lock_guard<std::mutex> lock(_instanceMutex);
            for (auto const& guid : bots)
            {
                _managedBots.insert(guid);
                // Mark as instance bot - enables 60s idle timeout
                sBotWorldSessionMgr->MarkAsInstanceBot(guid);
            }
        }

        if (request.onBotsReady)
            request.onBotsReady(allianceBots, hordeBots);
    };
    factoryReq.onFailed = [request](std::string const& error) {
        if (request.onFailed)
            request.onFailed(error);
    };

    sJITBotFactory->SubmitRequest(std::move(factoryReq));

    return true;
}

bool InstanceBotOrchestrator::FulfillArenaRequest(ArenaRequest& request)
{
    TC_LOG_DEBUG("playerbot.orchestrator", "InstanceBotOrchestrator::FulfillArenaRequest - Processing request {}",
        request.requestId);

    ContentRequirement const* req = sContentRequirementDb->GetArenaRequirement(request.arenaType);
    if (!req)
    {
        if (request.onFailed)
            request.onFailed("Unknown arena type");
        return false;
    }

    GroupState groupState;
    groupState.totalPlayers = 1 + static_cast<uint32>(request.existingTeammates.size());
    groupState.leaderFaction = request.playerFaction;

    BotsNeeded needed = sContentRequirementDb->CalculateArenaBots(request.arenaType, groupState, request.needOpponents);

    std::vector<ObjectGuid> teammates;
    std::vector<ObjectGuid> opponents;

    // Use player's actual level for bot selection
    uint32 playerLevel = request.playerLevel;

    // Get teammates (same faction)
    uint32 teammatesNeeded = request.arenaType - 1 - static_cast<uint32>(request.existingTeammates.size());
    if (teammatesNeeded > 0)
    {
        // Prefer 1 healer, rest DPS for arena
        if (teammatesNeeded >= 2)
        {
            auto healer = SelectBotsFromPool(BotRole::Healer, 1, request.playerFaction, playerLevel, needed.minGearScore);
            teammates.insert(teammates.end(), healer.begin(), healer.end());
            --teammatesNeeded;
        }

        if (teammatesNeeded > 0)
        {
            auto dps = SelectBotsFromPool(BotRole::DPS, teammatesNeeded, request.playerFaction, playerLevel, needed.minGearScore);
            teammates.insert(teammates.end(), dps.begin(), dps.end());
        }
    }

    // Get opponents (opposite faction)
    if (request.needOpponents)
    {
        Faction opponentFaction = (request.playerFaction == Faction::Alliance) ? Faction::Horde : Faction::Alliance;

        // Mirror team composition
        if (request.arenaType >= 3)
        {
            auto healer = SelectBotsFromPool(BotRole::Healer, 1, opponentFaction, playerLevel, needed.minGearScore);
            opponents.insert(opponents.end(), healer.begin(), healer.end());
        }

        uint32 opponentDpsNeeded = request.arenaType - static_cast<uint32>(opponents.size());
        auto dps = SelectBotsFromPool(BotRole::DPS, opponentDpsNeeded, opponentFaction, playerLevel, needed.minGearScore);
        opponents.insert(opponents.end(), dps.begin(), dps.end());
    }

    // Track bots and mark them as instance bots
    {
        std::lock_guard<std::mutex> lock(_instanceMutex);
        for (auto const& guid : teammates)
        {
            _managedBots.insert(guid);
            // Mark as instance bot - enables 60s idle timeout
            sBotWorldSessionMgr->MarkAsInstanceBot(guid);
        }
        for (auto const& guid : opponents)
        {
            _managedBots.insert(guid);
            // Mark as instance bot - enables 60s idle timeout
            sBotWorldSessionMgr->MarkAsInstanceBot(guid);
        }
    }

    if (request.onBotsReady)
        request.onBotsReady(teammates, opponents);

    return true;
}

// ============================================================================
// INTERNAL METHODS - Bot Selection
// ============================================================================

std::vector<ObjectGuid> InstanceBotOrchestrator::SelectBotsFromPool(
    BotRole role,
    uint32 count,
    Faction faction,
    uint32 level,
    uint32 minGearScore)
{
    if (count == 0)
        return {};

    // Use pool assignment
    switch (role)
    {
        case BotRole::Tank:
            return sInstanceBotPool->AssignForDungeon(0, level, faction, count, 0, 0);
        case BotRole::Healer:
            return sInstanceBotPool->AssignForDungeon(0, level, faction, 0, count, 0);
        case BotRole::DPS:
            return sInstanceBotPool->AssignForDungeon(0, level, faction, 0, 0, count);
        default:
            return {};
    }
}

std::vector<ObjectGuid> InstanceBotOrchestrator::CreateOverflowBots(
    BotRole role,
    uint32 count,
    Faction faction,
    uint32 level,
    uint32 minGearScore)
{
    if (count == 0)
        return {};

    TC_LOG_INFO("playerbot.orchestrator", "InstanceBotOrchestrator::CreateOverflowBots - Creating {} {} bots at level {} via JIT",
        count, BotRoleToString(role), level);

    // Synchronous clone for immediate need
    BatchCloneRequest cloneReq;
    cloneReq.role = role;
    cloneReq.count = count;
    cloneReq.targetLevel = level;
    cloneReq.faction = faction;
    cloneReq.minGearScore = minGearScore;

    auto results = sBotCloneEngine->BatchClone(cloneReq);

    std::vector<ObjectGuid> bots;
    uint32 loginSuccessCount = 0;

    for (auto const& result : results)
    {
        if (!result.success)
        {
            TC_LOG_WARN("playerbot.orchestrator", "CreateOverflowBots - Bot creation failed: {}",
                result.errorMessage);
            continue;
        }

        bots.push_back(result.botGuid);

        // CRITICAL FIX: Login the bot so it's available in world for callbacks!
        // Without this, ObjectAccessor::FindPlayer() will return null
        // and bots won't be added to LFG/BG queues.
        if (sBotWorldSessionMgr->AddPlayerBot(result.botGuid, result.accountId, true))
        {
            ++loginSuccessCount;
            TC_LOG_DEBUG("playerbot.orchestrator", "CreateOverflowBots - Logged in bot {} (account {})",
                result.botGuid.ToString(), result.accountId);
        }
        else
        {
            TC_LOG_WARN("playerbot.orchestrator", "CreateOverflowBots - Failed to login bot {} (account {})",
                result.botGuid.ToString(), result.accountId);
        }
    }

    TC_LOG_INFO("playerbot.orchestrator", "InstanceBotOrchestrator::CreateOverflowBots - Created {} bots, logged in {}/{}",
        bots.size(), loginSuccessCount, bots.size());

    return bots;
}

bool InstanceBotOrchestrator::ShouldUseOverflow(BotRole role, Faction faction, uint32 count) const
{
    if (!_config.preferPoolBots)
        return true;

    uint32 available = sInstanceBotPool->GetAvailableCount(role, faction);
    uint32 total = sInstanceBotPool->GetTotalPoolSize();

    if (total == 0)
        return true;

    float usedPct = 100.0f * (1.0f - (static_cast<float>(available) / static_cast<float>(total)));
    return usedPct >= static_cast<float>(_config.useOverflowThresholdPct);
}

// ============================================================================
// INTERNAL METHODS - Instance Management
// ============================================================================

void InstanceBotOrchestrator::TrackBotsInInstance(
    uint32 instanceId,
    std::vector<ObjectGuid> const& bots)
{
    std::lock_guard<std::mutex> lock(_instanceMutex);

    auto it = _activeInstances.find(instanceId);
    if (it != _activeInstances.end())
    {
        it->second.assignedBots.insert(
            it->second.assignedBots.end(),
            bots.begin(),
            bots.end());
    }

    for (auto const& guid : bots)
    {
        _managedBots.insert(guid);
        // Mark as instance bot - enables 60s idle timeout
        sBotWorldSessionMgr->MarkAsInstanceBot(guid);
    }
}

void InstanceBotOrchestrator::ReleaseBotsFromInstance(uint32 instanceId)
{
    std::vector<ObjectGuid> botsToRelease;

    {
        std::lock_guard<std::mutex> lock(_instanceMutex);

        auto it = _activeInstances.find(instanceId);
        if (it != _activeInstances.end())
        {
            botsToRelease = it->second.assignedBots;

            for (auto const& guid : botsToRelease)
                _managedBots.erase(guid);
        }
    }

    if (!botsToRelease.empty())
    {
        TC_LOG_DEBUG("playerbot.orchestrator", "InstanceBotOrchestrator::ReleaseBotsFromInstance - "
            "Releasing {} bots from instance {}", botsToRelease.size(), instanceId);

        sInstanceBotPool->ReleaseBots(botsToRelease);

        // Also recycle for JIT factory
        sJITBotFactory->RecycleBots(botsToRelease);
    }
}

void InstanceBotOrchestrator::ProcessTimeouts()
{
    auto now = std::chrono::system_clock::now();

    // Check dungeon queue
    {
        std::lock_guard<std::mutex> lock(_queueMutex);

        // Create temporary queue to filter timeouts
        std::queue<DungeonRequest> tempQueue;
        while (!_dungeonQueue.empty())
        {
            auto& request = _dungeonQueue.front();
            if ((now - request.createdAt) >= request.timeout)
            {
                TC_LOG_WARN("playerbot.orchestrator", "InstanceBotOrchestrator::ProcessTimeouts - "
                    "Dungeon request {} timed out", request.requestId);
                if (request.onFailed)
                    request.onFailed("Request timed out");
                _requestsFailed.fetch_add(1);
            }
            else
            {
                tempQueue.push(std::move(request));
            }
            _dungeonQueue.pop();
        }
        _dungeonQueue = std::move(tempQueue);
    }

    // Similar timeout processing for other queues would go here
}

} // namespace Playerbot
