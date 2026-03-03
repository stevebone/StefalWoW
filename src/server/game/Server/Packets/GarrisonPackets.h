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

#ifndef TRINITYCORE_GARRISON_PACKETS_H
#define TRINITYCORE_GARRISON_PACKETS_H

#include "Packet.h"
#include "ItemPacketsCommon.h"
#include "ObjectGuid.h"
#include "Optional.h"
#include "Position.h"
#include "PacketUtilities.h"
#include <list>
#include <unordered_set>
#include <vector>

struct GarrAbilityEntry;

namespace WorldPackets
{
    namespace Garrison
    {
        class GarrisonCreateResult final : public ServerPacket
        {
        public:
            explicit GarrisonCreateResult() : ServerPacket(SMSG_GARRISON_CREATE_RESULT, 4 + 4) { }

            WorldPacket const* Write() override;

            uint32 GarrSiteLevelID = 0;
            uint32 Result = 0;
        };

        class GarrisonDeleteResult final : public ServerPacket
        {
        public:
            explicit GarrisonDeleteResult() : ServerPacket(SMSG_GARRISON_DELETE_RESULT, 4 + 4) { }

            WorldPacket const* Write() override;

            uint32 Result = 0;
            uint32 GarrSiteID = 0;
        };

        class GetGarrisonInfo final : public ClientPacket
        {
        public:
            explicit GetGarrisonInfo(WorldPacket&& packet) : ClientPacket(CMSG_GET_GARRISON_INFO, std::move(packet)) { }

            void Read() override { }
        };

        struct GarrisonPlotInfo
        {
            uint32 GarrPlotInstanceID = 0;
            TaggedPosition<Position::XYZO> PlotPos;
            uint8 PlotType = 0;
        };

        struct GarrisonBuildingInfo
        {
            Timestamp<> TimeBuilt;
            uint32 GarrPlotInstanceID = 0;
            uint32 GarrBuildingID = 0;
            uint32 CurrentGarSpecID = 0;
            Timestamp<> TimeSpecCooldown = time_t(2288912640);   // 06/07/1906 18:35:44 - another in the series of magic blizz dates
            bool Active = false;
        };

        struct GarrisonFollower
        {
            uint64 DbID = 0;
            uint32 GarrFollowerID = 0;
            uint32 Quality = 0;
            uint32 FollowerLevel = 0;
            uint32 ItemLevelWeapon = 0;
            uint32 ItemLevelArmor = 0;
            uint32 Xp = 0;
            uint32 Durability = 0;
            uint32 CurrentBuildingID = 0;
            uint32 CurrentMissionID = 0;
            std::list<GarrAbilityEntry const*> AbilityID;
            uint32 ZoneSupportSpellID = 0;
            uint32 FollowerStatus = 0;
            int32 Health = 0;
            Timestamp<> HealingTimestamp;
            int8 BoardIndex = 0;
            std::string CustomName;
        };

        struct GarrisonEncounter
        {
            int32 GarrEncounterID = 0;
            std::vector<int32> Mechanics;
            int32 GarrAutoCombatantID = 0;
            int32 Health = 0;
            int32 MaxHealth = 0;
            int32 Attack = 0;
            int8 BoardIndex = 0;
        };

        struct GarrisonMissionReward
        {
            int32 ItemID = 0;
            uint32 ItemQuantity = 0;
            int32 CurrencyID = 0;
            uint32 CurrencyQuantity = 0;
            uint32 FollowerXP = 0;
            uint32 GarrMssnBonusAbilityID = 0;
            int32 ItemFileDataID = 0;
            Optional<Item::ItemInstance> ItemInstance;
        };

        struct GarrisonMission
        {
            uint64 DbID = 0;
            int32 MissionRecID = 0;
            Timestamp<> OfferTime;
            Duration<Seconds> OfferDuration;
            Timestamp<> StartTime = time_t(2288912640);
            Duration<Seconds> TravelDuration;
            Duration<Seconds> MissionDuration;
            int32 MissionState = 0;
            int32 SuccessChance = 0;
            uint32 Flags = 0;
            float MissionScalar = 1.0f;
            int32 ContentTuningID = 0;
            std::vector<GarrisonEncounter> Encounters;
            std::vector<GarrisonMissionReward> Rewards;
            std::vector<GarrisonMissionReward> OvermaxRewards;
        };

        struct GarrisonMissionBonusAbility
        {
            uint32 GarrMssnBonusAbilityID = 0;
            Timestamp<> StartTime;
        };

        struct GarrisonTalentSocketData
        {
            int32 SoulbindConduitID = 0;
            int32 SoulbindConduitRank = 0;
        };

        struct GarrisonTalent
        {
            int32 GarrTalentID = 0;
            int32 Rank = 0;
            Timestamp<> ResearchStartTime;
            int32 Flags = 0;
            Optional<GarrisonTalentSocketData> Socket;
        };

        struct GarrisonCollectionEntry
        {
            int32 EntryID = 0;
            int32 Rank = 0;
        };

        struct GarrisonCollection
        {
            int32 Type = 0;
            std::vector<GarrisonCollectionEntry> Entries;
        };

        struct GarrisonEventEntry
        {
            int32 EntryID = 0;
            int64 EventValue = 0;
        };

