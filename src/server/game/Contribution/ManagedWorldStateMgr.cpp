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
#include "DB2Stores.h"
#include "Log.h"
#include "Player.h"
#include "Timer.h"
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

        state.CurrentPhase = Phase::Up;
        state.PhaseTimerMs = entry->UpTimeSecs * IN_MILLISECONDS;
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
    if (state.Entry->ProgressWorldStateID)
        WorldStateMgr::SetValueAndSaveInDb(state.Entry->ProgressWorldStateID, state.Progress, false, nullptr);
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
