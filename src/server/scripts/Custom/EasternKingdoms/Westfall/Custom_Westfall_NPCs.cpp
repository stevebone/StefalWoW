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

#include "Containers.h"
#include "Creature.h"
#include "CreatureAIImpl.h"
#include "EventMap.h"
#include "Map.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ObjectGuid.h"
#include "ObjectMgr.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "TaskScheduler.h"
#include "TemporarySummon.h"
#include "Unit.h"

#include <queue>

#include "Custom_Westfall_Defines.h"

namespace Scripts::EasternKingdoms::Westfall
{
    /*######
    ## npc_custom_westfall_stew_42617
    ######*/

    struct npc_custom_westfall_stew_42617 : public ScriptedAI
    {
        npc_custom_westfall_stew_42617(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            me->m_Events.AddEventAtOffset([this]()
                {
                    if (me && me->IsAlive())
                        FindHobosToFeed();
                }, std::chrono::seconds(1));
        }

        void FindHobosToFeed()
        {
            std::list<Creature*> list;
            GetCreatureListWithEntryInGrid(list, me, Creatures::HomelessStormwindCitizen, 10.f);
            GetCreatureListWithEntryInGrid(list, me, Creatures::HomelessStormwindCitizen2, 10.f);

            for (Creature* hungryHobo : list)
            {
                if (!hungryHobo || !hungryHobo->IsAlive())
                    continue;

                if (hungryHobo->HasAura(Spells::FullBelly))
                    continue;

                if (hungryHobo->HasAura(Spells::SleepAura))
                    hungryHobo->RemoveAurasDueToSpell(Spells::SleepAura);

                hungryHobo->SetStandState(UNIT_STAND_STATE_STAND);

                hungryHobo->m_Events.AddEventAtOffset([hungryHobo, this]()
                    {
                        if (hungryHobo && hungryHobo->IsAlive())
                            hungryHobo->GetMotionMaster()->MovePoint(1, me->GetRandomNearPosition(2.f));
                    }, std::chrono::seconds(1));

                hungryHobo->m_Events.AddEventAtOffset([hungryHobo, this]()
                    {
                        if (hungryHobo && hungryHobo->IsAlive())
                        {
                            hungryHobo->SetFacingToObject(me, true);
                            hungryHobo->SetStandState(UNIT_STAND_STATE_SIT);
                            hungryHobo->AI()->Talk(13);
                        }
                    }, std::chrono::seconds(4));

                hungryHobo->m_Events.AddEventAtOffset([hungryHobo, this]()
                    {
                        if (hungryHobo && hungryHobo->IsAlive())
                        {
                            hungryHobo->CastSpell(hungryHobo, Spells::FullBelly, true);
                            WorldObject* summoner = me->ToTempSummon()->GetSummoner();
                            if (summoner && summoner->ToPlayer())
                                summoner->ToPlayer()->KilledMonsterCredit(Creatures::WestfallStew);

                            hungryHobo->DespawnOrUnsummon(100s, 10s);
                        }
                    }, std::chrono::seconds(7));
            }

        }
    };

    /*######
    ## npc_custom_westfall_guard_42407
    ## ID 42407, SpawnID 275390
    ######*/

    struct npc_custom_westfall_guard_42407 : public ScriptedAI
    {
        npc_custom_westfall_guard_42407(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();

            // This is needed since there might be an issue with factions OR
            // The npcs at the Sentinel Hill need to be passive
            _events.ScheduleEvent(Events::WestfalGuardPacifyDrifters, 500ms);

            _events.ScheduleEvent(Events::WestfalGuardCrowdReaction, 11s, 19s); // weird timers to create randomness
            _events.ScheduleEvent(Events::WestfalGuardCrowdReaction, 13s, 26s); // weird timers to create randomness
            _events.ScheduleEvent(Events::WestfalGuardCrowdReaction, 35s, 46s); // weird timers to create randomness
        }

