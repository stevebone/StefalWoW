/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 */

#ifndef _PLAYERBOT_HEALINGTARGETSELECTOR_H
#define _PLAYERBOT_HEALINGTARGETSELECTOR_H

#include "Common.h"
#include "ObjectGuid.h"
#include "SharedDefines.h"  // DispelType
#include <vector>

class Player;
class Unit;

namespace Playerbot {
namespace bot { namespace ai {

/**
 * @struct TargetPriority
 * @brief Comprehensive healing priority calculation
 *
 * Factors considered:
 * - Health deficit (lower health = higher priority)
 * - Role priority (tank > healer > DPS)
 * - Distance (closer = higher priority)
 * - Incoming heals (already being healed = lower priority)
 * - Debuff count (dispellable debuffs increase priority)
 */
struct TargetPriority
{
    Player* player;
    float healthDeficit;        // 100 - healthPct (0-100)
    float rolePriority;         // Tank: 2.0, Healer: 1.5, DPS: 1.0
    float distanceFactor;       // 0.0-1.0 (1.0 = close, 0.0 = far)
    bool hasIncomingHeals;      // Already being healed?
    uint32 debuffCount;         // Dispellable debuffs
    float threatFactor;         // 0.0-1.0 (1.0 = high threat)
    bool isMainTank;            // Designated main tank?

    TargetPriority()
        : player(nullptr)
        , healthDeficit(0.0f)
        , rolePriority(1.0f)
        , distanceFactor(1.0f)
        , hasIncomingHeals(false)
        , debuffCount(0)
        , threatFactor(0.0f)
        , isMainTank(false)
    {}

    /**
     * @brief Calculate final priority score
     *
     * Formula:
     * Score = (healthDeficit × rolePriority × distanceFactor) + (debuffCount × 10)
     * - Reduced by 30% if already being healed
     * - Increased by 20% if main tank
     * - Increased by threat factor
     *
     * @return Priority score (higher = more urgent)
     */
    [[nodiscard]] float CalculateScore() const;
};

/**
 * @class HealingTargetSelector
 * @brief Unified healing target selection for all healer specs
 *
 * **Problem**: SelectHealingTarget() duplicated 8+ times across healer specs
 * - HolyPriest.h
 * - MistweaverMonk.h
 * - HolyPaladin.h
 * - RestorationDruid.h
 * - RestorationShaman.h
 * - 3+ other specs
 * - Total: ~1,600 lines of duplication
 *
 * **Solution**: Single unified service with sophisticated priority calculation
 *
 * **Usage Example**:
 * @code
 * // BEFORE (40+ lines in each healer spec):
 * Player* SelectHealingTarget()
 * {
 *     Group* group = this->GetBot()->GetGroup();
 *     if (!group) return nullptr;
 *     Player* lowestHealthAlly = nullptr;
 *     float lowestHealth = 100.0f;
 *     for (GroupReference& ref : group->GetMembers())
 *     {
 *         Player* member = ref.GetSource();
 *         if (!member || member->isDead()) continue;
 *         float healthPct = member->GetHealthPct();
 *         if (healthPct < lowestHealth)
 *         {
 *             lowestHealth = healthPct;
 *             lowestHealthAlly = member;
 *         }
 *     }
 *     return lowestHealthAlly;
 * }
 *
 * // AFTER (1 line):
 * Player* target = HealingTargetSelector::SelectTarget(this->GetBot());
 * @endcode
 *
 * **Expected Impact**:
 * -  Eliminate 1,600 lines of duplication
 * -  Single source of truth for healing logic
 * -  Easier to improve (improve once = all specs benefit)
 * -  Consistent behavior across healers
 * -  Advanced features: role priority, threat awareness, incoming heal tracking
 */
class TC_GAME_API HealingTargetSelector
{
public:
    /**
     * @brief Select best healing target
     *
     * @param healer Healer bot
     * @param range Max healing range (default: 40 yards)
     * @param minHealthPercent Only consider targets below this % (default: 100)
     * @return Best target to heal or nullptr if no valid target
     *
     * Example:
     * - Tank at 60% HP: score = 40 × 2.0 × 1.0 = 80
     * - DPS at 30% HP: score = 70 × 1.0 × 1.0 = 70
     * - Result: Tank healed first (higher priority despite higher health)
     */
    static Player* SelectTarget(
        Player* healer,
        float range = 40.0f,
        float minHealthPercent = 100.0f
    );

