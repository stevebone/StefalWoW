#pragma once

#include <string>
#include <vector>

#include "Unit.h"
#include "Creature.h"

#include "followship_bots_utils_spells.h"
#include "followship_bots_ai_base.h" // for FSB_Roles and FSBUtils::GetRole

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

    /// Sends a combat-related NPC say message with optional spell name and cooldown to prevent spam
    void SayCombatMessage(Creature* me, Unit* target, uint32 integer, FSBSayType sayType, uint32 spellId);   
}

namespace FSBUtilsBotCombat
{
    bool BotCanAttack(Unit* target, Creature* bot, uint16 moveState);
    void BotAttackStart(Creature* bot, Unit* target, uint16 moveState);
    void BotDoAttack(Creature* bot, Unit* target, bool chase, uint16 moveState);
    Unit* BotSelectNextTarget(Creature* bot, bool allowAutoSelect, std::vector<Unit*> botGroup_);
    void BotHandleReturnMovement(Creature* bot, uint16 moveState, float followDist, float followAngle);

    float GetBotChaseDistance(Creature* bot);
    float GetFallbackChaseDistanceForClass(FSB_Class cls);
    bool ShouldForceMeleeRange(Creature* bot);
}

namespace FSBUtilsOwnerCombat
{
    // Owner attacked victim
    bool CheckBotOwnerAttacked(Player* owner, ObjectGuid& lastVictim);

    // Owner was attacked by target
    Unit* CheckBotOwnerAttackedBy(Player* owner);

    // Bot reaction when owner attacks target
    void OnBotOwnerAttacked(Unit* victim, Creature* bot, uint16 moveState);

    // Bot reaction when owner is attacked by target
    void OnBotOwnerAttackedBy(Unit* attacker, Creature* bot, uint16 moveState);
}

namespace FSBCombatUtils
{
    bool HasHostileInRange(Unit* me, float range, uint32 count);
    bool IsOutOfCombatFor(Creature* bot, uint32 ms);
}
