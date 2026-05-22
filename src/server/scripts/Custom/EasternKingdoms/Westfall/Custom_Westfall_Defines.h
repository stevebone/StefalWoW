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

#include "Position.h"

namespace Scripts::EasternKingdoms::Westfall
{
    namespace Quests
    {
        static constexpr uint32 TheDefiasBrotherhoodFinal = 166;

        static constexpr uint32 LivinTheLife = 26228;
        static constexpr uint32 LousPartingThoughts = 26232;
        static constexpr uint32 NewWestfallStew = 26241;
        static constexpr uint32 HopeForThePeople = 26266;
        static constexpr uint32 InDefenseOfWestfall = 26286;
    }

    namespace Creatures
    {
        static constexpr uint32 WestfallStew = 42617;
        static constexpr uint32 HomelessStormwindCitizen = 42384;
        static constexpr uint32 HomelessStormwindCitizen2 = 42386; // female version
        static constexpr uint32 Transient = 42383;
        static constexpr uint32 WestPlainsDrifter = 42391;
        static constexpr uint32 SentinelHillGuard = 42407;

        static constexpr uint32 SmallTimeHustler = 42390;
        static constexpr uint32 WestfallThug = 42387;
        static constexpr uint32 LousPartingThoughtsTrigger = 42562;
        static constexpr uint32 LousPartingThoughtsCredit = 42417;
        static constexpr uint32 StormwindInvestigator = 42559;
        static constexpr uint32 LtHorationLaine = 42558;

        static constexpr uint32 SalmaSaldean = 235;
        static constexpr uint32 WestfallOrphan = 42385;

        static constexpr uint32 SpawnedScoutGaliaanAtTower = 42752;
        static constexpr uint32 SpawnedCaptainDanuvinAtTower = 42751;
        static constexpr uint32 SpawnedGryanStoutmantleAtTower = 42750;
        static constexpr uint32 SpawnedHopeSaldeanAtTower = 42749;
        static constexpr uint32 SpawnedRipsnarlAtTower = 42748;
        static constexpr uint32 SpawnedStormwindInvestigatorAtTower = 42745;
        static constexpr uint32 SpawnedHoratioLaineAtTower = 42744;

        static constexpr uint32 GryanStoutmantle = 234;
        static constexpr uint32 LtHorationLaineAtTower = 42308;
        static constexpr uint32 RipsnarlAtTower = 42635;
    }

    namespace Spawns
    {
        static constexpr uint32 WestfallGuard = 275390;
        static constexpr uint32 RightSideCitizen = 275382;
        static constexpr uint32 RightBackCitizen = 274898;
        static constexpr uint32 LeftSideCitizen = 275403;
        static constexpr uint32 LeftBackCitizen = 274914;
        static constexpr uint32 DrifterWithCartBuddy = 280419;
        static constexpr uint32 CartBuddy = 280420;
        static constexpr uint32 StormwindInvestigator = 251574;
        static constexpr uint32 StormwindCitizenInterogation = 251572;
    }

    namespace Spells
    {
        static constexpr uint32 FullBelly = 79451;
        static constexpr uint32 SleepAura = 78677;
        static constexpr uint32 RottenAppleToss = 58509;
        static constexpr uint32 RottenBananaToss = 58513;
        static constexpr uint32 HoboStew = 80384;

        // Invis spells
        static constexpr uint32 DetectQuestInvis1 = 79229;
        static constexpr uint32 DetectQuestInvis2 = 79341;

        static constexpr uint32 HoratiosSunglasses = 78935;

        // Sentinel Hill
        static constexpr uint32 SummonRipsnarl = 79670;
        static constexpr uint32 SummonScoutGaliaan = 79669;
        static constexpr uint32 SummonCaptainDanuvin = 79668;
        static constexpr uint32 SummonStormwindInvestigator2 = 79667;
        static constexpr uint32 SummonStormwindInvestigator1 = 79666;
        static constexpr uint32 SummonHopeSaldean = 79665;
        static constexpr uint32 SummonHoratioLaine = 79664;
        static constexpr uint32 SummonGryanStoutmantle = 79663;
    }

    namespace Events
    {
        static constexpr int8 WestfalGuardPacifyDrifters = 1;
        static constexpr int8 WestfalGuardCrowdReaction = 2;
    }

