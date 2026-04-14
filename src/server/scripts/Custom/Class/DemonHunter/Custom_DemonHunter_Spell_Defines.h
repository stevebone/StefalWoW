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

namespace Scripts::Custom::DemonHunter
{
    namespace SpellCategories
    {
        static constexpr uint32 spell_category_dh_eye_beam = 1582;
        static constexpr uint32 spell_category_dh_blade_dance = 1640;
        static constexpr uint32 spell_category_dh_voidblade = 2426;
    }

    namespace Spells
    {
        static constexpr uint32 spell_dh_awaken_the_demon_within_cd = 207128;

        static constexpr uint32 spell_dh_annihilation = 201427;
        static constexpr uint32 spell_dh_annihilation_mh = 227518;
        static constexpr uint32 spell_dh_annihilation_oh = 201428;

        static constexpr uint32 spell_dh_blade_dance = 199552;

        static constexpr uint32 spell_dh_bloodlet_dot = 207690;

        static constexpr uint32 spell_dh_chaos_nova = 179057;
        static constexpr uint32 spell_dh_chaos_strike = 162794;
        static constexpr uint32 spell_dh_chaos_strike_energize = 193840;
        static constexpr uint32 spell_dh_chaos_strike_mh = 222031;
        static constexpr uint32 spell_dh_chaos_strike_oh = 199547;
        static constexpr uint32 spell_dh_chaos_theory_talent = 389687;
        static constexpr uint32 spell_dh_chaos_theory_crit = 390195;

        static constexpr uint32 spell_dh_death_sweep = 210153;

        static constexpr uint32 spell_dh_fel_devastation = 212084;
        static constexpr uint32 spell_dh_fel_devastation_dmg = 212105;
        static constexpr uint32 spell_dh_fel_devastation_heal = 212106;

        static constexpr uint32 spell_dh_fel_barrage = 211053;
        static constexpr uint32 spell_dh_fel_barrage_dmg = 211052;
        static constexpr uint32 spell_dh_fel_barrage_proc = 222703;

        static constexpr uint32 spell_dh_mana_rift = 235903;
        static constexpr uint32 spell_dh_mana_rift_damage_power_burn = 235904;

        static constexpr uint32 spell_dh_metamorphosis = 191428;
        static constexpr uint32 spell_dh_metamorphosis_devourer_transform = 1217607;
        static constexpr uint32 spell_dh_metamorphosis_dummy = 191427;
        static constexpr uint32 spell_dh_metamorphosis_impact_damage = 200166;
        static constexpr uint32 spell_dh_metamorphosis_reset = 320645;
        static constexpr uint32 spell_dh_metamorphosis_transform = 162264;
        static constexpr uint32 spell_dh_metamorphosis_vengeance_transform = 187827;

        static constexpr uint32 spell_dh_nemesis_aberrations = 208607;
        static constexpr uint32 spell_dh_nemesis_beasts = 208608;
        static constexpr uint32 spell_dh_nemesis_critters = 208609;
        static constexpr uint32 spell_dh_nemesis_demons = 208579;
        static constexpr uint32 spell_dh_nemesis_dragonkin = 208610;
        static constexpr uint32 spell_dh_nemesis_elementals = 208611;
        static constexpr uint32 spell_dh_nemesis_giants = 208612;
        static constexpr uint32 spell_dh_nemesis_humanoids = 208605;
        static constexpr uint32 spell_dh_nemesis_mechanicals = 208613;
        static constexpr uint32 spell_dh_nemesis_undead = 208614;

        static constexpr uint32 spell_dh_infernal_strike_cast = 189110;
        static constexpr uint32 spell_dh_infernal_strike_impact_damage = 189112;
        static constexpr uint32 spell_dh_infernal_strike_jump = 189111;
        static constexpr uint32 spell_dh_infernal_strike_visual = 208461;

        static constexpr uint32 spell_dh_abyssal_strike = 207550;

        static constexpr uint32 spell_dh_rain_of_chaos = 205628;
        static constexpr uint32 spell_dh_rain_of_chaos_impact = 232538;

        static constexpr uint32 spell_dh_sigil_of_chains = 202138;
        static constexpr uint32 spell_dh_sigil_of_chains_grip = 208674;
        static constexpr uint32 spell_dh_sigil_of_chains_jump = 208674;
        static constexpr uint32 spell_dh_sigil_of_chains_slow = 204843;
        static constexpr uint32 spell_dh_sigil_of_chains_snare = 204843;
        static constexpr uint32 spell_dh_sigil_of_chains_target_select = 204834;
        static constexpr uint32 spell_dh_sigil_of_chains_visual = 208673;

