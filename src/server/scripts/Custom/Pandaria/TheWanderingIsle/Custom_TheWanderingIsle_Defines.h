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

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Define.h"
#include <vector>
#include <chrono>

namespace Scripts::Custom::TheWanderingIsle
{
    namespace Misc
    {
        static constexpr uint16 Jojo_AiAnimKitID = 2935;
        // Shu spawn water spout bunnies
        static constexpr uint8 BUNNY_SPAWN_SLOT_0 = 0;
        static constexpr uint8 BUNNY_SPAWN_SLOT_1 = 1;
        static constexpr uint8 BUNNY_SPAWN_SLOT_2 = 2;
        static constexpr uint8 BUNNY_SPAWN_SLOT_3 = 3;
        static constexpr uint8 BUNNY_SPAWN_SLOT_4 = 4;
        static constexpr uint8 BUNNY_SPAWN_SLOT_5 = 5;
        static constexpr uint8 BUNNY_SPAWN_SLOT_6 = 6;
        static constexpr uint8 BUNNY_SPAWN_MAX_SLOTS = 7;
        static constexpr uint8 SHU_JUMP_POSITION_0 = 0;
        static constexpr uint8 SHU_JUMP_POSITION_1 = 1;
        static constexpr uint8 SHU_JUMP_POSITION_2 = 2;
        static constexpr uint8 SHU_JUMP_POSITION_3 = 3;
        static constexpr uint8 SHU_DATA_JUMP_POSITION = 1;

        // Shu at farmstead
        static constexpr uint32 shu_farmstead_gossip_menu = 13140;

        // Zhaoren Event
        static constexpr uint8 ZHAO_PHASE_FLYING = 1;
        static constexpr uint8 ZHAO_PHASE_GROUNDED = 2;
        static constexpr uint8 ZHAO_PHASE_STAY_IN_CENTER = 3;
        static constexpr uint8 EVENT_DATA_1 = 1;
        static constexpr uint8 DATA_COMBAT = 2;
        static constexpr uint8 DATA_AYSA_TALK_3 = 3;
        static constexpr uint8 DATA_PHASE_OOC = 4;
        static constexpr uint8 DATA_ZHAOREN_DEATH = 5;
        static constexpr uint8 DATA_EVADE = 6;
        static constexpr uint8 LAUNCHER_STATE_NONE = 0;
        static constexpr uint8 LAUNCHER_STATE_READY = 1;
        static constexpr uint8 LAUNCHER_STATE_USED = 2;
        static constexpr uint8 LAUNCHER_STATE_RECHARGING = 3;

        // Balloon Ride Event
        static constexpr uint32 shenzinsu_bunny_text_1 = 55550;
        static constexpr uint32 shenzinsu_bunny_text_2 = 55568;
        static constexpr uint32 shenzinsu_bunny_text_3 = 55569;
        static constexpr uint32 shenzinsu_bunny_text_4 = 55570;
        static constexpr uint32 shenzinsu_bunny_text_5 = 55572;
        static constexpr uint32 shenzinsu_bunny_text_6 = 63407;
    }

    namespace Spells
    {
        // Lorewalker Story
        static constexpr uint32 spell_ruolin_singing = 126804;

        // Singing Pools
        static constexpr uint32 SummonChild1 = 116190;
        static constexpr uint32 SummonChild2 = 116191;

        static constexpr uint32 spell_razor_beak = 109088;
        static constexpr uint32 spell_serpent_strike = 128409;

        static constexpr uint32 spell_jojo_headbash_reeds_cast = 129272;
        static constexpr uint32 spell_jojo_headbash_stack_of_reeds_impact = 108798;

        static constexpr uint32 spell_force_cast_ride_pole = 103031;
        static constexpr uint32 spell_throw_rock = 109308;
        static constexpr uint32 spell_monk_ride_pole = 103030;
        static constexpr uint32 spell_ride_vehicle_pole = 102717;
        static constexpr uint32 spell_training_bell_force_cast_ride_vehicle = 107050;
        static constexpr uint32 spell_curse_of_the_frog = 102938;
        static constexpr uint32 spell_training_bell_ride_vehicle = 107049;
        static constexpr uint32 spell_training_bell_exclusion_aura = 133381;

        static constexpr uint32 spell_jump_front_right = 117033;
        static constexpr uint32 spell_jump_front_left = 117034;
        static constexpr uint32 spell_jump_back_right = 117035;
        static constexpr uint32 spell_jump_back_left = 117036;
        static constexpr uint32 spell_summon_water_spout = 116810;
        static constexpr uint32 spell_water_spout = 117063;
        static constexpr uint32 spell_forcecast_rock_jump_a = 103071;
        static constexpr uint32 spell_aysa_congrats_timer = 128589;
        static constexpr uint32 spell_aysa_congrats_trigger_aura = 128588;
        static constexpr uint32 spell_summon_spirit_of_water = 103538;
        static constexpr uint32 spell_water_spout_credit = 117054;

