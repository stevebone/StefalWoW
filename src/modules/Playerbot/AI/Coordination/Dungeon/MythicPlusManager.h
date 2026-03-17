/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "DungeonState.h"
#include "Define.h"
#include "ObjectGuid.h"
#include <vector>
#include <set>
#include <map>

namespace Playerbot {

class DungeonCoordinator;

/**
 * @enum MythicPlusAffix
 * @brief Mythic+ affix identifiers
 */
enum class MythicPlusAffix : uint32
{
    NONE = 0,

    // Level 2+ (Base affixes)
    FORTIFIED = 10,     // Trash has more health and damage
    TYRANNICAL = 9,     // Bosses have more health and damage

    // Level 4+
    BOLSTERING = 7,     // Dying enemies buff nearby allies
    RAGING = 6,         // Enemies enrage at 30% health
    SANGUINE = 8,       // Dying enemies leave healing pools
    BURSTING = 11,      // Dying enemies apply stacking DoT

    // Level 7+
    NECROTIC = 4,       // Attacks apply healing reduction
    VOLCANIC = 3,       // Spawn volcanic pools
    EXPLOSIVE = 13,     // Spawn explosive orbs
    QUAKING = 14,       // Periodic AoE damage
    GRIEVOUS = 12,      // Wounded players take DoT
    STORMING = 124,     // Spawn tornados

    // Level 10+
    INSPIRING = 122,    // Certain enemies buff allies
    SPITEFUL = 123,     // Dying enemies spawn Shades

    // Seasonal
    AWAKENED = 120,
    PRIDEFUL = 121,
    TORMENTED = 128,
    ENCRYPTED = 130,
    SHROUDED = 131,
    THUNDERING = 132,
    AFFLICTED = 135,
    INCORPOREAL = 136
};

/**
 * @struct KeystoneInfo
 * @brief Information about the active keystone
 */
struct KeystoneInfo
{
    uint32 dungeonId = 0;
    uint8 level = 0;
    ::std::vector<MythicPlusAffix> affixes;
    uint32 timeLimit = 0;       // In milliseconds
    float timeLimitMod = 0.8f;  // Multiplier for 2 chest (0.6 for 3 chest)

    [[nodiscard]] bool HasAffix(MythicPlusAffix affix) const
    {
        for (auto a : affixes)
            if (a == affix) return true;
        return false;
    }

    [[nodiscard]] bool IsFortified() const { return HasAffix(MythicPlusAffix::FORTIFIED); }
    [[nodiscard]] bool IsTyrannical() const { return HasAffix(MythicPlusAffix::TYRANNICAL); }
};

/**
 * @struct EnemyForces
 * @brief Enemy forces contribution data
 */
struct EnemyForces
{
    uint32 creatureId = 0;
    float forcesValue = 0.0f;   // Percentage this mob gives
    bool isPriority = false;    // High value target

    [[nodiscard]] bool IsWorthKilling() const { return forcesValue > 0.0f; }
};

/**
 * @class MythicPlusManager
 * @brief Manages Mythic+ specific mechanics
 *
 * Responsibilities:
 * - Track timer and death penalty
 * - Monitor enemy forces percentage
 * - Handle affix-specific mechanics
 * - Optimize route for time/forces
 *
 * Affix Handling:
 * - Bolstering: Don't AoE packs, kill evenly
 * - Raging: Save interrupts/stuns for 30%
 * - Sanguine: Move mobs out of pools
 * - Bursting: Control kill rate, heal through stacks
 * - Necrotic: Tank kiting at high stacks
 * - Volcanic: Dodge pools
 * - Explosive: Kill orbs (priority)
 * - Quaking: Stop casting, spread
 *
 * Usage:
 * @code
 * MythicPlusManager manager(&coordinator);
 * manager.Initialize(keystoneInfo);
 *
 * // Check timer
 * if (!manager.IsOnTime())
 *     // Rush strategy
 *
 * // Check affixes
 * if (manager.HasAffix(MythicPlusAffix::EXPLOSIVE))
 *     if (manager.ShouldKillExplosive(orbGuid))
 *         // Kill explosive orb
 *
 * // Enemy forces
 * if (manager.HasEnoughForces())
 *     // Can skip remaining packs
 * @endcode
 */
class TC_GAME_API MythicPlusManager
{
public:
    explicit MythicPlusManager(DungeonCoordinator* coordinator);
    ~MythicPlusManager() = default;

