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
#include "BattlePetMgr.h"
#include "BattlePetPackets.h"
#include "Creature.h"
#include "DB2Stores.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "PetBattleMgr.h"
#include "Player.h"
#include "TemporarySummon.h"

void WorldSession::HandleBattlePetRequestJournal(WorldPackets::BattlePet::BattlePetRequestJournal& /*battlePetRequestJournal*/)
{
    GetBattlePetMgr()->SendJournal();
}

void WorldSession::HandleBattlePetRequestJournalLock(WorldPackets::BattlePet::BattlePetRequestJournalLock& /*battlePetRequestJournalLock*/)
{
    GetBattlePetMgr()->SendJournalLockStatus();

    if (GetBattlePetMgr()->HasJournalLock())
        GetBattlePetMgr()->SendJournal();
}

void WorldSession::HandleBattlePetSetBattleSlot(WorldPackets::BattlePet::BattlePetSetBattleSlot& battlePetSetBattleSlot)
{
    if (BattlePets::BattlePet* pet = GetBattlePetMgr()->GetPet(battlePetSetBattleSlot.PetGuid))
        if (WorldPackets::BattlePet::BattlePetSlot* slot = GetBattlePetMgr()->GetSlot(BattlePets::BattlePetSlot(battlePetSetBattleSlot.Slot)))
            slot->Pet = pet->PacketInfo;
}

void WorldSession::HandleBattlePetModifyName(WorldPackets::BattlePet::BattlePetModifyName& battlePetModifyName)
{
    GetBattlePetMgr()->ModifyName(battlePetModifyName.PetGuid, battlePetModifyName.Name, std::move(battlePetModifyName.DeclinedNames));
}

void WorldSession::HandleQueryBattlePetName(WorldPackets::BattlePet::QueryBattlePetName& queryBattlePetName)
{
    WorldPackets::BattlePet::QueryBattlePetNameResponse response;
    response.BattlePetID = queryBattlePetName.BattlePetID;

    Creature* summonedBattlePet = ObjectAccessor::GetCreatureOrPetOrVehicle(*_player, queryBattlePetName.UnitGUID);
    if (!summonedBattlePet || !summonedBattlePet->IsSummon())
    {
        SendPacket(response.Write());
        return;
    }

    response.CreatureID = summonedBattlePet->GetEntry();
    response.Timestamp = summonedBattlePet->GetBattlePetCompanionNameTimestamp();

    Unit* petOwner = summonedBattlePet->ToTempSummon()->GetSummonerUnit();
    if (!petOwner->IsPlayer())
    {
        SendPacket(response.Write());
        return;
    }

    BattlePets::BattlePet const* battlePet = petOwner->ToPlayer()->GetSession()->GetBattlePetMgr()->GetPet(queryBattlePetName.BattlePetID);
    if (!battlePet)
    {
        SendPacket(response.Write());
        return;
    }

    response.Name = battlePet->PacketInfo.Name;
    if (battlePet->DeclinedName)
    {
        response.HasDeclined = true;
        response.DeclinedNames = *battlePet->DeclinedName;
    }

    response.Allow = !response.Name.empty();

    SendPacket(response.Write());
}

void WorldSession::HandleBattlePetDeletePet(WorldPackets::BattlePet::BattlePetDeletePet& battlePetDeletePet)
{
    GetBattlePetMgr()->RemovePet(battlePetDeletePet.PetGuid);
}

void WorldSession::HandleBattlePetSetFlags(WorldPackets::BattlePet::BattlePetSetFlags& battlePetSetFlags)
{
    if (!GetBattlePetMgr()->HasJournalLock())
        return;

    if (BattlePets::BattlePet* pet = GetBattlePetMgr()->GetPet(battlePetSetFlags.PetGuid))
    {
        if (battlePetSetFlags.ControlType == BattlePets::FLAGS_CONTROL_TYPE_APPLY)
            pet->PacketInfo.Flags |= battlePetSetFlags.Flags;
        else // FLAGS_CONTROL_TYPE_REMOVE
            pet->PacketInfo.Flags &= ~battlePetSetFlags.Flags;

        if (pet->SaveInfo != BattlePets::BATTLE_PET_NEW)
            pet->SaveInfo = BattlePets::BATTLE_PET_CHANGED;
    }
}

