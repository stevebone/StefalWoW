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
#include "DB2Structure.h"
#include "GameObject.h"
#include "GameTime.h"
#include "GarrisonAutoCombat.h"
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
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "VehicleDefines.h"
#include "advstd.h"

Garrison::Garrison(Player* owner) : _owner(owner), _garrType(GARRISON_TYPE_GARRISON), _siteLevel(nullptr), _followerActivationsRemainingToday(1)
{
    // Fire the first periodic pass on the very next tick after login (instead of waiting a full interval),
    // so finished-order crates activate, completed constructions/research resolve, etc. right away.
    _updateTimer = GARRISON_UPDATE_INTERVAL;
}

bool Garrison::LoadFromDB(PreparedQueryResult garrison, PreparedQueryResult blueprints, PreparedQueryResult buildings,
    PreparedQueryResult followers, PreparedQueryResult abilities, PreparedQueryResult missions,
    PreparedQueryResult specializations, PreparedQueryResult shipments, PreparedQueryResult talents,
    PreparedQueryResult trophies, PreparedQueryResult archivedMissions)
{
    if (!garrison)
        return false;

    Field* fields = garrison->Fetch();
    _siteLevel = sGarrSiteLevelStore.LookupEntry(fields[0].GetUInt32());
    _followerActivationsRemainingToday = fields[1].GetUInt32();
    _garrType = static_cast<GarrisonType>(fields[2].GetUInt32());
    _missionsStartedToday = fields[3].GetUInt32();
    _lastMissionStartDay = fields[4].GetUInt32();
    _cacheLastUsed = fields[5].GetInt64();
    // Legacy rows (created before the cache was persisted) have 0 here; start their timer now so the
    // resource cache begins accruing from this login rather than paying out for all of history at once.
    if (!_cacheLastUsed)
        _cacheLastUsed = GameTime::GetGameTime();
    // WoD Shipyard tier (GarrBuilding 205/206/207); 0 = not built. Not a plot building, so tracked directly.
    _shipyardBuilding = fields[6].GetUInt32();
    if (!_siteLevel)
        return false;

    // Reset daily mission counter if the day has changed
    uint32 today = static_cast<uint32>(GameTime::GetGameTime() / DAY);
    if (today != _lastMissionStartDay)
    {
        _missionsStartedToday = 0;
        _lastMissionStartDay = today;
    }

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
            uint32 currentGarSpecId = fields[4].GetUInt32();
            time_t timeSpecCooldown = fields[5].GetInt64();

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
            plot->BuildingInfo.PacketInfo->CurrentGarSpecID = currentGarSpecId;
            plot->BuildingInfo.PacketInfo->TimeSpecCooldown = timeSpecCooldown;

        } while (buildings->NextRow());
    }

    if (specializations)
    {
        do
        {
            fields = specializations->Fetch();
            if (sGarrSpecializationStore.LookupEntry(fields[0].GetUInt32()))
                _knownSpecializations.insert(fields[0].GetUInt32());

        } while (specializations->NextRow());
    }

    //           0      1           2              3             4         5
    // SELECT dbId, shipmentId, plotInstanceId, creationTime, duration, assignedFollowerDbId FROM character_garrison_shipments WHERE guid = ?
    if (shipments)
    {
        do
        {
            fields = shipments->Fetch();
            Shipment& shipment = _shipments[fields[0].GetUInt64()];
            shipment.DbID = fields[0].GetUInt64();
            shipment.ShipmentRecID = fields[1].GetUInt32();
            shipment.PlotInstanceID = fields[2].GetUInt32();
            shipment.CreationTime = time_t(fields[3].GetInt64());
            shipment.Duration = fields[4].GetInt32();
            shipment.AssignedFollowerDBID = fields[5].GetUInt64();

        } while (shipments->NextRow());
    }

    //           0              1       2                   3      4                  5
    // SELECT garrTalentId, `rank`, researchStartTime, flags, soulbindConduitId, soulbindConduitRank FROM character_garrison_talents WHERE guid = ?
    if (talents)
    {
        do
        {
            fields = talents->Fetch();
            uint32 garrTalentID = fields[0].GetUInt32();
            Talent& talent = _talents[garrTalentID];
            talent.GarrTalentID = garrTalentID;
            talent.Rank = fields[1].GetInt32();
            talent.ResearchStartTime = time_t(fields[2].GetInt64());
            talent.Flags = fields[3].GetInt32();
            talent.SoulbindConduitID = fields[4].GetInt32();
            talent.SoulbindConduitRank = fields[5].GetInt32();

        } while (talents->NextRow());
    }

    //           0
    // SELECT trophyId FROM character_garrison_trophies WHERE guid = ?
    if (trophies)
    {
        do
        {
            fields = trophies->Fetch();
            _trophies.insert(fields[0].GetUInt32());
        } while (trophies->NextRow());
    }

    //           0          1
    // SELECT garrType, missionRecID FROM character_garrison_archived_missions WHERE guid = ?
    if (archivedMissions)
    {
        do
        {
            fields = archivedMissions->Fetch();
            uint32 garrType = fields[0].GetUInt32();
            if (garrType == static_cast<uint32>(_garrType))
                _archivedMissions.push_back(fields[1].GetInt32());
        } while (archivedMissions->NextRow());
    }

    //           0           1        2      3                4               5   6                7               8       9          10         11
    // SELECT dbId, followerId, quality, level, itemLevelWeapon, itemLevelArmor, xp, currentBuilding, currentMission, status, durability, customName FROM character_garrison_followers WHERE guid = ?
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
            follower.PacketInfo.Durability = fields[10].GetUInt32();
            follower.PacketInfo.CustomName = fields[11].GetString();
            follower.PacketInfo.ZoneSupportSpellID = sGarrisonMgr.GetFollowerZoneSupportSpell(followerId, GetFaction());
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
            {
                mission.PacketInfo.MissionScalar = missionEntry->AutoMissionScalar;
                mission.PacketInfo.Flags = missionEntry->Flags;

                // Encounters, Rewards and OvermaxRewards are runtime-only (not stored in
                // character_garrison_missions), so regenerate them from DB2 on load — otherwise a mission
                // that was in progress across a restart reloads with an empty Rewards vector and
                // ClaimMissionReward grants the player nothing. Same source used when the mission is first
                // offered (AddMission -> PopulateMissionData), so the data is identical.
                PopulateMissionData(mission, missionEntry);
            }

        } while (missions->NextRow());
    }

    // Rebuild the mission -> assigned-followers link. CurrentFollowerDBIDs is runtime-only and not
    // persisted; the authoritative record is each follower's persisted CurrentMissionID (== missionRecID,
    // set in StartMission). Without this rebuild, a mission that was in progress across a restart reloads
    // with an empty follower list, so ClaimMissionReward awards no follower XP and never clears
    // CurrentMissionID — leaving the follower permanently stuck "on a mission". Followers whose mission is
    // no longer present (already claimed/removed) are orphans and get freed here.
    for (auto& [followerDbId, follower] : _followers)
    {
        if (follower.PacketInfo.CurrentMissionID == 0)
            continue;

        if (Mission* mission = GetMissionByRecID(follower.PacketInfo.CurrentMissionID))
            mission->CurrentFollowerDBIDs.push_back(follower.PacketInfo.DbID);
        else
            follower.PacketInfo.CurrentMissionID = 0; // orphaned link — the mission is gone, free the follower
    }

    // Complete any talent research that finished while offline
    CompleteAllTalentResearch();

    return true;
}

void Garrison::SaveToDB(CharacterDatabaseTransaction trans)
{
    DeleteFromDB(_owner->GetGUID().GetCounter(), trans);

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHARACTER_GARRISON);
    stmt->setUInt64(0, _owner->GetGUID().GetCounter());
    stmt->setUInt32(1, _siteLevel->ID);
    stmt->setUInt32(2, _followerActivationsRemainingToday);
    stmt->setUInt32(3, static_cast<uint32>(_garrType));
    stmt->setUInt32(4, _missionsStartedToday);
    stmt->setUInt32(5, _lastMissionStartDay);
    stmt->setInt64(6, _cacheLastUsed);
    stmt->setUInt32(7, _shipyardBuilding);
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
            stmt->setUInt32(5, plot.BuildingInfo.PacketInfo->CurrentGarSpecID);
            stmt->setInt64(6, plot.BuildingInfo.PacketInfo->TimeSpecCooldown);
            trans->Append(stmt);
        }
    }

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHARACTER_GARRISON_SPECIALIZATIONS);
    stmt->setUInt64(0, _owner->GetGUID().GetCounter());
    trans->Append(stmt);

    for (uint32 specId : _knownSpecializations)
    {
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHARACTER_GARRISON_SPECIALIZATIONS);
        stmt->setUInt64(0, _owner->GetGUID().GetCounter());
        stmt->setUInt32(1, specId);
        trans->Append(stmt);
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
        stmt->setUInt32(index++, follower.PacketInfo.Durability);
        stmt->setString(index++, follower.PacketInfo.CustomName);
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

    for (auto const& p : _shipments)
    {
        Shipment const& shipment = p.second;
        uint8 index = 0;
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHARACTER_GARRISON_SHIPMENTS);
        stmt->setUInt64(index++, shipment.DbID);
        stmt->setUInt64(index++, _owner->GetGUID().GetCounter());
        stmt->setUInt32(index++, shipment.ShipmentRecID);
        stmt->setUInt32(index++, shipment.PlotInstanceID);
        stmt->setInt64(index++, shipment.CreationTime);
        stmt->setInt32(index++, shipment.Duration);
        stmt->setUInt64(index++, shipment.AssignedFollowerDBID);
        trans->Append(stmt);
    }

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHARACTER_GARRISON_TALENTS);
    stmt->setUInt64(0, _owner->GetGUID().GetCounter());
    trans->Append(stmt);

    for (auto const& [talentId, talent] : _talents)
    {
        uint8 index = 0;
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHARACTER_GARRISON_TALENT);
        stmt->setUInt64(index++, _owner->GetGUID().GetCounter());
        stmt->setUInt32(index++, talent.GarrTalentID);
        stmt->setInt32(index++, talent.Rank);
        stmt->setInt64(index++, talent.ResearchStartTime);
        stmt->setInt32(index++, talent.Flags);
        stmt->setInt32(index++, talent.SoulbindConduitID);
        stmt->setInt32(index++, talent.SoulbindConduitRank);
        trans->Append(stmt);
    }

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHARACTER_GARRISON_TROPHIES);
    stmt->setUInt64(0, _owner->GetGUID().GetCounter());
    trans->Append(stmt);

    for (uint32 trophyId : _trophies)
    {
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHARACTER_GARRISON_TROPHY);
        stmt->setUInt64(0, _owner->GetGUID().GetCounter());
        stmt->setUInt32(1, trophyId);
        trans->Append(stmt);
    }

    for (int32 missionRecID : _archivedMissions)
    {
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHARACTER_GARRISON_ARCHIVED_MISSION);
        stmt->setUInt64(0, _owner->GetGUID().GetCounter());
        stmt->setUInt32(1, static_cast<uint32>(_garrType));
        stmt->setInt32(2, missionRecID);
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

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHARACTER_GARRISON_SPECIALIZATIONS);
    stmt->setUInt64(0, ownerGuid);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHARACTER_GARRISON_SHIPMENTS);
    stmt->setUInt64(0, ownerGuid);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHARACTER_GARRISON_TALENTS);
    stmt->setUInt64(0, ownerGuid);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHARACTER_GARRISON_TROPHIES);
    stmt->setUInt64(0, ownerGuid);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHARACTER_GARRISON_ARCHIVED_MISSIONS);
    stmt->setUInt64(0, ownerGuid);
    trans->Append(stmt);
}

static GarrisonType GetGarrisonTypeFromSiteId(uint32 garrSiteId)
{
    switch (garrSiteId)
    {
        case 2:   return GARRISON_TYPE_GARRISON;      // WoD
        case 71:  return GARRISON_TYPE_CLASS_ORDER;    // Legion
        case 173: return GARRISON_TYPE_WAR_CAMPAIGN;   // BfA
        case 500: return GARRISON_TYPE_COVENANT;       // Shadowlands
        default:  return GARRISON_TYPE_GARRISON;
    }
}

bool Garrison::Create(uint32 garrSiteId)
{
    GarrSiteLevelEntry const* siteLevel = sGarrisonMgr.GetGarrSiteLevelEntry(garrSiteId, 1);
    if (!siteLevel)
        return false;

    _siteLevel = siteLevel;
    _garrType = GetGarrisonTypeFromSiteId(garrSiteId);
    _cacheLastUsed = GameTime::GetGameTime(); // start the resource cache accruing from creation

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
    GarrSiteLevelEntry const* nextLevel = sGarrisonMgr.GetGarrSiteLevelEntry(_siteLevel->GarrSiteID, _siteLevel->GarrLevel + 1);
    if (!nextLevel)
        return;

    // Play upgrade cinematic before changing level
    if (nextLevel->UpgradeMovieID)
        _owner->SendMovieStart(nextLevel->UpgradeMovieID);

    // Save existing buildings keyed by plot instance ID
    std::unordered_map<uint32 /*garrPlotInstanceId*/, WorldPackets::Garrison::GarrisonBuildingInfo> existingBuildings;
    for (auto const& p : _plots)
        if (p.second.BuildingInfo.PacketInfo)
            existingBuildings[p.first] = *p.second.BuildingInfo.PacketInfo;

    // Remove existing plot game objects from the map
    Map* map = FindMap();
    if (map)
        for (auto& p : _plots)
            p.second.DeleteGameObject(map);

    // Advance to next site level
    _siteLevel = nextLevel;

    // Credit the "reach garrison level N" criteria so the level-upgrade quests complete (e.g. quest 36615
    // "My Very Own Castle" has a CRITERIA_TREE objective "Upgrade your garrison to Tier 3"). Without this the
    // quest never registers the upgrade. miscValue1 = the new GarrLevel the criteria tree checks against.
    _owner->UpdateCriteria(CriteriaType::UpgradeGarrison, _siteLevel->GarrLevel);

    // Re-initialize plots for the new level (this adds new plot slots)
    _plots.clear();
    InitializePlots();

    // Restore buildings that still exist in the new layout
    for (auto const& [plotInstanceId, buildingInfo] : existingBuildings)
    {
        Plot* plot = GetPlot(plotInstanceId);
        if (!plot)
            continue;

        plot->BuildingInfo.PacketInfo = buildingInfo;
    }

    // Spawn game objects for all plots on the map
    if (map)
    {
        for (auto& p : _plots)
            if (GameObject* go = p.second.CreateGameObject(map, GetFaction()))
                map->AddToMap(go);
    }

    // Update phasing for the new garrison level
    PhasingHandler::OnConditionChange(_owner);
    SendRemoteInfo();

    // WoD garrison levels are distinct child maps (GarrSiteLevel.MapID), not just phases. The world only
    // re-renders at the new level once the player is moved onto that level's map instance - its GarrisonMap
    // grid-loader then spawns the level-appropriate buildings (mirrors what happens when the player re-enters
    // the garrison via the entrance AreaTrigger -> Garrison::Enter). Retail performs this teleport under the
    // upgrade cinematic; without it the Architect UI advances but the player stays on the old level's map.
    if (_owner->IsInWorld() && int32(_owner->GetMapId()) != int32(_siteLevel->MapID))
        _owner->TeleportTo(WorldLocation(_siteLevel->MapID, *_owner), TELE_TO_SEAMLESS);

    // Push fresh full garrison info so the world map (M) and Architect reflect the new site level + plot layout
    // immediately, rather than showing the previous level until the client next re-requests (relog / re-enter).
    SendInfo();
}

