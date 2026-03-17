/*
 * Copyright (C) 2008-2024 TrinityCore <https://www.trinitycore.org/>
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

/**
 * @file InstanceBotScripts.cpp
 * @brief TrinityCore Script-based integration for Instance Bot System
 *
 * This file provides MODULE-ONLY integration with TrinityCore's LFG and
 * Battleground queue systems using the ScriptMgr hook system.
 *
 * NO CORE MODIFICATIONS REQUIRED!
 *
 * Note: Full opcode-based integration will be implemented in a future update.
 * For now, this provides basic player login/logout tracking.
 */

#include "InstanceBotHooks.h"
#include "InstanceBotOrchestrator.h"
#include "ContentRequirements.h"
#include "ScriptMgr.h"
#include "Player.h"
#include "WorldSession.h"
#include "Log.h"

namespace Playerbot
{

// ============================================================================
// INSTANCE BOT PLAYER SCRIPT
// Tracks player login/logout for pool management
// ============================================================================

class InstanceBotPlayerScript : public PlayerScript
{
public:
    InstanceBotPlayerScript() : PlayerScript("InstanceBotPlayerScript") { }

    void OnLogin(Player* player, bool /*firstLogin*/) override
    {
        if (!InstanceBotHooks::IsEnabled() || !player)
            return;

        TC_LOG_DEBUG("playerbots.instance",
            "InstanceBotPlayerScript: Player {} logged in",
            player->GetName());

        // Could track human player count for demand calculation
    }

    void OnLogout(Player* player) override
    {
        if (!InstanceBotHooks::IsEnabled() || !player)
            return;

        TC_LOG_DEBUG("playerbots.instance",
            "InstanceBotPlayerScript: Player {} logging out",
            player->GetName());

        // Cancel any pending requests for this player
        InstanceBotHooks::OnPlayerLeaveLfg(player);
    }
};

// ============================================================================
// SCRIPT REGISTRATION
// ============================================================================

/**
 * @brief Register all Instance Bot scripts with TrinityCore ScriptMgr
 *
 * Called from PlayerbotModule initialization
 */
void RegisterInstanceBotScripts()
{
    TC_LOG_INFO("playerbots.instance", "Registering Instance Bot Scripts...");

    new InstanceBotPlayerScript();

    TC_LOG_INFO("playerbots.instance", "Instance Bot Scripts registered - NO CORE MODIFICATIONS REQUIRED!");
    TC_LOG_INFO("playerbots.instance", "Note: Full LFG/BG queue integration will be added in a future update.");
}

} // namespace Playerbot
