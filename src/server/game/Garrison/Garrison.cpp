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

#include "Garrison.h"
#include "Containers.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "DB2Stores.h"
#include "GameObject.h"
#include "GameTime.h"
#include "GarrisonMgr.h"
#include "Item.h"
#include "Log.h"
#include "Mail.h"
#include "Map.h"
#include "MapManager.h"
#include "ObjectMgr.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "Random.h"
#include "VehicleDefines.h"
#include "advstd.h"

Garrison::Garrison(Player* owner) : _owner(owner), _siteLevel(nullptr), _followerActivationsRemainingToday(1)
{
}

bool Garrison::LoadFromDB(PreparedQueryResult garrison, PreparedQueryResult blueprints, PreparedQueryResult buildings,
    PreparedQueryResult followers, PreparedQueryResult abilities, PreparedQueryResult missions)
{
    if (!garrison)
        return false;

    Field* fields = garrison->Fetch();
    _siteLevel = sGarrSiteLevelStore.LookupEntry(fields[0].GetUInt32());
    _followerActivationsRemainingToday = fields[1].GetUInt32();
    if (!_siteLevel)
        return false;

    InitializePlots();

    if (blueprints)
    {
        do
        {
            fields = blueprints->Fetch();
            if (GarrBuildingEntry const* building = sGarrBuildingStore.LookupEntry(fields[0].GetUInt32()))
                _knownBuildings.insert(building->ID);

        } while (blueprints->NextRow());
    }

    if (buildings)
    {
        do
        {
            fields = buildings->Fetch();
            uint32 plotInstanceId = fields[0].GetUInt32();
            uint32 buildingId = fields[1].GetUInt32();
            time_t timeBuilt = fields[2].GetInt64();
            bool active = fields[3].GetBool();

            Plot* plot = GetPlot(plotInstanceId);
            if (!plot)
                continue;

            if (!sGarrBuildingStore.LookupEntry(buildingId))
                continue;

            plot->BuildingInfo.PacketInfo.emplace();
            plot->BuildingInfo.PacketInfo->GarrPlotInstanceID = plotInstanceId;
            plot->BuildingInfo.PacketInfo->GarrBuildingID = buildingId;
            plot->BuildingInfo.PacketInfo->TimeBuilt = timeBuilt;
            plot->BuildingInfo.PacketInfo->Active = active;

        } while (buildings->NextRow());
    }

    //           0           1        2      3                4               5   6                7               8       9
    // SELECT dbId, followerId, quality, level, itemLevelWeapon, itemLevelArmor, xp, currentBuilding, currentMission, status FROM character_garrison_followers WHERE guid = ?
    if (followers)
    {
        do
        {
            fields = followers->Fetch();

            uint64 dbId = fields[0].GetUInt64();
            uint32 followerId = fields[1].GetUInt32();
            if (!sGarrFollowerStore.LookupEntry(followerId))
                continue;

            _followerIds.insert(followerId);
            Follower& follower = _followers[dbId];
            follower.PacketInfo.DbID = dbId;
            follower.PacketInfo.GarrFollowerID = followerId;
            follower.PacketInfo.Quality = fields[2].GetUInt32();
            follower.PacketInfo.FollowerLevel = fields[3].GetUInt32();
            follower.PacketInfo.ItemLevelWeapon = fields[4].GetUInt32();
            follower.PacketInfo.ItemLevelArmor = fields[5].GetUInt32();
            follower.PacketInfo.Xp = fields[6].GetUInt32();
            follower.PacketInfo.CurrentBuildingID = fields[7].GetUInt32();
            follower.PacketInfo.CurrentMissionID = fields[8].GetUInt32();
            follower.PacketInfo.FollowerStatus = fields[9].GetUInt32();
            if (!sGarrBuildingStore.LookupEntry(follower.PacketInfo.CurrentBuildingID))
                follower.PacketInfo.CurrentBuildingID = 0;

            //if (!sGarrMissionStore.LookupEntry(follower.PacketInfo.CurrentMissionID))
            //    follower.PacketInfo.CurrentMissionID = 0;

        } while (followers->NextRow());

        if (abilities)
        {
            do
            {
                fields = abilities->Fetch();
                uint64 dbId = fields[0].GetUInt64();
                GarrAbilityEntry const* ability = sGarrAbilityStore.LookupEntry(fields[1].GetUInt32());

                if (!ability)
                    continue;

                auto itr = _followers.find(dbId);
                if (itr == _followers.end())
                    continue;

                itr->second.PacketInfo.AbilityID.push_back(ability);
            } while (abilities->NextRow());
        }
    }

    //           0      1            2          3              4          5                6              7               8        9
    // SELECT dbId, guid, missionRecID, offerTime, offerDuration, startTime, travelDuration, missionDuration, missionState, successChance FROM character_garrison_missions WHERE guid = ?
    if (missions)
    {
        do
        {
            fields = missions->Fetch();

            uint64 dbId = fields[0].GetUInt64();
            uint32 missionRecID = fields[2].GetUInt32();

            if (!sGarrMissionStore.LookupEntry(missionRecID))
                continue;

            if (_missionDbIdGenerator <= dbId)
                _missionDbIdGenerator = dbId + 1;

            Mission& mission = _missions[dbId];
            mission.PacketInfo.DbID = dbId;
            mission.PacketInfo.MissionRecID = missionRecID;
            mission.PacketInfo.OfferTime = fields[3].GetInt64();
            mission.PacketInfo.OfferDuration = Seconds(fields[4].GetInt32());
            mission.PacketInfo.StartTime = fields[5].GetInt64();
            mission.PacketInfo.TravelDuration = Seconds(fields[6].GetInt32());
            mission.PacketInfo.MissionDuration = Seconds(fields[7].GetInt32());
            mission.PacketInfo.MissionState = fields[8].GetInt32();
            mission.PacketInfo.SuccessChance = fields[9].GetInt32();

            GarrMissionEntry const* missionEntry = sGarrMissionStore.LookupEntry(missionRecID);
            if (missionEntry)
                mission.PacketInfo.MissionScalar = missionEntry->AutoMissionScalar;

        } while (missions->NextRow());
    }

    return true;
}

void Garrison::SaveToDB(CharacterDatabaseTransaction trans)
{
    DeleteFromDB(_owner->GetGUID().GetCounter(), trans);

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHARACTER_GARRISON);
    stmt->setUInt64(0, _owner->GetGUID().GetCounter());
    stmt->setUInt32(1, _siteLevel->ID);
    stmt->setUInt32(2, _followerActivationsRemainingToday);
    trans->Append(stmt);

    for (uint32 building : _knownBuildings)
    {
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHARACTER_GARRISON_BLUEPRINTS);
        stmt->setUInt64(0, _owner->GetGUID().GetCounter());
        stmt->setUInt32(1, building);
        trans->Append(stmt);
    }

    for (auto const& p : _plots)
    {
        Plot const& plot = p.second;
        if (plot.BuildingInfo.PacketInfo)
        {
            stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHARACTER_GARRISON_BUILDINGS);
            stmt->setUInt64(0, _owner->GetGUID().GetCounter());
            stmt->setUInt32(1, plot.BuildingInfo.PacketInfo->GarrPlotInstanceID);
            stmt->setUInt32(2, plot.BuildingInfo.PacketInfo->GarrBuildingID);
            stmt->setInt64(3, plot.BuildingInfo.PacketInfo->TimeBuilt);
            stmt->setBool(4, plot.BuildingInfo.PacketInfo->Active);
            trans->Append(stmt);
        }
    }

    for (auto const& p : _followers)
    {
        Follower const& follower = p.second;
        uint8 index = 0;
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHARACTER_GARRISON_FOLLOWERS);
        stmt->setUInt64(index++, follower.PacketInfo.DbID);
        stmt->setUInt64(index++, _owner->GetGUID().GetCounter());
        stmt->setUInt32(index++, follower.PacketInfo.GarrFollowerID);
        stmt->setUInt32(index++, follower.PacketInfo.Quality);
        stmt->setUInt32(index++, follower.PacketInfo.FollowerLevel);
        stmt->setUInt32(index++, follower.PacketInfo.ItemLevelWeapon);
        stmt->setUInt32(index++, follower.PacketInfo.ItemLevelArmor);
        stmt->setUInt32(index++, follower.PacketInfo.Xp);
        stmt->setUInt32(index++, follower.PacketInfo.CurrentBuildingID);
        stmt->setUInt32(index++, follower.PacketInfo.CurrentMissionID);
        stmt->setUInt32(index++, follower.PacketInfo.FollowerStatus);
        trans->Append(stmt);

        uint8 slot = 0;
        for (GarrAbilityEntry const* ability : follower.PacketInfo.AbilityID)
        {
            stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHARACTER_GARRISON_FOLLOWER_ABILITIES);
            stmt->setUInt64(0, follower.PacketInfo.DbID);
            stmt->setUInt32(1, ability->ID);
            stmt->setUInt8(2, slot++);
            trans->Append(stmt);
        }
    }

    for (auto const& p : _missions)
    {
        Mission const& mission = p.second;
        uint8 index = 0;
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHARACTER_GARRISON_MISSIONS);
        stmt->setUInt64(index++, mission.PacketInfo.DbID);
        stmt->setUInt64(index++, _owner->GetGUID().GetCounter());
        stmt->setUInt32(index++, mission.PacketInfo.MissionRecID);
        stmt->setInt64(index++, mission.PacketInfo.OfferTime);
        stmt->setInt32(index++, static_cast<int32>(Seconds(mission.PacketInfo.OfferDuration).count()));
        stmt->setInt64(index++, mission.PacketInfo.StartTime);
        stmt->setInt32(index++, static_cast<int32>(Seconds(mission.PacketInfo.TravelDuration).count()));
        stmt->setInt32(index++, static_cast<int32>(Seconds(mission.PacketInfo.MissionDuration).count()));
        stmt->setInt32(index++, mission.PacketInfo.MissionState);
        stmt->setInt32(index++, mission.PacketInfo.SuccessChance);
        trans->Append(stmt);
    }
}

