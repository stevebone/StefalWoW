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
#include "MotionMaster.h"
#include "Player.h"
#include "Position.h"
#include "TaskScheduler.h"
#include "Unit.h"

#include "Custom_Stormwind_Defines.h"

namespace Scripts::EasternKingdoms::Stormwind
{
    // Helper class for player mapped cooldown
    class PlayerAreaTriggerCooldown
    {
    public:
        PlayerAreaTriggerCooldown() = default;

        bool CanTrigger(Player* player, uint32 areaTriggerId, uint32 cooldownSeconds)
        {
            time_t now = time(nullptr);
            auto& playerMap = lastTrigger[player->GetGUID()];
            time_t& t = playerMap[areaTriggerId];

            if (now - t < cooldownSeconds)
                return false;

            t = now;
            return true;
        }

    private:
        std::unordered_map<ObjectGuid, std::unordered_map<uint32, time_t>> lastTrigger;
    };

    static PlayerAreaTriggerCooldown g_areaTriggerCooldown;

    // 7990 - Stormwind Trade District - Moni and Alyn
    struct at_stormwind_trade_district_7990 : AreaTriggerAI
    {
        at_stormwind_trade_district_7990(AreaTrigger* areatrigger) : AreaTriggerAI(areatrigger) { }

        void OnUnitEnter(Unit* unit) override
        {
            Player* player = unit->ToPlayer();
            if (!player || player->GetQuestStatus(Quests::JoiningTheAlliance) != QUEST_STATUS_COMPLETE)
                return;

            // add cooldown of 45s to prevent spam talk
            if (!g_areaTriggerCooldown.CanTrigger(player, AreaTriggers::StormwindTradeDistrictMoniAlyn, 45))
                return;

            Creature* moni = player->FindNearestCreature(Creatures::MoniWiddlesprock, 50.f);
            if (moni && moni->IsAlive())
            {
                moni->AI()->Talk(0);

                scheduler.Schedule(8s, [moni](TaskContext context)
                    {
                        if (moni && moni->IsAlive())
                            moni->AI()->Talk(1);
                    });
            }

            Creature* alyn = player->FindNearestCreature(Creatures::AlynBlack, 50.f);
            if (alyn && alyn->IsAlive())
            {
                scheduler.Schedule(3s, [alyn](TaskContext context)
                    {
                        if (alyn && alyn->IsAlive())
                            alyn->AI()->Talk(1);
                    });
            }
        }

        void OnUpdate(uint32 diff) override
        {
            scheduler.Update(diff);
        }

    private:
        TaskScheduler scheduler;
    };
}

void AddSC_custom_stormwind_at()
{
    using namespace Scripts::EasternKingdoms::Stormwind;

    RegisterAreaTriggerAI(at_stormwind_trade_district_7990);
    
}
