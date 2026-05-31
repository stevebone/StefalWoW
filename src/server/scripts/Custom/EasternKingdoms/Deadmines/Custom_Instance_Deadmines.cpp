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

#include "Map.h"
#include "PhasingHandler.h"
#include "ScriptMgr.h"
#include "InstanceScript.h"

#include "Custom_Instance_Deadmines.h"
#include "Custom_DeadminesOLD_Defines.h"

namespace Scripts::EasternKingdoms::Deadmines
{

    class custom_instance_deadmines : public InstanceMapScript
    {
    public:
        custom_instance_deadmines() : InstanceMapScript(Misc::DMScriptName, 36) {  }

        struct custom_instance_deadmines_InstanceMapScript : public InstanceScript
        {
            custom_instance_deadmines_InstanceMapScript(InstanceMap* map) : InstanceScript(map),
                _version(*this, "DeadminesVersion", Version::NotSet),
                _cannonState(*this, "CannonState", CannonEvent::STATE_CANNON_NOT_USED),
                _secondSmiteAlarm(*this, "SecondSmiteAlarm", 0)
            {
                SetHeaders(Misc::DataHeader);
                SetBossNumber(Misc::EncounterCount);
                LoadObjectData(modernCreatureData, {});
                LoadDoorData(doorData);
                LoadDungeonEncounterData(modernEncounters);
            }

            bool SetBossState(uint32 id, EncounterState state) override
            {
                return InstanceScript::SetBossState(id, state);
            }

            bool IsClassicVersion() const
            {
                return _version == Version::Classic;
            }

            void SetDeadminesVersion(uint8 version)
            {
                if (_version != Version::NotSet)
                    return;

                _version = version;
            }

            void SetData(uint32 type, uint32 data) override
            {
                switch (type)
                {
                    case Data::DeadminesVersion:
                        SetDeadminesVersion(uint8(data));
                        break;
                    case CannonEvent::DATA_EVENT:
                        _cannonState = data;
                        break;
                    case MiscOLD::SecondSmiteAlarm:
                        _secondSmiteAlarm = data;
                        break;
                    default:
                        break;
                }
            }

            uint32 GetData(uint32 type) const override
            {
                switch (type)
                {
                    case Data::DeadminesVersion:
                        return _version;
                    case CannonEvent::DATA_EVENT:
                        return _cannonState;
                    case MiscOLD::SecondSmiteAlarm:
                        return _secondSmiteAlarm;
                    default:
                        return 0;
                }
            }

            void OnGameObjectCreate(GameObject* go) override
            {
                InstanceScript::OnGameObjectCreate(go);

                switch (go->GetEntry())
                {
                case Objects::IronCladDoor:
                    _ironCladDoorGUID = go->GetGUID();
                    break;
                case Objects::DefiasCannon:
                    _defiasCannonGUID = go->GetGUID();
                    break;
                case Objects::DoorLever:
                    _doorLeverGUID = go->GetGUID();
                    break;
                case ObjectsOLD::MrSmiteChest:
                    _mrSmiteChestGUID = go->GetGUID();
                    break;
                default:
                    break;
                }
            }

            void Update(uint32 diff) override
            {
                // Defias Cannon Event only runs in Classic version
                if (!IsClassicVersion())
                    return;

                // Defias Cannon Event Starts Here
                GameObject* IronCladDoor = instance->GetGameObject(_ironCladDoorGUID);
                if (!IronCladDoor)
                    return;

                switch (_cannonState)
                {
                case CannonEvent::STATE_CANNON_GUNPOWDER_USED:
                    _cannonBlastTimer = CannonEvent::BLAST_TIMER;
                    _cannonState = CannonEvent::STATE_CANNON_BLAST_INITIATED;
                    break;
                case CannonEvent::STATE_CANNON_BLAST_INITIATED:
                    if (_cannonBlastTimer <= diff)
                    {
                        SummonCreatures();
                        ShootCannon();
                        BlastOutDoor();
                        LeverStuck();
                        instance->LoadGrid(-22.8f, -797.24f); // Loads Mr. Smite's grid.
                        if (Creature* smite = instance->GetCreatureBySpawnId(SpawnsOLD::MrSmite)) // goes off when door blows up
                            smite->AI()->Talk(TextsOLD::SmiteAlarm1);
                        _piratesTimer = CannonEvent::PIRATES_TIMER;
                        _cannonState = CannonEvent::STATE_PIRATES_ATTACK;
                    }
                    else _cannonBlastTimer -= diff;
                    break;
                case CannonEvent::STATE_PIRATES_ATTACK:
                    if (_piratesTimer <= diff)
                    {
                        MoveCreaturesInside();
                        _cannonState = CannonEvent::STATE_DONE;
                    }
                    else _piratesTimer -= diff;
                    break;
                }
            }

