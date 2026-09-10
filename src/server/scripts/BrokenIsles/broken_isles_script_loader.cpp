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

void AddSC_zone_dalaran_broken_isle();
void AddSC_zone_mardum();

// Maw of Souls
void AddSC_boss_ymiron_the_fallen_king();
void AddSC_instance_maw_of_souls();

// Trial of Valor
void AddSC_boss_guarm();
void AddSC_instance_trial_of_valor();

// Black Rook Hold
void AddSC_boss_amalgam_of_souls();
void AddSC_instance_black_rook_hold();

// Neltharion's Lair
void AddSC_boss_rokmora();
void AddSC_instance_neltharions_lair();

// Eye of Azshara
void AddSC_boss_king_deepbeard();
void AddSC_instance_eye_of_azshara();

// Orderhalls
void AddSC_orderhall_warrior();
void AddSC_orderhall_rogue();
void AddSC_orderhall_hunter();
void AddSC_orderhall_legion();   // generic class-hall framework (unlock + Dalaran class messenger)

// Legion artifact acquisitions (per class, all specs)
void AddSC_artifact_warrior();
void AddSC_artifact_paladin();
void AddSC_artifact_deathknight();
void AddSC_artifact_shaman();
void AddSC_artifact_mage();
void AddSC_artifact_warlock();
void AddSC_artifact_monk();
void AddSC_artifact_druid();
void AddSC_artifact_rogue();
void AddSC_artifact_priest();
void AddSC_artifact_demonhunter();

// The name of this function should match:
// void Add${NameOfDirectory}Scripts()
void AddBrokenIslesScripts()
{
    AddSC_zone_dalaran_broken_isle();
    AddSC_zone_mardum();

    // Maw of Souls
    AddSC_boss_ymiron_the_fallen_king();
    AddSC_instance_maw_of_souls();

    // Trial of Valor
    AddSC_boss_guarm();
    AddSC_instance_trial_of_valor();

    // Black Rook Hold
    AddSC_boss_amalgam_of_souls();
    AddSC_instance_black_rook_hold();

    // Neltharion's Lair
    AddSC_boss_rokmora();
    AddSC_instance_neltharions_lair();

    // Eye of Azshara
    AddSC_boss_king_deepbeard();
    AddSC_instance_eye_of_azshara();

    // Orderhalls
    AddSC_orderhall_warrior();
    AddSC_orderhall_rogue();
    AddSC_orderhall_hunter();    // BM Hunter artifact ("Stolen Thunder" / Titanstrike) + MM/SV
    AddSC_orderhall_legion();    // generic class-hall framework (unlock + Dalaran class messenger)

    // Legion artifact acquisitions (per class, all specs)
    AddSC_artifact_warrior();
    AddSC_artifact_paladin();
    AddSC_artifact_deathknight();
    AddSC_artifact_shaman();
    AddSC_artifact_mage();
    AddSC_artifact_warlock();
    AddSC_artifact_monk();
    AddSC_artifact_druid();
    AddSC_artifact_rogue();
    AddSC_artifact_priest();
    AddSC_artifact_demonhunter();
}
