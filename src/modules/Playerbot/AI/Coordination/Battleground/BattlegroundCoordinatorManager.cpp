/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BattlegroundCoordinatorManager.h"
#include "BattlegroundCoordinator.h"
#include "Battleground.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "Log.h"
#include "../Core/PlayerBotHooks.h"

namespace Playerbot {

// ============================================================================
// SINGLETON
// ============================================================================

BattlegroundCoordinatorManager* BattlegroundCoordinatorManager::instance()
{
    static BattlegroundCoordinatorManager instance;
    return &instance;
}

BattlegroundCoordinatorManager::BattlegroundCoordinatorManager()
    : _initialized(false)
{
}

BattlegroundCoordinatorManager::~BattlegroundCoordinatorManager()
{
    Shutdown();
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void BattlegroundCoordinatorManager::Initialize()
{
    std::lock_guard lock(_mutex);

    if (_initialized)
        return;

    TC_LOG_INFO("playerbots.bg.coordinator", "BattlegroundCoordinatorManager: Initializing...");

    _coordinators.clear();
    _pendingCreations.clear();
    _initialized = true;

    TC_LOG_INFO("playerbots.bg.coordinator", "BattlegroundCoordinatorManager: Initialized");
}

void BattlegroundCoordinatorManager::Shutdown()
{
    std::lock_guard lock(_mutex);

    if (!_initialized)
        return;

    TC_LOG_INFO("playerbots.bg.coordinator", "BattlegroundCoordinatorManager: Shutting down...");

    // Shutdown all coordinators
    for (auto& [bgId, coordinator] : _coordinators)
    {
        if (coordinator)
            coordinator->Shutdown();
    }

    _coordinators.clear();
    _pendingCreations.clear();
    _initialized = false;

    TC_LOG_INFO("playerbots.bg.coordinator", "BattlegroundCoordinatorManager: Shut down");
}

void BattlegroundCoordinatorManager::Update(uint32 diff)
{
    if (!_initialized)
        return;

    // ========================================================================
    // PHASE 1: Process pending coordinator creations from worker threads.
    // This runs on the MAIN THREAD where Battleground access is safe.
    // ========================================================================
    ProcessPendingCreations();

    // ========================================================================
    // PHASE 2: Copy-and-release for coordinator updates.
    // Only hold _mutex for the map snapshot, NOT during coordinator->Update()
    // which can be expensive (spatial cache rebuild, strategy evaluation).
    // ========================================================================
    std::vector<BattlegroundCoordinator*> activeCoordinators;
    {
        std::lock_guard lock(_mutex);
        activeCoordinators.reserve(_coordinators.size());
        for (auto& [bgId, coordinator] : _coordinators)
        {
            if (coordinator)
                activeCoordinators.push_back(coordinator.get());
        }
    }
    // _mutex released — update coordinators without blocking worker threads
    for (auto* coordinator : activeCoordinators)
        coordinator->Update(diff);
}

// ============================================================================
// COORDINATOR MANAGEMENT
// ============================================================================

void BattlegroundCoordinatorManager::OnBattlegroundStart(Battleground* bg)
{
    if (!bg || !_initialized)
        return;

    // OnBattlegroundStart runs on the main thread — safe to create directly
    CreateCoordinatorForBG(bg);
}

void BattlegroundCoordinatorManager::OnBattlegroundEnd(Battleground* bg)
{
    if (!bg || !_initialized)
        return;

    std::lock_guard lock(_mutex);

    uint32 bgInstanceId = bg->GetInstanceID();

    // Remove from pending creations if queued
    _pendingCreations.erase(bgInstanceId);

    auto itr = _coordinators.find(bgInstanceId);
    if (itr == _coordinators.end())
        return;

    TC_LOG_INFO("playerbots.bg.coordinator",
        "BattlegroundCoordinatorManager: Removing coordinator for BG instance {}",
        bgInstanceId);

    if (itr->second)
        itr->second->Shutdown();

    _coordinators.erase(itr);
}

BattlegroundCoordinator* BattlegroundCoordinatorManager::GetCoordinator(uint32 bgInstanceId)
{
    std::lock_guard lock(_mutex);

    auto itr = _coordinators.find(bgInstanceId);
    if (itr != _coordinators.end())
        return itr->second.get();

    return nullptr;
}

BattlegroundCoordinator* BattlegroundCoordinatorManager::GetCoordinatorForPlayer(Player* player)
{
    if (!player)
        return nullptr;

    Battleground* bg = player->GetBattleground();
    if (!bg)
        return nullptr;

    return GetCoordinator(bg->GetInstanceID());
}

void BattlegroundCoordinatorManager::UpdateBot(Player* bot, uint32 diff)
{
    if (!bot || !_initialized)
        return;

    Battleground* bg = bot->GetBattleground();
    if (!bg || bg->GetStatus() != STATUS_IN_PROGRESS)
        return;

    uint32 bgInstanceId = bg->GetInstanceID();

    std::lock_guard lock(_mutex);

    auto itr = _coordinators.find(bgInstanceId);
    if (itr != _coordinators.end() && itr->second)
    {
        // Coordinator exists — ensure this bot is tracked (handles late-joiners)
        itr->second->AddBot(bot);
        return;
    }

    // ========================================================================
    // No coordinator yet. DON'T create it here — we're on a WORKER THREAD.
    //
    // Coordinator creation calls Initialize() which accesses Battleground
    // data (GetPlayers, GetMapId), loads BG scripts, runs map grid operations
    // (FindNearestGameObject with 500yd radius), and does pathfinding.
    // None of these are thread-safe from worker threads.
    //
    // Instead, queue a creation request for the main thread to process
    // in the next Update() call. The coordinator will be available within
    // one server tick (~50-100ms).
    // ========================================================================
    if (!_pendingCreations.count(bgInstanceId))
    {
        _pendingCreations[bgInstanceId] = bg;
        TC_LOG_DEBUG("playerbots.bg.coordinator",
            "BattlegroundCoordinatorManager: Queued coordinator creation for BG instance {} (requested by bot {})",
            bgInstanceId, bot->GetName());
    }
}

bool BattlegroundCoordinatorManager::HasCoordinator(uint32 bgInstanceId) const
{
    std::lock_guard lock(_mutex);
    return _coordinators.find(bgInstanceId) != _coordinators.end();
}

uint32 BattlegroundCoordinatorManager::GetActiveCoordinatorCount() const
{
    std::lock_guard lock(_mutex);
    return static_cast<uint32>(_coordinators.size());
}

// ============================================================================
// PRIVATE — MAIN THREAD ONLY
// ============================================================================

void BattlegroundCoordinatorManager::CreateCoordinatorForBG(Battleground* bg)
{
    if (!bg)
        return;

    uint32 bgInstanceId = bg->GetInstanceID();

    // Check if coordinator already exists
    {
        std::lock_guard lock(_mutex);
        if (_coordinators.find(bgInstanceId) != _coordinators.end())
        {
            TC_LOG_DEBUG("playerbots.bg.coordinator",
                "BattlegroundCoordinatorManager: Coordinator already exists for BG instance {}",
                bgInstanceId);
            return;
        }
    }

    // Collect all bots in the BG (safe — we're on the main thread)
    std::vector<Player*> bots;
    for (auto const& itr : bg->GetPlayers())
    {
        if (Player* player = ObjectAccessor::FindPlayer(itr.first))
        {
            if (PlayerBotHooks::IsPlayerBot(player))
                bots.push_back(player);
        }
    }

    if (bots.empty())
    {
        TC_LOG_DEBUG("playerbots.bg.coordinator",
            "BattlegroundCoordinatorManager: No bots in BG instance {}, not creating coordinator",
            bgInstanceId);
        return;
    }

    TC_LOG_INFO("playerbots.bg.coordinator",
        "BattlegroundCoordinatorManager: Creating coordinator for BG instance {} ({}) with {} bots",
        bgInstanceId, bg->GetName(), bots.size());

    // Create and initialize coordinator (safe — main thread, no lock held
    // during expensive operations like grid scans and script loading)
    auto coordinator = std::make_unique<BattlegroundCoordinator>(bg, bots);
    coordinator->Initialize();

    // Insert under lock
    {
        std::lock_guard lock(_mutex);
        if (_coordinators.find(bgInstanceId) == _coordinators.end())
        {
            _coordinators[bgInstanceId] = std::move(coordinator);
            TC_LOG_INFO("playerbots.bg.coordinator",
                "BattlegroundCoordinatorManager: Coordinator created for BG {} (instance {})",
                bg->GetName(), bgInstanceId);
        }
    }
}

void BattlegroundCoordinatorManager::ProcessPendingCreations()
{
    // Snapshot pending creations under lock, then process without lock.
    // This runs on the MAIN THREAD where Battleground access is safe.
    std::unordered_map<uint32, Battleground*> pending;
    {
        std::lock_guard lock(_mutex);
        if (_pendingCreations.empty())
            return;
        pending.swap(_pendingCreations);
    }

    for (auto const& [bgInstanceId, bg] : pending)
    {
        if (!bg)
            continue;

        // Validate BG is still in progress
        if (bg->GetStatus() != STATUS_IN_PROGRESS)
            continue;

        CreateCoordinatorForBG(bg);
    }
}

} // namespace Playerbot
