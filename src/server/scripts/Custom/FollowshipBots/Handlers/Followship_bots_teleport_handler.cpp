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

#include "Log.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "InstanceScript.h"

#include "Followship_bots_mgr.h"
#include "Followship_bots_utils.h"

#include "Followship_bots_pet_handler.h"
#include "Followship_bots_teleport_handler.h"


namespace FSBTeleport
{
    bool BotTeleport(Creature* bot, FSB_TeleportReasons reason)
    {
        if (!bot)
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        Player* player = FSBMgr::Get()->GetBotOwner(bot);

        //TC_LOG_DEBUG("scripts.ai.fsb", "FSB BotTeleport - With Death teleport for bot {}", bot->GetName());

        switch (reason)
        {
        case BOT_DEATH:
        {
            if (bot->IsAlive())
                return false;

            if (bot->GetMap()->IsDungeon())
            {
                Map* map = bot->GetMap();

                uint32 mapId = bot->GetMapId();

                AreaTriggerTeleport const* areaTrigger = nullptr;

                if (InstanceScript const* instanceScript = map->ToInstanceMap()->GetInstanceScript())
                    areaTrigger = sObjectMgr->GetWorldSafeLoc(instanceScript->GetEntranceLocation());

                if (!areaTrigger)
                    areaTrigger = sObjectMgr->GetMapEntranceTrigger(mapId);

                if (areaTrigger)
                {
                    Position pos = areaTrigger->Loc;

                    TC_LOG_DEBUG("scripts.ai.fsb", "FSB: bot died in dungeon. Teleport entrance: X {}, Y {}, Z {}", pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ());

                    bot->NearTeleportTo(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation());
                    return true;
                }
            }
            else
            {
                Team team = FSBUtils::GetTeamFromFSBRace(bot);
                WorldSafeLocsEntry const* graveyard = sObjectMgr->GetClosestGraveyard(*bot, team, bot);

                if (graveyard)
                {
                    Position pos = graveyard->Loc;
                    bot->NearTeleportTo(pos);
                    return true;
                }

            }

            return false;
        }
            
        case BOT_TOO_FAR:
        {
            if (!player)
                return false;

            if (!bot->IsAlive() || !player->IsAlive())
                return false;

            if (bot->HasAura(SPELL_SPECIAL_GHOST))
                return false;

            if (FSBCombatUtils::IsCombatActive(bot))
                return false;

            if (baseAI->botMoveState == FSB_MOVE_STATE_STAY)
                return false;

            if (player->IsInFlight())
                return false;

            if (bot->GetMapId() == player->GetMapId() && bot->GetDistance(player) > 100.0f)
            {
                bot->NearTeleportTo(
                    player->GetPositionX() + frand(3.f, 10.f),
                    player->GetPositionY(),
                    player->GetPositionZ(),
                    player->GetOrientation());

                TC_LOG_DEBUG("scripts.fsb.movement", "FSB: BotTeleport Teleported bot {} to player {} due to distance > 100.", bot->GetName(), player->GetName());

                return true;
            }
            return false;
        }
        default:
            return false;
        }
    }

    void BotPetTeleport(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return;

        if (FSBCombatUtils::IsCombatActive(bot))
            return;

        Unit* pet = nullptr;

        if (FSBPet::BotHasPet(bot))
            pet = FSBPet::GetBotPet(bot);

        if (pet && pet->IsAlive())
        {
            if (bot->GetMapId() == pet->GetMapId() && bot->GetDistance(pet) > 100.0f)
            {
                pet->NearTeleportTo(
                    bot->GetPositionX() + frand(3.f, 10.f),
                    bot->GetPositionY(),
                    bot->GetPositionZ(),
                    bot->GetOrientation());

                TC_LOG_DEBUG("scripts.fsb.movement", "FSB: BotPetTeleport Teleported bot pet {} to bot {} due to distance > 100.", pet->GetName(), bot->GetName());
            }
        }


    }
}
