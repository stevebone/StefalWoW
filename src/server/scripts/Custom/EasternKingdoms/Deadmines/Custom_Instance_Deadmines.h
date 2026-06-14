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

#ifndef CUSTOM_INSTANCE_DEADMINES_H
#define CUSTOM_INSTANCE_DEADMINES_H

#include "CreatureAIImpl.h"
#include "InstanceScript.h"
#include "Position.h"

namespace Scripts::EasternKingdoms::Deadmines
{
    namespace Misc
    {
        constexpr char const* DMScriptName = "custom_instance_deadmines";
        constexpr char const* DataHeader = "DM";
        static constexpr int8 EncounterCount = 7;
        static constexpr uint32 DeadminesVersion = 1000;
        static constexpr uint32 EntranceCannonFired = 1001;
        static constexpr uint32 HelixOafDead = 1002;
        static constexpr uint32 GoblinFoundryEventStarted = 1003;
        static constexpr uint32 GoblinFoundryEventNPC = 1004;
        static constexpr uint32 GoblinFoundryTrigger1Fired = 1005;
        static constexpr uint32 GoblinFoundryTrigger2Fired = 1006;
        static constexpr uint32 FoeReaperAddCounter = 1007;
        static constexpr uint32 FoeReaper5000AOEWarning = 1008;
    }

    namespace Version
    {
        static constexpr int8 NotSet = 0;
        static constexpr int8 Classic = 1;
        static constexpr int8 Modern = 2;
    }

    namespace DataTypes
    {
        static constexpr int8 BOSS_GLUBTOK = 0;
        static constexpr int8 BOSS_HELIX_GEARBREAKER = 1;
        static constexpr int8 BOSS_FOE_REAPER_5000 = 2;
        static constexpr int8 BOSS_ADMIRAL_RIPSNARL = 3;
        static constexpr int8 BOSS_CAPTAIN_COOKIE = 4;
        static constexpr int8 BOSS_VANESSA_VANCLEEF = 5;
    }

    namespace Creatures
    {
        // Bosses
        static constexpr uint32 Glubtok = 47162;
        static constexpr uint32 HelixGearbreaker = 47296;
        static constexpr uint32 FoeReaper5000 = 43778;
        static constexpr uint32 AdmiralRipsnarl = 47626;
        static constexpr uint32 CaptainCookie = 47739;
        static constexpr uint32 VanessaVanCleef = 49541;

        // Glubtok Adds
        static constexpr uint32 FireBlossom = 48957;
        static constexpr uint32 FrostBlossom = 48958;
        static constexpr uint32 FireWall2A = 48976;
        static constexpr uint32 FireWall1A = 48975;
        static constexpr uint32 FireWall1B = 49039;
        static constexpr uint32 FireWall2B = 49041;
        static constexpr uint32 FireWall1C = 49040;
        static constexpr uint32 FireWall2C = 49042;
        static constexpr uint32 GlubtokMainPlatter = 48974;
        static constexpr uint32 FrostBlossomDummy = 47284;
        static constexpr uint32 FireBlossomDummy = 47282;
        static constexpr uint32 BeamBunny = 47242;
        static constexpr uint32 FirewallBunny = 101052;
        static constexpr uint32 GlubtokDead = 47290;

        // Trash
        static constexpr uint32 OgreHenchman = 48230;
        static constexpr uint32 OgreBodyguard = 48262;
        static constexpr uint32 MiningPowder = 48284;
        static constexpr uint32 MiningMonkey1 = 48278;
        static constexpr uint32 MiningMonkey2 = 48440;
        static constexpr uint32 MiningMonkey3 = 48441;
        static constexpr uint32 MiningMonkey4 = 48442;
        static constexpr uint32 DefiasOverseer = 48421;

        // Helix Gearbreaker
        static constexpr uint32 HelixCrew = 49136;
        static constexpr uint32 StickyBomb = 47314;
        static constexpr uint32 LumberingOaf = 47297;
        static constexpr uint32 HelixGearBreaker = 47296;
        static constexpr uint32 MineRat = 51462;

