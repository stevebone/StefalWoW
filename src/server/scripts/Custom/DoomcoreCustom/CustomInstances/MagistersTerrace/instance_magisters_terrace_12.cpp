/*
 * DoomCore Scripts - Magister's Terrace 12.0.1 Revamp (Map 2811)
 * Instance Script
 */

#include "ScriptMgr.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "GameObject.h"
#include "InstanceScript.h"
#include "magisters_terrace_12.h"
#include "Map.h"

/*
0  - Arcanotron Custos
1  - Seranel Sunlash
2  - Gemellus
3  - Degentrius
*/

static constexpr ObjectData creatureData[] =
{
    { BOSS_ARCANOTRON_CUSTOS,       DATA_ARCANOTRON_CUSTOS      },
    { BOSS_SERANEL_SUNLASH,         DATA_SERANEL_SUNLASH        },
    { BOSS_GEMELLUS,                DATA_GEMELLUS               },
    { BOSS_DEGENTRIUS,              DATA_DEGENTRIUS             },
    { NPC_MAGISTER_UMBRIC,          DATA_MAGISTER_UMBRIC        },
    { NPC_GRAND_MAGISTER_ROMMATH,   DATA_GRAND_MAGISTER_ROMMATH },
};

static constexpr ObjectData gameObjectData[] =
{
    { GO_CYNOSURE_OF_TWILIGHT,      DATA_CYNOSURE_OF_TWILIGHT   },
};

static constexpr DoorData doorData[] =
{
    { GO_SUNWELL_DOOR_1,    DATA_ARCANOTRON_CUSTOS,     EncounterDoorBehavior::OpenWhenDone              },
    { GO_SUNWELL_DOOR_2,    DATA_SERANEL_SUNLASH,       EncounterDoorBehavior::OpenWhenDone              },
    { GO_RAMPART_DOOR,      DATA_GEMELLUS,              EncounterDoorBehavior::OpenWhenDone              },
};

// DungeonEncounter IDs are not yet in the DB2 client data for Map 2811.
// Uncomment and replace with real IDs once they are available.
// static constexpr DungeonEncounterData encounters[] =
// {
//     { DATA_ARCANOTRON_CUSTOS,       {{ 2878 }} },
//     { DATA_SERANEL_SUNLASH,         {{ 2879 }} },
//     { DATA_GEMELLUS,                {{ 2880 }} },
//     { DATA_DEGENTRIUS,              {{ 2881 }} },
// };

class instance_magisters_terrace_12 : public InstanceMapScript
{
    public:
        instance_magisters_terrace_12() : InstanceMapScript(MT12ScriptName, 2811) { }

        struct instance_magisters_terrace_12_InstanceMapScript : public InstanceScript
        {
            instance_magisters_terrace_12_InstanceMapScript(InstanceMap* map) : InstanceScript(map)
            {
                SetHeaders(MT12DataHeader);
                SetBossNumber(MT12EncounterCount);
                LoadObjectData(creatureData, gameObjectData);
                LoadDoorData(doorData);
                // LoadDungeonEncounterData(encounters); // Enable once real DungeonEncounter DB2 IDs are known
            }

            void OnCreatureCreate(Creature* creature) override
            {
                InstanceScript::OnCreatureCreate(creature);

                switch (creature->GetEntry())
                {
                    case NPC_ENERGY_ORB:
                        _energyOrbGUIDs.insert(creature->GetGUID());
                        break;
                    case NPC_GEMELLUS_CLONE:
                        _gemellusCloneGUIDs.insert(creature->GetGUID());
                        break;
                    default:
                        break;
                }
            }

            void OnCreatureRemove(Creature* creature) override
            {
                InstanceScript::OnCreatureRemove(creature);

                switch (creature->GetEntry())
                {
                    case NPC_ENERGY_ORB:
                        _energyOrbGUIDs.erase(creature->GetGUID());
                        break;
                    case NPC_GEMELLUS_CLONE:
                        _gemellusCloneGUIDs.erase(creature->GetGUID());
                        break;
                    default:
                        break;
                }
            }

            bool SetBossState(uint32 type, EncounterState state) override
            {
                if (!InstanceScript::SetBossState(type, state))
                    return false;

                switch (type)
                {
                    case DATA_ARCANOTRON_CUSTOS:
                        if (state == DONE)
                        {
                            if (Creature* umbric = GetCreature(DATA_MAGISTER_UMBRIC))
                                umbric->AI()->DoAction(ACTION_UMBRIC_BOSS1_DONE);
                        }
                        break;
                    case DATA_SERANEL_SUNLASH:
                        if (state == DONE)
                        {
                            if (Creature* umbric = GetCreature(DATA_MAGISTER_UMBRIC))
                                umbric->AI()->DoAction(ACTION_UMBRIC_BOSS2_DONE);
                        }
                        break;
                    case DATA_GEMELLUS:
                        if (state == DONE)
                        {
                            if (Creature* umbric = GetCreature(DATA_MAGISTER_UMBRIC))
                                umbric->AI()->DoAction(ACTION_UMBRIC_BOSS3_DONE);
                        }
                        break;
                    case DATA_DEGENTRIUS:
                        if (state == DONE)
                        {
                            if (Creature* umbric = GetCreature(DATA_MAGISTER_UMBRIC))
                                umbric->AI()->DoAction(ACTION_UMBRIC_BOSS4_DONE);
                        }
                        break;
                    default:
                        break;
                }
                return true;
            }

            GuidSet const& GetEnergyOrbGUIDs() const { return _energyOrbGUIDs; }
            GuidSet const& GetGemellusCloneGUIDs() const { return _gemellusCloneGUIDs; }

        protected:
            GuidSet _energyOrbGUIDs;
            GuidSet _gemellusCloneGUIDs;
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const override
        {
            return new instance_magisters_terrace_12_InstanceMapScript(map);
        }
};

void AddSC_instance_magisters_terrace_12()
{
    new instance_magisters_terrace_12();
}