void WorldSession::HandleBattlePetClearFanfare(WorldPackets::BattlePet::BattlePetClearFanfare& battlePetClearFanfare)
{
    GetBattlePetMgr()->ClearFanfare(battlePetClearFanfare.PetGuid);
}

void WorldSession::HandleCageBattlePet(WorldPackets::BattlePet::CageBattlePet& cageBattlePet)
{
    GetBattlePetMgr()->CageBattlePet(cageBattlePet.PetGuid);
}

void WorldSession::HandleBattlePetSummon(WorldPackets::BattlePet::BattlePetSummon& battlePetSummon)
{
    if (*_player->m_activePlayerData->SummonedBattlePetGUID != battlePetSummon.PetGuid)
        GetBattlePetMgr()->SummonPet(battlePetSummon.PetGuid);
    else
        GetBattlePetMgr()->DismissPet();
}

void WorldSession::HandleBattlePetUpdateNotify(WorldPackets::BattlePet::BattlePetUpdateNotify& battlePetUpdateNotify)
{
    GetBattlePetMgr()->UpdateBattlePetData(battlePetUpdateNotify.PetGuid);
}

void WorldSession::HandleBattlePetUpdateDisplayNotify(WorldPackets::BattlePet::BattlePetUpdateDisplayNotify& battlePetUpdateDisplayNotify)
{
    GetBattlePetMgr()->UpdateBattlePetData(battlePetUpdateDisplayNotify.PetGuid);
}

// ============================================================================
// Pet Battle Combat Handlers
// ============================================================================

static void BuildPetBattlePlayerUpdate(WorldPackets::BattlePet::PetBattlePlayerUpdateInfo& update,
    PetBattles::PetBattleTeamData const& team, bool isWildTeam, PetBattles::PetBattle const* battle, uint8 teamIdx)
{
    update.CharacterID = team.PlayerGUID;
    update.TrapAbilityID = team.TrapAbilityID;
    update.TrapStatus = battle ? battle->GetTrapStatus(teamIdx) : team.TrapStatus;
    update.RoundTimeSecs = PetBattles::PET_BATTLE_MAX_ROUND_TIME;
    update.FrontPet = team.FrontPetIndex;
    update.InputFlags = team.InputFlags;

    for (uint8 i = 0; i < team.PetCount; ++i)
    {
        PetBattles::PetBattlePetData const& petData = team.Pets[i];

        WorldPackets::BattlePet::PetBattlePetUpdateInfo petInfo;
        petInfo.BattlePetGUID = petData.BattlePetGUID;
        petInfo.SpeciesID = petData.Species;
        petInfo.CreatureID = petData.CreatureID;
        petInfo.DisplayID = petData.DisplayID;
        petInfo.Level = petData.Level;
        petInfo.Xp = petData.Xp;
        petInfo.CurHealth = petData.Health;
        petInfo.MaxHealth = petData.MaxHealth;
        petInfo.Power = petData.Power;
        petInfo.Speed = petData.Speed;
        petInfo.NpcTeamMemberID = isWildTeam ? petData.CreatureID : 0;
        petInfo.BreedQuality = petData.Quality;
        petInfo.Slot = static_cast<int8>(i);
        petInfo.CustomName = petData.CustomName;

        // Compute StatusFlags from pet state
        uint16 statusFlags = 0;
        if (petData.IsCaptured)
            statusFlags |= PetBattles::PET_BATTLE_PET_STATUS_TRAPPED;
        if (petData.IsStunned)
            statusFlags |= PetBattles::PET_BATTLE_PET_STATUS_STUNNED;
        if (petData.IsLockedByMultiTurn)
            statusFlags |= PetBattles::PET_BATTLE_PET_STATUS_SWAP_OUT_LOCKED;
        petInfo.StatusFlags = statusFlags;

        // Fill ability info
        for (uint8 j = 0; j < PetBattles::MAX_PET_BATTLE_ABILITIES; ++j)
        {
            if (petData.AbilityIDs[j] == 0)
                continue;

            WorldPackets::BattlePet::PetBattleAbilityInfo ability;
            ability.AbilityID = petData.AbilityIDs[j];
            ability.CooldownRemaining = petData.AbilityCooldowns[j];
            ability.LockdownRemaining = petData.AbilityLockdowns[j];
            ability.AbilityIndex = static_cast<int8>(j);
            ability.Pboid = i;
            petInfo.Abilities.push_back(ability);
        }

        update.Pets.push_back(std::move(petInfo));
    }
}

