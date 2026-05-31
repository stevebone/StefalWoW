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
#include "GameObject.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"

#include "Custom_Instance_Deadmines.h"
#include "Custom_DeadminesOLD_Defines.h"

namespace Scripts::EasternKingdoms::Deadmines
{
    struct boss_rhahkzor : public BossAI
    {
        boss_rhahkzor(Creature* creature) : BossAI(creature, DataTypesOLD::BOSS_RHAHKZOR) { }

        void JustEngagedWith(Unit* who) override
        {
            BossAI::JustEngagedWith(who);
            Talk(0);
            events.ScheduleEvent(EventsOLD::RhahkzorCastSlam, 8s, 12s);
        }

        void JustDied(Unit* killer) override
        {
            if (instance)
                instance->SetData(Misc::DeadminesVersion, Version::Classic);

            BossAI::JustDied(killer);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EventsOLD::RhahkzorCastSlam:
                        DoCastVictim(SpellsOLD::RhahkZorSlam);
                        events.Repeat(8s, 12s);
                        break;
                    default:
                        break;
                }
            }
        }
    };

    struct boss_sneed : public BossAI
    {
        boss_sneed(Creature* creature) : BossAI(creature, DataTypesOLD::BOSS_SNEED) { }

        void JustEngagedWith(Unit* who) override
        {
            BossAI::JustEngagedWith(who);
            events.ScheduleEvent(EventsOLD::SneedCastDisarm, 8s, 18s);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EventsOLD::SneedCastDisarm:
                        DoCastVictim(SpellsOLD::SneedDisarm);
                        events.Repeat(30s, 45s);
                        break;
                    default:
                        break;
                }
            }
        }
    };

    struct boss_gilnid : public BossAI
    {
        boss_gilnid(Creature* creature) : BossAI(creature, DataTypesOLD::BOSS_GILNID) { }

        void Reset() override
        {
            BossAI::Reset();
            events.ScheduleEvent(EventsOLD::GilnidTalk, 2min);
        }

        void JustEngagedWith(Unit* who) override
        {
            BossAI::JustEngagedWith(who);
            events.CancelEvent(EventsOLD::GilnidTalk);
            events.ScheduleEvent(EventsOLD::GilnidCastMoltenMetal, 2s, 5s);
        }

        void EnterEvadeMode(EvadeReason why) override
        {
            BossAI::EnterEvadeMode(why);
            events.ScheduleEvent(EventsOLD::GilnidTalk, 2min);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EventsOLD::GilnidTalk:
                        Talk(TextsOLD::GilnidOOC);
                        events.Repeat(2min);
                        break;
                    default:
                        break;
                    }
                }
                return;
            }

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EventsOLD::GilnidCastMoltenMetal:
                        DoCastVictim(SpellsOLD::GilnidMoltenMetal);
                        events.Repeat(23s, 45s);
                        break;
                    default:
                        break;
                }
            }
        }
    };

    struct boss_mr_smite : public BossAI
    {
        boss_mr_smite(Creature* creature) : BossAI(creature, DataTypesOLD::BOSS_MR_SMITE), _phase(0), _isMoving(false), _mrSmiteChestGUID() { }

        void Reset() override
        {
            BossAI::Reset();
            SetEquipmentSlots(false, EquipmentOLD::SmiteSword, EQUIP_UNEQUIP, EQUIP_NO_CHANGE);
            me->SetStandState(UNIT_STAND_STATE_STAND);
            me->SetReactState(REACT_AGGRESSIVE);
            me->SetNoCallAssistance(true);
            _phase = 0;
            _isMoving = false;
            _healthCheck = 0;
            

            if (instance)
            {
                if (GameObject* chest = ObjectAccessor::GetGameObject(*me, instance->GetGuidData(MiscOLD::DATA_SMITE_CHEST)))
                    _mrSmiteChestGUID = chest->GetGUID();
            }
        }

        void JustEngagedWith(Unit* who) override
        {
            BossAI::JustEngagedWith(who);
            events.ScheduleEvent(EventsOLD::SmiteCastTrash, 5s, 9s);
            events.ScheduleEvent(EventsOLD::SmiteCastSlam, 9s);
        }

        void MovementInform(uint32 type, uint32 /*id*/) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            me->SetFacingTo(5.47f);
            me->SetStandState(UNIT_STAND_STATE_KNEEL);
            _phaseTimer = 2000;
            _phase = 2;
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            // Phase handling (equipment swap, stand up, return to combat)
            if (_phase)
            {
                if (_phaseTimer <= diff)
                {
                    switch (_phase)
                    {
                    case 1:
                        if (!_isMoving)
                        {
                            if (GameObject* chest = ObjectAccessor::GetGameObject(*me, instance->GetGuidData(MiscOLD::DATA_SMITE_CHEST)))
                            {
                                me->GetMotionMaster()->Clear();
                                me->GetMotionMaster()->MovePoint(0, chest->GetPositionX() - 1.5f, chest->GetPositionY() + 1.4f, chest->GetPositionZ());
                                _isMoving = true;
                            }
                        }
                        break;
                    case 2:
                        if (_healthCheck == 1)
                            SetEquipmentSlots(false, EquipmentOLD::SmiteAxe, EquipmentOLD::SmiteAxe, EQUIP_NO_CHANGE);
                        else
                            SetEquipmentSlots(false, EquipmentOLD::SmiteMace, EQUIP_UNEQUIP, EQUIP_NO_CHANGE);
                        _phaseTimer = 500;
                        _phase = 3;
                        break;
                    case 3:
                        me->SetStandState(UNIT_STAND_STATE_STAND);
                        _phaseTimer = 750;
                        _phase = 4;
                        break;
                    case 4:
                        me->SetReactState(REACT_AGGRESSIVE);
                        SetCombatMovement(true);
                        me->GetMotionMaster()->MoveChase(me->GetVictim(), me->m_CombatDistance);
                        _isMoving = false;
                        _phase = 0;
                        break;
                    }
                }
                else _phaseTimer -= diff;
            }

            // Abilities only when not in phase transition
            if (!_isMoving)
            {
                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EventsOLD::SmiteCastTrash:
                        if (urand(0, 99) > 15)
                            DoCastSelf(SpellsOLD::SmiteTrash);
                        events.Repeat(6s, 15s);
                        break;
                    case EventsOLD::SmiteCastSlam:
                        if (urand(0, 99) > 15)
                            DoCastVictim(SpellsOLD::SmiteSlam);
                        events.Repeat(11s);
                        break;
                    default:
                        break;
                    }
                }
            }

            // Phase changes at 66% and 33% health
            if ((_healthCheck == 0 && !HealthAbovePct(66)) || (_healthCheck == 1 && !HealthAbovePct(33)))
            {
                ++_healthCheck;
                DoCastAOE(SpellsOLD::SmiteStomp, false);
                SetCombatMovement(false);
                me->AttackStop();
                me->InterruptNonMeleeSpells(false);
                me->SetReactState(REACT_PASSIVE);
                _phaseTimer = 2500;
                _phase = 1;

                Talk(_healthCheck == 1 ? TextsOLD::SmitePhase1 : TextsOLD::SmitePhase2);
            }

            me->DoMeleeAttackIfReady();
        }

    private:
        uint8 _phase = 0;
        uint8 _healthCheck = 0;
        bool _isMoving = false;
        uint32 _phaseTimer = 0;
        ObjectGuid _mrSmiteChestGUID;
    };

    struct npc_zidormi_deadmines_old : public ScriptedAI
    {
        npc_zidormi_deadmines_old(Creature* creature) : ScriptedAI(creature) {}

        void Reset() override
        {
            if (me->GetMapId() == 36)
                me->ReplaceAllNpcFlags(UNIT_NPC_FLAG_GOSSIP);
        }

        bool OnGossipHello(Player* player) override
        {
            if (me->GetMapId() != 36)
                return false;

            AddGossipItemFor(player, GossipOptionNpc::None, "I would like to face Edwin VanCleef!", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            SendGossipMenuFor(player, MiscOLD::ZidormiTextId, me->GetGUID());

            return true;
        }

        bool OnGossipSelect(Player* player, uint32 /*menuId*/, uint32 gossipListId) override
        {
            uint32 const action = player->PlayerTalkClass->GetGossipOptionAction(gossipListId);

            ClearGossipMenuFor(player);

            switch (action)
            {
            case GOSSIP_ACTION_INFO_DEF + 1:
                PhasingHandler::AddPhase(player, MiscOLD::PhaseDeadminesOLD, true);
                CloseGossipMenuFor(player);
                break;
            default:
                CloseGossipMenuFor(player);
                break;
            }

            return true;
        }
    };
}

void AddSC_custom_deadmines_old_npcs()
{
    using namespace Scripts::EasternKingdoms::Deadmines;

    RegisterCreatureAI(boss_rhahkzor);
    RegisterCreatureAI(boss_sneed);
    RegisterCreatureAI(boss_gilnid);
    RegisterCreatureAI(boss_mr_smite);
    RegisterCreatureAI(npc_zidormi_deadmines_old);
}
