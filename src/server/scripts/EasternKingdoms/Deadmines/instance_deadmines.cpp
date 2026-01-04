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
#include "deadmines.h"
#include "InstanceScript.h"

ObjectData const creatureData[] =
{
    { NPC_GLUBTOK,              BOSS_GLUBTOK            },
    { NPC_HELIX_GEARBREAKER,    BOSS_HELIX_GEARBREAKER  },
    { NPC_FOE_REAPER_5000,      BOSS_FOE_REAPER_5000    },
    { NPC_ADMIRAL_RIPSNARL,     BOSS_ADMIRAL_RIPSNARL   },
    { NPC_CAPTAIN_COOKIE,       BOSS_CAPTAIN_COOKIE     },
    { NPC_VANESSA_VAN_CLEEF,    BOSS_VANESSA_VANCLEEF   },
    { 0,                        0                       } // END
};

DoorData const doorData[] =
{
    { GO_FACTORY_DOOR,      BOSS_GLUBTOK,           EncounterDoorBehavior::OpenWhenDone             },
    { GO_MAST_ROOM_DOOR,    BOSS_HELIX_GEARBREAKER, EncounterDoorBehavior::OpenWhenDone             },
    { GO_HEAVY_DOOR,        BOSS_HELIX_GEARBREAKER, EncounterDoorBehavior::OpenWhenNotInProgress    },
    { GO_FOUNDRY_DOOR,      BOSS_FOE_REAPER_5000,   EncounterDoorBehavior::OpenWhenDone             },
    { 0,                    0,                      EncounterDoorBehavior::OpenWhenDone             } // END
};

DungeonEncounterData const encounters[] =
{
    { BOSS_GLUBTOK,             {{ 2976, 2981 }}  },
    { BOSS_HELIX_GEARBREAKER,   {{ 2977, 2982 }}  },
    { BOSS_FOE_REAPER_5000,     {{ 2975, 2980 }}  },
    { BOSS_ADMIRAL_RIPSNARL,    {{ 2974, 2979 }}  },
    { BOSS_CAPTAIN_COOKIE,      {{ 2973, 2978 }}  },
    { BOSS_VANESSA_VANCLEEF,    {{ 1081 }}  }
};

enum Sounds
{
    SOUND_CANNONFIRE = 1400,
    SOUND_DESTROYDOOR = 3079,
    SOUND_MR_SMITE_ALARM1 = 5775,
    SOUND_MR_SMITE_ALARM2 = 5777
};

//#define SAY_MR_SMITE_ALARM1 "You there, check out that noise!"
//#define SAY_MR_SMITE_ALARM2 "We're under attack! A vast, ye swabs! Repel the invaders!"

enum Misc
{
    DATA_CANNON_BLAST_TIMER = 3000,
    DATA_PIRATES_DELAY_TIMER = 1000
};

class instance_deadmines : public InstanceMapScript
{
public:
    instance_deadmines() : InstanceMapScript(DMScriptName, 36) {  }

    struct instance_deadmines_InstanceMapScript : public InstanceScript
    {
        instance_deadmines_InstanceMapScript(InstanceMap* map) : InstanceScript(map)
        {
            SetHeaders(DataHeader);
            SetBossNumber(EncounterCount);
            LoadObjectData(creatureData, nullptr);
            LoadDoorData(doorData);
            LoadDungeonEncounterData(encounters);

            State = CANNON_NOT_USED;
            CannonBlast_Timer = 0;
            PiratesDelay_Timer = 0;
        }

        //ObjectGuid FactoryDoorGUID;
        ObjectGuid IronCladDoorGUID;
        ObjectGuid DefiasCannonGUID;
        ObjectGuid DoorLeverGUID;
        ObjectGuid DefiasPirate1GUID;
        ObjectGuid DefiasPirate2GUID;
        ObjectGuid MrSmiteGUID;

        uint32 State;
        uint32 CannonBlast_Timer;
        uint32 PiratesDelay_Timer;
        ObjectGuid uiSmiteChestGUID;

