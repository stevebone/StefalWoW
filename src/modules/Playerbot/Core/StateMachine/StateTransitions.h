/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "BotStateTypes.h"
#include "Common.h"
#include <array>
#include <functional>
#include <optional>
#include <vector>
#include <string>

namespace Playerbot::StateMachine {

// Forward declarations
class BotStateMachine;
class Player;

/**
 * @enum TransitionPolicy
 * @brief Defines how strict the state machine is with transitions
 */
enum class TransitionPolicy : uint8_t {
    STRICT,        // Only allow transitions in INIT_STATE_TRANSITIONS table
    RELAXED,       // Allow any transition but log warnings for invalid ones
    DEBUGGING      // Allow any transition, verbose logging
};

/**
 * @struct StateTransitionRule
 * @brief Defines a valid state transition with preconditions
 */
struct StateTransitionRule {
    BotInitState fromState;
    BotInitState toState;
    ::std::string_view description;
    Priority priority;

    // Precondition function - returns true if transition is allowed
    // Takes BotStateMachine* for context (bot, group, etc.)
    ::std::function<bool(const BotStateMachine*)> precondition;

    // Optional: Event that should trigger this transition
    ::std::optional<EventType> triggerEvent;

    // Whether this transition can be forced (bypassing preconditions)
    bool allowForce;
};

/**
 * @struct TransitionEvent
 * @brief Data associated with a state transition
 */
struct TransitionEvent {
    BotInitState fromState;
    BotInitState toState;
    EventType triggerEvent;
    uint64_t timestamp;      // GameTime::GetGameTimeMS()
    ::std::string_view reason; // Human-readable reason
    bool wasForced;          // True if transition was forced