    // ====================================================================
    // LIFECYCLE
    // ====================================================================

    /**
     * @brief Initialize with keystone
     * @param keystone Keystone information
     */
    void Initialize(const KeystoneInfo& keystone);

    /**
     * @brief Update logic
     * @param diff Time since last update
     */
    void Update(uint32 diff);

    /**
     * @brief Reset state
     */
    void Reset();

    /**
     * @brief Start the M+ timer
     */
    void StartTimer();

    /**
     * @brief Check if M+ is active
     */
    [[nodiscard]] bool IsActive() const { return _startTime > 0; }

    // ====================================================================
    // KEYSTONE INFO
    // ====================================================================

    /**
     * @brief Get keystone level
     */
    [[nodiscard]] uint8 GetKeystoneLevel() const { return _keystone.level; }

    /**
     * @brief Check if has specific affix
     * @param affix Affix to check
     */
    [[nodiscard]] bool HasAffix(MythicPlusAffix affix) const { return _keystone.HasAffix(affix); }

    /**
     * @brief Get all active affixes
     */
    [[nodiscard]] const ::std::vector<MythicPlusAffix>& GetActiveAffixes() const { return _keystone.affixes; }

    /**
     * @brief Get keystone info
     */
    [[nodiscard]] const KeystoneInfo& GetKeystoneInfo() const { return _keystone; }

    // ====================================================================
    // TIMER
    // ====================================================================

    /**
     * @brief Get time limit
     * @return Time limit in milliseconds
     */
    [[nodiscard]] uint32 GetTimeLimit() const { return _keystone.timeLimit; }

    /**
     * @brief Get elapsed time
     * @return Elapsed time in milliseconds
     */
    [[nodiscard]] uint32 GetElapsedTime() const;

    /**
     * @brief Get remaining time
     * @return Remaining time in milliseconds
     */
    [[nodiscard]] uint32 GetRemainingTime() const;

    /**
     * @brief Check if on pace for timing
     */
    [[nodiscard]] bool IsOnTime() const;

    /**
     * @brief Check if can 2-chest
     */
    [[nodiscard]] bool CanTwoChest() const;

    /**
     * @brief Check if can 3-chest
     */
    [[nodiscard]] bool CanThreeChest() const;

    /**
     * @brief Get time progress (0.0 - 1.0)
     */
    [[nodiscard]] float GetTimeProgress() const;

    /**
     * @brief Get expected completion time
     * @return Expected time in milliseconds
     */
    [[nodiscard]] uint32 GetExpectedCompletionTime() const;

    // ====================================================================
    // ENEMY FORCES
    // ====================================================================

    /**
     * @brief Get enemy forces percentage
     */
    [[nodiscard]] float GetEnemyForcesPercent() const { return _enemyForces; }

    /**
     * @brief Get required enemy forces (always 100%)
     */
    [[nodiscard]] float GetRequiredEnemyForces() const { return 100.0f; }

    /**
     * @brief Check if have enough forces
     */
    [[nodiscard]] bool HasEnoughForces() const { return _enemyForces >= 100.0f; }

    /**
     * @brief Called when enemy is killed
     * @param creatureId Creature entry ID
     */
    void OnEnemyKilled(uint32 creatureId);

    /**
     * @brief Get forces value for creature
     * @param creatureId Creature entry ID
     * @return Forces percentage value
     */
    [[nodiscard]] float GetForcesValue(uint32 creatureId) const;

