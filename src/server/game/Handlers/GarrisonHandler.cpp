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
#include "Creature.h"
#include "DB2Stores.h"
#include "DB2Structure.h"
#include "GameTime.h"
#include "Garrison.h"
#include "GarrisonMgr.h"
#include "GossipDef.h"
#include "GarrisonPackets.h"
#include "Group.h"
#include "NPCPackets.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Random.h"
#include <set>

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
    Garrison* garrison = _player->GetGarrisonWithMission(garrisonStartMission.MissionRecID);
    if (!garrison)
        return;

    GarrisonError result = garrison->StartMission(garrisonStartMission.MissionRecID, garrisonStartMission.FollowerDBIDs);

    WorldPackets::Garrison::GarrisonStartMissionResult startResult;
    startResult.Result = result;
    if (result == GARRISON_SUCCESS)
        startResult.NumOfferedToday = static_cast<uint16>(garrison->GetAndIncrementSessionMissionCount());
    if (Garrison::Mission const* mission = garrison->GetMissionByRecID(garrisonStartMission.MissionRecID))
        startResult.Mission = mission->PacketInfo;

    startResult.FollowerInfos.reserve(garrisonStartMission.FollowerDBIDs.size());
    for (uint64 dbId : garrisonStartMission.FollowerDBIDs)
    {
        WorldPackets::Garrison::GarrisonMissionFollowerEntry entry;
        entry.DbID = dbId;
        // BoardIndex/Health/HasFollowerEntry mirror the CMSG values; defaults match the
        // shape of what the client sends in CMSG. The full GarrisonFollower trailer
        // (Followers vector) stays empty in the standard "mission accepted" response.
        startResult.FollowerInfos.push_back(entry);
    }
    SendPacket(startResult.Write());
}

void WorldSession::HandleGarrisonCompleteMission(WorldPackets::Garrison::GarrisonCompleteMission& garrisonCompleteMission)
{
    Garrison* garrison = _player->GetGarrisonWithMission(garrisonCompleteMission.MissionRecID);
    if (!garrison)
        return;

    Garrison::Mission const* mission = garrison->GetMissionByRecID(garrisonCompleteMission.MissionRecID);
    if (!mission)
        return;

    GarrisonError result = garrison->CompleteMission(garrisonCompleteMission.MissionRecID);

    WorldPackets::Garrison::GarrisonCompleteMissionResult completeResult;
    completeResult.Result = result;
    completeResult.MissionRecID = garrisonCompleteMission.MissionRecID;
    completeResult.GarrTypeID = static_cast<uint8>(garrison->GetType());

    // Re-fetch mission after completion (state may have changed)
    mission = garrison->GetMissionByRecID(garrisonCompleteMission.MissionRecID);
    bool succeeded = false;
    if (mission)
    {
        completeResult.Mission = mission->PacketInfo;
        // Report the outcome CompleteMission already rolled and stored — do NOT roll again here, or the
        // banner the player sees could disagree with the rewards granted at finalize.
        completeResult.Succeeded = mission->Succeeded;
        succeeded = mission->Succeeded;
    }

    // FollowerInfos / Rounds left empty: no auto-combat replay generated for non-auto
    // missions. The auto-combat simulator (GarrisonAutoCombat::ProcessTurn) populates
    // these for Legion+ class hall and Shadowlands covenant missions. WoD-era classic
    // missions don't drive the replay UI so they ship empty arrays here.

    SendPacket(completeResult.Write());

    // On FAILURE the WoD client sends no bonus roll (there is no chest to open), so finalize the mission
    // now: follower XP is still awarded, followers are freed and the mission is removed. On SUCCESS we
    // wait for CMSG_GARRISON_MISSION_BONUS_ROLL (the chest open) to grant rewards and remove the mission.
    if (result == GARRISON_SUCCESS && !succeeded)
    {
        GarrisonError finalizeResult = garrison->FinalizeMission(garrisonCompleteMission.MissionRecID, false);

        // The complete-result banner does not tell the client the mission record is gone, so without an
        // explicit delete the failed mission lingers and reappears on the next scouting-map open. Mirror
        // the reward path (HandleGarrisonGetMissionReward) and send a targeted deletion.
        WorldPackets::Garrison::GarrisonDeleteMissionResult deleteMissionResult;
        deleteMissionResult.Result = finalizeResult;
        deleteMissionResult.MissionRecID = garrisonCompleteMission.MissionRecID;
        deleteMissionResult.GarrTypeID = garrison->GetType();
        SendPacket(deleteMissionResult.Write());
    }
}

