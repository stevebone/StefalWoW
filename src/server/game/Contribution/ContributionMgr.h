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

private:
    bool CreatureOffersContribution(uint32 creatureEntry, uint32 contributionId) const;

    std::unordered_map<uint32 /*creatureEntry*/, std::vector<uint32 /*contributionId*/>> _contributionsByCreature;
};

#define sContributionMgr ContributionMgr::instance()

#endif // TRINITYCORE_CONTRIBUTION_MGR_H
