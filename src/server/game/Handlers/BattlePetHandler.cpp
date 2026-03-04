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
#include "MotionMaster.h"
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
    BattlePets::BattlePetMgr* petMgr = GetBattlePetMgr();

    BattlePets::BattlePet* pet = petMgr->GetPet(battlePetSetBattleSlot.PetGuid);
    if (!pet)
        return;

    WorldPackets::BattlePet::BattlePetSlot* targetSlot = petMgr->GetSlot(BattlePets::BattlePetSlot(battlePetSetBattleSlot.Slot));
    if (!targetSlot || targetSlot->Locked)
        return;

    // Find which slot this pet currently occupies (if any)
    WorldPackets::BattlePet::BattlePetSlot* sourceSlot = nullptr;
    for (uint8 i = 0; i < uint8(BattlePets::BattlePetSlot::Count); ++i)
    {
        WorldPackets::BattlePet::BattlePetSlot* slot = petMgr->GetSlot(BattlePets::BattlePetSlot(i));
        if (slot && slot->Pet.Guid == battlePetSetBattleSlot.PetGuid)
        {
            sourceSlot = slot;
            break;
        }
    }

    if (sourceSlot == targetSlot)
        return; // Already in that slot

    // Swap: move whatever is in the target slot to the source slot
    if (sourceSlot)
    {
        sourceSlot->Pet = targetSlot->Pet;
    }

    // Place the pet in the target slot
    targetSlot->Pet = pet->PacketInfo;
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
    update.RoundTimeSecs = 0; // Initial update uses 0; round-level timer is in PvpMaxRoundTime
    update.FrontPet = team.FrontPetIndex;
    update.InputFlags = team.InputFlags;

    for (uint8 i = 0; i < team.PetCount; ++i)
    {
        PetBattles::PetBattlePetData const& petData = team.Pets[i];

        WorldPackets::BattlePet::PetBattlePetUpdateInfo petInfo;
        petInfo.BattlePetGUID = petData.BattlePetGUID;
        petInfo.SpeciesID = petData.Species;
        petInfo.DisplayID = isWildTeam ? 0 : petData.DisplayID;
        petInfo.Level = petData.Level;
        petInfo.Xp = petData.Xp;
        petInfo.CurHealth = petData.Health;
        petInfo.MaxHealth = petData.MaxHealth;
        petInfo.Power = petData.Power;
        petInfo.Speed = petData.Speed;
        petInfo.NpcTeamMemberID = petData.NpcTeamMemberID;
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
            ability.Pboid = teamIdx * PetBattles::MAX_PET_BATTLE_TEAM_SIZE + i;
            petInfo.Abilities.push_back(ability);
        }

        // Populate auras currently active on this pet
        for (PetBattles::PetBattleAura const& aura : petData.Auras)
        {
            WorldPackets::BattlePet::PetBattleAuraInfo auraInfo;
            auraInfo.AbilityID = aura.AbilityID;
            auraInfo.InstanceID = aura.AuraInstanceID;
            auraInfo.RoundsRemaining = aura.RemainingRounds;
            auraInfo.CurrentRound = aura.CurrentRound;
            auraInfo.CasterPBOID = aura.CasterTeam * PetBattles::MAX_PET_BATTLE_TEAM_SIZE + aura.CasterPet;
            petInfo.Auras.push_back(auraInfo);
        }

        // Populate States with base breed+species stats from DB2 (retail format)
        petInfo.States.push_back({ BattlePets::STATE_STAT_POWER, petData.BasePower });
        petInfo.States.push_back({ BattlePets::STATE_STAT_STAMINA, petData.BaseStamina });
        petInfo.States.push_back({ BattlePets::STATE_STAT_SPEED, petData.BaseSpeed });
        petInfo.States.push_back({ 40, 5 }); // CritChance = 5%
        if (petData.PetType >= 0 && petData.PetType < PetBattles::PET_TYPE_COUNT)
            petInfo.States.push_back({ uint32(44 + petData.PetType), 1 }); // Family passive flag

        update.Pets.push_back(std::move(petInfo));
    }
}