        // 29422 Huo, the Spirit of Fire
        static constexpr uint32 spell_fan_the_flames_throw_wood = 109090;
        static constexpr uint32 spell_fan_the_flames_blow_air = 109095;
        static constexpr uint32 spell_fan_the_flames_blow_air_big = 109105;
        static constexpr uint32 spell_fan_the_flames_blow_air_bigger = 109109;
        static constexpr uint32 spell_fan_the_flames_credit = 109107;

        // 29423 The Passion of Shen-zin Su
        static constexpr uint32 spell_summon_spirit_of_fire = 128700;
        static constexpr uint32 spell_despawn_spirit_of_fire = 109178;
        static constexpr uint32 spell_summon_spirit_of_fire_on_relog = 102632;
        static constexpr uint32 spell_start_talk_event = 116220;
        static constexpr uint32 spell_fire_form = 109135;
        static constexpr uint32 spell_forcecast_fire_turn_in_statue_brazier_change = 106665;

        // Dai-Lo Farmstead
        static constexpr uint32 spell_shu_splash = 107030;
        static constexpr uint32 spell_water_spirit_laugh = 118035;
        static constexpr uint32 spell_shu_watersplash_credit = 104023;
        static constexpr uint32 spell_shu_watersplash = 118027;
        static constexpr uint32 spell_shu_watersplash_wugou = 118034;
        static constexpr uint32 spell_aura_sleep = 42386;
        static constexpr uint32 spell_aura_invisibility = 80797; // 105889 ??
        static constexpr uint32 spell_summon_spirits_water_earth = 104017;

        // RukRuk
        static constexpr uint32 spell_ookslosions_triggered = 125885;
        static constexpr uint32 spell_aim = 125609;
        static constexpr uint32 spell_ookslosions = 125699;
        static constexpr uint32 spell_aim_visual = 26079;
        static constexpr uint32 spell_rocket_explosion_visual = 125612;
        static constexpr uint32 spell_rocket_explosion_damage = 125619;

        // Forlorn Hut in the Forbidden forest
        static constexpr uint32 spell_summon_ji_at_forlorn_hut = 117973;

        // None left behind
        static constexpr uint32 spell_injured_sailor_feign_death = 108806;
        static constexpr uint32 spell_rescue_sailor = 129341;
        static constexpr uint32 spell_rescue_sailor_male = 105520;
        static constexpr uint32 spell_rescue_sailor_female = 129340;
        static constexpr uint32 spell_cancel_rescue_injured_sailor = 117987;

        // Vordraka boss fight
        static constexpr uint32 spell_aysa_tempered_fury = 117275;
        static constexpr uint32 spell_aysa_combat_roll = 117312;
        static constexpr uint32 spell_forcecast_summon_aysa_after_fight = 117499;
        static constexpr uint32 spell_vordraka_deep_sea_smash = 117287;
        static constexpr uint32 spell_vordraka_rupture = 117456;
        static constexpr uint32 spell_vordraka_summon_aggressor = 117403;
        static constexpr uint32 spell_vordraka_death_invis = 117555;
        static constexpr uint32 spell_vordraka_see_death_invis = 117491;

        // Explosion
        static constexpr uint32 spell_forcecast_summon_ji_after_fight = 117600;
        static constexpr uint32 spell_summon_ji_after_vordraka_fight = 117597;
        static constexpr uint32 spell_forcecast_trigger_blackout = 117610;
        static constexpr uint32 spell_trigger_blackout = 106037;

        // Healing of Shenzinsu
        static constexpr uint32 spell_turtle_healed_phase_update = 118232;
        static constexpr uint32 spell_turtle_healed_phase_timer = 118233;
        static constexpr uint32 spell_healing_shenzinsu = 117783;
        static constexpr uint32 spell_healing_shenzinsu_credit = 108898;
        static constexpr uint32 spell_druid_wound_heal = 117932;
        static constexpr uint32 spell_priest_wound_heal = 117784;
        static constexpr uint32 spell_alliance_priest_smite = 117935;
        static constexpr uint32 spell_alliance_priest_greater_heal = 117933;
        static constexpr uint32 spell_horde_druid_wrath = 117767;
        static constexpr uint32 spell_horde_druid_healing_touch = 117765;
        static constexpr uint32 spell_fighting_healer_rescued_aura = 117950;

