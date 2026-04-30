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

#include "WorldSession.h"
#include "DB2Stores.h"
#include "DB2Structure.h"
#include "GameTime.h"
#include "Garrison.h"
#include "GarrisonMgr.h"
#include "GarrisonPackets.h"
#include "Group.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Random.h"

void WorldSession::HandleGetGarrisonInfo(WorldPackets::Garrison::GetGarrisonInfo& /*getGarrisonInfo*/)
{
    // Sniff-confirmed: troop quality refresh packets sent BEFORE main garrison info
    for (auto const& [type, garrison] : _player->GetGarrisons())
        garrison->SendTroopQualityRefresh();

    WorldPackets::Garrison::GetGarrisonInfoResult garrisonInfo;
    garrisonInfo.FactionIndex = Garrison::GetFaction(_player->GetTeam());

    for (auto const& [type, garrison] : _player->GetGarrisons())
        garrison->BuildInfoPacket(garrisonInfo.Garrisons.emplace_back());

    garrisonInfo.FollowerSoftCaps = {
        { FOLLOWER_TYPE_GARRISON,   20 },
        { FOLLOWER_TYPE_SHIPYARD,   6 },
        { FOLLOWER_TYPE_CLASS_ORDER, 6 },
        { FOLLOWER_TYPE_WAR_CAMPAIGN, 30 },
        { FOLLOWER_TYPE_COVENANT,   100 }
    };

    SendPacket(garrisonInfo.Write());

    // Follow up with expired mission cleanup and mission start condition updates per garrison
    for (auto const& [type, garrison] : _player->GetGarrisons())
    {
        garrison->SendDeleteExpiredMissionsResult();
        garrison->SendMissionStartConditionUpdate();
    }
}

void WorldSession::HandleGarrisonPurchaseBuilding(WorldPackets::Garrison::GarrisonPurchaseBuilding& garrisonPurchaseBuilding)
{
    if (!_player->GetNPCIfCanInteractWith(garrisonPurchaseBuilding.NpcGUID, UNIT_NPC_FLAG_NONE, UNIT_NPC_FLAG_2_GARRISON_ARCHITECT))
        return;

    if (Garrison* garrison = _player->GetGarrison())
        garrison->PlaceBuilding(garrisonPurchaseBuilding.PlotInstanceID, garrisonPurchaseBuilding.BuildingID);
}

void WorldSession::HandleGarrisonCancelConstruction(WorldPackets::Garrison::GarrisonCancelConstruction& garrisonCancelConstruction)
{
    if (!_player->GetNPCIfCanInteractWith(garrisonCancelConstruction.NpcGUID, UNIT_NPC_FLAG_NONE, UNIT_NPC_FLAG_2_GARRISON_ARCHITECT))
        return;

    if (Garrison* garrison = _player->GetGarrison())
        garrison->CancelBuildingConstruction(garrisonCancelConstruction.PlotInstanceID);
}

void WorldSession::HandleGarrisonRequestBlueprintAndSpecializationData(WorldPackets::Garrison::GarrisonRequestBlueprintAndSpecializationData& /*garrisonRequestBlueprintAndSpecializationData*/)
{
    for (auto const& [type, garrison] : _player->GetGarrisons())
        garrison->SendBlueprintAndSpecializationData();
}

void WorldSession::HandleGarrisonGetMapData(WorldPackets::Garrison::GarrisonGetMapData& /*garrisonGetMapData*/)
{
    if (Garrison* garrison = _player->GetGarrison())
    {
        garrison->SendMapData(_player);

        // Send monument/trophy selections after map data (sniff-confirmed zone-in sequence)
        WorldPackets::Garrison::GarrisonUpdateGarrisonMonumentSelections selections;
        for (uint32 trophyId : garrison->GetTrophies())
        {
            WorldPackets::Garrison::GarrisonTrophyData data;
            data.TrophyID = trophyId;
            selections.Trophies.push_back(data);
        }
        SendPacket(selections.Write());
    }
}

// ============================================================
// Mission handlers
// ============================================================