        struct GarrisonEventList
        {
            int32 Type = 0;
            std::vector<GarrisonEventEntry> Events;
        };

        struct GarrisonSpecGroup
        {
            int32 ChrSpecializationID = 0;
            int32 SoulbindID = 0;
        };

        struct GarrisonInfo
        {
            uint8 GarrTypeID = 0;
            uint32 GarrSiteID = 0;
            uint32 GarrSiteLevelID = 0;
            uint32 NumFollowerActivationsRemaining = 0;
            uint32 NumMissionsStartedToday = 0;   // might mean something else, but sending 0 here enables follower abilities "Increase success chance of the first mission of the day by %."
            int32 MinAutoTroopLevel = 0;
            std::vector<GarrisonPlotInfo const*> Plots;
            std::vector<GarrisonBuildingInfo const*> Buildings;
            std::vector<GarrisonFollower const*> Followers;
            std::vector<GarrisonFollower const*> AutoTroops;
            std::vector<GarrisonMission const*> Missions;
            std::vector<std::vector<GarrisonMissionReward>> MissionRewards;
            std::vector<std::vector<GarrisonMissionReward>> MissionOvermaxRewards;
            std::vector<GarrisonMissionBonusAbility const*> MissionAreaBonuses;
            std::vector<GarrisonTalent> Talents;
            std::vector<GarrisonCollection> Collections;
            std::vector<GarrisonEventList> EventLists;
            std::vector<GarrisonSpecGroup> SpecGroups;
            std::vector<bool> CanStartMission;
            std::vector<int32> ArchivedMissions;
        };

        struct FollowerSoftCapInfo
        {
            uint8 GarrFollowerTypeID;
            uint32 Count;
        };

        class GetGarrisonInfoResult final : public ServerPacket
        {
        public:
            explicit GetGarrisonInfoResult() : ServerPacket(SMSG_GET_GARRISON_INFO_RESULT) { }

            WorldPacket const* Write() override;

            int8 FactionIndex = 0;
            std::vector<GarrisonInfo> Garrisons;
            std::vector<FollowerSoftCapInfo> FollowerSoftCaps;
        };

        struct GarrisonRemoteBuildingInfo
        {
            GarrisonRemoteBuildingInfo() : GarrPlotInstanceID(0), GarrBuildingID(0) { }
            GarrisonRemoteBuildingInfo(uint32 plotInstanceId, uint32 buildingId) : GarrPlotInstanceID(plotInstanceId), GarrBuildingID(buildingId) { }

            uint32 GarrPlotInstanceID;
            uint32 GarrBuildingID;
        };

        struct GarrisonRemoteSiteInfo
        {
            uint32 GarrSiteLevelID = 0;
            std::vector<GarrisonRemoteBuildingInfo> Buildings;
        };

        class GarrisonRemoteInfo final : public ServerPacket
        {
        public:
            explicit GarrisonRemoteInfo() : ServerPacket(SMSG_GARRISON_REMOTE_INFO) { }

            WorldPacket const* Write() override;

            std::vector<GarrisonRemoteSiteInfo> Sites;
        };

        class GarrisonPurchaseBuilding final : public ClientPacket
        {
        public:
            explicit GarrisonPurchaseBuilding(WorldPacket&& packet) : ClientPacket(CMSG_GARRISON_PURCHASE_BUILDING, std::move(packet)) { }

            void Read() override;

            ObjectGuid NpcGUID;
            uint32 BuildingID = 0;
            uint32 PlotInstanceID = 0;
        };

        class GarrisonPlaceBuildingResult final : public ServerPacket
        {
        public:
            explicit GarrisonPlaceBuildingResult() : ServerPacket(SMSG_GARRISON_PLACE_BUILDING_RESULT) { }

            WorldPacket const* Write() override;

            uint8 GarrTypeID = 0;
            uint32 Result = 0;
            GarrisonBuildingInfo BuildingInfo;
            bool PlayActivationCinematic = false;
        };

        class GarrisonCancelConstruction final : public ClientPacket
        {
        public:
            explicit GarrisonCancelConstruction(WorldPacket&& packet) : ClientPacket(CMSG_GARRISON_CANCEL_CONSTRUCTION, std::move(packet)) { }

            void Read() override;

            ObjectGuid NpcGUID;
            uint32 PlotInstanceID = 0;
        };

        class GarrisonBuildingRemoved final : public ServerPacket
        {
        public:
            explicit GarrisonBuildingRemoved() : ServerPacket(SMSG_GARRISON_BUILDING_REMOVED, 4 + 4 + 4 + 4) { }

            WorldPacket const* Write() override;

            uint8 GarrTypeID = 0;
            uint32 Result = 0;
            uint32 GarrPlotInstanceID = 0;
            uint32 GarrBuildingID = 0;
        };

        class GarrisonLearnBlueprintResult final : public ServerPacket
        {
        public:
            explicit GarrisonLearnBlueprintResult() : ServerPacket(SMSG_GARRISON_LEARN_BLUEPRINT_RESULT, 4 + 4 + 4) { }

            WorldPacket const* Write() override;

            uint8 GarrTypeID = 0;
            uint32 BuildingID = 0;
            uint32 Result = 0;
        };