static void BuildPetBattleRoundPlayerData(WorldPackets::BattlePet::PetBattleRoundPlayerData& roundData,
    PetBattles::PetBattleTeamData const& team, PetBattles::PetBattle const* battle, uint8 teamIdx)
{
    roundData.NextInputFlags = team.InputFlags;
    roundData.NextTrapStatus = battle ? static_cast<int8>(battle->GetTrapStatus(teamIdx)) : static_cast<int8>(team.TrapStatus);
    // RoundTimeSecs is 0 for PvE/NPC battles; only PvP uses a timer
    roundData.RoundTimeSecs = (battle && (battle->GetBattleType() == PetBattles::PET_BATTLE_TYPE_PVP || battle->GetBattleType() == PetBattles::PET_BATTLE_TYPE_LFPB))
        ? PetBattles::PET_BATTLE_MAX_ROUND_TIME : 0;
}

static void BuildRoundCooldowns(std::vector<WorldPackets::BattlePet::PetBattleCooldownInfo>& cooldowns,
    PetBattles::PetBattle const* battle)
{
    for (uint8 t = 0; t < PetBattles::MAX_PET_BATTLE_PLAYERS; ++t)
    {
        PetBattles::PetBattleTeamData const& team = battle->GetTeam(t);
        for (uint8 i = 0; i < team.PetCount; ++i)
        {
            for (uint8 j = 0; j < PetBattles::MAX_PET_BATTLE_ABILITIES; ++j)
            {
                if (team.Pets[i].AbilityIDs[j] == 0)
                    continue;

                WorldPackets::BattlePet::PetBattleCooldownInfo cd;
                cd.AbilityID = team.Pets[i].AbilityIDs[j];
                cd.CooldownRemaining = team.Pets[i].AbilityCooldowns[j];
                cd.LockdownRemaining = team.Pets[i].AbilityLockdowns[j];
                cd.AbilityIndex = static_cast<int8>(j);
                cd.Pboid = t * PetBattles::MAX_PET_BATTLE_TEAM_SIZE + i;
                cooldowns.push_back(cd);
            }
        }
    }
}