    /**
     * @brief Register forces value
     * @param creatureId Creature entry ID
     * @param forces Forces data
     */
    void RegisterEnemyForces(uint32 creatureId, const EnemyForces& forces);

    // ====================================================================
    // DEATH COUNTER
    // ====================================================================

    /**
     * @brief Get death count
     */
    [[nodiscard]] uint32 GetDeathCount() const { return _deathCount; }

    /**
     * @brief Get time penalty from deaths
     * @return Penalty in milliseconds
     */
    [[nodiscard]] uint32 GetTimePenalty() const { return _deathCount * DEATH_PENALTY_MS; }

    /**
     * @brief Called when player dies
     */
    void OnPlayerDied();

    /**
     * @brief Check if death would deplete key
     */
    [[nodiscard]] bool WouldDeplete() const;

    // ====================================================================
    // ROUTE OPTIMIZATION
    // ====================================================================

    /**
     * @brief Check if should skip pack
     * @param packId Pack ID
     * @return true if can/should skip
     */
    [[nodiscard]] bool ShouldSkipPack(uint32 packId) const;

    /**
     * @brief Check if should pull extra packs
     */
    [[nodiscard]] bool ShouldPullExtra() const;

    /**
     * @brief Get optimal route
     * @return Ordered list of pack IDs
     */
    [[nodiscard]] ::std::vector<uint32> GetOptimalRoute() const;

    /**
     * @brief Calculate route progress
     */
    [[nodiscard]] float GetRouteProgress() const;

    // ====================================================================
    // AFFIX HANDLING
    // ====================================================================

    /**
     * @brief Called when affix mechanic triggers
     * @param affix Affix type
     * @param source Source of the mechanic
     */
    void OnAffixTriggered(MythicPlusAffix affix, ObjectGuid source);

    /**
     * @brief Check if should kill explosive orb
     * @param explosive Orb GUID
     * @return true if should prioritize
     */
    [[nodiscard]] bool ShouldKillExplosive(ObjectGuid explosive) const;

    /**
     * @brief Check if should avoid sanguine pool
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     * @return true if position is in pool
     */
    [[nodiscard]] bool ShouldAvoidSanguine(float x, float y, float z) const;

    /**
     * @brief Check if quaking is active
     */
    [[nodiscard]] bool IsQuakingActive() const { return _quakingActive; }

    /**
     * @brief Get volcanic positions to avoid
     */
    [[nodiscard]] const ::std::set<ObjectGuid>& GetVolcanicPositions() const { return _volcanicPools; }

    /**
     * @brief Check if spiteful shade should be kited
     */
    [[nodiscard]] bool ShouldKiteSpiteful() const { return HasAffix(MythicPlusAffix::SPITEFUL); }

    /**
     * @brief Register sanguine pool
     * @param pool Pool GUID
     */
    void AddSanguinePool(ObjectGuid pool);

    /**
     * @brief Remove sanguine pool
     * @param pool Pool GUID
     */
    void RemoveSanguinePool(ObjectGuid pool);

    /**
     * @brief Register explosive orb
     * @param orb Orb GUID
     */
    void AddExplosiveOrb(ObjectGuid orb);

    /**
     * @brief Remove explosive orb
     * @param orb Orb GUID
     */
    void RemoveExplosiveOrb(ObjectGuid orb);

    // ====================================================================
    // STRATEGY ADJUSTMENTS
    // ====================================================================

    /**
     * @brief Get damage modifier from affixes
     * @return Multiplier (1.0 = normal)
     */
    [[nodiscard]] float GetDamageModifier() const;

    /**
     * @brief Get health modifier from affixes
     * @return Multiplier (1.0 = normal)
     */
    [[nodiscard]] float GetHealthModifier() const;

    /**
     * @brief Check if should use cooldowns
     */
    [[nodiscard]] bool ShouldUseCooldowns() const;

