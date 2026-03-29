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
#include "EventMap.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ObjectGuid.h"
#include "ScriptedCreature.h"

#include "Custom_TirisfalGlades_Defines.h"

 // Darnell 49337
class npc_custom_darnell : public CreatureScript
{
public:
    npc_custom_darnell() : CreatureScript("npc_custom_darnell") { }

    struct npc_custom_darnellAI : public ScriptedAI
    {
        npc_custom_darnellAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetReactState(REACT_PASSIVE);
        }

        // -------------------------
        // VARIABLES FOR OLD SCRIPT
        // -------------------------
        bool talk2 = false;
        bool talk3 = false;
        bool talk5 = false;

        // -------------------------
        // NEW CORPSE PICKUP SYSTEM
        // -------------------------
        EventMap events;
        ObjectGuid CorpseGUID;
        ObjectGuid PlayerGUID;

        // Called when Darnell is summoned as a quest follower
        void JustAppeared() override
        {
            Player* player = me->GetOwner()->ToPlayer();
            if (player && player->IsActiveQuest(QUEST_RECRUITMENT))
                events.ScheduleEvent(EVENT_DARNELL_RETURN, 1s);

            Talk(DEATHKNELL_DARNELL_SAY_HELLO, player); // Hello again.

            // I know the way to Deathknell. Come with me!
            if(player->GetQuestStatus(QUEST_THE_WAKENING) == QUEST_STATUS_COMPLETE)
                events.ScheduleEvent(EVENT_DARNELL_SAY_WAY_TO_DEATHKNELL, 5s);
        }

        // Handle delayed talk from JustAppeared
        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_DARNELL_SAY_WAY_TO_DEATHKNELL:
                {
                    Player* player = me->GetOwner()->ToPlayer();
                    me->AI()->Talk(1); // I know the way to Deathknell. Come with me!
                    break;
                }

                // -------------------------
                // CORPSE PICKUP SEQUENCE
                // -------------------------

                case EVENT_DARNELL_PICKUP:
                {
                    if (Creature* corpse = ObjectAccessor::GetCreature(*me, CorpseGUID))
                    {
                        // Pickup visuals
                        me->CastSpell(corpse, SPELL_DEATHKNELL_REVERSE_CAST_RIDE_VEHICLE, true);
                        me->CastSpell(me, SPELL_DEATHKNELL_SLUMPED_OVER, true);

                        if (TempSummon* clone = me->SummonCreature(
                            corpse->GetEntry(),
                            corpse->GetPosition(),
                            TEMPSUMMON_MANUAL_DESPAWN))
                        {
                            // Make clone cosmetic-only
                            //clone->SetUnitFlag(UNIT_FLAG_UNINTERACTIBLE | UNIT_FLAG_NON_ATTACKABLE);
                            clone->SetReactState(REACT_PASSIVE);
                            me->CastSpell(clone, SPELL_DEATHKNELL_GRAB_PASSENGER, TRIGGERED_FULL_MASK);

                            // Find next free seat on Darnell
                            if (Vehicle* veh = me->GetVehicleKit())
                            {
                                int8 seatId = veh->GetNextEmptySeat(0, false)->first;
                                clone->EnterVehicle(me, seatId);
                            }
                        }

                        // Remove original corpse
                        corpse->DespawnOrUnsummon(500ms);
                    }

                    events.ScheduleEvent(EVENT_DARNELL_RETURN, 1s);
                    break;
                }