void WorldSession::HandleGarrisonMissionBonusRoll(WorldPackets::Garrison::GarrisonMissionBonusRoll& garrisonMissionBonusRoll)
{
    Garrison* garrison = _player->GetGarrisonWithMission(garrisonMissionBonusRoll.MissionRecID);
    if (!garrison)
        return;

    WorldPackets::Garrison::GarrisonMissionBonusRollResult bonusResult;
    bonusResult.MissionRecID = garrisonMissionBonusRoll.MissionRecID;

    // Snapshot the mission (including its overmax/chest rewards) BEFORE finalizing — MissionBonusRoll grants
    // the rewards and removes the mission, so the record is gone afterwards and the chest reveal needs it.
    if (Garrison::Mission const* mission = garrison->GetMissionByRecID(garrisonMissionBonusRoll.MissionRecID))
        bonusResult.Mission = mission->PacketInfo;

    bonusResult.Result = garrison->MissionBonusRoll(garrisonMissionBonusRoll.MissionRecID);

    SendPacket(bonusResult.Write());
}

void WorldSession::HandleGarrisonGetMissionReward(WorldPackets::Garrison::GarrisonGetMissionReward& garrisonGetMissionReward)
{
    Garrison* garrison = _player->GetGarrisonWithMission(garrisonGetMissionReward.MissionRecID);
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
    if (_player->GetGarrisons().empty())
        return;

    // Match the retail WoD open sequence EXACTLY (sniff 66102 + 68275 garrisonlevel2upgrade):
    // the client already entered the GarrMission interaction from the gossip select
    // (SMSG_GOSSIP_OPTION_NPC_INTERACTION / GossipNpcOptionID 30323). The ONLY server->client
    // garrison packet retail sends in response to CMSG_OPEN_MISSION_NPC is
    // SMSG_DELETE_EXPIRED_MISSIONS_RESULT, immediately followed by SMSG_GOSSIP_COMPLETE.
    //
    // Retail delivers the mission board once at login via GET_GARRISON_INFO and the frame reads it from
    // cache; re-sending the whole board on every open (GenerateAvailableMissions + SendOfferedMissions +
    // SendMissionStartConditionUpdate) is a non-retail ADD_MISSION_RESULT burst that floods
    // GARRISON_MISSION_LIST_UPDATE and is the suspected cause of the client not firing its legacy
    // open-event. So the default open sends only SMSG_DELETE_EXPIRED_MISSIONS_RESULT then GOSSIP_COMPLETE.
    for (auto const& [type, garr] : _player->GetGarrisons())
        garr->SendDeleteExpiredMissionsResult();

    // Exception: when the board is already at its cap, the periodic GenerateAvailableMissions has nothing
    // to add, so no ADD_MISSION_RESULT reaches the client on open and the table can appear empty for a
    // garrison sitting at 15 offered missions. Re-send the existing offers ONLY in that full-pool case.
    // While the board is still filling, GenerateAvailableMissions trickles new missions (each with its own
    // ADD_MISSION_RESULT), so an extra full re-send here would be redundant and reintroduce the burst.
    for (auto const& [type, garr] : _player->GetGarrisons())
        if (garr->IsOfferPoolFull())
            garr->SendOfferedMissions();

    _player->PlayerTalkClass->SendCloseGossip();
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
    {
        if (Garrison::Follower const* follower = garrison->GetFollowerByEntry(followerID))
            recruitResult.Follower = follower->PacketInfo;
    }
    SendPacket(recruitResult.Write());
}

