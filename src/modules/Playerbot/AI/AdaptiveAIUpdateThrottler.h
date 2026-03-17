/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * ST-1: Adaptive AI Update Throttling System
 *
 * Purpose:
 * Dynamically adjusts bot AI update frequency based on contextual factors:
 * - Proximity to human (non-bot) players
 * - Combat state (in combat vs idle)
 * - Bot activity level (questing, following, idle)
 *
 * Performance Target: 10-15% CPU reduction for bots far from human players
 *
 * Design Principles:
 * - Bots near human players get FULL update rate (100%) - they're visible/interactive
 * - Bots in combat get FULL update rate (100%) - combat requires responsiveness
 * - Bots far from players get REDUCED update rate (25-50%) - invisible to players
 * - Idle bots get MINIMAL update rate (10%) - no active tasks
 */

#ifndef PLAYERBOT_AI_ADAPTIVE_AI_UPDATE_THROTTLER_H
#define PLAYERBOT_AI_ADAPTIVE_AI_UPDATE_THROTTLER_H

#include "Define.h"
#include "ObjectGuid.h"
#include <atomic>
#include <chrono>

class Player;

namespace Playerbot
{

// Forward declarations
class BotAI;

// ============================================================================
// THROTTLE TIER DEFINITIONS
// ============================================================================

/**
 * @brief Throttle tier determines the update frequency multiplier
 *
 * Based on proximity, combat state, and activity level
 */
enum class ThrottleTier : uint8
{
    FULL_RATE,      // 100% - Near humans, in combat, or group with human leader
    HIGH_RATE,      // 75%  - Moderate distance, active questing/grinding
    MEDIUM_RATE,    // 50%  - Far from players, simple following
    LOW_RATE,       // 25%  - Very far, minimal activity
    MINIMAL_RATE    // 10%  - Idle, out of range, no tasks
};

/**
 * @brief Activity classification for throttle calculation
 */
enum class AIActivityType : uint8
{
    COMBAT,         // In active combat - highest priority
    QUESTING,       // Actively completing quest objectives
    GRINDING,       // Killing mobs for XP/loot
    FOLLOWING,      // Following a player or group
    GATHERING,      // Gathering resources
    TRAVELING,      // Moving to destination
    SOCIALIZING,    // Trading, talking, interacting
    RESTING,        // Eating/drinking to restore resources
    IDLE            // No active task
};

// ============================================================================
// THROTTLE CONFIGURATION
// ============================================================================

/**
 * @brief Configuration for throttling behavior
 */
struct ThrottleConfig
{
    // Distance thresholds (in yards)
    float nearHumanDistance;        // Distance considered "near" a human player
    float midRangeDistance;         // Mid-range distance
    float farDistance;              // Far distance
    float outOfRangeDistance;       // Out of range (minimal updates)

    // Update interval multipliers by tier
    float fullRateMultiplier;       // 1.0 = no throttling
    float highRateMultiplier;       // 0.75 = 75% update rate
    float mediumRateMultiplier;     // 0.50 = 50% update rate
    float lowRateMultiplier;        // 0.25 = 25% update rate
    float minimalRateMultiplier;    // 0.10 = 10% update rate

    // Base intervals (ms)
    uint32 baseUpdateInterval;      // Base interval before throttling

    // Refresh intervals for expensive calculations
    uint32 proximityCheckInterval;  // How often to check nearby humans
    uint32 activityCheckInterval;   // How often to reassess activity

    // Default configuration
    static ThrottleConfig Default()
    {
        return ThrottleConfig{
            .nearHumanDistance = 100.0f,
            .midRangeDistance = 250.0f,
            .farDistance = 500.0f,
            .outOfRangeDistance = 1000.0f,

            .fullRateMultiplier = 1.0f,
            .highRateMultiplier = 0.75f,
            .mediumRateMultiplier = 0.50f,
            .lowRateMultiplier = 0.25f,
            .minimalRateMultiplier = 0.10f,

            .baseUpdateInterval = 100,    // 100ms = 10 updates/sec base
            .proximityCheckInterval = 2000, // Check humans every 2 seconds
            .activityCheckInterval = 1000   // Check activity every 1 second
        };
    }
};

// ============================================================================
// THROTTLE METRICS
// ============================================================================

/**
 * @brief Metrics for monitoring throttle effectiveness
 */
struct ThrottleMetrics
{
    uint64 totalUpdatesSkipped;
    uint64 totalUpdatesProcessed;
    uint32 currentThrottleTier;
    float averageUpdateInterval;
    float nearestHumanDistance;
    bool inCombat;
    AIActivityType currentActivity;

    // Reset metrics
    void Reset()
    {
        totalUpdatesSkipped = 0;
        totalUpdatesProcessed = 0;
        currentThrottleTier = 0;
        averageUpdateInterval = 0.0f;
        nearestHumanDistance = 0.0f;
        inCombat = false;
        currentActivity = AIActivityType::IDLE;
    }
};

// ============================================================================
// ADAPTIVE AI UPDATE THROTTLER CLASS
// ============================================================================

/**
 * @brief Per-bot throttler that determines update frequency
 *
 * Each bot has its own throttler instance that tracks:
 * - Proximity to nearest human player
 * - Current combat state
 * - Activity level
 *
 * Based on these factors, it calculates the appropriate update interval.
 */
class AdaptiveAIUpdateThrottler
{
public:
    explicit AdaptiveAIUpdateThrottler(Player* bot, BotAI* ai);
    ~AdaptiveAIUpdateThrottler() = default;

