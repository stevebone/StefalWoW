/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * SESSION TRANSITIONS
 *
 * Phase 3: Humanization Core
 *
 * Handles intelligent activity transitions with:
 * - Transition rules and restrictions
 * - Natural flow between activities
 * - Context-aware transition timing
 * - Personality-based transition preferences
 */

#pragma once

#include "Define.h"
#include "../Core/ActivityType.h"
#include "../Core/PersonalityProfile.h"
#include "ActivitySession.h"
#include "ObjectGuid.h"
#include "Position.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>

class Player;

namespace Playerbot
{
namespace Humanization
{

/**
 * @brief Transition state
 */
enum class TransitionState : uint8
{
    NONE = 0,           // No transition in progress
    WRAP_UP,            // Wrapping up current activity
    TRAVEL,             // Traveling to new location
    PREPARATION,        // Preparing for new activity
    READY,              // Ready to start new activity
    COMPLETED,          // Transition completed
    FAILED              // Transition failed
};

/**
 * @brief Why a transition was blocked
 */
enum class TransitionBlockReason : uint8
{
    NONE = 0,
    IN_COMBAT,
    IN_DUNGEON,
    IN_BATTLEGROUND,
    DEAD,
    IN_VEHICLE,
    ACTIVITY_LOCKED,        // Current activity requires completion
    COOLDOWN,               // Too soon since last transition
    CONTEXT_MISMATCH,       // Target activity not available in context
    TRAVEL_IMPOSSIBLE       // Can't reach destination
};

/**
 * @brief Transition rule definition
 */
struct TransitionRule
{
    ActivityType fromActivity;
    ActivityType toActivity;
    uint8 priority;                 // Higher = more preferred (0-100)
    uint32 minWrapUpMs;             // Minimum wrap-up time
    uint32 maxWrapUpMs;             // Maximum wrap-up time
    uint32 prepTimeMs;              // Preparation time
    bool requiresTravel;            // Does transition need travel?
    bool allowInterrupt;            // Can this transition be interrupted?
    std::string transitionName;     // Human-readable name

    TransitionRule()
        : fromActivity(ActivityType::NONE)
        , toActivity(ActivityType::NONE)
        , priority(50)
        , minWrapUpMs(1000)
        , maxWrapUpMs(5000)
        , prepTimeMs(2000)
        , requiresTravel(false)
        , allowInterrupt(true)
    {}
};

/**
 * @brief Active transition tracking
 */
struct ActiveTransition
{
    ActivityType fromActivity;
    ActivityType toActivity;
    TransitionState state;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point stateStartTime;
    uint32 wrapUpDurationMs;
    uint32 travelDurationMs;
    uint32 prepDurationMs;
    Position targetPosition;
    bool isForced;

    ActiveTransition()
        : fromActivity(ActivityType::NONE)
        , toActivity(ActivityType::NONE)
        , state(TransitionState::NONE)
        , wrapUpDurationMs(0)
        , travelDurationMs(0)
        , prepDurationMs(0)
        , isForced(false)
    {}

    uint32 GetElapsedMs() const
    {
        auto now = std::chrono::steady_clock::now();
        return static_cast<uint32>(
            std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count());
    }

    uint32 GetStateElapsedMs() const
    {
        auto now = std::chrono::steady_clock::now();
        return static_cast<uint32>(
            std::chrono::duration_cast<std::chrono::milliseconds>(now - stateStartTime).count());
    }
};

/**
 * @brief Transition flow pattern
 *
 * Defines common activity sequences that feel natural.
 */
struct TransitionFlowPattern
{
    std::string patternName;
    std::vector<ActivityType> sequence;
    uint8 weight;                   // Selection weight (0-100)
    PersonalityType preferredBy;    // Which personality prefers this

    TransitionFlowPattern()
        : weight(50)
        , preferredBy(PersonalityType::CASUAL)
    {}
};

/**
 * @brief Session Transitions
 *
 * Manages the logic of transitioning between activities:
 * - Validates transition requests
 * - Determines transition timing
 * - Handles wrap-up, travel, and preparation phases
 * - Suggests natural next activities based on context
 *
 * **Thread Safety:** Per-bot instance, no shared state.
 */
class TC_GAME_API SessionTransitions
{
public:
    /**
     * @brief Construct transitions manager for a bot
     * @param bot The player bot this manager serves
     */
    explicit SessionTransitions(Player* bot);

    ~SessionTransitions();

    // Non-copyable
    SessionTransitions(SessionTransitions const&) = delete;
    SessionTransitions& operator=(SessionTransitions const&) = delete;

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /**
     * @brief Initialize transition system
     */
    void Initialize();

    /**
     * @brief Update active transition
     * @param diff Time since last update in milliseconds
     * @return true if transition completed this update
     */
    bool Update(uint32 diff);