void WorldSession::HandleGarrisonStartMission(WorldPackets::Garrison::GarrisonStartMission& garrisonStartMission)
{
    Garrison* garrison = _player->GetGarrison();
    if (!garrison)
        return;

    GarrisonError result = garrison->StartMission(garrisonStartMission.MissionRecID, garrisonStartMission.FollowerDBIDs);

    WorldPackets::Garrison::GarrisonStartMissionResult startResult;
    startResult.Result = result;
    if (result == GARRISON_SUCCESS)
        startResult.NumOfferedToday = static_cast<uint16>(garrison->GetAndIncrementSessionMissionCount());
    if (Garrison::Mission const* mission = garrison->GetMissionByRecID(garrisonStartMission.MissionRecID))
        startResult.Mission = mission->PacketInfo;

    startResult.Followers.reserve(garrisonStartMission.FollowerDBIDs.size());
    for (uint64 dbId : garrisonStartMission.FollowerDBIDs)
    {
        WorldPackets::Garrison::GarrisonMissionFollowerEntry entry;
        entry.DbID = dbId;
        // BoardIndex/Health/HasFollowerEntry mirror the CMSG values; we don't override them
        // here, so they default to (-1, 0, 0) — same as what the client sends in CMSG.
        startResult.Followers.push_back(entry);
    }
    SendPacket(startResult.Write());
}

void WorldSession::HandleGarrisonCompleteMission(WorldPackets::Garrison::GarrisonCompleteMission& garrisonCompleteMission)
{
    Garrison* garrison = _player->GetGarrison();
    if (!garrison)
        return;

    Garrison::Mission const* mission = garrison->GetMissionByRecID(garrisonCompleteMission.MissionRecID);
    if (!mission)
        return;

    GarrisonError result = garrison->CompleteMission(garrisonCompleteMission.MissionRecID);

    WorldPackets::Garrison::GarrisonCompleteMissionResult completeResult;
    completeResult.Result = result;
    completeResult.MissionRecID = garrisonCompleteMission.MissionRecID;

    // Re-fetch mission after completion (state may have changed)
    mission = garrison->GetMissionByRecID(garrisonCompleteMission.MissionRecID);
    if (mission)
    {
        completeResult.Mission = mission->PacketInfo;
        // Determine success based on the success chance roll
        completeResult.Succeeded = static_cast<int32>(urand(0, 99)) < mission->PacketInfo.SuccessChance;
    }

    SendPacket(completeResult.Write());
}

void WorldSession::HandleGarrisonMissionBonusRoll(WorldPackets::Garrison::GarrisonMissionBonusRoll& garrisonMissionBonusRoll)
{
    Garrison* garrison = _player->GetGarrison();
    if (!garrison)
        return;

    GarrisonError result = garrison->MissionBonusRoll(garrisonMissionBonusRoll.MissionRecID);

    WorldPackets::Garrison::GarrisonMissionBonusRollResult bonusResult;
    bonusResult.MissionRecID = garrisonMissionBonusRoll.MissionRecID;
    bonusResult.Result = result;

    if (Garrison::Mission const* mission = garrison->GetMissionByRecID(garrisonMissionBonusRoll.MissionRecID))
        bonusResult.Mission = mission->PacketInfo;

    SendPacket(bonusResult.Write());
}

void WorldSession::HandleGarrisonGetMissionReward(WorldPackets::Garrison::GarrisonGetMissionReward& garrisonGetMissionReward)
{
    Garrison* garrison = _player->GetGarrison();
    if (!garrison)
        return;

    GarrisonError result = garrison->ClaimMissionReward(garrisonGetMissionReward.MissionRecID);

    // ClaimMissionReward already sends GarrisonFollowerChangedXP for each follower,
    // and removes the mission internally. Send a targeted deletion notification
    // instead of a full GetGarrisonInfoResult to reduce bandwidth.
    WorldPackets::Garrison::GarrisonDeleteMissionResult deleteMissionResult;
    deleteMissionResult.Result = result;
    deleteMissionResult.MissionRecID = garrisonGetMissionReward.MissionRecID;
    deleteMissionResult.GarrTypeID = garrison->GetType();
    SendPacket(deleteMissionResult.Write());
}

