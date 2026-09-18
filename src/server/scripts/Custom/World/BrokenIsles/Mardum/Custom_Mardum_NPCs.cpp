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
#include "CreatureAI.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "Unit.h"

#include "Custom_Mardum_Defines.h"

namespace Scripts::Custom::Mardum
{
    // 99916 - Sevis Brightflame (Ashtongue Gateway, shared/non-private spawn)
    struct npc_sevis_brightflame_ashtongue_gateway : public ScriptedAI
    {
        npc_sevis_brightflame_ashtongue_gateway(Creature* creature) : ScriptedAI(creature) { }

        void MoveInLineOfSight(Unit* who) override
        {
            Player* player = who->ToPlayer();
            if (!player)
                return;

            if (!me->IsWithinDist(player, Misc::TalkDistance))
                return;

            if (player->GetQuestStatus(Quests::EnterTheIllidariAshtongue) != QUEST_STATUS_INCOMPLETE)
                return;

            // greet each player only once
            if (_greetedPlayers.insert(player->GetGUID()).second)
                Talk(CreatureText::SevisAshtongueGreeting, player);
        }

    private:
        GuidUnorderedSet _greetedPlayers;
    };

    // Factory used by SevisBrightflameAshtongueGatewayAISelector in zone_mardum.cpp
    // for the shared (non-private) spawn. Returns CreatureAI* so the concrete type
    // stays private to this translation unit.
    CreatureAI* CreateSevisBrightflameAshtongueGatewayAI(Creature* creature)
    {
        return new npc_sevis_brightflame_ashtongue_gateway(creature);
    }
}
