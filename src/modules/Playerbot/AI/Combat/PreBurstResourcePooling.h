/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * PRE-BURST RESOURCE POOLING
 *
 * Detects when major burst cooldowns are about to come off cooldown and
 * signals the bot's rotation to pool resources (hold generators, reduce
 * spending) so the bot enters the burst window with 80-100% resources.
 *
 * Architecture:
 *   - Per-bot component, called during combat update
 *   - Reads burst CD data from ClassSpellDatabase (CooldownSpellEntry)
 *   - Queries SpellHistory for remaining cooldown on each burst CD
 *   - When a burst CD is 3-5 seconds from ready, sets pooling state
 *   - Rotation systems check ShouldPoolResources() to reduce spending
 *   - Thread-safe (called from bot AI update thread only)
 *
 * Integration Points:
 *   - ClassAI rotation: Check ShouldPoolResources() before low-priority spenders
 *   - ResourceManager: Uses GetResourcePercent() to track current resource level
 *   - CooldownStackingOptimizer: Can signal upcoming burst windows
 *   - CombatSpecializationBase: Check via ShouldPool() in rotation decision
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <vector>
#include <string>

class Player;
class SpellInfo;

namespace Playerbot
{

// ============================================================================
// POOLING STATE
// ============================================================================

/// Current pooling recommendation for the rotation
enum class PoolingState : uint8
{
    NONE            = 0,    // No pooling needed, spend normally
    LIGHT           = 1,    // Skip low-priority fillers, maintain resource
    MODERATE        = 2,    // Only spend on high-priority abilities
    AGGRESSIVE      = 3     // Spend nothing, pure pooling
};

/// Info about a tracked burst cooldown
struct BurstCooldownInfo
{
    uint32 spellId = 0;
    std::string name;
    uint32 cooldownMs = 0;          // Total CD duration
    uint32 remainingMs = 0;         // Time until ready
    bool isReady = false;           // Currently off cooldown
    bool isActive = false;          // Buff is currently active
    bool requiresResource = true;   // Does the burst window need resources?
    float resourceThreshold = 0.8f; // Target resource % before burst (0.8 = 80%)
};

/// Current pooling recommendation with details
struct PoolingRecommendation
{
    PoolingState state = PoolingState::NONE;
    uint32 poolingForSpellId = 0;       // Which burst CD we're pooling for
    std::string poolingReason;           // Human-readable reason
    float currentResourcePct = 0.0f;     // Current resource percentage
    float targetResourcePct = 0.0f;      // Target resource percentage
    uint32 timeUntilBurstMs = 0;         // Time until the burst CD is ready
    bool atTargetResource = false;        // Already at target resource level
    float resourceDeficit = 0.0f;         // How far below target (0.0 = at target)

    void Reset()
    {
        state = PoolingState::NONE;
        poolingForSpellId = 0;
        poolingReason.clear();
        currentResourcePct = targetResourcePct = 0.0f;
        timeUntilBurstMs = 0;
        atTargetResource = false;
        resourceDeficit = 0.0f;
    }
};

// ============================================================================
// PRE-BURST RESOURCE POOLING
// ============================================================================

class TC_GAME_API PreBurstResourcePooling
{
public:
    explicit PreBurstResourcePooling(Player* bot);
    ~PreBurstResourcePooling() = default;

    // ========================================================================
    // CORE UPDATE
    // ========================================================================

    /// Update pooling state based on burst CD cooldowns.
    /// @param diff Time since last update in milliseconds
    void Update(uint32 diff);

    /// Reset pooling state (e.g., combat end)
    void Reset();

    /// Initialize burst CD tracking from ClassSpellDatabase for this bot's spec
    void Initialize();

    // ========================================================================
    // POOLING QUERIES (used by rotation systems)
    // ========================================================================

    /// Should the bot pool resources right now?
    /// This is the primary query for rotation integration.
    bool ShouldPoolResources() const { return _recommendation.state != PoolingState::NONE; }

