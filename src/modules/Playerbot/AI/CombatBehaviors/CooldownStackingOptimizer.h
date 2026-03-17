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
#include <memory>
#include <vector>
#include <unordered_map>
#include <array>
#include <chrono>
#include <queue>
#include <set>

class Player;
class Unit;
class SpellInfo;
class Aura;

namespace Playerbot
{

class BotAI;

/**
 * @class CooldownStackingOptimizer
 * @brief Optimizes major cooldown usage for maximum damage output
 *
 * This optimizer implements intelligent cooldown management including:
 * - Boss phase detection and phase-specific cooldown reservation
 * - Cooldown stacking window calculation for burst phases
 * - Bloodlust/Heroism alignment and optimization
 * - Diminishing returns calculation for stacked buffs
 * - Role-specific cooldown priorities and usage patterns
 *
 * Performance: <0.02ms per update per bot
 */
class TC_GAME_API CooldownStackingOptimizer
{
public:
    explicit CooldownStackingOptimizer(BotAI* ai);
    ~CooldownStackingOptimizer();

    // ========================================================================
    // CORE UPDATE
    // ========================================================================

    /**
     * Update cooldown optimization state
     * @param diff Time since last update in milliseconds
     */
    void Update(uint32 diff);

    // ========================================================================
    // BOSS PHASES
    // ========================================================================

    enum BossPhase : uint8
    {
        NORMAL = 0,      // Standard rotation phase
        BURN = 1,        // High damage burn phase
        DEFENSIVE = 2,   // Defensive/survival phase
        ADD = 3,         // Add/minion phase
        TRANSITION = 4,  // Phase transition (save cooldowns)
        EXECUTE = 5      // Execute phase (<20% health)
    };

    /**
     * Detect current boss phase based on fight state
     * @param boss Boss unit to analyze
     * @return Current boss phase
     */
    BossPhase DetectBossPhase(Unit* boss) const;

    /**
     * Get current active boss phase
     * @return Current phase
     */
    BossPhase GetCurrentPhase() const { return _currentPhase; }

    // ========================================================================
    // COOLDOWN CATEGORIES
    // ========================================================================

    enum CooldownCategory : uint8
    {
        MAJOR_DPS = 0,    // Major damage cooldowns (3min+)
        MINOR_DPS = 1,    // Minor damage cooldowns (1-2min)
        BURST = 2,        // Short burst cooldowns (<1min)
        DEFENSIVE_CD = 3, // Defensive cooldowns (renamed to avoid conflict)
        UTILITY = 4,      // Utility cooldowns
        RESOURCE = 5      // Resource generation cooldowns
    };

    // ========================================================================
    // COOLDOWN DATA
    // ========================================================================

    struct CooldownData
    {
        uint32 spellId;
        CooldownCategory category;
        uint32 cooldownMs;        // Cooldown duration in ms
        uint32 durationMs;         // Buff duration in ms
        float damageIncrease;      // Damage increase multiplier (1.0 = +100%)
        float hasteIncrease;       // Haste increase (0.3 = +30%)
        float critIncrease;        // Crit increase (0.2 = +20%)
        float masteryIncrease;     // Mastery increase (0.1 = +10%)
        float healingIncrease;     // Healing increase multiplier (0.25 = +25%)
        bool stacksWithOthers;     // Can stack with other cooldowns
        bool affectedByHaste;      // Cooldown affected by haste
        uint32 lastUsedTime;       // Last time this was used
        uint32 nextAvailable;      // Next time available

        CooldownData() : spellId(0), category(MAJOR_DPS), cooldownMs(0),
                        durationMs(0), damageIncrease(0.0f), hasteIncrease(0.0f),
                        critIncrease(0.0f), masteryIncrease(0.0f), healingIncrease(0.0f),
                        stacksWithOthers(true), affectedByHaste(false),
                        lastUsedTime(0), nextAvailable(0) {}
    };

    /**
     * Register a cooldown for tracking
     * @param data Cooldown data to register
     */
    void RegisterCooldown(CooldownData const& data);

    /**
     * Update cooldown availability
     * @param spellId Spell that was just used
     */
    void UpdateCooldownUsed(uint32 spellId);

    // ========================================================================
    // STACKING WINDOWS
    // ========================================================================

    struct StackWindow
    {
        uint32 startTime;          // Window start time
        uint32 duration;           // Window duration
        ::std::vector<uint32> cooldowns;  // Cooldowns to use
        float totalMultiplier;     // Combined damage multiplier
        float score;               // Window score for optimization

        StackWindow() : startTime(0), duration(0), totalMultiplier(1.0f), score(0.0f) {}
    };

    /**
     * Find optimal cooldown stacking window
     * @param lookAheadMs How far ahead to look (ms)
     * @return Optimal stacking window
     */
    StackWindow FindOptimalStackWindow(uint32 lookAheadMs = 30000) const;

    /**
     * Calculate stacked multiplier with diminishing returns
     * @param cooldowns List of cooldown spell IDs
     * @return Combined multiplier with DR applied
     */
    float CalculateStackedMultiplier(::std::vector<uint32> const& cooldowns) const;

    // ========================================================================
    // COOLDOWN DECISIONS
    // ========================================================================

    /**
     * Check if major cooldown should be used
     * @param target Current target
     * @return True if cooldown should be used
     */
    bool ShouldUseMajorCooldown(Unit* target) const;

    /**
     * Check if cooldown should be used in current phase
     * @param spellId Cooldown spell ID
     * @param phase Current boss phase
     * @return True if cooldown is appropriate for phase
     */
    bool ShouldUseCooldownInPhase(uint32 spellId, BossPhase phase) const;

