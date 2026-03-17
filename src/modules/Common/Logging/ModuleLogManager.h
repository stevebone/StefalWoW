/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "Log.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace Trinity
{

/**
 * @brief Centralized logging manager for TrinityCore modules
 *
 * This class provides a standardized logging API for modules, following TrinityCore
 * patterns while maintaining module isolation. It integrates with TrinityCore's
 * existing logging system and worldserver.conf configuration.
 *
 * Design Goals:
 * - Consistent logging API across all modules
 * - Integration with existing TrinityCore logging infrastructure
 * - Configuration through worldserver.conf (no separate config files)
 * - Automatic log file management
 * - Module isolation (no core modifications)
 */
class TC_GAME_API ModuleLogManager
{
public:
    ModuleLogManager(ModuleLogManager const&) = delete;
    ModuleLogManager& operator=(ModuleLogManager const&) = delete;

    static ModuleLogManager* instance();

    /**
     * @brief Register a module for logging
     * @param moduleName Name of the module (e.g., "playerbot", "auction")
     * @param defaultLevel Default log level if not configured
     * @param logFileName Optional specific log file name (defaults to ModuleName.log)
     */
    bool RegisterModule(std::string const& moduleName, uint8 defaultLevel = 3,
                       std::string const& logFileName = "");

    /**
     * @brief Initialize logging for a registered module
     * @param moduleName Name of the module to initialize
     * @return true if initialization succeeded
     */
    bool InitializeModuleLogging(std::string const& moduleName);

    /**
     * @brief Check if a module is registered and initialized
     * @param moduleName Name of the module to check
     * @return true if module logging is ready
     */
    bool IsModuleInitialized(std::string const& moduleName) const;

    /**
     * @brief Get the log level for a specific module
     * @param moduleName Name of the module
     * @return Log level (0-5), or 255 if module not found
     */
    uint8 GetModuleLogLevel(std::string const& moduleName) const;

    /**
     * @brief Log a message for a specific module
     * @param moduleName Name of the module
     * @param level Log level
     * @param message Message to log
     */
    void LogModuleMessage(std::string const& moduleName, uint8 level, std::string const& message);

    /**
     * @brief Set module-specific configuration after registration
     * @param moduleName Name of the module
     * @param logLevel Log level override
     * @param logFileName Log file name override
     */
    bool SetModuleConfig(std::string const& moduleName, uint8 logLevel, std::string const& logFileName);

    /**
     * @brief Shutdown logging for all registered modules
     */
    void Shutdown();

private:
    ModuleLogManager() = default;
    ~ModuleLogManager() = default;

    struct ModuleLogInfo
    {
        std::string name;
        std::string logFileName;
        std::string loggerName;     // Logger name in TrinityCore format
        std::string appenderName;   // Appender name in TrinityCore format
        uint8 logLevel;
        bool initialized;

        ModuleLogInfo() : logLevel(3), initialized(false) {}
    };

    std::unordered_map<std::string, std::unique_ptr<ModuleLogInfo>> _moduleLoggers;
    mutable std::mutex _loggerMutex;

    /**
     * @brief Create TrinityCore logger and appender for a module
     * @param info Module log information
     * @return true if creation succeeded
     */
    bool CreateModuleLogger(ModuleLogInfo& info);

    /**
     * @brief Get configuration from worldserver.conf for a module
     * @param moduleName Name of the module
     * @param info Module log information to populate
     */
    void LoadModuleConfig(std::string const& moduleName, ModuleLogInfo& info);

    /**
     * @brief Generate standardized logger/appender names
     * @param moduleName Name of the module
     * @return pair of logger name and appender name
     */
    std::pair<std::string, std::string> GenerateLoggerNames(std::string const& moduleName);
};

// Convenience macros for module logging
#define TC_LOG_MODULE_FATAL(module, format, ...) \
    do { \
        if (Trinity::ModuleLogManager::instance()->IsModuleInitialized(module)) { \
            std::string loggerName = std::string("module.") + module; \
            TC_LOG_FATAL(loggerName.c_str(), format, ##__VA_ARGS__); \
        } \
    } while (0)

#define TC_LOG_MODULE_ERROR(module, format, ...) \
    do { \
        if (Trinity::ModuleLogManager::instance()->IsModuleInitialized(module)) { \
            std::string loggerName = std::string("module.") + module; \
            TC_LOG_ERROR(loggerName.c_str(), format, ##__VA_ARGS__); \
        } \
    } while (0)

#define TC_LOG_MODULE_WARN(module, format, ...) \
    do { \
        if (Trinity::ModuleLogManager::instance()->IsModuleInitialized(module)) { \
            std::string loggerName = std::string("module.") + module; \
            TC_LOG_WARN(loggerName.c_str(), format, ##__VA_ARGS__); \
        } \
    } while (0)

#define TC_LOG_MODULE_INFO(module, format, ...) \
    do { \
        if (Trinity::ModuleLogManager::instance()->IsModuleInitialized(module)) { \
            std::string loggerName = std::string("module.") + module; \
            TC_LOG_INFO(loggerName.c_str(), format, ##__VA_ARGS__); \
        } \
    } while (0)

#define TC_LOG_MODULE_DEBUG(module, format, ...) \
    do { \
        if (Trinity::ModuleLogManager::instance()->IsModuleInitialized(module)) { \
            std::string loggerName = std::string("module.") + module; \
            TC_LOG_DEBUG(loggerName.c_str(), format, ##__VA_ARGS__); \
        } \
    } while (0)

#define TC_LOG_MODULE_TRACE(module, format, ...) \
    do { \
        if (Trinity::ModuleLogManager::instance()->IsModuleInitialized(module)) { \
            std::string loggerName = std::string("module.") + module; \
            TC_LOG_TRACE(loggerName.c_str(), format, ##__VA_ARGS__); \
        } \
    } while (0)

// Singleton access
#define sModuleLogManager Trinity::ModuleLogManager::instance()

} // namespace Trinity