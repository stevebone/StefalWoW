/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BotLifecycleManager.h"
#include "Log.h"
#include "Player.h"
#include "GameTime.h"
#include "Item.h"
#include "Bag.h"
#include "../Session/BotSession.h"
#include "../AI/BotAI.h"

namespace Playerbot
{

// ============================================================================
// BotLifecycle Implementation
// ============================================================================

BotLifecycle::BotLifecycle(ObjectGuid botGuid, std::shared_ptr<BotSession> session)
    : _botGuid(botGuid)
    , _session(session)
    , _state(BotLifecycleState::CREATED)
    , _stateChangeTime(std::chrono::steady_clock::now())
    , _idleTimer(0)
    , _nextIdleAction(0)
{
    TC_LOG_DEBUG("playerbot.lifecycle", "BotLifecycle::BotLifecycle - Created lifecycle for bot {}",
        botGuid.ToString());

    // Initialize metrics
    _metrics.loginTime = std::chrono::steady_clock::now();
    _metrics.lastActivityTime = std::chrono::steady_clock::now();
}

BotLifecycle::~BotLifecycle()
{
    TC_LOG_DEBUG("playerbot.lifecycle", "BotLifecycle::~BotLifecycle - Destroying lifecycle for bot {}",
        _botGuid.ToString());
}

bool BotLifecycle::Start()
{
    TC_LOG_DEBUG("playerbot.lifecycle", "BotLifecycle::Start - Starting lifecycle for bot {}",
        _botGuid.ToString());

    if (_state != BotLifecycleState::CREATED && _state != BotLifecycleState::OFFLINE)
    {
        TC_LOG_WARN("playerbot.lifecycle", "BotLifecycle::Start - Bot {} already started (state: {})",
            _botGuid.ToString(), static_cast<uint8>(_state.load()));
        return false;
    }

    if (!TransitionToState(BotLifecycleState::LOGGING_IN))
        return false;

    // The actual login process will be handled by the session
    // Once logged in, OnLogin() will be called which transitions to ACTIVE
    return true;
}

void BotLifecycle::Stop(bool immediate)
{
    TC_LOG_DEBUG("playerbot.lifecycle", "BotLifecycle::Stop - Stopping lifecycle for bot {} (immediate: {})",
        _botGuid.ToString(), immediate);

    if (_state == BotLifecycleState::TERMINATED)
        return;

    if (immediate)
    {
        TransitionToState(BotLifecycleState::TERMINATED);
        _pendingCleanup = true;
        return;
    }

    // Start graceful logout
    if (_state != BotLifecycleState::LOGGING_OUT)
    {
        TransitionToState(BotLifecycleState::LOGGING_OUT);
    }
}

void BotLifecycle::Update(uint32 diff)
{
    if (_pendingCleanup)
        return;

    auto startTime = std::chrono::high_resolution_clock::now();

    BotLifecycleState currentState = _state.load();

    switch (currentState)
    {
        case BotLifecycleState::CREATED:
            // Waiting for Start() to be called
            break;

        case BotLifecycleState::LOGGING_IN:
            // Handled by session
            break;

        case BotLifecycleState::ACTIVE:
            HandleActiveState(diff);
            break;

        case BotLifecycleState::IDLE:
            HandleIdleState(diff);
            break;

        case BotLifecycleState::COMBAT:
            HandleCombatState(diff);
            break;

        case BotLifecycleState::QUESTING:
            HandleQuestingState(diff);
            break;

        case BotLifecycleState::FOLLOWING:
            // Following state handled by movement AI
            break;

        case BotLifecycleState::RESTING:
            HandleRestingState(diff);
            break;

        case BotLifecycleState::LOGGING_OUT:
            // Handled by session
            TransitionToState(BotLifecycleState::OFFLINE);
            break;

        case BotLifecycleState::OFFLINE:
            // Bot is offline, no updates needed
            break;

        case BotLifecycleState::TERMINATED:
            // Cleanup pending
            break;
    }

    // Update metrics
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    UpdateMetrics(static_cast<uint32>(duration));
}

void BotLifecycle::Pause()
{
    TC_LOG_DEBUG("playerbot.lifecycle", "BotLifecycle::Pause - Pausing bot {}", _botGuid.ToString());

    if (_state == BotLifecycleState::ACTIVE ||
        _state == BotLifecycleState::COMBAT ||
        _state == BotLifecycleState::QUESTING)
    {
        TransitionToState(BotLifecycleState::IDLE);
    }
}

void BotLifecycle::Resume()
{
    TC_LOG_DEBUG("playerbot.lifecycle", "BotLifecycle::Resume - Resuming bot {}", _botGuid.ToString());

    if (_state == BotLifecycleState::IDLE)
    {
        TransitionToState(BotLifecycleState::ACTIVE);
    }
}

bool BotLifecycle::IsOnline() const
{
    BotLifecycleState state = _state.load();
    return state == BotLifecycleState::ACTIVE ||
           state == BotLifecycleState::IDLE ||
           state == BotLifecycleState::COMBAT ||
           state == BotLifecycleState::QUESTING ||
           state == BotLifecycleState::FOLLOWING ||
           state == BotLifecycleState::RESTING;
}

bool BotLifecycle::TransitionToState(BotLifecycleState newState)
{
    BotLifecycleState oldState = _state.load();

    // Validate state transition
    bool validTransition = false;

    switch (oldState)
    {
        case BotLifecycleState::CREATED:
            validTransition = (newState == BotLifecycleState::LOGGING_IN ||
                              newState == BotLifecycleState::TERMINATED);
            break;

        case BotLifecycleState::LOGGING_IN:
            validTransition = (newState == BotLifecycleState::ACTIVE ||
                              newState == BotLifecycleState::TERMINATED);
            break;

        case BotLifecycleState::ACTIVE:
            validTransition = (newState == BotLifecycleState::IDLE ||
                              newState == BotLifecycleState::COMBAT ||
                              newState == BotLifecycleState::QUESTING ||
                              newState == BotLifecycleState::FOLLOWING ||
                              newState == BotLifecycleState::RESTING ||
                              newState == BotLifecycleState::LOGGING_OUT ||
                              newState == BotLifecycleState::TERMINATED);
            break;

        case BotLifecycleState::IDLE:
            validTransition = (newState == BotLifecycleState::ACTIVE ||
                              newState == BotLifecycleState::COMBAT ||
                              newState == BotLifecycleState::QUESTING ||
                              newState == BotLifecycleState::FOLLOWING ||
                              newState == BotLifecycleState::LOGGING_OUT ||
                              newState == BotLifecycleState::TERMINATED);
            break;

        case BotLifecycleState::COMBAT:
            validTransition = (newState == BotLifecycleState::ACTIVE ||
                              newState == BotLifecycleState::IDLE ||
                              newState == BotLifecycleState::RESTING ||
                              newState == BotLifecycleState::LOGGING_OUT ||
                              newState == BotLifecycleState::TERMINATED);
            break;

        case BotLifecycleState::QUESTING:
            validTransition = (newState == BotLifecycleState::ACTIVE ||
                              newState == BotLifecycleState::IDLE ||
                              newState == BotLifecycleState::COMBAT ||
                              newState == BotLifecycleState::LOGGING_OUT ||
                              newState == BotLifecycleState::TERMINATED);
            break;

        case BotLifecycleState::FOLLOWING:
            validTransition = (newState == BotLifecycleState::ACTIVE ||
                              newState == BotLifecycleState::IDLE ||
                              newState == BotLifecycleState::COMBAT ||
                              newState == BotLifecycleState::LOGGING_OUT ||
                              newState == BotLifecycleState::TERMINATED);
            break;

        case BotLifecycleState::RESTING:
            validTransition = (newState == BotLifecycleState::ACTIVE ||
                              newState == BotLifecycleState::IDLE ||
                              newState == BotLifecycleState::COMBAT ||
                              newState == BotLifecycleState::LOGGING_OUT ||
                              newState == BotLifecycleState::TERMINATED);
            break;

        case BotLifecycleState::LOGGING_OUT:
            validTransition = (newState == BotLifecycleState::OFFLINE ||
                              newState == BotLifecycleState::TERMINATED);
            break;

        case BotLifecycleState::OFFLINE:
            validTransition = (newState == BotLifecycleState::LOGGING_IN ||
                              newState == BotLifecycleState::TERMINATED);
            break;

        case BotLifecycleState::TERMINATED:
            // No transitions allowed from terminated state
            validTransition = false;
            break;
    }

    if (!validTransition)
    {
        TC_LOG_WARN("playerbot.lifecycle", "BotLifecycle::TransitionToState - Invalid state transition "
            "for bot {} from {} to {}", _botGuid.ToString(),
            static_cast<uint8>(oldState), static_cast<uint8>(newState));
        return false;
    }

    _state = newState;
    _stateChangeTime = std::chrono::steady_clock::now();

    TC_LOG_DEBUG("playerbot.lifecycle", "BotLifecycle::TransitionToState - Bot {} transitioned from {} to {}",
        _botGuid.ToString(), static_cast<uint8>(oldState), static_cast<uint8>(newState));

    return true;
}

void BotLifecycle::SetActivity(BotActivity const& activity)
{
    _currentActivity = activity;
    _currentActivity.startTime = std::chrono::steady_clock::now();
    _metrics.lastActivityTime = _currentActivity.startTime;

    TC_LOG_DEBUG("playerbot.lifecycle", "BotLifecycle::SetActivity - Bot {} activity set to type {}",
        _botGuid.ToString(), static_cast<uint8>(activity.type));
}

BotActivity BotLifecycle::GetCurrentActivity() const
{
    return _currentActivity;
}

void BotLifecycle::UpdateMetrics(uint32 aiUpdateTime)
{
    _metrics.aiUpdateTime += aiUpdateTime;
    _metrics.aiUpdateCount++;

    if (_metrics.aiUpdateCount > 0)
    {
        _metrics.avgAiUpdateTime = static_cast<float>(_metrics.aiUpdateTime) /
            static_cast<float>(_metrics.aiUpdateCount) / 1000.0f; // Convert to ms
    }
}

void BotLifecycle::OnLogin()
{
    TC_LOG_DEBUG("playerbot.lifecycle", "BotLifecycle::OnLogin - Bot {} logged in", _botGuid.ToString());

    _metrics.loginTime = std::chrono::steady_clock::now();
    _metrics.lastActivityTime = _metrics.loginTime;

    TransitionToState(BotLifecycleState::ACTIVE);
}

void BotLifecycle::OnLogout()
{
    TC_LOG_DEBUG("playerbot.lifecycle", "BotLifecycle::OnLogout - Bot {} logged out", _botGuid.ToString());

    // Calculate total active time
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - _metrics.loginTime);
    _metrics.totalActiveTime += duration.count();