        static constexpr uint32 spell_dh_sigil_of_flame = 204596;
        static constexpr uint32 spell_dh_sigil_of_flame_aoe = 204598;
        static constexpr uint32 spell_dh_sigil_of_flame_energize = 389787;
        static constexpr uint32 spell_dh_sigil_of_flame_flame_crash = 228973;
        static constexpr uint32 spell_dh_sigil_of_flame_visual = 208710;

        static constexpr uint32 spell_dh_sigil_of_misery = 207684;
        static constexpr uint32 spell_dh_sigil_of_misery_aoe = 207685;

        static constexpr uint32 spell_dh_sigil_of_silence = 202137;
        static constexpr uint32 spell_dh_sigil_of_silence_aoe = 204490;

        static constexpr uint32 spell_dh_sigil_of_spite = 390163;
        static constexpr uint32 spell_dh_sigil_of_spite_aoe = 389860;

        static constexpr uint32 spell_dh_consume_soul_spawn = 1223448;
        static constexpr uint32 spell_dh_void_metamorphosis_passive = 471306;
        static constexpr uint32 spell_dh_void_metamorphosis_counter = 1225789;
        static constexpr uint32 spell_dh_void_metamorphosis_active = 1217605;
        static constexpr uint32 spell_dh_void_metamorphosis_buff = 1217607;
        static constexpr uint32 spell_dh_can_metamorphosis = 1213809;
        static constexpr uint32 spell_dh_void_ray = 473728;

        static constexpr uint32 spell_dh_eye_of_leotheras_dmg = 206650;
        static constexpr uint32 spell_dh_jagged_spikes = 205627;
        static constexpr uint32 spell_dh_jagged_spikes_dmg = 208790;
        static constexpr uint32 spell_dh_jagged_spikes_proc = 208796;

        static constexpr uint32 spell_dh_reap = 1226019;
        static constexpr uint32 spell_dh_reap_damage = 1225823;
        static constexpr uint32 spell_dh_reap_fury = 1261679;

        static constexpr uint32 spell_dh_cull = 1245453;
        static constexpr uint32 spell_dh_cull_damage = 1245455;

        static constexpr uint32 spell_dh_devour = 1217610;

        static constexpr uint32 spell_dh_moment_of_craving_passive = 1238488;
        static constexpr uint32 spell_dh_moment_of_craving_buff = 1238495;

        static constexpr uint32 spell_dh_eradicate_passive = 1226033;
        static constexpr uint32 spell_dh_eradicate = 1225826;
        static constexpr uint32 spell_dh_eradicate_damage = 1225827;
        static constexpr uint32 spell_dh_eradicate_damage_meta = 1279200;
        static constexpr uint32 spell_dh_eradicate_override = 1239524;

        static constexpr uint32 spell_dh_eye_beam = 198013;
        static constexpr uint32 spell_dh_eye_beam_damage = 198030;

        static constexpr uint32 spell_dh_collapsing_star_passive = 1221167;
        static constexpr uint32 spell_dh_collapsing_star_counter = 1227702;
        static constexpr uint32 spell_dh_collapsing_star = 1221150;
        static constexpr uint32 spell_dh_collapsing_star_damage = 1221162;
        static constexpr uint32 spell_dh_collapsing_star_override = 1221171;
        static constexpr uint32 spell_dh_collapsing_star_fragments = 1240204;

        static constexpr uint32 spell_dh_consume_soul_devourer = 1223423;
        static constexpr uint32 spell_dh_consume_soul_havoc_demon = 228556;
        static constexpr uint32 spell_dh_consume_soul_havoc_lesser = 228542;
        static constexpr uint32 spell_dh_consume_soul_havoc_shattered = 228540;
        static constexpr uint32 spell_dh_consume_soul_heal = 203794;
        static constexpr uint32 spell_dh_consume_soul_vengeance_demon = 210050;
        static constexpr uint32 spell_dh_consume_soul_vengeance_lesser = 208014;
        static constexpr uint32 spell_dh_consume_soul_vengeance_shattered = 210047;

        static constexpr uint32 spell_dh_soul_cleave = 228477;
        static constexpr uint32 spell_dh_soul_cleave_dmg = 228478;

        static constexpr uint32 spell_dh_painbringer_dummy = 225413;
        static constexpr uint32 spell_dh_painbringer_stack = 212988;
        static constexpr uint32 spell_dh_painbringer = 207387;
        static constexpr uint32 spell_dh_painbringer_buff = 212988;

        static constexpr uint32 spell_dh_gluttony_buff = 227330;

        static constexpr uint32 spell_dh_soul_barrier = 227225;

        static constexpr uint32 spell_dh_demon_spikes = 203819;
        static constexpr uint32 spell_dh_demon_spikes_trigger = 203720;

        static constexpr uint32 spell_dh_demonic_appetite = 206478;
        static constexpr uint32 spell_dh_darkness_absorb = 209426;
        static constexpr uint32 spell_dh_cover_of_darkness = 227635;
        static constexpr uint32 spell_dh_feast_on_the_souls = 201468;

