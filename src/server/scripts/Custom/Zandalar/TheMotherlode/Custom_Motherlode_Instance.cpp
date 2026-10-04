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
#include "CreatureAI.h"
#include "InstanceScript.h"
#include "Map.h"
#include "ScriptMgr.h"

#include "Custom_Motherlode_Defines.h"

namespace Scripts::Zandalar::TheMotherlode
{
    static constexpr ObjectData creatureData[] =
    {
        { Creatures::CoinOperatedCrowdPummeler, DataTypes::BOSS_COIN_OPERATED_CROWD_PUMMELER },
        { Creatures::Azerokk,                   DataTypes::BOSS_AZEROKK                    },
        { Creatures::RixxaFluxflame,            DataTypes::BOSS_RIXXA_FLUXFLAME            },
        { Creatures::MogulRazdunk,              DataTypes::BOSS_MOGUL_RAZDUNK              },
    };

    static constexpr DungeonEncounterData encounterData[] =
    {
        { DataTypes::BOSS_COIN_OPERATED_CROWD_PUMMELER, {{ Encounters::CrowdPummeler  }} },
        { DataTypes::BOSS_AZEROKK,                    {{ Encounters::Azerokk        }} },
        { DataTypes::BOSS_RIXXA_FLUXFLAME,            {{ Encounters::RixxaFluxflame }} },
        { DataTypes::BOSS_MOGUL_RAZDUNK,              {{ Encounters::MogulRazdunk   }} },
    };

    class custom_instance_the_motherlode : public InstanceMapScript
    {
    public:
        custom_instance_the_motherlode() : InstanceMapScript(Misc::ScriptName, Misc::MapId) { }

        struct custom_instance_the_motherlode_InstanceMapScript : public InstanceScript
        {
            custom_instance_the_motherlode_InstanceMapScript(InstanceMap* map) : InstanceScript(map)
            {
                SetHeaders(Misc::DataHeader);
                SetBossNumber(Misc::EncounterCount);
                LoadObjectData(creatureData, {});
                LoadDungeonEncounterData(encounterData);
            }
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const override
        {
            return new custom_instance_the_motherlode_InstanceMapScript(map);
        }
    };
}

void AddSC_custom_instance_motherlode()
{
    new Scripts::Zandalar::TheMotherlode::custom_instance_the_motherlode();
}
