/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITY_MODULE_MANAGER_H
#define TRINITY_MODULE_MANAGER_H

#include "Define.h"
#include <functional>
#include <vector>
#include <string>
#include <memory>

/**
 * @brief Universal Module Lifecycle Interface
 *
 * This system provides a reliable way for any TrinityCore module to register
 * for lifecycle events without depending on ScriptMgr inconsistencies.
 *
 * Usage:
 *   ModuleManager::RegisterModule("MyModule",
 *     MyModule::OnStartup,
 *     MyModule::OnUpdate,
 *     MyModule::OnShutdown);
 */

struct ModuleInterface
{
    std::string name;
    std::function<void()> onStartup;
    std::function<void(uint32)> onUpdate;
    std::function<void()> onShutdown;
    bool enabled = true;
};

class TC_GAME_API ModuleManager
{
public:
    /**
     * @brief Register a module for lifecycle events
     * @param name Module identifier (e.g. "Playerbot")
     * @param onStartup Called after world initialization
     * @param onUpdate Called every world update cycle
     * @param onShutdown Called during server shutdown
     */
    static void RegisterModule(std::string const& name,
                              std::function<void()> onStartup,
                              std::function<void(uint32)> onUpdate,
                              std::function<void()> onShutdown);

    /**
     * @brief Enable/disable a registered module
     * @param name Module identifier
     * @param enabled true to enable, false to disable
     */
    static void SetModuleEnabled(std::string const& name, bool enabled);

    // Internal lifecycle methods called by TrinityCore
    static void CallOnStartup();
    static void CallOnUpdate(uint32 diff);
    static void CallOnShutdown();

    // Status and debugging
    static std::vector<std::string> GetRegisteredModules();
    static bool IsModuleRegistered(std::string const& name);

private:
    static std::vector<ModuleInterface> s_modules;
    static bool s_initialized;
};

/**
 * @brief Convenience macro for module registration
 *
 * Usage in module initialization:
 *   REGISTER_MODULE("Playerbot",
 *     MyModule::OnStartup, MyModule::OnUpdate, MyModule::OnShutdown);
 */
#define REGISTER_MODULE(name, startup, update, shutdown) \
    ModuleManager::RegisterModule(name, \
        std::bind(&startup), \
        std::bind(&update, std::placeholders::_1), \
        std::bind(&shutdown))

#endif // TRINITY_MODULE_MANAGER_H