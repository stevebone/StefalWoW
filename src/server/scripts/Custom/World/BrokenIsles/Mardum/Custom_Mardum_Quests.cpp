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
#include "EventProcessor.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "QuestDef.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "WaypointDefines.h"

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
    // 39663 - On Felbat Wings
    // Bound through quest_template_addon.ScriptName - only fires for this quest.
    class quest_on_felbat_wings : public QuestScript
    {
    public:
        quest_on_felbat_wings() : QuestScript("quest_on_felbat_wings") { }

        void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
        {
            if (newStatus != QUEST_STATUS_INCOMPLETE)
                return;

            auto findFootholdNpc = [player](uint32 entry) -> Creature*
            {
                FindCreatureOptions options;
                options.CreatureId = entry;
                options.IgnorePhases = true;
                return player->FindNearestCreatureWithOptions(Misc::SearchRange, options);
            };

            Creature* kayn = findFootholdNpc(Creatures::KaynFoothold);
            if (!kayn)
                return;

            std::vector<ObjectGuid> escortGuids{ kayn->GetGUID() };
            for (uint32 entry : { Creatures::KorvasFoothold, Creatures::CyanaFoothold, Creatures::AllariFoothold })
                if (Creature* follower = findFootholdNpc(entry))
                {
                    follower->GetMotionMaster()->MoveFollow(kayn, Misc::FelbatWingsFollowDist);
                    escortGuids.push_back(follower->GetGUID());
                }

            kayn->m_Events.AddEventAtOffset([kayn, escortGuids]()
            {
                for (ObjectGuid guid : escortGuids)
                    if (Creature* npc = ObjectAccessor::GetCreature(*kayn, guid))
                        npc->Mount(Misc::FelSaberMountDisplay);
            }, 3s);

            kayn->m_Events.AddEventAtOffset([kayn]()
            {
                kayn->GetMotionMaster()->MovePath(
                    WaypointPath(Misc::FelbatWingsPathId, {
                        WaypointNode(0, 1418.64f, 1753.663f, 55.6051f),
                        WaypointNode(1, 1452.15f, 1688.47f, 47.3072f)
                    }, WaypointMoveType::Run, WaypointPathFlags::None), false);
            }, 5s);
        }
    };
}

void AddSC_custom_mardum_quests()
{
    using namespace Scripts::Custom::Mardum;

    new quest_enter_the_illidari_coilskar();
    new quest_enter_the_illidari_shivarra();
    new quest_on_felbat_wings();
}