        // Talking to Shenzinsu turtle spells - not working
        // Just for later reference
        static constexpr uint32 spell_trigger_with_anim_0 = 114898;
        static constexpr uint32 spell_trigger = 106759;
        static constexpr uint32 spell_trigger_with_anim_1 = 118571;
        static constexpr uint32 spell_trigger_with_turn = 118572;

        // Mandori Village - not working
        // Just for later reference
        // Blizz is using the trigger spell at the gate to summon the 3 npcs
        // The summoned NPCs are the ones doing the escort quest and not the phased npcs at the gate
        static constexpr uint32 spell_trigger_mandori_gate_npcs = 115345;
        static constexpr uint32 spell_summon_ji_mandori_gate = 115494;
        static constexpr uint32 spell_summon_jojo_mandori_gate = 115495;
        static constexpr uint32 spell_summon_aysa_mandori_gate = 115493;
        static constexpr uint32 spell_summon_hermit_hut_npcs = 115491;
        static constexpr uint32 spell_csa_area_trigger_dummy_timer_aura_self = 115644;
        static constexpr uint32 spell_mandori_gate_entrance_credit = 115442;
        static constexpr uint32 spell_pei_wu_gate_entrance_credit = 115443;

        static constexpr uint32 spell_force_vehicle_ride = 46598;
        static constexpr uint32 spell_eject_passengers = 50630;
        //static constexpr uint32 spell_rope_left = 108627;
        //static constexpr uint32 spell_rope_right = 108691;
    }

    namespace Data
    {
        // Vordraka boss fight
        static constexpr int8 data_aysa_vordraka_fight_talk_1 = 1;
        static constexpr int8 data_aysa_vordraka_fight_talk_2 = 2;
        static constexpr int8 data_vordraka_summon_aggressors = 3;
        static constexpr int8 data_vordraka_death = 4;

        // Healing of Shenzinsu
        static constexpr int8 data_healer_active = 1;
        static constexpr int8 data_healer_died = 2;
        static constexpr uint32 worldstate_healers_active = 6488;
    }

    namespace SpellsZhaorenEvent
    {
        static constexpr uint32 spell_summon_aysa_outside_chambers = 104593;
        static constexpr uint32 spell_lightning_pool = 126006;
        static constexpr uint32 spell_stunned_by_fireworks = 125992;
        static constexpr uint32 spell_serpent_sweep = 125990;
        static constexpr uint32 spell_forcecast_summon_shang = 128808;
        static constexpr uint32 spell_overpacked_firework = 104855;
        static constexpr uint32 spell_firework_inactive = 125964;
        static constexpr uint32 spell_overpacked_firework_launcher_ping = 125961;
        static constexpr uint32 spell_aicast_overpacked_fireworkd = 125970;
    }

    namespace SpellsQ29787
    {
        static constexpr uint32 spell_summon_shang_q29787 = 105333;
    }

    namespace SpellsBalloonEvent
    {
        static constexpr uint32 spell_credit_speak_with_shenzinsu = 105010;
        static constexpr uint32 spell_eject_passenger_1 = 60603;
        static constexpr uint32 spell_parachute = 45472;
    }

    namespace TalksQ29423
    {
        static constexpr int8 chia_hui_autumnleaf_talk = 0;
        static constexpr int8 brewer_lin_talk = 0;
        static constexpr int8 shanxi_talk_0 = 0;
        static constexpr int8 shanxi_talk_1 = 1;
        static constexpr int8 shanxi_talk_2 = 2;
        static constexpr int8 shanxi_talk_3 = 3;
        static constexpr int8 shanxi_talk_4 = 4;
        static constexpr int8 shanxi_talk_5 = 5;
        static constexpr int8 shanxi_talk_6 = 6;
        static constexpr int8 aysa_talk = 0;
        static constexpr int8 ji_talk = 0;
    }

    namespace TalksQ29662
    {
        static constexpr int8 Jojo_Talk_0 = 0;
        static constexpr int8 Jojo_Talk_1 = 1;
    }

    namespace TalksQ29775
    {
        static constexpr int8 shanxi_talk_7 = 7;
        static constexpr int8 shanxi_talk_8 = 8;
        static constexpr int8 shanxi_talk_9 = 9;
    }

    namespace Talks
    {
        // Singing Pools Children
        static constexpr int8 DengTalk0 = 0;
        static constexpr int8 DengTalk1 = 1;
        static constexpr int8 DengTalk2 = 2;
        static constexpr int8 CaiTalk0 = 0;
        static constexpr int8 CaiTalk1 = 1;
        static constexpr int8 CaiTalk2 = 2;
        static constexpr int8 CaiTalk3 = 3;

        // Cart Tender
        static constexpr int8 Cart_Tender_Talk_0 = 0;
        static constexpr int8 Cart_Tender_Talk_1 = 1;

