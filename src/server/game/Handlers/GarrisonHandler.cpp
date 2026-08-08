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
#include "ConditionMgr.h"
#include "Creature.h"
#include "DB2Stores.h"
#include "DB2Structure.h"
#include "GameObject.h"
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

        // Send monument/trophy selections after map data (sniff-confirmed zone-in sequence). This is what puts
        // the statues on the plinths when the player zones into the garrison, before touching any monument.
        WorldPackets::Garrison::GarrisonUpdateGarrisonMonumentSelections selections;
        for (auto const& [trophyInstanceId, trophyId] : garrison->GetTrophies())
        {
            WorldPackets::Garrison::GarrisonMonumentSelection selection;
            selection.TrophyInstanceID = trophyInstanceId;
            selection.TrophyID = trophyId;
            selections.Selections.push_back(selection);
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
    // Resolve by follower, not by garrison type: order hall and covenant followers live in a different
    // garrison than the WoD one the no-arg GetGarrison() returns, and dismissing one of them silently no-opped.
    Garrison* garrison = _player->GetGarrisonWithFollower(garrisonRemoveFollower.FollowerDBID);
    if (!garrison)
        return;

    garrison->RemoveFollower(garrisonRemoveFollower.FollowerDBID);
}

void WorldSession::HandleGarrisonRenameFollower(WorldPackets::Garrison::GarrisonRenameFollower& garrisonRenameFollower)
{
    Garrison* garrison = _player->GetGarrisonWithFollower(garrisonRenameFollower.FollowerDBID);
    if (!garrison)
        return;

    garrison->RenameFollower(garrisonRenameFollower.FollowerDBID, garrisonRenameFollower.FollowerName);
}

void WorldSession::HandleGarrisonSetFollowerFavorite(WorldPackets::Garrison::GarrisonSetFollowerFavorite& garrisonSetFollowerFavorite)
{
    Garrison* garrison = _player->GetGarrisonWithFollower(garrisonSetFollowerFavorite.FollowerDBID);
    if (!garrison)
        return;

    garrison->SetFollowerFavorite(garrisonSetFollowerFavorite.FollowerDBID, garrisonSetFollowerFavorite.Favorite);
}

void WorldSession::HandleGarrisonSetFollowerInactive(WorldPackets::Garrison::GarrisonSetFollowerInactive& garrisonSetFollowerInactive)
{
    Garrison* garrison = _player->GetGarrisonWithFollower(garrisonSetFollowerInactive.FollowerDBID);
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
    // "Heal all" carries no discriminator on the wire, and healing followers is not WoD-only - it is the
    // Adventures (GarrType 111) and order-hall mechanic too (see HandleGarrisonAddFollowerHealth below, which
    // already loops). Resolving only the WoD garrison meant a covenant/order-hall owner got a silent no-op.
    for (auto const& [garrType, garrison] : _player->GetGarrisons())
    {
        garrison->HealAllFollowers();

        // Send individual GarrisonUpdateFollower packets for each follower
        // instead of a full GetGarrisonInfoResult to reduce bandwidth
        garrison->SendAllFollowerUpdates();
    }
}

