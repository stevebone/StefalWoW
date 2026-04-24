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
#include "GameObject.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ObjectGuid.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "TemporarySummon.h"
#include "Vehicle.h"

#include "Custom_TheWanderingIsle_Defines.h"

using namespace Scripts::Custom::TheWanderingIsle;

namespace Scripts::Custom::TheWanderingIsle
{
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

                player->CastSpell(chia, Spells::spell_start_talk_event);
            }
            return true;
        }
    };    

    // 7784
    class at_the_singing_pools_children_summon : public AreaTriggerScript
    {
    public:
        at_the_singing_pools_children_summon() : AreaTriggerScript("at_the_singing_pools_children_summon") {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* areaTrigger) override
        {
            // add cooldown of 45s to prevent spam talk
            if (!g_areaTriggerCooldown.CanTrigger(player, areaTrigger->ID, 45))
                return false;

            if (player->GetQuestStatus(Quests::quest_the_singing_pools) == QUEST_STATUS_COMPLETE)
                player->CastSpell(player, Spells::SummonChild1);

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

                TempSummon* aysa = player->SummonCreature(Npcs::npc_aysa_q29423, Positions::aysaSpawnPos, TEMPSUMMON_MANUAL_DESPAWN, 0s, 0, 0, player->GetGUID());
                TempSummon* ji = player->SummonCreature(Npcs::npc_ji_q29423, Positions::jiSpawnPos, TEMPSUMMON_MANUAL_DESPAWN, 0s, 0, 0, player->GetGUID());

                if (!aysa || !ji)
                    return false;

                aysa->SetSpeed(MOVE_WALK, 6.0f);
                ji->SetSpeed(MOVE_WALK, 6.0f);
                aysa->GetMotionMaster()->MovePath(Paths::aysa, false);
                ji->GetMotionMaster()->MovePath(Paths::ji, false);
            }
            else if (player->GetQuestStatus(Quests::quest_the_passion_of_shen_zin_su) == QUEST_STATUS_COMPLETE)
            {
                TempSummon* aysa = player->SummonCreature(Npcs::npc_aysa_q29423, Positions::aysaSpawnPos, TEMPSUMMON_MANUAL_DESPAWN, 0s, 0, 0, player->GetGUID());
                TempSummon* ji = player->SummonCreature(Npcs::npc_ji_q29423, Positions::jiSpawnPos, TEMPSUMMON_MANUAL_DESPAWN, 0s, 0, 0, player->GetGUID());

                if (!aysa || !ji)
                    return false;

                aysa->SetSpeed(MOVE_WALK, 6.0f);
                ji->SetSpeed(MOVE_WALK, 6.0f);
                aysa->GetMotionMaster()->MovePath(Paths::aysa, false);
                ji->GetMotionMaster()->MovePath(Paths::ji, false);
            }
            return true;
        }
    };

    // Area Trigger 8287
    class at_lorewalker_zan : public AreaTriggerScript
    {
    public:
        at_lorewalker_zan() : AreaTriggerScript("at_lorewalker_zan") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* areaTrigger) override
        {
            if (player->IsAlive())
            {
                // add cooldown of 45s to prevent spam talk
                if (!g_areaTriggerCooldown.CanTrigger(player, areaTrigger->ID, 45))
                    return false;

                Creature* zan = GetClosestCreatureWithEntry(player, Npcs::npc_lorewalker_zan, 30.0f);
                if (zan)
                {
                    zan->AI()->Talk(Talks::lorewalker_zan_0, player);

                    Creature* ruolin = zan->FindNearestCreatureWithOptions(100.f, { .CreatureId = Npcs::npc_lorewalker_ruolin, .IgnorePhases = true });
                    if (ruolin)
                        ruolin->AI()->SetData(1, 1);
                }

                return true;
            }
            return false;
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
                player->AddAura(Spells::spell_training_bell_exclusion_aura, player);
                return true;
            }
            return false;
        }

        bool OnExit(Player* player, AreaTriggerEntry const* /*areaTrigger*/) override
        {
            if (player->IsAlive())
            {
                player->RemoveAura(Spells::spell_training_bell_exclusion_aura);
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
            if (player->IsAlive() && player->GetQuestStatus(Quests::quest_shu_the_spirit_of_water) == QUEST_STATUS_INCOMPLETE)
            {
                player->KilledMonsterCredit(Npcs::credit_shu_the_spirit_of_water);
                return true;
            }
            return false;
        }
    };

    // 7258 // 7822 // 7265
    class at_cart_locations : public AreaTriggerScript
    {
    public:
        at_cart_locations() : AreaTriggerScript("at_cart_locations") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* areaTrigger) override
        {
            // add cooldown of 2min to prevent spam talk
            if (!g_areaTriggerCooldown.CanTrigger(player, areaTrigger->ID, 120))
                return false;

            Creature* cartTender = player->FindNearestCreature(Npcs::npc_cart_tender, 30.0f);
            if (!cartTender || !player->IsAlive())
                return false;

            switch (areaTrigger->ID)
            {
            case AreaTriggers::singing_pools_cart:
                cartTender->AI()->Talk(Talks::Cart_Tender_Talk_0);
                return true;
            case AreaTriggers::forbidden_forest_cart:
            case AreaTriggers::farmstead_cart:
                cartTender->AI()->Talk(Talks::Cart_Tender_Talk_1);
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
                shu->LoadPath(Paths::path_shu_temple_stairs);
                shu->GetMotionMaster()->MovePath(Paths::path_shu_temple_stairs, false);
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
                Creature* zhaoSpawn = player->GetMap()->GetCreatureBySpawnId(SpawnIds::zhaoren_flyby);
                if(!zhaoSpawn)
                    zhaoSpawn = player->SummonCreature(Npcs::npc_zhaoren_flying_temple_spire, Positions::ZhaoSkySpawn);

                if (zhaoSpawn)
                {
                    zhaoSpawn->setActive(true);
                    zhaoSpawn->SetFarVisible(true);
                    zhaoSpawn->SetCanFly(true);
                    zhaoSpawn->SetWalk(false);
                    zhaoSpawn->SetSpeed(MOVE_FLIGHT, 11.0f);
                    zhaoSpawn->GetMotionMaster()->MovePath(Paths::path_zhaoren_at_temple, false);
                    zhaoSpawn->DespawnOrUnsummon(120s);
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

        bool OnTrigger(Player* player, AreaTriggerEntry const* areaTrigger) override
        {
            if (player->IsAlive() && player->IsActiveQuest(Quests::quest_da_feng_the_spirit_of_air))
            {
                // add cooldown of 2min to prevent spam summon
                if (!g_areaTriggerCooldown.CanTrigger(player, areaTrigger->ID, 120))
                    return false;

                if (player->GetQuestStatus(Quests::quest_da_feng_the_spirit_of_air) != QUEST_STATUS_INCOMPLETE)
                    return false;

                player->CastSpell(player, Spells::spell_summon_aysa_outside_chambers, true);

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
                    aysha->AI()->Talk(Talks::aysa_chamber_of_whispers_0);

                player->KilledMonsterCredit(Npcs::credit_da_feng_the_spirit_of_air);

                return true;
            }
            return false;
        }
    };

    class BalloonIntroEvent_Ji : public BasicEvent
    {
    public:
        BalloonIntroEvent_Ji(Creature* controller, ObjectGuid creatureGuid)
            : _controller(controller), _creatureGuid(creatureGuid) { }

        bool Execute(uint64 /*time*/, uint32 /*diff*/) override
        {
            if (Creature* ji = ObjectAccessor::GetCreature(*_controller, _creatureGuid))
                ji->AI()->Talk(0); // Ji's line

            return true;
        }

    private:
        Creature* _controller;
        ObjectGuid _creatureGuid;
    };

    class BalloonIntroEvent_AysaReply : public BasicEvent
    {
    public:
        BalloonIntroEvent_AysaReply(Creature* controller, ObjectGuid creatureGuid)
            : _controller(controller), _creatureGuid(creatureGuid) { }

        bool Execute(uint64 /*time*/, uint32 /*diff*/) override
        {
            if (Creature* aysa = ObjectAccessor::GetCreature(*_controller, _creatureGuid))
                aysa->AI()->Talk(1); // Aysa's reply

            return true;
        }

    private:
        Creature* _controller;
        ObjectGuid _creatureGuid;
    };

    class BalloonIntroEvent_AysaHopIn : public BasicEvent
    {
    public:
        BalloonIntroEvent_AysaHopIn(Creature* controller, ObjectGuid creatureGuid)
            : _controller(controller), _creatureGuid(creatureGuid) { }

        bool Execute(uint64 /*time*/, uint32 /*diff*/) override
        {
            if (Creature* aysa = ObjectAccessor::GetCreature(*_controller, _creatureGuid))
                aysa->AI()->Talk(2); // Aysa's reply

            return true;
        }

    private:
        Creature* _controller;
        ObjectGuid _creatureGuid;
    };

    // 7106 - Balloon Intro Trigger
    // Player needs to have quest 29790 and phase 1885
    class at_balloon_intro_q29790 : public AreaTriggerScript
    {
    public:
        at_balloon_intro_q29790() : AreaTriggerScript("at_balloon_intro_q29790") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/) override
        {
            // Check if player has active quest
            if (!player->IsActiveQuest(Quests::quest_passing_wisdom))
                return false;

            // Check if controller already exists (conversation already triggered)
            if (player->FindNearestCreature(Npcs::npc_balloon_arrival_controller, 30.0f))
                return false;

            // Spawn controller as a marker
            Creature* controller = player->SummonCreature(Npcs::npc_balloon_arrival_controller, Positions::BalloonArrivalControllerSpawn);
            if (controller)
            {
                // Find Aysa and Ji
                Creature* aysa = player->FindNearestCreature(Npcs::npc_aysa_q29790, 30.0f);
                Creature* ji = player->FindNearestCreature(Npcs::npc_ji_q29790, 30.0f);
                if (aysa && ji)
                {
                    aysa->AI()->Talk(0, player); // Aysa first line

                    // Schedule Ji's line
                    controller->m_Events.AddEvent(new BalloonIntroEvent_Ji(controller, ji->GetGUID()), controller->m_Events.CalculateTime(6s));

                    // Schedule Aysa's reply after Ji finishes
                    controller->m_Events.AddEvent(new BalloonIntroEvent_AysaReply(controller, aysa->GetGUID()), controller->m_Events.CalculateTime(12s));

                    // Aysa hop in
                    controller->m_Events.AddEvent(new BalloonIntroEvent_AysaHopIn(controller, aysa->GetGUID()), controller->m_Events.CalculateTime(17s));

                    return true;
                }
            }

            return false;
        }
    };

    // 7710
    class at_mandori_village_7710 : public AreaTriggerScript
    {
    public:
        at_mandori_village_7710() : AreaTriggerScript("at_mandori_village_7710") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* areaTrigger) override
        {
            if (!player->IsAlive())
                return false;             

            if (player->IsActiveQuest(Quests::quest_bidden_to_greatness))
            {
                // add cooldown of 2min to prevent spam
                if (!g_areaTriggerCooldown.CanTrigger(player, areaTrigger->ID, 120))
                    return false;

                GameObject* door = GetClosestGameObjectWithEntry(player, Objects::go_forest_door, 50.f);
                Creature* aysha = GetClosestCreatureWithEntry(player, Npcs::npc_aysa_q29792, 30.0f);
                
                if (aysha)
                {
                    aysha->AI()->SetGUID(player->GetGUID(), 1);
                    aysha->AI()->Talk(0, player);
                }

                door->SetFaction(0);
                door->SetGoStateFor(GO_STATE_READY, player);

                return true;
            }
            return false;
        }
    };

    // 7714
    class at_forlorn_hut_7714 : public AreaTriggerScript
    {
    public:
        at_forlorn_hut_7714() : AreaTriggerScript("at_forlorn_hut_7714") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* areaTrigger) override
        {
            if (!player->IsAlive())
                return false;

            if (player->IsActiveQuest(Quests::quest_bidden_to_greatness))
            {
                // add cooldown of 5min to prevent spam
                if (!g_areaTriggerCooldown.CanTrigger(player, areaTrigger->ID, 300))
                    return false;

                Creature* korga = GetClosestCreatureWithEntry(player, Npcs::npc_korga_hut, 50.0f);

                if (korga)
                {
                    korga->AI()->SetData(1, 1);
                    korga->AI()->SetGUID(player->GetGUID(), 1);
                }

                return true;
            }
            return false;
        }
    };

    // 7087
    class at_wreck_of_the_skyseeker_injured_sailor : public AreaTriggerScript
    {
    public:
        at_wreck_of_the_skyseeker_injured_sailor() : AreaTriggerScript("at_wreck_of_the_skyseeker_injured_sailor") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/) override
        {
            if (player->IsAlive() && player->IsVehicle() && player->GetQuestStatus(Quests::quest_none_left_behind) == QUEST_STATUS_INCOMPLETE)
            {
                Vehicle* veh = player->GetVehicleKit();
                if (veh)
                {
                    Unit* sailor = veh->GetPassenger(0);
                    if (sailor)
                    {
                        sailor->SetWalk(true);
                        sailor->CastSpell(player, Spells::spell_cancel_rescue_injured_sailor, true);
                        sailor->SetStandState(UNIT_STAND_STATE_STAND);
                        sailor->ToCreature()->AI()->Talk(0);
                        player->KilledMonsterCredit(Npcs::npc_injured_rescued_sailor);

                        Creature* controller = sailor->FindNearestCreature(56476, 50.f);
                        if (controller)
                            sailor->GetMotionMaster()->MovePoint(1, controller->GetRandomNearPosition(5.f));

                        sailor->ToCreature()->DespawnOrUnsummon(30s);
                    }

                    return true;
                }
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
    new at_lorewalker_zan();
    new at_chamber_of_whispers_outside();
    new at_chamber_of_whispers();
    new at_balloon_intro_q29790();
    new at_mandori_village_7710();
    new at_forlorn_hut_7714();
    new at_wreck_of_the_skyseeker_injured_sailor();

}