        // Lorewalker Event
        static constexpr int8 lorewalker_zan_0 = 0;

        // Aysa outside chamber of whispers
        static constexpr int8 aysa_chamber_of_whispers_0 = 0;
        static constexpr int8 aysa_chamber_of_whispers_1 = 1;
        static constexpr int8 aysa_chamber_of_whispers_2 = 2;

        // 29787 Worthy of passing
        static constexpr int8 shang_talk_0 = 0;
        static constexpr int8 shang_talk_1 = 1;
        static constexpr int8 shang_talk_2 = 2;
        static constexpr int8 shang_talk_3 = 3;
        static constexpr int8 shang_talk_4 = 4;

        // Aysa Vordraka boss fight
        static constexpr int8 aysa_vordraka_boss_0 = 0;
        static constexpr int8 aysa_vordraka_boss_1 = 1;
        static constexpr int8 aysa_vordraka_boss_2 = 2;
        static constexpr int8 aysa_vordraka_boss_3 = 3;
        static constexpr int8 aysa_vordraka_boss_4 = 4;
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

    namespace PositionsQ29662
    {
        static constexpr Position JojoSpawnPoint = { 1039.49f, 3283.11f, 129.523f, 1.81514f };
        static constexpr Position StackOfReedsSpawnPoint = { 1038.5538330078125f, 3286.385498046875f, 128.25982666015625f, 4.921828269958496093f };
        static constexpr Position JojoMovePoint = { 1039.19f, 3284.26f, 129.3971f, 0.0f };
    }

    namespace PositionsQ29678
    {
        // Rock Jumping Quest
        static constexpr Position RockJumpFinal = { 1077.019f, 2844.103f, 95.27103f };
    }

    namespace PositionsQ29679
    {
        static constexpr Position JumpToBackRight = { 1127.26f, 2859.8f, 97.2817f };
        static constexpr Position JumpToBackLeft = { 1120.16f, 2882.66f, 96.345f };
        static constexpr Position JumpToFrontLeft = { 1100.83f, 2881.36f, 94.0386f };
        static constexpr Position JumpToFrontRight = { 1111.13f, 2850.21f, 94.6873f };

        static constexpr Position ShuSpawnPositions[4][Misc::BUNNY_SPAWN_MAX_SLOTS] =
        {
            {
                { 1117.516f, 2848.437f, 92.14017f },
                { 1105.92f, 2853.432f, 92.14017f },
                { 1105.231f, 2847.766f, 92.14017f },
                { 1114.819f, 2844.094f, 92.14017f },
                { 1110.618f, 2856.7f, 92.14017f },
                { 1109.559f, 2843.255f, 92.14017f },
                { 1116.04f, 2854.104f, 92.14017f }
            },
            {
                { 1106.743f, 2879.544f, 92.14017f },
                { 1105.793f, 2885.37f, 92.14017f },
                { 1098.16f, 2874.628f, 92.14017f },
                { 1104.28f, 2875.759f, 92.14017f },
                { 1095.38f, 2885.097f, 92.14017f },
                { 1100.078f, 2888.365f, 92.14017f },
                { 1094.693f, 2879.431f, 92.14017f }
            },
            {
                { 1132.911f, 2864.381f, 92.14017f },
                { 1125.672f, 2851.84f, 92.14017f },
                { 1121.057f, 2856.08f, 92.14017f },
                { 1134.373f, 2858.654f, 92.14017f },
                { 1126.556f, 2867.097f, 92.14017f },
                { 1120.064f, 2863.003f, 92.14017f },
                { 1131.856f, 2852.781f, 92.14017f }
            },
            {
                { 1118.22f, 2875.427f, 92.14017f },
                { 1113.274f, 2879.232f, 92.14017f },
                { 1125.439f, 2887.632f, 92.14017f },
                { 1118.766f, 2890.419f, 92.14017f },
                { 1113.783f, 2886.404f, 92.14017f },
                { 1123.7f, 2876.575f, 92.14017f },
                { 1126.358f, 2881.005f, 92.14017f }
            }
        };
    }

    namespace Positions
    {
        // Spell Positions
        static constexpr Position WugouSpawn = { 631.479f, 3140.7f,	87.8357f, 3.05745f };
        static constexpr Position pos_spell_117973 = { 424.717f, 3635.59f, 92.7838f, 2.49844f };
        static constexpr Position pos_spell_117597 = { 229.9426f, 4006.6967f, 87.2537f, 3.3859f };
        static constexpr Position JiTempleSpireSpawn = { 909.137f, 3610.38f, 252.092f };

