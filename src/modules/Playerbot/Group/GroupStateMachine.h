/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_GROUP_STATE_MACHINE_H
#define PLAYERBOT_GROUP_STATE_MACHINE_H

#include "Define.h"
#include "ObjectGuid.h"
#include <string>
#include <chrono>
#include <unordered_map>
#include <functional>

// Forward declarations
class Group;

namespace Playerbot
{

/**
 * @enum GroupState
 * @brief Defines all possible states a bot's group can be in
 *
 * State machine tracks the lifecycle and configuration of a group from
 * the bot's perspective, allowing appropriate behavior at each stage.
 */
enum class GroupState : uint8
{
    // Lifecycle states
    NOT_IN_GROUP = 0,           // Bot is not in any group
    INVITED,                    // Bot has pending group invite
    FORMING,                    // Group is forming (< 3 members)
    ACTIVE,                     // Group is active (3-5 members)
    RAID_FORMING,               // Converting to raid (5-10 members)
    RAID_ACTIVE,                // Raid is active (10+ members)

    // Special states
    READY_CHECK,                // Ready check in progress
    PREPARING_FOR_COMBAT,       // Group preparing for pull
    IN_COMBAT,                  // Group is in combat
    DISBANDING,                 // Group is disbanding

    // Instance states
    ENTERING_INSTANCE,          // Zoning into instance
    IN_INSTANCE,                // Inside instance
    INSTANCE_COMPLETE,          // Instance cleared

    MAX_STATE
};

/**
 * @enum GroupStateTransition
 * @brief Defines all valid state transitions
 */
enum class GroupStateTransition : uint8
{
    // Invite transitions
    RECEIVE_INVITE,             // NOT_IN_GROUP → INVITED
    ACCEPT_INVITE,              // INVITED → FORMING/ACTIVE
    DECLINE_INVITE,             // INVITED → NOT_IN_GROUP

    // Formation transitions
    MEMBER_JOINED,              // FORMING → ACTIVE (reach 3 members)
    MEMBER_LEFT,                // ACTIVE → FORMING (drop below 3)
    CONVERT_TO_RAID,            // ACTIVE → RAID_FORMING (5+ members)
    RAID_READY,                 // RAID_FORMING → RAID_ACTIVE (10+ members)
    CONVERT_TO_PARTY,           // RAID_ACTIVE → ACTIVE (drop below 10)

    // Combat transitions
    ENTER_COMBAT,               // ACTIVE/RAID_ACTIVE → IN_COMBAT
    LEAVE_COMBAT,               // IN_COMBAT → ACTIVE/RAID_ACTIVE
    READY_CHECK_START,          // ACTIVE/RAID_ACTIVE → READY_CHECK
    READY_CHECK_COMPLETE,       // READY_CHECK → PREPARING_FOR_COMBAT
    READY_FOR_PULL,             // PREPARING_FOR_COMBAT → ACTIVE/RAID_ACTIVE

    // Instance transitions
    ZONE_INTO_INSTANCE,         // ACTIVE/RAID_ACTIVE → ENTERING_INSTANCE
    INSTANCE_ENTERED,           // ENTERING_INSTANCE → IN_INSTANCE
    INSTANCE_CLEARED,           // IN_INSTANCE → INSTANCE_COMPLETE
    LEAVE_INSTANCE,             // IN_INSTANCE/INSTANCE_COMPLETE → ACTIVE/RAID_ACTIVE

    // Disband transitions
    START_DISBAND,              // ANY → DISBANDING
    COMPLETE_DISBAND,           // DISBANDING → NOT_IN_GROUP

    MAX_TRANSITION
};

/**
 * @class GroupStateMachine
 * @brief State machine for tracking bot's group state and transitions
 *
 * This class implements a finite state machine (FSM) to track and validate
 * all group-related state changes. It ensures bots only perform valid
 * actions based on current group state.
 *
 * **Design Pattern**: State Machine Pattern
 * - Each state has defined entry/exit actions
 * - Transitions are validated before execution
 * - State history is maintained for debugging
 * - Guards prevent invalid transitions
 *
 * **Thread Safety**:
 * - State changes are atomic
 * - No shared state between bots
 * - Safe to query from any thread
 *
 * **Performance**:
 * - State transitions: O(1)
 * - State queries: O(1)
 * - Memory per bot: ~500 bytes
 */
class TC_GAME_API GroupStateMachine
{
public:
    /**
     * Constructor
     * @param botGuid GUID of the bot this state machine belongs to
     */
    explicit GroupStateMachine(ObjectGuid botGuid);
    ~GroupStateMachine();

