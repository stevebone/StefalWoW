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

namespace Scripts::Custom::TirisfalGlades
{
    namespace SpellsMisc
    {
        static constexpr uint32 spell_summon_darnell_0 = 91576;
        static constexpr uint32 spell_summon_darnell_1 = 91938;
        static constexpr uint32 spell_ride_vehicle = 60683;
    }

    namespace SpellsQ26800
    {
        static constexpr uint32 spell_darnell_slumped_over = 91935;
        static constexpr uint32 spell_select_scarlet_corpse = 91942;
        static constexpr uint32 spell_visual_pick_corpse = 91945;
        static constexpr uint32 spell_feign_death = 29266;
        static constexpr uint32 spell_cosmetic_yellow_arrow = 92230; // does not seem to be working atm in TC
        
    }

    namespace PositionsQ28608
    {
        static constexpr Position pos_darnell_grave_entrance = { 1665.368896f, 1662.722656f, 141.850983f };
        static constexpr Position pos_darnell_grave_stairs_1 = { 1642.761963f, 1662.547729f, 132.477753f };
        static constexpr Position pos_darnell_grave_stairs_2 = { 1642.498779f, 1677.809937f, 126.932129f };
        static constexpr Position pos_darnell_grave_ground =   { 1656.714478f, 1678.538330f, 120.718788f };
        static constexpr Position pos_darnell_grave_center =   { 1664.128052f, 1679.201294f, 120.530205f };

        static constexpr Position pos_darnell_supplies[] =
        {
            { 1663.849609f, 1694.495239f, 120.719284f },
            { 1672.939331f, 1668.029541f, 120.719307f },
            { 1656.963379f, 1667.456299f, 120.719093f },
            { 1656.098999f, 1688.312866f, 120.719093f }
        };
    }

    namespace PositionsQ25089
    {
        static constexpr Position pos_darnell_to_deathknell_1 = { 1685.67f, 1647.569f, 137.3269f };
        static constexpr Position pos_darnell_to_deathknell_2 = { 1768.192f, 1597.831f, 108.8092f };
        static constexpr Position pos_darnell_at_gate = { 1820.484f, 1590.924f, 95.7224f, 6.1732f };
    }

    namespace PositionsQ26800
    {
        static constexpr Position pos_darnell_drop_corpses = { 1864.555f, 1611.069f, 95.3388f, 4.3260f };
    }

    namespace EventsDarnell0
    {
        static constexpr int8 event_darnell_way_to_grave = 1;
        static constexpr int8 event_darnell_downstairs_1 = 2;
        static constexpr int8 event_darnell_despawn = 4;
        
    }

    namespace EventsDarnell
    {
        static constexpr int8 event_darnell_way_to_deathknell = 1;
        static constexpr int8 event_darnell_corpse_pickup = 2;
        static constexpr int8 event_darnell_return_to_player = 3;
        static constexpr int8 event_darnell_start_corpses_quest = 4;
    }

    namespace TalksDarnell0
    {
        static constexpr int8 talk_darnell_hello = 0;
        static constexpr int8 talk_darnell_to_grave = 1;
        static constexpr int8 talk_darnell_this_way = 2;
        static constexpr int8 talk_darnell_ask_about_supplies = 3;
        static constexpr int8 talk_darnell_supplies_loc_1 = 4;
        static constexpr int8 talk_darnell_hmm = 5;
        static constexpr int8 talk_darnell_nope = 6;
        static constexpr int8 talk_darnell_ask_player = 7;
        static constexpr int8 talk_darnell_supplies_loc_2 = 8;
        static constexpr int8 talk_darnell_supplies_found = 9;
        static constexpr int8 talk_darnell_joke = 10;
    }

    namespace TalksDarnell
    {
        static constexpr int8 talk_darnell_hello = 0;
        static constexpr int8 talk_darnell_to_deathknell = 1;
        static constexpr int8 talk_darnell_arrived_deathknell = 2;
        static constexpr int8 talk_darnell_arrived_saltain = 3;
        static constexpr int8 talk_darnell_start_corpses_quest = 4;
        static constexpr int8 talk_darnell_saw_corpses = 5;
    }

    namespace Misc
    {
        static constexpr int8 point_darnell_grave_entrance = 1;
        static constexpr int8 point_darnell_grave_stairs_1 = 2;
        static constexpr int8 point_darnell_grave_stairs_2 = 3;
        static constexpr int8 point_darnell_grave_ground = 4;
        static constexpr int8 point_darnell_grave_center = 5;
        static constexpr int8 point_darnell_grave_supplies_1 = 6;
        static constexpr int16 area_deathknell = 154;
        static constexpr int16 area_deathknell_graves = 5692;
    }

    namespace Npcs
    {
        // Deathknell
        static constexpr uint32 npc_darnell_0 = 49141;
        static constexpr uint32 npc_darnell_1 = 49337;
        static constexpr uint32 npc_scarlet_corpse = 49340;
        static constexpr uint32 npc_saltain = 1740;
        static constexpr uint32 npc_gate_bunny = 41200;
    }

    namespace Quests
    {
        static constexpr uint32 quest_fresh_out_of_the_grave = 24959; // on reward Darnell is summoned
        static constexpr uint32 quest_the_shadow_grave = 28608;
        static constexpr uint32 quest_recruitment = 26800; // on reward Darnell is despawned
        static constexpr uint32 quest_the_wakening = 24960; // on reward Darnell is summoned
        static constexpr uint32 quest_beyond_the_graves = 25089; // darnell is relocated to the deathknell gate
    }
}

enum Custom_Deathknell_Quests
{
    QUEST_FRESH_OUT_OF_THE_GRAVE = 24959, // on reward Darnell is summoned
    QUEST_RECRUITMENT = 26800, // on reward Darnell is despawned
    QUEST_THE_SHADOW_GRAVE = 28608, // on complete Darnell despawns
    QUEST_THOSE_THAT_COULDNT_BE_SAVED = 26799,
    QUEST_CARETAKER_CAICE = 28652,
    QUEST_THE_WAKENING = 24960, // on reward Darnell is summoned
    QUEST_BEYOND_THE_GRAVES = 25089, 
};

