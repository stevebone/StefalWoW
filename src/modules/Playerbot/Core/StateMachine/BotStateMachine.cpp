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

#include "BotStateMachine.h"
#include "Log.h"
#include <algorithm>
#include <sstream>

namespace Playerbot::StateMachine
{

BotStateMachine::BotStateMachine(Player* bot, BotInitState initialState, TransitionPolicy policy)
    : m_bot(bot)
    , m_policy(policy)
    , m_lastStateChangeTime(::std::chrono::steady_clock::now())
    , m_lastUpdateTime(::std::chrono::steady_clock::now())
{
    // Initialize state info
    m_stateInfo.current.store(initialState, ::std::memory_order_release);
    m_stateInfo.previous.store(BotInitState::CREATED, ::std::memory_order_release);
    m_stateInfo.flags.store(StateFlags::NONE, ::std::memory_order_release);
    m_stateInfo.transitionTime = ::std::chrono::steady_clock::now();
    m_stateInfo.entryTime = m_stateInfo.transitionTime;

    // Initialize transition history with empty events
    for (auto& event : m_transitionHistory)
    {
        event = TransitionEvent{};
    }

    if (m_loggingEnabled)
    {
        TC_LOG_DEBUG("bot.statemachine", "BotStateMachine created for bot {} with initial state {}",
            m_bot ? m_bot->GetName() : "null", GetStateName(initialState));
    }
}

BotStateMachine::~BotStateMachine()
{
    if (m_loggingEnabled && m_totalTransitions > 0)
    {
        uint64 avgTimeMicros = m_totalTransitionTimeMicros / m_totalTransitions;
        TC_LOG_DEBUG("bot.statemachine", "BotStateMachine destroyed. Total transitions: {}, Avg time: {}μs",
            m_totalTransitions.load(), avgTimeMicros);
    }
}

// ========================================================================
// STATE QUERIES
// ========================================================================

BotInitState BotStateMachine::GetCurrentState() const
{
    return m_stateInfo.current.load(::std::memory_order_acquire);
}

BotInitState BotStateMachine::GetPreviousState() const
{
    return m_stateInfo.previous.load(::std::memory_order_acquire);
}

bool BotStateMachine::IsInState(BotInitState state) const
{
    return GetCurrentState() == state;
}

bool BotStateMachine::IsInAnyState(const ::std::vector<BotInitState>& states) const
{
    BotInitState current = GetCurrentState();
    return ::std::any_of(states.begin(), states.end(),
        [current](BotInitState state) { return state == current; });
}

bool BotStateMachine::HasFlags(StateFlags flags) const
{
    StateFlags currentFlags = m_stateInfo.flags.load(::std::memory_order_acquire);
    return (static_cast<uint32>(currentFlags) & static_cast<uint32>(flags)) == static_cast<uint32>(flags);
}

uint32 BotStateMachine::GetRetryCount() const
{
    return m_retryCount.load(::std::memory_order_acquire);
}

// ========================================================================
// STATE TRANSITIONS
// ========================================================================

TransitionValidation BotStateMachine::TransitionTo(BotInitState newState, ::std::string_view reason, bool force)
{
    return TransitionInternal(newState, reason, ::std::nullopt, force);
}

TransitionValidation BotStateMachine::TransitionOnEvent(EventType event, BotInitState newState, ::std::string_view reason)
{
    return TransitionInternal(newState, reason, event, false);
}

TransitionValidation BotStateMachine::ForceTransition(BotInitState newState, ::std::string_view reason)
{
    if (m_loggingEnabled)
    {
        TC_LOG_WARNING("bot.statemachine", "Forcing transition from {} to {} for bot {}: {}",
            GetStateName(GetCurrentState()), GetStateName(newState),
            m_bot ? m_bot->GetName() : "null", reason);
    }

    return TransitionInternal(newState, reason, ::std::nullopt, true);
}

TransitionValidation BotStateMachine::Reset()
{
    return TransitionTo(BotInitState::CREATED, "State machine reset");
}

TransitionValidation BotStateMachine::TransitionInternal(
    BotInitState newState,
    ::std::string_view reason,
    ::std::optional<EventType> event,
    bool force)
{
    // Measure transition time
    auto startTime = ::std::chrono::high_resolution_clock::now();

    ::std::lock_guard lock(m_stateMutex);

    BotInitState currentState = GetCurrentState();

    // Check if already in target state
    if (currentState == newState && !force)
    {
        return { StateTransitionResult::ALREADY_IN_STATE, "Already in target state" };
    }

    // Validate transition unless forced
    if (!force)
    {
        TransitionValidation validation = StateTransitionValidator::ValidateTransition(
            currentState, newState, this);

        if (validation.result != StateTransitionResult::SUCCESS)
        {
            OnTransitionFailed(currentState, newState, validation);

            if (m_loggingEnabled)
            {
                TC_LOG_DEBUG("bot.statemachine", "Transition from {} to {} failed for bot {}: {}",
                    GetStateName(currentState), GetStateName(newState),
                    m_bot ? m_bot->GetName() : "null", validation.reason);
            }

            return validation;
        }
    }

    // Execute OnExit for current state
    OnExit(currentState, newState);

    // Update state atomically
    BotInitState previousState = m_stateInfo.current.exchange(newState, ::std::memory_order_acq_rel);
    m_stateInfo.previous.store(previousState, ::std::memory_order_release);
    m_stateInfo.transitionTime = ::std::chrono::steady_clock::now();
    m_stateInfo.entryTime = m_stateInfo.transitionTime;
    m_lastStateChangeTime = m_stateInfo.transitionTime;

    // Execute OnEnter for new state
    OnEnter(newState, previousState);

    // Create and log transition event
    TransitionEvent transitionEvent;
    transitionEvent.fromState = previousState;
    transitionEvent.toState = newState;
    transitionEvent.eventType = event.value_or(EventType::NONE);
    transitionEvent.timestamp = ::std::chrono::steady_clock::now();
    transitionEvent.success = true;
    transitionEvent.forced = force;
    LogTransition(transitionEvent);

    // Update performance metrics
    auto endTime = ::std::chrono::high_resolution_clock::now();
    auto durationMicros = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime).count();
    m_totalTransitionTimeMicros.fetch_add(durationMicros, ::std::memory_order_relaxed);
    m_totalTransitions.fetch_add(1, ::std::memory_order_relaxed);

