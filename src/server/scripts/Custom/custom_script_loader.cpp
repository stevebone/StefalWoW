/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
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
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// This is where scripts' loading functions should be declared:

// ==================== Player Scripts ======================== //
void AddSC_custom_player_mails();

// ==================== Class Scripts ========================= //
void AddSC_custom_evoker_spell_fixes();
void AddSC_custom_shaman_totem_npcs();
void AddSC_custom_warlock_spell_fixes();
void AddSC_custom_warlock_demon_npcs();

// ==================== Per Zone Scripts ====================== //
void AddSC_custom_elwynn_forest_npcs();
void AddSC_custom_the_wandering_isle_at();
void AddSC_custom_the_wandering_isle_npcs();
void AddSC_custom_the_wandering_isle_player();
void AddSC_custom_the_wandering_isle_quests();
void AddSC_custom_the_wandering_isle_spells();
void AddSC_custom_tirisfal_glades_npcs();
void AddSC_custom_tirisfal_glades_player();
void AddSC_custom_westfall_npcs();
void AddSC_custom_westfall_player();

// ==================== Followship Scripts ==================== //
void AddSC_followship_bots();                // FSB main Script
void AddSC_followship_bots_player();                // FSB Player Scripts
void AddSC_followship_bots_commandscript();

// ==================== Delves Scripts ======================= //
void AddSC_delve_system();
void AddSC_instance_atal_aman_delve();
void AddSC_instance_shadow_enclave_delve();
void AddSC_delve_commands();

// ==================== Instance Scripts ===================== //
// Magister's Terrace 12.0.1 (Map 2811)
void AddSC_instance_magisters_terrace_12();
void AddSC_boss_arcanotron_custos();
void AddSC_boss_seranel_sunlash();
void AddSC_boss_gemellus();
void AddSC_boss_degentrius();
void AddSC_npc_magister_umbric_mt();
void AddSC_magisters_terrace_12_trash();

// ==================== Creature Codex Scripts =============== //
//void AddSC_creature_codex_sniffer();
//void AddSC_creature_codex_commands();

// ==================== Trading Post Scripts ================= //
void AddSC_npc_perks_program_vendor();

// ==================== Spells Scripts ======================= //
void AddSC_advanced_flying_spell_scripts();

// The name of this function should match:
// void Add${NameOfDirectory}Scripts()
void AddCustomScripts()
{
    AddSC_custom_player_mails();

    AddSC_custom_evoker_spell_fixes();
    AddSC_custom_shaman_totem_npcs();
    AddSC_custom_warlock_spell_fixes();
    AddSC_custom_warlock_demon_npcs();

    AddSC_custom_elwynn_forest_npcs();
    AddSC_custom_the_wandering_isle_at();
    AddSC_custom_the_wandering_isle_npcs();
    AddSC_custom_the_wandering_isle_player();
    AddSC_custom_the_wandering_isle_quests();
    AddSC_custom_the_wandering_isle_spells();
    AddSC_custom_tirisfal_glades_npcs();
    AddSC_custom_tirisfal_glades_player();
    AddSC_custom_westfall_npcs();
    AddSC_custom_westfall_player();

    AddSC_followship_bots();
    AddSC_followship_bots_player();
    AddSC_followship_bots_commandscript();

    // Creature Codex
    //AddSC_creature_codex_sniffer();
    //AddSC_creature_codex_commands();

    // Perks Program (Trading Post)
    AddSC_npc_perks_program_vendor();

    // Magister's Terrace 12.0.1 (Map 2811)
    AddSC_instance_magisters_terrace_12();
    AddSC_boss_arcanotron_custos();
    AddSC_boss_seranel_sunlash();
    AddSC_boss_gemellus();
    AddSC_boss_degentrius();
    AddSC_npc_magister_umbric_mt();
    AddSC_magisters_terrace_12_trash();

    // Delves
    AddSC_delve_system();
    AddSC_instance_atal_aman_delve();
    AddSC_instance_shadow_enclave_delve();
    AddSC_delve_commands();

    AddSC_advanced_flying_spell_scripts();
}