        class GarrisonUnlearnBlueprintResult final : public ServerPacket
        {
        public:
            explicit GarrisonUnlearnBlueprintResult() : ServerPacket(SMSG_GARRISON_UNLEARN_BLUEPRINT_RESULT, 4 + 4) { }

            WorldPacket const* Write() override;

            uint8 GarrTypeID = 0;
            uint32 Result = 0;
            uint32 BuildingID = 0;
        };

        class GarrisonRequestBlueprintAndSpecializationData final : public ClientPacket
        {
        public:
            explicit GarrisonRequestBlueprintAndSpecializationData(WorldPacket&& packet) : ClientPacket(CMSG_GARRISON_REQUEST_BLUEPRINT_AND_SPECIALIZATION_DATA, std::move(packet)) { }

            void Read() override { }
        };

        class GarrisonRequestBlueprintAndSpecializationDataResult final : public ServerPacket
        {
        public:
            explicit GarrisonRequestBlueprintAndSpecializationDataResult() : ServerPacket(SMSG_GARRISON_REQUEST_BLUEPRINT_AND_SPECIALIZATION_DATA_RESULT, 400) { }

            WorldPacket const* Write() override;

            uint8 GarrTypeID = 0;
            std::unordered_set<uint32> const* BlueprintsKnown = nullptr;
            std::unordered_set<uint32> const* SpecializationsKnown = nullptr;
        };

        class GarrisonGetMapData final : public ClientPacket
        {
        public:
            explicit GarrisonGetMapData(WorldPacket&& packet) : ClientPacket(CMSG_GARRISON_GET_MAP_DATA, std::move(packet)) { }

            void Read() override { }
        };

        struct GarrisonBuildingMapData
        {
            GarrisonBuildingMapData() : GarrBuildingPlotInstID(0), Pos() { }
            GarrisonBuildingMapData(uint32 buildingPlotInstId, Position const& pos) : GarrBuildingPlotInstID(buildingPlotInstId), Pos(pos) { }

            uint32 GarrBuildingPlotInstID;
            TaggedPosition<Position::XYZ> Pos;
        };

        class GarrisonMapDataResponse final : public ServerPacket
        {
        public:
            explicit GarrisonMapDataResponse() : ServerPacket(SMSG_GARRISON_MAP_DATA_RESPONSE) { }

            WorldPacket const* Write() override;

            std::vector<GarrisonBuildingMapData> Buildings;
        };

        class GarrisonPlotPlaced final : public ServerPacket
        {
        public:
            explicit GarrisonPlotPlaced() : ServerPacket(SMSG_GARRISON_PLOT_PLACED) { }

            WorldPacket const* Write() override;

            uint8 GarrTypeID = 0;
            GarrisonPlotInfo* PlotInfo = nullptr;
        };

        class GarrisonPlotRemoved final : public ServerPacket
        {
        public:
            explicit GarrisonPlotRemoved() : ServerPacket(SMSG_GARRISON_PLOT_REMOVED, 4) { }

            WorldPacket const* Write() override;

            uint32 GarrPlotInstanceID = 0;
        };

        class GarrisonAddFollowerResult final : public ServerPacket
        {
        public:
            explicit GarrisonAddFollowerResult() : ServerPacket(SMSG_GARRISON_ADD_FOLLOWER_RESULT, 8 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 5 * 4 + 4) { }

            WorldPacket const* Write() override;

            uint8 GarrTypeID = 0;
            GarrisonFollower Follower;
            uint32 Result = 0;
        };

        class GarrisonRemoveFollowerResult final : public ServerPacket
        {
        public:
            explicit GarrisonRemoveFollowerResult() : ServerPacket(SMSG_GARRISON_REMOVE_FOLLOWER_RESULT, 1 + 4 + 8 + 4) { }

            WorldPacket const* Write() override;

            uint8 GarrTypeID = 0;
            uint32 Result = 0;
            uint64 FollowerDBID = 0;
            uint32 Destroyed = 0;
        };

        class GarrisonBuildingActivated final : public ServerPacket
        {
        public:
            explicit GarrisonBuildingActivated() : ServerPacket(SMSG_GARRISON_BUILDING_ACTIVATED, 4) { }

            WorldPacket const* Write() override;

            uint32 GarrPlotInstanceID = 0;
        };

        // ============================================================
        // Mission CMSG packets
        // ============================================================

        class GarrisonStartMission final : public ClientPacket
        {
        public:
            explicit GarrisonStartMission(WorldPacket&& packet) : ClientPacket(CMSG_GARRISON_START_MISSION, std::move(packet)) { }

            void Read() override;

            ObjectGuid NpcGUID;
            std::vector<uint64> FollowerDBIDs;
            uint32 MissionRecID = 0;
        };

        class GarrisonCompleteMission final : public ClientPacket
        {
        public:
            explicit GarrisonCompleteMission(WorldPacket&& packet) : ClientPacket(CMSG_GARRISON_COMPLETE_MISSION, std::move(packet)) { }

            void Read() override;

            ObjectGuid NpcGUID;
            uint32 MissionRecID = 0;
        };

