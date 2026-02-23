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
#include "GameTime.h"
#include "Garrison.h"
#include "GarrisonMgr.h"
#include "GarrisonPackets.h"
#include "Log.h"
#include "Player.h"
#include "Random.h"

void WorldSession::HandleGetGarrisonInfo(WorldPackets::Garrison::GetGarrisonInfo& /*getGarrisonInfo*/)
{
    WorldPackets::Garrison::GetGarrisonInfoResult garrisonInfo;
    garrisonInfo.FactionIndex = Garrison::GetFaction(_player->GetTeam());

    for (auto const& [type, garrison] : _player->GetGarrisons())
        garrison->BuildInfoPacket(garrisonInfo.Garrisons.emplace_back());

    SendPacket(garrisonInfo.Write());
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
    if (Garrison* garrison = _player->GetGarrison())
        garrison->SendBlueprintAndSpecializationData();
}

void WorldSession::HandleGarrisonGetMapData(WorldPackets::Garrison::GarrisonGetMapData& /*garrisonGetMapData*/)
{
    if (Garrison* garrison = _player->GetGarrison())
        garrison->SendMapData(_player);
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
    if (Garrison::Mission const* mission = garrison->GetMissionByRecID(garrisonStartMission.MissionRecID))
        startResult.Mission = mission->PacketInfo;
    startResult.FollowerDBIDs = garrisonStartMission.FollowerDBIDs;
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

    // After claiming, send updated garrison info so client refreshes
    if (result == GARRISON_SUCCESS)
    {
        WorldPackets::Garrison::GetGarrisonInfoResult garrisonInfo;
        garrisonInfo.FactionIndex = Garrison::GetFaction(_player->GetTeam());
        garrison->BuildInfoPacket(garrisonInfo.Garrisons.emplace_back());
        SendPacket(garrisonInfo.Write());
    }
}

void WorldSession::HandleOpenMissionNpc(WorldPackets::Garrison::OpenMissionNpc& /*openMissionNpc*/)
{
    Garrison* garrison = _player->GetGarrison();
    if (!garrison)
        return;

    // Remove expired offers and generate new missions if needed
    garrison->RemoveExpiredMissions();
    garrison->GenerateAvailableMissions();

    // Send updated garrison info so client refreshes mission list
    WorldPackets::Garrison::GetGarrisonInfoResult garrisonInfo;
    garrisonInfo.FactionIndex = Garrison::GetFaction(_player->GetTeam());
    garrison->BuildInfoPacket(garrisonInfo.Garrisons.emplace_back());
    SendPacket(garrisonInfo.Write());
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

    // Send updated garrison info so client sees healed followers
    WorldPackets::Garrison::GetGarrisonInfoResult garrisonInfo;
    garrisonInfo.FactionIndex = Garrison::GetFaction(_player->GetTeam());
    garrison->BuildInfoPacket(garrisonInfo.Garrisons.emplace_back());
    SendPacket(garrisonInfo.Write());
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

void WorldSession::HandleGarrisonSetRecruitmentPreferences(WorldPackets::Garrison::GarrisonSetRecruitmentPreferences& /*garrisonSetRecruitmentPreferences*/)
{
    // Recruitment preferences affect which ability types appear on generated recruits
    // Stored on the Garrison object but primarily influences GenerateRecruits()
    // For now, preferences are applied client-side and the server generates random recruits
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

void WorldSession::HandleGarrisonCheckUpgradeable(WorldPackets::Garrison::GarrisonCheckUpgradeable& /*garrisonCheckUpgradeable*/)
{
    Garrison* garrison = _player->GetGarrison();
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

void WorldSession::HandleGarrisonSwapBuildings(WorldPackets::Garrison::GarrisonSwapBuildings& /*garrisonSwapBuildings*/)
{
    // Building swapping is not supported in the current garrison implementation
    WorldPackets::Garrison::GarrisonSwapBuildingsResponse result;
    result.Result = GARRISON_ERROR_OPERATION_NOT_SUPPORTED;
    SendPacket(result.Write());
}

// ============================================================
// Talent handlers
// ============================================================

void WorldSession::HandleGarrisonLearnTalent(WorldPackets::Garrison::GarrisonLearnTalent& /*garrisonLearnTalent*/)
{
    // Garrison talents require GarrTalent DB2 data which is not yet loaded
    WorldPackets::Garrison::GarrisonResearchTalentResult result;
    result.Result = GARRISON_ERROR_INVALID_TALENT;
    result.GarrTypeID = GARRISON_TYPE_GARRISON;
    SendPacket(result.Write());
}

void WorldSession::HandleGarrisonResearchTalent(WorldPackets::Garrison::GarrisonResearchTalent& /*garrisonResearchTalent*/)
{
    // Garrison talents require GarrTalent DB2 data which is not yet loaded
    WorldPackets::Garrison::GarrisonResearchTalentResult result;
    result.Result = GARRISON_ERROR_INVALID_TALENT;
    result.GarrTypeID = GARRISON_TYPE_GARRISON;
    SendPacket(result.Write());
}

void WorldSession::HandleGarrisonSocketTalent(WorldPackets::Garrison::GarrisonSocketTalent& /*garrisonSocketTalent*/)
{
    // Garrison talents require GarrTalent DB2 data which is not yet loaded
    WorldPackets::Garrison::GarrisonResearchTalentResult result;
    result.Result = GARRISON_ERROR_INVALID_TALENT;
    result.GarrTypeID = GARRISON_TYPE_GARRISON;
    SendPacket(result.Write());
}

// ============================================================
// Other utility handlers
// ============================================================

void WorldSession::HandleGarrisonRequestShipmentInfo(WorldPackets::Garrison::GarrisonRequestShipmentInfo& /*garrisonRequestShipmentInfo*/)
{
    // Shipments (work orders) require CharShipment DB2 data
    // Send empty response to prevent client errors
}

void WorldSession::HandleSetUsingPartyGarrison(WorldPackets::Garrison::SetUsingPartyGarrison& /*setUsingPartyGarrison*/)
{
    // Party garrison sharing preference - client tracks this state
}

void WorldSession::HandleQueryGarrisonPetName(WorldPackets::Garrison::QueryGarrisonPetName& /*queryGarrisonPetName*/)
{
    // Garrison pet name query - used for stables building pets
}

void WorldSession::HandleRequestGarrisonTalentWorldQuestUnlocks(WorldPackets::Garrison::RequestGarrisonTalentWorldQuestUnlocks& /*requestGarrisonTalentWorldQuestUnlocks*/)
{
    // Talent-gated world quest unlocks - not applicable for WoD garrisons
}
