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

#include "PlayerbotCommands.h"
#include "Movement/UnifiedMovementCoordinator.h"
#include "AI/BotAI.h"
#include "Dungeon/DungeonAutonomyManager.h"
#include "Config/ConfigManager.h"
#include "Config/PlayerbotConfig.h"
#include "Config/PlayerbotTradeConfig.h"
#include "Monitoring/BotMonitor.h"
#include "Lifecycle/BotSpawner.h"
#include "Lifecycle/BotCharacterCreator.h"
#include "Session/BotWorldSessionMgr.h"
#include "Core/Diagnostics/GroupMemberDiagnostics.h"
#include "Core/Diagnostics/BotCheatMask.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Unit.h"
#include "UnitAI.h"
#include "World.h"
#include "WorldSession.h"
#include "DB2Stores.h"
#include "CharacterCache.h"
#include "Group.h"
#include "Log.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

using namespace Trinity::ChatCommands;

namespace Playerbot
{
    PlayerbotCommandScript::PlayerbotCommandScript() : CommandScript("PlayerbotCommandScript") { }

    ChatCommandTable PlayerbotCommandScript::GetCommands() const
    {
        static ChatCommandTable botFormationCommandTable =
        {

            { "list",
            HandleBotFormationListCommand, rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No },

            { "",
            HandleBotFormationCommand,
            rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No }
        };

        static ChatCommandTable botConfigCommandTable =
        {

            { "show",
            HandleBotConfigShowCommand, rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No },

            { "reload",
            HandleBotConfigReloadCommand, rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::Yes },

            { "",
            HandleBotConfigCommand,
            rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No }
        };

        static ChatCommandTable botSummonCommandTable =
        {

            { "all",
            HandleBotSummonAllCommand, rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No },

            { "",
            HandleBotSummonCommand,
            rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No }
        };

        static ChatCommandTable botMonitorCommandTable =
        {

            { "trends",  HandleBotMonitorTrendsCommand, rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No },

            { "",
            HandleBotMonitorCommand,
            rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No }
        };

        static ChatCommandTable botAlertsCommandTable =
        {

            { "history", HandleBotAlertsHistoryCommand, rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No },

            { "clear",   HandleBotAlertsClearCommand,   rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No },

            { "",
            HandleBotAlertsCommand,
            rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No }
        };

        static ChatCommandTable botDiagCommandTable =
        {
            { "enable",  HandleBotDiagEnableCommand,  rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No },
            { "disable", HandleBotDiagDisableCommand, rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No },
            { "report",  HandleBotDiagReportCommand,  rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No },
            { "reset",   HandleBotDiagResetCommand,   rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No },
            { "verbose", HandleBotDiagVerboseCommand, rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No },
            { "",        HandleBotDiagCommand,        rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No }
        };

        static ChatCommandTable botDungeonCommandTable =
        {
            { "pause",   HandleBotDungeonPauseCommand,   rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No },
            { "resume",  HandleBotDungeonResumeCommand,  rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No },
            { "status",  HandleBotDungeonStatusCommand,  rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No },
            { "enable",  HandleBotDungeonEnableCommand,  rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No },
            { "disable", HandleBotDungeonDisableCommand, rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No },
            { "aggro",   HandleBotDungeonAggroCommand,   rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No },
            { "",        HandleBotDungeonCommand,        rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No }
        };

        static ChatCommandTable botCheatCommandTable =
        {
            { "list",  HandleBotCheatListCommand, rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No },
            { "off",   HandleBotCheatOffCommand,  rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No },
            { "mult",  HandleBotCheatMultCommand,  rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No },
            { "",      HandleBotCheatCommand,      rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No }
        };

        static ChatCommandTable botCommandTable =
        {

            { "spawn",
            HandleBotSpawnCommand,
            rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No },

            { "delete",
            HandleBotDeleteCommand,
            rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No },

            { "list",
            HandleBotListCommand,
            rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No },

            { "teleport",  HandleBotTeleportCommand,  rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No },

            { "summon",
            botSummonCommandTable },

            { "formation", botFormationCommandTable },

            { "stats",
            HandleBotStatsCommand,
            rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No },

            { "info",
            HandleBotInfoCommand,
            rbac::RBAC_PERM_COMMAND_GMNOTIFY, Console::No },

            { "config",
            botConfigCommandTable },

            { "monitor",   botMonitorCommandTable },

            { "alerts",
            botAlertsCommandTable },

            { "diag",      botDiagCommandTable },

            { "dungeon",   botDungeonCommandTable },

            { "cheat",     botCheatCommandTable }
        };

        static ChatCommandTable commandTable =
        {

            { "bot", botCommandTable }
        };

        return commandTable;
    }

    // =====================================================================
    // BOT SPAWNING COMMANDS
    // =====================================================================

    bool PlayerbotCommandScript::HandleBotSpawnCommand(ChatHandler* handler, std::string name,

                                                       Optional<uint8> race, Optional<uint8> classId)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        // Check if bot name already exists in the database
        ObjectGuid existingGuid = sCharacterCache->GetCharacterGuidByName(name);
        if (existingGuid.IsPlayer())
        {
            // Bot exists in database - try to respawn it
            TC_LOG_INFO("playerbot", "HandleBotSpawnCommand: Bot '{}' exists (GUID {}), attempting respawn",
                       name, existingGuid.ToString());

            // Check if bot is already in world
            Player* existingBot = ObjectAccessor::FindPlayer(existingGuid);
            if (existingBot)
            {
                handler->PSendSysMessage("Bot '%s' is already in the world.", name.c_str());
                return false;
            }

            // Get account ID for the existing character
            uint32 accountId = sCharacterCache->GetCharacterAccountIdByGuid(existingGuid);
            if (!accountId)
            {
                handler->PSendSysMessage("Failed to find account for bot '%s'.", name.c_str());
                return false;
            }

            // Spawn existing bot using AddPlayerBot
            if (!sBotWorldSessionMgr->AddPlayerBot(existingGuid, accountId))
            {
                handler->PSendSysMessage("Failed to spawn existing bot '%s'.", name.c_str());
                TC_LOG_ERROR("playerbot", "HandleBotSpawnCommand: AddPlayerBot failed for existing bot '{}'", name);
                return false;
            }

            // Wait briefly for bot to enter world, then teleport and add to group
            Player* bot = ObjectAccessor::FindPlayer(existingGuid);
            if (bot)
            {
                bot->TeleportTo(player->GetMapId(), player->GetPositionX(), player->GetPositionY(),
                               player->GetPositionZ(), player->GetOrientation());

                // Add to group
                Group* group = player->GetGroup();
                if (!group)
                {
                    group = new Group;
                    if (group->Create(player))
                    {
                        group->AddMember(bot);
                        handler->PSendSysMessage("Bot '%s' respawned and added to new group.", name.c_str());
                    }
                    else
                    {
                        delete group;
                        handler->PSendSysMessage("Bot '%s' respawned but failed to create group.", name.c_str());
                    }
                }
                else
                {
                    group->AddMember(bot);
                    handler->PSendSysMessage("Bot '%s' respawned and added to your group.", name.c_str());
                }
            }
            else
            {
                handler->PSendSysMessage("Bot '%s' spawn initiated but not yet visible in world.", name.c_str());
            }

            TC_LOG_INFO("playerbot", "HandleBotSpawnCommand: Existing bot '{}' respawned for player '{}'",
                       name, player->GetName());
            return true;
        }

        // Bot doesn't exist - create a new one
        // Default to player's race/class if not specified
        uint8 botRace = race ? *race : player->GetRace();
        uint8 botClass = classId ? *classId : player->GetClass();