    TransitionToState(BotLifecycleState::OFFLINE);
}

void BotLifecycle::OnEnterCombat()
{
    TC_LOG_DEBUG("playerbot.lifecycle", "BotLifecycle::OnEnterCombat - Bot {} entered combat",
        _botGuid.ToString());

    BotActivity combatActivity;
    combatActivity.type = BotActivity::COMBAT;
    combatActivity.description = "In combat";
    SetActivity(combatActivity);

    TransitionToState(BotLifecycleState::COMBAT);
}

void BotLifecycle::OnLeaveCombat()
{
    TC_LOG_DEBUG("playerbot.lifecycle", "BotLifecycle::OnLeaveCombat - Bot {} left combat",
        _botGuid.ToString());

    _currentActivity.type = BotActivity::NONE;
    _currentActivity.description = "";

    // Check if we need rest after combat
    if (NeedsRest())
        TransitionToState(BotLifecycleState::RESTING);
    else
        TransitionToState(BotLifecycleState::ACTIVE);
}

void BotLifecycle::OnDeath()
{
    TC_LOG_DEBUG("playerbot.lifecycle", "BotLifecycle::OnDeath - Bot {} died", _botGuid.ToString());

    _currentActivity.type = BotActivity::NONE;
    _metrics.failedEncounters++;
}