        // Goblin Foundry Event
        static constexpr uint32 GoblinCraftsman = 48280;
        static constexpr uint32 GoblinEngineer = 48439;

        // Foe Reaper 5000
        static constexpr uint32 DefiasReaper = 47403;
        static constexpr uint32 DefiasWatcher = 47404;
        static constexpr uint32 PrototypeReaper = 49208;
        static constexpr uint32 MoltenSlag = 49229;
        static constexpr uint32 GeneralPurposeBunny = 47242;
        static constexpr uint32 HarvestTargetVisual = 47468;

        // Entrance Event
        static constexpr uint32 DefiasCannon = 48266;
        static constexpr uint32 SchorchMarkBunny = 48446;

        // Faction-Specific NPCs
        static constexpr uint32 StormwindDefender = 50595;
        static constexpr uint32 HoratioLaine = 46612;
        static constexpr uint32 CrimeSceneBot = 46613;
        static constexpr uint32 StormwindInvestigator = 46614;
        static constexpr uint32 QuartermasterLewis = 491;

        static constexpr uint32 MissMayhem = 46902;
        static constexpr uint32 MayhemPrototype = 46903;
        static constexpr uint32 Kagtha = 46889;
        static constexpr uint32 SlinkySharpshiv = 46906;
        static constexpr uint32 ShatteredHandAssassin = 46890;

        // Cannon Tunnel
        static constexpr uint32 DefiasEnforcer = 48502;
        static constexpr uint32 DefiasBloodWizzard = 48505;
        static constexpr uint32 GeneralPurposeBunnyJMF = 45979;
    }

    namespace CreatureSpawns
    {
        static constexpr uint32 OgreHenchmanByCannon = 375931;
        static constexpr uint32 OgreHenchmanSleeper = 375925;

        static constexpr uint32 OafMoveToBunny = 375849;
        static constexpr uint32 OafChargePosition = 375850;
    }

    namespace Objects
    {
        // Doors
        static constexpr uint32 FactoryDoor = 13965;
        static constexpr uint32 IronCladDoor = 16397;
        static constexpr uint32 MastRoomDoor = 16400;
        static constexpr uint32 HeavyDoor = 17153;
        static constexpr uint32 FoundryDoor = 16399;
        static constexpr uint32 DefiasCannon = 442661;
        static constexpr uint32 DoorLever = 101833;

        static constexpr uint32 MysteriousDeadminesChest = 180024;

        // Teleporters
        static constexpr uint32 GoblinTeleporter = 208002;
        static constexpr uint32 BallAndChain = 207079;
    }

    namespace TeleporterSpawnIds
    {
        static constexpr uint32 IronCladCove = 235317;
        static constexpr uint32 DungeonEntrance = 235318;
        static constexpr uint32 FoundryRoom = 235319;
        static constexpr uint32 MastRoom = 235320;
    }

    namespace Achievements
    {
        static constexpr uint32 ReadyForRaidingDeadmines = 5366;
    }

    namespace Texts
    {
        static constexpr uint32 GoblinTeleporterMenuID = 12691;

        // Glubtok
        static constexpr int8 GlubtokDeath = 0;
        static constexpr int8 GlubtokArcanePower = 1;
        static constexpr int8 GlubtokAggro = 2;
        static constexpr int8 GlubtokKill = 3;
        static constexpr int8 GlubtokFire = 4;
        static constexpr int8 GlubtokHead1 = 5;
        static constexpr int8 GlubtokFrost = 6;
        static constexpr int8 GlubtokHead2 = 7;
        static constexpr int8 GlubtokFirewall = 8;

        // Ogre Henchman
        static constexpr int8 OgreHenchmanOOC = 0;
        static constexpr int8 OgreHenchmanSleep = 1;
        static constexpr int8 OgreHenchmanCandle = 2;
        static constexpr int8 OgreHenchmanAggro = 3;

