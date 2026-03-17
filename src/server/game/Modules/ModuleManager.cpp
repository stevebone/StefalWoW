/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ModuleManager.h"
#include "Log.h"
#include "Timer.h"
#include <algorithm>

std::vector<ModuleInterface> ModuleManager::s_modules;
bool ModuleManager::s_initialized = false;

void ModuleManager::RegisterModule(std::string const& name,
                                  std::function<void()> onStartup,
                                  std::function<void(uint32)> onUpdate,
                                  std::function<void()> onShutdown)
{
    // Check if module already registered
    auto it = std::find_if(s_modules.begin(), s_modules.end(),
        [&name](const ModuleInterface& module) { return module.name == name; });

    if (it != s_modules.end())
    {
        TC_LOG_WARN("modules", "ModuleManager: Module '{}' already registered, replacing", name);
        s_modules.erase(it);
    }

    ModuleInterface module;
    module.name = name;
    module.onStartup = onStartup;
    module.onUpdate = onUpdate;
    module.onShutdown = onShutdown;
    module.enabled = true;

    s_modules.push_back(module);

    TC_LOG_INFO("modules", "ModuleManager: Registered module '{}'", name);
}

void ModuleManager::SetModuleEnabled(std::string const& name, bool enabled)
{
    auto it = std::find_if(s_modules.begin(), s_modules.end(),
        [&name](ModuleInterface& module) { return module.name == name; });

    if (it != s_modules.end())
    {
        it->enabled = enabled;
        TC_LOG_INFO("modules", "ModuleManager: Module '{}' {}", name, enabled ? "enabled" : "disabled");
    }
    else
    {
        TC_LOG_WARN("modules", "ModuleManager: Cannot set enabled state for unknown module '{}'", name);
    }
}

void ModuleManager::CallOnStartup()
{
    if (s_initialized)
    {
        TC_LOG_WARN("modules", "ModuleManager: OnStartup called multiple times");
        return;
    }

    TC_LOG_INFO("modules", "ModuleManager: Calling OnStartup for {} registered modules", s_modules.size());

    for (auto& module : s_modules)
    {
        if (!module.enabled)
            continue;

        try
        {
            TC_LOG_DEBUG("modules", "ModuleManager: Calling OnStartup for module '{}'", module.name);
            uint32 startTime = getMSTime();

            if (module.onStartup)
                module.onStartup();

            uint32 duration = GetMSTimeDiffToNow(startTime);
            TC_LOG_DEBUG("modules", "ModuleManager: Module '{}' OnStartup completed in {}ms", module.name, duration);
        }
        catch (std::exception const& e)
        {
            TC_LOG_ERROR("modules", "ModuleManager: Module '{}' OnStartup failed: {}", module.name, e.what());
        }
    }

    s_initialized = true;
    TC_LOG_INFO("modules", "ModuleManager: All modules started successfully");
}

void ModuleManager::CallOnUpdate(uint32 diff)
{
    if (!s_initialized)
        return;

    for (auto& module : s_modules)
    {
        if (!module.enabled)
            continue;

        try
        {
            if (module.onUpdate)
                module.onUpdate(diff);
        }
        catch (std::exception const& e)
        {
            TC_LOG_ERROR("modules", "ModuleManager: Module '{}' OnUpdate failed: {}", module.name, e.what());
            // Disable module on repeated failures to prevent spam
            module.enabled = false;
            TC_LOG_WARN("modules", "ModuleManager: Disabled module '{}' due to errors", module.name);
        }
    }
}

void ModuleManager::CallOnShutdown()
{
    if (!s_initialized)
        return;

    TC_LOG_INFO("modules", "ModuleManager: Shutting down {} modules", s_modules.size());

    // Call shutdown in reverse order
    for (auto it = s_modules.rbegin(); it != s_modules.rend(); ++it)
    {
        if (!it->enabled)
            continue;

        try
        {
            TC_LOG_DEBUG("modules", "ModuleManager: Calling OnShutdown for module '{}'", it->name);

            if (it->onShutdown)
                it->onShutdown();

            TC_LOG_DEBUG("modules", "ModuleManager: Module '{}' shutdown completed", it->name);
        }
        catch (std::exception const& e)
        {
            TC_LOG_ERROR("modules", "ModuleManager: Module '{}' OnShutdown failed: {}", it->name, e.what());
        }
    }

    TC_LOG_INFO("modules", "ModuleManager: All modules shut down");
}

std::vector<std::string> ModuleManager::GetRegisteredModules()
{
    std::vector<std::string> names;
    for (const auto& module : s_modules)
        names.push_back(module.name);
    return names;
}

bool ModuleManager::IsModuleRegistered(std::string const& name)
{
    return std::find_if(s_modules.begin(), s_modules.end(),
        [&name](const ModuleInterface& module) { return module.name == name; }) != s_modules.end();
}