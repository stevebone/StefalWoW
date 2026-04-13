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

    Team GetTeamFromFSBRace(Creature* bot)
    {
        if (!bot)
        {
            TC_LOG_WARN("scripts.fsb.general", "GetTeamFromFSBRace: bot pointer was null!");
            return Team::PANDARIA_NEUTRAL;
        }

        FSB_Race race = FSBMgr::Get()->GetBotRaceForEntry(bot->GetEntry());

        switch (race)
        {
            // Alliance races
        case FSB_Race::Human:
        case FSB_Race::Dwarf:
        case FSB_Race::NightElf:
        case FSB_Race::Gnome:
        case FSB_Race::Draenei:
        case FSB_Race::Worgen:
        case FSB_Race::VoidElf:
        case FSB_Race::Pandaren:
            return Team::ALLIANCE;

            // Pandaren can be neutral or faction-chosen later
        //case FSB_Race::Pandaren:
        //    TC_LOG_WARN("server", "GetTeamFromFSBRace: Pandaren bot has no faction assigned, defaulting to NEUTRAL.");
        //    return Team::PANDARIA_NEUTRAL;

            // No race / unknown
        case FSB_Race::None:
        default:
            TC_LOG_WARN("scripts.fsb.general", "GetTeamFromFSBRace: Unknown or unsupported FSB race {} for bot {}. Defaulting to NEUTRAL.",
                uint8(race), bot->GetName());
            return Team::PANDARIA_NEUTRAL;
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

    Unit* FindCreatureByName(WorldObject* bot, std::string name, float range)
    {
        if (!bot)
            return nullptr;

        // Replace underscores with spaces
        std::replace(name.begin(), name.end(), '_', ' ');

        // Optional: lowercase for case-insensitive matching
        std::string searchName = name;
        std::transform(searchName.begin(), searchName.end(), searchName.begin(), ::tolower);

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

            // Lowercase creature name for comparison
            std::string creatureName = c->GetName();
            std::transform(creatureName.begin(), creatureName.end(), creatureName.begin(), ::tolower);

            if (creatureName == searchName)
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

    bool IsBotInTradeCity(Creature* bot)
    {
        if (!bot)
            return false;

        uint32 zoneId = bot->GetZoneId();

        switch (zoneId)
        {
        case 1519:  // Stormwind City
        case 1537:  // Ironforge
        case 1657:  // Darnassus
        case 3557:  // The Exodar
        case 1637:  // Orgrimmar
        case 1638:  // Thunder Bluff
        case 1497:  // Undercity
        case 3487:  // Silvermoon City
            return true;

        default:
            return false;
        }
    }
}