void WorldSession::HandleOpenMissionNpc(WorldPackets::Garrison::OpenMissionNpc& /*openMissionNpc*/)
{
    Garrison* garrison = _player->GetGarrison();
    if (!garrison)
        return;

    // Send expired mission cleanup results for all garrison types
    for (auto const& [type, garr] : _player->GetGarrisons())
        garr->SendDeleteExpiredMissionsResult();

    // Remove expired offers (sends GarrisonDeleteMissionResult per expired mission)
    // and generate new missions (sends GarrisonAddMissionResult per new mission).
    // Individual targeted packets are sent instead of a full GetGarrisonInfoResult.
    garrison->RemoveExpiredMissions();
    garrison->GenerateAvailableMissions();

    // Send mission start condition update
    garrison->SendMissionStartConditionUpdate();
}

// ============================================================
// Follower handlers
// ============================================================

void WorldSession::HandleGarrisonAssignFollowerToBuilding(WorldPackets::Garrison::GarrisonAssignFollowerToBuilding& garrisonAssignFollowerToBuilding)
{
    Garrison* garrison = _player->GetGarrison();
    if (!garrison)
        return;

    garrison->AssignFollowerToBuilding(garrisonAssignFollowerToBuilding.FollowerDBID, garrisonAssignFollowerToBuilding.PlotInstanceID);
}

void WorldSession::HandleGarrisonRemoveFollowerFromBuilding(WorldPackets::Garrison::GarrisonRemoveFollowerFromBuilding& garrisonRemoveFollowerFromBuilding)
{
    Garrison* garrison = _player->GetGarrison();
    if (!garrison)
        return;

    garrison->RemoveFollowerFromBuilding(garrisonRemoveFollowerFromBuilding.FollowerDBID);
}

void WorldSession::HandleGarrisonRemoveFollower(WorldPackets::Garrison::GarrisonRemoveFollower& garrisonRemoveFollower)
{
    Garrison* garrison = _player->GetGarrison();
    if (!garrison)
        return;

    garrison->RemoveFollower(garrisonRemoveFollower.FollowerDBID);
}

void WorldSession::HandleGarrisonRenameFollower(WorldPackets::Garrison::GarrisonRenameFollower& garrisonRenameFollower)
{
    Garrison* garrison = _player->GetGarrison();
    if (!garrison)
        return;

    garrison->RenameFollower(garrisonRenameFollower.FollowerDBID, garrisonRenameFollower.FollowerName);
}

void WorldSession::HandleGarrisonSetFollowerFavorite(WorldPackets::Garrison::GarrisonSetFollowerFavorite& garrisonSetFollowerFavorite)
{
    Garrison* garrison = _player->GetGarrison();
    if (!garrison)
        return;

    garrison->SetFollowerFavorite(garrisonSetFollowerFavorite.FollowerDBID, garrisonSetFollowerFavorite.Favorite);
}

void WorldSession::HandleGarrisonSetFollowerInactive(WorldPackets::Garrison::GarrisonSetFollowerInactive& garrisonSetFollowerInactive)
{
    Garrison* garrison = _player->GetGarrison();
    if (!garrison)
        return;

    garrison->SetFollowerInactive(garrisonSetFollowerInactive.FollowerDBID, garrisonSetFollowerInactive.Inactive);
}

void WorldSession::HandleGarrisonRecruitFollower(WorldPackets::Garrison::GarrisonRecruitFollower& garrisonRecruitFollower)
{
    Garrison* garrison = _player->GetGarrison();
    if (!garrison)
        return;

    // FollowerIndex references the index in the available recruits list
    auto const& recruits = garrison->GetAvailableRecruits();
    if (garrisonRecruitFollower.FollowerIndex >= recruits.size())
    {
        WorldPackets::Garrison::GarrisonRecruitFollowerResult recruitResult;
        recruitResult.Result = GARRISON_ERROR_INVALID_AVAILABLE_RECRUIT;
        SendPacket(recruitResult.Write());
        return;
    }
    uint32 followerID = recruits[garrisonRecruitFollower.FollowerIndex].GarrFollowerID;
    GarrisonError result = garrison->RecruitFollower(followerID);

    WorldPackets::Garrison::GarrisonRecruitFollowerResult recruitResult;
    recruitResult.Result = result;
    if (result == GARRISON_SUCCESS)
        recruitResult.Followers = garrison->GetAvailableRecruits();
    SendPacket(recruitResult.Write());
}

