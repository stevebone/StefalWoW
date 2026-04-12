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
#include "MotionMaster.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "TemporarySummon.h"

#include "Custom_TheWanderingIsle_Defines.h"

using namespace Scripts::Custom::TheWanderingIsle;

namespace Scripts::Custom::TheWanderingIsle
{
    // 7750
    class at_talk_on_huo_follow_quest_29423 : public AreaTriggerScript
    {
    public:
        at_talk_on_huo_follow_quest_29423() : AreaTriggerScript("at_talk_on_huo_follow_quest_29423") {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/) override
        {
            if (player->GetQuestStatus(Quests::quest_the_passion_of_shen_zin_su) == QUEST_STATUS_INCOMPLETE)
            {
                Creature* chia = player->FindNearestCreatureWithOptions(30.0f, { .StringId = "Chia_Hui_Talk_Event_Starter", .IgnorePhases = true });

                if (!chia)
                    return false;

                player->CastSpell(chia, SpellsQ29423::spell_start_talk_event);
            }
            return true;
        }
    };    

    // 7784
    class at_the_singing_pools_children_summon : public AreaTriggerScript
    {
    public:
        at_the_singing_pools_children_summon() : AreaTriggerScript("at_the_singing_pools_children_summon") {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/) override
        {
            if (!g_singingPoolsMemory.CanTrigger(player))
                return false;

            if (player->GetQuestStatus(Quests::quest_the_singing_pools) == QUEST_STATUS_COMPLETE)
                player->CastSpell(player, SpellsQ29521::SummonChild1);

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
            if (player->GetQuestStatus(Quests::quest_the_passion_of_shen_zin_su) == QUEST_STATUS_INCOMPLETE)
            {
                Creature* shanxi = player->FindNearestCreatureWithOptions(30.0f, { .StringId = "ShanXi_Talk", .IgnorePhases = true });

                if (!shanxi)
                    return false;

                shanxi->AI()->Talk(TalksQ29423::shanxi_talk_0);

                player->KilledMonsterCredit(Npcs::credit_the_passion_of_shen_zin_su);

                TempSummon* aysa = player->SummonCreature(Npcs::npc_aysa_q29423, PositionsQ29423::aysaSpawnPos, TEMPSUMMON_MANUAL_DESPAWN, 0s, 0, 0, player->GetGUID());
                TempSummon* ji = player->SummonCreature(Npcs::npc_ji_q29423, PositionsQ29423::jiSpawnPos, TEMPSUMMON_MANUAL_DESPAWN, 0s, 0, 0, player->GetGUID());

                if (!aysa || !ji)
                    return false;

                aysa->SetSpeed(MOVE_WALK, 6.0f);
                ji->SetSpeed(MOVE_WALK, 6.0f);
                aysa->GetMotionMaster()->MovePath(PathQ29423::aysa, false);
                ji->GetMotionMaster()->MovePath(PathQ29423::ji, false);
            }
            else if (player->GetQuestStatus(Quests::quest_the_passion_of_shen_zin_su) == QUEST_STATUS_COMPLETE)
            {
                TempSummon* aysa = player->SummonCreature(Npcs::npc_aysa_q29423, PositionsQ29423::aysaSpawnPos, TEMPSUMMON_MANUAL_DESPAWN, 0s, 0, 0, player->GetGUID());
                TempSummon* ji = player->SummonCreature(Npcs::npc_ji_q29423, PositionsQ29423::jiSpawnPos, TEMPSUMMON_MANUAL_DESPAWN, 0s, 0, 0, player->GetGUID());

                if (!aysa || !ji)
                    return false;

                aysa->SetSpeed(MOVE_WALK, 6.0f);
                ji->SetSpeed(MOVE_WALK, 6.0f);
                aysa->GetMotionMaster()->MovePath(PathQ29423::aysa, false);
                ji->GetMotionMaster()->MovePath(PathQ29423::ji, false);
            }
            return true;
        }
    };

    // 8628
    class at_singing_pools_training_bell : public AreaTriggerScript
    {
    public:
        at_singing_pools_training_bell() : AreaTriggerScript("at_singing_pools_training_bell") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/) override
        {
            if (player->IsAlive())
            {
                player->AddAura(SpellsQ29661Q29663::spell_training_bell_exclusion_aura, player);
                return true;
            }
            return false;
        }

