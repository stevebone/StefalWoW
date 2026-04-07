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
#include "Player.h"
#include "TaskScheduler.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "TemporarySummon.h"
#include "Vehicle.h"

#include "Custom_TirisfalGlades_Defines.h"

namespace Scripts::Custom::TirisfalGlades
{
    // 1740
    class npc_custom_deathguard_saltain : public CreatureScript
    {
    public:
        npc_custom_deathguard_saltain() : CreatureScript("npc_custom_deathguard_saltain") {}

        struct npc_deathguard_saltainAI : public ScriptedAI
        {
            npc_deathguard_saltainAI(Creature* creature) : ScriptedAI(creature) {}

            void OnQuestAccept(Player* player, Quest const* quest) override
            {
                if (quest->GetQuestId() == Quests::quest_recruitment)
                {
                    player->CastSpell(player, SpellsMisc::spell_summon_darnell_1, true);
                }
            }

            void OnQuestReward(Player* player, Quest const* quest, LootItemType /*type*/, uint32 /*opt*/) override
            {
                if (quest->GetQuestId() == 26800)
                {
                    if (Creature* darnell = player->FindNearestCreature(Npcs::npc_darnell_1, 60.0f, true))
                        darnell->GetMotionMaster()->MovePoint(2, PositionsQ26800::pos_darnell_drop_corpses);
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_deathguard_saltainAI(creature);
        }
    };

    // Darnell 49141
    class npc_custom_darnell_0 : public CreatureScript
    {
    public:
        npc_custom_darnell_0() : CreatureScript("npc_custom_darnell_0") { }

        struct npc_custom_darnellAI : public ScriptedAI
        {
            npc_custom_darnellAI(Creature* creature) : ScriptedAI(creature)
            {
            }

            void Reset() override
            {
                Player* player = nullptr;

                // First try: owner
                if (Unit* owner = me->GetOwner())
                {
                    player = owner->ToPlayer();
                    if (player)
                        PlayerGUID = player->GetGUID();
                }

                Talk(TalksDarnell0::talk_darnell_hello, player);

                if (player && player->IsActiveQuest(Quests::quest_the_shadow_grave))
                    events.ScheduleEvent(EventsDarnell0::event_darnell_way_to_grave, 3s, 5s);
            }

            // -------------------------
            // MOVEMENT INFORM
            // -------------------------
            void MovementInform(uint32 type, uint32 id) override
            {
                if (type != POINT_MOTION_TYPE)
                    return;

                switch (id)
                {
                case Misc::point_darnell_grave_entrance: // reached entrance
                {
                    Talk(TalksDarnell0::talk_darnell_this_way);
                    events.ScheduleEvent(EventsDarnell0::event_darnell_downstairs_1, 2s, 3s);
                    break;
                }

                case Misc::point_darnell_grave_stairs_1:
                {
                    me->GetMotionMaster()->MovePoint(
                        Misc::point_darnell_grave_stairs_2,
                        PositionsQ28608::pos_darnell_grave_stairs_2
                    );
                    break;
                }

                case Misc::point_darnell_grave_stairs_2:
                {
                    me->GetMotionMaster()->MovePoint(
                        Misc::point_darnell_grave_ground,
                        PositionsQ28608::pos_darnell_grave_ground
                    );
                    break;
                }

                case Misc::point_darnell_grave_ground:
                {
                    Talk(TalksDarnell0::talk_darnell_ask_about_supplies);
                    me->GetMotionMaster()->MovePoint(
                        Misc::point_darnell_grave_center,
                        PositionsQ28608::pos_darnell_grave_center
                    );
                    break;
                }

                case Misc::point_darnell_grave_center:
                {
                    Talk(TalksDarnell0::talk_darnell_supplies_loc_1);
                    me->GetMotionMaster()->MovePoint(
                        Misc::point_darnell_grave_supplies_1,
                        PositionsQ28608::pos_darnell_supplies[urand(0, 3)]
                    );
                    break;
                }

                case Misc::point_darnell_grave_supplies_1:
                {
                    Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID);
                    if (!player)
                        return;

                    Talk(TalksDarnell0::talk_darnell_hmm);

                    scheduler.Schedule(3s, [this](TaskContext /*task*/)
                        {
                            Talk(TalksDarnell0::talk_darnell_nope);
                        });

                    scheduler.Schedule(6s, [this, player](TaskContext /*task*/)
                        {
                            Talk(TalksDarnell0::talk_darnell_ask_player, player);
                        });

                    scheduler.Schedule(9s, [this](TaskContext /*task*/)
                        {
                            Talk(TalksDarnell0::talk_darnell_supplies_loc_2);
                        });

                    break;
                }
                }
            }