void BotLifecycle::OnRespawn()
{
    TC_LOG_DEBUG("playerbot.lifecycle", "BotLifecycle::OnRespawn - Bot {} respawned", _botGuid.ToString());

    TransitionToState(BotLifecycleState::ACTIVE);
}

void BotLifecycle::OnQuestComplete(uint32 questId)
{
    TC_LOG_DEBUG("playerbot.lifecycle", "BotLifecycle::OnQuestComplete - Bot {} completed quest {}",
        _botGuid.ToString(), questId);

    _metrics.questsCompleted++;
}

void BotLifecycle::OnLevelUp(uint32 newLevel)
{
    TC_LOG_DEBUG("playerbot.lifecycle", "BotLifecycle::OnLevelUp - Bot {} leveled up to {}",
        _botGuid.ToString(), newLevel);
}

Player* BotLifecycle::GetPlayer() const
{
    if (!_session)
        return nullptr;

    return _session->GetPlayer();
}

BotAI* BotLifecycle::GetAI() const
{
    // The BotSession directly stores a reference to the BotAI
    // No need to go through Player - BotSession::SetAI/GetAI handles this
    if (!_session)
        return nullptr;

    return _session->GetAI();
}

void BotLifecycle::HandleActiveState(uint32 diff)
{
    _idleTimer += diff;

    // Check for idle timeout
    if (_idleTimer > IDLE_ACTION_INTERVAL)
    {
        _idleTimer = 0;

        // Determine if we should enter idle state
        if (_currentActivity.type == BotActivity::NONE)
        {
            TransitionToState(BotLifecycleState::IDLE);
        }
    }

    _metrics.lastActivityTime = std::chrono::steady_clock::now();
}

