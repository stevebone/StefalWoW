/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_WORLD_SCRIPT_H
#define PLAYERBOT_WORLD_SCRIPT_H

#include "ScriptMgr.h"
#include "Define.h"

/**
 * @class PlayerbotWorldScript
 * @brief World update integration for the Playerbot module
 *
 * This class provides proper integration with TrinityCore's world update cycle
 * using the official WorldScript system. It handles:
 * - Periodic bot spawner updates
 * - Configuration reload handling
 * - Clean startup and shutdown
 * - Performance monitoring
 */
class TC_GAME_API PlayerbotWorldScript : public WorldScript
{
public:
    PlayerbotWorldScript();

    /**
     * @brief Called every world update cycle
     * @param diff Time difference since last update in milliseconds
     *
     * This is the main update hook that drives all playerbot systems:
     * - BotSpawner population management
     * - Bot AI updates
     * - Session management
     * - Performance monitoring
     */
    void OnUpdate(uint32 diff);

    /**
     * @brief Called when configuration is loaded or reloaded
     * @param reload true if this is a reload, false for initial load
     *
     * Handles dynamic configuration changes for:
     * - Bot spawn settings
     * - Performance parameters
     * - Feature flags
     */
    void OnConfigLoad(bool reload);

    /**
     * @brief Called during world startup
     *
     * Performs initialization that requires the world to be fully loaded:
     * - Register with script systems
     * - Initialize performance monitoring
     * - Start background services
     */
    void OnStartup();

    /**
     * @brief Called when world shutdown is initiated
     * @param code Shutdown exit code
     * @param mask Shutdown mask
     *
     * Ensures clean shutdown of all playerbot systems:
     * - Despawn all active bots
     * - Save state to database
     * - Release resources
     */
    void OnShutdownInitiate(ShutdownExitCode code, ShutdownMask mask);

private:
    /**
     * @brief Update all bot systems
     * @param diff Time difference in milliseconds
     */
    void UpdateBotSystems(uint32 diff);

    /**
     * @brief Check if playerbot system is enabled
     * @return true if enabled and operational
     */
    bool IsPlayerbotEnabled() const;

    /**
     * @brief Update performance metrics
     * @param updateTime Time spent in update cycle
     */
    void UpdateMetrics(uint32 updateTime);

    // Performance tracking
    uint32 _lastMetricUpdate = 0;
    uint32 _totalUpdateTime = 0;
    uint32 _updateCount = 0;

    // Level distribution rebalancing timer
    uint32 _lastRebalanceCheck = 0;

    static constexpr uint32 METRIC_UPDATE_INTERVAL = 60000; // 1 minute
    static constexpr uint32 REBALANCE_INTERVAL = 300000;    // 5 minutes
};

/**
 * @brief Register Playerbot world scripts
 *
 * This function is called by the script loader to register
 * all playerbot world scripts with the ScriptMgr system.
 */
void AddSC_playerbot_world();

#endif // PLAYERBOT_WORLD_SCRIPT_H