        // Temple
        static constexpr Position ZhaoSkySpawn = { 750.5781f, 4262.676f, 323.0713f, 5.042483f }; // Zhaoren spawn point for Temple flyby

        // Dai-Lo Farmstead
        static constexpr Position ShuFarmsteadPlay[5] =
        {
                { 670.978882f, 3131.201904f, 88.328094f },
                { 679.594482f, 3131.564697f, 88.328094f },
                { 678.705139f, 3140.990234f, 88.328094f },
                { 673.825623f, 3143.905273f, 88.327415f },
                { 670.607544f, 3137.363525f, 88.327415f }
        };

        // Zhao-ren Chamber of Whispers
        static constexpr Position AysaOutsideChamber = { 644.8831f, 4227.2998f, 202.909271f, 2.4453f };
        static constexpr Position AysaInsideChamber = { 599.9302f, 4267.8554f, 206.552856f, 2.4319f };
        static constexpr Position ZhaoCenter = { 699.134f, 4170.06f, 216.06f };
        static constexpr Position ZhaoPos = { 723.163f, 4163.8f, 204.999f };

        // Balloon Ride event
        static constexpr Position BalloonArrivalControllerSpawn = { 904.059f, 4561.99f, 231.053f };

        // Wreck Explosion
        static constexpr Position JiDeckMove = { 231.112f, 3985.74f, 86.4415f };
        static constexpr Position JiDeckJump1 = { 230.404f, 3975.61f, 87.7406f };
        static constexpr Position JiDeckJump2 = { 231.49f, 3946.6f, 61.091f };
        static constexpr Position AysaDeckMove = { 233.022f, 3985.45f, 86.7175f };
        static constexpr Position AysaDeckJump1 = { 232.002f, 3974.86f, 87.766f };
        static constexpr Position AysaDeckJump2 = { 239.453f, 3942.2f, 62.5987f };

        // Healing Shenzinsu event
        static constexpr Position ShenzinsuHealerSpots[5] =
        {
            { 230.9681f, 3961.6079f, 59.6684f },
            { 235.9752f, 3960.1010f, 60.4301f },
            { 244.5514f, 3957.5493f, 62.2980f },
            { 253.4969f, 3957.4597f, 64.0818f },
            { 261.8049f, 3954.3000f, 67.4468f }
        };

        static constexpr Position Ravager_Spawn_1 = { 215.764f, 3950.22f, 71.3403f, 6.07209f }; 
        static constexpr Position Ravager_Jump_1 = { 234.4787f, 3957.618f, 60.7057f, 0.0167851f };
        static constexpr Position Ravager_Spawn_2 = { 288.583f, 3939.21f, 86.5738f, 2.65644f };
        static constexpr Position Ravager_Jump_2 = { 261.967f, 3950.88f, 68.3229f, 4.53972f };

    }

    namespace PositionsQ29787
    {
        static constexpr Position MasterShangSpawn = { 711.335f, 4178.049f, 197.845f };
    }

    namespace PositionsQ29792
    {
        static constexpr Position Aysha_point_1 = { 659.1265f, 3602.2268f, 146.8527f };
        static constexpr Position Aysha_point_2 = { 630.4346f, 3629.0419f, 134.3486f };
        static constexpr Position Aysha_point_3 = { 608.2857f, 3624.8295f, 122.1815f };
        static constexpr Position Aysha_point_4 = { 594.3450f, 3584.2138f, 98.6662f };
        static constexpr Position Aysha_point_5 = { 572.1107f, 3582.7983f, 94.9156f };

        static constexpr Position Aysha_point_gate = { 567.9053f, 3585.1166f, 94.6662f };
        static constexpr Position Ji_point_gate = { 567.7111f, 3582.3249f, 94.7847f };
        static constexpr Position Aysa_point_gate_back = { 573.6115f, 3586.9646f, 94.6854f };
        static constexpr Position Ji_point_gate_back = { 573.9601f, 3577.0935f, 95.0039f };
        static constexpr Position Jojo_point_gate = { 567.7102f, 3583.4360f, 94.7208f };

        static constexpr Position ForlornHut = { 424.717f, 3635.59f, 92.7838f };
    }


    namespace PathQ29423
    {
        static constexpr uint32 aysa = 6112600;
        static constexpr uint32 ji = 6112700;
        static constexpr uint32 ji_away = 6112701;
        static constexpr uint32 aysa_away = 6112601;
    }

    namespace PathQ29662
    {
        static constexpr uint32 path_jojo = 5763800;
    }

    namespace PathQ29680
    {
        static constexpr uint32 path_shu_follower = 5521300;
        static constexpr uint8 path_node_shu_remove = 4;
    }