    /**
     * Get priority of cooldown for current situation
     * @param spellId Cooldown spell ID
     * @return Priority score (0.0 = don't use, 1.0 = use immediately)
     */
    float GetCooldownPriority(uint32 spellId) const;

    // ========================================================================
    // PHASE RESERVATION
    // ========================================================================

    /**
     * Reserve cooldowns for upcoming phase
     * @param phase Phase to reserve for
     * @param timeUntilMs Time until phase starts (ms)
     */
    void ReserveCooldownsForPhase(BossPhase phase, uint32 timeUntilMs);

    /**
     * Check if cooldown is reserved
     * @param spellId Cooldown spell ID
     * @return True if reserved
     */
    bool IsCooldownReserved(uint32 spellId) const;

    /**
     * Clear all cooldown reservations
     */
    void ClearReservations();

    // ========================================================================
    // BLOODLUST/HEROISM
    // ========================================================================

    /**
     * Check if Bloodlust/Heroism is active
     * @return True if raid buff is active
     */
    bool IsBloodlustActive() const;

    /**
     * Predict when Bloodlust will be used
     * @return Predicted time in ms (0 if unknown)
     */
    uint32 PredictBloodlustTiming() const;

    /**
     * Check if cooldowns should align with Bloodlust
     * @param cooldownDuration Duration of the cooldown buff
     * @return True if should wait for Bloodlust
     */
    bool ShouldAlignWithBloodlust(uint32 cooldownDuration) const;

    // ========================================================================
    // OPTIMIZATION METRICS
    // ========================================================================

    /**
     * Calculate damage gain from using cooldown now
     * @param spellId Cooldown spell ID
     * @return Expected damage gain multiplier
     */
    float CalculateDamageGain(uint32 spellId) const;

    /**
     * Get time until next burn phase
     * @return Time in ms (0 if in burn phase)
     */
    uint32 GetTimeUntilBurnPhase() const;

    /**
     * Check if target will live long enough for cooldown
     * @param target Target unit
     * @param duration Cooldown duration
     * @return True if target will survive
     */
    bool WillTargetSurvive(Unit* target, uint32 duration) const;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /**
     * Set aggressive cooldown usage
     * @param aggressive Use cooldowns more liberally
     */
    void SetAggressiveUsage(bool aggressive);

    /**
     * Set phase prediction lookahead
     * @param ms Milliseconds to look ahead
     */
    void SetPhaseLookahead(uint32 ms);

    /**
     * Enable/disable Bloodlust alignment
     * @param align Align cooldowns with Bloodlust
     */
    void SetBloodlustAlignment(bool align);

private:
    // ========================================================================
    // INTERNAL HELPERS
    // ========================================================================

    /**
     * Initialize class-specific cooldowns
     */
    void InitializeClassCooldowns();

    /**
     * Update phase detection
     */
    void UpdatePhaseDetection();

    /**
     * Calculate phase score for cooldown usage
     */
    float CalculatePhaseScore(BossPhase phase) const;

    /**
     * Check for active damage increasing auras
     */
    float GetCurrentDamageModifier() const;

    /**
     * Estimate target time to die
     */
    uint32 EstimateTimeToDie(Unit* target) const;

    /**
     * Apply diminishing returns to stacked multipliers
     */
    float ApplyDiminishingReturns(float baseMultiplier, uint32 stackCount) const;

    // ========================================================================
    // MEMBER VARIABLES
    // ========================================================================

    BotAI* _ai;
    Player* _bot;

    // Cooldown tracking
    ::std::unordered_map<uint32, CooldownData> _cooldowns;
    ::std::set<uint32> _reservedCooldowns;

    // Phase tracking
    BossPhase _currentPhase;
    BossPhase _lastPhase;
    uint32 _phaseStartTime;
    uint32 _lastPhaseUpdate;

    struct PhaseReservation
    {
        BossPhase phase;
        uint32 timeUntil;
        ::std::vector<uint32> cooldowns;
    };
    ::std::vector<PhaseReservation> _phaseReservations;

    // Bloodlust tracking
    uint32 _lastBloodlustTime;
    uint32 _predictedBloodlustTime;
    bool _bloodlustUsed;

    // Configuration
    bool _aggressiveUsage;
    uint32 _phaseLookaheadMs;
    bool _alignWithBloodlust;

    // Performance metrics
    mutable uint32 _lastOptimizationCalc;
    mutable StackWindow _cachedOptimalWindow;

    // Damage tracking for phase detection
    struct DamageSnapshot
    {
        uint32 timestamp;
        float damageDealt;
        float damageTaken;
    };
    ::std::queue<DamageSnapshot> _damageHistory;
    static constexpr size_t MAX_DAMAGE_HISTORY = 20;

    // Static cooldown database
    static inline ::std::unordered_map<uint32, CooldownData> s_defaultCooldowns;
    static inline bool s_defaultsInitialized = false;

    /**
     * Initialize default cooldown database
     */
    static void InitializeDefaults();

    // Class-specific major cooldowns
    void InitializeWarriorCooldowns();
    void InitializePaladinCooldowns();
    void InitializeHunterCooldowns();
    void InitializeRogueCooldowns();
    void InitializePriestCooldowns();
    void InitializeShamanCooldowns();
    void InitializeMageCooldowns();
    void InitializeWarlockCooldowns();
    void InitializeDruidCooldowns();
    void InitializeDeathKnightCooldowns();
    void InitializeMonkCooldowns();
    void InitializeDemonHunterCooldowns();
};

} // namespace Playerbot