    // Reset retry count on successful transition
    m_retryCount.store(0, ::std::memory_order_release);

    if (m_loggingEnabled)
    {
        TC_LOG_DEBUG("bot.statemachine", "Bot {} transitioned from {} to {} ({}μs): {}",
            m_bot ? m_bot->GetName() : "null",
            GetStateName(previousState), GetStateName(newState),
            durationMicros, reason);
    }

    return { StateTransitionResult::SUCCESS, "Transition successful" };
}

// ========================================================================
// STATE FLAGS
// ========================================================================

void BotStateMachine::SetFlags(StateFlags flags)
{
    ::std::lock_guard lock(m_stateMutex);
    StateFlags currentFlags = m_stateInfo.flags.load(::std::memory_order_acquire);
    StateFlags newFlags = static_cast<StateFlags>(
        static_cast<uint32>(currentFlags) | static_cast<uint32>(flags));
    m_stateInfo.flags.store(newFlags, ::std::memory_order_release);

    if (m_loggingEnabled)
    {
        TC_LOG_DEBUG("bot.statemachine", "Bot {} set flags: 0x{:X}",
            m_bot ? m_bot->GetName() : "null", static_cast<uint32>(flags));
    }
}

void BotStateMachine::ClearFlags(StateFlags flags)
{
    ::std::lock_guard lock(m_stateMutex);
    StateFlags currentFlags = m_stateInfo.flags.load(::std::memory_order_acquire);
    StateFlags newFlags = static_cast<StateFlags>(
        static_cast<uint32>(currentFlags) & ~static_cast<uint32>(flags));
    m_stateInfo.flags.store(newFlags, ::std::memory_order_release);

    if (m_loggingEnabled)
    {
        TC_LOG_DEBUG("bot.statemachine", "Bot {} cleared flags: 0x{:X}",
            m_bot ? m_bot->GetName() : "null", static_cast<uint32>(flags));
    }
}

void BotStateMachine::ToggleFlags(StateFlags flags)
{
    ::std::lock_guard lock(m_stateMutex);
    StateFlags currentFlags = m_stateInfo.flags.load(::std::memory_order_acquire);
    StateFlags newFlags = static_cast<StateFlags>(
        static_cast<uint32>(currentFlags) ^ static_cast<uint32>(flags));
    m_stateInfo.flags.store(newFlags, ::std::memory_order_release);

    if (m_loggingEnabled)
    {
        TC_LOG_DEBUG("bot.statemachine", "Bot {} toggled flags: 0x{:X}",
            m_bot ? m_bot->GetName() : "null", static_cast<uint32>(flags));
    }
}

