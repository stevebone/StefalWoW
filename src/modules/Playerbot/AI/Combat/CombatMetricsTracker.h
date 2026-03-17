/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * COMBAT METRICS TRACKER
 *
 * Provides WoW-style DPS/HPS combat meters for bot performance analysis.
 * Tracks damage done, healing done, damage taken per spell, per encounter,
 * and per session. Supports rolling-window DPS/HPS calculations, spell
 * breakdowns, and formatted report generation.
 *
 * Architecture:
 *   - Per-bot instance (not singleton) - each bot has its own tracker
 *   - Circular buffer for event history (fixed memory, no allocations in combat)
 *   - Encounter tracking with start/end detection
 *   - Spell breakdown with category aggregation
 *   - Formatted output for .bot dps / .bot hps chat commands
 *
 * Usage:
 *   CombatMetricsTracker tracker(bot);
 *   tracker.RecordDamage(spellId, target, amount, overkill, isCrit);
 *   tracker.RecordHealing(spellId, target, amount, overheal, isCrit);
 *   float dps = tracker.GetCurrentDPS();
 *   std::string report = tracker.FormatDPSReport();
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <array>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <chrono>

class Player;
class Unit;

namespace Playerbot
{

// ============================================================================
// COMBAT EVENT TYPES
// ============================================================================

enum class CombatEventType : uint8
{
    DAMAGE_DONE     = 0,    // Bot dealt damage
    DAMAGE_TAKEN    = 1,    // Bot received damage
    HEALING_DONE    = 2,    // Bot healed someone
    HEALING_TAKEN   = 3,    // Bot received healing
    ABSORB_DONE     = 4,    // Bot's absorb shielded damage
    ABSORB_TAKEN    = 5     // Bot was shielded by absorb
};

// ============================================================================
// COMBAT EVENT RECORD (Fixed-size for circular buffer)
// ============================================================================

struct CombatEvent
{
    uint32 timestamp{0};            // GameTime in ms
    uint32 spellId{0};              // Spell that caused the event
    uint32 amount{0};               // Raw amount (damage/healing)
    uint32 overhealOrOverkill{0};   // Overheal or overkill amount
    ObjectGuid targetGuid;          // Who was the target
    CombatEventType type{CombatEventType::DAMAGE_DONE};
    bool isCrit{false};             // Was this a critical strike
    bool isAbsorbed{false};         // Was this (partially) absorbed
    bool isPeriodic{false};         // Was this from a DoT/HoT
};

// ============================================================================
// SPELL METRICS (Aggregated per-spell data)
// ============================================================================

struct SpellMetrics
{
    uint32 spellId{0};
    uint64 totalAmount{0};          // Total damage or healing
    uint64 totalOverhealOverkill{0};// Total overheal/overkill
    uint32 hitCount{0};             // Number of hits
    uint32 critCount{0};            // Number of critical strikes
    uint32 maxHit{0};               // Largest single hit
    uint32 maxCrit{0};              // Largest single crit
    uint32 periodicCount{0};        // Number of periodic ticks

    float GetAverageHit() const
    {
        return hitCount > 0 ? static_cast<float>(totalAmount) / static_cast<float>(hitCount) : 0.0f;
    }

    float GetCritRate() const
    {
        return hitCount > 0 ? static_cast<float>(critCount) / static_cast<float>(hitCount) * 100.0f : 0.0f;
    }

    float GetEfficiency() const
    {
        uint64 total = totalAmount + totalOverhealOverkill;
        return total > 0 ? static_cast<float>(totalAmount) / static_cast<float>(total) * 100.0f : 100.0f;
    }
};

// ============================================================================
// ENCOUNTER SUMMARY
// ============================================================================

struct EncounterSummary
{
    uint32 startTime{0};            // When combat started
    uint32 endTime{0};              // When combat ended
    uint32 durationMs{0};           // Duration in ms
    uint64 totalDamage{0};          // Total damage done
    uint64 totalHealing{0};         // Total healing done
    uint64 totalDamageTaken{0};     // Total damage taken
    uint64 totalOverheal{0};        // Total overhealing
    uint64 totalOverkill{0};        // Total overkill
    uint32 spellsCast{0};           // Total events
    uint32 critCount{0};            // Total crits
    float dps{0.0f};               // Average DPS for encounter
    float hps{0.0f};               // Average HPS for encounter
    float dtps{0.0f};              // Damage taken per second

