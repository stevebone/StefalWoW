#include <algorithm>
#include <vector>

#include "Containers.h"

#include "CreatureAI.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
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
}

namespace FSBUtilsMovement
{
    bool FSBUtilsMovement::EnsureInRange(Creature* me, Unit* target, float requiredRange)
    {
        if (!me || !target || !target->IsAlive())
            return false;

        float dist = me->GetDistance(target);

        // Already in range
        if (dist <= requiredRange)
            return false;

        MotionMaster* mm = me->GetMotionMaster();
        if (!mm)
            return false;

        // Avoid restarting the same chase every tick
        if (mm->GetCurrentMovementGeneratorType() == CHASE_MOTION_TYPE)
            return true;

        mm->Clear();
        mm->MoveChase(target, requiredRange);

        TC_LOG_DEBUG("scripts.ai.fsb",
            "FSB EnsureInRange: chasing {} to {:.1f} yards",
            target->GetName(), requiredRange);

        return true;
    }

    bool FSBUtilsMovement::EnsureLOS(Unit* me, Unit* target)
    {
        if (!me || !target)
            return false;

        if (me->IsWithinLOSInMap(target))
            return false; // LOS OK ? no movement needed

        MotionMaster* mm = me->GetMotionMaster();
        if (!mm)
            return false;

        // Move directly toward target to resolve LOS
        mm->MoveChase(target);

        return true; // movement started
    }

    
}


namespace FSBUtilsTexts
{
    // Converts an int64 price in copper to a string like "10 silver"
    std::string MoneyToString(int64 price)
    {
        int32 gold = price / 10000;
        int32 silver = (price % 10000) / 100;
        int32 copper = price % 100;
        std::string result;
        if (gold > 0)   result += std::to_string(gold) + " gold ";
        if (silver > 0) result += std::to_string(silver) + " silver ";
        if (copper > 0) result += std::to_string(copper) + " copper";
        return result.empty() ? "0 copper" : result;
    }

    // Builds the hire option text dynamically
    std::string BuildHireText(int64 price, uint32 hours)
    {
        return std::to_string(hours) + " hour service: " + MoneyToString(price);
    }

    void ReplaceAll(std::string& text, const std::string& from, const std::string& to)
    {
        size_t pos = 0;
        while ((pos = text.find(from, pos)) != std::string::npos)
        {
            text.replace(pos, from.length(), to);
            pos += to.length();
        }
    }

    std::string BuildNPCSayText(const std::string& playerName, uint32 duration, FSBSayType type, const std::string& string2)
    {
        switch (type)
        {
        case FSBSayType::Hire:
        {
            static const std::vector<std::string> texts =
            {
                "Hey <name>, I am all yours for the next <duration> hours.",
                "Alright <name>, let's band together for <duration> hours.",
                "Ok, I'll come along, <name>, since you paid me for <duration> hours.",
                "Only <duration> hours? Sure thing, <name>.",
                "Well <name>, my calendar is empty for <duration> hours. Let's make poor life choices.",
                "I suppose <duration> hours with you won't kill me, <name>. Probably.",
                "You pay, I follow, <name>. <duration> hours it is.",
                "For <duration> hours, <name>, I am your loyal companion. After that, we renegotiate.",
                "Ah yes, <duration> hours of danger, glory, and questionable decisions, <name>.",
                "Very well <name>, I shall tolerate you for <duration> hours.",
                "<duration> hours? Fine. But if we die, it's your fault, <name>.",
                "I was bored anyway, <name>. <duration> hours sounds perfect.",
                "Gold talks, <name>. And it says '<duration> hours of adventure.'",
                "Let's do this, <name>. <duration> hours in, we pretend we planned it."
            };

            std::string chosen = Trinity::Containers::SelectRandomContainerElement(texts);
            ReplaceAll(chosen, "<name>", playerName);
            ReplaceAll(chosen, "<duration>", std::to_string(duration));
            return chosen;
        }

        


        

        default:
            return "Hello " + playerName + ".";
        }
    }

    
}


