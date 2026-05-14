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
#include "CreatureAIImpl.h"
#include "EventMap.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "TaskScheduler.h"

#include "Custom_Stormwind_Defines.h"

namespace Scripts::EasternKingdoms::StormwindCity
{
    // 61796 - King Varian (Spawned for Quest: The Alliance Way)
    struct npc_king_varian_61796 : public ScriptedAI
    {
        npc_king_varian_61796(Creature* creature) : ScriptedAI(creature) {}

        void Reset() override
        {
            _events.Reset();
            _events.ScheduleEvent(Events::KingVarianSpawnedStartPath, 2s);
            _events.ScheduleEvent(Events::KingVarianSpawnedStartDialogue, 5s);
        }

        void SetGUID(ObjectGuid const& guid, int32 id) override
        {
            if (id == 1)
                _playerGUID = guid;
        }

        void WaypointPathEnded(uint32 /*nodeId*/, uint32 /*pathId*/) override
        {
            if (Player* player = ObjectAccessor::GetPlayer(*me, _playerGUID))
            {
                player->KilledMonsterCredit(Creatures::CreditWalkWithKingVarian);
                me->m_Events.AddEventAtOffset([this, player]()
                    {
                        if (me && me->IsAlive())
                            Talk(14, player);
                    }, std::chrono::seconds(7));
            }
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                case Events::KingVarianSpawnedStartPath:
                    me->GetMotionMaster()->MovePath(Paths::StormwindKeepKingVarian, false);
                    break;

                case Events::KingVarianSpawnedStartDialogue:
                    StartDialogue();
                    break;

                case Events::KingVarianDialogueStep:
                {
                    if (_dialogueIndex >= std::size(Dialogue::VarianDialogue))
                        break;

                    DialogueEntry const& entry = Dialogue::VarianDialogue[_dialogueIndex];

                    Creature* speaker = nullptr;

                    if (entry.NpcEntry == me->GetEntry())
                        speaker = me;
                    else
                    {
                        // We are using the Player to find Aysa as she may be a private object
                        // This ensures we only find Aysa of the current player
                        Player* player = ObjectAccessor::GetPlayer(*me, _playerGUID);
                        if(player)
                            speaker = player->FindNearestCreature(entry.NpcEntry, 50.f);
                    }

                    if (speaker && speaker->IsAlive())
                        speaker->AI()->Talk(entry.TalkId);

                    ++_dialogueIndex;

                    if (!entry.IsFinal)
                        _events.ScheduleEvent(Events::KingVarianDialogueStep, entry.NextDelay);

                    break;
                }
                }
            }
        }

        void StartDialogue()
        {
            _dialogueIndex = 0;
            // Start dialogue sequence
            _events.ScheduleEvent(Events::KingVarianDialogueStep, 0s);
        }

    private:
        EventMap _events;
        ObjectGuid _playerGUID;
        size_t _dialogueIndex = 0;
    };
}

void AddSC_custom_stormwind_npcs()
{
    using namespace Scripts::EasternKingdoms::StormwindCity;

    RegisterCreatureAI(npc_king_varian_61796);
}