    // Top 5 spells by damage/healing
    struct SpellEntry
    {
        uint32 spellId{0};
        uint64 amount{0};
        float percentage{0.0f};     // % of total
    };
    ::std::array<SpellEntry, 5> topDamageSpells{};
    ::std::array<SpellEntry, 5> topHealingSpells{};
    uint32 topDamageCount{0};
    uint32 topHealingCount{0};
};

// ============================================================================
// COMBAT METRICS TRACKER
// ============================================================================

class CombatMetricsTracker
{
public:
    explicit CombatMetricsTracker(Player* bot);
    ~CombatMetricsTracker() = default;

    // ========================================================================
    // EVENT RECORDING
    // ========================================================================

    /// Record damage dealt by the bot
    void RecordDamage(uint32 spellId, Unit* target, uint32 amount,
                      uint32 overkill = 0, bool isCrit = false, bool isPeriodic = false);

    /// Record healing done by the bot
    void RecordHealing(uint32 spellId, Unit* target, uint32 amount,
                       uint32 overheal = 0, bool isCrit = false, bool isPeriodic = false);

    /// Record damage taken by the bot
    void RecordDamageTaken(uint32 spellId, Unit* attacker, uint32 amount,
                           bool isAbsorbed = false);

    /// Record healing received by the bot
    void RecordHealingTaken(uint32 spellId, Unit* healer, uint32 amount,
                            uint32 overheal = 0);

    /// Record absorb shield preventing damage
    void RecordAbsorb(uint32 spellId, Unit* target, uint32 amount);

    // ========================================================================
    // CURRENT METRICS (Rolling Window)
    // ========================================================================

    /// Get current DPS (rolling window, default 5 seconds)
    float GetCurrentDPS(uint32 windowMs = 5000) const;

    /// Get current HPS (rolling window, default 5 seconds)
    float GetCurrentHPS(uint32 windowMs = 5000) const;

    /// Get current DTPS - damage taken per second (rolling window)
    float GetCurrentDTPS(uint32 windowMs = 5000) const;

    /// Get overall DPS since combat start
    float GetOverallDPS() const;

    /// Get overall HPS since combat start
    float GetOverallHPS() const;

    /// Get overall DTPS since combat start
    float GetOverallDTPS() const;

    // ========================================================================
    // SPELL BREAKDOWN
    // ========================================================================

    /// Get damage breakdown by spell
    ::std::vector<SpellMetrics> GetDamageBySpell() const;

    /// Get healing breakdown by spell
    ::std::vector<SpellMetrics> GetHealingBySpell() const;

    /// Get damage taken breakdown by spell
    ::std::vector<SpellMetrics> GetDamageTakenBySpell() const;

    /// Get metrics for a specific spell
    SpellMetrics GetSpellDamageMetrics(uint32 spellId) const;
    SpellMetrics GetSpellHealingMetrics(uint32 spellId) const;

    // ========================================================================
    // ENCOUNTER TRACKING
    // ========================================================================

    /// Notify that combat has started
    void OnCombatStart();

    /// Notify that combat has ended
    void OnCombatEnd();

    /// Check if currently in combat
    bool IsInCombat() const { return _inCombat; }

    /// Get the current encounter duration in ms
    uint32 GetCurrentEncounterDuration() const;

    /// Get the last completed encounter summary
    EncounterSummary const& GetLastEncounterSummary() const { return _lastEncounter; }

    /// Get encounter history (last N encounters)
    ::std::vector<EncounterSummary> const& GetEncounterHistory() const { return _encounterHistory; }

    // ========================================================================
    // SESSION TOTALS
    // ========================================================================

    /// Total damage done this session
    uint64 GetSessionTotalDamage() const { return _sessionTotalDamage; }