static void BuildPetBattleRoundPlayerData(WorldPackets::BattlePet::PetBattleRoundPlayerData& roundData,
    PetBattles::PetBattleTeamData const& team, PetBattles::PetBattle const* battle, uint8 teamIdx)
{
    roundData.NextInputFlags = team.InputFlags;
    roundData.NextTrapStatus = battle ? static_cast<int8>(battle->GetTrapStatus(teamIdx)) : static_cast<int8>(team.TrapStatus);
    roundData.RoundTimeSecs = PetBattles::PET_BATTLE_MAX_ROUND_TIME;

    for (uint8 i = 0; i < team.PetCount; ++i)
    {
        std::vector<WorldPackets::BattlePet::PetBattleCooldownInfo> petCds;
        for (uint8 j = 0; j < PetBattles::MAX_PET_BATTLE_ABILITIES; ++j)
        {
            if (team.Pets[i].AbilityIDs[j] == 0)
                continue;

            WorldPackets::BattlePet::PetBattleCooldownInfo cd;
            cd.AbilityID = team.Pets[i].AbilityIDs[j];
            cd.CooldownRemaining = team.Pets[i].AbilityCooldowns[j];
            cd.LockdownRemaining = team.Pets[i].AbilityLockdowns[j];
            petCds.push_back(cd);
        }
        roundData.PetCooldowns.push_back(std::move(petCds));
    }
}

static void BuildRoundEffects(std::vector<WorldPackets::BattlePet::PetBattleEffectInfo>& effectList,
    PetBattles::PetBattle const* battle)
{
    for (PetBattles::PetBattleRoundEffect const& roundEffect : battle->GetRoundEffects())
    {
        WorldPackets::BattlePet::PetBattleEffectInfo effect;
        effect.AbilityEffectID = roundEffect.AbilityEffectID;
        effect.Flags = roundEffect.Flags;
        effect.PetBattleEffectType = roundEffect.EffectType;
        effect.CasterPBOID = static_cast<int8>(roundEffect.SourceTeam * PetBattles::MAX_PET_BATTLE_TEAM_SIZE + roundEffect.SourcePet);

        WorldPackets::BattlePet::PetBattleEffectTargetInfo target;
        target.Type = roundEffect.EffectType;
        target.Petx = static_cast<uint8>(roundEffect.TargetTeam * PetBattles::MAX_PET_BATTLE_TEAM_SIZE + roundEffect.TargetPet);
        target.Param1 = roundEffect.Param1;
        target.Param2 = roundEffect.Param2;
        effect.Targets.push_back(target);

        effectList.push_back(std::move(effect));
    }
}