        virtual void Update(uint32 diff) override
        {
            /*
            if (!IronCladDoorGUID)
                TC_LOG_DEBUG("scripts.ai.core", "DEFIAS CANNON Missing IronCladDoorGUID");
            if (!DefiasCannonGUID)
                TC_LOG_DEBUG("scripts.ai.core", "DEFIAS CANNON Missing DefiasCannonGUID");
            if (!DoorLeverGUID)
                TC_LOG_DEBUG("scripts.ai.core", "DEFIAS CANNON Missing DoorLeverGUID");
            */

            if (!IronCladDoorGUID || !DefiasCannonGUID || !DoorLeverGUID)
            {
                return;
            }

            GameObject* pIronCladDoor = instance->GetGameObject(IronCladDoorGUID);
            if (!pIronCladDoor)
            {
                //TC_LOG_DEBUG("scripts.ai.core", "DEFIAS CANNON We do not have Door GO");
                return;
            }

            switch (State)
            {
            case CANNON_GUNPOWDER_USED:
                //TC_LOG_DEBUG("scripts.ai.core", "DEFIAS CANNON We got gunpowder");
                CannonBlast_Timer = DATA_CANNON_BLAST_TIMER;
                // it's a hack - Mr. Smite should do that but his too far away
                //pIronCladDoor->SetName("Mr. Smite");
                //pIronCladDoor->MonsterYell(SAY_MR_SMITE_ALARM1, LANG_UNIVERSAL, nullptr);
                //pIronCladDoor->PlayDirectSound(SOUND_MR_SMITE_ALARM1); // This is no longer used in modern wow
                State = CANNON_BLAST_INITIATED;
                break;
            case CANNON_BLAST_INITIATED:
                //TC_LOG_DEBUG("scripts.ai.core", "DEFIAS CANNON before blast");
                PiratesDelay_Timer = DATA_PIRATES_DELAY_TIMER;
                if (CannonBlast_Timer <= diff)
                {
                    SummonCreatures();
                    ShootCannon();
                    BlastOutDoor();
                    LeverStucked();
                    //pIronCladDoor->MonsterYell(SAY_MR_SMITE_ALARM2, LANG_UNIVERSAL, nullptr);
                    //pIronCladDoor->PlayDirectSound(SOUND_MR_SMITE_ALARM2); //this is NO longer used in modern wow
                    State = PIRATES_ATTACK;
                }
                else CannonBlast_Timer -= diff;
                break;
            case PIRATES_ATTACK:
                if (PiratesDelay_Timer <= diff)
                {
                    MoveCreaturesInside();
                    State = EVENT_DONE;
                }
                else PiratesDelay_Timer -= diff;
                break;
            }
        }

        void SummonCreatures()
        {
            if (GameObject* pIronCladDoor = instance->GetGameObject(IronCladDoorGUID))
            {
                Creature* DefiasPirate1 = pIronCladDoor->SummonCreature(NPC_DEFIAS_BLOODWIZZARD, pIronCladDoor->GetPositionX() - 2, pIronCladDoor->GetPositionY() - 7, pIronCladDoor->GetPositionZ(), 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3s);
                Creature* DefiasPirate2 = pIronCladDoor->SummonCreature(NPC_DEFIAS_ENFORCER, pIronCladDoor->GetPositionX() + 3, pIronCladDoor->GetPositionY() - 6, pIronCladDoor->GetPositionZ(), 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3s);

                DefiasPirate1GUID = DefiasPirate1->GetGUID();
                DefiasPirate2GUID = DefiasPirate2->GetGUID();
            }
        }

        void MoveCreaturesInside()
        {
            if (!DefiasPirate1GUID || !DefiasPirate2GUID)
                return;

            Creature* pDefiasPirate1 = instance->GetCreature(DefiasPirate1GUID);
            Creature* pDefiasPirate2 = instance->GetCreature(DefiasPirate2GUID);
            if (!pDefiasPirate1 || !pDefiasPirate2)
                return;

            MoveCreatureInside(pDefiasPirate1);
            MoveCreatureInside(pDefiasPirate2);
        }

        void MoveCreatureInside(Creature* creature)
        {
            creature->SetWalk(false);
            creature->GetMotionMaster()->MovePoint(0, -102.7f, -655.9f, creature->GetPositionZ());
        }

        void ShootCannon()
        {
            if (GameObject* pDefiasCannon = instance->GetGameObject(DefiasCannonGUID))
            {
                pDefiasCannon->SetGoState(GO_STATE_ACTIVE);
                pDefiasCannon->PlayDirectSound(SOUND_CANNONFIRE);
            }
        }

        void BlastOutDoor()
        {
            if (GameObject* pIronCladDoor = instance->GetGameObject(IronCladDoorGUID))
            {
                pIronCladDoor->SetGoState(GO_STATE_DESTROYED);
                pIronCladDoor->PlayDirectSound(SOUND_DESTROYDOOR);
            }
        }

        void LeverStucked()
        {
            if (GameObject* pDoorLever = instance->GetGameObject(DoorLeverGUID))
                pDoorLever->SetFlag(GO_FLAG_INTERACT_COND);
        }

        void OnGameObjectCreate(GameObject* go) override
        {
            switch (go->GetEntry())
            {
            //case GO_FACTORY_DOOR:   FactoryDoorGUID = go->GetGUID(); break;
            case GO_IRON_CLAD_DOOR:  IronCladDoorGUID = go->GetGUID();  break;
            case GO_DEFIAS_CANNON:  DefiasCannonGUID = go->GetGUID();  break;
            case GO_DOOR_LEVER:     DoorLeverGUID = go->GetGUID();     break;
            case GO_MR_SMITE_CHEST: uiSmiteChestGUID = go->GetGUID();  break;
            }
        }

        void SetData(uint32 type, uint32 data) override
        {
            switch (type)
            {
            case EVENT_STATE:
                if (!DefiasCannonGUID.IsEmpty() && !IronCladDoorGUID.IsEmpty())
                    State = data;
                break;
            //case EVENT_RHAHKZOR:
            //    if (GameObject* go = instance->GetGameObject(FactoryDoorGUID))
            //        go->SetGoState(GO_STATE_ACTIVE);
            //    break;
            }
        }

        uint32 GetData(uint32 type) const override
        {
            switch (type)
            {
            case EVENT_STATE:
                return State;
            }

            return 0;
        }
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_deadmines_InstanceMapScript(map);
    }
};

void AddSC_instance_deadmines()
{
    new instance_deadmines();
}