        void MoveInLineOfSight(Unit* who) override
        {
            ScriptedAI::MoveInLineOfSight(who);

            if (!who || !who->IsCreature())
                return;

            Creature* drifterWithCartBuddy = who->ToCreature();

            if (drifterWithCartBuddy->GetSpawnId() == Spawns::DrifterWithCartBuddy)
            {
                if (!_drifterWithCartConvoOnCooldown && me->IsWithinDistInMap(who, 15.0f))
                {
                    _drifterWithCartConvoOnCooldown = true;
                    StartGuardConversation(drifterWithCartBuddy);
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
                case Events::WestfalGuardCrowdReaction:
                {
                    // Get crowd
                    std::list<Creature*> crowd;
                    GetCreatureListWithEntryInGrid(crowd, me, Creatures::HomelessStormwindCitizen, 50.f);
                    GetCreatureListWithEntryInGrid(crowd, me, Creatures::HomelessStormwindCitizen2, 50.f);
                    GetCreatureListWithEntryInGrid(crowd, me, Creatures::Transient, 50.f);
                    GetCreatureListWithEntryInGrid(crowd, me, Creatures::WestPlainsDrifter, 50.f);
                    GetCreatureListWithEntryInGrid(crowd, me, Creatures::SmallTimeHustler, 50.f);

                    // Remove hobos that are sitting down
                    crowd.remove_if([](Creature* c)
                        {
                            return !c || c->GetStandState() == UNIT_STAND_STATE_SIT || c->HasAura(Spells::SleepAura);
                        });

                    Creature* hobo = Trinity::Containers::SelectRandomContainerElement(crowd);
                    if(!hobo || !hobo->IsAlive())
                        hobo = Trinity::Containers::SelectRandomContainerElement(crowd);

                    if (hobo)
                    {
                        // Get guard target
                        // First check if we are left/right side citizens for Hobo Stew spit
                        if (hobo->GetSpawnId() == Spawns::LeftSideCitizen ||
                            hobo->GetSpawnId() == Spawns::RightSideCitizen ||
                            hobo->GetSpawnId() == Spawns::RightBackCitizen ||
                            hobo->GetSpawnId() == Spawns::LeftBackCitizen)
                        {
                            Creature* guard = hobo->FindNearestCreature(Creatures::SentinelHillGuard, 10.f);
                            if (guard)
                                hobo->CastSpell(guard, Spells::HoboStew);

                            _events.ScheduleEvent(Events::WestfalGuardCrowdReaction, 10s, 30s); // weird timers to create randomness
                            break; // break here for this action
                        }

                        // roll for throw banana/apple VS talk
                        if (roll_chance(50))
                        {
                            uint32 spell = RAND(Spells::RottenAppleToss, Spells::RottenBananaToss);
                            Creature* guard = hobo->FindNearestCreature(Creatures::SentinelHillGuard, 20.f);
                            if (guard)
                                hobo->CastSpell(guard, spell);

                            _events.ScheduleEvent(Events::WestfalGuardCrowdReaction, 10s, 30s); // weird timers to create randomness
                            break;
                        }

                        hobo->AI()->Talk(12);
                    }

                    _events.ScheduleEvent(Events::WestfalGuardCrowdReaction, 10s, 30s); // weird timers to create randomness
                    break;
                }
                case Events::WestfalGuardPacifyDrifters:
                {
                    // We do this by adding Immune To NPC to the Homeless
                    std::list<Creature*> list;
                    GetCreatureListWithEntryInGrid(list, me, Creatures::HomelessStormwindCitizen, 50.f);
                    GetCreatureListWithEntryInGrid(list, me, Creatures::HomelessStormwindCitizen2, 50.f);
                    GetCreatureListWithEntryInGrid(list, me, Creatures::SmallTimeHustler, 50.f);

                    for (Creature* hobo : list)
                    {
                        if (!hobo->IsAlive())
                            continue;

                        hobo->SetImmuneToNPC(true);
                    }
                    break;
                }
                }
            }
        }

        void StartGuardConversation(Creature* drifter)
        {
            Talk(0);

            me->m_Events.AddEventAtOffset([this]()
                {
                    if(me && me->IsAlive())
                        Talk(1);
                }, std::chrono::seconds(4));

            drifter->m_Events.AddEventAtOffset([drifter]()
                {
                    if (drifter && drifter->IsAlive())
                        drifter->AI()->Talk(11);
                }, std::chrono::seconds(10));

            me->m_Events.AddEventAtOffset([this]()
                {
                    if (me && me->IsAlive())
                        Talk(2);
                }, std::chrono::seconds(15));

            me->m_Events.AddEventAtOffset([this]()
                {
                    if (me && me->IsAlive())
                        Talk(3);
                }, std::chrono::seconds(20));

            if (Creature* cart = me->GetMap()->GetCreatureBySpawnId(Spawns::CartBuddy))
            {
                cart->m_Events.AddEventAtOffset([cart]()
                    {
                        if (cart && cart->IsAlive())
                            cart->AI()->Talk(0);
                    }, std::chrono::seconds(25));

                cart->m_Events.AddEventAtOffset([cart, drifter]()
                    {
                        if (cart && cart->IsAlive())
                        {
                            cart->AI()->Talk(1);
                            cart->DespawnOrUnsummon(20s);
                            drifter->DespawnOrUnsummon(20s);
                        }
                    }, std::chrono::seconds(30));
            }
        }