void WorldSession::HandlePetBattleRequestWild(WorldPackets::BattlePet::PetBattleRequestWild& petBattleRequestWild)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    // Validate player state
    if (!player->IsAlive())
    {
        WorldPackets::BattlePet::PetBattleRequestFailed failed;
        failed.Reason = PetBattles::PET_BATTLE_REQUEST_FAIL_NOT_WHILE_DEAD;
        SendPacket(failed.Write());
        return;
    }

    if (player->IsInCombat())
    {
        WorldPackets::BattlePet::PetBattleRequestFailed failed;
        failed.Reason = PetBattles::PET_BATTLE_REQUEST_FAIL_NOT_WHILE_IN_COMBAT;
        SendPacket(failed.Write());
        return;
    }

    if (sPetBattleMgr->IsPlayerInBattle(player->GetGUID()))
    {
        WorldPackets::BattlePet::PetBattleRequestFailed failed;
        failed.Reason = PetBattles::PET_BATTLE_REQUEST_FAIL_IN_BATTLE;
        SendPacket(failed.Write());
        return;
    }

    // Check that player has pets in battle slots
    BattlePets::BattlePetMgr* petMgr = GetBattlePetMgr();
    bool hasPet = false;
    for (uint8 i = 0; i < uint8(BattlePets::BattlePetSlot::Count); ++i)
    {
        WorldPackets::BattlePet::BattlePetSlot* slot = petMgr->GetSlot(BattlePets::BattlePetSlot(i));
        if (slot && !slot->Locked && !slot->Pet.Guid.IsEmpty())
        {
            BattlePets::BattlePet* pet = petMgr->GetPet(slot->Pet.Guid);
            if (pet && pet->PacketInfo.Health > 0)
            {
                hasPet = true;
                break;
            }
        }
    }

    if (!hasPet)
    {
        WorldPackets::BattlePet::PetBattleRequestFailed failed;
        failed.Reason = PetBattles::PET_BATTLE_REQUEST_FAIL_INVALID_LOADOUT_ALL_DEAD;
        SendPacket(failed.Write());
        return;
    }

    // Validate target creature
    Creature* creature = ObjectAccessor::GetCreature(*player, petBattleRequestWild.TargetGUID);
    if (!creature || !creature->IsWildBattlePet())
    {
        WorldPackets::BattlePet::PetBattleRequestFailed failed;
        failed.Reason = PetBattles::PET_BATTLE_REQUEST_FAIL_TARGET_INVALID;
        SendPacket(failed.Write());
        return;
    }

    // Create the battle
    PetBattles::PetBattle* battle = sPetBattleMgr->CreateWildBattle(player, petBattleRequestWild.TargetGUID);
    if (!battle)
    {
        WorldPackets::BattlePet::PetBattleRequestFailed failed;
        failed.Reason = PetBattles::PET_BATTLE_REQUEST_FAIL_IN_BATTLE;
        SendPacket(failed.Write());
        return;
    }

    // Send finalize location
    WorldPackets::BattlePet::PetBattleFinalizeLocation finalizeLocation;
    finalizeLocation.Location = player->GetPosition();
    SendPacket(finalizeLocation.Write());

    // Build and send initial update
    WorldPackets::BattlePet::PetBattleInitialUpdate initialUpdate;

    BuildPetBattlePlayerUpdate(initialUpdate.Players[0], battle->GetTeam(PetBattles::PET_BATTLE_TEAM_1), false, battle, PetBattles::PET_BATTLE_TEAM_1);
    BuildPetBattlePlayerUpdate(initialUpdate.Players[1], battle->GetTeam(PetBattles::PET_BATTLE_TEAM_2), true, battle, PetBattles::PET_BATTLE_TEAM_2);

    initialUpdate.CurRound = battle->GetCurrentRound();
    initialUpdate.CurPetBattleState = static_cast<int8>(battle->GetBattleState());
    initialUpdate.NpcCreatureID = creature->GetEntry();
    initialUpdate.NpcDisplayID = creature->GetDisplayId();
    initialUpdate.InitialWildPetGUID = petBattleRequestWild.TargetGUID;
    initialUpdate.IsPVP = false;
    initialUpdate.CanAwardXP = true;
    initialUpdate.WaitingForFrontPetsMaxSecs = 30;
    initialUpdate.PvpMaxRoundTime = PetBattles::PET_BATTLE_MAX_ROUND_TIME;

    SendPacket(initialUpdate.Write());

    // Start the battle
    battle->Start();

    // Auto-submit input for wild/NPC team using AI
    battle->GenerateWildTeamInput();

    TC_LOG_DEBUG("server.loading", "PetBattleHandler: Player {} started wild battle against {}",
        player->GetGUID().ToString(), petBattleRequestWild.TargetGUID.ToString());
}

