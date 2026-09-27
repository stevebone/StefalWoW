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

#include "Conversation.h"
#include "EventProcessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestDef.h"
#include "ScriptMgr.h"
#include "Spell.h"

#include "Custom_Mardum_Defines.h"

namespace Scripts::Custom::Mardum
{
    // 39279 - Assault on Mardum (bonus objective, auto-granted to Demon Hunters entering Mardum)
    class player_mardum_assault_bonus_objective : public PlayerScript
    {
    public:
        player_mardum_assault_bonus_objective() : PlayerScript("player_mardum_assault_bonus_objective") { }

        void OnLogin(Player* player, bool firstLogin) override
        {
            if (firstLogin)
                TryGrant(player);
        }

        // Covers DHs that did not spawn directly in Mardum (e.g. Void Elf DH starts on map 1865)
        // and reach it later via teleport/hearth/quest travel.
        void OnMapChanged(Player* player) override
        {
            TryGrant(player);
        }

    private:
        void TryGrant(Player* player)
        {
            if (player->GetClass() != CLASS_DEMON_HUNTER || player->GetMapId() != Maps::Mardum)
                return;

            // QUEST_STATUS_NONE only when never taken (or abandoned) - skips in-log, complete and rewarded
            if (player->GetQuestStatus(Quests::AssaultOnMardum) != QUEST_STATUS_NONE)
                return;

            if (Quest const* quest = sObjectMgr->GetQuestTemplate(Quests::AssaultOnMardum))
                if (player->CanAddQuest(quest, false))
                    player->AddQuest(quest, nullptr);
        }
    };

    // 40379 - Enter the Illidari: Coilskar
    // Covers relogging after the Coilskar Forces objective is done but before the
    // Sea-Caller summon fired - the 30s delayed cast does not survive a logout.
    class player_mardum_coilskar_forces : public PlayerScript
    {
    public:
        player_mardum_coilskar_forces() : PlayerScript("player_mardum_coilskar_forces") { }

        void OnLogin(Player* player, bool /*firstLogin*/) override
        {
            bool needsSpawn = false;
            if (player->GetQuestStatus(Quests::EnterTheIllidariCoilskar) == QUEST_STATUS_INCOMPLETE &&
                player->IsQuestObjectiveComplete(Quests::EnterTheIllidariCoilskar, Objectives::CoilskarForces))
                needsSpawn = true;

            if (player->GetQuestStatus(Quests::EnterTheIllidariShivarra) == QUEST_STATUS_COMPLETE)
                needsSpawn = true;

            if(needsSpawn)
                player->CastSpell(player, Spells::SummonCoilskarSeaCaller, CastSpellExtraArgs(TRIGGERED_FULL_MASK));
        }
    };

    // 38727/38819/38725 - Illidari assault quests; 569 - Illidari Foothold
    // Once all three are taken, the last accept schedules the briefing conversation.
    static bool IsAssaultQuest(uint32 questId)
    {
        return questId == Quests::StopTheBombardment
            || questId == Quests::TheirNumbersAreLegion
            || questId == Quests::IntoTheFoulCreche;
    }

    // "Taken" = in the quest log (not abandoned).
    static bool AllAssaultQuestsTaken(Player* player)
    {
        for (uint32 questId : { Quests::StopTheBombardment, Quests::TheirNumbersAreLegion, Quests::IntoTheFoulCreche })
        {
            QuestStatus status = player->GetQuestStatus(questId);
            if (status == QUEST_STATUS_NONE)
                return false;
        }
        return true;
    }

    class player_mardum_illidari_foothold : public PlayerScript
    {
    public:
        player_mardum_illidari_foothold() : PlayerScript("player_mardum_illidari_foothold") { }

        void OnQuestStatusChange(Player* player, uint32 questId) override
        {
            // Only a fresh accept/re-accept of an assault quest can complete the set;
            // this also fires on other status changes so both conditions are checked.
            if (!IsAssaultQuest(questId) || player->GetQuestStatus(questId) != QUEST_STATUS_INCOMPLETE)
                return;

            if (!AllAssaultQuestsTaken(player))
                return;

            // Scheduled on the player's EventProcessor so it is dropped on logout;
            // the in-log re-check cancels it if a quest is abandoned in the window.
            player->m_Events.AddEventAtOffset([player]()
            {
                if (AllAssaultQuestsTaken(player))
                    Conversation::CreateConversation(Conversations::IllidariFoothold, player, *player, player->GetGUID(), nullptr);
            }, 30s);
        }
    };
}

void AddSC_custom_mardum_player()
{
    using namespace Scripts::Custom::Mardum;

    new player_mardum_assault_bonus_objective();
    new player_mardum_coilskar_forces();
    new player_mardum_illidari_foothold();
}
