/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "SpellInfo.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <queue>
#include <chrono>

class Player;
class Unit;
class Aura;

namespace Playerbot
{

class BotAI;

/**
 * @class DefensiveBehaviorManager
 * @brief Manages defensive cooldown usage and survival behaviors for bots
 *
 * This manager implements intelligent defensive cooldown usage based on:
 * - Current health percentage and incoming damage
 * - Role-specific thresholds (tank/healer/DPS)
 * - Cooldown prioritization and tier system
 * - Group-wide defensive coordination
 *
 * Performance: <0.02ms per update per bot
 */
class TC_GAME_API DefensiveBehaviorManager
{
public:
    explicit DefensiveBehaviorManager(BotAI* ai);
    ~DefensiveBehaviorManager();

    // ========================================================================
    // CORE UPDATE
    // ========================================================================

    /**
     * Update defensive behavior evaluation
     * @param diff Time since last update in milliseconds
     */
    void Update(uint32 diff);

    /**
     * Check if bot needs to use defensive cooldowns
     * @return true if defensive action is needed
     */
    bool NeedsDefensive() const;

    /**
     * Select the best defensive cooldown to use
     * @return Spell ID of defensive to use, 0 if none
     */
    uint32 SelectDefensive() const;

    // ========================================================================
    // DEFENSIVE STATE
    // ========================================================================

    enum class DefensivePriority : uint8_t
    {
        PRIORITY_CRITICAL = 5,    // Death imminent (<20% HP)
        PRIORITY_MAJOR = 4,       // Heavy damage (20-40% HP)
        PRIORITY_MODERATE = 3,    // Sustained damage (40-60% HP)
        PRIORITY_MINOR = 2,       // Light damage (60-80% HP)
        PRIORITY_PREEMPTIVE = 1   // No immediate danger (>80% HP)
    };

    struct DefensiveState
    {
        float healthPercent = 100.0f;
        float incomingDPS = 0.0f;       // Damage per second over last 3 seconds
        float predictedHealth = 100.0f;  // Health in 2 seconds based on current DPS
        uint32 debuffCount = 0;          // Number of harmful debuffs
        bool hasMajorDebuff = false;     // Stun, Fear, Polymorph, etc.
        uint32 nearbyEnemies = 0;        // Enemies within 10 yards
        bool tankDead = false;           // Group tank status
        bool healerOOM = false;          // Group healer mana < 20%
        uint32 lastUpdateTime = 0;
    };

    const DefensiveState& GetState() const { return _currentState; }
    DefensivePriority GetCurrentPriority() const;

    // ========================================================================
    // DAMAGE TRACKING
    // ========================================================================

    /**
     * Register damage taken for DPS calculation
     * @param damage Amount of damage taken
     * @param timestamp When damage was taken (default: now)
     */
    void RegisterDamage(uint32 damage, uint32 timestamp = 0);

    /**
     * Prepare for incoming damage spike
     * @param spellId Incoming spell that will deal damage
     */
    void PrepareForIncoming(uint32 spellId);

    /**
     * Get current incoming DPS
     * @return Damage per second over recent history
     */
    float GetIncomingDPS() const;

    /**
     * Predict health in future based on current damage rate
     * @param secondsAhead How many seconds to look ahead
     * @return Predicted health percentage
     */
    float PredictHealth(float secondsAhead) const;

    // ========================================================================
    // COOLDOWN MANAGEMENT
    // ========================================================================

    enum class DefensiveSpellTier : uint8_t
    {
        TIER_IMMUNITY = 5,           // Complete immunity (Divine Shield, Ice Block)
        TIER_MAJOR_REDUCTION = 4,    // 50%+ damage reduction (Shield Wall)
        TIER_MODERATE_REDUCTION = 3, // 20-50% reduction (Barkskin)
        TIER_AVOIDANCE = 2,          // Dodge/Parry/Block increase (Evasion)
        TIER_REGENERATION = 1        // Self-healing (Frenzied Regeneration)
    };

    struct DefensiveCooldown
    {
        uint32 spellId = 0;
        DefensiveSpellTier tier = DefensiveSpellTier::TIER_REGENERATION;
        float minHealthPercent = 0.0f;      // Don't use above this HP
        float maxHealthPercent = 100.0f;    // Don't use below this HP (save for emergency)
        uint32 cooldownMs = 0;              // Cooldown duration
        uint32 durationMs = 0;              // Buff duration
        bool requiresGCD = true;            // Uses global cooldown
        bool breakOnDamage = false;         // Broken by damage

        // Situational requirements
        bool requiresMelee = false;         // Only vs melee
        bool requiresMagic = false;         // Only vs casters
        bool requiresMultipleEnemies = false; // AOE situations
        uint32 minEnemyCount = 0;          // Minimum enemies for use

        // Usage tracking
        uint32 lastUsedTime = 0;
        uint32 usageCount = 0;
    };

    /**
     * Register a defensive cooldown spell
     * @param cooldown Defensive cooldown data
     */
    void RegisterDefensiveCooldown(const DefensiveCooldown& cooldown);

    /**
     * Check if a defensive cooldown is available
     * @param spellId Spell ID to check
     * @return true if available for use
     */
    bool IsDefensiveAvailable(uint32 spellId) const;

    /**
     * Mark a defensive as used
     * @param spellId Spell that was used
     */
    void MarkDefensiveUsed(uint32 spellId);

    // ========================================================================
    // EXTERNAL DEFENSIVE COORDINATION
    // ========================================================================

    struct ExternalDefensiveRequest
    {
        ObjectGuid targetGuid;
        float healthPercent = 0.0f;
        float incomingDPS = 0.0f;
        DefensivePriority priority = DefensivePriority::PRIORITY_MINOR;
        uint32 requestTime = 0;
        bool fulfilled = false;
    };

