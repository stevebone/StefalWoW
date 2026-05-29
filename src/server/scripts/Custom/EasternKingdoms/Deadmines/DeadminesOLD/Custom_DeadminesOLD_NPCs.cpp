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
                instance->SetData(Data::DeadminesVersion, Version::Classic);

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
    RegisterCreatureAI(npc_zidormi_deadmines_old);
}