void WorldSession::HandlePetBattleInput(WorldPackets::BattlePet::PetBattleInput& petBattleInput)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    PetBattles::PetBattle* battle = sPetBattleMgr->GetBattleByPlayer(player->GetGUID());
    if (!battle)
        return;

    // Determine which team this player is
    uint8 teamIdx = PetBattles::PET_BATTLE_TEAM_1;
    if (battle->GetTeam(PetBattles::PET_BATTLE_TEAM_2).PlayerGUID == player->GetGUID())
        teamIdx = PetBattles::PET_BATTLE_TEAM_2;

    battle->SubmitInput(teamIdx,
        PetBattles::PetBattleMoveType(petBattleInput.MoveType),
        petBattleInput.AbilityID,
        petBattleInput.NewFrontPetIndex);

    // Check if round should process
    if (battle->BothTeamsReady())
    {
        bool wasFirstRound = (battle->GetCurrentRound() == 0);
        battle->ProcessRound();

        if (battle->IsFinished())
        {
            // Send final round
            WorldPackets::BattlePet::PetBattleFinalRound finalRound;
            finalRound.CurRound = battle->GetCurrentRound();
            finalRound.NextPetBattleState = static_cast<uint8>(battle->GetBattleState());
            for (uint8 i = 0; i < PetBattles::MAX_PET_BATTLE_PLAYERS; ++i)
                BuildPetBattleRoundPlayerData(finalRound.Players[i], battle->GetTeam(i), battle, i);
            BuildRoundEffects(finalRound.Effects, battle);

            finalRound.Winners[battle->GetWinnerTeam()] = true;

            // Send to player team 1
            if (Player* p1 = battle->GetPlayerForTeam(PetBattles::PET_BATTLE_TEAM_1))
                p1->SendDirectMessage(finalRound.Write());
            // Send to player team 2 (PvP)
            if (Player* p2 = battle->GetPlayerForTeam(PetBattles::PET_BATTLE_TEAM_2))
                p2->SendDirectMessage(finalRound.Write());

            // Send finished
            WorldPackets::BattlePet::PetBattleFinished finished;
            if (Player* p1 = battle->GetPlayerForTeam(PetBattles::PET_BATTLE_TEAM_1))
                p1->SendDirectMessage(finished.Write());
            if (Player* p2 = battle->GetPlayerForTeam(PetBattles::PET_BATTLE_TEAM_2))
                p2->SendDirectMessage(finished.Write());

            // Sync battle pet health back to journal and auto-heal 50%
            for (uint8 t = 0; t < PetBattles::MAX_PET_BATTLE_PLAYERS; ++t)
            {
                Player* teamPlayer = battle->GetPlayerForTeam(t);
                if (!teamPlayer)
                    continue;

                BattlePets::BattlePetMgr* petMgr = teamPlayer->GetSession()->GetBattlePetMgr();
                PetBattles::PetBattleTeamData const& team = battle->GetTeam(t);
                for (uint8 p = 0; p < team.PetCount; ++p)
                {
                    if (!team.Pets[p].BattlePetGUID.IsEmpty())
                        petMgr->SyncBattlePetHealth(team.Pets[p].BattlePetGUID, team.Pets[p].Health);
                }
                petMgr->HealBattlePetsPct(50);
                petMgr->SendJournal();
            }
        }
        else
        {
            // Send round result
            if (wasFirstRound)
            {
                WorldPackets::BattlePet::PetBattleFirstRound firstRound;
                firstRound.CurRound = battle->GetCurrentRound();
                firstRound.NextPetBattleState = static_cast<uint8>(battle->GetBattleState());
                for (uint8 i = 0; i < PetBattles::MAX_PET_BATTLE_PLAYERS; ++i)
                    BuildPetBattleRoundPlayerData(firstRound.Players[i], battle->GetTeam(i), battle, i);
                BuildRoundEffects(firstRound.Effects, battle);

                if (Player* p1 = battle->GetPlayerForTeam(PetBattles::PET_BATTLE_TEAM_1))
                    p1->SendDirectMessage(firstRound.Write());
                if (Player* p2 = battle->GetPlayerForTeam(PetBattles::PET_BATTLE_TEAM_2))
                    p2->SendDirectMessage(firstRound.Write());
            }
            else
            {
                WorldPackets::BattlePet::PetBattleRoundResult roundResult;
                roundResult.CurRound = battle->GetCurrentRound();
                roundResult.NextPetBattleState = static_cast<uint8>(battle->GetBattleState());
                for (uint8 i = 0; i < PetBattles::MAX_PET_BATTLE_PLAYERS; ++i)
                    BuildPetBattleRoundPlayerData(roundResult.Players[i], battle->GetTeam(i), battle, i);
                BuildRoundEffects(roundResult.Effects, battle);

                if (Player* p1 = battle->GetPlayerForTeam(PetBattles::PET_BATTLE_TEAM_1))
                    p1->SendDirectMessage(roundResult.Write());
                if (Player* p2 = battle->GetPlayerForTeam(PetBattles::PET_BATTLE_TEAM_2))
                    p2->SendDirectMessage(roundResult.Write());
            }

            // Auto-submit AI team input for next round
            if (battle->GetBattleType() == PetBattles::PET_BATTLE_TYPE_PVE)
                battle->GenerateWildTeamInput();
            else if (battle->GetBattleType() == PetBattles::PET_BATTLE_TYPE_NPC)
                battle->GenerateNPCTeamInput();
        }
    }
}

