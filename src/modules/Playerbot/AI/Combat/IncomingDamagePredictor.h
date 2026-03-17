/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * INCOMING DAMAGE PREDICTOR
 *
 * Provides proactive damage prediction by analyzing enemy spell casts
 * in progress, historical damage patterns, and spell data from SpellInfo.
 * Feeds into DefensiveBehaviorManager to trigger defensive CDs BEFORE
 * damage lands, not after.
 *
 * Architecture:
 *   - Per-bot component, created and owned by BotAI or CombatStateAnalyzer
 *   - Polls InterruptAwareness for detected enemy casts each update
 *   - Uses SpellInfo damage effects + caster level to estimate damage
 *   - Tracks historical DPS per enemy for baseline predictions
 *   - Provides time-bucketed forecast: damage expected in next 1/2/3/5 seconds
 *   - Thread-safe (called from bot AI update thread only)
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <vector>
#include <unordered_map>
#include <string>

class Player;
class Unit;
class SpellInfo;

namespace Playerbot
{

class InterruptAwareness;
struct DetectedSpellCast;

// ============================================================================
// DAMAGE PREDICTION STRUCTURES
// ============================================================================

/// Severity classification for incoming damage
enum class DamageSeverity : uint8
{
    NONE        = 0,    // No significant damage predicted
    LOW         = 1,    // Normal auto-attack / minor spells
    MODERATE    = 2,    // Sustained damage, manageable by healers
    HIGH        = 3,    // Heavy damage, consider minor defensive
    CRITICAL    = 4,    // Lethal damage incoming, use major defensive
    LETHAL      = 5     // Will kill bot without immunity/external CD
};

/// Predicted damage from a single source
struct PredictedDamageEvent
{
    ObjectGuid sourceGuid;          // Who is dealing the damage
    uint32 spellId = 0;            // Spell causing the damage (0 = melee)
    uint32 estimatedDamage = 0;    // Estimated raw damage amount
    uint32 timeToImpactMs = 0;     // Milliseconds until damage lands
    uint32 schoolMask = 0;         // Damage school (for resistance calc)
    bool isAoE = false;            // AoE damage (may hit others too)
    bool isAvoidable = false;      // Can be dodged/moved from
    bool isInterruptible = false;  // Can be interrupted to prevent
    float confidence = 0.0f;       // Prediction confidence (0.0-1.0)
    std::string sourceName;        // Human-readable source name
};

/// Time-bucketed damage forecast
struct DamageForecast
{
    // Predicted damage in each time window (cumulative)
    uint32 damageIn1Sec = 0;       // Total damage expected in next 1 second
    uint32 damageIn2Sec = 0;       // Total damage expected in next 2 seconds
    uint32 damageIn3Sec = 0;       // Total damage expected in next 3 seconds
    uint32 damageIn5Sec = 0;       // Total damage expected in next 5 seconds

    // As percentage of bot's max health
    float healthLossIn1Sec = 0.0f;
    float healthLossIn2Sec = 0.0f;
    float healthLossIn3Sec = 0.0f;
    float healthLossIn5Sec = 0.0f;

    // Event list (sorted by time to impact)
    std::vector<PredictedDamageEvent> events;

    // Summary
    DamageSeverity severity = DamageSeverity::NONE;
    uint32 highestSingleHit = 0;
    uint32 totalSpellCasts = 0;
    uint32 totalMeleeAttackers = 0;
    bool hasBossCast = false;       // Boss-level creature casting
    bool hasLethalCast = false;     // Single cast that would kill bot

    void Reset()
    {
        damageIn1Sec = damageIn2Sec = damageIn3Sec = damageIn5Sec = 0;
        healthLossIn1Sec = healthLossIn2Sec = healthLossIn3Sec = healthLossIn5Sec = 0.0f;
        events.clear();
        severity = DamageSeverity::NONE;
        highestSingleHit = 0;
        totalSpellCasts = 0;
        totalMeleeAttackers = 0;
        hasBossCast = false;
        hasLethalCast = false;
    }
};

/// Defensive recommendation from the predictor
struct DefensiveRecommendation
{
    bool shouldUseDefensive = false;    // Should bot pop a defensive?
    DamageSeverity severity = DamageSeverity::NONE;
    uint32 timeWindowMs = 0;           // How much time before damage
    uint32 estimatedDamage = 0;        // How much damage to mitigate
    uint32 schoolMask = 0;             // Primary damage school
    bool preferImmunity = false;       // Damage is lethal, need immunity
    bool preferMagicDefense = false;   // Primarily magic damage
    bool preferPhysicalDefense = false;// Primarily physical damage
    std::string reason;                // Human-readable reason for logging
};

// ============================================================================
// INCOMING DAMAGE PREDICTOR
// ============================================================================

class TC_GAME_API IncomingDamagePredictor
{
public:
    explicit IncomingDamagePredictor(Player* bot);
    ~IncomingDamagePredictor() = default;

    // ========================================================================
    // CORE UPDATE
    // ========================================================================

    /// Update predictions based on current combat state.
    /// @param diff Time since last update in milliseconds
    /// @param interruptAwareness Pointer to spell detection system (may be null)
    void Update(uint32 diff, InterruptAwareness const* interruptAwareness);