    namespace Positions
    {
        static constexpr Position Q26232ThugPositions[4] =
        {
            { -9859.36f, 1332.42f, 41.985f, 2.49f },
            { -9862.51f, 1332.08f, 41.985f, 0.85f },
            { -9863.49f, 1335.49f, 41.985f, 5.63f },
            { -9860.42f, 1335.46f, 41.985f, 4.11f },
        };

        static constexpr Position SalmaSaldeanStew = { -10106.9238f, 1038.5223f, 37.947f, 5.64f };

        static constexpr Position GryanStoutmantleTalkRipsnarl = { -10512.5126f, 1046.5996f, 60.5181f, 3.6035f };
    }

    namespace Talks
    {
        // Quest: Lous parting thoughts
        // Thugs
        static constexpr int8 Q26232ThreatenPlayer = 0;
        static constexpr int8 Q26232AskMeet = 1;
        static constexpr int8 Q26232NoticePlayer = 2;
        static constexpr int8 Q26232ConfirmMeet = 3;
        static constexpr int8 Q26232Congratulate = 4;
        static constexpr int8 Q26232AskIdentity = 5;
        static constexpr int8 Q26232Die = 6;
        // Trigger
        static constexpr int8 Q26232ReturnToFurlsbrowCottage = 3;

    }

    namespace Dialogue
    {
        // Phase table describing the scripted sequence
        struct Q26232PhaseStep
        {
            uint8 thugIndex;   // 255 = special case
            uint32 talkId;     // 0 = no talk
            uint32 delay;      // next phase delay
        };

        static constexpr Q26232PhaseStep Q26232Steps[] =
        {
            { 1, Talks::Q26232AskMeet,        3500 }, // Phase 0
            { 0, Talks::Q26232ConfirmMeet,    4000 }, // Phase 1
            { 0, Talks::Q26232Congratulate,   7000 }, // Phase 2
            { 3, Talks::Q26232AskIdentity,    4000 }, // Phase 3
            { 255, 0,                         1000 }, // Phase 4: face player
            { 2, Talks::Q26232NoticePlayer,   4500 }, // Phase 5
            { 1, Talks::Q26232ThreatenPlayer, 4500 }, // Phase 6
            { 0, Talks::Q26232Die,            2000 }, // Phase 7
            { 255, 0,                         1000 }, // Phase 8: enable combat
            { 255, 0,                            0 }, // Phase 9: engage
        };
    }

    namespace Sounds
    {
        static constexpr uint32 Warning = 8174;
        static constexpr uint32 WomanScream = 17852;
        static constexpr uint32 WeaponFire1 = 161625;
        static constexpr uint32 WeaponFire2 = 161611;
    }
}

enum Custom_Westfall_Generic_NPCs
{
    

    NPC_WESTFALL_LIEUTENANT_HORATIO = 42308,

    NPC_WESTFALL_KEARNEN = 7024,
    NPC_WESTFALL_ELITE_MERCENARY = 42656,
    NPC_WESTFALL_SHADOWY_FIGURE = 42662,
    NPC_WESTFALL_HELIX_MORTWAKE = 42655,
    NPC_WESTFALL_OAF_MORTWAKE = 42754
};

enum Custom_Westfall_Quests
{
    
    QUEST_THE_DEFIAS_BROTHERHOOD_FINAL = 166,
    QUEST_FEEDING_THE_HUNGRY = 26271,
    QUEST_SECRETS_OF_THE_TOWER = 26290,
};

enum Custom_Westfall_Spells
{
    
    
    SPELL_WESTFALL_POTION_SHROUDING = 79528,
    SPELL_WESTFALL_KILL_SHOT = 79526,
    SPELL_WESTFALL_TELEPORT_VISUAL = 64446,
    SPELL_WESTFALL_Q26290_CREDIT = 79534,
};

enum Custom_Westfall_Events
{
    EVENT_KEARNEN_KILL_SHOT = 1,
};



constexpr int SAY_FURLBROW_RETURN = 42446;



enum Custom_Westfall_Q26290_TALK
{
    Q26232_SAY_HELIX_0 = 0,
    Q26232_SAY_HELIX_1 = 1,
    Q26232_SAY_HELIX_2 = 2,
    Q26232_SAY_HELIX_3 = 3,

    Q26232_SAY_SHADOWY_0 = 0,
    Q26232_SAY_SHADOWY_1 = 1,
    Q26232_SAY_SHADOWY_2 = 2,
    Q26232_SAY_SHADOWY_3 = 3,
    Q26232_SAY_SHADOWY_4 = 4,
};
