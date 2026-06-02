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
    public:
        deadmines_mysterious_chest_at() : AreaTriggerScript("deadmines_mysterious_chest_at") {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/) override
        {
            QuestStatus status = player->GetQuestStatus(Quests::YourFortuneAwaitsInDeadmines);

            if (status == QUEST_STATUS_COMPLETE)
            {
                if (GameObject* chest = player->FindNearestGameObjectWithOptions(50.f, { .GameObjectId = Objects::MysteriousDeadminesChest, .PrivateObjectOwnerGuid = player->GetGUID() }))
                    return false;

                if (GameObject* chest = player->SummonGameObject(Objects::MysteriousDeadminesChest, Positions::MysteriousDeadminesChest, QuaternionData(), 0s, GO_SUMMON_TIMED_DESPAWN))
                {
                    chest->SetPrivateObjectOwner(player->GetGUID());
                    chest->DespawnOrUnsummon(5min);
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