void Garrison::DeleteFromDB(ObjectGuid::LowType ownerGuid, CharacterDatabaseTransaction trans)
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHARACTER_GARRISON);
    stmt->setUInt64(0, ownerGuid);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHARACTER_GARRISON_BLUEPRINTS);
    stmt->setUInt64(0, ownerGuid);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHARACTER_GARRISON_BUILDINGS);
    stmt->setUInt64(0, ownerGuid);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHARACTER_GARRISON_FOLLOWERS);
    stmt->setUInt64(0, ownerGuid);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHARACTER_GARRISON_MISSIONS);
    stmt->setUInt64(0, ownerGuid);
    trans->Append(stmt);
}

bool Garrison::Create(uint32 garrSiteId)
{
    GarrSiteLevelEntry const* siteLevel = sGarrisonMgr.GetGarrSiteLevelEntry(garrSiteId, 1);
    if (!siteLevel)
        return false;

    _siteLevel = siteLevel;

    InitializePlots();

    WorldPackets::Garrison::GarrisonCreateResult garrisonCreateResult;
    garrisonCreateResult.GarrSiteLevelID = _siteLevel->ID;
    _owner->SendDirectMessage(garrisonCreateResult.Write());
    PhasingHandler::OnConditionChange(_owner);
    SendRemoteInfo();
    return true;
}

void Garrison::Delete()
{
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    DeleteFromDB(_owner->GetGUID().GetCounter(), trans);
    CharacterDatabase.CommitTransaction(trans);

    WorldPackets::Garrison::GarrisonDeleteResult garrisonDelete;
    garrisonDelete.Result = GARRISON_SUCCESS;
    garrisonDelete.GarrSiteID = _siteLevel->GarrSiteID;
    _owner->SendDirectMessage(garrisonDelete.Write());
}

void Garrison::InitializePlots()
{
    if (std::vector<GarrSiteLevelPlotInstEntry const*> const* plots = sGarrisonMgr.GetGarrPlotInstForSiteLevel(_siteLevel->ID))
    {
        for (std::size_t i = 0; i < plots->size(); ++i)
        {
            uint32 garrPlotInstanceId = plots->at(i)->GarrPlotInstanceID;
            GarrPlotInstanceEntry const* plotInstance = sGarrPlotInstanceStore.LookupEntry(garrPlotInstanceId);
            GameObjectsEntry const* gameObject = sGarrisonMgr.GetPlotGameObject(_siteLevel->MapID, garrPlotInstanceId);
            if (!plotInstance || !gameObject)
                continue;

            GarrPlotEntry const* plot = sGarrPlotStore.LookupEntry(plotInstance->GarrPlotID);
            if (!plot)
                continue;

            Plot& plotInfo = _plots[garrPlotInstanceId];
            plotInfo.PacketInfo.GarrPlotInstanceID = garrPlotInstanceId;
            plotInfo.PacketInfo.PlotPos = Position(gameObject->Pos.X, gameObject->Pos.Y, gameObject->Pos.Z, 2 * std::acos(gameObject->Rot[3]));
            plotInfo.PacketInfo.PlotType = plot->PlotType;
            plotInfo.Rotation = QuaternionData(gameObject->Rot[0], gameObject->Rot[1], gameObject->Rot[2], gameObject->Rot[3]);
            plotInfo.EmptyGameObjectId = gameObject->ID;
            plotInfo.GarrSiteLevelPlotInstId = plots->at(i)->ID;
        }
    }
}

void Garrison::Upgrade()
{
}

void Garrison::Enter() const
{
    if (MapEntry const* map = sMapStore.LookupEntry(_siteLevel->MapID))
        if (int32(_owner->GetMapId()) == map->ParentMapID)
            _owner->TeleportTo(WorldLocation(_siteLevel->MapID, *_owner), TELE_TO_SEAMLESS);
}

void Garrison::Leave() const
{
    if (MapEntry const* map = sMapStore.LookupEntry(_siteLevel->MapID))
        if (_owner->GetMapId() == _siteLevel->MapID)
            _owner->TeleportTo(WorldLocation(map->ParentMapID, *_owner), TELE_TO_SEAMLESS);
}

GarrisonFactionIndex Garrison::GetFaction() const
{
    return GetFaction(_owner->GetTeam());
}

std::vector<Garrison::Plot*> Garrison::GetPlots()
{
    std::vector<Plot*> plots;
    plots.reserve(_plots.size());
    for (auto& p : _plots)
        plots.push_back(&p.second);

    return plots;
}

Garrison::Plot* Garrison::GetPlot(uint32 garrPlotInstanceId)
{
    auto itr = _plots.find(garrPlotInstanceId);
    if (itr != _plots.end())
        return &itr->second;

    return nullptr;
}

Garrison::Plot const* Garrison::GetPlot(uint32 garrPlotInstanceId) const
{
    auto itr = _plots.find(garrPlotInstanceId);
    if (itr != _plots.end())
        return &itr->second;

    return nullptr;
}

void Garrison::LearnBlueprint(uint32 garrBuildingId)
{
    WorldPackets::Garrison::GarrisonLearnBlueprintResult learnBlueprintResult;
    learnBlueprintResult.GarrTypeID = GetType();
    learnBlueprintResult.BuildingID = garrBuildingId;
    learnBlueprintResult.Result = GARRISON_SUCCESS;

    if (!sGarrBuildingStore.LookupEntry(garrBuildingId))
        learnBlueprintResult.Result = GARRISON_ERROR_INVALID_BUILDINGID;
    else if (HasBlueprint(garrBuildingId))
        learnBlueprintResult.Result = GARRISON_ERROR_BLUEPRINT_EXISTS;
    else
        _knownBuildings.insert(garrBuildingId);

    _owner->SendDirectMessage(learnBlueprintResult.Write());
}

void Garrison::UnlearnBlueprint(uint32 garrBuildingId)
{
    WorldPackets::Garrison::GarrisonUnlearnBlueprintResult unlearnBlueprintResult;
    unlearnBlueprintResult.GarrTypeID = GetType();
    unlearnBlueprintResult.BuildingID = garrBuildingId;
    unlearnBlueprintResult.Result = GARRISON_SUCCESS;

    if (!sGarrBuildingStore.LookupEntry(garrBuildingId))
        unlearnBlueprintResult.Result = GARRISON_ERROR_INVALID_BUILDINGID;
    else if (!HasBlueprint(garrBuildingId))
        unlearnBlueprintResult.Result = GARRISON_ERROR_REQUIRES_BLUEPRINT;
    else
        _knownBuildings.erase(garrBuildingId);

    _owner->SendDirectMessage(unlearnBlueprintResult.Write());
}

void Garrison::PlaceBuilding(uint32 garrPlotInstanceId, uint32 garrBuildingId)
{
    WorldPackets::Garrison::GarrisonPlaceBuildingResult placeBuildingResult;
    placeBuildingResult.GarrTypeID = GetType();
    placeBuildingResult.Result = CheckBuildingPlacement(garrPlotInstanceId, garrBuildingId);
    if (placeBuildingResult.Result == GARRISON_SUCCESS)
    {
        placeBuildingResult.BuildingInfo.GarrPlotInstanceID = garrPlotInstanceId;
        placeBuildingResult.BuildingInfo.GarrBuildingID = garrBuildingId;
        placeBuildingResult.BuildingInfo.TimeBuilt = GameTime::GetGameTime();

        Plot* plot = GetPlot(garrPlotInstanceId);
        uint32 oldBuildingId = 0;
        Map* map = FindMap();
        GarrBuildingEntry const* building = sGarrBuildingStore.AssertEntry(garrBuildingId);
        if (map)
            plot->DeleteGameObject(map);

        if (plot->BuildingInfo.PacketInfo)
        {
            oldBuildingId = plot->BuildingInfo.PacketInfo->GarrBuildingID;
            if (sGarrBuildingStore.AssertEntry(oldBuildingId)->BuildingType != building->BuildingType)
                plot->ClearBuildingInfo(GetType(), _owner);
        }

        plot->SetBuildingInfo(placeBuildingResult.BuildingInfo, _owner);
        if (map)
            if (GameObject* go = plot->CreateGameObject(map, GetFaction()))
                map->AddToMap(go);

        _owner->RemoveCurrency(building->CurrencyTypeID, building->CurrencyQty, CurrencyDestroyReason::Garrison);
        _owner->ModifyMoney(-building->GoldCost * GOLD, false);

        if (oldBuildingId)
        {
            WorldPackets::Garrison::GarrisonBuildingRemoved buildingRemoved;
            buildingRemoved.GarrTypeID = GetType();
            buildingRemoved.Result = GARRISON_SUCCESS;
            buildingRemoved.GarrPlotInstanceID = garrPlotInstanceId;
            buildingRemoved.GarrBuildingID = oldBuildingId;
            _owner->SendDirectMessage(buildingRemoved.Write());
        }

        _owner->UpdateCriteria(CriteriaType::PlaceGarrisonBuilding, garrBuildingId);
    }

    _owner->SendDirectMessage(placeBuildingResult.Write());
}

