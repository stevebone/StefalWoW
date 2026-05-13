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
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "TaskScheduler.h"

#include "Custom_Stormwind_Defines.h"

namespace Scripts::EasternKingdoms::StormwindCity
{
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

                Creature* aysa = player->FindNearestCreature(Creatures::AysaSpawnedAtGate, 20.f);
                if (!aysa)
                {
                    aysa = player->SummonCreature(Creatures::AysaSpawnedAtGate, player->GetRandomNearPosition(3.f));
                    if (aysa)
                        aysa->SetOwnerGUID(player->GetGUID());

                }

                Creature* jojo = player->FindNearestCreature(Creatures::JojoSpawnedAtGate, 20.f);
                if (!jojo)
                {
                    jojo = player->SummonCreature(Creatures::JojoSpawnedAtGate, player->GetRandomNearPosition(3.f));
                    if (jojo)
                        jojo->SetOwnerGUID(player->GetGUID());

                }

            }
        }
    };
}

void AddSC_custom_stormwind_quests()
{
    using namespace Scripts::EasternKingdoms::StormwindCity;

    new quest_30988_the_alliance_way();
}