// Build the WoD Shipyard. It is a garrison sub-feature (GarrBuilding 205/206/207 = Shipyard L1/L2/L3, BuildingType
// 9) that, unlike normal buildings, has NO architect plot (no GarrBuildingPlotInst entry) and physically lives on
// the naval map (1473 Alliance / 1474 Horde). We therefore track only its tier (_shipyardBuilding) rather than a
// plot. Gated on a full garrison (type 2) at site level 3 - the same prerequisite retail uses (the naval command
// table becomes available once the garrison reaches Tier 3). Persisted immediately so it survives a crash.
//
// NOTE (Phase 1): this establishes the server-side shipyard state + persistence. The client-facing pieces - showing
// the shipyard building in GarrisonInfo (needs the exact naval-map plot-instance id the 12.0.7 client expects) and
// the walk-in naval map + terrain swaps - are deliberately NOT wired here: pushing a guessed plot-instance id into
// the info packet risks a client-side placement error (same failure class as the earlier crate/gossip issues), so
// that value must be sniff-verified before it goes on the wire. See [[shipyard_foundation_68275]].
void Garrison::CreateShipyard()
{
    // Only a real garrison has a shipyard, and only from Tier 3 onward.
    if (GetType() != GARRISON_TYPE_GARRISON || !_siteLevel || _siteLevel->GarrLevel < 3)
        return;

    if (HasShipyard())
        return;

    GarrBuildingEntry const* shipyard = sGarrBuildingStore.LookupEntry(GARRISON_SHIPYARD_BUILDING_L1);
    if (!shipyard)
        return;

    _shipyardBuilding = GARRISON_SHIPYARD_BUILDING_L1;

    // Crash-safe immediate persistence (mirrors the work-order INSERT-on-place pattern) rather than waiting for the
    // next full garrison save.
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHARACTER_GARRISON_SHIPYARD);
    stmt->setUInt32(0, _shipyardBuilding);
    stmt->setUInt64(1, _owner->GetGUID().GetCounter());
    stmt->setUInt32(2, static_cast<uint32>(_garrType));
    CharacterDatabase.Execute(stmt);

    // Refresh garrison info so the client picks up the new state on its next read.
    SendInfo();
}

bool Garrison::IsMissionFollowerTypeAvailable(int8 followerTypeId) const
{
    // The garrison's own primary follower type is always available.
    if (followerTypeId == static_cast<int8>(sGarrisonMgr.GetPrimaryFollowerType(static_cast<int8>(GetType()))))
        return true;
    // Naval (shipyard) missions/ships only become available once the shipyard has been built. Both share
    // GarrTypeID 2 with the garrison, so without this gate the naval mission pool (GarrFollowerTypeID 2) would
    // either never appear or leak in before the shipyard exists.
    if (followerTypeId == static_cast<int8>(FOLLOWER_TYPE_SHIPYARD))
        return HasShipyard();
    return false;
}