        class GarrisonMissionBonusRoll final : public ClientPacket
        {
        public:
            explicit GarrisonMissionBonusRoll(WorldPacket&& packet) : ClientPacket(CMSG_GARRISON_MISSION_BONUS_ROLL, std::move(packet)) { }

            void Read() override;

            ObjectGuid NpcGUID;
            uint32 MissionRecID = 0;
        };

        class OpenMissionNpc final : public ClientPacket
        {
        public:
            explicit OpenMissionNpc(WorldPacket&& packet) : ClientPacket(CMSG_OPEN_MISSION_NPC, std::move(packet)) { }

            void Read() override;

            ObjectGuid NpcGUID;
        };

        // ============================================================
        // Mission SMSG packets
        // ============================================================

        class GarrisonStartMissionResult final : public ServerPacket
        {
        public:
            explicit GarrisonStartMissionResult() : ServerPacket(SMSG_GARRISON_START_MISSION_RESULT) { }

            WorldPacket const* Write() override;

            uint32 Result = 0;
            uint32 SessionMissionCount = 0;
            GarrisonMission Mission;
            std::vector<uint64> FollowerDBIDs;
        };

        class GarrisonCompleteMissionResult final : public ServerPacket
        {
        public:
            explicit GarrisonCompleteMissionResult() : ServerPacket(SMSG_GARRISON_COMPLETE_MISSION_RESULT) { }

            WorldPacket const* Write() override;

            uint32 Result = 0;
            GarrisonMission Mission;
            uint32 MissionRecID = 0;
            bool Succeeded = false;
        };

        class GarrisonMissionBonusRollResult final : public ServerPacket
        {
        public:
            explicit GarrisonMissionBonusRollResult() : ServerPacket(SMSG_GARRISON_MISSION_BONUS_ROLL_RESULT) { }

            WorldPacket const* Write() override;

            GarrisonMission Mission;
            uint32 MissionRecID = 0;
            uint32 Result = 0;
        };

        class GarrisonAddMissionResult final : public ServerPacket
        {
        public:
            explicit GarrisonAddMissionResult() : ServerPacket(SMSG_GARRISON_ADD_MISSION_RESULT) { }

            WorldPacket const* Write() override;

            uint8 GarrTypeID = 0;
            uint32 Result = 0;
            uint8 State = 0;
            GarrisonMission Mission;
            std::vector<GarrisonMissionReward> Rewards;
            std::vector<GarrisonMissionReward> BonusRewards;
            bool CanStartMission = true;
        };

        class GarrisonDeleteMissionResult final : public ServerPacket
        {
        public:
            explicit GarrisonDeleteMissionResult() : ServerPacket(SMSG_GARRISON_DELETE_MISSION_RESULT, 4 + 4 + 1) { }

            WorldPacket const* Write() override;

            uint32 Result = 0;
            uint32 MissionRecID = 0;
            uint8 GarrTypeID = 0;
        };

        class DeleteExpiredMissionsResult final : public ServerPacket
        {
        public:
            explicit DeleteExpiredMissionsResult() : ServerPacket(SMSG_DELETE_EXPIRED_MISSIONS_RESULT) { }

            WorldPacket const* Write() override;

            uint8 GarrTypeID = 0;
            uint32 Result = 0;
            std::vector<int32> RemovedMissions;
            bool Succeeded = true;
            bool LegionUnkBit = true;
        };

        class GarrisonMissionStartConditionUpdate final : public ServerPacket
        {
        public:
            explicit GarrisonMissionStartConditionUpdate() : ServerPacket(SMSG_GARRISON_MISSION_START_CONDITION_UPDATE) { }

            WorldPacket const* Write() override;

            std::vector<int32> MissionRecIDs;
            std::vector<bool> CanStartMission;
        };

        class GarrisonIsUpgradeableResponse final : public ServerPacket
        {
        public:
            explicit GarrisonIsUpgradeableResponse() : ServerPacket(SMSG_GARRISON_IS_UPGRADEABLE_RESPONSE, 4) { }

            WorldPacket const* Write() override;

            uint32 Result = 0;
        };

        class GarrisonUpgradeResult final : public ServerPacket
        {
        public:
            explicit GarrisonUpgradeResult() : ServerPacket(SMSG_GARRISON_UPGRADE_RESULT, 4 + 4) { }

            WorldPacket const* Write() override;

            uint32 GarrSiteLevelID = 0;
            uint32 Result = 0;
        };

        // ============================================================
        // Follower CMSG packets
        // ============================================================

        class GarrisonAssignFollowerToBuilding final : public ClientPacket
        {
        public:
            explicit GarrisonAssignFollowerToBuilding(WorldPacket&& packet) : ClientPacket(CMSG_GARRISON_ASSIGN_FOLLOWER_TO_BUILDING, std::move(packet)) { }

            void Read() override;

            ObjectGuid NpcGUID;
            uint32 PlotInstanceID = 0;
            uint64 FollowerDBID = 0;
        };

        class GarrisonRemoveFollowerFromBuilding final : public ClientPacket
        {
        public:
            explicit GarrisonRemoveFollowerFromBuilding(WorldPacket&& packet) : ClientPacket(CMSG_GARRISON_REMOVE_FOLLOWER_FROM_BUILDING, std::move(packet)) { }

