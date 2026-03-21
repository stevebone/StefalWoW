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
        case RACE_HUMAN:
            itemId = 14646;
            senderEntry = NPC_MERISSA_STILWELL;
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
}
