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

#include "DB2Stores.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "TemporarySummon.h"

#include "Custom_RedridgeMountains_Defines.h"

namespace Scripts::EasternKingdoms::RedridgeMountains
{
    // 682 - Lakeshire Inn - Summon Keeshan Riverboat
    class at_lakeshire_inn_682 : public AreaTriggerScript
    {
    public:
        at_lakeshire_inn_682() : AreaTriggerScript("at_lakeshire_inn_682") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/) override
        {
            if (player->GetQuestStatus(Quests::ItsNeverOver) != QUEST_STATUS_INCOMPLETE)
                return false;

            std::list<TempSummon*> minions;
            player->GetAllMinionsByEntry(minions, Creatures::KeeshanRiverboat);
            if (!minions.empty())
                return false;

            player->CastSpell(player, Spells::SummonKeeshanRiverboat, true);
            return true;
        }
    };
}

void AddSC_custom_redridge_mountains_at()
{
    using namespace Scripts::EasternKingdoms::RedridgeMountains;

    new at_lakeshire_inn_682();
}