        bool OnExit(Player* player, AreaTriggerEntry const* /*areaTrigger*/) override
        {
            if (player->IsAlive())
            {
                player->RemoveAura(SpellsQ29661Q29663::spell_training_bell_exclusion_aura);
                return true;
            }
            return false;
        }
    };

    // 7783
    class at_pools_of_reflection : public AreaTriggerScript
    {
    public:
        at_pools_of_reflection() : AreaTriggerScript("at_pools_of_reflection") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/) override
        {
            if (player->IsInWorld() && player->GetQuestStatus(Quests::quest_shu_the_spirit_of_water) == QUEST_STATUS_INCOMPLETE)
            {
                player->KilledMonsterCredit(Npcs::credit_shu_the_spirit_of_water);
                return true;
            }
            return false;
        }
    };

    // 7258
    // 7822
    class at_cart_locations : public AreaTriggerScript
    {
    public:
        at_cart_locations() : AreaTriggerScript("at_cart_locations") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* areaTrigger) override
        {
            if (!g_singingPoolsMemory.CanTrigger(player))
                return false;

            Creature* cartTender = player->FindNearestCreature(Npcs::npc_cart_tender, 30.0f);
            if (!cartTender || !player->IsAlive())
                return false;

            switch (areaTrigger->ID)
            {
            case AreaTriggers::areaTrigger_singing_pools_cart:
                cartTender->AI()->Talk(TalksCartTender::Cart_Tender_Talk_0);
                return true;
            case AreaTriggers::areaTrigger_farmstead_cart:
                cartTender->AI()->Talk(TalksCartTender::Cart_Tender_Talk_1);
                return true;
            default: return false;
            }
            return false;
        }
    };

    // 7116
    class at_temple_stairs_from_farmstead : public AreaTriggerScript
    {
    public:
        at_temple_stairs_from_farmstead() : AreaTriggerScript("at_temple_stairs_from_farmstead") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/) override
        {
            std::list<TempSummon*> summonShu;
            std::list<TempSummon*> summonWugou;

            player->GetAllMinionsByEntry(summonShu, Npcs::npc_shu_q29775);
            player->GetAllMinionsByEntry(summonWugou, Npcs::npc_wugou_q29775);

            if (summonShu.empty() && summonWugou.empty())
                return false;

            for (TempSummon* shu : summonShu)
            {
                if (!shu || !shu->IsInWorld())
                    continue;

                for (TempSummon* wugou : summonWugou)
                {
                    if (!wugou || !wugou->IsInWorld())
                        continue;

                    wugou->GetMotionMaster()->Clear();
                    wugou->GetMotionMaster()->MoveFollow(shu, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
                    wugou->DespawnOrUnsummon(35s);
                }

                shu->GetMotionMaster()->Clear();
                shu->LoadPath(PathQ29775::path_shu_temple_stairs);
                shu->GetMotionMaster()->MovePath(PathQ29775::path_shu_temple_stairs, false);
                shu->DespawnOrUnsummon(35s);

                return true;
            }

            return false;
        }
    };

    // Area Trigger 8276
    // Zhaoren Temple Spire bridge Flyby
    // Player needs to have quest 29776 and phase 1429
    class at_temple_of_five_dawns_summon_zhaoren : public AreaTriggerScript
    {
    public:
        at_temple_of_five_dawns_summon_zhaoren() : AreaTriggerScript("at_temple_of_five_dawns_summon_zhaoren") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/) override
        {
            if (player->IsAlive() && player->IsActiveQuest(Quests::quest_morning_breeze_village))
            {
                Creature* zhaoSpawn = player->FindNearestCreatureWithOptions(500.f, { .CreatureId = Npcs::npc_zhaoren_flying_temple_spire, .PrivateObjectOwnerGuid = player->GetGUID() });
                if (zhaoSpawn)
                    return false;

                TempSummon* Zhao = player->SummonCreature(Npcs::npc_zhaoren_flying_temple_spire, PositionsQ29776::ZhaoSkySpawn, TEMPSUMMON_TIMED_DESPAWN, 240s,
                    0, 0, player->GetGUID());
                if (Zhao)
                {
                    Zhao->setActive(true);
                    Zhao->SetFarVisible(true);
                    Zhao->SetCanFly(true);
                    Zhao->StopMoving();
                    Zhao->GetMotionMaster()->Clear();
                    Zhao->SetWalk(false);
                    Zhao->SetSpeed(MOVE_RUN, 7.0f);
                    Zhao->SetSpeed(MOVE_FLIGHT, 7.0f);
                    PhasingHandler::AddPhase(Zhao, 1429, true);
                    Zhao->LoadPath(PathZhaoren::path_zhaoren_at_temple);
                    Zhao->GetMotionMaster()->MovePath(PathZhaoren::path_zhaoren_at_temple, false);
                }

                return true;
            }
            return false;
        }
    };

    // 7041
    class at_chamber_of_whispers_outside : public AreaTriggerScript
    {
    public:
        at_chamber_of_whispers_outside() : AreaTriggerScript("at_chamber_of_whispers_outside") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/) override
        {
            if (player->IsAlive() && player->IsActiveQuest(Quests::quest_da_feng_the_spirit_of_air))
            {
                if (player->GetQuestStatus(Quests::quest_da_feng_the_spirit_of_air) != QUEST_STATUS_INCOMPLETE)
                    return false;

                player->CastSpell(player, SpellsZhaorenEvent::spell_summon_aysa_outside_chambers, true);

                return true;
            }
            return false;
        }
    };

    // 7037
    class at_chamber_of_whispers : public AreaTriggerScript
    {
    public:
        at_chamber_of_whispers() : AreaTriggerScript("at_chamber_of_whispers") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/) override
        {
            if (player->IsAlive() && player->IsActiveQuest(Quests::quest_da_feng_the_spirit_of_air))
            {
                Creature* aysha = GetClosestCreatureWithEntry(player, Npcs::npc_aysa_q29785, 20.0f);

                if (aysha)
                    aysha->AI()->Talk(TalksZhaorenEvent::aysa_chamber_of_whispers_0);

                player->KilledMonsterCredit(Npcs::credit_da_feng_the_spirit_of_air);

                return true;
            }
            return false;
        }
    };
}

void AddSC_custom_the_wandering_isle_at()
{
    using namespace Scripts::Custom::TheWanderingIsle;

    new at_talk_on_huo_follow_quest_29423();
    new at_enter_temple_quest_29423();
    new at_the_singing_pools_children_summon();
    new at_singing_pools_training_bell();
    new at_pools_of_reflection();
    new at_cart_locations();
    new at_temple_stairs_from_farmstead();
    new at_temple_of_five_dawns_summon_zhaoren();
    new at_chamber_of_whispers_outside();
    new at_chamber_of_whispers();
}
