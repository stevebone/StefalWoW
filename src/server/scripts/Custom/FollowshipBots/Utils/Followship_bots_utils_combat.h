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
    bool IsCombatActive(Unit* me);

    // Counts active attackers
    uint8 CountActiveAttackers(Unit* me);

    // Counts active attackers on bot
    uint8 CountAttackersOn(Unit* who);

    /// Sends a combat-related NPC say message with optional spell name and cooldown to prevent spam
    void SayCombatMessage(Creature* me, Unit* target, uint32 integer, FSBSayType sayType, uint32 spellId);

    /// Selects a damage target for the bot based on its role and the group context
    Unit* SelectDamageTarget(Creature* me, const std::vector<Unit*>& group, Unit* playerTarget = nullptr);

    /// Selects a heal target for the bot based on its role and the group context
    Unit* SelectHealTarget(Unit* me, const std::vector<Unit*>& group);

    // Return units that are in an emergency state (low HP, dispellable debuffs, etc.)
    std::vector<Unit*> GetEmergencyCandidates(const std::vector<Unit*>& group, float lowHpThreshold = 30.0f);

    /// Calculates an emergency priority value for a unit (higher = more urgent)
    float CalculateEmergencyPriority(Unit* unit);

    /// Sorts units by emergency priority (highest first)
    void SortEmergencyTargets(std::vector<Unit*>& targets);

    // Builds a list containing:
    // - owner (player)
    // - the calling bot (me)
    // - other bots owned by the same player
    void BuildBotGroup(Unit* me, std::vector<Unit*>& outGroup, float searchRange = 100.0f);
}