void WorldSession::HandlePetBattleReplaceFrontPet(WorldPackets::BattlePet::PetBattleReplaceFrontPet& petBattleReplaceFrontPet)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    PetBattles::PetBattle* battle = sPetBattleMgr->GetBattleByPlayer(player->GetGUID());
    if (!battle)
        return;

    uint8 teamIdx = PetBattles::PET_BATTLE_TEAM_1;
    if (battle->GetTeam(PetBattles::PET_BATTLE_TEAM_2).PlayerGUID == player->GetGUID())
        teamIdx = PetBattles::PET_BATTLE_TEAM_2;

    battle->SubmitInput(teamIdx, PetBattles::PET_BATTLE_MOVE_SWAP, 0, petBattleReplaceFrontPet.FrontPetIndex);

    // If both teams have submitted (wild auto-submits), process
    if (battle->BothTeamsReady())
    {
        battle->ProcessRound();

        WorldPackets::BattlePet::PetBattleReplacementsMade replacements;
        replacements.CurRound = battle->GetCurrentRound();
        replacements.NextPetBattleState = static_cast<uint8>(battle->GetBattleState());
        for (uint8 i = 0; i < PetBattles::MAX_PET_BATTLE_PLAYERS; ++i)
            BuildPetBattleRoundPlayerData(replacements.Players[i], battle->GetTeam(i), battle, i);

        if (Player* p1 = battle->GetPlayerForTeam(PetBattles::PET_BATTLE_TEAM_1))
            p1->SendDirectMessage(replacements.Write());
        if (Player* p2 = battle->GetPlayerForTeam(PetBattles::PET_BATTLE_TEAM_2))
            p2->SendDirectMessage(replacements.Write());

        // Re-submit AI team for next round
        if (battle->GetBattleType() == PetBattles::PET_BATTLE_TYPE_PVE)
            battle->GenerateWildTeamInput();
        else if (battle->GetBattleType() == PetBattles::PET_BATTLE_TYPE_NPC)
            battle->GenerateNPCTeamInput();
    }
}

void WorldSession::HandlePetBattleQuitNotify(WorldPackets::BattlePet::PetBattleQuitNotify& /*petBattleQuitNotify*/)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    PetBattles::PetBattle* battle = sPetBattleMgr->GetBattleByPlayer(player->GetGUID());
    if (!battle)
        return;

    uint8 teamIdx = PetBattles::PET_BATTLE_TEAM_1;
    if (battle->GetTeam(PetBattles::PET_BATTLE_TEAM_2).PlayerGUID == player->GetGUID())
        teamIdx = PetBattles::PET_BATTLE_TEAM_2;

    battle->Forfeit(teamIdx);

    // Send final round to both players
    WorldPackets::BattlePet::PetBattleFinalRound finalRound;
    finalRound.CurRound = battle->GetCurrentRound();
    finalRound.NextPetBattleState = static_cast<uint8>(battle->GetBattleState());
    for (uint8 i = 0; i < PetBattles::MAX_PET_BATTLE_PLAYERS; ++i)
        BuildPetBattleRoundPlayerData(finalRound.Players[i], battle->GetTeam(i), battle, i);
    finalRound.Winners[battle->GetWinnerTeam()] = true;

    if (Player* p1 = battle->GetPlayerForTeam(PetBattles::PET_BATTLE_TEAM_1))
        p1->SendDirectMessage(finalRound.Write());
    if (Player* p2 = battle->GetPlayerForTeam(PetBattles::PET_BATTLE_TEAM_2))
        p2->SendDirectMessage(finalRound.Write());

    WorldPackets::BattlePet::PetBattleFinished finished;
    if (Player* p1 = battle->GetPlayerForTeam(PetBattles::PET_BATTLE_TEAM_1))
        p1->SendDirectMessage(finished.Write());
    if (Player* p2 = battle->GetPlayerForTeam(PetBattles::PET_BATTLE_TEAM_2))
        p2->SendDirectMessage(finished.Write());

    // Sync battle pet health back to journal and auto-heal 50%
    for (uint8 t = 0; t < PetBattles::MAX_PET_BATTLE_PLAYERS; ++t)
    {
        Player* teamPlayer = battle->GetPlayerForTeam(t);
        if (!teamPlayer)
            continue;

        BattlePets::BattlePetMgr* petMgr = teamPlayer->GetSession()->GetBattlePetMgr();
        PetBattles::PetBattleTeamData const& team = battle->GetTeam(t);
        for (uint8 p = 0; p < team.PetCount; ++p)
        {
            if (!team.Pets[p].BattlePetGUID.IsEmpty())
                petMgr->SyncBattlePetHealth(team.Pets[p].BattlePetGUID, team.Pets[p].Health);
        }
        petMgr->HealBattlePetsPct(50);
        petMgr->SendJournal();
    }
}

