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

#ifndef TRINITY_ORDERHALL_ARTIFACT_COMMON_H
#define TRINITY_ORDERHALL_ARTIFACT_COMMON_H

// Shared helpers for the Legion artifact-acquisition scripts under BrokenIsles/Orderhalls/. The proven pattern (from
// the Beast Mastery Hunter Titanstrike scenario, zone_orderhall_hunter.cpp) is captured here so each class/spec file
// can reuse it instead of duplicating the tricky bits.

#include "DB2Structure.h"
#include "Map.h"
#include "Player.h"
#include "Scenario.h"
#include "SharedDefines.h"
#include <initializer_list>

// Advance a running InstanceScenario one step by hand.
//
// WHY: on this server the scenario step criteria (Type-92 "AnyoneTriggerGameEventScenario" game events) do NOT fire -
// the CriteriaTree->ScenarioStep->gameEvent wiring is absent from the loaded DB2 data, so GameEvents::Trigger never
// advances a step. But Scenario::SetStepState and CompleteStep are PUBLIC, so we mark the current step done and let
// CompleteStep pick the next (and CompleteScenario on the final step) - exactly what the criteria path would have done.
//
// NOTE: for an intermediate step CompleteStep logs a benign "Scenario::CompleteStep ... could not determine new step,
// or validate scenario completion" - that is expected TC noise here (the step DID advance), not a failure.
inline void ArtifactAdvanceScenario(WorldObject* from)
{
    if (Scenario* scenario = from->GetScenario())
        if (ScenarioStepEntry const* current = scenario->GetStep())
        {
            scenario->SetStepState(current, SCENARIO_STEP_DONE);
            scenario->CompleteStep(current);
        }
}

// Order index of the scenario's current step (0 if no scenario / already complete).
inline uint8 ArtifactScenarioOrder(WorldObject* from)
{
    if (Scenario* scenario = from->GetScenario())
        if (ScenarioStepEntry const* current = scenario->GetStep())
            return current->OrderIndex;
    return 0;
}

// Grant a set of kill-credit objectives (Legion artifact quests use kill-credit dummy creatures for flight / gossip /
// scene / "received the blessing" objectives).
inline void ArtifactGrantCredits(Player* player, std::initializer_list<uint32> creditEntries)
{
    for (uint32 entry : creditEntries)
        player->KilledMonsterCredit(entry);
}

// First live player within `within` yards of `pos` on the object's map, else nullptr.
inline Player* ArtifactPlayerNear(WorldObject* from, Position const& pos, float within)
{
    for (auto const& ref : from->GetMap()->GetPlayers())
        if (Player* p = ref.GetSource())
            if (p->IsInWorld() && p->IsAlive() && p->GetDistance(pos) <= within)
                return p;
    return nullptr;
}

// First live player who has pushed at least `dist` yards from `origin` (monotonic progress into a linear encounter).
inline Player* ArtifactPlayerBeyond(WorldObject* from, Position const& origin, float dist)
{
    for (auto const& ref : from->GetMap()->GetPlayers())
        if (Player* p = ref.GetSource())
            if (p->IsInWorld() && p->IsAlive() && p->GetDistance(origin) >= dist)
                return p;
    return nullptr;
}

#endif // TRINITY_ORDERHALL_ARTIFACT_COMMON_H
