#pragma once

#include <string>
#include <vector>

#include "Unit.h"
#include "Creature.h"

#include "Followship_bots_utils_spells.h"
#include "Followship_bots_ai_base.h" // for FSB_Roles and FSBUtils::GetRole

enum class FSBSayType;

namespace FSBUtilsCombat
{
    // Checks if either bot or player is in combat
    bool IsCombatActive(Creature* me);

    Unit* GetRandomAttacker(Creature* bot);

    // Counts active attackers
    uint8 CountActiveAttackers(Unit* me);

    // Counts active attackers on bot
    uint8 CountAttackersOn(Unit* who);
}

namespace FSBCombatUtils
{
    float GetBotChaseDistance(Creature* bot);
    float GetFallbackChaseDistanceForClass(FSB_Class cls);

    bool HasHostileInRange(Unit* me, float range, uint32 count);
    bool IsOutOfCombatFor(Creature* bot, uint32 ms);
}