void Garrison::Update(uint32 diff)
{
    _updateTimer += diff;
    if (_updateTimer < GARRISON_UPDATE_INTERVAL)
        return;

    _updateTimer -= GARRISON_UPDATE_INTERVAL;

    // Finished work orders stay "ready" and are collected by clicking the building's work-order crate
    // (GameObject::Use -> CollectReadyShipments). The crate is made interactable purely by its base
    // gameobject_template_addon.flags (GO_FLAG_IGNORE_CURRENT_STATE_FOR_USE_SPELL_EXCEPT_UNLOCKED,
    // 0x40000) - matching retail's on-wire crate - so no per-tick flag maintenance is needed here.

    // Keep each work-order crate's "filled with goods" display in sync with the orders on its plot.
    UpdateWorkOrderCrates();

    // Buildings are NOT auto-completed when their construction timer finishes. Retail leaves the finished
    // building as "ready to complete": the player walks to the plot and clicks it (construction sign), the
    // client then sends CMSG_GARRISON_SET_BUILDING_ACTIVE -> HandleGarrisonSetBuildingActive -> ActivateBuilding.
    // The client already knows a building is ready from its TimeBuilt + BuildSeconds, so no server push is
    // needed here. (Buildings that finished while offline get their finalizer/complete state on the next
    // garrison map entry via Plot::CreateGameObject's CanActivate branch.)

    // Complete talent research that has finished
    CompleteAllTalentResearch();

    // Remove expired unclaimed missions
    RemoveExpiredMissions();

    // Refill the mission board over time - retail continuously offers new follower missions up to the
    // cap. Throttled so it tops up a few at a time rather than on every 60s tick. (This is what was
    // missing: GenerateAvailableMissions existed but nothing ever called it, so the board never refilled.)
    static constexpr time_t MISSION_GENERATION_INTERVAL = 10 * MINUTE;
    if (GameTime::GetGameTime() - _lastMissionGenerationTime >= MISSION_GENERATION_INTERVAL)
        GenerateAvailableMissions();

    // #17: the client's garrison report only re-evaluates mission completion when it receives garrison
    // info (which fires GARRISON_MISSION_LIST_UPDATE) - a follower mission finishing is a purely time-based
    // client computation with no server event, so an open report goes stale until the next interaction.
    // Count in-progress missions whose timer has elapsed; when that grows, re-send the garrison info so the
    // report refreshes and shows them as ready to complete.
    time_t const now = GameTime::GetGameTime();
    uint32 finishedMissions = 0;
    for (auto const& [dbId, mission] : _missions)
    {
        if (mission.PacketInfo.MissionState != 1) // 1 = In Progress
            continue;
        int64 const finishAt = int64(mission.PacketInfo.StartTime)
            + Seconds(mission.PacketInfo.TravelDuration).count()
            + Seconds(mission.PacketInfo.MissionDuration).count();
        if (finishAt <= int64(now))
            ++finishedMissions;
    }

    if (finishedMissions != _lastFinishedMissionCount)
    {
        _lastFinishedMissionCount = finishedMissions;
        SendRemoteInfo();
    }
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
            {
                // Send BuildingRemoved BEFORE ClearBuildingInfo (which sends PlotRemoved)
                // so the client processes removal in the correct order
                WorldPackets::Garrison::GarrisonBuildingRemoved buildingRemoved;
                buildingRemoved.GarrTypeID = GetType();
                buildingRemoved.Result = GARRISON_SUCCESS;
                buildingRemoved.GarrPlotInstanceID = garrPlotInstanceId;
                buildingRemoved.GarrBuildingID = oldBuildingId;
                _owner->SendDirectMessage(buildingRemoved.Write());

                plot->ClearBuildingInfo(GetType(), _owner);
            }
        }

        plot->SetBuildingInfo(placeBuildingResult.BuildingInfo, _owner);
        if (map)
            if (GameObject* go = plot->CreateGameObject(map, GetFaction()))
                map->AddToMap(go);

        if (building->CurrencyTypeID && building->CurrencyQty)
            _owner->RemoveCurrency(building->CurrencyTypeID, building->CurrencyQty, CurrencyDestroyReason::Garrison);
        if (building->GoldCost)
            _owner->ModifyMoney(-building->GoldCost * GOLD, false);

        if (oldBuildingId)
        {
            GarrBuildingEntry const* oldBuilding = sGarrBuildingStore.AssertEntry(oldBuildingId);
            // Same-type upgrade: BuildingRemoved wasn't sent above, send it now
            if (oldBuilding->BuildingType == building->BuildingType)
            {
                WorldPackets::Garrison::GarrisonBuildingRemoved buildingRemoved;
                buildingRemoved.GarrTypeID = GetType();
                buildingRemoved.Result = GARRISON_SUCCESS;
                buildingRemoved.GarrPlotInstanceID = garrPlotInstanceId;
                buildingRemoved.GarrBuildingID = oldBuildingId;
                _owner->SendDirectMessage(buildingRemoved.Write());
            }
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
        // Refund construction/upgrade cost (only what was actually charged)
        if (constructing->CurrencyTypeID && constructing->CurrencyQty)
            _owner->AddCurrency(constructing->CurrencyTypeID, constructing->CurrencyQty, CurrencyGainSource::GarrisonBuildingRefund);
        if (constructing->GoldCost)
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

void Garrison::SwapBuildings(uint32 plotId1, uint32 plotId2)
{
    WorldPackets::Garrison::GarrisonSwapBuildingsResponse swapResult;
    swapResult.Result = GARRISON_SUCCESS;
    swapResult.PlotInstanceID1 = plotId1;
    swapResult.PlotInstanceID2 = plotId2;

    Plot* plot1 = GetPlot(plotId1);
    Plot* plot2 = GetPlot(plotId2);

    if (!plot1 || !plot2)
    {
        swapResult.Result = GARRISON_ERROR_INVALID_PLOT_INSTANCEID;
        _owner->SendDirectMessage(swapResult.Write());
        return;
    }

    if (!plot1->BuildingInfo.PacketInfo || !plot2->BuildingInfo.PacketInfo)
    {
        swapResult.Result = GARRISON_ERROR_NO_BUILDING;
        _owner->SendDirectMessage(swapResult.Write());
        return;
    }

    // Both plots must have the same PlotType to swap
    if (plot1->PacketInfo.PlotType != plot2->PacketInfo.PlotType)
    {
        swapResult.Result = GARRISON_ERROR_INVALID_PLOT_BUILDING;
        _owner->SendDirectMessage(swapResult.Write());
        return;
    }

    Map* map = FindMap();

    // Remove game objects from both plots
    if (map)
    {
        plot1->DeleteGameObject(map);
        plot2->DeleteGameObject(map);
    }

    // Swap building info between the two plots
    std::swap(plot1->BuildingInfo.PacketInfo, plot2->BuildingInfo.PacketInfo);

    // Update the GarrPlotInstanceID in each building's packet info to match the new plot
    plot1->BuildingInfo.PacketInfo->GarrPlotInstanceID = plotId1;
    plot2->BuildingInfo.PacketInfo->GarrPlotInstanceID = plotId2;

    // Recreate game objects
    if (map)
    {
        if (GameObject* go1 = plot1->CreateGameObject(map, GetFaction()))
            map->AddToMap(go1);
        if (GameObject* go2 = plot2->CreateGameObject(map, GetFaction()))
            map->AddToMap(go2);
    }

    _owner->SendDirectMessage(swapResult.Write());
}

GarrBuildingEntry const* Garrison::GetActiveBuildingByType(uint32 buildingType) const
{
    for (auto const& [plotId, plot] : _plots)
    {
        if (!plot.BuildingInfo.PacketInfo)
            continue;
        if (!plot.BuildingInfo.PacketInfo->Active)
            continue;

        GarrBuildingEntry const* building = sGarrBuildingStore.LookupEntry(plot.BuildingInfo.PacketInfo->GarrBuildingID);
        if (building && building->BuildingType == buildingType)
            return building;
    }
    return nullptr;
}

uint32 Garrison::GetBonusFollowerSlots() const
{
    static constexpr uint32 BUILDING_TYPE_BARRACKS = 26;

    GarrBuildingEntry const* barracks = GetActiveBuildingByType(BUILDING_TYPE_BARRACKS);
    if (!barracks)
        return 0;

    // Barracks level 2: +5 follower slots, level 3: +5 more (total +10)
    if (barracks->UpgradeLevel >= 3)
        return 10;
    if (barracks->UpgradeLevel >= 2)
        return 5;

    return 0;
}

void Garrison::LearnSpecialization(uint32 garrSpecId)
{
    GarrSpecializationEntry const* specEntry = sGarrSpecializationStore.LookupEntry(garrSpecId);
    if (!specEntry)
        return;

    if (specEntry->GarrTypeID != static_cast<uint8>(GetType()))
        return;

    if (_knownSpecializations.count(garrSpecId))
        return;

    _knownSpecializations.insert(garrSpecId);
    SendBlueprintAndSpecializationData();
}

void Garrison::SetBuildingSpecialization(uint32 garrPlotInstanceId, uint32 garrSpecId)
{
    Plot* plot = GetPlot(garrPlotInstanceId);
    if (!plot || !plot->BuildingInfo.PacketInfo || !plot->BuildingInfo.PacketInfo->Active)
        return;

    if (garrSpecId != 0)
    {
        GarrSpecializationEntry const* specEntry = sGarrSpecializationStore.LookupEntry(garrSpecId);
        if (!specEntry)
            return;

        if (!HasSpecialization(garrSpecId))
            return;

        // Check cooldown
        if (plot->BuildingInfo.PacketInfo->CurrentGarSpecID != 0 &&
            plot->BuildingInfo.PacketInfo->TimeSpecCooldown > GameTime::GetGameTime())
            return;
    }

    plot->BuildingInfo.PacketInfo->CurrentGarSpecID = garrSpecId;

    // Set cooldown for changing specialization (1 day)
    if (garrSpecId != 0)
        plot->BuildingInfo.PacketInfo->TimeSpecCooldown = GameTime::GetGameTime() + DAY;
}

void Garrison::AddFollower(uint32 garrFollowerId)
{
    WorldPackets::Garrison::GarrisonAddFollowerResult addFollowerResult;
    addFollowerResult.GarrTypeID = GetType();
    GarrFollowerEntry const* followerEntry = sGarrFollowerStore.LookupEntry(garrFollowerId);
    if (!followerEntry)
    {
        addFollowerResult.Result = GARRISON_ERROR_FOLLOWER_EXISTS;
        _owner->SendDirectMessage(addFollowerResult.Write());
        return;
    }

    if (followerEntry->GarrTypeID != static_cast<uint8>(GetType()))
    {
        addFollowerResult.Result = GARRISON_ERROR_INVALID_GARRISON;
        _owner->SendDirectMessage(addFollowerResult.Write());
        return;
    }

    if (_followerIds.count(garrFollowerId))
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
    // Initial quality from DB2; quality upgrades happen post-recruit via SPELL_EFFECT_SET_FOLLOWER_QUALITY (Spell::EffectSetFollowerQuality)
    follower.PacketInfo.Quality = followerEntry->Quality;
    follower.PacketInfo.FollowerLevel = followerEntry->FollowerLevel;
    follower.PacketInfo.ItemLevelWeapon = followerEntry->ItemLevelWeapon;
    follower.PacketInfo.ItemLevelArmor = followerEntry->ItemLevelArmor;
    follower.PacketInfo.Xp = 0;
    follower.PacketInfo.CurrentBuildingID = 0;
    follower.PacketInfo.CurrentMissionID = 0;
    follower.PacketInfo.AbilityID = sGarrisonMgr.RollFollowerAbilities(garrFollowerId, followerEntry, follower.PacketInfo.Quality, GetFaction(), true);
    follower.PacketInfo.FollowerStatus = 0;
    follower.PacketInfo.ZoneSupportSpellID = sGarrisonMgr.GetFollowerZoneSupportSpell(garrFollowerId, GetFaction());

    addFollowerResult.Follower = follower.PacketInfo;
    _owner->SendDirectMessage(addFollowerResult.Write());

    _owner->UpdateCriteria(CriteriaType::RecruitGarrisonFollower, follower.PacketInfo.DbID);
}

void Garrison::AddTroop(uint32 garrFollowerId, uint32 durability)
{
    WorldPackets::Garrison::GarrisonAddFollowerResult addFollowerResult;
    addFollowerResult.GarrTypeID = GetType();
    GarrFollowerEntry const* followerEntry = sGarrFollowerStore.LookupEntry(garrFollowerId);
    if (!followerEntry)
    {
        addFollowerResult.Result = GARRISON_ERROR_INVALID_FOLLOWER;
        _owner->SendDirectMessage(addFollowerResult.Write());
        return;
    }

    uint64 dbId = sGarrisonMgr.GenerateFollowerDbId();
    Follower& follower = _followers[dbId];
    follower.PacketInfo.DbID = dbId;
    follower.PacketInfo.GarrFollowerID = garrFollowerId;
    follower.PacketInfo.Quality = followerEntry->Quality;
    follower.PacketInfo.FollowerLevel = followerEntry->FollowerLevel;
    follower.PacketInfo.ItemLevelWeapon = followerEntry->ItemLevelWeapon;
    follower.PacketInfo.ItemLevelArmor = followerEntry->ItemLevelArmor;
    follower.PacketInfo.Xp = 0;
    follower.PacketInfo.Durability = durability;
    follower.PacketInfo.CurrentBuildingID = 0;
    follower.PacketInfo.CurrentMissionID = 0;
    follower.PacketInfo.AbilityID = sGarrisonMgr.RollFollowerAbilities(garrFollowerId, followerEntry, follower.PacketInfo.Quality, GetFaction(), true);
    follower.PacketInfo.FollowerStatus = FOLLOWER_STATUS_TROOP;
    follower.PacketInfo.ZoneSupportSpellID = sGarrisonMgr.GetFollowerZoneSupportSpell(garrFollowerId, GetFaction());

    addFollowerResult.Follower = follower.PacketInfo;
    _owner->SendDirectMessage(addFollowerResult.Write());
}

Garrison::Follower const* Garrison::GetFollower(uint64 dbId) const
{
    auto itr = _followers.find(dbId);
    if (itr != _followers.end())
        return &itr->second;

    return nullptr;
}

Garrison::Follower const* Garrison::GetFollowerByEntry(uint32 garrFollowerId) const
{
    for (auto const& [dbId, follower] : _followers)
        if (follower.PacketInfo.GarrFollowerID == garrFollowerId)
            return &follower;

    return nullptr;
}

void Garrison::BuildInfoPacket(WorldPackets::Garrison::GarrisonInfo& garrison) const
{
    garrison.GarrTypeID = GetType();
    garrison.GarrSiteID = _siteLevel->GarrSiteID;
    garrison.GarrSiteLevelID = _siteLevel->ID;
    garrison.NumFollowerActivationsRemaining = _followerActivationsRemainingToday;
    garrison.NumMissionsStartedToday = _missionsStartedToday;
    garrison.ArchivedMissions = _archivedMissions;
    for (auto& p : _plots)
    {
        Plot const& plot = p.second;
        garrison.Plots.push_back(&plot.PacketInfo);
        if (plot.BuildingInfo.PacketInfo)
            garrison.Buildings.push_back(&*plot.BuildingInfo.PacketInfo);
    }

    for (auto const& p : _followers)
        garrison.Followers.push_back(&p.second.PacketInfo);

    // Missions: inline Encounters/Rewards/OvermaxRewards must be empty in the
    // GarrisonInfo mission structs — the rewards go ONLY in the garrison-level
    // parallel arrays.  The client reads both, so writing them in both places
    // causes a packet desync (double data).  We build temporary copies with
    // the inline vectors cleared.
    _infoMissions.clear();
    _infoMissions.reserve(_missions.size());
    for (auto const& p : _missions)
    {
        auto& copy = _infoMissions.emplace_back(p.second.PacketInfo);
        garrison.MissionRewards.push_back(copy.Rewards);
        garrison.MissionOvermaxRewards.push_back(copy.OvermaxRewards);
        copy.Encounters.clear();
        copy.Rewards.clear();
        copy.OvermaxRewards.clear();
        garrison.Missions.push_back(&copy);
        garrison.CanStartMission.push_back(true);
    }

    for (auto const& [talentId, talent] : _talents)
    {
        WorldPackets::Garrison::GarrisonTalent garrisonTalent;
        garrisonTalent.GarrTalentID = talent.GarrTalentID;
        garrisonTalent.Rank = talent.Rank;
        garrisonTalent.ResearchStartTime = time_t(talent.ResearchStartTime);
        garrisonTalent.Flags = talent.Flags;
        if (talent.SoulbindConduitID != 0)
        {
            WorldPackets::Garrison::GarrisonTalentSocketData socket;
            socket.SoulbindConduitID = talent.SoulbindConduitID;
            socket.SoulbindConduitRank = talent.SoulbindConduitRank;
            garrisonTalent.Socket = socket;
        }
        garrison.Talents.push_back(garrisonTalent);
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

// Resend the full garrison info unsolicited (same payload as HandleGetGarrisonInfo). The client normally
// requests this on login / on entering the garrison; after an in-session change that alters the site level or
// building layout (e.g. an upgrade), the world map keeps showing the stale layout until the client re-requests.
// Pushing it explicitly refreshes the site level + per-plot buildings so the world map (M) reflects the new level.
void Garrison::SendInfo() const
{
    SendTroopQualityRefresh();

    WorldPackets::Garrison::GetGarrisonInfoResult garrisonInfo;
    garrisonInfo.FactionIndex = GetFaction();
    BuildInfoPacket(garrisonInfo.Garrisons.emplace_back());
    garrisonInfo.FollowerSoftCaps = {
        { FOLLOWER_TYPE_GARRISON,     20 },
        { FOLLOWER_TYPE_SHIPYARD,      6 },
        { FOLLOWER_TYPE_CLASS_ORDER,   6 },
        { FOLLOWER_TYPE_WAR_CAMPAIGN, 30 },
        { FOLLOWER_TYPE_COVENANT,    100 }
    };
    _owner->SendDirectMessage(garrisonInfo.Write());

    SendDeleteExpiredMissionsResult();
    SendMissionStartConditionUpdate();
}

void Garrison::SendBlueprintAndSpecializationData()
{
    WorldPackets::Garrison::GarrisonRequestBlueprintAndSpecializationDataResult data;
    data.GarrTypeID = GetType();
    data.BlueprintsKnown = &_knownBuildings;
    data.SpecializationsKnown = &_knownSpecializations;
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

void Garrison::SendMissionStartConditionUpdate() const
{
    WorldPackets::Garrison::GarrisonMissionStartConditionUpdate update;
    update.MissionRecIDs.reserve(_missions.size());
    update.CanStartMission.reserve(_missions.size());

    for (auto const& [dbId, mission] : _missions)
    {
        update.MissionRecIDs.push_back(mission.PacketInfo.MissionRecID);
        update.CanStartMission.push_back(mission.PacketInfo.MissionState == 0);
    }

    _owner->SendDirectMessage(update.Write());
}

void Garrison::SendDeleteExpiredMissionsResult() const
{
    WorldPackets::Garrison::DeleteExpiredMissionsResult result;
    result.GarrTypeID = static_cast<uint8>(_garrType);
    result.Result = GARRISON_SUCCESS; // must be 0 (client gates the mission-open fire on it) — GARRISON_SUCCESS == 0
    result.Succeeded = true;
    // CRITICAL: this bit (wire bit6, the second packed bit) MUST be 0. The 68275 client fires the
    // legacy GARRISON_MISSION_NPC_OPENED event from the SMSG_DELETE_EXPIRED_MISSIONS_RESULT (0x4C0022)
    // handler *while PlayerInteractionType == GarrMission(32)* — but ONLY if the second u32 (Result) is 0
    // AND this trailing bit is 0. Sending it as 1 makes the client silently skip the fire, which is why
    // the WoD command table never opened. (Binary-traced: fire fn sub_7FF72AD3DAD0, gate cmp [mgr+0x30],0x20.)
    result.LegionUnkBit = false;
    // RemovedMissions empty (count=0) is sufficient; expired missions already cleaned by RemoveExpiredMissions()
    _owner->SendDirectMessage(result.Write());
}

void Garrison::SendTroopQualityRefresh() const
{
    // Sniff-confirmed: server sends GarrisonFollowerChangedQuality for each troop
    // at login BEFORE the main GetGarrisonInfoResult packet
    for (auto const& [dbId, follower] : _followers)
    {
        if (follower.PacketInfo.FollowerStatus & FOLLOWER_STATUS_TROOP)
        {
            WorldPackets::Garrison::GarrisonFollowerChangedQuality changedQuality;
            changedQuality.OldFollower = follower.PacketInfo;
            changedQuality.Follower = follower.PacketInfo;
            _owner->SendDirectMessage(changedQuality.Write());
        }
    }
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

    // Troops cannot be favorited
    if (follower->PacketInfo.FollowerStatus & FOLLOWER_STATUS_TROOP)
    {
        result.Result = GARRISON_ERROR_INVALID_FOLLOWER;
        _owner->SendDirectMessage(result.Write());
        return;
    }

    if (favorite)
        follower->PacketInfo.FollowerStatus |= FOLLOWER_STATUS_FAVORITE;
    else
        follower->PacketInfo.FollowerStatus &= ~FOLLOWER_STATUS_FAVORITE;

    result.Follower = follower->PacketInfo;
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

    // Troops cannot be activated/deactivated
    if (follower->PacketInfo.FollowerStatus & FOLLOWER_STATUS_TROOP)
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

    if (inactive)
    {
        follower->PacketInfo.FollowerStatus |= FOLLOWER_STATUS_INACTIVE;
    }
    else
    {
        // Check champion activation cap before activating
        GarrFollowerTypeEntry const* followerType = sGarrisonMgr.GetFollowerTypeForGarrType(static_cast<int8>(GetType()));
        if (followerType && followerType->MaxFollowers > 0)
        {
            uint32 activeCount = 0;
            for (auto const& p : _followers)
            {
                if (!(p.second.PacketInfo.FollowerStatus & FOLLOWER_STATUS_INACTIVE)
                    && !(p.second.PacketInfo.FollowerStatus & FOLLOWER_STATUS_TROOP))
                    ++activeCount;
            }

            if (activeCount >= followerType->MaxFollowers + GetBonusFollowerSlots())
            {
                result.Result = GARRISON_ERROR_FOLLOWER_SOFT_CAP_EXCEEDED;
                _owner->SendDirectMessage(result.Write());
                return;
            }
        }

        // Activating a follower costs one daily activation
        if (_followerActivationsRemainingToday == 0)
        {
            result.Result = GARRISON_ERROR_FOLLOWER_ACTIVATION_UNAVAILABLE;
            _owner->SendDirectMessage(result.Write());
            return;
        }

        follower->PacketInfo.FollowerStatus &= ~FOLLOWER_STATUS_INACTIVE;
        --_followerActivationsRemainingToday;
    }

    result.Follower = follower->PacketInfo;
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

    // Troops cannot be renamed
    if (follower->PacketInfo.FollowerStatus & FOLLOWER_STATUS_TROOP)
    {
        result.Result = GARRISON_ERROR_INVALID_FOLLOWER;
        result.Follower = follower->PacketInfo;
        _owner->SendDirectMessage(result.Write());
        return;
    }

    follower->PacketInfo.CustomName = name;
    result.Follower = follower->PacketInfo;
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

            // Populate auto-combat data from combatant linked to this encounter
            if (GarrAutoCombatantEntry const* combatant = sGarrisonMgr.GetAutoCombatantForEncounter(encounterEntry->ID))
            {
                encounter.GarrAutoCombatantID = combatant->ID;
                encounter.Health = combatant->Health;
                encounter.MaxHealth = combatant->MaxHealth;
                encounter.Attack = combatant->Attack;
                encounter.BoardIndex = static_cast<int8>(combatant->BoardIndex);
            }

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

        // The follower must match the mission's follower type: garrison followers crew garrison missions,
        // ships (GarrFollowerType 2) crew naval missions. Without this a ship could be slotted on a land
        // mission (or vice versa) - the client filters by type, but validate server-side too.
        if (GarrFollowerEntry const* followerEntry = sGarrFollowerStore.LookupEntry(follower->PacketInfo.GarrFollowerID))
            if (followerEntry->GarrFollowerTypeID != missionEntry->GarrFollowerTypeID)
                return GARRISON_ERROR_INVALID_FOLLOWER;
    }

    // Check required followers (GarrMissionXFollower.db2)
    if (std::vector<GarrMissionXFollowerEntry const*> const* requiredFollowers = sGarrisonMgr.GetMissionRequiredFollowers(missionRecID))
    {
        for (GarrMissionXFollowerEntry const* required : *requiredFollowers)
        {
            bool found = false;
            for (uint64 followerDbId : followerDBIDs)
            {
                if (Follower const* follower = GetFollower(followerDbId))
                {
                    if (static_cast<int32>(follower->PacketInfo.GarrFollowerID) == required->GarrFollowerID)
                    {
                        found = true;
                        break;
                    }
                }
            }
            if (!found)
                return GARRISON_ERROR_MISSION_MISSING_REQUIRED_FOLLOWER;
        }
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

    // Track missions started today (daily reset based on day boundary)
    uint32 today = static_cast<uint32>(GameTime::GetGameTime() / DAY);
    if (today != _lastMissionStartDay)
    {
        _missionsStartedToday = 0;
        _lastMissionStartDay = today;
    }
    ++_missionsStartedToday;

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

    // Determine the outcome exactly once, here, and remember it. Both the result shown to the player
    // (SMSG_GARRISON_COMPLETE_MISSION_RESULT) and the later reward grant (FinalizeMission) read this
    // stored value, so a "success" banner can never diverge from a "no loot" grant.
    if (!mission->ResultDetermined)
    {
        mission->Succeeded = RollMissionOutcome(*mission, missionRecID);
        mission->ResultDetermined = true;
    }

    mission->PacketInfo.MissionState = 2; // Completed
    return GARRISON_SUCCESS;
}

// Rolls a mission's success outcome: auto-combat simulation for adventure missions, otherwise a
// straight roll against the pre-computed SuccessChance. Pure computation — no state change, no grants.
bool Garrison::RollMissionOutcome(Mission const& mission, uint32 missionRecID) const
{
    bool isAutoCombatMission = false;
    for (auto const& encounter : mission.PacketInfo.Encounters)
    {
        if (encounter.GarrAutoCombatantID != 0)
        {
            isAutoCombatMission = true;
            break;
        }
    }

    if (isAutoCombatMission)
    {
        std::vector<AutoCombatCombatant> playerUnits;
        int8 boardIdx = 0;
        for (uint64 followerDbId : mission.CurrentFollowerDBIDs)
        {
            if (Follower const* follower = GetFollower(followerDbId))
            {
                AutoCombatCombatant unit = GarrisonAutoCombat::BuildFollowerCombatant(
                    follower->PacketInfo.FollowerLevel, follower->PacketInfo.Quality,
                    follower->PacketInfo.ItemLevelWeapon, follower->PacketInfo.ItemLevelArmor,
                    follower->PacketInfo.BoardIndex >= 0 ? follower->PacketInfo.BoardIndex : boardIdx,
                    followerDbId);
                playerUnits.push_back(std::move(unit));
                ++boardIdx;
            }
        }

        std::vector<AutoCombatCombatant> enemyUnits;
        for (auto const& encounter : mission.PacketInfo.Encounters)
        {
            if (encounter.GarrAutoCombatantID == 0)
                continue;

            GarrAutoCombatantEntry const* combatant = sGarrisonMgr.GetAutoCombatant(encounter.GarrAutoCombatantID);
            if (!combatant)
                continue;

            enemyUnits.push_back(GarrisonAutoCombat::BuildEnemyCombatant(combatant));
        }

        AutoCombatResult combatResult = GarrisonAutoCombat::SimulateCombat(playerUnits, enemyUnits);
        TC_LOG_DEBUG("garrison", "Auto-combat for mission {}: {} in {} rounds",
            missionRecID, combatResult.PlayerWon ? "WON" : "LOST", combatResult.TotalRounds);
        return combatResult.PlayerWon;
    }

    return static_cast<int32>(urand(0, 99)) < mission.PacketInfo.SuccessChance;
}

// Grants rewards + follower XP, frees the followers and removes the mission. Called from the opcodes the
// WoD client actually sends (BONUS_ROLL on success, COMPLETE on failure) — NOT from the never-sent
// GET_MISSION_REWARD. Uses the outcome stored at CompleteMission; re-rolls once only if the mission was
// caught mid-completion by a restart (ResultDetermined lost with the runtime state).
GarrisonError Garrison::FinalizeMission(uint32 missionRecID, bool grantOvermax)
{
    Mission* mission = GetMissionByRecID(missionRecID);
    if (!mission)
        return GARRISON_ERROR_INVALID_MISSION;

    if (mission->PacketInfo.MissionState != 2 && mission->PacketInfo.MissionState != 3)
        return GARRISON_ERROR_MISSION_NOT_COMPLETE;

    GarrMissionEntry const* missionEntry = sGarrMissionStore.LookupEntry(missionRecID);
    if (!missionEntry)
        return GARRISON_ERROR_INVALID_MISSION;

    if (!mission->ResultDetermined)
    {
        mission->Succeeded = RollMissionOutcome(*mission, missionRecID);
        mission->ResultDetermined = true;
    }
    bool succeeded = mission->Succeeded;

    // Award follower XP (awarded regardless of success) and handle troop durability
    std::vector<uint64> troopsToRemove;
    uint32 followerXP = missionEntry->BaseFollowerXP;
    for (uint64 followerDbId : mission->CurrentFollowerDBIDs)
    {
        if (Follower* follower = GetFollower(followerDbId))
        {
            follower->PacketInfo.CurrentMissionID = 0;

            // Troops lose 1 durability per mission
            if (follower->PacketInfo.FollowerStatus & FOLLOWER_STATUS_TROOP)
            {
                if (follower->PacketInfo.Durability > 0)
                    --follower->PacketInfo.Durability;

                if (follower->PacketInfo.Durability == 0)
                    troopsToRemove.push_back(followerDbId);
            }

            if (followerXP > 0
                && !(follower->PacketInfo.FollowerStatus & FOLLOWER_STATUS_NO_XP_GAIN)
                && !(follower->PacketInfo.FollowerStatus & FOLLOWER_STATUS_TROOP))
            {
                // Capture old state before XP modification
                WorldPackets::Garrison::GarrisonFollower oldFollowerState = follower->PacketInfo;

                GarrFollowerEntry const* followerEntry = sGarrFollowerStore.LookupEntry(follower->PacketInfo.GarrFollowerID);
                uint8 followerTypeID = followerEntry ? followerEntry->GarrFollowerTypeID : uint8(FOLLOWER_TYPE_GARRISON);

                follower->PacketInfo.Xp += followerXP;

                // Level progression using GarrFollowerLevelXP DB2
                GarrFollowerLevelXPEntry const* levelXP = sGarrisonMgr.GetFollowerLevelXP(followerTypeID, follower->PacketInfo.FollowerLevel);
                while (levelXP && levelXP->XpToNextLevel > 0 && follower->PacketInfo.Xp >= levelXP->XpToNextLevel)
                {
                    follower->PacketInfo.Xp -= levelXP->XpToNextLevel;
                    follower->PacketInfo.FollowerLevel++;
                    levelXP = sGarrisonMgr.GetFollowerLevelXP(followerTypeID, follower->PacketInfo.FollowerLevel);
                }

                // Only a follower at its TRUE terminal level rolls excess XP into quality (iLvl). The DB2
                // marks that level with XpToNextLevel == 0. A NULL levelXP means we simply have no row for
                // this (type, level) — treat that as "can't level right now" and KEEP the accumulated XP;
                // it must never be silently deleted (that zeroed every mission's follower XP when the
                // GarrFollowerLevelXP row for the follower's level was absent).
                if (levelXP && levelXP->XpToNextLevel == 0)
                {
                    GarrFollowerQualityEntry const* qualityEntry = sGarrisonMgr.GetFollowerQuality(followerTypeID, follower->PacketInfo.Quality);
                    while (qualityEntry && qualityEntry->XpThreshold > 0 && follower->PacketInfo.Xp >= static_cast<uint32>(qualityEntry->XpThreshold))
                    {
                        follower->PacketInfo.Xp -= qualityEntry->XpThreshold;
                        follower->PacketInfo.Quality++;

                        // Re-roll abilities for new quality tier
                        if (followerEntry)
                        {
                            follower->PacketInfo.AbilityID = sGarrisonMgr.RollFollowerAbilities(
                                follower->PacketInfo.GarrFollowerID, followerEntry,
                                follower->PacketInfo.Quality, GetFaction(), false);
                        }

                        qualityEntry = sGarrisonMgr.GetFollowerQuality(followerTypeID, follower->PacketInfo.Quality);
                    }

                    // Fully maxed (top level AND top quality): no bar left to fill.
                    if (!qualityEntry || qualityEntry->XpThreshold == 0)
                        follower->PacketInfo.Xp = 0;
                }
                else if (!levelXP)
                {
                    TC_LOG_DEBUG("garrison", "No GarrFollowerLevelXP row for type={} level={}; follower {} keeps Xp={} without levelling (DB2 data gap)",
                        followerTypeID, int32(follower->PacketInfo.FollowerLevel), follower->PacketInfo.DbID, follower->PacketInfo.Xp);
                }

                // Send follower XP update with old and new state
                WorldPackets::Garrison::GarrisonFollowerChangedXP followerXPUpdate;
                followerXPUpdate.Result = GARRISON_SUCCESS;
                followerXPUpdate.TotalXp = followerXP;
                followerXPUpdate.OldFollower = oldFollowerState;
                followerXPUpdate.Follower = follower->PacketInfo;
                _owner->SendDirectMessage(followerXPUpdate.Write());
            }
        }
    }

    // Remove troops that have exhausted their durability
    for (uint64 troopDbId : troopsToRemove)
        RemoveFollower(troopDbId);

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
                    // Mail overflow items (SendMailTo needs a real transaction - a null one crashes)
                    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
                    MailDraft draft("Garrison Mission Reward", "A reward from a completed garrison mission.");
                    if (Item* item = Item::CreateItem(reward.ItemID, reward.ItemQuantity, ItemContext::NONE, _owner))
                    {
                        item->SaveToDB(trans);
                        draft.AddItem(item);
                    }
                    draft.SendMailTo(trans, MailReceiver(_owner), MailSender(MAIL_CREATURE, 0));
                    CharacterDatabase.CommitTransaction(trans);
                }
            }
            if (reward.CurrencyID > 0 && reward.CurrencyQuantity > 0)
                _owner->AddCurrency(reward.CurrencyID, reward.CurrencyQuantity, CurrencyGainSource::GarrisonMissionReward);
        }

        // Award overmax (bonus-roll chest) rewards only when finalizing via BONUS_ROLL.
        if (grantOvermax)
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
                        CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
                        MailDraft draft("Garrison Mission Bonus", "A bonus reward from a garrison mission.");
                        if (Item* item = Item::CreateItem(reward.ItemID, reward.ItemQuantity, ItemContext::NONE, _owner))
                        {
                            item->SaveToDB(trans);
                            draft.AddItem(item);
                        }
                        draft.SendMailTo(trans, MailReceiver(_owner), MailSender(MAIL_CREATURE, 0));
                        CharacterDatabase.CommitTransaction(trans);
                    }
                }
                if (reward.CurrencyID > 0 && reward.CurrencyQuantity > 0)
                    _owner->AddCurrency(reward.CurrencyID, reward.CurrencyQuantity, CurrencyGainSource::GarrisonMissionReward);
            }
        }
    }

    // Salvage Yard is a WoD-only building (BuildingType 35) that has no equivalent in
    // Class Order Halls / War Campaign / Covenant. Guard the WoD-specific reward path here
    // — the equivalent expansion-specific bonus reward mechanics live in their own scripts.
    if (GetType() == GARRISON_TYPE_GARRISON)
    {
        static constexpr uint32 BUILDING_TYPE_SALVAGE_YARD = 35;
        static constexpr uint32 ITEM_SALVAGE_CRATE_SMALL = 114116;  // Bag of Salvaged Goods (level 1)
        static constexpr uint32 ITEM_SALVAGE_CRATE_MEDIUM = 114119; // Crate of Salvage (level 2)
        static constexpr uint32 ITEM_SALVAGE_CRATE_LARGE = 114120;  // Big Crate of Salvage (level 3)

        GarrBuildingEntry const* salvageYard = GetActiveBuildingByType(BUILDING_TYPE_SALVAGE_YARD);
        if (salvageYard)
        {
            uint32 salvageItemId = 0;
            if (salvageYard->UpgradeLevel >= 3)
                salvageItemId = ITEM_SALVAGE_CRATE_LARGE;
            else if (salvageYard->UpgradeLevel >= 2)
                salvageItemId = ITEM_SALVAGE_CRATE_MEDIUM;
            else
                salvageItemId = ITEM_SALVAGE_CRATE_SMALL;

            if (salvageItemId)
            {
                ItemPosCountVec dest;
                if (_owner->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, salvageItemId, 1) == EQUIP_ERR_OK)
                {
                    if (Item* item = _owner->StoreNewItem(dest, salvageItemId, true))
                        _owner->SendNewItem(item, 1, true, false);
                }
                else
                {
                    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
                    MailDraft draft("Salvage", "Salvage from a garrison mission.");
                    if (Item* item = Item::CreateItem(salvageItemId, 1, ItemContext::NONE, _owner))
                    {
                        item->SaveToDB(trans);
                        draft.AddItem(item);
                    }
                    draft.SendMailTo(trans, MailReceiver(_owner), MailSender(MAIL_CREATURE, 0));
                    CharacterDatabase.CommitTransaction(trans);
                }
            }
        }
    }

    // Archive the completed mission
    _archivedMissions.push_back(static_cast<int32>(missionRecID));

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
    // The WoD client sends CMSG_GARRISON_MISSION_BONUS_ROLL when the player opens the reward chest on a
    // successful mission — the client itself comments this call "-- complete mission". It is the finalize
    // step: grant base + overmax (chest) rewards + follower XP, free the followers and remove the mission.
    // (The reward code used to live only in ClaimMissionReward / GET_MISSION_REWARD, an opcode the WoD
    // client never sends, so completed missions granted nothing and lingered forever.)
    Mission* mission = GetMissionByRecID(missionRecID);
    if (!mission)
        return GARRISON_ERROR_INVALID_MISSION;

    if (mission->PacketInfo.MissionState != 2 && mission->PacketInfo.MissionState != 3)
        return GARRISON_ERROR_MISSION_NOT_COMPLETE;

    return FinalizeMission(missionRecID, true); // include overmax (chest) rewards
}

