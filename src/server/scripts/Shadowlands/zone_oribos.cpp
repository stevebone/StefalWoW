/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
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
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "Log.h"
#include "Player.h"
#include "QuestDef.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include <algorithm>
#include <array>

enum TalInaraData
{
    // Gossip menu of Tal-Inara 159478 "Honored Voice" (Ring of Fates, Oribos, map 2222).
    GOSSIP_MENU_TAL_INARA                   = 26284,

    // gossip_menu_option.OptionID 4, "I am ready to go.".
    // CreatureAI::OnGossipSelect is handed GossipMenuItem::OrderIndex, which is loaded from that column
    // (ObjectMgr::LoadGossipMenuItems), so this compares against OptionID and not against GossipOptionID.
    GOSSIP_OPTION_I_AM_READY_TO_GO          = 4,

    // Creature 173614 "Scene Kill Credit". Zero spawns anywhere in the world - it is a credit-only entry, the
    // same arrangement 167383 has for quest 62000 "Choosing Your Purpose", so the gossip selection is what has
    // to hand the credit out.
    NPC_CREDIT_SCENE_KILL_CREDIT            = 173614,

    // The four quests whose single objective is QUEST_OBJECTIVE_MONSTER on 173614, described as
    // "Speak with Tal-Inara to choose where to go". They are the four "pick the next Shadowlands zone" steps of
    // the levelling campaign and are all given and turned in by Tal-Inara; only one of them can be in the log
    // at a time.
    //
    //   QuestID | Objective | LogTitle
    //   --------+-----------+---------------------------
    //    62159  |  407308   | Aiding the Shadowlands     (follows 62000 "Choosing Your Purpose")
    //    63208  |  409231   | The Next Step
    //    63209  |  409233   | Furthering the Purpose
    //    63210  |  409235   | The Last Step
    QUEST_AIDING_THE_SHADOWLANDS            = 62159,
    QUEST_THE_NEXT_STEP                     = 63208,
    QUEST_FURTHERING_THE_PURPOSE            = 63209,
    QUEST_THE_LAST_STEP                     = 63210
};

std::array<uint32, 4> constexpr ChooseWhereToGoQuests =
{
    QUEST_AIDING_THE_SHADOWLANDS,
    QUEST_THE_NEXT_STEP,
    QUEST_FURTHERING_THE_PURPOSE,
    QUEST_THE_LAST_STEP
};

/*
 * 159478 - Tal-Inara
 *
 * "I am ready to go." (menu 26284, option 4) is the answer to "Speak with Tal-Inara to choose where to go".
 * On retail it opens the Shadowlands zone-choice scene, which is where the credit on 173614 ("Scene Kill
 * Credit") comes from; the scene package id and the destination it flies the player to are not in our data, so
 * only the credit is reproduced here. That is what completes the quest - Tal-Inara is also the quest ender, so
 * the player turns it in to her straight afterwards.
 */
struct npc_tal_inara : public ScriptedAI
{
    npc_tal_inara(Creature* creature) : ScriptedAI(creature) { }

    bool OnGossipSelect(Player* player, uint32 menuId, uint32 gossipListId) override
    {
        if (menuId != GOSSIP_MENU_TAL_INARA || gossipListId != GOSSIP_OPTION_I_AM_READY_TO_GO)
            return false;

        // The option is gated by CONDITION_SOURCE_TYPE_GOSSIP_MENU_OPTION on exactly these four quests, so it
        // cannot normally be picked outside of them. The check is repeated here so that the script stays a no-op
        // if the conditions are missing from the world database, and so that picking the option twice (the menu
        // can still be open on the client when the credit lands) cannot do anything a second time.
        auto questItr = std::ranges::find_if(ChooseWhereToGoQuests, [player](uint32 questId)
        {
            return player->GetQuestStatus(questId) == QUEST_STATUS_INCOMPLETE;
        });

        if (questItr == ChooseWhereToGoQuests.end())
        {
            TC_LOG_DEBUG("scripts", "npc_tal_inara: {} picked menu {} option {} without any of the "
                "\"choose where to go\" quests outstanding, ignored",
                player->GetGUID().ToString(), uint32(GOSSIP_MENU_TAL_INARA), uint32(GOSSIP_OPTION_I_AM_READY_TO_GO));

            CloseGossipMenuFor(player);
            return true;
        }

        // Credits the QUEST_OBJECTIVE_MONSTER on 173614 of whichever of the four is in the log. KilledMonsterCredit
        // only touches objectives that are still outstanding, so this is idempotent on its own as well.
        player->KilledMonsterCredit(NPC_CREDIT_SCENE_KILL_CREDIT);

        CloseGossipMenuFor(player);
        return true;
    }
};

void AddSC_oribos()
{
    RegisterCreatureAI(npc_tal_inara);
}