    /**
     * @brief Shutdown and cleanup
     */
    void Shutdown();

    // ========================================================================
    // TRANSITION CONTROL
    // ========================================================================

    /**
     * @brief Start a transition to a new activity
     * @param fromActivity Current activity (NONE if idle)
     * @param toActivity Target activity
     * @param forced Force transition even if restrictions apply
     * @return true if transition started
     */
    bool StartTransition(ActivityType fromActivity, ActivityType toActivity, bool forced = false);

    /**
     * @brief Cancel active transition
     */
    void CancelTransition();

    /**
     * @brief Is a transition in progress?
     */
    bool IsTransitioning() const { return _activeTransition.state != TransitionState::NONE; }

    /**
     * @brief Get active transition state
     */
    TransitionState GetTransitionState() const { return _activeTransition.state; }

    /**
     * @brief Get active transition details
     */
    ActiveTransition const& GetActiveTransition() const { return _activeTransition; }

    /**
     * @brief Is transition ready to complete?
     */
    bool IsTransitionReady() const { return _activeTransition.state == TransitionState::READY; }

    /**
     * @brief Complete the transition (should be called when READY)
     */
    void CompleteTransition();

    // ========================================================================
    // VALIDATION
    // ========================================================================

    /**
     * @brief Can we transition from one activity to another?
     * @param fromActivity Current activity
     * @param toActivity Target activity
     * @return true if transition is allowed
     */
    bool CanTransition(ActivityType fromActivity, ActivityType toActivity) const;

    /**
     * @brief Check why a transition is blocked
     * @param fromActivity Current activity
     * @param toActivity Target activity
     * @return Block reason or NONE if allowed
     */
    TransitionBlockReason GetBlockReason(ActivityType fromActivity, ActivityType toActivity) const;

    /**
     * @brief Is the bot in a state that blocks all transitions?
     */
    bool IsTransitionBlocked() const;

    /**
     * @brief Get the reason transitions are blocked
     */
    TransitionBlockReason GetGlobalBlockReason() const;

    // ========================================================================
    // SUGGESTIONS
    // ========================================================================

    /**
     * @brief Get suggested next activity based on context
     * @param currentActivity Current activity (NONE if idle)
     * @param personality Bot's personality
     * @return Suggested next activity
     */
    ActivityType SuggestNextActivity(
        ActivityType currentActivity,
        PersonalityProfile const& personality) const;

    /**
     * @brief Get ranked list of possible next activities
     * @param currentActivity Current activity
     * @param personality Bot's personality
     * @param maxResults Maximum results to return
     * @return Vector of (activity, score) pairs, highest score first
     */
    std::vector<std::pair<ActivityType, float>> GetRankedNextActivities(
        ActivityType currentActivity,
        PersonalityProfile const& personality,
        uint32 maxResults = 5) const;

    /**
     * @brief Should the bot take a break based on activity history?
     * @param recentActivities Recent activity types
     * @param personality Bot's personality
     * @return true if break is recommended
     */
    bool ShouldTakeBreak(
        std::vector<ActivityType> const& recentActivities,
        PersonalityProfile const& personality) const;

    // ========================================================================
    // TIMING
    // ========================================================================

    /**
     * @brief Calculate total transition time
     * @param fromActivity Current activity
     * @param toActivity Target activity
     * @return Total time in milliseconds
     */
    uint32 CalculateTransitionTime(ActivityType fromActivity, ActivityType toActivity) const;

    /**
     * @brief Get wrap-up time for an activity
     * @param activity Activity type
     * @param personality Bot's personality (affects timing)
     * @return Wrap-up time in milliseconds
     */
    uint32 GetWrapUpTime(ActivityType activity, PersonalityProfile const& personality) const;

    /**
     * @brief Get preparation time for an activity
     * @param activity Activity type
     * @param personality Bot's personality
     * @return Preparation time in milliseconds
     */
    uint32 GetPreparationTime(ActivityType activity, PersonalityProfile const& personality) const;

    // ========================================================================
    // RULES MANAGEMENT
    // ========================================================================

    /**
     * @brief Get transition rule between activities
     * @param fromActivity Current activity
     * @param toActivity Target activity
     * @return Pointer to rule, nullptr if no specific rule
     */
    TransitionRule const* GetRule(ActivityType fromActivity, ActivityType toActivity) const;

    /**
     * @brief Get all outgoing transitions from an activity
     * @param fromActivity Current activity
     * @return Vector of valid target activities
     */
    std::vector<ActivityType> GetValidTargets(ActivityType fromActivity) const;

    /**
     * @brief Set custom transition rule
     * @param rule Rule to add/update
     */
    void SetRule(TransitionRule const& rule);