static void BuildPetXDied(std::vector<int8>& petXDied, PetBattles::PetBattle const* battle)
{
    for (uint8 t = 0; t < PetBattles::MAX_PET_BATTLE_PLAYERS; ++t)
    {
        PetBattles::PetBattleTeamData const& team = battle->GetTeam(t);
        for (uint8 i = 0; i < team.PetCount; ++i)
        {
            if (battle->WasPetKilledThisRound(t, i))
                petXDied.push_back(static_cast<int8>(t * PetBattles::MAX_PET_BATTLE_TEAM_SIZE + i));
        }
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
        effect.SourceAuraInstanceID = 0;
        effect.TurnInstanceID = 0;
        // Wire offset 12 is the PetBattleEffectType — client switches on this to process effects
        // (SetHealth=0, AuraApply=1, PetSwap=4, SetState=6, etc.), NOT a sequential index
        effect.EffectIndex = roundEffect.EffectType;
        effect.CasterPBOID = static_cast<int32>(roundEffect.SourceTeam * PetBattles::MAX_PET_BATTLE_TEAM_SIZE + roundEffect.SourcePet);
        effect.StackDepth = 0;

        WorldPackets::BattlePet::PetBattleEffectTargetInfo target;
        target.Remaining = static_cast<int32>(roundEffect.TargetTeam * PetBattles::MAX_PET_BATTLE_TEAM_SIZE + roundEffect.TargetPet);

        // Map effect type to target type and variable-length params
        // Target types: 0=none, 1=aura(4 i32), 2=state(2 i32), 3=health(1 i32),
        //               4=stat(1 i32), 5=trigger(1 i32), 6=cooldown(3 i32), 7=broadcast(1 i32)
        switch (roundEffect.EffectType)
        {
            case PetBattles::PET_BATTLE_EFFECT_SET_HEALTH:
            case PetBattles::PET_BATTLE_EFFECT_SET_MAX_HEALTH:
                target.Type = 3; // Health
                target.Params.push_back(roundEffect.Param1);
                break;
            case PetBattles::PET_BATTLE_EFFECT_AURA_APPLY:
            case PetBattles::PET_BATTLE_EFFECT_AURA_CANCEL:
            case PetBattles::PET_BATTLE_EFFECT_AURA_CHANGE:
            {
                target.Type = 1; // Aura: 4 params
                target.Params.push_back(roundEffect.Param1); // AuraInstanceID
                target.Params.push_back(roundEffect.Param2); // AuraAbilityID
                // Use pre-captured values from creation time (Param3/Param4)
                // instead of live aura lookup — TickAuras may have decremented
                // or removed the aura before this serialization runs.
                target.Params.push_back(roundEffect.Param3); // RoundsRemaining
                target.Params.push_back(roundEffect.Param4); // CurrentRound
                break;
            }
            case PetBattles::PET_BATTLE_EFFECT_SET_STATE:
                target.Type = 2; // State
                target.Params.push_back(roundEffect.Param1); // StateID
                target.Params.push_back(roundEffect.Param2); // StateValue
                break;
            case PetBattles::PET_BATTLE_EFFECT_STATUS_CHANGE:
            case PetBattles::PET_BATTLE_EFFECT_SET_SPEED:
            case PetBattles::PET_BATTLE_EFFECT_SET_POWER:
                target.Type = 4; // NewStatValue
                target.Params.push_back(roundEffect.Param1);
                break;
            case PetBattles::PET_BATTLE_EFFECT_TRIGGER_ABILITY:
                target.Type = 5; // TriggerAbilityID
                target.Params.push_back(roundEffect.Param1);
                break;
            case PetBattles::PET_BATTLE_EFFECT_ABILITY_CHANGE:
                target.Type = 6; // Cooldown
                target.Params.push_back(roundEffect.Param1); // ChangedAbilityID
                target.Params.push_back(roundEffect.Param2); // CooldownRemaining
                target.Params.push_back(0);                   // LockdownRemaining
                break;
            case PetBattles::PET_BATTLE_EFFECT_NPC_EMOTE:
                target.Type = 7; // BroadcastTextID
                target.Params.push_back(roundEffect.Param1);
                break;
            default:
                target.Type = 0; // No data
                break;
        }

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

    // Validate that the creature has a valid BattlePetSpecies entry (Species=0 crashes client)
    if (!BattlePets::BattlePetMgr::GetBattlePetSpeciesByCreature(creature->GetEntry()))
    {
        WorldPackets::BattlePet::PetBattleRequestFailed failed;
        failed.Reason = PetBattles::PET_BATTLE_REQUEST_FAIL_TARGET_INVALID;
        SendPacket(failed.Write());
        TC_LOG_ERROR("server.loading", "PetBattle: Creature {} (entry {}) has no BattlePetSpecies entry, rejecting battle",
            creature->GetGUID().ToString(), creature->GetEntry());
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

    // Freeze the wild creature during battle (stop movement, make non-interactable)
    creature->SetUnitFlag(UNIT_FLAG_PACIFIED);
    creature->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE);
    creature->GetMotionMaster()->MoveIdle();

    // Send finalize location with OK result
    // BattleOrigin is the midpoint between player and creature (matching retail)
    WorldPackets::BattlePet::PetBattleFinalizeLocation finalizeLocation;
    finalizeLocation.Location.LocationResult = PetBattles::PET_BATTLE_REQUEST_FAIL_OK;
    Position midpoint;
    midpoint.m_positionX = (player->GetPositionX() + creature->GetPositionX()) / 2.0f;
    midpoint.m_positionY = (player->GetPositionY() + creature->GetPositionY()) / 2.0f;
    midpoint.m_positionZ = (player->GetPositionZ() + creature->GetPositionZ()) / 2.0f;
    finalizeLocation.Location.BattleOrigin = midpoint;
    finalizeLocation.Location.BattleFacing = player->GetAbsoluteAngle(creature);
    finalizeLocation.Location.PlayerPositions[0] = player->GetPosition();
    finalizeLocation.Location.PlayerPositions[1] = creature->GetPosition();
    SendPacket(finalizeLocation.Write());

    // Build and send initial update
    WorldPackets::BattlePet::PetBattleInitialUpdate initialUpdate;

    BuildPetBattlePlayerUpdate(initialUpdate.Players[0], battle->GetTeam(PetBattles::PET_BATTLE_TEAM_1), false, battle, PetBattles::PET_BATTLE_TEAM_1);
    BuildPetBattlePlayerUpdate(initialUpdate.Players[1], battle->GetTeam(PetBattles::PET_BATTLE_TEAM_2), true, battle, PetBattles::PET_BATTLE_TEAM_2);

    initialUpdate.CurRound = battle->GetCurrentRound();
    initialUpdate.CurPetBattleState = static_cast<int8>(battle->GetBattleState());
    initialUpdate.NpcCreatureID = 0; // Wild battles have no NPC trainer; creature identified by InitialWildPetGUID
    initialUpdate.NpcDisplayID = 0;
    initialUpdate.InitialWildPetGUID = petBattleRequestWild.TargetGUID;
    initialUpdate.IsPVP = false;
    initialUpdate.CanAwardXP = true;
    initialUpdate.WaitingForFrontPetsMaxSecs = 30;
    initialUpdate.PvpMaxRoundTime = PetBattles::PET_BATTLE_MAX_ROUND_TIME;

    WorldPacket const* pkt = initialUpdate.Write();

    // Dump all critical fields for debugging
    for (uint8 t = 0; t < 2; ++t)
    {
        auto const& pu = initialUpdate.Players[t];
        TC_LOG_ERROR("server.loading", "PetBattle InitialUpdate Player[{}]: CharID={} TrapAbilityID={} TrapStatus={} RoundTimeSecs={} FrontPet={} InputFlags={} PetCount={}",
            t, pu.CharacterID.ToString(), pu.TrapAbilityID, pu.TrapStatus, pu.RoundTimeSecs, pu.FrontPet, pu.InputFlags, pu.Pets.size());
        for (uint8 p = 0; p < pu.Pets.size(); ++p)
        {
            auto const& pet = pu.Pets[p];
            TC_LOG_ERROR("server.loading", "  Pet[{}]: GUID={} Species={} Display={} Collar={} Lvl={} Xp={} HP={}/{} Pow={} Spd={} NpcTM={} Quality={} Status={} Slot={} Abilities={} Auras={} States={} Name='{}'",
                p, pet.BattlePetGUID.ToString(), pet.SpeciesID, pet.DisplayID, pet.CollarID, pet.Level, pet.Xp,
                pet.CurHealth, pet.MaxHealth, pet.Power, pet.Speed, pet.NpcTeamMemberID,
                pet.BreedQuality, pet.StatusFlags, pet.Slot, pet.Abilities.size(), pet.Auras.size(), pet.States.size(), pet.CustomName);
            for (uint8 a = 0; a < pet.Abilities.size(); ++a)
                TC_LOG_ERROR("server.loading", "    Ability[{}]: ID={} CD={} LD={} Idx={} Pboid={}",
                    a, pet.Abilities[a].AbilityID, pet.Abilities[a].CooldownRemaining, pet.Abilities[a].LockdownRemaining, pet.Abilities[a].AbilityIndex, pet.Abilities[a].Pboid);
            for (uint8 s = 0; s < pet.States.size(); ++s)
                TC_LOG_ERROR("server.loading", "    State[{}]: ID={} Val={}",
                    s, pet.States[s].StateID, pet.States[s].StateValue);
        }
    }
    TC_LOG_ERROR("server.loading", "PetBattle InitialUpdate: CurRound={} State={} NpcCreature={} NpcDisplay={} WildGUID={} IsPVP={} CanAwardXP={} PktSize={}",
        initialUpdate.CurRound, initialUpdate.CurPetBattleState, initialUpdate.NpcCreatureID, initialUpdate.NpcDisplayID,
        initialUpdate.InitialWildPetGUID.ToString(), initialUpdate.IsPVP, initialUpdate.CanAwardXP, pkt->size());

    SendPacket(pkt);

    // Start the battle (sets state to ROUND_IN_PROGRESS, round=0)
    battle->Start();

    // Send SMSG_PET_BATTLE_FIRST_ROUND to unlock abilities for the client
    // Retail sends this ~1 second after INITIAL_UPDATE; we send it immediately since
    // we don't have intro animations. This packet tells the client abilities are available.
    {
        WorldPackets::BattlePet::PetBattleFirstRound firstRound;
        firstRound.CurRound = 0;
        firstRound.NextPetBattleState = static_cast<int8>(PetBattles::PET_BATTLE_STATE_ROUND_IN_PROGRESS);

        // Unlock abilities for both teams (NextInputFlags=0 means no restrictions)
        for (uint8 i = 0; i < PetBattles::MAX_PET_BATTLE_PLAYERS; ++i)
        {
            firstRound.Players[i].NextInputFlags = 0;
            firstRound.Players[i].NextTrapStatus = static_cast<int8>(battle->GetTrapStatus(i));
            firstRound.Players[i].RoundTimeSecs = 0;
        }
        // No effects, cooldowns, or deaths for the initial first-round packet

        SendPacket(firstRound.Write());
    }

    // NOTE: Do NOT call GenerateWildTeamInput() here. The wild team AI submits
    // its input when the player submits theirs (in HandlePetBattleInput).

    TC_LOG_DEBUG("server.loading", "PetBattleHandler: Player {} started wild battle against {}",
        player->GetGUID().ToString(), petBattleRequestWild.TargetGUID.ToString());
}

void WorldSession::StartNPCPetBattle(Creature* trainer)
{
    Player* player = GetPlayer();
    if (!player || !trainer)
        return;

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

    // Check that player has alive pets in battle slots
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

    // Validate that NPC has a team configured
    if (!sPetBattleMgr->GetNPCTeam(trainer->GetEntry()))
    {
        WorldPackets::BattlePet::PetBattleRequestFailed failed;
        failed.Reason = PetBattles::PET_BATTLE_REQUEST_FAIL_TARGET_INVALID;
        SendPacket(failed.Write());
        TC_LOG_ERROR("server.loading", "PetBattle: NPC trainer {} (entry {}) has no pet team configured",
            trainer->GetGUID().ToString(), trainer->GetEntry());
        return;
    }

    // Create the NPC battle
    PetBattles::PetBattle* battle = sPetBattleMgr->CreateNPCBattle(player, trainer);
    if (!battle)
    {
        WorldPackets::BattlePet::PetBattleRequestFailed failed;
        failed.Reason = PetBattles::PET_BATTLE_REQUEST_FAIL_IN_BATTLE;
        SendPacket(failed.Write());
        return;
    }

    // Stop NPC movement during battle
    trainer->GetMotionMaster()->MoveIdle();

    // Send finalize location with OK result
    WorldPackets::BattlePet::PetBattleFinalizeLocation finalizeLocation;
    finalizeLocation.Location.LocationResult = PetBattles::PET_BATTLE_REQUEST_FAIL_OK;
    Position midpoint;
    midpoint.m_positionX = (player->GetPositionX() + trainer->GetPositionX()) / 2.0f;
    midpoint.m_positionY = (player->GetPositionY() + trainer->GetPositionY()) / 2.0f;
    midpoint.m_positionZ = (player->GetPositionZ() + trainer->GetPositionZ()) / 2.0f;
    finalizeLocation.Location.BattleOrigin = midpoint;
    finalizeLocation.Location.BattleFacing = player->GetAbsoluteAngle(trainer);
    finalizeLocation.Location.PlayerPositions[0] = player->GetPosition();
    finalizeLocation.Location.PlayerPositions[1] = trainer->GetPosition();
    SendPacket(finalizeLocation.Write());

    // Build and send initial update
    WorldPackets::BattlePet::PetBattleInitialUpdate initialUpdate;

    BuildPetBattlePlayerUpdate(initialUpdate.Players[0], battle->GetTeam(PetBattles::PET_BATTLE_TEAM_1), false, battle, PetBattles::PET_BATTLE_TEAM_1);
    BuildPetBattlePlayerUpdate(initialUpdate.Players[1], battle->GetTeam(PetBattles::PET_BATTLE_TEAM_2), false, battle, PetBattles::PET_BATTLE_TEAM_2);

    initialUpdate.CurRound = battle->GetCurrentRound();
    initialUpdate.CurPetBattleState = static_cast<int8>(battle->GetBattleState());
    initialUpdate.NpcCreatureID = trainer->GetEntry();
    initialUpdate.NpcDisplayID = trainer->GetDisplayId();
    initialUpdate.InitialWildPetGUID = ObjectGuid::Empty;
    initialUpdate.IsPVP = false;
    initialUpdate.CanAwardXP = true;
    initialUpdate.WaitingForFrontPetsMaxSecs = 30;
    initialUpdate.PvpMaxRoundTime = PetBattles::PET_BATTLE_MAX_ROUND_TIME;

    SendPacket(initialUpdate.Write());

    // Start the battle
    battle->Start();

    // Send first round to unlock abilities
    {
        WorldPackets::BattlePet::PetBattleFirstRound firstRound;
        firstRound.CurRound = 0;
        firstRound.NextPetBattleState = static_cast<int8>(PetBattles::PET_BATTLE_STATE_ROUND_IN_PROGRESS);

        for (uint8 i = 0; i < PetBattles::MAX_PET_BATTLE_PLAYERS; ++i)
        {
            firstRound.Players[i].NextInputFlags = 0;
            firstRound.Players[i].NextTrapStatus = static_cast<int8>(battle->GetTrapStatus(i));
            firstRound.Players[i].RoundTimeSecs = 0;
        }

        SendPacket(firstRound.Write());
    }

    TC_LOG_DEBUG("server.loading", "PetBattleHandler: Player {} started NPC battle against trainer {} (entry {})",
        player->GetGUID().ToString(), trainer->GetGUID().ToString(), trainer->GetEntry());
}

void WorldSession::HandlePetBattleInput(WorldPackets::BattlePet::PetBattleInput& petBattleInput)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    PetBattles::PetBattle* battle = sPetBattleMgr->GetBattleByPlayer(player->GetGUID());
    if (!battle)
        return;

    // Client sends MoveType=FinalRoundOk while waiting for FinalNotify — ignore it
    if (battle->IsFinalRound())
        return;

    // Determine which team this player is
    uint8 teamIdx = PetBattles::PET_BATTLE_TEAM_1;
    if (battle->GetTeam(PetBattles::PET_BATTLE_TEAM_2).PlayerGUID == player->GetGUID())
        teamIdx = PetBattles::PET_BATTLE_TEAM_2;

    battle->SubmitInput(teamIdx,
        PetBattles::PetBattleMoveType(petBattleInput.MoveType),
        petBattleInput.AbilityID,
        petBattleInput.NewFrontPetIndex);

    // Auto-submit AI team input now that the player has committed
    if (battle->GetBattleType() == PetBattles::PET_BATTLE_TYPE_PVE)
        battle->GenerateWildTeamInput();
    else if (battle->GetBattleType() == PetBattles::PET_BATTLE_TYPE_NPC)
        battle->GenerateNPCTeamInput();

    // Check if round should process
    if (battle->BothTeamsReady())
    {
        battle->ProcessRound();

        // Always send RoundResult with effects (including death effects)
        // so the client plays animations. FinalRound is sent separately
        // after a delay by PetBattle::Update() when the battle is ending.
        {
            WorldPackets::BattlePet::PetBattleRoundResult roundResult;
            roundResult.CurRound = battle->GetCurrentRound();
            roundResult.NextPetBattleState = static_cast<int8>(battle->GetBattleState());
            for (uint8 i = 0; i < PetBattles::MAX_PET_BATTLE_PLAYERS; ++i)
                BuildPetBattleRoundPlayerData(roundResult.Players[i], battle->GetTeam(i), battle, i);
            BuildRoundEffects(roundResult.Effects, battle);
            BuildRoundCooldowns(roundResult.Cooldowns, battle);
            BuildPetXDied(roundResult.PetXDied, battle);

            TC_LOG_ERROR("server.loading", "PetBattle ROUND_RESULT: Round={} State={} FinalRound={} Effects={} Cooldowns={} Deaths={} P0Flags={} P1Flags={}",
                roundResult.CurRound, roundResult.NextPetBattleState,
                battle->IsFinalRound(),
                roundResult.Effects.size(), roundResult.Cooldowns.size(), roundResult.PetXDied.size(),
                roundResult.Players[0].NextInputFlags, roundResult.Players[1].NextInputFlags);

            // Dump each effect for debugging
            for (std::size_t e = 0; e < roundResult.Effects.size(); ++e)
            {
                auto const& eff = roundResult.Effects[e];
                TC_LOG_ERROR("server.loading", "  Effect[{}]: AbilEffID={} Flags=0x{:X} Idx={} CasterPBOID={} StackDepth={} Targets={}",
                    e, eff.AbilityEffectID, eff.Flags, eff.EffectIndex, eff.CasterPBOID, eff.StackDepth, eff.Targets.size());
                for (std::size_t tgt = 0; tgt < eff.Targets.size(); ++tgt)
                {
                    auto const& t = eff.Targets[tgt];
                    std::string paramStr;
                    for (int32 p : t.Params)
                        paramStr += std::to_string(p) + " ";
                    TC_LOG_ERROR("server.loading", "    Target[{}]: Type={} Remaining(PBOID)={} Params=[{}]",
                        tgt, t.Type, t.Remaining, paramStr);
                }
            }

            if (Player* p1 = battle->GetPlayerForTeam(PetBattles::PET_BATTLE_TEAM_1))
                p1->SendDirectMessage(roundResult.Write());
            if (Player* p2 = battle->GetPlayerForTeam(PetBattles::PET_BATTLE_TEAM_2))
                p2->SendDirectMessage(roundResult.Write());
        }

        // FinalRound packet is now sent after a delay by PetBattle::Update()
        // to allow the client time to play death/capture animations first
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

    // Validate pet index
    int8 newPetIdx = petBattleReplaceFrontPet.FrontPetIndex;
    PetBattles::PetBattleTeamData& team = battle->GetTeam(teamIdx);

    TC_LOG_ERROR("server.loading", "PetBattle ReplaceFrontPet: team={} newPetIdx={} petCount={} battleState={} needsSwap={}",
        teamIdx, newPetIdx, team.PetCount, int(battle->GetBattleState()), battle->NeedsFrontPetSwap(teamIdx));

    if (newPetIdx < 0 || newPetIdx >= team.PetCount)
    {
        TC_LOG_ERROR("server.loading", "PetBattle ReplaceFrontPet: REJECTED - invalid index");
        return;
    }
    if (!team.Pets[newPetIdx].IsAlive())
    {
        TC_LOG_ERROR("server.loading", "PetBattle ReplaceFrontPet: REJECTED - pet {} is dead (HP={})",
            newPetIdx, team.Pets[newPetIdx].Health);
        return;
    }

    TC_LOG_ERROR("server.loading", "PetBattle ReplaceFrontPet: Swapping team {} front pet {} -> {}",
        teamIdx, team.FrontPetIndex, newPetIdx);

    // Set the new front pet and transition state back to ROUND_IN_PROGRESS
    team.FrontPetIndex = newPetIdx;
    team.HasInputThisRound = false;
    battle->ClearNeedsFrontPetSwap(teamIdx);
    battle->SetBattleState(PetBattles::PET_BATTLE_STATE_ROUND_IN_PROGRESS);

    // Send REPLACEMENTS_MADE to acknowledge the swap
    WorldPackets::BattlePet::PetBattleReplacementsMade replacements;
    replacements.CurRound = battle->GetCurrentRound();
    replacements.NextPetBattleState = static_cast<int8>(PetBattles::PET_BATTLE_STATE_ROUND_IN_PROGRESS);
    for (uint8 i = 0; i < PetBattles::MAX_PET_BATTLE_PLAYERS; ++i)
    {
        replacements.Players[i].NextInputFlags = 0; // Abilities unlocked for next round
        replacements.Players[i].NextTrapStatus = static_cast<int8>(battle->GetTrapStatus(i));
        replacements.Players[i].RoundTimeSecs = 0;
    }

    // Include a PetSwap effect so client updates the active pet and ability bar
    WorldPackets::BattlePet::PetBattleEffectInfo swapEffect;
    swapEffect.AbilityEffectID = 0;
    swapEffect.Flags = 0;
    swapEffect.SourceAuraInstanceID = 0;
    swapEffect.TurnInstanceID = 0;
    swapEffect.EffectIndex = PetBattles::PET_BATTLE_EFFECT_PET_SWAP; // Wire offset 12 = EffectType, client case 4
    swapEffect.CasterPBOID = static_cast<int32>(teamIdx * PetBattles::MAX_PET_BATTLE_TEAM_SIZE + newPetIdx);
    swapEffect.StackDepth = 0;
    WorldPackets::BattlePet::PetBattleEffectTargetInfo swapTarget;
    swapTarget.Type = 0; // No extra params needed for PetSwap
    swapTarget.Remaining = static_cast<int32>(teamIdx * PetBattles::MAX_PET_BATTLE_TEAM_SIZE + newPetIdx);
    swapEffect.Targets.push_back(swapTarget);
    replacements.Effects.push_back(std::move(swapEffect));

    if (Player* p1 = battle->GetPlayerForTeam(PetBattles::PET_BATTLE_TEAM_1))
        p1->SendDirectMessage(replacements.Write());
    if (Player* p2 = battle->GetPlayerForTeam(PetBattles::PET_BATTLE_TEAM_2))
        p2->SendDirectMessage(replacements.Write());
}

void WorldSession::HandlePetBattleQuitNotify(WorldPackets::BattlePet::PetBattleQuitNotify& /*petBattleQuitNotify*/)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    PetBattles::PetBattle* battle = sPetBattleMgr->GetBattleByPlayer(player->GetGUID());
    if (!battle)
        return;

    uint32 battleID = battle->GetBattleID();

    uint8 teamIdx = PetBattles::PET_BATTLE_TEAM_1;
    if (battle->GetTeam(PetBattles::PET_BATTLE_TEAM_2).PlayerGUID == player->GetGUID())
        teamIdx = PetBattles::PET_BATTLE_TEAM_2;

    battle->Forfeit(teamIdx);

    // Forfeit sends FinalRound immediately (no death animation delay needed)
    battle->SendFinalRoundPacket(true);

    // Forfeit completes immediately (sends Finished packet, syncs health, sends journal)
    battle->CompleteBattle();

    // Remove the battle from tracking maps so player can start a new one
    sPetBattleMgr->RemoveBattle(battleID);
}

void WorldSession::HandlePetBattleFinalNotify(WorldPackets::BattlePet::PetBattleFinalNotify& /*petBattleFinalNotify*/)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    PetBattles::PetBattle* battle = sPetBattleMgr->GetBattleByPlayer(player->GetGUID());
    if (!battle || !battle->IsFinalRound())
        return;

    // Ignore if FinalRound packet hasn't been sent yet (death animation still playing)
    if (battle->HasPendingFinishDelay())
        return;

    // Transition FINAL_ROUND → FINISHED (sends Finished packet, syncs health, sends journal)
    battle->CompleteBattle();

    // Remove the battle from tracking maps so player can start a new one
    sPetBattleMgr->RemoveBattle(battle->GetBattleID());
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
    challenge.Location.LocationResult = 0;
    challenge.Location.BattleOrigin = player->GetPosition();
    challenge.Location.BattleFacing = player->GetAbsoluteAngle(target);
    challenge.Location.PlayerPositions[0] = player->GetPosition();
    challenge.Location.PlayerPositions[1] = target->GetPosition();
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
