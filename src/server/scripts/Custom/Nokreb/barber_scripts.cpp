/*
 * Barber command script for StefalWoW (TrinityCore fork)
 * Adapted from free_share_scripts.cpp — only the .barber command retained.
 *
 * Usage in game (as GM):
 *   .barber            — opens the barber shop on the player's screen
 *   .barber <mask>     — same, with a specific customization feature mask
 *                        (mask argument is accepted for compatibility but
 *                         currently forced to 0 inside the packet — see code)
 */

#include "Chat.h"
#include "ChatCommandTags.h"
#include "ChatCommand.h"
#include "MiscPackets.h"
#include "ScriptMgr.h"
#include "WorldSession.h"
#include "RBAC.h"

using namespace Trinity::ChatCommands;

class barber_scripts : public CommandScript
{
public:
    barber_scripts() : CommandScript("barber_scripts") {}

    std::span<ChatCommandBuilder const> GetCommands() const override
    {
        static ChatCommandTable commandTable =
        {
            // .barber [mask]
            { "barber", HandleBarberCommand, rbac::RBAC_PERM_COMMAND_GM, Trinity::ChatCommands::Console::No },
        };

        return commandTable;
    }

    static bool HandleBarberCommand(ChatHandler* handler, Optional<uint32> featureMask)
    {
        // mask is accepted for forward-compatibility; the SMSG_ENABLE_BARBER_SHOP
        // packet itself only carries a single u32 — we currently always send 0
        // so the client opens the full customization UI.
        if (WorldSession* session = handler->GetSession())
        {
            Player* player = session->GetPlayer();
            if (!player)
            {
                handler->SendSysMessage(LANG_USE_BOL);
                handler->SetSentErrorMessage(true);
                return false;
            }

            WorldPackets::Misc::EnableBarberShop enableBarberShop;
            enableBarberShop.CustomizationFeatureMask = featureMask ? *featureMask : 0u;
            player->SendDirectMessage(enableBarberShop.Write());
            return true;
        }

        handler->SendSysMessage(LANG_USE_BOL);
        handler->SetSentErrorMessage(true);
        return false;
    }
};

void AddSC_barber_scripts()
{
    new barber_scripts();
}