            void UpdateAI(uint32 diff) override
            {
                if (!CheckPlayerValid())
                    return;

                Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID);

                scheduler.Update(diff);
                events.Update(diff);

                if (player->GetQuestStatus(Quests::quest_the_shadow_grave) == QUEST_STATUS_COMPLETE && !suppliesFound)
                {
                    suppliesFound = true;
                    Talk(TalksDarnell0::talk_darnell_supplies_found);
                    events.ScheduleEvent(EventsDarnell0::event_darnell_despawn, 3s);
                }

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EventsDarnell0::event_darnell_way_to_grave:
                    {
                        Talk(TalksDarnell0::talk_darnell_to_grave, player);
                        me->GetMotionMaster()->MovePoint(Misc::point_darnell_grave_entrance, PositionsQ28608::pos_darnell_grave_entrance);
                        break;
                    }
                    case EventsDarnell0::event_darnell_downstairs_1:
                    {
                        me->GetMotionMaster()->MovePoint(Misc::point_darnell_grave_stairs_1, PositionsQ28608::pos_darnell_grave_stairs_1);
                        break;
                    }
                    case EventsDarnell0::event_darnell_despawn:
                    {
                        Talk(TalksDarnell0::talk_darnell_joke);
                        me->GetMotionMaster()->MoveFollow(player, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
                        me->DespawnOrUnsummon(20s);
                        break;
                    }
                    }
                }
            }

            bool CheckPlayerValid()
            {
                Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID);
                if (!player) return false;

                if (!player->IsInWorld() ||
                    player->isDead() ||
                    player->GetQuestStatus(Quests::quest_the_shadow_grave) == QUEST_STATUS_NONE ||
                    player->GetQuestStatus(Quests::quest_the_shadow_grave) == QUEST_STATUS_FAILED)
                {
                    me->DespawnOrUnsummon();
                    return false;
                }
                return true;
            }

