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

class Creature;
class Unit;

namespace FSBDeadmines
{
    // Main entry point called from CheckDungeonInCombatHandlingNeeded.
    // Returns true if a deadmines-specific mechanic consumed this tick
    // (the caller should skip further generic handling for this bot).
    bool Handle(Creature* bot, Unit* target);

    // OOC entry point called from CheckDungeonHandlingNeeded.
    // Handles scheduling and out-of-combat mechanics.
    void HandleOutOfCombat(Creature* bot);

    namespace Encounters
    {
        // Foe Reaper 5000: tank and melee AOE evasion during Harvest / Overdrive
        bool HandleFoeReaper5000(Creature* bot, Unit* target);

        // Captain Cookie: food click cycle and bad food avoidance
        bool HandleCaptainCookie(Creature* bot, Unit* target);

        // Helper: find nearest cookie food (good or bad)
        Creature* FindNearestCookieFood(Creature* bot, bool searchBadFood);

        // Event handler: start a new cookie food cycle
        void HandleCaptainCookieFoodCycle(Creature* bot);

        // Process one cookie food click (move to food, spellclick, decrement counter)
        bool ProcessCaptainCookieFoodClick(Creature* bot);
    }
}
