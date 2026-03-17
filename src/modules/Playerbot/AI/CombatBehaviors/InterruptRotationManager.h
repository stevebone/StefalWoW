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
#include <functional>

class Player;
class Unit;
class Spell;

namespace Playerbot
{

class BotAI;

/**
 * @class InterruptRotationManager
 * @brief Manages interrupt coordination and rotation among group bots
 *
 * This manager implements intelligent interrupt assignment based on:
 * - Spell priority (mandatory heals/CC vs optional damage)
 * - Interrupt availability and cooldowns
 * - Range and positioning requirements
 * - Rotation fairness to distribute interrupt responsibilities
 * - Fallback strategies when primary interrupter unavailable
 *
 * Performance: <0.01ms per spell cast evaluation
 */
class TC_GAME_API InterruptRotationManager
{
public:
    explicit InterruptRotationManager(BotAI* ai);
    ~InterruptRotationManager();

    // ========================================================================
    // CORE UPDATE
    // ========================================================================

    /**
     * Update interrupt tracking and cooldowns
     * @param diff Time since last update in milliseconds
     */
    void Update(uint32 diff);

    /**
     * Register a spell cast that may need interrupting
     * @param caster Unit casting the spell
     * @param spellId Spell being cast
     * @param castTime Time until cast completes (ms)
     */
    void RegisterCast(Unit* caster, uint32 spellId, uint32 castTime = 0);

    /**
     * Select which bot should interrupt a cast
     * @param caster Unit casting the spell
     * @param spellId Spell being cast
     * @return GUID of bot who should interrupt, empty if none
     */
    ObjectGuid SelectInterrupter(Unit* caster, uint32 spellId);

    // ========================================================================
    // INTERRUPT PRIORITY
    // ========================================================================

    enum class InterruptPriority : uint8_t
    {
        PRIORITY_MANDATORY = 5,     // Must interrupt or wipe (Heal, Fear, MC)
        PRIORITY_HIGH = 4,          // High damage or dangerous (Pyroblast, Chaos Bolt)
        PRIORITY_MEDIUM = 3,        // Moderate impact (Frostbolt, Shadow Bolt)
        PRIORITY_LOW = 2,           // Minor impact (Wrath, Lightning Bolt)
        PRIORITY_OPTIONAL = 1       // Nice to have (Buffs, Minor Heals)
    };

    struct InterruptableSpell
    {
        uint32 spellId = 0;
        InterruptPriority priority = InterruptPriority::PRIORITY_OPTIONAL;
        uint32 castTimeMs = 0;
        bool isChanneled = false;
        bool isAOE = false;
        float dangerRadius = 0.0f;         // Danger zone for AOE
        uint32 estimatedDamage = 0;
        bool causesCC = false;             // Causes crowd control
        bool isHeal = false;
        float interruptWindowMs = 0.0f;    // Time window to interrupt
    };

    /**
     * Register an interruptable spell to the database
     * @param spell Spell data to register
     */
    void RegisterInterruptableSpell(const InterruptableSpell& spell);

    /**
     * Get interrupt priority for a spell
     * @param spellId Spell to check
     * @return Priority level
     */
    InterruptPriority GetSpellPriority(uint32 spellId) const;

    /**
     * Check if spell should be interrupted
     * @param spellId Spell to check
     * @return true if spell is worth interrupting
     */
    bool ShouldInterrupt(uint32 spellId) const;

    // ========================================================================
    // INTERRUPTER MANAGEMENT
    // ========================================================================

    struct InterrupterBot
    {
        ObjectGuid botGuid;
        uint32 interruptSpellId = 0;       // Primary interrupt spell
        uint32 cooldownRemaining = 0;       // milliseconds
        uint32 range = 0;                  // yards
        bool isInRange = false;
        uint32 lastInterruptTime = 0;
        uint32 interruptsPerformed = 0;
        bool isAssigned = false;            // Currently assigned to interrupt

        // Additional interrupt abilities (stuns, silences)
        ::std::vector<uint32> alternativeInterrupts;
    };

