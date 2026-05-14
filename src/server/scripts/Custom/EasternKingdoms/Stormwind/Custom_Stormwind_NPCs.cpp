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
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "TaskScheduler.h"

#include "Custom_Stormwind_Defines.h"

namespace Scripts::EasternKingdoms::StormwindCity
{
    // 61796 - King Varian (Spawned for Quest: The Alliance Way)
    // Also Quest: An Old Pit Fighter
    struct npc_king_varian_61796 : public ScriptedAI
    {
        npc_king_varian_61796(Creature* creature) : ScriptedAI(creature) {}

        void Reset() override
        {
            _events.Reset();
            hitMe = false;
            _events.ScheduleEvent(Events::KingVarianSpawnedStartPath, 2s);
            _events.ScheduleEvent(Events::KingVarianSpawnedStartDialogue, 5s);
        }

        void SetGUID(ObjectGuid const& guid, int32 id) override
        {
            if (id == 1)
                _playerGUID = guid;
        }

        void OnQuestAccept(Player* player, Quest const* quest) override
        {
            if (quest->GetQuestId() == Quests::AnOldPitFighter)
            {
                Creature* aysa = player->FindNearestCreature(Creatures::AysaSpawnedAtGate, 50.f);
                if (aysa && aysa->IsAlive())
                {
                    aysa->GetMotionMaster()->Clear();
                    aysa->GetMotionMaster()->MovePoint(3, Positions::AysaStormwindKeepVarianSpar);
                }

                Creature* jojo = player->FindNearestCreature(Creatures::JojoSpawnedAtGate, 50.f);
                if (jojo && jojo->IsAlive())
                {
                    jojo->GetMotionMaster()->Clear();
                    jojo->GetMotionMaster()->MovePoint(3, Positions::JojoStormwindKeepVarianSpar);
                }

                me->m_Events.AddEventAtOffset([this, player]()
                    {
                        if (me && me->IsAlive())
                        {
                            PhasingHandler::AddPhase(me, Phases::StormwindKeepKingVarianSpar, false);
                            player->AddAura(Spells::VariansPhaseInvisAura, player);

                            Talk(15);
                            _events.ScheduleEvent(Events::KingVarianCombatStart, 10s);
                        }
                    }, std::chrono::seconds(3));
            }
        }

        void SpellHit(WorldObject* caster, SpellInfo const* spellInfo) override
        {
            if (spellInfo->Id == Spells::PandarenQuackingPalm && hitMe)
            {
                me->RemoveAllAuras();
                me->SetStandState(UNIT_STAND_STATE_STAND);

                Player* player = caster->ToPlayer();
                if (player)
                {
                    if (!sObjectMgr->GetSceneTemplate(Misc::KingVarianSparScene))
                        return;

                    player->GetSceneMgr().PlayScene(Misc::KingVarianSparScene);
                    me->HandleEmoteCommand(Emote(482));
                    me->RestoreFaction();
                    player->CombatStop(true);
                    me->ClearInCombat();
                    _events.ScheduleEvent(Events::KingVarianCombatEnd, 10s);
                }
            }
        }

        void WaypointPathEnded(uint32 /*nodeId*/, uint32 pathId) override
        {
            if (pathId == Paths::StormwindKeepKingVarian)
            {
                if (Player* player = ObjectAccessor::GetPlayer(*me, _playerGUID))
                {
                    player->KilledMonsterCredit(Creatures::CreditWalkWithKingVarian);
                    me->m_Events.AddEventAtOffset([this, player]()
                        {
                            if (me && me->IsAlive())
                                Talk(14, player);
                        }, std::chrono::seconds(9));
                }
            }
        }

        void MovementInform(uint32 type, uint32 pointId) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (pointId == 1)
            {
                Player* player = ObjectAccessor::GetPlayer(*me, _playerGUID);
                if (player)
                {
                    PhasingHandler::AddPhase(player, Phases::StormwindKeepKingVarian, true);
                    me->DespawnOrUnsummon();
                }
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

                case Events::KingVarianCombatStart:
                {
                    Talk(16); // Now, pandaren... let me see what you''ve got!
                    me->SetFaction(14); // may need to find a more appropriate faction
                    me->SetEmoteState(Emote(663));

                    me->m_Events.AddEventAtOffset([this]()
                        {
                            if (me && me->IsAlive())
                                Talk(17); // Don't hold back now! Let's have it!
                        }, std::chrono::seconds(7));

                    me->m_Events.AddEventAtOffset([this]()
                        {
                            if (me && me->IsAlive())
                                Talk(18); // What's the matter? You'll have to do better than that.
                        }, std::chrono::seconds(15));

                    Player* player = ObjectAccessor::GetPlayer(*me, _playerGUID);
                    if (player && player->IsAlive())
                    {
                        Creature* aysa = player->FindNearestCreature(Creatures::AysaAtStormwindGateKeep, 50.f);
                        if (aysa && aysa->IsAlive())
                        {
                            aysa->m_Events.AddEventAtOffset([aysa, player]()
                                {
                                    if (aysa && aysa->IsAlive())
                                        aysa->AI()->Talk(0, player); // Careful, $p! You do not want to hurt your new king...
                                }, std::chrono::seconds(20));
                        }
                    }

                    me->m_Events.AddEventAtOffset([this]()
                        {
                            if (me && me->IsAlive())
                                Talk(19); // Come on, pandaren! HIT ME!
                            hitMe = true; // now we can hit the mofo
                        }, std::chrono::seconds(25));

                    break;
                }

                case Events::KingVarianCombatEnd:
                {
                    Player* player = ObjectAccessor::GetPlayer(*me, _playerGUID);
                    if (player && player->IsAlive())
                    {
                        me->SetStandState(UNIT_STAND_STATE_STAND);

                        player->KilledMonsterCredit(Creatures::CreditFightWithKingVarian);
                        Talk(20, player);

                        _events.ScheduleEvent(Events::KingVarianSpawnedFinalPath, 5s);

                        PhasingHandler::RemovePhase(me, Phases::StormwindKeepKingVarianSpar, false);
                        player->RemoveAurasDueToSpell(Spells::VariansPhaseInvisAura);

                        Creature* aysa = player->FindNearestCreature(Creatures::AysaSpawnedAtGate, 50.f);
                        if (aysa && aysa->IsAlive())
                        {
                            aysa->GetMotionMaster()->Clear();
                            aysa->GetMotionMaster()->MoveFollow(player, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
                        }

                        Creature* jojo = player->FindNearestCreature(Creatures::JojoSpawnedAtGate, 50.f);
                        if (jojo && jojo->IsAlive())
                        {
                            jojo->GetMotionMaster()->Clear();
                            jojo->GetMotionMaster()->MoveFollow(player, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
                        }
                    }
                    break;
                }

                case Events::KingVarianSpawnedFinalPath:
                {
                    Creature* phasedVarian = me->FindNearestCreatureWithOptions(100.f, { .CreatureId = Creatures::KingVarianPhased, .IgnorePhases = true });

                    if (phasedVarian)
                    {
                        me->GetMotionMaster()->Clear();
                        me->GetMotionMaster()->MovePoint(1, phasedVarian->GetPosition());
                    }
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
        bool hitMe = false;
    };
}

void AddSC_custom_stormwind_npcs()
{
    using namespace Scripts::EasternKingdoms::StormwindCity;

    RegisterCreatureAI(npc_king_varian_61796);
}
