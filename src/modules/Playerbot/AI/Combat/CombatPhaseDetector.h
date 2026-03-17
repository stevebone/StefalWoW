/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * COMBAT PHASE DETECTOR
 *
 * Unified spec-aware combat phase detection for all 39 specializations.
 * Detects Opener, Sustained, and Execute phases with spec-specific
 * thresholds and provides rotation override hints (priority ability IDs,
 * resource behavior, cooldown usage guidance).
 *
 * Architecture:
 *   - Per-bot component, called during combat update
 *   - Reads spec data to determine phase thresholds
 *   - Tracks time-in-combat for opener window detection
 *   - Tracks target health for execute phase detection
 *   - Provides spec-specific execute spell list and opener sequences
 *   - Thread-safe (called from bot AI update thread only)
 *
 * Phase Definitions:
 *   OPENER:    First 3-8 seconds of combat (spec-dependent), use burst CDs,
 *              execute opening sequence (stealth abilities, prepull, etc.)
 *   SUSTAINED: Normal rotation between opener and execute phases
 *   EXECUTE:   Target below spec-specific health threshold (20-35%),
 *              prioritize execute abilities and burn remaining CDs
 *
 * Integration Points:
 *   - ClassAI rotation: Check GetPhase() to branch rotation logic
 *   - CombatSpecializationBase: Query ShouldUseExecuteAbility(spellId)
 *   - PreBurstResourcePooling: Check IsInOpenerWindow() for burst timing
 *   - CombatAIIntegrator: Drive CombatPhase state from this detector
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "DBCEnums.h"
#include <vector>
#include <string>

class Player;
class Unit;

namespace Playerbot
{

// ============================================================================
// COMBAT PHASE
// ============================================================================

/// Current combat phase
enum class DetectedCombatPhase : uint8
{
    OUT_OF_COMBAT   = 0,    // Not in combat
    PRE_PULL        = 1,    // About to pull (precasting/positioning)
    OPENER          = 2,    // Opening burst window (first few seconds)
    SUSTAINED       = 3,    // Normal rotation
    EXECUTE         = 4,    // Target below execute threshold
    FINISHING       = 5     // Target very low HP (<5%), all-out burn
};

/// What the rotation should prioritize
enum class PhaseGuidance : uint8
{
    NORMAL          = 0,    // No special guidance
    USE_BURST_CDS   = 1,    // Pop major offensive cooldowns
    CONSERVE        = 2,    // Pool resources for upcoming phase
    EXECUTE_SPAM    = 3,    // Prioritize execute abilities
    OPENER_SEQUENCE = 4,    // Follow class-specific opener sequence
    ALL_OUT_BURN    = 5     // Use everything, target is dying
};

// ============================================================================
// SPEC PHASE CONFIGURATION
// ============================================================================

/// Per-specialization configuration for phase detection
struct SpecPhaseConfig
{
    ChrSpecialization spec = ChrSpecialization::None;

    // Opener window
    float openerDurationSec = 6.0f;         // How long the opener phase lasts
    bool hasStealthOpener = false;           // Rogue/Feral stealth openers
    bool hasPrepull = false;                 // Prepull abilities (precasting)

    // Execute thresholds
    float executeThresholdPct = 20.0f;      // Target HP % to enter execute phase
    float finishingThresholdPct = 5.0f;     // Target HP % to enter finishing phase
    bool hasExecuteAbility = false;         // Does the spec have a dedicated execute spell?

    // Execute-phase spell IDs (spells that gain value below execute threshold)
    ::std::vector<uint32> executeSpellIds;

    // Opener-phase spell IDs (spells to prioritize in opener)
    ::std::vector<uint32> openerSpellIds;

    // Burst cooldown spell IDs to use in opener
    ::std::vector<uint32> openerBurstCDs;

    // Resource guidance
    bool poolBeforeExecute = false;         // Should pool resources before execute
    float executeResourceTarget = 0.0f;     // Target resource % at execute entry

    // Description for logging
    ::std::string specName;
};

// ============================================================================
// PHASE RECOMMENDATION
// ============================================================================

/// Current phase recommendation with details
struct PhaseRecommendation
{
    DetectedCombatPhase phase = DetectedCombatPhase::OUT_OF_COMBAT;
    PhaseGuidance guidance = PhaseGuidance::NORMAL;