    // ========================================================================
    // STATE QUERIES
    // ========================================================================

    /**
     * Get current group state
     * @return Current state
     */
    GroupState GetState() const { return _currentState; }

    /**
     * Get previous group state
     * @return Previous state (before last transition)
     */
    GroupState GetPreviousState() const { return _previousState; }

    /**
     * Check if in a specific state
     * @param state State to check
     * @return true if current state matches
     */
    bool IsInState(GroupState state) const { return _currentState == state; }

    /**
     * Check if in any of multiple states
     * @param states Vector of states to check
     * @return true if current state is in the list
     */
    bool IsInAnyState(::std::vector<GroupState> const& states) const;

    /**
     * Check if bot is in a group
     * @return true if state is not NOT_IN_GROUP
     */
    bool IsInGroup() const { return _currentState != GroupState::NOT_IN_GROUP; }

    /**
     * Check if bot is in a raid
     * @return true if in raid state
     */
    bool IsInRaid() const
    {
        return _currentState == GroupState::RAID_FORMING ||
               _currentState == GroupState::RAID_ACTIVE ||
               (_currentState == GroupState::IN_COMBAT && _wasRaidBeforeCombat);
    }

    /**
     * Check if group is in combat
     * @return true if in combat state
     */
    bool IsInCombat() const { return _currentState == GroupState::IN_COMBAT; }

    /**
     * Check if in instance
     * @return true if in instance-related state
     */
    bool IsInInstance() const
    {
        return _currentState == GroupState::ENTERING_INSTANCE ||
               _currentState == GroupState::IN_INSTANCE ||
               _currentState == GroupState::INSTANCE_COMPLETE;
    }

    /**
     * Get time in current state
     * @return Duration in current state (milliseconds)
     */
    uint32 GetTimeInState() const;

    /**
     * Get total number of state changes
     * @return Transition count
     */
    uint32 GetTransitionCount() const { return _transitionCount; }

    // ========================================================================
    // STATE TRANSITIONS
    // ========================================================================

    /**
     * Attempt a state transition
     * @param transition The transition to perform
     * @param group The group (for validation, optional)
     * @return true if transition was successful, false if invalid
     */
    bool Transition(GroupStateTransition transition, Group* group = nullptr);

    /**
     * Force a state change (bypasses validation)
     * Use only for error recovery or special cases
     * @param newState State to force
     */
    void ForceState(GroupState newState);

    /**
     * Check if a transition is valid from current state
     * @param transition Transition to validate
     * @param group The group (for additional validation)
     * @return true if transition is allowed
     */
    bool CanTransition(GroupStateTransition transition, Group* group = nullptr) const;

    // ========================================================================
    // STATE CALLBACKS
    // ========================================================================

    /**
     * Callback function type for state entry/exit
     * Parameters: (oldState, newState, transition)
     */
    using StateCallback = ::std::function<void(GroupState, GroupState, GroupStateTransition)>;

    /**
     * Register callback for state entry
     * @param state State to monitor
     * @param callback Function to call when entering state
     */
    void OnStateEnter(GroupState state, StateCallback callback);

    /**
     * Register callback for state exit
     * @param state State to monitor
     * @param callback Function to call when exiting state
     */
    void OnStateExit(GroupState state, StateCallback callback);

    /**
     * Register callback for any state change
     * @param callback Function to call on any transition
     */
    void OnAnyTransition(StateCallback callback);

    // ========================================================================
    // STATE HISTORY
    // ========================================================================

    /**
     * State history entry
     */
    struct StateHistoryEntry
    {
        GroupState fromState;
        GroupState toState;
        GroupStateTransition transition;
        ::std::chrono::steady_clock::time_point timestamp;
        uint32 durationInPreviousStateMs;
    };

    /**
     * Get state history
     * @param maxEntries Maximum entries to return (0 = all)
     * @return Vector of recent state changes
     */
    ::std::vector<StateHistoryEntry> GetHistory(uint32 maxEntries = 10) const;

    /**
     * Clear state history
     */
    void ClearHistory();

    // ========================================================================
    // DEBUGGING
    // ========================================================================

