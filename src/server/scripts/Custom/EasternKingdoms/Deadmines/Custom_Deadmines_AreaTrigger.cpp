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

#include "AreaTrigger.h"
#include "AreaTriggerAI.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "GameObject.h"
#include "InstanceScript.h"
#include "Map.h"
#include "Player.h"
#include "ScriptMgr.h"

#include "Custom_DeadminesOLD_Defines.h"
#include "Custom_Instance_Deadmines.h"

namespace Scripts::EasternKingdoms::Deadmines
{
    // 6361 - Deadmines - Cannon Event  Door (outside)
    class deadmines_door_cannon_event_at : public AreaTriggerScript
    {
    public:
        deadmines_door_cannon_event_at() : AreaTriggerScript("deadmines_door_cannon_event_at") {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/) override
        {
            if (InstanceScript* instance = player->GetInstanceScript())
            {
                if (instance->GetData(Misc::DeadminesVersion) != Version::Classic)
                    return false;

                if (instance->GetData(MiscOLD::SecondSmiteAlarm) == 1)
                    return false;

                player->GetMap()->LoadGrid(-22.8f, -797.24f); // Loads Mr. Smite's grid.

                if (Creature* smite = player->GetMap()->GetCreatureBySpawnId(SpawnsOLD::MrSmite))
                {
                    instance->SetData(MiscOLD::SecondSmiteAlarm, 1);
                    smite->AI()->Talk(TextsOLD::SmiteAlarm2);
                }
            }
            return true;
        }
    };

    // 3746 - Deadmines - Mysterious Chest
    class deadmines_mysterious_chest_at : public AreaTriggerScript
    {
        class CannonFireEvent : public BasicEvent
        {
        public:
            CannonFireEvent(Creature* cannon, Creature* bunny, uint32 spellId)
                : _cannon(cannon), _bunny(bunny), _spellId(spellId) { }

            bool Execute(uint64 /*time*/, uint32 /*diff*/) override
            {
                if (!_cannon || !_bunny)
                    return true;

                // Cast spell at bunny's position
                _cannon->CastSpell(_bunny->GetPosition(), _spellId, true);

                // Schedule displayid change after 2 seconds
                _bunny->m_Events.AddEventAtOffset([bunny = _bunny]()
                {
                    if (bunny)
                        bunny->SetDisplayId(36147);
                }, 2s);

                return true;
            }

        private:
            Creature* _cannon;
            Creature* _bunny;
            uint32 _spellId;
        };

    public:
        deadmines_mysterious_chest_at() : AreaTriggerScript("deadmines_mysterious_chest_at") {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/) override
        {
            QuestStatus status = player->GetQuestStatus(Quests::YourFortuneAwaitsInDeadmines);

            if (status == QUEST_STATUS_COMPLETE)
            {
                GameObject* chest = player->FindNearestGameObjectWithOptions(50.f, { .GameObjectId = Objects::MysteriousDeadminesChest, .PrivateObjectOwnerGuid = player->GetGUID() });
                if (chest)
                    return false;

                if (GameObject* chest = player->SummonGameObject(Objects::MysteriousDeadminesChest, Positions::MysteriousDeadminesChest, QuaternionData(), 0s, GO_SUMMON_TIMED_DESPAWN))
                {
                    chest->SetPrivateObjectOwner(player->GetGUID());
                    chest->DespawnOrUnsummon(5min);
                }
            }

            // Entrance cannon event
            if (InstanceScript* instance = player->GetInstanceScript())
            {
                if (instance->GetData(Misc::EntranceCannonFired) == 0)
                {
                    Creature* cannon = player->FindNearestCreature(Creatures::DefiasCannon, 200.0f);
                    Creature* bunny = player->FindNearestCreature(Creatures::SchorchMarkBunny, 200.0f);

                    if (cannon && bunny)
                    {
                        instance->SetData(Misc::EntranceCannonFired, 1);

                        // Schedule cannon fire event with 5 second delay
                        player->m_Events.AddEventAtOffset(new CannonFireEvent(cannon, bunny, Spells::DefiasCannonCannonballFire), 5s);
                    }
                }
            }

            return true;
        }
    };
}

void AddSC_custom_deadmines_areatrigger()
{
    using namespace Scripts::EasternKingdoms::Deadmines;

    new deadmines_mysterious_chest_at();
    new deadmines_door_cannon_event_at();
}
