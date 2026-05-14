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

#include "AreaTriggerAI.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "DB2Stores.h"
#include "Map.h"
#include "MotionMaster.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "Position.h"
#include "ScriptMgr.h"
#include "TaskScheduler.h"
#include "Unit.h"

#include "Custom_Stormwind_Defines.h"

namespace Scripts::EasternKingdoms::StormwindCity
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

            // If this is the first time, last == time_point{} ? always allow
            if (now - last < cooldown)
                return false;

            last = now;
            return true;
        }

    private:
        std::unordered_map<ObjectGuid, std::unordered_map<uint32, std::chrono::steady_clock::time_point>> lastTrigger;
    };

    static PlayerAreaTriggerCooldown g_areaTriggerCooldown;

    // 5828 - Valley of Heroes - General Marcus Jonathan
    class at_stormwind_valley_of_heroes_5828 : public AreaTriggerScript
    {
    public:
        at_stormwind_valley_of_heroes_5828() : AreaTriggerScript("at_stormwind_valley_of_heroes_5828") {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* areaTrigger) override
        {
            if (player->GetQuestStatus(Quests::JoiningTheAlliance) == QUEST_STATUS_COMPLETE)
            {
                // add cooldown to prevent spam talk
                if (!g_areaTriggerCooldown.CanTrigger(player, areaTrigger->ID, 2min))
                    return false;

                Creature* marcus = player->FindNearestCreature(Creatures::GeneralMarcusJonathan, 50.f);
                if (marcus && marcus->IsAlive())
                {
                    marcus->AI()->Talk(1);

                    marcus->m_Events.AddEventAtOffset([marcus]()
                        {
                            if(marcus && marcus->IsAlive())
                                marcus->AI()->Talk(2);
                        }, std::chrono::seconds(10));

                    marcus->m_Events.AddEventAtOffset([marcus]()
                        {
                            if (marcus && marcus->IsAlive())
                                marcus->AI()->Talk(3);
                        }, std::chrono::seconds(12));

                    marcus->m_Events.AddEventAtOffset([marcus]()
                        {
                            if (marcus && marcus->IsAlive())
                                marcus->AI()->Talk(4);
                        }, std::chrono::seconds(15));
                }

                Creature* aysa = player->FindNearestCreature(Creatures::AysaSpawnedAtGate, 50.f);
                if (aysa && aysa->IsAlive())
                {
                    aysa->GetMotionMaster()->Clear();
                    aysa->GetMotionMaster()->MovePoint(2, Positions::AysaStormwindCityGeneralMarcus);

                    aysa->m_Events.AddEventAtOffset([aysa]()
                        {
                            if (aysa && aysa->IsAlive())
                            {
                                aysa->AI()->Talk(1);
                            }
                        }, std::chrono::seconds(5));

                    aysa->m_Events.AddEventAtOffset([aysa, player]()
                        {
                            if (aysa && aysa->IsAlive())
                                aysa->GetMotionMaster()->MoveFollow(player, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
                        }, std::chrono::seconds(15));
                }
            }
            return true;
        }
    };

    // 7989 - Stormwind City Entrance - Josie/Marty
    class at_stormwind_entrance_7989 : public AreaTriggerScript
    {
    public:
        at_stormwind_entrance_7989() : AreaTriggerScript("at_stormwind_entrance_7989") {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* areaTrigger) override
        {
            if (player->GetQuestStatus(Quests::JoiningTheAlliance) == QUEST_STATUS_COMPLETE)
            {
                // add cooldown of 45s to prevent spam talk
                if (!g_areaTriggerCooldown.CanTrigger(player, areaTrigger->ID, 45s))
                    return false;

                Creature* marty = player->FindNearestCreature(Creatures::Marty, 50.f);
                if (marty && marty->IsAlive())
                    marty->AI()->Talk(0);

                Creature* josie = player->FindNearestCreature(Creatures::Josie, 50.f);
                if (josie && josie->IsAlive())
                {
                    josie->m_Events.AddEventAtOffset([josie]()
                        {
                            if (josie && josie->IsAlive())
                                josie->AI()->Talk(0);
                        }, std::chrono::seconds(5));

                    josie->m_Events.AddEventAtOffset([josie]()
                        {
                            if (josie && josie->IsAlive())
                                josie->AI()->Talk(1);
                        }, std::chrono::seconds(8));
                }
            }
            return true;
        }
    };

    // 7995 - Valley of Heroes - Stormwind City Guard
    class at_stormwind_valley_of_heroes_7995 : public AreaTriggerScript
    {
    public:
        at_stormwind_valley_of_heroes_7995() : AreaTriggerScript("at_stormwind_valley_of_heroes_7995") {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* areaTrigger) override
        {
            if (player->GetQuestStatus(Quests::JoiningTheAlliance) == QUEST_STATUS_COMPLETE)
            {
                // add cooldown to prevent spam talk
                if (!g_areaTriggerCooldown.CanTrigger(player, areaTrigger->ID, 2min))
                    return false;

                Creature* guard = player->GetMap()->GetCreatureBySpawnId(Spawns::StormwindCityGuardOnBridge);
                if (guard && guard->IsAlive())
                    guard->AI()->Talk(6);
            }
            return true;
        }
    };

    // 7996 - Stormwind City Trade District Center
    class at_stormwind_trade_district_center_7996 : public AreaTriggerScript
    {
    public:
        at_stormwind_trade_district_center_7996() : AreaTriggerScript("at_stormwind_trade_district_center_7996") {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* areaTrigger) override
        {
            if (player->GetQuestStatus(Quests::JoiningTheAlliance) == QUEST_STATUS_COMPLETE)
            {
                // add cooldown to prevent spam talk
                if (!g_areaTriggerCooldown.CanTrigger(player, areaTrigger->ID, 2min))
                    return false;

                Creature* aysa = player->FindNearestCreature(Creatures::AysaSpawnedAtGate, 50.f);
                if (aysa && aysa->IsAlive())
                {
                    aysa->AI()->Talk(2);
                    aysa->GetMotionMaster()->Clear();
                    aysa->GetMotionMaster()->MovePoint(1, Positions::AysaStormwindCityCenter);
                    aysa->m_Events.AddEventAtOffset([aysa, player]()
                        {
                            if (aysa && aysa->IsAlive())
                            {
                                aysa->AI()->Talk(3);
                                aysa->GetMotionMaster()->MoveFollow(player, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
                            }
                        }, std::chrono::seconds(15));
                }
            }
            return true;
        }
    };

    // 7990 - Stormwind Trade District - Moni and Alyn
    class at_stormwind_trade_district_7990 : public AreaTriggerScript
    {
    public:
        at_stormwind_trade_district_7990() : AreaTriggerScript("at_stormwind_trade_district_7990") {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* areaTrigger) override
        {
            if (player->GetQuestStatus(Quests::JoiningTheAlliance) == QUEST_STATUS_COMPLETE)
            {
                // add cooldown of 45s to prevent spam talk
                if (!g_areaTriggerCooldown.CanTrigger(player, areaTrigger->ID, 45s))
                    return false;

                Creature* moni = player->FindNearestCreature(Creatures::MoniWiddlesprock, 50.f);
                if (moni && moni->IsAlive())
                {
                    moni->AI()->Talk(0);
                    moni->m_Events.AddEventAtOffset([moni]()
                        {
                            if (moni && moni->IsAlive())
                                moni->AI()->Talk(1);
                        }, std::chrono::seconds(10));
                }

                Creature* alyn = player->FindNearestCreature(Creatures::AlynBlack, 50.f);
                if (alyn && alyn->IsAlive())
                {
                    alyn->m_Events.AddEventAtOffset([alyn]()
                        {
                            if (alyn && alyn->IsAlive())
                                alyn->AI()->Talk(0);
                        }, std::chrono::seconds(5));
                }
            }
            return true;
        }
    };

    // 7991 - Stormwind Canals - Gavin
    class at_stormwind_canals_7991 : public AreaTriggerScript
    {
    public:
        at_stormwind_canals_7991() : AreaTriggerScript("at_stormwind_canals_7991") {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* areaTrigger) override
        {
            if (player->GetQuestStatus(Quests::JoiningTheAlliance) == QUEST_STATUS_COMPLETE)
            {
                // add cooldown of 45s to prevent spam talk
                if (!g_areaTriggerCooldown.CanTrigger(player, areaTrigger->ID, 45s))
                    return false;

                Creature* gavin = player->FindNearestCreature(Creatures::GavinMarlsbury, 50.f);
                if (gavin && gavin->IsAlive())
                {
                    gavin->SetFacingToObject(player, true);
                    gavin->RemoveAurasDueToSpell(Spells::Stealth);
                    gavin->AI()->Talk(0);
                    gavin->m_Events.AddEventAtOffset([gavin]()
                        {
                            if (gavin && gavin->IsAlive())
                                gavin->AI()->Talk(1);
                        }, std::chrono::seconds(5));

                    gavin->m_Events.AddEventAtOffset([gavin]()
                        {
                            if (gavin && gavin->IsAlive())
                                gavin->CastSpell(gavin, Spells::Stealth);
                        }, std::chrono::seconds(35));
                }

                Creature* jojo = player->FindNearestCreature(Creatures::JojoSpawnedAtGate, 50.f);
                if (jojo && jojo->IsAlive())
                {
                    jojo->m_Events.AddEventAtOffset([jojo]()
                        {
                            if (jojo && jojo->IsAlive())
                                jojo->AI()->Talk(0);
                        }, std::chrono::seconds(10));
                }
            }
            return true;
        }
    };

    // 7993 - Stormwind Canals - Leria Nightwind
    class at_stormwind_canals_7993 : public AreaTriggerScript
    {
    public:
        at_stormwind_canals_7993() : AreaTriggerScript("at_stormwind_canals_7993") {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* areaTrigger) override
        {
            if (player->GetQuestStatus(Quests::JoiningTheAlliance) == QUEST_STATUS_COMPLETE)
            {
                // add cooldown of 45s to prevent spam talk
                if (!g_areaTriggerCooldown.CanTrigger(player, areaTrigger->ID, 45s))
                    return false;

                Creature* leria = player->FindNearestCreature(Creatures::LeriaNightwind, 50.f);
                if (leria && leria->IsAlive())
                    leria->AI()->Talk(0);
            }
            return true;
        }
    };

    // 7994 - Stormwind Canals - Brunn Goldenmug
    class at_stormwind_canals_7994 : public AreaTriggerScript
    {
    public:
        at_stormwind_canals_7994() : AreaTriggerScript("at_stormwind_canals_7994") {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* areaTrigger) override
        {
            if (player->GetQuestStatus(Quests::JoiningTheAlliance) == QUEST_STATUS_COMPLETE)
            {
                // add cooldown of 45s to prevent spam talk
                if (!g_areaTriggerCooldown.CanTrigger(player, areaTrigger->ID, 45s))
                    return false;

                Creature* brunn = player->FindNearestCreature(Creatures::BrunnGoldenmug, 50.f);
                if (brunn && brunn->IsAlive())
                    brunn->AI()->Talk(0);
            }
            return true;
        }
    };

    // 7992 - Stormwind Keep - Stairs/Fountain
    class at_stormwind_keep_7992 : public AreaTriggerScript
    {
    public:
        at_stormwind_keep_7992() : AreaTriggerScript("at_stormwind_keep_7992") {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* areaTrigger) override
        {
            if (player->GetQuestStatus(Quests::JoiningTheAlliance) == QUEST_STATUS_COMPLETE)
            {
                // add cooldown of 45s to prevent spam talk
                if (!g_areaTriggerCooldown.CanTrigger(player, areaTrigger->ID, 45s))
                    return false;

                Creature* lucas = player->FindNearestCreature(Creatures::LucasSevering, 50.f);
                if (lucas && lucas->IsAlive())
                    lucas->AI()->Talk(0);

                Creature* naanae = player->FindNearestCreature(Creatures::Naanae, 50.f);
                if (naanae && naanae->IsAlive())
                {
                    naanae->m_Events.AddEventAtOffset([naanae]()
                        {
                            if (naanae && naanae->IsAlive())
                                naanae->AI()->Talk(0);
                        }, std::chrono::seconds(5));

                    naanae->m_Events.AddEventAtOffset([naanae]()
                        {
                            if (naanae && naanae->IsAlive())
                                naanae->AI()->Talk(1);
                        }, std::chrono::seconds(10));
                }

                Creature* aysa = player->FindNearestCreature(Creatures::AysaSpawnedAtGate, 50.f);
                if (aysa && aysa->IsAlive())
                {
                    aysa->m_Events.AddEventAtOffset([aysa]()
                        {
                            if (aysa && aysa->IsAlive())
                                aysa->AI()->Talk(4);
                        }, std::chrono::seconds(20));
                }

                Creature* jojo = player->FindNearestCreature(Creatures::JojoSpawnedAtGate, 50.f);
                if (jojo && jojo->IsAlive())
                {
                    jojo->m_Events.AddEventAtOffset([jojo]()
                        {
                            if (jojo && jojo->IsAlive())
                                jojo->AI()->Talk(1);
                        }, std::chrono::seconds(25));
                }
            }
            return true;
        }
    };
}

void AddSC_custom_stormwind_at()
{
    using namespace Scripts::EasternKingdoms::StormwindCity;

    new at_stormwind_valley_of_heroes_5828();
    new at_stormwind_entrance_7989();
    new at_stormwind_trade_district_7990();
    new at_stormwind_canals_7991();
    new at_stormwind_keep_7992();
    new at_stormwind_canals_7993();
    new at_stormwind_canals_7994();
    new at_stormwind_valley_of_heroes_7995();
    new at_stormwind_trade_district_center_7996();
}
