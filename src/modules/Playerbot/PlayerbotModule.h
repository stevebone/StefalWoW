/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRINITY_PLAYERBOTMODULE_H
#define TRINITY_PLAYERBOTMODULE_H

#ifdef BUILD_PLAYERBOT

#include "Define.h"
#include <string>

/**
 * @class PlayerbotModule
 * @brief Main entry point for the TrinityCore Playerbot Module
 *
 * This class provides the primary interface for initializing, managing, and shutting down
 * the optional playerbot system. It follows TrinityCore's module pattern for optional
 * functionality that can be completely disabled at compile-time.
 *
 * Key Features:
 * - Optional compilation via BUILD_PLAYERBOT CMake flag
 * - Zero impact on core TrinityCore when disabled
 * - Separate configuration system (playerbots.conf)
 * - Clean shutdown and resource management
 * - Runtime enable/disable capability
 */
class TC_GAME_API PlayerbotModule
{
public:
    /**
     * @brief Initialize the playerbot module
     * @return true if initialization successful, false otherwise
     *
     * Called during server startup to:
     * - Load configuration from playerbots.conf
     * - Initialize database connections
     * - Register event hooks with TrinityCore
     * - Validate system requirements
     */
    static bool Initialize();

    /**
     * @brief Shutdown the playerbot module
     *
     * Called during server shutdown to:
     * - Clean up active bot sessions
     * - Close database connections
     * - Unregister event hooks
     * - Free allocated resources
     */
    static void Shutdown();

    /**
     * @brief Check if playerbot system is enabled
     * @return true if enabled and operational, false otherwise
     *
     * Runtime check that considers:
     * - Configuration setting (Playerbot.Enable)
     * - Successful initialization state
     * - Current operational status
     */
    static bool IsEnabled();

    /**
     * @brief Get module version information
     * @return Version string in format "Major.Minor.Patch"
     */
    static std::string GetVersion();

    /**
     * @brief Get module build information
     * @return Build info including compile date and options
     */
    static std::string GetBuildInfo();

    /**
     * @brief Trigger character login for all bot sessions without players
     *
     * Manually initiates StartAsyncLogin for existing bot sessions that have
     * no associated Player objects. This is useful for completing login for
     * sessions that were created but never properly logged in.
     */
    static void TriggerBotCharacterLogins();

    /**
     * @brief Update all bot systems
     * @param diff Time difference since last update in milliseconds
     *
     * Called periodically by ModuleManager to:
     * - Update BotSpawner population targets
     * - Process spawn/despawn requests
     * - Maintain bot populations in zones
     */
    static void OnWorldUpdate(uint32 diff);

private:
    /**
     * @brief Register hooks with TrinityCore event system
     *
     * Registers callbacks for:
     * - Player login/logout events
     * - World update cycles
     * - Configuration reload events
     * - Server shutdown events
     */
    static void RegisterHooks();

    /**
     * @brief Unregister all module hooks
     */
    static void UnregisterHooks();

    /**
     * @brief Validate configuration settings
     * @return true if configuration is valid, false otherwise
     *
     * Performs sanity checks on:
     * - Database connection parameters
     * - Bot count limits
     * - Performance thresholds
     * - Feature flags
     */
    static bool ValidateConfig();

    /**
     * @brief Initialize module logging
     *
     * Sets up separate log channels for:
     * - Bot AI decisions
     * - Performance metrics
     * - Error conditions
     * - Debug information
     */
    static void InitializeLogging();

    /**
     * @brief Initialize Playerbot Database connection
     * @return true if database initialization successful, false otherwise
     *
     * Initializes the separate PlayerbotDatabase connection using
     * configuration from playerbots.conf. This provides isolated
     * database access for all playerbot-related data.
     */
    static bool InitializeDatabase();

    /**
     * @brief Initialize all sub-managers
     * @return true if all managers initialized successfully
     */
    static bool InitializeManagers();

    /**
     * @brief Shutdown Playerbot Database connection
     *
     * Cleanly closes the PlayerbotDatabase connection and
     * releases all associated resources.
     */
    static void ShutdownDatabase();

    // Module state
    inline static bool _initialized = false;
    inline static bool _enabled = false;
    inline static std::string _lastError = "";

    // Version constants
    inline static constexpr uint32 MODULE_VERSION_MAJOR = 1;
    inline static constexpr uint32 MODULE_VERSION_MINOR = 0;
    inline static constexpr uint32 MODULE_VERSION_PATCH = 0;
};

#endif // BUILD_PLAYERBOT
#endif // TRINITY_PLAYERBOTMODULE_H