void WorldSession::HandleGarrisonAddFollowerHealth(WorldPackets::Garrison::GarrisonAddFollowerHealth& garrisonAddFollowerHealth)
{
    // Follower DbIDs are unique across all of the character's garrisons, and healing a follower is an
    // Adventures (GarrType 111) mechanic - the no-arg GetGarrison() only ever searched the WoD
    // garrison, so a covenant companion could never be found here.
    Garrison::Follower* follower = nullptr;
    for (auto const& [garrType, garrison] : _player->GetGarrisons())
    {
        follower = garrison->GetFollower(garrisonAddFollowerHealth.FollowerDBID);
        if (follower)
            break;
    }

    if (!follower)
        return;

    // Cap at the follower's own maximum: the GarrAutoCombatant statline for Adventures companions,
    // the durability charge count for the older follower model that publishes no statline.
    GarrFollowerEntry const* followerEntry = sGarrFollowerStore.LookupEntry(follower->PacketInfo.GarrFollowerID);
    int32 maxHealth = Garrison::GetFollowerMaxHealth(followerEntry, follower->PacketInfo.FollowerLevel);
    if (!maxHealth)
        maxHealth = static_cast<int32>(follower->PacketInfo.Durability);

    follower->PacketInfo.Health = std::min(follower->PacketInfo.Health + garrisonAddFollowerHealth.HealthToAdd, maxHealth);

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

// Troop recruiters for the class Order Hall (GarrisonType 3) are ordinary world creatures, not part of the
// WoD garrison. The shipment CMSGs carry only the NPC GUID, and no-arg GetGarrison() resolves the WoD garrison
// (type 2) -- which is null for an order-hall-only character (e.g. a Hunter). Route a known order-hall recruiter
// to its type-3 garrison; everything else keeps the WoD default.
static Garrison* ResolveShipmentGarrison(Player* player, ObjectGuid npcGUID)
{
    if (Creature const* npc = ObjectAccessor::GetCreature(*player, npcGUID))
        if (sGarrisonMgr.GetShipmentContainerForNpc(npc->GetEntry()))
            return player->GetGarrison(GARRISON_TYPE_CLASS_ORDER);
    return player->GetGarrison();
}

void WorldSession::HandleGarrisonRequestShipmentInfo(WorldPackets::Garrison::GarrisonRequestShipmentInfo& garrisonRequestShipmentInfo)
{
    Garrison* garrison = ResolveShipmentGarrison(_player, garrisonRequestShipmentInfo.NpcGUID);
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
    Garrison* garrison = ResolveShipmentGarrison(_player, openShipmentNpc.NpcGUID);
    if (!garrison)
        return;

    // Collects finished orders + opens the crafter UI (shared with the crate GO's OnGossipHello).
    garrison->SendOpenShipmentUI(openShipmentNpc.NpcGUID);
}

void WorldSession::HandleCreateShipment(WorldPackets::Garrison::CreateShipment& createShipment)
{
    Garrison* garrison = ResolveShipmentGarrison(_player, createShipment.NpcGUID);
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
        // Player wants to leave the party garrison — teleport back out.
        // CMSG carries GarrTypeID (GarrisonPackets.h SetUsingPartyGarrison::GarrTypeID, parsed in its Read()),
        // and the enter branch above already resolves the leader's garrison with it; the leave branch resolved
        // only the WoD garrison, so leaving a non-WoD party garrison was a silent no-op.
        if (Garrison* ownGarrison = _player->GetGarrison(static_cast<GarrisonType>(setUsingPartyGarrison.GarrTypeID)))
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
    //
    // The CMSG carries no garrison type and the response carries exactly one (GarrTypeID), so this is
    // the same shape as CMSG_GET_LANDING_PAGE_SHIPMENTS: answer once per owned garrison. The previous
    // no-arg GetGarrison() resolves ONLY the WoD garrison (type 2), so an order-hall / war-campaign /
    // covenant character got nullptr and we replied with GarrTypeID 0 and an empty tree list - which
    // reads as "no talent trees unlocked" for every non-WoD garrison the character owns.
    for (auto const& [type, garrison] : _player->GetGarrisons())
    {
        WorldPackets::Garrison::GarrisonTalentWorldQuestUnlocksResponse response;
        response.GarrTypeID = static_cast<uint8>(garrison->GetType());

        // Build the list of unique talent tree IDs from the player's known talents.
        std::set<int32> trees;
        for (auto const& [talentID, talent] : garrison->GetAllTalents())
        {
            if (GarrTalentEntry const* entry = sGarrTalentStore.LookupEntry(talentID))
                trees.insert(entry->GarrTalentTreeID);
        }
        response.UnlockedTalentTreeIDs.assign(trees.begin(), trees.end());

        SendPacket(response.Write());
    }
}

// ============================================================
// Garrison monument trophies
// ============================================================
//
// How the 68275 client drives this, from Blizzard_GarrisonMonumentUI.lua + the C_Trophy namespace (9 functions,
// registered at client data RVA 0x420ED30, implemented in Source\Ui\TrophyInfo.cpp):
//
//   interact with a Monument Base  -> PlayerInteractionType::Trophy (36) opens GarrisonMonumentFrame
//   C_Trophy.MonumentLoadList()    -> CMSG_GET_TROPHY_LIST     -> the CATALOGUE for this monument
//   MonumentLoadSelectedTrophyID() -> CMSG_LOAD_SELECTED_TROPHY-> the player's CURRENT selection
//   arrow keys / MonumentChangeAppearanceToTrophyID(id)        -> CMSG_CHANGE_MONUMENT_APPEARANCE (preview only)
//   MonumentRevertAppearanceToSaved()                          -> CMSG_REVERT_MONUMENT_APPEARANCE (preview only)
//   MonumentSaveSelection(id)      -> CMSG_REPLACE_TROPHY      -> the ONLY call that persists anything
//
// The list and the selection are deliberately two round trips - the Lua calls MonumentLoadList() and then, on
// GARRISON_MONUMENT_LIST_LOADED, MonumentLoadSelectedTrophyID(), and compares the returned id against the list
// entries to find which one to highlight. Answering CMSG_GET_TROPHY_LIST with the player's own trophies (what
// this used to do) gave the browse UI nothing to browse.
//
// Which monument is being edited is on the wire for three of the five: CMSG_REPLACE_TROPHY,
// CMSG_CHANGE_MONUMENT_APPEARANCE and CMSG_REVERT_MONUMENT_APPEARANCE each open with the monument's PackedGuid
// (client serializers at RVA 0x6A9E90 / 0x6A9EF0 / 0x6A9F50), which TrinityCore was not reading at all - so
// CMSG_REPLACE_TROPHY was parsing its TrophyID out of the guid's mask bytes. CMSG_GET_TROPHY_LIST sends the
// monument's TrophyTypeID and CMSG_LOAD_SELECTED_TROPHY its TrophyInstanceID instead. Every handler below
// still checks the guid against the interaction GameObject::Use opened, because a guid off the wire is a
// claim rather than a fact.

namespace
{
// Resolve the monument gameobject a monument packet names. The three monument CMSGs all carry the monument's
// PackedGuid, but a guid off the wire is a claim, not a fact - it is only accepted if the player actually has
// an open Trophy interaction with that exact object, which GameObject::Use established.
GameObject* GetMonument(Player* player, ObjectGuid const& monumentGuid)
{
    if (!player->PlayerTalkClass->GetInteractionData().IsInteractingWith(monumentGuid, PlayerInteractionType::Trophy))
        return nullptr;

    GameObject* monument = ObjectAccessor::GetGameObject(*player, monumentGuid);
    if (!monument || monument->GetGoType() != GAMEOBJECT_TYPE_GARRISON_MONUMENT)
        return nullptr;

    return monument;
}

// The monument the player currently has open, for the packets that do not name one.
GameObject* GetInteractedMonument(Player* player)
{
    InteractionData const& interaction = player->PlayerTalkClass->GetInteractionData();
    if (interaction.Type != PlayerInteractionType::Trophy)
        return nullptr;

    return GetMonument(player, interaction.SourceGuid);
}

// Trophies live on the WoD garrison. This is a data fact rather than the usual "someone forgot the argument"
// default: the only GAMEOBJECT_TYPE_GARRISON_MONUMENT objects in the world are the six Monument Bases on maps
// 1159 (Lunarfall) and 1153 (Frostwall), and every displayable Trophy.db2 row is TrophyTypeID 3 or 4, which are
// exactly those two garrisons. A character with no WoD garrison therefore genuinely has nowhere to put a trophy.
Garrison* GetMonumentGarrison(Player* player)
{
    return player->GetGarrison(GARRISON_TYPE_GARRISON);
}

// Push the garrison's saved monument selections to the client. This is what actually makes a trophy appear:
// the client keeps this array and its monument tooltip/display resolves a monument by matching its own
// TrophyInstanceID against it, then reads Trophy.db2 for the statue.
void SendMonumentSelections(WorldSession* session, Garrison const* garrison)
{
    WorldPackets::Garrison::GarrisonUpdateGarrisonMonumentSelections selections;
    if (garrison)
    {
        for (auto const& [trophyInstanceId, trophyId] : garrison->GetTrophies())
        {
            WorldPackets::Garrison::GarrisonMonumentSelection selection;
            selection.TrophyInstanceID = trophyInstanceId;
            selection.TrophyID = trophyId;
            selections.Selections.push_back(selection);
        }
    }

    session->SendPacket(selections.Write());
}
}

void WorldSession::HandleGetTrophyList(WorldPackets::Garrison::GetTrophyList& getTrophyList)
{
    WorldPackets::Garrison::GetTrophyListResponse response;

    // TrophyTypeID DOES partition the reply, and it is not a garrison type. Trophy.db2 carries a TrophyTypeID and
    // so does the monument gameobject (GAMEOBJECT_TYPE_GARRISON_MONUMENT Data0). In the 68275 client Trophy.db2
    // has 16 rows: 7 of TrophyTypeID 3, 7 of TrophyTypeID 4 and 2 of TrophyTypeID 0 (NoValue, displayable
    // nowhere); the six spawned monuments are Data0 = 3 in Frostwall and Data0 = 4 in Lunarfall. The pairs mirror
    // each other - "Master of Apexis" is row 1 at type 3 and row 14 at type 4, same unlock, different statue - so
    // replying with the union would offer a Horde player the Alliance statues.
    //
    // The reply is the full catalogue for the requested type, INCLUDING trophies the player has not unlocked.
    // That is not laziness: the Lua walks 1..MonumentGetCount() and draws a lock overlay plus
    // GARRISON_TROPHY_LOCKED_SUBTEXT and the blocking achievement's name on each entry it cannot use, and it
    // refuses to call MonumentSaveSelection for one. If the server pre-filtered, that entire path would be dead
    // code. The client decides lock state itself from Trophy.PlayerConditionID; the server's job is to enforce it
    // on save, which HandleReplaceTrophy does.
    // Confirmed against the client: its response handler (RVA 0x24A09A0) copies the list verbatim into a global
    // and never compares anything to the TrophyTypeID it asked for - it does not even keep the requested value.
    // So the filtering has to happen here or not at all.
    for (TrophyEntry const* trophy : sTrophyStore)
        if (trophy->TrophyTypeID == getTrophyList.TrophyTypeID)
        {
            WorldPackets::Garrison::TrophyInfo info;
            info.TrophyID = trophy->ID;
            // Unk1/Unk2 are the Lua's lock_code and lock_reason. Which is which, and what value means
            // "unlocked", is not derivable offline: MATCH_CONDITION_SUCCESS (57) and
            // MATCH_CONDITION_WRONG_ACHIEVEMENT (34) are client constants that no server code in this build
            // is known to produce, and no JAM descriptor names these fields. Sending 0 rather than guessing
            // 57 - a wrong guess would silently mislabel every trophy's lock state.
            response.Trophies.push_back(info);
        }

    // We answered the question that was asked. An empty list for a TrophyTypeID with no rows is a real answer,
    // not a failure - Success = false means "the list could not be retrieved".
    response.Success = true;

    SendPacket(response.Write());
}

void WorldSession::HandleReplaceTrophy(WorldPackets::Garrison::ReplaceTrophy& replaceTrophy)
{
    // C_Trophy.MonumentSaveSelection(trophyID) - the only trophy opcode that changes persisted state, and so the
    // only one that has to revalidate. The Lua checks the lock before calling this, but it will happily PREVIEW a
    // locked trophy through MonumentChangeAppearanceToTrophyID first, so the check cannot live in the client.
    // This previously stored whatever uint32 arrived, unvalidated, straight into character_garrison_trophies.
    WorldPackets::Garrison::ReplaceTrophyResponse response;
    response.Success = false;

    GameObject* monument = GetMonument(_player, replaceTrophy.MonumentGUID);
    TrophyEntry const* trophy = sTrophyStore.LookupEntry(replaceTrophy.TrophyID);
    Garrison* garrison = GetMonumentGarrison(_player);

    if (monument && trophy && garrison
        // the trophy has to belong to the monument being edited, not merely exist
        && trophy->TrophyTypeID == monument->GetGOInfo()->garrisonMonument.TrophyTypeID
        // and the player has to have unlocked it.
        //
        // READ THIS BEFORE ASSUMING THE GATE BITES. Trophy.PlayerConditionID is the unlock, and it is real
        // client data - all 16 rows carry a non-zero one, and the mirrored faction pairs share it (rows 1 and
        // 14, both "Master of Apexis", are both PlayerCondition 28227). But of the 9 distinct conditions the
        // table references, only 24827 still EXISTS in the 68275 PlayerCondition.db2; the other 8 - every one
        // used by a displayable TrophyTypeID 3 or 4 row - are dangling ids Blizzard deleted while leaving
        // Trophy.db2 behind. Verified by parsing the client file directly: its unencrypted section 0 ends
        // byte-exactly at section 1's offset, and neighbouring ids (27767, 27798, 28231, 28232 ...) resolve
        // fine while the trophy ones do not; none of them is hiding in the 8 encrypted sections either.
        //
        // IsPlayerMeetingCondition returns true for a missing condition (ConditionMgr.cpp), so today this
        // gate passes for every trophy. That is deliberately NOT a special case here: the client evaluates
        // the same PlayerConditionID out of the same file to decide whether to draw the lock, so it also
        // sees "no condition" and shows the trophy as unlocked. Denying server-side would mean refusing a
        // selection the client just told the player was available. If the 8 rows are ever restored - a TDB
        // update, or authoring them in `integ_hotfixes.player_condition` - this gate starts biting with no
        // code change, and so does the client's lock overlay.
        && ConditionMgr::IsPlayerMeetingCondition(_player, trophy->PlayerConditionID))
    {
        // Keyed by the monument's own TrophyInstanceID, so the three plinths in a garrison hold three
        // independent selections and re-selecting on one replaces only that one.
        garrison->SetSelectedTrophy(monument->GetGOInfo()->garrisonMonument.TrophyInstanceID, trophy->ID);
        response.Success = true;
        SendMonumentSelections(this, garrison);
    }

    SendPacket(response.Write());
}

void WorldSession::HandleLoadSelectedTrophy(WorldPackets::Garrison::LoadSelectedTrophy& loadSelectedTrophy)
{
    // C_Trophy.MonumentLoadSelectedTrophyID() takes no argument in Lua - the client is asking us what is on a
    // monument, so the uint32 it sends cannot be a trophy it is nominating. It is the monument's
    // TrophyInstanceID, which the client reads out of the gameobject's Data1. The old code treated it as a
    // Trophy.db2 id and echoed it straight back if the player "had" it, which answered a different question.
    WorldPackets::Garrison::GetSelectedTrophyIDResponse response;

    // Still require an open interaction: this reveals what a character has configured, and the instance id
    // alone is guessable (1, 2, 6).
    if (GetInteractedMonument(_player))
    {
        if (Garrison* garrison = GetMonumentGarrison(_player))
            response.TrophyID = garrison->GetSelectedTrophy(loadSelectedTrophy.TrophyInstanceID);

        // We knew which monument was asked about and answered for it. TrophyID 0 is the honest "nothing is
        // selected here" - the client has a GARRISON_TROPHY_NOT_SELECTED_TOOLTIP for exactly that case.
        response.Success = true;
    }

    SendPacket(response.Write());
}

void WorldSession::HandleChangeMonumentAppearance(WorldPackets::Garrison::ChangeMonumentAppearance& /*changeMonumentAppearance*/)
{
    // C_Trophy.MonumentChangeAppearanceToTrophyID(trophyID) is the arrow-key PREVIEW: the Lua calls it for every
    // trophy the player scrolls past, including locked ones, and then either commits with MonumentSaveSelection
    // or throws the preview away with MonumentRevertAppearanceToSaved.
    //
    // It must therefore not persist anything. It used to call Garrison::AddTrophy, so merely browsing the list
    // permanently added every trophy scrolled past - locked ones included - to character_garrison_trophies.
    //
    // What the server should do instead is unresolved: whether the previewed statue is meant to be visible to
    // other players (which would need the monument gameobject's display to be swapped and reverted) is not
    // derivable from the client, and the packet has no response. Doing nothing keeps the preview client-local,
    // which is correct for the player previewing it and cannot corrupt saved state.
}

void WorldSession::HandleRevertMonumentAppearance(WorldPackets::Garrison::RevertMonumentAppearance& revertMonumentAppearance)
{
    // C_Trophy.MonumentRevertAppearanceToSaved() - "throw away the preview and go back to what is saved". The Lua
    // calls it on frame close and whenever the player backs out of a locked selection.
    //
    // It used to delete every trophy the character had and reply with an empty selection list, i.e. closing the
    // monument window wiped the saved selection it was supposed to be restoring. Re-sending what is saved is the
    // whole job.
    if (!GetMonument(_player, revertMonumentAppearance.MonumentGUID))
        return;

    SendMonumentSelections(this, GetMonumentGarrison(_player));
}

void WorldSession::HandleGarrisonSocketTalent(WorldPackets::Garrison::GarrisonSocketTalent& packet)
{
    // Socket a conduit into a garrison/soulbind talent node. Validated against the talent's tree + the player's
    // garrison of that type, then persisted through the garrison (character_garrison_talents SoulbindConduitID/Rank).
    //
    // Two persistence paths exist and both are needed - they are complementary, not alternatives:
    //   * Garrison::SocketTalent    -> character_garrison_talents (the generic garrison-talent path, all GarrTypes)
    //   * Player::SocketConduit     -> character_soulbind_conduit_socket + applies the conduit's spell, and is the
    //                                  only path that validates conduit ownership and covenant match
    // A Covenant (GarrType 111) soulbind tree therefore runs BOTH; every other garrison type runs the garrison path
    // only. The conduit path deliberately runs before the garrison lookup, because a covenant soulbind tree is edited
    // through the soulbind UI and must keep working even if the player has no GARRISON_TYPE_COVENANT garrison object.
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

    uint32 const treeId = uint32(talentEntry->GarrTalentTreeID);

    // Covenant/soulbind path. The client only edits the currently-active soulbind's tree, so accept the node either
    // because its tree is a Covenant tree or because it is the active soulbind's tree. Each socket is validated
    // server-side (conduit exists, is owned, its covenant matches) inside Player::SocketConduit, which fails closed.
    bool socketedConduit = false;
    if (treeEntry->GarrTypeID == GARRISON_TYPE_COVENANT)
        socketedConduit = true;
    else if (SoulbindEntry const* soulbind = sSoulbindStore.LookupEntry(_player->GetActiveSoulbind()))
        socketedConduit = uint32(soulbind->GarrTalentTreeID) == treeId;

    if (socketedConduit)
        for (WorldPackets::Garrison::GarrisonTalentSocketData const& socket : packet.Sockets)
            _player->SocketConduit(treeId, uint32(packet.GarrTalentID), uint32(socket.SoulbindConduitID));

    Garrison* garrison = _player->GetGarrison(static_cast<GarrisonType>(treeEntry->GarrTypeID));
    if (!garrison)
    {
        // Already handled as a soulbind conduit socket - a missing garrison object is not an error in that case.
        if (socketedConduit)
            return;

        WorldPackets::Garrison::GarrisonResearchTalentResult result;
        result.Result = GARRISON_ERROR_NO_GARRISON;
        result.GarrTypeID = treeEntry->GarrTypeID;
        SendPacket(result.Write());
        return;
    }

    for (WorldPackets::Garrison::GarrisonTalentSocketData const& socket : packet.Sockets)
        garrison->SocketTalent(packet.GarrTalentID, socket.SoulbindConduitID, socket.SoulbindConduitRank);
}