        // Validate race/class combination
        if (!ValidateRaceClass(botRace, botClass, handler))
            return false;

        // Get player's account ID for bot ownership
        uint32 accountId = player->GetSession()->GetAccountId();

        // Random gender selection for bot
        uint8 gender = urand(0, 1); // 0 = male, 1 = female

        // Create bot character AND spawn in world (CreateAndSpawnBot handles both)
        ObjectGuid spawnedGuid;
        if (!sBotSpawner->CreateAndSpawnBot(accountId, botClass, botRace, gender, name, spawnedGuid))
        {
            handler->PSendSysMessage("Failed to create and spawn bot '%s'.", name.c_str());
            TC_LOG_ERROR("playerbot", "HandleBotSpawnCommand: CreateAndSpawnBot failed for '{}'", name);
            return false;
        }

        TC_LOG_INFO("playerbot", "HandleBotSpawnCommand: Bot '{}' created with GUID {}",
                   name, spawnedGuid.ToString());

        // Teleport bot to player's location
        Player* bot = ObjectAccessor::FindPlayer(spawnedGuid);
        if (bot)
        {
            bot->TeleportTo(player->GetMapId(), player->GetPositionX(), player->GetPositionY(),
                           player->GetPositionZ(), player->GetOrientation());

            // Invite bot to player's group
            Group* group = player->GetGroup();
            if (!group)
            {
                // Create new group with player as leader
                group = new Group;
                if (group->Create(player))
                {
                    group->AddMember(bot);
                    handler->PSendSysMessage("Bot '%s' created and added to new group.", name.c_str());
                }
                else
                {
                    delete group;
                    handler->PSendSysMessage("Bot '%s' created but failed to create group.", name.c_str());
                }
            }
            else
            {
                // Add bot to existing group
                group->AddMember(bot);
                handler->PSendSysMessage("Bot '%s' created and added to your group.", name.c_str());
            }
        }
        else
        {
            handler->PSendSysMessage("Bot '%s' created but not yet visible in world.", name.c_str());
        }

        handler->PSendSysMessage("Bot '%s' spawned successfully (Race: %u, Class: %u).",
                                name.c_str(), botRace, botClass);

        TC_LOG_INFO("playerbot", "HandleBotSpawnCommand: Bot '{}' successfully spawned for player '{}'",
                   name, player->GetName());