        // Ogre Bodyguard
        static constexpr int8 OgreBodyguardAggro = 0;
        static constexpr int8 OgreBodyguardFrenzy = 1;
        static constexpr int8 OgreBodyguardConversation1 = 2;
        static constexpr int8 OgreBodyguardConversation2 = 3;
        static constexpr int8 OgreBodyguardConversation3 = 4;
        static constexpr int8 OgreBodyguardConversation4 = 5;
        static constexpr int8 OgreBodyguardConversation5 = 6;
        static constexpr int8 OgreBodyguardConversation6 = 7;
        static constexpr int8 OgreBodyguardConversation7 = 8;
        static constexpr int8 OgreBodyguardConversation8 = 9;
        static constexpr int8 OgreBodyguardConversation9 = 10;
        static constexpr int8 OgreBodyguardConversation10 = 11;

        // Helix Crew
        static constexpr int8 HelixCrewStickyBomb = 0;

        // Lumbering Oaf
        static constexpr int8 OafCharge = 0;
        static constexpr int8 OafSmash = 1;

        // Helix
        static constexpr int8 HelixDeath = 0;
        static constexpr int8 HelixKill = 1;
        static constexpr int8 HelixOafDead = 2;
        static constexpr int8 HelixBombs = 3;
        static constexpr int8 HelixFaceRide = 4;
        static constexpr int8 HelixAggro = 5;
        static constexpr int8 HelixChestBomb = 6;

        // Defias Overseer
        static constexpr int8 OverseerLine1 = 0;
        static constexpr int8 OverseerLine2 = 1;
        static constexpr int8 OverseerLine3 = 2;
        static constexpr int8 OverseerLine4 = 3;
        static constexpr int8 OverseerLine5 = 4;
        static constexpr int8 OverseerLine6 = 5;
        static constexpr int8 OverseerLine7 = 6;
        static constexpr int8 OverseerLine8 = 7;
        static constexpr int8 OverseerLine9 = 8;
        static constexpr int8 OverseerLine10 = 9;
        static constexpr int8 OverseerFlee = 10;

        // Foe Reaper 5000
        static constexpr int8 FoeReaperInit = 9;
        static constexpr int8 FoeReaperAggro = 0;
        static constexpr int8 FoeReaperKill = 2;
        static constexpr int8 FoeReaperDeath = 1;
        static constexpr int8 FoeReaperMoltenSlag = 10;
        static constexpr int8 FoeReaperOverdrive = 5;
        static constexpr int8 FoeReaperOverdriveWarning = 6;
        static constexpr int8 FoeReaperHarvest = 4;
        static constexpr int8 FoeReaperHarvestSweep = 3;
        static constexpr int8 FoeReaperSafetyOff = 7;
        static constexpr int8 FoeReaperSafetyOffWarning = 8;
    }

    namespace Texts
    {
        // Goblin Foundry Event
        // Goblin Craftsman (48280)
        static constexpr int8 GoblinCraftsmanAlive = 0;
        static constexpr int8 GoblinCraftsmanYellAlive = 1;
        static constexpr int8 GoblinCraftsmanNo = 2;

        // Goblin Engineer (48439)
        static constexpr int8 GoblinEngineerRun = 0;
    }

    namespace Spells
    {
        static constexpr uint32 GoblinTeleporter = 55695;

