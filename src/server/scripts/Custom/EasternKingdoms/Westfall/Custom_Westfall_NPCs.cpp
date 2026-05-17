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
                            hungryHobo->AI()->Talk(11);
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
}

 /*######
 ## npc_custom_lous_parting_thoughts_trigger
 ######*/

class npc_custom_lous_parting_thoughts_trigger : public CreatureScript
{
public:
    npc_custom_lous_parting_thoughts_trigger() : CreatureScript("npc_custom_lous_parting_thoughts_trigger") { }

    struct npc_lous_parting_thoughts_triggerAI : public ScriptedAI
    {
        npc_lous_parting_thoughts_triggerAI(Creature* creature) : ScriptedAI(creature) { }

        ObjectGuid PlayerGUID;
        std::array<ObjectGuid, 4> ThugGUIDs;

        uint32 PhaseTimer = 0;
        uint8  Phase = 0;
        uint8  DeadThugs = 0;
        bool   EventStarted = false;
        bool   EventLocked = false;

        void Reset() override
        {
            Phase = 0;
            DeadThugs = 0;
            EventStarted = false;
            EventLocked = false;
            PhaseTimer = 1000;

            for (auto& guid : ThugGUIDs)
                guid.Clear();
        }

        void MoveInLineOfSight(Unit* who) override
        {
            if (EventStarted)
                return;

            Player* player = who->ToPlayer();
            if (!player)
                return;

            if (player->GetQuestStatus(QUEST_LOUS_PARTING_THOUGHTS) != QUEST_STATUS_INCOMPLETE)
                return;

            if (!who->IsWithinDistInMap(me, 30.0f))
                return;

            PlayerGUID = player->GetGUID();
            StartEvent();
        }

        void StartEvent()
        {
            EventStarted = true;

            for (uint8 i = 0; i < 4; ++i)
            {
                if (Creature* thug = me->SummonCreature(
                    NPC_WESTFALL_THUG,
                    Q26232ThugPositions[i],
                    TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT,
                    90s))
                {
                    thug->SetReactState(REACT_PASSIVE);
                    thug->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC);

                    ThugGUIDs[i] = thug->GetGUID();
                }

            }

            PhaseTimer = 2000;
        }

        Creature* GetThug(uint8 index)
        {
            return ObjectAccessor::GetCreature(*me, ThugGUIDs[index]);
        }

        void SetData(uint32 id, uint32 /*value*/) override
        {
            if (id != 1)
                return;

            ++DeadThugs;

            if (DeadThugs >= 4)
            {
                if (Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID))
                {
                    me->TextEmote(SAY_FURLBROW_RETURN);
                    player->KilledMonsterCredit(NPC_WESTFALL_Q26232_CREDIT, PlayerGUID);
                    player->RemoveAurasDueToSpell(SPELL_WESTFALL_DETECT_QUEST_INVIS_1);
                    player->CastSpell(player, SPELL_WESTFALL_DETECT_QUEST_INVIS_2);
                }


                Reset();
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!EventStarted || EventLocked)
                return;

            if (PhaseTimer <= diff)
            {
                switch (Phase)
                {
                case 0:
                    if (Creature* thug = GetThug(1))
                        thug->AI()->Talk(Q26232_SAY_ASK_MEET);
                    PhaseTimer = 3500;
                    break;

                case 1:
                    if (Creature* thug = GetThug(0))
                        thug->AI()->Talk(Q26232_SAY_CONFIRM_MEET);
                    PhaseTimer = 4000;
                    break;

                case 2:
                    if (Creature* thug = GetThug(0))
                        thug->AI()->Talk(Q26232_SAY_CONGRATULATE);
                    PhaseTimer = 7000;
                    break;

                case 3:
                    if (Creature* thug = GetThug(3))
                        thug->AI()->Talk(Q26232_SAY_ASK_IDENTITY);
                    PhaseTimer = 4000;
                    break;

                case 4:
                    if (Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID))
                        for (auto guid : ThugGUIDs)
                            if (Creature* thug = ObjectAccessor::GetCreature(*me, guid))
                                thug->SetFacingToObject(player);
                    PhaseTimer = 1000;
                    break;

                case 5:
                    if (Creature* thug = GetThug(2))
                        thug->AI()->Talk(Q26232_SAY_NOTICE_PLAYER);
                    PhaseTimer = 4500;
                    break;

                case 6:
                    if (Creature* thug = GetThug(1))
                        thug->AI()->Talk(Q26232_SAY_THREATEN_PLAYER);
                    PhaseTimer = 4500;
                    break;

                case 7:
                    if (Creature* thug = GetThug(0))
                        thug->AI()->Talk(Q26232_SAY_DIE);
                    PhaseTimer = 2000;
                    break;

                case 8:
                    for (auto guid : ThugGUIDs)
                    {
                        if (Creature* thug = ObjectAccessor::GetCreature(*me, guid))
                        {
                            thug->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC);
                            thug->SetFaction(14);
                            thug->SetReactState(REACT_AGGRESSIVE);
                        }
                    }
                    PhaseTimer = 1000;
                    break;

