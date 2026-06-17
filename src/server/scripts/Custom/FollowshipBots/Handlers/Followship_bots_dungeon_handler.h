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

#include "Custom_Instance_Deadmines.h"

namespace FSBDungeon
{
    // Default minimum distance casters keep from dungeon bosses (raw center-to-center)
    static constexpr float DungeonBossMinDistance = 10.f;

    namespace Maps
    {
        static constexpr uint32 Deadmines = 36;
    }

    namespace Creatures
    {
        namespace Deadmines
        {
            static constexpr uint32 BossIds[] = { 47162, 47296, 43778, 47626, 47739, 43908 };
        }
    }

    void CheckDungeonHandlingNeeded(Creature* bot);
    bool CheckDungeonInCombatHandlingNeeded(Creature* bot);
    uint32 GetBotDungeonId(Creature* bot);
    void UpdateBotDungeonId(Creature* bot);
    bool IsBotInDungeon(Creature* bot);
    bool ShouldPreserveMana(Creature* bot);
    bool IsTargetBoss(Creature* bot, Unit* target);
    bool ShouldDPSInExecutePhase(Creature* bot, Unit* target);
    bool ShouldMaintainBossDistance(Creature* bot, Unit* target, float minDistance);
    float GetDungeonBossMinDistance(uint32 bossEntry);
    void CheckAndQueueDeadUnits(Creature* bot, float searchRange);
    bool IsFoeReaperAOEActive(Unit* boss);
    bool HandleFoeReaperAOEEvasion(Creature* bot, Unit* target);
    Unit* GetDungeonTargetOverride(Creature* bot, Unit* target);

    namespace Deadmines
    {
        static constexpr float PROTOTYPE_REAPER_RANGE = 30.0f; // bot scan range
        static constexpr float BOSS_DISTANCE = 100.0f; // prototype reaper range scan

        static constexpr Position PrototypeReaperSlagsPosition = { -190.8952f, -573.4275f, 20.9768f, 3.7041f };

        // Per-boss caster minimum distance overrides (see GetDungeonBossMinDistance)
        static constexpr float FoeReaper5000MinDistance = 25.0f;
        // Per-boss melee minimum distance for AOE avoidance
        static constexpr float FoeReaper5000MinAOEDistance = 17.f;

        // Captain Cookie food interaction
        static constexpr float COOKIE_FOOD_SEARCH_RADIUS = 30.0f;
        static constexpr float COOKIE_FOOD_SPELLCLICK_RANGE = 5.0f;
        static constexpr float COOKIE_BAD_FOOD_AVOID_DISTANCE = 10.0f;

        void CheckPrototypeReaperEntry(Creature* bot);
        void HandleVehicleCombatCheck(Creature* bot);
        void HandleCaptainCookieFoodCycle(Creature* bot);
        bool ProcessCaptainCookieFoodClick(Creature* bot);
        Creature* FindNearestCookieFood(Creature* bot, bool searchBadFood);
    }
}
