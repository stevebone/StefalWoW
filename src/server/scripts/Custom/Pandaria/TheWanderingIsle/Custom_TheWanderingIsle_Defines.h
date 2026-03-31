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
#pragma once

#include "Define.h"

namespace Scripts::TheWanderingIsle::Defines
{
    namespace SpellsQ29422
    {
        static constexpr uint32 spell_fan_the_flames_throw_wood = 109090;
        static constexpr uint32 spell_fan_the_flames_blow_air = 109095;
        static constexpr uint32 spell_fan_the_flames_blow_air_big = 109105;
        static constexpr uint32 spell_fan_the_flames_blow_air_bigger = 109109;
        static constexpr uint32 spell_fan_the_flames_credit = 109107;
    }

    namespace SpellsQ29423
    {
        static constexpr uint32 spell_summon_spirit_of_fire = 128700;
        static constexpr uint32 spell_despawn_spirit_of_fire = 109178;
        static constexpr uint32 spell_summon_spirit_of_fire_on_relog = 102632;
        static constexpr uint32 spell_start_talk_event = 116220;
        static constexpr uint32 spell_fire_form = 109135;
        static constexpr uint32 spell_forcecast_fire_turn_in_statue_brazier_change = 106665;
    }

    namespace SpellsQ29521
    {
        static constexpr uint32 SummonChild1 = 116190;
        static constexpr uint32 SummonChild2 = 116191;
    }

    namespace TalksQ29423
    {
        static constexpr uint32 chia_hui_autumnleaf_talk = 0;
        static constexpr uint32 brewer_lin_talk = 0;
        static constexpr uint32 shanxi_talk_0 = 0;
        static constexpr uint32 shanxi_talk_1 = 1;
        static constexpr uint32 shanxi_talk_2 = 2;
        static constexpr uint32 shanxi_talk_3 = 3;
        static constexpr uint32 shanxi_talk_4 = 4;
        static constexpr uint32 shanxi_talk_5 = 5;
        static constexpr uint32 shanxi_talk_6 = 6;
        static constexpr uint32 aysa_talk = 0;
        static constexpr uint32 ji_talk = 0;
    }

    namespace TalksQ29521
    {
        // Singing Pools
        static constexpr uint32 DengTalk0 = 0;
        static constexpr uint32 DengTalk1 = 1;
        static constexpr uint32 DengTalk2 = 2;
        static constexpr uint32 CaiTalk0 = 0;
        static constexpr uint32 CaiTalk1 = 1;
        static constexpr uint32 CaiTalk2 = 2;
        static constexpr uint32 CaiTalk3 = 3;
    }

    namespace PositionsQ29423
    {
        static constexpr Position aysaSpawnPos = { 992.00347900390625f, 3600.757080078125f, 193.11480712890625f, 3.087874650955200195f };
        static constexpr Position jiSpawnPos = { 992.107666015625f, 3604.552978515625f, 193.1151580810546875f, 2.968008279800415039f };
        static constexpr Position huoFirstPoint = { 955.1213f, 3604.0388f, 200.71686f, 6.249388f };
        static constexpr Position huoSecondPoint = { 950.00757f, 3601.0044f, 203.8194f };
    }

    namespace PositionsQ29521
    {
        // Singing Pools
        static constexpr Position CaiSpawnPos = { 934.0156f, 3513.154f, 188.1347f, 0.0f };
        static constexpr Position DengSpawnPos = { 949.37f, 3510.0f, 187.7983f, 0.0f };
    }

    namespace PathQ29423
    {
        static constexpr uint32 aysa = 6112600;
        static constexpr uint32 ji = 6112700;
        static constexpr uint32 ji_away = 6112701;
        static constexpr uint32 aysa_away = 6112601;
    }

    namespace EventsQ29423
    {
        static constexpr uint32 event_delivery_huo = 1;
        static constexpr uint32 event_second_huo_position = 2;
    }

    namespace Npcs
    {
        static constexpr uint32 npc_huo_q29422 = 57779;
        static constexpr uint32 credit_the_passion_of_shen_zin_su = 61128;
        static constexpr uint32 npc_huo_q29423 = 54958;
        static constexpr uint32 npc_aysa_q29423 = 61126;
        static constexpr uint32 npc_ji_q29423 = 61127;
        // Singing Pools
        static constexpr uint32 Cai = 60250;
        static constexpr uint32 Deng = 60249;
    }

    namespace Quests
    {
        static constexpr uint32 quest_huo_the_spirit_of_fire = 29422;
        static constexpr uint32 quest_the_passion_of_shen_zin_su = 29423;
        static constexpr uint32 quest_the_singing_pools = 29521;
    }    
}

namespace Scripts::TheWanderingIsle::PScripts
{
    class player_singing_pools_memory
    {
    public:
        player_singing_pools_memory();
        bool CanTrigger(Player* player);

    private:
        std::unordered_map<ObjectGuid, time_t> lastTrigger;
    };

    extern player_singing_pools_memory g_singingPoolsMemory;
}

