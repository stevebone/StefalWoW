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
        static constexpr uint32 ACurseWeCannotLift = 26720;
        static constexpr uint32 CryForTheMoon = 26760;
    }

    namespace Creatures
    {
        static constexpr uint32 ApprenticeFess  = 43738;
        static constexpr uint32 LurkingWorgen   = 43799;
        static constexpr uint32 LurkingWorgenAddleStead = 43814;
        static constexpr uint32 LurkingWorgenKillCredit = 43860;
        static constexpr uint32 Stiches = 43862;
        static constexpr uint32 StalvanMistmantle = 315;
        static constexpr uint32 TobiasMistmantle = 43453;
        static constexpr uint32 TobiasMistmantleWorgen = 43797;
        static constexpr uint32 SpawnedOliverHarris = 43858;
        static constexpr uint32 SpawnedJitters = 43859;
        static constexpr uint32 SvenYorgen = 43861;
        static constexpr uint32 LurkingWorgenRavenHill = 43950;
        static constexpr uint32 CryForTheMoonCredit = 43969;
        static constexpr uint32 ForlornSpirit = 43923;
        static constexpr uint32 ForlornSpiritKillCredit = 43930;
        static constexpr uint32 MorbentFel = 43761;
        static constexpr uint32 WeakenedMorbentFel = 43762;
    }

    namespace Gameobjects
    {
        static constexpr uint32 MoundOfLooseDirt = 204777;
    }

    namespace Spells
    {
        static constexpr uint32 StunningPounce = 81949;
        static constexpr uint32 Camouflage = 90954;
        static constexpr uint32 HarrissAmpule = 82058;
        static constexpr uint32 AuraOfRot      = 3106;
        static constexpr uint32 WorgenTransformVisual = 81908;
        static constexpr uint32 CurseOfStalvan = 3105;
        static constexpr uint32 RavenHillQuestgiverVision1 = 82288;
        static constexpr uint32 RavenHillQuestgiverVision2 = 82289;
        static constexpr uint32 SummonJitters = 82056;
        static constexpr uint32 SummonOliverHarris = 82055;
        static constexpr uint32 ChockedBySven = 82266;
        static constexpr uint32 RegainQuestInvisDetection = 82286;
        static constexpr uint32 InStocks = 69196;
        static constexpr uint32 RideVehicle = 46598; // 60683;
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

        static constexpr Position LurkingWorgenRavenHillSpawn = { -10747.3818f, 332.1483f, 37.7059f, 4.5219f };
        static constexpr Position OliverGivingCure = { -10746.0058f, 331.979f, 37.7435f, 3.7095f };
        static constexpr Position JittersHoldingWorgen = { -10748.7841f, 332.4266f, 37.4959f, 5.9658f };
        static constexpr Position LurkingWorgenRavenHillJump = { -10746.1f, 333.433f, 37.639f, 3.4f };
        static constexpr Position JittersExitVehicle = { -10748.6f, 333.025f, 37.484f };
    }

    namespace Events
    {
        static constexpr uint8 LurkingWorgenMoveFlee = 1;
        static constexpr uint8 LurkingWorgenAddleSteadAmbush = 2;

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

        static constexpr uint8 OliverMoveToCure = 1;
        static constexpr uint8 OliverTalk1 = 2;
        static constexpr uint8 OliverTalk2 = 3;
        static constexpr uint8 OliverTalk3 = 4;
        static constexpr uint8 OliverWalkHome = 5;

        static constexpr uint8 JittersRunToWorgen = 1;
        static constexpr uint8 JittersTalk0 = 2;
        static constexpr uint8 JittersChokeAndEnter = 3;
        static constexpr uint8 JittersExitAndRunBack = 4;

        static constexpr uint8 WorgenRavenHillTalk0 = 1;
        static constexpr uint8 WorgenRavenHillTalk1 = 2;
        static constexpr uint8 WorgenRavenHillTalk2 = 3;
        static constexpr uint8 WorgenRavenHillTalk3 = 4;
        static constexpr uint8 WorgenRavenHillTalk4 = 5;
        static constexpr uint8 WorgenRavenHillTalk5 = 6;

        static constexpr uint8 SoothingIncenseCloudSelectTarget = 1;
    }

    namespace Points
    {
        static constexpr uint32 LurkingWorgenFlee = 1;

        static constexpr uint32 TobiasSpawn = 1;
        static constexpr uint32 TobiasFlee = 2;

        static constexpr uint32 OliverMoveToCure = 1;
        static constexpr uint32 OliverWalkHome = 2;

        static constexpr uint32 JittersRunToWorgen = 1;
        static constexpr uint32 JittersRunHome = 2;

        static constexpr uint32 WorgenWalkToSven = 1;
    }

    namespace Data
    {
        static constexpr uint32 LurkingWorgenPounce = 1;

        static constexpr uint32 StalvanDied = 1;

        static constexpr uint32 CryForTheMoonStart = 1;
        static constexpr uint32 JittersTalk = 2;
        static constexpr uint32 JittersChoke = 3;
        static constexpr uint32 JittersRelease = 4;
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

        static constexpr uint8 LurkingWorgenAddleSteadSay00 = 0;

        static constexpr uint8 OliverSay0 = 0;
        static constexpr uint8 OliverSay1 = 1;
        static constexpr uint8 OliverSay2 = 2;
        static constexpr uint8 OliverSay3 = 3;

        static constexpr uint8 JittersSay0 = 0;

        static constexpr uint8 WorgenRavenHillSay0 = 0;
        static constexpr uint8 WorgenRavenHillSay1 = 1;
        static constexpr uint8 WorgenRavenHillSay2 = 2;
        static constexpr uint8 WorgenRavenHillSay3 = 3;
        static constexpr uint8 WorgenRavenHillSay4 = 4;
        static constexpr uint8 WorgenRavenHillSay5 = 0; // this is actually Sven
    }
}
