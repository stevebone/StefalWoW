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
                _phaseSyncTimer(1000),
                _phaseSyncCount(0)
            {
                SetHeaders(Misc::DataHeader);
                SetBossNumber(Misc::EncounterCount);
                LoadObjectData(modernCreatureData, {});
                LoadDoorData(doorData);
                LoadDungeonEncounterData(modernEncounters);
            }

            void OnPlayerEnter(Player* player) override
            {
                SyncPlayerPhase(player);
                _phaseSyncCount = 10;
                _phaseSyncTimer = 1000;
            }

            void AfterDataLoad() override
            {
                if (GetBossState(DataTypesOLD::BOSS_RHAHKZOR) == DONE && _version == Version::NotSet)
                    _version.LoadValue(Version::Classic);

                if (IsClassicVersion())
                    _phaseSyncCount = 10;
            }

            void Update(uint32 diff) override
            {
                if (!_phaseSyncCount)
                    return;

                if (_phaseSyncTimer > diff)
                {
                    _phaseSyncTimer -= diff;
                    return;
                }

                SyncPlayerPhases();
                --_phaseSyncCount;
                _phaseSyncTimer = 1000;
            }

            bool SetBossState(uint32 id, EncounterState state) override
            {
                if (!InstanceScript::SetBossState(id, state))
                    return false;

                if (id == DataTypes::BOSS_GLUBTOK && state == DONE && _version == Version::NotSet)
                {
                    if (HasPlayerInOldPhase())
                        SetDeadminesVersion(Version::Classic);
                    else
                        SetDeadminesVersion(Version::Modern);
                }

                return true;
            }

            bool HasPlayerInOldPhase() const
            {
                Map::PlayerList const& players = instance->GetPlayers();

                for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                    if (Player* player = itr->GetSource())
                        if (player->GetPhaseShift().HasPhase(MiscOLD::PhaseDeadminesOLD))
                            return true;

                return false;
            }

            bool IsClassicVersion() const
            {
                return _version == Version::Classic || GetBossState(DataTypesOLD::BOSS_RHAHKZOR) == DONE;
            }

            void SetDeadminesVersion(uint8 version)
            {
                if (_version != Version::NotSet)
                    return;

                _version = version;

                if (version == Version::Classic)
                    SyncPlayerPhases();
            }

            void SyncPlayerPhase(Player* player) const
            {
                if (!player || !IsClassicVersion())
                    return;

                PhasingHandler::OnAreaChange(player);

                if (!player->GetPhaseShift().HasPhase(MiscOLD::PhaseDeadminesOLD))
                    PhasingHandler::AddPhase(player, MiscOLD::PhaseDeadminesOLD, true);

                player->UpdateObjectVisibility();
            }

            void SyncPlayerPhases() const
            {
                Map::PlayerList const& players = instance->GetPlayers();

                for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                    SyncPlayerPhase(itr->GetSource());
            }

            void SetData(uint32 type, uint32 data) override
            {
                switch (type)
                {
                    case Data::DeadminesVersion:
                        SetDeadminesVersion(uint8(data));
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
                    default:
                        return 0;
                }
            }

        private:
            PersistentInstanceScriptValue<uint8> _version;
            uint32 _phaseSyncTimer;
            uint8 _phaseSyncCount;
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