                case EVENT_DARNELL_RETURN:
                {
                    if (Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID))
                    {
                        me->GetMotionMaster()->MoveFollow(player, PET_FOLLOW_DIST, me->GetFollowAngle());
                    }
                    break;
                }
                }
            }
        }

        // -------------------------
        // CORPSE PICKUP ENTRY POINT
        // Called by corpse script
        // -------------------------
        void StartPickup(ObjectGuid corpseGuid, ObjectGuid playerGuid)
        {
            CorpseGUID = corpseGuid;
            PlayerGUID = playerGuid;

            if (Creature* corpse = ObjectAccessor::GetCreature(*me, corpseGuid))
            {
                // Move to corpse
                me->GetMotionMaster()->MovePoint(1, corpse->GetPosition());
            }
        }

        // -------------------------
        // MOVEMENT INFORM
        // -------------------------
        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id == 1) // reached corpse
            {
                // Schedule pickup animation
                events.ScheduleEvent(EVENT_DARNELL_PICKUP, 500ms);
            }
        }

        // -------------------------
        // ORIGINAL DIALOGUE TRIGGERS
        // -------------------------
        void MoveInLineOfSight(Unit* who) override
        {
            // entering deathknell
            if (who->IsInDist(me, 10.0f) && who->IsCreature() && who->ToCreature()->GetSpawnId() == 192714 && !talk2)
            {
                me->AI()->Talk(2); // Good, you're still here. Now, where's Deathguard Saltain?
                talk2 = true;
            }

            // reaching saltain
            if (who->IsInDist(me, 20.0f) && who->IsCreature() && who->ToCreature()->GetEntry() == NPC_DEATHKNELL_SALTAIN && !talk3)
            {
                talk3 = true;
                me->AI()->Talk(3); // Ah, here he is.

                Player* player = me->GetOwner()->ToPlayer();

                // Let's get moving, $n. Saltain said that we'd find some corpses up here.
                events.ScheduleEvent(200, 5s);
            }

            // reaching a corpse nearly
            if (who->IsInDist(me, 10.0f) && who->IsCreature() && who->ToCreature()->GetEntry() == NPC_DEATHKNELL_SCARLET_CORPSE && !talk5)
            {
                talk5 = true;

                Player* player = me->GetOwner()->ToPlayer();

                me->AI()->Talk(5, player); // I think I see some corpses up ahead...
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_custom_darnellAI(creature);
    }
};

// Scarlet Corpse 49340
class npc_custom_scarlet_corpse : public CreatureScript
{
public:
    npc_custom_scarlet_corpse() : CreatureScript("npc_custom_scarlet_corpse") { }

    struct npc_custom_scarlet_corpseAI : public ScriptedAI
    {
        npc_custom_scarlet_corpseAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetReactState(REACT_PASSIVE);
            me->SetNpcFlag(UNIT_NPC_FLAG_SPELLCLICK);
        }

        void OnSpellClick(Unit* clicker, bool /*spellClickHandled*/)
        {
            TC_LOG_DEBUG("scripts.ai.fsb", "OnSpellClick for scarlet corpse");
            if (Player* player = clicker->ToPlayer())
            {
                if (player->GetQuestStatus(QUEST_RECRUITMENT) != QUEST_STATUS_COMPLETE)
                {
                    player->CastSpell(me, SPELL_DEATHKNELL_SELECT_SCARLET_CORPSE);

                    if (Creature* darnell = player->FindNearestCreature(NPC_DEATHKNELL_DARNELL, 20.0f, true))
                    {
                        if (auto* ai = CAST_AI(npc_custom_darnell::npc_custom_darnellAI, darnell->AI()))
                            ai->StartPickup(me->GetGUID(), player->GetGUID());

                        me->RemoveNpcFlag(UNIT_NPC_FLAG_SPELLCLICK);
                        me->RemoveAura(SPELL_DEATHKNELL_COSMETIC_YELLOW_ARROW);
                        me->RemoveAura(SPELL_DEATHKNELL_FEIGN_DEATH);
                        me->SetReactState(REACT_PASSIVE);

                        me->DespawnOrUnsummon(120s);
                        me->SetRespawnDelay(120000);
                    }
                    else
                    {
                        player->KilledMonsterCredit(NPC_DEATHKNELL_SCARLET_CORPSE);
                    }
                }
            }
        }

        
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_custom_scarlet_corpseAI(creature);
    }
};

void AddSC_custom_tirisfal_glades_npcs()
{
    new npc_custom_darnell();
    new npc_custom_scarlet_corpse();
}
