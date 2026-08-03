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
#include <array>
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

        // Wire shape: 2 * int32. The 68275 reflection descriptors (GARRISON_WIRE_NAMED_68275.md)
        // resolve this: JamGarrisonTalentConduitPair = {garrTalentID@0, soulbindConduitID@4},
        // confirming the socket payload carries a soulbind conduit id (not a bare socket index).
        // TC's SoulbindConduitID naming stands.
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

        // JamGarrisonCompleteMissionFollowerInfo per c:/dumps/AGENT_BRIEF_GARRISON.md (Deserialize_CompleteMissionFollowerInfo @ 0x7FF75C175900)
        struct GarrisonMissionEndingFollower
        {
            uint64 DbID = 0;
            int32 Health = 0;
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

        // CMSG_GARRISON_SOCKET_TALENT (client serializer sub_7FF72914B630): { u32 GarrTalentID, u32 count,
        // count x { int32 SoulbindConduitID, int32 SoulbindConduitRank } }. The leading id and the pair fields are
        // read from the wire as-is; the pair layout mirrors GarrisonTalentSocketData (this protocol's socket record).
        // NEEDS-CONFIRM (sniff): whether the leading id is the node or the tree, and pair order. Handler fails closed
        // (unowned/invalid conduit ids no-op), so a misread cannot corrupt server state.
        class GarrisonSocketTalent final : public ClientPacket
        {
        public:
            explicit GarrisonSocketTalent(WorldPacket&& packet) : ClientPacket(CMSG_GARRISON_SOCKET_TALENT, std::move(packet)) { }

            void Read() override;

            int32 GarrTalentID = 0;
            std::vector<GarrisonTalentSocketData> Sockets;
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

        // Conservative shape (no IDA byte detail in audit): {u32 Result, u32 GarrSpecID,
        // u32 GarrPlotInstanceID}. Sent when a specialization is learned for a building.
        class GarrisonLearnSpecializationResult final : public ServerPacket
        {
        public:
            explicit GarrisonLearnSpecializationResult() : ServerPacket(SMSG_GARRISON_LEARN_SPECIALIZATION_RESULT, 4 + 4 + 4) { }

            WorldPacket const* Write() override;

            uint32 Result = 0;
            uint32 GarrSpecID = 0;
            uint32 GarrPlotInstanceID = 0;
        };

        // Conservative shape: {u32 Result, u32 GarrPlotInstanceID, u32 GarrSpecID}.
        // Sent when a building's active specialization is changed.
        class GarrisonBuildingSetActiveSpecializationResult final : public ServerPacket
        {
        public:
            explicit GarrisonBuildingSetActiveSpecializationResult() : ServerPacket(SMSG_GARRISON_BUILDING_SET_ACTIVE_SPECIALIZATION_RESULT, 4 + 4 + 4) { }

            WorldPacket const* Write() override;

            uint32 Result = 0;
            uint32 GarrPlotInstanceID = 0;
            uint32 GarrSpecID = 0;
        };

        // IDA case 4980797 (§8.47): u32 Result, u64 BuildingDbID, u32 GarrPlotInstanceID.
        // Sent when a building finishes construction (timer expires, becomes activatable).
        class GarrisonCompleteBuildingConstructionResult final : public ServerPacket
        {
        public:
            explicit GarrisonCompleteBuildingConstructionResult() : ServerPacket(SMSG_GARRISON_COMPLETE_BUILDING_CONSTRUCTION_RESULT, 4 + 8 + 4) { }

            WorldPacket const* Write() override;

            uint32 Result = 0;
            uint64 BuildingDbID = 0;
            uint32 GarrPlotInstanceID = 0;
        };

        // IDA case 4980791 (§8.45): PackedGuid NpcGUID, sub-call (likely a small descriptor),
        // varU32 size, varU32[size]. Conservative interpretation: {NpcGUID, u32 GarrTypeID,
        // u32[] CraftableItemIDs}.
        class GarrisonOpenCrafter final : public ServerPacket
        {
        public:
            explicit GarrisonOpenCrafter() : ServerPacket(SMSG_GARRISON_OPEN_CRAFTER) { }

            WorldPacket const* Write() override;

            ObjectGuid NpcGUID;
            uint32 GarrTypeID = 0;
            std::vector<uint32> CraftableItemIDs;
        };

        // Opens the class-hall Order Advancement talent tree. Fires GARRISON_TALENT_NPC_OPENED client-side ->
        // OrderHallTalentFrame:SetGarrisonType(garrType, tree). Opcode recovered from the client binary; wire is a
        // reconstruction (NpcGUID, tree, garrType) - verify against a sniff.
        class GarrisonOpenTalentNpc final : public ServerPacket
        {
        public:
            explicit GarrisonOpenTalentNpc() : ServerPacket(SMSG_GARRISON_OPEN_TALENT_NPC, 16 + 4 + 4) { }

            WorldPacket const* Write() override;

            ObjectGuid NpcGUID;
            int32 GarrTalentTreeID = 0;
            int32 GarrTypeID = 0;
        };

        // IDA case 4980817 (§8.51): generic byte-block helper. Conservative shape: {u32 NewMinLevel}.
        class GarrisonAutoTroopMinLevelUpdateResult final : public ServerPacket
        {
        public:
            explicit GarrisonAutoTroopMinLevelUpdateResult() : ServerPacket(SMSG_GARRISON_AUTO_TROOP_MIN_LEVEL_UPDATE_RESULT, 4) { }

            WorldPacket const* Write() override;

            uint32 NewMinLevel = 0;
        };

        // IDA case 4980772 (§8.30): u8 GarrTypeID + GarrisonSmallStruct (likely
        // {u32 MissionRecID, u32 BonusAbilityID}). Sent when a mission bonus ability activates.
        class GarrisonActivateMissionBonusAbility final : public ServerPacket
        {
        public:
            explicit GarrisonActivateMissionBonusAbility() : ServerPacket(SMSG_GARRISON_ACTIVATE_MISSION_BONUS_ABILITY, 1 + 4 + 4) { }

            WorldPacket const* Write() override;

            uint8 GarrTypeID = 0;
            uint32 MissionRecID = 0;
            uint32 BonusAbilityID = 0;
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
            // NOTE: the 68275 client appends a trailing uint8 (GarrFollowerTypeID) after the PackedGuid,
            // producing a harmless "read stop at 14 from 15" tail warning. We intentionally do NOT read it:
            // the handler ignores the packet entirely, and adding a field here changed the packet object's
            // size, which — against a stale opcode-table wrapper — placed the field write on the stack GS
            // cookie and hard-crashed (FAST_FAIL_STACK_COOKIE_CHECK). Leave the field out.
        };

        // ============================================================
        // Mission SMSG packets
        // ============================================================

        // Per-follower entry — IDA-confirmed 17-byte (or 21-byte) tuple. The first 17 bytes
        // mirror the CMSG_GARRISON_START_MISSION shape; if HasFollowerEntry is set the wire
        // appends an additional u32 FollowerEntry. See SNIFF_AUDIT_12.0.1.66102.md §8.1.
        struct GarrisonMissionFollowerEntry
        {
            uint64 DbID = 0;
            int32 BoardIndex = -1;
            int32 Health = 0;
            uint8 HasFollowerEntry = 0;
            uint32 FollowerEntry = 0;
        };

        class GarrisonStartMissionResult final : public ServerPacket
        {
        public:
            explicit GarrisonStartMissionResult() : ServerPacket(SMSG_GARRISON_START_MISSION_RESULT) { }

            WorldPacket const* Write() override;

            // IDA-confirmed (12.0.5.67186) + sniff-verified (12.0.1.66102) layout:
            //   u32 Result, u16 NumOfferedToday, u32 FollowerInfoCount, u32 FollowersCount,
            //   GarrisonMission Mission, FollowerInfo[FollowerInfoCount], GarrisonFollower[FollowersCount].
            // See SNIFF_AUDIT_12.0.1.66102.md §8.1 for the byte-by-byte trace.
            uint32 Result = 0;
            uint16 NumOfferedToday = 0;
            GarrisonMission Mission;
            std::vector<GarrisonMissionFollowerEntry> FollowerInfos;
            std::vector<GarrisonFollower> Followers;
        };

        // 32-byte per-follower complete-mission record — IDA-confirmed
        // (12.0.5.67186) via Deserialize_CompleteMissionFollowerInfo @ 0x7FF75C175900.
        // See SNIFF_AUDIT §10.1.3 + §11 (deepest-pass field-naming).
        //
        // Wire format:
        //     varint64  DbID              CONFIRMED — follower DbID
        //     varU32    Health            CONFIRMED — HP at end of auto-combat
        //     varint64  HealingTimestamp  CONFIRMED — 68275 reflection descriptor
        //                                            (GARRISON_WIRE_NAMED_68275.md):
        //                                            JamGarrisonCompleteMissionFollowerInfo =
        //                                            followerDBID@0, newHealth@8,
        //                                            healingTimestamp@16, missionCompleteState@24.
        //     varU32    State             CONFIRMED — GarrFollowerMissionCompleteState enum:
        //                                            0=Alive, 1=KilledByMissionFailure,
        //                                            2=SavedByPreventDeath, 3=OutOfDurability.
        struct GarrisonCompleteMissionFollowerInfo
        {
            uint64 DbID = 0;
            uint32 Health = 0;
            uint64 HealingTimestamp = 0; // UnixTime; mirrors GarrisonFollower.HealingTimestamp
            uint32 State = 0;            // GarrFollowerMissionCompleteState
        };

        // 24-byte per-target record produced by one auto-combat event. IDA-confirmed
        // via the inner combatant loop in sub_7FF75C1750F0 + Lua C-binding accessor
        // sub_7FF75CB37FF0 — see SNIFF_AUDIT §10.1.6. All 6 fields CONFIRMED via
        // matching Lua table keys ("boardIndex", "oldHealth", "newHealth", "maxHealth",
        // "points").
        struct GarrisonAutoMissionTargetInfo
        {
            uint32 BoardIndex = 0;
            uint32 OldHealth = 0;
            uint32 NewHealth = 0;
            uint32 MaxHealth = 0;
            Optional<uint32> Points;     // wire: u8 HasPoints + optional u32 Points
        };

        // 48-byte per-event auto-combat record (one spell cast / aura tick / ...).
        // IDA-confirmed via Deserialize @ 0x7FF75C1750F0 + Lua C-binding accessor
        // sub_7FF75CB38250 — see SNIFF_AUDIT §10.1.5. All 7 fields CONFIRMED via
        // matching Lua table keys ("type", "spellID", "schoolMask", "effectIndex",
        // "casterBoardIndex", "auraType", "targetInfo").
        struct GarrisonAutoMissionEvent
        {
            uint32 Type = 0;             // GarrAutoMissionEventType enum
            uint32 SpellID = 0;
            uint32 SchoolMask = 0;
            uint8  EffectIndex = 0;      // wire is u8, in-memory u32 (low byte only)
            uint32 CasterBoardIndex = 0;
            uint32 AuraType = 0;
            std::vector<GarrisonAutoMissionTargetInfo> TargetInfo;
        };

        // 24-byte pure-container round struct — IDA-confirmed via the cleanup symbol
        // string "struct JamGarrisonAutoMissionRoundInfo" — see SNIFF_AUDIT §10.1.4.
        // The Lua API only exposes "events"; no scalar fields. The round index is
        // implicit by array position.
        struct GarrisonAutoMissionRound
        {
            std::vector<GarrisonAutoMissionEvent> Events;
        };

        // IDA-confirmed (12.0.5.67186) layout — see SNIFF_AUDIT §10.1.
        //   u32 Result, u32 MissionRecID, u8 GarrTypeID,
        //   u32 FollowerInfoCount, u32 RoundsCount,
        //   FollowerInfo[FollowerInfoCount]  (32 bytes each),
        //   GarrisonMission Mission,
        //   single byte (bit7=Succeeded, bit6=OvermaxSucceeded, 6 padding bits),
        //   Round[RoundsCount].
        class GarrisonCompleteMissionResult final : public ServerPacket
        {
        public:
            explicit GarrisonCompleteMissionResult() : ServerPacket(SMSG_GARRISON_COMPLETE_MISSION_RESULT) { }

            WorldPacket const* Write() override;

            uint32 Result = 0;
            uint32 MissionRecID = 0;
            uint8 GarrTypeID = 0;
            GarrisonMission Mission;
            bool Succeeded = false;
            bool OvermaxSucceeded = false;
            std::vector<GarrisonCompleteMissionFollowerInfo> FollowerInfos;
            std::vector<GarrisonAutoMissionRound> Rounds;
        };

        // IDA-confirmed (12.0.5.67186) layout — see SNIFF_AUDIT §10.2.
        //   GarrisonMission Mission, u32 MissionRecID, u32 Result,
        //   u32 FollowerInfoCount, FollowerInfo[FollowerInfoCount]   (32 bytes each),
        //   single byte (bit7=Succeeded, 7 padding bits).
        // Same FollowerInfo struct as COMPLETE_MISSION_RESULT (§10.1.3).
        class GarrisonMissionBonusRollResult final : public ServerPacket
        {
        public:
            explicit GarrisonMissionBonusRollResult() : ServerPacket(SMSG_GARRISON_MISSION_BONUS_ROLL_RESULT) { }

            WorldPacket const* Write() override;

            GarrisonMission Mission;
            uint32 MissionRecID = 0;
            uint32 Result = 0;
            bool Succeeded = false;
            std::vector<GarrisonCompleteMissionFollowerInfo> FollowerInfos;
        };

        // IDA case 4980762: u8 GarrTypeID, u32 Result, u8 State, Bits<1>, GarrisonMission.
        // The Mission carries its own Rewards/OvermaxRewards inline, so the wire has no
        // outer reward arrays. See SNIFF_AUDIT §8.23.
        class GarrisonAddMissionResult final : public ServerPacket
        {
        public:
            explicit GarrisonAddMissionResult() : ServerPacket(SMSG_GARRISON_ADD_MISSION_RESULT) { }

            WorldPacket const* Write() override;

            uint8 GarrTypeID = 0;
            uint32 Result = 0;
            uint8 State = 0;
            bool CanStartMission = true;
            GarrisonMission Mission;
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

        // IDA case 4980765 (§8.25): u32 Result, u32 MissionRecID, GarrisonMission.
        // Sent when a mission's start time is moved (e.g., Master Plan reduces wait).
        class GarrisonChangeMissionStartTimeResult final : public ServerPacket
        {
        public:
            explicit GarrisonChangeMissionStartTimeResult() : ServerPacket(SMSG_GARRISON_CHANGE_MISSION_START_TIME_RESULT) { }

            WorldPacket const* Write() override;

            uint32 Result = 0;
            uint32 MissionRecID = 0;
            GarrisonMission Mission;
        };

        // IDA case 4980766 (§8.26): u32 Result, u64 LastUsedTimestamp.
        class GarrisonGetRecallPortalLastUsedTimeResult final : public ServerPacket
        {
        public:
            explicit GarrisonGetRecallPortalLastUsedTimeResult() : ServerPacket(SMSG_GARRISON_GET_RECALL_PORTAL_LAST_USED_TIME_RESULT, 4 + 8) { }

            WorldPacket const* Write() override;

            uint32 Result = 0;
            uint64 LastUsedTimestamp = 0;
        };

        // IDA case 4980767 (§8.27): u32 Result, u32 ?, u64 ?, GarrisonMission.
        // Conservative interpretation: {Result, MissionRecID, RecallTimestamp, Mission}.
        class GarrisonUseRecallPortalResult final : public ServerPacket
        {
        public:
            explicit GarrisonUseRecallPortalResult() : ServerPacket(SMSG_GARRISON_USE_RECALL_PORTAL_RESULT) { }

            WorldPacket const* Write() override;

            uint32 Result = 0;
            uint32 MissionRecID = 0;
            uint64 RecallTimestamp = 0;
            GarrisonMission Mission;
        };

        // IDA case 4980803 (§8.53): u32 Result, u64 ItemDbID, u32 size, u32 size,
        // GarrisonMissionReward[size], GarrisonMissionReward[size].
        // Conservative naming: {Result, ItemDbID, Rewards, OvermaxRewards}.
        class GarrisonMissionRequestRewardInfoResponse final : public ServerPacket
        {
        public:
            explicit GarrisonMissionRequestRewardInfoResponse() : ServerPacket(SMSG_GARRISON_MISSION_REQUEST_REWARD_INFO_RESPONSE) { }

            WorldPacket const* Write() override;

            uint32 Result = 0;
            uint64 ItemDbID = 0;
            std::vector<GarrisonMissionReward> Rewards;
            std::vector<GarrisonMissionReward> OvermaxRewards;
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

        // IDA case 4980782: u32 Result, GarrisonFollower (the renamed CustomName lives in
        // the trailing SizedString INSIDE the follower struct). See SNIFF_AUDIT §8.39.
        class GarrisonRenameFollowerResult final : public ServerPacket
        {
        public:
            explicit GarrisonRenameFollowerResult() : ServerPacket(SMSG_GARRISON_RENAME_FOLLOWER_RESULT) { }

            WorldPacket const* Write() override;

            uint32 Result = 0;
            GarrisonFollower Follower;
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

        // IDA case 4980788: u32 Result, GarrisonFollower (single follower, no array prefix).
        // See SNIFF_AUDIT §8.43.
        class GarrisonRecruitFollowerResult final : public ServerPacket
        {
        public:
            explicit GarrisonRecruitFollowerResult() : ServerPacket(SMSG_GARRISON_RECRUIT_FOLLOWER_RESULT) { }

            WorldPacket const* Write() override;

            uint32 Result = 0;
            GarrisonFollower Follower;
        };

        // IDA-confirmed (12.0.5.67186) layout — see SNIFF_AUDIT §10.3, all 6 fields
        // CONFIRMED with two-source evidence (IDA + Lua C-binding accessor names).
        //   PackedGuid NpcGUID, u32 MechanicTypeID, u32 TraitID,
        //   GarrisonFollower Followers[3]   (FIXED 3, no count prefix),
        //   Bits<1> CanGenerateRecruits + Bits<1> CanSetRecruitmentPreference + 6 padding bits.
        class GarrisonOpenRecruitmentNpc final : public ServerPacket
        {
        public:
            explicit GarrisonOpenRecruitmentNpc() : ServerPacket(SMSG_GARRISON_OPEN_RECRUITMENT_NPC) { }

            WorldPacket const* Write() override;

            ObjectGuid NpcGUID;
            uint32 MechanicTypeID = 0;
            uint32 TraitID = 0;
            std::array<GarrisonFollower, 3> Followers;
            bool CanGenerateRecruits = false;
            bool CanSetRecruitmentPreference = false;
        };

        // IDA case 4980778 (§8.35): u8, u32. Conservative: u8 GarrTypeID, u32 Result.
        class GarrisonFollowerFatigueCleared final : public ServerPacket
        {
        public:
            explicit GarrisonFollowerFatigueCleared() : ServerPacket(SMSG_GARRISON_FOLLOWER_FATIGUE_CLEARED, 1 + 4) { }

            WorldPacket const* Write() override;

            uint8 GarrTypeID = 0;
            uint32 Result = 0;
        };

        // IDA case 4980783 (§8.40): single full GarrisonFollower.
        class GarrisonRemoveFollowerAbilityResult final : public ServerPacket
        {
        public:
            explicit GarrisonRemoveFollowerAbilityResult() : ServerPacket(SMSG_GARRISON_REMOVE_FOLLOWER_ABILITY_RESULT) { }

            WorldPacket const* Write() override;

            GarrisonFollower Follower;
        };

        // IDA case 4980787 (§8.42): exactly 3 inline GarrisonFollowers (no count prefix).
        // Sent in response to CMSG_GARRISON_GENERATE_RECRUITS once new offers are rolled.
        class GarrisonGenerateFollowersResult final : public ServerPacket
        {
        public:
            explicit GarrisonGenerateFollowersResult() : ServerPacket(SMSG_GARRISON_GENERATE_FOLLOWERS_RESULT) { }

            WorldPacket const* Write() override;

            std::array<GarrisonFollower, 3> Followers;
        };

        // IDA case 4980761 (§8.22): u32 size, GarrisonFollower[size].
        // Cheat opcode — invoked by .garrison list-followers GM command.
        class GarrisonListFollowersCheatResult final : public ServerPacket
        {
        public:
            explicit GarrisonListFollowersCheatResult() : ServerPacket(SMSG_GARRISON_LIST_FOLLOWERS_CHEAT_RESULT) { }

            WorldPacket const* Write() override;

            std::vector<GarrisonFollower> Followers;
        };

        // IDA case 4980800 (§8.50): u32 size, u32[size].
        // Cheat opcode — list of completed mission rec IDs.
        class GarrisonListCompletedMissionsCheatResult final : public ServerPacket
        {
        public:
            explicit GarrisonListCompletedMissionsCheatResult() : ServerPacket(SMSG_GARRISON_LIST_COMPLETED_MISSIONS_CHEAT_RESULT) { }

            WorldPacket const* Write() override;

            std::vector<uint32> MissionRecIDs;
        };

        // IDA case 4980816 (§8.62-8.66 group): u32 Result, u32 MissionRecID, u32 NewState, GarrisonMission.
        // Cheat opcode — used by GM commands to force a mission state change.
        class GarrisonUpdateMissionCheatResult final : public ServerPacket
        {
        public:
            explicit GarrisonUpdateMissionCheatResult() : ServerPacket(SMSG_GARRISON_UPDATE_MISSION_CHEAT_RESULT) { }

            WorldPacket const* Write() override;

            uint32 Result = 0;
            uint32 MissionRecID = 0;
            uint32 NewState = 0;
            GarrisonMission Mission;
        };

        // ============================================================
        // Collection / event-list / spec-group SMSG packets (BfA / Shadowlands campaign tracking)
        // IDA cases 0x4C0045 - 0x4C004C, see SNIFF_AUDIT §8.54-8.61.
        // ============================================================

        // IDA case 4980805 (§8.54): wrapper + 1 byte + varU32 GarrTalentID + JamGarrisonTalentSocket.
        // Conservative: {u8 GarrTypeID, u8 CollectionEntryFlags, u32 GarrTalentID, GarrisonTalentSocketData}.
        class GarrisonCollectionUpdateEntry final : public ServerPacket
        {
        public:
            explicit GarrisonCollectionUpdateEntry() : ServerPacket(SMSG_GARRISON_COLLECTION_UPDATE_ENTRY) { }

            WorldPacket const* Write() override;

            uint8 GarrTypeID = 0;
            uint8 CollectionEntryFlags = 0;
            uint32 GarrTalentID = 0;
            GarrisonTalentSocketData Socket;
        };

        // IDA case 4980806 (§8.55): u8 GarrTypeID, u32 CollectionType, u32 GarrTalentID.
        class GarrisonCollectionRemoveEntry final : public ServerPacket
        {
        public:
            explicit GarrisonCollectionRemoveEntry() : ServerPacket(SMSG_GARRISON_COLLECTION_REMOVE_ENTRY, 1 + 4 + 4) { }

            WorldPacket const* Write() override;

            uint8 GarrTypeID = 0;
            uint32 CollectionType = 0;
            uint32 GarrTalentID = 0;
        };

        // IDA case 4980807 (§8.56): u8 GarrTypeID, u32 CollectionType.
        class GarrisonClearCollection final : public ServerPacket
        {
        public:
            explicit GarrisonClearCollection() : ServerPacket(SMSG_GARRISON_CLEAR_COLLECTION, 1 + 4) { }

            WorldPacket const* Write() override;

            uint8 GarrTypeID = 0;
            uint32 CollectionType = 0;
        };

        // IDA case 4980808 (§8.57): u8 GarrTypeID, u32 EventListID, u64 Timestamp, u32 EventValue.
        class GarrisonAddEvent final : public ServerPacket
        {
        public:
            explicit GarrisonAddEvent() : ServerPacket(SMSG_GARRISON_ADD_EVENT, 1 + 4 + 8 + 4) { }

            WorldPacket const* Write() override;

            uint8 GarrTypeID = 0;
            uint32 EventListID = 0;
            uint64 Timestamp = 0;
            uint32 EventValue = 0;
        };

        // IDA case 4980809 (§8.58): u8 GarrTypeID, u32 EventListID, u32 EventValue.
        class GarrisonRemoveEvent final : public ServerPacket
        {
        public:
            explicit GarrisonRemoveEvent() : ServerPacket(SMSG_GARRISON_REMOVE_EVENT, 1 + 4 + 4) { }

            WorldPacket const* Write() override;

            uint8 GarrTypeID = 0;
            uint32 EventListID = 0;
            uint32 EventValue = 0;
        };

        // IDA case 4980810 (§8.59): u8 GarrTypeID, u32 EventListID.
        class GarrisonClearEventList final : public ServerPacket
        {
        public:
            explicit GarrisonClearEventList() : ServerPacket(SMSG_GARRISON_CLEAR_EVENT_LIST, 1 + 4) { }

            WorldPacket const* Write() override;

            uint8 GarrTypeID = 0;
            uint32 EventListID = 0;
        };

        // IDA case 4980811 (§8.60): u8 GarrTypeID, u32 size, SpecGroup[size].
        // Conservative SpecGroup shape: {u32 GarrSpecGroupID, u32 SelectedTalentTreeID}.
        class GarrisonAddSpecGroups final : public ServerPacket
        {
        public:
            struct GarrisonSpecGroup
            {
                uint32 GarrSpecGroupID = 0;
                uint32 SelectedTalentTreeID = 0;
            };

            explicit GarrisonAddSpecGroups() : ServerPacket(SMSG_GARRISON_ADD_SPEC_GROUPS) { }

            WorldPacket const* Write() override;

            uint8 GarrTypeID = 0;
            std::vector<GarrisonSpecGroup> SpecGroups;
        };

        // IDA case 4980812 (§8.61): single byte GarrTypeID.
        class GarrisonClearSpecGroups final : public ServerPacket
        {
        public:
            explicit GarrisonClearSpecGroups() : ServerPacket(SMSG_GARRISON_CLEAR_SPEC_GROUPS, 1) { }

            WorldPacket const* Write() override;

            uint8 GarrTypeID = 0;
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

        // IDA case 4980796: 3 × u32. Likely shape is {Result, PlotInstanceID1, PlotInstanceID2}
        // mirroring the CMSG. Field meaning of the trailing two u32 is not byte-confirmed
        // (no sniff sample); echoing the CMSG plot pair is the conservative choice.
        // See SNIFF_AUDIT §8.46.
        class GarrisonSwapBuildingsResponse final : public ServerPacket
        {
        public:
            explicit GarrisonSwapBuildingsResponse() : ServerPacket(SMSG_GARRISON_SWAP_BUILDINGS_RESPONSE, 4 + 4 + 4) { }

            WorldPacket const* Write() override;

            uint32 Result = 0;
            uint32 PlotInstanceID1 = 0;
            uint32 PlotInstanceID2 = 0;
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

        // IDA case 4980750: u32 Result, u8 GarrTypeID, Bits<1> (purpose unknown — observed
        // value 0 in the static analysis), GarrisonTalent. See SNIFF_AUDIT §8.11.
        class GarrisonResearchTalentResult final : public ServerPacket
        {
        public:
            explicit GarrisonResearchTalentResult() : ServerPacket(SMSG_GARRISON_RESEARCH_TALENT_RESULT) { }

            WorldPacket const* Write() override;

            uint32 Result = 0;
            uint8 GarrTypeID = 0;
            bool UnknownBit = false;
            GarrisonTalent Talent;
        };

        // IDA case 4980751: u8 GarrTypeID, u32 GarrTalentID, u32 Rank, u32 ResearchStartTime.
        // Sent when a talent's research timer finishes. See SNIFF_AUDIT §8.12.
        class GarrisonTalentCompleted final : public ServerPacket
        {
        public:
            explicit GarrisonTalentCompleted() : ServerPacket(SMSG_GARRISON_TALENT_COMPLETED, 1 + 4 + 4 + 4) { }

            WorldPacket const* Write() override;

            uint8 GarrTypeID = 0;
            uint32 GarrTalentID = 0;
            uint32 Rank = 0;
            uint32 ResearchStartTime = 0;
        };

        // IDA case 4980752: u8 GarrTypeID, u32 GarrTalentID. See SNIFF_AUDIT §8.13.
        class GarrisonTalentRemoved final : public ServerPacket
        {
        public:
            explicit GarrisonTalentRemoved() : ServerPacket(SMSG_GARRISON_TALENT_REMOVED, 1 + 4) { }

            WorldPacket const* Write() override;

            uint8 GarrTypeID = 0;
            uint32 GarrTalentID = 0;
        };

        // IDA case 4980753: u8 GarrTypeID, u32 GarrTalentID, optional<GarrisonTalentSocketData>
        // (top-bit-gated by Bits<1>). See SNIFF_AUDIT §8.14.
        class GarrisonTalentUpdateSocketData final : public ServerPacket
        {
        public:
            explicit GarrisonTalentUpdateSocketData() : ServerPacket(SMSG_GARRISON_TALENT_UPDATE_SOCKET_DATA) { }

            WorldPacket const* Write() override;

            uint8 GarrTypeID = 0;
            uint32 GarrTalentID = 0;
            Optional<GarrisonTalentSocketData> Socket;
        };

        // IDA case 4980754: u8 GarrTypeID, u32 GarrTalentID. See SNIFF_AUDIT §8.15.
        class GarrisonTalentRemoveSocketData final : public ServerPacket
        {
        public:
            explicit GarrisonTalentRemoveSocketData() : ServerPacket(SMSG_GARRISON_TALENT_REMOVE_SOCKET_DATA, 1 + 4) { }

            WorldPacket const* Write() override;

            uint8 GarrTypeID = 0;
            uint32 GarrTalentID = 0;
        };

        // IDA case 4980755: u8 GarrTypeID, u32 GarrTalentTreeID. See SNIFF_AUDIT §8.16.
        class GarrisonResetTalentTree final : public ServerPacket
        {
        public:
            explicit GarrisonResetTalentTree() : ServerPacket(SMSG_GARRISON_RESET_TALENT_TREE, 1 + 4) { }

            WorldPacket const* Write() override;

            uint8 GarrTypeID = 0;
            uint32 GarrTalentTreeID = 0;
        };

        // IDA case 4980756: u8 GarrTypeID, u32 GarrTalentTreeID. See SNIFF_AUDIT §8.17.
        class GarrisonResetTalentTreeSocketData final : public ServerPacket
        {
        public:
            explicit GarrisonResetTalentTreeSocketData() : ServerPacket(SMSG_GARRISON_RESET_TALENT_TREE_SOCKET_DATA, 1 + 4) { }

            WorldPacket const* Write() override;

            uint8 GarrTypeID = 0;
            uint32 GarrTalentTreeID = 0;
        };

        // IDA case 4980813: u8 GarrTypeID, u32 size, GarrisonTalent[size]. See SNIFF_AUDIT §8.62.
        class GarrisonSwitchTalentTreeBranch final : public ServerPacket
        {
        public:
            explicit GarrisonSwitchTalentTreeBranch() : ServerPacket(SMSG_GARRISON_SWITCH_TALENT_TREE_BRANCH) { }

            WorldPacket const* Write() override;

            uint8 GarrTypeID = 0;
            std::vector<GarrisonTalent> Talents;
        };

        // IDA case 4980814: opaque generic-byte-block helper. Field shape unknown without sniff.
        // TC sends a single u8 GarrTypeID + size-prefixed list of unlocked talent tree IDs as
        // the conservative interpretation. See SNIFF_AUDIT §8.63.
        class GarrisonTalentWorldQuestUnlocksResponse final : public ServerPacket
        {
        public:
            explicit GarrisonTalentWorldQuestUnlocksResponse() : ServerPacket(SMSG_GARRISON_TALENT_WORLD_QUEST_UNLOCKS_RESPONSE) { }

            WorldPacket const* Write() override;

            uint8 GarrTypeID = 0;
            std::vector<int32> UnlockedTalentTreeIDs;
        };

        // IDA case 4980815: opaque generic-byte-block helper. TC sends u8 GarrTypeID +
        // size-prefixed array of {u32 GarrTalentID, GarrisonTalentSocketData} pairs. See SNIFF_AUDIT §8.64.
        class GarrisonApplyTalentSocketDataChanges final : public ServerPacket
        {
        public:
            struct TalentSocketChange
            {
                uint32 GarrTalentID = 0;
                GarrisonTalentSocketData Socket;
            };

            explicit GarrisonApplyTalentSocketDataChanges() : ServerPacket(SMSG_GARRISON_APPLY_TALENT_SOCKET_DATA_CHANGES) { }

            WorldPacket const* Write() override;

            uint8 GarrTypeID = 0;
            std::vector<TalentSocketChange> Changes;
        };

        // ============================================================
        // Shipment packets
        // ============================================================

        struct CharacterShipment
        {
            int32 ShipmentRecID = 0;
            uint64 ShipmentID = 0;
            uint64 AssignedFollowerDBID = 0;
            uint32 ContainerID = 0;   // sniff-decoded: sits between AssignedFollowerDBID and CreationTime
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
            explicit OpenShipmentNpcResult() : ServerPacket(SMSG_OPEN_SHIPMENT_NPC_RESULT, 1 + 16 + 4) { }

            WorldPacket const* Write() override;

            bool Success = true;   // leading bit — sniff-verified (0x80): without it the client misaligns the guid read and crashes
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

        // IDA case 4980801 (§8.51 in main audit table — pet name): u64 NpcGUID-as-DBID + sub-call
        // (likely PackedGuid serializer) + 1-byte top-bit-prefixed string. Conservative shape:
        // {ObjectGuid NpcGUID, SizedString PetName}.
        class QueryGarrisonPetNameResponse final : public ServerPacket
        {
        public:
            explicit QueryGarrisonPetNameResponse() : ServerPacket(SMSG_QUERY_GARRISON_PET_NAME_RESPONSE) { }

            WorldPacket const* Write() override;

            ObjectGuid NpcGUID;
            std::string PetName;
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
