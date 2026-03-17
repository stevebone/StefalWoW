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

#ifndef PLAYERBOT_INSTANCE_BOT_SCRIPTS_H
#define PLAYERBOT_INSTANCE_BOT_SCRIPTS_H

namespace Playerbot
{

/**
 * @brief Register Instance Bot scripts with TrinityCore ScriptMgr
 *
 * This registers:
 * - InstanceBotServerScript: Intercepts LFG/BG queue packets
 * - InstanceBotPlayerScript: Tracks player login/logout/map changes
 *
 * Uses TrinityCore's native hook system - NO CORE MODIFICATIONS REQUIRED!
 *
 * Called from PlayerbotModule::Initialize()
 */
void RegisterInstanceBotScripts();

} // namespace Playerbot

#endif // PLAYERBOT_INSTANCE_BOT_SCRIPTS_H