    // Performance tracking
    uint32_t transitionDurationMicros;
    uint32_t preconditionCheckMicros;
};

/**
 * @struct TransitionValidation
 * @brief Result of transition validation
 */
struct TransitionValidation {
    StateTransitionResult result;
    ::std::string failureReason;
    const StateTransitionRule* rule;
    bool preconditionPassed;
};

/**
 * @brief Complete state transition rules for BotInitState
 *
 * This table defines the ONLY valid transitions. Any transition not in this
 * table will be rejected as INVALID_TRANSITION.
 *
 * State Machine Diagram:
 *
 *   CREATED
 *      ↓
 *   LOADING_CHARACTER
 *      ↓
 *   IN_WORLD  ←
 *      ↓                    
 *   CHECKING_GROUP          (retry on failure)
 *      ↓                    
 *   ACTIVATING_STRATEGIES  
 *      ↓                    
 *   READY 
 *      ↓
 *   (any state can go to FAILED on error)
 *
 * Example usage:
 * @code
 *   auto validation = StateTransitionValidator::ValidateTransition(
 *       BotInitState::CREATED,
 *       BotInitState::LOADING_CHARACTER,
 *       stateMachine);
 *   if (validation.result == StateTransitionResult::SUCCESS) {
 *       // Perform transition
 *   }
 * @endcode
 */
constexpr ::std::array<StateTransitionRule, 20> INIT_STATE_TRANSITIONS = {{
    // Transition 1: CREATED → LOADING_CHARACTER
    {
        BotInitState::CREATED,
        BotInitState::LOADING_CHARACTER,
        "Begin character data loading from database",
        PRIORITY_HIGH,
        nullptr, // No precondition (always allowed)
        EventType::BOT_CREATED,
        false // Cannot force
    },

    // Transition 2: LOADING_CHARACTER → IN_WORLD
    {
        BotInitState::LOADING_CHARACTER,
        BotInitState::IN_WORLD,
        "Character data loaded, bot added to world",
        PRIORITY_HIGH,
        [](const BotStateMachine* sm) {
            // Precondition: Bot must be IsInWorld()
            return sm && sm->GetBot() && sm->GetBot()->IsInWorld();
        },
        EventType::BOT_ADDED_TO_WORLD,
        false
    },

    // Transition 3: IN_WORLD → CHECKING_GROUP
    {
        BotInitState::IN_WORLD,
        BotInitState::CHECKING_GROUP,
        "Check for existing group membership",
        PRIORITY_NORMAL,
        [](const BotStateMachine* sm) {
            // Precondition: Bot must be in world and alive
            return sm && sm->GetBot() &&
                   sm->GetBot()->IsInWorld() &&
                   sm->GetBot()->IsAlive();
        },
        ::std::nullopt, // No specific event
        false
    },

    // Transition 4: CHECKING_GROUP → ACTIVATING_STRATEGIES
    {
        BotInitState::CHECKING_GROUP,
        BotInitState::ACTIVATING_STRATEGIES,
        "Activate strategies (follow if in group, idle otherwise)",
        PRIORITY_NORMAL,
        [](const BotStateMachine* sm) {
            // Always allowed after group check
            return true;
        },
        ::std::nullopt,
        false
    },

    // Transition 5: ACTIVATING_STRATEGIES → READY
    {
        BotInitState::ACTIVATING_STRATEGIES,
        BotInitState::READY,
        "Bot fully initialized and operational",
        PRIORITY_NORMAL,
        [](const BotStateMachine* sm) {
            // Precondition: Bot AI must be initialized
            return sm && sm->GetBot() && sm->GetBot()->GetBotAI() &&
                   sm->GetBot()->GetBotAI()->IsInitialized();
        },
        ::std::nullopt,
        false
    },

    // Error transitions (any state → FAILED)

    // Transition 6: CREATED → FAILED
    {
        BotInitState::CREATED,
        BotInitState::FAILED,
        "Failed to create bot session",
        PRIORITY_CRITICAL,
        nullptr, // Always allowed
        EventType::ERROR_OCCURRED,
        true // Can force
    },

    // Transition 7: LOADING_CHARACTER → FAILED
    {
        BotInitState::LOADING_CHARACTER,
        BotInitState::FAILED,
        "Failed to load character data from database",
        PRIORITY_CRITICAL,
        nullptr, // Always allowed
        EventType::ERROR_OCCURRED,
        true // Can force
    },

    // Transition 8: IN_WORLD → FAILED
    {
        BotInitState::IN_WORLD,
        BotInitState::FAILED,
        "Failed during world initialization",
        PRIORITY_CRITICAL,
        nullptr, // Always allowed
        EventType::ERROR_OCCURRED,
        true // Can force
    },

    // Transition 9: CHECKING_GROUP → FAILED
    {
        BotInitState::CHECKING_GROUP,
        BotInitState::FAILED,
        "Failed during group check",
        PRIORITY_CRITICAL,
        nullptr, // Always allowed
        EventType::ERROR_OCCURRED,
        true // Can force
    },

    // Transition 10: ACTIVATING_STRATEGIES → FAILED
    {
        BotInitState::ACTIVATING_STRATEGIES,
        BotInitState::FAILED,
        "Failed to activate AI strategies",
        PRIORITY_CRITICAL,
        nullptr, // Always allowed
        EventType::ERROR_OCCURRED,
        true // Can force
    },

    // Transition 11: READY → FAILED
    {
        BotInitState::READY,
        BotInitState::FAILED,
        "Bot encountered critical error during operation",
        PRIORITY_CRITICAL,
        nullptr, // Always allowed
        EventType::ERROR_OCCURRED,
        true // Can force
    },

    // Transition 12: FAILED → LOADING_CHARACTER (retry)
    {
        BotInitState::FAILED,
        BotInitState::LOADING_CHARACTER,
        "Retry initialization after failure",
        PRIORITY_LOW,
        [](const BotStateMachine* sm) {
            // Limit retry count
            return sm && sm->GetRetryCount() < 3;
        },
        ::std::nullopt,
        true // Can force retry
    },

    // Recovery transitions (for error recovery paths)

    // Transition 13: FAILED → CREATED (full reset)
    {
        BotInitState::FAILED,
        BotInitState::CREATED,
        "Full reset after catastrophic failure",
        PRIORITY_LOW,
        [](const BotStateMachine* sm) {
            // Only allow if retry count exceeded
            return sm && sm->GetRetryCount() >= 3;
        },
        ::std::nullopt,
        true // Can force reset
    },

    // Transition 14: READY → IN_WORLD (soft reset)
    {
        BotInitState::READY,
        BotInitState::IN_WORLD,
        "Soft reset to recheck group and strategies",
        PRIORITY_NORMAL,
        [](const BotStateMachine* sm) {
            // Allow if bot needs reconfiguration
            return sm && sm->GetBot() && sm->GetBot()->IsInWorld();
        },
        EventType::BOT_RESET_REQUEST,
        true // Can force
    },

    // Transition 15: CHECKING_GROUP → IN_WORLD (retry group check)
    {
        BotInitState::CHECKING_GROUP,
        BotInitState::IN_WORLD,
        "Retry group check after timeout",
        PRIORITY_LOW,
        [](const BotStateMachine* sm) {
            // Allow retry if timeout exceeded
            return sm && sm->GetTimeInState() > 5000; // 5 seconds
        },
        ::std::nullopt,
        false
    },

    // Transition 16: ACTIVATING_STRATEGIES → CHECKING_GROUP (retry strategy)
    {
        BotInitState::ACTIVATING_STRATEGIES,
        BotInitState::CHECKING_GROUP,
        "Retry strategy activation with group recheck",
        PRIORITY_LOW,
        [](const BotStateMachine* sm) {
            // Allow if strategy activation failed
            return sm && !sm->GetBot()->GetBotAI()->IsInitialized();
        },
        ::std::nullopt,
        false
    },

    // Special transitions for disconnection/removal

    // Transition 17: READY → CREATED (bot removed from world)
    {
        BotInitState::READY,
        BotInitState::CREATED,
        "Bot removed from world, awaiting re-initialization",
        PRIORITY_HIGH,
        [](const BotStateMachine* sm) {
            // Bot must be removed from world
            return sm && sm->GetBot() && !sm->GetBot()->IsInWorld();
        },
        EventType::BOT_REMOVED_FROM_WORLD,
        false
    },

    // Transition 18: IN_WORLD → CREATED (immediate removal)
    {
        BotInitState::IN_WORLD,
        BotInitState::CREATED,
        "Bot removed before group check",
        PRIORITY_HIGH,
        [](const BotStateMachine* sm) {
            // Bot must be removed from world
            return sm && sm->GetBot() && !sm->GetBot()->IsInWorld();
        },
        EventType::BOT_REMOVED_FROM_WORLD,
        false
    },

    // Transition 19: LOADING_CHARACTER → FAILED (timeout)
    {
        BotInitState::LOADING_CHARACTER,
        BotInitState::FAILED,
        "Character loading timeout exceeded",
        PRIORITY_HIGH,
        [](const BotStateMachine* sm) {
            // Timeout after 10 seconds
            return sm && sm->GetTimeInState() > 10000;
        },
        ::std::nullopt,
        false
    },

    // Transition 20: NONE → CREATED (initial state)
    {
        BotInitState::NONE,
        BotInitState::CREATED,
        "Initial bot creation",
        PRIORITY_CRITICAL,
        nullptr, // Always allowed
        EventType::BOT_CREATED,
        false
    }
}};

/**
 * @class StateTransitionValidator
 * @brief Validates state transitions and provides diagnostic information
 *
 * Thread-safe: All methods are stateless and can be called concurrently
 */
class TC_GAME_API StateTransitionValidator {
public:
    /**
     * @brief Check if a transition is valid
     * @param from Current state
     * @param to Target state
     * @param context State machine context for precondition checks
     * @return Validation result with reason
     */
    static TransitionValidation ValidateTransition(
        BotInitState from,
        BotInitState to,
        const BotStateMachine* context
    );