        // Glubtok
        static constexpr uint32 ArcanePower = 88009;
        static constexpr uint32 FistOfFlame = 87859;
        static constexpr uint32 FistOfFlame0 = 87896;
        static constexpr uint32 FistOfFrost = 87861;
        static constexpr uint32 FistOfFrost0 = 87901;
        static constexpr uint32 FireBlossom = 88129;
        static constexpr uint32 FireBlossomVisual = 88164;
        static constexpr uint32 FrostBlossom = 88169;
        static constexpr uint32 FrostBlossomVisual = 88165;
        static constexpr uint32 TeleportVisual = 88002;
        static constexpr uint32 Blink = 87925;
        static constexpr uint32 BlossomTargetting = 88140;
        static constexpr uint32 ArcaneFire = 88007;
        static constexpr uint32 ArcaneFireBeam = 88072;
        static constexpr uint32 ArcaneFrostBeam = 88093;
        static constexpr uint32 SummonFireBlossom = 91301;
        static constexpr uint32 SummonFrostBlossom = 91302;
        static constexpr uint32 TriggerFireWall = 91398;
        static constexpr uint32 FireWallTriggered = 91397;
        static constexpr uint32 ArcaneOverloadInitial = 88183;
        static constexpr uint32 FeignDeath = 70628;
        static constexpr uint32 ArcaneOverloadSuicide = 88185;
        static constexpr uint32 ArcaneOverloadBoom = 90520;

        // Ogre Henchman
        static constexpr uint32 Uppercut = 91045;
        static constexpr uint32 OgrishMotivationNormal = 89652;
        static constexpr uint32 OgrishMotivationHeroic = 92747;
        static constexpr uint32 AuraSleep = 78677;

        // Ogre Bodyguard
        static constexpr uint32 Bonk = 91050;
        static constexpr uint32 Frenzy = 91053;

        // Mining Powder
        static constexpr uint32 Explode = 89769;

        // Defias Cannon
        static constexpr uint32 DefiasCannonCannonballFire = 89697;

        // Helix Gearbreaker
        static constexpr uint32 RideOaf = 88277;
        static constexpr uint32 OafSmash = 88300;
        static constexpr uint32 OafSmashHeroic = 91568;
        static constexpr uint32 OafCharge = 88288;
        static constexpr uint32 OafGrabTargeting = 88289;
        static constexpr uint32 HelixRide = 88337;
        static constexpr uint32 HelixRideFaceTimerAura = 88351;
        static constexpr uint32 RideVehicle = 88360; // used for Helix face riding player
        static constexpr uint32 ThrowBomb = 88264;
        static constexpr uint32 OafGuard = 90546;
        static constexpr uint32 HelixLeap = 86345;
        static constexpr uint32 ChestBomb = 88352;
        static constexpr uint32 ChestBombDMG = 88250;

        // Sticky Bomb
        static constexpr uint32 ArmingVisualYellow = 88315;
        static constexpr uint32 ArmingVisualOrange = 88316;
        static constexpr uint32 ArmingVisualRed = 88317;
        static constexpr uint32 ArmedState = 88319;
        static constexpr uint32 StickyBombExplode = 95500; // has normal and heroic effects
        //static constexpr uint32 StickyBombExplode = 88974;
        //static constexpr uint32 StickyBombExplodeHeroic = 91566;

        // Goblin Foundry Event
        static constexpr uint32 CosmeticCower = 78087;

        // Foe Reaper 5000
        static constexpr uint32 Offline = 88348;
        static constexpr uint32 RedEyes = 24263;
        static constexpr uint32 FoeEnergize = 89200; // cast self when immune to dmg
        static constexpr uint32 FoeMoltenSlag = 91839;
        static constexpr uint32 Energize = 89132; // from other reapers to Foe

        static constexpr uint32 Cleave = 90980;
        static constexpr uint32 Energized = 90978;
        static constexpr uint32 OnFire = 91737;
        static constexpr uint32 Watch = 90099;
        static constexpr uint32 ReaperStrike = 88490; //91723 this is for the prototype reaper
        static constexpr uint32 Overdrive = 88481;
        static constexpr uint32 Harvest = 88497; // 88495
        static constexpr uint32 HarvestAura = 88501; // 88497;
        static constexpr uint32 HarvestTargetVisual = 71371;
        static constexpr uint32 HarvestSweep = 88521;
        static constexpr uint32 SafetyOffline = 88522;

