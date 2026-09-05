#include "custom_display_handler.h"
#include "Define.h"
#include "Chat.h"
#include "ChatCommand.h"
#include "Common.h"
#include "Hyperlinks.h"
#include "RBAC.h"
#include "ScriptMgr.h"

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
                { "head",      Display<DISPLAY_TYPE_HEAD>,            rbac::RBAC_PERM_COMMAND_GUILD, Console::No },
                { "shoulders", Display<DISPLAY_TYPE_SHOULDERS>,       rbac::RBAC_PERM_COMMAND_GUILD, Console::No },
                { "lshoulder", Display<DISPLAY_TYPE_SHOULDERS, true>, rbac::RBAC_PERM_COMMAND_GUILD, Console::No },
                { "shirt",     Display<DISPLAY_TYPE_SHIRT>,           rbac::RBAC_PERM_COMMAND_GUILD, Console::No },
                { "chest",     Display<DISPLAY_TYPE_CHEST>,           rbac::RBAC_PERM_COMMAND_GUILD, Console::No },
                { "waist",     Display<DISPLAY_TYPE_WAIST>,           rbac::RBAC_PERM_COMMAND_GUILD, Console::No },
                { "legs",      Display<DISPLAY_TYPE_PANTS>,           rbac::RBAC_PERM_COMMAND_GUILD, Console::No },
                { "feet",      Display<DISPLAY_TYPE_BOOTS>,           rbac::RBAC_PERM_COMMAND_GUILD, Console::No },
                { "wrists",    Display<DISPLAY_TYPE_WRISTS>,          rbac::RBAC_PERM_COMMAND_GUILD, Console::No },
                { "hands",     Display<DISPLAY_TYPE_HANDS>,           rbac::RBAC_PERM_COMMAND_GUILD, Console::No },
                { "back",      Display<DISPLAY_TYPE_BACK>,            rbac::RBAC_PERM_COMMAND_GUILD, Console::No },
                { "tabard",    Display<DISPLAY_TYPE_TABARD>,          rbac::RBAC_PERM_COMMAND_GUILD, Console::No },
                { "mainhand",  Display<DISPLAY_TYPE_MAIN>,            rbac::RBAC_PERM_COMMAND_GUILD, Console::No },
                { "offhand",   Display<DISPLAY_TYPE_OFF>,             rbac::RBAC_PERM_COMMAND_GUILD, Console::No },
            };

            static ChatCommandTable commandTable =
            {
                { "display", displayCommandTable},
            };

            return commandTable;
        }

        template <DisplayType T, bool secondary = false>
        static bool Display(ChatHandler* handler, Variant<Hyperlink<item>, uint32> itemArg, Optional<uint32> bonusArg)
        {
            if (!handler)
                return false;

            uint32 itemId = 0;
            if (Hyperlink<item> const* itemLinkData = std::get_if<Hyperlink<item>>(&itemArg))
                itemId = (*itemLinkData)->Item->GetId();
            else if (uint32 const* itemIdPtr = std::get_if<uint32>(&itemArg))
                itemId = *itemIdPtr;

            uint32 bonus = bonusArg.value_or(0);

            DisplayHandler::GetInstance().Display(handler, T, itemId, bonus, secondary);
            return true;
        }
    };
}

void AddSC_CustomDisplayCommands()
{
    new RoleplayCore::DisplayCommands();
}
