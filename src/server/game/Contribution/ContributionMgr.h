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

#ifndef TRINITYCORE_CONTRIBUTION_MGR_H
#define TRINITYCORE_CONTRIBUTION_MGR_H

#include "Define.h"
#include "ObjectGuid.h"
#include <unordered_map>
#include <vector>

class Player;

// Handles the war-effort Contribution Collector flow: a player at a collector NPC turns in the required
// currency/items (defined by the contribution's ManagedWorldStateInput.QuestID) to advance the associated
// ManagedWorldState, which the ManagedWorldStateMgr exposes through the progress world states.
class TC_GAME_API ContributionMgr
{
    ContributionMgr();
    ~ContributionMgr();

public:
    ContributionMgr(ContributionMgr const&) = delete;
    ContributionMgr(ContributionMgr&&) = delete;
    ContributionMgr& operator=(ContributionMgr const&) = delete;
    ContributionMgr& operator=(ContributionMgr&&) = delete;

    static ContributionMgr* instance();

    // Builds the collector-creature -> contribution index from CreatureXContribution.db2.
    void Load();

    // Validates the collector and the player's eligibility, consumes the turn-in cost, and advances the
    // associated managed world state.
    void Contribute(Player* player, ObjectGuid collectorGuid, uint32 contributionId);

    // --- native UI last-update round-trip (SMSG_CONTRIBUTION_LAST_UPDATE_RESPONSE) ------------------------------
    // The response is a twelve-byte timestamp acknowledgement { uint32 Data, uint32 ContributionID,
    // uint32 ContributionGUID } - it carries no state and no bar values. Everything the Contribution Collector frame
    // displays is resolved client-side from Contribution.db2 / ManagedWorldState.db2 against the world-state values
    // ManagedWorldStateMgr pushes through WorldStateMgr (SMSG_UPDATE_WORLD_STATE / SMSG_INIT_WORLD_STATES).
    //
    // Kept behind worldserver.conf Warfront.NativeUI.Enable (default 0) until a 12.0.7 sniff confirms the opcode is
    // still shaped this way.
    static bool IsNativeUiEnabled();

    // Time (unix) at which the contribution's managed world state last moved. 0 when it has not moved this uptime.
    uint32 GetLastUpdateTime(uint32 contributionId) const;

    // Sends SMSG_CONTRIBUTION_LAST_UPDATE_RESPONSE for one contribution to one player. No-op unless enabled.
    void SendLastUpdate(Player* player, uint32 contributionId, uint32 contributionGuid = 0) const;

    // Stamps every contribution fed by this managed world state as updated now and re-acks all in-world players, so
    // a client that is sitting on a collector frame learns its cached data is stale. Timestamps are always recorded;
    // only the wire ack is gated on Warfront.NativeUI.Enable.
    void BroadcastManagedWorldStateUpdate(uint32 managedWorldStateId);

private:
    bool CreatureOffersContribution(uint32 creatureEntry, uint32 contributionId) const;

    std::unordered_map<uint32 /*creatureEntry*/, std::vector<uint32 /*contributionId*/>> _contributionsByCreature;
    // Reverse index built in Load(): ManagedWorldState id -> the contributions whose bar it drives.
    std::unordered_map<uint32 /*managedWorldStateId*/, std::vector<uint32 /*contributionId*/>> _contributionsByManagedWorldState;
    // Last time (unix) each contribution's bar moved - the payload of SMSG_CONTRIBUTION_LAST_UPDATE_RESPONSE.
    std::unordered_map<uint32 /*contributionId*/, uint32 /*lastUpdate*/> _lastUpdateTimes;
};

#define sContributionMgr ContributionMgr::instance()

#endif // TRINITYCORE_CONTRIBUTION_MGR_H
