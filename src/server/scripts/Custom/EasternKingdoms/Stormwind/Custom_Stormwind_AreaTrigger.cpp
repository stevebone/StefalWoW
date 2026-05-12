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
#include "MotionMaster.h"
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
    class at_stormwind_trade_district_7990 : public AreaTriggerScript
    {
    public:
        at_stormwind_trade_district_7990() : AreaTriggerScript("at_stormwind_trade_district_7990") {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* areaTrigger) override
        {
            if (player->GetQuestStatus(Quests::JoiningTheAlliance) == QUEST_STATUS_COMPLETE)
            {
                // add cooldown of 45s to prevent spam talk
                if (!g_areaTriggerCooldown.CanTrigger(player, areaTrigger->ID, 45))
                    return false;

                Creature* moni = player->FindNearestCreature(Creatures::MoniWiddlesprock, 50.f);
                if (moni && moni->IsAlive())
                    moni->AI()->SetData(1, 1);

                Creature* alyn = player->FindNearestCreature(Creatures::AlynBlack, 50.f);
                if (alyn && alyn->IsAlive())
                    alyn->AI()->SetData(1, 1);
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
                if (!g_areaTriggerCooldown.CanTrigger(player, areaTrigger->ID, 45))
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
                if (!g_areaTriggerCooldown.CanTrigger(player, areaTrigger->ID, 45))
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
                if (!g_areaTriggerCooldown.CanTrigger(player, areaTrigger->ID, 45))
                    return false;

                Creature* lucas = player->FindNearestCreature(Creatures::LucasSevering, 50.f);
                if (lucas && lucas->IsAlive())
                    lucas->AI()->Talk(0);

                Creature* naanae = player->FindNearestCreature(Creatures::Naanae, 50.f);
                if (naanae && naanae->IsAlive())
                    naanae->AI()->SetData(1, 1);
            }
            return true;
        }
    };
}

void AddSC_custom_stormwind_at()
{
    using namespace Scripts::EasternKingdoms::StormwindCity;

    new at_stormwind_trade_district_7990();
    new at_stormwind_keep_7992();
    new at_stormwind_canals_7993();
    new at_stormwind_canals_7994();
    
}