    /**
     * Register a bot as capable of interrupting
     * @param bot Bot to register
     * @param interruptSpellId Primary interrupt spell
     * @param range Range of interrupt ability
     */
    void RegisterInterrupter(ObjectGuid bot, uint32 interruptSpellId, uint32 range);

    /**
     * Update interrupter availability
     * @param bot Bot to update
     * @param available Whether bot can currently interrupt
     * @param cooldownMs Remaining cooldown in milliseconds
     */
    void UpdateInterrupterStatus(ObjectGuid bot, bool available, uint32 cooldownMs = 0);

    /**
     * Get next bot in interrupt rotation
     * @return GUID of next interrupter
     */
    ObjectGuid GetNextInRotation() const;

    /**
     * Mark interrupt as used by a bot
     * @param bot Bot that interrupted
     * @param timeMs When interrupt was used
     */
    void MarkInterruptUsed(ObjectGuid bot, uint32 timeMs = 0);

    // ========================================================================
    // ACTIVE CASTS TRACKING
    // ========================================================================

    struct ActiveCast
    {
        ObjectGuid casterGuid;
        uint32 spellId = 0;
        uint32 castStartTime = 0;
        uint32 castEndTime = 0;
        InterruptPriority priority = InterruptPriority::PRIORITY_OPTIONAL;
        ObjectGuid assignedInterrupter;
        bool interrupted = false;
        bool fallbackTriggered = false;
    };

    /**
     * Get all active casts being tracked
     * @return Vector of active casts
     */
    const ::std::vector<ActiveCast>& GetActiveCasts() const { return _activeCasts; }

    /**
     * Check if we're tracking a specific cast
     * @param caster Caster to check
     * @param spellId Spell to check (0 for any spell)
     * @return true if cast is being tracked
     */
    bool IsTrackingCast(ObjectGuid caster, uint32 spellId = 0) const;

    // ========================================================================
    // FALLBACK STRATEGIES
    // ========================================================================

    enum class FallbackMethod : uint8_t
    {
        FALLBACK_NONE = 0,
        FALLBACK_STUN = 1,          // Use stun instead
        FALLBACK_SILENCE = 2,       // Use silence (longer CD)
        FALLBACK_LOS = 3,           // Line of sight
        FALLBACK_RANGE = 4,         // Run out of range
        FALLBACK_DEFENSIVE = 5      // Use defensive CD
    };

    /**
     * Handle failed interrupt with fallback strategy
     * @param caster Unit casting the spell
     * @param spellId Spell being cast
     */
    void HandleFailedInterrupt(Unit* caster, uint32 spellId);

    /**
     * Determine best fallback method for a spell
     * @param spellId Spell to handle
     * @return Best fallback method
     */
    FallbackMethod SelectFallbackMethod(uint32 spellId) const;

    /**
     * Execute fallback strategy
     * @param method Fallback method to use
     * @param caster Target caster
     * @return true if fallback succeeded
     */
    bool ExecuteFallback(FallbackMethod method, Unit* caster);

    // ========================================================================
    // ROTATION COORDINATION
    // ========================================================================

    /**
     * Schedule delayed interrupt for when cooldown available
     * @param bot Bot to schedule
     * @param target Target to interrupt
     * @param spellId Spell to interrupt
     * @param delayMs Delay in milliseconds
     */
    void ScheduleDelayedInterrupt(ObjectGuid bot, ObjectGuid target,
                                  uint32 spellId, uint32 delayMs);

    struct DelayedInterrupt
    {
        ObjectGuid interrupter;
        ObjectGuid target;
        uint32 spellId = 0;
        uint32 executeTime = 0;
    };

    /**
     * Process scheduled interrupts
     */
    void ProcessDelayedInterrupts();

    /**
     * Coordinate interrupts across group
     * @param casters List of enemy casters
     */
    void CoordinateGroupInterrupts(const ::std::vector<Unit*>& casters);

    // ========================================================================
    // STATISTICS AND LEARNING
    // ========================================================================