            void Read() override;

            ObjectGuid NpcGUID;
            uint64 FollowerDBID = 0;
        };

        class GarrisonRemoveFollower final : public ClientPacket
        {
        public:
            explicit GarrisonRemoveFollower(WorldPacket&& packet) : ClientPacket(CMSG_GARRISON_REMOVE_FOLLOWER, std::move(packet)) { }

            void Read() override;

            ObjectGuid NpcGUID;
            uint64 FollowerDBID = 0;
        };

        class GarrisonRenameFollower final : public ClientPacket
        {
        public:
            explicit GarrisonRenameFollower(WorldPacket&& packet) : ClientPacket(CMSG_GARRISON_RENAME_FOLLOWER, std::move(packet)) { }

            void Read() override;

            uint64 FollowerDBID = 0;
            std::string FollowerName;
        };

        class GarrisonSetFollowerFavorite final : public ClientPacket
        {
        public:
            explicit GarrisonSetFollowerFavorite(WorldPacket&& packet) : ClientPacket(CMSG_GARRISON_SET_FOLLOWER_FAVORITE, std::move(packet)) { }

            void Read() override;

            uint64 FollowerDBID = 0;
            bool Favorite = false;
        };

        class GarrisonSetFollowerInactive final : public ClientPacket
        {
        public:
            explicit GarrisonSetFollowerInactive(WorldPacket&& packet) : ClientPacket(CMSG_GARRISON_SET_FOLLOWER_INACTIVE, std::move(packet)) { }

            void Read() override;

            uint64 FollowerDBID = 0;
            bool Inactive = false;
        };

        class GarrisonRecruitFollower final : public ClientPacket
        {
        public:
            explicit GarrisonRecruitFollower(WorldPacket&& packet) : ClientPacket(CMSG_GARRISON_RECRUIT_FOLLOWER, std::move(packet)) { }

            void Read() override;

            ObjectGuid NpcGUID;
            uint32 FollowerIndex = 0;
        };

        class GarrisonGenerateRecruits final : public ClientPacket
        {
        public:
            explicit GarrisonGenerateRecruits(WorldPacket&& packet) : ClientPacket(CMSG_GARRISON_GENERATE_RECRUITS, std::move(packet)) { }

            void Read() override;

            ObjectGuid NpcGUID;
            uint32 MechanicTypeID = 0;
            uint32 TraitID = 0;
        };

        class GarrisonFullyHealAllFollowers final : public ClientPacket
        {
        public:
            explicit GarrisonFullyHealAllFollowers(WorldPacket&& packet) : ClientPacket(CMSG_GARRISON_FULLY_HEAL_ALL_FOLLOWERS, std::move(packet)) { }

            void Read() override;

            ObjectGuid NpcGUID;
        };

        class GarrisonAddFollowerHealth final : public ClientPacket
        {
        public:
            explicit GarrisonAddFollowerHealth(WorldPacket&& packet) : ClientPacket(CMSG_GARRISON_ADD_FOLLOWER_HEALTH, std::move(packet)) { }

            void Read() override;

            ObjectGuid NpcGUID;
            uint64 FollowerDBID = 0;
            int32 HealthToAdd = 0;
        };

        class GarrisonGetClassSpecCategoryInfo final : public ClientPacket
        {
        public:
            explicit GarrisonGetClassSpecCategoryInfo(WorldPacket&& packet) : ClientPacket(CMSG_GARRISON_GET_CLASS_SPEC_CATEGORY_INFO, std::move(packet)) { }

            void Read() override;

            uint8 GarrFollowerTypeID = 0;
        };

        class GarrisonSetRecruitmentPreferences final : public ClientPacket
        {
        public:
            explicit GarrisonSetRecruitmentPreferences(WorldPacket&& packet) : ClientPacket(CMSG_GARRISON_SET_RECRUITMENT_PREFERENCES, std::move(packet)) { }

            void Read() override;

            ObjectGuid NpcGUID;
            uint32 AbilityID = 0;
            uint32 TraitID = 0;
        };

        // ============================================================
        // Follower SMSG packets
        // ============================================================

        class GarrisonAssignFollowerToBuildingResult final : public ServerPacket
        {
        public:
            explicit GarrisonAssignFollowerToBuildingResult() : ServerPacket(SMSG_GARRISON_ASSIGN_FOLLOWER_TO_BUILDING_RESULT, 4 + 8 + 4) { }

            WorldPacket const* Write() override;

            uint64 FollowerDBID = 0;
            uint32 Result = 0;
            uint32 PlotInstanceID = 0;
        };

        class GarrisonRemoveFollowerFromBuildingResult final : public ServerPacket
        {
        public:
            explicit GarrisonRemoveFollowerFromBuildingResult() : ServerPacket(SMSG_GARRISON_REMOVE_FOLLOWER_FROM_BUILDING_RESULT, 4 + 8) { }

            WorldPacket const* Write() override;

            uint64 FollowerDBID = 0;
            uint32 Result = 0;
        };

        class GarrisonRenameFollowerResult final : public ServerPacket
        {
        public:
            explicit GarrisonRenameFollowerResult() : ServerPacket(SMSG_GARRISON_RENAME_FOLLOWER_RESULT) { }

