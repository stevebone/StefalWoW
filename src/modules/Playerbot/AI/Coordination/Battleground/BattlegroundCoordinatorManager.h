/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <mutex>

class Battleground;
class Player;

namespace Playerbot {

class BattlegroundCoordinator;

/**
 * @class BattlegroundCoordinatorManager
 * @brief Manages BattlegroundCoordinator instances for each active battleground
 *
 * This singleton manages the lifecycle of BattlegroundCoordinator instances,
 * creating them when a BG starts and destroying them when it ends.
 * It also routes update calls to the appropriate coordinator.
 */
class TC_GAME_API BattlegroundCoordinatorManager final
{
public:
    static BattlegroundCoordinatorManager* instance();

    // Delete copy/move
    BattlegroundCoordinatorManager(const BattlegroundCoordinatorManager&) = delete;
    BattlegroundCoordinatorManager(BattlegroundCoordinatorManager&&) = delete;
    BattlegroundCoordinatorManager& operator=(const BattlegroundCoordinatorManager&) = delete;
    BattlegroundCoordinatorManager& operator=(BattlegroundCoordinatorManager&&) = delete;

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    void Initialize();
    void Shutdown();
    void Update(uint32 diff);

    // ========================================================================
    // COORDINATOR MANAGEMENT
    // ========================================================================

    /**
     * @brief Called when a battleground starts - creates a coordinator
     * @param bg The battleground instance
     */
    void OnBattlegroundStart(Battleground* bg);

    /**
     * @brief Called when a battleground ends - destroys the coordinator
     * @param bg The battleground instance
     */
    void OnBattlegroundEnd(Battleground* bg);

    /**
     * @brief Get the coordinator for a battleground instance
     * @param bgInstanceId The BG instance ID
     * @return Coordinator or nullptr if not found
     */
    BattlegroundCoordinator* GetCoordinator(uint32 bgInstanceId);

    /**
     * @brief Get the coordinator for a player's current BG
     * @param player The player
     * @return Coordinator or nullptr if not found/not in BG
     */
    BattlegroundCoordinator* GetCoordinatorForPlayer(Player* player);

    /**
     * @brief Update a specific bot in their BG coordinator
     * @param bot The bot player
     * @param diff Time since last update
     */
    void UpdateBot(Player* bot, uint32 diff);

    /**
     * @brief Check if coordinator exists for a BG
     * @param bgInstanceId The BG instance ID
     * @return true if coordinator exists
     */
    bool HasCoordinator(uint32 bgInstanceId) const;

    /**
     * @brief Get read-only access to all coordinators for iteration
     * @return Const reference to the coordinators map
     *
     * NOTE: Caller should not hold references across calls as coordinators may be destroyed.
     * Used for spatial cache queries when no specific player context is available.
     */
    std::unordered_map<uint32, std::unique_ptr<BattlegroundCoordinator>> const& GetAllCoordinators() const
    {
        return _coordinators;
    }

    // ========================================================================
    // STATISTICS
    // ========================================================================

    uint32 GetActiveCoordinatorCount() const;

private:
    BattlegroundCoordinatorManager();
    ~BattlegroundCoordinatorManager();

    /// Creates a coordinator for a BG on the main thread (thread-safe)
    void CreateCoordinatorForBG(Battleground* bg);

    /// Processes pending coordinator creation requests (called from main thread Update)
    void ProcessPendingCreations();

    // Map of BG instance ID -> Coordinator
    std::unordered_map<uint32, std::unique_ptr<BattlegroundCoordinator>> _coordinators;

    // BG instance ID → Battleground* queued for coordinator creation by worker threads.
    // Processed on the main thread in Update() to avoid thread-safety issues
    // with Battleground::GetPlayers(), map grid operations, etc.
    // The Battleground* is safe to store because BG destruction only happens
    // on the main thread, and we process pending creations before that.
    std::unordered_map<uint32, Battleground*> _pendingCreations;

    mutable OrderedRecursiveMutex<LockOrder::BEHAVIOR_MANAGER> _mutex;

    bool _initialized;
};

} // namespace Playerbot

#define sBGCoordinatorMgr Playerbot::BattlegroundCoordinatorManager::instance()
