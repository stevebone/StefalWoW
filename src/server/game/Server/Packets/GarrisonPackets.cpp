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
    // itemInstance = OptionalInit bit + FlushBits. This is the SNIFF-CONFIRMED encoding (ADD_MISSION_RESULT
    // decoded byte-exact / 0 trailing bytes). The talent socketData is the SAME reflection kind and must match
    // this (bit), NOT a uint32 count - see operator<<(GarrisonTalent).
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
    // Wire order (client JamGarrisonTalent, Ghidra-confirmed via ctor @client 0x729093890): garrTalentID(i32),
    // garrTalentRank(i32), researchStartTime(i64), flags(i32), socketData(OptionalInit bit + FlushBits).
    //
    // The client reads the wire `flags` field with its OWN semantics, which differ from TC's internal
    // Garrison::Talent::Flags. Bits were pinned by live client testing (the builder that computes these is
    // control-flow-flattened, so it isn't offline-decompilable):
    //   client bit 0x4 = "this talent is being researched" — gates whether the Order Advancement UI reads
    //       researchStartTime and renders the research timer. Verified live: flags=0 kept the client's startTime in
    //       memory (proven by a server-side diag log + a sentinel injection) but drew the talent idle; setting 0x4
    //       makes C_Garrison.GetTalentInfo(id).isBeingResearched=true with the real DB2 researchDuration.
    //   client bit 0x2 = a duration override that forces researchDuration to a fixed 24h; MUST NEVER be set.
    // TC's internal flags (only GARRISON_TALENT_FLAG_TEMPORARY=0x1 today, an unrelated meaning) are not
    // wire-compatible, so derive the wire value purely from research state.
    int32 wireFlags = 0;
    if (talent.ResearchStartTime != 0)
        wireFlags |= 0x4; // client "being researched" bit (never 0x2 = duration override)
    data << int32(talent.GarrTalentID);
    data << int32(talent.Rank);
    data << talent.ResearchStartTime;
    data << int32(wireFlags);
    // socketData is the SAME reflection kind (0x80003) as the reward itemInstance, which is sniff-confirmed as
    // an OptionalInit bit (NOT a uint32 count). The prior uint32 "fix" made each talent 3 bytes too long,
    // shifting the mission-reward data written right after the Talents block (rewards showed value x 2^16).
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