    namespace PathQ29774
    {
        static constexpr uint32 path_shu_farmstead_1 = 5555800;
        static constexpr uint32 path_shu_farmstead_2 = 5555801;
    }

    namespace PathQ29775
    {
        static constexpr uint32 path_shu_temple_stairs = 5555802;
    }

    namespace PathOxCart
    {
        static constexpr uint32 path_ox = 5720700;
        static constexpr uint32 path_ox_farmstead = 5949800;
        static constexpr uint32 path_cart = 5720800;
        static constexpr uint32 path_cart_farmstead = 5949600;
        static constexpr uint8 path_node_remove_passenger = 27;
    }

    namespace PathZhaoren
    {
        static constexpr uint32 path_zhaoren_at_temple = 6455400;
    }

    namespace PathZhaorenEvent
    {
        static constexpr uint32 path_zhaoren_at_chamber = 5578600;
    }

    namespace PathQ29787
    {
        static constexpr uint32 path_zhang_0 = 5615900;
        static constexpr uint32 path_zhang_1 = 5615901;
    }

    namespace EventsQ29423
    {
        static constexpr int8 event_delivery_huo = 1;
        static constexpr int8 event_second_huo_position = 2;
    }

    namespace EventsQ29787
    {
        static constexpr int8 event_shang_talk_0 = 1;
        static constexpr int8 event_shang_path_0 = 2;
        static constexpr int8 event_shang_talk_4 = 3;
    }

    namespace Events
    {
        // Singing Pools
        static constexpr int8 event_shu_follower_path_start = 1;
        static constexpr int8 event_shu_follower_check_player_quest = 2;
        static constexpr int8 event_shu_singing_pools_check_player_vehicle = 3;

        static constexpr int8 event_cast_jump_spell = 1;
        static constexpr int8 event_shu_set_orientation = 2;
        static constexpr int8 event_shu_summon_water_spout = 3;

        static constexpr int8 event_cast_throw_rock = 1;
        static constexpr int8 event_monk_switch_pole = 2;
        static constexpr int8 event_monk_despawn = 3;
        static constexpr int8 event_cast_transform = 4;

        static constexpr int8 event_cast_serpent_strike = 1;
        static constexpr int8 event_check_players = 1;
        static constexpr int8 event_cast_razor_beak = 2;
        static constexpr int8 event_ox_cart_path_start = 1;

        // Lorewalker story
        static constexpr int8 event_lorewalker_check_player = 1;
        static constexpr int8 event_lorewalker_start = 2;
        static constexpr int8 event_start_dialogue = 3;

        // Dai-Lo farmstead
        static constexpr int8 event_shu_farmstead_play = 1;
        static constexpr int8 event_shu_farmstead_path_start_1 = 2;
        static constexpr int8 event_shu_farmstead_path_start_2 = 3;
        static constexpr int8 event_shu_farmstead_path_start_3 = 4;
        static constexpr int8 event_shu_wakes_wugou = 5;
        static constexpr int8 event_shu_farmstead_check_player_vehicle = 6;
        static constexpr int8 event_try_remove_wugou_sleep = 7;

        // Rukruk
        static constexpr int8 event_rukruk_cast_aim = 1;
        static constexpr int8 event_rukruk_cast_ooksplosions = 2;
        static constexpr int8 event_rukruk_rocket_fire = 3;

        // Zhao-ren battle
        static constexpr int8 event_aysa_outside_chambers_init = 1;
        static constexpr int8 event_aysa_outside_chambers_move1 = 2;
        static constexpr int8 event_aysa_outside_chambers_move2 = 3;
        static constexpr int8 event_aysa_inside_chambers_move_dafeng = 4;
        static constexpr int8 event_zhao_cast_lightning = 5;
        static constexpr int8 event_zhao_move_center = 6;
        static constexpr int8 event_zhao_state_stun = 7;
        static constexpr int8 event_zhao_cast_sweep = 8;
        static constexpr int8 event_zhao_resume_path = 9;
        static constexpr int8 event_firework_launcher_recharge = 1;
        static constexpr int8 event_firework_launcher_periodic_ping = 2;

        // Balloon ride
        static constexpr int8 event_aysa_jump_in = 1;
        static constexpr int8 event_spawn_balloon = 2;

        // Vordraka boss fight
        static constexpr int8 event_aysa_cast_tempered_fury = 1;
        static constexpr int8 event_aysa_cast_combat_roll = 2;
        static constexpr int8 event_aysa_update_phases = 3;
        static constexpr int8 event_vordraka_cast_smash = 1;
        static constexpr int8 event_vordraka_cast_rupture = 2;
        static constexpr int8 event_vordraka_cast_summon = 3;