    /**
     * Get string representation of current state
     * @return State name
     */
    ::std::string GetStateName() const;

    /**
     * Get string representation of any state
     * @param state State to convert
     * @return State name
     */
    static ::std::string GetStateName(GroupState state);

    /**
     * Get string representation of transition
     * @param transition Transition to convert
     * @return Transition name
     */
    static ::std::string GetTransitionName(GroupStateTransition transition);

    /**
     * Dump current state and history to log
     */
    void DumpState() const;

    /**
     * Generate state diagram (DOT format)
     * @return GraphViz DOT format string
     */
    static ::std::string GenerateStateDiagram();

private:
    /**
     * Execute a state transition
     * @param newState Target state
     * @param transition Transition that triggered the change
     */
    void ExecuteTransition(GroupState newState, GroupStateTransition transition);

    /**
     * Validate transition based on current state
     * @param transition Transition to validate
     * @param group Group for context (optional)
     * @return true if valid
     */
    bool ValidateTransition(GroupStateTransition transition, Group* group) const;

    /**
     * Call entry callbacks for new state
     * @param oldState Previous state
     * @param newState New state
     * @param transition Transition used
     */
    void CallEntryCallbacks(GroupState oldState, GroupState newState, GroupStateTransition transition);

    /**
     * Call exit callbacks for old state
     * @param oldState State being exited
     * @param newState State being entered
     * @param transition Transition used
     */
    void CallExitCallbacks(GroupState oldState, GroupState newState, GroupStateTransition transition);

    /**
     * Add entry to state history
     * @param fromState Previous state
     * @param toState New state
     * @param transition Transition used
     */
    void RecordTransition(GroupState fromState, GroupState toState, GroupStateTransition transition);

private:
    ObjectGuid _botGuid;                            ///< Bot this state machine belongs to
    GroupState _currentState;                        ///< Current state
    GroupState _previousState;                       ///< Previous state
    ::std::chrono::steady_clock::time_point _stateEntryTime; ///< When current state was entered
    uint32 _transitionCount;                         ///< Total number of transitions

    // State tracking
    bool _wasRaidBeforeCombat;                       ///< Was raid before entering combat

    // Callbacks
    ::std::unordered_map<GroupState, ::std::vector<StateCallback>> _entryCallbacks;
    ::std::unordered_map<GroupState, ::std::vector<StateCallback>> _exitCallbacks;
    ::std::vector<StateCallback> _anyTransitionCallbacks;

    // History
    ::std::vector<StateHistoryEntry> _history;
    static constexpr uint32 MAX_HISTORY_SIZE = 50;

