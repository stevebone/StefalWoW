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

#include "GarrisonMap.h"
#include "Creature.h"
#include "DB2Stores.h"
#include "DBCEnums.h"
#include "GameObject.h"
#include "Garrison.h"
#include "Group.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "ObjectGridLoader.h"
#include "Player.h"
#include "World.h"

class GarrisonGridLoader
{
public:
    GarrisonGridLoader(NGridType* grid, GarrisonMap* map)
        : i_grid(grid), i_map(map), i_garrison(map->GetGarrison()), i_gameObjects(0), i_creatures(0)
    { }

    void LoadN();

private:
    NGridType* i_grid;
    GarrisonMap* i_map;
    Garrison* i_garrison;
    uint32 i_gameObjects;
    uint32 i_creatures;
};

void GarrisonGridLoader::LoadN()
{
    if (i_garrison)
    {
        for (Garrison::Plot* plot : i_garrison->GetPlots())
        {
            // LoadN() runs once PER GRID (EnsureGridLoaded -> LoadGridObjects -> LoadN). This loader
            // synthesizes its objects from the owner's Garrison state instead of a grid-local GUID set, so it
            // must skip plots whose position lies in another grid - otherwise every grid load re-spawns the
            // whole garrison (N loaded grids -> N stacked copies, since AddToMap places by object position,
            // not by the grid being loaded). Upstream's generic ObjectGridLoader gets this for free.
            Position const& plotPos = plot->PacketInfo.PlotPos.Pos;
            GridCoord const plotGrid = Trinity::ComputeGridCoord(plotPos.GetPositionX(), plotPos.GetPositionY());
            if (plotGrid.x_coord != uint32(i_grid->getX()) || plotGrid.y_coord != uint32(i_grid->getY()))
                continue;

            GameObject* go = plot->CreateGameObject(i_map, i_garrison->GetFaction());
            if (!go)
                continue;

            ObjectGridLoaderBase::AddToMap(go, i_map, i_gameObjects);
        }

        // Spawn active garrison followers as creatures near their plots. Branch feature, re-hosted into
        // 12.1's rewritten LoadN(): upstream dropped the per-cell Visit(CreatureMapType&) mechanism and now
        // loads the whole grid inline, so the followers are spawned here alongside the plot gameobjects, via
        // the same ObjectGridLoaderBase::AddToMap path (AddToGrid + AddToWorld). The old per-cell filter that
        // the Visit mechanism provided is replaced below by a per-grid filter (see the plot loop above).
        std::vector<Garrison::Plot*> plots = i_garrison->GetPlots();
        if (!plots.empty())
        {
            GarrisonFactionIndex faction = i_garrison->GetFaction();
            uint32 plotCount = static_cast<uint32>(plots.size());
            uint32 followerIndex = 0;
            for (auto const& [dbId, follower] : i_garrison->GetFollowerMap())
            {
                if (follower.PacketInfo.CurrentMissionID != 0)                       // on a mission
                    continue;
                if (follower.PacketInfo.FollowerStatus & FOLLOWER_STATUS_INACTIVE)   // inactive
                    continue;
                GarrFollowerEntry const* followerEntry = sGarrFollowerStore.LookupEntry(follower.PacketInfo.GarrFollowerID);
                if (!followerEntry)
                    continue;
                if (followerEntry->GarrTypeID != static_cast<int8>(i_garrison->GetType()))
                    continue;
                uint32 creatureId = faction == GARRISON_FACTION_INDEX_HORDE ?
                    followerEntry->HordeCreatureID : followerEntry->AllianceCreatureID;
                if (!creatureId)
                    continue;

                Position spawnPos;
                bool positionFound = false;
                if (follower.PacketInfo.CurrentBuildingID != 0)
                {
                    for (Garrison::Plot* plot : plots)
                    {
                        if (plot->BuildingInfo.PacketInfo &&
                            plot->BuildingInfo.PacketInfo->GarrBuildingID == follower.PacketInfo.CurrentBuildingID)
                        {
                            spawnPos = plot->PacketInfo.PlotPos.Pos;
                            spawnPos.RelocateOffset({ 3.0f, 3.0f, 0.0f, 0.0f });
                            positionFound = true;
                            break;
                        }
                    }
                }
                if (!positionFound)
                {
                    Garrison::Plot* plot = plots[followerIndex % plotCount];
                    spawnPos = plot->PacketInfo.PlotPos.Pos;
                    float angle = static_cast<float>(followerIndex % 8) * (float(M_PI) / 4.0f);
                    float dist = 5.0f + 2.0f * (followerIndex / 8);
                    spawnPos.m_positionX += dist * std::cos(angle);
                    spawnPos.m_positionY += dist * std::sin(angle);
                }
                ++followerIndex;   // advanced before the grid check, so the scatter is grid-independent

                // Same per-grid guard as the plots above: only spawn this follower when its computed position
                // falls in the grid being loaded.
                GridCoord const followerGrid = Trinity::ComputeGridCoord(spawnPos.GetPositionX(), spawnPos.GetPositionY());
                if (followerGrid.x_coord != uint32(i_grid->getX()) || followerGrid.y_coord != uint32(i_grid->getY()))
                    continue;

                Creature* creature = Creature::CreateCreature(creatureId, i_map, spawnPos);
                if (!creature)
                    continue;
                creature->SetHomePosition(spawnPos);
                ObjectGridLoaderBase::AddToMap(creature, i_map, i_creatures);
            }
        }
    }

    TC_LOG_DEBUG("maps", "{} GameObjects and {} Creatures loaded for grid {} on map {}", i_gameObjects, i_creatures, i_grid->GetGridId(), i_map->GetId());
}

