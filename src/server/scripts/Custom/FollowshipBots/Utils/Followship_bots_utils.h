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

#include "Followship_bots_ai_base.h"
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

    Classes FSBToTCClass(FSB_Class botClass);
    Races BotRaceToTC(FSB_Race race);

    constexpr float SIDE_OFFSET_MAX = float(M_PI) / 6.0f; // +-30 degrees from pure left/right

    // Returns a random angle slightly to the left of the player
    float GetRandomLeftAngle();

    // Returns a random angle slightly to the right of the player
    float GetRandomRightAngle();

    bool TryChargeHire(Player* player, uint32 duration);

    bool BotIsHealerClass(Creature* bot);

    Unit* FindCreatureByName(WorldObject* bot, std::string const& name, float range = 50.0f);
    std::vector<Creature*> FindNearbyBots(Creature* center, float radius = 300.f);
}