    // ========================================================================
    // FLOW PATTERNS
    // ========================================================================

    /**
     * @brief Get a recommended flow pattern
     * @param personality Bot's personality
     * @return Selected flow pattern
     */
    TransitionFlowPattern const* GetRecommendedFlow(PersonalityProfile const& personality) const;

    /**
     * @brief Check if current sequence matches a flow pattern
     * @param recentActivities Recent activity sequence
     * @return Matching pattern or nullptr
     */
    TransitionFlowPattern const* MatchFlowPattern(
        std::vector<ActivityType> const& recentActivities) const;

    // ========================================================================
    // METRICS
    // ========================================================================

    struct TransitionMetrics
    {
        std::atomic<uint32> totalTransitions{0};
        std::atomic<uint32> completedTransitions{0};
        std::atomic<uint32> cancelledTransitions{0};
        std::atomic<uint32> failedTransitions{0};
        std::atomic<uint32> blockedAttempts{0};
        std::atomic<uint64> totalTransitionTimeMs{0};

        void Reset()
        {
            totalTransitions = 0;
            completedTransitions = 0;
            cancelledTransitions = 0;
            failedTransitions = 0;
            blockedAttempts = 0;
            totalTransitionTimeMs = 0;
        }
    };

    TransitionMetrics const& GetMetrics() const { return _metrics; }
    static TransitionMetrics const& GetGlobalMetrics() { return _globalMetrics; }

private:
    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /**
     * @brief Initialize default transition rules
     */
    void InitializeDefaultRules();

    /**
     * @brief Initialize flow patterns
     */
    void InitializeFlowPatterns();

    /**
     * @brief Advance to next transition state
     */
    void AdvanceTransitionState();

    /**
     * @brief Process WRAP_UP state
     * @param diff Time since last update
     */
    void ProcessWrapUp(uint32 diff);

    /**
     * @brief Process TRAVEL state
     * @param diff Time since last update
     */
    void ProcessTravel(uint32 diff);

    /**
     * @brief Process PREPARATION state
     * @param diff Time since last update
     */
    void ProcessPreparation(uint32 diff);

    /**
     * @brief Calculate travel time to activity location
     * @param toActivity Target activity
     * @return Estimated travel time in milliseconds
     */
    uint32 EstimateTravelTime(ActivityType toActivity) const;

    /**
     * @brief Get position for an activity
     * @param activity Activity type
     * @return Target position (or current if no travel needed)
     */
    Position GetActivityPosition(ActivityType activity) const;

    /**
     * @brief Calculate activity score for suggestions
     * @param fromActivity Current activity
     * @param toActivity Candidate next activity
     * @param personality Bot's personality
     * @return Score (higher = better)
     */
    float CalculateActivityScore(
        ActivityType fromActivity,
        ActivityType toActivity,
        PersonalityProfile const& personality) const;

    /**
     * @brief Apply personality modifiers to timing
     * @param baseTimeMs Base time in milliseconds
     * @param personality Bot's personality
     * @param isPatient true for patient activities (longer is okay)
     * @return Modified time
     */
    uint32 ApplyPersonalityTiming(
        uint32 baseTimeMs,
        PersonalityProfile const& personality,
        bool isPatient) const;

    // ========================================================================
    // DATA MEMBERS
    // ========================================================================

    // Bot reference
    Player* _bot;
    ObjectGuid _botGuid;

    // Active transition
    ActiveTransition _activeTransition;

    // Transition rules: (from, to) -> rule
    std::unordered_map<uint64, TransitionRule> _rules;

    // Flow patterns
    std::vector<TransitionFlowPattern> _flowPatterns;

    // Last transition time (for cooldown)
    std::chrono::steady_clock::time_point _lastTransitionTime;

    // Initialization flag
    bool _initialized{false};

    // Per-bot metrics
    TransitionMetrics _metrics;

    // Global metrics
    static TransitionMetrics _globalMetrics;

    // Constants
    static constexpr uint32 MIN_TRANSITION_COOLDOWN_MS = 5000;   // 5 seconds between transitions
    static constexpr uint32 DEFAULT_WRAP_UP_MS = 3000;           // 3 seconds default wrap-up
    static constexpr uint32 DEFAULT_PREP_MS = 2000;              // 2 seconds default prep
    static constexpr uint32 DEFAULT_TRAVEL_MS = 30000;           // 30 seconds default travel estimate

    // ========================================================================
    // HELPERS
    // ========================================================================

    /**
     * @brief Generate key for rule lookup
     */
    static uint64 MakeRuleKey(ActivityType from, ActivityType to)
    {
        return (static_cast<uint64>(from) << 32) | static_cast<uint64>(to);
    }
};

} // namespace Humanization
} // namespace Playerbot
