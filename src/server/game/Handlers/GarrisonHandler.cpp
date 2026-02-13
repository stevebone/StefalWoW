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
#include "Garrison.h"
#include "GarrisonMgr.h"
#include "GarrisonPackets.h"
#include "Log.h"
#include "Player.h"

void WorldSession::HandleGetGarrisonInfo(WorldPackets::Garrison::GetGarrisonInfo& /*getGarrisonInfo*/)
{
    WorldPackets::Garrison::GetGarrisonInfoResult garrisonInfo;
    garrisonInfo.FactionIndex = Garrison::GetFaction(_player->GetTeam());

    if (Garrison* garrison = _player->GetGarrison())
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

    mission = garrison->GetMissionByRecID(garrisonCompleteMission.MissionRecID);
    if (mission)
    {
        completeResult.Mission = mission->PacketInfo;
        completeResult.Succeeded = mission->PacketInfo.SuccessChance > 0;
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

    garrison->ClaimMissionReward(garrisonGetMissionReward.MissionRecID);
}

void WorldSession::HandleOpenMissionNpc(WorldPackets::Garrison::OpenMissionNpc& openMissionNpc)
{
    Garrison* garrison = _player->GetGarrison();
    if (!garrison)
        return;

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

void WorldSession::HandleGarrisonRecruitFollower(WorldPackets::Garrison::GarrisonRecruitFollower& /*garrisonRecruitFollower*/)
{
    Garrison* garrison = _player->GetGarrison();
    if (!garrison)
        return;

    WorldPackets::Garrison::GarrisonRecruitFollowerResult result;
    result.Result = GARRISON_ERROR_RECRUITMENT_NPC_NOT_AVAILABLE;
    SendPacket(result.Write());
}

void WorldSession::HandleGarrisonGenerateRecruits(WorldPackets::Garrison::GarrisonGenerateRecruits& /*garrisonGenerateRecruits*/)
{
    Garrison* garrison = _player->GetGarrison();
    if (!garrison)
        return;

    WorldPackets::Garrison::GarrisonRecruitFollowerResult result;
    result.Result = GARRISON_ERROR_RECRUITMENT_NPC_NOT_AVAILABLE;
    SendPacket(result.Write());
}

void WorldSession::HandleGarrisonFullyHealAllFollowers(WorldPackets::Garrison::GarrisonFullyHealAllFollowers& /*garrisonFullyHealAllFollowers*/)
{
    Garrison* garrison = _player->GetGarrison();
    if (!garrison)
        return;

    // Heal all followers - clear exhaustion/fatigue status
    // This is handled by updating follower health and status
}

void WorldSession::HandleGarrisonAddFollowerHealth(WorldPackets::Garrison::GarrisonAddFollowerHealth& garrisonAddFollowerHealth)
{
    Garrison* garrison = _player->GetGarrison();
    if (!garrison)
        return;

    Garrison::Follower* follower = garrison->GetFollower(garrisonAddFollowerHealth.FollowerDBID);
    if (!follower)
        return;

    follower->PacketInfo.Health = std::min(follower->PacketInfo.Health + garrisonAddFollowerHealth.HealthToAdd, int32(5));

    WorldPackets::Garrison::GarrisonUpdateFollower updateFollower;
    updateFollower.Result = GARRISON_SUCCESS;
    updateFollower.Follower = follower->PacketInfo;
    SendPacket(updateFollower.Write());
}

void WorldSession::HandleGarrisonGetClassSpecCategoryInfo(WorldPackets::Garrison::GarrisonGetClassSpecCategoryInfo& /*garrisonGetClassSpecCategoryInfo*/)
{
    WorldPackets::Garrison::GarrisonGetClassSpecCategoryInfoResult result;
    SendPacket(result.Write());
}

void WorldSession::HandleGarrisonSetRecruitmentPreferences(WorldPackets::Garrison::GarrisonSetRecruitmentPreferences& /*garrisonSetRecruitmentPreferences*/)
{
    // Preferences are stored client-side mostly
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

    WorldPackets::Garrison::GarrisonUpgradeResult result;
    result.Result = GARRISON_ERROR_UPGRADE_CONDITION_FAILED;
    result.GarrSiteLevelID = garrison->GetSiteLevel() ? garrison->GetSiteLevel()->ID : 0;
    SendPacket(result.Write());
}

void WorldSession::HandleGarrisonCheckUpgradeable(WorldPackets::Garrison::GarrisonCheckUpgradeable& /*garrisonCheckUpgradeable*/)
{
    WorldPackets::Garrison::GarrisonIsUpgradeableResponse result;
    result.Result = GARRISON_ERROR_UPGRADE_CONDITION_FAILED;
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
    WorldPackets::Garrison::GarrisonSwapBuildingsResponse result;
    result.Result = GARRISON_ERROR_OPERATION_NOT_SUPPORTED;
    SendPacket(result.Write());
}

// ============================================================
// Talent handlers
// ============================================================

void WorldSession::HandleGarrisonLearnTalent(WorldPackets::Garrison::GarrisonLearnTalent& /*garrisonLearnTalent*/)
{
    WorldPackets::Garrison::GarrisonResearchTalentResult result;
    result.Result = GARRISON_ERROR_INVALID_TALENT;
    result.GarrTypeID = GARRISON_TYPE_GARRISON;
    SendPacket(result.Write());
}

void WorldSession::HandleGarrisonResearchTalent(WorldPackets::Garrison::GarrisonResearchTalent& /*garrisonResearchTalent*/)
{
    WorldPackets::Garrison::GarrisonResearchTalentResult result;
    result.Result = GARRISON_ERROR_INVALID_TALENT;
    result.GarrTypeID = GARRISON_TYPE_GARRISON;
    SendPacket(result.Write());
}

void WorldSession::HandleGarrisonSocketTalent(WorldPackets::Garrison::GarrisonSocketTalent& /*garrisonSocketTalent*/)
{
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
    // Shipments not yet implemented
}

void WorldSession::HandleSetUsingPartyGarrison(WorldPackets::Garrison::SetUsingPartyGarrison& /*setUsingPartyGarrison*/)
{
    // Party garrison sharing - store preference
}

void WorldSession::HandleQueryGarrisonPetName(WorldPackets::Garrison::QueryGarrisonPetName& /*queryGarrisonPetName*/)
{
    // Garrison pet name queries
}

void WorldSession::HandleRequestGarrisonTalentWorldQuestUnlocks(WorldPackets::Garrison::RequestGarrisonTalentWorldQuestUnlocks& /*requestGarrisonTalentWorldQuestUnlocks*/)
{
    // Talent world quest unlocks - send empty response
}
