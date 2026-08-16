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
        static constexpr uint32 MistmantlesRevenge = 26674;
    }

    namespace Creatures
    {
        static constexpr uint32 ApprenticeFess  = 43738;
        static constexpr uint32 LurkingWorgen   = 43799;
        static constexpr uint32 Stiches = 43862;
        static constexpr uint32 StalvanMistmantle = 315;
        static constexpr uint32 TobiasMistmantle = 43453;
        static constexpr uint32 TobiasMistmantleWorgen = 43797;
    }

    namespace Gameobjects
    {
        static constexpr uint32 MoundOfLooseDirt = 204777;
    }

    namespace Spells
    {
        static constexpr uint32 StunningPounce = 81949;
        static constexpr uint32 AuraOfRot      = 3106;
        static constexpr uint32 WorgenTransformVisual = 81908;
        static constexpr uint32 CurseOfStalvan = 3105;
    }

    namespace Positions
    {
        static constexpr Position LurkingWorgenSummonPos = { -11127.582f, -518.2431f, 35.2151f, 0.4713f };
        static constexpr Position LurkingWorgenFleePos   = { -11127.8056f, -462.2764f, 37.0339f };

        static constexpr Position StichesSpawn           = { -10553.90f, -1171.27f, 27.8604f, 1.48514f };

        static constexpr Position StalvanSpawn = { -10371.72f, -1251.92f, 35.99339f, 5.532694f };
        static constexpr Position StalvanMoveTo = { -10369.932617f, -1253.7677f, 35.909294f };
        static constexpr Position TobiasSpawn = { -10351.5f, -1256.7f, 35.3011f };
        static constexpr Position TobiasMoveTo = { -10365.8f, -1255.7f, 35.9098f };
        static constexpr Position TobiasFlee = { -10350.37f, -1230.15f, 37.53f };
    }

    namespace Events
    {
        static constexpr uint8 LurkingWorgenMoveFlee = 1;

        static constexpr uint8 StalvanStep1 = 1;
        static constexpr uint8 StalvanStep2 = 2;
        static constexpr uint8 StalvanStep3 = 3;
        static constexpr uint8 StalvanStep4 = 4;
        static constexpr uint8 StalvanStep5 = 5;
        static constexpr uint8 StalvanStep6 = 6;
        static constexpr uint8 StalvanCastSpell = 7;

        static constexpr uint8 TobiasStep1 = 1;
        static constexpr uint8 TobiasStep2 = 2;
        static constexpr uint8 TobiasStep3 = 3;
        static constexpr uint8 TobiasStep4 = 4;
    }

    namespace Points
    {
        static constexpr uint32 LurkingWorgenFlee = 1;

        static constexpr uint32 TobiasSpawn = 1;
        static constexpr uint32 TobiasFlee = 2;
    }

    namespace Data
    {
        static constexpr uint32 LurkingWorgenPounce = 1;

        static constexpr uint32 StalvanDied = 1;
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

    namespace Talks
    {
        static constexpr uint8 StalvanSay00 = 0;
        static constexpr uint8 StalvanSay01 = 1;
        static constexpr uint8 StalvanSay02 = 2;
        static constexpr uint8 StalvanSay03 = 3;
        static constexpr uint8 StalvanSay04 = 4;
        static constexpr uint8 StalvanSay05 = 5;
        static constexpr uint8 StalvanSay06 = 6;

        static constexpr uint8 TobiasSay00 = 0;
        static constexpr uint8 TobiasSay01 = 1;
        static constexpr uint8 TobiasSay02 = 2;
        static constexpr uint8 TobiasSay03 = 3;
        static constexpr uint8 TobiasSay04 = 4;
    }
}
