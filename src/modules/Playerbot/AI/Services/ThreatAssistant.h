/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 */

#ifndef _PLAYERBOT_THREATASSISTANT_H
#define _PLAYERBOT_THREATASSISTANT_H

#include "Common.h"
#include "ObjectGuid.h"
#include <vector>

class Player;
class Unit;

namespace Playerbot {
namespace bot { namespace ai {

/**
 * @struct ThreatTarget
 * @brief Comprehensive threat assessment for a target
 *
 * Factors considered:
 * - Threat percentage relative to tank
 * - Target is attacking vulnerable allies (healer/DPS)
 * - Distance to tank
 * - Duration out of control
 * - Target's danger level (damage output, special abilities)
 */
struct ThreatTarget
{
    Unit* unit;
    float threatPercent;        // % of tank's threat (100 = equal, 200 = double)
    bool isDangerous;           // Attacking healer/DPS?
    float distanceToTank;       // Distance from tank
    uint32 timeOutOfControl;    // Milliseconds not being tanked
    float dangerRating;         // 0.0-10.0 (how dangerous is this target?)
    ObjectGuid currentTarget;   // Who is it attacking?

    ThreatTarget()
        : unit(nullptr)
        , threatPercent(0.0f)
        , isDangerous(false)
        , distanceToTank(0.0f)
        , timeOutOfControl(0)
        , dangerRating(0.0f)
        , currentTarget()
    {}

    /**
     * @brief Calculate taunt priority score
     *
     * Formula:
     * Score = (threatPercent / 100) × dangerRating × distance_penalty
     * - Doubled if attacking healer
     * - +50% if out of control > 3 seconds
     *
     * @return Priority score (higher = more urgent to taunt)
     */
    [[nodiscard]] float CalculateTauntPriority() const;
};

/**
 * @class ThreatAssistant
 * @brief Unified threat management for all tank specs
 *
 * **Problem**: Taunt logic duplicated 35+ times across tank specs
 * - ProtectionWarrior.h
 * - ProtectionPaladin.h
 * - BloodDeathKnight.h
 * - VengeanceDemonHunter.h
 * - GuardianDruid.h
 * - BrewmasterMonk.h
 * - Total: ~500 lines of duplication
 *
 * **Solution**: Single unified service with sophisticated threat assessment
 *
 * **Usage Example**:
 * @code
 * // BEFORE (each tank spec):
 * if (Unit* target = GetTarget())
 * {
 *     if (ShouldTaunt(target)) // complex threat calculation logic
 *     {
 *         if (this->CanCastSpell(SPELL_TAUNT, target))
 *             this->CastSpell(SPELL_TAUNT, target);
 *     }
 * }
 *
 * // AFTER (1 line):
 * if (Unit* target = ThreatAssistant::GetTauntTarget(this->GetBot()))
 *     ThreatAssistant::ExecuteTaunt(this->GetBot(), target, SPELL_TAUNT);
 * @endcode
 *
 * **Expected Impact**:
 * -  Eliminate 500 lines of duplication
 * -  Consistent threat management
 * -  Easier to tune (one place)
 * -  Better multi-tank coordination
 * -  Smart target prioritization (protect healers first)
 */
class TC_GAME_API ThreatAssistant
{
public:
    /**
     * @brief Check if taunt is needed and get best target
     *
     * @param tank Tank bot
     * @return Target that needs taunting or nullptr
     *
     * Decision logic:
     * 1. Find all targets in combat
     * 2. Calculate threat for each target
     * 3. Identify targets not on tank
     * 4. Prioritize: Healer attackers > DPS attackers > others
     * 5. Return highest priority target
     *
     * Example:
     * - Mob A attacking healer (50% threat): Priority = HIGH
     * - Mob B attacking DPS (80% threat): Priority = MEDIUM
     * - Mob C attacking tank (100% threat): Priority = NONE
     * Result: Taunt Mob A (protect healer)
     */
    static Unit* GetTauntTarget(Player* tank);

    /**
     * @brief Execute taunt ability (spec-specific)
     *
     * @param tank Tank bot
     * @param target Target to taunt
     * @param tauntSpellId Taunt spell for this spec
     * @return True if taunt was executed successfully
     *
     * Handles:
     * - Range check
     * - Line of sight check
     * - Cooldown check
     * - Taunt immunity check
     * - Execution
     */
    static bool ExecuteTaunt(Player* tank, Unit* target, uint32 tauntSpellId);

