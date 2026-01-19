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

    // Counts active attackers
    uint8 CountActiveAttackers(Unit* me);

    // Counts active attackers on bot
    uint8 CountAttackersOn(Unit* who);

    /// Sends a combat-related NPC say message with optional spell name and cooldown to prevent spam
    void SayCombatMessage(Creature* me, Unit* target, uint32 integer, FSBSayType sayType, uint32 spellId);

    // Return units that are in an emergency state (low HP, dispellable debuffs, etc.)
    std::vector<Unit*> BotGetHealCandidates(const std::vector<Unit*>& group, float lowHpThreshold);

    /// Calculates an emergency priority value for a unit (higher = more urgent)
    float CalculateEmergencyPriority(Unit* unit);

    /// Sorts units by emergency priority (highest first)
    void SortEmergencyTargets(std::vector<Unit*>& targets);

    // Builds a list containing:
    // - owner (player)
    // - the calling bot (me)
    // - other bots owned by the same player
    void CheckBotAllies(Unit* me, std::vector<Unit*>& outGroup, float searchRange = 100.0f);
}

namespace FSBUtilsBotCombat
{
    bool BotCanAttack(Unit* target, Creature* bot, uint16 moveState);
    void BotAttackStart(Creature* bot, Unit* target, uint16 moveState);
    void BotDoAttack(Creature* bot, Unit* target, bool chase, uint16 moveState);
    Unit* BotSelectNextTarget(Creature* bot, bool allowAutoSelect);
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
