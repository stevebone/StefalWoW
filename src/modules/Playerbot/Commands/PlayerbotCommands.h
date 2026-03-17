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

#ifndef _PLAYERBOT_COMMANDS_H
#define _PLAYERBOT_COMMANDS_H

#include "Chat.h"
#include "ChatCommand.h"
#include "Language.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include <optional>
#include <string_view>

namespace Playerbot
{
    /**
     * @class PlayerbotCommandScript
     * @brief Admin command script for managing playerbots
     *
     * Provides comprehensive admin commands for:
     * - Bot spawning and deletion
     * - Bot teleportation
     * - Formation management
     * - Statistics and monitoring
     * - Configuration
     *
     * Commands:
     * - .bot spawn <name> [race] [class] - Spawn a new bot
     * - .bot delete <name> - Delete a bot
     * - .bot list - List all active bots
     * - .bot teleport <name> - Teleport bot to you
     * - .bot summon <name> - Summon bot to your location
     * - .bot formation <type> - Set group formation
     * - .bot stats - Show performance statistics
     * - .bot info <name> - Show detailed bot information
     *
     * RBAC Permissions:
     * All commands require RBAC_PERM_COMMAND_PLAYERBOT_* permissions
     *
     * Usage Example:
     * @code
     * .bot spawn Healbot 1 5  // Spawn Human Priest
     * .bot formation wedge    // Set wedge formation
     * .bot stats              // Show performance stats
     * @endcode
     */
    class PlayerbotCommandScript : public CommandScript
    {
    public:
        PlayerbotCommandScript();

        Trinity::ChatCommands::ChatCommandTable GetCommands() const;

    private:
        // Bot spawning commands
        static bool HandleBotSpawnCommand(ChatHandler* handler, ::std::string name,
                                         ::std::optional<uint8> race,
                                         ::std::optional<uint8> classId);
        static bool HandleBotDeleteCommand(ChatHandler* handler, ::std::string name);
        static bool HandleBotListCommand(ChatHandler* handler);

        // Bot teleportation commands
        static bool HandleBotTeleportCommand(ChatHandler* handler, ::std::string name);
        static bool HandleBotSummonCommand(ChatHandler* handler, ::std::string name);
        static bool HandleBotSummonAllCommand(ChatHandler* handler);

        // Formation commands
        static bool HandleBotFormationCommand(ChatHandler* handler, ::std::string formationType);
        static bool HandleBotFormationListCommand(ChatHandler* handler);

        // Statistics and monitoring commands
        static bool HandleBotStatsCommand(ChatHandler* handler);
        static bool HandleBotInfoCommand(ChatHandler* handler, ::std::string name);

        // Configuration commands
        static bool HandleBotConfigCommand(ChatHandler* handler, ::std::string key, ::std::string value);
        static bool HandleBotConfigShowCommand(ChatHandler* handler);
        static bool HandleBotConfigReloadCommand(ChatHandler* handler);

        // Monitoring dashboard commands
        static bool HandleBotMonitorCommand(ChatHandler* handler);
        static bool HandleBotMonitorTrendsCommand(ChatHandler* handler);
        static bool HandleBotAlertsCommand(ChatHandler* handler);
        static bool HandleBotAlertsHistoryCommand(ChatHandler* handler);
        static bool HandleBotAlertsClearCommand(ChatHandler* handler);

        // Diagnostic commands for group member lookup issues
        static bool HandleBotDiagCommand(ChatHandler* handler);
        static bool HandleBotDiagEnableCommand(ChatHandler* handler);
        static bool HandleBotDiagDisableCommand(ChatHandler* handler);
        static bool HandleBotDiagReportCommand(ChatHandler* handler);
        static bool HandleBotDiagResetCommand(ChatHandler* handler);
        static bool HandleBotDiagVerboseCommand(ChatHandler* handler, bool enable);

        // Dungeon autonomy commands (critical safeguard for autonomous navigation)
        static bool HandleBotDungeonCommand(ChatHandler* handler);
        static bool HandleBotDungeonPauseCommand(ChatHandler* handler);
        static bool HandleBotDungeonResumeCommand(ChatHandler* handler);
        static bool HandleBotDungeonStatusCommand(ChatHandler* handler);
        static bool HandleBotDungeonEnableCommand(ChatHandler* handler);
        static bool HandleBotDungeonDisableCommand(ChatHandler* handler);
        static bool HandleBotDungeonAggroCommand(ChatHandler* handler, ::std::string level);

        // Cheat commands
        static bool HandleBotCheatCommand(ChatHandler* handler, ::std::string cheatName);
        static bool HandleBotCheatListCommand(ChatHandler* handler);
        static bool HandleBotCheatOffCommand(ChatHandler* handler);
        static bool HandleBotCheatMultCommand(ChatHandler* handler, ::std::string cheatName, float multiplier);

        // Helper methods
        static Player* FindBotByName(::std::string const& name);
        static bool ValidateRaceClass(uint8 race, uint8 classId, ChatHandler* handler);
        static ::std::string FormatBotList(::std::vector<Player*> const& bots);
        static ::std::string FormatBotStats();
        static ::std::string FormatFormationList();
    };

} // namespace Playerbot

#endif // _PLAYERBOT_COMMANDS_H
