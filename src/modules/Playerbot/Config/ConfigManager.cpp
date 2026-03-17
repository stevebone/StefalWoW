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

#include "ConfigManager.h"
#include "Log.h"
#include <fstream>
#include <sstream>

namespace Playerbot
{
    ConfigManager* ConfigManager::instance()
    {
        static ConfigManager instance;
        return &instance;
    }

    bool ConfigManager::Initialize()
    {
        ::std::lock_guard lock(_mutex);

        if (_initialized)
            return true;

        TC_LOG_INFO("playerbot.config", "Initializing ConfigManager...");

        // Register all default configuration entries
        RegisterDefaultEntries();

        _initialized = true;
        TC_LOG_INFO("playerbot.config", "ConfigManager initialized successfully");

        return true;
    }

    void ConfigManager::RegisterDefaultEntries()
    {
        // Bot Limits
        _entries["MaxActiveBots"] = ConfigEntry{
            static_cast<uint32>(100),
            "Maximum number of concurrent bots",
            static_cast<uint32>(100),
            true,  // persistent
            false  // not read-only
        };

        _entries["MaxBotsPerAccount"] = ConfigEntry{
            static_cast<uint32>(10),
            "Maximum bots per account",
            static_cast<uint32>(10),
            true,
            false
        };

        _entries["GlobalMaxBots"] = ConfigEntry{
            static_cast<uint32>(1000),
            "Global bot limit (all accounts)",
            static_cast<uint32>(1000),
            true,
            false
        };

        // Performance Settings
        _entries["BotUpdateInterval"] = ConfigEntry{
            static_cast<uint32>(100),
            "Bot update interval in milliseconds",
            static_cast<uint32>(100),
            true,
            false
        };

        _entries["AIDecisionTimeLimit"] = ConfigEntry{
            static_cast<uint32>(50),
            "AI decision time limit in milliseconds",
            static_cast<uint32>(50),
            true,
            false
        };

        _entries["DatabaseBatchSize"] = ConfigEntry{
            static_cast<uint32>(100),
            "Database batch operation size",
            static_cast<uint32>(100),
            true,
            false
        };

        // AI Behavior Toggles
        _entries["EnableCombatAI"] = ConfigEntry{
            true,
            "Enable combat AI for bots",
            true,
            true,
            false
        };

        _entries["EnableQuestAI"] = ConfigEntry{
            true,
            "Enable quest automation AI",
            true,
            true,
            false
        };

        _entries["EnableSocialAI"] = ConfigEntry{
            true,
            "Enable social interaction AI",
            true,
            true,
            false
        };

        _entries["EnableProfessionAI"] = ConfigEntry{
            false,
            "Enable profession automation AI",
            false,
            true,
            false
        };

        // Logging Settings
        _entries["LogLevel"] = ConfigEntry{
            static_cast<uint32>(4),
            "Logging level (0=Disabled, 1=Error, 2=Warn, 3=Info, 4=Debug, 5=Trace)",
            static_cast<uint32>(4),
            true,
            false
        };

        _entries["LogFile"] = ConfigEntry{
            ::std::string("Playerbot.log"),
            "Log file name",
            ::std::string("Playerbot.log"),
            true,
            false
        };

        // Formation Settings
        _entries["DefaultFormation"] = ConfigEntry{
            ::std::string("wedge"),
            "Default tactical formation",
            ::std::string("wedge"),
            true,
            false
        };

        _entries["FormationSpacing"] = ConfigEntry{
            3.0f,
            "Formation spacing in meters",
            3.0f,
            true,
            false
        };

        // Database Settings
        _entries["DatabaseTimeout"] = ConfigEntry{
            static_cast<uint32>(30),
            "Database query timeout in seconds",
            static_cast<uint32>(30),
            true,
            false
        };

        _entries["ConnectionPoolSize"] = ConfigEntry{
            static_cast<uint32>(50),
            "Database connection pool size",
            static_cast<uint32>(50),
            true,
            false
        };

        // Register validation rules
        _validationRules["MaxActiveBots"] = ValidationRule{
            "MaxActiveBots",
            [](ConfigValue const& value) {
                uint32 val = ::std::get<uint32>(value);
                return val > 0 && val <= 5000;
            },
            "MaxActiveBots must be between 1 and 5000"
        };

        _validationRules["BotUpdateInterval"] = ValidationRule{
            "BotUpdateInterval",
            [](ConfigValue const& value) {
                uint32 val = ::std::get<uint32>(value);
                return val >= 10 && val <= 10000;
            },
            "BotUpdateInterval must be between 10 and 10000 milliseconds"
        };

        _validationRules["LogLevel"] = ValidationRule{
            "LogLevel",
            [](ConfigValue const& value) {
                uint32 val = ::std::get<uint32>(value);
                return val <= 5;
            },
            "LogLevel must be between 0 and 5"
        };

        _validationRules["FormationSpacing"] = ValidationRule{
            "FormationSpacing",
            [](ConfigValue const& value) {
                float val = ::std::get<float>(value);
                return val >= 1.0f && val <= 10.0f;
            },
            "FormationSpacing must be between 1.0 and 10.0 meters"
        };

        // ========================================================================
        // HUMANIZATION SETTINGS (Phase 3)
        // ========================================================================

        // General
        _entries["Humanization.Enabled"] = ConfigEntry{
            true,
            "Enable humanization system for realistic bot behavior",
            true,
            true,
            false
        };

        _entries["Humanization.DebugLevel"] = ConfigEntry{
            static_cast<uint32>(0),
            "Debug logging level (0=off, 1=basic, 2=verbose)",
            static_cast<uint32>(0),
            true,
            false
        };

        _entries["Humanization.ApplyToExisting"] = ConfigEntry{
            true,
            "Apply humanization to existing bots on load",
            true,
            true,
            false
        };

        // Session
        _entries["Humanization.Session.MinDuration"] = ConfigEntry{
            static_cast<uint32>(1800000),
            "Minimum session duration in milliseconds (30 min default)",
            static_cast<uint32>(1800000),
            true,
            false
        };

        _entries["Humanization.Session.MaxDuration"] = ConfigEntry{
            static_cast<uint32>(5400000),
            "Maximum session duration in milliseconds (90 min default)",
            static_cast<uint32>(5400000),
            true,
            false
        };

        _entries["Humanization.Session.ExtendChance"] = ConfigEntry{
            static_cast<uint32>(20),
            "Chance to extend session (percent)",
            static_cast<uint32>(20),
            true,
            false
        };

        _entries["Humanization.Session.MaxExtensions"] = ConfigEntry{
            static_cast<uint32>(2),
            "Maximum number of session extensions",
            static_cast<uint32>(2),
            true,
            false
        };

        // Breaks
        _entries["Humanization.Break.Short.Min"] = ConfigEntry{
            static_cast<uint32>(60000),
            "Short break minimum duration in ms (1 min)",
            static_cast<uint32>(60000),
            true,
            false
        };

        _entries["Humanization.Break.Short.Max"] = ConfigEntry{
            static_cast<uint32>(300000),
            "Short break maximum duration in ms (5 min)",
            static_cast<uint32>(300000),
            true,
            false
        };

        _entries["Humanization.Break.Long.Min"] = ConfigEntry{
            static_cast<uint32>(300000),
            "Long break minimum duration in ms (5 min)",
            static_cast<uint32>(300000),
            true,
            false
        };

        _entries["Humanization.Break.Long.Max"] = ConfigEntry{
            static_cast<uint32>(900000),
            "Long break maximum duration in ms (15 min)",
            static_cast<uint32>(900000),
            true,
            false
        };

        _entries["Humanization.Break.LongChance"] = ConfigEntry{
            static_cast<uint32>(10),
            "Chance for long break instead of short (percent)",
            static_cast<uint32>(10),
            true,
            false
        };

        // AFK
        _entries["Humanization.AFK.Enabled"] = ConfigEntry{
            true,
            "Enable AFK simulation",
            true,
            true,
            false
        };

        _entries["Humanization.AFK.ChancePerHour"] = ConfigEntry{
            static_cast<uint32>(5),
            "Chance per hour of going AFK (percent)",
            static_cast<uint32>(5),
            true,
            false
        };

        // Personality
        _entries["Humanization.Personality.Random"] = ConfigEntry{
            true,
            "Assign random personalities to new bots",
            true,
            true,
            false
        };

        _entries["Humanization.Personality.Variance"] = ConfigEntry{
            0.15f,
            "Personality variance (0.0-1.0)",
            0.15f,
            true,
            false
        };

        // Emotes
        _entries["Humanization.Emotes.Enabled"] = ConfigEntry{
            true,
            "Enable idle emotes",
            true,
            true,
            false
        };

        _entries["Humanization.Emotes.Frequency"] = ConfigEntry{
            0.1f,
            "Base emote frequency (0.0-1.0)",
            0.1f,
            true,
            false
        };

        // Time of Day
        _entries["Humanization.TimeOfDay.Enabled"] = ConfigEntry{
            true,
            "Enable time-of-day activity variation",
            true,
            true,
            false
        };
    }