            WorldPacket const* Write() override;

            uint64 FollowerDBID = 0;
            uint32 Result = 0;
            std::string FollowerName;
        };

        class GarrisonFollowerChangedFlags final : public ServerPacket
        {
        public:
            explicit GarrisonFollowerChangedFlags() : ServerPacket(SMSG_GARRISON_FOLLOWER_CHANGED_FLAGS) { }

            WorldPacket const* Write() override;

            uint32 Result = 0;
            GarrisonFollower Follower;
        };

        class GarrisonFollowerChangedXP final : public ServerPacket
        {
        public:
            explicit GarrisonFollowerChangedXP() : ServerPacket(SMSG_GARRISON_FOLLOWER_CHANGED_XP) { }

            WorldPacket const* Write() override;

            uint32 Result = 0;
            uint32 TotalXp = 0;
            GarrisonFollower OldFollower;
            GarrisonFollower Follower;
        };

        class GarrisonFollowerChangedQuality final : public ServerPacket
        {
        public:
            explicit GarrisonFollowerChangedQuality() : ServerPacket(SMSG_GARRISON_FOLLOWER_CHANGED_QUALITY) { }

            WorldPacket const* Write() override;

            GarrisonFollower OldFollower;
            GarrisonFollower Follower;
        };

        class GarrisonFollowerChangedItemLevel final : public ServerPacket
        {
        public:
            explicit GarrisonFollowerChangedItemLevel() : ServerPacket(SMSG_GARRISON_FOLLOWER_CHANGED_ITEM_LEVEL) { }

            WorldPacket const* Write() override;

            uint32 Result = 0;
            GarrisonFollower OldFollower;
            GarrisonFollower Follower;
        };

        class GarrisonUpdateFollower final : public ServerPacket
        {
        public:
            explicit GarrisonUpdateFollower() : ServerPacket(SMSG_GARRISON_UPDATE_FOLLOWER) { }

            WorldPacket const* Write() override;

            uint32 Result = 0;
            GarrisonFollower Follower;
        };

        class GarrisonRecruitFollowerResult final : public ServerPacket
        {
        public:
            explicit GarrisonRecruitFollowerResult() : ServerPacket(SMSG_GARRISON_RECRUIT_FOLLOWER_RESULT) { }

            WorldPacket const* Write() override;

            uint32 Result = 0;
            std::vector<GarrisonFollower> Followers;
        };

        class GarrisonOpenRecruitmentNpc final : public ServerPacket
        {
        public:
            explicit GarrisonOpenRecruitmentNpc() : ServerPacket(SMSG_GARRISON_OPEN_RECRUITMENT_NPC) { }

            WorldPacket const* Write() override;

            ObjectGuid NpcGUID;
            std::vector<GarrisonFollower> Followers;
            std::vector<uint32> AbilityCounters;
            std::vector<uint32> AbilityTraits;
            uint32 GarrTypeID = 0;
            bool CanRecruitFollower = false;
            bool UnknownPurpose = false;
        };

        class GarrisonGetClassSpecCategoryInfoResult final : public ServerPacket
        {
        public:
            explicit GarrisonGetClassSpecCategoryInfoResult() : ServerPacket(SMSG_GARRISON_GET_CLASS_SPEC_CATEGORY_INFO_RESULT) { }

            WorldPacket const* Write() override;

            struct GarrisonFollowerCategoryInfo
            {
                uint32 GarrClassSpecID = 0;
                uint32 GarrFollowerTypeID = 0;
            };

            std::vector<GarrisonFollowerCategoryInfo> FollowerClassSpecCategoryInfos;
        };

        class GarrisonFollowerActivationsSet final : public ServerPacket
        {
        public:
            explicit GarrisonFollowerActivationsSet() : ServerPacket(SMSG_GARRISON_FOLLOWER_ACTIVATIONS_SET, 4 + 4) { }

            WorldPacket const* Write() override;

            uint32 GarrSiteLevelID = 0;
            uint32 NumActivationsRemaining = 0;
        };

        // ============================================================
        // Building/Utility CMSG packets
        // ============================================================

        class UpgradeGarrison final : public ClientPacket
        {
        public:
            explicit UpgradeGarrison(WorldPacket&& packet) : ClientPacket(CMSG_UPGRADE_GARRISON, std::move(packet)) { }

            void Read() override;

            ObjectGuid NpcGUID;
        };

        class GarrisonCheckUpgradeable final : public ClientPacket
        {
        public:
            explicit GarrisonCheckUpgradeable(WorldPacket&& packet) : ClientPacket(CMSG_GARRISON_CHECK_UPGRADEABLE, std::move(packet)) { }

            void Read() override;

            uint32 GarrSiteID = 0;
        };

        class GarrisonSetBuildingActive final : public ClientPacket
        {
        public:
            explicit GarrisonSetBuildingActive(WorldPacket&& packet) : ClientPacket(CMSG_GARRISON_SET_BUILDING_ACTIVE, std::move(packet)) { }

            void Read() override;

            uint32 PlotInstanceID = 0;
        };

        class GarrisonSwapBuildings final : public ClientPacket
        {
        public:
            explicit GarrisonSwapBuildings(WorldPacket&& packet) : ClientPacket(CMSG_GARRISON_SWAP_BUILDINGS, std::move(packet)) { }

