/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * PROC EXPIRY MONITOR
 *
 * Tracks active proc/buff auras on bots and escalates priority when they
 * are about to expire without being consumed. This prevents waste of
 * valuable procs like:
 *
 *   - Art of War (Paladin) - Free Flash of Light, 15s
 *   - Brain Freeze (Mage) - Free Flurry, 15s
 *   - Maelstrom Weapon (Shaman) - Stacks for instant cast, 30s
 *   - Lock and Load (Hunter) - Free Aimed Shot, 15s
 *   - Eclipse (Druid) - Solar/Lunar buff, variable
 *   - Sudden Death (Warrior) - Free Execute, 10s
 *   - Rime (DK) - Free Howling Blast, 15s
 *   - Heating Up / Hot Streak (Mage) - Pyroblast proc
 *   - Fingers of Frost (Mage) - Treat as frozen, 15s
 *
 * Architecture:
 *   - Per-bot monitor attached to the combat AI
 *   - Scans active auras each update for tracked proc IDs
 *   - Calculates urgency score: 0.0 (plenty of time) to 1.0 (about to expire)
 *   - Rotation systems query GetHighestUrgencyProc() to decide if they
 *     should consume a proc immediately rather than cast something else
 *   - Configurable urgency thresholds per proc type
 */

#pragma once

#include "Define.h"
#include <string>
#include <vector>
#include <unordered_map>

class Player;

namespace Playerbot
{

// ============================================================================
// PROC URGENCY LEVEL
// ============================================================================

enum class ProcUrgency : uint8
{
    NONE        = 0,    // No proc active or plenty of time
    LOW         = 1,    // >50% duration remaining
    MODERATE    = 2,    // 25-50% duration remaining
    HIGH        = 3,    // 10-25% duration remaining
    CRITICAL    = 4     // <10% duration remaining - consume NOW
};

// ============================================================================
// PROC CATEGORY
// ============================================================================

enum class ProcCategory : uint8
{
    DAMAGE_BOOST    = 0,    // Free/instant damage spell (Art of War, Brain Freeze)
    INSTANT_CAST    = 1,    // Makes a cast-time spell instant (Maelstrom stacks)
    FREE_CAST       = 2,    // Makes a spell free (Clearcasting, Lock and Load)
    STAT_BUFF       = 3,    // Temporary stat increase (trinket procs)
    STACKING        = 4,    // Stacking proc (Maelstrom Weapon - consume at max)
    EXECUTE_UNLOCK  = 5,    // Unlocks an ability (Sudden Death, Kill Shot reset)
    HEALING_BOOST   = 6     // Free/instant healing spell
};

// ============================================================================
// TRACKED PROC INFO
// ============================================================================

struct TrackedProcInfo
{
    uint32 procAuraId{0};           // The aura/buff spell ID to watch
    uint32 consumeSpellId{0};       // The spell that should be cast to consume it
    ::std::string name;
    ProcCategory category{ProcCategory::DAMAGE_BOOST};
    uint32 baseDurationMs{15000};   // Expected base duration in ms
    uint8 maxStacks{1};             // 1 for non-stacking, >1 for stacking procs
    uint8 consumeAtStacks{0};       // 0 = consume immediately, N = consume at N stacks
    float urgencyThreshold{0.25f};  // Duration fraction below which urgency is HIGH

    // Class/spec filter (0 = all)
    uint8 classId{0};
    uint32 specId{0};
};

// ============================================================================
// ACTIVE PROC STATE
// ============================================================================

struct ActiveProcState
{
    uint32 procAuraId{0};
    uint32 consumeSpellId{0};
    ::std::string name;
    ProcCategory category{ProcCategory::DAMAGE_BOOST};
    int32 remainingMs{0};           // Remaining duration in ms
    int32 totalDurationMs{0};       // Total duration when it was applied
    uint8 currentStacks{0};
    uint8 maxStacks{1};
    uint8 consumeAtStacks{0};
    float urgencyScore{0.0f};       // 0.0 to 1.0 (1.0 = about to expire)
    ProcUrgency urgencyLevel{ProcUrgency::NONE};
};

// ============================================================================
// PROC EXPIRY STATISTICS
// ============================================================================

struct ProcExpiryStats
{
    uint32 procsDetected{0};
    uint32 procsConsumedInTime{0};
    uint32 procsExpiredUnused{0};
    uint32 urgentWarningsIssued{0};
    uint32 criticalWarningsIssued{0};
};

// ============================================================================
// PROC EXPIRY MONITOR
// ============================================================================

class ProcExpiryMonitor
{
public:
    explicit ProcExpiryMonitor(Player* bot);
    ~ProcExpiryMonitor() = default;