void BotLifecycle::HandleIdleState(uint32 diff)
{
    _idleTimer += diff;
    _metrics.totalIdleTime += diff / 1000; // Convert to seconds

    // Periodically perform idle actions
    if (_idleTimer > _nextIdleAction)
    {
        _idleTimer = 0;
        _nextIdleAction = IDLE_ACTION_INTERVAL + (rand() % 10000); // 30-40 seconds

        SelectIdleBehavior();
    }
}

void BotLifecycle::HandleCombatState(uint32 diff)
{
    // Combat is primarily handled by BotAI
    // Here we just track combat duration
    _metrics.lastActivityTime = std::chrono::steady_clock::now();
}

void BotLifecycle::HandleQuestingState(uint32 diff)
{
    // Questing behavior is handled by quest AI systems
    _metrics.lastActivityTime = std::chrono::steady_clock::now();
}

void BotLifecycle::HandleRestingState(uint32 diff)
{
    Player* player = GetPlayer();
    if (!player)
    {
        TransitionToState(BotLifecycleState::IDLE);
        return;
    }

    // Check if resting is complete
    if (!NeedsRest())
    {
        TransitionToState(BotLifecycleState::ACTIVE);
    }
}

void BotLifecycle::SelectIdleBehavior()
{
    // Select random idle behavior
    int behaviorType = rand() % 5;

    switch (behaviorType)
    {
        case 0:
            // Check for repairs
            if (NeedsRepair())
            {
                BotActivity activity;
                activity.type = BotActivity::TRAVEL;
                activity.description = "Traveling to repair";
                SetActivity(activity);
            }
            break;

        case 1:
            // Check for vendor needs
            if (NeedsVendor())
            {
                BotActivity activity;
                activity.type = BotActivity::TRADING;
                activity.description = "Selling items";
                SetActivity(activity);
            }
            break;

        case 2:
            // Look for nearby quests
            {
                BotActivity activity;
                activity.type = BotActivity::QUEST;
                activity.description = "Looking for quests";
                SetActivity(activity);
                TransitionToState(BotLifecycleState::QUESTING);
            }
            break;

        case 3:
            // Social behavior - emotes, chat
            {
                BotActivity activity;
                activity.type = BotActivity::SOCIAL;
                activity.description = "Socializing";
                SetActivity(activity);
            }
            break;

        case 4:
        default:
            // Just stand around or wander slightly
            PerformIdleAction();
            break;
    }
}

void BotLifecycle::PerformIdleAction()
{
    Player* player = GetPlayer();
    if (!player)
        return;

    // Perform random idle emotes or minor movements
    int action = rand() % 10;

    if (action < 3)
    {
        // Do a random emote
        // This would trigger emote through BotAI when implemented
    }
    // Otherwise just stand idle
}

bool BotLifecycle::NeedsRest() const
{
    Player* player = GetPlayer();
    if (!player)
        return false;

    // Need rest if health or mana below 50%
    float healthPct = player->GetHealthPct();
    float manaPct = player->GetPowerPct(POWER_MANA);

    return healthPct < 50.0f || (player->GetPowerType() == POWER_MANA && manaPct < 30.0f);
}

bool BotLifecycle::NeedsRepair() const
{
    Player* player = GetPlayer();
    if (!player)
        return false;

    // Check equipment durability - any broken item means we need repair
    for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        if (item && item->IsBroken())
            return true;
    }

    // Also check if repair cost would be significant (indicating low durability)
    // This leverages the existing TrinityCore API
    uint64 totalRepairCost = 0;
    for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        if (item)
            totalRepairCost += item->CalculateDurabilityRepairCost(1.0f);
    }

    // If repair cost is significant, suggest repair
    return totalRepairCost > 0;
}

