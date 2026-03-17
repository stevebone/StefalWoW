/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PlayerbotModuleAdapter.h"
#include "Modules/ModuleManager.h"
#include "Log.h"
#include "Config/PlayerbotConfig.h"
#include "PlayerbotModule.h"
#include "Lifecycle/BotSpawner.h"

namespace Playerbot
{

// Static members now inline static in header for DLL compatibility

void PlayerbotModuleAdapter::RegisterWithModuleManager()
{
    if (s_registered)
    {
        TC_LOG_WARN("module.playerbot", "PlayerbotModuleAdapter: Already registered with ModuleManager");
        return;
    }

    TC_LOG_INFO("module.playerbot", "PlayerbotModuleAdapter: Registering with ModuleManager");

    ModuleManager::RegisterModule("Playerbot",
        OnModuleStartup,
        OnModuleUpdate,
        OnModuleShutdown);

    s_registered = true;
    TC_LOG_INFO("module.playerbot", "PlayerbotModuleAdapter: Successfully registered with ModuleManager");
}

void PlayerbotModuleAdapter::OnModuleStartup()
{
    // Check if playerbot is enabled

    bool enabled = sPlayerbotConfig->GetBool("Playerbot.Enable", false);
    if (!enabled)
    {
        TC_LOG_INFO("module.playerbot", "PlayerbotModuleAdapter: Playerbot disabled (Playerbot.Enable = false)");
        return;
    }

    TC_LOG_INFO("module.playerbot", "PlayerbotModuleAdapter: Playerbot enabled - initializing bot spawning systems");

    try
    {
        // Initialize bot systems now that everything is ready
        // This timing ensures the world is fully loaded and module is initialized

        TC_LOG_INFO("module.playerbot", "PlayerbotModuleAdapter: Initializing Bot Spawner...");

        // Initialize Bot Spawner (moved from PlayerbotModule::Initialize for proper timing)
        // NOTE: BotAccountMgr is already initialized in PlayerbotModule::Initialize()
        // which runs earlier during server startup
    if (!Playerbot::sBotSpawner->Initialize())
        {
            TC_LOG_ERROR("module.playerbot", "PlayerbotModuleAdapter: Failed to initialize Bot Spawner");
            return;
        }

        TC_LOG_INFO("module.playerbot", "PlayerbotModuleAdapter: Bot Spawner initialized successfully");

        s_initialized = true;
        TC_LOG_INFO("module.playerbot", "PlayerbotModuleAdapter: Startup completed successfully - bot spawning active");
    }
    catch (::std::exception const& e)
    {
        TC_LOG_ERROR("module.playerbot", "PlayerbotModuleAdapter: Startup failed: {}", e.what());
    }
}

void PlayerbotModuleAdapter::OnModuleUpdate(uint32 diff)
{
    if (!s_initialized)
        return;

    // Check if playerbot is enabled
    if (!sPlayerbotConfig || !sPlayerbotConfig->GetBool("Playerbot.Enable", false))
        return;

    try
    {
        // Delegate to the main PlayerbotModule OnWorldUpdate which handles all the systems
        PlayerbotModule::OnWorldUpdate(diff);
    }
    catch (::std::exception const& e)
    {
        TC_LOG_ERROR("module.playerbot", "PlayerbotModuleAdapter: Update failed: {}", e.what());
    }
}

void PlayerbotModuleAdapter::OnModuleShutdown()
{
    if (!s_initialized)
        return;

    TC_LOG_INFO("module.playerbot", "PlayerbotModuleAdapter: Shutting down Playerbot systems");

    try
    {
        // Delegate to main PlayerbotModule shutdown which handles all systems
        // Note: We don't call PlayerbotModule::Shutdown() here because it's called
        // separately during server shutdown. This is just for ModuleManager cleanup.

        s_initialized = false;
        TC_LOG_INFO("module.playerbot", "PlayerbotModuleAdapter: Shutdown completed");
    }
    catch (::std::exception const& e)
    {
        TC_LOG_ERROR("module.playerbot", "PlayerbotModuleAdapter: Shutdown failed: {}", e.what());
    }
}

} // namespace Playerbot