    /// Reset all prediction state (e.g., on combat end)
    void Reset();

    // ========================================================================
    // PREDICTION QUERIES
    // ========================================================================

    /// Get the current damage forecast
    DamageForecast const& GetForecast() const { return _forecast; }

    /// Get defensive recommendation based on current predictions
    DefensiveRecommendation GetDefensiveRecommendation() const;

    /// Get current damage severity
    DamageSeverity GetSeverity() const { return _forecast.severity; }

    /// Check if proactive defensive is warranted
    bool ShouldUsePreemptiveDefensive() const;

    /// Get the highest predicted single damage event
    PredictedDamageEvent const* GetMostDangerousEvent() const;

    /// Get predicted health percentage after incoming damage
    /// @param windowMs Time window in milliseconds
    float GetPredictedHealthPercent(uint32 windowMs = 3000) const;

    /// Get total predicted damage in a time window
    /// @param windowMs Time window in milliseconds
    uint32 GetPredictedDamage(uint32 windowMs = 3000) const;

    // ========================================================================
    // HISTORICAL TRACKING
    // ========================================================================

    /// Record actual damage taken for calibration
    /// @param sourceGuid Who dealt the damage
    /// @param damage Amount of damage
    /// @param spellId Spell that dealt damage (0 = melee)
    void RecordDamageTaken(ObjectGuid sourceGuid, uint32 damage, uint32 spellId);

    /// Get average DPS from a specific enemy
    float GetEnemyDPS(ObjectGuid enemyGuid) const;

    /// Get total incoming DPS from all sources
    float GetTotalIncomingDPS() const;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /// Set health percentage below which predictions are more aggressive
    void SetLowHealthThreshold(float pct) { _lowHealthThreshold = pct; }

    /// Set whether this bot is a tank (tanks accept more damage before defensive)
    void SetIsTank(bool isTank) { _isTank = isTank; }

    /// Set whether this bot is a healer (healers value staying alive for group)
    void SetIsHealer(bool isHealer) { _isHealer = isHealer; }

private:
    // ========================================================================
    // PREDICTION METHODS
    // ========================================================================

    /// Build predictions from detected enemy spell casts
    void PredictFromSpellCasts(InterruptAwareness const* awareness);

    /// Build predictions from melee attackers
    void PredictFromMeleeAttackers();

    /// Build predictions from historical damage patterns
    void PredictFromHistory();

    /// Estimate damage from a specific spell cast
    /// @param cast Detected spell cast info
    /// @return Estimated damage amount
    uint32 EstimateSpellDamage(DetectedSpellCast const& cast) const;

    /// Estimate damage from SpellInfo effects
    /// @param spellInfo The spell data
    /// @param casterLevel Level of the caster
    /// @return Estimated base damage
    uint32 EstimateSpellInfoDamage(SpellInfo const* spellInfo, uint32 casterLevel) const;

    /// Classify the overall damage severity based on the forecast
    DamageSeverity ClassifySeverity() const;

    /// Update historical damage tracking
    void UpdateHistoricalDPS(uint32 diff);

    /// Clean up stale historical entries
    void PruneHistory();

    // ========================================================================
    // HELPERS
    // ========================================================================

    /// Check if a spell targets the bot (or is AoE near bot)
    bool IsSpellTargetingBot(Unit* caster, SpellInfo const* spellInfo) const;

    /// Check if an enemy is in melee range of the bot
    bool IsInMeleeRange(Unit* enemy) const;

    /// Get the bot's effective health after armor/resistance mitigation
    float GetEffectiveHealth(uint32 schoolMask) const;

    /// Get approximate melee damage per swing from an attacker
    uint32 EstimateMeleeDamage(Unit* attacker) const;

    // ========================================================================
    // STATE
    // ========================================================================

    Player* _bot;

    // Current forecast
    DamageForecast _forecast;

    // Update timer
    uint32 _updateTimer = 0;
    static constexpr uint32 UPDATE_INTERVAL_MS = 200; // 5 updates per second

    // Historical damage tracking per enemy
    struct EnemyDamageHistory
    {
        ObjectGuid guid;
        uint32 totalDamage = 0;        // Total damage dealt in tracking window
        uint32 firstDamageTime = 0;    // When tracking started
        uint32 lastDamageTime = 0;     // Most recent damage
        uint32 hitCount = 0;           // Number of damage events
        uint32 lastSpellId = 0;        // Last spell used
        float dps = 0.0f;             // Calculated DPS
    };

    std::unordered_map<ObjectGuid, EnemyDamageHistory> _enemyHistory;
    static constexpr uint32 HISTORY_WINDOW_MS = 10000;  // 10-second DPS window
    static constexpr uint32 HISTORY_PRUNE_INTERVAL_MS = 5000;
    uint32 _pruneTimer = 0;

    // Configuration
    float _lowHealthThreshold = 40.0f;
    bool _isTank = false;
    bool _isHealer = false;

    // Cached total incoming DPS
    float _totalIncomingDPS = 0.0f;
    uint32 _dpsCalcTimer = 0;
    static constexpr uint32 DPS_CALC_INTERVAL_MS = 1000;
};

} // namespace Playerbot