    private:
        EventMap _events;
        bool _drifterWithCartConvoOnCooldown = false;
    };

    /*######
    ## npc_custom_lous_parting_thoughts_trigger
    ## ID 42562
    ######*/

    struct npc_custom_lous_parting_thoughts_trigger : public ScriptedAI
    {
        npc_custom_lous_parting_thoughts_trigger(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _phase = 0;
            _deadThugs = 0;
            _eventStarted = false;
            _eventLocked = false;
            _phaseTimer = 1000;

            for (auto& guid : _thugGuids)
                guid.Clear();
        }

        void MoveInLineOfSight(Unit* who) override
        {
            if (_eventStarted)
                return;

            Player* player = who->ToPlayer();
            if (!player)
                return;

            if (player->GetQuestStatus(Quests::LousPartingThoughts) != QUEST_STATUS_INCOMPLETE)
                return;

            if (!who->IsWithinDistInMap(me, 30.0f))
                return;

            _playerGuid = player->GetGUID();

            StartEvent();
        }

        void StartEvent()
        {
            _eventStarted = true;

            for (uint8 i = 0; i < 4; ++i)
            {
                if (Creature* thug = me->SummonCreature(Creatures::WestfallThug, Positions::Q26232ThugPositions[i], TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                {
                    thug->SetReactState(REACT_PASSIVE);
                    thug->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC);

                    _thugGuids[i] = thug->GetGUID();
                }
            }

            _phaseTimer = 2000;
        }

        Creature* GetThug(uint8 index)
        {
            return ObjectAccessor::GetCreature(*me, _thugGuids[index]);
        }

        void SetData(uint32 id, uint32 /*value*/) override
        {
            if (id != 1)
                return;

            ++_deadThugs;

            if (_deadThugs >= 4)
            {
                if (Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid))
                {
                    me->m_Events.AddEventAtOffset([this, player]()
                        {
                            if (me)
                                me->PlayDirectSound(Sounds::WeaponFire1, player);
                        }, std::chrono::seconds(1));

                    me->m_Events.AddEventAtOffset([this, player]()
                        {
                            if (me)
                                me->PlayDirectSound(Sounds::WeaponFire2, player);
                        }, std::chrono::seconds(2));

                    me->m_Events.AddEventAtOffset([this, player]()
                        {
                            if (me)
                                me->PlayDirectSound(Sounds::WomanScream, player);
                        }, std::chrono::seconds(3));

                    me->m_Events.AddEventAtOffset([this, player]()
                        {
                            if (me)
                                me->PlayDirectSound(Sounds::Warning, player);
                            Talk(Talks::Q26232ReturnToFurlsbrowCottage);
                        }, std::chrono::seconds(5));

                    player->KilledMonsterCredit(Creatures::LousPartingThoughtsCredit, _playerGuid);
                }
                Reset();
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!_eventStarted || _eventLocked)
                return;

            if (_phaseTimer > diff)
            {
                _phaseTimer -= diff;
                return;
            }

            // Out of bounds = event finished
            if (_phase >= std::size(Dialogue::Q26232Steps))
                return;

            auto const& step = Dialogue::Q26232Steps[_phase];

            // Handle normal talk phases
            if (step.thugIndex != 255)
            {
                if (Creature* thug = GetThug(step.thugIndex))
                    thug->AI()->Talk(step.talkId);
            }
            else
            {
                // Handle special phases
                switch (_phase)
                {
                case 4: // Face player
                    if (Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid))
                        for (auto guid : _thugGuids)
                            if (Creature* thug = ObjectAccessor::GetCreature(*me, guid))
                                thug->SetFacingToObject(player);
                    break;

                case 8: // Enable combat
                    for (auto guid : _thugGuids)
                        if (Creature* thug = ObjectAccessor::GetCreature(*me, guid))
                        {
                            thug->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC);
                            thug->SetFaction(14);
                            thug->SetReactState(REACT_AGGRESSIVE);
                        }
                    break;

                case 9: // Engage player
                    if (Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid))
                        for (auto guid : _thugGuids)
                            if (Creature* thug = ObjectAccessor::GetCreature(*me, guid))
                                thug->EngageWithTarget(player);