void WorldSession::HandlePetBattleFinalNotify(WorldPackets::BattlePet::PetBattleFinalNotify& /*petBattleFinalNotify*/)
{
    // Client acknowledges battle end - cleanup happens via PetBattleMgr::Update() removing finished battles
}

void WorldSession::HandlePetBattleRequestPVP(WorldPackets::BattlePet::PetBattleRequestPVP& petBattleRequestPVP)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    // Full validation
    if (!player->IsAlive())
    {
        WorldPackets::BattlePet::PetBattleRequestFailed failed;
        failed.Reason = PetBattles::PET_BATTLE_REQUEST_FAIL_NOT_WHILE_DEAD;
        SendPacket(failed.Write());
        return;
    }

    if (player->IsInCombat())
    {
        WorldPackets::BattlePet::PetBattleRequestFailed failed;
        failed.Reason = PetBattles::PET_BATTLE_REQUEST_FAIL_NOT_WHILE_IN_COMBAT;
        SendPacket(failed.Write());
        return;
    }

    if (player->IsFlying())
    {
        WorldPackets::BattlePet::PetBattleRequestFailed failed;
        failed.Reason = PetBattles::PET_BATTLE_REQUEST_FAIL_NOT_WHILE_FLYING;
        SendPacket(failed.Write());
        return;
    }

    if (sPetBattleMgr->IsPlayerInBattle(player->GetGUID()))
    {
        WorldPackets::BattlePet::PetBattleRequestFailed failed;
        failed.Reason = PetBattles::PET_BATTLE_REQUEST_FAIL_IN_BATTLE;
        SendPacket(failed.Write());
        return;
    }

    // Check player has alive pets
    BattlePets::BattlePetMgr* petMgr = GetBattlePetMgr();
    bool hasPet = false;
    for (uint8 i = 0; i < uint8(BattlePets::BattlePetSlot::Count); ++i)
    {
        WorldPackets::BattlePet::BattlePetSlot* slot = petMgr->GetSlot(BattlePets::BattlePetSlot(i));
        if (slot && !slot->Locked && !slot->Pet.Guid.IsEmpty())
        {
            BattlePets::BattlePet* pet = petMgr->GetPet(slot->Pet.Guid);
            if (pet && pet->PacketInfo.Health > 0)
            {
                hasPet = true;
                break;
            }
        }
    }

    if (!hasPet)
    {
        WorldPackets::BattlePet::PetBattleRequestFailed failed;
        failed.Reason = PetBattles::PET_BATTLE_REQUEST_FAIL_INVALID_LOADOUT_ALL_DEAD;
        SendPacket(failed.Write());
        return;
    }

    Player* target = ObjectAccessor::FindPlayer(petBattleRequestPVP.TargetGUID);
    if (!target)
    {
        WorldPackets::BattlePet::PetBattleRequestFailed failed;
        failed.Reason = PetBattles::PET_BATTLE_REQUEST_FAIL_TARGET_INVALID;
        SendPacket(failed.Write());
        return;
    }

    if (sPetBattleMgr->IsPlayerInBattle(target->GetGUID()))
    {
        WorldPackets::BattlePet::PetBattleRequestFailed failed;
        failed.Reason = PetBattles::PET_BATTLE_REQUEST_FAIL_OPPONENT_NOT_AVAILABLE;
        SendPacket(failed.Write());
        return;
    }

    // Send PVP challenge to target
    WorldPackets::BattlePet::PetBattlePVPChallenge challenge;
    challenge.ChallengerGUID = player->GetGUID();
    challenge.BattleOrigin = player->GetPosition();
    target->SendDirectMessage(challenge.Write());
}

