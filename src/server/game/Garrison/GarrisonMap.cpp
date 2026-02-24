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
    GarrisonGridLoader(NGridType* grid, GarrisonMap* map, Cell const& cell)
        : i_cell(cell), i_grid(grid), i_map(map), i_garrison(map->GetGarrison()), i_gameObjects(0), i_creatures(0)
    { }

    void Visit(GameObjectMapType& m);
    void Visit(CreatureMapType& m);

    void LoadN();

    template<class T> static void SetObjectCell(T* obj, CellCoord const& cellCoord);
    template<class T> void Visit(GridRefManager<T>& /*m*/) { }

private:
    Cell i_cell;
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
        i_cell.data.Part.cell_y = 0;
        for (uint32 x = 0; x < MAX_NUMBER_OF_CELLS; ++x)
        {
            i_cell.data.Part.cell_x = x;
            for (uint32 y = 0; y < MAX_NUMBER_OF_CELLS; ++y)
            {
                i_cell.data.Part.cell_y = y;

                //Load creatures and game objects
                TypeContainerVisitor<GarrisonGridLoader, GridTypeMapContainer> visitor(*this);
                i_grid->VisitGrid(x, y, visitor);
            }
        }
    }

    TC_LOG_DEBUG("maps", "{} GameObjects and {} Creatures loaded for grid {} on map {}", i_gameObjects, i_creatures, i_grid->GetGridId(), i_map->GetId());
}

void GarrisonGridLoader::Visit(GameObjectMapType& m)
{
    std::vector<Garrison::Plot*> plots = i_garrison->GetPlots();
    if (!plots.empty())
    {
        CellCoord cellCoord = i_cell.GetCellCoord();
        for (Garrison::Plot* plot : plots)
        {
            Position const& spawn = plot->PacketInfo.PlotPos.Pos;
            if (cellCoord != Trinity::ComputeCellCoord(spawn.GetPositionX(), spawn.GetPositionY()))
                continue;

            GameObject* go = plot->CreateGameObject(i_map, i_garrison->GetFaction());
            if (!go)
                continue;

            go->AddToGrid(m);
            ObjectGridLoader::SetObjectCell(go, cellCoord);
            go->AddToWorld();
            ++i_gameObjects;
        }
    }
}

void GarrisonGridLoader::Visit(CreatureMapType& m)
{
    if (!i_garrison)
        return;

    CellCoord cellCoord = i_cell.GetCellCoord();
    GarrisonFactionIndex faction = i_garrison->GetFaction();
    std::vector<Garrison::Plot*> plots = i_garrison->GetPlots();

    if (plots.empty())
        return;

    uint32 plotCount = static_cast<uint32>(plots.size());
    uint32 followerIndex = 0;

    for (auto const& [dbId, follower] : i_garrison->GetFollowerMap())
    {
        // Skip followers currently on missions
        if (follower.PacketInfo.CurrentMissionID != 0)
            continue;

        // Skip inactive followers
        if (follower.PacketInfo.FollowerStatus & FOLLOWER_STATUS_INACTIVE)
            continue;

        GarrFollowerEntry const* followerEntry = sGarrFollowerStore.LookupEntry(follower.PacketInfo.GarrFollowerID);
        if (!followerEntry)
            continue;

        // Only spawn followers matching this garrison type
        if (followerEntry->GarrTypeID != static_cast<int8>(i_garrison->GetType()))
            continue;

        uint32 creatureId = faction == GARRISON_FACTION_INDEX_HORDE ?
            followerEntry->HordeCreatureID : followerEntry->AllianceCreatureID;
        if (!creatureId)
            continue;

        // Determine spawn position
        Position spawnPos;
        bool positionFound = false;

        if (follower.PacketInfo.CurrentBuildingID != 0)
        {
            // Follower is assigned to a building - spawn near that building's plot
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
            // Unassigned follower - scatter around plots
            Garrison::Plot* plot = plots[followerIndex % plotCount];
            spawnPos = plot->PacketInfo.PlotPos.Pos;
            // Distribute followers in a circle around the plot center
            float angle = static_cast<float>(followerIndex % 8) * (float(M_PI) / 4.0f);
            float dist = 5.0f + 2.0f * (followerIndex / 8);
            spawnPos.m_positionX += dist * std::cos(angle);
            spawnPos.m_positionY += dist * std::sin(angle);
        }

        ++followerIndex;

        // Check if this position belongs to current cell
        if (cellCoord != Trinity::ComputeCellCoord(spawnPos.GetPositionX(), spawnPos.GetPositionY()))
            continue;

        Creature* creature = Creature::CreateCreature(creatureId, i_map, spawnPos);
        if (!creature)
            continue;

        creature->SetHomePosition(spawnPos);
        creature->AddToGrid(m);
        ObjectGridLoader::SetObjectCell(creature, cellCoord);
        creature->AddToWorld();
        ++i_creatures;
    }
}

GarrisonMap::GarrisonMap(uint32 id, time_t expiry, uint32 instanceId, ObjectGuid const& owner)
    : Map(id, expiry, instanceId, DIFFICULTY_NORMAL), _owner(owner), _loadingPlayer(nullptr)
{
    GarrisonMap::InitVisibilityDistance();
}

void GarrisonMap::LoadGridObjects(NGridType* grid, Cell const& cell)
{
    Map::LoadGridObjects(grid, cell);

    GarrisonGridLoader loader(grid, this, cell);
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