    bool ConfigManager::SetValue(::std::string const& key, ConfigValue const& value)
    {
        ::std::lock_guard lock(_mutex);

        if (!_initialized)
        {
            _lastError = "ConfigManager not initialized";
            return false;
        }

        // Check if key exists
        auto it = _entries.find(key);
        if (it == _entries.end())
        {
            _lastError = "Configuration key '" + key + "' does not exist";
            return false;
        }

        // Check if read-only
    if (it->second.readOnly)
        {
            _lastError = "Configuration key '" + key + "' is read-only";
            return false;
        }

        // Validate new value
    if (!ValidateValue(key, value))
        {
            // _lastError set by ValidateValue
            return false;
        }

        // Update value
        ConfigValue oldValue = it->second.value;
        it->second.value = value;

        TC_LOG_INFO("playerbot.config", "Configuration changed: %s", key.c_str());

        // Trigger callbacks
        TriggerCallbacks(key, value);

        return true;
    }

    bool ConfigManager::GetBool(::std::string const& key, bool defaultValue) const
    {
        ::std::lock_guard lock(_mutex);

        auto it = _entries.find(key);
        if (it == _entries.end())
            return defaultValue;

        try
        {
            return ::std::get<bool>(it->second.value);
        }
        catch (::std::bad_variant_access const&)
        {
            TC_LOG_WARN("playerbot.config", "Type mismatch for key '%s', expected bool", key.c_str());
            return defaultValue;
        }
    }