        // Prototype Reaper
        static constexpr uint32 PrototypeReaperCharge = 91726;
        static constexpr uint32 PrototypeReaperReaperStrike = 91723;
        static constexpr uint32 PrototypeReaperPressurizedStrike = 91727;
    }

    namespace Events
    {
        static constexpr int8 GlubtokCastFistOfFlame = 1;
        static constexpr int8 GlubtokCastFistOfFrost = 2;
        static constexpr int8 GlubtokCastBlink = 3;
        static constexpr int8 GlubtokCastBlossom = 4;
        static constexpr int8 GlubtokCastArcanePower1 = 5;
        static constexpr int8 GlubtokCastArcanePower2 = 6;
        static constexpr int8 GlubtokCastArcanePower3 = 7;
        static constexpr int8 GlubtokFallGround = 8;
        static constexpr int8 GlubtokSayFirewall = 9;
        static constexpr int8 GlubtokSpawnFirewallBunny = 10;
        static constexpr int8 GlubtokTransformDead = 11;

        // Ogre Henchman
        static constexpr int8 OgreHenchmanUppercut = 12;
        static constexpr int8 OgreHenchmanOOCSay = 13;
        static constexpr int8 OgreHenchmanSleepEvent = 14;
        static constexpr int8 OgreHenchmanSleepAura = 15;
        static constexpr int8 OgreHenchmanWakeUp = 16;

        // Ogre Bodyguard
        static constexpr int8 OgreBodyguardBonk = 17;
        static constexpr int8 OgreBodyguardConversation = 18;
        static constexpr int8 OgreBodyguardDialogueLine = 19;

        // Helix Crew
        static constexpr int8 HelixCrewStickyBomb = 20;

        // Sticky Bomb
        static constexpr int8 StickyBombInitial = 21;
        static constexpr int8 StickyBombYellow = 22;
        static constexpr int8 StickyBombOrange = 23;
        static constexpr int8 StickyBombRed = 24;
        static constexpr int8 StickyBombArmed = 25;
        static constexpr int8 StickyBombExplode = 26;

        // Lumbering Oaf
        static constexpr int8 OafCharge0 = 27;
        static constexpr int8 OafCharge1 = 28;
        static constexpr int8 OafCharge2 = 29;
        static constexpr int8 OafCharge3 = 30;
        static constexpr int8 OafThrowHelix = 31;
        static constexpr int8 OafPickupHelix = 32;

        // Helix Gearbreaker
        static constexpr int8 HelixThrowBomb = 33;
        static constexpr int8 HelixFaceRide = 34;
        static constexpr int8 HelixExitFaceRide = 35;
        static constexpr int8 HelixSummonCrew = 36;

        // Defias Overseer
        static constexpr int8 OverseerInitController = 37;
        static constexpr int8 OverseerDialogueStart = 38;
        static constexpr int8 OverseerDialogueLine = 39;

        // Goblin Foundry Event
        static constexpr int8 GoblinFoundryEventStarted = 40;
        static constexpr int8 GoblinFoundryEventNPC = 41; // SetData flag to mark event NPCs

        // Goblin Foundry Worker Events
        static constexpr int8 GoblinWorkerStartMovement = 1;
        static constexpr int8 GoblinWorkerRunToFinal = 2;
        static constexpr int8 GoblinWorkerCraftsmanNo = 3;

        // Foe Reaper 5000 Add Events
        static constexpr int8 FoeReaperAddCleave = 1;
        static constexpr int8 FoeReaperAddWatch = 2;
        static constexpr int8 FoeReaperAddDeath = 3;

