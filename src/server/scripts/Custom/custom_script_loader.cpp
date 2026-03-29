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
void AddSC_custom_shaman_totem_npcs();
void AddSC_custom_warlock_spell_fixes();
void AddSC_custom_warlock_demon_npcs();

// ==================== Per Zone Scripts ====================== //
void AddSC_custom_elwynn_forest_npcs();
void AddSC_custom_tirisfal_glades_npcs();
void AddSC_custom_tirisfal_glades_player();
void AddSC_custom_westfall_npcs();
void AddSC_custom_westfall_player();

// ==================== Followship Scripts ==================== //
void AddSC_followship_bots();                // FSB main Script
void AddSC_followship_bots_player();                // FSB Player Scripts
void AddSC_followship_bots_commandscript();

// The name of this function should match:
// void Add${NameOfDirectory}Scripts()
void AddCustomScripts()
{
    AddSC_custom_player_mails();

    AddSC_custom_shaman_totem_npcs();
    AddSC_custom_warlock_spell_fixes();
    AddSC_custom_warlock_demon_npcs();

    AddSC_custom_elwynn_forest_npcs();
    AddSC_custom_tirisfal_glades_npcs();
    AddSC_custom_tirisfal_glades_player();
    AddSC_custom_westfall_npcs();
    AddSC_custom_westfall_player();

    AddSC_followship_bots();
    AddSC_followship_bots_player();
    AddSC_followship_bots_commandscript();
}