        static constexpr uint32 spell_dh_rain_from_above_slowfall = 206804;
        static constexpr uint32 spell_dh_razor_spikes = 210003;

        static constexpr uint32 spell_dh_soul_fragment_heal_vengeance = 210042;
        static constexpr uint32 spell_dh_soul_fragment_demon_bonus = 163073;

        static constexpr uint32 spell_dh_spirit_bomb_damage = 218677;
        static constexpr uint32 spell_dh_spirit_bomb_heal = 227255;
        static constexpr uint32 spell_dh_spirit_bomb_visual = 218678;

        static constexpr uint32 spell_dh_shattered_souls_demon = 204256;
        static constexpr uint32 spell_dh_lesser_soul_shard = 203795;

        static constexpr uint32 spell_dh_feast_of_souls = 207697;
        static constexpr uint32 spell_dh_feast_of_souls_periodic_heal = 207693;

        static constexpr uint32 spell_dh_nether_bond = 207810;
        static constexpr uint32 spell_dh_nether_bond_damage = 207812;
        static constexpr uint32 spell_dh_nether_bond_periodic = 207811;

        static constexpr uint32 spell_dh_shattered_soul_devourer_lesser_left = 1223448;
        static constexpr uint32 spell_dh_shattered_soul_lesser_right = 228533;
        static constexpr uint32 spell_dh_shattered_soul_lesser_left = 237867;

        static constexpr uint32 spell_dh_shattered_souls_devourer = 1227619;
        static constexpr uint32 spell_dh_shattered_souls_devourer_dummy = 1223450;

        static constexpr uint32 spell_dh_shattered_souls_havoc = 209651;
        static constexpr uint32 spell_dh_shattered_souls_havoc_demon_trigger = 226370;
        static constexpr uint32 spell_dh_shattered_souls_havoc_lesser_trigger = 228536;
        static constexpr uint32 spell_dh_shattered_souls_havoc_shattered_trigger = 209687;

        static constexpr uint32 spell_dh_immolation_aura = 258920;

        static constexpr uint32 spell_dh_fiery_brand = 204021;
        static constexpr uint32 spell_dh_fiery_brand_rank_2 = 320962;
        static constexpr uint32 spell_dh_fiery_brand_debuff_rank_1 = 207744;
        static constexpr uint32 spell_dh_fiery_brand_debuff_rank_2 = 207771;

        static constexpr uint32 spell_dh_solitude_buff = 211510;

        static constexpr uint32 spell_dh_cleansed_by_flame = 205625;
        static constexpr uint32 spell_dh_cleansed_by_flame_dispel = 208770;

        static constexpr uint32 spell_dh_fallout = 227174;

        static constexpr uint32 spell_dh_charred_flesh = 336639;

        static constexpr uint32 spell_dh_shattered_souls_marker = 221461;
        static constexpr uint32 spell_dh_shattered_souls_vengeance = 204254;

        static constexpr uint32 spell_dh_feed_the_demon = 218612;

        static constexpr uint32 spell_dh_demon_reborn = 193897;
        static constexpr uint32 spell_dh_blur = 212800;
        static constexpr uint32 spell_dh_blur_trigger = 198589;

        static constexpr uint32 spell_dh_demonic = 213410;
        static constexpr uint32 spell_dh_demonic_appetite_energize = 210041;
        static constexpr uint32 spell_dh_demonic_origins = 235893;
        static constexpr uint32 spell_dh_demonic_origins_buff = 235894;
        static constexpr uint32 spell_dh_demonic_trample_dmg = 208645;
        static constexpr uint32 spell_dh_demonic_trample_stun = 213491;

        static constexpr uint32 spell_dh_feast_of_souls_passive = 1237270;
        static constexpr uint32 spell_dh_feast_of_souls_buff = 1232310;

        static constexpr uint32 spell_dh_entropy = 1261684;

        static constexpr uint32 spell_dh_hungering_slash_passive = 1239519;
        static constexpr uint32 spell_dh_hungering_slash_override = 1239525;
        static constexpr uint32 spell_dh_hungering_slash = 1239123;
        static constexpr uint32 spell_dh_hungering_slash_damage = 1239127;
        static constexpr uint32 spell_dh_hungering_slash_fury = 1239507;
        static constexpr uint32 spell_dh_hungering_slash_voidstep = 1223157;

        static constexpr uint32 spell_dh_the_hunt_damage = 1246169;
        static constexpr uint32 spell_dh_voidstep_damage = 1239526;
        static constexpr uint32 spell_dh_vengeful_retreat_damage = 198813;
        static constexpr uint32 spell_dh_vengeful_retreat_trigger = 198793,

        static constexpr uint32 spell_dh_emptiness_passive = 1242492;
        static constexpr uint32 spell_dh_emptiness_buff = 1242504;