GarrisonMap::GarrisonMap(uint32 id, time_t expiry, uint32 instanceId, ObjectGuid const& owner)
    : Map(id, expiry, instanceId, DIFFICULTY_NORMAL), _owner(owner), _loadingPlayer(nullptr)
{
    GarrisonMap::InitVisibilityDistance();
}

void GarrisonMap::LoadGridObjects(NGridType* grid)
{
    Map::LoadGridObjects(grid);

    GarrisonGridLoader loader(grid, this);
    loader.LoadN();
}

Garrison* GarrisonMap::GetGarrison()
{
    if (_loadingPlayer)
        return _loadingPlayer->GetGarrison();

    if (Player* owner = ObjectAccessor::FindConnectedPlayer(_owner))
        return owner->GetGarrison();

    return nullptr;
}

void GarrisonMap::InitVisibilityDistance()
{
    //init visibility distance for instances
    m_VisibleDistance = sWorld->getFloatConfig(CONFIG_MAX_VISIBILITY_DISTANCE_INSTANCE);
    m_VisibilityNotifyPeriod = sWorld->getIntConfig(CONFIG_VISIBILITY_NOTIFY_PERIOD_INSTANCE);
}

bool GarrisonMap::AddPlayerToMap(Player* player, bool initPlayer /*= true*/)
{
    if (player->GetGUID() == _owner)
        _loadingPlayer = player;
    else
    {
        // Allow party members to enter if they are in the same group as the owner
        // and the party garrison feature is enabled
        Player* owner = ObjectAccessor::FindConnectedPlayer(_owner);
        if (!owner)
            return false;

        Group* group = player->GetGroup();
        if (!group || !group->IsMember(owner->GetGUID()))
        {
            TC_LOG_DEBUG("garrison", "Player {} denied entry to garrison map {} - not in owner's group",
                player->GetGUID().ToString().c_str(), GetId());
            return false;
        }
    }

    bool result = Map::AddPlayerToMap(player, initPlayer);

    if (player->GetGUID() == _owner)
        _loadingPlayer = nullptr;

    return result;
}
