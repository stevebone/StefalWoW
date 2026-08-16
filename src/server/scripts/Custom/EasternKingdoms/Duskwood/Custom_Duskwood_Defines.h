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

namespace Scripts::EasternKingdoms::Duskwood
{
    namespace Quests
    {
        static constexpr uint32 TheYorgenWorgen = 26717;
        static constexpr uint32 TheEmbalmersRevenge = 26727;
    }

    namespace Creatures
    {
        static constexpr uint32 ApprenticeFess  = 43738;
        static constexpr uint32 LurkingWorgen   = 43799;
        static constexpr uint32 Stiches = 43862;
    }

    namespace Gameobjects
    {
        static constexpr uint32 MoundOfLooseDirt = 204777;
    }

    namespace Spells
    {
        static constexpr uint32 StunningPounce = 81949;
        static constexpr uint32 AuraOfRot      = 3106;
    }

    namespace Positions
    {
        static constexpr Position LurkingWorgenSummonPos = { -11127.582f, -518.2431f, 35.2151f, 0.4713f };
        static constexpr Position LurkingWorgenFleePos   = { -11127.8056f, -462.2764f, 37.0339f };

        static constexpr Position StichesSpawn           = { -10553.90f, -1171.27f, 27.8604f, 1.48514f };
    }

    namespace Events
    {
        static constexpr uint8 MoveFlee = 1;
    }

    namespace Points
    {
        static constexpr uint32 PointFlee = 2;
    }

    namespace Data
    {
        static constexpr uint32 DataPounce = 1;
    }

    namespace Watchers
    {
        static constexpr uint32 Brownell  = 11040;
        static constexpr uint32 Fraizer   = 2470;
        static constexpr uint32 Hartin    = 886;
        static constexpr uint32 Jordan    = 887;
        static constexpr uint32 Keefer    = 495;
        static constexpr uint32 Ladimore  = 576;
    }
}
