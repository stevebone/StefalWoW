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

#include "Followship_bots_spells_handler.h"

extern std::vector<FSBSpellDefinition> ShamanSpellsTable;

enum FSB_SHAMAN_GENERIC_SPELLS
{
    SPELL_SHAMAN_ANCESTRAL_SPIRIT = 2008, //RESS
    SPELL_SHAMAN_GHOST_WOLF = 2645,

    SPELL_SHAMAN_HEALING_SURGE = 8004, //0s, 40f, 10% mana

    SPELL_SHAMAN_SKYFURY = 462854, //buff self
    SPELL_SHAMAN_LIGHTNING_SHIELD = 192106, //buff self
    SPELL_SHAMAN_HEROISM = 32182, //10m, self

    SPELL_SHAMAN_WATER_WALKING = 546, //buff 30f

    SPELL_SHAMAN_LIGHTNING_BOLT = 224721, //188196, //0s, 40f, 0.2% mana
    SPELL_SHAMAN_PRIMAL_STRIKE = 73899, //
    SPELL_SHAMAN_FLAME_SHOCK = 163057, //6s 25f

    // these are used by totems but we need them for aura application detection
    SPELL_MANA_TIDE_TOTEM = 320763,

    // 12.0.1 shared spells
    SPELL_SHAMAN_WIND_SHEAR = 57994, //interrupt, 25f, 12s
    SPELL_SHAMAN_PURGE = 370, //offensive dispel, 30f
    SPELL_SHAMAN_HEX = 51514, //CC, 30f, 30s
    SPELL_SHAMAN_CHAIN_LIGHTNING = 188443, //AoE damage, 40f
    SPELL_SHAMAN_FROST_SHOCK = 196840, //damage+slow, 25f, 6s
    SPELL_SHAMAN_ASTRAL_SHIFT = 108271, //defensive self, 90s

    SPELL_SHAMAN_CLEANSE_SPIRIT = 51886, //40f 8s dispel curses
};

enum FSB_SHAMAN_TOTEM_SPELLS
{
    SPELL_SHAMAN_EARTH_TOTEM = 2484, //30s, 40f //ANY
    SPELL_SHAMAN_HEALING_STREAM_TOTEM = 5394, //30s, 0f //HEAL
    SPELL_SHAMAN_WIND_RUSH_TOTEM = 192077, //2m, 40f /DMG
    SPELL_SHAMAN_LIQUID_MAGMA_TOTEM = 192222, //30s, 40f /DMG
    SPELL_SHAMAN_EARTHGRAB_TOTEM = 51485, //30s, 35f //TANK
    SPELL_SHAMAN_MANA_TIDE_TOTEM = 16191, //3m //HEAL
    SPELL_SHAMAN_HEALING_TIDE_TOTEM = 108280, //3m //HEAL
    SPELL_SHAMAN_FIRE_NOVA_TOTEM = 32062, //15s //TANK
    SPELL_SHAMAN_CAPACITATOR_TOTEM = 192058, //1m 40f //TANK
    SPELL_SHAMAN_SCORCHING_TOTEM = 15038, //35s, //DMG
};

enum FSB_SHAMAN_ELEMENTAL_SPELLS
{
    SPELL_SHAMAN_LAVA_BURST = 51505, //40f, 8s
    //SPELL_SHAMAN_EARTHQUAKE = 61882, //GROUND TARGET AoE, 40f, 6s //requires maelstrom
    //SPELL_SHAMAN_ASCENDANCE_FLAME = 114049, //NYI
    SPELL_SHAMAN_ELEMENTAL_BLAST = 117014, //40f, 12s
    SPELL_SHAMAN_EARTH_SHOCK = 8042, //damage, 25f
    SPELL_SHAMAN_THUNDERSTORM = 51490, //knockback, self, 30s

    SPELL_SHAMAN_STORMKEEPER = 191634, //self buff, 1m
};

enum FSB_SHAMAN_ENHANCEMENT_SPELLS
{
    SPELL_SHAMAN_STORMSTRIKE = 17364, //melee, 7.5s
    SPELL_SHAMAN_LAVA_LASH = 60103, //melee, 18s
    SPELL_SHAMAN_CRASH_LIGHTNING = 187874, //melee AoE, 12s
    //SPELL_SHAMAN_FERAL_SPIRIT = 51533, //summon, 2m //NYI in TC
    SPELL_SHAMAN_SUNDERING = 197214, //frontal AoE, 40s
    SPELL_SHAMAN_DOOM_WINDS = 384352, //AOE 1m self
    SPELL_SHAMAN_FLAMETONGUE_WEAPON = 78273, //self buff 30m
    SPELL_SHAMAN_ASCENDANCE_AIR = 114051, //3m self
};

enum FSB_SHAMAN_RESTO_SPELLS
{
    SPELL_SHAMAN_HEALING_WAVE = 77472, //40f
    SPELL_SHAMAN_PURIFY_SPIRIT = 77130, //8s 40f //Dispell
    SPELL_SHAMAN_RIPTIDE = 61295, //6s 40f
    SPELL_SHAMAN_UNLEASH_LIFE = 73685, //20s 40f
    SPELL_SHAMAN_HEALING_RAIN = 73920, //18s 40f GROUND TARGET
    SPELL_SHAMAN_ASCENDANCE = 114052, //3m self
    SPELL_SHAMAN_CHAIN_HEAL = 1064, //40f
    //SPELL_SHAMAN_SPIRIT_LINK_TOTEM = 98008, //totem GROUND TARGET, 3m //NYI in TC
    //SPELL_SHAMAN_SURGING_TOTEM
    SPELL_SHAMAN_DOWNPOUR = 207778, //GROUND TARGET AoE heal, 35f, 35s

    SPELL_SHAMAN_WATER_SHIELD = 52127, //buff self
    SPELL_SHAMAN_EARTH_SHIELD = 974, // 40f buff tank/player
};

enum FSB_SHAMAN_TANK_SPELLS
{
    //potential spells for a tank spec
    SPELL_SMAMAN_ROCKBITER_WEAPON = 159974, //selfbuff

    SPELL_SHAMAN_STRENGTH_TOTEM = 31633, // needs implementation with 31634
    SPELL_SHAMAN_WRATH_TOTEM = 204330, // needs implementation with 208963
    SPELL_SHAMAN_WINDFURY_TOTEM = 6112, // needs implementation with 327942 //maybe exchange with another totem
    SPELL_SHAMAN_SEARING_TOTEM = 3599, //maybe exchange with another totem

    SPELL_SHAMAN_EARTHEN_SPIKE = 300975, //40f 3s or more, 5% mana
    SPELL_SHAMAN_ICE_FURY = 210714, //40f
    SPELL_SHAMAN_STONE_BULWARK = 166584, //40f but use as self, 30s, 5% mana

    SPELL_SHAMAN_SHOCKWAVE = 46968, //40s 2f
    SPELL_SHAMAN_SUMMON_EARTH_ELEMENTAL = 242563, //out of combat self, maybe add cooldown?
    SPELL_SHAMAN_STATIC_CHARGE = 88043, //40f, 30s, 5% mana
};

namespace FSBShaman
{
    bool BotOOCBuffSelf(Creature* bot, uint32& outSpellId);
    bool BotInitialCombatSpells(Creature* bot);
    bool BotOOCHealOwner(Creature* bot);
}