void WorldSession::HandleGarrisonGenerateRecruits(WorldPackets::Garrison::GarrisonGenerateRecruits& /*garrisonGenerateRecruits*/)
{
    Garrison* garrison = _player->GetGarrison();
    if (!garrison)
        return;

    uint32 faction = static_cast<uint32>(Garrison::GetFaction(_player->GetTeam()));
    garrison->GenerateRecruits(faction);

    WorldPackets::Garrison::GarrisonRecruitFollowerResult result;
    result.Result = GARRISON_SUCCESS;
    result.Followers = garrison->GetAvailableRecruits();
    SendPacket(result.Write());
}

void WorldSession::HandleGarrisonFullyHealAllFollowers(WorldPackets::Garrison::GarrisonFullyHealAllFollowers& /*garrisonFullyHealAllFollowers*/)
{
    Garrison* garrison = _player->GetGarrison();
    if (!garrison)
        return;

    garrison->HealAllFollowers();

    // Send individual GarrisonUpdateFollower packets for each follower
    // instead of a full GetGarrisonInfoResult to reduce bandwidth
    garrison->SendAllFollowerUpdates();
}

void WorldSession::HandleGarrisonAddFollowerHealth(WorldPackets::Garrison::GarrisonAddFollowerHealth& garrisonAddFollowerHealth)
{
    Garrison* garrison = _player->GetGarrison();
    if (!garrison)
        return;

    Garrison::Follower* follower = garrison->GetFollower(garrisonAddFollowerHealth.FollowerDBID);
    if (!follower)
        return;

    follower->PacketInfo.Health = std::min(follower->PacketInfo.Health + garrisonAddFollowerHealth.HealthToAdd, static_cast<int32>(follower->PacketInfo.Durability));

    WorldPackets::Garrison::GarrisonUpdateFollower updateFollower;
    updateFollower.Result = GARRISON_SUCCESS;
    updateFollower.Follower = follower->PacketInfo;
    SendPacket(updateFollower.Write());
}

void WorldSession::HandleGarrisonGetClassSpecCategoryInfo(WorldPackets::Garrison::GarrisonGetClassSpecCategoryInfo& /*garrisonGetClassSpecCategoryInfo*/)
{
    WorldPackets::Garrison::GarrisonGetClassSpecCategoryInfoResult result;

    // Populate class spec categories from DB2
    for (GarrClassSpecEntry const* classSpec : sGarrClassSpecStore)
    {
        WorldPackets::Garrison::GarrisonGetClassSpecCategoryInfoResult::GarrisonFollowerCategoryInfo info;
        info.GarrClassSpecID = classSpec->ID;
        info.GarrFollowerTypeID = classSpec->FollowerClassLimit;
        result.FollowerClassSpecCategoryInfos.push_back(info);
    }

    SendPacket(result.Write());
}

void WorldSession::HandleGarrisonSetRecruitmentPreferences(WorldPackets::Garrison::GarrisonSetRecruitmentPreferences& garrisonSetRecruitmentPreferences)
{
    if (!_player->GetGarrison())
        return;

    _player->GetGarrison()->SetRecruitmentPreferences(
        garrisonSetRecruitmentPreferences.AbilityID,
        garrisonSetRecruitmentPreferences.TraitID);
}

// ============================================================
// Building/Utility handlers
// ============================================================