    /**
     * @brief Get all targets threatening group
     *
     * @param tank Tank bot
     * @param minThreatPercent Only include targets above this threat % (default: 60)
     * @return Vector of threatening targets sorted by priority
     *
     * Use case: Identify all targets that need attention
     */
    static std::vector<ThreatTarget> GetDangerousTargets(
        Player* tank,
        float minThreatPercent = 60.0f
    );

    /**
     * @brief Calculate if tank should use AoE taunt
     *
     * @param tank Tank bot
     * @param minTargets Minimum targets to justify AoE taunt (default: 3)
     * @return True if AoE taunt is recommended
     *
     * Use case: Challenging Shout, Mass Taunt
     * Logic: Count targets not on tank, recommend if >= minTargets
     */
    static bool ShouldAoETaunt(Player* tank, uint32 minTargets = 3);

    /**
     * @brief Get threat percentage for target
     *
     * @param tank Tank
     * @param target Target to check
     * @return Threat percentage (100 = equal, 200 = double tank's threat)
     *
     * Use case: Check if target is about to pull aggro
     */
    static float GetThreatPercentage(Player* tank, Unit* target);

    /**
     * @brief Check if target is on tank
     *
     * @param tank Tank
     * @param target Target to check
     * @return True if target is attacking tank
     *
     * Use case: Quick check if taunt needed
     */
    static bool IsTargetOnTank(Player* tank, Unit* target);

    /**
     * @brief Get target's current victim
     *
     * @param target Target to check
     * @return Who the target is attacking or nullptr
     */
    static Unit* GetTargetVictim(Unit* target);

    /**
     * @brief Check if target is taunt immune
     *
     * @param target Target to check
     * @return True if target cannot be taunted
     *
     * Use case: Avoid wasting taunt on immune targets (bosses, mechanical)
     */
    static bool IsTauntImmune(Unit* target);

    /**
     * @brief Get all enemies in combat with group
     *
     * @param tank Tank bot
     * @param range Maximum range (default: 40 yards)
     * @return Vector of all hostile units in combat
     */
    static std::vector<Unit*> GetCombatEnemies(Player* tank, float range = 40.0f);

    /**
     * @brief Coordinate multi-tank taunting
     *
     * @param tank Tank bot
     * @param otherTanks Other tanks in group
     * @param target Target to check
     * @return True if this tank should taunt (avoid taunt conflicts)
     *
     * Use case: Prevent multiple tanks taunting same target
     * Logic: Assign targets to tanks based on proximity and role
     */
    static bool ShouldThisTankTaunt(
        Player* tank,
        const std::vector<Player*>& otherTanks,
        Unit* target
    );

private:
    /**
     * @brief Calculate danger rating for target
     *
     * @param target Target to evaluate
     * @return Danger rating 0.0-10.0
     *
     * Factors:
     * - Damage output (high damage = high danger)
     * - Special abilities (caster, healer)
     * - Elite/boss status
     */
    static float CalculateDangerRating(Unit* target);

    /**
     * @brief Check if target is attacking vulnerable ally
     *
     * @param target Target to check
     * @return True if attacking healer or low-armor DPS
     */
    static bool IsAttackingVulnerableAlly(Unit* target);

    /**
     * @brief Get role of player
     *
     * @param player Player to check
     * @return Role (tank, healer, dps)
     */
    enum class PlayerRole { TANK, HEALER, DPS, UNKNOWN };
    static PlayerRole GetPlayerRole(Player* player);

    /**
     * @brief Calculate distance penalty for taunt priority
     *
     * @param tank Tank position
     * @param target Target position
     * @return Penalty multiplier (closer = higher priority)
     */
    static float CalculateDistancePenalty(Player* tank, Unit* target);

    /**
     * @brief Track time out of control for target
     *
     * @param target Target to check
     * @return Milliseconds since target lost aggro on tank
     */
    static uint32 GetTimeOutOfControl(Unit* target);

    // Threat tracking
    static std::unordered_map<ObjectGuid, uint32> _lostAggroTimestamps;
};

}} // namespace bot::ai
} // namespace Playerbot

#endif