void WorldSession::HandleJoinPetBattleQueue(WorldPackets::BattlePet::JoinPetBattleQueue& /*joinPetBattleQueue*/)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    if (sPetBattleMgr->IsPlayerInBattle(player->GetGUID()))
    {
        WorldPackets::BattlePet::PetBattleQueueStatus status;
        status.Status = 0;
        SendPacket(status.Write());
        return;
    }

    sPetBattleMgr->JoinQueue(player->GetGUID());

    WorldPackets::BattlePet::PetBattleQueueStatus status;
    status.Status = 1; // Queued
    SendPacket(status.Write());
}

void WorldSession::HandleLeavePetBattleQueue(WorldPackets::BattlePet::LeavePetBattleQueue& /*leavePetBattleQueue*/)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    sPetBattleMgr->LeaveQueue(player->GetGUID());

    WorldPackets::BattlePet::PetBattleQueueStatus status;
    status.Status = 0; // Not in queue
    SendPacket(status.Write());
}

void WorldSession::HandlePetBattleQueueProposeMatchResult(WorldPackets::BattlePet::PetBattleQueueProposeMatchResult& petBattleQueueProposeMatchResult)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    sPetBattleMgr->HandleProposalResult(player->GetGUID(), petBattleQueueProposeMatchResult.Accepted);
}

void WorldSession::HandlePetBattleRequestUpdate(WorldPackets::BattlePet::PetBattleRequestUpdate& petBattleRequestUpdate)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    // This packet is used for PvP challenge accept/decline
    if (!petBattleRequestUpdate.Canceled)
    {
        // Accept PvP challenge
        Player* challenger = ObjectAccessor::FindPlayer(petBattleRequestUpdate.TargetGUID);
        if (!challenger)
            return;

        PetBattles::PetBattle* battle = sPetBattleMgr->CreatePvPBattle(challenger, player);
        if (!battle)
        {
            WorldPackets::BattlePet::PetBattleRequestFailed failed;
            failed.Reason = PetBattles::PET_BATTLE_REQUEST_FAIL_IN_BATTLE;
            SendPacket(failed.Write());
            return;
        }

        // Send initial update to both players
        WorldPackets::BattlePet::PetBattleInitialUpdate initialUpdate;
        BuildPetBattlePlayerUpdate(initialUpdate.Players[0], battle->GetTeam(PetBattles::PET_BATTLE_TEAM_1), false, battle, PetBattles::PET_BATTLE_TEAM_1);
        BuildPetBattlePlayerUpdate(initialUpdate.Players[1], battle->GetTeam(PetBattles::PET_BATTLE_TEAM_2), false, battle, PetBattles::PET_BATTLE_TEAM_2);
        initialUpdate.CurRound = battle->GetCurrentRound();
        initialUpdate.CurPetBattleState = static_cast<int8>(battle->GetBattleState());
        initialUpdate.IsPVP = true;
        initialUpdate.CanAwardXP = false;
        initialUpdate.ForfeitPenalty = 10;
        initialUpdate.WaitingForFrontPetsMaxSecs = 30;
        initialUpdate.PvpMaxRoundTime = PetBattles::PET_BATTLE_MAX_ROUND_TIME;

        challenger->SendDirectMessage(initialUpdate.Write());
        player->SendDirectMessage(initialUpdate.Write());

        battle->Start();
    }
    else
    {
        // Decline PvP challenge
        Player* challenger = ObjectAccessor::FindPlayer(petBattleRequestUpdate.TargetGUID);
        if (challenger)
        {
            WorldPackets::BattlePet::PetBattleRequestFailed failed;
            failed.Reason = PetBattles::PET_BATTLE_REQUEST_FAIL_DECLINED;
            challenger->SendDirectMessage(failed.Write());
        }
    }
}

void WorldSession::HandlePetBattleScriptErrorNotify(WorldPackets::BattlePet::PetBattleScriptErrorNotify& /*petBattleScriptErrorNotify*/)
{
    // Client reports script error - log only
    TC_LOG_DEBUG("server.loading", "PetBattleHandler: Client reported pet battle script error");
}

void WorldSession::HandlePetBattleWildLocationFail(WorldPackets::BattlePet::PetBattleWildLocationFail& /*petBattleWildLocationFail*/)
{
    // Client reports wild battle location failure - log only
    TC_LOG_DEBUG("server.loading", "PetBattleHandler: Client reported wild pet battle location failure");
}