        // Wreck explosion
        static constexpr int8 event_aysa_explosion_intro_talk = 1;
        static constexpr int8 event_aysa_explosion_leave_deck = 2;

        // Healing of Shenzinsu
        static constexpr int8 event_healing_bunny_spawn_adds = 1;
        static constexpr int8 event_healing_bunny_ji_yell = 2;
    }

    namespace Npcs
    {
        static constexpr uint32 npc_huo_q29422 = 57779;
        static constexpr uint32 credit_the_passion_of_shen_zin_su = 61128;
        static constexpr uint32 npc_huo_q29423 = 54958;
        static constexpr uint32 npc_aysa_q29423 = 61126;
        static constexpr uint32 npc_ji_q29423 = 61127;
        static constexpr uint32 npc_stack_of_reeds = 57636;
        // Singing Pools
        static constexpr uint32 Cai = 60250;
        static constexpr uint32 Deng = 60249;
        static constexpr uint32 npc_monk_on_pole_1 = 55019;
        static constexpr uint32 npc_monk_on_pole_2 = 65468; 
        static constexpr uint32 npc_training_bell_pole = 55083;
        static constexpr uint32 npc_training_pole_1 = 54993;
        static constexpr uint32 npc_training_pole_2 = 57431;
        static constexpr uint32 npc_fang_she = 55292;
        static constexpr uint32 credit_shu_the_spirit_of_water = 57476;
        static constexpr uint32 npc_bunny_water_spout = 60488;
        static constexpr uint32 npc_aysa_q29679 = 54975;

        static constexpr uint32 npc_cart_tender = 57712;
        static constexpr uint32 npc_cart = 57710;
        static constexpr uint32 npc_vehicle_cart = 57208;
        static constexpr uint32 npc_vehicle_cart_farmstead = 59496;
        static constexpr uint32 npc_vehicle_ox = 57207;
        static constexpr uint32 npc_vehicle_ox_farmstead = 59498;
        static constexpr uint32 npc_ox = 57709;

        static constexpr uint32 npc_shu_follower = 55213;
        static constexpr uint32 npc_bunny_water_spout_farmstead = 66941;
        static constexpr uint32 credit_not_in_the_face_1 = 55548;
        static constexpr uint32 credit_not_in_the_face_2 = 55547;
        static constexpr uint32 npc_wugou_q29774 = 60916;
        static constexpr uint32 npc_wugou_farmstead = 55539;

        static constexpr uint32 npc_shu_q29775 = 55558;
        static constexpr uint32 npc_wugou_q29775 = 60916;

        static constexpr uint32 npc_zhaoren_flying_temple_spire = 64554;

        // Lorewalker Story
        static constexpr uint32 npc_lorewalker_zan = 64885;
        static constexpr uint32 npc_lorewalker_ruolin = 64876;
        static constexpr uint32 npc_lorewalker_amai = 64875;
        static constexpr uint32 npc_lorewalker_hao = 64881;
        static constexpr uint32 npc_lorewalker_nan = 64880;
        static constexpr uint32 npc_lorewalker_yin = 64879;

        // Zhaoren Event
        static constexpr uint32 credit_da_feng_the_spirit_of_air = 55666;
        static constexpr uint32 npc_aysa_q29785 = 55595;
        static constexpr uint32 npc_dafeng_q29785 = 55592;
        static constexpr uint32 npc_aysa_outside_chambers = 55744;
        static constexpr uint32 npc_chamber_winds = 55665;
        static constexpr uint32 npc_ji_q29786 = 64505; // at the fight with Zhao
        static constexpr uint32 npc_aysa_q29786 = 64506; // at the fight with Zhao
        static constexpr uint32 npc_dafeng_q29786 = 64532;
        static constexpr uint32 npc_firework_launcher = 64507;
        static constexpr uint32 npc_shang_q29786 = 55586;

        // Q 29787
        static constexpr uint32 npc_guardian_q29787 = 56274;

        // Balloon ride event
        static constexpr uint32 npc_balloon_arrival_controller = 56679;
        static constexpr uint32 npc_aysa_q29790 = 56662;
        static constexpr uint32 npc_ji_q29790 = 56663;
        static constexpr uint32 npc_aysa_q29791 = 56661;
        static constexpr uint32 npc_ji_q29791 = 56660;
        static constexpr uint32 npc_balloon_q29791 = 55918;
        static constexpr uint32 npc_balloon_spawned = 55649;
        static constexpr uint32 credit_the_suffering_of_shenzinsu_1 = 56378;
        static constexpr uint32 credit_the_suffering_of_shenzinsu_2 = 55939;
        static constexpr uint32 npc_shenzinsu_bunny = 56676;