    int32 ConfigManager::GetInt(::std::string const& key, int32 defaultValue) const
    {
        ::std::lock_guard lock(_mutex);

        auto it = _entries.find(key);
        if (it == _entries.end())
            return defaultValue;

        try
        {
            return ::std::get<int32>(it->second.value);
        }
        catch (::std::bad_variant_access const&)
        {
            TC_LOG_WARN("playerbot.config", "Type mismatch for key '%s', expected int32", key.c_str());
            return defaultValue;
        }
    }

    uint32 ConfigManager::GetUInt(::std::string const& key, uint32 defaultValue) const
    {
        ::std::lock_guard lock(_mutex);

        auto it = _entries.find(key);
        if (it == _entries.end())
            return defaultValue;

        try
        {
            return ::std::get<uint32>(it->second.value);
        }
        catch (::std::bad_variant_access const&)
        {
            TC_LOG_WARN("playerbot.config", "Type mismatch for key '%s', expected uint32", key.c_str());
            return defaultValue;
        }
    }

    float ConfigManager::GetFloat(::std::string const& key, float defaultValue) const
    {
        ::std::lock_guard lock(_mutex);

        auto it = _entries.find(key);
        if (it == _entries.end())
            return defaultValue;

        try
        {
            return ::std::get<float>(it->second.value);
        }
        catch (::std::bad_variant_access const&)
        {
            TC_LOG_WARN("playerbot.config", "Type mismatch for key '%s', expected float", key.c_str());
            return defaultValue;
        }
    }

    ::std::string ConfigManager::GetString(::std::string const& key, ::std::string const& defaultValue) const
    {
        ::std::lock_guard lock(_mutex);

        auto it = _entries.find(key);
        if (it == _entries.end())
            return defaultValue;

        try
        {
            return ::std::get<::std::string>(it->second.value);
        }
        catch (::std::bad_variant_access const&)
        {
            TC_LOG_WARN("playerbot.config", "Type mismatch for key '%s', expected string", key.c_str());
            return defaultValue;
        }
    }

    void ConfigManager::RegisterCallback(::std::string const& key, ChangeCallback callback)
    {
        ::std::lock_guard lock(_mutex);
        _callbacks[key].push_back(callback);
    }

    ::std::map<::std::string, ConfigManager::ConfigEntry> ConfigManager::GetAllEntries() const
    {
        ::std::lock_guard lock(_mutex);
        return _entries;
    }

    void ConfigManager::ResetToDefaults()
    {
        ::std::lock_guard lock(_mutex);

        TC_LOG_INFO("playerbot.config", "Resetting all configuration to defaults");

        for (auto& [key, entry] : _entries)
        {
            entry.value = entry.defaultValue;
        }
    }