            void Read() override;

            ObjectGuid NpcGUID;
            uint32 PlotInstanceID1 = 0;
            uint32 PlotInstanceID2 = 0;
        };

        class GarrisonSwapBuildingsResponse final : public ServerPacket
        {
        public:
            explicit GarrisonSwapBuildingsResponse() : ServerPacket(SMSG_GARRISON_SWAP_BUILDINGS_RESPONSE, 4) { }

            WorldPacket const* Write() override;

            uint32 Result = 0;
        };

        // ============================================================
        // Talent CMSG packets
        // ============================================================

        class GarrisonLearnTalent final : public ClientPacket
        {
        public:
            explicit GarrisonLearnTalent(WorldPacket&& packet) : ClientPacket(CMSG_GARRISON_LEARN_TALENT, std::move(packet)) { }

            void Read() override;

            int32 GarrTalentID = 0;
            bool IsTemporary = false;
        };

        class GarrisonResearchTalent final : public ClientPacket
        {
        public:
            explicit GarrisonResearchTalent(WorldPacket&& packet) : ClientPacket(CMSG_GARRISON_RESEARCH_TALENT, std::move(packet)) { }

            void Read() override;

            int32 GarrTalentID = 0;
        };

        class GarrisonSocketTalent final : public ClientPacket
        {
        public:
            explicit GarrisonSocketTalent(WorldPacket&& packet) : ClientPacket(CMSG_GARRISON_SOCKET_TALENT, std::move(packet)) { }

            void Read() override;

            int32 GarrTalentID = 0;
            int32 SoulbindConduitID = 0;
            int32 SoulbindConduitRank = 0;
        };

        class GarrisonResearchTalentResult final : public ServerPacket
        {
        public:
            explicit GarrisonResearchTalentResult() : ServerPacket(SMSG_GARRISON_RESEARCH_TALENT_RESULT) { }

            WorldPacket const* Write() override;

            uint32 Result = 0;
            uint8 GarrTypeID = 0;
            GarrisonTalent Talent;
        };

        // ============================================================
        // Shipment packets
        // ============================================================

        struct CharacterShipment
        {
            int32 ShipmentRecID = 0;
            uint64 ShipmentID = 0;
            uint64 AssignedFollowerDBID = 0;
            Timestamp<> CreationTime;
            int32 ShipmentDuration = 0;
            int32 BuildingTypeID = 0;
            int32 UnkInt32 = 0;
            uint8 GarrTypeID = 0;
        };

        class GarrisonRequestShipmentInfo final : public ClientPacket
        {
        public:
            explicit GarrisonRequestShipmentInfo(WorldPacket&& packet) : ClientPacket(CMSG_GARRISON_REQUEST_SHIPMENT_INFO, std::move(packet)) { }

            void Read() override;

            ObjectGuid NpcGUID;
        };

        class GetShipmentInfoResponse final : public ServerPacket
        {
        public:
            explicit GetShipmentInfoResponse() : ServerPacket(SMSG_GET_SHIPMENT_INFO_RESPONSE) { }

            WorldPacket const* Write() override;

            bool Success = false;
            int32 ShipmentID = 0;
            int32 MaxShipments = 0;
            int32 PlotInstanceID = 0;
            std::vector<CharacterShipment> Shipments;
        };

        class OpenShipmentNpc final : public ClientPacket
        {
        public:
            explicit OpenShipmentNpc(WorldPacket&& packet) : ClientPacket(CMSG_OPEN_SHIPMENT_NPC, std::move(packet)) { }

            void Read() override;

            ObjectGuid NpcGUID;
        };

        class OpenShipmentNpcResult final : public ServerPacket
        {
        public:
            explicit OpenShipmentNpcResult() : ServerPacket(SMSG_OPEN_SHIPMENT_NPC_RESULT, 16 + 4) { }

            WorldPacket const* Write() override;

            ObjectGuid NpcGUID;
            uint32 CharShipmentContainerID = 0;
        };

        class CreateShipment final : public ClientPacket
        {
        public:
            explicit CreateShipment(WorldPacket&& packet) : ClientPacket(CMSG_CREATE_SHIPMENT, std::move(packet)) { }

            void Read() override;

            ObjectGuid NpcGUID;
            uint32 Count = 1;
        };

        class CreateShipmentResponse final : public ServerPacket
        {
        public:
            explicit CreateShipmentResponse() : ServerPacket(SMSG_CREATE_SHIPMENT_RESPONSE, 8 + 4 + 4) { }

            WorldPacket const* Write() override;

            uint64 ShipmentID = 0;
            uint32 ShipmentRecID = 0;
            uint32 Result = 0;
        };

        class GetLandingPageShipments final : public ClientPacket
        {
        public:
            explicit GetLandingPageShipments(WorldPacket&& packet) : ClientPacket(CMSG_GET_LANDING_PAGE_SHIPMENTS, std::move(packet)) { }

            void Read() override { }
        };

        class GetLandingPageShipmentsResponse final : public ServerPacket
        {
        public:
            explicit GetLandingPageShipmentsResponse() : ServerPacket(SMSG_GET_LANDING_PAGE_SHIPMENTS_RESPONSE) { }