void Garrison::CancelBuildingConstruction(uint32 garrPlotInstanceId)
{
    WorldPackets::Garrison::GarrisonBuildingRemoved buildingRemoved;
    buildingRemoved.GarrTypeID = GetType();
    buildingRemoved.Result = CheckBuildingRemoval(garrPlotInstanceId);
    if (buildingRemoved.Result == GARRISON_SUCCESS)
    {
        Plot* plot = GetPlot(garrPlotInstanceId);

        buildingRemoved.GarrPlotInstanceID = garrPlotInstanceId;
        buildingRemoved.GarrBuildingID = plot->BuildingInfo.PacketInfo->GarrBuildingID;

        Map* map = FindMap();
        if (map)
            plot->DeleteGameObject(map);

        plot->ClearBuildingInfo(GetType(), _owner);
        _owner->SendDirectMessage(buildingRemoved.Write());

        GarrBuildingEntry const* constructing = sGarrBuildingStore.AssertEntry(buildingRemoved.GarrBuildingID);
        // Refund construction/upgrade cost
        _owner->AddCurrency(constructing->CurrencyTypeID, constructing->CurrencyQty, CurrencyGainSource::GarrisonBuildingRefund);
        _owner->ModifyMoney(constructing->GoldCost * GOLD, false);

        if (constructing->UpgradeLevel > 1)
        {
            // Restore previous level building
            uint32 restored = sGarrisonMgr.GetPreviousLevelBuildingId(constructing->BuildingType, constructing->UpgradeLevel);
            ASSERT(restored);

            WorldPackets::Garrison::GarrisonPlaceBuildingResult placeBuildingResult;
            placeBuildingResult.GarrTypeID = GetType();
            placeBuildingResult.Result = GARRISON_SUCCESS;
            placeBuildingResult.BuildingInfo.GarrPlotInstanceID = garrPlotInstanceId;
            placeBuildingResult.BuildingInfo.GarrBuildingID = restored;
            placeBuildingResult.BuildingInfo.TimeBuilt = GameTime::GetGameTime();
            placeBuildingResult.BuildingInfo.Active = true;

            plot->SetBuildingInfo(placeBuildingResult.BuildingInfo, _owner);
            _owner->SendDirectMessage(placeBuildingResult.Write());
        }

        if (map)
            if (GameObject* go = plot->CreateGameObject(map, GetFaction()))
                map->AddToMap(go);
    }
    else
        _owner->SendDirectMessage(buildingRemoved.Write());
}

void Garrison::ActivateBuilding(uint32 garrPlotInstanceId)
{
    if (Plot* plot = GetPlot(garrPlotInstanceId))
    {
        if (plot->BuildingInfo.CanActivate() && plot->BuildingInfo.PacketInfo && !plot->BuildingInfo.PacketInfo->Active)
        {
            plot->BuildingInfo.PacketInfo->Active = true;
            if (Map* map = FindMap())
            {
                plot->DeleteGameObject(map);
                if (GameObject* go = plot->CreateGameObject(map, GetFaction()))
                    map->AddToMap(go);
            }

            WorldPackets::Garrison::GarrisonBuildingActivated buildingActivated;
            buildingActivated.GarrPlotInstanceID = garrPlotInstanceId;
            _owner->SendDirectMessage(buildingActivated.Write());

            _owner->UpdateCriteria(CriteriaType::ActivateAnyGarrisonBuilding, plot->BuildingInfo.PacketInfo->GarrBuildingID);
        }
    }
}

void Garrison::AddFollower(uint32 garrFollowerId)
{
    WorldPackets::Garrison::GarrisonAddFollowerResult addFollowerResult;
    addFollowerResult.GarrTypeID = GetType();
    GarrFollowerEntry const* followerEntry = sGarrFollowerStore.LookupEntry(garrFollowerId);
    if (_followerIds.count(garrFollowerId) || !followerEntry)
    {
        addFollowerResult.Result = GARRISON_ERROR_FOLLOWER_EXISTS;
        _owner->SendDirectMessage(addFollowerResult.Write());
        return;
    }

    _followerIds.insert(garrFollowerId);
    uint64 dbId = sGarrisonMgr.GenerateFollowerDbId();
    Follower& follower = _followers[dbId];
    follower.PacketInfo.DbID = dbId;
    follower.PacketInfo.GarrFollowerID = garrFollowerId;
    follower.PacketInfo.Quality = followerEntry->Quality;   // TODO: handle magic upgrades
    follower.PacketInfo.FollowerLevel = followerEntry->FollowerLevel;
    follower.PacketInfo.ItemLevelWeapon = followerEntry->ItemLevelWeapon;
    follower.PacketInfo.ItemLevelArmor = followerEntry->ItemLevelArmor;
    follower.PacketInfo.Xp = 0;
    follower.PacketInfo.CurrentBuildingID = 0;
    follower.PacketInfo.CurrentMissionID = 0;
    follower.PacketInfo.AbilityID = sGarrisonMgr.RollFollowerAbilities(garrFollowerId, followerEntry, follower.PacketInfo.Quality, GetFaction(), true);
    follower.PacketInfo.FollowerStatus = 0;

    addFollowerResult.Follower = follower.PacketInfo;
    _owner->SendDirectMessage(addFollowerResult.Write());

    _owner->UpdateCriteria(CriteriaType::RecruitGarrisonFollower, follower.PacketInfo.DbID);
}

Garrison::Follower const* Garrison::GetFollower(uint64 dbId) const
{
    auto itr = _followers.find(dbId);
    if (itr != _followers.end())
        return &itr->second;

    return nullptr;
}

void Garrison::BuildInfoPacket(WorldPackets::Garrison::GarrisonInfo& garrison) const
{
    garrison.GarrTypeID = GetType();
    garrison.GarrSiteID = _siteLevel->GarrSiteID;
    garrison.GarrSiteLevelID = _siteLevel->ID;
    garrison.NumFollowerActivationsRemaining = _followerActivationsRemainingToday;
    for (auto& p : _plots)
    {
        Plot const& plot = p.second;
        garrison.Plots.push_back(&plot.PacketInfo);
        if (plot.BuildingInfo.PacketInfo)
            garrison.Buildings.push_back(&*plot.BuildingInfo.PacketInfo);
    }

    for (auto const& p : _followers)
        garrison.Followers.push_back(&p.second.PacketInfo);

    for (auto const& p : _missions)
    {
        garrison.Missions.push_back(&p.second.PacketInfo);
        garrison.MissionRewards.push_back(p.second.PacketInfo.Rewards);
        garrison.MissionOvermaxRewards.push_back(p.second.PacketInfo.OvermaxRewards);
        garrison.CanStartMission.push_back(p.second.PacketInfo.MissionState == 0);
    }
}

void Garrison::SendRemoteInfo() const
{
    MapEntry const* garrisonMap = sMapStore.LookupEntry(_siteLevel->MapID);
    if (!garrisonMap || int32(_owner->GetMapId()) != garrisonMap->ParentMapID)
        return;

    WorldPackets::Garrison::GarrisonRemoteInfo remoteInfo;
    remoteInfo.Sites.resize(1);

    WorldPackets::Garrison::GarrisonRemoteSiteInfo& remoteSiteInfo = remoteInfo.Sites[0];
    remoteSiteInfo.GarrSiteLevelID = _siteLevel->ID;
    for (auto const& p : _plots)
        if (p.second.BuildingInfo.PacketInfo)
            remoteSiteInfo.Buildings.emplace_back(p.first, p.second.BuildingInfo.PacketInfo->GarrBuildingID);

    _owner->SendDirectMessage(remoteInfo.Write());
}

void Garrison::SendBlueprintAndSpecializationData()
{
    WorldPackets::Garrison::GarrisonRequestBlueprintAndSpecializationDataResult data;
    data.GarrTypeID = GetType();
    data.BlueprintsKnown = &_knownBuildings;
    _owner->SendDirectMessage(data.Write());
}