                case 9:
                    if (Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID))
                        for (auto guid : ThugGUIDs)
                            if (Creature* thug = ObjectAccessor::GetCreature(*me, guid))
                                thug->EngageWithTarget(player);
                    EventLocked = true;
                    PhaseTimer = 0;
                    break;
                }

                ++Phase;
            }
            else
                PhaseTimer -= diff;
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_lous_parting_thoughts_triggerAI(creature);
    }
};

/*######
## npc_custom_lous_parting_thoughts_thug
######*/

class npc_custom_lous_parting_thoughts_thug : public CreatureScript
{
public:
    npc_custom_lous_parting_thoughts_thug() : CreatureScript("npc_custom_lous_parting_thoughts_thug") { }

    struct npc_lous_parting_thoughts_thugAI : public ScriptedAI
    {
        npc_lous_parting_thoughts_thugAI(Creature* creature) : ScriptedAI(creature) { }

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

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_lous_parting_thoughts_thugAI(creature);
    }
};

class npc_custom_agent_kearnen : public CreatureScript
{
public:
    npc_custom_agent_kearnen() : CreatureScript("npc_custom_agent_kearnen") { }

    struct npc_agent_kearnenAI : public ScriptedAI
    {
        npc_agent_kearnenAI(Creature* creature) : ScriptedAI(creature) { }

        EventMap events;
        ObjectGuid PlayerGUID;
        std::queue<ObjectGuid> PendingTargets;

        void Reset() override
        {
            events.Reset();
            PlayerGUID.Clear();

            std::queue<ObjectGuid> empty;
            std::swap(PendingTargets, empty);
        }

        void Assist(Player* player, Creature* target)
        {
            if (!player || !target)
                return;

            PlayerGUID = player->GetGUID();
            PendingTargets.push(target->GetGUID());
            events.ScheduleEvent(EVENT_KEARNEN_KILL_SHOT, 5s, 10s);
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_KEARNEN_KILL_SHOT:
                {
                    if (PendingTargets.empty())
                        break;

                    ObjectGuid TargetGUID = PendingTargets.front();
                    PendingTargets.pop();

                    Creature* target = ObjectAccessor::GetCreature(*me, TargetGUID);
                    Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID);

                    if (target && target->IsAlive() && player)
                    {
                        me->SetFacingToObject(target);
                        me->CastSpell(target, SPELL_WESTFALL_KILL_SHOT, true);
                        Talk(0, player);
                    }

                    // If more targets remain, schedule again
                    if (!PendingTargets.empty())
                        events.ScheduleEvent(EVENT_KEARNEN_KILL_SHOT, 5s, 10s);

                    break;
                }
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_agent_kearnenAI(creature);
    }
};

class npc_custom_elite_mercenary : public CreatureScript
{
public:
    npc_custom_elite_mercenary() : CreatureScript("npc_custom_elite_mercenary") { }

    struct npc_elite_mercenaryAI : public ScriptedAI
    {
        npc_elite_mercenaryAI(Creature* creature) : ScriptedAI(creature) { }

        bool AssistTriggered = false;

        void Reset() override
        {
            AssistTriggered = false;
        }