            WorldPacket const* Write() override;

            uint32 GarrTypeID = 0;
            std::vector<CharacterShipment> Shipments;
        };

        class CompleteShipmentResponse final : public ServerPacket
        {
        public:
            explicit CompleteShipmentResponse() : ServerPacket(SMSG_COMPLETE_SHIPMENT_RESPONSE, 8 + 4) { }

            WorldPacket const* Write() override;

            uint64 ShipmentID = 0;
            uint32 Result = 0;
        };

        // ============================================================
        // Other utility CMSG packets
        // ============================================================

        class SetUsingPartyGarrison final : public ClientPacket
        {
        public:
            explicit SetUsingPartyGarrison(WorldPacket&& packet) : ClientPacket(CMSG_SET_USING_PARTY_GARRISON, std::move(packet)) { }

            void Read() override;

            uint8 GarrTypeID = 0;
            bool UsingPartyGarrison = false;
        };

        class QueryGarrisonPetName final : public ClientPacket
        {
        public:
            explicit QueryGarrisonPetName(WorldPacket&& packet) : ClientPacket(CMSG_QUERY_GARRISON_PET_NAME, std::move(packet)) { }

            void Read() override;

            ObjectGuid NpcGUID;
        };

        class RequestGarrisonTalentWorldQuestUnlocks final : public ClientPacket
        {
        public:
            explicit RequestGarrisonTalentWorldQuestUnlocks(WorldPacket&& packet) : ClientPacket(CMSG_REQUEST_GARRISON_TALENT_WORLD_QUEST_UNLOCKS, std::move(packet)) { }

            void Read() override { }
        };

        class GarrisonGetMissionReward final : public ClientPacket
        {
        public:
            explicit GarrisonGetMissionReward(WorldPacket&& packet) : ClientPacket(CMSG_GARRISON_GET_MISSION_REWARD, std::move(packet)) { }

            void Read() override;

            ObjectGuid NpcGUID;
            uint32 MissionRecID = 0;
        };

        // ============================================================
        // Trophy / Monument packets
        // ============================================================

        struct GarrisonTrophyData
        {
            uint32 TrophyID = 0;
            uint32 Unk1 = 0;
        };

        class GetTrophyList final : public ClientPacket
        {
        public:
            explicit GetTrophyList(WorldPacket&& packet) : ClientPacket(CMSG_GET_TROPHY_LIST, std::move(packet)) { }

            void Read() override;

            uint32 TrophyTypeID = 0;
        };

        class GetTrophyListResponse final : public ServerPacket
        {
        public:
            explicit GetTrophyListResponse() : ServerPacket(SMSG_GET_TROPHY_LIST_RESPONSE) { }

            WorldPacket const* Write() override;

            bool Success = false;
            std::vector<GarrisonTrophyData> Trophies;
        };

        class ReplaceTrophy final : public ClientPacket
        {
        public:
            explicit ReplaceTrophy(WorldPacket&& packet) : ClientPacket(CMSG_REPLACE_TROPHY, std::move(packet)) { }

            void Read() override;

            uint32 TrophyID = 0;
        };

        class ReplaceTrophyResponse final : public ServerPacket
        {
        public:
            explicit ReplaceTrophyResponse() : ServerPacket(SMSG_REPLACE_TROPHY_RESPONSE, 1) { }

            WorldPacket const* Write() override;

            bool Success = false;
        };

        class LoadSelectedTrophy final : public ClientPacket
        {
        public:
            explicit LoadSelectedTrophy(WorldPacket&& packet) : ClientPacket(CMSG_LOAD_SELECTED_TROPHY, std::move(packet)) { }

            void Read() override;

            uint32 TrophyID = 0;
        };

        class GetSelectedTrophyIDResponse final : public ServerPacket
        {
        public:
            explicit GetSelectedTrophyIDResponse() : ServerPacket(SMSG_GET_SELECTED_TROPHY_ID_RESPONSE, 4 + 1) { }

            WorldPacket const* Write() override;

            uint32 TrophyID = 0;
            bool Success = false;
        };

        class ChangeMonumentAppearance final : public ClientPacket
        {
        public:
            explicit ChangeMonumentAppearance(WorldPacket&& packet) : ClientPacket(CMSG_CHANGE_MONUMENT_APPEARANCE, std::move(packet)) { }

            void Read() override;

            uint32 TrophyID = 0;
        };

        class RevertMonumentAppearance final : public ClientPacket
        {
        public:
            explicit RevertMonumentAppearance(WorldPacket&& packet) : ClientPacket(CMSG_REVERT_MONUMENT_APPEARANCE, std::move(packet)) { }

            void Read() override { }
        };

        class GarrisonUpdateGarrisonMonumentSelections final : public ServerPacket
        {
        public:
            explicit GarrisonUpdateGarrisonMonumentSelections() : ServerPacket(SMSG_GARRISON_UPDATE_GARRISON_MONUMENT_SELECTIONS) { }

            WorldPacket const* Write() override;

            std::vector<GarrisonTrophyData> Trophies;
        };
    }
}

#endif // TRINITYCORE_GARRISON_PACKETS_H