void Garrison::SendMapData(Player* receiver) const
{
    WorldPackets::Garrison::GarrisonMapDataResponse mapData;
    mapData.Buildings.reserve(_plots.size());

    for (auto const& p : _plots)
    {
        Plot const& plot = p.second;
        if (plot.BuildingInfo.PacketInfo)
            if (uint32 garrBuildingPlotInstId = sGarrisonMgr.GetGarrBuildingPlotInst(plot.BuildingInfo.PacketInfo->GarrBuildingID, plot.GarrSiteLevelPlotInstId))
                mapData.Buildings.emplace_back(garrBuildingPlotInstId, plot.PacketInfo.PlotPos.Pos);
    }

    receiver->SendDirectMessage(mapData.Write());
}

// ============================================================
// Follower management
// ============================================================

Garrison::Follower* Garrison::GetFollower(uint64 dbId)
{
    auto itr = _followers.find(dbId);
    if (itr != _followers.end())
        return &itr->second;

    return nullptr;
}

void Garrison::RemoveFollower(uint64 dbId)
{
    WorldPackets::Garrison::GarrisonRemoveFollowerResult removeFollowerResult;
    removeFollowerResult.GarrTypeID = GetType();
    removeFollowerResult.Result = GARRISON_SUCCESS;

    Follower const* follower = GetFollower(dbId);
    if (!follower)
    {
        removeFollowerResult.Result = GARRISON_ERROR_INVALID_FOLLOWER;
        _owner->SendDirectMessage(removeFollowerResult.Write());
        return;
    }

    if (follower->PacketInfo.CurrentMissionID != 0)
    {
        removeFollowerResult.Result = GARRISON_ERROR_FOLLOWER_ALREADY_ON_MISSION;
        _owner->SendDirectMessage(removeFollowerResult.Write());
        return;
    }

    removeFollowerResult.FollowerDBID = dbId;
    removeFollowerResult.Destroyed = 1;
    _followerIds.erase(follower->PacketInfo.GarrFollowerID);
    _followers.erase(dbId);
    _owner->SendDirectMessage(removeFollowerResult.Write());
}

void Garrison::SetFollowerFavorite(uint64 dbId, bool favorite)
{
    WorldPackets::Garrison::GarrisonFollowerChangedFlags result;
    result.Result = GARRISON_SUCCESS;

    Follower* follower = GetFollower(dbId);
    if (!follower)
    {
        result.Result = GARRISON_ERROR_INVALID_FOLLOWER;
        _owner->SendDirectMessage(result.Write());
        return;
    }

    result.FollowerDBID = dbId;
    if (favorite)
        follower->PacketInfo.FollowerStatus |= FOLLOWER_STATUS_FAVORITE;
    else
        follower->PacketInfo.FollowerStatus &= ~FOLLOWER_STATUS_FAVORITE;

    result.Flags = follower->PacketInfo.FollowerStatus;
    _owner->SendDirectMessage(result.Write());
}

void Garrison::SetFollowerInactive(uint64 dbId, bool inactive)
{
    WorldPackets::Garrison::GarrisonFollowerChangedFlags result;
    result.Result = GARRISON_SUCCESS;

    Follower* follower = GetFollower(dbId);
    if (!follower)
    {
        result.Result = GARRISON_ERROR_INVALID_FOLLOWER;
        _owner->SendDirectMessage(result.Write());
        return;
    }

    if (follower->PacketInfo.CurrentMissionID != 0)
    {
        result.Result = GARRISON_ERROR_FOLLOWER_ALREADY_ON_MISSION;
        _owner->SendDirectMessage(result.Write());
        return;
    }

    result.FollowerDBID = dbId;
    if (inactive)
        follower->PacketInfo.FollowerStatus |= FOLLOWER_STATUS_INACTIVE;
    else
        follower->PacketInfo.FollowerStatus &= ~FOLLOWER_STATUS_INACTIVE;

    result.Flags = follower->PacketInfo.FollowerStatus;
    _owner->SendDirectMessage(result.Write());
}

void Garrison::RenameFollower(uint64 dbId, std::string const& name)
{
    WorldPackets::Garrison::GarrisonRenameFollowerResult result;
    result.Result = GARRISON_SUCCESS;

    Follower* follower = GetFollower(dbId);
    if (!follower)
    {
        result.Result = GARRISON_ERROR_INVALID_FOLLOWER;
        _owner->SendDirectMessage(result.Write());
        return;
    }

    result.FollowerDBID = dbId;
    follower->PacketInfo.CustomName = name;
    result.FollowerName = name;
    _owner->SendDirectMessage(result.Write());
}

void Garrison::AssignFollowerToBuilding(uint64 dbId, uint32 plotInstanceId)
{
    WorldPackets::Garrison::GarrisonAssignFollowerToBuildingResult result;
    result.Result = GARRISON_SUCCESS;

    Follower* follower = GetFollower(dbId);
    if (!follower)
    {
        result.Result = GARRISON_ERROR_INVALID_FOLLOWER;
        _owner->SendDirectMessage(result.Write());
        return;
    }

    Plot* plot = GetPlot(plotInstanceId);
    if (!plot || !plot->BuildingInfo.PacketInfo || !plot->BuildingInfo.PacketInfo->Active)
    {
        result.Result = GARRISON_ERROR_INVALID_PLOT_INSTANCEID;
        _owner->SendDirectMessage(result.Write());
        return;
    }

    if (follower->PacketInfo.CurrentMissionID != 0)
    {
        result.Result = GARRISON_ERROR_FOLLOWER_ALREADY_ON_MISSION;
        _owner->SendDirectMessage(result.Write());
        return;
    }

    result.FollowerDBID = dbId;
    result.PlotInstanceID = plotInstanceId;
    follower->PacketInfo.CurrentBuildingID = plot->BuildingInfo.PacketInfo->GarrBuildingID;
    _owner->SendDirectMessage(result.Write());
}

void Garrison::RemoveFollowerFromBuilding(uint64 dbId)
{
    WorldPackets::Garrison::GarrisonRemoveFollowerFromBuildingResult result;
    result.Result = GARRISON_SUCCESS;

    Follower* follower = GetFollower(dbId);
    if (!follower)
    {
        result.Result = GARRISON_ERROR_INVALID_FOLLOWER;
        _owner->SendDirectMessage(result.Write());
        return;
    }

    result.FollowerDBID = dbId;
    follower->PacketInfo.CurrentBuildingID = 0;
    _owner->SendDirectMessage(result.Write());
}

// ============================================================
// Mission management
// ============================================================

void Garrison::PopulateMissionData(Mission& mission, GarrMissionEntry const* missionEntry) const
{
    // Populate encounters from DB2
    if (std::vector<GarrMissionXEncounterEntry const*> const* missionEncounters = sGarrisonMgr.GetMissionEncounters(missionEntry->ID))
    {
        for (GarrMissionXEncounterEntry const* missionEncounter : *missionEncounters)
        {
            GarrEncounterEntry const* encounterEntry = sGarrEncounterStore.LookupEntry(missionEncounter->GarrEncounterID);
            if (!encounterEntry)
                continue;

            WorldPackets::Garrison::GarrisonEncounter encounter;
            encounter.GarrEncounterID = encounterEntry->ID;

            // Populate mechanics for this encounter
            if (std::vector<GarrMechanicEntry const*> const* mechanics = sGarrisonMgr.GetEncounterMechanics(encounterEntry->ID))
            {
                for (GarrMechanicEntry const* mechanic : *mechanics)
                    encounter.Mechanics.push_back(mechanic->GarrMechanicTypeID);
            }

            // Also add the encounter's environment mechanic type if it has one
            if (encounterEntry->EnvGarrMechanicTypeID != 0)
                encounter.Mechanics.push_back(encounterEntry->EnvGarrMechanicTypeID);

            mission.PacketInfo.Encounters.push_back(std::move(encounter));
        }
    }

    // Populate rewards from OvermaxRewardPackID (used for both base and bonus rewards)
    // WoD garrison missions use the RewardPack system
    if (missionEntry->OvermaxRewardPackID != 0)
    {
        // Items from RewardPackXItem
        if (std::vector<RewardPackXItemEntry const*> const* items = sDB2Manager.GetRewardPackItemsByRewardID(missionEntry->OvermaxRewardPackID))
        {
            for (RewardPackXItemEntry const* item : *items)
            {
                WorldPackets::Garrison::GarrisonMissionReward reward;
                reward.ItemID = item->ItemID;
                reward.ItemQuantity = item->ItemQuantity;
                mission.PacketInfo.OvermaxRewards.push_back(std::move(reward));
            }
        }

        // Currency from RewardPackXCurrencyType
        if (std::vector<RewardPackXCurrencyTypeEntry const*> const* currencies = sDB2Manager.GetRewardPackCurrencyTypesByRewardID(missionEntry->OvermaxRewardPackID))
        {
            for (RewardPackXCurrencyTypeEntry const* currency : *currencies)
            {
                WorldPackets::Garrison::GarrisonMissionReward reward;
                reward.CurrencyID = currency->CurrencyTypeID;
                reward.CurrencyQuantity = currency->Quantity;
                mission.PacketInfo.OvermaxRewards.push_back(std::move(reward));
            }
        }

        // Gold from RewardPack
        if (RewardPackEntry const* pack = sRewardPackStore.LookupEntry(missionEntry->OvermaxRewardPackID))
        {
            if (pack->Money > 0)
            {
                WorldPackets::Garrison::GarrisonMissionReward reward;
                reward.CurrencyID = 0; // Gold
                reward.CurrencyQuantity = pack->Money;
                mission.PacketInfo.OvermaxRewards.push_back(std::move(reward));
            }
        }
    }

    // Base rewards: follower XP is always included, plus any from GarrMissionSetID-linked packs
    // For WoD garrisons, the primary reward is follower XP + whatever the mission offers
    if (missionEntry->BaseFollowerXP > 0)
    {
        WorldPackets::Garrison::GarrisonMissionReward reward;
        reward.FollowerXP = missionEntry->BaseFollowerXP;
        mission.PacketInfo.Rewards.push_back(std::move(reward));
    }

    // Add currency/gold from mission cost currency as a reward if mission uses a reward pack
    // (many missions grant garrison resources as primary reward)
    if (missionEntry->MissionCostCurrencyTypesID != 0 && missionEntry->MissionCost > 0)
    {
        // The reward is typically more than the cost, but varies per mission
        // For now, base rewards are whatever the OvermaxRewards specify (this is correct for WoD)
    }
}

