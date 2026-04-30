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

#include "GarrisonPackets.h"
#include "DB2Structure.h"
#include "Errors.h"
#include "PacketOperators.h"

namespace WorldPackets::Garrison
{
WorldPacket const* GarrisonCreateResult::Write()
{
    _worldPacket << uint32(Result);
    _worldPacket << uint32(GarrSiteLevelID);

    return &_worldPacket;
}

WorldPacket const* GarrisonDeleteResult::Write()
{
    _worldPacket << uint32(Result);
    _worldPacket << uint32(GarrSiteID);

    return &_worldPacket;
}

ByteBuffer& operator<<(ByteBuffer& data, GarrisonPlotInfo const& plotInfo)
{
    data << uint32(plotInfo.GarrPlotInstanceID);
    data << plotInfo.PlotPos;
    data << uint8(plotInfo.PlotType);

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, GarrisonBuildingInfo const& buildingInfo)
{
    data << buildingInfo.TimeBuilt;
    data << uint32(buildingInfo.GarrPlotInstanceID);
    data << uint32(buildingInfo.GarrBuildingID);
    data << uint32(buildingInfo.CurrentGarSpecID);
    data << buildingInfo.TimeSpecCooldown;
    data << Bits<1>(buildingInfo.Active);
    data.FlushBits();

    return data;
}

// Sniff-verified for 12.0.1.66102 (148-byte and 156-byte FOLLOWER_CHANGED_QUALITY bodies
// match this writer byte-for-byte — see SNIFF_AUDIT_12.0.1.66102.md §3.1).
// Note: AGENT_BRIEF_GARRISON.md's Deserialize_JamGarrisonFollower @ 0x7FF75C1752A0 decodes
// the JAM-mirror wire format (account-data field-mask packet, 16 VarUInt32 reads ending in
// HealCost/HealStartTime/HealDuration). That is a SEPARATE code path from the dedicated
// SMSGs — do NOT transplant the brief's layout here. Audit §5.1 verified that doing so
// produces nonsense values (e.g. DbID=49148642 vs the actual 397927906).
ByteBuffer& operator<<(ByteBuffer& data, GarrisonFollower const& follower)
{
    data << uint64(follower.DbID);
    data << uint32(follower.GarrFollowerID);
    data << uint32(follower.Quality);
    data << uint32(follower.FollowerLevel);
    data << uint32(follower.ItemLevelWeapon);
    data << uint32(follower.ItemLevelArmor);
    data << uint32(follower.Xp);
    data << uint32(follower.Durability);
    data << uint32(follower.CurrentBuildingID);
    data << uint32(follower.CurrentMissionID);
    data << Size<uint32>(follower.AbilityID);
    data << uint32(follower.ZoneSupportSpellID);
    data << uint32(follower.FollowerStatus);
    data << int32(follower.Health);
    data << int8(follower.BoardIndex);
    data << follower.HealingTimestamp;
    for (GarrAbilityEntry const* ability : follower.AbilityID)
        data << uint32(ability->ID);

    data << SizedString::BitsSize<7>(follower.CustomName);
    data.FlushBits();

    data << SizedString::Data(follower.CustomName);

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, GarrisonEncounter const& encounter)
{
    data << int32(encounter.GarrEncounterID);
    data << Size<uint32>(encounter.Mechanics);
    data << int32(encounter.GarrAutoCombatantID);
    data << int32(encounter.Health);
    data << int32(encounter.MaxHealth);
    data << int32(encounter.Attack);
    data << int8(encounter.BoardIndex);

    if (!encounter.Mechanics.empty())
        data.append(encounter.Mechanics.data(), encounter.Mechanics.size());

    return data;
}

// Sniff-verified for 12.0.1.66102 — observed in the 8316-byte SMSG_GET_GARRISON_INFO_RESULT
// body (SNIFF_AUDIT_12.0.1.66102.md §5.3). int32 ItemFileDataID is always present
// (sometimes zero, sometimes a real DBD ID like 1599042); the trailing OptionalInit bit
// gates an optional ItemInstance blob.
// The brief's Deserialize_JamGarrisonMissionReward @ 0x7FF75C1754C0 (Byte-gated VarUInt32)
// decodes the JAM-mirror code path, NOT this dedicated SMSG. Both formats coexist for
// different sub-collection sync paths.
ByteBuffer& operator<<(ByteBuffer& data, GarrisonMissionReward const& missionRewardItem)
{
    data << int32(missionRewardItem.ItemID);
    data << uint32(missionRewardItem.ItemQuantity);
    data << int32(missionRewardItem.CurrencyID);
    data << uint32(missionRewardItem.CurrencyQuantity);
    data << uint32(missionRewardItem.FollowerXP);
    data << uint32(missionRewardItem.GarrMssnBonusAbilityID);
    data << int32(missionRewardItem.ItemFileDataID);
    data << OptionalInit(missionRewardItem.ItemInstance);
    data.FlushBits();

    if (missionRewardItem.ItemInstance)
        data << *missionRewardItem.ItemInstance;

    return data;
}

// Sniff-verified for 12.0.1.66102 (SNIFF_AUDIT_12.0.1.66102.md §3.15, embedded in
// the 188-byte SMSG_GET_GARRISON_INFO_RESULT). MissionScalar(float) sits between
// Flags and ContentTuningID followed by 3 size fields. Note: SMSG_GARRISON_START_MISSION_RESULT
// embeds a Mission whose exact byte layout has a residual ~2-byte misalignment vs this
// writer (audit §4.1) — needs IDA pseudocode of Build_GarrisonStartMissionResult to fully
// resolve. The brief's Deserialize_JamGarrisonMission @ 0x7FF75C1755E0 with Currency/
// BonusActions/AutoMissionData tail is the JAM-mirror path, NOT this dedicated SMSG.
ByteBuffer& operator<<(ByteBuffer& data, GarrisonMission const& mission)
{
    data << uint64(mission.DbID);
    data << int32(mission.MissionRecID);
    data << mission.OfferTime;
    data << mission.OfferDuration;
    data << mission.StartTime;
    data << mission.TravelDuration;
    data << mission.MissionDuration;
    data << int32(mission.MissionState);
    data << int32(mission.SuccessChance);
    data << uint32(mission.Flags);
    data << float(mission.MissionScalar);
    data << int32(mission.ContentTuningID);
    data << Size<uint32>(mission.Encounters);
    data << Size<uint32>(mission.Rewards);
    data << Size<uint32>(mission.OvermaxRewards);

    for (GarrisonEncounter const& encounter : mission.Encounters)
        data << encounter;

    for (GarrisonMissionReward const& missionRewardItem : mission.Rewards)
        data << missionRewardItem;

    for (GarrisonMissionReward const& missionRewardItem : mission.OvermaxRewards)
        data << missionRewardItem;

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, GarrisonMissionBonusAbility const& areaBonus)
{
    data << areaBonus.StartTime;
    data << uint32(areaBonus.GarrMssnBonusAbilityID);

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, GarrisonTalentSocketData const& talentSocketData)
{
    data << int32(talentSocketData.SoulbindConduitID);
    data << int32(talentSocketData.SoulbindConduitRank);

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, GarrisonTalent const& talent)
{
    data << int32(talent.GarrTalentID);
    data << int32(talent.Rank);
    data << talent.ResearchStartTime;
    data << int32(talent.Flags);
    data << OptionalInit(talent.Socket);
    data.FlushBits();

    if (talent.Socket)
        data << *talent.Socket;

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, GarrisonCollectionEntry const& collectionEntry)
{
    data << int32(collectionEntry.EntryID);
    data << int32(collectionEntry.Rank);

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, GarrisonMissionEndingFollower const& endingFollower)
{
    data << uint64(endingFollower.DbID);
    data << int32(endingFollower.Health);

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, GarrisonCollection const& collection)
{
    data << int32(collection.Type);
    data << Size<uint32>(collection.Entries);
    for (GarrisonCollectionEntry const& collectionEntry : collection.Entries)
        data << collectionEntry;

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, GarrisonEventEntry const& event)
{
    data << int64(event.EventValue);
    data << int32(event.EntryID);

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, GarrisonEventList const& eventList)
{
    data << int32(eventList.Type);
    data << Size<uint32>(eventList.Events);
    for (GarrisonEventEntry const& event : eventList.Events)
        data << event;

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, GarrisonSpecGroup const& specGroup)
{
    data << int32(specGroup.ChrSpecializationID);
    data << int32(specGroup.SoulbindID);

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, GarrisonInfo const& garrison)
{
    ASSERT(garrison.Missions.size() == garrison.MissionRewards.size());
    ASSERT(garrison.Missions.size() == garrison.MissionOvermaxRewards.size());
    ASSERT(garrison.Missions.size() == garrison.CanStartMission.size());

    data << uint8(garrison.GarrTypeID);
    data << int32(garrison.GarrSiteID);
    data << int32(garrison.GarrSiteLevelID);
    data << Size<uint32>(garrison.Buildings);
    data << Size<uint32>(garrison.Plots);
    data << Size<uint32>(garrison.Followers);
    data << Size<uint32>(garrison.AutoTroops);
    data << Size<uint32>(garrison.Missions);
    data << Size<uint32>(garrison.MissionRewards);
    data << Size<uint32>(garrison.MissionOvermaxRewards);
    data << Size<uint32>(garrison.MissionAreaBonuses);
    data << Size<uint32>(garrison.Talents);
    data << Size<uint32>(garrison.Collections);
    data << Size<uint32>(garrison.EventLists);
    data << Size<uint32>(garrison.SpecGroups);
    data << Size<uint32>(garrison.CanStartMission);
    data << Size<uint32>(garrison.ArchivedMissions);
    data << int32(garrison.NumFollowerActivationsRemaining);
    data << uint32(garrison.NumMissionsStartedToday);
    data << int32(garrison.MinAutoTroopLevel);

    for (GarrisonPlotInfo const* plot : garrison.Plots)
        data << *plot;

    for (std::vector<GarrisonMissionReward> const& missionReward : garrison.MissionRewards)
        data << Size<uint32>(missionReward);

    for (std::vector<GarrisonMissionReward> const& missionReward : garrison.MissionOvermaxRewards)
        data << Size<uint32>(missionReward);

    for (GarrisonMissionBonusAbility const* areaBonus : garrison.MissionAreaBonuses)
        data << *areaBonus;

    for (GarrisonCollection const& collection : garrison.Collections)
        data << collection;

    for (GarrisonEventList const& eventList : garrison.EventLists)
        data << eventList;

    for (GarrisonSpecGroup const& specGroup : garrison.SpecGroups)
        data << specGroup;

    if (!garrison.ArchivedMissions.empty())
        data.append(garrison.ArchivedMissions.data(), garrison.ArchivedMissions.size());

    for (GarrisonBuildingInfo const* building : garrison.Buildings)
        data << *building;

    for (bool canStartMission : garrison.CanStartMission)
        data << Bits<1>(canStartMission);

    data.FlushBits();

    for (GarrisonFollower const* follower : garrison.Followers)
        data << *follower;

    for (GarrisonFollower const* follower : garrison.AutoTroops)
        data << *follower;

    for (GarrisonMission const* mission : garrison.Missions)
        data << *mission;

    for (GarrisonTalent const& talent : garrison.Talents)
        data << talent;

    for (std::vector<GarrisonMissionReward> const& missionReward : garrison.MissionRewards)
        for (GarrisonMissionReward const& missionRewardItem : missionReward)
            data << missionRewardItem;

    for (std::vector<GarrisonMissionReward> const& missionReward : garrison.MissionOvermaxRewards)
        for (GarrisonMissionReward const& missionRewardItem : missionReward)
            data << missionRewardItem;

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, FollowerSoftCapInfo const& followerSoftCapInfo)
{
    data << uint8(followerSoftCapInfo.GarrFollowerTypeID);
    data << uint32(followerSoftCapInfo.Count);
    return data;
}

WorldPacket const* GetGarrisonInfoResult::Write()
{
    _worldPacket << int8(FactionIndex);
    _worldPacket << Size<uint32>(Garrisons);
    _worldPacket << Size<uint32>(FollowerSoftCaps);
    for (FollowerSoftCapInfo const& followerSoftCapInfo : FollowerSoftCaps)
        _worldPacket << followerSoftCapInfo;

    for (GarrisonInfo const& garrison : Garrisons)
        _worldPacket << garrison;

    return &_worldPacket;
}

ByteBuffer& operator<<(ByteBuffer& data, GarrisonRemoteBuildingInfo const& building)
{
    data << uint32(building.GarrPlotInstanceID);
    data << uint32(building.GarrBuildingID);

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, GarrisonRemoteSiteInfo const& site)
{
    data << uint32(site.GarrSiteLevelID);
    data << Size<uint32>(site.Buildings);
    for (GarrisonRemoteBuildingInfo const& building : site.Buildings)
        data << building;

    return data;
}

WorldPacket const* GarrisonRemoteInfo::Write()
{
    _worldPacket << Size<uint32>(Sites);
    for (GarrisonRemoteSiteInfo const& site : Sites)
        _worldPacket << site;

    return &_worldPacket;
}

void GarrisonPurchaseBuilding::Read()
{
    _worldPacket >> NpcGUID;
    _worldPacket >> PlotInstanceID;
    _worldPacket >> BuildingID;
}

WorldPacket const* GarrisonPlaceBuildingResult::Write()
{
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << uint32(Result);
    _worldPacket << BuildingInfo;
    _worldPacket << Bits<1>(PlayActivationCinematic);
    _worldPacket.FlushBits();

    return &_worldPacket;
}

void GarrisonCancelConstruction::Read()
{
    _worldPacket >> NpcGUID;
    _worldPacket >> PlotInstanceID;
}

WorldPacket const* GarrisonBuildingRemoved::Write()
{
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << uint32(Result);
    _worldPacket << uint32(GarrPlotInstanceID);
    _worldPacket << uint32(GarrBuildingID);

    return &_worldPacket;
}

WorldPacket const* GarrisonLearnBlueprintResult::Write()
{
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << uint32(Result);
    _worldPacket << uint32(BuildingID);

    return &_worldPacket;
}

WorldPacket const* GarrisonUnlearnBlueprintResult::Write()
{
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << uint32(Result);
    _worldPacket << uint32(BuildingID);

    return &_worldPacket;
}

WorldPacket const* GarrisonRequestBlueprintAndSpecializationDataResult::Write()
{
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << uint32(BlueprintsKnown ? BlueprintsKnown->size() : 0);
    _worldPacket << uint32(SpecializationsKnown ? SpecializationsKnown->size() : 0);
    if (BlueprintsKnown)
        for (uint32 blueprint : *BlueprintsKnown)
            _worldPacket << uint32(blueprint);

    if (SpecializationsKnown)
        for (uint32 specialization : *SpecializationsKnown)
            _worldPacket << uint32(specialization);

    return &_worldPacket;
}

ByteBuffer& operator<<(ByteBuffer& data, GarrisonBuildingMapData const& building)
{
    data << uint32(building.GarrBuildingPlotInstID);
    data << building.Pos;

    return data;
}

WorldPacket const* GarrisonMapDataResponse::Write()
{
    _worldPacket << Size<uint32>(Buildings);
    for (GarrisonBuildingMapData& landmark : Buildings)
        _worldPacket << landmark;

    return &_worldPacket;
}

WorldPacket const* GarrisonPlotPlaced::Write()
{
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << *PlotInfo;

    return &_worldPacket;
}

WorldPacket const* GarrisonPlotRemoved::Write()
{
    _worldPacket << uint32(GarrPlotInstanceID);

    return &_worldPacket;
}

WorldPacket const* GarrisonAddFollowerResult::Write()
{
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << uint32(Result);
    _worldPacket << Follower;

    return &_worldPacket;
}

WorldPacket const* GarrisonRemoveFollowerResult::Write()
{
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << uint32(Result);
    _worldPacket << uint64(FollowerDBID);
    _worldPacket << uint32(Destroyed);

    return &_worldPacket;
}

WorldPacket const* GarrisonBuildingActivated::Write()
{
    _worldPacket << uint32(GarrPlotInstanceID);

    return &_worldPacket;
}

// ============================================================
// Mission CMSG Read implementations
// ============================================================

void GarrisonStartMission::Read()
{
    _worldPacket >> NpcGUID;
    uint32 followerCount = 0;
    _worldPacket >> followerCount;
    _worldPacket >> MissionRecID;

    FollowerDBIDs.resize(followerCount);
    for (uint32 i = 0; i < followerCount; ++i)
    {
        _worldPacket >> FollowerDBIDs[i];
        _worldPacket.read_skip<int32>();  // BoardIndex (unused, always -1)
        _worldPacket.read_skip<int32>();  // Health (unused, always 0)
        _worldPacket.read_skip<uint8>();  // HasFollowerEntry (unused, always false)
    }
}

void GarrisonCompleteMission::Read()
{
    _worldPacket >> NpcGUID;
    _worldPacket >> MissionRecID;
}

void GarrisonMissionBonusRoll::Read()
{
    _worldPacket >> NpcGUID;
    _worldPacket >> MissionRecID;
}

void OpenMissionNpc::Read()
{
    _worldPacket >> NpcGUID;
}

void GarrisonGetMissionReward::Read()
{
    _worldPacket >> NpcGUID;
    _worldPacket >> MissionRecID;
}

// ============================================================
// Mission SMSG Write implementations
// ============================================================

// Wire layout: IDA dispatcher case 4980763 (sub_7FF75C1449A0) + sniff-verified
// against 12.0.1.66102 captures (115 / 149 / 149 byte bodies, all decoding cleanly):
//   u32 Result, u16 NumOfferedToday,
//   u32 FollowerInfoCount, u32 FollowersCount,
//   GarrisonMission Mission,
//   FollowerInfo[FollowerInfoCount]   (each: u64 DbID, i32 BoardIndex, i32 Health,
//                                      u8 HasFollowerEntry; if HasFollowerEntry then u32 FollowerEntry),
//   GarrisonFollower[FollowersCount]
// See SNIFF_AUDIT_12.0.1.66102.md §8.1.
WorldPacket const* GarrisonStartMissionResult::Write()
{
    _worldPacket << uint32(Result);
    _worldPacket << uint16(NumOfferedToday);
    _worldPacket << uint32(FollowerInfos.size());
    _worldPacket << uint32(Followers.size());
    _worldPacket << Mission;
    for (GarrisonMissionFollowerEntry const& info : FollowerInfos)
    {
        _worldPacket << uint64(info.DbID);
        _worldPacket << int32(info.BoardIndex);
        _worldPacket << int32(info.Health);
        _worldPacket << uint8(info.HasFollowerEntry);
        if (info.HasFollowerEntry)
            _worldPacket << uint32(info.FollowerEntry);
    }
    for (GarrisonFollower const& follower : Followers)
        _worldPacket << follower;

    return &_worldPacket;
}

WorldPacket const* GarrisonCompleteMissionResult::Write()
{
    _worldPacket << uint32(Result);
    _worldPacket << Mission;
    _worldPacket << uint32(MissionRecID);
    _worldPacket << Bits<1>(Succeeded);
    _worldPacket.FlushBits();

    return &_worldPacket;
}

WorldPacket const* GarrisonMissionBonusRollResult::Write()
{
    _worldPacket << Mission;
    _worldPacket << uint32(MissionRecID);
    _worldPacket << uint32(Result);

    return &_worldPacket;
}

// IDA case 4980762: u8 GarrTypeID, u32 Result, u8 State, Bits<1>+Flush, GarrisonMission.
// The Mission struct already contains its own Rewards/OvermaxRewards arrays — duplicating
// them at the outer level (as TC previously did) wrote bytes the client never reads.
// See SNIFF_AUDIT §8.23.
WorldPacket const* GarrisonAddMissionResult::Write()
{
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << uint32(Result);
    _worldPacket << uint8(State);
    _worldPacket << Bits<1>(CanStartMission);
    _worldPacket.FlushBits();
    _worldPacket << Mission;

    return &_worldPacket;
}

// IDA case 4980771: u8 GarrTypeID, u32 Result, u32 MissionRecID. See SNIFF_AUDIT §8.29.
WorldPacket const* GarrisonDeleteMissionResult::Write()
{
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << uint32(Result);
    _worldPacket << uint32(MissionRecID);

    return &_worldPacket;
}

WorldPacket const* GarrisonMissionStartConditionUpdate::Write()
{
    ASSERT(MissionRecIDs.size() == CanStartMission.size());

    _worldPacket << Size<uint32>(MissionRecIDs);
    _worldPacket << Size<uint32>(CanStartMission);

    if (!MissionRecIDs.empty())
        _worldPacket.append(MissionRecIDs.data(), MissionRecIDs.size());

    for (bool canStart : CanStartMission)
        _worldPacket << Bits<1>(canStart);

    _worldPacket.FlushBits();

    return &_worldPacket;
}

WorldPacket const* GarrisonIsUpgradeableResponse::Write()
{
    _worldPacket << uint32(Result);

    return &_worldPacket;
}

WorldPacket const* GarrisonUpgradeResult::Write()
{
    _worldPacket << uint32(GarrSiteLevelID);
    _worldPacket << uint32(Result);

    return &_worldPacket;
}

// ============================================================
// Follower CMSG Read implementations
// ============================================================

void GarrisonAssignFollowerToBuilding::Read()
{
    _worldPacket >> NpcGUID;
    _worldPacket >> PlotInstanceID;
    _worldPacket >> FollowerDBID;
}

void GarrisonRemoveFollowerFromBuilding::Read()
{
    _worldPacket >> NpcGUID;
    _worldPacket >> FollowerDBID;
}

void GarrisonRemoveFollower::Read()
{
    _worldPacket >> NpcGUID;
    _worldPacket >> FollowerDBID;
}

void GarrisonRenameFollower::Read()
{
    _worldPacket >> FollowerDBID;
    _worldPacket >> SizedString::BitsSize<7>(FollowerName);
    _worldPacket >> SizedString::Data(FollowerName);
}

void GarrisonSetFollowerFavorite::Read()
{
    _worldPacket >> FollowerDBID;
    _worldPacket >> Bits<1>(Favorite);
}

void GarrisonSetFollowerInactive::Read()
{
    _worldPacket >> FollowerDBID;
    _worldPacket >> Bits<1>(Inactive);
}

void GarrisonRecruitFollower::Read()
{
    _worldPacket >> NpcGUID;
    _worldPacket >> FollowerIndex;
}

void GarrisonGenerateRecruits::Read()
{
    _worldPacket >> NpcGUID;
    _worldPacket >> MechanicTypeID;
    _worldPacket >> TraitID;
}

void GarrisonFullyHealAllFollowers::Read()
{
    _worldPacket >> NpcGUID;
}

void GarrisonAddFollowerHealth::Read()
{
    _worldPacket >> NpcGUID;
    _worldPacket >> FollowerDBID;
    _worldPacket >> HealthToAdd;
}

void GarrisonGetClassSpecCategoryInfo::Read()
{
    _worldPacket >> GarrFollowerTypeID;
}

void GarrisonSetRecruitmentPreferences::Read()
{
    _worldPacket >> NpcGUID;
    _worldPacket >> AbilityID;
    _worldPacket >> TraitID;
}

// ============================================================
// Follower SMSG Write implementations
// ============================================================

// IDA case 4980780: u32 Result, u64 FollowerDBID, u32 PlotInstanceID. See SNIFF_AUDIT §8.37.
WorldPacket const* GarrisonAssignFollowerToBuildingResult::Write()
{
    _worldPacket << uint32(Result);
    _worldPacket << uint64(FollowerDBID);
    _worldPacket << uint32(PlotInstanceID);

    return &_worldPacket;
}

// IDA case 4980781: u32 Result, u64 FollowerDBID. See SNIFF_AUDIT §8.38.
WorldPacket const* GarrisonRemoveFollowerFromBuildingResult::Write()
{
    _worldPacket << uint32(Result);
    _worldPacket << uint64(FollowerDBID);

    return &_worldPacket;
}

// IDA case 4980782: u32 Result, GarrisonFollower. See SNIFF_AUDIT §8.39.
WorldPacket const* GarrisonRenameFollowerResult::Write()
{
    _worldPacket << uint32(Result);
    _worldPacket << Follower;

    return &_worldPacket;
}

WorldPacket const* GarrisonFollowerChangedFlags::Write()
{
    _worldPacket << uint32(Result);
    _worldPacket << Follower;

    return &_worldPacket;
}

WorldPacket const* GarrisonFollowerChangedXP::Write()
{
    _worldPacket << uint32(Result);
    _worldPacket << uint32(TotalXp);
    _worldPacket << OldFollower;
    _worldPacket << Follower;

    return &_worldPacket;
}

WorldPacket const* GarrisonFollowerChangedQuality::Write()
{
    _worldPacket << OldFollower;
    _worldPacket << Follower;

    return &_worldPacket;
}

WorldPacket const* GarrisonUpdateFollower::Write()
{
    _worldPacket << uint32(Result);
    _worldPacket << Follower;

    return &_worldPacket;
}

// IDA case 4980788: u32 Result, GarrisonFollower (single follower).
// See SNIFF_AUDIT §8.43.
WorldPacket const* GarrisonRecruitFollowerResult::Write()
{
    _worldPacket << uint32(Result);
    _worldPacket << Follower;

    return &_worldPacket;
}

WorldPacket const* GarrisonOpenRecruitmentNpc::Write()
{
    _worldPacket << NpcGUID;
    _worldPacket << Size<uint32>(Followers);
    _worldPacket << Size<uint32>(AbilityCounters);
    _worldPacket << Size<uint32>(AbilityTraits);
    _worldPacket << uint32(GarrTypeID);

    for (GarrisonFollower const& follower : Followers)
        _worldPacket << follower;

    for (uint32 counter : AbilityCounters)
        _worldPacket << uint32(counter);

    for (uint32 trait : AbilityTraits)
        _worldPacket << uint32(trait);

    _worldPacket << Bits<1>(CanRecruitFollower);
    _worldPacket << Bits<1>(UnknownPurpose);
    _worldPacket.FlushBits();

    return &_worldPacket;
}

WorldPacket const* GarrisonGetClassSpecCategoryInfoResult::Write()
{
    _worldPacket << Size<uint32>(FollowerClassSpecCategoryInfos);

    for (auto const& info : FollowerClassSpecCategoryInfos)
    {
        _worldPacket << uint32(info.GarrClassSpecID);
        _worldPacket << uint32(info.GarrFollowerTypeID);
    }

    return &_worldPacket;
}

WorldPacket const* GarrisonFollowerActivationsSet::Write()
{
    _worldPacket << uint32(GarrSiteLevelID);
    _worldPacket << uint32(NumActivationsRemaining);

    return &_worldPacket;
}

// ============================================================
// Building/Utility CMSG Read implementations
// ============================================================

void UpgradeGarrison::Read()
{
    _worldPacket >> NpcGUID;
}

void GarrisonCheckUpgradeable::Read()
{
    _worldPacket >> GarrSiteID;
}

void GarrisonSetBuildingActive::Read()
{
    _worldPacket >> PlotInstanceID;
}

void GarrisonSwapBuildings::Read()
{
    _worldPacket >> NpcGUID;
    _worldPacket >> PlotInstanceID1;
    _worldPacket >> PlotInstanceID2;
}

// IDA case 4980796: 3 × u32. See SNIFF_AUDIT §8.46.
WorldPacket const* GarrisonSwapBuildingsResponse::Write()
{
    _worldPacket << uint32(Result);
    _worldPacket << uint32(PlotInstanceID1);
    _worldPacket << uint32(PlotInstanceID2);

    return &_worldPacket;
}

// ============================================================
// Talent CMSG/SMSG implementations
// ============================================================

void GarrisonLearnTalent::Read()
{
    _worldPacket >> GarrTalentID;
    _worldPacket >> Bits<1>(IsTemporary);
}

void GarrisonResearchTalent::Read()
{
    _worldPacket >> GarrTalentID;
}

void GarrisonSocketTalent::Read()
{
    _worldPacket >> GarrTalentID;
    _worldPacket >> SoulbindConduitID;
    _worldPacket >> SoulbindConduitRank;
}

// IDA case 4980750: u32 Result, u8 GarrTypeID, Bits<1>+Flush, GarrisonTalent.
// See SNIFF_AUDIT §8.11.
WorldPacket const* GarrisonResearchTalentResult::Write()
{
    _worldPacket << uint32(Result);
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << Bits<1>(UnknownBit);
    _worldPacket.FlushBits();
    _worldPacket << Talent;

    return &_worldPacket;
}

// ============================================================
// Other utility CMSG Read implementations
// ============================================================

void GarrisonRequestShipmentInfo::Read()
{
    _worldPacket >> NpcGUID;
}

WorldPacket const* GetShipmentInfoResponse::Write()
{
    _worldPacket << Bits<1>(Success);
    _worldPacket.FlushBits();
    _worldPacket << int32(ShipmentID);
    _worldPacket << int32(MaxShipments);
    _worldPacket << uint32(Shipments.size());
    _worldPacket << int32(PlotInstanceID);
    for (CharacterShipment const& shipment : Shipments)
    {
        _worldPacket << int32(shipment.ShipmentRecID);
        _worldPacket << uint64(shipment.ShipmentID);
        _worldPacket << uint64(shipment.AssignedFollowerDBID);
        _worldPacket << shipment.CreationTime;
        _worldPacket << int32(shipment.ShipmentDuration);
        _worldPacket << int32(shipment.BuildingTypeID);
        _worldPacket << int32(shipment.UnkInt32);
        _worldPacket << uint8(shipment.GarrTypeID);
    }

    return &_worldPacket;
}

void OpenShipmentNpc::Read()
{
    _worldPacket >> NpcGUID;
}

WorldPacket const* OpenShipmentNpcResult::Write()
{
    _worldPacket << NpcGUID;
    _worldPacket << uint32(CharShipmentContainerID);

    return &_worldPacket;
}

void CreateShipment::Read()
{
    _worldPacket >> NpcGUID;
    _worldPacket >> Count;
}

WorldPacket const* CreateShipmentResponse::Write()
{
    _worldPacket << uint64(ShipmentID);
    _worldPacket << uint32(ShipmentRecID);
    _worldPacket << uint32(Result);

    return &_worldPacket;
}

WorldPacket const* GetLandingPageShipmentsResponse::Write()
{
    _worldPacket << uint32(GarrTypeID);
    _worldPacket << uint32(Shipments.size());
    for (CharacterShipment const& shipment : Shipments)
    {
        _worldPacket << int32(shipment.ShipmentRecID);
        _worldPacket << uint64(shipment.ShipmentID);
        _worldPacket << uint64(shipment.AssignedFollowerDBID);
        _worldPacket << shipment.CreationTime;
        _worldPacket << int32(shipment.ShipmentDuration);
        _worldPacket << int32(shipment.BuildingTypeID);
        _worldPacket << int32(shipment.UnkInt32);
        _worldPacket << uint8(shipment.GarrTypeID);
    }

    return &_worldPacket;
}

WorldPacket const* CompleteShipmentResponse::Write()
{
    _worldPacket << uint64(ShipmentID);
    _worldPacket << uint32(Result);

    return &_worldPacket;
}

void SetUsingPartyGarrison::Read()
{
    _worldPacket >> GarrTypeID;
    _worldPacket >> Bits<1>(UsingPartyGarrison);
}

void QueryGarrisonPetName::Read()
{
    _worldPacket >> NpcGUID;
}

// ============================================================
// Trophy / Monument packet implementations
// ============================================================

void GetTrophyList::Read()
{
    _worldPacket >> TrophyTypeID;
}

WorldPacket const* GetTrophyListResponse::Write()
{
    _worldPacket << Bits<1>(Success);
    _worldPacket.FlushBits();
    _worldPacket << uint32(Trophies.size());
    for (GarrisonTrophyData const& trophy : Trophies)
    {
        _worldPacket << uint32(trophy.TrophyID);
        _worldPacket << uint32(trophy.Unk1);
    }

    return &_worldPacket;
}

void ReplaceTrophy::Read()
{
    _worldPacket >> TrophyID;
}

WorldPacket const* ReplaceTrophyResponse::Write()
{
    _worldPacket << Bits<1>(Success);
    _worldPacket.FlushBits();

    return &_worldPacket;
}

void LoadSelectedTrophy::Read()
{
    _worldPacket >> TrophyID;
}

WorldPacket const* GetSelectedTrophyIDResponse::Write()
{
    _worldPacket << uint32(TrophyID);
    _worldPacket << Bits<1>(Success);
    _worldPacket.FlushBits();

    return &_worldPacket;
}

void ChangeMonumentAppearance::Read()
{
    _worldPacket >> TrophyID;
}

WorldPacket const* GarrisonUpdateGarrisonMonumentSelections::Write()
{
    _worldPacket << uint32(Trophies.size());
    for (GarrisonTrophyData const& trophy : Trophies)
    {
        _worldPacket << uint32(trophy.TrophyID);
        _worldPacket << uint32(trophy.Unk1);
    }

    return &_worldPacket;
}

WorldPacket const* GarrisonFollowerChangedItemLevel::Write()
{
    _worldPacket << uint32(Result);
    _worldPacket << OldFollower;
    _worldPacket << Follower;

    return &_worldPacket;
}

WorldPacket const* DeleteExpiredMissionsResult::Write()
{
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << uint32(Result);
    _worldPacket << Size<uint32>(RemovedMissions);
    for (int32 missionId : RemovedMissions)
        _worldPacket << int32(missionId);

    _worldPacket << Bits<1>(Succeeded);
    _worldPacket << Bits<1>(LegionUnkBit);
    _worldPacket.FlushBits();

    return &_worldPacket;
}
}