// ========================================================================
// HISTORY & DIAGNOSTICS
// ========================================================================

::std::vector<TransitionEvent> BotStateMachine::GetTransitionHistory() const
{
    ::std::lock_guard lock(m_stateMutex);
    ::std::vector<TransitionEvent> history;

    // Build history from circular buffer
    for (uint32 i = 0; i < 10; ++i)
    {
        uint32 index = (m_historyIndex + i) % 10;
        const TransitionEvent& event = m_transitionHistory[index];

        // Skip uninitialized entries
    if (event.timestamp != ::std::chrono::steady_clock::time_point{})
        {
            history.push_back(event);
        }
    }

    // Sort by timestamp (oldest first)
    ::std::sort(history.begin(), history.end(),
        [](const TransitionEvent& a, const TransitionEvent& b) {
            return a.timestamp < b.timestamp;
        });
    return history;
}

::std::optional<TransitionEvent> BotStateMachine::GetLastTransition() const
{
    ::std::lock_guard lock(m_stateMutex);

    if (m_historyIndex == 0)
    {
        // Check if we have any transitions
    if (m_transitionHistory[9].timestamp == ::std::chrono::steady_clock::time_point{})
            return ::std::nullopt;
        return m_transitionHistory[9];
    }

    return m_transitionHistory[m_historyIndex - 1];
}

uint32 BotStateMachine::GetTimeInCurrentState() const
{
    auto now = ::std::chrono::steady_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(now - m_lastStateChangeTime);
    return static_cast<uint32>(duration.count());
}

uint64 BotStateMachine::GetTransitionCount() const
{
    return m_totalTransitions.load(::std::memory_order_acquire);
}

void BotStateMachine::DumpState() const
{
    ::std::lock_guard lock(m_stateMutex);

    TC_LOG_INFO("bot.statemachine", "=== State Machine Dump for Bot {} ===",
        m_bot ? m_bot->GetName() : "null");
    TC_LOG_INFO("bot.statemachine", "Current State: {} ({}ms in state)",
        GetStateName(GetCurrentState()), GetTimeInCurrentState());
    TC_LOG_INFO("bot.statemachine", "Previous State: {}", GetStateName(GetPreviousState()));
    TC_LOG_INFO("bot.statemachine", "Flags: 0x{:X}", static_cast<uint32>(m_stateInfo.flags.load()));
    TC_LOG_INFO("bot.statemachine", "Policy: {}",
        m_policy == TransitionPolicy::STRICT ? "STRICT" :
        m_policy == TransitionPolicy::RELAXED ? "RELAXED" : "DEBUGGING");
    TC_LOG_INFO("bot.statemachine", "Total Transitions: {}", m_totalTransitions.load());

    if (m_totalTransitions > 0)
    {
        uint64 avgTimeMicros = m_totalTransitionTimeMicros / m_totalTransitions;
        TC_LOG_INFO("bot.statemachine", "Avg Transition Time: {}μs", avgTimeMicros);
    }

    TC_LOG_INFO("bot.statemachine", "Retry Count: {}", m_retryCount.load());

    // Dump transition history
    auto history = GetTransitionHistory();
    if (!history.empty())
    {
        TC_LOG_INFO("bot.statemachine", "=== Last {} Transitions ===", history.size());
        for (const auto& event : history)
        {
            auto timeSinceTransition = ::std::chrono::duration_cast<::std::chrono::seconds>(
                ::std::chrono::steady_clock::now() - event.timestamp).count();

            TC_LOG_INFO("bot.statemachine", "  {} -> {} ({}s ago, {}, {})",
                GetStateName(event.fromState), GetStateName(event.toState),
                timeSinceTransition,
                event.success ? "SUCCESS" : "FAILED",
                event.forced ? "FORCED" : "VALIDATED");
        }
    }
}

// ========================================================================
// POLICY & CONFIGURATION
// ========================================================================

void BotStateMachine::SetPolicy(TransitionPolicy policy)
{
    ::std::lock_guard lock(m_stateMutex);
    TransitionPolicy oldPolicy = m_policy;
    m_policy = policy;

    if (m_loggingEnabled)
    {
        TC_LOG_DEBUG("bot.statemachine", "Bot {} transition policy changed from {} to {}",
            m_bot ? m_bot->GetName() : "null",
            oldPolicy == TransitionPolicy::STRICT ? "STRICT" :
            oldPolicy == TransitionPolicy::RELAXED ? "RELAXED" : "DEBUGGING",
            policy == TransitionPolicy::STRICT ? "STRICT" :
            policy == TransitionPolicy::RELAXED ? "RELAXED" : "DEBUGGING");
    }
}

