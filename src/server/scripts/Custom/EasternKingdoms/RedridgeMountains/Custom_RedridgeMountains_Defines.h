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
        static constexpr uint32 Jorgensen = 26560;
        static constexpr uint32 AndLastButNotLeastDanforth = 26562;
        static constexpr uint32 ItsNeverOver = 26616;
    }

    namespace AreaTriggers
    {
        static constexpr uint32 LakeshireGraveyard = 6034;
        static constexpr uint32 LakeshireInn = 682;
        static constexpr uint32 CampEverstill = 6079;
    }

    namespace Creatures
    {
        static constexpr uint32 GuardBateman = 43081;
        static constexpr uint32 Messner = 43270;
        static constexpr uint32 BlackrockScout = 4064;
        static constexpr uint32 BlackrockBattleWorg = 43340;
        static constexpr uint32 SpawnedMessner = 43300;
        static constexpr uint32 Jorgensen = 43272;
        static constexpr uint32 SpawnedJorgensen = 43305;
        static constexpr uint32 SpawnedKrakauer = 43303;
        static constexpr uint32 Danforth = 43302;
        static constexpr uint32 DanforthCaptured = 43275;
        static constexpr uint32 KeeshanRiverboat = 43450;
        static constexpr uint32 MessnerBoat = 43448;
        static constexpr uint32 KeeshanBoat = 43449;
        static constexpr uint32 KeeshanCamp = 43458;
        static constexpr uint32 DanforthCamp = 43462;
        static constexpr uint32 KrakauerCamp = 43461;
        static constexpr uint32 JorgensenCamp = 43460;
        static constexpr uint32 MessnerCamp = 43459;
    }

    namespace GameObjects
    {
        static constexpr uint32 MessnersCage = 204383;
        static constexpr uint32 JorgensensCage = 204393;
        static constexpr uint32 ChainLever = 204403;
    }

    namespace Spells
    {
        static constexpr uint32 CosmeticSleep = 55701;
        static constexpr uint32 HuntersMark = 80016;
        static constexpr uint32 MoltenArmor = 79849;
        static constexpr uint32 BlastWave = 79857;
        static constexpr uint32 FireBlast = 79855;
        static constexpr uint32 Fireball = 79854;
        static constexpr uint32 Flamestrike = 79856;
        static constexpr uint32 SummonMessner = 80893;
        static constexpr uint32 SummonJorgensen = 80940;
        static constexpr uint32 SummonKrakauer = 80941;
        static constexpr uint32 SummonDanforth = 80943;
        static constexpr uint32 ConcentrationAura = 79963;
        static constexpr uint32 SealOfRighteousness = 79962;
        static constexpr uint32 Exorcism = 79964;
        static constexpr uint32 HolyShock = 79961;
        static constexpr uint32 HolyLight = 79960;
        static constexpr uint32 BattleShout = 32064;
        static constexpr uint32 Slam = 79881;
        static constexpr uint32 CommandingShout = 80983;
        static constexpr uint32 Charge = 22120;
        static constexpr uint32 Shockwave = 79872;
        static constexpr uint32 Thunderclap = 8078;
        static constexpr uint32 CosmeticChainsRightHand = 88283;
        static constexpr uint32 CosmeticChainsLeftHand = 88284;
        static constexpr uint32 DetectInvis8 = 81080;
        static constexpr uint32 MessnerBoatEngine = 81260;
        static constexpr uint32 RiverboatQuestCredit = 81265;
        static constexpr uint32 SummonKeeshanRiverboat = 81243;
    }

    namespace Events
    {
        static constexpr uint32 GuardBatemanClearCooldown = 1;
        static constexpr uint32 MessnerTalk1 = 2;
        static constexpr uint32 MessnerClearTalkCooldown = 3;
        static constexpr uint32 SpawnedMessnerRandomTalk = 4;
        static constexpr uint32 SpawnedMessnerTalkSequence = 5;
        static constexpr uint32 SpawnedMessnerMoltenArmor = 6;
        static constexpr uint32 SpawnedMessnerCombatSpell = 7;
        static constexpr uint32 JorgensenTalk1 = 8;
        static constexpr uint32 JorgensenClearTalkCooldown = 9;
        static constexpr uint32 SpawnedJorgensenRandomTalk = 10;
        static constexpr uint32 SpawnedJorgensenSealOfRighteousness = 11;
        static constexpr uint32 SpawnedJorgensenCombatSpell = 12;
        static constexpr uint32 SpawnedJorgensenHeal = 13;
        static constexpr uint32 DanforthInitChains = 14;
        static constexpr uint32 SpawnedKrakauerRandomTalk = 15;
        static constexpr uint32 SpawnedKrakauerBattleShout = 16;
        static constexpr uint32 SpawnedKrakauerCombatSpell = 17;
        static constexpr uint32 DanforthRelease = 18;
        static constexpr uint32 SpawnedDanforthRandomTalk = 19;
        static constexpr uint32 SpawnedDanforthCommandingShout = 20;
        static constexpr uint32 SpawnedDanforthCombatSpell = 21;
        static constexpr uint32 RiverboatKeeshanTalk = 22;
        static constexpr uint32 RiverboatMessnerTalk = 23;
        static constexpr uint32 RiverboatMessnerEngine = 24;
        static constexpr uint32 RiverboatStartPath = 25;
    }

    namespace Talks
    {
        static constexpr uint32 GuardBatemanSay00 = 0;
        static constexpr uint32 MessnerSay00 = 0;
        static constexpr uint32 MessnerSay01 = 1;
        static constexpr uint32 MessnerSay02 = 2;
        static constexpr uint32 JorgensenSay00 = 0;
        static constexpr uint32 JorgensenSay01 = 1;
    }

    namespace Spawns
    {
        static constexpr uint32 DanforthChainDummyRight = 334866;
        static constexpr uint32 DanforthChainDummyLeft = 334867;
    }
}
