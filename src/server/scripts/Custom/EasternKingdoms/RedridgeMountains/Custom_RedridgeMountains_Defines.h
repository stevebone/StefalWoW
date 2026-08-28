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

#include "Creature.h"
#include "Position.h"

namespace Scripts::EasternKingdoms::RedridgeMountains
{
    namespace Quests
    {
        static constexpr uint32 TuningTheGnomecorder = 26512;
        static constexpr uint32 InSearchOfBravoCompany = 26586;
        static constexpr uint32 BreakingOutIsHardToDo = 26587;
    }

    namespace AreaTriggers
    {
        static constexpr uint32 LakeshireGraveyard = 6034;
    }

    namespace Creatures
    {
        static constexpr uint32 GuardBateman = 43081;
        static constexpr uint32 Messner = 43270;
        static constexpr uint32 BlackrockScout = 4064;
        static constexpr uint32 BlackrockBattleWorg = 43340;
    }

    namespace GameObjects
    {
        static constexpr uint32 MessnersCage = 204383;
    }

    namespace Spells
    {
        static constexpr uint32 CosmeticSleep = 55701;
        static constexpr uint32 HuntersMark = 80016;
    }

    namespace Events
    {
        static constexpr uint32 GuardBatemanClearCooldown = 1;
        static constexpr uint32 MessnerTalk1 = 2;
        static constexpr uint32 MessnerClearTalkCooldown = 3;
    }

    namespace Talks
    {
        static constexpr uint32 GuardBatemanSay00 = 0;
        static constexpr uint32 MessnerSay00 = 0;
        static constexpr uint32 MessnerSay01 = 1;
        static constexpr uint32 MessnerSay02 = 2;
    }
}
