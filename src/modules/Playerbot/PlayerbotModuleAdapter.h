/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_MODULE_ADAPTER_H
#define PLAYERBOT_MODULE_ADAPTER_H

#include "Define.h"

namespace Playerbot
{

/**
 * @brief Adapter to register Playerbot with the universal ModuleManager
 *
 * This provides a clean integration point using TrinityCore's ModuleManager
 * instead of relying on ScriptMgr which has inconsistent behavior.
 */
class TC_GAME_API PlayerbotModuleAdapter
{
public:
    /**
     * @brief Register Playerbot module with ModuleManager
     *
     * Call this during Playerbot module initialization to ensure
     * reliable lifecycle event delivery.
     */
    static void RegisterWithModuleManager();

private:
    // Module lifecycle callbacks
    static void OnModuleStartup();
    static void OnModuleUpdate(uint32 diff);
    static void OnModuleShutdown();

    // State tracking
    inline static bool s_registered = false;
    inline static bool s_initialized = false;
};

} // namespace Playerbot

#endif // PLAYERBOT_MODULE_ADAPTER_H