        void JustEngagedWith(Unit* who) override
        {
            if (!who)
                return;

            Player* player = who->ToPlayer();
            if (!player)
                return;

            if (AssistTriggered)
                return;

            if (player->GetQuestStatus(QUEST_SECRETS_OF_THE_TOWER) != QUEST_STATUS_INCOMPLETE)
                return;

            Creature* agent = me->FindNearestCreature(NPC_WESTFALL_KEARNEN, 150.0f, true);
            if (!agent)
                return;

            if (auto* ai = CAST_AI(npc_custom_agent_kearnen::npc_agent_kearnenAI, agent->AI()))
                ai->Assist(player, me);

            AssistTriggered = true;
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_elite_mercenaryAI(creature);
    }
};


class npc_custom_trigger_mortwake_tower : public CreatureScript
{
public:
    npc_custom_trigger_mortwake_tower() : CreatureScript("npc_custom_trigger_mortwake_tower") {}

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_custom_trigger_mortwake_towerAI(creature);
    }

    struct npc_custom_trigger_mortwake_towerAI : public ScriptedAI
    {
        npc_custom_trigger_mortwake_towerAI(Creature* creature) : ScriptedAI(creature) {}

        uint8 Phase = 0;
        uint32 SummonTimer = 0;
        ObjectGuid PlayerGUID;
        ObjectGuid OafGUID;
        ObjectGuid Helix3GUID;
        ObjectGuid Shadowy3GUID;

        bool bSumm = false;
        bool bSumm1 = false;
        bool bSumm2 = false;
        bool bExit = false;
        bool EventTriggered = false;   // prevents retriggering
        bool EventRunning = false;     // allows UpdateAI to run the sequence

        void Reset() override
        {
            Phase = 0;
            SummonTimer = 2000;
            bSumm = false;
            bSumm1 = false;
            bSumm2 = false;
            bExit = false;

            EventTriggered = false;   // prevents retriggering
            EventRunning = false;     // allows UpdateAI to run the sequence
        }