    /// Get the current pooling state
    PoolingState GetPoolingState() const { return _recommendation.state; }

    /// Get the full pooling recommendation with details
    PoolingRecommendation const& GetRecommendation() const { return _recommendation; }

    /// Check if a specific ability should be skipped due to pooling.
    /// High-priority abilities are never skipped. Low-priority fillers are skipped first.
    /// @param spellId The ability being considered
    /// @param priority The ability's rotation priority (0=highest, higher=lower)
    /// @return true if the ability should be skipped to conserve resources
    bool ShouldSkipForPooling(uint32 spellId, float priority) const;

    /// Get the resource percentage target for the upcoming burst
    float GetTargetResourcePercent() const { return _recommendation.targetResourcePct; }

    /// Get time in ms until the next burst window
    uint32 GetTimeUntilBurst() const { return _recommendation.timeUntilBurstMs; }

    /// Check if burst CD is currently active (we're IN the burst window)
    bool IsInBurstWindow() const { return _isInBurstWindow; }

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /// Set the time window before burst to start pooling (default: 5000ms)
    void SetPoolingWindowMs(uint32 ms) { _poolingWindowMs = ms; }

    /// Set minimum resource percentage to pool to (default: 0.80 = 80%)
    void SetMinPoolTarget(float pct) { _minPoolTarget = pct; }

    /// Set maximum resource percentage to pool to (default: 0.95 = 95%)
    void SetMaxPoolTarget(float pct) { _maxPoolTarget = pct; }

    /// Override burst CDs to track (normally auto-detected from ClassSpellDatabase)
    void AddBurstCooldown(uint32 spellId, std::string name, float resourceThreshold = 0.8f);

    /// Clear all tracked burst CDs
    void ClearBurstCooldowns() { _trackedBurstCDs.clear(); }

private:
    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /// Detect the bot's class/spec and load burst CDs from ClassSpellDatabase
    void LoadSpecBurstCooldowns();

    /// Update remaining cooldown on all tracked burst CDs
    void UpdateCooldownTracking();

    /// Calculate the pooling recommendation based on tracked CDs
    void CalculateRecommendation();

    /// Get the bot's current primary resource percentage (mana/energy/rage/etc.)
    float GetCurrentResourcePercent() const;

    /// Determine pooling state based on time until burst and current resource
    PoolingState DeterminePoolingState(uint32 timeUntilBurstMs, float currentResourcePct,
                                        float targetResourcePct) const;

    /// Check if the bot currently has a burst buff active
    bool CheckBurstActive() const;

    // ========================================================================
    // STATE
    // ========================================================================

    Player* _bot;
    bool _initialized = false;

    // Tracked burst cooldowns
    std::vector<BurstCooldownInfo> _trackedBurstCDs;

    // Current recommendation
    PoolingRecommendation _recommendation;
    bool _isInBurstWindow = false;

    // Update timer
    uint32 _updateTimer = 0;
    static constexpr uint32 UPDATE_INTERVAL_MS = 250; // 4 updates/sec

    // Configuration
    uint32 _poolingWindowMs = 5000;     // Start pooling 5s before burst
    float _minPoolTarget = 0.80f;        // Pool to at least 80%
    float _maxPoolTarget = 0.95f;        // Pool to at most 95% (leave room for procs)

    // Priority threshold: abilities with priority above this are skipped during pooling
    // Lower priority number = higher priority ability
    static constexpr float LIGHT_POOL_SKIP_THRESHOLD = 8.0f;     // Skip priority 8+ in light pooling
    static constexpr float MODERATE_POOL_SKIP_THRESHOLD = 5.0f;  // Skip priority 5+ in moderate pooling
    static constexpr float AGGRESSIVE_POOL_SKIP_THRESHOLD = 2.0f; // Skip all but top 2 priorities
};

} // namespace Playerbot
