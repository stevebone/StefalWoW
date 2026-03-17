/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * PROACTIVE LINE-OF-SIGHT FIXER
 *
 * Intercepts spell cast attempts and proactively repositions the bot to
 * a valid line-of-sight position before attempting the cast. This prevents
 * wasted GCDs on casts that would fail due to LOS and ensures smooth
 * combat flow.
 *
 * Architecture:
 *   - Per-bot component, called before each spell cast attempt
 *   - Maintains a pending cast queue: when LOS is broken, the spell is
 *     queued and the bot is moved to a valid position first
 *   - Uses LineOfSightManager::FindBestLineOfSightPosition() for smart
 *     position selection (considers terrain, preferred range, movement cost)
 *   - Healers maintain LOS to priority heal targets proactively
 *   - Respects movement deduplication via BotMovementUtil
 *
 * Integration Points:
 *   - LineOfSightManager: CheckLineOfSight, FindBestLineOfSightPosition,
 *                         GetClosestUnblockedPosition
 *   - BotMovementUtil: MoveToPosition for safe movement
 *   - SpellInfo: GetMaxRange, HasAttribute for spell-specific LOS rules
 *   - CombatPhaseDetector: Phase-aware urgency (execute phase = more urgent)
 *
 * Flow:
 *   1. Bot wants to cast spell on target
 *   2. ProactiveLoSFixer::PreCastCheck(spellId, target) is called
 *   3. If LOS is clear, returns CLEAR (proceed to cast)
 *   4. If LOS is broken:
 *      a. Finds best LOS position via LineOfSightManager
 *      b. Queues the pending cast
 *      c. Issues movement command via BotMovementUtil
 *      d. Returns REPOSITIONING (don't cast yet)
 *   5. On next Update(), checks if bot has reached LOS position
 *   6. When in position, returns the pending cast via GetPendingCast()
 *
 * Performance:
 *   - Update interval: 200ms (5 checks/sec)
 *   - LOS checks are cached by LineOfSightManager (1s TTL)
 *   - Position finding is throttled (max once per 500ms)
 *   - Memory: ~200 bytes per bot
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "Position.h"
#include <string>

class Player;
class Unit;
class SpellInfo;

namespace Playerbot
{

class LineOfSightManager;

// ============================================================================
// PRE-CAST CHECK RESULT
// ============================================================================

/// Result of checking LOS before a spell cast
enum class LoSPreCastResult : uint8
{
    CLEAR           = 0,    // LOS is clear, proceed to cast
    REPOSITIONING   = 1,    // Bot is moving to LOS position, cast queued
    SPELL_IGNORES   = 2,    // Spell ignores LOS (proceed to cast)
    NO_TARGET       = 3,    // No valid target
    ALREADY_MOVING  = 4,    // Bot is already repositioning for a different cast
    NO_POSITION     = 5,    // Could not find a valid LOS position
    TOO_FAR         = 6     // Target is out of max theoretical range even after repositioning
};

// ============================================================================
// PENDING CAST INFO
// ============================================================================

/// Information about a queued spell cast waiting for LOS repositioning
struct PendingLoSCast
{
    uint32 spellId = 0;            // Spell to cast after reaching LOS
    ObjectGuid targetGuid;          // Target for the cast
    Position targetPosition;        // Snapshotted target position (for ground-targeted spells)
    Position repositionTarget;      // Where the bot is moving to
    uint32 queueTimeMs = 0;        // When the cast was queued (server time)
    uint32 maxWaitMs = 5000;        // Maximum time to wait for repositioning (5s default)
    float spellMaxRange = 0.0f;     // Max range of the queued spell
    bool isGroundTargeted = false;  // Is this a ground-targeted AoE?

    bool IsValid() const { return spellId > 0; }
    bool IsExpired(uint32 currentTimeMs) const
    {
        return currentTimeMs > queueTimeMs + maxWaitMs;
    }

    void Reset()
    {
        spellId = 0;
        targetGuid.Clear();
        targetPosition = Position();
        repositionTarget = Position();
        queueTimeMs = 0;
        maxWaitMs = 5000;
        spellMaxRange = 0.0f;
        isGroundTargeted = false;
    }
};

// ============================================================================
// PROACTIVE LOS STATISTICS
// ============================================================================

struct ProactiveLoSStats
{
    uint32 totalPreCastChecks = 0;      // Total PreCastCheck calls
    uint32 losWasClear = 0;             // Times LOS was already clear
    uint32 repositionAttempts = 0;      // Times repositioning was initiated
    uint32 repositionSuccesses = 0;     // Times repositioning led to successful cast
    uint32 repositionTimeouts = 0;      // Times repositioning timed out
    uint32 spellIgnoredLos = 0;         // Times spell ignored LOS rules
    uint32 noPositionFound = 0;         // Times no valid LOS position was found
    uint32 healerLoSChecks = 0;         // Healer proactive LOS maintenance checks
    uint32 healerRepositions = 0;       // Healer proactive repositions for group LOS
};

// ============================================================================
// PROACTIVE LOS FIXER
// ============================================================================

class TC_GAME_API ProactiveLoSFixer
{
public:
    explicit ProactiveLoSFixer(Player* bot);
    ~ProactiveLoSFixer() = default;

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /// Initialize the fixer (requires LineOfSightManager to be available)
    void Initialize(LineOfSightManager* losMgr);

    /// Update pending cast state. Call once per combat update.
    /// @param diff Time since last update in milliseconds
    void Update(uint32 diff);

    /// Reset state on combat start
    void OnCombatStart();

    /// Reset state on combat end
    void OnCombatEnd();

    // ========================================================================
    // CORE: PRE-CAST LOS CHECK
    // ========================================================================

    /// Check LOS before attempting a spell cast.
    /// If LOS is blocked, queues the cast and initiates repositioning.
    /// @param spellId Spell to cast
    /// @param target Target unit (or nullptr for self-cast)
    /// @return LoSPreCastResult indicating whether to proceed or wait
    LoSPreCastResult PreCastCheck(uint32 spellId, Unit* target);

    /// Check LOS for a ground-targeted AoE spell
    /// @param spellId Spell to cast
    /// @param targetPos Ground target position
    /// @return LoSPreCastResult indicating whether to proceed or wait
    LoSPreCastResult PreCastCheckPosition(uint32 spellId, Position const& targetPos);

    // ========================================================================
    // PENDING CAST MANAGEMENT
    // ========================================================================

    /// Is there a pending cast waiting for repositioning to complete?
    bool HasPendingCast() const { return _pendingCast.IsValid(); }

    /// Get the pending cast info (for the caller to execute after repositioning)
    PendingLoSCast const& GetPendingCast() const { return _pendingCast; }

    /// Is the pending cast ready to execute? (bot has reached LOS position)
    bool IsPendingCastReady() const;

    /// Clear the pending cast (after it's been executed or abandoned)
    void ClearPendingCast();

    /// Cancel any pending repositioning and cast
    void CancelPendingCast();

    // ========================================================================
    // HEALER LOS MAINTENANCE
    // ========================================================================

    /// For healers: proactively check and maintain LOS to group members.
    /// Call periodically to ensure the healer can see heal targets.
    /// @return true if the healer needs to reposition for group LOS
    bool CheckHealerGroupLoS();

    /// Get the best position for a healer to see all priority targets
    /// @return Position that maximizes LOS to group, or current position if already good
    Position GetBestHealerPosition() const;

    // ========================================================================
    // QUERIES
    // ========================================================================

    /// Is the bot currently repositioning for LOS?
    bool IsRepositioning() const { return _isRepositioning; }

    /// Get time spent repositioning (milliseconds)
    uint32 GetRepositioningTime() const;

    /// Get statistics
    ProactiveLoSStats const& GetStats() const { return _stats; }

    /// Get a text summary for debugging
    ::std::string GetDebugSummary() const;

private:
    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /// Find the best position to cast from considering spell range and LOS
    Position FindCastPosition(Unit* target, float spellRange) const;

    /// Find position to cast a ground-targeted spell from
    Position FindCastPositionForGround(Position const& targetPos, float spellRange) const;

    /// Check if the bot has arrived at the repositioning target
    bool HasReachedRepositionTarget() const;

    /// Check if a spell ignores LOS requirements
    bool DoesSpellIgnoreLos(SpellInfo const* spellInfo) const;

    /// Get the effective max range for a spell (accounting for talents, etc.)
    float GetEffectiveSpellRange(uint32 spellId) const;

    /// Determine combat role for LOS urgency
    bool IsHealerRole() const;
    bool IsRangedRole() const;

    /// Issue movement command to reposition for LOS
    void MoveToLoSPosition(Position const& pos);

    // ========================================================================
    // STATE
    // ========================================================================

    Player* _bot;
    LineOfSightManager* _losMgr = nullptr;
    bool _initialized = false;
    bool _inCombat = false;
    bool _isRepositioning = false;

    /// Currently pending cast
    PendingLoSCast _pendingCast;

    /// Statistics
    ProactiveLoSStats _stats;

    /// Update throttle
    uint32 _updateTimer = 0;
    static constexpr uint32 UPDATE_INTERVAL_MS = 200;   // 5 checks/sec

    /// Position-finding throttle (expensive operation)
    uint32 _lastPositionFindMs = 0;
    static constexpr uint32 POSITION_FIND_COOLDOWN_MS = 500;

    /// Maximum repositioning time before giving up
    static constexpr uint32 MAX_REPOSITION_TIME_MS = 5000;

    /// Arrival tolerance for repositioning (yards)
    static constexpr float REPOSITION_ARRIVAL_TOLERANCE = 3.0f;

    /// Healer: minimum LOS targets threshold (at least this % of group visible)
    static constexpr float HEALER_MIN_LOS_PCT = 0.6f;

    /// Healer: check interval for group LOS maintenance
    uint32 _healerLoSCheckTimer = 0;
    static constexpr uint32 HEALER_LOS_CHECK_INTERVAL_MS = 2000;
};

} // namespace Playerbot
