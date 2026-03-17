/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Time-to-Kill (TTK) Estimator for bot combat intelligence.
 * Tracks rolling damage windows per target to estimate how quickly
 * a target will die, enabling bots to skip long-cast spells on
 * targets that will die before the cast finishes.
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "Threading/LockHierarchy.h"
#include <unordered_map>
#include <deque>

class Player;
class Unit;
class Group;

namespace Playerbot
{

// Per-target damage tracking entry
struct DamageEvent
{
    uint32 timestamp;
    uint32 damage;
};

// Per-target TTK tracking data
struct TargetTTKData
{
    ObjectGuid targetGuid;
    std::deque<DamageEvent> damageHistory;   // Rolling window of damage events
    uint64 totalHealthAtFirstSight;          // Max health when first seen
    uint64 lastKnownHealth;                  // Last observed health
    uint32 lastUpdated;                      // Last time we updated this entry
    float cachedTTK;                         // Cached TTK value (seconds)
    uint32 cacheTimestamp;                   // When cachedTTK was computed
    uint32 noHealthChangeStart;             // Timestamp when health stopped changing (invuln detection)
    bool invulnerable;                       // Target appears invulnerable

    TargetTTKData()
        : totalHealthAtFirstSight(0)
        , lastKnownHealth(0)
        , lastUpdated(0)
        , cachedTTK(0.0f)
        , cacheTimestamp(0)
        , noHealthChangeStart(0)
        , invulnerable(false)
    {}
};

/// TTK Estimator: tracks group DPS on each target and predicts time-to-kill.
/// Used by TargetSelector::EstimateTimeToKill() and SpellFallbackChain.
class TC_GAME_API TTKEstimator
{
public:
    explicit TTKEstimator(Player* bot);
    ~TTKEstimator() = default;

    // Called every combat update cycle (50ms) from CombatBehaviorIntegration::UpdateManagers
    void Update(uint32 diff);

    // Estimate time-to-kill in seconds for a specific target.
    // Returns INFINITY for invulnerable or unknown targets.
    float EstimateTTK(Unit* target) const;

    // Check if a spell cast time exceeds the TTK threshold.
    // castTimeMs: spell cast time in milliseconds
    // target: the enemy target
    // Returns true if the spell should be skipped (cast too long for dying target).
    bool ShouldSkipLongCast(uint32 castTimeMs, Unit* target) const;

    // Get the rolling DPS being dealt to a target by the group
    float GetGroupDPSOnTarget(Unit* target) const;

    // Reset all tracking data (called on combat end)
    void Reset();

    // Remove stale entries for targets no longer relevant
    void PruneStaleTargets();

private:
    // Sample health of all nearby enemies to track damage
    void SampleTargetHealth();

    // Calculate TTK from damage history
    float CalculateTTK(const TargetTTKData& data) const;

    // Detect invulnerability (no health change over 3s while taking hits)
    void DetectInvulnerability(TargetTTKData& data, uint64 currentHealth);

    Player* _bot;

    // Per-target tracking
    mutable std::unordered_map<ObjectGuid, TargetTTKData> _targetData;

    // Thread safety
    mutable Playerbot::OrderedMutex<Playerbot::LockOrder::BOT_AI_STATE> _mutex;

    // Configuration
    static constexpr uint32 DAMAGE_WINDOW_MS = 5000;       // 5s rolling DPS window
    static constexpr uint32 SAMPLE_INTERVAL_MS = 250;      // Sample health every 250ms
    static constexpr uint32 CACHE_DURATION_MS = 200;        // Reuse TTK for 200ms
    static constexpr uint32 STALE_TARGET_MS = 10000;        // Prune after 10s without update
    static constexpr uint32 INVULN_DETECT_MS = 3000;        // 3s no health change = invulnerable
    static constexpr float SOLO_TTK_RATIO = 1.0f;           // Solo: skip if castTime > TTK * 1.0
    static constexpr float GROUP_TTK_RATIO = 0.8f;          // Group: skip if castTime > TTK * 0.8

    uint32 _sampleTimer;
    uint32 _pruneTimer;
};

} // namespace Playerbot
