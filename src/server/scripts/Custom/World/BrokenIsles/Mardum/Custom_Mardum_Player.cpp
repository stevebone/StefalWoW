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
#include "DB2Stores.h"
#include "DBCEnums.h"
#include "EventProcessor.h"
#include "ObjectMgr.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "PlayerChoice.h"
#include "QuestDef.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "TemporarySummon.h"

#include "Custom_Mardum_Defines.h"

namespace Scripts::Custom::Mardum
{
    // 39279 - Assault on Mardum (bonus objective, auto-granted to Demon Hunters entering Mardum)
    class player_mardum_assault_bonus_objective : public PlayerScript
    {
    public:
        player_mardum_assault_bonus_objective() : PlayerScript("player_mardum_assault_bonus_objective") { }

        void OnQuestStatusChange(Player* player, uint32 questId) override
        {
            if (questId != Quests::TheInvasionBegins)
                return;

            QuestStatus status = player->GetQuestStatus(questId);

            if (status == QUEST_STATUS_COMPLETE)
                PhasingHandler::OnConditionChange(player, true);

            if (status == QUEST_STATUS_REWARDED)
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

    // 38729 - Return to the Black Temple
    // Spell 192140 applies the fel screen effect (aura 260, misc 827) and
    // triggers movie 471. When the client reports the movie finished or
    // skipped, the player is teleported to the Fel Hammer and bound there.
    class player_mardum_back_to_black_temple : public PlayerScript
    {
    public:
        player_mardum_back_to_black_temple() : PlayerScript("player_mardum_back_to_black_temple") { }

        void OnMovieComplete(Player* player, uint32 movieId) override
        {
            // The aura marks a player mid-sequence; removing it makes re-entry safe.
            if (movieId != Movies::BackToBlackTemple || !player->HasAura(Spells::BackToBlackTemple))
                return;

            player->RemoveAurasDueToSpell(Spells::BackToBlackTemple);
            player->CastSpell(player, Spells::TeleportVaultOfWardens, true);
            player->m_Events.AddEventAtOffset([player]()
            {
                player->SetHomebind(player->GetWorldLocation(), Misc::KeystoneReturnBindArea);
            }, 1s);
        }

        // Covers relogging while the movie was playing - the aura survives the
        // logout but CMSG_COMPLETE_MOVIE will never arrive for it.
        void OnLogin(Player* player, bool /*firstLogin*/) override
        {
            if (player->HasAura(Spells::BackToBlackTemple) &&
                player->GetQuestRewardStatus(Quests::ReturnToTheBlackTemple))
                OnMovieComplete(player, Movies::BackToBlackTemple);
        }
    };

    // PlayerChoice 231 - Fel Secrets (quest 40051)
    class playerchoice_fel_secrets : public PlayerChoiceScript
    {
    public:
        playerchoice_fel_secrets() : PlayerChoiceScript("playerchoice_fel_secrets") { }

        void OnResponse(WorldObject* /*object*/, Player* player, PlayerChoice const* /*choice*/,
            PlayerChoiceResponse const* response, uint16 /*clientIdentifier*/) override
        {
            uint32 choiceSpell;
            ChrSpecialization spec;
            switch (response->ResponseId)
            {
                case PlayerChoices::FelSecretsHavocResponse:
                    choiceSpell = Spells::FelSecretsHavocChosen;
                    spec = ChrSpecialization::DemonHunterHavoc;
                    break;
                case PlayerChoices::FelSecretsVengeanceResponse:
                    choiceSpell = Spells::FelSecretsVengeanceChosen;
                    spec = ChrSpecialization::DemonHunterVengeance;
                    break;
                default:
                    return;
            }

            // spec-chosen spell: grants the quest kill credit and completes the tracking quest
            player->CastSpell(player, choiceSpell, CastSpellExtraArgs(TRIGGERED_FULL_MASK));

            if (ChrSpecializationEntry const* specEntry = sChrSpecializationStore.LookupEntry(uint32(spec)))
                player->ActivateTalentGroup(specEntry);
        }
    };
}

void AddSC_custom_mardum_player()
{
    using namespace Scripts::Custom::Mardum;

    new player_mardum_assault_bonus_objective();
    new player_mardum_illidari_foothold();
    new player_mardum_back_to_black_temple();
    new playerchoice_fel_secrets();
}