        private:
            EventMap events;
            ObjectGuid PlayerGUID;
            TaskScheduler scheduler;
            bool suppliesFound = false;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_custom_darnellAI(creature);
        }
    };

    // Darnell 49337
    // For Recruitment quest we need to spawn a normal version of Darnell
    // Otherwise the Ride vehicle spell DOES NOT WORK.
    class npc_custom_darnell : public CreatureScript
    {
    public:
        npc_custom_darnell() : CreatureScript("npc_custom_darnell") { }

        struct npc_custom_darnellAI : public ScriptedAI
        {
            npc_custom_darnellAI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetReactState(REACT_PASSIVE);
                if (!me->HasUnitFlag(UNIT_FLAG_PLAYER_CONTROLLED))
                    isClone = true;
            }

            void Reset() override
            {
                // --- Resolve player safely ---
                Player* player = nullptr;

                if (!isClone)
                {
                    // First try: owner
                    if (Unit* owner = me->GetOwner())
                    {
                        player = owner->ToPlayer();
                        if (player)
                            PlayerGUID = player->GetGUID();
                    }
                }

                if (isClone)
                {
                    // Try to resolve player from stored GUID first
                    if (!PlayerGUID.IsEmpty())
                        player = ObjectAccessor::GetPlayer(*me, PlayerGUID);

                    // Fallback: nearest player
                    if (!player)
                        player = me->SelectNearestPlayer(10.f);

                    if (player)
                        PlayerGUID = player->GetGUID();
                }

                if (!player)
                    return; // Still no player? Abort safely.

                // --- Quest: The Wakening ---
                if (player->GetQuestStatus(Quests::quest_the_wakening) == QUEST_STATUS_COMPLETE)
                    events.ScheduleEvent(EventsDarnell::event_darnell_way_to_deathknell, 5s);

                // --- Quest: Recruitment ---
                if (player->IsActiveQuest(Quests::quest_recruitment))
                {
                    // If this is the original summoned Darnell ? replace with clone
                    if (!isClone)
                    {
                        Creature* clone = me->SummonCreature(
                            Npcs::npc_darnell_1,
                            player->GetRandomNearPosition(3.f)
                        );

                        if (clone)
                        {
                            // Pass player GUID to clone AI
                            clone->AI()->SetData(0, player->GetGUID().GetCounter());

                            me->DespawnOrUnsummon(100ms);
                            return;
                        }
                    }

                    // If clone ? follow player
                    events.ScheduleEvent(EventsDarnell::event_darnell_return_to_player, 1s);
                }

                if (player->IsActiveQuest(Quests::quest_beyond_the_graves))
                {
                    // If Darnell is already in Deathknell skip walk
                    if (me->GetAreaId() == Misc::area_deathknell)
                    {
                        arrivedDeathknellGate = true;
                        arrivedDeathknell = true;
                        wayToDeathknell = true;
                        me->GetMotionMaster()->Clear();
                        events.ScheduleEvent(EventsDarnell::event_darnell_return_to_player, 1s);
                    }

                     // If player is still around the graves get Darnell directly to gate
                    else if (me->GetAreaId() == Misc::area_deathknell_graves)
                    {
                        wayToDeathknell = true;
                        me->GetMotionMaster()->MovePoint(5, PositionsQ25089::pos_darnell_at_gate);
                    }

                }

                // --- Greeting ---
                Talk(TalksDarnell::talk_darnell_hello, player);
            }

            void SetData(uint32 id, uint32 value) override
            {
                if (id == 0)
                    PlayerGUID = ObjectGuid::Create<HighGuid::Player>(value);
            }

            void PassengerBoarded(Unit* passenger, int8 /*seatId*/, bool apply) override
            {
                if (!apply) // passenger removed
                {
                    if (Creature* corpse = passenger->ToCreature())
                    {
                        // Optional: put corpse back into dead state
                        corpse->CastSpell(corpse, SpellsQ26800::spell_feign_death);

                        // Despawn after a few seconds
                        corpse->DespawnOrUnsummon(30s);
                    }
                }
            }

            void UpdateAI(uint32 diff) override
            {
                Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID);
                if (!player || !player->IsInWorld())
                {
                    me->DespawnOrUnsummon();
                    return;
                }

                events.Update(diff);

                if (Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID))
                {
                    if (player->IsActiveQuest(Quests::quest_beyond_the_graves) && !wayToDeathknell)
                    {
                        wayToDeathknell = true;
                        Talk(TalksDarnell::talk_darnell_to_deathknell);
                        me->GetMotionMaster()->Clear();
                        me->GetMotionMaster()->MovePoint(3, PositionsQ25089::pos_darnell_to_deathknell_1);
                    }

                    // Check Deathknell
                    if (!arrivedDeathknell && arrivedDeathknellGate)
                    {
                        Creature* bunny = me->FindNearestCreature(Npcs::npc_gate_bunny, 10.f);
                        if (bunny)
                        {
                            if (player->IsWithinDist(me, 5.f))
                            {
                                arrivedDeathknell = true;
                                Talk(TalksDarnell::talk_darnell_arrived_deathknell);
                                events.ScheduleEvent(EventsDarnell::event_darnell_return_to_player, 2s, 3s);
                            }
                        }
                    }

                    // Check Saltain
                    if (!arrivedSaltain)
                    {
                        Creature* saltain = me->FindNearestCreature(Npcs::npc_saltain, 10.f);
                        if (saltain)
                        {
                            arrivedSaltain = true;
                            Talk(TalksDarnell::talk_darnell_arrived_saltain);
                        }
                    }

                    if (player->IsActiveQuest(Quests::quest_recruitment))
                    {
                        if (!corpseQuest)
                        {
                            Creature* saltain = me->FindNearestCreature(Npcs::npc_saltain, 10.f);
                            if (saltain)
                            {
                                corpseQuest = true;
                                events.ScheduleEvent(EventsDarnell::event_darnell_start_corpses_quest, 5s);
                            }
                        }

                        // Check corpses
                        if (!sawCorpse)
                        {
                            Creature* corpse = me->FindNearestCreature(Npcs::npc_scarlet_corpse, 10.f);
                            if (corpse)
                            {
                                sawCorpse = true;
                                Talk(TalksDarnell::talk_darnell_saw_corpses, player);
                            }
                        }
                    }
                }

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EventsDarnell::event_darnell_way_to_deathknell:
                    {
                        Talk(TalksDarnell::talk_darnell_to_deathknell); // I know the way to Deathknell. Come with me!
                        break;
                    }

                    // -------------------------
                    // CORPSE PICKUP SEQUENCE
                    // -------------------------

                    case EventsDarnell::event_darnell_corpse_pickup:
                    {
                        if (Creature* corpse = ObjectAccessor::GetCreature(*me, CorpseGUID))
                        {
                            // Pickup visuals
                            me->CastSpell(corpse, SpellsQ26800::spell_visual_pick_corpse, true);
                            if(!me->HasAura(SpellsQ26800::spell_darnell_slumped_over))
                                me->CastSpell(me, SpellsQ26800::spell_darnell_slumped_over, true);
                            corpse->RemoveAllAuras();
                            corpse->CastSpell(me, SpellsMisc::spell_ride_vehicle, true);

                            Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID);
                            if (player)
                                player->KilledMonsterCredit(Npcs::npc_scarlet_corpse);
                        }

                        events.ScheduleEvent(EventsDarnell::event_darnell_return_to_player, 1s);
                        break;
                    }

                    case EventsDarnell::event_darnell_return_to_player:
                    {
                        if (Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID))
                        {
                            me->GetMotionMaster()->Clear();
                            me->GetMotionMaster()->MoveFollow(player, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
                        }
                        break;
                    }
                    case EventsDarnell::event_darnell_start_corpses_quest:
                    {
                        Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID);
                        // Let's get moving, $n. Saltain said that we'd find some corpses up here.
                        if(player)
                            Talk(TalksDarnell::talk_darnell_start_corpses_quest, player);
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
                    me->GetMotionMaster()->MovePoint(1, corpse->GetRandomNearPosition(1.f));
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
                    events.ScheduleEvent(EventsDarnell::event_darnell_corpse_pickup, 500ms);
                }

                if (id == 2)
                {
                    me->GetVehicleKit()->RemoveAllPassengers();
                    me->DespawnOrUnsummon(5s);
                }

                if (id == 3)
                    me->GetMotionMaster()->MovePoint(4, PositionsQ25089::pos_darnell_to_deathknell_2);

                if (id == 4)
                    me->GetMotionMaster()->MovePoint(5, PositionsQ25089::pos_darnell_at_gate);

                if (id == 5)
                    arrivedDeathknellGate = true;
            }

        private:
            EventMap events;
            ObjectGuid CorpseGUID;
            ObjectGuid PlayerGUID;
            bool isClone = false;
            bool arrivedDeathknell = false;
            bool arrivedSaltain = false;
            bool corpseQuest = false;
            bool sawCorpse = false;
            bool wayToDeathknell = false;
            bool arrivedDeathknellGate = false;

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
                me->ReplaceAllNpcFlags(UNIT_NPC_FLAG_SPELLCLICK);
            }

            void OnSpellClick(Unit* clicker, bool /*spellClickHandled*/) override
            {
                if (Player* player = clicker->ToPlayer())
                {
                    if (player->GetQuestStatus(Quests::quest_recruitment) != QUEST_STATUS_COMPLETE)
                    {
                        player->CastSpell(me, SpellsQ26800::spell_select_scarlet_corpse);

                        if (Creature* darnell = player->FindNearestCreature(Npcs::npc_darnell_1, 20.0f, true))
                        {
                            if (auto* ai = CAST_AI(npc_custom_darnell::npc_custom_darnellAI, darnell->AI()))
                                ai->StartPickup(me->GetGUID(), player->GetGUID());

                            me->RemoveNpcFlag(UNIT_NPC_FLAG_SPELLCLICK);
                            me->SetReactState(REACT_PASSIVE);
                        }
                    }
                }
            }

            void ReceiveEmote(Player* player, uint32 emoteId) override
            {
                if (emoteId != TEXT_EMOTE_POINT)
                    return;

                if (!player->IsActiveQuest(Quests::quest_recruitment))
                    return;

                // Find Darnell
                if (Creature* darnell = player->FindNearestCreature(Npcs::npc_darnell_1, 20.f))
                    if (auto* ai = CAST_AI(npc_custom_darnell::npc_custom_darnellAI, darnell->AI()))
                        ai->StartPickup(me->GetGUID(), player->GetGUID());
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_custom_scarlet_corpseAI(creature);
        }
    };
}

void AddSC_custom_tirisfal_glades_npcs()
{
    using namespace Scripts::Custom::TirisfalGlades;

    new npc_custom_deathguard_saltain();
    new npc_custom_darnell_0();
    new npc_custom_darnell();
    new npc_custom_scarlet_corpse();
}