void Garrison::AddMission(uint32 garrMissionId)
{
    GarrMissionEntry const* missionEntry = sGarrMissionStore.LookupEntry(garrMissionId);
    if (!missionEntry)
        return;

    // Don't add duplicate missions
    if (_activeMissionRecIDs.count(garrMissionId))
        return;

    uint64 dbId = GenerateMissionDbId();
    Mission& mission = _missions[dbId];
    mission.PacketInfo.DbID = dbId;
    mission.PacketInfo.MissionRecID = garrMissionId;
    mission.PacketInfo.OfferTime = GameTime::GetGameTime();
    mission.PacketInfo.OfferDuration = Seconds(missionEntry->OfferDuration);
    mission.PacketInfo.StartTime = time_t(2288912640);
    mission.PacketInfo.TravelDuration = Seconds(missionEntry->TravelDuration);
    mission.PacketInfo.MissionDuration = Seconds(missionEntry->MissionDuration);
    mission.PacketInfo.MissionState = 0; // Offered
    mission.PacketInfo.SuccessChance = 0;
    mission.PacketInfo.Flags = missionEntry->Flags;
    mission.PacketInfo.MissionScalar = missionEntry->AutoMissionScalar;

    // Populate encounters and rewards from DB2 data
    PopulateMissionData(mission, missionEntry);

    _activeMissionRecIDs.insert(garrMissionId);

    WorldPackets::Garrison::GarrisonAddMissionResult addMissionResult;
    addMissionResult.GarrTypeID = missionEntry->GarrTypeID;
    addMissionResult.Result = GARRISON_SUCCESS;
    addMissionResult.State = 0;
    addMissionResult.Mission = mission.PacketInfo;
    addMissionResult.CanStartMission = true;
    _owner->SendDirectMessage(addMissionResult.Write());
}

Garrison::Mission const* Garrison::GetMission(uint64 dbId) const
{
    auto itr = _missions.find(dbId);
    if (itr != _missions.end())
        return &itr->second;

    return nullptr;
}

Garrison::Mission* Garrison::GetMission(uint64 dbId)
{
    auto itr = _missions.find(dbId);
    if (itr != _missions.end())
        return &itr->second;

    return nullptr;
}

Garrison::Mission const* Garrison::GetMissionByRecID(uint32 missionRecID) const
{
    for (auto const& p : _missions)
        if (static_cast<uint32>(p.second.PacketInfo.MissionRecID) == missionRecID)
            return &p.second;

    return nullptr;
}

Garrison::Mission* Garrison::GetMissionByRecID(uint32 missionRecID)
{
    for (auto& p : _missions)
        if (static_cast<uint32>(p.second.PacketInfo.MissionRecID) == missionRecID)
            return &p.second;

    return nullptr;
}

int32 Garrison::CalculateSuccessChance(uint32 missionRecID, std::vector<uint64> const& followerDBIDs) const
{
    GarrMissionEntry const* missionEntry = sGarrMissionStore.LookupEntry(missionRecID);
    if (!missionEntry)
        return 0;

    int32 successChance = missionEntry->BaseCompletionChance;

    // Collect all follower counter abilities
    std::unordered_set<uint8> counteredMechanicCategories;
    int32 totalFollowerLevel = 0;
    int32 totalFollowerItemLevel = 0;
    uint32 followerCount = 0;

    for (uint64 followerDbId : followerDBIDs)
    {
        Follower const* follower = GetFollower(followerDbId);
        if (!follower)
            continue;

        ++followerCount;
        totalFollowerLevel += follower->PacketInfo.FollowerLevel;
        totalFollowerItemLevel += follower->GetItemLevel();

        // Check each follower ability against mission mechanics
        for (GarrAbilityEntry const* ability : follower->PacketInfo.AbilityID)
        {
            if (!ability || (ability->Flags & GARRISON_ABILITY_FLAG_TRAIT))
                continue; // Skip traits, only counter abilities matter

            // Check against each encounter's mechanics
            Mission const* missionData = GetMissionByRecID(missionRecID);
            if (missionData)
            {
                for (auto const& encounter : missionData->PacketInfo.Encounters)
                {
                    for (int32 mechanicTypeID : encounter.Mechanics)
                    {
                        GarrMechanicTypeEntry const* mechanicType = sGarrisonMgr.GetMechanicType(mechanicTypeID);
                        if (mechanicType && sGarrisonMgr.DoesAbilityCounterMechanic(ability, mechanicType))
                            counteredMechanicCategories.insert(mechanicType->GarrAbilityCategoryID);
                    }
                }
            }
        }
    }

    if (followerCount == 0)
        return 0;

    // Count total mechanics across all encounters
    uint32 totalMechanics = 0;
    Mission const* mission = GetMissionByRecID(missionRecID);
    if (mission)
    {
        for (auto const& encounter : mission->PacketInfo.Encounters)
            totalMechanics += static_cast<uint32>(encounter.Mechanics.size());
    }

    // Each countered mechanic adds a bonus proportional to mission complexity
    // For a typical mission with 3 mechanics, each counter is worth ~10-15%
    if (totalMechanics > 0)
    {
        uint32 countered = static_cast<uint32>(counteredMechanicCategories.size());
        float counterBonus = (static_cast<float>(countered) / static_cast<float>(totalMechanics)) * 45.0f;
        successChance += static_cast<int32>(counterBonus);
    }

    // Level difference penalty (only for under-leveled followers)
    int32 avgFollowerLevel = totalFollowerLevel / static_cast<int32>(followerCount);
    int32 levelDiff = avgFollowerLevel - static_cast<int32>(missionEntry->TargetLevel);
    if (levelDiff < 0)
        successChance += levelDiff * 3; // -3% per level below target

    // Item level bonus for missions with item level requirements
    if (missionEntry->TargetItemLevel > 0 && followerCount > 0)
    {
        int32 avgFollowerItemLevel = totalFollowerItemLevel / static_cast<int32>(followerCount);
        int32 iLvlDiff = avgFollowerItemLevel - static_cast<int32>(missionEntry->TargetItemLevel);
        if (iLvlDiff > 0)
            successChance += std::min(iLvlDiff / 10, 10); // +1% per 10 iLvl above target, capped at +10%
    }

    return std::clamp(successChance, 0, 100);
}

