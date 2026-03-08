#include "Log.h"
#include "Map.h"

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
                WorldSafeLocsEntry const* graveyard = sObjectMgr->GetClosestGraveyard(*bot, TEAM_ALLIANCE, bot);

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
            if (!bot->IsAlive())
                return false;

            if (FSBCombatUtils::IsCombatActive(bot))
                return false;

            Player* player = FSBMgr::Get()->GetBotOwner(bot);
            if (!player)
                return false;

            if (player->IsInFlight())
                return false;

            if (!player->IsAlive())
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
