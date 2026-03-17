/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
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

#ifndef _PLAYERBOT_CONFIG_MANAGER_H
#define _PLAYERBOT_CONFIG_MANAGER_H

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include <map>
#include <string>
#include <mutex>
#include <variant>
#include <optional>
#include <functional>
#include <vector>

namespace Playerbot
{
    /**
     * @class ConfigManager
     * @brief Runtime configuration manager for playerbots
     *
     * Provides runtime modification of playerbot configuration values
     * with validation, persistence, and hot-reload capabilities.
     *
     * Features:
     * - Runtime value modification (via .bot config commands)
     * - Type-safe configuration storage
     * - Validation rules for each configuration key
     * - Thread-safe access with mutex protection
     * - Optional persistence to playerbots.conf
     * - Configuration change callbacks
     * - Hot-reload support
     *
     * Usage:
     * @code
     * ConfigManager* mgr = ConfigManager::instance();
     *
     * // Set configuration value
     * if (mgr->SetValue("MaxActiveBots", 200))
     * {
     *     // Successfully set
     * }
     *
     * // Get configuration value
     * uint32 maxBots = mgr->GetUInt("MaxActiveBots", 100);
     *
     * // Register change callback
     * mgr->RegisterCallback("MaxActiveBots", [](ConfigValue const& newValue) {
     *     // Handle configuration change
     * });
     * @endcode
     */
    class TC_GAME_API ConfigManager final
    {
    public:
        /**
         * @brief Configuration value type (supports multiple types)
         */
        using ConfigValue = ::std::variant<bool, int32, uint32, float, ::std::string>;

        /**
         * @brief Configuration entry with metadata
         */
        struct ConfigEntry
        {
            ConfigValue value;
            ::std::string description;
            ConfigValue defaultValue;
            bool persistent{false};
            bool readOnly{false};
            bool isModified{false};
            uint32 lastModifiedTime{0};
        };

        /**
         * @brief Configuration change callback type
         */
        using ChangeCallback = ::std::function<void(ConfigValue const&)>;

        /**
         * @brief Configuration validation rule
         */
        struct ValidationRule
        {
            ::std::string key;
            ::std::function<bool(ConfigValue const&)> validator;
            ::std::string errorMessage;
        };

        /**
         * @brief Get singleton instance
         */
        static ConfigManager* instance();

        // IConfigManager interface implementation
        /**
         * @brief Initialize configuration manager
         * @return true if successful, false otherwise
         */
        bool Initialize();

        /**
         * @brief Set configuration value (runtime modification)
         * @param key Configuration key
         * @param value New value
         * @return true if successful, false if validation failed
         */
        bool SetValue(::std::string const& key, ConfigValue const& value);

        /**
         * @brief Get boolean configuration value
         * @param key Configuration key
         * @param defaultValue Default value if not found
         * @return Configuration value or default
         */
        bool GetBool(::std::string const& key, bool defaultValue) const;

        /**
         * @brief Get signed integer configuration value
         * @param key Configuration key
         * @param defaultValue Default value if not found
         * @return Configuration value or default
         */
        int32 GetInt(::std::string const& key, int32 defaultValue) const;

        /**
         * @brief Get unsigned integer configuration value
         * @param key Configuration key
         * @param defaultValue Default value if not found
         * @return Configuration value or default
         */
        uint32 GetUInt(::std::string const& key, uint32 defaultValue) const;

        /**
         * @brief Get float configuration value
         * @param key Configuration key
         * @param defaultValue Default value if not found
         * @return Configuration value or default
         */
        float GetFloat(::std::string const& key, float defaultValue) const;

        /**
         * @brief Get string configuration value
         * @param key Configuration key
         * @param defaultValue Default value if not found
         * @return Configuration value or default
         */
        ::std::string GetString(::std::string const& key, ::std::string const& defaultValue) const;

        /**
         * @brief Register configuration change callback
         * @param key Configuration key to monitor
         * @param callback Function to call when value changes
         */
        void RegisterCallback(::std::string const& key, ChangeCallback callback);

        /**
         * @brief Get all configuration entries
         * @return Map of all configuration entries
         */
        ::std::map<::std::string, ConfigEntry> GetAllEntries() const;

        /**
         * @brief Reset configuration to defaults
         */
        void ResetToDefaults();

        /**
         * @brief Save configuration to file
         * @param filePath Path to configuration file (optional, uses default if empty)
         * @return true if successful, false otherwise
         */
        bool SaveToFile(::std::string const& filePath = "") const;

        /**
         * @brief Load configuration from file
         * @param filePath Path to configuration file
         * @return true if successful, false otherwise
         */
        bool LoadFromFile(::std::string const& filePath);

        /**
         * @brief Get last error message
         * @return Error description
         */
        ::std::string GetLastError() const { return _lastError; }

        /**
         * @brief Check if configuration key exists
         * @param key Configuration key
         * @return true if exists, false otherwise
         */
        bool HasKey(::std::string const& key) const;

        /**
         * @brief Get configuration entry (with metadata)
         * @param key Configuration key
         * @return Configuration entry or nullopt if not found
         */
        ::std::optional<ConfigEntry> GetEntry(::std::string const& key) const;

    private:
        ConfigManager() = default;
        ~ConfigManager() = default;

        // Non-copyable
        ConfigManager(ConfigManager const&) = delete;
        ConfigManager& operator=(ConfigManager const&) = delete;

        /**
         * @brief Register default configuration entries
         */
        void RegisterDefaultEntries();

        /**
         * @brief Validate configuration value
         * @param key Configuration key
         * @param value Value to validate
         * @return true if valid, false otherwise
         */
        bool ValidateValue(::std::string const& key, ConfigValue const& value);

        /**
         * @brief Trigger change callbacks for key
         * @param key Configuration key
         * @param newValue New value
         */
        void TriggerCallbacks(::std::string const& key, ConfigValue const& newValue);

        // Configuration storage
        ::std::map<::std::string, ConfigEntry> _entries;
        ::std::map<::std::string, ::std::vector<ChangeCallback>> _callbacks;
        ::std::map<::std::string, ValidationRule> _validationRules;

        // Thread safety
        mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::CONFIG_MANAGER> _mutex;

        // State
        ::std::string _lastError;
        bool _initialized = false;
    };

} // namespace Playerbot

#endif // _PLAYERBOT_CONFIG_MANAGER_H