        // Foe Reaper 5000 Boss Events
        static constexpr int8 FoeReaperInit = 4;
        static constexpr int8 FoeReaperAggro = 5;
        static constexpr int8 FoeReaperReaperStrike = 6;
        static constexpr int8 FoeReaperMoltenSlag = 7;
        static constexpr int8 FoeReaperMoltenSlagWarning = 8;
        static constexpr int8 FoeReaperOverdrive = 9;
        static constexpr int8 FoeReaperOverdriveWarning = 10;
        static constexpr int8 FoeReaperOverdriveSwitchTarget = 11;
        static constexpr int8 FoeReaperHarvest = 12;
        static constexpr int8 FoeReaperHarvestSweep = 13;
        static constexpr int8 FoeReaperSafetyOfflineWarning = 14;
        static constexpr int8 FoeReaperSafetyOffline = 15;
    }

    namespace Actions
    {
        static constexpr int8 OafCharge = 1;
    }

    namespace Glubtok
    {
        static constexpr Position Phase2Center = { -193.43f, -437.86f, 54.38f, 4.88f };

        static constexpr uint32 PhaseTransitionHealthPct = 50;
        static constexpr uint32 DeathHealthPct = 1;

        // Timers in Seconds
        static constexpr int8 FistOfFlameTimer = 10;
        static constexpr int8 FistOfFrostTimer = 21;
        static constexpr int8 BlinkTimer = 20;
        static constexpr int8 BlossomTimer = 5;
        static constexpr int8 ArcanePower1Timer = 2;
        static constexpr int8 ArcanePower2Timer = 3;
        static constexpr int8 ArcanePower3Timer = 1;
        static constexpr int8 FallGroundTimer = 5;
        static constexpr int8 BoomTimer = 1;
        static constexpr int8 SuicideTimer = 3;
    }

    namespace Items
    {
        static constexpr uint32 DefiasGunpowder = 221485;
    }

    namespace Quests
    {
        static constexpr uint32 YourFortuneAwaitsInDeadmines = 7938;
    }

    namespace Positions
    {
        static constexpr Position MysteriousDeadminesChest = { -32.1232f, -374.64f, 59.06f, 3.082f };

        // Goblin Foundry Event
        static constexpr Position GoblinCraftsman1Spawn = { -227.5972f, -587.4663f, 51.2344f, 2.3098f};
        static constexpr Position GoblinCraftsman1MoveTo = { -235.5213f, -581.7625f, 51.2185f, 2.5064f };
        static constexpr Position GoblinCraftsman2Spawn = { -217.2094f, -559.3613f, 51.23f, 3.8655f };
        static constexpr Position GoblinEngineer1Spawn = { -226.0326f, -567.9048f, 51.2304f, 4.1899f };
        static constexpr Position GoblinEngineer2Spawn = { -209.1832f, -554.1334f, 51.2297f, 3.2826f };
        static constexpr Position GoblinMoveTo = { -235.8898f, -577.8178f, 51.2258f, 3.1784f };
        static constexpr Position GoblinRunTo1 = { -277.722f, -495.944f, 49.6921f, 4.76221f };
        static constexpr Position GoblinRunTo2 = { -272.104f, -488.595f, 49.2876f, 5.79361f };
        static constexpr Position GoblinRunTo3 = { -273.826f, -477.703f, 49.2435f, 1.04438f };
        static constexpr Position GoblinRunTo4 = { -278.252f, -485.319f, 48.8387f, 0.289022f };

        // Helix Encounter
        static constexpr Position HelixCrewSpawn[] =
        {
            {-281.68f, -504.10f, 60.51f, 4.75f},
            {-284.71f, -504.13f, 60.42f, 4.72f},
            {-288.65f, -503.74f, 60.38f, 4.64f},
            {-293.88f, -503.90f, 60.07f, 4.77f},
        };

        static constexpr Position OafRatPos[8] =
        {
            {-290.90f, -486.49f, 49.88f},
            {-288.98f, -483.20f, 49.88f},
            {-293.78f, -483.81f, 49.15f},
            {-286.94f, -482.96f, 49.88f},
            {-288.16f, -484.81f, 49.88f},
            {-291.99f, -486.26f, 49.88f},
            {-289.67f, -487.22f, 49.88f},
            {-290.44f, -484.32f, 49.88f}
        };