bool BotLifecycle::NeedsVendor() const
{
    Player* player = GetPlayer();
    if (!player)
        return false;

    // Check if bags are mostly full
    uint32 freeSlots = 0;
    uint32 totalSlots = 0;

    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
    {
        if (Bag* pBag = player->GetBagByPos(bag))
        {
            totalSlots += pBag->GetBagSize();
            freeSlots += pBag->GetFreeSlots();
        }
    }

    // Also check backpack
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
    {
        totalSlots++;
        if (!player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            freeSlots++;
    }

    // Need vendor if less than 20% free slots
    if (totalSlots > 0)
    {
        float freeRatio = static_cast<float>(freeSlots) / static_cast<float>(totalSlots);
        return freeRatio < 0.2f;
    }

    return false;
}

// ============================================================================
// BotLifecycleManager Implementation
// ============================================================================

BotLifecycleManager::BotLifecycleManager(Player* bot)
    : _bot(bot)
    , _lastStatsUpdate(std::chrono::steady_clock::now())
{
    if (!_bot)
        TC_LOG_ERROR("playerbot.lifecycle", "BotLifecycleManager: null bot!");
    else
        TC_LOG_DEBUG("playerbot.lifecycle", "BotLifecycleManager: Created for bot {}",
            _bot->GetGUID().ToString());
}

BotLifecycleManager::~BotLifecycleManager()
{
    // Stop all bots on destruction
    StopAll(true);
}

std::shared_ptr<BotLifecycle> BotLifecycleManager::CreateBotLifecycle(ObjectGuid botGuid,
    std::shared_ptr<BotSession> session)
{
    TC_LOG_DEBUG("playerbot.lifecycle", "BotLifecycleManager::CreateBotLifecycle - Creating lifecycle for {}",
        botGuid.ToString());

    // Check if lifecycle already exists
    auto itr = _botLifecycles.find(botGuid);
    if (itr != _botLifecycles.end())
    {
        TC_LOG_WARN("playerbot.lifecycle", "BotLifecycleManager::CreateBotLifecycle - Lifecycle already "
            "exists for bot {}", botGuid.ToString());
        return itr->second;
    }

    // Create new lifecycle
    auto lifecycle = std::make_shared<BotLifecycle>(botGuid, session);
    _botLifecycles[botGuid] = lifecycle;

    // Broadcast creation event
    BroadcastStateChange(botGuid, BotLifecycleState::TERMINATED, BotLifecycleState::CREATED);

    return lifecycle;
}

void BotLifecycleManager::RemoveBotLifecycle(ObjectGuid botGuid)
{
    TC_LOG_DEBUG("playerbot.lifecycle", "BotLifecycleManager::RemoveBotLifecycle - Removing lifecycle for {}",
        botGuid.ToString());

    auto itr = _botLifecycles.find(botGuid);
    if (itr == _botLifecycles.end())
    {
        TC_LOG_WARN("playerbot.lifecycle", "BotLifecycleManager::RemoveBotLifecycle - No lifecycle found "
            "for bot {}", botGuid.ToString());
        return;
    }

    // Stop the lifecycle if it's running
    if (itr->second)
    {
        BotLifecycleState oldState = itr->second->GetState();
        itr->second->Stop(true);
        BroadcastStateChange(botGuid, oldState, BotLifecycleState::TERMINATED);
    }

    _botLifecycles.erase(itr);
}

std::shared_ptr<BotLifecycle> BotLifecycleManager::GetBotLifecycle(ObjectGuid botGuid) const
{
    auto itr = _botLifecycles.find(botGuid);
    if (itr != _botLifecycles.end())
        return itr->second;

    return nullptr;
}

std::vector<std::shared_ptr<BotLifecycle>> BotLifecycleManager::GetActiveLifecycles() const
{
    std::vector<std::shared_ptr<BotLifecycle>> activeLifecycles;
    activeLifecycles.reserve(_botLifecycles.size());

    for (auto const& pair : _botLifecycles)
    {
        if (pair.second && pair.second->IsOnline())
        {
            activeLifecycles.push_back(pair.second);
        }
    }

    return activeLifecycles;
}

void BotLifecycleManager::UpdateAll(uint32 diff)
{
    for (auto& pair : _botLifecycles)
    {
        if (pair.second)
        {
            pair.second->Update(diff);
        }
    }

    // Periodic stats update
    auto now = std::chrono::steady_clock::now();
    if (now - _lastStatsUpdate >= STATS_UPDATE_INTERVAL)
    {
        _lastStatsUpdate = now;
        // Stats are calculated on-demand in GetGlobalStats()
    }
}

void BotLifecycleManager::StopAll(bool immediate)
{
    TC_LOG_INFO("playerbot.lifecycle", "BotLifecycleManager::StopAll - Stopping all bots (immediate: {})",
        immediate);

    for (auto& pair : _botLifecycles)
    {
        if (pair.second)
        {
            BotLifecycleState oldState = pair.second->GetState();
            pair.second->Stop(immediate);
            BroadcastStateChange(pair.first, oldState, BotLifecycleState::TERMINATED);
        }
    }

    if (immediate)
    {
        _botLifecycles.clear();
    }
}

GlobalStats BotLifecycleManager::GetGlobalStats() const
{
    GlobalStats stats;

    for (auto const& pair : _botLifecycles)
    {
        if (!pair.second)
            continue;

        stats.totalBots++;

        BotLifecycleState state = pair.second->GetState();
        switch (state)
        {
            case BotLifecycleState::ACTIVE:
            case BotLifecycleState::FOLLOWING:
                stats.activeBots++;
                break;
            case BotLifecycleState::IDLE:
            case BotLifecycleState::RESTING:
                stats.idleBots++;
                break;
            case BotLifecycleState::COMBAT:
                stats.combatBots++;
                break;
            case BotLifecycleState::QUESTING:
                stats.questingBots++;
                break;
            case BotLifecycleState::OFFLINE:
            case BotLifecycleState::TERMINATED:
                stats.offlineBots++;
                break;
            default:
                break;
        }

        // Aggregate metrics
        BotPerformanceMetrics const& metrics = pair.second->GetMetrics();
        stats.avgAiUpdateTime += metrics.avgAiUpdateTime;
        stats.totalMemoryUsage += metrics.currentMemoryUsage;

        // Calculate actions per second based on activity
        if (pair.second->IsOnline())
        {
            stats.totalActionsPerSecond += metrics.actionsExecuted > 0 ?
                metrics.actionsExecuted / std::max(1u, static_cast<uint32>(metrics.totalActiveTime)) : 0;
        }
    }

    // Average the AI update time
    if (stats.totalBots > 0)
    {
        stats.avgAiUpdateTime /= static_cast<float>(stats.totalBots);
    }

    return stats;
}

void BotLifecycleManager::PrintPerformanceReport() const
{
    GlobalStats stats = GetGlobalStats();

    TC_LOG_INFO("playerbot.lifecycle", "=== Bot Lifecycle Performance Report ===");
    TC_LOG_INFO("playerbot.lifecycle", "Total Bots: {}", stats.totalBots);
    TC_LOG_INFO("playerbot.lifecycle", "  Active: {}", stats.activeBots);
    TC_LOG_INFO("playerbot.lifecycle", "  Idle: {}", stats.idleBots);
    TC_LOG_INFO("playerbot.lifecycle", "  Combat: {}", stats.combatBots);
    TC_LOG_INFO("playerbot.lifecycle", "  Questing: {}", stats.questingBots);
    TC_LOG_INFO("playerbot.lifecycle", "  Offline: {}", stats.offlineBots);
    TC_LOG_INFO("playerbot.lifecycle", "Average AI Update Time: {:.3f} ms", stats.avgAiUpdateTime);
    TC_LOG_INFO("playerbot.lifecycle", "Total Memory Usage: {} bytes", stats.totalMemoryUsage);
    TC_LOG_INFO("playerbot.lifecycle", "Actions Per Second: {}", stats.totalActionsPerSecond);
    TC_LOG_INFO("playerbot.lifecycle", "=========================================");
}

void BotLifecycleManager::RegisterEventHandler(LifecycleEventHandler handler)
{
    std::lock_guard<Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_SPAWNER>> lock(_eventMutex);
    _eventHandlers.push_back(std::move(handler));
}

void BotLifecycleManager::BroadcastStateChange(ObjectGuid botGuid, BotLifecycleState oldState,
    BotLifecycleState newState)
{
    std::lock_guard<Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_SPAWNER>> lock(_eventMutex);

    for (auto const& handler : _eventHandlers)
    {
        if (handler)
        {
            handler(botGuid, oldState, newState);
        }
    }
}

} // namespace Playerbot