        void StartEvent()
        {
            if (!bSumm)
            {
                if (!bSumm1)
                {
                    if (Creature* Shadowy3 = me->SummonCreature(NPC_WESTFALL_SHADOWY_FIGURE, -11138.659f, 545.20f, 70.30f, 0.19f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                    {
                        Shadowy3GUID = Shadowy3->GetGUID();
                        Shadowy3->SetUnitFlag(UNIT_FLAG_UNINTERACTIBLE);
                        Shadowy3->SetWalk(true);
                        Shadowy3->GetMotionMaster()->MovePoint(0, -11131.710f, 546.810f, 70.380f);
                        bSumm1 = true;
                    }
                }

                if (!bSumm2)
                {
                    if (Creature* oafVehicle = me->SummonCreature(NPC_WESTFALL_OAF_MORTWAKE, -11128.11f, 547.52f, 70.41f, 3.32f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                    {
                        OafGUID = oafVehicle->GetGUID();
                        oafVehicle->SetUnitFlag(UNIT_FLAG_UNINTERACTIBLE);

                        if (Creature* helix = me->SummonCreature(NPC_WESTFALL_HELIX_MORTWAKE, -11128.11f, 547.52f, 70.41f, 3.32f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                        {
                            Helix3GUID = helix->GetGUID();
                            helix->SetUnitFlag(UNIT_FLAG_UNINTERACTIBLE);

                            // Attach Helix to the Oaf
                            helix->EnterVehicle(oafVehicle, 0);

                            bSumm2 = true;
                        }
                    }
                }

                if (bSumm1 && bSumm2)
                {
                    bSumm = true;
                    SummonTimer = 2000;
                }
            }
        }

        void UpdateAI(uint32 diff) override
        {
            // 1. Detect player (only if event not triggered yet)
            if (!EventTriggered)
            {
                if (Player* player = me->SelectNearestPlayer(30.0f))
                {
                    if (player->HasAura(SPELL_WESTFALL_POTION_SHROUDING) &&
                        player->GetQuestStatus(QUEST_SECRETS_OF_THE_TOWER) == QUEST_STATUS_INCOMPLETE &&
                        fabs(player->GetPositionZ() - me->GetPositionZ()) < 2.0f &&
                        player->IsWithinDistInMap(me, 20.0f))
                    {
                        PlayerGUID = player->GetGUID();
                        StartEvent();
                        EventTriggered = true;
                        EventRunning = true;
                    }
                }
            }

            if (EventRunning)
            {
                if (SummonTimer < diff)
                {
                    if (bSumm)
                    {
                        if (Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID))
                            if (Creature* oaf = ObjectAccessor::GetCreature(*me, OafGUID))
                                if (Creature* Shadowy3 = ObjectAccessor::GetCreature(*me, Shadowy3GUID))
                                {
                                    Creature* Helix = ObjectAccessor::GetCreature(*me, Helix3GUID);
                                    if (!Helix)
                                        return;

                                    switch (Phase)
                                    {
                                    case 0:
                                    {
                                        oaf->SetInFront(me);
                                        SummonTimer = 1000;
                                        Phase++;
                                        break;
                                    }
                                    case 1:
                                    {
                                        Helix->AI()->Talk(Q26232_SAY_HELIX_0);
                                        SummonTimer = 6000;
                                        Phase++;
                                        break;
                                    }
                                    case 2:
                                    {
                                        Shadowy3->AI()->Talk(Q26232_SAY_SHADOWY_0);
                                        SummonTimer = 6000;
                                        Phase++;
                                        break;
                                    }
                                    case 3:
                                    {
                                        Helix->AI()->Talk(Q26232_SAY_HELIX_1);
                                        SummonTimer = 6000;
                                        Phase++;
                                        break;
                                    }
                                    case 4:
                                    {
                                        Shadowy3->AI()->Talk(Q26232_SAY_SHADOWY_1);
                                        SummonTimer = 6000;
                                        Phase++;
                                        break;
                                    }
                                    case 5:
                                    {
                                        Helix->AI()->Talk(Q26232_SAY_HELIX_2);
                                        SummonTimer = 6000;
                                        Phase++;
                                        break;
                                    }
                                    case 6:
                                    {
                                        Shadowy3->AI()->Talk(Q26232_SAY_SHADOWY_2);
                                        SummonTimer = 6000;
                                        Phase++;
                                        break;
                                    }
                                    case 7:
                                    {
                                        Shadowy3->AI()->Talk(Q26232_SAY_SHADOWY_3);
                                        SummonTimer = 6000;
                                        Phase++;
                                        break;
                                    }
                                    case 8:
                                    {
                                        Helix->AI()->Talk(Q26232_SAY_HELIX_3);
                                        SummonTimer = 6000;
                                        Phase++;
                                        break;
                                    }
                                    case 9:
                                    {
                                        Shadowy3->AI()->Talk(Q26232_SAY_SHADOWY_4);
                                        SummonTimer = 2000;
                                        Phase++;
                                        break;
                                    }
                                    case 10:
                                    {
                                        player->CastSpell(player, SPELL_WESTFALL_Q26290_CREDIT, true);
                                        SummonTimer = 1000;
                                        Phase++;
                                        break;
                                    }
                                    case 11:
                                    {
                                        if (!bExit)
                                        {
                                            Shadowy3->CastSpell(Shadowy3, SPELL_WESTFALL_TELEPORT_VISUAL, true);
                                            Shadowy3->DespawnOrUnsummon(1s);
                                            oaf->CastSpell(Shadowy3, SPELL_WESTFALL_TELEPORT_VISUAL, true);
                                            Helix->DespawnOrUnsummon(1s);
                                            oaf->DespawnOrUnsummon(1s);
                                            bExit = true;
                                            Reset();
                                        }
                                    }
                                    break;
                                    default:
                                        break;
                                    }
                                }
                    }
                }
                else SummonTimer -= diff;
            }
        }
    };
};

void AddSC_custom_westfall_npcs()
{
    using namespace Scripts::EasternKingdoms::Westfall;

    RegisterCreatureAI(npc_custom_westfall_stew_42617);
    RegisterCreatureAI(npc_custom_westfall_guard_42407);

    new npc_custom_lous_parting_thoughts_trigger();
    new npc_custom_lous_parting_thoughts_thug();
    new npc_custom_agent_kearnen();
    new npc_custom_elite_mercenary();
    new npc_custom_trigger_mortwake_tower();
}