        // Mandori Village
        static constexpr uint32 npc_aysa_q29792 = 59962;
        static constexpr uint32 npc_ji_q29792 = 59960;
        static constexpr uint32 npc_jojo_q29792 = 59963;
        static constexpr uint32 credit_forest_door_one = 59946;
        static constexpr uint32 credit_forest_door_two = 59947;
        static constexpr uint32 npc_wei_palerage = 55943;
        static constexpr uint32 npc_korga_hut = 60042;

        // Forbidden Forest
        static constexpr uint32 npc_rescued_sailor = 56236;
        static constexpr uint32 npc_injured_rescued_sailor = 55999;
        static constexpr uint32 npc_vordraka = 56009;
        static constexpr uint32 npc_deepscale_aggressor = 60685;
        static constexpr uint32 npc_aysa_vordraka_fight = 56416;
        static constexpr uint32 npc_ji_after_vordraka_fight = 60741;
        static constexpr uint32 npc_healer_alliance_priest = 60877;
        static constexpr uint32 npc_healer_horde_druid = 60770;
        static constexpr uint32 npc_healer_alliance_priest2 = 60878;
        static constexpr uint32 npc_healer_horde_druid2 = 60834;
        static constexpr uint32 npc_spawned_deepscale_ravager = 60780;
        static constexpr uint32 npc_spawned_deepscale_fleshripper = 60858;
        static constexpr uint32 npc_ji_during_healing = 56418;
    }

    namespace Objects
    {
        static constexpr uint32 go_ancient_clam = 209584;
        static constexpr uint32 go_rock_jump_a = 209575;
        static constexpr uint32 go_rock_jump_b = 209576;
        static constexpr uint32 go_rock_jump_c = 209577;
        static constexpr uint32 go_spirit_wall = 209922;
        static constexpr uint32 go_forest_door = 210965;
        static constexpr uint32 go_forest_door2 = 210964;
    }

    namespace Quests
    {
        static constexpr uint32 quest_huo_the_spirit_of_fire = 29422;
        static constexpr uint32 quest_the_passion_of_shen_zin_su = 29423;
        static constexpr uint32 quest_the_singing_pools = 29521;
        static constexpr uint32 quest_stronger_than_reeds = 29662;
        static constexpr uint32 quest_the_lesson_of_the_balanced_rock = 29663;
        static constexpr uint32 quest_the_lesson_of_the_dry_fur = 29661;
        static constexpr uint32 quest_the_sun_pearl = 29677;
        static constexpr uint32 quest_shu_the_spirit_of_water = 29678;
        static constexpr uint32 quest_a_new_friend = 29679;
        static constexpr uint32 quest_the_source_of_livelihood = 29680;
        static constexpr uint32 quest_not_in_the_face = 29774;
        static constexpr uint32 quest_the_spirit_and_body_of_shenzinsu = 29775;
        static constexpr uint32 quest_morning_breeze_village = 29776;
        static constexpr uint32 quest_da_feng_the_spirit_of_air = 29785;
        static constexpr uint32 quest_battle_for_the_skies = 29786;
        static constexpr uint32 quest_passing_wisdom = 29790;
        static constexpr uint32 quest_suffering_of_shenzinsu = 29791;
        static constexpr uint32 quest_bidden_to_greatness = 29792;
        static constexpr uint32 quest_wrecking_the_wreck = 30589;
        static constexpr uint32 quest_none_left_behind = 29794;
        static constexpr uint32 quest_an_ancient_evil = 29798;
        static constexpr uint32 quest_risking_it_all = 30767;
        static constexpr uint32 quest_the_healing_of_shenzinsu = 29799;
    }

    namespace AreaTriggers
    {
        static constexpr uint32 areaTrigger_singing_pools_cart = 7258;
        static constexpr uint32 areaTrigger_farmstead_cart = 7822;
    }

    namespace MonkeyWisdomTexts
    {
        static constexpr uint32 text_monkey_wisdom_1 = 54073;
        static constexpr uint32 text_monkey_wisdom_2 = 54074;
        static constexpr uint32 text_monkey_wisdom_3 = 54075;
        static constexpr uint32 text_monkey_wisdom_4 = 54076;
        static constexpr uint32 text_monkey_wisdom_5 = 54077;
        static constexpr uint32 text_monkey_wisdom_6 = 54078;
        static constexpr uint32 text_monkey_wisdom_7 = 54079;
        static constexpr uint32 text_monkey_wisdom_8 = 54080;
    }

    struct LorewalkerDialogueEntry
    {
        uint32 npcEntry;
        uint8 talkId;
        std::chrono::milliseconds delay;
        bool isFinal = false;
    };

    extern const std::vector<LorewalkerDialogueEntry> dialogueSequence;
}