void GarrisonSocketTalent::Read()
{
    _worldPacket >> GarrTalentID;
    uint32 count = _worldPacket.read<uint32>();
    // Sanity cap the client-supplied socket count before resize(): a soulbind/conduit tree has at most a few dozen
    // sockets, so this is well above any legitimate value while stopping a crafted count (e.g. 0xFFFFFFFF) from
    // requesting a multi-gigabyte allocation. resize() throws std::bad_alloc, which the opcode dispatcher does not
    // catch (only ByteBufferException), so an unbounded count would crash the world thread instead of disconnecting.
    count = std::min<uint32>(count, 64);
    Sockets.resize(count);
    for (GarrisonTalentSocketData& socket : Sockets)
    {
        _worldPacket >> socket.SoulbindConduitID;
        _worldPacket >> socket.SoulbindConduitRank;
    }
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

// Conservative shape: u32 Result, u32 GarrSpecID, u32 GarrPlotInstanceID.
WorldPacket const* GarrisonLearnSpecializationResult::Write()
{
    _worldPacket << uint32(Result);
    _worldPacket << uint32(GarrSpecID);
    _worldPacket << uint32(GarrPlotInstanceID);

    return &_worldPacket;
}

// Conservative shape: u32 Result, u32 GarrPlotInstanceID, u32 GarrSpecID.
WorldPacket const* GarrisonBuildingSetActiveSpecializationResult::Write()
{
    _worldPacket << uint32(Result);
    _worldPacket << uint32(GarrPlotInstanceID);
    _worldPacket << uint32(GarrSpecID);

    return &_worldPacket;
}

// IDA case 4980797 (§8.47): u32 Result, u64 BuildingDbID, u32 GarrPlotInstanceID.
WorldPacket const* GarrisonCompleteBuildingConstructionResult::Write()
{
    _worldPacket << uint32(Result);
    _worldPacket << uint64(BuildingDbID);
    _worldPacket << uint32(GarrPlotInstanceID);

    return &_worldPacket;
}

// IDA case 4980791 (§8.45): PackedGuid + sub-call + varU32 size + varU32[size].
WorldPacket const* GarrisonOpenCrafter::Write()
{
    _worldPacket << NpcGUID;
    _worldPacket << uint32(GarrTypeID);
    _worldPacket << uint32(CraftableItemIDs.size());
    for (uint32 itemID : CraftableItemIDs)
        _worldPacket << uint32(itemID);

    return &_worldPacket;
}

// IDA case 4980817 (§8.51): generic byte-block helper. Conservative: u32 NewMinLevel.
WorldPacket const* GarrisonAutoTroopMinLevelUpdateResult::Write()
{
    _worldPacket << uint32(NewMinLevel);

    return &_worldPacket;
}

// IDA case 4980772 (§8.30): u8 GarrTypeID + GarrisonSmallStruct.
// Conservative inner shape: {u32 MissionRecID, u32 BonusAbilityID}.
WorldPacket const* GarrisonActivateMissionBonusAbility::Write()
{
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << uint32(MissionRecID);
    _worldPacket << uint32(BonusAbilityID);

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

    // Cap before resize(): a client-controlled count can't legitimately exceed the packet's own byte size (each
    // element is >= 1 byte), so this bounds the allocation to a few KB. resize() throws std::bad_alloc, which the
    // opcode dispatcher does NOT catch (only ByteBufferException) -> an uncapped count crashes the world thread.
    followerCount = std::min<uint32>(followerCount, _worldPacket.size());
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
    // Trailing uint8 (GarrFollowerTypeID) is intentionally left unread — see GarrisonPackets.h.
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

// IDA-confirmed (12.0.5.67186) layout — see SNIFF_AUDIT §10.1.
// 7 CONFIRMED + 3 HIGH + 2 LOW fields (the LOW are field NAMES inside the 32-byte
// FollowerInfo sub-struct; the wire SHAPE is fully locked in).
WorldPacket const* GarrisonCompleteMissionResult::Write()
{
    _worldPacket << uint32(Result);
    _worldPacket << uint32(MissionRecID);
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << uint32(FollowerInfos.size());
    _worldPacket << uint32(Rounds.size());

    for (GarrisonCompleteMissionFollowerInfo const& info : FollowerInfos)
    {
        _worldPacket << uint64(info.DbID);
        _worldPacket << uint32(info.Health);
        _worldPacket << uint64(info.HealingTimestamp);
        _worldPacket << uint32(info.State);
    }

    _worldPacket << Mission;

    // Single byte where the client reads bit7 as Succeeded and bit6 as OvermaxSucceeded.
    // Written directly because TC's Bits<1> writer is LSB-first while the client reads
    // MSB-first (see SNIFF_AUDIT §10.1.8 bit-pack note).
    _worldPacket << uint8((Succeeded ? 0x80u : 0u) | (OvermaxSucceeded ? 0x40u : 0u));

    for (GarrisonAutoMissionRound const& round : Rounds)
    {
        _worldPacket << uint32(round.Events.size());
        for (GarrisonAutoMissionEvent const& evt : round.Events)
        {
            _worldPacket << uint32(evt.Type);
            _worldPacket << uint32(evt.SpellID);
            _worldPacket << uint32(evt.SchoolMask);
            _worldPacket << uint8(evt.EffectIndex);
            _worldPacket << uint32(evt.CasterBoardIndex);
            _worldPacket << uint32(evt.AuraType);
            _worldPacket << uint32(evt.TargetInfo.size());
            for (GarrisonAutoMissionTargetInfo const& tgt : evt.TargetInfo)
            {
                _worldPacket << uint32(tgt.BoardIndex);
                _worldPacket << uint32(tgt.OldHealth);
                _worldPacket << uint32(tgt.NewHealth);
                _worldPacket << uint32(tgt.MaxHealth);
                // bit7 set when Points is present; the client reads (n & 0x80) >> 7 then
                // gates the optional u32 read on that bit.
                _worldPacket << uint8(tgt.Points.has_value() ? 0x80u : 0u);
                if (tgt.Points)
                    _worldPacket << uint32(*tgt.Points);
            }
        }
    }

    return &_worldPacket;
}

// IDA-confirmed (12.0.5.67186) layout — see SNIFF_AUDIT §10.2. 3 CONFIRMED + 2 HIGH
// fields. FollowerInfo struct shape matches COMPLETE_MISSION_RESULT (§10.1.3).
WorldPacket const* GarrisonMissionBonusRollResult::Write()
{
    _worldPacket << Mission;
    _worldPacket << uint32(MissionRecID);
    _worldPacket << uint32(Result);
    _worldPacket << uint32(FollowerInfos.size());

    for (GarrisonCompleteMissionFollowerInfo const& info : FollowerInfos)
    {
        _worldPacket << uint64(info.DbID);
        _worldPacket << uint32(info.Health);
        _worldPacket << uint64(info.HealingTimestamp);
        _worldPacket << uint32(info.State);
    }

    // Wire reads bit7 → Succeeded. The remaining 7 low bits are unused.
    _worldPacket << uint8(Succeeded ? 0x80u : 0u);

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

// IDA case 4980765 (§8.25): u32 Result, u32 MissionRecID, GarrisonMission.
WorldPacket const* GarrisonChangeMissionStartTimeResult::Write()
{
    _worldPacket << uint32(Result);
    _worldPacket << uint32(MissionRecID);
    _worldPacket << Mission;

    return &_worldPacket;
}

// IDA case 4980766 (§8.26): u32 Result, u64 LastUsedTimestamp.
WorldPacket const* GarrisonGetRecallPortalLastUsedTimeResult::Write()
{
    _worldPacket << uint32(Result);
    _worldPacket << uint64(LastUsedTimestamp);

    return &_worldPacket;
}

// IDA case 4980767 (§8.27): u32 Result, u32 MissionRecID, u64 RecallTimestamp, GarrisonMission.
WorldPacket const* GarrisonUseRecallPortalResult::Write()
{
    _worldPacket << uint32(Result);
    _worldPacket << uint32(MissionRecID);
    _worldPacket << uint64(RecallTimestamp);
    _worldPacket << Mission;

    return &_worldPacket;
}

// IDA case 4980803 (§8.53): u32 Result, u64 ItemDbID, u32 size, u32 size,
// GarrisonMissionReward[size], GarrisonMissionReward[size].
WorldPacket const* GarrisonMissionRequestRewardInfoResponse::Write()
{
    _worldPacket << uint32(Result);
    _worldPacket << uint64(ItemDbID);
    _worldPacket << uint32(Rewards.size());
    _worldPacket << uint32(OvermaxRewards.size());
    for (GarrisonMissionReward const& reward : Rewards)
        _worldPacket << reward;
    for (GarrisonMissionReward const& reward : OvermaxRewards)
        _worldPacket << reward;

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

// IDA-confirmed (12.0.5.67186) — see SNIFF_AUDIT §10.3.
// All 6 fields have two-source evidence (IDA pseudocode + C_Garrison Lua accessors:
// CanGenerateRecruits, CanSetRecruitmentPreference, GetAvailableRecruits,
// SetRecruitmentPreferences). The previous {AbilityCounters, AbilityTraits, GarrTypeID,
// UnknownPurpose} fields had NO wire counterpart and have been removed.
WorldPacket const* GarrisonOpenRecruitmentNpc::Write()
{
    _worldPacket << NpcGUID;
    _worldPacket << uint32(MechanicTypeID);
    _worldPacket << uint32(TraitID);

    for (GarrisonFollower const& follower : Followers)
        _worldPacket << follower;

    // Wire format: bit7 = CanGenerateRecruits, bit6 = CanSetRecruitmentPreference.
    // The remaining 6 low bits are unused; the client masks them off. Written as a
    // direct byte rather than via Bits<1> because TC's Bits<1> writer accumulates
    // LSB-first while the client reads bit7-first.
    _worldPacket << uint8((CanGenerateRecruits ? 0x80u : 0u) | (CanSetRecruitmentPreference ? 0x40u : 0u));

    return &_worldPacket;
}

// IDA case 4980778 (§8.35): u8 GarrTypeID, u32 Result.
WorldPacket const* GarrisonFollowerFatigueCleared::Write()
{
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << uint32(Result);

    return &_worldPacket;
}

// IDA case 4980783 (§8.40): full GarrisonFollower.
WorldPacket const* GarrisonRemoveFollowerAbilityResult::Write()
{
    _worldPacket << Follower;

    return &_worldPacket;
}

// IDA case 4980787 (§8.42): exactly 3 inline GarrisonFollowers (no count prefix).
WorldPacket const* GarrisonGenerateFollowersResult::Write()
{
    for (GarrisonFollower const& follower : Followers)
        _worldPacket << follower;

    return &_worldPacket;
}

// IDA case 4980761 (§8.22): u32 size, GarrisonFollower[size].
WorldPacket const* GarrisonListFollowersCheatResult::Write()
{
    _worldPacket << uint32(Followers.size());
    for (GarrisonFollower const& follower : Followers)
        _worldPacket << follower;

    return &_worldPacket;
}

// IDA case 4980800 (§8.50): u32 size, u32[size].
WorldPacket const* GarrisonListCompletedMissionsCheatResult::Write()
{
    _worldPacket << uint32(MissionRecIDs.size());
    for (uint32 missionRecID : MissionRecIDs)
        _worldPacket << uint32(missionRecID);

    return &_worldPacket;
}

// IDA case 4980816 (§8.62-66): u32 Result, u32 MissionRecID, u32 NewState, GarrisonMission.
WorldPacket const* GarrisonUpdateMissionCheatResult::Write()
{
    _worldPacket << uint32(Result);
    _worldPacket << uint32(MissionRecID);
    _worldPacket << uint32(NewState);
    _worldPacket << Mission;

    return &_worldPacket;
}

// ============================================================
// Collection / event-list / spec-group SMSG implementations (§8.54-8.61)
// ============================================================

WorldPacket const* GarrisonCollectionUpdateEntry::Write()
{
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << uint8(CollectionEntryFlags);
    _worldPacket << uint32(GarrTalentID);
    _worldPacket << Socket;

    return &_worldPacket;
}

WorldPacket const* GarrisonCollectionRemoveEntry::Write()
{
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << uint32(CollectionType);
    _worldPacket << uint32(GarrTalentID);

    return &_worldPacket;
}

WorldPacket const* GarrisonClearCollection::Write()
{
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << uint32(CollectionType);

    return &_worldPacket;
}

WorldPacket const* GarrisonAddEvent::Write()
{
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << uint32(EventListID);
    _worldPacket << uint64(Timestamp);
    _worldPacket << uint32(EventValue);

    return &_worldPacket;
}

WorldPacket const* GarrisonRemoveEvent::Write()
{
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << uint32(EventListID);
    _worldPacket << uint32(EventValue);

    return &_worldPacket;
}

WorldPacket const* GarrisonClearEventList::Write()
{
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << uint32(EventListID);

    return &_worldPacket;
}

WorldPacket const* GarrisonAddSpecGroups::Write()
{
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << uint32(SpecGroups.size());
    for (GarrisonSpecGroup const& specGroup : SpecGroups)
    {
        _worldPacket << uint32(specGroup.GarrSpecGroupID);
        _worldPacket << uint32(specGroup.SelectedTalentTreeID);
    }

    return &_worldPacket;
}

WorldPacket const* GarrisonClearSpecGroups::Write()
{
    _worldPacket << uint8(GarrTypeID);

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
    _worldPacket >> NpcGUID;
    _worldPacket >> GarrTalentID;
    _worldPacket >> GarrTalentRank;
    _worldPacket >> Bits<1>(Unused);
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

// IDA case 4980751: u8, u32, u32, u32. See SNIFF_AUDIT §8.12.
WorldPacket const* GarrisonTalentCompleted::Write()
{
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << uint32(GarrTalentID);
    _worldPacket << uint32(Rank);
    _worldPacket << uint32(ResearchStartTime);

    return &_worldPacket;
}

// IDA case 4980752: u8, u32. See SNIFF_AUDIT §8.13.
WorldPacket const* GarrisonTalentRemoved::Write()
{
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << uint32(GarrTalentID);

    return &_worldPacket;
}

// IDA case 4980753: u8, u32, optional<TalentSocket> (top-bit-gated). See SNIFF_AUDIT §8.14.
WorldPacket const* GarrisonTalentUpdateSocketData::Write()
{
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << uint32(GarrTalentID);
    _worldPacket << OptionalInit(Socket);
    _worldPacket.FlushBits();
    if (Socket)
        _worldPacket << *Socket;

    return &_worldPacket;
}

// IDA case 4980754: u8, u32. See SNIFF_AUDIT §8.15.
WorldPacket const* GarrisonTalentRemoveSocketData::Write()
{
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << uint32(GarrTalentID);

    return &_worldPacket;
}

// IDA case 4980755: u8, u32. See SNIFF_AUDIT §8.16.
WorldPacket const* GarrisonResetTalentTree::Write()
{
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << uint32(GarrTalentTreeID);

    return &_worldPacket;
}

// IDA case 4980756: u8, u32. See SNIFF_AUDIT §8.17.
WorldPacket const* GarrisonResetTalentTreeSocketData::Write()
{
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << uint32(GarrTalentTreeID);

    return &_worldPacket;
}

// IDA case 4980813: u8 GarrTypeID, u32 size, GarrisonTalent[size]. See SNIFF_AUDIT §8.62.
WorldPacket const* GarrisonSwitchTalentTreeBranch::Write()
{
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << uint32(Talents.size());
    for (GarrisonTalent const& talent : Talents)
        _worldPacket << talent;

    return &_worldPacket;
}

// IDA case 4980814: opaque (helper). Conservative shape: u8 GarrTypeID + size-prefixed
// list of unlocked talent tree IDs. See SNIFF_AUDIT §8.63.
WorldPacket const* GarrisonTalentWorldQuestUnlocksResponse::Write()
{
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << uint32(UnlockedTalentTreeIDs.size());
    for (int32 treeID : UnlockedTalentTreeIDs)
        _worldPacket << int32(treeID);

    return &_worldPacket;
}

// IDA case 4980815: opaque (helper). Conservative shape: u8 GarrTypeID + size-prefixed
// {GarrTalentID, Socket} pairs. See SNIFF_AUDIT §8.64.
WorldPacket const* GarrisonApplyTalentSocketDataChanges::Write()
{
    _worldPacket << uint8(GarrTypeID);
    _worldPacket << uint32(Changes.size());
    for (TalentSocketChange const& change : Changes)
    {
        _worldPacket << uint32(change.GarrTalentID);
        _worldPacket << change.Socket;
    }

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
        _worldPacket << uint32(shipment.ContainerID);   // sniff-decoded: sits between AssignedFollowerDBID and CreationTime.
                                                        // Omitting it made every shipment 4 bytes short; with >=2 pending
                                                        // the shortfall accumulated and the client read the last shipment's
                                                        // duration past the buffer end -> nil -> SecondsToTime(nil) error.
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
    _worldPacket.WriteBit(Success);
    _worldPacket.FlushBits();
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
        // Wire layout decoded from the 12.0.7 client binary (per-shipment reader RVA 0x6BCE20 reads
        // exactly u32, u64, u64, u64, u64, u8 = 37 bytes). The client's C_Garrison.GetLandingPageShipmentInfo
        // builds creationTime from field D's low dword ORed with field E's HIGH dword, so ANY non-zero value
        // in field E's high 32 bits corrupts the cooldown (SetCooldownUNIX out of range). There is NO
        // separate duration on the wire - the landing page reads duration from the client's CharShipment.db2.
        // Field E is therefore BuildingType as a full u64 (high dword must be zero):
        //   RecID(u32) ShipmentID(u64) AssignedFollowerDBID(u64) CreationTime(i64) BuildingType(u64) GarrType(u8)
        _worldPacket << uint32(shipment.ShipmentRecID);
        _worldPacket << uint64(shipment.ShipmentID);
        _worldPacket << uint64(shipment.AssignedFollowerDBID);
        _worldPacket << shipment.CreationTime;
        _worldPacket << uint64(uint32(shipment.BuildingTypeID));
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

// IDA case 4980801 (§8.51 pet name): {ObjectGuid NpcGUID, 7-bit-prefixed string PetName}.
WorldPacket const* QueryGarrisonPetNameResponse::Write()
{
    _worldPacket << NpcGUID;
    _worldPacket << SizedString::BitsSize<7>(PetName);
    _worldPacket.FlushBits();
    _worldPacket << SizedString::Data(PetName);

    return &_worldPacket;
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