GarrisonError Garrison::StartMission(uint32 missionRecID, std::vector<uint64> const& followerDBIDs)
{
    GarrMissionEntry const* missionEntry = sGarrMissionStore.LookupEntry(missionRecID);
    if (!missionEntry)
        return GARRISON_ERROR_INVALID_MISSION;

    Mission* mission = GetMissionByRecID(missionRecID);
    if (!mission)
        return GARRISON_ERROR_INVALID_MISSION;

    if (mission->PacketInfo.MissionState != 0)
        return GARRISON_ERROR_ALREADY_ON_MISSION;

    if (followerDBIDs.size() > missionEntry->MaxFollowers)
        return GARRISON_ERROR_MISSION_SIZE_INVALID;

    if (followerDBIDs.empty())
        return GARRISON_ERROR_MISSION_SIZE_INVALID;

    // Validate all followers
    for (uint64 followerDbId : followerDBIDs)
    {
        Follower const* follower = GetFollower(followerDbId);
        if (!follower)
            return GARRISON_ERROR_INVALID_FOLLOWER;

        if (follower->PacketInfo.CurrentMissionID != 0)
            return GARRISON_ERROR_FOLLOWER_ALREADY_ON_MISSION;

        if (follower->PacketInfo.FollowerStatus & FOLLOWER_STATUS_INACTIVE)
            return GARRISON_ERROR_FOLLOWER_INACTIVE;
    }

    // Deduct mission cost
    if (missionEntry->MissionCost > 0)
    {
        if (missionEntry->MissionCostCurrencyTypesID != 0)
        {
            if (!_owner->HasCurrency(missionEntry->MissionCostCurrencyTypesID, missionEntry->MissionCost))
                return GARRISON_ERROR_NOT_ENOUGH_CURRENCY;
            _owner->RemoveCurrency(missionEntry->MissionCostCurrencyTypesID, missionEntry->MissionCost, CurrencyDestroyReason::Garrison);
        }
        else
        {
            if (!_owner->HasEnoughMoney(uint64(missionEntry->MissionCost) * GOLD))
                return GARRISON_ERROR_NOT_ENOUGH_GOLD;
            _owner->ModifyMoney(-int64(missionEntry->MissionCost) * GOLD, false);
        }
    }

    // Assign followers to mission
    mission->CurrentFollowerDBIDs = followerDBIDs;
    for (uint64 followerDbId : followerDBIDs)
    {
        if (Follower* follower = GetFollower(followerDbId))
            follower->PacketInfo.CurrentMissionID = missionRecID;
    }

    // Calculate success chance using encounter-based mechanic system
    int32 successChance = CalculateSuccessChance(missionRecID, followerDBIDs);

    mission->PacketInfo.MissionState = 1; // In Progress
    mission->PacketInfo.StartTime = GameTime::GetGameTime();
    mission->PacketInfo.SuccessChance = successChance;

    return GARRISON_SUCCESS;
}

GarrisonError Garrison::CompleteMission(uint32 missionRecID)
{
    Mission* mission = GetMissionByRecID(missionRecID);
    if (!mission)
        return GARRISON_ERROR_INVALID_MISSION;

    if (mission->PacketInfo.MissionState != 1) // Not in progress
        return GARRISON_ERROR_NOT_ON_MISSION;

    // Check if mission duration has elapsed
    time_t now = GameTime::GetGameTime();
    time_t missionEnd = time_t(mission->PacketInfo.StartTime) +
        Seconds(mission->PacketInfo.TravelDuration).count() +
        Seconds(mission->PacketInfo.MissionDuration).count();

    if (now < missionEnd)
        return GARRISON_ERROR_MISSION_NOT_COMPLETE;

    mission->PacketInfo.MissionState = 2; // Completed
    return GARRISON_SUCCESS;
}

GarrisonError Garrison::ClaimMissionReward(uint32 missionRecID)
{
    Mission* mission = GetMissionByRecID(missionRecID);
    if (!mission)
        return GARRISON_ERROR_INVALID_MISSION;

    if (mission->PacketInfo.MissionState != 2 && mission->PacketInfo.MissionState != 3)
        return GARRISON_ERROR_MISSION_NOT_COMPLETE;

    GarrMissionEntry const* missionEntry = sGarrMissionStore.LookupEntry(missionRecID);
    if (!missionEntry)
        return GARRISON_ERROR_INVALID_MISSION;

    // Roll success based on calculated success chance
    bool succeeded = static_cast<int32>(urand(0, 99)) < mission->PacketInfo.SuccessChance;

    // Award follower XP (awarded regardless of success)
    uint32 followerXP = missionEntry->BaseFollowerXP;
    for (uint64 followerDbId : mission->CurrentFollowerDBIDs)
    {
        if (Follower* follower = GetFollower(followerDbId))
        {
            follower->PacketInfo.CurrentMissionID = 0;

            if (followerXP > 0 && !(follower->PacketInfo.FollowerStatus & FOLLOWER_STATUS_NO_XP_GAIN))
            {
                follower->PacketInfo.Xp += followerXP;

                // Level up check: XP scales with level (level * 200 + 400)
                uint32 xpToLevel = follower->PacketInfo.FollowerLevel * 200 + 400;
                while (follower->PacketInfo.Xp >= xpToLevel && follower->PacketInfo.FollowerLevel < 100)
                {
                    follower->PacketInfo.Xp -= xpToLevel;
                    follower->PacketInfo.FollowerLevel++;
                    xpToLevel = follower->PacketInfo.FollowerLevel * 200 + 400;
                }

                // Send follower XP update
                WorldPackets::Garrison::GarrisonFollowerChangedXP followerXPUpdate;
                followerXPUpdate.Result = GARRISON_SUCCESS;
                followerXPUpdate.TotalXp = followerXP;
                followerXPUpdate.Follower = follower->PacketInfo;
                _owner->SendDirectMessage(followerXPUpdate.Write());
            }
        }
    }

    // Award mission rewards if succeeded
    if (succeeded)
    {
        // Award base rewards
        for (auto const& reward : mission->PacketInfo.Rewards)
        {
            if (reward.ItemID > 0 && reward.ItemQuantity > 0)
            {
                ItemPosCountVec dest;
                if (_owner->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, reward.ItemID, reward.ItemQuantity) == EQUIP_ERR_OK)
                {
                    if (Item* item = _owner->StoreNewItem(dest, reward.ItemID, true))
                        _owner->SendNewItem(item, reward.ItemQuantity, true, false);
                }
                else
                {
                    // Mail overflow items
                    MailDraft draft("Garrison Mission Reward", "A reward from a completed garrison mission.");
                    if (Item* item = Item::CreateItem(reward.ItemID, reward.ItemQuantity, ItemContext::NONE, _owner))
                    {
                        item->SaveToDB(CharacterDatabaseTransaction(nullptr));
                        draft.AddItem(item);
                        draft.SendMailTo(CharacterDatabaseTransaction(nullptr), MailReceiver(_owner), MailSender(MAIL_CREATURE, 0));
                    }
                }
            }
            if (reward.CurrencyID > 0 && reward.CurrencyQuantity > 0)
                _owner->AddCurrency(reward.CurrencyID, reward.CurrencyQuantity, CurrencyGainSource::GarrisonMissionReward);
        }

        // Check if bonus roll was done (state 3) and award overmax rewards
        if (mission->PacketInfo.MissionState == 3)
        {
            for (auto const& reward : mission->PacketInfo.OvermaxRewards)
            {
                if (reward.ItemID > 0 && reward.ItemQuantity > 0)
                {
                    ItemPosCountVec dest;
                    if (_owner->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, reward.ItemID, reward.ItemQuantity) == EQUIP_ERR_OK)
                    {
                        if (Item* item = _owner->StoreNewItem(dest, reward.ItemID, true))
                            _owner->SendNewItem(item, reward.ItemQuantity, true, false);
                    }
                    else
                    {
                        MailDraft draft("Garrison Mission Bonus", "A bonus reward from a garrison mission.");
                        if (Item* item = Item::CreateItem(reward.ItemID, reward.ItemQuantity, ItemContext::NONE, _owner))
                        {
                            item->SaveToDB(CharacterDatabaseTransaction(nullptr));
                            draft.AddItem(item);
                            draft.SendMailTo(CharacterDatabaseTransaction(nullptr), MailReceiver(_owner), MailSender(MAIL_CREATURE, 0));
                        }
                    }
                }
                if (reward.CurrencyID > 0 && reward.CurrencyQuantity > 0)
                    _owner->AddCurrency(reward.CurrencyID, reward.CurrencyQuantity, CurrencyGainSource::GarrisonMissionReward);
            }
        }
    }

    // Remove mission from active list
    _activeMissionRecIDs.erase(missionRecID);
    for (auto itr = _missions.begin(); itr != _missions.end(); ++itr)
    {
        if (static_cast<uint32>(itr->second.PacketInfo.MissionRecID) == missionRecID)
        {
            _missions.erase(itr);
            break;
        }
    }

    return GARRISON_SUCCESS;
}

GarrisonError Garrison::MissionBonusRoll(uint32 missionRecID)
{
    Mission* mission = GetMissionByRecID(missionRecID);
    if (!mission)
        return GARRISON_ERROR_INVALID_MISSION;

    if (mission->PacketInfo.MissionState != 2)
        return GARRISON_ERROR_MISSION_NOT_COMPLETE;

    // The bonus roll uses the same success chance as the mission
    // If the roll succeeds, the overmax rewards will be given when claiming
    bool bonusSucceeded = static_cast<int32>(urand(0, 99)) < mission->PacketInfo.SuccessChance;

    mission->PacketInfo.MissionState = bonusSucceeded ? 3 : 2;
    // State 3 = bonus rolled successfully (overmax rewards will be awarded)
    // Keep state 2 if bonus failed (only base rewards will be awarded on claim)

    return GARRISON_SUCCESS;
}

