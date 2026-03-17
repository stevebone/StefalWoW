/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ModuleLogManager.h"
#include "Config.h"
#include "Log.h"
#include "StringFormat.h"
#include <mutex>

namespace Trinity
{

ModuleLogManager* ModuleLogManager::instance()
{
    static ModuleLogManager instance;
    return &instance;
}

bool ModuleLogManager::RegisterModule(std::string const& moduleName, uint8 defaultLevel, std::string const& logFileName)
{
    printf("=== MODULE LOG DEBUG: RegisterModule() entered for '%s' ===\n", moduleName.c_str());

    std::lock_guard<std::mutex> lock(_loggerMutex);
    printf("=== MODULE LOG DEBUG: Got mutex lock ===\n");

    if (_moduleLoggers.find(moduleName) != _moduleLoggers.end())
    {
        printf("=== MODULE LOG DEBUG: Module already registered ===\n");
        TC_LOG_WARN("server.loading", "ModuleLogManager: Module '{}' already registered", moduleName);
        return false;
    }

    printf("=== MODULE LOG DEBUG: Creating ModuleLogInfo ===\n");
    auto info = std::make_unique<ModuleLogInfo>();
    info->name = moduleName;
    info->logLevel = defaultLevel;
    info->logFileName = logFileName.empty() ? Trinity::StringFormat("{}.log", moduleName) : logFileName;
    info->initialized = false;
    printf("=== MODULE LOG DEBUG: ModuleLogInfo created, logFileName='%s' ===\n", info->logFileName.c_str());

    // Generate standardized logger names
    printf("=== MODULE LOG DEBUG: About to call GenerateLoggerNames ===\n");
    auto names = GenerateLoggerNames(moduleName);
    printf("=== MODULE LOG DEBUG: GenerateLoggerNames returned ===\n");
    info->loggerName = names.first;
    info->appenderName = names.second;
    printf("=== MODULE LOG DEBUG: loggerName='%s', appenderName='%s' ===\n", info->loggerName.c_str(), info->appenderName.c_str());

    // Store log file name before moving the unique_ptr
    std::string logFileForDebug = info->logFileName;

    _moduleLoggers[moduleName] = std::move(info);

    TC_LOG_DEBUG("server.loading", "ModuleLogManager: Registered module '{}' with log file '{}'",
                 moduleName, logFileForDebug);

    return true;
}

bool ModuleLogManager::InitializeModuleLogging(std::string const& moduleName)
{
    std::lock_guard<std::mutex> lock(_loggerMutex);

    auto it = _moduleLoggers.find(moduleName);
    if (it == _moduleLoggers.end())
    {
        TC_LOG_ERROR("server.loading", "ModuleLogManager: Cannot initialize unregistered module '{}'", moduleName);
        return false;
    }

    auto& info = *it->second;
    if (info.initialized)
    {
        printf("=== MODULE LOG DEBUG: Module '%s' already initialized, re-initializing with updated config ===\n", moduleName.c_str());
        // Mark as not initialized so we can recreate with new settings
        info.initialized = false;
    }

    // Load configuration from worldserver.conf
    LoadModuleConfig(moduleName, info);

    // Create TrinityCore logger and appender
    if (!CreateModuleLogger(info))
    {
        TC_LOG_ERROR("server.loading", "ModuleLogManager: Failed to create logger for module '{}'", moduleName);
        return false;
    }

    info.initialized = true;

    TC_LOG_INFO("server.loading", "ModuleLogManager: Successfully initialized logging for module '{}' - Level: {}, File: '{}'",
                moduleName, info.logLevel, info.logFileName);

    return true;
}

bool ModuleLogManager::IsModuleInitialized(std::string const& moduleName) const
{
    std::lock_guard<std::mutex> lock(_loggerMutex);

    auto it = _moduleLoggers.find(moduleName);
    return it != _moduleLoggers.end() && it->second->initialized;
}

uint8 ModuleLogManager::GetModuleLogLevel(std::string const& moduleName) const
{
    std::lock_guard<std::mutex> lock(_loggerMutex);

    auto it = _moduleLoggers.find(moduleName);
    if (it == _moduleLoggers.end())
        return 255; // Invalid level

    return it->second->logLevel;
}

void ModuleLogManager::LogModuleMessage(std::string const& moduleName, uint8 level, std::string const& message)
{
    if (!IsModuleInitialized(moduleName))
        return;

    // Use TrinityCore's logging system with our unique file logger name
    std::string loggerName = Trinity::StringFormat("module.{}.file", moduleName);

    switch (level)
    {
        case 0: TC_LOG_FATAL(loggerName.c_str(), "{}", message); break;
        case 1: TC_LOG_ERROR(loggerName.c_str(), "{}", message); break;
        case 2: TC_LOG_WARN(loggerName.c_str(), "{}", message); break;
        case 3: TC_LOG_INFO(loggerName.c_str(), "{}", message); break;
        case 4: TC_LOG_DEBUG(loggerName.c_str(), "{}", message); break;
        case 5: TC_LOG_TRACE(loggerName.c_str(), "{}", message); break;
        default: TC_LOG_INFO(loggerName.c_str(), "{}", message); break;
    }
}

bool ModuleLogManager::SetModuleConfig(std::string const& moduleName, uint8 logLevel, std::string const& logFileName)
{
    std::lock_guard<std::mutex> lock(_loggerMutex);

    auto it = _moduleLoggers.find(moduleName);
    if (it == _moduleLoggers.end())
    {
        TC_LOG_ERROR("server.loading", "ModuleLogManager: Cannot set config for unregistered module '{}'", moduleName);
        return false;
    }

    auto& info = *it->second;
    info.logLevel = logLevel;
    if (!logFileName.empty())
        info.logFileName = logFileName;

    TC_LOG_DEBUG("server.loading", "ModuleLogManager: Updated config for module '{}' - Level: {}, File: '{}'",
                 moduleName, info.logLevel, info.logFileName);
    return true;
}

void ModuleLogManager::Shutdown()
{
    std::lock_guard<std::mutex> lock(_loggerMutex);

    TC_LOG_INFO("server.loading", "ModuleLogManager: Shutting down {} module loggers", _moduleLoggers.size());

    for (auto& [moduleName, info] : _moduleLoggers)
    {
        if (info->initialized)
        {
            TC_LOG_DEBUG("server.loading", "ModuleLogManager: Shutting down logger for module '{}'", moduleName);
            info->initialized = false;
        }
    }

    _moduleLoggers.clear();
}

bool ModuleLogManager::CreateModuleLogger(ModuleLogInfo& info)
{
    printf("=== MODULE LOG DEBUG: CreateModuleLogger() entered for '%s' ===\n", info.name.c_str());

    try
    {
        // Check if ModulePlayerbot appender already exists from worldserver.conf
        std::string preferredAppenderName = "ModulePlayerbot";
        printf("=== MODULE LOG DEBUG: Checking if existing appender '%s' exists ===\n", preferredAppenderName.c_str());

        // For playerbot module, create a NEW appender for the Playerbot.log file
        // The existing ModulePlayerbot appender in worldserver.conf goes to Playerbot1.log
        // We need our own appender for Playerbot.log (from playerbots.conf)
        std::string appenderConfig = Trinity::StringFormat("2,0,1,{}", info.logFileName);
        printf("=== MODULE LOG DEBUG: appenderConfig = '%s' ===\n", appenderConfig.c_str());

        std::string fullAppenderName = "Appender." + info.appenderName;
        printf("=== MODULE LOG DEBUG: About to call CreateAppenderFromConfigLine with '%s' ===\n", fullAppenderName.c_str());
        sLog->CreateAppenderFromConfigLine(fullAppenderName, appenderConfig);
        printf("=== MODULE LOG DEBUG: CreateAppenderFromConfigLine SUCCESS ===\n");

        // Check if logger already exists from worldserver.conf
        Logger const* existingLogger = sLog->GetEnabledLogger(info.loggerName, LOG_LEVEL_TRACE);
        if (existingLogger) {
            printf("=== MODULE LOG DEBUG: Found existing logger '%s', it already exists from worldserver.conf ===\n", info.loggerName.c_str());
            printf("=== MODULE LOG DEBUG: Existing logger found - our TC_LOG calls should work with existing configuration ===\n");
        } else {
            printf("=== MODULE LOG DEBUG: No existing logger, creating new one ===\n");
            // Create logger that routes to our appender
            std::string loggerConfig = Trinity::StringFormat("4,{}", info.appenderName);
            std::string fullLoggerName = "Logger." + info.loggerName;
            printf("=== MODULE LOG DEBUG: loggerConfig = '%s' ===\n", loggerConfig.c_str());
            printf("=== MODULE LOG DEBUG: About to call CreateLoggerFromConfigLine with '%s' ===\n", fullLoggerName.c_str());
            sLog->CreateLoggerFromConfigLine(fullLoggerName, loggerConfig);
            printf("=== MODULE LOG DEBUG: CreateLoggerFromConfigLine SUCCESS ===\n");
        }

        // Test the logger immediately - try both the specific logger and a direct appender test
        printf("=== MODULE LOG DEBUG: About to test TC_LOG to our logger '%s' ===\n", info.loggerName.c_str());
        TC_LOG_ERROR(info.loggerName.c_str(), "DIRECT TEST MESSAGE: ModuleLogManager successfully created logger!");
        printf("=== MODULE LOG DEBUG: TC_LOG test call completed ===\n");

        // Also test with the base module.playerbot logger that we know exists in worldserver.conf
        if (info.name == "playerbot") {
            printf("=== MODULE LOG DEBUG: Testing base module.playerbot logger from worldserver.conf ===\n");
            TC_LOG_ERROR("module.playerbot", "BASE LOGGER TEST: Testing existing module.playerbot logger from worldserver.conf!");
            printf("=== MODULE LOG DEBUG: Base logger test completed ===\n");

            // The issue is that Logger.module.playerbot=4,Console Server doesn't include ModulePlayerbot appender
            // Let's try to create a logger configuration that includes the ModulePlayerbot appender
            printf("=== MODULE LOG DEBUG: Creating logger configuration that routes to ModulePlayerbot appender ===\n");
            std::string enhancedLoggerConfig = Trinity::StringFormat("4,Console Server {}", info.appenderName);
            std::string enhancedLoggerName = "Logger.module.playerbot.enhanced";
            printf("=== MODULE LOG DEBUG: enhancedLoggerConfig = '%s' ===\n", enhancedLoggerConfig.c_str());
            printf("=== MODULE LOG DEBUG: About to create enhanced logger '%s' ===\n", enhancedLoggerName.c_str());
            sLog->CreateLoggerFromConfigLine(enhancedLoggerName, enhancedLoggerConfig);

            // Test the enhanced logger
            printf("=== MODULE LOG DEBUG: Testing enhanced logger ===\n");
            TC_LOG_ERROR("module.playerbot.enhanced", "ENHANCED LOGGER TEST: This should appear in Playerbot.log!");
            printf("=== MODULE LOG DEBUG: Enhanced logger test completed ===\n");
        }

        TC_LOG_DEBUG("server.loading", "ModuleLogManager: Created logger '{}' with appender '{}' for file '{}'",
                     info.loggerName, info.appenderName, info.logFileName);

        return true;
    }
    catch (std::exception const& e)
    {
        TC_LOG_ERROR("server.loading", "ModuleLogManager: Exception creating logger for '{}': {}", info.name, e.what());
        return false;
    }
    catch (...)
    {
        TC_LOG_ERROR("server.loading", "ModuleLogManager: Unknown exception creating logger for '{}'", info.name);
        return false;
    }
}

void ModuleLogManager::LoadModuleConfig(std::string const& moduleName, ModuleLogInfo& info)
{
    printf("=== MODULE LOG DEBUG: LoadModuleConfig() entered for '%s' ===\n", moduleName.c_str());

    // For playerbot module, try to get config from PlayerbotConfig if it's loaded
    if (moduleName == "playerbot") {
        printf("=== MODULE LOG DEBUG: This is playerbot module, trying to get config from PlayerbotConfig ===\n");

        // Try to get PlayerbotConfig instance (may not be loaded yet)
        // If PlayerbotConfig is loaded, it will call SetModuleConfig later
        // For now, we use the default values passed during registration
        printf("=== MODULE LOG DEBUG: Using registration defaults - Level: %d, File: '%s' ===\n",
               (int)info.logLevel, info.logFileName.c_str());
    }

    TC_LOG_DEBUG("server.loading", "ModuleLogManager: Using config for module '{}' - Level: {}, File: '{}'",
                 moduleName, info.logLevel, info.logFileName);
}

std::pair<std::string, std::string> ModuleLogManager::GenerateLoggerNames(std::string const& moduleName)
{
    // Generate unique names that won't conflict with worldserver.conf
    // Use "module.playerbot.file" instead of "module.playerbot" to avoid conflicts
    std::string loggerName = Trinity::StringFormat("module.{}.file", moduleName);
    std::string appenderName = Trinity::StringFormat("Module{}File", moduleName);

    // Capitalize first letter of module name in appender for consistency
    if (appenderName.length() > 6)
        appenderName[6] = std::toupper(appenderName[6]); // Position 6 is after "Module"

    return std::make_pair(loggerName, appenderName);
}

} // namespace Trinity