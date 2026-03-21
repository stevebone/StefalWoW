/*
 * This file is part of the Stefal WoW Project.
 * It is designed to work exclusively with the TrinityCore framework.
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
 * This code is provided for personal and educational use within the
 * Stefal WoW Project. It is not intended for commercial distribution,
 * resale, or any form of monetization.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"                 // PlayerScript
#include "Player.h"

#include "Followship_bots_mail_handler.h"

#include "Custom_Player_Defines.h"

class player_custom_voucher_mailer : public PlayerScript
{
public:
    player_custom_voucher_mailer() : PlayerScript("player_custom_voucher_mailer") { }

    void OnLogin(Player* player, bool firstLogin) override
    {
        // Example: send a welcome mail with an item
        if (firstLogin)
            SendVoucherMail(player);
    }

    void SendVoucherMail(Player* player)
    {
        // Create the mail
        std::string subject = "Welcome newcomer!";
        std::string text = "Hi there! I heard you are new in town and thought you might want some company. Come see me when you can.";

        uint32 itemId = 0;
        uint32 senderEntry = 0;

        switch (player->GetRace())
        {
        case RACE_PANDAREN_ALLIANCE:
        case RACE_KUL_TIRAN:
        case RACE_HUMAN:
            itemId = 14646;
            senderEntry = NPC_MERISSA_STILWELL;
            break;

        case RACE_NIGHTBORNE:
        case RACE_WORGEN:
        case RACE_NIGHTELF:
            itemId = 14648;
            senderEntry = NPC_ORENTHIL_WHISPERWIND;
            break;

        case RACE_MECHAGNOME:
        case RACE_GNOME:
        case RACE_EARTHEN_DWARF_ALLIANCE:
        case RACE_DARK_IRON_DWARF:
        case RACE_DWARF:
            itemId = 14647;
            senderEntry = NPC_YORI_CRACKHELM;
            break;

        case RACE_LIGHTFORGED_DRAENEI:
        case RACE_DRAENEI:
            itemId = 22888;
            senderEntry = NPC_JAELEIL;
            break;

        case RACE_PANDAREN_HORDE:
        case RACE_MAGHAR_ORC:
        case RACE_ZANDALARI_TROLL:
        case RACE_TROLL:
        case RACE_GOBLIN:
        case RACE_VULPERA:
        case RACE_ORC:
            itemId = 14649;
            senderEntry = NPC_MAGGA;
            break;

        case RACE_HIGHMOUNTAIN_TAUREN:
        case RACE_TAUREN:
            itemId = 14650;
            senderEntry = NPC_VORN_SKYSEER;
            break;

        case RACE_UNDEAD_PLAYER:
        case RACE_EARTHEN_DWARF_HORDE:
            itemId = 14651;
            senderEntry = NPC_CLAIRE_WILLOWER;
            break;

        case RACE_VOID_ELF:
        case RACE_BLOODELF:
            itemId = 20938;
            senderEntry = NPC_MARSILLA_DAWNSTAR;
            break;

        default:
            break;
        }

        if (itemId)
            FSBMail::SendMail(senderEntry, player, subject, text, { {itemId, 1} });
    }
};

class player_custom_riding_mailer : public PlayerScript
{
public:
    player_custom_riding_mailer() : PlayerScript("player_custom_riding_mailer") { }

    void OnLevelChanged(Player* player, uint8 /*oldLevel*/) override
    {
        if (!player)
            return;

        if (player->GetLevel() != 10)
            return;

        SendRidingMail(player);
    }

    void SendRidingMail(Player* player)
    {
        // Create the mail
        std::string subject = "Ready yet for a trusted mount?";
        std::string text = "Hi there! I heard you might be interested in moving about somewhat faster. You may think you do not need my guidance but it would still be worth coming to see me.";

        uint32 itemId = 0;
        uint32 senderEntry = 0;

        switch (player->GetRace())
        {
        case RACE_PANDAREN_ALLIANCE:
        case RACE_KUL_TIRAN:
        case RACE_HUMAN:
            itemId = 46875;
            senderEntry = NPC_RANDAL_HUNTER;
            break;

        case RACE_NIGHTBORNE:
        case RACE_WORGEN:
        case RACE_NIGHTELF:
            itemId = 46876;
            senderEntry = NPC_JARTSAM;
            break;

        case RACE_EARTHEN_DWARF_ALLIANCE:
        case RACE_DARK_IRON_DWARF:
        case RACE_DWARF:
            itemId = 46877;
            senderEntry = NPC_ULTHAM_IRONHORN;
            break;

        case RACE_MECHAGNOME:
        case RACE_GNOME:
            itemId = 46878;
            senderEntry = NPC_BINJY_FEATHERWHISTLE;
            break;

        case RACE_LIGHTFORGED_DRAENEI:
        case RACE_DRAENEI:
            itemId = 46879;
            senderEntry = NPC_AALUN;
            break;

        case RACE_PANDAREN_HORDE:
        case RACE_MAGHAR_ORC:
        case RACE_GOBLIN:
        case RACE_VULPERA:
        case RACE_ORC:
            itemId = 46880;
            senderEntry = NPC_KILDAR;
            break;

        case RACE_ZANDALARI_TROLL:
        case RACE_TROLL:
            itemId = 46883;
            senderEntry = NPC_XARTI;
            break;

        case RACE_HIGHMOUNTAIN_TAUREN:
        case RACE_TAUREN:
            itemId = 46884;
            senderEntry = NPC_KAR_STORMWSINGER;
            break;

        case RACE_UNDEAD_PLAYER:
        case RACE_EARTHEN_DWARF_HORDE:
            itemId = 46881;
            senderEntry = NPC_VELMA_WARNAM;
            break;

        case RACE_VOID_ELF:
        case RACE_BLOODELF:
            itemId = 46882;
            senderEntry = NPC_PERASCAMIN;
            break;

        default:
            break;
        }

        if (itemId)
            FSBMail::SendMail(senderEntry, player, subject, text, { {itemId, 1} });
    }
};

void AddSC_custom_player_mails()
{
    new player_custom_voucher_mailer();
    new player_custom_riding_mailer();
}