void WorldSession::HandleUpgradeGarrison(WorldPackets::Garrison::UpgradeGarrison& upgradeGarrison)
{
    if (!_player->GetNPCIfCanInteractWith(upgradeGarrison.NpcGUID, UNIT_NPC_FLAG_NONE, UNIT_NPC_FLAG_2_GARRISON_ARCHITECT))
        return;

    Garrison* garrison = _player->GetGarrison();
    if (!garrison)
        return;

    GarrSiteLevelEntry const* currentLevel = garrison->GetSiteLevel();
    if (!currentLevel)
    {
        WorldPackets::Garrison::GarrisonUpgradeResult result;
        result.Result = GARRISON_ERROR_INVALID_GARRISON;
        result.GarrSiteLevelID = 0;
        SendPacket(result.Write());
        return;
    }

    // Look for next level
    GarrSiteLevelEntry const* nextLevel = sGarrisonMgr.GetGarrSiteLevelEntry(currentLevel->GarrSiteID, currentLevel->GarrLevel + 1);
    if (!nextLevel)
    {
        WorldPackets::Garrison::GarrisonUpgradeResult result;
        result.Result = GARRISON_ERROR_UPGRADE_LEVEL_EXCEEDS_GARRISON_LEVEL;
        result.GarrSiteLevelID = currentLevel->ID;
        SendPacket(result.Write());
        return;
    }

    // Check upgrade cost (from GarrSiteLevelEntry)
    if (nextLevel->UpgradeGoldCost > 0 && !_player->HasEnoughMoney(uint64(nextLevel->UpgradeGoldCost)))
    {
        WorldPackets::Garrison::GarrisonUpgradeResult result;
        result.Result = GARRISON_ERROR_NOT_ENOUGH_GOLD;
        result.GarrSiteLevelID = currentLevel->ID;
        SendPacket(result.Write());
        return;
    }

    if (nextLevel->UpgradeCost > 0 && !_player->HasCurrency(824 /*Garrison Resources*/, nextLevel->UpgradeCost))
    {
        WorldPackets::Garrison::GarrisonUpgradeResult result;
        result.Result = GARRISON_ERROR_NOT_ENOUGH_CURRENCY;
        result.GarrSiteLevelID = currentLevel->ID;
        SendPacket(result.Write());
        return;
    }

    // Deduct costs
    if (nextLevel->UpgradeGoldCost > 0)
        _player->ModifyMoney(-int64(nextLevel->UpgradeGoldCost), false);
    if (nextLevel->UpgradeCost > 0)
        _player->RemoveCurrency(824 /*Garrison Resources*/, nextLevel->UpgradeCost, CurrencyDestroyReason::Garrison);

    garrison->Upgrade();

    WorldPackets::Garrison::GarrisonUpgradeResult result;
    result.Result = GARRISON_SUCCESS;
    result.GarrSiteLevelID = garrison->GetSiteLevel() ? garrison->GetSiteLevel()->ID : 0;
    SendPacket(result.Write());
}

void WorldSession::HandleGarrisonCheckUpgradeable(WorldPackets::Garrison::GarrisonCheckUpgradeable& garrisonCheckUpgradeable)
{
    // Client sends GarrSiteID, not GarrTypeID. Find the matching garrison.
    Garrison* garrison = nullptr;
    for (auto const& [type, garr] : _player->GetGarrisons())
    {
        if (garr->GetSiteLevel() && garr->GetSiteLevel()->GarrSiteID == garrisonCheckUpgradeable.GarrSiteID)
        {
            garrison = garr.get();
            break;
        }
    }
    GarrisonError upgradeResult = GARRISON_ERROR_UPGRADE_CONDITION_FAILED;

    if (garrison)
    {
        GarrSiteLevelEntry const* currentLevel = garrison->GetSiteLevel();
        if (currentLevel)
        {
            GarrSiteLevelEntry const* nextLevel = sGarrisonMgr.GetGarrSiteLevelEntry(currentLevel->GarrSiteID, currentLevel->GarrLevel + 1);
            if (nextLevel)
            {
                bool canAfford = true;
                if (nextLevel->UpgradeGoldCost > 0 && !_player->HasEnoughMoney(uint64(nextLevel->UpgradeGoldCost)))
                    canAfford = false;
                if (nextLevel->UpgradeCost > 0 && !_player->HasCurrency(824 /*Garrison Resources*/, nextLevel->UpgradeCost))
                    canAfford = false;

                if (canAfford)
                    upgradeResult = GARRISON_SUCCESS;
                else
                    upgradeResult = GARRISON_ERROR_NOT_ENOUGH_CURRENCY;
            }
            else
                upgradeResult = GARRISON_ERROR_UPGRADE_LEVEL_EXCEEDS_GARRISON_LEVEL;
        }
    }

    WorldPackets::Garrison::GarrisonIsUpgradeableResponse result;
    result.Result = upgradeResult;
    SendPacket(result.Write());
}

void WorldSession::HandleGarrisonSetBuildingActive(WorldPackets::Garrison::GarrisonSetBuildingActive& garrisonSetBuildingActive)
{
    Garrison* garrison = _player->GetGarrison();
    if (!garrison)
        return;

    garrison->ActivateBuilding(garrisonSetBuildingActive.PlotInstanceID);
}