    // ========================================================================
    // INITIALIZATION
    // ========================================================================

    /// Initialize tracked procs for this bot's class/spec
    void Initialize();

    /// Re-initialize after spec change
    void OnSpecChanged();

    // ========================================================================
    // UPDATE
    // ========================================================================

    /// Main update - scans auras and calculates urgency
    void Update(uint32 diff);

    // ========================================================================
    // QUERIES
    // ========================================================================

    /// Get the most urgent active proc that needs consuming
    /// Returns nullptr if no urgent procs
    ActiveProcState const* GetHighestUrgencyProc() const;

    /// Get all currently active procs sorted by urgency (highest first)
    ::std::vector<ActiveProcState> const& GetActiveProcs() const { return _activeProcs; }

    /// Check if a specific proc is active and urgent
    bool IsProcUrgent(uint32 procAuraId) const;

    /// Get urgency level for a specific proc
    ProcUrgency GetProcUrgency(uint32 procAuraId) const;

    /// Get the spell that should be cast to consume a proc
    uint32 GetConsumeSpellForProc(uint32 procAuraId) const;

    /// Is there any proc at HIGH or CRITICAL urgency?
    bool HasUrgentProc() const;

    /// How many procs are currently active?
    uint32 GetActiveProcCount() const { return static_cast<uint32>(_activeProcs.size()); }

    // ========================================================================
    // STATISTICS
    // ========================================================================

    ProcExpiryStats const& GetStats() const { return _stats; }
    void ResetStats() { _stats = ProcExpiryStats{}; }

    /// Get formatted status string
    ::std::string FormatStatus() const;

private:
    // ========================================================================
    // INTERNAL
    // ========================================================================

    /// Build the proc database for all classes
    void BuildProcDatabase();

    /// Scan bot's active auras for tracked procs
    void ScanActiveProcs();

    /// Calculate urgency score from remaining time and total duration
    float CalculateUrgencyScore(int32 remainingMs, int32 totalDurationMs,
                                 uint8 currentStacks, uint8 consumeAtStacks,
                                 float urgencyThreshold) const;

    /// Convert urgency score to level
    static ProcUrgency ScoreToUrgency(float score);

    // ========================================================================
    // MEMBER VARIABLES
    // ========================================================================

    Player* _bot;
    bool _initialized{false};

    // All possible tracked procs (filtered by class/spec)
    ::std::vector<TrackedProcInfo> _trackedProcs;

    // Currently active procs (sorted by urgency, highest first)
    ::std::vector<ActiveProcState> _activeProcs;

    // Statistics
    ProcExpiryStats _stats;

    // Previously active procs (to detect expiry without consumption)
    ::std::unordered_map<uint32, bool> _previouslyActive;

    // Update throttle
    uint32 _updateTimer{0};
    static constexpr uint32 UPDATE_INTERVAL_MS = 250;   // Check every 250ms for responsiveness

    // Urgency thresholds (as fraction of total duration)
    static constexpr float URGENCY_LOW_THRESHOLD = 0.50f;      // 50% remaining
    static constexpr float URGENCY_MODERATE_THRESHOLD = 0.25f;  // 25% remaining
    static constexpr float URGENCY_HIGH_THRESHOLD = 0.10f;      // 10% remaining
    static constexpr float URGENCY_CRITICAL_THRESHOLD = 0.05f;  // 5% remaining
};

} // namespace Playerbot