    /// Total healing done this session
    uint64 GetSessionTotalHealing() const { return _sessionTotalHealing; }

    /// Total damage taken this session
    uint64 GetSessionTotalDamageTaken() const { return _sessionTotalDamageTaken; }

    /// Total overhealing this session
    uint64 GetSessionTotalOverheal() const { return _sessionTotalOverheal; }

    /// Number of encounters this session
    uint32 GetEncounterCount() const { return static_cast<uint32>(_encounterHistory.size()); }

    // ========================================================================
    // FORMATTED REPORTS (For chat commands)
    // ========================================================================

    /// Format a DPS report for chat output
    /// Returns multi-line string suitable for SendChatMessage
    ::std::string FormatDPSReport() const;

    /// Format an HPS report for chat output
    ::std::string FormatHPSReport() const;

    /// Format a damage taken report
    ::std::string FormatDTReport() const;

    /// Format a full combat summary (last encounter)
    ::std::string FormatEncounterSummary() const;

    /// Format a brief one-line DPS summary
    ::std::string FormatBriefDPS() const;

    /// Format a brief one-line HPS summary
    ::std::string FormatBriefHPS() const;

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /// Reset all metrics (full reset)
    void Reset();

    /// Reset encounter-specific data (keeps session totals)
    void ResetEncounter();

    /// Update internal timers
    void Update(uint32 diff);

private:
    // ========================================================================
    // INTERNAL HELPERS
    // ========================================================================

    void RecordEvent(CombatEvent const& event);
    float CalculateRateInWindow(CombatEventType type, uint32 windowMs) const;
    void BuildEncounterSummary();
    void UpdateSpellMetrics(uint32 spellId, uint32 amount, uint32 overhealOverkill,
                            bool isCrit, bool isPeriodic, CombatEventType type);
    ::std::string FormatNumber(uint64 number) const;
    ::std::string FormatDuration(uint32 ms) const;
    ::std::string GetSpellName(uint32 spellId) const;
    uint32 GetCurrentTimeMs() const;

    // Sorted spell list for reports (by amount descending)
    ::std::vector<SpellMetrics> GetSortedSpellMetrics(
        ::std::unordered_map<uint32, SpellMetrics> const& metrics) const;

    // ========================================================================
    // MEMBER VARIABLES
    // ========================================================================

    Player* _bot;

    // Circular buffer for recent events (fixed-size, no allocation in combat)
    static constexpr uint32 EVENT_BUFFER_SIZE = 512;
    ::std::array<CombatEvent, EVENT_BUFFER_SIZE> _eventBuffer;
    uint32 _eventWriteIndex{0};
    uint32 _eventCount{0};

    // Per-spell aggregated metrics (damage done)
    ::std::unordered_map<uint32, SpellMetrics> _damageBySpell;

    // Per-spell aggregated metrics (healing done)
    ::std::unordered_map<uint32, SpellMetrics> _healingBySpell;

    // Per-spell aggregated metrics (damage taken)
    ::std::unordered_map<uint32, SpellMetrics> _damageTakenBySpell;

    // Encounter tracking
    bool _inCombat{false};
    uint32 _combatStartTime{0};
    uint32 _combatEndTime{0};

    // Current encounter running totals
    uint64 _encounterDamage{0};
    uint64 _encounterHealing{0};
    uint64 _encounterDamageTaken{0};
    uint64 _encounterOverheal{0};
    uint64 _encounterOverkill{0};
    uint32 _encounterCrits{0};
    uint32 _encounterEvents{0};

    // Session totals
    uint64 _sessionTotalDamage{0};
    uint64 _sessionTotalHealing{0};
    uint64 _sessionTotalDamageTaken{0};
    uint64 _sessionTotalOverheal{0};

    // Encounter history
    EncounterSummary _lastEncounter;
    ::std::vector<EncounterSummary> _encounterHistory;
    static constexpr uint32 MAX_ENCOUNTER_HISTORY = 10;

    // Update timer
    uint32 _updateTimer{0};
    static constexpr uint32 UPDATE_INTERVAL = 500;  // 500ms update interval
};

} // namespace Playerbot
