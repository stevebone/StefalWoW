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

#include "ManagedWorldStateMgr.h"
#include "ConditionMgr.h"
#include "ContributionMgr.h"
#include "DB2Stores.h"
#include "Log.h"
#include "Player.h"
#include "Timer.h"
#include "WarfrontMgr.h"
#include "World.h"
#include "WorldSession.h"
#include "WorldStateMgr.h"
#include <algorithm>

namespace
{
    constexpr uint32 ACCUMULATION_INTERVAL_MS = MINUTE * IN_MILLISECONDS;
}

ManagedWorldStateMgr::ManagedWorldStateMgr() = default;
ManagedWorldStateMgr::~ManagedWorldStateMgr() = default;

ManagedWorldStateMgr* ManagedWorldStateMgr::instance()
{
    static ManagedWorldStateMgr instance;
    return &instance;
}

void ManagedWorldStateMgr::Load()
{
    uint32 const oldMSTime = getMSTime();
    _states.clear();
    _buffsByState.clear();

    // Index the stage buffs by their managed world state so OnReachedTarget / login can apply them cheaply.
    for (ManagedWorldStateBuffEntry const* buff : sManagedWorldStateBuffStore)
        if (buff->BuffSpellID)
            _buffsByState[uint32(buff->ManagedWorldStateID)].push_back(buff);

    for (ManagedWorldStateEntry const* entry : sManagedWorldStateStore)
    {
        StateData& state = _states[entry->ID];
        state.Entry = entry;

        // Restore the persisted display values from the world-state DB (realm-global world states).
        if (entry->ProgressWorldStateID)
            state.Progress = WorldStateMgr::GetValue(entry->ProgressWorldStateID, nullptr);
        if (entry->CurrentStageWorldStateID)
            state.Stage = WorldStateMgr::GetValue(entry->CurrentStageWorldStateID, nullptr);
        if (entry->OccurrencesWorldStateID[0])
            state.Occurrences = WorldStateMgr::GetValue(entry->OccurrencesWorldStateID[0], nullptr);

        // Derive the phase from the restored Progress rather than always starting in Up. The phase + timers are NOT
        // persisted (only Progress/Stage/Occurrences are), so a cycling state (both windows set) that was saved at or
        // above its accumulation target was mid-DOWN (depleting). Forcing it back to Up would re-accumulate to the
        // target and re-fire OnReachedTarget - a duplicate stage advance + buff (and turn-in over-reward) on every
        // restart.
        bool const cycles = entry->UpTimeSecs && entry->DownTimeSecs;
        if (cycles && entry->AccumulationStateTargetValue && state.Progress >= entry->AccumulationStateTargetValue)
        {
            state.CurrentPhase = Phase::Down;
            state.PhaseTimerMs = entry->DownTimeSecs * IN_MILLISECONDS;
        }
        else
        {
            state.CurrentPhase = Phase::Up;
            state.PhaseTimerMs = entry->UpTimeSecs * IN_MILLISECONDS;
        }
        state.AccumTimerMs = 0;
    }

    TC_LOG_INFO("server.loading", ">> Loaded {} managed world states in {} ms", _states.size(), GetMSTimeDiffToNow(oldMSTime));
}

void ManagedWorldStateMgr::Update(uint32 diff)
{
    for (auto& [id, state] : _states)
    {
        ManagedWorldStateEntry const* entry = state.Entry;

        // Advance the up/down phase cycle only when the state actually cycles (both windows non-zero). A state with
        // zero windows is purely contribution-driven and stays in its Up phase.
        if (entry->UpTimeSecs && entry->DownTimeSecs)
        {
            if (diff >= state.PhaseTimerMs)
            {
                state.CurrentPhase = state.CurrentPhase == Phase::Up ? Phase::Down : Phase::Up;
                state.PhaseTimerMs = (state.CurrentPhase == Phase::Up ? entry->UpTimeSecs : entry->DownTimeSecs) * IN_MILLISECONDS;
            }
            else
                state.PhaseTimerMs -= diff;
        }

        // Time-based accumulation/depletion is applied once per elapsed minute.
        if (entry->AccumulationAmountPerMinute || entry->DepletionAmountPerMinute)
        {
            state.AccumTimerMs += diff;
            while (state.AccumTimerMs >= ACCUMULATION_INTERVAL_MS)
            {
                state.AccumTimerMs -= ACCUMULATION_INTERVAL_MS;
                ApplyMinuteTick(state);
            }
        }
    }
}

void ManagedWorldStateMgr::ApplyMinuteTick(StateData& state)
{
    ManagedWorldStateEntry const* entry = state.Entry;
    int32 const oldProgress = state.Progress;

    if (state.CurrentPhase == Phase::Up && entry->AccumulationAmountPerMinute)
        state.Progress = std::min(state.Progress + entry->AccumulationAmountPerMinute, entry->AccumulationStateTargetValue);
    else if (state.CurrentPhase == Phase::Down && entry->DepletionAmountPerMinute)
        state.Progress = std::max(state.Progress - entry->DepletionAmountPerMinute, entry->DepletionStateTargetValue);

    if (state.Progress == oldProgress)
        return;

    PushProgress(state);
    if (state.Progress >= entry->AccumulationStateTargetValue && oldProgress < entry->AccumulationStateTargetValue)
        OnReachedTarget(state);
}

