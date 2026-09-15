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

#ifndef TRINITYCORE_WARFRONT_QUEUE_H
#define TRINITYCORE_WARFRONT_QUEUE_H

#include "Define.h"
#include "ObjectGuid.h"
#include "SharedDefines.h"      // TeamId
#include <vector>

class Player;

// A single-team enrollment queue for one warfront's assault scenario. A Warfront is one faction versus NPCs, so
// unlike BattlegroundQueue there is no cross-faction matchmaking: exactly one team (the challenger) fills. This is
// the "single-team fill" role BattlegroundQueue::AddWargameSide plays for war games, reduced to what a warfront
// needs. State is purely in-memory (like the BG queue) - nothing is persisted. Owned by WarfrontMgr (one per
// warfront). See WARFRONTS_DESIGN.md §3.
class TC_GAME_API WarfrontQueue
{
public:
    explicit WarfrontQueue(uint32 warfrontId);

    // Opens the queue for the given challenging team, routing pops to the map that faction assaults (1943/1876 for
    // Arathi, 2105/2111 for Darkshore). Called by WarfrontMgr when the zone enters SIEGE. Re-opening for a new
    // challenger clears any stale enrollment.
    void Open(TeamId challengerTeam, uint32 battleMapId, uint32 minPlayers);

    // Closes the queue and drops every enrolled player (called on flip / siege expiry).
    void Close();

    bool IsOpen() const { return _open; }
    TeamId GetChallengerTeam() const { return _challengerTeam; }
    uint32 GetBattleMapId() const { return _battleMapId; }
    uint32 GetMinPlayers() const { return _minPlayers; }
    uint32 GetWarfrontId() const { return _warfrontId; }

    // Enrolls a player. Returns true if newly enrolled, false if the queue is closed or the player was already in.
    // Caller (WarfrontMgr::EnqueuePlayer) is responsible for eligibility gating via CanQueue.
    bool Enqueue(Player* player);

    // Removes a player from the queue (leave / logout / dequeue). No-op if not enrolled.
    void Dequeue(ObjectGuid guid);

    bool IsEnrolled(ObjectGuid guid) const;
    std::size_t GetEnrolledCount() const { return _enrolled.size(); }
    std::vector<ObjectGuid> const& GetEnrolled() const { return _enrolled; }

    // Ready once at least minPlayers are enrolled.
    bool IsReadyToForm() const;

    // When enough enrolled players are online, materialize one instanced copy of GetBattleMapId() (which auto-binds
    // the InstanceScript + faction scenario) and teleport the party into that exact instance, then clear enrollment.
    // Returns true when a group launched. No-op (returns false) if fewer than minPlayers are actually online.
    bool FormBattleGroup();

private:
    uint32 _warfrontId;
    bool   _open;
    TeamId _challengerTeam;
    uint32 _battleMapId;
    uint32 _minPlayers;
    std::vector<ObjectGuid> _enrolled;      // preserves join order; small N, linear scans are fine
};

#endif // TRINITYCORE_WARFRONT_QUEUE_H