    /**
     * Request external defensive from group
     * @param target Who needs the defensive
     * @param priority How urgent the need is
     */
    void RequestExternalDefensive(ObjectGuid target, DefensivePriority priority);

    /**
     * Check if we should provide external defensive to ally
     * @return Target GUID if we should help, empty otherwise
     */
    ObjectGuid GetExternalDefensiveTarget() const;

    /**
     * Coordinate external defensives across group
     */
    void CoordinateExternalDefensives();

    // ========================================================================
    // ROLE-SPECIFIC THRESHOLDS
    // ========================================================================

    struct RoleThresholds
    {
        float criticalHP = 0.20f;        // Use everything threshold
        float majorCooldownHP = 0.40f;   // Major cooldown threshold
        float minorCooldownHP = 0.60f;   // Minor cooldown threshold
        float preemptiveHP = 0.80f;      // Maintain buffs threshold
        float incomingDPSThreshold = 0.30f; // % of max HP per second
        uint32 fleeEnemyCount = 5;       // Run away threshold
    };

    /**
     * Get role-specific thresholds
     * @param role Bot's combat role
     * @return Appropriate thresholds
     */
    static RoleThresholds GetRoleThresholds(uint8 role);

    // ========================================================================
    // CONSUMABLES
    // ========================================================================

    /**
     * Check if should use health potion
     * @return true if potion should be used
     */
    bool ShouldUseHealthPotion() const;

    /**
     * Check if should use healthstone
     * @return true if healthstone should be used
     */
    bool ShouldUseHealthstone() const;

    /**
     * Check if should use bandage (out of combat)
     * @return true if bandage should be used
     */
    bool ShouldUseBandage() const;

    // ========================================================================
    // UTILITY
    // ========================================================================

    /**
     * Get defensive cooldowns for a specific class
     * @param classId Class to get defensives for
     * @return Vector of defensive cooldowns
     */
    static ::std::vector<DefensiveCooldown> GetClassDefensives(uint8 classId);

    /**
     * Initialize default defensive cooldowns based on bot's class
     */
    void InitializeClassDefensives();

    /**
     * Clear all defensive cooldowns and state
     */
    void Reset();

    /**
     * Get performance metrics
     */
    struct PerformanceMetrics
    {
        uint32 updatesPerformed = 0;
        uint32 defensivesUsed = 0;
        uint32 externalDefensivesProvided = 0;
        ::std::chrono::microseconds averageUpdateTime{0};
        ::std::chrono::microseconds maxUpdateTime{0};
    };

    const PerformanceMetrics& GetMetrics() const { return _metrics; }

private:
    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /**
     * Update current defensive state
     */
    void UpdateState();

    /**
     * Evaluate defensive priority based on current state
     */
    DefensivePriority EvaluatePriority() const;

    /**
     * Select best defensive from available cooldowns
     * @param priority Current defensive priority
     * @return Best defensive spell ID, 0 if none
     */
    uint32 SelectBestDefensive(DefensivePriority priority) const;

    /**
     * Check if defensive meets situational requirements
     * @param cooldown Defensive to check
     * @return true if all requirements met
     */
    bool MeetsRequirements(const DefensiveCooldown& cooldown) const;

    /**
     * Calculate defensive score for prioritization
     * @param cooldown Defensive to score
     * @param priority Current priority level
     * @return Score (higher is better)
     */
    float CalculateDefensiveScore(const DefensiveCooldown& cooldown,
                                  DefensivePriority priority) const;

    /**
     * Count enemies in range
     * @param range Check radius in yards
     * @return Number of hostile units
     */
    uint32 CountNearbyEnemies(float range) const;

    /**
     * Check if current damage is magical
     * @return true if majority of recent damage was magical
     */
    bool IsDamageMostlyMagical() const;

    /**
     * Check if current damage is physical
     * @return true if majority of recent damage was physical
     */
    bool IsDamageMostlyPhysical() const;

    /**
     * Track performance metrics
     * @param startTime When update started
     */
    void UpdateMetrics(::std::chrono::steady_clock::time_point startTime);

private:
    // Core components
    BotAI* _ai;
    Player* _bot;

    // Current state
    DefensiveState _currentState;
    mutable DefensivePriority _cachedPriority;
    mutable uint32 _priorityCacheTime;

    // Damage tracking (circular buffer for performance)
    struct DamageEntry
    {
        uint32 damage;
        uint32 timestamp;
        bool isMagical;
    };
    ::std::vector<DamageEntry> _damageHistory;
    size_t _damageHistoryIndex;
    static constexpr size_t DAMAGE_HISTORY_SIZE = 30; // 3 seconds at 100ms updates

    // Defensive cooldowns
    ::std::unordered_map<uint32, DefensiveCooldown> _defensiveCooldowns;
    mutable ::std::vector<uint32> _sortedDefensives; // Cache for performance
    mutable uint32 _sortedDefensivesTime;

    // External defensive coordination
    ::std::vector<ExternalDefensiveRequest> _externalRequests;
    ::std::unordered_map<ObjectGuid, uint32> _providedDefensives; // Target -> last time

    // Role-specific thresholds
    RoleThresholds _thresholds;

    // Performance metrics
    mutable PerformanceMetrics _metrics;

    // Cache durations (ms)
    static constexpr uint32 PRIORITY_CACHE_DURATION = 100;
    static constexpr uint32 SORTED_DEFENSIVES_CACHE_DURATION = 500;
    static constexpr uint32 STATE_UPDATE_INTERVAL = 100;
};

} // namespace Playerbot