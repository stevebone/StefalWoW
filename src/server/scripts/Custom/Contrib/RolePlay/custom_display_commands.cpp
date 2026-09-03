#include "custom_display_handler.h"
#include "Define.h"
#include "Chat.h"
#include "ChatCommand.h"
#include "Common.h"

namespace RoleplayCore
{
    using namespace Trinity::ChatCommands;

    class DisplayCommands : public CommandScript
    {
    public:
        DisplayCommands() : CommandScript("player_display_commands") { }

        std::span<ChatCommandBuilder const> GetCommands() const override
        {
            // RPCORE_DISPLAY_NO_DB: reuse a stock player-level RBAC permission instead of
            // the custom RBAC_PERM_COMMAND_DISP_* (1008..1021). The custom perms are only
            // declared in RBAC.h; they must be inserted into the rbac_permissions table
            // and granted to a role, otherwise the command table is rejected at load and
            // none of the .display subcommands work. RBAC_PERM_COMMAND_GUILD (401) is a
            // stock permission that every player has by default (the same level used by
            // NpcLLM with RBAC_PERM_COMMAND_NPC_SAY), so .display works out of the box
            // with zero database setup.
            static ChatCommandTable displayCommandTable =
            {
                { "head",      rbac::RBAC_PERM_COMMAND_GUILD, false, Display<DISPLAY_TYPE_HEAD>,            "" },
                { "shoulders", rbac::RBAC_PERM_COMMAND_GUILD, false, Display<DISPLAY_TYPE_SHOULDERS>,       "" },
                { "lshoulder", rbac::RBAC_PERM_COMMAND_GUILD, false, Display<DISPLAY_TYPE_SHOULDERS, true>, "" },
                { "shirt",     rbac::RBAC_PERM_COMMAND_GUILD, false, Display<DISPLAY_TYPE_SHIRT>,           "" },
                { "chest",     rbac::RBAC_PERM_COMMAND_GUILD, false, Display<DISPLAY_TYPE_CHEST>,           "" },
                { "waist",     rbac::RBAC_PERM_COMMAND_GUILD, false, Display<DISPLAY_TYPE_WAIST>,           "" },
                { "legs",      rbac::RBAC_PERM_COMMAND_GUILD, false, Display<DISPLAY_TYPE_PANTS>,           "" },
                { "feet",      rbac::RBAC_PERM_COMMAND_GUILD, false, Display<DISPLAY_TYPE_BOOTS>,           "" },
                { "wrists",    rbac::RBAC_PERM_COMMAND_GUILD, false, Display<DISPLAY_TYPE_WRISTS>,          "" },
                { "hands",     rbac::RBAC_PERM_COMMAND_GUILD, false, Display<DISPLAY_TYPE_HANDS>,           "" },
                { "back",      rbac::RBAC_PERM_COMMAND_GUILD, false, Display<DISPLAY_TYPE_BACK>,            "" },
                { "tabard",    rbac::RBAC_PERM_COMMAND_GUILD, false, Display<DISPLAY_TYPE_TABARD>,          "" },
                { "mainhand",  rbac::RBAC_PERM_COMMAND_GUILD, false, Display<DISPLAY_TYPE_MAIN>,            "" },
                { "offhand",   rbac::RBAC_PERM_COMMAND_GUILD, false, Display<DISPLAY_TYPE_OFF>,             "" },
            };

            static ChatCommandTable commandTable =
            {
                { "display", displayCommandTable},
            };

            return commandTable;
        }

        // Optimized template method for processing display commands
        template <DisplayType T, bool secondary = false>
        static bool Display(ChatHandler* handler, char const* args)
        {
            if (!handler || !args)
                return false;

            char const* id = handler->extractKeyFromLink((char*)args, "Hitem");
            if (!id)
                return false;

            // Retrieve item ID
            uint32 itemId = static_cast<uint32>(strtoul(id, nullptr, 10));

            // Retrieve the bonus, if specified
            uint32 bonus = 0;
            char* bonusStr = strtok(NULL, " ");
            if (bonusStr)
                bonus = strtol(bonusStr, NULL, 10);

            // Call the display handler
            DisplayHandler::GetInstance().Display(handler, T, itemId, bonus, secondary);
            return true;
        }
    };
}

void AddSC_CustomDisplayCommands()
{
    new RoleplayCore::DisplayCommands();
}
