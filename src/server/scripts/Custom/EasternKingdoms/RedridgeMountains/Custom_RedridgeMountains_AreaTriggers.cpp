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
#include "DB2Stores.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "TemporarySummon.h"

#include "Custom_RedridgeMountains_Defines.h"

namespace Scripts::EasternKingdoms::RedridgeMountains
{
    // Helper class for player mapped cooldown
    class PlayerAreaTriggerCooldown
    {
    public:
        PlayerAreaTriggerCooldown() = default;

        template<class Rep, class Period>
        bool CanTrigger(Player* player, uint32 areaTriggerId, std::chrono::duration<Rep, Period> cooldown)
        {
            using namespace std::chrono;

            auto now = steady_clock::now();
            auto& playerMap = lastTrigger[player->GetGUID()];
            auto& last = playerMap[areaTriggerId];

            if (now - last < cooldown)
                return false;

            last = now;
            return true;
        }

    private:
        std::unordered_map<ObjectGuid, std::unordered_map<uint32, std::chrono::steady_clock::time_point>> lastTrigger;
    };

    static PlayerAreaTriggerCooldown g_areaTriggerCooldown;

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

    // 6079 - Camp Everstill - Keeshan & Danforth Talks
    class at_camp_everstill_6079 : public AreaTriggerScript
    {
    public:
        at_camp_everstill_6079() : AreaTriggerScript("at_camp_everstill_6079") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/) override
        {
            if (player->GetQuestStatus(Quests::ItsNeverOver) != QUEST_STATUS_COMPLETE)
                return false;

            if (!g_areaTriggerCooldown.CanTrigger(player, AreaTriggers::CampEverstill, 2min))
                return false;

            if (Creature* keeshan = player->FindNearestCreature(Creatures::KeeshanCamp, 20.f))
                keeshan->AI()->Talk(0, player);

            if (Creature* danforth = player->FindNearestCreature(Creatures::DanforthCamp, 20.f))
            {
                ObjectGuid playerGuid = player->GetGUID();
                danforth->m_Events.AddEventAtOffset([danforth, playerGuid]()
                {
                    if (Player* player = ObjectAccessor::GetPlayer(*danforth, playerGuid))
                        danforth->AI()->Talk(0, player);
                }, 3s);
            }

            return true;
        }
    };
}

void AddSC_custom_redridge_mountains_at()
{
    using namespace Scripts::EasternKingdoms::RedridgeMountains;

    new at_lakeshire_inn_682();
    new at_camp_everstill_6079();
}
