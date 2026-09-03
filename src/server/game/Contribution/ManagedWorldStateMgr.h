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

#ifndef TRINITYCORE_MANAGED_WORLD_STATE_MGR_H
#define TRINITYCORE_MANAGED_WORLD_STATE_MGR_H

#include "Define.h"
#include <unordered_map>
#include <vector>

class Player;
struct ManagedWorldStateEntry;
struct ManagedWorldStateBuffEntry;

// Read-only view of a managed world state's live runtime values, for the display/UI layer (the war-effort and
// warfront contribution bars). Everything here is real server state - none of it is inferred.
struct ManagedWorldStateSnapshot
{
    int32 Progress = 0;             // current bar counter
    int32 Target = 0;               // AccumulationStateTargetValue ("full")
    int32 Floor = 0;                // DepletionStateTargetValue ("empty")
    int32 Stage = 0;
    int32 Occurrences = 0;
    bool Accumulating = true;       // true while in the Up (filling) window
    uint32 PhaseRemainingMs = 0;    // time left in the current up/down window (0 when the state does not cycle)
};

// Runtime for ManagedWorldState.db2: a realm-wide progress driver that accumulates toward a target during its "up"
// window and depletes during its "down" window, exposing the value through the Progress / CurrentStage / Occurrences
// world states so the client can draw the war-effort/building progress bar. War-effort contributions (P2) feed extra
// progress in through AddProgress(). Values are pushed as realm-global world states (persisted in the world-state DB),
// which covers the common realm-wide managed states; zone-scoped states would additionally need per-map propagation.
class TC_GAME_API ManagedWorldStateMgr
{
    ManagedWorldStateMgr();
    ~ManagedWorldStateMgr();

public:
    ManagedWorldStateMgr(ManagedWorldStateMgr const&) = delete;
    ManagedWorldStateMgr(ManagedWorldStateMgr&&) = delete;
    ManagedWorldStateMgr& operator=(ManagedWorldStateMgr const&) = delete;
    ManagedWorldStateMgr& operator=(ManagedWorldStateMgr&&) = delete;

    static ManagedWorldStateMgr* instance();

    // Builds the runtime state from ManagedWorldState.db2, restoring persisted progress from the world-state DB.
    void Load();

    // Ticks accumulation/depletion and the up/down phase cycle.
    void Update(uint32 diff);

    // Adds (or, with a negative amount, removes) progress to a managed world state; clamps to the depletion/target
    // bounds and pushes the updated world states. Returns false when the id is unknown OR when the clamp meant the
    // bar did not actually move (i.e. it was already full) - the Contribute path relies on that to refuse a
    // donation instead of consuming its cost for nothing.
    bool AddProgress(uint32 managedWorldStateId, int32 amount);

    // Resets a managed world state's progress bar to its empty (depletion-target) floor and restarts its up window.
    // No-op for an unknown id. Used by WarfrontMgr on a control flip so the new challenger's bar starts fresh.
    void ResetProgress(uint32 managedWorldStateId);

    // Read-only snapshot of a managed world state's live values (progress/target/stage/phase timer), used by the
    // Contribution display round-trip to compute the native UI's bar percentage and next-state-change time.
    // Returns false for an unknown id.
    bool GetSnapshot(uint32 managedWorldStateId, ManagedWorldStateSnapshot& out) const;

    // Applies every managed-world-state stage buff the player is currently eligible for (occurrence already reached
    // + PlayerCondition met). Called on login so stage rewards persist for players who arrive after the stage flip.
    void ApplyActiveBuffs(Player* player) const;

private:
    enum class Phase : uint8 { Up, Down };

    struct StateData
    {
        ManagedWorldStateEntry const* Entry = nullptr;
        int32 Progress = 0;
        int32 Stage = 0;
        int32 Occurrences = 0;
        Phase CurrentPhase = Phase::Up;
        uint32 PhaseTimerMs = 0;
        uint32 AccumTimerMs = 0;
    };

    void ApplyMinuteTick(StateData& state);
    void OnReachedTarget(StateData& state);
    void ApplyBuffsForOccurrence(StateData const& state) const;
    void PushProgress(StateData const& state) const;
    void PushStage(StateData const& state) const;
    void PushOccurrences(StateData const& state) const;

    std::unordered_map<uint32, StateData> _states;
    std::unordered_map<uint32 /*managedWorldStateId*/, std::vector<ManagedWorldStateBuffEntry const*>> _buffsByState;
};

#define sManagedWorldStateMgr ManagedWorldStateMgr::instance()

#endif // TRINITYCORE_MANAGED_WORLD_STATE_MGR_H