    /**
     * @brief Find the transition rule for a given state pair
     * @param from Current state
     * @param to Target state
     * @return Pointer to rule if found, nullptr otherwise
     */
    static const StateTransitionRule* FindTransitionRule(
        BotInitState from,
        BotInitState to
    );

    /**
     * @brief Get all valid transitions from a given state
     * @param from Current state
     * @return Vector of possible target states
     */
    static ::std::vector<BotInitState> GetValidTransitions(BotInitState from);

    /**
     * @brief Check if a transition can be forced
     * @param from Current state
     * @param to Target state
     * @return true if transition supports forcing
     */
    static bool CanForceTransition(BotInitState from, BotInitState to);

    /**
     * @brief Get human-readable description of why transition failed
     * @param result Validation result
     * @param from Current state
     * @param to Target state
     * @return Detailed error message
     */
    static ::std::string GetFailureReason(
        StateTransitionResult result,
        BotInitState from,
        BotInitState to
    );

    /**
     * @brief Get the transition policy for the current environment
     * @return Current transition policy
     */
    static TransitionPolicy GetTransitionPolicy();

    /**
     * @brief Set the transition policy (for testing/debugging)
     * @param policy New policy to apply
     */
    static void SetTransitionPolicy(TransitionPolicy policy);

private:
    static inline TransitionPolicy s_transitionPolicy = TransitionPolicy::STRICT;
};

} // namespace Playerbot::StateMachine