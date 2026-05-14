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
#include "CreatureAIImpl.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "QuestDef.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "TaskScheduler.h"

#include "Custom_Stormwind_Defines.h"

namespace Scripts::EasternKingdoms::StormwindCity
{
    // 30987 - Joining The Alliance
    class quest_30987_joining_the_alliance : public QuestScript
    {
    public:
        quest_30987_joining_the_alliance() : QuestScript("quest_30987_joining_the_alliance") {}

        void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
        {
            if (newStatus == QUEST_STATUS_INCOMPLETE)
            {
                player->m_Events.AddEventAtOffset([player]()
                    {
                        if (Creature* aysa = player->FindNearestCreature(Creatures::AysaSpawnedAtGate, 20.f))
                        {
                            aysa->AI()->Talk(0, player);
                            aysa->GetMotionMaster()->MoveFollow(player, 2.f);
                            PhasingHandler::RemovePhase(player, Phases::StormwindGateAysa, true);
                        }

                        if (Creature* jojo = player->FindNearestCreature(Creatures::JojoSpawnedAtGate, 20.f))
                        {
                            jojo->GetMotionMaster()->MoveFollow(player, 3.f);
                            PhasingHandler::RemovePhase(player, Phases::StormwindGateJojo, true);
                        }
                    }, std::chrono::seconds(1));
            }
        }
    };

    // 30988 - The Alliance Way
    class quest_30988_the_alliance_way : public QuestScript
    {
    public:
        quest_30988_the_alliance_way() : QuestScript("quest_30988_the_alliance_way") {}

        void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
        {
            if (newStatus == QUEST_STATUS_INCOMPLETE)
            {
                Creature* phasedVarian = player->FindNearestCreatureWithOptions(10.f, { .CreatureId = Creatures::KingVarianPhased, .IgnorePhases = true });
                Position pos = phasedVarian->GetPosition();
                PhasingHandler::RemovePhase(player, Phases::StormwindKeepKingVarian, true);
                Creature* spawnedVarian = player->SummonCreature(Creatures::KingVarianSpawned, pos);
                if(spawnedVarian)
                    spawnedVarian->AI()->SetGUID(player->GetGUID(), 1);
            }
        }
    };

    // 30989 - An Old Pit Fighter
    class quest_30989_an_old_pit_fighter : public QuestScript
    {
    public:
        quest_30989_an_old_pit_fighter() : QuestScript("quest_30989_an_old_pit_fighter") {}

        void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
        {
            if (newStatus == QUEST_STATUS_REWARDED)
            {
                if (Creature* aysa = player->FindNearestCreature(Creatures::AysaSpawnedAtGate, 20.f))
                    aysa->DespawnOrUnsummon();

                if (Creature* jojo = player->FindNearestCreature(Creatures::JojoSpawnedAtGate, 20.f))
                    jojo->DespawnOrUnsummon();
            }
        }
    };
}

void AddSC_custom_stormwind_quests()
{
    using namespace Scripts::EasternKingdoms::StormwindCity;

    new quest_30987_joining_the_alliance();
    new quest_30988_the_alliance_way();
    new quest_30989_an_old_pit_fighter();
}
