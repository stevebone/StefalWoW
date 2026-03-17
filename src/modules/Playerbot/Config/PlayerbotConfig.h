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

#ifndef TRINITY_PLAYERBOTCONFIG_H
#define TRINITY_PLAYERBOTCONFIG_H

#ifndef BUILD_PLAYERBOT
#define BUILD_PLAYERBOT 1
#endif

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "Logging/ModuleLogManager.h"
#include <map>
#include <string>
#include <mutex>

/**
 * @struct PerformanceMetrics
 * @brief Performance tracking metrics for configuration system
 */
struct PerformanceMetrics
{
    uint32 configLoadCount{0};
    mutable uint32 configLookups{0};
    mutable uint32 cacheHits{0};
    mutable uint32 cacheMisses{0};
    uint64 totalLoadTimeMs{0};
};

/**
 * @class PlayerbotConfig
 * @brief Configuration manager for the Playerbot module
 *
 * This singleton class manages all configuration settings for the playerbot system.
 * It loads settings from playerbots.conf (NOT worldserver.conf) and provides
 * type-safe access to configuration values with reasonable defaults.
 *
 * Key Features:
 * - Separate configuration file (playerbots.conf)
 * - Hot-reload capability
 * - Type-safe configuration access
 * - Default value fallbacks
 * - Configuration validation
 * - Zero impact on core TrinityCore configuration
 */
class TC_GAME_API PlayerbotConfig final
{
public:
    /**
     * @brief Get the singleton instance
     * @return Reference to the configuration instance
     */
    static PlayerbotConfig* instance();

    /**
     * @brief Initialize the configuration system
     * @return true if successful, false otherwise
     */
    bool Initialize();

    /**
     * @brief Reload configuration from file
     * @return true if successful, false otherwise
     */
    bool Reload();

    /**
     * @brief Check if configuration is loaded and valid
     * @return true if valid, false otherwise
     */
    bool IsValid() const { return _loaded; }

    // Configuration value getters with type safety and defaults

    /**
     * @brief Get boolean configuration value
     * @param key Configuration key
     * @param defaultValue Default value if key not found
     * @return Configuration value or default
     */
    bool GetBool(std::string const& key, bool defaultValue) const;

    /**
     * @brief Get integer configuration value
     * @param key Configuration key
     * @param defaultValue Default value if key not found
     * @return Configuration value or default
     */
    int32 GetInt(std::string const& key, int32 defaultValue) const;

    /**
     * @brief Get unsigned integer configuration value
     * @param key Configuration key
     * @param defaultValue Default value if key not found
     * @return Configuration value or default
     */
    uint32 GetUInt(std::string const& key, uint32 defaultValue) const;

    /**
     * @brief Get float configuration value
     * @param key Configuration key
     * @param defaultValue Default value if key not found
     * @return Configuration value or default
     */
    float GetFloat(std::string const& key, float defaultValue) const;

    /**
     * @brief Get string configuration value
     * @param key Configuration key
     * @param defaultValue Default value if key not found
     * @return Configuration value or default
     */
    std::string GetString(std::string const& key, std::string const& defaultValue) const;

    /**
     * @brief Get configuration file path
     * @return Path to playerbots.conf file
     */
    std::string GetConfigPath() const { return _configPath; }

    /**
     * @brief Get last error message
     * @return Error description or empty string
     */
    std::string GetLastError() const { return _lastError; }

    /**
     * @brief Initialize the playerbot logging system
     *
     * Sets up TrinityCore logging integration with:
     * - Separate Playerbot.log file
     * - Configurable log levels from playerbots.conf
     * - Specialized logging categories for different subsystems
     */
    void InitializeLogging();

    /**
     * @brief Get cached configuration value for performance-critical access
     * @param key Configuration key
     * @param defaultValue Default value if not cached
     * @return Cached value or fallback to normal lookup
     */
    template<typename T>
    T GetCached(std::string const& key, T defaultValue) const;

    /**
     * @brief Refresh configuration cache for frequently accessed values
     */
    void RefreshCache();

    /**
     * @brief Get performance metrics for monitoring
     * @return Performance statistics
     */
    PerformanceMetrics GetPerformanceMetrics() const;


private:
    PlayerbotConfig() = default;
    ~PlayerbotConfig() = default;

