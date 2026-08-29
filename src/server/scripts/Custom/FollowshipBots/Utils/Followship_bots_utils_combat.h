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

#include "Unit.h"
#include "Creature.h"

namespace FSBCombatUtils
{
    // Actually gets a random target from attackers so maybe rename?
    Unit* GetRandomAttacker(Creature* bot);

    // Checks if either bot or player is in combat
    bool IsCombatActive(Creature* me);

    float GetBotChaseDistance(Creature* bot);
    float GetFallbackChaseDistanceForClass(FSB_Class cls);

    bool HasHostileInRange(Unit* bot, float range, uint32 count);
    bool IsOutOfCombatFor(Creature* bot, uint32 ms);
}
