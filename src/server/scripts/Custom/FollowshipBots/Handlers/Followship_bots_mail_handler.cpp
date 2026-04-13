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

#include "Creature.h"
#include "DatabaseEnv.h"
#include "GameTime.h"                 // GameTime::GetGameTime
#include "Item.h"
#include "Mail.h"
#include "ObjectMgr.h"                // ItemContext
#include "Player.h"

#include "Followship_bots_mail_handler.h"

namespace FSBMail
{
    void SendMail(uint32 creatureEntry, Player* player, std::string const& subject, std::string const& body,
        std::vector<std::pair<uint32, uint32>> const& items, uint32 money, uint32 delaySeconds)
    {
        if (!player || !creatureEntry)
            return;

        MailDraft draft = MailDraft(subject, body);

        CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

        for (auto const& entry : items)
        {
            uint32 itemId = entry.first;
            uint32 count = entry.second;

            if (Item* item = Item::CreateItem(itemId, count, ItemContext::NONE, player))
            {
                // Attach item to the mail
                draft.AddItem(item);
                // save new item before send
                item->SaveToDB(trans);
            }
        }

        //Add money
        if (money > 0)
            draft.AddMoney(money);

        // Sender: system (0)
        MailSender sender(MAIL_CREATURE, creatureEntry, MAIL_STATIONERY_DEFAULT);

        // Send instantly
        draft.SendMailTo(trans, player, sender, MAIL_CHECK_MASK_NONE, delaySeconds);
    }
}