    // Non-copyable
    PlayerbotConfig(const PlayerbotConfig&) = delete;
    PlayerbotConfig& operator=(const PlayerbotConfig&) = delete;

    /**
     * @brief Find configuration file location
     * @return Path to configuration file
     */
    std::string FindConfigFile() const;

    /**
     * @brief Load configuration from file
     * @param filePath Path to configuration file
     * @return true if successful, false otherwise
     */
    bool LoadConfigFile(const std::string& filePath);

    /**
     * @brief Load profile overrides if Playerbot.Profile is specified
     * @note Called automatically after main config is loaded
     */
    void LoadProfile();

    /**
     * @brief Find profile configuration file
     * @param profileName Name of the profile (minimal, standard, performance, singleplayer)
     * @return Path to profile file, empty if not found
     */
    std::string FindProfileFile(std::string const& profileName);

    /**
     * @brief Validate loaded configuration
     * @return true if valid, false otherwise
     */
    bool ValidateConfiguration();

    /**
     * @brief Validate bot limit settings
     * @return true if valid, false otherwise
     */
    bool ValidateBotLimits();

    /**
     * @brief Validate timing and interval settings
     * @return true if valid, false otherwise
     */
    bool ValidateTimingSettings();

    /**
     * @brief Validate logging configuration
     * @return true if valid, false otherwise
     */
    bool ValidateLoggingSettings();

    /**
     * @brief Validate database settings
     * @return true if valid, false otherwise
     */
    bool ValidateDatabaseSettings();

    // Configuration state
    std::map<std::string, std::string> _configValues;
    std::string _configPath;
    std::string _lastError;
    bool _loaded = false;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::CONFIG_MANAGER> _configMutex;

    // Performance: Configuration caching for frequently accessed values
    struct ConfigCache {
        // Bot limits (accessed during every bot spawn)
        uint32 maxBotsPerAccount = 10;
        uint32 globalMaxBots = 1000;

        // Timing settings (accessed during every update cycle)
        uint32 updateInterval = 1000;
        uint32 aiDecisionTimeLimit = 50;
        uint32 loginDelay = 1000;

        // Group coordination intervals (milliseconds)
        uint32 groupUpdateInterval = 1000;      // Group state synchronization
        uint32 inviteResponseDelay = 2000;      // Bot invite acceptance delay
        uint32 readyCheckTimeout = 30000;       // Ready check expiration
        uint32 lootRollTimeout = 60000;         // Loot roll window duration
        uint32 targetUpdateInterval = 500;      // Target selection refresh

        // System update intervals (milliseconds)
        uint32 bankingCheckInterval = 300000;   // Banking evaluation (5 min)
        uint32 goldCheckInterval = 60000;       // Gold management (1 min)
        uint32 mountUpdateInterval = 5000;      // Mount state update
        uint32 petUpdateInterval = 5000;        // Battle pet update

        // Session management (milliseconds)
        uint32 sessionCleanupInterval = 10000;  // Session cleanup cycle
        uint32 maxLoadingTime = 30000;          // Max bot loading time
        uint32 sessionTimeout = 60000;          // Session expiration
        uint32 instanceBotIdleTimeout = 60000;  // Instance bot idle logout (1 min)
        uint32 instanceBotQueueTimeout = 300000; // Instance bot queue timeout (5 min)
        uint32 simulatedLatency = 50;           // Simulated network latency (ms)

        // History/transaction limits
        uint32 maxTransactionHistory = 100;     // Banking transaction log size
        uint32 maxConcurrentCommands = 5;       // Chat command queue limit

        // Account management
        uint32 targetPoolSize = 50;             // Bot account pool target

        // Logging settings (accessed during log operations)
        uint32 logLevel = 4;
        std::string logFile = "Playerbot.log";

        // Database settings (accessed during DB operations)
        uint32 databaseTimeout = 30;

        // Cache validity
        bool isValid = false;
    };
    mutable ConfigCache _cache;

    // Performance monitoring
    mutable PerformanceMetrics _metrics;

    // Default configuration filename
    static constexpr const char* CONFIG_FILENAME = "playerbots.conf";
};

// Global accessor
#define sPlayerbotConfig PlayerbotConfig::instance()

#endif // TRINITY_PLAYERBOTCONFIG_H