    /**
     * @brief Check if should use bloodlust
     */
    [[nodiscard]] bool ShouldLust() const;

    /**
     * @brief Get pull size recommendation
     * @return Recommended number of packs to pull together
     */
    [[nodiscard]] uint8 GetRecommendedPullSize() const;

private:
    DungeonCoordinator* _coordinator;

    KeystoneInfo _keystone;
    uint32 _startTime = 0;
    float _enemyForces = 0.0f;
    uint32 _deathCount = 0;

    // Enemy forces data
    ::std::map<uint32, EnemyForces> _forcesTable;

    // Affix state
    bool _quakingActive = false;
    uint32 _quakingEndTime = 0;
    ::std::set<ObjectGuid> _sanguinePools;
    ::std::set<ObjectGuid> _explosiveOrbs;
    ::std::set<ObjectGuid> _volcanicPools;

    // Route optimization
    ::std::vector<uint32> _plannedRoute;
    uint32 _currentRouteIndex = 0;
    mutable bool _routeDirty = true;

    // Constants
    static constexpr uint32 DEATH_PENALTY_MS = 5000;  // 5 seconds per death
    static constexpr uint32 QUAKING_DURATION_MS = 4000;  // 4 second quaking window
    static constexpr float THREE_CHEST_TIME_MOD = 0.6f;  // 60% of time for 3 chest
    static constexpr float TWO_CHEST_TIME_MOD = 0.8f;    // 80% of time for 2 chest

    // ====================================================================
    // HELPERS
    // ====================================================================

    /**
     * @brief Load forces table for dungeon
     * @param dungeonId Dungeon map ID
     */
    void LoadForcesTable(uint32 dungeonId);

    /**
     * @brief Calculate optimal route
     */
    void CalculateOptimalRoute();

    /**
     * @brief Get affix scaling for keystone level
     * @return Scaling multiplier
     */
    [[nodiscard]] float GetAffixScaling() const;
};

/**
 * @brief Convert MythicPlusAffix to string
 */
inline const char* MythicPlusAffixToString(MythicPlusAffix affix)
{
    switch (affix)
    {
        case MythicPlusAffix::NONE:         return "None";
        case MythicPlusAffix::FORTIFIED:    return "Fortified";
        case MythicPlusAffix::TYRANNICAL:   return "Tyrannical";
        case MythicPlusAffix::BOLSTERING:   return "Bolstering";
        case MythicPlusAffix::RAGING:       return "Raging";
        case MythicPlusAffix::SANGUINE:     return "Sanguine";
        case MythicPlusAffix::BURSTING:     return "Bursting";
        case MythicPlusAffix::NECROTIC:     return "Necrotic";
        case MythicPlusAffix::VOLCANIC:     return "Volcanic";
        case MythicPlusAffix::EXPLOSIVE:    return "Explosive";
        case MythicPlusAffix::QUAKING:      return "Quaking";
        case MythicPlusAffix::GRIEVOUS:     return "Grievous";
        case MythicPlusAffix::STORMING:     return "Storming";
        case MythicPlusAffix::INSPIRING:    return "Inspiring";
        case MythicPlusAffix::SPITEFUL:     return "Spiteful";
        case MythicPlusAffix::AWAKENED:     return "Awakened";
        case MythicPlusAffix::PRIDEFUL:     return "Prideful";
        case MythicPlusAffix::TORMENTED:    return "Tormented";
        case MythicPlusAffix::ENCRYPTED:    return "Encrypted";
        case MythicPlusAffix::SHROUDED:     return "Shrouded";
        case MythicPlusAffix::THUNDERING:   return "Thundering";
        case MythicPlusAffix::AFFLICTED:    return "Afflicted";
        case MythicPlusAffix::INCORPOREAL:  return "Incorporeal";
        default:                            return "Unknown";
    }
}

} // namespace Playerbot