                    _eventLocked = true;
                    break;
                }
            }

            // Set next timer and advance phase
            _phaseTimer = step.delay;
            ++_phase;
        }

    private:
        ObjectGuid _playerGuid;
        std::array<ObjectGuid, 4> _thugGuids;
        uint32 _phaseTimer = 0;
        uint8  _phase = 0;
        uint8  _deadThugs = 0;
        bool   _eventStarted = false;
        bool   _eventLocked = false;
    };

    /*######
    ## npc_custom_lous_parting_thoughts_thug
    ## ID 42387
    ######*/

    struct npc_custom_lous_parting_thoughts_thug : public ScriptedAI
    {
        npc_custom_lous_parting_thoughts_thug(Creature* creature) : ScriptedAI(creature) { }

        void JustDied(Unit* /*killer*/) override
        {
            if (TempSummon* summon = me->ToTempSummon())
            {
                if (Creature* trigger = ObjectAccessor::GetCreature(*me, summon->GetSummonerGUID()))
                {
                    if (trigger->AI())
                        trigger->AI()->SetData(1, 1); // notify kill
                }
            }
        }

    };

    /*######
    ## npc_custom_salma_saldean_235
    ## ID 235
    ######*/

    struct npc_custom_salma_saldean_235 : public ScriptedAI
    {
        npc_custom_salma_saldean_235(Creature* creature) : ScriptedAI(creature) { }

        void OnQuestReward(Player* player, Quest const* quest, LootItemType /*type*/, uint32 /*opt*/) override
        {
            if (quest->GetQuestId() == Quests::NewWestfallStew)
            {
                // We remove the flag for the duration of the event
                // To avoid multiple triggers
                me->RemoveNpcFlag(UNIT_NPC_FLAG_QUESTGIVER); 
                Talk(0, player);
                me->SetWalk(true);

                me->m_Events.AddEventAtOffset([this]()
                    {
                        if (me && me->IsAlive())
                            me->GetMotionMaster()->MovePoint(1, Positions::SalmaSaldeanStew);
                    }, std::chrono::seconds(2));

                if (Creature* orphan = player->FindNearestCreature(Creatures::WestfallOrphan, 10.f))
                {
                    orphan->m_Events.AddEventAtOffset([orphan, player]()
                        {
                            if (orphan && orphan->IsAlive())
                                orphan->AI()->Talk(0, player);
                        }, std::chrono::seconds(15));

                    orphan->m_Events.AddEventAtOffset([orphan, player]()
                        {
                            if (orphan && orphan->IsAlive())
                            {
                                orphan->AI()->Talk(1, player);
                                orphan->AI()->Talk(2);
                            }
                        }, std::chrono::seconds(20));
                }
            }
        }

        void MovementInform(uint32 type, uint32 pointId) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (pointId == 1)
            {
                me->HandleEmoteCommand(EMOTE_ONESHOT_USE_STANDING);

                me->m_Events.AddEventAtOffset([this]()
                    {
                        if (me && me->IsAlive())
                        {
                            me->SetOrientation(2.4434f);
                            Talk(1);

                            std::list<Creature*> orphans;
                            GetCreatureListWithEntryInGrid(orphans, me, Creatures::WestfallOrphan, 20.f);

                            for (Creature* orphan : orphans)
                            {
                                if (orphan && orphan->IsAlive())
                                {
                                    orphan->GetMotionMaster()->MovePoint(1, me->GetRandomNearPosition(2.5f));
                                    orphan->DespawnOrUnsummon(15s, 1s);
                                }

                            }

                            me->DespawnOrUnsummon(15s, 1s);
                        }
                    }, std::chrono::seconds(5));

            }
        }
    };

    /*######
    ## npc_custom_gryan_stoutmantle_234
    ## ID 234
    ######*/

    struct npc_custom_gryan_stoutmantle_234 : public ScriptedAI
    {
        npc_custom_gryan_stoutmantle_234(Creature* creature) : ScriptedAI(creature) { }

        void OnQuestReward(Player* player, Quest const* quest, LootItemType /*type*/, uint32 /*opt*/) override
        {
            if (quest->GetQuestId() == Quests::TheDefiasBrotherhoodFinal)
                Talk(0, player);

            if (quest->GetQuestId() == Quests::InDefenseOfWestfall)
            {
                me->SetWalk(true);
                me->GetMotionMaster()->MovePoint(1, Positions::GryanStoutmantleTalkRipsnarl);
            }
        }

        void MovementInform(uint32 type, uint32 pointId) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (pointId == 1)
            {
                Talk(3);

                Creature* ripsnarl = me->FindNearestCreatureWithOptions(10.f, { .CreatureId = Creatures::RipsnarlAtTower, .IgnorePhases = true });
                if (ripsnarl && ripsnarl->IsAlive())
                {
                    me->SetFacingToObject(ripsnarl);

                    ripsnarl->m_Events.AddEventAtOffset([ripsnarl]()
                        {
                            if (ripsnarl && ripsnarl->IsAlive())
                                ripsnarl->AI()->Talk(1);
                        }, std::chrono::seconds(7));
                }

                me->m_Events.AddEventAtOffset([this]()
                    {
                        if (me && me->IsAlive())
                            me->AI()->Talk(4);
                    }, std::chrono::seconds(12));

                me->m_Events.AddEventAtOffset([this]()
                    {
                        if (me && me->IsAlive())
                            me->GetMotionMaster()->MovePoint(2, me->GetHomePosition());
                    }, std::chrono::seconds(17));
            }
        }
    };

    /*######
    ## npc_custom_agent_kearnen
    ## ID 7024
    ######*/

    struct npc_custom_agent_kearnen : public ScriptedAI
    {
        npc_custom_agent_kearnen(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            _playerGuid.Clear();

            std::queue<ObjectGuid> empty;
            _pendingTargets = {};
        }

        void Assist(Player* player, Creature* target)
        {
            if (!player || !target || !target->IsAlive())
                return;

            _playerGuid = player->GetGUID();
            _pendingTargets.push(target->GetGUID());
            _events.ScheduleEvent(Events::AgentKearnenCastKillShot, 5s, 10s);
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                case Events::AgentKearnenCastKillShot:
                {
                    if (_pendingTargets.empty())
                        break;

                    ObjectGuid TargetGUID = _pendingTargets.front();
                    _pendingTargets.pop();

                    Creature* target = ObjectAccessor::GetCreature(*me, TargetGUID);
                    Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid);

                    if (target && target->IsAlive() && player)
                    {
                        me->SetFacingToObject(target);
                        me->CastSpell(target, Spells::KillShot, true);
                        Talk(0, player);
                    }

                    // If more targets remain, schedule again
                    if (!_pendingTargets.empty())
                        _events.Repeat(5s, 10s);

                    break;
                }
                }
            }
        }

    private:
        EventMap _events;
        ObjectGuid _playerGuid;
        std::queue<ObjectGuid> _pendingTargets;
    };

    /*######
    ## npc_custom_elite_mercenary
    ## ID 42656
    ######*/

    struct npc_custom_elite_mercenary : public ScriptedAI
    {
        npc_custom_elite_mercenary(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _assistTriggered = false;
        }

        void JustEngagedWith(Unit* who) override
        {
            if (!who)
                return;

            Player* player = who->GetCharmerOrOwnerPlayerOrPlayerItself();
            if (!player)
                return;

            if (_assistTriggered)
                return;

            if (player->GetQuestStatus(Quests::SecretsOfTheTower) != QUEST_STATUS_INCOMPLETE)
                return;

            Creature* agent = me->FindNearestCreature(Creatures::AgentKearnen, 150.0f, true);
            if (!agent)
                return;

            if (auto* ai = CAST_AI(npc_custom_agent_kearnen, agent->AI()))
                ai->Assist(player, me);

            _assistTriggered = true;
        }

    private:
        bool _assistTriggered = false;
    };

    /*######
    ## npc_custom_trigger_mortwake_tower
    ## ID 17234
    ######*/

    struct npc_custom_trigger_mortwake_tower : public ScriptedAI
    {
        npc_custom_trigger_mortwake_tower(Creature* creature) : ScriptedAI(creature) {}

        void Reset() override
        {
            _stepIndex = 0;
            _summonTimer = 2000;
            _bSumm = false;
            _bSumm1 = false;
            _bSumm2 = false;
            _eventRunning = false;
            _playerGUID.Clear();
            _oafGUID.Clear();
            _helixGUID.Clear();
            _shadowyGUID.Clear();
        }

        void SetGUID(ObjectGuid const& guid, int32 id) override
        {
            if (id == 1 && !_eventRunning)
            {
                _playerGUID = guid;
                StartEvent();
                _eventRunning = true;
            }
        }

        void StartEvent()
        {
            if (_bSumm)
                return;

            if (!_bSumm1)
            {
                if (Creature* shadowy = me->SummonCreature(Creatures::MortwakeShadowyFigure, -11138.659f, 545.20f, 70.30f, 0.19f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                {
                    _shadowyGUID = shadowy->GetGUID();
                    shadowy->SetUnitFlag(UNIT_FLAG_UNINTERACTIBLE);
                    shadowy->SetWalk(true);
                    shadowy->GetMotionMaster()->MovePoint(0, -11131.710f, 546.810f, 70.380f);
                    _bSumm1 = true;
                }
            }

            if (!_bSumm2)
            {
                if (Creature* oaf = me->SummonCreature(Creatures::MortwakeOaf, -11128.11f, 547.52f, 70.41f, 3.32f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                {
                    _oafGUID = oaf->GetGUID();
                    oaf->SetUnitFlag(UNIT_FLAG_UNINTERACTIBLE);

                    if (Creature* helix = me->SummonCreature(Creatures::MortwakeHelix, -11128.11f, 547.52f, 70.41f, 3.32f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                    {
                        _helixGUID = helix->GetGUID();
                        helix->SetUnitFlag(UNIT_FLAG_UNINTERACTIBLE);
                        helix->EnterVehicle(oaf, 0);
                        _bSumm2 = true;
                    }
                }
            }

            if (_bSumm1 && _bSumm2)
            {
                _bSumm = true;
                _summonTimer = 2000;
                _stepIndex = 0;
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!_eventRunning || !_bSumm)
                return;

            if (_summonTimer > diff)
            {
                _summonTimer -= diff;
                return;
            }

            Player* player = ObjectAccessor::GetPlayer(*me, _playerGUID);
            Creature* oaf = ObjectAccessor::GetCreature(*me, _oafGUID);
            Creature* helix = ObjectAccessor::GetCreature(*me, _helixGUID);
            Creature* shadowy = ObjectAccessor::GetCreature(*me, _shadowyGUID);

            if (!player || !oaf || !helix || !shadowy)
                return;

            // Initial facing once, before dialogue
            if (_stepIndex == 0)
                oaf->SetInFront(me);

            if (_stepIndex >= std::size(Dialogue::MortwakeSequence))
                return;

            Dialogue::MortwakeStep const& step = Dialogue::MortwakeSequence[_stepIndex];

            switch (step.actor)
            {
            case Dialogue::MortwakeActor::Helix:
                if (step.textId != -1)
                    helix->AI()->Talk(step.textId);
                break;
            case Dialogue::MortwakeActor::Shadowy:
                if (step.textId != -1)
                    shadowy->AI()->Talk(step.textId);
                break;
            case Dialogue::MortwakeActor::PlayerCredit:
                player->CastSpell(player, Spells::QuestCredit26290, true);
                break;
            case Dialogue::MortwakeActor::Exit:
            {
                shadowy->CastSpell(shadowy, Spells::TeleportVisual, true);
                shadowy->DespawnOrUnsummon(1s);
                oaf->CastSpell(shadowy, Spells::TeleportVisual, true);
                helix->DespawnOrUnsummon(1s);
                oaf->DespawnOrUnsummon(1s);
                Reset();
                return;
            }
            }

            _summonTimer = step.delayMs;
            ++_stepIndex;
        }

        private:
            uint32 _summonTimer = 0;
            uint8 _stepIndex = 0;
            ObjectGuid _playerGUID;
            ObjectGuid _oafGUID;
            ObjectGuid _helixGUID;
            ObjectGuid _shadowyGUID;

            bool _bSumm = false;
            bool _bSumm1 = false;
            bool _bSumm2 = false;
            bool _bExit = false;
            bool _eventRunning = false;
    };
}



void AddSC_custom_westfall_npcs()
{
    using namespace Scripts::EasternKingdoms::Westfall;

    RegisterCreatureAI(npc_custom_westfall_stew_42617);
    RegisterCreatureAI(npc_custom_westfall_guard_42407);
    RegisterCreatureAI(npc_custom_lous_parting_thoughts_trigger);
    RegisterCreatureAI(npc_custom_lous_parting_thoughts_thug);
    RegisterCreatureAI(npc_custom_salma_saldean_235);
    RegisterCreatureAI(npc_custom_gryan_stoutmantle_234);
    RegisterCreatureAI(npc_custom_agent_kearnen);
    RegisterCreatureAI(npc_custom_elite_mercenary);
    RegisterCreatureAI(npc_custom_trigger_mortwake_tower);
}
