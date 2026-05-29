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

    namespace Data
    {
        static constexpr uint32 DeadminesVersion = 1000;
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
    }

    namespace Objects
    {
        // Doors
        static constexpr uint32 FactoryDoor = 13965;
        static constexpr uint32 IronCladDoor = 16397;
        static constexpr uint32 MastRoomDoor = 16400;
        static constexpr uint32 HeavyDoor = 17153;
        static constexpr uint32 FoundryDoor = 16399;
        static constexpr uint32 DefiasCannon = 16398;
        static constexpr uint32 DoorLever = 101833;
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
