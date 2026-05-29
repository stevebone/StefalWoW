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

#ifndef CUSTOM_DEADMINES_OLD_DEFINES_H
#define CUSTOM_DEADMINES_OLD_DEFINES_H

#include "Define.h"

namespace Scripts::EasternKingdoms::Deadmines
{
    namespace MiscOLD
    {
        static constexpr int8 EncounterCount = 7;
        static constexpr uint32 PhaseDeadminesOLD = 50007;
        static constexpr uint32 ZidormiTextId = 7187;
    }

    namespace CreaturesOLD
    {
        static constexpr uint32 RhahkZor = 644;
    }

    namespace SpellsOLD
    {
        static constexpr uint32 RhahkZorSlam = 6304;
    }

    namespace DataTypesOLD
    {
        static constexpr int8 BOSS_RHAHKZOR = 0;
        static constexpr int8 BOSS_SNEED = 1;
        static constexpr int8 BOSS_GILNID = 2;
        static constexpr int8 BOSS_MR_SMITE = 3;
        static constexpr int8 BOSS_GREENSKIN = 4;
        static constexpr int8 BOSS_VANCLEEF = 5;
        static constexpr int8 BOSS_COOKIE = 6;
    }

    namespace EventsOLD
    {
        static constexpr int8 RhahkzorCastSlam = 1;
    }
}

#endif // CUSTOM_DEADMINES_OLD_DEFINES_H
