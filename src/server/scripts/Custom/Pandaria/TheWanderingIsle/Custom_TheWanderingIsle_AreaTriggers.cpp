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
#include "MotionMaster.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "TemporarySummon.h"

#include "Custom_TheWanderingIsle_Defines.h"

namespace Scripts::TheWanderingIsle::ATScripts
{
    // 7750
    class at_talk_on_huo_follow_quest_29423 : public AreaTriggerScript
    {
    public:
        at_talk_on_huo_follow_quest_29423() : AreaTriggerScript("at_talk_on_huo_follow_quest_29423") {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/) override
        {
            if (player->GetQuestStatus(Defines::Quests::quest_the_passion_of_shen_zin_su) == QUEST_STATUS_INCOMPLETE)
            {
                Creature* chia = player->FindNearestCreatureWithOptions(30.0f, { .StringId = "Chia_Hui_Talk_Event_Starter", .IgnorePhases = true });

                if (!chia)
                    return false;

                player->CastSpell(chia, Defines::SpellsQ29423::spell_start_talk_event);
            }
            return true;
        }
    };

    // 7835
    class at_enter_temple_quest_29423 : public AreaTriggerScript
    {
    public:
        at_enter_temple_quest_29423() : AreaTriggerScript("at_enter_temple_quest_29423") {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/) override
        {
            if (player->GetQuestStatus(Defines::Quests::quest_the_passion_of_shen_zin_su) == QUEST_STATUS_INCOMPLETE)
            {
                Creature* shanxi = player->FindNearestCreatureWithOptions(30.0f, { .StringId = "ShanXi_Talk", .IgnorePhases = true });

                if (!shanxi)
                    return false;

                shanxi->AI()->Talk(Defines::TalksQ29423::shanxi_talk_0);

                player->KilledMonsterCredit(Defines::Npcs::credit_the_passion_of_shen_zin_su);

                TempSummon* aysa = player->SummonCreature(Defines::Npcs::npc_aysa_q29423, Defines::PositionsQ29423::aysaSpawnPos, TEMPSUMMON_MANUAL_DESPAWN, 0s, 0, 0, player->GetGUID());
                TempSummon* ji = player->SummonCreature(Defines::Npcs::npc_ji_q29423, Defines::PositionsQ29423::jiSpawnPos, TEMPSUMMON_MANUAL_DESPAWN, 0s, 0, 0, player->GetGUID());

                if (!aysa || !ji)
                    return false;

                aysa->SetSpeed(MOVE_WALK, 6.0f);
                ji->SetSpeed(MOVE_WALK, 6.0f);
                aysa->GetMotionMaster()->MovePath(Defines::PathQ29423::aysa, false);
                ji->GetMotionMaster()->MovePath(Defines::PathQ29423::ji, false);
            }
            else if (player->GetQuestStatus(Defines::Quests::quest_the_passion_of_shen_zin_su) == QUEST_STATUS_COMPLETE)
            {
                TempSummon* aysa = player->SummonCreature(Defines::Npcs::npc_aysa_q29423, Defines::PositionsQ29423::aysaSpawnPos, TEMPSUMMON_MANUAL_DESPAWN, 0s, 0, 0, player->GetGUID());
                TempSummon* ji = player->SummonCreature(Defines::Npcs::npc_ji_q29423, Defines::PositionsQ29423::jiSpawnPos, TEMPSUMMON_MANUAL_DESPAWN, 0s, 0, 0, player->GetGUID());

                if (!aysa || !ji)
                    return false;

                aysa->SetSpeed(MOVE_WALK, 6.0f);
                ji->SetSpeed(MOVE_WALK, 6.0f);
                aysa->GetMotionMaster()->MovePath(Defines::PathQ29423::aysa, false);
                ji->GetMotionMaster()->MovePath(Defines::PathQ29423::ji, false);
            }
            return true;
        }
    };
}

void AddSC_custom_the_wandering_isle_at()
{
    using namespace Scripts::TheWanderingIsle::ATScripts;
    new at_talk_on_huo_follow_quest_29423();
    new at_enter_temple_quest_29423();
}
