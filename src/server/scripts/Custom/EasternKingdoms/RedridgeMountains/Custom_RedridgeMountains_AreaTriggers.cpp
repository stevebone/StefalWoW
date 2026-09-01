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
#include "CreatureAI.h"
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

            ObjectGuid playerGuid = player->GetGUID();

            if (Creature* keeshan = player->FindNearestCreature(Creatures::KeeshanCamp, 10.f))
            {
                keeshan->AI()->Talk(0, player);

                keeshan->m_Events.AddEventAtOffset([keeshan, playerGuid]()
                    {
                        if (Player* player = ObjectAccessor::GetPlayer(*keeshan, playerGuid))
                            keeshan->AI()->Talk(1, player);
                    }, 8s);

                keeshan->m_Events.AddEventAtOffset([keeshan, playerGuid]()
                    {
                        if (Player* player = ObjectAccessor::GetPlayer(*keeshan, playerGuid))
                            keeshan->AI()->Talk(2, player);
                    }, 21s);

                keeshan->m_Events.AddEventAtOffset([keeshan, playerGuid]()
                    {
                        if (Player* player = ObjectAccessor::GetPlayer(*keeshan, playerGuid))
                            keeshan->AI()->Talk(3, player);
                    }, 32s);

                keeshan->m_Events.AddEventAtOffset([keeshan, playerGuid]()
                    {
                        if (Player* player = ObjectAccessor::GetPlayer(*keeshan, playerGuid))
                            keeshan->AI()->Talk(4, player);
                    }, 42s);

                keeshan->m_Events.AddEventAtOffset([keeshan, playerGuid]()
                    {
                        if (Player* player = ObjectAccessor::GetPlayer(*keeshan, playerGuid))
                            keeshan->AI()->Talk(5, player);
                    }, 47s);
            }

            if (Creature* danforth = player->FindNearestCreature(Creatures::DanforthCamp, 10.f))
            {
                danforth->m_Events.AddEventAtOffset([danforth, playerGuid]()
                {
                    if (Player* player = ObjectAccessor::GetPlayer(*danforth, playerGuid))
                        danforth->AI()->Talk(0, player);
                }, 3s);
            }

            if (Creature* krakauer = player->FindNearestCreature(Creatures::KrakauerCamp, 10.f))
            {
                krakauer->m_Events.AddEventAtOffset([krakauer, playerGuid]()
                    {
                        if (Player* player = ObjectAccessor::GetPlayer(*krakauer, playerGuid))
                            krakauer->AI()->Talk(0, player);
                    }, 11s);

                krakauer->m_Events.AddEventAtOffset([krakauer, playerGuid]()
                    {
                        if (Player* player = ObjectAccessor::GetPlayer(*krakauer, playerGuid))
                            krakauer->AI()->Talk(1, player);
                    }, 16s);
            }

            if (Creature* jorgensen = player->FindNearestCreature(Creatures::JorgensenCamp, 10.f))
            {
                jorgensen->m_Events.AddEventAtOffset([jorgensen, playerGuid]()
                    {
                        if (Player* player = ObjectAccessor::GetPlayer(*jorgensen, playerGuid))
                            jorgensen->AI()->Talk(0, player);
                    }, 25s);
            }

            if (Creature* messner = player->FindNearestCreature(Creatures::MessnerCamp, 10.f))
            {
                messner->m_Events.AddEventAtOffset([messner, playerGuid]()
                    {
                        if (Player* player = ObjectAccessor::GetPlayer(*messner, playerGuid))
                            messner->AI()->Talk(0, player);
                    }, 37s);
            }

            return true;
        }
    };

    // 6082 - Camp Everstill Hill - Jorgensen Guardian Talk 1
    class at_camp_everstill_hill_6082 : public AreaTriggerScript
    {
    public:
        at_camp_everstill_hill_6082() : AreaTriggerScript("at_camp_everstill_hill_6082") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/) override
        {
            std::list<TempSummon*> minions;
            player->GetAllMinionsByEntry(minions, Creatures::JorgensenGuardian);
            for (TempSummon* minion : minions)
                minion->AI()->SetData(1, 1);

            return true;
        }
    };

    // 6083 - Render's Valley Entry - Jorgensen Guardian Talk 2
    class at_renders_valley_entry_6083 : public AreaTriggerScript
    {
    public:
        at_renders_valley_entry_6083() : AreaTriggerScript("at_renders_valley_entry_6083") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/) override
        {
            std::list<TempSummon*> minions;
            player->GetAllMinionsByEntry(minions, Creatures::JorgensenGuardian);
            for (TempSummon* minion : minions)
                minion->AI()->SetData(1, 2);

            return true;
        }
    };

    // 6084 - Render's Valley Cavern - Jorgensen Guardian Talk 4
    class at_renders_valley_cavern_6084 : public AreaTriggerScript
    {
    public:
        at_renders_valley_cavern_6084() : AreaTriggerScript("at_renders_valley_cavern_6084") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/) override
        {
            std::list<TempSummon*> minions;
            player->GetAllMinionsByEntry(minions, Creatures::JorgensenGuardian);
            for (TempSummon* minion : minions)
                minion->AI()->SetData(1, 4);

            return true;
        }
    };
}

void AddSC_custom_redridge_mountains_at()
{
    using namespace Scripts::EasternKingdoms::RedridgeMountains;

    new at_lakeshire_inn_682();
    new at_camp_everstill_6079();
    new at_camp_everstill_hill_6082();
    new at_renders_valley_entry_6083();
    new at_renders_valley_cavern_6084();
}
