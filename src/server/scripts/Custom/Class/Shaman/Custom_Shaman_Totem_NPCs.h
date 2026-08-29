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

enum Custom_Shaman_Totem_Generic_Spells
{
    SPELL_EARTHGRAB_TOTEM = 64695,
    SPELL_MANA_TIDE_TOTEM = 320763,
    SPELL_HEALING_TIDE_TOTEM = 255021,
    SPELL_FIRE_NOVA_TOTEM = 96004,
    SPELL_CAPACITATOR_TOTEM = 118905,
    SPELL_SCORCH_TOTEM = 15037,
};

enum Custom_Shaman_Totem_Generic_NPCS
{
    NPC_EARTHGRAB_TOTEM = 60561,
    NPC_MANA_TIDE_TOTEM = 10467,
    NPC_HEALING_TIDE_TOTEM = 59764,
    NPC_FIRE_NOVA_TOTEM = 15483,
    NPC_CAPACITATOR_TOTEM = 61245,
    NPC_SCORCHING_TOTEM = 9637,
};

enum Custom_Shaman_Totem_Generic_Events
{
    EVENT_PERIODIC_SPELL_CAST = 1,
    EVENT_TIMED_SPELL_CAST_NOVA,
    EVENT_TIMED_SPELL_CAST_CAPACITATOR,
};