void WorldSession::HandleGarrisonSwapBuildings(WorldPackets::Garrison::GarrisonSwapBuildings& garrisonSwapBuildings)
{
    if (!_player->GetNPCIfCanInteractWith(garrisonSwapBuildings.NpcGUID, UNIT_NPC_FLAG_NONE, UNIT_NPC_FLAG_2_GARRISON_ARCHITECT))
        return;

    if (Garrison* garrison = _player->GetGarrison())
        garrison->SwapBuildings(garrisonSwapBuildings.PlotInstanceID1, garrisonSwapBuildings.PlotInstanceID2);
}

// ============================================================
// Talent handlers
// ============================================================

void WorldSession::HandleGarrisonLearnTalent(WorldPackets::Garrison::GarrisonLearnTalent& garrisonLearnTalent)
{
    GarrTalentEntry const* talentEntry = sGarrTalentStore.LookupEntry(garrisonLearnTalent.GarrTalentID);
    if (!talentEntry)
    {
        WorldPackets::Garrison::GarrisonResearchTalentResult result;
        result.Result = GARRISON_ERROR_INVALID_TALENT;
        SendPacket(result.Write());
        return;
    }

    GarrTalentTreeEntry const* treeEntry = sGarrTalentTreeStore.LookupEntry(talentEntry->GarrTalentTreeID);
    if (!treeEntry)
    {
        WorldPackets::Garrison::GarrisonResearchTalentResult result;
        result.Result = GARRISON_ERROR_INVALID_TALENT;
        SendPacket(result.Write());
        return;
    }

    Garrison* garrison = _player->GetGarrison(static_cast<GarrisonType>(treeEntry->GarrTypeID));
    if (!garrison)
    {
        WorldPackets::Garrison::GarrisonResearchTalentResult result;
        result.Result = GARRISON_ERROR_NO_GARRISON;
        result.GarrTypeID = treeEntry->GarrTypeID;
        SendPacket(result.Write());
        return;
    }

    garrison->LearnTalent(garrisonLearnTalent.GarrTalentID, garrisonLearnTalent.IsTemporary);
}

void WorldSession::HandleGarrisonResearchTalent(WorldPackets::Garrison::GarrisonResearchTalent& garrisonResearchTalent)
{
    GarrTalentEntry const* talentEntry = sGarrTalentStore.LookupEntry(garrisonResearchTalent.GarrTalentID);
    if (!talentEntry)
    {
        WorldPackets::Garrison::GarrisonResearchTalentResult result;
        result.Result = GARRISON_ERROR_INVALID_TALENT;
        SendPacket(result.Write());
        return;
    }

    GarrTalentTreeEntry const* treeEntry = sGarrTalentTreeStore.LookupEntry(talentEntry->GarrTalentTreeID);
    if (!treeEntry)
    {
        WorldPackets::Garrison::GarrisonResearchTalentResult result;
        result.Result = GARRISON_ERROR_INVALID_TALENT;
        SendPacket(result.Write());
        return;
    }

    Garrison* garrison = _player->GetGarrison(static_cast<GarrisonType>(treeEntry->GarrTypeID));
    if (!garrison)
    {
        WorldPackets::Garrison::GarrisonResearchTalentResult result;
        result.Result = GARRISON_ERROR_NO_GARRISON;
        result.GarrTypeID = treeEntry->GarrTypeID;
        SendPacket(result.Write());
        return;
    }

    garrison->ResearchTalent(garrisonResearchTalent.GarrTalentID);
}

void WorldSession::HandleGarrisonSocketTalent(WorldPackets::Garrison::GarrisonSocketTalent& garrisonSocketTalent)
{
    GarrTalentEntry const* talentEntry = sGarrTalentStore.LookupEntry(garrisonSocketTalent.GarrTalentID);
    if (!talentEntry)
    {
        WorldPackets::Garrison::GarrisonResearchTalentResult result;
        result.Result = GARRISON_ERROR_INVALID_TALENT;
        SendPacket(result.Write());
        return;
    }

    GarrTalentTreeEntry const* treeEntry = sGarrTalentTreeStore.LookupEntry(talentEntry->GarrTalentTreeID);
    if (!treeEntry)
    {
        WorldPackets::Garrison::GarrisonResearchTalentResult result;
        result.Result = GARRISON_ERROR_INVALID_TALENT;
        SendPacket(result.Write());
        return;
    }

    Garrison* garrison = _player->GetGarrison(static_cast<GarrisonType>(treeEntry->GarrTypeID));
    if (!garrison)
    {
        WorldPackets::Garrison::GarrisonResearchTalentResult result;
        result.Result = GARRISON_ERROR_NO_GARRISON;
        result.GarrTypeID = treeEntry->GarrTypeID;
        SendPacket(result.Write());
        return;
    }

    garrison->SocketTalent(garrisonSocketTalent.GarrTalentID, garrisonSocketTalent.SoulbindConduitID, garrisonSocketTalent.SoulbindConduitRank);
}