// ========================================================================
// PROTECTED VIRTUAL METHODS
// ========================================================================

void BotStateMachine::OnEnter(BotInitState newState, BotInitState previousState)
{
    // Default implementation logs entry
    if (m_loggingEnabled)
    {
        TC_LOG_DEBUG("bot.statemachine", "Bot {} entered state {} from {}",
            m_bot ? m_bot->GetName() : "null",
            GetStateName(newState), GetStateName(previousState));
    }
}

void BotStateMachine::OnExit(BotInitState currentState, BotInitState nextState)
{
    // Default implementation logs exit
    if (m_loggingEnabled)
    {
        TC_LOG_DEBUG("bot.statemachine", "Bot {} exiting state {} to {}",
            m_bot ? m_bot->GetName() : "null",
            GetStateName(currentState), GetStateName(nextState));
    }
}

void BotStateMachine::OnTransitionFailed(BotInitState from, BotInitState to, TransitionValidation result)
{
    // Increment retry counter
    m_retryCount.fetch_add(1, ::std::memory_order_relaxed);

    // Default implementation logs failure
    if (m_loggingEnabled)
    {
        const char* resultStr = "UNKNOWN";
        switch (result.result)
        {
            case StateTransitionResult::INVALID_TRANSITION:
                resultStr = "INVALID_TRANSITION";
                break;
            case StateTransitionResult::PRECONDITION_FAILED:
                resultStr = "PRECONDITION_FAILED";
                break;
            case StateTransitionResult::BOT_NULL:
                resultStr = "BOT_NULL";
                break;
            case StateTransitionResult::ALREADY_IN_STATE:
                resultStr = "ALREADY_IN_STATE";
                break;
            case StateTransitionResult::LOCKED:
                resultStr = "LOCKED";
                break;
            default:
                break;
        }

        TC_LOG_WARNING("bot.statemachine", "Bot {} transition failed from {} to {}: {} - {}",
            m_bot ? m_bot->GetName() : "null",
            GetStateName(from), GetStateName(to),
            resultStr, result.reason);
    }
}

void BotStateMachine::OnUpdate(uint32 diff)
{
    // Default implementation does nothing
    // Derived classes can override for state-specific updates
    m_lastUpdateTime = ::std::chrono::steady_clock::now();
}

// ========================================================================
// PROTECTED HELPERS
// ========================================================================

void BotStateMachine::LogTransition(const TransitionEvent& transitionEvent)
{
    // Add to circular buffer
    m_transitionHistory[m_historyIndex] = transitionEvent;
    m_historyIndex = (m_historyIndex + 1) % 10;
}

const char* BotStateMachine::GetStateName(BotInitState state)
{
    switch (state)
    {
        case BotInitState::CREATED:
            return "CREATED";
        case BotInitState::LOADING_FROM_DB:
            return "LOADING_FROM_DB";
        case BotInitState::CREATING_IN_DB:
            return "CREATING_IN_DB";
        case BotInitState::CHAR_ENUM_PENDING:
            return "CHAR_ENUM_PENDING";
        case BotInitState::VALIDATING_LOGIN:
            return "VALIDATING_LOGIN";
        case BotInitState::WORLD_INIT_PENDING:
            return "WORLD_INIT_PENDING";
        case BotInitState::LOADING_INVENTORY:
            return "LOADING_INVENTORY";
        case BotInitState::LOADING_SKILLS:
            return "LOADING_SKILLS";
        case BotInitState::LOADING_SPELLS:
            return "LOADING_SPELLS";
        case BotInitState::LOADING_QUESTS:
            return "LOADING_QUESTS";
        case BotInitState::WORLD_ENTERING:
            return "WORLD_ENTERING";
        case BotInitState::MAP_LOADING:
            return "MAP_LOADING";
        case BotInitState::SPAWNING:
            return "SPAWNING";
        case BotInitState::READY:
            return "READY";
        case BotInitState::FAILED:
            return "FAILED";
        case BotInitState::DISCONNECTING:
            return "DISCONNECTING";
        case BotInitState::DISCONNECTED:
            return "DISCONNECTED";
        case BotInitState::CLEANUP:
            return "CLEANUP";
        case BotInitState::DESTROYED:
            return "DESTROYED";
        default:
            return "UNKNOWN";
    }
}

} // namespace Playerbot::StateMachine