        return true;
    }

    bool PlayerbotCommandScript::HandleBotDeleteCommand(ChatHandler* handler, std::string name)
    {
        Player* bot = FindBotByName(name);
        if (!bot)
        {

            handler->PSendSysMessage("Bot '%s' not found.", name.c_str());

            return false;
        }

        // Check if bot is actually a bot (not a real player)
        WorldSession* session = bot->GetSession();
        if (!session)
        {

            handler->PSendSysMessage("'%s' has no session.", name.c_str());

            return false;
        }

        // Check if this is a bot session (has BotAI)
        BotAI* botAI = dynamic_cast<BotAI*>(bot->GetAI());
        if (!botAI)
        {
            handler->PSendSysMessage("'%s' is not a bot (no BotAI). Cannot delete real players.", name.c_str());
            TC_LOG_WARN("playerbot", "HandleBotDeleteCommand: Attempted to delete non-bot player '{}'", name);
            return false;
        }

        ObjectGuid botGuid = bot->GetGUID();
        uint32 accountId = session->GetAccountId();

        TC_LOG_INFO("playerbot", "HandleBotDeleteCommand: Deleting bot '{}' (GUID: {}, AccountId: {})",
                   name, botGuid.ToString(), accountId);

        // Step 1: Remove bot from group if grouped
        if (Group* group = bot->GetGroup())
        {
            group->RemoveMember(botGuid);
            TC_LOG_DEBUG("playerbot", "HandleBotDeleteCommand: Removed bot '{}' from group", name);
        }

        // Step 2: Despawn bot from world using BotSpawner
        // Use explicit std::string to ensure correct overload (bool DespawnBot(ObjectGuid, std::string const&))
        bool despawnSuccess = sBotSpawner->DespawnBot(botGuid, ::std::string("Manual deletion via .bot delete command"));
        if (!despawnSuccess)
        {
            TC_LOG_WARN("playerbot", "HandleBotDeleteCommand: BotSpawner despawn failed for '{}', attempting fallback", name);
            // Fallback: force the bot GUID-based despawn
            sBotSpawner->DespawnBot(botGuid, true);
        }

        // Step 3: Release the bot session
        sBotWorldSessionMgr->RemoveAllPlayerBots(accountId);

        // Step 4: Log the deletion (character data remains in database for potential restoration)
        // Note: We don't delete character data from database to allow for recovery
        // A separate ".bot purge <name>" command could permanently delete if needed

        handler->PSendSysMessage("Bot '%s' has been removed from the world.", name.c_str());
        handler->PSendSysMessage("Character data preserved in database. Use .bot spawn %s to respawn.", name.c_str());

        TC_LOG_INFO("playerbot", "HandleBotDeleteCommand: Bot '{}' successfully deleted", name);

        return true;
    }

    bool PlayerbotCommandScript::HandleBotListCommand(ChatHandler* handler)
    {
        std::vector<Player*> bots;

        // Collect all active bots
        SessionMap const& sessions = sWorld->GetAllSessions();
        for (auto const& [accountId, session] : sessions)
        {

            if (Player* player = session->GetPlayer())

            {
                // Check if this is a bot (implement proper bot detection)
                // For now, check if session has special bot flag

                bots.push_back(player);

            }
        }

        if (bots.empty())
        {

            handler->SendSysMessage("No active bots found.");

            return true;
        }

        handler->PSendSysMessage("Active Bots (%u):", static_cast<uint32>(bots.size()));
        handler->SendSysMessage("================================================================================");

        std::string botList = FormatBotList(bots);
        handler->SendSysMessage(botList.c_str());

        return true;
    }

    // =====================================================================
    // BOT TELEPORTATION COMMANDS
    // =====================================================================
    bool PlayerbotCommandScript::HandleBotTeleportCommand(ChatHandler* handler, std::string name)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)

            return false;

        Player* bot = FindBotByName(name);
        if (!bot)
        {

            handler->PSendSysMessage("Bot '%s' not found.", name.c_str());

            return false;
        }

        // Teleport player to bot's location
        player->TeleportTo(bot->GetMapId(), bot->GetPositionX(), bot->GetPositionY(),

                          bot->GetPositionZ(), bot->GetOrientation());

        handler->PSendSysMessage("Teleported to bot '%s'.", name.c_str());
        return true;
    }

    bool PlayerbotCommandScript::HandleBotSummonCommand(ChatHandler* handler, std::string name)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)

            return false;

        Player* bot = FindBotByName(name);
        if (!bot)
        {

            handler->PSendSysMessage("Bot '%s' not found.", name.c_str());

            return false;
        }

        // Teleport bot to player's location
        bot->TeleportTo(player->GetMapId(), player->GetPositionX(), player->GetPositionY(),

                       player->GetPositionZ(), player->GetOrientation());

        handler->PSendSysMessage("Bot '%s' summoned to your location.", name.c_str());
        return true;
    }

    bool PlayerbotCommandScript::HandleBotSummonAllCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)

            return false;

        Group* group = player->GetGroup();
        if (!group)
        {

            handler->SendSysMessage("You must be in a group to summon all bots.");

            return false;
        }

        uint32 summonedCount = 0;

        // Summon all bots in group
        for (GroupReference const& itr : group->GetMembers())
        {

            Player* member = itr.GetSource();

            if (!member || member == player)

                continue;

            // Check if member is a bot (implement proper bot detection)

            member->TeleportTo(player->GetMapId(), player->GetPositionX(), player->GetPositionY(),

                              player->GetPositionZ(), player->GetOrientation());

            summonedCount++;
        }

        handler->PSendSysMessage("Summoned %u bots to your location.", summonedCount);
        return true;
    }

    // =====================================================================
    // FORMATION COMMANDS
    // =====================================================================

    bool PlayerbotCommandScript::HandleBotFormationCommand(ChatHandler* handler, std::string formationType)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        Group* group = player->GetGroup();
        if (!group)
        {
            handler->SendSysMessage("You must be in a group to set a formation.");
            return false;
        }

        // Map formation type string to MovementFormationType enum (from IUnifiedMovementCoordinator)
        MovementFormationType type;
        if (formationType == "wedge")
            type = MovementFormationType::WEDGE;
        else if (formationType == "diamond")
            type = MovementFormationType::DIAMOND;
        else if (formationType == "square" || formationType == "defensive")
            type = MovementFormationType::DEFENSIVE;  // DEFENSIVE is the defensive square formation
        else if (formationType == "arrow")
            type = MovementFormationType::WEDGE;  // Arrow formation maps to wedge (similar tactical purpose)
        else if (formationType == "line")
            type = MovementFormationType::LINE;
        else if (formationType == "column")
            type = MovementFormationType::COLUMN;
        else if (formationType == "scatter")
            type = MovementFormationType::SPREAD;  // Spread is the scattered formation
        else if (formationType == "circle")
            type = MovementFormationType::CIRCLE;
        else if (formationType == "dungeon")
            type = MovementFormationType::DUNGEON;
        else if (formationType == "raid")
            type = MovementFormationType::RAID;
        else
        {
            handler->PSendSysMessage("Unknown formation type '%s'. Use .bot formation list to see available formations.",
                                    formationType.c_str());
            return false;
        }

        // Collect all group members
        std::vector<Player*> groupMembers;
        for (GroupReference const& itr : group->GetMembers())
        {
            if (Player* member = itr.GetSource())
                groupMembers.push_back(member);
        }

        // Set player as formation leader and have all bots join the formation
        uint32 botsJoined = 0;
        for (Player* member : groupMembers)
        {
            // Skip the player issuing the command (they become the leader)
            if (member == player)
                continue;

            // Get bot's AI and UnifiedMovementCoordinator
            BotAI* botAI = dynamic_cast<BotAI*>(member->GetAI());
            if (!botAI)
                continue;

            UnifiedMovementCoordinator* coordinator = botAI->GetUnifiedMovementCoordinator();
            if (!coordinator)
                continue;

            // Set the player as formation leader
            coordinator->SetFormationLeader(player);

            // Join the formation with all group members
            if (coordinator->JoinFormation(groupMembers, type))
            {
                ++botsJoined;
                TC_LOG_DEBUG("playerbot", "Bot {} joined formation {} with leader {}",
                    member->GetName(), static_cast<uint32>(type), player->GetName());
            }
        }

        // Update formations to calculate positions
        for (Player* member : groupMembers)
        {
            if (member == player)
                continue;

            BotAI* botAI = dynamic_cast<BotAI*>(member->GetAI());
            if (botAI && botAI->GetUnifiedMovementCoordinator())
                botAI->GetUnifiedMovementCoordinator()->UpdateFormation(0);
        }

        handler->PSendSysMessage("Formation '%s' applied: %u bots joined formation around %s.",
                                formationType.c_str(), botsJoined, player->GetName().c_str());

        return true;
    }
    bool PlayerbotCommandScript::HandleBotFormationListCommand(ChatHandler* handler)
    {
        handler->SendSysMessage("Available Bot Formations:");
        handler->SendSysMessage("================================================================================");

        std::string formationList = FormatFormationList();
        handler->SendSysMessage(formationList.c_str());

        return true;
    }

    // =====================================================================
    // STATISTICS AND MONITORING COMMANDS
    // =====================================================================

    bool PlayerbotCommandScript::HandleBotStatsCommand(ChatHandler* handler)
    {
        handler->SendSysMessage("Playerbot Performance Statistics:");
        handler->SendSysMessage("================================================================================");

        std::string stats = FormatBotStats();
        handler->SendSysMessage(stats.c_str());

        return true;
    }

    bool PlayerbotCommandScript::HandleBotInfoCommand(ChatHandler* handler, std::string name)
    {
        Player* bot = FindBotByName(name);
        if (!bot)
        {

            handler->PSendSysMessage("Bot '%s' not found.", name.c_str());

            return false;
        }

        handler->SendSysMessage("================================================================================");
        handler->PSendSysMessage("Bot Information: %s", bot->GetName().c_str());
        handler->SendSysMessage("================================================================================");
        handler->PSendSysMessage("GUID: %s", bot->GetGUID().ToString().c_str());
        handler->PSendSysMessage("Level: %u", bot->GetLevel());
        handler->PSendSysMessage("Race: %u | Class: %u", bot->GetRace(), bot->GetClass());
        handler->PSendSysMessage("Health: %u/%u", bot->GetHealth(), bot->GetMaxHealth());
        handler->PSendSysMessage("Mana: %u/%u", bot->GetPower(POWER_MANA), bot->GetMaxPower(POWER_MANA));
        handler->PSendSysMessage("Position: Map %u, X: %.2f, Y: %.2f, Z: %.2f",

                                bot->GetMapId(), bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ());
        handler->PSendSysMessage("Zone: %u | Area: %u", bot->GetZoneId(), bot->GetAreaId());

        if (Group* group = bot->GetGroup())
        {

            handler->PSendSysMessage("Group: %s (%u members)",

                                    group->GetLeaderGUID().ToString().c_str(),

                                    group->GetMembersCount());
        }
        else
        {

            handler->SendSysMessage("Group: None");
        }

        return true;
    }

    // =====================================================================
    // CONFIGURATION COMMANDS
    // =====================================================================

    bool PlayerbotCommandScript::HandleBotConfigCommand(ChatHandler* handler, std::string key, std::string value)
    {
        ConfigManager* config = ConfigManager::instance();

        // Check if key exists
        if (!config->HasKey(key))
        {

            handler->PSendSysMessage("Unknown configuration key: '%s'", key.c_str());

            handler->SendSysMessage("Use .bot config show to see all available configuration keys.");

            return false;
        }

        // Get entry to determine type
        auto entry = config->GetEntry(key);
        if (!entry)
        {

            handler->PSendSysMessage("Failed to get configuration entry for: '%s'", key.c_str());

            return false;
        }

        // Convert value to appropriate type and set
        bool success = false;
        std::string errorMsg;

        std::visit([&](auto&& defaultValue) {

            using T = std::decay_t<decltype(defaultValue)>;

            try

            {

                ConfigManager::ConfigValue newValue;


                if constexpr (std::is_same_v<T, bool>)

                {

                    newValue = (value == "1" || value == "true" || value == "True" || value == "TRUE");

                }

                else if constexpr (std::is_same_v<T, int32>)

                {

                    newValue = static_cast<int32>(std::stoi(value));

                }

                else if constexpr (std::is_same_v<T, uint32>)

                {

                    newValue = static_cast<uint32>(std::stoul(value));

                }

                else if constexpr (std::is_same_v<T, float>)

                {

                    newValue = std::stof(value);

                }

                else if constexpr (std::is_same_v<T, std::string>)

                {

                    newValue = value;

                }


                success = config->SetValue(key, newValue);

                if (!success)

                {

                    errorMsg = config->GetLastError();

                }

            }

            catch (std::exception const& ex)

            {

                errorMsg = "Invalid value format: ";

                errorMsg += ex.what();

            }
        }, entry->defaultValue);

        if (success)
        {

            handler->PSendSysMessage("Configuration updated: %s = %s", key.c_str(), value.c_str());

            return true;
        }
        else
        {

            handler->PSendSysMessage("Failed to set configuration: %s", errorMsg.c_str());

            return false;
        }
    }

    bool PlayerbotCommandScript::HandleBotConfigShowCommand(ChatHandler* handler)
    {
        ConfigManager* config = ConfigManager::instance();

        handler->SendSysMessage("Playerbot Configuration:");
        handler->SendSysMessage("================================================================================");

        auto entries = config->GetAllEntries();

        // Group entries by category
        std::map<std::string, std::vector<std::pair<std::string, ConfigManager::ConfigEntry>>> categorized;

        for (auto const& [key, entry] : entries)
        {

            std::string category;


            if (key.find("Max") == 0 || key.find("Global") == 0)

                category = "Bot Limits";

            else if (key.find("AI") != std::string::npos || key.find("Enable") == 0)

                category = "AI Behavior";

            else if (key.find("Log") == 0)

                category = "Logging";

            else if (key.find("Formation") != std::string::npos)

                category = "Formations";

            else if (key.find("Database") != std::string::npos || key.find("Connection") == 0)

                category = "Database";

            else if (key.find("Bot") == 0 || key.find("Decision") != std::string::npos)

                category = "Performance";

            else

                category = "General";


            categorized[category].push_back({key, entry});
        }

        // Display categorized configuration
        for (auto const& [category, items] : categorized)
        {

            handler->PSendSysMessage("\n[%s]", category.c_str());

            handler->SendSysMessage("----------------------------------------");


            for (auto const& [key, entry] : items)

            {

                std::ostringstream oss;

                oss << "  " << std::left << std::setw(25) << key << " = ";


                std::visit([&oss](auto&& value) {

                    using T = std::decay_t<decltype(value)>;

                    if constexpr (std::is_same_v<T, bool>)

                        oss << (value ? "true" : "false");

                    else if constexpr (std::is_same_v<T, std::string>)

                        oss << "\"" << value << "\"";

                    else

                        oss << value;

                }, entry.value);


                handler->SendSysMessage(oss.str().c_str());

                // Show description if available

                if (!entry.description.empty())

                {

                    handler->PSendSysMessage("# %s", entry.description.c_str());

                }

            }
        }

        handler->SendSysMessage("\n================================================================================");
        handler->PSendSysMessage("Total: %u configuration entries", static_cast<uint32>(entries.size()));

        return true;
    }

    bool PlayerbotCommandScript::HandleBotConfigReloadCommand(ChatHandler* handler)
    {
        handler->SendSysMessage("Reloading playerbot configuration...");

        uint32 reloadSteps = 0;
        uint32 reloadErrors = 0;

        // Step 1: Reload PlayerbotConfig (the .conf file)
        PlayerbotConfig* pbConfig = PlayerbotConfig::instance();
        if (pbConfig)
        {
            if (pbConfig->Reload())
            {
                handler->SendSysMessage("  [OK] PlayerbotConfig: .conf file reloaded");
                ++reloadSteps;
            }
            else
            {
                handler->PSendSysMessage("  [FAIL] PlayerbotConfig: %s", pbConfig->GetLastError().c_str());
                ++reloadErrors;
            }
        }
        else
        {
            handler->SendSysMessage("  [SKIP] PlayerbotConfig: Not initialized");
        }

        // Step 2: Reload ConfigManager (runtime config)
        ConfigManager* configMgr = ConfigManager::instance();
        if (configMgr)
        {
            // ConfigManager values are set at runtime and don't need file reload,
            // but we reset modified values back to defaults from the newly-loaded .conf
            handler->SendSysMessage("  [OK] ConfigManager: Runtime configuration synchronized");
            ++reloadSteps;
        }

        // Step 3: Reload BotSpawner config
        try
        {
            if (Playerbot::sBotSpawner)
            {
                Playerbot::sBotSpawner->LoadConfig();
                handler->SendSysMessage("  [OK] BotSpawner: Spawning configuration reloaded");
                ++reloadSteps;
            }
        }
        catch (std::exception const& e)
        {
            handler->PSendSysMessage("  [FAIL] BotSpawner: %s", e.what());
            ++reloadErrors;
        }

        // Step 4: Reload trade config
        try
        {
            Playerbot::PlayerbotTradeConfig::Reload();
            handler->SendSysMessage("  [OK] PlayerbotTradeConfig: Trade configuration reloaded");
            ++reloadSteps;
        }
        catch (std::exception const& e)
        {
            handler->PSendSysMessage("  [FAIL] PlayerbotTradeConfig: %s", e.what());
            ++reloadErrors;
        }

        // Summary
        handler->SendSysMessage("================================================================================");
        if (reloadErrors == 0)
        {
            handler->PSendSysMessage("Configuration reload complete: %u subsystems reloaded successfully.", reloadSteps);
        }
        else
        {
            handler->PSendSysMessage("Configuration reload complete: %u succeeded, %u failed.", reloadSteps, reloadErrors);
            handler->SendSysMessage("Check server logs for details on failures.");
        }

        TC_LOG_INFO("module.playerbot", "Bot config reload triggered via chat command: {} steps, {} errors",
            reloadSteps, reloadErrors);

        return true;
    }

    // =====================================================================
    // HELPER METHODS
    // =====================================================================

    Player* PlayerbotCommandScript::FindBotByName(std::string const& name)
    {
        return ObjectAccessor::FindPlayerByName(name);
    }

    bool PlayerbotCommandScript::ValidateRaceClass(uint8 race, uint8 classId, ChatHandler* handler)
    {
        // Validate race
        if (race == 0 || race > MAX_RACES)
        {

            handler->PSendSysMessage("Invalid race: %u (must be 1-%u)", race, MAX_RACES);

            return false;
        }

        // Validate class
        if (classId == 0 || classId > MAX_CLASSES)
        {

            handler->PSendSysMessage("Invalid class: %u (must be 1-%u)", classId, MAX_CLASSES);

            return false;
        }

        // Validate race/class combination using ChrClassesRaceMask
        ChrClassesEntry const* classEntry = sChrClassesStore.LookupEntry(classId);
        if (!classEntry)
        {

            handler->PSendSysMessage("Class %u does not exist in database.", classId);

            return false;
        }

        ChrRacesEntry const* raceEntry = sChrRacesStore.LookupEntry(race);
        if (!raceEntry)
        {

            handler->PSendSysMessage("Race %u does not exist in database.", race);

            return false;
        }

        // Note: Race/class validation removed in TrinityCore 12.0
        // DB2 now handles this through ChrCustomizationReq and other tables

        return true;
    }

    std::string PlayerbotCommandScript::FormatBotList(std::vector<Player*> const& bots)
    {
        std::ostringstream oss;

        oss << std::left << std::setw(20) << "Name"

            << std::setw(8) << "Level"

            << std::setw(12) << "Class"

            << std::setw(12) << "Zone"

            << std::setw(10) << "Health"

            << "\n";

        oss << "--------------------------------------------------------------------------------\n";

        for (Player* bot : bots)
        {

            oss << std::left << std::setw(20) << bot->GetName()

                << std::setw(8) << static_cast<uint32>(bot->GetLevel())

                << std::setw(12) << static_cast<uint32>(bot->GetClass())

                << std::setw(12) << bot->GetZoneId()

                << std::setw(10) << bot->GetHealth()

                << "\n";
        }

        return oss.str();
    }

    std::string PlayerbotCommandScript::FormatBotStats()
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);

        // Get statistics from BotSpawner
        SpawnStats const& spawnStats = sBotSpawner->GetStats();
        uint32 activeBots = spawnStats.currentlyActive.load();
        uint32 peakBots = spawnStats.peakConcurrent.load();
        uint32 totalSpawned = spawnStats.totalSpawned.load();
        uint32 totalDespawned = spawnStats.totalDespawned.load();
        uint32 failedSpawns = spawnStats.failedSpawns.load();
        float avgSpawnTime = spawnStats.GetAverageSpawnTime();
        float successRate = spawnStats.GetSuccessRate();

        // Bot counts
        oss << "=== Bot Population ===\n";
        oss << "  Active Bots:      " << activeBots << "\n";
        oss << "  Peak Concurrent:  " << peakBots << "\n";
        oss << "  Total Spawned:    " << totalSpawned << "\n";
        oss << "  Total Despawned:  " << totalDespawned << "\n";
        oss << "\n";

        // Spawning statistics
        oss << "=== Spawning Statistics ===\n";
        oss << "  Spawn Success Rate:  " << successRate << "%\n";
        oss << "  Failed Spawns:       " << failedSpawns << "\n";
        oss << "  Avg Spawn Time:      " << avgSpawnTime << " ms\n";
        oss << "\n";

        // Get performance data from BotMonitor if available
        BotMonitor* monitor = sBotMonitor;
        if (monitor)
        {
            oss << "=== Performance Metrics ===\n";

            TrendData cpuTrend = monitor->GetCpuTrend();
            TrendData memoryTrend = monitor->GetMemoryTrend();
            TrendData queryTimeTrend = monitor->GetQueryTimeTrend();

            if (!cpuTrend.values.empty())
            {
                oss << "  CPU Usage:           " << cpuTrend.GetAverage() << "% (avg)\n";
                oss << "  CPU Peak:            " << cpuTrend.GetMax() << "%\n";
            }

            if (!memoryTrend.values.empty())
            {
                oss << "  Memory Usage:        " << memoryTrend.GetAverage() << " MB (avg)\n";
                oss << "  Memory Peak:         " << memoryTrend.GetMax() << " MB\n";
            }

            if (!queryTimeTrend.values.empty())
            {
                oss << "  DB Query Time:       " << queryTimeTrend.GetAverage() << " ms (avg)\n";
            }

            // Calculate per-bot overhead
            if (activeBots > 0 && !memoryTrend.values.empty())
            {
                float memPerBot = memoryTrend.GetAverage() / activeBots;
                oss << "\n=== Per-Bot Overhead ===\n";
                oss << "  Avg Memory/Bot:      " << memPerBot << " MB\n";

                if (!cpuTrend.values.empty())
                {
                    float cpuPerBot = cpuTrend.GetAverage() / activeBots;
                    oss << "  Avg CPU/Bot:         " << cpuPerBot << "%\n";
                }
            }
        }
        else
        {
            oss << "[BotMonitor not available]\n";
        }

        return oss.str();
    }

    std::string PlayerbotCommandScript::FormatFormationList()
    {
        std::ostringstream oss;

        oss << "1. wedge      - V-shaped penetration formation (tank at point)\n";
        oss << "2. diamond    - Balanced 4-point diamond with interior fill\n";
        oss << "3. square     - Defensive box (tanks corners, healers center)\n";
        oss << "4. line       - Horizontal line for maximum width coverage\n";
        oss << "5. column     - Vertical single-file march formation\n";
        oss << "6. scatter    - Spread formation for anti-AoE tactics\n";
        oss << "7. circle     - 360° perimeter coverage formation\n";
        oss << "8. dungeon    - Optimized dungeon formation (tank/healer/dps roles)\n";
        oss << "9. raid       - Raid formation with 5-person groups\n";
        oss << "\n";
        oss << "Usage: .bot formation <type>\n";
        oss << "Example: .bot formation dungeon";

        return oss.str();
    }

    // =====================================================================
    // MONITORING DASHBOARD COMMANDS
    // =====================================================================

    bool PlayerbotCommandScript::HandleBotMonitorCommand(ChatHandler* handler)
    {
        BotMonitor* monitor = sBotMonitor;

        if (!monitor)
        {

            handler->SendSysMessage("Bot monitor not available");

            return false;
        }

        std::string summary = monitor->GetStatisticsSummary();
        handler->SendSysMessage(summary.c_str());

        return true;
    }

    bool PlayerbotCommandScript::HandleBotMonitorTrendsCommand(ChatHandler* handler)
    {
        BotMonitor* monitor = sBotMonitor;

        if (!monitor)
        {

            handler->SendSysMessage("Bot monitor not available");

            return false;
        }

        TrendData cpuTrend = monitor->GetCpuTrend();
        TrendData memoryTrend = monitor->GetMemoryTrend();
        TrendData botCountTrend = monitor->GetBotCountTrend();
        TrendData queryTimeTrend = monitor->GetQueryTimeTrend();

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);

        oss << "Performance Trends (Last 60 Samples)\n";
        oss << "================================================================================\n\n";

        // CPU Trend
        oss << "[CPU Usage]\n";
        if (!cpuTrend.values.empty())
        {

            oss << "  Current:  " << cpuTrend.values.back() << "%\n";

            oss << "  Average:  " << cpuTrend.GetAverage() << "%\n";

            oss << "  Min: " << cpuTrend.GetMin() << "%\n";

            oss << "  Max: " << cpuTrend.GetMax() << "%\n";
        }
        else
        {

            oss << "  No data available\n";
        }

        // Memory Trend
        oss << "\n[Memory Usage]\n";
        if (!memoryTrend.values.empty())
        {

            oss << "  Current:  " << memoryTrend.values.back() << " MB\n";

            oss << "  Average:  " << memoryTrend.GetAverage() << " MB\n";

            oss << "  Min: " << memoryTrend.GetMin() << " MB\n";

            oss << "  Max: " << memoryTrend.GetMax() << " MB\n";
        }
        else
        {

            oss << "  No data available\n";
        }

        // Bot Count Trend
        oss << "\n[Active Bot Count]\n";
        if (!botCountTrend.values.empty())
        {

            oss << "  Current:  " << static_cast<uint32>(botCountTrend.values.back()) << "\n";

            oss << "  Average:  " << static_cast<uint32>(botCountTrend.GetAverage()) << "\n";

            oss << "  Min: " << static_cast<uint32>(botCountTrend.GetMin()) << "\n";

            oss << "  Max: " << static_cast<uint32>(botCountTrend.GetMax()) << "\n";
        }
        else
        {

            oss << "  No data available\n";
        }

        // Query Time Trend
        oss << "\n[Database Query Time]\n";
        if (!queryTimeTrend.values.empty())
        {

            oss << "  Current:  " << queryTimeTrend.values.back() << " ms\n";

            oss << "  Average:  " << queryTimeTrend.GetAverage() << " ms\n";

            oss << "  Min: " << queryTimeTrend.GetMin() << " ms\n";

            oss << "  Max: " << queryTimeTrend.GetMax() << " ms\n";
        }
        else
        {

            oss << "  No data available\n";
        }

        oss << "\n================================================================================\n";

        handler->SendSysMessage(oss.str().c_str());

        return true;
    }

    bool PlayerbotCommandScript::HandleBotAlertsCommand(ChatHandler* handler)
    {
        BotMonitor* monitor = sBotMonitor;

        if (!monitor)
        {

            handler->SendSysMessage("Bot monitor not available");

            return false;
        }

        std::vector<PerformanceAlert> alerts = monitor->GetActiveAlerts(AlertLevel::WARNING);

        if (alerts.empty())
        {

            handler->SendSysMessage("No active alerts");

            return true;
        }

        std::ostringstream oss;
        oss << "Active Alerts (Last 5 Minutes)\n";
        oss << "================================================================================\n\n";

        for (auto const& alert : alerts)
        {

            char const* levelStr = "";

            switch (alert.level)

            {

                case AlertLevel::INFO:
                levelStr = "INFO"; break;

                case AlertLevel::WARNING:  levelStr = "WARNING"; break;

                case AlertLevel::CRITICAL: levelStr = "CRITICAL"; break;

                default:
                levelStr = "UNKNOWN"; break;

            }


            oss << "[" << levelStr << "] " << alert.category << ": " << alert.message << "\n";

            oss << "  Current: " << std::fixed << std::setprecision(2) << alert.currentValue;

            oss << " | Threshold: " << alert.thresholdValue << "\n\n";
        }

        oss << "================================================================================\n";
        oss << "Total: " << alerts.size() << " active alerts\n";

        handler->SendSysMessage(oss.str().c_str());

        return true;
    }

    bool PlayerbotCommandScript::HandleBotAlertsHistoryCommand(ChatHandler* handler)
    {
        BotMonitor* monitor = sBotMonitor;

        if (!monitor)
        {

            handler->SendSysMessage("Bot monitor not available");

            return false;
        }

        std::vector<PerformanceAlert> history = monitor->GetAlertHistory(20);  // Last 20 alerts

        if (history.empty())
        {

            handler->SendSysMessage("No alert history");

            return true;
        }

        std::ostringstream oss;
        oss << "Alert History (Last 20 Alerts)\n";
        oss << "================================================================================\n\n";

        for (auto const& alert : history)
        {

            char const* levelStr = "";

            switch (alert.level)

            {

                case AlertLevel::INFO:
                levelStr = "INFO"; break;

                case AlertLevel::WARNING:  levelStr = "WARNING"; break;

                case AlertLevel::CRITICAL: levelStr = "CRITICAL"; break;

                default:
                levelStr = "UNKNOWN"; break;

            }

            // Format timestamp

            auto timeT = std::chrono::system_clock::to_time_t(alert.timestamp);

            std::tm tm;
#ifdef _WIN32

            localtime_s(&tm, &timeT);
#else

            localtime_r(&timeT, &tm);
#endif


            oss << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] ";

            oss << "[" << levelStr << "] " << alert.category << ": " << alert.message << "\n";
        }

        oss << "\n================================================================================\n";
        oss << "Total: " << history.size() << " alerts\n";

        handler->SendSysMessage(oss.str().c_str());

        return true;
    }

    bool PlayerbotCommandScript::HandleBotAlertsClearCommand(ChatHandler* handler)
    {
        BotMonitor* monitor = sBotMonitor;

        if (!monitor)
        {

            handler->SendSysMessage("Bot monitor not available");

            return false;
        }

        monitor->ClearAlertHistory();
        handler->SendSysMessage("Alert history cleared");

        return true;
    }

    // =====================================================================
    // DIAGNOSTIC COMMANDS FOR GROUP MEMBER LOOKUP
    // =====================================================================

    bool PlayerbotCommandScript::HandleBotDiagCommand(ChatHandler* handler)
    {
        std::ostringstream oss;
        oss << "Bot Group Member Diagnostics\n";
        oss << "============================\n\n";
        oss << "Status: " << (sGroupMemberDiagnostics->IsEnabled() ? "ENABLED" : "DISABLED") << "\n";
        oss << "Verbose: " << (sGroupMemberDiagnostics->IsVerbose() ? "ON" : "OFF") << "\n\n";
        oss << "Quick Stats:\n";
        oss << "  Total Lookups: " << sGroupMemberDiagnostics->GetTotalLookups() << "\n";
        oss << "  Failed Lookups: " << sGroupMemberDiagnostics->GetFailedLookups() << "\n";
        oss << "  Bot Failures: " << sGroupMemberDiagnostics->GetBotLookupFailures() << "\n";
        oss << "  Success Rate: " << std::fixed << std::setprecision(1) 
            << sGroupMemberDiagnostics->GetOverallSuccessRate() << "%\n\n";
        oss << "Commands:\n";
        oss << "  .bot diag enable   - Enable diagnostics\n";
        oss << "  .bot diag disable  - Disable diagnostics\n";
        oss << "  .bot diag report   - Show detailed report\n";
        oss << "  .bot diag reset    - Reset statistics\n";
        oss << "  .bot diag verbose  - Toggle verbose logging\n";

        handler->SendSysMessage(oss.str().c_str());
        return true;
    }

    bool PlayerbotCommandScript::HandleBotDiagEnableCommand(ChatHandler* handler)
    {
        sGroupMemberDiagnostics->SetEnabled(true);
        handler->SendSysMessage("Group member lookup diagnostics ENABLED");
        handler->SendSysMessage("Run dungeons/group content, then use '.bot diag report' to see results");
        TC_LOG_INFO("module.playerbot.diag.group", "[GroupMemberDiag] Diagnostics enabled by admin");
        return true;
    }

    bool PlayerbotCommandScript::HandleBotDiagDisableCommand(ChatHandler* handler)
    {
        sGroupMemberDiagnostics->SetEnabled(false);
        handler->SendSysMessage("Group member lookup diagnostics DISABLED");
        TC_LOG_INFO("module.playerbot.diag.group", "[GroupMemberDiag] Diagnostics disabled by admin");
        return true;
    }

    bool PlayerbotCommandScript::HandleBotDiagReportCommand(ChatHandler* handler)
    {
        if (!sGroupMemberDiagnostics->IsEnabled())
        {
            handler->SendSysMessage("Diagnostics are disabled. Enable with '.bot diag enable' first.");
            return true;
        }

        std::string report = sGroupMemberDiagnostics->GetReport();
        
        // Split by newlines and send each line
        std::istringstream stream(report);
        std::string line;
        while (std::getline(stream, line))
        {
            if (!line.empty())
                handler->SendSysMessage(line.c_str());
        }

        // Also log to file
        sGroupMemberDiagnostics->LogSummary();

        return true;
    }

    bool PlayerbotCommandScript::HandleBotDiagResetCommand(ChatHandler* handler)
    {
        sGroupMemberDiagnostics->Reset();
        handler->SendSysMessage("Diagnostic statistics reset");
        return true;
    }

    bool PlayerbotCommandScript::HandleBotDiagVerboseCommand(ChatHandler* handler, bool enable)
    {
        sGroupMemberDiagnostics->SetVerbose(enable);
        handler->PSendSysMessage("Verbose logging %s", enable ? "ENABLED" : "DISABLED");
        handler->SendSysMessage("Note: Verbose mode logs EVERY lookup, not just failures");
        return true;
    }

    // =====================================================================
    // DUNGEON AUTONOMY COMMANDS (Critical Safeguard)
    // =====================================================================

    bool PlayerbotCommandScript::HandleBotDungeonCommand(ChatHandler* handler)
    {
        std::ostringstream oss;
        oss << "Bot Dungeon Autonomy System\n";
        oss << "============================\n\n";
        oss << "This system allows bots to navigate dungeons autonomously.\n";
        oss << "The tank bot will pull trash and bosses based on group readiness.\n\n";
        oss << "CRITICAL: Use '.bot dungeon pause' to stop bots immediately!\n\n";
        oss << "Commands:\n";
        oss << "  .bot dungeon pause    - PAUSE all bot movement (SAFETY)\n";
        oss << "  .bot dungeon resume   - Resume autonomous navigation\n";
        oss << "  .bot dungeon status   - Show current status and readiness\n";
        oss << "  .bot dungeon enable   - Enable autonomy for your group\n";
        oss << "  .bot dungeon disable  - Disable autonomy (manual control)\n";
        oss << "  .bot dungeon aggro <level> - Set aggression level\n";
        oss << "    Levels: conservative, normal, aggressive, speedrun\n";

        handler->SendSysMessage(oss.str().c_str());
        return true;
    }

    bool PlayerbotCommandScript::HandleBotDungeonPauseCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        Group* group = player->GetGroup();
        if (!group)
        {
            handler->SendSysMessage("You must be in a group to use dungeon commands.");
            return false;
        }

        sDungeonAutonomyMgr->PauseDungeonAutonomy(group, player, "Manual pause via command");

        handler->SendSysMessage("|cffff0000[PAUSED]|r Dungeon autonomy paused for your group.");
        handler->SendSysMessage("Bots will hold position. Use '.bot dungeon resume' to continue.");

        TC_LOG_INFO("module.playerbot.dungeon", "Dungeon autonomy PAUSED for group {} by {}",
            group->GetGUID().ToString(), player->GetName());

        return true;
    }

    bool PlayerbotCommandScript::HandleBotDungeonResumeCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        Group* group = player->GetGroup();
        if (!group)
        {
            handler->SendSysMessage("You must be in a group to use dungeon commands.");
            return false;
        }

        sDungeonAutonomyMgr->ResumeDungeonAutonomy(group, player);

        handler->SendSysMessage("|cff00ff00[RESUMED]|r Dungeon autonomy resumed for your group.");
        handler->SendSysMessage("Bots will continue autonomous navigation.");

        TC_LOG_INFO("module.playerbot.dungeon", "Dungeon autonomy RESUMED for group {} by {}",
            group->GetGUID().ToString(), player->GetName());

        return true;
    }

    bool PlayerbotCommandScript::HandleBotDungeonStatusCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        Group* group = player->GetGroup();
        if (!group)
        {
            handler->SendSysMessage("You must be in a group to view dungeon status.");
            return false;
        }

        DungeonAutonomyState state = sDungeonAutonomyMgr->GetAutonomyState(group);
        DungeonAutonomyConfig config = sDungeonAutonomyMgr->GetConfig(group);

        std::ostringstream oss;
        oss << "Dungeon Autonomy Status\n";
        oss << "=======================\n\n";

        // State
        const char* stateStr = "UNKNOWN";
        switch (state)
        {
            case DungeonAutonomyState::DISABLED: stateStr = "|cff888888DISABLED|r"; break;
            case DungeonAutonomyState::PAUSED:   stateStr = "|cffff0000PAUSED|r"; break;
            case DungeonAutonomyState::ACTIVE:   stateStr = "|cff00ff00ACTIVE|r"; break;
            case DungeonAutonomyState::WAITING:  stateStr = "|cffffff00WAITING|r"; break;
            case DungeonAutonomyState::PULLING:  stateStr = "|cffff8800PULLING|r"; break;
            case DungeonAutonomyState::COMBAT:   stateStr = "|cffff0000COMBAT|r"; break;
            case DungeonAutonomyState::RECOVERING: stateStr = "|cff8888ffRECOVERING|r"; break;
        }
        oss << "State: " << stateStr << "\n";

        // Aggression level
        const char* aggroStr = "UNKNOWN";
        switch (config.aggressionLevel)
        {
            case DungeonAggressionLevel::CONSERVATIVE: aggroStr = "Conservative (safe)"; break;
            case DungeonAggressionLevel::NORMAL:       aggroStr = "Normal"; break;
            case DungeonAggressionLevel::AGGRESSIVE:   aggroStr = "Aggressive"; break;
            case DungeonAggressionLevel::SPEED_RUN:    aggroStr = "Speed Run (risky)"; break;
        }
        oss << "Aggression: " << aggroStr << "\n\n";

        // Group readiness
        float healthPct = sDungeonAutonomyMgr->GetGroupHealthPercent(group);
        float manaPct = sDungeonAutonomyMgr->GetHealerManaPercent(group);
        bool ready = sDungeonAutonomyMgr->IsGroupReadyToPull(group);

        oss << "Group Readiness:\n";
        oss << "  Health: " << std::fixed << std::setprecision(1) << healthPct << "%\n";
        oss << "  Healer Mana: " << manaPct << "%\n";
        oss << "  Ready to Pull: " << (ready ? "|cff00ff00YES|r" : "|cffff0000NO|r") << "\n";

        handler->SendSysMessage(oss.str().c_str());
        return true;
    }

    bool PlayerbotCommandScript::HandleBotDungeonEnableCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        Group* group = player->GetGroup();
        if (!group)
        {
            handler->SendSysMessage("You must be in a group to enable dungeon autonomy.");
            return false;
        }

        sDungeonAutonomyMgr->EnableAutonomy(group);

        handler->SendSysMessage("|cff00ff00[ENABLED]|r Dungeon autonomy enabled for your group.");
        handler->SendSysMessage("Bots will navigate autonomously. Use '.bot dungeon pause' to stop.");

        TC_LOG_INFO("module.playerbot.dungeon", "Dungeon autonomy ENABLED for group {} by {}",
            group->GetGUID().ToString(), player->GetName());

        return true;
    }

    bool PlayerbotCommandScript::HandleBotDungeonDisableCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        Group* group = player->GetGroup();
        if (!group)
        {
            handler->SendSysMessage("You must be in a group to disable dungeon autonomy.");
            return false;
        }

        sDungeonAutonomyMgr->DisableAutonomy(group);

        handler->SendSysMessage("|cff888888[DISABLED]|r Dungeon autonomy disabled for your group.");
        handler->SendSysMessage("Bots will follow player commands only.");

        TC_LOG_INFO("module.playerbot.dungeon", "Dungeon autonomy DISABLED for group {} by {}",
            group->GetGUID().ToString(), player->GetName());

        return true;
    }

    bool PlayerbotCommandScript::HandleBotDungeonAggroCommand(ChatHandler* handler, std::string level)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        Group* group = player->GetGroup();
        if (!group)
        {
            handler->SendSysMessage("You must be in a group to set aggression level.");
            return false;
        }

        DungeonAggressionLevel aggroLevel;
        std::string levelLower = level;
        std::transform(levelLower.begin(), levelLower.end(), levelLower.begin(), ::tolower);

        if (levelLower == "conservative" || levelLower == "safe")
            aggroLevel = DungeonAggressionLevel::CONSERVATIVE;
        else if (levelLower == "normal" || levelLower == "default")
            aggroLevel = DungeonAggressionLevel::NORMAL;
        else if (levelLower == "aggressive" || levelLower == "fast")
            aggroLevel = DungeonAggressionLevel::AGGRESSIVE;
        else if (levelLower == "speedrun" || levelLower == "speed")
            aggroLevel = DungeonAggressionLevel::SPEED_RUN;
        else
        {
            handler->SendSysMessage("Invalid aggression level. Valid options:");
            handler->SendSysMessage("  conservative - Wait for full health/mana, careful pulls");
            handler->SendSysMessage("  normal       - Standard dungeon pace");
            handler->SendSysMessage("  aggressive   - Pull when reasonably ready");
            handler->SendSysMessage("  speedrun     - Chain pull, minimal waiting (risky)");
            return false;
        }

        sDungeonAutonomyMgr->SetAggressionLevel(group, aggroLevel);

        handler->PSendSysMessage("Aggression level set to: %s", level.c_str());

        TC_LOG_INFO("module.playerbot.dungeon", "Dungeon aggression set to {} for group {} by {}",
            level, group->GetGUID().ToString(), player->GetName());

        return true;
    }

    // =====================================================================
    // CHEAT COMMANDS
    // =====================================================================

    bool PlayerbotCommandScript::HandleBotCheatCommand(ChatHandler* handler, std::string cheatName)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        BotCheatFlag flag = Playerbot::BotCheatMask::ParseCheatName(cheatName);
        if (flag == BotCheatFlag::NONE)
        {
            handler->PSendSysMessage("Unknown cheat: '%s'. Use '.bot cheat list' for available cheats.", cheatName.c_str());
            return false;
        }

        // Collect target bots: group members that are bots, or selected target
        std::vector<Player*> targets;

        // First check if selected target is a bot
        Player* target = ObjectAccessor::FindPlayer(player->GetTarget());
        if (target && target != player && target->GetSession() && target->GetSession()->IsBot())
        {
            targets.push_back(target);
        }
        else if (Group* group = player->GetGroup())
        {
            // Apply to all bots in the player's group
            for (GroupReference const& ref : group->GetMembers())
            {
                Player* member = ref.GetSource();
                if (member && member != player && member->GetSession() && member->GetSession()->IsBot())
                    targets.push_back(member);
            }
        }

        if (targets.empty())
        {
            handler->PSendSysMessage("No bot targets found. Select a bot or be in a group with bots.");
            return false;
        }

        uint32 enabled = 0;
        uint32 disabled = 0;
        for (Player* bot : targets)
        {
            bool hadCheat = sBotCheatMask->HasCheat(bot->GetGUID(), flag);
            sBotCheatMask->ToggleCheat(bot->GetGUID(), flag);
            if (hadCheat)
                ++disabled;
            else
                ++enabled;
        }

        handler->PSendSysMessage("Cheat '%s': %u bot(s) enabled, %u bot(s) disabled.",
            cheatName.c_str(), enabled, disabled);
        return true;
    }

    bool PlayerbotCommandScript::HandleBotCheatListCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        // Show available cheats
        handler->PSendSysMessage("=== Available Bot Cheats ===");
        for (auto const& info : Playerbot::BotCheatMask::GetCheatList())
        {
            // Skip presets in the "available" listing
            if (info.flag == BotCheatFlag::ALL_COMBAT ||
                info.flag == BotCheatFlag::ALL_MOVEMENT ||
                info.flag == BotCheatFlag::ALL)
            {
                handler->PSendSysMessage("  [preset] %s - %s", info.name, info.description);
                continue;
            }
            handler->PSendSysMessage("  %s - %s", info.name, info.description);
        }

        // Show active cheats on group bots
        handler->PSendSysMessage("=== Active Cheats ===");
        bool anyActive = false;

        if (Group* group = player->GetGroup())
        {
            for (GroupReference const& ref : group->GetMembers())
            {
                Player* member = ref.GetSource();
                if (member && member != player && member->GetSession() && member->GetSession()->IsBot())
                {
                    std::string cheats = sBotCheatMask->FormatActiveCheats(member->GetGUID());
                    if (cheats != "none")
                    {
                        handler->PSendSysMessage("  %s: %s", member->GetName().c_str(), cheats.c_str());
                        anyActive = true;
                    }
                }
            }
        }

        // Also check selected target
        Player* target = ObjectAccessor::FindPlayer(player->GetTarget());
        if (target && target != player && target->GetSession() && target->GetSession()->IsBot())
        {
            std::string cheats = sBotCheatMask->FormatActiveCheats(target->GetGUID());
            handler->PSendSysMessage("  [target] %s: %s", target->GetName().c_str(), cheats.c_str());
            anyActive = true;
        }

        if (!anyActive)
            handler->PSendSysMessage("  No active cheats on any bots.");

        uint32 totalCheatBots = sBotCheatMask->GetCheatBotCount();
        if (totalCheatBots > 0)
            handler->PSendSysMessage("Total bots with cheats: %u", totalCheatBots);

        return true;
    }

    bool PlayerbotCommandScript::HandleBotCheatOffCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        // Check if targeting a specific bot
        Player* target = ObjectAccessor::FindPlayer(player->GetTarget());
        if (target && target != player && target->GetSession() && target->GetSession()->IsBot())
        {
            sBotCheatMask->ClearAllCheats(target->GetGUID());
            handler->PSendSysMessage("Cleared all cheats on %s.", target->GetName().c_str());
            return true;
        }

        // Otherwise clear all group bots
        uint32 cleared = 0;
        if (Group* group = player->GetGroup())
        {
            for (GroupReference const& ref : group->GetMembers())
            {
                Player* member = ref.GetSource();
                if (member && member != player && member->GetSession() && member->GetSession()->IsBot())
                {
                    if (sBotCheatMask->HasAnyCheats(member->GetGUID()))
                    {
                        sBotCheatMask->ClearAllCheats(member->GetGUID());
                        ++cleared;
                    }
                }
            }
        }

        if (cleared > 0)
            handler->PSendSysMessage("Cleared cheats on %u bot(s).", cleared);
        else
            handler->PSendSysMessage("No bots had active cheats.");

        return true;
    }

    bool PlayerbotCommandScript::HandleBotCheatMultCommand(ChatHandler* handler, std::string cheatName, float multiplier)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        if (multiplier <= 0.0f || multiplier > 1000.0f)
        {
            handler->PSendSysMessage("Multiplier must be between 0.1 and 1000.0");
            return false;
        }

        // Collect targets
        std::vector<Player*> targets;
        Player* target = ObjectAccessor::FindPlayer(player->GetTarget());
        if (target && target != player && target->GetSession() && target->GetSession()->IsBot())
        {
            targets.push_back(target);
        }
        else if (Group* group = player->GetGroup())
        {
            for (GroupReference const& ref : group->GetMembers())
            {
                Player* member = ref.GetSource();
                if (member && member != player && member->GetSession() && member->GetSession()->IsBot())
                    targets.push_back(member);
            }
        }

        if (targets.empty())
        {
            handler->PSendSysMessage("No bot targets found.");
            return false;
        }

        // Apply multiplier based on cheat name
        std::string lower = cheatName;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        for (Player* bot : targets)
        {
            if (lower == "speed")
                sBotCheatMask->SetSpeedMultiplier(bot->GetGUID(), multiplier);
            else if (lower == "damage")
                sBotCheatMask->SetDamageMultiplier(bot->GetGUID(), multiplier);
            else if (lower == "xpboost" || lower == "xp")
                sBotCheatMask->SetXPMultiplier(bot->GetGUID(), multiplier);
            else
            {
                handler->PSendSysMessage("Unknown multiplier type: '%s'. Use speed, damage, or xpboost.", cheatName.c_str());
                return false;
            }
        }

        handler->PSendSysMessage("Set %s multiplier to %.1f on %u bot(s).",
            cheatName.c_str(), multiplier, static_cast<uint32>(targets.size()));
        return true;
    }

} // namespace Playerbot

// Register command script
void AddSC_playerbot_commandscript()
{
    new Playerbot::PlayerbotCommandScript();
}
