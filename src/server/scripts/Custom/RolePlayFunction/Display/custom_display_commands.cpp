#include "custom_display_handler.h"
#include "Define.h"
#include "Base32.h"
#include "Chat.h"
#include "ChatCommand.h"
#include "Common.h"


namespace Noblegarden
{
    using namespace Trinity::ChatCommands;
    class DisplayCommands : public CommandScript
    {
    public:
        DisplayCommands() : CommandScript("player_display_commands") { }

        ChatCommandTable GetCommands() const override
        {
            static ChatCommandTable displayCommandTable =
            {
                { "head",      rbac::RBAC_PERM_COMMAND_DISP_HEAD,       false, Display<DISPLAY_TYPE_HEAD>,            "" },
                { "shoulders", rbac::RBAC_PERM_COMMAND_DISP_SHOULDERS,  false, Display<DISPLAY_TYPE_SHOULDERS>,       "" },
                { "lshoulder", rbac::RBAC_PERM_COMMAND_DISP_SHOULDERS,  false, Display<DISPLAY_TYPE_SHOULDERS, true>, "" },
                { "shirt",     rbac::RBAC_PERM_COMMAND_DISP_SHIRT,      false, Display<DISPLAY_TYPE_SHIRT>,           "" },
                { "chest",     rbac::RBAC_PERM_COMMAND_DISP_CHEST,      false, Display<DISPLAY_TYPE_CHEST>,           "" },
                { "waist",     rbac::RBAC_PERM_COMMAND_DISP_WAIST,      false, Display<DISPLAY_TYPE_WAIST>,           "" },
                { "legs",      rbac::RBAC_PERM_COMMAND_DISP_LEGS,       false, Display<DISPLAY_TYPE_PANTS>,           "" },
                { "feet",      rbac::RBAC_PERM_COMMAND_DISP_FEET,       false, Display<DISPLAY_TYPE_BOOTS>,           "" },
                { "wrists",    rbac::RBAC_PERM_COMMAND_DISP_WRISTS,     false, Display<DISPLAY_TYPE_WRISTS>,          "" },
                { "hands",     rbac::RBAC_PERM_COMMAND_DISP_HANDS,      false, Display<DISPLAY_TYPE_HANDS>,           "" },
                { "back",      rbac::RBAC_PERM_COMMAND_DISP_BACK,       false, Display<DISPLAY_TYPE_BACK>,            "" },
                { "tabard",    rbac::RBAC_PERM_COMMAND_DISP_TABARD,     false, Display<DISPLAY_TYPE_TABARD>,          "" },
                { "mainhand",  rbac::RBAC_PERM_COMMAND_DISP_MAINHAND,   false, Display<DISPLAY_TYPE_MAIN>,            "" },
                { "offhand",   rbac::RBAC_PERM_COMMAND_DISP_OFFHAND,    false, Display<DISPLAY_TYPE_OFF>,             "" },
                //{ "current",   rbac::RBAC_ROLE_PLAYER, false, SendCurrentTrasmogrification,       "" },
            };

            static ChatCommandTable commandTable =
            {
                { "display", displayCommandTable},
            };

            return commandTable;
        }

        template <DisplayType T, bool secondary = false> static bool Display(ChatHandler* handler, char const* args)
        {
            if (char const* id = handler->extractKeyFromLink((char*)args, "Hitem"))
            {
                uint32 bonus = 0;

                if (auto bonuses = strtok(NULL, " "))
                {
                    bonus = strtol(bonuses, NULL, 10);
                }

                DisplayHandler::GetInstance().Display(handler, T, (uint32)strtoul(id, nullptr, 10), bonus, secondary);
                return true;
            }
            else
            {
                return false;
            }
        }
    };
};

void AddSC_CustomDisplayCommands()
{
    new Noblegarden::DisplayCommands();
}
