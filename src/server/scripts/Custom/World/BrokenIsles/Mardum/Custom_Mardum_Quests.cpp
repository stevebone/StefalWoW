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

#include "EventProcessor.h"
#include "Player.h"
#include "QuestDef.h"
#include "ScriptMgr.h"
#include "Spell.h"

#include "Custom_Mardum_Defines.h"

namespace Scripts::Custom::Mardum
{
    // 40379 - Enter the Illidari: Coilskar
    // Bound through quest_template_addon.ScriptName - only fires for this quest.
    class quest_enter_the_illidari_coilskar : public QuestScript
    {
    public:
        quest_enter_the_illidari_coilskar() : QuestScript("quest_enter_the_illidari_coilskar") { }

        void OnQuestObjectiveChange(Player* player, Quest const* quest, QuestObjective const& objective, int32 /*oldAmount*/, int32 /*newAmount*/) override
        {
            if (objective.ID != Objectives::CoilskarForces)
                return;

            if (!player->IsQuestObjectiveComplete(quest->GetQuestId(), objective.ID))
                return;

            // Scheduled on the player's EventProcessor so it is dropped on logout;
            // the login check in player_mardum_coilskar_forces covers that case.
            player->m_Events.AddEventAtOffset([player]()
            {
                player->CastSpell(player, Spells::SummonCoilskarSeaCaller, CastSpellExtraArgs(TRIGGERED_FULL_MASK));
            }, 30s);
        }
    };

    // 38765 - Enter the Illidari: Shivarra
    // Bound through quest_template_addon.ScriptName - only fires for this quest.
    class quest_enter_the_illidari_shivarra : public QuestScript
    {
    public:
        quest_enter_the_illidari_shivarra() : QuestScript("quest_enter_the_illidari_shivarra") { }

        void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
        {
            if (newStatus == QUEST_STATUS_NONE)
            {
                player->RemoveActiveQuest(Quests::SevisSacrificeTracker, false);
                player->RemoveRewardedQuest(Quests::SevisSacrificeTracker);
            }

            if (newStatus == QUEST_STATUS_COMPLETE)
            {
                // Scheduled on the player's EventProcessor so it is dropped on logout;
                // the login check in player_mardum_coilskar_forces covers that case.
                player->m_Events.AddEventAtOffset([player]()
                    {
                        player->CastSpell(player, Spells::SummonCoilskarSeaCaller, CastSpellExtraArgs(TRIGGERED_FULL_MASK));
                    }, 30s);
            }
        }
    };
}

void AddSC_custom_mardum_quests()
{
    using namespace Scripts::Custom::Mardum;

    new quest_enter_the_illidari_coilskar();
    new quest_enter_the_illidari_shivarra();
}