// ============================================================
// Other utility handlers
// ============================================================

void WorldSession::HandleGarrisonRequestShipmentInfo(WorldPackets::Garrison::GarrisonRequestShipmentInfo& garrisonRequestShipmentInfo)
{
    Garrison* garrison = _player->GetGarrison();
    if (!garrison)
    {
        WorldPackets::Garrison::GetShipmentInfoResponse response;
        SendPacket(response.Write());
        return;
    }

    garrison->SendShipmentInfo(garrisonRequestShipmentInfo.NpcGUID);
}

void WorldSession::HandleOpenShipmentNpc(WorldPackets::Garrison::OpenShipmentNpc& openShipmentNpc)
{
    Garrison* garrison = _player->GetGarrison();
    if (!garrison)
        return;

    uint32 plotInstanceId = garrison->FindPlotInstanceForNpc(openShipmentNpc.NpcGUID);
    if (!plotInstanceId)
        return;

    Garrison::Plot const* plot = garrison->GetPlot(plotInstanceId);
    if (!plot || !plot->BuildingInfo.PacketInfo || !plot->BuildingInfo.PacketInfo->Active)
        return;

    GarrBuildingEntry const* building = sGarrBuildingStore.LookupEntry(plot->BuildingInfo.PacketInfo->GarrBuildingID);
    if (!building)
        return;

    CharShipmentContainerEntry const* container = sGarrisonMgr.GetShipmentContainerForBuilding(building->BuildingType);
    if (!container)
        return;

    WorldPackets::Garrison::OpenShipmentNpcResult result;
    result.NpcGUID = openShipmentNpc.NpcGUID;
    result.CharShipmentContainerID = container->ID;
    SendPacket(result.Write());
}

void WorldSession::HandleCreateShipment(WorldPackets::Garrison::CreateShipment& createShipment)
{
    Garrison* garrison = _player->GetGarrison();
    if (!garrison)
        return;

    garrison->CreateShipment(createShipment.NpcGUID, createShipment.Count);
}

void WorldSession::HandleGetLandingPageShipments(WorldPackets::Garrison::GetLandingPageShipments& /*getLandingPageShipments*/)
{
    Garrison* garrison = _player->GetGarrison();
    if (!garrison)
        return;

    garrison->SendLandingPageShipments();
}

void WorldSession::HandleSetUsingPartyGarrison(WorldPackets::Garrison::SetUsingPartyGarrison& setUsingPartyGarrison)
{
    if (setUsingPartyGarrison.UsingPartyGarrison)
    {
        // Player wants to visit party leader's garrison
        Group* group = _player->GetGroup();
        if (!group)
            return;

        Player* leader = ObjectAccessor::FindPlayer(group->GetLeaderGUID());
        if (!leader || leader == _player)
            return;

        Garrison* leaderGarrison = leader->GetGarrison(static_cast<GarrisonType>(setUsingPartyGarrison.GarrTypeID));
        if (!leaderGarrison)
            return;

        GarrSiteLevelEntry const* siteLevel = leaderGarrison->GetSiteLevel();
        if (!siteLevel)
            return;

        // Teleport the visiting player to the leader's garrison map instance
        // The garrison map instance ID is the owner's GUID counter
        _player->TeleportTo(WorldLocation(siteLevel->MapID, *_player), TELE_TO_SEAMLESS);
    }
    else
    {
        // Player wants to leave the party garrison — teleport back to Draenor
        if (Garrison* ownGarrison = _player->GetGarrison())
            ownGarrison->Leave();
    }
}

