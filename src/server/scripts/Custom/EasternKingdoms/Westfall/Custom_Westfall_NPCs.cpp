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

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Creature.h"
#include "TemporarySummon.h"
#include "Unit.h"
#include "ObjectGuid.h"
#include "MotionMaster.h"

#include <queue>

#include "Custom_Westfall_Defines.h"

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

class npc_custom_hungry_hobo : public CreatureScript
{
public:
    npc_custom_hungry_hobo() : CreatureScript("npc_custom_hungry_hobo") {}

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_hungry_hoboAI(creature);
    }

    struct npc_hungry_hoboAI : public ScriptedAI
    {
        npc_hungry_hoboAI(Creature* creature) : ScriptedAI(creature) {}

        uint8 count = 0;
        uint32 Miam = 0;

        void Reset() override
        {
            count = 0;
            Miam = 2000;
        }

        void Eat()
        {
            me->CastSpell(me, SPELL_WESTFALL_FULL_BELLY, true);
            me->SetStandState(UNIT_STAND_STATE_SIT);
        }

        void UpdateAI(uint32 diff) override
        {
            if (Miam < diff)
            {
                if (Creature* stew = me->FindNearestCreature(NPC_WESTFALL_STEW, 10.0f, true))
                {
                    if (me->HasAura(SPELL_WESTFALL_FULL_BELLY) && count == 0)
                        return;

                    switch (count)
                    {
                    case 0:
                    {
                        static constexpr uint32 SPELL_SLEEP = 78677; // replace with real spell ID

                        if (me->HasAura(SPELL_SLEEP))
                        {
                            // Creature is sleeping so we remove the aura
                            me->RemoveAurasDueToSpell(SPELL_SLEEP);
                        }
                        me->SetStandState(UNIT_STAND_STATE_STAND);
                        Miam = 1000;
                        count++;
                        break;
                    }
                    case 1:
                    {
                        Eat();
                        Miam = 2000;
                        me->SetStandState(UNIT_STAND_STATE_SIT);
                        count++;
                        break;
                    }
                    case 2:
                    {
                        if (stew->ToTempSummon())
                            if (WorldObject* player = stew->ToTempSummon()->GetSummoner())
                                player->ToPlayer()->KilledMonsterCredit(42617);
                        Miam = 25000;
                        count++;
                        break;
                    }
                    default:
                        break;
                    }
                }
                else Miam = 3000;

                if (!me->HasAura(SPELL_WESTFALL_FULL_BELLY))
                    me->SetStandState(UNIT_STAND_STATE_STAND);

                if (count == 3)
                    Reset();
            }
            else Miam -= diff;
        }
    };
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
    new npc_custom_lous_parting_thoughts_trigger();
    new npc_custom_lous_parting_thoughts_thug();
    new npc_custom_hungry_hobo();
    new npc_custom_agent_kearnen();
    new npc_custom_elite_mercenary();
    new npc_custom_trigger_mortwake_tower();
}