            void SummonCreatures()
            {
                if (GameObject* pIronCladDoor = instance->GetGameObject(_ironCladDoorGUID))
                {
                    if(Creature* DefiasPirate1 = pIronCladDoor->SummonCreature(CreaturesOLD::DefiasPirate,
                        pIronCladDoor->GetPositionX() - 2,
                        pIronCladDoor->GetPositionY() - 7,
                        pIronCladDoor->GetPositionZ(),
                        0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3s))
                    {
                        _defiasPirate1GUID = DefiasPirate1->GetGUID();
                        DefiasPirate1->RemoveAllAuras();
                    }

                    if (Creature* DefiasPirate2 = pIronCladDoor->SummonCreature(CreaturesOLD::DefiasPirate,
                        pIronCladDoor->GetPositionX() + 3,
                        pIronCladDoor->GetPositionY() - 6,
                        pIronCladDoor->GetPositionZ(),
                        0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3s))
                    {
                        _defiasPirate2GUID = DefiasPirate2->GetGUID();
                        DefiasPirate2->RemoveAllAuras();
                    }
                }
            }

            void MoveCreaturesInside()
            {
                if (!_defiasPirate1GUID || !_defiasPirate2GUID)
                    return;

                Creature* pDefiasPirate1 = instance->GetCreature(_defiasPirate1GUID);
                Creature* pDefiasPirate2 = instance->GetCreature(_defiasPirate2GUID);
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
                if (GameObject* pDefiasCannon = instance->GetGameObject(_defiasCannonGUID))
                {
                    pDefiasCannon->SetGoState(GO_STATE_ACTIVE);
                    pDefiasCannon->PlayDirectSound(SoundsOLD::CannonFire);
                }
            }

            void BlastOutDoor()
            {
                if (GameObject* pIronCladDoor = instance->GetGameObject(_ironCladDoorGUID))
                {
                    pIronCladDoor->SetGoState(GO_STATE_DESTROYED);
                    pIronCladDoor->PlayDirectSound(SoundsOLD::DestroyDoor);
                }
            }

            void LeverStuck()
            {
                if (GameObject* pDoorLever = instance->GetGameObject(_doorLeverGUID))
                    pDoorLever->SetFlag(GO_FLAG_INTERACT_COND);
            }

            ObjectGuid GetGuidData(uint32 data) const override
            {
                switch (data)
                {
                case MiscOLD::DATA_SMITE_CHEST:
                    return _mrSmiteChestGUID;
                }

                return ObjectGuid::Empty;
            }

        private:
            PersistentInstanceScriptValue<uint8> _version;
            PersistentInstanceScriptValue<uint8> _cannonState;
            PersistentInstanceScriptValue<uint8> _secondSmiteAlarm;

            uint32 _cannonBlastTimer = 0;
            uint32 _piratesTimer = 0;

            ObjectGuid _ironCladDoorGUID;
            ObjectGuid _doorLeverGUID;
            ObjectGuid _defiasCannonGUID;
            ObjectGuid _mrSmiteChestGUID;

            ObjectGuid _defiasPirate1GUID;
            ObjectGuid _defiasPirate2GUID;
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const override
        {
            return new custom_instance_deadmines_InstanceMapScript(map);
        }
    };
}

void AddSC_custom_instance_deadmines()
{
    using namespace Scripts::EasternKingdoms::Deadmines;

    new custom_instance_deadmines();
}