void WorldSession::HandleQueryGarrisonPetName(WorldPackets::Garrison::QueryGarrisonPetName& queryGarrisonPetName)
{
    // SMSG_QUERY_GARRISON_PET_NAME_RESPONSE (0x4C0041) wire format is not visible in
    // observed WoD garrison traffic — the response packet would be safer to send only
    // once a sniff confirms its byte layout (likely { ObjectGuid NpcGUID; SizedString Name;
    // uint32 PetNameTimestamp; } based on the corresponding entity-update field). Until
    // then, leave as a no-op since the client tolerates the missing response (no UI path
    // observed waiting on it). RE-blocked, intentionally not stubbed with a guess.
    TC_LOG_DEBUG("garrison", "HandleQueryGarrisonPetName: Player {} queried pet name for NPC {}",
        _player->GetGUID().ToString().c_str(), queryGarrisonPetName.NpcGUID.ToString().c_str());
}

void WorldSession::HandleRequestGarrisonTalentWorldQuestUnlocks(WorldPackets::Garrison::RequestGarrisonTalentWorldQuestUnlocks& /*requestGarrisonTalentWorldQuestUnlocks*/)
{
    // SMSG_GARRISON_TALENT_WORLD_QUEST_UNLOCKS_RESPONSE (0x4C004E) — Legion+ talent-gated
    // map POIs. Wire format unsniffed; expected to be a { uint32 GarrTalentID;
    // uint32 MapPOIID; }[] array, sourced by joining researched talents against
    // GarrTalentMapPOI.db2. Same RE-blocked status as pet name query — leave as no-op
    // until sniff data lands. Client UI tolerates the missing response.
    TC_LOG_DEBUG("garrison", "HandleRequestGarrisonTalentWorldQuestUnlocks: Player {} requested talent world quest unlocks",
        _player->GetGUID().ToString().c_str());
}

void WorldSession::HandleGetTrophyList(WorldPackets::Garrison::GetTrophyList& /*getTrophyList*/)
{
    WorldPackets::Garrison::GetTrophyListResponse response;

    Garrison* garrison = _player->GetGarrison();
    if (garrison)
    {
        response.Success = true;
        for (uint32 trophyId : garrison->GetTrophies())
        {
            WorldPackets::Garrison::GarrisonTrophyData data;
            data.TrophyID = trophyId;
            response.Trophies.push_back(data);
        }
    }

    SendPacket(response.Write());
}

void WorldSession::HandleReplaceTrophy(WorldPackets::Garrison::ReplaceTrophy& replaceTrophy)
{
    WorldPackets::Garrison::ReplaceTrophyResponse response;

    Garrison* garrison = _player->GetGarrison();
    if (garrison)
    {
        garrison->AddTrophy(replaceTrophy.TrophyID);
        response.Success = true;

        WorldPackets::Garrison::GarrisonUpdateGarrisonMonumentSelections selections;
        for (uint32 trophyId : garrison->GetTrophies())
        {
            WorldPackets::Garrison::GarrisonTrophyData data;
            data.TrophyID = trophyId;
            selections.Trophies.push_back(data);
        }
        SendPacket(selections.Write());
    }

    SendPacket(response.Write());
}

void WorldSession::HandleLoadSelectedTrophy(WorldPackets::Garrison::LoadSelectedTrophy& loadSelectedTrophy)
{
    WorldPackets::Garrison::GetSelectedTrophyIDResponse response;

    Garrison* garrison = _player->GetGarrison();
    if (garrison && garrison->HasTrophy(loadSelectedTrophy.TrophyID))
    {
        response.TrophyID = loadSelectedTrophy.TrophyID;
        response.Success = true;
    }

    SendPacket(response.Write());
}

void WorldSession::HandleChangeMonumentAppearance(WorldPackets::Garrison::ChangeMonumentAppearance& changeMonumentAppearance)
{
    Garrison* garrison = _player->GetGarrison();
    if (garrison)
        garrison->AddTrophy(changeMonumentAppearance.TrophyID);
}

void WorldSession::HandleRevertMonumentAppearance(WorldPackets::Garrison::RevertMonumentAppearance& /*revertMonumentAppearance*/)
{
    Garrison* garrison = _player->GetGarrison();
    if (!garrison)
        return;

    // Clear all trophies from the garrison (revert to default monument appearance)
    std::vector<uint32> trophiesToRemove(garrison->GetTrophies().begin(), garrison->GetTrophies().end());
    for (uint32 trophyId : trophiesToRemove)
        garrison->RemoveTrophy(trophyId);

    // Send updated (empty) monument selections to the client
    WorldPackets::Garrison::GarrisonUpdateGarrisonMonumentSelections selections;
    SendPacket(selections.Write());
}