bool ManagedWorldStateMgr::AddProgress(uint32 managedWorldStateId, int32 amount)
{
    auto itr = _states.find(managedWorldStateId);
    if (itr == _states.end())
        return false;

    StateData& state = itr->second;
    ManagedWorldStateEntry const* entry = state.Entry;
    int32 const oldProgress = state.Progress;

    state.Progress = std::clamp(state.Progress + amount, entry->DepletionStateTargetValue, entry->AccumulationStateTargetValue);
    if (state.Progress == oldProgress)
        return true;

    PushProgress(state);
    if (state.Progress >= entry->AccumulationStateTargetValue && oldProgress < entry->AccumulationStateTargetValue)
        OnReachedTarget(state);

    return true;
}

void ManagedWorldStateMgr::OnReachedTarget(StateData& state)
{
    // Reaching the accumulation target completes one occurrence of the managed world state and advances the stage.
    ++state.Occurrences;
    ++state.Stage;
    PushStage(state);
    PushOccurrences(state);
    ApplyBuffsForOccurrence(state);

    // Integration seam (WARFRONTS_DESIGN.md §5, §Key integration seams): when a warfront's challenger contribution
    // bar reaches its target, transition that zone CONTRIBUTION -> SIEGE. WarfrontMgr matches this managed world
    // state id against the challenger bar of each warfront currently in CONTRIBUTION and no-ops for every other id.
    sWarfrontMgr->OnContributionTargetReached(state.Entry->ID);
}

bool ManagedWorldStateMgr::GetSnapshot(uint32 managedWorldStateId, ManagedWorldStateSnapshot& out) const
{
    auto itr = _states.find(managedWorldStateId);
    if (itr == _states.end())
        return false;

    StateData const& state = itr->second;
    out.Progress = state.Progress;
    out.Target = state.Entry->AccumulationStateTargetValue;
    out.Floor = state.Entry->DepletionStateTargetValue;
    out.Stage = state.Stage;
    out.Occurrences = state.Occurrences;
    out.Accumulating = state.CurrentPhase == Phase::Up;
    // The phase timer only means anything for a state that actually cycles (both windows non-zero).
    out.PhaseRemainingMs = (state.Entry->UpTimeSecs && state.Entry->DownTimeSecs) ? state.PhaseTimerMs : 0;
    return true;
}

void ManagedWorldStateMgr::ResetProgress(uint32 managedWorldStateId)
{
    auto itr = _states.find(managedWorldStateId);
    if (itr == _states.end())
        return;

    StateData& state = itr->second;

    // Drop the bar back to its empty floor and restart the up window so it can fill again from scratch next cycle.
    state.Progress = state.Entry->DepletionStateTargetValue;
    state.CurrentPhase = Phase::Up;
    state.PhaseTimerMs = state.Entry->UpTimeSecs * IN_MILLISECONDS;
    state.AccumTimerMs = 0;
    PushProgress(state);
}

void ManagedWorldStateMgr::ApplyBuffsForOccurrence(StateData const& state) const
{
    auto itr = _buffsByState.find(state.Entry->ID);
    if (itr == _buffsByState.end())
        return;

    for (ManagedWorldStateBuffEntry const* buff : itr->second)
    {
        // Apply each buff exactly as its occurrence threshold is newly reached (occurrences advance one at a time).
        if (buff->OccurrenceValue != uint32(state.Occurrences))
            continue;

        for (auto const& [accountId, session] : sWorld->GetAllSessions())
        {
            Player* player = session->GetPlayer();
            if (!player || !player->IsInWorld())
                continue;

            if (buff->PlayerConditionID && !ConditionMgr::IsPlayerMeetingCondition(player, buff->PlayerConditionID))
                continue;

            player->CastSpell(player, uint32(buff->BuffSpellID), true);
        }
    }
}

void ManagedWorldStateMgr::ApplyActiveBuffs(Player* player) const
{
    for (auto const& [stateId, state] : _states)
    {
        auto itr = _buffsByState.find(stateId);
        if (itr == _buffsByState.end())
            continue;

        for (ManagedWorldStateBuffEntry const* buff : itr->second)
        {
            if (uint32(state.Occurrences) < buff->OccurrenceValue)
                continue;

            if (buff->PlayerConditionID && !ConditionMgr::IsPlayerMeetingCondition(player, buff->PlayerConditionID))
                continue;

            player->CastSpell(player, uint32(buff->BuffSpellID), true);
        }
    }
}

void ManagedWorldStateMgr::PushProgress(StateData const& state) const
{
    // THIS is what paints the Contribution Collector / warfront bar on the client: a realm-wide world state, which
    // WorldStateMgr::Update() broadcasts to every session as SMSG_UPDATE_WORLD_STATE and replays to new logins in
    // SMSG_INIT_WORLD_STATES. Confirmed against the 8.0.1.27377 war-effort sniff, where the live war-effort totals
    // arrived purely as UPDATE_WORLD_STATE {int32 VariableID, int32 Value, bit Hidden} pushes every ~5s.
    if (state.Entry->ProgressWorldStateID)
        WorldStateMgr::SetValueAndSaveInDb(state.Entry->ProgressWorldStateID, state.Progress, false, nullptr);

    // Stamp the "last update" time of every Contribution Collector fed by this state (and ack it to online players
    // when Warfront.NativeUI.Enable is on). This carries no bar data - the world state above does.
    sContributionMgr->BroadcastManagedWorldStateUpdate(state.Entry->ID);
}

void ManagedWorldStateMgr::PushStage(StateData const& state) const
{
    if (state.Entry->CurrentStageWorldStateID)
        WorldStateMgr::SetValueAndSaveInDb(state.Entry->CurrentStageWorldStateID, state.Stage, false, nullptr);
}

void ManagedWorldStateMgr::PushOccurrences(StateData const& state) const
{
    if (state.Entry->OccurrencesWorldStateID[0])
        WorldStateMgr::SetValueAndSaveInDb(state.Entry->OccurrencesWorldStateID[0], state.Occurrences, false, nullptr);
}