// Legacy CMSG_GARRISON_GET_MISSION_REWARD path. The WoD command table never sends this opcode (it uses
// COMPLETE + BONUS_ROLL), but keep it wired as a direct finalize for any Legion+/order-hall flow that does.
GarrisonError Garrison::ClaimMissionReward(uint32 missionRecID)
{
    return FinalizeMission(missionRecID, true);
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

            // Notify the client that the mission was removed
            WorldPackets::Garrison::GarrisonDeleteMissionResult deleteMissionResult;
            deleteMissionResult.Result = GARRISON_SUCCESS;
            deleteMissionResult.MissionRecID = missionRecID;
            deleteMissionResult.GarrTypeID = GetType();
            _owner->SendDirectMessage(deleteMissionResult.Write());
            break;
        }
    }
}

bool Garrison::IsAutoCombatMission(Mission const& mission) const
{
    for (auto const& encounter : mission.PacketInfo.Encounters)
        if (encounter.GarrAutoCombatantID != 0)
            return true;
    return false;
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

    // Use the garrison's own type for mission lookups
    int8 garrTypeID = static_cast<int8>(GetType());

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

    // Trickle new missions a few per pass rather than dumping the whole board at once: a large
    // ADD_MISSION_RESULT burst floods GARRISON_MISSION_LIST_UPDATE (and previously broke the command
    // table's open-event). The periodic caller tops the board back up to the cap over several ticks.
    static constexpr uint32 MAX_MISSIONS_PER_GENERATION = 4;
    missionsToGenerate = std::min(missionsToGenerate, MAX_MISSIONS_PER_GENERATION);

    // Average follower level, tracked PER follower type. A garrison offers land missions scaled to its
    // garrison followers and naval missions scaled to its ships independently - using one combined average
    // (e.g. level-~100 garrison followers) would filter naval missions against the wrong roster and could
    // empty the naval board once the player has land followers. Keyed by GarrFollowerTypeID.
    std::unordered_map<int8, std::pair<int64 /*sumLevel*/, uint32 /*count*/>> levelByType;
    for (auto const& p : _followers)
    {
        if (p.second.PacketInfo.FollowerStatus & FOLLOWER_STATUS_INACTIVE)
            continue;
        int8 type = static_cast<int8>(FOLLOWER_TYPE_GARRISON);
        if (GarrFollowerEntry const* fe = sGarrFollowerStore.LookupEntry(p.second.PacketInfo.GarrFollowerID))
            type = fe->GarrFollowerTypeID;
        auto& acc = levelByType[type];
        acc.first += p.second.PacketInfo.FollowerLevel;
        ++acc.second;
    }

    // Average level of the followers that can actually crew a mission of the given type, or -1 if the player
    // has none of that type yet (in which case the level filter is skipped and the whole pool is offered).
    auto avgLevelForType = [&levelByType](int8 followerTypeId) -> int32
    {
        auto it = levelByType.find(followerTypeId);
        if (it == levelByType.end() || it->second.second == 0)
            return -1;
        return static_cast<int32>(it->second.first / it->second.second);
    };

    // Build eligible mission pool
    std::vector<GarrMissionEntry const*> eligibleMissions;
    for (GarrMissionEntry const* mission : *availableMissions)
    {
        // Skip missions already active
        if (_activeMissionRecIDs.count(mission->ID))
            continue;

        // Filter by follower type: the garrison's primary type, plus naval (shipyard) missions once the
        // shipyard is built. Both share GarrTypeID 2, so the shipyard gate is what keeps naval missions off
        // the board until the player has a shipyard.
        if (!IsMissionFollowerTypeAvailable(mission->GarrFollowerTypeID))
            continue;

        // Filter by target level, but ONLY when we actually have active followers OF THIS MISSION'S TYPE to
        // scale against. Retail offers the standard mission pool to a garrison with no active followers (sniff
        // "garrison and hall of class table quest.pkt": 42 missions offered), so a type with no roster yet must
        // not be starved to zero - a just-built shipyard with no ships still offers the full naval pool.
        if (int32 avgLevel = avgLevelForType(mission->GarrFollowerTypeID); avgLevel >= 0)
        {
            if (std::abs(avgLevel - static_cast<int32>(mission->TargetLevel)) > 5)
                continue;
        }

        // NOTE: intentionally NOT gating the OFFER on current idle-follower count.
        // Retail offers missions the player cannot yet staff (the above sniff offers all 42
        // regardless of roster); the follower requirement is enforced at mission START
        // (StartMission validates MaxFollowers), not at offer time. A previous
        // "MaxFollowers > availableFollowers" gate here zeroed the command table whenever the
        // player's followers were all busy or inactive.

        // Skip missions with 0 duration (usually internal/debug)
        if (mission->MissionDuration == 0)
            continue;

        eligibleMissions.push_back(mission);
    }

    // Inn/Tavern level 2+ unlocks WoD treasure missions. Inn (BuildingType 34) is WoD-only;
    // Class Order Halls / War Campaign / Covenants have their own bonus-mission generators
    // outside this code path.
    if (GetType() == GARRISON_TYPE_GARRISON)
    {
        static constexpr uint32 BUILDING_TYPE_INN = 34;
        GarrBuildingEntry const* inn = GetActiveBuildingByType(BUILDING_TYPE_INN);
        if (inn && inn->UpgradeLevel >= 2)
        {
            // Re-scan pool for treasure-flagged missions (Flags & 0x10 = treasure mission)
            // that were excluded by level range, and add some with relaxed constraints
            for (GarrMissionEntry const* mission : *availableMissions)
            {
                if (_activeMissionRecIDs.count(mission->ID))
                    continue;
                if (mission->GarrFollowerTypeID != sGarrisonMgr.GetPrimaryFollowerType(garrTypeID))
                    continue;
                if (mission->MissionDuration == 0)
                    continue;
                // Treasure missions have flag 0x10
                if (!(mission->Flags & 0x10))
                    continue;
                // Don't duplicate
                bool alreadyEligible = false;
                for (GarrMissionEntry const* existing : eligibleMissions)
                {
                    if (existing->ID == mission->ID)
                    {
                        alreadyEligible = true;
                        break;
                    }
                }
                if (!alreadyEligible)
                    eligibleMissions.push_back(mission);
            }
        }
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

void Garrison::SetRecruitmentPreferences(uint32 abilityId, uint32 traitId)
{
    // Validate ability if specified
    if (abilityId)
    {
        GarrAbilityEntry const* ability = sGarrAbilityStore.LookupEntry(abilityId);
        if (!ability || (ability->Flags & GARRISON_ABILITY_FLAG_TRAIT))
        {
            TC_LOG_DEBUG("garrison", "Garrison::SetRecruitmentPreferences: Invalid ability {} (must be counter, not trait)", abilityId);
            return;
        }
    }

    // Validate trait if specified
    if (traitId)
    {
        GarrAbilityEntry const* trait = sGarrAbilityStore.LookupEntry(traitId);
        if (!trait || !(trait->Flags & GARRISON_ABILITY_FLAG_TRAIT))
        {
            TC_LOG_DEBUG("garrison", "Garrison::SetRecruitmentPreferences: Invalid trait {} (must have TRAIT flag)", traitId);
            return;
        }
    }

    _recruitmentPreferenceAbilityId = abilityId;
    _recruitmentPreferenceTraitId = traitId;

    TC_LOG_DEBUG("garrison", "Garrison::SetRecruitmentPreferences: Player {} set ability={}, trait={}",
        _owner->GetGUID().ToString().c_str(), abilityId, traitId);
}

void Garrison::GenerateRecruits(uint32 faction)
{
    _availableRecruits.clear();

    // Find all followers of the garrison type that the player doesn't already have
    std::vector<GarrFollowerEntry const*> eligibleFollowers;
    for (GarrFollowerEntry const* follower : sGarrFollowerStore)
    {
        if (follower->GarrFollowerTypeID != sGarrisonMgr.GetPrimaryFollowerType(static_cast<int8>(GetType())))
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

    auto buildRecruit = [&](GarrFollowerEntry const* followerEntry) -> WorldPackets::Garrison::GarrisonFollower
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

        // Roll abilities based on quality
        std::list<GarrAbilityEntry const*> abilities = sGarrisonMgr.RollFollowerAbilities(
            followerEntry->ID, followerEntry, recruit.Quality, faction, true);

        for (GarrAbilityEntry const* ability : abilities)
            recruit.AbilityID.push_back(ability);

        return recruit;
    };

    for (GarrFollowerEntry const* followerEntry : eligibleFollowers)
        _availableRecruits.push_back(buildRecruit(followerEntry));

    // Apply recruitment preferences: guarantee the first recruit has the preferred ability/trait
    // This mimics the Blizzlike behavior where the Inn/Tavern NPC allows the player to choose
    // a specific counter or trait and one of the 3 recruits is guaranteed to have it
    if ((_recruitmentPreferenceAbilityId || _recruitmentPreferenceTraitId) && !_availableRecruits.empty())
    {
        WorldPackets::Garrison::GarrisonFollower& preferredRecruit = _availableRecruits[0];

        // Re-roll abilities for the first recruit, forcing the preferred ones
        preferredRecruit.AbilityID.clear();
        GarrFollowerEntry const* followerEntry = sGarrFollowerStore.LookupEntry(preferredRecruit.GarrFollowerID);
        if (followerEntry)
        {
            std::list<GarrAbilityEntry const*> abilities = sGarrisonMgr.RollFollowerAbilities(
                followerEntry->ID, followerEntry, preferredRecruit.Quality, faction, true);

            // Check if preferred ability is already in the rolled set; if not, replace the first non-forced counter
            if (_recruitmentPreferenceAbilityId)
            {
                GarrAbilityEntry const* preferredAbility = sGarrAbilityStore.LookupEntry(_recruitmentPreferenceAbilityId);
                if (preferredAbility)
                {
                    bool found = false;
                    for (GarrAbilityEntry const* a : abilities)
                    {
                        if (a->ID == _recruitmentPreferenceAbilityId)
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        // Replace first counter (non-trait, non-forced) with the preferred one
                        for (auto it = abilities.begin(); it != abilities.end(); ++it)
                        {
                            if (!((*it)->Flags & GARRISON_ABILITY_FLAG_TRAIT) &&
                                !((*it)->Flags & GARRISON_ABILITY_FLAG_CANNOT_REMOVE))
                            {
                                *it = preferredAbility;
                                break;
                            }
                        }
                    }
                }
            }

            // Check if preferred trait is already in the rolled set; if not, replace the first non-forced trait
            if (_recruitmentPreferenceTraitId)
            {
                GarrAbilityEntry const* preferredTrait = sGarrAbilityStore.LookupEntry(_recruitmentPreferenceTraitId);
                if (preferredTrait)
                {
                    bool found = false;
                    for (GarrAbilityEntry const* a : abilities)
                    {
                        if (a->ID == _recruitmentPreferenceTraitId)
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        // Replace first trait (non-forced) with the preferred one
                        for (auto it = abilities.begin(); it != abilities.end(); ++it)
                        {
                            if (((*it)->Flags & GARRISON_ABILITY_FLAG_TRAIT) &&
                                !((*it)->Flags & GARRISON_ABILITY_FLAG_CANNOT_REMOVE))
                            {
                                *it = preferredTrait;
                                break;
                            }
                        }
                    }
                }
            }

            for (GarrAbilityEntry const* ability : abilities)
                preferredRecruit.AbilityID.push_back(ability);
        }
    }

    // Clear preferences after generating recruits (one-time use per recruitment cycle)
    _recruitmentPreferenceAbilityId = 0;
    _recruitmentPreferenceTraitId = 0;
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

uint32 Garrison::GetShipCount() const
{
    uint32 count = 0;
    for (auto const& p : _followers)
        if (GarrFollowerEntry const* entry = sGarrFollowerStore.LookupEntry(p.second.PacketInfo.GarrFollowerID))
            if (entry->GarrFollowerTypeID == static_cast<int8>(FOLLOWER_TYPE_SHIPYARD))
                ++count;
    return count;
}

// Build a ship at the shipyard. A "ship" is a GarrFollowerType-2 GarrFollower (102 exist in 12.0.7, ids 469+);
// building one adds it as a follower via the normal AddFollower path. Retail builds ships over time from the naval
// command table; the client-facing build request (CMSG) + build timer are sniff-gated (see [[shipyard_foundation_68275]]),
// so this method is the validated server entry point that flow will call once its wire is known.
GarrisonError Garrison::BuildShip(uint32 garrFollowerId)
{
    if (!HasShipyard())
        return GARRISON_ERROR_NO_BUILDING;

    GarrFollowerEntry const* shipEntry = sGarrFollowerStore.LookupEntry(garrFollowerId);
    if (!shipEntry || shipEntry->GarrFollowerTypeID != static_cast<int8>(FOLLOWER_TYPE_SHIPYARD)
        || shipEntry->GarrTypeID != static_cast<int8>(GetType()))
        return GARRISON_ERROR_INVALID_FOLLOWER;

    if (_followerIds.count(garrFollowerId))
        return GARRISON_ERROR_FOLLOWER_EXISTS;

    if (GetShipCount() >= SHIPYARD_FOLLOWER_SOFT_CAP)
        return GARRISON_ERROR_INVALID_FOLLOWER;

    AddFollower(garrFollowerId);
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

void Garrison::SendAllFollowerUpdates()
{
    for (auto const& p : _followers)
    {
        WorldPackets::Garrison::GarrisonUpdateFollower updateFollower;
        updateFollower.Result = GARRISON_SUCCESS;
        updateFollower.Follower = p.second.PacketInfo;
        _owner->SendDirectMessage(updateFollower.Write());
    }
}

void Garrison::FinishMission(uint32 garrMissionRecID)
{
    Mission* mission = GetMissionByRecID(garrMissionRecID);
    if (!mission)
        return;

    // Only finish missions that are in progress
    if (mission->PacketInfo.MissionState != 1)
        return;

    mission->PacketInfo.MissionState = 2; // Completed
    mission->PacketInfo.SuccessChance = 100; // Instant complete = guaranteed success
}

void Garrison::FinishShipment(uint32 plotInstanceId)
{
    // Find the oldest incomplete shipment for this plot and complete it
    Shipment* oldest = nullptr;
    for (auto& [dbId, shipment] : _shipments)
    {
        if (shipment.PlotInstanceID == plotInstanceId && !shipment.IsReady())
        {
            if (!oldest || shipment.CreationTime < oldest->CreationTime)
                oldest = &shipment;
        }
    }

    if (oldest)
        oldest->CreationTime = time_t(0); // Set creation time to epoch so IsReady() returns true
}

void Garrison::SetFollowerQuality(uint64 dbId, uint32 quality)
{
    Follower* follower = GetFollower(dbId);
    if (!follower)
        return;

    // Capture old state before quality modification
    WorldPackets::Garrison::GarrisonFollower oldFollowerState = follower->PacketInfo;

    follower->PacketInfo.Quality = quality;

    // Re-roll abilities for the new quality tier
    GarrFollowerEntry const* followerEntry = sGarrFollowerStore.LookupEntry(follower->PacketInfo.GarrFollowerID);
    if (followerEntry)
    {
        follower->PacketInfo.AbilityID = sGarrisonMgr.RollFollowerAbilities(
            follower->PacketInfo.GarrFollowerID, followerEntry,
            quality, GetFaction(), false);
    }

    WorldPackets::Garrison::GarrisonFollowerChangedQuality changedQuality;
    changedQuality.OldFollower = oldFollowerState;
    changedQuality.Follower = follower->PacketInfo;
    _owner->SendDirectMessage(changedQuality.Write());
}

void Garrison::SetFollowerLevel(uint64 dbId, uint32 level)
{
    Follower* follower = GetFollower(dbId);
    if (!follower)
        return;

    follower->PacketInfo.FollowerLevel = level;
    follower->PacketInfo.Xp = 0;

    WorldPackets::Garrison::GarrisonUpdateFollower updateFollower;
    updateFollower.Result = GARRISON_SUCCESS;
    updateFollower.Follower = follower->PacketInfo;
    _owner->SendDirectMessage(updateFollower.Write());
}

void Garrison::AddFollowerXP(uint64 dbId, uint32 xp)
{
    Follower* follower = GetFollower(dbId);
    if (!follower)
        return;

    if (follower->PacketInfo.FollowerStatus & FOLLOWER_STATUS_NO_XP_GAIN)
        return;

    if (follower->PacketInfo.FollowerStatus & FOLLOWER_STATUS_TROOP)
        return;

    // Capture old state before XP modification
    WorldPackets::Garrison::GarrisonFollower oldFollowerState = follower->PacketInfo;

    GarrFollowerEntry const* followerEntry = sGarrFollowerStore.LookupEntry(follower->PacketInfo.GarrFollowerID);
    uint8 followerTypeID = followerEntry ? followerEntry->GarrFollowerTypeID : uint8(FOLLOWER_TYPE_GARRISON);

    follower->PacketInfo.Xp += xp;

    // Level progression using GarrFollowerLevelXP DB2
    GarrFollowerLevelXPEntry const* levelXP = sGarrisonMgr.GetFollowerLevelXP(followerTypeID, follower->PacketInfo.FollowerLevel);
    while (levelXP && levelXP->XpToNextLevel > 0 && follower->PacketInfo.Xp >= levelXP->XpToNextLevel)
    {
        follower->PacketInfo.Xp -= levelXP->XpToNextLevel;
        follower->PacketInfo.FollowerLevel++;
        levelXP = sGarrisonMgr.GetFollowerLevelXP(followerTypeID, follower->PacketInfo.FollowerLevel);
    }

    // Only a follower at its TRUE terminal level (DB2 row present with XpToNextLevel == 0) rolls excess XP
    // into quality. A NULL levelXP just means we have no row for this (type, level) — keep the accumulated
    // XP rather than deleting it. (Mirrors the mission-reward path; see FinalizeMission.)
    if (levelXP && levelXP->XpToNextLevel == 0)
    {
        GarrFollowerQualityEntry const* qualityEntry = sGarrisonMgr.GetFollowerQuality(followerTypeID, follower->PacketInfo.Quality);
        while (qualityEntry && qualityEntry->XpThreshold > 0 && follower->PacketInfo.Xp >= static_cast<uint32>(qualityEntry->XpThreshold))
        {
            follower->PacketInfo.Xp -= qualityEntry->XpThreshold;
            follower->PacketInfo.Quality++;

            if (followerEntry)
            {
                follower->PacketInfo.AbilityID = sGarrisonMgr.RollFollowerAbilities(
                    follower->PacketInfo.GarrFollowerID, followerEntry,
                    follower->PacketInfo.Quality, GetFaction(), false);
            }

            qualityEntry = sGarrisonMgr.GetFollowerQuality(followerTypeID, follower->PacketInfo.Quality);
        }

        if (!qualityEntry || qualityEntry->XpThreshold == 0)
            follower->PacketInfo.Xp = 0;
    }

    WorldPackets::Garrison::GarrisonFollowerChangedXP followerXPUpdate;
    followerXPUpdate.Result = GARRISON_SUCCESS;
    followerXPUpdate.TotalXp = xp;
    followerXPUpdate.OldFollower = oldFollowerState;
    followerXPUpdate.Follower = follower->PacketInfo;
    _owner->SendDirectMessage(followerXPUpdate.Write());
}

void Garrison::LearnFollowerAbility(uint64 dbId, uint32 abilityId)
{
    Follower* follower = GetFollower(dbId);
    if (!follower)
        return;

    GarrAbilityEntry const* ability = sGarrAbilityStore.LookupEntry(abilityId);
    if (!ability)
        return;

    // Check if follower already has this ability
    if (follower->HasAbility(abilityId))
        return;

    follower->PacketInfo.AbilityID.push_back(ability);

    WorldPackets::Garrison::GarrisonUpdateFollower updateFollower;
    updateFollower.Result = GARRISON_SUCCESS;
    updateFollower.Follower = follower->PacketInfo;
    _owner->SendDirectMessage(updateFollower.Write());
}

void Garrison::RandomizeFollowerAbilities(uint64 dbId)
{
    Follower* follower = GetFollower(dbId);
    if (!follower)
        return;

    GarrFollowerEntry const* followerEntry = sGarrFollowerStore.LookupEntry(follower->PacketInfo.GarrFollowerID);
    if (!followerEntry)
        return;

    follower->PacketInfo.AbilityID = sGarrisonMgr.RollFollowerAbilities(
        follower->PacketInfo.GarrFollowerID, followerEntry,
        follower->PacketInfo.Quality, GetFaction(), false);

    WorldPackets::Garrison::GarrisonUpdateFollower updateFollower;
    updateFollower.Result = GARRISON_SUCCESS;
    updateFollower.Follower = follower->PacketInfo;
    _owner->SendDirectMessage(updateFollower.Write());
}

void Garrison::EndBuildingConstruction(uint32 garrPlotInstanceId)
{
    Plot* plot = GetPlot(garrPlotInstanceId);
    if (!plot || !plot->BuildingInfo.PacketInfo)
        return;

    if (plot->BuildingInfo.PacketInfo->Active)
        return;

    // Set time built to the past so CanActivate() returns true
    plot->BuildingInfo.PacketInfo->TimeBuilt = GameTime::GetGameTime() - DAY;
    ActivateBuilding(garrPlotInstanceId);
}

void Garrison::SetGarrisonCacheSize(uint32 size)
{
    _garrisonCacheSize = size;
}

uint32 Garrison::GetPendingCacheResources() const
{
    if (!_cacheLastUsed)
        return 0;

    time_t now = GameTime::GetGameTime();
    if (now <= _cacheLastUsed)
        return 0;

    uint32 accrued = static_cast<uint32>((now - _cacheLastUsed) / CACHE_RESOURCE_INTERVAL);
    return std::min(accrued, _garrisonCacheSize);
}

uint32 Garrison::CollectGarrisonCache()
{
    uint32 amount = GetPendingCacheResources();
    if (!amount)
        return 0;

    // Advance the timer by the whole intervals we are paying out, so the sub-interval remainder keeps
    // accruing toward the next resource instead of being discarded.
    _cacheLastUsed += time_t(amount) * CACHE_RESOURCE_INTERVAL;

    _owner->AddCurrency(CURRENCY_GARRISON_RESOURCES, amount, CurrencyGainSource::GarrisonResourceOverTime);
    return amount;
}

Garrison::Follower* Garrison::GetFollowerByGarrFollowerID(uint32 garrFollowerID)
{
    for (auto& [dbId, follower] : _followers)
        if (follower.PacketInfo.GarrFollowerID == garrFollowerID)
            return &follower;
    return nullptr;
}

GarrisonError Garrison::UpgradeFollowerItemLevel(uint64 dbId, int32 amount, int32 slot, GarrItemLevelUpgradeDataEntry const* upgradeData)
{
    Follower* follower = GetFollower(dbId);
    if (!follower)
        return GARRISON_ERROR_INVALID_FOLLOWER;

    GarrFollowerEntry const* followerEntry = sGarrFollowerStore.LookupEntry(follower->PacketInfo.GarrFollowerID);
    if (!followerEntry)
        return GARRISON_ERROR_INVALID_FOLLOWER;

    // Capture old state before item level modification
    WorldPackets::Garrison::GarrisonFollower oldFollowerState = follower->PacketInfo;

    // Get item level cap from GarrFollowerType
    uint16 maxItemLevel = 0;
    GarrFollowerTypeEntry const* followerType = sGarrisonMgr.GetFollowerTypeForGarrType(followerEntry->GarrTypeID);
    if (followerType)
        maxItemLevel = followerType->MaxItemLevel;

    int32 minILevel = 0;
    int32 maxILevel = maxItemLevel > 0 ? maxItemLevel : 675; // WoD default cap

    if (upgradeData)
    {
        minILevel = upgradeData->MinItemLevel;
        if (upgradeData->MaxItemLevel > 0)
            maxILevel = std::min<int32>(maxILevel, upgradeData->MaxItemLevel);
    }

    auto applyUpgrade = [&](uint32& currentLevel)
    {
        if (upgradeData && upgradeData->Operation == 0)
        {
            // Operation 0 = set to specific value
            currentLevel = static_cast<uint32>(std::clamp<int32>(amount, minILevel, maxILevel));
        }
        else
        {
            // Operation 1 or default = add delta
            if (static_cast<int32>(currentLevel) < minILevel)
                return; // Below minimum, item doesn't apply
            int32 newLevel = static_cast<int32>(currentLevel) + amount;
            currentLevel = static_cast<uint32>(std::clamp<int32>(newLevel, 0, maxILevel));
        }
    };

    if (slot == 0 || slot < 0 || slot > 1)
        applyUpgrade(follower->PacketInfo.ItemLevelWeapon);
    if (slot == 1 || slot < 0 || slot > 1)
        applyUpgrade(follower->PacketInfo.ItemLevelArmor);

    // Send item level change with old and new state
    WorldPackets::Garrison::GarrisonFollowerChangedItemLevel changedItemLevel;
    changedItemLevel.Result = GARRISON_SUCCESS;
    changedItemLevel.OldFollower = oldFollowerState;
    changedItemLevel.Follower = follower->PacketInfo;
    _owner->SendDirectMessage(changedItemLevel.Write());

    return GARRISON_SUCCESS;
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

    // Some buildings (e.g. the Lumber Mill and Trading Post) carry no resource cost (CurrencyTypeID/Qty = 0).
    // HasCurrency(0, 0) fails on the invalid currency type 0, which wrongly reported "insufficient resources"
    // and blocked construction. Only enforce the cost when the building actually has one (mirrors the garrison
    // upgrade handler's `Cost > 0 &&` guard).
    if (building->CurrencyTypeID && building->CurrencyQty && !_owner->HasCurrency(building->CurrencyTypeID, building->CurrencyQty))
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

// ============================================================
// Shipment sub-struct
// ============================================================

bool Garrison::Shipment::IsReady() const
{
    return CreationTime + Duration <= GameTime::GetGameTime();
}

// ============================================================
// Shipment management
// ============================================================

GarrisonError Garrison::CreateShipment(ObjectGuid npcGUID, uint32 count)
{
    uint32 plotInstanceId = FindPlotInstanceForNpc(npcGUID);
    if (!plotInstanceId)
        return GARRISON_ERROR_INVALID_PLOT_INSTANCEID;

    Plot* plot = GetPlot(plotInstanceId);
    if (!plot || !plot->BuildingInfo.PacketInfo || !plot->BuildingInfo.PacketInfo->Active)
        return GARRISON_ERROR_BUILDING_NOT_ACTIVE;

    GarrBuildingEntry const* building = sGarrBuildingStore.LookupEntry(plot->BuildingInfo.PacketInfo->GarrBuildingID);
    if (!building)
        return GARRISON_ERROR_NO_BUILDING;

    CharShipmentContainerEntry const* container = sGarrisonMgr.GetShipmentContainerForBuilding(building->BuildingType, uint8(GetFaction()));
    if (!container)
        return GARRISON_ERROR_INTERNAL_ERROR;

    std::vector<CharShipmentEntry const*> const* shipmentEntries = sGarrisonMgr.GetShipmentsForContainer(container->ID);
    if (!shipmentEntries || shipmentEntries->empty())
        return GARRISON_ERROR_INTERNAL_ERROR;

    // Pick the shipment for this container. Retail pairs each building with a fast "quest/tutorial" shipment
    // (CharShipment.Flags & 0x1, Duration 0, outputs the intro quest's item) and a regular shipment (longer
    // duration). Use the tutorial shipment only while the player still needs its reward for an active quest
    // and hasn't already queued one; otherwise the regular shipment. Fully data-driven for every profession -
    // the quest shipment's own output item completing the "Collected" objective needs no hardcoded ids.
    constexpr int32 CHAR_SHIPMENT_FLAG_QUEST = 0x1;
    CharShipmentEntry const* regularEntry = nullptr;
    CharShipmentEntry const* questEntry = nullptr;
    for (CharShipmentEntry const* s : *shipmentEntries)
    {
        if (s->Flags & CHAR_SHIPMENT_FLAG_QUEST)
        {
            if (!questEntry)
                questEntry = s;
        }
        else if (!regularEntry)
            regularEntry = s;
    }

    // Is an intro "Your First X Work Order" quest active whose ITEM objective is this container's quest
    // shipment output? (Player::HasQuestForItem only matches legacy Quest::RequiredItemId, NOT modern
    // quest_objectives - the tutorial quests use an ITEM objective - so scan the quest log directly.)
    bool tutorialActive = false;
    if (questEntry && questEntry->DummyItemID)
    {
        for (uint16 questSlot = 0; questSlot < MAX_QUEST_LOG_SIZE && !tutorialActive; ++questSlot)
        {
            uint32 questId = _owner->GetQuestSlotQuestId(questSlot);
            if (!questId || _owner->GetQuestStatus(questId) != QUEST_STATUS_INCOMPLETE)
                continue;

            Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
            if (!quest)
                continue;

            for (QuestObjective const& obj : quest->GetObjectives())
                if (obj.Type == QUEST_OBJECTIVE_ITEM && uint32(obj.ObjectID) == questEntry->DummyItemID)
                {
                    tutorialActive = true;
                    break;
                }
        }
    }

    bool questOrderQueued = false;
    if (questEntry)
        for (auto const& p : _shipments)
            if (p.second.PlotInstanceID == plotInstanceId && p.second.ShipmentRecID == questEntry->ID)
            {
                questOrderQueued = true;
                break;
            }

    // Count existing shipments for this plot
    uint32 existingCount = 0;
    for (auto const& p : _shipments)
        if (p.second.PlotInstanceID == plotInstanceId)
            ++existingCount;

    // Queue capacity is the BUILDING's ShipmentCapacity (scales with level, e.g. 7/14/21) - NOT the container
    // BaseCapacity (which is 1). Using BaseCapacity wrongly limited the player to a single work order at a time
    // even though the client showed more free slots.
    uint32 const maxShipments = building->ShipmentCapacity ? building->ShipmentCapacity : container->BaseCapacity;

    for (uint32 i = 0; i < count; ++i)
    {
        if (existingCount >= maxShipments)
            break;

        // Select PER ORDER: the tutorial (quest) shipment only for the FIRST order while the intro quest is
        // active and no quest order is queued; the regular shipment otherwise. Selecting once for the whole
        // batch made "start all work orders" place N instant tutorial orders instead of one instant + the rest
        // regular (each showing tutorial time / instantly finished).
        CharShipmentEntry const* shipmentEntry = regularEntry ? regularEntry : shipmentEntries->front();
        if (questEntry && tutorialActive && !questOrderQueued)
            shipmentEntry = questEntry;

        // A work order's cost is the reagents/currencies of its spell (CharShipment.SpellID) - exactly what
        // the client shows. TC's db2 loader decodes the SpellReagents pallet-array into SpellInfo.
        SpellInfo const* costSpell = shipmentEntry->SpellID > 0 ? sSpellMgr->GetSpellInfo(uint32(shipmentEntry->SpellID), DIFFICULTY_NONE) : nullptr;
        bool affordable = true;
        if (costSpell)
        {
            for (std::size_t r = 0; r < costSpell->Reagent.size() && affordable; ++r)
                if (costSpell->Reagent[r] > 0 && costSpell->ReagentCount[r] > 0
                    && !_owner->HasItemCount(uint32(costSpell->Reagent[r]), uint32(costSpell->ReagentCount[r])))
                    affordable = false;
            for (SpellReagentsCurrencyEntry const* rc : costSpell->ReagentsCurrency)
                if (affordable && rc->CurrencyCount > 0 && !_owner->HasCurrency(uint32(rc->CurrencyTypesID), uint32(rc->CurrencyCount)))
                    affordable = false;
        }

        // Stop the batch cleanly once the player can no longer pay.
        if (!affordable)
        {
            WorldPackets::Garrison::CreateShipmentResponse fail;
            fail.ShipmentID = 0;
            fail.ShipmentRecID = shipmentEntry->ID;
            fail.Result = GARRISON_ERROR_NOT_ENOUGH_CURRENCY;
            _owner->SendDirectMessage(fail.Write());
            break;
        }

        // Consume the cost (items + currencies) for this order.
        if (costSpell)
        {
            for (std::size_t r = 0; r < costSpell->Reagent.size(); ++r)
                if (costSpell->Reagent[r] > 0 && costSpell->ReagentCount[r] > 0)
                    _owner->DestroyItemCount(uint32(costSpell->Reagent[r]), uint32(costSpell->ReagentCount[r]), true);
            for (SpellReagentsCurrencyEntry const* rc : costSpell->ReagentsCurrency)
                if (rc->CurrencyCount > 0)
                    _owner->RemoveCurrency(uint32(rc->CurrencyTypesID), rc->CurrencyCount, CurrencyDestroyReason::Garrison);
        }

        uint64 dbId = sGarrisonMgr.GenerateShipmentDbId();

        // Work orders queue: a new order begins working when the latest in-progress order on this plot
        // finishes (retail behaviour, matches FirestormWoD). CreationTime = start-of-work, not request time.
        time_t startTime = GameTime::GetGameTime();
        for (auto const& p : _shipments)
            if (p.second.PlotInstanceID == plotInstanceId)
                startTime = std::max<time_t>(startTime, p.second.CreationTime + p.second.Duration);

        Shipment& shipment = _shipments[dbId];
        shipment.DbID = dbId;
        shipment.ShipmentRecID = shipmentEntry->ID;
        shipment.PlotInstanceID = plotInstanceId;
        shipment.CreationTime = startTime;
        shipment.Duration = shipmentEntry->Duration;
        shipment.AssignedFollowerDBID = 0;

        // Check if a follower is assigned to this building
        for (auto const& followerPair : _followers)
        {
            if (followerPair.second.PacketInfo.CurrentBuildingID == plot->BuildingInfo.PacketInfo->GarrBuildingID)
            {
                shipment.AssignedFollowerDBID = followerPair.second.PacketInfo.DbID;
                break;
            }
        }

        // Persist immediately - a placed work order is a committed player action (materials were just
        // consumed) and must survive a crash, not wait for the periodic character save.
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHARACTER_GARRISON_SHIPMENTS);
        uint8 index = 0;
        stmt->setUInt64(index++, shipment.DbID);
        stmt->setUInt64(index++, _owner->GetGUID().GetCounter());
        stmt->setUInt32(index++, shipment.ShipmentRecID);
        stmt->setUInt32(index++, shipment.PlotInstanceID);
        stmt->setInt64(index++, shipment.CreationTime);
        stmt->setInt32(index++, shipment.Duration);
        stmt->setUInt64(index++, shipment.AssignedFollowerDBID);
        CharacterDatabase.Execute(stmt);

        ++existingCount;
        if (shipmentEntry == questEntry)
            questOrderQueued = true; // the tutorial's single instant order is placed; the rest of the batch is regular

        // Advance the tutorial quest on placement by crediting its "Work Order Started" objective.
        // NOTE: do NOT cast the shipment spell here - that spell CREATES the output item (it is the
        // craft), so casting it on placement instantly completed the order. The order must instead
        // mature over its timer and yield the good on collection.
        //
        // Data-driven for every profession: the "Your First X Work Order" quests pair a monster objective
        // ("X Work Order Started", e.g. Alchemy 86114 / Leatherworking 86112 / Tailoring 86113) with an item
        // objective whose item == the quest shipment's DummyItemID. So when this (quest) shipment is placed,
        // find the player's active quest that needs this shipment's item and credit its monster objective.
        if (shipmentEntry->DummyItemID)
        {
            for (uint16 questSlot = 0; questSlot < MAX_QUEST_LOG_SIZE; ++questSlot)
            {
                uint32 questId = _owner->GetQuestSlotQuestId(questSlot);
                if (!questId)
                    continue;

                Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
                if (!quest || _owner->GetQuestStatus(questId) != QUEST_STATUS_INCOMPLETE)
                    continue;

                bool wantsShipmentItem = false;
                uint32 startedCreatureId = 0;
                for (QuestObjective const& obj : quest->GetObjectives())
                {
                    if (obj.Type == QUEST_OBJECTIVE_ITEM && uint32(obj.ObjectID) == shipmentEntry->DummyItemID)
                        wantsShipmentItem = true;
                    else if (obj.Type == QUEST_OBJECTIVE_MONSTER && obj.ObjectID > 0)
                        startedCreatureId = uint32(obj.ObjectID);
                }

                if (wantsShipmentItem && startedCreatureId)
                    _owner->KilledMonsterCredit(startedCreatureId);
            }
        }

        WorldPackets::Garrison::CreateShipmentResponse response;
        response.ShipmentID = shipment.DbID;
        response.ShipmentRecID = 0; // Sniff confirms Blizzard sends 0 here
        response.Result = GARRISON_SUCCESS;
        _owner->SendDirectMessage(response.Write());
    }

    return GARRISON_SUCCESS;
}

void Garrison::CompleteShipment(uint64 dbId)
{
    auto itr = _shipments.find(dbId);
    if (itr == _shipments.end())
        return;

    Shipment& shipment = itr->second;

    CharShipmentEntry const* shipmentEntry = sCharShipmentStore.LookupEntry(shipment.ShipmentRecID);
    if (shipmentEntry)
    {
        // Follower/buff shipments deliver their reward via a completion spell.
        if (shipmentEntry->OnCompleteSpellID)
            _owner->CastSpell(_owner, shipmentEntry->OnCompleteSpellID, true);

        // Profession buildings yield a produced good: DummyItemID is the output item the work order
        // creates (e.g. the Tannery yields Burnished Leather). Deliver to bags, mailing any overflow.
        if (shipmentEntry->DummyItemID)
        {
            uint32 const itemId = shipmentEntry->DummyItemID;
            uint32 const quantity = 1;
            ItemPosCountVec dest;
            if (_owner->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, quantity) == EQUIP_ERR_OK)
            {
                if (Item* item = _owner->StoreNewItem(dest, itemId, true))
                    _owner->SendNewItem(item, quantity, true, false);
            }
            else
            {
                // Bags full - mail the goods. MailDraft::SendMailTo appends its INSERTs to the passed
                // transaction and dereferences it, so it MUST be a real transaction; a null
                // CharacterDatabaseTransaction(nullptr) crashes here (access violation on collect).
                CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
                MailDraft draft("Garrison Work Order", "The goods produced by your completed work order.");
                if (Item* item = Item::CreateItem(itemId, quantity, ItemContext::NONE, _owner))
                {
                    item->SaveToDB(trans);
                    draft.AddItem(item);
                }
                draft.SendMailTo(trans, MailReceiver(_owner), MailSender(MAIL_CREATURE, 0));
                CharacterDatabase.CommitTransaction(trans);
            }
        }
    }

    WorldPackets::Garrison::CompleteShipmentResponse response;
    response.ShipmentID = dbId;
    response.Result = GARRISON_SUCCESS;
    _owner->SendDirectMessage(response.Write());

    _shipments.erase(itr);

    // Remove the persisted row immediately so a crash can't resurrect an already-collected order.
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHARACTER_GARRISON_SHIPMENT);
    stmt->setUInt64(0, dbId);
    CharacterDatabase.Execute(stmt);
}

void Garrison::CollectReadyShipments(uint32 plotInstanceId)
{
    // Collect (complete) every finished work order on this plot - triggered when the player interacts
    // with the building's work-order crate. Each completed order yields its produced good.
    std::vector<uint64> readyShipments;
    for (auto const& p : _shipments)
        if (p.second.PlotInstanceID == plotInstanceId && p.second.IsReady())
            readyShipments.push_back(p.first);

    for (uint64 dbId : readyShipments)
        CompleteShipment(dbId);
}

void Garrison::SendOpenShipmentUI(ObjectGuid npcGuid)
{
    // Shared by the work-order NPC (CMSG_GARRISON_OPEN_SHIPMENT_NPC) and the crate GO's OnGossipHello.
    // npcGuid is the interacted creature/gameobject; it belongs to a building plot via BuildingInfo.Spawns.
    uint32 plotInstanceId = FindPlotInstanceForNpc(npcGuid);
    if (!plotInstanceId)
        return;

    Plot const* plot = GetPlot(plotInstanceId);
    if (!plot || !plot->BuildingInfo.PacketInfo || !plot->BuildingInfo.PacketInfo->Active)
        return;

    GarrBuildingEntry const* building = sGarrBuildingStore.LookupEntry(plot->BuildingInfo.PacketInfo->GarrBuildingID);
    if (!building)
        return;

    CharShipmentContainerEntry const* container = sGarrisonMgr.GetShipmentContainerForBuilding(building->BuildingType, uint8(GetFaction()));
    if (!container)
        return;

    // Opening the crafter UI is placement only - it must NOT collect finished orders, otherwise opening
    // the UI to queue a new order would instantly hand over the previous order's goods. Collection is a
    // separate action at the crate (GameObject::Use -> CollectReadyShipments).
    WorldPackets::Garrison::OpenShipmentNpcResult result;
    result.NpcGUID = npcGuid;
    result.CharShipmentContainerID = container->ID;
    _owner->SendDirectMessage(result.Write());
}

void Garrison::UpdateWorkOrderCrates()
{
    // Fill each building's work-order crate GO with goods while it holds work orders. The crate's DisplayID
    // is swapped to the CharShipmentContainer's Small/Medium/Large model (by order count vs the Medium/Large
    // thresholds), and back to the GO's base (empty) model when no orders remain. DisplayID is a plain object
    // field (not recomputed by ViewerDependentValue like dynamicFlags), so the swap reaches the client as-is.
    if (!_owner->IsInWorld())
        return;

    Map* map = _owner->GetMap();
    if (!map)
        return;

    for (auto const& [plotInstanceId, plot] : _plots)
    {
        if (!plot.BuildingInfo.PacketInfo || !plot.BuildingInfo.PacketInfo->Active)
            continue;

        GameObject* crate = nullptr;
        for (ObjectGuid const& guid : plot.BuildingInfo.Spawns)
            if (GameObject* go = map->GetGameObject(guid))
                if (go->GetGoType() == GAMEOBJECT_TYPE_GARRISON_SHIPMENT)
                {
                    crate = go;
                    break;
                }

        if (!crate)
            continue;

        // Only READY orders show as goods to collect; in-progress orders show the "working" model (or
        // stay on the base/empty model if the container has none). Counting all orders made the crate look
        // full/ready while an order was still cooking, so clicking it collected nothing.
        uint32 readyCount = 0;
        uint32 inProgressCount = 0;
        for (auto const& p : _shipments)
            if (p.second.PlotInstanceID == plotInstanceId)
            {
                if (p.second.IsReady())
                    ++readyCount;
                else
                    ++inProgressCount;
            }

        uint32 displayId = crate->GetGOInfo()->displayId; // base / empty
        GarrBuildingEntry const* building = sGarrBuildingStore.LookupEntry(plot.BuildingInfo.PacketInfo->GarrBuildingID);
        CharShipmentContainerEntry const* container = building
            ? sGarrisonMgr.GetShipmentContainerForBuilding(building->BuildingType, uint8(GetFaction())) : nullptr;
        if (container)
        {
            if (readyCount > 0)
            {
                if (container->LargeThreshold && readyCount >= container->LargeThreshold && container->LargeDisplayInfoID)
                    displayId = container->LargeDisplayInfoID;
                else if (container->MediumThreshold && readyCount >= container->MediumThreshold && container->MediumDisplayInfoID)
                    displayId = container->MediumDisplayInfoID;
                else if (container->SmallDisplayInfoID)
                    displayId = container->SmallDisplayInfoID;
            }
            else if (inProgressCount > 0 && container->WorkingDisplayInfoID)
                displayId = container->WorkingDisplayInfoID;
        }

        if (displayId && crate->GetDisplayId() != displayId)
            crate->SetDisplayId(displayId);
    }
}

std::vector<Garrison::Shipment const*> Garrison::GetShipmentsForPlot(uint32 plotInstanceId) const
{
    std::vector<Shipment const*> result;
    for (auto const& p : _shipments)
        if (p.second.PlotInstanceID == plotInstanceId)
            result.push_back(&p.second);

    return result;
}

std::vector<Garrison::Shipment const*> Garrison::GetAllShipments() const
{
    std::vector<Shipment const*> result;
    result.reserve(_shipments.size());
    for (auto const& p : _shipments)
        result.push_back(&p.second);

    return result;
}

uint32 Garrison::GetBuildingTypeForPlot(uint32 plotInstanceId) const
{
    Plot const* plot = GetPlot(plotInstanceId);
    if (!plot || !plot->BuildingInfo.PacketInfo)
        return 0;

    GarrBuildingEntry const* building = sGarrBuildingStore.LookupEntry(plot->BuildingInfo.PacketInfo->GarrBuildingID);
    if (!building)
        return 0;

    return building->BuildingType;
}

uint32 Garrison::FindPlotInstanceForNpc(ObjectGuid npcGUID) const
{
    // Find which plot this NPC belongs to by checking building spawns
    for (auto const& p : _plots)
    {
        Plot const& plot = p.second;
        if (plot.BuildingInfo.Spawns.count(npcGUID) > 0)
            return p.first;
    }

    return 0;
}

void Garrison::SendShipmentInfo(ObjectGuid npcGUID)
{
    WorldPackets::Garrison::GetShipmentInfoResponse response;

    uint32 plotInstanceId = FindPlotInstanceForNpc(npcGUID);
    if (!plotInstanceId)
    {
        _owner->SendDirectMessage(response.Write());
        return;
    }

    Plot* plot = GetPlot(plotInstanceId);
    if (!plot || !plot->BuildingInfo.PacketInfo || !plot->BuildingInfo.PacketInfo->Active)
    {
        _owner->SendDirectMessage(response.Write());
        return;
    }

    GarrBuildingEntry const* building = sGarrBuildingStore.LookupEntry(plot->BuildingInfo.PacketInfo->GarrBuildingID);
    if (!building)
    {
        _owner->SendDirectMessage(response.Write());
        return;
    }

    CharShipmentContainerEntry const* container = sGarrisonMgr.GetShipmentContainerForBuilding(building->BuildingType, uint8(GetFaction()));
    if (!container)
    {
        _owner->SendDirectMessage(response.Write());
        return;
    }

    std::vector<CharShipmentEntry const*> const* shipmentEntries = sGarrisonMgr.GetShipmentsForContainer(container->ID);
    if (!shipmentEntries || shipmentEntries->empty())
    {
        _owner->SendDirectMessage(response.Write());
        return;
    }

    // ShipmentID is a CharShipment.db2 id (the work-order recipe), NOT the container id — the client looks
    // it up to load reagents/output, so an invalid value (the container id) null-derefs and crashes it
    // (sniff-verified: retail sends the CharShipment whose ContainerID == this container).
    CharShipmentEntry const* recipe = shipmentEntries->front();
    response.Success = true;
    response.ShipmentID = recipe->ID;
    response.MaxShipments = building->ShipmentCapacity ? building->ShipmentCapacity : container->BaseCapacity; // per-level queue capacity, not container BaseCapacity(=1)
    response.PlotInstanceID = plotInstanceId;

    std::vector<Shipment const*> plotShipments = GetShipmentsForPlot(plotInstanceId);
    response.Shipments.reserve(plotShipments.size());
    for (Shipment const* shipment : plotShipments)
    {
        WorldPackets::Garrison::CharacterShipment& packetShipment = response.Shipments.emplace_back();
        packetShipment.ShipmentRecID = shipment->ShipmentRecID;
        packetShipment.ShipmentID = shipment->DbID;
        packetShipment.AssignedFollowerDBID = shipment->AssignedFollowerDBID;
        packetShipment.CreationTime = shipment->CreationTime;
        packetShipment.ShipmentDuration = shipment->Duration;
        packetShipment.BuildingTypeID = building->BuildingType;
        packetShipment.GarrTypeID = static_cast<uint8>(GetType());
    }

    _owner->SendDirectMessage(response.Write());
}

void Garrison::SendLandingPageShipments()
{
    WorldPackets::Garrison::GetLandingPageShipmentsResponse response;
    response.GarrTypeID = static_cast<uint32>(GetType());

    for (auto const& p : _shipments)
    {
        Shipment const& shipment = p.second;
        WorldPackets::Garrison::CharacterShipment& packetShipment = response.Shipments.emplace_back();
        packetShipment.ShipmentRecID = shipment.ShipmentRecID;
        packetShipment.ShipmentID = shipment.DbID;
        packetShipment.AssignedFollowerDBID = shipment.AssignedFollowerDBID;
        packetShipment.CreationTime = shipment.CreationTime;
        packetShipment.ShipmentDuration = shipment.Duration;
        uint8 buildingType = GetBuildingTypeForPlot(shipment.PlotInstanceID);
        packetShipment.BuildingTypeID = buildingType;
        if (CharShipmentContainerEntry const* container = sGarrisonMgr.GetShipmentContainerForBuilding(buildingType, uint8(GetFaction())))
            packetShipment.ContainerID = container->ID;
        packetShipment.GarrTypeID = static_cast<uint8>(GetType());
    }

    _owner->SendDirectMessage(response.Write());
}

// ============================================================
// Talent helpers
// ============================================================

bool Garrison::Talent::IsResearching() const
{
    return ResearchStartTime != 0;
}

bool Garrison::Talent::IsResearchComplete() const
{
    if (!IsResearching())
        return false;

    std::vector<GarrTalentRankEntry const*> const* ranks = sGarrisonMgr.GetTalentRanksForTalent(GarrTalentID);
    if (!ranks || Rank >= static_cast<int32>(ranks->size()))
        return false;

    GarrTalentRankEntry const* rankEntry = (*ranks)[Rank];
    return GameTime::GetGameTime() >= ResearchStartTime + rankEntry->ResearchDurationSecs;
}

// ============================================================
// Talent management
// ============================================================

Garrison::Talent const* Garrison::GetTalent(uint32 garrTalentID) const
{
    auto itr = _talents.find(garrTalentID);
    if (itr != _talents.end())
        return &itr->second;

    return nullptr;
}

void Garrison::CompleteAllTalentResearch()
{
    for (auto& [talentId, talent] : _talents)
    {
        if (!talent.IsResearching())
            continue;

        if (!talent.IsResearchComplete())
            continue;

        talent.Rank++;
        talent.ResearchStartTime = 0;

        TC_LOG_DEBUG("garrison", "Garrison::CompleteAllTalentResearch: Player {} talent {} completed research to rank {}",
            _owner->GetGUID().ToString().c_str(), talentId, talent.Rank);
    }
}

uint32 Garrison::LearnTalent(uint32 garrTalentID, bool isTemporary)
{
    GarrTalentEntry const* talentEntry = sGarrTalentStore.LookupEntry(garrTalentID);
    if (!talentEntry)
        return GARRISON_ERROR_INVALID_TALENT;

    // Verify the talent tree belongs to this garrison type
    GarrTalentTreeEntry const* treeEntry = sGarrTalentTreeStore.LookupEntry(talentEntry->GarrTalentTreeID);
    if (!treeEntry || treeEntry->GarrTypeID != static_cast<int8>(GetType()))
        return GARRISON_ERROR_INVALID_TALENT;

    // Check if already learned
    auto itr = _talents.find(garrTalentID);
    if (itr != _talents.end())
        return GARRISON_ERROR_INVALID_TALENT;

    // Check prerequisite talent
    if (talentEntry->PrerequisiteTalentID)
    {
        auto prereqItr = _talents.find(talentEntry->PrerequisiteTalentID);
        if (prereqItr == _talents.end() || prereqItr->second.Rank < 1)
            return GARRISON_ERROR_INVALID_TALENT;
    }

    // Learn the talent at rank 0 (researching to rank 1 happens via ResearchTalent)
    Talent& talent = _talents[garrTalentID];
    talent.GarrTalentID = garrTalentID;
    talent.Rank = 0;
    talent.ResearchStartTime = 0;
    talent.Flags = isTemporary ? GARRISON_TALENT_FLAG_TEMPORARY : GARRISON_TALENT_FLAG_NONE;
    talent.SoulbindConduitID = 0;
    talent.SoulbindConduitRank = 0;

    // Send result
    WorldPackets::Garrison::GarrisonResearchTalentResult result;
    result.Result = GARRISON_SUCCESS;
    result.GarrTypeID = static_cast<uint8>(GetType());
    result.Talent.GarrTalentID = talent.GarrTalentID;
    result.Talent.Rank = talent.Rank;
    result.Talent.ResearchStartTime = time_t(talent.ResearchStartTime);
    result.Talent.Flags = talent.Flags;
    _owner->SendDirectMessage(result.Write());

    return GARRISON_SUCCESS;
}

uint32 Garrison::ResearchTalent(uint32 garrTalentID)
{
    GarrTalentEntry const* talentEntry = sGarrTalentStore.LookupEntry(garrTalentID);
    if (!talentEntry)
        return GARRISON_ERROR_INVALID_TALENT;

    // Verify the talent tree belongs to this garrison type
    GarrTalentTreeEntry const* treeEntry = sGarrTalentTreeStore.LookupEntry(talentEntry->GarrTalentTreeID);
    if (!treeEntry || treeEntry->GarrTypeID != static_cast<int8>(GetType()))
        return GARRISON_ERROR_INVALID_TALENT;

    // Must already be learned (or learn it now if not)
    auto itr = _talents.find(garrTalentID);
    if (itr == _talents.end())
    {
        // Auto-learn if not yet learned
        Talent& talent = _talents[garrTalentID];
        talent.GarrTalentID = garrTalentID;
        talent.Rank = 0;
        talent.ResearchStartTime = 0;
        talent.Flags = 0;
        talent.SoulbindConduitID = 0;
        talent.SoulbindConduitRank = 0;
        itr = _talents.find(garrTalentID);
    }

    Talent& talent = itr->second;

    // Check if already researching any talent in this tree
    for (auto const& [id, t] : _talents)
    {
        if (id == garrTalentID)
            continue;

        if (t.IsResearching())
        {
            GarrTalentEntry const* otherTalent = sGarrTalentStore.LookupEntry(id);
            if (otherTalent && otherTalent->GarrTalentTreeID == talentEntry->GarrTalentTreeID)
                return GARRISON_ERROR_ALREADY_RESEARCHING_TALENT;
        }
    }

    // Check if this talent is already researching
    if (talent.IsResearching())
        return GARRISON_ERROR_ALREADY_RESEARCHING_TALENT;

    // Get the rank entry for the next rank
    std::vector<GarrTalentRankEntry const*> const* ranks = sGarrisonMgr.GetTalentRanksForTalent(garrTalentID);
    if (!ranks || talent.Rank >= static_cast<int32>(ranks->size()))
        return GARRISON_ERROR_INVALID_TALENT;

    GarrTalentRankEntry const* rankEntry = (*ranks)[talent.Rank];

    // Deduct currency cost
    if (rankEntry->ResearchCostCurrencyTypesID && rankEntry->ResearchCost > 0)
    {
        if (!_owner->HasCurrency(rankEntry->ResearchCostCurrencyTypesID, rankEntry->ResearchCost))
            return GARRISON_ERROR_NOT_ENOUGH_CURRENCY;

        _owner->RemoveCurrency(rankEntry->ResearchCostCurrencyTypesID, rankEntry->ResearchCost, CurrencyDestroyReason::Garrison);
    }

    // Deduct gold cost
    if (rankEntry->ResearchGoldCost > 0)
    {
        if (!_owner->HasEnoughMoney(uint64(rankEntry->ResearchGoldCost) * GOLD))
            return GARRISON_ERROR_NOT_ENOUGH_GOLD;

        _owner->ModifyMoney(-int64(uint64(rankEntry->ResearchGoldCost) * GOLD));
    }

    // Start research
    talent.ResearchStartTime = GameTime::GetGameTime();

    // If research is instant (duration 0), complete immediately
    if (rankEntry->ResearchDurationSecs <= 0)
    {
        talent.Rank++;
        talent.ResearchStartTime = 0;
    }

    // Send result
    WorldPackets::Garrison::GarrisonResearchTalentResult result;
    result.Result = GARRISON_SUCCESS;
    result.GarrTypeID = static_cast<uint8>(GetType());
    result.Talent.GarrTalentID = talent.GarrTalentID;
    result.Talent.Rank = talent.Rank;
    result.Talent.ResearchStartTime = time_t(talent.ResearchStartTime);
    result.Talent.Flags = talent.Flags;
    if (talent.SoulbindConduitID != 0)
    {
        WorldPackets::Garrison::GarrisonTalentSocketData socket;
        socket.SoulbindConduitID = talent.SoulbindConduitID;
        socket.SoulbindConduitRank = talent.SoulbindConduitRank;
        result.Talent.Socket = socket;
    }
    _owner->SendDirectMessage(result.Write());

    return GARRISON_SUCCESS;
}

uint32 Garrison::SocketTalent(uint32 garrTalentID, int32 soulbindConduitID, int32 soulbindConduitRank)
{
    GarrTalentEntry const* talentEntry = sGarrTalentStore.LookupEntry(garrTalentID);
    if (!talentEntry)
        return GARRISON_ERROR_INVALID_TALENT;

    // Must have socket properties
    if (!talentEntry->GarrTalentSocketPropertiesID)
        return GARRISON_ERROR_INVALID_TALENT;

    auto itr = _talents.find(garrTalentID);
    if (itr == _talents.end())
        return GARRISON_ERROR_INVALID_TALENT;

    Talent& talent = itr->second;
    talent.SoulbindConduitID = soulbindConduitID;
    talent.SoulbindConduitRank = soulbindConduitRank;

    // Send result
    WorldPackets::Garrison::GarrisonResearchTalentResult result;
    result.Result = GARRISON_SUCCESS;
    result.GarrTypeID = static_cast<uint8>(GetType());
    result.Talent.GarrTalentID = talent.GarrTalentID;
    result.Talent.Rank = talent.Rank;
    result.Talent.ResearchStartTime = time_t(talent.ResearchStartTime);
    result.Talent.Flags = talent.Flags;
    WorldPackets::Garrison::GarrisonTalentSocketData socket;
    socket.SoulbindConduitID = talent.SoulbindConduitID;
    socket.SoulbindConduitRank = talent.SoulbindConduitRank;
    result.Talent.Socket = socket;
    _owner->SendDirectMessage(result.Write());

    return GARRISON_SUCCESS;
}

// ============================================================
// Trophy system
// ============================================================

void Garrison::AddTrophy(uint32 trophyID)
{
    _trophies.insert(trophyID);
}

void Garrison::RemoveTrophy(uint32 trophyID)
{
    _trophies.erase(trophyID);
}
