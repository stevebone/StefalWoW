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

#include "WarfrontQueue.h"
#include "Log.h"
#include "Map.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include <algorithm>

namespace
{
    // Real ground coordinates on each warfront battle map where the assault party lands. Arathi (1876/1943) carries
    // no DB spawns of its own, so we drop the raid on the shared Arathi terrain near the Stromgarde landing; the
    // Darkshore coords are taken straight from those maps' existing spawn clusters. o = facing.
    //
    // INVARIANT: the final boss must NOT muster on (or near) any of these positions - otherwise it materialises behind
    // the raid on the spot they just spawned on and the assault has no destination. These same coordinates are
    // mirrored into WarfrontBattle::BattleMaps (scripts/Warfronts/warfront_common.h) as Landing*, where a static_assert
    // enforces a >=100 yd separation from the boss muster point. Keep the two tables in sync when either moves.
    struct BattleEntry { float X, Y, Z, O; };

    BattleEntry const* GetBattleEntry(uint32 mapId)
    {
        switch (mapId)
        {
            case 1876: case 1943: { static BattleEntry const e{ -1050.0f, -2370.0f, 53.0f, 3.2f }; return &e; }   // Arathi (Stromgarde)
            case 2105:            { static BattleEntry const e{ 6268.43f, 558.82f, 3.18f, 5.52f }; return &e; }   // Darkshore Alliance
            case 2111:            { static BattleEntry const e{ 6740.36f, 35.31f, 46.65f, 0.64f }; return &e; }   // Darkshore Horde
            default: return nullptr;
        }
    }
}

WarfrontQueue::WarfrontQueue(uint32 warfrontId) :
    _warfrontId(warfrontId), _open(false), _challengerTeam(TEAM_ALLIANCE), _battleMapId(0), _minPlayers(5)
{
}

void WarfrontQueue::Open(TeamId challengerTeam, uint32 battleMapId, uint32 minPlayers)
{
    _enrolled.clear();
    _open = true;
    _challengerTeam = challengerTeam;
    _battleMapId = battleMapId;
    _minPlayers = std::max<uint32>(1, minPlayers);

    TC_LOG_INFO("warfront", "WarfrontQueue[{}] opened for challenger team {} -> battle map {} (min {}).",
        _warfrontId, uint32(_challengerTeam), _battleMapId, _minPlayers);
}

void WarfrontQueue::Close()
{
    if (!_open && _enrolled.empty())
        return;

    _open = false;
    std::size_t const dropped = _enrolled.size();
    _enrolled.clear();

    TC_LOG_INFO("warfront", "WarfrontQueue[{}] closed ({} enrolled players dropped).", _warfrontId, dropped);
}

bool WarfrontQueue::Enqueue(Player* player)
{
    if (!_open || !player)
        return false;

    ObjectGuid const guid = player->GetGUID();
    if (IsEnrolled(guid))
        return false;

    _enrolled.push_back(guid);

    TC_LOG_DEBUG("warfront", "WarfrontQueue[{}] enrolled player {} ({}) - {} now waiting.",
        _warfrontId, player->GetName(), guid.ToString(), _enrolled.size());

    // P3 will act on readiness here (form the battle group when enough are waiting). For P2 we only report it.
    if (IsReadyToForm())
        FormBattleGroup();

    return true;
}

void WarfrontQueue::Dequeue(ObjectGuid guid)
{
    _enrolled.erase(std::remove(_enrolled.begin(), _enrolled.end(), guid), _enrolled.end());
}

bool WarfrontQueue::IsEnrolled(ObjectGuid guid) const
{
    return std::find(_enrolled.begin(), _enrolled.end(), guid) != _enrolled.end();
}

bool WarfrontQueue::IsReadyToForm() const
{
    return _open && _enrolled.size() >= _minPlayers;
}

bool WarfrontQueue::FormBattleGroup()
{
    if (!IsReadyToForm())
        return false;

    BattleEntry const* entry = GetBattleEntry(_battleMapId);
    if (!entry)
    {
        TC_LOG_ERROR("warfront", "WarfrontQueue[{}] cannot form a battle group: no landing coordinates for battle map {}.",
            _warfrontId, _battleMapId);
        return false;
    }

    // Only players still online and in the world can actually be pulled into the assault.
    std::vector<Player*> party;
    party.reserve(_enrolled.size());
    for (ObjectGuid const& guid : _enrolled)
        if (Player* p = ObjectAccessor::FindConnectedPlayer(guid))
            if (p->IsInWorld())
                party.push_back(p);

    if (party.size() < _minPlayers)
        return false;   // enough enrolled on paper, but not enough are online right now to launch

    // Materialize ONE instanced copy of the scenario battle map, then send everyone into that exact instance.
    // MapManager::CreateMap is the standard instance-creation entry point: for a warfront battle map (Map.db2
    // InstanceType 5 = scenario) it builds the InstanceMap, binds the InstanceScript (instance_template.script) and
    // auto-attaches the faction scenario (scenarios row -> ScenarioMgr::CreateInstanceScenarioForTeam). We anchor it
    // on the first player so the instance resolves that player's difficulty (Warfront Normal 147), then teleport
    // every enrolled player into that instance id.
    Map* map = sMapMgr->CreateMap(_battleMapId, party.front());
    InstanceMap* instance = map ? map->ToInstanceMap() : nullptr;
    if (!instance)
    {
        TC_LOG_ERROR("warfront", "WarfrontQueue[{}] failed to create the battle instance for map {}.", _warfrontId, _battleMapId);
        return false;
    }

    uint32 const instanceId = instance->GetInstanceId();
    for (Player* p : party)
    {
        // Bind each player to this instance id so the far-teleport (which looks the map up by id rather than
        // re-creating it) lands them all in the same copy.
        p->SetRecentInstance(_battleMapId, instanceId);
        p->TeleportTo(_battleMapId, entry->X, entry->Y, entry->Z, entry->O, TELE_TO_NONE, instanceId);
    }

    TC_LOG_INFO("warfront", "WarfrontQueue[{}] launched assault: {} players -> map {} instance {} (challenger team {}).",
        _warfrontId, party.size(), _battleMapId, instanceId, uint32(_challengerTeam));

    // The party has launched; clear enrollment. The queue stays open for the remainder of the siege window so a
    // fresh group can muster for another run.
    _enrolled.clear();
    return true;
}