    float targetHealthPct = 100.0f;         // Current target health percentage
    float timeSinceCombatStartSec = 0.0f;   // Seconds since combat began

    // Execute phase details
    bool inExecutePhase = false;
    bool isFinishing = false;
    float executeThreshold = 20.0f;         // Active execute threshold

    // Opener phase details
    bool inOpenerWindow = false;
    float openerTimeRemainingSec = 0.0f;    // Seconds left in opener window

    // Active spec-specific spells to prioritize
    ::std::vector<uint32> const* prioritySpells = nullptr;

    void Reset()
    {
        phase = DetectedCombatPhase::OUT_OF_COMBAT;
        guidance = PhaseGuidance::NORMAL;
        targetHealthPct = 100.0f;
        timeSinceCombatStartSec = 0.0f;
        inExecutePhase = false;
        isFinishing = false;
        executeThreshold = 20.0f;
        inOpenerWindow = false;
        openerTimeRemainingSec = 0.0f;
        prioritySpells = nullptr;
    }
};

// ============================================================================
// COMBAT PHASE DETECTOR
// ============================================================================

class TC_GAME_API CombatPhaseDetector
{
public:
    explicit CombatPhaseDetector(Player* bot);
    ~CombatPhaseDetector() = default;

    // ========================================================================
    // CORE UPDATE
    // ========================================================================

    /// Update phase detection. Call once per combat update cycle.
    /// @param diff Time since last update in milliseconds
    void Update(uint32 diff);

    /// Reset state on combat start
    void OnCombatStart();

    /// Reset state on combat end
    void OnCombatEnd();

    /// Initialize spec-specific configuration
    void Initialize();

    // ========================================================================
    // PHASE QUERIES (used by rotation systems)
    // ========================================================================

    /// Get the current detected combat phase
    DetectedCombatPhase GetPhase() const { return _recommendation.phase; }

    /// Get the full phase recommendation
    PhaseRecommendation const& GetRecommendation() const { return _recommendation; }

    /// Is the bot in the opening burst window?
    bool IsInOpenerWindow() const { return _recommendation.inOpenerWindow; }

    /// Is the target in execute range?
    bool IsInExecutePhase() const { return _recommendation.inExecutePhase; }

    /// Is the target about to die (<5%)?
    bool IsFinishing() const { return _recommendation.isFinishing; }

    /// Get the current rotation guidance
    PhaseGuidance GetGuidance() const { return _recommendation.guidance; }

    /// Should a specific spell be prioritized in the current phase?
    /// Returns true if the spell is in the current phase's priority list.
    bool ShouldPrioritizeSpell(uint32 spellId) const;

    /// Is a specific spell an execute-phase ability for this spec?
    bool IsExecuteAbility(uint32 spellId) const;

    /// Get the spec-specific execute threshold (health %)
    float GetExecuteThreshold() const;

    /// Get time in combat (seconds)
    float GetTimeSinceCombatStart() const { return _recommendation.timeSinceCombatStartSec; }

    /// Does this spec have stealth openers?
    bool HasStealthOpener() const { return _specConfig.hasStealthOpener; }

    /// Get the spec phase configuration
    SpecPhaseConfig const& GetSpecConfig() const { return _specConfig; }

    // ========================================================================
    // CONFIGURATION OVERRIDE
    // ========================================================================

    /// Override the execute threshold for this bot
    void SetExecuteThreshold(float pct) { _specConfig.executeThresholdPct = pct; }

    /// Override the opener duration for this bot
    void SetOpenerDuration(float seconds) { _specConfig.openerDurationSec = seconds; }

private:
    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /// Load spec-specific configuration based on bot's active spec
    void LoadSpecConfig();

    /// Detect current combat phase based on state
    void DetectPhase();

    /// Get the target we're monitoring (primary target)
    Unit* GetTarget() const;

    // ========================================================================
    // STATE
    // ========================================================================

    Player* _bot;
    bool _initialized = false;
    bool _inCombat = false;

    // Spec configuration
    SpecPhaseConfig _specConfig;

    // Combat timing
    uint32 _combatStartTimeMs = 0;       // Server time when combat started
    uint32 _lastUpdateTimeMs = 0;        // Last update time

    // Current recommendation
    PhaseRecommendation _recommendation;

    // Update throttle
    uint32 _updateTimer = 0;
    static constexpr uint32 UPDATE_INTERVAL_MS = 200; // 5 updates/sec
};

} // namespace Playerbot
