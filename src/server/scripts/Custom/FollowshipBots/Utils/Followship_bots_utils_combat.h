#pragma once

#include "Unit.h"
#include "Creature.h"

namespace FSBCombatUtils
{
    // Actually gets a random target from attackers so maybe rename?
    Unit* GetRandomAttacker(Creature* bot);

    // Checks if either bot or player is in combat
    bool IsCombatActive(Creature* me);

    float GetBotChaseDistance(Creature* bot);
    float GetFallbackChaseDistanceForClass(FSB_Class cls);

    bool HasHostileInRange(Unit* bot, float range, uint32 count);
    bool IsOutOfCombatFor(Creature* bot, uint32 ms);
}