void Garrison::RemoveMission(uint32 missionRecID)
{
    for (auto itr = _missions.begin(); itr != _missions.end(); ++itr)
    {
        if (static_cast<uint32>(itr->second.PacketInfo.MissionRecID) == missionRecID)
        {
            // Unassign followers
            for (uint64 followerDbId : itr->second.CurrentFollowerDBIDs)
                if (Follower* follower = GetFollower(followerDbId))
                    follower->PacketInfo.CurrentMissionID = 0;

            _activeMissionRecIDs.erase(missionRecID);
            _missions.erase(itr);
            break;
        }
    }
}

void Garrison::RemoveExpiredMissions()
{
    time_t now = GameTime::GetGameTime();
    std::vector<uint32> expiredMissions;

    for (auto const& p : _missions)
    {
        // Only check offered missions (not in-progress or completed)
        if (p.second.PacketInfo.MissionState != 0)
            continue;

        // Check if offer has expired
        if (Seconds(p.second.PacketInfo.OfferDuration).count() > 0)
        {
            time_t offerEnd = time_t(p.second.PacketInfo.OfferTime) + Seconds(p.second.PacketInfo.OfferDuration).count();
            if (now >= offerEnd)
                expiredMissions.push_back(p.second.PacketInfo.MissionRecID);
        }
    }

    for (uint32 missionRecID : expiredMissions)
        RemoveMission(missionRecID);
}

void Garrison::GenerateAvailableMissions()
{
    if (!_siteLevel)
        return;

    // Determine garrison type from site level
    // GarrSiteLevelEntry links to GarrSiteID which maps to a garrison type
    int8 garrTypeID = GARRISON_TYPE_GARRISON; // WoD garrisons

    std::vector<GarrMissionEntry const*> const* availableMissions = sGarrisonMgr.GetMissionsByGarrType(garrTypeID);
    if (!availableMissions || availableMissions->empty())
        return;

    // Remove expired offers first
    RemoveExpiredMissions();

    // Count current offered missions (not in-progress or completed)
    uint32 currentOffered = 0;
    for (auto const& p : _missions)
        if (p.second.PacketInfo.MissionState == 0)
            ++currentOffered;

    // Target: up to 15 available missions at a time
    static constexpr uint32 MAX_AVAILABLE_MISSIONS = 15;
    if (currentOffered >= MAX_AVAILABLE_MISSIONS)
    {
        _lastMissionGenerationTime = GameTime::GetGameTime();
        return;
    }

    uint32 missionsToGenerate = MAX_AVAILABLE_MISSIONS - currentOffered;

    // Get average follower level for filtering
    int32 avgFollowerLevel = 0;
    uint32 followerCount = 0;
    for (auto const& p : _followers)
    {
        if (!(p.second.PacketInfo.FollowerStatus & FOLLOWER_STATUS_INACTIVE))
        {
            avgFollowerLevel += p.second.PacketInfo.FollowerLevel;
            ++followerCount;
        }
    }

    if (followerCount > 0)
        avgFollowerLevel /= static_cast<int32>(followerCount);
    else
        avgFollowerLevel = 90; // Default for no followers

    // Build eligible mission pool
    std::vector<GarrMissionEntry const*> eligibleMissions;
    for (GarrMissionEntry const* mission : *availableMissions)
    {
        // Skip missions already active
        if (_activeMissionRecIDs.count(mission->ID))
            continue;

        // Filter by follower type (WoD garrison = FOLLOWER_TYPE_GARRISON)
        if (mission->GarrFollowerTypeID != FOLLOWER_TYPE_GARRISON)
            continue;

        // Filter by target level (within +-5 levels of average follower level)
        int32 levelDiff = std::abs(avgFollowerLevel - static_cast<int32>(mission->TargetLevel));
        if (levelDiff > 5)
            continue;

        // Skip missions requiring more followers than available
        uint32 availableFollowers = 0;
        for (auto const& p : _followers)
        {
            if (p.second.PacketInfo.CurrentMissionID == 0
                && !(p.second.PacketInfo.FollowerStatus & FOLLOWER_STATUS_INACTIVE))
                ++availableFollowers;
        }

        if (mission->MaxFollowers > availableFollowers)
            continue;

        // Skip missions with 0 duration (usually internal/debug)
        if (mission->MissionDuration == 0)
            continue;

        eligibleMissions.push_back(mission);
    }

    // Random selection from eligible pool
    if (eligibleMissions.size() > missionsToGenerate)
    {
        Trinity::Containers::RandomResize(eligibleMissions, missionsToGenerate);
    }

    for (GarrMissionEntry const* missionEntry : eligibleMissions)
        AddMission(missionEntry->ID);

    _lastMissionGenerationTime = GameTime::GetGameTime();
}

uint64 Garrison::GenerateMissionDbId()
{
    return _missionDbIdGenerator++;
}

// ============================================================
// Recruitment
// ============================================================

void Garrison::GenerateRecruits(uint32 faction)
{
    _availableRecruits.clear();

    // Find all followers of the garrison type that the player doesn't already have
    std::vector<GarrFollowerEntry const*> eligibleFollowers;
    for (GarrFollowerEntry const* follower : sGarrFollowerStore)
    {
        if (follower->GarrFollowerTypeID != FOLLOWER_TYPE_GARRISON)
            continue;

        // Skip followers the player already has
        if (_followerIds.count(follower->ID))
            continue;

        // Skip unique followers that are faction-specific
        if (follower->Flags & GARRISON_FOLLOWER_FLAG_UNIQUE)
            continue;

        eligibleFollowers.push_back(follower);
    }

    if (eligibleFollowers.empty())
        return;

    // Pick up to 3 random followers
    Trinity::Containers::RandomResize(eligibleFollowers, std::min<size_t>(3, eligibleFollowers.size()));

    for (GarrFollowerEntry const* followerEntry : eligibleFollowers)
    {
        WorldPackets::Garrison::GarrisonFollower recruit;
        recruit.DbID = 0; // Not yet in DB
        recruit.GarrFollowerID = followerEntry->ID;
        recruit.Quality = urand(1, 3); // Uncommon to Rare
        recruit.FollowerLevel = std::max<int32>(90, _owner->GetLevel() - 5);
        recruit.ItemLevelWeapon = 600;
        recruit.ItemLevelArmor = 600;
        recruit.CurrentMissionID = 0;
        recruit.CurrentBuildingID = 0;
        recruit.FollowerStatus = 0;
        recruit.Xp = 0;
        recruit.Durability = 5;
        // Durability represents remaining health points for the follower (max 5 for WoD)

        // Roll abilities based on quality
        std::list<GarrAbilityEntry const*> abilities = sGarrisonMgr.RollFollowerAbilities(
            followerEntry->ID, followerEntry, recruit.Quality, faction, true);

        for (GarrAbilityEntry const* ability : abilities)
            recruit.AbilityID.push_back(ability);

        _availableRecruits.push_back(std::move(recruit));
    }
}

GarrisonError Garrison::RecruitFollower(uint32 garrFollowerID)
{
    // Find the recruit in available recruits
    auto itr = std::find_if(_availableRecruits.begin(), _availableRecruits.end(),
        [garrFollowerID](WorldPackets::Garrison::GarrisonFollower const& f) {
            return f.GarrFollowerID == garrFollowerID;
        });

    if (itr == _availableRecruits.end())
        return GARRISON_ERROR_INVALID_AVAILABLE_RECRUIT;

    // Check if already recruited
    if (_followerIds.count(garrFollowerID))
        return GARRISON_ERROR_FOLLOWER_ALREADY_RECRUITED;

    // Add the follower (this uses the normal AddFollower which handles DB, packet, etc.)
    AddFollower(garrFollowerID);

    // Clear recruits after one is chosen
    _availableRecruits.clear();

    return GARRISON_SUCCESS;
}

void Garrison::HealAllFollowers()
{
    for (auto& p : _followers)
    {
        p.second.PacketInfo.Health = static_cast<int32>(p.second.PacketInfo.Durability);
        p.second.PacketInfo.FollowerStatus &= ~FOLLOWER_STATUS_EXHAUSTED;
    }
}

Map* Garrison::FindMap() const
{
    return sMapMgr->FindMap(_siteLevel->MapID, _owner->GetGUID().GetCounter());
}

