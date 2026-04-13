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

extern std::vector<FSBSpellDefinition> WarriorSpellsTable;

enum FSB_WARRIOR_STANCES
{
    SPELL_WARRIOR_DEFENSIVE_STANCE = 386208,
    SPELL_WARRIOR_BATTLE_STANCE = 386164,
    SPELL_WARRIOR_BERSERKER_STANCE = 386196
};

enum FSB_WARRIOR_BUFFS
{
    SPELL_WARRIOR_BATTLE_SHOUT = 6673
};

enum FSB_WARRIOR_COMBAT_SPELLS
{
    // PROTECTION
    SPELL_WARRIOR_DEVASTATE = 20243,
    SPELL_WARRIOR_REVENGE = 6572,
    SPELL_WARRIOR_SHIELD_SLAM = 23922,
    SPELL_WARRIOR_SHIELD_BLOCK = 2565,
    SPELL_WARRIOR_DISARM = 236077,
    SPELL_WARRIOR_REND = 394062,
    SPELL_WARRIOR_EXECUTE = 163201,
    SPELL_WARRIOR_IGNORE_PAIN = 190456,
    SPELL_WARRIOR_DEMO_SHOUT = 1160,
    SPELL_WARRIOR_CHALLENGING_SHOUT = 1161,
    SPELL_WARRIOR_DISRUPTING_SHOUT = 386071,
    SPELL_WARRIOR_LAST_STAND = 12975,
    SPELL_WARRIOR_SHIELD_CHARGE = 385952,
    SPELL_WARRIOR_SHIELD_WALL = 871,

    // FURY + ARMS
    SPELL_WARRIOR_CLEAVE = 845,
    SPELL_WARRIOR_MORTAL_STRIKE = 12294,
    SPELL_WARRIOR_COLOSSUS_SMASH = 167105,
    SPELL_WARRIOR_SLAM = 1464,
    SPELL_WARRIOR_BLOODTHIRST = 23881,
    SPELL_WARRIOR_RAMPAGE = 184367,
    SPELL_WARRIOR_RAGING_BLOW = 85288,
    SPELL_WARRIOR_FURIOUS_SLASH = 100130,
    SPELL_WARRIOR_BLADESTORM = 227847,
    SPELL_WARRIOR_DEMOLISH = 436358,
    SPELL_WARRIOR_OVERPOWER = 7384,
    SPELL_WARRIOR_SKULLSPLITTER = 260643,
    SPELL_WARRIOR_ONSLAUGHT = 315720,

    // SHARED
    SPELL_WARRIOR_CHARGE = 100,
    SPELL_WARRIOR_TAUNT = 355,
    SPELL_WARRIOR_RECKLESSNESS = 1719,
    SPELL_WARRIOR_PUMMEL = 6552,
    SPELL_WARRIOR_WHIRLWIND = 1680,
    SPELL_WARRIOR_HEROIC_THROW = 57755,
    SPELL_WARRIOR_HAMSTRING = 1715
};

namespace FSBWarrior
{
    bool BotHasDefensiveStance(Creature* bot);
    bool BotHasBattleStance(Creature* bot);
    bool BotHasBerserkerStance(Creature* bot);
    bool BotHasShieldWall(Creature* bot);

    void HandleOnSpellCast(Creature* bot, uint32 spellId);
}