    struct InterruptStatistics
    {
        uint32 totalCastsDetected = 0;
        uint32 successfulInterrupts = 0;
        uint32 failedInterrupts = 0;
        uint32 fallbacksUsed = 0;
        ::std::unordered_map<uint32, uint32> interruptsBySpell;
        ::std::unordered_map<ObjectGuid, uint32> interruptsByBot;
    };

    const InterruptStatistics& GetStatistics() const { return _statistics; }

    /**
     * Learn from interrupt success/failure
     * @param spellId Spell that was interrupted (or not)
     * @param success Whether interrupt succeeded
     */
    void RecordInterruptAttempt(uint32 spellId, bool success);

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    struct Configuration
    {
        uint32 reactionTimeMs = 200;           // Human-like reaction delay
        uint32 coordinationDelayMs = 100;      // Delay between group interrupts
        float interruptRangeBuffer = 2.0f;     // Safety buffer for range checks
        bool preferMeleeInterrupts = true;     // Prefer melee interrupts (no travel time)
        bool useRotation = true;                // Use rotation vs first available
        uint32 maxInterruptsPerMinute = 20;    // Throttle to avoid looking robotic
    };

    void SetConfiguration(const Configuration& config) { _config = config; }
    const Configuration& GetConfiguration() const { return _config; }

    // ========================================================================
    // UTILITY
    // ========================================================================

    /**
     * Initialize default interrupt database
     */
    static void InitializeGlobalDatabase();

    /**
     * Get interrupt spells for a specific class
     * @param classId Class to check
     * @return Vector of interrupt spell IDs
     */
    static ::std::vector<uint32> GetClassInterrupts(uint8 classId);

    /**
     * Calculate time until spell completes
     * @param cast Active cast to check
     * @return Milliseconds until completion
     */
    uint32 GetTimeToComplete(const ActiveCast& cast) const;

    /**
     * Clear all tracking data
     */
    void Reset();

private:
    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /**
     * Clean up expired casts and assignments
     */
    void CleanupExpiredData();

    /**
     * Find best interrupter for a cast
     * @param cast Cast needing interrupt
     * @return Best interrupter GUID
     */
    ObjectGuid FindBestInterrupter(const ActiveCast& cast) const;

    /**
     * Calculate interrupter score for assignment
     * @param interrupter Bot to score
     * @param cast Cast to interrupt
     * @return Score (higher is better)
     */
    float CalculateInterrupterScore(const InterrupterBot& interrupter,
                                    const ActiveCast& cast) const;

    /**
     * Check if bot can reach target in time
     * @param bot Interrupter bot
     * @param target Cast target
     * @param timeAvailable Time until cast completes
     * @return true if can interrupt in time
     */
    bool CanReachInTime(const InterrupterBot& bot, Unit* target,
                        uint32 timeAvailable) const;

    /**
     * Update range status for all interrupters
     * @param target Target being cast on
     */
    void UpdateRangeStatus(Unit* target);

    /**
     * Try alternative interrupt method (stun/silence)
     * @param target Target to interrupt
     * @return true if alternative succeeded
     */
    bool TryAlternativeInterrupt(Unit* target);

private:
    // Core components
    BotAI* _ai;
    Player* _bot;

    // Interrupt database (static, shared across all managers)
    static inline ::std::unordered_map<uint32, InterruptableSpell> s_interruptDatabase;
    static inline bool s_databaseInitialized = false;

    // Interrupter tracking
    ::std::vector<InterrupterBot> _interrupters;
    ::std::queue<ObjectGuid> _rotationQueue;

    // Active cast tracking
    ::std::vector<ActiveCast> _activeCasts;

    // Delayed interrupt scheduling
    ::std::vector<DelayedInterrupt> _delayedInterrupts;

    // Statistics
    InterruptStatistics _statistics;

    // Configuration
    Configuration _config;

    // Performance tracking
    uint32 _lastCleanupTime;
    uint32 _lastUpdateTime;

    // Cache for performance
    mutable ::std::unordered_map<ObjectGuid, Unit*> _unitCache;
    mutable uint32 _unitCacheTime;
    static constexpr uint32 UNIT_CACHE_DURATION = 100; // ms
};

} // namespace Playerbot