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
#include "Player.h"
#include "QuestDef.h"
#include "ScriptMgr.h"
#include "TemporarySummon.h"

#include "Custom_Duskwood_Defines.h"

namespace Scripts::EasternKingdoms::Duskwood
{
    // 26760 - Cry For The Moon
    class quest_26760_cry_for_the_moon : public QuestScript
    {
    public:
        quest_26760_cry_for_the_moon() : QuestScript("quest_26760_cry_for_the_moon") {}

        void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
        {
            if (newStatus != QUEST_STATUS_INCOMPLETE)
                return;

            player->RemoveAura(Spells::RavenHillQuestgiverVision1);
            player->CastSpell(player, Spells::SummonJitters, true);
            player->CastSpell(player, Spells::SummonOliverHarris, true);
            player->SummonCreature(Creatures::LurkingWorgenRavenHill, Positions::LurkingWorgenRavenHillSpawn, TEMPSUMMON_MANUAL_DESPAWN, 0s, 0, 0, player->GetGUID());

            player->m_Events.AddEventAtOffset([player]()
            {
                if (Creature* oliver = player->FindNearestCreature(Creatures::SpawnedOliverHarris, 50.0f))
                    oliver->AI()->SetData(Data::CryForTheMoonStart, 1);

                if (Creature* jitters = player->FindNearestCreature(Creatures::SpawnedJitters, 50.0f))
                    jitters->AI()->SetData(Data::CryForTheMoonStart, 1);

                if (Creature* worgen = player->FindNearestCreature(Creatures::LurkingWorgenRavenHill, 50.0f))
                    worgen->AI()->SetData(Data::CryForTheMoonStart, 1);
            }, 1s);
        }
    };
}

void AddSC_custom_duskwood_quests()
{
    using namespace Scripts::EasternKingdoms::Duskwood;

    new quest_26760_cry_for_the_moon();
}
