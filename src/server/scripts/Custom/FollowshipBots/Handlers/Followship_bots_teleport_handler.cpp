
#include "Followship_bots_teleport_handler.h"
#include "Followship_bots_utils_combat.h"
#include "Followship_bots_mgr.h"

namespace FSBTeleport
{
    bool BotTeleport(Creature* bot, FSB_TeleportReasons reason)
    {
        if (!bot)
            return false;

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

            if (FSBUtilsCombat::IsCombatActive(bot))
                return false;

            Player* player = FSBMgr::GetBotOwner(bot);
            if (!player)
                return false;

            if (player->IsInFlight())
                return false;

            if (!player->IsAlive())
                return false;

            if (bot->GetMapId() == player->GetMapId() && bot->GetDistance(player) > 200.0f)
            {
                bot->NearTeleportTo(
                    player->GetPositionX() + frand(3.f, 10.f),
                    player->GetPositionY(),
                    player->GetPositionZ(),
                    player->GetOrientation());

                TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Teleported bot {} to player {} due to distance > 200.", bot->GetName(), player->GetName());
                return true;
            }
            return false;
        }
        default:
            return false;
        }
    }
}