    // Non-copyable, movable
    AdaptiveAIUpdateThrottler(const AdaptiveAIUpdateThrottler&) = delete;
    AdaptiveAIUpdateThrottler& operator=(const AdaptiveAIUpdateThrottler&) = delete;
    AdaptiveAIUpdateThrottler(AdaptiveAIUpdateThrottler&&) = default;
    AdaptiveAIUpdateThrottler& operator=(AdaptiveAIUpdateThrottler&&) = default;

    // ========================================================================
    // MAIN THROTTLE INTERFACE
    // ========================================================================

    /**
     * @brief Check if update should be processed this tick
     * @param diff Time since last tick (ms)
     * @return true if update should proceed, false if throttled
     */
    bool ShouldUpdate(uint32 diff);

    /**
     * @brief Force next update to proceed (for critical events)
     */
    void ForceNextUpdate();

    /**
     * @brief Get current throttle tier
     */
    ThrottleTier GetCurrentTier() const { return _currentTier; }

    /**
     * @brief Get current effective update interval (ms)
     */
    uint32 GetEffectiveUpdateInterval() const { return _effectiveInterval; }

    /**
     * @brief Get metrics for monitoring
     */
    ThrottleMetrics const& GetMetrics() const { return _metrics; }

    // ========================================================================
    // STATE NOTIFICATIONS
    // ========================================================================

    /**
     * @brief Notify throttler that combat has started
     * Immediately switches to FULL_RATE tier
     */
    void OnCombatStart();

    /**
     * @brief Notify throttler that combat has ended
     * Allows tier to be recalculated on next check
     */
    void OnCombatEnd();

    /**
     * @brief Notify that a human player is now in range
     * @param humanGuid GUID of the human player
     * @param distance Distance to the human player
     */
    void OnHumanNearby(ObjectGuid humanGuid, float distance);

    /**
     * @brief Notify activity change
     * @param newActivity The new activity state
     */
    void OnActivityChange(AIActivityType newActivity);

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /**
     * @brief Set custom throttle configuration
     */
    void SetConfig(ThrottleConfig const& config) { _config = config; }

    /**
     * @brief Get current configuration
     */
    ThrottleConfig const& GetConfig() const { return _config; }

    /**
     * @brief Enable/disable throttling (for debugging)
     */
    void SetEnabled(bool enabled) { _enabled = enabled; }

    /**
     * @brief Check if throttling is enabled
     */
    bool IsEnabled() const { return _enabled; }

private:
    // ========================================================================
    // INTERNAL CALCULATIONS
    // ========================================================================

    /**
     * @brief Find nearest human player and update distance
     */
    void UpdateNearestHumanDistance();

    /**
     * @brief Calculate throttle tier from current state
     */
    ThrottleTier CalculateThrottleTier() const;

    /**
     * @brief Calculate effective update interval from tier
     */
    uint32 CalculateEffectiveInterval(ThrottleTier tier) const;

    /**
     * @brief Classify current bot activity
     */
    AIActivityType ClassifyActivity() const;

    /**
     * @brief Check if bot is in a group with human leader
     */
    bool IsInGroupWithHumanLeader() const;

    // ========================================================================
    // MEMBER VARIABLES
    // ========================================================================

    Player* _bot;                       // The bot being throttled
    BotAI* _ai;                         // The bot's AI

    ThrottleConfig _config;             // Throttle configuration
    ThrottleMetrics _metrics;           // Performance metrics

    ThrottleTier _currentTier;          // Current throttle tier
    uint32 _effectiveInterval;          // Current effective update interval

    float _nearestHumanDistance;        // Distance to nearest human player
    ObjectGuid _nearestHumanGuid;       // GUID of nearest human player

    AIActivityType _currentActivity;       // Current activity classification
    bool _inCombat;                      // Whether bot is in combat

    // Timing
    uint32 _timeSinceLastUpdate;        // Time accumulator for throttling
    uint32 _timeSinceProximityCheck;    // Time since last human proximity check
    uint32 _timeSinceActivityCheck;     // Time since last activity classification

    bool _forceNextUpdate;              // Force next update flag
    bool _enabled;                       // Whether throttling is enabled
};

// ============================================================================
// GLOBAL THROTTLE STATISTICS (Optional - for monitoring all bots)
// ============================================================================

/**
 * @brief Singleton for tracking global throttle statistics
 */
class GlobalThrottleStatistics
{
public:
    static GlobalThrottleStatistics& Instance()
    {
        static GlobalThrottleStatistics instance;
        return instance;
    }

    // Aggregate metrics
    void RecordUpdateSkipped() { _totalSkipped.fetch_add(1, ::std::memory_order_relaxed); }
    void RecordUpdateProcessed() { _totalProcessed.fetch_add(1, ::std::memory_order_relaxed); }

    uint64 GetTotalSkipped() const { return _totalSkipped.load(::std::memory_order_relaxed); }
    uint64 GetTotalProcessed() const { return _totalProcessed.load(::std::memory_order_relaxed); }

    float GetSkipRate() const
    {
        uint64 total = _totalSkipped + _totalProcessed;
        return total > 0 ? static_cast<float>(_totalSkipped) / total : 0.0f;
    }

    void Reset()
    {
        _totalSkipped.store(0, ::std::memory_order_relaxed);
        _totalProcessed.store(0, ::std::memory_order_relaxed);
    }

private:
    GlobalThrottleStatistics() : _totalSkipped(0), _totalProcessed(0) {}

    ::std::atomic<uint64> _totalSkipped;
    ::std::atomic<uint64> _totalProcessed;
};

} // namespace Playerbot

#endif // PLAYERBOT_AI_ADAPTIVE_AI_UPDATE_THROTTLER_H
