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
#include "MotionMaster.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "QuestDef.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "TaskScheduler.h"
#include "TemporarySummon.h"

#include "Custom_Westfall_Defines.h"

namespace Scripts::EasternKingdoms::Westfall
{
    // 26297 - The Dawning of a new day
    class quest_26297_the_dawning_of_a_new_day : public QuestScript
    {
    public:
        quest_26297_the_dawning_of_a_new_day() : QuestScript("quest_26297_the_dawning_of_a_new_day") { }

        void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
        {
            if (!player)
                return;

            if (newStatus != QUEST_STATUS_INCOMPLETE)
                return;

            player->CastSpell(player, Spells::SummonShadowyFigureMoonbrook);

            SummonCrowd(player);
        }
    };

    // 26322 - Rise of the brotherhood
    class quest_26322_rise_of_the_brotherhood : public QuestScript
    {
    public:
        quest_26322_rise_of_the_brotherhood() : QuestScript("quest_26322_rise_of_the_brotherhood") { }

        void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
        {
            if (!player)
                return;

            if (newStatus != QUEST_STATUS_INCOMPLETE)
                return;

            _playerGuid = player->GetGUID();
            PhasingHandler::AddPhase(player, Phases::WestfallAct2, true);

            SummonSpellNPCS(player);

            if (Creature* gryan = player->FindNearestCreature(Creatures::SpawnedGryanStoutmantleAtTower, 20.f))
                gryan->AI()->SetGUID(player->GetGUID(), 1);
            //SummonBrotherhood(player);
        }

        void SummonSpellNPCS(Player* player)
        {
            if (!player)
                return;

            for (uint32 spellId : PlayerSummonSpells)
                player->CastSpell(player, spellId, true);
        }

        void SummonBrotherhood(Player* player)
        {
            if (!player)
                return;

            for (auto const& spawn : Brotherhood)
            {
                if (Creature* c = player->SummonCreature(
                    spawn.entry,
                    spawn.pos,
                    TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT,
                    std::chrono::seconds(spawn.despawnTime)))
                {
                    c->SetReactState(REACT_PASSIVE);
                    _brotherhoodGuids.push_back(c->GetGUID());
                }
            }
        }

    private:
        ObjectGuid _playerGuid;
        std::vector<ObjectGuid> _brotherhoodGuids;    
    };
}

void AddSC_custom_westfall_quests()
{
    using namespace Scripts::EasternKingdoms::Westfall;

    new quest_26297_the_dawning_of_a_new_day();
    new quest_26322_rise_of_the_brotherhood();
}
