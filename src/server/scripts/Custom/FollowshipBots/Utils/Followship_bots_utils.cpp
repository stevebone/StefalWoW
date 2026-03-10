#include <algorithm>
#include <vector>

#include "Containers.h"
#include "CreatureAI.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Log.h"
#include "ThreatManager.h"

#include "Followship_bots.h"
#include "Followship_bots_config.h"
#include "Followship_bots_utils.h"
#include "Followship_bots_ai_base.h"
#include "Followship_bots_mgr.h"

#include "Followship_bots_gossip_handler.h"
#include "Followship_bots_powers_handler.h"

namespace FSBUtils
{
    const char* BotClassToString(FSB_Class cls)
    {
        switch (cls)
        {
        case FSB_Class::Warrior: return "Warrior";
        case FSB_Class::Priest:  return "Priest";
        case FSB_Class::Mage:    return "Mage";
        case FSB_Class::Rogue:   return "Rogue";
        case FSB_Class::Druid:   return "Druid";
        case FSB_Class::Paladin: return "Paladin";
        case FSB_Class::Hunter:  return "Hunter";
        case FSB_Class::Warlock: return "Warlock";
        case FSB_Class::Shaman:  return "Shaman";
        default:                 return "Unknown";
        }
    }

    const char* BotRaceToString(FSB_Race race)
    {
        switch (race)
        {
        case FSB_Race::Human:    return "Human";
        case FSB_Race::Dwarf:    return "Dwarf";
        case FSB_Race::Draenei:  return "Draenei";
        case FSB_Race::NightElf: return "NightElf";
        case FSB_Race::Gnome:    return "Gnome";
        case FSB_Race::Worgen:   return "Worgen";
        case FSB_Race::Pandaren: return "Pandaren";
        default:                 return "Unknown";
        }
    }

    Races BotRaceToTC(FSB_Race race)
    {
        switch (race)
        {
        case FSB_Race::Human:    return RACE_HUMAN;
        case FSB_Race::Dwarf:    return RACE_DWARF;
        case FSB_Race::Draenei:  return RACE_DRAENEI;
        case FSB_Race::NightElf: return RACE_NIGHTELF;
        case FSB_Race::Gnome:    return RACE_GNOME;
        case FSB_Race::Worgen:   return RACE_WORGEN;
        case FSB_Race::Pandaren: return RACE_PANDAREN_ALLIANCE;
        default:
            TC_LOG_WARN("scripts.ai.fsb", "FSB Utils BotRaceToTC has no mapping for race {}", race);
            return RACE_NONE;
        }
    }

    const char* PowerTypeToString(Powers power)
    {
        switch (power)
        {
        case POWER_MANA:   return "Mana";
        case POWER_RAGE:   return "Rage";
        case POWER_ENERGY: return "Energy";
        case POWER_FOCUS:  return "Focus";
        case POWER_HOLY_POWER: return "Holy";
        case POWER_SOUL_SHARDS:return "Sould Shards";
        case POWER_CHI:    return "Chi";
        default:           return "Other";
        }
    }

    Classes FSBToTCClass(FSB_Class botClass)
    {
        switch (botClass)
        {
        case FSB_Class::Warrior:            return CLASS_WARRIOR;
        case FSB_Class::Priest:             return CLASS_PRIEST;
        case FSB_Class::Mage:               return CLASS_MAGE;
        case FSB_Class::Rogue:              return CLASS_ROGUE;
        case FSB_Class::Druid:              return CLASS_DRUID;
        case FSB_Class::Paladin:            return CLASS_PALADIN;
        case FSB_Class::Hunter:             return CLASS_HUNTER;
        case FSB_Class::Warlock:            return CLASS_WARLOCK;
        case FSB_Class::Shaman:             return CLASS_SHAMAN;
        case FSB_Class::None:               return CLASS_NONE;
        default:
            break;
        }

        return CLASS_NONE;
    }


    float GetRandomLeftAngle()
    {
        // Base left is +90 degrees (M_PI/2)
        // Add a random small offset for natural variation
        return (M_PI / 2.0f) + frand(-SIDE_OFFSET_MAX, SIDE_OFFSET_MAX);
    }

    float GetRandomRightAngle()
    {
        // Base right is -90 degrees (-M_PI/2)
        // Add a random small offset for natural variation
        return (-M_PI / 2.0f) + frand(-SIDE_OFFSET_MAX, SIDE_OFFSET_MAX);
    }

    bool TryChargeHire(Player* player, uint32 duration)
    {
        uint32 pLevel = player->GetLevel();

        int64 price = int64(FollowshipBotsConfig::configFSBPricePerLevel) * pLevel * duration;

        if (!player->HasEnoughMoney(price))
        {
            player->GetSession()->SendNotification(FSB_PLAYER_NOTIFICATION_PAYMENT_FAIL);
            return false;
        }

        player->ModifyMoney(-price);
        player->GetSession()->SendNotification(FSB_PLAYER_NOTIFICATION_PAYMENT_SUCCESS);
        return true;
    }

    bool BotIsHealerClass(Creature* bot)
    {
        if (!bot)
            return false;

        FSB_Class cls = FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry());

        if (cls == FSB_Class::Priest || cls == FSB_Class::Druid || cls == FSB_Class::Paladin || cls == FSB_Class::Shaman || cls == FSB_Class::Monk)
            return true;

        return false;
    }

    Unit* FindCreatureByName(WorldObject* bot, std::string const& name, float range)
    {
        if (!bot)
            return nullptr;

        std::list<WorldObject*> nearbyObjects;

        Trinity::AllWorldObjectsInRange checker(bot, range);
        Trinity::WorldObjectListSearcher<Trinity::AllWorldObjectsInRange> searcher(bot, nearbyObjects, checker);

        Cell::VisitAllObjects(bot, searcher, range);
        for (WorldObject* object : nearbyObjects)
        {
            if (!object)
                continue;

            Unit* c = object->ToUnit();
            if (!c)
                continue;

            if (c->GetName() == name)
                return c;
        }


        return nullptr;
    }

    std::vector<Creature*> FindNearbyBots(Creature* center, float radius)
    {
        std::vector<Creature*> result;

        Trinity::AnyUnitInObjectRangeCheck checker(center, radius);
        Trinity::CreatureListSearcher<Trinity::AnyUnitInObjectRangeCheck> searcher(center, result, checker);

        Cell::VisitAllObjects(center, searcher, radius);

        // Filter only your bot entries
        result.erase(
            std::remove_if(result.begin(), result.end(),
                [](Creature* c)
                {
                    return !c->IsAlive() || dynamic_cast<FSB_BaseAI*>(c->AI()) == nullptr;
                }),
            result.end()
        );

        return result;
    }
}