    // Transition validation table (inline to avoid DLL linkage issues)
    static inline const ::std::unordered_map<GroupState, ::std::vector<GroupStateTransition>> _validTransitions = {
        // NOT_IN_GROUP can only receive invites or be forced
        {GroupState::NOT_IN_GROUP, {
            GroupStateTransition::RECEIVE_INVITE
        }},

        // INVITED can accept, decline, or timeout
        {GroupState::INVITED, {
            GroupStateTransition::ACCEPT_INVITE,
            GroupStateTransition::DECLINE_INVITE,
            GroupStateTransition::START_DISBAND
        }},

        // FORMING can gain members to become ACTIVE or disband
        {GroupState::FORMING, {
            GroupStateTransition::MEMBER_JOINED,      // → ACTIVE (3+ members)
            GroupStateTransition::MEMBER_LEFT,        // → stay FORMING or NOT_IN_GROUP
            GroupStateTransition::START_DISBAND
        }},

        // ACTIVE is the main state with many transitions
        {GroupState::ACTIVE, {
            GroupStateTransition::MEMBER_LEFT,        // → FORMING (< 3 members)
            GroupStateTransition::CONVERT_TO_RAID,    // → RAID_FORMING (5+ members)
            GroupStateTransition::ENTER_COMBAT,       // → IN_COMBAT
            GroupStateTransition::READY_CHECK_START,  // → READY_CHECK
            GroupStateTransition::ZONE_INTO_INSTANCE, // → ENTERING_INSTANCE
            GroupStateTransition::START_DISBAND
        }},

        // RAID_FORMING can become RAID_ACTIVE or drop back to ACTIVE
        {GroupState::RAID_FORMING, {
            GroupStateTransition::RAID_READY,         // → RAID_ACTIVE (10+ members)
            GroupStateTransition::MEMBER_LEFT,        // → ACTIVE or FORMING
            GroupStateTransition::CONVERT_TO_PARTY,   // → ACTIVE
            GroupStateTransition::ENTER_COMBAT,
            GroupStateTransition::START_DISBAND
        }},

        // RAID_ACTIVE has raid-specific transitions
        {GroupState::RAID_ACTIVE, {
            GroupStateTransition::CONVERT_TO_PARTY,   // → ACTIVE (< 10 members)
            GroupStateTransition::ENTER_COMBAT,       // → IN_COMBAT
            GroupStateTransition::READY_CHECK_START,  // → READY_CHECK
            GroupStateTransition::ZONE_INTO_INSTANCE, // → ENTERING_INSTANCE
            GroupStateTransition::START_DISBAND
        }},

        // READY_CHECK can complete or be cancelled
        {GroupState::READY_CHECK, {
            GroupStateTransition::READY_CHECK_COMPLETE, // → PREPARING_FOR_COMBAT
            GroupStateTransition::ENTER_COMBAT,       // → IN_COMBAT (pull during ready check)
            GroupStateTransition::START_DISBAND
        }},

        // PREPARING_FOR_COMBAT transitions to combat or cancels
        {GroupState::PREPARING_FOR_COMBAT, {
            GroupStateTransition::READY_FOR_PULL,     // → ACTIVE or RAID_ACTIVE
            GroupStateTransition::ENTER_COMBAT,       // → IN_COMBAT
            GroupStateTransition::START_DISBAND
        }},

        // IN_COMBAT can only leave combat or disband
        {GroupState::IN_COMBAT, {
            GroupStateTransition::LEAVE_COMBAT,       // → ACTIVE or RAID_ACTIVE
            GroupStateTransition::START_DISBAND       // Wipe
        }},

        // ENTERING_INSTANCE can enter or fail
        {GroupState::ENTERING_INSTANCE, {
            GroupStateTransition::INSTANCE_ENTERED,   // → IN_INSTANCE
            GroupStateTransition::LEAVE_INSTANCE,     // → ACTIVE or RAID_ACTIVE (failed to enter)
            GroupStateTransition::START_DISBAND
        }},

        // IN_INSTANCE can clear, leave, or wipe
        {GroupState::IN_INSTANCE, {
            GroupStateTransition::INSTANCE_CLEARED,   // → INSTANCE_COMPLETE
            GroupStateTransition::LEAVE_INSTANCE,     // → ACTIVE or RAID_ACTIVE
            GroupStateTransition::ENTER_COMBAT,       // → IN_COMBAT (in instance)
            GroupStateTransition::START_DISBAND
        }},

        // INSTANCE_COMPLETE can leave instance
        {GroupState::INSTANCE_COMPLETE, {
            GroupStateTransition::LEAVE_INSTANCE,     // → ACTIVE or RAID_ACTIVE
            GroupStateTransition::START_DISBAND
        }},

        // DISBANDING can only complete
        {GroupState::DISBANDING, {
            GroupStateTransition::COMPLETE_DISBAND    // → NOT_IN_GROUP
        }}
    };
};

/**
 * @class GroupStateGuard
 * @brief RAII guard for expected state
 *
 * Use this to ensure a bot is in a specific state before performing
 * an action, and restore previous state if needed.
 *
 * Example:
 * ```cpp
 * GroupStateGuard guard(stateMachine, GroupState::ACTIVE);
 * if (!guard.IsValid())
 {
 *     TC_LOG_ERROR("Bot is not in ACTIVE state!");
 *     return false;
 * }
 * // Perform action knowing bot is in ACTIVE state
 * ```
 */
class TC_GAME_API GroupStateGuard
{
public:
    GroupStateGuard(GroupStateMachine& stateMachine, GroupState expectedState)
        : _stateMachine(stateMachine), _expectedState(expectedState)
    {
        _isValid = _stateMachine.IsInState(_expectedState);
    }

    ~GroupStateGuard() = default;

    bool IsValid() const { return _isValid; }
    GroupState GetExpectedState() const { return _expectedState; }
    GroupState GetActualState() const { return _stateMachine.GetState(); }

private:
    GroupStateMachine& _stateMachine;
    GroupState _expectedState;
    bool _isValid;
};

} // namespace Playerbot

#endif // PLAYERBOT_GROUP_STATE_MACHINE_H
