/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "GroupStateMachine.h"
#include "Group.h"
#include "Log.h"
#include <algorithm>
#include <sstream>

namespace Playerbot
{

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

GroupStateMachine::GroupStateMachine(ObjectGuid botGuid)
    : _botGuid(botGuid)
    , _currentState(GroupState::NOT_IN_GROUP)
    , _previousState(GroupState::NOT_IN_GROUP)
    , _stateEntryTime(::std::chrono::steady_clock::now())
    , _transitionCount(0)
    , _wasRaidBeforeCombat(false)
{
    TC_LOG_DEBUG("playerbot.group.statemachine", "GroupStateMachine created for bot {}",
        _botGuid.ToString());
}

GroupStateMachine::~GroupStateMachine()
{
    TC_LOG_DEBUG("playerbot.group.statemachine", "GroupStateMachine destroyed for bot {} (final state: {})",
        _botGuid.ToString(), GetStateName());
}

// ============================================================================
// STATE QUERIES
// ============================================================================

bool GroupStateMachine::IsInAnyState(::std::vector<GroupState> const& states) const
{
    return ::std::find(states.begin(), states.end(), _currentState) != states.end();
}

uint32 GroupStateMachine::GetTimeInState() const
{
    auto now = ::std::chrono::steady_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(now - _stateEntryTime);
    return static_cast<uint32>(duration.count());
}

// ============================================================================
// STATE TRANSITIONS
// ============================================================================

bool GroupStateMachine::Transition(GroupStateTransition transition, Group* group)
{
    if (!ValidateTransition(transition, group))
    {
        TC_LOG_WARN("playerbot.group.statemachine",
            "Bot {}: Invalid transition {} from state {}",
            _botGuid.ToString(),
            GetTransitionName(transition),
            GetStateName());
        return false;
    }

    // Determine target state based on transition
    GroupState newState = _currentState; // Default: stay in same state
    switch (transition)
    {
        case GroupStateTransition::RECEIVE_INVITE:
            newState = GroupState::INVITED;
            break;

        case GroupStateTransition::ACCEPT_INVITE:
            // Determine if joining a forming or active group
    if (group && group->GetMembersCount() < 3)
                newState = GroupState::FORMING;
            else
                newState = GroupState::ACTIVE;
            break;

        case GroupStateTransition::DECLINE_INVITE:
            newState = GroupState::NOT_IN_GROUP;
            break;

        case GroupStateTransition::MEMBER_JOINED:
            if (group && group->GetMembersCount() >= 3)
                newState = GroupState::ACTIVE;
            else
                newState = GroupState::FORMING;
            break;

        case GroupStateTransition::MEMBER_LEFT:
            if (group && group->GetMembersCount() < 3)
                newState = GroupState::FORMING;
            else if (group && group->isRaidGroup() && group->GetMembersCount() < 10)
                newState = GroupState::ACTIVE;
            // else stay in current state
            break;

        case GroupStateTransition::CONVERT_TO_RAID:
            newState = GroupState::RAID_FORMING;
            break;

        case GroupStateTransition::RAID_READY:
            newState = GroupState::RAID_ACTIVE;
            break;

        case GroupStateTransition::CONVERT_TO_PARTY:
            newState = GroupState::ACTIVE;
            break;

        case GroupStateTransition::ENTER_COMBAT:
            _wasRaidBeforeCombat = IsInRaid();
            newState = GroupState::IN_COMBAT;
            break;

        case GroupStateTransition::LEAVE_COMBAT:
            newState = _wasRaidBeforeCombat ? GroupState::RAID_ACTIVE : GroupState::ACTIVE;
            break;

        case GroupStateTransition::READY_CHECK_START:
            newState = GroupState::READY_CHECK;
            break;

        case GroupStateTransition::READY_CHECK_COMPLETE:
            newState = GroupState::PREPARING_FOR_COMBAT;
            break;

        case GroupStateTransition::READY_FOR_PULL:
            newState = IsInRaid() ? GroupState::RAID_ACTIVE : GroupState::ACTIVE;
            break;

        case GroupStateTransition::ZONE_INTO_INSTANCE:
            newState = GroupState::ENTERING_INSTANCE;
            break;

        case GroupStateTransition::INSTANCE_ENTERED:
            newState = GroupState::IN_INSTANCE;
            break;

        case GroupStateTransition::INSTANCE_CLEARED:
            newState = GroupState::INSTANCE_COMPLETE;
            break;

        case GroupStateTransition::LEAVE_INSTANCE:
            newState = IsInRaid() ? GroupState::RAID_ACTIVE : GroupState::ACTIVE;
            break;

        case GroupStateTransition::START_DISBAND:
            newState = GroupState::DISBANDING;
            break;

        case GroupStateTransition::COMPLETE_DISBAND:
            newState = GroupState::NOT_IN_GROUP;
            break;

        default:
            TC_LOG_ERROR("playerbot.group.statemachine",
                "Bot {}: Unhandled transition {}",
                _botGuid.ToString(),
                GetTransitionName(transition));
            return false;
    }

    // Execute the transition
    ExecuteTransition(newState, transition);
    return true;
}

void GroupStateMachine::ForceState(GroupState newState)
{
    TC_LOG_WARN("playerbot.group.statemachine",
        "Bot {}: Forcing state change from {} to {}",
        _botGuid.ToString(),
        GetStateName(),
        GetStateName(newState));

    ExecuteTransition(newState, GroupStateTransition::MAX_TRANSITION);
}

bool GroupStateMachine::CanTransition(GroupStateTransition transition, Group* group) const
{
    return ValidateTransition(transition, group);
}

// ============================================================================
// STATE CALLBACKS
// ============================================================================

void GroupStateMachine::OnStateEnter(GroupState state, StateCallback callback)
{
    _entryCallbacks[state].push_back(callback);
}

void GroupStateMachine::OnStateExit(GroupState state, StateCallback callback)
{
    _exitCallbacks[state].push_back(callback);
}

void GroupStateMachine::OnAnyTransition(StateCallback callback)
{
    _anyTransitionCallbacks.push_back(callback);
}

// ============================================================================
// STATE HISTORY
// ============================================================================

::std::vector<GroupStateMachine::StateHistoryEntry> GroupStateMachine::GetHistory(uint32 maxEntries) const
{
    if (maxEntries == 0 || maxEntries >= _history.size())
        return _history;

    // Return last N entries
    auto startIt = _history.end() - ::std::min<size_t>(maxEntries, _history.size());
    return ::std::vector<StateHistoryEntry>(startIt, _history.end());
}

void GroupStateMachine::ClearHistory()
{
    _history.clear();
    TC_LOG_DEBUG("playerbot.group.statemachine", "Bot {}: History cleared", _botGuid.ToString());
}

// ============================================================================
// DEBUGGING
// ============================================================================

::std::string GroupStateMachine::GetStateName() const
{
    return GetStateName(_currentState);
}

::std::string GroupStateMachine::GetStateName(GroupState state)
{
    switch (state)
    {
        case GroupState::NOT_IN_GROUP: return "NOT_IN_GROUP";
        case GroupState::INVITED: return "INVITED";
        case GroupState::FORMING: return "FORMING";
        case GroupState::ACTIVE: return "ACTIVE";
        case GroupState::RAID_FORMING: return "RAID_FORMING";
        case GroupState::RAID_ACTIVE: return "RAID_ACTIVE";
        case GroupState::READY_CHECK: return "READY_CHECK";
        case GroupState::PREPARING_FOR_COMBAT: return "PREPARING_FOR_COMBAT";
        case GroupState::IN_COMBAT: return "IN_COMBAT";
        case GroupState::ENTERING_INSTANCE: return "ENTERING_INSTANCE";
        case GroupState::IN_INSTANCE: return "IN_INSTANCE";
        case GroupState::INSTANCE_COMPLETE: return "INSTANCE_COMPLETE";
        case GroupState::DISBANDING: return "DISBANDING";
        default: return "UNKNOWN";
    }
}

::std::string GroupStateMachine::GetTransitionName(GroupStateTransition transition)
{
    switch (transition)
    {
        case GroupStateTransition::RECEIVE_INVITE: return "RECEIVE_INVITE";
        case GroupStateTransition::ACCEPT_INVITE: return "ACCEPT_INVITE";
        case GroupStateTransition::DECLINE_INVITE: return "DECLINE_INVITE";
        case GroupStateTransition::MEMBER_JOINED: return "MEMBER_JOINED";
        case GroupStateTransition::MEMBER_LEFT: return "MEMBER_LEFT";
        case GroupStateTransition::CONVERT_TO_RAID: return "CONVERT_TO_RAID";
        case GroupStateTransition::RAID_READY: return "RAID_READY";
        case GroupStateTransition::CONVERT_TO_PARTY: return "CONVERT_TO_PARTY";
        case GroupStateTransition::ENTER_COMBAT: return "ENTER_COMBAT";
        case GroupStateTransition::LEAVE_COMBAT: return "LEAVE_COMBAT";
        case GroupStateTransition::READY_CHECK_START: return "READY_CHECK_START";
        case GroupStateTransition::READY_CHECK_COMPLETE: return "READY_CHECK_COMPLETE";
        case GroupStateTransition::READY_FOR_PULL: return "READY_FOR_PULL";
        case GroupStateTransition::ZONE_INTO_INSTANCE: return "ZONE_INTO_INSTANCE";
        case GroupStateTransition::INSTANCE_ENTERED: return "INSTANCE_ENTERED";
        case GroupStateTransition::INSTANCE_CLEARED: return "INSTANCE_CLEARED";
        case GroupStateTransition::LEAVE_INSTANCE: return "LEAVE_INSTANCE";
        case GroupStateTransition::START_DISBAND: return "START_DISBAND";
        case GroupStateTransition::COMPLETE_DISBAND: return "COMPLETE_DISBAND";
        default: return "UNKNOWN";
    }
}

void GroupStateMachine::DumpState() const
{
    TC_LOG_INFO("playerbot.group.statemachine",
        "GroupStateMachine for bot {}:\n"
        "  Current State: {}\n"
        "  Previous State: {}\n"
        "  Time in State: {}ms\n"
        "  Total Transitions: {}\n"
        "  History Entries: {}",
        _botGuid.ToString(),
        GetStateName(),
        GetStateName(_previousState),
        GetTimeInState(),
        _transitionCount,
        _history.size());

    if (!_history.empty())
    {
        TC_LOG_INFO("playerbot.group.statemachine", "Recent transitions:");
        auto recentHistory = GetHistory(5);
        for (auto const& entry : recentHistory)
        {
            TC_LOG_INFO("playerbot.group.statemachine",
                "  {} → {} via {} ({}ms ago)",
                GetStateName(entry.fromState),
                GetStateName(entry.toState),
                GetTransitionName(entry.transition),
                entry.durationInPreviousStateMs);
        }
    }
}

::std::string GroupStateMachine::GenerateStateDiagram()
{
    ::std::ostringstream dot;

    dot << "digraph GroupStateMachine {\n";
    dot << "  rankdir=LR;\n";
    dot << "  node [shape=box, style=rounded];\n\n";

    // Special styling for initial and terminal states
    dot << "  NOT_IN_GROUP [shape=doublecircle, style=filled, fillcolor=lightgray];\n";
    dot << "  DISBANDING [shape=doublecircle, style=filled, fillcolor=lightcoral];\n";
    dot << "  IN_COMBAT [style=filled, fillcolor=lightpink];\n\n";

    // Define all transitions
    for (auto const& [fromState, transitions] : _validTransitions)
    {
        for (auto transition : transitions)
        {
            // For brevity, only show main transitions in diagram
            ::std::string label = GetTransitionName(transition);

            // This is simplified - in reality transitions go to various states
            // You would need to implement full logic here
            dot << "  " << GetStateName(fromState) << " -> ";

            // Simplified target state determination for diagram
    switch (transition)
            {
                case GroupStateTransition::RECEIVE_INVITE:
                    dot << "INVITED";
                    break;
                case GroupStateTransition::ACCEPT_INVITE:
                    dot << "FORMING";
                    break;
                case GroupStateTransition::MEMBER_JOINED:
                    dot << "ACTIVE";
                    break;
                case GroupStateTransition::CONVERT_TO_RAID:
                    dot << "RAID_FORMING";
                    break;
                case GroupStateTransition::RAID_READY:
                    dot << "RAID_ACTIVE";
                    break;
                case GroupStateTransition::ENTER_COMBAT:
                    dot << "IN_COMBAT";
                    break;
                case GroupStateTransition::COMPLETE_DISBAND:
                    dot << "NOT_IN_GROUP";
                    break;
                default:
                    dot << "...";
                    break;
            }

            dot << " [label=\"" << label << "\"];\n";
        }
    }

    dot << "}\n";

    return dot.str();
}

// ============================================================================
// PRIVATE METHODS
// ============================================================================

void GroupStateMachine::ExecuteTransition(GroupState newState, GroupStateTransition transition)
{
    GroupState oldState = _currentState;
    uint32 timeInOldState = GetTimeInState();

    // Call exit callbacks
    CallExitCallbacks(oldState, newState, transition);

    // Update state
    _previousState = _currentState;
    _currentState = newState;
    _stateEntryTime = ::std::chrono::steady_clock::now();
    _transitionCount++;

    // Record transition
    RecordTransition(oldState, newState, transition);

    // Call entry callbacks
    CallEntryCallbacks(oldState, newState, transition);

    // Log transition
    TC_LOG_DEBUG("playerbot.group.statemachine",
        "Bot {}: {} → {} via {} (spent {}ms in previous state)",
        _botGuid.ToString(),
        GetStateName(oldState),
        GetStateName(newState),
        GetTransitionName(transition),
        timeInOldState);
}

bool GroupStateMachine::ValidateTransition(GroupStateTransition transition, Group* group) const
{
    // Check if transition is valid from current state
    auto it = _validTransitions.find(_currentState);
    if (it == _validTransitions.end())
        return false;

    auto const& validTransitions = it->second;
    if (::std::find(validTransitions.begin(), validTransitions.end(), transition) == validTransitions.end())
        return false;

    // Additional context-specific validation
    if (group)
    {
        switch (transition)
        {
            case GroupStateTransition::CONVERT_TO_RAID:
                if (group->GetMembersCount() < 5)
                    return false;
                break;

            case GroupStateTransition::RAID_READY:
                if (group->GetMembersCount() < 10)
                    return false;
                break;

            case GroupStateTransition::CONVERT_TO_PARTY:
                if (group->GetMembersCount() > 5)
                    return false;
                break;

            default:
                break;
        }
    }

    return true;
}

void GroupStateMachine::CallEntryCallbacks(GroupState oldState, GroupState newState, GroupStateTransition transition)
{
    // Call state-specific entry callbacks
    auto it = _entryCallbacks.find(newState);
    if (it != _entryCallbacks.end())
    {
        for (auto const& callback : it->second)
            callback(oldState, newState, transition);
    }

    // Call generic transition callbacks
    for (auto const& callback : _anyTransitionCallbacks)
        callback(oldState, newState, transition);
}

void GroupStateMachine::CallExitCallbacks(GroupState oldState, GroupState newState, GroupStateTransition transition)
{
    auto it = _exitCallbacks.find(oldState);
    if (it != _exitCallbacks.end())
    {
        for (auto const& callback : it->second)
            callback(oldState, newState, transition);
    }
}

void GroupStateMachine::RecordTransition(GroupState fromState, GroupState toState, GroupStateTransition transition)
{
    StateHistoryEntry entry;
    entry.fromState = fromState;
    entry.toState = toState;
    entry.transition = transition;
    entry.timestamp = ::std::chrono::steady_clock::now();
    entry.durationInPreviousStateMs = GetTimeInState();

    _history.push_back(entry);

    // Limit history size
    if (_history.size() > MAX_HISTORY_SIZE)
        _history.erase(_history.begin());
}

} // namespace Playerbot
