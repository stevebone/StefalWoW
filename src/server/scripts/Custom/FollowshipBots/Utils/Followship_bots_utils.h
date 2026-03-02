#pragma once

#include <cmath>
#include <string>
#include <vector>
#include <random>
#include <cstdint>

#include "Creature.h"
#include "CreatureAIImpl.h"
#include "MotionMaster.h"
#include "Player.h"
#include "Unit.h"

#include "Followship_bots_defines.h"
#include "Followship_bots_utils_combat.h"
#include "Followship_bots_utils_gossip.h"
#include "Followship_bots_utils_spells.h"
#include "Followship_bots_utils_stats.h"


namespace FSBUtils
{
    const char* BotClassToString(FSB_Class cls);
    const char* BotRaceToString(FSB_Race race);
    const char* PowerTypeToString(Powers power);

    Races BotRaceToTC(FSB_Race race);

    constexpr float SIDE_OFFSET_MAX = float(M_PI) / 6.0f; // +-30 degrees from pure left/right

    // Returns a random angle slightly to the left of the player
    float GetRandomLeftAngle();

    // Returns a random angle slightly to the right of the player
    float GetRandomRightAngle();

    bool TryChargeHire(Player* player, uint32 duration);

    bool BotIsHealerClass(Creature* bot);

    Unit* FindCreatureByName(WorldObject* bot, std::string const& name, float range = 50.0f);
}

namespace FSBUtilsMovement
{
    // Returns true if chase was started or is ongoing
    bool EnsureInRange(Creature* me, Unit* target, float requiredRange);
    bool EnsureLOS(Unit* me, Unit* target);

    
}

namespace FSBUtilsTexts
{
    // Converts an int64 price in copper to a string like 10 silver
    std::string MoneyToString(int64 price);

    void ReplaceAll(std::string& text, const std::string& from, const std::string& to);

    // Builds the hire option text dynamically
    std::string BuildHireText(int64 price, uint32 hours);
}