    bool ConfigManager::SaveToFile(::std::string const& filePath) const
    {
        ::std::lock_guard lock(_mutex);

        ::std::string path = filePath.empty() ? "playerbots_runtime.conf" : filePath;

        ::std::ofstream file(path);
        if (!file.is_open())
        {
            const_cast<ConfigManager*>(this)->_lastError = "Failed to open file for writing: " + path;
            return false;
        }

        file << "###############################################\n";
        file << "# Playerbot Runtime Configuration\n";
        file << "# Auto-generated by ConfigManager\n";
        file << "###############################################\n\n";

        for (auto const& [key, entry] : _entries)
        {
            if (!entry.persistent)
                continue;

            file << "# " << entry.description << "\n";

            ::std::visit([&file, &key](auto&& value) {
                using T = ::std::decay_t<decltype(value)>;
                if constexpr (::std::is_same_v<T, bool>)
                    file << key << " = " << (value ? "1" : "0") << "\n";
                else if constexpr (::std::is_same_v<T, ::std::string>)
                    file << key << " = \"" << value << "\"\n";
                else
                    file << key << " = " << value << "\n";
            }, entry.value);

            file << "\n";
        }

        file.close();
        TC_LOG_INFO("playerbot.config", "Configuration saved to: %s", path.c_str());

        return true;
    }

    bool ConfigManager::LoadFromFile(::std::string const& filePath)
    {
        ::std::lock_guard lock(_mutex);

        ::std::ifstream file(filePath);
        if (!file.is_open())
        {
            _lastError = "Failed to open configuration file: " + filePath;
            return false;
        }

        ::std::string line;
        uint32 lineNumber = 0;

        while (::std::getline(file, line))
        {
            lineNumber++;

            // Skip empty lines and comments
    if (line.empty() || line[0] == '#')
                continue;

            // Parse key = value
            size_t equalPos = line.find('=');
            if (equalPos == ::std::string::npos)
                continue;

            ::std::string key = line.substr(0, equalPos);
            ::std::string value = line.substr(equalPos + 1);

            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            // Remove quotes from string values
    if (!value.empty() && value.front() == '"' && value.back() == '"')
                value = value.substr(1, value.length() - 2);

            // Find entry and convert value
            auto it = _entries.find(key);
            if (it == _entries.end())
            {
                TC_LOG_WARN("playerbot.config", "Unknown configuration key '%s' on line %u", key.c_str(), lineNumber);
                continue;
            }

            // Convert value to appropriate type
            ConfigValue configValue;
            try
            {
                ::std::visit([&value, &configValue](auto&& defaultVal) {
                    using T = ::std::decay_t<decltype(defaultVal)>;
                    if constexpr (::std::is_same_v<T, bool>)
                        configValue = (value == "1" || value == "true");
                    else if constexpr (::std::is_same_v<T, int32>)
                        configValue = static_cast<int32>(::std::stoi(value));
                    else if constexpr (::std::is_same_v<T, uint32>)
                        configValue = static_cast<uint32>(::std::stoul(value));
                    else if constexpr (::std::is_same_v<T, float>)
                        configValue = ::std::stof(value);
                    else if constexpr (::std::is_same_v<T, ::std::string>)
                        configValue = value;
                }, it->second.defaultValue);

                // Set value (with validation)
                SetValue(key, configValue);
            }
            catch (::std::exception const& ex)
            {
                TC_LOG_ERROR("playerbot.config", "Failed to parse value for key '%s' on line %u: %s",
                            key.c_str(), lineNumber, ex.what());
            }
        }

        file.close();
        TC_LOG_INFO("playerbot.config", "Configuration loaded from: %s", filePath.c_str());

        return true;
    }

    bool ConfigManager::HasKey(::std::string const& key) const
    {
        ::std::lock_guard lock(_mutex);
        return _entries.find(key) != _entries.end();
    }

    ::std::optional<ConfigManager::ConfigEntry> ConfigManager::GetEntry(::std::string const& key) const
    {
        ::std::lock_guard lock(_mutex);

        auto it = _entries.find(key);
        if (it == _entries.end())
            return ::std::nullopt;

        return it->second;
    }

    bool ConfigManager::ValidateValue(::std::string const& key, ConfigValue const& value)
    {
        // Check if validation rule exists
        auto it = _validationRules.find(key);
        if (it == _validationRules.end())
            return true;  // No validation rule, accept value

        // Run validation
    if (!it->second.validator(value))
        {
            _lastError = it->second.errorMessage;
            return false;
        }

        return true;
    }

    void ConfigManager::TriggerCallbacks(::std::string const& key, ConfigValue const& newValue)
    {
        auto it = _callbacks.find(key);
        if (it == _callbacks.end())
            return;

        for (auto const& callback : it->second)
        {
            try
            {
                callback(newValue);
            }
            catch (::std::exception const& ex)
            {
                TC_LOG_ERROR("playerbot.config", "Exception in config callback for key '%s': %s",
                            key.c_str(), ex.what());
            }
        }
    }

} // namespace Playerbot