        static constexpr uint32 spell_dh_fel_rush = 195072;
        static constexpr uint32 spell_dh_fel_rush_dmg = 192611;
        static constexpr uint32 spell_dh_fel_rush_damage = 223107;
        static constexpr uint32 spell_dh_fel_rush_ground = 197922;
        static constexpr uint32 spell_dh_fel_rush_water_air = 197923;

        static constexpr uint32 spell_dh_glide = 131347;
        static constexpr uint32 spell_dh_glide_duration = 197154;
        static constexpr uint32 spell_dh_glide_knockback = 196353;

        static constexpr uint32 spell_dh_fel_mastery = 192939;
        static constexpr uint32 spell_dh_fel_mastery_fury= 234244;

        static constexpr uint32 spell_dh_momentum = 206476;
        static constexpr uint32 spell_dh_momentum_buff = 208628;

        static constexpr uint32 spell_dh_rolling_torment_passive = 1244237;
        static constexpr uint32 spell_dh_rolling_torment_buff = 1244235;

        static constexpr uint32 spell_dh_soul_fragment_counter = 203981;
        static constexpr uint32 spell_dh_soul_fragment_devourer = 1223412;
        static constexpr uint32 spell_dh_soul_fragments_devourer_counter = 1245577;
        static constexpr uint32 spell_dh_soul_fragments_damage_taken_tracker = 210788;

        static constexpr uint32 spell_dh_soul_immolation = 1241937;
        static constexpr uint32 spell_dh_soul_immolation_fury = 1242475;

        static constexpr uint32 spell_dh_spontaneous_immolation = 1246556;
        static constexpr uint32 spell_dh_spontaneous_immolation_buff = 1266696;

        static constexpr uint32 spell_dh_void_ray_damage = 1213649;

        static constexpr uint32 spell_dh_voidfall_passive = 1253304;
        static constexpr uint32 spell_dh_voidfall_stack = 1256301;
        static constexpr uint32 spell_dh_voidfall_meteor_vengeance = 1256303;
        static constexpr uint32 spell_dh_voidfall_meteor_havoc = 1256304;

        static constexpr uint32 spell_dh_world_killer_passive = 1256353;
        static constexpr uint32 spell_dh_world_killer_meteor_vengeance = 1256616;
        static constexpr uint32 spell_dh_world_killer_meteor_havoc = 1256618;

        static constexpr uint32 spell_dh_catastrophe_passive = 1253769;
        static constexpr uint32 spell_dh_catastrophe_dot = 1256667;

        static constexpr uint32 spell_dh_meteoric_rise_passive = 1253377;
        static constexpr uint32 spell_dh_mass_acceleration_passive = 1256295;
        static constexpr uint32 spell_dh_meteoric_fall_passive = 1253391;

        static constexpr uint32 spell_dh_fel_eruption_damage = 225102;

        static constexpr uint32 spell_dh_final_hour_passive = 1253805;
        static constexpr uint32 spell_dh_final_hour_buff = 1256322;

        static constexpr uint32 spell_dh_doomsayer_passive = 1253676;
        static constexpr uint32 spell_dh_doomsayer_buff = 1265768;

        static constexpr uint32 spell_dh_dark_matter_passive = 1256307;

        static constexpr uint32 spell_dh_meteor_shower_devourer = 1264126;
        static constexpr uint32 spell_dh_meteor_shower_vengeance = 1264128;

        static constexpr uint32 spell_dh_midnight_passive = 1250088;
        static constexpr uint32 spell_dh_midnight_soul_erupt = 1250094;

        static constexpr uint32 spell_dh_devourer_spec = 1213636;
        static constexpr uint32 spell_dh_vengeance_spec = 212613;
        static constexpr uint32 spell_dh_havoc_spec = 212612;

        static constexpr uint32 spell_dh_shattered_soul_devourer_lesser_right = 1223445;

        static constexpr uint32 spell_dh_throw_glaive = 185123;

        static constexpr uint32 spell_dh_chaos_cleave_proc = 236237;

        static constexpr uint32 spell_dh_felblade = 232893;
        static constexpr uint32 spell_dh_felblade_charge = 213241;
        static constexpr uint32 spell_dh_felblade_cooldown_reset_proc_havoc = 236167;
        static constexpr uint32 spell_dh_felblade_cooldown_reset_proc_vengeance = 203557;
        static constexpr uint32 spell_dh_felblade_cooldown_reset_proc_visual = 204497;
        static constexpr uint32 spell_dh_felblade_damage = 213243;

        static constexpr uint32 spell_dh_reward_pain = 187724;

        static constexpr uint32 spell_dh_shear = 203782;
        static constexpr uint32 spell_dh_shear_passive = 203783;

        static constexpr uint32 spell_dh_shatter_the_souls = 212827;
    }
}
