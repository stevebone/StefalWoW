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

#include "Chat.h"
#include "ScriptMgr.h"
#include "Player.h"

#include "Custom_TirisfalGlades_Defines.h"

namespace Scripts::Custom::TirisfalGlades
{
    class player_tirisfal_glades_darnell_handler : public PlayerScript
    {
    public:
        player_tirisfal_glades_darnell_handler() : PlayerScript("player_tirisfal_glades_darnell_handler") { }

        void OnLogin(Player* player, bool /*firstLogin*/) override
        {
            if (!player || !player->IsInWorld())
                return;

            if (player->IsActiveQuest(Quests::quest_recruitment) || player->IsActiveQuest(Quests::quest_beyond_the_graves))
                player->CastSpell(player, SpellsMisc::spell_summon_darnell_1, true);

            if (player->IsActiveQuest(Quests::quest_the_shadow_grave))
                player->CastSpell(player, SpellsMisc::spell_summon_darnell_0, true);
        }

        void OnQuestStatusChange(Player* player, uint32 questId) override
        {
            QuestStatus status = player->GetQuestStatus(questId);
            if(questId == Quests::quest_the_wakening && status == QUEST_STATUS_REWARDED)
                player->CastSpell(player, SpellsMisc::spell_summon_darnell_1, true);
        }
    };
}

void AddSC_custom_tirisfal_glades_player()
{
    using namespace Scripts::Custom::TirisfalGlades;

    new player_tirisfal_glades_darnell_handler();
}