void WorldSession::HandleGarrisonGenerateRecruits(WorldPackets::Garrison::GarrisonGenerateRecruits& /*garrisonGenerateRecruits*/)
{
    Garrison* garrison = _player->GetGarrison();
    if (!garrison)
        return;

    uint32 faction = static_cast<uint32>(Garrison::GetFaction(_player->GetTeam()));
    garrison->GenerateRecruits(faction);

    // SMSG_GARRISON_GENERATE_FOLLOWERS_RESULT (§8.42): exactly 3 inline GarrisonFollowers.
    auto const& recruits = garrison->GetAvailableRecruits();
    WorldPackets::Garrison::GarrisonGenerateFollowersResult result;
    for (size_t i = 0; i < result.Followers.size(); ++i)
    {
        if (i < recruits.size())
            result.Followers[i] = recruits[i];
        // Slots beyond the rolled count stay default-constructed (empty follower record).
    }
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
            // The client's upgrade button (C_Garrison.CanUpgradeGarrison) reflects whether an upgrade is
            // AVAILABLE, not whether it is currently affordable. Retail enables the button whenever a next
            // site level exists and shows the cost; affordability is enforced only at purchase time
            // (HandleUpgradeGarrison already checks gold + Garrison Resources). Gating this response on
            // affordability left the Architect's upgrade button greyed/"locked" whenever the player was
            // short on Garrison Resources - even after finishing the prerequisite quests.
            if (nextLevel)
                upgradeResult = GARRISON_SUCCESS;
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

    // Collects finished orders + opens the crafter UI (shared with the crate GO's OnGossipHello).
    garrison->SendOpenShipmentUI(openShipmentNpc.NpcGUID);
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
    // A character may own several garrisons (WoD garrison type 2, Legion order hall type 3, BfA war campaign,
    // covenant sanctum). The CMSG carries no type, and the no-arg GetGarrison() resolves ONLY the WoD garrison
    // (type 2) -- so an order-hall-only character got null here and we never sent the response. That left the
    // client's GARRISON_LANDINGPAGE_SHIPMENTS event unfired, so the class-hall report never rebuilt its shipment
    // list (and thus never showed the talent-research progress bar even though the research data was correct).
    // Send for every owned garrison; the client's report filters shipments to its own garrison type.
    for (auto const& [type, garrison] : _player->GetGarrisons())
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
    // IDA case 4980801 (§8.51 pet name): {ObjectGuid NpcGUID, SizedString PetName}.
    // Look up the queried NPC and echo back its custom name (if any) — for non-pet NPCs
    // or NPCs without a stored custom name, send an empty string.
    WorldPackets::Garrison::QueryGarrisonPetNameResponse response;
    response.NpcGUID = queryGarrisonPetName.NpcGUID;
    if (Creature const* creature = ObjectAccessor::GetCreature(*_player, queryGarrisonPetName.NpcGUID))
    {
        // Garrison pets/bodyguards may carry a custom name on the creature template or summon.
        // Until the BattlePet/garrison-pet system is wired up, echo the creature's localized name
        // as a sane default. Empty string is also a valid response per IDA pseudocode.
        if (CreatureTemplate const* tmpl = creature->GetCreatureTemplate())
            response.PetName = tmpl->Name;
    }
    SendPacket(response.Write());
}

void WorldSession::HandleRequestGarrisonTalentWorldQuestUnlocks(WorldPackets::Garrison::RequestGarrisonTalentWorldQuestUnlocks& /*requestGarrisonTalentWorldQuestUnlocks*/)
{
    // SMSG_GARRISON_TALENT_WORLD_QUEST_UNLOCKS_RESPONSE (0x4C004E) — Legion+ talent-gated
    // map POIs. IDA dispatcher uses opaque helper so exact field shape is unconfirmed; the
    // best conservative match is a size-prefixed list of unlocked talent tree IDs (the
    // server's view of which trees the player has unlocked talents in for world-quest UI).
    WorldPackets::Garrison::GarrisonTalentWorldQuestUnlocksResponse response;
    if (Garrison* garrison = _player->GetGarrison())
    {
        response.GarrTypeID = static_cast<uint8>(garrison->GetType());
        // Build the list of unique talent tree IDs from the player's known talents.
        std::set<int32> trees;
        for (auto const& [talentID, talent] : garrison->GetAllTalents())
        {
            if (GarrTalentEntry const* entry = sGarrTalentStore.LookupEntry(talentID))
                trees.insert(entry->GarrTalentTreeID);
        }
        response.UnlockedTalentTreeIDs.assign(trees.begin(), trees.end());
    }
    SendPacket(response.Write());
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

void WorldSession::HandleGarrisonSocketTalent(WorldPackets::Garrison::GarrisonSocketTalent& packet)
{
    // Socket a conduit into a garrison/soulbind talent node. Validated against the talent's tree + the player's
    // garrison of that type, then persisted through the garrison (character_garrison_talents SoulbindConduitID/Rank).
    // NOTE: the integration branch routed this through the Covenant/Soulbind feature (Player::SocketConduit); that
    // feature is not part of this garrison branch, so socketing goes straight through Garrison::SocketTalent here.
    GarrTalentEntry const* talentEntry = sGarrTalentStore.LookupEntry(packet.GarrTalentID);
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

    for (WorldPackets::Garrison::GarrisonTalentSocketData const& socket : packet.Sockets)
        garrison->SocketTalent(packet.GarrTalentID, socket.SoulbindConduitID, socket.SoulbindConduitRank);
}