    /**
     * @brief Get all injured allies sorted by priority
     *
     * @param healer Healer bot
     * @param range Max healing range (default: 40 yards)
     * @param minHealthPercent Only consider targets below this % (default: 100)
     * @return Vector of targets sorted by priority (highest first)
     *
     * Use case: Multi-target healing (e.g., Chain Heal, Beacon prioritization)
     */
    static std::vector<TargetPriority> GetInjuredAllies(
        Player* healer,
        float range = 40.0f,
        float minHealthPercent = 100.0f
    );

    /**
     * @brief Check if target needs dispel
     *
     * @param target Target to check
     * @param dispelType Dispel type (magic, curse, disease, poison)
     * @return True if target has dispellable debuffs of that type
     *
     * Use case: Smart dispel priority (e.g., dispel before healing)
     */
    static bool NeedsDispel(Player* target, DispelType type);

    /**
     * @brief Get targets needing dispel
     *
     * @param healer Healer bot
     * @param dispelType Dispel type to check
     * @param range Max range (default: 40 yards)
     * @return Targets needing dispel sorted by priority
     *
     * Priority: Tank > Healer > DPS, lower health = higher priority
     */
    static std::vector<Player*> GetTargetsNeedingDispel(
        Player* healer,
        DispelType type,
        float range = 40.0f
    );

    /**
     * @brief Select AoE healing position
     *
     * @param healer Healer bot
     * @param minTargets Minimum injured targets to consider (default: 3)
     * @param range AoE spell range (default: 30 yards)
     * @return Best position for AoE heal or nullptr if insufficient targets
     *
     * Use case: Healing Rain, Efflorescence, Holy Word: Sanctify placement
     * Algorithm: Find cluster with most injured allies
     */
    static Unit* SelectAoEHealingTarget(
        Player* healer,
        uint32 minTargets = 3,
        float range = 30.0f
    );

    /**
     * @brief Check if healing is needed
     *
     * @param healer Healer bot
     * @param urgencyThreshold Consider targets below this % (default: 95)
     * @return True if any ally needs healing
     *
     * Use case: Quick check before entering healing rotation
     */
    static bool IsHealingNeeded(
        Player* healer,
        float urgencyThreshold = 95.0f
    );

    /**
     * @brief Get target's incoming heal amount
     *
     * @param target Target to check
     * @return Estimated incoming healing amount
     *
     * Use case: Avoid overhealing, coordinate with other healers
     * Note: Estimates based on active HoTs and pending direct heals
     */
    static float GetIncomingHealAmount(Player* target);

    /**
     * @brief Predict target's health in N seconds
     *
     * @param target Target to predict
     * @param seconds Time to predict (default: 3 seconds)
     * @return Predicted health percentage
     *
     * Use case: Proactive healing (start casting before damage lands)
     * Algorithm: Current HP + incoming heals - incoming damage
     */
    static float PredictHealthInSeconds(Player* target, float seconds = 3.0f);

private:
    /**
     * @brief Calculate role priority for target
     *
     * @param target Target to evaluate
     * @return Priority multiplier (Tank: 2.0, Healer: 1.5, DPS: 1.0)
     */
    static float CalculateRolePriority(Player* target);

    /**
     * @brief Calculate distance factor
     *
     * @param healer Healer position
     * @param target Target position
     * @param maxRange Maximum healing range
     * @return Distance factor 0.0-1.0 (closer = higher)
     */
    static float CalculateDistanceFactor(Player* healer, Player* target, float maxRange);

    /**
     * @brief Check if target has incoming heals
     *
     * @param target Target to check
     * @return True if other healers are casting on this target
     */
    static bool HasIncomingHeals(Player* target);

    /**
     * @brief Count dispellable debuffs
     *
     * @param target Target to check
     * @param dispelType Type to count (or all types if unspecified)
     * @return Number of dispellable debuffs
     */
    static uint32 CountDispellableDebuffs(Player* target, DispelType type);

    /**
     * @brief Calculate threat factor
     *
     * @param target Target to evaluate
     * @return Threat factor 0.0-1.0 (higher = more threat)
     */
    static float CalculateThreatFactor(Player* target);

    /**
     * @brief Check if target is main tank
     *
     * @param target Target to check
     * @return True if designated as main tank
     */
    static bool IsMainTank(Player* target);

    /**
     * @brief Get all group members in range
     *
     * @param healer Healer bot
     * @param range Maximum range
     * @return Vector of group members in range
     */
    static std::vector<Player*> GetGroupMembersInRange(Player* healer, float range);

    /**
     * @brief Calculate AoE healing score for position
     *
     * @param position Position to evaluate
     * @param healer Healer bot
     * @param range AoE range
     * @return Number of injured allies × average health deficit
     */
    static float CalculateAoEHealingScore(Unit* position, Player* healer, float range);
};

}} // namespace bot::ai
} // namespace Playerbot

#endif
