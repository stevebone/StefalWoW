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

namespace Scripts::EasternKingdoms::Deadmines
{
    namespace Misc
    {
        constexpr char const* DMScriptName = "custom_instance_deadmines";
        constexpr char const* DataHeader = "DM";
        static constexpr int8 EncounterCount = 7;
        static constexpr uint32 DeadminesVersion = 1000;
        static constexpr uint32 EntranceCannonFired = 1001;
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
    }

    namespace CreatureSpawns
    {
        static constexpr uint32 OgreHenchmanByCannon = 375931;
        static constexpr uint32 OgreHenchmanSleeper = 375925;
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

        // Defias Cannon
        static constexpr uint32 DefiasCannonCannonballFire = 89697;
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
    }

    namespace GlubtokActions
    {
        static constexpr int8 StartFirewall = 1;
        static constexpr int8 StopFirewall = 2;
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
