/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
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
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "AreaBoundary.h"
#include "InstanceScript.h"
#include "Log.h"
#include "Map.h"
#include "Player.h"
#include "sunwell_plateau.h"

/* Sunwell Plateau:
0 - Kalecgos and Sathrovarr
1 - Brutallus
2 - Felmyst
3 - Eredar Twins (Alythess and Sacrolash)
4 - M'uru
5 - Kil'Jaeden
*/

DoorData const doorData[] =
{
    { GO_FIRE_BARRIER,     DATA_FELMYST,  EncounterDoorBehavior::OpenWhenDone },
    { GO_MURUS_GATE_1,     DATA_MURU,     EncounterDoorBehavior::OpenWhenNotInProgress },
    { GO_MURUS_GATE_2,     DATA_MURU,     EncounterDoorBehavior::OpenWhenDone },
    { GO_BOSS_COLLISION_1, DATA_KALECGOS, EncounterDoorBehavior::OpenWhenNotInProgress },
    { GO_BOSS_COLLISION_2, DATA_KALECGOS, EncounterDoorBehavior::OpenWhenNotInProgress },
    { GO_FORCE_FIELD,      DATA_KALECGOS, EncounterDoorBehavior::OpenWhenNotInProgress },
    { 0,                   0,             EncounterDoorBehavior::OpenWhenNotInProgress } // END
};

ObjectData const creatureData[] =
{
    { NPC_KALECGOS,               DATA_KALECGOS_DRAGON      },
    { NPC_KALECGOS_HUMAN,         DATA_KALECGOS_HUMAN       },
    { NPC_SATHROVARR,             DATA_SATHROVARR           },
    { NPC_BRUTALLUS,              DATA_BRUTALLUS            },
    { NPC_MADRIGOSA,              DATA_MADRIGOSA            },
    { NPC_FELMYST,                DATA_FELMYST              },
    { NPC_GRAND_WARLOCK_ALYTHESS, DATA_ALYTHESS             },
    { NPC_LADY_SACROLASH,         DATA_SACROLASH            },
    { NPC_MURU,                   DATA_MURU                 },
    { NPC_KILJAEDEN,              DATA_KILJAEDEN            },
    { NPC_KILJAEDEN_CONTROLLER,   DATA_KILJAEDEN_CONTROLLER },
    { NPC_ANVEENA,                DATA_ANVEENA              },
    { NPC_KALECGOS_KJ,            DATA_KALECGOS_KJ          },
    { 0,                          0                         } // END
};

BossBoundaryData const boundaries =
{
    { DATA_KALECGOS, new BoundaryUnionBoundary(new CircleBoundary(Position(1704.9f, 928.4f), 34.0f), new RectangleBoundary(1689.2f, 1713.3f, 762.2f, 1074.8f)) }
};

DungeonEncounterData const encounters[] =
{
    { DATA_KALECGOS, {{ 724 }} },
    { DATA_BRUTALLUS, {{ 725 }} },
    { DATA_FELMYST, {{ 726 }} },
    { DATA_EREDAR_TWINS, {{ 727 }} },
    { DATA_MURU, {{ 728 }} },
    { DATA_KILJAEDEN, {{ 729 }} }
};

class instance_sunwell_plateau : public InstanceMapScript
{
    public:
        instance_sunwell_plateau() : InstanceMapScript(SunwellPlateauScriptName, 580) { }

        struct instance_sunwell_plateau_InstanceMapScript : public InstanceScript
        {
            instance_sunwell_plateau_InstanceMapScript(InstanceMap* map) : InstanceScript(map)
            {
                SetHeaders(DataHeader);
                SetBossNumber(EncounterCount);
                LoadDoorData(doorData);
                LoadObjectData(creatureData, nullptr);
                LoadBossBoundaries(boundaries);
                LoadDungeonEncounterData(encounters);
            }

            Player const* GetPlayerInMap() const
            {
                Map::PlayerList const& players = instance->GetPlayers();

                if (!players.empty())
                {
                    for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                    {
                        Player* player = itr->GetSource();
                        if (player && !player->HasAura(45839))
                            return player;
                    }
                }
                else
                    TC_LOG_DEBUG("scripts", "Instance Sunwell Plateau: GetPlayerInMap, but PlayerList is empty!");

                return nullptr;
            }

            ObjectGuid GetGuidData(uint32 id) const override
            {
                switch (id)
                {
                    case DATA_PLAYER_GUID:
                    {
                        Player const* target = GetPlayerInMap();
                        return target ? target->GetGUID() : ObjectGuid::Empty;
                    }
                    default:
                        break;
                }
                return ObjectGuid::Empty;
            }
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const override
        {
            return new instance_sunwell_plateau_InstanceMapScript(map);
        }
};

void AddSC_instance_sunwell_plateau()
{
    new instance_sunwell_plateau();
}