GarrisonError Garrison::CheckBuildingPlacement(uint32 garrPlotInstanceId, uint32 garrBuildingId) const
{
    GarrPlotInstanceEntry const* plotInstance = sGarrPlotInstanceStore.LookupEntry(garrPlotInstanceId);
    Plot const* plot = GetPlot(garrPlotInstanceId);
    if (!plotInstance || !plot)
        return GARRISON_ERROR_INVALID_PLOT_INSTANCEID;

    GarrBuildingEntry const* building = sGarrBuildingStore.LookupEntry(garrBuildingId);
    if (!building)
        return GARRISON_ERROR_INVALID_BUILDINGID;

    if (!sGarrisonMgr.IsPlotMatchingBuilding(plotInstance->GarrPlotID, garrBuildingId))
        return GARRISON_ERROR_INVALID_PLOT_BUILDING;

    // Cannot place buldings of higher level than garrison level
    if (building->UpgradeLevel > _siteLevel->MaxBuildingLevel)
        return GARRISON_ERROR_INVALID_BUILDINGID;

    if (building->Flags & GARRISON_BUILDING_FLAG_NEEDS_PLAN)
    {
        if (!HasBlueprint(garrBuildingId))
            return GARRISON_ERROR_REQUIRES_BLUEPRINT;
    }
    else // Building is built as a quest reward
        return GARRISON_ERROR_INVALID_BUILDINGID;

    // Check all plots to find if we already have this building
    GarrBuildingEntry const* existingBuilding;
    for (auto const& p : _plots)
    {
        if (p.second.BuildingInfo.PacketInfo)
        {
            existingBuilding = sGarrBuildingStore.AssertEntry(p.second.BuildingInfo.PacketInfo->GarrBuildingID);
            if (existingBuilding->BuildingType == building->BuildingType)
                if (p.first != garrPlotInstanceId || existingBuilding->UpgradeLevel + 1 != building->UpgradeLevel)    // check if its an upgrade in same plot
                    return GARRISON_ERROR_BUILDING_EXISTS;
        }
    }

    if (!_owner->HasCurrency(building->CurrencyTypeID, building->CurrencyQty))
        return GARRISON_ERROR_NOT_ENOUGH_CURRENCY;

    if (!_owner->HasEnoughMoney(uint64(building->GoldCost) * GOLD))
        return GARRISON_ERROR_NOT_ENOUGH_GOLD;

    // New building cannot replace another building currently under construction
    if (plot->BuildingInfo.PacketInfo)
        if (!plot->BuildingInfo.PacketInfo->Active)
            return GARRISON_ERROR_NO_BUILDING;

    return GARRISON_SUCCESS;
}

GarrisonError Garrison::CheckBuildingRemoval(uint32 garrPlotInstanceId) const
{
    Plot const* plot = GetPlot(garrPlotInstanceId);
    if (!plot)
        return GARRISON_ERROR_INVALID_PLOT_INSTANCEID;

    if (!plot->BuildingInfo.PacketInfo)
        return GARRISON_ERROR_NO_BUILDING;

    if (plot->BuildingInfo.CanActivate())
        return GARRISON_ERROR_BUILDING_EXISTS;

    return GARRISON_SUCCESS;
}

template<class T, void(T::*SecondaryRelocate)(Position const&)>
T* BuildingSpawnHelper(GameObject* building, ObjectGuid::LowType spawnId, Map* map)
{
    T* spawn = new T();
    if (!spawn->LoadFromDB(spawnId, map, false, false))
    {
        delete spawn;
        return nullptr;
    }

    Position globalPosition = building->GetPositionWithOffset(spawn->GetPosition());

    spawn->Relocate(globalPosition);
    (spawn->*SecondaryRelocate)(globalPosition);

    if (!spawn->IsPositionValid())
    {
        delete spawn;
        return nullptr;
    }

    if (!map->AddToMap(spawn))
    {
        delete spawn;
        return nullptr;
    }

    return spawn;
}

GameObject* Garrison::Plot::CreateGameObject(Map* map, GarrisonFactionIndex faction)
{
    uint32 entry = EmptyGameObjectId;
    if (BuildingInfo.PacketInfo)
    {
        GarrPlotInstanceEntry const* plotInstance = sGarrPlotInstanceStore.AssertEntry(PacketInfo.GarrPlotInstanceID);
        GarrPlotEntry const* plot = sGarrPlotStore.AssertEntry(plotInstance->GarrPlotID);
        GarrBuildingEntry const* building = sGarrBuildingStore.AssertEntry(BuildingInfo.PacketInfo->GarrBuildingID);
        entry = faction == GARRISON_FACTION_INDEX_HORDE ? plot->HordeConstructObjID : plot->AllianceConstructObjID;
        if (BuildingInfo.PacketInfo->Active || !entry)
            entry = faction == GARRISON_FACTION_INDEX_HORDE ? building->HordeGameObjectID : building->AllianceGameObjectID;
    }

    if (!sObjectMgr->GetGameObjectTemplate(entry))
    {
        TC_LOG_ERROR("garrison", "Garrison attempted to spawn gameobject whose template doesn't exist ({})", entry);
        return nullptr;
    }

    GameObject* building = GameObject::CreateGameObject(entry, map, PacketInfo.PlotPos.Pos, Rotation, 255, GO_STATE_READY);
    if (!building)
        return nullptr;

    if (BuildingInfo.CanActivate() && BuildingInfo.PacketInfo && !BuildingInfo.PacketInfo->Active)
    {
        if (FinalizeGarrisonPlotGOInfo const* finalizeInfo = sGarrisonMgr.GetPlotFinalizeGOInfo(PacketInfo.GarrPlotInstanceID))
        {
            Position const& pos2 = finalizeInfo->FactionInfo[faction].Pos;
            if (GameObject* finalizer = GameObject::CreateGameObject(finalizeInfo->FactionInfo[faction].GameObjectId, map, pos2, QuaternionData::fromEulerAnglesZYX(pos2.GetOrientation(), 0.0f, 0.0f), 255, GO_STATE_READY))
            {
                // set some spell id to make the object delete itself after use
                finalizer->SetSpellId(finalizer->GetGOInfo()->goober.spell);
                finalizer->SetRespawnTime(0);

                if (uint16 animKit = finalizeInfo->FactionInfo[faction].AnimKitId)
                    finalizer->SetAnimKitId(animKit, false);

                map->AddToMap(finalizer);
            }
        }
    }

    if (building->GetGoType() == GAMEOBJECT_TYPE_GARRISON_BUILDING && building->GetGOInfo()->garrisonBuilding.SpawnMap)
    {
        if (CellObjectGuidsMap const* cells = sObjectMgr->GetMapObjectGuids(building->GetGOInfo()->garrisonBuilding.SpawnMap, map->GetDifficultyID()))
        {
            for (auto const& [cellId, guids] : *cells)
            {
                for (ObjectGuid::LowType spawnId : guids.gameobjects)
                    if (GameObject* spawn = BuildingSpawnHelper<GameObject, &GameObject::RelocateStationaryPosition>(building, spawnId, map))
                        BuildingInfo.Spawns.insert(spawn->GetGUID());

                for (ObjectGuid::LowType spawnId : guids.creatures)
                    if (Creature* spawn = BuildingSpawnHelper<Creature, &Creature::SetHomePosition>(building, spawnId, map))
                        BuildingInfo.Spawns.insert(spawn->GetGUID());
            }
        }
    }

    BuildingInfo.Guid = building->GetGUID();
    return building;
}

void Garrison::Plot::DeleteGameObject(Map* map)
{
    if (BuildingInfo.Guid.IsEmpty())
        return;

    for (ObjectGuid const& guid : BuildingInfo.Spawns)
    {
        WorldObject* object = nullptr;
        switch (guid.GetHigh())
        {
            case HighGuid::Creature:
                object = map->GetCreature(guid);
                break;
            case HighGuid::GameObject:
                object = map->GetGameObject(guid);
                break;
            default:
                continue;
        }

        if (object)
            object->AddObjectToRemoveList();
    }

    BuildingInfo.Spawns.clear();

    if (GameObject* oldBuilding = map->GetGameObject(BuildingInfo.Guid))
        oldBuilding->Delete();

    BuildingInfo.Guid.Clear();
}

void Garrison::Plot::ClearBuildingInfo(GarrisonType garrisonType, Player* owner)
{
    WorldPackets::Garrison::GarrisonPlotPlaced plotPlaced;
    plotPlaced.GarrTypeID = garrisonType;
    plotPlaced.PlotInfo = &PacketInfo;
    owner->SendDirectMessage(plotPlaced.Write());

    BuildingInfo.PacketInfo.reset();
}

void Garrison::Plot::SetBuildingInfo(WorldPackets::Garrison::GarrisonBuildingInfo const& buildingInfo, Player* owner)
{
    if (!BuildingInfo.PacketInfo)
    {
        WorldPackets::Garrison::GarrisonPlotRemoved plotRemoved;
        plotRemoved.GarrPlotInstanceID = PacketInfo.GarrPlotInstanceID;
        owner->SendDirectMessage(plotRemoved.Write());
    }

    BuildingInfo.PacketInfo = buildingInfo;
}

bool Garrison::Building::CanActivate() const
{
    if (PacketInfo)
    {
        GarrBuildingEntry const* building = sGarrBuildingStore.AssertEntry(PacketInfo->GarrBuildingID);
        if (PacketInfo->TimeBuilt + building->BuildSeconds <= GameTime::GetGameTime())
            return true;
    }

    return false;
}

uint32 Garrison::Follower::GetItemLevel() const
{
    return (PacketInfo.ItemLevelWeapon + PacketInfo.ItemLevelArmor) / 2;
}

bool Garrison::Follower::HasAbility(uint32 garrAbilityId) const
{
    return advstd::ranges::contains(PacketInfo.AbilityID, garrAbilityId, &GarrAbilityEntry::ID);
}