        // Foe Reaper Encounter
        static constexpr Position PrototypeSpawn = { -200.499f, -553.946f, 51.2295f, 4.32651f };

        static constexpr Position FoeReaper5000AddSpawns[4] =
        {
            { -228.675f, -565.753f, 19.3898f, 5.98648f },
            { -182.743f, -565.969f, 19.3898f, 3.35103f },
            { -205.535f, -552.747f, 19.3898f, 4.53786f },
            { -229.724f, -590.372f, 19.3898f, 0.715585f }
        };

        static constexpr Position MoltenSlagSpawn[4] =
        {
            { -205.582f, -572.034f, 20.97f, 1.59f },
            { -199.143f, -579.843f, 20.97f, 6.16f },
            { -206.385f, -585.898f, 20.97f, 5.17f },
            { -212.704f, -579.072f, 20.97f, 3.09f }
        };
    }

    namespace CannonEvent
    {
        static constexpr int8 DATA_EVENT = 1;

        static constexpr int8 STATE_CANNON_NOT_USED = 0;
        static constexpr int8 STATE_CANNON_GUNPOWDER_USED = 1;
        static constexpr int8 STATE_CANNON_BLAST_INITIATED = 2;
        static constexpr int8 STATE_PIRATES_ATTACK = 3;
        static constexpr int8 STATE_DONE = 4;

        static constexpr uint32 BLAST_TIMER = 3000;
        static constexpr uint32 PIRATES_TIMER = 1000;
    }

    static constexpr ObjectData modernCreatureData[] =
    {
        { Creatures::Glubtok,              DataTypes::BOSS_GLUBTOK            },
        { Creatures::HelixGearbreaker,     DataTypes::BOSS_HELIX_GEARBREAKER  },
        { Creatures::FoeReaper5000,        DataTypes::BOSS_FOE_REAPER_5000    },
        { Creatures::AdmiralRipsnarl,      DataTypes::BOSS_ADMIRAL_RIPSNARL   },
        { Creatures::CaptainCookie,        DataTypes::BOSS_CAPTAIN_COOKIE     },
        { Creatures::VanessaVanCleef,      DataTypes::BOSS_VANESSA_VANCLEEF   },
    };

    static constexpr DoorData doorData[] =
    {
        { Objects::FactoryDoor,      DataTypes::BOSS_GLUBTOK,           EncounterDoorBehavior::OpenWhenDone             },
        { Objects::MastRoomDoor,     DataTypes::BOSS_HELIX_GEARBREAKER, EncounterDoorBehavior::OpenWhenDone             },
        { Objects::HeavyDoor,        DataTypes::BOSS_HELIX_GEARBREAKER, EncounterDoorBehavior::OpenWhenNotInProgress    },
        { Objects::FoundryDoor,      DataTypes::BOSS_FOE_REAPER_5000,   EncounterDoorBehavior::OpenWhenDone             },
    };

    static constexpr DungeonEncounterData modernEncounters[] =
    {
        { DataTypes::BOSS_GLUBTOK,             {{ 2976, 2981 }}  },
        { DataTypes::BOSS_HELIX_GEARBREAKER,   {{ 2977, 2982 }}  },
        { DataTypes::BOSS_FOE_REAPER_5000,     {{ 2975, 2980 }}  },
        { DataTypes::BOSS_ADMIRAL_RIPSNARL,    {{ 2974, 2979 }}  },
        { DataTypes::BOSS_CAPTAIN_COOKIE,      {{ 2973, 2978 }}  },
        { DataTypes::BOSS_VANESSA_VANCLEEF,    {{ 1081 }}  }
    };
    template <class AI, class T>
    inline AI* GetDeadminesAI(T* obj)
    {
        return GetInstanceAI<AI>(obj, Misc::DMScriptName);
    }
}

#endif // CUSTOM_INSTANCE_DEADMINES_H
